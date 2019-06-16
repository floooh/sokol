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
    int num_channels;                   /* number of channels to fetch requests in parallel, default is 1 */
    int max_active_requests;            /* max number of requests 'in flight' */
    int state_change_timeout_frames;    /* number of frames until request is discarded in polling mode if
                                           user code doesn't respond to a request state change */
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
} sfetch_state_t;

typedef struct sfetch_buffer_t {
    uint8_t* ptr;
    uint32_t num_bytes;
} sfetch_buffer_t;

typedef struct sfetch_request_t {
    uint32_t channel;
    const char* path;
    void (*state_changed_cb)(sfetch_handle_t, sfetch_state_t);  /* optional, can also use a polling model */
    sfetch_buffer_t buffer;     /* it's optional to provide a buffer upfront, can also happen in OPENED state */
    void* user_data;            /* optional user-data will be memcpy'ed into a memory region set aside for each request */
    int user_data_size;
} sfetch_request_t;

/* setup sokol-fetch (FIXME: per-thread contexts?) */
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
SOKOL_API_DECL sfetch_handle_t sfetch_request(const sfetch_request_t* request);
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

#if (defined(__APPLE__) || defined(__linux__) || defined(__unix__)) && !defined(__EMSCRIPTEN__)
    #include <pthread.h>
    #define _SFETCH_PTHREADS (1)
#elif defined(_WIN32)
    #ifndef WIN32_LEAN_AND_MEAN
    #define WIN32_LEAN_AND_MEAN
    #endif
    #include <windows.h>
    #define _SFETCH_WINTHREADS (1)
#else
    #define _SFETCH_NOTHREADS (1)
#endif

/*=== private type definitions ===============================================*/
typedef struct _sfetch_path_t {
    char buf[SFETCH_MAX_PATH];
} _sfetch_path_t;

/* a thread with incoming and outgoing message queue syncing */
#if defined(_SFETCH_PTHREADS)
typedef struct {
    pthread_t thread;
    pthread_cond_t incoming_cond;
    pthread_mutex_t incoming_mutex;
    pthread_mutex_t outgoing_mutex;
    bool stop_requested;
    bool valid;
} _sfetch_thread_t;
#elif defined(_SFETCH_WINTHREADS)
#error "FIXME WIndows"
#else
typedef struct { } _sfetch_thread_t;
#endif

/* internal per-request item */
typedef struct {
    sfetch_handle_t handle;
    sfetch_state_t state;
    void (*callback)(sfetch_handle_t, sfetch_state_t);
    sfetch_buffer_t buffer;
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

/* an IO queue (associated with an IO thread) */
typedef struct {
    _sfetch_ring_t incoming;
    _sfetch_ring_t outgoing;
    _sfetch_thread_t thread;
    bool valid;
} _sfetch_queue_t;

/* an IO channel is a thread with an IO queue */
typedef struct {
    _sfetch_queue_t queue;
    _sfetch_thread_t thread;
} _sfetch_channel_t;

/*=== general helper functions ===============================================*/
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

_SOKOL_PRIVATE uint32_t _sfetch_slot_index(uint32_t slot_id) {
    return slot_id & 0xFFFF;
}

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

/*=== a message circular queue ===============================================*/
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

/*=== threading wrappers =====================================================*/
#if defined(_SFETCH_PTHREADS)

_SOKOL_PRIVATE bool _sfetch_thread_init(_sfetch_thread_t* thread, void*(*thread_func)(void*), void* thread_arg) {
    SOKOL_ASSERT(thread && (thread->thread == 0));

    pthread_mutexattr_t attr;
    pthread_mutexattr_init(&attr);
    pthread_mutex_init(&thread->incoming_mutex, &attr);
    pthread_mutexattr_destroy(&attr);

    pthread_mutexattr_init(&attr);
    pthread_mutex_init(&thread->outgoing_mutex, &attr);
    pthread_mutexattr_destroy(&attr);

    pthread_condattr_t cond_attr;
    pthread_condattr_init(&cond_attr);
    pthread_cond_init(&thread->incoming_cond, &cond_attr);
    pthread_condattr_destroy(&cond_attr);

    thread->stop_requested = false;
    int res = pthread_create(&thread->thread, 0, thread_func, thread_arg);
    thread->valid = (0 == res);
    return thread->valid;
}

_SOKOL_PRIVATE void _sfetch_thread_join(_sfetch_thread_t* thread) {
    SOKOL_ASSERT(thread && (thread->thread != 0));
    pthread_mutex_lock(&thread->incoming_mutex);
    thread->stop_requested = true;
    pthread_cond_signal(&thread->incoming_cond);
    pthread_mutex_unlock(&thread->incoming_mutex);
    pthread_join(thread->thread, 0);
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

#elif defined(_SFETCH_WINTHREADS)
#error "FIXME Windows"
#else
#error "FIXME Emscripten"
#endif

/*=== request pool implementation ============================================*/
_SOKOL_PRIVATE void _sfetch_item_init(_sfetch_item_t* item, uint32_t slot_id, const sfetch_request_t* request) {
    SOKOL_ASSERT(item && (0 == item->handle.id));
    SOKOL_ASSERT(request && request->path);
    item->handle.id = slot_id;
    item->state = SFETCH_STATE_INITIAL;
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

/*=== IO QUEUE implementation ================================================*/

_SOKOL_PRIVATE void* _sfetch_queue_thread_func(void* arg) {
    _sfetch_queue_t* queue = (_sfetch_queue_t*) arg;
    while (!queue->thread.stop_requested) {
        /* block until work arrives */
        uint32_t slot_id = _sfetch_thread_dequeue_incoming(&queue->thread, &queue->incoming);
        // FIXME: process request
        /* slot_id will be invalid if the thread was woken up to join */
        if (!queue->thread.stop_requested) {
            if (!_sfetch_thread_enqueue_outgoing(&queue->thread, &queue->outgoing, slot_id)) {
                // FIXME: what to do if outgoing queue is overflowing because
                // the user thread doesn't empty it?
            }
        }
    }
    return 0;
}

_SOKOL_PRIVATE void _sfetch_queue_discard(_sfetch_queue_t* queue) {
    SOKOL_ASSERT(queue);
    _sfetch_ring_discard(&queue->incoming);
    _sfetch_ring_discard(&queue->outgoing);
    if (queue->valid) {
        _sfetch_thread_join(&queue->thread);
    }
    queue->valid = false;
}

_SOKOL_PRIVATE bool _sfetch_queue_init(_sfetch_queue_t* queue, uint32_t num_items) {
    SOKOL_ASSERT(queue && (num_items > 0));
    SOKOL_ASSERT(!queue->valid);
    bool valid = true;
    valid &= _sfetch_ring_init(&queue->incoming, num_items);
    valid &= _sfetch_ring_init(&queue->outgoing, num_items);
    if (valid) {
        queue->valid = true;
        _sfetch_thread_init(&queue->thread, _sfetch_queue_thread_func, queue);
        return true;
    }
    else {
        _sfetch_queue_discard(queue);
        return false;
    }
}

/* Move items from a source ringbuffer into the incoming ringbuffer, this
    is called once per frame from the user-thread to move all items that
    changed from a user-thread-state into a IO-thread-state back into the
    IO thread.
*/
_SOKOL_PRIVATE void _sfetch_queue_push(_sfetch_queue_t* queue, _sfetch_ring_t* src) {
    SOKOL_ASSERT(queue && queue->valid);
    SOKOL_ASSERT(src && src->queue);
    _sfetch_thread_enqueue_incoming(&queue->thread, &queue->incoming, src);
}

/* Move items from the outgoing ringbuffer into an destination ringbuffer until
    either the outgoing queue is empty or the destination ringbuffer is full.
    This is called once per frame from the user thread.
*/
_SOKOL_PRIVATE void _sfetch_queue_pop(_sfetch_queue_t* queue, _sfetch_ring_t* dst) {
    SOKOL_ASSERT(queue && queue->valid);
    SOKOL_ASSERT(dst && dst->queue);
    _sfetch_thread_dequeue_outgoing(&queue->thread, &queue->outgoing, dst);
}

/*=== validation functions ===================================================*/
_SOKOL_PRIVATE bool _sfetch_validate_request(sfetch_request_t* req) {
    // FIXME
    return true;
}

/*=== PUBLIC API FUNCTIONS ===================================================*/
SOKOL_API_IMPL void sfetch_setup(const sfetch_desc_t* desc) {
    // FIXME
}

SOKOL_API_IMPL void sfetch_shutdown(void) {
    // FIXME
}

SOKOL_API_IMPL bool sfetch_valid(void) {
    // FIXME
    return false;
}

SOKOL_API_IMPL int sfetch_max_userdata_bytes(void) {
    return SFETCH_MAX_USERDATA_UINT64 * 8;
}

SOKOL_API_DECL int sfetch_max_path(void) {
    return SFETCH_MAX_PATH;
}

#endif /* SOKOL_IMPL */

