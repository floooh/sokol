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

    Streaming Buffer vs Push Buffers
    ================================
    Streaming buffers are created by and under control of the native audio
    backend, and they are populated by the streaming callback (which may
    be running on a separate thread). The size of the streaming buffer
    defines the 'low-level-latency' (the smaller the streaming buffer, the
    lower the latency).

    Push Buffers are smaller 'packet-size buffer' managed by sokol-audio. 
    They are used in a buffer queue where audio data coming from the 
    application main thread (via saudio_push()) is stored until consumed
    by the audio backend streaming callback. The size of push buffers
    must be smaller or equal to the size of the streaming buffer, and the
    streaming buffer size must be a multiple of the push buffer size.

    Whenever a complete push buffer has been filled with sample data, it
    will be moved from an internal "write queue" to another internal
    "read queue", where it is waiting to be pulled by the streaming callback.

    Whenever the streaming callback is called, it will pull push buffers
    from the read queue until the streaming buffer is completely filled.
    If there are not enough push buffers waiting to fill a complete streaming
    buffer, there's a "starving situation". The streaming callback will 
    fill the remaing stream buffer with silence, and communicate back to the
    main thread how much data was missing (this can be queried with 
    saudio_query_push_state()). After a starving situation, the streaming callback
    will wait until there's enough data in the read queue to fill a complete
    streaming buffer until 'resuming normal operation'. The opposite of
    starving is when the push buffer queue is full and cannot accept new
    data, this is happens when the main thread is providing new audio faster
    then the streaming callback consumes the data.

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
#include <string.h> /* memset, memcpy */

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    int sample_rate;        /* requested sample rate */
    int stream_buffer_size; /* number of samples in streaming buffer */
    int push_buffer_size;   /* size of a push buffer, must be a multiple of buffer_size */
    int num_push_buffers;   /* number of push buffers available for queueing audio data */
    bool stereo;            /* default: false */
    /* optional low-level callback to provide samples */
    void (*stream_cb)(float* buffer, int num_samples);
} saudio_desc;

typedef enum {
    SAUDIO_PUSHSTATE_STARVING,        /* audio thread is starving and had to include silence */
    SAUDIO_PUSHSTATE_OK,              /* all ok */
    SAUDIO_PUSHSTATE_FULL,            /* push buffer queue is full */
} saudio_push_state;

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
/* get number of samples that can be pushed */
extern int saudio_pushable_samples(void);
/* query current state of push buffer queue */
extern saudio_push_state saudio_query_push_state(void);

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
#define _SAUDIO_DEFAULT_SAMPLE_RATE (44100)
#define _SAUDIO_DEFAULT_STREAM_BUFFER_SIZE (512)
#define _SAUDIO_DEFAULT_PUSH_BUFFER_SIZE (64)
#define _SAUDIO_DEFAULT_NUM_PUSH_BUFFERS (32)
#define _SAUDIO_RING_MAX_SLOTS (64)

/*--- mutex wrappers ---------------------------------------------------------*/
/* FIXME: we should probably allow to override the lock/unlock functions via macro */
#if defined(__APPLE__) || defined(linux)
#include "pthread.h"
static pthread_mutex_t _saudio_mutex;

_SOKOL_PRIVATE void _saudio_mutex_init(void) {
    pthread_mutexattr_t attr;
    pthread_mutexattr_init(&attr);
    pthread_mutex_init(&_saudio_mutex, &attr);
}

_SOKOL_PRIVATE void _saudio_mutex_destroy(void) {
    pthread_mutex_destroy(&_saudio_mutex);
}

_SOKOL_PRIVATE void _saudio_mutex_lock(void) {
    pthread_mutex_lock(&_saudio_mutex);
}

_SOKOL_PRIVATE void _saudio_mutex_unlock(void) {
    pthread_mutex_unlock(&_saudio_mutex);
}
#else
// FIXME!
_SOKOL_PRIVATE void _saudio_mutex_init(void) { }
_SOKOL_PRIVATE void _saudio_mutex_destroy(void) { }
_SOKOL_PRIVATE void _saudio_mutex_lock(void) { }
_SOKOL_PRIVATE void _saudio_mutex_unlock(void) { }
#endif

/*--- a ring-buffer queue implementation -------------------------------------*/
typedef struct {
    int head;  /* next slot to write to */
    int tail;  /* next slot to read from */
    int num;   /* number of slots in queue */
    int queue[_SAUDIO_RING_MAX_SLOTS];
} _saudio_ring;

_SOKOL_PRIVATE uint16_t _saudio_ring_idx(_saudio_ring* ring, int i) {
    return i % ring->num;
}

_SOKOL_PRIVATE void _saudio_ring_init(_saudio_ring* ring, int num_slots) {
    SOKOL_ASSERT((num_slots + 1) <= _SAUDIO_RING_MAX_SLOTS);
    ring->head = 0;
    ring->tail = 0;
    /* one slot reserved to detect 'full' vs 'empty' */
    ring->num = num_slots + 1;
    memset(ring->queue, 0, sizeof(ring->queue));
}

_SOKOL_PRIVATE bool _saudio_ring_full(_saudio_ring* ring) {
    return _saudio_ring_idx(ring, ring->head + 1) == ring->tail;
}

_SOKOL_PRIVATE bool _saudio_ring_empty(_saudio_ring* ring) {
    return ring->head == ring->tail;
}

_SOKOL_PRIVATE int _saudio_ring_count(_saudio_ring* ring) {
    int count;
    if (ring->head >= ring->tail) {
        count = ring->head - ring->tail;
    }
    else {
        count = (ring->head + ring->num) - ring->tail;
    }
    SOKOL_ASSERT((count >= 0) && (count < ring->num));
    return count;
}

_SOKOL_PRIVATE void _saudio_ring_enqueue(_saudio_ring* ring, int val) {
    SOKOL_ASSERT(!_saudio_ring_full(ring));
    ring->queue[ring->head] = val;
    ring->head = _saudio_ring_idx(ring, ring->head + 1);
}

_SOKOL_PRIVATE int _saudio_ring_dequeue(_saudio_ring* ring) {
    SOKOL_ASSERT(!_saudio_ring_empty(ring));
    int val = ring->queue[ring->tail];
    ring->tail = _saudio_ring_idx(ring, ring->tail + 1);
    return val;
}

/*---  a buffer fifo for queueing audio data from main thread ----------------*/
typedef struct {
    int buffer_size;            /* size of a single push-buffer in bytes */
    int num_buffers;            /* number of push buffers in fifo */
    uint8_t* buffer_base;       /* push buffers base pointer (dynamically allocated) */
    int cur_buffer;             /* current write-buffer */
    int cur_offset;             /* current byte-offset into current write buffer */
    _saudio_ring read_queue;    /* buffers with data, ready to be streamed */
    _saudio_ring write_queue;   /* empty buffers, ready to be pushed to */
} _saudio_fifo;

_SOKOL_PRIVATE void _saudio_fifo_init(_saudio_fifo* fifo, int buffer_size, int num_buffers) {
    SOKOL_ASSERT((buffer_size > 0) && (num_buffers > 0));
    fifo->buffer_size = buffer_size;
    fifo->num_buffers = num_buffers;
    fifo->buffer_base = SOKOL_MALLOC(buffer_size * num_buffers);
    SOKOL_ASSERT(fifo->buffer_base);
    fifo->cur_buffer = -1;
    fifo->cur_offset = 0;
    _saudio_ring_init(&fifo->read_queue, num_buffers);
    _saudio_ring_init(&fifo->write_queue, num_buffers);
    for (int i = 0; i < num_buffers; i++) {
        _saudio_ring_enqueue(&fifo->write_queue, i);
    }
    SOKOL_ASSERT(_saudio_ring_full(&fifo->write_queue));
    SOKOL_ASSERT(_saudio_ring_count(&fifo->write_queue) == num_buffers);
    SOKOL_ASSERT(_saudio_ring_empty(&fifo->read_queue));
    SOKOL_ASSERT(_saudio_ring_count(&fifo->read_queue) == 0);
}

_SOKOL_PRIVATE void _saudio_fifo_shutdown(_saudio_fifo* fifo) {
    SOKOL_ASSERT(fifo->buffer_base);
    SOKOL_FREE(fifo->buffer_base);
    fifo->buffer_base = 0;
}

_SOKOL_PRIVATE int _saudio_fifo_writable_bytes(_saudio_fifo* fifo) {
    _saudio_mutex_lock();
    int num_bytes = _saudio_ring_count(&fifo->write_queue) * fifo->buffer_size;
    _saudio_mutex_unlock();
    return num_bytes;
}

_SOKOL_PRIVATE int _saudio_fifo_write(_saudio_fifo* fifo, const uint8_t* ptr, int num_bytes) {
    /* returns the number of bytes written, this will be smaller then requested
        if the write queue runs full
    */
    int all_to_copy = num_bytes;
    while (all_to_copy > 0) {
        if ((fifo->cur_buffer == -1) || (fifo->cur_offset == fifo->buffer_size)) {
            _saudio_mutex_lock();
            if (fifo->cur_buffer != -1) {
                /* current buffer is full, transfer to read queue */
                _saudio_ring_enqueue(&fifo->read_queue, fifo->cur_buffer);
            }
            /* grab next buffer from write queue */
            if (!_saudio_ring_empty(&fifo->write_queue)) {
                fifo->cur_buffer = _saudio_ring_dequeue(&fifo->write_queue);
            }
            else {
                /* write queue is empty, we're starving */
                fifo->cur_buffer = -1;
            }
            _saudio_mutex_unlock();
            fifo->cur_offset = 0;
        }
        if (fifo->cur_buffer != -1) {
            int to_copy = all_to_copy;
            const int max_copy = fifo->buffer_size - fifo->cur_offset;
            if (to_copy > max_copy) {
                to_copy = max_copy;
            }
            uint8_t* dst = fifo->buffer_base + fifo->cur_buffer*fifo->buffer_size + fifo->cur_offset;
            memcpy(dst, ptr, to_copy);
            ptr += to_copy;
            fifo->cur_offset += to_copy;
            all_to_copy -= to_copy;
            SOKOL_ASSERT(fifo->cur_offset <= fifo->buffer_size);
            SOKOL_ASSERT(all_to_copy >= 0);
        }
        else {
            /* early out if starving */
            int bytes_copied = num_bytes - all_to_copy;
            SOKOL_ASSERT((bytes_copied >= 0) && (bytes_copied < num_bytes));
            return bytes_copied;
        }
    }
    SOKOL_ASSERT(all_to_copy == 0);
    return num_bytes;
}

_SOKOL_PRIVATE int _saudio_fifo_read(_saudio_fifo* fifo, uint8_t* ptr, int num_bytes) {
    SOKOL_ASSERT(0 == num_bytes % fifo->buffer_size);
    SOKOL_ASSERT(num_bytes <= (fifo->buffer_size * fifo->num_buffers));
    const int num_push_buffers_needed = num_bytes / fifo->buffer_size;
    int num_bytes_copied = 0;
    uint8_t* dst = ptr;
    _saudio_mutex_lock();
    if (_saudio_ring_count(&fifo->read_queue) >= num_push_buffers_needed) {
        for (int i = 0; i < num_push_buffers_needed; i++) {
            int buf = _saudio_ring_dequeue(&fifo->read_queue);
            _saudio_ring_enqueue(&fifo->write_queue, buf);
            const uint8_t* src = fifo->buffer_base + buf*fifo->buffer_size;
            memcpy(dst, src, fifo->buffer_size);
            dst += fifo->buffer_size;
            num_bytes_copied += fifo->buffer_size;
        }
        SOKOL_ASSERT(num_bytes == num_bytes_copied);
    }
    _saudio_mutex_unlock();
    return num_bytes_copied;
}

/* the main system state */
typedef struct {
    bool valid;
    void (*stream_cb)(float* buffer, int num_samples);
    int sample_rate;            /* sample rate */
    int stream_buffer_size;     /* buffer size in number of samples */
    int bytes_per_sample;       /* filled by backend */
    int push_buffer_size;       /* push buffer size in number of samples */
    int num_push_buffers;
    bool stereo;                /* actual mono/stereo state */
    saudio_desc desc;
    _saudio_fifo fifo;
    saudio_push_state push_state;   /* starving, ok or full */
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
        uint8_t* ptr = (uint8_t*)buffer->mAudioData;
        int num_bytes = (int) buffer->mAudioDataByteSize;
        if (0 == _saudio_fifo_read(&_saudio.fifo, ptr, num_bytes)) {
            /* not enough read data available, fill the entire buffer with silence */
            memset(ptr, 0, num_bytes);
        }
    }
    AudioQueueEnqueueBuffer(queue, buffer, 0, NULL);
}

_SOKOL_PRIVATE void _saudio_backend_init(void) {
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
        const uint32_t buf_byte_size = _saudio.stream_buffer_size * fmt.mBytesPerPacket;
        res = AudioQueueAllocateBuffer(_saudio_ca_audio_queue, buf_byte_size, &buf);
        SOKOL_ASSERT((res == 0) && buf);
        buf->mAudioDataByteSize = buf_byte_size;
        memset(buf->mAudioData, 0, buf->mAudioDataByteSize);
        AudioQueueEnqueueBuffer(_saudio_ca_audio_queue, buf, 0, NULL);
    }

    /* ...and start playback */
    res = AudioQueueStart(_saudio_ca_audio_queue, NULL);
    SOKOL_ASSERT(0 == res);

    /* init or modify actual playback parameters */
    _saudio.bytes_per_sample = 4;
}

_SOKOL_PRIVATE void _saudio_backend_shutdown(void) {
    AudioQueueStop(_saudio_ca_audio_queue, true);
    AudioQueueDispose(_saudio_ca_audio_queue, false);
    _saudio_ca_audio_queue = NULL;
}

#else /* __APPLE__ */
void _saudio_backend_init(void) { };
void _saudio_backend_shutdown(void) { };
#endif

/*=== PUBLIC API FUNCTIONS ===================================================*/
void saudio_setup(const saudio_desc* desc) {
    SOKOL_ASSERT(!_saudio.valid);
    SOKOL_ASSERT(desc);
    memset(&_saudio, 0, sizeof(_saudio));
    _saudio.desc = *desc;
    _saudio.stream_cb = desc->stream_cb;
    _saudio.sample_rate = _saudio_def(_saudio.desc.sample_rate, _SAUDIO_DEFAULT_SAMPLE_RATE);
    _saudio.stream_buffer_size = _saudio_def(_saudio.desc.stream_buffer_size, _SAUDIO_DEFAULT_STREAM_BUFFER_SIZE);
    _saudio.push_buffer_size = _saudio_def(_saudio.desc.push_buffer_size, _SAUDIO_DEFAULT_PUSH_BUFFER_SIZE);
    _saudio.num_push_buffers = _saudio_def(_saudio.desc.num_push_buffers, _SAUDIO_DEFAULT_NUM_PUSH_BUFFERS);
    _saudio.stereo = false; // FIXME!
    _saudio.push_state = SAUDIO_PUSHSTATE_STARVING;
    _saudio_mutex_init();
    _saudio_backend_init();
    SOKOL_ASSERT(0 == (_saudio.stream_buffer_size % _saudio.push_buffer_size));
    SOKOL_ASSERT(_saudio.bytes_per_sample > 0);
    _saudio_fifo_init(&_saudio.fifo, _saudio.push_buffer_size * _saudio.bytes_per_sample, _saudio.num_push_buffers);
    _saudio.valid = true;
}

void saudio_shutdown(void) {
    SOKOL_ASSERT(_saudio.valid);
    _saudio_backend_shutdown();
    _saudio_fifo_shutdown(&_saudio.fifo);
    _saudio_mutex_destroy();
    _saudio.valid = false;
}

bool saudio_isvalid(void) {
    return _saudio.valid;
}

int saudio_sample_rate(void) {
    return _saudio.sample_rate;
}

int saudio_stream_buffer_size(void) {
    return _saudio.stream_buffer_size;
}

bool saudio_stereo(void) {
    return _saudio.stereo;
}

bool saudio_push(const float* samples, int num_samples) {
    SOKOL_ASSERT(samples && (num_samples > 0));
    const int num_bytes = num_samples * _saudio.bytes_per_sample;
    const int num_written = _saudio_fifo_write(&_saudio.fifo, (const uint8_t*)samples, num_bytes);
    return num_written == num_bytes;
}

int saudio_pushable_samples(void) {
    const int num_samples = _saudio_fifo_writable_bytes(&_saudio.fifo) / _saudio.bytes_per_sample;
    return num_samples;
}

#undef _saudio_def
#undef _saudio_def_flt

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* SOKOL_IMPL */
