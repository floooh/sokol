#if defined(SOKOL_IMPL) && !defined(SOKOL_APP_IMGUI_IMPL)
#define SOKOL_APP_IMGUI_IMPL
#endif
#ifndef SOKOL_APP_IMGUI_INCLUDED
/*
    sokol_app_imgui.h - debug-inspection UI for sokol_app.h using Dear ImGui

    Project URL: https://github.com/floooh/sokol

    Do this:
        #define SOKOL_IMPL or
        #define SOKOL_APP_IMGUI_IMPL

    before you include this file in *one* C or C++ file to create the
    implementation.

    NOTE that the implementation can be compiled either as C++ or as C.
    When compiled as C++, sokol_app_imgui.h will directly call into the
    Dear ImGui C++ API. When compiled as C, sokol_app_imgui.h will call
    cimgui.h functions instead.

    Include the following file(s) before including sokol_app_imgui.h:

        sokol_app.h

    Additionally, include the following headers before including the
    implementation:

    If the implementation is compiled as C++:
        imgui.h

    If the implementation is compiled as C:
        cimgui.h

    Before including the sokol_app_imgui.h implementation, optionally
    override the following macros:

        SOKOL_ASSERT(c)     -- your own assert macro, default: assert(c)
        SOKOL_UNREACHABLE   -- your own macro to annotate unreachable code,
                               default: SOKOL_ASSERT(false)
        SOKOL_APP_IMGUI_API_DECL    - public function declaration prefix (default: extern)
        SOKOL_APP_IMGUI_CPREFIX     - defines the function prefix for the Dear ImGui C bindings (default: ig)
        SOKOL_API_DECL      - same as SOKOL_GFX_IMGUI_API_DECL
        SOKOL_API_IMPL      - public function implementation prefix (default: -)

    If sokol_app_imgui.h is compiled as a DLL, define the following before
    including the declaration or implementation:

    SOKOL_DLL

    On Windows, SOKOL_DLL will define SOKOL_APP_IMGUI_API_DECL as __declspec(dllexport)
    or __declspec(dllimport) as needed.

    STEP BY STEP
    ============
    - call sappimgui_setup() before any other sappimgui functions:

        sappimgui_setup();

    - in your sokol-app event event handler callback, this records
      the events for the event-viewer window:

        sappimgui_track_event(ev);

    - somewhere at the start of your sokol-app frame callback, this
      records the frame duration for the debug hud:

        sappimgui_track_frame();

    - inside Dear ImGui's BeginMainMenuBar/EndMainMenuBar

        sappimgui_draw_menu("sokol-app");

    - and somewhere in your Dear ImGui top-level rendering code:

        sappimgui_draw();

    ALTERNATIVE DRAWING FUNCTIONS:
    ==============================
    Instead of the convenient but all-in-one sappimgui_draw() function,
    you can also use the following granular functions which might allow
    better integration with your existing UI:

    The following functions only render the window *content* (so you
    can integrate the UI into you own windows):

        void sappimgui_draw_hud_window_content(void);
        void sappimgui_draw_publicstate_window_content(void);
        void sappimgui_draw_event_window_content(void);

    And these are the 'full window' drawing functions:

        void sappimgui_draw_hud_window(const char* title);
        void sappimgui_draw_publicstate_window(const char* title);
        void sappimgui_draw_event_window(const char* title);

    To draw the individual menu items:

        void sappimgui_draw_hud_menu_item(const char* label);
        void sappimgui_draw_publicstate_menu_item(const char* label);
        void sappimgui_draw_event_menu_item(const char* label);

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
#define SOKOL_APP_IMGUI_INCLUDED (1)
#include <stdint.h>
#include <stdbool.h>

#if !defined(SOKOL_APP_INCLUDED)
#error "Please include sokol_app.h before sokol_app_imgui.h"
#endif

#if defined(SOKOL_API_DECL) && !defined(SOKOL_APP_IMGUI_API_DECL)
#define SOKOL_APP_IMGUI_API_DECL SOKOL_API_DECL
#endif
#ifndef SOKOL_APP_IMGUI_API_DECL
#if defined(_WIN32) && defined(SOKOL_DLL) && defined(SOKOL_APP_IMGUI_IMPL)
#define SOKOL_APP_IMGUI_API_DECL __declspec(dllexport)
#elif defined(_WIN32) && defined(SOKOL_DLL)
#define SOKOL_APP_IMGUI_API_DECL __declspec(dllimport)
#else
#define SOKOL_APP_IMGUI_API_DECL extern
#endif
#endif

#if defined(__cplusplus)
extern "C" {
#endif

SOKOL_APP_IMGUI_API_DECL void sappimgui_setup(void);
SOKOL_APP_IMGUI_API_DECL void sappimgui_shutdown(void);
SOKOL_APP_IMGUI_API_DECL void sappimgui_track_frame(void);
SOKOL_APP_IMGUI_API_DECL void sappimgui_track_event(const sapp_event* ev);
SOKOL_APP_IMGUI_API_DECL void sappimgui_draw(void);
SOKOL_APP_IMGUI_API_DECL void sappimgui_draw_menu(const char* title);

SOKOL_APP_IMGUI_API_DECL void sappimgui_draw_hud_window_content(void);
SOKOL_APP_IMGUI_API_DECL void sappimgui_draw_publicstate_window_content(void);
SOKOL_APP_IMGUI_API_DECL void sappimgui_draw_event_window_content(void);

SOKOL_APP_IMGUI_API_DECL void sappimgui_draw_hud_window(const char* title);
SOKOL_APP_IMGUI_API_DECL void sappimgui_draw_publicstate_window(const char* title);
SOKOL_APP_IMGUI_API_DECL void sappimgui_draw_event_window(const char* title);

SOKOL_APP_IMGUI_API_DECL void sappimgui_draw_hud_menu_item(const char* label);
SOKOL_APP_IMGUI_API_DECL void sappimgui_draw_publicstate_menu_item(const char* label);
SOKOL_APP_IMGUI_API_DECL void sappimgui_draw_event_menu_item(const char* label);

#if defined(__cplusplus)
} // extern "C"
#endif
#endif // SOKOL_APP_IMGUI_INCLUDED

/*=== IMPLEMENTATION =========================================================*/
#ifdef SOKOL_APP_IMGUI_IMPL
#define SOKOL_APP_IMGUI_IMPL_INCLUDED (1)

#if defined(__cplusplus)
    #if !defined(IMGUI_VERSION)
    #error "Please include imgui.h before the sokol_app_imgui.h implementation"
    #endif
#else
    #if !defined(CIMGUI_API)
    #error "Please include cimgui.h before the sokol_app_imgui.h implementation"
    #endif
#endif
#ifndef SOKOL_ASSERT
    #include <assert.h>
    #define SOKOL_ASSERT(c) assert(c)
#endif
#ifndef SOKOL_UNREACHABLE
    #define SOKOL_UNREACHABLE SOKOL_ASSERT(false)
#endif
#ifndef _SOKOL_PRIVATE
    #if defined(__GNUC__) || defined(__clang__)
        #define _SOKOL_PRIVATE __attribute__((unused)) static
    #else
        #define _SOKOL_PRIVATE static
    #endif
#endif
#ifndef SOKOL_DEBUG
    #ifndef NDEBUG
        #define SOKOL_DEBUG
    #endif
#endif
#ifndef _SOKOL_UNUSED
#define _SOKOL_UNUSED(x) (void)(x)
#endif
#ifndef SOKOL_API_IMPL
#define SOKOL_API_IMPL
#endif

#include <stddef.h> // size_t
#include <stdio.h> // snprintf
#include <string.h> // memset, strncmp
#include <math.h> // round()
#include <stdarg.h> // va_list

#ifdef __cplusplus
#define _SAPP_STRUCT(TYPE, NAME) TYPE NAME = {}
#else
#define _SAPP_STRUCT(TYPE, NAME) TYPE NAME = {0}
#endif

//-- ring buffer helper --------------------------------------------------------
#define _SAPPIMGUI_RING_NUM_SLOTS (256)
typedef struct {
    int head;
    int tail;
} _sappimgui_ring_t;

_SOKOL_PRIVATE int _sappimgui_ring_idx(int i) {
    return i % _SAPPIMGUI_RING_NUM_SLOTS;
}

_SOKOL_PRIVATE void _sappimgui_ring_init(_sappimgui_ring_t* ring) {
    ring->head = 0;
    ring->tail = 0;
}

_SOKOL_PRIVATE bool _sappimgui_ring_full(_sappimgui_ring_t* ring) {
    return _sappimgui_ring_idx(ring->head + 1) == ring->tail;
}

#if defined(SOKOL_DEBUG)
_SOKOL_PRIVATE bool _sappimgui_ring_empty(_sappimgui_ring_t* ring) {
    return ring->head == ring->tail;
}
#endif

_SOKOL_PRIVATE int _sappimgui_ring_count(_sappimgui_ring_t* ring) {
    int count;
    if (ring->head >= ring->tail) {
        count = ring->head - ring->tail;
    } else {
        count = (ring->head + _SAPPIMGUI_RING_NUM_SLOTS) - ring->tail;
    }
    SOKOL_ASSERT((count >= 0) && (count < _SAPPIMGUI_RING_NUM_SLOTS));
    return count;
}

_SOKOL_PRIVATE int _sappimgui_ring_rem(_sappimgui_ring_t* ring) {
    SOKOL_ASSERT(!_sappimgui_ring_empty(ring));
    const int idx = ring->tail;
    ring->tail = _sappimgui_ring_idx(ring->tail + 1);
    return idx;
}

_SOKOL_PRIVATE int _sappimgui_ring_add(_sappimgui_ring_t* ring) {
    if (_sappimgui_ring_full(ring)) {
        _sappimgui_ring_rem(ring);
    }
    const int idx = ring->head;
    ring->head = _sappimgui_ring_idx(idx + 1);
    return idx;
}

//-- internal state ------------------------------------------------------------
typedef struct {
    uint64_t frame_count;
    double raw_dt;
    double filtered_dt;
} _sappimgui_frame_t;

typedef struct {
    bool open;
} _sappimgui_hud_window_t;

typedef struct {
    bool open;
} _sappimgui_publicstate_window_t;

typedef struct {
    bool open;
    sapp_event events[_SAPP_EVENTTYPE_NUM];
} _sappimgui_event_window_t;

typedef struct {
    uint32_t init_tag;
    struct {
        _sappimgui_ring_t ring;
        _sappimgui_frame_t items[_SAPPIMGUI_RING_NUM_SLOTS];
    } frames;
    _sappimgui_hud_window_t hud_window;
    _sappimgui_publicstate_window_t publicstate_window;
    _sappimgui_event_window_t event_window;
} _sappimgui_state_t;
static _sappimgui_state_t _sappimgui;

//--- utils --------------------------------------------------------------------
_SOKOL_PRIVATE void _sappimgui_clear(void* ptr, size_t size) {
    SOKOL_ASSERT(ptr && (size > 0));
    memset(ptr, 0, size);
}

_SOKOL_PRIVATE void _sappimgui_add_frame(const _sappimgui_frame_t* f) {
    SOKOL_ASSERT(f);
    _sappimgui.frames.items[_sappimgui_ring_add(&_sappimgui.frames.ring)] = *f;
}

_SOKOL_PRIVATE int _sappimgui_num_frames(void) {
    return _sappimgui_ring_count(&_sappimgui.frames.ring);
}

_SOKOL_PRIVATE const _sappimgui_frame_t* _sappimgui_frame_at(int idx) {
    SOKOL_ASSERT((idx >= 0) && (idx < _sappimgui_ring_count(&_sappimgui.frames.ring)));
    const int frame_idx = _sappimgui_ring_idx(_sappimgui.frames.ring.tail + idx);
    SOKOL_ASSERT((frame_idx >= 0) && (frame_idx < _SAPPIMGUI_RING_NUM_SLOTS));
    return &_sappimgui.frames.items[frame_idx];
}

typedef struct {
    int num;
    struct {
        double average;
        double minimum;
        double maximum;
    } filtered;
    struct {
        double average;
        double minimum;
        double maximum;
    } raw;
} _sappimgui_frame_stats_t;

_SOKOL_PRIVATE _sappimgui_frame_stats_t _sappimgui_frame_stats(void) {
    _sappimgui_frame_stats_t res;
    _sappimgui_clear(&res, sizeof(res));
    res.num = _sappimgui_num_frames();
    if (res.num > 0) {
        for (int i = 0; i < res.num; i++) {
            const _sappimgui_frame_t* f = &_sappimgui.frames.items[_sappimgui_ring_idx(_sappimgui.frames.ring.tail + i)];
            const double filtered_dt = f->filtered_dt;
            const double raw_dt = f->raw_dt;
            res.filtered.average += filtered_dt;
            if ((res.filtered.minimum == 0.0) || (res.filtered.minimum > filtered_dt)) {
                res.filtered.minimum = filtered_dt;
            }
            if (res.filtered.maximum < filtered_dt) {
                res.filtered.maximum = filtered_dt;
            }
            res.raw.average += raw_dt;
            if ((res.raw.minimum == 0.0) || (res.raw.minimum > raw_dt)) {
                res.raw.minimum = raw_dt;
            }
            if (res.raw.maximum < raw_dt) {
                res.raw.maximum = raw_dt;
            }
        }
        res.filtered.average /= (double)res.num;
        res.raw.average /= (double)res.num;
    }
    return res;
}

_SOKOL_PRIVATE float _sappimgui_filtered_dt_getter(void* data, int idx) {
    _SOKOL_UNUSED(data);
    if (idx < _sappimgui_num_frames()) {
        return (float)(_sappimgui_frame_at(idx)->filtered_dt * 1000.0);
    } else {
        return 0.0f;
    }
}

_SOKOL_PRIVATE float _sappimgui_raw_dt_getter(void* data, int idx) {
    _SOKOL_UNUSED(data);
    if (idx < _sappimgui_num_frames()) {
        return (float)(_sappimgui_frame_at(idx)->raw_dt * 1000.0);
    } else {
        return 0.0f;
    }
}

_SOKOL_PRIVATE const char* _sappimgui_bool_string(bool b) {
    return b ? "true" : "false";
}

_SOKOL_PRIVATE const char* _sappimgui_pixelformat_string(sapp_pixel_format fmt) {
    switch (fmt) {
        case SAPP_PIXELFORMAT_NONE: return "NONE";
        case SAPP_PIXELFORMAT_RGBA8: return "RGBA8";
        case SAPP_PIXELFORMAT_SRGB8A8: return "SRGB8A8";
        case SAPP_PIXELFORMAT_BGRA8: return "BGRA8";
        case SAPP_PIXELFORMAT_SBGRA8: return "SBGRA8";
        case SAPP_PIXELFORMAT_DEPTH: return "DEPTH";
        case SAPP_PIXELFORMAT_DEPTH_STENCIL: return "DEPTH_STENCIL";
        default: return "???";
    }
}

_SOKOL_PRIVATE const char* _sappimgui_mousecursor_string(sapp_mouse_cursor c) {
    switch (c) {
        case SAPP_MOUSECURSOR_ARROW: return "ARROW";
        case SAPP_MOUSECURSOR_IBEAM: return "IBEAM";
        case SAPP_MOUSECURSOR_CROSSHAIR: return "CROSSHAIR";
        case SAPP_MOUSECURSOR_POINTING_HAND: return "POINTING_HAND";
        case SAPP_MOUSECURSOR_RESIZE_EW: return "RESIZE_EW";
        case SAPP_MOUSECURSOR_RESIZE_NS: return "RESIZE_NS";
        case SAPP_MOUSECURSOR_RESIZE_NWSE: return "RESIZE_NWSE";
        case SAPP_MOUSECURSOR_RESIZE_NESW: return "RESIZE_NESW";
        case SAPP_MOUSECURSOR_RESIZE_ALL: return "RESIZE_ALL";
        case SAPP_MOUSECURSOR_NOT_ALLOWED: return "NOT_ALLOWED";
        case SAPP_MOUSECURSOR_CUSTOM_0: return "CUSTOM_0";
        case SAPP_MOUSECURSOR_CUSTOM_1: return "CUSTOM_1";
        case SAPP_MOUSECURSOR_CUSTOM_2: return "CUSTOM_2";
        case SAPP_MOUSECURSOR_CUSTOM_3: return "CUSTOM_3";
        case SAPP_MOUSECURSOR_CUSTOM_4: return "CUSTOM_4";
        case SAPP_MOUSECURSOR_CUSTOM_5: return "CUSTOM_5";
        case SAPP_MOUSECURSOR_CUSTOM_6: return "CUSTOM_6";
        case SAPP_MOUSECURSOR_CUSTOM_7: return "CUSTOM_7";
        case SAPP_MOUSECURSOR_CUSTOM_8: return "CUSTOM_8";
        case SAPP_MOUSECURSOR_CUSTOM_9: return "CUSTOM_9";
        case SAPP_MOUSECURSOR_CUSTOM_10: return "CUSTOM_10";
        case SAPP_MOUSECURSOR_CUSTOM_11: return "CUSTOM_11";
        case SAPP_MOUSECURSOR_CUSTOM_12: return "CUSTOM_12";
        case SAPP_MOUSECURSOR_CUSTOM_13: return "CUSTOM_13";
        case SAPP_MOUSECURSOR_CUSTOM_14: return "CUSTOM_14";
        case SAPP_MOUSECURSOR_CUSTOM_15: return "CUSTOM_15";
        default: return "???";
    }
}

_SOKOL_PRIVATE const char* _sappimgui_eventtype_string(sapp_event_type ev_type) {
    switch (ev_type) {
        case SAPP_EVENTTYPE_INVALID: return "INVALID";
        case SAPP_EVENTTYPE_KEY_DOWN: return "KEY_DOWN";
        case SAPP_EVENTTYPE_KEY_UP: return "KEY_UP";
        case SAPP_EVENTTYPE_CHAR: return "CHAR";
        case SAPP_EVENTTYPE_MOUSE_DOWN: return "MOUSE_DOWN";
        case SAPP_EVENTTYPE_MOUSE_UP: return "MOUSE_UP";
        case SAPP_EVENTTYPE_MOUSE_SCROLL: return "MOUSE_SCROLL";
        case SAPP_EVENTTYPE_MOUSE_MOVE: return "MOUSE_MOVE";
        case SAPP_EVENTTYPE_MOUSE_ENTER: return "MOUSE_ENTER";
        case SAPP_EVENTTYPE_MOUSE_LEAVE: return "MOUSE_LEAVE";
        case SAPP_EVENTTYPE_TOUCHES_BEGAN: return "TOUCHES_BEGAN";
        case SAPP_EVENTTYPE_TOUCHES_MOVED: return "TOUCHES_MOVED";
        case SAPP_EVENTTYPE_TOUCHES_ENDED: return "TOUCHES_ENDED";
        case SAPP_EVENTTYPE_TOUCHES_CANCELLED: return "TOUCHES_CANCELLED";
        case SAPP_EVENTTYPE_RESIZED: return "RESIZED";
        case SAPP_EVENTTYPE_ICONIFIED: return "ICONIFIED";
        case SAPP_EVENTTYPE_RESTORED: return "RESTORED";
        case SAPP_EVENTTYPE_FOCUSED: return "FOCUSED";
        case SAPP_EVENTTYPE_UNFOCUSED: return "UNFOCUSED";
        case SAPP_EVENTTYPE_SUSPENDED: return "SUSPENDED";
        case SAPP_EVENTTYPE_RESUMED: return "RESUMED";
        case SAPP_EVENTTYPE_QUIT_REQUESTED: return "QUIT_REQUESTED";
        case SAPP_EVENTTYPE_CLIPBOARD_PASTED: return "CLIPBOARD_PASTED";
        case SAPP_EVENTTYPE_FILES_DROPPED: return "FILES_DROPPED";
        default: return "???";
    }
}

_SOKOL_PRIVATE const char* _sappimgui_keycode_string(sapp_keycode k) {
    switch (k) {
        case SAPP_KEYCODE_INVALID:      return "INVALID";
        case SAPP_KEYCODE_SPACE:        return "SPACE";
        case SAPP_KEYCODE_APOSTROPHE:   return "APOSTROPHE";
        case SAPP_KEYCODE_COMMA:        return "COMMA";
        case SAPP_KEYCODE_MINUS:        return "MINUS";
        case SAPP_KEYCODE_PERIOD:       return "PERIOD";
        case SAPP_KEYCODE_SLASH:        return "SLASH";
        case SAPP_KEYCODE_0:            return "0";
        case SAPP_KEYCODE_1:            return "1";
        case SAPP_KEYCODE_2:            return "2";
        case SAPP_KEYCODE_3:            return "3";
        case SAPP_KEYCODE_4:            return "4";
        case SAPP_KEYCODE_5:            return "5";
        case SAPP_KEYCODE_6:            return "6";
        case SAPP_KEYCODE_7:            return "7";
        case SAPP_KEYCODE_8:            return "8";
        case SAPP_KEYCODE_9:            return "9";
        case SAPP_KEYCODE_SEMICOLON:    return "SEMICOLON";
        case SAPP_KEYCODE_EQUAL:        return "EQUAL";
        case SAPP_KEYCODE_A:            return "A";
        case SAPP_KEYCODE_B:            return "B";
        case SAPP_KEYCODE_C:            return "C";
        case SAPP_KEYCODE_D:            return "D";
        case SAPP_KEYCODE_E:            return "E";
        case SAPP_KEYCODE_F:            return "F";
        case SAPP_KEYCODE_G:            return "G";
        case SAPP_KEYCODE_H:            return "H";
        case SAPP_KEYCODE_I:            return "I";
        case SAPP_KEYCODE_J:            return "J";
        case SAPP_KEYCODE_K:            return "K";
        case SAPP_KEYCODE_L:            return "L";
        case SAPP_KEYCODE_M:            return "M";
        case SAPP_KEYCODE_N:            return "N";
        case SAPP_KEYCODE_O:            return "O";
        case SAPP_KEYCODE_P:            return "P";
        case SAPP_KEYCODE_Q:            return "Q";
        case SAPP_KEYCODE_R:            return "R";
        case SAPP_KEYCODE_S:            return "S";
        case SAPP_KEYCODE_T:            return "T";
        case SAPP_KEYCODE_U:            return "U";
        case SAPP_KEYCODE_V:            return "V";
        case SAPP_KEYCODE_W:            return "W";
        case SAPP_KEYCODE_X:            return "X";
        case SAPP_KEYCODE_Y:            return "Y";
        case SAPP_KEYCODE_Z:            return "Z";
        case SAPP_KEYCODE_LEFT_BRACKET: return "LEFT_BRACKET";
        case SAPP_KEYCODE_BACKSLASH:    return "BACKSLASH";
        case SAPP_KEYCODE_RIGHT_BRACKET:    return "RIGHT_BRACKET";
        case SAPP_KEYCODE_GRAVE_ACCENT: return "ACCENT";
        case SAPP_KEYCODE_WORLD_1:      return "WORLD_1";
        case SAPP_KEYCODE_WORLD_2:      return "WORLD_2";
        case SAPP_KEYCODE_ESCAPE:       return "ESCAPE";
        case SAPP_KEYCODE_ENTER:        return "ENTER";
        case SAPP_KEYCODE_TAB:          return "TAB";
        case SAPP_KEYCODE_BACKSPACE:    return "BACKSPACE";
        case SAPP_KEYCODE_INSERT:       return "INSERT";
        case SAPP_KEYCODE_DELETE:       return "DELETE";
        case SAPP_KEYCODE_RIGHT:        return "RIGHT";
        case SAPP_KEYCODE_LEFT:         return "LEFT";
        case SAPP_KEYCODE_DOWN:         return "DOWN";
        case SAPP_KEYCODE_UP:           return "UP";
        case SAPP_KEYCODE_PAGE_UP:      return "PAGE_UP";
        case SAPP_KEYCODE_PAGE_DOWN:    return "PAGE_DOWN";
        case SAPP_KEYCODE_HOME:         return "HOME";
        case SAPP_KEYCODE_END:          return "END";
        case SAPP_KEYCODE_CAPS_LOCK:    return "CAPS_LOCK";
        case SAPP_KEYCODE_SCROLL_LOCK:  return "SCROLL_LOCK";
        case SAPP_KEYCODE_NUM_LOCK:     return "NUM_LOCK";
        case SAPP_KEYCODE_PRINT_SCREEN: return "PRINT_SCREEN";
        case SAPP_KEYCODE_PAUSE:        return "PAUSE";
        case SAPP_KEYCODE_F1:           return "F1";
        case SAPP_KEYCODE_F2:           return "F2";
        case SAPP_KEYCODE_F3:           return "F3";
        case SAPP_KEYCODE_F4:           return "F4";
        case SAPP_KEYCODE_F5:           return "F5";
        case SAPP_KEYCODE_F6:           return "F6";
        case SAPP_KEYCODE_F7:           return "F7";
        case SAPP_KEYCODE_F8:           return "F8";
        case SAPP_KEYCODE_F9:           return "F9";
        case SAPP_KEYCODE_F10:          return "F10";
        case SAPP_KEYCODE_F11:          return "F11";
        case SAPP_KEYCODE_F12:          return "F12";
        case SAPP_KEYCODE_F13:          return "F13";
        case SAPP_KEYCODE_F14:          return "F14";
        case SAPP_KEYCODE_F15:          return "F15";
        case SAPP_KEYCODE_F16:          return "F16";
        case SAPP_KEYCODE_F17:          return "F17";
        case SAPP_KEYCODE_F18:          return "F18";
        case SAPP_KEYCODE_F19:          return "F19";
        case SAPP_KEYCODE_F20:          return "F20";
        case SAPP_KEYCODE_F21:          return "F21";
        case SAPP_KEYCODE_F22:          return "F22";
        case SAPP_KEYCODE_F23:          return "F23";
        case SAPP_KEYCODE_F24:          return "F24";
        case SAPP_KEYCODE_F25:          return "F25";
        case SAPP_KEYCODE_KP_0:         return "KP_0";
        case SAPP_KEYCODE_KP_1:         return "KP_1";
        case SAPP_KEYCODE_KP_2:         return "KP_2";
        case SAPP_KEYCODE_KP_3:         return "KP_3";
        case SAPP_KEYCODE_KP_4:         return "KP_4";
        case SAPP_KEYCODE_KP_5:         return "KP_5";
        case SAPP_KEYCODE_KP_6:         return "KP_6";
        case SAPP_KEYCODE_KP_7:         return "KP_7";
        case SAPP_KEYCODE_KP_8:         return "KP_8";
        case SAPP_KEYCODE_KP_9:         return "KP_9";
        case SAPP_KEYCODE_KP_DECIMAL:   return "KP_DECIMAL";
        case SAPP_KEYCODE_KP_DIVIDE:    return "KP_DIVIDE";
        case SAPP_KEYCODE_KP_MULTIPLY:  return "KP_MULTIPLY";
        case SAPP_KEYCODE_KP_SUBTRACT:  return "KP_SUBTRACT";
        case SAPP_KEYCODE_KP_ADD:       return "KP_ADD";
        case SAPP_KEYCODE_KP_ENTER:     return "KP_ENTER";
        case SAPP_KEYCODE_KP_EQUAL:     return "KP_EQUAL";
        case SAPP_KEYCODE_LEFT_SHIFT:   return "LEFT_SHIFT";
        case SAPP_KEYCODE_LEFT_CONTROL: return "LEFT_CONTROL";
        case SAPP_KEYCODE_LEFT_ALT:     return "LEFT_ALT";
        case SAPP_KEYCODE_LEFT_SUPER:   return "LEFT_SUPER";
        case SAPP_KEYCODE_RIGHT_SHIFT:  return "RIGHT_SHIFT";
        case SAPP_KEYCODE_RIGHT_CONTROL:    return "RIGHT_CONTROL";
        case SAPP_KEYCODE_RIGHT_ALT:    return "RIGHT_ALT";
        case SAPP_KEYCODE_RIGHT_SUPER:  return "RIGHT_SUPER";
        case SAPP_KEYCODE_MENU:         return "MENU";
        default:                        return "???";
    }
}

_SOKOL_PRIVATE const char* _sappimgui_mousebutton_string(sapp_mousebutton btn) {
    switch (btn) {
        case SAPP_MOUSEBUTTON_INVALID:  return "INVALID";
        case SAPP_MOUSEBUTTON_LEFT:     return "LEFT";
        case SAPP_MOUSEBUTTON_RIGHT:    return "RIGHT";
        case SAPP_MOUSEBUTTON_MIDDLE:   return "MIDDLE";
        default:                        return "???";
    }
}

_SOKOL_PRIVATE const char* _sappimgui_androidtooltype_string(sapp_android_tooltype t) {
    switch (t) {
        case SAPP_ANDROIDTOOLTYPE_UNKNOWN: return "UNKNOWN";
        case SAPP_ANDROIDTOOLTYPE_FINGER: return "FINGER";
        case SAPP_ANDROIDTOOLTYPE_STYLUS: return "STYLUS";
        case SAPP_ANDROIDTOOLTYPE_MOUSE: return "MOUSE";
        default: return "???";
    }
}

//--- C => C++ layer -----------------------------------------------------------

#if defined(__cplusplus)
#define _SAPPIMGUI_IMGUI_FUNC(name) ImGui::name
#else
#ifndef SOKOL_APP_IMGUI_CPREFIX
#define SOKOL_APP_IMGUI_CPREFIX ig
#endif
#define _SAPPIMGUI_CONCAT2(prefix, name) prefix ## name
#define _SAPPIMGUI_CONCAT(prefix, name) _SAPPIMGUI_CONCAT2(prefix, name)
#define _SAPPIMGUI_IMGUI_FUNC(name) _SAPPIMGUI_CONCAT(SOKOL_APP_IMGUI_CPREFIX, name)
#endif

#if defined(__cplusplus)
#define IMVEC2(x,y) ImVec2(x,y)
#define IMVEC4(x,y,z,w) ImVec4(x,y,z,w)
#else
#define IMVEC2(x,y) (ImVec2){x,y}
#define IMVEC4(x,y,z,w) (ImVec4){x,y,z,w}
#endif

_SOKOL_PRIVATE void _sappimgui_igsameline(void) {
    _SAPPIMGUI_IMGUI_FUNC(SameLine)();
}

_SOKOL_PRIVATE void _sappimgui_igtext(const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);
    _SAPPIMGUI_IMGUI_FUNC(TextV)(fmt, args);
    va_end(args);
}

_SOKOL_PRIVATE void _sappimgui_pushstylevar(ImGuiStyleVar v, float val) {
    _SAPPIMGUI_IMGUI_FUNC(PushStyleVar)(v, val);
}

_SOKOL_PRIVATE void _sappimgui_popstylevar(void) {
    _SAPPIMGUI_IMGUI_FUNC(PopStyleVar)();
}

_SOKOL_PRIVATE ImGuiViewport* _sappimgui_iggetmainviewport(void) {
    return _SAPPIMGUI_IMGUI_FUNC(GetMainViewport)();
}

_SOKOL_PRIVATE void _sappimgui_igsetnextwindowpos(const ImVec2 pos, ImGuiCond cond) {
    _SAPPIMGUI_IMGUI_FUNC(SetNextWindowPos)(pos, cond);
}

_SOKOL_PRIVATE void _sappimgui_igsetnextwindowsize(const ImVec2 size, ImGuiCond cond) {
    _SAPPIMGUI_IMGUI_FUNC(SetNextWindowSize)(size, cond);
}

_SOKOL_PRIVATE void _sappimgui_igsetnextwindowbgalpha(float a) {
    _SAPPIMGUI_IMGUI_FUNC(SetNextWindowBgAlpha)(a);
}

_SOKOL_PRIVATE bool _sappimgui_igbegin(const char* name, bool* p_open, ImGuiWindowFlags flags) {
    return _SAPPIMGUI_IMGUI_FUNC(Begin)(name, p_open, flags);
}

_SOKOL_PRIVATE void _sappimgui_igend(void) {
    _SAPPIMGUI_IMGUI_FUNC(End)();
}

_SOKOL_PRIVATE bool _sappimgui_igbeginmenu(const char* label) {
    return _SAPPIMGUI_IMGUI_FUNC(BeginMenu)(label);
}

_SOKOL_PRIVATE void _sappimgui_igendmenu(void) {
    _SAPPIMGUI_IMGUI_FUNC(EndMenu)();
}

_SOKOL_PRIVATE bool _sappimgui_igmenuitemboolptr(const char* label, const char* shortcut, bool* p_selected, bool enabled) {
    #if defined(__cplusplus)
        return ImGui::MenuItem(label, shortcut, p_selected, enabled);
    #else
        return _SAPPIMGUI_IMGUI_FUNC(MenuItemBoolPtr)(label, shortcut, p_selected, enabled);
    #endif
}

_SOKOL_PRIVATE void _sappimgui_igplotlines(const char* label, float(*values_getter)(void* data, int idx), void* data, int values_count, int values_offset, const char* overlay_text, float scale_min, float scale_max, ImVec2 graph_size) {
    #if defined(__cplusplus)
        ImGui::PlotLines(label, values_getter, data, values_count, values_offset, overlay_text, scale_min, scale_max, graph_size);
    #else
        _SAPPIMGUI_IMGUI_FUNC(PlotLinesCallbackEx)(label, values_getter, data, values_count, values_offset, overlay_text, scale_min, scale_max, graph_size);
    #endif
}

_SOKOL_PRIVATE bool _sappimgui_igcollapsingheader(const char* label, ImGuiTreeNodeFlags flags) {
    return _SAPPIMGUI_IMGUI_FUNC(CollapsingHeader)(label, flags);
}

_SOKOL_PRIVATE ImVec4 _sappimgui_getstylecolorvec4(ImGuiCol c) {
    #if defined(__cplusplus)
        return ImGui::GetStyleColorVec4(c);
    #else
        return *_SAPPIMGUI_IMGUI_FUNC(GetStyleColorVec4)(c);
    #endif
}

_SOKOL_PRIVATE void _sappimgui_pushstylecolor(ImGuiCol idx, ImVec4 c) {
    #if defined(__cplusplus)
        ImGui::PushStyleColor(idx, c);
    #else
        _SAPPIMGUI_IMGUI_FUNC(PushStyleColorImVec4)(idx, c);
    #endif
}

_SOKOL_PRIVATE void _sappimgui_popstylecolor(void) {
    _SAPPIMGUI_IMGUI_FUNC(PopStyleColor)();
}

//--- internal ui functions -----------------------------------------------------
_SOKOL_PRIVATE void _sappimgui_draw_modifiers(uint32_t modifiers) {
    _sappimgui_igsameline();
    if (0 == modifiers) {
        _sappimgui_igtext("NONE");
    } else {
        if (0 != (modifiers & SAPP_MODIFIER_SHIFT)) {
            _sappimgui_igsameline(); _sappimgui_igtext("SHIFT");
        }
        if (0 != (modifiers & SAPP_MODIFIER_CTRL)) {
            _sappimgui_igsameline(); _sappimgui_igtext("CTRL");
        }
        if (0 != (modifiers & SAPP_MODIFIER_ALT)) {
            _sappimgui_igsameline(); _sappimgui_igtext("ALT");
        }
        if (0 != (modifiers & SAPP_MODIFIER_SUPER)) {
            _sappimgui_igsameline(); _sappimgui_igtext("SUPER");
        }
        if (0 != (modifiers & SAPP_MODIFIER_LMB)) {
            _sappimgui_igsameline(); _sappimgui_igtext("LMB");
        }
        if (0 != (modifiers & SAPP_MODIFIER_RMB)) {
            _sappimgui_igsameline(); _sappimgui_igtext("RMB");
        }
        if (0 != (modifiers & SAPP_MODIFIER_MMB)) {
            _sappimgui_igsameline(); _sappimgui_igtext("MMB");
        }
    }
}

_SOKOL_PRIVATE void _sappimgui_draw_event(sapp_event_type ev_type, uint64_t cur_frame_count) {
    SOKOL_ASSERT(((int)ev_type >= 0) && ((int)ev_type < _SAPP_EVENTTYPE_NUM));
    const sapp_event* ev = &_sappimgui.event_window.events[ev_type];
    const char* ev_name = _sappimgui_eventtype_string(ev_type);
    char imgui_id[32];
    snprintf(imgui_id, sizeof(imgui_id), "###id_%d", (int)ev_type);
    char title[128];
    if (ev->frame_count == 0) {
        snprintf(title, sizeof(title), "%s [none]%s", ev_name, imgui_id);
    } else switch (ev_type) {
        // NOTE: '###' is the special Dear ImGui marker to define the item id without the visible part
        case SAPP_EVENTTYPE_KEY_DOWN:
        case SAPP_EVENTTYPE_KEY_UP:
            snprintf(title, sizeof(title), "%s %s%s", ev_name, _sappimgui_keycode_string(ev->key_code), imgui_id);
            break;
        case SAPP_EVENTTYPE_CHAR:
            snprintf(title, sizeof(title), "%s 0x%05X%s", ev_name, ev->char_code, imgui_id);
            break;
        case SAPP_EVENTTYPE_MOUSE_DOWN:
        case SAPP_EVENTTYPE_MOUSE_UP:
            snprintf(title, sizeof(title), "%s %s [%.2f, %.2f]%s", ev_name, _sappimgui_mousebutton_string(ev->mouse_button), ev->mouse_x, ev->mouse_y, imgui_id);
            break;
        case SAPP_EVENTTYPE_MOUSE_SCROLL:
            snprintf(title, sizeof(title), "%s [%.2f, %.2f]%s", ev_name, ev->scroll_x, ev->scroll_y, imgui_id);
            break;
        case SAPP_EVENTTYPE_MOUSE_MOVE:
            snprintf(title, sizeof(title), "%s [%.2f, %.2f] [%.2f, %.2f]%s", ev_name, ev->mouse_x, ev->mouse_y, ev->mouse_dx, ev->mouse_dy, imgui_id);
            break;
        case SAPP_EVENTTYPE_RESIZED:
            snprintf(title, sizeof(title), "%s [%d, %d]%s", ev_name, ev->framebuffer_width, ev->framebuffer_height, imgui_id);
            break;
        case SAPP_EVENTTYPE_MOUSE_ENTER:
        case SAPP_EVENTTYPE_MOUSE_LEAVE:
        case SAPP_EVENTTYPE_ICONIFIED:
        case SAPP_EVENTTYPE_RESTORED:
        case SAPP_EVENTTYPE_FOCUSED:
        case SAPP_EVENTTYPE_UNFOCUSED:
        case SAPP_EVENTTYPE_SUSPENDED:
        case SAPP_EVENTTYPE_RESUMED:
        case SAPP_EVENTTYPE_QUIT_REQUESTED:
        case SAPP_EVENTTYPE_CLIPBOARD_PASTED:
        case SAPP_EVENTTYPE_FILES_DROPPED:
            snprintf(title, sizeof(title), "%s%s", ev_name, imgui_id);
            break;
        case SAPP_EVENTTYPE_TOUCHES_BEGAN:
        case SAPP_EVENTTYPE_TOUCHES_MOVED:
        case SAPP_EVENTTYPE_TOUCHES_ENDED:
        case SAPP_EVENTTYPE_TOUCHES_CANCELLED:
            snprintf(title, sizeof(title), "%s %d [%.2f, %.2f]%s", ev_name, ev->num_touches, ev->touches[0].pos_x, ev->touches[0].pos_y, imgui_id);
            break;
        default:
            snprintf(title, sizeof(title), "???%s", imgui_id);
            break;
    }
    const float frame_age = (float)(cur_frame_count - ev->frame_count);
    float flash_intensity = (40.0f - frame_age) / 40.0f;
    if (flash_intensity < 0.0f) { flash_intensity = 0.0f; }
    else if (flash_intensity > 1.0f) { flash_intensity = 1.0f; }
    ImVec4 c = _sappimgui_getstylecolorvec4(ImGuiCol_Header);
    c.x += flash_intensity;
    c.y -= flash_intensity;
    c.z -= flash_intensity;
    c.w += flash_intensity;
    _sappimgui_pushstylecolor(ImGuiCol_Header, c);
    if (_sappimgui_igcollapsingheader(title, 0)) {
        _sappimgui_igtext("frame: %d", ev->frame_count);
        _sappimgui_igtext("type: %s", ev_name);
        _sappimgui_igtext("key code: %s", _sappimgui_keycode_string(ev->key_code));
        _sappimgui_igtext("char code: 0x%05X", ev->char_code);
        _sappimgui_igtext("key repeat: %s", _sappimgui_bool_string(ev->key_repeat));
        _sappimgui_igtext("modifiers: "); _sappimgui_draw_modifiers(ev->modifiers);
        _sappimgui_igtext("mouse button: %s", _sappimgui_mousebutton_string(ev->mouse_button));
        _sappimgui_igtext("mouse x: %.2f", ev->mouse_x);
        _sappimgui_igtext("mouse y: %.2f", ev->mouse_y);
        _sappimgui_igtext("scroll x: %.2f", ev->scroll_x);
        _sappimgui_igtext("scroll y: %.2f", ev->scroll_y);
        _sappimgui_igtext("window width: %d", ev->window_width);
        _sappimgui_igtext("window height: %d", ev->window_height);
        _sappimgui_igtext("framebuffer width: %d", ev->framebuffer_width);
        _sappimgui_igtext("framebuffer height: %d", ev->framebuffer_height);
        _sappimgui_igtext("num touches: %d", ev->num_touches);
        for (int i = 0; i < ev->num_touches; i++) {
            _sappimgui_igtext("touch point %d", i);
            _sappimgui_igtext("  identifier: %x", ev->touches[i].identifier);
            _sappimgui_igtext("  pos x: %.2f", ev->touches[i].pos_x);
            _sappimgui_igtext("  pos y: %.2f", ev->touches[i].pos_y);
            _sappimgui_igtext("  changed: %s", _sappimgui_bool_string(ev->touches[i].changed));
            _sappimgui_igtext("  android tooltype: %s", _sappimgui_androidtooltype_string(ev->touches[i].android_tooltype));
        }
    }
    _sappimgui_popstylecolor();
}

//--- public functions ---------------------------------------------------------
#define _SAPPIMGUI_INIT_TAG (0xABCDABCD)

SOKOL_API_IMPL void sappimgui_setup(void) {
    _sappimgui_clear(&_sappimgui, sizeof(_sappimgui));
    _sappimgui.init_tag = _SAPPIMGUI_INIT_TAG;
    _sappimgui_ring_init(&_sappimgui.frames.ring);
}

SOKOL_API_IMPL void sappimgui_shutdown(void) {
    SOKOL_ASSERT(_SAPPIMGUI_INIT_TAG == _sappimgui.init_tag);
    _sappimgui.init_tag = 0;
}

SOKOL_API_IMPL void sappimgui_track_frame(void) {
    SOKOL_ASSERT(_SAPPIMGUI_INIT_TAG == _sappimgui.init_tag);
    _sappimgui_frame_t frame;
    _sappimgui_clear(&frame, sizeof(frame));
    frame.frame_count = sapp_frame_count();
    frame.filtered_dt = sapp_frame_duration();
    frame.raw_dt = sapp_frame_duration_unfiltered();
    _sappimgui_add_frame(&frame);
}

SOKOL_API_IMPL void sappimgui_track_event(const sapp_event* ev) {
    SOKOL_ASSERT(_SAPPIMGUI_INIT_TAG == _sappimgui.init_tag);
    SOKOL_ASSERT(ev);
    SOKOL_ASSERT(((int)ev->type >= 0) && ((int)ev->type < _SAPP_EVENTTYPE_NUM));
    _sappimgui.event_window.events[(int)ev->type] = *ev;
}

SOKOL_API_IMPL void sappimgui_draw(void) {
    SOKOL_ASSERT(_SAPPIMGUI_INIT_TAG == _sappimgui.init_tag);
    sappimgui_draw_hud_window("[sapp] Hud");
    sappimgui_draw_publicstate_window("[sapp] Public State");
    sappimgui_draw_event_window("[sapp] Events");
}

SOKOL_API_IMPL void sappimgui_draw_menu(const char* title) {
    SOKOL_ASSERT(_SAPPIMGUI_INIT_TAG == _sappimgui.init_tag);
    SOKOL_ASSERT(title);
    if (_sappimgui_igbeginmenu(title)) {
        sappimgui_draw_hud_menu_item("Hud");
        sappimgui_draw_publicstate_menu_item("Public State");
        sappimgui_draw_event_menu_item("Events");
        _sappimgui_igendmenu();
    }
}

SOKOL_API_IMPL void sappimgui_draw_hud_menu_item(const char* label) {
    SOKOL_ASSERT(_SAPPIMGUI_INIT_TAG == _sappimgui.init_tag);
    SOKOL_ASSERT(label);
    _sappimgui_igmenuitemboolptr(label, 0, &_sappimgui.hud_window.open, true);
}

SOKOL_API_IMPL void sappimgui_draw_publicstate_menu_item(const char* label) {
    SOKOL_ASSERT(_SAPPIMGUI_INIT_TAG == _sappimgui.init_tag);
    SOKOL_ASSERT(label);
    _sappimgui_igmenuitemboolptr(label, 0, &_sappimgui.publicstate_window.open, true);
}

SOKOL_API_IMPL void sappimgui_draw_event_menu_item(const char* label) {
    SOKOL_ASSERT(_SAPPIMGUI_INIT_TAG == _sappimgui.init_tag);
    SOKOL_ASSERT(label);
    _sappimgui_igmenuitemboolptr(label, 0, &_sappimgui.event_window.open, true);
}

SOKOL_API_IMPL void sappimgui_draw_hud_window(const char* title) {
    SOKOL_ASSERT(_SAPPIMGUI_INIT_TAG == _sappimgui.init_tag);
    SOKOL_ASSERT(title);
    if (!_sappimgui.hud_window.open) {
        return;
    }
    const ImVec2 vp_workpos = _sappimgui_iggetmainviewport()->WorkPos;
    _sappimgui_igsetnextwindowpos(IMVEC2(vp_workpos.x + 20, vp_workpos.y + 10), ImGuiCond_Once);
    _sappimgui_igsetnextwindowsize(IMVEC2(256, 30), ImGuiCond_Once);
    _sappimgui_igsetnextwindowbgalpha(0.5f);
    _sappimgui_pushstylevar(ImGuiStyleVar_WindowRounding, 8.0f);
    const ImGuiWindowFlags f = ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoDecoration;
    if (_sappimgui_igbegin(title, &_sappimgui.hud_window.open, f)) {
        sappimgui_draw_hud_window_content();
    }
    _sappimgui_popstylevar();
    _sappimgui_igend();
}

SOKOL_API_IMPL void sappimgui_draw_publicstate_window(const char* title) {
    SOKOL_ASSERT(_SAPPIMGUI_INIT_TAG == _sappimgui.init_tag);
    SOKOL_ASSERT(title);
    if (!_sappimgui.publicstate_window.open) {
        return;
    }
    //_sappimgui_igsetnextwindowsize(IMVEC2(128, 128), ImGuiCond_Once);
    if (_sappimgui_igbegin(title, &_sappimgui.publicstate_window.open, 0)) {
        sappimgui_draw_publicstate_window_content();
    }
    _sappimgui_igend();
}

SOKOL_API_IMPL void sappimgui_draw_event_window(const char* title) {
    SOKOL_ASSERT(_SAPPIMGUI_INIT_TAG == _sappimgui.init_tag);
    SOKOL_ASSERT(title);
    if (!_sappimgui.event_window.open) {
        return;
    }
    _sappimgui_igsetnextwindowsize(IMVEC2(360, 512), ImGuiCond_Once);
    if (_sappimgui_igbegin(title, &_sappimgui.event_window.open, 0)) {
        sappimgui_draw_event_window_content();
    }
    _sappimgui_igend();
}

SOKOL_API_IMPL void sappimgui_draw_hud_window_content(void) {
    SOKOL_ASSERT(_SAPPIMGUI_INIT_TAG == _sappimgui.init_tag);
    const _sappimgui_frame_stats_t stats = _sappimgui_frame_stats();
    float scale_min = (float)(stats.filtered.average * 1000.0) - 4.0f;
    float scale_max = (float)(stats.filtered.average * 1000.0) + 4.0f;
    double cur_dt = sapp_frame_duration();
    int fps = (int)round(1.0 / cur_dt);
    _sappimgui_igtext("fps: %d (%.3fms)", fps, cur_dt * 1000.0);
    _sappimgui_igplotlines("##filtered", _sappimgui_filtered_dt_getter, 0, _SAPPIMGUI_RING_NUM_SLOTS - 1, 0, "filtered frame dt (ms)", scale_min, scale_max, IMVEC2(256,48));
    _sappimgui_igsameline();
    _sappimgui_igtext("min: %6.3fms\nmax: %6.3fms", stats.filtered.minimum * 1000.0, stats.filtered.maximum * 1000.0);
    _sappimgui_igplotlines("##raw", _sappimgui_raw_dt_getter, 0, _SAPPIMGUI_RING_NUM_SLOTS - 1, 0, "raw frame dt (ms)", scale_min, scale_max, IMVEC2(256,48));
    _sappimgui_igsameline();
    _sappimgui_igtext("min: %6.3fms\nmax: %6.3fms", stats.raw.minimum * 1000.0, stats.raw.maximum * 1000.0);
}

SOKOL_API_IMPL void sappimgui_draw_publicstate_window_content(void) {
    SOKOL_ASSERT(_SAPPIMGUI_INIT_TAG == _sappimgui.init_tag);
    _sappimgui_igtext("width: %d", sapp_width());
    _sappimgui_igtext("height: %d", sapp_height());
    _sappimgui_igtext("color format: %s", _sappimgui_pixelformat_string(sapp_color_format()));
    _sappimgui_igtext("depth format: %s", _sappimgui_pixelformat_string(sapp_depth_format()));
    _sappimgui_igtext("sample count: %d", sapp_sample_count());
    _sappimgui_igtext("high dpi: %s", _sappimgui_bool_string(sapp_high_dpi()));
    _sappimgui_igtext("dpi scale: %f", sapp_dpi_scale());
    _sappimgui_igtext("frame count: %d", sapp_frame_count());
    _sappimgui_igtext("frame duration: %.6f", sapp_frame_duration());
    _sappimgui_igtext("frame duration unfiltered: %.6f", sapp_frame_duration_unfiltered());
    _sappimgui_igtext("is fullscreen: %s", _sappimgui_bool_string(sapp_is_fullscreen()));
    _sappimgui_igtext("mouse shown: %s", _sappimgui_bool_string(sapp_mouse_shown()));
    _sappimgui_igtext("mouse locked: %s", _sappimgui_bool_string(sapp_mouse_locked()));
    _sappimgui_igtext("mouse cursor: %s", _sappimgui_mousecursor_string(sapp_get_mouse_cursor()));
    _sappimgui_igtext("user data: %p", sapp_userdata());
    _sappimgui_igtext("clipboard string: %s", sapp_get_clipboard_string());
    _sappimgui_igtext("num dropped files: %d", sapp_get_num_dropped_files());
    for (int i = 0; i < sapp_get_num_dropped_files(); i++) {
        _sappimgui_igtext("dropped file path #%d: %s", sapp_get_dropped_file_path(i));
    }
}

SOKOL_API_IMPL void sappimgui_draw_event_window_content(void) {
    SOKOL_ASSERT(_SAPPIMGUI_INIT_TAG == _sappimgui.init_tag);
    const uint64_t cur_frame_count = sapp_frame_count();
    for (int i = 1; i < (int)_SAPP_EVENTTYPE_NUM; i++) {
        _sappimgui_draw_event((sapp_event_type)i, cur_frame_count);
    }
}

#endif // SOKOL_APP_IMGUI_IMPL
