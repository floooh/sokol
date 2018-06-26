#pragma once
/*
    sokol_app.h -- cross-platform application wrapper

    Portions of the OSX and Windows GL initialization and event code have been
    taken from GLFW (http://www.glfw.org/)

    Do this:
        #define SOKOL_IMPL
    before you include this file in *one* C or C++ file to create the 
    implementation.

    If you use sokol_app.h together with sokol_gfx.h, include both headers
    in the implementation source file, and include sokol_app.h before
    sokol_gfx.h since sokol_app.h will also include the required 3D-API 
    headers.

    Optionally provide the following defines with your own implementations:

    SOKOL_ASSERT(c)     - your own assert macro (default: assert(c))
    SOKOL_LOG(msg)      - your own logging function (default: puts(msg))
    SOKOL_UNREACHABLE() - a guard macro for unreachable code (default: assert(false))
    SOKOL_ABORT()       - called after an unrecoverable error (default: abort())

    Optionally define the following to force debug checks and validations
    even in release mode:

    SOKOL_DEBUG         - by default this is defined if _DEBUG is defined

    FIXME: ERROR HANDLING (this will need an error callback function)

    On Windows, a minimal 'GL header' and function loader is integrated which contains
    just enough of GL for sokol_gfx.h. If you want to use your own GL
    header-generator/loader instead, define SOKOL_WIN32_NO_GL_LOADER
    before including the implementation part of sokol_app.h.


    FEATURE OVERVIEW
    ================
    sokol_app.h provides a simple cross-platform API which implements the
    minimal 'application-wrapper' parts of a 3D application:

    - a common entry function
    - creates a window and 3D-API context/device with a 'default framebuffer'
    - makes the rendered frame visible
    - provides keyboard-, mouse- and low-level touch-events
    - platforms: MacOS, iOS, HTML5, (Win32, Linux, Android, RaspberryPi)
    - 3D-APIs: Metal, D3D11, GL3.2, GLES2, GLES3, WebGL, WebGL2

    sokol_app.h is not a whole 3D-API wrapper, but also does not depend
    on a specific 3D-API wrapper.

    HOW TO USE
    ==========
    FIXME

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
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

enum {
    SAPP_MAX_TOUCH_POINTS = 8,
    SAPP_MAX_MOUSE_BUTTONS = 3,
    SAPP_MAX_KEYCODES = 512,
};

typedef enum {
    SAPP_EVENTTYPE_INVALID,
    SAPP_EVENTTYPE_KEY_DOWN,
    SAPP_EVENTTYPE_KEY_UP,
    SAPP_EVENTTYPE_CHAR,
    SAPP_EVENTTYPE_MOUSE_DOWN,
    SAPP_EVENTTYPE_MOUSE_UP,
    SAPP_EVENTTYPE_MOUSE_SCROLL,
    SAPP_EVENTTYPE_MOUSE_MOVE,
    SAPP_EVENTTYPE_MOUSE_ENTER,
    SAPP_EVENTTYPE_MOUSE_LEAVE,
    SAPP_EVENTTYPE_TOUCHES_BEGAN,
    SAPP_EVENTTYPE_TOUCHES_MOVED,
    SAPP_EVENTTYPE_TOUCHES_ENDED,
    SAPP_EVENTTYPE_TOUCHES_CANCELLED,
    _SAPP_EVENTTYPE_NUM,
    _SAPP_EVENTTYPE_FORCE_U32 = 0x7FFFFFF
} sapp_event_type;

/* key codes are the same names and values as GLFW */
typedef enum {
    SAPP_KEYCODE_INVALID          = 0,
    SAPP_KEYCODE_SPACE            = 32,
    SAPP_KEYCODE_APOSTROPHE       = 39,  /* ' */
    SAPP_KEYCODE_COMMA            = 44,  /* , */
    SAPP_KEYCODE_MINUS            = 45,  /* - */
    SAPP_KEYCODE_PERIOD           = 46,  /* . */
    SAPP_KEYCODE_SLASH            = 47,  /* / */
    SAPP_KEYCODE_0                = 48,
    SAPP_KEYCODE_1                = 49,
    SAPP_KEYCODE_2                = 50,
    SAPP_KEYCODE_3                = 51,
    SAPP_KEYCODE_4                = 52,
    SAPP_KEYCODE_5                = 53,
    SAPP_KEYCODE_6                = 54,
    SAPP_KEYCODE_7                = 55,
    SAPP_KEYCODE_8                = 56,
    SAPP_KEYCODE_9                = 57,
    SAPP_KEYCODE_SEMICOLON        = 59,  /* ; */
    SAPP_KEYCODE_EQUAL            = 61,  /* = */
    SAPP_KEYCODE_A                = 65,
    SAPP_KEYCODE_B                = 66,
    SAPP_KEYCODE_C                = 67,
    SAPP_KEYCODE_D                = 68,
    SAPP_KEYCODE_E                = 69,
    SAPP_KEYCODE_F                = 70,
    SAPP_KEYCODE_G                = 71,
    SAPP_KEYCODE_H                = 72,
    SAPP_KEYCODE_I                = 73,
    SAPP_KEYCODE_J                = 74,
    SAPP_KEYCODE_K                = 75,
    SAPP_KEYCODE_L                = 76,
    SAPP_KEYCODE_M                = 77,
    SAPP_KEYCODE_N                = 78,
    SAPP_KEYCODE_O                = 79,
    SAPP_KEYCODE_P                = 80,
    SAPP_KEYCODE_Q                = 81,
    SAPP_KEYCODE_R                = 82,
    SAPP_KEYCODE_S                = 83,
    SAPP_KEYCODE_T                = 84,
    SAPP_KEYCODE_U                = 85,
    SAPP_KEYCODE_V                = 86,
    SAPP_KEYCODE_W                = 87,
    SAPP_KEYCODE_X                = 88,
    SAPP_KEYCODE_Y                = 89,
    SAPP_KEYCODE_Z                = 90,
    SAPP_KEYCODE_LEFT_BRACKET     = 91,  /* [ */
    SAPP_KEYCODE_BACKSLASH        = 92,  /* \ */
    SAPP_KEYCODE_RIGHT_BRACKET    = 93,  /* ] */
    SAPP_KEYCODE_GRAVE_ACCENT     = 96,  /* ` */
    SAPP_KEYCODE_WORLD_1          = 161, /* non-US #1 */
    SAPP_KEYCODE_WORLD_2          = 162, /* non-US #2 */
    SAPP_KEYCODE_ESCAPE           = 256,
    SAPP_KEYCODE_ENTER            = 257,
    SAPP_KEYCODE_TAB              = 258,
    SAPP_KEYCODE_BACKSPACE        = 259,
    SAPP_KEYCODE_INSERT           = 260,
    SAPP_KEYCODE_DELETE           = 261,
    SAPP_KEYCODE_RIGHT            = 262,
    SAPP_KEYCODE_LEFT             = 263,
    SAPP_KEYCODE_DOWN             = 264,
    SAPP_KEYCODE_UP               = 265,
    SAPP_KEYCODE_PAGE_UP          = 266,
    SAPP_KEYCODE_PAGE_DOWN        = 267,
    SAPP_KEYCODE_HOME             = 268,
    SAPP_KEYCODE_END              = 269,
    SAPP_KEYCODE_CAPS_LOCK        = 280,
    SAPP_KEYCODE_SCROLL_LOCK      = 281,
    SAPP_KEYCODE_NUM_LOCK         = 282,
    SAPP_KEYCODE_PRINT_SCREEN     = 283,
    SAPP_KEYCODE_PAUSE            = 284,
    SAPP_KEYCODE_F1               = 290,
    SAPP_KEYCODE_F2               = 291,
    SAPP_KEYCODE_F3               = 292,
    SAPP_KEYCODE_F4               = 293,
    SAPP_KEYCODE_F5               = 294,
    SAPP_KEYCODE_F6               = 295,
    SAPP_KEYCODE_F7               = 296,
    SAPP_KEYCODE_F8               = 297,
    SAPP_KEYCODE_F9               = 298,
    SAPP_KEYCODE_F10              = 299,
    SAPP_KEYCODE_F11              = 300,
    SAPP_KEYCODE_F12              = 301,
    SAPP_KEYCODE_F13              = 302,
    SAPP_KEYCODE_F14              = 303,
    SAPP_KEYCODE_F15              = 304,
    SAPP_KEYCODE_F16              = 305,
    SAPP_KEYCODE_F17              = 306,
    SAPP_KEYCODE_F18              = 307,
    SAPP_KEYCODE_F19              = 308,
    SAPP_KEYCODE_F20              = 309,
    SAPP_KEYCODE_F21              = 310,
    SAPP_KEYCODE_F22              = 311,
    SAPP_KEYCODE_F23              = 312,
    SAPP_KEYCODE_F24              = 313,
    SAPP_KEYCODE_F25              = 314,
    SAPP_KEYCODE_KP_0             = 320,
    SAPP_KEYCODE_KP_1             = 321,
    SAPP_KEYCODE_KP_2             = 322,
    SAPP_KEYCODE_KP_3             = 323,
    SAPP_KEYCODE_KP_4             = 324,
    SAPP_KEYCODE_KP_5             = 325,
    SAPP_KEYCODE_KP_6             = 326,
    SAPP_KEYCODE_KP_7             = 327,
    SAPP_KEYCODE_KP_8             = 328,
    SAPP_KEYCODE_KP_9             = 329,
    SAPP_KEYCODE_KP_DECIMAL       = 330,
    SAPP_KEYCODE_KP_DIVIDE        = 331,
    SAPP_KEYCODE_KP_MULTIPLY      = 332,
    SAPP_KEYCODE_KP_SUBTRACT      = 333,
    SAPP_KEYCODE_KP_ADD           = 334,
    SAPP_KEYCODE_KP_ENTER         = 335,
    SAPP_KEYCODE_KP_EQUAL         = 336,
    SAPP_KEYCODE_LEFT_SHIFT       = 340,
    SAPP_KEYCODE_LEFT_CONTROL     = 341,
    SAPP_KEYCODE_LEFT_ALT         = 342,
    SAPP_KEYCODE_LEFT_SUPER       = 343,
    SAPP_KEYCODE_RIGHT_SHIFT      = 344,
    SAPP_KEYCODE_RIGHT_CONTROL    = 345,
    SAPP_KEYCODE_RIGHT_ALT        = 346,
    SAPP_KEYCODE_RIGHT_SUPER      = 347,
    SAPP_KEYCODE_MENU             = 348,
} sapp_keycode; 

typedef struct {
    uintptr_t identifier;
    float pos_x;
    float pos_y;
    bool changed;
} sapp_touchpoint;

typedef enum {
    SAPP_MOUSEBUTTON_LEFT = 0,
    SAPP_MOUSEBUTTON_RIGHT = 1,
    SAPP_MOUSEBUTTON_MIDDLE = 2,
} sapp_mousebutton;

enum {
    SAPP_MODIFIER_SHIFT = (1<<0),
    SAPP_MODIFIER_CTRL = (1<<1),
    SAPP_MODIFIER_ALT = (1<<2),
    SAPP_MODIFIER_SUPER = (1<<3)
};

typedef struct {
    sapp_event_type type;
    uint32_t frame_count;
    sapp_keycode key_code;
    uint32_t char_code;
    uint32_t modifiers;
    sapp_mousebutton mouse_button;
    float mouse_x;
    float mouse_y;
    float scroll_x;
    float scroll_y;
    int num_touches;
    sapp_touchpoint touches[SAPP_MAX_TOUCH_POINTS];
} sapp_event;

typedef struct {
    void (*init_cb)(void);
    void (*frame_cb)(void);
    void (*cleanup_cb)(void);
    void (*event_cb)(const sapp_event*);
    void (*fail_cb)(const char*);
    int width;
    int height;
    int sample_count;
    bool high_dpi;
    bool fullscreen;
    bool alpha;
    bool premultiplied_alpha;
    bool preserve_drawing_buffer;
    const char* window_title;
    const char* html5_canvas_name;
    bool html5_canvas_resize;
} sapp_desc;

/* user-provided functions */
extern sapp_desc sokol_main(int argc, char* argv[]);

/* sokol_app API functions */
extern bool sapp_isvalid(void);
extern int sapp_width(void);
extern int sapp_height(void);
extern bool sapp_high_dpi(void);
extern float sapp_dpi_scale(void);

/* GL/GLES specific functions */
extern bool sapp_gles2(void);

/* Metal specific functions */
extern const void* sapp_metal_get_device(void);
extern const void* sapp_metal_get_renderpass_descriptor(void);
extern const void* sapp_metal_get_drawable(void); 

/* D3D11 specific functions */
extern const void* sapp_d3d11_get_device(void);
extern const void* sapp_d3d11_get_device_context(void);
extern const void* sapp_d3d11_get_render_target_view(void);
extern const void* sapp_d3d11_get_depth_stencil_view(void);

#ifdef __cplusplus
} /* extern "C" */
#endif

/*-- IMPLEMENTATION ----------------------------------------------------------*/
#ifdef SOKOL_IMPL

#include <string.h> /* memset */

/* check if the config defines are alright */
#if defined(__APPLE__)
    #if !__has_feature(objc_arc)
        #error "sokol_app.h requires ARC (Automatic Reference Counting) on MacOS and iOS"
    #endif
    #include <TargetConditionals.h>
    #if TARGET_OS_IPHONE
        /* iOS */
        #if !defined(SOKOL_METAL) && !defined(SOKOL_GLES3)
        #error("sokol_app.h: unknown 3D API selected for iOS, must be SOKOL_METAL or SOKOL_GLES3")
        #endif
    #else
        /* MacOS */
        #if !defined(SOKOL_METAL)
        #error("sokol_app.h: unknown 3D API selected for MacOS, must be SOKOL_METAL")
        #endif
    #endif
#elif defined(__EMSCRIPTEN__)
    /* emscripten (asm.js or wasm) */
    #if !defined(SOKOL_GLES3) && !defined(SOKOL_GLES2)
    #error("sokol_app.h: unknown 3D API selected for emscripten, must be SOKOL_GLES3 or SOKOL_GLES2")
    #endif
#elif defined(_WIN32) 
    #if !defined(SOKOL_D3D11) && !defined(SOKOL_GLCORE33)
    #error("sokol_app.h: unknown 3D API selected for Win32, must be SOKOL_D3D11 or SOKOL_GLCORE33")
    #endif
#else
#error "sokol_app.h: Unknown platform"
#endif

#ifndef SOKOL_DEBUG
    #ifdef _DEBUG
        #define SOKOL_DEBUG (1)
    #endif
#endif
#ifndef SOKOL_ASSERT
    #include <assert.h>
    #define SOKOL_ASSERT(c) assert(c)
#endif
#ifndef SOKOL_LOG
    #ifdef SOKOL_DEBUG 
        #include <stdio.h>
        #define SOKOL_LOG(s) { SOKOL_ASSERT(s); puts(s); }
    #else
        #define SOKOL_LOG(s)
    #endif
#endif
#ifndef SOKOL_ABORT
    #include <stdlib.h>
    #define SOKOL_ABORT() abort()
#endif
#ifndef _SOKOL_PRIVATE
    #if defined(__GNUC__)
        #define _SOKOL_PRIVATE __attribute__((unused)) static
    #else
        #define _SOKOL_PRIVATE static
    #endif
#endif

#ifdef __cplusplus
extern "C" {
#endif

/* helper macros */
#define _sapp_def(val, def) (((val) == 0) ? (def) : (val))
#define _sapp_absf(a) (((a)<0.0f)?-(a):(a))

enum {
    _SAPP_MAX_TITLE_LENGTH = 128,
};

typedef struct {
    bool valid;
    int window_width;
    int window_height;
    int framebuffer_width;
    int framebuffer_height;
    int sample_count;
    float dpi_scale;
    bool gles2_fallback;
    bool first_frame;
    bool init_called;
    bool html5_canvas_resize;
    const char* html5_canvas_name;
    char window_title[_SAPP_MAX_TITLE_LENGTH];      /* UTF-8 */
    wchar_t window_title_wide[_SAPP_MAX_TITLE_LENGTH];   /* UTF-32 or UCS-2 */
    uint32_t frame_count;
    float mouse_x;
    float mouse_y;
    bool win32_mouse_tracked;
    sapp_event event;
    sapp_desc desc;
    int argc;
    char** argv;
    sapp_keycode keycodes[SAPP_MAX_KEYCODES];
} _sapp_state;
static _sapp_state _sapp;

_SOKOL_PRIVATE void _sapp_fail(const char* msg) {
    if (_sapp.desc.fail_cb) {
        _sapp.desc.fail_cb(msg);
    }
    else {
        SOKOL_LOG(msg);
    }
    SOKOL_ABORT();
}

_SOKOL_PRIVATE void _sapp_strcpy(const char* src, char* dst, int max_len) {
    SOKOL_ASSERT(src && dst && (max_len > 0));
    char* const end = &(dst[max_len-1]);
    char c = 0;
    for (int i = 0; i < max_len; i++) {
        c = *src;
        if (c != 0) {
            src++;
        }
        *dst++ = c;
    }
    /* truncated? */
    if (c != 0) {
        *end = 0;
    }
}

_SOKOL_PRIVATE void _sapp_init_state(sapp_desc* desc, int argc, char* argv[]) {
    SOKOL_ASSERT(desc->init_cb);
    SOKOL_ASSERT(desc->frame_cb);
    SOKOL_ASSERT(desc->cleanup_cb);
    memset(&_sapp, 0, sizeof(_sapp));
    _sapp.argc = argc;
    _sapp.argv = argv;
    _sapp.desc = *desc;
    _sapp.first_frame = true;
    _sapp.window_width = _sapp_def(_sapp.desc.width, 640);
    _sapp.window_height = _sapp_def(_sapp.desc.height, 480);
    _sapp.framebuffer_width = _sapp.window_width;
    _sapp.framebuffer_height = _sapp.window_height;
    _sapp.sample_count = _sapp_def(_sapp.desc.sample_count, 1);
    _sapp.html5_canvas_name = _sapp_def(_sapp.desc.html5_canvas_name, "#canvas");
    _sapp.html5_canvas_resize = _sapp.desc.html5_canvas_resize;
    if (_sapp.desc.window_title) {
        _sapp_strcpy(_sapp.desc.window_title, _sapp.window_title, sizeof(_sapp.window_title));
    }
    else {
        _sapp_strcpy("sokol_app", _sapp.window_title, sizeof(_sapp.window_title));
    }
    _sapp.window_title[_SAPP_MAX_TITLE_LENGTH-1] = 0;
    _sapp.dpi_scale = 1.0f;
}

_SOKOL_PRIVATE void _sapp_init_event(sapp_event_type type) {
    memset(&_sapp.event, 0, sizeof(_sapp.event));
    _sapp.event.type = type;
    _sapp.event.frame_count = _sapp.frame_count;
}

_SOKOL_PRIVATE bool _sapp_events_enabled(void) {
    /* only send events when an event callback is set, and the init function was called */
    return _sapp.desc.event_cb && _sapp.init_called;
}

_SOKOL_PRIVATE sapp_keycode _sapp_translate_key(int scan_code) {
    if ((scan_code >= 0) && (scan_code < SAPP_MAX_KEYCODES)) {
        return _sapp.keycodes[scan_code];
    }
    else {
        return SAPP_KEYCODE_INVALID;
    }
}

_SOKOL_PRIVATE void _sapp_frame(void) {
    if (_sapp.first_frame) {
        _sapp.first_frame = false;
        _sapp.desc.init_cb();
        _sapp.init_called = true;
    }
    _sapp.desc.frame_cb();
    _sapp.frame_count++;
}

/*== MacOS/iOS ===============================================================*/

#if defined(__APPLE__)

/*== MacOS ===================================================================*/
#if !TARGET_OS_IPHONE
#import <Cocoa/Cocoa.h>
#import <Metal/Metal.h>
#import <MetalKit/MetalKit.h>

@interface _sapp_app_delegate : NSObject<NSApplicationDelegate>
@end
@interface _sapp_window_delegate : NSObject<NSWindowDelegate>
@end
@interface _sapp_mtk_view_dlg : NSObject<MTKViewDelegate>
@end
@interface _sapp_view : MTKView;
@end

static NSWindow* _sapp_window_obj;
static _sapp_window_delegate* _sapp_win_dlg_obj;
static _sapp_app_delegate* _sapp_app_dlg_obj;
static _sapp_view* _sapp_view_obj;
static _sapp_mtk_view_dlg* _sapp_mtk_view_dlg_obj;
static id<MTLDevice> _sapp_mtl_device_obj;

_SOKOL_PRIVATE void _sapp_macos_init_keytable(void) {
    _sapp.keycodes[0x1D] = SAPP_KEYCODE_0;
    _sapp.keycodes[0x12] = SAPP_KEYCODE_1;
    _sapp.keycodes[0x13] = SAPP_KEYCODE_2;
    _sapp.keycodes[0x14] = SAPP_KEYCODE_3;
    _sapp.keycodes[0x15] = SAPP_KEYCODE_4;
    _sapp.keycodes[0x17] = SAPP_KEYCODE_5;
    _sapp.keycodes[0x16] = SAPP_KEYCODE_6;
    _sapp.keycodes[0x1A] = SAPP_KEYCODE_7;
    _sapp.keycodes[0x1C] = SAPP_KEYCODE_8;
    _sapp.keycodes[0x19] = SAPP_KEYCODE_9;
    _sapp.keycodes[0x00] = SAPP_KEYCODE_A;
    _sapp.keycodes[0x0B] = SAPP_KEYCODE_B;
    _sapp.keycodes[0x08] = SAPP_KEYCODE_C;
    _sapp.keycodes[0x02] = SAPP_KEYCODE_D;
    _sapp.keycodes[0x0E] = SAPP_KEYCODE_E;
    _sapp.keycodes[0x03] = SAPP_KEYCODE_F;
    _sapp.keycodes[0x05] = SAPP_KEYCODE_G;
    _sapp.keycodes[0x04] = SAPP_KEYCODE_H;
    _sapp.keycodes[0x22] = SAPP_KEYCODE_I;
    _sapp.keycodes[0x26] = SAPP_KEYCODE_J;
    _sapp.keycodes[0x28] = SAPP_KEYCODE_K;
    _sapp.keycodes[0x25] = SAPP_KEYCODE_L;
    _sapp.keycodes[0x2E] = SAPP_KEYCODE_M;
    _sapp.keycodes[0x2D] = SAPP_KEYCODE_N;
    _sapp.keycodes[0x1F] = SAPP_KEYCODE_O;
    _sapp.keycodes[0x23] = SAPP_KEYCODE_P;
    _sapp.keycodes[0x0C] = SAPP_KEYCODE_Q;
    _sapp.keycodes[0x0F] = SAPP_KEYCODE_R;
    _sapp.keycodes[0x01] = SAPP_KEYCODE_S;
    _sapp.keycodes[0x11] = SAPP_KEYCODE_T;
    _sapp.keycodes[0x20] = SAPP_KEYCODE_U;
    _sapp.keycodes[0x09] = SAPP_KEYCODE_V;
    _sapp.keycodes[0x0D] = SAPP_KEYCODE_W;
    _sapp.keycodes[0x07] = SAPP_KEYCODE_X;
    _sapp.keycodes[0x10] = SAPP_KEYCODE_Y;
    _sapp.keycodes[0x06] = SAPP_KEYCODE_Z;
    _sapp.keycodes[0x27] = SAPP_KEYCODE_APOSTROPHE;
    _sapp.keycodes[0x2A] = SAPP_KEYCODE_BACKSLASH;
    _sapp.keycodes[0x2B] = SAPP_KEYCODE_COMMA;
    _sapp.keycodes[0x18] = SAPP_KEYCODE_EQUAL;
    _sapp.keycodes[0x32] = SAPP_KEYCODE_GRAVE_ACCENT;
    _sapp.keycodes[0x21] = SAPP_KEYCODE_LEFT_BRACKET;
    _sapp.keycodes[0x1B] = SAPP_KEYCODE_MINUS;
    _sapp.keycodes[0x2F] = SAPP_KEYCODE_PERIOD;
    _sapp.keycodes[0x1E] = SAPP_KEYCODE_RIGHT_BRACKET;
    _sapp.keycodes[0x29] = SAPP_KEYCODE_SEMICOLON;
    _sapp.keycodes[0x2C] = SAPP_KEYCODE_SLASH;
    _sapp.keycodes[0x0A] = SAPP_KEYCODE_WORLD_1;
    _sapp.keycodes[0x33] = SAPP_KEYCODE_BACKSPACE;
    _sapp.keycodes[0x39] = SAPP_KEYCODE_CAPS_LOCK;
    _sapp.keycodes[0x75] = SAPP_KEYCODE_DELETE;
    _sapp.keycodes[0x7D] = SAPP_KEYCODE_DOWN;
    _sapp.keycodes[0x77] = SAPP_KEYCODE_END;
    _sapp.keycodes[0x24] = SAPP_KEYCODE_ENTER;
    _sapp.keycodes[0x35] = SAPP_KEYCODE_ESCAPE;
    _sapp.keycodes[0x7A] = SAPP_KEYCODE_F1;
    _sapp.keycodes[0x78] = SAPP_KEYCODE_F2;
    _sapp.keycodes[0x63] = SAPP_KEYCODE_F3;
    _sapp.keycodes[0x76] = SAPP_KEYCODE_F4;
    _sapp.keycodes[0x60] = SAPP_KEYCODE_F5;
    _sapp.keycodes[0x61] = SAPP_KEYCODE_F6;
    _sapp.keycodes[0x62] = SAPP_KEYCODE_F7;
    _sapp.keycodes[0x64] = SAPP_KEYCODE_F8;
    _sapp.keycodes[0x65] = SAPP_KEYCODE_F9;
    _sapp.keycodes[0x6D] = SAPP_KEYCODE_F10;
    _sapp.keycodes[0x67] = SAPP_KEYCODE_F11;
    _sapp.keycodes[0x6F] = SAPP_KEYCODE_F12;
    _sapp.keycodes[0x69] = SAPP_KEYCODE_F13;
    _sapp.keycodes[0x6B] = SAPP_KEYCODE_F14;
    _sapp.keycodes[0x71] = SAPP_KEYCODE_F15;
    _sapp.keycodes[0x6A] = SAPP_KEYCODE_F16;
    _sapp.keycodes[0x40] = SAPP_KEYCODE_F17;
    _sapp.keycodes[0x4F] = SAPP_KEYCODE_F18;
    _sapp.keycodes[0x50] = SAPP_KEYCODE_F19;
    _sapp.keycodes[0x5A] = SAPP_KEYCODE_F20;
    _sapp.keycodes[0x73] = SAPP_KEYCODE_HOME;
    _sapp.keycodes[0x72] = SAPP_KEYCODE_INSERT;
    _sapp.keycodes[0x7B] = SAPP_KEYCODE_LEFT;
    _sapp.keycodes[0x3A] = SAPP_KEYCODE_LEFT_ALT;
    _sapp.keycodes[0x3B] = SAPP_KEYCODE_LEFT_CONTROL;
    _sapp.keycodes[0x38] = SAPP_KEYCODE_LEFT_SHIFT;
    _sapp.keycodes[0x37] = SAPP_KEYCODE_LEFT_SUPER;
    _sapp.keycodes[0x6E] = SAPP_KEYCODE_MENU;
    _sapp.keycodes[0x47] = SAPP_KEYCODE_NUM_LOCK;
    _sapp.keycodes[0x79] = SAPP_KEYCODE_PAGE_DOWN;
    _sapp.keycodes[0x74] = SAPP_KEYCODE_PAGE_UP;
    _sapp.keycodes[0x7C] = SAPP_KEYCODE_RIGHT;
    _sapp.keycodes[0x3D] = SAPP_KEYCODE_RIGHT_ALT;
    _sapp.keycodes[0x3E] = SAPP_KEYCODE_RIGHT_CONTROL;
    _sapp.keycodes[0x3C] = SAPP_KEYCODE_RIGHT_SHIFT;
    _sapp.keycodes[0x36] = SAPP_KEYCODE_RIGHT_SUPER;
    _sapp.keycodes[0x31] = SAPP_KEYCODE_SPACE;
    _sapp.keycodes[0x30] = SAPP_KEYCODE_TAB;
    _sapp.keycodes[0x7E] = SAPP_KEYCODE_UP;
    _sapp.keycodes[0x52] = SAPP_KEYCODE_KP_0;
    _sapp.keycodes[0x53] = SAPP_KEYCODE_KP_1;
    _sapp.keycodes[0x54] = SAPP_KEYCODE_KP_2;
    _sapp.keycodes[0x55] = SAPP_KEYCODE_KP_3;
    _sapp.keycodes[0x56] = SAPP_KEYCODE_KP_4;
    _sapp.keycodes[0x57] = SAPP_KEYCODE_KP_5;
    _sapp.keycodes[0x58] = SAPP_KEYCODE_KP_6;
    _sapp.keycodes[0x59] = SAPP_KEYCODE_KP_7;
    _sapp.keycodes[0x5B] = SAPP_KEYCODE_KP_8;
    _sapp.keycodes[0x5C] = SAPP_KEYCODE_KP_9;
    _sapp.keycodes[0x45] = SAPP_KEYCODE_KP_ADD;
    _sapp.keycodes[0x41] = SAPP_KEYCODE_KP_DECIMAL;
    _sapp.keycodes[0x4B] = SAPP_KEYCODE_KP_DIVIDE;
    _sapp.keycodes[0x4C] = SAPP_KEYCODE_KP_ENTER;
    _sapp.keycodes[0x51] = SAPP_KEYCODE_KP_EQUAL;
    _sapp.keycodes[0x43] = SAPP_KEYCODE_KP_MULTIPLY;
    _sapp.keycodes[0x4E] = SAPP_KEYCODE_KP_SUBTRACT;
}

/* MacOS entry function */
int main(int argc, char* argv[]) {
    sapp_desc desc = sokol_main(argc, argv);
    _sapp_init_state(&desc, argc, argv);
    _sapp_macos_init_keytable();
    [NSApplication sharedApplication];
    [NSApp setActivationPolicy:NSApplicationActivationPolicyRegular];
    _sapp_app_dlg_obj = [[_sapp_app_delegate alloc] init];
    [NSApp setDelegate:_sapp_app_dlg_obj];
    [NSApp activateIgnoringOtherApps:YES];
    [NSApp run];
    return 0;
}

_SOKOL_PRIVATE void _sapp_macos_frame(void) {
    const CGSize fb_size = [_sapp_view_obj drawableSize];
    _sapp.framebuffer_width = fb_size.width;
    _sapp.framebuffer_height = fb_size.height;
    const NSRect bounds = [_sapp_view_obj bounds];
    _sapp.window_width = bounds.size.width;
    _sapp.window_height = bounds.size.height;
    SOKOL_ASSERT((_sapp.framebuffer_width > 0) && (_sapp.framebuffer_height > 0));
    _sapp.dpi_scale = (float)_sapp.framebuffer_width / (float)_sapp.window_width;
    const NSPoint mouse_pos = [_sapp_window_obj mouseLocationOutsideOfEventStream];
    _sapp.mouse_x = mouse_pos.x * _sapp.dpi_scale;
    _sapp.mouse_y = _sapp.framebuffer_height - (mouse_pos.y * _sapp.dpi_scale) - 1;
    _sapp_frame();
}

@implementation _sapp_app_delegate
- (void)applicationDidFinishLaunching:(NSNotification*)aNotification {
    const NSUInteger style =
        NSWindowStyleMaskTitled |
        NSWindowStyleMaskClosable |
        NSWindowStyleMaskMiniaturizable |
        NSWindowStyleMaskResizable;
    _sapp_window_obj = [[NSWindow alloc]
        initWithContentRect:NSMakeRect(0, 0, _sapp.window_width, _sapp.window_height)
        styleMask:style
        backing:NSBackingStoreBuffered
        defer:NO];
    [_sapp_window_obj setTitle:[NSString stringWithUTF8String:_sapp.window_title]];
    [_sapp_window_obj setAcceptsMouseMovedEvents:YES];
    [_sapp_window_obj center];
    [_sapp_window_obj setRestorable:YES];
    _sapp_win_dlg_obj = [[_sapp_window_delegate alloc] init];
    [_sapp_window_obj setDelegate:_sapp_win_dlg_obj];
    _sapp_mtl_device_obj = MTLCreateSystemDefaultDevice();
    _sapp_mtk_view_dlg_obj = [[_sapp_mtk_view_dlg alloc] init];
    _sapp_view_obj = [[_sapp_view alloc] init];
    [_sapp_view_obj setPreferredFramesPerSecond:60];
    [_sapp_view_obj setDelegate:_sapp_mtk_view_dlg_obj];
    [_sapp_view_obj setDevice:_sapp_mtl_device_obj];
    [_sapp_view_obj setColorPixelFormat:MTLPixelFormatBGRA8Unorm];
    [_sapp_view_obj setDepthStencilPixelFormat:MTLPixelFormatDepth32Float_Stencil8];
    [_sapp_view_obj setSampleCount:_sapp.sample_count];
    [_sapp_window_obj setContentView:_sapp_view_obj];
    [_sapp_window_obj makeFirstResponder:_sapp_view_obj];
    if (!_sapp.desc.high_dpi) {
        CGSize drawable_size = { (CGFloat) _sapp.framebuffer_width, (CGFloat) _sapp.framebuffer_height };
        [_sapp_view_obj setDrawableSize:drawable_size];
    }
    CGSize drawable_size = _sapp_view_obj.drawableSize;
    _sapp.framebuffer_width = drawable_size.width;
    _sapp.framebuffer_height = drawable_size.height;
    SOKOL_ASSERT((_sapp.framebuffer_width > 0) && (_sapp.framebuffer_height > 0));
    _sapp.dpi_scale = (float)_sapp.framebuffer_width / (float)_sapp.window_width;
    [[_sapp_view_obj layer] setMagnificationFilter:kCAFilterNearest];
    [_sapp_window_obj makeKeyAndOrderFront:nil];
    _sapp.valid = true;
}

- (BOOL)applicationShouldTerminateAfterLastWindowClosed:(NSApplication*)sender {
    return YES;
}
@end

@implementation _sapp_window_delegate
- (BOOL)windowShouldClose:(id)sender {
    _sapp.desc.cleanup_cb();
    return YES;
}

- (void)windowDidMiniaturize:(NSNotification*)notification {
    /* FIXME */
}

- (void)windowDidDeminiaturize:(NSNotification*)notification {
    /* FIXME */
}

- (void)windowDidBecomeKey:(NSNotification*)notification {
    /* FIXME */
}

- (void)windowDidResignKey:(NSNotification*)notification {
    /* FIXME */
}
@end

@implementation _sapp_mtk_view_dlg
- (void)drawInMTKView:(MTKView*)view {
    @autoreleasepool {
        _sapp_macos_frame();
    }
}
- (void)mtkView:(MTKView*)view drawableSizeWillChange:(CGSize)size {
    /* this is required by the protocol, but we can't do anything useful here */
}
@end

_SOKOL_PRIVATE uint32_t _sapp_macos_mod(NSEventModifierFlags f) {
    uint32_t m = 0;
    if (f & NSEventModifierFlagShift) {
        m |= SAPP_MODIFIER_SHIFT;
    }
    if (f & NSEventModifierFlagControl) {
        m |= SAPP_MODIFIER_CTRL;
    }
    if (f & NSEventModifierFlagOption) {
        m |= SAPP_MODIFIER_ALT;
    }
    if (f & NSEventModifierFlagCommand) {
        m |= SAPP_MODIFIER_SUPER;
    }
    return m;
}

_SOKOL_PRIVATE void _sapp_macos_mouse_event(sapp_event_type type, sapp_mousebutton btn, uint32_t mod) {
    if (_sapp_events_enabled() && (btn < SAPP_MAX_MOUSE_BUTTONS)) {
        _sapp_init_event(type);
        _sapp.event.mouse_button = btn;
        _sapp.event.modifiers = mod;
        _sapp.event.mouse_x = _sapp.mouse_x;
        _sapp.event.mouse_y = _sapp.mouse_y;
        _sapp.desc.event_cb(&_sapp.event);
    }
}

_SOKOL_PRIVATE void _sapp_macos_key_event(sapp_event_type type, sapp_keycode key, uint32_t mod) {
    if (_sapp_events_enabled()) {
        _sapp_init_event(type);
        _sapp.event.key_code = key;
        _sapp.event.modifiers = mod;
        _sapp.desc.event_cb(&_sapp.event);
    }
}

@implementation _sapp_view
- (BOOL)isOpaque {
    return YES;
}
- (BOOL)canBecomeKey {
    return YES;
}
- (BOOL)acceptsFirstResponder {
    return YES;
}
- (void)mouseDown:(NSEvent*)event {
    _sapp_macos_mouse_event(SAPP_EVENTTYPE_MOUSE_DOWN, SAPP_MOUSEBUTTON_LEFT, _sapp_macos_mod(event.modifierFlags));
}
- (void)mouseUp:(NSEvent*)event {
    _sapp_macos_mouse_event(SAPP_EVENTTYPE_MOUSE_UP, SAPP_MOUSEBUTTON_LEFT, _sapp_macos_mod(event.modifierFlags));
}
- (void)rightMouseDown:(NSEvent*)event {
    _sapp_macos_mouse_event(SAPP_EVENTTYPE_MOUSE_DOWN, SAPP_MOUSEBUTTON_RIGHT, _sapp_macos_mod(event.modifierFlags));
}
- (void)rightMouseUp:(NSEvent*)event {
    _sapp_macos_mouse_event(SAPP_EVENTTYPE_MOUSE_UP, SAPP_MOUSEBUTTON_RIGHT, _sapp_macos_mod(event.modifierFlags));
}
- (void)mouseMoved:(NSEvent*)event {
    _sapp_macos_mouse_event(SAPP_EVENTTYPE_MOUSE_MOVE, 0, _sapp_macos_mod(event.modifierFlags));
}
- (void)mouseDragged:(NSEvent*)event {
    _sapp_macos_mouse_event(SAPP_EVENTTYPE_MOUSE_MOVE, 0, _sapp_macos_mod(event.modifierFlags));
}
- (void)rightMouseDragged:(NSEvent*)event {
    _sapp_macos_mouse_event(SAPP_EVENTTYPE_MOUSE_MOVE, 0, _sapp_macos_mod(event.modifierFlags));
}
- (void)scrollWheel:(NSEvent*)event {
    if (_sapp_events_enabled()) {
        float dx = (float) event.scrollingDeltaX;
        float dy = (float) event.scrollingDeltaY;
        if (event.hasPreciseScrollingDeltas) {
            dx *= 0.1;
            dy *= 0.1;
        }
        if ((_sapp_absf(dx) > 0.0f) || (_sapp_absf(dy) > 0.0f)) {
            _sapp_init_event(SAPP_EVENTTYPE_MOUSE_SCROLL);
            _sapp.event.modifiers = _sapp_macos_mod(event.modifierFlags);
            _sapp.event.mouse_x = _sapp.mouse_x;
            _sapp.event.mouse_y = _sapp.mouse_y;
            _sapp.event.scroll_x = dx;
            _sapp.event.scroll_y = dy;
            _sapp.desc.event_cb(&_sapp.event);
        }
    }
}
- (void)keyDown:(NSEvent*)event {
    if (_sapp_events_enabled()) {
        const uint32_t mods = _sapp_macos_mod(event.modifierFlags);
        _sapp_macos_key_event(SAPP_EVENTTYPE_KEY_DOWN, _sapp_translate_key(event.keyCode), mods);
        const NSString* chars = event.characters;
        const NSUInteger len = chars.length;
        if (len > 0) {
            _sapp_init_event(SAPP_EVENTTYPE_CHAR);
            _sapp.event.modifiers = mods;
            for (NSUInteger i = 0; i < len; i++) {
                const unichar codepoint = [chars characterAtIndex:i];
                if ((codepoint & 0xFF00) == 0xF700) {
                    continue;
                }
                _sapp.event.char_code = codepoint;
                _sapp.desc.event_cb(&_sapp.event);
            }
        }
    }
}
- (void)keyUp:(NSEvent*)event {
    _sapp_macos_key_event(SAPP_EVENTTYPE_KEY_UP, 
        _sapp_translate_key(event.keyCode), 
        _sapp_macos_mod(event.modifierFlags));
}
- (void)flagsChanged:(NSEvent*)event {
    /* FIXME */
}
@end

#endif /* MacOS */

/*== iOS =====================================================================*/
#if TARGET_OS_IPHONE
#import <UIKit/UIKit.h>
#if defined(SOKOL_METAL)
#import <Metal/Metal.h>
#import <MetalKit/MetalKit.h>
#else
#import <GLKit/GLKit.h>
#include <OpenGLES/ES3/gl.h>
#include <OpenGLES/ES3/glext.h>
#endif

@interface _sapp_app_delegate : NSObject<UIApplicationDelegate>
@end
#if defined(SOKOL_METAL)
@interface _sapp_mtk_view_dlg : NSObject<MTKViewDelegate>
@end
@interface _sapp_view : MTKView;
@end
#else
@interface _sapp_glk_view_dlg : NSObject<GLKViewDelegate>
@end
@interface _sapp_view : GLKView
@end
#endif

static UIWindow* _sapp_window_obj;
static _sapp_view* _sapp_view_obj;
#if defined(SOKOL_METAL)
static _sapp_mtk_view_dlg* _sapp_mtk_view_dlg_obj;
static UIViewController<MTKViewDelegate>* _sapp_mtk_view_ctrl_obj;
static id<MTLDevice> _sapp_mtl_device_obj;
#else
static EAGLContext* _sapp_eagl_ctx_obj;
static _sapp_glk_view_dlg* _sapp_glk_view_dlg_obj;
static GLKViewController* _sapp_glk_view_ctrl_obj;
#endif

/* iOS entry function */
int main(int argc, char** argv) {
    @autoreleasepool {
        sapp_desc desc = sokol_main(argc, argv);
        _sapp_init_state(&desc, argc, argv);
        UIApplicationMain(argc, argv, nil, NSStringFromClass([_sapp_app_delegate class]));
    }
    return 0;
}

_SOKOL_PRIVATE void _sapp_ios_frame(void) {
    CGRect screen_rect = [[UIScreen mainScreen] bounds];
    _sapp.window_width = (int) screen_rect.size.width;
    _sapp.window_height = (int) screen_rect.size.height;
    #if defined(SOKOL_METAL)
        const CGSize fb_size = _sapp_view_obj.drawableSize;
        _sapp.framebuffer_width = fb_size.width;
        _sapp.framebuffer_height = fb_size.height;
    #else
        _sapp.framebuffer_width = (int) _sapp_view_obj.drawableWidth;
        _sapp.framebuffer_height = (int) _sapp_view_obj.drawableHeight;
    #endif
    SOKOL_ASSERT((_sapp.framebuffer_width > 0) && (_sapp.framebuffer_height > 0));
    _sapp.dpi_scale = (float)_sapp.framebuffer_width / (float) _sapp.window_width;
    _sapp_frame();
}

@implementation _sapp_app_delegate
- (BOOL)application:(UIApplication*)application didFinishLaunchingWithOptions:(NSDictionary*)launchOptions {
    CGRect screen_rect = [[UIScreen mainScreen] bounds];
    _sapp_window_obj = [[UIWindow alloc] initWithFrame:screen_rect];
    _sapp.window_width = screen_rect.size.width;
    _sapp.window_height = screen_rect.size.height;
    if (_sapp.desc.high_dpi) {
        _sapp.framebuffer_width = 2 * _sapp.window_width;
        _sapp.framebuffer_height = 2 * _sapp.window_height;
    }
    else {
        _sapp.framebuffer_width = _sapp.window_width;
        _sapp.framebuffer_height = _sapp.window_height;
    }
    _sapp.dpi_scale = (float)_sapp.framebuffer_width / (float) _sapp.window_width;
    #if defined(SOKOL_METAL)
        _sapp_mtl_device_obj = MTLCreateSystemDefaultDevice();
        _sapp_mtk_view_dlg_obj = [[_sapp_mtk_view_dlg alloc] init];
        _sapp_view_obj = [[_sapp_view alloc] init];
        [_sapp_view_obj setPreferredFramesPerSecond:60];
        [_sapp_view_obj setDelegate:_sapp_mtk_view_dlg_obj];
        [_sapp_view_obj setDevice:_sapp_mtl_device_obj];
        [_sapp_view_obj setColorPixelFormat:MTLPixelFormatBGRA8Unorm];
        [_sapp_view_obj setDepthStencilPixelFormat:MTLPixelFormatDepth32Float_Stencil8];
        [_sapp_view_obj setSampleCount:_sapp.sample_count];
        if (_sapp.desc.high_dpi) {
            [_sapp_view_obj setContentScaleFactor:2.0];
        }
        else {
            [_sapp_view_obj setContentScaleFactor:1.0];
        }
        [_sapp_view_obj setUserInteractionEnabled:YES];
        [_sapp_view_obj setMultipleTouchEnabled:YES];
        [_sapp_window_obj addSubview:_sapp_view_obj];
        _sapp_mtk_view_ctrl_obj = [[UIViewController<MTKViewDelegate> alloc] init];
        [_sapp_mtk_view_ctrl_obj setView:_sapp_view_obj];
        [_sapp_window_obj setRootViewController:_sapp_mtk_view_ctrl_obj];
    #else
        _sapp_eagl_ctx_obj = [[EAGLContext alloc] initWithAPI:kEAGLRenderingAPIOpenGLES3];
        if (_sapp_eagl_ctx_obj == nil) {
            _sapp_eagl_ctx_obj = [[EAGLContext alloc] initWithAPI:kEAGLRenderingAPIOpenGLES2];
            _sapp.gles2_fallback = true;
        }
        _sapp_glk_view_dlg_obj = [[_sapp_glk_view_dlg alloc] init];
        _sapp_view_obj = [[_sapp_view alloc] initWithFrame:screen_rect];
        [_sapp_view_obj setDrawableColorFormat:GLKViewDrawableColorFormatRGBA8888];
        [_sapp_view_obj setDrawableDepthFormat:GLKViewDrawableDepthFormat24];
        [_sapp_view_obj setDrawableStencilFormat:GLKViewDrawableStencilFormatNone];
        [_sapp_view_obj setDrawableMultisample:GLKViewDrawableMultisampleNone]; /* FIXME */
        [_sapp_view_obj setContext:_sapp_eagl_ctx_obj];
        [_sapp_view_obj setDelegate:_sapp_glk_view_dlg_obj];
        [_sapp_view_obj setEnableSetNeedsDisplay:NO];
        [_sapp_view_obj setUserInteractionEnabled:YES];
        [_sapp_view_obj setMultipleTouchEnabled:YES];
        if (_sapp.desc.high_dpi) {
            [_sapp_view_obj setContentScaleFactor:2.0];
        }
        else {
            [_sapp_view_obj setContentScaleFactor:1.0];
        }
        [_sapp_window_obj addSubview:_sapp_view_obj];
        _sapp_glk_view_ctrl_obj = [[GLKViewController alloc] init];
        [_sapp_glk_view_ctrl_obj setView:_sapp_view_obj];
        [_sapp_glk_view_ctrl_obj setPreferredFramesPerSecond:60];
        [_sapp_window_obj setRootViewController:_sapp_glk_view_ctrl_obj];
    #endif
    [_sapp_window_obj makeKeyAndVisible];
    _sapp.valid = true;
    return YES;
}
@end

#if defined(SOKOL_METAL)
@implementation _sapp_mtk_view_dlg
- (void)drawInMTKView:(MTKView*)view {
    @autoreleasepool {
        _sapp_ios_frame();
    }
}

- (void)mtkView:(MTKView*)view drawableSizeWillChange:(CGSize)size {
    /* this is required by the protocol, but we can't do anything useful here */
}
@end
#else
@implementation _sapp_glk_view_dlg
- (void)glkView:(GLKView*)view drawInRect:(CGRect)rect {
    @autoreleasepool {
        _sapp_ios_frame();
    }
}
@end
#endif

_SOKOL_PRIVATE void _sapp_ios_touch_event(sapp_event_type type, NSSet<UITouch *>* touches, UIEvent* event) {
    if (_sapp_events_enabled()) {
        _sapp_init_event(type);
        NSEnumerator* enumerator = [[event allTouches] objectEnumerator];
        UITouch* ios_touch;
        while ((ios_touch = [enumerator nextObject])) {
            if ((_sapp.event.num_touches + 1) < SAPP_MAX_TOUCH_POINTS) {
                CGPoint ios_pos = [ios_touch locationInView:_sapp_view_obj];
                sapp_touchpoint* cur_point = &_sapp.event.touches[_sapp.event.num_touches++];
                cur_point->identifier = (uintptr_t) ios_touch;
                cur_point->pos_x = ios_pos.x * _sapp.dpi_scale;
                cur_point->pos_y = ios_pos.y * _sapp.dpi_scale;
                cur_point->changed = [touches containsObject:ios_touch];
            }
        }
        if (_sapp.event.num_touches > 0) {
            _sapp.desc.event_cb(&_sapp.event);
        }
    }
}

@implementation _sapp_view
- (BOOL) isOpaque {
    return YES;
}
- (void)touchesBegan:(NSSet<UITouch *> *)touches withEvent:(UIEvent*)event {
    _sapp_ios_touch_event(SAPP_EVENTTYPE_TOUCHES_BEGAN, touches, event);
}
- (void)touchesMoved:(NSSet<UITouch *> *)touches withEvent:(UIEvent*)event {
    _sapp_ios_touch_event(SAPP_EVENTTYPE_TOUCHES_MOVED, touches, event);
}
- (void)touchesEnded:(NSSet<UITouch *> *)touches withEvent:(UIEvent*)event {
    _sapp_ios_touch_event(SAPP_EVENTTYPE_TOUCHES_ENDED, touches, event);
}
- (void)touchesCancelled:(NSSet<UITouch *> *)touches withEvent:(UIEvent*)event {
    _sapp_ios_touch_event(SAPP_EVENTTYPE_TOUCHES_CANCELLED, touches, event);
}
@end
#endif /* TARGET_OS_IPHONE */

#endif /* __APPLE__ */

/*== EMSCRIPTEN ==============================================================*/
#if defined(__EMSCRIPTEN__)
#if defined(SOKOL_GLES3)
#include <GLES3/gl3.h>
#else
#ifndef GL_EXT_PROTOTYPES 
#define GL_GLEXT_PROTOTYPES
#endif
#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>
#endif
#include <emscripten/emscripten.h>
#include <emscripten/html5.h>

_SOKOL_PRIVATE EM_BOOL _sapp_emsc_size_changed(int event_type, const EmscriptenUiEvent* ui_event, void* user_data) {
    double w, h;
    emscripten_get_element_css_size(_sapp.html5_canvas_name, &w, &h);
    _sapp.window_width = (int) w;
    _sapp.window_height = (int) h;
    if (_sapp.desc.high_dpi) {
        _sapp.dpi_scale = emscripten_get_device_pixel_ratio();
        w *= _sapp.dpi_scale;
        h *= _sapp.dpi_scale;
    }
    _sapp.framebuffer_width = (int) w;
    _sapp.framebuffer_height = (int) h;
    SOKOL_ASSERT((_sapp.framebuffer_width > 0) && (_sapp.framebuffer_height > 0));
    emscripten_set_canvas_element_size(_sapp.html5_canvas_name, w, h);
    return true;
}

_SOKOL_PRIVATE void _sapp_emsc_frame(void) {
    _sapp_frame();
}

_SOKOL_PRIVATE EM_BOOL _sapp_emsc_mouse_cb(int emsc_type, const EmscriptenMouseEvent* emsc_event, void* user_data) {
    _sapp.mouse_x = (emsc_event->canvasX * _sapp.dpi_scale);
    _sapp.mouse_y = (emsc_event->canvasY * _sapp.dpi_scale);
    if (_sapp_events_enabled() && (emsc_event->button < SAPP_MAX_MOUSE_BUTTONS)) {
        sapp_event_type type;
        switch (emsc_type) {
            case EMSCRIPTEN_EVENT_MOUSEDOWN:
                type = SAPP_EVENTTYPE_MOUSE_DOWN;
                break;
            case EMSCRIPTEN_EVENT_MOUSEUP:
                type = SAPP_EVENTTYPE_MOUSE_UP;
                break;
            case EMSCRIPTEN_EVENT_MOUSEMOVE:
                type = SAPP_EVENTTYPE_MOUSE_MOVE;
                break;
            case EMSCRIPTEN_EVENT_MOUSEENTER:
                type = SAPP_EVENTTYPE_MOUSE_ENTER;
                break;
            case EMSCRIPTEN_EVENT_MOUSELEAVE:
                type = SAPP_EVENTTYPE_MOUSE_LEAVE;
                break;
            default:
                type = SAPP_EVENTTYPE_INVALID;
                break;
        }
        if (type != SAPP_EVENTTYPE_INVALID) {
            _sapp_init_event(type);
            if (emsc_event->ctrlKey) {
                _sapp.event.modifiers |= SAPP_MODIFIER_CTRL;
            }
            if (emsc_event->shiftKey) {
                _sapp.event.modifiers |= SAPP_MODIFIER_SHIFT;
            }
            if (emsc_event->altKey) {
                _sapp.event.modifiers |= SAPP_MODIFIER_ALT;
            }
            if (emsc_event->metaKey) {
                _sapp.event.modifiers |= SAPP_MODIFIER_SUPER;
            }
            switch (emsc_event->button) {
                case 0: _sapp.event.mouse_button = SAPP_MOUSEBUTTON_LEFT; break;
                case 1: _sapp.event.mouse_button = SAPP_MOUSEBUTTON_MIDDLE; break;
                case 2: _sapp.event.mouse_button = SAPP_MOUSEBUTTON_RIGHT; break;
                default: _sapp.event.mouse_button = emsc_event->button; break;
            }
            _sapp.event.mouse_x = _sapp.mouse_x;
            _sapp.event.mouse_y = _sapp.mouse_y;
            _sapp.desc.event_cb(&_sapp.event);
        }
    }
    return true;
}

_SOKOL_PRIVATE EM_BOOL _sapp_emsc_wheel_cb(int emsc_type, const EmscriptenWheelEvent* emsc_event, void* user_data) {
    if (_sapp_events_enabled()) {
        _sapp_init_event(SAPP_EVENTTYPE_MOUSE_SCROLL);
        if (emsc_event->mouse.ctrlKey) {
            _sapp.event.modifiers |= SAPP_MODIFIER_CTRL;
        }
        if (emsc_event->mouse.shiftKey) {
            _sapp.event.modifiers |= SAPP_MODIFIER_SHIFT;
        }
        if (emsc_event->mouse.altKey) {
            _sapp.event.modifiers |= SAPP_MODIFIER_ALT;
        }
        if (emsc_event->mouse.metaKey) {
            _sapp.event.modifiers |= SAPP_MODIFIER_SUPER;
        }
        _sapp.event.scroll_x = -0.1 * (float)emsc_event->deltaX;
        _sapp.event.scroll_y = -0.1 * (float)emsc_event->deltaY;
        _sapp.desc.event_cb(&_sapp.event);
    }
    return true;
}

_SOKOL_PRIVATE EM_BOOL _sapp_emsc_key_cb(int emsc_type, const EmscriptenKeyboardEvent* emsc_event, void* user_data) {
    bool retval = true;
    if (_sapp_events_enabled()) {
        sapp_event_type type;
        switch (emsc_type) {
            case EMSCRIPTEN_EVENT_KEYDOWN:
                type = SAPP_EVENTTYPE_KEY_DOWN;
                break;
            case EMSCRIPTEN_EVENT_KEYUP:
                type = SAPP_EVENTTYPE_KEY_UP;
                break;
            case EMSCRIPTEN_EVENT_KEYPRESS:
                type = SAPP_EVENTTYPE_CHAR;
                break;
            default:
                type = SAPP_EVENTTYPE_INVALID;
                break; 
        }
        if (type != SAPP_EVENTTYPE_INVALID) {
            _sapp_init_event(type);
            if (emsc_event->ctrlKey) {
                _sapp.event.modifiers |= SAPP_MODIFIER_CTRL;
            }
            if (emsc_event->shiftKey) {
                _sapp.event.modifiers |= SAPP_MODIFIER_SHIFT;
            }
            if (emsc_event->altKey) {
                _sapp.event.modifiers |= SAPP_MODIFIER_ALT;
            }
            if (emsc_event->metaKey) {
                _sapp.event.modifiers |= SAPP_MODIFIER_SUPER;
            }
            if (type == SAPP_EVENTTYPE_CHAR) {
                _sapp.event.char_code = emsc_event->charCode;
            }
            else {
                _sapp.event.key_code = _sapp_translate_key(emsc_event->keyCode);
                /* only forward alpha-numeric keys to browser */
                retval = emsc_event->keyCode < 32;
            }
            _sapp.desc.event_cb(&_sapp.event);
        }
    }
    return retval;
}

_SOKOL_PRIVATE EM_BOOL _sapp_emsc_touch_cb(int emsc_type, const EmscriptenTouchEvent* emsc_event, void* user_data) {
    bool retval = true;
    if (_sapp_events_enabled()) {
        sapp_event_type type;
        switch (emsc_type) {
            case EMSCRIPTEN_EVENT_TOUCHSTART:
                type = SAPP_EVENTTYPE_TOUCHES_BEGAN;
                break;
            case EMSCRIPTEN_EVENT_TOUCHMOVE:
                type = SAPP_EVENTTYPE_TOUCHES_MOVED;
                break;
            case EMSCRIPTEN_EVENT_TOUCHEND:
                type = SAPP_EVENTTYPE_TOUCHES_ENDED;
                break;
            case EMSCRIPTEN_EVENT_TOUCHCANCEL:
                type = SAPP_EVENTTYPE_TOUCHES_CANCELLED;
                break;
            default:
                type = SAPP_EVENTTYPE_INVALID;
                retval = false;
                break;
        }
        if (type != SAPP_EVENTTYPE_INVALID) {
            _sapp_init_event(type);
            if (emsc_event->ctrlKey) {
                _sapp.event.modifiers |= SAPP_MODIFIER_CTRL;
            }
            if (emsc_event->shiftKey) {
                _sapp.event.modifiers |= SAPP_MODIFIER_SHIFT;
            }
            if (emsc_event->altKey) {
                _sapp.event.modifiers |= SAPP_MODIFIER_ALT;
            }
            if (emsc_event->metaKey) {
                _sapp.event.modifiers |= SAPP_MODIFIER_SUPER;
            }
            _sapp.event.num_touches = emsc_event->numTouches;
            if (_sapp.event.num_touches > SAPP_MAX_TOUCH_POINTS) {
                _sapp.event.num_touches = SAPP_MAX_TOUCH_POINTS;
            }
            for (int i = 0; i < _sapp.event.num_touches; i++) {
                const EmscriptenTouchPoint* src = &emsc_event->touches[i];
                sapp_touchpoint* dst = &_sapp.event.touches[i];
                dst->identifier = src->identifier;
                dst->pos_x = src->canvasX * _sapp.dpi_scale;
                dst->pos_y = src->canvasY * _sapp.dpi_scale;
                dst->changed = src->isChanged;
            }
            _sapp.desc.event_cb(&_sapp.event);
        }
    }
    return retval;
}

_SOKOL_PRIVATE void _sapp_emsc_init_keytable(void) {
    _sapp.keycodes[8]   = SAPP_KEYCODE_BACKSPACE;
    _sapp.keycodes[9]   = SAPP_KEYCODE_TAB;
    _sapp.keycodes[13]  = SAPP_KEYCODE_ENTER;
    _sapp.keycodes[16]  = SAPP_KEYCODE_LEFT_SHIFT;
    _sapp.keycodes[17]  = SAPP_KEYCODE_LEFT_CONTROL;
    _sapp.keycodes[18]  = SAPP_KEYCODE_LEFT_ALT;
    _sapp.keycodes[19]  = SAPP_KEYCODE_PAUSE;
    _sapp.keycodes[27]  = SAPP_KEYCODE_ESCAPE;
    _sapp.keycodes[32]  = SAPP_KEYCODE_SPACE;
    _sapp.keycodes[33]  = SAPP_KEYCODE_PAGE_UP;
    _sapp.keycodes[34]  = SAPP_KEYCODE_PAGE_DOWN;
    _sapp.keycodes[35]  = SAPP_KEYCODE_END;
    _sapp.keycodes[36]  = SAPP_KEYCODE_HOME;
    _sapp.keycodes[37]  = SAPP_KEYCODE_LEFT;
    _sapp.keycodes[38]  = SAPP_KEYCODE_UP;
    _sapp.keycodes[39]  = SAPP_KEYCODE_RIGHT;
    _sapp.keycodes[40]  = SAPP_KEYCODE_DOWN;
    _sapp.keycodes[45]  = SAPP_KEYCODE_INSERT;
    _sapp.keycodes[46]  = SAPP_KEYCODE_DELETE;
    _sapp.keycodes[48]  = SAPP_KEYCODE_0;
    _sapp.keycodes[49]  = SAPP_KEYCODE_1;
    _sapp.keycodes[50]  = SAPP_KEYCODE_2;
    _sapp.keycodes[51]  = SAPP_KEYCODE_3;
    _sapp.keycodes[52]  = SAPP_KEYCODE_4;
    _sapp.keycodes[53]  = SAPP_KEYCODE_5;
    _sapp.keycodes[54]  = SAPP_KEYCODE_6;
    _sapp.keycodes[55]  = SAPP_KEYCODE_7;
    _sapp.keycodes[56]  = SAPP_KEYCODE_8;
    _sapp.keycodes[57]  = SAPP_KEYCODE_9;
    _sapp.keycodes[59]  = SAPP_KEYCODE_SEMICOLON;
    _sapp.keycodes[64]  = SAPP_KEYCODE_EQUAL;
    _sapp.keycodes[65]  = SAPP_KEYCODE_A;
    _sapp.keycodes[66]  = SAPP_KEYCODE_B;
    _sapp.keycodes[67]  = SAPP_KEYCODE_C;
    _sapp.keycodes[68]  = SAPP_KEYCODE_D;
    _sapp.keycodes[69]  = SAPP_KEYCODE_E;
    _sapp.keycodes[70]  = SAPP_KEYCODE_F;
    _sapp.keycodes[71]  = SAPP_KEYCODE_G;
    _sapp.keycodes[72]  = SAPP_KEYCODE_H;
    _sapp.keycodes[73]  = SAPP_KEYCODE_I;
    _sapp.keycodes[74]  = SAPP_KEYCODE_J;
    _sapp.keycodes[75]  = SAPP_KEYCODE_K;
    _sapp.keycodes[76]  = SAPP_KEYCODE_L;
    _sapp.keycodes[77]  = SAPP_KEYCODE_M;
    _sapp.keycodes[78]  = SAPP_KEYCODE_N;
    _sapp.keycodes[79]  = SAPP_KEYCODE_O;
    _sapp.keycodes[80]  = SAPP_KEYCODE_P;
    _sapp.keycodes[81]  = SAPP_KEYCODE_Q;
    _sapp.keycodes[82]  = SAPP_KEYCODE_R;
    _sapp.keycodes[83]  = SAPP_KEYCODE_S;
    _sapp.keycodes[84]  = SAPP_KEYCODE_T;
    _sapp.keycodes[85]  = SAPP_KEYCODE_U;
    _sapp.keycodes[86]  = SAPP_KEYCODE_V;
    _sapp.keycodes[87]  = SAPP_KEYCODE_W;
    _sapp.keycodes[88]  = SAPP_KEYCODE_X;
    _sapp.keycodes[89]  = SAPP_KEYCODE_Y;
    _sapp.keycodes[90]  = SAPP_KEYCODE_Z;
    _sapp.keycodes[91]  = SAPP_KEYCODE_LEFT_SUPER;
    _sapp.keycodes[93]  = SAPP_KEYCODE_MENU;
    _sapp.keycodes[96]  = SAPP_KEYCODE_KP_0;
    _sapp.keycodes[97]  = SAPP_KEYCODE_KP_1;
    _sapp.keycodes[98]  = SAPP_KEYCODE_KP_2;
    _sapp.keycodes[99]  = SAPP_KEYCODE_KP_3;
    _sapp.keycodes[100] = SAPP_KEYCODE_KP_4;
    _sapp.keycodes[101] = SAPP_KEYCODE_KP_5;
    _sapp.keycodes[102] = SAPP_KEYCODE_KP_6;
    _sapp.keycodes[103] = SAPP_KEYCODE_KP_7;
    _sapp.keycodes[104] = SAPP_KEYCODE_KP_8;
    _sapp.keycodes[105] = SAPP_KEYCODE_KP_9;
    _sapp.keycodes[106] = SAPP_KEYCODE_KP_MULTIPLY;
    _sapp.keycodes[107] = SAPP_KEYCODE_KP_ADD;
    _sapp.keycodes[109] = SAPP_KEYCODE_KP_SUBTRACT;
    _sapp.keycodes[110] = SAPP_KEYCODE_KP_DECIMAL;
    _sapp.keycodes[111] = SAPP_KEYCODE_KP_DIVIDE;
    _sapp.keycodes[112] = SAPP_KEYCODE_F1;
    _sapp.keycodes[113] = SAPP_KEYCODE_F2;
    _sapp.keycodes[114] = SAPP_KEYCODE_F3;
    _sapp.keycodes[115] = SAPP_KEYCODE_F4;
    _sapp.keycodes[116] = SAPP_KEYCODE_F5;
    _sapp.keycodes[117] = SAPP_KEYCODE_F6;
    _sapp.keycodes[118] = SAPP_KEYCODE_F7;
    _sapp.keycodes[119] = SAPP_KEYCODE_F8;
    _sapp.keycodes[120] = SAPP_KEYCODE_F9;
    _sapp.keycodes[121] = SAPP_KEYCODE_F10;
    _sapp.keycodes[122] = SAPP_KEYCODE_F11;
    _sapp.keycodes[123] = SAPP_KEYCODE_F12;
    _sapp.keycodes[144] = SAPP_KEYCODE_NUM_LOCK;
    _sapp.keycodes[145] = SAPP_KEYCODE_SCROLL_LOCK;
    _sapp.keycodes[173] = SAPP_KEYCODE_MINUS;
    _sapp.keycodes[186] = SAPP_KEYCODE_SEMICOLON;
    _sapp.keycodes[187] = SAPP_KEYCODE_EQUAL;
    _sapp.keycodes[188] = SAPP_KEYCODE_COMMA;
    _sapp.keycodes[189] = SAPP_KEYCODE_MINUS;
    _sapp.keycodes[190] = SAPP_KEYCODE_PERIOD;
    _sapp.keycodes[191] = SAPP_KEYCODE_SLASH;
    _sapp.keycodes[192] = SAPP_KEYCODE_GRAVE_ACCENT;
    _sapp.keycodes[219] = SAPP_KEYCODE_LEFT_BRACKET;
    _sapp.keycodes[220] = SAPP_KEYCODE_BACKSLASH;
    _sapp.keycodes[221] = SAPP_KEYCODE_RIGHT_BRACKET;
    _sapp.keycodes[222] = SAPP_KEYCODE_APOSTROPHE;
    _sapp.keycodes[224] = SAPP_KEYCODE_LEFT_SUPER;
}

int main(int argc, char* argv[]) {
    sapp_desc desc = sokol_main(argc, argv);
    _sapp_init_state(&desc, 0, 0);
    _sapp_emsc_init_keytable();
    double w, h;
    if (_sapp.html5_canvas_resize) {
        w = (double) desc.width;
        h = (double) desc.height;
    }
    else {
        emscripten_get_element_css_size(_sapp.html5_canvas_name, &w, &h);
        emscripten_set_resize_callback(0, 0, false, _sapp_emsc_size_changed);
    }
    if (_sapp.desc.high_dpi) {
        _sapp.dpi_scale = emscripten_get_device_pixel_ratio();
        w *= _sapp.dpi_scale;
        h *= _sapp.dpi_scale;
    }
    emscripten_set_canvas_element_size(_sapp.html5_canvas_name, w, h);
    _sapp.framebuffer_width = (int) w;
    _sapp.framebuffer_height = (int) h;
    EmscriptenWebGLContextAttributes attrs;
    emscripten_webgl_init_context_attributes(&attrs);
    attrs.alpha = _sapp.desc.alpha;
    attrs.depth = true;
    attrs.stencil = true;
    attrs.antialias = _sapp.sample_count > 1;
    attrs.premultipliedAlpha = _sapp.desc.premultiplied_alpha;
    attrs.preserveDrawingBuffer = _sapp.desc.preserve_drawing_buffer;
    attrs.enableExtensionsByDefault = true;
    #if defined(SOKOL_GLES3)
        attrs.majorVersion = 2;
    #endif
    EMSCRIPTEN_WEBGL_CONTEXT_HANDLE ctx = emscripten_webgl_create_context(0, &attrs);
    if (!ctx) {
        if (attrs.majorVersion == 2) {
            _sapp.gles2_fallback = true;
        }
        attrs.majorVersion = 1;
        ctx = emscripten_webgl_create_context(0, &attrs);
    }
    emscripten_webgl_make_context_current(ctx);
    _sapp.valid = true;
    emscripten_set_mousedown_callback(_sapp.html5_canvas_name, 0, true, _sapp_emsc_mouse_cb);
    emscripten_set_mouseup_callback(_sapp.html5_canvas_name, 0, true, _sapp_emsc_mouse_cb);
    emscripten_set_mousemove_callback(_sapp.html5_canvas_name, 0, true, _sapp_emsc_mouse_cb);
    emscripten_set_mouseenter_callback(_sapp.html5_canvas_name, 0, true, _sapp_emsc_mouse_cb);
    emscripten_set_mouseleave_callback(_sapp.html5_canvas_name, 0, true, _sapp_emsc_mouse_cb);
    emscripten_set_wheel_callback(_sapp.html5_canvas_name, 0, true, _sapp_emsc_wheel_cb);
    emscripten_set_keydown_callback(0, 0, true, _sapp_emsc_key_cb);
    emscripten_set_keyup_callback(0, 0, true, _sapp_emsc_key_cb);
    emscripten_set_keypress_callback(0, 0, true, _sapp_emsc_key_cb);
    emscripten_set_touchstart_callback(_sapp.html5_canvas_name, 0, true, _sapp_emsc_touch_cb);
    emscripten_set_touchmove_callback(_sapp.html5_canvas_name, 0, true, _sapp_emsc_touch_cb);
    emscripten_set_touchend_callback(_sapp.html5_canvas_name, 0, true, _sapp_emsc_touch_cb);
    emscripten_set_touchcancel_callback(_sapp.html5_canvas_name, 0, true, _sapp_emsc_touch_cb);
    emscripten_set_main_loop(_sapp_emsc_frame, 0, 1);
}
#endif  /* __EMSCRIPTEN__ */

/*== WINDOW ==================================================================*/
#if defined(_WIN32)
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <windowsx.h>

#if defined(SOKOL_D3D11)
#define COBJMACROS
#include <d3d11.h>
#include <dxgi.h>
#endif

#ifndef DPI_ENUMS_DECLARED
typedef enum PROCESS_DPI_AWARENESS
{
    PROCESS_DPI_UNAWARE = 0,
    PROCESS_SYSTEM_DPI_AWARE = 1,
    PROCESS_PER_MONITOR_DPI_AWARE = 2
} PROCESS_DPI_AWARENESS;
typedef enum MONITOR_DPI_TYPE {
    MDT_EFFECTIVE_DPI = 0,
    MDT_ANGULAR_DPI = 1,
    MDT_RAW_DPI = 2,
    MDT_DEFAULT = MDT_EFFECTIVE_DPI
} MONITOR_DPI_TYPE;
#endif /*DPI_ENUMS_DECLARED*/

static HWND _sapp_win32_hwnd;
static HDC _sapp_win32_dc;
static bool _sapp_win32_in_create_window;
static bool _sapp_win32_dpi_aware;
static int _sapp_win32_content_scale;
static int _sapp_win32_window_scale;
static float _sapp_win32_mouse_scale;
typedef BOOL(WINAPI * SETPROCESSDPIAWARE_T)(void);
typedef HRESULT(WINAPI * SETPROCESSDPIAWARENESS_T)(PROCESS_DPI_AWARENESS);
typedef HRESULT(WINAPI * GETDPIFORMONITOR_T)(HMONITOR, MONITOR_DPI_TYPE, UINT*, UINT*);
static SETPROCESSDPIAWARE_T _sapp_win32_setprocessdpiaware;
static SETPROCESSDPIAWARENESS_T _sapp_win32_setprocessdpiawareness;
static GETDPIFORMONITOR_T _sapp_win32_getdpiformonitor;
#if defined(SOKOL_D3D11)
static ID3D11Device* _sapp_d3d11_device;
static ID3D11DeviceContext* _sapp_d3d11_device_context;
static DXGI_SWAP_CHAIN_DESC _sapp_dxgi_swap_chain_desc;
static IDXGISwapChain* _sapp_dxgi_swap_chain;
static ID3D11Texture2D* _sapp_d3d11_rt;
static ID3D11RenderTargetView* _sapp_d3d11_rtv;
static ID3D11Texture2D* _sapp_d3d11_ds;
static ID3D11DepthStencilView* _sapp_d3d11_dsv;
#endif
#if defined(SOKOL_GLCORE33)
#define WGL_NUMBER_PIXEL_FORMATS_ARB 0x2000
#define WGL_SUPPORT_OPENGL_ARB 0x2010
#define WGL_DRAW_TO_WINDOW_ARB 0x2001
#define WGL_PIXEL_TYPE_ARB 0x2013
#define WGL_TYPE_RGBA_ARB 0x202b
#define WGL_ACCELERATION_ARB 0x2003
#define WGL_NO_ACCELERATION_ARB 0x2025
#define WGL_RED_BITS_ARB 0x2015
#define WGL_RED_SHIFT_ARB 0x2016
#define WGL_GREEN_BITS_ARB 0x2017
#define WGL_GREEN_SHIFT_ARB 0x2018
#define WGL_BLUE_BITS_ARB 0x2019
#define WGL_BLUE_SHIFT_ARB 0x201a
#define WGL_ALPHA_BITS_ARB 0x201b
#define WGL_ALPHA_SHIFT_ARB 0x201c
#define WGL_ACCUM_BITS_ARB 0x201d
#define WGL_ACCUM_RED_BITS_ARB 0x201e
#define WGL_ACCUM_GREEN_BITS_ARB 0x201f
#define WGL_ACCUM_BLUE_BITS_ARB 0x2020
#define WGL_ACCUM_ALPHA_BITS_ARB 0x2021
#define WGL_DEPTH_BITS_ARB 0x2022
#define WGL_STENCIL_BITS_ARB 0x2023
#define WGL_AUX_BUFFERS_ARB 0x2024
#define WGL_STEREO_ARB 0x2012
#define WGL_DOUBLE_BUFFER_ARB 0x2011
#define WGL_SAMPLES_ARB 0x2042
#define WGL_FRAMEBUFFER_SRGB_CAPABLE_ARB 0x20a9
#define WGL_CONTEXT_DEBUG_BIT_ARB 0x00000001
#define WGL_CONTEXT_FORWARD_COMPATIBLE_BIT_ARB 0x00000002
#define WGL_CONTEXT_PROFILE_MASK_ARB 0x9126
#define WGL_CONTEXT_CORE_PROFILE_BIT_ARB 0x00000001
#define WGL_CONTEXT_COMPATIBILITY_PROFILE_BIT_ARB 0x00000002
#define WGL_CONTEXT_MAJOR_VERSION_ARB 0x2091
#define WGL_CONTEXT_MINOR_VERSION_ARB 0x2092
#define WGL_CONTEXT_FLAGS_ARB 0x2094
#define WGL_CONTEXT_ES2_PROFILE_BIT_EXT 0x00000004
#define WGL_CONTEXT_ROBUST_ACCESS_BIT_ARB 0x00000004
#define WGL_LOSE_CONTEXT_ON_RESET_ARB 0x8252
#define WGL_CONTEXT_RESET_NOTIFICATION_STRATEGY_ARB 0x8256
#define WGL_NO_RESET_NOTIFICATION_ARB 0x8261
#define WGL_CONTEXT_RELEASE_BEHAVIOR_ARB 0x2097
#define WGL_CONTEXT_RELEASE_BEHAVIOR_NONE_ARB 0
#define WGL_CONTEXT_RELEASE_BEHAVIOR_FLUSH_ARB 0x2098
#define WGL_CONTEXT_OPENGL_NO_ERROR_ARB 0x31b3
#define WGL_COLORSPACE_EXT 0x309d
#define WGL_COLORSPACE_SRGB_EXT 0x3089
#define ERROR_INVALID_VERSION_ARB 0x2095
#define ERROR_INVALID_PROFILE_ARB 0x2096
#define ERROR_INCOMPATIBLE_DEVICE_CONTEXTS_ARB 0x2054
typedef BOOL (WINAPI * PFNWGLSWAPINTERVALEXTPROC)(int);
typedef BOOL (WINAPI * PFNWGLGETPIXELFORMATATTRIBIVARBPROC)(HDC,int,int,UINT,const int*,int*);
typedef const char* (WINAPI * PFNWGLGETEXTENSIONSSTRINGEXTPROC)(void);
typedef const char* (WINAPI * PFNWGLGETEXTENSIONSSTRINGARBPROC)(HDC);
typedef HGLRC (WINAPI * PFNWGLCREATECONTEXTATTRIBSARBPROC)(HDC,HGLRC,const int*);
typedef HGLRC (WINAPI * PFN_wglCreateContext)(HDC);
typedef BOOL (WINAPI * PFN_wglDeleteContext)(HGLRC);
typedef PROC (WINAPI * PFN_wglGetProcAddress)(LPCSTR);
typedef HDC (WINAPI * PFN_wglGetCurrentDC)(void);
typedef BOOL (WINAPI * PFN_wglMakeCurrent)(HDC,HGLRC);
static HINSTANCE _sapp_opengl32;
static HGLRC _sapp_gl_ctx;
static PFN_wglCreateContext _sapp_wglCreateContext;
static PFN_wglDeleteContext _sapp_wglDeleteContext;
static PFN_wglGetProcAddress _sapp_wglGetProcAddress;
static PFN_wglGetCurrentDC _sapp_wglGetCurrentDC;
static PFN_wglMakeCurrent _sapp_wglMakeCurrent;
static PFNWGLSWAPINTERVALEXTPROC _sapp_SwapIntervalEXT;
static PFNWGLGETPIXELFORMATATTRIBIVARBPROC _sapp_GetPixelFormatAttribivARB;
static PFNWGLGETEXTENSIONSSTRINGEXTPROC _sapp_GetExtensionsStringEXT;
static PFNWGLGETEXTENSIONSSTRINGARBPROC _sapp_GetExtensionsStringARB;
static PFNWGLCREATECONTEXTATTRIBSARBPROC _sapp_CreateContextAttribsARB;
static bool _sapp_ext_swap_control;
static bool _sapp_arb_multisample;
static bool _sapp_arb_pixel_format;
static bool _sapp_arb_create_context;
static bool _sapp_arb_create_context_profile;
static HWND _sapp_win32_msg_hwnd;
static HDC _sapp_win32_msg_dc;

/* NOTE: the optional GL loader only contains the GL constants and functions required for sokol_gfx.h, if you need
more, you'll need to use you own gl header-generator/loader
*/
#if !defined(SOKOL_WIN32_NO_GL_LOADER)
#define __gl_h_ 1
#define __gl32_h_ 1
#define __gl31_h_ 1
#define __GL_H__ 1
#define __glext_h_ 1
#define __GLEXT_H_ 1
#define __gltypes_h_ 1
#define __glcorearb_h_ 1
#define __gl_glcorearb_h_ 1
#define GL_APIENTRY APIENTRY

typedef unsigned int  GLenum;
typedef unsigned int  GLuint;
typedef int  GLsizei;
typedef char  GLchar;
typedef ptrdiff_t  GLintptr;
typedef ptrdiff_t  GLsizeiptr;
typedef double  GLclampd;
typedef unsigned short  GLushort;
typedef unsigned char  GLubyte;
typedef unsigned char  GLboolean;
typedef uint64_t  GLuint64;
typedef double  GLdouble;
typedef unsigned short  GLhalf;
typedef float  GLclampf;
typedef unsigned int  GLbitfield;
typedef signed char  GLbyte;
typedef short  GLshort;
typedef void  GLvoid;
typedef int64_t  GLint64;
typedef float  GLfloat;
typedef struct __GLsync * GLsync;
typedef int  GLint;
#define GL_INT_2_10_10_10_REV 0x8D9F
#define GL_R32F 0x822E
#define GL_PROGRAM_POINT_SIZE 0x8642
#define GL_STENCIL_ATTACHMENT 0x8D20
#define GL_DEPTH_ATTACHMENT 0x8D00
#define GL_COLOR_ATTACHMENT2 0x8CE2
#define GL_COLOR_ATTACHMENT0 0x8CE0
#define GL_R16F 0x822D
#define GL_COLOR_ATTACHMENT22 0x8CF6
#define GL_DRAW_FRAMEBUFFER 0x8CA9
#define GL_FRAMEBUFFER_COMPLETE 0x8CD5
#define GL_NUM_EXTENSIONS 0x821D
#define GL_INFO_LOG_LENGTH 0x8B84
#define GL_VERTEX_SHADER 0x8B31
#define GL_INCR 0x1E02
#define GL_DYNAMIC_DRAW 0x88E8
#define GL_STATIC_DRAW 0x88E4
#define GL_TEXTURE_CUBE_MAP_POSITIVE_Z 0x8519
#define GL_TEXTURE_CUBE_MAP 0x8513
#define GL_FUNC_SUBTRACT 0x800A
#define GL_FUNC_REVERSE_SUBTRACT 0x800B
#define GL_CONSTANT_COLOR 0x8001
#define GL_DECR_WRAP 0x8508
#define GL_R8 0x8229
#define GL_LINEAR_MIPMAP_LINEAR 0x2703
#define GL_ELEMENT_ARRAY_BUFFER 0x8893
#define GL_SHORT 0x1402
#define GL_DEPTH_TEST 0x0B71
#define GL_TEXTURE_CUBE_MAP_NEGATIVE_Y 0x8518
#define GL_LINK_STATUS 0x8B82
#define GL_TEXTURE_CUBE_MAP_POSITIVE_Y 0x8517
#define GL_SAMPLE_ALPHA_TO_COVERAGE 0x809E
#define GL_RGBA16F 0x881A
#define GL_CONSTANT_ALPHA 0x8003
#define GL_READ_FRAMEBUFFER 0x8CA8
#define GL_TEXTURE0 0x84C0
#define GL_TEXTURE_MIN_LOD 0x813A
#define GL_CLAMP_TO_EDGE 0x812F
#define GL_UNSIGNED_SHORT_5_6_5 0x8363
#define GL_TEXTURE_WRAP_R 0x8072
#define GL_UNSIGNED_SHORT_5_5_5_1 0x8034
#define GL_NEAREST_MIPMAP_NEAREST 0x2700
#define GL_UNSIGNED_SHORT_4_4_4_4 0x8033
#define GL_SRC_ALPHA_SATURATE 0x0308
#define GL_STREAM_DRAW 0x88E0
#define GL_ONE 1
#define GL_NEAREST_MIPMAP_LINEAR 0x2702
#define GL_RGB10_A2 0x8059
#define GL_RGBA8 0x8058
#define GL_COLOR_ATTACHMENT1 0x8CE1
#define GL_RGBA4 0x8056
#define GL_RGB8 0x8051
#define GL_ARRAY_BUFFER 0x8892
#define GL_STENCIL 0x1802
#define GL_TEXTURE_2D 0x0DE1
#define GL_DEPTH 0x1801
#define GL_FRONT 0x0404
#define GL_STENCIL_BUFFER_BIT 0x00000400
#define GL_REPEAT 0x2901
#define GL_RGBA 0x1908
#define GL_TEXTURE_CUBE_MAP_POSITIVE_X 0x8515
#define GL_DECR 0x1E03
#define GL_FRAGMENT_SHADER 0x8B30
#define GL_FLOAT 0x1406
#define GL_TEXTURE_MAX_LOD 0x813B
#define GL_DEPTH_COMPONENT 0x1902
#define GL_ONE_MINUS_DST_ALPHA 0x0305
#define GL_COLOR 0x1800
#define GL_TEXTURE_2D_ARRAY 0x8C1A
#define GL_TRIANGLES 0x0004
#define GL_UNSIGNED_BYTE 0x1401
#define GL_TEXTURE_MAG_FILTER 0x2800
#define GL_ONE_MINUS_CONSTANT_ALPHA 0x8004
#define GL_NONE 0
#define GL_SRC_COLOR 0x0300
#define GL_BYTE 0x1400
#define GL_TEXTURE_CUBE_MAP_NEGATIVE_Z 0x851A
#define GL_LINE_STRIP 0x0003
#define GL_TEXTURE_3D 0x806F
#define GL_CW 0x0900
#define GL_LINEAR 0x2601
#define GL_RENDERBUFFER 0x8D41
#define GL_GEQUAL 0x0206
#define GL_COLOR_BUFFER_BIT 0x00004000
#define GL_RGBA32F 0x8814
#define GL_BLEND 0x0BE2
#define GL_ONE_MINUS_SRC_ALPHA 0x0303
#define GL_ONE_MINUS_CONSTANT_COLOR 0x8002
#define GL_TEXTURE_WRAP_T 0x2803
#define GL_TEXTURE_WRAP_S 0x2802
#define GL_TEXTURE_MIN_FILTER 0x2801
#define GL_LINEAR_MIPMAP_NEAREST 0x2701
#define GL_EXTENSIONS 0x1F03
#define GL_NO_ERROR 0
#define GL_REPLACE 0x1E01
#define GL_KEEP 0x1E00
#define GL_CCW 0x0901
#define GL_TEXTURE_CUBE_MAP_NEGATIVE_X 0x8516
#define GL_RGB 0x1907
#define GL_TRIANGLE_STRIP 0x0005
#define GL_FALSE 0
#define GL_ZERO 0
#define GL_CULL_FACE 0x0B44
#define GL_INVERT 0x150A
#define GL_UNSIGNED_INT 0x1405
#define GL_UNSIGNED_SHORT 0x1403
#define GL_NEAREST 0x2600
#define GL_SCISSOR_TEST 0x0C11
#define GL_LEQUAL 0x0203
#define GL_STENCIL_TEST 0x0B90
#define GL_DITHER 0x0BD0
#define GL_DEPTH_COMPONENT16 0x81A5
#define GL_EQUAL 0x0202
#define GL_FRAMEBUFFER 0x8D40
#define GL_RGB5 0x8050
#define GL_LINES 0x0001
#define GL_DEPTH_BUFFER_BIT 0x00000100
#define GL_SRC_ALPHA 0x0302
#define GL_INCR_WRAP 0x8507
#define GL_LESS 0x0201
#define GL_MULTISAMPLE 0x809D
#define GL_FRAMEBUFFER_BINDING 0x8CA6
#define GL_BACK 0x0405
#define GL_ALWAYS 0x0207
#define GL_FUNC_ADD 0x8006
#define GL_ONE_MINUS_DST_COLOR 0x0307
#define GL_NOTEQUAL 0x0205
#define GL_DST_COLOR 0x0306
#define GL_COMPILE_STATUS 0x8B81
#define GL_RED 0x1903
#define GL_COLOR_ATTACHMENT3 0x8CE3
#define GL_DST_ALPHA 0x0304
#define GL_RGB5_A1 0x8057
#define GL_GREATER 0x0204
#define GL_POLYGON_OFFSET_FILL 0x8037
#define GL_TRUE 1
#define GL_NEVER 0x0200
#define GL_POINTS 0x0000
#define GL_ONE_MINUS_SRC_COLOR 0x0301
#define GL_MIRRORED_REPEAT 0x8370

typedef void  (GL_APIENTRY *PFN_glBindVertexArray)(GLuint array);
static PFN_glBindVertexArray _sapp_glBindVertexArray;
typedef void  (GL_APIENTRY *PFN_glFramebufferTextureLayer)(GLenum target, GLenum attachment, GLuint texture, GLint level, GLint layer);
static PFN_glFramebufferTextureLayer _sapp_glFramebufferTextureLayer;
typedef void  (GL_APIENTRY *PFN_glGenFramebuffers)(GLsizei n, GLuint * framebuffers);
static PFN_glGenFramebuffers _sapp_glGenFramebuffers;
typedef void  (GL_APIENTRY *PFN_glBindFramebuffer)(GLenum target, GLuint framebuffer);
static PFN_glBindFramebuffer _sapp_glBindFramebuffer;
typedef void  (GL_APIENTRY *PFN_glBindRenderbuffer)(GLenum target, GLuint renderbuffer);
static PFN_glBindRenderbuffer _sapp_glBindRenderbuffer;
typedef const GLubyte * (GL_APIENTRY *PFN_glGetStringi)(GLenum name, GLuint index);
static PFN_glGetStringi _sapp_glGetStringi;
typedef void  (GL_APIENTRY *PFN_glClearBufferfi)(GLenum buffer, GLint drawbuffer, GLfloat depth, GLint stencil);
static PFN_glClearBufferfi _sapp_glClearBufferfi;
typedef void  (GL_APIENTRY *PFN_glClearBufferfv)(GLenum buffer, GLint drawbuffer, const GLfloat * value);
static PFN_glClearBufferfv _sapp_glClearBufferfv;
typedef void  (GL_APIENTRY *PFN_glClearBufferuiv)(GLenum buffer, GLint drawbuffer, const GLuint * value);
static PFN_glClearBufferuiv _sapp_glClearBufferuiv;
typedef void  (GL_APIENTRY *PFN_glDeleteRenderbuffers)(GLsizei n, const GLuint * renderbuffers);
static PFN_glDeleteRenderbuffers _sapp_glDeleteRenderbuffers;
typedef void  (GL_APIENTRY *PFN_glUniform4fv)(GLint location, GLsizei count, const GLfloat * value);
static PFN_glUniform4fv _sapp_glUniform4fv;
typedef void  (GL_APIENTRY *PFN_glUniform2fv)(GLint location, GLsizei count, const GLfloat * value);
static PFN_glUniform2fv _sapp_glUniform2fv;
typedef void  (GL_APIENTRY *PFN_glUseProgram)(GLuint program);
static PFN_glUseProgram _sapp_glUseProgram;
typedef void  (GL_APIENTRY *PFN_glShaderSource)(GLuint shader, GLsizei count, const GLchar *const* string, const GLint * length);
static PFN_glShaderSource _sapp_glShaderSource;
typedef void  (GL_APIENTRY *PFN_glLinkProgram)(GLuint program);
static PFN_glLinkProgram _sapp_glLinkProgram;
typedef GLint (GL_APIENTRY *PFN_glGetUniformLocation)(GLuint program, const GLchar * name);
static PFN_glGetUniformLocation _sapp_glGetUniformLocation;
typedef void  (GL_APIENTRY *PFN_glGetShaderiv)(GLuint shader, GLenum pname, GLint * params);
static PFN_glGetShaderiv _sapp_glGetShaderiv;
typedef void  (GL_APIENTRY *PFN_glGetProgramInfoLog)(GLuint program, GLsizei bufSize, GLsizei * length, GLchar * infoLog);
static PFN_glGetProgramInfoLog _sapp_glGetProgramInfoLog;
typedef GLint (GL_APIENTRY *PFN_glGetAttribLocation)(GLuint program, const GLchar * name);
static PFN_glGetAttribLocation _sapp_glGetAttribLocation;
typedef void  (GL_APIENTRY *PFN_glDisableVertexAttribArray)(GLuint index);
static PFN_glDisableVertexAttribArray _sapp_glDisableVertexAttribArray;
typedef void  (GL_APIENTRY *PFN_glDeleteShader)(GLuint shader);
static PFN_glDeleteShader _sapp_glDeleteShader;
typedef void  (GL_APIENTRY *PFN_glDeleteProgram)(GLuint program);
static PFN_glDeleteProgram _sapp_glDeleteProgram;
typedef void  (GL_APIENTRY *PFN_glCompileShader)(GLuint shader);
static PFN_glCompileShader _sapp_glCompileShader;
typedef void  (GL_APIENTRY *PFN_glStencilFuncSeparate)(GLenum face, GLenum func, GLint ref, GLuint mask);
static PFN_glStencilFuncSeparate _sapp_glStencilFuncSeparate;
typedef void  (GL_APIENTRY *PFN_glStencilOpSeparate)(GLenum face, GLenum sfail, GLenum dpfail, GLenum dppass);
static PFN_glStencilOpSeparate _sapp_glStencilOpSeparate;
typedef void  (GL_APIENTRY *PFN_glRenderbufferStorageMultisample)(GLenum target, GLsizei samples, GLenum internalformat, GLsizei width, GLsizei height);
static PFN_glRenderbufferStorageMultisample _sapp_glRenderbufferStorageMultisample;
typedef void  (GL_APIENTRY *PFN_glDrawBuffers)(GLsizei n, const GLenum * bufs);
static PFN_glDrawBuffers _sapp_glDrawBuffers;
typedef void  (GL_APIENTRY *PFN_glVertexAttribDivisor)(GLuint index, GLuint divisor);
static PFN_glVertexAttribDivisor _sapp_glVertexAttribDivisor;
typedef void  (GL_APIENTRY *PFN_glBufferSubData)(GLenum target, GLintptr offset, GLsizeiptr size, const void * data);
static PFN_glBufferSubData _sapp_glBufferSubData;
typedef void  (GL_APIENTRY *PFN_glGenBuffers)(GLsizei n, GLuint * buffers);
static PFN_glGenBuffers _sapp_glGenBuffers;
typedef GLenum (GL_APIENTRY *PFN_glCheckFramebufferStatus)(GLenum target);
static PFN_glCheckFramebufferStatus _sapp_glCheckFramebufferStatus;
typedef void  (GL_APIENTRY *PFN_glFramebufferRenderbuffer)(GLenum target, GLenum attachment, GLenum renderbuffertarget, GLuint renderbuffer);
static PFN_glFramebufferRenderbuffer _sapp_glFramebufferRenderbuffer;
typedef void  (GL_APIENTRY *PFN_glCompressedTexImage2D)(GLenum target, GLint level, GLenum internalformat, GLsizei width, GLsizei height, GLint border, GLsizei imageSize, const void * data);
static PFN_glCompressedTexImage2D _sapp_glCompressedTexImage2D;
typedef void  (GL_APIENTRY *PFN_glCompressedTexImage3D)(GLenum target, GLint level, GLenum internalformat, GLsizei width, GLsizei height, GLsizei depth, GLint border, GLsizei imageSize, const void * data);
static PFN_glCompressedTexImage3D _sapp_glCompressedTexImage3D;
typedef void  (GL_APIENTRY *PFN_glActiveTexture)(GLenum texture);
static PFN_glActiveTexture _sapp_glActiveTexture;
typedef void  (GL_APIENTRY *PFN_glTexSubImage3D)(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLsizei width, GLsizei height, GLsizei depth, GLenum format, GLenum type, const void * pixels);
static PFN_glTexSubImage3D _sapp_glTexSubImage3D;
typedef void  (GL_APIENTRY *PFN_glUniformMatrix4fv)(GLint location, GLsizei count, GLboolean transpose, const GLfloat * value);
static PFN_glUniformMatrix4fv _sapp_glUniformMatrix4fv;
typedef void  (GL_APIENTRY *PFN_glRenderbufferStorage)(GLenum target, GLenum internalformat, GLsizei width, GLsizei height);
static PFN_glRenderbufferStorage _sapp_glRenderbufferStorage;
typedef void  (GL_APIENTRY *PFN_glGenTextures)(GLsizei n, GLuint * textures);
static PFN_glGenTextures _sapp_glGenTextures;
typedef void  (GL_APIENTRY *PFN_glPolygonOffset)(GLfloat factor, GLfloat units);
static PFN_glPolygonOffset _sapp_glPolygonOffset;
typedef void  (GL_APIENTRY *PFN_glDrawElements)(GLenum mode, GLsizei count, GLenum type, const void * indices);
static PFN_glDrawElements _sapp_glDrawElements;
typedef void  (GL_APIENTRY *PFN_glDeleteFramebuffers)(GLsizei n, const GLuint * framebuffers);
static PFN_glDeleteFramebuffers _sapp_glDeleteFramebuffers;
typedef void  (GL_APIENTRY *PFN_glBlendEquationSeparate)(GLenum modeRGB, GLenum modeAlpha);
static PFN_glBlendEquationSeparate _sapp_glBlendEquationSeparate;
typedef void  (GL_APIENTRY *PFN_glDeleteTextures)(GLsizei n, const GLuint * textures);
static PFN_glDeleteTextures _sapp_glDeleteTextures;
typedef void  (GL_APIENTRY *PFN_glGetProgramiv)(GLuint program, GLenum pname, GLint * params);
static PFN_glGetProgramiv _sapp_glGetProgramiv;
typedef void  (GL_APIENTRY *PFN_glBindTexture)(GLenum target, GLuint texture);
static PFN_glBindTexture _sapp_glBindTexture;
typedef void  (GL_APIENTRY *PFN_glTexImage3D)(GLenum target, GLint level, GLint internalformat, GLsizei width, GLsizei height, GLsizei depth, GLint border, GLenum format, GLenum type, const void * pixels);
static PFN_glTexImage3D _sapp_glTexImage3D;
typedef GLuint (GL_APIENTRY *PFN_glCreateShader)(GLenum type);
static PFN_glCreateShader _sapp_glCreateShader;
typedef void  (GL_APIENTRY *PFN_glTexSubImage2D)(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLenum type, const void * pixels);
static PFN_glTexSubImage2D _sapp_glTexSubImage2D;
typedef void  (GL_APIENTRY *PFN_glClearDepth)(GLdouble depth);
static PFN_glClearDepth _sapp_glClearDepth;
typedef void  (GL_APIENTRY *PFN_glFramebufferTexture2D)(GLenum target, GLenum attachment, GLenum textarget, GLuint texture, GLint level);
static PFN_glFramebufferTexture2D _sapp_glFramebufferTexture2D;
typedef GLuint (GL_APIENTRY *PFN_glCreateProgram)();
static PFN_glCreateProgram _sapp_glCreateProgram;
typedef void  (GL_APIENTRY *PFN_glViewport)(GLint x, GLint y, GLsizei width, GLsizei height);
static PFN_glViewport _sapp_glViewport;
typedef void  (GL_APIENTRY *PFN_glDeleteBuffers)(GLsizei n, const GLuint * buffers);
static PFN_glDeleteBuffers _sapp_glDeleteBuffers;
typedef void  (GL_APIENTRY *PFN_glDrawArrays)(GLenum mode, GLint first, GLsizei count);
static PFN_glDrawArrays _sapp_glDrawArrays;
typedef void  (GL_APIENTRY *PFN_glDrawElementsInstanced)(GLenum mode, GLsizei count, GLenum type, const void * indices, GLsizei instancecount);
static PFN_glDrawElementsInstanced _sapp_glDrawElementsInstanced;
typedef void  (GL_APIENTRY *PFN_glVertexAttribPointer)(GLuint index, GLint size, GLenum type, GLboolean normalized, GLsizei stride, const void * pointer);
static PFN_glVertexAttribPointer _sapp_glVertexAttribPointer;
typedef void  (GL_APIENTRY *PFN_glUniform1i)(GLint location, GLint v0);
static PFN_glUniform1i _sapp_glUniform1i;
typedef void  (GL_APIENTRY *PFN_glDisable)(GLenum cap);
static PFN_glDisable _sapp_glDisable;
typedef void  (GL_APIENTRY *PFN_glColorMask)(GLboolean red, GLboolean green, GLboolean blue, GLboolean alpha);
static PFN_glColorMask _sapp_glColorMask;
typedef void  (GL_APIENTRY *PFN_glBindBuffer)(GLenum target, GLuint buffer);
static PFN_glBindBuffer _sapp_glBindBuffer;
typedef void  (GL_APIENTRY *PFN_glDeleteVertexArrays)(GLsizei n, const GLuint * arrays);
static PFN_glDeleteVertexArrays _sapp_glDeleteVertexArrays;
typedef void  (GL_APIENTRY *PFN_glDepthMask)(GLboolean flag);
static PFN_glDepthMask _sapp_glDepthMask;
typedef void  (GL_APIENTRY *PFN_glDrawArraysInstanced)(GLenum mode, GLint first, GLsizei count, GLsizei instancecount);
static PFN_glDrawArraysInstanced _sapp_glDrawArraysInstanced;
typedef void  (GL_APIENTRY *PFN_glClearStencil)(GLint s);
static PFN_glClearStencil _sapp_glClearStencil;
typedef void  (GL_APIENTRY *PFN_glScissor)(GLint x, GLint y, GLsizei width, GLsizei height);
static PFN_glScissor _sapp_glScissor;
typedef void  (GL_APIENTRY *PFN_glUniform3fv)(GLint location, GLsizei count, const GLfloat * value);
static PFN_glUniform3fv _sapp_glUniform3fv;
typedef void  (GL_APIENTRY *PFN_glGenRenderbuffers)(GLsizei n, GLuint * renderbuffers);
static PFN_glGenRenderbuffers _sapp_glGenRenderbuffers;
typedef void  (GL_APIENTRY *PFN_glBufferData)(GLenum target, GLsizeiptr size, const void * data, GLenum usage);
static PFN_glBufferData _sapp_glBufferData;
typedef void  (GL_APIENTRY *PFN_glBlendFuncSeparate)(GLenum sfactorRGB, GLenum dfactorRGB, GLenum sfactorAlpha, GLenum dfactorAlpha);
static PFN_glBlendFuncSeparate _sapp_glBlendFuncSeparate;
typedef void  (GL_APIENTRY *PFN_glTexParameteri)(GLenum target, GLenum pname, GLint param);
static PFN_glTexParameteri _sapp_glTexParameteri;
typedef void  (GL_APIENTRY *PFN_glGetIntegerv)(GLenum pname, GLint * data);
static PFN_glGetIntegerv _sapp_glGetIntegerv;
typedef void  (GL_APIENTRY *PFN_glEnable)(GLenum cap);
static PFN_glEnable _sapp_glEnable;
typedef void  (GL_APIENTRY *PFN_glBlitFramebuffer)(GLint srcX0, GLint srcY0, GLint srcX1, GLint srcY1, GLint dstX0, GLint dstY0, GLint dstX1, GLint dstY1, GLbitfield mask, GLenum filter);
static PFN_glBlitFramebuffer _sapp_glBlitFramebuffer;
typedef void  (GL_APIENTRY *PFN_glStencilMask)(GLuint mask);
static PFN_glStencilMask _sapp_glStencilMask;
typedef void  (GL_APIENTRY *PFN_glAttachShader)(GLuint program, GLuint shader);
static PFN_glAttachShader _sapp_glAttachShader;
typedef GLenum (GL_APIENTRY *PFN_glGetError)();
static PFN_glGetError _sapp_glGetError;
typedef void  (GL_APIENTRY *PFN_glClearColor)(GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha);
static PFN_glClearColor _sapp_glClearColor;
typedef void  (GL_APIENTRY *PFN_glBlendColor)(GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha);
static PFN_glBlendColor _sapp_glBlendColor;
typedef void  (GL_APIENTRY *PFN_glTexParameterf)(GLenum target, GLenum pname, GLfloat param);
static PFN_glTexParameterf _sapp_glTexParameterf;
typedef void  (GL_APIENTRY *PFN_glGetShaderInfoLog)(GLuint shader, GLsizei bufSize, GLsizei * length, GLchar * infoLog);
static PFN_glGetShaderInfoLog _sapp_glGetShaderInfoLog;
typedef void  (GL_APIENTRY *PFN_glDepthFunc)(GLenum func);
static PFN_glDepthFunc _sapp_glDepthFunc;
typedef void  (GL_APIENTRY *PFN_glStencilOp)(GLenum fail, GLenum zfail, GLenum zpass);
static PFN_glStencilOp _sapp_glStencilOp;
typedef void  (GL_APIENTRY *PFN_glStencilFunc)(GLenum func, GLint ref, GLuint mask);
static PFN_glStencilFunc _sapp_glStencilFunc;
typedef void  (GL_APIENTRY *PFN_glEnableVertexAttribArray)(GLuint index);
static PFN_glEnableVertexAttribArray _sapp_glEnableVertexAttribArray;
typedef void  (GL_APIENTRY *PFN_glBlendFunc)(GLenum sfactor, GLenum dfactor);
static PFN_glBlendFunc _sapp_glBlendFunc;
typedef void  (GL_APIENTRY *PFN_glUniform1fv)(GLint location, GLsizei count, const GLfloat * value);
static PFN_glUniform1fv _sapp_glUniform1fv;
typedef void  (GL_APIENTRY *PFN_glReadBuffer)(GLenum src);
static PFN_glReadBuffer _sapp_glReadBuffer;
typedef void  (GL_APIENTRY *PFN_glClear)(GLbitfield mask);
static PFN_glClear _sapp_glClear;
typedef void  (GL_APIENTRY *PFN_glTexImage2D)(GLenum target, GLint level, GLint internalformat, GLsizei width, GLsizei height, GLint border, GLenum format, GLenum type, const void * pixels);
static PFN_glTexImage2D _sapp_glTexImage2D;
typedef void  (GL_APIENTRY *PFN_glGenVertexArrays)(GLsizei n, GLuint * arrays);
static PFN_glGenVertexArrays _sapp_glGenVertexArrays;
typedef void  (GL_APIENTRY *PFN_glFrontFace)(GLenum mode);
static PFN_glFrontFace _sapp_glFrontFace;
typedef void  (GL_APIENTRY *PFN_glCullFace)(GLenum mode);
static PFN_glCullFace _sapp_glCullFace;

_SOKOL_PRIVATE void* _sapp_win32_glgetprocaddr(const char* name) {
    void* proc_addr = (void*) _sapp_wglGetProcAddress(name);
    if (0 == proc_addr) {
        proc_addr = (void*) GetProcAddress(_sapp_opengl32, name);
    }
    SOKOL_ASSERT(proc_addr);
    return proc_addr;
}

#define _SAPP_GLPROC(name) _sapp_ ## name = (PFN_ ## name) _sapp_win32_glgetprocaddr(#name)

_SOKOL_PRIVATE  void _sapp_win32_gl_loadfuncs(void) {
    SOKOL_ASSERT(_sapp_wglGetProcAddress);
    SOKOL_ASSERT(_sapp_opengl32);
    _SAPP_GLPROC(glBindVertexArray);
    _SAPP_GLPROC(glFramebufferTextureLayer);
    _SAPP_GLPROC(glGenFramebuffers);
    _SAPP_GLPROC(glBindFramebuffer);
    _SAPP_GLPROC(glBindRenderbuffer);
    _SAPP_GLPROC(glGetStringi);
    _SAPP_GLPROC(glClearBufferfi);
    _SAPP_GLPROC(glClearBufferfv);
    _SAPP_GLPROC(glClearBufferuiv);
    _SAPP_GLPROC(glDeleteRenderbuffers);
    _SAPP_GLPROC(glUniform4fv);
    _SAPP_GLPROC(glUniform2fv);
    _SAPP_GLPROC(glUseProgram);
    _SAPP_GLPROC(glShaderSource);
    _SAPP_GLPROC(glLinkProgram);
    _SAPP_GLPROC(glGetUniformLocation);
    _SAPP_GLPROC(glGetShaderiv);
    _SAPP_GLPROC(glGetProgramInfoLog);
    _SAPP_GLPROC(glGetAttribLocation);
    _SAPP_GLPROC(glDisableVertexAttribArray);
    _SAPP_GLPROC(glDeleteShader);
    _SAPP_GLPROC(glDeleteProgram);
    _SAPP_GLPROC(glCompileShader);
    _SAPP_GLPROC(glStencilFuncSeparate);
    _SAPP_GLPROC(glStencilOpSeparate);
    _SAPP_GLPROC(glRenderbufferStorageMultisample);
    _SAPP_GLPROC(glDrawBuffers);
    _SAPP_GLPROC(glVertexAttribDivisor);
    _SAPP_GLPROC(glBufferSubData);
    _SAPP_GLPROC(glGenBuffers);
    _SAPP_GLPROC(glCheckFramebufferStatus);
    _SAPP_GLPROC(glFramebufferRenderbuffer);
    _SAPP_GLPROC(glCompressedTexImage2D);
    _SAPP_GLPROC(glCompressedTexImage3D);
    _SAPP_GLPROC(glActiveTexture);
    _SAPP_GLPROC(glTexSubImage3D);
    _SAPP_GLPROC(glUniformMatrix4fv);
    _SAPP_GLPROC(glRenderbufferStorage);
    _SAPP_GLPROC(glGenTextures);
    _SAPP_GLPROC(glPolygonOffset);
    _SAPP_GLPROC(glDrawElements);
    _SAPP_GLPROC(glDeleteFramebuffers);
    _SAPP_GLPROC(glBlendEquationSeparate);
    _SAPP_GLPROC(glDeleteTextures);
    _SAPP_GLPROC(glGetProgramiv);
    _SAPP_GLPROC(glBindTexture);
    _SAPP_GLPROC(glTexImage3D);
    _SAPP_GLPROC(glCreateShader);
    _SAPP_GLPROC(glTexSubImage2D);
    _SAPP_GLPROC(glClearDepth);
    _SAPP_GLPROC(glCreateProgram);
    _SAPP_GLPROC(glViewport);
    _SAPP_GLPROC(glDeleteBuffers);
    _SAPP_GLPROC(glDrawArrays);
    _SAPP_GLPROC(glDrawElementsInstanced);
    _SAPP_GLPROC(glVertexAttribPointer);
    _SAPP_GLPROC(glUniform1i);
    _SAPP_GLPROC(glDisable);
    _SAPP_GLPROC(glColorMask);
    _SAPP_GLPROC(glBindBuffer);
    _SAPP_GLPROC(glDeleteVertexArrays);
    _SAPP_GLPROC(glDepthMask);
    _SAPP_GLPROC(glDrawArraysInstanced);
    _SAPP_GLPROC(glClearStencil);
    _SAPP_GLPROC(glScissor);
    _SAPP_GLPROC(glUniform3fv);
    _SAPP_GLPROC(glGenRenderbuffers);
    _SAPP_GLPROC(glBufferData);
    _SAPP_GLPROC(glBlendFuncSeparate);
    _SAPP_GLPROC(glTexParameteri);
    _SAPP_GLPROC(glGetIntegerv);
    _SAPP_GLPROC(glEnable);
    _SAPP_GLPROC(glBlitFramebuffer);
    _SAPP_GLPROC(glStencilMask);
    _SAPP_GLPROC(glAttachShader);
    _SAPP_GLPROC(glGetError);
    _SAPP_GLPROC(glClearColor);
    _SAPP_GLPROC(glBlendColor);
    _SAPP_GLPROC(glTexParameterf);
    _SAPP_GLPROC(glGetShaderInfoLog);
    _SAPP_GLPROC(glDepthFunc);
    _SAPP_GLPROC(glStencilOp);
    _SAPP_GLPROC(glStencilFunc);
    _SAPP_GLPROC(glEnableVertexAttribArray);
    _SAPP_GLPROC(glBlendFunc);
    _SAPP_GLPROC(glUniform1fv);
    _SAPP_GLPROC(glReadBuffer);
    _SAPP_GLPROC(glClear);
    _SAPP_GLPROC(glTexImage2D);
    _SAPP_GLPROC(glGenVertexArrays);
    _SAPP_GLPROC(glFrontFace);
    _SAPP_GLPROC(glCullFace);
}
#define glBindVertexArray _sapp_glBindVertexArray
#define glFramebufferTextureLayer _sapp_glFramebufferTextureLayer
#define glGenFramebuffers _sapp_glGenFramebuffers
#define glBindFramebuffer _sapp_glBindFramebuffer
#define glBindRenderbuffer _sapp_glBindRenderbuffer
#define glGetStringi _sapp_glGetStringi
#define glClearBufferfi _sapp_glClearBufferfi
#define glClearBufferfv _sapp_glClearBufferfv
#define glClearBufferuiv _sapp_glClearBufferuiv
#define glDeleteRenderbuffers _sapp_glDeleteRenderbuffers
#define glUniform4fv _sapp_glUniform4fv
#define glUniform2fv _sapp_glUniform2fv
#define glUseProgram _sapp_glUseProgram
#define glShaderSource _sapp_glShaderSource
#define glLinkProgram _sapp_glLinkProgram
#define glGetUniformLocation _sapp_glGetUniformLocation
#define glGetShaderiv _sapp_glGetShaderiv
#define glGetProgramInfoLog _sapp_glGetProgramInfoLog
#define glGetAttribLocation _sapp_glGetAttribLocation
#define glDisableVertexAttribArray _sapp_glDisableVertexAttribArray
#define glDeleteShader _sapp_glDeleteShader
#define glDeleteProgram _sapp_glDeleteProgram
#define glCompileShader _sapp_glCompileShader
#define glStencilFuncSeparate _sapp_glStencilFuncSeparate
#define glStencilOpSeparate _sapp_glStencilOpSeparate
#define glRenderbufferStorageMultisample _sapp_glRenderbufferStorageMultisample
#define glDrawBuffers _sapp_glDrawBuffers
#define glVertexAttribDivisor _sapp_glVertexAttribDivisor
#define glBufferSubData _sapp_glBufferSubData
#define glGenBuffers _sapp_glGenBuffers
#define glCheckFramebufferStatus _sapp_glCheckFramebufferStatus
#define glFramebufferRenderbuffer _sapp_glFramebufferRenderbuffer
#define glCompressedTexImage2D _sapp_glCompressedTexImage2D
#define glCompressedTexImage3D _sapp_glCompressedTexImage3D
#define glActiveTexture _sapp_glActiveTexture
#define glTexSubImage3D _sapp_glTexSubImage3D
#define glUniformMatrix4fv _sapp_glUniformMatrix4fv
#define glRenderbufferStorage _sapp_glRenderbufferStorage
#define glGenTextures _sapp_glGenTextures
#define glPolygonOffset _sapp_glPolygonOffset
#define glDrawElements _sapp_glDrawElements
#define glDeleteFramebuffers _sapp_glDeleteFramebuffers
#define glBlendEquationSeparate _sapp_glBlendEquationSeparate
#define glDeleteTextures _sapp_glDeleteTextures
#define glGetProgramiv _sapp_glGetProgramiv
#define glBindTexture _sapp_glBindTexture
#define glTexImage3D _sapp_glTexImage3D
#define glCreateShader _sapp_glCreateShader
#define glTexSubImage2D _sapp_glTexSubImage2D
#define glClearDepth _sapp_glClearDepth
#define glFramebufferTexture2D _sapp_glFramebufferTexture2D
#define glCreateProgram _sapp_glCreateProgram
#define glViewport _sapp_glViewport
#define glDeleteBuffers _sapp_glDeleteBuffers
#define glDrawArrays _sapp_glDrawArrays
#define glDrawElementsInstanced _sapp_glDrawElementsInstanced
#define glVertexAttribPointer _sapp_glVertexAttribPointer
#define glUniform1i _sapp_glUniform1i
#define glDisable _sapp_glDisable
#define glColorMask _sapp_glColorMask
#define glBindBuffer _sapp_glBindBuffer
#define glDeleteVertexArrays _sapp_glDeleteVertexArrays
#define glDepthMask _sapp_glDepthMask
#define glDrawArraysInstanced _sapp_glDrawArraysInstanced
#define glClearStencil _sapp_glClearStencil
#define glScissor _sapp_glScissor
#define glUniform3fv _sapp_glUniform3fv
#define glGenRenderbuffers _sapp_glGenRenderbuffers
#define glBufferData _sapp_glBufferData
#define glBlendFuncSeparate _sapp_glBlendFuncSeparate
#define glTexParameteri _sapp_glTexParameteri
#define glGetIntegerv _sapp_glGetIntegerv
#define glEnable _sapp_glEnable
#define glBlitFramebuffer _sapp_glBlitFramebuffer
#define glStencilMask _sapp_glStencilMask
#define glAttachShader _sapp_glAttachShader
#define glGetError _sapp_glGetError
#define glClearColor _sapp_glClearColor
#define glBlendColor _sapp_glBlendColor
#define glTexParameterf _sapp_glTexParameterf
#define glGetShaderInfoLog _sapp_glGetShaderInfoLog
#define glDepthFunc _sapp_glDepthFunc
#define glStencilOp _sapp_glStencilOp
#define glStencilFunc _sapp_glStencilFunc
#define glEnableVertexAttribArray _sapp_glEnableVertexAttribArray
#define glBlendFunc _sapp_glBlendFunc
#define glUniform1fv _sapp_glUniform1fv
#define glReadBuffer _sapp_glReadBuffer
#define glClear _sapp_glClear
#define glTexImage2D _sapp_glTexImage2D
#define glGenVertexArrays _sapp_glGenVertexArrays
#define glFrontFace _sapp_glFrontFace
#define glCullFace _sapp_glCullFace

#endif /* SOKOL_WIN32_NO_GL_LOADER */

#endif /* SOKOL_GLCORE33 */

#define _SAPP_SAFE_RELEASE(class, obj) if (obj) { class##_Release(obj); obj=0; }

_SOKOL_PRIVATE bool _sapp_win32_utf8_to_wide(const char* src, wchar_t* dst, int dst_num_bytes) {
    SOKOL_ASSERT(src && dst && (dst_num_bytes > 1));
    memset(dst, 0, dst_num_bytes);
    const int dst_chars = dst_num_bytes / sizeof(wchar_t);
    const int dst_needed = MultiByteToWideChar(CP_UTF8, 0, src, -1, 0, 0);
    if ((dst_needed > 0) && (dst_needed < dst_chars)) {
        MultiByteToWideChar(CP_UTF8, 0, src, -1, dst, dst_chars);
        return true;
    }
    else {
        /* input string doesn't fit into destination buffer */
        return false;
    }
}

_SOKOL_PRIVATE void _sapp_win32_init_keytable(void) {
    /* same as GLFW */
    _sapp.keycodes[0x00B] = SAPP_KEYCODE_0;
    _sapp.keycodes[0x002] = SAPP_KEYCODE_1;
    _sapp.keycodes[0x003] = SAPP_KEYCODE_2;
    _sapp.keycodes[0x004] = SAPP_KEYCODE_3;
    _sapp.keycodes[0x005] = SAPP_KEYCODE_4;
    _sapp.keycodes[0x006] = SAPP_KEYCODE_5;
    _sapp.keycodes[0x007] = SAPP_KEYCODE_6;
    _sapp.keycodes[0x008] = SAPP_KEYCODE_7;
    _sapp.keycodes[0x009] = SAPP_KEYCODE_8;
    _sapp.keycodes[0x00A] = SAPP_KEYCODE_9;
    _sapp.keycodes[0x01E] = SAPP_KEYCODE_A;
    _sapp.keycodes[0x030] = SAPP_KEYCODE_B;
    _sapp.keycodes[0x02E] = SAPP_KEYCODE_C;
    _sapp.keycodes[0x020] = SAPP_KEYCODE_D;
    _sapp.keycodes[0x012] = SAPP_KEYCODE_E;
    _sapp.keycodes[0x021] = SAPP_KEYCODE_F;
    _sapp.keycodes[0x022] = SAPP_KEYCODE_G;
    _sapp.keycodes[0x023] = SAPP_KEYCODE_H;
    _sapp.keycodes[0x017] = SAPP_KEYCODE_I;
    _sapp.keycodes[0x024] = SAPP_KEYCODE_J;
    _sapp.keycodes[0x025] = SAPP_KEYCODE_K;
    _sapp.keycodes[0x026] = SAPP_KEYCODE_L;
    _sapp.keycodes[0x032] = SAPP_KEYCODE_M;
    _sapp.keycodes[0x031] = SAPP_KEYCODE_N;
    _sapp.keycodes[0x018] = SAPP_KEYCODE_O;
    _sapp.keycodes[0x019] = SAPP_KEYCODE_P;
    _sapp.keycodes[0x010] = SAPP_KEYCODE_Q;
    _sapp.keycodes[0x013] = SAPP_KEYCODE_R;
    _sapp.keycodes[0x01F] = SAPP_KEYCODE_S;
    _sapp.keycodes[0x014] = SAPP_KEYCODE_T;
    _sapp.keycodes[0x016] = SAPP_KEYCODE_U;
    _sapp.keycodes[0x02F] = SAPP_KEYCODE_V;
    _sapp.keycodes[0x011] = SAPP_KEYCODE_W;
    _sapp.keycodes[0x02D] = SAPP_KEYCODE_X;
    _sapp.keycodes[0x015] = SAPP_KEYCODE_Y;
    _sapp.keycodes[0x02C] = SAPP_KEYCODE_Z;
    _sapp.keycodes[0x028] = SAPP_KEYCODE_APOSTROPHE;
    _sapp.keycodes[0x02B] = SAPP_KEYCODE_BACKSLASH;
    _sapp.keycodes[0x033] = SAPP_KEYCODE_COMMA;
    _sapp.keycodes[0x00D] = SAPP_KEYCODE_EQUAL;
    _sapp.keycodes[0x029] = SAPP_KEYCODE_GRAVE_ACCENT;
    _sapp.keycodes[0x01A] = SAPP_KEYCODE_LEFT_BRACKET;
    _sapp.keycodes[0x00C] = SAPP_KEYCODE_MINUS;
    _sapp.keycodes[0x034] = SAPP_KEYCODE_PERIOD;
    _sapp.keycodes[0x01B] = SAPP_KEYCODE_RIGHT_BRACKET;
    _sapp.keycodes[0x027] = SAPP_KEYCODE_SEMICOLON;
    _sapp.keycodes[0x035] = SAPP_KEYCODE_SLASH;
    _sapp.keycodes[0x056] = SAPP_KEYCODE_WORLD_2;
    _sapp.keycodes[0x00E] = SAPP_KEYCODE_BACKSPACE;
    _sapp.keycodes[0x153] = SAPP_KEYCODE_DELETE;
    _sapp.keycodes[0x14F] = SAPP_KEYCODE_END;
    _sapp.keycodes[0x01C] = SAPP_KEYCODE_ENTER;
    _sapp.keycodes[0x001] = SAPP_KEYCODE_ESCAPE;
    _sapp.keycodes[0x147] = SAPP_KEYCODE_HOME;
    _sapp.keycodes[0x152] = SAPP_KEYCODE_INSERT;
    _sapp.keycodes[0x15D] = SAPP_KEYCODE_MENU;
    _sapp.keycodes[0x151] = SAPP_KEYCODE_PAGE_DOWN;
    _sapp.keycodes[0x149] = SAPP_KEYCODE_PAGE_UP;
    _sapp.keycodes[0x045] = SAPP_KEYCODE_PAUSE;
    _sapp.keycodes[0x146] = SAPP_KEYCODE_PAUSE;
    _sapp.keycodes[0x039] = SAPP_KEYCODE_SPACE;
    _sapp.keycodes[0x00F] = SAPP_KEYCODE_TAB;
    _sapp.keycodes[0x03A] = SAPP_KEYCODE_CAPS_LOCK;
    _sapp.keycodes[0x145] = SAPP_KEYCODE_NUM_LOCK;
    _sapp.keycodes[0x046] = SAPP_KEYCODE_SCROLL_LOCK;
    _sapp.keycodes[0x03B] = SAPP_KEYCODE_F1;
    _sapp.keycodes[0x03C] = SAPP_KEYCODE_F2;
    _sapp.keycodes[0x03D] = SAPP_KEYCODE_F3;
    _sapp.keycodes[0x03E] = SAPP_KEYCODE_F4;
    _sapp.keycodes[0x03F] = SAPP_KEYCODE_F5;
    _sapp.keycodes[0x040] = SAPP_KEYCODE_F6;
    _sapp.keycodes[0x041] = SAPP_KEYCODE_F7;
    _sapp.keycodes[0x042] = SAPP_KEYCODE_F8;
    _sapp.keycodes[0x043] = SAPP_KEYCODE_F9;
    _sapp.keycodes[0x044] = SAPP_KEYCODE_F10;
    _sapp.keycodes[0x057] = SAPP_KEYCODE_F11;
    _sapp.keycodes[0x058] = SAPP_KEYCODE_F12;
    _sapp.keycodes[0x064] = SAPP_KEYCODE_F13;
    _sapp.keycodes[0x065] = SAPP_KEYCODE_F14;
    _sapp.keycodes[0x066] = SAPP_KEYCODE_F15;
    _sapp.keycodes[0x067] = SAPP_KEYCODE_F16;
    _sapp.keycodes[0x068] = SAPP_KEYCODE_F17;
    _sapp.keycodes[0x069] = SAPP_KEYCODE_F18;
    _sapp.keycodes[0x06A] = SAPP_KEYCODE_F19;
    _sapp.keycodes[0x06B] = SAPP_KEYCODE_F20;
    _sapp.keycodes[0x06C] = SAPP_KEYCODE_F21;
    _sapp.keycodes[0x06D] = SAPP_KEYCODE_F22;
    _sapp.keycodes[0x06E] = SAPP_KEYCODE_F23;
    _sapp.keycodes[0x076] = SAPP_KEYCODE_F24;
    _sapp.keycodes[0x038] = SAPP_KEYCODE_LEFT_ALT;
    _sapp.keycodes[0x01D] = SAPP_KEYCODE_LEFT_CONTROL;
    _sapp.keycodes[0x02A] = SAPP_KEYCODE_LEFT_SHIFT;
    _sapp.keycodes[0x15B] = SAPP_KEYCODE_LEFT_SUPER;
    _sapp.keycodes[0x137] = SAPP_KEYCODE_PRINT_SCREEN;
    _sapp.keycodes[0x138] = SAPP_KEYCODE_RIGHT_ALT;
    _sapp.keycodes[0x11D] = SAPP_KEYCODE_RIGHT_CONTROL;
    _sapp.keycodes[0x036] = SAPP_KEYCODE_RIGHT_SHIFT;
    _sapp.keycodes[0x15C] = SAPP_KEYCODE_RIGHT_SUPER;
    _sapp.keycodes[0x150] = SAPP_KEYCODE_DOWN;
    _sapp.keycodes[0x14B] = SAPP_KEYCODE_LEFT;
    _sapp.keycodes[0x14D] = SAPP_KEYCODE_RIGHT;
    _sapp.keycodes[0x148] = SAPP_KEYCODE_UP;
    _sapp.keycodes[0x052] = SAPP_KEYCODE_KP_0;
    _sapp.keycodes[0x04F] = SAPP_KEYCODE_KP_1;
    _sapp.keycodes[0x050] = SAPP_KEYCODE_KP_2;
    _sapp.keycodes[0x051] = SAPP_KEYCODE_KP_3;
    _sapp.keycodes[0x04B] = SAPP_KEYCODE_KP_4;
    _sapp.keycodes[0x04C] = SAPP_KEYCODE_KP_5;
    _sapp.keycodes[0x04D] = SAPP_KEYCODE_KP_6;
    _sapp.keycodes[0x047] = SAPP_KEYCODE_KP_7;
    _sapp.keycodes[0x048] = SAPP_KEYCODE_KP_8;
    _sapp.keycodes[0x049] = SAPP_KEYCODE_KP_9;
    _sapp.keycodes[0x04E] = SAPP_KEYCODE_KP_ADD;
    _sapp.keycodes[0x053] = SAPP_KEYCODE_KP_DECIMAL;
    _sapp.keycodes[0x135] = SAPP_KEYCODE_KP_DIVIDE;
    _sapp.keycodes[0x11C] = SAPP_KEYCODE_KP_ENTER;
    _sapp.keycodes[0x037] = SAPP_KEYCODE_KP_MULTIPLY;
    _sapp.keycodes[0x04A] = SAPP_KEYCODE_KP_SUBTRACT;
}

_SOKOL_PRIVATE uint32_t _sapp_win32_mods(void) {
    uint32_t mods = 0;
    if (GetKeyState(VK_SHIFT) & (1<<31)) {
        mods |= SAPP_MODIFIER_SHIFT;
    }
    if (GetKeyState(VK_CONTROL) & (1<<31)) {
        mods |= SAPP_MODIFIER_CTRL;
    }
    if (GetKeyState(VK_MENU) & (1<<31)) {
        mods |= SAPP_MODIFIER_ALT;
    }
    if ((GetKeyState(VK_LWIN) | GetKeyState(VK_RWIN)) & (1<<31)) {
        mods |= SAPP_MODIFIER_SUPER;
    }
    return mods;
}

_SOKOL_PRIVATE void _sapp_win32_mouse_event(sapp_event_type type, sapp_mousebutton btn) {
    if (_sapp_events_enabled()) {
        _sapp_init_event(type);
        _sapp.event.modifiers = _sapp_win32_mods();
        _sapp.event.mouse_button = btn;
        _sapp.event.mouse_x = _sapp.mouse_x;
        _sapp.event.mouse_y = _sapp.mouse_y;
        _sapp.desc.event_cb(&_sapp.event);
    }
}

_SOKOL_PRIVATE void _sapp_win32_scroll_event(float val) {
    if (_sapp_events_enabled()) {
        _sapp_init_event(SAPP_EVENTTYPE_MOUSE_SCROLL);
        _sapp.event.modifiers = _sapp_win32_mods();
        _sapp.event.scroll_y = val / 30.0f;
        _sapp.desc.event_cb(&_sapp.event);
    }
}

_SOKOL_PRIVATE void _sapp_win32_key_event(sapp_event_type type, int vk) {
    if (_sapp_events_enabled() && (vk < SAPP_MAX_KEYCODES)) {
        _sapp_init_event(type);
        _sapp.event.modifiers = _sapp_win32_mods();
        _sapp.event.key_code = _sapp.keycodes[vk];
        _sapp.desc.event_cb(&_sapp.event);
    }
}

_SOKOL_PRIVATE void _sapp_win32_char_event(uint32_t c) {
    if (_sapp_events_enabled()) {
        _sapp_init_event(SAPP_EVENTTYPE_CHAR);
        _sapp.event.modifiers = _sapp_win32_mods();
        _sapp.event.char_code = c;
        _sapp.desc.event_cb(&_sapp.event);
    }
}

_SOKOL_PRIVATE LRESULT CALLBACK _sapp_win32_wndproc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
        case WM_CLOSE:
            PostQuitMessage(0);
            return 0;
        case WM_ERASEBKGND:
            return 1;
        case WM_LBUTTONDOWN:
            _sapp_win32_mouse_event(SAPP_EVENTTYPE_MOUSE_DOWN, SAPP_MOUSEBUTTON_LEFT);
            break;
        case WM_RBUTTONDOWN:
            _sapp_win32_mouse_event(SAPP_EVENTTYPE_MOUSE_DOWN, SAPP_MOUSEBUTTON_RIGHT);
            break;
        case WM_MBUTTONDOWN:
            _sapp_win32_mouse_event(SAPP_EVENTTYPE_MOUSE_DOWN, SAPP_MOUSEBUTTON_MIDDLE);
            break;
        case WM_LBUTTONUP:
            _sapp_win32_mouse_event(SAPP_EVENTTYPE_MOUSE_UP, SAPP_MOUSEBUTTON_LEFT);
            break;
        case WM_RBUTTONUP:
            _sapp_win32_mouse_event(SAPP_EVENTTYPE_MOUSE_UP, SAPP_MOUSEBUTTON_RIGHT);
            break;
        case WM_MBUTTONUP:
            _sapp_win32_mouse_event(SAPP_EVENTTYPE_MOUSE_UP, SAPP_MOUSEBUTTON_MIDDLE);
            break;
        case WM_MOUSEMOVE:
            _sapp.mouse_x = (float)GET_X_LPARAM(lParam) * _sapp_win32_mouse_scale;
            _sapp.mouse_y = (float)GET_Y_LPARAM(lParam) * _sapp_win32_mouse_scale;
            if (!_sapp.win32_mouse_tracked) {
                _sapp.win32_mouse_tracked = true;
                TRACKMOUSEEVENT tme;
                memset(&tme, 0, sizeof(tme));
                tme.cbSize = sizeof(tme);
                tme.dwFlags = TME_LEAVE;
                tme.hwndTrack = _sapp_win32_hwnd;
                TrackMouseEvent(&tme);
                _sapp_win32_mouse_event(SAPP_EVENTTYPE_MOUSE_ENTER, 0);
            }
            _sapp_win32_mouse_event(SAPP_EVENTTYPE_MOUSE_MOVE, 0);
            break;
        case WM_MOUSELEAVE:
            _sapp.win32_mouse_tracked = false;
            _sapp_win32_mouse_event(SAPP_EVENTTYPE_MOUSE_LEAVE, 0);
            break;
        case WM_MOUSEWHEEL:
            _sapp_win32_scroll_event((float)((SHORT)HIWORD(wParam)));
            break;
        case WM_CHAR:
            _sapp_win32_char_event((uint32_t)wParam);
            break;
        case WM_KEYDOWN:
            _sapp_win32_key_event(SAPP_EVENTTYPE_KEY_DOWN, (int)(HIWORD(lParam)&0x1FF));
            break;
        case WM_KEYUP:
            _sapp_win32_key_event(SAPP_EVENTTYPE_KEY_UP, (int)(HIWORD(lParam)&0x1FF));
            break;
        default:
            break;
    }
    return DefWindowProcW(hWnd, uMsg, wParam, lParam);
}

_SOKOL_PRIVATE void _sapp_win32_create_window(void) {
    WNDCLASSW wndclassw;
    memset(&wndclassw, 0, sizeof(wndclassw));
    wndclassw.style = CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
    wndclassw.lpfnWndProc = (WNDPROC) _sapp_win32_wndproc;
    wndclassw.hInstance = GetModuleHandleW(NULL);
    wndclassw.hCursor = LoadCursor(NULL, IDC_ARROW);
    wndclassw.hIcon = LoadIcon(NULL, IDI_WINLOGO);
    wndclassw.lpszClassName = L"SOKOLAPP";
    RegisterClassW(&wndclassw);

    /* FIXME: HighDPI support */
    const DWORD win_style = WS_CLIPSIBLINGS | WS_CLIPCHILDREN | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX | WS_MAXIMIZEBOX | WS_SIZEBOX; 
    const DWORD win_ex_style = WS_EX_APPWINDOW | WS_EX_WINDOWEDGE;
    RECT rect = { 0, 0, 
        (int) (_sapp.window_width * _sapp_win32_window_scale),
        (int) (_sapp.window_height * _sapp_win32_window_scale)
    };
    AdjustWindowRectEx(&rect, win_style, FALSE, win_ex_style);
    const int win_width = rect.right - rect.left;
    const int win_height = rect.bottom - rect.top;
    _sapp_win32_in_create_window = true;
    _sapp_win32_hwnd = CreateWindowExW(
        win_ex_style,               /* dwExStyle */
        L"SOKOLAPP",                /* lpClassName */
        _sapp.window_title_wide,    /* lpWindowName */
        win_style,                  /* dwStyle */
        CW_USEDEFAULT,              /* X */
        CW_USEDEFAULT,              /* Y */
        win_width,                  /* nWidth */
        win_height,                 /* nHeight */
        NULL,                       /* hWndParent */
        NULL,                       /* hMenu */
        GetModuleHandle(NULL),      /* hInstance */
        NULL);                      /* lParam */
    ShowWindow(_sapp_win32_hwnd, SW_SHOW);
    _sapp_win32_in_create_window = false;
    _sapp_win32_dc = GetDC(_sapp_win32_hwnd);
    SOKOL_ASSERT(_sapp_win32_dc);
        
    if (GetClientRect(_sapp_win32_hwnd, &rect)) {
        _sapp.window_width = (rect.right - rect.left) / _sapp_win32_window_scale;
        _sapp.window_height = (rect.bottom - rect.top) / _sapp_win32_window_scale;
        _sapp.framebuffer_width = _sapp.window_width * _sapp_win32_content_scale;
        _sapp.framebuffer_height = _sapp.window_height * _sapp_win32_content_scale;
    }
}

_SOKOL_PRIVATE void _sapp_win32_destroy_window(void) {
    DestroyWindow(_sapp_win32_hwnd); _sapp_win32_hwnd = 0;
    UnregisterClassW(L"SOKOLAPP", GetModuleHandleW(NULL));
}

_SOKOL_PRIVATE void _sapp_win32_init_dpi(void) {
    SOKOL_ASSERT(0 == _sapp_win32_setprocessdpiaware);
    SOKOL_ASSERT(0 == _sapp_win32_setprocessdpiawareness);
    SOKOL_ASSERT(0 == _sapp_win32_getdpiformonitor);
    HINSTANCE user32 = LoadLibraryA("user32.dll");
    if (user32) {
        _sapp_win32_setprocessdpiaware = (SETPROCESSDPIAWARE_T) GetProcAddress(user32, "SetProcessDPIAware");
    }
    HINSTANCE shcore = LoadLibraryA("shcore.dll");
    if (shcore) {
        _sapp_win32_setprocessdpiawareness = (SETPROCESSDPIAWARENESS_T) GetProcAddress(shcore, "SetProcessDpiAwareness");
        _sapp_win32_getdpiformonitor = (GETDPIFORMONITOR_T) GetProcAddress(shcore, "GetDpiForMonitor");
    }
    if (_sapp_win32_setprocessdpiawareness) {
        /* if the app didn't request HighDPI rendering, let Windows do the upscaling */
        PROCESS_DPI_AWARENESS process_dpi_awareness = PROCESS_SYSTEM_DPI_AWARE;
        _sapp_win32_dpi_aware = true;
        if (!_sapp.desc.high_dpi) {
            process_dpi_awareness = PROCESS_DPI_UNAWARE;
            _sapp_win32_dpi_aware = false;
        }
        _sapp_win32_setprocessdpiawareness(process_dpi_awareness);
    }
    else if (_sapp_win32_setprocessdpiaware) {
        _sapp_win32_setprocessdpiaware();
        _sapp_win32_dpi_aware = true;
    }
    /* get dpi scale factor for main monitor */
    if (_sapp_win32_getdpiformonitor && _sapp_win32_dpi_aware) {
        POINT pt = { 1, 1 };
        HMONITOR hm = MonitorFromPoint(pt, MONITOR_DEFAULTTONEAREST);
        UINT dpix, dpiy;
        HRESULT hr = _sapp_win32_getdpiformonitor(hm, MDT_EFFECTIVE_DPI, &dpix, &dpiy);
        SOKOL_ASSERT(SUCCEEDED(hr));
        /* clamp window scale to an integer factor */
        _sapp_win32_window_scale = (int)((float)dpix / 96.0f);
    }
    else {
        _sapp_win32_window_scale = 1;
    }
    if (_sapp.desc.high_dpi) {
        _sapp_win32_content_scale = _sapp_win32_window_scale;
        _sapp_win32_mouse_scale = 1.0f;
    }
    else {
        _sapp_win32_content_scale = 1;
        _sapp_win32_mouse_scale = 1.0f / _sapp_win32_window_scale;
    }
    _sapp.dpi_scale = (float) _sapp_win32_content_scale;
    if (user32) {
        FreeLibrary(user32);
    }
    if (shcore) {
        FreeLibrary(shcore);
    }
}

#if defined(SOKOL_D3D11)
_SOKOL_PRIVATE void _sapp_d3d11_create_device_and_swapchain(void) {
    DXGI_SWAP_CHAIN_DESC* sc_desc = &_sapp_dxgi_swap_chain_desc;
    sc_desc->BufferDesc.Width = _sapp.framebuffer_width;
    sc_desc->BufferDesc.Height = _sapp.framebuffer_height;
    sc_desc->BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    sc_desc->BufferDesc.RefreshRate.Numerator = 60;
    sc_desc->BufferDesc.RefreshRate.Denominator = 1;
    sc_desc->OutputWindow = _sapp_win32_hwnd;
    sc_desc->Windowed = true;
    sc_desc->SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
    sc_desc->BufferCount = 1;
    sc_desc->SampleDesc.Count = _sapp.sample_count;
    sc_desc->SampleDesc.Quality = _sapp.sample_count > 1 ? D3D11_STANDARD_MULTISAMPLE_PATTERN : 0;
    sc_desc->BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    int create_flags = D3D11_CREATE_DEVICE_SINGLETHREADED;
    #if defined(SOKOL_DEBUG)
        create_flags |= D3D11_CREATE_DEVICE_DEBUG;
    #endif
    D3D_FEATURE_LEVEL feature_level;
    HRESULT hr = D3D11CreateDeviceAndSwapChain(
        NULL,                           /* pAdapter (use default) */
        D3D_DRIVER_TYPE_HARDWARE,       /* DriverType */
        NULL,                           /* Software */
        create_flags,                   /* Flags */
        NULL,                           /* pFeatureLevels */
        0,                              /* FeatureLevels */
        D3D11_SDK_VERSION,              /* SDKVersion */
        sc_desc,                        /* pSwapChainDesc */
        &_sapp_dxgi_swap_chain,         /* ppSwapChain */
        &_sapp_d3d11_device,            /* ppDevice */
        &feature_level,                 /* pFeatureLevel */
        &_sapp_d3d11_device_context);   /* ppImmediateContext */
    SOKOL_ASSERT(SUCCEEDED(hr) && _sapp_dxgi_swap_chain && _sapp_d3d11_device && _sapp_d3d11_device_context);
}

_SOKOL_PRIVATE _sapp_d3d11_destroy_device_and_swapchain(void) {
    _SAPP_SAFE_RELEASE(IDXGISwapChain, _sapp_dxgi_swap_chain);
    _SAPP_SAFE_RELEASE(ID3D11DeviceContext, _sapp_d3d11_device_context);
    _SAPP_SAFE_RELEASE(ID3D11Device, _sapp_d3d11_device);
}

_SOKOL_PRIVATE void _sapp_d3d11_create_default_render_target(void) {
    HRESULT hr;
    hr = IDXGISwapChain_GetBuffer(_sapp_dxgi_swap_chain, 0, &IID_ID3D11Texture2D, (void**)&_sapp_d3d11_rt);
    SOKOL_ASSERT(SUCCEEDED(hr) && _sapp_d3d11_rt);
    hr = ID3D11Device_CreateRenderTargetView(_sapp_d3d11_device, (ID3D11Resource*)_sapp_d3d11_rt, NULL, &_sapp_d3d11_rtv);
    SOKOL_ASSERT(SUCCEEDED(hr) && _sapp_d3d11_rtv);
    D3D11_TEXTURE2D_DESC ds_desc;
    memset(&ds_desc, 0, sizeof(ds_desc));
    ds_desc.Width = _sapp.framebuffer_width;
    ds_desc.Height = _sapp.framebuffer_height;
    ds_desc.MipLevels = 1;
    ds_desc.ArraySize = 1;
    ds_desc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
    ds_desc.SampleDesc = _sapp_dxgi_swap_chain_desc.SampleDesc;
    ds_desc.Usage = D3D11_USAGE_DEFAULT;
    ds_desc.BindFlags = D3D11_BIND_DEPTH_STENCIL;
    hr = ID3D11Device_CreateTexture2D(_sapp_d3d11_device, &ds_desc, NULL, &_sapp_d3d11_ds);
    SOKOL_ASSERT(SUCCEEDED(hr) && _sapp_d3d11_ds);
    D3D11_DEPTH_STENCIL_VIEW_DESC dsv_desc;
    memset(&dsv_desc, 0, sizeof(dsv_desc));
    dsv_desc.Format = ds_desc.Format;
    dsv_desc.ViewDimension = _sapp.sample_count > 1 ? D3D11_DSV_DIMENSION_TEXTURE2DMS : D3D11_DSV_DIMENSION_TEXTURE2D;
    hr = ID3D11Device_CreateDepthStencilView(_sapp_d3d11_device, (ID3D11Resource*)_sapp_d3d11_ds, &dsv_desc, &_sapp_d3d11_dsv);
    SOKOL_ASSERT(SUCCEEDED(hr) && _sapp_d3d11_dsv);
}

_SOKOL_PRIVATE void _sapp_d3d11_destroy_default_render_target(void) {
    _SAPP_SAFE_RELEASE(ID3D11Texture2D, _sapp_d3d11_rt);
    _SAPP_SAFE_RELEASE(ID3D11RenderTargetView, _sapp_d3d11_rtv);
    _SAPP_SAFE_RELEASE(ID3D11Texture2D, _sapp_d3d11_ds);
    _SAPP_SAFE_RELEASE(ID3D11DepthStencilView, _sapp_d3d11_dsv);
}

_SOKOL_PRIVATE void _sapp_d3d11_resize_default_render_target(void) {
    if (_sapp_dxgi_swap_chain) {
        _sapp_d3d11_destroy_default_render_target();
        IDXGISwapChain_ResizeBuffers(_sapp_dxgi_swap_chain, 1, _sapp.framebuffer_width, _sapp.framebuffer_height, DXGI_FORMAT_R8G8B8A8_UNORM, 0);
        _sapp_d3d11_create_default_render_target();
    }
}
#endif

#if defined(SOKOL_GLCORE33)
_SOKOL_PRIVATE void _sapp_wgl_init(void) {
    _sapp_opengl32 = LoadLibraryA("opengl32.dll");
    if (!_sapp_opengl32) {
        _sapp_fail("Failed to load opengl32.dll\n");
    }
    SOKOL_ASSERT(_sapp_opengl32);
    _sapp_wglCreateContext = (PFN_wglCreateContext) GetProcAddress(_sapp_opengl32, "wglCreateContext");
    SOKOL_ASSERT(_sapp_wglCreateContext);
    _sapp_wglDeleteContext = (PFN_wglDeleteContext) GetProcAddress(_sapp_opengl32, "wglDeleteContext");
    SOKOL_ASSERT(_sapp_wglDeleteContext);
    _sapp_wglGetProcAddress = (PFN_wglGetProcAddress) GetProcAddress(_sapp_opengl32, "wglGetProcAddress");
    SOKOL_ASSERT(_sapp_wglGetProcAddress);
    _sapp_wglGetCurrentDC = (PFN_wglGetCurrentDC) GetProcAddress(_sapp_opengl32, "wglGetCurrentDC");
    SOKOL_ASSERT(_sapp_wglGetCurrentDC);
    _sapp_wglMakeCurrent = (PFN_wglMakeCurrent) GetProcAddress(_sapp_opengl32, "wglMakeCurrent");
    SOKOL_ASSERT(_sapp_wglMakeCurrent);

    _sapp_win32_msg_hwnd = CreateWindowExW(WS_EX_OVERLAPPEDWINDOW,
        L"SOKOLAPP",
        L"sokol-app message window",
        WS_CLIPSIBLINGS|WS_CLIPCHILDREN,
        0, 0, 1, 1,
        NULL, NULL,
        GetModuleHandleW(NULL),
        NULL);
    if (!_sapp_win32_msg_hwnd) {
        _sapp_fail("Win32: failed to create helper window!\n");
    }
    ShowWindow(_sapp_win32_msg_hwnd, SW_HIDE);
    MSG msg;
    while (PeekMessageW(&msg, _sapp_win32_msg_hwnd, 0, 0, PM_REMOVE)) {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }
    _sapp_win32_msg_dc = GetDC(_sapp_win32_msg_hwnd);
    if (!_sapp_win32_msg_dc) {
        _sapp_fail("Win32: failed to obtain helper window DC!\n");
    }
}

_SOKOL_PRIVATE void _sapp_wgl_shutdown(void) {
    SOKOL_ASSERT(_sapp_opengl32 && _sapp_win32_msg_hwnd);
    DestroyWindow(_sapp_win32_msg_hwnd); _sapp_win32_msg_hwnd = 0;
    FreeLibrary(_sapp_opengl32); _sapp_opengl32 = 0;
}

_SOKOL_PRIVATE bool _sapp_wgl_has_ext(const char* ext, const char* extensions) {
    SOKOL_ASSERT(ext && extensions);
    const char* start = extensions;
    while (true) {
        const char* where = strstr(start, ext);
        if (!where) {
            return false;
        }
        const char* terminator = where + strlen(ext);
        if ((where == start) || (*(where - 1) == ' ')) {
            if (*terminator == ' ' || *terminator == '\0') {
                break;
            }
        }
        start = terminator;
    }
    return true;
}

_SOKOL_PRIVATE bool _sapp_wgl_ext_supported(const char* ext) {
    SOKOL_ASSERT(ext);
    if (_sapp_GetExtensionsStringEXT) {
        const char* extensions = _sapp_GetExtensionsStringEXT();
        if (extensions) {
            if (_sapp_wgl_has_ext(ext, extensions)) {
                return true;
            }
        }
    }
    if (_sapp_GetExtensionsStringARB) {
        const char* extensions = _sapp_GetExtensionsStringARB(_sapp_wglGetCurrentDC());
        if (extensions) {
            if (_sapp_wgl_has_ext(ext, extensions)) {
                return true;
            }
        }
    }
    return false;
}

_SOKOL_PRIVATE void _sapp_wgl_load_extensions(void) {
    SOKOL_ASSERT(_sapp_win32_msg_dc);
    PIXELFORMATDESCRIPTOR pfd;
    memset(&pfd, 0, sizeof(pfd));
    pfd.nSize = sizeof(pfd);
    pfd.nVersion = 1;
    pfd.dwFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER;
    pfd.iPixelType = PFD_TYPE_RGBA;
    pfd.cColorBits = 24;
    if (!SetPixelFormat(_sapp_win32_msg_dc, ChoosePixelFormat(_sapp_win32_msg_dc, &pfd), &pfd)) {
        _sapp_fail("WGL: failed to set pixel format for dummy context\n");
    }
    HGLRC rc = _sapp_wglCreateContext(_sapp_win32_msg_dc);
    if (!rc) {
        _sapp_fail("WGL: Failed to create dummy context\n");
    }
    if (!_sapp_wglMakeCurrent(_sapp_win32_msg_dc, rc)) {
        _sapp_fail("WGL: Failed to make context current\n");
    }
    _sapp_GetExtensionsStringEXT = (PFNWGLGETEXTENSIONSSTRINGEXTPROC) _sapp_wglGetProcAddress("wglGetExtensionsStringEXT");
    _sapp_GetExtensionsStringARB = (PFNWGLGETEXTENSIONSSTRINGARBPROC) _sapp_wglGetProcAddress("wglGetExtensionsStringARB");
    _sapp_CreateContextAttribsARB = (PFNWGLCREATECONTEXTATTRIBSARBPROC) _sapp_wglGetProcAddress("wglCreateContextAttribsARB");
    _sapp_SwapIntervalEXT = (PFNWGLSWAPINTERVALEXTPROC) _sapp_wglGetProcAddress("wglSwapIntervalEXT");
    _sapp_GetPixelFormatAttribivARB = (PFNWGLGETPIXELFORMATATTRIBIVARBPROC) _sapp_wglGetProcAddress("wglGetPixelFormatAttribivARB");
    _sapp_arb_multisample = _sapp_wgl_ext_supported("WGL_ARB_multisample");
    _sapp_arb_create_context = _sapp_wgl_ext_supported("WGL_ARB_create_context");
    _sapp_arb_create_context_profile = _sapp_wgl_ext_supported("WGL_ARB_create_context_profile");
    _sapp_ext_swap_control = _sapp_wgl_ext_supported("WGL_EXT_swap_control");
    _sapp_arb_pixel_format = _sapp_wgl_ext_supported("WGL_ARB_pixel_format");
    _sapp_wglMakeCurrent(_sapp_win32_msg_dc, 0);
    _sapp_wglDeleteContext(rc);
}

_SOKOL_PRIVATE int _sapp_wgl_attrib(int pixel_format, int attrib) {
    SOKOL_ASSERT(_sapp_arb_pixel_format);
    int value = 0;
    if (!_sapp_GetPixelFormatAttribivARB(_sapp_win32_dc, pixel_format, 0, 1, &attrib, &value)) {
        _sapp_fail("WGL: Failed to retrieve pixel format attribute\n");
    }
    return value;
}

_SOKOL_PRIVATE int _sapp_wgl_find_pixel_format(void) {
    SOKOL_ASSERT(_sapp_win32_dc);
    SOKOL_ASSERT(_sapp_arb_pixel_format);
    int native_count = _sapp_wgl_attrib(1, WGL_NUMBER_PIXEL_FORMATS_ARB);
    for (int i = 0; i < native_count; i++) {
        const int pf = i + 1;
        if (_sapp_arb_pixel_format) {
            if (!_sapp_wgl_attrib(pf, WGL_SUPPORT_OPENGL_ARB) || !_sapp_wgl_attrib(pf, WGL_DRAW_TO_WINDOW_ARB)) {
                continue;
            }
            if (_sapp_wgl_attrib(pf, WGL_PIXEL_TYPE_ARB) != WGL_TYPE_RGBA_ARB) {
                continue;
            }
            if (_sapp_wgl_attrib(pf, WGL_ACCELERATION_ARB) == WGL_NO_ACCELERATION_ARB) {
                continue;
            }
            if ((_sapp_wgl_attrib(pf, WGL_RED_BITS_ARB) == 8) &&
                (_sapp_wgl_attrib(pf, WGL_GREEN_BITS_ARB) == 8) &&
                (_sapp_wgl_attrib(pf, WGL_BLUE_BITS_ARB) == 8) &&
                (_sapp_wgl_attrib(pf, WGL_ALPHA_BITS_ARB) == 8) &&
                (_sapp_wgl_attrib(pf, WGL_DEPTH_BITS_ARB) == 24) &&
                (_sapp_wgl_attrib(pf, WGL_STENCIL_BITS_ARB) == 8) &&
                (_sapp_wgl_attrib(pf, WGL_DOUBLE_BUFFER_ARB) != 0))
            {
                if ((_sapp.sample_count > 1) && _sapp_arb_multisample) {
                    if (_sapp_wgl_attrib(pf, WGL_SAMPLES_ARB) != _sapp.sample_count) {
                        continue;
                    }
                }
                return pf;
            }
        }
    }
    return 0;
}

_SOKOL_PRIVATE void _sapp_wgl_create_context(void) {
    int pixel_format = _sapp_wgl_find_pixel_format();
    if (0 == pixel_format) {
        _sapp_fail("WGL: Didn't find matching pixel format.\n");
    }
    PIXELFORMATDESCRIPTOR pfd;
    if (!DescribePixelFormat(_sapp_win32_dc, pixel_format, sizeof(pfd), &pfd)) {
        _sapp_fail("WGL: Failed to retrieve PFD for selected pixel format!\n");
    }
    if (!SetPixelFormat(_sapp_win32_dc, pixel_format, &pfd)) {
        _sapp_fail("WGL: Failed to set selected pixel format!\n");
    }
    if (!_sapp_arb_create_context) {
        _sapp_fail("WGL: ARB_create_context required!\n");
    }
    if (!_sapp_arb_create_context_profile) {
        _sapp_fail("WGL: ARB_create_context_profile required!\n");
    }
    const int attrs[] = {
        WGL_CONTEXT_MAJOR_VERSION_ARB, 3,
        WGL_CONTEXT_MINOR_VERSION_ARB, 3,
        WGL_CONTEXT_FLAGS_ARB, WGL_CONTEXT_FORWARD_COMPATIBLE_BIT_ARB,
        WGL_CONTEXT_PROFILE_MASK_ARB, WGL_CONTEXT_CORE_PROFILE_BIT_ARB,
        0, 0
    };
    _sapp_gl_ctx = _sapp_CreateContextAttribsARB(_sapp_win32_dc, 0, attrs);
    if (!_sapp_gl_ctx) {
        const DWORD err = GetLastError();
        if (err == (0xc0070000 | ERROR_INVALID_VERSION_ARB)) {
            _sapp_fail("WGL: Driver does not support OpenGL version 3.3\n");
        }
        else if (err == (0xc0070000 | ERROR_INVALID_PROFILE_ARB)) {
            _sapp_fail("WGL: Driver does not support the requested OpenGL profile");
        }
        else if (err == (0xc0070000 | ERROR_INCOMPATIBLE_DEVICE_CONTEXTS_ARB)) {
            _sapp_fail("WGL: The share context is not compatible with the requested context");
        }
        else {
            _sapp_fail("WGL: Failed to create OpenGL context");
        }
    }
    _sapp_wglMakeCurrent(_sapp_win32_dc, _sapp_gl_ctx);
    if (_sapp_ext_swap_control) {
        /* FIXME: DwmIsCompositionEnabled() (see GLFW) */
        _sapp_SwapIntervalEXT(1);
    }
}

_SOKOL_PRIVATE void _sapp_wgl_destroy_context(void) {
    SOKOL_ASSERT(_sapp_gl_ctx);
    _sapp_wglDeleteContext(_sapp_gl_ctx);
    _sapp_gl_ctx = 0;
}

_SOKOL_PRIVATE void _sapp_wgl_swap_buffers(void) {
    SOKOL_ASSERT(_sapp_win32_dc);
    /* FIXME: DwmIsCompositionEnabled? (see GLFW) */
    SwapBuffers(_sapp_win32_dc);
}
#endif

int WINAPI WinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPSTR lpCmdLine, _In_ int nCmdShow) {
    sapp_desc desc = sokol_main(__argc, __argv);
    _sapp_init_state(&desc, __argc, __argv);
    _sapp_win32_init_keytable();
    _sapp_win32_utf8_to_wide(_sapp.window_title, _sapp.window_title_wide, sizeof(_sapp.window_title_wide));
    _sapp_win32_init_dpi();
    _sapp_win32_create_window();
    #if defined(SOKOL_D3D11) 
        _sapp_d3d11_create_device_and_swapchain();
        _sapp_d3d11_create_default_render_target();
    #endif
    #if defined(SOKOL_GLCORE33)
        _sapp_wgl_init();
        _sapp_wgl_load_extensions();
        _sapp_wgl_create_context();
        #if !defined(SOKOL_WIN32_NO_GL_LOADER)
            _sapp_win32_gl_loadfuncs();
        #endif
    #endif
    _sapp.valid = true;

    bool done = false;
    RECT rect;
    while (!done) {
        if (GetClientRect(_sapp_win32_hwnd, &rect)) {
            const int cur_width = (rect.right - rect.left) / _sapp_win32_window_scale;
            const int cur_height = (rect.bottom - rect.top) / _sapp_win32_window_scale;
            if ((cur_width != _sapp.window_width) || (cur_height != _sapp.window_height)) {
                _sapp.window_width = cur_width;
                _sapp.window_height = cur_height;
                _sapp.framebuffer_width = _sapp.window_width * _sapp_win32_content_scale;
                _sapp.framebuffer_height = _sapp.window_height * _sapp_win32_content_scale;
                #if defined(SOKOL_D3D11)
                _sapp_d3d11_resize_default_render_target();
                #endif
            }
        }
        _sapp_frame();
        #if defined(SOKOL_D3D11)
            IDXGISwapChain_Present(_sapp_dxgi_swap_chain, 1, 0);
        #endif
        #if defined(SOKOL_GLCORE33)
            _sapp_wgl_swap_buffers();
        #endif
        MSG msg;
        while (PeekMessageW(&msg, NULL, 0, 0, PM_REMOVE)) {
            if (WM_QUIT == msg.message) {
                done = true;
            }
            else {
                TranslateMessage(&msg);
                DispatchMessage(&msg);
            }
        }
    }
    _sapp.desc.cleanup_cb();

    #if defined(SOKOL_D3D11)
        _sapp_d3d11_destroy_default_render_target();
        _sapp_d3d11_destroy_device_and_swapchain();
    #else
        _sapp_wgl_destroy_context();
        _sapp_wgl_shutdown();
    #endif
    _sapp_win32_destroy_window();
    return 0;
}
 
#undef _SAPP_SAFE_RELEASE
#endif

/*== PUBLIC API FUNCTIONS ====================================================*/
bool sapp_isvalid(void) {
    return _sapp.valid;
}

int sapp_width(void) {
    return _sapp.framebuffer_width;
}

int sapp_height(void) {
    return _sapp.framebuffer_height;
}

bool sapp_high_dpi(void) {
    return _sapp.desc.high_dpi && (_sapp.dpi_scale > 1.5f);
}

float sapp_dpi_scale(void) {
    return _sapp.dpi_scale;
}

bool sapp_gles2(void) {
    return _sapp.gles2_fallback;
}

const void* sapp_metal_get_device(void) {
    SOKOL_ASSERT(_sapp.valid);
    #if defined(SOKOL_METAL)
        const void* obj = (__bridge const void*) _sapp_mtl_device_obj;
        SOKOL_ASSERT(obj);
        return obj;
    #else
        return 0;
    #endif
}

const void* sapp_metal_get_renderpass_descriptor(void) {
    SOKOL_ASSERT(_sapp.valid);
    #if defined(SOKOL_METAL)
        const void* obj =  (__bridge const void*) [_sapp_view_obj currentRenderPassDescriptor];
        SOKOL_ASSERT(obj);
        return obj;
    #else
        return 0;
    #endif
}

const void* sapp_metal_get_drawable(void) {
    SOKOL_ASSERT(_sapp.valid);
    #if defined(SOKOL_METAL)
        const void* obj = (__bridge const void*) [_sapp_view_obj currentDrawable];
        SOKOL_ASSERT(obj);
        return obj;
    #else
        return 0;
    #endif
}

const void* sapp_d3d11_get_device(void) {
    SOKOL_ASSERT(_sapp.valid);
    #if defined(SOKOL_D3D11)
        return _sapp_d3d11_device;
    #else
        return 0;
    #endif
}

const void* sapp_d3d11_get_device_context(void) {
    SOKOL_ASSERT(_sapp.valid);
    #if defined(SOKOL_D3D11)
        return _sapp_d3d11_device_context;
    #else
        return 0;
    #endif
}

const void* sapp_d3d11_get_render_target_view(void) {
    SOKOL_ASSERT(_sapp.valid);
    #if defined(SOKOL_D3D11)
        return _sapp_d3d11_rtv;
    #else
        return 0;
    #endif
}

const void* sapp_d3d11_get_depth_stencil_view(void) {
    SOKOL_ASSERT(_sapp.valid);
    #if defined(SOKOL_D3D11)
        return _sapp_d3d11_dsv;
    #else
        return 0;
    #endif
}

#undef _sapp_def

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* SOKOL_IMPL */
