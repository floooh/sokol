#pragma once
/*
    sokol_text.h -- text render on top of fontstash.h, stb_truetype.h and sokol_gl.h

    WIP!!!

    Project URL: https://github.com/floooh/sokol

    Do this:
        #define SOKOL_TEXT_IMPL
    before you include this file in *one* C or C++ file to create the
    implementation.

    ...optionally provide the following macros to override defaults:

    SOKOL_ASSERT(c)     - your own assert macro (default: assert(c))
    SOKOL_MALLOC(s)     - your own malloc function (default: malloc(s))
    SOKOL_FREE(p)       - your own free function (default: free(p))
    SOKOL_API_DECL      - public function declaration prefix (default: extern)
    SOKOL_API_IMPL      - public function implementation prefix (default: -)
    SOKOL_LOG(msg)      - your own logging function (default: puts(msg))
    SOKOL_UNREACHABLE() - a guard macro for unreachable code (default: assert(false))

    If sokol_text.h is compiled as a DLL, define the following before
    including the declaration or implementation:

    SOKOL_DLL

    On Windows, SOKOL_DLL will define SOKOL_API_DECL as __declspec(dllexport)
    or __declspec(dllimport) as needed.

    Include the following headers before including sokol_text.h:

        sokol_gfx.h
        sokol_gl.h

    Additionally include the following headers before including the
    sokol_text.h implementation:

        fontstash.h
        stb_truetype.h

    The fontstash.h and matching stb_truetype.h headers can be found here:

    https://github.com/memononen/fontstash/tree/master/src


    FIXME: documentation

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
#define SOKOL_TEXT_INCLUDED (1)
#include <stdint.h>
#include <stdbool.h>


#if !defined(SOKOL_GFX_INCLUDED)
#error "Please include sokol_gfx.h before sokol_text.h"
#endif
#if !defined(SOKOL_GL_INCLUDED)
#error "Please include sokol_gl.h before sokol_text.h"
#endif

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

/* a sokol_text.h font handle */
typedef struct stx_font_t { uint32_t id; } stx_font_t;

/* setup parameters */
typedef stx_desc_t {
    int max_fonts;          /* max number of fonts that will be added, default is 16 */
} stx_desc_t;

typedef enum stx_align_t {
    STX_ALIGN_LEFT      = (1<<0),
    STX_ALIGN_CENTER    = (1<<1),
    STX_ALIGN_RIGHT     = (1<<2),
    STX_ALIGN_TOP       = (1<<3),
    STX_ALIGN_MIDDLE    = (1<<4),
    STX_ALIGN_BOTTOM    = (1<<5),
    STX_ALIGN_BASELINE  = (1<<6),
} stx_align_t;

/* font setup parameters */
typedef stx_font_desc_t {
    const char* name;
    struct {
        const void* ptr;        /* pointer to TTF font data in memory */
        int size;               /* size of TTF font data in number of bytes */
    } ttf_data;
    int atlas_texture_width;    /* width of atlas texture (default: 1024) */
    int atlas_texture_height;   /* height of atlas texture (default: 1024) */
} stx_font_desc_t;

/* text bounding rectangle */
typedef stx_bounds_t {
    struct { float x, y; } min;
    struct { float x, y; } max;
    float width, height;
} stx_bounds_t;

SOKOL_API_DECL void stx_setup(const stx_desc_t* desc);
SOKOL_API_DECL void stx_shutdown(void);
SOKOL_API_DECL stx_font_t stx_make_font(const stx_font_desc_t* desc);
SOKOL_API_DECL stx_destroy_font(stx_font_t font);

SOKOL_API_DECL void stx_push_state(void);
SOKOL_API_DECL void stx_pop_state(void);
SOKOL_API_DECL void stx_clear_state(void);

SOKOL_API_DECL void stx_font(stx_font_t font);
SOKOL_API_DECL void stx_size(float size);
SOKOL_API_DECL void stx_color_u32(uint32_t rgba);
SOKOL_API_DECL void stx_color_rgbaf(float r, float g, float b, float a);
SOKOL_API_DECL void stx_color_rgba8(uint8_t r, uint8_t g, uint8_t b, uint8_t a);
SOKOL_API_DECL void stx_spacing(float spacing);
SOKOL_API_DECL void stx_blur(float blur);
SOKOL_API_DECL void stx_align(stx_align_t align);

SOKOL_API_DECL void stx_draw_string(float x, float y, const char* str);
SOKOL_API_DECL void stx_draw_slice(float x, float y, const char* str, const char* end);
SOKOL_API_DECL stx_bounds_t stx_bounds_string(float x, float y, const char* str);
SOKOL_API_DECL stx_bounds_t stx_bounds_slice(float x, float y, const char* str, const char* end);

SOKOL_API_DECL stx_font_t stx_alloc_font(void);
SOKOL_API_DECL void stx_init_font(stx_font_t font, const stx_font_desc* desc);

#ifdef __cplusplus
} /* extern "C" */
#endif

/*-- IMPLEMENTATION ----------------------------------------------------------*/
#ifdef SOKOL_TEXT_IMPL
#define SOKOL_TEXT_IMPL_INCLUDED (1)

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
#ifndef SOKOL_UNREACHABLE
    #define SOKOL_UNREACHABLE SOKOL_ASSERT(false)
#endif


#endif /* SOKOL_TEXT_IMPL */
