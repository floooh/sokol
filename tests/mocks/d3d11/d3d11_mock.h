/*
    LLM maintained.

    d3d11_mock.h -- public API of the mocked D3D11 runtime.
*/
#ifndef D3D11_MOCK_H_INCLUDED
#define D3D11_MOCK_H_INCLUDED

#include "d3d11.h"
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Create/destroy a mocked D3D11 device + immediate context pair. Feed the
   returned pointers into sg_desc.environment.d3d11.{device,device_context}. */
extern ID3D11Device* d3d11_mock_create_device(void);
extern ID3D11DeviceContext* d3d11_mock_get_device_context(ID3D11Device* dev);
extern void d3d11_mock_destroy_device(ID3D11Device* dev);

/* Allocate a mock RTV/DSV suitable for filling sg_swapchain.d3d11.*_view.
   Ownership stays with the caller. */
extern ID3D11RenderTargetView* d3d11_mock_create_rtv(ID3D11Device* dev);
extern ID3D11DepthStencilView* d3d11_mock_create_dsv(ID3D11Device* dev);

/* Total number of mock COM objects currently alive (refcount>0). Useful in
   tests for leak checks. */
extern int d3d11_mock_live_object_count(void);

/* Reset the mock to a clean slate. */
extern void d3d11_mock_reset(void);

/* Fault injection -- when the counter is > 0, the next N Create* / D3DCompile
   calls return E_FAIL / NULL. The counter decrements per call. */
extern void d3d11_mock_fail_next_create(int n);
extern void d3d11_mock_fail_next_compile(int n);
extern void d3d11_mock_fail_d3dcompiler_dll(bool fail);
/* Like d3d11_mock_fail_next_create(n), but first lets 'skip' Create* calls
   succeed (reaches failure paths for the 2nd/3rd/... object of a resource). */
extern void d3d11_mock_fail_create_after(int skip, int n);
/* When > 0, the next N ID3D11DeviceContext::Map calls return E_FAIL. */
extern void d3d11_mock_fail_next_map(int n);

/* ---- call recording --------------------------------------------------------
   Every resource-creating device call and the 'interesting' context calls
   (copies, updates, map/unmap, draws, clears, resolves, ...) are appended to
   a call log in submission order. Pure state setters are not recorded. The
   log is cleared by d3d11_mock_reset() and d3d11_mock_clear_calls(). */
typedef enum d3d11_mock_call_kind_t {
    D3D11_MOCK_CALL_NONE = 0,
    /* ID3D11Device */
    D3D11_MOCK_CALL_CREATE_BUFFER,
    D3D11_MOCK_CALL_CREATE_TEXTURE2D,
    D3D11_MOCK_CALL_CREATE_TEXTURE3D,
    D3D11_MOCK_CALL_CREATE_SRV,
    D3D11_MOCK_CALL_CREATE_UAV,
    D3D11_MOCK_CALL_CREATE_RTV,
    D3D11_MOCK_CALL_CREATE_DSV,
    D3D11_MOCK_CALL_CREATE_SAMPLER_STATE,
    D3D11_MOCK_CALL_CREATE_INPUT_LAYOUT,
    D3D11_MOCK_CALL_CREATE_RASTERIZER_STATE,
    D3D11_MOCK_CALL_CREATE_DEPTH_STENCIL_STATE,
    D3D11_MOCK_CALL_CREATE_BLEND_STATE,
    /* ID3D11DeviceContext */
    D3D11_MOCK_CALL_CLEAR_STATE,
    D3D11_MOCK_CALL_OM_SET_RENDER_TARGETS,
    D3D11_MOCK_CALL_IA_SET_VERTEX_BUFFERS,
    D3D11_MOCK_CALL_IA_SET_INDEX_BUFFER,
    D3D11_MOCK_CALL_CLEAR_RENDER_TARGET_VIEW,
    D3D11_MOCK_CALL_CLEAR_DEPTH_STENCIL_VIEW,
    D3D11_MOCK_CALL_RESOLVE_SUBRESOURCE,
    D3D11_MOCK_CALL_UPDATE_SUBRESOURCE,
    D3D11_MOCK_CALL_COPY_SUBRESOURCE_REGION,
    D3D11_MOCK_CALL_MAP,
    D3D11_MOCK_CALL_UNMAP,
    D3D11_MOCK_CALL_DRAW,
    D3D11_MOCK_CALL_DRAW_INDEXED,
    D3D11_MOCK_CALL_DRAW_INSTANCED,
    D3D11_MOCK_CALL_DRAW_INDEXED_INSTANCED,
    D3D11_MOCK_CALL_DISPATCH,
    /* resource binding setters (args[0]=StartSlot, args[1]=Num, ptrs[]) */
    D3D11_MOCK_CALL_VS_SET_SHADER_RESOURCES,
    D3D11_MOCK_CALL_PS_SET_SHADER_RESOURCES,
    D3D11_MOCK_CALL_CS_SET_SHADER_RESOURCES,
    D3D11_MOCK_CALL_VS_SET_SAMPLERS,
    D3D11_MOCK_CALL_PS_SET_SAMPLERS,
    D3D11_MOCK_CALL_CS_SET_SAMPLERS,
    D3D11_MOCK_CALL_CS_SET_UNORDERED_ACCESS_VIEWS
} d3d11_mock_call_kind_t;

#define D3D11_MOCK_DATA_SNAPSHOT_SIZE (64)
#define D3D11_MOCK_MAX_RECORDED_PTRS (16)
#define D3D11_MOCK_MAX_INPUT_ELEMENTS (16)

typedef struct d3d11_mock_call_t {
    d3d11_mock_call_kind_t kind;
    HRESULT result;             /* Create*() / Map() return value */
    /* dst: created object, mapped/updated/copy-dst/resolve-dst resource,
       cleared view, first bound buffer; src: copy/resolve source resource */
    const void* dst;
    const void* src;
    UINT dst_subres;
    UINT src_subres;
    UINT dst_x, dst_y, dst_z;   /* CopySubresourceRegion */
    bool has_box;               /* UpdateSubresource dst box / Copy src box */
    D3D11_BOX box;
    const void* src_data;       /* UpdateSubresource pSrcData / Create* pSysMem */
    UINT src_row_pitch;
    UINT src_depth_pitch;
    /* first bytes of src_data (UpdateSubresource, CreateBuffer init data) */
    size_t data_size;
    unsigned char data[D3D11_MOCK_DATA_SNAPSHOT_SIZE];
    D3D11_MAP map_type;
    bool has_init_data;         /* Create* got a D3D11_SUBRESOURCE_DATA */
    D3D11_BUFFER_DESC buffer_desc;
    D3D11_TEXTURE2D_DESC tex2d_desc;
    D3D11_TEXTURE3D_DESC tex3d_desc;
    /* Create*View: copy of the view desc (has_view_desc false if NULL desc) */
    bool has_view_desc;
    D3D11_SHADER_RESOURCE_VIEW_DESC srv_desc;
    D3D11_UNORDERED_ACCESS_VIEW_DESC uav_desc;
    D3D11_RENDER_TARGET_VIEW_DESC rtv_desc;
    D3D11_DEPTH_STENCIL_VIEW_DESC dsv_desc;
    /* pipeline-state / sampler create descs (args[0] = num input elements) */
    D3D11_SAMPLER_DESC sampler_desc;
    D3D11_RASTERIZER_DESC rasterizer_desc;
    D3D11_DEPTH_STENCIL_DESC depth_stencil_desc;
    D3D11_BLEND_DESC blend_desc;
    D3D11_INPUT_ELEMENT_DESC input_elements[D3D11_MOCK_MAX_INPUT_ELEMENTS];
    /* generic args: draw/dispatch counts, clear flags, index format/offset,
       vertex-buffer start slot/count/first offset, resolve format, ... */
    UINT args[5];
    /* *Set{ShaderResources,Samplers,UnorderedAccessViews}: first N pointers */
    const void* ptrs[D3D11_MOCK_MAX_RECORDED_PTRS];
} d3d11_mock_call_t;

extern void d3d11_mock_clear_calls(void);
extern int d3d11_mock_num_calls(void);
/* NULL if index is out of range */
extern const d3d11_mock_call_t* d3d11_mock_get_call(int index);
extern int d3d11_mock_count_calls(d3d11_mock_call_kind_t kind);
/* n-th (0-based) recorded call of a kind, NULL if there are fewer */
extern const d3d11_mock_call_t* d3d11_mock_nth_call(d3d11_mock_call_kind_t kind, int n);
/* most recent call of a kind, NULL if none */
extern const d3d11_mock_call_t* d3d11_mock_last_call(d3d11_mock_call_kind_t kind);

/* ---- object inspection ---------------------------------------------------- */
/* Current refcount of a live mock COM object (0 if obj is not alive). */
extern ULONG d3d11_mock_refcount(const void* obj);
/* Scratch memory handed out by the most recent Map() of a resource (NULL if
   the resource was never mapped). Lets tests check what was memcpy'd. */
extern const void* d3d11_mock_mapped_data(const void* res, size_t* out_size);

#ifdef __cplusplus
}
#endif

#endif /* D3D11_MOCK_H_INCLUDED */
