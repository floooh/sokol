/*
    Sokol Gfx generic implementation code
*/
enum {
    _SG_CONST_SLOT_SHIFT = 16,
    _SG_CONST_SLOT_MASK = (1<<_SG_CONST_SLOT_SHIFT)-1,
    _SG_CONST_MAX_POOL_SIZE = (1<<_SG_CONST_SLOT_SHIFT),
};

typedef enum {
    _SG_SLOT_STATE_FREE,
    _SG_SLOT_STATE_ALLOC,
    _SG_SLOT_STATE_VALID,
} _sg_slot_state;

//------------------------------------------------------------------------------
typedef struct {
    sg_id id;
    _sg_slot_state state;
} _sg_slot;

static void _sg_init_slot(_sg_slot* slot) {
    SOKOL_ASSERT(slot);
    slot->id = SG_INVALID_ID;
    slot->state = _SG_SLOT_STATE_FREE;
}

static int _sg_slot_index(sg_id id) {
    return id & _SG_CONST_SLOT_MASK;
}

//------------------------------------------------------------------------------
#if defined(SOKOL_USE_GL) || defined(SOKOL_USE_GLES2) || defined(SOKOL_USE_GLES3)
#include "_sokol_gfx_gl.impl.h"
#elif defined(SOKOL_USE_D3D11)
#include "_sokol_gfx_d3d11.impl.h"
#elif defined(SOKOL_USE_METAL)
#include "_sokol_gfx_metal.impl.h"
#else
#error "No rendering backend configured (SOKOL_USE_GL, SOKOL_USE_GLES2, SOKOL_USE_GLES3, SOKOL_USE_D3D11 or SOKOL_GFX_USE_METAL"
#endif
#include "_sokol_gfx_pools.impl.h"

typedef struct {
    _sg_pools pools;
    _sg_backend backend;
} _sg_state;
_sg_state* _sg = 0;

void sg_setup(sg_setup_desc* desc) {
    SOKOL_ASSERT(!_sg);
    SOKOL_ASSERT(desc);
    SOKOL_ASSERT((desc->width > 0) && (desc->height > 0));
    SOKOL_ASSERT(desc->sample_count >= 1);
    _sg = SOKOL_MALLOC(sizeof(_sg_state));
    _sg_setup_pools(&_sg->pools, desc);
    _sg_setup_backend(&_sg->backend);
}

void sg_discard() {
    SOKOL_ASSERT(_sg);
    _sg_discard_backend(&_sg->backend);
    _sg_discard_pools(&_sg->pools);
    SOKOL_FREE(_sg);
    _sg = 0;
}

bool sg_isvalid() {
    return _sg != 0;
}

void sg_begin_pass(sg_id pass_id, sg_pass_action* pass_action, int width, int height) {
    SOKOL_ASSERT(_sg);
    _sg_pass* pass = _sg_lookup_pass(&_sg->pools, pass_id);  // can be 0
    _sg_begin_pass(&_sg->backend, pass, pass_action, width, height);
}

void sg_end_pass() {
    _sg_end_pass(&_sg->backend);
}

void sg_commit() {
    _sg_commit(&_sg->backend);
} 
