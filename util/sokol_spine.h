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

// FIXME FIXME FIXME

#ifdef __cplusplus
} // extern "C"
#endif

/*-- IMPLEMENTATION ----------------------------------------------------------*/
#ifdef SOKOL_SPINE_IMPL
#define SOKOL_SPINE_IMPL_INCLUDED (1)

#if !defined(SPINE_SPINE_H_)
#error "Please include spine/spine.h before sokol_spine.h"
#endif

// FIXME FIXME FIXME

#endif // SOKOL_SPINE_IMPL
