/*
    Sokol Gfx GL rendering backend
*/
#ifndef SOKOL_IMPL_GUARD
#error "Please do not include *.impl.h files directly"
#endif

/* strstr(), memset() */
#include <string.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifndef GL_UNSIGNED_INT_2_10_10_10_REV
#define GL_UNSIGNED_INT_2_10_10_10_REV 0x8368
#endif
#ifndef GL_UNSIGNED_INT_24_8
#define GL_UNSIGNED_INT_24_8 0x84FA
#endif
#ifndef GL_TEXTURE_MAX_ANISOTROPY_EXT
#define GL_TEXTURE_MAX_ANISOTROPY_EXT 0x84FE
#endif
#ifndef GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT
#define GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT 0x84FF
#endif
#ifndef GL_COMPRESSED_RGBA_S3TC_DXT1_EXT
#define GL_COMPRESSED_RGBA_S3TC_DXT1_EXT 0x83F1
#endif
#ifndef GL_COMPRESSED_RGBA_S3TC_DXT3_EXT
#define GL_COMPRESSED_RGBA_S3TC_DXT3_EXT 0x83F2
#endif
#ifndef GL_COMPRESSED_RGBA_S3TC_DXT5_EXT
#define GL_COMPRESSED_RGBA_S3TC_DXT5_EXT 0x83F3
#endif
#ifndef GL_COMPRESSED_RGB_PVRTC_2BPPV1_IMG
#define GL_COMPRESSED_RGB_PVRTC_2BPPV1_IMG 0x8C01
#endif
#ifndef GL_COMPRESSED_RGB_PVRTC_4BPPV1_IMG
#define GL_COMPRESSED_RGB_PVRTC_4BPPV1_IMG 0x8C00
#endif
#ifndef GL_COMPRESSED_RGBA_PVRTC_2BPPV1_IMG
#define GL_COMPRESSED_RGBA_PVRTC_2BPPV1_IMG 0x8C03
#endif
#ifndef GL_COMPRESSED_RGBA_PVRTC_4BPPV1_IMG
#define GL_COMPRESSED_RGBA_PVRTC_4BPPV1_IMG 0x8C02
#endif
#ifndef GL_COMPRESSED_RGB8_ETC2
#define GL_COMPRESSED_RGB8_ETC2 0x9274
#endif
#ifndef GL_COMPRESSED_SRGB8_ETC2
#define GL_COMPRESSED_SRGB8_ETC2 0x9275
#endif
#ifndef GL_DEPTH24_STENCIL8
#define GL_DEPTH24_STENCIL8 0x88F0
#endif
#ifndef GL_HALF_FLOAT
#define GL_HALF_FLOAT 0x140B
#endif
#ifndef GL_DEPTH_STENCIL
#define GL_DEPTH_STENCIL 0x84F9
#endif
#ifdef SOKOL_GLES2
#define glVertexAttribDivisor(index, divisor) glVertexAttribDivisorEXT(index, divisor)
#define glDrawArraysInstanced(mode, first, count, instancecount) glDrawArraysInstancedEXT(mode, first, count, instancecount)
#define glDrawElementsInstanced(mode, count, type, indices, instancecount) glDrawElementsInstancedEXT(mode, count, type, indices, instancecount)
#endif
#define _SG_GL_CHECK_ERROR() { SOKOL_ASSERT(glGetError() == GL_NO_ERROR); } 

/* true if runnin in GLES2-fallback mode */
static bool _sg_gl_gles2;

/*-- type translation --------------------------------------------------------*/
_SOKOL_PRIVATE GLenum _sg_gl_buffer_target(sg_buffer_type t) {
    switch (t) {
        case SG_BUFFERTYPE_VERTEXBUFFER:    return GL_ARRAY_BUFFER;
        case SG_BUFFERTYPE_INDEXBUFFER:     return GL_ELEMENT_ARRAY_BUFFER;
        default: SOKOL_UNREACHABLE; return 0;
    }
}

_SOKOL_PRIVATE GLenum _sg_gl_texture_target(sg_image_type t) {
    switch (t) {
        case SG_IMAGETYPE_2D:   return GL_TEXTURE_2D;
        case SG_IMAGETYPE_CUBE: return GL_TEXTURE_CUBE_MAP;
        #if !defined(SOKOL_GLES2)
        case SG_IMAGETYPE_3D:       return GL_TEXTURE_3D;
        case SG_IMAGETYPE_ARRAY:    return GL_TEXTURE_2D_ARRAY;
        #endif
        default: SOKOL_UNREACHABLE; return 0;
    }
}

_SOKOL_PRIVATE GLenum _sg_gl_usage(sg_usage u) {
    switch (u) {
        case SG_USAGE_IMMUTABLE:    return GL_STATIC_DRAW;
        case SG_USAGE_DYNAMIC:      return GL_DYNAMIC_DRAW;
        case SG_USAGE_STREAM:       return GL_STREAM_DRAW;
        default: SOKOL_UNREACHABLE; return 0;
    }
}

_SOKOL_PRIVATE GLenum _sg_gl_shader_stage(sg_shader_stage stage) {
    switch (stage) {
        case SG_SHADERSTAGE_VS:     return GL_VERTEX_SHADER;
        case SG_SHADERSTAGE_FS:     return GL_FRAGMENT_SHADER;
        default: SOKOL_UNREACHABLE; return 0;
    }
}

_SOKOL_PRIVATE GLint _sg_gl_vertexformat_size(sg_vertex_format fmt) {
    switch (fmt) {
        case SG_VERTEXFORMAT_FLOAT:     return 1;
        case SG_VERTEXFORMAT_FLOAT2:    return 2;
        case SG_VERTEXFORMAT_FLOAT3:    return 3;
        case SG_VERTEXFORMAT_FLOAT4:    return 4;
        case SG_VERTEXFORMAT_BYTE4:     return 4;
        case SG_VERTEXFORMAT_BYTE4N:    return 4;
        case SG_VERTEXFORMAT_UBYTE4:    return 4;
        case SG_VERTEXFORMAT_UBYTE4N:   return 4;
        case SG_VERTEXFORMAT_SHORT2:    return 2;
        case SG_VERTEXFORMAT_SHORT2N:   return 2;
        case SG_VERTEXFORMAT_SHORT4:    return 4;
        case SG_VERTEXFORMAT_SHORT4N:   return 4;
        case SG_VERTEXFORMAT_UINT10_N2: return 4;
        default: SOKOL_UNREACHABLE; return 0;
    }
}

_SOKOL_PRIVATE GLenum _sg_gl_vertexformat_type(sg_vertex_format fmt) {
    switch (fmt) {
        case SG_VERTEXFORMAT_FLOAT:
        case SG_VERTEXFORMAT_FLOAT2:
        case SG_VERTEXFORMAT_FLOAT3:
        case SG_VERTEXFORMAT_FLOAT4:
            return GL_FLOAT;
        case SG_VERTEXFORMAT_BYTE4:
        case SG_VERTEXFORMAT_BYTE4N:
            return GL_BYTE;
        case SG_VERTEXFORMAT_UBYTE4:
        case SG_VERTEXFORMAT_UBYTE4N:
            return GL_UNSIGNED_BYTE;
        case SG_VERTEXFORMAT_SHORT2:
        case SG_VERTEXFORMAT_SHORT2N:
        case SG_VERTEXFORMAT_SHORT4:
        case SG_VERTEXFORMAT_SHORT4N:
            return GL_SHORT;
        case SG_VERTEXFORMAT_UINT10_N2:
            return GL_UNSIGNED_INT_2_10_10_10_REV;
        default:
            SOKOL_UNREACHABLE; return 0;
    }
}

_SOKOL_PRIVATE GLboolean _sg_gl_vertexformat_normalized(sg_vertex_format fmt) {
    switch (fmt) {
        case SG_VERTEXFORMAT_BYTE4N:
        case SG_VERTEXFORMAT_UBYTE4N:
        case SG_VERTEXFORMAT_SHORT2N:
        case SG_VERTEXFORMAT_SHORT4N:
        case SG_VERTEXFORMAT_UINT10_N2:
            return GL_TRUE;
        default:
            return GL_FALSE;
    }
}

_SOKOL_PRIVATE GLenum _sg_gl_primitive_type(sg_primitive_type t) {
    switch (t) {
        case SG_PRIMITIVETYPE_POINTS:           return GL_POINTS;
        case SG_PRIMITIVETYPE_LINES:            return GL_LINES;
        case SG_PRIMITIVETYPE_LINE_STRIP:       return GL_LINE_STRIP;
        case SG_PRIMITIVETYPE_TRIANGLES:        return GL_TRIANGLES;
        case SG_PRIMITIVETYPE_TRIANGLE_STRIP:   return GL_TRIANGLE_STRIP;
        default: SOKOL_UNREACHABLE; return 0;
    }
}

_SOKOL_PRIVATE GLenum _sg_gl_index_type(sg_index_type t) {
    switch (t) {
        case SG_INDEXTYPE_NONE:     return 0;
        case SG_INDEXTYPE_UINT16:   return GL_UNSIGNED_SHORT;
        case SG_INDEXTYPE_UINT32:   return GL_UNSIGNED_INT;
        default: SOKOL_UNREACHABLE; return 0;
    }
}

_SOKOL_PRIVATE GLenum _sg_gl_compare_func(sg_compare_func cmp) {
    switch (cmp) {
        case SG_COMPAREFUNC_NEVER:          return GL_NEVER;
        case SG_COMPAREFUNC_LESS:           return GL_LESS;
        case SG_COMPAREFUNC_EQUAL:          return GL_EQUAL;
        case SG_COMPAREFUNC_LESS_EQUAL:     return GL_LEQUAL;
        case SG_COMPAREFUNC_GREATER:        return GL_GREATER;
        case SG_COMPAREFUNC_NOT_EQUAL:      return GL_NOTEQUAL;
        case SG_COMPAREFUNC_GREATER_EQUAL:  return GL_GEQUAL;
        case SG_COMPAREFUNC_ALWAYS:         return GL_ALWAYS;
        default: SOKOL_UNREACHABLE; return 0;
    }
}

_SOKOL_PRIVATE GLenum _sg_gl_stencil_op(sg_stencil_op op) {
    switch (op) {
        case SG_STENCILOP_KEEP:         return GL_KEEP;
        case SG_STENCILOP_ZERO:         return GL_ZERO;
        case SG_STENCILOP_REPLACE:      return GL_REPLACE;
        case SG_STENCILOP_INCR_CLAMP:   return GL_INCR;
        case SG_STENCILOP_DECR_CLAMP:   return GL_DECR;
        case SG_STENCILOP_INVERT:       return GL_INVERT;
        case SG_STENCILOP_INCR_WRAP:    return GL_INCR_WRAP;
        case SG_STENCILOP_DECR_WRAP:    return GL_DECR_WRAP;
        default: SOKOL_UNREACHABLE; return 0;
    }
}

_SOKOL_PRIVATE GLenum _sg_gl_blend_factor(sg_blend_factor f) {
    switch (f) {
        case SG_BLENDFACTOR_ZERO:                   return GL_ZERO;
        case SG_BLENDFACTOR_ONE:                    return GL_ONE;
        case SG_BLENDFACTOR_SRC_COLOR:              return GL_SRC_COLOR;
        case SG_BLENDFACTOR_ONE_MINUS_SRC_COLOR:    return GL_ONE_MINUS_SRC_COLOR;
        case SG_BLENDFACTOR_SRC_ALPHA:              return GL_SRC_ALPHA;
        case SG_BLENDFACTOR_ONE_MINUS_SRC_ALPHA:    return GL_ONE_MINUS_SRC_ALPHA;
        case SG_BLENDFACTOR_DST_COLOR:              return GL_DST_COLOR;
        case SG_BLENDFACTOR_ONE_MINUS_DST_COLOR:    return GL_ONE_MINUS_DST_COLOR;
        case SG_BLENDFACTOR_DST_ALPHA:              return GL_DST_ALPHA;
        case SG_BLENDFACTOR_ONE_MINUS_DST_ALPHA:    return GL_ONE_MINUS_DST_ALPHA;
        case SG_BLENDFACTOR_SRC_ALPHA_SATURATED:    return GL_SRC_ALPHA_SATURATE;
        case SG_BLENDFACTOR_BLEND_COLOR:            return GL_CONSTANT_COLOR;
        case SG_BLENDFACTOR_ONE_MINUS_BLEND_COLOR:  return GL_ONE_MINUS_CONSTANT_COLOR;
        case SG_BLENDFACTOR_BLEND_ALPHA:            return GL_CONSTANT_ALPHA;
        case SG_BLENDFACTOR_ONE_MINUS_BLEND_ALPHA:  return GL_ONE_MINUS_CONSTANT_ALPHA;
        default: SOKOL_UNREACHABLE; return 0;
    }
}

_SOKOL_PRIVATE GLenum _sg_gl_blend_op(sg_blend_op op) {
    switch (op) {
        case SG_BLENDOP_ADD:                return GL_FUNC_ADD;
        case SG_BLENDOP_SUBTRACT:           return GL_FUNC_SUBTRACT;
        case SG_BLENDOP_REVERSE_SUBTRACT:   return GL_FUNC_REVERSE_SUBTRACT;
        default: SOKOL_UNREACHABLE; return 0;
    }
}

_SOKOL_PRIVATE GLenum _sg_gl_filter(sg_filter f) {
    switch (f) {
        case SG_FILTER_NEAREST:                 return GL_NEAREST;
        case SG_FILTER_LINEAR:                  return GL_LINEAR;
        case SG_FILTER_NEAREST_MIPMAP_NEAREST:  return GL_NEAREST_MIPMAP_NEAREST;
        case SG_FILTER_NEAREST_MIPMAP_LINEAR:   return GL_NEAREST_MIPMAP_LINEAR;
        case SG_FILTER_LINEAR_MIPMAP_NEAREST:   return GL_LINEAR_MIPMAP_NEAREST;
        case SG_FILTER_LINEAR_MIPMAP_LINEAR:    return GL_LINEAR_MIPMAP_LINEAR;
        default: SOKOL_UNREACHABLE; return 0;
    }
}

_SOKOL_PRIVATE GLenum _sg_gl_wrap(sg_wrap w) {
    switch (w) {
        case SG_WRAP_CLAMP_TO_EDGE:     return GL_CLAMP_TO_EDGE;
        case SG_WRAP_REPEAT:            return GL_REPEAT;
        case SG_WRAP_MIRRORED_REPEAT:   return GL_MIRRORED_REPEAT;
        default: SOKOL_UNREACHABLE; return 0;
    }
}

_SOKOL_PRIVATE GLenum _sg_gl_teximage_type(sg_pixel_format fmt) {
    switch (fmt) {
        case SG_PIXELFORMAT_RGBA32F:
        case SG_PIXELFORMAT_R32F:
            return GL_FLOAT;
        case SG_PIXELFORMAT_RGBA16F:
        case SG_PIXELFORMAT_R16F:
            return GL_HALF_FLOAT;
        case SG_PIXELFORMAT_RGBA8:
        case SG_PIXELFORMAT_RGB8:
        case SG_PIXELFORMAT_L8:
            return GL_UNSIGNED_BYTE;
        case SG_PIXELFORMAT_R10G10B10A2:
            return GL_UNSIGNED_INT_2_10_10_10_REV;
        case SG_PIXELFORMAT_R5G5B5A1:
            return GL_UNSIGNED_SHORT_5_5_5_1;
        case SG_PIXELFORMAT_R5G6B5:
            return GL_UNSIGNED_SHORT_5_6_5;
        case SG_PIXELFORMAT_RGBA4:
            return GL_UNSIGNED_SHORT_4_4_4_4;
        case SG_PIXELFORMAT_DEPTH:
            /* FIXME */
            return GL_UNSIGNED_SHORT;
        case SG_PIXELFORMAT_DEPTHSTENCIL:
            /* FIXME */
            return GL_UNSIGNED_INT_24_8;
        default:
            SOKOL_UNREACHABLE; return 0;
    }
}

_SOKOL_PRIVATE GLenum _sg_gl_teximage_format(sg_pixel_format fmt) {
    switch (fmt) {
        case SG_PIXELFORMAT_NONE:
            return 0;
        case SG_PIXELFORMAT_RGBA8:
        case SG_PIXELFORMAT_R5G5B5A1:
        case SG_PIXELFORMAT_RGBA4:
        case SG_PIXELFORMAT_RGBA32F:
        case SG_PIXELFORMAT_RGBA16F:
        case SG_PIXELFORMAT_R10G10B10A2:
            return GL_RGBA;
        case SG_PIXELFORMAT_RGB8:
        case SG_PIXELFORMAT_R5G6B5:
            return GL_RGB;
        case SG_PIXELFORMAT_L8:
        case SG_PIXELFORMAT_R32F:
        case SG_PIXELFORMAT_R16F:
            #if defined(SOKOL_GLES2)
            return GL_LUMINANCE;
            #else
            return GL_RED;
            #endif
        case SG_PIXELFORMAT_DEPTH:
            return GL_DEPTH_COMPONENT;
        case SG_PIXELFORMAT_DEPTHSTENCIL:
            return GL_DEPTH_STENCIL;
        case SG_PIXELFORMAT_DXT1:
            return GL_COMPRESSED_RGBA_S3TC_DXT1_EXT;
        case SG_PIXELFORMAT_DXT3:
            return GL_COMPRESSED_RGBA_S3TC_DXT3_EXT;
        case SG_PIXELFORMAT_DXT5:
            return GL_COMPRESSED_RGBA_S3TC_DXT5_EXT;
        case SG_PIXELFORMAT_PVRTC2_RGB:
            return GL_COMPRESSED_RGB_PVRTC_2BPPV1_IMG;
        case SG_PIXELFORMAT_PVRTC4_RGB:
            return GL_COMPRESSED_RGB_PVRTC_4BPPV1_IMG; 
        case SG_PIXELFORMAT_PVRTC2_RGBA:
            return GL_COMPRESSED_RGBA_PVRTC_2BPPV1_IMG;
        case SG_PIXELFORMAT_PVRTC4_RGBA:
            return GL_COMPRESSED_RGBA_PVRTC_4BPPV1_IMG;
        case SG_PIXELFORMAT_ETC2_RGB8:
            return GL_COMPRESSED_RGB8_ETC2;
        case SG_PIXELFORMAT_ETC2_SRGB8:
            return GL_COMPRESSED_SRGB8_ETC2;
        default:
            SOKOL_UNREACHABLE; return 0;
    }
}

_SOKOL_PRIVATE GLenum _sg_gl_teximage_internal_format(sg_pixel_format fmt) {
    #if defined(SOKOL_GLES2)
    return _sg_gl_teximage_format(fmt);
    #else
    if (_sg_gl_gles2) {
        return _sg_gl_teximage_format(fmt);
    }
    else {
        switch (fmt) {
            case SG_PIXELFORMAT_NONE:
                return 0;
            case SG_PIXELFORMAT_RGBA8:
                return GL_RGBA8;
            case SG_PIXELFORMAT_RGB8:
                return GL_RGB8;
            case SG_PIXELFORMAT_RGBA4:
                return GL_RGBA4;
            case SG_PIXELFORMAT_R5G6B5:
                #if defined(SOKOL_GLES3)
                    return GL_RGB565;
                #else
                    return GL_RGB5;
                #endif
            case SG_PIXELFORMAT_R5G5B5A1:
                return GL_RGB5_A1;
            case SG_PIXELFORMAT_R10G10B10A2:
                return GL_RGB10_A2;
            case SG_PIXELFORMAT_RGBA32F:
                return GL_RGBA32F;
            case SG_PIXELFORMAT_RGBA16F:
                return GL_RGBA16F;
            case SG_PIXELFORMAT_R32F:
                return GL_R32F;
            case SG_PIXELFORMAT_R16F:
                return GL_R16F;
            case SG_PIXELFORMAT_L8:
                return GL_R8;
            case SG_PIXELFORMAT_DEPTH:
                /* FIXME */
                return GL_DEPTH_COMPONENT16;
            case SG_PIXELFORMAT_DEPTHSTENCIL:
                return GL_DEPTH24_STENCIL8;
            case SG_PIXELFORMAT_DXT1:
                return GL_COMPRESSED_RGBA_S3TC_DXT1_EXT;
            case SG_PIXELFORMAT_DXT3:
                return GL_COMPRESSED_RGBA_S3TC_DXT3_EXT;
            case SG_PIXELFORMAT_DXT5:
                return GL_COMPRESSED_RGBA_S3TC_DXT5_EXT;
            case SG_PIXELFORMAT_PVRTC2_RGB:
                return GL_COMPRESSED_RGB_PVRTC_2BPPV1_IMG;
            case SG_PIXELFORMAT_PVRTC4_RGB:
                return GL_COMPRESSED_RGB_PVRTC_4BPPV1_IMG; 
            case SG_PIXELFORMAT_PVRTC2_RGBA:
                return GL_COMPRESSED_RGBA_PVRTC_2BPPV1_IMG;
            case SG_PIXELFORMAT_PVRTC4_RGBA:
                return GL_COMPRESSED_RGBA_PVRTC_4BPPV1_IMG;
            case SG_PIXELFORMAT_ETC2_RGB8:
                return GL_COMPRESSED_RGB8_ETC2;
            case SG_PIXELFORMAT_ETC2_SRGB8:
                return GL_COMPRESSED_SRGB8_ETC2;
            default:
                SOKOL_UNREACHABLE; return 0;
        }
    }
    #endif
}

_SOKOL_PRIVATE GLenum _sg_gl_cubeface_target(int face_index) {
    switch (face_index) {
        case 0: return GL_TEXTURE_CUBE_MAP_POSITIVE_X;
        case 1: return GL_TEXTURE_CUBE_MAP_NEGATIVE_X;
        case 2: return GL_TEXTURE_CUBE_MAP_POSITIVE_Y;
        case 3: return GL_TEXTURE_CUBE_MAP_NEGATIVE_Y;
        case 4: return GL_TEXTURE_CUBE_MAP_POSITIVE_Z;
        case 5: return GL_TEXTURE_CUBE_MAP_NEGATIVE_Z;
        default: SOKOL_UNREACHABLE; return 0;
    }
}

_SOKOL_PRIVATE GLenum _sg_gl_depth_attachment_format(sg_pixel_format fmt) {
    switch (fmt) {
        case SG_PIXELFORMAT_DEPTH:          return GL_DEPTH_COMPONENT16;
        case SG_PIXELFORMAT_DEPTHSTENCIL:   return GL_DEPTH24_STENCIL8;
        default: SOKOL_UNREACHABLE; return 0;
    }
}

/*-- GL backend resource declarations ----------------------------------------*/
typedef struct {
    _sg_slot slot;
    int size;
    sg_buffer_type type;
    sg_usage usage;
    uint32_t upd_frame_index;
    int num_slots;
    int active_slot;
    GLuint gl_buf[SG_NUM_INFLIGHT_FRAMES];
    bool ext_buffers;   /* if true, external buffers were injected with sg_buffer_desc.gl_buffers */
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
    GLenum gl_target;
    GLuint gl_depth_render_buffer;
    GLuint gl_msaa_render_buffer;
    uint32_t upd_frame_index;
    int num_slots;
    int active_slot;
    GLuint gl_tex[SG_NUM_INFLIGHT_FRAMES];
    bool ext_textures;  /* if true, external textures were injected with sg_image_desc.gl_textures */
} _sg_image;

_SOKOL_PRIVATE void _sg_init_image(_sg_image* img) {
    SOKOL_ASSERT(img);
    memset(img, 0, sizeof(_sg_image));
}

typedef struct {
    GLint gl_loc;
    sg_uniform_type type;
    uint8_t count;
    uint16_t offset;
} _sg_uniform;

typedef struct {
    int size;
    int num_uniforms;
    _sg_uniform uniforms[SG_MAX_UB_MEMBERS];
} _sg_uniform_block;

typedef struct {
    sg_image_type type;
    GLint gl_loc;
    int gl_tex_slot;
} _sg_shader_image;

typedef struct {
    int num_uniform_blocks;
    int num_images;
    _sg_uniform_block uniform_blocks[SG_MAX_SHADERSTAGE_UBS];
    _sg_shader_image images[SG_MAX_SHADERSTAGE_IMAGES];
} _sg_shader_stage;

typedef struct {
    _sg_slot slot;
    GLuint gl_prog;
    _sg_shader_stage stage[SG_NUM_SHADER_STAGES];
} _sg_shader;

_SOKOL_PRIVATE void _sg_init_shader(_sg_shader* shd) {
    SOKOL_ASSERT(shd);
    memset(shd, 0, sizeof(_sg_shader));
}

typedef struct {
    int8_t vb_index;        /* -1 if attr is not enabled */
    int8_t divisor;         /* -1 if not initialized */
    uint8_t stride;
    uint8_t size;
    uint8_t normalized;
    uint8_t offset;
    GLenum type;
} _sg_gl_attr;

_SOKOL_PRIVATE void _sg_gl_init_attr(_sg_gl_attr* attr) {
    attr->vb_index = -1;
    attr->divisor = -1;
    attr->stride = 0;
    attr->size = 0;
    attr->normalized = 0;
    attr->offset = 0;
    attr->type = 0;
}

typedef struct {
    _sg_slot slot;
    _sg_shader* shader;
    sg_shader shader_id;
    sg_primitive_type primitive_type;
    sg_index_type index_type;
    bool vertex_layout_valid[SG_MAX_SHADERSTAGE_BUFFERS];
    int color_attachment_count;
    sg_pixel_format color_format;
    sg_pixel_format depth_format;
    int sample_count;
    _sg_gl_attr gl_attrs[SG_MAX_VERTEX_ATTRIBUTES];
    sg_depth_stencil_state depth_stencil;
    sg_blend_state blend;
    sg_rasterizer_state rast;
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
    GLuint gl_msaa_resolve_buffer;
} _sg_attachment;

typedef struct {
    _sg_slot slot;
    GLuint gl_fb;
    int num_color_atts;
    _sg_attachment color_atts[SG_MAX_COLOR_ATTACHMENTS];
    _sg_attachment ds_att;
} _sg_pass;

_SOKOL_PRIVATE void _sg_init_pass(_sg_pass* pass) {
    SOKOL_ASSERT(pass);
    memset(pass, 0, sizeof(_sg_pass));
}

_SOKOL_PRIVATE void _sg_gl_init_stencil_state(sg_stencil_state* s) {
    SOKOL_ASSERT(s);
    s->fail_op = SG_STENCILOP_KEEP;
    s->depth_fail_op = SG_STENCILOP_KEEP;
    s->pass_op = SG_STENCILOP_KEEP;
    s->compare_func = SG_COMPAREFUNC_ALWAYS;
}

_SOKOL_PRIVATE void _sg_gl_init_depth_stencil_state(sg_depth_stencil_state* s) {
    SOKOL_ASSERT(s);
    _sg_gl_init_stencil_state(&s->stencil_front);
    _sg_gl_init_stencil_state(&s->stencil_back);
    s->depth_compare_func = SG_COMPAREFUNC_ALWAYS;
    s->depth_write_enabled = false;
    s->stencil_enabled = false;
    s->stencil_read_mask = 0;
    s->stencil_write_mask = 0;
    s->stencil_ref = 0;
}

_SOKOL_PRIVATE void _sg_gl_init_blend_state(sg_blend_state* s) {
    SOKOL_ASSERT(s);
    s->enabled = false;
    s->src_factor_rgb = SG_BLENDFACTOR_ONE;
    s->dst_factor_rgb = SG_BLENDFACTOR_ZERO;
    s->op_rgb = SG_BLENDOP_ADD;
    s->src_factor_alpha = SG_BLENDFACTOR_ONE;
    s->dst_factor_alpha = SG_BLENDFACTOR_ZERO;
    s->op_alpha = SG_BLENDOP_ADD;
    s->color_write_mask = SG_COLORMASK_RGBA;
    for (int i = 0; i < 4; i++) {
        s->blend_color[i] = 0.0f;
    }
}

_SOKOL_PRIVATE void _sg_gl_init_rasterizer_state(sg_rasterizer_state* s) {
    SOKOL_ASSERT(s);
    s->alpha_to_coverage_enabled = false;
    s->cull_mode = SG_CULLMODE_NONE;
    s->face_winding = SG_FACEWINDING_CW;
    s->sample_count = 1;
    s->depth_bias = 0.0f;
    s->depth_bias_slope_scale = 0.0f;
    s->depth_bias_clamp = 0.0f;
}

/*-- state cache implementation ----------------------------------------------*/
/*-- state cache and backend structs -----------------------------------------*/
typedef struct {
    _sg_gl_attr gl_attr;
    GLuint gl_vbuf;
} _sg_gl_cache_attr;

typedef struct {
    sg_depth_stencil_state ds;
    sg_blend_state blend;
    sg_rasterizer_state rast;
    bool polygon_offset_enabled;
    _sg_gl_cache_attr attrs[SG_MAX_VERTEX_ATTRIBUTES];
    GLuint cur_gl_ib;
    GLenum cur_primitive_type;
    GLenum cur_index_type;
    _sg_pipeline* cur_pipeline;
    sg_pipeline cur_pipeline_id; 
} _sg_state_cache;

_SOKOL_PRIVATE void _sg_gl_reset_state_cache(_sg_state_cache* cache) {
    SOKOL_ASSERT(cache);
    
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    for (int i = 0; i < SG_MAX_VERTEX_ATTRIBUTES; i++) {
        _sg_gl_init_attr(&cache->attrs[i].gl_attr);
        cache->attrs[i].gl_vbuf = 0;
        glDisableVertexAttribArray(i);
    }
    cache->cur_gl_ib = 0;
    cache->cur_primitive_type = GL_TRIANGLES;
    cache->cur_index_type = 0;

    /* resource bindings */
    cache->cur_pipeline = 0;
    cache->cur_pipeline_id.id = SG_INVALID_ID;

    /* depth-stencil state */
    _sg_gl_init_depth_stencil_state(&cache->ds);
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_ALWAYS);
    glDepthMask(GL_FALSE);
    glDisable(GL_STENCIL_TEST);
    glStencilFunc(GL_ALWAYS, 0, 0);
    glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);
    glStencilMask(0);

    /* blend state */
    _sg_gl_init_blend_state(&cache->blend);
    glDisable(GL_BLEND);
    glBlendFuncSeparate(GL_ONE, GL_ZERO, GL_ONE, GL_ZERO);
    glBlendEquationSeparate(GL_FUNC_ADD, GL_FUNC_ADD);
    glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
    glBlendColor(0.0f, 0.0f, 0.0f, 0.0f);

    /* rasterizer state */
    _sg_gl_init_rasterizer_state(&cache->rast);
    cache->polygon_offset_enabled = false;
    glPolygonOffset(0.0f, 0.0f);
    glDisable(GL_POLYGON_OFFSET_FILL);
    glDisable(GL_CULL_FACE);
    glFrontFace(GL_CW);
    glCullFace(GL_BACK);
    glEnable(GL_SCISSOR_TEST);
    glDisable(GL_SAMPLE_ALPHA_TO_COVERAGE);
    glEnable(GL_DITHER);
    glDisable(GL_POLYGON_OFFSET_FILL);
    #if defined(SOKOL_GLCORE33)
        glEnable(GL_MULTISAMPLE);
        glEnable(GL_PROGRAM_POINT_SIZE);
    #endif
}

/*-- main GL backend state and functions -------------------------------------*/
typedef struct {
    bool valid;
    bool in_pass;
    GLuint default_framebuffer;
    int cur_pass_width;
    int cur_pass_height;
    _sg_pass* cur_pass;
    sg_pass cur_pass_id;
    _sg_state_cache cache;
    bool features[SG_NUM_FEATURES];
    bool ext_anisotropic;
    GLint max_anisotropy;
    #if !defined(SOKOL_GLES2)
    GLuint vao; 
    #endif
} _sg_backend;

static _sg_backend _sg_gl;

_SOKOL_PRIVATE void _sg_setup_backend(const sg_desc* desc) {
    _sg_gl_gles2 = desc->gl_force_gles2;
    memset(&_sg_gl, 0, sizeof(_sg_gl));
    _sg_gl.valid = true;
    _sg_gl.in_pass = false;
    glGetIntegerv(GL_FRAMEBUFFER_BINDING, (GLint*)&_sg_gl.default_framebuffer);
    _sg_gl.cur_pass_width = 0;
    _sg_gl.cur_pass_height = 0;
    _sg_gl.cur_pass = 0;
    _sg_gl.cur_pass_id.id = SG_INVALID_ID;
    #if !defined(SOKOL_GLES2)
    if (!_sg_gl_gles2) {
        glGenVertexArrays(1, &_sg_gl.vao);
        glBindVertexArray(_sg_gl.vao);
    }
    #endif
    _sg_gl_reset_state_cache(&_sg_gl.cache);
    
    /* initialize feature flags */
    for (int i = 0; i < SG_NUM_FEATURES; i++) {
        _sg_gl.features[i] = false;
    }
    _sg_gl.ext_anisotropic = false;
    _sg_gl.features[SG_FEATURE_ORIGIN_BOTTOM_LEFT] = true;
    #if defined(SOKOL_GLCORE33)
        _sg_gl.features[SG_FEATURE_INSTANCING] = true;
        _sg_gl.features[SG_FEATURE_TEXTURE_FLOAT] = true;
        _sg_gl.features[SG_FEATURE_TEXTURE_HALF_FLOAT] = true;
        _sg_gl.features[SG_FEATURE_MSAA_RENDER_TARGETS] = true;
        _sg_gl.features[SG_FEATURE_PACKED_VERTEX_FORMAT_10_2] = true;
        _sg_gl.features[SG_FEATURE_MULTIPLE_RENDER_TARGET] = true;
        _sg_gl.features[SG_FEATURE_IMAGETYPE_3D] = true;
        _sg_gl.features[SG_FEATURE_IMAGETYPE_ARRAY] = true;
        GLint num_ext = 0;
        glGetIntegerv(GL_NUM_EXTENSIONS, &num_ext);
        for (int i = 0; i < num_ext; i++) {
            const char* ext = (const char*) glGetStringi(GL_EXTENSIONS, i);
            if (strstr(ext, "_texture_compression_s3tc")) {
                _sg_gl.features[SG_FEATURE_TEXTURE_COMPRESSION_DXT] = true;
                continue;
            }
            else if (strstr(ext, "_texture_filter_anisotropic")) {
                _sg_gl.ext_anisotropic = true;
                continue;
            }
        }
    #elif defined(SOKOL_GLES3)
        _sg_gl.features[SG_FEATURE_INSTANCING] = true;
        _sg_gl.features[SG_FEATURE_TEXTURE_FLOAT] = true;
        _sg_gl.features[SG_FEATURE_TEXTURE_HALF_FLOAT] = true;
        _sg_gl.features[SG_FEATURE_MSAA_RENDER_TARGETS] = true;
        _sg_gl.features[SG_FEATURE_PACKED_VERTEX_FORMAT_10_2] = true;
        _sg_gl.features[SG_FEATURE_MULTIPLE_RENDER_TARGET] = true;
        _sg_gl.features[SG_FEATURE_IMAGETYPE_3D] = true;
        _sg_gl.features[SG_FEATURE_IMAGETYPE_ARRAY] = true;
        const char* ext = (const char*) glGetString(GL_EXTENSIONS);
        _sg_gl.ext_anisotropic = strstr(ext, "_texture_filter_anisotropic");
        _sg_gl.features[SG_FEATURE_TEXTURE_COMPRESSION_DXT] =
            strstr(ext, "_texture_compression_s3tc") ||
            strstr(ext, "_compressed_texture_s3tc") ||
            strstr(ext, "texture_compression_dxt1");
        _sg_gl.features[SG_FEATURE_TEXTURE_COMPRESSION_PVRTC] =
            strstr(ext, "_texture_compression_pvrtc") ||
            strstr(ext, "_compressed_texture_pvrtc");
        _sg_gl.features[SG_FEATURE_TEXTURE_COMPRESSION_ATC] =
            strstr(ext, "_compressed_texture_atc");
    #elif defined(SOKOL_GLES2)
        const char* ext = (const char*) glGetString(GL_EXTENSIONS);
        _sg_gl.features[SG_FEATURE_INSTANCING] =
            strstr(ext, "_instanced_arrays");
        _sg_gl.features[SG_FEATURE_TEXTURE_FLOAT] =
            strstr(ext, "_texture_float");
        _sg_gl.features[SG_FEATURE_TEXTURE_HALF_FLOAT] =
            strstr(ext, "_texture_half_float");
        _sg_gl.features[SG_FEATURE_TEXTURE_COMPRESSION_DXT] =
            strstr(ext, "_texture_compression_s3tc") ||
            strstr(ext, "_compressed_texture_s3tc") ||
            strstr(ext, "texture_compression_dxt1");
        _sg_gl.features[SG_FEATURE_TEXTURE_COMPRESSION_PVRTC] =
            strstr(ext, "_texture_compression_pvrtc") ||
            strstr(ext, "_compressed_texture_pvrtc");
        _sg_gl.features[SG_FEATURE_TEXTURE_COMPRESSION_ATC] =
            strstr(ext, "_compressed_texture_atc");
        _sg_gl.ext_anisotropic = 
            strstr(ext, "_texture_filter_anisotropic");
    #endif
    _sg_gl.max_anisotropy = 1;
    if (_sg_gl.ext_anisotropic) {
        glGetIntegerv(GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, &_sg_gl.max_anisotropy);
    }
}

_SOKOL_PRIVATE void _sg_discard_backend() {
    SOKOL_ASSERT(_sg_gl.valid);
    #if !defined(SOKOL_GLES2)
    if (!_sg_gl_gles2) {
        glDeleteVertexArrays(1, &_sg_gl.vao);
        _sg_gl.vao = 0;
    }
    #endif
    _sg_gl.valid = false;
}

_SOKOL_PRIVATE bool _sg_query_feature(sg_feature f) {
    SOKOL_ASSERT((f>=0) && (f<SG_NUM_FEATURES));
    return _sg_gl.features[f];
}

/*-- GL backend resource creation and destruction ----------------------------*/
_SOKOL_PRIVATE void _sg_create_buffer(_sg_buffer* buf, const sg_buffer_desc* desc) {
    SOKOL_ASSERT(buf && desc);
    SOKOL_ASSERT(buf->slot.state == SG_RESOURCESTATE_ALLOC);
    _SG_GL_CHECK_ERROR();
    buf->size = desc->size;
    buf->type = _sg_def(desc->type, SG_BUFFERTYPE_VERTEXBUFFER);
    buf->usage = _sg_def(desc->usage, SG_USAGE_IMMUTABLE);
    buf->upd_frame_index = 0;
    buf->num_slots = (buf->usage == SG_USAGE_IMMUTABLE) ? 1 : SG_NUM_INFLIGHT_FRAMES;
    buf->active_slot = 0;
    buf->ext_buffers = (0 != desc->gl_buffers[0]);
    GLenum gl_target = _sg_gl_buffer_target(buf->type);
    GLenum gl_usage  = _sg_gl_usage(buf->usage);
    for (int slot = 0; slot < buf->num_slots; slot++) {
        GLuint gl_buf = 0;
        if (buf->ext_buffers) {
            SOKOL_ASSERT(desc->gl_buffers[slot]);
            gl_buf = desc->gl_buffers[slot];
        }
        else {
            glGenBuffers(1, &gl_buf);
            glBindBuffer(gl_target, gl_buf);
            glBufferData(gl_target, buf->size, 0, gl_usage);
            if (buf->usage == SG_USAGE_IMMUTABLE) {
                SOKOL_ASSERT(desc->content);
                glBufferSubData(gl_target, 0, buf->size, desc->content);
            }
        }
        buf->gl_buf[slot] = gl_buf;
    }
    _SG_GL_CHECK_ERROR();
    buf->slot.state = SG_RESOURCESTATE_VALID;
}

_SOKOL_PRIVATE void _sg_destroy_buffer(_sg_buffer* buf) {
    SOKOL_ASSERT(buf);
    _SG_GL_CHECK_ERROR();
    if (!buf->ext_buffers) {
        for (int slot = 0; slot < buf->num_slots; slot++) {
            if (buf->gl_buf[slot]) {
                glDeleteBuffers(1, &buf->gl_buf[slot]);
            }
        }
        _SG_GL_CHECK_ERROR();
    }
    _sg_init_buffer(buf);
}

_SOKOL_PRIVATE bool _sg_gl_supported_texture_format(sg_pixel_format fmt) {
    switch (fmt) {
        case SG_PIXELFORMAT_DXT1:
        case SG_PIXELFORMAT_DXT3:
        case SG_PIXELFORMAT_DXT5:
            return _sg_gl.features[SG_FEATURE_TEXTURE_COMPRESSION_DXT];
        case SG_PIXELFORMAT_PVRTC2_RGB:
        case SG_PIXELFORMAT_PVRTC4_RGB:
        case SG_PIXELFORMAT_PVRTC2_RGBA:
        case SG_PIXELFORMAT_PVRTC4_RGBA:
            return _sg_gl.features[SG_FEATURE_TEXTURE_COMPRESSION_PVRTC];
        case SG_PIXELFORMAT_ETC2_RGB8:
        case SG_PIXELFORMAT_ETC2_SRGB8:
            return _sg_gl.features[SG_FEATURE_TEXTURE_COMPRESSION_ETC2];
        default:
            return true;
    }
}

_SOKOL_PRIVATE void _sg_create_image(_sg_image* img, const sg_image_desc* desc) {
    SOKOL_ASSERT(img && desc);
    SOKOL_ASSERT(img->slot.state == SG_RESOURCESTATE_ALLOC);
    _SG_GL_CHECK_ERROR();
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

    /* check if texture format is support */
    if (!_sg_gl_supported_texture_format(img->pixel_format)) {
        SOKOL_LOG("compressed texture format not supported by GL context\n");
        img->slot.state = SG_RESOURCESTATE_FAILED;
        return;
    }
    /* check for optional texture types */
    if ((img->type == SG_IMAGETYPE_3D) && !_sg_gl.features[SG_FEATURE_IMAGETYPE_3D]) {
        SOKOL_LOG("3D textures not supported by GL context\n");
        img->slot.state = SG_RESOURCESTATE_FAILED;
        return;
    }
    if ((img->type == SG_IMAGETYPE_ARRAY) && !_sg_gl.features[SG_FEATURE_IMAGETYPE_ARRAY]) {
        SOKOL_LOG("array textures not supported by GL context\n");
        img->slot.state = SG_RESOURCESTATE_FAILED;
        return;
    }

    /* create 1 or 2 GL textures, depending on requested update strategy */
    img->num_slots = (img->usage == SG_USAGE_IMMUTABLE) ? 1 : SG_NUM_INFLIGHT_FRAMES;
    img->active_slot = 0;
    img->ext_textures = (0 != desc->gl_textures[0]);

    #if !defined(SOKOL_GLES2)
    bool msaa = false;
    if (!_sg_gl_gles2) {
        msaa = (img->sample_count > 1) && (_sg_gl.features[SG_FEATURE_MSAA_RENDER_TARGETS]);
    }
    #endif
    
    if (_sg_is_valid_rendertarget_depth_format(img->pixel_format)) {
        /* special case depth-stencil-buffer? */
        SOKOL_ASSERT((img->usage == SG_USAGE_IMMUTABLE) && (img->num_slots == 1));
        SOKOL_ASSERT(!img->ext_textures);   /* cannot provide external texture for depth images */
        glGenRenderbuffers(1, &img->gl_depth_render_buffer);
        glBindRenderbuffer(GL_RENDERBUFFER, img->gl_depth_render_buffer);
        GLenum gl_depth_format = _sg_gl_depth_attachment_format(img->pixel_format);
        #if !defined(SOKOL_GLES2)
        if (msaa) {
            glRenderbufferStorageMultisample(GL_RENDERBUFFER, img->sample_count, gl_depth_format, img->width, img->height);
        }
        else
        #endif
        {
            glRenderbufferStorage(GL_RENDERBUFFER, gl_depth_format, img->width, img->height);
        }
    }
    else {
        /* regular color texture */
        img->gl_target = _sg_gl_texture_target(img->type);
        const GLenum gl_internal_format = _sg_gl_teximage_internal_format(img->pixel_format);

        /* if this is a MSAA render target, need to create a separate render buffer */
        #if !defined(SOKOL_GLES2)
        if (img->render_target && msaa) {
            glGenRenderbuffers(1, &img->gl_msaa_render_buffer);
            glBindRenderbuffer(GL_RENDERBUFFER, img->gl_msaa_render_buffer);
            glRenderbufferStorageMultisample(GL_RENDERBUFFER, img->sample_count, gl_internal_format, img->width, img->height);
        }
        #endif

        if (img->ext_textures) {
            /* inject externally GL textures */
            for (int slot = 0; slot < img->num_slots; slot++) {
                SOKOL_ASSERT(desc->gl_textures[slot]);
                img->gl_tex[slot] = desc->gl_textures[slot];
            } 
        }
        else {
            /* create our own GL texture(s) */
            const GLenum gl_format = _sg_gl_teximage_format(img->pixel_format);
            const bool is_compressed = _sg_is_compressed_pixel_format(img->pixel_format);
            for (int slot = 0; slot < img->num_slots; slot++) {
                glGenTextures(1, &img->gl_tex[slot]);
                glActiveTexture(GL_TEXTURE0);
                glBindTexture(img->gl_target, img->gl_tex[slot]);
                GLenum gl_min_filter = _sg_gl_filter(img->min_filter);
                GLenum gl_mag_filter = _sg_gl_filter(img->mag_filter);
                glTexParameteri(img->gl_target, GL_TEXTURE_MIN_FILTER, gl_min_filter);
                glTexParameteri(img->gl_target, GL_TEXTURE_MAG_FILTER, gl_mag_filter);
                if (_sg_gl.ext_anisotropic && (img->max_anisotropy > 1)) {
                    GLint max_aniso = (GLint) img->max_anisotropy;
                    if (max_aniso > _sg_gl.max_anisotropy) {
                        max_aniso = _sg_gl.max_anisotropy;
                    }
                    glTexParameteri(img->gl_target, GL_TEXTURE_MAX_ANISOTROPY_EXT, max_aniso);
                }
                if (img->type == SG_IMAGETYPE_CUBE) {
                    glTexParameteri(img->gl_target, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
                    glTexParameteri(img->gl_target, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
                }
                else {
                    glTexParameteri(img->gl_target, GL_TEXTURE_WRAP_S, _sg_gl_wrap(img->wrap_u));
                    glTexParameteri(img->gl_target, GL_TEXTURE_WRAP_T, _sg_gl_wrap(img->wrap_v));
                    #if !defined(SOKOL_GLES2)
                    if (!_sg_gl_gles2 && (img->type == SG_IMAGETYPE_3D)) {
                        glTexParameteri(img->gl_target, GL_TEXTURE_WRAP_R, _sg_gl_wrap(img->wrap_w));
                    }
                    #endif
                }
                #if !defined(SOKOL_GLES2)
                    if (!_sg_gl_gles2) {
                        /* GL spec has strange defaults for mipmap min/max lod: -1000 to +1000 */
                        const float min_lod = _sg_clamp(desc->min_lod, 0.0f, 1000.0f);
                        const float max_lod = _sg_clamp(_sg_def_flt(desc->max_lod, 1000.0f), 0.0f, 1000.0f);
                        glTexParameterf(img->gl_target, GL_TEXTURE_MIN_LOD, min_lod);
                        glTexParameterf(img->gl_target, GL_TEXTURE_MAX_LOD, max_lod);
                    }
                #endif
                const int num_faces = img->type == SG_IMAGETYPE_CUBE ? 6 : 1;
                int data_index = 0;
                for (int face_index = 0; face_index < num_faces; face_index++) {
                    for (int mip_index = 0; mip_index < img->num_mipmaps; mip_index++, data_index++) {
                        GLenum gl_img_target = img->gl_target;
                        if (SG_IMAGETYPE_CUBE == img->type) {
                            gl_img_target = _sg_gl_cubeface_target(face_index);
                        }
                        const GLvoid* data_ptr = desc->content.subimage[face_index][mip_index].ptr;
                        const int data_size = desc->content.subimage[face_index][mip_index].size;
                        int mip_width = img->width >> mip_index;
                        if (mip_width == 0) {
                            mip_width = 1;
                        }
                        int mip_height = img->height >> mip_index;
                        if (mip_height == 0) {
                            mip_height = 1;
                        }
                        if ((SG_IMAGETYPE_2D == img->type) || (SG_IMAGETYPE_CUBE == img->type)) {
                            if (is_compressed) {
                                glCompressedTexImage2D(gl_img_target, mip_index, gl_internal_format,
                                    mip_width, mip_height, 0, data_size, data_ptr);
                            }
                            else {
                                const GLenum gl_type = _sg_gl_teximage_type(img->pixel_format);
                                glTexImage2D(gl_img_target, mip_index, gl_internal_format,
                                    mip_width, mip_height, 0, gl_format, gl_type, data_ptr);
                            }
                        }
                        #if !defined(SOKOL_GLES2)
                        else if (!_sg_gl_gles2 && ((SG_IMAGETYPE_3D == img->type) || (SG_IMAGETYPE_ARRAY == img->type))) {
                            int mip_depth = img->depth >> mip_index;
                            if (mip_depth == 0) {
                                mip_depth = 1;
                            }
                            if (is_compressed) {
                                glCompressedTexImage3D(gl_img_target, mip_index, gl_internal_format,
                                    mip_width, mip_height, mip_depth, 0, data_size, data_ptr);
                            }
                            else {
                                const GLenum gl_type = _sg_gl_teximage_type(img->pixel_format);
                                glTexImage3D(gl_img_target, mip_index, gl_internal_format,
                                    mip_width, mip_height, mip_depth, 0, gl_format, gl_type, data_ptr);
                            }
                        }
                        #endif
                    }
                }
            }
        }
    }
    _SG_GL_CHECK_ERROR();
    img->slot.state = SG_RESOURCESTATE_VALID;
}

_SOKOL_PRIVATE void _sg_destroy_image(_sg_image* img) {
    SOKOL_ASSERT(img);
    _SG_GL_CHECK_ERROR();
    if (!img->ext_textures) {
        for (int slot = 0; slot < img->num_slots; slot++) {
            if (img->gl_tex[slot]) {
                glDeleteTextures(1, &img->gl_tex[slot]);
            }
        }
    }
    if (img->gl_depth_render_buffer) {
        glDeleteRenderbuffers(1, &img->gl_depth_render_buffer);
    }
    if (img->gl_msaa_render_buffer) {
        glDeleteRenderbuffers(1, &img->gl_msaa_render_buffer);
    }
    _SG_GL_CHECK_ERROR();
    _sg_init_image(img);
}

_SOKOL_PRIVATE GLuint _sg_gl_compile_shader(sg_shader_stage stage, const char* src) {
    SOKOL_ASSERT(src);
    _SG_GL_CHECK_ERROR();
    GLuint gl_shd = glCreateShader(_sg_gl_shader_stage(stage));
    glShaderSource(gl_shd, 1, &src, 0);
    glCompileShader(gl_shd);
    GLint compile_status = 0;
    glGetShaderiv(gl_shd, GL_COMPILE_STATUS, &compile_status);
    if (!compile_status) {
        /* compilation failed, log error and delete shader */
        GLint log_len = 0;
        glGetShaderiv(gl_shd, GL_INFO_LOG_LENGTH, &log_len);
        if (log_len > 0) {
            GLchar* log_buf = (GLchar*) SOKOL_MALLOC(log_len);
            glGetShaderInfoLog(gl_shd, log_len, &log_len, log_buf);
            SOKOL_LOG(log_buf);
            SOKOL_FREE(log_buf);
        }
        glDeleteShader(gl_shd);
        gl_shd = 0;
    }
    _SG_GL_CHECK_ERROR();
    return gl_shd;
}

_SOKOL_PRIVATE void _sg_create_shader(_sg_shader* shd, const sg_shader_desc* desc) {
    SOKOL_ASSERT(shd && desc);
    SOKOL_ASSERT(shd->slot.state == SG_RESOURCESTATE_ALLOC);
    SOKOL_ASSERT(!shd->gl_prog);
    _SG_GL_CHECK_ERROR();
    GLuint gl_vs = _sg_gl_compile_shader(SG_SHADERSTAGE_VS, desc->vs.source);
    GLuint gl_fs = _sg_gl_compile_shader(SG_SHADERSTAGE_FS, desc->fs.source);
    if (!(gl_vs && gl_fs)) {
        shd->slot.state = SG_RESOURCESTATE_FAILED;
        return;
    }
    GLuint gl_prog = glCreateProgram();
    glAttachShader(gl_prog, gl_vs);
    glAttachShader(gl_prog, gl_fs);
    glLinkProgram(gl_prog);
    glDeleteShader(gl_vs);
    glDeleteShader(gl_fs);
    _SG_GL_CHECK_ERROR();

    GLint link_status;
    glGetProgramiv(gl_prog, GL_LINK_STATUS, &link_status);
    if (!link_status) {
        GLint log_len = 0;
        glGetProgramiv(gl_prog, GL_INFO_LOG_LENGTH, &log_len);
        if (log_len > 0) {
            GLchar* log_buf = (GLchar*) SOKOL_MALLOC(log_len);
            glGetProgramInfoLog(gl_prog, log_len, &log_len, log_buf);
            SOKOL_LOG(log_buf);
            SOKOL_FREE(log_buf);
        }
        glDeleteProgram(gl_prog);
        shd->slot.state = SG_RESOURCESTATE_FAILED;
        return;
    }
    shd->gl_prog = gl_prog;

    /* resolve uniforms */
    _SG_GL_CHECK_ERROR();
    for (int stage_index = 0; stage_index < SG_NUM_SHADER_STAGES; stage_index++) {
        const sg_shader_stage_desc* stage_desc = (stage_index == SG_SHADERSTAGE_VS)? &desc->vs : &desc->fs;
        _sg_shader_stage* stage = &shd->stage[stage_index];
        SOKOL_ASSERT(stage->num_uniform_blocks == 0);
        for (int ub_index = 0; ub_index < SG_MAX_SHADERSTAGE_UBS; ub_index++) {
            const sg_shader_uniform_block_desc* ub_desc = &stage_desc->uniform_blocks[ub_index];
            if (0 == ub_desc->size) {
                break;
            }
            _sg_uniform_block* ub = &stage->uniform_blocks[ub_index];
            ub->size = ub_desc->size;
            SOKOL_ASSERT(ub->num_uniforms == 0);
            int cur_uniform_offset = 0;
            for (int u_index = 0; u_index < SG_MAX_UB_MEMBERS; u_index++) {
                const sg_shader_uniform_desc* u_desc = &ub_desc->uniforms[u_index];
                if (u_desc->type == SG_UNIFORMTYPE_INVALID) {
                    break;
                }
                _sg_uniform* u = &ub->uniforms[u_index];
                u->type = u_desc->type;
                u->count = _sg_def(u_desc->array_count, 1);
                u->offset = cur_uniform_offset;
                cur_uniform_offset += _sg_uniform_size(u->type, u->count);
                if (u_desc->name) {
                    u->gl_loc = glGetUniformLocation(gl_prog, u_desc->name);
                }
                else {
                    u->gl_loc = u_index;
                }
                ub->num_uniforms++;
            }
            SOKOL_ASSERT(ub_desc->size == cur_uniform_offset);
            stage->num_uniform_blocks++;
        }
    }

    /* resolve image locations */
    _SG_GL_CHECK_ERROR();
    int gl_tex_slot = 0;
    for (int stage_index = 0; stage_index < SG_NUM_SHADER_STAGES; stage_index++) {
        const sg_shader_stage_desc* stage_desc = (stage_index == SG_SHADERSTAGE_VS)? &desc->vs : &desc->fs;
        _sg_shader_stage* stage = &shd->stage[stage_index];
        SOKOL_ASSERT(stage->num_images == 0);
        for (int img_index = 0; img_index < SG_MAX_SHADERSTAGE_IMAGES; img_index++) {
            const sg_shader_image_desc* img_desc = &stage_desc->images[img_index];
            if (img_desc->type == _SG_IMAGETYPE_DEFAULT) {
                break;
            }
            _sg_shader_image* img = &stage->images[img_index];
            img->type = img_desc->type;
            img->gl_loc = img_index;
            if (img_desc->name) {
                img->gl_loc = glGetUniformLocation(gl_prog, img_desc->name);
            }
            if (img->gl_loc != -1) {
                img->gl_tex_slot = gl_tex_slot++;
            }
            else {
                img->gl_tex_slot = -1;
            }
            stage->num_images++;
        }
    }
    _SG_GL_CHECK_ERROR();
    shd->slot.state = SG_RESOURCESTATE_VALID;
}

_SOKOL_PRIVATE void _sg_destroy_shader(_sg_shader* shd) {
    SOKOL_ASSERT(shd);
    _SG_GL_CHECK_ERROR();
    if (shd->gl_prog) {
        glDeleteProgram(shd->gl_prog);
    }
    _SG_GL_CHECK_ERROR();
    _sg_init_shader(shd);
}

_SOKOL_PRIVATE void _sg_gl_load_stencil(const sg_stencil_state* src, sg_stencil_state* dst) {
    dst->fail_op = _sg_def(src->fail_op, SG_STENCILOP_KEEP);
    dst->depth_fail_op = _sg_def(src->depth_fail_op, SG_STENCILOP_KEEP);
    dst->pass_op = _sg_def(src->pass_op, SG_STENCILOP_KEEP);
    dst->compare_func = _sg_def(src->compare_func, SG_COMPAREFUNC_ALWAYS);
}

_SOKOL_PRIVATE void _sg_gl_load_depth_stencil(const sg_depth_stencil_state* src, sg_depth_stencil_state* dst) {
    _sg_gl_load_stencil(&src->stencil_front, &dst->stencil_front);
    _sg_gl_load_stencil(&src->stencil_back, &dst->stencil_back);
    dst->depth_compare_func = _sg_def(src->depth_compare_func, SG_COMPAREFUNC_ALWAYS);
    dst->depth_write_enabled = src->depth_write_enabled;
    dst->stencil_enabled = src->stencil_enabled;
    dst->stencil_read_mask = src->stencil_read_mask;
    dst->stencil_write_mask = src->stencil_write_mask;
    dst->stencil_ref = src->stencil_ref; 
}

_SOKOL_PRIVATE void _sg_gl_load_blend(const sg_blend_state* src, sg_blend_state* dst) {
    dst->enabled = src->enabled;
    dst->src_factor_rgb = _sg_def(src->src_factor_rgb, SG_BLENDFACTOR_ONE);
    dst->dst_factor_rgb = _sg_def(src->dst_factor_rgb, SG_BLENDFACTOR_ZERO);
    dst->op_rgb = _sg_def(src->op_rgb, SG_BLENDOP_ADD);
    dst->src_factor_alpha = _sg_def(src->src_factor_alpha, SG_BLENDFACTOR_ONE);
    dst->dst_factor_alpha = _sg_def(src->dst_factor_alpha, SG_BLENDFACTOR_ZERO);
    dst->op_alpha = _sg_def(src->op_alpha, SG_BLENDOP_ADD);
    if (src->color_write_mask == SG_COLORMASK_NONE) {
        dst->color_write_mask = 0;
    }
    else {
        dst->color_write_mask = _sg_def((sg_color_mask)src->color_write_mask, SG_COLORMASK_RGBA);
    }
    for (int i = 0; i < 4; i++) {
        dst->blend_color[i] = src->blend_color[i];
    }
}

_SOKOL_PRIVATE void _sg_gl_load_rasterizer(const sg_rasterizer_state* src, sg_rasterizer_state* dst) {
    dst->alpha_to_coverage_enabled = src->alpha_to_coverage_enabled;
    dst->cull_mode = _sg_def(src->cull_mode, SG_CULLMODE_NONE);
    dst->face_winding = _sg_def(src->face_winding, SG_FACEWINDING_CW);
    dst->sample_count = _sg_def(src->sample_count, 1);
    dst->depth_bias = src->depth_bias;
    dst->depth_bias_slope_scale = src->depth_bias_slope_scale;
    dst->depth_bias_clamp = src->depth_bias_clamp;
}

_SOKOL_PRIVATE void _sg_create_pipeline(_sg_pipeline* pip, _sg_shader* shd, const sg_pipeline_desc* desc) {
    SOKOL_ASSERT(pip && shd && desc);
    SOKOL_ASSERT(pip->slot.state == SG_RESOURCESTATE_ALLOC);
    SOKOL_ASSERT(!pip->shader && pip->shader_id.id == SG_INVALID_ID);
    SOKOL_ASSERT(desc->shader.id == shd->slot.id);
    SOKOL_ASSERT(shd->gl_prog);
    pip->shader = shd;
    pip->shader_id = desc->shader;
    pip->primitive_type = _sg_def(desc->primitive_type, SG_PRIMITIVETYPE_TRIANGLES);
    pip->index_type = _sg_def(desc->index_type, SG_INDEXTYPE_NONE);
    pip->color_attachment_count = _sg_def(desc->blend.color_attachment_count, 1);
    pip->color_format = _sg_def(desc->blend.color_format, SG_PIXELFORMAT_RGBA8);
    pip->depth_format = _sg_def(desc->blend.depth_format, SG_PIXELFORMAT_DEPTHSTENCIL);
    pip->sample_count = _sg_def(desc->rasterizer.sample_count, 1);
    _sg_gl_load_depth_stencil(&desc->depth_stencil, &pip->depth_stencil);
    _sg_gl_load_blend(&desc->blend, &pip->blend);
    _sg_gl_load_rasterizer(&desc->rasterizer, &pip->rast);

    /* resolve vertex attributes */
    for (int i = 0; i < SG_MAX_VERTEX_ATTRIBUTES; i++) {
        pip->gl_attrs[i].vb_index = -1;
    }
    for (int layout_index = 0; layout_index < SG_MAX_SHADERSTAGE_BUFFERS; layout_index++) {
        const sg_vertex_layout_desc* layout_desc = &desc->vertex_layouts[layout_index];
        if (layout_desc->stride == 0) {
            break;
        }
        pip->vertex_layout_valid[layout_index] = true;
        const sg_vertex_step step_func = _sg_def(layout_desc->step_func, SG_VERTEXSTEP_PER_VERTEX);
        const int step_rate = _sg_def(layout_desc->step_rate, 1);
        for (int attr_index = 0; attr_index < SG_MAX_VERTEX_ATTRIBUTES; attr_index++) {
            const sg_vertex_attr_desc* attr_desc = &layout_desc->attrs[attr_index];
            if (attr_desc->format == SG_VERTEXFORMAT_INVALID) {
                break;
            }
            GLint attr_loc = attr_index;
            if (attr_desc->name) {
                attr_loc = glGetAttribLocation(pip->shader->gl_prog, attr_desc->name);
            }
            SOKOL_ASSERT(attr_loc < SG_MAX_VERTEX_ATTRIBUTES);
            if (attr_loc != -1) {
                _sg_gl_attr* gl_attr = &pip->gl_attrs[attr_loc];
                SOKOL_ASSERT(gl_attr->vb_index == -1);
                gl_attr->vb_index = layout_index;
                if (step_func == SG_VERTEXSTEP_PER_VERTEX) {
                    gl_attr->divisor = 0;
                }
                else {
                    gl_attr->divisor = step_rate;
                }
                gl_attr->stride = layout_desc->stride;
                gl_attr->offset = attr_desc->offset;
                const sg_vertex_format fmt = attr_desc->format;
                gl_attr->size = _sg_gl_vertexformat_size(fmt);
                gl_attr->type = _sg_gl_vertexformat_type(fmt);
                gl_attr->normalized = _sg_gl_vertexformat_normalized(fmt);
            }
            else {
                SOKOL_LOG("Vertex attribute not found in shader: ");
                SOKOL_LOG(attr_desc->name);
            }
        }
    }
    pip->slot.state = SG_RESOURCESTATE_VALID;
}

_SOKOL_PRIVATE void _sg_destroy_pipeline(_sg_pipeline* pip) {
    SOKOL_ASSERT(pip);
    _sg_init_pipeline(pip);
}

/*
    _sg_create_pass

    att_imgs must point to a _sg_image* att_imgs[SG_MAX_COLOR_ATTACHMENTS+1] array,
    first entries are the color attachment images (or nullptr), last entry
    is the depth-stencil image (or nullptr).
*/
_SOKOL_PRIVATE void _sg_create_pass(_sg_pass* pass, _sg_image** att_images, const sg_pass_desc* desc) {
    SOKOL_ASSERT(pass && att_images && desc);
    SOKOL_ASSERT(pass->slot.state == SG_RESOURCESTATE_ALLOC);
    SOKOL_ASSERT(att_images && att_images[0]);
    _SG_GL_CHECK_ERROR();

    /* copy image pointers and desc attributes */
    const sg_attachment_desc* att_desc;
    _sg_attachment* att;
    for (int i = 0; i < SG_MAX_COLOR_ATTACHMENTS; i++) {
        SOKOL_ASSERT(0 == pass->color_atts[i].image);
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
        }
    }
    SOKOL_ASSERT(0 == pass->ds_att.image);
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
    }

    /* store current framebuffer binding (restored at end of function) */
    GLuint gl_orig_fb;
    glGetIntegerv(GL_FRAMEBUFFER_BINDING, (GLint*)&gl_orig_fb);

    /* create a framebuffer object */
    glGenFramebuffers(1, &pass->gl_fb);
    glBindFramebuffer(GL_FRAMEBUFFER, pass->gl_fb);

    /* attach msaa render buffer or textures */
    const bool is_msaa = (0 != att_images[0]->gl_msaa_render_buffer);
    if (is_msaa) {
        for (int i = 0; i < SG_MAX_COLOR_ATTACHMENTS; i++) {
            const _sg_image* att_img = pass->color_atts[i].image;
            if (att_img) {
                const GLuint gl_render_buffer = att_img->gl_msaa_render_buffer;
                SOKOL_ASSERT(gl_render_buffer);
                glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0+i, GL_RENDERBUFFER, gl_render_buffer);
            }
        }
    }
    else {
        for (int i = 0; i < SG_MAX_COLOR_ATTACHMENTS; i++) {
            const _sg_image* att_img = pass->color_atts[i].image;
            const int mip_level = pass->color_atts[i].mip_level;
            const int slice = pass->color_atts[i].slice;
            if (att_img) {
                const GLuint gl_tex = att_img->gl_tex[0];
                SOKOL_ASSERT(gl_tex);
                const GLenum gl_att = GL_COLOR_ATTACHMENT0 + i;
                switch (att_img->type) {
                    case SG_IMAGETYPE_2D:
                        glFramebufferTexture2D(GL_FRAMEBUFFER, gl_att, GL_TEXTURE_2D, gl_tex, mip_level);
                        break;
                    case SG_IMAGETYPE_CUBE:
                        glFramebufferTexture2D(GL_FRAMEBUFFER, gl_att, _sg_gl_cubeface_target(slice), gl_tex, mip_level);
                        break;
                    default:
                        /* 3D- or array-texture */
                        #if !defined(SOKOL_GLES2)
                        if (!_sg_gl_gles2) {
                            glFramebufferTextureLayer(GL_FRAMEBUFFER, gl_att, gl_tex, mip_level, slice);
                        }
                        #endif
                        break;
                }
            }
        }
    }

    /* attach depth-stencil buffer to framebuffer */
    if (pass->ds_att.image) {
        const GLuint gl_render_buffer = pass->ds_att.image->gl_depth_render_buffer;
        SOKOL_ASSERT(gl_render_buffer);
        glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, gl_render_buffer);
        if (_sg_is_depth_stencil_format(pass->ds_att.image->pixel_format)) {
            glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_STENCIL_ATTACHMENT, GL_RENDERBUFFER, gl_render_buffer);
        }
    }

    /* check if framebuffer is complete */
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        SOKOL_LOG("Framebuffer completeness check failed!\n");
        pass->slot.state = SG_RESOURCESTATE_FAILED;
        return;
    }
    
    /* create MSAA resolve framebuffers if necessary */
    if (is_msaa) {
        for (int i = 0; i < SG_MAX_COLOR_ATTACHMENTS; i++) {
            _sg_attachment* att = &pass->color_atts[i];
            if (att->image) {
                SOKOL_ASSERT(0 == att->gl_msaa_resolve_buffer);
                glGenFramebuffers(1, &att->gl_msaa_resolve_buffer);
                glBindFramebuffer(GL_FRAMEBUFFER, att->gl_msaa_resolve_buffer);
                const GLuint gl_tex = att->image->gl_tex[0];
                SOKOL_ASSERT(gl_tex);
                switch (att->image->type) {
                    case SG_IMAGETYPE_2D:
                        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, 
                            GL_TEXTURE_2D, gl_tex, att->mip_level);
                        break;
                    case SG_IMAGETYPE_CUBE:
                        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, 
                            _sg_gl_cubeface_target(att->slice), gl_tex, att->mip_level);
                        break;
                    default:
                        #if !defined(SOKOL_GLES2)
                        if (!_sg_gl_gles2) {
                            glFramebufferTextureLayer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, gl_tex, att->mip_level, att->slice);
                        }
                        #endif
                        break;
                }
                /* check if framebuffer is complete */
                if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
                    SOKOL_LOG("Framebuffer completeness check failed (msaa resolve buffer)!\n");
                    pass->slot.state = SG_RESOURCESTATE_FAILED;
                    return;
                }
            }
        }
    }

    /* restore original framebuffer binding */
    glBindFramebuffer(GL_FRAMEBUFFER, gl_orig_fb);
    _SG_GL_CHECK_ERROR();
    pass->slot.state = SG_RESOURCESTATE_VALID;
}

_SOKOL_PRIVATE void _sg_destroy_pass(_sg_pass* pass) {
    SOKOL_ASSERT(pass);
    _SG_GL_CHECK_ERROR();
    if (0 != pass->gl_fb) {
        glDeleteFramebuffers(1, &pass->gl_fb);
    }
    for (int i = 0; i < SG_MAX_COLOR_ATTACHMENTS; i++) {
        if (pass->color_atts[i].gl_msaa_resolve_buffer) {
            glDeleteFramebuffers(1, &pass->color_atts[i].gl_msaa_resolve_buffer);
        }
    }
    if (pass->ds_att.gl_msaa_resolve_buffer) {
        glDeleteFramebuffers(1, &pass->ds_att.gl_msaa_resolve_buffer);
    }
    _SG_GL_CHECK_ERROR();
    _sg_init_pass(pass);
}

/*-- GL backend rendering functions ------------------------------------------*/
_SOKOL_PRIVATE void _sg_begin_pass(_sg_pass* pass, const sg_pass_action* action, int w, int h) {
    /* FIXME: what if a texture used as render target is still bound, should we
       unbind all currently bound textures in begin pass? */
    SOKOL_ASSERT(action);
    SOKOL_ASSERT(!_sg_gl.in_pass);
    _SG_GL_CHECK_ERROR();
    _sg_gl.in_pass = true;
    _sg_gl.cur_pass = pass; /* can be 0 */
    if (pass) {
        _sg_gl.cur_pass_id.id = pass->slot.id;
    }
    else {
        _sg_gl.cur_pass_id.id = SG_INVALID_ID;
    }
    _sg_gl.cur_pass_width = w;
    _sg_gl.cur_pass_height = h;
    if (pass) {
        /* offscreen pass */
        SOKOL_ASSERT(pass->gl_fb);
        glBindFramebuffer(GL_FRAMEBUFFER, pass->gl_fb);
        #if !defined(SOKOL_GLES2)
        if (!_sg_gl_gles2) {
            GLenum att[SG_MAX_COLOR_ATTACHMENTS] = {
                GL_COLOR_ATTACHMENT0,
                GL_COLOR_ATTACHMENT1,
                GL_COLOR_ATTACHMENT2,
                GL_COLOR_ATTACHMENT3
            };
            int num_attrs = 0;
            for (int i = 0; i < SG_MAX_COLOR_ATTACHMENTS; i++) {
                if (pass->color_atts[num_attrs].image) {
                    num_attrs++;
                }
                else {
                    break;
                }
            }
            glDrawBuffers(num_attrs, att);
        }
        #endif
    }
    else {
        /* default pass */
        glBindFramebuffer(GL_FRAMEBUFFER, _sg_gl.default_framebuffer);
    }
    glViewport(0, 0, w, h);
    glScissor(0, 0, w, h);
    bool need_pip_cache_flush = false;
    if (_sg_gl.cache.blend.color_write_mask != SG_COLORMASK_RGBA) {
        need_pip_cache_flush = true;
        _sg_gl.cache.blend.color_write_mask = SG_COLORMASK_RGBA;
        glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
    }
    if (!_sg_gl.cache.ds.depth_write_enabled) {
        need_pip_cache_flush = true;
        _sg_gl.cache.ds.depth_write_enabled = true;
        glDepthMask(GL_TRUE);
    }
    if (_sg_gl.cache.ds.stencil_write_mask != 0xFF) {
        need_pip_cache_flush = true;
        _sg_gl.cache.ds.stencil_write_mask = 0xFF;
        glStencilMask(0xFF);
    }
    if (need_pip_cache_flush) {
        /* we messed with the state cache directly, need to clear cached 
           pipeline to force re-evaluation in next sg_apply_draw_state() */
        _sg_gl.cache.cur_pipeline = 0;
        _sg_gl.cache.cur_pipeline_id.id = SG_INVALID_ID;
    }
    bool use_mrt_clear = (0 != pass);
    #if defined(SOKOL_GLES2)
    use_mrt_clear = false;
    #else
    if (_sg_gl_gles2) {
        use_mrt_clear = false;
    }
    #endif
    if (!use_mrt_clear) {
        GLbitfield clear_mask = 0;
        if (action->colors[0].action == SG_ACTION_CLEAR) {
            clear_mask |= GL_COLOR_BUFFER_BIT;
            const float* c = action->colors[0].val;
            glClearColor(c[0], c[1], c[2], c[3]);
        }
        if (action->depth.action == SG_ACTION_CLEAR) {
            clear_mask |= GL_DEPTH_BUFFER_BIT;
            #ifdef SOKOL_GLCORE33
            glClearDepth(action->depth.val);
            #else
            glClearDepthf(action->depth.val);
            #endif
        }
        if (action->stencil.action == SG_ACTION_CLEAR) {
            clear_mask |= GL_STENCIL_BUFFER_BIT;
            glClearStencil(action->stencil.val);
        }
        if (0 != clear_mask) {
            glClear(clear_mask);
        }
    }
    #if !defined SOKOL_GLES2
    else {
        SOKOL_ASSERT(pass);
        for (int i = 0; i < SG_MAX_COLOR_ATTACHMENTS; i++) {
            if (pass->color_atts[i].image) {
                if (action->colors[i].action == SG_ACTION_CLEAR) {
                    glClearBufferfv(GL_COLOR, i, action->colors[i].val);
                }
            }
            else {
                break;
            }
        }
        if (pass->ds_att.image) {
            if ((action->depth.action == SG_ACTION_CLEAR) && (action->stencil.action == SG_ACTION_CLEAR)) {
                glClearBufferfi(GL_DEPTH_STENCIL, 0, action->depth.val, action->stencil.val);
            }
            else if (action->depth.action == SG_ACTION_CLEAR) {
                glClearBufferfv(GL_DEPTH, 0, &action->depth.val);
            }
            else if (action->stencil.action == SG_ACTION_CLEAR) {
                GLuint val = action->stencil.val;
                glClearBufferuiv(GL_STENCIL, 0, &val);
            }
        }
    }
    #endif
    _SG_GL_CHECK_ERROR();
}

_SOKOL_PRIVATE void _sg_end_pass() {
    SOKOL_ASSERT(_sg_gl.in_pass);
    _SG_GL_CHECK_ERROR();

    /* if this was an offscreen pass, and MSAA rendering was used, need 
       to resolve into the pass images */
    #if !defined(SOKOL_GLES2)
    if (!_sg_gl_gles2 && _sg_gl.cur_pass) {
        /* check if the pass object is still valid */
        const _sg_pass* pass = _sg_gl.cur_pass;
        SOKOL_ASSERT(pass->slot.id == _sg_gl.cur_pass_id.id);
        bool is_msaa = (0 != _sg_gl.cur_pass->color_atts[0].gl_msaa_resolve_buffer);
        if (is_msaa) {
            SOKOL_ASSERT(pass->gl_fb);
            glBindFramebuffer(GL_READ_FRAMEBUFFER, pass->gl_fb);
            SOKOL_ASSERT(pass->color_atts[0].image);
            const int w = pass->color_atts[0].image->width;
            const int h = pass->color_atts[0].image->height;
            for (int att_index = 0; att_index < SG_MAX_COLOR_ATTACHMENTS; att_index++) {
                const _sg_attachment* att = &pass->color_atts[att_index];
                if (att->image) {
                    SOKOL_ASSERT(att->gl_msaa_resolve_buffer);
                    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, att->gl_msaa_resolve_buffer);
                    glReadBuffer(GL_COLOR_ATTACHMENT0 + att_index);
                    const GLenum gl_att = GL_COLOR_ATTACHMENT0;
                    glDrawBuffers(1, &gl_att);
                    glBlitFramebuffer(0, 0, w, h, 0, 0, w, h, GL_COLOR_BUFFER_BIT, GL_NEAREST);
                }
                else {
                    break;
                }
            }
        }
    }
    #endif
    _sg_gl.cur_pass = 0;
    _sg_gl.cur_pass_id.id = SG_INVALID_ID;
    _sg_gl.cur_pass_width = 0;
    _sg_gl.cur_pass_height = 0;

    glBindFramebuffer(GL_FRAMEBUFFER, _sg_gl.default_framebuffer);
    _sg_gl.in_pass = false;
    _SG_GL_CHECK_ERROR();
}

_SOKOL_PRIVATE void _sg_apply_viewport(int x, int y, int w, int h, bool origin_top_left) {
    SOKOL_ASSERT(_sg_gl.in_pass);
    y = origin_top_left ? (_sg_gl.cur_pass_height - (y+h)) : y;
    glViewport(x, y, w, h);
}

_SOKOL_PRIVATE void _sg_apply_scissor_rect(int x, int y, int w, int h, bool origin_top_left) {
    SOKOL_ASSERT(_sg_gl.in_pass);
    y = origin_top_left ? (_sg_gl.cur_pass_height - (y+h)) : y;
    glScissor(x, y, w, h);
}

_SOKOL_PRIVATE void _sg_apply_draw_state( 
    _sg_pipeline* pip, 
    _sg_buffer** vbs, int num_vbs, _sg_buffer* ib,
    _sg_image** vs_imgs, int num_vs_imgs,
    _sg_image** fs_imgs, int num_fs_imgs)
{
    SOKOL_ASSERT(pip);
    SOKOL_ASSERT(pip->shader);
    _SG_GL_CHECK_ERROR();

    /* need to apply pipeline state? */
    if ((_sg_gl.cache.cur_pipeline != pip) || (_sg_gl.cache.cur_pipeline_id.id != pip->slot.id)) {
        _sg_gl.cache.cur_pipeline = pip;
        _sg_gl.cache.cur_pipeline_id.id = pip->slot.id;
        _sg_gl.cache.cur_primitive_type = _sg_gl_primitive_type(pip->primitive_type);
        _sg_gl.cache.cur_index_type = _sg_gl_index_type(pip->index_type);

        /* update depth-stencil state */
        const sg_depth_stencil_state* new_ds = &pip->depth_stencil;
        sg_depth_stencil_state* cache_ds = &_sg_gl.cache.ds;
        if (new_ds->depth_compare_func != cache_ds->depth_compare_func) {
            cache_ds->depth_compare_func = new_ds->depth_compare_func;
            glDepthFunc(_sg_gl_compare_func(new_ds->depth_compare_func));
        }
        if (new_ds->depth_write_enabled != cache_ds->depth_write_enabled) {
            cache_ds->depth_write_enabled = new_ds->depth_write_enabled;
            glDepthMask(new_ds->depth_write_enabled);
        }
        if (new_ds->stencil_enabled != cache_ds->stencil_enabled) {
            cache_ds->stencil_enabled = new_ds->stencil_enabled;
            if (new_ds->stencil_enabled) glEnable(GL_STENCIL_TEST); 
            else glDisable(GL_STENCIL_TEST);
        }
        if (new_ds->stencil_write_mask != cache_ds->stencil_write_mask) {
            cache_ds->stencil_write_mask = new_ds->stencil_write_mask;
            glStencilMask(new_ds->stencil_write_mask);
        }
        for (int i = 0; i < 2; i++) {
            const sg_stencil_state* new_ss = (i==0)? &new_ds->stencil_front : &new_ds->stencil_back;
            sg_stencil_state* cache_ss = (i==0)? &cache_ds->stencil_front : &cache_ds->stencil_back;
            GLenum gl_face = (i==0)? GL_FRONT : GL_BACK;
            if ((new_ss->compare_func != cache_ss->compare_func) ||
                (new_ds->stencil_read_mask != cache_ds->stencil_read_mask) ||
                (new_ds->stencil_ref != cache_ds->stencil_ref))
            {
                cache_ss->compare_func = new_ss->compare_func;
                glStencilFuncSeparate(gl_face, 
                    _sg_gl_compare_func(new_ss->compare_func), 
                    new_ds->stencil_ref, 
                    new_ds->stencil_read_mask);
            }
            if ((new_ss->fail_op != cache_ss->fail_op) ||
                (new_ss->depth_fail_op != cache_ss->depth_fail_op) ||
                (new_ss->pass_op != cache_ss->pass_op))
            {
                cache_ss->fail_op = new_ss->fail_op;
                cache_ss->depth_fail_op = new_ss->depth_fail_op;
                cache_ss->pass_op = new_ss->pass_op;
                glStencilOpSeparate(gl_face,
                    _sg_gl_stencil_op(new_ss->fail_op),
                    _sg_gl_stencil_op(new_ss->depth_fail_op),
                    _sg_gl_stencil_op(new_ss->pass_op));
            }
        }
        cache_ds->stencil_read_mask = new_ds->stencil_read_mask;
        cache_ds->stencil_ref = new_ds->stencil_ref;

        /* update blend state */
        const sg_blend_state* new_b = &pip->blend;
        sg_blend_state* cache_b = &_sg_gl.cache.blend;
        if (new_b->enabled != cache_b->enabled) {
            cache_b->enabled = new_b->enabled;
            if (new_b->enabled) glEnable(GL_BLEND);
            else glDisable(GL_BLEND);
        }
        if ((new_b->src_factor_rgb != cache_b->src_factor_rgb) ||
            (new_b->dst_factor_rgb != cache_b->dst_factor_rgb) ||
            (new_b->src_factor_alpha != cache_b->src_factor_alpha) ||
            (new_b->dst_factor_alpha != cache_b->dst_factor_alpha))
        {
            cache_b->src_factor_rgb = new_b->src_factor_rgb;
            cache_b->dst_factor_rgb = new_b->dst_factor_rgb;
            cache_b->src_factor_alpha = new_b->src_factor_alpha;
            cache_b->dst_factor_alpha = new_b->dst_factor_alpha;
            glBlendFuncSeparate(_sg_gl_blend_factor(new_b->src_factor_rgb),
                _sg_gl_blend_factor(new_b->dst_factor_rgb),
                _sg_gl_blend_factor(new_b->src_factor_alpha),
                _sg_gl_blend_factor(new_b->dst_factor_alpha));
        }
        if ((new_b->op_rgb != cache_b->op_rgb) || (new_b->op_alpha != cache_b->op_alpha)) {
            cache_b->op_rgb = new_b->op_rgb;
            cache_b->op_alpha = new_b->op_alpha;
            glBlendEquationSeparate(_sg_gl_blend_op(new_b->op_rgb), _sg_gl_blend_op(new_b->op_alpha));
        }
        if (new_b->color_write_mask != cache_b->color_write_mask) {
            cache_b->color_write_mask = new_b->color_write_mask;
            glColorMask((new_b->color_write_mask & SG_COLORMASK_R) != 0,
                        (new_b->color_write_mask & SG_COLORMASK_G) != 0,
                        (new_b->color_write_mask & SG_COLORMASK_B) != 0,
                        (new_b->color_write_mask & SG_COLORMASK_A) != 0);
        }
        if (!_sg_fequal(new_b->blend_color[0], cache_b->blend_color[0], 0.0001f) ||
            !_sg_fequal(new_b->blend_color[1], cache_b->blend_color[1], 0.0001f) ||
            !_sg_fequal(new_b->blend_color[2], cache_b->blend_color[2], 0.0001f) ||
            !_sg_fequal(new_b->blend_color[3], cache_b->blend_color[3], 0.0001f))
        {
            const float* bc = new_b->blend_color;
            for (int i=0; i<4; i++) {
                cache_b->blend_color[i] = bc[i];
            }
            glBlendColor(bc[0], bc[1], bc[2], bc[3]);
        }

        /* update rasterizer state */
        const sg_rasterizer_state* new_r = &pip->rast;
        sg_rasterizer_state* cache_r = &_sg_gl.cache.rast;
        if (new_r->cull_mode != cache_r->cull_mode) {
            cache_r->cull_mode = new_r->cull_mode;
            if (SG_CULLMODE_NONE == new_r->cull_mode) {
                glDisable(GL_CULL_FACE);
            }
            else {
                glEnable(GL_CULL_FACE);
                GLenum gl_mode = (SG_CULLMODE_FRONT == new_r->cull_mode) ? GL_FRONT : GL_BACK;
                glCullFace(gl_mode);
            }
        }
        if (new_r->face_winding != cache_r->face_winding) {
            cache_r->face_winding = new_r->face_winding;
            GLenum gl_winding = (SG_FACEWINDING_CW == new_r->face_winding) ? GL_CW : GL_CCW;
            glFrontFace(gl_winding);
        }
        if (new_r->alpha_to_coverage_enabled != cache_r->alpha_to_coverage_enabled) {
            cache_r->alpha_to_coverage_enabled = new_r->alpha_to_coverage_enabled;
            if (new_r->alpha_to_coverage_enabled) glEnable(GL_SAMPLE_ALPHA_TO_COVERAGE);
            else glDisable(GL_SAMPLE_ALPHA_TO_COVERAGE);
        }
        #ifdef SOKOL_GLCORE33
        if (new_r->sample_count != cache_r->sample_count) {
            cache_r->sample_count = new_r->sample_count;
            if (new_r->sample_count > 1) glEnable(GL_MULTISAMPLE);
            else glDisable(GL_MULTISAMPLE);
        }
        #endif
        if (!_sg_fequal(new_r->depth_bias, cache_r->depth_bias, 0.000001f) ||
            !_sg_fequal(new_r->depth_bias_slope_scale, cache_r->depth_bias_slope_scale, 0.000001f))
        {
            /* according to ANGLE's D3D11 backend:
                D3D11 SlopeScaledDepthBias ==> GL polygonOffsetFactor
                D3D11 DepthBias ==> GL polygonOffsetUnits
                DepthBiasClamp has no meaning on GL
            */
            cache_r->depth_bias = new_r->depth_bias;
            cache_r->depth_bias_slope_scale = new_r->depth_bias_slope_scale;
            glPolygonOffset(new_r->depth_bias_slope_scale, new_r->depth_bias);
            bool po_enabled = true;
            if (_sg_fequal(new_r->depth_bias, 0.0f, 0.000001f) &&
                _sg_fequal(new_r->depth_bias_slope_scale, 0.0f, 0.000001f))
            {
                po_enabled = false;
            }
            if (po_enabled != _sg_gl.cache.polygon_offset_enabled) {
                _sg_gl.cache.polygon_offset_enabled = po_enabled;
                if (po_enabled) glEnable(GL_POLYGON_OFFSET_FILL);
                else glDisable(GL_POLYGON_OFFSET_FILL);
            }
        }

        /* bind shader program */
        glUseProgram(pip->shader->gl_prog);
    }

    /* bind textures */
    _SG_GL_CHECK_ERROR();
    for (int stage_index = 0; stage_index < SG_NUM_SHADER_STAGES; stage_index++) {
        const _sg_shader_stage* stage = &pip->shader->stage[stage_index];
        _sg_image** imgs = (stage_index == SG_SHADERSTAGE_VS)? vs_imgs : fs_imgs;
        SOKOL_ASSERT(((stage_index == SG_SHADERSTAGE_VS)? num_vs_imgs : num_fs_imgs) == stage->num_images);
        for (int img_index = 0; img_index < stage->num_images; img_index++) {
            const _sg_shader_image* shd_img = &stage->images[img_index];
            if (shd_img->gl_loc != -1) {
                _sg_image* img = imgs[img_index];
                const GLuint gl_tex = img->gl_tex[img->active_slot];
                SOKOL_ASSERT(img && img->gl_target);
                SOKOL_ASSERT((shd_img->gl_tex_slot != -1) && gl_tex);
                glUniform1i(shd_img->gl_loc, shd_img->gl_tex_slot);
                glActiveTexture(GL_TEXTURE0+shd_img->gl_tex_slot);
                glBindTexture(img->gl_target, gl_tex);
            }
        }
    }
    _SG_GL_CHECK_ERROR();

    /* index buffer (can be 0) */
    const GLuint gl_ib = ib ? ib->gl_buf[ib->active_slot] : 0;
    if (gl_ib != _sg_gl.cache.cur_gl_ib) {
        _sg_gl.cache.cur_gl_ib = gl_ib;
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, gl_ib);
    }

    /* vertex attributes */
    GLuint gl_vb = 0;
    for (int attr_index = 0; attr_index < SG_MAX_VERTEX_ATTRIBUTES; attr_index++) {
        _sg_gl_attr* attr = &pip->gl_attrs[attr_index];
        _sg_gl_cache_attr* cache_attr = &_sg_gl.cache.attrs[attr_index];
        bool cache_attr_dirty = false;
        if (attr->vb_index >= 0) {
            /* attribute is enabled */
            SOKOL_ASSERT(attr->vb_index < num_vbs);
            _sg_buffer* vb = vbs[attr->vb_index];
            SOKOL_ASSERT(vb);
            if ((vb->gl_buf[vb->active_slot] != cache_attr->gl_vbuf) ||
                (attr->size != cache_attr->gl_attr.size) ||
                (attr->type != cache_attr->gl_attr.type) ||
                (attr->normalized != cache_attr->gl_attr.normalized) ||
                (attr->stride != cache_attr->gl_attr.stride) ||
                (attr->offset != cache_attr->gl_attr.offset))
            {
                if (gl_vb != vb->gl_buf[vb->active_slot]) {
                    gl_vb = vb->gl_buf[vb->active_slot];
                    glBindBuffer(GL_ARRAY_BUFFER, gl_vb);
                }
                glVertexAttribPointer(attr_index, attr->size, attr->type, 
                    attr->normalized, attr->stride, 
                    (const GLvoid*)(GLintptr)attr->offset);
                cache_attr_dirty = true;
            }
            if (cache_attr->gl_attr.vb_index == -1) {
                glEnableVertexAttribArray(attr_index);
                cache_attr_dirty = true;
            }
            if (_sg_gl.features[SG_FEATURE_INSTANCING]) {
                if (cache_attr->gl_attr.divisor != attr->divisor) {
                    glVertexAttribDivisor(attr_index, attr->divisor);
                    cache_attr_dirty = true;
                }
            }
        }
        else {
            /* attribute is disabled */
            if (cache_attr->gl_attr.vb_index != -1) {
                glDisableVertexAttribArray(attr_index);
                cache_attr_dirty = true;
            }
        }
        if (cache_attr_dirty) {
            cache_attr->gl_attr = *attr;
            cache_attr->gl_vbuf = gl_vb;
        }
    }
    _SG_GL_CHECK_ERROR();
}

_SOKOL_PRIVATE void _sg_apply_uniform_block(sg_shader_stage stage_index, int ub_index, const void* data, int num_bytes) {
    SOKOL_ASSERT(data && (num_bytes > 0));
    SOKOL_ASSERT((stage_index >= 0) && ((int)stage_index < SG_NUM_SHADER_STAGES));
    SOKOL_ASSERT(_sg_gl.cache.cur_pipeline);
    SOKOL_ASSERT(_sg_gl.cache.cur_pipeline->slot.id == _sg_gl.cache.cur_pipeline_id.id);
    SOKOL_ASSERT(_sg_gl.cache.cur_pipeline->shader->slot.id == _sg_gl.cache.cur_pipeline->shader_id.id);
    _sg_shader_stage* stage = &_sg_gl.cache.cur_pipeline->shader->stage[stage_index];
    SOKOL_ASSERT(ub_index < stage->num_uniform_blocks);
    _sg_uniform_block* ub = &stage->uniform_blocks[ub_index];
    SOKOL_ASSERT(ub->size == num_bytes);
    for (int u_index = 0; u_index < ub->num_uniforms; u_index++) {
        _sg_uniform* u = &ub->uniforms[u_index];
        SOKOL_ASSERT(u->type != SG_UNIFORMTYPE_INVALID);
        if (u->gl_loc == -1) {
            continue;
        }
        GLfloat* ptr = (GLfloat*) (((uint8_t*)data) + u->offset);
        switch (u->type) {
            case SG_UNIFORMTYPE_INVALID:
                break;
            case SG_UNIFORMTYPE_FLOAT:
                glUniform1fv(u->gl_loc, u->count, ptr);
                break;
            case SG_UNIFORMTYPE_FLOAT2:
                glUniform2fv(u->gl_loc, u->count, ptr);
                break;
            case SG_UNIFORMTYPE_FLOAT3:
                glUniform3fv(u->gl_loc, u->count, ptr);
                break;
            case SG_UNIFORMTYPE_FLOAT4:
                glUniform4fv(u->gl_loc, u->count, ptr);
                break;
            case SG_UNIFORMTYPE_MAT4:
                glUniformMatrix4fv(u->gl_loc, u->count, GL_FALSE, ptr);
                break;
            default:
                SOKOL_UNREACHABLE;
                break;
        }
    }
}

_SOKOL_PRIVATE void _sg_draw(int base_element, int num_elements, int num_instances) {
    const GLenum i_type = _sg_gl.cache.cur_index_type;
    const GLenum p_type = _sg_gl.cache.cur_primitive_type;
    if (0 != i_type) {
        /* indexed rendering */
        const int i_size = (i_type == GL_UNSIGNED_SHORT) ? 2 : 4;
        const GLvoid* indices = (const GLvoid*)(GLintptr)(base_element*i_size);
        if (num_instances == 1) {
            glDrawElements(p_type, num_elements, i_type, indices);
        }
        else {
            if (_sg_gl.features[SG_FEATURE_INSTANCING]) {
                glDrawElementsInstanced(p_type, num_elements, i_type, indices, num_instances);
            }
        }
    }
    else {
        /* non-indexed rendering */
        if (num_instances == 1) {
            glDrawArrays(p_type, base_element, num_elements);
        }
        else {
            if (_sg_gl.features[SG_FEATURE_INSTANCING]) {
                glDrawArraysInstanced(p_type, base_element, num_elements, num_instances);
            }
        }
    }
}

_SOKOL_PRIVATE void _sg_commit() {
    SOKOL_ASSERT(!_sg_gl.in_pass);
}

_SOKOL_PRIVATE void _sg_update_buffer(_sg_buffer* buf, const void* data_ptr, int data_size) {
    SOKOL_ASSERT(buf && data_ptr && (data_size > 0));
    /* only one update per buffer per frame allowed */
    if (++buf->active_slot >= buf->num_slots) {
        buf->active_slot = 0;
    }
    GLenum gl_tgt = _sg_gl_buffer_target(buf->type);
    SOKOL_ASSERT(buf->active_slot < SG_NUM_INFLIGHT_FRAMES);
    GLuint gl_buf = buf->gl_buf[buf->active_slot];
    SOKOL_ASSERT(gl_buf);
    _SG_GL_CHECK_ERROR();
    glBindBuffer(gl_tgt, gl_buf);
    glBufferSubData(gl_tgt, 0, data_size, data_ptr);
    _SG_GL_CHECK_ERROR();
}

_SOKOL_PRIVATE void _sg_update_image(_sg_image* img, const sg_image_content* data) {
    SOKOL_ASSERT(img && data);
    /* only one update per image per frame allowed */
    if (++img->active_slot >= img->num_slots) {
        img->active_slot = 0;
    }
    SOKOL_ASSERT(img->active_slot < SG_NUM_INFLIGHT_FRAMES);
    SOKOL_ASSERT(0 != img->gl_tex[img->active_slot]);
    glBindTexture(img->gl_target, img->gl_tex[img->active_slot]);
    const GLenum gl_img_format = _sg_gl_teximage_format(img->pixel_format);
    const GLenum gl_img_type = _sg_gl_teximage_type(img->pixel_format);
    const int num_faces = img->type == SG_IMAGETYPE_CUBE ? 6 : 1;
    const int num_mips = img->num_mipmaps;
    for (int face_index = 0; face_index < num_faces; face_index++) {
        for (int mip_index = 0; mip_index < num_mips; mip_index++) {
            GLenum gl_img_target = img->gl_target;
            if (SG_IMAGETYPE_CUBE == img->type) {
                gl_img_target = _sg_gl_cubeface_target(face_index);
            }
            const GLvoid* data_ptr = data->subimage[face_index][mip_index].ptr;
            int mip_width = img->width >> mip_index;
            if (mip_width == 0) {
                mip_width = 1;
            }
            int mip_height = img->height >> mip_index;
            if (mip_height == 0) {
                mip_height = 1;
            }
            if ((SG_IMAGETYPE_2D == img->type) || (SG_IMAGETYPE_CUBE == img->type)) {
                glTexSubImage2D(gl_img_target, mip_index,
                    0, 0,
                    mip_width, mip_height,
                    gl_img_format, gl_img_type,
                    data_ptr);
            }
            #if !defined(SOKOL_GLES2)
            else if (!_sg_gl_gles2 && ((SG_IMAGETYPE_3D == img->type) || (SG_IMAGETYPE_ARRAY == img->type))) {
                int mip_depth = img->depth >> mip_index;
                if (mip_depth == 0) {
                    mip_depth = 1;
                }
                glTexSubImage3D(gl_img_target, mip_index,
                    0, 0, 0,
                    mip_width, mip_height, mip_depth,
                    gl_img_format, gl_img_type,
                    data_ptr);

            }
            #endif
        }
    }
}

_SOKOL_PRIVATE void _sg_reset_state_cache() {
    #if !defined(SOKOL_GLES2)
    if (!_sg_gl_gles2) {
        glBindVertexArray(_sg_gl.vao);
    }
    #endif
    _sg_gl_reset_state_cache(&_sg_gl.cache);
}

#ifdef __cplusplus
} /* extern "C" */
#endif
