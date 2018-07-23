#pragma once
/*
    sokol_audio.h   -- minimalist cross-platform buffer-streaming audio API

    THIS IS HIGHLY EXPERIMENTAL AND WON'T BE FINISHED FOR A WHILE, 
    DON'T USE!

    TODO:
        - stereo support
        - Windows + Linux backends
        - write tests for the helper classes (ring queue, packet fifo)
        - need a callback when sample rate in the backend changes (this
          may happen when attaching/removing a playback device)
        - MAYBE: implement some sort of sample-rate limiting if the 
          actual sample rate is very high?
        - if the actual sample rate is a lot different than what was
          requested, the other requested parameters (most notably the
          stream buffer size and number of packets) might not be sufficient
          for glitch-free audioplayback... maybe need to do a 2-step setup?

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

    Glossary:
    =========
    stream buffer:
        A chunk of memory managed by the audio backend to be filled with
        new audio data by the stream callback. The size of the stream buffer
        defines the 'primary latency' between audio data written in the
        stream callback, and audible playback of the audio data.

    stream callback:
        A user-provided function which fills the stream buffer with new audio
        data. Depending on the audio backend, this may run in a separate thread.
        Providing a stream-callback function is the lowest-level and
        lowest-latency way to provide audio data.

    channel:
        A discrete track of audio, currently only 1-channel (mono) and
        2-channel (stereo) is supported.

    sample:
        The magnitude of an audio signal on one channel at a given time.
        In sokol-audio, samples are 32-bit floating numbers in the range
        -1.0 to +1.0.

    frame:
        The tightly packed set of samples for all channels at a given time.
        For mono 1 frame is 1 sample. For stereo, 1 frame is 2 samples.

    packet:
        In sokol-audio, a small chunk of audio data that is moved from the
        main thread to the audio streaming thread in order to decouple the
        rate at which the main thread provides new audio data, and the
        streaming thread consuming audio data. When the main thread
        'pushes' data to sokol-audio, the data will be copied into
        intermediate packets and queued up for the streaming callback. When
        the streaming callback is called, it will pull packets from the
        queue and copy the data into the stream buffer. If there are not
        enough packets waiting to be streamed, streaming will 'starve'
        and the stream buffer will be filled with silence until enough
        packets are queued up again the fill the entire buffer (this will lead
        to audible artefacts like crackling). The opposite is that the main
        thread provides data faster then is streamed, in this case the packet
        queue will run full,
        and data written by the main thread will be discarded. This will also
        lead to crackling. To provide glitch-free playback, the main thread
        should always push as much data required to fill up the packet
        queue (this can be queried via saudio_expect()).

    WebAudio Backend:
    =================
    The first implementation will use ScriptProcessorNode, later, Audio Worklets
    will be used:

    https://developers.google.com/web/updates/2017/12/audio-worklet
    https://developers.google.com/web/updates/2018/06/audio-worklet-design-pattern

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
    int buffer_frames;      /* number of frames in streaming buffer */
    int packet_frames;      /* number of frames in a packet */
    int num_packets;        /* number of packets in packet queue */
    int num_channels;       /* number of channels, default: 1 (mono) */
    void (*stream_cb)(float* buffer, int num_samples);  /* optional streaming callback */
} saudio_desc;

/* setup sokol-audio */
extern void saudio_setup(const saudio_desc* desc);
/* shutdown sokol-audio */
extern void saudio_shutdown(void);
/* true after setup if audio backend was successfully initialized */
extern bool saudio_isvalid(void);
/* actual sample rate */
extern int saudio_sample_rate(void);
/* actual backend buffer size */
extern int saudio_buffer_size(void);
/* actual number of channels */
extern int saudio_channels(void);
/* get current number of frames to fill packet queue */
extern int saudio_expect(void);
/* push sample frames from main thread, returns number of frames actually pushed */
extern int saudio_push(const float* frames, int num_frames);

#ifdef __cplusplus
} /* extern "C" */
#endif

/*--- IMPLEMENTATION ---------------------------------------------------------*/
#ifdef SOKOL_IMPL

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable:4505)   /* unreferenced local function has been removed */
#endif

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

#define _saudio_def(val, def) (((val) == 0) ? (def) : (val))
#define _saudio_def_flt(val, def) (((val) == 0.0f) ? (def) : (val))

/*--- implementation-private structures --------------------------------------*/
#define _SAUDIO_DEFAULT_SAMPLE_RATE (44100)
#define _SAUDIO_DEFAULT_BUFFER_FRAMES (512)
#define _SAUDIO_DEFAULT_PACKET_FRAMES (128)
#define _SAUDIO_DEFAULT_NUM_PACKETS ((_SAUDIO_DEFAULT_BUFFER_FRAMES/_SAUDIO_DEFAULT_PACKET_FRAMES)*4)
#define _SAUDIO_RING_MAX_SLOTS (64)

/*--- mutex wrappers ---------------------------------------------------------*/
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
    return (uint16_t) (i % ring->num);
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

/*---  a packet fifo for queueing audio data from main thread ----------------*/
typedef struct {
    int packet_size;            /* size of a single packets in bytes(!) */
    int num_packets;            /* number of packet in fifo */
    uint8_t* base_ptr;          /* packet memory chunk base pointer (dynamically allocated) */
    int cur_packet;             /* current write-packet */
    int cur_offset;             /* current byte-offset into current write packet */
    _saudio_ring read_queue;    /* buffers with data, ready to be streamed */
    _saudio_ring write_queue;   /* empty buffers, ready to be pushed to */
} _saudio_fifo;

_SOKOL_PRIVATE void _saudio_fifo_init(_saudio_fifo* fifo, int packet_size, int num_packets) {
    SOKOL_ASSERT((packet_size > 0) && (num_packets > 0));
    fifo->packet_size = packet_size;
    fifo->num_packets = num_packets;
    fifo->base_ptr = (uint8_t*) SOKOL_MALLOC(packet_size * num_packets);
    SOKOL_ASSERT(fifo->base_ptr);
    fifo->cur_packet = -1;
    fifo->cur_offset = 0;
    _saudio_ring_init(&fifo->read_queue, num_packets);
    _saudio_ring_init(&fifo->write_queue, num_packets);
    for (int i = 0; i < num_packets; i++) {
        _saudio_ring_enqueue(&fifo->write_queue, i);
    }
    SOKOL_ASSERT(_saudio_ring_full(&fifo->write_queue));
    SOKOL_ASSERT(_saudio_ring_count(&fifo->write_queue) == num_packets);
    SOKOL_ASSERT(_saudio_ring_empty(&fifo->read_queue));
    SOKOL_ASSERT(_saudio_ring_count(&fifo->read_queue) == 0);
}

_SOKOL_PRIVATE void _saudio_fifo_shutdown(_saudio_fifo* fifo) {
    SOKOL_ASSERT(fifo->base_ptr);
    SOKOL_FREE(fifo->base_ptr);
    fifo->base_ptr = 0;
}

_SOKOL_PRIVATE int _saudio_fifo_writable_bytes(_saudio_fifo* fifo) {
    _saudio_mutex_lock();
    int num_bytes = (_saudio_ring_count(&fifo->write_queue) * fifo->packet_size);
    if (fifo->cur_packet != -1) {
        num_bytes += fifo->packet_size - fifo->cur_offset;
    }
    _saudio_mutex_unlock();
    SOKOL_ASSERT((num_bytes >= 0) && (num_bytes <= (fifo->num_packets * fifo->packet_size)));
    return num_bytes;
}

/* write new data to the write queue, this is called from main thread */
_SOKOL_PRIVATE int _saudio_fifo_write(_saudio_fifo* fifo, const uint8_t* ptr, int num_bytes) {
    /* returns the number of bytes written, this will be smaller then requested
        if the write queue runs full
    */
    int all_to_copy = num_bytes;
    while (all_to_copy > 0) {
        /* need to grab a new packet? */
        if (fifo->cur_packet == -1) {
            _saudio_mutex_lock();
            if (!_saudio_ring_empty(&fifo->write_queue)) {
                fifo->cur_packet = _saudio_ring_dequeue(&fifo->write_queue);
            }
            _saudio_mutex_unlock();
            SOKOL_ASSERT(fifo->cur_offset == 0);
        }
        /* append data to current write packet */
        if (fifo->cur_packet != -1) {
            int to_copy = all_to_copy;
            const int max_copy = fifo->packet_size - fifo->cur_offset;
            if (to_copy > max_copy) {
                to_copy = max_copy;
            }
            uint8_t* dst = fifo->base_ptr + fifo->cur_packet * fifo->packet_size + fifo->cur_offset;
            memcpy(dst, ptr, to_copy);
            ptr += to_copy;
            fifo->cur_offset += to_copy;
            all_to_copy -= to_copy;
            SOKOL_ASSERT(fifo->cur_offset <= fifo->packet_size);
            SOKOL_ASSERT(all_to_copy >= 0);
        }
        else {
            /* early out if we're starving */
            int bytes_copied = num_bytes - all_to_copy;
            SOKOL_ASSERT((bytes_copied >= 0) && (bytes_copied < num_bytes));
            return bytes_copied;
        }
        /* if write packet is full, push to read queue */
        if (fifo->cur_offset == fifo->packet_size) {
            _saudio_mutex_lock();
            _saudio_ring_enqueue(&fifo->read_queue, fifo->cur_packet);
            _saudio_mutex_unlock();
            fifo->cur_packet = -1;
            fifo->cur_offset = 0;
        }
    }
    SOKOL_ASSERT(all_to_copy == 0);
    return num_bytes;
}

/* read queued data, this is called form the stream callback (maybe separate thread) */
_SOKOL_PRIVATE int _saudio_fifo_read(_saudio_fifo* fifo, uint8_t* ptr, int num_bytes) {
    SOKOL_ASSERT(0 == (num_bytes % fifo->packet_size));
    SOKOL_ASSERT(num_bytes <= (fifo->packet_size * fifo->num_packets));
    const int num_packets_needed = num_bytes / fifo->packet_size;
    int num_bytes_copied = 0;
    uint8_t* dst = ptr;
    /* either pull a full buffer worth of data, or nothing */
    _saudio_mutex_lock();
    if (_saudio_ring_count(&fifo->read_queue) >= num_packets_needed) {
        for (int i = 0; i < num_packets_needed; i++) {
            int packet_index = _saudio_ring_dequeue(&fifo->read_queue);
            _saudio_ring_enqueue(&fifo->write_queue, packet_index);
            const uint8_t* src = fifo->base_ptr + packet_index * fifo->packet_size;
            memcpy(dst, src, fifo->packet_size);
            dst += fifo->packet_size;
            num_bytes_copied += fifo->packet_size;
        }
        SOKOL_ASSERT(num_bytes == num_bytes_copied);
    }
    _saudio_mutex_unlock();
    return num_bytes_copied;
}

/* sokol-audio state */
typedef struct {
    bool valid;
    void (*stream_cb)(float* buffer, int num_samples);
    int sample_rate;            /* sample rate */
    int buffer_frames;          /* number of frames in streaming buffer */
    int bytes_per_frame;        /* filled by backend */
    int packet_frames;          /* number of frames in a packet */
    int num_packets;            /* number of packets in packet queue */
    int num_channels;           /* actual number of channels */
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
        uint8_t* ptr = (uint8_t*)buffer->mAudioData;
        int num_bytes = (int) buffer->mAudioDataByteSize;
        if (0 == _saudio_fifo_read(&_saudio.fifo, ptr, num_bytes)) {
            /* not enough read data available, fill the entire buffer with silence */
            memset(ptr, 0, num_bytes);
        }
    }
    AudioQueueEnqueueBuffer(queue, buffer, 0, NULL);
}

_SOKOL_PRIVATE bool _saudio_backend_init(void) {
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
        const uint32_t buf_byte_size = _saudio.buffer_frames * fmt.mBytesPerFrame;
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
    _saudio.bytes_per_frame = fmt.mBytesPerFrame;
    return true;
}

_SOKOL_PRIVATE void _saudio_backend_shutdown(void) {
    AudioQueueStop(_saudio_ca_audio_queue, true);
    AudioQueueDispose(_saudio_ca_audio_queue, false);
    _saudio_ca_audio_queue = NULL;
}

/*=== EMSCRIPTEN BACKEND =====================================================*/

/* FIXME: resume WebAudio context on user interaction */

#elif defined(__EMSCRIPTEN__)
#include <emscripten/emscripten.h>

static uint8_t* _saudio_emsc_buffer;

EMSCRIPTEN_KEEPALIVE int _saudio_emsc_pull(int num_frames) {
    SOKOL_ASSERT(_saudio_emsc_buffer);
    if (num_frames == _saudio.buffer_frames) {
        const int num_bytes = num_frames * _saudio.bytes_per_frame;
        if (0 == _saudio_fifo_read(&_saudio.fifo, _saudio_emsc_buffer, num_bytes)) {
            /* not enough read data available, fill the entire buffer with silence */
            memset(_saudio_emsc_buffer, 0, num_bytes);
        }
        int res = (int) _saudio_emsc_buffer;
        return res;
    }
    else {
        return 0;
    }
}

/* setup the WebAudio context and attach a ScriptProcessorNode */
EM_JS(int, _saudio_js_init, (int sample_rate, int buffer_size), {
    Module._saudio_context = null;
    Module._saudio_node = null;
    if (typeof AudioContext !== 'undefined') {
        Module._saudio_context = new AudioContext({
            sampleRate: sample_rate,
            latencyHint: 'interactive',
        });
        console.log('sokol_audio.h: created AudioContext');
    }
    else if (typeof webkitAudioContext !== 'undefined') {
        Module._saudio_context = new webkitAudioContext({
            sampleRate: sample_rate,
            latencyHint: 'interactive',
        });
        console.log('sokol_audio.h: created webkitAudioContext');
    }
    else {
        Module._saudio_context = null;
        console.log('sokol_audio.h: no WebAudio support');
    }
    if (Module._saudio_context) {
        console.log('sokol_audio.h: sample rate ', Module._saudio_context.sampleRate);
        Module._saudio_node = Module._saudio_context.createScriptProcessor(buffer_size, 0, 1);
        Module._saudio_node.onaudioprocess = function pump_audio(event) {
            var buf_size = event.outputBuffer.length;
            var ptr = Module.ccall('_saudio_emsc_pull', 'number', ['number'], [buf_size]);
            if (ptr) {
                var chan = event.outputBuffer.getChannelData(0);
                for (var i = 0; i < buf_size; i++) {
                    var heap_index = (ptr>>2) + i;
                    chan[i] = HEAPF32[heap_index];
                }
            }
        };
        Module._saudio_node.connect(Module._saudio_context.destination);
        return 1;
    }
    else {
        return 0;
    }
});

/* get the actual sample rate back from the WebAudio context */
EM_JS(int, _saudio_js_sample_rate, (), {
    if (Module._saudio_context) {
        return Module._saudio_context.sampleRate;
    }
    else {
        return 0;
    }
});

/* get the actual buffer size in number of frames */
EM_JS(int, _saudio_js_buffer_frames, (), {
    if (Module._saudio_node) {
        return Module._saudio_node.bufferSize;
    }
    else {
        return 0;
    }
});

_SOKOL_PRIVATE bool _saudio_backend_init(void) {
    if (_saudio_js_init(_saudio.sample_rate, _saudio.buffer_frames)) {
        _saudio.num_channels = 1;
        _saudio.bytes_per_frame = 4;
        _saudio.sample_rate = _saudio_js_sample_rate();
        _saudio.buffer_frames = _saudio_js_buffer_frames();
        const int buf_size = _saudio.buffer_frames * _saudio.bytes_per_frame;
        _saudio_emsc_buffer = SOKOL_MALLOC(buf_size);
        return true;
    }
    else {
        return false;
    }
}

_SOKOL_PRIVATE void _saudio_backend_shutdown(void) {
    /* on HTML5, there's always a 'hard exit' without warning,
        so nothing useful to do here
    */
}

#else /* dummy backend */
bool _saudio_backend_init(void) { return false; };
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
    _saudio.buffer_frames = _saudio_def(_saudio.desc.buffer_frames, _SAUDIO_DEFAULT_BUFFER_FRAMES);
    _saudio.packet_frames = _saudio_def(_saudio.desc.packet_frames, _SAUDIO_DEFAULT_PACKET_FRAMES);
    _saudio.num_packets = _saudio_def(_saudio.desc.num_packets, _SAUDIO_DEFAULT_NUM_PACKETS);
    _saudio.num_channels = _saudio_def(_saudio.desc.num_channels, 1);
    _saudio_mutex_init();
    if (_saudio_backend_init()) {
        SOKOL_ASSERT(0 == (_saudio.buffer_frames % _saudio.packet_frames));
        SOKOL_ASSERT(_saudio.bytes_per_frame > 0);
        _saudio_fifo_init(&_saudio.fifo, _saudio.packet_frames * _saudio.bytes_per_frame, _saudio.num_packets);
        _saudio.valid = true;
    }
}

void saudio_shutdown(void) {
    if (_saudio.valid) {
        _saudio_backend_shutdown();
        _saudio_fifo_shutdown(&_saudio.fifo);
        _saudio.valid = false;
    }
    _saudio_mutex_destroy();
}

bool saudio_isvalid(void) {
    return _saudio.valid;
}

int saudio_sample_rate(void) {
    return _saudio.sample_rate;
}

int saudio_buffer_frames(void) {
    return _saudio.buffer_frames;
}

int saudio_channels(void) {
    return _saudio.num_channels;
}

int saudio_expect(void) {
    if (_saudio.valid) {
        const int num_frames = _saudio_fifo_writable_bytes(&_saudio.fifo) / _saudio.bytes_per_frame;
        return num_frames;
    }
    else {
        return 0;
    }
}

int saudio_push(const float* frames, int num_frames) {
    SOKOL_ASSERT(frames && (num_frames > 0));
    if (_saudio.valid) {
        const int num_bytes = num_frames * _saudio.bytes_per_frame;
        const int num_written = _saudio_fifo_write(&_saudio.fifo, (const uint8_t*)frames, num_bytes);
        return num_written / _saudio.bytes_per_frame;
    }
    else {
        return 0;
    }
}

#undef _saudio_def
#undef _saudio_def_flt

#ifdef _MSC_VER
#pragma warning(pop)
#endif

#endif /* SOKOL_IMPL */
