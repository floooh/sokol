/*
    Sokol Gfx generic implementation code
*/
typedef enum {
    _SG_SLOT_STATE_FREE,
    _SG_SLOT_STATE_ALLOC,
    _SG_SLOT_STATE_VALID,
} _sg_slot_state;

typedef struct {
    sg_id id;
    sg_label label;
    _sg_slot_state state;
} _sg_slot;

static void _sg_init_slot(_sg_slot* slot) {
    SOKOL_ASSERT(slot);
    slot->id = SG_INVALID_ID;
    slot->label = SG_INVALID_LABEL;
    slot->state = _SG_SLOT_STATE_FREE;
}

static int _sg_slot_index(sg_id id) {
    return id & SG_CONST_SLOT_MASK;
}

#ifdef SOKOL_GFX_USE_GL
#include "_sokol_gfx_gl.inl"
#elif SOKOL_GFX_USE_D3D11
#include "_sokol_gfx_d3d11.inl"
#elif SOKOL_GFX_USE_METAL
#include "_sokol_gfx_metal.inl"
#else
#error "Please select rendering backend by definining SOKOL_GFX_USE_GL, SOKOL_GFX_USE_D3D11 or SOKOL_GFX_USE_METAL"
#endif

//------------------------------------------------------------------------------
typedef struct {
    int num;
    int cur;
    uint32_t unique_counter;
    int* free_queue;
} _sg_pool;

static void _sg_init_pool(_sg_pool* pool, int num) {
    SOKOL_ASSERT(pool && (num > 0));
    pool->num = num;
    pool->cur = 0;
    pool->unique_counter = 0;
    pool->free_queue = SOKOL_MALLOC(sizeof(int)*num);
    for (int i = num-1; i >= 0; i--) {
        pool->free_queue[pool->cur++] = i;
    }
}

static void _sg_destroy_pool(_sg_pool* pool) {
    SOKOL_ASSERT(pool);
    SOKOL_FREE(pool->free_queue);
    pool->free_queue = 0;
    pool->num = 0;
    pool->cur = 0;
    pool->unique_counter = 0;
}

static sg_id _sg_pool_alloc_id(_sg_pool* pool) {
    SOKOL_ASSERT(pool);
    SOKOL_ASSERT(pool->cur > 0);
    SOKOL_ASSERT(pool->free_queue);
    int slot_index = pool->free_queue[--pool->cur];
    return ((pool->unique_counter++)<<SG_CONST_SLOT_SHIFT)|slot_index;
}

static void _sg_pool_free_id(_sg_pool* pool, sg_id id) {
    SOKOL_ASSERT(id != SG_INVALID_ID);
    SOKOL_ASSERT(pool);
    SOKOL_ASSERT(pool->free_queue);
    SOKOL_ASSERT(pool->cur < pool->num);
    #if _DEBUG
    int slot_index = _sg_slot_index(id);
    for (int i = 0; i < pool->cur; i++) {
        SOKOL_ASSERT(pool->free_queue[i] != slot_index);
    }
    #endif
    pool->free_queue[pool->cur++] = id;
}

//------------------------------------------------------------------------------
void sg_init_setup_desc(sg_setup_desc* desc) {
    SOKOL_ASSERT(desc);
    desc->width = 640;
    desc->height = 400;
    desc->sample_count = 1;
    desc->buffer_pool_size = 128;
    desc->image_pool_size = 128;
    desc->shader_pool_size = 128;
    desc->pipeline_pool_size = 128;
    desc->pass_pool_size = 128;
    desc->label_stack_size = 32;
    //FIXME!
    //desc->default_pass_action
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
} _sg_state;
_sg_state* _sg = 0;

void sg_setup(sg_setup_desc* desc) {
    SOKOL_ASSERT(!_sg);
    SOKOL_ASSERT(desc);
    SOKOL_ASSERT((desc->width > 0) && (desc->height > 0));
    SOKOL_ASSERT(desc->sample_count >= 1);
    SOKOL_ASSERT((desc->buffer_pool_size > 0) && (desc->buffer_pool_size < SG_CONST_MAX_POOL_SIZE));
    SOKOL_ASSERT((desc->image_pool_size > 0) && (desc->image_pool_size < SG_CONST_MAX_POOL_SIZE));
    SOKOL_ASSERT((desc->shader_pool_size > 0) && (desc->shader_pool_size < SG_CONST_MAX_POOL_SIZE));
    SOKOL_ASSERT((desc->pipeline_pool_size > 0) && (desc->pipeline_pool_size < SG_CONST_MAX_POOL_SIZE));
    SOKOL_ASSERT((desc->pass_pool_size > 0) && (desc->pass_pool_size < SG_CONST_MAX_POOL_SIZE));
    SOKOL_ASSERT(desc->label_stack_size > 0);

    _sg = SOKOL_MALLOC(sizeof(_sg_state));
    _sg_init_pool(&_sg->buffer_pool, desc->buffer_pool_size);
    _sg->buffers = SOKOL_MALLOC(sizeof(_sg_buffer) * _sg->buffer_pool.num);
    for (int i = 0; i < _sg->buffer_pool.num; i++) {
        _sg_init_buffer(&_sg->buffers[i]);
    }
    _sg_init_pool(&_sg->image_pool, desc->image_pool_size);
    _sg->images = SOKOL_MALLOC(sizeof(_sg_image) * _sg->image_pool.num);
    for (int i = 0; i < _sg->image_pool.num; i++) {
        _sg_init_image(&_sg->images[i]);
    }
    _sg_init_pool(&_sg->shader_pool, desc->shader_pool_size);
    _sg->shaders = SOKOL_MALLOC(sizeof(_sg_shader) * _sg->shader_pool.num);
    for (int i = 0; i < _sg->shader_pool.num; i++) {
        _sg_init_shader(&_sg->shaders[i]);
    }
    _sg_init_pool(&_sg->pipeline_pool, desc->pipeline_pool_size);
    _sg->pipelines = SOKOL_MALLOC(sizeof(_sg_pipeline) * _sg->pipeline_pool.num);
    for (int i = 0; i < _sg->pipeline_pool.num; i++) {
        _sg_init_pipeline(&_sg->pipelines[i]);
    }
    _sg_init_pool(&_sg->pass_pool, desc->pass_pool_size);
    _sg->passes = SOKOL_MALLOC(sizeof(_sg_pass) * _sg->pass_pool.num);
    for (int i = 0; i < _sg->pass_pool.num; i++) {
        _sg_init_pass(&_sg->passes[i]);
    }
}

void sg_discard() {
    SOKOL_ASSERT(_sg);
    SOKOL_FREE(_sg->passes);    _sg->passes = 0;
    SOKOL_FREE(_sg->pipelines); _sg->pipelines = 0;
    SOKOL_FREE(_sg->shaders);   _sg->shaders = 0;
    SOKOL_FREE(_sg->images);    _sg->images = 0;
    SOKOL_FREE(_sg->buffers);   _sg->buffers = 0;
    _sg_destroy_pool(&_sg->pass_pool);
    _sg_destroy_pool(&_sg->pipeline_pool);
    _sg_destroy_pool(&_sg->shader_pool);
    _sg_destroy_pool(&_sg->image_pool);
    _sg_destroy_pool(&_sg->buffer_pool);
}

sg_label sg_gen_label() {
    SOKOL_ASSERT(_sg);
    // FIXME
    return SG_INVALID_LABEL;
}

void sg_push_label(sg_label label) {
    SOKOL_ASSERT(_sg);
    // FIXME
}

sg_label sg_pop_label() {
    SOKOL_ASSERT(_sg);
    return SG_INVALID_LABEL;
}


