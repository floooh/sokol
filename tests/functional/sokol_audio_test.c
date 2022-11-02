//------------------------------------------------------------------------------
//  sokol_audio_test.c
//------------------------------------------------------------------------------
#define SOKOL_IMPL
#ifndef SOKOL_DUMMY_BACKEND
#define SOKOL_DUMMY_BACKEND
#endif
#include "sokol_audio.h"
#include "utest.h"

#define T(b) EXPECT_TRUE(b)

UTEST(sokol_audio, ring_buffer) {
    _saudio_ring_t rb;
    _saudio_ring_init(&rb, 4);
    T(0 == rb.head);
    T(0 == rb.tail);
    T(5 == rb.num);
    T(!_saudio_ring_full(&rb));
    T(_saudio_ring_empty(&rb));
    T(0 == _saudio_ring_count(&rb));

    _saudio_ring_enqueue(&rb, 23);
    T(1 == rb.head);
    T(0 == rb.tail);
    T(!_saudio_ring_full(&rb));
    T(!_saudio_ring_empty(&rb));
    T(1 == _saudio_ring_count(&rb));

    T(23 == _saudio_ring_dequeue(&rb));
    T(1 == rb.head);
    T(1 == rb.tail);
    T(!_saudio_ring_full(&rb));
    T(_saudio_ring_empty(&rb));
    T(0 == _saudio_ring_count(&rb));

    _saudio_ring_enqueue(&rb, 23);
    _saudio_ring_enqueue(&rb, 46);
    T(3 == rb.head);
    T(1 == rb.tail);
    T(!_saudio_ring_full(&rb));
    T(!_saudio_ring_empty(&rb));
    T(2 == _saudio_ring_count(&rb));
    T(23 == _saudio_ring_dequeue(&rb));
    T(46 == _saudio_ring_dequeue(&rb));
    T(3 == rb.head);
    T(3 == rb.tail);
    T(!_saudio_ring_full(&rb));
    T(_saudio_ring_empty(&rb));
    T(0 == _saudio_ring_count(&rb));

    _saudio_ring_enqueue(&rb, 12);
    _saudio_ring_enqueue(&rb, 34);
    _saudio_ring_enqueue(&rb, 56);
    _saudio_ring_enqueue(&rb, 78);
    T(2 == rb.head);
    T(3 == rb.tail);
    T(_saudio_ring_full(&rb));
    T(!_saudio_ring_empty(&rb));
    T(4 == _saudio_ring_count(&rb));
    T(12 == _saudio_ring_dequeue(&rb));
    T(2 == rb.head);
    T(4 == rb.tail);
    T(!_saudio_ring_full(&rb));
    T(!_saudio_ring_empty(&rb));
    T(3 == _saudio_ring_count(&rb));
    _saudio_ring_enqueue(&rb, 90);
    T(3 == rb.head);
    T(4 == rb.tail);
    T(_saudio_ring_full(&rb));
    T(!_saudio_ring_empty(&rb));
    T(4 == _saudio_ring_count(&rb));
    T(34 == _saudio_ring_dequeue(&rb));
    T(56 == _saudio_ring_dequeue(&rb));
    T(78 == _saudio_ring_dequeue(&rb));
    T(90 == _saudio_ring_dequeue(&rb));
    T(!_saudio_ring_full(&rb));
    T(_saudio_ring_empty(&rb));
    T(3 == rb.head);
    T(3 == rb.tail);
}

UTEST(saudio, api_test) {
    saudio_setup(&(saudio_desc){
        .sample_rate = 22050,
        .num_channels = 2,
        .buffer_frames = 8192,
        .num_packets = 128,
        .packet_frames = 8192 / 128,
        .user_data = (void*)12345
    });
    T(saudio_isvalid());
    T(saudio_query_desc().sample_rate == 22050);
    T(saudio_userdata() == (void*)12345);
    T(saudio_sample_rate() == 22050);
    T(saudio_channels() == 2);
    T(saudio_expect() == 8192);
    T(saudio_buffer_frames() == 8192);
}
