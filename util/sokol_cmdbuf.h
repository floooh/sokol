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


    OVERVIEW
    ========
    Allows to record sokol-gfx apply/draw/dispatch calls into command buffers
    outside of sokol-gfx passes and then submit the recorded calls inside
    sokol-gfx render or compute passes. This is mainly useful in two situations:

    - Interleaving resource updates and draw calls (e.g. append
      data to buffers and then immediately issue a draw/dispatch call which
      uses this data). Such an interleaved update/consume model cannot be
      implemented efficiently in some sokol-gfx backends and is disallowed in
      the 'new' write-transient/persistent update model.
    - Separating the core frame rendering code from code that's normally
      not concerned about rendering (e.g. UI or debug rendering).

    Some 'tier 2' sokol headers already use a similar record/replay
    system internally (e.g. sokol_gl.h, sokol_debugtext.h, sokol_spine.h)
    and will switch to using sokol_cmdbuf.h to reduce redundant code.

    STEP BY STEP:
    =============

    - Initialize sokol_cmdbuf.h, provide at least a logging function
      (for instance slog_func from sokol_log.h), otherwise you won't
      see any logging output:

        scb_setup(&(scb_desc){
            .logger.func = slog_func,
        });

      If you need more than (the default) 16 command buffers to be alive at
      the same time, set the .cmdbuf_pool_size:

        scb_setup(&(scb_desc){
            .cmdbuf_pool_size = 128,
            .logger.func = slog_func,
        });

      To provide your own memory allocation functions:

        void* my_alloc(size_t size, void* user_data) {
            return malloc(size);
        }

        void my_free(void* ptr, void* user_data) {
            free(ptr);
        }

        scb_setup(&(scb_desc){
            .allocator = {
                .alloc_fn = my_alloc,
                .free_fn = my_free,
                .user_data = ...,
            },
            .logger.func = slog_func,
        });

    - Next create command buffer objects, the default command buffer size
      is 256 kbytes:

        scb_cmdbuf cb = scb_make_cmdbuf(&(scb_cmdbuf_desc){0});

      It often makes sense to provide a specific size in bytes:

        scb_cmdbuf cb = scb_make_cmdbuf(&(scb_cmdbuf_desc){
            .size = 128 * 1024,     // 128 kbytes
        });

      For information on how to estimate the required size see the section
      'ESTIMATING COMMAND BUFFER SIZES' below.

      You can provide a label string for the command buffer:

        scb_cmdbuf cb = scb_make_cmdbuf(&(scb_cmdbuf_desc){
            .label = "dbg-physics",
        });

      When a label string exists, sokol_cmdbuf.h will wrap submitted commands
      with `sg_push_debug_group(label)` / `sg_pop_debug_group()`

    - Record apply/draw/dispatch commands into a command buffer object
      (note that these functions directly use sokol_gfx.h types):

        scb_apply_viewport(cb, x, y, width, height, origin_top_left);
        scb_apply_viewportf(cb, x, y, width, height, origin_top_left);

        scb_apply_scissor_rect(cb, x, y, width, height, origin_top_left);
        scb_apply_scissor_rectf(cb, x, y, width, height, origin_top_left);

        scb_apply_pipeline(cb, pip);
        scb_apply_bindings(cb, &(sg_bindings){ ... });
        scb_apply_uniforms(cb, ub_slot, &(sg_range){ ... });
        scb_draw(cb, base_element, num_elements, num_instances);
        scb_draw_ex(cb, base_element, num_elements, num_instances, base_vertex, base_instance);
        scb_dispatch(cb, num_groups_x, num_groups_y, num_groups_z);

      Uniform data will be copied into the command buffer, and with the
      required alignment.

      Trying to record more data than fits into the command buffer will
      result in a logged error message, and the command buffer to
      go into an 'overflown' state. Submitting an overflown command buffer
      will only rewind the command buffer but not issue the partially recorded
      commands to sokol-gfx.

    - Finally, inside a sokol-gfx render- or compute-pass, submit the
      command buffer. This will decode the recorded commands and call
      sokol-gfx functions:

        sg_begin_pass(...);
        // ...
        scb_submit(cb);
        // ...
        sg_end_pass();

      Submitting a command buffer will also automatically rewind, so that the
      command buffer can be reused for recording new commands.

    - To rewind a recorded command buffer without submitting, call:

        scb_reset(cb)

    - To get current information about a command buffer:

        scb_cmdbuf_info info = scb_query_cmdbuf_info(cb);

      The result contains:

        info.size       the command buffer size in bytes
        info.remaining  the currently remaining number of free bytes in the command buffer
        info.overflown  true when the command buffer is currently in overflown state

    - To get a command buffer's 'resource state', call:

        scb_resource_state state = scb_query_cmdbuf_state(cb);

      This returns one of:

        SCB_RESOURCESTATE_VALID:    the command buffer is valid to use
        SCB_RESOURCESTATE_FAILED:   command buffer allocation has failed
                                    (can only happen when memory allocation failed)
        SCB_RESOURCESTATE_INVALID   the handle is invalid or the command buffer
                                    no longer exists

    - To destroy a command buffer object:

        scb_destroy_cmdbuf(cb);

    - ...and finally to shutdown sokol_cmdbuf.h:

        scb_shutdown();

      ...this will also destroy all remaining command buffer objects.


    ESTIMATING COMMAND BUFFER SIZES
    ===============================

    For most commands, the size taken up in the command buffer can be
    estimated by adding the parameter sizes plus one byte for the
    command, e.g.:

    scb_apply_viewport takes 4 integers and one boolean:

        1 byte for the command
        + (4 * 4) bytes for the integers
        + 1 byte for the boolean

    There are two special cases:

    - scb_apply_uniforms copies the actual uniform data with 4-byte
        alignment into the command buffer, the required size is:

        1 byte for the command
        + 4 bytes for ub_slot
        + 4 bytes for the uniform data size (truncated from size_t)
        + up to 3 bytes 'alignment gap'
        + the actual uniform data

    - scb_apply_bindings applies a simple form of compression by
      not writing unoccupied bind slots. Instead a 64-bit bitmask identifies
      occupied slots:

        1 byte for the command
        + 8 bytes for the 64-bit occupation bitmask
        + 4 bytes for each valid sg_buffer, sg_view, sg_sampler
          handle in the sg_bindings struct
        + 4 bytes extra for the buffer offset of each occupied vertex buffer slot
        + 4 bytes extra for the index buffer offset if the index buffer slot is occupied

      ...or just assume around 256 bytes worst case for an scb_apply_bindings call


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
typedef struct scb_cmdbuf { uint32_t id; } scb_cmdbuf;

/*
    scb_resource_state

    The state of a command buffer object, obtainable via scb_query_cmdbuf_state().
    Publicly visible values are only SCB_RESOURCESTATE_VALID,
    SCB_RESOURCESTATE_FAILED and SCB_RESOURCESTATE_INVALID.
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
    scb_cmdbuf_desc

    Creation parameters of a command buffer object. Used
    in scb_make_cmdbuf().

    See doc section ESTIMATING COMMAND BUFFER SIZES about
    how command buffer size can be estimated.

    When a label is set, sokol_cmdbuf.h will wrap
    submitted commands with `sg_push/pop_debug_group()`.
*/
typedef struct scb_cmdbuf_desc {
    size_t size;        // command buffer size in bytes (default: 256 * 1024 (256 KBytes))
    const char* label;
} scb_cmdbuf_desc;

/*
    scb_cmdbuf_info

    Result of scb_query_cmdbuf_info.
*/
typedef struct scb_cmdbuf_info {
    size_t size;        // overall command buffer size in bytes
    size_t remaining;   // number of remaining bytes in command buffer
    bool overflown;     // set to true if the cmdbuf is in overflown state
} scb_cmdbuf_info;

/*
    scb_log_item

    Log items are defined via X-Macro, expanded into an enum scb_log_item
    and (in debug-mode only also to human readable strings).

    Used as parameter to the logging callback.
*/
#define _SCB_LOG_ITEMS \
    _SCB_LOGITEM_XMACRO(OK, "Ok") \
    _SCB_LOGITEM_XMACRO(MALLOC_FAILED, "memory allocation failed") \
    _SCB_LOGITEM_XMACRO(CMDBUF_POOL_EXHAUSTED, "command buffer pool is exhausted (hint: increase scb_desc.cmdbuf_pool_size)") \
    _SCB_LOGITEM_XMACRO(CMDBUF_OVERFLOW, "command buffer has overflown") \
    _SCB_LOGITEM_XMACRO(CMDBUF_NOT_VALID, "command buffer no longer exists or invalid handle") \
    _SCB_LOGITEM_XMACRO(SUBMIT_CMDBUF_OVERFLOWN, "scb_submit: command buffer was overflown") \
    _SCB_LOGITEM_XMACRO(SUBMIT_INVALID_COMMAND, "scb_submit: invalid command (command buffer corrupted?)") \

#define _SCB_LOGITEM_XMACRO(item,msg) SCB_LOGITEM_##item,
typedef enum scb_log_item {
    _SCB_LOG_ITEMS
} scb_log_item;
#undef _SCB_LOGITEM_XMACRO

/*
    scb_logger

    Used in scb_desc to provide a custom logging and error reporting
    callback to sokol_cmdbuf.h
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
    scb_allocator

    Used in scb_desc to provide custom memory-alloc and -free functions
    to sokol_cmdbuf.h. If memory management should be overridden, both the
    alloc_fn and free_fn function must be provided (e.g. it's not valid to
    override one function but not the other).
*/
typedef struct scb_allocator {
    void* (*alloc_fn)(size_t size, void* user_data);
    void (*free_fn)(void* ptr, void* user_data);
    void* user_data;
} scb_allocator;

/*
    scb_desc

    Initialization options passed into scb_setup.
*/
typedef struct scb_desc {
    int cmdbuf_pool_size;       // max number of command buffers that can be alive simultaneously (default: 16)
    scb_allocator allocator;    // optional memory allocation overrides (default: malloc/free)
    scb_logger logger;          // optional log override functions (default: NO LOGGING)
} scb_desc;

// setup sokol-cmdbuf
SOKOL_CMDBUF_API_DECL void scb_setup(const scb_desc* desc);
// shutdown sokol-cmdbuf
SOKOL_CMDBUF_API_DECL void scb_shutdown(void);

// create a cmdbuf object
SOKOL_CMDBUF_API_DECL scb_cmdbuf scb_make_cmdbuf(const scb_cmdbuf_desc* desc);
// destroy cmdbuf object
SOKOL_CMDBUF_API_DECL void scb_destroy_cmdbuf(scb_cmdbuf cb);
// submit command buffer to sokol-gfx and rewind the command buffer (call inside a sokol-gfx pass)
SOKOL_CMDBUF_API_DECL void scb_submit(scb_cmdbuf cb);
// reset a recorded command buffer, discarding its content
SOKOL_CMDBUF_API_DECL void scb_reset(scb_cmdbuf cb);

// record apply-viewport command (integer variant)
SOKOL_CMDBUF_API_DECL void scb_apply_viewport(scb_cmdbuf cb, int x, int y, int width, int height, bool origin_top_left);
// record apply-viewport command (float variant)
SOKOL_CMDBUF_API_DECL void scb_apply_viewportf(scb_cmdbuf cb, float x, float y, float width, float height, bool origin_top_left);
// record apply-scissor-rect command (integer variant)
SOKOL_CMDBUF_API_DECL void scb_apply_scissor_rect(scb_cmdbuf cb, int x, int y, int width, int height, bool origin_top_left);
// record apply-scissor-rect command (float variant)
SOKOL_CMDBUF_API_DECL void scb_apply_scissor_rectf(scb_cmdbuf cb, float x, float y, float width, float height, bool origin_top_left);
// record apply pipeline command
SOKOL_CMDBUF_API_DECL void scb_apply_pipeline(scb_cmdbuf cb, sg_pipeline pip);
// record apply bindings command
SOKOL_CMDBUF_API_DECL void scb_apply_bindings(scb_cmdbuf cb, const sg_bindings* bindings);
// record apply uniforms command
SOKOL_CMDBUF_API_DECL void scb_apply_uniforms(scb_cmdbuf cb, int ub_slot, const sg_range* data);
// record draw command
SOKOL_CMDBUF_API_DECL void scb_draw(scb_cmdbuf cb, int base_element, int num_elements, int num_instances);
// record draw-ex command
SOKOL_CMDBUF_API_DECL void scb_draw_ex(scb_cmdbuf cb, int base_element, int num_elements, int num_instances, int base_vertex, int base_instance);
// record dispatch command
SOKOL_CMDBUF_API_DECL void scb_dispatch(scb_cmdbuf cb, int num_groups_x, int num_groups_y, int num_groups_z);

// query command buffer resource state (valid, failed, invalid)
SOKOL_CMDBUF_API_DECL scb_resource_state scb_query_cmdbuf_state(scb_cmdbuf cb);
// query current command buffer properties
SOKOL_CMDBUF_API_DECL scb_cmdbuf_info scb_query_cmdbuf_info(scb_cmdbuf cb);

#ifdef __cplusplus
} // extern "C"
// C++ const-ref wrappers
inline void scb_setup(const scb_desc& desc) { return scb_setup(&desc); }
inline scb_cmdbuf scb_make_cmdbuf(const scb_cmdbuf_desc& desc) { return scb_make_cmdbuf(&desc); }
inline void scb_apply_bindings(scb_cmdbuf cb, const sg_bindings& bindings) { return scb_apply_bindings(cb, &bindings); }
inline void scb_apply_uniforms(scb_cmdbuf cb, int ub_slot, const sg_range& data) { return scb_apply_uniforms(cb, ub_slot, &data); }
#endif
#endif /* SOKOL_CMDBUF_INCLUDED */

//------------------------------------------------------------------------------
// >>implementation
#ifdef SOKOL_CMDBUF_IMPL
#define SOKOL_CMDBUF_IMPL_INCLUDED (1)

#include <string.h> // memset, memcpy, strncpy
#include <stdlib.h> // malloc/free/abort

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
#define _SCB_INIT_TAG (0xACBAABCA)

#define _SCB_STRING_SIZE (32)
#define _SCB_DEFAULT_CMDBUF_POOL_SIZE (16)
#define _SCB_DEFAULT_CMDBUF_SIZE (262144)   // 256 * 1024
#define _SCB_INVALID_SLOT_INDEX (0)
#define _SCB_SLOT_SHIFT (16)
#define _SCB_MAX_POOL_SIZE (1<<_SCB_SLOT_SHIFT)
#define _SCB_SLOT_MASK (_SCB_MAX_POOL_SIZE-1)

// >>structs
typedef enum {
    _SCB_CMD_NONE = 0,
    _SCB_CMD_APPLY_VIEWPORT,
    _SCB_CMD_APPLY_SCISSOR_RECT,
    _SCB_CMD_APPLY_PIPELINE,
    _SCB_CMD_APPLY_BINDINGS,
    _SCB_CMD_APPLY_UNIFORMS,
    _SCB_CMD_DRAW,
    _SCB_CMD_DRAW_EX,
    _SCB_CMD_DISPATCH,
} _scb_cmd_t;

typedef struct {
    uint32_t id;
    scb_resource_state state;
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
    uint8_t* buf;
    uint8_t* cur;
    const uint8_t* end;
    const uint8_t* cmd_max_end;
    bool overflown; // tried to encode cmd past end
    _scb_str_t label;
} _scb_cmdbuf_t;

typedef struct {
    _scb_pool_t cmdbuf_pool;
    _scb_cmdbuf_t* cmdbufs;
} _scb_pools_t;

typedef struct {
    uint32_t init_tag;
    scb_desc desc;
    _scb_pools_t pools;
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

static void _scb_log(scb_log_item log_item, uint32_t log_level, uint32_t line_nr) {
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
        _SCB_ERROR(MALLOC_FAILED);
    }
    return ptr;
}

static void* _scb_malloc_clear(size_t size) {
    void* ptr = _scb_malloc(size);
    if (ptr) {
        _scb_clear(ptr, size);
    }
    return ptr;
}

static void _scb_free(void* ptr) {
    if (_scb.desc.allocator.free_fn) {
        _scb.desc.allocator.free_fn(ptr, _scb.desc.allocator.user_data);
    } else {
        free(ptr);
    }
}

static void _scb_strcpy(_scb_str_t* dst, const char* src) {
    SOKOL_ASSERT(dst);
    if (src) {
        #if defined(_MSC_VER)
        strncpy_s(dst->buf, _SCB_STRING_SIZE, src, (_SCB_STRING_SIZE-1));
        #else
        strncpy(dst->buf, src, _SCB_STRING_SIZE);
        #endif
        dst->buf[_SCB_STRING_SIZE-1] = 0;
    } else {
        _scb_clear(dst->buf, _SCB_STRING_SIZE);
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
    pool->gen_ctrs = (uint32_t*) _scb_malloc_clear(gen_ctrs_size);
    SOKOL_ASSERT(pool->gen_ctrs);
    // it's not a bug to only reserve 'num' here
    pool->free_queue = (int*) _scb_malloc_clear(sizeof(int) * (size_t)num);
    SOKOL_ASSERT(pool->free_queue);
    // never allocate the zero-th pool item since the invalid id is 0
    for (int i = pool->size-1; i >= 1; i--) {
        pool->free_queue[pool->queue_top++] = i;
    }
}

static void _scb_pool_discard(_scb_pool_t* pool) {
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

static void _scb_setup_pools(_scb_pools_t* p, const scb_desc* desc) {
    SOKOL_ASSERT(p);
    SOKOL_ASSERT(desc);
    // note: the pools will have an additional item, since slot 0 is reserved
    SOKOL_ASSERT((desc->cmdbuf_pool_size > 0) && (desc->cmdbuf_pool_size < _SCB_MAX_POOL_SIZE));
    _scb_pool_init(&p->cmdbuf_pool, desc->cmdbuf_pool_size);
    size_t cb_pool_byte_size = sizeof(_scb_cmdbuf_t) * (size_t)p->cmdbuf_pool.size;
    p->cmdbufs = (_scb_cmdbuf_t*)_scb_malloc_clear(cb_pool_byte_size);
    SOKOL_ASSERT(p->cmdbufs);
}

static void _scb_discard_pools(_scb_pools_t* p) {
    SOKOL_ASSERT(p);
    SOKOL_ASSERT(p->cmdbufs);
    _scb_free(p->cmdbufs); p->cmdbufs = 0;
    _scb_pool_discard(&p->cmdbuf_pool);
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

// get cmdbuf pointer without id-check
static _scb_cmdbuf_t* _scb_cmdbuf_at(uint32_t cb_id) {
    SOKOL_ASSERT(SCB_INVALID_ID != cb_id);
    int slot_index = _scb_slot_index(cb_id);
    SOKOL_ASSERT((slot_index > _SCB_INVALID_SLOT_INDEX) && (slot_index < _scb.pools.cmdbuf_pool.size));
    return &_scb.pools.cmdbufs[slot_index];
}

// get cmdbuf pointer with id-check, returns 0 if no match
static _scb_cmdbuf_t* _scb_lookup_cmdbuf(uint32_t cb_id) {
    if (SCB_INVALID_ID != cb_id) {
        _scb_cmdbuf_t* cb = _scb_cmdbuf_at(cb_id);
        if (cb->slot.id == cb_id) {
            return cb;
        }
    }
    return 0;
}

// make cmdbuf handle from raw uint32_t id
static scb_cmdbuf _scb_make_cmdbuf_id(uint32_t cb_id) {
    scb_cmdbuf cb;
    cb.id = cb_id;
    return cb;
}

static scb_cmdbuf _scb_alloc_cmdbuf(void) {
    scb_cmdbuf cb_id;
    int slot_index = _scb_pool_alloc_index(&_scb.pools.cmdbuf_pool);
    if (_SCB_INVALID_SLOT_INDEX != slot_index) {
        cb_id = _scb_make_cmdbuf_id(_scb_slot_alloc(&_scb.pools.cmdbuf_pool, &_scb.pools.cmdbufs[slot_index].slot, slot_index));
    } else {
        // pool is exhausted
        _SCB_ERROR(CMDBUF_POOL_EXHAUSTED);
        cb_id = _scb_make_cmdbuf_id(SCB_INVALID_ID);
    }
    return cb_id;
}

static void _scb_dealloc_cmdbuf(_scb_cmdbuf_t* cb) {
    SOKOL_ASSERT(cb && (cb->slot.state == SCB_RESOURCESTATE_ALLOC) && (cb->slot.id != SCB_INVALID_ID));
    _scb_pool_free_index(&_scb.pools.cmdbuf_pool, _scb_slot_index(cb->slot.id));
    _scb_clear(cb, sizeof(_scb_cmdbuf_t));
}

static void _scb_init_cmdbuf(_scb_cmdbuf_t* cb, const scb_cmdbuf_desc* desc) {
    SOKOL_ASSERT(cb && (cb->slot.state == SCB_RESOURCESTATE_ALLOC));
    SOKOL_ASSERT(desc);
    SOKOL_ASSERT(desc->size > 0);
    cb->buf = _scb_malloc(desc->size);
    if (0 == cb->buf) {
        // NOTE: allocation failure already logged _scb_malloc
        cb->slot.state = SCB_RESOURCESTATE_FAILED;
        return;
    }
    cb->cur = cb->buf;
    cb->end = cb->buf + desc->size;
    _scb_strcpy(&cb->label, desc->label);
    cb->slot.state = SCB_RESOURCESTATE_VALID;
}

static void _scb_uninit_cmdbuf(_scb_cmdbuf_t* cb) {
    SOKOL_ASSERT(cb && ((cb->slot.state == SCB_RESOURCESTATE_VALID) || (cb->slot.state == SCB_RESOURCESTATE_FAILED)));
    if (cb->buf) {
        _scb_free(cb->buf);
        cb->buf = 0;
        cb->cur = 0;
        cb->end = 0;
    }
    cb->slot.state = SCB_RESOURCESTATE_ALLOC;
}

static scb_desc _scb_desc_defaults(const scb_desc* desc) {
    SOKOL_ASSERT(desc);
    scb_desc res = *desc;
    res.cmdbuf_pool_size = _scb_def(res.cmdbuf_pool_size, _SCB_DEFAULT_CMDBUF_POOL_SIZE);
    return res;
}

static scb_cmdbuf_desc _scb_cmdbuf_desc_defaults(const scb_cmdbuf_desc* desc) {
    SOKOL_ASSERT(desc);
    scb_cmdbuf_desc res = *desc;
    res.size = _scb_def(res.size, _SCB_DEFAULT_CMDBUF_SIZE);
    return res;
}

static void _scb_discard_all_resources(void) {
    for (int i = 1; i < _scb.pools.cmdbuf_pool.size; i++) {
        const scb_resource_state state = _scb.pools.cmdbufs[i].slot.state;
        if ((state == SCB_RESOURCESTATE_VALID) || (state == SCB_RESOURCESTATE_FAILED)) {
            _scb_uninit_cmdbuf(&_scb.pools.cmdbufs[i]);
        }
    }
}

static bool _scb_cmdbuf_valid(_scb_cmdbuf_t* cb) {
    return cb && (cb->slot.state == SCB_RESOURCESTATE_VALID);
}

static void _scb_enc_u8(_scb_cmdbuf_t* cb, uint8_t val) {
    SOKOL_ASSERT((cb->cur + sizeof(uint8_t)) <= cb->end);
    *cb->cur++ = val;
}

static void _scb_enc_bool(_scb_cmdbuf_t* cb, bool val) {
    SOKOL_ASSERT((cb->cur + sizeof(uint8_t)) <= cb->end);
    *cb->cur++ = (uint8_t)val;
}

static const uint8_t* _scb_dec_bool(const uint8_t* ptr, bool* out_val) {
    *out_val = (bool)(*ptr++);
    return ptr;
}

static void _scb_enc_i32(_scb_cmdbuf_t* cb, int32_t val) {
    SOKOL_ASSERT((cb->cur + sizeof(int32_t)) <= cb->end);
    cb->cur[0] = (uint8_t)val;
    cb->cur[1] = (uint8_t)(val >> 8);
    cb->cur[2] = (uint8_t)(val >> 16);
    cb->cur[3] = (uint8_t)(val >> 24);
    cb->cur += sizeof(int32_t);
}

static const uint8_t* _scb_dec_i32(const uint8_t* ptr, int32_t* out_val) {
    *out_val = (int32_t)((uint32_t)ptr[0]
        | ((uint32_t)ptr[1]<<8)
        | ((uint32_t)ptr[2]<<16)
        | ((uint32_t)ptr[3]<<24));
    return ptr + sizeof(int32_t);
}

static void _scb_enc_u32(_scb_cmdbuf_t* cb, uint32_t val) {
    SOKOL_ASSERT((cb->cur + sizeof(uint32_t)) <= cb->end);
    cb->cur[0] = (uint8_t)val;
    cb->cur[1] = (uint8_t)(val >> 8);
    cb->cur[2] = (uint8_t)(val >> 16);
    cb->cur[3] = (uint8_t)(val >> 24);
    cb->cur += sizeof(uint32_t);
}

static const uint8_t* _scb_dec_u32(const uint8_t* ptr, uint32_t* out_val) {
    *out_val = (uint32_t)ptr[0]
        | ((uint32_t)ptr[1]<<8)
        | ((uint32_t)ptr[2]<<16)
        | ((uint32_t)ptr[3]<<24);
    return ptr + sizeof(uint32_t);
}

static void _scb_enc_u64(_scb_cmdbuf_t* cb, uint64_t val) {
    SOKOL_ASSERT((cb->cur + sizeof(uint64_t)) <= cb->end);
    cb->cur[0] = (uint8_t)val;
    cb->cur[1] = (uint8_t)(val >> 8);
    cb->cur[2] = (uint8_t)(val >> 16);
    cb->cur[3] = (uint8_t)(val >> 24);
    cb->cur[4] = (uint8_t)(val >> 32);
    cb->cur[5] = (uint8_t)(val >> 40);
    cb->cur[6] = (uint8_t)(val >> 48);
    cb->cur[7] = (uint8_t)(val >> 56);
    cb->cur += sizeof(uint64_t);
}

static const uint8_t* _scb_dec_u64(const uint8_t* ptr, uint64_t* out_val) {
    *out_val = (uint64_t)ptr[0]
        | ((uint64_t)ptr[1]<<8)
        | ((uint64_t)ptr[2]<<16)
        | ((uint64_t)ptr[3]<<24)
        | ((uint64_t)ptr[4]<<32)
        | ((uint64_t)ptr[5]<<40)
        | ((uint64_t)ptr[6]<<48)
        | ((uint64_t)ptr[7]<<56);
    return ptr + sizeof(uint64_t);
}

static void _scb_enc_blob(_scb_cmdbuf_t* cb, const void* ptr, size_t size) {
    SOKOL_ASSERT((cb->cur + size) <= cb->end);
    memcpy(cb->cur, ptr, size);
    cb->cur += size;
}

static const uint8_t* _scb_ptr_align4(const uint8_t* ptr) {
    const uintptr_t align_minus_one = (4 - 1);
    const uintptr_t mask = ~align_minus_one;
    return (uint8_t*)(((uintptr_t)ptr + align_minus_one) & mask);
}

static void _scb_enc_align4(_scb_cmdbuf_t* cb) {
    uint8_t* ptr = (uint8_t*)_scb_ptr_align4(cb->cur);
    SOKOL_ASSERT((ptr >= cb->cur) && (ptr <= cb->end));
    for (; cb->cur < ptr; cb->cur++) {
        *cb->cur = 0;
    }
}

static const uint8_t* _scb_dec_align4(const uint8_t* ptr) {
    return _scb_ptr_align4(ptr);
}

static bool _scb_enc_cmd(_scb_cmdbuf_t* cb, _scb_cmd_t cmd, size_t max_payload_size) {
    SOKOL_ASSERT(cb->cmd_max_end == 0);
    if (cb->overflown) {
        return false;
    }
    const size_t max_cmd_payload_size = max_payload_size + 1;
    if ((cb->cur + max_cmd_payload_size) > cb->end) {
        cb->overflown = true;
        _SCB_ERROR(CMDBUF_OVERFLOW);
        return false;
    }
    cb->cmd_max_end = cb->cur + max_cmd_payload_size;
    _scb_enc_u8(cb, (uint8_t)cmd);
    return true;
}

static const uint8_t* _scb_dec_cmd(const uint8_t* ptr, _scb_cmd_t* out_cmd) {
    *out_cmd = (_scb_cmd_t)(*ptr++);
    return ptr;
}

static void _scb_enc_end(_scb_cmdbuf_t* cb) {
    SOKOL_ASSERT(cb->cmd_max_end && (cb->cur <= cb->cmd_max_end));
    cb->cmd_max_end = 0;
}

static void _scb_enc_apply_viewport(_scb_cmdbuf_t* cb, int x, int y, int width, int height, bool origin_top_left) {
    const size_t payload_size = 4 * sizeof(int32_t) + sizeof(uint8_t);
    if (_scb_enc_cmd(cb, _SCB_CMD_APPLY_VIEWPORT, payload_size)) {
        _scb_enc_i32(cb, x);
        _scb_enc_i32(cb, y);
        _scb_enc_i32(cb, width);
        _scb_enc_i32(cb, height);
        _scb_enc_bool(cb, origin_top_left);
        _scb_enc_end(cb);
    }
}

static const uint8_t* _scb_dec_apply_viewport(const uint8_t* ptr) {
    int x, y, width, height;
    bool origin_top_left;
    ptr = _scb_dec_i32(ptr, &x);
    ptr = _scb_dec_i32(ptr, &y);
    ptr = _scb_dec_i32(ptr, &width);
    ptr = _scb_dec_i32(ptr, &height);
    ptr = _scb_dec_bool(ptr, &origin_top_left);
    sg_apply_viewport(x, y, width, height, origin_top_left);
    return ptr;
}

static void _scb_enc_apply_scissor_rect(_scb_cmdbuf_t* cb, int x, int y, int width, int height, bool origin_top_left) {
    const size_t payload_size = 4 * sizeof(int32_t) + sizeof(uint8_t);
    if (_scb_enc_cmd(cb, _SCB_CMD_APPLY_SCISSOR_RECT, payload_size)) {
        _scb_enc_i32(cb, x);
        _scb_enc_i32(cb, y);
        _scb_enc_i32(cb, width);
        _scb_enc_i32(cb, height);
        _scb_enc_bool(cb, origin_top_left);
        _scb_enc_end(cb);
    }
}

static const uint8_t* _scb_dec_apply_scissor_rect(const uint8_t* ptr) {
    int x, y, width, height;
    bool origin_top_left;
    ptr = _scb_dec_i32(ptr, &x);
    ptr = _scb_dec_i32(ptr, &y);
    ptr = _scb_dec_i32(ptr, &width);
    ptr = _scb_dec_i32(ptr, &height);
    ptr = _scb_dec_bool(ptr, &origin_top_left);
    sg_apply_scissor_rect(x, y, width, height, origin_top_left);
    return ptr;
}

static void _scb_enc_apply_pipeline(_scb_cmdbuf_t* cb, uint32_t pip_id) {
    const size_t payload_size = sizeof(uint32_t);
    if (_scb_enc_cmd(cb, _SCB_CMD_APPLY_PIPELINE, payload_size)) {
        _scb_enc_u32(cb, pip_id);
        _scb_enc_end(cb);
    }
}

static const uint8_t* _scb_dec_apply_pipeline(const uint8_t* ptr) {
    sg_pipeline pip;
    ptr = _scb_dec_u32(ptr, &pip.id);
    sg_apply_pipeline(pip);
    return ptr;
}

static void _scb_enc_draw(_scb_cmdbuf_t* cb, int base_element, int num_elements, int num_instances) {
    const size_t payload_size = 3 * sizeof(int32_t);
    if (_scb_enc_cmd(cb, _SCB_CMD_DRAW, payload_size)) {
        _scb_enc_i32(cb, base_element);
        _scb_enc_i32(cb, num_elements);
        _scb_enc_i32(cb, num_instances);
        _scb_enc_end(cb);
    }
}

static const uint8_t* _scb_dec_draw(const uint8_t* ptr) {
    int base_element, num_elements, num_instances;
    ptr = _scb_dec_i32(ptr, &base_element);
    ptr = _scb_dec_i32(ptr, &num_elements);
    ptr = _scb_dec_i32(ptr, &num_instances);
    sg_draw(base_element, num_elements, num_instances);
    return ptr;
}

static void _scb_enc_draw_ex(_scb_cmdbuf_t* cb, int base_element, int num_elements, int num_instances, int base_vertex, int base_instance) {
    const size_t payload_size = 5 * sizeof(int32_t);
    if (_scb_enc_cmd(cb, _SCB_CMD_DRAW_EX, payload_size)) {
        _scb_enc_i32(cb, base_element);
        _scb_enc_i32(cb, num_elements);
        _scb_enc_i32(cb, num_instances);
        _scb_enc_i32(cb, base_vertex);
        _scb_enc_i32(cb, base_instance);
        _scb_enc_end(cb);
    }
}

static const uint8_t* _scb_dec_draw_ex(const uint8_t* ptr) {
    int base_element, num_elements, num_instances, base_vertex, base_instance;
    ptr = _scb_dec_i32(ptr, &base_element);
    ptr = _scb_dec_i32(ptr, &num_elements);
    ptr = _scb_dec_i32(ptr, &num_instances);
    ptr = _scb_dec_i32(ptr, &base_vertex);
    ptr = _scb_dec_i32(ptr, &base_instance);
    sg_draw_ex(base_element, num_elements, num_instances, base_vertex, base_instance);
    return ptr;
}

static void _scb_enc_dispatch(_scb_cmdbuf_t* cb, int num_groups_x, int num_groups_y, int num_groups_z) {
    const size_t payload_size = 3 * sizeof(int32_t);
    if (_scb_enc_cmd(cb, _SCB_CMD_DISPATCH, payload_size)) {
        _scb_enc_i32(cb, num_groups_x);
        _scb_enc_i32(cb, num_groups_y);
        _scb_enc_i32(cb, num_groups_z);
        _scb_enc_end(cb);
    }
}

static const uint8_t* _scb_dec_dispatch(const uint8_t* ptr) {
    int num_groups_x, num_groups_y, num_groups_z;
    ptr = _scb_dec_i32(ptr, &num_groups_x);
    ptr = _scb_dec_i32(ptr, &num_groups_y);
    ptr = _scb_dec_i32(ptr, &num_groups_z);
    sg_dispatch(num_groups_x, num_groups_y, num_groups_z);
    return ptr;
}

static void _scb_enc_apply_bindings(_scb_cmdbuf_t* cb, const sg_bindings* bindings) {
    // create an occupied slot mask and compute payload size
    size_t payload_size = 0;
    uint64_t slot_mask = 0;
    const int vb_start = 0;
    const int ib_start = vb_start + SG_MAX_VERTEXBUFFER_BINDSLOTS;
    const int view_start = ib_start + 1;
    const int smp_start = view_start + SG_MAX_VIEW_BINDSLOTS;
    SOKOL_ASSERT((smp_start + SG_MAX_SAMPLER_BINDSLOTS) <= 64);
    // vertex buffer handles and offsets
    for (int i = 0; i < SG_MAX_VERTEXBUFFER_BINDSLOTS; i++) {
        if (bindings->vertex_buffers[i].id != SG_INVALID_ID) {
            slot_mask |= (1ULL << (i + vb_start));
            payload_size += sizeof(uint32_t) + sizeof(int32_t);
        }
    }
    // index buffer handle and offset
    if (bindings->index_buffer.id != SG_INVALID_ID) {
        slot_mask |= (1ULL << ib_start);
        payload_size += sizeof(uint32_t) + sizeof(int32_t);
    }
    // view handles
    for (int i = 0; i < SG_MAX_VIEW_BINDSLOTS; i++) {
        if (bindings->views[i].id != SG_INVALID_ID) {
            slot_mask |= (1ULL << (view_start + i));
            payload_size += sizeof(uint32_t);
        }
    }
    // sampler handles
    for (int i = 0; i < SG_MAX_SAMPLER_BINDSLOTS; i++) {
        if (bindings->samplers[i].id != SG_INVALID_ID) {
            slot_mask |= (1ULL << (smp_start + i));
            payload_size += sizeof(uint32_t);
        }
    }
    // the slot_mask is part of the payload
    payload_size += sizeof(slot_mask);
    if (_scb_enc_cmd(cb, _SCB_CMD_APPLY_BINDINGS, payload_size)) {
        _scb_enc_u64(cb, slot_mask);
        for (int i = 0; i < SG_MAX_VERTEXBUFFER_BINDSLOTS; i++) {
            if (bindings->vertex_buffers[i].id != SG_INVALID_ID) {
                _scb_enc_u32(cb, bindings->vertex_buffers[i].id);
                _scb_enc_i32(cb, bindings->vertex_buffer_offsets[i]);
            }
        }
        if (bindings->index_buffer.id != SG_INVALID_ID) {
            _scb_enc_u32(cb, bindings->index_buffer.id);
            _scb_enc_i32(cb, bindings->index_buffer_offset);
        }
        for (int i = 0; i < SG_MAX_VIEW_BINDSLOTS; i++) {
            if (bindings->views[i].id != SG_INVALID_ID) {
                _scb_enc_u32(cb, bindings->views[i].id);
            }
        }
        for (int i = 0; i < SG_MAX_SAMPLER_BINDSLOTS; i++) {
            if (bindings->samplers[i].id != SG_INVALID_ID) {
                _scb_enc_u32(cb, bindings->samplers[i].id);
            }
        }
        _scb_enc_end(cb);
    }
}

static const uint8_t* _scb_dec_apply_bindings(const uint8_t* ptr) {
    const int vb_start = 0;
    const int ib_start = vb_start + SG_MAX_VERTEXBUFFER_BINDSLOTS;
    const int view_start = ib_start + 1;
    const int smp_start = view_start + SG_MAX_VIEW_BINDSLOTS;
    uint64_t slot_mask;
    ptr = _scb_dec_u64(ptr, &slot_mask);
    sg_bindings bnd;
    _scb_clear(&bnd, sizeof(bnd));
    for (int i = 0; i < SG_MAX_VERTEXBUFFER_BINDSLOTS; i++) {
        if (slot_mask & (1ULL << (i + vb_start))) {
            ptr = _scb_dec_u32(ptr, &bnd.vertex_buffers[i].id);
            ptr = _scb_dec_i32(ptr, &bnd.vertex_buffer_offsets[i]);
        }
    }
    if (slot_mask & (1ULL << ib_start)) {
        ptr = _scb_dec_u32(ptr, &bnd.index_buffer.id);
        ptr = _scb_dec_i32(ptr, &bnd.index_buffer_offset);
    }
    for (int i = 0; i < SG_MAX_VIEW_BINDSLOTS; i++) {
        if (slot_mask & (1ULL << (i + view_start))) {
            ptr = _scb_dec_u32(ptr, &bnd.views[i].id);
        }
    }
    for (int i = 0; i < SG_MAX_SAMPLER_BINDSLOTS; i++) {
        if (slot_mask & (1ULL << (i + smp_start))) {
            ptr = _scb_dec_u32(ptr, &bnd.samplers[i].id);
        }
    }
    sg_apply_bindings(&bnd);
    return ptr;
}

static void _scb_enc_apply_uniforms(_scb_cmdbuf_t* cb, int ub_slot, const sg_range* data) {
    SOKOL_ASSERT(data->size <= UINT32_MAX);
    SOKOL_ASSERT((ub_slot >= 0) && (ub_slot < SG_MAX_UNIFORMBLOCK_BINDSLOTS));
    // NOTE: add max alignment gap of 3 bytes to max payload size
    const size_t max_payload_size = sizeof(int32_t) + sizeof(uint32_t) + data->size + 3;
    if (_scb_enc_cmd(cb, _SCB_CMD_APPLY_UNIFORMS, max_payload_size)) {
        _scb_enc_i32(cb, ub_slot);
        _scb_enc_u32(cb, (uint32_t)data->size);
        _scb_enc_align4(cb);
        _scb_enc_blob(cb, data->ptr, data->size);
        _scb_enc_end(cb);
    }
}

static const uint8_t* _scb_dec_apply_uniforms(const uint8_t* ptr) {
    int ub_slot;
    uint32_t size_u32;
    ptr = _scb_dec_i32(ptr, &ub_slot);
    SOKOL_ASSERT((ub_slot >= 0) && (ub_slot < SG_MAX_UNIFORMBLOCK_BINDSLOTS));
    ptr = _scb_dec_u32(ptr, &size_u32);
    ptr = _scb_dec_align4(ptr);
    sg_range data;
    _scb_clear(&data, sizeof(data));
    data.ptr = ptr;
    data.size = size_u32;
    sg_apply_uniforms(ub_slot, &data);
    return ptr + size_u32;
}

static void _scb_rewind(_scb_cmdbuf_t* cb) {
    SOKOL_ASSERT(cb->cur);
    SOKOL_ASSERT(cb->buf);
    SOKOL_ASSERT(cb->cur >= cb->buf);
    cb->cur = cb->buf;
    cb->overflown = false;
}

// >>public
SOKOL_API_IMPL void scb_setup(const scb_desc* desc) {
    SOKOL_ASSERT(desc);
    SOKOL_ASSERT((desc->allocator.alloc_fn && desc->allocator.free_fn) || (!desc->allocator.alloc_fn && !desc->allocator.free_fn));
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

SOKOL_API_IMPL scb_cmdbuf scb_make_cmdbuf(const scb_cmdbuf_desc* desc) {
    SOKOL_ASSERT(_SCB_INIT_TAG == _scb.init_tag);
    SOKOL_ASSERT(desc);
    scb_cmdbuf_desc desc_def = _scb_cmdbuf_desc_defaults(desc);
    scb_cmdbuf cb_id = _scb_alloc_cmdbuf();
    if (cb_id.id != SCB_INVALID_ID) {
        _scb_cmdbuf_t* cb = _scb_cmdbuf_at(cb_id.id);
        SOKOL_ASSERT(cb && (cb->slot.state == SCB_RESOURCESTATE_ALLOC));
        _scb_init_cmdbuf(cb, &desc_def);
        SOKOL_ASSERT((cb->slot.state == SCB_RESOURCESTATE_VALID) || (cb->slot.state == SCB_RESOURCESTATE_FAILED));
    }
    return cb_id;
}

SOKOL_API_IMPL void scb_destroy_cmdbuf(scb_cmdbuf cb_id) {
    SOKOL_ASSERT(_SCB_INIT_TAG == _scb.init_tag);
    _scb_cmdbuf_t* cb = _scb_lookup_cmdbuf(cb_id.id);
    if (cb) {
        if ((cb->slot.state == SCB_RESOURCESTATE_VALID) || (cb->slot.state == SCB_RESOURCESTATE_FAILED)) {
            _scb_uninit_cmdbuf(cb);
            SOKOL_ASSERT(cb->slot.state == SCB_RESOURCESTATE_ALLOC);
        }
        if (cb->slot.state == SCB_RESOURCESTATE_ALLOC) {
            _scb_dealloc_cmdbuf(cb);
            SOKOL_ASSERT(cb->slot.state == SCB_RESOURCESTATE_INITIAL);
        }
    }
}

SOKOL_API_IMPL scb_resource_state scb_query_cmdbuf_state(scb_cmdbuf cb_id) {
    SOKOL_ASSERT(_SCB_INIT_TAG == _scb.init_tag);
    _scb_cmdbuf_t* cb = _scb_lookup_cmdbuf(cb_id.id);
    return cb ? cb->slot.state : SCB_RESOURCESTATE_INVALID;
}

SOKOL_API_IMPL scb_cmdbuf_info scb_query_cmdbuf_info(scb_cmdbuf cb_id) {
    SOKOL_ASSERT(_SCB_INIT_TAG == _scb.init_tag);
    _scb_cmdbuf_t* cb = _scb_lookup_cmdbuf(cb_id.id);
    scb_cmdbuf_info info;
    _scb_clear(&info, sizeof(info));
    if (_scb_cmdbuf_valid(cb)) {
        SOKOL_ASSERT(cb->buf && cb->cur && cb->end);
        SOKOL_ASSERT((cb->cur >= cb->buf) && (cb->cur <= cb->end));
        SOKOL_ASSERT(cb->end > cb->buf);
        info.size = (size_t)(cb->end - cb->buf);
        info.remaining = (size_t)(cb->end - cb->cur);
        info.overflown = cb->overflown;
    }
    return info;
}

SOKOL_API_IMPL void scb_apply_viewport(scb_cmdbuf cb_id, int x, int y, int width, int height, bool origin_top_left) {
    SOKOL_ASSERT(_SCB_INIT_TAG == _scb.init_tag);
    _scb_cmdbuf_t* cb = _scb_lookup_cmdbuf(cb_id.id);
    if (!_scb_cmdbuf_valid(cb)) {
        _SCB_ERROR(CMDBUF_NOT_VALID);
        return;
    }
    _scb_enc_apply_viewport(cb, x, y, width, height, origin_top_left);
}

SOKOL_API_IMPL void scb_apply_viewportf(scb_cmdbuf cb_id, float x, float y, float width, float height, bool origin_top_left) {
    SOKOL_ASSERT(_SCB_INIT_TAG == _scb.init_tag);
    _scb_cmdbuf_t* cb = _scb_lookup_cmdbuf(cb_id.id);
    if (!_scb_cmdbuf_valid(cb)) {
        _SCB_ERROR(CMDBUF_NOT_VALID);
        return;
    }
    // NOTE: truncating the floats is intended and matched sokol-gfx behaviour
    _scb_enc_apply_viewport(cb, (int)x, (int)y, (int)width, (int)height, origin_top_left);
}

SOKOL_API_IMPL void scb_apply_scissor_rect(scb_cmdbuf cb_id, int x, int y, int width, int height, bool origin_top_left) {
    SOKOL_ASSERT(_SCB_INIT_TAG == _scb.init_tag);
    _scb_cmdbuf_t* cb = _scb_lookup_cmdbuf(cb_id.id);
    if (!_scb_cmdbuf_valid(cb)) {
        _SCB_ERROR(CMDBUF_NOT_VALID);
        return;
    }
    _scb_enc_apply_scissor_rect(cb, x, y, width, height, origin_top_left);
}

SOKOL_API_IMPL void scb_apply_scissor_rectf(scb_cmdbuf cb_id, float x, float y, float width, float height, bool origin_top_left) {
    SOKOL_ASSERT(_SCB_INIT_TAG == _scb.init_tag);
    _scb_cmdbuf_t* cb = _scb_lookup_cmdbuf(cb_id.id);
    if (!_scb_cmdbuf_valid(cb)) {
        _SCB_ERROR(CMDBUF_NOT_VALID);
        return;
    }
    // NOTE: truncating the floats is intended and matched sokol-gfx behaviour
    _scb_enc_apply_scissor_rect(cb, (int)x, (int)y, (int)width, (int)height, origin_top_left);
}

SOKOL_API_IMPL void scb_apply_pipeline(scb_cmdbuf cb_id, sg_pipeline pip) {
    SOKOL_ASSERT(_SCB_INIT_TAG == _scb.init_tag);
    _scb_cmdbuf_t* cb = _scb_lookup_cmdbuf(cb_id.id);
    if (!_scb_cmdbuf_valid(cb)) {
        _SCB_ERROR(CMDBUF_NOT_VALID);
        return;
    }
    _scb_enc_apply_pipeline(cb, pip.id);
}

SOKOL_API_IMPL void scb_apply_bindings(scb_cmdbuf cb_id, const sg_bindings* bindings) {
    SOKOL_ASSERT(_SCB_INIT_TAG == _scb.init_tag);
    SOKOL_ASSERT(bindings);
    _scb_cmdbuf_t* cb = _scb_lookup_cmdbuf(cb_id.id);
    if (!_scb_cmdbuf_valid(cb)) {
        _SCB_ERROR(CMDBUF_NOT_VALID);
        return;
    }
    _scb_enc_apply_bindings(cb, bindings);
}

SOKOL_API_IMPL void scb_apply_uniforms(scb_cmdbuf cb_id, int ub_slot, const sg_range* data) {
    SOKOL_ASSERT(_SCB_INIT_TAG == _scb.init_tag);
    SOKOL_ASSERT(data && data->ptr && (data->size > 0));
    _scb_cmdbuf_t* cb = _scb_lookup_cmdbuf(cb_id.id);
    if (!_scb_cmdbuf_valid(cb)) {
        _SCB_ERROR(CMDBUF_NOT_VALID);
        return;
    }
    _scb_enc_apply_uniforms(cb, ub_slot, data);
}

SOKOL_API_IMPL void scb_draw(scb_cmdbuf cb_id, int base_element, int num_elements, int num_instances) {
    SOKOL_ASSERT(_SCB_INIT_TAG == _scb.init_tag);
    _scb_cmdbuf_t* cb = _scb_lookup_cmdbuf(cb_id.id);
    if (!_scb_cmdbuf_valid(cb)) {
        _SCB_ERROR(CMDBUF_NOT_VALID);
        return;
    }
    _scb_enc_draw(cb, base_element, num_elements, num_instances);
}

SOKOL_API_IMPL void scb_draw_ex(scb_cmdbuf cb_id, int base_element, int num_elements, int num_instances, int base_vertex, int base_instance) {
    SOKOL_ASSERT(_SCB_INIT_TAG == _scb.init_tag);
    _scb_cmdbuf_t* cb = _scb_lookup_cmdbuf(cb_id.id);
    if (!_scb_cmdbuf_valid(cb)) {
        _SCB_ERROR(CMDBUF_NOT_VALID);
        return;
    }
    _scb_enc_draw_ex(cb, base_element, num_elements, num_instances, base_vertex, base_instance);
}

SOKOL_API_IMPL void scb_dispatch(scb_cmdbuf cb_id, int num_groups_x, int num_groups_y, int num_groups_z) {
    SOKOL_ASSERT(_SCB_INIT_TAG == _scb.init_tag);
    _scb_cmdbuf_t* cb = _scb_lookup_cmdbuf(cb_id.id);
    if (!_scb_cmdbuf_valid(cb)) {
        _SCB_ERROR(CMDBUF_NOT_VALID);
        return;
    }
    _scb_enc_dispatch(cb, num_groups_x, num_groups_y, num_groups_z);
}

SOKOL_API_IMPL void scb_submit(scb_cmdbuf cb_id) {
    SOKOL_ASSERT(_SCB_INIT_TAG == _scb.init_tag);
    _scb_cmdbuf_t* cb = _scb_lookup_cmdbuf(cb_id.id);
    if (!_scb_cmdbuf_valid(cb)) {
        _SCB_ERROR(CMDBUF_NOT_VALID);
        return;
    }
    if (cb->overflown) {
        _SCB_ERROR(SUBMIT_CMDBUF_OVERFLOWN);
        _scb_rewind(cb);
        return;
    }
    SOKOL_ASSERT(cb->buf);
    SOKOL_ASSERT(cb->cur);
    SOKOL_ASSERT(cb->buf <= cb->cur);
    const uint8_t* ptr = cb->buf;
    bool has_label = cb->label.buf[0] != 0;
    if (has_label) {
        sg_push_debug_group(cb->label.buf);
    }
    while (ptr < cb->cur) {
        _scb_cmd_t cmd = _SCB_CMD_NONE;
        ptr = _scb_dec_cmd(ptr, &cmd);
        switch (cmd) {
            case _SCB_CMD_APPLY_VIEWPORT:
                ptr = _scb_dec_apply_viewport(ptr);
                break;
            case _SCB_CMD_APPLY_SCISSOR_RECT:
                ptr = _scb_dec_apply_scissor_rect(ptr);
                break;
            case _SCB_CMD_APPLY_PIPELINE:
                ptr = _scb_dec_apply_pipeline(ptr);
                break;
            case _SCB_CMD_APPLY_BINDINGS:
                ptr = _scb_dec_apply_bindings(ptr);
                break;
            case _SCB_CMD_APPLY_UNIFORMS:
                ptr = _scb_dec_apply_uniforms(ptr);
                break;
            case _SCB_CMD_DRAW:
                ptr = _scb_dec_draw(ptr);
                break;
            case _SCB_CMD_DRAW_EX:
                ptr = _scb_dec_draw_ex(ptr);
                break;
            case _SCB_CMD_DISPATCH:
                ptr = _scb_dec_dispatch(ptr);
                break;
            default:
                _SCB_ERROR(SUBMIT_INVALID_COMMAND);
                // break loop
                ptr = cb->cur;
                break;
        }
    }
    _scb_rewind(cb);
    if (has_label) {
        sg_pop_debug_group();
    }
}

SOKOL_API_IMPL void scb_reset(scb_cmdbuf cb_id) {
    SOKOL_ASSERT(_SCB_INIT_TAG == _scb.init_tag);
    _scb_cmdbuf_t* cb = _scb_lookup_cmdbuf(cb_id.id);
    if (!_scb_cmdbuf_valid(cb)) {
        _SCB_ERROR(CMDBUF_NOT_VALID);
        return;
    }
    _scb_rewind(cb);
}
#endif // SOKOL_CMDBUF_IMPL
