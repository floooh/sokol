#ifndef SOKOL_DEBUGTEXT_INCLUDED
/*
    WIP WIP WIP

    sokol_debugtext.h   - simple ASCII debug text rendering on top of sokol_gfx.h

    Project URL: https://github.com/floooh/sokol

    Do this:
        #define SOKOL_DEBUGTEXT_IMPL
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
    SOKOL_WGPU

    Define at least one of the following fonts to use:

    SOKOL_DEBUGTEXT_FONT_KC853
    SOKOL_DEBUGTEXT_FONT_KC854
    SOKOL_DEBUGTEXT_FONT_Z1013
    SOKOL_DEBUGTEXT_FONT_CPC
    SOKOL_DEBUGTEXT_FONT_C64

    ...optionally provide the following macros to override defaults:

    SOKOL_SNPRINTF      - the function name of an alternative snprintf() function (default: snprintf)
    SOKOL_ASSERT(c)     - your own assert macro (default: assert(c))
    SOKOL_MALLOC(s)     - your own malloc function (default: malloc(s))
    SOKOL_FREE(p)       - your own free function (default: free(p))
    SOKOL_API_DECL      - public function declaration prefix (default: extern)
    SOKOL_API_IMPL      - public function implementation prefix (default: -)
    SOKOL_LOG(msg)      - your own logging function (default: puts(msg))
    SOKOL_UNREACHABLE() - a guard macro for unreachable code (default: assert(false))

    If sokol_debugtext.h is compiled as a DLL, define the following before
    including the declaration or implementation:

    SOKOL_DLL

    On Windows, SOKOL_DLL will define SOKOL_API_DECL as __declspec(dllexport)
    or __declspec(dllimport) as needed.

    Include the following headers before including sokol_debugtext.h:

        sokol_gfx.h

    FEATURE OVERVIEW
    ================
    [TODO]

    STEP BY STEP
    ============
    [TODO]


    LICENSE
    =======
    zlib/libpng license

    Copyright (c) 2020 Andre Weissflog

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
#define SOKOL_DEBUGTEXT_INCLUDED (1)
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

#if defined(__GNUC__)
#define SOKOL_DEBUGTEXT_PRINTF_ATTR __attribute__((format(printf, 1, 2)))
#else
#define SOKOL_DEBUGTEXT_PRINTF_ATTR
#endif

#ifdef __cplusplus
extern "C" {
#endif

/*
    sdtx_font_t

    Identifies one of the built-in 'vintage fonts'. To use a specific font it
    must also be 'activated' with the matching SOKOL_DEBUGTEXT_FONT_*
    compile-time define.
*/
typedef enum sdtx_font_t {
    SDTX_FONT_KC853,
    SDTX_FONT_KC854,
    SDTX_FONT_CPC,
    SDTC_FONT_C64,
} sdtx_font_t;

/* a rendering context handle */
typedef struct sdtx_context { uint32_t id; } sdtx_context;

/*
    sdtx_context_desc_t

    Describes the initialization parameters of a rendering context. Creating
    additional rendering contexts is useful if you want to render in
    different sokol-gfx rendering passes, or when rendering several layers
    of text.
*/
typedef struct sdtx_context_desc_t {
    int max_draw_chars;             // max number of characters rendered in one frame, default: 16384
    int max_printf_chars;           // size of internal buffer for snprintf()
    int canvas_width;               // the initial virtual canvas width, default: 640
    int canvas_height;              // the initial virtual canvas height, default: 400
    sdtx_font_t font;               // the default font (default is the first valid embedded font)
    sg_pixel_format color_format;   // color pixel format of target render pass
    sg_pixel_format depth_format;   // depth pixel format of target render pass
    int sample_count;               // MSAA sample count of target render pass
} sdtx_context_desc_t;

/*
    sdtx_desc_t

    Describes the sokol-debugtext API initialization parameters. Passed
    to the sdtx_setup() function.
*/
typedef struct sdtx_desc_t {
    int context_pool_size;          // max number of rendering contexts that can be created, default: 16
    sdtx_context_desc_t context;    // the default context creation parameters
} sdtx_desc_t;

/* initialization/shutdown */
SOKOL_API_DECL void sdtx_setup(const sdtx_desc_t* desc);
SOKOL_API_DECL void sdtx_shutdown(void);

/* context functions */
SOKOL_API_DECL sdtx_context sdtx_make_context(const sdtx_context_desc_t* desc);
SOKOL_API_DECL void sdtx_destroy_context(sdtx_context ctx);
SOKOL_API_DECL void sdtx_set_context(sdtx_context ctx);
SOKOL_API_DECL sdtx_context sdtx_get_context(void);

/* draw and 'rewind' the current context */
SOKOL_API_DECL void sdtx_draw(void);

/* set a new virtual canvas size in screen pixels */
SOKOL_API_DECL void sdtx_canvas(int w, int h);

/* set a new origin in virtual canvas pixels */
SOKOL_API_DECL void sdtx_origin(int x, int y);

/* cursor movement functions */
SOKOL_API_DECL void sdtx_home(void);        // move to origin
SOKOL_API_DECL void sdtx_pos(int x, int y); // move to character grid pos
SOKOL_API_DECL void sdtx_x(int x);          // move to abs x, keep y unchanged
SOKOL_API_DECL void sdtx_y(int y);          // move to abs y, keep x unchanged
SOKOL_API_DECL void sdtx_dx(int dx);        // move left/right by dx
SOKOL_API_DECL void sdtx_dy(int dy);        // move up/down by dy
SOKOL_API_DECL void sdtx_crlf(void);        // start new line, same as sdtx_x(0); sdtx_dy(1)

/* get current cursor position */
SOKOL_API_DECL int sdtx_get_x(void);
SOKOL_API_DECL int sdtx_get_y(void);

/* set the current text color */
SOKOL_API_DECL void sdtx_color3b(uint8_t r, uint8_t g, uint8_t b);              // RGB 0..255, A=255
SOKOL_API_DECL void sdtx_color3f(float r, float g, float b);                    // RGB 0..1, A=1
SOKOL_API_DECL void sdtx_color4b(uint8_t r, uint8_t g, uint8_t b, uint8_t a);   // RGBA 0..255
SOKOL_API_DECL void sdtx_color4f(float r, float g, float b, float a);           // RGBA 0..1
SOKOL_API_DECL void sdtx_color1i(uint32_t rgba);

/* text rendering */
SOKOL_API_DECL void sdtx_putc(int chr);
SOKOL_API_DECL void sdtx_puts(const char* str);             // does NOT append newline!
SOKOL_API_DECL void sdtx_putr(const char* str, int len);    // 'put range', also stops at zero-char
SOKOL_API_DECL int sdtx_printf(const char* fmt, ...) SOKOL_DEBUGTEXT_PRINTF_ATTR;

#ifdef __cplusplus
} /* extern "C" */
#endif
#endif /* SOKOL_DEBUGTEXT_INCLUDED */


/*-- IMPLEMENTATION ----------------------------------------------------------*/
#ifdef SOKOL_DEBUGTEXT_IMPL
#define SOKOL_DEBUGTEXT_IMPL_INCLUDED (1)

#ifndef SOKOL_SNPRINTF
#define SOKOL_SNPRINTF snprintf
#endif

#endif /* SOKOL_DEBUGTEXT_IMPL */
