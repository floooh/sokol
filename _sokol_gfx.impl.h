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
#if !(defined(SOKOL_USE_GLCORE33)||defined(SOKOL_USE_GLES2)||defined(SOKOL_USE_GLES3)||defined(SOKOL_USE_D3D11)||defined(SOKOL_USE_METAL))
#error "Please select a backend with SOKOL_USE_GLCORE33, SOKOL_USE_GLES2, SOKOL_USE_GLES3, SOKOL_USE_D3D11 or SOKOL_USE_METAL"
#endif

#include <string.h>

enum {
    _SG_INIT_GUARD = 0xC07FEFE,
    _SG_CONST_SLOT_SHIFT = 16,
    _SG_CONST_SLOT_MASK = (1<<_SG_CONST_SLOT_SHIFT)-1,
    _SG_CONST_MAX_POOL_SIZE = (1<<_SG_CONST_SLOT_SHIFT),
};

/*-- struct initializers -----------------------------------------------------*/
void sg_init_desc(sg_desc* desc) {
    SOKOL_ASSERT(desc);
    desc->_init_guard = _SG_INIT_GUARD;
    for (int i = 0; i < SG_NUM_RESOURCETYPES; i++) {
        desc->resource_pool_size[i] = 128;
    }
    /* shaders are the biggest object type, but usually don't need that many */
    desc->resource_pool_size[SG_RESOURCETYPE_SHADER] = 32;
}

void sg_init_buffer_desc(sg_buffer_desc* desc) {
    SOKOL_ASSERT(desc);
    desc->_init_guard = _SG_INIT_GUARD;
    desc->size = 0;
    desc->type = SG_BUFFERTYPE_VERTEXBUFFER;
    desc->usage = SG_USAGE_IMMUTABLE; 
    desc->data_ptr = 0;
    desc->data_size = 0;
}

void sg_init_image_desc(sg_image_desc* desc) {
    SOKOL_ASSERT(desc);
    desc->_init_guard = _SG_INIT_GUARD;
    desc->type = SG_IMAGETYPE_2D;
    desc->render_target = false;
    desc->width = 0;
    desc->height = 0;
    desc->depth = 1;
    desc->num_mipmaps = 1;
    desc->usage = SG_USAGE_IMMUTABLE;
    desc->color_format = SG_PIXELFORMAT_RGBA8;
    desc->depth_format = SG_PIXELFORMAT_NONE;
    desc->sample_count = 1;
    desc->min_filter = SG_FILTER_NEAREST;
    desc->mag_filter = SG_FILTER_NEAREST;
    desc->wrap_u = SG_WRAP_REPEAT;
    desc->wrap_v = SG_WRAP_REPEAT;
    desc->wrap_w = SG_WRAP_REPEAT;
    desc->num_data_items = 0;
    desc->data_ptrs = 0;
    desc->data_sizes = 0;
}

static void _sg_init_shader_stage_desc(sg_shader_stage_desc* desc) {
    SOKOL_ASSERT(desc);
    desc->source = 0;
    desc->num_ubs = 0;
    desc->num_images = 0;
    for (int ub_index = 0; ub_index < SG_MAX_SHADERSTAGE_UBS; ub_index++) {
        sg_shader_uniform_block_desc* ub_desc = &desc->ub[ub_index];
        ub_desc->size = 0;
        ub_desc->num_uniforms = 0;
        for (int u_index = 0; u_index < SG_MAX_UNIFORMS; u_index++) {
            sg_shader_uniform_desc* u_desc = &ub_desc->u[u_index];
            u_desc->name = 0;
            u_desc->offset = 0;
            u_desc->type = SG_UNIFORMTYPE_INVALID;
            u_desc->array_count = 1;
        }
    }
    for (int img_index = 0; img_index < SG_MAX_SHADERSTAGE_IMAGES; img_index++) {
        sg_shader_image_desc* img_desc = &desc->image[img_index];
        img_desc->name = 0;
        img_desc->type = SG_IMAGETYPE_INVALID;
    }
}

void sg_init_shader_desc(sg_shader_desc* desc) {
    SOKOL_ASSERT(desc);
    desc->_init_guard = _SG_INIT_GUARD;
    _sg_init_shader_stage_desc(&desc->vs);
    _sg_init_shader_stage_desc(&desc->fs);
}

void sg_init_uniform_block(sg_shader_desc* desc, sg_shader_stage stage, int ub_size) {
    SOKOL_ASSERT(desc);
    SOKOL_ASSERT((stage == SG_SHADERSTAGE_VS) || (stage == SG_SHADERSTAGE_FS));
    SOKOL_ASSERT(ub_size > 0);
    sg_shader_stage_desc* s = (stage == SG_SHADERSTAGE_VS) ? &desc->vs : &desc->fs;
    SOKOL_ASSERT(s->num_ubs < SG_MAX_SHADERSTAGE_UBS);
    SOKOL_ASSERT(s->ub[s->num_ubs].size == 0);
    s->ub[s->num_ubs++].size = ub_size;
}

void sg_init_named_uniform(sg_shader_desc* desc, sg_shader_stage stage, const char* name, int ub_offset, sg_uniform_type type, int array_count) {
    SOKOL_ASSERT(desc);
    SOKOL_ASSERT((stage == SG_SHADERSTAGE_VS) || (stage == SG_SHADERSTAGE_FS));
    SOKOL_ASSERT(name);
    SOKOL_ASSERT(ub_offset >= 0);
    SOKOL_ASSERT(type != SG_UNIFORMTYPE_INVALID);
    SOKOL_ASSERT(array_count >= 1);
    sg_shader_stage_desc* s = (stage == SG_SHADERSTAGE_VS) ? &desc->vs : &desc->fs;
    SOKOL_ASSERT(s->num_ubs >= 1);
    sg_shader_uniform_block_desc* ub = &(s->ub[s->num_ubs-1]);
    SOKOL_ASSERT(ub->num_uniforms < SG_MAX_UNIFORMS);
    sg_shader_uniform_desc* u_desc = &(ub->u[ub->num_uniforms++]);
    u_desc->name = name;
    u_desc->offset = ub_offset;
    u_desc->type = type;
    u_desc->array_count = array_count;
}

void sg_init_named_image(sg_shader_desc* desc, sg_shader_stage stage, const char* name, sg_image_type type) {
    SOKOL_ASSERT(desc);
    SOKOL_ASSERT((stage == SG_SHADERSTAGE_VS) || (stage == SG_SHADERSTAGE_FS));
    SOKOL_ASSERT(name);
    SOKOL_ASSERT(type != SG_IMAGETYPE_INVALID);
    sg_shader_stage_desc* s = (stage == SG_SHADERSTAGE_VS) ? &desc->vs : &desc->fs;
    SOKOL_ASSERT(s->num_images < SG_MAX_SHADERSTAGE_IMAGES);
    SOKOL_ASSERT(s->image[s->num_images].type == SG_IMAGETYPE_INVALID);
    sg_shader_image_desc* img_desc = &s->image[s->num_images++];
    img_desc->name = name;
    img_desc->type = type;
}

static void _sg_init_vertex_layout_desc(sg_vertex_layout_desc* layout) {
    SOKOL_ASSERT(layout);
    layout->stride = 0;
    layout->step_func = SG_STEPFUNC_PER_VERTEX;
    layout->step_rate = 1;
    layout->num_attrs = 0;
    for (int i = 0; i < SG_MAX_VERTEX_ATTRIBUTES; i++) {
        /* vertex attributes can be either defined by name
           or by bind slot index (on GLES2 only by name) */
        layout->attrs[i].name = 0;
        layout->attrs[i].index = -1;
        layout->attrs[i].offset = 0;
        layout->attrs[i].format = SG_VERTEXFORMAT_INVALID;
    }
}
static void _sg_init_stencil_state(sg_stencil_state* s) {
    SOKOL_ASSERT(s);
    s->fail_op = SG_STENCILOP_KEEP;
    s->depth_fail_op = SG_STENCILOP_KEEP;
    s->pass_op = SG_STENCILOP_KEEP;
    s->compare_func = SG_COMPAREFUNC_ALWAYS;
}
static void _sg_init_depth_stencil_state(sg_depth_stencil_state* s) {
    SOKOL_ASSERT(s);
    _sg_init_stencil_state(&s->stencil_front);
    _sg_init_stencil_state(&s->stencil_back);
    s->depth_compare_func = SG_COMPAREFUNC_ALWAYS;
    s->depth_write_enabled = false;
    s->stencil_enabled = false;
    s->stencil_read_mask = 0xFF;
    s->stencil_write_mask = 0xFF;
    s->stencil_ref = 0;
}
static void _sg_init_blend_state(sg_blend_state* s) {
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
        s->blend_color[i] = 1.0f;
    }
}
static void _sg_init_rasterizer_state(sg_rasterizer_state* s) {
    SOKOL_ASSERT(s);
    s->cull_face_enabled = false;
    s->scissor_test_enabled = false;
    s->dither_enabled = true;
    s->alpha_to_coverage_enabled = false;
    s->cull_face = SG_FACE_BACK;
    s->sample_count = 1;
}
void sg_init_pipeline_desc(sg_pipeline_desc* desc) {
    SOKOL_ASSERT(desc);
    desc->_init_guard = _SG_INIT_GUARD;
    desc->shader = SG_INVALID_ID;
    desc->primitive_type = SG_PRIMITIVETYPE_TRIANGLES;
    desc->index_type = SG_INDEXTYPE_NONE;
    for (int i = 0; i < SG_MAX_SHADERSTAGE_BUFFERS; i++) {
        _sg_init_vertex_layout_desc(&desc->input_layouts[i]);
    }
    _sg_init_depth_stencil_state(&desc->depth_stencil);
    _sg_init_blend_state(&desc->blend);
    _sg_init_rasterizer_state(&desc->rast);
}

void sg_init_vertex_stride(sg_pipeline_desc* desc, int input_slot, int stride) {
    SOKOL_ASSERT(desc);
    SOKOL_ASSERT(desc->_init_guard == _SG_INIT_GUARD);
    SOKOL_ASSERT((input_slot >= 0) && (input_slot < SG_MAX_SHADERSTAGE_BUFFERS));
    SOKOL_ASSERT(stride > 0);
    SOKOL_ASSERT((stride & 3) == 0);    /* must be multiple of 4 */
    desc->input_layouts[input_slot].stride = stride;
}

void sg_init_vertex_step(sg_pipeline_desc* desc, int input_slot, sg_step_func step_func, int step_rate) {
    SOKOL_ASSERT(desc);
    SOKOL_ASSERT(desc->_init_guard == _SG_INIT_GUARD);
    SOKOL_ASSERT((input_slot >= 0) && (input_slot < SG_MAX_SHADERSTAGE_BUFFERS));
    sg_vertex_layout_desc* layout = &desc->input_layouts[input_slot];
    layout->step_func = step_func;
    layout->step_rate = step_rate;
}

void sg_init_named_vertex_attr(sg_pipeline_desc* desc, int input_slot, const char* name, int offset, sg_vertex_format format) {
    SOKOL_ASSERT(desc);
    SOKOL_ASSERT(desc->_init_guard == _SG_INIT_GUARD);
    SOKOL_ASSERT((input_slot >= 0) && (input_slot < SG_MAX_SHADERSTAGE_BUFFERS));
    SOKOL_ASSERT(name);
    SOKOL_ASSERT(offset >= 0);
    SOKOL_ASSERT(format != SG_VERTEXFORMAT_INVALID);
    SOKOL_ASSERT(desc->input_layouts[input_slot].num_attrs < SG_MAX_VERTEX_ATTRIBUTES);
    sg_vertex_layout_desc* layout = &desc->input_layouts[input_slot];
    sg_vertex_attr_desc* attr = &layout->attrs[layout->num_attrs++];
    attr->name = name;
    attr->index = -1;
    attr->offset = offset;
    attr->format = format;
}

void sg_init_indexed_vertex_attr(sg_pipeline_desc* desc, int input_slot, int attr_index, int offset, sg_vertex_format format) {
    SOKOL_ASSERT(desc);
    SOKOL_ASSERT(desc->_init_guard == _SG_INIT_GUARD);
    SOKOL_ASSERT((input_slot >= 0) && (input_slot < SG_MAX_SHADERSTAGE_BUFFERS));
    SOKOL_ASSERT((attr_index >= 0) && (attr_index < SG_MAX_VERTEX_ATTRIBUTES));
    SOKOL_ASSERT(offset >= 0);
    SOKOL_ASSERT(format != SG_VERTEXFORMAT_INVALID);
    SOKOL_ASSERT(desc->input_layouts[input_slot].num_attrs < SG_MAX_VERTEX_ATTRIBUTES);
    sg_vertex_layout_desc* layout = &desc->input_layouts[input_slot];
    sg_vertex_attr_desc* attr = &layout->attrs[layout->num_attrs++];
    attr->name = 0;
    attr->index = attr_index;
    attr->offset = offset;
    attr->format = format;
}

void sg_init_pass_desc(sg_pass_desc* desc) {
    SOKOL_ASSERT(desc);
    desc->_init_guard = _SG_INIT_GUARD;
    sg_attachment_desc* att_desc = &desc->depth_stencil_attachment;
    att_desc->image = SG_INVALID_ID;
    att_desc->mip_level = 0;
    att_desc->slice = 0;
    for (int i = 0; i < SG_MAX_COLOR_ATTACHMENTS; i++) {
        att_desc = &desc->color_attachments[i];
        att_desc->image = SG_INVALID_ID;
        att_desc->mip_level = 0;
        att_desc->slice = 0;
    }
}

void sg_init_pass_action(sg_pass_action* pa) {
    SOKOL_ASSERT(pa);
    pa->_init_guard = _SG_INIT_GUARD;
    for (int att_index = 0; att_index < SG_MAX_COLOR_ATTACHMENTS; att_index++) {
        for (int c = 0; c < 3; c++) {
            pa->color[att_index][c] = 0.5f;
        }
        pa->color[att_index][3] = 1.0f;
    }
    pa->depth = 1.0f;
    pa->stencil = 0;
    pa->actions = SG_PASSACTION_CLEAR_ALL;
}

void sg_init_draw_state(sg_draw_state* ds) {
    SOKOL_ASSERT(ds);
    ds->_init_guard = _SG_INIT_GUARD;
    ds->pipeline = SG_INVALID_ID;
    for (int i = 0; i < SG_MAX_SHADERSTAGE_BUFFERS; i++) {
        ds->vertex_buffers[i] = SG_INVALID_ID;
    }
    ds->index_buffer = SG_INVALID_ID;
    for (int i = 0; i < SG_MAX_SHADERSTAGE_IMAGES; i++) {
        ds->vs_images[i] = SG_INVALID_ID;
        ds->fs_images[i] = SG_INVALID_ID;
    }
}

/*-- helper functions --------------------------------------------------------*/

/* return byte size of a vertex format */
static int _sg_vertexformat_bytesize(sg_vertex_format fmt) {
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
    }
}

/* return the byte size of a shader uniform */
static int _sg_uniform_size(sg_uniform_type type, int count) {
    switch (type) {
        case SG_UNIFORMTYPE_INVALID:    return 0;
        case SG_UNIFORMTYPE_FLOAT:      return 4 * count;
        case SG_UNIFORMTYPE_FLOAT2:     return 8 * count;
        case SG_UNIFORMTYPE_FLOAT3:     return 12 * count; /* FIXME: std140??? */
        case SG_UNIFORMTYPE_FLOAT4:     return 16 * count;
        case SG_UNIFORMTYPE_MAT4:       return 64 * count;
    }
}

/* return true if pixel format is a compressed format */
static bool _sg_is_compressed_pixel_format(sg_pixel_format fmt) {
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
static bool _sg_is_valid_rendertarget_color_format(sg_pixel_format fmt) {
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
static bool _sg_is_valid_rendertarget_depth_format(sg_pixel_format fmt) {
    switch (fmt) {
        case SG_PIXELFORMAT_DEPTH:
        case SG_PIXELFORMAT_DEPTHSTENCIL:
            return true;
        default:
            return false;
    }
}

/* return true if pixel format is a depth-stencil format */
static bool _sg_is_depth_stencil_format(sg_pixel_format fmt) {
    /* FIXME: more depth stencil formats? */
    return (SG_PIXELFORMAT_DEPTHSTENCIL == fmt);
}

/*-- resource pool slots (must be defined before rendering backend) ----------*/
typedef struct {
    sg_id id;
    sg_resource_state state;
} _sg_slot;

static void _sg_init_slot(_sg_slot* slot) {
    SOKOL_ASSERT(slot);
    slot->id = SG_INVALID_ID;
    slot->state = SG_RESOURCESTATE_INITIAL;
}

static int _sg_slot_index(sg_id id) {
    return id & _SG_CONST_SLOT_MASK;
}

//-- include the selected rendering backend ----------------------------------*/
#if defined(SOKOL_USE_GLCORE33) || defined(SOKOL_USE_GLES2) || defined(SOKOL_USE_GLES3)
#include "_sokol_gfx_gl.impl.h"
#elif defined(SOKOL_USE_D3D11)
#include "_sokol_gfx_d3d11.impl.h"
#elif defined(SOKOL_USE_METAL)
#include "_sokol_gfx_metal.impl.h"
#else
#error "No rendering backend selected"
#endif

/*-- resource pools ----------------------------------------------------------*/
typedef struct {
    int size;
    uint32_t unique_counter;
    int queue_top;
    int* free_queue;
} _sg_pool;

static void _sg_init_pool(_sg_pool* pool, int num) {
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

static void _sg_discard_pool(_sg_pool* pool) {
    SOKOL_ASSERT(pool);
    SOKOL_FREE(pool->free_queue);
    pool->free_queue = 0;
    pool->size = 0;
    pool->queue_top = 0;
    pool->unique_counter = 0;
}

static sg_id _sg_pool_alloc_id(_sg_pool* pool) {
    SOKOL_ASSERT(pool);
    SOKOL_ASSERT(pool->free_queue);
    if (pool->queue_top > 0) {
        int slot_index = pool->free_queue[--pool->queue_top];
        return ((pool->unique_counter++)<<_SG_CONST_SLOT_SHIFT)|slot_index;
    }
    else {
        /* pool exhausted */
        return SG_INVALID_ID;
    }
}

static void _sg_pool_free_id(_sg_pool* pool, sg_id id) {
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
    _sg_pool pool[SG_NUM_RESOURCETYPES];
    _sg_buffer* buffers;
    _sg_image* images;
    _sg_shader* shaders;
    _sg_pipeline* pipelines;
    _sg_pass* passes;
} _sg_pools;

static void _sg_setup_pools(_sg_pools* p, const sg_desc* desc) {
    SOKOL_ASSERT(p);
    SOKOL_ASSERT(desc);
    for (int res_type = 0; res_type < SG_NUM_RESOURCETYPES; res_type++) {
        SOKOL_ASSERT(desc->resource_pool_size[res_type] > 0);
        SOKOL_ASSERT(desc->resource_pool_size[res_type] < _SG_CONST_MAX_POOL_SIZE);
        _sg_init_pool(&p->pool[res_type], desc->resource_pool_size[res_type]);
    }
    /* note: the pools here will have an additional item, since slot 0 is reserved */
    p->buffers = SOKOL_MALLOC(sizeof(_sg_buffer) * p->pool[SG_RESOURCETYPE_BUFFER].size);
    for (int i = 0; i < p->pool[SG_RESOURCETYPE_BUFFER].size; i++) {
        _sg_init_buffer(&p->buffers[i]);
    }
    p->images = SOKOL_MALLOC(sizeof(_sg_image) * p->pool[SG_RESOURCETYPE_IMAGE].size);
    for (int i = 0; i < p->pool[SG_RESOURCETYPE_IMAGE].size; i++) {
        _sg_init_image(&p->images[i]);
    }
    p->shaders = SOKOL_MALLOC(sizeof(_sg_shader) * p->pool[SG_RESOURCETYPE_SHADER].size);
    for (int i = 0; i < p->pool[SG_RESOURCETYPE_SHADER].size; i++) {
        _sg_init_shader(&p->shaders[i]);
    }
    p->pipelines = SOKOL_MALLOC(sizeof(_sg_pipeline) * p->pool[SG_RESOURCETYPE_PIPELINE].size);
    for (int i = 0; i < p->pool[SG_RESOURCETYPE_PIPELINE].size; i++) {
        _sg_init_pipeline(&p->pipelines[i]);
    }
    p->passes = SOKOL_MALLOC(sizeof(_sg_pass) * p->pool[SG_RESOURCETYPE_PASS].size);
    for (int i = 0; i < p->pool[SG_RESOURCETYPE_PASS].size; i++) {
        _sg_init_pass(&p->passes[i]);
    }
}

static void _sg_discard_pools(_sg_pools* p) {
    SOKOL_ASSERT(p);
    SOKOL_FREE(p->passes);      p->passes = 0;
    SOKOL_FREE(p->pipelines);   p->pipelines = 0;
    SOKOL_FREE(p->shaders);     p->shaders = 0;
    SOKOL_FREE(p->images);      p->images = 0;
    SOKOL_FREE(p->buffers);     p->buffers = 0;
    for (int res_type = 0; res_type < SG_NUM_RESOURCETYPES; res_type++) {
        _sg_discard_pool(&p->pool[res_type]);
    }
}

/* returns pointer to resource by id without matching id check */
static _sg_buffer* _sg_buffer_at(const _sg_pools* p, sg_id buf_id) {
    SOKOL_ASSERT(p && SG_INVALID_ID != buf_id);
    int slot_index = _sg_slot_index(buf_id);
    SOKOL_ASSERT((slot_index >= 0) && (slot_index < p->pool[SG_RESOURCETYPE_BUFFER].size));
    return &p->buffers[slot_index];
}

static _sg_image* _sg_image_at(const _sg_pools* p, sg_id img_id) {
    SOKOL_ASSERT(p && SG_INVALID_ID != img_id);
    int slot_index = _sg_slot_index(img_id);
    SOKOL_ASSERT((slot_index >= 0) && (slot_index < p->pool[SG_RESOURCETYPE_IMAGE].size));
    return &p->images[slot_index];
}

static _sg_shader* _sg_shader_at(const _sg_pools* p, sg_id shd_id) {
    SOKOL_ASSERT(p && SG_INVALID_ID != shd_id);
    int slot_index = _sg_slot_index(shd_id);
    SOKOL_ASSERT((slot_index >= 0) && (slot_index < p->pool[SG_RESOURCETYPE_SHADER].size));
    return &p->shaders[slot_index];
}

static _sg_pipeline* _sg_pipeline_at(const _sg_pools* p, sg_id pip_id) {
    SOKOL_ASSERT(p && SG_INVALID_ID != pip_id);
    int slot_index = _sg_slot_index(pip_id);
    SOKOL_ASSERT((slot_index >= 0) && (slot_index < p->pool[SG_RESOURCETYPE_PIPELINE].size));
    return &p->pipelines[slot_index];
}

static _sg_pass* _sg_pass_at(const _sg_pools* p, sg_id pass_id) {
    SOKOL_ASSERT(p && SG_INVALID_ID != pass_id);
    int slot_index = _sg_slot_index(pass_id);
    SOKOL_ASSERT((slot_index >= 0) && (slot_index < p->pool[SG_RESOURCETYPE_PASS].size));
    return &p->passes[slot_index];
}

/* returns pointer to buffer with matching id check, may return 0 */
static _sg_buffer* _sg_lookup_buffer(const _sg_pools* p, sg_id buf_id) {
    if (SG_INVALID_ID != buf_id) {
        _sg_buffer* buf = _sg_buffer_at(p, buf_id);
        if (buf->slot.id == buf_id) {
            return buf;
        }
    }
    return 0;
}

static _sg_image* _sg_lookup_image(const _sg_pools* p, sg_id img_id) {
    if (SG_INVALID_ID != img_id) {
        _sg_image* img = _sg_image_at(p, img_id); 
        if (img->slot.id == img_id) {
            return img;
        }
    }
    return 0;
}

static _sg_shader* _sg_lookup_shader(const _sg_pools* p, sg_id shd_id) {
    SOKOL_ASSERT(p);
    if (SG_INVALID_ID != shd_id) {
        _sg_shader* shd = _sg_shader_at(p, shd_id);
        if (shd->slot.id == shd_id) {
            return shd;
        }
    }
    return 0;
}

static _sg_pipeline* _sg_lookup_pipeline(const _sg_pools* p, sg_id pip_id) {
    SOKOL_ASSERT(p);
    if (SG_INVALID_ID != pip_id) {
        _sg_pipeline* pip = _sg_pipeline_at(p, pip_id);
        if (pip->slot.id == pip_id) {
            return pip;
        }
    }
    return 0;
}

static _sg_pass* _sg_lookup_pass(const _sg_pools* p, sg_id pass_id) {
    SOKOL_ASSERT(p);
    if (SG_INVALID_ID != pass_id) {
        _sg_pass* pass = _sg_pass_at(p, pass_id);
        if (pass->slot.id == pass_id) {
            return pass;
        }
    }
    return 0;
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
    SOKOL_ASSERT(desc->_init_guard == _SG_INIT_GUARD);
    _sg = SOKOL_MALLOC(sizeof(_sg_state));
    _sg_setup_pools(&_sg->pools, desc);
    _sg_setup_backend(&_sg->backend);
}

void sg_shutdown() {
    SOKOL_ASSERT(_sg);
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
sg_id sg_alloc_buffer() {
    SOKOL_ASSERT(_sg);
    sg_id buf_id = _sg_pool_alloc_id(&_sg->pools.pool[SG_RESOURCETYPE_BUFFER]);
    if (buf_id != SG_INVALID_ID) {
        _sg_buffer* buf = _sg_buffer_at(&_sg->pools, buf_id);
        SOKOL_ASSERT(buf && (buf->slot.state == SG_RESOURCESTATE_INITIAL) && (buf->slot.id == SG_INVALID_ID));
        buf->slot.id = buf_id;
        buf->slot.state = SG_RESOURCESTATE_ALLOC;
    }
    return buf_id;
}

sg_id sg_alloc_image() {
    SOKOL_ASSERT(_sg);
    sg_id img_id = _sg_pool_alloc_id(&_sg->pools.pool[SG_RESOURCETYPE_IMAGE]);
    if (img_id != SG_INVALID_ID) {
        _sg_image* img = _sg_image_at(&_sg->pools, img_id);
        SOKOL_ASSERT(img && (img->slot.state == SG_RESOURCESTATE_INITIAL) && (img->slot.id == SG_INVALID_ID));
        img->slot.id = img_id;
        img->slot.state = SG_RESOURCESTATE_ALLOC;
    }
    return img_id;
}

sg_id sg_alloc_shader() {
    SOKOL_ASSERT(_sg);
    sg_id shd_id = _sg_pool_alloc_id(&_sg->pools.pool[SG_RESOURCETYPE_SHADER]);
    if (shd_id != SG_INVALID_ID) {
        _sg_shader* shd = _sg_shader_at(&_sg->pools, shd_id);
        SOKOL_ASSERT(shd && (shd->slot.state == SG_RESOURCESTATE_INITIAL) && (shd->slot.id == SG_INVALID_ID));
        shd->slot.id = shd_id;
        shd->slot.state = SG_RESOURCESTATE_ALLOC;
    }
    return shd_id;
}

sg_id sg_alloc_pipeline() {
    SOKOL_ASSERT(_sg);
    sg_id pip_id = _sg_pool_alloc_id(&_sg->pools.pool[SG_RESOURCETYPE_PIPELINE]);
    if (pip_id != SG_INVALID_ID) {
        _sg_pipeline* pip = _sg_pipeline_at(&_sg->pools, pip_id);
        SOKOL_ASSERT(pip && (pip->slot.state == SG_RESOURCESTATE_INITIAL) && (pip->slot.id == SG_INVALID_ID));
        pip->slot.id = pip_id;
        pip->slot.state = SG_RESOURCESTATE_ALLOC;
    }
    return pip_id;
}

sg_id sg_alloc_pass() {
    SOKOL_ASSERT(_sg);
    sg_id pass_id = _sg_pool_alloc_id(&_sg->pools.pool[SG_RESOURCETYPE_PASS]);
    if (pass_id != SG_INVALID_ID) {
        _sg_pass* pass = _sg_pass_at(&_sg->pools, pass_id);
        SOKOL_ASSERT(pass && (pass->slot.state == SG_RESOURCESTATE_INITIAL) && (pass->slot.id == SG_INVALID_ID));
        pass->slot.id = pass_id;
        pass->slot.state = SG_RESOURCESTATE_ALLOC;
    }
    return pass_id;
}

/*-- validate description structs --------------------------------------------*/
static void _sg_validate_buffer_desc(const sg_buffer_desc* desc) {
    SOKOL_ASSERT(_sg && desc);
    SOKOL_ASSERT(desc->size > 0);
    SOKOL_ASSERT((desc->type==SG_BUFFERTYPE_VERTEXBUFFER)||(desc->type==SG_BUFFERTYPE_INDEXBUFFER));
    SOKOL_ASSERT((desc->usage==SG_USAGE_IMMUTABLE)||(desc->usage==SG_USAGE_DYNAMIC)||(desc->usage==SG_USAGE_STREAM));
    SOKOL_ASSERT(desc->data_size <= desc->size);
    #ifdef SOKOL_DEBUG 
    if (desc->usage == SG_USAGE_IMMUTABLE) {
        SOKOL_ASSERT(desc->data_ptr);
    }
    #endif
}

static void _sg_validate_image_desc(const sg_image_desc* desc) {
    SOKOL_ASSERT(_sg && desc);
    SOKOL_ASSERT((desc->type==SG_IMAGETYPE_2D)||(desc->type==SG_IMAGETYPE_CUBE)||(desc->type==SG_IMAGETYPE_3D)||(desc->type==SG_IMAGETYPE_ARRAY));
    SOKOL_ASSERT((desc->width > 0) && (desc->height > 0) && (desc->depth > 0));
    SOKOL_ASSERT((desc->num_mipmaps > 0) && (desc->num_mipmaps <= SG_MAX_MIPMAPS));
    SOKOL_ASSERT((desc->usage==SG_USAGE_IMMUTABLE)||(desc->usage==SG_USAGE_STREAM)||(desc->usage==SG_USAGE_DYNAMIC));
    SOKOL_ASSERT((desc->color_format != SG_PIXELFORMAT_NONE));
    SOKOL_ASSERT(desc->sample_count >= 1);
    #if SOKOL_DEBUG
    if (desc->render_target) {
        SOKOL_ASSERT(desc->usage == SG_USAGE_IMMUTABLE);
        SOKOL_ASSERT(_sg_is_valid_rendertarget_color_format(desc->color_format));
        if (desc->depth_format != SG_PIXELFORMAT_NONE) {
            SOKOL_ASSERT(_sg_is_valid_rendertarget_depth_format(desc->depth_format));
        }
        SOKOL_ASSERT((0 == desc->num_data_items) && (0 == desc->data_ptrs) && (0 == desc->data_sizes));
    }
    else {
        SOKOL_ASSERT(desc->depth_format == SG_PIXELFORMAT_NONE);
    }
    if (desc->num_data_items > 0) {
        SOKOL_ASSERT(desc->data_ptrs);
        SOKOL_ASSERT(desc->data_sizes);
    }
    #endif
}

static void _sg_validate_shader_desc(const sg_shader_desc* desc) {
    SOKOL_ASSERT(_sg && desc);
    #if defined(SOKOL_USE_GLCORE33) || defined(SOKOL_USE_GLES2) || defined(SOKOL_USE_GLES3)
    SOKOL_ASSERT(desc->vs.source);
    SOKOL_ASSERT(desc->fs.source);
    #endif
    #ifdef SOKOL_DEBUG
    for (int i = 0; i < SG_NUM_SHADER_STAGES; i++) {
        const sg_shader_stage_desc* stage_desc = (i == 0)? &desc->vs : &desc->fs;
        SOKOL_ASSERT((stage_desc->num_ubs >= 0) && (stage_desc->num_ubs <= SG_MAX_SHADERSTAGE_UBS));
        for (int ub_index = 0; ub_index < stage_desc->num_ubs; ub_index++) {
            const sg_shader_uniform_block_desc* ub_desc = &stage_desc->ub[ub_index];
            SOKOL_ASSERT(ub_desc->size > 0);
            SOKOL_ASSERT((ub_desc->num_uniforms > 0) && (ub_desc->num_uniforms <= SG_MAX_UNIFORMS));
            for (int u_index = 0; u_index < ub_desc->num_uniforms; u_index++) {
                const sg_shader_uniform_desc* u_desc = &ub_desc->u[u_index];
                SOKOL_ASSERT(u_desc->type != SG_UNIFORMTYPE_INVALID);
                #ifdef SOKOL_USE_GLES2
                SOKOL_ASSERT(u_desc->name);
                #endif
                SOKOL_ASSERT(u_desc->array_count >= 1);
                SOKOL_ASSERT(u_desc->offset >= 0);
                SOKOL_ASSERT((u_desc->offset + _sg_uniform_size(u_desc->type, u_desc->array_count)) <= ub_desc->size);
            }
        }
    }
    #endif
}

static void _sg_validate_pipeline_desc(const sg_pipeline_desc* desc) {
    SOKOL_ASSERT(_sg && desc);
    SOKOL_ASSERT(desc->shader != SG_INVALID_ID);
    SOKOL_ASSERT(desc->input_layouts[0].num_attrs > 0);
    #ifdef SOKOL_DEBUG
    int num_attrs = 0;
    for (int i = 0; i < SG_MAX_SHADERSTAGE_BUFFERS; i++) {
        const sg_vertex_layout_desc* layout_desc = &desc->input_layouts[i];
        num_attrs += layout_desc->num_attrs;
        if (layout_desc->num_attrs > 0) {
            SOKOL_ASSERT(layout_desc->stride > 0);
            SOKOL_ASSERT((layout_desc->stride & 3) == 0);
        }
    }
    SOKOL_ASSERT(num_attrs < SG_MAX_VERTEX_ATTRIBUTES);
    #endif
}

static void _sg_validate_pass_desc(const sg_pass_desc* desc) {
    SOKOL_ASSERT(desc->color_attachments[0].image != SG_INVALID_ID);
}

static void _sg_validate_draw_state(const sg_draw_state* ds) {
    SOKOL_ASSERT(_sg && ds);
    SOKOL_ASSERT(ds->pipeline);
    SOKOL_ASSERT(ds->vertex_buffers[0]);    
}

static void _sg_validate_begin_pass(const _sg_pass* pass, const sg_pass_action* pass_action) {
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
            SOKOL_ASSERT(att->image->slot.id == att->image_id);
            /* all images must be render target */
            SOKOL_ASSERT(att->image->render_target);
            /* all images must be immutable */
            SOKOL_ASSERT(att->image->usage == SG_USAGE_IMMUTABLE);
            /* all images must have same size */
            SOKOL_ASSERT(att->image->width == img->width);
            SOKOL_ASSERT(att->image->height == img->height);
            /* all images must have same pixel format */
            SOKOL_ASSERT(att->image->color_format == img->color_format);
            /* must be a valid color render target pixel format */
            SOKOL_ASSERT(_sg_is_valid_rendertarget_color_format(att->image->color_format));
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
        SOKOL_ASSERT(ds_att->image->slot.id == ds_att->image_id);
        SOKOL_ASSERT(ds_att->image->render_target);
        SOKOL_ASSERT(ds_att->image->usage == SG_USAGE_IMMUTABLE);
        SOKOL_ASSERT(ds_att->image->width == img->width);
        SOKOL_ASSERT(ds_att->image->height == img->height);
        SOKOL_ASSERT(_sg_is_valid_rendertarget_depth_format(ds_att->image->depth_format));
    }
    #endif /* SOKOL_DEBUG */
}

static bool _sg_validate_draw(_sg_pipeline* pip, 
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
    if (pip->shader->slot.id != pip->shader_id) {
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
            /* image exists, but not valid for for rendering */
            return false;
        }
        SOKOL_ASSERT(img->type == pip->shader->stage[SG_SHADERSTAGE_FS].images[i].type);
    }
    /* all ok for rendering! */
    return true;
}

/*-- initialize an allocated resource ----------------------------------------*/
void sg_init_buffer(sg_id buf_id, const sg_buffer_desc* desc) {
    SOKOL_ASSERT(_sg && buf_id != SG_INVALID_ID && desc);
    SOKOL_ASSERT(desc->_init_guard == _SG_INIT_GUARD);
    _sg_validate_buffer_desc(desc);
    _sg_buffer* buf = _sg_lookup_buffer(&_sg->pools, buf_id);
    SOKOL_ASSERT(buf && buf->slot.state == SG_RESOURCESTATE_ALLOC);
    _sg_create_buffer(&_sg->backend, buf, desc);
    SOKOL_ASSERT((buf->slot.state == SG_RESOURCESTATE_VALID)||(buf->slot.state == SG_RESOURCESTATE_FAILED));
}

void sg_init_image(sg_id img_id, const sg_image_desc* desc) {
    SOKOL_ASSERT(_sg && img_id != SG_INVALID_ID && desc);
    SOKOL_ASSERT(desc->_init_guard == _SG_INIT_GUARD);
    _sg_validate_image_desc(desc);
    _sg_image* img = _sg_lookup_image(&_sg->pools, img_id);
    SOKOL_ASSERT(img && img->slot.state == SG_RESOURCESTATE_ALLOC);
    _sg_create_image(&_sg->backend, img, desc);
    SOKOL_ASSERT((img->slot.state == SG_RESOURCESTATE_VALID)||(img->slot.state == SG_RESOURCESTATE_FAILED));
}

void sg_init_shader(sg_id shd_id, const sg_shader_desc* desc) {
    SOKOL_ASSERT(_sg && shd_id != SG_INVALID_ID && desc);
    SOKOL_ASSERT(desc->_init_guard == _SG_INIT_GUARD);
    _sg_validate_shader_desc(desc);
    _sg_shader* shd = _sg_lookup_shader(&_sg->pools, shd_id);
    SOKOL_ASSERT(shd && shd->slot.state == SG_RESOURCESTATE_ALLOC);
    _sg_create_shader(&_sg->backend, shd, desc);
    SOKOL_ASSERT((shd->slot.state == SG_RESOURCESTATE_VALID)||(shd->slot.state == SG_RESOURCESTATE_FAILED));
}

void sg_init_pipeline(sg_id pip_id, const sg_pipeline_desc* desc) {
    SOKOL_ASSERT(_sg && pip_id != SG_INVALID_ID && desc);
    SOKOL_ASSERT(desc->_init_guard == _SG_INIT_GUARD);
    _sg_validate_pipeline_desc(desc);
    _sg_pipeline* pip = _sg_lookup_pipeline(&_sg->pools, pip_id);
    SOKOL_ASSERT(pip && pip->slot.state == SG_RESOURCESTATE_ALLOC);
    _sg_shader* shd = _sg_lookup_shader(&_sg->pools, desc->shader);
    SOKOL_ASSERT(shd && shd->slot.state == SG_RESOURCESTATE_VALID);
    _sg_create_pipeline(&_sg->backend, pip, shd, desc);
    SOKOL_ASSERT((pip->slot.state == SG_RESOURCESTATE_VALID)||(pip->slot.state == SG_RESOURCESTATE_FAILED)); 
}

void sg_init_pass(sg_id pass_id, const sg_pass_desc* desc) {
    SOKOL_ASSERT(_sg && pass_id != SG_INVALID_ID && desc);
    SOKOL_ASSERT(desc->_init_guard == _SG_INIT_GUARD);
    _sg_validate_pass_desc(desc);
    _sg_pass* pass = _sg_lookup_pass(&_sg->pools, pass_id);
    SOKOL_ASSERT(pass && pass->slot.state == SG_RESOURCESTATE_ALLOC);
    /* lookup pass attachment image pointers */
    _sg_image* att_imgs[SG_MAX_COLOR_ATTACHMENTS + 1];
    for (int i = 0; i < SG_MAX_COLOR_ATTACHMENTS; i++) {
        if (desc->color_attachments[i].image) {
            att_imgs[i] = _sg_lookup_image(&_sg->pools, desc->color_attachments[i].image);
            SOKOL_ASSERT(att_imgs[i] && att_imgs[i]->slot.state == SG_RESOURCESTATE_VALID);
        }
        else {
            att_imgs[i] = 0;
        }
    }
    const int ds_att_index = SG_MAX_COLOR_ATTACHMENTS;
    if (desc->depth_stencil_attachment.image) {
        att_imgs[ds_att_index] = _sg_lookup_image(&_sg->pools, desc->depth_stencil_attachment.image);
        SOKOL_ASSERT(att_imgs[ds_att_index] && att_imgs[ds_att_index]->slot.state == SG_RESOURCESTATE_VALID);
    }
    else {
        att_imgs[ds_att_index] = 0;
    }
    _sg_create_pass(&_sg->backend, pass, att_imgs, desc);
    SOKOL_ASSERT((pass->slot.state == SG_RESOURCESTATE_VALID)||(pass->slot.state == SG_RESOURCESTATE_FAILED)); 
}

/*-- allocate and initialize resource ----------------------------------------*/
sg_id sg_make_buffer(const sg_buffer_desc* desc) {
    SOKOL_ASSERT(_sg && desc);
    sg_id buf_id = sg_alloc_buffer();
    if (buf_id != SG_INVALID_ID) {
        sg_init_buffer(buf_id, desc);
    }
    return buf_id;
}

sg_id sg_make_image(const sg_image_desc* desc) {
    SOKOL_ASSERT(_sg && desc);
    sg_id img_id = sg_alloc_image();
    if (img_id != SG_INVALID_ID) {
        sg_init_image(img_id, desc);
    }
    return img_id;
}

sg_id sg_make_shader(const sg_shader_desc* desc) {
    SOKOL_ASSERT(_sg && desc);
    sg_id shd_id = sg_alloc_shader();
    if (shd_id != SG_INVALID_ID) {
        sg_init_shader(shd_id, desc);
    }
    return shd_id;
}

sg_id sg_make_pipeline(const sg_pipeline_desc* desc) {
    SOKOL_ASSERT(_sg && desc);
    sg_id pip_id = sg_alloc_pipeline();
    if (pip_id != SG_INVALID_ID) {
        sg_init_pipeline(pip_id, desc);
    }
    return pip_id;
}

sg_id sg_make_pass(const sg_pass_desc* desc) {
    SOKOL_ASSERT(_sg && desc);
    sg_id pass_id = sg_alloc_pass();
    if (pass_id != SG_INVALID_ID) {
        sg_init_pass(pass_id, desc);
    }
    return pass_id;
}

/*-- destroy resource --------------------------------------------------------*/
void sg_destroy_buffer(sg_id buf_id) {
    SOKOL_ASSERT(_sg);
    _sg_buffer* buf = _sg_lookup_buffer(&_sg->pools, buf_id);
    if (buf) {
        _sg_destroy_buffer(&_sg->backend, buf);
        _sg_pool_free_id(&_sg->pools.pool[SG_RESOURCETYPE_BUFFER], buf_id);
    }
}

void sg_destroy_image(sg_id img_id) {
    SOKOL_ASSERT(_sg);
    _sg_image* img = _sg_lookup_image(&_sg->pools, img_id);
    if (img) {
        _sg_destroy_image(&_sg->backend, img);
        _sg_pool_free_id(&_sg->pools.pool[SG_RESOURCETYPE_IMAGE], img_id);
    }
}

void sg_destroy_shader(sg_id shd_id) {
    SOKOL_ASSERT(_sg);
    _sg_shader* shd = _sg_lookup_shader(&_sg->pools, shd_id);
    if (shd) {
        _sg_destroy_shader(&_sg->backend, shd);
        _sg_pool_free_id(&_sg->pools.pool[SG_RESOURCETYPE_SHADER], shd_id);
    }
}

void sg_destroy_pipeline(sg_id pip_id) {
    SOKOL_ASSERT(_sg);
    _sg_pipeline* pip = _sg_lookup_pipeline(&_sg->pools, pip_id);
    if (pip) {
        _sg_destroy_pipeline(&_sg->backend, pip);
        _sg_pool_free_id(&_sg->pools.pool[SG_RESOURCETYPE_PIPELINE], pip_id);
    }
}

void sg_destroy_pass(sg_id pass_id) {
    SOKOL_ASSERT(_sg);
    _sg_pass* pass = _sg_lookup_pass(&_sg->pools, pass_id);
    if (pass) {
        _sg_destroy_pass(&_sg->backend, pass);
        _sg_pool_free_id(&_sg->pools.pool[SG_RESOURCETYPE_PASS], pass_id);
    }
}

void sg_begin_default_pass(const sg_pass_action* pass_action, int width, int height) {
    SOKOL_ASSERT(_sg && pass_action);
    SOKOL_ASSERT(pass_action->_init_guard == _SG_INIT_GUARD);
    _sg_begin_pass(&_sg->backend, 0, pass_action, width, height);
}

void sg_begin_pass(sg_id pass_id, const sg_pass_action* pass_action) {
    SOKOL_ASSERT(_sg && pass_action);
    SOKOL_ASSERT(pass_action->_init_guard == _SG_INIT_GUARD);
    _sg_pass* pass = _sg_lookup_pass(&_sg->pools, pass_id);
    SOKOL_ASSERT(pass && pass->slot.state == SG_RESOURCESTATE_VALID);
    _sg_validate_begin_pass(pass, pass_action);
    const int w = pass->color_atts[0].image->width;
    const int h = pass->color_atts[0].image->height;
    _sg_begin_pass(&_sg->backend, pass, pass_action, w, h);
}

void sg_apply_draw_state(const sg_draw_state* ds) {
    SOKOL_ASSERT(_sg && ds);
    SOKOL_ASSERT(ds->_init_guard == _SG_INIT_GUARD);
    _sg_validate_draw_state(ds);
    /* lookup resource pointers (lookups might yield 0, but this must be handled in backend) */
    _sg_pipeline* pip = _sg_lookup_pipeline(&_sg->pools, ds->pipeline);
    _sg_buffer* vbs[SG_MAX_SHADERSTAGE_BUFFERS] = { 0 };
    int num_vbs = 0;
    for (int i = 0; i < SG_MAX_SHADERSTAGE_BUFFERS; i++, num_vbs++) {
        if (ds->vertex_buffers[i]) {
            vbs[i] = _sg_lookup_buffer(&_sg->pools, ds->vertex_buffers[i]);
        }
        else {
            break;
        }
    }
    _sg_buffer* ib = _sg_lookup_buffer(&_sg->pools, ds->index_buffer);
    _sg_image* vs_imgs[SG_MAX_SHADERSTAGE_IMAGES] = { 0 };
    int num_vs_imgs = 0;
    for (int i = 0; i < SG_MAX_SHADERSTAGE_IMAGES; i++, num_vs_imgs++) {
        if (ds->vs_images[i]) {
            vs_imgs[i] = _sg_lookup_image(&_sg->pools, ds->vs_images[i]);
        }
        else {
            break;
        }
    }
    _sg_image* fs_imgs[SG_MAX_SHADERSTAGE_IMAGES] = { 0 };
    int num_fs_imgs = 0;
    for (int i = 0; i < SG_MAX_SHADERSTAGE_IMAGES; i++, num_fs_imgs++) {
        if (ds->fs_images[i]) {
            fs_imgs[i] = _sg_lookup_image(&_sg->pools, ds->fs_images[i]);
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

void sg_update_buffer(sg_id buf_id, const void* data, int num_bytes) {
    SOKOL_ASSERT(_sg && data);
    if (num_bytes > 0) {
        _sg_buffer* buf = _sg_lookup_buffer(&_sg->pools, buf_id);
        if (buf && buf->slot.state == SG_RESOURCESTATE_VALID) {
            _sg_update_buffer(&_sg->backend, buf, data, num_bytes);
        }
    }
}
