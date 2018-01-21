/*
    Sokol Gfx generic implementation code
*/
#ifndef SOKOL_IMPL_GUARD
#error "Please do not include *.impl.h files directly"
#endif

#ifndef SOKOL_DEBUG
    #ifdef _DEBUG
        #define SOKOL_DEBUG (1)
    #endif
#endif
#ifndef SOKOL_ASSERT
    #include <assert.h>
    #define SOKOL_ASSERT(c) assert(c)
#endif
#ifndef SOKOL_VALIDATE_BEGIN
    #define SOKOL_VALIDATE_BEGIN() _sg_validate_begin()
#endif
#ifndef SOKOL_VALIDATE
    #define SOKOL_VALIDATE(cond, err) _sg_validate(cond, err)
#endif
#ifndef SOKOL_VALIDATE_END
    #define SOKOL_VALIDATE_END() _sg_validate_end()
#endif
#ifndef SOKOL_UNREACHABLE
    #define SOKOL_UNREACHABLE SOKOL_ASSERT(false)
#endif
#ifndef SOKOL_MALLOC
    #include <stdlib.h>
    #define SOKOL_MALLOC(s) malloc(s)
    #define SOKOL_FREE(p) free(p)
#endif
#ifndef SOKOL_LOG
    #ifdef SOKOL_DEBUG 
        #include <stdio.h>
        #define SOKOL_LOG(s) { SOKOL_ASSERT(s); puts(s); }
    #else
        #define SOKOL_LOG(s)
    #endif
#endif
#if !(defined(SOKOL_GLCORE33)||defined(SOKOL_GLES2)||defined(SOKOL_GLES3)||defined(SOKOL_D3D11)||defined(SOKOL_METAL_MACOS)||defined(SOKOL_METAL_IOS))
#error "Please select a backend with SOKOL_GLCORE33, SOKOL_GLES2, SOKOL_GLES3, SOKOL_D3D11, SOKOL_METAL_MACOS or SOKOL_METAL_IOS"
#endif

#ifndef _SOKOL_PRIVATE
    #if defined(__GNUC__)
        #define _SOKOL_PRIVATE __attribute__((unused)) static
    #else
        #define _SOKOL_PRIVATE static
    #endif
#endif

/* default clear values */
#ifndef SG_DEFAULT_CLEAR_RED
#define SG_DEFAULT_CLEAR_RED (0.5f)
#endif
#ifndef SG_DEFAULT_CLEAR_GREEN
#define SG_DEFAULT_CLEAR_GREEN (0.5f)
#endif
#ifndef SG_DEFAULT_CLEAR_BLUE
#define SG_DEFAULT_CLEAR_BLUE (0.5f)
#endif
#ifndef SG_DEFAULT_CLEAR_ALPHA
#define SG_DEFAULT_CLEAR_ALPHA (1.0f)
#endif
#ifndef SG_DEFAULT_CLEAR_DEPTH
#define SG_DEFAULT_CLEAR_DEPTH (1.0f)
#endif
#ifndef SG_DEFAULT_CLEAR_STENCIL
#define SG_DEFAULT_CLEAR_STENCIL (0)
#endif

#ifdef __cplusplus
extern "C" {
#endif

enum {
    _SG_SLOT_SHIFT = 16,
    _SG_SLOT_MASK = (1<<_SG_SLOT_SHIFT)-1,
    _SG_MAX_POOL_SIZE = (1<<_SG_SLOT_SHIFT),
    _SG_DEFAULT_BUFFER_POOL_SIZE = 128,
    _SG_DEFAULT_IMAGE_POOL_SIZE = 128,
    _SG_DEFAULT_SHADER_POOL_SIZE = 32,
    _SG_DEFAULT_PIPELINE_POOL_SIZE = 64,
    _SG_DEFAULT_PASS_POOL_SIZE = 16,
};

/* helper macros */
#define _sg_def(val, def) (((val) == 0) ? (def) : (val))
#define _sg_def_flt(val, def) (((val) == 0.0f) ? (def) : (val))
#define _sg_min(a,b) ((a<b)?a:b)
#define _sg_max(a,b) ((a>b)?a:b)
#define _sg_clamp(v,v0,v1) ((v<v0)?(v0):((v>v1)?(v1):(v)))
#define _sg_fequal(val,cmp,delta) (((val-cmp)> -delta)&&((val-cmp)<delta))


/*-- helper functions --------------------------------------------------------*/

/* return byte size of a vertex format */
_SOKOL_PRIVATE int _sg_vertexformat_bytesize(sg_vertex_format fmt) {
    switch (fmt) {
        case SG_VERTEXFORMAT_FLOAT:     return 4;
        case SG_VERTEXFORMAT_FLOAT2:    return 8;
        case SG_VERTEXFORMAT_FLOAT3:    return 12;
        case SG_VERTEXFORMAT_FLOAT4:    return 16;
        case SG_VERTEXFORMAT_BYTE4:     return 4;
        case SG_VERTEXFORMAT_BYTE4N:    return 4;
        case SG_VERTEXFORMAT_UBYTE4:    return 4;
        case SG_VERTEXFORMAT_UBYTE4N:   return 4;
        case SG_VERTEXFORMAT_SHORT2:    return 4;
        case SG_VERTEXFORMAT_SHORT2N:   return 4;
        case SG_VERTEXFORMAT_SHORT4:    return 8;
        case SG_VERTEXFORMAT_SHORT4N:   return 8;
        case SG_VERTEXFORMAT_UINT10_N2: return 4;
        case SG_VERTEXFORMAT_INVALID:   return 0;
        default:
            SOKOL_UNREACHABLE;
            return -1;
    }
}

/* return the byte size of a shader uniform */
_SOKOL_PRIVATE int _sg_uniform_size(sg_uniform_type type, int count) {
    switch (type) {
        case SG_UNIFORMTYPE_INVALID:    return 0;
        case SG_UNIFORMTYPE_FLOAT:      return 4 * count;
        case SG_UNIFORMTYPE_FLOAT2:     return 8 * count;
        case SG_UNIFORMTYPE_FLOAT3:     return 12 * count; /* FIXME: std140??? */
        case SG_UNIFORMTYPE_FLOAT4:     return 16 * count;
        case SG_UNIFORMTYPE_MAT4:       return 64 * count;
        default:
            SOKOL_UNREACHABLE;
            return -1;
    }
}

/* return true if pixel format is a compressed format */
_SOKOL_PRIVATE bool _sg_is_compressed_pixel_format(sg_pixel_format fmt) {
    switch (fmt) {
        case SG_PIXELFORMAT_DXT1:
        case SG_PIXELFORMAT_DXT3:
        case SG_PIXELFORMAT_DXT5:
        case SG_PIXELFORMAT_PVRTC2_RGB:
        case SG_PIXELFORMAT_PVRTC4_RGB:
        case SG_PIXELFORMAT_PVRTC2_RGBA:
        case SG_PIXELFORMAT_PVRTC4_RGBA:
        case SG_PIXELFORMAT_ETC2_RGB8:
        case SG_PIXELFORMAT_ETC2_SRGB8:
            return true;
        default:
            return false;
    }
}

/* return true if pixel format is a valid render target format */
_SOKOL_PRIVATE bool _sg_is_valid_rendertarget_color_format(sg_pixel_format fmt) {
    switch (fmt) {
        case SG_PIXELFORMAT_RGBA8:
        case SG_PIXELFORMAT_R10G10B10A2:
        case SG_PIXELFORMAT_RGBA32F:
        case SG_PIXELFORMAT_RGBA16F:
            return true;
        default:
            return false;
    }
}

/* return true if pixel format is a valid depth format */
_SOKOL_PRIVATE bool _sg_is_valid_rendertarget_depth_format(sg_pixel_format fmt) {
    switch (fmt) {
        case SG_PIXELFORMAT_DEPTH:
        case SG_PIXELFORMAT_DEPTHSTENCIL:
            return true;
        default:
            return false;
    }
}

/* return true if pixel format is a depth-stencil format */
_SOKOL_PRIVATE bool _sg_is_depth_stencil_format(sg_pixel_format fmt) {
    /* FIXME: more depth stencil formats? */
    return (SG_PIXELFORMAT_DEPTHSTENCIL == fmt);
}

/* return the bytes-per-pixel for a pixel format */
_SOKOL_PRIVATE int _sg_pixelformat_bytesize(sg_pixel_format fmt) {
    switch (fmt) {
        case SG_PIXELFORMAT_RGBA32F:
            return 16;
        case SG_PIXELFORMAT_RGBA16F:
            return 8;
        case SG_PIXELFORMAT_RGBA8:
        case SG_PIXELFORMAT_R10G10B10A2:
        case SG_PIXELFORMAT_R32F:
            return 4;
        case SG_PIXELFORMAT_RGB8:
            return 3;
        case SG_PIXELFORMAT_R5G5B5A1:
        case SG_PIXELFORMAT_R5G6B5:
        case SG_PIXELFORMAT_RGBA4:
        case SG_PIXELFORMAT_R16F:
            return 2;
        case SG_PIXELFORMAT_L8:
            return 1;
        default:
            SOKOL_UNREACHABLE;
            return 0;
    }
}

/* return row pitch for an image */
_SOKOL_PRIVATE int _sg_row_pitch(sg_pixel_format fmt, int width) {
    int pitch;
    switch (fmt) {
        case SG_PIXELFORMAT_DXT1:
        case SG_PIXELFORMAT_ETC2_RGB8:
        case SG_PIXELFORMAT_ETC2_SRGB8:
            pitch = ((width + 3) / 4) * 8;
            pitch = pitch < 8 ? 8 : pitch;
            break;
        case SG_PIXELFORMAT_DXT3:
        case SG_PIXELFORMAT_DXT5:
            pitch = ((width + 3) / 4) * 16;
            pitch = pitch < 16 ? 16 : pitch;
            break;
        case SG_PIXELFORMAT_PVRTC4_RGB:
        case SG_PIXELFORMAT_PVRTC4_RGBA:
            {
                const int block_size = 4*4;
                const int bpp = 4;
                int width_blocks = width / 4;
                width_blocks = width_blocks < 2 ? 2 : width_blocks;
                pitch = width_blocks * ((block_size * bpp) / 8);
            }
            break;
        case SG_PIXELFORMAT_PVRTC2_RGB:
        case SG_PIXELFORMAT_PVRTC2_RGBA:
            {
                const int block_size = 8*4;
                const int bpp = 2;
                int width_blocks = width / 4;
                width_blocks = width_blocks < 2 ? 2 : width_blocks;
                pitch = width_blocks * ((block_size * bpp) / 8);
            }
            break;
        default:
            pitch = width * _sg_pixelformat_bytesize(fmt);
            break;
    }
    return pitch;
}

/* return pitch of a 2D subimage / texture slice */
_SOKOL_PRIVATE int _sg_surface_pitch(sg_pixel_format fmt, int width, int height) {
    int num_rows = 0;
    switch (fmt) {
        case SG_PIXELFORMAT_DXT1:
        case SG_PIXELFORMAT_DXT3:
        case SG_PIXELFORMAT_DXT5:
        case SG_PIXELFORMAT_ETC2_RGB8:
        case SG_PIXELFORMAT_ETC2_SRGB8:
        case SG_PIXELFORMAT_PVRTC2_RGB:
        case SG_PIXELFORMAT_PVRTC2_RGBA:
        case SG_PIXELFORMAT_PVRTC4_RGB:
        case SG_PIXELFORMAT_PVRTC4_RGBA:
            num_rows = ((height + 3) / 4);
            break;
        default:
            num_rows = height;
            break;
    }
    if (num_rows < 1) {
        num_rows = 1;
    }
    return num_rows * _sg_row_pitch(fmt, width);
}

/* resolve pass action defaults into a new pass action struct */
_SOKOL_PRIVATE void _sg_resolve_default_pass_action(const sg_pass_action* from, sg_pass_action* to) {
    SOKOL_ASSERT(from && to);
    *to = *from;
    for (int i = 0; i < SG_MAX_COLOR_ATTACHMENTS; i++) {
        if (to->colors[i].action  == _SG_ACTION_DEFAULT) {
            to->colors[i].action = SG_ACTION_CLEAR;
            to->colors[i].val[0] = SG_DEFAULT_CLEAR_RED;
            to->colors[i].val[1] = SG_DEFAULT_CLEAR_GREEN;
            to->colors[i].val[2] = SG_DEFAULT_CLEAR_BLUE;
            to->colors[i].val[3] = SG_DEFAULT_CLEAR_ALPHA;
        }
    }
    if (to->depth.action == _SG_ACTION_DEFAULT) {
        to->depth.action = SG_ACTION_CLEAR;
        to->depth.val = SG_DEFAULT_CLEAR_DEPTH;
    }
    if (to->stencil.action == _SG_ACTION_DEFAULT) {
        to->stencil.action = SG_ACTION_CLEAR;
        to->stencil.val = SG_DEFAULT_CLEAR_STENCIL;
    }
}

/*-- resource pool slots (must be defined before rendering backend) ----------*/
typedef struct {
    uint32_t id;
    sg_resource_state state;
} _sg_slot;

_SOKOL_PRIVATE int _sg_slot_index(uint32_t id) {
    return id & _SG_SLOT_MASK;
}

/*-- include the selected rendering backend ----------------------------------*/
#ifdef __cplusplus
} /* extern "C" */
#endif
#if defined(SOKOL_GLCORE33) || defined(SOKOL_GLES2) || defined(SOKOL_GLES3)
#include "_sokol_gfx_gl.impl.h"
#elif defined(SOKOL_D3D11)
#include "_sokol_gfx_d3d11.impl.h"
#elif defined(SOKOL_METAL_MACOS) || defined(SOKOL_METAL_IOS)
#include "_sokol_gfx_metal.impl.h"
#else
#error "No rendering backend selected"
#endif
#ifdef __cplusplus
extern "C" {
#endif

/*-- resource pools ----------------------------------------------------------*/
typedef struct {
    int size;
    uint32_t unique_counter;
    int queue_top;
    int* free_queue;
} _sg_pool;

_SOKOL_PRIVATE void _sg_init_pool(_sg_pool* pool, int num) {
    SOKOL_ASSERT(pool && (num > 1));
    /* slot 0 is reserved for the 'invalid id', so bump the pool size by 1 */
    pool->size = num + 1;
    pool->queue_top = 0;
    pool->unique_counter = 0;
    /* it's not a bug to only reserve 'num' here */
    pool->free_queue = (int*) SOKOL_MALLOC(sizeof(int)*num);
    SOKOL_ASSERT(pool->free_queue);
    /* never allocate the zero-th pool item since the invalid id is 0 */
    for (int i = pool->size-1; i >= 1; i--) {
        pool->free_queue[pool->queue_top++] = i;
    }
}

_SOKOL_PRIVATE void _sg_discard_pool(_sg_pool* pool) {
    SOKOL_ASSERT(pool);
    SOKOL_FREE(pool->free_queue);
    pool->free_queue = 0;
    pool->size = 0;
    pool->queue_top = 0;
    pool->unique_counter = 0;
}

_SOKOL_PRIVATE uint32_t _sg_pool_alloc_id(_sg_pool* pool) {
    SOKOL_ASSERT(pool);
    SOKOL_ASSERT(pool->free_queue);
    if (pool->queue_top > 0) {
        int slot_index = pool->free_queue[--pool->queue_top];
        return ((pool->unique_counter++)<<_SG_SLOT_SHIFT)|slot_index;
    }
    else {
        /* pool exhausted */
        return SG_INVALID_ID;
    }
}

_SOKOL_PRIVATE void _sg_pool_free_id(_sg_pool* pool, uint32_t id) {
    SOKOL_ASSERT(id != SG_INVALID_ID);
    SOKOL_ASSERT(pool);
    SOKOL_ASSERT(pool->free_queue);
    SOKOL_ASSERT(pool->queue_top < pool->size);
    #ifdef SOKOL_DEBUG
    /* debug check against double-free */
    int slot_index = _sg_slot_index(id);
    for (int i = 0; i < pool->queue_top; i++) {
        SOKOL_ASSERT(pool->free_queue[i] != slot_index);
    }
    #endif
    pool->free_queue[pool->queue_top++] = id;
    SOKOL_ASSERT(pool->queue_top <= (pool->size-1));
}

typedef struct {
    _sg_pool buffer_pool;
    _sg_pool image_pool;
    _sg_pool shader_pool;
    _sg_pool pipeline_pool;
    _sg_pool pass_pool;
    _sg_buffer* buffers;
    _sg_image* images;
    _sg_shader* shaders;
    _sg_pipeline* pipelines;
    _sg_pass* passes;
} _sg_pools;

_SOKOL_PRIVATE void _sg_setup_pools(_sg_pools* p, const sg_desc* desc) {
    SOKOL_ASSERT(p);
    SOKOL_ASSERT(desc);
    /* note: the pools here will have an additional item, since slot 0 is reserved */
    SOKOL_ASSERT((desc->buffer_pool_size >= 0) && (desc->buffer_pool_size < _SG_MAX_POOL_SIZE));
    _sg_init_pool(&p->buffer_pool, _sg_def(desc->buffer_pool_size, _SG_DEFAULT_BUFFER_POOL_SIZE));
    p->buffers = (_sg_buffer*) SOKOL_MALLOC(sizeof(_sg_buffer) * p->buffer_pool.size);
    SOKOL_ASSERT(p->buffers);
    for (int i = 0; i < p->buffer_pool.size; i++) {
        _sg_init_buffer(&p->buffers[i]);
    }

    SOKOL_ASSERT((desc->image_pool_size >= 0) && (desc->image_pool_size < _SG_MAX_POOL_SIZE));
    _sg_init_pool(&p->image_pool, _sg_def(desc->image_pool_size, _SG_DEFAULT_IMAGE_POOL_SIZE));
    p->images = (_sg_image*) SOKOL_MALLOC(sizeof(_sg_image) * p->image_pool.size);
    SOKOL_ASSERT(p->images);
    for (int i = 0; i < p->image_pool.size; i++) {
        _sg_init_image(&p->images[i]);
    }

    SOKOL_ASSERT((desc->shader_pool_size >= 0) && (desc->shader_pool_size < _SG_MAX_POOL_SIZE));
    _sg_init_pool(&p->shader_pool, _sg_def(desc->shader_pool_size, _SG_DEFAULT_SHADER_POOL_SIZE));
    p->shaders = (_sg_shader*) SOKOL_MALLOC(sizeof(_sg_shader) * p->shader_pool.size);
    SOKOL_ASSERT(p->shaders);
    for (int i = 0; i < p->shader_pool.size; i++) {
        _sg_init_shader(&p->shaders[i]);
    }

    SOKOL_ASSERT((desc->pipeline_pool_size >= 0) && (desc->pipeline_pool_size < _SG_MAX_POOL_SIZE));
    _sg_init_pool(&p->pipeline_pool, _sg_def(desc->pipeline_pool_size, _SG_DEFAULT_PIPELINE_POOL_SIZE));
    p->pipelines = (_sg_pipeline*) SOKOL_MALLOC(sizeof(_sg_pipeline) * p->pipeline_pool.size);
    SOKOL_ASSERT(p->pipelines);
    for (int i = 0; i < p->pipeline_pool.size; i++) {
        _sg_init_pipeline(&p->pipelines[i]);
    }

    SOKOL_ASSERT((desc->pass_pool_size >= 0) && (desc->pass_pool_size < _SG_MAX_POOL_SIZE));
    _sg_init_pool(&p->pass_pool, _sg_def(desc->pass_pool_size, _SG_DEFAULT_PASS_POOL_SIZE));
    p->passes = (_sg_pass*) SOKOL_MALLOC(sizeof(_sg_pass) * p->pass_pool.size);
    SOKOL_ASSERT(p->passes);
    for (int i = 0; i < p->pass_pool.size; i++) {
        _sg_init_pass(&p->passes[i]);
    }
}

_SOKOL_PRIVATE void _sg_discard_pools(_sg_pools* p) {
    SOKOL_ASSERT(p);
    SOKOL_FREE(p->passes);      p->passes = 0;
    SOKOL_FREE(p->pipelines);   p->pipelines = 0;
    SOKOL_FREE(p->shaders);     p->shaders = 0;
    SOKOL_FREE(p->images);      p->images = 0;
    SOKOL_FREE(p->buffers);     p->buffers = 0;
    _sg_discard_pool(&p->pass_pool);
    _sg_discard_pool(&p->pipeline_pool);
    _sg_discard_pool(&p->shader_pool);
    _sg_discard_pool(&p->image_pool);
    _sg_discard_pool(&p->buffer_pool);
}

/* returns pointer to resource by id without matching id check */
_SOKOL_PRIVATE _sg_buffer* _sg_buffer_at(const _sg_pools* p, uint32_t buf_id) {
    SOKOL_ASSERT(p && SG_INVALID_ID != buf_id);
    int slot_index = _sg_slot_index(buf_id);
    SOKOL_ASSERT((slot_index >= 0) && (slot_index < p->buffer_pool.size));
    return &p->buffers[slot_index];
}

_SOKOL_PRIVATE _sg_image* _sg_image_at(const _sg_pools* p, uint32_t img_id) {
    SOKOL_ASSERT(p && SG_INVALID_ID != img_id);
    int slot_index = _sg_slot_index(img_id);
    SOKOL_ASSERT((slot_index >= 0) && (slot_index < p->image_pool.size));
    return &p->images[slot_index];
}

_SOKOL_PRIVATE _sg_shader* _sg_shader_at(const _sg_pools* p, uint32_t shd_id) {
    SOKOL_ASSERT(p && SG_INVALID_ID != shd_id);
    int slot_index = _sg_slot_index(shd_id);
    SOKOL_ASSERT((slot_index >= 0) && (slot_index < p->shader_pool.size));
    return &p->shaders[slot_index];
}

_SOKOL_PRIVATE _sg_pipeline* _sg_pipeline_at(const _sg_pools* p, uint32_t pip_id) {
    SOKOL_ASSERT(p && SG_INVALID_ID != pip_id);
    int slot_index = _sg_slot_index(pip_id);
    SOKOL_ASSERT((slot_index >= 0) && (slot_index < p->pipeline_pool.size));
    return &p->pipelines[slot_index];
}

_SOKOL_PRIVATE _sg_pass* _sg_pass_at(const _sg_pools* p, uint32_t pass_id) {
    SOKOL_ASSERT(p && SG_INVALID_ID != pass_id);
    int slot_index = _sg_slot_index(pass_id);
    SOKOL_ASSERT((slot_index >= 0) && (slot_index < p->pass_pool.size));
    return &p->passes[slot_index];
}

/* returns pointer to resource with matching id check, may return 0 */
_SOKOL_PRIVATE _sg_buffer* _sg_lookup_buffer(const _sg_pools* p, uint32_t buf_id) {
    if (SG_INVALID_ID != buf_id) {
        _sg_buffer* buf = _sg_buffer_at(p, buf_id);
        if (buf->slot.id == buf_id) {
            return buf;
        }
    }
    return 0;
}

_SOKOL_PRIVATE _sg_image* _sg_lookup_image(const _sg_pools* p, uint32_t img_id) {
    if (SG_INVALID_ID != img_id) {
        _sg_image* img = _sg_image_at(p, img_id); 
        if (img->slot.id == img_id) {
            return img;
        }
    }
    return 0;
}

_SOKOL_PRIVATE _sg_shader* _sg_lookup_shader(const _sg_pools* p, uint32_t shd_id) {
    SOKOL_ASSERT(p);
    if (SG_INVALID_ID != shd_id) {
        _sg_shader* shd = _sg_shader_at(p, shd_id);
        if (shd->slot.id == shd_id) {
            return shd;
        }
    }
    return 0;
}

_SOKOL_PRIVATE _sg_pipeline* _sg_lookup_pipeline(const _sg_pools* p, uint32_t pip_id) {
    SOKOL_ASSERT(p);
    if (SG_INVALID_ID != pip_id) {
        _sg_pipeline* pip = _sg_pipeline_at(p, pip_id);
        if (pip->slot.id == pip_id) {
            return pip;
        }
    }
    return 0;
}

_SOKOL_PRIVATE _sg_pass* _sg_lookup_pass(const _sg_pools* p, uint32_t pass_id) {
    SOKOL_ASSERT(p);
    if (SG_INVALID_ID != pass_id) {
        _sg_pass* pass = _sg_pass_at(p, pass_id);
        if (pass->slot.id == pass_id) {
            return pass;
        }
    }
    return 0;
}

_SOKOL_PRIVATE void _sg_destroy_all_resources(_sg_pools* p) {
    /*  this is a bit dumb since it loops over all pool slots to
        find the occupied slots, on the other hand it is only ever
        executed at shutdown
    */
    for (int i = 0; i < p->buffer_pool.size; i++) {
        if (p->buffers[i].slot.state == SG_RESOURCESTATE_VALID) {
            _sg_destroy_buffer(&p->buffers[i]);
        }
    }
    for (int i = 0; i < p->image_pool.size; i++) {
        if (p->images[i].slot.state == SG_RESOURCESTATE_VALID) {
            _sg_destroy_image(&p->images[i]);
        }
    }
    for (int i = 0; i < p->shader_pool.size; i++) {
        if (p->shaders[i].slot.state == SG_RESOURCESTATE_VALID) {
            _sg_destroy_shader(&p->shaders[i]);
        }
    }
    for (int i = 0; i < p->pipeline_pool.size; i++) {
        if (p->pipelines[i].slot.state == SG_RESOURCESTATE_VALID) {
            _sg_destroy_pipeline(&p->pipelines[i]);
        }
    }
    for (int i = 0; i < p->pass_pool.size; i++) {
        if (p->passes[i].slot.state == SG_RESOURCESTATE_VALID) {
            _sg_destroy_pass(&p->passes[i]);
        }
    }
}

/*-- validation error codes --------------------------------------------------*/
#if defined(SOKOL_DEBUG)
typedef enum {
    /* special case 'validation was successful' */
    _SG_VALIDATE_SUCCESS,

    /* buffer creation */
    _SG_VALIDATE_BUFFERDESC_CANARY,
    _SG_VALIDATE_BUFFERDESC_SIZE,
    _SG_VALIDATE_BUFFERDESC_CONTENT,
    _SG_VALIDATE_BUFFERDESC_NO_CONTENT,

    /* image creation */
    _SG_VALIDATE_IMAGEDESC_CANARY,
    _SG_VALIDATE_IMAGEDESC_WIDTH,
    _SG_VALIDATE_IMAGEDESC_HEIGHT,
    _SG_VALIDATE_IMAGEDESC_RT_PIXELFORMAT,
    _SG_VALIDATE_IMAGEDESC_NONRT_PIXELFORMAT,
    _SG_VALIDATE_IMAGEDESC_MSAA_BUT_NO_RT,
    _SG_VALIDATE_IMAGEDESC_NO_MSAA_RT_SUPPORT,
    _SG_VALIDATE_IMAGEDESC_RT_IMMUTABLE,
    _SG_VALIDATE_IMAGEDESC_RT_NO_CONTENT,
    _SG_VALIDATE_IMAGEDESC_CONTENT,
    _SG_VALIDATE_IMAGEDESC_NO_CONTENT,

    /* shader creation */
    _SG_VALIDATE_SHADERDESC_CANARY,
    _SG_VALIDATE_SHADERDESC_SOURCE,
    _SG_VALIDATE_SHADERDESC_BYTECODE,
    _SG_VALIDATE_SHADERDESC_SOURCE_OR_BYTECODE,
    _SG_VALIDATE_SHADERDESC_NO_BYTECODE_SIZE,
    _SG_VALIDATE_SHADERDESC_NO_CONT_UBS,
    _SG_VALIDATE_SHADERDESC_NO_CONT_IMGS,
    _SG_VALIDATE_SHADERDESC_NO_CONT_UB_MEMBERS,
    _SG_VALIDATE_SHADERDESC_NO_UB_MEMBERS,
    _SG_VALIDATE_SHADERDESC_UB_MEMBER_NAME,
    _SG_VALIDATE_SHADERDESC_UB_SIZE_MISMATCH,
    _SG_VALIDATE_SHADERDESC_IMG_NAME,

    /* pipeline creation */
    _SG_VALIDATE_PIPELINEDESC_CANARY,
    _SG_VALIDATE_PIPELINEDESC_SHADER,
    _SG_VALIDATE_PIPELINEDESC_NO_LAYOUT,
    _SG_VALIDATE_PIPELINEDESC_NO_CONT_LAYOUTS,
    _SG_VALIDATE_PIPELINEDESC_LAYOUT_STRIDE4,
    _SG_VALIDATE_PIPELINEDESC_NO_ATTRS,
    _SG_VALIDATE_PIPELINEDESC_TOO_MANY_ATTRS,
    _SG_VALIDATE_PIPELINEDESC_NO_CONT_ATTRS,
    _SG_VALIDATE_PIPELINEDESC_ATTR_NAME,
    _SG_VALIDATE_PIPELINEDESC_ATTR_SEMANTICS,
    _SG_VALIDATE_PIPELINEDESC_ATTR_OVERFLOW,

    /* pass creation */
    _SG_VALIDATE_PASSDESC_CANARY,
    _SG_VALIDATE_PASSDESC_NO_COLOR_ATTS,
    _SG_VALIDATE_PASSDESC_NO_CONT_COLOR_ATTS,
    _SG_VALIDATE_PASSDESC_IMAGE,
    _SG_VALIDATE_PASSDESC_MIPLEVEL,
    _SG_VALIDATE_PASSDESC_FACE,
    _SG_VALIDATE_PASSDESC_LAYER,
    _SG_VALIDATE_PASSDESC_SLICE,
    _SG_VALIDATE_PASSDESC_IMAGE_NO_RT,
    _SG_VALIDATE_PASSDESC_COLOR_PIXELFORMATS,
    _SG_VALIDATE_PASSDESC_COLOR_INV_PIXELFORMAT,
    _SG_VALIDATE_PASSDESC_DEPTH_INV_PIXELFORMAT,
    _SG_VALIDATE_PASSDESC_IMAGE_SIZES,
    _SG_VALIDATE_PASSDESC_IMAGE_SAMPLE_COUNTS,

    /* sg_begin_pass validation */
    _SG_VALIDATE_BEGINPASS_PASS,
    _SG_VALIDATE_BEGINPASS_IMAGE,

    /* sg_apply_draw_state validation */
    _SG_VALIDATE_ADS_PIP,
    _SG_VALIDATE_ADS_VBS,
    _SG_VALIDATE_ADS_VB_TYPE,
    _SG_VALIDATE_ADS_NO_IB,
    _SG_VALIDATE_ADS_IB,
    _SG_VALIDATE_ADS_IB_TYPE,
    _SG_VALIDATE_ADS_VS_IMGS,
    _SG_VALIDATE_ADS_VS_IMG_TYPES,
    _SG_VALIDATE_ADS_FS_IMGS,
    _SG_VALIDATE_ADS_FS_IMG_TYPES,
    _SG_VALIDATE_ADS_ATT_COUNT,
    _SG_VALIDATE_ADS_COLOR_FORMAT,
    _SG_VALIDATE_ADS_DEPTH_FORMAT,
    _SG_VALIDATE_ADS_SAMPLE_COUNT,

    /* sg_apply_uniform_block validation */
    _SG_VALIDATE_AUB_NO_PIPELINE,
    _SG_VALIDATE_AUB_NO_UB_AT_SLOT,
    _SG_VALIDATE_AUB_SIZE,
        
    /* sg_update_buffer validation */
    _SG_VALIDATE_UPDBUF_USAGE,
    _SG_VALIDATE_UPDBUF_SIZE,
    _SG_VALIDATE_UPDBUF_ONCE,
    
    /* sg_update_image validation */
    _SG_VALIDATE_UPDIMG_USAGE,
    _SG_VALIDATE_UPDIMG_NOTENOUGHDATA,
    _SG_VALIDATE_UPDIMG_SIZEMISMATCH,
    _SG_VALIDATE_UPDIMG_COMPRESSED,
    _SG_VALIDATE_UPDIMG_ONCE

} _sg_validate_error;

/* return a human readable string for an _sg_validate_error */
_SOKOL_PRIVATE const char* _sg_validate_string(_sg_validate_error err) {
    switch (err) {
        /* buffer creation validation errors */
        case _SG_VALIDATE_BUFFERDESC_CANARY:        return "sg_buffer_desc not initialized";
        case _SG_VALIDATE_BUFFERDESC_SIZE:          return "sg_buffer_desc.size cannot be 0";
        case _SG_VALIDATE_BUFFERDESC_CONTENT:       return "immutable buffers must be initialized with content (sg_buffer_desc.content)";
        case _SG_VALIDATE_BUFFERDESC_NO_CONTENT:    return "dynamic/stream usage buffers cannot be initialized with content";

        /* image creation validation errros */
        case _SG_VALIDATE_IMAGEDESC_CANARY:             return "sg_image_desc not initialized";
        case _SG_VALIDATE_IMAGEDESC_WIDTH:              return "sg_image_desc.width must be > 0";
        case _SG_VALIDATE_IMAGEDESC_HEIGHT:             return "sg_image_desc.height must be > 0";
        case _SG_VALIDATE_IMAGEDESC_RT_PIXELFORMAT:     return "invalid pixel format for render-target image";
        case _SG_VALIDATE_IMAGEDESC_NONRT_PIXELFORMAT:  return "invalid pixel format for non-render-target image";
        case _SG_VALIDATE_IMAGEDESC_MSAA_BUT_NO_RT:     return "non-render-target images cannot be multisampled";
        case _SG_VALIDATE_IMAGEDESC_NO_MSAA_RT_SUPPORT: return "MSAA render targets not supported (SG_FEATURE_MSAA_RENDER_TARGETS)";
        case _SG_VALIDATE_IMAGEDESC_RT_IMMUTABLE:       return "render target images must be SG_USAGE_IMMUTABLE";
        case _SG_VALIDATE_IMAGEDESC_RT_NO_CONTENT:      return "render target images cannot be initialized with content";
        case _SG_VALIDATE_IMAGEDESC_CONTENT:            return "missing or invalid content for immutable image";
        case _SG_VALIDATE_IMAGEDESC_NO_CONTENT:         return "dynamic/stream usage images cannot be initialized with content";

        /* shader creation */
        case _SG_VALIDATE_SHADERDESC_CANARY:                return "sg_shader_desc not initialized";
        case _SG_VALIDATE_SHADERDESC_SOURCE:                return "shader source code required";
        case _SG_VALIDATE_SHADERDESC_BYTECODE:              return "shader byte code required";
        case _SG_VALIDATE_SHADERDESC_SOURCE_OR_BYTECODE:    return "shader source or byte code required";
        case _SG_VALIDATE_SHADERDESC_NO_BYTECODE_SIZE:      return "shader byte code length (in bytes) required";
        case _SG_VALIDATE_SHADERDESC_NO_CONT_UBS:           return "shader uniform blocks must occupy continuous slots";
        case _SG_VALIDATE_SHADERDESC_NO_CONT_UB_MEMBERS:    return "uniform block members must occupy continuous slots";
        case _SG_VALIDATE_SHADERDESC_NO_UB_MEMBERS:         return "GL backend requires uniform block member declarations";
        case _SG_VALIDATE_SHADERDESC_UB_MEMBER_NAME:        return "uniform block member name missing";
        case _SG_VALIDATE_SHADERDESC_UB_SIZE_MISMATCH:      return "size of uniform block members doesn't match uniform block size";
        case _SG_VALIDATE_SHADERDESC_NO_CONT_IMGS:          return "shader images must occupy continuous slots";
        case _SG_VALIDATE_SHADERDESC_IMG_NAME:              return "GL backend requires uniform block member names";
        
        /* pipeline creation */
        case _SG_VALIDATE_PIPELINEDESC_CANARY:          return "sg_pipeline_desc not initialized";
        case _SG_VALIDATE_PIPELINEDESC_SHADER:          return "sg_pipeline_desc.shader missing or invalid";
        case _SG_VALIDATE_PIPELINEDESC_NO_LAYOUT:       return "no vertex layout in sg_pipeline_desc.vertex_layouts[0]";
        case _SG_VALIDATE_PIPELINEDESC_NO_CONT_LAYOUTS: return "vertex layouts must occupy continuous slots";
        case _SG_VALIDATE_PIPELINEDESC_LAYOUT_STRIDE4:  return "vertex layout stride must be multiple of 4";
        case _SG_VALIDATE_PIPELINEDESC_NO_ATTRS:        return "sg_pipeline_desc.vertex_layout has stride but no attrs";
        case _SG_VALIDATE_PIPELINEDESC_TOO_MANY_ATTRS:  return "too many vertex attributes across all layouts";
        case _SG_VALIDATE_PIPELINEDESC_NO_CONT_ATTRS:   return "vertex attributes must occupy continuous slots";
        case _SG_VALIDATE_PIPELINEDESC_ATTR_NAME:       return "GLES2/WebGL vertex layouts must have attribute names";
        case _SG_VALIDATE_PIPELINEDESC_ATTR_SEMANTICS:  return "D3D11 vertex layouts must have attribute semantics (sem_name and sem_index)";
        case _SG_VALIDATE_PIPELINEDESC_ATTR_OVERFLOW:   return "vertex attribute offset+sizeof(format) is greater than vertex layout stride";

        /* pass creation */
        case _SG_VALIDATE_PASSDESC_CANARY:                  return "sg_pass_desc not initialized";
        case _SG_VALIDATE_PASSDESC_NO_COLOR_ATTS:           return "sg_pass_desc.color_attachments[0] must be valid";
        case _SG_VALIDATE_PASSDESC_NO_CONT_COLOR_ATTS:      return "color attachments must occupy continuous slots";
        case _SG_VALIDATE_PASSDESC_IMAGE:                   return "pass attachment image is not valid";
        case _SG_VALIDATE_PASSDESC_MIPLEVEL:                return "pass attachment mip level is bigger than image has mipmaps";
        case _SG_VALIDATE_PASSDESC_FACE:                    return "pass attachment image is cubemap, but face index is too big";
        case _SG_VALIDATE_PASSDESC_LAYER:                   return "pass attachment image is array texture, but layer index is too big";
        case _SG_VALIDATE_PASSDESC_SLICE:                   return "pass attachment image is 3d texture, but slice value is too big";
        case _SG_VALIDATE_PASSDESC_IMAGE_NO_RT:             return "pass attachment image must be render targets";
        case _SG_VALIDATE_PASSDESC_COLOR_PIXELFORMATS:      return "all pass color attachment images must have the same pixel format";
        case _SG_VALIDATE_PASSDESC_COLOR_INV_PIXELFORMAT:   return "pass color-attachment images must have a renderable pixel format";
        case _SG_VALIDATE_PASSDESC_DEPTH_INV_PIXELFORMAT:   return "pass depth-attachment image must have depth pixel format";
        case _SG_VALIDATE_PASSDESC_IMAGE_SIZES:             return "all pass attachments must have the same size";
        case _SG_VALIDATE_PASSDESC_IMAGE_SAMPLE_COUNTS:     return "all pass attachments must have the same sample count";

        /* sg_begin_pass */
        case _SG_VALIDATE_BEGINPASS_PASS:       return "sg_begin_pass: pass must be valid";
        case _SG_VALIDATE_BEGINPASS_IMAGE:      return "sg_begin_pass: one or more attachment images are not valid";

        /* sg_apply_draw_state */
        case _SG_VALIDATE_ADS_PIP:          return "sg_apply_draw_state: pipeline object required";
        case _SG_VALIDATE_ADS_VBS:          return "sg_apply_draw_state: number of vertex buffers doesn't match number of pipeline vertex layouts";
        case _SG_VALIDATE_ADS_VB_TYPE:      return "sg_apply_draw_state: buffer in vertex buffer slot is not a SG_BUFFERTYPE_VERTEXBUFFER";
        case _SG_VALIDATE_ADS_NO_IB:        return "sg_apply_draw_state: pipeline object defines indexed rendering, but no index buffer provided";
        case _SG_VALIDATE_ADS_IB:           return "sg_apply_draw_state: pipeline object defines non-indexed rendering, but index buffer provided";
        case _SG_VALIDATE_ADS_IB_TYPE:      return "sg_apply_draw_state: buffer in index buffer slot is not a SG_BUFFERTYPE_INDEXBUFFER";
        case _SG_VALIDATE_ADS_VS_IMGS:      return "sg_apply_draw_state: vertex shader image count doesn't match sg_shader_desc";
        case _SG_VALIDATE_ADS_VS_IMG_TYPES: return "sg_apply_draw_state: one or more vertex shader image types don't match sg_shader_desc";
        case _SG_VALIDATE_ADS_FS_IMGS:      return "sg_apply_draw_state: fragment shader image count doesn't match sg_shader_desc";
        case _SG_VALIDATE_ADS_FS_IMG_TYPES: return "sg_apply_draw_state: one or more fragment shader image types don't match sg_shader_desc";
        case _SG_VALIDATE_ADS_ATT_COUNT:    return "sg_apply_draw_state: color_attachment_count in pipeline doesn't match number of pass color attachments";
        case _SG_VALIDATE_ADS_COLOR_FORMAT: return "sg_apply_draw_state: color_format in pipeline doesn't match pass color attachment pixel format";
        case _SG_VALIDATE_ADS_DEPTH_FORMAT: return "sg_apply_draw_state: depth_format in pipeline doesn't match pass depth attachment pixel format";
        case _SG_VALIDATE_ADS_SAMPLE_COUNT: return "sg_apply_draw_state: MSAA sample count in pipeline doesn't match render pass attachment sample count";

        /* sg_apply_uniform_block */
        case _SG_VALIDATE_AUB_NO_PIPELINE:      return "sg_apply_uniform_block: must be called after sg_apply_draw_state()";
        case _SG_VALIDATE_AUB_NO_UB_AT_SLOT:    return "sg_apply_uniform_block: no uniform block declaration at this shader stage UB slot";
        case _SG_VALIDATE_AUB_SIZE:             return "sg_apply_uniform_block: data size exceeds declared uniform block size";

        /* sg_update_buffer */
        case _SG_VALIDATE_UPDBUF_USAGE:     return "sg_update_buffer: cannot update immutable buffer";
        case _SG_VALIDATE_UPDBUF_SIZE:      return "sg_update_buffer: update size is bigger than buffer size";
        case _SG_VALIDATE_UPDBUF_ONCE:      return "sg_update_buffer: only one update allowed per buffer and frame";

        /* sg_update_image */
        case _SG_VALIDATE_UPDIMG_USAGE:         return "sg_update_image: cannot update immutable image";
        case _SG_VALIDATE_UPDIMG_NOTENOUGHDATA: return "sg_update_image: not enough subimage data provided";
        case _SG_VALIDATE_UPDIMG_SIZEMISMATCH:  return "sg_update_image: subimage data size mismatch";
        case _SG_VALIDATE_UPDIMG_COMPRESSED:    return "sg_update_image: cannot update images with compressed format";
        case _SG_VALIDATE_UPDIMG_ONCE:          return "sg_update_image: only one update allowed per image and frame";

        default: return "unknown validation error";
    }
}
#endif /* defined(SOKOL_DEBUG) */

/*-- generic backend state ---------------------------------------------------*/ 
typedef struct {
    _sg_pools pools;
    bool valid;
    uint32_t frame_index;
    sg_pass cur_pass;
    sg_pipeline cur_pipeline;
    bool pass_valid;
    bool next_draw_valid;
    #if defined(SOKOL_DEBUG)
    _sg_validate_error validate_error;
    #endif
} _sg_state;
static _sg_state _sg;

/*-- validation checks -------------------------------------------------------*/
#if defined(SOKOL_DEBUG)
_SOKOL_PRIVATE void _sg_validate_begin() {
    _sg.validate_error = _SG_VALIDATE_SUCCESS;
}

_SOKOL_PRIVATE void _sg_validate(bool cond, _sg_validate_error err) {
    if (!cond) {
        _sg.validate_error = err;
        SOKOL_LOG(_sg_validate_string(err));
    }
}

_SOKOL_PRIVATE bool _sg_validate_end() {
    if (_sg.validate_error != _SG_VALIDATE_SUCCESS) {
        #if !defined(SOKOL_VALIDATE_NON_FATAL)
            SOKOL_LOG("^^^^  VALIDATION FAILED, TERMINATING ^^^^");
            SOKOL_ASSERT(false);
        #endif
        return false;
    }
    else {
        return true;
    }
}
#endif

_SOKOL_PRIVATE bool _sg_validate_buffer_desc(const sg_buffer_desc* desc) {
    #if !defined(SOKOL_DEBUG)
        return true;
    #else
        SOKOL_ASSERT(desc);
        SOKOL_VALIDATE_BEGIN();
        SOKOL_VALIDATE(desc->_start_canary == 0, _SG_VALIDATE_BUFFERDESC_CANARY);
        SOKOL_VALIDATE(desc->_end_canary == 0, _SG_VALIDATE_BUFFERDESC_CANARY);
        SOKOL_VALIDATE(desc->size > 0, _SG_VALIDATE_BUFFERDESC_SIZE);
        bool ext = (0 != desc->gl_buffers[0]) || (0 != desc->mtl_buffers[0]) || (0 != desc->d3d11_buffer);
        if (!ext && (_sg_def(desc->usage, SG_USAGE_IMMUTABLE) == SG_USAGE_IMMUTABLE)) {
            SOKOL_VALIDATE(0 != desc->content, _SG_VALIDATE_BUFFERDESC_CONTENT);
        }
        else {
            SOKOL_VALIDATE(0 == desc->content, _SG_VALIDATE_BUFFERDESC_NO_CONTENT);
        }
        return SOKOL_VALIDATE_END();
    #endif
}

_SOKOL_PRIVATE bool _sg_validate_image_desc(const sg_image_desc* desc) {
    #if !defined(SOKOL_DEBUG)
        return true;
    #else
        SOKOL_ASSERT(desc);
        SOKOL_VALIDATE_BEGIN();
        SOKOL_VALIDATE(desc->_start_canary == 0, _SG_VALIDATE_IMAGEDESC_CANARY);
        SOKOL_VALIDATE(desc->_end_canary == 0, _SG_VALIDATE_IMAGEDESC_CANARY);
        SOKOL_VALIDATE(desc->width > 0, _SG_VALIDATE_IMAGEDESC_WIDTH);
        SOKOL_VALIDATE(desc->height > 0, _SG_VALIDATE_IMAGEDESC_HEIGHT);
        const sg_pixel_format fmt = _sg_def(desc->pixel_format, SG_PIXELFORMAT_RGBA8);
        const sg_usage usage = _sg_def(desc->usage, SG_USAGE_IMMUTABLE);
        const bool ext = (0 != desc->gl_textures[0]) || (0 != desc->mtl_textures[0]) || (0 != desc->d3d11_texture);
        if (desc->render_target) {
            if (desc->sample_count > 1) {
                SOKOL_VALIDATE(_sg_query_feature(SG_FEATURE_MSAA_RENDER_TARGETS), _SG_VALIDATE_IMAGEDESC_NO_MSAA_RT_SUPPORT);
            }
            const bool valid_color_fmt = _sg_is_valid_rendertarget_color_format(fmt);
            const bool valid_depth_fmt = _sg_is_valid_rendertarget_depth_format(fmt);
            SOKOL_VALIDATE(valid_color_fmt || valid_depth_fmt, _SG_VALIDATE_IMAGEDESC_RT_PIXELFORMAT);
            SOKOL_VALIDATE(usage == SG_USAGE_IMMUTABLE, _SG_VALIDATE_IMAGEDESC_RT_IMMUTABLE);
            SOKOL_VALIDATE(desc->content.subimage[0][0].ptr==0, _SG_VALIDATE_IMAGEDESC_RT_NO_CONTENT);
        }
        else {
            SOKOL_VALIDATE(desc->sample_count <= 1, _SG_VALIDATE_IMAGEDESC_MSAA_BUT_NO_RT);
            const bool valid_nonrt_fmt = !_sg_is_valid_rendertarget_depth_format(fmt);
            SOKOL_VALIDATE(valid_nonrt_fmt, _SG_VALIDATE_IMAGEDESC_NONRT_PIXELFORMAT);
            /* FIXME: should use the same "expected size" computation as in _sg_validate_update_image() here */
            if (!ext && (usage == SG_USAGE_IMMUTABLE)) {
                const int num_faces = _sg_def(desc->type, SG_IMAGETYPE_2D)==SG_IMAGETYPE_CUBE ? 6:1;
                const int num_mips = _sg_def(desc->num_mipmaps, 1);
                for (int face_index = 0; face_index < num_faces; face_index++) {
                    for (int mip_index = 0; mip_index < num_mips; mip_index++) {
                        const bool has_data = desc->content.subimage[face_index][mip_index].ptr != 0;
                        const bool has_size = desc->content.subimage[face_index][mip_index].size > 0;
                        SOKOL_VALIDATE(has_data && has_size, _SG_VALIDATE_IMAGEDESC_CONTENT);
                    }
                }
            }
            else {
                for (int face_index = 0; face_index < SG_CUBEFACE_NUM; face_index++) {
                    for (int mip_index = 0; mip_index < SG_MAX_MIPMAPS; mip_index++) {
                        const bool no_data = 0 == desc->content.subimage[face_index][mip_index].ptr;
                        const bool no_size = 0 == desc->content.subimage[face_index][mip_index].size;
                        SOKOL_VALIDATE(no_data && no_size, _SG_VALIDATE_IMAGEDESC_NO_CONTENT);
                    }
                }
            }
        }
        return SOKOL_VALIDATE_END();
    #endif
}

_SOKOL_PRIVATE bool _sg_validate_shader_desc(const sg_shader_desc* desc) {
    #if !defined(SOKOL_DEBUG)
        return true;
    #else
        SOKOL_ASSERT(desc);
        SOKOL_VALIDATE_BEGIN();
        SOKOL_VALIDATE(desc->_start_canary == 0, _SG_VALIDATE_SHADERDESC_CANARY);
        SOKOL_VALIDATE(desc->_end_canary == 0, _SG_VALIDATE_SHADERDESC_CANARY);
        #if defined(SOKOL_GLCORE33) || defined(SOKOL_GLES2) || defined(SOKOL_GLES3)
            /* on GL, must provide shader source code */
            SOKOL_VALIDATE(0 != desc->vs.source, _SG_VALIDATE_SHADERDESC_SOURCE);
            SOKOL_VALIDATE(0 != desc->fs.source, _SG_VALIDATE_SHADERDESC_SOURCE);
        #elif defined(SOKOL_METAL_MACOS) || defined(SOKOL_METAL_IOS) || defined(SOKOL_D3D11_SHADER_COMPILER)
            /* on Metal or D3D with shader compiler, must provide shader source code or byte code */
            SOKOL_VALIDATE((0 != desc->vs.source)||(0 != desc->vs.byte_code), _SG_VALIDATE_SHADERDESC_SOURCE_OR_BYTECODE);
            SOKOL_VALIDATE((0 != desc->fs.source)||(0 != desc->fs.byte_code), _SG_VALIDATE_SHADERDESC_SOURCE_OR_BYTECODE);
        #else
            /* on D3D11 without shader compiler, must provide byte code */
            SOKOL_VALIDATE(0 != desc->vs.byte_code, _SG_VALIDATE_SHADERDESC_BYTECODE);
            SOKOL_VALIDATE(0 != desc->fs.byte_code, _SG_VALIDATE_SHADERDESC_BYTECODE); 
        #endif
        /* if shader byte code, the size must also be provided */
        if (0 != desc->vs.byte_code) {
            SOKOL_VALIDATE(desc->vs.byte_code_size > 0, _SG_VALIDATE_SHADERDESC_NO_BYTECODE_SIZE);
        }
        if (0 != desc->fs.byte_code) {
            SOKOL_VALIDATE(desc->fs.byte_code_size > 0, _SG_VALIDATE_SHADERDESC_NO_BYTECODE_SIZE);
        }
        for (int stage_index = 0; stage_index < SG_NUM_SHADER_STAGES; stage_index++) {
            const sg_shader_stage_desc* stage_desc = (stage_index == 0)? &desc->vs : &desc->fs;
            bool uniform_blocks_continuous = true;
            for (int ub_index = 0; ub_index < SG_MAX_SHADERSTAGE_UBS; ub_index++) {
                const sg_shader_uniform_block_desc* ub_desc = &stage_desc->uniform_blocks[ub_index];
                if (ub_desc->size > 0) {
                    SOKOL_VALIDATE(uniform_blocks_continuous, _SG_VALIDATE_SHADERDESC_NO_CONT_UBS);
                    bool uniforms_continuous = true;
                    int uniform_offset = 0;
                    int num_uniforms = 0;
                    for (int u_index = 0; u_index < SG_MAX_UB_MEMBERS; u_index++) {
                        const sg_shader_uniform_desc* u_desc = &ub_desc->uniforms[u_index];
                        if (u_desc->type != SG_UNIFORMTYPE_INVALID) {
                            SOKOL_VALIDATE(uniforms_continuous, _SG_VALIDATE_SHADERDESC_NO_CONT_UB_MEMBERS);
                            #if defined(SOKOL_GLES2)
                            SOKOL_VALIDATE(u_desc->name, _SG_VALIDATE_SHADERDESC_UB_MEMBER_NAME);
                            #endif
                            const int array_count = _sg_def(u_desc->array_count, 1);
                            uniform_offset += _sg_uniform_size(u_desc->type, array_count);
                            num_uniforms++;
                        }
                        else {
                            uniforms_continuous = false;
                        }
                    }
                    #if defined(SOKOL_GLCORE33) || defined(SOKOL_GLES2) || defined(SOKOL_GLES3)
                    SOKOL_VALIDATE(uniform_offset == ub_desc->size, _SG_VALIDATE_SHADERDESC_UB_SIZE_MISMATCH);
                    SOKOL_VALIDATE(num_uniforms > 0, _SG_VALIDATE_SHADERDESC_NO_UB_MEMBERS);
                    #endif
                }
                else {
                    uniform_blocks_continuous = false;
                }
            }
            bool images_continuous = true;
            for (int img_index = 0; img_index < SG_MAX_SHADERSTAGE_IMAGES; img_index++) {
                const sg_shader_image_desc* img_desc = &stage_desc->images[img_index];
                if (img_desc->type != _SG_IMAGETYPE_DEFAULT) {
                    SOKOL_VALIDATE(images_continuous, _SG_VALIDATE_SHADERDESC_NO_CONT_IMGS);
                    #if defined(SOKOL_GLES2)
                    SOKOL_VALIDATE(img_desc->name, _SG_VALIDATE_SHADERDESC_IMG_NAME);
                    #endif
                } 
                else {
                    images_continuous = false;
                }
            }
        }
        return SOKOL_VALIDATE_END();
    #endif
}

_SOKOL_PRIVATE bool _sg_validate_pipeline_desc(const sg_pipeline_desc* desc) {
    #if !defined(SOKOL_DEBUG)
        return true;
    #else
        SOKOL_ASSERT(desc);
        SOKOL_VALIDATE_BEGIN();
        SOKOL_VALIDATE(desc->_start_canary == 0, _SG_VALIDATE_PIPELINEDESC_CANARY);
        SOKOL_VALIDATE(desc->_end_canary == 0, _SG_VALIDATE_PIPELINEDESC_CANARY);
        SOKOL_VALIDATE(desc->shader.id != SG_INVALID_ID, _SG_VALIDATE_PIPELINEDESC_SHADER);
        const _sg_shader* shd = _sg_lookup_shader(&_sg.pools, desc->shader.id);
        SOKOL_VALIDATE(shd && shd->slot.state == SG_RESOURCESTATE_VALID, _SG_VALIDATE_PIPELINEDESC_SHADER);
        SOKOL_VALIDATE(desc->vertex_layouts[0].stride > 0, _SG_VALIDATE_PIPELINEDESC_NO_LAYOUT);
        bool layouts_continuous = true;
        int num_attrs = 0;
        for (int layout_index = 0; layout_index < SG_MAX_SHADERSTAGE_BUFFERS; layout_index++) {
            const sg_vertex_layout_desc* layout_desc = &desc->vertex_layouts[layout_index];
            if (layout_desc->stride == 0) {
                layouts_continuous = false;
                continue;
            }
            SOKOL_VALIDATE((layout_desc->stride & 3) == 0, _SG_VALIDATE_PIPELINEDESC_LAYOUT_STRIDE4);
            SOKOL_VALIDATE(layouts_continuous, _SG_VALIDATE_PIPELINEDESC_NO_CONT_LAYOUTS);
            bool attrs_continuous = true;
            int num_layout_attrs = 0;
            for (int attr_index = 0; attr_index < SG_MAX_VERTEX_ATTRIBUTES; attr_index++) {
                const sg_vertex_attr_desc* attr_desc = &layout_desc->attrs[attr_index];
                if (attr_desc->format == SG_VERTEXFORMAT_INVALID) {
                    attrs_continuous = false;
                    continue;
                }
                SOKOL_VALIDATE(attrs_continuous, _SG_VALIDATE_PIPELINEDESC_NO_CONT_ATTRS);
                const bool attr_overflow = (attr_desc->offset + _sg_vertexformat_bytesize(attr_desc->format)) > layout_desc->stride;
                SOKOL_VALIDATE(!attr_overflow, _SG_VALIDATE_PIPELINEDESC_ATTR_OVERFLOW);
                #if defined(SOKOL_GLES2)
                /* on GLES2, vertex attribute names must be provided */
                SOKOL_VALIDATE(attr_desc->name, _SG_VALIDATE_PIPELINEDESC_ATTR_NAME);
                #elif defined(SOKOL_D3D11)
                /* on D3D11, semantic names (and semantic indices) must be provided */
                SOKOL_VALIDATE(attr_desc->sem_name, _SG_VALIDATE_PIPELINEDESC_ATTR_SEMANTICS);
                #endif
                num_layout_attrs++;
                num_attrs++;
            }
            SOKOL_VALIDATE(num_layout_attrs > 0, _SG_VALIDATE_PIPELINEDESC_NO_ATTRS);
        }
        SOKOL_VALIDATE(num_attrs <= SG_MAX_VERTEX_ATTRIBUTES, _SG_VALIDATE_PIPELINEDESC_TOO_MANY_ATTRS);
        return SOKOL_VALIDATE_END();
    #endif
}

_SOKOL_PRIVATE bool _sg_validate_pass_desc(const sg_pass_desc* desc) {
    #if !defined(SOKOL_DEBUG)
        return true;
    #else
        SOKOL_ASSERT(desc);
        SOKOL_VALIDATE_BEGIN();
        SOKOL_VALIDATE(desc->_start_canary == 0, _SG_VALIDATE_PASSDESC_CANARY);
        SOKOL_VALIDATE(desc->_end_canary == 0, _SG_VALIDATE_PASSDESC_CANARY);
        bool atts_cont = true;
        sg_pixel_format color_fmt = SG_PIXELFORMAT_NONE;
        int width = -1, height = -1, sample_count = -1;
        for (int att_index = 0; att_index < SG_MAX_COLOR_ATTACHMENTS; att_index++) {
            const sg_attachment_desc* att = &desc->color_attachments[att_index];
            if (att->image.id == SG_INVALID_ID) {
                SOKOL_VALIDATE(att_index > 0, _SG_VALIDATE_PASSDESC_NO_COLOR_ATTS);
                atts_cont = false;
                continue;
            }
            SOKOL_VALIDATE(atts_cont, _SG_VALIDATE_PASSDESC_NO_CONT_COLOR_ATTS);
            const _sg_image* img = _sg_lookup_image(&_sg.pools, att->image.id);
            SOKOL_VALIDATE(img && img->slot.state == SG_RESOURCESTATE_VALID, _SG_VALIDATE_PASSDESC_IMAGE);
            SOKOL_VALIDATE(att->mip_level < img->num_mipmaps, _SG_VALIDATE_PASSDESC_MIPLEVEL);
            if (img->type == SG_IMAGETYPE_CUBE) {
                SOKOL_VALIDATE(att->face < 6, _SG_VALIDATE_PASSDESC_FACE);
            }
            else if (img->type == SG_IMAGETYPE_ARRAY) {
                SOKOL_VALIDATE(att->layer < img->depth, _SG_VALIDATE_PASSDESC_LAYER);
            }
            else if (img->type == SG_IMAGETYPE_3D) {
                SOKOL_VALIDATE(att->slice < img->depth, _SG_VALIDATE_PASSDESC_SLICE);
            }
            SOKOL_VALIDATE(img->render_target, _SG_VALIDATE_PASSDESC_IMAGE_NO_RT);
            if (att_index == 0) {
                color_fmt = img->pixel_format;
                width = img->width >> att->mip_level;
                height = img->height >> att->mip_level;
                sample_count = img->sample_count;
            }
            else {
                SOKOL_VALIDATE(img->pixel_format == color_fmt, _SG_VALIDATE_PASSDESC_COLOR_PIXELFORMATS);
                SOKOL_VALIDATE(width == img->width >> att->mip_level, _SG_VALIDATE_PASSDESC_IMAGE_SIZES);
                SOKOL_VALIDATE(height == img->height >> att->mip_level, _SG_VALIDATE_PASSDESC_IMAGE_SIZES);
                SOKOL_VALIDATE(sample_count == img->sample_count, _SG_VALIDATE_PASSDESC_IMAGE_SAMPLE_COUNTS);
            }
            SOKOL_VALIDATE(_sg_is_valid_rendertarget_color_format(img->pixel_format), _SG_VALIDATE_PASSDESC_COLOR_INV_PIXELFORMAT);
        }
        if (desc->depth_stencil_attachment.image.id != SG_INVALID_ID) {
            const sg_attachment_desc* att = &desc->depth_stencil_attachment;
            const _sg_image* img = _sg_lookup_image(&_sg.pools, att->image.id);
            SOKOL_VALIDATE(img && img->slot.state == SG_RESOURCESTATE_VALID, _SG_VALIDATE_PASSDESC_IMAGE);
            SOKOL_VALIDATE(att->mip_level < img->num_mipmaps, _SG_VALIDATE_PASSDESC_MIPLEVEL);
            if (img->type == SG_IMAGETYPE_CUBE) {
                SOKOL_VALIDATE(att->face < 6, _SG_VALIDATE_PASSDESC_FACE);
            }
            else if (img->type == SG_IMAGETYPE_ARRAY) {
                SOKOL_VALIDATE(att->layer < img->depth, _SG_VALIDATE_PASSDESC_LAYER);
            }
            else if (img->type == SG_IMAGETYPE_3D) {
                SOKOL_VALIDATE(att->slice < img->depth, _SG_VALIDATE_PASSDESC_SLICE);
            }
            SOKOL_VALIDATE(img->render_target, _SG_VALIDATE_PASSDESC_IMAGE_NO_RT);
            SOKOL_VALIDATE(width == img->width >> att->mip_level, _SG_VALIDATE_PASSDESC_IMAGE_SIZES);
            SOKOL_VALIDATE(height == img->height >> att->mip_level, _SG_VALIDATE_PASSDESC_IMAGE_SIZES);
            SOKOL_VALIDATE(sample_count == img->sample_count, _SG_VALIDATE_PASSDESC_IMAGE_SAMPLE_COUNTS);
            SOKOL_VALIDATE(_sg_is_valid_rendertarget_depth_format(img->pixel_format), _SG_VALIDATE_PASSDESC_DEPTH_INV_PIXELFORMAT);
        }
        return SOKOL_VALIDATE_END();
    #endif
}

_SOKOL_PRIVATE bool _sg_validate_begin_pass(_sg_pass* pass) {
    #if !defined(SOKOL_DEBUG)
        return true;
    #else
        SOKOL_VALIDATE_BEGIN();
        SOKOL_VALIDATE(pass->slot.state == SG_RESOURCESTATE_VALID, _SG_VALIDATE_BEGINPASS_PASS);
        for (int i = 0; i < SG_MAX_COLOR_ATTACHMENTS; i++) {
            const _sg_attachment* att = &pass->color_atts[i];
            if (att->image) {
                SOKOL_VALIDATE(att->image->slot.state == SG_RESOURCESTATE_VALID, _SG_VALIDATE_BEGINPASS_IMAGE);
                SOKOL_VALIDATE(att->image->slot.id == att->image_id.id, _SG_VALIDATE_BEGINPASS_IMAGE);
            }
        }
        if (pass->ds_att.image) {
            const _sg_attachment* att = &pass->ds_att;
            SOKOL_VALIDATE(att->image->slot.state == SG_RESOURCESTATE_VALID, _SG_VALIDATE_BEGINPASS_IMAGE);
            SOKOL_VALIDATE(att->image->slot.id == att->image_id.id, _SG_VALIDATE_BEGINPASS_IMAGE);
        }
        return SOKOL_VALIDATE_END();
    #endif
}

_SOKOL_PRIVATE bool _sg_validate_draw_state(const sg_draw_state* ds) {
    #if !defined(SOKOL_DEBUG)
        return true;
    #else
        SOKOL_VALIDATE_BEGIN();
        /* has pipeline and pipeline still exists */
        SOKOL_VALIDATE(ds->pipeline.id != SG_INVALID_ID, _SG_VALIDATE_ADS_PIP);
        const _sg_pipeline* pip = _sg_lookup_pipeline(&_sg.pools, ds->pipeline.id);
        if (!pip) {
            /* cannot continue with validation without pipeline object */
            return SOKOL_VALIDATE_END();
        }
        SOKOL_ASSERT(pip->shader);

        /* has expected vertex buffers, and vertex buffers still exist */
        for (int i = 0; i < SG_MAX_SHADERSTAGE_BUFFERS; i++) {
            if (ds->vertex_buffers[i].id != SG_INVALID_ID) {
                SOKOL_VALIDATE(pip->vertex_layout_valid[i], _SG_VALIDATE_ADS_VBS);
                /* buffers in vertex-buffer-slots must be of type SG_BUFFERTYPE_VERTEXBUFFER */
                const _sg_buffer* buf = _sg_lookup_buffer(&_sg.pools, ds->vertex_buffers[i].id);
                SOKOL_ASSERT(buf);
                if (buf->slot.state == SG_RESOURCESTATE_VALID) {
                    SOKOL_VALIDATE(SG_BUFFERTYPE_VERTEXBUFFER == buf->type, _SG_VALIDATE_ADS_VB_TYPE);
                }
            }
            else {
                /* vertex buffer provided in a slot which has no vertex layout in pipeline */
                SOKOL_VALIDATE(!pip->vertex_layout_valid[i], _SG_VALIDATE_ADS_VBS);
            }
        }

        /* index buffer expected or not, and index buffer still exists */
        if (pip->index_type == SG_INDEXTYPE_NONE) {
            /* pipeline defines non-indexed rendering, but index buffer provided */
            SOKOL_VALIDATE(ds->index_buffer.id == SG_INVALID_ID, _SG_VALIDATE_ADS_IB);
        }
        else {
            /* pipeline defines indexed rendering, but no index buffer provided */
            SOKOL_VALIDATE(ds->index_buffer.id != SG_INVALID_ID, _SG_VALIDATE_ADS_NO_IB);
        }
        if (ds->index_buffer.id != SG_INVALID_ID) {
            /* buffer in index-buffer-slot must be of type SG_BUFFERTYPE_VERTEXBUFFER */
            const _sg_buffer* buf = _sg_lookup_buffer(&_sg.pools, ds->index_buffer.id);
            SOKOL_ASSERT(buf);
            if (buf->slot.state == SG_RESOURCESTATE_VALID) {
                SOKOL_VALIDATE(SG_BUFFERTYPE_INDEXBUFFER == buf->type, _SG_VALIDATE_ADS_VB_TYPE);
            }
        }

        /* has expected vertex shader images */
        for (int i = 0; i < SG_MAX_SHADERSTAGE_IMAGES; i++) {
            _sg_shader_stage* stage = &pip->shader->stage[SG_SHADERSTAGE_VS];
            if (ds->vs_images[i].id != SG_INVALID_ID) {
                SOKOL_VALIDATE(i < stage->num_images, _SG_VALIDATE_ADS_VS_IMGS);
                const _sg_image* img = _sg_lookup_image(&_sg.pools, ds->vs_images[i].id);
                SOKOL_ASSERT(img);
                if (img->slot.state == SG_RESOURCESTATE_VALID) {
                    SOKOL_VALIDATE(img->type == stage->images[i].type, _SG_VALIDATE_ADS_VS_IMG_TYPES);
                }
            }
            else {
                SOKOL_VALIDATE(i >= stage->num_images, _SG_VALIDATE_ADS_VS_IMGS);
            }
        }

        /* has expected fragment shader images */
        for (int i = 0; i < SG_MAX_SHADERSTAGE_IMAGES; i++) {
            _sg_shader_stage* stage = &pip->shader->stage[SG_SHADERSTAGE_FS];
            if (ds->fs_images[i].id != SG_INVALID_ID) {
                SOKOL_VALIDATE(i < stage->num_images, _SG_VALIDATE_ADS_FS_IMGS);
                const _sg_image* img = _sg_lookup_image(&_sg.pools, ds->fs_images[i].id);
                SOKOL_ASSERT(img);
                if (img->slot.state == SG_RESOURCESTATE_VALID) {
                    SOKOL_VALIDATE(img->type == stage->images[i].type, _SG_VALIDATE_ADS_FS_IMG_TYPES);
                }
            }
            else {
                SOKOL_VALIDATE(i >= stage->num_images, _SG_VALIDATE_ADS_FS_IMGS);
            }
        }

        /* check that pipeline attributes match current pass attributes */
        const _sg_pass* pass = _sg_lookup_pass(&_sg.pools, _sg.cur_pass.id);
        if (pass) {
            /* an offscreen pass */
            SOKOL_VALIDATE(pip->color_attachment_count == pass->num_color_atts, _SG_VALIDATE_ADS_ATT_COUNT);
            SOKOL_VALIDATE(pip->color_format == pass->color_atts[0].image->pixel_format, _SG_VALIDATE_ADS_COLOR_FORMAT);
            SOKOL_VALIDATE(pip->sample_count == pass->color_atts[0].image->sample_count, _SG_VALIDATE_ADS_SAMPLE_COUNT);
            if (pass->ds_att.image) {
                SOKOL_VALIDATE(pip->depth_format == pass->ds_att.image->pixel_format, _SG_VALIDATE_ADS_DEPTH_FORMAT);
            }
            else {
                SOKOL_VALIDATE(pip->depth_format == SG_PIXELFORMAT_NONE, _SG_VALIDATE_ADS_DEPTH_FORMAT);
            }
        }
        else {
            /* default pass */
            SOKOL_VALIDATE(pip->color_attachment_count == 1, _SG_VALIDATE_ADS_ATT_COUNT);
            SOKOL_VALIDATE(pip->color_format == SG_PIXELFORMAT_RGBA8, _SG_VALIDATE_ADS_COLOR_FORMAT);
            SOKOL_VALIDATE(pip->depth_format == SG_PIXELFORMAT_DEPTHSTENCIL, _SG_VALIDATE_ADS_DEPTH_FORMAT);
            /* FIXME: hmm, we don't know if the default framebuffer is multisampled here */
        }
        return SOKOL_VALIDATE_END();
    #endif
}

_SOKOL_PRIVATE bool _sg_validate_apply_uniform_block(sg_shader_stage stage_index, int ub_index, const void* data, int num_bytes) {
    #if !defined(SOKOL_DEBUG)
        return true;
    #else
        SOKOL_ASSERT((stage_index == SG_SHADERSTAGE_VS) || (stage_index == SG_SHADERSTAGE_FS));
        SOKOL_ASSERT((ub_index >= 0) && (ub_index < SG_MAX_SHADERSTAGE_UBS));
        SOKOL_VALIDATE_BEGIN();
        SOKOL_VALIDATE(_sg.cur_pipeline.id != SG_INVALID_ID, _SG_VALIDATE_AUB_NO_PIPELINE);
        const _sg_pipeline* pip = _sg_lookup_pipeline(&_sg.pools, _sg.cur_pipeline.id);
        SOKOL_ASSERT(pip && (pip->slot.id == _sg.cur_pipeline.id));
        SOKOL_ASSERT(pip->shader && (pip->shader->slot.id == pip->shader_id.id));

        /* check that there is a uniform block at 'stage' and 'ub_index' */
        const _sg_shader_stage* stage = &pip->shader->stage[stage_index];
        SOKOL_VALIDATE(ub_index < stage->num_uniform_blocks, _SG_VALIDATE_AUB_NO_UB_AT_SLOT);

        /* check that the provided data size doesn't exceed the uniform block size */
        SOKOL_VALIDATE(num_bytes <= stage->uniform_blocks[ub_index].size, _SG_VALIDATE_AUB_SIZE);

        return SOKOL_VALIDATE_END();
    #endif
}

_SOKOL_PRIVATE bool _sg_validate_update_buffer(const _sg_buffer* buf, const void* data, int size) {
    #if !defined(SOKOL_DEBUG)
        return true;
    #else
        SOKOL_ASSERT(buf && data);
        SOKOL_VALIDATE_BEGIN();
        SOKOL_VALIDATE(buf->usage != SG_USAGE_IMMUTABLE, _SG_VALIDATE_UPDBUF_USAGE);
        SOKOL_VALIDATE(buf->size >= size, _SG_VALIDATE_UPDBUF_SIZE);
        SOKOL_VALIDATE(buf->upd_frame_index != _sg.frame_index, _SG_VALIDATE_UPDBUF_ONCE);
        return SOKOL_VALIDATE_END();
    #endif
}

_SOKOL_PRIVATE bool _sg_validate_update_image(const _sg_image* img, const sg_image_content* data) {
    #if !defined(SOKOL_DEBUG)
        return true;
    #else
        SOKOL_ASSERT(img && data);
        SOKOL_VALIDATE_BEGIN();
        SOKOL_VALIDATE(img->usage != SG_USAGE_IMMUTABLE, _SG_VALIDATE_UPDIMG_USAGE);
        SOKOL_VALIDATE(img->upd_frame_index != _sg.frame_index, _SG_VALIDATE_UPDIMG_ONCE);
        SOKOL_VALIDATE(!_sg_is_compressed_pixel_format(img->pixel_format), _SG_VALIDATE_UPDIMG_COMPRESSED);
        const int num_faces = (img->type == SG_IMAGETYPE_CUBE) ? 6 : 1;
        const int num_mips = img->num_mipmaps;
        for (int face_index = 0; face_index < num_faces; face_index++) {
            for (int mip_index = 0; mip_index < num_mips; mip_index++) {
                SOKOL_VALIDATE(0 != data->subimage[face_index][mip_index].ptr, _SG_VALIDATE_UPDIMG_NOTENOUGHDATA);
                const int mip_width = _sg_max(img->width >> mip_index, 1);
                const int mip_height = _sg_max(img->height >> mip_index, 1);
                const int bytes_per_slice = _sg_surface_pitch(img->pixel_format, mip_width, mip_height);
                const int expected_size = bytes_per_slice * img->depth;
                SOKOL_VALIDATE(data->subimage[face_index][mip_index].size == expected_size, _SG_VALIDATE_UPDIMG_SIZEMISMATCH);
            }
        }
        return SOKOL_VALIDATE_END();
    #endif
}

/*-- public API functions ----------------------------------------------------*/
void sg_setup(const sg_desc* desc) {
    SOKOL_ASSERT(desc);
    SOKOL_ASSERT((desc->_start_canary == 0) && (desc->_end_canary == 0));
    memset(&_sg, 0, sizeof(_sg));
    _sg_setup_pools(&_sg.pools, desc);
    _sg.frame_index = 1;
    _sg.next_draw_valid = false;
    _sg_setup_backend(desc);
    _sg.valid = true;
}

void sg_shutdown() {
    _sg_destroy_all_resources(&_sg.pools);
    _sg_discard_backend();
    _sg_discard_pools(&_sg.pools);
    _sg.valid = false;
}

bool sg_isvalid() {
    return _sg.valid;
}

bool sg_query_feature(sg_feature f) {
    return _sg_query_feature(f);
}

/*-- allocate resource id ----------------------------------------------------*/
sg_buffer sg_alloc_buffer() {
    sg_buffer res;
    res.id = _sg_pool_alloc_id(&_sg.pools.buffer_pool);
    if (res.id != SG_INVALID_ID) {
        _sg_buffer* buf = _sg_buffer_at(&_sg.pools, res.id);
        SOKOL_ASSERT(buf && (buf->slot.state == SG_RESOURCESTATE_INITIAL) && (buf->slot.id == SG_INVALID_ID));
        buf->slot.id = res.id;
        buf->slot.state = SG_RESOURCESTATE_ALLOC;
    }
    return res;
}

sg_image sg_alloc_image() {
    sg_image res;
    res.id = _sg_pool_alloc_id(&_sg.pools.image_pool);
    if (res.id != SG_INVALID_ID) {
        _sg_image* img = _sg_image_at(&_sg.pools, res.id);
        SOKOL_ASSERT(img && (img->slot.state == SG_RESOURCESTATE_INITIAL) && (img->slot.id == SG_INVALID_ID));
        img->slot.id = res.id;
        img->slot.state = SG_RESOURCESTATE_ALLOC;
    }
    return res;
}

sg_shader sg_alloc_shader() {
    sg_shader res;
    res.id = _sg_pool_alloc_id(&_sg.pools.shader_pool);
    if (res.id != SG_INVALID_ID) {
        _sg_shader* shd = _sg_shader_at(&_sg.pools, res.id);
        SOKOL_ASSERT(shd && (shd->slot.state == SG_RESOURCESTATE_INITIAL) && (shd->slot.id == SG_INVALID_ID));
        shd->slot.id = res.id;
        shd->slot.state = SG_RESOURCESTATE_ALLOC;
    }
    return res;
}

sg_pipeline sg_alloc_pipeline() {
    sg_pipeline res;
    res.id = _sg_pool_alloc_id(&_sg.pools.pipeline_pool);
    if (res.id != SG_INVALID_ID) {
        _sg_pipeline* pip = _sg_pipeline_at(&_sg.pools, res.id);
        SOKOL_ASSERT(pip && (pip->slot.state == SG_RESOURCESTATE_INITIAL) && (pip->slot.id == SG_INVALID_ID));
        pip->slot.id = res.id;
        pip->slot.state = SG_RESOURCESTATE_ALLOC;
    }
    return res;
}

sg_pass sg_alloc_pass() {
    sg_pass res;
    res.id = _sg_pool_alloc_id(&_sg.pools.pass_pool);
    if (res.id != SG_INVALID_ID) {
        _sg_pass* pass = _sg_pass_at(&_sg.pools, res.id);
        SOKOL_ASSERT(pass && (pass->slot.state == SG_RESOURCESTATE_INITIAL) && (pass->slot.id == SG_INVALID_ID));
        pass->slot.id = res.id;
        pass->slot.state = SG_RESOURCESTATE_ALLOC;
    }
    return res;
}

/*-- initialize an allocated resource ----------------------------------------*/
void sg_init_buffer(sg_buffer buf_id, const sg_buffer_desc* desc) {
    SOKOL_ASSERT(buf_id.id != SG_INVALID_ID && desc);
    _sg_buffer* buf = _sg_lookup_buffer(&_sg.pools, buf_id.id);
    SOKOL_ASSERT(buf && buf->slot.state == SG_RESOURCESTATE_ALLOC);
    if (_sg_validate_buffer_desc(desc)) {
        _sg_create_buffer(buf, desc);
    }
    else {
        buf->slot.state = SG_RESOURCESTATE_FAILED;
    }
    SOKOL_ASSERT((buf->slot.state == SG_RESOURCESTATE_VALID)||(buf->slot.state == SG_RESOURCESTATE_FAILED));
}

void sg_init_image(sg_image img_id, const sg_image_desc* desc) {
    SOKOL_ASSERT(img_id.id != SG_INVALID_ID && desc);
    _sg_image* img = _sg_lookup_image(&_sg.pools, img_id.id);
    SOKOL_ASSERT(img && img->slot.state == SG_RESOURCESTATE_ALLOC);
    if (_sg_validate_image_desc(desc)) {
        _sg_create_image(img, desc);
    }
    else {
        img->slot.state = SG_RESOURCESTATE_FAILED;
    }
    SOKOL_ASSERT((img->slot.state == SG_RESOURCESTATE_VALID)||(img->slot.state == SG_RESOURCESTATE_FAILED));
}

void sg_init_shader(sg_shader shd_id, const sg_shader_desc* desc) {
    SOKOL_ASSERT(shd_id.id != SG_INVALID_ID && desc);
    _sg_shader* shd = _sg_lookup_shader(&_sg.pools, shd_id.id);
    SOKOL_ASSERT(shd && shd->slot.state == SG_RESOURCESTATE_ALLOC);
    if (_sg_validate_shader_desc(desc)) {
        _sg_create_shader(shd, desc);
    }
    else {
        shd->slot.state = SG_RESOURCESTATE_FAILED;
    }
    SOKOL_ASSERT((shd->slot.state == SG_RESOURCESTATE_VALID)||(shd->slot.state == SG_RESOURCESTATE_FAILED));
}

void sg_init_pipeline(sg_pipeline pip_id, const sg_pipeline_desc* desc) {
    SOKOL_ASSERT(pip_id.id != SG_INVALID_ID && desc);
    _sg_pipeline* pip = _sg_lookup_pipeline(&_sg.pools, pip_id.id);
    SOKOL_ASSERT(pip && pip->slot.state == SG_RESOURCESTATE_ALLOC);
    if (_sg_validate_pipeline_desc(desc)) {
        _sg_shader* shd = _sg_lookup_shader(&_sg.pools, desc->shader.id);
        SOKOL_ASSERT(shd && shd->slot.state == SG_RESOURCESTATE_VALID);
        _sg_create_pipeline(pip, shd, desc);
    }
    else {
        pip->slot.state = SG_RESOURCESTATE_FAILED;
    }
    SOKOL_ASSERT((pip->slot.state == SG_RESOURCESTATE_VALID)||(pip->slot.state == SG_RESOURCESTATE_FAILED)); 
}

void sg_init_pass(sg_pass pass_id, const sg_pass_desc* desc) {
    SOKOL_ASSERT(pass_id.id != SG_INVALID_ID && desc);
    _sg_pass* pass = _sg_lookup_pass(&_sg.pools, pass_id.id);
    SOKOL_ASSERT(pass && pass->slot.state == SG_RESOURCESTATE_ALLOC);
    if (_sg_validate_pass_desc(desc)) {
        /* lookup pass attachment image pointers */
        _sg_image* att_imgs[SG_MAX_COLOR_ATTACHMENTS + 1];
        for (int i = 0; i < SG_MAX_COLOR_ATTACHMENTS; i++) {
            if (desc->color_attachments[i].image.id) {
                att_imgs[i] = _sg_lookup_image(&_sg.pools, desc->color_attachments[i].image.id);
                SOKOL_ASSERT(att_imgs[i] && att_imgs[i]->slot.state == SG_RESOURCESTATE_VALID);
            }
            else {
                att_imgs[i] = 0;
            }
        }
        const int ds_att_index = SG_MAX_COLOR_ATTACHMENTS;
        if (desc->depth_stencil_attachment.image.id) {
            att_imgs[ds_att_index] = _sg_lookup_image(&_sg.pools, desc->depth_stencil_attachment.image.id);
            SOKOL_ASSERT(att_imgs[ds_att_index] && att_imgs[ds_att_index]->slot.state == SG_RESOURCESTATE_VALID);
        }
        else {
            att_imgs[ds_att_index] = 0;
        }
        _sg_create_pass(pass, att_imgs, desc);
    }
    else {
        pass->slot.state = SG_RESOURCESTATE_FAILED;
    }
    SOKOL_ASSERT((pass->slot.state == SG_RESOURCESTATE_VALID)||(pass->slot.state == SG_RESOURCESTATE_FAILED)); 
}

/*-- set allocated resource to failed state ----------------------------------*/
void sg_fail_buffer(sg_buffer buf_id) {
    SOKOL_ASSERT(buf_id.id != SG_INVALID_ID);
    _sg_buffer* buf = _sg_lookup_buffer(&_sg.pools, buf_id.id);
    SOKOL_ASSERT(buf && buf->slot.state == SG_RESOURCESTATE_ALLOC);
    buf->slot.state = SG_RESOURCESTATE_FAILED;
}

void sg_fail_image(sg_image img_id) {
    SOKOL_ASSERT(img_id.id != SG_INVALID_ID);
    _sg_image* img = _sg_lookup_image(&_sg.pools, img_id.id);
    SOKOL_ASSERT(img && img->slot.state == SG_RESOURCESTATE_ALLOC);
    img->slot.state = SG_RESOURCESTATE_FAILED;
}

void sg_fail_shader(sg_shader shd_id) {
    SOKOL_ASSERT(shd_id.id != SG_INVALID_ID);
    _sg_shader* shd = _sg_lookup_shader(&_sg.pools, shd_id.id);
    SOKOL_ASSERT(shd && shd->slot.state == SG_RESOURCESTATE_ALLOC);
    shd->slot.state = SG_RESOURCESTATE_FAILED;
}

void sg_fail_pipeline(sg_pipeline pip_id) {
    SOKOL_ASSERT(pip_id.id != SG_INVALID_ID);
    _sg_pipeline* pip = _sg_lookup_pipeline(&_sg.pools, pip_id.id);
    SOKOL_ASSERT(pip && pip->slot.state == SG_RESOURCESTATE_ALLOC);
    pip->slot.state = SG_RESOURCESTATE_FAILED;
}

void sg_fail_pass(sg_pass pass_id) {
    SOKOL_ASSERT(pass_id.id != SG_INVALID_ID);
    _sg_pass* pass = _sg_lookup_pass(&_sg.pools, pass_id.id);
    SOKOL_ASSERT(pass && pass->slot.state == SG_RESOURCESTATE_ALLOC);
    pass->slot.state = SG_RESOURCESTATE_FAILED;
}

/*-- get resource state */
sg_resource_state sg_query_buffer_state(sg_buffer buf_id) {
    if (buf_id.id != SG_INVALID_ID) {
        _sg_buffer* buf = _sg_lookup_buffer(&_sg.pools, buf_id.id);
        if (buf) {
            return buf->slot.state;
        }
    }
    return SG_RESOURCESTATE_INVALID;
}

sg_resource_state sg_query_image_state(sg_image img_id) {
    if (img_id.id != SG_INVALID_ID) {
        _sg_image* img = _sg_lookup_image(&_sg.pools, img_id.id);
        if (img) {
            return img->slot.state;
        }
    }
    return SG_RESOURCESTATE_INVALID;
}

sg_resource_state sg_query_shader_state(sg_shader shd_id) {
    if (shd_id.id != SG_INVALID_ID) {
        _sg_shader* shd = _sg_lookup_shader(&_sg.pools, shd_id.id);
        if (shd) {
            return shd->slot.state;
        }
    }
    return SG_RESOURCESTATE_INVALID;
}

sg_resource_state sg_query_pipeline_state(sg_pipeline pip_id) {
    if (pip_id.id != SG_INVALID_ID) {
        _sg_pipeline* pip = _sg_lookup_pipeline(&_sg.pools, pip_id.id);
        if (pip) {
            return pip->slot.state;
        }
    }
    return SG_RESOURCESTATE_INVALID;
}

sg_resource_state sg_query_pass_state(sg_pass pass_id) {
    if (pass_id.id != SG_INVALID_ID) {
        _sg_pass* pass = _sg_lookup_pass(&_sg.pools, pass_id.id);
        if (pass) {
            return pass->slot.state;
        }
    }
    return SG_RESOURCESTATE_INVALID;
}

/*-- allocate and initialize resource ----------------------------------------*/
sg_buffer sg_make_buffer(const sg_buffer_desc* desc) {
    SOKOL_ASSERT(desc);
    sg_buffer buf_id = sg_alloc_buffer();
    if (buf_id.id != SG_INVALID_ID) {
        sg_init_buffer(buf_id, desc);
    }
    else {
        SOKOL_LOG("buffer pool exhausted!");
    }
    return buf_id;
}

sg_image sg_make_image(const sg_image_desc* desc) {
    SOKOL_ASSERT(desc);
    sg_image img_id = sg_alloc_image();
    if (img_id.id != SG_INVALID_ID) {
        sg_init_image(img_id, desc);
    }
    else {
        SOKOL_LOG("image pool exhausted!");
    }
    return img_id;
}

sg_shader sg_make_shader(const sg_shader_desc* desc) {
    SOKOL_ASSERT(desc);
    sg_shader shd_id = sg_alloc_shader();
    if (shd_id.id != SG_INVALID_ID) {
        sg_init_shader(shd_id, desc);
    }
    else {
        SOKOL_LOG("shader pool exhausted!");
    }
    return shd_id;
}

sg_pipeline sg_make_pipeline(const sg_pipeline_desc* desc) {
    SOKOL_ASSERT(desc);
    sg_pipeline pip_id = sg_alloc_pipeline();
    if (pip_id.id != SG_INVALID_ID) {
        sg_init_pipeline(pip_id, desc);
    }
    else {
        SOKOL_LOG("pipeline pool exhausted!");
    }
    return pip_id;
}

sg_pass sg_make_pass(const sg_pass_desc* desc) {
    SOKOL_ASSERT(desc);
    sg_pass pass_id = sg_alloc_pass();
    if (pass_id.id != SG_INVALID_ID) {
        sg_init_pass(pass_id, desc);
    }
    else {
        SOKOL_LOG("pass pool exhausted!");
    }
    return pass_id;
}

/*-- destroy resource --------------------------------------------------------*/
void sg_destroy_buffer(sg_buffer buf_id) {
    _sg_buffer* buf = _sg_lookup_buffer(&_sg.pools, buf_id.id);
    if (buf) {
        _sg_destroy_buffer(buf);
        _sg_pool_free_id(&_sg.pools.buffer_pool, buf_id.id);
    }
}

void sg_destroy_image(sg_image img_id) {
    _sg_image* img = _sg_lookup_image(&_sg.pools, img_id.id);
    if (img) {
        _sg_destroy_image(img);
        _sg_pool_free_id(&_sg.pools.image_pool, img_id.id);
    }
}

void sg_destroy_shader(sg_shader shd_id) {
    _sg_shader* shd = _sg_lookup_shader(&_sg.pools, shd_id.id);
    if (shd) {
        _sg_destroy_shader(shd);
        _sg_pool_free_id(&_sg.pools.shader_pool, shd_id.id);
    }
}

void sg_destroy_pipeline(sg_pipeline pip_id) {
    _sg_pipeline* pip = _sg_lookup_pipeline(&_sg.pools, pip_id.id);
    if (pip) {
        _sg_destroy_pipeline(pip);
        _sg_pool_free_id(&_sg.pools.pipeline_pool, pip_id.id);
    }
}

void sg_destroy_pass(sg_pass pass_id) {
    _sg_pass* pass = _sg_lookup_pass(&_sg.pools, pass_id.id);
    if (pass) {
        _sg_destroy_pass(pass);
        _sg_pool_free_id(&_sg.pools.pass_pool, pass_id.id);
    }
}

void sg_begin_default_pass(const sg_pass_action* pass_action, int width, int height) {
    SOKOL_ASSERT(pass_action);
    SOKOL_ASSERT((pass_action->_start_canary == 0) && (pass_action->_end_canary == 0));
    sg_pass_action pa;
    _sg_resolve_default_pass_action(pass_action, &pa);
    _sg.cur_pass.id = SG_INVALID_ID;
    _sg.pass_valid = true;
    _sg_begin_pass(0, &pa, width, height);
}

void sg_begin_pass(sg_pass pass_id, const sg_pass_action* pass_action) {
    SOKOL_ASSERT(pass_action);
    SOKOL_ASSERT((pass_action->_start_canary == 0) && (pass_action->_end_canary == 0));
    _sg.cur_pass = pass_id;
    _sg_pass* pass = _sg_lookup_pass(&_sg.pools, pass_id.id);
    if (pass && _sg_validate_begin_pass(pass)) {
        _sg.pass_valid = true;
        sg_pass_action pa;
        _sg_resolve_default_pass_action(pass_action, &pa);
        const int w = pass->color_atts[0].image->width;
        const int h = pass->color_atts[0].image->height;
        _sg_begin_pass(pass, &pa, w, h);
    }
    else {
        _sg.pass_valid = false;
    }
}

void sg_apply_viewport(int x, int y, int width, int height, bool origin_top_left) {
    if (!_sg.pass_valid) {
        return;
    }
    _sg_apply_viewport(x, y, width, height, origin_top_left);
}

void sg_apply_scissor_rect(int x, int y, int width, int height, bool origin_top_left) {
    if (!_sg.pass_valid) {
        return;
    }
    _sg_apply_scissor_rect(x, y, width, height, origin_top_left);
}

void sg_apply_draw_state(const sg_draw_state* ds) {
    SOKOL_ASSERT(ds);
    SOKOL_ASSERT((ds->_start_canary==0) && (ds->_end_canary==0));
    if (!_sg_validate_draw_state(ds)) {
        _sg.next_draw_valid = false;
        return;
    }
    if (!_sg.pass_valid) {
        return;
    }
    _sg.next_draw_valid = true;
    _sg.cur_pipeline = ds->pipeline;

    /* lookup resource pointers, resources which are not in SG_RESOURCESTATE_VALID
       are not a fatal error, but supress the following drawcalls, this is to
       allow for simple asynchronous resource setup
    */
    _sg_pipeline* pip = _sg_lookup_pipeline(&_sg.pools, ds->pipeline.id);
    SOKOL_ASSERT(pip);
    _sg.next_draw_valid &= (SG_RESOURCESTATE_VALID == pip->slot.state);
    SOKOL_ASSERT(pip->shader && (pip->shader->slot.id == pip->shader_id.id));

    _sg_buffer* vbs[SG_MAX_SHADERSTAGE_BUFFERS] = { 0 };
    int num_vbs = 0;
    for (int i = 0; i < SG_MAX_SHADERSTAGE_BUFFERS; i++, num_vbs++) {
        if (ds->vertex_buffers[i].id) {
            vbs[i] = _sg_lookup_buffer(&_sg.pools, ds->vertex_buffers[i].id);
            SOKOL_ASSERT(vbs[i]);
            _sg.next_draw_valid &= (SG_RESOURCESTATE_VALID == vbs[i]->slot.state);
        }
        else {
            break;
        }
    }

    _sg_buffer* ib = 0;
    if (ds->index_buffer.id) {
        ib = _sg_lookup_buffer(&_sg.pools, ds->index_buffer.id);
        SOKOL_ASSERT(ib);
        _sg.next_draw_valid &= (SG_RESOURCESTATE_VALID == ib->slot.state);
    }

    _sg_image* vs_imgs[SG_MAX_SHADERSTAGE_IMAGES] = { 0 };
    int num_vs_imgs = 0;
    for (int i = 0; i < SG_MAX_SHADERSTAGE_IMAGES; i++, num_vs_imgs++) {
        if (ds->vs_images[i].id) {
            vs_imgs[i] = _sg_lookup_image(&_sg.pools, ds->vs_images[i].id);
            SOKOL_ASSERT(vs_imgs[i]);
            _sg.next_draw_valid &= (SG_RESOURCESTATE_VALID == vs_imgs[i]->slot.state);
        }
        else {
            break;
        }
    }

    _sg_image* fs_imgs[SG_MAX_SHADERSTAGE_IMAGES] = { 0 };
    int num_fs_imgs = 0;
    for (int i = 0; i < SG_MAX_SHADERSTAGE_IMAGES; i++, num_fs_imgs++) {
        if (ds->fs_images[i].id) {
            fs_imgs[i] = _sg_lookup_image(&_sg.pools, ds->fs_images[i].id);
            SOKOL_ASSERT(fs_imgs[i]);
            _sg.next_draw_valid &= (SG_RESOURCESTATE_VALID == fs_imgs[i]->slot.state);
        }
        else {
            break;
        }
    }
    if (_sg.next_draw_valid) {
        _sg_apply_draw_state(pip, vbs, num_vbs, ib, vs_imgs, num_vs_imgs, fs_imgs, num_fs_imgs);
    }
}

void sg_apply_uniform_block(sg_shader_stage stage, int ub_index, const void* data, int num_bytes) {
    SOKOL_ASSERT((stage == SG_SHADERSTAGE_VS) || (stage == SG_SHADERSTAGE_FS));
    SOKOL_ASSERT((ub_index >= 0) && (ub_index < SG_MAX_SHADERSTAGE_UBS));
    SOKOL_ASSERT(data && (num_bytes > 0));
    if (!_sg_validate_apply_uniform_block(stage, ub_index, data, num_bytes)) {
        _sg.next_draw_valid = false;
        return;
    }
    if (!(_sg.pass_valid && _sg.next_draw_valid)) {
        return;
    }
    _sg_apply_uniform_block(stage, ub_index, data, num_bytes);
}

void sg_draw(int base_element, int num_elements, int num_instances) {
    if (!(_sg.pass_valid && _sg.next_draw_valid)) {
        return;
    }
    _sg_draw(base_element, num_elements, num_instances);
}

void sg_end_pass() {
    if (!_sg.pass_valid) {
        return;
    }
    _sg_end_pass();
    _sg.cur_pass.id = SG_INVALID_ID;
    _sg.cur_pipeline.id = SG_INVALID_ID;
    _sg.pass_valid = false;
}

void sg_commit() {
    _sg_commit();
    _sg.frame_index++;
}

void sg_reset_state_cache() {
    _sg_reset_state_cache();
}

void sg_update_buffer(sg_buffer buf_id, const void* data, int num_bytes) {
    if (num_bytes == 0) {
        return;
    }
    _sg_buffer* buf = _sg_lookup_buffer(&_sg.pools, buf_id.id);
    if (!(buf && buf->slot.state == SG_RESOURCESTATE_VALID)) {
        return;
    }
    if (_sg_validate_update_buffer(buf, data, num_bytes)) {
        SOKOL_ASSERT(buf->upd_frame_index != _sg.frame_index);
        _sg_update_buffer(buf, data, num_bytes);
        buf->upd_frame_index = _sg.frame_index;
    }
}

void sg_update_image(sg_image img_id, const sg_image_content* data) {
    _sg_image* img = _sg_lookup_image(&_sg.pools, img_id.id);
    if (!(img && img->slot.state == SG_RESOURCESTATE_VALID)) {
        return;
    }
    if (_sg_validate_update_image(img, data)) {
        SOKOL_ASSERT(img->upd_frame_index != _sg.frame_index);
        _sg_update_image(img, data);
        img->upd_frame_index = _sg.frame_index;
    }
}

sg_vertex_attr_desc sg_named_attr(const char* name, int offset, sg_vertex_format format) {
    sg_vertex_attr_desc desc;
    desc.name = name;
    desc.offset = offset;
    desc.format = format;
    return desc;
}

sg_vertex_attr_desc sg_sem_attr(const char* sem_name, int sem_index, int offset, sg_vertex_format format) {
    sg_vertex_attr_desc desc;
    desc.sem_name = sem_name;
    desc.sem_index = sem_index;
    desc.offset = offset;
    desc.format = format;
    return desc;
}

sg_shader_uniform_desc sg_named_uniform(const char* name, sg_uniform_type type, int array_count) {
    sg_shader_uniform_desc desc;
    desc.name = name;
    desc.type = type;
    desc.array_count = array_count;
    return desc;
}

sg_shader_image_desc sg_named_image(const char* name, sg_image_type type) {
    sg_shader_image_desc desc;
    desc.name = name;
    desc.type = type;
    return desc;
}

#ifdef __cplusplus
} /* extern "C" */
#endif

