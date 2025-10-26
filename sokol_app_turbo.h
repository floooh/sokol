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

// C11 Static assertions for compile-time validation
_Static_assert(sizeof(uint32_t) == 4, "uint32_t must be 4 bytes");
_Static_assert(sizeof(uint64_t) == 8, "uint64_t must be 8 bytes");
_Static_assert(sizeof(void*) >= sizeof(uintptr_t), "Pointer size assumption failed");

// Display/Monitor information
typedef struct sapp_display {
    int width_mm;               // Physical width in millimeters
    int height_mm;              // Physical height in millimeters
    int width_px;               // Current resolution width in pixels
    int height_px;              // Current resolution height in pixels
    int refresh_rate;           // Refresh rate in Hz
    float dpi_scale;            // DPI scale factor
    const char* name;           // Display name (may be NULL)
    bool is_primary;            // True if this is the primary display
    int pos_x;                  // Display position X in virtual screen
    int pos_y;                  // Display position Y in virtual screen
} sapp_display;

_Static_assert(sizeof(sapp_display) <= 128, "Display struct should remain reasonably sized");

#ifndef SAPP_MAX_DISPLAYS
#define SAPP_MAX_DISPLAYS (4)
#endif

// Extended sapp API to drive the main loop
SOKOL_APP_TURBO_API_DECL void sapp_setup(const sapp_desc* desc);
SOKOL_APP_TURBO_API_DECL void sapp_shutdown(void);
SOKOL_APP_TURBO_API_DECL bool sapp_should_close(void);
SOKOL_APP_TURBO_API_DECL void sapp_begin_tick(void);
SOKOL_APP_TURBO_API_DECL void sapp_end_tick(void);

// Extended sapp display API functions
SOKOL_APP_TURBO_API_DECL const sapp_display* sapp_display_get_primary(void);
SOKOL_APP_TURBO_API_DECL const sapp_display* sapp_display_get_window_display(void);
SOKOL_APP_TURBO_API_DECL int sapp_display_get_count(void);
SOKOL_APP_TURBO_API_DECL const sapp_display* sapp_display_get_at_index(int index);

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


typedef struct {
    uint64_t poll_count;
    uint64_t begin_count;
    uint64_t end_count;

    sapp_display displays[SAPP_MAX_DISPLAYS];
    int display_count;
    int window_display_index;
} _sat_state_t;

static _sat_state_t _sat = {
    .poll_count = 0,
    .begin_count = 0,
    .end_count = 0,
    .display_count = 0,
    .window_display_index = 0,
};


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

    #if defined(_SAPP_ANY_GL)
    // vsync off => swap interval to 0
    GLint swapInt = 0;
    NSOpenGLContext* ctx = [_sapp.macos.view openGLContext];
    [ctx setValues:&swapInt forParameter:NSOpenGLContextParameterSwapInterval];
    [ctx makeCurrentContext];
    #endif
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
    _sapp_timing_measure(&_sapp.timing);
    NSEvent* event;
    while ((event = [NSApp nextEventMatchingMask:NSEventMaskAny
                                             untilDate:nil
                                                inMode:NSDefaultRunLoopMode
                                               dequeue:YES])) {
        [NSApp sendEvent:event];
    }
    // first part of _sapp_macos_frame()

    // NOTE: DO NOT call _sapp_macos_update_dimensions() function from within the
    // frame callback (at least when called from MTKView's drawRect function).
    // This will trigger a chicken-egg situation that triggers a
    // Metal validation layer error about different render target sizes.
    #if defined(_SAPP_ANY_GL)
    glGetIntegerv(GL_FRAMEBUFFER_BINDING, (GLint*)&_sapp.gl.framebuffer);
    #endif

    // parts of _sapp_frame in charge of calling init callback
    if (_sapp.first_frame) {
        _sapp.first_frame = false;
        _sapp_call_init();
        #if defined(_SAPP_ANY_GL)
        // vsync off => swap interval to 0
        GLint swapInt = 0;
        NSOpenGLContext* ctx = [_sapp.macos.view openGLContext];
        [ctx setValues:&swapInt forParameter:NSOpenGLContextParameterSwapInterval];
        #endif
    }
    _sapp.frame_count++;
}

_SOKOL_PRIVATE void _sapp_macos_end_tick(void) {
    // second part of _sapp_macos_frame()
    #if defined(_SAPP_ANY_GL)
    [[_sapp.macos.view openGLContext] flushBuffer];
    [[_sapp.macos.view openGLContext] update];
    #endif
    if (_sapp.quit_requested || _sapp.quit_ordered) {
        [_sapp.macos.window performClose:nil];
    }
}

#elif defined(__EMSCRIPTEN__)

#else // Linux and similar

#include <X11/extensions/Xrandr.h>

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

_SOKOL_PRIVATE void _sapp_linux_begin_tick(void) {
    _sapp_timing_measure(&_sapp.timing);
    int count = XPending(_sapp.x11.display);
    while (count--) {
        XEvent event;
        XNextEvent(_sapp.x11.display, &event);
        _sapp_x11_process_event(&event);
    }
    // first part of _sapp_linux_frame()
    _sapp_x11_update_dimensions_from_window_size();

    // parts of _sapp_frame in charge of calling init callback
    if (_sapp.first_frame) {
        _sapp.first_frame = false;
        _sapp_call_init();
    }
    _sapp.frame_count++;
}

_SOKOL_PRIVATE void _sapp_linux_end_tick(void) {
    // second part of _sapp_linux_frame()
    #if defined(_SAPP_GLX)
        _sapp_glx_swap_buffers();
    #elif defined(_SAPP_EGL)
        eglSwapBuffers(_sapp.egl.display, _sapp.egl.surface);
    #endif

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

// Display API

_SOKOL_PRIVATE void _sapp_linux_init_displays(void) {
    if (!_sapp.x11.display) return;

    _sat.display_count = 0;

    // Check if RandR extension is available
    int randr_event_base, randr_error_base;
    if (!XRRQueryExtension(_sapp.x11.display, &randr_event_base, &randr_error_base)) {
        // Fallback to single display (default screen)
        sapp_display* display = &_sat.displays[0];
        display->width_px = DisplayWidth(_sapp.x11.display, _sapp.x11.screen);
        display->height_px = DisplayHeight(_sapp.x11.display, _sapp.x11.screen);
        display->width_mm = DisplayWidthMM(_sapp.x11.display, _sapp.x11.screen);
        display->height_mm = DisplayHeightMM(_sapp.x11.display, _sapp.x11.screen);
        display->refresh_rate = 60; // Default fallback
        display->dpi_scale = _sapp.dpi_scale;
        display->pos_x = 0;
        display->pos_y = 0;
        display->is_primary = true;
        display->name = "Display";
        _sat.display_count = 1;
        return;
    }

    // Use RandR extension for detailed display information
    XRRScreenResources* screen_resources = XRRGetScreenResources(_sapp.x11.display, _sapp.x11.root);
    if (!screen_resources) return;

    for (int i = 0; i < screen_resources->noutput && _sat.display_count < SAPP_MAX_DISPLAYS; i++) {
        XRROutputInfo* output_info = XRRGetOutputInfo(_sapp.x11.display, screen_resources, screen_resources->outputs[i]);
        if (!output_info || output_info->connection != RR_Connected || output_info->crtc == None) {
            XRRFreeOutputInfo(output_info);
            continue;
        }

        XRRCrtcInfo* crtc_info = XRRGetCrtcInfo(_sapp.x11.display, screen_resources, output_info->crtc);
        if (!crtc_info) {
            XRRFreeOutputInfo(output_info);
            continue;
        }

        sapp_display* display = &_sat.displays[_sat.display_count];

        // Resolution and position
        display->width_px = crtc_info->width;
        display->height_px = crtc_info->height;
        display->pos_x = crtc_info->x;
        display->pos_y = crtc_info->y;

        // Physical size
        display->width_mm = output_info->mm_width;
        display->height_mm = output_info->mm_height;

        // Refresh rate
        display->refresh_rate = 60; // Default
        if (crtc_info->mode != None) {
            for (int j = 0; j < screen_resources->nmode; j++) {
                if (screen_resources->modes[j].id == crtc_info->mode) {
                    XRRModeInfo* mode = &screen_resources->modes[j];
                    if (mode->hTotal && mode->vTotal) {
                        display->refresh_rate = (int)round((double)mode->dotClock / (mode->hTotal * mode->vTotal));
                    }
                    break;
                }
            }
        }

        // Content scale
        display->dpi_scale = _sapp.dpi_scale;

        // Display name (convert to static string)
        static char display_names[4][128];
        if (output_info->name) {
            strncpy(display_names[_sat.display_count], output_info->name, 127);
            display_names[_sat.display_count][127] = '\0';
            display->name = display_names[_sat.display_count];
        } else {
            snprintf(display_names[_sat.display_count], 128, "Display %d", _sat.display_count);
            display->name = display_names[_sat.display_count];
        }

        // Primary display (first one is considered primary)
        display->is_primary = (_sat.display_count == 0);

        XRRFreeCrtcInfo(crtc_info);
        XRRFreeOutputInfo(output_info);
        _sat.display_count++;
    }

    XRRFreeScreenResources(screen_resources);

    // Fallback if no displays found
    if (_sat.display_count == 0) {
        sapp_display* display = &_sat.displays[0];
        display->width_px = DisplayWidth(_sapp.x11.display, _sapp.x11.screen);
        display->height_px = DisplayHeight(_sapp.x11.display, _sapp.x11.screen);
        display->width_mm = DisplayWidthMM(_sapp.x11.display, _sapp.x11.screen);
        display->height_mm = DisplayHeightMM(_sapp.x11.display, _sapp.x11.screen);
        display->refresh_rate = 60;
        display->dpi_scale = _sapp.dpi_scale;
        display->pos_x = 0;
        display->pos_y = 0;
        display->is_primary = true;
        display->name = "Display";
        _sat.display_count = 1;
    }
}

_SOKOL_PRIVATE void sapp_linux_shutdown_displays(void) {
    _sat.display_count = 0;
}

_SOKOL_PRIVATE void sapp_linux_update_displays(void) {
    if (!_sapp.valid || _sat.display_count == 0) return;

    // Get window position
    Window root_return;
    int x, y;
    unsigned int width, height, border_width, depth;

    if (XGetGeometry(_sapp.x11.display, _sapp.x11.window, &root_return, &x, &y, &width, &height, &border_width, &depth)) {
        // Translate to root coordinates
        Window child_return;
        XTranslateCoordinates(_sapp.x11.display, _sapp.x11.window, _sapp.x11.root, 0, 0, &x, &y, &child_return);

        // Find which display contains the center of the window
        int center_x = x + width / 2;
        int center_y = y + height / 2;

        for (int i = 0; i < _sat.display_count; i++) {
            if (center_x >= _sat.displays[i].pos_x &&
                center_x < _sat.displays[i].pos_x + _sat.displays[i].width_px &&
                center_y >= _sat.displays[i].pos_y &&
                center_y < _sat.displays[i].pos_y + _sat.displays[i].height_px) {
                _sat.window_display_index = i;
                return;
            }
        }
    }

    // Default to first display if not found
    _sat.window_display_index = 0;
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

    // checks we are properly using poll_events, begin_tick, and end_tick: once per loop iteration
    _sat.poll_count += 1;
    // printf("poll: %lu / begin: %lu\n", _sat.poll_count, _sat.begin_count);
    assert(_sat.poll_count == _sat.begin_count + 1);
    assert(_sat.poll_count == _sat.end_count + 1);
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

    _sat.begin_count += 1;
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

    _sat.end_count += 1;
    SOKOL_ASSERT(_sat.begin_count == _sat.end_count);
}

SOKOL_API_IMPL const sapp_display* sapp_display_get_primary(void) {
    for (int i = 0; i < _sat.display_count; i++) {
        if (_sat.displays[i].is_primary) {
            return &_sat.displays[i];
        }
    }
    return _sat.display_count > 0 ? &_sat.displays[0] : NULL;
}

SOKOL_API_IMPL const sapp_display* sapp_display_get_window_display(void) {
    SOKOL_ASSERT(_sat.window_display_index >= 0 && _sat.window_display_index < SAPP_MAX_DISPLAYS);
    return &_sat.displays[_sat.window_display_index];
}

SOKOL_API_IMPL int sapp_display_get_count(void) {
    return _sat.display_count;
}

SOKOL_API_IMPL const sapp_display* sapp_display_get_at_index(int index) {
    SOKOL_ASSERT(index >= 0 && index < SAPP_MAX_DISPLAYS);
    return &_sat.displays[index];
}

#endif /* SOKOL_APP_TURBO_IMPL */

