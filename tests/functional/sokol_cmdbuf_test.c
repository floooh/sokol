//------------------------------------------------------------------------------
//  sokol_cmdbuf_test.c
//  For best results, run with ASAN and UBSAN.
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
