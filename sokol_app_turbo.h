#if defined(SOKOL_IMPL) && !defined(SOKOL_APP_TURBO_IMPL)
#define SOKOL_APP_TURBO_IMPL
#endif
#ifndef SOKOL_APP_TURBO_INCLUDED
/*
    SOKOL_APP_TURBO.h    -- simple cross-platform time measurement

    Project URL: https://github.com/floooh/sokol

    Do this:
        #define SOKOL_IMPL or
        #define SOKOL_APP_TURBO_IMPL
    before you include this file in *one* C or C++ file to create the
    implementation.

    Optionally provide the following defines with your own implementations:
    SOKOL_ASSERT(c)     - your own assert macro (default: assert(c))
    SOKOL_APP_TURBO_API_DECL - public function declaration prefix (default: extern)
    SOKOL_API_DECL      - same as SOKOL_APP_TURBO_API_DECL
    SOKOL_API_IMPL      - public function implementation prefix (default: -)

    If SOKOL_APP_TURBO.h is compiled as a DLL, define the following before
    including the declaration or implementation:

    SOKOL_DLL

    On Windows, SOKOL_DLL will define SOKOL_APP_TURBO_API_DECL as __declspec(dllexport)
    or __declspec(dllimport) as needed.

    void stm_setup();
        Call once before any other functions to initialize SOKOL_APP_TURBO
        (this calls for instance QueryPerformanceFrequency on Windows)

    uint64_t stm_now();
        Get current point in time in unspecified 'ticks'. The value that
        is returned has no relation to the 'wall-clock' time and is
        not in a specific time unit, it is only useful to compute
        time differences.

    zlib/libpng license

    Copyright (c) 2025 Michaël Palomas

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
#define SOKOL_APP_TURBO_INCLUDED (1)
#include <stdint.h>
#include "sokol_app.h"

#if defined(SOKOL_API_DECL) && !defined(SOKOL_APP_TURBO_API_DECL)
#define SOKOL_APP_TURBO_API_DECL SOKOL_API_DECL
#endif
#ifndef SOKOL_APP_TURBO_API_DECL
#if defined(_WIN32) && defined(SOKOL_DLL) && defined(SOKOL_APP_TURBO_IMPL)
#define SOKOL_APP_TURBO_API_DECL __declspec(dllexport)
#elif defined(_WIN32) && defined(SOKOL_DLL)
#define SOKOL_APP_TURBO_API_DECL __declspec(dllimport)
#else
#define SOKOL_APP_TURBO_API_DECL extern
#endif
#endif

#ifdef __cplusplus
extern "C" {
#endif

SOKOL_APP_TURBO_API_DECL void sapp_setup(const sapp_desc* desc);
SOKOL_APP_TURBO_API_DECL void sapp_shutdown(void);
SOKOL_APP_TURBO_API_DECL bool sapp_should_close(void);
SOKOL_APP_TURBO_API_DECL void sapp_poll_events(void);

SOKOL_APP_TURBO_API_DECL void sapp_begin_tick(void);
SOKOL_APP_TURBO_API_DECL void sapp_end_tick(void);

#ifdef __cplusplus
} /* extern "C" */
#endif
#endif // SOKOL_APP_TURBO_INCLUDED

/*-- IMPLEMENTATION ----------------------------------------------------------*/
#ifdef SOKOL_APP_TURBO_IMPL
#define SOKOL_APP_TURBO_IMPL_INCLUDED (1)
#include <string.h> /* memset */

#ifndef SOKOL_API_IMPL
    #define SOKOL_API_IMPL
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

#if defined(_WIN32)
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
typedef struct {
    uint32_t initialized;
    LARGE_INTEGER freq;
    LARGE_INTEGER start;
} _sat_state_t;
#elif defined(__APPLE__) && defined(__MACH__)
#include <mach/mach_time.h>
typedef struct {
    uint32_t initialized;
    mach_timebase_info_data_t timebase;
    uint64_t start;
} _sat_state_t;
#elif defined(__EMSCRIPTEN__)
#include <emscripten/emscripten.h>
typedef struct {
    uint32_t initialized;
    double start;
} _sat_state_t;
#else /* anything else, this will need more care for non-Linux platforms */
#ifdef ESP8266
// On the ESP8266, clock_gettime ignores the first argument and CLOCK_MONOTONIC isn't defined
#define CLOCK_MONOTONIC 0
#endif
#include <time.h>
typedef struct {
    uint32_t initialized;
    uint64_t start;
} _sat_state_t;
#endif
static _sat_state_t _sat;


#if defined(_WIN32)

_SOKOL_PRIVATE void _sapp_windows_setup(const sapp_desc* desc) {
    (void)desc;
}

_SOKOL_PRIVATE void _sapp_windows_shutdown(void) {
    
}

_SOKOL_PRIVATE void _sapp_windows_poll_events(void) {

}

_SOKOL_PRIVATE void _sapp_windows_begin_tick(void) {

}

_SOKOL_PRIVATE void _sapp_windows_end_tick(void) {

}

#elif defined(__APPLE__) && defined(__MACH__)

_SOKOL_PRIVATE void _sapp_macos_setup(const sapp_desc* desc) {
    (void)desc;
    _sapp_init_state(desc);
    _sapp_macos_init_keytable();
    [NSApplication sharedApplication];
    [NSApp setActivationPolicy:NSApplicationActivationPolicyRegular];

    // set the application dock icon as early as possible, otherwise
    // the dummy icon will be visible for a short time
    sapp_set_icon(&_sapp.desc.icon);
    _sapp.macos.app_dlg = [[_sapp_macos_app_delegate alloc] init];
    NSApp.delegate = _sapp.macos.app_dlg;

    // workaround for "no key-up sent while Cmd is pressed" taken from GLFW:
    NSEvent* (^keyup_monitor)(NSEvent*) = ^NSEvent* (NSEvent* event) {
        if ([event modifierFlags] & NSEventModifierFlagCommand) {
            [[NSApp keyWindow] sendEvent:event];
        }
        return event;
    };
    _sapp.macos.keyup_monitor = [NSEvent addLocalMonitorForEventsMatchingMask:NSEventMaskKeyUp handler:keyup_monitor];

    // Manually create the window (normally done in applicationDidFinishLaunching)
    _sapp_macos_init_cursors();
    if ((_sapp.window_width == 0) || (_sapp.window_height == 0)) {
        // use 4/5 of screen size as default size
        NSRect screen_rect = NSScreen.mainScreen.frame;
        if (_sapp.window_width == 0) {
            _sapp.window_width = (int)((screen_rect.size.width * 4.0f) / 5.0f);
        }
        if (_sapp.window_height == 0) {
            _sapp.window_height = (int)((screen_rect.size.height * 4.0f) / 5.0f);
        }
    }
    const NSUInteger style =
        NSWindowStyleMaskTitled |
        NSWindowStyleMaskClosable |
        NSWindowStyleMaskMiniaturizable |
        NSWindowStyleMaskResizable;
    NSRect window_rect = NSMakeRect(0, 0, _sapp.window_width, _sapp.window_height);
    _sapp.macos.window = [[_sapp_macos_window alloc]
        initWithContentRect:window_rect
        styleMask:style
        backing:NSBackingStoreBuffered
        defer:NO];
    _sapp.macos.window.releasedWhenClosed = NO;
    _sapp.macos.window.title = [NSString stringWithUTF8String:_sapp.window_title];
    _sapp.macos.window.acceptsMouseMovedEvents = YES;
    _sapp.macos.window.restorable = YES;

    _sapp.macos.win_dlg = [[_sapp_macos_window_delegate alloc] init];
    _sapp.macos.window.delegate = _sapp.macos.win_dlg;
    #if defined(SOKOL_METAL)
        _sapp_macos_mtl_init();
    #elif defined(SOKOL_GLCORE)
        _sapp_macos_gl_init(window_rect);
    #elif defined(SOKOL_WGPU)
        _sapp_macos_wgpu_init();
    #endif
    _sapp.macos.window.contentView = _sapp.macos.view;
    [_sapp.macos.window makeFirstResponder:_sapp.macos.view];
    [_sapp.macos.window center];
    _sapp.valid = true;
    if (_sapp.fullscreen) {
        [_sapp.macos.window toggleFullScreen:nil];
    }
    [NSApp activateIgnoringOtherApps:YES];
    [_sapp.macos.window makeKeyAndOrderFront:nil];
    _sapp_macos_update_dimensions();
    [NSEvent setMouseCoalescingEnabled:NO];

    // workaround for window not being focused during a long init callback
    NSEvent *focusevent = [NSEvent otherEventWithType:NSEventTypeAppKitDefined
        location:NSZeroPoint
        modifierFlags:0x40
        timestamp:0
        windowNumber:0
        context:nil
        subtype:NSEventSubtypeApplicationActivated
        data1:0
        data2:0];
    [NSApp postEvent:focusevent atStart:YES];

    // vsync off => swap interval to 0
    GLint swapInt = 1;
    NSOpenGLContext* ctx = [_sapp.macos.view openGLContext];
    [ctx setValues:&swapInt forParameter:NSOpenGLContextParameterSwapInterval];
    [ctx makeCurrentContext];
}

_SOKOL_PRIVATE void _sapp_macos_shutdown(void) {

}

_SOKOL_PRIVATE void _sapp_macos_poll_events(void) {
    NSEvent* event;
    while ((event = [NSApp nextEventMatchingMask:NSEventMaskAny
                                             untilDate:nil
                                                inMode:NSDefaultRunLoopMode
                                               dequeue:YES])) {
        [NSApp sendEvent:event];
    }
}

_SOKOL_PRIVATE void _sapp_macos_begin_tick(void) {

}

_SOKOL_PRIVATE void _sapp_macos_end_tick(void) {

}

#elif defined(__EMSCRIPTEN__)

#else // Linux and similar

_SOKOL_PRIVATE void _sapp_linux_setup(const sapp_desc* desc) {
    (void)_sat;
    /* The following lines are here to trigger a linker error instead of an
        obscure runtime error if the user has forgotten to add -pthread to
        the compiler or linker options. They have no other purpose.
    */
    pthread_attr_t pthread_attr;
    pthread_attr_init(&pthread_attr);
    pthread_attr_destroy(&pthread_attr);

    _sapp_init_state(desc);
    _sapp.x11.window_state = NormalState;

    XInitThreads();
    XrmInitialize();
    _sapp.x11.display = XOpenDisplay(NULL);
    if (!_sapp.x11.display) {
        _SAPP_PANIC(LINUX_X11_OPEN_DISPLAY_FAILED);
    }
    _sapp.x11.screen = DefaultScreen(_sapp.x11.display);
    _sapp.x11.root = DefaultRootWindow(_sapp.x11.display);
    _sapp_x11_query_system_dpi();
    // NOTE: on Linux system-window-size to frame-buffer-size mapping is always 1:1
    _sapp.dpi_scale = _sapp.x11.dpi / 96.0f;
    _sapp_x11_init_extensions();
    _sapp_x11_create_standard_cursors();
    XkbSetDetectableAutoRepeat(_sapp.x11.display, true, NULL);
    _sapp_x11_init_keytable();
    #if defined(_SAPP_GLX)
        _sapp_glx_init();
        Visual* visual = 0;
        int depth = 0;
        _sapp_glx_choose_visual(&visual, &depth);
        _sapp_x11_create_window(visual, depth);
        _sapp_glx_create_context();
        // _sapp_glx_swapinterval(_sapp.swap_interval);
        // vsync off => swap interval to 0
        _sapp_glx_swapinterval(0);
    #elif defined(_SAPP_EGL)
        _sapp_egl_init();
    #elif defined(SOKOL_WGPU)
        _sapp_x11_create_window(0, 0);
        _sapp_wgpu_init();
    #endif
    sapp_set_icon(&desc->icon);
    _sapp.valid = true;
    _sapp_x11_show_window();
    if (_sapp.fullscreen) {
        _sapp_x11_set_fullscreen(true);
    }

    XFlush(_sapp.x11.display);
}

_SOKOL_PRIVATE void _sapp_linux_poll_events(void) {
    while (!_sapp.quit_ordered) {
        _sapp_timing_measure(&_sapp.timing);
        int count = XPending(_sapp.x11.display);
        while (count--) {
            XEvent event;
            XNextEvent(_sapp.x11.display, &event);
            _sapp_x11_process_event(&event);
        }
        _sapp_linux_frame();
        XFlush(_sapp.x11.display);
        // handle quit-requested, either from window or from sapp_request_quit()
        if (_sapp.quit_requested && !_sapp.quit_ordered) {
            // give user code a chance to intervene
            _sapp_x11_app_event(SAPP_EVENTTYPE_QUIT_REQUESTED);
            /* if user code hasn't intervened, quit the app */
            if (_sapp.quit_requested) {
                _sapp.quit_ordered = true;
            }
        }
    }
}

_SOKOL_PRIVATE void _sapp_linux_begin_tick(void) {
    _sapp_timing_measure(&_sapp.timing);
    int count = XPending(_sapp.x11.display);
    while (count--) {
        XEvent event;
        XNextEvent(_sapp.x11.display, &event);
        _sapp_x11_process_event(&event);
    }
}

_SOKOL_PRIVATE void _sapp_linux_end_tick(void) {
    XFlush(_sapp.x11.display);
    // handle quit-requested, either from window or from sapp_request_quit()
    if (_sapp.quit_requested && !_sapp.quit_ordered) {
        // give user code a chance to intervene
        _sapp_x11_app_event(SAPP_EVENTTYPE_QUIT_REQUESTED);
        /* if user code hasn't intervened, quit the app */
        if (_sapp.quit_requested) {
            _sapp.quit_ordered = true;
        }
    }
}

_SOKOL_PRIVATE void _sapp_linux_shutdown(void) {
    _sapp_call_cleanup();
    #if defined(_SAPP_GLX)
        _sapp_glx_destroy_context();
    #elif defined(_SAPP_EGL)
        _sapp_egl_destroy();
    #elif defined(SOKOL_WGPU)
        _sapp_wgpu_discard();
    #endif
    _sapp_x11_destroy_window();
    _sapp_x11_destroy_standard_cursors();
    XCloseDisplay(_sapp.x11.display);
    _sapp_discard_state();
}

#endif


/* prevent 64-bit overflow when computing relative timestamp
    see https://gist.github.com/jspohr/3dc4f00033d79ec5bdaf67bc46c813e3
*/
#if defined(_WIN32) || (defined(__APPLE__) && defined(__MACH__))
_SOKOL_PRIVATE int64_t _sat_int64_muldiv(int64_t value, int64_t numer, int64_t denom) {
    int64_t q = value / denom;
    int64_t r = value % denom;
    return q * numer + r * numer / denom;
}
#endif

SOKOL_API_IMPL void sapp_setup(const sapp_desc* desc) {
    (void)desc;
    #if defined(_WIN32)
    _sapp_windows_setup(desc);
    #elif defined(__APPLE__) && defined(__MACH__)
    _sapp_macos_setup(desc);    
    #elif defined(__EMSCRIPTEN__)
        
    #else
    _sapp_linux_setup(desc);
    #endif
}

SOKOL_API_IMPL void sapp_shutdown(void) {
    #if defined(_WIN32)
    _sapp_windows_shutdown();    
    #elif defined(__APPLE__) && defined(__MACH__)
    _sapp_macos_shutdown();
    #elif defined(__EMSCRIPTEN__)
    
    #else
    _sapp_linux_shutdown();
    #endif
}

SOKOL_API_IMPL bool sapp_should_close(void) {
    return _sapp.quit_ordered;
}

SOKOL_API_IMPL void sapp_poll_events(void) {
#if defined(_WIN32)
    _sapp_windows_poll_events();    
#elif defined(__APPLE__) && defined(__MACH__)
    _sapp_macos_poll_events();
#elif defined(__EMSCRIPTEN__)
    
#else
    _sapp_linux_poll_events();
#endif
}

SOKOL_API_IMPL void sapp_begin_tick(void) {
    #if defined(_WIN32)
    _sapp_windows_begin_tick();    
    #elif defined(__APPLE__) && defined(__MACH__)
    _sapp_macos_begin_tick();
    #elif defined(__EMSCRIPTEN__)
    
    #else
    _sapp_linux_begin_tick();
    #endif
}

SOKOL_API_IMPL void sapp_end_tick(void) {
    #if defined(_WIN32)
    _sapp_windows_end_tick();    
    #elif defined(__APPLE__) && defined(__MACH__)
    _sapp_macos_end_tick();
    #elif defined(__EMSCRIPTEN__)
    
    #else
    _sapp_linux_end_tick();
    #endif
}

#endif /* SOKOL_APP_TURBO_IMPL */

