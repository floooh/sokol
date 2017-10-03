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
#if defined(SOKOL_D3D11_SHADER_COMPILER)
#include <d3dcompiler.h>
#pragma comment (lib, "d3dcompiler.lib")
#endif

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

_SOKOL_PRIVATE D3D11_PRIMITIVE_TOPOLOGY _sg_d3d11_primitive_topology(sg_primitive_type prim_type) {
    switch (prim_type) {
        case SG_PRIMITIVETYPE_POINTS:           return D3D11_PRIMITIVE_TOPOLOGY_POINTLIST;
        case SG_PRIMITIVETYPE_LINES:            return D3D11_PRIMITIVE_TOPOLOGY_LINELIST;
        case SG_PRIMITIVETYPE_LINE_STRIP:       return D3D11_PRIMITIVE_TOPOLOGY_LINESTRIP;
        case SG_PRIMITIVETYPE_TRIANGLES:        return D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
        case SG_PRIMITIVETYPE_TRIANGLE_STRIP:   return D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP;
        default: SOKOL_UNREACHABLE; return 0;
    }
}

_SOKOL_PRIVATE DXGI_FORMAT _sg_d3d11_index_format(sg_index_type index_type) {
    switch (index_type) {
        case SG_INDEXTYPE_NONE:     return DXGI_FORMAT_UNKNOWN;
        case SG_INDEXTYPE_UINT16:   return DXGI_FORMAT_R16_UINT;
        case SG_INDEXTYPE_UINT32:   return DXGI_FORMAT_R32_UINT;
        default: SOKOL_UNREACHABLE; return 0;
    }
}

_SOKOL_PRIVATE DXGI_FORMAT _sg_d3d11_vertex_format(sg_vertex_format fmt) {
    switch (fmt) {
        case SG_VERTEXFORMAT_FLOAT:     return DXGI_FORMAT_R32_FLOAT;
        case SG_VERTEXFORMAT_FLOAT2:    return DXGI_FORMAT_R32G32_FLOAT;
        case SG_VERTEXFORMAT_FLOAT3:    return DXGI_FORMAT_R32G32B32_FLOAT;
        case SG_VERTEXFORMAT_FLOAT4:    return DXGI_FORMAT_R32G32B32A32_FLOAT;
        case SG_VERTEXFORMAT_BYTE4:     return DXGI_FORMAT_R8G8B8A8_SINT;
        case SG_VERTEXFORMAT_BYTE4N:    return DXGI_FORMAT_R8G8B8A8_SNORM;
        case SG_VERTEXFORMAT_UBYTE4:    return DXGI_FORMAT_R8G8B8A8_UINT;
        case SG_VERTEXFORMAT_UBYTE4N:   return DXGI_FORMAT_R8G8B8A8_UNORM;
        case SG_VERTEXFORMAT_SHORT2:    return DXGI_FORMAT_R16G16_SINT;
        case SG_VERTEXFORMAT_SHORT2N:   return DXGI_FORMAT_R16G16_SNORM;
        case SG_VERTEXFORMAT_SHORT4:    return DXGI_FORMAT_R16G16B16A16_SINT;
        case SG_VERTEXFORMAT_SHORT4N:   return DXGI_FORMAT_R16G16B16A16_SNORM;
        /* FIXME: signed 10-10-10-2 vertex format not supported on d3d11 (only unsigned) */
        default: SOKOL_UNREACHABLE; return 0;
    }
}

_SOKOL_PRIVATE D3D11_INPUT_CLASSIFICATION _sg_d3d11_input_classification(sg_vertex_step step) {
    switch (step) {
        case SG_VERTEXSTEP_PER_VERTEX:      return D3D11_INPUT_PER_VERTEX_DATA;
        case SG_VERTEXSTEP_PER_INSTANCE:    return D3D11_INPUT_PER_INSTANCE_DATA;
        default: SOKOL_UNREACHABLE; return 0;
    }
}

_SOKOL_PRIVATE D3D11_CULL_MODE _sg_d3d11_cull_mode(sg_cull_mode m) {
    switch (m) {
        case SG_CULLMODE_NONE:      return D3D11_CULL_NONE;
        case SG_CULLMODE_FRONT:     return D3D11_CULL_FRONT;
        case SG_CULLMODE_BACK:      return D3D11_CULL_BACK;
        default: SOKOL_UNREACHABLE; return 0;
    }
}

_SOKOL_PRIVATE D3D11_COMPARISON_FUNC _sg_d3d11_compare_func(sg_compare_func f) {
    switch (f) {
        case SG_COMPAREFUNC_NEVER:          return D3D11_COMPARISON_NEVER;
        case SG_COMPAREFUNC_LESS:           return D3D11_COMPARISON_LESS;
        case SG_COMPAREFUNC_EQUAL:          return D3D11_COMPARISON_EQUAL;
        case SG_COMPAREFUNC_LESS_EQUAL:     return D3D11_COMPARISON_LESS_EQUAL;
        case SG_COMPAREFUNC_GREATER:        return D3D11_COMPARISON_GREATER;
        case SG_COMPAREFUNC_NOT_EQUAL:      return D3D11_COMPARISON_NOT_EQUAL;
        case SG_COMPAREFUNC_GREATER_EQUAL:  return D3D11_COMPARISON_GREATER_EQUAL;
        case SG_COMPAREFUNC_ALWAYS:         return D3D11_COMPARISON_ALWAYS;
        default: SOKOL_UNREACHABLE; return 0;
    }
}

_SOKOL_PRIVATE D3D11_STENCIL_OP _sg_d3d11_stencil_op(sg_stencil_op op) {
    switch (op) {
        case SG_STENCILOP_KEEP:         return D3D11_STENCIL_OP_KEEP;
        case SG_STENCILOP_ZERO:         return D3D11_STENCIL_OP_ZERO;
        case SG_STENCILOP_REPLACE:      return D3D11_STENCIL_OP_REPLACE;
        case SG_STENCILOP_INCR_CLAMP:   return D3D11_STENCIL_OP_INCR_SAT;
        case SG_STENCILOP_DECR_CLAMP:   return D3D11_STENCIL_OP_DECR_SAT;
        case SG_STENCILOP_INVERT:       return D3D11_STENCIL_OP_INVERT;
        case SG_STENCILOP_INCR_WRAP:    return D3D11_STENCIL_OP_INCR;
        case SG_STENCILOP_DECR_WRAP:    return D3D11_STENCIL_OP_DECR;
        default: SOKOL_UNREACHABLE; return 0;
    }
}

_SOKOL_PRIVATE D3D11_BLEND _sg_d3d11_blend_factor(sg_blend_factor f) {
    switch (f) {
        case SG_BLENDFACTOR_ZERO:                   return D3D11_BLEND_ZERO;
        case SG_BLENDFACTOR_ONE:                    return D3D11_BLEND_ONE;
        case SG_BLENDFACTOR_SRC_COLOR:              return D3D11_BLEND_SRC_COLOR;
        case SG_BLENDFACTOR_ONE_MINUS_SRC_COLOR:    return D3D11_BLEND_INV_SRC_COLOR;
        case SG_BLENDFACTOR_SRC_ALPHA:              return D3D11_BLEND_SRC_ALPHA;
        case SG_BLENDFACTOR_ONE_MINUS_SRC_ALPHA:    return D3D11_BLEND_INV_SRC_ALPHA;
        case SG_BLENDFACTOR_DST_COLOR:              return D3D11_BLEND_DEST_COLOR;
        case SG_BLENDFACTOR_ONE_MINUS_DST_COLOR:    return D3D11_BLEND_INV_DEST_COLOR;
        case SG_BLENDFACTOR_DST_ALPHA:              return D3D11_BLEND_DEST_ALPHA;
        case SG_BLENDFACTOR_ONE_MINUS_DST_ALPHA:    return D3D11_BLEND_INV_DEST_ALPHA;
        case SG_BLENDFACTOR_SRC_ALPHA_SATURATED:    return D3D11_BLEND_SRC_ALPHA_SAT;
        case SG_BLENDFACTOR_BLEND_COLOR:            return D3D11_BLEND_BLEND_FACTOR;
        case SG_BLENDFACTOR_ONE_MINUS_BLEND_COLOR:  return D3D11_BLEND_INV_BLEND_FACTOR;
        case SG_BLENDFACTOR_BLEND_ALPHA:            return D3D11_BLEND_BLEND_FACTOR;
        case SG_BLENDFACTOR_ONE_MINUS_BLEND_ALPHA:  return D3D11_BLEND_INV_BLEND_FACTOR;
        default: SOKOL_UNREACHABLE; return 0;
    }
}

_SOKOL_PRIVATE D3D11_BLEND_OP _sg_d3d11_blend_op(sg_blend_op op) {
    switch (op) {
        case SG_BLENDOP_ADD:                return D3D11_BLEND_OP_ADD;
        case SG_BLENDOP_SUBTRACT:           return D3D11_BLEND_OP_SUBTRACT;
        case SG_BLENDOP_REVERSE_SUBTRACT:   return D3D11_BLEND_OP_REV_SUBTRACT;
        default: SOKOL_UNREACHABLE; return 0;
    }
}

_SOKOL_PRIVATE UINT8 _sg_d3d11_color_write_mask(sg_color_mask m) {
    UINT8 res = 0;
    if (m & SG_COLORMASK_R) {
        res |= D3D11_COLOR_WRITE_ENABLE_RED;
    }
    if (m & SG_COLORMASK_G) {
        res |= D3D11_COLOR_WRITE_ENABLE_GREEN;
    }
    if (m & SG_COLORMASK_B) {
        res |= D3D11_COLOR_WRITE_ENABLE_BLUE;
    }
    if (m & SG_COLORMASK_A) {
        res |= D3D11_COLOR_WRITE_ENABLE_ALPHA;
    }
    return res;
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
    ID3D11Buffer* d3d11_cbs[SG_MAX_SHADERSTAGE_UBS];
} _sg_shader_stage;

typedef struct {
    _sg_slot slot;
    _sg_shader_stage stage[SG_NUM_SHADER_STAGES];
    ID3D11VertexShader* d3d11_vs;
    ID3D11PixelShader* d3d11_fs;
    void* d3d11_vs_blob;
    int d3d11_vs_blob_length;
} _sg_shader;

_SOKOL_PRIVATE void _sg_init_shader(_sg_shader* shd) {
    SOKOL_ASSERT(shd);
    memset(shd, 0, sizeof(_sg_shader));
}

typedef struct {
    _sg_slot slot;
    _sg_shader* shader;
    sg_shader shader_id;
    sg_index_type index_type;
    float blend_color[4];
    UINT d3d11_stencil_ref;
    UINT d3d11_vb_strides[SG_MAX_SHADERSTAGE_BUFFERS];
    D3D_PRIMITIVE_TOPOLOGY d3d11_topology;
    DXGI_FORMAT d3d11_index_format;
    ID3D11InputLayout* d3d11_il;
    ID3D11RasterizerState* d3d11_rs;
    ID3D11DepthStencilState* d3d11_dss;
    ID3D11BlendState* d3d11_bs;
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
    bool use_indexed_draw;
    uint32_t frame_index;
    int cur_width;
    int cur_height;
    int num_rtvs;
    ID3D11RenderTargetView* cur_rtvs[SG_MAX_COLOR_ATTACHMENTS];
    ID3D11DepthStencilView* cur_dsv;
    /* the following arrays are used for unbinding resources, they will always contain zeroes */ 
    ID3D11RenderTargetView* zero_rtvs[SG_MAX_COLOR_ATTACHMENTS];
    ID3D11Buffer* zero_vbs[SG_MAX_SHADERSTAGE_BUFFERS];
    UINT zero_vb_offsets[SG_MAX_SHADERSTAGE_BUFFERS];
    UINT zero_vb_strides[SG_MAX_SHADERSTAGE_BUFFERS];
    ID3D11Buffer* zero_cbs[SG_MAX_SHADERSTAGE_UBS];
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
        case SG_FEATURE_MULTIPLE_RENDER_TARGET:
        case SG_FEATURE_IMAGETYPE_3D:
        case SG_FEATURE_IMAGETYPE_ARRAY:
            return true;
        default:
            return false;
    }
}

_SOKOL_PRIVATE void _sg_d3d11_clear_state() {
    /* clear all the device context state, so that resource refs don't keep stuck in the d3d device context */
    ID3D11DeviceContext_OMSetRenderTargets(_sg_d3d11.ctx, SG_MAX_COLOR_ATTACHMENTS, _sg_d3d11.zero_rtvs, NULL);
    ID3D11DeviceContext_RSSetState(_sg_d3d11.ctx, NULL);
    ID3D11DeviceContext_OMSetDepthStencilState(_sg_d3d11.ctx, NULL, 0);
    ID3D11DeviceContext_OMSetBlendState(_sg_d3d11.ctx, NULL, NULL, 0xFFFFFFFF);
    ID3D11DeviceContext_IASetVertexBuffers(_sg_d3d11.ctx, 0, SG_MAX_SHADERSTAGE_BUFFERS, _sg_d3d11.zero_vbs, _sg_d3d11.zero_vb_strides, _sg_d3d11.zero_vb_offsets);
    ID3D11DeviceContext_IASetIndexBuffer(_sg_d3d11.ctx, NULL, DXGI_FORMAT_UNKNOWN, 0); 
    ID3D11DeviceContext_IASetInputLayout(_sg_d3d11.ctx, NULL);
    ID3D11DeviceContext_VSSetShader(_sg_d3d11.ctx, NULL, NULL, 0);
    ID3D11DeviceContext_PSSetShader(_sg_d3d11.ctx, NULL, NULL, 0);
    ID3D11DeviceContext_VSSetConstantBuffers(_sg_d3d11.ctx, 0, SG_MAX_SHADERSTAGE_UBS, _sg_d3d11.zero_cbs);
    ID3D11DeviceContext_PSSetConstantBuffers(_sg_d3d11.ctx, 0, SG_MAX_SHADERSTAGE_UBS, _sg_d3d11.zero_cbs);
    /* FIXME: textures and samplers */
}

_SOKOL_PRIVATE void _sg_create_buffer(_sg_buffer* buf, const sg_buffer_desc* desc) {
    SOKOL_ASSERT(buf && desc);
    SOKOL_ASSERT(buf->slot.state == SG_RESOURCESTATE_ALLOC);
    SOKOL_ASSERT(!buf->d3d11_buf);
    buf->size = desc->size;
    buf->type = _sg_select(desc->type, SG_BUFFERTYPE_VERTEXBUFFER);
    buf->usage = _sg_select(desc->usage, SG_USAGE_IMMUTABLE);
    buf->upd_frame_index = 0;
    D3D11_BUFFER_DESC d3d11_desc;
    memset(&d3d11_desc, 0, sizeof(d3d11_desc));
    d3d11_desc.ByteWidth = buf->size,
    d3d11_desc.Usage = _sg_d3d11_usage(buf->usage),
    d3d11_desc.BindFlags = buf->type == SG_BUFFERTYPE_VERTEXBUFFER ? D3D11_BIND_VERTEX_BUFFER : D3D11_BIND_INDEX_BUFFER,
    d3d11_desc.CPUAccessFlags = _sg_d3d11_cpu_access_flags(buf->usage);
    D3D11_SUBRESOURCE_DATA* init_data_ptr = 0;
    D3D11_SUBRESOURCE_DATA init_data;
    memset(&init_data, 0, sizeof(init_data));
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
    if (buf->d3d11_buf) {
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

#if defined(SOKOL_D3D11_SHADER_COMPILER)
_SOKOL_PRIVATE ID3DBlob* _sg_d3d11_compile_shader(const sg_shader_stage_desc* stage_desc, const char* target) {
    ID3DBlob* output = NULL;
    ID3DBlob* errors = NULL;
    HRESULT hr = D3DCompile(
        stage_desc->source,             /* pSrcData */
        strlen(stage_desc->source),     /* SrcDataSize */
        NULL,                           /* pSourceName */
        NULL,                           /* pDefines */
        NULL,                           /* pInclude */
        stage_desc->entry ? stage_desc->entry : "main",     /* pEntryPoint */
        target,     /* pTarget (vs_5_0 or ps_5_0) */
        D3DCOMPILE_PACK_MATRIX_COLUMN_MAJOR | D3DCOMPILE_OPTIMIZATION_LEVEL3,   /* Flags1 */
        0,          /* Flags2 */
        &output,    /* ppCode */
        &errors);   /* ppErrorMsgs */
    if (errors) {
        SOKOL_LOG(ID3D10Blob_GetBufferPointer(errors));
        ID3D10Blob_Release(errors); errors = NULL;
    }
    return output;
}
#endif

#define _sg_d3d11_roundup(val, round_to) (((val)+((round_to)-1))&~((round_to)-1))

_SOKOL_PRIVATE void _sg_create_shader(_sg_shader* shd, const sg_shader_desc* desc) {
    SOKOL_ASSERT(shd && desc);
    SOKOL_ASSERT(shd->slot.state == SG_RESOURCESTATE_ALLOC);
    SOKOL_ASSERT(!shd->d3d11_vs && !shd->d3d11_fs && !shd->d3d11_vs_blob);
    HRESULT hr;

    /* shader stage uniform blocks and image slots */
    for (int stage_index = 0; stage_index < SG_NUM_SHADER_STAGES; stage_index++) {
        const sg_shader_stage_desc* stage_desc = (stage_index == SG_SHADERSTAGE_VS) ? &desc->vs : &desc->fs;
        _sg_shader_stage* stage = &shd->stage[stage_index];
        SOKOL_ASSERT(stage->num_uniform_blocks == 0);
        for (int ub_index = 0; ub_index < SG_MAX_SHADERSTAGE_UBS; ub_index++) {
            const sg_shader_uniform_block_desc* ub_desc = &stage_desc->uniform_blocks[ub_index];
            if (0 == ub_desc->size) {
                break;
            }
            _sg_uniform_block* ub = &stage->uniform_blocks[ub_index];
            ub->size = ub_desc->size;

            /* create a D3D constant buffer */
            /* FIXME: on D3D11.1 we should use a global per-frame constant buffer */
            SOKOL_ASSERT(!stage->d3d11_cbs[ub_index]);
            D3D11_BUFFER_DESC cb_desc;
            memset(&cb_desc, 0, sizeof(cb_desc));
            cb_desc.ByteWidth = _sg_d3d11_roundup(ub->size, 16);
            cb_desc.Usage = D3D11_USAGE_DEFAULT;
            cb_desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
            hr = ID3D11Device_CreateBuffer(_sg_d3d11.dev, &cb_desc, NULL, &stage->d3d11_cbs[ub_index]);
            SOKOL_ASSERT(SUCCEEDED(hr) && stage->d3d11_cbs[ub_index]);

            stage->num_uniform_blocks++;
        }
        SOKOL_ASSERT(stage->num_images == 0);
        for (int img_index = 0; img_index < SG_MAX_SHADERSTAGE_IMAGES; img_index++) {
            const sg_shader_image_desc* img_desc = &stage_desc->images[img_index];
            if (img_desc->type == _SG_IMAGETYPE_DEFAULT) {
                break;
            }
            stage->images[img_index].type = img_desc->type;
            stage->num_images++;
        }
    }

    /* FIXME: byte code */

    /* compile shader code */
    #if defined(SOKOL_D3D11_SHADER_COMPILER)
    ID3DBlob* vs_blob = _sg_d3d11_compile_shader(&desc->vs, "vs_5_0");
    ID3DBlob* fs_blob = _sg_d3d11_compile_shader(&desc->fs, "ps_5_0");
    if (vs_blob && fs_blob) {
        const void* vs_ptr = ID3D10Blob_GetBufferPointer(vs_blob);
        SIZE_T vs_length = ID3D10Blob_GetBufferSize(vs_blob);
        SOKOL_ASSERT(vs_ptr && vs_length > 0);
        hr = ID3D11Device_CreateVertexShader(_sg_d3d11.dev, vs_ptr, vs_length, NULL, &shd->d3d11_vs);
        SOKOL_ASSERT(SUCCEEDED(hr) && shd->d3d11_vs);
        const void* fs_ptr = ID3D10Blob_GetBufferPointer(fs_blob);
        SIZE_T fs_length = ID3D10Blob_GetBufferSize(fs_blob);
        SOKOL_ASSERT(fs_ptr && fs_length > 0);
        hr = ID3D11Device_CreatePixelShader(_sg_d3d11.dev, fs_ptr, fs_length, NULL, &shd->d3d11_fs);
        SOKOL_ASSERT(SUCCEEDED(hr) && shd->d3d11_fs);

        /* need to store the vertex shader byte code, this is needed later in sg_create_pipeline */
        shd->d3d11_vs_blob_length = vs_length;
        shd->d3d11_vs_blob = SOKOL_MALLOC(vs_length);
        memcpy(shd->d3d11_vs_blob, vs_ptr, vs_length);

        shd->slot.state = SG_RESOURCESTATE_VALID;
    }
    else {
        /* compilation errors */
        shd->slot.state = SG_RESOURCESTATE_FAILED;
    }
    if (vs_blob) {
        ID3D10Blob_Release(vs_blob); vs_blob = NULL;
    }
    if (fs_blob) {
        ID3D10Blob_Release(fs_blob); fs_blob = NULL;
    }
    #else
        /* FIXME: shader byte code */
    #endif
}

_SOKOL_PRIVATE void _sg_destroy_shader(_sg_shader* shd) {
    SOKOL_ASSERT(shd);
    if (shd->d3d11_vs) {
        ID3D11VertexShader_Release(shd->d3d11_vs);
    }
    if (shd->d3d11_fs) {
        ID3D11PixelShader_Release(shd->d3d11_fs);
    }
    if (shd->d3d11_vs_blob) {
        SOKOL_FREE(shd->d3d11_vs_blob);
    }
    for (int stage_index = 0; stage_index < SG_NUM_SHADER_STAGES; stage_index++) {
        _sg_shader_stage* stage = &shd->stage[stage_index];
        for (int ub_index = 0; ub_index < stage->num_uniform_blocks; ub_index++) {
            if (stage->d3d11_cbs[ub_index]) {
                ID3D11Buffer_Release(stage->d3d11_cbs[ub_index]);
            }
        }
    }
    _sg_init_shader(shd);
}

_SOKOL_PRIVATE void _sg_create_pipeline(_sg_pipeline* pip, _sg_shader* shd, const sg_pipeline_desc* desc) {
    SOKOL_ASSERT(pip && shd && desc);
    SOKOL_ASSERT(pip->slot.state == SG_RESOURCESTATE_ALLOC);
    SOKOL_ASSERT(desc->shader.id == shd->slot.id);
    SOKOL_ASSERT(shd->slot.state == SG_RESOURCESTATE_VALID);
    SOKOL_ASSERT(shd->d3d11_vs_blob && shd->d3d11_vs_blob_length > 0);
    SOKOL_ASSERT(!pip->d3d11_il && !pip->d3d11_rs && !pip->d3d11_dss && !pip->d3d11_bs);
    HRESULT hr;

    pip->shader = shd;
    pip->shader_id = desc->shader;
    pip->index_type = _sg_select(desc->index_type, SG_INDEXTYPE_NONE);
    pip->d3d11_index_format = _sg_d3d11_index_format(pip->index_type);
    pip->d3d11_topology = _sg_d3d11_primitive_topology(_sg_select(desc->primitive_type, SG_PRIMITIVETYPE_TRIANGLES));
    for (int i = 0; i < 4; i++) {
        pip->blend_color[i] = desc->blend.blend_color[i];
    }
    pip->d3d11_stencil_ref = desc->depth_stencil.stencil_ref;

    /* create input layout object */
    D3D11_INPUT_ELEMENT_DESC d3d11_comps[SG_MAX_VERTEX_ATTRIBUTES];
    memset(d3d11_comps, 0, sizeof(d3d11_comps));
    int d3d11_attr_index = 0;
    for (int layout_index = 0; layout_index < SG_MAX_SHADERSTAGE_BUFFERS; layout_index++) {
        const sg_vertex_layout_desc* layout_desc = &desc->vertex_layouts[layout_index];
        if (layout_desc->stride == 0) {
            break;
        }
        pip->d3d11_vb_strides[layout_index] = layout_desc->stride;
        for (int attr_index = 0; attr_index < SG_MAX_VERTEX_ATTRIBUTES; attr_index++) {
            const sg_vertex_attr_desc* attr_desc = &layout_desc->attrs[attr_index];
            if (attr_desc->format == SG_VERTEXFORMAT_INVALID) {
                break;
            }
            SOKOL_ASSERT(d3d11_attr_index < SG_MAX_VERTEX_ATTRIBUTES);
            D3D11_INPUT_ELEMENT_DESC* d3d11_comp = &d3d11_comps[d3d11_attr_index++];
            d3d11_comp->SemanticName = attr_desc->name;
            d3d11_comp->SemanticIndex = 0;
            d3d11_comp->Format = _sg_d3d11_vertex_format(attr_desc->format);
            d3d11_comp->InputSlot = layout_index;
            d3d11_comp->AlignedByteOffset = attr_desc->offset;
            const sg_vertex_step step_func = _sg_select(layout_desc->step_func, SG_VERTEXSTEP_PER_VERTEX);
            d3d11_comp->InputSlotClass = _sg_d3d11_input_classification(step_func);
            if (SG_VERTEXSTEP_PER_INSTANCE == step_func) {
                d3d11_comp->InstanceDataStepRate = _sg_select(layout_desc->step_rate, 1);
            }
        }
    }
    hr = ID3D11Device_CreateInputLayout(_sg_d3d11.dev,
        d3d11_comps,                /* pInputElementDesc */
        d3d11_attr_index,           /* NumElements */
        shd->d3d11_vs_blob,         /* pShaderByteCodeWithInputSignature */
        shd->d3d11_vs_blob_length,  /* BytecodeLength */
        &pip->d3d11_il);
    SOKOL_ASSERT(SUCCEEDED(hr) && pip->d3d11_il);

    /* create rasterizer state */
    D3D11_RASTERIZER_DESC rs_desc;
    memset(&rs_desc, 0, sizeof(rs_desc));
    rs_desc.FillMode = D3D11_FILL_SOLID;
    rs_desc.CullMode = _sg_d3d11_cull_mode(_sg_select(desc->rasterizer.cull_mode, SG_CULLMODE_NONE));
    rs_desc.FrontCounterClockwise = _sg_select(desc->rasterizer.face_winding, SG_FACEWINDING_CW) == SG_FACEWINDING_CCW;
    rs_desc.DepthBias = 0;
    rs_desc.DepthBiasClamp = 0.0f;
    rs_desc.SlopeScaledDepthBias = 0.0f;
    rs_desc.DepthClipEnable = TRUE;
    rs_desc.ScissorEnable = desc->rasterizer.scissor_test_enabled;
    rs_desc.MultisampleEnable = _sg_select(desc->rasterizer.sample_count, 1) > 1;
    rs_desc.AntialiasedLineEnable = FALSE;
    hr = ID3D11Device_CreateRasterizerState(_sg_d3d11.dev, &rs_desc, &pip->d3d11_rs);
    SOKOL_ASSERT(SUCCEEDED(hr) && pip->d3d11_rs);

    /* create depth-stencil state */
    D3D11_DEPTH_STENCIL_DESC dss_desc;
    memset(&dss_desc, 0, sizeof(dss_desc));
    dss_desc.DepthEnable = TRUE;
    dss_desc.DepthWriteMask = desc->depth_stencil.depth_write_enabled ? D3D11_DEPTH_WRITE_MASK_ALL : D3D11_DEPTH_WRITE_MASK_ZERO;
    dss_desc.DepthFunc = _sg_d3d11_compare_func(_sg_select(desc->depth_stencil.depth_compare_func, SG_COMPAREFUNC_ALWAYS));
    dss_desc.StencilEnable = desc->depth_stencil.stencil_enabled;
    dss_desc.StencilReadMask = desc->depth_stencil.stencil_read_mask;
    dss_desc.StencilWriteMask = desc->depth_stencil.stencil_write_mask;
    const sg_stencil_state* sf = &desc->depth_stencil.stencil_front;
    dss_desc.FrontFace.StencilFailOp = _sg_d3d11_stencil_op(_sg_select(sf->fail_op, SG_STENCILOP_KEEP));
    dss_desc.FrontFace.StencilDepthFailOp = _sg_d3d11_stencil_op(_sg_select(sf->depth_fail_op, SG_STENCILOP_KEEP));
    dss_desc.FrontFace.StencilPassOp = _sg_d3d11_stencil_op(_sg_select(sf->pass_op, SG_STENCILOP_KEEP));
    dss_desc.FrontFace.StencilFunc = _sg_d3d11_compare_func(_sg_select(sf->compare_func, SG_COMPAREFUNC_ALWAYS));
    const sg_stencil_state* sb = &desc->depth_stencil.stencil_back;
    dss_desc.BackFace.StencilFailOp = _sg_d3d11_stencil_op(_sg_select(sb->fail_op, SG_STENCILOP_KEEP));
    dss_desc.BackFace.StencilDepthFailOp = _sg_d3d11_stencil_op(_sg_select(sb->depth_fail_op, SG_STENCILOP_KEEP));
    dss_desc.BackFace.StencilPassOp = _sg_d3d11_stencil_op(_sg_select(sb->pass_op, SG_STENCILOP_KEEP));
    dss_desc.BackFace.StencilFunc = _sg_d3d11_compare_func(_sg_select(sb->compare_func, SG_COMPAREFUNC_ALWAYS));
    hr = ID3D11Device_CreateDepthStencilState(_sg_d3d11.dev, &dss_desc, &pip->d3d11_dss);
    SOKOL_ASSERT(SUCCEEDED(hr) && pip->d3d11_dss);
    
    /* create blend state */
    D3D11_BLEND_DESC bs_desc;
    memset(&bs_desc, 0, sizeof(bs_desc));
    bs_desc.AlphaToCoverageEnable = desc->rasterizer.alpha_to_coverage_enabled;
    bs_desc.IndependentBlendEnable = FALSE;
    bs_desc.RenderTarget[0].BlendEnable = desc->blend.enabled;
    bs_desc.RenderTarget[0].SrcBlend = _sg_d3d11_blend_factor(_sg_select(desc->blend.src_factor_rgb, SG_BLENDFACTOR_ONE));
    bs_desc.RenderTarget[0].DestBlend = _sg_d3d11_blend_factor(_sg_select(desc->blend.dst_factor_rgb, SG_BLENDFACTOR_ZERO));
    bs_desc.RenderTarget[0].BlendOp = _sg_d3d11_blend_op(_sg_select(desc->blend.op_rgb, SG_BLENDOP_ADD));
    bs_desc.RenderTarget[0].SrcBlendAlpha = _sg_d3d11_blend_factor(_sg_select(desc->blend.src_factor_alpha, SG_BLENDFACTOR_ONE));
    bs_desc.RenderTarget[0].DestBlendAlpha = _sg_d3d11_blend_factor(_sg_select(desc->blend.dst_factor_alpha, SG_BLENDFACTOR_ZERO));
    bs_desc.RenderTarget[0].BlendOpAlpha = _sg_d3d11_blend_op(_sg_select(desc->blend.op_alpha, SG_BLENDOP_ADD));
    bs_desc.RenderTarget[0].RenderTargetWriteMask = _sg_d3d11_color_write_mask(_sg_select(desc->blend.color_write_mask, SG_COLORMASK_RGBA));
    hr = ID3D11Device_CreateBlendState(_sg_d3d11.dev, &bs_desc, &pip->d3d11_bs);
    SOKOL_ASSERT(SUCCEEDED(hr) && pip->d3d11_bs);

    pip->slot.state = SG_RESOURCESTATE_VALID;
}

_SOKOL_PRIVATE void _sg_destroy_pipeline(_sg_pipeline* pip) {
    SOKOL_ASSERT(pip);
    if (pip->d3d11_il) {
        ID3D11InputLayout_Release(pip->d3d11_il);
    }
    if (pip->d3d11_rs) {
        ID3D11RasterizerState_Release(pip->d3d11_rs);
    }
    if (pip->d3d11_dss) {
        ID3D11DepthStencilState_Release(pip->d3d11_dss);
    }
    if (pip->d3d11_bs) {
        ID3D11BlendState_Release(pip->d3d11_bs);
    }
    _sg_init_pipeline(pip);
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
    ID3D11DeviceContext_OMSetRenderTargets(_sg_d3d11.ctx, SG_MAX_COLOR_ATTACHMENTS, _sg_d3d11.cur_rtvs, _sg_d3d11.cur_dsv);

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
    _sg_d3d11_clear_state();
}

_SOKOL_PRIVATE void _sg_apply_viewport(int x, int y, int w, int h, bool origin_top_left) {
    SOKOL_ASSERT(_sg_d3d11.ctx);
    SOKOL_ASSERT(_sg_d3d11.in_pass);
    D3D11_VIEWPORT vp;
    vp.TopLeftX = (FLOAT) x;
    vp.TopLeftY = (FLOAT) (origin_top_left ? y : (_sg_d3d11.cur_height - (y + h)));
    vp.Width = (FLOAT) w;
    vp.Height = (FLOAT) h;
    vp.MinDepth = 0.0f;
    vp.MaxDepth = 1.0f;
    ID3D11DeviceContext_RSSetViewports(_sg_d3d11.ctx, 1, &vp);
}

_SOKOL_PRIVATE void _sg_apply_scissor_rect(int x, int y, int w, int h, bool origin_top_left) {
    SOKOL_ASSERT(_sg_d3d11.ctx);
    SOKOL_ASSERT(_sg_d3d11.in_pass);
    D3D11_RECT rect;
    rect.left = x;
    rect.top = (origin_top_left ? y : (_sg_d3d11.cur_height - (y + h)));
    rect.right = x + w;
    rect.bottom = origin_top_left ? (y + h) : (_sg_d3d11.cur_height - y);
    ID3D11DeviceContext_RSSetScissorRects(_sg_d3d11.ctx, 1, &rect);
}

_SOKOL_PRIVATE void _sg_apply_draw_state(
    _sg_pipeline* pip,
    _sg_buffer** vbs, int num_vbs, _sg_buffer* ib,
    _sg_image** vs_imgs, int num_vs_imgs,
    _sg_image** fs_imgs, int num_fs_imgs)
{
    SOKOL_ASSERT(pip);
    SOKOL_ASSERT(pip->shader);
    SOKOL_ASSERT(_sg_d3d11.ctx);
    SOKOL_ASSERT(_sg_d3d11.in_pass);
    SOKOL_ASSERT(pip->d3d11_rs && pip->d3d11_bs && pip->d3d11_dss && pip->d3d11_il);

    _sg_d3d11.use_indexed_draw = (pip->d3d11_index_format != DXGI_FORMAT_UNKNOWN);

    /* FIXME: is it worth it to implement a state cache here? measure! */
    ID3D11DeviceContext_RSSetState(_sg_d3d11.ctx, pip->d3d11_rs);
    ID3D11DeviceContext_OMSetDepthStencilState(_sg_d3d11.ctx, pip->d3d11_dss, pip->d3d11_stencil_ref);
    ID3D11DeviceContext_OMSetBlendState(_sg_d3d11.ctx, pip->d3d11_bs, pip->blend_color, 0xFFFFFFFF);
    ID3D11Buffer* d3d11_vbs[SG_MAX_SHADERSTAGE_BUFFERS];
    UINT d3d11_offsets[SG_MAX_SHADERSTAGE_BUFFERS];
    int vb_slot;
    for (vb_slot = 0; vb_slot < num_vbs; vb_slot++) {
        SOKOL_ASSERT(vbs[vb_slot]->d3d11_buf);
        d3d11_vbs[vb_slot] = vbs[vb_slot]->d3d11_buf;
        d3d11_offsets[vb_slot] = 0;
    }
    for (; vb_slot < SG_MAX_SHADERSTAGE_BUFFERS; vb_slot++) {
        d3d11_vbs[vb_slot] = 0; 
        d3d11_offsets[vb_slot] = 0;
    }
    ID3D11DeviceContext_IASetVertexBuffers(_sg_d3d11.ctx, 0, SG_MAX_SHADERSTAGE_BUFFERS, d3d11_vbs, pip->d3d11_vb_strides, d3d11_offsets);
    ID3D11DeviceContext_IASetPrimitiveTopology(_sg_d3d11.ctx, pip->d3d11_topology);
    ID3D11Buffer* d3d11_ib = ib ? ib->d3d11_buf : 0;
    ID3D11DeviceContext_IASetIndexBuffer(_sg_d3d11.ctx, d3d11_ib, pip->d3d11_index_format, 0); 
    ID3D11DeviceContext_IASetInputLayout(_sg_d3d11.ctx, pip->d3d11_il);
    ID3D11DeviceContext_VSSetShader(_sg_d3d11.ctx, pip->shader->d3d11_vs, NULL, 0);
    ID3D11DeviceContext_PSSetShader(_sg_d3d11.ctx, pip->shader->d3d11_fs, NULL, 0);
    ID3D11DeviceContext_VSSetConstantBuffers(_sg_d3d11.ctx, 0, SG_MAX_SHADERSTAGE_UBS, pip->shader->stage[SG_SHADERSTAGE_VS].d3d11_cbs);
    ID3D11DeviceContext_PSSetConstantBuffers(_sg_d3d11.ctx, 0, SG_MAX_SHADERSTAGE_UBS, pip->shader->stage[SG_SHADERSTAGE_FS].d3d11_cbs);

    // FIXME: apply images and samplers
}

_SOKOL_PRIVATE void _sg_apply_uniform_block(sg_shader_stage stage_index, int ub_index, const void* data, int num_bytes) {
    // FIXME
}

_SOKOL_PRIVATE void _sg_draw(int base_element, int num_elements, int num_instances) {
    SOKOL_ASSERT(_sg_d3d11.in_pass);
    if (_sg_d3d11.use_indexed_draw) {
        if (1 == num_instances) {
            ID3D11DeviceContext_DrawIndexed(_sg_d3d11.ctx, num_elements, base_element, 0);
        }
        else {
            ID3D11DeviceContext_DrawIndexedInstanced(_sg_d3d11.ctx, num_elements, num_instances, base_element, 0, 0);
        }
    }
    else {
        if (1 == num_instances) {
            ID3D11DeviceContext_Draw(_sg_d3d11.ctx, num_elements, base_element);
        }
        else {
            ID3D11DeviceContext_DrawInstanced(_sg_d3d11.ctx, num_elements, num_instances, base_element, 0);
        }
    }
}

_SOKOL_PRIVATE void _sg_commit() {
    SOKOL_ASSERT(!_sg_d3d11.in_pass);
    _sg_d3d11.frame_index++;
}

_SOKOL_PRIVATE void _sg_update_buffer(_sg_buffer* buf, const void* data_ptr, int data_size) {
    // FIXME
}

_SOKOL_PRIVATE void _sg_update_image(_sg_image* img, const sg_image_content* data) {
    // FIXME
}

_SOKOL_PRIVATE void _sg_reset_state_cache() {
    /* just clear the d3d11 device context state */
    _sg_d3d11_clear_state();
}

#ifdef __cplusplus
} // extern "C"
#endif
