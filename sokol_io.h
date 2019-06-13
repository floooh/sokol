#pragma once
/*
    sokol_io.h -- asynchronous file operations

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

    If sokol_io.h is compiled as a DLL, define the following before
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

typedef struct sio_path_t {
    char buf[512];
} sio_path_t;

typedef enum sio_request_type_t {
    SIO_REQUESTTYPE_LOAD,       /* default, load entire file */
    SIO_REQUESTTYPE_STREAM,     /* stream a file in smaller chunks */
} sio_request_type_t;

typedef struct sio_buffer_t {
    uint8_t* ptr;
    uint32_t num_bytes;
} sio_buffer_t;

typedef enum sio_error_t {
    SIO_ERROR_SUCCESS,      /* special value 0 means success */
    SIO_ERROR_NOT_FOUND,    /* file not found */
    SIO_ERROR_UNKNOWN,      /*  */
} sio_error_t;

typedef enum sio_response_type_t {
    SIO_RESPONSETYPE_START,         /* content-size and status is available */
    SIO_RESPONSETYPE_DATA,          /* partial (streamed) data available */
    SIO_RESPONSETYPE_COMPLETE,      /* operation complete (check status for error!) */
} sio_response_type_t;

typedef struct sio_response_t {
    sio_response_type_t type;
    bool success;
    sio_buffer_t buffer;              /* may be initialized in STARTED response */
    uint64_t user_id;
    void* user_data;
} sio_response_t;

typedef struct sio_request_t {
    sio_request_type_t type;
    bool (*response_cb)(sio_response_t*);
    uint64_t user_id;
    void* user_data;
    sio_buffer_t buffer;      /* ptr and num_bytes are both optional */
    sio_path_t path;
} sio_request_t;

/* create a path 'object' from a string */
sio_path_t sio_make_path(const char* str);
/* build a new path by appending a string to an existing path, inserts '/' if needed */
sio_path_t sio_append_path(const sio_path_t* path, const char* str);
/* send a request */
void sio_request(const sio_request_t* request);

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
typedef struct {
    pthread_mutex_t mutex;
} _sio_mutex_t;
#elif defined(_WIN32)
typedef struct {
    CRITICAL_SECTION critsec;
} _sio_mutex_t;
#else
typedef struct { } _sio_mutex_t;
#endif

/*=== IO QUEUE implementation ================================================*/

/* an io queue item is a request bundled with its response */
typedef struct {
    sio_request_t request;
    sio_response_t response;
} _sio_queue_item_t;

/* an io queue is a sparse array of active io requests */
typedef struct {
    bool valid;
    int num_items;          /* number of items in all allocated arrays */
    int free_top;           /* next free slot in free_items array */
    int active_top;         /* next free slot in active_items array */
    _sio_queue_item_t* items;
    int* free_slots;        /* item indices of all free item slots */
    int* active_slots;      /* item indices of all active item slots */
    _sio_mutex_t mutex;
} _sio_queue_t;

_SOKOL_PRIVATE void _sio_discard_queue(_sio_queue* queue) {
    if (queue->items) {
        SOKOL_FREE(queue->items); queue->items = 0;
    }
    if (queue->free_slots) {
        SOKOL_FREE(queue->free_slots); queue->free_slots = 0;
    }
    if (queue->active_slots) {
        SOKOL_FREE(queue->active_slots); queue->active_slots = 0;
    }
    _sio_discard_mutex(&queue->mutex);
    queue->valid = false;
}

_SOKOL_PRIVATE bool _sio_init_queue(_sio_queue_t* queue, int num_items) {
    SOKOL_ASSERT(queue && (num_items > 0));
    SOKOL_ASSERT(!queue->valid);
    _sio_init_mutex(&queue->mutex);
    queue->num_items = num_items;
    const size_t items_size = num_items * sizeof(_sio_queue_item_t);
    const size_t slots_size = num_items * sizeof(int);
    queue->items = (_sio_queue_item_t*) SOKOL_MALLOC(items_size);
    queue->free_slots = (int*) SOKOL_MALLOC(slots_size);
    queue->active_slots = (int*) SOKOL_MALLOC(slots_size);
    if (queue->items && queue->free_slots && queue->active_slots) {
        queue->valid = true;
        return true;
    }
    else {
        _sio_discard_queue(queue);
        return false;
    }
}


#endif /* SOKOL_IMPL */

