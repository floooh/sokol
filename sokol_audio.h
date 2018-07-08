#pragma once
/*
    sokol_audio.h   -- minimalist cross-platform buffer-streaming audio API

    THIS IS HIGHLY EXPERIMENTAL AND WON'T BE FINISHED FOR A WHILE, 
    DON'T USE!

    In the beginning this is mainly a testbed to get the simplest possible
    glitch-free audio solution for WASM/asm.js, and to have that simplest-
    possible API for other platforms too.

    On non-emscripten platforms, essentially use the SoLoud backend
    code. If the minimal emscripten buffer-streaming works well,
    try to get a PR into SoLoud instead of the current 
    SDL-static backend (which has a fairly big JS shim).

    Two ways to provide audio data:

    - low-level: streaming callback, will be called when audio backend needs
      data, may be called from a separate thread
    - push: application enqueues new data into a queue-buffer from main thread,
      sokol_audio streams from this

    Both methods to provide audio data are mutually exclusive.

    The push model needs a way to keep track of how many samples have been
    processed, to prevent the audio playback from starving.

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

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    uint32_t sample_rate;
    uint32_t audio_buffer_size; /* smaller buffer size means less latency */
    uint32_t queue_buffer_size; /* only used if no stream_cb is provided */
    bool stereo;                /* default false */
    /* optional low-level callback to provide samples */
    void (*stream_cb)(float* buffer, uint32_t num_samples);
} saudio_desc;

extern void saudio_setup(const saudio_desc* desc);
extern void saudio_shutdown(void);
/* true between setup and shutdown */
extern bool saudio_isvalid(void);
/* actual sample rate */
extern int saudio_sample_rate(void);
/* actual backend buffer size */
extern int saudio_buffer_size(void);
/* did we actually get a stereo buffer? */
extern bool saudio_stereo(void);
/* push samples to queue buffer (main-thread alternative to stream_cb) */
extern void saudio_enqueue(const float* samples, uint32_t num_samples);
/* overall number of samples written to the audio backend */
extern uint64_t saudio_samples_written(void);
/* overall number of samples enqueued */
extern uint64_t saudio_samples_enqueued(void);
/* enqueued samples waiting to be written */
extern uint64_t saudio_samples_pending(void);

#ifdef __cplusplus
} /* extern "C" */
#endif
