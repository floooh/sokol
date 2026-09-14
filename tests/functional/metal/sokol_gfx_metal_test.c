//------------------------------------------------------------------------------
//  LLM maintained.
//
//  sokol_gfx_metal_test.c
//  Tests for the sokol_gfx.h Metal backend, run against the Metal mock library
//  in tests/mocks/metal. The tests aim for code coverage of the _sg_mtl_*
//  functions, assertions concentrate on the observable Metal API effects.
//
//  Rough coverage map (referenced sokol_gfx.h symbol on the right):
//    - backend setup, caps, storage mode   _sg_mtl_setup_backend / _sg_mtl_init_caps
//    - resource pool and deferred release  _sg_mtl_add_resource / _sg_mtl_release_resource
//    - buffer creation and updates         _sg_mtl_create_buffer / _sg_mtl_write_buffer_*
//    - managed storage flush               _sg_mtl_commit_write_range
//    - image creation and updates          _sg_mtl_create_image / _sg_mtl_write_miplevel_data
//    - pixel format mapping                _sg_mtl_pixel_format / _sg_mtl_texture_type
//    - sampler creation                    _sg_mtl_create_sampler / _sg_mtl_address_mode / ...
//    - shader creation                     _sg_mtl_create_shader / _sg_mtl_compile_library
//    - MSL bind slot ranges                _sg_mtl_ensure_msl_bindslot_ranges
//    - render pipeline creation            _sg_mtl_create_pipeline / _sg_mtl_vertex_format / ...
//    - compute pipeline creation           _sg_mtl_create_pipeline
//    - texture views                       _sg_mtl_create_view
//    - swapchain and offscreen passes      _sg_mtl_begin_pass / _sg_mtl_begin_render_pass
//    - compute passes                      _sg_mtl_begin_compute_pass
//    - render state                        _sg_mtl_apply_viewport / _sg_mtl_apply_scissor_rect
//    - resource bindings                   _sg_mtl_apply_pipeline / _sg_mtl_apply_bindings
//    - uniform updates                     _sg_mtl_apply_uniforms / _sg_mtl_bind_uniform_buffers
//    - draws and dispatches                _sg_mtl_draw_ex / _sg_mtl_dispatch
//    - debug groups                        _sg_mtl_push_debug_group / _sg_mtl_pop_debug_group
//    - frame end and garbage collection    _sg_mtl_commit / _sg_mtl_garbage_collect
//------------------------------------------------------------------------------
#define SOKOL_IMPL
#include "metal_mock.h"
#include "sokol_gfx.h"
#include "utest.h"
#include <string.h>

#define T(b) EXPECT_TRUE(b)
// debug labels are only set in debug builds
#if defined(SOKOL_DEBUG)
#define T_LABEL(v, s) T(0 == strcmp(v, s))
#else
#define T_LABEL(v, s) do { (void)(v); (void)(s); } while (0)
#endif

// Metal vertex buffer bind slots start after the uniform- and storage-buffer slots
#define MTL_VB_SLOT(n) (23 + (n))

//== test harness ==============================================================
#define MAX_LOG_ITEMS (64)
static sg_log_item log_items[MAX_LOG_ITEMS];
static int num_log_items;

static void reset_log(void) {
    num_log_items = 0;
    memset(log_items, 0, sizeof(log_items));
}

static void capture_log(const char* tag, uint32_t log_level, uint32_t log_item_id, const char* msg, uint32_t line_nr, const char* file, void* user_data) {
    (void)tag; (void)log_level; (void)msg; (void)line_nr; (void)file; (void)user_data;
    if (num_log_items < MAX_LOG_ITEMS) {
        log_items[num_log_items++] = (sg_log_item)log_item_id;
    }
}

static bool logged(sg_log_item item) {
    for (int i = 0; i < num_log_items; i++) {
        if (log_items[i] == item) {
            return true;
        }
    }
    return false;
}

typedef struct {
    bool non_apple_gpu;             // let supportsFamily:MTLGPUFamilyApple1 return NO
    bool no_border_color;           // let all border-color families return NO
    bool force_managed_storage_mode;
    bool disable_validation;
    bool retained_cmd_buffer;
} setup_desc_t;

static void setup_with(setup_desc_t s) {
    reset_log();
    metal_mock_setup();
    if (s.non_apple_gpu) {
        metal_mock_set_supports_family(MTLGPUFamilyApple1, false);
    }
    if (s.no_border_color) {
        metal_mock_set_supports_family(MTLGPUFamilyApple7, false);
        metal_mock_set_supports_family(MTLGPUFamilyMac2, false);
        metal_mock_set_supports_family(MTLGPUFamilyMetal3, false);
    }
    sg_setup(&(sg_desc){
        .environment.metal.device = metal_mock_device(),
        .metal = {
            .force_managed_storage_mode = s.force_managed_storage_mode,
            .use_command_buffer_with_retained_references = s.retained_cmd_buffer,
        },
        .disable_validation = s.disable_validation,
        .logger.func = capture_log,
    });
}

static void setup(void) {
    setup_with((setup_desc_t){0});
}

// teardown() is a macro so the call-log check can report through utest
#define teardown() teardown_impl(utest_result)
static void teardown_impl(int* utest_result) {
    // completion handlers of uncommitted command buffers must run, otherwise
    // sg_shutdown() blocks forever on the inflight-frame semaphore
    metal_mock_complete_pending();
    sg_shutdown();
    // a truncated call log makes the call counts of a test meaningless
    T(!metal_mock_call_log_overflow());
    // sokol-gfx must return the device with the refcount it got it with,
    // the mock owns the only other reference
    T(metal_mock_retain_count(metal_mock_device()) == 1);
    metal_mock_drain_pool();
    metal_mock_shutdown();
}

//== helpers ===================================================================
static uint8_t scratch[64 * 64 * 16];

typedef struct {
    int width, height, sample_count;
    sg_pixel_format depth_format;
    const void* drawable;
    const void* depth_tex;
    const void* msaa_tex;
} swapchain_t;

static swapchain_t make_swapchain(int w, int h, int sample_count, bool with_depth) {
    swapchain_t sc = {
        .width = w,
        .height = h,
        .sample_count = sample_count,
        .depth_format = with_depth ? SG_PIXELFORMAT_DEPTH_STENCIL : SG_PIXELFORMAT_NONE,
        .drawable = metal_mock_create_drawable(w, h, MTLPixelFormatBGRA8Unorm),
    };
    if (with_depth) {
        sc.depth_tex = metal_mock_create_texture(w, h, MTLPixelFormatDepth32Float_Stencil8, sample_count);
    }
    if (sample_count > 1) {
        sc.msaa_tex = metal_mock_create_texture(w, h, MTLPixelFormatBGRA8Unorm, sample_count);
    }
    return sc;
}

static void discard_swapchain(swapchain_t* sc) {
    if (sc->drawable) { metal_mock_release(sc->drawable); }
    if (sc->depth_tex) { metal_mock_release(sc->depth_tex); }
    if (sc->msaa_tex) { metal_mock_release(sc->msaa_tex); }
    memset(sc, 0, sizeof(*sc));
}

static sg_swapchain swapchain(const swapchain_t* sc) {
    return (sg_swapchain){
        .width = sc->width,
        .height = sc->height,
        .sample_count = sc->sample_count,
        .color_format = SG_PIXELFORMAT_BGRA8,
        .depth_format = sc->depth_format,
        .metal = {
            .current_drawable = sc->drawable,
            .depth_stencil_texture = sc->depth_tex,
            .msaa_color_texture = sc->msaa_tex,
        },
    };
}

static sg_shader make_shader(void) {
    return sg_make_shader(&(sg_shader_desc){
        .vertex_func.source = "vs_source",
        .fragment_func.source = "fs_source",
        .label = "shd",
    });
}

static sg_shader make_compute_shader(void) {
    return sg_make_shader(&(sg_shader_desc){
        .compute_func.source = "cs_source",
        .mtl_threads_per_threadgroup = { .x = 32, .y = 1, .z = 1 },
        .label = "cs",
    });
}

// minimal pipeline matching the default swapchain (BGRA8 + depth-stencil)
static sg_pipeline make_pipeline(sg_shader shd) {
    return sg_make_pipeline(&(sg_pipeline_desc){
        .shader = shd,
        .layout.attrs[0].format = SG_VERTEXFORMAT_FLOAT3,
        .depth.pixel_format = SG_PIXELFORMAT_DEPTH_STENCIL,
        .colors[0].pixel_format = SG_PIXELFORMAT_BGRA8,
        .label = "pip",
    });
}

static sg_buffer make_vbuf(void) {
    return sg_make_buffer(&(sg_buffer_desc){
        .usage = { .vertex_buffer = true, .immutable = true },
        .data = { .ptr = scratch, .size = 128 },
        .label = "vbuf",
    });
}

static sg_image make_2d_image(void) {
    return sg_make_image(&(sg_image_desc){
        .width = 16,
        .height = 16,
        .pixel_format = SG_PIXELFORMAT_RGBA8,
        .usage.immutable = true,
        .data.mip_levels[0] = { .ptr = scratch, .size = 16 * 16 * 4 },
        .label = "img",
    });
}

static sg_view make_texture_view(sg_image img) {
    return sg_make_view(&(sg_view_desc){ .texture.image = img, .label = "texview" });
}

//== backend setup, caps and storage mode ======================================
UTEST(sokol_gfx_metal, setup_shutdown) {
    setup();
    T(sg_isvalid());
    T(sg_query_backend() == SG_BACKEND_METAL_MACOS);
    // one command queue and SG_NUM_INFLIGHT_FRAMES uniform buffers
    T(metal_mock_count_calls(METAL_MOCK_FUNC_newCommandQueue) == 1);
    T(metal_mock_count_calls(METAL_MOCK_FUNC_newBufferWithLength) == SG_NUM_INFLIGHT_FRAMES);
    T(metal_mock_num_created(METAL_MOCK_OBJ_COMMAND_QUEUE) == 1);
    teardown();
    T(!sg_isvalid());
}

UTEST(sokol_gfx_metal, init_caps) {
    setup();
    const sg_features f = sg_query_features();
    T(f.origin_top_left);
    T(f.mrt_independent_blend_state);
    T(f.mrt_independent_write_mask);
    T(f.compute);
    T(f.msaa_texture_bindings);
    T(f.draw_base_vertex);
    T(f.draw_base_instance);
    T(f.dual_source_blending);
    T(f.vertexformat_int10_n2);
    T(f.image_clamp_to_border);
    const sg_limits l = sg_query_limits();
    T(l.max_image_size_2d == 16384);
    T(l.max_image_size_cube == 16384);
    T(l.max_image_size_3d == 2048);
    T(l.max_image_array_layers == 2048);
    T(l.max_color_attachments == SG_MAX_COLOR_ATTACHMENTS);
    T(l.max_vertex_attrs == SG_MAX_VERTEX_ATTRIBUTES);
    T(sg_query_pixelformat(SG_PIXELFORMAT_BGRA8).render);
    T(sg_query_pixelformat(SG_PIXELFORMAT_DEPTH_STENCIL).depth);
    teardown();
}

UTEST(sokol_gfx_metal, init_caps_no_border_color) {
    setup_with((setup_desc_t){ .no_border_color = true });
    T(!sg_query_features().image_clamp_to_border);
    teardown();
}

UTEST(sokol_gfx_metal, shared_storage_mode) {
    setup();
    sg_buffer buf = sg_make_buffer(&(sg_buffer_desc){
        .usage = { .vertex_buffer = true, .immutable = true },
        .data = { .ptr = scratch, .size = 64 },
    });
    T(sg_query_buffer_state(buf) == SG_RESOURCESTATE_VALID);
    metal_mock_buffer_info_t info = {0};
    T(metal_mock_buffer_info(sg_mtl_query_buffer_info(buf).buf[0], &info));
    T((info.options & MTLResourceStorageModeShared) == MTLResourceStorageModeShared);
    teardown();
}

UTEST(sokol_gfx_metal, managed_storage_mode) {
    setup_with((setup_desc_t){ .non_apple_gpu = true });
    T(metal_mock_count_calls(METAL_MOCK_FUNC_supportsFamily) > 0);
    sg_buffer buf = sg_make_buffer(&(sg_buffer_desc){
        .usage = { .vertex_buffer = true, .dynamic_update = true },
        .size = 64,
    });
    metal_mock_buffer_info_t info = {0};
    T(metal_mock_buffer_info(sg_mtl_query_buffer_info(buf).buf[0], &info));
    T((info.options & MTLResourceStorageModeManaged) == MTLResourceStorageModeManaged);
    // updating a managed buffer must flush the modified range, the update goes
    // into the next inflight slot
    sg_update_buffer(buf, &(sg_range){ .ptr = scratch, .size = 32 });
    T(metal_mock_buffer_info(sg_mtl_query_buffer_info(buf).buf[1], &info));
    T(info.num_did_modify_range == 1);
    T(info.last_modified_offset == 0);
    T(info.last_modified_length == 32);
    teardown();
}

UTEST(sokol_gfx_metal, force_managed_storage_mode) {
    setup_with((setup_desc_t){ .force_managed_storage_mode = true });
    sg_buffer buf = sg_make_buffer(&(sg_buffer_desc){
        .usage = { .vertex_buffer = true, .dynamic_update = true },
        .size = 64,
    });
    metal_mock_buffer_info_t info = {0};
    T(metal_mock_buffer_info(sg_mtl_query_buffer_info(buf).buf[0], &info));
    T((info.options & MTLResourceStorageModeManaged) == MTLResourceStorageModeManaged);
    teardown();
}

//== buffers ===================================================================
UTEST(sokol_gfx_metal, create_buffer_with_data) {
    setup();
    sg_buffer buf = sg_make_buffer(&(sg_buffer_desc){
        .usage = { .vertex_buffer = true, .immutable = true },
        .data = { .ptr = scratch, .size = 128 },
        .label = "immutable-buf",
    });
    T(sg_query_buffer_state(buf) == SG_RESOURCESTATE_VALID);
    T(metal_mock_count_calls(METAL_MOCK_FUNC_newBufferWithBytes) == 1);
    metal_mock_buffer_info_t info = {0};
    T(metal_mock_buffer_info(sg_mtl_query_buffer_info(buf).buf[0], &info));
    T(info.with_bytes);
    T(info.length == 128);
    T_LABEL(info.label, "immutable-buf.0");
    teardown();
}

UTEST(sokol_gfx_metal, create_buffer_without_data) {
    setup();
    const int num_before = metal_mock_count_calls(METAL_MOCK_FUNC_newBufferWithLength);
    sg_buffer buf = sg_make_buffer(&(sg_buffer_desc){
        .usage = { .vertex_buffer = true, .dynamic_update = true },
        .size = 256,
    });
    T(sg_query_buffer_state(buf) == SG_RESOURCESTATE_VALID);
    // double-buffered: one MTLBuffer per inflight frame
    T(metal_mock_count_calls(METAL_MOCK_FUNC_newBufferWithLength) > num_before);
    metal_mock_buffer_info_t info = {0};
    T(metal_mock_buffer_info(sg_mtl_query_buffer_info(buf).buf[0], &info));
    T(!info.with_bytes);
    T(info.length == 256);
    T((info.options & MTLResourceCPUCacheModeWriteCombined) == MTLResourceCPUCacheModeWriteCombined);
    teardown();
}

UTEST(sokol_gfx_metal, create_buffer_injected) {
    setup();
    sg_buffer src = sg_make_buffer(&(sg_buffer_desc){
        .usage = { .vertex_buffer = true, .immutable = true },
        .data = { .ptr = scratch, .size = 64 },
    });
    const void* mtl_buf = sg_mtl_query_buffer_info(src).buf[0];
    const int rc = metal_mock_retain_count(mtl_buf);
    metal_mock_clear_calls();
    sg_buffer buf = sg_make_buffer(&(sg_buffer_desc){
        .usage = { .vertex_buffer = true, .immutable = true },
        .size = 64,
        .mtl_buffers[0] = mtl_buf,
    });
    T(sg_query_buffer_state(buf) == SG_RESOURCESTATE_VALID);
    T(sg_mtl_query_buffer_info(buf).buf[0] == mtl_buf);
    // sokol-gfx must retain the injected buffer instead of taking the reference
    // of the caller
    T(metal_mock_retain_count(mtl_buf) == (rc + 1));
    // no new MTLBuffer is created for the injected slot
    T(metal_mock_count_calls(METAL_MOCK_FUNC_newBufferWithBytes) == 0);
    sg_destroy_buffer(buf);
    teardown();
}

UTEST(sokol_gfx_metal, create_image_injected) {
    setup();
    const void* mtl_tex = metal_mock_create_texture(16, 16, MTLPixelFormatRGBA8Unorm, 1);
    metal_mock_clear_calls();
    sg_image img = sg_make_image(&(sg_image_desc){
        .width = 16,
        .height = 16,
        .pixel_format = SG_PIXELFORMAT_RGBA8,
        .usage.immutable = true,
        .mtl_textures[0] = mtl_tex,
    });
    T(sg_query_image_state(img) == SG_RESOURCESTATE_VALID);
    // sokol-gfx must retain the injected texture instead of taking the
    // reference of the caller, so the test can drop its own reference
    T(metal_mock_retain_count(mtl_tex) == 2);
    metal_mock_release(mtl_tex);
    T(sg_mtl_query_image_info(img).tex[0] == mtl_tex);
    T(metal_mock_count_calls(METAL_MOCK_FUNC_newTextureWithDescriptor) == 0);
    sg_destroy_image(img);
    teardown();
}

UTEST(sokol_gfx_metal, create_sampler_injected) {
    setup();
    sg_sampler src = sg_make_sampler(&(sg_sampler_desc){0});
    const void* mtl_smp = sg_mtl_query_sampler_info(src).smp;
    const int rc = metal_mock_retain_count(mtl_smp);
    metal_mock_clear_calls();
    sg_sampler smp = sg_make_sampler(&(sg_sampler_desc){ .mtl_sampler = mtl_smp });
    T(sg_query_sampler_state(smp) == SG_RESOURCESTATE_VALID);
    T(sg_mtl_query_sampler_info(smp).smp == mtl_smp);
    // sokol-gfx must retain the injected sampler instead of taking the
    // reference of the caller
    T(metal_mock_retain_count(mtl_smp) == (rc + 1));
    T(metal_mock_count_calls(METAL_MOCK_FUNC_newSamplerStateWithDescriptor) == 0);
    sg_destroy_sampler(smp);
    teardown();
}

UTEST(sokol_gfx_metal, create_buffer_failed) {
    setup();
    metal_mock_fail_next(METAL_MOCK_OBJ_BUFFER, 1);
    sg_buffer buf = sg_make_buffer(&(sg_buffer_desc){
        .usage = { .vertex_buffer = true, .immutable = true },
        .data = { .ptr = scratch, .size = 64 },
    });
    T(sg_query_buffer_state(buf) == SG_RESOURCESTATE_FAILED);
    T(logged(SG_LOGITEM_METAL_CREATE_BUFFER_FAILED));
    teardown();
}

UTEST(sokol_gfx_metal, update_and_append_buffer) {
    setup();
    sg_buffer buf = sg_make_buffer(&(sg_buffer_desc){
        .usage = { .vertex_buffer = true, .dynamic_update = true },
        .size = 256,
    });
    metal_mock_clear_calls();
    sg_update_buffer(buf, &(sg_range){ .ptr = scratch, .size = 64 });
    T(metal_mock_count_calls(METAL_MOCK_FUNC_contents) == 1);
    // append into the *other* inflight slot after a frame boundary is not needed,
    // appends within the same frame accumulate in the current slot
    sg_buffer abuf = sg_make_buffer(&(sg_buffer_desc){
        .usage = { .vertex_buffer = true, .dynamic_update = true },
        .size = 256,
    });
    T(sg_append_buffer(abuf, &(sg_range){ .ptr = scratch, .size = 32 }) == 0);
    T(sg_append_buffer(abuf, &(sg_range){ .ptr = scratch, .size = 32 }) == 32);
    T(!sg_query_buffer_overflow(abuf));
    teardown();
}

UTEST(sokol_gfx_metal, write_buffer_transient) {
    setup();
    sg_buffer buf = sg_make_buffer(&(sg_buffer_desc){
        .usage = { .vertex_buffer = true, .write_transient = true },
        .size = 256,
    });
    metal_mock_clear_calls();
    sg_write_buffer_transient(&(sg_write_buffer_desc){
        .src.data = { .ptr = scratch, .size = 64 },
        .dst = { .buffer = buf, .offset = 0 },
        .size = 64,
    });
    T(metal_mock_count_calls(METAL_MOCK_FUNC_contents) == 1);
    teardown();
}

UTEST(sokol_gfx_metal, write_buffer_unsealed) {
    setup_with((setup_desc_t){ .non_apple_gpu = true });
    sg_buffer buf = sg_make_buffer(&(sg_buffer_desc){
        .usage = { .vertex_buffer = true, .write_unsealed = true },
        .size = 256,
    });
    const void* mtl_buf = sg_mtl_query_buffer_info(buf).buf[0];
    metal_mock_clear_calls();
    sg_write_buffer_unsealed(&(sg_write_buffer_desc){
        .src.data = { .ptr = scratch, .size = 64 },
        .dst = { .buffer = buf, .offset = 128 },
        .size = 64,
    });
    metal_mock_buffer_info_t info = {0};
    T(metal_mock_buffer_info(mtl_buf, &info));
    T(info.num_did_modify_range == 0);
    // sealing flushes the recorded dirty range
    sg_seal_buffer(buf);
    T(sg_query_buffer_state(buf) == SG_RESOURCESTATE_VALID);
    T(metal_mock_buffer_info(mtl_buf, &info));
    T(info.num_did_modify_range == 1);
    // the flushed range must cover the written range. It is intentionally not
    // checked for an exact match: the unsealed path leaves write_range.start at
    // 0, so the flush is a superset of the written range. That is correct, just
    // more work for managed storage, so don't tighten this check.
    T(info.last_modified_offset <= 128);
    T((info.last_modified_offset + info.last_modified_length) >= 192);
    teardown();
}

UTEST(sokol_gfx_metal, discard_buffer_deferred_release) {
    setup();
    sg_buffer buf = make_vbuf();
    T(metal_mock_live_objects(METAL_MOCK_OBJ_BUFFER) > SG_NUM_INFLIGHT_FRAMES);
    sg_destroy_buffer(buf);
    teardown();
    // everything is force-collected in _sg_mtl_discard_backend
    T(metal_mock_live_objects(METAL_MOCK_OBJ_BUFFER) == 0);
}

UTEST(sokol_gfx_metal, storage_buffer) {
    setup();
    sg_buffer buf = sg_make_buffer(&(sg_buffer_desc){
        .usage = { .storage_buffer = true, .immutable = true },
        .data = { .ptr = scratch, .size = 256 },
    });
    T(sg_query_buffer_state(buf) == SG_RESOURCESTATE_VALID);
    sg_view view = sg_make_view(&(sg_view_desc){ .storage_buffer.buffer = buf });
    T(sg_query_view_state(view) == SG_RESOURCESTATE_VALID);
    teardown();
}

//== images ====================================================================
UTEST(sokol_gfx_metal, create_image_2d) {
    setup();
    sg_image img = sg_make_image(&(sg_image_desc){
        .width = 16,
        .height = 16,
        .num_mipmaps = 2,
        .pixel_format = SG_PIXELFORMAT_RGBA8,
        .usage.immutable = true,
        .data.mip_levels[0] = { .ptr = scratch, .size = 16 * 16 * 4 },
        .data.mip_levels[1] = { .ptr = scratch, .size = 8 * 8 * 4 },
        .label = "img2d",
    });
    T(sg_query_image_state(img) == SG_RESOURCESTATE_VALID);
    metal_mock_texture_info_t info = {0};
    T(metal_mock_texture_info(sg_mtl_query_image_info(img).tex[0], &info));
    T(info.texture_type == MTLTextureType2D);
    T(info.pixel_format == MTLPixelFormatRGBA8Unorm);
    T(info.width == 16);
    T(info.height == 16);
    T(info.mipmap_level_count == 2);
    T(info.usage & MTLTextureUsageShaderRead);
    T(info.num_replace_region == 2);
    T_LABEL(info.label, "img2d.0");
    teardown();
}

UTEST(sokol_gfx_metal, create_image_3d) {
    setup();
    sg_image img = sg_make_image(&(sg_image_desc){
        .type = SG_IMAGETYPE_3D,
        .width = 8,
        .height = 8,
        .num_slices = 4,
        .pixel_format = SG_PIXELFORMAT_RGBA8,
        .usage.immutable = true,
        .data.mip_levels[0] = { .ptr = scratch, .size = 8 * 8 * 4 * 4 },
    });
    T(sg_query_image_state(img) == SG_RESOURCESTATE_VALID);
    metal_mock_texture_info_t info = {0};
    T(metal_mock_texture_info(sg_mtl_query_image_info(img).tex[0], &info));
    T(info.texture_type == MTLTextureType3D);
    T(info.depth == 4);
    // 3D images upload all slices in a single replaceRegion with bytesPerImage set
    T(info.num_replace_region == 1);
    const metal_mock_call_t* call = metal_mock_last_call(METAL_MOCK_FUNC_replaceRegion);
    T(call != 0);
    T(call->args[7].u == 8 * 8 * 4);
    teardown();
}

UTEST(sokol_gfx_metal, create_image_cube) {
    setup();
    sg_image img = sg_make_image(&(sg_image_desc){
        .type = SG_IMAGETYPE_CUBE,
        .width = 8,
        .height = 8,
        .pixel_format = SG_PIXELFORMAT_RGBA8,
        .usage.immutable = true,
        .data.mip_levels[0] = { .ptr = scratch, .size = 6 * 8 * 8 * 4 },
    });
    T(sg_query_image_state(img) == SG_RESOURCESTATE_VALID);
    metal_mock_texture_info_t info = {0};
    T(metal_mock_texture_info(sg_mtl_query_image_info(img).tex[0], &info));
    T(info.texture_type == MTLTextureTypeCube);
    T(info.num_replace_region == 6);
    teardown();
}

UTEST(sokol_gfx_metal, create_image_array) {
    setup();
    sg_image img = sg_make_image(&(sg_image_desc){
        .type = SG_IMAGETYPE_ARRAY,
        .width = 8,
        .height = 8,
        .num_slices = 3,
        .pixel_format = SG_PIXELFORMAT_RGBA8,
        .usage.immutable = true,
        .data.mip_levels[0] = { .ptr = scratch, .size = 3 * 8 * 8 * 4 },
    });
    T(sg_query_image_state(img) == SG_RESOURCESTATE_VALID);
    metal_mock_texture_info_t info = {0};
    T(metal_mock_texture_info(sg_mtl_query_image_info(img).tex[0], &info));
    T(info.texture_type == MTLTextureType2DArray);
    T(info.array_length == 3);
    T(info.num_replace_region == 3);
    teardown();
}

UTEST(sokol_gfx_metal, create_image_msaa_attachment) {
    setup();
    sg_image img = sg_make_image(&(sg_image_desc){
        .width = 16,
        .height = 16,
        .sample_count = 4,
        .pixel_format = SG_PIXELFORMAT_RGBA8,
        .usage = { .color_attachment = true, .immutable = true },
    });
    T(sg_query_image_state(img) == SG_RESOURCESTATE_VALID);
    metal_mock_texture_info_t info = {0};
    T(metal_mock_texture_info(sg_mtl_query_image_info(img).tex[0], &info));
    T(info.texture_type == MTLTextureType2DMultisample);
    T(info.sample_count == 4);
    T(info.usage & MTLTextureUsageRenderTarget);
    T((info.resource_options & MTLResourceStorageModePrivate) == MTLResourceStorageModePrivate);
    teardown();
}

UTEST(sokol_gfx_metal, create_image_storage) {
    setup();
    sg_image img = sg_make_image(&(sg_image_desc){
        .width = 16,
        .height = 16,
        .pixel_format = SG_PIXELFORMAT_RGBA8,
        .usage = { .storage_image = true, .immutable = true },
    });
    T(sg_query_image_state(img) == SG_RESOURCESTATE_VALID);
    metal_mock_texture_info_t info = {0};
    T(metal_mock_texture_info(sg_mtl_query_image_info(img).tex[0], &info));
    T(info.usage & MTLTextureUsageShaderWrite);
    teardown();
}

UTEST(sokol_gfx_metal, create_image_failed) {
    setup();
    metal_mock_fail_next(METAL_MOCK_OBJ_TEXTURE, 1);
    sg_image img = sg_make_image(&(sg_image_desc){
        .width = 16,
        .height = 16,
        .pixel_format = SG_PIXELFORMAT_RGBA8,
        .usage.dynamic_update = true,
    });
    T(sg_query_image_state(img) == SG_RESOURCESTATE_FAILED);
    T(logged(SG_LOGITEM_METAL_CREATE_TEXTURE_FAILED));
    teardown();
}

UTEST(sokol_gfx_metal, pixelformat_sweep) {
    setup();
    int num_created = 0;
    for (int i = SG_PIXELFORMAT_NONE + 1; i < _SG_PIXELFORMAT_NUM; i++) {
        const sg_pixel_format fmt = (sg_pixel_format)i;
        const sg_pixelformat_info fmt_info = sg_query_pixelformat(fmt);
        sg_image_desc desc = { .width = 32, .height = 32, .pixel_format = fmt };
        if (fmt_info.depth) {
            desc.usage.depth_stencil_attachment = true;
            desc.usage.immutable = true;
        } else if (!fmt_info.sample) {
            continue;
        } else if (fmt_info.compressed) {
            const int size = sg_query_surface_pitch(fmt, 32, 32, 1);
            if ((size <= 0) || ((size_t)size > sizeof(scratch))) {
                continue;
            }
            desc.usage.immutable = true;
            desc.data.mip_levels[0] = (sg_range){ .ptr = scratch, .size = (size_t)size };
        } else {
            desc.usage.dynamic_update = true;
        }
        sg_image img = sg_make_image(&desc);
        T(sg_query_image_state(img) == SG_RESOURCESTATE_VALID);
        sg_destroy_image(img);
        num_created++;
    }
    T(num_created > 20);
    teardown();
}

UTEST(sokol_gfx_metal, update_image) {
    setup();
    sg_image img = sg_make_image(&(sg_image_desc){
        .width = 16,
        .height = 16,
        .pixel_format = SG_PIXELFORMAT_RGBA8,
        .usage.dynamic_update = true,
    });
    metal_mock_clear_calls();
    sg_update_image(img, &(sg_image_data){
        .mip_levels[0] = { .ptr = scratch, .size = 16 * 16 * 4 },
    });
    T(metal_mock_count_calls(METAL_MOCK_FUNC_replaceRegion) == 1);
    const metal_mock_call_t* call = metal_mock_last_call(METAL_MOCK_FUNC_replaceRegion);
    T(call->args[2].u == 16);   // region width
    T(call->args[3].u == 16);   // region height
    T(call->args[6].u == 16 * 4);
    teardown();
}

UTEST(sokol_gfx_metal, write_image_transient) {
    setup();
    sg_image img = sg_make_image(&(sg_image_desc){
        .width = 16,
        .height = 16,
        .pixel_format = SG_PIXELFORMAT_RGBA8,
        .usage.write_transient = true,
    });
    metal_mock_clear_calls();
    sg_write_image_transient(&(sg_write_image_desc){
        .src = {
            .data = { .ptr = scratch, .size = 8 * 8 * 4 },
            .bytes_per_row = 8 * 4,
            .bytes_per_slice = 8 * 8 * 4,
        },
        .dst = { .image = img, .x = 4, .y = 4 },
        .size = { .width = 8, .height = 8, .num_slices = 1 },
    });
    const metal_mock_call_t* call = metal_mock_last_call(METAL_MOCK_FUNC_replaceRegion);
    T(call != 0);
    T(call->args[0].u == 4);
    T(call->args[1].u == 4);
    T(call->args[2].u == 8);
    T(call->args[3].u == 8);
    teardown();
}

UTEST(sokol_gfx_metal, write_image_unsealed) {
    setup();
    sg_image img = sg_make_image(&(sg_image_desc){
        .width = 16,
        .height = 16,
        .pixel_format = SG_PIXELFORMAT_RGBA8,
        .usage.write_unsealed = true,
    });
    metal_mock_clear_calls();
    sg_write_image_unsealed(&(sg_write_image_desc){
        .src = { .data = { .ptr = scratch, .size = 16 * 16 * 4 }, .bytes_per_row = 16 * 4 },
        .dst = { .image = img },
        .size = { .width = 16, .height = 16, .num_slices = 1 },
    });
    T(metal_mock_count_calls(METAL_MOCK_FUNC_replaceRegion) == 1);
    sg_seal_image(img);
    T(sg_query_image_state(img) == SG_RESOURCESTATE_VALID);
    teardown();
}

//== samplers ==================================================================
UTEST(sokol_gfx_metal, create_sampler_default) {
    setup();
    sg_sampler smp = sg_make_sampler(&(sg_sampler_desc){ .label = "smp" });
    T(sg_query_sampler_state(smp) == SG_RESOURCESTATE_VALID);
    metal_mock_sampler_info_t info = {0};
    T(metal_mock_sampler_info(sg_mtl_query_sampler_info(smp).smp, &info));
    T(info.min_filter == MTLSamplerMinMagFilterNearest);
    T(info.mag_filter == MTLSamplerMinMagFilterNearest);
    T(info.s_address_mode == MTLSamplerAddressModeRepeat);
    T(info.normalized_coordinates);
    T_LABEL(info.label, "smp");
    teardown();
}

UTEST(sokol_gfx_metal, create_sampler_sweep) {
    setup();
    const sg_wrap wraps[4] = {
        SG_WRAP_REPEAT, SG_WRAP_CLAMP_TO_EDGE, SG_WRAP_CLAMP_TO_BORDER, SG_WRAP_MIRRORED_REPEAT,
    };
    const sg_border_color borders[3] = {
        SG_BORDERCOLOR_TRANSPARENT_BLACK, SG_BORDERCOLOR_OPAQUE_BLACK, SG_BORDERCOLOR_OPAQUE_WHITE,
    };
    for (int i = 0; i < 4; i++) {
        sg_sampler smp = sg_make_sampler(&(sg_sampler_desc){
            .min_filter = SG_FILTER_LINEAR,
            .mag_filter = SG_FILTER_LINEAR,
            .mipmap_filter = SG_FILTER_LINEAR,
            .wrap_u = wraps[i],
            .wrap_v = wraps[(i + 1) % 4],
            .wrap_w = wraps[(i + 2) % 4],
            .border_color = borders[i % 3],
            .min_lod = 0.0f,
            .max_lod = 8.0f,
            .max_anisotropy = 4,
        });
        T(sg_query_sampler_state(smp) == SG_RESOURCESTATE_VALID);
        metal_mock_sampler_info_t info = {0};
        T(metal_mock_sampler_info(sg_mtl_query_sampler_info(smp).smp, &info));
        T(info.min_filter == MTLSamplerMinMagFilterLinear);
        T(info.mip_filter == MTLSamplerMipFilterLinear);
        T(info.max_anisotropy == 4);
        T(info.lod_max_clamp == 8.0f);
        sg_destroy_sampler(smp);
    }
    // comparison sampler
    sg_sampler cmp_smp = sg_make_sampler(&(sg_sampler_desc){
        .compare = SG_COMPAREFUNC_LESS_EQUAL,
    });
    metal_mock_sampler_info_t info = {0};
    T(metal_mock_sampler_info(sg_mtl_query_sampler_info(cmp_smp).smp, &info));
    T(info.compare_function == MTLCompareFunctionLessEqual);
    teardown();
}

UTEST(sokol_gfx_metal, create_sampler_failed) {
    setup();
    metal_mock_fail_next(METAL_MOCK_OBJ_SAMPLER, 1);
    sg_sampler smp = sg_make_sampler(&(sg_sampler_desc){0});
    T(sg_query_sampler_state(smp) == SG_RESOURCESTATE_FAILED);
    T(logged(SG_LOGITEM_METAL_CREATE_SAMPLER_FAILED));
    teardown();
}

//== shaders ===================================================================
UTEST(sokol_gfx_metal, create_shader_from_source) {
    setup();
    sg_shader shd = make_shader();
    T(sg_query_shader_state(shd) == SG_RESOURCESTATE_VALID);
    T(metal_mock_count_calls(METAL_MOCK_FUNC_newLibraryWithSource) == 2);
    T(metal_mock_count_calls(METAL_MOCK_FUNC_newFunctionWithName) == 2);
    metal_mock_library_info_t lib_info = {0};
    T(metal_mock_library_info(sg_mtl_query_shader_info(shd).vertex_lib, &lib_info));
    T(!lib_info.from_bytecode);
    T(0 == strcmp(lib_info.source, "vs_source"));
    metal_mock_function_info_t func_info = {0};
    T(metal_mock_function_info(sg_mtl_query_shader_info(shd).vertex_func, &func_info));
    T(0 == strcmp(func_info.name, "_main"));
    teardown();
}

UTEST(sokol_gfx_metal, create_shader_from_bytecode) {
    setup();
    static const uint8_t vs_bytes[16] = {1};
    static const uint8_t fs_bytes[24] = {2};
    sg_shader shd = sg_make_shader(&(sg_shader_desc){
        .vertex_func = { .bytecode = SG_RANGE(vs_bytes), .entry = "vs_main" },
        .fragment_func = { .bytecode = SG_RANGE(fs_bytes), .entry = "fs_main" },
    });
    T(sg_query_shader_state(shd) == SG_RESOURCESTATE_VALID);
    T(metal_mock_count_calls(METAL_MOCK_FUNC_newLibraryWithData) == 2);
    metal_mock_library_info_t lib_info = {0};
    T(metal_mock_library_info(sg_mtl_query_shader_info(shd).vertex_lib, &lib_info));
    T(lib_info.from_bytecode);
    metal_mock_function_info_t func_info = {0};
    T(metal_mock_function_info(sg_mtl_query_shader_info(shd).vertex_func, &func_info));
    T(0 == strcmp(func_info.name, "vs_main"));
    teardown();
}

UTEST(sokol_gfx_metal, create_shader_compilation_failed) {
    setup();
    metal_mock_fail_next(METAL_MOCK_OBJ_LIBRARY, 2);
    sg_shader shd = make_shader();
    T(sg_query_shader_state(shd) == SG_RESOURCESTATE_FAILED);
    T(logged(SG_LOGITEM_METAL_SHADER_COMPILATION_FAILED));
    teardown();
}

UTEST(sokol_gfx_metal, create_shader_creation_failed) {
    setup();
    static const uint8_t bytes[16] = {1};
    metal_mock_fail_next(METAL_MOCK_OBJ_LIBRARY, 2);
    sg_shader shd = sg_make_shader(&(sg_shader_desc){
        .vertex_func.bytecode = SG_RANGE(bytes),
        .fragment_func.bytecode = SG_RANGE(bytes),
    });
    T(sg_query_shader_state(shd) == SG_RESOURCESTATE_FAILED);
    T(logged(SG_LOGITEM_METAL_SHADER_CREATION_FAILED));
    teardown();
}

UTEST(sokol_gfx_metal, create_shader_entry_not_found) {
    setup();
    metal_mock_fail_next(METAL_MOCK_OBJ_FUNCTION, 2);
    sg_shader shd = make_shader();
    T(sg_query_shader_state(shd) == SG_RESOURCESTATE_FAILED);
    T(logged(SG_LOGITEM_METAL_SHADER_ENTRY_NOT_FOUND));
    // a library without a function is released right away, it never reaches the id pool
    T(metal_mock_live_objects(METAL_MOCK_OBJ_LIBRARY) == 0);
    teardown();
}

UTEST(sokol_gfx_metal, create_shader_partial_failure_library) {
    setup();
    // let the vertex func succeed and the fragment lib fail, the vertex lib and
    // func are already in the id pool at that point
    metal_mock_fail_next_after(METAL_MOCK_OBJ_LIBRARY, 1, 1);
    sg_shader shd = make_shader();
    T(sg_query_shader_state(shd) == SG_RESOURCESTATE_FAILED);
    T(logged(SG_LOGITEM_METAL_SHADER_COMPILATION_FAILED));
    T(metal_mock_num_created(METAL_MOCK_OBJ_LIBRARY) == 1);
    T(metal_mock_live_objects(METAL_MOCK_OBJ_LIBRARY) == 1);
    T(metal_mock_live_objects(METAL_MOCK_OBJ_FUNCTION) == 1);
    // the objects of the vertex func must be released exactly once, discarding
    // the failed shader must not release the id pool slots a second time
    teardown();
    T(metal_mock_live_objects(METAL_MOCK_OBJ_LIBRARY) == 0);
    T(metal_mock_live_objects(METAL_MOCK_OBJ_FUNCTION) == 0);
}

UTEST(sokol_gfx_metal, create_shader_partial_failure_function) {
    setup();
    // same as above, but the fragment lib compiles and only its entry lookup fails
    metal_mock_fail_next_after(METAL_MOCK_OBJ_FUNCTION, 1, 1);
    sg_shader shd = make_shader();
    T(sg_query_shader_state(shd) == SG_RESOURCESTATE_FAILED);
    T(logged(SG_LOGITEM_METAL_SHADER_ENTRY_NOT_FOUND));
    T(metal_mock_num_created(METAL_MOCK_OBJ_LIBRARY) == 2);
    // the fragment lib is released right away, only the vertex lib is in the id pool
    T(metal_mock_live_objects(METAL_MOCK_OBJ_LIBRARY) == 1);
    T(metal_mock_live_objects(METAL_MOCK_OBJ_FUNCTION) == 1);
    teardown();
    T(metal_mock_live_objects(METAL_MOCK_OBJ_LIBRARY) == 0);
    T(metal_mock_live_objects(METAL_MOCK_OBJ_FUNCTION) == 0);
}

UTEST(sokol_gfx_metal, create_compute_shader) {
    setup();
    sg_shader shd = make_compute_shader();
    T(sg_query_shader_state(shd) == SG_RESOURCESTATE_VALID);
    T(metal_mock_count_calls(METAL_MOCK_FUNC_newLibraryWithSource) == 1);
    teardown();
}

UTEST(sokol_gfx_metal, shader_msl_bindslot_out_of_range) {
    // the MSL bind slot range checks are backend code, not validation layer code
    setup_with((setup_desc_t){ .disable_validation = true });
    sg_shader shd0 = sg_make_shader(&(sg_shader_desc){
        .vertex_func.source = "vs",
        .fragment_func.source = "fs",
        .uniform_blocks[0] = { .stage = SG_SHADERSTAGE_VERTEX, .size = 16, .msl_buffer_n = 100 },
    });
    T(sg_query_shader_state(shd0) == SG_RESOURCESTATE_FAILED);
    T(logged(SG_LOGITEM_METAL_UNIFORMBLOCK_MSL_BUFFER_SLOT_OUT_OF_RANGE));

    sg_shader shd1 = sg_make_shader(&(sg_shader_desc){
        .vertex_func.source = "vs",
        .fragment_func.source = "fs",
        .views[0].storage_buffer = { .stage = SG_SHADERSTAGE_VERTEX, .readonly = true, .msl_buffer_n = 100 },
    });
    T(sg_query_shader_state(shd1) == SG_RESOURCESTATE_FAILED);
    T(logged(SG_LOGITEM_METAL_STORAGEBUFFER_MSL_BUFFER_SLOT_OUT_OF_RANGE));

    sg_shader shd2 = sg_make_shader(&(sg_shader_desc){
        .vertex_func.source = "vs",
        .fragment_func.source = "fs",
        .views[0].texture = {
            .stage = SG_SHADERSTAGE_FRAGMENT,
            .image_type = SG_IMAGETYPE_2D,
            .sample_type = SG_IMAGESAMPLETYPE_FLOAT,
            .msl_texture_n = 100,
        },
    });
    T(sg_query_shader_state(shd2) == SG_RESOURCESTATE_FAILED);
    T(logged(SG_LOGITEM_METAL_IMAGE_MSL_TEXTURE_SLOT_OUT_OF_RANGE));

    sg_shader shd3 = sg_make_shader(&(sg_shader_desc){
        .vertex_func.source = "vs",
        .fragment_func.source = "fs",
        .samplers[0] = {
            .stage = SG_SHADERSTAGE_FRAGMENT,
            .sampler_type = SG_SAMPLERTYPE_FILTERING,
            .msl_sampler_n = 100,
        },
    });
    T(sg_query_shader_state(shd3) == SG_RESOURCESTATE_FAILED);
    T(logged(SG_LOGITEM_METAL_SAMPLER_MSL_SAMPLER_SLOT_OUT_OF_RANGE));

    sg_shader shd4 = sg_make_shader(&(sg_shader_desc){
        .compute_func.source = "cs",
        .mtl_threads_per_threadgroup = { .x = 32, .y = 1, .z = 1 },
        .views[0].storage_image = {
            .stage = SG_SHADERSTAGE_COMPUTE,
            .image_type = SG_IMAGETYPE_2D,
            .access_format = SG_PIXELFORMAT_RGBA8,
            .writeonly = true,
            .msl_texture_n = 100,
        },
    });
    T(sg_query_shader_state(shd4) == SG_RESOURCESTATE_FAILED);
    T(logged(SG_LOGITEM_METAL_STORAGEIMAGE_MSL_TEXTURE_SLOT_OUT_OF_RANGE));
    teardown();
}

//== pipelines =================================================================
UTEST(sokol_gfx_metal, create_render_pipeline) {
    setup();
    sg_shader shd = make_shader();
    sg_pipeline pip = sg_make_pipeline(&(sg_pipeline_desc){
        .shader = shd,
        .layout = {
            .buffers[0].stride = 20,
            .attrs = {
                [0] = { .format = SG_VERTEXFORMAT_FLOAT3, .offset = 0, .buffer_index = 0 },
                [1] = { .format = SG_VERTEXFORMAT_UBYTE4N, .offset = 12, .buffer_index = 0 },
                [2] = { .format = SG_VERTEXFORMAT_SHORT2N, .offset = 16, .buffer_index = 0 },
            },
        },
        .cull_mode = SG_CULLMODE_BACK,
        .face_winding = SG_FACEWINDING_CCW,
        .depth = {
            .pixel_format = SG_PIXELFORMAT_DEPTH_STENCIL,
            .compare = SG_COMPAREFUNC_LESS_EQUAL,
            .write_enabled = true,
            .bias = 1.0f,
            .bias_slope_scale = 2.0f,
            .bias_clamp = 3.0f,
        },
        .colors[0].pixel_format = SG_PIXELFORMAT_BGRA8,
        .label = "render-pip",
    });
    T(sg_query_pipeline_state(pip) == SG_RESOURCESTATE_VALID);
    metal_mock_render_pipeline_info_t info = {0};
    T(metal_mock_render_pipeline_info(sg_mtl_query_pipeline_info(pip).rps, &info));
    T(info.has_vertex_descriptor);
    T(info.raster_sample_count == 1);
    T(info.rasterization_enabled);
    T(info.depth_attachment_pixel_format == MTLPixelFormatDepth32Float_Stencil8);
    T(info.stencil_attachment_pixel_format == MTLPixelFormatDepth32Float_Stencil8);
    T(info.color_attachments[0].pixel_format == MTLPixelFormatBGRA8Unorm);
    T(info.color_attachments[0].write_mask == MTLColorWriteMaskAll);
    T(!info.color_attachments[0].blending_enabled);
    T(info.vertex_attrs[0].format == MTLVertexFormatFloat3);
    T(info.vertex_attrs[0].buffer_index == MTL_VB_SLOT(0));
    T(info.vertex_attrs[1].format == MTLVertexFormatUChar4Normalized);
    T(info.vertex_attrs[1].offset == 12);
    T(info.vertex_attrs[2].format == MTLVertexFormatShort2Normalized);
    T(info.vertex_layouts[MTL_VB_SLOT(0)].stride == 20);
    T(info.vertex_layouts[MTL_VB_SLOT(0)].step_function == MTLVertexStepFunctionPerVertex);
    T_LABEL(info.label, "render-pip");

    metal_mock_depth_stencil_info_t ds_info = {0};
    T(metal_mock_depth_stencil_info(sg_mtl_query_pipeline_info(pip).dss, &ds_info));
    T(ds_info.depth_compare_function == MTLCompareFunctionLessEqual);
    T(ds_info.depth_write_enabled);
    T(!ds_info.has_front_stencil);
    teardown();
}

UTEST(sokol_gfx_metal, pipeline_vertex_format_sweep) {
    setup();
    sg_shader shd = make_shader();
    for (int i = 1; i < _SG_VERTEXFORMAT_NUM; i++) {
        sg_pipeline pip = sg_make_pipeline(&(sg_pipeline_desc){
            .shader = shd,
            .layout.attrs[0].format = (sg_vertex_format)i,
            .depth.pixel_format = SG_PIXELFORMAT_DEPTH_STENCIL,
            .colors[0].pixel_format = SG_PIXELFORMAT_BGRA8,
        });
        T(sg_query_pipeline_state(pip) == SG_RESOURCESTATE_VALID);
        sg_destroy_pipeline(pip);
    }
    teardown();
}

UTEST(sokol_gfx_metal, pipeline_blend_sweep) {
    setup();
    sg_shader shd = make_shader();
    for (int i = 1; i < _SG_BLENDFACTOR_NUM; i++) {
        sg_pipeline pip = sg_make_pipeline(&(sg_pipeline_desc){
            .shader = shd,
            .layout.attrs[0].format = SG_VERTEXFORMAT_FLOAT3,
            .depth.pixel_format = SG_PIXELFORMAT_DEPTH_STENCIL,
            .colors[0] = {
                .pixel_format = SG_PIXELFORMAT_BGRA8,
                .write_mask = SG_COLORMASK_RGB,
                .blend = {
                    .enabled = true,
                    .src_factor_rgb = (sg_blend_factor)i,
                    .dst_factor_rgb = (sg_blend_factor)i,
                    .op_rgb = SG_BLENDOP_ADD,
                    .src_factor_alpha = (sg_blend_factor)i,
                    .dst_factor_alpha = (sg_blend_factor)i,
                    .op_alpha = SG_BLENDOP_SUBTRACT,
                },
            },
        });
        T(sg_query_pipeline_state(pip) == SG_RESOURCESTATE_VALID);
        metal_mock_render_pipeline_info_t info = {0};
        T(metal_mock_render_pipeline_info(sg_mtl_query_pipeline_info(pip).rps, &info));
        T(info.color_attachments[0].blending_enabled);
        T(info.color_attachments[0].op_rgb == MTLBlendOperationAdd);
        T(info.color_attachments[0].op_alpha == MTLBlendOperationSubtract);
        T(info.color_attachments[0].write_mask == (MTLColorWriteMaskRed|MTLColorWriteMaskGreen|MTLColorWriteMaskBlue));
        sg_destroy_pipeline(pip);
    }
    // min/max blend ops require ONE/ONE factors
    const sg_blend_op ops[2] = { SG_BLENDOP_MIN, SG_BLENDOP_MAX };
    for (int i = 0; i < 2; i++) {
        sg_pipeline pip = sg_make_pipeline(&(sg_pipeline_desc){
            .shader = shd,
            .layout.attrs[0].format = SG_VERTEXFORMAT_FLOAT3,
            .depth.pixel_format = SG_PIXELFORMAT_DEPTH_STENCIL,
            .colors[0] = {
                .pixel_format = SG_PIXELFORMAT_BGRA8,
                .write_mask = SG_COLORMASK_NONE,
                .blend = {
                    .enabled = true,
                    .src_factor_rgb = SG_BLENDFACTOR_ONE,
                    .dst_factor_rgb = SG_BLENDFACTOR_ONE,
                    .op_rgb = ops[i],
                    .src_factor_alpha = SG_BLENDFACTOR_ONE,
                    .dst_factor_alpha = SG_BLENDFACTOR_ONE,
                    .op_alpha = ops[i],
                },
            },
        });
        T(sg_query_pipeline_state(pip) == SG_RESOURCESTATE_VALID);
        metal_mock_render_pipeline_info_t info = {0};
        T(metal_mock_render_pipeline_info(sg_mtl_query_pipeline_info(pip).rps, &info));
        T(info.color_attachments[0].write_mask == MTLColorWriteMaskNone);
        sg_destroy_pipeline(pip);
    }
    teardown();
}

UTEST(sokol_gfx_metal, pipeline_stencil) {
    setup();
    sg_shader shd = make_shader();
    sg_pipeline pip = sg_make_pipeline(&(sg_pipeline_desc){
        .shader = shd,
        .layout.attrs[0].format = SG_VERTEXFORMAT_FLOAT3,
        .depth.pixel_format = SG_PIXELFORMAT_DEPTH_STENCIL,
        .colors[0].pixel_format = SG_PIXELFORMAT_BGRA8,
        .stencil = {
            .enabled = true,
            .front = {
                .compare = SG_COMPAREFUNC_GREATER,
                .fail_op = SG_STENCILOP_KEEP,
                .depth_fail_op = SG_STENCILOP_INCR_CLAMP,
                .pass_op = SG_STENCILOP_REPLACE,
            },
            .back = {
                .compare = SG_COMPAREFUNC_NOT_EQUAL,
                .fail_op = SG_STENCILOP_ZERO,
                .depth_fail_op = SG_STENCILOP_DECR_WRAP,
                .pass_op = SG_STENCILOP_INVERT,
            },
            .read_mask = 0xF0,
            .write_mask = 0x0F,
            .ref = 0x42,
        },
    });
    T(sg_query_pipeline_state(pip) == SG_RESOURCESTATE_VALID);
    metal_mock_depth_stencil_info_t info = {0};
    T(metal_mock_depth_stencil_info(sg_mtl_query_pipeline_info(pip).dss, &info));
    T(info.has_front_stencil);
    T(info.has_back_stencil);
    T(info.front.compare_function == MTLCompareFunctionGreater);
    T(info.front.depth_fail_op == MTLStencilOperationIncrementClamp);
    T(info.front.pass_op == MTLStencilOperationReplace);
    T(info.back.compare_function == MTLCompareFunctionNotEqual);
    T(info.back.fail_op == MTLStencilOperationZero);
    T(info.back.depth_fail_op == MTLStencilOperationDecrementWrap);
    T(info.back.pass_op == MTLStencilOperationInvert);
    T(info.front.read_mask == 0xF0);
    T(info.front.write_mask == 0x0F);
    teardown();
}

UTEST(sokol_gfx_metal, pipeline_instanced_layout) {
    setup();
    sg_shader shd = make_shader();
    sg_pipeline pip = sg_make_pipeline(&(sg_pipeline_desc){
        .shader = shd,
        .layout = {
            .buffers = {
                [0] = { .stride = 12, .step_func = SG_VERTEXSTEP_PER_VERTEX },
                [1] = { .stride = 16, .step_func = SG_VERTEXSTEP_PER_INSTANCE, .step_rate = 1 },
            },
            .attrs = {
                [0] = { .format = SG_VERTEXFORMAT_FLOAT3, .buffer_index = 0 },
                [1] = { .format = SG_VERTEXFORMAT_FLOAT4, .buffer_index = 1 },
            },
        },
        .depth.pixel_format = SG_PIXELFORMAT_DEPTH_STENCIL,
        .colors[0].pixel_format = SG_PIXELFORMAT_BGRA8,
        .primitive_type = SG_PRIMITIVETYPE_TRIANGLE_STRIP,
        .index_type = SG_INDEXTYPE_UINT32,
    });
    T(sg_query_pipeline_state(pip) == SG_RESOURCESTATE_VALID);
    metal_mock_render_pipeline_info_t info = {0};
    T(metal_mock_render_pipeline_info(sg_mtl_query_pipeline_info(pip).rps, &info));
    T(info.vertex_layouts[MTL_VB_SLOT(0)].step_function == MTLVertexStepFunctionPerVertex);
    T(info.vertex_layouts[MTL_VB_SLOT(1)].step_function == MTLVertexStepFunctionPerInstance);
    T(info.vertex_layouts[MTL_VB_SLOT(1)].stride == 16);
    T(info.vertex_layouts[MTL_VB_SLOT(1)].step_rate == 1);
    teardown();
}

UTEST(sokol_gfx_metal, pipeline_mrt) {
    setup();
    sg_shader shd = make_shader();
    sg_pipeline pip = sg_make_pipeline(&(sg_pipeline_desc){
        .shader = shd,
        .layout.attrs[0].format = SG_VERTEXFORMAT_FLOAT3,
        .color_count = 3,
        .colors = {
            [0] = { .pixel_format = SG_PIXELFORMAT_RGBA8, .write_mask = SG_COLORMASK_R },
            [1] = { .pixel_format = SG_PIXELFORMAT_RGBA8, .write_mask = SG_COLORMASK_RG },
            [2] = { .pixel_format = SG_PIXELFORMAT_RGBA32F, .write_mask = SG_COLORMASK_RGBA },
        },
        .depth.pixel_format = SG_PIXELFORMAT_NONE,
        .sample_count = 1,
    });
    T(sg_query_pipeline_state(pip) == SG_RESOURCESTATE_VALID);
    metal_mock_render_pipeline_info_t info = {0};
    T(metal_mock_render_pipeline_info(sg_mtl_query_pipeline_info(pip).rps, &info));
    T(info.color_attachments[0].write_mask == MTLColorWriteMaskRed);
    T(info.color_attachments[1].write_mask == (MTLColorWriteMaskRed|MTLColorWriteMaskGreen));
    T(info.color_attachments[2].pixel_format == MTLPixelFormatRGBA32Float);
    T(info.depth_attachment_pixel_format == MTLPixelFormatInvalid);
    teardown();
}

UTEST(sokol_gfx_metal, create_pipeline_rps_failed) {
    setup();
    sg_shader shd = make_shader();
    metal_mock_fail_next(METAL_MOCK_OBJ_RENDER_PIPELINE, 1);
    sg_pipeline pip = make_pipeline(shd);
    T(sg_query_pipeline_state(pip) == SG_RESOURCESTATE_FAILED);
    T(logged(SG_LOGITEM_METAL_CREATE_RPS_FAILED));
    teardown();
}

UTEST(sokol_gfx_metal, create_pipeline_dss_failed) {
    setup();
    sg_shader shd = make_shader();
    metal_mock_fail_next(METAL_MOCK_OBJ_DEPTH_STENCIL, 1);
    sg_pipeline pip = make_pipeline(shd);
    T(sg_query_pipeline_state(pip) == SG_RESOURCESTATE_FAILED);
    T(logged(SG_LOGITEM_METAL_CREATE_DSS_FAILED));
    teardown();
}

UTEST(sokol_gfx_metal, create_compute_pipeline) {
    setup();
    sg_shader shd = sg_make_shader(&(sg_shader_desc){
        .compute_func.source = "cs_source",
        .mtl_threads_per_threadgroup = { .x = 32, .y = 1, .z = 1 },
        .views[0].storage_buffer = {
            .stage = SG_SHADERSTAGE_COMPUTE, .readonly = true, .msl_buffer_n = 8,
        },
        .views[1].storage_buffer = {
            .stage = SG_SHADERSTAGE_COMPUTE, .readonly = false, .msl_buffer_n = 9,
        },
    });
    sg_pipeline pip = sg_make_pipeline(&(sg_pipeline_desc){
        .compute = true,
        .shader = shd,
        .label = "compute-pip",
    });
    T(sg_query_pipeline_state(pip) == SG_RESOURCESTATE_VALID);
    // the MTLComputePipelineState is not exposed by the public query API, get
    // it from the compute encoder after binding the pipeline
    sg_begin_pass(&(sg_pass){ .compute = true });
    sg_apply_pipeline(pip);
    metal_mock_compute_pipeline_info_t info = {0};
    T(metal_mock_compute_pipeline_info(metal_mock_compute_encoder_state()->pipeline_state, &info));
    T(info.threadgroup_size_is_multiple_of_thread_execution_width);
    T(info.buffer_mutability[8] == MTLMutabilityImmutable);
    T(info.buffer_mutability[9] == MTLMutabilityDefault);
    T_LABEL(info.label, "compute-pip");
    sg_end_pass();
    sg_commit();
    teardown();
}

UTEST(sokol_gfx_metal, create_compute_pipeline_failed) {
    setup();
    sg_shader shd = make_compute_shader();
    metal_mock_fail_next(METAL_MOCK_OBJ_COMPUTE_PIPELINE, 1);
    sg_pipeline pip = sg_make_pipeline(&(sg_pipeline_desc){ .compute = true, .shader = shd });
    T(sg_query_pipeline_state(pip) == SG_RESOURCESTATE_FAILED);
    T(logged(SG_LOGITEM_METAL_CREATE_CPS_FAILED));
    teardown();
}

//== views =====================================================================
UTEST(sokol_gfx_metal, create_texture_view) {
    setup();
    sg_image img = sg_make_image(&(sg_image_desc){
        .type = SG_IMAGETYPE_ARRAY,
        .width = 8,
        .height = 8,
        .num_slices = 4,
        .num_mipmaps = 2,
        .pixel_format = SG_PIXELFORMAT_RGBA8,
        .usage.immutable = true,
        .data.mip_levels = {
            [0] = { .ptr = scratch, .size = 4 * 8 * 8 * 4 },
            [1] = { .ptr = scratch, .size = 4 * 4 * 4 * 4 },
        },
    });
    metal_mock_clear_calls();
    sg_view view = sg_make_view(&(sg_view_desc){
        .texture = {
            .image = img,
            .mip_levels = { .base = 1, .count = 1 },
            .slices = { .base = 2, .count = 2 },
        },
    });
    T(sg_query_view_state(view) == SG_RESOURCESTATE_VALID);
    T(metal_mock_count_calls(METAL_MOCK_FUNC_newTextureViewWithPixelFormat) == 1);
    const metal_mock_call_t* call = metal_mock_last_call(METAL_MOCK_FUNC_newTextureViewWithPixelFormat);
    T(call->args[0].u == MTLPixelFormatRGBA8Unorm);
    T(call->args[1].u == MTLTextureType2DArray);
    T(call->args[2].u == 1);    // mip level base
    T(call->args[3].u == 1);    // mip level count
    T(call->args[4].u == 2);    // slice base
    T(call->args[5].u == 2);    // slice count
    // a texture view is tracked as its own object kind
    T(metal_mock_live_objects(METAL_MOCK_OBJ_TEXTURE_VIEW) == 1);
    teardown();
}

UTEST(sokol_gfx_metal, create_texture_view_failed) {
    setup();
    sg_image img = sg_make_image(&(sg_image_desc){
        .width = 16,
        .height = 16,
        .pixel_format = SG_PIXELFORMAT_RGBA8,
        .usage.immutable = true,
        .data.mip_levels[0] = { .ptr = scratch, .size = 16 * 16 * 4 },
    });
    metal_mock_clear_calls();
    metal_mock_fail_next(METAL_MOCK_OBJ_TEXTURE_VIEW, 1);
    sg_view view = sg_make_view(&(sg_view_desc){ .texture.image = img });
    T(sg_query_view_state(view) == SG_RESOURCESTATE_FAILED);
    T(logged(SG_LOGITEM_METAL_CREATE_TEXTUREVIEW_FAILED));
    T(metal_mock_count_calls(METAL_MOCK_FUNC_newTextureViewWithPixelFormat) == 1);
    T(metal_mock_num_created(METAL_MOCK_OBJ_TEXTURE_VIEW) == 0);
    teardown();
    T(metal_mock_live_objects(METAL_MOCK_OBJ_TEXTURE_VIEW) == 0);
}

UTEST(sokol_gfx_metal, create_texture_view_failed_second_slot) {
    setup();
    // a non-immutable image has one MTLTexture per inflight frame, so
    // _sg_mtl_create_view() creates one texture view per slot
    sg_image img = sg_make_image(&(sg_image_desc){
        .width = 16,
        .height = 16,
        .pixel_format = SG_PIXELFORMAT_RGBA8,
        .usage.write_transient = true,
    });
    metal_mock_clear_calls();
    // let the second slot fail, the view of the first slot is already created
    metal_mock_fail_next_after(METAL_MOCK_OBJ_TEXTURE_VIEW, 1, 1);
    sg_view view = sg_make_view(&(sg_view_desc){ .texture.image = img });
    T(sg_query_view_state(view) == SG_RESOURCESTATE_FAILED);
    T(logged(SG_LOGITEM_METAL_CREATE_TEXTUREVIEW_FAILED));
    T(metal_mock_count_calls(METAL_MOCK_FUNC_newTextureViewWithPixelFormat) == 2);
    // only the view of the first slot was actually created
    T(metal_mock_num_created(METAL_MOCK_OBJ_TEXTURE_VIEW) == 1);
    T(metal_mock_live_objects(METAL_MOCK_OBJ_TEXTURE_VIEW) == 1);
    teardown();
    // the view of the first slot must be released exactly once
    T(metal_mock_live_objects(METAL_MOCK_OBJ_TEXTURE_VIEW) == 0);
}

UTEST(sokol_gfx_metal, create_storage_image_view) {
    setup();
    sg_image img = sg_make_image(&(sg_image_desc){
        .width = 16,
        .height = 16,
        .pixel_format = SG_PIXELFORMAT_RGBA8,
        .usage = { .storage_image = true, .immutable = true },
    });
    metal_mock_clear_calls();
    sg_view view = sg_make_view(&(sg_view_desc){ .storage_image.image = img });
    T(sg_query_view_state(view) == SG_RESOURCESTATE_VALID);
    T(metal_mock_count_calls(METAL_MOCK_FUNC_newTextureViewWithPixelFormat) == 1);
    teardown();
}

UTEST(sokol_gfx_metal, create_attachment_view_no_mtl_object) {
    setup();
    sg_image img = sg_make_image(&(sg_image_desc){
        .width = 16,
        .height = 16,
        .pixel_format = SG_PIXELFORMAT_RGBA8,
        .usage = { .color_attachment = true, .immutable = true },
    });
    metal_mock_clear_calls();
    sg_view view = sg_make_view(&(sg_view_desc){ .color_attachment.image = img });
    T(sg_query_view_state(view) == SG_RESOURCESTATE_VALID);
    // attachment views reuse the image's MTLTexture, no texture view is created
    T(metal_mock_count_calls(METAL_MOCK_FUNC_newTextureViewWithPixelFormat) == 0);
    T(metal_mock_live_objects(METAL_MOCK_OBJ_TEXTURE_VIEW) == 0);
    teardown();
}

//== passes ====================================================================
UTEST(sokol_gfx_metal, swapchain_pass) {
    setup();
    swapchain_t sc = make_swapchain(64, 32, 1, true);
    metal_mock_clear_calls();
    sg_begin_pass(&(sg_pass){
        .action = {
            .colors[0] = { .load_action = SG_LOADACTION_CLEAR, .clear_value = { 0.25f, 0.5f, 0.75f, 1.0f } },
            .depth = { .load_action = SG_LOADACTION_CLEAR, .clear_value = 0.5f },
            .stencil = { .load_action = SG_LOADACTION_CLEAR, .clear_value = 7 },
        },
        .swapchain = swapchain(&sc),
    });
    T(metal_mock_count_calls(METAL_MOCK_FUNC_commandBufferWithUnretainedReferences) == 1);
    T(metal_mock_count_calls(METAL_MOCK_FUNC_enqueue) == 1);
    T(metal_mock_count_calls(METAL_MOCK_FUNC_addCompletedHandler) == 1);
    T(metal_mock_count_calls(METAL_MOCK_FUNC_renderCommandEncoderWithDescriptor) == 1);
    const metal_mock_render_pass_info_t* pass = metal_mock_last_render_pass();
    T(pass->num_color_attachments == 1);
    T(pass->color_attachments[0].load_action == MTLLoadActionClear);
    T(pass->color_attachments[0].store_action == MTLStoreActionStore);
    T(pass->color_attachments[0].clear_color.red == 0.25);
    T(pass->color_attachments[0].resolve_texture == 0);
    T(pass->has_depth_attachment);
    T(pass->depth_attachment.texture == sc.depth_tex);
    T(pass->depth_attachment.clear_depth == 0.5);
    T(pass->has_stencil_attachment);
    T(pass->stencil_attachment.clear_stencil == 7);
    T(sg_mtl_render_command_encoder() != 0);
    sg_end_pass();
    T(metal_mock_count_calls(METAL_MOCK_FUNC_endEncoding) == 1);
    T(metal_mock_count_calls(METAL_MOCK_FUNC_presentDrawable) == 1);
    sg_commit();
    T(metal_mock_count_calls(METAL_MOCK_FUNC_commit) == 1);
    teardown();
    discard_swapchain(&sc);
}

UTEST(sokol_gfx_metal, swapchain_pass_retained_cmd_buffer) {
    setup_with((setup_desc_t){ .retained_cmd_buffer = true });
    swapchain_t sc = make_swapchain(64, 32, 1, true);
    sg_begin_pass(&(sg_pass){ .swapchain = swapchain(&sc) });
    T(metal_mock_count_calls(METAL_MOCK_FUNC_commandBuffer) == 1);
    T(metal_mock_count_calls(METAL_MOCK_FUNC_commandBufferWithUnretainedReferences) == 0);
    sg_end_pass();
    sg_commit();
    teardown();
    discard_swapchain(&sc);
}

UTEST(sokol_gfx_metal, swapchain_pass_msaa) {
    setup();
    swapchain_t sc = make_swapchain(64, 32, 4, true);
    metal_mock_clear_calls();
    sg_begin_pass(&(sg_pass){ .swapchain = swapchain(&sc) });
    const metal_mock_render_pass_info_t* pass = metal_mock_last_render_pass();
    T(pass->color_attachments[0].texture == sc.msaa_tex);
    T(pass->color_attachments[0].resolve_texture != 0);
    T(pass->color_attachments[0].store_action == MTLStoreActionMultisampleResolve);
    sg_end_pass();
    sg_commit();
    teardown();
    discard_swapchain(&sc);
}

UTEST(sokol_gfx_metal, swapchain_pass_no_depth) {
    setup();
    swapchain_t sc = make_swapchain(64, 32, 1, false);
    metal_mock_clear_calls();
    sg_begin_pass(&(sg_pass){
        .action.colors[0] = { .load_action = SG_LOADACTION_DONTCARE },
        .swapchain = swapchain(&sc),
    });
    const metal_mock_render_pass_info_t* pass = metal_mock_last_render_pass();
    T(!pass->has_depth_attachment);
    T(pass->color_attachments[0].load_action == MTLLoadActionDontCare);
    sg_end_pass();
    sg_commit();
    teardown();
    discard_swapchain(&sc);
}

UTEST(sokol_gfx_metal, swapchain_pass_no_drawable) {
    // a null drawable is rejected by the validation layer, test the backend bail-out path
    setup_with((setup_desc_t){ .disable_validation = true });
    metal_mock_clear_calls();
    sg_begin_pass(&(sg_pass){
        .swapchain = {
            .width = 64, .height = 32, .sample_count = 1,
            .color_format = SG_PIXELFORMAT_BGRA8,
            .depth_format = SG_PIXELFORMAT_NONE,
        },
    });
    T(metal_mock_count_calls(METAL_MOCK_FUNC_renderCommandEncoderWithDescriptor) == 0);
    sg_end_pass();
    sg_commit();
    teardown();
}

UTEST(sokol_gfx_metal, swapchain_pass_encoder_failed) {
    setup();
    swapchain_t sc = make_swapchain(64, 32, 1, true);
    metal_mock_fail_next(METAL_MOCK_OBJ_RENDER_ENCODER, 1);
    sg_begin_pass(&(sg_pass){ .swapchain = swapchain(&sc) });
    T(sg_mtl_render_command_encoder() == 0);
    sg_end_pass();
    sg_commit();
    teardown();
    discard_swapchain(&sc);
}

UTEST(sokol_gfx_metal, offscreen_pass_load_store_actions) {
    setup();
    sg_image color_img = sg_make_image(&(sg_image_desc){
        .width = 32, .height = 32,
        .pixel_format = SG_PIXELFORMAT_RGBA8,
        .usage = { .color_attachment = true, .immutable = true },
    });
    sg_image depth_img = sg_make_image(&(sg_image_desc){
        .width = 32, .height = 32,
        .pixel_format = SG_PIXELFORMAT_DEPTH_STENCIL,
        .usage = { .depth_stencil_attachment = true, .immutable = true },
    });
    sg_view color_view = sg_make_view(&(sg_view_desc){ .color_attachment.image = color_img });
    sg_view ds_view = sg_make_view(&(sg_view_desc){ .depth_stencil_attachment.image = depth_img });

    const sg_load_action loads[3] = { SG_LOADACTION_CLEAR, SG_LOADACTION_LOAD, SG_LOADACTION_DONTCARE };
    const MTLLoadAction expected_loads[3] = { MTLLoadActionClear, MTLLoadActionLoad, MTLLoadActionDontCare };
    const sg_store_action stores[2] = { SG_STOREACTION_STORE, SG_STOREACTION_DONTCARE };
    const MTLStoreAction expected_stores[2] = { MTLStoreActionStore, MTLStoreActionDontCare };
    for (int l = 0; l < 3; l++) {
        for (int s = 0; s < 2; s++) {
            sg_begin_pass(&(sg_pass){
                .action = {
                    .colors[0] = { .load_action = loads[l], .store_action = stores[s] },
                    .depth = { .load_action = loads[l], .store_action = stores[s] },
                    .stencil = { .load_action = loads[l], .store_action = stores[s] },
                },
                .attachments = { .colors[0] = color_view, .depth_stencil = ds_view },
            });
            const metal_mock_render_pass_info_t* pass = metal_mock_last_render_pass();
            T(pass->color_attachments[0].load_action == expected_loads[l]);
            T(pass->color_attachments[0].store_action == expected_stores[s]);
            T(pass->has_depth_attachment);
            T(pass->depth_attachment.load_action == expected_loads[l]);
            T(pass->has_stencil_attachment);
            sg_end_pass();
        }
    }
    // no drawable in an offscreen pass
    T(metal_mock_count_calls(METAL_MOCK_FUNC_presentDrawable) == 0);
    sg_commit();
    teardown();
}

UTEST(sokol_gfx_metal, offscreen_pass_msaa_resolve) {
    setup();
    sg_image msaa_img = sg_make_image(&(sg_image_desc){
        .width = 32, .height = 32, .sample_count = 4,
        .pixel_format = SG_PIXELFORMAT_RGBA8,
        .usage = { .color_attachment = true, .immutable = true },
    });
    sg_image resolve_img = sg_make_image(&(sg_image_desc){
        .width = 32, .height = 32, .sample_count = 1,
        .pixel_format = SG_PIXELFORMAT_RGBA8,
        .usage = { .resolve_attachment = true, .immutable = true },
    });
    sg_view color_view = sg_make_view(&(sg_view_desc){ .color_attachment.image = msaa_img });
    sg_view resolve_view = sg_make_view(&(sg_view_desc){ .resolve_attachment.image = resolve_img });
    sg_begin_pass(&(sg_pass){
        .attachments = { .colors[0] = color_view, .resolves[0] = resolve_view },
    });
    const metal_mock_render_pass_info_t* pass = metal_mock_last_render_pass();
    T(pass->color_attachments[0].texture == sg_mtl_query_image_info(msaa_img).tex[0]);
    T(pass->color_attachments[0].resolve_texture == sg_mtl_query_image_info(resolve_img).tex[0]);
    // default store action is 'store', which resolves and stores
    T(pass->color_attachments[0].store_action == MTLStoreActionStoreAndMultisampleResolve);
    sg_end_pass();
    // ...and 'dontcare' only resolves
    sg_begin_pass(&(sg_pass){
        .attachments = { .colors[0] = color_view, .resolves[0] = resolve_view },
        .action.colors[0].store_action = SG_STOREACTION_DONTCARE,
    });
    pass = metal_mock_last_render_pass();
    T(pass->color_attachments[0].store_action == MTLStoreActionMultisampleResolve);
    sg_end_pass();
    sg_commit();
    teardown();
}

UTEST(sokol_gfx_metal, offscreen_pass_cube_slice) {
    setup();
    sg_image img = sg_make_image(&(sg_image_desc){
        .type = SG_IMAGETYPE_CUBE,
        .width = 16, .height = 16,
        .pixel_format = SG_PIXELFORMAT_RGBA8,
        .usage = { .color_attachment = true, .immutable = true },
    });
    sg_view view = sg_make_view(&(sg_view_desc){
        .color_attachment = { .image = img, .slice = 3 },
    });
    sg_begin_pass(&(sg_pass){ .attachments.colors[0] = view });
    const metal_mock_render_pass_info_t* pass = metal_mock_last_render_pass();
    T(pass->color_attachments[0].slice == 3);
    T(pass->color_attachments[0].depth_plane == 0);
    sg_end_pass();
    sg_commit();
    teardown();
}

UTEST(sokol_gfx_metal, offscreen_pass_3d_depth_plane) {
    setup();
    sg_image img = sg_make_image(&(sg_image_desc){
        .type = SG_IMAGETYPE_3D,
        .width = 16, .height = 16, .num_slices = 4,
        .pixel_format = SG_PIXELFORMAT_RGBA8,
        .usage = { .color_attachment = true, .immutable = true },
    });
    // NOTE: the validation layer only allows slice 0 on 3D attachment views, so
    // depthPlane can never be anything but 0 here. This is a known restriction
    // tracked in https://github.com/floooh/sokol/issues/1302, not a bug to report.
    sg_view view = sg_make_view(&(sg_view_desc){
        .color_attachment = { .image = img },
    });
    sg_begin_pass(&(sg_pass){ .attachments.colors[0] = view });
    const metal_mock_render_pass_info_t* pass = metal_mock_last_render_pass();
    T(pass->color_attachments[0].depth_plane == 0);
    T(pass->color_attachments[0].slice == 0);
    sg_end_pass();
    sg_commit();
    teardown();
}

UTEST(sokol_gfx_metal, offscreen_pass_mrt) {
    setup();
    sg_view views[3];
    for (int i = 0; i < 3; i++) {
        sg_image img = sg_make_image(&(sg_image_desc){
            .width = 16, .height = 16,
            .pixel_format = SG_PIXELFORMAT_RGBA8,
            .usage = { .color_attachment = true, .immutable = true },
        });
        views[i] = sg_make_view(&(sg_view_desc){ .color_attachment.image = img });
    }
    sg_begin_pass(&(sg_pass){
        .attachments.colors = { views[0], views[1], views[2] },
    });
    const metal_mock_render_pass_info_t* pass = metal_mock_last_render_pass();
    T(pass->num_color_attachments == 3);
    T(pass->color_attachments[2].texture != 0);
    sg_end_pass();
    sg_commit();
    teardown();
}

UTEST(sokol_gfx_metal, compute_pass) {
    setup();
    metal_mock_clear_calls();
    sg_begin_pass(&(sg_pass){ .compute = true, .label = "compute-pass" });
    T(metal_mock_count_calls(METAL_MOCK_FUNC_computeCommandEncoder) == 1);
    T(metal_mock_count_calls(METAL_MOCK_FUNC_renderCommandEncoderWithDescriptor) == 0);
    T(sg_mtl_compute_command_encoder() != 0);
    sg_end_pass();
    T(metal_mock_count_calls(METAL_MOCK_FUNC_endEncoding) == 1);
    sg_commit();
    teardown();
}

UTEST(sokol_gfx_metal, compute_pass_encoder_failed) {
    setup();
    metal_mock_fail_next(METAL_MOCK_OBJ_COMPUTE_ENCODER, 1);
    sg_begin_pass(&(sg_pass){ .compute = true });
    T(sg_mtl_compute_command_encoder() == 0);
    sg_end_pass();
    sg_commit();
    teardown();
}

UTEST(sokol_gfx_metal, multiple_frames) {
    setup();
    swapchain_t sc = make_swapchain(64, 32, 1, true);
    for (int i = 0; i < 4; i++) {
        sg_begin_pass(&(sg_pass){ .swapchain = swapchain(&sc) });
        sg_end_pass();
        sg_commit();
    }
    T(metal_mock_count_calls(METAL_MOCK_FUNC_commit) == 4);
    teardown();
    discard_swapchain(&sc);
}

//== render state, bindings, draws =============================================
UTEST(sokol_gfx_metal, apply_viewport_and_scissor) {
    setup();
    swapchain_t sc = make_swapchain(64, 32, 1, true);
    sg_begin_pass(&(sg_pass){ .swapchain = swapchain(&sc) });
    sg_apply_viewport(4, 8, 16, 12, true);
    sg_apply_scissor_rect(2, 4, 8, 6, true);
    const metal_mock_render_encoder_state_t* state = metal_mock_render_encoder_state();
    T(state->viewport.originX == 4.0);
    T(state->viewport.originY == 8.0);
    T(state->viewport.width == 16.0);
    T(state->viewport.height == 12.0);
    T(state->viewport.znear == 0.0);
    T(state->viewport.zfar == 1.0);
    T(state->scissor_rect.x == 2);
    T(state->scissor_rect.y == 4);
    T(state->scissor_rect.width == 8);
    T(state->scissor_rect.height == 6);
    // origin-bottom-left flips the y coordinate
    sg_apply_viewport(4, 8, 16, 12, false);
    T(state->viewport.originY == (32.0 - (8.0 + 12.0)));
    sg_end_pass();
    sg_commit();
    teardown();
    discard_swapchain(&sc);
}

UTEST(sokol_gfx_metal, apply_pipeline) {
    setup();
    swapchain_t sc = make_swapchain(64, 32, 1, true);
    sg_shader shd = make_shader();
    sg_pipeline pip = sg_make_pipeline(&(sg_pipeline_desc){
        .shader = shd,
        .layout.attrs[0].format = SG_VERTEXFORMAT_FLOAT3,
        .colors[0].pixel_format = SG_PIXELFORMAT_BGRA8,
        .cull_mode = SG_CULLMODE_FRONT,
        .face_winding = SG_FACEWINDING_CW,
        .depth = { .pixel_format = SG_PIXELFORMAT_DEPTH_STENCIL, .bias = 1.0f, .bias_slope_scale = 2.0f, .bias_clamp = 3.0f },
        .stencil = { .enabled = true, .ref = 0x7F },
        .blend_color = { 0.1f, 0.2f, 0.3f, 0.4f },
    });
    sg_begin_pass(&(sg_pass){ .swapchain = swapchain(&sc) });
    sg_apply_pipeline(pip);
    const metal_mock_render_encoder_state_t* state = metal_mock_render_encoder_state();
    T(state->pipeline_state == sg_mtl_query_pipeline_info(pip).rps);
    T(state->depth_stencil_state == sg_mtl_query_pipeline_info(pip).dss);
    T(state->cull_mode == MTLCullModeFront);
    T(state->winding == MTLWindingClockwise);
    T(state->stencil_ref == 0x7F);
    T(state->blend_color[0] == 0.1f);
    T(state->depth_bias == 1.0f);
    T(state->depth_bias_slope_scale == 2.0f);
    T(state->depth_bias_clamp == 3.0f);
    // re-applying the same pipeline is filtered by the frontend state cache
    const int num_calls = metal_mock_count_calls(METAL_MOCK_FUNC_setRenderPipelineState);
    sg_apply_pipeline(pip);
    T(metal_mock_count_calls(METAL_MOCK_FUNC_setRenderPipelineState) == num_calls);
    sg_end_pass();
    sg_commit();
    teardown();
    discard_swapchain(&sc);
}

UTEST(sokol_gfx_metal, apply_bindings_vertex_and_index_buffers) {
    setup();
    swapchain_t sc = make_swapchain(64, 32, 1, true);
    sg_shader shd = make_shader();
    sg_pipeline pip = sg_make_pipeline(&(sg_pipeline_desc){
        .shader = shd,
        .layout = {
            .buffers = { [0].stride = 12, [1].stride = 16 },
            .attrs = {
                [0] = { .format = SG_VERTEXFORMAT_FLOAT3, .buffer_index = 0 },
                [1] = { .format = SG_VERTEXFORMAT_FLOAT4, .buffer_index = 1 },
            },
        },
        .index_type = SG_INDEXTYPE_UINT16,
        .depth.pixel_format = SG_PIXELFORMAT_DEPTH_STENCIL,
        .colors[0].pixel_format = SG_PIXELFORMAT_BGRA8,
    });
    sg_buffer vbuf0 = make_vbuf();
    sg_buffer vbuf1 = make_vbuf();
    sg_buffer ibuf = sg_make_buffer(&(sg_buffer_desc){
        .usage = { .index_buffer = true, .immutable = true },
        .data = { .ptr = scratch, .size = 64 },
    });
    sg_begin_pass(&(sg_pass){ .swapchain = swapchain(&sc) });
    sg_apply_pipeline(pip);
    metal_mock_clear_calls();
    sg_apply_bindings(&(sg_bindings){
        .vertex_buffers = { vbuf0, vbuf1 },
        .vertex_buffer_offsets = { 0, 16 },
        .index_buffer = ibuf,
    });
    const metal_mock_render_encoder_state_t* state = metal_mock_render_encoder_state();
    T(state->vertex_buffers[MTL_VB_SLOT(0)].buffer == sg_mtl_query_buffer_info(vbuf0).buf[0]);
    T(state->vertex_buffers[MTL_VB_SLOT(0)].offset == 0);
    T(state->vertex_buffers[MTL_VB_SLOT(1)].buffer == sg_mtl_query_buffer_info(vbuf1).buf[0]);
    T(state->vertex_buffers[MTL_VB_SLOT(1)].offset == 16);
    T(metal_mock_count_calls(METAL_MOCK_FUNC_setVertexBuffer) == 2);
    // same buffers, new offsets: only the offsets are updated
    metal_mock_clear_calls();
    sg_apply_bindings(&(sg_bindings){
        .vertex_buffers = { vbuf0, vbuf1 },
        .vertex_buffer_offsets = { 8, 32 },
        .index_buffer = ibuf,
    });
    T(metal_mock_count_calls(METAL_MOCK_FUNC_setVertexBuffer) == 0);
    T(metal_mock_count_calls(METAL_MOCK_FUNC_setVertexBufferOffset) == 2);
    T(state->vertex_buffers[MTL_VB_SLOT(0)].offset == 8);
    T(state->vertex_buffers[MTL_VB_SLOT(1)].offset == 32);
    sg_end_pass();
    sg_commit();
    teardown();
    discard_swapchain(&sc);
}

UTEST(sokol_gfx_metal, apply_bindings_textures_and_samplers) {
    setup();
    swapchain_t sc = make_swapchain(64, 32, 1, true);
    sg_shader shd = sg_make_shader(&(sg_shader_desc){
        .vertex_func.source = "vs",
        .fragment_func.source = "fs",
        .views = {
            [0].texture = {
                .stage = SG_SHADERSTAGE_VERTEX,
                .image_type = SG_IMAGETYPE_2D,
                .sample_type = SG_IMAGESAMPLETYPE_FLOAT,
                .msl_texture_n = 0,
            },
            [1].texture = {
                .stage = SG_SHADERSTAGE_FRAGMENT,
                .image_type = SG_IMAGETYPE_2D,
                .sample_type = SG_IMAGESAMPLETYPE_FLOAT,
                .msl_texture_n = 1,
            },
        },
        .samplers = {
            [0] = { .stage = SG_SHADERSTAGE_VERTEX, .sampler_type = SG_SAMPLERTYPE_FILTERING, .msl_sampler_n = 0 },
            [1] = { .stage = SG_SHADERSTAGE_FRAGMENT, .sampler_type = SG_SAMPLERTYPE_FILTERING, .msl_sampler_n = 1 },
        },
        .texture_sampler_pairs = {
            [0] = { .stage = SG_SHADERSTAGE_VERTEX, .view_slot = 0, .sampler_slot = 0 },
            [1] = { .stage = SG_SHADERSTAGE_FRAGMENT, .view_slot = 1, .sampler_slot = 1 },
        },
    });
    sg_pipeline pip = make_pipeline(shd);
    sg_image img = make_2d_image();
    sg_view view0 = make_texture_view(img);
    sg_view view1 = make_texture_view(img);
    sg_sampler smp = sg_make_sampler(&(sg_sampler_desc){
        .min_filter = SG_FILTER_LINEAR, .mag_filter = SG_FILTER_LINEAR,
    });
    sg_begin_pass(&(sg_pass){ .swapchain = swapchain(&sc) });
    sg_apply_pipeline(pip);
    metal_mock_clear_calls();
    sg_apply_bindings(&(sg_bindings){
        .vertex_buffers[0] = make_vbuf(),
        .views = { view0, view1 },
        .samplers = { smp, smp },
    });
    const metal_mock_render_encoder_state_t* state = metal_mock_render_encoder_state();
    T(state->vertex_textures[0] != 0);
    T(state->fragment_textures[1] != 0);
    T(state->vertex_samplers[0] == sg_mtl_query_sampler_info(smp).smp);
    T(state->fragment_samplers[1] == sg_mtl_query_sampler_info(smp).smp);
    T(metal_mock_count_calls(METAL_MOCK_FUNC_setVertexTexture) == 1);
    T(metal_mock_count_calls(METAL_MOCK_FUNC_setFragmentTexture) == 1);
    T(metal_mock_count_calls(METAL_MOCK_FUNC_setVertexSamplerState) == 1);
    T(metal_mock_count_calls(METAL_MOCK_FUNC_setFragmentSamplerState) == 1);
    // identical bindings are filtered by the Metal backend's binding cache
    metal_mock_clear_calls();
    sg_apply_bindings(&(sg_bindings){
        .vertex_buffers[0] = make_vbuf(),
        .views = { view0, view1 },
        .samplers = { smp, smp },
    });
    T(metal_mock_count_calls(METAL_MOCK_FUNC_setVertexTexture) == 0);
    T(metal_mock_count_calls(METAL_MOCK_FUNC_setFragmentSamplerState) == 0);
    sg_end_pass();
    sg_commit();
    teardown();
    discard_swapchain(&sc);
}

UTEST(sokol_gfx_metal, apply_bindings_storage_buffer) {
    setup();
    swapchain_t sc = make_swapchain(64, 32, 1, true);
    sg_shader shd = sg_make_shader(&(sg_shader_desc){
        .vertex_func.source = "vs",
        .fragment_func.source = "fs",
        .views[0].storage_buffer = {
            .stage = SG_SHADERSTAGE_VERTEX, .readonly = true, .msl_buffer_n = 8,
        },
    });
    sg_pipeline pip = make_pipeline(shd);
    sg_buffer sbuf = sg_make_buffer(&(sg_buffer_desc){
        .usage = { .storage_buffer = true, .immutable = true },
        .data = { .ptr = scratch, .size = 256 },
    });
    sg_view sview = sg_make_view(&(sg_view_desc){ .storage_buffer.buffer = sbuf });
    sg_begin_pass(&(sg_pass){ .swapchain = swapchain(&sc) });
    sg_apply_pipeline(pip);
    metal_mock_clear_calls();
    sg_apply_bindings(&(sg_bindings){
        .vertex_buffers[0] = make_vbuf(),
        .views[0] = sview,
    });
    const metal_mock_render_encoder_state_t* state = metal_mock_render_encoder_state();
    T(state->vertex_buffers[8].buffer == sg_mtl_query_buffer_info(sbuf).buf[0]);
    sg_end_pass();
    sg_commit();
    teardown();
    discard_swapchain(&sc);
}

UTEST(sokol_gfx_metal, apply_uniforms) {
    setup();
    swapchain_t sc = make_swapchain(64, 32, 1, true);
    sg_shader shd = sg_make_shader(&(sg_shader_desc){
        .vertex_func.source = "vs",
        .fragment_func.source = "fs",
        .uniform_blocks = {
            [0] = { .stage = SG_SHADERSTAGE_VERTEX, .size = 64, .msl_buffer_n = 0 },
            [1] = { .stage = SG_SHADERSTAGE_FRAGMENT, .size = 32, .msl_buffer_n = 1 },
        },
    });
    sg_pipeline pip = make_pipeline(shd);
    sg_begin_pass(&(sg_pass){ .swapchain = swapchain(&sc) });
    // the uniform buffer is bound to all uniform block slots at pass start
    T(metal_mock_count_calls(METAL_MOCK_FUNC_setVertexBuffer) >= 8);
    T(metal_mock_count_calls(METAL_MOCK_FUNC_setFragmentBuffer) >= 8);
    sg_apply_pipeline(pip);
    sg_apply_bindings(&(sg_bindings){ .vertex_buffers[0] = make_vbuf() });
    metal_mock_clear_calls();
    sg_apply_uniforms(0, &(sg_range){ .ptr = scratch, .size = 64 });
    sg_apply_uniforms(1, &(sg_range){ .ptr = scratch, .size = 32 });
    T(metal_mock_count_calls(METAL_MOCK_FUNC_setVertexBufferOffset) == 1);
    T(metal_mock_count_calls(METAL_MOCK_FUNC_setFragmentBufferOffset) == 1);
    const metal_mock_call_t* vs_call = metal_mock_last_call(METAL_MOCK_FUNC_setVertexBufferOffset);
    T(vs_call->args[1].u == 0);     // uniform block 0 -> MSL buffer slot 0
    const metal_mock_call_t* fs_call = metal_mock_last_call(METAL_MOCK_FUNC_setFragmentBufferOffset);
    T(fs_call->args[1].u == 1);     // uniform block 1 -> MSL buffer slot 1
    T(fs_call->args[0].u >= 64);    // second block is behind the first, 256-byte aligned
    sg_end_pass();
    sg_commit();
    teardown();
    discard_swapchain(&sc);
}

UTEST(sokol_gfx_metal, draw_variants) {
    setup();
    swapchain_t sc = make_swapchain(64, 32, 1, true);
    sg_shader shd = make_shader();
    sg_pipeline pip = sg_make_pipeline(&(sg_pipeline_desc){
        .shader = shd,
        .layout.attrs[0].format = SG_VERTEXFORMAT_FLOAT3,
        .primitive_type = SG_PRIMITIVETYPE_TRIANGLE_STRIP,
        .depth.pixel_format = SG_PIXELFORMAT_DEPTH_STENCIL,
        .colors[0].pixel_format = SG_PIXELFORMAT_BGRA8,
    });
    sg_begin_pass(&(sg_pass){ .swapchain = swapchain(&sc) });
    sg_apply_pipeline(pip);
    sg_apply_bindings(&(sg_bindings){ .vertex_buffers[0] = make_vbuf() });
    metal_mock_clear_calls();
    // non-indexed
    sg_draw(0, 3, 1);
    const metal_mock_call_t* call = metal_mock_last_call(METAL_MOCK_FUNC_drawPrimitives);
    T(call != 0);
    T(call->args[0].u == MTLPrimitiveTypeTriangleStrip);
    T(call->args[1].u == 0);
    T(call->args[2].u == 3);
    T(call->args[3].u == 1);
    // non-indexed with base instance
    sg_draw_ex(0, 3, 2, 0, 1);
    call = metal_mock_last_call(METAL_MOCK_FUNC_drawPrimitivesBaseInstance);
    T(call != 0);
    T(call->args[3].u == 2);
    T(call->args[4].u == 1);
    // zero-element draw is a no-op
    const int num_draws = metal_mock_count_calls(METAL_MOCK_FUNC_drawPrimitives);
    sg_draw(0, 0, 1);
    T(metal_mock_count_calls(METAL_MOCK_FUNC_drawPrimitives) == num_draws);
    sg_end_pass();
    sg_commit();
    teardown();
    discard_swapchain(&sc);
}

UTEST(sokol_gfx_metal, draw_indexed) {
    setup();
    swapchain_t sc = make_swapchain(64, 32, 1, true);
    sg_shader shd = make_shader();
    sg_pipeline pip = sg_make_pipeline(&(sg_pipeline_desc){
        .shader = shd,
        .layout.attrs[0].format = SG_VERTEXFORMAT_FLOAT3,
        .index_type = SG_INDEXTYPE_UINT16,
        .depth.pixel_format = SG_PIXELFORMAT_DEPTH_STENCIL,
        .colors[0].pixel_format = SG_PIXELFORMAT_BGRA8,
    });
    sg_buffer ibuf = sg_make_buffer(&(sg_buffer_desc){
        .usage = { .index_buffer = true, .immutable = true },
        .data = { .ptr = scratch, .size = 64 },
    });
    sg_begin_pass(&(sg_pass){ .swapchain = swapchain(&sc) });
    sg_apply_pipeline(pip);
    sg_apply_bindings(&(sg_bindings){
        .vertex_buffers[0] = make_vbuf(),
        .index_buffer = ibuf,
        .index_buffer_offset = 8,
    });
    metal_mock_clear_calls();
    sg_draw(3, 6, 1);
    const metal_mock_call_t* call = metal_mock_last_call(METAL_MOCK_FUNC_drawIndexedPrimitives);
    T(call != 0);
    T(call->args[1].u == 6);
    T(call->args[2].u == MTLIndexTypeUInt16);
    T(call->args[3].p == sg_mtl_query_buffer_info(ibuf).buf[0]);
    T(call->args[4].u == (8 + 3 * 2));
    T(call->args[5].u == 1);
    // base vertex / base instance
    sg_draw_ex(3, 6, 2, 4, 5);
    call = metal_mock_last_call(METAL_MOCK_FUNC_drawIndexedPrimitivesBaseVertex);
    T(call != 0);
    T(call->args[6].i == 4);
    T(call->args[7].u == 5);
    sg_end_pass();
    sg_commit();
    teardown();
    discard_swapchain(&sc);
}

UTEST(sokol_gfx_metal, dispatch) {
    setup();
    sg_shader shd = sg_make_shader(&(sg_shader_desc){
        .compute_func.source = "cs",
        .mtl_threads_per_threadgroup = { .x = 8, .y = 4, .z = 1 },
        .views[0].storage_image = {
            .stage = SG_SHADERSTAGE_COMPUTE,
            .image_type = SG_IMAGETYPE_2D,
            .access_format = SG_PIXELFORMAT_RGBA8,
            .writeonly = true,
            .msl_texture_n = 0,
        },
    });
    sg_pipeline pip = sg_make_pipeline(&(sg_pipeline_desc){ .compute = true, .shader = shd });
    sg_image img = sg_make_image(&(sg_image_desc){
        .width = 16, .height = 16,
        .pixel_format = SG_PIXELFORMAT_RGBA8,
        .usage = { .storage_image = true, .immutable = true },
    });
    sg_view view = sg_make_view(&(sg_view_desc){ .storage_image.image = img });
    sg_begin_pass(&(sg_pass){ .compute = true });
    sg_apply_pipeline(pip);
    sg_apply_bindings(&(sg_bindings){ .views[0] = view });
    const metal_mock_compute_encoder_state_t* state = metal_mock_compute_encoder_state();
    T(metal_mock_is_object(METAL_MOCK_OBJ_COMPUTE_PIPELINE, state->pipeline_state));
    T(state->textures[0] != 0);
    sg_dispatch(2, 3, 1);
    T(state->num_dispatches == 1);
    T(state->last_threadgroups_per_grid.width == 2);
    T(state->last_threadgroups_per_grid.height == 3);
    T(state->last_threads_per_threadgroup.width == 8);
    T(state->last_threads_per_threadgroup.height == 4);
    sg_end_pass();
    sg_commit();
    teardown();
}

UTEST(sokol_gfx_metal, debug_groups) {
    setup();
    swapchain_t sc = make_swapchain(64, 32, 1, true);
    // outside a pass the debug group calls are dropped
    sg_push_debug_group("outside");
    sg_pop_debug_group();
    T(metal_mock_count_calls(METAL_MOCK_FUNC_pushDebugGroup) == 0);
    sg_begin_pass(&(sg_pass){ .swapchain = swapchain(&sc) });
    sg_push_debug_group("render-group");
    const metal_mock_call_t* call = metal_mock_last_call(METAL_MOCK_FUNC_pushDebugGroup);
    T(call != 0);
    T(0 == strcmp((const char*)call->args[0].p, "render-group"));
    T(metal_mock_render_encoder_state()->debug_group_depth == 1);
    sg_pop_debug_group();
    T(metal_mock_render_encoder_state()->debug_group_depth == 0);
    sg_end_pass();
    sg_commit();
    // the logged string is a copy and survives a pool drain
    metal_mock_drain_pool();
    call = metal_mock_last_call(METAL_MOCK_FUNC_pushDebugGroup);
    T(0 == strcmp((const char*)call->args[0].p, "render-group"));
    // and in a compute pass
    sg_begin_pass(&(sg_pass){ .compute = true });
    sg_push_debug_group("compute-group");
    T(metal_mock_compute_encoder_state()->debug_group_depth == 1);
    sg_pop_debug_group();
    sg_end_pass();
    sg_commit();
    teardown();
    discard_swapchain(&sc);
}

//== leak check ================================================================
UTEST(sokol_gfx_metal, no_leaks_full_lifecycle) {
    setup();
    swapchain_t sc = make_swapchain(64, 32, 1, true);
    sg_shader shd = make_shader();
    sg_pipeline pip = make_pipeline(shd);
    sg_buffer vbuf = make_vbuf();
    sg_image img = make_2d_image();
    sg_view view = make_texture_view(img);
    sg_sampler smp = sg_make_sampler(&(sg_sampler_desc){0});
    for (int i = 0; i < 3; i++) {
        sg_begin_pass(&(sg_pass){ .swapchain = swapchain(&sc) });
        sg_apply_pipeline(pip);
        sg_apply_bindings(&(sg_bindings){ .vertex_buffers[0] = vbuf });
        sg_draw(0, 3, 1);
        sg_end_pass();
        sg_commit();
    }
    sg_destroy_view(view);
    sg_destroy_sampler(smp);
    sg_destroy_image(img);
    sg_destroy_buffer(vbuf);
    sg_destroy_pipeline(pip);
    sg_destroy_shader(shd);
    teardown();
    discard_swapchain(&sc);
    T(metal_mock_live_objects_total() == 0);
}

UTEST_MAIN()
