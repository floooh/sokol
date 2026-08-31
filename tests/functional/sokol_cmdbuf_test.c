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
