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
    int sample_rate;        /* requested sample rate */
    int buffer_size;        /* number of samples in streaming buffer */
    int num_push_buffers;   /* number of push buffers available for queueing audio data */
    bool stereo;            /* default: false */
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
/* push sample from main-thread, returns false if no room for new data */
extern bool saudio_push(const float* samples, int num_samples);

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

/*--- implementation-private structures --------------------------------------*/
#define _SAUDIO_RING_SLOTS (8) /* MUST BE 2^N */
#define _SAUDIO_DEFAULT_SAMPLE_RATE (44100)
#define _SAUDIO_DEFAULT_BUFFER_SIZE (1024)
#define _SAUDIO_DEFAULT_PUSH_BUFFERS (4)

/*--- a ring-buffer queue implementation -------------------------------------*/
typedef struct {
    uint16_t head;  /* next slot to write to */
    uint16_t tail;  /* next slot to read from */
    int queue[_SAUDIO_RING_SLOTS];
} _saudio_ring;

_SOKOL_PRIVATE uint16_t _saudio_ring_idx(uint16_t i) {
    return i & (_SAUDIO_RING_SLOTS - 1);
}

_SOKOL_PRIVATE void _saudio_ring_init(_saudio_ring* ring) {
    ring->head = 0;
    ring->tail = 0;
    memset(ring->queue, 0, sizeof(ring->queue));
}

_SOKOL_PRIVATE bool _saudio_ring_full(_saudio_ring* ring) {
    return _saudio_ring_idx(ring->head + 1) == ring->tail;
}

_SOKOL_PRIVATE bool _saudio_ring_empty(_saudio_ring* ring) {
    return ring->head == ring->tail;
}

_SOKOL_PRIVATE void _saudio_ring_enqueue(_saudio_ring* ring, int val) {
    SOKOL_ASSERT(!_saudio_ring_full(ring));
    ring->queue[ring->head] = val;
    ring->head = _saudio_ring_idx(ring->head + 1);
}

_SOKOL_PRIVATE int _saudio_ring_dequeue(_saudio_ring* ring) {
    SOKOL_ASSERT(!_saudio_ring_empty(ring));
    int val = ring->queue[ring->tail];
    ring->tail = _saudio_ring_idx(ring->tail + 1);
    return val;
}

/*---  a buffer fifo for queueing audio data from main thread ----------------*/
typedef struct {
    int buffer_size;            /* size of a single push-buffer in bytes */
    _saudio_ring read_queue;    /* buffers with data, ready to be streamed */
    _saudio_ring write_queue;   /* empty buffers, ready to be pushed to */
} _saudio_fifo;

_SOKOL_PRIVATE void _saudio_fifo_init(_saudio_fifo* fifo, int buffer_size, int num_buffers) {
    fifo->buffer_size = buffer_size;
    _saudio_ring_init(&fifo->read_queue);
    _saudio_ring_init(&fifo->write_queue);
    for (int i = 0; i < (_SAUDIO_RING_SLOTS-1); i++) {
        _saudio_ring_enqueue(&fifo->write_queue, i);
    }
    SOKOL_ASSERT(_saudio_ring_full(&fifo->write_queue));
    SOKOL_ASSERT(_saudio_ring_empty(&fifo->read_queue));
}


/* the main system state */
typedef struct {
    bool valid;
    void (*stream_cb)(float* buffer, int num_samples);
    int sample_rate;            /* actual sample rate */
    int buffer_size;            /* actual buffer size in number of samples */
    int num_push_buffers;       /* number of push buffers for audio queuing */
    bool stereo;                /* actual mono/stereo state */
    saudio_desc desc;
    _saudio_fifo fifo;
} _saudio_state;
static _saudio_state _saudio;

/*=== COREAUDIO BACKEND ======================================================*/
#if defined(__APPLE__)
#include <AudioToolbox/AudioToolbox.h>

static AudioQueueRef _saudio_ca_audio_queue;

/* NOTE: the buffer data callback is called on a separate thread! */
_SOKOL_PRIVATE void _sapp_ca_callback(void* user_data, AudioQueueRef queue, AudioQueueBufferRef buffer) {
    if (_saudio.stream_cb) {
        _saudio.stream_cb((float*)buffer->mAudioData, buffer->mAudioDataByteSize / 4);
    }
    else {
        // FIXME: push model, pull audio samples from push buffer
    }
    AudioQueueEnqueueBuffer(queue, buffer, 0, NULL);
    // FIXME: update the 'play cursor position'
}

_SOKOL_PRIVATE void _saudio_init(void) {
    SOKOL_ASSERT(0 == _saudio_ca_audio_queue);

    /* create an audio queue with fp32 samples */
    AudioStreamBasicDescription fmt;
    memset(&fmt, 0, sizeof(fmt));
    fmt.mSampleRate = (Float64) _saudio.sample_rate;
    fmt.mFormatID = kAudioFormatLinearPCM;
    fmt.mFormatFlags = kLinearPCMFormatFlagIsFloat | kAudioFormatFlagIsPacked;
    fmt.mBytesPerPacket = 4;
    fmt.mFramesPerPacket = 1;
    fmt.mBytesPerFrame = 4;
    fmt.mChannelsPerFrame = 1;
    fmt.mBitsPerChannel = 32;
    OSStatus res = AudioQueueNewOutput(&fmt, _sapp_ca_callback, 0, NULL, NULL, 0, &_saudio_ca_audio_queue);
    SOKOL_ASSERT((res == 0) && _saudio_ca_audio_queue);

    /* create 2 audio buffers */
    for (int i = 0; i < 2; i++) {
        AudioQueueBufferRef buf = NULL;
        const uint32_t buf_byte_size = _saudio.buffer_size * fmt.mBytesPerPacket;
        res = AudioQueueAllocateBuffer(_saudio_ca_audio_queue, buf_byte_size, &buf);
        SOKOL_ASSERT((res == 0) && buf);
        buf->mAudioDataByteSize = buf_byte_size;
        memset(buf->mAudioData, 0, buf->mAudioDataByteSize);
        AudioQueueEnqueueBuffer(_saudio_ca_audio_queue, buf, 0, NULL);
    }

    /* ...and start playback */
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
    _saudio.stream_cb = desc->stream_cb;
    _saudio.sample_rate = _saudio_def(_saudio.desc.sample_rate, _SAUDIO_DEFAULT_SAMPLE_RATE);
    _saudio.buffer_size = _saudio_def(_saudio.desc.buffer_size, _SAUDIO_DEFAULT_BUFFER_SIZE);
    _saudio.num_push_buffers = _saudio_def(_saudio.desc.num_push_buffers, _SAUDIO_DEFAULT_PUSH_BUFFERS);
    _saudio.stereo = false; // FIXME!
    _saudio_fifo_init(&_saudio.fifo, _saudio.buffer_size, _saudio.num_push_buffers);
    _saudio_init();
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
    return _saudio.sample_rate;
}

int saudio_buffer_size(void) {
    return _saudio.buffer_size;
}

bool saudio_stereo(void) {
    return _saudio.stereo;
}
#undef _saudio_def
#undef _saudio_def_flt

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* SOKOL_IMPL */
