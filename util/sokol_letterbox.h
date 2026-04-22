#if defined(SOKOL_IMPL) && !defined(SOKOL_LETTERBOX_IMPL)
#define SOKOL_LETTERBOX_IMPL
#endif
#ifndef SOKOL_LETTERBOX_INCLUDED
/*
    sokol_letterbox.h -- provide fixed-aspect viewport for random-aspect framebuffer

    Project URL: https://github.com/floooh/sokol

    Optionally provide the following defines with your own implementations:

    SOKOL_ASSERT(c)     - your own assert macro (default: assert(c))
    SOKOL_LETTERBOX_API_DECL - public function declaration prefix (default: extern)
    SOKOL_API_DECL      - same as SOKOL_LETTERBOX_API_DECL
    SOKOL_API_IMPL      - public function implementation prefix (default: -)

    If sokol_letterbox.h is compiled as a DLL, define the following before
    including the declaration or implementation:

    SOKOL_DLL

    WHAT
    ====
    Computes viewport parameters to render fixed-aspect content in a variable-aspect
    framebuffer (e.g. position a 16:9 frame in a randomly sized window) - commonly
    known as 'letterboxing'.

    Check the WASM example here:

        https://floooh.github.io/sokol-html5/letterbox-sapp.html

    USAGE
    =====
    Just call slbx_letterbox() and plug the result into sg_apply_viewport().

    Takes a framebuffer width/height as input and a pointer to an slbx_letterbox_desc
    struct:

    ```c
    int w = sapp_width();
    int h = sapp_height();
    slbx_viewport vp = slbx_letterbox(w, h, &(slbx_letterbox_desc){
        .content_aspect_ratio = 16.0f / 9.0f,
    });
    ```

    ...then plug the resulting viewport parameters into `sg_apply_viewport()` (or
    a similar viewport function).

    ```c
    sg_apply_viewport(vp.x, vp.y, vp.width, vp.height, true);
    ```

    You can define a 'safe border' in pixels:
    ```c
    slbx_viewport vp = slbx_letterbox(w, h, &(slbx_letterbox_desc){
        .content_aspect_ratio = 16.0f / 9.0f,
        .border = {
            .left = 10,
            .right = 10,
            .top = 10,
            .bottom = 10,
        },
    });
    ```

    ...and finally you can anchor the content so that it sticks to an edge
    of the framebuffer (the default behaviour is to center the content):

    ```c
    slbx_viewport vp = slbx_letterbox(w, h, &(slbx_letterbox_desc){
        .content_aspect_ratio = 16.0f / 9.0f,
        .anchor = SLBX_ANCHOR_TOP,
        .border = {
            .left = 10,
            .right = 10,
            .top = 10,
            .bottom = 10,
        },
    });
    ```

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
    Defines a 'safe border' in pixels. Used as nested struct
    in slbx_letterbox_desc.
*/
typedef struct slbx_border {
    int left;
    int right;
    int top;
    int bottom;
} slbx_border;

/*
    Anchor the content to a side. The default is to center the content.
    Used in slbx_letterbox_desc.
*/
typedef enum slbx_anchor {
    SLBX_ANCHOR_CENTER = 0,
    SLBX_ANCHOR_TOP,
    SLBX_ANCHOR_BOTTOM,
    SLBX_ANCHOR_LEFT,
    SLBX_ANCHOR_RIGHT,
    _SLBX_ANCHOR_FORCE_U32 = 0x7FFFFFFF,
} slbx_anchor;

/*
    The content letterbox description. Used as input to the
    slbx_letterbox() function.
*/
typedef struct slbx_letterbox_desc {
    float content_aspect_ratio;     // width / height, default: 1.0f
    slbx_anchor anchor;
    slbx_border border;
} slbx_letterbox_desc;

/*
    The resulting viewport. Return value of slbx_letterbox()
*/
typedef struct slbx_viewport {
    int x;
    int y;
    int width;
    int height;
} slbx_viewport;

// compute viewport for 'letterboxing' fixed-aspect content in a variable-aspect framebuffer
SOKOL_LETTERBOX_API_DECL slbx_viewport slbx_letterbox(int width, int height, const slbx_letterbox_desc* desc);

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

slbx_viewport slbx_letterbox(int width, int height, const slbx_letterbox_desc* desc) {
    SOKOL_ASSERT((width > 0) && (height > 0));
    SOKOL_ASSERT(desc);
    const float b_top = (float)desc->border.top;
    const float b_bottom = (float)desc->border.bottom;
    const float b_left = (float)desc->border.left;
    const float b_right = (float)desc->border.right;
    float cw = (float)width - (b_left + b_right);
    if (cw < 1.0f) {
        cw = 1.0f;
    }
    float ch = (float)height - (b_top + b_bottom);
    if (ch < 1.0f) {
        ch = 1.0f;
    }
    const float content_aspect = desc->content_aspect_ratio == 0.0f ? 1.0f : desc->content_aspect_ratio;
    const float canvas_aspect = cw / ch;
    float vp_x, vp_y, vp_w, vp_h;
    if (content_aspect < canvas_aspect) {
        vp_y = b_top;
        vp_h = ch;
        vp_w = ch * content_aspect;
        if (desc->anchor == SLBX_ANCHOR_LEFT) {
            vp_x = b_left;
        } else if (desc->anchor == SLBX_ANCHOR_RIGHT) {
            vp_x = width - (b_right + vp_w);
        } else {
            vp_x = b_left + (cw - vp_w) * 0.5f;
        }
    } else {
        vp_x = b_left;
        vp_w = cw;
        vp_h = cw / content_aspect;
        if (desc->anchor == SLBX_ANCHOR_TOP) {
            vp_y = b_top;
        } else if (desc->anchor == SLBX_ANCHOR_BOTTOM) {
            vp_y = height - (b_bottom + vp_h);
        } else {
            vp_y = b_top + (ch - vp_h) * 0.5f;
        }
    }
    const slbx_viewport vp = { (int)vp_x, (int)vp_y, (int)vp_w, (int)vp_h };
    return vp;
}

#endif // SOKOL_LETTERBOX_IMPL
