/*
    Sokol Gfx D3D11 rendering backend.
*/
#ifndef SOKOL_IMPL_GUARD
#error "Please do not include *.impl.h files directly"
#endif

#ifndef D3D11_NO_HELPERS
#define D3D11_NO_HELPERS
#endif
#ifndef CINTERFACE
#define CINTERFACE
#endif
#ifndef COBJMACROS
#define COBJMACROS
#endif
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
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
            return (D3D11_USAGE) 0;
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

_SOKOL_PRIVATE DXGI_FORMAT _sg_d3d11_texture_format(sg_pixel_format fmt) {
    switch (fmt) {
        case SG_PIXELFORMAT_RGBA8:          return DXGI_FORMAT_R8G8B8A8_UNORM;
        case SG_PIXELFORMAT_RGBA4:          return DXGI_FORMAT_B4G4R4A4_UNORM;
        case SG_PIXELFORMAT_R5G6B5:         return DXGI_FORMAT_B5G6R5_UNORM;
        case SG_PIXELFORMAT_R5G5B5A1:       return DXGI_FORMAT_B5G5R5A1_UNORM;
        case SG_PIXELFORMAT_R10G10B10A2:    return DXGI_FORMAT_R10G10B10A2_UNORM;
        case SG_PIXELFORMAT_RGBA32F:        return DXGI_FORMAT_R32G32B32A32_FLOAT;
        case SG_PIXELFORMAT_RGBA16F:        return DXGI_FORMAT_R16G16B16A16_FLOAT;
        case SG_PIXELFORMAT_R32F:           return DXGI_FORMAT_R32_FLOAT;
        case SG_PIXELFORMAT_R16F:           return DXGI_FORMAT_R16_FLOAT;
        case SG_PIXELFORMAT_L8:             return DXGI_FORMAT_R8_UNORM;
        case SG_PIXELFORMAT_DXT1:           return DXGI_FORMAT_BC1_UNORM;
        case SG_PIXELFORMAT_DXT3:           return DXGI_FORMAT_BC2_UNORM;
        case SG_PIXELFORMAT_DXT5:           return DXGI_FORMAT_BC3_UNORM;
        default:                            return DXGI_FORMAT_UNKNOWN;
    };
}

_SOKOL_PRIVATE DXGI_FORMAT _sg_d3d11_rendertarget_color_format(sg_pixel_format fmt) {
    switch (fmt) {
        case SG_PIXELFORMAT_RGBA8:          return DXGI_FORMAT_R8G8B8A8_UNORM;
        case SG_PIXELFORMAT_RGBA32F:        return DXGI_FORMAT_R32G32B32A32_FLOAT;
        case SG_PIXELFORMAT_RGBA16F:        return DXGI_FORMAT_R16G16B16A16_FLOAT;
        case SG_PIXELFORMAT_R32F:           return DXGI_FORMAT_R32_FLOAT;
        case SG_PIXELFORMAT_R16F:           return DXGI_FORMAT_R16_FLOAT;
        case SG_PIXELFORMAT_L8:             return DXGI_FORMAT_R8_UNORM;
        default:                            return DXGI_FORMAT_UNKNOWN;
    }
}

_SOKOL_PRIVATE DXGI_FORMAT _sg_d3d11_rendertarget_depth_format(sg_pixel_format fmt) {
    switch (fmt) {
        case SG_PIXELFORMAT_DEPTH:          return DXGI_FORMAT_D16_UNORM;
        case SG_PIXELFORMAT_DEPTHSTENCIL:   return DXGI_FORMAT_D24_UNORM_S8_UINT;
        default:                            return DXGI_FORMAT_UNKNOWN;
    }
}

_SOKOL_PRIVATE D3D11_PRIMITIVE_TOPOLOGY _sg_d3d11_primitive_topology(sg_primitive_type prim_type) {
    switch (prim_type) {
        case SG_PRIMITIVETYPE_POINTS:           return D3D11_PRIMITIVE_TOPOLOGY_POINTLIST;
        case SG_PRIMITIVETYPE_LINES:            return D3D11_PRIMITIVE_TOPOLOGY_LINELIST;
        case SG_PRIMITIVETYPE_LINE_STRIP:       return D3D11_PRIMITIVE_TOPOLOGY_LINESTRIP;
        case SG_PRIMITIVETYPE_TRIANGLES:        return D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
        case SG_PRIMITIVETYPE_TRIANGLE_STRIP:   return D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP;
        default: SOKOL_UNREACHABLE; return (D3D11_PRIMITIVE_TOPOLOGY) 0;
    }
}

_SOKOL_PRIVATE DXGI_FORMAT _sg_d3d11_index_format(sg_index_type index_type) {
    switch (index_type) {
        case SG_INDEXTYPE_NONE:     return DXGI_FORMAT_UNKNOWN;
        case SG_INDEXTYPE_UINT16:   return DXGI_FORMAT_R16_UINT;
        case SG_INDEXTYPE_UINT32:   return DXGI_FORMAT_R32_UINT;
        default: SOKOL_UNREACHABLE; return (DXGI_FORMAT) 0;
    }
}

_SOKOL_PRIVATE D3D11_FILTER _sg_d3d11_filter(sg_filter min_f, sg_filter mag_f, uint32_t max_anisotropy) {
    if (max_anisotropy > 1) {
        return D3D11_FILTER_ANISOTROPIC;
    }
    else if (mag_f == SG_FILTER_NEAREST) {
        switch (min_f) {
            case SG_FILTER_NEAREST:
            case SG_FILTER_NEAREST_MIPMAP_NEAREST:
                return D3D11_FILTER_MIN_MAG_MIP_POINT;
            case SG_FILTER_LINEAR:
            case SG_FILTER_LINEAR_MIPMAP_NEAREST:
                return D3D11_FILTER_MIN_LINEAR_MAG_MIP_POINT;
            case SG_FILTER_NEAREST_MIPMAP_LINEAR:
                return D3D11_FILTER_MIN_MAG_POINT_MIP_LINEAR;
            case SG_FILTER_LINEAR_MIPMAP_LINEAR:
                return D3D11_FILTER_MIN_LINEAR_MAG_POINT_MIP_LINEAR;
            default:
                SOKOL_UNREACHABLE; break;
        }
    }
    else if (mag_f == SG_FILTER_LINEAR) {
        switch (min_f) {
            case SG_FILTER_NEAREST:
            case SG_FILTER_NEAREST_MIPMAP_NEAREST:
                return D3D11_FILTER_MIN_POINT_MAG_LINEAR_MIP_POINT;
            case SG_FILTER_LINEAR:
            case SG_FILTER_LINEAR_MIPMAP_NEAREST:
                return D3D11_FILTER_MIN_MAG_LINEAR_MIP_POINT;
            case SG_FILTER_NEAREST_MIPMAP_LINEAR:
                return D3D11_FILTER_MIN_POINT_MAG_MIP_LINEAR;
            case SG_FILTER_LINEAR_MIPMAP_LINEAR:
                return D3D11_FILTER_MIN_MAG_MIP_LINEAR;
            default:
                SOKOL_UNREACHABLE; break;
        }
    }
    /* invalid value for mag filter */
    SOKOL_UNREACHABLE;
    return D3D11_FILTER_MIN_MAG_MIP_POINT;
}

_SOKOL_PRIVATE D3D11_TEXTURE_ADDRESS_MODE _sg_d3d11_address_mode(sg_wrap m) {
    switch (m) {
        case SG_WRAP_REPEAT:            return D3D11_TEXTURE_ADDRESS_WRAP;
        case SG_WRAP_CLAMP_TO_EDGE:     return D3D11_TEXTURE_ADDRESS_CLAMP;
        case SG_WRAP_MIRRORED_REPEAT:   return D3D11_TEXTURE_ADDRESS_MIRROR;
        default: SOKOL_UNREACHABLE; return (D3D11_TEXTURE_ADDRESS_MODE) 0;
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
        default: SOKOL_UNREACHABLE; return (DXGI_FORMAT) 0;
    }
}

_SOKOL_PRIVATE D3D11_INPUT_CLASSIFICATION _sg_d3d11_input_classification(sg_vertex_step step) {
    switch (step) {
        case SG_VERTEXSTEP_PER_VERTEX:      return D3D11_INPUT_PER_VERTEX_DATA;
        case SG_VERTEXSTEP_PER_INSTANCE:    return D3D11_INPUT_PER_INSTANCE_DATA;
        default: SOKOL_UNREACHABLE; return (D3D11_INPUT_CLASSIFICATION) 0;
    }
}

_SOKOL_PRIVATE D3D11_CULL_MODE _sg_d3d11_cull_mode(sg_cull_mode m) {
    switch (m) {
        case SG_CULLMODE_NONE:      return D3D11_CULL_NONE;
        case SG_CULLMODE_FRONT:     return D3D11_CULL_FRONT;
        case SG_CULLMODE_BACK:      return D3D11_CULL_BACK;
        default: SOKOL_UNREACHABLE; return (D3D11_CULL_MODE) 0;
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
        default: SOKOL_UNREACHABLE; return (D3D11_COMPARISON_FUNC) 0;
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
        default: SOKOL_UNREACHABLE; return (D3D11_STENCIL_OP) 0;
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
        default: SOKOL_UNREACHABLE; return (D3D11_BLEND) 0;
    }
}

_SOKOL_PRIVATE D3D11_BLEND_OP _sg_d3d11_blend_op(sg_blend_op op) {
    switch (op) {
        case SG_BLENDOP_ADD:                return D3D11_BLEND_OP_ADD;
        case SG_BLENDOP_SUBTRACT:           return D3D11_BLEND_OP_SUBTRACT;
        case SG_BLENDOP_REVERSE_SUBTRACT:   return D3D11_BLEND_OP_REV_SUBTRACT;
        default: SOKOL_UNREACHABLE; return (D3D11_BLEND_OP) 0;
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
    uint32_t max_anisotropy;
    int upd_frame_index;
    DXGI_FORMAT d3d11_format;
    ID3D11Texture2D* d3d11_tex2d;
    ID3D11Texture3D* d3d11_tex3d;
    ID3D11Texture2D* d3d11_texds;
    ID3D11Texture2D* d3d11_texmsaa;
    ID3D11ShaderResourceView* d3d11_srv;
    ID3D11SamplerState* d3d11_smp;
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
    bool vertex_layout_valid[SG_MAX_SHADERSTAGE_BUFFERS];
    int color_attachment_count;
    sg_pixel_format color_format;
    sg_pixel_format depth_format;
    int sample_count;
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
    int num_color_atts;
    _sg_attachment color_atts[SG_MAX_COLOR_ATTACHMENTS];
    _sg_attachment ds_att;
    ID3D11RenderTargetView* d3d11_rtvs[SG_MAX_COLOR_ATTACHMENTS];
    ID3D11DepthStencilView* d3d11_dsv;
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
    int cur_width;
    int cur_height;
    int num_rtvs;
    _sg_pass* cur_pass;
    sg_pass cur_pass_id;
    _sg_pipeline* cur_pipeline;
    sg_pipeline cur_pipeline_id;
    ID3D11RenderTargetView* cur_rtvs[SG_MAX_COLOR_ATTACHMENTS];
    ID3D11DepthStencilView* cur_dsv;
    /* the following arrays are used for unbinding resources, they will always contain zeroes */ 
    ID3D11RenderTargetView* zero_rtvs[SG_MAX_COLOR_ATTACHMENTS];
    ID3D11Buffer* zero_vbs[SG_MAX_SHADERSTAGE_BUFFERS];
    UINT zero_vb_offsets[SG_MAX_SHADERSTAGE_BUFFERS];
    UINT zero_vb_strides[SG_MAX_SHADERSTAGE_BUFFERS];
    ID3D11Buffer* zero_cbs[SG_MAX_SHADERSTAGE_UBS];
    ID3D11ShaderResourceView* zero_srvs[SG_MAX_SHADERSTAGE_IMAGES];
    ID3D11SamplerState* zero_smps[SG_MAX_SHADERSTAGE_IMAGES];
    /* global subresourcedata array for texture updates */
    D3D11_SUBRESOURCE_DATA subres_data[SG_MAX_MIPMAPS * SG_MAX_TEXTUREARRAY_LAYERS];
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
        case SG_FEATURE_INSTANCING:
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
    ID3D11DeviceContext_VSSetShaderResources(_sg_d3d11.ctx, 0, SG_MAX_SHADERSTAGE_IMAGES, _sg_d3d11.zero_srvs);
    ID3D11DeviceContext_PSSetShaderResources(_sg_d3d11.ctx, 0, SG_MAX_SHADERSTAGE_IMAGES, _sg_d3d11.zero_srvs);
    ID3D11DeviceContext_VSSetSamplers(_sg_d3d11.ctx, 0, SG_MAX_SHADERSTAGE_IMAGES, _sg_d3d11.zero_smps);
    ID3D11DeviceContext_PSSetSamplers(_sg_d3d11.ctx, 0, SG_MAX_SHADERSTAGE_IMAGES, _sg_d3d11.zero_smps);
}

_SOKOL_PRIVATE void _sg_create_buffer(_sg_buffer* buf, const sg_buffer_desc* desc) {
    SOKOL_ASSERT(buf && desc);
    SOKOL_ASSERT(buf->slot.state == SG_RESOURCESTATE_ALLOC);
    SOKOL_ASSERT(!buf->d3d11_buf);
    buf->size = desc->size;
    buf->type = _sg_def(desc->type, SG_BUFFERTYPE_VERTEXBUFFER);
    buf->usage = _sg_def(desc->usage, SG_USAGE_IMMUTABLE);
    buf->upd_frame_index = 0;
    const bool injected = (0 != desc->d3d11_buffer);
    if (injected) {
        buf->d3d11_buf = (ID3D11Buffer*) desc->d3d11_buffer;
        ID3D11Buffer_AddRef(buf->d3d11_buf);
    }
    else {
        D3D11_BUFFER_DESC d3d11_desc;
        memset(&d3d11_desc, 0, sizeof(d3d11_desc));
        d3d11_desc.ByteWidth = buf->size;
        d3d11_desc.Usage = _sg_d3d11_usage(buf->usage);
        d3d11_desc.BindFlags = buf->type == SG_BUFFERTYPE_VERTEXBUFFER ? D3D11_BIND_VERTEX_BUFFER : D3D11_BIND_INDEX_BUFFER;
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
    }
    buf->slot.state = SG_RESOURCESTATE_VALID;
}

_SOKOL_PRIVATE void _sg_destroy_buffer(_sg_buffer* buf) {
    SOKOL_ASSERT(buf);
    if (buf->d3d11_buf) {
        ID3D11Buffer_Release(buf->d3d11_buf);
    }
    _sg_init_buffer(buf);
}

_SOKOL_PRIVATE void _sg_d3d11_fill_subres_data(const _sg_image* img, const sg_image_content* content) {
    const int num_faces = (img->type == SG_IMAGETYPE_CUBE) ? 6:1;
    const int num_slices = (img->type == SG_IMAGETYPE_ARRAY) ? img->depth:1;
    int subres_index = 0;
    for (int face_index = 0; face_index < num_faces; face_index++) {
        for (int slice_index = 0; slice_index < num_slices; slice_index++) {
            for (int mip_index = 0; mip_index < img->num_mipmaps; mip_index++, subres_index++) {
                SOKOL_ASSERT(subres_index < (SG_MAX_MIPMAPS * SG_MAX_TEXTUREARRAY_LAYERS));
                D3D11_SUBRESOURCE_DATA* subres_data = &_sg_d3d11.subres_data[subres_index];
                const int mip_width = ((img->width>>mip_index)>0) ? img->width>>mip_index : 1;
                const int mip_height = ((img->height>>mip_index)>0) ? img->height>>mip_index : 1;
                const sg_subimage_content* subimg_content = &(content->subimage[face_index][mip_index]);
                const int slice_size = subimg_content->size / num_slices;
                const int slice_offset = slice_size * slice_index;
                const uint8_t* ptr = (const uint8_t*) subimg_content->ptr;
                subres_data->pSysMem = ptr + slice_offset;
                subres_data->SysMemPitch = _sg_row_pitch(img->pixel_format, mip_width);
                if (img->type == SG_IMAGETYPE_3D) {
                    const int mip_depth = ((img->depth>>mip_index)>0) ? img->depth>>mip_index : 1;
                    subres_data->SysMemSlicePitch = _sg_surface_pitch(img->pixel_format, mip_width, mip_height);
                }
                else {
                    subres_data->SysMemSlicePitch = 0;
                }
            }
        }
    }
}

_SOKOL_PRIVATE void _sg_create_image(_sg_image* img, const sg_image_desc* desc) {
    SOKOL_ASSERT(img && desc);
    SOKOL_ASSERT(img->slot.state == SG_RESOURCESTATE_ALLOC);
    SOKOL_ASSERT(!img->d3d11_tex2d && !img->d3d11_tex3d && !img->d3d11_texds && !img->d3d11_texmsaa);
    SOKOL_ASSERT(!img->d3d11_srv && !img->d3d11_smp);
    HRESULT hr;
    
    img->type = _sg_def(desc->type, SG_IMAGETYPE_2D);
    img->render_target = desc->render_target;
    img->width = desc->width;
    img->height = desc->height;
    img->depth = _sg_def(desc->depth, 1);
    img->num_mipmaps = _sg_def(desc->num_mipmaps, 1);
    img->usage = _sg_def(desc->usage, SG_USAGE_IMMUTABLE);
    img->pixel_format = _sg_def(desc->pixel_format, SG_PIXELFORMAT_RGBA8);
    img->sample_count = _sg_def(desc->sample_count, 1);
    img->min_filter = _sg_def(desc->min_filter, SG_FILTER_NEAREST);
    img->mag_filter = _sg_def(desc->mag_filter, SG_FILTER_NEAREST);
    img->wrap_u = _sg_def(desc->wrap_u, SG_WRAP_REPEAT);
    img->wrap_v = _sg_def(desc->wrap_v, SG_WRAP_REPEAT);
    img->wrap_w = _sg_def(desc->wrap_w, SG_WRAP_REPEAT);
    img->max_anisotropy = _sg_def(desc->max_anisotropy, 1);
    img->upd_frame_index = 0;
    const bool injected = (0 != desc->d3d11_texture);

    /* special case depth-stencil buffer? */
    if (_sg_is_valid_rendertarget_depth_format(img->pixel_format)) {
        /* create only a depth-texture */
        SOKOL_ASSERT(!injected);
        img->d3d11_format = _sg_d3d11_rendertarget_depth_format(img->pixel_format);
        D3D11_TEXTURE2D_DESC d3d11_desc;
        memset(&d3d11_desc, 0, sizeof(d3d11_desc));
        d3d11_desc.Width = img->width;
        d3d11_desc.Height = img->height;
        d3d11_desc.MipLevels = 1;
        d3d11_desc.ArraySize = 1;
        d3d11_desc.Format = img->d3d11_format;
        d3d11_desc.Usage = D3D11_USAGE_DEFAULT;
        d3d11_desc.BindFlags = D3D11_BIND_DEPTH_STENCIL;
        d3d11_desc.SampleDesc.Count = img->sample_count;
        d3d11_desc.SampleDesc.Quality = (img->sample_count > 1) ? D3D11_STANDARD_MULTISAMPLE_PATTERN : 0;
        hr = ID3D11Device_CreateTexture2D(_sg_d3d11.dev, &d3d11_desc, NULL, &img->d3d11_texds);
        SOKOL_ASSERT(SUCCEEDED(hr) && img->d3d11_texds);
    }
    else {
        /* create (or inject) color texture */

        /* prepare initial content pointers */
        D3D11_SUBRESOURCE_DATA* init_data = 0;
        if (!injected && (img->usage == SG_USAGE_IMMUTABLE) && !img->render_target) {
            _sg_d3d11_fill_subres_data(img, &desc->content);
            init_data = _sg_d3d11.subres_data;
        }
        if (img->type != SG_IMAGETYPE_3D) {
            /* 2D-, cube- or array-texture */
            /* if this is an MSAA render target, the following texture will be the 'resolve-texture' */
            D3D11_TEXTURE2D_DESC d3d11_tex_desc;
            memset(&d3d11_tex_desc, 0, sizeof(d3d11_tex_desc));
            d3d11_tex_desc.Width = img->width;
            d3d11_tex_desc.Height = img->height;
            d3d11_tex_desc.MipLevels = img->num_mipmaps;
            switch (img->type) {
                case SG_IMAGETYPE_ARRAY:    d3d11_tex_desc.ArraySize = img->depth; break;
                case SG_IMAGETYPE_CUBE:     d3d11_tex_desc.ArraySize = 6; break;
                default:                    d3d11_tex_desc.ArraySize = 1; break;
            }
            d3d11_tex_desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
            if (img->render_target) {
                img->d3d11_format = _sg_d3d11_rendertarget_color_format(img->pixel_format);
                d3d11_tex_desc.Format = img->d3d11_format;
                d3d11_tex_desc.Usage = D3D11_USAGE_DEFAULT;
                if (img->sample_count == 1) {
                    d3d11_tex_desc.BindFlags |= D3D11_BIND_RENDER_TARGET;
                }
                d3d11_tex_desc.CPUAccessFlags = 0;
            }
            else {
                img->d3d11_format = _sg_d3d11_texture_format(img->pixel_format);
                d3d11_tex_desc.Format = img->d3d11_format;
                d3d11_tex_desc.Usage = _sg_d3d11_usage(img->usage);
                d3d11_tex_desc.CPUAccessFlags = _sg_d3d11_cpu_access_flags(img->usage);
            }
            d3d11_tex_desc.SampleDesc.Count = 1;
            d3d11_tex_desc.SampleDesc.Quality = 0;
            d3d11_tex_desc.MiscFlags = (img->type == SG_IMAGETYPE_CUBE) ? D3D11_RESOURCE_MISC_TEXTURECUBE : 0;
            if (injected) {
                img->d3d11_tex2d = (ID3D11Texture2D*) desc->d3d11_texture;
                ID3D11Texture2D_AddRef(img->d3d11_tex2d);
            }
            else {
                hr = ID3D11Device_CreateTexture2D(_sg_d3d11.dev, &d3d11_tex_desc, init_data, &img->d3d11_tex2d);
                SOKOL_ASSERT(SUCCEEDED(hr) && img->d3d11_tex2d);
            }

            /* also need to create a separate MSAA render target texture? */
            if (img->sample_count > 1) {
                d3d11_tex_desc.BindFlags |= D3D11_BIND_RENDER_TARGET;
                d3d11_tex_desc.SampleDesc.Count = img->sample_count;
                d3d11_tex_desc.SampleDesc.Quality = D3D11_STANDARD_MULTISAMPLE_PATTERN;
                hr = ID3D11Device_CreateTexture2D(_sg_d3d11.dev, &d3d11_tex_desc, NULL, &img->d3d11_texmsaa);
                SOKOL_ASSERT(SUCCEEDED(hr) && img->d3d11_texmsaa);
            }

            /* shader-resource-view */
            D3D11_SHADER_RESOURCE_VIEW_DESC d3d11_srv_desc;
            memset(&d3d11_srv_desc, 0, sizeof(d3d11_srv_desc));
            d3d11_srv_desc.Format = d3d11_tex_desc.Format;
            switch (img->type) {
                case SG_IMAGETYPE_2D:
                    d3d11_srv_desc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
                    d3d11_srv_desc.Texture2D.MipLevels = img->num_mipmaps;
                    break;
                case SG_IMAGETYPE_CUBE:
                    d3d11_srv_desc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURECUBE;
                    d3d11_srv_desc.TextureCube.MipLevels = img->num_mipmaps;
                    break;
                case SG_IMAGETYPE_ARRAY:
                    d3d11_srv_desc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2DARRAY;
                    d3d11_srv_desc.Texture2DArray.MipLevels = img->num_mipmaps;
                    d3d11_srv_desc.Texture2DArray.ArraySize = img->depth;
                    break;
                default:
                    SOKOL_UNREACHABLE; break;
            }
            hr = ID3D11Device_CreateShaderResourceView(_sg_d3d11.dev, (ID3D11Resource*)img->d3d11_tex2d, &d3d11_srv_desc, &img->d3d11_srv);
            SOKOL_ASSERT(SUCCEEDED(hr) && img->d3d11_srv);
        }
        else {
            /* 3D texture */
            D3D11_TEXTURE3D_DESC d3d11_tex_desc;
            memset(&d3d11_tex_desc, 0, sizeof(d3d11_tex_desc));
            d3d11_tex_desc.Width = img->width;
            d3d11_tex_desc.Height = img->height;
            d3d11_tex_desc.Depth = img->depth;
            d3d11_tex_desc.MipLevels = img->num_mipmaps;
            if (img->render_target) {
                img->d3d11_format = _sg_d3d11_rendertarget_color_format(img->pixel_format);
                d3d11_tex_desc.Format = img->d3d11_format;
                d3d11_tex_desc.Usage = D3D11_USAGE_DEFAULT;
                d3d11_tex_desc.BindFlags = D3D11_BIND_SHADER_RESOURCE|D3D11_BIND_RENDER_TARGET;
                d3d11_tex_desc.CPUAccessFlags = 0;
            }
            else {
                img->d3d11_format = _sg_d3d11_texture_format(img->pixel_format);
                d3d11_tex_desc.Format = img->d3d11_format;
                d3d11_tex_desc.Usage = _sg_d3d11_usage(img->usage);
                d3d11_tex_desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
                d3d11_tex_desc.CPUAccessFlags = _sg_d3d11_cpu_access_flags(img->usage);
            }
            if (injected) {
                img->d3d11_tex3d = (ID3D11Texture3D*) desc->d3d11_texture;
                ID3D11Texture3D_AddRef(img->d3d11_tex3d);
            }
            else {
                hr = ID3D11Device_CreateTexture3D(_sg_d3d11.dev, &d3d11_tex_desc, init_data, &img->d3d11_tex3d);
                SOKOL_ASSERT(SUCCEEDED(hr) && img->d3d11_tex3d);
            }

            /* shader resource view for 3d texture */
            D3D11_SHADER_RESOURCE_VIEW_DESC d3d11_srv_desc;
            memset(&d3d11_srv_desc, 0, sizeof(d3d11_srv_desc));
            d3d11_srv_desc.Format = d3d11_tex_desc.Format;
            d3d11_srv_desc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE3D;
            d3d11_srv_desc.Texture3D.MipLevels = img->num_mipmaps;
            hr = ID3D11Device_CreateShaderResourceView(_sg_d3d11.dev, (ID3D11Resource*)img->d3d11_tex3d, &d3d11_srv_desc, &img->d3d11_srv);
            SOKOL_ASSERT(SUCCEEDED(hr) && img->d3d11_srv);
        }

        /* sampler state object, note D3D11 implements an internal shared-pool for sampler objects */
        D3D11_SAMPLER_DESC d3d11_smp_desc;
        memset(&d3d11_smp_desc, 0, sizeof(d3d11_smp_desc));
        d3d11_smp_desc.Filter = _sg_d3d11_filter(img->min_filter, img->mag_filter, img->max_anisotropy);
        d3d11_smp_desc.AddressU = _sg_d3d11_address_mode(img->wrap_u);
        d3d11_smp_desc.AddressV = _sg_d3d11_address_mode(img->wrap_v);
        d3d11_smp_desc.AddressW = _sg_d3d11_address_mode(img->wrap_w);
        d3d11_smp_desc.MaxAnisotropy = img->max_anisotropy;
        d3d11_smp_desc.ComparisonFunc = D3D11_COMPARISON_NEVER;
        d3d11_smp_desc.MinLOD = desc->min_lod;
        d3d11_smp_desc.MaxLOD = _sg_def_flt(desc->max_lod, D3D11_FLOAT32_MAX);
        hr = ID3D11Device_CreateSamplerState(_sg_d3d11.dev, &d3d11_smp_desc, &img->d3d11_smp);
        SOKOL_ASSERT(SUCCEEDED(hr) && img->d3d11_smp);
    }
    SOKOL_ASSERT(img->d3d11_format != DXGI_FORMAT_UNKNOWN);
    img->slot.state = SG_RESOURCESTATE_VALID;
}

_SOKOL_PRIVATE void _sg_destroy_image(_sg_image* img) {
    SOKOL_ASSERT(img);
    if (img->d3d11_tex2d) {
        ID3D11Texture2D_Release(img->d3d11_tex2d);
    }
    if (img->d3d11_tex3d) {
        ID3D11Texture3D_Release(img->d3d11_tex3d);
    }
    if (img->d3d11_texds) {
        ID3D11Texture2D_Release(img->d3d11_texds);
    }
    if (img->d3d11_texmsaa) {
        ID3D11Texture2D_Release(img->d3d11_texmsaa);
    }
    if (img->d3d11_srv) {
        ID3D11ShaderResourceView_Release(img->d3d11_srv);
    }
    if (img->d3d11_smp) {
        ID3D11SamplerState_Release(img->d3d11_smp);
    }
    _sg_init_image(img);
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
        SOKOL_LOG((LPCSTR)ID3D10Blob_GetBufferPointer(errors));
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

    const void* vs_ptr = 0, *fs_ptr = 0;
    SIZE_T vs_length = 0, fs_length = 0;
    #if defined(SOKOL_D3D11_SHADER_COMPILER)
    ID3DBlob* vs_blob = 0, *fs_blob = 0;
    #endif
    if (desc->vs.byte_code && desc->fs.byte_code) {
        /* create from byte code */
        vs_ptr = desc->vs.byte_code;
        fs_ptr = desc->fs.byte_code;
        vs_length = desc->vs.byte_code_size;
        fs_length = desc->fs.byte_code_size;
    }
    else {
        /* compile shader code */
        #if defined(SOKOL_D3D11_SHADER_COMPILER)
        vs_blob = _sg_d3d11_compile_shader(&desc->vs, "vs_5_0");
        fs_blob = _sg_d3d11_compile_shader(&desc->fs, "ps_5_0");
        if (vs_blob && fs_blob) {
            vs_ptr = ID3D10Blob_GetBufferPointer(vs_blob);
            vs_length = ID3D10Blob_GetBufferSize(vs_blob);
            fs_ptr = ID3D10Blob_GetBufferPointer(fs_blob);
            fs_length = ID3D10Blob_GetBufferSize(fs_blob);
        }
        #endif
    }
    if (vs_ptr && fs_ptr && (vs_length > 0) && (fs_length > 0)) {
        /* create the D3D vertex- and pixel-shader objects */
        hr = ID3D11Device_CreateVertexShader(_sg_d3d11.dev, vs_ptr, vs_length, NULL, &shd->d3d11_vs);
        SOKOL_ASSERT(SUCCEEDED(hr) && shd->d3d11_vs);
        hr = ID3D11Device_CreatePixelShader(_sg_d3d11.dev, fs_ptr, fs_length, NULL, &shd->d3d11_fs);
        SOKOL_ASSERT(SUCCEEDED(hr) && shd->d3d11_fs);

        /* need to store the vertex shader byte code, this is needed later in sg_create_pipeline */
        shd->d3d11_vs_blob_length = vs_length;
        shd->d3d11_vs_blob = SOKOL_MALLOC(vs_length);
        SOKOL_ASSERT(shd->d3d11_vs_blob);
        memcpy(shd->d3d11_vs_blob, vs_ptr, vs_length);

        shd->slot.state = SG_RESOURCESTATE_VALID;
    }
    else {
        shd->slot.state = SG_RESOURCESTATE_FAILED;
    }
    #if defined(SOKOL_D3D11_SHADER_COMPILER)
    if (vs_blob) {
        ID3D10Blob_Release(vs_blob); vs_blob = 0;
    }
    if (fs_blob) {
        ID3D10Blob_Release(fs_blob); fs_blob = 0;
    }
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
    pip->index_type = _sg_def(desc->index_type, SG_INDEXTYPE_NONE);
    pip->color_attachment_count = _sg_def(desc->blend.color_attachment_count, 1);
    pip->color_format = _sg_def(desc->blend.color_format, SG_PIXELFORMAT_RGBA8);
    pip->depth_format = _sg_def(desc->blend.depth_format, SG_PIXELFORMAT_DEPTHSTENCIL);
    pip->sample_count = _sg_def(desc->rasterizer.sample_count, 1);
    pip->d3d11_index_format = _sg_d3d11_index_format(pip->index_type);
    pip->d3d11_topology = _sg_d3d11_primitive_topology(_sg_def(desc->primitive_type, SG_PRIMITIVETYPE_TRIANGLES));
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
        pip->vertex_layout_valid[layout_index] = true;
        pip->d3d11_vb_strides[layout_index] = layout_desc->stride;
        for (int attr_index = 0; attr_index < SG_MAX_VERTEX_ATTRIBUTES; attr_index++) {
            const sg_vertex_attr_desc* attr_desc = &layout_desc->attrs[attr_index];
            if (attr_desc->format == SG_VERTEXFORMAT_INVALID) {
                break;
            }
            SOKOL_ASSERT(d3d11_attr_index < SG_MAX_VERTEX_ATTRIBUTES);
            D3D11_INPUT_ELEMENT_DESC* d3d11_comp = &d3d11_comps[d3d11_attr_index++];
            d3d11_comp->SemanticName = attr_desc->sem_name;
            d3d11_comp->SemanticIndex = attr_desc->sem_index;
            d3d11_comp->Format = _sg_d3d11_vertex_format(attr_desc->format);
            d3d11_comp->InputSlot = layout_index;
            d3d11_comp->AlignedByteOffset = attr_desc->offset;
            const sg_vertex_step step_func = _sg_def(layout_desc->step_func, SG_VERTEXSTEP_PER_VERTEX);
            d3d11_comp->InputSlotClass = _sg_d3d11_input_classification(step_func);
            if (SG_VERTEXSTEP_PER_INSTANCE == step_func) {
                d3d11_comp->InstanceDataStepRate = _sg_def(layout_desc->step_rate, 1);
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
    rs_desc.CullMode = _sg_d3d11_cull_mode(_sg_def(desc->rasterizer.cull_mode, SG_CULLMODE_NONE));
    rs_desc.FrontCounterClockwise = _sg_def(desc->rasterizer.face_winding, SG_FACEWINDING_CW) == SG_FACEWINDING_CCW;
    rs_desc.DepthBias = desc->rasterizer.depth_bias;
    rs_desc.DepthBiasClamp = desc->rasterizer.depth_bias_clamp;
    rs_desc.SlopeScaledDepthBias = desc->rasterizer.depth_bias_slope_scale;
    rs_desc.DepthClipEnable = TRUE;
    rs_desc.ScissorEnable = TRUE;
    rs_desc.MultisampleEnable = _sg_def(desc->rasterizer.sample_count, 1) > 1;
    rs_desc.AntialiasedLineEnable = FALSE;
    hr = ID3D11Device_CreateRasterizerState(_sg_d3d11.dev, &rs_desc, &pip->d3d11_rs);
    SOKOL_ASSERT(SUCCEEDED(hr) && pip->d3d11_rs);

    /* create depth-stencil state */
    D3D11_DEPTH_STENCIL_DESC dss_desc;
    memset(&dss_desc, 0, sizeof(dss_desc));
    dss_desc.DepthEnable = TRUE;
    dss_desc.DepthWriteMask = desc->depth_stencil.depth_write_enabled ? D3D11_DEPTH_WRITE_MASK_ALL : D3D11_DEPTH_WRITE_MASK_ZERO;
    dss_desc.DepthFunc = _sg_d3d11_compare_func(_sg_def(desc->depth_stencil.depth_compare_func, SG_COMPAREFUNC_ALWAYS));
    dss_desc.StencilEnable = desc->depth_stencil.stencil_enabled;
    dss_desc.StencilReadMask = desc->depth_stencil.stencil_read_mask;
    dss_desc.StencilWriteMask = desc->depth_stencil.stencil_write_mask;
    const sg_stencil_state* sf = &desc->depth_stencil.stencil_front;
    dss_desc.FrontFace.StencilFailOp = _sg_d3d11_stencil_op(_sg_def(sf->fail_op, SG_STENCILOP_KEEP));
    dss_desc.FrontFace.StencilDepthFailOp = _sg_d3d11_stencil_op(_sg_def(sf->depth_fail_op, SG_STENCILOP_KEEP));
    dss_desc.FrontFace.StencilPassOp = _sg_d3d11_stencil_op(_sg_def(sf->pass_op, SG_STENCILOP_KEEP));
    dss_desc.FrontFace.StencilFunc = _sg_d3d11_compare_func(_sg_def(sf->compare_func, SG_COMPAREFUNC_ALWAYS));
    const sg_stencil_state* sb = &desc->depth_stencil.stencil_back;
    dss_desc.BackFace.StencilFailOp = _sg_d3d11_stencil_op(_sg_def(sb->fail_op, SG_STENCILOP_KEEP));
    dss_desc.BackFace.StencilDepthFailOp = _sg_d3d11_stencil_op(_sg_def(sb->depth_fail_op, SG_STENCILOP_KEEP));
    dss_desc.BackFace.StencilPassOp = _sg_d3d11_stencil_op(_sg_def(sb->pass_op, SG_STENCILOP_KEEP));
    dss_desc.BackFace.StencilFunc = _sg_d3d11_compare_func(_sg_def(sb->compare_func, SG_COMPAREFUNC_ALWAYS));
    hr = ID3D11Device_CreateDepthStencilState(_sg_d3d11.dev, &dss_desc, &pip->d3d11_dss);
    SOKOL_ASSERT(SUCCEEDED(hr) && pip->d3d11_dss);
    
    /* create blend state */
    D3D11_BLEND_DESC bs_desc;
    memset(&bs_desc, 0, sizeof(bs_desc));
    bs_desc.AlphaToCoverageEnable = desc->rasterizer.alpha_to_coverage_enabled;
    bs_desc.IndependentBlendEnable = FALSE;
    bs_desc.RenderTarget[0].BlendEnable = desc->blend.enabled;
    bs_desc.RenderTarget[0].SrcBlend = _sg_d3d11_blend_factor(_sg_def(desc->blend.src_factor_rgb, SG_BLENDFACTOR_ONE));
    bs_desc.RenderTarget[0].DestBlend = _sg_d3d11_blend_factor(_sg_def(desc->blend.dst_factor_rgb, SG_BLENDFACTOR_ZERO));
    bs_desc.RenderTarget[0].BlendOp = _sg_d3d11_blend_op(_sg_def(desc->blend.op_rgb, SG_BLENDOP_ADD));
    bs_desc.RenderTarget[0].SrcBlendAlpha = _sg_d3d11_blend_factor(_sg_def(desc->blend.src_factor_alpha, SG_BLENDFACTOR_ONE));
    bs_desc.RenderTarget[0].DestBlendAlpha = _sg_d3d11_blend_factor(_sg_def(desc->blend.dst_factor_alpha, SG_BLENDFACTOR_ZERO));
    bs_desc.RenderTarget[0].BlendOpAlpha = _sg_d3d11_blend_op(_sg_def(desc->blend.op_alpha, SG_BLENDOP_ADD));
    bs_desc.RenderTarget[0].RenderTargetWriteMask = _sg_d3d11_color_write_mask(_sg_def((sg_color_mask)desc->blend.color_write_mask, SG_COLORMASK_RGBA));
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
    SOKOL_ASSERT(pass && desc);
    SOKOL_ASSERT(pass->slot.state == SG_RESOURCESTATE_ALLOC);
    SOKOL_ASSERT(att_images && att_images[0]);
    SOKOL_ASSERT(_sg_d3d11.dev);

    const sg_attachment_desc* att_desc;
    _sg_attachment* att;
    for (int i = 0; i < SG_MAX_COLOR_ATTACHMENTS; i++) {
        SOKOL_ASSERT(0 == pass->color_atts[i].image);
        SOKOL_ASSERT(pass->d3d11_rtvs[i] == 0);
        att_desc = &desc->color_attachments[i];
        if (att_desc->image.id != SG_INVALID_ID) {
            pass->num_color_atts++;
            SOKOL_ASSERT(att_images[i] && (att_images[i]->slot.id == att_desc->image.id));
            SOKOL_ASSERT(_sg_is_valid_rendertarget_color_format(att_images[i]->pixel_format));
            att = &pass->color_atts[i];
            SOKOL_ASSERT((att->image == 0) && (att->image_id.id == SG_INVALID_ID));
            att->image = att_images[i];
            att->image_id = att_desc->image;
            att->mip_level = att_desc->mip_level;
            att->slice = att_desc->slice;

            /* create D3D11 render-target-view */
            ID3D11Resource* d3d11_res = 0;
            const bool is_msaa = att->image->sample_count > 1;
            D3D11_RENDER_TARGET_VIEW_DESC d3d11_rtv_desc;
            memset(&d3d11_rtv_desc, 0, sizeof(d3d11_rtv_desc));
            d3d11_rtv_desc.Format = att->image->d3d11_format;
            switch (att->image->type) {
                case SG_IMAGETYPE_2D:
                    if (is_msaa) {
                        d3d11_res = (ID3D11Resource*) att->image->d3d11_texmsaa;
                        d3d11_rtv_desc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2DMS;
                    }
                    else {
                        d3d11_res = (ID3D11Resource*) att->image->d3d11_tex2d;
                        d3d11_rtv_desc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;
                        d3d11_rtv_desc.Texture2D.MipSlice = att->mip_level;
                    }
                    break;
                case SG_IMAGETYPE_CUBE:
                case SG_IMAGETYPE_ARRAY:
                    if (is_msaa) {
                        d3d11_res = (ID3D11Resource*) att->image->d3d11_texmsaa;
                        d3d11_rtv_desc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2DMSARRAY;
                        d3d11_rtv_desc.Texture2DMSArray.FirstArraySlice = att->slice;
                        d3d11_rtv_desc.Texture2DMSArray.ArraySize = 1;
                    }
                    else {
                        d3d11_res = (ID3D11Resource*) att->image->d3d11_tex2d;
                        d3d11_rtv_desc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2DARRAY;
                        d3d11_rtv_desc.Texture2DArray.MipSlice = att->mip_level;
                        d3d11_rtv_desc.Texture2DArray.FirstArraySlice = att->slice;
                        d3d11_rtv_desc.Texture2DArray.ArraySize = 1;
                    }
                    break;
                case SG_IMAGETYPE_3D:
                    SOKOL_ASSERT(!is_msaa);
                    d3d11_res = (ID3D11Resource*) att->image->d3d11_tex3d;
                    d3d11_rtv_desc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE3D;
                    d3d11_rtv_desc.Texture3D.MipSlice = att->mip_level;
                    d3d11_rtv_desc.Texture3D.FirstWSlice = att->slice;
                    d3d11_rtv_desc.Texture3D.WSize = 1;
                    break;
                default:
                    SOKOL_UNREACHABLE; break;
            }
            SOKOL_ASSERT(d3d11_res);
            HRESULT hr = ID3D11Device_CreateRenderTargetView(_sg_d3d11.dev, d3d11_res, &d3d11_rtv_desc, &pass->d3d11_rtvs[i]);
            SOKOL_ASSERT(SUCCEEDED(hr) && pass->d3d11_rtvs[i]);
        }
    }

    /* optional depth-stencil image */
    SOKOL_ASSERT(0 == pass->ds_att.image);
    SOKOL_ASSERT(pass->d3d11_dsv == 0);
    att_desc = &desc->depth_stencil_attachment;
    const int ds_img_index = SG_MAX_COLOR_ATTACHMENTS;
    if (att_desc->image.id != SG_INVALID_ID) {
        SOKOL_ASSERT(att_images[ds_img_index] && (att_images[ds_img_index]->slot.id == att_desc->image.id));
        SOKOL_ASSERT(_sg_is_valid_rendertarget_depth_format(att_images[ds_img_index]->pixel_format));
        att = &pass->ds_att;
        SOKOL_ASSERT((att->image == 0) && (att->image_id.id == SG_INVALID_ID));
        att->image = att_images[ds_img_index];
        att->image_id = att_desc->image;
        att->mip_level = att_desc->mip_level;
        att->slice = att_desc->slice;

        /* create D3D11 depth-stencil-view */
        D3D11_DEPTH_STENCIL_VIEW_DESC d3d11_dsv_desc;
        memset(&d3d11_dsv_desc, 0, sizeof(d3d11_dsv_desc));
        d3d11_dsv_desc.Format = att->image->d3d11_format;
        const bool is_msaa = att->image->sample_count > 1;
        if (is_msaa) {
            d3d11_dsv_desc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2DMS;
        }
        else {
            d3d11_dsv_desc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
        }
        ID3D11Resource* d3d11_res = (ID3D11Resource*) att->image->d3d11_texds;
        SOKOL_ASSERT(d3d11_res);
        HRESULT hr = ID3D11Device_CreateDepthStencilView(_sg_d3d11.dev, d3d11_res, &d3d11_dsv_desc, &pass->d3d11_dsv);
        SOKOL_ASSERT(SUCCEEDED(hr) && pass->d3d11_dsv);
    }
    pass->slot.state = SG_RESOURCESTATE_VALID;
}

_SOKOL_PRIVATE void _sg_destroy_pass(_sg_pass* pass) {
    SOKOL_ASSERT(pass);
    for (int i = 0; i < SG_MAX_COLOR_ATTACHMENTS; i++) {
        if (pass->d3d11_rtvs[i]) {
            ID3D11RenderTargetView_Release(pass->d3d11_rtvs[i]);
        }
    }
    if (pass->d3d11_dsv) {
        ID3D11DepthStencilView_Release(pass->d3d11_dsv);
    }
    _sg_init_pass(pass);
}

_SOKOL_PRIVATE void _sg_begin_pass(_sg_pass* pass, const sg_pass_action* action, int w, int h) {
    SOKOL_ASSERT(action);
    SOKOL_ASSERT(!_sg_d3d11.in_pass);
    _sg_d3d11.in_pass = true;
    _sg_d3d11.cur_width = w;
    _sg_d3d11.cur_height = h;
    if (pass) {
        _sg_d3d11.cur_pass = pass;
        _sg_d3d11.cur_pass_id.id = pass->slot.id;
        _sg_d3d11.num_rtvs = 0;
        for (int i = 0; i < SG_MAX_COLOR_ATTACHMENTS; i++) {
            _sg_d3d11.cur_rtvs[i] = pass->d3d11_rtvs[i];
            if (_sg_d3d11.cur_rtvs[i]) {
                _sg_d3d11.num_rtvs++;
            }
        }
        _sg_d3d11.cur_dsv = pass->d3d11_dsv;
    }
    else {
        /* render to default frame buffer */
        _sg_d3d11.cur_pass = 0;
        _sg_d3d11.cur_pass_id.id = SG_INVALID_ID;
        _sg_d3d11.num_rtvs = 1;
        _sg_d3d11.cur_rtvs[0] = (ID3D11RenderTargetView*) _sg_d3d11.rtv_cb();
        for (int i = 1; i < SG_MAX_COLOR_ATTACHMENTS; i++) {
            _sg_d3d11.cur_rtvs[i] = 0;
        }
        _sg_d3d11.cur_dsv = (ID3D11DepthStencilView*) _sg_d3d11.dsv_cb();
        SOKOL_ASSERT(_sg_d3d11.cur_rtvs[0] && _sg_d3d11.cur_dsv);
    }
    /* apply the render-target- and depth-stencil-views */
    ID3D11DeviceContext_OMSetRenderTargets(_sg_d3d11.ctx, SG_MAX_COLOR_ATTACHMENTS, _sg_d3d11.cur_rtvs, _sg_d3d11.cur_dsv);

    /* set viewport and scissor rect to cover whole screen */
    D3D11_VIEWPORT vp;
    memset(&vp, 0, sizeof(vp));
    vp.Width = w;
    vp.Height = h;
    vp.MaxDepth = 1.0f;
    ID3D11DeviceContext_RSSetViewports(_sg_d3d11.ctx, 1, &vp);
    D3D11_RECT rect;
    rect.left = 0;
    rect.top = 0;
    rect.right = w;
    rect.bottom = h;
    ID3D11DeviceContext_RSSetScissorRects(_sg_d3d11.ctx, 1, &rect);

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

/* D3D11CalcSubresource only exists for C++ */
_SOKOL_PRIVATE UINT _sg_d3d11_calcsubresource(UINT mip_slice, UINT array_slice, UINT mip_levels) {
    return mip_slice + array_slice * mip_levels;
}

_SOKOL_PRIVATE void _sg_end_pass() {
    SOKOL_ASSERT(_sg_d3d11.in_pass && _sg_d3d11.ctx);
    _sg_d3d11.in_pass = false;

    /* need to resolve MSAA render target into texture? */
    if (_sg_d3d11.cur_pass) {
        SOKOL_ASSERT(_sg_d3d11.cur_pass->slot.id == _sg_d3d11.cur_pass_id.id);
        for (int i = 0; i < _sg_d3d11.num_rtvs; i++) {
            _sg_attachment* att = &_sg_d3d11.cur_pass->color_atts[i];
            SOKOL_ASSERT(att->image && (att->image->slot.id == att->image_id.id));
            if (att->image->sample_count > 1) {
                SOKOL_ASSERT(att->image->d3d11_tex2d && att->image->d3d11_texmsaa && !att->image->d3d11_tex3d);
                SOKOL_ASSERT(DXGI_FORMAT_UNKNOWN != att->image->d3d11_format);
                const _sg_image* img = att->image;
                UINT subres = _sg_d3d11_calcsubresource(att->mip_level, att->slice, img->num_mipmaps);
                ID3D11DeviceContext_ResolveSubresource(_sg_d3d11.ctx,
                    (ID3D11Resource*) img->d3d11_tex2d,     /* pDstResource */
                    subres,                                 /* DstSubresource */
                    (ID3D11Resource*) img->d3d11_texmsaa,   /* pSrcResource */
                    subres,                                 /* SrcSubresource */
                    img->d3d11_format);
            }
        }
    }
    _sg_d3d11.cur_pass = 0;
    _sg_d3d11.cur_pass_id.id = SG_INVALID_ID;
    _sg_d3d11.cur_pipeline = 0;
    _sg_d3d11.cur_pipeline_id.id = SG_INVALID_ID;
    for (int i = 0; i < SG_MAX_COLOR_ATTACHMENTS; i++) {
        _sg_d3d11.cur_rtvs[i] = 0;
    }
    _sg_d3d11.cur_dsv = 0;
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

    _sg_d3d11.cur_pipeline = pip;
    _sg_d3d11.cur_pipeline_id.id = pip->slot.id;
    _sg_d3d11.use_indexed_draw = (pip->d3d11_index_format != DXGI_FORMAT_UNKNOWN);

    /* gather all the D3D11 resources into arrays */
    ID3D11Buffer* d3d11_ib = ib ? ib->d3d11_buf : 0;
    ID3D11Buffer* d3d11_vbs[SG_MAX_SHADERSTAGE_BUFFERS];
    UINT d3d11_vb_offsets[SG_MAX_SHADERSTAGE_BUFFERS];
    ID3D11ShaderResourceView* d3d11_vs_srvs[SG_MAX_SHADERSTAGE_IMAGES];
    ID3D11SamplerState* d3d11_vs_smps[SG_MAX_SHADERSTAGE_IMAGES];
    ID3D11ShaderResourceView* d3d11_fs_srvs[SG_MAX_SHADERSTAGE_IMAGES];
    ID3D11SamplerState* d3d11_fs_smps[SG_MAX_SHADERSTAGE_IMAGES];
    int i;
    for (i = 0; i < num_vbs; i++) {
        SOKOL_ASSERT(vbs[i]->d3d11_buf);
        d3d11_vbs[i] = vbs[i]->d3d11_buf;
        d3d11_vb_offsets[i] = 0;
    }
    for (; i < SG_MAX_SHADERSTAGE_BUFFERS; i++) {
        d3d11_vbs[i] = 0; 
        d3d11_vb_offsets[i] = 0;
    }
    for (i = 0; i < num_vs_imgs; i++) {
        SOKOL_ASSERT(vs_imgs[i]->d3d11_srv);
        SOKOL_ASSERT(vs_imgs[i]->d3d11_smp);
        d3d11_vs_srvs[i] = vs_imgs[i]->d3d11_srv;
        d3d11_vs_smps[i] = vs_imgs[i]->d3d11_smp;
    }
    for (; i < SG_MAX_SHADERSTAGE_IMAGES; i++) {
        d3d11_vs_srvs[i] = 0;
        d3d11_vs_smps[i] = 0;
    }
    for (i = 0; i < num_fs_imgs; i++) {
        SOKOL_ASSERT(fs_imgs[i]->d3d11_srv);
        SOKOL_ASSERT(fs_imgs[i]->d3d11_smp);
        d3d11_fs_srvs[i] = fs_imgs[i]->d3d11_srv;
        d3d11_fs_smps[i] = fs_imgs[i]->d3d11_smp;
    }
    for (; i < SG_MAX_SHADERSTAGE_IMAGES; i++) {
        d3d11_fs_srvs[i] = 0;
        d3d11_fs_smps[i] = 0;
    }

    /* FIXME: is it worth it to implement a state cache here? measure! */
    ID3D11DeviceContext_RSSetState(_sg_d3d11.ctx, pip->d3d11_rs);
    ID3D11DeviceContext_OMSetDepthStencilState(_sg_d3d11.ctx, pip->d3d11_dss, pip->d3d11_stencil_ref);
    ID3D11DeviceContext_OMSetBlendState(_sg_d3d11.ctx, pip->d3d11_bs, pip->blend_color, 0xFFFFFFFF);

    ID3D11DeviceContext_IASetVertexBuffers(_sg_d3d11.ctx, 0, SG_MAX_SHADERSTAGE_BUFFERS, d3d11_vbs, pip->d3d11_vb_strides, d3d11_vb_offsets);
    ID3D11DeviceContext_IASetPrimitiveTopology(_sg_d3d11.ctx, pip->d3d11_topology);
    ID3D11DeviceContext_IASetIndexBuffer(_sg_d3d11.ctx, d3d11_ib, pip->d3d11_index_format, 0); 
    ID3D11DeviceContext_IASetInputLayout(_sg_d3d11.ctx, pip->d3d11_il);
    
    ID3D11DeviceContext_VSSetShader(_sg_d3d11.ctx, pip->shader->d3d11_vs, NULL, 0);
    ID3D11DeviceContext_VSSetConstantBuffers(_sg_d3d11.ctx, 0, SG_MAX_SHADERSTAGE_UBS, pip->shader->stage[SG_SHADERSTAGE_VS].d3d11_cbs);
    ID3D11DeviceContext_VSSetShaderResources(_sg_d3d11.ctx, 0, SG_MAX_SHADERSTAGE_IMAGES, d3d11_vs_srvs);
    ID3D11DeviceContext_VSSetSamplers(_sg_d3d11.ctx, 0, SG_MAX_SHADERSTAGE_IMAGES, d3d11_vs_smps);
    
    ID3D11DeviceContext_PSSetShader(_sg_d3d11.ctx, pip->shader->d3d11_fs, NULL, 0);
    ID3D11DeviceContext_PSSetConstantBuffers(_sg_d3d11.ctx, 0, SG_MAX_SHADERSTAGE_UBS, pip->shader->stage[SG_SHADERSTAGE_FS].d3d11_cbs);
    ID3D11DeviceContext_PSSetShaderResources(_sg_d3d11.ctx, 0, SG_MAX_SHADERSTAGE_IMAGES, d3d11_fs_srvs);
    ID3D11DeviceContext_PSSetSamplers(_sg_d3d11.ctx, 0, SG_MAX_SHADERSTAGE_IMAGES, d3d11_fs_smps);
}

_SOKOL_PRIVATE void _sg_apply_uniform_block(sg_shader_stage stage_index, int ub_index, const void* data, int num_bytes) {
    SOKOL_ASSERT(_sg_d3d11.ctx && _sg_d3d11.in_pass);
    SOKOL_ASSERT(data && (num_bytes > 0));
    SOKOL_ASSERT((stage_index >= 0) && (stage_index < SG_NUM_SHADER_STAGES));
    SOKOL_ASSERT((ub_index >= 0) && (ub_index < SG_MAX_SHADERSTAGE_UBS));
    SOKOL_ASSERT(_sg_d3d11.cur_pipeline && _sg_d3d11.cur_pipeline->slot.id == _sg_d3d11.cur_pipeline_id.id);
    SOKOL_ASSERT(_sg_d3d11.cur_pipeline->shader && _sg_d3d11.cur_pipeline->shader->slot.id == _sg_d3d11.cur_pipeline->shader_id.id);
    SOKOL_ASSERT(ub_index < _sg_d3d11.cur_pipeline->shader->stage[stage_index].num_uniform_blocks);
    SOKOL_ASSERT(num_bytes == _sg_d3d11.cur_pipeline->shader->stage[stage_index].uniform_blocks[ub_index].size);
    ID3D11Buffer* cb = _sg_d3d11.cur_pipeline->shader->stage[stage_index].d3d11_cbs[ub_index];
    SOKOL_ASSERT(cb);
    ID3D11DeviceContext_UpdateSubresource(_sg_d3d11.ctx, (ID3D11Resource*)cb, 0, NULL, data, 0, 0);
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
}

_SOKOL_PRIVATE void _sg_update_buffer(_sg_buffer* buf, const void* data_ptr, int data_size) {
    SOKOL_ASSERT(buf && data_ptr && data_size);
    SOKOL_ASSERT(_sg_d3d11.ctx);
    SOKOL_ASSERT(buf->d3d11_buf);
    D3D11_MAPPED_SUBRESOURCE d3d11_msr;
    HRESULT hr = ID3D11DeviceContext_Map(_sg_d3d11.ctx, (ID3D11Resource*)buf->d3d11_buf, 0, D3D11_MAP_WRITE_DISCARD, 0, &d3d11_msr);
    SOKOL_ASSERT(SUCCEEDED(hr));
    memcpy(d3d11_msr.pData, data_ptr, data_size);
    ID3D11DeviceContext_Unmap(_sg_d3d11.ctx, (ID3D11Resource*)buf->d3d11_buf, 0);
}

_SOKOL_PRIVATE void _sg_update_image(_sg_image* img, const sg_image_content* data) {
    SOKOL_ASSERT(img && data);
    SOKOL_ASSERT(_sg_d3d11.ctx);
    SOKOL_ASSERT(img->d3d11_tex2d || img->d3d11_tex3d);
    ID3D11Resource* d3d11_res = 0;
    if (img->d3d11_tex3d) {
        d3d11_res = (ID3D11Resource*) img->d3d11_tex3d;
    }
    else {
        d3d11_res = (ID3D11Resource*) img->d3d11_tex2d;
    }
    SOKOL_ASSERT(d3d11_res);
    const int num_faces = (img->type == SG_IMAGETYPE_CUBE) ? 6:1;
    const int num_slices = (img->type == SG_IMAGETYPE_ARRAY) ? img->depth:1;
    int subres_index = 0;
    HRESULT hr;
    D3D11_MAPPED_SUBRESOURCE d3d11_msr;
    for (int face_index = 0; face_index < num_faces; face_index++) {
        for (int slice_index = 0; slice_index < num_slices; slice_index++) {
            for (int mip_index = 0; mip_index < img->num_mipmaps; mip_index++, subres_index++) {
                SOKOL_ASSERT(subres_index < (SG_MAX_MIPMAPS * SG_MAX_TEXTUREARRAY_LAYERS));
                const int mip_width = ((img->width>>mip_index)>0) ? img->width>>mip_index : 1;
                const int mip_height = ((img->height>>mip_index)>0) ? img->height>>mip_index : 1;
                const sg_subimage_content* subimg_content = &(data->subimage[face_index][mip_index]);
                const int slice_size = subimg_content->size / num_slices;
                const int slice_offset = slice_size * slice_index;
                const uint8_t* slice_ptr = ((const uint8_t*)subimg_content->ptr) + slice_offset;
                hr = ID3D11DeviceContext_Map(_sg_d3d11.ctx, d3d11_res, subres_index, D3D11_MAP_WRITE_DISCARD, 0, &d3d11_msr);
                SOKOL_ASSERT(SUCCEEDED(hr));
                memcpy(d3d11_msr.pData, slice_ptr, slice_size);
                ID3D11DeviceContext_Unmap(_sg_d3d11.ctx, d3d11_res, subres_index);
            }
        }
    }
}

_SOKOL_PRIVATE void _sg_reset_state_cache() {
    /* just clear the d3d11 device context state */
    _sg_d3d11_clear_state();
}

#ifdef __cplusplus
} // extern "C"
#endif
