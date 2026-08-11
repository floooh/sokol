#if defined(SOKOL_IMPL) && !defined(SOKOL_CMDBUF_IMPL)
#define SOKOL_CMDBUF_IMPL
#endif
#ifndef SOKOL_CMDBUF_INCLUDED
/*
    sokol_cmdbuf.h -- command buffers for deferred execution

    Project URL: https://github.com/floooh/sokol

    Optionally provide the following defines with your own implementations:

    SOKOL_ASSERT(c)       - your own assert macro (default: assert(c))
    SOKOL_CMDBUF_API_DECL - public function declaration prefix (default: extern)
    SOKOL_API_DECL        - same as SOKOL_CMDBUF_API_DECL
    SOKOL_API_IMPL        - public function implementation prefix (default: -)

    If sokol_cmdbuf.h is compiled as a DLL, define the following before
    including the declaration or implementation:

    SOKOL_DLL

    WHAT
    ====
    Provides memory buffers for recording sokol-gfx commands (like viewport changes,
    pipeline bindings, and draw calls) for deferred execution.

    HOW
    ===
    First initialize sokol_cmdbuf.h via:

        scb_setup(&(scb_desc){
            .logger.func = slog_func,
        });

    If you need more than 8 command buffers at the same time, increase the pool size

        scb_setup(&(scb_desc){
            .buffer_pool_size = 33,
            .logger.func = slog_func,
        });

    You can also provide a custom allocator for the internal byte arenas:

        scb_setup(&(scb_desc){
            .buffer_pool_size = 33,
            .allocator = {
                .alloc_fn = my_malloc,
                .free_fn = my_free,
                .user_data = my_user_data,
            },
            .logger.func = slog_func,
        });

    Next, create a command buffer. You must provide a maximum capacity in bytes.
    This allocates a backing memory arena for the commands:

        scb_buffer cb = scb_make_buffer(&(scb_buffer_desc){
            .size = 65535,
        });

    Now you can record commands into the buffer. These functions mirror their
    sokol-gfx equivalents, but they don't execute immediately. INstead,
    they serialize their parameters into the buffer's byte arena:

        scb_apply_pipeline(cb, pip);
        scb_apply_bindings(cb, &bind);
        scb_apply_uniforms(cb, 0, &SG_RANGE(uniforms));
        scb_draw(cb, 0, 3, 1);

    NOTE: If a buffer exceeds its byte capacity during recording,
    subsequent commands are ignored and a warning is logged.

    When you call scb_apply_bindings() or scb_apply_uniforms()
    sokol-cmdbuf performs a copy of the data directly into the buffer.
    This means that you *do not* have to keep the data alive past the moment
    the apply function returns. You can pass pointers to local stack variables
    or temporary data.

    To execute the recorded commands, call scb_submit() *inside* an active
    sokol-gfx render pass:

        sg_begin_pass(...);
        scb_submit(cb);
        sg_end_pass();
        sg_commit();

    Calling scb_submit() *does not* clear the buffer. This allows to
    submit a cached static buffer multiple times.

    To record new commands into an existing buffer (e.g. at the start
    of a new frame) you must manually rewind its internal write offset:

        scb_rewind(cb);

    To destroy a buffer and free its memory arena:

        scb_destroy_buffer(cb);

    ...calling scb_shutdown() will also destroy any remaining buffer
    objects:

        scb_shutdown();

    FUTURE PLANS
    ============
    - Support multithreading.
      Command buffers could be populated in threads,
      but resource update calls would still need
      to happen on the main-thread.
      (via floooh)
    - Extend API with resource update calls.
      (also via floooh)

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
#include <stddef.h> // size_t
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

/*
    Public constants.
*/
enum {
    // the value of an invalid command buffer handle
    SCB_INVALID_ID = 0,
};

/*
    scb_resource_state

    The state of a command buffer object, obtainable via scb_query_buffer_state().
    Publicly visible values are only SCB_RESOURCESTATE_VALID
    and SCB_RESOURCESTATE_FAILED.
*/
typedef enum scb_resource_state {
    SCB_RESOURCESTATE_INITIAL,
    SCB_RESOURCESTATE_ALLOC,
    SCB_RESOURCESTATE_VALID,
    SCB_RESOURCESTATE_FAILED,
    SCB_RESOURCESTATE_INVALID,
    _SCB_RESOURCESTATE_FORCE_U32 = 0x7FFFFFFF
} scb_resource_state;

/*
    scb_buffer

    A command buffer handle, created with scb_make_buffer(), destroyed
    with scb_destroy_buffer()
*/
typedef struct scb_buffer { uint32_t id; } scb_buffer;

/*
    scb_buffer_desc

    Creation parameters for a command buffer object. Passed into scb_make_buffer().
*/
typedef struct scb_buffer_desc {
    size_t size; // max capacity of the command buffer, in bytes
} scb_buffer_desc;

/*
    scb_allocator

    Used in scb_desc to provide custom memory-alloc and -free functions
    to sokol_cmdbuf.h. If memory management should be overridden, both the
    alloc and free function must be provided (e.g. it's not valid to
    override one function but not the other).
*/
typedef struct scb_allocator {
    void* (*alloc_fn)(size_t size, void* user_data);
    void (*free_fn)(void* ptr, void* user_data);
    void* user_data;
} scb_allocator;

/*
    scb_logger

    Used in scb_desc to provide a custom logging and error reporting
    callback to sokol_cmdbuf.h.
*/
typedef struct scb_logger {
    void (*func)(
        const char* tag,                // always "scb"
        uint32_t log_level,             // 0=panic, 1=error, 2=warning, 3=info
        uint32_t log_item_id,           // SCB_LOGITEM_*
        const char* message_or_null,    // a message string, may be nullptr in release mode
        uint32_t line_nr,               // line number in sokol_cmdbuf.h
        const char* filename_or_null,   // source filename, may be nullptr in release mode
        void* user_data);
    void* user_data;
} scb_logger;

/*
    scb_desc

    Initialization parameters passed into scb_setup(). You should at least
    provide a logging function, otherwise you won't see any error logging.
*/
typedef struct scb_desc {
    int buffer_pool_size;      // default: 128
    scb_allocator allocator;
    scb_logger logger;
} scb_desc;

// setup sokol-cmdbuf
SOKOL_CMDBUF_API_DECL void scb_setup(const scb_desc* desc);
// shutdown sokol-cmdbuf
SOKOL_CMDBUF_API_DECL void scb_shutdown(void);

// create a buffer object
SOKOL_CMDBUF_API_DECL scb_buffer scb_make_buffer(const scb_buffer_desc* desc);
// destroy a buffer object
SOKOL_CMDBUF_API_DECL void scb_destroy_buffer(scb_buffer buf);
// reset a buffer so it can be recorded into from the beginning without submitting
SOKOL_CMDBUF_API_DECL void scb_rewind(scb_buffer buf);

// apply viewport state
SOKOL_CMDBUF_API_DECL void scb_apply_viewport(scb_buffer buf, int x, int y, int width, int height, bool origin_top_left);
SOKOL_CMDBUF_API_DECL void scb_apply_viewportf(scb_buffer buf, float x, float y, float width, float height, bool origin_top_left);
// apply scissor rectangle state
SOKOL_CMDBUF_API_DECL void scb_apply_scissor_rect(scb_buffer buf, int x, int y, int width, int height, bool origin_top_left);
SOKOL_CMDBUF_API_DECL void scb_apply_scissor_rectf(scb_buffer buf, float x, float y, float width, float height, bool origin_top_left);
// apply pipeline object
SOKOL_CMDBUF_API_DECL void scb_apply_pipeline(scb_buffer buf, sg_pipeline pip);
// apply resource bindings NOTE: copied to the buffer
SOKOL_CMDBUF_API_DECL void scb_apply_bindings(scb_buffer buf, const sg_bindings* bindings);
// apply uniform data block NOTE: copied to the buffer
SOKOL_CMDBUF_API_DECL void scb_apply_uniforms(scb_buffer buf, int ub_slot, const sg_range* data);
// issue a draw call
SOKOL_CMDBUF_API_DECL void scb_draw(scb_buffer buf, int base_element, int num_elements, int num_instances);
SOKOL_CMDBUF_API_DECL void scb_draw_ex(scb_buffer buf, int base_element, int num_elements, int num_instances, int base_vertex, int base_instance);
// issue a compute dispatch call
SOKOL_CMDBUF_API_DECL void scb_dispatch(scb_buffer buf, int num_groups_x, int num_groups_y, int num_groups_z);

// submit and execute the recorded command buffer in an active render pass
SOKOL_CMDBUF_API_DECL void scb_submit(scb_buffer buf);

#ifdef __cplusplus
} /* extern "C" */
inline void scb_setup(const scb_desc& desc) { return scb_setup(&desc); }
inline scb_buffer scb_make_buffer(const scb_buffer_desc& desc) { return scb_make_buffer(&desc); }
inline void scb_apply_bindings(scb_buffer buf, const sg_bindings& bindings) { return scb_apply_bindings(buf, &bindings); }
inline void scb_apply_uniforms(scb_buffer buf, int ub_slot, const sg_range& data) { return scb_apply_uniforms(buf, ub_slot, &data); }
#endif
#endif // SOKOL_CMDBUF_INCLUDED

/*=== IMPLEMENTATION =========================================================*/
#ifdef SOKOL_CMDBUF_IMPL
#define SOKOL_CMDBUF_IMPL_INCLUDED (1)

#include <stdlib.h> // malloc, free, abort
#include <string.h> // memset

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

#define _scb_def(val, def) (((val) == 0) ? (def) : (val))

// >>structs
enum {
    _SCB_SLOT_SHIFT = 16,
    _SCB_SLOT_MASK = (1<<_SCB_SLOT_SHIFT)-1,
    _SCB_MAX_POOL_SIZE = (1<<_SCB_SLOT_SHIFT),
    _SCB_DEFAULT_BUFFER_POOL_SIZE = 128,
};

#define _SCB_INVALID_SLOT_INDEX (0)

typedef struct {
    uint32_t id;
    scb_resource_state state;
} _scb_slot_t;

typedef enum _scb_cmd_t {
    _SCB_CMD_APPLY_VIEWPORT,
    _SCB_CMD_APPLY_VIEWPORTF,
    _SCB_CMD_APPLY_SCISSOR_RECT,
    _SCB_CMD_APPLY_SCISSOR_RECTF,
    _SCB_CMD_APPLY_PIPELINE,
    _SCB_CMD_APPLY_BINDINGS,
    _SCB_CMD_APPLY_UNIFORMS,
    _SCB_CMD_DRAW,
    _SCB_CMD_DRAW_EX,
    _SCB_CMD_DISPATCH,
} _scb_cmd_t;

// the generic header that prefixes every command
typedef struct {
    _scb_cmd_t cmd;
} _scb_cmd_header_t;

// specific command payloads
typedef struct {
    _scb_cmd_t cmd;
    int x;
    int y;
    int width;
    int height;
    bool origin_top_left;
} _scb_cmd_viewport_t;

typedef struct {
    _scb_cmd_t cmd;
    float x;
    float y;
    float width;
    float height;
    bool origin_top_left;
} _scb_cmd_viewportf_t;

typedef struct {
    _scb_cmd_t cmd;
    int x;
    int y;
    int width;
    int height;
    bool origin_top_left;
} _scb_cmd_scissor_rect_t;

typedef struct {
    _scb_cmd_t cmd;
    float x;
    float y;
    float width;
    float height;
    bool origin_top_left;
} _scb_cmd_scissor_rectf_t;

typedef struct {
    _scb_cmd_t cmd;
    sg_pipeline pip;
} _scb_cmd_pipeline_t;

typedef struct {
    _scb_cmd_t cmd;
    sg_bindings bindings; // NOTE: deep copy of the bidings struct
} _scb_cmd_bindings_t;

typedef struct {
    _scb_cmd_t cmd;
    int ub_slot;
    size_t data_size;
    // NOTE: the actual uniform data bytes follow immediately in memory
} _scb_cmd_uniforms_t;

typedef struct {
    _scb_cmd_t cmd;
    int base_element;
    int num_elements;
    int num_instances;
} _scb_cmd_draw_t;

typedef struct {
    _scb_cmd_t cmd;
    int base_element;
    int num_elements;
    int num_instances;
    int base_vertex;
    int base_instance;
} _scb_cmd_draw_ex_t;

typedef struct {
    _scb_cmd_t cmd;
    int num_groups_x;
    int num_groups_y;
    int num_groups_z;
} _scb_cmd_dispatch_t;

// the actual internal command buffer object
typedef struct {
    _scb_slot_t slot; // the pool slot this buffer occupies
    size_t size;      // total allocated bytes
    size_t offset;    // current write position
    bool overflow;    // flagged if a command exceeds capacity
    uint8_t* ptr;     // raw memory arena
} _scb_buffer_t;

// the resource pool for allocating buffers
typedef struct {
    int size;
    int queue_top;
    uint32_t* gen_ctrs;
    int* queue;
} _scb_pool_t;

typedef struct {
    _scb_pool_t buffer_pool;
    _scb_buffer_t* buffers;
} _scb_pools_t;

typedef struct {
    uint32_t init_tag;
    scb_desc desc;
    _scb_pools_t pools;
} _scb_state_t;
static _scb_state_t _scb;

// >>logging
#define _SCB_LOG_ITEMS \
    _SCB_LOGITEM_XMACRO(OK, "Ok") \
    _SCB_LOGITEM_XMACRO(MALLOC_FAILED, "memory allocation failed") \
    _SCB_LOGITEM_XMACRO(BUFFER_POOL_EXHAUSTED, "command buffer pool exhausted (scb_desc.buffer_pool_size)") \

#define _SCB_LOGITEM_XMACRO(item,msg) _SCB_LOGITEM_##item,
typedef enum {
    _SCB_LOG_ITEMS
} _scb_log_item_t;
#undef _SCB_LOGITEM_XMACRO

#if defined(SOKOL_DEBUG)
#define _SCB_LOGITEM_XMACRO(item,msg) #item ": " msg,
static const char* _scb_log_messages[] = {
    _SCB_LOG_ITEMS
};
#undef _SCB_LOGITEM_XMACRO
#endif // SOKOL_DEBUG

#define _SCB_PANIC(code) _scb_log(_SCB_LOGITEM_ ##code, 0, 0, __LINE__)
#define _SCB_ERROR(code) _scb_log(_SCB_LOGITEM_ ##code, 1, 0, __LINE__)
#define _SCB_WARN(code) _scb_log(_SCB_LOGITEM_ ##code, 2, 0, __LINE__)
#define _SCB_INFO(code) _scb_log(_SCB_LOGITEM_ ##code, 3, 0, __LINE__)
#define _SCB_LOGMSG(code,msg) _scb_log(_SCB_LOGITEM_ ##code, 3, msg, __LINE__)

static void _scb_log(_scb_log_item_t log_item, uint32_t log_level, const char* msg, uint32_t line_nr) {
    if (_scb.desc.logger.func) {
        const char* filename = 0;
        #if defined(SOKOL_DEBUG)
            filename = __FILE__;
            if (0 == msg) {
                msg = _scb_log_messages[log_item];
            }
        #endif
        _scb.desc.logger.func("scb", log_level, (uint32_t)log_item, msg, line_nr, filename, _scb.desc.logger.user_data);
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
static void _scb_pool_init(_scb_pool_t* pool, int num) {
    SOKOL_ASSERT(pool && (num >= 1));
    // slot 0 is reserved for the 'invalid id', so bump the pool size by 1
    pool->size = num + 1;
    pool->queue_top = 0;
    // generation counters indexable by pool slot index, slot 0 is reserved
    size_t gen_ctrs_size = sizeof(uint32_t) * (size_t)pool->size;
    pool->gen_ctrs = (uint32_t*)_scb_malloc_clear(gen_ctrs_size);
    // it's not a bug to only reserve 'num' here
    pool->queue = (int*) _scb_malloc_clear(sizeof(int) * (size_t)num);
    // never allocate the zero-th pool item since the invalid id is 0
    for (int i = pool->size-1; i >= 1; i--) {
        pool->queue[pool->queue_top++] = i;
    }
}

static void _scb_pool_discard(_scb_pool_t* pool) {
    SOKOL_ASSERT(pool);
    SOKOL_ASSERT(pool->queue);
    _scb_free(pool->queue);
    pool->queue = 0;
    SOKOL_ASSERT(pool->gen_ctrs);
    _scb_free(pool->gen_ctrs);
    pool->gen_ctrs = 0;
    pool->size = 0;
    pool->queue_top = 0;
}


static int _scb_pool_alloc_index(_scb_pool_t* pool) {
    SOKOL_ASSERT(pool);
    SOKOL_ASSERT(pool->queue);
    if (pool->queue_top > 0) {
        int slot_index = pool->queue[--pool->queue_top];
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
    SOKOL_ASSERT(pool->queue);
    SOKOL_ASSERT(pool->queue_top < pool->size);
    #ifdef SOKOL_DEBUG
    // debug check against double-free
    for (int i = 0; i < pool->queue_top; i++) {
        SOKOL_ASSERT(pool->queue[i] != slot_index);
    }
    #endif
    pool->queue[pool->queue_top++] = slot_index;
    SOKOL_ASSERT(pool->queue_top <= (pool->size-1));
}

static void _scb_setup_pools(_scb_pools_t* p, const scb_desc* desc) {
    SOKOL_ASSERT(p);
    SOKOL_ASSERT(desc);
    // NOTE: the pools here will have an additional item, since slot 0 is reserved
    SOKOL_ASSERT((desc->buffer_pool_size > 0) && (desc->buffer_pool_size < _SCB_MAX_POOL_SIZE));
    _scb_pool_init(&p->buffer_pool, desc->buffer_pool_size);
    size_t buf_pool_byte_size = sizeof(_scb_buffer_t) * (size_t)p->buffer_pool.size;
    p->buffers = (_scb_buffer_t*) _scb_malloc_clear(buf_pool_byte_size);
}

static void _scb_discard_pools(_scb_pools_t* p) {
    SOKOL_ASSERT(p);
    _scb_free(p->buffers); p->buffers = 0;
    _scb_pool_discard(&p->buffer_pool);
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
    SOKOL_ASSERT(slot->id == SCB_INVALID_ID);
    SOKOL_ASSERT(slot->state == SCB_RESOURCESTATE_INITIAL);
    uint32_t ctr = ++pool->gen_ctrs[slot_index];
    slot->id = (ctr<<_SCB_SLOT_SHIFT)|(slot_index & _SCB_SLOT_MASK);
    slot->state = SCB_RESOURCESTATE_ALLOC;
    return slot->id;
}

// extract slot index from id
static int _scb_slot_index(uint32_t id) {
    int slot_index = (int) (id & _SCB_SLOT_MASK);
    SOKOL_ASSERT(_SCB_INVALID_SLOT_INDEX != slot_index);
    return slot_index;
}

// returns pointer to resource by id without matching id check
static _scb_buffer_t* _scb_buffer_at(uint32_t cb_id) {
    SOKOL_ASSERT(SCB_INVALID_ID != cb_id);
    int slot_index = _scb_slot_index(cb_id);
    SOKOL_ASSERT((slot_index > _SCB_INVALID_SLOT_INDEX) && (slot_index < _scb.pools.buffer_pool.size));
    return &_scb.pools.buffers[slot_index];
}

// returns pointer to resource with matching id check, may return 0
static _scb_buffer_t* _scb_lookup_buffer(uint32_t cb_id) {
    if (SCB_INVALID_ID != cb_id) {
        _scb_buffer_t* cb = _scb_buffer_at(cb_id);
        if (cb->slot.id == cb_id) {
            return cb;
        }
    }
    return 0;
}

static scb_buffer _scb_alloc_buffer(void) {
    scb_buffer res;
    int slot_index = _scb_pool_alloc_index(&_scb.pools.buffer_pool);
    if (_SCB_INVALID_SLOT_INDEX != slot_index) {
        res.id = _scb_slot_alloc(&_scb.pools.buffer_pool, &_scb.pools.buffers[slot_index].slot, slot_index);
    } else {
        res.id = SCB_INVALID_ID;
        _SCB_ERROR(BUFFER_POOL_EXHAUSTED);
    }
    return res;
}

static void _scb_dealloc_buffer(_scb_buffer_t* cb) {
    SOKOL_ASSERT(cb && (cb->slot.state == SCB_RESOURCESTATE_ALLOC) && (cb->slot.id != SCB_INVALID_ID));
    _scb_pool_free_index(&_scb.pools.buffer_pool, _scb_slot_index(cb->slot.id));
    _scb_clear(cb, sizeof(_scb_buffer_t));
}


static scb_desc _scb_desc_defaults(const scb_desc* desc) {
    SOKOL_ASSERT(desc);
    scb_desc res = *desc;
    res.buffer_pool_size = _scb_def(res.buffer_pool_size, _SCB_DEFAULT_BUFFER_POOL_SIZE);
    return res;
}

static void _scb_init_buffer(_scb_buffer_t* cb, const scb_buffer_desc* desc) {
    SOKOL_ASSERT(cb && (cb->slot.state == SCB_RESOURCESTATE_ALLOC));
    SOKOL_ASSERT(desc);
    SOKOL_ASSERT(desc->size > 0);
    cb->size = desc->size;
    cb->offset = 0;
    cb->overflow = false;
    cb->ptr = (uint8_t*)_scb_malloc(desc->size);
    bool valid = (cb->ptr != 0);
    cb->slot.state = valid ? SCB_RESOURCESTATE_VALID : SCB_RESOURCESTATE_FAILED;
}

static void _scb_uninit_buffer(_scb_buffer_t* cb) {
    SOKOL_ASSERT(cb && ((cb->slot.state == SCB_RESOURCESTATE_VALID) || (cb->slot.state == SCB_RESOURCESTATE_FAILED)));
    if (cb->ptr) {
        _scb_free(cb->ptr);
        cb->ptr = 0;
    }
    cb->slot.state = SCB_RESOURCESTATE_ALLOC;
}

static void _scb_discard_all_resources(void) {
    for (int i = 1; i < _scb.pools.buffer_pool.size; i++) {
        scb_resource_state state = _scb.pools.buffers[i].slot.state;
        if ((state == SCB_RESOURCESTATE_VALID) || (state == SCB_RESOURCESTATE_FAILED)) {
            _scb_uninit_buffer(&_scb.pools.buffers[i]);
        }
    }
}

// >>commands
// align to 8-byte to prevent unaligned memory access
#define _SCB_ALIGN8(s) (((s) + 7) & ~7)

static void* _scb_append(_scb_buffer_t* cb, size_t size) {
    size_t align_size = _SCB_ALIGN8(size);
    if ((cb->offset + align_size) > cb->size) {
        if (!cb->overflow) {
            _SCB_WARN(BUFFER_POOL_EXHAUSTED);
        }
        cb->overflow = true;
        return 0;
    }
    void* ptr = cb->ptr + cb->offset;
    cb->offset += align_size;
    return ptr;
}

static void _scb_execute(_scb_buffer_t* cb) {
    SOKOL_ASSERT(cb);
    if (cb->offset == 0) {
        return;
    }
    size_t read_offset = 0;
    while (read_offset < cb->offset) {
        _scb_cmd_header_t* header = (_scb_cmd_header_t*) (cb->ptr + read_offset);
        size_t cmd_size = 0;

        switch (header->cmd) {
            case _SCB_CMD_APPLY_VIEWPORT:
                {
                    _scb_cmd_viewport_t* cmd = (_scb_cmd_viewport_t*) header;
                    sg_apply_viewport(cmd->x, cmd->y, cmd->width, cmd->height, cmd->origin_top_left);
                    cmd_size = sizeof(_scb_cmd_viewport_t);
                }
                break;
            case _SCB_CMD_APPLY_VIEWPORTF:
                {
                    _scb_cmd_viewportf_t* cmd = (_scb_cmd_viewportf_t*) header;
                    sg_apply_viewportf(cmd->x, cmd->y, cmd->width, cmd->height, cmd->origin_top_left);
                    cmd_size = sizeof(_scb_cmd_viewportf_t);
                }
                break;
            case _SCB_CMD_APPLY_SCISSOR_RECT:
                {
                    _scb_cmd_scissor_rect_t* cmd = (_scb_cmd_scissor_rect_t*) header;
                    sg_apply_scissor_rect(cmd->x, cmd->y, cmd->width, cmd->height, cmd->origin_top_left);
                    cmd_size = sizeof(_scb_cmd_scissor_rect_t);
                }
                break;
            case _SCB_CMD_APPLY_SCISSOR_RECTF:
                {
                    _scb_cmd_scissor_rectf_t* cmd = (_scb_cmd_scissor_rectf_t*) header;
                    sg_apply_scissor_rectf(cmd->x, cmd->y, cmd->width, cmd->height, cmd->origin_top_left);
                    cmd_size = sizeof(_scb_cmd_scissor_rectf_t);
                }
                break;
            case _SCB_CMD_APPLY_PIPELINE:
                {
                    _scb_cmd_pipeline_t* cmd = (_scb_cmd_pipeline_t*) header;
                    sg_apply_pipeline(cmd->pip);
                    cmd_size = sizeof(_scb_cmd_pipeline_t);
                }
                break;
            case _SCB_CMD_APPLY_BINDINGS:
                {
                    _scb_cmd_bindings_t* cmd = (_scb_cmd_bindings_t*) header;
                    sg_apply_bindings(&cmd->bindings);
                    cmd_size = sizeof(_scb_cmd_bindings_t);
                }
                break;
            case _SCB_CMD_APPLY_UNIFORMS:
                {
                    _scb_cmd_uniforms_t* cmd = (_scb_cmd_uniforms_t*) header;
                    sg_range uniform_data;
                    uniform_data.size = cmd->data_size;
                    uniform_data.ptr = (const void*)(cmd + 1); // data lives immediately after struct
                    sg_apply_uniforms(cmd->ub_slot, &uniform_data);
                    cmd_size = sizeof(_scb_cmd_uniforms_t) + cmd->data_size;
                }
                break;
            case _SCB_CMD_DRAW:
                {
                    _scb_cmd_draw_t* cmd = (_scb_cmd_draw_t*) header;
                    sg_draw(cmd->base_element, cmd->num_elements, cmd->num_instances);
                    cmd_size = sizeof(_scb_cmd_draw_t);
                }
                break;
            case _SCB_CMD_DRAW_EX:
                {
                    _scb_cmd_draw_ex_t* cmd = (_scb_cmd_draw_ex_t*) header;
                    sg_draw_ex(cmd->base_element, cmd->num_elements, cmd->num_instances, cmd->base_vertex, cmd->base_instance);
                    cmd_size = sizeof(_scb_cmd_draw_ex_t);
                }
                break;
            case _SCB_CMD_DISPATCH:
                {
                    _scb_cmd_dispatch_t* cmd = (_scb_cmd_dispatch_t*) header;
                    sg_dispatch(cmd->num_groups_x, cmd->num_groups_y, cmd->num_groups_z);
                    cmd_size = sizeof(_scb_cmd_dispatch_t);
                }
                break;
            default:
                SOKOL_ASSERT(false);
                return;
        }

        read_offset += _SCB_ALIGN8(cmd_size);
    }
}

// >>public
#define _SCB_INIT_TAG (0xDCBADCBA)

SOKOL_API_IMPL void scb_setup(const scb_desc* desc) {
    SOKOL_ASSERT(desc);
    SOKOL_ASSERT((desc->allocator.alloc_fn && desc->allocator.free_fn) || (!desc->allocator.alloc_fn && !desc->allocator.free_fn));
    SOKOL_ASSERT(_scb.init_tag != _SCB_INIT_TAG);
    _scb_clear(&_scb, sizeof(_scb));
    _scb.init_tag = _SCB_INIT_TAG;
    _scb.desc = _scb_desc_defaults(desc);
    _scb_setup_pools(&_scb.pools, &_scb.desc);
}

SOKOL_API_IMPL void scb_shutdown(void) {
    SOKOL_ASSERT(_SCB_INIT_TAG == _scb.init_tag);
    _scb_discard_all_resources();
    _scb_discard_pools(&_scb.pools);
    _scb_clear(&_scb, sizeof(_scb));
}

SOKOL_API_IMPL scb_buffer scb_make_buffer(const scb_buffer_desc* desc) {
    SOKOL_ASSERT(_SCB_INIT_TAG == _scb.init_tag);
    SOKOL_ASSERT(desc);
    scb_buffer cb_id = _scb_alloc_buffer();
    if (cb_id.id != SCB_INVALID_ID) {
        _scb_buffer_t* buf = _scb_buffer_at(cb_id.id);
        SOKOL_ASSERT(buf && (buf->slot.state == SCB_RESOURCESTATE_ALLOC));
        _scb_init_buffer(buf, desc);
        SOKOL_ASSERT((buf->slot.state == SCB_RESOURCESTATE_VALID) || (buf->slot.state == SCB_RESOURCESTATE_FAILED));
    }
    return cb_id;
}

SOKOL_API_IMPL void scb_destroy_buffer(scb_buffer buf) {
    SOKOL_ASSERT(_SCB_INIT_TAG == _scb.init_tag);
    _scb_buffer_t* cb = _scb_lookup_buffer(buf.id);
    if (cb) {
        if ((cb->slot.state == SCB_RESOURCESTATE_VALID) || (cb->slot.state == SCB_RESOURCESTATE_FAILED)) {
            _scb_uninit_buffer(cb);
            SOKOL_ASSERT(cb->slot.state == SCB_RESOURCESTATE_ALLOC);
        }
        if (cb->slot.state == SCB_RESOURCESTATE_ALLOC) {
            _scb_dealloc_buffer(cb);
            SOKOL_ASSERT(cb->slot.state == SCB_RESOURCESTATE_INITIAL);
        }
    }
}

SOKOL_API_IMPL void scb_rewind(scb_buffer buf) {
    SOKOL_ASSERT(_SCB_INIT_TAG == _scb.init_tag);
    _scb_buffer_t* cb = _scb_lookup_buffer(buf.id);
    if (cb) {
        cb->offset = 0;
        cb->overflow = false;
    }
}

SOKOL_API_IMPL void scb_apply_viewport(scb_buffer buf, int x, int y, int width, int height, bool origin_top_left) {
    SOKOL_ASSERT(_SCB_INIT_TAG == _scb.init_tag);
    _scb_buffer_t* cb = _scb_lookup_buffer(buf.id);
    if (cb && !cb->overflow) {
        _scb_cmd_viewport_t* cmd = (_scb_cmd_viewport_t*) _scb_append(cb, sizeof(_scb_cmd_viewport_t));
        if (cmd) {
            cmd->cmd = _SCB_CMD_APPLY_VIEWPORT;
            cmd->x = x;
            cmd->y = y;
            cmd->width = width;
            cmd->height = height;
            cmd->origin_top_left = origin_top_left;
        }
    }
}

SOKOL_API_IMPL void scb_apply_viewportf(scb_buffer buf, float x, float y, float width, float height, bool origin_top_left) {
    SOKOL_ASSERT(_SCB_INIT_TAG == _scb.init_tag);
    _scb_buffer_t* cb = _scb_lookup_buffer(buf.id);
    if (cb && !cb->overflow) {
        _scb_cmd_viewportf_t* cmd = (_scb_cmd_viewportf_t*) _scb_append(cb, sizeof(_scb_cmd_viewportf_t));
        if (cmd) {
            cmd->cmd = _SCB_CMD_APPLY_VIEWPORTF;
            cmd->x = x;
            cmd->y = y;
            cmd->width = width;
            cmd->height = height;
            cmd->origin_top_left = origin_top_left;
        }
    }
}

SOKOL_API_IMPL void scb_apply_scissor_rect(scb_buffer buf, int x, int y, int width, int height, bool origin_top_left) {
    SOKOL_ASSERT(_SCB_INIT_TAG == _scb.init_tag);
    _scb_buffer_t* cb = _scb_lookup_buffer(buf.id);
    if (cb && !cb->overflow) {
        _scb_cmd_scissor_rect_t* cmd = (_scb_cmd_scissor_rect_t*) _scb_append(cb, sizeof(_scb_cmd_scissor_rect_t));
        if (cmd) {
            cmd->cmd = _SCB_CMD_APPLY_SCISSOR_RECT;
            cmd->x = x;
            cmd->y = y;
            cmd->width = width;
            cmd->height = height;
            cmd->origin_top_left = origin_top_left;
        }
    }
}

SOKOL_API_IMPL void scb_apply_scissor_rectf(scb_buffer buf, float x, float y, float width, float height, bool origin_top_left) {
    SOKOL_ASSERT(_SCB_INIT_TAG == _scb.init_tag);
    _scb_buffer_t* cb = _scb_lookup_buffer(buf.id);
    if (cb && !cb->overflow) {
        _scb_cmd_scissor_rectf_t* cmd = (_scb_cmd_scissor_rectf_t*) _scb_append(cb, sizeof(_scb_cmd_scissor_rectf_t));
        if (cmd) {
            cmd->cmd = _SCB_CMD_APPLY_SCISSOR_RECTF;
            cmd->x = x;
            cmd->y = y;
            cmd->width = width;
            cmd->height = height;
            cmd->origin_top_left = origin_top_left;
        }
    }
}

SOKOL_API_IMPL void scb_apply_pipeline(scb_buffer buf, sg_pipeline pip) {
    SOKOL_ASSERT(_SCB_INIT_TAG == _scb.init_tag);
    _scb_buffer_t* cb = _scb_lookup_buffer(buf.id);
    if (cb && !cb->overflow) {
        _scb_cmd_pipeline_t* cmd = (_scb_cmd_pipeline_t*) _scb_append(cb, sizeof(_scb_cmd_pipeline_t));
        if (cmd) {
            cmd->cmd = _SCB_CMD_APPLY_PIPELINE;
            cmd->pip = pip;
        }
    }
}

SOKOL_API_IMPL void scb_apply_bindings(scb_buffer buf, const sg_bindings* bindings) {
    SOKOL_ASSERT(_SCB_INIT_TAG == _scb.init_tag);
    _scb_buffer_t* cb = _scb_lookup_buffer(buf.id);
    if (cb && !cb->overflow) {
        _scb_cmd_bindings_t* cmd = (_scb_cmd_bindings_t*) _scb_append(cb, sizeof(_scb_cmd_bindings_t));
        if (cmd) {
            cmd->cmd = _SCB_CMD_APPLY_BINDINGS;
            cmd->bindings = *bindings;
        }
    }
}

SOKOL_API_IMPL void scb_apply_uniforms(scb_buffer buf, int ub_slot, const sg_range* data) {
    SOKOL_ASSERT(_SCB_INIT_TAG == _scb.init_tag);
    SOKOL_ASSERT(data && data->ptr && (data->size > 0));
    _scb_buffer_t* cb = _scb_lookup_buffer(buf.id);
    if (cb && !cb->overflow) {
        size_t total_size = sizeof(_scb_cmd_uniforms_t) + data->size;
        _scb_cmd_uniforms_t* cmd = (_scb_cmd_uniforms_t*) _scb_append(cb, total_size);
        if (cmd) {
            cmd->cmd = _SCB_CMD_APPLY_UNIFORMS;
            cmd->ub_slot = ub_slot;
            cmd->data_size = data->size;
            uint8_t* data_ptr = (uint8_t*)(cmd + 1);
            memcpy(data_ptr, data->ptr, data->size);
        }
    }
}

SOKOL_API_IMPL void scb_draw(scb_buffer buf, int base_element, int num_elements, int num_instances) {
    SOKOL_ASSERT(_SCB_INIT_TAG == _scb.init_tag);
    _scb_buffer_t* cb = _scb_lookup_buffer(buf.id);
    if (cb && !cb->overflow) {
        _scb_cmd_draw_t* cmd = (_scb_cmd_draw_t*) _scb_append(cb, sizeof(_scb_cmd_draw_t));
        if (cmd) {
            cmd->cmd = _SCB_CMD_DRAW;
            cmd->base_element = base_element;
            cmd->num_elements = num_elements;
            cmd->num_instances = num_instances;
        }
    }
}

SOKOL_API_IMPL void scb_draw_ex(scb_buffer buf, int base_element, int num_elements, int num_instances, int base_vertex, int base_instance) {
    SOKOL_ASSERT(_SCB_INIT_TAG == _scb.init_tag);
    _scb_buffer_t* cb = _scb_lookup_buffer(buf.id);
    if (cb && !cb->overflow) {
        _scb_cmd_draw_ex_t* cmd = (_scb_cmd_draw_ex_t*) _scb_append(cb, sizeof(_scb_cmd_draw_ex_t));
        if (cmd) {
            cmd->cmd = _SCB_CMD_DRAW_EX;
            cmd->base_element = base_element;
            cmd->num_elements = num_elements;
            cmd->num_instances = num_instances;
            cmd->base_vertex = base_vertex;
            cmd->base_instance = base_instance;
        }
    }
}

SOKOL_API_IMPL void scb_dispatch(scb_buffer buf, int num_groups_x, int num_groups_y, int num_groups_z) {
    SOKOL_ASSERT(_SCB_INIT_TAG == _scb.init_tag);
    _scb_buffer_t* cb = _scb_lookup_buffer(buf.id);
    if (cb && !cb->overflow) {
        _scb_cmd_dispatch_t* cmd = (_scb_cmd_dispatch_t*) _scb_append(cb, sizeof(_scb_cmd_dispatch_t));
        if (cmd) {
            cmd->cmd = _SCB_CMD_DISPATCH;
            cmd->num_groups_x = num_groups_x;
            cmd->num_groups_y = num_groups_y;
            cmd->num_groups_z = num_groups_z;
        }
    }
}

SOKOL_API_IMPL void scb_submit(scb_buffer buf) {
    SOKOL_ASSERT(_SCB_INIT_TAG == _scb.init_tag);
    _scb_buffer_t* cb = _scb_lookup_buffer(buf.id);
    if (cb) {
        _scb_execute(cb);
    }
}

#endif /* SOKOL_CMDBUF_IMPL */
