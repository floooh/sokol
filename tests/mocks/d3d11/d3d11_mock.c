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
static int fail_next_compile_n = 0;
static bool fail_dll_load = false;

/* Consume one Create* fault slot; returns true if this call should fail. */
static bool mock_consume_create_fault(void) {
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
    (void)self; (void)pInit;
    if (mock_consume_create_fault()) { *ppOut = NULL; return E_FAIL; }
    mock_obj_t* obj = mock_alloc(MOCK_KIND_BUFFER, &buffer_vtbl);
    obj->size_bytes = pDesc ? pDesc->ByteWidth : 0;
    *ppOut = (ID3D11Buffer*)obj;
    return S_OK;
}

static HRESULT dev_CreateTexture2D(ID3D11Device* self, const D3D11_TEXTURE2D_DESC* pDesc, const D3D11_SUBRESOURCE_DATA* pInit, ID3D11Texture2D** ppOut) {
    (void)self; (void)pInit;
    if (mock_consume_create_fault()) { *ppOut = NULL; return E_FAIL; }
    mock_obj_t* obj = mock_alloc(MOCK_KIND_TEXTURE2D, &texture2d_vtbl);
    /* Reserve a generous per-subresource scratch: worst case width*height*16 */
    size_t w = pDesc ? pDesc->Width  : 1;
    size_t h = pDesc ? pDesc->Height : 1;
    obj->row_pitch = (UINT)(w * 16u);
    obj->depth_pitch = (UINT)(obj->row_pitch * h);
    obj->size_bytes = obj->depth_pitch;
    *ppOut = (ID3D11Texture2D*)obj;
    return S_OK;
}

static HRESULT dev_CreateTexture3D(ID3D11Device* self, const D3D11_TEXTURE3D_DESC* pDesc, const D3D11_SUBRESOURCE_DATA* pInit, ID3D11Texture3D** ppOut) {
    (void)self; (void)pInit;
    if (mock_consume_create_fault()) { *ppOut = NULL; return E_FAIL; }
    mock_obj_t* obj = mock_alloc(MOCK_KIND_TEXTURE3D, &texture3d_vtbl);
    size_t w = pDesc ? pDesc->Width  : 1;
    size_t h = pDesc ? pDesc->Height : 1;
    size_t d = pDesc ? pDesc->Depth  : 1;
    obj->row_pitch = (UINT)(w * 16u);
    obj->depth_pitch = (UINT)(obj->row_pitch * h);
    obj->size_bytes = obj->depth_pitch * d;
    *ppOut = (ID3D11Texture3D*)obj;
    return S_OK;
}

static HRESULT dev_CreateShaderResourceView(ID3D11Device* self, ID3D11Resource* pRes, const D3D11_SHADER_RESOURCE_VIEW_DESC* pDesc, ID3D11ShaderResourceView** ppOut) {
    (void)self; (void)pDesc;
    if (mock_consume_create_fault()) { *ppOut = NULL; return E_FAIL; }
    mock_obj_t* obj = mock_alloc(MOCK_KIND_SRV, &srv_vtbl);
    obj->view_resource = (mock_obj_t*)pRes;
    *ppOut = (ID3D11ShaderResourceView*)obj;
    return S_OK;
}

static HRESULT dev_CreateUnorderedAccessView(ID3D11Device* self, ID3D11Resource* pRes, const D3D11_UNORDERED_ACCESS_VIEW_DESC* pDesc, ID3D11UnorderedAccessView** ppOut) {
    (void)self; (void)pDesc;
    if (mock_consume_create_fault()) { *ppOut = NULL; return E_FAIL; }
    mock_obj_t* obj = mock_alloc(MOCK_KIND_UAV, &uav_vtbl);
    obj->view_resource = (mock_obj_t*)pRes;
    *ppOut = (ID3D11UnorderedAccessView*)obj;
    return S_OK;
}

static HRESULT dev_CreateRenderTargetView(ID3D11Device* self, ID3D11Resource* pRes, const D3D11_RENDER_TARGET_VIEW_DESC* pDesc, ID3D11RenderTargetView** ppOut) {
    (void)self; (void)pDesc;
    if (mock_consume_create_fault()) { *ppOut = NULL; return E_FAIL; }
    mock_obj_t* obj = mock_alloc(MOCK_KIND_RTV, &rtv_vtbl);
    obj->view_resource = (mock_obj_t*)pRes;
    *ppOut = (ID3D11RenderTargetView*)obj;
    return S_OK;
}

static HRESULT dev_CreateDepthStencilView(ID3D11Device* self, ID3D11Resource* pRes, const D3D11_DEPTH_STENCIL_VIEW_DESC* pDesc, ID3D11DepthStencilView** ppOut) {
    (void)self; (void)pDesc;
    if (mock_consume_create_fault()) { *ppOut = NULL; return E_FAIL; }
    mock_obj_t* obj = mock_alloc(MOCK_KIND_DSV, &dsv_vtbl);
    obj->view_resource = (mock_obj_t*)pRes;
    *ppOut = (ID3D11DepthStencilView*)obj;
    return S_OK;
}

static HRESULT dev_CreateSamplerState(ID3D11Device* self, const D3D11_SAMPLER_DESC* pDesc, ID3D11SamplerState** ppOut) {
    (void)self; (void)pDesc;
    if (mock_consume_create_fault()) { *ppOut = NULL; return E_FAIL; }
    *ppOut = (ID3D11SamplerState*)mock_alloc(MOCK_KIND_SAMPLER, &sampler_vtbl);
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
    (void)self; (void)pElems; (void)numElems; (void)code; (void)len;
    if (mock_consume_create_fault()) { *ppOut = NULL; return E_FAIL; }
    *ppOut = (ID3D11InputLayout*)mock_alloc(MOCK_KIND_INPUT_LAYOUT, &input_layout_vtbl);
    return S_OK;
}

static HRESULT dev_CreateRasterizerState(ID3D11Device* self, const D3D11_RASTERIZER_DESC* pDesc, ID3D11RasterizerState** ppOut) {
    (void)self; (void)pDesc;
    if (mock_consume_create_fault()) { *ppOut = NULL; return E_FAIL; }
    *ppOut = (ID3D11RasterizerState*)mock_alloc(MOCK_KIND_RASTERIZER_STATE, &rasterizer_vtbl);
    return S_OK;
}

static HRESULT dev_CreateDepthStencilState(ID3D11Device* self, const D3D11_DEPTH_STENCIL_DESC* pDesc, ID3D11DepthStencilState** ppOut) {
    (void)self; (void)pDesc;
    if (mock_consume_create_fault()) { *ppOut = NULL; return E_FAIL; }
    *ppOut = (ID3D11DepthStencilState*)mock_alloc(MOCK_KIND_DEPTH_STENCIL_STATE, &depth_stencil_vtbl);
    return S_OK;
}

static HRESULT dev_CreateBlendState(ID3D11Device* self, const D3D11_BLEND_DESC* pDesc, ID3D11BlendState** ppOut) {
    (void)self; (void)pDesc;
    if (mock_consume_create_fault()) { *ppOut = NULL; return E_FAIL; }
    *ppOut = (ID3D11BlendState*)mock_alloc(MOCK_KIND_BLEND_STATE, &blend_vtbl);
    return S_OK;
}

/* ---- ID3D11DeviceContext methods --------------------------------------- */
static void ctx_ClearState(ID3D11DeviceContext* self)                                                                                                                                       { (void)self; }
static void ctx_OMSetRenderTargets(ID3D11DeviceContext* self, UINT n, ID3D11RenderTargetView* const* rtvs, ID3D11DepthStencilView* dsv)                                                       { (void)self; (void)n; (void)rtvs; (void)dsv; }
static void ctx_RSSetState(ID3D11DeviceContext* self, ID3D11RasterizerState* rs)                                                                                                              { (void)self; (void)rs; }
static void ctx_OMSetDepthStencilState(ID3D11DeviceContext* self, ID3D11DepthStencilState* dss, UINT ref)                                                                                     { (void)self; (void)dss; (void)ref; }
static void ctx_OMSetBlendState(ID3D11DeviceContext* self, ID3D11BlendState* bs, const FLOAT bf[4], UINT mask)                                                                                { (void)self; (void)bs; (void)bf; (void)mask; }
static void ctx_IASetVertexBuffers(ID3D11DeviceContext* self, UINT s, UINT n, ID3D11Buffer* const* bufs, const UINT* strides, const UINT* offsets)                                            { (void)self; (void)s; (void)n; (void)bufs; (void)strides; (void)offsets; }
static void ctx_IASetIndexBuffer(ID3D11DeviceContext* self, ID3D11Buffer* buf, DXGI_FORMAT fmt, UINT off)                                                                                     { (void)self; (void)buf; (void)fmt; (void)off; }
static void ctx_IASetInputLayout(ID3D11DeviceContext* self, ID3D11InputLayout* il)                                                                                                            { (void)self; (void)il; }
static void ctx_VSSetShader(ID3D11DeviceContext* self, ID3D11VertexShader* sh, ID3D11ClassInstance* const* ci, UINT n)                                                                        { (void)self; (void)sh; (void)ci; (void)n; }
static void ctx_PSSetShader(ID3D11DeviceContext* self, ID3D11PixelShader* sh, ID3D11ClassInstance* const* ci, UINT n)                                                                         { (void)self; (void)sh; (void)ci; (void)n; }
static void ctx_CSSetShader(ID3D11DeviceContext* self, ID3D11ComputeShader* sh, ID3D11ClassInstance* const* ci, UINT n)                                                                       { (void)self; (void)sh; (void)ci; (void)n; }
static void ctx_VSSetConstantBuffers(ID3D11DeviceContext* self, UINT s, UINT n, ID3D11Buffer* const* bufs)                                                                                    { (void)self; (void)s; (void)n; (void)bufs; }
static void ctx_PSSetConstantBuffers(ID3D11DeviceContext* self, UINT s, UINT n, ID3D11Buffer* const* bufs)                                                                                    { (void)self; (void)s; (void)n; (void)bufs; }
static void ctx_CSSetConstantBuffers(ID3D11DeviceContext* self, UINT s, UINT n, ID3D11Buffer* const* bufs)                                                                                    { (void)self; (void)s; (void)n; (void)bufs; }
static void ctx_VSSetShaderResources(ID3D11DeviceContext* self, UINT s, UINT n, ID3D11ShaderResourceView* const* srvs)                                                                        { (void)self; (void)s; (void)n; (void)srvs; }
static void ctx_PSSetShaderResources(ID3D11DeviceContext* self, UINT s, UINT n, ID3D11ShaderResourceView* const* srvs)                                                                        { (void)self; (void)s; (void)n; (void)srvs; }
static void ctx_CSSetShaderResources(ID3D11DeviceContext* self, UINT s, UINT n, ID3D11ShaderResourceView* const* srvs)                                                                        { (void)self; (void)s; (void)n; (void)srvs; }
static void ctx_VSSetSamplers(ID3D11DeviceContext* self, UINT s, UINT n, ID3D11SamplerState* const* smps)                                                                                     { (void)self; (void)s; (void)n; (void)smps; }
static void ctx_PSSetSamplers(ID3D11DeviceContext* self, UINT s, UINT n, ID3D11SamplerState* const* smps)                                                                                     { (void)self; (void)s; (void)n; (void)smps; }
static void ctx_CSSetSamplers(ID3D11DeviceContext* self, UINT s, UINT n, ID3D11SamplerState* const* smps)                                                                                     { (void)self; (void)s; (void)n; (void)smps; }
static void ctx_CSSetUnorderedAccessViews(ID3D11DeviceContext* self, UINT s, UINT n, ID3D11UnorderedAccessView* const* uavs, const UINT* counts)                                              { (void)self; (void)s; (void)n; (void)uavs; (void)counts; }
static void ctx_RSSetViewports(ID3D11DeviceContext* self, UINT n, const D3D11_VIEWPORT* vps)                                                                                                  { (void)self; (void)n; (void)vps; }
static void ctx_RSSetScissorRects(ID3D11DeviceContext* self, UINT n, const D3D11_RECT* rects)                                                                                                 { (void)self; (void)n; (void)rects; }
static void ctx_ClearRenderTargetView(ID3D11DeviceContext* self, ID3D11RenderTargetView* rtv, const FLOAT c[4])                                                                               { (void)self; (void)rtv; (void)c; }
static void ctx_ClearDepthStencilView(ID3D11DeviceContext* self, ID3D11DepthStencilView* dsv, UINT flags, FLOAT d, UINT8 s)                                                                   { (void)self; (void)dsv; (void)flags; (void)d; (void)s; }
static void ctx_ResolveSubresource(ID3D11DeviceContext* self, ID3D11Resource* dst, UINT dsub, ID3D11Resource* src, UINT ssub, DXGI_FORMAT fmt)                                                { (void)self; (void)dst; (void)dsub; (void)src; (void)ssub; (void)fmt; }
static void ctx_IASetPrimitiveTopology(ID3D11DeviceContext* self, D3D11_PRIMITIVE_TOPOLOGY t)                                                                                                 { (void)self; (void)t; }
static void ctx_UpdateSubresource(ID3D11DeviceContext* self, ID3D11Resource* dst, UINT dsub, const D3D11_BOX* box, const void* src, UINT rp, UINT dp)                                         { (void)self; (void)dst; (void)dsub; (void)box; (void)src; (void)rp; (void)dp; }
static void ctx_DrawIndexed(ID3D11DeviceContext* self, UINT n, UINT s, INT b)                                                                                                                 { (void)self; (void)n; (void)s; (void)b; }
static void ctx_DrawIndexedInstanced(ID3D11DeviceContext* self, UINT n, UINT i, UINT s, INT b, UINT bi)                                                                                       { (void)self; (void)n; (void)i; (void)s; (void)b; (void)bi; }
static void ctx_Draw(ID3D11DeviceContext* self, UINT n, UINT s)                                                                                                                               { (void)self; (void)n; (void)s; }
static void ctx_DrawInstanced(ID3D11DeviceContext* self, UINT n, UINT i, UINT s, UINT bi)                                                                                                     { (void)self; (void)n; (void)i; (void)s; (void)bi; }
static void ctx_Dispatch(ID3D11DeviceContext* self, UINT x, UINT y, UINT z)                                                                                                                   { (void)self; (void)x; (void)y; (void)z; }

static HRESULT ctx_Map(ID3D11DeviceContext* self, ID3D11Resource* pRes, UINT sub, D3D11_MAP mt, UINT flags, D3D11_MAPPED_SUBRESOURCE* pMapped) {
    (void)self; (void)sub; (void)mt; (void)flags;
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
    (void)self; (void)pRes; (void)sub;
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
    fail_next_compile_n = 0;
    fail_dll_load = false;
}

void d3d11_mock_fail_next_create(int n) {
    fail_next_create_n = n;
}

void d3d11_mock_fail_next_compile(int n) {
    fail_next_compile_n = n;
}

void d3d11_mock_fail_d3dcompiler_dll(bool fail) {
    fail_dll_load = fail;
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
