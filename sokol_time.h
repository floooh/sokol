#pragma once
/*
    sokol_time.h    -- simple cross-platform time measurement

    Do this:
        #define SOKOL_IMPL
    before you include this file in *one* C or C++ file to create the
    implementation.

    Optionally provide the following defines with your own implementations:
    SOKOL_ASSERT(c)     - your own assert macro (default: assert(c))
    SOKOL_API_DECL      - public function declaration prefix (default: extern)
    SOKOL_API_IMPL      - public function implementation prefix (default: -)

    void stm_setup();
        Call once before any other functions to initialize sokol_time
        (this calls for instance QueryPerformanceFrequency on Windows)

    uint64_t stm_now();
        Get current point in time in unspecified 'ticks'. The value that
        is returned has no relation to the 'wall-clock' time and is
        not in a specific time unit, it is only useful to compute
        time differences.

    uint64_t stm_diff(uint64_t new, uint64_t old);
        Computes the time difference between new and old. This will always
        return a positive, non-zero value.

    uint64_t stm_since(uint64_t start);
        Takes the current time, and returns the elapsed time since start
        (this is a shortcut for "stm_diff(stm_now(), start)")

    uint64_t stm_laptime(uint64_t* last_time);
        This is useful for measuring frame time and other recurring
        events. It takes the current time, returns the time difference
        to the value in last_time, and stores the current time in
        last_time for the next call. If the value in last_time is 0,
        the return value will be zero (this usually happens on the
        very first call).

    Use the following functions to convert a duration in ticks into
    useful time units:

    double stm_sec(uint64_t ticks);
    double stm_ms(uint64_t ticks);
    double stm_us(uint64_t ticks);
    double stm_ns(uint64_t ticks);
        Converts a tick value into seconds, milliseconds, microseconds
        or nanoseconds. Note that not all platforms will have nanosecond
        or even microsecond precision.

    Uses the following time measurement functions under the hood:

    Windows:        QueryPerformanceFrequency() / QueryPerformanceCounter()
    MacOS/iOS:      mach_absolute_time()
    emscripten:     performance.now()
    Linux+others:   clock_gettime(CLOCK_MONOTONIC)

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
#define SOKOL_TIME_INCLUDED (1)
#include <stdint.h>

#ifndef SOKOL_API_DECL
    #define SOKOL_API_DECL extern
#endif

#ifdef __cplusplus
extern "C" {
#endif

SOKOL_API_DECL void stm_setup(void);
SOKOL_API_DECL uint64_t stm_now(void);
SOKOL_API_DECL uint64_t stm_diff(uint64_t new_ticks, uint64_t old_ticks);
SOKOL_API_DECL uint64_t stm_since(uint64_t start_ticks);
SOKOL_API_DECL uint64_t stm_laptime(uint64_t* last_time);
SOKOL_API_DECL double stm_sec(uint64_t ticks);
SOKOL_API_DECL double stm_ms(uint64_t ticks);
SOKOL_API_DECL double stm_us(uint64_t ticks);
SOKOL_API_DECL double stm_ns(uint64_t ticks);

#ifdef __cplusplus
} /* extern "C" */
#endif

/*-- IMPLEMENTATION ----------------------------------------------------------*/
#ifdef SOKOL_IMPL
#define SOKOL_TIME_IMPL_INCLUDED (1)
#include <string.h> /* memset */

#ifndef SOKOL_API_IMPL
    #define SOKOL_API_IMPL
#endif
#ifndef SOKOL_ASSERT
    #include <assert.h>
    #define SOKOL_ASSERT(c) assert(c)
#endif
#ifndef _SOKOL_PRIVATE
    #if defined(__GNUC__)
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
} _stm_state_t;
#elif defined(__APPLE__) && defined(__MACH__)
#include <mach/mach_time.h>
typedef struct {
    uint32_t initialized;
    mach_timebase_info_data_t timebase;
    uint64_t start;
} _stm_state_t;
#elif defined(__EMSCRIPTEN__)
#include <emscripten/emscripten.h>
typedef struct {
    uint32_t initialized;
    double start;
} _stm_state_t;
#else /* anything else, this will need more care for non-Linux platforms */
#include <time.h>
typedef struct {
    uint32_t initialized;
    uint64_t start;
} _stm_state_t;
#endif
static _stm_state_t _stm;

/* prevent 64-bit overflow when computing relative timestamp
    see https://gist.github.com/jspohr/3dc4f00033d79ec5bdaf67bc46c813e3
*/
#if defined(_WIN32) || (defined(__APPLE__) && defined(__MACH__))
_SOKOL_PRIVATE int64_t int64_muldiv(int64_t value, int64_t numer, int64_t denom) {
    int64_t q = value / denom;
    int64_t r = value % denom;
    return q * numer + r * numer / denom;
}
#endif

#if defined(__EMSCRIPTEN__)
EM_JS(double, _stm_js_perfnow, (void), {
    return performance.now();
});
#endif

SOKOL_API_IMPL void stm_setup(void) {
    memset(&_stm, 0, sizeof(_stm));
    _stm.initialized = 0xABCDABCD;
    #if defined(_WIN32)
        QueryPerformanceFrequency(&_stm.freq);
        QueryPerformanceCounter(&_stm.start);
    #elif defined(__APPLE__) && defined(__MACH__)
        mach_timebase_info(&_stm.timebase);
        _stm.start = mach_absolute_time();
    #elif defined(__EMSCRIPTEN__)
        _stm.start = _stm_js_perfnow();
    #else
        struct timespec ts;
        clock_gettime(CLOCK_MONOTONIC, &ts);
        _stm.start = (uint64_t)ts.tv_sec*1000000000 + (uint64_t)ts.tv_nsec;
    #endif
}

SOKOL_API_IMPL uint64_t stm_now(void) {
    SOKOL_ASSERT(_stm.initialized == 0xABCDABCD);
    uint64_t now;
    #if defined(_WIN32)
        LARGE_INTEGER qpc_t;
        QueryPerformanceCounter(&qpc_t);
        now = int64_muldiv(qpc_t.QuadPart - _stm.start.QuadPart, 1000000000, _stm.freq.QuadPart);
    #elif defined(__APPLE__) && defined(__MACH__)
        const uint64_t mach_now = mach_absolute_time() - _stm.start;
        now = int64_muldiv(mach_now, _stm.timebase.numer, _stm.timebase.denom);
    #elif defined(__EMSCRIPTEN__)
        double js_now = _stm_js_perfnow() - _stm.start;
        SOKOL_ASSERT(js_now >= 0.0);
        now = (uint64_t) (js_now * 1000000.0);
    #else
        struct timespec ts;
        clock_gettime(CLOCK_MONOTONIC, &ts);
        now = ((uint64_t)ts.tv_sec*1000000000 + (uint64_t)ts.tv_nsec) - _stm.start;
    #endif
    return now;
}

SOKOL_API_IMPL uint64_t stm_diff(uint64_t new_ticks, uint64_t old_ticks) {
    if (new_ticks > old_ticks) {
        return new_ticks - old_ticks;
    }
    else {
        return 1;
    }
}

SOKOL_API_IMPL uint64_t stm_since(uint64_t start_ticks) {
    return stm_diff(stm_now(), start_ticks);
}

SOKOL_API_IMPL uint64_t stm_laptime(uint64_t* last_time) {
    SOKOL_ASSERT(last_time);
    uint64_t dt = 0;
    uint64_t now = stm_now();
    if (0 != *last_time) {
        dt = stm_diff(now, *last_time);
    }
    *last_time = now;
    return dt;
}

SOKOL_API_IMPL double stm_sec(uint64_t ticks) {
    return (double)ticks / 1000000000.0;
}

SOKOL_API_IMPL double stm_ms(uint64_t ticks) {
    return (double)ticks / 1000000.0;
}

SOKOL_API_IMPL double stm_us(uint64_t ticks) {
    return (double)ticks / 1000.0;
}

SOKOL_API_IMPL double stm_ns(uint64_t ticks) {
    return (double)ticks;
}
#endif /* SOKOL_IMPL */

