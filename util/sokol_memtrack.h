#if defined(SOKOL_IMPL) && !defined(SOKOL_MEMTRACK_IMPL)
#define SOKOL_MEMTRACK_IMPL
#endif
#ifndef SOKOL_MEMTRACK_INCLUDED
/*
    sokol_memtrack.h -- memory allocation wrapper to track memory usage
                        of sokol libraries

    Project URL: https://github.com/floooh/sokol

    Simply include this file before the sokol header implementation includes
    and after the SOKOL_IMPL macro:

    #define SOKOL_IMPL
    #include "sokol_memtrack.h"
    #include "sokol_app.h"
    #include "sokol_gfx.h"
    ...

    Optionally provide the following defines with your own implementations:

    SOKOL_MEMTRACK_API_DECL - public function declaration prefix (default: extern)
    SOKOL_API_DECL      - same as SOKOL_MEMTRACK_API_DECL
    SOKOL_API_IMPL      - public function implementation prefix (default: -)

    If sokol_memtrack.h is compiled as a DLL, define the following before
    including the declaration or implementation:

    SOKOL_DLL

    The sokol_memtrack.h header will redirect the macros SOKOL_MALLOC,
    SOKOL_FREE and SOKOL_CALLOC to use its own function wrappers which
    keep track of number and size of allocations. The wrapper functions
    then call the CRT functions malloc(), calloc() or free().

    Naturally, only the memory management calls in sokol header code are tracked,
    not in underlying APIs such as D3D11 or OpenGL.

    To get the current number and overall size of allocations, call:

        smemtrack_info_t alloc_info = smemtrack_info();

        int num_allocations = info.num_alloc;
        int allocation_size = info.num_bytes;

    The allocation wrapper functions are *not* thread-safe (which shouldn't
    be a problem because the sokol headers don't allocate or deallocate
    in threads).

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

#ifdef __cplusplus
} /* extern "C" */
#endif
#endif /* SOKOL_MEMTRACK_INCLUDED */

/*=== IMPLEMENTATION =========================================================*/
#ifdef SOKOL_MEMTRACK_IMPL
#define SOKOL_MEMTRACK_IMPL_INCLUDED (1)
#include <stdlib.h> /* malloc, free, calloc */
#include <string.h> /* memset */
#include <stddef.h> /* size_t */

#ifndef SOKOL_API_IMPL
    #define SOKOL_API_IMPL
#endif
#ifndef SOKOL_DEBUG
    #ifndef NDEBUG
        #define SOKOL_DEBUG (1)
    #endif
#endif
#ifndef _SOKOL_PRIVATE
    #if defined(__GNUC__) || defined(__clang__)
        #define _SOKOL_PRIVATE __attribute__((unused)) static
    #else
        #define _SOKOL_PRIVATE static
    #endif
#endif

#define SOKOL_MALLOC(s) _smemtrack_malloc(s)
#define SOKOL_FREE(p) _smemtrack_free(p)
#define SOKOL_CALLOC(n,s) _smemtrack_calloc(n,s)

#define _SMEMTRACK_HEADER_SIZE (16)

static struct {
    smemtrack_info_t state;
} _smemtrack;

_SOKOL_PRIVATE void* _smemtrack_malloc(size_t size) {
    _smemtrack.state.num_allocs++;
    _smemtrack.state.num_bytes += (int) size;
    uint8_t* ptr = (uint8_t*) malloc(size + _SMEMTRACK_HEADER_SIZE);
    *(size_t*)ptr = size;
    return ptr + _SMEMTRACK_HEADER_SIZE;
}

_SOKOL_PRIVATE void _smemtrack_free(void* ptr) {
    uint8_t* alloc_ptr = ((uint8_t*)ptr) - _SMEMTRACK_HEADER_SIZE;
    size_t size = *(size_t*)alloc_ptr;
    _smemtrack.state.num_allocs--;
    _smemtrack.state.num_bytes -= (int) size;
    free(alloc_ptr);
}

_SOKOL_PRIVATE void* _smemtrack_calloc(size_t num, size_t size) {
    size_t mem_size = num * size;
    _smemtrack.state.num_allocs++;
    _smemtrack.state.num_bytes += (int) mem_size;
    uint8_t* ptr = (uint8_t*) malloc(mem_size + _SMEMTRACK_HEADER_SIZE);
    memset(ptr + _SMEMTRACK_HEADER_SIZE, 0, mem_size);
    *(size_t*)ptr = size;
    return ptr + _SMEMTRACK_HEADER_SIZE;
}

SOKOL_API_IMPL smemtrack_info_t smemtrack_info(void) {
    return _smemtrack.state;
}

#endif /* SOKOL_MEMTRACK_IMPL */
