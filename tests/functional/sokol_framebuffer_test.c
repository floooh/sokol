//------------------------------------------------------------------------------
//  sokol_framebuffer_test.c
//  For best results, run with ASAN and UBSAN.
//
//  LLM generated!
//------------------------------------------------------------------------------
#include "sokol_gfx.h"
#include "sokol_log.h"
#define SOKOL_FRAMEBUFFER_IMPL
#include "sokol_framebuffer.h"
#include "utest.h"

#define T(b) EXPECT_TRUE(b)

static void init(void) {
    sg_setup(&(sg_desc){ .logger = { .func = slog_func }});
    sfb_setup(&(sfb_desc){ .logger = { .func = slog_func }});
}

static void init_with(const sfb_desc* desc) {
    sg_setup(&(sg_desc){0});
    sfb_setup(desc);
}

static void shutdown(void) {
    sfb_shutdown();
    sg_shutdown();
}

UTEST(sokol_framebuffer, default_init_shutdown) {
    init();
    T(_sfb.init_tag == _SFB_INIT_TAG);
    T(_sfb.desc.framebuffer_pool_size == _SFB_DEFAULT_FRAMEBUFFER_POOL_SIZE);
    T(_sfb.pools.framebuffers);
    T(_sfb.pools.framebuffer_pool.size == (_SFB_DEFAULT_FRAMEBUFFER_POOL_SIZE + 1));
    T(_sfb.pools.framebuffer_pool.free_queue);
    T(_sfb.pools.framebuffer_pool.gen_ctrs);
    shutdown();
    T(_sfb.init_tag == 0);
}

static void* my_alloc(size_t size, void* user_data) {
    (void)user_data;
    return malloc(size);
}

static void my_free(void* ptr, void* user_data) {
    (void)user_data;
    free(ptr);
}

UTEST(sokol_framebuffer, init_with_params) {
    init_with(&(sfb_desc){
        .framebuffer_pool_size = 128,
        .allocator = {
            .alloc_fn = my_alloc,
            .free_fn = my_free,
            .user_data = (void*)12345,
        },
    });
    T(_sfb.init_tag == _SFB_INIT_TAG);
    T(_sfb.desc.framebuffer_pool_size == 128);
    T(_sfb.pools.framebuffers);
    T(_sfb.pools.framebuffer_pool.size == 129);
    T(_sfb.pools.framebuffer_pool.free_queue);
    T(_sfb.pools.framebuffer_pool.gen_ctrs);
    T(_sfb.desc.allocator.alloc_fn == my_alloc);
    T(_sfb.desc.allocator.free_fn == my_free);
    T(_sfb.desc.allocator.user_data == (void*)12345);
    shutdown();
}

UTEST(sokol_framebuffer, make_destroy_framebuffer_defaults) {
    init();
    sfb_framebuffer fb = sfb_make_framebuffer(&(sfb_framebuffer_desc){
        .width = 320,
        .height = 256,
    });
    T(fb.id != SFB_INVALID_ID);
    T(sfb_query_framebuffer_state(fb) == SFB_RESOURCESTATE_VALID);
    _sfb_framebuffer_t* fbptr = _sfb_lookup_framebuffer(fb.id);
    T(fbptr);
    T(fbptr->slot.state == SFB_RESOURCESTATE_VALID);
    T(fbptr->slot.id == fb.id);
    T(fbptr->width == 320);
    T(fbptr->height == 256);
    T(fbptr->prescale == 1);
    T(fbptr->format == SFB_FORMAT_RGBA8);
    T(fbptr->cliprect.x == 0);
    T(fbptr->cliprect.y == 0);
    T(fbptr->cliprect.width == 320);
    T(fbptr->cliprect.height == 256);
    T(fbptr->rotate90 == false);
    sfb_destroy_framebuffer(fb);
    T(fbptr->slot.id == SFB_INVALID_ID);
    T(sfb_query_framebuffer_state(fb) == SFB_RESOURCESTATE_INVALID);
    shutdown();
}

UTEST(sokol_framebuffer, make_framebuffer_palette8) {
    init();
    sfb_framebuffer fb = sfb_make_framebuffer(&(sfb_framebuffer_desc){
        .width = 320,
        .height = 256,
        .format = SFB_FORMAT_PALETTE8,
        .prescale = 2,
        .rotate90 = true,
    });
    T(fb.id != SFB_INVALID_ID);
    T(sfb_query_framebuffer_state(fb) == SFB_RESOURCESTATE_VALID);
    _sfb_framebuffer_t* fbptr = _sfb_lookup_framebuffer(fb.id);
    T(fbptr);
    T(fbptr->format == SFB_FORMAT_PALETTE8);
    T(fbptr->prescale == 2);
    T(fbptr->rotate90 == true);
    T(fbptr->cliprect.width == 320);
    T(fbptr->cliprect.height == 256);
    sfb_destroy_framebuffer(fb);
    shutdown();
}

UTEST(sokol_framebuffer, make_framebuffer_cliprect) {
    init();
    sfb_framebuffer fb = sfb_make_framebuffer(&(sfb_framebuffer_desc){
        .width = 512,
        .height = 512,
        .cliprect = {
            .x = 0,
            .y = 0,
            .width = 160,
            .height = 128,
        },
    });
    T(fb.id != SFB_INVALID_ID);
    T(sfb_query_framebuffer_state(fb) == SFB_RESOURCESTATE_VALID);
    _sfb_framebuffer_t* fbptr = _sfb_lookup_framebuffer(fb.id);
    T(fbptr);
    T(fbptr->width == 512);
    T(fbptr->height == 512);
    T(fbptr->cliprect.x == 0);
    T(fbptr->cliprect.y == 0);
    T(fbptr->cliprect.width == 160);
    T(fbptr->cliprect.height == 128);
    sfb_destroy_framebuffer(fb);
    shutdown();
}

UTEST(sokol_framebuffer, make_framebuffer_invalid_width) {
    init();
    sfb_framebuffer fb = sfb_make_framebuffer(&(sfb_framebuffer_desc){
        .width = 0,
        .height = 256,
    });
    T(fb.id != SFB_INVALID_ID);
    T(sfb_query_framebuffer_state(fb) == SFB_RESOURCESTATE_FAILED);
    sfb_destroy_framebuffer(fb);
    T(sfb_query_framebuffer_state(fb) == SFB_RESOURCESTATE_INVALID);
    shutdown();
}

UTEST(sokol_framebuffer, make_framebuffer_invalid_height) {
    init();
    sfb_framebuffer fb = sfb_make_framebuffer(&(sfb_framebuffer_desc){
        .width = 320,
        .height = 0,
    });
    T(fb.id != SFB_INVALID_ID);
    T(sfb_query_framebuffer_state(fb) == SFB_RESOURCESTATE_FAILED);
    sfb_destroy_framebuffer(fb);
    shutdown();
}

UTEST(sokol_framebuffer, query_framebuffer_desc) {
    init();
    sfb_framebuffer fb = sfb_make_framebuffer(&(sfb_framebuffer_desc){
        .width = 320,
        .height = 256,
        .format = SFB_FORMAT_PALETTE8,
        .prescale = 2,
        .rotate90 = true,
        .cliprect = { .x = 16, .y = 32, .width = 200, .height = 100 },
    });
    T(fb.id != SFB_INVALID_ID);
    sfb_framebuffer_desc desc = sfb_query_framebuffer_desc(fb);
    T(desc.width == 320);
    T(desc.height == 256);
    T(desc.prescale == 2);
    T(desc.format == SFB_FORMAT_PALETTE8);
    T(desc.rotate90 == true);
    T(desc.cliprect.x == 16);
    T(desc.cliprect.y == 32);
    T(desc.cliprect.width == 200);
    T(desc.cliprect.height == 100);
    sfb_destroy_framebuffer(fb);
    shutdown();
}

UTEST(sokol_framebuffer, query_framebuffer_desc_invalid) {
    init();
    sfb_framebuffer_desc desc = sfb_query_framebuffer_desc((sfb_framebuffer){ .id = SFB_INVALID_ID });
    T(desc.width == 0);
    T(desc.height == 0);
    T(desc.prescale == 0);
    T(desc.format == _SFB_FORMAT_DEFAULT);
    shutdown();
}

UTEST(sokol_framebuffer, query_framebuffer_info) {
    init();
    sfb_framebuffer fb = sfb_make_framebuffer(&(sfb_framebuffer_desc){
        .width = 320,
        .height = 256,
        .prescale = 2,
    });
    T(fb.id != SFB_INVALID_ID);
    sfb_framebuffer_info info = sfb_query_framebuffer_info(fb);
    T(info.update.width == 320);
    T(info.update.height == 256);
    T(info.update.pixel_format == SG_PIXELFORMAT_RGBA8);
    T(info.update.image.id != SG_INVALID_ID);
    T(info.update.tex_view.id != SG_INVALID_ID);
    T(info.offscreen.width == 640);
    T(info.offscreen.height == 512);
    T(info.offscreen.pixel_format == SG_PIXELFORMAT_RGBA8);
    T(info.offscreen.image.id != SG_INVALID_ID);
    T(info.offscreen.tex_view.id != SG_INVALID_ID);
    T(info.palette.width == 256);
    T(info.palette.height == 1);
    T(info.palette.pixel_format == SG_PIXELFORMAT_RGBA8);
    // palette image is only created for SFB_FORMAT_PALETTE8
    T(info.palette.image.id == SG_INVALID_ID);
    T(info.palette.tex_view.id == SG_INVALID_ID);
    T(info.nearest_sampler.id != SG_INVALID_ID);
    T(info.linear_sampler.id != SG_INVALID_ID);
    sfb_destroy_framebuffer(fb);
    shutdown();
}

UTEST(sokol_framebuffer, query_framebuffer_info_palette8) {
    init();
    sfb_framebuffer fb = sfb_make_framebuffer(&(sfb_framebuffer_desc){
        .width = 320,
        .height = 256,
        .format = SFB_FORMAT_PALETTE8,
    });
    T(fb.id != SFB_INVALID_ID);
    sfb_framebuffer_info info = sfb_query_framebuffer_info(fb);
    T(info.update.pixel_format == SG_PIXELFORMAT_R8);
    T(info.palette.image.id != SG_INVALID_ID);
    T(info.palette.tex_view.id != SG_INVALID_ID);
    sfb_destroy_framebuffer(fb);
    shutdown();
}

UTEST(sokol_framebuffer, query_framebuffer_info_invalid) {
    init();
    sfb_framebuffer_info info = sfb_query_framebuffer_info((sfb_framebuffer){ .id = SFB_INVALID_ID });
    T(info.update.image.id == SG_INVALID_ID);
    T(info.offscreen.image.id == SG_INVALID_ID);
    T(info.palette.image.id == SG_INVALID_ID);
    shutdown();
}

UTEST(sokol_framebuffer, resize_noop) {
    init();
    sfb_framebuffer fb = sfb_make_framebuffer(&(sfb_framebuffer_desc){
        .width = 320,
        .height = 256,
    });
    T(fb.id != SFB_INVALID_ID);
    bool recreated = sfb_resize(fb, &(sfb_resize_desc){
        .width = 320,
        .height = 256,
    });
    T(!recreated);
    T(sfb_query_framebuffer_state(fb) == SFB_RESOURCESTATE_VALID);
    sfb_destroy_framebuffer(fb);
    shutdown();
}

UTEST(sokol_framebuffer, resize_changes_size) {
    init();
    sfb_framebuffer fb = sfb_make_framebuffer(&(sfb_framebuffer_desc){
        .width = 320,
        .height = 256,
    });
    T(fb.id != SFB_INVALID_ID);
    bool recreated = sfb_resize(fb, &(sfb_resize_desc){
        .width = 640,
        .height = 480,
    });
    T(recreated);
    T(sfb_query_framebuffer_state(fb) == SFB_RESOURCESTATE_VALID);
    sfb_framebuffer_desc desc = sfb_query_framebuffer_desc(fb);
    T(desc.width == 640);
    T(desc.height == 480);
    T(desc.cliprect.width == 640);
    T(desc.cliprect.height == 480);
    sfb_destroy_framebuffer(fb);
    shutdown();
}

UTEST(sokol_framebuffer, make_destroy_many_framebuffers) {
    init();
    sfb_framebuffer fbs[_SFB_DEFAULT_FRAMEBUFFER_POOL_SIZE];
    for (int i = 0; i < _SFB_DEFAULT_FRAMEBUFFER_POOL_SIZE; i++) {
        fbs[i] = sfb_make_framebuffer(&(sfb_framebuffer_desc){
            .width = 64,
            .height = 64,
        });
        T(fbs[i].id != SFB_INVALID_ID);
        T(sfb_query_framebuffer_state(fbs[i]) == SFB_RESOURCESTATE_VALID);
    }
    // one more should exhaust the pool
    sfb_framebuffer overflow = sfb_make_framebuffer(&(sfb_framebuffer_desc){
        .width = 64,
        .height = 64,
    });
    T(overflow.id == SFB_INVALID_ID);
    for (int i = 0; i < _SFB_DEFAULT_FRAMEBUFFER_POOL_SIZE; i++) {
        sfb_destroy_framebuffer(fbs[i]);
        T(sfb_query_framebuffer_state(fbs[i]) == SFB_RESOURCESTATE_INVALID);
    }
    shutdown();
}

UTEST(sokol_framebuffer, shutdown_destroys_remaining_framebuffers) {
    init();
    sfb_framebuffer fb0 = sfb_make_framebuffer(&(sfb_framebuffer_desc){ .width = 32, .height = 32 });
    sfb_framebuffer fb1 = sfb_make_framebuffer(&(sfb_framebuffer_desc){ .width = 64, .height = 64 });
    T(fb0.id != SFB_INVALID_ID);
    T(fb1.id != SFB_INVALID_ID);
    T(sfb_query_framebuffer_state(fb0) == SFB_RESOURCESTATE_VALID);
    T(sfb_query_framebuffer_state(fb1) == SFB_RESOURCESTATE_VALID);
    // don't destroy explicitly - sfb_shutdown() should clean them up
    shutdown();
}
