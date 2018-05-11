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
    const char* window_title;
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

/* user-provided functions */
extern void sokol_enter();
extern void sokol_frame();
extern int sokol_exit();

extern void sapp_setup(const sapp_desc* desc);
extern void sapp_shutdown();
extern bool sapp_isvalid();

#ifdef __cplusplus
} /* extern "C" */
#endif

/*-- IMPLEMENTATION ----------------------------------------------------------*/
#ifdef SOKOL_IMPL

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
#ifndef _SOKOL_PRIVATE
    #if defined(__GNUC__)
        #define _SOKOL_PRIVATE __attribute__((unused)) static
    #else
        #define _SOKOL_PRIVATE static
    #endif
#endif

#ifdef __cplusplus
extern "C"
#endif

/* helper macros */
#define _sapp_def(val, def) (((val) == 0) ? (def) : (val))

enum {
    _SAPP_MAX_TITLE_LENGTH = 128,
};

typedef struct {
    bool valid;
    int width;
    int height;
    int sample_count;
    char window_title[_SAPP_MAX_TITLE_LENGTH];
} _sapp_state;
static _sapp_state _sapp;

/*== MacOS ===================================================================*/

#if defined(__APPLE__)

#if !__has_feature(objc_arc)
#error "sokol_app.h requires ARC (Automatic Reference Counting) on MacOS and iOS"
#endif

#include <TargetConditionals.h>
#if !TARGET_OS_IPHONE
#import <Cocoa/Cocoa.h>
#import <QuartzCore/QuartzCore.h>

static id _sapp_window;
static id _sapp_win_dlg;
static id _sapp_app_dlg;

@interface _sapp_app_delegate : NSObject<NSApplicationDelegate>
@end
@interface _sapp_window_delegate : NSObject<NSWindowDelegate>
@end 

@implementation _sapp_app_delegate
- (void)applicationDidFinishLaunching:(NSNotification*)aNotification {
    const NSUInteger style =
        NSWindowStyleMaskTitled |
        NSWindowStyleMaskClosable |
        NSWindowStyleMaskMiniaturizable |
        NSWindowStyleMaskResizable;
    _sapp_window = [[NSWindow alloc]
        initWithContentRect:NSMakeRect(0, 0, _sapp.width, _sapp.height)
        styleMask:style
        backing:NSBackingStoreBuffered
        defer:NO];
    [_sapp_window setTitle:[NSString stringWithUTF8String:_sapp.window_title]];
    [_sapp_window setAcceptsMouseMovedEvents:YES];
    [_sapp_window center];
    [_sapp_window setRestorable:YES];
    _sapp_win_dlg = [[_sapp_window_delegate alloc] init];
    [_sapp_window setDelegate:_sapp_win_dlg];

    [_sapp_window makeKeyAndOrderFront:nil];
}
- (BOOL)applicationShouldTerminateAfterLastWindowClosed:(NSApplication*)sender {
    return YES;
}
@end

@implementation _sapp_window_delegate
- (BOOL)windowShouldClose:(id)sender {
    sokol_exit();
    return YES;
}

- (void)windowDidResize:(NSNotification*)notification {
    /* FIXME */
}

- (void)windowDidMove:(NSNotification*)notification {
    /* FIXME */
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

_SOKOL_PRIVATE void _sapp_setup(const sapp_desc* desc) {
    _sapp.width = _sapp_def(desc->width, 640);
    _sapp.height = _sapp_def(desc->height, 480);
    _sapp.sample_count = _sapp_def(desc->sample_count, 1);
    strncpy(_sapp.window_title, desc->window_title, sizeof(_sapp.window_title));
    _sapp.window_title[_SAPP_MAX_TITLE_LENGTH-1] = 0;
    [NSApplication sharedApplication];
    [NSApp setActivationPolicy:NSApplicationActivationPolicyRegular];
    _sapp_app_dlg = [[_sapp_app_delegate alloc] init];
    [NSApp setDelegate:_sapp_app_dlg];
    [NSApp activateIgnoringOtherApps:YES];
}

_SOKOL_PRIVATE void _sapp_run() {
    SOKOL_ASSERT(_sapp.valid);
    [NSApp run];
}

_SOKOL_PRIVATE void _sapp_shutdown() {
    /* FIXME */
}
#endif

#if TARGET_OS_IPHONE

#endif

/* OSX/iOS entry function */
int main() {
    sokol_enter();
    _sapp_run();
    return 0;
}

#else
#error "sokol_app.h: Unknown OS"
#endif

/*== PUBLIC API FUNCTIONS ====================================================*/
void sapp_setup(const sapp_desc* desc) {
    SOKOL_ASSERT(desc);
    memset(&_sapp, 0, sizeof(_sapp));
    _sapp_setup(desc);
    _sapp.valid = true;
}

bool sapp_isvalid() {
    return _sapp.valid;
}

void sapp_shutdown() {
    _sapp_shutdown();
    _sapp.valid = false;
}

#undef _sapp_def

#endif /* SOKOL_IMPL */

#ifdef __cplusplus
} /* extern "C" */
#endif
