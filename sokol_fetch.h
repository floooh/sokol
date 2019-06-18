#pragma once
/*
    sokol_fetch.h -- asynchronous data loading

    Project URL: https://github.com/floooh/sokol

    Do this:
        #define SOKOL_IMPL
    before you include this file in *one* C or C++ file to create the
    implementation.

    Optionally provide the following defines with your own implementations:

    SOKOL_ASSERT(c)             - your own assert macro (default: assert(c))
    SOKOL_MALLOC(s)             - your own malloc function (default: malloc(s))
    SOKOL_FREE(p)               - your own free function (default: free(p))
    SOKOL_LOG(msg)              - your own logging function (default: puts(msg))
    SOKOL_UNREACHABLE()         - a guard macro for unreachable code (default: assert(false))
    SOKOL_API_DECL              - public function declaration prefix (default: extern)
    SOKOL_API_IMPL              - public function implementation prefix (default: -)
    SFETCH_MAX_PATH             - max length of UTF-8 filesystem path / URL (default: 512 bytes)
    SFETCH_MAX_USERDATA_UINT64  - max size of embedded userdata in number of uint64_t, userdata
                                  will be copied into an 8-byte aligned memory region associated
                                  with each in-flight request, default value is 16 (== 128 bytes)
    SFETCH_MAX_CHANNELS         - max number of IO channels (default is 16, also see sfetch_desc_t.num_channels)

    If sokol_fetch.h is compiled as a DLL, define the following before
    including the declaration or implementation:

    SOKOL_DLL

    On Windows, SOKOL_DLL will define SOKOL_API_DECL as __declspec(dllexport)
    or __declspec(dllimport) as needed.


    zlib/libpng license

    Copyright (c) 2019 Andre Weissflog

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
#define SOKOL_FETCH_INCLUDED (1)
#include <stdint.h>
#include <stdbool.h>

#ifndef SOKOL_API_DECL
#if defined(_WIN32) && defined(SOKOL_DLL) && defined(SOKOL_IMPL)
#define SOKOL_API_DECL __declspec(dllexport)
#elif defined(_WIN32) && defined(SOKOL_DLL)
#define SOKOL_API_DECL __declspec(dllimport)
#else
#define SOKOL_API_DECL extern
#endif
#endif

#ifdef __cplusplus
extern "C" {
#endif

/* configuration values for sfetch_setup() */
typedef struct sfetch_desc_t {
    uint32_t _start_canary;
    uint32_t num_channels;          /* number of channels to fetch requests in parallel, default is 1 */
    uint32_t max_requests;          /* max number of requests 'in flight' */
    uint32_t timeout_num_frames;    /* number of frames until request is discarded in polling mode if
                                       user code doesn't respond to a request state change */
    uint32_t _end_canary;
} sfetch_desc_t;

/* a request handle to identify an active fetch request */
typedef struct sfetch_handle_t { uint32_t id; } sfetch_handle_t;

/* a request goes through the following states, ping-ponging between IO and user thread */
typedef enum sfetch_state_t {
    SFETCH_STATE_INITIAL = 0,   /* user thread: request has just been initialized */
    SFETCH_STATE_ALLOCATED,     /* user thread: request has been allocated from internal pool */
    SFETCH_STATE_OPENING,       /* IO thread: waiting to be opened */
    SFETCH_STATE_OPENED,        /* user thread: opened, size and status valid, waiting for buffer */
    SFETCH_STATE_FETCHING,      /* IO thread: waiting for data to be fetched */
    SFETCH_STATE_FETCHED,       /* user thread: processing fetched data */
    SFETCH_STATE_CLOSING,       /* IO thread: waiting to be closed */
    SFETCH_STATE_CLOSED,        /* user thread: user-side cleanup */

    _SFETCH_STATE_NUM,
} sfetch_state_t;

typedef struct sfetch_buffer_t {
    uint8_t* ptr;
    uint64_t num_bytes;
} sfetch_buffer_t;

typedef struct sfetch_request_t {
    uint32_t _start_canary;
    uint32_t channel;
    const char* path;
    void (*state_changed_cb)(sfetch_handle_t, sfetch_state_t);  /* optional, can also use a polling model */
    sfetch_buffer_t buffer;     /* it's optional to provide a buffer upfront, can also happen in OPENED state */
    void* user_data;            /* optional user-data will be memcpy'ed into a memory region set aside for each request */
    uint32_t user_data_size;
    uint32_t _end_canary;
} sfetch_request_t;

/* setup sokol-fetch (FIXME: allow per-thread contexts?) */
SOKOL_API_DECL void sfetch_setup(const sfetch_desc_t* desc);
/* discard a sokol-fetch context */
SOKOL_API_DECL void sfetch_shutdown(void);
/* return true if sokol-fetch has been setup */
SOKOL_API_DECL bool sfetch_valid(void);
/* get the desc struct that was passed to sfetch_setup() */
SOKOL_API_DECL sfetch_desc_t sfetch_desc(void);
/* return the max userdata size in number of bytes (SFETCH_MAX_USERDATA_UINT64 * sizeof(uint64_t)) */
SOKOL_API_DECL int sfetch_max_userdata_bytes(void);
/* return the value of the SFETCH_MAX_PATH implementation config value */
SOKOL_API_DECL int sfetch_max_path(void);

/* send a fetch-request */
SOKOL_API_DECL sfetch_handle_t sfetch_send(const sfetch_request_t* request);
/* do per-frame work, moves requests into and out of IO threads, and invoces callbacks */
SOKOL_API_DECL void sfetch_dowork(void);

/* get pointer to user data (fetch request must be in OPENED, FETCHED, CLOSED state */
SOKOL_API_DECL void* sfetch_user_data(sfetch_handle_t req);
/* get the path of a request */
SOKOL_API_DECL const char* sfetch_path(sfetch_handle_t req);
/* set the data buffer associated with a request in OPENED state */
SOKOL_API_DECL void sfetch_set_buffer(sfetch_handle_t req, const sfetch_buffer_t* buffer);
/* get the buffer currently associated with a request */
SOKOL_API_DECL sfetch_buffer_t sfetch_buffer(sfetch_handle_t req);
/* get the fetched buffer content of a request in FETCHED state */
SOKOL_API_DECL sfetch_buffer_t sfetch_content(sfetch_handle_t req);

/* polling API: check if a request has changed state and must be processed by user code */
SOKOL_API_DECL bool sfetch_state_changed(sfetch_handle_t req);
/* get the current state of a request */
SOKOL_API_DECL sfetch_state_t sfetch_state(sfetch_handle_t req);
/* polling API: advance request to next state (returns control over request to sokol-fetch) */
SOKOL_API_DECL void sfetch_advance_state(sfetch_handle_t req);

#ifdef __cplusplus
} /* extern "C" */
#endif

/*--- IMPLEMENTATION ---------------------------------------------------------*/
#ifdef SOKOL_IMPL
#define SOKOL_FETCH_IMPL_INCLUDED (1)
#include <string.h> /* memset, memcpy */

#ifndef SFETCH_MAX_PATH
#define SFETCH_MAX_PATH (512)
#endif
#ifndef SFETCH_MAX_USERDATA_UINT64
#define SFETCH_MAX_USERDATA_UINT64 (16)
#endif
#ifndef SFETCH_MAX_CHANNELS
#define SFETCH_MAX_CHANNELS (16)
#endif

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
#ifndef SOKOL_MALLOC
    #include <stdlib.h>
    #define SOKOL_MALLOC(s) malloc(s)
    #define SOKOL_FREE(p) free(p)
#endif
#ifndef SOKOL_LOG
    #ifdef SOKOL_DEBUG
        #include <stdio.h>
        #define SOKOL_LOG(s) { SOKOL_ASSERT(s); puts(s); }
    #else
        #define SOKOL_LOG(s)
    #endif
#endif

#ifndef _SOKOL_PRIVATE
    #if defined(__GNUC__)
        #define _SOKOL_PRIVATE __attribute__((unused)) static
    #else
        #define _SOKOL_PRIVATE static
    #endif
#endif

#if defined(__EMSCRIPTEN__)
    #define SFETCH_PLATFORM_EMSCRIPTEN (1)
#elif defined(_WIN32)
    #ifndef WIN32_LEAN_AND_MEAN
    #define WIN32_LEAN_AND_MEAN
    #endif
    #include <windows.h>
    #define _SFETCH_PLATFORM_WINDOWS (1)
#else
    #include <pthread.h>
    #include <stdio.h>  /* fopen, fread, fseek, fclose */
    #define _SFETCH_PLATFORM_POSIX (1)
#endif

/*=== private type definitions ===============================================*/
typedef struct _sfetch_path_t {
    char buf[SFETCH_MAX_PATH];
} _sfetch_path_t;

/* a thread with incoming and outgoing message queue syncing */
#if defined(_SFETCH_PLATFORM_POSIX)
typedef struct {
    pthread_t thread;
    pthread_cond_t incoming_cond;
    pthread_mutex_t incoming_mutex;
    pthread_mutex_t outgoing_mutex;
    pthread_mutex_t running_mutex;
    bool stop_requested;
    bool valid;
} _sfetch_thread_t;
#elif defined(_SFETCH_PLATFORM_WINDOWS)
#error "FIXME WIndows"
#else
typedef struct { } _sfetch_thread_t;
#endif

/* file handle abstraction */
#if defined(_SFETCH_PLATFORM_POSIX)
typedef FILE* _sfetch_file_handle_t;
#elif defined(_SFETCH_PLATFORM_WINDOWS)
typedef HANDLE _sfetch_file_handle_t;
#else
#error "FIXME: file handle emscripten"
#endif

/* internal per-request item */
typedef struct {
    sfetch_handle_t handle;
    sfetch_state_t state;
    uint32_t channel;
    void (*callback)(sfetch_handle_t, sfetch_state_t);
    sfetch_buffer_t buffer;

    /* updated by IO thread */
    uint64_t num_content_bytes;         // set after OPENING
    uint64_t num_fetched_bytes;         // overall fetched bytes
    uint64_t num_buffer_bytes;          // currently valid fetched bytes in buffer
    bool failed;                        // FIXME: should be an error code
    _sfetch_file_handle_t file_handle;

    /* big stuff at the end */
    _sfetch_path_t path;
    int user_data_size;
    uint64_t user_data[SFETCH_MAX_USERDATA_UINT64];
} _sfetch_item_t;

/* a pool of internal per-request items */
typedef struct {
    uint32_t size;
    uint32_t free_top;
    _sfetch_item_t* items;
    uint32_t* free_slots;
    uint32_t* gen_ctrs;
    bool valid;
} _sfetch_pool_t;

/* a ringbuffer for pool-slot ids */
typedef struct {
    uint32_t head;
    uint32_t tail;
    uint32_t num;
    uint32_t* queue;
} _sfetch_ring_t;

/* an IO channel with its own IO thread */
typedef struct {
    _sfetch_ring_t incoming;
    _sfetch_ring_t outgoing;
    _sfetch_thread_t thread;
    void (*work_func)(uint32_t slot_id);
    bool valid;
} _sfetch_channel_t;

/* the sfetch global state (FIXME: allow per-thread contexts) */
typedef struct {
    bool setup;
    bool valid;
    sfetch_desc_t desc;
    _sfetch_pool_t pool;
    _sfetch_ring_t user_incoming[SFETCH_MAX_CHANNELS];
    _sfetch_ring_t user_outgoing;
    _sfetch_ring_t user_pending;        /* items without callback waiting for user-polling */
    _sfetch_channel_t chn[SFETCH_MAX_CHANNELS];
} _sfetch_t;
static _sfetch_t _sfetch;

/*=== general helper functions and macros =====================================*/
#define _sfetch_def(val, def) (((val) == 0) ? (def) : (val))

_SOKOL_PRIVATE void _sfetch_path_copy(_sfetch_path_t* dst, const char* src) {
    SOKOL_ASSERT(dst);
    if (src && (strlen(src) < SFETCH_MAX_PATH)) {
        #if defined(_MSC_VER)
        strncpy_s(dst->buf, SFETCH_MAX_PATH, src, (SFETCH_MAX_PATH-1));
        #else
        strncpy(dst->buf, src, SFETCH_MAX_PATH);
        #endif
        dst->buf[SFETCH_MAX_PATH-1] = 0;
    }
    else {
        memset(dst->buf, 0, SFETCH_MAX_PATH);
    }
}

_SOKOL_PRIVATE _sfetch_path_t _sfetch_path_make(const char* str) {
    _sfetch_path_t res;
    _sfetch_path_copy(&res, str);
    return res;
}

_SOKOL_PRIVATE uint32_t _sfetch_make_id(uint32_t index, uint32_t gen_ctr) {
    return (gen_ctr<<16) | (index & 0xFFFF);
}

_SOKOL_PRIVATE sfetch_handle_t _sfetch_make_handle(uint32_t slot_id) {
    sfetch_handle_t h;
    h.id = slot_id;
    return h;
}

_SOKOL_PRIVATE uint32_t _sfetch_slot_index(uint32_t slot_id) {
    return slot_id & 0xFFFF;
}

/*=== a circular message queue ===============================================*/
_SOKOL_PRIVATE uint32_t _sfetch_ring_index(const _sfetch_ring_t* rb, uint32_t i) {
    return i % rb->num;
}

_SOKOL_PRIVATE void _sfetch_ring_discard(_sfetch_ring_t* rb) {
    SOKOL_ASSERT(rb);
    if (rb->queue) {
        SOKOL_FREE(rb->queue);
        rb->queue = 0;
    }
    rb->head = 0;
    rb->tail = 0;
    rb->num = 0;
}

_SOKOL_PRIVATE bool _sfetch_ring_init(_sfetch_ring_t* rb, uint32_t num_slots) {
    SOKOL_ASSERT(rb && (num_slots > 0));
    SOKOL_ASSERT(0 == rb->queue);
    rb->head = 0;
    rb->tail = 0;
    /* one slot reserved to detect full vs empty */
    rb->num = num_slots + 1;
    const size_t queue_size = rb->num * sizeof(sfetch_handle_t);
    rb->queue = (uint32_t*) SOKOL_MALLOC(queue_size);
    if (rb->queue) {
        memset(rb->queue, 0, queue_size);
        return true;
    }
    else {
        _sfetch_ring_discard(rb);
        return false;
    }
}

_SOKOL_PRIVATE bool _sfetch_ring_full(const _sfetch_ring_t* rb) {
    SOKOL_ASSERT(rb && rb->queue);
    return _sfetch_ring_index(rb, rb->head + 1) == rb->tail;
}

_SOKOL_PRIVATE bool _sfetch_ring_empty(const _sfetch_ring_t* rb) {
    SOKOL_ASSERT(rb && rb->queue);
    return rb->head == rb->tail;
}

_SOKOL_PRIVATE uint32_t _sfetch_ring_count(const _sfetch_ring_t* rb) {
    SOKOL_ASSERT(rb && rb->queue);
    uint32_t count;
    if (rb->head >= rb->tail) {
        count = rb->head - rb->tail;
    }
    else {
        count = (rb->head + rb->num) - rb->tail;
    }
    SOKOL_ASSERT((count >= 0) && (count < rb->num));
    return count;
}

_SOKOL_PRIVATE void _sfetch_ring_enqueue(_sfetch_ring_t* rb, uint32_t slot_id) {
    SOKOL_ASSERT(rb && rb->queue);
    SOKOL_ASSERT(!_sfetch_ring_full(rb));
    SOKOL_ASSERT((rb->head >= 0) && (rb->head < rb->num));
    rb->queue[rb->head] = slot_id;
    rb->head = _sfetch_ring_index(rb, rb->head + 1);
}

_SOKOL_PRIVATE uint32_t _sfetch_ring_dequeue(_sfetch_ring_t* rb) {
    SOKOL_ASSERT(rb && rb->queue);
    SOKOL_ASSERT(!_sfetch_ring_empty(rb));
    SOKOL_ASSERT((rb->tail >= 0) && (rb->tail < rb->num));
    uint32_t slot_id = rb->queue[rb->tail];
    rb->tail = _sfetch_ring_index(rb, rb->tail + 1);
    return slot_id;
}

_SOKOL_PRIVATE uint32_t _sfetch_ring_peek(_sfetch_ring_t* rb, uint32_t index) {
    SOKOL_ASSERT(rb && rb->queue);
    SOKOL_ASSERT(!_sfetch_ring_empty(rb));
    SOKOL_ASSERT(index < _sfetch_ring_count(rb));
    uint32_t rb_index = _sfetch_ring_index(rb, rb->tail + index);
    return rb->queue[rb_index];
}

/*=== threading wrappers =====================================================*/
#if defined(_SFETCH_PLATFORM_POSIX)

_SOKOL_PRIVATE bool _sfetch_thread_init(_sfetch_thread_t* thread, void*(*thread_func)(void*), void* thread_arg) {
    SOKOL_ASSERT(thread && !thread->valid);

    pthread_mutexattr_t attr;
    pthread_mutexattr_init(&attr);
    pthread_mutex_init(&thread->incoming_mutex, &attr);
    pthread_mutexattr_destroy(&attr);

    pthread_mutexattr_init(&attr);
    pthread_mutex_init(&thread->outgoing_mutex, &attr);
    pthread_mutexattr_destroy(&attr);

    pthread_mutexattr_init(&attr);
    pthread_mutex_init(&thread->running_mutex, &attr);
    pthread_mutexattr_destroy(&attr);

    pthread_condattr_t cond_attr;
    pthread_condattr_init(&cond_attr);
    pthread_cond_init(&thread->incoming_cond, &cond_attr);
    pthread_condattr_destroy(&cond_attr);

    /* FIXME: in debug mode, the threads should be named */
    pthread_mutex_lock(&thread->running_mutex);
    thread->stop_requested = false;
    int res = pthread_create(&thread->thread, 0, thread_func, thread_arg);
    thread->valid = (0 == res);
    pthread_mutex_unlock(&thread->running_mutex);
    return thread->valid;
}

_SOKOL_PRIVATE void _sfetch_thread_join(_sfetch_thread_t* thread) {
    SOKOL_ASSERT(thread);
    if (thread->valid) {
        pthread_mutex_lock(&thread->incoming_mutex);
        thread->stop_requested = true;
        pthread_cond_signal(&thread->incoming_cond);
        pthread_mutex_unlock(&thread->incoming_mutex);
        pthread_join(thread->thread, 0);
        thread->valid = false;
    }
    pthread_mutex_destroy(&thread->running_mutex);
    pthread_mutex_destroy(&thread->incoming_mutex);
    pthread_mutex_destroy(&thread->outgoing_mutex);
    pthread_cond_destroy(&thread->incoming_cond);
}

/* called when the thread-func is entered, this blocks the thread func until
   the _sfetch_thread_t object is fully initialized
*/
_SOKOL_PRIVATE void _sfetch_thread_entered(_sfetch_thread_t* thread) {
    pthread_mutex_lock(&thread->running_mutex);
}

/* called by the thread-func right before it is left */
_SOKOL_PRIVATE void _sfetch_thread_leaving(_sfetch_thread_t* thread) {
    pthread_mutex_unlock(&thread->running_mutex);
}

_SOKOL_PRIVATE void _sfetch_thread_enqueue_incoming(_sfetch_thread_t* thread, _sfetch_ring_t* incoming, _sfetch_ring_t* src) {
    /* called from user thread */
    SOKOL_ASSERT(thread && thread->valid);
    SOKOL_ASSERT(incoming && incoming->queue);
    SOKOL_ASSERT(src && src->queue);
    pthread_mutex_lock(&thread->incoming_mutex);
    while (!_sfetch_ring_full(incoming) && !_sfetch_ring_empty(src)) {
        _sfetch_ring_enqueue(incoming, _sfetch_ring_dequeue(src));
    }
    pthread_cond_signal(&thread->incoming_cond);
    pthread_mutex_unlock(&thread->incoming_mutex);
}

_SOKOL_PRIVATE uint32_t _sfetch_thread_dequeue_incoming(_sfetch_thread_t* thread, _sfetch_ring_t* incoming) {
    /* called from thread function */
    SOKOL_ASSERT(thread && thread->valid);
    SOKOL_ASSERT(incoming && incoming->queue);
    pthread_mutex_lock(&thread->incoming_mutex);
    while (_sfetch_ring_empty(incoming) && !thread->stop_requested) {
        pthread_cond_wait(&thread->incoming_cond, &thread->incoming_mutex);
    }
    uint32_t item = 0;
    if (!thread->stop_requested) {
        item = _sfetch_ring_dequeue(incoming);
    }
    pthread_mutex_unlock(&thread->incoming_mutex);
    return item;
}

_SOKOL_PRIVATE bool _sfetch_thread_enqueue_outgoing(_sfetch_thread_t* thread, _sfetch_ring_t* outgoing, uint32_t item) {
    /* called from thread function */
    SOKOL_ASSERT(thread && thread->valid);
    SOKOL_ASSERT(outgoing && outgoing->queue);
    pthread_mutex_lock(&thread->outgoing_mutex);
    bool result = false;
    if (!_sfetch_ring_full(outgoing)) {
        _sfetch_ring_enqueue(outgoing, item);
    }
    pthread_mutex_unlock(&thread->outgoing_mutex);
    return result;
}

_SOKOL_PRIVATE void _sfetch_thread_dequeue_outgoing(_sfetch_thread_t* thread, _sfetch_ring_t* outgoing, _sfetch_ring_t* dst) {
    /* called from user thread */
    SOKOL_ASSERT(thread && thread->valid);
    SOKOL_ASSERT(outgoing && outgoing->queue);
    SOKOL_ASSERT(dst && dst->queue);
    pthread_mutex_lock(&thread->outgoing_mutex);
    while (!_sfetch_ring_full(dst) && !_sfetch_ring_empty(outgoing)) {
        _sfetch_ring_enqueue(dst, _sfetch_ring_dequeue(outgoing));
    }
    pthread_mutex_unlock(&thread->outgoing_mutex);
}

#elif defined(_SFETCH_PLATFORM_WINDOWS)
#error "FIXME Windows"
#else
#error "FIXME Emscripten"
#endif

/*=== file I/O wrappers ======================================================*/
#if defined(_SFETCH_PLATFORM_POSIX)
_SOKOL_PRIVATE _sfetch_file_handle_t _sfetch_file_open(const _sfetch_path_t* path) {
    SOKOL_ASSERT(path && (path->buf[0] != 0));
    return fopen(path->buf, "rb");
}

_SOKOL_PRIVATE void _sfetch_file_close(_sfetch_file_handle_t h) {
    fclose(h);
}

_SOKOL_PRIVATE bool _sfetch_file_handle_valid(_sfetch_file_handle_t h) {
    return h != 0;
}

_SOKOL_PRIVATE uint64_t _sfetch_file_size(_sfetch_file_handle_t h) {
    fseek(h, 0, SEEK_END);
    return ftell(h);
}

_SOKOL_PRIVATE bool _sfetch_file_read(_sfetch_file_handle_t h, uint64_t offset, uint64_t num_bytes, void* ptr) {
    fseek(h, offset, num_bytes);
    uint64_t bytes_read = fread(ptr, num_bytes, 1, h);
    return bytes_read == num_bytes;
}

#elif defined(_SFETCH_PLATFORM_WINDOWS)
// FIXME: on Windows, convert UTF-8 paths with MultiByteToWideChar(), and
// use the native Windows file IO functions
#else

#endif


/*=== request pool implementation ============================================*/
_SOKOL_PRIVATE void _sfetch_item_init(_sfetch_item_t* item, uint32_t slot_id, const sfetch_request_t* request) {
    SOKOL_ASSERT(item && (0 == item->handle.id));
    SOKOL_ASSERT(request && request->path);
    item->handle.id = slot_id;
    item->state = SFETCH_STATE_INITIAL;
    item->channel = request->channel;
    item->buffer = request->buffer;
    item->path = _sfetch_path_make(request->path);
    item->callback = request->state_changed_cb;
    if (request->user_data &&
        (request->user_data_size > 0) &&
        (request->user_data_size <= (SFETCH_MAX_USERDATA_UINT64*8)))
    {
        item->user_data_size = request->user_data_size;
        memcpy(item->user_data, request->user_data, request->user_data_size);
    }
}

_SOKOL_PRIVATE void _sfetch_item_discard(_sfetch_item_t* item) {
    SOKOL_ASSERT(item && (0 != item->handle.id));
    memset(item, 0, sizeof(_sfetch_item_t));
}

_SOKOL_PRIVATE void _sfetch_pool_discard(_sfetch_pool_t* pool) {
    SOKOL_ASSERT(pool);
    if (pool->free_slots) {
        SOKOL_FREE(pool->free_slots);
        pool->free_slots = 0;
    }
    if (pool->gen_ctrs) {
        SOKOL_FREE(pool->gen_ctrs);
        pool->gen_ctrs = 0;
    }
    if (pool->items) {
        SOKOL_FREE(pool->items);
        pool->items = 0;
    }
    pool->size = 0;
    pool->free_top = 0;
    pool->valid = false;
}

_SOKOL_PRIVATE bool _sfetch_pool_init(_sfetch_pool_t* pool, uint32_t num_items) {
    SOKOL_ASSERT(pool && (num_items > 0) && (num_items < ((1<<16)-1)));
    SOKOL_ASSERT(0 == pool->items);
    /* NOTE: item slot 0 is reserved for the special "invalid" item index 0*/
    pool->size = num_items + 1;
    pool->free_top = 0;
    const size_t items_size = pool->size * sizeof(_sfetch_item_t);
    pool->items = (_sfetch_item_t*) SOKOL_MALLOC(items_size);
    /* generation counters indexable by pool slot index, slot 0 is reserved */
    const size_t gen_ctrs_size = sizeof(uint32_t) * pool->size;
    pool->gen_ctrs = (uint32_t*) SOKOL_MALLOC(gen_ctrs_size);
    SOKOL_ASSERT(pool->gen_ctrs);
    /* NOTE: it's not a bug to only reserve num_items here */
    const size_t free_slots_size = num_items * sizeof(int);
    pool->free_slots = (uint32_t*) SOKOL_MALLOC(free_slots_size);
    if (pool->items && pool->free_slots) {
        memset(pool->items, 0, items_size);
        memset(pool->gen_ctrs, 0, gen_ctrs_size);
        /* never allocate the 0-th item, this is the reserved 'invalid item' */
        for (uint32_t i = pool->size - 1; i >= 1; i--) {
            pool->free_slots[pool->free_top++] = i;
        }
        pool->valid = true;
    }
    else {
        /* allocation error */
        _sfetch_pool_discard(pool);
    }
    return pool->valid;
}

_SOKOL_PRIVATE uint32_t _sfetch_pool_item_alloc(_sfetch_pool_t* pool, const sfetch_request_t* request) {
    SOKOL_ASSERT(pool && pool->valid);
    if (pool->free_top > 0) {
        uint32_t slot_index = pool->free_slots[--pool->free_top];
        SOKOL_ASSERT((slot_index > 0) && (slot_index < pool->size));
        uint32_t slot_id = _sfetch_make_id(slot_index, ++pool->gen_ctrs[slot_index]);
        _sfetch_item_init(&pool->items[slot_index], slot_id, request);
        pool->items[slot_index].state = SFETCH_STATE_ALLOCATED;
        return slot_id;
    }
    else {
        /* pool exhausted, return the 'invalid handle' */
        return _sfetch_make_id(0, 0);
    }
}

_SOKOL_PRIVATE void _sfetch_pool_item_free(_sfetch_pool_t* pool, uint32_t slot_id) {
    SOKOL_ASSERT(pool && pool->valid);
    uint32_t slot_index = _sfetch_slot_index(slot_id);
    SOKOL_ASSERT((slot_index > 0) && (slot_index < pool->size));
    SOKOL_ASSERT(pool->items[slot_index].handle.id == slot_id);
    #ifdef SOKOL_DEBUG
    /* debug check against double-free */
    for (uint32_t i = 0; i < pool->free_top; i++) {
        SOKOL_ASSERT(pool->free_slots[i] != slot_index);
    }
    #endif
    _sfetch_item_discard(&pool->items[slot_index]);
    pool->free_slots[pool->free_top++] = slot_index;
    SOKOL_ASSERT(pool->free_top <= (pool->size - 1));
}

/* return pointer to item by handle without matching id check */
_SOKOL_PRIVATE _sfetch_item_t* _sfetch_pool_item_at(_sfetch_pool_t* pool, uint32_t slot_id) {
    SOKOL_ASSERT(pool && pool->valid);
    uint32_t slot_index = _sfetch_slot_index(slot_id);
    SOKOL_ASSERT((slot_index > 0) && (slot_index < pool->size));
    return &pool->items[slot_index];
}

/* return pointer to item by handle with matching id check */
_SOKOL_PRIVATE _sfetch_item_t* _sfetch_pool_item_lookup(_sfetch_pool_t* pool, uint32_t slot_id) {
    SOKOL_ASSERT(pool && pool->valid);
    if (0 != slot_id) {
        _sfetch_item_t* item = _sfetch_pool_item_at(pool, slot_id);
        if (item->handle.id == slot_id) {
            return item;
        }
    }
    return 0;
}

/*=== IO CHANNEL implementation ==============================================*/
_SOKOL_PRIVATE void _sfetch_channel_worker(uint32_t slot_id) {
    _sfetch_item_t* item = _sfetch_pool_item_lookup(&_sfetch.pool, slot_id);
    if (!item) {
        return;
    }
    if (item->failed) {
        // FIXME: should this be a hard error?
        return;
    }
    SOKOL_ASSERT((item->state == SFETCH_STATE_OPENING) ||
                 (item->state == SFETCH_STATE_FETCHING) ||
                 (item->state == SFETCH_STATE_CLOSING));
    if (item->state == SFETCH_STATE_OPENING) {
        SOKOL_ASSERT(!_sfetch_file_handle_valid(item->file_handle));
        SOKOL_ASSERT(item->path.buf[0]);
        SOKOL_ASSERT(item->num_content_bytes == 0);
        SOKOL_ASSERT(item->num_fetched_bytes == 0);
        SOKOL_ASSERT(item->num_buffer_bytes == 0);
        item->file_handle = _sfetch_file_open(&item->path);
        if (_sfetch_file_handle_valid(item->file_handle)) {
            item->num_content_bytes = _sfetch_file_size(item->file_handle);
        }
        else {
            item->failed = true;
        }
    }
    else if (item->state == SFETCH_STATE_FETCHING) {
        SOKOL_ASSERT(_sfetch_file_handle_valid(item->file_handle));
        SOKOL_ASSERT(item->num_content_bytes > item->num_fetched_bytes);
        SOKOL_ASSERT(item->buffer.ptr && (item->buffer.ptr > 0));
        uint64_t bytes_to_read = item->num_content_bytes - item->num_fetched_bytes;
        if (bytes_to_read > item->buffer.num_bytes) {
            bytes_to_read = item->buffer.num_bytes;
        }
        const uint64_t offset = item->num_fetched_bytes;
        if (_sfetch_file_read(item->file_handle, offset, bytes_to_read, item->buffer.ptr)) {
            item->num_buffer_bytes = bytes_to_read;
            item->num_fetched_bytes += bytes_to_read;
        }
        else {
            item->failed = true;
        }
    }
    else if (item->state == SFETCH_STATE_CLOSING) {
        SOKOL_ASSERT(_sfetch_file_handle_valid(item->file_handle));
        _sfetch_file_close(item->file_handle);
        item->file_handle = 0;
    }
}

_SOKOL_PRIVATE void* _sfetch_channel_thread_func(void* arg) {
    _sfetch_channel_t* chn = (_sfetch_channel_t*) arg;
    _sfetch_thread_entered(&chn->thread);
    while (!chn->thread.stop_requested) {
        /* block until work arrives */
        uint32_t slot_id = _sfetch_thread_dequeue_incoming(&chn->thread, &chn->incoming);
        /* slot_id will be invalid if the thread was woken up to join */
        if (!chn->thread.stop_requested) {
            chn->work_func(slot_id);
            if (!_sfetch_thread_enqueue_outgoing(&chn->thread, &chn->outgoing, slot_id)) {
                // FIXME: what to do if outgoing queue is overflowing because
                // the user thread doesn't empty it?
            }
        }
    }
    _sfetch_thread_leaving(&chn->thread);
    return 0;
}

_SOKOL_PRIVATE void _sfetch_channel_discard(_sfetch_channel_t* chn) {
    SOKOL_ASSERT(chn);
    if (chn->valid) {
        _sfetch_thread_join(&chn->thread);
    }
    _sfetch_ring_discard(&chn->incoming);
    _sfetch_ring_discard(&chn->outgoing);
    chn->valid = false;
}

_SOKOL_PRIVATE bool _sfetch_channel_init(_sfetch_channel_t* chn, uint32_t num_items, void (*work_func)(uint32_t)) {
    SOKOL_ASSERT(chn && (num_items > 0) && work_func);
    SOKOL_ASSERT(!chn->valid);
    bool valid = true;
    chn->work_func = work_func;
    valid &= _sfetch_ring_init(&chn->incoming, num_items);
    valid &= _sfetch_ring_init(&chn->outgoing, num_items);
    if (valid) {
        chn->valid = true;
        _sfetch_thread_init(&chn->thread, _sfetch_channel_thread_func, chn);
        return true;
    }
    else {
        _sfetch_channel_discard(chn);
        return false;
    }
}

/* Move items from a source ringbuffer into the incoming ringbuffer, this
    is called once per frame from the user-thread to move all items that
    changed from a user-thread-state into a IO-thread-state back into the
    IO thread.
*/
_SOKOL_PRIVATE void _sfetch_channel_push(_sfetch_channel_t* chn, _sfetch_ring_t* src) {
    SOKOL_ASSERT(chn && chn->valid);
    SOKOL_ASSERT(src && src->queue);
    _sfetch_thread_enqueue_incoming(&chn->thread, &chn->incoming, src);
}

/* Move items from the outgoing ringbuffer into an destination ringbuffer until
    either the outgoing queue is empty or the destination ringbuffer is full.
    This is called once per frame from the user thread.
*/
_SOKOL_PRIVATE void _sfetch_channel_pop(_sfetch_channel_t* chn, _sfetch_ring_t* dst) {
    SOKOL_ASSERT(chn && chn->valid);
    SOKOL_ASSERT(dst && dst->queue);
    _sfetch_thread_dequeue_outgoing(&chn->thread, &chn->outgoing, dst);
}

/*=== private high-level functions ===========================================*/
_SOKOL_PRIVATE bool _sfetch_validate_request(const sfetch_request_t* req) {
    #if defined(SOKOL_DEBUG)
    if (req->channel >= _sfetch.desc.num_channels) {
        SOKOL_LOG("_sfetch_validate_request: request.num_channels too big!");
        return false;
    }
    if (!req->path) {
        SOKOL_LOG("_sfetch_validate_request: request.path is null!");
        return false;
    }
    if (strlen(req->path) >= (SFETCH_MAX_PATH-1)) {
        SOKOL_LOG("_sfetch_validate_request: request.path is too long (must be < SFETCH_MAX_PATH-1)");
        return false;
    }
    if (req->user_data && (req->user_data_size == 0)) {
        SOKOL_LOG("_sfetch_validate_request: request.user_data is set, but req.user_data_size is null");
        return false;
    }
    if (!req->user_data && (req->user_data_size > 0)) {
        SOKOL_LOG("_sfetch_validate_request: request.user_data is null, but req.user_data_size is not");
        return false;
    }
    if (req->user_data_size > SFETCH_MAX_USERDATA_UINT64 * sizeof(uint64_t)) {
        SOKOL_LOG("_sfetch_validate_request: request.user_data_size is too big (see SFETCH_MAX_USERDATA_UINT64");
        return false;
    }
    #endif
    return true;
}

/* this goes through all items in the user-incoming queues and updates their
   state right before they're moved into the IO-threads
*/
_SOKOL_PRIVATE void _sfetch_update_incoming_states(void) {
    for (uint32_t chn_index = 0; chn_index < _sfetch.desc.num_channels; chn_index++) {
        _sfetch_ring_t* rb = &_sfetch.user_incoming[chn_index];
        const uint32_t count = _sfetch_ring_count(rb);
        for (uint32_t i = 0; i < count; i++) {
            uint32_t slot_id = _sfetch_ring_peek(rb, i);
            _sfetch_item_t* item = _sfetch_pool_item_lookup(&_sfetch.pool, slot_id);
            if (item) {
                // FIXME: handle items in failed state
                SOKOL_ASSERT(item->state != SFETCH_STATE_INITIAL);
                SOKOL_ASSERT(item->state != SFETCH_STATE_CLOSED);
                SOKOL_ASSERT(item->state != SFETCH_STATE_OPENING);
                SOKOL_ASSERT(item->state != SFETCH_STATE_FETCHING);
                SOKOL_ASSERT(item->state != SFETCH_STATE_CLOSING);
                switch (item->state) {
                    case SFETCH_STATE_ALLOCATED:
                        item->state = SFETCH_STATE_OPENING;
                        break;
                    case SFETCH_STATE_OPENED:
                        item->state = SFETCH_STATE_FETCHING;
                        break;
                    case SFETCH_STATE_FETCHED:
                        if (item->num_fetched_bytes < item->num_content_bytes) {
                            item->state = SFETCH_STATE_FETCHING;
                        }
                        else {
                            item->state = SFETCH_STATE_CLOSING;
                        }
                        break;
                    default:
                        break;
                }
            }
        }
    }
}

/* this goes through all items in the user-outgoing queue and updates their
   state after they've moved from the IO-thread into the user-thread
*/
_SOKOL_PRIVATE void _sfetch_update_outgoing_states(void) {
    _sfetch_ring_t* rb = &_sfetch.user_outgoing;
    const uint32_t count = _sfetch_ring_count(rb);
    for (uint32_t i = 0; i < count; i++) {
        uint32_t slot_id = _sfetch_ring_peek(rb, i);
        _sfetch_item_t* item = _sfetch_pool_item_lookup(&_sfetch.pool, slot_id);
        if (item) {
            SOKOL_ASSERT(item->state != SFETCH_STATE_INITIAL);
            SOKOL_ASSERT(item->state != SFETCH_STATE_ALLOCATED);
            SOKOL_ASSERT(item->state != SFETCH_STATE_OPENED);
            SOKOL_ASSERT(item->state != SFETCH_STATE_FETCHED);
            SOKOL_ASSERT(item->state != SFETCH_STATE_CLOSED);
            if (item->failed) {
                item->state = SFETCH_STATE_CLOSED;
            }
            else {
                switch (item->state) {
                    case SFETCH_STATE_OPENING:
                        item->state = SFETCH_STATE_OPENED;
                        break;
                    case SFETCH_STATE_FETCHING:
                        item->state = SFETCH_STATE_FETCHED;
                        break;
                    case SFETCH_STATE_CLOSING:
                        item->state = SFETCH_STATE_CLOSED;
                        break;
                    default:
                        break;
                }
            }
        }
    }
}

/*=== PUBLIC API FUNCTIONS ===================================================*/
SOKOL_API_IMPL void sfetch_setup(const sfetch_desc_t* desc) {
    SOKOL_ASSERT(desc);
    SOKOL_ASSERT((desc->_start_canary == 0) && (desc->_end_canary == 0));
    memset(&_sfetch, 0, sizeof(_sfetch));
    _sfetch.desc = *desc;
    _sfetch.setup = true;
    _sfetch.valid = true;

    /* replace zero-init items with default values */
    _sfetch.desc.max_requests = _sfetch_def(_sfetch.desc.max_requests, 128);
    _sfetch.desc.num_channels = _sfetch_def(_sfetch.desc.num_channels, 1);
    if (_sfetch.desc.num_channels > SFETCH_MAX_CHANNELS) {
        _sfetch.desc.num_channels = SFETCH_MAX_CHANNELS;
        SOKOL_LOG("sfetch_setup: clamping num_channels to SFETCH_MAX_CHANNELS");
    }
    _sfetch.desc.timeout_num_frames = _sfetch_def(_sfetch.desc.timeout_num_frames, 30);

    /* setup the global request item pool */
    _sfetch.valid &= _sfetch_pool_init(&_sfetch.pool, _sfetch.desc.max_requests);

    /* setup user-side message queues */
    for (uint32_t i = 0; i < _sfetch.desc.num_channels; i++) {
        _sfetch.valid &= _sfetch_ring_init(&_sfetch.user_incoming[i], _sfetch.desc.max_requests);
    }
    _sfetch.valid &= _sfetch_ring_init(&_sfetch.user_outgoing, _sfetch.desc.max_requests);

    /* setup IO channels (one thread per channel) */
    for (uint32_t i = 0; i < _sfetch.desc.num_channels; i++) {
        _sfetch.valid &= _sfetch_channel_init(&_sfetch.chn[i], _sfetch.desc.max_requests, _sfetch_channel_worker);
    }
}

SOKOL_API_IMPL void sfetch_shutdown(void) {
    SOKOL_ASSERT(_sfetch.setup);
    /* IO threads must be shutdown first */
    for (uint32_t i = 0; i < _sfetch.desc.num_channels; i++) {
        if (_sfetch.chn[i].valid) {
            _sfetch_channel_discard(&_sfetch.chn[i]);
        }
    }
    _sfetch_ring_discard(&_sfetch.user_outgoing);
    for (uint32_t i = 0; i < _sfetch.desc.num_channels; i++) {
        _sfetch_ring_discard(&_sfetch.user_incoming[i]);
    }
    _sfetch_pool_discard(&_sfetch.pool);
    _sfetch.valid = false;
    _sfetch.setup = false;
}

SOKOL_API_IMPL bool sfetch_valid(void) {
    return _sfetch.valid;
}

SOKOL_API_IMPL sfetch_desc_t sfetch_desc(void) {
    return _sfetch.desc;
}

SOKOL_API_IMPL int sfetch_max_userdata_bytes(void) {
    return SFETCH_MAX_USERDATA_UINT64 * 8;
}

SOKOL_API_IMPL int sfetch_max_path(void) {
    return SFETCH_MAX_PATH;
}

SOKOL_API_IMPL sfetch_handle_t sfetch_send(const sfetch_request_t* request) {
    SOKOL_ASSERT(_sfetch.setup);
    SOKOL_ASSERT(request && (request->_start_canary == 0) && (request->_end_canary == 0));

    const sfetch_handle_t invalid_handle = _sfetch_make_handle(0);
    if (!_sfetch.valid) {
        return invalid_handle;
    }
    if (!_sfetch_validate_request(request)) {
        return invalid_handle;
    }
    SOKOL_ASSERT(request->channel < _sfetch.desc.num_channels);

    uint32_t slot_id = _sfetch_pool_item_alloc(&_sfetch.pool, request);
    if (0 == slot_id) {
        SOKOL_LOG("sfetch_send: request pool exhausted (too many active requests)");
        return invalid_handle;
    }
    if (!_sfetch_ring_full(&_sfetch.user_incoming[request->channel])) {
        SOKOL_LOG("sfetch_send: user_incoming queue is full)");
        return invalid_handle;
    }
    _sfetch_ring_enqueue(&_sfetch.user_incoming[request->channel], slot_id);
    return _sfetch_make_handle(slot_id);
}

SOKOL_API_IMPL void sfetch_dowork(void) {
    SOKOL_ASSERT(_sfetch.setup);
    if (!_sfetch.valid) {
        return;
    }

    /* update state of items moving from user- into IO-threads */
    _sfetch_update_incoming_states();

    /* move new requests into the IO channels and processed requests into the user-thread */
    for (uint32_t i = 0; i < _sfetch.desc.num_channels; i++) {
        if (!_sfetch_ring_empty(&_sfetch.user_incoming[i])) {
            _sfetch_channel_push(&_sfetch.chn[i], &_sfetch.user_incoming[i]);
        }
        _sfetch_channel_pop(&_sfetch.chn[i], &_sfetch.user_outgoing);
    }

    /* upate state of items which moved from IO-threads into user-thread */
    _sfetch_update_outgoing_states();

    /* for each item coming out of the IO thread, either call the user callback,
       or put it into the pending queue for polling
    */
    while (!_sfetch_ring_empty(&_sfetch.user_outgoing)) {
        uint32_t slot_id = _sfetch_ring_dequeue(&_sfetch.user_outgoing);
        _sfetch_item_t* item = _sfetch_pool_item_lookup(&_sfetch.pool, slot_id);
        SOKOL_ASSERT(item);
        if (item->callback) {
            item->callback(item->handle, item->state);
            /* either put the handled item back into the user_incoming queue
               to proceed to the next state, or delete it when it has arrived
               at the last state.
            */
            if (item->state == SFETCH_STATE_CLOSED) {
                _sfetch_pool_item_free(&_sfetch.pool, slot_id);
            }
            else {
                _sfetch_ring_enqueue(&_sfetch.user_incoming[item->channel], slot_id);
            }
        }
        else {
            SOKOL_ASSERT(!_sfetch_ring_full(&_sfetch.user_pending));
            _sfetch_ring_enqueue(&_sfetch.user_pending, slot_id);
        }
    }

    // FIXME: there needs to be a "garbage collection" for items in the user_pending
    // queue when user code "forgets" to handle the item.
}

#endif /* SOKOL_IMPL */

