#pragma once
/*
    Sokol Gfx resource pool functions
*/

typedef struct {
    int size;
    uint32_t unique_counter;
    int queue_top;
    int* free_queue;
} _sg_pool;

static void _sg_init_pool(_sg_pool* pool, int num) {
    SOKOL_ASSERT(pool && (num > 0));
    pool->size = num;
    pool->queue_top = 0;
    pool->unique_counter = 0;
    pool->free_queue = SOKOL_MALLOC(sizeof(int)*num);
    /* never allocate the zero-th pool item since the invalid id is 0 */
    for (int i = num-1; i >= 1; i--) {
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
    SOKOL_ASSERT(pool->queue_top > 0);
    SOKOL_ASSERT(pool->free_queue);
    int slot_index = pool->free_queue[--pool->queue_top];
    return ((pool->unique_counter++)<<_SG_CONST_SLOT_SHIFT)|slot_index;
}

static void _sg_pool_free_id(_sg_pool* pool, sg_id id) {
    SOKOL_ASSERT(id != SG_INVALID_ID);
    SOKOL_ASSERT(pool);
    SOKOL_ASSERT(pool->free_queue);
    SOKOL_ASSERT(pool->queue_top < pool->size);
    #if _DEBUG
    /* debug check against double-free */
    int slot_index = _sg_slot_index(id);
    for (int i = 0; i < pool->queue_top; i++) {
        SOKOL_ASSERT(pool->free_queue[i] != slot_index);
    }
    #endif
    pool->free_queue[pool->queue_top++] = id;
}

typedef struct {
    _sg_pool pool[SG_NUM_RESOURCE_TYPES];
    _sg_buffer* buffers;
    _sg_image* images;
    _sg_shader* shaders;
    _sg_pipeline* pipelines;
    _sg_pass* passes;
} _sg_pools;

static void _sg_setup_pools(_sg_pools* p, sg_setup_desc* desc) {
    SOKOL_ASSERT(p);
    SOKOL_ASSERT(desc);
    for (int res_type = 0; res_type < SG_NUM_RESOURCE_TYPES; res_type++) {
        SOKOL_ASSERT(desc->resource_pool_size[res_type] > 0);
        SOKOL_ASSERT(desc->resource_pool_size[res_type] < _SG_CONST_MAX_POOL_SIZE);
        _sg_init_pool(&p->pool[res_type], desc->resource_pool_size[res_type]);
    }
    p->buffers = SOKOL_MALLOC(sizeof(_sg_buffer) * p->pool[SG_BUFFER].size);
    for (int i = 0; i < p->pool[SG_BUFFER].size; i++) {
        _sg_init_buffer(&p->buffers[i]);
    }
    p->images = SOKOL_MALLOC(sizeof(_sg_image) * p->pool[SG_IMAGE].size);
    for (int i = 0; i < p->pool[SG_IMAGE].size; i++) {
        _sg_init_image(&p->images[i]);
    }
    p->shaders = SOKOL_MALLOC(sizeof(_sg_shader) * p->pool[SG_SHADER].size);
    for (int i = 0; i < p->pool[SG_SHADER].size; i++) {
        _sg_init_shader(&p->shaders[i]);
    }
    p->pipelines = SOKOL_MALLOC(sizeof(_sg_pipeline) * p->pool[SG_PIPELINE].size);
    for (int i = 0; i < p->pool[SG_PIPELINE].size; i++) {
        _sg_init_pipeline(&p->pipelines[i]);
    }
    p->passes = SOKOL_MALLOC(sizeof(_sg_pass) * p->pool[SG_PASS].size);
    for (int i = 0; i < p->pool[SG_PASS].size; i++) {
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
    for (int res_type = 0; res_type < SG_NUM_RESOURCE_TYPES; res_type++) {
        _sg_discard_pool(&p->pool[res_type]);
    }
}

static _sg_pass* _sg_lookup_pass(_sg_pools* p, sg_id pass_id) {
    SOKOL_ASSERT(p);
    if (SG_INVALID_ID != pass_id) {
        int slot_index = _sg_slot_index(pass_id);
        SOKOL_ASSERT((slot_index >= 0) && (slot_index < p->pool[SG_PASS].size));
        _sg_pass* pass = &p->passes[slot_index];
        if (pass->slot.id == pass_id) {
            return pass;
        }
    }
    return 0;
}
