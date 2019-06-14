#pragma once
/*
    sokol_fetch.h -- asynchronous data loading

    Project URL: https://github.com/floooh/sokol

    Do this:
        #define SOKOL_IMPL
    before you include this file in *one* C or C++ file to create the
    implementation.

    Optionally provide the following defines with your own implementations:

    SOKOL_ASSERT(c)     - your own assert macro (default: assert(c))
    SOKOL_MALLOC(s)     - your own malloc function (default: malloc(s))
    SOKOL_FREE(p)       - your own free function (default: free(p))
    SOKOL_LOG(msg)      - your own logging function (default: puts(msg))
    SOKOL_UNREACHABLE() - a guard macro for unreachable code (default: assert(false))
    SOKOL_API_DECL      - public function declaration prefix (default: extern)
    SOKOL_API_IMPL      - public function implementation prefix (default: -)

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

/* a request goes through the following states, ping-ponging between IO and user thread */
typedef enum sfetch_state_t {
    SFETCH_STATE_OPENING = 0,   /* IO thread: waiting to be opened */
    SFETCH_STATE_OPENED,        /* user thread: opened, size and status valid, waiting for buffer */
    SFETCH_STATE_FETCHING,      /* IO thread: waiting for data to be fetched */
    SFETCH_STATE_FETCHED,       /* user thread: processing fetched data */
    SFETCH_STATE_CLOSING,       /* IO thread: waiting to be closed */
    SFETCH_STATE_CLOSED,        /* user thread: user-side cleanup */
} sfetch_state_t;

typedef struct sfetch_path_t {
    char buf[512];
} sfetch_path_t;

typedef struct sfetch_buffer_t {
    uint8_t* ptr;
    uint32_t num_bytes;
} sfetch_buffer_t;

typedef struct sfetch_request_t {
    bool (*callback)(sfetch_request_t*);
    void* user_data;
    sfetch_buffer_t buffer;
    sfetch_path_t path;
    sfetch_state_t state;
} sfetch_request_t;

/* create a path 'object' from a string */
sfetch_path_t sfetch_make_path(const char* str);
/* build a new path by appending a string to an existing path, inserts '/' if needed */
sfetch_path_t sfetch_append_path(const sio_path_t* path, const char* str);
/* send a fetch-request */
void sfetch_request(const sfetch_request_t* request);

#ifdef __cplusplus
} /* extern "C" */
#endif

/*--- IMPLEMENTATION ---------------------------------------------------------*/
#ifdef SOKOL_IMPL
#define SOKOL_FETCH_IMPL_INCLUDED (1)

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

/*=== threading primitives ===================================================*/
#if (defined(__APPLE__) || defined(__linux__) || defined(__unix__)) && !defined(__EMSCRIPTEN__)
#define _SFETCH_PTHREAD (1)
typedef struct {
    pthread_mutex_t mutex;
} _sfetch_mutex_t;

_SOKOL_PRIVATE void _sfetch_mutex_init(_sfetch_mutex_t* m) {
    pthread_mutex_attr_t attr;
    pthread_mutexattr_init(&attr);
    pthread_mutex_init(&m->mutex, &attr);
}

_SOKOL_PRIVATE void _sfetch_mutex_discard(_sfetch_mutex_t* m) {
    pthread_mutex_destroy(&m->mutex);
}

_SOKOL_PRIVATE void _sfetch_mutex_lock(_sfetch_mutex_t* m) {
    pthread_mutex_lock(&m->mutex);
}

_SOKOL_PRIVATE void _sfetch_mutex_unlock(_sfetch_mutex_t* m) {
    pthread_mutex_unlock(&m->mutex);
}
#elif defined(_WIN32)
#define _SFETCH_WINTHREAD (1)
typedef struct {
    CRITICAL_SECTION critsec;
} _sfetch_mutex_t;

_SOKOL_PRIVATE void _sfetch_mutex_init(_sfetch_mutex_t* m) {
    InitializeCriticalSection(&m->critsect);
}

_SOKOL_PRIVATE void _sfetch_mutex_discard(_sfetch_mutex_t* m) {
    DeleteCriticalSection(&m->critsect);
}

_SOKOL_PRIVATE void _sfetch_mutex_lock(_sfetch_mutex_t* m) {
    EnterCriticalSection(&m->critsect);
}

_SOKOL_PRIVATE void _sfetch_mutex_unlock(_sfetch_mutex_t* m) {
    LeaveCriticalSection(&m->critsect);
#else
#define _SFETCH_NOTHREAD (1)
typedef struct { } _sio_mutex_t;
_SOKOL_PRIVATE void _sfetch_mutex_init(_sfetch_mutex_t* m) { (void)m; }
_SOKOL_PRIVATE void _sfetch_mutex_destroy(_sfetch_mutex_t* m) { (void)m; }
_SOKOL_PRIVATE void _sfetch_mutex_lock(_sfetch_mutex_t* m) { (void)m; }
_SOKOL_PRIVATE void _sfetch_mutex_unlock(_sfetch_mutex_t* m) { (void)m; }
#endif
#endif

/*=== request pool implementation ============================================*/

/* internal per-request item (request plus private data) */
typedef struct {
    sfetch_request_t request;
    bool valid;
} _sfetch_item_t;

_SOKOL_PRIVATE void _sfetch_item_init(_sfetch_item_t* item, const sfetch_request_t* request) {
    SOKOL_ASSERT(item && !item->valid);
    SOKOL_ASSERT(request);
    item->request = *request;
    item->valid = true;
}

_SOKOL_PRIVATE void _sfetch_item_discard(_sfetch_item_t* item) {
    SOKOL_ASSERT(item && item->valid);
    memset(item, 0, sizeof(_sfetch_item_t));
}

typedef struct {
    int size;
    int free_top;
    _sfetch_item_t* items;
    int* free_slots;
    bool valid;
} _sfetch_pool_t;

_SOKOL_PRIVATE void _sfetch_pool_discard(_sfetch_pool_t* pool) {
    SOKOL_ASSERT(pool);
    if (pool->items) {
        SOKOL_FREE(pool->items);
        pool->items = 0;
    }
    if (pool->free_slots) {
        SOKOL_FREE(pool->free_slots);
        pool->free_slots = 0;
    }
    pool->size = 0;
    pool->free_top = 0;
    pool->valid = false;
}

_SOKOL_PRIVATE bool _sfetch_pool_init(_sfetch_pool_t* pool, int num_items) {
    SOKOL_ASSERT(pool && (num_items > 0));
    SOKOL_ASSERT(0 == pool->items);
    /* NOTE: item slot 0 is reserved for the special "invalid" item index 0*/
    pool->size = num_items + 1;
    pool->free_top = 0;
    const int items_size = pool->size * sizeof(_sfetch_item_t);
    pool->items = (_sfetch_item_t*) SOKOL_MALLOC(items_size);
    /* NOTE: it's not a bug to only reserve num_items here */
    const int free_slots_size = num_items * sizeof(int);
    pool->free_slots = (int*) SOKOL_MALLOC(free_slots_size);
    if (pool->items && pool->free_slots) {
        memset(pool->items, 0, items_size);
        /* never allocate the 0-th item, this is the reserved 'invalid item' */
        for (int i = pool->size - 1; i >= 1; i--) {
            pool->free_slots[pool->free_top++] = i;
        }
        pool->valid = true;
    }
    else {
        /* allocation error */
        _sfetch_pool_discard(pool);
        return false;
    }
}

_SOKOL_PRIVATE int _sfetch_pool_alloc_item(_sfetch_pool_t* pool, const sfetch_request_t* request) {
    SOKOL_ASSERT(pool && pool->valid);
    if (pool->free_top > 0) {
        int item_index = pool->free_slots[--pool->free_top];
        SOKOL_ASSERT((item_index > 0) && (item_index < pool->size));
        _sfetch_item_init(&pool->items[item_index], request);
        return item_index;
    }
    else {
        /* pool exhausted, return the 'invalid index' */
        return 0;
    }
}

_SOKOL_PRIVATE void _sfetch_pool_free_item(_sfetch_pool_t* pool, int item_index) {
    SOKOL_ASSERT(pool && pool->valid);
    SOKOL_ASSERT((item_index > 0) && (item_index < pool->size));
    #ifdef SOKOL_DEBUG
    /* debug check against double-free */
    for (int i = 0; i < pool->free_top; i++) {
        SOKOL_ASSERT(pool->free_slots[i] != item_index);
    }
    #endif
    _sfetch_item_discard(&pool->items[item_index]);
    pool->free_slots[pool->free_top++] = slot_index;
    SOKOL_ASSERT(pool->free_top <= (pool->size - 1));
}

/*=== IO QUEUE implementation ================================================*/

/*
    An IO queue is a set of request items waiting to be processed by
    an IO thread, or the user thread which issued the request.

    The user thread own a single IO queue with items waiting to
    be processed by the user callback (request states
    OPENED, FETCHED, CLOSED).

    Each IO thread owns a single IO queue with items waiting to
    be worked on (OPENING, FETCHING, CLOSING).

    Queue items are indices into the global item pool.

    Each IO queue has 3 internal 'sub-queues' of:
        - waiting: items waiting to be processed (that's basically the main-queue)
        - incoming: incoming items to be added to the waiting queue
        - outgoing: processed items from the waiting queue
    The incoming and outgoing queues exist to reduce and simplify
    thread-locking.

    FIXME: maybe implement support for multiple user threads later?
    This would allow to issue IO requests from multiple user threads
    into multiple IO threads.
*/

/* a ringbuffer for the actual internal queues */
typedef struct {
    int head;
    int tail;
    int num;
    int* queue;
} _sfetch_ring_t;

_SOKOL_PRIVATE int _sfetch_ring_index(const _sfetch_ring_t* rb, int i) {
    return (int) (i % rb->num);
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

_SOKOL_PRIVATE bool _sfetch_ring_init(_sfetch_ring_t* rb, int num_slots) {
    SOKOL_ASSERT(rb && (num_slots > 0));
    SOKOL_ASSERT(0 == rb->queue);
    rb->head = 0;
    rb->tail = 0;
    /* one slot reserved to detect full vs empty */
    rb->num = num_slots + 1;
    rb->queue = (int*) SOKOL_MALLOC(rb->num * sizeof(int));
    if (rb->queue) {
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

_SOKOL_PRIVATE bool _sfetch_ring_count(const _sfetch_ring_t* rb) {
    SOKOL_ASSERT(rb && rb->queue);
    int count;
    if (ring->head >= ring->tail) {
        count = ring->head - ring->tail;
    }
    else {
        count = (ring->head + ring->num) - ring->tail;
    }
    SOKOL_ASSERT((count >= 0) && (count < ring->num));
    return count;
}

_SOKOL_PRIVATE void _sfetch_ring_enqueue(_sfetch_ring_t* rb, int val) {
    SOKOL_ASSERT(rb && rb->queue);
    SOKOL_ASSERT(!_saudio_ring_full(rb));
    SOKOL_ASSERT((rb->head >= 0) && (rb->head < rb->num));
    rb->queue[rb->head] = val;
    rb->head = _sfetch_ring_index(rb, rb->head + 1);
}

_SOKOL_PRIVATE int _sfetch_ring_dequeue(_sfetch_ring_t* rb) {
    SOKOL_ASSERT(rb && rb->queue);
    SOKOL_ASSERT(!_saudio_ring_empty(rb));
    SOKOL_ASSERT((rb->tail >= 0) && (rb->tail < rb->num));
    int res = rb->queue[rb->tail];
    rb->tail = _sfetch_ring_index(rb, rb->tail + 1);
    return res;
}

/* the actual IO queue implementation */
typedef struct {
    _sfetch_ring_t waiting;
    _sfetch_ring_t incoming;
    _sfetch_ring_t outgoing;
    _sfetch_mutex_t incoming_mutex;
    _sfetch_mutex_t outgoing_mutex;
    bool valid;
} _sfetch_queue_t;

_SOKOL_PRIVATE void _sfetch_queue_discard(_sio_queue* queue) {
    SOKOL_ASSERT(queue);
    _sfetch_ring_discard(&queue->waiting);
    _sfetch_ring_discard(&queue->incoming);
    _sfetch_ring_discard(&queue->outgoing);
    _sfetch_mutex_discard(&queue->incoming_mutex);
    _sfetch_mutex_discard(&queue->outgoing_mutex);
    queue->valid = false;
}

_SOKOL_PRIVATE bool _fetch_queue_init(_sfetch_queue_t* queue, int num_items) {
    SOKOL_ASSERT(queue && (num_items > 0));
    SOKOL_ASSERT(!queue->valid);
    bool valid = true;
    valid &= _sfetch_mutex_init(&queue->incoming_mutex);
    valid &= _sfetch_mutex_init(&queue->outgoing_mutex);
    valid &= _sfetch_ring_init(&queue->waiting, num_items);
    valid &= _sfetch_ring_init(&queue->incoming, num_items);
    valid &= _sfetch_ring_init(&queue->outgoing, num_items);
    if (valid) {
        queue->valid = true;
        return true;
    }
    else {
        _sio_queue_discard(queue);
        return false;
    }
}

#endif /* SOKOL_IMPL */

