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
    Dear ImGui C++ API. When compiled as C, sokol_gfx_imgui.h will call
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
    [FIXME]

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
SOKOL_APP_IMGUI_API_DECL void sappimgui_draw_timing_window_content(void);
SOKOL_APP_IMGUI_API_DECL void sappimgui_draw_framebuffer_window_content(void);
SOKOL_APP_IMGUI_API_DECL void sappimgui_draw_event_window_content(void);

SOKOL_APP_IMGUI_API_DECL void sappimgui_draw_hud_window(const char* title);
SOKOL_APP_IMGUI_API_DECL void sappimgui_draw_timing_window(const char* title);
SOKOL_APP_IMGUI_API_DECL void sappimgui_draw_framebuffer_window(const char* title);
SOKOL_APP_IMGUI_API_DECL void sappimgui_draw_event_window(const char* title);

SOKOL_APP_IMGUI_API_DECL void sappimgui_draw_hud_menu_item(const char* label);
SOKOL_APP_IMGUI_API_DECL void sappimgui_draw_timing_menu_item(const char* label);
SOKOL_APP_IMGUI_API_DECL void sappimgui_draw_framebuffer_menu_item(const char* label);
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
#ifndef _SOKOL_UNUSED
#define _SOKOL_UNUSED(x) (void)(x)
#endif
#ifndef SOKOL_API_IMPL
#define SOKOL_API_IMPL
#endif

#include <stddef.h> // size_t
#include <stdio.h> // snprintf
#include <string.h> // memset, strncmp

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

_SOKOL_PRIVATE bool _sappimgui_ring_empty(_sappimgui_ring_t* ring) {
    return ring->head == ring->tail;
}

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
} _sappimgui_timing_window_t;

typedef struct {
    bool open;
} _sappimgui_framebuffer_window_t;

typedef struct {
    bool open;
} _sappimgui_event_window_t;

typedef struct {
    uint32_t init_tag;
    struct {
        _sappimgui_ring_t ring;
        _sappimgui_frame_t items[_SAPPIMGUI_RING_NUM_SLOTS];
    } frames;
    struct {
        _sappimgui_ring_t ring;
        sapp_event items[_SAPPIMGUI_RING_NUM_SLOTS];
    } events;
    _sappimgui_hud_window_t hud_window;
    _sappimgui_timing_window_t timing_window;
    _sappimgui_framebuffer_window_t framebuffer_window;
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

_SOKOL_PRIVATE void _sappimgui_add_event(const sapp_event* ev) {
    SOKOL_ASSERT(ev);
    _sappimgui.events.items[_sappimgui_ring_add(&_sappimgui.events.ring)] = *ev;
}

_SOKOL_PRIVATE int _sappimgui_num_events(void) {
    return _sappimgui_ring_count(&_sappimgui.events.ring);
}

_SOKOL_PRIVATE const sapp_event* _sappimgui_event_at(int idx) {
    SOKOL_ASSERT((idx >= 0) && (idx < _sappimgui_ring_count(&_sappimgui.events.ring)));
    const int ev_idx = _sappimgui_ring_idx(_sappimgui.events.ring.tail + idx);
    SOKOL_ASSERT((ev_idx >= 0) && (ev_idx < _SAPPIMGUI_RING_NUM_SLOTS));
    return &_sappimgui.events.items[ev_idx];
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
    _SGIMGUI_IMGUI_FUNC(SameLine)();
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

_SOKOL_PRIVATE void _sappimgui_popstylevar(int count) {
    _SAPPIMGUI_IMGUI_FUNC(PopStyleVar)(count);
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
    _SAPPIMGUI_IMGUI_FUNC(PlotLines)(label, values_getter, data, values_count, values_offset, overlay_text, scale_min, scale_max, graph_size);
}


//--- public functions ---------------------------------------------------------
#define _SAPPIMGUI_INIT_TAG (0xABCDABCD)

SOKOL_API_IMPL void sappimgui_setup(void) {
    _sappimgui_clear(&_sappimgui, sizeof(_sappimgui));
    _sappimgui.init_tag = _SAPPIMGUI_INIT_TAG;
    _sappimgui_ring_init(&_sappimgui.frames.ring);
    _sappimgui_ring_init(&_sappimgui.events.ring);
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
    _sappimgui_add_event(ev);
}

SOKOL_API_IMPL void sappimgui_draw(void) {
    SOKOL_ASSERT(_SAPPIMGUI_INIT_TAG == _sappimgui.init_tag);
    sappimgui_draw_hud_window("Hud");
    sappimgui_draw_timing_window("Timing");
    sappimgui_draw_framebuffer_window("Framebuffer");
    sappimgui_draw_event_window("Events");
}

SOKOL_API_IMPL void sappimgui_draw_menu(const char* title) {
    SOKOL_ASSERT(_SAPPIMGUI_INIT_TAG == _sappimgui.init_tag);
    SOKOL_ASSERT(title);
    if (_sappimgui_igbeginmenu(title)) {
        sappimgui_draw_hud_menu_item("Hud");
        sappimgui_draw_timing_menu_item("Timing");
        sappimgui_draw_framebuffer_menu_item("Framebuffer");
        sappimgui_draw_event_menu_item("Events");
        _sappimgui_igendmenu();
    }
}

SOKOL_API_IMPL void sappimgui_draw_hud_menu_item(const char* label) {
    SOKOL_ASSERT(_SAPPIMGUI_INIT_TAG == _sappimgui.init_tag);
    SOKOL_ASSERT(label);
    _sappimgui_igmenuitemboolptr(label, 0, &_sappimgui.hud_window.open, true);
}

SOKOL_API_IMPL void sappimgui_draw_timing_menu_item(const char* label) {
    SOKOL_ASSERT(_SAPPIMGUI_INIT_TAG == _sappimgui.init_tag);
    SOKOL_ASSERT(label);
    _sappimgui_igmenuitemboolptr(label, 0, &_sappimgui.timing_window.open, true);
}

SOKOL_API_IMPL void sappimgui_draw_framebuffer_menu_item(const char* label) {
    SOKOL_ASSERT(_SAPPIMGUI_INIT_TAG == _sappimgui.init_tag);
    SOKOL_ASSERT(label);
    _sappimgui_igmenuitemboolptr(label, 0, &_sappimgui.framebuffer_window.open, true);
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
    _sappimgui_popstylevar(1);
    _sappimgui_igend();
}

SOKOL_API_IMPL void sappimgui_draw_timing_window(const char* title) {
    SOKOL_ASSERT(_SAPPIMGUI_INIT_TAG == _sappimgui.init_tag);
    SOKOL_ASSERT(title);
    if (!_sappimgui.timing_window.open) {
        return;
    }
    _sappimgui_igsetnextwindowsize(IMVEC2(128, 128), ImGuiCond_Once);
    if (_sappimgui_igbegin(title, &_sappimgui.timing_window.open, 0)) {
        sappimgui_draw_timing_window_content();
    }
    _sappimgui_igend();
}

SOKOL_API_IMPL void sappimgui_draw_framebuffer_window(const char* title) {
    SOKOL_ASSERT(_SAPPIMGUI_INIT_TAG == _sappimgui.init_tag);
    SOKOL_ASSERT(title);
    if (!_sappimgui.framebuffer_window.open) {
        return;
    }
    _sappimgui_igsetnextwindowsize(IMVEC2(128, 128), ImGuiCond_Once);
    if (_sappimgui_igbegin(title, &_sappimgui.framebuffer_window.open, 0)) {
        sappimgui_draw_framebuffer_window_content();
    }
    _sappimgui_igend();
}

SOKOL_API_IMPL void sappimgui_draw_event_window(const char* title) {
    SOKOL_ASSERT(_SAPPIMGUI_INIT_TAG == _sappimgui.init_tag);
    SOKOL_ASSERT(title);
    if (!_sappimgui.event_window.open) {
        return;
    }
    _sappimgui_igsetnextwindowsize(IMVEC2(128, 128), ImGuiCond_Once);
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
    _sappimgui_igplotlines("##filtered", _sappimgui_filtered_dt_getter, 0, _SAPPIMGUI_RING_NUM_SLOTS - 1, 0, "filtered frame dt (ms)", scale_min, scale_max, IMVEC2(256,48));
    _sappimgui_igsameline();
    _sappimgui_igtext("min: %.3fms\nmax: %.3fms", stats.filtered.minimum * 1000.0, stats.filtered.maximum * 1000.0);
    _sappimgui_igplotlines("##raw", _sappimgui_raw_dt_getter, 0, _SAPPIMGUI_RING_NUM_SLOTS - 1, 0, "raw frame dt (ms)", scale_min, scale_max, IMVEC2(256,48));
    _sappimgui_igsameline();
    _sappimgui_igtext("min: %.3fms\nmax: %.3fms", stats.raw.minimum * 1000.0, stats.raw.maximum * 1000.0);
}

SOKOL_API_IMPL void sappimgui_draw_timing_window_content(void) {
    SOKOL_ASSERT(_SAPPIMGUI_INIT_TAG == _sappimgui.init_tag);
    _sappimgui_igtext("FIXME");
}

SOKOL_API_IMPL void sappimgui_draw_framebuffer_window_content(void) {
    SOKOL_ASSERT(_SAPPIMGUI_INIT_TAG == _sappimgui.init_tag);
    _sappimgui_igtext("FIXME");
}

SOKOL_API_IMPL void sappimgui_draw_event_window_content(void) {
    SOKOL_ASSERT(_SAPPIMGUI_INIT_TAG == _sappimgui.init_tag);
    _sappimgui_igtext("FIXME");
}

#endif // SOKOL_APP_IMGUI_IMPL
