/*
    Sokol Gfx GL rendering backend
*/
#ifndef SOKOL_IMPL_GUARD
#error "Please do not include *.impl.h files directly"
#endif

/* strstr() */
#include <string.h>

enum {
    _SG_GL_NUM_UPDATE_SLOTS = 2,
};

#ifndef GL_UNSIGNED_INT_2_10_10_10_REV
#define GL_UNSIGNED_INT_2_10_10_10_REV 0x8368
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

#define _SG_GL_CHECK_ERROR() { SOKOL_ASSERT(glGetError() == GL_NO_ERROR); } 

/*-- type translation --------------------------------------------------------*/
static GLenum _sg_gl_buffer_target(sg_buffer_type t) {
    switch (t) {
        case SG_BUFFERTYPE_VERTEXBUFFER:    return GL_ARRAY_BUFFER;
        case SG_BUFFERTYPE_INDEXBUFFER:     return GL_ELEMENT_ARRAY_BUFFER;
    }
}

static GLenum _sg_gl_texture_target(sg_image_type t) {
    switch (t) {
        case SG_IMAGETYPE_2D:   return GL_TEXTURE_2D;
        case SG_IMAGETYPE_CUBE: return GL_TEXTURE_CUBE_MAP;
        #if !defined(SOKOL_USE_GLES2)
        case SG_IMAGETYPE_3D:       return GL_TEXTURE_3D;
        case SG_IMAGETYPE_ARRAY:    return GL_TEXTURE_2D_ARRAY;
        #endif
        default: return 0;
    }
}

static GLenum _sg_gl_usage(sg_usage u) {
    switch (u) {
        case SG_USAGE_IMMUTABLE:    return GL_STATIC_DRAW;
        case SG_USAGE_DYNAMIC:      return GL_DYNAMIC_DRAW;
        case SG_USAGE_STREAM:       return GL_STREAM_DRAW;
    }
}

static GLenum _sg_gl_shader_stage(sg_shader_stage stage) {
    switch (stage) {
        case SG_SHADERSTAGE_VS:     return GL_VERTEX_SHADER;
        case SG_SHADERSTAGE_FS:     return GL_FRAGMENT_SHADER;
    }
}

static GLint _sg_gl_vertexformat_size(sg_vertex_format fmt) {
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
        default:    return 0;
    }
}

static GLenum _sg_gl_vertexformat_type(sg_vertex_format fmt) {
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
            return 0;
    }
}

static GLboolean _sg_gl_vertexformat_normalized(sg_vertex_format fmt) {
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

static GLenum _sg_gl_primitive_type(sg_primitive_type t) {
    switch (t) {
        case SG_PRIMITIVETYPE_POINTS:           return GL_POINTS;
        case SG_PRIMITIVETYPE_LINES:            return GL_LINES;
        case SG_PRIMITIVETYPE_LINE_STRIP:       return GL_LINE_STRIP;
        case SG_PRIMITIVETYPE_TRIANGLES:        return GL_TRIANGLES;
        case SG_PRIMITIVETYPE_TRIANLE_STRIP:    return GL_TRIANGLE_STRIP;
    }
}

static GLenum _sg_gl_index_type(sg_index_type t) {
    switch (t) {
        case SG_INDEXTYPE_NONE:     return 0;
        case SG_INDEXTYPE_UINT16:   return GL_UNSIGNED_SHORT;
        case SG_INDEXTYPE_UINT32:   return GL_UNSIGNED_INT;
    }
}

static GLenum _sg_gl_compare_func(sg_compare_func cmp) {
    switch (cmp) {
        case SG_COMPAREFUNC_NEVER:          return GL_NEVER;
        case SG_COMPAREFUNC_LESS:           return GL_LESS;
        case SG_COMPAREFUNC_EQUAL:          return GL_EQUAL;
        case SG_COMPAREFUNC_LESS_EQUAL:     return GL_LEQUAL;
        case SG_COMPAREFUNC_GREATER:        return GL_GREATER;
        case SG_COMPAREFUNC_NOT_EQUAL:      return GL_NOTEQUAL;
        case SG_COMPAREFUNC_GREATER_EQUAL:  return GL_GEQUAL;
        case SG_COMPAREFUNC_ALWAYS:         return GL_ALWAYS;
    }
}

static GLenum _sg_gl_stencil_op(sg_stencil_op op) {
    switch (op) {
        case SG_STENCILOP_KEEP:         return GL_KEEP;
        case SG_STENCILOP_ZERO:         return GL_ZERO;
        case SG_STENCILOP_REPLACE:      return GL_REPLACE;
        case SG_STENCILOP_INCR_CLAMP:   return GL_INCR;
        case SG_STENCILOP_DECR_CLAMP:   return GL_DECR;
        case SG_STENCILOP_INVERT:       return GL_INVERT;
        case SG_STENCILOP_INCR_WRAP:    return GL_INCR_WRAP;
        case SG_STENCILOP_DECR_WRAP:    return GL_DECR_WRAP;
    }
}

static GLenum _sg_gl_blend_factor(sg_blend_factor f) {
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
    }
}

static GLenum _sg_gl_blend_op(sg_blend_op op) {
    switch (op) {
        case SG_BLENDOP_ADD:                return GL_FUNC_ADD;
        case SG_BLENDOP_SUBTRACT:           return GL_FUNC_SUBTRACT;
        case SG_BLENDOP_REVERSE_SUBTRACT:   return GL_FUNC_REVERSE_SUBTRACT;
    }
}

static GLenum _sg_gl_cull_face(sg_face f) {
    switch (f) {
        case SG_FACE_FRONT: return GL_FRONT;
        case SG_FACE_BACK:  return GL_BACK;
        case SG_FACE_BOTH:  return GL_FRONT_AND_BACK;
    }
}

static GLenum _sg_gl_filter(sg_filter f) {
    switch (f) {
        case SG_FILTER_NEAREST:                 return GL_NEAREST;
        case SG_FILTER_LINEAR:                  return GL_LINEAR;
        case SG_FILTER_NEAREST_MIPMAP_NEAREST:  return GL_NEAREST_MIPMAP_NEAREST;
        case SG_FILTER_NEAREST_MIPMAP_LINEAR:   return GL_NEAREST_MIPMAP_LINEAR;
        case SG_FILTER_LINEAR_MIPMAP_NEAREST:   return GL_LINEAR_MIPMAP_NEAREST;
        case SG_FILTER_LINEAR_MIPMAP_LINEAR:    return GL_LINEAR_MIPMAP_LINEAR;
    }
}

static GLenum _sg_gl_wrap(sg_wrap w) {
    switch (w) {
        case SG_WRAP_CLAMP_TO_EDGE:     return GL_CLAMP_TO_EDGE;
        case SG_WRAP_REPEAT:            return GL_REPEAT;
        case SG_WRAP_MIRRORED_REPEAT:   return GL_MIRRORED_REPEAT;
    }
}

static GLenum _sg_gl_teximage_type(sg_pixel_format fmt) {
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
            SOKOL_LOG("_sg_gl_teximage_type(): invalid pixel format!\n");
            return 0;
    }
}

static GLenum _sg_gl_teximage_format(sg_pixel_format fmt) {
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
            #if defined(SOKOL_USE_GLES2)
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
    }
}

static GLenum _sg_gl_teximage_internal_format(sg_pixel_format fmt) {
    #if defined(SOKOL_USE_GLES2)
    return _sg_gl_teximage_format(fmt);
    #else
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
            #if defined(SOKOL_USE_GLES3)
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
            /* FIXME */
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
    }
    #endif
}

static GLenum _sg_gl_cubeface_target(int face_index) {
    switch (face_index) {
        case 0: return GL_TEXTURE_CUBE_MAP_POSITIVE_X;
        case 1: return GL_TEXTURE_CUBE_MAP_NEGATIVE_X;
        case 2: return GL_TEXTURE_CUBE_MAP_POSITIVE_Y;
        case 3: return GL_TEXTURE_CUBE_MAP_NEGATIVE_Y;
        case 4: return GL_TEXTURE_CUBE_MAP_POSITIVE_Z;
        default: return GL_TEXTURE_CUBE_MAP_NEGATIVE_Z;
    }
}

static GLenum _sg_gl_depth_attachment_format(sg_pixel_format fmt) {
    switch (fmt) {
        case SG_PIXELFORMAT_DEPTH:          return GL_DEPTH_COMPONENT16; /* FIXME */
        case SG_PIXELFORMAT_DEPTHSTENCIL:   return GL_DEPTH24_STENCIL8;  /* FIXME */
        default:    return 0;
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
    GLuint gl_buf[_SG_GL_NUM_UPDATE_SLOTS];
} _sg_buffer;

static void _sg_init_buffer(_sg_buffer* buf) {
    SOKOL_ASSERT(buf);
    _sg_init_slot(&buf->slot);
    buf->size = 0;
    buf->type = SG_BUFFERTYPE_VERTEXBUFFER;
    buf->usage = SG_USAGE_IMMUTABLE;
    buf->upd_frame_index = 0;
    buf->num_slots = 0;
    buf->active_slot = 0;
    for (int i = 0; i < _SG_GL_NUM_UPDATE_SLOTS; i++) {
        buf->gl_buf[i] = 0;
    }
}

typedef struct {
    _sg_slot slot;
    sg_image_type type;
    bool render_target;
    uint16_t width;
    uint16_t height;
    uint16_t depth;
    uint16_t num_mipmaps;
    sg_usage usage;
    sg_pixel_format color_format;
    sg_pixel_format depth_format;
    int sample_count;
    sg_filter min_filter;
    sg_filter mag_filter;
    sg_wrap wrap_u;
    sg_wrap wrap_v;
    sg_wrap wrap_w;
    GLenum gl_target;
    GLuint gl_depth_render_buffer;
    GLuint gl_msaa_render_buffer;
    uint32_t upd_frame_index;
    int num_slots;
    int active_slot;
    GLuint gl_tex[_SG_GL_NUM_UPDATE_SLOTS];
} _sg_image;

static void _sg_init_image(_sg_image* img) {
    SOKOL_ASSERT(img);
    _sg_init_slot(&img->slot);
    img->type = SG_IMAGETYPE_INVALID;
    img->render_target = false;
    img->width = 0;
    img->height = 0;
    img->depth = 0;
    img->num_mipmaps = 0;
    img->usage = SG_USAGE_IMMUTABLE;
    img->color_format = SG_PIXELFORMAT_NONE;
    img->depth_format = SG_PIXELFORMAT_NONE;
    img->sample_count = 0;
    img->min_filter = SG_FILTER_NEAREST;
    img->mag_filter = SG_FILTER_NEAREST;
    img->wrap_u = SG_WRAP_REPEAT;
    img->wrap_v = SG_WRAP_REPEAT;
    img->wrap_w = SG_WRAP_REPEAT;
    img->gl_target = 0;
    img->gl_depth_render_buffer = 0;
    img->gl_msaa_render_buffer = 0;
    img->upd_frame_index = 0;
    img->num_slots = 0;
    img->active_slot = 0;
    for (int i = 0; i < _SG_GL_NUM_UPDATE_SLOTS; i++) {
        img->gl_tex[i] = 0;
    }
}

typedef struct {
    GLint gl_loc;
    sg_uniform_type type;
    uint8_t count;
    uint16_t offset;
} _sg_uniform;

typedef struct {
    uint16_t size;
    uint16_t num_uniforms;
    _sg_uniform uniforms[SG_MAX_UNIFORMS];
} _sg_uniform_block;

typedef struct {
    sg_image_type type;
    GLint gl_loc;
    int gl_tex_slot;
} _sg_shader_image;

typedef struct {
    uint16_t num_uniform_blocks;
    uint16_t num_images;
    _sg_uniform_block uniform_blocks[SG_MAX_SHADERSTAGE_UBS];
    _sg_shader_image images[SG_MAX_SHADERSTAGE_IMAGES];
} _sg_shader_stage;

typedef struct {
    _sg_slot slot;
    GLuint gl_prog;
    _sg_shader_stage stage[SG_NUM_SHADER_STAGES];
} _sg_shader;

static void _sg_init_shader(_sg_shader* shd) {
    SOKOL_ASSERT(shd);
    _sg_init_slot(&shd->slot);
    shd->gl_prog = 0;
    for (int stage_index = 0; stage_index < SG_NUM_SHADER_STAGES; stage_index++) {
        _sg_shader_stage* stage = &shd->stage[stage_index];
        stage->num_uniform_blocks = 0;
        stage->num_images = 0;
        for (int ub_index = 0; ub_index < SG_MAX_SHADERSTAGE_UBS; ub_index++) {
            _sg_uniform_block* ub = &stage->uniform_blocks[ub_index];
            ub->size = 0;
            ub->num_uniforms = 0;
            for (int u_index = 0; u_index < SG_MAX_UNIFORMS; u_index++) {
                _sg_uniform* u = &ub->uniforms[u_index];
                u->gl_loc = 0;
                u->type = SG_UNIFORMTYPE_INVALID;
                u->offset = 0;
                u->count = 0;
            }
        }
        for (int img_index = 0; img_index < SG_MAX_SHADERSTAGE_IMAGES; img_index++) {
            _sg_shader_image* img = &stage->images[img_index];
            img->type = SG_IMAGETYPE_INVALID;
            img->gl_loc = -1;
            img->gl_tex_slot = -1;
        }
    }
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

static void _sg_init_gl_attr(_sg_gl_attr* attr) {
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
    sg_id shader_id;
    sg_primitive_type primitive_type;
    sg_index_type index_type;
    _sg_gl_attr gl_attrs[SG_MAX_VERTEX_ATTRIBUTES];
    sg_depth_stencil_state depth_stencil;
    sg_blend_state blend;
    sg_rasterizer_state rast;
} _sg_pipeline;

static void _sg_init_pipeline(_sg_pipeline* pip) {
    SOKOL_ASSERT(pip);
    _sg_init_slot(&pip->slot);
    pip->shader = 0;
    pip->shader_id = SG_INVALID_ID;
    pip->primitive_type = SG_PRIMITIVETYPE_TRIANGLES;
    pip->index_type = SG_INDEXTYPE_NONE;
    for (int i = 0; i < SG_MAX_VERTEX_ATTRIBUTES; i++) {
        _sg_init_gl_attr(&pip->gl_attrs[i]);
    }
    _sg_init_depth_stencil_state(&pip->depth_stencil);
    _sg_init_blend_state(&pip->blend);
    _sg_init_rasterizer_state(&pip->rast);
}

typedef struct {
    _sg_image* image;
    sg_id image_id;
    int mip_level;
    int slice;
    GLuint gl_msaa_resolve_buffer;
} _sg_attachment;

static void _sg_init_attachment(_sg_attachment* att) {
    SOKOL_ASSERT(att);
    att->image = 0;
    att->image_id = SG_INVALID_ID;
    att->mip_level = 0;
    att->slice = 0;
    att->gl_msaa_resolve_buffer = 0;
}

typedef struct {
    _sg_slot slot;
    GLuint gl_fb;
    _sg_attachment color_atts[SG_MAX_COLOR_ATTACHMENTS];
    _sg_attachment ds_att;
} _sg_pass;

static void _sg_init_pass(_sg_pass* pass) {
    SOKOL_ASSERT(pass);
    _sg_init_slot(&pass->slot);
    pass->gl_fb = 0;
    for (int i = 0; i < SG_MAX_COLOR_ATTACHMENTS; i++) {
        _sg_init_attachment(&pass->color_atts[i]);
    }
    _sg_init_attachment(&pass->ds_att);
}

/*-- state cache implementation ----------------------------------------------*/
typedef struct {
    sg_depth_stencil_state ds;
    sg_blend_state blend;
    sg_rasterizer_state rast;
    _sg_gl_attr attrs[SG_MAX_VERTEX_ATTRIBUTES];
} _sg_state_cache;

static void _sg_init_state_cache(_sg_state_cache* state) {
    SOKOL_ASSERT(state);
    
    for (int i = 0; i < SG_MAX_VERTEX_ATTRIBUTES; i++) {
        _sg_init_gl_attr(&state->attrs[i]);
        glDisableVertexAttribArray(i);
    }

    /* depth-stencil state */
    _sg_init_depth_stencil_state(&state->ds);
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_ALWAYS);
    glDepthMask(GL_FALSE);
    glDisable(GL_STENCIL_TEST);
    glStencilFunc(GL_ALWAYS, 0, 0xFFFFFFFF);
    glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);
    glStencilMask(0xFFFFFFFF);

    /* blend state */
    _sg_init_blend_state(&state->blend);
    glDisable(GL_BLEND);
    glBlendFuncSeparate(GL_ONE, GL_ZERO, GL_ONE, GL_ZERO);
    glBlendEquationSeparate(GL_FUNC_ADD, GL_FUNC_ADD);
    glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
    glBlendColor(1.0f, 1.0f, 1.0f, 1.0f);

    /* rasterizer state */
    _sg_init_rasterizer_state(&state->rast);
    glDisable(GL_CULL_FACE);
    glFrontFace(GL_CW);
    glCullFace(GL_BACK);
    glDisable(GL_POLYGON_OFFSET_FILL);
    glDisable(GL_SCISSOR_TEST);
    glEnable(GL_DITHER);
    #if defined(SOKOL_USE_GLCORE33)
        glEnable(GL_MULTISAMPLE);
    #endif
}

/*-- main GL backend state and functions -------------------------------------*/
typedef struct {
    bool valid;
    bool in_pass;
    bool next_draw_valid;
    uint32_t frame_index;
    GLenum cur_primitive_type;
    GLenum cur_index_type;
    int cur_pass_width;
    int cur_pass_height;
    _sg_pipeline* cur_pipeline;
    sg_id cur_pipeline_id; 
    _sg_state_cache cache;
    bool features[SG_NUM_FEATURES];
    #if !defined(SOKOL_USE_GLES2)
    GLuint vao; 
    #endif
} _sg_backend;

static void _sg_setup_backend(_sg_backend* state) {
    SOKOL_ASSERT(state);
    #if !defined(SOKOL_USE_GLES2)
    glGenVertexArrays(1, &state->vao);
    glBindVertexArray(state->vao);
    #endif
    state->in_pass = false;
    state->next_draw_valid = false;
    state->frame_index = 1;
    state->cur_primitive_type = GL_TRIANGLES;
    state->cur_index_type = 0;
    state->cur_pass_width = 0;
    state->cur_pass_height = 0;
    state->cur_pipeline = 0;
    state->cur_pipeline_id = SG_INVALID_ID;
    state->valid = true;
    _sg_init_state_cache(&state->cache);
    
    /* initialize feature flags */
    for (int i = 0; i < SG_NUM_FEATURES; i++) {
        state->features[i] = false;
    }
    state->features[SG_FEATURE_ORIGIN_BOTTOM_LEFT] = true;
    #if !defined(SOKOL_USE_GLCORE33)
        const char* ext = (const char*) glGetString(GL_EXTENSIONS);
        state->features[SG_FEATURE_TEXTURE_COMPRESSION_DXT] =
            strstr(ext, "_texture_compression_s3tc") ||
            strstr(ext, "_compressed_texture_s3tc") ||
            strstr(ext, "texture_compression_dxt1");
        state->features[SG_FEATURE_TEXTURE_COMPRESSION_PVRTC] =
            strstr(ext, "_texture_compression_pvrtc") ||
            strstr(ext, "_compressed_texture_pvrtc");
        state->features[SG_FEATURE_TEXTURE_COMPRESSION_ATC] = strstr(ext, "_compressed_texture_atc");
        state->features[SG_FEATURE_TEXTURE_FLOAT] = strstr(ext, "_texture_float");
        state->features[SG_FEATURE_INSTANCED_ARRAYS] = strstr(ext, "_instanced_arrays");
        #if defined(SOKOL_USE_GLES2)
            state->features[SG_FEATURE_TEXTURE_HALF_FLOAT] = strstr(ext, "_texture_half_float");
        #else
            state->features[SG_FEATURE_TEXTURE_HALF_FLOAT] = state->features[SG_FEATURE_TEXTURE_FLOAT];
        #endif
    #endif
    #if defined(SOKOL_USE_GLCORE33) || defined(SOKOL_USE_GLES3)
        #if defined(SOKOL_USE_GLCORE33)
        state->features[SG_FEATURE_TEXTURE_COMPRESSION_DXT] = true;
        #endif
        state->features[SG_FEATURE_INSTANCED_ARRAYS] = true;
        state->features[SG_FEATURE_TEXTURE_FLOAT] = true;
        state->features[SG_FEATURE_TEXTURE_HALF_FLOAT] = true;
        state->features[SG_FEATURE_MSAA_RENDER_TARGETS] = true;
        state->features[SG_FEATURE_PACKED_VERTEX_FORMAT_10_2] = true;
        state->features[SG_FEATURE_MULTIPLE_RENDER_TARGET] = true;
        state->features[SG_FEATURE_TEXTURE_3D] = true;
        state->features[SG_FEATURE_TEXTURE_ARRAY] = true;
    #endif
}

static void _sg_discard_backend(_sg_backend* state) {
    SOKOL_ASSERT(state);
    SOKOL_ASSERT(state->valid);
    #if !defined(SOKOL_USE_GLES2)
    glDeleteVertexArrays(1, &state->vao);
    state->vao = 0;
    #endif
    state->valid = false;
}

static bool _sg_query_feature(_sg_backend* state, sg_feature f) {
    SOKOL_ASSERT(state && (f>=0) && (f<SG_NUM_FEATURES));
    return state->features[f];
}

/*-- GL backend resource creation and destruction ----------------------------*/
static void _sg_create_buffer(_sg_backend* state, _sg_buffer* buf, const sg_buffer_desc* desc) {
    SOKOL_ASSERT(state && buf && desc);
    SOKOL_ASSERT(buf->slot.state == SG_RESOURCESTATE_ALLOC);
    SOKOL_ASSERT(desc->data_size <= desc->size);
    _SG_GL_CHECK_ERROR();
    buf->size = desc->size;
    buf->type = desc->type;
    buf->usage = desc->usage;
    buf->num_slots = desc->usage == SG_USAGE_STREAM ? _SG_GL_NUM_UPDATE_SLOTS : 1;
    buf->active_slot = 0;
    GLenum gl_target = _sg_gl_buffer_target(buf->type);
    GLenum gl_usage  = _sg_gl_usage(buf->usage);
    for (int slot = 0; slot < buf->num_slots; slot++) {
        GLuint gl_buf;
        glGenBuffers(1, &gl_buf);
        glBindBuffer(gl_target, gl_buf);
        glBufferData(gl_target, buf->size, 0, gl_usage);
        if (desc->data_ptr) {
            glBufferSubData(gl_target, 0, desc->data_size, desc->data_ptr);
        }
        buf->gl_buf[slot] = gl_buf;
    }
    _SG_GL_CHECK_ERROR();
    buf->slot.state = SG_RESOURCESTATE_VALID;
}

static void _sg_destroy_buffer(_sg_backend* state, _sg_buffer* buf) {
    SOKOL_ASSERT(state && buf);
    _SG_GL_CHECK_ERROR();
    for (int slot = 0; slot < buf->num_slots; slot++) {
        if (buf->gl_buf[slot]) {
            glDeleteBuffers(1, &buf->gl_buf[slot]);
        }
    }
    _SG_GL_CHECK_ERROR();
    _sg_init_buffer(buf);
}

static bool _sg_gl_valid_texture_format(_sg_backend* state, sg_pixel_format fmt) {
    SOKOL_ASSERT(state);
    switch (fmt) {
        case SG_PIXELFORMAT_DXT1:
        case SG_PIXELFORMAT_DXT3:
        case SG_PIXELFORMAT_DXT5:
            return state->features[SG_FEATURE_TEXTURE_COMPRESSION_DXT];
        case SG_PIXELFORMAT_PVRTC2_RGB:
        case SG_PIXELFORMAT_PVRTC4_RGB:
        case SG_PIXELFORMAT_PVRTC2_RGBA:
        case SG_PIXELFORMAT_PVRTC4_RGBA:
            return state->features[SG_FEATURE_TEXTURE_COMPRESSION_PVRTC];
        case SG_PIXELFORMAT_ETC2_RGB8:
        case SG_PIXELFORMAT_ETC2_SRGB8:
            return state->features[SG_FEATURE_TEXTURE_COMPRESSION_ETC2];
        default:
            return true;
    }
}

static void _sg_create_image(_sg_backend* state, _sg_image* img, const sg_image_desc* desc) {
    SOKOL_ASSERT(state && img && desc);
    SOKOL_ASSERT(img->slot.state == SG_RESOURCESTATE_ALLOC);
    _SG_GL_CHECK_ERROR();
    img->type = desc->type;
    img->render_target = desc->render_target;
    img->width = desc->width;
    img->height = desc->height;
    img->depth = desc->depth;
    img->num_mipmaps = desc->num_mipmaps;
    img->usage = desc->usage;
    img->color_format = desc->color_format;
    img->depth_format = desc->depth_format;
    img->sample_count = desc->sample_count;
    img->min_filter = desc->min_filter;
    img->mag_filter = desc->mag_filter;
    img->wrap_u = desc->wrap_u;
    img->wrap_v = desc->wrap_v;
    img->wrap_w = desc->wrap_w;

    /* check if texture format is support */
    if (!_sg_gl_valid_texture_format(state, img->color_format)) {
        SOKOL_LOG("compressed texture format not supported by GL context\n");
        img->slot.state = SG_RESOURCESTATE_FAILED;
        return;
    }
    /* check for optional texture types */
    if ((img->type == SG_IMAGETYPE_3D) && !state->features[SG_FEATURE_TEXTURE_3D]) {
        SOKOL_LOG("3D textures not supported by GL context\n");
        img->slot.state = SG_RESOURCESTATE_FAILED;
        return;
    }
    if ((img->type == SG_IMAGETYPE_ARRAY) && !state->features[SG_FEATURE_TEXTURE_ARRAY]) {
        SOKOL_LOG("array textures not supported by GL context\n");
        img->slot.state = SG_RESOURCESTATE_FAILED;
        return;
    }
    if ((img->depth_format != SG_PIXELFORMAT_NONE) && !_sg_is_valid_rendertarget_depth_format(img->depth_format)) {
        SOKOL_LOG("depth_format is not a valid render target depth format!\n");
        img->slot.state = SG_RESOURCESTATE_FAILED;
        return;
    }

    /* create 1 or 2 GL textures, depending on requested update strategy */
    img->num_slots = img->usage == SG_USAGE_STREAM ? _SG_GL_NUM_UPDATE_SLOTS : 1;
    img->active_slot = 0;

    /* create the GL color texture(s) */
    img->gl_target = _sg_gl_texture_target(img->type);
    const GLenum gl_internal_format = _sg_gl_teximage_internal_format(img->color_format);
    const GLenum gl_format = _sg_gl_teximage_format(img->color_format);
    const bool is_compressed = _sg_is_compressed_pixel_format(img->color_format);
    for (int slot = 0; slot < img->num_slots; slot++) {
        glGenTextures(1, &img->gl_tex[slot]);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(img->gl_target, img->gl_tex[slot]);
        GLenum gl_min_filter = _sg_gl_filter(desc->min_filter);
        GLenum gl_mag_filter = _sg_gl_filter(desc->mag_filter);
        if (1 == img->num_mipmaps) {
            if ((gl_min_filter==GL_NEAREST_MIPMAP_NEAREST)||(gl_min_filter==GL_NEAREST_MIPMAP_LINEAR)) {
                gl_min_filter = GL_NEAREST;
            }
            else if ((gl_min_filter==GL_LINEAR_MIPMAP_NEAREST)||(gl_min_filter==GL_LINEAR_MIPMAP_LINEAR)) {
                gl_min_filter = GL_LINEAR;
            }
            glTexParameteri(img->gl_target, GL_TEXTURE_MIN_FILTER, gl_min_filter);
            glTexParameteri(img->gl_target, GL_TEXTURE_MAG_FILTER, gl_mag_filter);
            if (img->type == SG_IMAGETYPE_CUBE) {
                glTexParameteri(img->gl_target, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
                glTexParameteri(img->gl_target, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
            }
            else {
                glTexParameteri(img->gl_target, GL_TEXTURE_WRAP_S, _sg_gl_wrap(img->wrap_u));
                glTexParameteri(img->gl_target, GL_TEXTURE_WRAP_T, _sg_gl_wrap(img->wrap_v));
                #if !defined(SOKOL_USE_GLES2)
                if (img->type == SG_IMAGETYPE_3D) {
                    glTexParameteri(img->gl_target, GL_TEXTURE_WRAP_R, _sg_gl_wrap(img->wrap_w));
                }
                #endif
            }
        }
        const uint16_t num_faces = img->type == SG_IMAGETYPE_CUBE ? 6 : 1;
        int data_index = 0;
        for (uint16_t mip_index = 0; mip_index < img->num_mipmaps; mip_index++) {
            for (uint16_t face_index = 0; face_index < num_faces; face_index++, data_index++) {
                GLenum gl_img_target = img->gl_target;
                if (SG_IMAGETYPE_CUBE == img->type) {
                    gl_img_target = _sg_gl_cubeface_target(face_index);
                }
                const GLvoid* data_ptr = 0;
                int data_size = 0;
                if (data_index < desc->num_data_items) {
                    SOKOL_ASSERT(desc->data_ptrs && desc->data_ptrs[data_index]);
                    SOKOL_ASSERT(desc->data_sizes && (desc->data_sizes[data_index] > 0));
                    data_ptr = desc->data_ptrs[data_index];
                    data_size = desc->data_sizes[data_index];
                }
                uint16_t mip_width = img->width >> mip_index;
                if (mip_width == 0) {
                    mip_width = 1;
                }
                uint16_t mip_height = img->height >> mip_index;
                if (mip_height == 0) {
                    mip_height = 1;
                }
                if ((SG_IMAGETYPE_2D == img->type) || (SG_IMAGETYPE_CUBE == img->type)) {
                    if (is_compressed) {
                        glCompressedTexImage2D(gl_img_target, mip_index, gl_internal_format,
                            mip_width, mip_height, 0, data_size, data_ptr);
                    }
                    else {
                        const GLenum gl_type = _sg_gl_teximage_type(img->color_format);
                        glTexImage2D(gl_img_target, mip_index, gl_internal_format,
                            mip_width, mip_height, 0, gl_format, gl_type, data_ptr);
                    }
                }
                #if !defined(SOKOL_USE_GLES2)
                else if ((SG_IMAGETYPE_3D == img->type) || (SG_IMAGETYPE_ARRAY == img->type)) {
                    uint16_t mip_depth = img->depth >> mip_index;
                    if (mip_depth == 0) {
                        mip_depth = 1;
                    }
                    if (is_compressed) {
                        glCompressedTexImage3D(gl_img_target, mip_index, gl_internal_format,
                            mip_width, mip_height, mip_depth, 0, data_size, data_ptr);
                    }
                    else {
                        const GLenum gl_type = _sg_gl_teximage_type(img->color_format);
                        glTexImage3D(gl_img_target, mip_index, gl_internal_format,
                            mip_width, mip_height, mip_depth, 0, gl_format, gl_type, data_ptr);
                    }
                }
                #endif
            }
        }
    }

    /* additional render target stuff */
    if (img->render_target) {
        /* MSAA render buffer */
        const bool msaa = (img->sample_count > 1) && (state->features[SG_FEATURE_MSAA_RENDER_TARGETS]);
        #if !defined(SOKOL_USE_GLES2)
        if (msaa) {
            glGenRenderbuffers(1, &img->gl_msaa_render_buffer);
            glBindRenderbuffer(GL_RENDERBUFFER, img->gl_msaa_render_buffer);
            glRenderbufferStorageMultisample(GL_RENDERBUFFER, img->sample_count, gl_internal_format, img->width, img->height);
        }
        #endif

        /* depth buffer */
        if (img->depth_format != SG_PIXELFORMAT_NONE) {
            glGenRenderbuffers(1, &img->gl_depth_render_buffer);
            glBindRenderbuffer(GL_RENDERBUFFER, img->gl_depth_render_buffer);
            GLenum gl_depth_format = _sg_gl_depth_attachment_format(img->depth_format);
            #if !defined(SOKOL_US_GLES2)
            if (msaa) {
                glRenderbufferStorageMultisample(GL_RENDERBUFFER, img->sample_count, gl_depth_format, img->width, img->height);
            }
            else
            #endif
            {
                glRenderbufferStorage(GL_RENDERBUFFER, gl_depth_format, img->width, img->height);
            }
        }
    }
    _SG_GL_CHECK_ERROR();
    img->slot.state = SG_RESOURCESTATE_VALID;
}

static void _sg_destroy_image(_sg_backend* state, _sg_image* img) {
    SOKOL_ASSERT(state && img);
    _SG_GL_CHECK_ERROR();
    for (int slot = 0; slot < img->num_slots; slot++) {
        if (img->gl_tex[slot]) {
            glDeleteTextures(1, &img->gl_tex[slot]);
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

static GLuint _sg_compile_shader(sg_shader_stage stage, const char* src) {
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
            GLchar* log_buf = SOKOL_MALLOC(log_len);
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

static void _sg_create_shader(_sg_backend* state, _sg_shader* shd, const sg_shader_desc* desc) {
    SOKOL_ASSERT(state && shd && desc);
    SOKOL_ASSERT(shd->slot.state == SG_RESOURCESTATE_ALLOC);
    SOKOL_ASSERT(!shd->gl_prog);
    _SG_GL_CHECK_ERROR();
    GLuint gl_vs = _sg_compile_shader(SG_SHADERSTAGE_VS, desc->vs.source);
    GLuint gl_fs = _sg_compile_shader(SG_SHADERSTAGE_FS, desc->fs.source);
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
            GLchar* log_buf = SOKOL_MALLOC(log_len);
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
        stage->num_uniform_blocks = stage_desc->num_ubs;
        for (int ub_index = 0; ub_index < stage_desc->num_ubs; ub_index++) {
            const sg_shader_uniform_block_desc* ub_desc = &stage_desc->ub[ub_index];
            _sg_uniform_block* ub = &stage->uniform_blocks[ub_index];
            ub->size = ub_desc->size;
            SOKOL_ASSERT(ub->num_uniforms == 0);
            ub->num_uniforms = ub_desc->num_uniforms;
            for (int u_index = 0; u_index < ub_desc->num_uniforms; u_index++) {
                const sg_shader_uniform_desc* u_desc = &ub_desc->u[u_index];
                _sg_uniform* u = &ub->uniforms[u_index];
                u->type = u_desc->type;
                u->offset = u_desc->offset;
                u->count = u_desc->array_count;
                if (u_desc->name) {
                    u->gl_loc = glGetUniformLocation(gl_prog, u_desc->name);
                }
                else {
                    u->gl_loc = u_index;
                }
            }
        }
    }

    /* resolve image locations */
    _SG_GL_CHECK_ERROR();
    int gl_tex_slot = 0;
    for (int stage_index = 0; stage_index < SG_NUM_SHADER_STAGES; stage_index++) {
        const sg_shader_stage_desc* stage_desc = (stage_index == SG_SHADERSTAGE_VS)? &desc->vs : &desc->fs;
        _sg_shader_stage* stage = &shd->stage[stage_index];
        SOKOL_ASSERT(stage->num_images == 0);
        stage->num_images = stage_desc->num_images;
        for (int img_index = 0; img_index < stage_desc->num_images; img_index++) {
            const sg_shader_image_desc* img_desc = &stage_desc->image[img_index];
            _sg_shader_image* img = &stage->images[img_index];
            SOKOL_ASSERT(img->type == SG_IMAGETYPE_INVALID);
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
        }
    }
    _SG_GL_CHECK_ERROR();
    shd->slot.state = SG_RESOURCESTATE_VALID;
}

static void _sg_destroy_shader(_sg_backend* state, _sg_shader* shd) {
    SOKOL_ASSERT(state && shd);
    _SG_GL_CHECK_ERROR();
    if (shd->gl_prog) {
        glDeleteProgram(shd->gl_prog);
    }
    _SG_GL_CHECK_ERROR();
    _sg_init_shader(shd);
}

static void _sg_create_pipeline(_sg_backend* state, _sg_pipeline* pip, _sg_shader* shd, const sg_pipeline_desc* desc) {
    SOKOL_ASSERT(pip && desc);
    SOKOL_ASSERT(pip->slot.state == SG_RESOURCESTATE_ALLOC);
    SOKOL_ASSERT(!pip->shader && pip->shader_id == SG_INVALID_ID);
    SOKOL_ASSERT(desc->shader == shd->slot.id);
    SOKOL_ASSERT(shd->gl_prog);
    #ifdef SOKOL_DEBUG
    for (int i = 0; i < SG_MAX_VERTEX_ATTRIBUTES; i++) {
        SOKOL_ASSERT(pip->gl_attrs[i].vb_index == -1);
    }
    #endif

    pip->shader = shd;
    pip->shader_id = desc->shader;
    pip->primitive_type = desc->primitive_type;
    pip->index_type = desc->index_type;
    pip->depth_stencil = desc->depth_stencil;
    pip->blend = desc->blend;
    pip->rast = desc->rast;
    
    /* resolve vertex attributes */
    for (int slot = 0; slot < SG_MAX_SHADERSTAGE_BUFFERS; slot++) {
        const sg_vertex_layout_desc* layout_desc = &desc->input_layouts[slot];
        for (int i = 0; i < layout_desc->num_attrs; i++) {
            const sg_vertex_attr_desc* attr_desc = &layout_desc->attrs[i];
            SOKOL_ASSERT(attr_desc->offset + _sg_vertexformat_bytesize(attr_desc->format) <= layout_desc->stride);
            #ifdef SOKOL_USE_GLES2
            /* on GLES2, attribute vertices must be bound by name */
            SOKOL_ASSERT(attr_desc->name);
            #else
            SOKOL_ASSERT(attr_desc->name || (attr_desc->index >= 0));
            #endif
            GLint attr_loc = attr_desc->index;
            if (attr_desc->name) {
                attr_loc = glGetAttribLocation(pip->shader->gl_prog, attr_desc->name);
            }
            SOKOL_ASSERT(attr_loc < SG_MAX_VERTEX_ATTRIBUTES);
            if (attr_loc != -1) {
                _sg_gl_attr* gl_attr = &pip->gl_attrs[attr_loc];
                gl_attr->vb_index = slot;
                if (layout_desc->step_func == SG_STEPFUNC_PER_VERTEX) {
                    gl_attr->divisor = 0;
                }
                else {
                    gl_attr->divisor = layout_desc->step_rate;
                }
                gl_attr->stride = layout_desc->stride;
                gl_attr->offset = attr_desc->offset;
                sg_vertex_format fmt = attr_desc->format;
                gl_attr->size = _sg_gl_vertexformat_size(fmt);
                gl_attr->type = _sg_gl_vertexformat_type(fmt);
                gl_attr->normalized = _sg_gl_vertexformat_normalized(fmt);
            }
        }
    }
    pip->slot.state = SG_RESOURCESTATE_VALID;
}

static void _sg_destroy_pipeline(_sg_backend* state, _sg_pipeline* pip) {
    SOKOL_ASSERT(pip);
    _sg_init_pipeline(pip);
}

/*
    _sg_create_pass

    att_imgs must point to a _sg_image* att_imgs[SG_MAX_COLOR_ATTACHMENTS+1] array,
    first entries are the color attachment images (or nullptr), last entry
    is the depth-stencil image (or nullptr).
*/
static void _sg_create_pass(_sg_backend* state, _sg_pass* pass, _sg_image** att_images, const sg_pass_desc* desc) {
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
        if (att_desc->image != SG_INVALID_ID) {
            SOKOL_ASSERT(att_images[i] && (att_images[i]->slot.id == att_desc->image));
            att = &pass->color_atts[i];
            SOKOL_ASSERT((att->image == 0) && (att->image_id == SG_INVALID_ID));
            att->image = att_images[i];
            att->image_id = att_desc->image;
            att->mip_level = att_desc->mip_level;
            att->slice = att_desc->slice;
        }
    }
    SOKOL_ASSERT(0 == pass->ds_att.image);
    att_desc = &desc->depth_stencil_attachment;
    const int ds_img_index = SG_MAX_COLOR_ATTACHMENTS;
    if (att_desc->image != SG_INVALID_ID) {
        SOKOL_ASSERT(att_images[ds_img_index] && (att_images[ds_img_index]->slot.id == att_desc->image));
        att = &pass->ds_att;
        SOKOL_ASSERT((att->image == 0) && (att->image_id == SG_INVALID_ID));
        att->image = att_images[ds_img_index];
        att->image_id = att_desc->image;
        att->mip_level = att_desc->mip_level;
        att->slice = att_desc->slice;
    }

    /* store current framebuffer binding (restored at end of function) */
    GLint gl_orig_fb;
    glGetIntegerv(GL_FRAMEBUFFER_BINDING, &gl_orig_fb);

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
                        #if !defined(ORYOL_USE_GLES2)
                        glFramebufferTextureLayer(GL_FRAMEBUFFER, gl_att, gl_tex, mip_level, slice);
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
        if (_sg_is_depth_stencil_format(pass->ds_att.image->depth_format)) {
            glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_STENCIL_ATTACHMENT, GL_RENDERBUFFER, gl_render_buffer);
        }
    }

    /* check if framebuffer is complete */
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        SOKOL_LOG("Framebuffer completeness check failed!\n");
        pass->slot.state = SG_RESOURCESTATE_FAILED;
        return;
    }
    
    /* FIXME: MSAA resolve buffers */

    /* restore original framebuffer binding */
    glBindFramebuffer(GL_FRAMEBUFFER, gl_orig_fb);
    _SG_GL_CHECK_ERROR();
    pass->slot.state = SG_RESOURCESTATE_VALID;
}

static void _sg_destroy_pass(_sg_backend* state, _sg_pass* pass) {
    SOKOL_ASSERT(state && pass);
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
static void _sg_begin_pass(_sg_backend* state, _sg_pass* pass, const sg_pass_action* action, int w, int h) {
    /* FIXME: what if a texture used as render target is still bound, should we
       unbind all currently bound textures in begin pass? */
    SOKOL_ASSERT(state);
    SOKOL_ASSERT(action);
    SOKOL_ASSERT(!state->in_pass);
    _SG_GL_CHECK_ERROR();
    state->in_pass = true;
    state->cur_pass_width = w;
    state->cur_pass_height = h;
    if (pass) {
        /* offscreen pass */
        SOKOL_ASSERT(pass->gl_fb);
        glBindFramebuffer(GL_FRAMEBUFFER, pass->gl_fb);
        #if !defined(SOKOL_USE_GLES2)
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
        #endif
    }
    else {
        /* default pass */
        /* FIXME: on some platforms default frame buffer isn't 0! */
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }
    glViewport(0, 0, w, h);
    if (state->cache.rast.scissor_test_enabled) {
        state->cache.rast.scissor_test_enabled = false;
        glDisable(GL_SCISSOR_TEST);
    }
    if (state->cache.blend.color_write_mask != SG_COLORMASK_RGBA) {
        state->cache.blend.color_write_mask = SG_COLORMASK_RGBA;
        glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
    }
    if (!state->cache.ds.depth_write_enabled) {
        state->cache.ds.depth_write_enabled = true;
        glDepthMask(GL_TRUE);
    }
    if (state->cache.ds.stencil_write_mask != 0xFF) {
        state->cache.ds.stencil_write_mask = 0xFF;
        glStencilMask(0xFF);
    }
    bool use_mrt_clear = (0 != pass);
    #if defined(SOKOL_USE_GLES2)
    use_mrt_clear = false;
    #endif
    if (!use_mrt_clear) {
        GLbitfield clear_mask = 0;
        if (action->actions & SG_PASSACTION_CLEAR_COLOR0) {
            clear_mask |= GL_COLOR_BUFFER_BIT;
            const float* c = action->color[0];
            glClearColor(c[0], c[1], c[2], c[3]);
        }
        if (action->actions & SG_PASSACTION_CLEAR_DEPTH_STENCIL) {
            /* FIXME: hmm separate depth/stencil clear? */
            clear_mask |= GL_DEPTH_BUFFER_BIT|GL_STENCIL_BUFFER_BIT;
            #ifdef SOKOL_USE_GLCORE33
            glClearDepth(action->depth);
            #else
            glClearDepthf(action->depth);
            #endif
            glClearStencil(action->stencil);
        }
        if (0 != clear_mask) {
            glClear(clear_mask);
        }
    }
    #if !defined SOKOL_USE_GLES2
    else {
        SOKOL_ASSERT(pass);
        for (int i = 0; i < SG_MAX_COLOR_ATTACHMENTS; i++) {
            if (pass->color_atts[i].image) {
                if (action->actions & (SG_PASSACTION_CLEAR_COLOR0<<i)) {
                    glClearBufferfv(GL_COLOR, i, action->color[i]);
                }
            }
            else {
                break;
            }
        }
        if (pass->ds_att.image && (action->actions & SG_PASSACTION_CLEAR_DEPTH_STENCIL)) {
            glClearBufferfi(GL_DEPTH_STENCIL, 0, action->depth, action->stencil);
        }
    }
    #endif
    _SG_GL_CHECK_ERROR();
}

static void _sg_end_pass(_sg_backend* state) {
    SOKOL_ASSERT(state);
    SOKOL_ASSERT(state->in_pass);
    /* FIXME: bind default framebuffer */
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    state->in_pass = false;
}

static void _sg_apply_draw_state(_sg_backend* state, 
    _sg_pipeline* pip, 
    _sg_buffer** vbs, int num_vbs, _sg_buffer* ib,
    _sg_image** vs_imgs, int num_vs_imgs,
    _sg_image** fs_imgs, int num_fs_imgs)
{
    SOKOL_ASSERT(state);
    SOKOL_ASSERT(pip);
    SOKOL_ASSERT(pip->shader);
    _SG_GL_CHECK_ERROR();

    state->cur_primitive_type = _sg_gl_primitive_type(pip->primitive_type);
    state->cur_index_type = _sg_gl_index_type(pip->index_type);
    state->cur_pipeline = pip;
    state->cur_pipeline_id = pip->slot.id;

    /* update depth-stencil state */
    const sg_depth_stencil_state* new_ds = &pip->depth_stencil;
    sg_depth_stencil_state* cache_ds = &state->cache.ds;
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
            cache_ds->stencil_read_mask = new_ds->stencil_read_mask;
            cache_ds->stencil_ref = new_ds->stencil_ref;
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

    /* update blend state */
    const sg_blend_state* new_b = &pip->blend;
    sg_blend_state* cache_b = &state->cache.blend;
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
    /* FIXME: fuzzy compare? */
    if ((new_b->blend_color[0] != cache_b->blend_color[0]) ||
        (new_b->blend_color[1] != cache_b->blend_color[1]) ||
        (new_b->blend_color[2] != cache_b->blend_color[2]) ||
        (new_b->blend_color[3] != cache_b->blend_color[3]))
    {
        const float* bc = new_b->blend_color;
        for (int i=0; i<4; i++) {
            cache_b->blend_color[i] = bc[i];
        }
        glBlendColor(bc[0], bc[1], bc[2], bc[3]);
    }

    /* update rasterizer state */
    const sg_rasterizer_state* new_r = &pip->rast;
    sg_rasterizer_state* cache_r = &state->cache.rast;
    if (new_r->cull_face_enabled != cache_r->cull_face_enabled) {
        cache_r->cull_face_enabled = new_r->cull_face_enabled;
        if (new_r->cull_face_enabled) glEnable(GL_CULL_FACE);
        else glDisable(GL_CULL_FACE);
    }
    if (new_r->cull_face != cache_r->cull_face) {
        cache_r->cull_face = new_r->cull_face;
        glCullFace(_sg_gl_cull_face(new_r->cull_face));
    }
    if (new_r->scissor_test_enabled != cache_r->scissor_test_enabled) {
        cache_r->scissor_test_enabled = new_r->scissor_test_enabled;
        if (new_r->scissor_test_enabled) glEnable(GL_SCISSOR_TEST);
        else glDisable(GL_SCISSOR_TEST);
    }
    if (new_r->dither_enabled != cache_r->dither_enabled) {
        cache_r->dither_enabled = new_r->dither_enabled;
        if (new_r->dither_enabled) glEnable(GL_DITHER);
        else glDisable(GL_DITHER);
    }
    #ifdef SOKOL_USE_GLCORE33
    if (new_r->sample_count != cache_r->sample_count) {
        cache_r->sample_count = new_r->sample_count;
        if (new_r->sample_count > 1) glEnable(GL_MULTISAMPLE);
        else glDisable(GL_MULTISAMPLE);
    }
    #endif

    /* bind shader program */
    glUseProgram(pip->shader->gl_prog);

    /* bind textures */
    _SG_GL_CHECK_ERROR();
    for (int stage_index = 0; stage_index < SG_NUM_SHADER_STAGES; stage_index++) {
        const _sg_shader_stage* stage = &pip->shader->stage[stage_index];
        _sg_image** imgs = (stage_index == SG_SHADERSTAGE_VS)? vs_imgs : fs_imgs;
        int num_imgs = (stage_index == SG_SHADERSTAGE_VS)? num_vs_imgs : num_fs_imgs;
        SOKOL_ASSERT(num_imgs == stage->num_images);
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
    if (ib) {
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ib->gl_buf[ib->active_slot]);
    }
    else {
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    }
    /* vertex attributes */
    GLuint gl_vb = 0;
    for (int attr_index = 0; attr_index < SG_MAX_VERTEX_ATTRIBUTES; attr_index++) {
        _sg_gl_attr* attr = &pip->gl_attrs[attr_index];
        _sg_gl_attr* cache_attr = &state->cache.attrs[attr_index];
        if (attr->vb_index >= 0) {
            /* attribute is enabled */
            SOKOL_ASSERT(attr->vb_index < num_vbs);
            _sg_buffer* vb = vbs[attr->vb_index];
            SOKOL_ASSERT(vb);
            if (gl_vb != vb->gl_buf[vb->active_slot]) {
                gl_vb = vb->gl_buf[vb->active_slot];
                glBindBuffer(GL_ARRAY_BUFFER, gl_vb);
            }
            glVertexAttribPointer(attr_index, attr->size, attr->type, 
                attr->normalized, attr->stride, 
                (const GLvoid*)(GLintptr)attr->offset);
            if (cache_attr->vb_index == -1) {
                glEnableVertexAttribArray(attr_index);
            }
            if (cache_attr->divisor != attr->divisor) {
                glVertexAttribDivisor(attr_index, attr->divisor);
            }
        }
        else {
            /* attribute is disabled */
            if (cache_attr->vb_index != -1) {
                glDisableVertexAttribArray(attr_index);
            }
        }
        *cache_attr = *attr;
    }
    _SG_GL_CHECK_ERROR();
}

static void _sg_apply_uniform_block(_sg_backend* state, sg_shader_stage stage_index, int ub_index, const void* data, int num_bytes) {
    SOKOL_ASSERT(state);
    SOKOL_ASSERT(data && (num_bytes > 0));
    SOKOL_ASSERT((stage_index >= 0) && ((int)stage_index < SG_NUM_SHADER_STAGES));
    if (!state->next_draw_valid) {
        return;
    }
    if (state->cur_pipeline->slot.id != state->cur_pipeline_id) {
        /* pipeline object was destroyed */
        return;
    }
    if (state->cur_pipeline->shader->slot.id != state->cur_pipeline->shader_id) {
        /* shader object was destroyed */
    }
    _sg_shader_stage* stage = &state->cur_pipeline->shader->stage[stage_index];
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
        }
    }
}

static void _sg_draw(_sg_backend* state, int base_element, int num_elements, int num_instances) {
    SOKOL_ASSERT(state);
    if (!state->next_draw_valid) {
        return;
    }
    const GLenum i_type = state->cur_index_type;
    const GLenum p_type = state->cur_primitive_type;
    if (0 != i_type) {
        /* indexed rendering */
        const int i_size = (i_type == GL_UNSIGNED_SHORT) ? 2 : 4;
        const GLvoid* indices = (const GLvoid*)(GLintptr)(base_element*i_size);
        if (num_instances == 1) {
            glDrawElements(p_type, num_elements, i_type, indices);
        }
        else {
            if (state->features[SG_FEATURE_INSTANCED_ARRAYS]) {
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
            if (state->features[SG_FEATURE_INSTANCED_ARRAYS]) {
                glDrawArraysInstanced(p_type, base_element, num_elements, num_instances);
            }
        }
    }
}

static void _sg_commit(_sg_backend* state) {
    SOKOL_ASSERT(state);
    SOKOL_ASSERT(!state->in_pass);
    state->frame_index++;
}

static void _sg_update_buffer(_sg_backend* state, _sg_buffer* buf, const void* data_ptr, int data_size) {
    SOKOL_ASSERT(state && buf && data_ptr && data_size > 0);
    /* only one update per buffer per frame allowed */
    SOKOL_ASSERT(buf->upd_frame_index != state->frame_index);
    SOKOL_ASSERT((buf->usage == SG_USAGE_DYNAMIC) || (buf->usage == SG_USAGE_STREAM));
    SOKOL_ASSERT(data_size <= buf->size);
    buf->upd_frame_index = state->frame_index;
    if (++buf->active_slot >= buf->num_slots) {
        buf->active_slot = 0;
    }
    GLenum gl_tgt = _sg_gl_buffer_target(buf->type);
    SOKOL_ASSERT(buf->active_slot < _SG_GL_NUM_UPDATE_SLOTS);
    GLuint gl_buf = buf->gl_buf[buf->active_slot];
    SOKOL_ASSERT(gl_buf);
    _SG_GL_CHECK_ERROR();
    glBindBuffer(gl_tgt, gl_buf);
    glBufferSubData(gl_tgt, 0, data_size, data_ptr);
    _SG_GL_CHECK_ERROR();
}
