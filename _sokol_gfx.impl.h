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
        #define SOKOL_LOG(s) puts(s)
    #else
        #define SOKOL_LOG(s)
    #endif
#endif
#if !(defined(SOKOL_GLCORE33)||defined(SOKOL_GLES2)||defined(SOKOL_GLES3)||defined(SOKOL_D3D11)||defined(SOKOL_METAL))
#error "Please select a backend with SOKOL_GLCORE33, SOKOL_GLES2, SOKOL_GLES3, SOKOL_D3D11 or SOKOL_METAL"
#endif

#if defined(__clang__)
/* I want to know about unused functions in debug mode */
#if !defined(SOKOL_DEBUG)
#define _SOKOL_PRIVATE __attribute__((unused)) static
#else
#define _SOKOL_PRIVATE static
#endif
#else
/* non-clang compilers */
#define _SOKOL_PRIVATE static
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

/* a helper macro to select a default if val is zero-initialized (which means 'default') */
#define _sg_select(val, def) ((val == 0) ? def : val)

_SOKOL_PRIVATE void _sg_init_stencil_state(sg_stencil_state* s) {
    SOKOL_ASSERT(s);
    s->fail_op = SG_STENCILOP_KEEP;
    s->depth_fail_op = SG_STENCILOP_KEEP;
    s->pass_op = SG_STENCILOP_KEEP;
    s->compare_func = SG_COMPAREFUNC_ALWAYS;
}

_SOKOL_PRIVATE void _sg_init_depth_stencil_state(sg_depth_stencil_state* s) {
    SOKOL_ASSERT(s);
    _sg_init_stencil_state(&s->stencil_front);
    _sg_init_stencil_state(&s->stencil_back);
    s->depth_compare_func = SG_COMPAREFUNC_ALWAYS;
    s->depth_write_enabled = false;
    s->stencil_enabled = false;
    s->stencil_read_mask = 0;
    s->stencil_write_mask = 0;
    s->stencil_ref = 0;
}

_SOKOL_PRIVATE void _sg_init_blend_state(sg_blend_state* s) {
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

_SOKOL_PRIVATE void _sg_init_rasterizer_state(sg_rasterizer_state* s) {
    SOKOL_ASSERT(s);
    s->scissor_test_enabled = false;
    s->alpha_to_coverage_enabled = false;
    s->cull_mode = SG_CULLMODE_NONE;
    s->face_winding = SG_FACEWINDING_CW;
    s->sample_count = 1;
}

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

//-- include the selected rendering backend ----------------------------------*/
#ifdef __cplusplus
} /* extern "C" */
#endif
#if defined(SOKOL_GLCORE33) || defined(SOKOL_GLES2) || defined(SOKOL_GLES3)
#include "_sokol_gfx_gl.impl.h"
#elif defined(SOKOL_D3D11)
#include "_sokol_gfx_d3d11.impl.h"
#elif defined(SOKOL_METAL)
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
    SOKOL_ASSERT(pool && (num > 0));
    /* slot 0 is reserved for the 'invalid id', so bump the pool size by 1 */
    pool->size = num + 1;
    pool->queue_top = 0;
    pool->unique_counter = 0;
    /* it's not a bug to only reserve 'num' here */
    pool->free_queue = SOKOL_MALLOC(sizeof(int)*num);
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
    _sg_init_pool(&p->buffer_pool, _sg_select(desc->buffer_pool_size, _SG_DEFAULT_BUFFER_POOL_SIZE));
    p->buffers = SOKOL_MALLOC(sizeof(_sg_buffer) * p->buffer_pool.size);
    for (int i = 0; i < p->buffer_pool.size; i++) {
        _sg_init_buffer(&p->buffers[i]);
    }

    SOKOL_ASSERT((desc->image_pool_size >= 0) && (desc->image_pool_size < _SG_MAX_POOL_SIZE));
    _sg_init_pool(&p->image_pool, _sg_select(desc->image_pool_size, _SG_DEFAULT_IMAGE_POOL_SIZE));
    p->images = SOKOL_MALLOC(sizeof(_sg_image) * p->image_pool.size);
    for (int i = 0; i < p->image_pool.size; i++) {
        _sg_init_image(&p->images[i]);
    }

    SOKOL_ASSERT((desc->shader_pool_size >= 0) && (desc->shader_pool_size < _SG_MAX_POOL_SIZE));
    _sg_init_pool(&p->shader_pool, _sg_select(desc->shader_pool_size, _SG_DEFAULT_SHADER_POOL_SIZE));
    p->shaders = SOKOL_MALLOC(sizeof(_sg_shader) * p->shader_pool.size);
    for (int i = 0; i < p->shader_pool.size; i++) {
        _sg_init_shader(&p->shaders[i]);
    }

    SOKOL_ASSERT((desc->pipeline_pool_size >= 0) && (desc->pipeline_pool_size < _SG_MAX_POOL_SIZE));
    _sg_init_pool(&p->pipeline_pool, _sg_select(desc->pipeline_pool_size, _SG_DEFAULT_PIPELINE_POOL_SIZE));
    p->pipelines = SOKOL_MALLOC(sizeof(_sg_pipeline) * p->pipeline_pool.size);
    for (int i = 0; i < p->pipeline_pool.size; i++) {
        _sg_init_pipeline(&p->pipelines[i]);
    }

    SOKOL_ASSERT((desc->pass_pool_size >= 0) && (desc->pass_pool_size < _SG_MAX_POOL_SIZE));
    _sg_init_pool(&p->pass_pool, _sg_select(desc->pass_pool_size, _SG_DEFAULT_PASS_POOL_SIZE));
    p->passes = SOKOL_MALLOC(sizeof(_sg_pass) * p->pass_pool.size);
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

_SOKOL_PRIVATE void _sg_destroy_all_resources(_sg_backend* b, _sg_pools* p) {
    /*  this is a bit dumb since it loops over all pool slots to
        find the occupied slots, on the other hand it is only ever
        executed at shutdown
    */
    for (int i = 0; i < p->buffer_pool.size; i++) {
        if (p->buffers[i].slot.state == SG_RESOURCESTATE_VALID) {
            _sg_destroy_buffer(b, &p->buffers[i]);
        }
    }
    for (int i = 0; i < p->image_pool.size; i++) {
        if (p->images[i].slot.state == SG_RESOURCESTATE_VALID) {
            _sg_destroy_image(b, &p->images[i]);
        }
    }
    for (int i = 0; i < p->shader_pool.size; i++) {
        if (p->shaders[i].slot.state == SG_RESOURCESTATE_VALID) {
            _sg_destroy_shader(b, &p->shaders[i]);
        }
    }
    for (int i = 0; i < p->pipeline_pool.size; i++) {
        if (p->pipelines[i].slot.state == SG_RESOURCESTATE_VALID) {
            _sg_destroy_pipeline(b, &p->pipelines[i]);
        }
    }
    for (int i = 0; i < p->pass_pool.size; i++) {
        if (p->passes[i].slot.state == SG_RESOURCESTATE_VALID) {
            _sg_destroy_pass(b, &p->passes[i]);
        }
    }
}

/*-- public API functions ----------------------------------------------------*/ 
typedef struct {
    _sg_pools pools;
    _sg_backend backend;
} _sg_state;
static _sg_state* _sg = 0;

void sg_setup(const sg_desc* desc) {
    SOKOL_ASSERT(!_sg);
    SOKOL_ASSERT(desc);
    SOKOL_ASSERT((desc->_start_canary == 0) && (desc->_end_canary == 0));
    _sg = SOKOL_MALLOC(sizeof(_sg_state));
    _sg_setup_pools(&_sg->pools, desc);
    _sg_setup_backend(&_sg->backend);
}

void sg_shutdown() {
    SOKOL_ASSERT(_sg);
    _sg_destroy_all_resources(&_sg->backend, &_sg->pools);
    _sg_discard_backend(&_sg->backend);
    _sg_discard_pools(&_sg->pools);
    SOKOL_FREE(_sg);
    _sg = 0;
}

bool sg_isvalid() {
    return _sg != 0;
}

bool sg_query_feature(sg_feature f) {
    SOKOL_ASSERT(_sg);
    return _sg_query_feature(&_sg->backend, f);
}

/*-- allocate resource id ----------------------------------------------------*/
sg_buffer sg_alloc_buffer() {
    SOKOL_ASSERT(_sg);
    sg_buffer res;
    res.id = _sg_pool_alloc_id(&_sg->pools.buffer_pool);
    if (res.id != SG_INVALID_ID) {
        _sg_buffer* buf = _sg_buffer_at(&_sg->pools, res.id);
        SOKOL_ASSERT(buf && (buf->slot.state == SG_RESOURCESTATE_INITIAL) && (buf->slot.id == SG_INVALID_ID));
        buf->slot.id = res.id;
        buf->slot.state = SG_RESOURCESTATE_ALLOC;
    }
    return res;
}

sg_image sg_alloc_image() {
    SOKOL_ASSERT(_sg);
    sg_image res;
    res.id = _sg_pool_alloc_id(&_sg->pools.image_pool);
    if (res.id != SG_INVALID_ID) {
        _sg_image* img = _sg_image_at(&_sg->pools, res.id);
        SOKOL_ASSERT(img && (img->slot.state == SG_RESOURCESTATE_INITIAL) && (img->slot.id == SG_INVALID_ID));
        img->slot.id = res.id;
        img->slot.state = SG_RESOURCESTATE_ALLOC;
    }
    return res;
}

sg_shader sg_alloc_shader() {
    SOKOL_ASSERT(_sg);
    sg_shader res;
    res.id = _sg_pool_alloc_id(&_sg->pools.shader_pool);
    if (res.id != SG_INVALID_ID) {
        _sg_shader* shd = _sg_shader_at(&_sg->pools, res.id);
        SOKOL_ASSERT(shd && (shd->slot.state == SG_RESOURCESTATE_INITIAL) && (shd->slot.id == SG_INVALID_ID));
        shd->slot.id = res.id;
        shd->slot.state = SG_RESOURCESTATE_ALLOC;
    }
    return res;
}

sg_pipeline sg_alloc_pipeline() {
    SOKOL_ASSERT(_sg);
    sg_pipeline res;
    res.id = _sg_pool_alloc_id(&_sg->pools.pipeline_pool);
    if (res.id != SG_INVALID_ID) {
        _sg_pipeline* pip = _sg_pipeline_at(&_sg->pools, res.id);
        SOKOL_ASSERT(pip && (pip->slot.state == SG_RESOURCESTATE_INITIAL) && (pip->slot.id == SG_INVALID_ID));
        pip->slot.id = res.id;
        pip->slot.state = SG_RESOURCESTATE_ALLOC;
    }
    return res;
}

sg_pass sg_alloc_pass() {
    SOKOL_ASSERT(_sg);
    sg_pass res;
    res.id = _sg_pool_alloc_id(&_sg->pools.pass_pool);
    if (res.id != SG_INVALID_ID) {
        _sg_pass* pass = _sg_pass_at(&_sg->pools, res.id);
        SOKOL_ASSERT(pass && (pass->slot.state == SG_RESOURCESTATE_INITIAL) && (pass->slot.id == SG_INVALID_ID));
        pass->slot.id = res.id;
        pass->slot.state = SG_RESOURCESTATE_ALLOC;
    }
    return res;
}

/*-- validate description structs --------------------------------------------*/
_SOKOL_PRIVATE void _sg_validate_buffer_desc(const sg_buffer_desc* desc) {
    SOKOL_ASSERT(_sg && desc);
    SOKOL_ASSERT(desc->size > 0);
    SOKOL_ASSERT((desc->type>=_SG_BUFFERTYPE_DEFAULT)&&(desc->type<_SG_BUFFERTYPE_NUM));
    SOKOL_ASSERT((desc->usage>=_SG_USAGE_DEFAULT)&&(desc->usage<_SG_USAGE_NUM));
    SOKOL_ASSERT(desc->data_size <= desc->size);
    #ifdef SOKOL_DEBUG 
    if (desc->usage == SG_USAGE_IMMUTABLE) {
        SOKOL_ASSERT(desc->data_ptr);
    }
    #endif
}

_SOKOL_PRIVATE void _sg_validate_image_desc(const sg_image_desc* desc) {
    SOKOL_ASSERT(_sg && desc);
    SOKOL_ASSERT((desc->type >= 0) && (desc->type < _SG_IMAGETYPE_NUM));
    SOKOL_ASSERT((desc->width > 0) && (desc->height > 0) && (desc->depth >= 0));
    SOKOL_ASSERT((desc->num_mipmaps >= 0) && (desc->num_mipmaps <= SG_MAX_MIPMAPS));
    SOKOL_ASSERT((desc->usage >= 0) && (desc->usage < _SG_USAGE_NUM));
    SOKOL_ASSERT((desc->pixel_format >= 0) && (desc->pixel_format < _SG_PIXELFORMAT_NUM));
    SOKOL_ASSERT(desc->sample_count >= 0);
    SOKOL_ASSERT((desc->min_filter >= 0) && (desc->min_filter < _SG_FILTER_NUM));
    SOKOL_ASSERT((desc->mag_filter >= 0) && (desc->mag_filter < _SG_FILTER_NUM));
    SOKOL_ASSERT((desc->wrap_u >= 0) && (desc->wrap_u < _SG_WRAP_NUM));
    SOKOL_ASSERT((desc->wrap_v >= 0) && (desc->wrap_v < _SG_WRAP_NUM));
    SOKOL_ASSERT((desc->wrap_w >= 0) && (desc->wrap_w < _SG_WRAP_NUM));
    SOKOL_ASSERT(desc->num_data_items >= 0);
    #if SOKOL_DEBUG
    if (desc->render_target) {
        SOKOL_ASSERT((desc->usage == _SG_USAGE_DEFAULT) || (desc->usage == SG_USAGE_IMMUTABLE));
        SOKOL_ASSERT((desc->pixel_format == _SG_PIXELFORMAT_DEFAULT) || 
                     _sg_is_valid_rendertarget_color_format(desc->pixel_format) || 
                     _sg_is_valid_rendertarget_depth_format(desc->pixel_format));
        SOKOL_ASSERT((0 == desc->num_data_items) && (0 == desc->data_ptrs) && (0 == desc->data_sizes));
        if (_sg_is_valid_rendertarget_depth_format(desc->pixel_format)) {
            SOKOL_ASSERT((desc->type==_SG_IMAGETYPE_DEFAULT)||(desc->type==SG_IMAGETYPE_2D));
            SOKOL_ASSERT((desc->num_mipmaps==0)||(desc->num_mipmaps==1));
        }
    }
    if (desc->num_data_items > 0) {
        SOKOL_ASSERT(desc->data_ptrs);
        SOKOL_ASSERT(desc->data_sizes);
    }
    #endif
}

_SOKOL_PRIVATE void _sg_validate_shader_desc(const sg_shader_desc* desc) {
    SOKOL_ASSERT(_sg && desc);
    #if defined(SOKOL_GLCORE33) || defined(SOKOL_GLES2) || defined(SOKOL_GLES3)
    SOKOL_ASSERT(desc->vs.source);
    SOKOL_ASSERT(desc->fs.source);
    #endif
    #ifdef SOKOL_DEBUG
    for (int i = 0; i < SG_NUM_SHADER_STAGES; i++) {
        const sg_shader_stage_desc* stage_desc = (i == 0)? &desc->vs : &desc->fs;
        bool uniform_blocks_continuous = true;
        for (int ub_index = 0; ub_index < SG_MAX_SHADERSTAGE_UBS; ub_index++) {
            const sg_shader_uniform_block_desc* ub_desc = &stage_desc->uniform_blocks[ub_index];
            SOKOL_ASSERT(ub_desc->size >= 0);
            if (ub_desc->size > 0) {
                SOKOL_ASSERT(uniform_blocks_continuous);
                bool uniforms_continuous = true;
                for (int u_index = 0; u_index < SG_MAX_UB_MEMBERS; u_index++) {
                    const sg_shader_uniform_desc* u_desc = &ub_desc->uniforms[u_index];
                    if (u_desc->type != SG_UNIFORMTYPE_INVALID) {
                        SOKOL_ASSERT(uniforms_continuous);
                        #ifdef SOKOL_GLES2
                        SOKOL_ASSERT(u_desc->name);
                        #endif
                        int array_count = _sg_select(u_desc->array_count, 1);
                        SOKOL_ASSERT(array_count >= 1);
                        SOKOL_ASSERT(u_desc->offset >= 0);
                        SOKOL_ASSERT((u_desc->offset + _sg_uniform_size(u_desc->type, array_count)) <= ub_desc->size);
                    }
                    else {
                        uniforms_continuous = false;
                    }
                }
            }
            else {
                uniform_blocks_continuous = false;
                /* check that invalid uniform block entries have no members */
                SOKOL_ASSERT(ub_desc->uniforms[0].type == SG_UNIFORMTYPE_INVALID);
            }
        }
        bool images_continuous = true;
        for (int img_index = 0; img_index < SG_MAX_SHADERSTAGE_IMAGES; img_index++) {
            const sg_shader_image_desc* img_desc = &stage_desc->images[img_index];
            if (img_desc->type != _SG_IMAGETYPE_DEFAULT) {
                SOKOL_ASSERT(images_continuous);
                #ifdef SOKOL_GLES2
                SOKOL_ASSERT(img_desc->name);
                #endif
            } 
            else {
                images_continuous = false;
            }
        }
    }
    #endif
}

_SOKOL_PRIVATE void _sg_validate_pipeline_desc(const sg_pipeline_desc* desc) {
    SOKOL_ASSERT(_sg && desc);
    SOKOL_ASSERT(desc->shader.id != SG_INVALID_ID);
    SOKOL_ASSERT(desc->vertex_layouts[0].attrs[0].format != SG_VERTEXFORMAT_INVALID);
    #ifdef SOKOL_DEBUG
    int num_attrs = 0;
    bool layouts_continuous = true;
    for (int layout_index = 0; layout_index < SG_MAX_SHADERSTAGE_BUFFERS; layout_index++) {
        const sg_vertex_layout_desc* layout_desc = &desc->vertex_layouts[layout_index];
        if (layout_desc->stride == 0) {
            layouts_continuous = false;
            continue;
        }
        SOKOL_ASSERT((layout_desc->stride & 3) == 0);
        SOKOL_ASSERT((layout_desc->step_func>=0) && (layout_desc->step_func<_SG_VERTEXSTEP_NUM));
        SOKOL_ASSERT(layouts_continuous);
        bool attrs_continuous = true;
        for (int attr_index = 0; attr_index < SG_MAX_VERTEX_ATTRIBUTES; attr_index++) {
            const sg_vertex_attr_desc* attr_desc = &layout_desc->attrs[attr_index];
            if (attr_desc->format == SG_VERTEXFORMAT_INVALID) {
                attrs_continuous = false;
                continue;
            }
            SOKOL_ASSERT(attrs_continuous);
            SOKOL_ASSERT(attr_desc->offset + _sg_vertexformat_bytesize(attr_desc->format) <= layout_desc->stride);
            SOKOL_ASSERT((attr_desc->format > SG_VERTEXFORMAT_INVALID)&&(attr_desc->format<_SG_VERTEXFORMAT_NUM));
            #ifdef SOKOL_GLES2
            SOKOL_ASSERT(attr_desc->name);
            #endif
            num_attrs++;
        }
    }
    SOKOL_ASSERT(num_attrs < SG_MAX_VERTEX_ATTRIBUTES);
    #endif
}

_SOKOL_PRIVATE void _sg_validate_pass_desc(const sg_pass_desc* desc) {
    SOKOL_ASSERT(desc->color_attachments[0].image.id != SG_INVALID_ID);
}

_SOKOL_PRIVATE void _sg_validate_draw_state(const sg_draw_state* ds) {
    SOKOL_ASSERT(_sg && ds);
    SOKOL_ASSERT(ds->pipeline.id);
    SOKOL_ASSERT(ds->vertex_buffers[0].id);
}

_SOKOL_PRIVATE void _sg_validate_begin_pass(const _sg_pass* pass, const sg_pass_action* pass_action) {
    SOKOL_ASSERT(pass && pass_action);
    /* must have at least one color attachment */
    SOKOL_ASSERT(pass->color_atts[0].image);
    /* check color attachments */
    #if defined(SOKOL_DEBUG)
    const _sg_image* img = pass->color_atts[0].image;
    bool img_continuous = true;
    for (int i = 0; i < SG_MAX_COLOR_ATTACHMENTS; i++) {
        const _sg_attachment* att = &pass->color_atts[i];
        if (att->image) {
            SOKOL_ASSERT(img_continuous);
            /* pass valid? */
            SOKOL_ASSERT(att->image->slot.state == SG_RESOURCESTATE_VALID);
            /* pass still exists? */
            SOKOL_ASSERT(att->image->slot.id == att->image_id.id);
            /* all images must be render target */
            SOKOL_ASSERT(att->image->render_target);
            /* all images must be immutable */
            SOKOL_ASSERT(att->image->usage == SG_USAGE_IMMUTABLE);
            /* all images must have same size */
            SOKOL_ASSERT(att->image->width == img->width);
            SOKOL_ASSERT(att->image->height == img->height);
            /* all images must have same pixel format */
            SOKOL_ASSERT(att->image->pixel_format == img->pixel_format);
            /* must be a valid color render target pixel format */
            SOKOL_ASSERT(_sg_is_valid_rendertarget_color_format(att->image->pixel_format));
            /* all images must have same sample count */
            SOKOL_ASSERT(att->image->sample_count == img->sample_count);
        }
        else {
            img_continuous = false;
        }
    }
    /* check depth-stencil attachment */
    const _sg_attachment* ds_att = &pass->ds_att;
    if (ds_att->image) {
        SOKOL_ASSERT(ds_att->image->slot.state == SG_RESOURCESTATE_VALID);
        SOKOL_ASSERT(ds_att->image->slot.id == ds_att->image_id.id);
        SOKOL_ASSERT(ds_att->image->render_target);
        SOKOL_ASSERT(ds_att->image->usage == SG_USAGE_IMMUTABLE);
        SOKOL_ASSERT(ds_att->image->width == img->width);
        SOKOL_ASSERT(ds_att->image->height == img->height);
        SOKOL_ASSERT(_sg_is_valid_rendertarget_depth_format(ds_att->image->pixel_format));
    }
    #endif /* SOKOL_DEBUG */
}

_SOKOL_PRIVATE bool _sg_validate_draw(_sg_pipeline* pip, 
    _sg_buffer** vbs, int num_vbs, const _sg_buffer* ib,
    _sg_image** vs_imgs, int num_vs_imgs,
    _sg_image** fs_imgs, int num_fs_imgs) 
{
    if (!pip) {
        /* pipeline no longer exists */
        return false;
    }
    if (pip->slot.state != SG_RESOURCESTATE_VALID) {
        /* pipeline hasn't been setup */
        return false;
    }
    if (pip->shader->slot.id != pip->shader_id.id) {
        /* shader no longer exists */
        return false;
    }
    if (pip->shader->slot.state != SG_RESOURCESTATE_VALID) {
        /* shader hasn't been setup (e.g. compile error) */
        return false;
    }
    if ((pip->index_type != SG_INDEXTYPE_NONE) && !ib) {
        /* indexed rendering requested, but no index buffer */
        return false;
    }
    if (ib) {
        SOKOL_ASSERT(ib->type == SG_BUFFERTYPE_INDEXBUFFER);
        if (ib->slot.state != SG_RESOURCESTATE_VALID) {
            /* index buffer exists, but not valid for rendering */
            return false;
        }
    }
    /* check vertex buffers */
    for (int i = 0; i < num_vbs; i++) {
        const _sg_buffer* vb = vbs[i];
        if (!vb) {
            /* vertex buffer no longer exists */
            return false;
        }
        SOKOL_ASSERT(vb->type == SG_BUFFERTYPE_VERTEXBUFFER);
        if (vb->slot.state != SG_RESOURCESTATE_VALID) {
            /* vertex buffer exists, but not valid for rendering */
            return false;
        }
    }
    /* check vertex shader textures */
    /* number and type of images must match what shader expects */
    SOKOL_ASSERT(num_vs_imgs == pip->shader->stage[SG_SHADERSTAGE_VS].num_images);
    for (int i = 0; i < num_vs_imgs; i++) {
        const _sg_image* img = vs_imgs[i];
        if (!img) {
            /* image no longer exists */
            return false;
        }
        if (img->slot.state != SG_RESOURCESTATE_VALID) {
            /* image exists, but not valid for rendering */
            return false;
        }
        SOKOL_ASSERT(img->type == pip->shader->stage[SG_SHADERSTAGE_VS].images[i].type);
    }
    /* check fragment shader textures */
    /* number and type of images must match what shader expects */
    SOKOL_ASSERT(num_fs_imgs == pip->shader->stage[SG_SHADERSTAGE_FS].num_images);
    for (int i = 0; i < num_fs_imgs; i++) {
        const _sg_image* img = fs_imgs[i];
        if (!img) {
            /* image no longer exists */
            return false;
        }
        if (img->slot.state != SG_RESOURCESTATE_VALID) {
            /* image exists, but not valid for rendering */
            return false;
        }
        SOKOL_ASSERT(img->type == pip->shader->stage[SG_SHADERSTAGE_FS].images[i].type);
        /* cannot use depth-stencil images as texture (FIXME: or can we? GLES2?) */
        SOKOL_ASSERT(!_sg_is_valid_rendertarget_depth_format(img->pixel_format));
    }
    /* all ok for rendering! */
    return true;
}

/*-- initialize an allocated resource ----------------------------------------*/
void sg_init_buffer(sg_buffer buf_id, const sg_buffer_desc* desc) {
    SOKOL_ASSERT(_sg && buf_id.id != SG_INVALID_ID && desc);
    SOKOL_ASSERT((desc->_start_canary == 0) && (desc->_end_canary == 0));
    _sg_validate_buffer_desc(desc);
    _sg_buffer* buf = _sg_lookup_buffer(&_sg->pools, buf_id.id);
    SOKOL_ASSERT(buf && buf->slot.state == SG_RESOURCESTATE_ALLOC);
    _sg_create_buffer(&_sg->backend, buf, desc);
    SOKOL_ASSERT((buf->slot.state == SG_RESOURCESTATE_VALID)||(buf->slot.state == SG_RESOURCESTATE_FAILED));
}

void sg_init_image(sg_image img_id, const sg_image_desc* desc) {
    SOKOL_ASSERT(_sg && img_id.id != SG_INVALID_ID && desc);
    SOKOL_ASSERT((desc->_start_canary == 0) && (desc->_end_canary == 0));
    _sg_validate_image_desc(desc);
    _sg_image* img = _sg_lookup_image(&_sg->pools, img_id.id);
    SOKOL_ASSERT(img && img->slot.state == SG_RESOURCESTATE_ALLOC);
    _sg_create_image(&_sg->backend, img, desc);
    SOKOL_ASSERT((img->slot.state == SG_RESOURCESTATE_VALID)||(img->slot.state == SG_RESOURCESTATE_FAILED));
}

void sg_init_shader(sg_shader shd_id, const sg_shader_desc* desc) {
    SOKOL_ASSERT(_sg && shd_id.id != SG_INVALID_ID && desc);
    SOKOL_ASSERT((desc->_start_canary == 0) && (desc->_end_canary == 0));
    _sg_validate_shader_desc(desc);
    _sg_shader* shd = _sg_lookup_shader(&_sg->pools, shd_id.id);
    SOKOL_ASSERT(shd && shd->slot.state == SG_RESOURCESTATE_ALLOC);
    _sg_create_shader(&_sg->backend, shd, desc);
    SOKOL_ASSERT((shd->slot.state == SG_RESOURCESTATE_VALID)||(shd->slot.state == SG_RESOURCESTATE_FAILED));
}

void sg_init_pipeline(sg_pipeline pip_id, const sg_pipeline_desc* desc) {
    SOKOL_ASSERT(_sg && pip_id.id != SG_INVALID_ID && desc);
    SOKOL_ASSERT((desc->_start_canary == 0) && (desc->_end_canary == 0));
    _sg_validate_pipeline_desc(desc);
    _sg_pipeline* pip = _sg_lookup_pipeline(&_sg->pools, pip_id.id);
    SOKOL_ASSERT(pip && pip->slot.state == SG_RESOURCESTATE_ALLOC);
    _sg_shader* shd = _sg_lookup_shader(&_sg->pools, desc->shader.id);
    SOKOL_ASSERT(shd && shd->slot.state == SG_RESOURCESTATE_VALID);
    _sg_create_pipeline(&_sg->backend, pip, shd, desc);
    SOKOL_ASSERT((pip->slot.state == SG_RESOURCESTATE_VALID)||(pip->slot.state == SG_RESOURCESTATE_FAILED)); 
}

void sg_init_pass(sg_pass pass_id, const sg_pass_desc* desc) {
    SOKOL_ASSERT(_sg && pass_id.id != SG_INVALID_ID && desc);
    SOKOL_ASSERT((desc->_start_canary == 0) && (desc->_end_canary == 0));
    _sg_validate_pass_desc(desc);
    _sg_pass* pass = _sg_lookup_pass(&_sg->pools, pass_id.id);
    SOKOL_ASSERT(pass && pass->slot.state == SG_RESOURCESTATE_ALLOC);
    /* lookup pass attachment image pointers */
    _sg_image* att_imgs[SG_MAX_COLOR_ATTACHMENTS + 1];
    for (int i = 0; i < SG_MAX_COLOR_ATTACHMENTS; i++) {
        if (desc->color_attachments[i].image.id) {
            att_imgs[i] = _sg_lookup_image(&_sg->pools, desc->color_attachments[i].image.id);
            SOKOL_ASSERT(att_imgs[i] && att_imgs[i]->slot.state == SG_RESOURCESTATE_VALID);
        }
        else {
            att_imgs[i] = 0;
        }
    }
    const int ds_att_index = SG_MAX_COLOR_ATTACHMENTS;
    if (desc->depth_stencil_attachment.image.id) {
        att_imgs[ds_att_index] = _sg_lookup_image(&_sg->pools, desc->depth_stencil_attachment.image.id);
        SOKOL_ASSERT(att_imgs[ds_att_index] && att_imgs[ds_att_index]->slot.state == SG_RESOURCESTATE_VALID);
    }
    else {
        att_imgs[ds_att_index] = 0;
    }
    _sg_create_pass(&_sg->backend, pass, att_imgs, desc);
    SOKOL_ASSERT((pass->slot.state == SG_RESOURCESTATE_VALID)||(pass->slot.state == SG_RESOURCESTATE_FAILED)); 
}

/*-- allocate and initialize resource ----------------------------------------*/
sg_buffer sg_make_buffer(const sg_buffer_desc* desc) {
    SOKOL_ASSERT(_sg && desc);
    sg_buffer buf_id = sg_alloc_buffer();
    if (buf_id.id != SG_INVALID_ID) {
        sg_init_buffer(buf_id, desc);
    }
    return buf_id;
}

sg_image sg_make_image(const sg_image_desc* desc) {
    SOKOL_ASSERT(_sg && desc);
    sg_image img_id = sg_alloc_image();
    if (img_id.id != SG_INVALID_ID) {
        sg_init_image(img_id, desc);
    }
    return img_id;
}

sg_shader sg_make_shader(const sg_shader_desc* desc) {
    SOKOL_ASSERT(_sg && desc);
    sg_shader shd_id = sg_alloc_shader();
    if (shd_id.id != SG_INVALID_ID) {
        sg_init_shader(shd_id, desc);
    }
    return shd_id;
}

sg_pipeline sg_make_pipeline(const sg_pipeline_desc* desc) {
    SOKOL_ASSERT(_sg && desc);
    sg_pipeline pip_id = sg_alloc_pipeline();
    if (pip_id.id != SG_INVALID_ID) {
        sg_init_pipeline(pip_id, desc);
    }
    return pip_id;
}

sg_pass sg_make_pass(const sg_pass_desc* desc) {
    SOKOL_ASSERT(_sg && desc);
    sg_pass pass_id = sg_alloc_pass();
    if (pass_id.id != SG_INVALID_ID) {
        sg_init_pass(pass_id, desc);
    }
    return pass_id;
}

/*-- destroy resource --------------------------------------------------------*/
void sg_destroy_buffer(sg_buffer buf_id) {
    SOKOL_ASSERT(_sg);
    _sg_buffer* buf = _sg_lookup_buffer(&_sg->pools, buf_id.id);
    if (buf) {
        _sg_destroy_buffer(&_sg->backend, buf);
        _sg_pool_free_id(&_sg->pools.buffer_pool, buf_id.id);
    }
}

void sg_destroy_image(sg_image img_id) {
    SOKOL_ASSERT(_sg);
    _sg_image* img = _sg_lookup_image(&_sg->pools, img_id.id);
    if (img) {
        _sg_destroy_image(&_sg->backend, img);
        _sg_pool_free_id(&_sg->pools.image_pool, img_id.id);
    }
}

void sg_destroy_shader(sg_shader shd_id) {
    SOKOL_ASSERT(_sg);
    _sg_shader* shd = _sg_lookup_shader(&_sg->pools, shd_id.id);
    if (shd) {
        _sg_destroy_shader(&_sg->backend, shd);
        _sg_pool_free_id(&_sg->pools.shader_pool, shd_id.id);
    }
}

void sg_destroy_pipeline(sg_pipeline pip_id) {
    SOKOL_ASSERT(_sg);
    _sg_pipeline* pip = _sg_lookup_pipeline(&_sg->pools, pip_id.id);
    if (pip) {
        _sg_destroy_pipeline(&_sg->backend, pip);
        _sg_pool_free_id(&_sg->pools.pipeline_pool, pip_id.id);
    }
}

void sg_destroy_pass(sg_pass pass_id) {
    SOKOL_ASSERT(_sg);
    _sg_pass* pass = _sg_lookup_pass(&_sg->pools, pass_id.id);
    if (pass) {
        _sg_destroy_pass(&_sg->backend, pass);
        _sg_pool_free_id(&_sg->pools.pass_pool, pass_id.id);
    }
}

void sg_begin_default_pass(const sg_pass_action* pass_action, int width, int height) {
    SOKOL_ASSERT(_sg && pass_action);
    SOKOL_ASSERT((pass_action->_start_canary == 0) && (pass_action->_end_canary == 0));
    sg_pass_action pa;
    _sg_resolve_default_pass_action(pass_action, &pa);
    _sg_begin_pass(&_sg->backend, 0, &pa, width, height);
}

void sg_begin_pass(sg_pass pass_id, const sg_pass_action* pass_action) {
    SOKOL_ASSERT(_sg && pass_action);
    SOKOL_ASSERT((pass_action->_start_canary == 0) && (pass_action->_end_canary == 0));
    _sg_pass* pass = _sg_lookup_pass(&_sg->pools, pass_id.id);
    SOKOL_ASSERT(pass && pass->slot.state == SG_RESOURCESTATE_VALID);
    sg_pass_action pa;
    _sg_resolve_default_pass_action(pass_action, &pa);
    _sg_validate_begin_pass(pass, &pa);
    const int w = pass->color_atts[0].image->width;
    const int h = pass->color_atts[0].image->height;
    _sg_begin_pass(&_sg->backend, pass, &pa, w, h);
}

void sg_apply_viewport(int x, int y, int width, int height, bool origin_top_left) {
    SOKOL_ASSERT(_sg);
    _sg_apply_viewport(&_sg->backend, x, y, width, height, origin_top_left);
}

void sg_apply_draw_state(const sg_draw_state* ds) {
    SOKOL_ASSERT(_sg && ds);
    SOKOL_ASSERT((ds->_start_canary==0) && (ds->_end_canary==0));
    _sg_validate_draw_state(ds);
    /* lookup resource pointers (lookups might yield 0, but this must be handled in backend) */
    _sg_pipeline* pip = _sg_lookup_pipeline(&_sg->pools, ds->pipeline.id);
    _sg_buffer* vbs[SG_MAX_SHADERSTAGE_BUFFERS] = { 0 };
    int num_vbs = 0;
    for (int i = 0; i < SG_MAX_SHADERSTAGE_BUFFERS; i++, num_vbs++) {
        if (ds->vertex_buffers[i].id) {
            vbs[i] = _sg_lookup_buffer(&_sg->pools, ds->vertex_buffers[i].id);
        }
        else {
            break;
        }
    }
    _sg_buffer* ib = _sg_lookup_buffer(&_sg->pools, ds->index_buffer.id);
    _sg_image* vs_imgs[SG_MAX_SHADERSTAGE_IMAGES] = { 0 };
    int num_vs_imgs = 0;
    for (int i = 0; i < SG_MAX_SHADERSTAGE_IMAGES; i++, num_vs_imgs++) {
        if (ds->vs_images[i].id) {
            vs_imgs[i] = _sg_lookup_image(&_sg->pools, ds->vs_images[i].id);
        }
        else {
            break;
        }
    }
    _sg_image* fs_imgs[SG_MAX_SHADERSTAGE_IMAGES] = { 0 };
    int num_fs_imgs = 0;
    for (int i = 0; i < SG_MAX_SHADERSTAGE_IMAGES; i++, num_fs_imgs++) {
        if (ds->fs_images[i].id) {
            fs_imgs[i] = _sg_lookup_image(&_sg->pools, ds->fs_images[i].id);
        }
        else {
            break;
        }
    }
    _sg->backend.next_draw_valid = _sg_validate_draw(pip, vbs, num_vbs, ib, vs_imgs, num_vs_imgs, fs_imgs, num_fs_imgs);
    if (_sg->backend.next_draw_valid) {
        _sg_apply_draw_state(&_sg->backend, pip, vbs, num_vbs, ib, vs_imgs, num_vs_imgs, fs_imgs, num_fs_imgs);
    }
}

void sg_apply_uniform_block(sg_shader_stage stage, int ub_index, const void* data, int num_bytes) {
    SOKOL_ASSERT((stage == SG_SHADERSTAGE_VS) || (stage == SG_SHADERSTAGE_FS));
    SOKOL_ASSERT((ub_index >= 0) && (ub_index < SG_MAX_SHADERSTAGE_UBS));
    SOKOL_ASSERT(data);
    SOKOL_ASSERT(num_bytes > 0);
    _sg_apply_uniform_block(&_sg->backend, stage, ub_index, data, num_bytes);
}

void sg_draw(int base_element, int num_elements, int num_instances) {
    SOKOL_ASSERT(_sg);
    _sg_draw(&_sg->backend, base_element, num_elements, num_instances);
}

void sg_end_pass() {
    SOKOL_ASSERT(_sg);
    _sg_end_pass(&_sg->backend);
}

void sg_commit() {
    SOKOL_ASSERT(_sg);
    _sg_commit(&_sg->backend);
} 

void sg_update_buffer(sg_buffer buf_id, const void* data, int num_bytes) {
    SOKOL_ASSERT(_sg && data);
    if (num_bytes > 0) {
        _sg_buffer* buf = _sg_lookup_buffer(&_sg->pools, buf_id.id);
        if (buf && buf->slot.state == SG_RESOURCESTATE_VALID) {
            _sg_update_buffer(&_sg->backend, buf, data, num_bytes);
        }
    }
}

#ifdef __cplusplus
} /* extern "C" */
#endif