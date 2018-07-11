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
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    int sample_rate;
    int audio_buffer_size;      /* smaller buffer size means less latency */
    int push_buffer_size;       /* only used if no stream_cb is provided */
    bool stereo;                /* default false */
    /* optional low-level callback to provide samples */
    void (*stream_cb)(float* buffer, int num_samples);
} saudio_desc;

/* setup sokol-audio */
extern void saudio_setup(const saudio_desc* desc);
/* shutdown sokol-audio */
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
extern void saudio_push(const float* samples, int num_samples);
/* overall number of samples written to the audio backend */
extern uint64_t saudio_samples_written(void);
/* overall number of samples enqueued */
extern uint64_t saudio_samples_pushed(void);
/* pushed samples still waiting to be written */
extern uint64_t saudio_samples_pending(void);

#ifdef __cplusplus
} /* extern "C" */
#endif

/*--- IMPLEMENTATION ---------------------------------------------------------*/
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
#ifndef SOKOL_MALLOC
    #include <stdlib.h>
    #define SOKOL_MALLOC(s) malloc(s)
    #define SOKOL_FREE(p) free(p)
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
extern "C" {
#endif

#define _saudio_def(val, def) (((val) == 0) ? (def) : (val))
#define _saudio_def_flt(val, def) (((val) == 0.0f) ? (def) : (val))

typedef struct {
    bool valid;
    int requested_sample_rate;
    int requested_audio_buffer_size;
    int push_buffer_size;
    int actual_sample_rate;
    int actual_audio_buffer_size;
    bool actual_stereo;
    saudio_desc desc;
} _saudio_state;
static _saudio_state _saudio;

/*=== COREAUDIO BACKEND ======================================================*/
#if defined(__APPLE__)
#include <AudioToolbox/AudioToolbox.h>

static AudioQueueRef _saudio_ca_audio_queue;

_SOKOL_PRIVATE void _sapp_ca_callback(void* user_data, AudioQueueRef queue, AudioQueueBufferRef buffer) {
    memset(buffer->mAudioData, 0, buffer->mAudioDataByteSize);
    AudioQueueEnqueueBuffer(queue, buffer, 0, NULL);
}

_SOKOL_PRIVATE void _saudio_init(void) {
    SOKOL_ASSERT(0 == _saudio_ca_audio_queue);

    _saudio.actual_sample_rate = _saudio.requested_sample_rate;
    _saudio.actual_audio_buffer_size = _saudio.requested_audio_buffer_size;
    _saudio.actual_stereo = true;

    AudioStreamBasicDescription fmt;
    memset(&fmt, 0, sizeof(fmt));
    fmt.mSampleRate = (Float64) _saudio.requested_sample_rate;
    fmt.mFormatID = kAudioFormatLinearPCM;
    /* FIXME: use float sample data? */
    fmt.mFormatFlags = kLinearPCMFormatFlagIsSignedInteger | kAudioFormatFlagIsPacked;
    fmt.mBytesPerPacket = 4;
    fmt.mFramesPerPacket = 1;
    fmt.mBytesPerFrame = 4;
    fmt.mChannelsPerFrame = 2;
    fmt.mBitsPerChannel = 16;
    OSStatus res = AudioQueueNewOutput(&fmt, _sapp_ca_callback, 0, NULL, NULL, 0, &_saudio_ca_audio_queue);
    SOKOL_ASSERT((res == 0) && _saudio_ca_audio_queue);

    for (int i = 0; i < 2; i++) {
        AudioQueueBufferRef buf = NULL;
        const uint32_t buf_byte_size = _saudio.requested_audio_buffer_size * fmt.mBytesPerPacket;
        res = AudioQueueAllocateBuffer(_saudio_ca_audio_queue, buf_byte_size, &buf);
        SOKOL_ASSERT((res == 0) && buf);
        buf->mAudioDataByteSize = buf_byte_size;
        memset(buf->mAudioData, 0, buf->mAudioDataByteSize);
        AudioQueueEnqueueBuffer(_saudio_ca_audio_queue, buf, 0, NULL);
    }

    res = AudioQueueStart(_saudio_ca_audio_queue, NULL);
    SOKOL_ASSERT(0 == res);
}

_SOKOL_PRIVATE void _saudio_shutdown(void) {
    AudioQueueStop(_saudio_ca_audio_queue, true);
    AudioQueueDispose(_saudio_ca_audio_queue, false);
    _saudio_ca_audio_queue = NULL;
}

#else /* __APPLE__ */
void _saudio_init(void) { };
void _saudio_shutdown(void) { };
#endif

/*=== PUBLIC API FUNCTIONS ===================================================*/
void saudio_setup(const saudio_desc* desc) {
    SOKOL_ASSERT(!_saudio.valid);
    SOKOL_ASSERT(desc);
    memset(&_saudio, 0, sizeof(_saudio));
    _saudio.desc = *desc;
    _saudio.requested_sample_rate = _saudio_def(desc->sample_rate, 44100);
    _saudio.requested_audio_buffer_size = _saudio_def(desc->audio_buffer_size, 4096);
    if (desc->stream_cb) {
        SOKOL_ASSERT(0 == desc->push_buffer_size);
        _saudio.push_buffer_size = 0;
    }
    else {
        _saudio.push_buffer_size = _saudio_def(desc->push_buffer_size, 8192);
    }
    _saudio_init();
    SOKOL_ASSERT(_saudio.actual_sample_rate > 0);
    SOKOL_ASSERT(_saudio.actual_audio_buffer_size > 0);
    _saudio.valid = true;
}

void saudio_shutdown(void) {
    SOKOL_ASSERT(_saudio.valid);
    _saudio_shutdown();
    _saudio.valid = false;
}

bool saudio_isvalid(void) {
    return _saudio.valid;
}

int saudio_sample_rate(void) {
    return _saudio.actual_sample_rate;
}

int saudio_buffer_size(void) {
    return _saudio.actual_audio_buffer_size;
}

bool saudio_stereo(void) {
    return _saudio.actual_stereo;
}
#undef _saudio_def
#undef _saudio_def_flt

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* SOKOL_IMPL */
