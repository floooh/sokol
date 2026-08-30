#if defined(SOKOL_IMPL) && !defined(SOKOL_CMDBUF_IMPL)
#define SOKOL_CMDBUF_IMPL
#endif
#ifndef SOKOL_CMDBUF_INCLUDED
/*
    sokol_cmdbuf.h  - a software command buffer for sokol_gfx.h

    Project URL: https://github.com/floooh/sokol

    Do this:
        #define SOKOL_IMPL or
        #define SOKOL_CMDBUF_IMPL
    before you include this file in *one* C or C++ file to create the
    implementation.

    ...optionally provide the following macros to override defaults:

    SOKOL_ASSERT(c)     - your own assert macro (default: assert(c))
    SOKOL_CMDBUF_API_DECL   - public function declaration prefix (default: extern)
    SOKOL_API_DECL      - same as SOKOL_CMDBUF_API_DECL
    SOKOL_API_IMPL      - public function implementation prefix (default: -)
    SOKOL_UNREACHABLE() - a guard macro for unreachable code (default: assert(false))

    If sokol_cmdbuf.h is compiled as a DLL, define the following before
    including the declaration or implementation:

    SOKOL_DLL

    On Windows, SOKOL_DLL will define SOKOL_CMDBUF_API_DECL as __declspec(dllexport)
    or __declspec(dllimport) as needed.

    Include the following headers before including sokol_cmdbuf.h:

        sokol_gfx.h


    FIXME docs


    LICENSE
    =======
    zlib/libpng license

    Copyright (c) 2026 Andre Weissflog

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
#define SOKOL_CMDBUF_INCLUDED (1)
#include <stdint.h>
#include <stdbool.h>

#if !defined(SOKOL_GFX_INCLUDED)
#error "Please include sokol_gfx.h before sokol_cmdbuf.h"
#endif

#if defined(SOKOL_API_DECL) && !defined(SOKOL_CMDBUF_API_DECL)
#define SOKOL_CMDBUF_API_DECL SOKOL_API_DECL
#endif
#ifndef SOKOL_CMDBUF_API_DECL
#if defined(_WIN32) && defined(SOKOL_DLL) && defined(SOKOL_CMDBUF_IMPL)
#define SOKOL_CMDBUF_API_DECL __declspec(dllexport)
#elif defined(_WIN32) && defined(SOKOL_DLL)
#define SOKOL_CMDBUF_API_DECL __declspec(dllimport)
#else
#define SOKOL_CMDBUF_API_DECL extern
#endif
#endif

#ifdef __cplusplus
extern "C" {
#endif

// public constants
enum {
    SCB_INVALID_ID = 0,
};

/*
    scb_cmdbuf

    A command buffer handle created with scb_make_cmdbuf().
*/
typedef struct scb_cmdbuf_s { uint32_t id; } scb_cmdbuf;

/*
    scb_cmdbuf_desc

    Creation parameters of a command buffer object. Used
    in scb_make_cmdbuf().

    TODO: information on how to estimate required size
*/
typedef struct scb_cmdbuf_desc_s {
    size_t size;            // command buffer size in bytes (default: 256 * 1024 (256 KBytes))
    const char* label;
} scb_cmdbuf_desc;

/*
    scb_cmdbuf_info

    Result of scb_query_cmdbuf_info.
*/
typedef struct scb_cmdbuf_info_s {
    size_t size;        // overall command buffer size in bytes
    size_t cur_offset;  // current recording byte offset
    size_t remaining;   // number of remaining bytes in command buffer
    bool overflown;     // true if command buffer is in overflown state
} scb_cmdbuf_info;

/*
    scb_log_item

    Log items are defined via X-Macro, expanded into an enum scb_log_item
    and (in debug-mode only also to human readable strings.

    Used as parameter to the logging callback.
*/
#define _SCB_LOG_ITEMS \
    _SCB_LOGITEM_XMACRO(OK, "Ok") \
    _SCB_LOGITEM_XMACRO(MALLOC_FAILED, "memory allocation failed") \

#define _SCB_LOGITEM_XMACRO(item,msg) SCB_LOGITEM_##item,
typedef enum scb_log_item_e {
    _SCB_LOG_ITEMS
} scb_log_item;
#undef _SCB_LOGITEM_XMACRO

/*
    scb_logger

    Used in scb_desc to provide a custom logging and error reporting
    callback to sokol_cmdbuf.h
*/
typedef struct scb_logger_s {
    void (*func)(
        const char* tag,                // always "scb"
        uint32_t log_level,             // 0=panic, 1=error, 2=warning, 3=info
        uint32_t log_item_id,           // SCB_LOGITEM_*
        const char* message_or_null,    // a message string, may be nullptr in release mode
        uint32_t line_nr,               // line number in sokol_debugtext.h
        const char* filename_or_null,   // source filename, may be nullptr in release mode
        void* user_data);
    void* user_data;
} scb_logger;

/*
    scb_allocator

    Used in scb_desc to provide custom memory-alloc and -free functions
    to sokol_cmdbuf.h. If memory management should be overridden, both the
    alloc_fn and free_fn function must be provided (e.g. it's not valid to
    override one function but not the other).
*/
typedef struct scb_allocator_s {
    void* (*alloc_fn)(size_t size, void* user_data);
    void (*free_fn)(void* ptr, void* user_data);
    void* user_data;
} scb_allocator;

/*
    scb_desc

    FIXME: docs
*/
typedef struct scb_desc_s {
    int buffer_pool_size;       // max number of command buffers that can be alive simultanously (default: 16)
    scb_allocator allocator;    // optional memory allocation overrides (default: malloc/free)
    scb_logger logger;          // optional log override functions (default: NO LOGGING)
} scb_desc;

// initialization and shutdown
SOKOL_CMDBUF_API_DECL void scb_setup(const scb_desc* desc);
SOKOL_CMDBUF_API_DECL void scb_shutdown(void);

// create and destroy command buffer objects
SOKOL_CMDBUF_API_DECL scb_cmdbuf scb_make_cmdbuf(const scb_cmdbuf_desc* desc);
SOKOL_CMDBUF_API_DECL void scb_destroy_cmdbuf(scb_cmdbuf cb);

// submit command buffer to sokol-gfx (call inside a sokol-gfx pass)
SOKOL_CMDBUF_API_DECL void scb_submit(scb_cmdbuf cb, bool rewind);

// record sokol-gfx commands into command buffer
SOKOL_CMDBUF_API_DECL void scb_apply_viewport(scb_cmdbuf cb, int x, int y, int width, int height, bool origin_top_left);
SOKOL_CMDBUF_API_DECL void scb_apply_viewportf(scb_cmdbuf cb, float x, float y, float width, float height, bool origin_top_left);
SOKOL_CMDBUF_API_DECL void scb_apply_scissor_rect(scb_cmdbuf cb, int x, int y, int width, int height, bool origin_top_left);
SOKOL_CMDBUF_API_DECL void scb_apply_scissor_rectf(scb_cmdbuf cb, float x, float y, float width, float height, bool origin_top_left);
SOKOL_CMDBUF_API_DECL void scb_apply_pipeline(scb_cmdbuf cb, sg_pipeline pip);
SOKOL_CMDBUF_API_DECL void scb_apply_bindings(scb_cmdbuf cb, const sg_bindings* bindings);
SOKOL_CMDBUF_API_DECL void scb_apply_uniforms(scb_cmdbuf cb, int ub_slot, const sg_range* data);
SOKOL_CMDBUF_API_DECL void scb_draw(scb_cmdbuf cb, int base_element, int num_elements, int num_instances);
SOKOL_CMDBUF_API_DECL void scb_draw_ex(scb_cmdbuf cb, int base_element, int num_elements, int num_instances, int base_vertex, int base_instance);
SOKOL_CMDBUF_API_DECL void scb_dispatch(scb_cmdbuf cb, int num_groups_x, int num_groups_y, int num_groups_z);

// getting info
SOKOL_CMDBUF_API_DECL scb_desc scb_query_desc(void);
SOKOL_CMDBUF_API_DECL scb_cmdbuf_desc scb_query_cmdbuf_desc(scb_cmdbuf cb);
SOKOL_CMDBUF_API_DECL scb_cmdbuf_info scb_query_cmdbuf_info(scb_cmdbuf cb);

#ifdef __cplusplus
} // extern "C"
// C++ const-ref wrappers
FIXME
#endif
#endif /* SOKOL_CMDBUF_INCLUDED */

//------------------------------------------------------------------------------
// >>implementation
#ifdef SOKOL_CMDBUF_IMPL
#define SOKOL_CMDBUF_IMPL_INCLUDED (1)

#ifndef SOKOL_API_IMPL
    #define SOKOL_API_IMPL
#endif
#ifndef SOKOL_DEBUG
    #ifndef NDEBUG
        #define SOKOL_DEBUG
    #endif
#endif
#ifndef SOKOL_ASSERT
    #include <assert.h>
    #define SOKOL_ASSERT(c) assert(c)
#endif

#ifndef SOKOL_UNREACHABLE
    #define SOKOL_UNREACHABLE SOKOL_ASSERT(false)
#endif
#ifndef _SOKOL_UNUSED
    #define _SOKOL_UNUSED(x) (void)(x)
#endif

#define _scb_def(val, def) (((val) == 0) ? (def) : (val))
#define _SCB_INIT_COOKIE (0xACBAABCA)

#define _SCB_STRING_SIZE (32)
#define _SCB_DEFAULT_CMDBUF_POOL_SIZE (16)
#define _SCB_DEFAULT_CMDBUF_SIZE (262144)   // 256 * 1024
#define _SCB_INVALID_SLOT_INDEX (0)
#define _SCB_SLOT_SHIFT (16)
#define _SCB_MAX_POOL_SIZE (1<<_SCB_SLOT_SHIFT)
#define _SCB_SLOT_MASK (_SCB_MAX_POOL_SIZE-1)

// >>structs
typedef struct {
    uint32_t id;
} _scb_slot_t;

typedef struct {
    int size;
    int queue_top;
    uint32_t* gen_ctrs;
    int* free_queue;
} _scb_pool_t;

typedef struct {
    char buf[_SCB_STRING_SIZE];
} _scb_str_t;

typedef struct {
    _scb_slot_t slot;
    uint8_t* buffer;
    size_t offset;
    bool overflown;
    scb_cmdbuf_desc desc;
    scb_str_t label;
} _scb_cmdbuf_t;

typedef struct {
    _scb_pool_t;
    _scb_cmdbuf_t* cmdbufs;
} _scb_cmdbuf_pool_t;

typedef struct {
    uint32_t init_cookie;
    scb_desc desc;
    _scb_cmdbuf_pool_t cmdbuf_pool;
} _scb_t;
static _scb_t _scb;

// >>logging
#if defined(SOKOL_DEBUG)
#define _SCB_LOGITEM_XMACRO(item,msg) #item ": " msg,
static const char* _scb_log_messages[] = {
    _SCB_LOG_ITEMS
};
#undef _SCB_LOGITEM_XMACRO
#endif // SOKOL_DEBUG

#define _SCB_PANIC(code) _scb_log(SCB_LOGITEM_ ##code, 0, __LINE__)
#define _SCB_ERROR(code) _scb_log(SCB_LOGITEM_ ##code, 1, __LINE__)
#define _SCB_WARN(code) _scb_log(SCB_LOGITEM_ ##code, 2, __LINE__)
#define _SCB_INFO(code) _scb_log(SCB_LOGITEM_ ##code, 3, __LINE__)

static void _scb_log(scb_log_item_t log_item, uint32_t log_level, uint32_t line_nr) {
    if (_scb.desc.logger.func) {
        #if defined(SOKOL_DEBUG)
            const char* filename = __FILE__;
            const char* message = _scb_log_messages[log_item];
        #else
            const char* filename = 0;
            const char* message = 0;
        #endif
        _scb.desc.logger.func("scb", log_level, (uint32_t)log_item, message, line_nr, filename, _scb.desc.logger.user_data);
    } else {
        // for log level PANIC it would be 'undefined behaviour' to continue
        if (log_level == 0) {
            abort();
        }
    }
}

// >>memory
static void _scb_clear(void* ptr, size_t size) {
    SOKOL_ASSERT(ptr && (size > 0));
    memset(ptr, 0, size);
}

static void* _scb_malloc(size_t size) {
    SOKOL_ASSERT(size > 0);
    void* ptr;
    if (_scb.desc.allocator.alloc_fn) {
        ptr = _scb.desc.allocator.alloc_fn(size, _scb.desc.allocator.user_data);
    } else {
        ptr = malloc(size);
    }
    if (0 == ptr) {
        _SCB_PANIC(MALLOC_FAILED);
    }
    return ptr;
}

static void* _scb_malloc_clear(size_t size) {
    void* ptr = _scb_malloc(size);
    _scb_clear(ptr, size);
    return ptr;
}

static void _scb_free(void* ptr) {
    if (_scb.desc.allocator.free_fn) {
        _scb.desc.allocator.free_fn(ptr, _scb.desc.allocator.user_data);
    } else {
        free(ptr);
    }
}

// >>pool
static void _scb(_scb_pool_t* pool, int num) {
    SOKOL_ASSERT(pool && (num >= 1));
    // slot 0 is reserved for the 'invalid id', so bump the pool size by 1
    pool->size = num + 1;
    pool->queue_top = 0;
    // generation counters indexable by pool slot index, slot 0 is reserved
    size_t gen_ctrs_size = sizeof(uint32_t) * (size_t)pool->size;
    pool->gen_ctrs = (uint32_t*) _scb_malloc_clear(gen_ctrs_size);
    // it's not a bug to only reserve 'num' here
    pool->free_queue = (int*) _scb_malloc_clear(sizeof(int) * (size_t)num);
    // never allocate the zero-th pool item since the invalid id is 0
    for (int i = pool->size-1; i >= 1; i--) {
        pool->free_queue[pool->queue_top++] = i;
    }
}

static void _scb_discard_pool(_scb_pool_t* pool) {
    SOKOL_ASSERT(pool);
    SOKOL_ASSERT(pool->free_queue);
    _scb_free(pool->free_queue);
    pool->free_queue = 0;
    SOKOL_ASSERT(pool->gen_ctrs);
    _scb_free(pool->gen_ctrs);
    pool->gen_ctrs = 0;
    pool->size = 0;
    pool->queue_top = 0;
}

static int _scb_pool_alloc_index(_scb_pool_t* pool) {
    SOKOL_ASSERT(pool);
    SOKOL_ASSERT(pool->free_queue);
    if (pool->queue_top > 0) {
        int slot_index = pool->free_queue[--pool->queue_top];
        SOKOL_ASSERT((slot_index > 0) && (slot_index < pool->size));
        return slot_index;
    } else {
        // pool exhausted
        return _SCB_INVALID_SLOT_INDEX;
    }
}

static void _scb_pool_free_index(_scb_pool_t* pool, int slot_index) {
    SOKOL_ASSERT((slot_index > _SCB_INVALID_SLOT_INDEX) && (slot_index < pool->size));
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

static void _scb_setup_cmdbuf_pool(const scb_desc_t* desc) {
    SOKOL_ASSERT(desc);
    // note: the pool will have an additional item, since slot 0 is reserved
    SOKOL_ASSERT((desc->context_pool_size > 0) && (desc->context_pool_size < _SCB_MAX_POOL_SIZE));
    _scb_init_pool(&_scb.context_pool.pool, desc->context_pool_size);
    size_t pool_byte_size = sizeof(_scb_context_t) * (size_t)_scb.context_pool.pool.size;
    _scb.context_pool.contexts = (_scb_context_t*) _scb_malloc_clear(pool_byte_size);
}

static void _scb_discard_cmdbuf_pool(void) {
    SOKOL_ASSERT(_scb.context_pool.contexts);
    _scb_free(_scb.context_pool.contexts);
    _scb.context_pool.contexts = 0;
    _scb_discard_pool(&_scb.context_pool.pool);
}

/* allocate the slot at slot_index:
    - bump the slot's generation counter
    - create a resource id from the generation counter and slot index
    - set the slot's id to this id
    - set the slot's state to ALLOC
    - return the resource id
*/
static uint32_t _scb_slot_alloc(_scb_pool_t* pool, _scb_slot_t* slot, int slot_index) {
    /* FIXME: add handling for an overflowing generation counter,
       for now, just overflow (another option is to disable
       the slot)
    */
    SOKOL_ASSERT(pool && pool->gen_ctrs);
    SOKOL_ASSERT((slot_index > _SCB_INVALID_SLOT_INDEX) && (slot_index < pool->size));
    SOKOL_ASSERT(slot->id == SG_INVALID_ID);
    uint32_t ctr = ++pool->gen_ctrs[slot_index];
    slot->id = (ctr<<_SCB_SLOT_SHIFT)|(slot_index & _SCB_SLOT_MASK);
    return slot->id;
}

// extract slot index from id
static int _scb_slot_index(uint32_t id) {
    int slot_index = (int) (id & _SCB_SLOT_MASK);
    SOKOL_ASSERT(_SCB_INVALID_SLOT_INDEX != slot_index);
    return slot_index;
}

// get cmdbuf pointer without id-check
static _scb_cmdbuf_t* _scb_cmdbuf_at(uint32_t cb_id) {
    SOKOL_ASSERT(SG_INVALID_ID != cb_id);
    int slot_index = _scb_slot_index(cb_id);
    SOKOL_ASSERT((slot_index > _SCB_INVALID_SLOT_INDEX) && (slot_index < _scb.cmdbuf_pool.pool.size));
    return &_scb.cmdbuf_pool.cmdbufs[slot_index];
}

// get cmdbuf pointer with id-check, returns 0 if no match
static _scb_cmdbuf_t* _scb_lookup_cmdbuf(uint32_t cb_id) {
    if (SG_INVALID_ID != cb_id) {
        _scb_cmdbuf_t* cb = _scb_cmdbuf_at(cb_id);
        if (cb->slot.id == cb_id) {
            return cb;
        }
    }
    return 0;
}

// make cmdbuf handle from raw uint32_t id
static scb_context _scb_make_cmdbuf_id(uint32_t cb_id) {
    scb_cmdbuf cb;
    cb.id = cb_id;
    return cb;
}

static scb_cmdbuf _scb_alloc_cmdbuf(void) {
    scb_cmdbuf cb_id;
    int slot_index = _scb_pool_alloc_index(&_scb.cmdbuf_pool.pool);
    if (_SCB_INVALID_SLOT_INDEX != slot_index) {
        cb_id = _scb_make_cmdbuf_id(_scb_slot_alloc(&_scb.cmdbuf_pool.pool, &_scb.cmdbuf_pool.cmdbufs[slot_index].slot, slot_index));
    } else {
        // pool is exhausted
        cb_id = _scb_make_cmdbuf_id(SCB_INVALID_ID);
    }
    return cb_id;
}

static scb_cmdbuf_desc _scb_cmdbuf_desc_defaults(const scb_cmdbuf_desc* desc) {
    scb_cmdbuf_desc res = *desc;
    res.size = _scb_def(res.size, _SCB_DEFAULT_CMDBUF_SIZE);
    return res;
}




#endif // SOKOL_CMDBUF_IMPL
