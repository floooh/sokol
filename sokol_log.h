#if defined(SOKOL_IMPL) && !defined(SOKOL_LOG_IMPL)
#define SOKOL_LOG_IMPL
#endif
#ifndef SOKOL_LOG_INCLUDED
/*
    sokol_log.h -- common logging callback for sokol headers

    Project URL: https://github.com/floooh/sokol

    Example code: https://github.com/floooh/sokol-samples

    Do this:
        #define SOKOL_IMPL or
        #define SOKOL_LOG_IMPL
    before you include this file in *one* C or C++ file to create the
    implementation.

    Optionally provide the following defines when building the implementation:

    SOKOL_ASSERT(c)             - your own assert macro (default: assert(c))
    SOKOL_UNREACHABLE()         - a guard macro for unreachable code (default: assert(false))
    SOKOL_LOG_API_DECL          - public function declaration prefix (default: extern)
    SOKOL_API_DECL              - same as SOKOL_GFX_API_DECL
    SOKOL_API_IMPL              - public function implementation prefix (default: -)

    Optionally define the following for verbose output:

    SOKOL_DEBUG         - by default this is defined if _DEBUG is defined

    FIXME: documentation

    LICENSE
    =======
    zlib/libpng license

    Copyright (c) 2023 Andre Weissflog

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
#define SOKOL_LOG_INCLUDED (1)
#include <stdint.h>

#if defined(SOKOL_API_DECL) && !defined(SOKOL_LOG_API_DECL)
#define SOKOL_LOG_API_DECL SOKOL_API_DECL
#endif
#ifndef SOKOL_LOG_API_DECL
#if defined(_WIN32) && defined(SOKOL_DLL) && defined(SOKOL_LOG_IMPL)
#define SOKOL_LOG_API_DECL __declspec(dllexport)
#elif defined(_WIN32) && defined(SOKOL_DLL)
#define SOKOL_LOG_API_DECL __declspec(dllimport)
#else
#define SOKOL_LOG_API_DECL extern
#endif
#endif

#ifdef __cplusplus
extern "C" {
#endif

/*
    Plug this function into the 'logger.func' struct item when initializating any of the sokol
    headers. For instance for sokol_audio.h it would loom like this:

    saudio_setup(&(saudio_desc){
        .logger = {
            .func = slog_func
        }
    });
*/
void slog_func(const char* tag, uint32_t log_level, uint32_t log_item, const char* message, int line_nr, const char* filename, void* user_data);

#ifdef __cplusplus
} // extern "C"
#endif
#endif // SOKOL_LOG_INCLUDED

// ██╗███╗   ███╗██████╗ ██╗     ███████╗███╗   ███╗███████╗███╗   ██╗████████╗ █████╗ ████████╗██╗ ██████╗ ███╗   ██╗
// ██║████╗ ████║██╔══██╗██║     ██╔════╝████╗ ████║██╔════╝████╗  ██║╚══██╔══╝██╔══██╗╚══██╔══╝██║██╔═══██╗████╗  ██║
// ██║██╔████╔██║██████╔╝██║     █████╗  ██╔████╔██║█████╗  ██╔██╗ ██║   ██║   ███████║   ██║   ██║██║   ██║██╔██╗ ██║
// ██║██║╚██╔╝██║██╔═══╝ ██║     ██╔══╝  ██║╚██╔╝██║██╔══╝  ██║╚██╗██║   ██║   ██╔══██║   ██║   ██║██║   ██║██║╚██╗██║
// ██║██║ ╚═╝ ██║██║     ███████╗███████╗██║ ╚═╝ ██║███████╗██║ ╚████║   ██║   ██║  ██║   ██║   ██║╚██████╔╝██║ ╚████║
// ╚═╝╚═╝     ╚═╝╚═╝     ╚══════╝╚══════╝╚═╝     ╚═╝╚══════╝╚═╝  ╚═══╝   ╚═╝   ╚═╝  ╚═╝   ╚═╝   ╚═╝ ╚═════╝ ╚═╝  ╚═══╝
//
// >>implementation
#ifdef SOKOL_LOG_IMPL
#define SOKOL_LOG_IMPL_INCLUDED (1)

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

#ifndef _SOKOL_PRIVATE
    #if defined(__GNUC__) || defined(__clang__)
        #define _SOKOL_PRIVATE __attribute__((unused)) static
    #else
        #define _SOKOL_PRIVATE static
    #endif
#endif

#ifndef _SOKOL_UNUSED
    #define _SOKOL_UNUSED(x) (void)(x)
#endif

// platform detection
#if defined(__APPLE__)
    #define _SLOG_APPLE (1)
#if defined(__EMSCRIPTEN__)
    #define _SLOG_EMSCRIPTEN (1)
#elif defined(_WIN32)
    #define _SLOG_WINDOWS (1)
#elif defined(__ANDROID__)
    #define _SLOG_ANDROID (1)
#elif defined(__linux__) || defined(__unix__)
    #define _SAUDIO_LINUX (1)
#else
#error "sokol_log.h: unknown platform"
#endif

#include <stdio.h>

#if defined(_SLOG_APPLE)
#include <os/log.h>
#elif defined(_SLOG_WINDOWS)
#ifndef WIN32_LEAN_AND_MEAN
    #define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
    #define NOMINMAX
#endif
#include <windows.h>



#endif // SOKOL_LOG_IMPL