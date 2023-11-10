//------------------------------------------------------------------------------
//  sokol-fetch-test.c
//
//  FIXME: simulate allocation errors
//------------------------------------------------------------------------------
#define SOKOL_IMPL
#define SFETCH_MAX_USERDATA_UINT64 (8)
#define SFETCH_MAX_PATH (32)
#include "sokol_fetch.h"
#include "utest.h"

#define T(b) EXPECT_TRUE(b)
#define TSTR(s0, s1) EXPECT_TRUE(0 == strcmp(s0,s1))

static uint8_t load_file_buf[500000];
static const uint64_t combatsignal_file_size = 409482;

typedef struct {
    int a, b, c;
} userdata_t;

static const _sfetch_item_t zeroed_item = {0};

#ifdef _WIN32
#include <windows.h>
static void sleep_ms(int ms) {
    Sleep((DWORD)ms);
}
#else
#include <unistd.h>
static void sleep_ms(uint32_t ms) {
    usleep(ms * 1000);
}
#endif

/* private implementation functions */
UTEST(sokol_fetch, path_make) {
    const char* str31 = "1234567890123456789012345678901";
    const char* str32 = "12345678901234567890123456789012";
    // max allowed string length (MAX_PATH - 1)
    _sfetch_path_t p31 = _sfetch_path_make(str31);
    TSTR(p31.buf, str31);
    // overflow
    _sfetch_path_t p32 = _sfetch_path_make(str32);
    T(p32.buf[0] == 0);
}

UTEST(sokol_fetch, make_id) {
    uint32_t slot_id = _sfetch_make_id(123, 456);
    T(slot_id == ((456<<16)|123));
    T(_sfetch_slot_index(slot_id) == 123);
}

UTEST(sokol_fetch, item_init_discard) {
    userdata_t user_data = {
        .a = 123,
        .b = 456,
        .c = 789
    };
    sfetch_request_t request = {
        .channel = 4,
        .path = "hello_world.txt",
        .chunk_size = 128,
        .user_data = SFETCH_RANGE(user_data)
    };
    _sfetch_item_t item = zeroed_item;
    uint32_t slot_id = _sfetch_make_id(1, 1);
    _sfetch_item_init(&item, slot_id, &request);
    T(item.handle.id == slot_id);
    T(item.channel == 4);
    T(item.lane == _SFETCH_INVALID_LANE);
    T(item.chunk_size == 128);
    T(item.state == _SFETCH_STATE_INITIAL);
    TSTR(item.path.buf, request.path);
    T(item.user.user_data_size == sizeof(userdata_t));
    const userdata_t* ud = (const userdata_t*) item.user.user_data;
    T((((uintptr_t)ud) & 0x7) == 0); // check alignment
    T(ud->a == 123);
    T(ud->b == 456);
    T(ud->c == 789);

    item.state = _SFETCH_STATE_FETCHING;
    _sfetch_item_discard(&item);
    T(item.handle.id == 0);
    T(item.path.buf[0] == 0);
    T(item.state == _SFETCH_STATE_INITIAL);
    T(item.user.user_data_size == 0);
    T(item.user.user_data[0] == 0);
}

UTEST(sokol_fetch, item_init_path_overflow) {
    sfetch_request_t request = {
        .path = "012345678901234567890123456789012",
    };
    _sfetch_item_t item = zeroed_item;
    _sfetch_item_init(&item, _sfetch_make_id(1, 1), &request);
    T(item.path.buf[0] == 0);
}

UTEST(sokol_fetch, item_init_userdata_overflow) {
    uint8_t big_data[128] = { 0xFF };
    sfetch_request_t request = {
        .path = "hello_world.txt",
        .user_data = SFETCH_RANGE(big_data),
    };
    _sfetch_item_t item = zeroed_item;
    _sfetch_item_init(&item, _sfetch_make_id(1, 1), &request);
    T(item.user.user_data_size == 0);
    T(item.user.user_data[0] == 0);
}

UTEST(sokol_fetch, pool_init_discard) {
    sfetch_setup(&(sfetch_desc_t){0});
    _sfetch_pool_t pool = {0};
    const uint32_t num_items = 127;
    T(_sfetch_pool_init(&pool, num_items));
    T(pool.valid);
    T(pool.size == 128);
    T(pool.free_top == 127);
    T(pool.free_slots[0] == 127);
    T(pool.free_slots[1] == 126);
    T(pool.free_slots[126] == 1);
    _sfetch_pool_discard(&pool);
    T(!pool.valid);
    T(pool.free_slots == 0);
    T(pool.items == 0);
    sfetch_shutdown();
}

UTEST(sokol_fetch, pool_alloc_free) {
    sfetch_setup(&(sfetch_desc_t){0});
    uint8_t buf[32];
    _sfetch_pool_t pool = {0};
    const uint32_t num_items = 16;
    _sfetch_pool_init(&pool, num_items);
    uint32_t slot_id = _sfetch_pool_item_alloc(&pool, &(sfetch_request_t){
        .path = "hello_world.txt",
        .buffer = SFETCH_RANGE(buf),
    });
    T(slot_id == 0x00010001);
    T(pool.items[1].state == _SFETCH_STATE_ALLOCATED);
    T(pool.items[1].handle.id == slot_id);
    TSTR(pool.items[1].path.buf, "hello_world.txt");
    T(pool.items[1].buffer.ptr == buf);
    T(pool.items[1].buffer.size == sizeof(buf));
    T(pool.free_top == 15);
    _sfetch_pool_item_free(&pool, slot_id);
    T(pool.items[1].handle.id == 0);
    T(pool.items[1].state == _SFETCH_STATE_INITIAL);
    T(pool.items[1].path.buf[0] == 0);
    T(pool.items[1].buffer.ptr == 0);
    T(pool.items[1].buffer.size == 0);
    T(pool.free_top == 16);
    _sfetch_pool_discard(&pool);
    sfetch_shutdown();
}

UTEST(sokol_fetch, pool_overflow) {
    sfetch_setup(&(sfetch_desc_t){0});
    _sfetch_pool_t pool = {0};
    _sfetch_pool_init(&pool, 4);
    uint32_t id0 = _sfetch_pool_item_alloc(&pool, &(sfetch_request_t){ .path="path0" });
    uint32_t id1 = _sfetch_pool_item_alloc(&pool, &(sfetch_request_t){ .path="path1" });
    uint32_t id2 = _sfetch_pool_item_alloc(&pool, &(sfetch_request_t){ .path="path2" });
    uint32_t id3 = _sfetch_pool_item_alloc(&pool, &(sfetch_request_t){ .path="path3" });
    // next alloc should fail
    uint32_t id4 = _sfetch_pool_item_alloc(&pool, &(sfetch_request_t){ .path="path4" });
    T(id0 == 0x00010001);
    T(id1 == 0x00010002);
    T(id2 == 0x00010003);
    T(id3 == 0x00010004);
    T(id4 == 0);
    T(pool.items[1].handle.id == id0);
    T(pool.items[2].handle.id == id1);
    T(pool.items[3].handle.id == id2);
    T(pool.items[4].handle.id == id3);
    // free one item, alloc should work now
    _sfetch_pool_item_free(&pool, id0);
    uint32_t id5 = _sfetch_pool_item_alloc(&pool, &(sfetch_request_t){ .path="path5" });
    T(id5 == 0x00020001);
    T(pool.items[1].handle.id == id5);
    TSTR(pool.items[1].path.buf, "path5");
    _sfetch_pool_discard(&pool);
    sfetch_shutdown();
}

UTEST(sokol_fetch, lookup_item) {
    sfetch_setup(&(sfetch_desc_t){0});
    _sfetch_pool_t pool = {0};
    _sfetch_pool_init(&pool, 4);
    uint32_t id0 = _sfetch_pool_item_alloc(&pool, &(sfetch_request_t){ .path="path0" });
    uint32_t id1 = _sfetch_pool_item_alloc(&pool, &(sfetch_request_t){ .path="path1" });
    const _sfetch_item_t* item0 = _sfetch_pool_item_lookup(&pool, id0);
    const _sfetch_item_t* item1 = _sfetch_pool_item_lookup(&pool, id1);
    T(item0 == &pool.items[1]);
    T(item1 == &pool.items[2]);
    /* invalid handle always returns 0-ptr */
    T(0 == _sfetch_pool_item_lookup(&pool, _sfetch_make_id(0, 0)));
    /* free an item and make sure it's detected as dangling */
    _sfetch_pool_item_free(&pool, id0);
    T(0 == _sfetch_pool_item_lookup(&pool, id0));
    _sfetch_pool_discard(&pool);
    sfetch_shutdown();
}

UTEST(sokol_fetch, ring_init_discard) {
    sfetch_setup(&(sfetch_desc_t){0});
    _sfetch_ring_t ring = {0};
    const uint32_t num_slots = 4;
    T(_sfetch_ring_init(&ring, num_slots));
    T(ring.head == 0);
    T(ring.tail == 0);
    T(ring.num == (num_slots + 1));
    T(ring.buf);
    _sfetch_ring_discard(&ring);
    T(ring.head == 0);
    T(ring.tail == 0);
    T(ring.num == 0);
    T(ring.buf == 0);
    sfetch_shutdown();
}

UTEST(sokol_fetch, ring_enqueue_dequeue) {
    sfetch_setup(&(sfetch_desc_t){0});
    _sfetch_ring_t ring = {0};
    const uint32_t num_slots = 4;
    _sfetch_ring_init(&ring, num_slots);
    T(_sfetch_ring_count(&ring) == 0);
    T(_sfetch_ring_empty(&ring));
    T(!_sfetch_ring_full(&ring));
    for (uint32_t i = 0; i < num_slots; i++) {
        T(!_sfetch_ring_full(&ring));
        _sfetch_ring_enqueue(&ring, _sfetch_make_id(1, i+1));
        T(_sfetch_ring_count(&ring) == (i+1));
        T(!_sfetch_ring_empty(&ring));
    }
    T(_sfetch_ring_count(&ring) == 4);
    T(!_sfetch_ring_empty(&ring));
    T(_sfetch_ring_full(&ring));
    for (uint32_t i = 0; i < num_slots; i++) {
        T(_sfetch_ring_peek(&ring, i) == _sfetch_make_id(1, i+1));
    }
    for (uint32_t i = 0; i < num_slots; i++) {
        T(!_sfetch_ring_empty(&ring));
        const uint32_t slot_id = _sfetch_ring_dequeue(&ring);
        T(slot_id == _sfetch_make_id(1, i+1));
        T(!_sfetch_ring_full(&ring));
    }
    T(_sfetch_ring_count(&ring) == 0);
    T(_sfetch_ring_empty(&ring));
    T(!_sfetch_ring_full(&ring));
    _sfetch_ring_discard(&ring);
    sfetch_shutdown();
}

UTEST(sokol_fetch, ring_wrap_around) {
    sfetch_setup(&(sfetch_desc_t){0});
    _sfetch_ring_t ring = {0};
    _sfetch_ring_init(&ring, 4);
    uint32_t i = 0;
    for (i = 0; i < 4; i++) {
        _sfetch_ring_enqueue(&ring, _sfetch_make_id(1, i+1));
    }
    T(_sfetch_ring_full(&ring));
    for (; i < 64; i++) {
        T(_sfetch_ring_full(&ring));
        T(_sfetch_ring_dequeue(&ring) == _sfetch_make_id(1, (i - 3)));
        T(!_sfetch_ring_full(&ring));
        _sfetch_ring_enqueue(&ring, _sfetch_make_id(1, i+1));
    }
    T(_sfetch_ring_full(&ring));
    for (i = 0; i < 4; i++) {
        T(_sfetch_ring_dequeue(&ring) == _sfetch_make_id(1, (i + 61)));
    }
    T(_sfetch_ring_empty(&ring));
    _sfetch_ring_discard(&ring);
    sfetch_shutdown();
}

UTEST(sokol_fetch, ring_wrap_count) {
    sfetch_setup(&(sfetch_desc_t){0});
    _sfetch_ring_t ring = {0};
    _sfetch_ring_init(&ring, 8);
    // add and remove 4 items to move tail to the middle
    for (uint32_t i = 0; i < 4; i++) {
        _sfetch_ring_enqueue(&ring, _sfetch_make_id(1, i+1));
        _sfetch_ring_dequeue(&ring);
        T(_sfetch_ring_empty(&ring));
    }
    // add another 8 items
    for (uint32_t i = 0; i < 8; i++) {
        _sfetch_ring_enqueue(&ring, _sfetch_make_id(1, i+1));
    }
    // now test, dequeue and test...
    T(_sfetch_ring_full(&ring));
    for (uint32_t i = 0; i < 8; i++) {
        T(_sfetch_ring_count(&ring) == (8 - i));
        _sfetch_ring_dequeue(&ring);
    }
    T(_sfetch_ring_count(&ring) == 0);
    T(_sfetch_ring_empty(&ring));
    _sfetch_ring_discard(&ring);
    sfetch_shutdown();
}

/* NOTE: channel_worker is called from a thread */
static int num_processed_items = 0;
static void channel_worker(_sfetch_t* ctx, uint32_t slot_id) {
    (void)ctx;
    (void)slot_id;
    num_processed_items++;
}

UTEST(sokol_fetch, channel_init_discard) {
    sfetch_setup(&(sfetch_desc_t){0});
    num_processed_items = 0;
    _sfetch_channel_t chn = {0};
    const uint32_t num_slots = 12;
    const uint32_t num_lanes = 64;
    _sfetch_channel_init(&chn, 0, num_slots, num_lanes, channel_worker);
    T(chn.valid);
    T(_sfetch_ring_full(&chn.free_lanes));
    T(_sfetch_ring_empty(&chn.user_sent));
    T(_sfetch_ring_empty(&chn.user_incoming));
    #if !defined(__EMSCRIPTEN__)
    T(_sfetch_ring_empty(&chn.thread_incoming));
    T(_sfetch_ring_empty(&chn.thread_outgoing));
    #endif
    T(_sfetch_ring_empty(&chn.user_outgoing));
    _sfetch_channel_discard(&chn);
    T(!chn.valid);
    sfetch_shutdown();
}

/* public API functions */
UTEST(sokol_fetch, setup_shutdown) {
    sfetch_setup(&(sfetch_desc_t){0});
    T(sfetch_valid());
    // check default values
    T(sfetch_desc().max_requests == 128);
    T(sfetch_desc().num_channels == 1);
    T(sfetch_desc().num_lanes == 1);
    sfetch_shutdown();
    T(!sfetch_valid());
}

UTEST(sokol_fetch, setup_too_many_channels) {
    /* try to initialize with too many channels, this should clamp to
       SFETCH_MAX_CHANNELS
    */
    sfetch_setup(&(sfetch_desc_t){
        .num_channels = 64
    });
    T(sfetch_valid());
    T(sfetch_desc().num_channels == SFETCH_MAX_CHANNELS);
    sfetch_shutdown();
}

UTEST(sokol_fetch, max_path) {
    T(sfetch_max_path() == SFETCH_MAX_PATH);
}

UTEST(sokol_fetch, max_userdata) {
    T(sfetch_max_userdata_bytes() == (SFETCH_MAX_USERDATA_UINT64 * sizeof(uint64_t)));
}

static uint8_t fail_open_buffer[128];
static bool fail_open_passed;
static void fail_open_callback(const sfetch_response_t* response) {
    /* if opening a file fails, it will immediate switch into CLOSED state */
    if ((response->failed) && (response->error_code == SFETCH_ERROR_FILE_NOT_FOUND)) {
        fail_open_passed = true;
    }
}

UTEST(sokol_fetch, fail_open) {
    sfetch_setup(&(sfetch_desc_t){0});
    sfetch_handle_t h = sfetch_send(&(sfetch_request_t){
        .path = "non_existing_file.txt",
        .callback = fail_open_callback,
        .buffer = SFETCH_RANGE(fail_open_buffer),
    });
    fail_open_passed = false;
    int frame_count = 0;
    const int max_frames = 10000;
    while (sfetch_handle_valid(h) && (frame_count++ < max_frames)) {
        sfetch_dowork();
        sleep_ms(1);
    }
    T(frame_count < max_frames);
    T(fail_open_passed);
    sfetch_shutdown();
}

static bool load_file_fixed_buffer_passed;

// The file callback is called from the "current user thread" (the same
// thread where the sfetch_send() for this request was called). Note that you
// can call sfetch_setup/shutdown() on multiple threads, each thread will
// get its own thread-local "sokol-fetch instance" and its own set of
// IO-channel threads.
static void load_file_fixed_buffer_callback(const sfetch_response_t* response) {
    // when loading the whole file at once, the fetched state
    // is the best place to grab/process the data
    if (response->fetched) {
        if ((response->data_offset == 0) &&
            (response->data.ptr == load_file_buf) &&
            (response->data.size == combatsignal_file_size) &&
            (response->buffer.ptr == load_file_buf) &&
            (response->buffer.size == sizeof(load_file_buf)) &&
            response->finished)
        {
            load_file_fixed_buffer_passed = true;
        }
    }
}

UTEST(sokol_fetch, load_file_fixed_buffer) {
    memset(load_file_buf, 0, sizeof(load_file_buf));
    sfetch_setup(&(sfetch_desc_t){0});
    // send a load-request for a file where we know the max size upfront,
    // so we can provide a buffer right in the fetch request (otherwise
    // the buffer needs to be provided in the callback when the request
    // is in OPENED state, since only then the file size will be known).
    sfetch_handle_t h = sfetch_send(&(sfetch_request_t){
        .path = "comsi.s3m",
        .callback = load_file_fixed_buffer_callback,
        .buffer = SFETCH_RANGE(load_file_buf),
    });
    // simulate a frame-loop for as long as the request is in flight, normally
    // the sfetch_dowork() function is just called somewhere in the frame
    // to pump messages in and out of the IO threads, and invoke user-callbacks
    int frame_count = 0;
    const int max_frames = 10000;
    while (sfetch_handle_valid(h) && (frame_count++ < max_frames)) {
        sfetch_dowork();
        sleep_ms(1);
    }
    T(frame_count < max_frames);
    T(load_file_fixed_buffer_passed);
    sfetch_shutdown();
}

/* tests whether files with unknown size are processed correctly */
static bool load_file_unknown_size_opened_passed;
static bool load_file_unknown_size_fetched_passed;
static void load_file_unknown_size_callback(const sfetch_response_t* response) {
    if (response->dispatched) {
        if ((response->data_offset == 0) &&
            (response->data.ptr == 0) &&
            (response->data.size == 0) &&
            (response->buffer.ptr == 0) &&
            (response->buffer.size == 0) &&
            !response->finished)
        {
            load_file_unknown_size_opened_passed = true;
            sfetch_bind_buffer(response->handle, SFETCH_RANGE(load_file_buf));
        }
    }
    else if (response->fetched) {
        if ((response->data_offset == 0) &&
            (response->data.ptr == load_file_buf) &&
            (response->data.size == combatsignal_file_size) &&
            (response->buffer.ptr == load_file_buf) &&
            (response->buffer.size == sizeof(load_file_buf)) &&
            response->finished)
        {
            load_file_unknown_size_fetched_passed = true;
        }
    }
}

UTEST(sokol_fetch, load_file_unknown_size) {
    memset(load_file_buf, 0, sizeof(load_file_buf));
    sfetch_setup(&(sfetch_desc_t){0});
    sfetch_handle_t h = sfetch_send(&(sfetch_request_t){
        .path = "comsi.s3m",
        .callback = load_file_unknown_size_callback
    });
    int frame_count = 0;
    const int max_frames = 10000;
    while (sfetch_handle_valid(h) && (frame_count++ < max_frames)) {
        sfetch_dowork();
        sleep_ms(1);
    }
    T(frame_count < max_frames);
    T(load_file_unknown_size_opened_passed);
    T(load_file_unknown_size_fetched_passed);
    sfetch_shutdown();
}

/* tests whether not providing a buffer in OPENED properly fails */
static bool load_file_no_buffer_opened_passed;
static bool load_file_no_buffer_failed_passed;
static void load_file_no_buffer_callback(const sfetch_response_t* response) {
    if (response->dispatched) {
        if ((response->data_offset == 0) &&
            (response->data.ptr == 0) &&
            (response->data.size == 0) &&
            (response->buffer.ptr == 0) &&
            (response->buffer.size == 0) &&
            !response->finished)
        {
            /* DO NOT provide a buffer here, see if that properly fails */
            load_file_no_buffer_opened_passed = true;
        }
    }
    else if ((response->failed) && (response->error_code == SFETCH_ERROR_NO_BUFFER)) {
        if (load_file_no_buffer_opened_passed) {
            load_file_no_buffer_failed_passed = true;
        }
    }
}

UTEST(sokol_fetch, load_file_no_buffer) {
    memset(load_file_buf, 0, sizeof(load_file_buf));
    sfetch_setup(&(sfetch_desc_t){0});
    sfetch_handle_t h = sfetch_send(&(sfetch_request_t){
        .path = "comsi.s3m",
        .callback = load_file_no_buffer_callback
    });
    int frame_count = 0;
    const int max_frames = 10000;
    while (sfetch_handle_valid(h) && (frame_count++ < max_frames)) {
        sfetch_dowork();
        sleep_ms(1);
    }
    T(frame_count < max_frames);
    T(load_file_no_buffer_opened_passed);
    T(load_file_no_buffer_failed_passed);
    sfetch_shutdown();
}

static bool load_file_too_small_passed;
static uint8_t load_file_too_small_buf[8192];
static void load_file_too_small_callback(const sfetch_response_t* response) {
    if (response->failed && (response->error_code == SFETCH_ERROR_BUFFER_TOO_SMALL)) {
        load_file_too_small_passed = true;
    }
}

UTEST(sokol_fetch, load_file_too_small_buffer) {
    memset(load_file_buf, 0, sizeof(load_file_buf));
    sfetch_setup(&(sfetch_desc_t){0});
    sfetch_handle_t h = sfetch_send(&(sfetch_request_t){
        .path = "comsi.s3m",
        .callback = load_file_too_small_callback,
        .buffer = SFETCH_RANGE(load_file_too_small_buf),
    });
    int frame_count = 0;
    const int max_frames = 10000;
    while (sfetch_handle_valid(h) && (frame_count++ < max_frames)) {
        sfetch_dowork();
        sleep_ms(1);
    }
    T(frame_count < max_frames);
    T(load_file_too_small_passed);
    sfetch_shutdown();
}


/* test loading a big file via a small chunk-buffer, the callback will
   be called multiple times with the FETCHED state until the entire file
   is loaded
*/
static bool load_file_chunked_passed;
static uint8_t load_chunk_buf[8192];
static uint8_t load_file_chunked_content[500000];
static void load_file_chunked_callback(const sfetch_response_t* response) {
    if (response->fetched) {
        uint8_t* dst = &load_file_chunked_content[response->data_offset];
        const uint8_t* src = response->data.ptr;
        size_t num_bytes = response->data.size;
        memcpy(dst, src, num_bytes);
        if (response->finished) {
            load_file_chunked_passed = true;
        }
    }
}

UTEST(sokol_fetch, load_file_chunked) {
    memset(load_file_buf, 0, sizeof(load_file_buf));
    memset(load_chunk_buf, 0, sizeof(load_chunk_buf));
    memset(load_file_chunked_content, 0, sizeof(load_file_chunked_content));
    load_file_fixed_buffer_passed = false;
    sfetch_setup(&(sfetch_desc_t){0});
    // request for chunked-loading
    sfetch_handle_t h0 = sfetch_send(&(sfetch_request_t){
        .path = "comsi.s3m",
        .callback = load_file_chunked_callback,
        .buffer = SFETCH_RANGE(load_chunk_buf),
        .chunk_size = sizeof(load_chunk_buf)
    });
    // request for all-in-one loading for comparing with the chunked buffer
    sfetch_handle_t h1 = sfetch_send(&(sfetch_request_t){
        .path = "comsi.s3m",
        .callback = load_file_fixed_buffer_callback,
        .buffer = SFETCH_RANGE(load_file_buf),
    });
    int frame_count = 0;
    const int max_frames = 10000;
    while ((sfetch_handle_valid(h0) || sfetch_handle_valid(h1)) && (frame_count++ < max_frames)) {
        sfetch_dowork();
        sleep_ms(1);
    }
    T(frame_count < max_frames);
    T(load_file_chunked_passed);
    T(0 == memcmp(load_file_chunked_content, load_file_buf, combatsignal_file_size));
    sfetch_shutdown();
}

/* load N big files in small chunks interleaved on the same channel via lanes */
#define LOAD_FILE_LANES_NUM_LANES (4)

static uint8_t load_file_lanes_chunk_buf[LOAD_FILE_LANES_NUM_LANES][8192];
static uint8_t load_file_lanes_content[LOAD_FILE_LANES_NUM_LANES][500000];
static int load_file_lanes_passed[LOAD_FILE_LANES_NUM_LANES];
static void load_file_lanes_callback(const sfetch_response_t* response) {
    assert((response->channel == 0) && (response->lane < LOAD_FILE_LANES_NUM_LANES));
    if (response->fetched) {
        uint8_t* dst = &load_file_lanes_content[response->lane][response->data_offset];
        const uint8_t* src = response->data.ptr;
        size_t num_bytes = response->data.size;
        memcpy(dst, src, num_bytes);
        if (response->finished) {
            load_file_lanes_passed[response->lane]++;
        }
    }
}

UTEST(sokol_fetch, load_file_lanes) {
    for (int i = 0; i < LOAD_FILE_LANES_NUM_LANES; i++) {
        memset(load_file_lanes_content[i], i, sizeof(load_file_lanes_content[i]));
    }
    sfetch_setup(&(sfetch_desc_t){
        .num_channels = 1,
        .num_lanes = LOAD_FILE_LANES_NUM_LANES,
    });
    sfetch_handle_t h[LOAD_FILE_LANES_NUM_LANES];
    for (int lane = 0; lane < LOAD_FILE_LANES_NUM_LANES; lane++) {
        h[lane] = sfetch_send(&(sfetch_request_t){
            .path = "comsi.s3m",
            .callback = load_file_lanes_callback,
            .buffer = { .ptr = load_file_lanes_chunk_buf[lane], .size = sizeof(load_file_lanes_chunk_buf[0]) },
            .chunk_size = sizeof(load_file_lanes_chunk_buf[0])
        });
    }
    bool done = false;
    int frame_count = 0;
    const int max_frames = 10000;
    while (!done && (frame_count++ < max_frames)) {
        done = true;
        for (int i = 0; i < LOAD_FILE_LANES_NUM_LANES; i++) {
            done &= !sfetch_handle_valid(h[i]);
        }
        sfetch_dowork();
        sleep_ms(1);
    }
    T(frame_count < max_frames);
    for (int i = 0; i < LOAD_FILE_LANES_NUM_LANES; i++) {
        T(1 == load_file_lanes_passed[i]);
        T(0 == memcmp(load_file_lanes_content[0], load_file_lanes_content[i], combatsignal_file_size));
    }
    sfetch_shutdown();
}

/* same as above, but issue more requests than available lanes to test if rate-limiting works */
#define LOAD_FILE_THROTTLE_NUM_LANES (4)
#define LOAD_FILE_THROTTLE_NUM_PASSES (3)
#define LOAD_FILE_THROTTLE_NUM_REQUESTS (12)    // lanes * passes

static uint8_t load_file_throttle_chunk_buf[LOAD_FILE_THROTTLE_NUM_LANES][128000];
static uint8_t load_file_throttle_content[LOAD_FILE_THROTTLE_NUM_PASSES][LOAD_FILE_THROTTLE_NUM_LANES][500000];
static int load_file_throttle_passed[LOAD_FILE_THROTTLE_NUM_LANES];

static void load_file_throttle_callback(const sfetch_response_t* response) {
    assert((response->channel == 0) && (response->lane < LOAD_FILE_LANES_NUM_LANES));
    if (response->fetched) {
        assert(load_file_throttle_passed[response->lane] < LOAD_FILE_THROTTLE_NUM_PASSES);
        uint8_t* dst = &load_file_throttle_content[load_file_throttle_passed[response->lane]][response->lane][response->data_offset];
        const uint8_t* src = response->data.ptr;
        size_t num_bytes = response->data.size;
        memcpy(dst, src, num_bytes);
        if (response->finished) {
            load_file_throttle_passed[response->lane]++;
        }
    }
}

UTEST(sokol_fetch, load_file_throttle) {
    for (int pass = 0; pass < LOAD_FILE_THROTTLE_NUM_PASSES; pass++) {
        for (int lane = 0; lane < LOAD_FILE_THROTTLE_NUM_LANES; lane++) {
            memset(load_file_throttle_content[pass][lane], 10*pass+lane, sizeof(load_file_throttle_content[pass][lane]));
        }
    }
    sfetch_setup(&(sfetch_desc_t){
        .num_channels = 1,
        .num_lanes = LOAD_FILE_THROTTLE_NUM_LANES,
    });
    sfetch_handle_t h[LOAD_FILE_THROTTLE_NUM_REQUESTS];
    for (int i = 0; i < LOAD_FILE_THROTTLE_NUM_REQUESTS; i++) {
        h[i] = sfetch_send(&(sfetch_request_t){
            .path = "comsi.s3m",
            .callback = load_file_throttle_callback,
            .buffer = {
                .ptr = load_file_throttle_chunk_buf[i % LOAD_FILE_THROTTLE_NUM_LANES],
                .size = sizeof(load_file_throttle_chunk_buf[0]),
            },
            .chunk_size = sizeof(load_file_throttle_chunk_buf[0])
        });
        T(sfetch_handle_valid(h[i]));
    }
    bool done = false;
    int frame_count = 0;
    const int max_frames = 10000;
    while (!done && (frame_count++ < max_frames)) {
        done = true;
        for (int i = 0; i < LOAD_FILE_THROTTLE_NUM_REQUESTS; i++) {
            done &= !sfetch_handle_valid(h[i]);
        }
        sfetch_dowork();
        sleep_ms(1);
    }
    T(frame_count < max_frames);
    for (int lane = 0; lane < LOAD_FILE_THROTTLE_NUM_LANES; lane++) {
        T(LOAD_FILE_THROTTLE_NUM_PASSES == load_file_throttle_passed[lane]);
        for (int pass = 0; pass < LOAD_FILE_THROTTLE_NUM_PASSES; pass++) {
            T(0 == memcmp(load_file_throttle_content[0][0], load_file_throttle_content[pass][lane], combatsignal_file_size));
        }
    }
    sfetch_shutdown();
}

/* test parallel fetches on multiple channels */
#define LOAD_CHANNEL_NUM_CHANNELS (16)
static uint8_t load_channel_buf[LOAD_CHANNEL_NUM_CHANNELS][500000];
static bool load_channel_passed[LOAD_CHANNEL_NUM_CHANNELS];

void load_channel_callback(const sfetch_response_t* response) {
    assert(response->channel < LOAD_CHANNEL_NUM_CHANNELS);
    assert(!load_channel_passed[response->channel]);
    if (response->fetched) {
        if ((response->data.size == combatsignal_file_size) && response->finished) {
            load_channel_passed[response->channel] = true;
        }
    }
}

UTEST(sokol_fetch, load_channel) {
    for (int chn = 0; chn < LOAD_CHANNEL_NUM_CHANNELS; chn++) {
        memset(load_channel_buf[chn], chn, sizeof(load_channel_buf[chn]));
    }
    sfetch_setup(&(sfetch_desc_t){
        .num_channels = LOAD_CHANNEL_NUM_CHANNELS
    });
    sfetch_handle_t h[LOAD_CHANNEL_NUM_CHANNELS];
    for (uint32_t chn = 0; chn < LOAD_CHANNEL_NUM_CHANNELS; chn++) {
        h[chn] = sfetch_send(&(sfetch_request_t){
            .path = "comsi.s3m",
            .channel = chn,
            .callback = load_channel_callback,
            .buffer = SFETCH_RANGE(load_channel_buf[chn]),
        });
    }
    bool done = false;
    int frame_count = 0;
    const int max_frames = 100000;
    while (!done && (frame_count++ < max_frames)) {
        done = true;
        for (int i = 0; i < LOAD_CHANNEL_NUM_CHANNELS; i++) {
            done &= !sfetch_handle_valid(h[i]);
        }
        sfetch_dowork();
        sleep_ms(1);
    }
    T(frame_count < max_frames);
    for (int chn = 0; chn < LOAD_CHANNEL_NUM_CHANNELS; chn++) {
        T(load_channel_passed[chn]);
        T(0 == memcmp(load_channel_buf[0], load_channel_buf[chn], combatsignal_file_size));
    }
    sfetch_shutdown();
}

static bool load_file_cancel_passed = false;
static void load_file_cancel_callback(const sfetch_response_t* response) {
    if (response->dispatched) {
        sfetch_cancel(response->handle);
    }
    if (response->cancelled && response->finished && response->failed && (response->error_code == SFETCH_ERROR_CANCELLED)) {
        load_file_cancel_passed = true;
    }
}

UTEST(sokol_fetch, load_file_cancel) {
    sfetch_setup(&(sfetch_desc_t){
        .num_channels = 1
    });
    sfetch_handle_t h = sfetch_send(&(sfetch_request_t){
        .path = "comsi.s3m",
        .callback = load_file_cancel_callback,
    });
    int frame_count = 0;
    const int max_frames = 10000;
    while (sfetch_handle_valid(h) && (frame_count++ < max_frames)) {
        sfetch_dowork();
        sleep_ms(1);
    }
    T(frame_count < max_frames);
    T(load_file_cancel_passed);
    sfetch_shutdown();
}

static bool load_file_cancel_before_dispatch_passed = false;
static void load_file_cancel_before_dispatch_callback(const sfetch_response_t* response) {
    // cancelled, finished, failed and error code must all be set
    if (response->cancelled && response->finished && response->failed && (response->error_code == SFETCH_ERROR_CANCELLED)) {
        load_file_cancel_before_dispatch_passed = true;
    }
}

UTEST(sokol_fetch, load_file_cancel_before_dispatch) {
    sfetch_setup(&(sfetch_desc_t){
        .num_channels = 1,
    });
    sfetch_handle_t h = sfetch_send(&(sfetch_request_t){
        .path = "comsi.s3m",
        .callback = load_file_cancel_before_dispatch_callback,
    });
    sfetch_cancel(h);
    sfetch_dowork();
    T(load_file_cancel_before_dispatch_passed);
    sfetch_shutdown();
}

static bool load_file_cancel_after_dispatch_passed = false;
static void load_file_cancel_after_dispatch_callback(const sfetch_response_t* response) {
    // when cancelled, then finished, failed and error code must all be set
    if (response->cancelled && response->finished && response->failed && (response->error_code == SFETCH_ERROR_CANCELLED)) {
        load_file_cancel_after_dispatch_passed = true;
    }
}

UTEST(sokol_fetch, load_file_cancel_after_dispatch) {
    sfetch_setup(&(sfetch_desc_t){
        .num_channels = 1,
    });
    sfetch_handle_t h = sfetch_send(&(sfetch_request_t){
        .path = "comsi.s3m",
        .callback = load_file_cancel_after_dispatch_callback,
        .buffer = SFETCH_RANGE(load_file_buf),
    });
    int frame_count = 0;
    const int max_frames = 10000;
    while (sfetch_handle_valid(h) && (frame_count++ < max_frames)) {
        sfetch_dowork();
        sfetch_cancel(h);
        sleep_ms(1);
    }
    T(frame_count < max_frames);
    T(load_file_cancel_after_dispatch_passed);
    sfetch_shutdown();
}
