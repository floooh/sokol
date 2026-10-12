/*
    LLM maintained.

    d3d11_mock.c -- runtime side of the D3D11 mock library.

    Objects are heap-allocated with a refcount that starts at 1 and is
    decremented by Release(). Map() hands out a scratch buffer sized to the
    resource, so sokol_gfx can round-trip data without hitting UB.

    Not thread-safe -- this is intended for the sokol-gfx unit-test loop,
    which runs on a single thread.
*/
#include "d3d11_mock.h"

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

/* ---- shared object header ------------------------------------------------
   Every mock resource is allocated as a single block whose first field is a
   vtbl pointer -- matching the layout the sokol_gfx.h wrappers assume. The
   rest of the fields are internal bookkeeping used by the mock. */
typedef enum {
    MOCK_KIND_DEVICE = 1,
    MOCK_KIND_CONTEXT,
    MOCK_KIND_BUFFER,
    MOCK_KIND_TEXTURE2D,
    MOCK_KIND_TEXTURE3D,
    MOCK_KIND_SRV,
    MOCK_KIND_UAV,
    MOCK_KIND_RTV,
    MOCK_KIND_DSV,
    MOCK_KIND_SAMPLER,
    MOCK_KIND_INPUT_LAYOUT,
    MOCK_KIND_RASTERIZER_STATE,
    MOCK_KIND_DEPTH_STENCIL_STATE,
    MOCK_KIND_BLEND_STATE,
    MOCK_KIND_VERTEX_SHADER,
    MOCK_KIND_PIXEL_SHADER,
    MOCK_KIND_COMPUTE_SHADER,
    MOCK_KIND_BLOB
} mock_kind_t;

typedef struct mock_obj_s {
    const void* lpVtbl;
    ULONG refcount;
    mock_kind_t kind;
    /* view resources track the resource they were created from */
    struct mock_obj_s* view_resource;
    /* buffers/textures store size info for Map() */
    size_t size_bytes;
    UINT row_pitch;
    UINT depth_pitch;
    /* blob payload */
    void* blob_data;
    SIZE_T blob_size;
    /* per-context scratch buffer for Map/Unmap */
    void* map_scratch;
    size_t map_scratch_size;
} mock_obj_t;

/* live-object accounting for leak checks */
static int live_object_count = 0;

/* fault-injection counters */
static int fail_next_create_n = 0;
static int fail_create_skip_n = 0;
static int fail_next_compile_n = 0;
static bool fail_dll_load = false;
static int fail_next_map_n = 0;

/* ---- call log ------------------------------------------------------------ */
static d3d11_mock_call_t* call_log = NULL;
static int call_log_num = 0;
static int call_log_cap = 0;

static d3d11_mock_call_t* mock_record(d3d11_mock_call_kind_t kind) {
    if (call_log_num >= call_log_cap) {
        int new_cap = call_log_cap ? (call_log_cap * 2) : 256;
        d3d11_mock_call_t* new_log = (d3d11_mock_call_t*)realloc(call_log, (size_t)new_cap * sizeof(d3d11_mock_call_t));
        if (!new_log) {
            abort();
        }
        call_log = new_log;
        call_log_cap = new_cap;
    }
    d3d11_mock_call_t* call = &call_log[call_log_num++];
    memset(call, 0, sizeof(d3d11_mock_call_t));
    call->kind = kind;
    call->result = S_OK;
    return call;
}

static void mock_snapshot(d3d11_mock_call_t* call, const void* data, size_t size) {
    if (data && (size > 0)) {
        call->data_size = (size < D3D11_MOCK_DATA_SNAPSHOT_SIZE) ? size : D3D11_MOCK_DATA_SNAPSHOT_SIZE;
        memcpy(call->data, data, call->data_size);
    }
}

/* record a Set*(StartSlot, NumViews, ppViews) style call, keeping the first
   D3D11_MOCK_MAX_RECORDED_PTRS pointers */
static d3d11_mock_call_t* mock_record_ptrs(d3d11_mock_call_kind_t kind, UINT start, UINT num, const void* const* ptrs) {
    d3d11_mock_call_t* call = mock_record(kind);
    call->args[0] = start;
    call->args[1] = num;
    if (ptrs) {
        call->dst = ptrs[0];
        for (UINT i = 0; (i < num) && (i < D3D11_MOCK_MAX_RECORDED_PTRS); i++) {
            call->ptrs[i] = ptrs[i];
        }
    }
    return call;
}

/* Consume one Create* fault slot; returns true if this call should fail. */
static bool mock_consume_create_fault(void) {
    if ((fail_next_create_n > 0) && (fail_create_skip_n > 0)) {
        fail_create_skip_n--;
        return false;
    }
    if (fail_next_create_n > 0) {
        fail_next_create_n--;
        return true;
    }
    return false;
}

/* keep a linked list of live objects so d3d11_mock_reset() can free them all */
typedef struct mock_obj_node_s {
    mock_obj_t* obj;
    struct mock_obj_node_s* next;
} mock_obj_node_t;
static mock_obj_node_t* live_head = NULL;

/* forward declares of the singleton vtbls, populated below */
static const struct ID3D11DeviceVtbl device_vtbl;
static const struct ID3D11DeviceContextVtbl context_vtbl;
static const struct ID3D11BufferVtbl buffer_vtbl;
static const struct ID3D11Texture2DVtbl texture2d_vtbl;
static const struct ID3D11Texture3DVtbl texture3d_vtbl;
static const struct ID3D11ShaderResourceViewVtbl srv_vtbl;
static const struct ID3D11UnorderedAccessViewVtbl uav_vtbl;
static const struct ID3D11RenderTargetViewVtbl rtv_vtbl;
static const struct ID3D11DepthStencilViewVtbl dsv_vtbl;
static const struct ID3D11SamplerStateVtbl sampler_vtbl;
static const struct ID3D11InputLayoutVtbl input_layout_vtbl;
static const struct ID3D11RasterizerStateVtbl rasterizer_vtbl;
static const struct ID3D11DepthStencilStateVtbl depth_stencil_vtbl;
static const struct ID3D11BlendStateVtbl blend_vtbl;
static const struct ID3D11VertexShaderVtbl vs_vtbl;
static const struct ID3D11PixelShaderVtbl ps_vtbl;
static const struct ID3D11ComputeShaderVtbl cs_vtbl;
static const struct ID3D10BlobVtbl blob_vtbl;

/* ---- object lifecycle ---------------------------------------------------- */
static mock_obj_t* mock_alloc(mock_kind_t kind, const void* vtbl) {
    mock_obj_t* obj = (mock_obj_t*)calloc(1, sizeof(mock_obj_t));
    obj->lpVtbl = vtbl;
    obj->refcount = 1;
    obj->kind = kind;
    mock_obj_node_t* node = (mock_obj_node_t*)calloc(1, sizeof(mock_obj_node_t));
    node->obj = obj;
    node->next = live_head;
    live_head = node;
    live_object_count++;
    return obj;
}

static void mock_free_payload(mock_obj_t* obj) {
    if (obj->blob_data) { free(obj->blob_data); obj->blob_data = NULL; }
    if (obj->map_scratch) { free(obj->map_scratch); obj->map_scratch = NULL; }
}

static void mock_unlink(mock_obj_t* obj) {
    mock_obj_node_t** cur = &live_head;
    while (*cur) {
        if ((*cur)->obj == obj) {
            mock_obj_node_t* dead = *cur;
            *cur = dead->next;
            free(dead);
            return;
        }
        cur = &(*cur)->next;
    }
}

static ULONG mock_addref(void* self) {
    mock_obj_t* obj = (mock_obj_t*)self;
    obj->refcount++;
    return obj->refcount;
}

static ULONG mock_release(void* self) {
    mock_obj_t* obj = (mock_obj_t*)self;
    obj->refcount--;
    ULONG rc = obj->refcount;
    if (rc == 0) {
        mock_free_payload(obj);
        mock_unlink(obj);
        live_object_count--;
        free(obj);
    }
    return rc;
}

static HRESULT mock_setprivatedata(void* self, REFGUID guid, UINT size, const void* data) {
    (void)self; (void)guid; (void)size; (void)data;
    return S_OK;
}

/* ---- ID3D11View::GetResource -------------------------------------------- */
static void mock_view_getresource(void* self, ID3D11Resource** ppResource) {
    mock_obj_t* view = (mock_obj_t*)self;
    mock_obj_t* res = view->view_resource;
    if (res) { res->refcount++; }
    *ppResource = (ID3D11Resource*)res;
}

/* ---- ID3D11Device methods ---------------------------------------------- */
static HRESULT dev_CheckFormatSupport(ID3D11Device* self, DXGI_FORMAT fmt, UINT* pOut) {
    (void)self;
    /* Report broad support so sokol picks up most formats. Depth-stencil is
       reported only for depth-typed formats -- sokol infers "is depth" from
       that cap and would misclassify color formats otherwise. */
    UINT caps = D3D11_FORMAT_SUPPORT_TEXTURE2D
              | D3D11_FORMAT_SUPPORT_SHADER_SAMPLE
              | D3D11_FORMAT_SUPPORT_RENDER_TARGET
              | D3D11_FORMAT_SUPPORT_BLENDABLE
              | D3D11_FORMAT_SUPPORT_MULTISAMPLE_RENDERTARGET
              | D3D11_FORMAT_SUPPORT_TYPED_UNORDERED_ACCESS_VIEW;
    switch (fmt) {
        case DXGI_FORMAT_D32_FLOAT:
        case DXGI_FORMAT_D32_FLOAT_S8X24_UINT:
            caps |= D3D11_FORMAT_SUPPORT_DEPTH_STENCIL;
            break;
        default:
            break;
    }
    *pOut = caps;
    return S_OK;
}

static D3D_FEATURE_LEVEL dev_GetFeatureLevel(ID3D11Device* self) {
    (void)self;
    return D3D_FEATURE_LEVEL_11_1;
}

static HRESULT dev_CreateBuffer(ID3D11Device* self, const D3D11_BUFFER_DESC* pDesc, const D3D11_SUBRESOURCE_DATA* pInit, ID3D11Buffer** ppOut) {
    (void)self;
    d3d11_mock_call_t* call = mock_record(D3D11_MOCK_CALL_CREATE_BUFFER);
    if (pDesc) { call->buffer_desc = *pDesc; }
    if (pInit) {
        call->has_init_data = true;
        call->src_data = pInit->pSysMem;
        mock_snapshot(call, pInit->pSysMem, pDesc ? pDesc->ByteWidth : 0);
    }
    if (mock_consume_create_fault()) { *ppOut = NULL; call->result = E_FAIL; return E_FAIL; }
    mock_obj_t* obj = mock_alloc(MOCK_KIND_BUFFER, &buffer_vtbl);
    obj->size_bytes = pDesc ? pDesc->ByteWidth : 0;
    call->dst = obj;
    *ppOut = (ID3D11Buffer*)obj;
    return S_OK;
}

static HRESULT dev_CreateTexture2D(ID3D11Device* self, const D3D11_TEXTURE2D_DESC* pDesc, const D3D11_SUBRESOURCE_DATA* pInit, ID3D11Texture2D** ppOut) {
    (void)self;
    d3d11_mock_call_t* call = mock_record(D3D11_MOCK_CALL_CREATE_TEXTURE2D);
    if (pDesc) { call->tex2d_desc = *pDesc; }
    if (pInit) {
        call->has_init_data = true;
        call->src_data = pInit->pSysMem;
        call->src_row_pitch = pInit->SysMemPitch;
        call->src_depth_pitch = pInit->SysMemSlicePitch;
    }
    if (mock_consume_create_fault()) { *ppOut = NULL; call->result = E_FAIL; return E_FAIL; }
    mock_obj_t* obj = mock_alloc(MOCK_KIND_TEXTURE2D, &texture2d_vtbl);
    /* Reserve a generous per-subresource scratch: worst case width*height*16 */
    size_t w = pDesc ? pDesc->Width  : 1;
    size_t h = pDesc ? pDesc->Height : 1;
    obj->row_pitch = (UINT)(w * 16u);
    obj->depth_pitch = (UINT)(obj->row_pitch * h);
    obj->size_bytes = obj->depth_pitch;
    call->dst = obj;
    *ppOut = (ID3D11Texture2D*)obj;
    return S_OK;
}

static HRESULT dev_CreateTexture3D(ID3D11Device* self, const D3D11_TEXTURE3D_DESC* pDesc, const D3D11_SUBRESOURCE_DATA* pInit, ID3D11Texture3D** ppOut) {
    (void)self;
    d3d11_mock_call_t* call = mock_record(D3D11_MOCK_CALL_CREATE_TEXTURE3D);
    if (pDesc) { call->tex3d_desc = *pDesc; }
    if (pInit) {
        call->has_init_data = true;
        call->src_data = pInit->pSysMem;
        call->src_row_pitch = pInit->SysMemPitch;
        call->src_depth_pitch = pInit->SysMemSlicePitch;
    }
    if (mock_consume_create_fault()) { *ppOut = NULL; call->result = E_FAIL; return E_FAIL; }
    mock_obj_t* obj = mock_alloc(MOCK_KIND_TEXTURE3D, &texture3d_vtbl);
    size_t w = pDesc ? pDesc->Width  : 1;
    size_t h = pDesc ? pDesc->Height : 1;
    size_t d = pDesc ? pDesc->Depth  : 1;
    obj->row_pitch = (UINT)(w * 16u);
    obj->depth_pitch = (UINT)(obj->row_pitch * h);
    obj->size_bytes = obj->depth_pitch * d;
    call->dst = obj;
    *ppOut = (ID3D11Texture3D*)obj;
    return S_OK;
}

static HRESULT dev_CreateShaderResourceView(ID3D11Device* self, ID3D11Resource* pRes, const D3D11_SHADER_RESOURCE_VIEW_DESC* pDesc, ID3D11ShaderResourceView** ppOut) {
    (void)self;
    d3d11_mock_call_t* call = mock_record(D3D11_MOCK_CALL_CREATE_SRV);
    call->src = pRes;
    if (pDesc) { call->has_view_desc = true; call->srv_desc = *pDesc; }
    if (mock_consume_create_fault()) { *ppOut = NULL; call->result = E_FAIL; return E_FAIL; }
    mock_obj_t* obj = mock_alloc(MOCK_KIND_SRV, &srv_vtbl);
    obj->view_resource = (mock_obj_t*)pRes;
    call->dst = obj;
    *ppOut = (ID3D11ShaderResourceView*)obj;
    return S_OK;
}

static HRESULT dev_CreateUnorderedAccessView(ID3D11Device* self, ID3D11Resource* pRes, const D3D11_UNORDERED_ACCESS_VIEW_DESC* pDesc, ID3D11UnorderedAccessView** ppOut) {
    (void)self;
    d3d11_mock_call_t* call = mock_record(D3D11_MOCK_CALL_CREATE_UAV);
    call->src = pRes;
    if (pDesc) { call->has_view_desc = true; call->uav_desc = *pDesc; }
    if (mock_consume_create_fault()) { *ppOut = NULL; call->result = E_FAIL; return E_FAIL; }
    mock_obj_t* obj = mock_alloc(MOCK_KIND_UAV, &uav_vtbl);
    obj->view_resource = (mock_obj_t*)pRes;
    call->dst = obj;
    *ppOut = (ID3D11UnorderedAccessView*)obj;
    return S_OK;
}

static HRESULT dev_CreateRenderTargetView(ID3D11Device* self, ID3D11Resource* pRes, const D3D11_RENDER_TARGET_VIEW_DESC* pDesc, ID3D11RenderTargetView** ppOut) {
    (void)self;
    d3d11_mock_call_t* call = mock_record(D3D11_MOCK_CALL_CREATE_RTV);
    call->src = pRes;
    if (pDesc) { call->has_view_desc = true; call->rtv_desc = *pDesc; }
    if (mock_consume_create_fault()) { *ppOut = NULL; call->result = E_FAIL; return E_FAIL; }
    mock_obj_t* obj = mock_alloc(MOCK_KIND_RTV, &rtv_vtbl);
    obj->view_resource = (mock_obj_t*)pRes;
    call->dst = obj;
    *ppOut = (ID3D11RenderTargetView*)obj;
    return S_OK;
}

static HRESULT dev_CreateDepthStencilView(ID3D11Device* self, ID3D11Resource* pRes, const D3D11_DEPTH_STENCIL_VIEW_DESC* pDesc, ID3D11DepthStencilView** ppOut) {
    (void)self;
    d3d11_mock_call_t* call = mock_record(D3D11_MOCK_CALL_CREATE_DSV);
    call->src = pRes;
    if (pDesc) { call->has_view_desc = true; call->dsv_desc = *pDesc; }
    if (mock_consume_create_fault()) { *ppOut = NULL; call->result = E_FAIL; return E_FAIL; }
    mock_obj_t* obj = mock_alloc(MOCK_KIND_DSV, &dsv_vtbl);
    obj->view_resource = (mock_obj_t*)pRes;
    call->dst = obj;
    *ppOut = (ID3D11DepthStencilView*)obj;
    return S_OK;
}

static HRESULT dev_CreateSamplerState(ID3D11Device* self, const D3D11_SAMPLER_DESC* pDesc, ID3D11SamplerState** ppOut) {
    (void)self;
    d3d11_mock_call_t* call = mock_record(D3D11_MOCK_CALL_CREATE_SAMPLER_STATE);
    if (pDesc) { call->sampler_desc = *pDesc; }
    if (mock_consume_create_fault()) { *ppOut = NULL; call->result = E_FAIL; return E_FAIL; }
    *ppOut = (ID3D11SamplerState*)mock_alloc(MOCK_KIND_SAMPLER, &sampler_vtbl);
    call->dst = *ppOut;
    return S_OK;
}

static HRESULT dev_CreateVertexShader(ID3D11Device* self, const void* code, SIZE_T len, ID3D11ClassLinkage* link, ID3D11VertexShader** ppOut) {
    (void)self; (void)code; (void)len; (void)link;
    if (mock_consume_create_fault()) { *ppOut = NULL; return E_FAIL; }
    *ppOut = (ID3D11VertexShader*)mock_alloc(MOCK_KIND_VERTEX_SHADER, &vs_vtbl);
    return S_OK;
}

static HRESULT dev_CreatePixelShader(ID3D11Device* self, const void* code, SIZE_T len, ID3D11ClassLinkage* link, ID3D11PixelShader** ppOut) {
    (void)self; (void)code; (void)len; (void)link;
    if (mock_consume_create_fault()) { *ppOut = NULL; return E_FAIL; }
    *ppOut = (ID3D11PixelShader*)mock_alloc(MOCK_KIND_PIXEL_SHADER, &ps_vtbl);
    return S_OK;
}

static HRESULT dev_CreateComputeShader(ID3D11Device* self, const void* code, SIZE_T len, ID3D11ClassLinkage* link, ID3D11ComputeShader** ppOut) {
    (void)self; (void)code; (void)len; (void)link;
    if (mock_consume_create_fault()) { *ppOut = NULL; return E_FAIL; }
    *ppOut = (ID3D11ComputeShader*)mock_alloc(MOCK_KIND_COMPUTE_SHADER, &cs_vtbl);
    return S_OK;
}

static HRESULT dev_CreateInputLayout(ID3D11Device* self, const D3D11_INPUT_ELEMENT_DESC* pElems, UINT numElems, const void* code, SIZE_T len, ID3D11InputLayout** ppOut) {
    (void)self; (void)code; (void)len;
    d3d11_mock_call_t* call = mock_record(D3D11_MOCK_CALL_CREATE_INPUT_LAYOUT);
    call->args[0] = numElems;
    for (UINT i = 0; pElems && (i < numElems) && (i < D3D11_MOCK_MAX_INPUT_ELEMENTS); i++) {
        call->input_elements[i] = pElems[i];
    }
    if (mock_consume_create_fault()) { *ppOut = NULL; call->result = E_FAIL; return E_FAIL; }
    *ppOut = (ID3D11InputLayout*)mock_alloc(MOCK_KIND_INPUT_LAYOUT, &input_layout_vtbl);
    call->dst = *ppOut;
    return S_OK;
}

static HRESULT dev_CreateRasterizerState(ID3D11Device* self, const D3D11_RASTERIZER_DESC* pDesc, ID3D11RasterizerState** ppOut) {
    (void)self;
    d3d11_mock_call_t* call = mock_record(D3D11_MOCK_CALL_CREATE_RASTERIZER_STATE);
    if (pDesc) { call->rasterizer_desc = *pDesc; }
    if (mock_consume_create_fault()) { *ppOut = NULL; call->result = E_FAIL; return E_FAIL; }
    *ppOut = (ID3D11RasterizerState*)mock_alloc(MOCK_KIND_RASTERIZER_STATE, &rasterizer_vtbl);
    call->dst = *ppOut;
    return S_OK;
}

static HRESULT dev_CreateDepthStencilState(ID3D11Device* self, const D3D11_DEPTH_STENCIL_DESC* pDesc, ID3D11DepthStencilState** ppOut) {
    (void)self;
    d3d11_mock_call_t* call = mock_record(D3D11_MOCK_CALL_CREATE_DEPTH_STENCIL_STATE);
    if (pDesc) { call->depth_stencil_desc = *pDesc; }
    if (mock_consume_create_fault()) { *ppOut = NULL; call->result = E_FAIL; return E_FAIL; }
    *ppOut = (ID3D11DepthStencilState*)mock_alloc(MOCK_KIND_DEPTH_STENCIL_STATE, &depth_stencil_vtbl);
    call->dst = *ppOut;
    return S_OK;
}

static HRESULT dev_CreateBlendState(ID3D11Device* self, const D3D11_BLEND_DESC* pDesc, ID3D11BlendState** ppOut) {
    (void)self;
    d3d11_mock_call_t* call = mock_record(D3D11_MOCK_CALL_CREATE_BLEND_STATE);
    if (pDesc) { call->blend_desc = *pDesc; }
    if (mock_consume_create_fault()) { *ppOut = NULL; call->result = E_FAIL; return E_FAIL; }
    *ppOut = (ID3D11BlendState*)mock_alloc(MOCK_KIND_BLEND_STATE, &blend_vtbl);
    call->dst = *ppOut;
    return S_OK;
}

/* ---- ID3D11DeviceContext methods --------------------------------------- */
static void ctx_ClearState(ID3D11DeviceContext* self) {
    (void)self;
    mock_record(D3D11_MOCK_CALL_CLEAR_STATE);
}
static void ctx_OMSetRenderTargets(ID3D11DeviceContext* self, UINT n, ID3D11RenderTargetView* const* rtvs, ID3D11DepthStencilView* dsv) {
    (void)self;
    d3d11_mock_call_t* call = mock_record(D3D11_MOCK_CALL_OM_SET_RENDER_TARGETS);
    call->dst = (rtvs && (n > 0)) ? (const void*)rtvs[0] : NULL;
    call->src = dsv;
    call->args[0] = n;
}
static void ctx_OMSetRenderTargetsAndUnorderedAccessViews(ID3D11DeviceContext* self, UINT num_rtvs, ID3D11RenderTargetView* const* rtvs, ID3D11DepthStencilView* dsv, UINT uav_start, UINT num_uavs, ID3D11UnorderedAccessView* const* uavs, const UINT* counts) {
    (void)self; (void)rtvs; (void)dsv; (void)counts;
    d3d11_mock_call_t* call = mock_record_ptrs(D3D11_MOCK_CALL_OM_SET_RENDER_TARGETS_AND_UNORDERED_ACCESS_VIEWS, uav_start, num_uavs, (const void* const*)uavs);
    call->args[2] = num_rtvs;
}
static void ctx_RSSetState(ID3D11DeviceContext* self, ID3D11RasterizerState* rs)                                                                                                              { (void)self; (void)rs; }
static void ctx_OMSetDepthStencilState(ID3D11DeviceContext* self, ID3D11DepthStencilState* dss, UINT ref)                                                                                     { (void)self; (void)dss; (void)ref; }
static void ctx_OMSetBlendState(ID3D11DeviceContext* self, ID3D11BlendState* bs, const FLOAT bf[4], UINT mask)                                                                                { (void)self; (void)bs; (void)bf; (void)mask; }
static void ctx_IASetVertexBuffers(ID3D11DeviceContext* self, UINT s, UINT n, ID3D11Buffer* const* bufs, const UINT* strides, const UINT* offsets) {
    (void)self;
    d3d11_mock_call_t* call = mock_record(D3D11_MOCK_CALL_IA_SET_VERTEX_BUFFERS);
    call->dst = (bufs && (n > 0)) ? (const void*)bufs[0] : NULL;
    call->args[0] = s;
    call->args[1] = n;
    call->args[2] = (strides && (n > 0)) ? strides[0] : 0;
    call->args[3] = (offsets && (n > 0)) ? offsets[0] : 0;
}
static void ctx_IASetIndexBuffer(ID3D11DeviceContext* self, ID3D11Buffer* buf, DXGI_FORMAT fmt, UINT off) {
    (void)self;
    d3d11_mock_call_t* call = mock_record(D3D11_MOCK_CALL_IA_SET_INDEX_BUFFER);
    call->dst = buf;
    call->args[0] = (UINT)fmt;
    call->args[1] = off;
}
static void ctx_IASetInputLayout(ID3D11DeviceContext* self, ID3D11InputLayout* il)                                                                                                            { (void)self; (void)il; }
static void ctx_VSSetShader(ID3D11DeviceContext* self, ID3D11VertexShader* sh, ID3D11ClassInstance* const* ci, UINT n)                                                                        { (void)self; (void)sh; (void)ci; (void)n; }
static void ctx_PSSetShader(ID3D11DeviceContext* self, ID3D11PixelShader* sh, ID3D11ClassInstance* const* ci, UINT n)                                                                         { (void)self; (void)sh; (void)ci; (void)n; }
static void ctx_CSSetShader(ID3D11DeviceContext* self, ID3D11ComputeShader* sh, ID3D11ClassInstance* const* ci, UINT n)                                                                       { (void)self; (void)sh; (void)ci; (void)n; }
static void ctx_VSSetConstantBuffers(ID3D11DeviceContext* self, UINT s, UINT n, ID3D11Buffer* const* bufs)                                                                                    { (void)self; (void)s; (void)n; (void)bufs; }
static void ctx_PSSetConstantBuffers(ID3D11DeviceContext* self, UINT s, UINT n, ID3D11Buffer* const* bufs)                                                                                    { (void)self; (void)s; (void)n; (void)bufs; }
static void ctx_CSSetConstantBuffers(ID3D11DeviceContext* self, UINT s, UINT n, ID3D11Buffer* const* bufs)                                                                                    { (void)self; (void)s; (void)n; (void)bufs; }
static void ctx_VSSetShaderResources(ID3D11DeviceContext* self, UINT s, UINT n, ID3D11ShaderResourceView* const* srvs) {
    (void)self;
    mock_record_ptrs(D3D11_MOCK_CALL_VS_SET_SHADER_RESOURCES, s, n, (const void* const*)srvs);
}
static void ctx_PSSetShaderResources(ID3D11DeviceContext* self, UINT s, UINT n, ID3D11ShaderResourceView* const* srvs) {
    (void)self;
    mock_record_ptrs(D3D11_MOCK_CALL_PS_SET_SHADER_RESOURCES, s, n, (const void* const*)srvs);
}
static void ctx_CSSetShaderResources(ID3D11DeviceContext* self, UINT s, UINT n, ID3D11ShaderResourceView* const* srvs) {
    (void)self;
    mock_record_ptrs(D3D11_MOCK_CALL_CS_SET_SHADER_RESOURCES, s, n, (const void* const*)srvs);
}
static void ctx_VSSetSamplers(ID3D11DeviceContext* self, UINT s, UINT n, ID3D11SamplerState* const* smps) {
    (void)self;
    mock_record_ptrs(D3D11_MOCK_CALL_VS_SET_SAMPLERS, s, n, (const void* const*)smps);
}
static void ctx_PSSetSamplers(ID3D11DeviceContext* self, UINT s, UINT n, ID3D11SamplerState* const* smps) {
    (void)self;
    mock_record_ptrs(D3D11_MOCK_CALL_PS_SET_SAMPLERS, s, n, (const void* const*)smps);
}
static void ctx_CSSetSamplers(ID3D11DeviceContext* self, UINT s, UINT n, ID3D11SamplerState* const* smps) {
    (void)self;
    mock_record_ptrs(D3D11_MOCK_CALL_CS_SET_SAMPLERS, s, n, (const void* const*)smps);
}
static void ctx_CSSetUnorderedAccessViews(ID3D11DeviceContext* self, UINT s, UINT n, ID3D11UnorderedAccessView* const* uavs, const UINT* counts) {
    (void)self; (void)counts;
    mock_record_ptrs(D3D11_MOCK_CALL_CS_SET_UNORDERED_ACCESS_VIEWS, s, n, (const void* const*)uavs);
}
static void ctx_RSSetViewports(ID3D11DeviceContext* self, UINT n, const D3D11_VIEWPORT* vps)                                                                                                  { (void)self; (void)n; (void)vps; }
static void ctx_RSSetScissorRects(ID3D11DeviceContext* self, UINT n, const D3D11_RECT* rects)                                                                                                 { (void)self; (void)n; (void)rects; }
static void ctx_ClearRenderTargetView(ID3D11DeviceContext* self, ID3D11RenderTargetView* rtv, const FLOAT c[4]) {
    (void)self;
    d3d11_mock_call_t* call = mock_record(D3D11_MOCK_CALL_CLEAR_RENDER_TARGET_VIEW);
    call->dst = rtv;
    mock_snapshot(call, c, 4 * sizeof(FLOAT));
}
static void ctx_ClearDepthStencilView(ID3D11DeviceContext* self, ID3D11DepthStencilView* dsv, UINT flags, FLOAT d, UINT8 s) {
    (void)self;
    d3d11_mock_call_t* call = mock_record(D3D11_MOCK_CALL_CLEAR_DEPTH_STENCIL_VIEW);
    call->dst = dsv;
    call->args[0] = flags;
    call->args[1] = s;
    mock_snapshot(call, &d, sizeof(d));
}
static void ctx_ResolveSubresource(ID3D11DeviceContext* self, ID3D11Resource* dst, UINT dsub, ID3D11Resource* src, UINT ssub, DXGI_FORMAT fmt) {
    (void)self;
    d3d11_mock_call_t* call = mock_record(D3D11_MOCK_CALL_RESOLVE_SUBRESOURCE);
    call->dst = dst;
    call->dst_subres = dsub;
    call->src = src;
    call->src_subres = ssub;
    call->args[0] = (UINT)fmt;
}
static void ctx_IASetPrimitiveTopology(ID3D11DeviceContext* self, D3D11_PRIMITIVE_TOPOLOGY t)                                                                                                 { (void)self; (void)t; }
static void ctx_UpdateSubresource(ID3D11DeviceContext* self, ID3D11Resource* dst, UINT dsub, const D3D11_BOX* box, const void* src, UINT rp, UINT dp) {
    (void)self;
    d3d11_mock_call_t* call = mock_record(D3D11_MOCK_CALL_UPDATE_SUBRESOURCE);
    call->dst = dst;
    call->dst_subres = dsub;
    call->src_data = src;
    call->src_row_pitch = rp;
    call->src_depth_pitch = dp;
    const mock_obj_t* res = (const mock_obj_t*)dst;
    size_t size = 0;
    if (box) {
        call->has_box = true;
        call->box = *box;
        if (res && (res->kind == MOCK_KIND_BUFFER)) {
            size = box->right - box->left;
        } else if (dp > 0) {
            size = (size_t)dp * (box->back - box->front);
        } else {
            size = (size_t)rp * (box->bottom - box->top);
        }
    } else if (res) {
        size = res->size_bytes;
    }
    mock_snapshot(call, src, size);
}
static void ctx_CopySubresourceRegion(ID3D11DeviceContext* self, ID3D11Resource* dst, UINT dsub, UINT x, UINT y, UINT z, ID3D11Resource* src, UINT ssub, const D3D11_BOX* box) {
    (void)self;
    d3d11_mock_call_t* call = mock_record(D3D11_MOCK_CALL_COPY_SUBRESOURCE_REGION);
    call->dst = dst;
    call->dst_subres = dsub;
    call->dst_x = x;
    call->dst_y = y;
    call->dst_z = z;
    call->src = src;
    call->src_subres = ssub;
    if (box) {
        call->has_box = true;
        call->box = *box;
    }
}
static void ctx_DrawIndexed(ID3D11DeviceContext* self, UINT n, UINT s, INT b) {
    (void)self;
    d3d11_mock_call_t* call = mock_record(D3D11_MOCK_CALL_DRAW_INDEXED);
    call->args[0] = n; call->args[1] = s; call->args[2] = (UINT)b;
}
static void ctx_DrawIndexedInstanced(ID3D11DeviceContext* self, UINT n, UINT i, UINT s, INT b, UINT bi) {
    (void)self;
    d3d11_mock_call_t* call = mock_record(D3D11_MOCK_CALL_DRAW_INDEXED_INSTANCED);
    call->args[0] = n; call->args[1] = i; call->args[2] = s; call->args[3] = (UINT)b; call->args[4] = bi;
}
static void ctx_Draw(ID3D11DeviceContext* self, UINT n, UINT s) {
    (void)self;
    d3d11_mock_call_t* call = mock_record(D3D11_MOCK_CALL_DRAW);
    call->args[0] = n; call->args[1] = s;
}
static void ctx_DrawInstanced(ID3D11DeviceContext* self, UINT n, UINT i, UINT s, UINT bi) {
    (void)self;
    d3d11_mock_call_t* call = mock_record(D3D11_MOCK_CALL_DRAW_INSTANCED);
    call->args[0] = n; call->args[1] = i; call->args[2] = s; call->args[3] = bi;
}
static void ctx_Dispatch(ID3D11DeviceContext* self, UINT x, UINT y, UINT z) {
    (void)self;
    d3d11_mock_call_t* call = mock_record(D3D11_MOCK_CALL_DISPATCH);
    call->args[0] = x; call->args[1] = y; call->args[2] = z;
}

static HRESULT ctx_Map(ID3D11DeviceContext* self, ID3D11Resource* pRes, UINT sub, D3D11_MAP mt, UINT flags, D3D11_MAPPED_SUBRESOURCE* pMapped) {
    (void)self;
    d3d11_mock_call_t* call = mock_record(D3D11_MOCK_CALL_MAP);
    call->dst = pRes;
    call->dst_subres = sub;
    call->map_type = mt;
    call->args[0] = flags;
    if (fail_next_map_n > 0) {
        fail_next_map_n--;
        call->result = E_FAIL;
        pMapped->pData = NULL;
        pMapped->RowPitch = 0;
        pMapped->DepthPitch = 0;
        return E_FAIL;
    }
    mock_obj_t* res = (mock_obj_t*)pRes;
    size_t need = res->size_bytes > 0 ? res->size_bytes : 64 * 1024;
    if (res->map_scratch_size < need) {
        free(res->map_scratch);
        res->map_scratch = calloc(1, need);
        res->map_scratch_size = need;
    }
    pMapped->pData = res->map_scratch;
    pMapped->RowPitch = res->row_pitch ? res->row_pitch : (UINT)need;
    pMapped->DepthPitch = res->depth_pitch ? res->depth_pitch : (UINT)need;
    return S_OK;
}

static void ctx_Unmap(ID3D11DeviceContext* self, ID3D11Resource* pRes, UINT sub) {
    (void)self;
    d3d11_mock_call_t* call = mock_record(D3D11_MOCK_CALL_UNMAP);
    call->dst = pRes;
    call->dst_subres = sub;
    /* keep the scratch allocated -- freed on Release() */
}

/* ---- ID3D10Blob methods ------------------------------------------------- */
static LPVOID blob_GetBufferPointer(ID3D10Blob* self) {
    return ((mock_obj_t*)self)->blob_data;
}
static SIZE_T blob_GetBufferSize(ID3D10Blob* self) {
    return ((mock_obj_t*)self)->blob_size;
}

/* ---- singleton vtbls --------------------------------------------------- */
static const struct ID3D11DeviceVtbl device_vtbl = {
    dev_CheckFormatSupport,
    dev_GetFeatureLevel,
    dev_CreateBuffer,
    dev_CreateTexture2D,
    dev_CreateTexture3D,
    dev_CreateShaderResourceView,
    dev_CreateUnorderedAccessView,
    dev_CreateRenderTargetView,
    dev_CreateDepthStencilView,
    dev_CreateSamplerState,
    dev_CreateVertexShader,
    dev_CreatePixelShader,
    dev_CreateComputeShader,
    dev_CreateInputLayout,
    dev_CreateRasterizerState,
    dev_CreateDepthStencilState,
    dev_CreateBlendState,
    (ULONG (STDMETHODCALLTYPE*)(ID3D11Device*))mock_addref,
    (ULONG (STDMETHODCALLTYPE*)(ID3D11Device*))mock_release
};

static const struct ID3D11DeviceContextVtbl context_vtbl = {
    ctx_ClearState,
    ctx_OMSetRenderTargets,
    ctx_OMSetRenderTargetsAndUnorderedAccessViews,
    ctx_RSSetState,
    ctx_OMSetDepthStencilState,
    ctx_OMSetBlendState,
    ctx_IASetVertexBuffers,
    ctx_IASetIndexBuffer,
    ctx_IASetInputLayout,
    ctx_VSSetShader,
    ctx_PSSetShader,
    ctx_CSSetShader,
    ctx_VSSetConstantBuffers,
    ctx_PSSetConstantBuffers,
    ctx_CSSetConstantBuffers,
    ctx_VSSetShaderResources,
    ctx_PSSetShaderResources,
    ctx_CSSetShaderResources,
    ctx_VSSetSamplers,
    ctx_PSSetSamplers,
    ctx_CSSetSamplers,
    ctx_CSSetUnorderedAccessViews,
    ctx_RSSetViewports,
    ctx_RSSetScissorRects,
    ctx_ClearRenderTargetView,
    ctx_ClearDepthStencilView,
    ctx_ResolveSubresource,
    ctx_IASetPrimitiveTopology,
    ctx_UpdateSubresource,
    ctx_CopySubresourceRegion,
    ctx_DrawIndexed,
    ctx_DrawIndexedInstanced,
    ctx_Draw,
    ctx_DrawInstanced,
    ctx_Dispatch,
    ctx_Map,
    ctx_Unmap,
    (ULONG (STDMETHODCALLTYPE*)(ID3D11DeviceContext*))mock_addref,
    (ULONG (STDMETHODCALLTYPE*)(ID3D11DeviceContext*))mock_release
};

#define _MOCK_DEVICE_CHILD_VTBL(SELF) \
    (ULONG   (STDMETHODCALLTYPE*)(SELF*))mock_addref, \
    (ULONG   (STDMETHODCALLTYPE*)(SELF*))mock_release, \
    (HRESULT (STDMETHODCALLTYPE*)(SELF*, REFGUID, UINT, const void*))mock_setprivatedata

#define _MOCK_VIEW_VTBL(SELF) \
    _MOCK_DEVICE_CHILD_VTBL(SELF), \
    (void (STDMETHODCALLTYPE*)(SELF*, ID3D11Resource**))mock_view_getresource

static const struct ID3D11BufferVtbl             buffer_vtbl             = { _MOCK_DEVICE_CHILD_VTBL(ID3D11Buffer) };
static const struct ID3D11Texture2DVtbl          texture2d_vtbl          = { _MOCK_DEVICE_CHILD_VTBL(ID3D11Texture2D) };
static const struct ID3D11Texture3DVtbl          texture3d_vtbl          = { _MOCK_DEVICE_CHILD_VTBL(ID3D11Texture3D) };
static const struct ID3D11ShaderResourceViewVtbl srv_vtbl                = { _MOCK_VIEW_VTBL(ID3D11ShaderResourceView) };
static const struct ID3D11UnorderedAccessViewVtbl uav_vtbl               = { _MOCK_VIEW_VTBL(ID3D11UnorderedAccessView) };
static const struct ID3D11RenderTargetViewVtbl   rtv_vtbl                = { _MOCK_VIEW_VTBL(ID3D11RenderTargetView) };
static const struct ID3D11DepthStencilViewVtbl   dsv_vtbl                = { _MOCK_VIEW_VTBL(ID3D11DepthStencilView) };
static const struct ID3D11SamplerStateVtbl       sampler_vtbl            = { _MOCK_DEVICE_CHILD_VTBL(ID3D11SamplerState) };
static const struct ID3D11InputLayoutVtbl        input_layout_vtbl       = { _MOCK_DEVICE_CHILD_VTBL(ID3D11InputLayout) };
static const struct ID3D11RasterizerStateVtbl    rasterizer_vtbl         = { _MOCK_DEVICE_CHILD_VTBL(ID3D11RasterizerState) };
static const struct ID3D11DepthStencilStateVtbl  depth_stencil_vtbl      = { _MOCK_DEVICE_CHILD_VTBL(ID3D11DepthStencilState) };
static const struct ID3D11BlendStateVtbl         blend_vtbl              = { _MOCK_DEVICE_CHILD_VTBL(ID3D11BlendState) };
static const struct ID3D11VertexShaderVtbl       vs_vtbl                 = { _MOCK_DEVICE_CHILD_VTBL(ID3D11VertexShader) };
static const struct ID3D11PixelShaderVtbl        ps_vtbl                 = { _MOCK_DEVICE_CHILD_VTBL(ID3D11PixelShader) };
static const struct ID3D11ComputeShaderVtbl      cs_vtbl                 = { _MOCK_DEVICE_CHILD_VTBL(ID3D11ComputeShader) };

static const struct ID3D10BlobVtbl blob_vtbl = {
    (ULONG (STDMETHODCALLTYPE*)(ID3D10Blob*))mock_addref,
    (ULONG (STDMETHODCALLTYPE*)(ID3D10Blob*))mock_release,
    blob_GetBufferPointer,
    blob_GetBufferSize
};

/* ---- Public API -------------------------------------------------------- */
/* The context is stored alongside the device so tests can retrieve it. */
static ID3D11DeviceContext* current_context = NULL;

ID3D11Device* d3d11_mock_create_device(void) {
    mock_obj_t* dev = mock_alloc(MOCK_KIND_DEVICE, &device_vtbl);
    mock_obj_t* ctx = mock_alloc(MOCK_KIND_CONTEXT, &context_vtbl);
    current_context = (ID3D11DeviceContext*)ctx;
    return (ID3D11Device*)dev;
}

ID3D11DeviceContext* d3d11_mock_get_device_context(ID3D11Device* dev) {
    (void)dev;
    return current_context;
}

void d3d11_mock_destroy_device(ID3D11Device* dev) {
    if (current_context) {
        mock_release(current_context);
        current_context = NULL;
    }
    if (dev) {
        mock_release(dev);
    }
}

ID3D11RenderTargetView* d3d11_mock_create_rtv(ID3D11Device* dev) {
    (void)dev;
    mock_obj_t* tex = mock_alloc(MOCK_KIND_TEXTURE2D, &texture2d_vtbl);
    mock_obj_t* rtv = mock_alloc(MOCK_KIND_RTV, &rtv_vtbl);
    rtv->view_resource = tex;   // GetResource() will hand out this backing texture
    return (ID3D11RenderTargetView*)rtv;
}

ID3D11DepthStencilView* d3d11_mock_create_dsv(ID3D11Device* dev) {
    (void)dev;
    mock_obj_t* tex = mock_alloc(MOCK_KIND_TEXTURE2D, &texture2d_vtbl);
    mock_obj_t* dsv = mock_alloc(MOCK_KIND_DSV, &dsv_vtbl);
    dsv->view_resource = tex;
    return (ID3D11DepthStencilView*)dsv;
}

int d3d11_mock_live_object_count(void) {
    return live_object_count;
}

void d3d11_mock_reset(void) {
    while (live_head) {
        mock_obj_t* obj = live_head->obj;
        mock_free_payload(obj);
        mock_obj_node_t* dead = live_head;
        live_head = dead->next;
        free(dead);
        free(obj);
    }
    live_object_count = 0;
    current_context = NULL;
    fail_next_create_n = 0;
    fail_create_skip_n = 0;
    fail_next_compile_n = 0;
    fail_dll_load = false;
    fail_next_map_n = 0;
    d3d11_mock_clear_calls();
}

void d3d11_mock_fail_next_create(int n) {
    fail_next_create_n = n;
    fail_create_skip_n = 0;
}

void d3d11_mock_fail_create_after(int skip, int n) {
    fail_next_create_n = n;
    fail_create_skip_n = skip;
}

void d3d11_mock_fail_next_compile(int n) {
    fail_next_compile_n = n;
}

void d3d11_mock_fail_d3dcompiler_dll(bool fail) {
    fail_dll_load = fail;
}

void d3d11_mock_fail_next_map(int n) {
    fail_next_map_n = n;
}

void d3d11_mock_clear_calls(void) {
    free(call_log);
    call_log = NULL;
    call_log_num = 0;
    call_log_cap = 0;
}

int d3d11_mock_num_calls(void) {
    return call_log_num;
}

const d3d11_mock_call_t* d3d11_mock_get_call(int index) {
    if ((index < 0) || (index >= call_log_num)) {
        return NULL;
    }
    return &call_log[index];
}

int d3d11_mock_count_calls(d3d11_mock_call_kind_t kind) {
    int n = 0;
    for (int i = 0; i < call_log_num; i++) {
        if (call_log[i].kind == kind) {
            n++;
        }
    }
    return n;
}

const d3d11_mock_call_t* d3d11_mock_nth_call(d3d11_mock_call_kind_t kind, int n) {
    for (int i = 0; i < call_log_num; i++) {
        if (call_log[i].kind == kind) {
            if (n == 0) {
                return &call_log[i];
            }
            n--;
        }
    }
    return NULL;
}

const d3d11_mock_call_t* d3d11_mock_last_call(d3d11_mock_call_kind_t kind) {
    for (int i = call_log_num - 1; i >= 0; i--) {
        if (call_log[i].kind == kind) {
            return &call_log[i];
        }
    }
    return NULL;
}

static const mock_obj_t* mock_find_live(const void* obj) {
    for (const mock_obj_node_t* node = live_head; node; node = node->next) {
        if (node->obj == obj) {
            return node->obj;
        }
    }
    return NULL;
}

ULONG d3d11_mock_refcount(const void* obj) {
    const mock_obj_t* o = mock_find_live(obj);
    return o ? o->refcount : 0;
}

const void* d3d11_mock_mapped_data(const void* res, size_t* out_size) {
    const mock_obj_t* o = mock_find_live(res);
    if (out_size) {
        *out_size = o ? o->map_scratch_size : 0;
    }
    return o ? o->map_scratch : NULL;
}

/* ---- D3DCompile mock --------------------------------------------------- */
/* Called through a function pointer that sokol_gfx.h obtains from
   GetProcAddress. Signature must match pD3DCompile. */
static HRESULT WINAPI mock_D3DCompile(
    LPCVOID pSrcData, SIZE_T SrcDataSize, LPCSTR pSourceName,
    const void* pDefines, void* pInclude,
    LPCSTR pEntry, LPCSTR pTarget, UINT Flags1, UINT Flags2,
    ID3DBlob** ppCode, ID3DBlob** ppErrorMsgs)
{
    (void)pSrcData; (void)SrcDataSize; (void)pSourceName;
    (void)pDefines; (void)pInclude; (void)pEntry; (void)pTarget;
    (void)Flags1; (void)Flags2;
    if (fail_next_compile_n > 0) {
        fail_next_compile_n--;
        *ppCode = NULL;
        if (ppErrorMsgs) {
            /* Emit a small "error message" blob so sokol logs it. */
            mock_obj_t* err = mock_alloc(MOCK_KIND_BLOB, &blob_vtbl);
            const char* msg = "mock: forced compile failure";
            err->blob_size = strlen(msg) + 1;
            err->blob_data = calloc(1, err->blob_size);
            memcpy(err->blob_data, msg, err->blob_size);
            *ppErrorMsgs = (ID3DBlob*)err;
        }
        return E_FAIL;
    }
    /* Emit a small dummy blob so downstream code paths (CreateVertexShader,
       CreateInputLayout) have a non-NULL, non-empty bytecode buffer. */
    mock_obj_t* blob = mock_alloc(MOCK_KIND_BLOB, &blob_vtbl);
    blob->blob_size = 64;
    blob->blob_data = calloc(1, blob->blob_size);
    *ppCode = (ID3DBlob*)blob;
    if (ppErrorMsgs) { *ppErrorMsgs = NULL; }
    return S_OK;
}

/* ---- LoadLibraryA / GetProcAddress dispatch --------------------------- */
/* Recognise the DLLs sokol_gfx.h loads and hand out a fake handle. */
static char mock_d3dcompiler_dll_handle;

HMODULE WINAPI LoadLibraryA(LPCSTR name) {
    if (name && strcmp(name, "d3dcompiler_47.dll") == 0) {
        if (fail_dll_load) { return NULL; }
        return (HMODULE)&mock_d3dcompiler_dll_handle;
    }
    return NULL;
}

int WINAPI FreeLibrary(HMODULE h) {
    (void)h;
    return 1;
}

void* WINAPI GetProcAddress(HMODULE h, LPCSTR name) {
    if (h == (HMODULE)&mock_d3dcompiler_dll_handle && name && strcmp(name, "D3DCompile") == 0) {
        return (void*)mock_D3DCompile;
    }
    return NULL;
}
