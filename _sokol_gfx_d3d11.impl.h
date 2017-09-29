/*
    Sokol Gfx D3D11 rendering backend.
*/
#ifndef SOKOL_IMPL_GUARD
#error "Please do not include *.impl.h files directly"
#endif

#ifndef UNICODE
#define UNICODE
#endif
#define COBJMACROS
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <d3d11.h>

#pragma comment (lib, "user32.lib")
#pragma comment (lib, "dxgi.lib")
#pragma comment (lib, "d3d11.lib")
#pragma comment (lib, "dxguid.lib")

#ifdef __cplusplus
extern "C" {
#endif

/*-- enum translation functions ----------------------------------------------*/
_SOKOL_PRIVATE D3D11_USAGE _sg_d3d11_usage(sg_usage usg) {
    switch (usg) {
        case SG_USAGE_IMMUTABLE:    
            return D3D11_USAGE_IMMUTABLE;
        case SG_USAGE_DYNAMIC:
        case SG_USAGE_STREAM:
            return D3D11_USAGE_DYNAMIC;
        default:
            SOKOL_UNREACHABLE;
            return 0;
    }
}

_SOKOL_PRIVATE UINT _sg_d3d11_cpu_access_flags(sg_usage usg) {
    switch (usg) {
        case SG_USAGE_IMMUTABLE:
            return 0;
        case SG_USAGE_DYNAMIC:
        case SG_USAGE_STREAM:
            return D3D11_CPU_ACCESS_WRITE;
        default:
            SOKOL_UNREACHABLE;
            return 0;
    }
}

/*-- backend resource structures ---------------------------------------------*/
typedef struct {
    _sg_slot slot;
    int size;
    sg_buffer_type type;
    sg_usage usage;
    uint32_t upd_frame_index;
    ID3D11Buffer* d3d11_buf;
} _sg_buffer;

_SOKOL_PRIVATE void _sg_init_buffer(_sg_buffer* buf) {
    SOKOL_ASSERT(buf);
    memset(buf, 0, sizeof(_sg_buffer));
}

typedef struct {
    _sg_slot slot;
    sg_image_type type;
    bool render_target;
    int width;
    int height;
    int depth;
    int num_mipmaps;
    sg_usage usage;
    sg_pixel_format pixel_format;
    int sample_count;
    sg_filter min_filter;
    sg_filter mag_filter;
    sg_wrap wrap_u;
    sg_wrap wrap_v;
    sg_wrap wrap_w;
} _sg_image;

_SOKOL_PRIVATE void _sg_init_image(_sg_image* img) {
    SOKOL_ASSERT(img);
    memset(img, 0, sizeof(_sg_image));
}

typedef struct {
    int size;
} _sg_uniform_block;

typedef struct {
    sg_image_type type;
} _sg_shader_image;

typedef struct {
    int num_uniform_blocks;
    int num_images;
    _sg_uniform_block uniform_blocks[SG_MAX_SHADERSTAGE_UBS];
    _sg_shader_image images[SG_MAX_SHADERSTAGE_IMAGES];
} _sg_shader_stage;

typedef struct {
    _sg_slot slot;
    _sg_shader_stage stage[SG_NUM_SHADER_STAGES];
} _sg_shader;

_SOKOL_PRIVATE void _sg_init_shader(_sg_shader* shd) {
    SOKOL_ASSERT(shd);
    memset(shd, 0, sizeof(_sg_shader));
}

typedef struct {
    _sg_slot slot;
    _sg_shader* shader;
    sg_shader shader_id;
    sg_primitive_type primitive_type;
    sg_index_type index_type;
} _sg_pipeline;

_SOKOL_PRIVATE void _sg_init_pipeline(_sg_pipeline* pip) {
    SOKOL_ASSERT(pip);
    memset(pip, 0, sizeof(_sg_pipeline));
}

typedef struct {
    _sg_image* image;
    sg_image image_id;
    int mip_level;
    int slice;
} _sg_attachment;

typedef struct {
    _sg_slot slot;
    _sg_attachment color_atts[SG_MAX_COLOR_ATTACHMENTS];
    _sg_attachment ds_att;
} _sg_pass;

_SOKOL_PRIVATE void _sg_init_pass(_sg_pass* pass) {
    SOKOL_ASSERT(pass);
    memset(pass, 0, sizeof(_sg_pass));
} 

/*-- main D3D11 backend state and functions ----------------------------------*/
typedef struct {
    bool valid;
    ID3D11Device* dev;
    ID3D11DeviceContext* ctx;
    const void* (*rtv_cb)(void);
    const void* (*dsv_cb)(void);
    bool in_pass;
    int cur_width;
    int cur_height;
    int num_rtvs;
    ID3D11RenderTargetView* cur_rtvs[SG_MAX_COLOR_ATTACHMENTS];
    ID3D11DepthStencilView* cur_dsv;
} _sg_backend;
static _sg_backend _sg_d3d11;

_SOKOL_PRIVATE void _sg_setup_backend(const sg_desc* desc) {
    SOKOL_ASSERT(desc);
    SOKOL_ASSERT(desc->d3d11_device);
    SOKOL_ASSERT(desc->d3d11_device_context);
    SOKOL_ASSERT(desc->d3d11_render_target_view_cb);
    SOKOL_ASSERT(desc->d3d11_depth_stencil_view_cb);
    SOKOL_ASSERT(desc->d3d11_render_target_view_cb != desc->d3d11_depth_stencil_view_cb);
    memset(&_sg_d3d11, 0, sizeof(_sg_d3d11));
    _sg_d3d11.valid = true;
    _sg_d3d11.dev = (ID3D11Device*) desc->d3d11_device;
    _sg_d3d11.ctx = (ID3D11DeviceContext*) desc->d3d11_device_context;
    _sg_d3d11.rtv_cb = desc->d3d11_render_target_view_cb;
    _sg_d3d11.dsv_cb = desc->d3d11_depth_stencil_view_cb;
}

_SOKOL_PRIVATE void _sg_discard_backend() {
    SOKOL_ASSERT(_sg_d3d11.valid);
    memset(&_sg_d3d11, 0, sizeof(_sg_d3d11));
}

_SOKOL_PRIVATE bool _sg_query_feature(sg_feature f) {
    switch (f) {
        case SG_FEATURE_INSTANCED_ARRAYS:
        case SG_FEATURE_TEXTURE_COMPRESSION_DXT:
        case SG_FEATURE_TEXTURE_FLOAT:
        case SG_FEATURE_TEXTURE_HALF_FLOAT:
        case SG_FEATURE_ORIGIN_TOP_LEFT:
        case SG_FEATURE_MSAA_RENDER_TARGETS:
        case SG_FEATURE_PACKED_VERTEX_FORMAT_10_2:
        case SG_FEATURE_MULTIPLE_RENDER_TARGET:
        case SG_FEATURE_IMAGETYPE_3D:
        case SG_FEATURE_IMAGETYPE_ARRAY:
            return true;
        default:
            return false;
    }
}

_SOKOL_PRIVATE void _sg_create_buffer(_sg_buffer* buf, const sg_buffer_desc* desc) {
    SOKOL_ASSERT(buf && desc);
    SOKOL_ASSERT(buf->slot.state == SG_RESOURCESTATE_ALLOC);
    SOKOL_ASSERT(!buf->d3d11_buf);
    buf->size = desc->size;
    buf->type = _sg_select(desc->type, SG_BUFFERTYPE_VERTEXBUFFER);
    buf->usage = _sg_select(desc->usage, SG_USAGE_IMMUTABLE);
    buf->upd_frame_index = 0;
    D3D11_BUFFER_DESC d3d11_desc = {
        .ByteWidth = buf->size,
        .Usage = _sg_d3d11_usage(buf->usage),
        .BindFlags = buf->type == SG_BUFFERTYPE_VERTEXBUFFER ? D3D11_BIND_VERTEX_BUFFER : D3D11_BIND_INDEX_BUFFER,
        .CPUAccessFlags = _sg_d3d11_cpu_access_flags(buf->usage)
    };
    D3D11_SUBRESOURCE_DATA* init_data_ptr = 0;
    D3D11_SUBRESOURCE_DATA init_data = { 0 };
    if (buf->usage == SG_USAGE_IMMUTABLE) {
        SOKOL_ASSERT(desc->content);
        init_data.pSysMem = desc->content;
        init_data_ptr = &init_data;
    }
    HRESULT hr = ID3D11Device_CreateBuffer(_sg_d3d11.dev, &d3d11_desc, init_data_ptr, &buf->d3d11_buf);
    SOKOL_ASSERT(SUCCEEDED(hr) && buf->d3d11_buf);
    buf->slot.state = SG_RESOURCESTATE_VALID;
}

_SOKOL_PRIVATE void _sg_destroy_buffer(_sg_buffer* buf) {
    SOKOL_ASSERT(buf);
    if (buf->slot.state == SG_RESOURCESTATE_VALID) {
        SOKOL_ASSERT(buf->d3d11_buf);
        ID3D11Buffer_Release(buf->d3d11_buf);
    }
    _sg_init_buffer(buf);
}

_SOKOL_PRIVATE void _sg_create_image(_sg_image* img, const sg_image_desc* desc) {
    // FIXME
}

_SOKOL_PRIVATE void _sg_destroy_image(_sg_image* img) {
    // FIXME
}

_SOKOL_PRIVATE void _sg_create_shader(_sg_shader* shd, const sg_shader_desc* desc) {
    // FIXME
}

_SOKOL_PRIVATE void _sg_destroy_shader(_sg_shader* shd) {
    // FIXME
}

_SOKOL_PRIVATE void _sg_create_pipeline(_sg_pipeline* pip, _sg_shader* shd, const sg_pipeline_desc* desc) {
    // FIXME
}

_SOKOL_PRIVATE void _sg_destroy_pipeline(_sg_pipeline* pip) {
    // FIXME
}

_SOKOL_PRIVATE void _sg_create_pass(_sg_pass* pass, _sg_image** att_images, const sg_pass_desc* desc) {
    // FIXME
}

_SOKOL_PRIVATE void _sg_destroy_pass(_sg_pass* pass) {
    // FIXME
}

_SOKOL_PRIVATE void _sg_begin_pass(_sg_pass* pass, const sg_pass_action* action, int w, int h) {
    SOKOL_ASSERT(action);
    SOKOL_ASSERT(!_sg_d3d11.in_pass);
    _sg_d3d11.in_pass = true;
    _sg_d3d11.cur_width = w;
    _sg_d3d11.cur_height = h;
    if (pass) {
        // FIXME: offscreen rendering 
    }
    else {
        /* render to default frame buffer */
        _sg_d3d11.num_rtvs = 1;
        _sg_d3d11.cur_rtvs[0] = (ID3D11RenderTargetView*) _sg_d3d11.rtv_cb();
        _sg_d3d11.cur_dsv = (ID3D11DepthStencilView*) _sg_d3d11.dsv_cb();
        for (int i = 1; i < SG_MAX_COLOR_ATTACHMENTS; i++) {
            _sg_d3d11.cur_rtvs[i] = 0;
        }
        SOKOL_ASSERT(_sg_d3d11.cur_rtvs[0] && _sg_d3d11.cur_dsv);
    }
    /* apply the render-target- and depth-stencil-views */
    ID3D11DeviceContext_OMSetRenderTargets(_sg_d3d11.ctx, _sg_d3d11.num_rtvs, _sg_d3d11.cur_rtvs, _sg_d3d11.cur_dsv);

    /* set viewport to cover whole screen */
    D3D11_VIEWPORT vp = { 0 };
    vp.Width = w;
    vp.Height = h;
    vp.MaxDepth = 1.0f;
    ID3D11DeviceContext_RSSetViewports(_sg_d3d11.ctx, 1, &vp);

    /* perform clear action */
    for (int i = 0; i < _sg_d3d11.num_rtvs; i++) {
        if (action->colors[i].action == SG_ACTION_CLEAR) {
            ID3D11DeviceContext_ClearRenderTargetView(_sg_d3d11.ctx, _sg_d3d11.cur_rtvs[i], action->colors[i].val);
        }
    }
    UINT ds_flags = 0;
    if (action->depth.action == SG_ACTION_CLEAR) {
        ds_flags |= D3D11_CLEAR_DEPTH;
    }
    if (action->stencil.action == SG_ACTION_CLEAR) {
        ds_flags |= D3D11_CLEAR_STENCIL;
    }
    if (0 != ds_flags) {
        ID3D11DeviceContext_ClearDepthStencilView(_sg_d3d11.ctx, _sg_d3d11.cur_dsv, ds_flags, action->depth.val, action->stencil.val);
    }
}

_SOKOL_PRIVATE void _sg_end_pass() {
    SOKOL_ASSERT(_sg_d3d11.in_pass);
    _sg_d3d11.in_pass = false;
    // FIXME: MSAA resolve
}

_SOKOL_PRIVATE void _sg_apply_viewport(int x, int y, int w, int h, bool origin_top_left) {
    // FIXME
}

_SOKOL_PRIVATE void _sg_apply_scissor_rect(int x, int y, int w, int h, bool origin_top_left) {
    // FIXME
}

_SOKOL_PRIVATE void _sg_apply_draw_state(
    _sg_pipeline* pip,
    _sg_buffer** vbs, int num_vbs, _sg_buffer* ib,
    _sg_image** vs_imgs, int num_vs_imgs,
    _sg_image** fs_imgs, int num_fs_imgs)
{
    // FIXME
}

_SOKOL_PRIVATE void _sg_apply_uniform_block(sg_shader_stage stage_index, int ub_index, const void* data, int num_bytes) {
    // FIXME
}

_SOKOL_PRIVATE void _sg_draw(int base_element, int num_elements, int num_instances) {
    // FIXME
}

_SOKOL_PRIVATE void _sg_commit() {
    // FIXME
}

_SOKOL_PRIVATE void _sg_update_buffer(_sg_buffer* buf, const void* data_ptr, int data_size) {
    // FIXME
}

_SOKOL_PRIVATE void _sg_update_image(_sg_image* img, const sg_image_content* data) {
    // FIXME
}

_SOKOL_PRIVATE void _sg_reset_state_cache() {
    // FIXME
}

#ifdef __cplusplus
} // extern "C"
#endif
