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

// FIXME: track events, event call back (in instance desc)

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

typedef struct sspine_image_desc {
    sg_pixel_format format;
    sg_filter min_filter;
    sg_filter mag_filter;
    sg_wrap wrap_u;
    sg_wrap wrap_v;
    int width;
    int height;
} sspine_image_desc;

typedef struct sspine_load_image_callback {
    sg_image (*callback)(const char* path, const sspine_image_desc* desc, void* user_data);
    void* user_data;
} sspine_load_image_callback;

typedef struct sspine_atlas_desc {
    sspine_range data;
    const char* dir;
    sspine_load_image_callback load_image_cb;
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
    sg_resource_state state;
} _sspine_slot_t;

typedef struct {
    int size;
    int queue_top;
    uint32_t* gen_ctrs;
    int* free_queue;
} _sspine_pool_t;

typedef struct {
    _sspine_slot_t slot;
    // FIXME
} _sspine_atlas_t;

typedef struct {
    _sspine_pool_t pool;
    _sspine_atlas_t* items;
} _sspine_atlas_pool_t;

typedef struct {
    _sspine_slot_t slot;
    // FIXME
} _sspine_skeleton_t;

typedef struct {
    _sspine_pool_t pool;
    _sspine_skeleton_t* items;
} _sspine_skeleton_pool_t;

typedef struct {
    _sspine_slot_t slot;
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
    _sspine_context_pool_t context_pool;
    _sspine_atlas_pool_t atlas_pool;
    _sspine_skeleton_pool_t skeleton_pool;
    _sspine_instance_pool_t instance_pool;
} _sspine_t;
static _sspine_t _sspine;

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

/* allocate the slot at slot_index:
    - bump the slot's generation counter
    - create a resource id from the generation counter and slot index
    - set the slot's id to this id
    - set the slot's state to ALLOC
    - return the resource id
*/
static uint32_t _sspine_slot_alloc(_sspine_pool_t* pool, _sspine_slot_t* slot, int slot_index) {
    /* FIXME: add handling for an overflowing generation counter,
       for now, just overflow (another option is to disable
       the slot)
    */
    SOKOL_ASSERT(pool && pool->gen_ctrs);
    SOKOL_ASSERT((slot_index > _SSPINE_INVALID_SLOT_INDEX) && (slot_index < pool->size));
    SOKOL_ASSERT((slot->state == SG_RESOURCESTATE_INITIAL) && (slot->id == SG_INVALID_ID));
    uint32_t ctr = ++pool->gen_ctrs[slot_index];
    slot->id = (ctr<<_SSPINE_SLOT_SHIFT)|(slot_index & _SSPINE_SLOT_MASK);
    slot->state = SG_RESOURCESTATE_ALLOC;
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

// context pool functions
static void _sspine_setup_context_pool(int pool_size) {
    _sspine_init_item_pool(&_sspine.context_pool.pool, pool_size, (void**)&_sspine.context_pool.items, sizeof(_sspine_context_t));
}

static void _sspine_discard_context_pool(void) {
    _sspine_discard_item_pool(&_sspine.context_pool.pool, (void**)&_sspine.context_pool.items);
}

// atlas pool functions
static void _sspine_setup_atlas_pool(int pool_size) {
    _sspine_init_item_pool(&_sspine.atlas_pool.pool, pool_size, (void**)&_sspine.atlas_pool.items, sizeof(_sspine_atlas_t));
}

static void _sspine_discard_atlas_pool(void) {
    _sspine_discard_item_pool(&_sspine.atlas_pool.pool, (void**)&_sspine.atlas_pool.items);
}

// skeleton pool functions
static void _sspine_setup_skeleton_pool(int pool_size) {
    _sspine_init_item_pool(&_sspine.skeleton_pool.pool, pool_size, (void**)&_sspine.skeleton_pool.items, sizeof(_sspine_skeleton_t));
}

static void _sspine_discard_skeleton_pool(void) {
    _sspine_discard_item_pool(&_sspine.skeleton_pool.pool, (void**)&_sspine.skeleton_pool.items);
}

// instance pool functions
static void _sspine_setup_instance_pool(int pool_size) {
    _sspine_init_item_pool(&_sspine.instance_pool.pool, pool_size, (void**)&_sspine.instance_pool.items, sizeof(_sspine_instance_t));
}

static void _sspine_discard_instance_pool(void) {
    _sspine_discard_item_pool(&_sspine.instance_pool.pool, (void**)&_sspine.instance_pool.items);
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
}

SOKOL_API_IMPL void sspine_shutdown(void) {
    SOKOL_ASSERT(_SSPINE_INIT_COOKIE == _sspine.init_cookie);
    // FIXME: destroy pool items

    _sspine_discard_instance_pool();
    _sspine_discard_skeleton_pool();
    _sspine_discard_atlas_pool();
    _sspine_discard_context_pool();
    _sspine.init_cookie = 0;
}

#endif // SOKOL_SPINE_IMPL

#endif // SOKOL_SPINE_INCLUDED
