#if defined(SOKOL_IMPL) && !defined(SOKOL_LETTERBOX_IMPL)
#define SOKOL_LETTERBOX_IMPL
#endif
#ifndef SOKOL_LETTERBOX_INCLUDED
/*
    sokol_letterbox.h -- provide fixed-aspect viewport for any framebuffer size

    Project URL: https://github.com/floooh/sokol

    Optionally provide the following defines with your own implementations:

    SOKOL_ASSERT(c)     - your own assert macro (default: assert(c))
    SOKOL_LETTERBOX_API_DECL - public function declaration prefix (default: extern)
    SOKOL_API_DECL      - same as SOKOL_LETTERBOX_API_DECL
    SOKOL_API_IMPL      - public function implementation prefix (default: -)

    If sokol_letterbox.h is compiled as a DLL, define the following before
    including the declaration or implementation:

    SOKOL_DLL

    USAGE
    =====
    [TODO]

    LICENSE
    =======

    zlib/libpng license

    Copyright (c) 2026 Andre Weissflog

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
#define SOKOL_LETTERBOX_INCLUDED (1)
#include <stdint.h>

#if defined(SOKOL_API_DECL) && !defined(SOKOL_LETTERBOX_API_DECL)
#define SOKOL_LETTERBOX_API_DECL SOKOL_API_DECL
#endif
#ifndef SOKOL_LETTERBOX_API_DECL
#if defined(_WIN32) && defined(SOKOL_DLL) && defined(SOKOL_LETTERBOX_IMPL)
#define SOKOL_LETTERBOX_API_DECL __declspec(dllexport)
#elif defined(_WIN32) && defined(SOKOL_DLL)
#define SOKOL_LETTERBOX_API_DECL __declspec(dllimport)
#else
#define SOKOL_LETTERBOX_API_DECL extern
#endif
#endif

#ifdef __cplusplus
extern "C" {
#endif

/*
    FIXME: docs
*/
typedef struct slbx_border {
    int left;
    int right;
    int top;
    int bottom;
} slbx_border;

/*
    FIXME: docs
*/
typedef enum slbx_anchor {
    _SLBX_ANCHOR_DEFAULT = 0,
    SLBX_ANCHOR_TOP = 0x1,
    SLBX_ANCHOR_BOTTOM = 0x2,
    SLBX_ANCHOR_LEFT = 0x4,
    SLBX_ANCHOR_RIGHT = 0x8,
    SLBX_ANCHOR_TOP_LEFT = 0x5,
    SLBX_ANCHOR_TOP_RIGHT = 0x9,
    SLBX_ANCHOR_BOTTOM_LEFT = 0x6,
    SLBX_ANCHOR_BOTTOM_RIGHT = 0xA,
    _SLBX_ANCHOR_FORCE_U32 = 0x7FFFFFFF,
} slbx_anchor;

/*
    FIXME: docs
*/
typedef struct slbx_options {
    float aspect;
    slbx_anchor anchor;
    slbx_border border;
} slbx_options;

/*
    FIXME: docs
*/
typedef struct slbx_rect {
    int x;
    int y;
    int width;
    int height;
} slbx_rect;

SOKOL_LETTERBOX_API_DECL slbx_rect slbx_viewport(int width, int height, const slbx_options* opts);

#ifdef __cplusplus
} // extern "C"
#endif
#endif // SOKOL_LETTERBOX_INCLUDED

/*=== IMPLEMENTATION =========================================================*/
#ifdef SOKOL_LETTERBOX_IMPL
#define SOKOL_LETTERBOX_IMPL_INCLUDED (1)

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

slbx_rect slbx_viewport(int width, int height, const slbx_options* opts) {
    SOKOL_ASSERT((width > 0) && (height > 0));
    SOKOL_ASSERT(opts);
    SOKOL_ASSERT(opts->aspect > 0.0f);
    const float b_top = (float)opts->border.top;
    const float b_bottom = (float)opts->border.bottom;
    const float b_left = (float)opts->border.left;
    const float b_right = (float)opts->border.right;
    float cw = (float)width - (b_left + b_right);
    if (cw < 1.0f) {
        cw = 1.0f;
    }
    float ch = (float)height - (b_top + b_bottom);
    if (ch < 1.0f) {
        ch = 1.0f;
    }
    const float content_aspect = opts->aspect;
    const float canvas_aspect = (float)width / (float)height;
    float vp_x, vp_y, vp_w, vp_h;
    if (content_aspect < canvas_aspect) {
        vp_y =
        vp_h = ch;
        vp_w = ch * content_aspect;
        vp_x = b_left + (cw - vp_w) * 0.5f;
    } else {
        vp_x = b_left;
        vp_w = cw;
        vp_h = cw / content_aspect;
        vp_y = b_top + (ch - vp_h) * 0.5f;
    }
    const slbx_rect res = { (int)vp_x, (int)vp_y, (int)vp_w, (int)vp_h };
    return res;
}

#endif // SOKOL_LETTERBOX_IMPL
