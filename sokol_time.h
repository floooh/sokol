#pragma once
/*
    sokol_time.h    -- simple cross-platform time measurement

    Do this:
        #define SOKOL_IMPL
    before you include this file in *one* C or C++ file to create the 
    implementation.

    Optionally provide the following defines with your own implementations:
    SOKOL_ASSERT(c)     - your own assert macro (default: assert(c))

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
    emscripten:     clock_gettime(CLOCK_MONOTONIC)
    Linux+others:   clock_gettime(CLOCK_MONITONIC)
    
    
    MIT License

    Copyright (c) 2017 Andre Weissflog

    Permission is hereby granted, free of charge, to any person obtaining a copy
    of this software and associated documentation files (the "Software"), to deal
    in the Software without restriction, including without limitation the rights
    to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
    copies of the Software, and to permit persons to whom the Software is
    furnished to do so, subject to the following conditions:

    The above copyright notice and this permission notice shall be included in all
    copies or substantial portions of the Software.

    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
    IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
    FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
    AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
    LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
    OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
    SOFTWARE.    
*/
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

extern void stm_setup();
extern uint64_t stm_now();
extern uint64_t stm_diff(uint64_t new_ticks, uint64_t old_ticks);
extern uint64_t stm_since(uint64_t start_ticks);
extern uint64_t stm_laptime(uint64_t* last_time);
extern double stm_sec(uint64_t ticks);
extern double stm_ms(uint64_t ticks);
extern double stm_us(uint64_t ticks);
extern double stm_ns(uint64_t ticks);

/*-- IMPLEMENTATION ----------------------------------------------------------*/
#ifdef SOKOL_IMPL
#ifndef SOKOL_ASSERT
    #include <assert.h>
    #define SOKOL_ASSERT(c) assert(c)
#endif

static int _stm_initialized;
#if defined(_WIN32)
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
static LARGE_INTEGER _stm_win_freq;
static LARGE_INTEGER _stm_win_start;
#elif defined(__APPLE__) && defined(__MACH__)
#include <mach/mach_time.h>
static mach_timebase_info_data_t _stm_osx_timebase;
static uint64_t _stm_osx_start;
#else /* anything else, this will need more care for non-Linux platforms */
#include <time.h>
static uint64_t _stm_posix_start;
#endif

void stm_setup() {
    SOKOL_ASSERT(0 == _stm_initialized);
    _stm_initialized = 1;
    #if defined(_WIN32)
        QueryPerformanceFrequency(&_stm_win_freq);
        QueryPerformanceCounter(&_stm_win_start);
    #elif defined(__APPLE__) && defined(__MACH__)
        mach_timebase_info(&_stm_osx_timebase);
        _stm_osx_start = (mach_absolute_time()*_stm_osx_timebase.numer)/_stm_osx_timebase.denom;
    #else
        struct timespec ts;
        clock_gettime(CLOCK_MONOTONIC, &ts);
        _stm_posix_start = (uint64_t)ts.tv_sec*1000000000 + (uint64_t)ts.tv_nsec; 
    #endif
}

uint64_t stm_now() {
    SOKOL_ASSERT(_stm_initialized);
    uint64_t now;
    #if defined(_WIN32)
        LARGE_INTEGER qpc_t;
        QueryPerformanceCounter(&qpc_t);
        now = ((qpc_t.QuadPart - _stm_win_start.QuadPart) * 1000000000) / _stm_win_freq.QuadPart;
    #elif defined(__APPLE__) && defined(__MACH__)
        now = ((mach_absolute_time()*_stm_osx_timebase.numer)/_stm_osx_timebase.denom) - _stm_osx_start;
    #else
        struct timespec ts;
        clock_gettime(CLOCK_MONOTONIC, &ts);
        now = ((uint64_t)ts.tv_sec*1000000000 + (uint64_t)ts.tv_nsec) - _stm_posix_start;
    #endif
    return now;
}

uint64_t stm_diff(uint64_t new_ticks, uint64_t old_ticks) {
    if (new_ticks > old_ticks) {
        return new_ticks - old_ticks;
    }
    else {
        /* FIXME: this should be a value that converts to a non-null double */
        return 1;
    }
}

uint64_t stm_since(uint64_t start_ticks) {
    return stm_diff(stm_now(), start_ticks);
}

uint64_t stm_laptime(uint64_t* last_time) {
    SOKOL_ASSERT(last_time);
    uint64_t dt = 0;
    uint64_t now = stm_now();
    if (0 != *last_time) {
        dt = stm_diff(now, *last_time);
    }
    *last_time = now;
    return dt;
}

double stm_sec(uint64_t ticks) {
    return (double)ticks / 1000000000.0;
}

double stm_ms(uint64_t ticks) {
    return (double)ticks / 1000000.0;
}

double stm_us(uint64_t ticks) {
    return (double)ticks / 1000.0;
}

double stm_ns(uint64_t ticks) {
    return (double)ticks;
}
#endif /* SOKOL_IMPL */

#ifdef __cplusplus
} /* extern "C" */
#endif
