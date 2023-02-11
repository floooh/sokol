#ifndef SOKOL_APP_INCLUDED
#error Please include sokol_app.h before sokol_poll.h
#endif

#if defined(SOKOL_IMPL) && !defined(SOKOL_POLL_IMPL)
#define SOKOL_POLL_IMPL
#endif
#ifndef SOKOL_POLL_INCLUDED
/*
    sokol_poll.h -- polling layer for sokol_app input

    Project URL: https://github.com/floooh/sokol

    Do this:
        #define SOKOL_IMPL or
        #define SOKOL_POLL_IMPL
    before you include this file in *one* C or C++ file to create the
    implementation.

    OVERVIEW
    ========
    This header provides a supplementary poll-based input API for sokol_app.h

    When a program that uses sokol_app.h needs to check whether a key
    is currently down, it must manage a boolean flag that gets set or
    reset upon KEYUP/KEYDOWN events. This header provides convenience
    functions that manage such flags, along with a simple API to check
    them.

    PROVIDED FUNCTIONS
    ==================

    bool spoll_handle_event(const sapp_event* evt);
    bool spoll_keydown(sapp_keycode key);

    LICENSE
    =======
    zlib/libpng license

    Copyright (c) 2023 Octave Crespel

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
#define SOKOL_POLL_INCLUDED

#if defined(SOKOL_API_DECL) && !defined(SOKOL_POLL_API_DECL)
#define SOKOL_POLL_API_DECL SOKOL_API_DECL
#endif
#ifndef SOKOL_POLL_API_DECL
#if defined(_WIN32) && defined(SOKOL_DLL) && defined(SOKOL_POLL_IMPL)
#define SOKOL_POLL_API_DECL __declspec(dllexport)
#elif defined(_WIN32) && defined(SOKOL_DLL)
#define SOKOL_POLL_API_DECL __declspec(dllimport)
#else
#define SOKOL_POLL_API_DECL extern
#endif
#endif

#ifdef __cplusplus
extern "C" {
#endif

/*
    spoll_handle_event : must be called in the event callback that was
    provided at sokol_app initialization. Returns `true` if the event
    was used to update the internal keyboard state.
*/
bool spoll_handle_event(const sapp_event* evt);

/*
    spoll_keydown : returns whether the key `key` is currently being held down.
*/
bool spoll_keydown(sapp_keycode key);

#ifdef __cplusplus
} /* extern "C" */
#endif
#endif /* SOKOL_POLL_INCLUDED */

/*-- IMPLEMENTATION ----------------------------------------------------------*/
#ifdef SOKOL_POLL_IMPL
#define SOKOL_POLL_IMPL_INCLUDED

#include <stdint.h>

#define SOKOL_KEYCODE_MAXIMUM 512

static uint8_t g_key_down[SOKOL_KEYCODE_MAXIMUM / 8];

#define BIT_INDEX(key) ((key) / 8)
#define BIT_MASK(key) (1 << ((key) % 8))

bool spoll_handle_event(const sapp_event* evt)
{
    switch (evt->type) {
    case SAPP_EVENTTYPE_KEY_DOWN:
        g_key_down[BIT_INDEX(evt->key_code)] |= BIT_MASK(evt->key_code);
        return true;
    case SAPP_EVENTTYPE_KEY_UP:
        g_key_down[BIT_INDEX(evt->key_code)] &= ~BIT_MASK(evt->key_code);
        return true;
    case SAPP_EVENTTYPE_UNFOCUSED:
    case SAPP_EVENTTYPE_SUSPENDED:
    case SAPP_EVENTTYPE_ICONIFIED:
        memset(g_key_down, 0, sizeof(g_key_down));
        return true;
    default:
        return false;
    }
}

bool spoll_keydown(sapp_keycode key)
{
    return g_key_down[BIT_INDEX(key)] & BIT_MASK(key);
}

#endif
