#if defined(SOKOL_IMPL) && !defined(SOKOL_SPINE_IMPL)
#define SOKOL_SPINE_IMPL
#endif
#ifndef SOKOL_SPINE_INCLUDED
/*
    sokol_spine.h -- a sokol-gfx renderer for the spine-c runtime
                     (see https://github.com/EsotericSoftware/spine-runtimes/tree/4.1/spine-c)

    Project URL: https://github.com/floooh/sokol

    Do this:
        #define SOKOL_IMPL or
        #define SOKOL_SPINE_IMPL

    before you include this file in *one* C or C++ file to create the
    implementation.

    The following defines are used by the implementation to select the
    platform-specific embedded shader code (these are the same defines as
    used by sokol_gfx.h and sokol_app.h):

    SOKOL_GLCORE33
    SOKOL_GLES2
    SOKOL_GLES3
    SOKOL_D3D11
    SOKOL_METAL

    ...optionally provide the following macros to override defaults:

    SOKOL_ASSERT(c)     - your own assert macro (default: assert(c))
    SOKOL_SPINE_API_DECL    - public function declaration prefix (default: extern)
    SOKOL_API_DECL      - same as SOKOL_SPINE_API_DECL
    SOKOL_API_IMPL      - public function implementation prefix (default: -)
    SOKOL_LOG(msg)      - your own logging function (default: puts(msg))
    SOKOL_UNREACHABLE() - a guard macro for unreachable code (default: assert(false))

    If sokol_spine.h is compiled as a DLL, define the following before
    including the declaration or implementation:

    SOKOL_DLL

    On Windows, SOKOL_DLL will define SOKOL_SPINE_API_DECL as __declspec(dllexport)
    or __declspec(dllimport) as needed.

    Include the following headers before including sokol_spine.h:

        sokol_gfx.h

    Include the following headers before include the sokol_spine.h *IMPLEMENTATION*:

        spine/spine.h

    FIXME FIXME FIXME
    =================

    LICENSE
    =======
    zlib/libpng license

    Copyright (c) 2022 Andre Weissflog

    This software is provided 'as-is', without any express or implied warranty.
    In no event will the authors be held liable for any damages arising from the
    use of this software.

    Permission is granted to anyone to use this software for any purpose,
    including commercial applications, and to alter it and redistribute it
    freely, subject to the following restrictions:

        1. The origin of this software must not be misrepresented; you must not
        claim that you wrote the original software. If you use this software in a
        product, an acknowledgment in the product documentation would be
        appreciated but is not required.

        2. Altered source versions must be plainly marked as such, and must not
        be misrepresented as being the original software.

        3. This notice may not be removed or altered from any source
        distribution.
*/
#define SOKOL_SPINE_INCLUDED (1)
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h> // size_t

#if !defined(SOKOL_GFX_INCLUDED)
#error "Please include sokol_gfx.h before sokol_spine.h"
#endif

#if defined(SOKOL_API_DECL) && !defined(SOKOL_SPINE_API_DECL)
#define SOKOL_SPINE_API_DECL SOKOL_API_DECL
#endif
#ifndef SOKOL_SPINE_API_DECL
#if defined(_WIN32) && defined(SOKOL_DLL) && defined(SOKOL_SPINE_IMPL)
#define SOKOL_SPINE_API_DECL __declspec(dllexport)
#elif defined(_WIN32) && defined(SOKOL_DLL)
#define SOKOL_SPINE_API_DECL __declspec(dllimport)
#else
#define SOKOL_SPINE_API_DECL extern
#endif
#endif

#ifdef __cplusplus
extern "C" {
#endif

enum {
    SSPINE_INVALID_ID = 0,
};

typedef struct sspine_context { uint32_t id; } sspine_context;
typedef struct sspine_atlas { uint32_t id; } sspine_atlas;
typedef struct sspine_skeleton { uint32_t id; } sspine_skeleton;
typedef struct sspine_instance { uint32_t id; } sspine_instance;

typedef struct sspine_bone { sspine_instance instance; int index; } sspine_bone;
typedef struct sspine_slot { sspine_instance instance; int index; } sspine_slot;
typedef struct sspine_anim { sspine_instance instance; int index; } sspine_anim;

typedef struct sspine_range { const void* ptr; size_t size; } sspine_range;
typedef struct sspine_vec2 { float x, y; } sspine_vec2;
typedef sg_color sspine_color;

typedef enum SSPINE_resource_state {
    SSPINE_RESOURCESTATE_INITIAL,
    SSPINE_RESOURCESTATE_ALLOC,
    SSPINE_RESOURCESTATE_VALID,
    SSPINE_RESOURCESTATE_FAILED,
    SSPINE_RESOURCESTATE_INVALID,
    _SSPINE_RESOURCESTATE_FORCE_U32 = 0x7FFFFFFF
} sspine_resource_state;

typedef struct sspine_bone_transform {
    sspine_vec2 position;
    float rotation;         // in degrees
    sspine_vec2 scale;
    sspine_vec2 shear;      // in degrees
} sspine_bone_transform;

typedef struct sspine_context_desc {
    int max_vertices;
    int max_commands;
    sg_pixel_format color_format;
    sg_pixel_format depth_format;
    int sample_count;
} sspine_context_desc;

typedef struct sspine_image_info {
    sg_image image;
    const char* filename;
    sg_filter min_filter;
    sg_filter mag_filter;
    sg_wrap wrap_u;
    sg_wrap wrap_v;
    int width;
    int height;
} sspine_image_info;

typedef struct sspine_atlas_desc {
    sspine_range data;
} sspine_atlas_desc;

typedef struct sspine_skeleton_desc {
    sspine_atlas atlas;
    const char* json_data;
    sspine_range binary_data;
} sspine_skeleton_desc;

typedef struct sspine_instance_desc {
    sspine_skeleton skeleton;
} sspine_instance_desc;

typedef struct sspine_allocator {
    void* (*alloc)(size_t size, void* user_data);
    void (*free)(void* ptr, void* user_data);
    void* user_data;
} sspine_allocator;


typedef struct sspine_desc {
    int max_vertices;
    int max_commands;
    int context_pool_size;
    int atlas_pool_size;
    int skeleton_pool_size;
    int instance_pool_size;
    sg_pixel_format color_format;
    sg_pixel_format depth_format;
    int sample_count;
    sspine_allocator allocator;
} sspine_desc;

// setup/shutdown
SOKOL_SPINE_API_DECL void sspine_setup(const sspine_desc* desc);
SOKOL_SPINE_API_DECL void sspine_shutdown(void);

// context functions
SOKOL_SPINE_API_DECL sspine_context sspine_make_context(const sspine_context_desc* desc);
SOKOL_SPINE_API_DECL void sspine_destroy_context(sspine_context ctx);
SOKOL_SPINE_API_DECL void sspine_set_context(sspine_context ctx);
SOKOL_SPINE_API_DECL sspine_context sspine_get_context(void);
SOKOL_SPINE_API_DECL sspine_context sspine_default_context(void);

// create and destroy spine objects
SOKOL_SPINE_API_DECL sspine_atlas sspine_make_atlas(const sspine_atlas_desc* desc);
SOKOL_SPINE_API_DECL sspine_skeleton sspine_make_skeleton(const sspine_skeleton_desc* desc);
SOKOL_SPINE_API_DECL sspine_instance sspine_make_instance(const sspine_instance_desc* desc);
SOKOL_SPINE_API_DECL void sspine_destroy_atlas(sspine_atlas atlas);
SOKOL_SPINE_API_DECL void sspine_destroy_skeleton(sspine_skeleton skeleton);
SOKOL_SPINE_API_DECL void sspine_destroy_instance(sspine_instance instance);

// get current resource state (INITIAL, ALLOC, VALID, FAILD, INVALID)
SOKOL_SPINE_API_DECL sspine_resource_state sspine_get_context_state(sspine_context context);
SOKOL_SPINE_API_DECL sspine_resource_state sspine_get_atlas_state(sspine_atlas atlas);
SOKOL_SPINE_API_DECL sspine_resource_state sspine_get_skeleton_state(sspine_skeleton skeleton);
SOKOL_SPINE_API_DECL sspine_resource_state sspine_get_instance_state(sspine_instance instance);

// shortcut for sspine_get_*_state() == SSPINE_RESOURCESTATE_VALID
SOKOL_SPINE_API_DECL bool sspine_context_valid(sspine_context context);
SOKOL_SPINE_API_DECL bool sspine_atlas_valid(sspine_atlas atlas);
SOKOL_SPINE_API_DECL bool sspine_skeleton_valid(sspine_skeleton skeleton);
SOKOL_SPINE_API_DECL bool sspine_instance_valid(sspine_instance instance);

// atlas images
SOKOL_SPINE_API_DECL int sspine_get_num_images(sspine_atlas atlas);
SOKOL_SPINE_API_DECL sspine_image_info sspine_get_image_info(sspine_atlas atlas, int image_index);

// instance transform functions
SOKOL_SPINE_API_DECL void sspine_set_position(sspine_instance instance, sspine_vec2 position);
SOKOL_SPINE_API_DECL void sspine_set_scale(sspine_instance instance, sspine_vec2 scale);
SOKOL_SPINE_API_DECL void sspine_set_color(sspine_instance instance, sspine_color color);
SOKOL_SPINE_API_DECL sspine_vec2 sspine_get_position(sspine_instance instance);
SOKOL_SPINE_API_DECL sspine_vec2 sspine_get_scale(sspine_instance instance);
SOKOL_SPINE_API_DECL sspine_color sspine_get_color(sspine_instance instance);

// find instance items by name
SOKOL_SPINE_API_DECL sspine_bone sspine_find_bone(sspine_instance instance, const char* name);
SOKOL_SPINE_API_DECL sspine_slot sspine_find_slot(sspine_instance instance, const char* name);
SOKOL_SPINE_API_DECL sspine_anim sspine_find_anim(sspine_instance instance, const char* name);

// instance animation functions
SOKOL_SPINE_API_DECL void sspine_update_animation(sspine_instance instance, float delta);
SOKOL_SPINE_API_DECL void sspine_clear_animation_tracks(sspine_instance instance);
SOKOL_SPINE_API_DECL void sspine_clear_animation_track(sspine_instance instance, int track_index);
SOKOL_SPINE_API_DECL void sspine_set_animation(sspine_instance instance, int track_index, sspine_anim anim, bool loop);
SOKOL_SPINE_API_DECL void sspine_set_animation_by_name(sspine_instance instance, int track_index, const char* anim_name, bool loop);
SOKOL_SPINE_API_DECL void sspine_add_animation(sspine_instance instance, int track_index, sspine_anim anim, bool loop, float delay);
SOKOL_SPINE_API_DECL void sspine_add_animation_by_name(sspine_instance instance, int track_index, const char* anim_name, bool loop, float delay);
SOKOL_SPINE_API_DECL void sspine_set_empty_animation(sspine_instance instance, int track_index, float mix_duration);
SOKOL_SPINE_API_DECL void sspine_add_empty_animation(sspine_instance instance, int track_index, float mix_duration, float delay);

// iterate over instance items
SOKOL_SPINE_API_DECL int sspine_num_bones(sspine_instance instance);
SOKOL_SPINE_API_DECL int sspine_num_slots(sspine_instance instance);
SOKOL_SPINE_API_DECL sspine_bone sspine_bone_at(sspine_instance instance, int index);
SOKOL_SPINE_API_DECL sspine_slot sspine_slot_at(sspine_instance instance, int index);

// check if a instance item handle is valid
SOKOL_SPINE_API_DECL bool sspine_bone_valid(sspine_bone bone);
SOKOL_SPINE_API_DECL bool sspine_slot_valid(sspine_slot slot);

// direct bone manipulation, transforms are in bone-local space, angles are in degrees
SOKOL_SPINE_API_DECL void sspine_bone_set_transform(sspine_bone bone, const sspine_bone_transform* transform);
SOKOL_SPINE_API_DECL void sspine_bone_set_position(sspine_bone bone, sspine_vec2 position);
SOKOL_SPINE_API_DECL void sspine_bone_set_rotation(sspine_bone bone, float rotation);
SOKOL_SPINE_API_DECL void sspine_bone_set_scale(sspine_bone bone, sspine_vec2 scale);
SOKOL_SPINE_API_DECL void sspine_bone_set_shear(sspine_bone bone, sspine_vec2 shear);
SOKOL_SPINE_API_DECL sspine_bone_transform sspine_bone_get_transform(sspine_bone bone);
SOKOL_SPINE_API_DECL sspine_vec2 sspine_bone_get_position(sspine_bone bone);
SOKOL_SPINE_API_DECL float sspine_bone_get_rotation(sspine_bone bone);
SOKOL_SPINE_API_DECL sspine_vec2 sspine_bone_get_scale(sspine_bone bone);
SOKOL_SPINE_API_DECL sspine_vec2 sspine_bone_get_shear(sspine_bone bone);

// direct slot manipulation
SOKOL_SPINE_API_DECL void sspine_slot_set_color(sspine_slot slot, sspine_color color);
SOKOL_SPINE_API_DECL sspine_color sspine_slot_get_color(sspine_slot slot);

#ifdef __cplusplus
} // extern "C"
#endif

//-- IMPLEMENTATION ------------------------------------------------------------
#ifdef SOKOL_SPINE_IMPL
#define SOKOL_SPINE_IMPL_INCLUDED (1)

#if !defined(SPINE_SPINE_H_)
#error "Please include spine/spine.h before the sokol_spine.h implementation"
#endif

#include <stdlib.h> // malloc/free
#include <string.h> // memset

#ifndef SOKOL_API_IMPL
    #define SOKOL_API_IMPL
#endif
#ifndef SOKOL_DEBUG
    #ifndef NDEBUG
        #define SOKOL_DEBUG (1)
    #endif
#endif
#ifndef SOKOL_ASSERT
    #include <assert.h>
    #define SOKOL_ASSERT(c) assert(c)
#endif
#ifndef SOKOL_LOG
    #ifdef SOKOL_DEBUG
        #include <stdio.h>
        #define SOKOL_LOG(s) { SOKOL_ASSERT(s); puts(s); }
    #else
        #define SOKOL_LOG(s)
    #endif
#endif
#ifndef SOKOL_UNREACHABLE
    #define SOKOL_UNREACHABLE SOKOL_ASSERT(false)
#endif

#define _sspine_def(val, def) (((val) == 0) ? (def) : (val))
#define _SSPINE_INIT_COOKIE (0xABBAABBA)
#define _SSPINE_INVALID_SLOT_INDEX (0)
#define _SSPINE_MAX_STACK_DEPTH (64)
#define _SSPINE_DEFAULT_CONTEXT_POOL_SIZE (4)
#define _SSPINE_DEFAULT_ATLAS_POOL_SIZE (64)
#define _SSPINE_DEFAULT_SKELETON_POOL_SIZE (64)
#define _SSPINE_DEFAULT_INSTANCE_POOL_SIZE (1024)
#define _SSPINE_DEFAULT_MAX_VERTICES (1<<16)
#define _SSPINE_DEFAULT_MAX_COMMANDS (1<<14)
#define _SSPINE_SLOT_SHIFT (16)
#define _SSPINE_MAX_POOL_SIZE (1<<_SSPINE_SLOT_SHIFT)
#define _SSPINE_SLOT_MASK (_SSPINE_MAX_POOL_SIZE-1)

typedef struct {
    uint32_t id;
    sspine_resource_state state;
} _sspine_slot_t;

typedef struct {
    int size;
    int queue_top;
    uint32_t* gen_ctrs;
    int* free_queue;
} _sspine_pool_t;

typedef struct {
    _sspine_slot_t slot;
    spAtlas* sp_atlas;
    int num_pages;
} _sspine_atlas_t;

typedef struct {
    _sspine_pool_t pool;
    _sspine_atlas_t* items;
} _sspine_atlas_pool_t;

typedef struct {
    uint32_t id;
    _sspine_atlas_t* ptr;
} _sspine_atlas_ref_t;

typedef struct {
    _sspine_slot_t slot;
    _sspine_atlas_ref_t atlas;
    spSkeletonData* sp_skel_data;
    spAnimationStateData* sp_anim_data;
} _sspine_skeleton_t;

typedef struct {
    _sspine_pool_t pool;
    _sspine_skeleton_t* items;
} _sspine_skeleton_pool_t;

typedef struct {
    uint32_t id;
    _sspine_skeleton_t* ptr;
} _sspine_skeleton_ref_t;

typedef struct {
    _sspine_slot_t slot;
    _sspine_atlas_ref_t atlas;
    _sspine_skeleton_ref_t skel;
    spSkeleton* sp_skel;
    spAnimationState* sp_anim;
    spSkeletonClipping* sp_clip;
} _sspine_instance_t;

typedef struct {
    _sspine_pool_t pool;
    _sspine_instance_t* items;
} _sspine_instance_pool_t;

typedef struct {
    _sspine_slot_t slot;
} _sspine_context_t;

typedef struct {
    _sspine_pool_t pool;
    _sspine_context_t* items;
} _sspine_context_pool_t;

typedef struct {
    uint32_t init_cookie;
    sspine_desc desc;
    sspine_context def_ctx_id;
    sspine_context cur_ctx_id;
    _sspine_context_t* cur_ctx;   // may be 0!
    _sspine_context_pool_t context_pool;
    _sspine_atlas_pool_t atlas_pool;
    _sspine_skeleton_pool_t skeleton_pool;
    _sspine_instance_pool_t instance_pool;
} _sspine_t;
static _sspine_t _sspine;

// dummy spine-c platform implementation functions
void _spAtlasPage_createTexture(spAtlasPage* self, const char* path) {
    // nothing to do here
    (void)self; (void)path;
}

char* _spUtil_readFile(const char* path, int* length) {
    (void)path;
    *length = 0;
    return 0;
}

//=== MEMORY MANAGEMENT FUNCTIONS ==============================================
static void _sspine_clear(void* ptr, size_t size) {
    SOKOL_ASSERT(ptr && (size > 0));
    memset(ptr, 0, size);
}

static void* _sspine_malloc(size_t size) {
    SOKOL_ASSERT(size > 0);
    void* ptr;
    if (_sspine.desc.allocator.alloc) {
        ptr = _sspine.desc.allocator.alloc(size, _sspine.desc.allocator.user_data);
    }
    else {
        ptr = malloc(size);
    }
    SOKOL_ASSERT(ptr);
    return ptr;
}

static void* _sspine_malloc_clear(size_t size) {
    void* ptr = _sspine_malloc(size);
    _sspine_clear(ptr, size);
    return ptr;
}

static void _sspine_free(void* ptr) {
    if (_sspine.desc.allocator.free) {
        _sspine.desc.allocator.free(ptr, _sspine.desc.allocator.user_data);
    }
    else {
        free(ptr);
    }
}

static bool _sspine_atlas_ref_valid(const _sspine_atlas_ref_t* ref) {
    return ref->ptr && (ref->ptr->slot.id == ref->id);
}

static bool _sspine_skeleton_ref_valid(const _sspine_skeleton_ref_t* ref) {
    return ref->ptr && (ref->ptr->slot.id == ref->id);
}

//=== HANDLE POOL FUNCTIONS ====================================================
static void _sspine_init_pool(_sspine_pool_t* pool, int num) {
    SOKOL_ASSERT(pool && (num >= 1));
    // slot 0 is reserved for the 'invalid id', so bump the pool size by 1
    pool->size = num + 1;
    pool->queue_top = 0;
    // generation counters indexable by pool slot index, slot 0 is reserved
    size_t gen_ctrs_size = sizeof(uint32_t) * (size_t)pool->size;
    pool->gen_ctrs = (uint32_t*) _sspine_malloc_clear(gen_ctrs_size);
    // it's not a bug to only reserve 'num' here
    pool->free_queue = (int*) _sspine_malloc_clear(sizeof(int) * (size_t)num);
    // never allocate the zero-th pool item since the invalid id is 0
    for (int i = pool->size-1; i >= 1; i--) {
        pool->free_queue[pool->queue_top++] = i;
    }
}

static void _sspine_discard_pool(_sspine_pool_t* pool) {
    SOKOL_ASSERT(pool);
    SOKOL_ASSERT(pool->free_queue);
    _sspine_free(pool->free_queue);
    pool->free_queue = 0;
    SOKOL_ASSERT(pool->gen_ctrs);
    _sspine_free(pool->gen_ctrs);
    pool->gen_ctrs = 0;
    pool->size = 0;
    pool->queue_top = 0;
}

static int _sspine_pool_alloc_index(_sspine_pool_t* pool) {
    SOKOL_ASSERT(pool);
    SOKOL_ASSERT(pool->free_queue);
    if (pool->queue_top > 0) {
        int slot_index = pool->free_queue[--pool->queue_top];
        SOKOL_ASSERT((slot_index > 0) && (slot_index < pool->size));
        return slot_index;
    }
    else {
        // pool exhausted
        return _SSPINE_INVALID_SLOT_INDEX;
    }
}

static void _sspine_pool_free_index(_sspine_pool_t* pool, int slot_index) {
    SOKOL_ASSERT((slot_index > _SSPINE_INVALID_SLOT_INDEX) && (slot_index < pool->size));
    SOKOL_ASSERT(pool);
    SOKOL_ASSERT(pool->free_queue);
    SOKOL_ASSERT(pool->queue_top < pool->size);
    #ifdef SOKOL_DEBUG
    // debug check against double-free
    for (int i = 0; i < pool->queue_top; i++) {
        SOKOL_ASSERT(pool->free_queue[i] != slot_index);
    }
    #endif
    pool->free_queue[pool->queue_top++] = slot_index;
    SOKOL_ASSERT(pool->queue_top <= (pool->size-1));
}

/* initiailize a pool slot:
    - bump the slot's generation counter
    - create a resource id from the generation counter and slot index
    - set the slot's id to this id
    - set the slot's state to ALLOC
    - return the handle id
*/
static uint32_t _sspine_slot_init(_sspine_pool_t* pool, _sspine_slot_t* slot, int slot_index) {
    /* FIXME: add handling for an overflowing generation counter,
       for now, just overflow (another option is to disable
       the slot)
    */
    SOKOL_ASSERT(pool && pool->gen_ctrs);
    SOKOL_ASSERT((slot_index > _SSPINE_INVALID_SLOT_INDEX) && (slot_index < pool->size));
    SOKOL_ASSERT((slot->state == SSPINE_RESOURCESTATE_INITIAL) && (slot->id == SSPINE_INVALID_ID));
    uint32_t ctr = ++pool->gen_ctrs[slot_index];
    slot->id = (ctr<<_SSPINE_SLOT_SHIFT)|(slot_index & _SSPINE_SLOT_MASK);
    slot->state = SSPINE_RESOURCESTATE_ALLOC;
    return slot->id;
}

// extract slot index from id
static int _sspine_slot_index(uint32_t id) {
    int slot_index = (int) (id & _SSPINE_SLOT_MASK);
    SOKOL_ASSERT(_SSPINE_INVALID_SLOT_INDEX != slot_index);
    return slot_index;
}

static void _sspine_init_item_pool(_sspine_pool_t* pool, int pool_size, void** items_ptr, size_t item_size_bytes) {
    // NOTE: the pools will have an additional item, since slot 0 is reserved
    SOKOL_ASSERT(pool && (pool->size == 0));
    SOKOL_ASSERT((pool_size > 0) && (pool_size < _SSPINE_MAX_POOL_SIZE));
    SOKOL_ASSERT(items_ptr && (*items_ptr == 0));
    SOKOL_ASSERT(item_size_bytes > 0);
    _sspine_init_pool(pool, pool_size);
    const size_t pool_size_bytes = item_size_bytes * (size_t)pool->size;
    *items_ptr = _sspine_malloc_clear(pool_size_bytes);
}

static void _sspine_discard_item_pool(_sspine_pool_t* pool, void** items_ptr) {
    SOKOL_ASSERT(pool && (pool->size != 0));
    SOKOL_ASSERT(items_ptr && (*items_ptr != 0));
    _sspine_free(*items_ptr); *items_ptr = 0;
    _sspine_discard_pool(pool);
}

//== CONTEXT POOL FUNCTIONS ====================================================
static void _sspine_setup_context_pool(int pool_size) {
    _sspine_context_pool_t* p = &_sspine.context_pool;
    _sspine_init_item_pool(&p->pool, pool_size, (void**)&p->items, sizeof(_sspine_context_t));
}

static void _sspine_discard_context_pool(void) {
    _sspine_context_pool_t* p = &_sspine.context_pool;
    _sspine_discard_item_pool(&p->pool, (void**)&p->items);
}

static sspine_context _sspine_make_context_handle(uint32_t id) {
    sspine_context handle = { id };
    return handle;
}

static _sspine_context_t* _sspine_context_at(uint32_t id) {
    SOKOL_ASSERT(SSPINE_INVALID_ID != id);
    const _sspine_context_pool_t* p = &_sspine.context_pool;
    int slot_index = _sspine_slot_index(id);
    SOKOL_ASSERT((slot_index > _SSPINE_INVALID_SLOT_INDEX) && (slot_index < p->pool.size));
    return &p->items[slot_index];
}

static _sspine_context_t* _sspine_lookup_context(uint32_t id) {
    if (SSPINE_INVALID_ID != id) {
        _sspine_context_t* ctx = _sspine_context_at(id);
        if (ctx->slot.id == id) {
            return ctx;
        }
    }
    return 0;
}

static sspine_context _sspine_alloc_context(void) {
    _sspine_context_pool_t* p = &_sspine.context_pool;
    sspine_context res;
    int slot_index = _sspine_pool_alloc_index(&p->pool);
    if (_SSPINE_INVALID_SLOT_INDEX != slot_index) {
        uint32_t id = _sspine_slot_init(&p->pool, &p->items[slot_index].slot, slot_index);
        res = _sspine_make_context_handle(id);
    }
    else {
        // pool exhausted
        res = _sspine_make_context_handle(SSPINE_INVALID_ID);
    }
    return res;
}

static sspine_resource_state _sspine_init_context(_sspine_context_t* ctx, const sspine_context_desc* desc) {
    SOKOL_ASSERT(ctx && (ctx->slot.state == SSPINE_RESOURCESTATE_ALLOC));
    SOKOL_ASSERT(desc);
    return SSPINE_RESOURCESTATE_FAILED;
}

static void _sspine_deinit_context(_sspine_context_t* ctx) {
    // FIXME
    (void)ctx;
}

static void _sspine_destroy_context(sspine_context ctx_id) {
    _sspine_context_t* ctx = _sspine_lookup_context(ctx_id.id);
    if (ctx) {
        _sspine_deinit_context(ctx);
        _sspine_context_pool_t* p = &_sspine.context_pool;
        _sspine_clear(ctx, sizeof(_sspine_context_t));
        _sspine_pool_free_index(&p->pool, _sspine_slot_index(ctx_id.id));
    }
}

static void _sspine_destroy_all_contexts(void) {
    _sspine_context_pool_t* p = &_sspine.context_pool;
    for (int i = 0; i < p->pool.size; i++) {
        _sspine_context_t* ctx = &p->items[i];
        _sspine_destroy_context(_sspine_make_context_handle(ctx->slot.id));
    }
}

static sspine_context_desc _sspine_context_desc_defaults(const sspine_context_desc* desc) {
    sspine_context_desc res = *desc;
    res.max_vertices = _sspine_def(desc->max_vertices, _SSPINE_DEFAULT_MAX_VERTICES);
    res.max_commands = _sspine_def(desc->max_commands, _SSPINE_DEFAULT_MAX_COMMANDS);
    return res;
}

static bool _sspine_is_default_context(sspine_context ctx_id) {
    return ctx_id.id == 0x00010001;
}

//=== ATLAS POOL FUNCTIONS =====================================================
static void _sspine_setup_atlas_pool(int pool_size) {
    _sspine_atlas_pool_t* p = &_sspine.atlas_pool;
    _sspine_init_item_pool(&p->pool, pool_size, (void**)&p->items, sizeof(_sspine_atlas_t));
}

static void _sspine_discard_atlas_pool(void) {
    _sspine_atlas_pool_t* p = &_sspine.atlas_pool;
    _sspine_discard_item_pool(&p->pool, (void**)&p->items);
}

static sspine_atlas _sspine_make_atlas_handle(uint32_t id) {
    sspine_atlas handle = { id };
    return handle;
}

static _sspine_atlas_t* _sspine_atlas_at(uint32_t id) {
    SOKOL_ASSERT(SSPINE_INVALID_ID != id);
    const _sspine_atlas_pool_t* p = &_sspine.atlas_pool;
    int slot_index = _sspine_slot_index(id);
    SOKOL_ASSERT((slot_index > _SSPINE_INVALID_SLOT_INDEX) && (slot_index < p->pool.size));
    return &p->items[slot_index];
}

static _sspine_atlas_t* _sspine_lookup_atlas(uint32_t id) {
    if (SSPINE_INVALID_ID != id) {
        _sspine_atlas_t* atlas = _sspine_atlas_at(id);
        if (atlas->slot.id == id) {
            return atlas;
        }
    }
    return 0;
}

static sspine_atlas _sspine_alloc_atlas(void) {
    _sspine_atlas_pool_t* p = &_sspine.atlas_pool;
    sspine_atlas res;
    int slot_index = _sspine_pool_alloc_index(&p->pool);
    if (_SSPINE_INVALID_SLOT_INDEX != slot_index) {
        uint32_t id = _sspine_slot_init(&p->pool, &p->items[slot_index].slot, slot_index);
        res = _sspine_make_atlas_handle(id);
    }
    else {
        // pool exhausted
        res = _sspine_make_atlas_handle(SSPINE_INVALID_ID);
    }
    return res;
}

void _spAtlasPage_disposeTexture(spAtlasPage* self) {
    if (self->rendererObject != 0) {
        const sg_image img = { (uint32_t)(uintptr_t)self->rendererObject };
        sg_destroy_image(img);
    }
}

static sspine_resource_state _sspine_init_atlas(_sspine_atlas_t* atlas, const sspine_atlas_desc* desc) {
    SOKOL_ASSERT(atlas && (atlas->slot.state == SSPINE_RESOURCESTATE_ALLOC));
    SOKOL_ASSERT(desc);
    SOKOL_ASSERT(atlas->sp_atlas == 0);

    if ((desc->data.ptr == 0) || (desc->data.size == 0)) {
        return SSPINE_RESOURCESTATE_FAILED;
    }

    // NOTE: Spine doesn't detect when invalid or corrupt data is passed here,
    // not much we can do about this...
    atlas->sp_atlas = spAtlas_create((const char*)desc->data.ptr, (int)desc->data.size, "", 0);
    if (0 == atlas->sp_atlas) {
        return SSPINE_RESOURCESTATE_FAILED;
    }

    // allocate a sokol-gfx image handle for each page, but the actual image initialization
    // needs to be delegated to the application
    for (spAtlasPage* page = atlas->sp_atlas->pages; page != 0; page = page->next) {
        atlas->num_pages++;
        const sg_image img = sg_alloc_image();
        if (sg_query_image_state(img) != SG_RESOURCESTATE_ALLOC) {
            return SSPINE_RESOURCESTATE_FAILED;
        }
        page->rendererObject = (void*)(uintptr_t)img.id;
    }
    return SSPINE_RESOURCESTATE_VALID;
}

static void _sspine_deinit_atlas(_sspine_atlas_t* atlas) {
    if (atlas->sp_atlas) {
        spAtlas_dispose(atlas->sp_atlas);
        atlas->sp_atlas = 0;
    }
}

static void _sspine_destroy_atlas(sspine_atlas atlas_id) {
    _sspine_atlas_t* atlas = _sspine_lookup_atlas(atlas_id.id);
    if (atlas) {
        _sspine_deinit_atlas(atlas);
        _sspine_atlas_pool_t* p = &_sspine.atlas_pool;
        _sspine_clear(atlas, sizeof(_sspine_atlas_t));
        _sspine_pool_free_index(&p->pool, _sspine_slot_index(atlas_id.id));
    }
}

static void _sspine_destroy_all_atlases(void) {
    _sspine_atlas_pool_t* p = &_sspine.atlas_pool;
    for (int i = 0; i < p->pool.size; i++) {
        _sspine_atlas_t* atlas = &p->items[i];
        _sspine_destroy_atlas(_sspine_make_atlas_handle(atlas->slot.id));
    }
}

static sspine_atlas_desc _sspine_atlas_desc_defaults(const sspine_atlas_desc* desc) {
    sspine_atlas_desc res = *desc;
    return res;
}

//=== SKELETON POOL FUNCTIONS ==================================================
static void _sspine_setup_skeleton_pool(int pool_size) {
    _sspine_skeleton_pool_t* p = &_sspine.skeleton_pool;
    _sspine_init_item_pool(&p->pool, pool_size, (void**)&p->items, sizeof(_sspine_skeleton_t));
}

static void _sspine_discard_skeleton_pool(void) {
    _sspine_skeleton_pool_t* p = &_sspine.skeleton_pool;
    _sspine_discard_item_pool(&p->pool, (void**)&p->items);
}

static sspine_skeleton _sspine_make_skeleton_handle(uint32_t id) {
    sspine_skeleton handle = { id };
    return handle;
}

static _sspine_skeleton_t* _sspine_skeleton_at(uint32_t id) {
    SOKOL_ASSERT(SSPINE_INVALID_ID != id);
    const _sspine_skeleton_pool_t* p = &_sspine.skeleton_pool;
    int slot_index = _sspine_slot_index(id);
    SOKOL_ASSERT((slot_index > _SSPINE_INVALID_SLOT_INDEX) && (slot_index < p->pool.size));
    return &p->items[slot_index];
}

static _sspine_skeleton_t* _sspine_lookup_skeleton(uint32_t id) {
    if (SSPINE_INVALID_ID != id) {
        _sspine_skeleton_t* skeleton = _sspine_skeleton_at(id);
        if (skeleton->slot.id == id) {
            return skeleton;
        }
    }
    return 0;
}

static sspine_skeleton _sspine_alloc_skeleton(void) {
    _sspine_skeleton_pool_t* p = &_sspine.skeleton_pool;
    sspine_skeleton res;
    int slot_index = _sspine_pool_alloc_index(&p->pool);
    if (_SSPINE_INVALID_SLOT_INDEX != slot_index) {
        uint32_t id = _sspine_slot_init(&p->pool, &p->items[slot_index].slot, slot_index);
        res = _sspine_make_skeleton_handle(id);
    }
    else {
        // pool exhausted
        res = _sspine_make_skeleton_handle(SSPINE_INVALID_ID);
    }
    return res;
}

static sspine_resource_state _sspine_init_skeleton(_sspine_skeleton_t* skeleton, const sspine_skeleton_desc* desc) {
    SOKOL_ASSERT(skeleton && (skeleton->slot.state = SSPINE_RESOURCESTATE_ALLOC));
    SOKOL_ASSERT(desc);

    if ((0 == desc->json_data) && ((0 == desc->binary_data.ptr) || (0 == desc->binary_data.size))) {
        return SSPINE_RESOURCESTATE_FAILED;
    }

    skeleton->atlas.id = desc->atlas.id;
    skeleton->atlas.ptr = _sspine_lookup_atlas(skeleton->atlas.id);
    if (!_sspine_atlas_ref_valid(&skeleton->atlas)) {
        return SSPINE_RESOURCESTATE_FAILED;
    }
    _sspine_atlas_t* atlas = skeleton->atlas.ptr;
    if (SSPINE_RESOURCESTATE_VALID != atlas->slot.state) {
        return SSPINE_RESOURCESTATE_FAILED;
    }
    SOKOL_ASSERT(atlas->sp_atlas);

    if (desc->json_data) {
        spSkeletonJson* skel_json = spSkeletonJson_create(atlas->sp_atlas);
        SOKOL_ASSERT(skel_json);
        skeleton->sp_skel_data = spSkeletonJson_readSkeletonData(skel_json, desc->json_data);
        spSkeletonJson_dispose(skel_json); skel_json = 0;
        if (0 == skeleton->sp_skel_data) {
            return SSPINE_RESOURCESTATE_FAILED;
        }
    }
    else {
        spSkeletonBinary* skel_bin = spSkeletonBinary_create(atlas->sp_atlas);
        SOKOL_ASSERT(skel_bin);
        skeleton->sp_skel_data = spSkeletonBinary_readSkeletonData(skel_bin, desc->binary_data.ptr, (int)desc->binary_data.size);
        spSkeletonBinary_dispose(skel_bin); skel_bin = 0;
        if (0 == skeleton->sp_skel_data) {
            return SSPINE_RESOURCESTATE_FAILED;
        }
    }
    SOKOL_ASSERT(skeleton->sp_skel_data);

    skeleton->sp_anim_data = spAnimationStateData_create(skeleton->sp_skel_data);
    SOKOL_ASSERT(skeleton->sp_anim_data);

    return SSPINE_RESOURCESTATE_VALID;
}

static void _sspine_deinit_skeleton(_sspine_skeleton_t* skeleton) {
    if (skeleton->sp_anim_data) {
        spAnimationStateData_dispose(skeleton->sp_anim_data);
        skeleton->sp_anim_data = 0;
    }
    if (skeleton->sp_skel_data) {
        spSkeletonData_dispose(skeleton->sp_skel_data);
        skeleton->sp_skel_data = 0;
    }
}

static void _sspine_destroy_skeleton(sspine_skeleton skeleton_id) {
    _sspine_skeleton_t* skeleton = _sspine_lookup_skeleton(skeleton_id.id);
    if (skeleton) {
        _sspine_deinit_skeleton(skeleton);
        _sspine_skeleton_pool_t* p = &_sspine.skeleton_pool;
        _sspine_clear(skeleton, sizeof(_sspine_skeleton_t));
        _sspine_pool_free_index(&p->pool, _sspine_slot_index(skeleton_id.id));
    }
}

static void _sspine_destroy_all_skeletons(void) {
    _sspine_skeleton_pool_t* p = &_sspine.skeleton_pool;
    for (int i = 0; i < p->pool.size; i++) {
        _sspine_skeleton_t* skeleton = &p->items[i];
        _sspine_destroy_skeleton(_sspine_make_skeleton_handle(skeleton->slot.id));
    }
}

static sspine_skeleton_desc _sspine_skeleton_desc_defaults(const sspine_skeleton_desc* desc) {
    sspine_skeleton_desc res = *desc;
    return res;
}

//=== INSTANCE POOL FUNCTIONS ==================================================
static void _sspine_setup_instance_pool(int pool_size) {
    _sspine_instance_pool_t* p = &_sspine.instance_pool;
    _sspine_init_item_pool(&p->pool, pool_size, (void**)&p->items, sizeof(_sspine_instance_t));
}

static void _sspine_discard_instance_pool(void) {
    _sspine_instance_pool_t* p = &_sspine.instance_pool;
    _sspine_discard_item_pool(&p->pool, (void**)&p->items);
}

static sspine_instance _sspine_make_instance_handle(uint32_t id) {
    sspine_instance handle = { id };
    return handle;
}

static _sspine_instance_t* _sspine_instance_at(uint32_t id) {
    SOKOL_ASSERT(SSPINE_INVALID_ID != id);
    const _sspine_instance_pool_t* p = &_sspine.instance_pool;
    int slot_index = _sspine_slot_index(id);
    SOKOL_ASSERT((slot_index > _SSPINE_INVALID_SLOT_INDEX) && (slot_index < p->pool.size));
    return &p->items[slot_index];
}

static _sspine_instance_t* _sspine_lookup_instance(uint32_t id) {
    if (SSPINE_INVALID_ID != id) {
        _sspine_instance_t* instance = _sspine_instance_at(id);
        if (instance->slot.id == id) {
            return instance;
        }
    }
    return 0;
}

static sspine_instance _sspine_alloc_instance(void) {
    _sspine_instance_pool_t* p = &_sspine.instance_pool;
    sspine_instance res;
    int slot_index = _sspine_pool_alloc_index(&p->pool);
    if (_SSPINE_INVALID_SLOT_INDEX != slot_index) {
        uint32_t id = _sspine_slot_init(&p->pool, &p->items[slot_index].slot, slot_index);
        res = _sspine_make_instance_handle(id);
    }
    else {
        // pool exhausted
        res = _sspine_make_instance_handle(SSPINE_INVALID_ID);
    }
    return res;
}

static sspine_resource_state _sspine_init_instance(_sspine_instance_t* instance, const sspine_instance_desc* desc) {
    SOKOL_ASSERT(instance && (instance->slot.state == SSPINE_RESOURCESTATE_ALLOC));
    SOKOL_ASSERT(desc);

    instance->skel.id = desc->skeleton.id;
    instance->skel.ptr = _sspine_lookup_skeleton(instance->skel.id);
    if (!_sspine_skeleton_ref_valid(&instance->skel)) {
        return SSPINE_RESOURCESTATE_FAILED;
    }
    _sspine_skeleton_t* skel = instance->skel.ptr;
    if (SSPINE_RESOURCESTATE_VALID != skel->slot.state) {
        return SSPINE_RESOURCESTATE_FAILED;
    }
    instance->atlas = skel->atlas;
    if (!_sspine_atlas_ref_valid(&instance->atlas)) {
        return SSPINE_RESOURCESTATE_FAILED;
    }
    if (SSPINE_RESOURCESTATE_VALID != instance->atlas.ptr->slot.state) {
        return SSPINE_RESOURCESTATE_FAILED;
    }
    SOKOL_ASSERT(skel->sp_skel_data);
    SOKOL_ASSERT(skel->sp_anim_data);

    instance->sp_skel = spSkeleton_create(skel->sp_skel_data);
    SOKOL_ASSERT(instance->sp_skel);
    instance->sp_anim = spAnimationState_create(skel->sp_anim_data);
    SOKOL_ASSERT(instance->sp_anim);
    instance->sp_clip = spSkeletonClipping_create();
    SOKOL_ASSERT(instance->sp_clip);

    return SSPINE_RESOURCESTATE_VALID;
}

static void _sspine_deinit_instance(_sspine_instance_t* instance) {
    if (instance->sp_clip) {
        spSkeletonClipping_dispose(instance->sp_clip);
        instance->sp_clip = 0;
    }
    if (instance->sp_anim) {
        spAnimationState_dispose(instance->sp_anim);
        instance->sp_anim = 0;
    }
    if (instance->sp_skel) {
        spSkeleton_dispose(instance->sp_skel);
        instance->sp_skel = 0;
    }
}

static void _sspine_destroy_instance(sspine_instance instance_id) {
    _sspine_instance_t* instance = _sspine_lookup_instance(instance_id.id);
    if (instance) {
        _sspine_deinit_instance(instance);
        _sspine_instance_pool_t* p = &_sspine.instance_pool;
        _sspine_clear(instance, sizeof(_sspine_instance_t));
        _sspine_pool_free_index(&p->pool, _sspine_slot_index(instance_id.id));
    }
}

static void _sspine_destroy_all_instances(void) {
    _sspine_instance_pool_t* p = &_sspine.instance_pool;
    for (int i = 0; i < p->pool.size; i++) {
        _sspine_instance_t* instance = &p->items[i];
        _sspine_destroy_instance(_sspine_make_instance_handle(instance->slot.id));
    }
}

static sspine_instance_desc _sspine_instance_desc_defaults(const sspine_instance_desc* desc) {
    sspine_instance_desc res = *desc;
    return res;
}

// return sspine_desc with patched defaults
static sspine_desc _sspine_desc_defaults(const sspine_desc* desc) {
    SOKOL_ASSERT((desc->allocator.alloc && desc->allocator.free) || (!desc->allocator.alloc && !desc->allocator.free));
    sspine_desc res = *desc;
    res.max_vertices = _sspine_def(desc->max_vertices, _SSPINE_DEFAULT_MAX_VERTICES);
    res.max_commands = _sspine_def(desc->max_commands, _SSPINE_DEFAULT_MAX_COMMANDS);
    res.context_pool_size = _sspine_def(desc->context_pool_size, _SSPINE_DEFAULT_CONTEXT_POOL_SIZE);
    res.atlas_pool_size = _sspine_def(desc->atlas_pool_size, _SSPINE_DEFAULT_ATLAS_POOL_SIZE);
    res.skeleton_pool_size = _sspine_def(desc->skeleton_pool_size, _SSPINE_DEFAULT_SKELETON_POOL_SIZE);
    res.instance_pool_size = _sspine_def(desc->instance_pool_size, _SSPINE_DEFAULT_INSTANCE_POOL_SIZE);
    return res;
}

static sspine_context_desc _sspine_as_context_desc(const sspine_desc* desc) {
    sspine_context_desc ctx_desc;
    _sspine_clear(&ctx_desc, sizeof(ctx_desc));
    ctx_desc.max_vertices = desc->max_vertices;
    ctx_desc.max_commands = desc->max_commands;
    ctx_desc.color_format = desc->color_format;
    ctx_desc.depth_format = desc->depth_format;
    ctx_desc.sample_count = desc->sample_count;
    return ctx_desc;
}

static spAtlasPage* _sspine_atlas_page_at(_sspine_atlas_t* atlas, int index) {
    SOKOL_ASSERT(atlas && atlas->sp_atlas);
    SOKOL_ASSERT((index >= 0) && (index < atlas->num_pages));
    int i = 0;
    for (spAtlasPage* page = atlas->sp_atlas->pages; page != 0; page = page->next, i++) {
        if (i == index) {
            return page;
        }
    }
    return 0;
}

static sg_filter _sspine_as_image_filter(spAtlasFilter filter) {
    switch (filter) {
        case SP_ATLAS_UNKNOWN_FILTER: return _SG_FILTER_DEFAULT;
        case SP_ATLAS_NEAREST: return SG_FILTER_NEAREST;
        case SP_ATLAS_LINEAR: return SG_FILTER_LINEAR;
        case SP_ATLAS_MIPMAP: return SG_FILTER_LINEAR_MIPMAP_NEAREST;
        case SP_ATLAS_MIPMAP_NEAREST_NEAREST: return SG_FILTER_NEAREST_MIPMAP_NEAREST;
        case SP_ATLAS_MIPMAP_LINEAR_NEAREST: return SG_FILTER_LINEAR_MIPMAP_NEAREST;
        case SP_ATLAS_MIPMAP_NEAREST_LINEAR: return SG_FILTER_NEAREST_MIPMAP_LINEAR;
        case SP_ATLAS_MIPMAP_LINEAR_LINEAR: return SG_FILTER_LINEAR_MIPMAP_LINEAR;
    }
}

static sg_wrap _sspine_as_image_wrap(spAtlasWrap wrap) {
    switch (wrap) {
        case SP_ATLAS_MIRROREDREPEAT: return SG_WRAP_MIRRORED_REPEAT;
        case SP_ATLAS_CLAMPTOEDGE: return SG_WRAP_CLAMP_TO_EDGE;
        case SP_ATLAS_REPEAT: return SG_WRAP_REPEAT;
    }
}

static void _sspine_init_image_info(_sspine_atlas_t* atlas, int index, sspine_image_info* info) {
    SOKOL_ASSERT((index >= 0) && (index < atlas->num_pages));
    if ((index < 0) || (index >= atlas->num_pages)) {
        return;
    }
    spAtlasPage* page = _sspine_atlas_page_at(atlas, index);
    SOKOL_ASSERT(page);
    SOKOL_ASSERT(page->name);
    info->image.id = (uint32_t)(uintptr_t)page->rendererObject;
    info->filename = page->name;
    info->min_filter = _sspine_as_image_filter(page->minFilter);
    info->mag_filter = _sspine_as_image_filter(page->magFilter);
    info->wrap_u = _sspine_as_image_wrap(page->uWrap);
    info->wrap_v = _sspine_as_image_wrap(page->vWrap);
    info->width = page->width;
    info->height = page->height;
}

//== PUBLIC FUNCTIONS ==========================================================
SOKOL_API_IMPL void sspine_setup(const sspine_desc* desc) {
    SOKOL_ASSERT(desc);
    _sspine_clear(&_sspine, sizeof(_sspine));
    _sspine.init_cookie = _SSPINE_INIT_COOKIE;
    _sspine.desc = _sspine_desc_defaults(desc);
    _sspine_setup_context_pool(_sspine.desc.context_pool_size);
    _sspine_setup_atlas_pool(_sspine.desc.atlas_pool_size);
    _sspine_setup_skeleton_pool(_sspine.desc.skeleton_pool_size);
    _sspine_setup_instance_pool(_sspine.desc.instance_pool_size);
    const sspine_context_desc ctx_desc = _sspine_as_context_desc(&_sspine.desc);
    _sspine.def_ctx_id = sspine_make_context(&ctx_desc);
    SOKOL_ASSERT(_sspine_is_default_context(_sspine.def_ctx_id));
    sspine_set_context(_sspine.def_ctx_id);
}

SOKOL_API_IMPL void sspine_shutdown(void) {
    SOKOL_ASSERT(_SSPINE_INIT_COOKIE == _sspine.init_cookie);
    _sspine_destroy_all_instances();
    _sspine_destroy_all_skeletons();
    _sspine_destroy_all_atlases();
    _sspine_destroy_all_contexts();
    _sspine_discard_instance_pool();
    _sspine_discard_skeleton_pool();
    _sspine_discard_atlas_pool();
    _sspine_discard_context_pool();
    _sspine.init_cookie = 0;
}

SOKOL_API_IMPL sspine_context sspine_make_context(const sspine_context_desc* desc) {
    SOKOL_ASSERT(_SSPINE_INIT_COOKIE == _sspine.init_cookie);
    SOKOL_ASSERT(desc);
    const sspine_context_desc desc_def = _sspine_context_desc_defaults(desc);
    sspine_context ctx_id = _sspine_alloc_context();
    _sspine_context_t* ctx = _sspine_lookup_context(ctx_id.id);
    if (ctx) {
        ctx->slot.state = _sspine_init_context(ctx, &desc_def);
        SOKOL_ASSERT((ctx->slot.state == SSPINE_RESOURCESTATE_VALID) || (ctx->slot.state == SSPINE_RESOURCESTATE_FAILED));
        if (ctx->slot.state == SSPINE_RESOURCESTATE_FAILED) {
            _sspine_deinit_context(ctx);
        }
    }
    else {
        ctx->slot.state = SSPINE_RESOURCESTATE_FAILED;
        SOKOL_LOG("sokol_spine.h: context pool exhausted");
    }
    return ctx_id;
}

SOKOL_API_IMPL void sspine_destroy_context(sspine_context ctx_id) {
    SOKOL_ASSERT(_SSPINE_INIT_COOKIE == _sspine.init_cookie);
    if (_sspine_is_default_context(ctx_id)) {
        SOKOL_LOG("sokol_spine.h: cannot destroy default context");
        return;
    }
    _sspine_destroy_context(ctx_id);
    // re-validate the current context pointer (this will return a nullptr
    // if we just destroyed the current context)
    _sspine.cur_ctx = _sspine_lookup_context(_sspine.cur_ctx_id.id);
}

SOKOL_API_IMPL void sspine_set_context(sspine_context ctx_id) {
    SOKOL_ASSERT(_SSPINE_INIT_COOKIE == _sspine.init_cookie);
    if (_sspine_is_default_context(ctx_id)) {
        _sspine.cur_ctx_id = _sspine.def_ctx_id;
    }
    else {
        _sspine.cur_ctx_id = ctx_id;
    }
    // this will return null if the handle isn't valid
    _sspine.cur_ctx = _sspine_lookup_context(_sspine.cur_ctx_id.id);
}

SOKOL_API_IMPL sspine_context sspine_get_context(void) {
    SOKOL_ASSERT(_SSPINE_INIT_COOKIE == _sspine.init_cookie);
    return _sspine.cur_ctx_id;
}

SOKOL_API_IMPL sspine_context sspine_default_context(void) {
    return _sspine_make_context_handle(0x00010001);
}

SOKOL_API_IMPL sspine_atlas sspine_make_atlas(const sspine_atlas_desc* desc) {
    SOKOL_ASSERT(_SSPINE_INIT_COOKIE == _sspine.init_cookie);
    SOKOL_ASSERT(desc);
    const sspine_atlas_desc desc_def = _sspine_atlas_desc_defaults(desc);
    sspine_atlas atlas_id = _sspine_alloc_atlas();
    _sspine_atlas_t* atlas = _sspine_lookup_atlas(atlas_id.id);
    if (atlas) {
        atlas->slot.state = _sspine_init_atlas(atlas, &desc_def);
        SOKOL_ASSERT((atlas->slot.state == SSPINE_RESOURCESTATE_VALID) || (atlas->slot.state == SSPINE_RESOURCESTATE_FAILED));
        if (atlas->slot.state == SSPINE_RESOURCESTATE_FAILED) {
            _sspine_deinit_atlas(atlas);
        }
    }
    else {
        atlas->slot.state = SSPINE_RESOURCESTATE_FAILED;
        SOKOL_LOG("sokol_spine.h: atlas pool exhausted");
    }
    return atlas_id;
}

SOKOL_API_IMPL void sspine_destroy_atlas(sspine_atlas atlas_id) {
    SOKOL_ASSERT(_SSPINE_INIT_COOKIE == _sspine.init_cookie);
    _sspine_destroy_atlas(atlas_id);
}

SOKOL_API_IMPL sspine_skeleton sspine_make_skeleton(const sspine_skeleton_desc* desc) {
    SOKOL_ASSERT(_SSPINE_INIT_COOKIE == _sspine.init_cookie);
    SOKOL_ASSERT(desc);
    const sspine_skeleton_desc desc_def = _sspine_skeleton_desc_defaults(desc);
    sspine_skeleton skeleton_id = _sspine_alloc_skeleton();
    _sspine_skeleton_t* skeleton = _sspine_lookup_skeleton(skeleton_id.id);
    if (skeleton) {
        skeleton->slot.state = _sspine_init_skeleton(skeleton, &desc_def);
        SOKOL_ASSERT((skeleton->slot.state == SSPINE_RESOURCESTATE_VALID) || (skeleton->slot.state == SSPINE_RESOURCESTATE_FAILED));
        if (skeleton->slot.state == SSPINE_RESOURCESTATE_FAILED) {
            _sspine_deinit_skeleton(skeleton);
        }
    }
    else {
        skeleton->slot.state = SSPINE_RESOURCESTATE_FAILED;
        SOKOL_LOG("sokol_spine.h: skeleton pool exhausted");
    }
    return skeleton_id;
}

SOKOL_API_IMPL void sspine_destroy_skeleton(sspine_skeleton skeleton_id) {
    SOKOL_ASSERT(_SSPINE_INIT_COOKIE == _sspine.init_cookie);
    _sspine_destroy_skeleton(skeleton_id);
}

SOKOL_API_IMPL sspine_instance sspine_make_instance(const sspine_instance_desc* desc) {
    SOKOL_ASSERT(_SSPINE_INIT_COOKIE == _sspine.init_cookie);
    SOKOL_ASSERT(desc);
    const sspine_instance_desc desc_def = _sspine_instance_desc_defaults(desc);
    sspine_instance instance_id = _sspine_alloc_instance();
    _sspine_instance_t* instance = _sspine_lookup_instance(instance_id.id);
    if (instance) {
        instance->slot.state = _sspine_init_instance(instance, &desc_def);
        SOKOL_ASSERT((instance->slot.state == SSPINE_RESOURCESTATE_VALID) || (instance->slot.state == SSPINE_RESOURCESTATE_FAILED));
        if (instance->slot.state == SSPINE_RESOURCESTATE_FAILED) {
            _sspine_deinit_instance(instance);
        }
    }
    else {
        instance->slot.state = SSPINE_RESOURCESTATE_FAILED;
        SOKOL_LOG("sokol_spine.h: instance pool exhausted");
    }
    return instance_id;
}

SOKOL_API_IMPL void sspine_destroy_instance(sspine_instance instance_id) {
    SOKOL_ASSERT(_SSPINE_INIT_COOKIE == _sspine.init_cookie);
    _sspine_destroy_instance(instance_id);
}

SOKOL_API_IMPL sspine_resource_state sspine_get_context_state(sspine_context ctx_id) {
    SOKOL_ASSERT(_SSPINE_INIT_COOKIE == _sspine.init_cookie);
    const _sspine_context_t* ctx = _sspine_lookup_context(ctx_id.id);
    if (ctx) {
        return ctx->slot.state;
    }
    else {
        return SSPINE_RESOURCESTATE_INVALID;
    }
}

SOKOL_API_IMPL sspine_resource_state sspine_get_atlas_state(sspine_atlas atlas_id) {
    SOKOL_ASSERT(_SSPINE_INIT_COOKIE == _sspine.init_cookie);
    const _sspine_atlas_t* atlas = _sspine_lookup_atlas(atlas_id.id);
    if (atlas) {
        return atlas->slot.state;
    }
    else {
        return SSPINE_RESOURCESTATE_INVALID;
    }
}

SOKOL_API_IMPL sspine_resource_state sspine_get_skeleton_state(sspine_skeleton skeleton_id) {
    SOKOL_ASSERT(_SSPINE_INIT_COOKIE == _sspine.init_cookie);
    const _sspine_skeleton_t* skeleton = _sspine_lookup_skeleton(skeleton_id.id);
    if (skeleton) {
        return skeleton->slot.state;
    }
    else {
        return SSPINE_RESOURCESTATE_INVALID;
    }
}

SOKOL_API_IMPL sspine_resource_state sspine_get_instance_state(sspine_instance instance_id) {
    SOKOL_ASSERT(_SSPINE_INIT_COOKIE == _sspine.init_cookie);
    const _sspine_instance_t* instance = _sspine_lookup_instance(instance_id.id);
    if (instance) {
        return instance->slot.state;
    }
    else {
        return SSPINE_RESOURCESTATE_INVALID;
    }
}

SOKOL_API_IMPL bool sspine_context_valid(sspine_context ctx_id) {
    SOKOL_ASSERT(_SSPINE_INIT_COOKIE == _sspine.init_cookie);
    return sspine_get_context_state(ctx_id) == SSPINE_RESOURCESTATE_VALID;
}

SOKOL_API_IMPL bool sspine_atlas_valid(sspine_atlas atlas_id) {
    SOKOL_ASSERT(_SSPINE_INIT_COOKIE == _sspine.init_cookie);
    return sspine_get_atlas_state(atlas_id) == SSPINE_RESOURCESTATE_VALID;
}

SOKOL_API_IMPL bool sspine_skeleton_valid(sspine_skeleton skeleton_id) {
    SOKOL_ASSERT(_SSPINE_INIT_COOKIE == _sspine.init_cookie);
    return sspine_get_skeleton_state(skeleton_id) == SSPINE_RESOURCESTATE_VALID;
}

SOKOL_API_IMPL bool sspine_instance_valid(sspine_instance instance_id) {
    SOKOL_ASSERT(_SSPINE_INIT_COOKIE == _sspine.init_cookie);
    return sspine_get_instance_state(instance_id) == SSPINE_RESOURCESTATE_VALID;
}

SOKOL_API_IMPL int sspine_get_num_images(sspine_atlas atlas_id) {
    SOKOL_ASSERT(_SSPINE_INIT_COOKIE == _sspine.init_cookie);
    _sspine_atlas_t* atlas = _sspine_lookup_atlas(atlas_id.id);
    return atlas ? atlas->num_pages : 0;
}

SOKOL_API_IMPL sspine_image_info sspine_get_image_info(sspine_atlas atlas_id, int image_index) {
    SOKOL_ASSERT(_SSPINE_INIT_COOKIE == _sspine.init_cookie);
    _sspine_atlas_t* atlas = _sspine_lookup_atlas(atlas_id.id);
    sspine_image_info img_info;
    _sspine_clear(&img_info, sizeof(img_info));
    _sspine_init_image_info(atlas, image_index, &img_info);
    return img_info;
}

#endif // SOKOL_SPINE_IMPL
#endif // SOKOL_SPINE_INCLUDED
