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

static uint64_t read_le_u64(const uint8_t* p) {
    return (uint64_t)read_le_u32(p) | ((uint64_t)read_le_u32(p + 4) << 32);
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
    T(cbptr->cmd_max_end == 0);
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
    T(cbptr->cmd_max_end == 0);
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

UTEST(sokol_cmdbuf, record_apply_bindings_empty) {
    // empty bindings: slot_mask == 0, no per-slot payload → 1 (opcode) + 8 (mask) = 9 bytes
    init();
    scb_cmdbuf cb = scb_make_cmdbuf(&(scb_cmdbuf_desc){0});
    _scb_cmdbuf_t* cbptr = _scb_lookup_cmdbuf(cb.id);
    T(cbptr);
    scb_apply_bindings(cb, &(sg_bindings){0});
    T((cbptr->cur - cbptr->buf) == 9);
    T(cbptr->buf[0] == (uint8_t)_SCB_CMD_APPLY_BINDINGS);
    T(read_le_u64(&cbptr->buf[1]) == 0);
    T(cbptr->overflown == false);
    T(cbptr->cmd_max_end == 0);
    scb_destroy_cmdbuf(cb);
    shutdown();
}

UTEST(sokol_cmdbuf, record_apply_bindings_vertex_buffer_only) {
    // single vertex buffer at slot 0 → mask bit 0, payload = id + offset
    init();
    scb_cmdbuf cb = scb_make_cmdbuf(&(scb_cmdbuf_desc){0});
    _scb_cmdbuf_t* cbptr = _scb_lookup_cmdbuf(cb.id);
    T(cbptr);
    scb_apply_bindings(cb, &(sg_bindings){
        .vertex_buffers[0] = { .id = 0x1111 },
        .vertex_buffer_offsets[0] = 64,
    });
    // 1 opcode + 8 mask + 4 vb id + 4 vb offset = 17 bytes
    T((cbptr->cur - cbptr->buf) == 17);
    T(cbptr->buf[0] == (uint8_t)_SCB_CMD_APPLY_BINDINGS);
    T(read_le_u64(&cbptr->buf[1]) == 0x1ULL);   // vb_start = 0
    T(read_le_u32(&cbptr->buf[9]) == 0x1111);
    T(read_le_i32(&cbptr->buf[13]) == 64);
    scb_destroy_cmdbuf(cb);
    shutdown();
}

UTEST(sokol_cmdbuf, record_apply_bindings_full_mix) {
    // vb0 + vb2 + ib + view0 + view5 + sampler0
    // slot layout: vb 0..7, ib 8, view 9..40, smp 41..52
    init();
    scb_cmdbuf cb = scb_make_cmdbuf(&(scb_cmdbuf_desc){0});
    _scb_cmdbuf_t* cbptr = _scb_lookup_cmdbuf(cb.id);
    T(cbptr);
    scb_apply_bindings(cb, &(sg_bindings){
        .vertex_buffers[0] = { .id = 0xAA },
        .vertex_buffer_offsets[0] = 4,
        .vertex_buffers[2] = { .id = 0xBB },
        .vertex_buffer_offsets[2] = 8,
        .index_buffer = { .id = 0xCC },
        .index_buffer_offset = 12,
        .views[0] = { .id = 0xD0 },
        .views[5] = { .id = 0xD5 },
        .samplers[0] = { .id = 0xE0 },
    });
    // sizes: opcode 1 + mask 8 + 2 vb (2 * 8) + ib 8 + 2 view (2 * 4) + 1 smp (1 * 4) = 45
    T((cbptr->cur - cbptr->buf) == 45);
    T(cbptr->buf[0] == (uint8_t)_SCB_CMD_APPLY_BINDINGS);
    const uint64_t expected_mask =
        (1ULL << 0) | (1ULL << 2) |     // vb 0, 2
        (1ULL << 8) |                    // ib
        (1ULL << 9) | (1ULL << 14) |     // view 0, 5 (view_start = 9)
        (1ULL << 41);                    // smp 0 (smp_start = 41)
    T(read_le_u64(&cbptr->buf[1]) == expected_mask);
    // vb0 id + offset at offset 9
    T(read_le_u32(&cbptr->buf[9]) == 0xAA);
    T(read_le_i32(&cbptr->buf[13]) == 4);
    // vb2 id + offset at offset 17
    T(read_le_u32(&cbptr->buf[17]) == 0xBB);
    T(read_le_i32(&cbptr->buf[21]) == 8);
    // ib id + offset at offset 25
    T(read_le_u32(&cbptr->buf[25]) == 0xCC);
    T(read_le_i32(&cbptr->buf[29]) == 12);
    // view 0 at offset 33, view 5 at offset 37
    T(read_le_u32(&cbptr->buf[33]) == 0xD0);
    T(read_le_u32(&cbptr->buf[37]) == 0xD5);
    // sampler 0 at offset 41
    T(read_le_u32(&cbptr->buf[41]) == 0xE0);
    T(cbptr->overflown == false);
    T(cbptr->cmd_max_end == 0);
    scb_destroy_cmdbuf(cb);
    shutdown();
}

UTEST(sokol_cmdbuf, record_apply_bindings_invalid_cmdbuf_noop) {
    init();
    scb_apply_bindings((scb_cmdbuf){ .id = SCB_INVALID_ID }, &(sg_bindings){
        .vertex_buffers[0] = { .id = 1 },
    });
    T(_scb.init_tag == _SCB_INIT_TAG);
    shutdown();
}

UTEST(sokol_cmdbuf, record_apply_uniforms) {
    // 16-byte uniform payload; the payload blob must be 16-byte aligned in the
    // recorded stream so replay can hand it to sg_apply_uniforms without a copy
    init();
    scb_cmdbuf cb = scb_make_cmdbuf(&(scb_cmdbuf_desc){0});
    _scb_cmdbuf_t* cbptr = _scb_lookup_cmdbuf(cb.id);
    T(cbptr);
    // sanity: malloc must return a 16-byte-aligned buffer for these offset
    // assertions to hold (guaranteed on 64-bit targets)
    T(((uintptr_t)cbptr->buf & 15) == 0);
    const float payload[4] = { 1.0f, 2.0f, 3.0f, 4.0f };
    scb_apply_uniforms(cb, 2, &(sg_range){ .ptr = payload, .size = sizeof(payload) });
    // 1 opcode + 4 ub_slot + 4 size = 9 bytes header, padded to 16 (7 pad
    // bytes), + 16 blob = 32 bytes total
    T((cbptr->cur - cbptr->buf) == 32);
    T(cbptr->buf[0] == (uint8_t)_SCB_CMD_APPLY_UNIFORMS);
    T(read_le_i32(&cbptr->buf[1]) == 2);
    T(read_le_u32(&cbptr->buf[5]) == sizeof(payload));
    // the padding bytes are zeroed by the encoder
    for (int i = 9; i < 16; i++) {
        T(cbptr->buf[i] == 0);
    }
    T(0 == memcmp(&cbptr->buf[16], payload, sizeof(payload)));
    // the payload address itself is 16-byte aligned
    T(((uintptr_t)&cbptr->buf[16] & 15) == 0);
    T(cbptr->overflown == false);
    T(cbptr->cmd_max_end == 0);
    scb_destroy_cmdbuf(cb);
    shutdown();
}

UTEST(sokol_cmdbuf, record_apply_uniforms_different_slot) {
    init();
    scb_cmdbuf cb = scb_make_cmdbuf(&(scb_cmdbuf_desc){0});
    _scb_cmdbuf_t* cbptr = _scb_lookup_cmdbuf(cb.id);
    T(cbptr);
    T(((uintptr_t)cbptr->buf & 15) == 0);
    const uint8_t payload[3] = { 0xAA, 0xBB, 0xCC };
    scb_apply_uniforms(cb, 5, &(sg_range){ .ptr = payload, .size = sizeof(payload) });
    // 1 + 4 + 4 = 9 bytes header, padded to 16, + 3 blob = 19 bytes
    T((cbptr->cur - cbptr->buf) == 19);
    T(cbptr->buf[0] == (uint8_t)_SCB_CMD_APPLY_UNIFORMS);
    T(read_le_i32(&cbptr->buf[1]) == 5);
    T(read_le_u32(&cbptr->buf[5]) == 3);
    // padding zeroed
    for (int i = 9; i < 16; i++) {
        T(cbptr->buf[i] == 0);
    }
    T(cbptr->buf[16] == 0xAA);
    T(cbptr->buf[17] == 0xBB);
    T(cbptr->buf[18] == 0xCC);
    T(((uintptr_t)&cbptr->buf[16] & 15) == 0);
    scb_destroy_cmdbuf(cb);
    shutdown();
}

UTEST(sokol_cmdbuf, record_apply_uniforms_alignment_after_odd_prefix) {
    // any command sequence before apply_uniforms leaves cur at an arbitrary
    // offset; the encoder must still land the payload on a 16-byte boundary
    init();
    scb_cmdbuf cb = scb_make_cmdbuf(&(scb_cmdbuf_desc){0});
    _scb_cmdbuf_t* cbptr = _scb_lookup_cmdbuf(cb.id);
    T(cbptr);
    T(((uintptr_t)cbptr->buf & 15) == 0);
    // a single apply_pipeline consumes 5 bytes → cur at buf+5, an odd offset
    scb_apply_pipeline(cb, (sg_pipeline){ .id = 0x42 });
    T((cbptr->cur - cbptr->buf) == 5);
    const float payload[4] = { 10.0f, 20.0f, 30.0f, 40.0f };
    const uint8_t* uniforms_start = cbptr->cur;
    scb_apply_uniforms(cb, 1, &(sg_range){ .ptr = payload, .size = sizeof(payload) });
    // opcode written at buf+5, header ends at buf+14 → next 16-aligned is buf+16
    T(uniforms_start[0] == (uint8_t)_SCB_CMD_APPLY_UNIFORMS);
    const uint8_t* payload_ptr = cbptr->buf + 16;
    T(((uintptr_t)payload_ptr & 15) == 0);
    T(0 == memcmp(payload_ptr, payload, sizeof(payload)));
    // total = 5 (pipeline) + 1 (opcode) + 4 (slot) + 4 (size) + 2 (pad) + 16 (blob) = 32
    T((cbptr->cur - cbptr->buf) == 32);
    // padding bytes are zeroed
    T(cbptr->buf[14] == 0);
    T(cbptr->buf[15] == 0);
    scb_destroy_cmdbuf(cb);
    shutdown();
}

UTEST(sokol_cmdbuf, record_apply_uniforms_alignment_across_offsets) {
    // sweep prefix sizes 0..15 and confirm every recorded payload is
    // 16-byte aligned regardless of where the command lands
    init();
    for (int pad_bytes = 0; pad_bytes < 16; pad_bytes++) {
        scb_cmdbuf cb = scb_make_cmdbuf(&(scb_cmdbuf_desc){0});
        _scb_cmdbuf_t* cbptr = _scb_lookup_cmdbuf(cb.id);
        T(cbptr);
        T(((uintptr_t)cbptr->buf & 15) == 0);
        // nudge cur by pad_bytes to simulate an arbitrary prior recording
        cbptr->cur = cbptr->buf + pad_bytes;
        const float payload[4] = { 1.0f, 2.0f, 3.0f, 4.0f };
        scb_apply_uniforms(cb, 0, &(sg_range){ .ptr = payload, .size = sizeof(payload) });
        // find the blob by scanning for its content — must land on 16-byte-aligned addr
        // header: opcode(1) + slot(4) + size(4) = 9 bytes past pad_bytes
        const size_t header_end = (size_t)pad_bytes + 9;
        const size_t aligned = (header_end + 15) & ~(size_t)15;
        const uint8_t* payload_ptr = cbptr->buf + aligned;
        T(((uintptr_t)payload_ptr & 15) == 0);
        T(0 == memcmp(payload_ptr, payload, sizeof(payload)));
        // header alignment pad bytes must be zero
        for (size_t i = header_end; i < aligned; i++) {
            T(cbptr->buf[i] == 0);
        }
        scb_destroy_cmdbuf(cb);
    }
    shutdown();
}

UTEST(sokol_cmdbuf, record_apply_uniforms_overflow_no_partial_write) {
    // cmdbuf too small for the payload — must set overflown, cur must not move,
    // and the source buffer must not be partially copied
    init();
    scb_cmdbuf cb = scb_make_cmdbuf(&(scb_cmdbuf_desc){ .size = 16 });
    _scb_cmdbuf_t* cbptr = _scb_lookup_cmdbuf(cb.id);
    T(cbptr);
    // max payload budget = 1 opcode + 4 slot + 4 size + 16 blob + 16 align pad = 41 > 16
    const uint8_t payload[16] = { 0 };
    scb_apply_uniforms(cb, 0, &(sg_range){ .ptr = payload, .size = sizeof(payload) });
    T((cbptr->cur - cbptr->buf) == 0);
    T(cbptr->overflown == true);
    T(cbptr->cmd_max_end == 0);
    scb_destroy_cmdbuf(cb);
    shutdown();
}

UTEST(sokol_cmdbuf, record_apply_uniforms_invalid_cmdbuf_noop) {
    init();
    const float payload[4] = { 0 };
    scb_apply_uniforms((scb_cmdbuf){ .id = SCB_INVALID_ID }, 0,
        &(sg_range){ .ptr = payload, .size = sizeof(payload) });
    T(_scb.init_tag == _SCB_INIT_TAG);
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

// ============================================================================
// scb_submit tests — use sokol-gfx trace hooks to observe the sg_* calls
// that scb_submit produces when decoding a recorded cmdbuf.
//
// Trace hooks fire BEFORE the sokol-gfx validation / pass check, so the
// captured arguments reflect exactly what scb_submit passed in, and no
// active pass or valid resources are needed.
// ============================================================================

typedef enum {
    TCALL_NONE = 0,
    TCALL_APPLY_VIEWPORT,
    TCALL_APPLY_SCISSOR_RECT,
    TCALL_APPLY_PIPELINE,
    TCALL_APPLY_BINDINGS,
    TCALL_APPLY_UNIFORMS,
    TCALL_DRAW,
    TCALL_DRAW_EX,
    TCALL_DISPATCH,
    TCALL_PUSH_DEBUG_GROUP,
    TCALL_POP_DEBUG_GROUP,
} tcall_kind;

#define TCALL_MAX_CALLS (32)
#define TCALL_MAX_UNIFORM_SIZE (256)

typedef struct {
    tcall_kind kind;
    int i0, i1, i2, i3, i4;
    bool b0;
    uint32_t u0;
    sg_bindings bindings;         // deep copy at hook time (bindings ptr is stack-local)
    int ub_slot;
    size_t uniform_size;
    uintptr_t uniform_ptr;        // raw ptr address to verify 16-byte alignment on submit
    uint8_t uniform_data[TCALL_MAX_UNIFORM_SIZE];
    char label[64];
} tcall_t;

static tcall_t tcalls[TCALL_MAX_CALLS];
static int num_tcalls = 0;

static void tcalls_reset(void) {
    num_tcalls = 0;
    memset(tcalls, 0, sizeof(tcalls));
}

static tcall_t* tcalls_push(tcall_kind kind) {
    if (num_tcalls < TCALL_MAX_CALLS) {
        tcall_t* c = &tcalls[num_tcalls++];
        c->kind = kind;
        return c;
    }
    return 0;
}

static void hook_apply_viewport(int x, int y, int w, int h, bool otl, void* ud) {
    (void)ud;
    tcall_t* c = tcalls_push(TCALL_APPLY_VIEWPORT);
    if (c) { c->i0 = x; c->i1 = y; c->i2 = w; c->i3 = h; c->b0 = otl; }
}

static void hook_apply_scissor_rect(int x, int y, int w, int h, bool otl, void* ud) {
    (void)ud;
    tcall_t* c = tcalls_push(TCALL_APPLY_SCISSOR_RECT);
    if (c) { c->i0 = x; c->i1 = y; c->i2 = w; c->i3 = h; c->b0 = otl; }
}

static void hook_apply_pipeline(sg_pipeline pip, void* ud) {
    (void)ud;
    tcall_t* c = tcalls_push(TCALL_APPLY_PIPELINE);
    if (c) { c->u0 = pip.id; }
}

static void hook_apply_bindings(const sg_bindings* bnd, void* ud) {
    (void)ud;
    tcall_t* c = tcalls_push(TCALL_APPLY_BINDINGS);
    if (c) { c->bindings = *bnd; }
}

static void hook_apply_uniforms(int ub_slot, const sg_range* data, void* ud) {
    (void)ud;
    tcall_t* c = tcalls_push(TCALL_APPLY_UNIFORMS);
    if (c) {
        c->ub_slot = ub_slot;
        c->uniform_size = data->size;
        c->uniform_ptr = (uintptr_t)data->ptr;
        SOKOL_ASSERT(data->size <= TCALL_MAX_UNIFORM_SIZE);
        memcpy(c->uniform_data, data->ptr, data->size);
    }
}

static void hook_draw(int base_element, int num_elements, int num_instances, void* ud) {
    (void)ud;
    tcall_t* c = tcalls_push(TCALL_DRAW);
    if (c) { c->i0 = base_element; c->i1 = num_elements; c->i2 = num_instances; }
}

static void hook_draw_ex(int base_element, int num_elements, int num_instances, int base_vertex, int base_instance, void* ud) {
    (void)ud;
    tcall_t* c = tcalls_push(TCALL_DRAW_EX);
    if (c) {
        c->i0 = base_element; c->i1 = num_elements; c->i2 = num_instances;
        c->i3 = base_vertex; c->i4 = base_instance;
    }
}

static void hook_dispatch(int x, int y, int z, void* ud) {
    (void)ud;
    tcall_t* c = tcalls_push(TCALL_DISPATCH);
    if (c) { c->i0 = x; c->i1 = y; c->i2 = z; }
}

static void hook_push_debug_group(const char* name, void* ud) {
    (void)ud;
    tcall_t* c = tcalls_push(TCALL_PUSH_DEBUG_GROUP);
    if (c && name) {
        strncpy(c->label, name, sizeof(c->label) - 1);
        c->label[sizeof(c->label) - 1] = 0;
    }
}

static void hook_pop_debug_group(void* ud) {
    (void)ud;
    tcalls_push(TCALL_POP_DEBUG_GROUP);
}

static void install_capture_hooks(void) {
    sg_install_trace_hooks(&(sg_trace_hooks){
        .apply_viewport = hook_apply_viewport,
        .apply_scissor_rect = hook_apply_scissor_rect,
        .apply_pipeline = hook_apply_pipeline,
        .apply_bindings = hook_apply_bindings,
        .apply_uniforms = hook_apply_uniforms,
        .draw = hook_draw,
        .draw_ex = hook_draw_ex,
        .dispatch = hook_dispatch,
        .push_debug_group = hook_push_debug_group,
        .pop_debug_group = hook_pop_debug_group,
    });
}

// scb log item capture — for verifying error paths in scb_submit
#define SCB_MAX_LOG_ITEMS (16)
static scb_log_item scb_log_items[SCB_MAX_LOG_ITEMS];
static int num_scb_log_items = 0;

static void scb_test_logger(const char* tag, uint32_t log_level, uint32_t log_item_id, const char* msg, uint32_t line_nr, const char* file, void* ud) {
    (void)tag; (void)log_level; (void)msg; (void)line_nr; (void)file; (void)ud;
    if (num_scb_log_items < SCB_MAX_LOG_ITEMS) {
        scb_log_items[num_scb_log_items++] = (scb_log_item)log_item_id;
    }
}

static void submit_test_init(void) {
    tcalls_reset();
    num_scb_log_items = 0;
    memset(scb_log_items, 0, sizeof(scb_log_items));
    sg_setup(&(sg_desc){ .logger = { .func = slog_func }});
    scb_setup(&(scb_desc){ .logger = { .func = scb_test_logger }});
    install_capture_hooks();
}

UTEST(sokol_cmdbuf, submit_empty_cmdbuf) {
    // submitting an empty cmdbuf must be a no-op (no sg calls fired)
    submit_test_init();
    scb_cmdbuf cb = scb_make_cmdbuf(&(scb_cmdbuf_desc){0});
    T(cb.id != SCB_INVALID_ID);
    _scb_cmdbuf_t* cbptr = _scb_lookup_cmdbuf(cb.id);
    T(cbptr);
    T(cbptr->cur == cbptr->buf);
    scb_submit(cb);
    T(num_tcalls == 0);
    T(num_scb_log_items == 0);
    T(cbptr->cur == cbptr->buf);
    scb_destroy_cmdbuf(cb);
    shutdown();
}

UTEST(sokol_cmdbuf, submit_all_commands_roundtrip) {
    // encode one of every command type and verify each sg call is dispatched
    // with the exact recorded arguments, in order
    submit_test_init();
    scb_cmdbuf cb = scb_make_cmdbuf(&(scb_cmdbuf_desc){0});
    _scb_cmdbuf_t* cbptr = _scb_lookup_cmdbuf(cb.id);
    T(cbptr);
    scb_apply_viewport(cb, 10, 20, 30, 40, true);
    scb_apply_scissor_rect(cb, 1, 2, 3, 4, false);
    scb_apply_pipeline(cb, (sg_pipeline){ .id = 0xC0FFEE });
    scb_apply_bindings(cb, &(sg_bindings){
        .vertex_buffers[0] = { .id = 0x1111 },
        .vertex_buffer_offsets[0] = 64,
        .index_buffer = { .id = 0x2222 },
        .index_buffer_offset = 8,
    });
    const float uniforms[4] = { 1.0f, 2.0f, 3.0f, 4.0f };
    scb_apply_uniforms(cb, 3, &(sg_range){ .ptr = uniforms, .size = sizeof(uniforms) });
    scb_draw(cb, 100, 200, 300);
    scb_draw_ex(cb, 1, 2, 3, 4, 5);
    scb_dispatch(cb, 8, 16, 32);
    T(cbptr->overflown == false);
    T(cbptr->cur > cbptr->buf);
    scb_submit(cb);
    // rewind: cur must be back at the start of the buffer
    T(cbptr->cur == cbptr->buf);
    T(cbptr->overflown == false);
    // captured trace
    T(num_tcalls == 8);
    T(tcalls[0].kind == TCALL_APPLY_VIEWPORT);
    T(tcalls[0].i0 == 10 && tcalls[0].i1 == 20 && tcalls[0].i2 == 30 && tcalls[0].i3 == 40);
    T(tcalls[0].b0 == true);
    T(tcalls[1].kind == TCALL_APPLY_SCISSOR_RECT);
    T(tcalls[1].i0 == 1 && tcalls[1].i1 == 2 && tcalls[1].i2 == 3 && tcalls[1].i3 == 4);
    T(tcalls[1].b0 == false);
    T(tcalls[2].kind == TCALL_APPLY_PIPELINE);
    T(tcalls[2].u0 == 0xC0FFEEu);
    T(tcalls[3].kind == TCALL_APPLY_BINDINGS);
    T(tcalls[3].bindings.vertex_buffers[0].id == 0x1111);
    T(tcalls[3].bindings.vertex_buffer_offsets[0] == 64);
    T(tcalls[3].bindings.index_buffer.id == 0x2222);
    T(tcalls[3].bindings.index_buffer_offset == 8);
    // unused vertex-buffer slots must decode as SG_INVALID_ID
    T(tcalls[3].bindings.vertex_buffers[1].id == SG_INVALID_ID);
    T(tcalls[3].bindings.views[0].id == SG_INVALID_ID);
    T(tcalls[3].bindings.samplers[0].id == SG_INVALID_ID);
    T(tcalls[4].kind == TCALL_APPLY_UNIFORMS);
    T(tcalls[4].ub_slot == 3);
    T(tcalls[4].uniform_size == sizeof(uniforms));
    T(0 == memcmp(tcalls[4].uniform_data, uniforms, sizeof(uniforms)));
    // the pointer handed to sg_apply_uniforms must be 16-byte aligned
    T((tcalls[4].uniform_ptr & 15) == 0);
    T(tcalls[5].kind == TCALL_DRAW);
    T(tcalls[5].i0 == 100 && tcalls[5].i1 == 200 && tcalls[5].i2 == 300);
    T(tcalls[6].kind == TCALL_DRAW_EX);
    T(tcalls[6].i0 == 1 && tcalls[6].i1 == 2 && tcalls[6].i2 == 3 && tcalls[6].i3 == 4 && tcalls[6].i4 == 5);
    T(tcalls[7].kind == TCALL_DISPATCH);
    T(tcalls[7].i0 == 8 && tcalls[7].i1 == 16 && tcalls[7].i2 == 32);
    T(num_scb_log_items == 0);
    scb_destroy_cmdbuf(cb);
    shutdown();
}

UTEST(sokol_cmdbuf, submit_apply_bindings_mask_decode) {
    // full-mix bindings — every category populated to exercise the mask-based
    // decode paths (vb + ib + view + sampler)
    submit_test_init();
    scb_cmdbuf cb = scb_make_cmdbuf(&(scb_cmdbuf_desc){0});
    _scb_cmdbuf_t* cbptr = _scb_lookup_cmdbuf(cb.id);
    T(cbptr);
    scb_apply_bindings(cb, &(sg_bindings){
        .vertex_buffers[0] = { .id = 0xAA },
        .vertex_buffer_offsets[0] = 4,
        .vertex_buffers[2] = { .id = 0xBB },
        .vertex_buffer_offsets[2] = 8,
        .index_buffer = { .id = 0xCC },
        .index_buffer_offset = 12,
        .views[0] = { .id = 0xD0 },
        .views[5] = { .id = 0xD5 },
        .samplers[0] = { .id = 0xE0 },
        .samplers[3] = { .id = 0xE3 },
    });
    scb_submit(cb);
    T(num_tcalls == 1);
    T(tcalls[0].kind == TCALL_APPLY_BINDINGS);
    T(tcalls[0].bindings.vertex_buffers[0].id == 0xAA);
    T(tcalls[0].bindings.vertex_buffer_offsets[0] == 4);
    T(tcalls[0].bindings.vertex_buffers[1].id == SG_INVALID_ID);
    T(tcalls[0].bindings.vertex_buffers[2].id == 0xBB);
    T(tcalls[0].bindings.vertex_buffer_offsets[2] == 8);
    T(tcalls[0].bindings.index_buffer.id == 0xCC);
    T(tcalls[0].bindings.index_buffer_offset == 12);
    T(tcalls[0].bindings.views[0].id == 0xD0);
    T(tcalls[0].bindings.views[1].id == SG_INVALID_ID);
    T(tcalls[0].bindings.views[5].id == 0xD5);
    T(tcalls[0].bindings.samplers[0].id == 0xE0);
    T(tcalls[0].bindings.samplers[1].id == SG_INVALID_ID);
    T(tcalls[0].bindings.samplers[3].id == 0xE3);
    T(cbptr->cur == cbptr->buf);
    scb_destroy_cmdbuf(cb);
    shutdown();
}

UTEST(sokol_cmdbuf, submit_apply_uniforms_preserves_payload) {
    // uniform blob must round-trip byte-identical for arbitrary sizes
    submit_test_init();
    scb_cmdbuf cb = scb_make_cmdbuf(&(scb_cmdbuf_desc){0});
    _scb_cmdbuf_t* cbptr = _scb_lookup_cmdbuf(cb.id);
    T(cbptr);
    uint8_t payload[73];
    for (int i = 0; i < (int)sizeof(payload); i++) {
        payload[i] = (uint8_t)(i * 7 + 3);
    }
    scb_apply_uniforms(cb, 5, &(sg_range){ .ptr = payload, .size = sizeof(payload) });
    scb_submit(cb);
    T(num_tcalls == 1);
    T(tcalls[0].kind == TCALL_APPLY_UNIFORMS);
    T(tcalls[0].ub_slot == 5);
    T(tcalls[0].uniform_size == sizeof(payload));
    T(0 == memcmp(tcalls[0].uniform_data, payload, sizeof(payload)));
    // sg_apply_uniforms receives a 16-byte-aligned pointer into the cmdbuf
    T((tcalls[0].uniform_ptr & 15) == 0);
    T(cbptr->cur == cbptr->buf);
    scb_destroy_cmdbuf(cb);
    shutdown();
}

UTEST(sokol_cmdbuf, submit_apply_uniforms_pointer_aligned_after_odd_prefix) {
    // regardless of preceding commands, the pointer passed to sg_apply_uniforms
    // at replay must be 16-byte aligned
    submit_test_init();
    scb_cmdbuf cb = scb_make_cmdbuf(&(scb_cmdbuf_desc){0});
    _scb_cmdbuf_t* cbptr = _scb_lookup_cmdbuf(cb.id);
    T(cbptr);
    // encode a handful of variable-sized commands ahead of the uniforms to
    // guarantee the uniforms opcode does not land on a 16-byte boundary
    scb_apply_pipeline(cb, (sg_pipeline){ .id = 1 });       // 5 bytes
    scb_apply_viewport(cb, 0, 0, 1, 1, true);               // 18 bytes
    scb_apply_pipeline(cb, (sg_pipeline){ .id = 2 });       // 5 bytes → cur at buf+28
    const float u0[3] = { 0.1f, 0.2f, 0.3f };
    scb_apply_uniforms(cb, 0, &(sg_range){ .ptr = u0, .size = sizeof(u0) });
    scb_apply_pipeline(cb, (sg_pipeline){ .id = 3 });       // shifts the next uniform
    const uint8_t u1[7] = { 1, 2, 3, 4, 5, 6, 7 };
    scb_apply_uniforms(cb, 1, &(sg_range){ .ptr = u1, .size = sizeof(u1) });
    scb_submit(cb);
    // capture the two apply_uniforms calls in order
    int n_uniforms = 0;
    for (int i = 0; i < num_tcalls; i++) {
        if (tcalls[i].kind == TCALL_APPLY_UNIFORMS) {
            // pointer must be 16-byte aligned
            T((tcalls[i].uniform_ptr & 15) == 0);
            n_uniforms++;
        }
    }
    T(n_uniforms == 2);
    T(cbptr->cur == cbptr->buf);
    scb_destroy_cmdbuf(cb);
    shutdown();
}

UTEST(sokol_cmdbuf, submit_rewinds_cmdbuf) {
    // record + submit → rewind; a second submit must be a no-op
    submit_test_init();
    scb_cmdbuf cb = scb_make_cmdbuf(&(scb_cmdbuf_desc){0});
    _scb_cmdbuf_t* cbptr = _scb_lookup_cmdbuf(cb.id);
    T(cbptr);
    scb_draw(cb, 0, 3, 1);
    scb_draw(cb, 1, 4, 2);
    T(cbptr->cur > cbptr->buf);
    scb_submit(cb);
    T(num_tcalls == 2);
    T(cbptr->cur == cbptr->buf);
    T(cbptr->overflown == false);
    // second submit: cmdbuf is empty, nothing to replay
    scb_submit(cb);
    T(num_tcalls == 2);
    T(cbptr->cur == cbptr->buf);
    // after rewind we can record and submit again without losing anything
    scb_dispatch(cb, 1, 2, 3);
    scb_submit(cb);
    T(num_tcalls == 3);
    T(tcalls[2].kind == TCALL_DISPATCH);
    T(tcalls[2].i0 == 1 && tcalls[2].i1 == 2 && tcalls[2].i2 == 3);
    T(cbptr->cur == cbptr->buf);
    scb_destroy_cmdbuf(cb);
    shutdown();
}

UTEST(sokol_cmdbuf, submit_invalid_handle) {
    // submit on invalid handle must log CMDBUF_NOT_VALID and fire no sg call
    submit_test_init();
    scb_submit((scb_cmdbuf){ .id = SCB_INVALID_ID });
    T(num_tcalls == 0);
    T(num_scb_log_items == 1);
    T(scb_log_items[0] == SCB_LOGITEM_CMDBUF_NOT_VALID);
    // also test with a stale handle
    scb_cmdbuf cb = scb_make_cmdbuf(&(scb_cmdbuf_desc){0});
    scb_destroy_cmdbuf(cb);
    scb_submit(cb);
    T(num_tcalls == 0);
    T(num_scb_log_items == 2);
    T(scb_log_items[1] == SCB_LOGITEM_CMDBUF_NOT_VALID);
    shutdown();
}

UTEST(sokol_cmdbuf, submit_overflown_cmdbuf) {
    // submit on an overflown cmdbuf must NOT replay the (partial) stream,
    // must log SUBMIT_CMDBUF_OVERFLOWN, and must rewind the buffer
    submit_test_init();
    scb_cmdbuf cb = scb_make_cmdbuf(&(scb_cmdbuf_desc){ .size = 32 });
    _scb_cmdbuf_t* cbptr = _scb_lookup_cmdbuf(cb.id);
    T(cbptr);
    // one draw_ex fits (21 bytes), a second one overflows the 32-byte buffer
    scb_draw_ex(cb, 1, 2, 3, 4, 5);
    scb_draw_ex(cb, 6, 7, 8, 9, 10);
    T(cbptr->overflown == true);
    T(cbptr->cur > cbptr->buf);
    // reset the log capture — the encoder already fired CMDBUF_OVERFLOW above,
    // and we only want to observe what scb_submit itself produces
    num_scb_log_items = 0;
    scb_submit(cb);
    // no sg calls dispatched, log item raised, cmdbuf rewound + overflown cleared
    T(num_tcalls == 0);
    T(num_scb_log_items == 1);
    T(scb_log_items[0] == SCB_LOGITEM_SUBMIT_CMDBUF_OVERFLOWN);
    T(cbptr->cur == cbptr->buf);
    T(cbptr->overflown == false);
    // after the rewind the cmdbuf is usable again
    scb_draw(cb, 0, 3, 1);
    scb_submit(cb);
    T(num_tcalls == 1);
    T(tcalls[0].kind == TCALL_DRAW);
    scb_destroy_cmdbuf(cb);
    shutdown();
}

UTEST(sokol_cmdbuf, submit_corrupt_opcode_terminates) {
    // corrupt the opcode byte in the recorded stream — scb_submit must
    // log SUBMIT_INVALID_COMMAND, exit the loop cleanly, and rewind
    submit_test_init();
    scb_cmdbuf cb = scb_make_cmdbuf(&(scb_cmdbuf_desc){0});
    _scb_cmdbuf_t* cbptr = _scb_lookup_cmdbuf(cb.id);
    T(cbptr);
    scb_draw(cb, 0, 3, 1);
    // poke the opcode of the recorded command with something unknown; the
    // decoder must not walk off the end and must not deadlock
    cbptr->buf[0] = 0xFF;
    scb_submit(cb);
    T(num_tcalls == 0);
    T(num_scb_log_items == 1);
    T(scb_log_items[0] == SCB_LOGITEM_SUBMIT_INVALID_COMMAND);
    T(cbptr->cur == cbptr->buf);
    scb_destroy_cmdbuf(cb);
    shutdown();
}

UTEST(sokol_cmdbuf, submit_wraps_in_debug_group_with_label) {
    // a labeled cmdbuf must bracket its replayed commands with
    // sg_push_debug_group(label) and sg_pop_debug_group()
    submit_test_init();
    scb_cmdbuf cb = scb_make_cmdbuf(&(scb_cmdbuf_desc){ .label = "my_cb" });
    T(cb.id != SCB_INVALID_ID);
    scb_draw(cb, 0, 3, 1);
    scb_submit(cb);
    T(num_tcalls == 3);
    T(tcalls[0].kind == TCALL_PUSH_DEBUG_GROUP);
    T(0 == strcmp(tcalls[0].label, "my_cb"));
    T(tcalls[1].kind == TCALL_DRAW);
    T(tcalls[2].kind == TCALL_POP_DEBUG_GROUP);
    // an unlabeled cmdbuf must NOT fire debug-group calls
    tcalls_reset();
    scb_cmdbuf cb2 = scb_make_cmdbuf(&(scb_cmdbuf_desc){0});
    scb_draw(cb2, 0, 3, 1);
    scb_submit(cb2);
    T(num_tcalls == 1);
    T(tcalls[0].kind == TCALL_DRAW);
    // a labeled but empty cmdbuf must still fire push and pop
    tcalls_reset();
    scb_cmdbuf cb3 = scb_make_cmdbuf(&(scb_cmdbuf_desc){ .label = "empty" });
    scb_submit(cb3);
    T(num_tcalls == 2);
    T(tcalls[0].kind == TCALL_PUSH_DEBUG_GROUP);
    T(0 == strcmp(tcalls[0].label, "empty"));
    T(tcalls[1].kind == TCALL_POP_DEBUG_GROUP);
    scb_destroy_cmdbuf(cb);
    scb_destroy_cmdbuf(cb2);
    scb_destroy_cmdbuf(cb3);
    shutdown();
}
