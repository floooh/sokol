//------------------------------------------------------------------------------
//  sokol_cmdbuf_test.c
//  For best results, run with ASAN and UBSAN.
//
//  LLM assisted!
//------------------------------------------------------------------------------
#include "sokol_gfx.h"
#include "sokol_log.h"
#define SOKOL_CMDBUF_IMPL
#include "sokol_cmdbuf.h"
#include "utest.h"

#define T(b) EXPECT_TRUE(b)
#define TFLT(f0,f1) {T(fabs((f0)-(f1))<=(0.000001));}

// little-endian read helpers for byte-level payload inspection
static int32_t read_le_i32(const uint8_t* p) {
    return (int32_t)((uint32_t)p[0]
                   | ((uint32_t)p[1] << 8)
                   | ((uint32_t)p[2] << 16)
                   | ((uint32_t)p[3] << 24));
}

static uint32_t read_le_u32(const uint8_t* p) {
    return (uint32_t)p[0]
         | ((uint32_t)p[1] << 8)
         | ((uint32_t)p[2] << 16)
         | ((uint32_t)p[3] << 24);
}

static void init(void) {
    sg_setup(&(sg_desc){ .logger = { .func = slog_func }});
    scb_setup(&(scb_desc){ .logger = { .func = slog_func }});
}

static void init_with(const scb_desc* desc) {
    sg_setup(&(sg_desc){0});
    scb_setup(desc);
}

static void shutdown(void) {
    scb_shutdown();
    sg_shutdown();
}

UTEST(sokol_cmdbuf, default_init_shutdown) {
    init();
    T(_scb.init_tag == _SCB_INIT_TAG);
    T(_scb.desc.cmdbuf_pool_size == _SCB_DEFAULT_CMDBUF_POOL_SIZE);
    T(_scb.pools.cmdbufs);
    T(_scb.pools.cmdbuf_pool.size == (_SCB_DEFAULT_CMDBUF_POOL_SIZE + 1));
    T(_scb.pools.cmdbuf_pool.free_queue);
    T(_scb.pools.cmdbuf_pool.gen_ctrs);
    shutdown();
    T(_scb.init_tag == 0);
}

static void* my_alloc(size_t size, void* user_data) {
    (void)user_data;
    return malloc(size);
}

static void my_free(void* ptr, void* user_data) {
    (void)user_data;
    free(ptr);
}

UTEST(sokol_cmdbuf, init_with_params) {
    init_with(&(scb_desc){
        .cmdbuf_pool_size = 128,
        .allocator = {
            .alloc_fn = my_alloc,
            .free_fn = my_free,
            .user_data = (void*)12345,
        },
    });
    T(_scb.init_tag == _SCB_INIT_TAG);
    T(_scb.desc.cmdbuf_pool_size == 128);
    T(_scb.pools.cmdbufs);
    T(_scb.pools.cmdbuf_pool.size == 129);
    T(_scb.pools.cmdbuf_pool.free_queue);
    T(_scb.pools.cmdbuf_pool.gen_ctrs);
    T(_scb.desc.allocator.alloc_fn == my_alloc);
    T(_scb.desc.allocator.free_fn == my_free);
    T(_scb.desc.allocator.user_data == (void*)12345);
    shutdown();
}

UTEST(sokol_cmdbuf, make_destroy_cmdbuf_defaults) {
    init();
    scb_cmdbuf cb = scb_make_cmdbuf(&(scb_cmdbuf_desc){0});
    T(cb.id != SCB_INVALID_ID);
    _scb_cmdbuf_t* cbptr = _scb_lookup_cmdbuf(cb.id);
    T(cbptr);
    T(cbptr->slot.state == SCB_RESOURCESTATE_VALID);
    T(cbptr->slot.id == cb.id);
    T(cbptr->buf);
    T(cbptr->buf == cbptr->cur);
    T((cbptr->buf + _SCB_DEFAULT_CMDBUF_SIZE) == cbptr->end);
    T(cbptr->label.buf[0] == 0);
    scb_destroy_cmdbuf(cb);
    T(cbptr->slot.id == SCB_INVALID_ID);
    shutdown();
}

UTEST(sokol_cmdbuf, make_cmdbuf_custom_size) {
    init();
    scb_cmdbuf cb = scb_make_cmdbuf(&(scb_cmdbuf_desc){ .size = 4096 });
    T(cb.id != SCB_INVALID_ID);
    _scb_cmdbuf_t* cbptr = _scb_lookup_cmdbuf(cb.id);
    T(cbptr);
    T(cbptr->slot.state == SCB_RESOURCESTATE_VALID);
    T(cbptr->buf);
    T(cbptr->buf == cbptr->cur);
    T((cbptr->buf + 4096) == cbptr->end);
    scb_destroy_cmdbuf(cb);
    shutdown();
}

UTEST(sokol_cmdbuf, make_cmdbuf_label) {
    init();
    scb_cmdbuf cb = scb_make_cmdbuf(&(scb_cmdbuf_desc){ .label = "hello" });
    T(cb.id != SCB_INVALID_ID);
    _scb_cmdbuf_t* cbptr = _scb_lookup_cmdbuf(cb.id);
    T(cbptr);
    T(0 == strcmp(cbptr->label.buf, "hello"));
    scb_destroy_cmdbuf(cb);
    shutdown();
}

UTEST(sokol_cmdbuf, make_cmdbuf_label_truncated) {
    init();
    // longer than _SCB_STRING_SIZE (32), must be truncated to fit + null-terminated
    const char* long_label = "0123456789ABCDEF0123456789ABCDEF_overflow_tail";
    scb_cmdbuf cb = scb_make_cmdbuf(&(scb_cmdbuf_desc){ .label = long_label });
    T(cb.id != SCB_INVALID_ID);
    _scb_cmdbuf_t* cbptr = _scb_lookup_cmdbuf(cb.id);
    T(cbptr);
    T(cbptr->label.buf[_SCB_STRING_SIZE - 1] == 0);
    T(0 == strncmp(cbptr->label.buf, long_label, _SCB_STRING_SIZE - 1));
    scb_destroy_cmdbuf(cb);
    shutdown();
}

UTEST(sokol_cmdbuf, cmdbuf_pool_exhausted) {
    init_with(&(scb_desc){
        .cmdbuf_pool_size = 4,
        .logger.func = slog_func,
    });
    scb_cmdbuf cbs[4];
    for (int i = 0; i < 4; i++) {
        cbs[i] = scb_make_cmdbuf(&(scb_cmdbuf_desc){0});
        T(cbs[i].id != SCB_INVALID_ID);
        T(_scb_lookup_cmdbuf(cbs[i].id));
    }
    // one more must fail
    scb_cmdbuf overflow = scb_make_cmdbuf(&(scb_cmdbuf_desc){0});
    T(overflow.id == SCB_INVALID_ID);
    T(_scb_lookup_cmdbuf(overflow.id) == 0);
    // after destroying one, a fresh alloc must succeed again
    scb_destroy_cmdbuf(cbs[2]);
    scb_cmdbuf recycled = scb_make_cmdbuf(&(scb_cmdbuf_desc){0});
    T(recycled.id != SCB_INVALID_ID);
    scb_destroy_cmdbuf(recycled);
    for (int i = 0; i < 4; i++) {
        if (i == 2) continue;
        scb_destroy_cmdbuf(cbs[i]);
    }
    shutdown();
}

UTEST(sokol_cmdbuf, destroy_invalid_handle) {
    init();
    // destroying the invalid handle must be a silent no-op
    scb_destroy_cmdbuf((scb_cmdbuf){ .id = SCB_INVALID_ID });
    T(_scb.init_tag == _SCB_INIT_TAG);
    shutdown();
}

UTEST(sokol_cmdbuf, stale_handle_after_destroy) {
    init();
    scb_cmdbuf cb = scb_make_cmdbuf(&(scb_cmdbuf_desc){0});
    T(cb.id != SCB_INVALID_ID);
    T(_scb_lookup_cmdbuf(cb.id));
    scb_destroy_cmdbuf(cb);
    // stale handle must not lookup, and a redundant destroy must not crash
    T(_scb_lookup_cmdbuf(cb.id) == 0);
    scb_destroy_cmdbuf(cb);
    shutdown();
}

UTEST(sokol_cmdbuf, generation_counter_bumps_on_reuse) {
    init_with(&(scb_desc){
        .cmdbuf_pool_size = 1,
        .logger.func = slog_func,
    });
    scb_cmdbuf a = scb_make_cmdbuf(&(scb_cmdbuf_desc){0});
    T(a.id != SCB_INVALID_ID);
    scb_destroy_cmdbuf(a);
    scb_cmdbuf b = scb_make_cmdbuf(&(scb_cmdbuf_desc){0});
    T(b.id != SCB_INVALID_ID);
    // same slot index, different generation counter → different id
    T((a.id & _SCB_SLOT_MASK) == (b.id & _SCB_SLOT_MASK));
    T(a.id != b.id);
    // the stale handle must no longer resolve
    T(_scb_lookup_cmdbuf(a.id) == 0);
    T(_scb_lookup_cmdbuf(b.id));
    scb_destroy_cmdbuf(b);
    shutdown();
}

UTEST(sokol_cmdbuf, query_cmdbuf_state) {
    init();
    scb_cmdbuf cb = scb_make_cmdbuf(&(scb_cmdbuf_desc){0});
    T(cb.id != SCB_INVALID_ID);
    T(scb_query_cmdbuf_state(cb) == SCB_RESOURCESTATE_VALID);
    scb_destroy_cmdbuf(cb);
    // stale handle → INVALID
    T(scb_query_cmdbuf_state(cb) == SCB_RESOURCESTATE_INVALID);
    // default handle → INVALID
    T(scb_query_cmdbuf_state((scb_cmdbuf){ .id = SCB_INVALID_ID }) == SCB_RESOURCESTATE_INVALID);
    shutdown();
}

UTEST(sokol_cmdbuf, query_cmdbuf_info_defaults) {
    init();
    scb_cmdbuf cb = scb_make_cmdbuf(&(scb_cmdbuf_desc){0});
    T(cb.id != SCB_INVALID_ID);
    scb_cmdbuf_info info = scb_query_cmdbuf_info(cb);
    T(info.size == _SCB_DEFAULT_CMDBUF_SIZE);
    T(info.remaining == _SCB_DEFAULT_CMDBUF_SIZE);
    T(info.overflown == false);
    scb_destroy_cmdbuf(cb);
    shutdown();
}

UTEST(sokol_cmdbuf, query_cmdbuf_info_custom_size) {
    init();
    scb_cmdbuf cb = scb_make_cmdbuf(&(scb_cmdbuf_desc){ .size = 4096 });
    T(cb.id != SCB_INVALID_ID);
    scb_cmdbuf_info info = scb_query_cmdbuf_info(cb);
    T(info.size == 4096);
    T(info.remaining == 4096);
    T(info.overflown == false);
    scb_destroy_cmdbuf(cb);
    shutdown();
}

UTEST(sokol_cmdbuf, query_cmdbuf_info_reflects_cur) {
    // no public recording API exists yet; simulate a partial record by nudging
    // the internal `cur` pointer and `overflown` flag to check that the info
    // accessor reads them back
    init();
    scb_cmdbuf cb = scb_make_cmdbuf(&(scb_cmdbuf_desc){ .size = 1024 });
    T(cb.id != SCB_INVALID_ID);
    _scb_cmdbuf_t* cbptr = _scb_lookup_cmdbuf(cb.id);
    T(cbptr);
    cbptr->cur = cbptr->buf + 128;
    scb_cmdbuf_info info = scb_query_cmdbuf_info(cb);
    T(info.size == 1024);
    T(info.remaining == (1024 - 128));
    T(info.overflown == false);
    cbptr->overflown = true;
    info = scb_query_cmdbuf_info(cb);
    T(info.overflown == true);
    scb_destroy_cmdbuf(cb);
    shutdown();
}

UTEST(sokol_cmdbuf, query_cmdbuf_info_invalid_handle) {
    init();
    // stale / default handles must return a zero-initialized info
    scb_cmdbuf_info info = scb_query_cmdbuf_info((scb_cmdbuf){ .id = SCB_INVALID_ID });
    T(info.size == 0);
    T(info.remaining == 0);
    T(info.overflown == false);
    scb_cmdbuf cb = scb_make_cmdbuf(&(scb_cmdbuf_desc){0});
    scb_destroy_cmdbuf(cb);
    info = scb_query_cmdbuf_info(cb);
    T(info.size == 0);
    T(info.remaining == 0);
    T(info.overflown == false);
    shutdown();
}

UTEST(sokol_cmdbuf, record_apply_viewport) {
    init();
    scb_cmdbuf cb = scb_make_cmdbuf(&(scb_cmdbuf_desc){0});
    T(cb.id != SCB_INVALID_ID);
    _scb_cmdbuf_t* cbptr = _scb_lookup_cmdbuf(cb.id);
    T(cbptr);
    scb_apply_viewport(cb, 10, 20, 30, 40, true);
    // 1 byte opcode + 4 * 4 byte ints + 1 byte bool = 18 bytes
    T((cbptr->cur - cbptr->buf) == 18);
    T(cbptr->buf[0] == (uint8_t)_SCB_CMD_APPLY_VIEWPORT);
    T(read_le_i32(&cbptr->buf[1]) == 10);
    T(read_le_i32(&cbptr->buf[5]) == 20);
    T(read_le_i32(&cbptr->buf[9]) == 30);
    T(read_le_i32(&cbptr->buf[13]) == 40);
    T(cbptr->buf[17] == 1);
    T(cbptr->overflown == false);
    T(cbptr->cmd_end == 0);
    scb_destroy_cmdbuf(cb);
    shutdown();
}

UTEST(sokol_cmdbuf, record_apply_viewportf_truncates) {
    // float variant truncates to int (matches sokol_gfx.h semantics)
    init();
    scb_cmdbuf cb = scb_make_cmdbuf(&(scb_cmdbuf_desc){0});
    T(cb.id != SCB_INVALID_ID);
    _scb_cmdbuf_t* cbptr = _scb_lookup_cmdbuf(cb.id);
    T(cbptr);
    scb_apply_viewportf(cb, 10.9f, 20.1f, -30.9f, 40.5f, false);
    T((cbptr->cur - cbptr->buf) == 18);
    T(cbptr->buf[0] == (uint8_t)_SCB_CMD_APPLY_VIEWPORT);
    T(read_le_i32(&cbptr->buf[1]) == 10);
    T(read_le_i32(&cbptr->buf[5]) == 20);
    T(read_le_i32(&cbptr->buf[9]) == -30);
    T(read_le_i32(&cbptr->buf[13]) == 40);
    T(cbptr->buf[17] == 0);
    scb_destroy_cmdbuf(cb);
    shutdown();
}

UTEST(sokol_cmdbuf, record_apply_scissor_rect) {
    init();
    scb_cmdbuf cb = scb_make_cmdbuf(&(scb_cmdbuf_desc){0});
    _scb_cmdbuf_t* cbptr = _scb_lookup_cmdbuf(cb.id);
    T(cbptr);
    scb_apply_scissor_rect(cb, 1, 2, 3, 4, true);
    T((cbptr->cur - cbptr->buf) == 18);
    T(cbptr->buf[0] == (uint8_t)_SCB_CMD_APPLY_SCISSOR_RECT);
    T(read_le_i32(&cbptr->buf[1]) == 1);
    T(read_le_i32(&cbptr->buf[5]) == 2);
    T(read_le_i32(&cbptr->buf[9]) == 3);
    T(read_le_i32(&cbptr->buf[13]) == 4);
    T(cbptr->buf[17] == 1);
    scb_destroy_cmdbuf(cb);
    shutdown();
}

UTEST(sokol_cmdbuf, record_apply_scissor_rectf_truncates) {
    init();
    scb_cmdbuf cb = scb_make_cmdbuf(&(scb_cmdbuf_desc){0});
    _scb_cmdbuf_t* cbptr = _scb_lookup_cmdbuf(cb.id);
    T(cbptr);
    scb_apply_scissor_rectf(cb, 1.9f, -2.1f, 3.5f, 4.99f, false);
    T((cbptr->cur - cbptr->buf) == 18);
    T(cbptr->buf[0] == (uint8_t)_SCB_CMD_APPLY_SCISSOR_RECT);
    T(read_le_i32(&cbptr->buf[1]) == 1);
    T(read_le_i32(&cbptr->buf[5]) == -2);
    T(read_le_i32(&cbptr->buf[9]) == 3);
    T(read_le_i32(&cbptr->buf[13]) == 4);
    T(cbptr->buf[17] == 0);
    scb_destroy_cmdbuf(cb);
    shutdown();
}

UTEST(sokol_cmdbuf, record_apply_pipeline) {
    init();
    scb_cmdbuf cb = scb_make_cmdbuf(&(scb_cmdbuf_desc){0});
    _scb_cmdbuf_t* cbptr = _scb_lookup_cmdbuf(cb.id);
    T(cbptr);
    scb_apply_pipeline(cb, (sg_pipeline){ .id = 0xDEADBEEF });
    // 1 byte opcode + 4 byte id = 5 bytes
    T((cbptr->cur - cbptr->buf) == 5);
    T(cbptr->buf[0] == (uint8_t)_SCB_CMD_APPLY_PIPELINE);
    T(read_le_u32(&cbptr->buf[1]) == 0xDEADBEEFu);
    scb_destroy_cmdbuf(cb);
    shutdown();
}

UTEST(sokol_cmdbuf, record_draw) {
    init();
    scb_cmdbuf cb = scb_make_cmdbuf(&(scb_cmdbuf_desc){0});
    _scb_cmdbuf_t* cbptr = _scb_lookup_cmdbuf(cb.id);
    T(cbptr);
    scb_draw(cb, 100, 200, 300);
    // 1 byte opcode + 3 * 4 byte ints = 13 bytes
    T((cbptr->cur - cbptr->buf) == 13);
    T(cbptr->buf[0] == (uint8_t)_SCB_CMD_DRAW);
    T(read_le_i32(&cbptr->buf[1]) == 100);
    T(read_le_i32(&cbptr->buf[5]) == 200);
    T(read_le_i32(&cbptr->buf[9]) == 300);
    scb_destroy_cmdbuf(cb);
    shutdown();
}

UTEST(sokol_cmdbuf, record_draw_ex) {
    init();
    scb_cmdbuf cb = scb_make_cmdbuf(&(scb_cmdbuf_desc){0});
    _scb_cmdbuf_t* cbptr = _scb_lookup_cmdbuf(cb.id);
    T(cbptr);
    scb_draw_ex(cb, 1, 2, 3, 4, 5);
    // 1 byte opcode + 5 * 4 byte ints = 21 bytes
    T((cbptr->cur - cbptr->buf) == 21);
    T(cbptr->buf[0] == (uint8_t)_SCB_CMD_DRAW_EX);
    T(read_le_i32(&cbptr->buf[1]) == 1);
    T(read_le_i32(&cbptr->buf[5]) == 2);
    T(read_le_i32(&cbptr->buf[9]) == 3);
    T(read_le_i32(&cbptr->buf[13]) == 4);
    T(read_le_i32(&cbptr->buf[17]) == 5);
    scb_destroy_cmdbuf(cb);
    shutdown();
}

UTEST(sokol_cmdbuf, record_dispatch) {
    init();
    scb_cmdbuf cb = scb_make_cmdbuf(&(scb_cmdbuf_desc){0});
    _scb_cmdbuf_t* cbptr = _scb_lookup_cmdbuf(cb.id);
    T(cbptr);
    scb_dispatch(cb, 8, 16, 32);
    // 1 byte opcode + 3 * 4 byte ints = 13 bytes
    T((cbptr->cur - cbptr->buf) == 13);
    T(cbptr->buf[0] == (uint8_t)_SCB_CMD_DISPATCH);
    T(read_le_i32(&cbptr->buf[1]) == 8);
    T(read_le_i32(&cbptr->buf[5]) == 16);
    T(read_le_i32(&cbptr->buf[9]) == 32);
    scb_destroy_cmdbuf(cb);
    shutdown();
}

UTEST(sokol_cmdbuf, record_multiple_commands) {
    init();
    scb_cmdbuf cb = scb_make_cmdbuf(&(scb_cmdbuf_desc){0});
    _scb_cmdbuf_t* cbptr = _scb_lookup_cmdbuf(cb.id);
    T(cbptr);
    scb_apply_pipeline(cb, (sg_pipeline){ .id = 42 });      // 5 bytes
    scb_apply_viewport(cb, 0, 0, 640, 480, true);           // 18 bytes
    scb_draw(cb, 0, 6, 1);                                  // 13 bytes
    // 5 + 18 + 13 = 36 bytes
    T((cbptr->cur - cbptr->buf) == 36);
    T(cbptr->overflown == false);
    T(cbptr->cmd_end == 0);
    // pipeline at offset 0
    T(cbptr->buf[0] == (uint8_t)_SCB_CMD_APPLY_PIPELINE);
    T(read_le_u32(&cbptr->buf[1]) == 42);
    // viewport at offset 5
    T(cbptr->buf[5] == (uint8_t)_SCB_CMD_APPLY_VIEWPORT);
    T(read_le_i32(&cbptr->buf[6]) == 0);
    T(read_le_i32(&cbptr->buf[10]) == 0);
    T(read_le_i32(&cbptr->buf[14]) == 640);
    T(read_le_i32(&cbptr->buf[18]) == 480);
    T(cbptr->buf[22] == 1);
    // draw at offset 23
    T(cbptr->buf[23] == (uint8_t)_SCB_CMD_DRAW);
    T(read_le_i32(&cbptr->buf[24]) == 0);
    T(read_le_i32(&cbptr->buf[28]) == 6);
    T(read_le_i32(&cbptr->buf[32]) == 1);
    // remaining reported via info
    scb_cmdbuf_info info = scb_query_cmdbuf_info(cb);
    T(info.size == _SCB_DEFAULT_CMDBUF_SIZE);
    T(info.remaining == (_SCB_DEFAULT_CMDBUF_SIZE - 36));
    T(info.overflown == false);
    scb_destroy_cmdbuf(cb);
    shutdown();
}

UTEST(sokol_cmdbuf, record_invalid_cmdbuf_noop) {
    init();
    // recording on the default/invalid handle must not crash and must be a no-op
    scb_cmdbuf invalid = { .id = SCB_INVALID_ID };
    scb_apply_viewport(invalid, 1, 2, 3, 4, true);
    scb_apply_scissor_rect(invalid, 1, 2, 3, 4, true);
    scb_apply_pipeline(invalid, (sg_pipeline){ .id = 1 });
    scb_draw(invalid, 0, 3, 1);
    scb_draw_ex(invalid, 0, 3, 1, 0, 0);
    scb_dispatch(invalid, 1, 1, 1);
    T(_scb.init_tag == _SCB_INIT_TAG);
    shutdown();
}

UTEST(sokol_cmdbuf, cmdbuf_overflow) {
    // size 32 fits one 21-byte draw_ex, but not two (42 > 32)
    init();
    scb_cmdbuf cb = scb_make_cmdbuf(&(scb_cmdbuf_desc){ .size = 32 });
    T(cb.id != SCB_INVALID_ID);
    _scb_cmdbuf_t* cbptr = _scb_lookup_cmdbuf(cb.id);
    T(cbptr);

    // first draw_ex fits — cur advances, overflown stays false
    scb_draw_ex(cb, 1, 2, 3, 4, 5);
    T((cbptr->cur - cbptr->buf) == 21);
    T(cbptr->overflown == false);
    T(cbptr->buf[0] == (uint8_t)_SCB_CMD_DRAW_EX);

    // second draw_ex overflows — cur must not advance, overflown flips to true
    scb_draw_ex(cb, 6, 7, 8, 9, 10);
    T((cbptr->cur - cbptr->buf) == 21);
    T(cbptr->overflown == true);
    // the second draw_ex payload must NOT be visible past the first one
    T(read_le_i32(&cbptr->buf[1]) == 1);

    // once overflown, subsequent recordings must silently no-op — cur frozen
    scb_apply_pipeline(cb, (sg_pipeline){ .id = 99 });
    scb_draw(cb, 0, 3, 1);
    T((cbptr->cur - cbptr->buf) == 21);
    T(cbptr->overflown == true);

    // info reflects overflow state
    scb_cmdbuf_info info = scb_query_cmdbuf_info(cb);
    T(info.size == 32);
    T(info.remaining == (32 - 21));
    T(info.overflown == true);

    scb_destroy_cmdbuf(cb);
    shutdown();
}

UTEST(sokol_cmdbuf, cmdbuf_overflow_smaller_than_one_cmd) {
    // even a single command doesn't fit — first recording must overflow
    init();
    scb_cmdbuf cb = scb_make_cmdbuf(&(scb_cmdbuf_desc){ .size = 4 });
    T(cb.id != SCB_INVALID_ID);
    _scb_cmdbuf_t* cbptr = _scb_lookup_cmdbuf(cb.id);
    T(cbptr);
    T(cbptr->overflown == false);
    scb_draw(cb, 0, 3, 1);  // needs 13, only 4 available
    T((cbptr->cur - cbptr->buf) == 0);
    T(cbptr->overflown == true);
    scb_destroy_cmdbuf(cb);
    shutdown();
}

UTEST(sokol_cmdbuf, shutdown_destroys_remaining_cmdbufs) {
    // rely on ASAN to catch a leaked cb->buf if shutdown forgets to release it
    init();
    scb_cmdbuf a = scb_make_cmdbuf(&(scb_cmdbuf_desc){0});
    scb_cmdbuf b = scb_make_cmdbuf(&(scb_cmdbuf_desc){ .size = 8192, .label = "b" });
    T(a.id != SCB_INVALID_ID);
    T(b.id != SCB_INVALID_ID);
    T(scb_query_cmdbuf_state(a) == SCB_RESOURCESTATE_VALID);
    T(scb_query_cmdbuf_state(b) == SCB_RESOURCESTATE_VALID);
    // no explicit destroy — shutdown must free the underlying buffers
    shutdown();
    T(_scb.init_tag == 0);
}
