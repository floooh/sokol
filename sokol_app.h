#pragma once
/*
    sokol_app.h -- cross-platform application wrapper

    WORK IN PROGRESS!

    Do this:
        #define SOKOL_IMPL
    before you include this file in *one* C or C++ file to create the 
    implementation.

    Optionally provide the following defines with your own implementations:

    SOKOL_ASSERT(c)     - your own assert macro (default: assert(c))
    SOKOL_LOG(msg)      - your own logging function (default: puts(msg))
    SOKOL_UNREACHABLE() - a guard macro for unreachable code (default: assert(false))

    Optionally define the following to force debug checks and validations
    even in release mode:

    SOKOL_DEBUG         - by default this is defined if _DEBUG is defined

    TODO: documentation

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
    SAPP_MAX_TOUCHPOINTS = 8,
};

typedef struct {
    int width;
    int height;
    int sample_count;
    bool fullscreen;
    bool alpha;
    bool depth;
    bool stencil;
    bool premultiplied_alpha;
    bool preserve_drawing_buffer;
    const char* title;
} sapp_desc;

typedef enum {
    SAPP_EVENTTYPE_INVALID,
    SAPP_EVENTTYPE_KEY_DOWN,
    SAPP_EVENTTYPE_KEY_UP,
    SAPP_EVENTTYPE_CHAR,
    SAPP_EVENTTYPE_MOUSE_DOWN,
    SAPP_EVENTTYPE_MOUSE_UP,
    SAPP_EVENTTYPE_MOUSE_SCROLL,
    SAPP_EVENTTYPE_MOUSE_MOVE,
    SAPP_EVENTTYPE_TOUCH_BEGAN,
    SAPP_EVENTTYPE_TOUCH_ENDED,
    SAPP_EVENTTYPE_TOUCH_CANCELLED,
    _SAPP_EVENTTYPE_NUM,
    _SAPP_EVENTTYPE_FORCE_U32 = 0x7FFFFFF
} sapp_event_type;

typedef struct {
    uintptr_t id;
    float pos_x;
    float pos_y;
    bool changed;
} sapp_touchpoint;

enum {
    SAPP_MODIFIER_SHIFT = (1<<0),
    SAPP_MODIFIER_CTRL = (1<<1),
    SAPP_MODIFIER_ALT = (1<<2),
    SAPP_MODIFIER_SUPER = (1<<3)
};

typedef struct {
    sapp_event_type type;
    uint32_t frame_count;
    uint32_t key_code;
    uint32_t char_code;
    uint32_t mouse_button;
    uint32_t modifiers;
    float mouse_x;
    float mouse_y;
    float scroll_x;
    float scroll_y;
    int num_touches;
    sapp_touchpoint touches[SAPP_MAX_TOUCHPOINTS];
} sapp_event;

typedef void (*sapp_event_callback)(const sapp_event*);

extern void sapp_setup(const sapp_desc* desc);
extern void sapp_shutdown();
extern bool sapp_isvalid();
extern bool sapp_process_events();
extern void sapp_present();
extern void sapp_on_event(sapp_event_callback cb);

/*-- IMPLEMENTATION ----------------------------------------------------------*/
#ifdef SOKOL_IMPL
#ifndef SOKOL_ASSERT
    #include <assert.h>
    #define SOKOL_ASSERT(c) assert(c)
#endif

#endif /* SOKOL_IMPL */

#ifdef __cplusplus
} /* extern "C" */
#endif