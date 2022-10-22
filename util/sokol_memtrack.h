#if defined(SOKOL_IMPL) && !defined(SOKOL_MEMTRACK_IMPL)
#define SOKOL_MEMTRACK_IMPL
#endif
#ifndef SOKOL_MEMTRACK_INCLUDED
/*
    sokol_memtrack.h -- memory allocation wrapper to track memory usage
                        of sokol libraries

    Project URL: https://github.com/floooh/sokol

    Optionally provide the following defines with your own implementations:

    SOKOL_MEMTRACK_API_DECL - public function declaration prefix (default: extern)
    SOKOL_API_DECL      - same as SOKOL_MEMTRACK_API_DECL
    SOKOL_API_IMPL      - public function implementation prefix (default: -)

    If sokol_memtrack.h is compiled as a DLL, define the following before
    including the declaration or implementation:

    SOKOL_DLL

    USAGE
    =====
    Just plug the malloc/free wrapper functions into the desc.allocator
    struct provided by most sokol header setup functions:

        sg_setup(&(sg_desc){
            //...
            .allocator = {
                .alloc = smemtrack_alloc,
                .free = smemtrack_free,
            }
        });

    Then call smemtrack_info() to get information about current number
    of allocations and overall allocation size:

        const smemtrack_info_t info = smemtrack_info();
        const int num_allocs = info.num_allocs;
        const int num_bytes = info.num_bytes;

    Note the sokol_memtrack.h can only track allocations issued by
    the sokol headers, not allocations that happen under the hood
    in system libraries.

    LICENSE
    =======

    zlib/libpng license

    Copyright (c) 2018 Andre Weissflog

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
#define SOKOL_MEMTRACK_INCLUDED (1)
#include <stdint.h>
#include <stddef.h> // size_t

#if defined(SOKOL_API_DECL) && !defined(SOKOL_MEMTRACK_API_DECL)
#define SOKOL_MEMTRACK_API_DECL SOKOL_API_DECL
#endif
#ifndef SOKOL_MEMTRACK_API_DECL
#if defined(_WIN32) && defined(SOKOL_DLL) && defined(SOKOL_MEMTRACK_IMPL)
#define SOKOL_MEMTRACK_API_DECL __declspec(dllexport)
#elif defined(_WIN32) && defined(SOKOL_DLL)
#define SOKOL_MEMTRACK_API_DECL __declspec(dllimport)
#else
#define SOKOL_MEMTRACK_API_DECL extern
#endif
#endif

#ifdef __cplusplus
extern "C" {
#endif

typedef struct smemtrack_info_t {
    int num_allocs;
    int num_bytes;
} smemtrack_info_t;

SOKOL_MEMTRACK_API_DECL smemtrack_info_t smemtrack_info(void);
SOKOL_MEMTRACK_API_DECL void* smemtrack_alloc(size_t size, void* user_data);
SOKOL_MEMTRACK_API_DECL void smemtrack_free(void* ptr, void* user_data);

#ifdef __cplusplus
} /* extern "C" */
#endif
#endif /* SOKOL_MEMTRACK_INCLUDED */

/*=== IMPLEMENTATION =========================================================*/
#ifdef SOKOL_MEMTRACK_IMPL
#define SOKOL_MEMTRACK_IMPL_INCLUDED (1)
#include <stdlib.h> // malloc, free
#include <string.h> // memset

#ifndef SOKOL_API_IMPL
    #define SOKOL_API_IMPL
#endif
#ifndef SOKOL_DEBUG
    #ifndef NDEBUG
        #define SOKOL_DEBUG
    #endif
#endif
#ifndef _SOKOL_PRIVATE
    #if defined(__GNUC__) || defined(__clang__)
        #define _SOKOL_PRIVATE __attribute__((unused)) static
    #else
        #define _SOKOL_PRIVATE static
    #endif
#endif

// per-allocation header used to keep track of the allocation size
#define _SMEMTRACK_HEADER_SIZE (16)

static struct {
    smemtrack_info_t state;
} _smemtrack;

SOKOL_API_IMPL void* smemtrack_alloc(size_t size, void* user_data) {
    (void)user_data;
    uint8_t* ptr = (uint8_t*) malloc(size + _SMEMTRACK_HEADER_SIZE);
    if (ptr) {
        // store allocation size (for allocation size tracking)
        *(size_t*)ptr = size;
        _smemtrack.state.num_allocs++;
        _smemtrack.state.num_bytes += (int) size;
        return ptr + _SMEMTRACK_HEADER_SIZE;
    }
    else {
        // allocation failed, return null pointer
        return ptr;
    }
}

SOKOL_API_IMPL void smemtrack_free(void* ptr, void* user_data) {
    (void)user_data;
    if (ptr) {
        uint8_t* alloc_ptr = ((uint8_t*)ptr) - _SMEMTRACK_HEADER_SIZE;
        size_t size = *(size_t*)alloc_ptr;
        _smemtrack.state.num_allocs--;
        _smemtrack.state.num_bytes -= (int) size;
        free(alloc_ptr);
    }
}

SOKOL_API_IMPL smemtrack_info_t smemtrack_info(void) {
    return _smemtrack.state;
}

#endif /* SOKOL_MEMTRACK_IMPL */
