//------------------------------------------------------------------------------
//  LLM maintained.
//
//  sokol_gfx_wgpu_test.c
//  Tests for the sokol_gfx.h WebGPU backend, run against the WebGPU mock in
//  tests/mocks/wgpu. The tests aim for code coverage of the _sg_wgpu_*
//  functions, assertions concentrate on the observable WebGPU API effects.
//  Every test must end without validation messages from the mock (see
//  teardown()), unless a test expects them and clears them before.
//
//  Rough coverage map (referenced sokol_gfx.h symbol on the right):
//    - backend setup, caps, limits         _sg_wgpu_setup_backend / _sg_wgpu_init_caps
//    - buffer creation and updates         _sg_wgpu_create_buffer / _sg_wgpu_copy_buffer_data
//    - image creation                      _sg_wgpu_create_image / _sg_wgpu_texture_format
//    - sampler creation                    _sg_wgpu_create_sampler
//    - shader creation                     _sg_wgpu_create_shader / _sg_wgpu_ensure_wgsl_bindslot_ranges
//    - pipeline creation                   _sg_wgpu_create_pipeline
//    - views                               _sg_wgpu_create_view
//    - passes                              _sg_wgpu_begin_pass / _sg_wgpu_end_pass
//    - viewport and scissor                _sg_wgpu_apply_viewport / _sg_wgpu_apply_scissor_rect
//    - bindings and bindgroups cache       _sg_wgpu_apply_bindings / _sg_wgpu_bindgroups_cache_*
//    - uniforms                            _sg_wgpu_uniform_system_*
//    - draws and dispatches                _sg_wgpu_draw / _sg_wgpu_dispatch
//    - copies                              _sg_wgpu_copy_buffer_to_buffer / _sg_wgpu_copy_*_to_image
//    - frame end                           _sg_wgpu_commit
//------------------------------------------------------------------------------
#define SOKOL_IMPL
#include "wgpu_mock.h"
#include "sokol_gfx.h"
#include "utest.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#define T(b) EXPECT_TRUE(b)
// debug labels are only set in debug builds
#if defined(SOKOL_DEBUG)
#define T_LABEL(v, s) T(0 == strcmp(v, s))
#else
#define T_LABEL(v, s) do { (void)(v); (void)(s); } while (0)
#endif

#define F(name) WGPU_MOCK_FUNC_##name

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
    if (getenv("WGPU_TEST_LOG")) {
        printf("sokol log: %s\n", msg);
    }
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
    bool disable_bindgroups_cache;
    int bindgroups_cache_size;
    bool disable_validation;
    int uniform_buffer_size;
    bool all_features;
} setup_desc_t;

static void setup_with(setup_desc_t s) {
    reset_log();
    wgpu_mock_setup();
    if (s.all_features) {
        wgpu_mock_set_feature(WGPUFeatureName_TextureCompressionBC, true);
        wgpu_mock_set_feature(WGPUFeatureName_TextureCompressionETC2, true);
        wgpu_mock_set_feature(WGPUFeatureName_TextureCompressionASTC, true);
        wgpu_mock_set_feature(WGPUFeatureName_Float32Filterable, true);
        wgpu_mock_set_feature(WGPUFeatureName_Float32Blendable, true);
        wgpu_mock_set_feature(WGPUFeatureName_TextureFormatsTier1, true);
        wgpu_mock_set_feature(WGPUFeatureName_TextureFormatsTier2, true);
        wgpu_mock_set_feature(WGPUFeatureName_DualSourceBlending, true);
    }
    sg_desc desc = {
        .environment.defaults.color_format = SG_PIXELFORMAT_BGRA8,
        .environment.wgpu.device = wgpu_mock_device(),
        .wgpu = {
            .disable_bindgroups_cache = s.disable_bindgroups_cache,
            .bindgroups_cache_size = s.bindgroups_cache_size,
        },
        .uniform_buffer_size = s.uniform_buffer_size,
        .disable_validation = s.disable_validation,
        .logger.func = capture_log,
    };
    sg_setup(&desc);
}

static void setup(void) {
    setup_with((setup_desc_t){0});
}

// teardown() is a macro so the checks can report through utest
#define teardown() teardown_impl(utest_result)
static void teardown_impl(int* utest_result) {
    sg_shutdown();
    // a truncated call log makes the call counts of a test meaningless
    T(!wgpu_mock_call_log_overflow());
    // sokol-gfx must not violate any WebGPU validation rule
    if (wgpu_mock_total_errors() != 0) {
        for (int i = 0; i < wgpu_mock_num_errors(); i++) {
            printf("wgpu mock error: %s\n", wgpu_mock_error(i));
        }
    }
    T(wgpu_mock_total_errors() == 0);
    // sokol-gfx must release everything it created or added a reference to
    T(wgpu_mock_live_objects_total() == 0);
    T(wgpu_mock_ref_count(wgpu_mock_queue()) == 1);
    wgpu_mock_shutdown();
}

//== helpers ===================================================================
static uint8_t scratch[64 * 64 * 16];

typedef struct {
    int width, height, sample_count;
    sg_pixel_format depth_format;
    WGPUTextureView render_view;
    WGPUTextureView resolve_view;
    WGPUTextureView depth_view;
} swapchain_t;

static swapchain_t make_swapchain(int w, int h, int sample_count, bool with_depth) {
    swapchain_t sc = {
        .width = w,
        .height = h,
        .sample_count = sample_count,
        .depth_format = with_depth ? SG_PIXELFORMAT_DEPTH_STENCIL : SG_PIXELFORMAT_NONE,
    };
    sc.render_view = wgpu_mock_create_view(w, h, WGPUTextureFormat_BGRA8Unorm, sample_count);
    if (sample_count > 1) {
        sc.resolve_view = wgpu_mock_create_view(w, h, WGPUTextureFormat_BGRA8Unorm, 1);
    }
    if (with_depth) {
        sc.depth_view = wgpu_mock_create_view(w, h, WGPUTextureFormat_Depth32FloatStencil8, sample_count);
    }
    return sc;
}

static void discard_swapchain(swapchain_t* sc) {
    if (sc->render_view) { wgpuTextureViewRelease(sc->render_view); }
    if (sc->resolve_view) { wgpuTextureViewRelease(sc->resolve_view); }
    if (sc->depth_view) { wgpuTextureViewRelease(sc->depth_view); }
    memset(sc, 0, sizeof(*sc));
}

static sg_swapchain swapchain(const swapchain_t* sc) {
    return (sg_swapchain){
        .width = sc->width,
        .height = sc->height,
        .sample_count = sc->sample_count,
        .color_format = SG_PIXELFORMAT_BGRA8,
        .depth_format = sc->depth_format,
        .wgpu = {
            .render_view = sc->render_view,
            .resolve_view = sc->resolve_view,
            .depth_stencil_view = sc->depth_view,
        },
    };
}

static const char* vs_src = "@vertex fn vs_main() -> @builtin(position) vec4f { return vec4f(0); }";
static const char* fs_src = "@fragment fn fs_main() -> @location(0) vec4f { return vec4f(1); }";
static const char* cs_src = "@compute @workgroup_size(1) fn cs_main() { }";

static sg_shader make_shader(void) {
    return sg_make_shader(&(sg_shader_desc){
        .vertex_func = { .source = vs_src, .entry = "vs_main" },
        .fragment_func = { .source = fs_src, .entry = "fs_main" },
        .label = "shd",
    });
}

static sg_shader make_compute_shader(void) {
    return sg_make_shader(&(sg_shader_desc){
        .compute_func = { .source = cs_src, .entry = "cs_main" },
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
        .usage.vertex_buffer = true,
        .data = { .ptr = scratch, .size = 128 },
        .label = "vbuf",
    });
}

static sg_buffer make_ibuf(void) {
    return sg_make_buffer(&(sg_buffer_desc){
        .usage.index_buffer = true,
        .data = { .ptr = scratch, .size = 64 },
        .label = "ibuf",
    });
}

static sg_image make_2d_image(void) {
    return sg_make_image(&(sg_image_desc){
        .width = 16,
        .height = 16,
        .pixel_format = SG_PIXELFORMAT_RGBA8,
        .data = { .ptr = scratch, .size = 16 * 16 * 4 },
        .label = "img",
    });
}

static sg_view make_texture_view(sg_image img) {
    return sg_make_view(&(sg_view_desc){ .texture.image = img, .label = "texview" });
}

//== backend setup, caps and limits ============================================
UTEST(sokol_gfx_wgpu, setup_shutdown) {
    setup();
    T(sg_isvalid());
    T(sg_query_backend() == SG_BACKEND_WGPU);
    T(wgpu_mock_count_calls(F(wgpuDeviceGetQueue)) == 1);
    T(wgpu_mock_count_calls(F(wgpuDeviceGetLimits)) == 1);
    T(sg_wgpu_device() == wgpu_mock_device());
    T(sg_wgpu_queue() == wgpu_mock_queue());
    T(sg_wgpu_command_encoder() == 0);
    T(sg_wgpu_render_pass_encoder() == 0);
    T(sg_wgpu_compute_pass_encoder() == 0);
    teardown();
    T(!sg_isvalid());
}

UTEST(sokol_gfx_wgpu, init_caps) {
    setup();
    const sg_features f = sg_query_features();
    T(f.origin_top_left);
    T(!f.image_clamp_to_border);
    T(f.mrt_independent_blend_state);
    T(f.mrt_independent_write_mask);
    T(f.compute);
    T(f.msaa_texture_bindings);
    T(f.draw_base_vertex);
    T(f.draw_base_instance);
    T(!f.dual_source_blending);
    T(!f.vertexformat_int10_n2);
    T(f.copy_buffer_to_image_relaxed_buffer_type);
    T(!f.copy_buffer_to_image_relaxed_bytes_per_row);
    T(f.copy_image_to_image_relaxed_pixel_format);
    T(!f.separate_buffer_types);
    const sg_limits l = sg_query_limits();
    T(l.max_image_size_2d == 8192);
    T(l.max_image_size_cube == 8192);
    T(l.max_image_size_3d == 2048);
    T(l.max_image_array_layers == 256);
    T(l.max_color_attachments == SG_MAX_COLOR_ATTACHMENTS);
    T(l.max_vertex_attrs == SG_MAX_VERTEX_ATTRIBUTES);
    T(l.max_texture_bindings_per_stage > 0);
    T(sg_query_pixelformat(SG_PIXELFORMAT_BGRA8).render);
    T(sg_query_pixelformat(SG_PIXELFORMAT_DEPTH_STENCIL).depth);
    teardown();
}

UTEST(sokol_gfx_wgpu, init_caps_dual_source_blending) {
    wgpu_mock_setup();
    wgpu_mock_set_feature(WGPUFeatureName_DualSourceBlending, true);
    sg_setup(&(sg_desc){ .environment.defaults.color_format = SG_PIXELFORMAT_BGRA8, .environment.wgpu.device = wgpu_mock_device(), .logger.func = capture_log });
    T(sg_query_features().dual_source_blending);
    teardown();
}

UTEST(sokol_gfx_wgpu, init_caps_texture_formats_tier2) {
    setup();
    T(!sg_query_pixelformat(SG_PIXELFORMAT_R8).write);
    T(sg_query_pixelformat(SG_PIXELFORMAT_RGBA8).write);
    sg_shutdown();
    T(wgpu_mock_total_errors() == 0);
    wgpu_mock_set_feature(WGPUFeatureName_TextureFormatsTier2, true);
    sg_setup(&(sg_desc){ .environment.defaults.color_format = SG_PIXELFORMAT_BGRA8, .environment.wgpu.device = wgpu_mock_device(), .logger.func = capture_log });
    T(sg_query_pixelformat(SG_PIXELFORMAT_R8).write);
    T(sg_query_pixelformat(SG_PIXELFORMAT_R16F).write);
    teardown();
}

UTEST(sokol_gfx_wgpu, bad_bindgroups_cache_size) {
    setup_with((setup_desc_t){ .bindgroups_cache_size = 1 });
    T(logged(SG_LOGITEM_WGPU_BINDGROUPSCACHE_SIZE_GREATER_ONE));
    teardown();
    setup_with((setup_desc_t){ .bindgroups_cache_size = 100 });
    T(logged(SG_LOGITEM_WGPU_BINDGROUPSCACHE_SIZE_POW2));
    teardown();
}

//== buffers ===================================================================
UTEST(sokol_gfx_wgpu, create_buffer_with_data) {
    setup();
    sg_buffer buf = make_vbuf();
    T(sg_query_buffer_state(buf) == SG_RESOURCESTATE_VALID);
    WGPUBuffer wb = (WGPUBuffer)sg_wgpu_query_buffer_info(buf).buf;
    wgpu_mock_buffer_info_t info = {0};
    T(wgpu_mock_buffer_info(wb, &info));
    T(info.size == 128);
    T(info.usage & WGPUBufferUsage_Vertex);
    T(info.mapped_at_creation);
    T(!info.mapped);
    T(info.num_unmaps == 1);
    T_LABEL(info.label, "vbuf");
    sg_destroy_buffer(buf);
    T(!wgpu_mock_is_alive(WGPU_MOCK_OBJ_BUFFER, wb));
    teardown();
}

UTEST(sokol_gfx_wgpu, create_buffer_usages) {
    setup();
    sg_buffer vb = sg_make_buffer(&(sg_buffer_desc){ .usage = { .vertex_buffer = true, .copy_dst = true }, .size = 64 });
    sg_buffer ib = sg_make_buffer(&(sg_buffer_desc){ .usage = { .index_buffer = true, .write_transient = true }, .size = 64 });
    sg_buffer sb = sg_make_buffer(&(sg_buffer_desc){ .usage = { .storage_buffer = true }, .size = 64 });
    wgpu_mock_buffer_info_t info = {0};
    T(wgpu_mock_buffer_info(sg_wgpu_query_buffer_info(vb).buf, &info));
    T(info.usage & WGPUBufferUsage_CopyDst);
    T(wgpu_mock_buffer_info(sg_wgpu_query_buffer_info(ib).buf, &info));
    T(info.usage & WGPUBufferUsage_Index);
    T(wgpu_mock_buffer_info(sg_wgpu_query_buffer_info(sb).buf, &info));
    T(info.usage & WGPUBufferUsage_Storage);
    sg_destroy_buffer(vb);
    sg_destroy_buffer(ib);
    sg_destroy_buffer(sb);
    teardown();
}

UTEST(sokol_gfx_wgpu, create_buffer_odd_size) {
    setup();
    sg_buffer buf = sg_make_buffer(&(sg_buffer_desc){
        .usage.vertex_buffer = true,
        .data = { .ptr = scratch, .size = 13 },
    });
    T(sg_query_buffer_state(buf) == SG_RESOURCESTATE_VALID);
    wgpu_mock_buffer_info_t info = {0};
    T(wgpu_mock_buffer_info(sg_wgpu_query_buffer_info(buf).buf, &info));
    T(info.size == 16);
    sg_destroy_buffer(buf);
    teardown();
}

UTEST(sokol_gfx_wgpu, create_buffer_fail) {
    setup();
    wgpu_mock_fail_next(WGPU_MOCK_OBJ_BUFFER, 1);
    sg_buffer buf = make_vbuf();
    T(sg_query_buffer_state(buf) == SG_RESOURCESTATE_FAILED);
    T(logged(SG_LOGITEM_WGPU_CREATE_BUFFER_FAILED));
    sg_destroy_buffer(buf);
    teardown();
}

UTEST(sokol_gfx_wgpu, create_buffer_unsealed) {
    setup();
    sg_buffer buf = sg_make_buffer(&(sg_buffer_desc){
        .usage = { .vertex_buffer = true, .write_unsealed = true },
        .size = 64,
    });
    wgpu_mock_buffer_info_t info = {0};
    T(wgpu_mock_buffer_info(sg_wgpu_query_buffer_info(buf).buf, &info));
    T(info.mapped);
    sg_write_buffer_unsealed(&(sg_write_buffer_desc){ .src.data = { .ptr = scratch, .size = 32 }, .dst.buffer = buf });
    sg_seal_buffer(buf);
    T(wgpu_mock_buffer_info(sg_wgpu_query_buffer_info(buf).buf, &info));
    T(!info.mapped);
    sg_destroy_buffer(buf);
    teardown();
}

UTEST(sokol_gfx_wgpu, discard_unsealed_buffer) {
    setup();
    sg_buffer buf = sg_make_buffer(&(sg_buffer_desc){
        .usage = { .vertex_buffer = true, .write_unsealed = true },
        .size = 64,
    });
    sg_destroy_buffer(buf);
    teardown();
}

UTEST(sokol_gfx_wgpu, create_buffer_injected) {
    setup();
    WGPUBufferDescriptor bd = {
        .usage = WGPUBufferUsage_Vertex | WGPUBufferUsage_CopyDst,
        .size = 64,
    };
    WGPUBuffer wb = wgpuDeviceCreateBuffer(wgpu_mock_device(), &bd);
    sg_buffer buf = sg_make_buffer(&(sg_buffer_desc){
        .usage.vertex_buffer = true,
        .size = 64,
        .wgpu_buffer = wb,
    });
    T(sg_wgpu_query_buffer_info(buf).buf == wb);
    T(wgpu_mock_ref_count(wb) == 2);
    sg_destroy_buffer(buf);
    T(wgpu_mock_ref_count(wb) == 1);
    wgpuBufferRelease(wb);
    teardown();
}

UTEST(sokol_gfx_wgpu, staging_buffer) {
    setup();
    sg_buffer buf = sg_make_buffer(&(sg_buffer_desc){
        .usage = { .staging_buffer = true, .copy_src = true, .write_transient = true },
        .size = 64,
    });
    T(sg_query_buffer_state(buf) == SG_RESOURCESTATE_VALID);
    // staging buffers have no WebGPU buffer
    T(sg_wgpu_query_buffer_info(buf).buf == 0);
    T(wgpu_mock_live_objects(WGPU_MOCK_OBJ_BUFFER) == 1);  // only the uniform buffer
    sg_destroy_buffer(buf);
    teardown();
}

UTEST(sokol_gfx_wgpu, write_buffer_transient) {
    setup();
    sg_buffer buf = sg_make_buffer(&(sg_buffer_desc){
        .usage = { .vertex_buffer = true, .write_transient = true },
        .size = 64,
    });
    for (int i = 0; i < 64; i++) {
        scratch[i] = (uint8_t)(i + 1);
    }
    wgpu_mock_clear_calls();
    sg_write_buffer_transient(&(sg_write_buffer_desc){ .src.data = { .ptr = scratch, .size = 32 }, .dst.buffer = buf });
    // multiple of four: one queue write
    T(wgpu_mock_count_calls(F(wgpuQueueWriteBuffer)) == 1);
    wgpu_mock_buffer_info_t info = {0};
    T(wgpu_mock_buffer_info(sg_wgpu_query_buffer_info(buf).buf, &info));
    T(info.num_queue_writes == 1);
    T(info.last_write_offset == 0);
    T(info.last_write_size == 32);
    // odd size: split into an aligned write and a 4-byte remainder write
    sg_write_buffer_transient(&(sg_write_buffer_desc){
        .src.data = { .ptr = scratch, .size = 13 },
        .dst = { .buffer = buf, .offset = 32 },
    });
    T(wgpu_mock_count_calls(F(wgpuQueueWriteBuffer)) == 3);
    T(wgpu_mock_buffer_info(sg_wgpu_query_buffer_info(buf).buf, &info));
    T(info.last_write_offset == 44);
    T(info.last_write_size == 4);
    const uint8_t* data = (const uint8_t*)wgpu_mock_buffer_data(sg_wgpu_query_buffer_info(buf).buf);
    T(data[32] == 1);
    T(data[44] == 13);
    sg_destroy_buffer(buf);
    teardown();
}

//== images ====================================================================
UTEST(sokol_gfx_wgpu, create_image_2d) {
    setup();
    sg_image img = make_2d_image();
    T(sg_query_image_state(img) == SG_RESOURCESTATE_VALID);
    WGPUTexture tex = (WGPUTexture)sg_wgpu_query_image_info(img).tex;
    wgpu_mock_texture_info_t info = {0};
    T(wgpu_mock_texture_info(tex, &info));
    T(info.width == 16 && info.height == 16 && info.depth_or_layers == 1);
    T(info.format == WGPUTextureFormat_RGBA8Unorm);
    T(info.dimension == WGPUTextureDimension_2D);
    T(info.num_queue_writes == 1);
    T_LABEL(info.label, "img");
    sg_destroy_image(img);
    teardown();
}

UTEST(sokol_gfx_wgpu, create_image_fail) {
    setup();
    wgpu_mock_fail_next(WGPU_MOCK_OBJ_TEXTURE, 1);
    sg_image img = make_2d_image();
    T(sg_query_image_state(img) == SG_RESOURCESTATE_FAILED);
    T(logged(SG_LOGITEM_WGPU_CREATE_TEXTURE_FAILED));
    sg_destroy_image(img);
    teardown();
}

UTEST(sokol_gfx_wgpu, create_image_injected) {
    setup();
    WGPUTextureDescriptor td = {
        .usage = WGPUTextureUsage_TextureBinding | WGPUTextureUsage_CopyDst,
        .dimension = WGPUTextureDimension_2D,
        .size = { 8, 8, 1 },
        .format = WGPUTextureFormat_RGBA8Unorm,
        .mipLevelCount = 1,
        .sampleCount = 1,
    };
    WGPUTexture tex = wgpuDeviceCreateTexture(wgpu_mock_device(), &td);
    sg_image img = sg_make_image(&(sg_image_desc){
        .width = 8,
        .height = 8,
        .pixel_format = SG_PIXELFORMAT_RGBA8,
        .wgpu_texture = tex,
    });
    T(sg_wgpu_query_image_info(img).tex == tex);
    T(wgpu_mock_ref_count(tex) == 2);
    sg_destroy_image(img);
    T(wgpu_mock_ref_count(tex) == 1);
    wgpuTextureRelease(tex);
    teardown();
}

//== shaders ===================================================================
UTEST(sokol_gfx_wgpu, create_render_shader) {
    setup();
    sg_shader shd = make_shader();
    T(sg_query_shader_state(shd) == SG_RESOURCESTATE_VALID);
    const sg_wgpu_shader_info info = sg_wgpu_query_shader_info(shd);
    T(info.vs_mod && info.fs_mod && info.bgl);
    sg_destroy_shader(shd);
    teardown();
}

//== pipelines and passes ======================================================
UTEST(sokol_gfx_wgpu, swapchain_pass_draw) {
    setup();
    swapchain_t sc = make_swapchain(64, 64, 1, true);
    sg_shader shd = make_shader();
    sg_pipeline pip = make_pipeline(shd);
    sg_buffer vbuf = make_vbuf();
    T(sg_query_pipeline_state(pip) == SG_RESOURCESTATE_VALID);
    sg_begin_pass(&(sg_pass){ .swapchain = swapchain(&sc) });
    sg_apply_pipeline(pip);
    sg_apply_bindings(&(sg_bindings){ .vertex_buffers[0] = vbuf });
    sg_draw(0, 3, 1);
    sg_end_pass();
    sg_commit();
    T(wgpu_mock_render_pass_encoder_state()->num_draws == 1);
    T(num_log_items == 0);
    sg_destroy_buffer(vbuf);
    sg_destroy_pipeline(pip);
    sg_destroy_shader(shd);
    discard_swapchain(&sc);
    teardown();
}

UTEST(sokol_gfx_wgpu, indexed_draw) {
    setup();
    swapchain_t sc = make_swapchain(64, 64, 1, true);
    sg_shader shd = make_shader();
    sg_pipeline pip = sg_make_pipeline(&(sg_pipeline_desc){
        .shader = shd,
        .layout.attrs[0].format = SG_VERTEXFORMAT_FLOAT3,
        .index_type = SG_INDEXTYPE_UINT16,
        .depth.pixel_format = SG_PIXELFORMAT_DEPTH_STENCIL,
        .colors[0].pixel_format = SG_PIXELFORMAT_BGRA8,
    });
    sg_buffer vbuf = make_vbuf();
    sg_buffer ibuf = make_ibuf();
    sg_begin_pass(&(sg_pass){ .swapchain = swapchain(&sc) });
    sg_apply_pipeline(pip);
    sg_apply_bindings(&(sg_bindings){ .vertex_buffers[0] = vbuf, .index_buffer = ibuf });
    sg_draw(0, 6, 2);
    const wgpu_mock_render_pass_encoder_state_t* rpe = wgpu_mock_render_pass_encoder_state();
    T(rpe->num_indexed_draws == 1);
    T(rpe->index_format == WGPUIndexFormat_Uint16);
    sg_end_pass();
    sg_commit();
    sg_destroy_buffer(ibuf);
    sg_destroy_buffer(vbuf);
    sg_destroy_pipeline(pip);
    sg_destroy_shader(shd);
    discard_swapchain(&sc);
    teardown();
}

UTEST(sokol_gfx_wgpu, compute_dispatch) {
    setup();
    sg_shader shd = make_compute_shader();
    T(sg_query_shader_state(shd) == SG_RESOURCESTATE_VALID);
    sg_pipeline pip = sg_make_pipeline(&(sg_pipeline_desc){ .compute = true, .shader = shd });
    T(sg_query_pipeline_state(pip) == SG_RESOURCESTATE_VALID);
    sg_begin_pass(&(sg_pass){ .compute = true });
    sg_apply_pipeline(pip);
    sg_dispatch(2, 3, 4);
    const wgpu_mock_compute_pass_encoder_state_t* cpe = wgpu_mock_compute_pass_encoder_state();
    T(cpe->num_dispatches == 1);
    T(cpe->last_dispatch[0] == 2);
    T(cpe->last_dispatch[1] == 3);
    T(cpe->last_dispatch[2] == 4);
    sg_end_pass();
    sg_commit();
    sg_destroy_pipeline(pip);
    sg_destroy_shader(shd);
    teardown();
}

UTEST(sokol_gfx_wgpu, texture_view_binding) {
    setup();
    swapchain_t sc = make_swapchain(64, 64, 1, true);
    sg_shader shd = sg_make_shader(&(sg_shader_desc){
        .vertex_func = { .source = vs_src, .entry = "vs_main" },
        .fragment_func = { .source = fs_src, .entry = "fs_main" },
        .views[0].texture = { .stage = SG_SHADERSTAGE_FRAGMENT, .image_type = SG_IMAGETYPE_2D, .sample_type = SG_IMAGESAMPLETYPE_FLOAT, .wgsl_group1_binding_n = 0 },
        .samplers[0] = { .stage = SG_SHADERSTAGE_FRAGMENT, .sampler_type = SG_SAMPLERTYPE_FILTERING, .wgsl_group1_binding_n = 1 },
        .texture_sampler_pairs[0] = { .stage = SG_SHADERSTAGE_FRAGMENT, .view_slot = 0, .sampler_slot = 0, .glsl_name = "tex_smp" },
    });
    T(sg_query_shader_state(shd) == SG_RESOURCESTATE_VALID);
    sg_pipeline pip = make_pipeline(shd);
    sg_buffer vbuf = make_vbuf();
    sg_image img = make_2d_image();
    sg_view view = make_texture_view(img);
    sg_sampler smp = sg_make_sampler(&(sg_sampler_desc){ .min_filter = SG_FILTER_LINEAR, .mag_filter = SG_FILTER_LINEAR });
    T(sg_query_view_state(view) == SG_RESOURCESTATE_VALID);
    T(sg_query_sampler_state(smp) == SG_RESOURCESTATE_VALID);
    sg_begin_pass(&(sg_pass){ .swapchain = swapchain(&sc) });
    sg_apply_pipeline(pip);
    sg_apply_bindings(&(sg_bindings){ .vertex_buffers[0] = vbuf, .views[0] = view, .samplers[0] = smp });
    sg_draw(0, 3, 1);
    sg_end_pass();
    sg_commit();
    sg_destroy_sampler(smp);
    sg_destroy_view(view);
    sg_destroy_image(img);
    sg_destroy_buffer(vbuf);
    sg_destroy_pipeline(pip);
    sg_destroy_shader(shd);
    discard_swapchain(&sc);
    teardown();
}

//== extra helpers for attachments, uniforms and copies ========================
static sg_image make_color_image(int w, int h, int sample_count) {
    return sg_make_image(&(sg_image_desc){
        .usage.color_attachment = true,
        .width = w,
        .height = h,
        .sample_count = sample_count,
        .pixel_format = SG_PIXELFORMAT_BGRA8,
        .label = "color-img",
    });
}

static sg_image make_resolve_image(int w, int h) {
    return sg_make_image(&(sg_image_desc){
        .usage.resolve_attachment = true,
        .width = w,
        .height = h,
        .pixel_format = SG_PIXELFORMAT_BGRA8,
        .label = "resolve-img",
    });
}

static sg_image make_depth_image(int w, int h, int sample_count) {
    return sg_make_image(&(sg_image_desc){
        .usage.depth_stencil_attachment = true,
        .width = w,
        .height = h,
        .sample_count = sample_count,
        .pixel_format = SG_PIXELFORMAT_DEPTH_STENCIL,
        .label = "depth-img",
    });
}

// shader with two uniform blocks, binding order differs from slot order
static sg_shader make_ub_shader(void) {
    return sg_make_shader(&(sg_shader_desc){
        .vertex_func = { .source = vs_src, .entry = "vs_main" },
        .fragment_func = { .source = fs_src, .entry = "fs_main" },
        .uniform_blocks[0] = { .stage = SG_SHADERSTAGE_VERTEX, .size = 16, .wgsl_group0_binding_n = 5 },
        .uniform_blocks[1] = { .stage = SG_SHADERSTAGE_FRAGMENT, .size = 32, .wgsl_group0_binding_n = 2 },
        .label = "ub-shd",
    });
}

//== image writes ==============================================================
UTEST(sokol_gfx_wgpu, write_image_unsealed) {
    setup();
    sg_image img = sg_make_image(&(sg_image_desc){
        .usage.write_unsealed = true,
        .width = 16,
        .height = 16,
        .pixel_format = SG_PIXELFORMAT_RGBA8,
    });
    T(sg_query_image_state(img) == SG_RESOURCESTATE_UNSEALED);
    sg_write_image_unsealed(&(sg_write_image_desc){
        .src.data = { .ptr = scratch, .size = 16 * 16 * 4 },
        .dst.image = img,
    });
    wgpu_mock_texture_info_t info = {0};
    T(wgpu_mock_texture_info(sg_wgpu_query_image_info(img).tex, &info));
    T(info.num_queue_writes == 1);
    T(info.last_write_size.width == 16);
    T(info.last_write_size.height == 16);
    T(info.last_write_layout.bytesPerRow == 64);
    sg_seal_image(img);
    T(sg_query_image_state(img) == SG_RESOURCESTATE_VALID);
    sg_destroy_image(img);
    teardown();
}

UTEST(sokol_gfx_wgpu, write_image_transient_partial) {
    setup();
    sg_image img = sg_make_image(&(sg_image_desc){
        .usage.write_transient = true,
        .width = 32,
        .height = 32,
        .pixel_format = SG_PIXELFORMAT_RGBA8,
    });
    T(sg_query_image_state(img) == SG_RESOURCESTATE_VALID);
    sg_write_image_transient(&(sg_write_image_desc){
        .src = { .data = { .ptr = scratch, .size = 8 * 4 * 4 }, .bytes_per_row = 32 },
        .dst = { .image = img, .x = 4, .y = 8 },
        .size = { .width = 8, .height = 4 },
    });
    wgpu_mock_texture_info_t info = {0};
    T(wgpu_mock_texture_info(sg_wgpu_query_image_info(img).tex, &info));
    T(info.num_queue_writes == 1);
    T(info.last_write_origin.x == 4);
    T(info.last_write_origin.y == 8);
    T(info.last_write_size.width == 8);
    T(info.last_write_size.height == 4);
    T(info.last_write_layout.bytesPerRow == 32);
    T(info.last_write_data_size == 128);
    sg_destroy_image(img);
    teardown();
}

UTEST(sokol_gfx_wgpu, write_image_compressed_block_rounding) {
    setup_with((setup_desc_t){ .all_features = true });
    sg_image img = sg_make_image(&(sg_image_desc){
        .usage.write_transient = true,
        .width = 16,
        .height = 16,
        .pixel_format = SG_PIXELFORMAT_BC1_RGBA,
    });
    T(sg_query_image_state(img) == SG_RESOURCESTATE_VALID);
    // partial write in whole blocks
    sg_write_image_transient(&(sg_write_image_desc){
        .src.data = { .ptr = scratch, .size = 256 },
        .dst = { .image = img, .x = 4, .y = 4 },
        .size = { .width = 12, .height = 8 },
    });
    wgpu_mock_texture_info_t info = {0};
    T(wgpu_mock_texture_info(sg_wgpu_query_image_info(img).tex, &info));
    T(info.last_write_origin.x == 4);
    T(info.last_write_size.width == 12);
    T(info.last_write_size.height == 8);
    sg_destroy_image(img);
    teardown();
}

// image with non-block-multiple dimensions: the mip tail handling
UTEST(sokol_gfx_wgpu, create_image_compressed_non_block_multiple) {
    setup_with((setup_desc_t){ .all_features = true });
    sg_image img = sg_make_image(&(sg_image_desc){
        .usage.write_transient = true,
        .width = 10,
        .height = 6,
        .pixel_format = SG_PIXELFORMAT_BC1_RGBA,
    });
    T(sg_query_image_state(img) == SG_RESOURCESTATE_VALID);
    // KNOWN WART: sokol creates the texture at the logical size, WebGPU
    // requires block-multiple sizes for compressed formats
    T(wgpu_mock_find_error("not a multiple of the block size") >= 0);
    wgpu_mock_clear_errors();
    sg_destroy_image(img);
    teardown();
}

// KNOWN WART: sg_make_image() does not check the device features for
// compressed formats, the texture reaches WebGPU and fails validation there
UTEST(sokol_gfx_wgpu, create_image_unsupported_format_not_rejected) {
    setup();
    T(!sg_query_pixelformat(SG_PIXELFORMAT_BC1_RGBA).sample);
    sg_image img = sg_make_image(&(sg_image_desc){
        .usage.write_transient = true,
        .width = 16,
        .height = 16,
        .pixel_format = SG_PIXELFORMAT_BC1_RGBA,
    });
    T(sg_query_image_state(img) == SG_RESOURCESTATE_VALID);
    T(wgpu_mock_find_error("requires feature") >= 0);
    wgpu_mock_clear_errors();
    sg_destroy_image(img);
    teardown();
}

//== copies ====================================================================
UTEST(sokol_gfx_wgpu, copy_buffer_to_buffer_gpu) {
    setup();
    sg_buffer src = sg_make_buffer(&(sg_buffer_desc){
        .usage.copy_src = true,
        .usage.vertex_buffer = true,
        .data = { .ptr = scratch, .size = 64 },
    });
    sg_buffer dst = sg_make_buffer(&(sg_buffer_desc){
        .usage.copy_dst = true,
        .usage.vertex_buffer = true,
        .size = 64,
    });
    sg_copy_buffer_to_buffer(&(sg_copy_buffer_to_buffer_desc){
        .src = { .buffer = src, .offset = 8 },
        .dst = { .buffer = dst, .offset = 16 },
        .size = 32,
    });
    const wgpu_mock_call_t* c = wgpu_mock_last_call(F(wgpuCommandEncoderCopyBufferToBuffer));
    T(c);
    if (c) {
        T(c->args[1].u == 8);
        T(c->args[3].u == 16);
        T(c->args[4].u == 32);
    }
    sg_commit();
    sg_destroy_buffer(dst);
    sg_destroy_buffer(src);
    teardown();
}

UTEST(sokol_gfx_wgpu, copy_buffer_to_buffer_staging) {
    setup();
    sg_buffer src = sg_make_buffer(&(sg_buffer_desc){
        .usage = { .staging_buffer = true, .copy_src = true, .write_transient = true },
        .size = 64,
    });
    sg_write_buffer_transient(&(sg_write_buffer_desc){ .src.data = { .ptr = scratch, .size = 64 }, .dst.buffer = src });
    sg_buffer dst = sg_make_buffer(&(sg_buffer_desc){
        .usage = { .copy_dst = true, .vertex_buffer = true },
        .size = 64,
    });
    wgpu_mock_clear_calls();
    sg_copy_buffer_to_buffer(&(sg_copy_buffer_to_buffer_desc){
        .src = { .buffer = src },
        .dst = { .buffer = dst },
        .size = 16,
    });
    // a staging copy is a queue write, not a command-encoder copy
    T(wgpu_mock_count_calls(F(wgpuCommandEncoderCopyBufferToBuffer)) == 0);
    T(wgpu_mock_count_calls(F(wgpuQueueWriteBuffer)) == 1);
    sg_destroy_buffer(dst);
    sg_destroy_buffer(src);
    teardown();
}

UTEST(sokol_gfx_wgpu, copy_buffer_to_image_gpu) {
    setup();
    sg_buffer src = sg_make_buffer(&(sg_buffer_desc){
        .usage.copy_src = true,
        .usage.vertex_buffer = true,
        .data = { .ptr = scratch, .size = 256 * 8 },
    });
    sg_image dst = sg_make_image(&(sg_image_desc){
        .usage.copy_dst = true,
        .width = 16,
        .height = 8,
        .pixel_format = SG_PIXELFORMAT_RGBA8,
    });
    sg_copy_buffer_to_image(&(sg_copy_buffer_to_image_desc){
        .src = { .buffer = src, .bytes_per_row = 256, .bytes_per_slice = 256 * 8 },
        .dst = { .image = dst },
        .size = { .width = 16, .height = 8, .num_slices = 1 },
    });
    T(wgpu_mock_count_calls(F(wgpuCommandEncoderCopyBufferToTexture)) == 1);
    sg_commit();
    T(wgpu_mock_queue_state()->num_submits == 1);
    sg_destroy_image(dst);
    sg_destroy_buffer(src);
    teardown();
}

UTEST(sokol_gfx_wgpu, copy_buffer_to_image_staging) {
    setup();
    sg_buffer src = sg_make_buffer(&(sg_buffer_desc){
        .usage = { .staging_buffer = true, .copy_src = true, .write_transient = true },
        .size = 16 * 8 * 4,
    });
    sg_write_buffer_transient(&(sg_write_buffer_desc){ .src.data = { .ptr = scratch, .size = 16 * 8 * 4 }, .dst.buffer = src });
    sg_image dst = sg_make_image(&(sg_image_desc){
        .usage.copy_dst = true,
        .width = 16,
        .height = 8,
        .pixel_format = SG_PIXELFORMAT_RGBA8,
    });
    wgpu_mock_clear_calls();
    sg_copy_buffer_to_image(&(sg_copy_buffer_to_image_desc){
        .src = { .buffer = src, .bytes_per_row = 64 },
        .dst = { .image = dst },
        .size = { .width = 16, .height = 8, .num_slices = 1 },
    });
    T(wgpu_mock_count_calls(F(wgpuCommandEncoderCopyBufferToTexture)) == 0);
    T(wgpu_mock_count_calls(F(wgpuQueueWriteTexture)) == 1);
    sg_destroy_image(dst);
    sg_destroy_buffer(src);
    teardown();
}

UTEST(sokol_gfx_wgpu, copy_image_to_image) {
    setup();
    sg_image src = sg_make_image(&(sg_image_desc){
        .usage.copy_src = true,
        .width = 16,
        .height = 16,
        .pixel_format = SG_PIXELFORMAT_RGBA8,
        .data = { .ptr = scratch, .size = 16 * 16 * 4 },
    });
    sg_image dst = sg_make_image(&(sg_image_desc){
        .usage.copy_dst = true,
        .width = 16,
        .height = 16,
        .pixel_format = SG_PIXELFORMAT_RGBA8,
    });
    sg_copy_image_to_image(&(sg_copy_image_to_image_desc){
        .src = { .image = src, .x = 2, .y = 2 },
        .dst = { .image = dst, .x = 4, .y = 4 },
        .size = { .width = 8, .height = 8, .num_slices = 1 },
    });
    T(wgpu_mock_count_calls(F(wgpuCommandEncoderCopyTextureToTexture)) == 1);
    sg_commit();
    sg_destroy_image(dst);
    sg_destroy_image(src);
    teardown();
}

//== viewport and scissor ======================================================
UTEST(sokol_gfx_wgpu, viewport_scissor_origin) {
    setup();
    swapchain_t sc = make_swapchain(64, 48, 1, true);
    sg_begin_pass(&(sg_pass){ .swapchain = swapchain(&sc) });
    const wgpu_mock_render_pass_encoder_state_t* rpe = wgpu_mock_render_pass_encoder_state();
    sg_apply_viewport(10, 4, 20, 10, true);
    T(rpe->has_viewport);
    T(rpe->viewport[0] == 10.0f && rpe->viewport[1] == 4.0f);
    T(rpe->viewport[2] == 20.0f && rpe->viewport[3] == 10.0f);
    // bottom-left origin: y is flipped
    sg_apply_viewport(10, 4, 20, 10, false);
    T(rpe->viewport[1] == 34.0f);
    sg_apply_scissor_rect(5, 6, 20, 10, true);
    T(rpe->has_scissor_rect);
    T(rpe->scissor_rect[0] == 5 && rpe->scissor_rect[1] == 6);
    T(rpe->scissor_rect[2] == 20 && rpe->scissor_rect[3] == 10);
    sg_apply_scissor_rect(5, 6, 20, 10, false);
    T(rpe->scissor_rect[1] == 32);
    // KNOWN WART: the float variants truncate to int
    sg_apply_viewportf(1.5f, 2.5f, 8.0f, 8.0f, true);
    T(rpe->viewport[0] == 1.0f);
    sg_apply_scissor_rectf(1.0f, 2.0f, 8.0f, 8.0f, true);
    T(rpe->scissor_rect[0] == 1);
    sg_end_pass();
    sg_commit();
    discard_swapchain(&sc);
    teardown();
}

//== uniforms ==================================================================
UTEST(sokol_gfx_wgpu, uniforms_dynamic_offsets) {
    setup();
    swapchain_t sc = make_swapchain(64, 64, 1, true);
    sg_shader shd = make_ub_shader();
    T(sg_query_shader_state(shd) == SG_RESOURCESTATE_VALID);
    sg_pipeline pip = make_pipeline(shd);
    sg_buffer vbuf = make_vbuf();
    sg_begin_pass(&(sg_pass){ .swapchain = swapchain(&sc) });
    sg_apply_pipeline(pip);
    sg_apply_bindings(&(sg_bindings){ .vertex_buffers[0] = vbuf });
    sg_apply_uniforms(0, &(sg_range){ .ptr = scratch, .size = 16 });
    sg_apply_uniforms(1, &(sg_range){ .ptr = scratch, .size = 32 });
    sg_draw(0, 3, 1);
    const wgpu_mock_render_pass_encoder_state_t* rpe = wgpu_mock_render_pass_encoder_state();
    // dynamic offsets are ordered by WebGPU binding (2 then 5), not by slot
    T(rpe->bind_groups[0].num_dynamic_offsets == 2);
    T(rpe->bind_groups[0].dynamic_offsets[0] == 256);
    T(rpe->bind_groups[0].dynamic_offsets[1] == 0);
    // a second draw without new uniforms does not set the bind group again
    const int n = wgpu_mock_count_calls(F(wgpuRenderPassEncoderSetBindGroup));
    sg_draw(0, 3, 1);
    T(wgpu_mock_count_calls(F(wgpuRenderPassEncoderSetBindGroup)) == n);
    // new uniforms are written after the previous ones
    sg_apply_uniforms(0, &(sg_range){ .ptr = scratch, .size = 16 });
    sg_draw(0, 3, 1);
    T(rpe->bind_groups[0].dynamic_offsets[1] == 512);
    sg_end_pass();
    wgpu_mock_clear_calls();
    sg_commit();
    const wgpu_mock_call_t* w = wgpu_mock_last_call(F(wgpuQueueWriteBuffer));
    T(w);
    if (w) {
        T(w->args[1].u == 0);
        T(w->args[2].u == 768);
    }
    sg_destroy_buffer(vbuf);
    sg_destroy_pipeline(pip);
    sg_destroy_shader(shd);
    discard_swapchain(&sc);
    teardown();
}

UTEST(sokol_gfx_wgpu, uniforms_compute_pass) {
    setup();
    sg_shader shd = sg_make_shader(&(sg_shader_desc){
        .compute_func = { .source = cs_src, .entry = "cs_main" },
        .uniform_blocks[0] = { .stage = SG_SHADERSTAGE_COMPUTE, .size = 16, .wgsl_group0_binding_n = 1 },
    });
    sg_pipeline pip = sg_make_pipeline(&(sg_pipeline_desc){ .compute = true, .shader = shd });
    sg_begin_pass(&(sg_pass){ .compute = true });
    sg_apply_pipeline(pip);
    sg_apply_uniforms(0, &(sg_range){ .ptr = scratch, .size = 16 });
    sg_dispatch(1, 1, 1);
    const wgpu_mock_compute_pass_encoder_state_t* cpe = wgpu_mock_compute_pass_encoder_state();
    T(cpe->bind_groups[0].group != 0);
    T(cpe->bind_groups[0].num_dynamic_offsets == 1);
    sg_end_pass();
    sg_commit();
    sg_destroy_pipeline(pip);
    sg_destroy_shader(shd);
    teardown();
}

//== offscreen passes ==========================================================
UTEST(sokol_gfx_wgpu, offscreen_pass_msaa_resolve) {
    setup();
    sg_image color = make_color_image(32, 32, 4);
    sg_image resolve = make_resolve_image(32, 32);
    sg_image depth = make_depth_image(32, 32, 4);
    sg_view color_view = sg_make_view(&(sg_view_desc){ .color_attachment.image = color });
    sg_view resolve_view = sg_make_view(&(sg_view_desc){ .resolve_attachment.image = resolve });
    sg_view depth_view = sg_make_view(&(sg_view_desc){ .depth_stencil_attachment.image = depth });
    T(sg_query_view_state(color_view) == SG_RESOURCESTATE_VALID);
    T(sg_query_view_state(resolve_view) == SG_RESOURCESTATE_VALID);
    T(sg_query_view_state(depth_view) == SG_RESOURCESTATE_VALID);
    sg_begin_pass(&(sg_pass){
        .attachments = { .colors[0] = color_view, .resolves[0] = resolve_view, .depth_stencil = depth_view },
        .action = {
            .colors[0] = { .load_action = SG_LOADACTION_CLEAR, .clear_value = { 1.0f, 0.5f, 0.25f, 1.0f }, .store_action = SG_STOREACTION_DONTCARE },
            .depth = { .load_action = SG_LOADACTION_CLEAR, .clear_value = 0.5f, .store_action = SG_STOREACTION_STORE },
            .stencil = { .load_action = SG_LOADACTION_CLEAR, .clear_value = 7, .store_action = SG_STOREACTION_STORE },
        },
        .label = "offscreen",
    });
    const wgpu_mock_render_pass_info_t* rp = wgpu_mock_last_render_pass();
    T(rp->num_color_attachments == 1);
    T(rp->color_attachments[0].view != 0);
    T(rp->color_attachments[0].resolve_target != 0);
    T(rp->color_attachments[0].load_op == WGPULoadOp_Clear);
    T(rp->color_attachments[0].store_op == WGPUStoreOp_Discard);
    T(rp->color_attachments[0].clear_value.g == 0.5);
    T(rp->has_depth_stencil);
    T(rp->depth_load_op == WGPULoadOp_Clear);
    T(rp->depth_store_op == WGPUStoreOp_Store);
    T(rp->depth_clear_value == 0.5f);
    T(rp->stencil_clear_value == 7);
    T_LABEL(rp->label, "offscreen");
    sg_end_pass();
    sg_commit();
    sg_destroy_view(depth_view);
    sg_destroy_view(resolve_view);
    sg_destroy_view(color_view);
    sg_destroy_image(depth);
    sg_destroy_image(resolve);
    sg_destroy_image(color);
    teardown();
}

UTEST(sokol_gfx_wgpu, offscreen_pass_load_actions) {
    setup();
    sg_image color = make_color_image(16, 16, 1);
    sg_view color_view = sg_make_view(&(sg_view_desc){ .color_attachment.image = color });
    sg_begin_pass(&(sg_pass){
        .attachments.colors[0] = color_view,
        .action.colors[0] = { .load_action = SG_LOADACTION_LOAD },
    });
    T(wgpu_mock_last_render_pass()->color_attachments[0].load_op == WGPULoadOp_Load);
    T(wgpu_mock_last_render_pass()->color_attachments[0].store_op == WGPUStoreOp_Store);
    T(!wgpu_mock_last_render_pass()->has_depth_stencil);
    sg_end_pass();
    sg_begin_pass(&(sg_pass){
        .attachments.colors[0] = color_view,
        .action.colors[0] = { .load_action = SG_LOADACTION_DONTCARE },
    });
    T(wgpu_mock_last_render_pass()->color_attachments[0].load_op == WGPULoadOp_Clear);
    sg_end_pass();
    sg_commit();
    sg_destroy_view(color_view);
    sg_destroy_image(color);
    teardown();
}

//== samplers ==================================================================
static bool get_sampler_info(sg_sampler smp, wgpu_mock_sampler_info_t* out) {
    return wgpu_mock_sampler_info(sg_wgpu_query_sampler_info(smp).smp, out);
}

UTEST(sokol_gfx_wgpu, sampler_defaults) {
    setup();
    sg_sampler smp = sg_make_sampler(&(sg_sampler_desc){ .label = "smp" });
    T(sg_query_sampler_state(smp) == SG_RESOURCESTATE_VALID);
    wgpu_mock_sampler_info_t info = {0};
    T(get_sampler_info(smp, &info));
    T(info.address_u == WGPUAddressMode_Repeat);
    T(info.address_v == WGPUAddressMode_Repeat);
    T(info.address_w == WGPUAddressMode_Repeat);
    T(info.mag_filter == WGPUFilterMode_Nearest);
    T(info.min_filter == WGPUFilterMode_Nearest);
    T(info.mipmap_filter == WGPUMipmapFilterMode_Nearest);
    T(info.lod_min_clamp == 0.0f);
    T(info.lod_max_clamp > 1000.0f);
    T(info.compare == WGPUCompareFunction_Undefined);
    T(info.max_anisotropy == 1);
    T_LABEL(info.label, "smp");
    sg_destroy_sampler(smp);
    teardown();
}

UTEST(sokol_gfx_wgpu, sampler_wrap_and_filter) {
    setup();
    sg_sampler smp = sg_make_sampler(&(sg_sampler_desc){
        .wrap_u = SG_WRAP_CLAMP_TO_EDGE,
        .wrap_v = SG_WRAP_MIRRORED_REPEAT,
        .wrap_w = SG_WRAP_REPEAT,
        .min_filter = SG_FILTER_LINEAR,
        .mag_filter = SG_FILTER_LINEAR,
        .mipmap_filter = SG_FILTER_LINEAR,
        .min_lod = 1.0f,
        .max_lod = 4.0f,
        .max_anisotropy = 8,
    });
    wgpu_mock_sampler_info_t info = {0};
    T(get_sampler_info(smp, &info));
    T(info.address_u == WGPUAddressMode_ClampToEdge);
    T(info.address_v == WGPUAddressMode_MirrorRepeat);
    T(info.address_w == WGPUAddressMode_Repeat);
    T(info.mag_filter == WGPUFilterMode_Linear);
    T(info.min_filter == WGPUFilterMode_Linear);
    T(info.mipmap_filter == WGPUMipmapFilterMode_Linear);
    T(info.lod_min_clamp == 1.0f);
    T(info.lod_max_clamp == 4.0f);
    T(info.max_anisotropy == 8);
    sg_destroy_sampler(smp);
    teardown();
}

// KNOWN WART: CLAMP_TO_BORDER is silently mapped to ClampToEdge (features.image_clamp_to_border is false)
UTEST(sokol_gfx_wgpu, sampler_clamp_to_border_is_clamp_to_edge) {
    setup();
    T(!sg_query_features().image_clamp_to_border);
    sg_sampler smp = sg_make_sampler(&(sg_sampler_desc){
        .wrap_u = SG_WRAP_CLAMP_TO_BORDER,
        .wrap_v = SG_WRAP_CLAMP_TO_BORDER,
        .wrap_w = SG_WRAP_CLAMP_TO_BORDER,
        .border_color = SG_BORDERCOLOR_TRANSPARENT_BLACK,
    });
    T(sg_query_sampler_state(smp) == SG_RESOURCESTATE_VALID);
    wgpu_mock_sampler_info_t info = {0};
    T(get_sampler_info(smp, &info));
    T(info.address_u == WGPUAddressMode_ClampToEdge);
    T(info.address_v == WGPUAddressMode_ClampToEdge);
    T(info.address_w == WGPUAddressMode_ClampToEdge);
    sg_destroy_sampler(smp);
    teardown();
}

UTEST(sokol_gfx_wgpu, sampler_compare_funcs) {
    setup();
    static const struct { sg_compare_func sg; WGPUCompareFunction wgpu; } table[] = {
        // NEVER is the default and means 'no comparison'
        { SG_COMPAREFUNC_NEVER, WGPUCompareFunction_Undefined },
        { SG_COMPAREFUNC_LESS, WGPUCompareFunction_Less },
        { SG_COMPAREFUNC_EQUAL, WGPUCompareFunction_Equal },
        { SG_COMPAREFUNC_LESS_EQUAL, WGPUCompareFunction_LessEqual },
        { SG_COMPAREFUNC_GREATER, WGPUCompareFunction_Greater },
        { SG_COMPAREFUNC_NOT_EQUAL, WGPUCompareFunction_NotEqual },
        { SG_COMPAREFUNC_GREATER_EQUAL, WGPUCompareFunction_GreaterEqual },
        { SG_COMPAREFUNC_ALWAYS, WGPUCompareFunction_Always },
    };
    for (size_t i = 0; i < sizeof(table) / sizeof(table[0]); i++) {
        sg_sampler smp = sg_make_sampler(&(sg_sampler_desc){ .compare = table[i].sg });
        wgpu_mock_sampler_info_t info = {0};
        T(get_sampler_info(smp, &info));
        T(info.compare == table[i].wgpu);
        sg_destroy_sampler(smp);
    }
    teardown();
}

UTEST(sokol_gfx_wgpu, sampler_fail) {
    setup();
    wgpu_mock_fail_next(WGPU_MOCK_OBJ_SAMPLER, 1);
    sg_sampler smp = sg_make_sampler(&(sg_sampler_desc){0});
    T(sg_query_sampler_state(smp) == SG_RESOURCESTATE_FAILED);
    T(logged(SG_LOGITEM_WGPU_CREATE_SAMPLER_FAILED));
    sg_destroy_sampler(smp);
    teardown();
}

UTEST(sokol_gfx_wgpu, sampler_injected) {
    setup();
    WGPUSamplerDescriptor sd = { .maxAnisotropy = 1, .lodMaxClamp = 32.0f };
    WGPUSampler wsmp = wgpuDeviceCreateSampler(wgpu_mock_device(), &sd);
    sg_sampler smp = sg_make_sampler(&(sg_sampler_desc){ .wgpu_sampler = wsmp });
    T(sg_query_sampler_state(smp) == SG_RESOURCESTATE_VALID);
    T(sg_wgpu_query_sampler_info(smp).smp == wsmp);
    T(wgpu_mock_ref_count(wsmp) == 2);
    sg_destroy_sampler(smp);
    T(wgpu_mock_ref_count(wsmp) == 1);
    wgpuSamplerRelease(wsmp);
    teardown();
}

//== pixel formats =============================================================
UTEST(sokol_gfx_wgpu, pixel_formats_create) {
    setup_with((setup_desc_t){ .all_features = true });
    static const struct { sg_pixel_format sg; WGPUTextureFormat wgpu; } table[] = {
        { SG_PIXELFORMAT_R8, WGPUTextureFormat_R8Unorm },
        { SG_PIXELFORMAT_R8SN, WGPUTextureFormat_R8Snorm },
        { SG_PIXELFORMAT_R8UI, WGPUTextureFormat_R8Uint },
        { SG_PIXELFORMAT_R8SI, WGPUTextureFormat_R8Sint },
        { SG_PIXELFORMAT_R16, WGPUTextureFormat_R16Unorm },
        { SG_PIXELFORMAT_R16SN, WGPUTextureFormat_R16Snorm },
        { SG_PIXELFORMAT_R16UI, WGPUTextureFormat_R16Uint },
        { SG_PIXELFORMAT_R16SI, WGPUTextureFormat_R16Sint },
        { SG_PIXELFORMAT_R16F, WGPUTextureFormat_R16Float },
        { SG_PIXELFORMAT_RG8, WGPUTextureFormat_RG8Unorm },
        { SG_PIXELFORMAT_RG8SN, WGPUTextureFormat_RG8Snorm },
        { SG_PIXELFORMAT_RG8UI, WGPUTextureFormat_RG8Uint },
        { SG_PIXELFORMAT_RG8SI, WGPUTextureFormat_RG8Sint },
        { SG_PIXELFORMAT_R32UI, WGPUTextureFormat_R32Uint },
        { SG_PIXELFORMAT_R32SI, WGPUTextureFormat_R32Sint },
        { SG_PIXELFORMAT_R32F, WGPUTextureFormat_R32Float },
        { SG_PIXELFORMAT_RG16, WGPUTextureFormat_RG16Unorm },
        { SG_PIXELFORMAT_RG16SN, WGPUTextureFormat_RG16Snorm },
        { SG_PIXELFORMAT_RG16UI, WGPUTextureFormat_RG16Uint },
        { SG_PIXELFORMAT_RG16SI, WGPUTextureFormat_RG16Sint },
        { SG_PIXELFORMAT_RG16F, WGPUTextureFormat_RG16Float },
        { SG_PIXELFORMAT_RGBA8, WGPUTextureFormat_RGBA8Unorm },
        { SG_PIXELFORMAT_SRGB8A8, WGPUTextureFormat_RGBA8UnormSrgb },
        { SG_PIXELFORMAT_RGBA8SN, WGPUTextureFormat_RGBA8Snorm },
        { SG_PIXELFORMAT_RGBA8UI, WGPUTextureFormat_RGBA8Uint },
        { SG_PIXELFORMAT_RGBA8SI, WGPUTextureFormat_RGBA8Sint },
        { SG_PIXELFORMAT_BGRA8, WGPUTextureFormat_BGRA8Unorm },
        { SG_PIXELFORMAT_SBGR8A8, WGPUTextureFormat_BGRA8UnormSrgb },
        { SG_PIXELFORMAT_RGB10A2, WGPUTextureFormat_RGB10A2Unorm },
        { SG_PIXELFORMAT_RG11B10F, WGPUTextureFormat_RG11B10Ufloat },
        { SG_PIXELFORMAT_RGB9E5, WGPUTextureFormat_RGB9E5Ufloat },
        { SG_PIXELFORMAT_RG32UI, WGPUTextureFormat_RG32Uint },
        { SG_PIXELFORMAT_RG32SI, WGPUTextureFormat_RG32Sint },
        { SG_PIXELFORMAT_RG32F, WGPUTextureFormat_RG32Float },
        { SG_PIXELFORMAT_RGBA16, WGPUTextureFormat_RGBA16Unorm },
        { SG_PIXELFORMAT_RGBA16SN, WGPUTextureFormat_RGBA16Snorm },
        { SG_PIXELFORMAT_RGBA16UI, WGPUTextureFormat_RGBA16Uint },
        { SG_PIXELFORMAT_RGBA16SI, WGPUTextureFormat_RGBA16Sint },
        { SG_PIXELFORMAT_RGBA16F, WGPUTextureFormat_RGBA16Float },
        { SG_PIXELFORMAT_RGBA32UI, WGPUTextureFormat_RGBA32Uint },
        { SG_PIXELFORMAT_RGBA32SI, WGPUTextureFormat_RGBA32Sint },
        { SG_PIXELFORMAT_RGBA32F, WGPUTextureFormat_RGBA32Float },
        { SG_PIXELFORMAT_BC1_RGBA, WGPUTextureFormat_BC1RGBAUnorm },
        { SG_PIXELFORMAT_BC2_RGBA, WGPUTextureFormat_BC2RGBAUnorm },
        { SG_PIXELFORMAT_BC3_RGBA, WGPUTextureFormat_BC3RGBAUnorm },
        { SG_PIXELFORMAT_BC3_SRGBA, WGPUTextureFormat_BC3RGBAUnormSrgb },
        { SG_PIXELFORMAT_BC4_R, WGPUTextureFormat_BC4RUnorm },
        { SG_PIXELFORMAT_BC4_RSN, WGPUTextureFormat_BC4RSnorm },
        { SG_PIXELFORMAT_BC5_RG, WGPUTextureFormat_BC5RGUnorm },
        { SG_PIXELFORMAT_BC5_RGSN, WGPUTextureFormat_BC5RGSnorm },
        { SG_PIXELFORMAT_BC6H_RGBF, WGPUTextureFormat_BC6HRGBFloat },
        { SG_PIXELFORMAT_BC6H_RGBUF, WGPUTextureFormat_BC6HRGBUfloat },
        { SG_PIXELFORMAT_BC7_RGBA, WGPUTextureFormat_BC7RGBAUnorm },
        { SG_PIXELFORMAT_BC7_SRGBA, WGPUTextureFormat_BC7RGBAUnormSrgb },
        { SG_PIXELFORMAT_ETC2_RGB8, WGPUTextureFormat_ETC2RGB8Unorm },
        { SG_PIXELFORMAT_ETC2_RGB8A1, WGPUTextureFormat_ETC2RGB8A1Unorm },
        { SG_PIXELFORMAT_ETC2_RGBA8, WGPUTextureFormat_ETC2RGBA8Unorm },
        { SG_PIXELFORMAT_ETC2_SRGB8, WGPUTextureFormat_ETC2RGB8UnormSrgb },
        { SG_PIXELFORMAT_ETC2_SRGB8A8, WGPUTextureFormat_ETC2RGBA8UnormSrgb },
        { SG_PIXELFORMAT_EAC_R11, WGPUTextureFormat_EACR11Unorm },
        { SG_PIXELFORMAT_EAC_R11SN, WGPUTextureFormat_EACR11Snorm },
        { SG_PIXELFORMAT_EAC_RG11, WGPUTextureFormat_EACRG11Unorm },
        { SG_PIXELFORMAT_EAC_RG11SN, WGPUTextureFormat_EACRG11Snorm },
        { SG_PIXELFORMAT_ASTC_4x4_RGBA, WGPUTextureFormat_ASTC4x4Unorm },
        { SG_PIXELFORMAT_ASTC_4x4_SRGBA, WGPUTextureFormat_ASTC4x4UnormSrgb },
    };
    for (size_t i = 0; i < sizeof(table) / sizeof(table[0]); i++) {
        sg_image img = sg_make_image(&(sg_image_desc){
            .usage.write_transient = true,
            .width = 8,
            .height = 8,
            .pixel_format = table[i].sg,
        });
        T(sg_query_image_state(img) == SG_RESOURCESTATE_VALID);
        wgpu_mock_texture_info_t info = {0};
        T(wgpu_mock_texture_info(sg_wgpu_query_image_info(img).tex, &info));
        T(info.format == table[i].wgpu);
        sg_destroy_image(img);
    }
    // depth formats can only be used as attachment
    static const struct { sg_pixel_format sg; WGPUTextureFormat wgpu; } depth_table[] = {
        { SG_PIXELFORMAT_DEPTH, WGPUTextureFormat_Depth32Float },
        { SG_PIXELFORMAT_DEPTH_STENCIL, WGPUTextureFormat_Depth32FloatStencil8 },
    };
    for (size_t i = 0; i < sizeof(depth_table) / sizeof(depth_table[0]); i++) {
        sg_image img = sg_make_image(&(sg_image_desc){
            .usage.depth_stencil_attachment = true,
            .width = 8,
            .height = 8,
            .pixel_format = depth_table[i].sg,
        });
        T(sg_query_image_state(img) == SG_RESOURCESTATE_VALID);
        wgpu_mock_texture_info_t info = {0};
        T(wgpu_mock_texture_info(sg_wgpu_query_image_info(img).tex, &info));
        T(info.format == depth_table[i].wgpu);
        T(info.usage & WGPUTextureUsage_RenderAttachment);
        sg_destroy_image(img);
    }
    teardown();
}

UTEST(sokol_gfx_wgpu, pixel_format_caps_by_feature) {
    setup();
    T(!sg_query_pixelformat(SG_PIXELFORMAT_BC1_RGBA).sample);
    T(!sg_query_pixelformat(SG_PIXELFORMAT_ETC2_RGB8).sample);
    T(!sg_query_pixelformat(SG_PIXELFORMAT_ASTC_4x4_RGBA).sample);
    T(!sg_query_pixelformat(SG_PIXELFORMAT_R32F).filter);
    T(!sg_query_pixelformat(SG_PIXELFORMAT_R32F).blend);
    T(!sg_query_pixelformat(SG_PIXELFORMAT_R16).sample);
    T(!sg_query_pixelformat(SG_PIXELFORMAT_RGBA16SN).sample);
    // KNOWN WART: depth-stencil is reported as supported without checking
    // the depth32float-stencil8 feature
    T(sg_query_pixelformat(SG_PIXELFORMAT_DEPTH_STENCIL).depth);
    sg_shutdown();
    wgpu_mock_set_feature(WGPUFeatureName_Depth32FloatStencil8, false);
    sg_setup(&(sg_desc){ .environment.defaults.color_format = SG_PIXELFORMAT_BGRA8, .environment.wgpu.device = wgpu_mock_device(), .logger.func = capture_log });
    T(sg_query_pixelformat(SG_PIXELFORMAT_DEPTH_STENCIL).depth);
    sg_shutdown();
    wgpu_mock_set_feature(WGPUFeatureName_TextureCompressionBC, true);
    wgpu_mock_set_feature(WGPUFeatureName_TextureCompressionETC2, true);
    wgpu_mock_set_feature(WGPUFeatureName_TextureCompressionASTC, true);
    wgpu_mock_set_feature(WGPUFeatureName_Float32Filterable, true);
    wgpu_mock_set_feature(WGPUFeatureName_Float32Blendable, true);
    wgpu_mock_set_feature(WGPUFeatureName_TextureFormatsTier1, true);
    sg_setup(&(sg_desc){ .environment.defaults.color_format = SG_PIXELFORMAT_BGRA8, .environment.wgpu.device = wgpu_mock_device(), .logger.func = capture_log });
    T(sg_query_pixelformat(SG_PIXELFORMAT_BC1_RGBA).sample);
    T(sg_query_pixelformat(SG_PIXELFORMAT_ETC2_RGB8).sample);
    T(sg_query_pixelformat(SG_PIXELFORMAT_ASTC_4x4_RGBA).sample);
    T(sg_query_pixelformat(SG_PIXELFORMAT_R32F).filter);
    T(sg_query_pixelformat(SG_PIXELFORMAT_R32F).blend);
    T(sg_query_pixelformat(SG_PIXELFORMAT_R16).sample);
    T(sg_query_pixelformat(SG_PIXELFORMAT_RGBA16SN).sample);
    teardown();
}

//== image types ===============================================================
UTEST(sokol_gfx_wgpu, image_3d_with_mips) {
    setup();
    sg_image img = sg_make_image(&(sg_image_desc){
        .type = SG_IMAGETYPE_3D,
        .width = 8,
        .height = 8,
        .num_slices = 4,
        .num_mipmaps = 2,
        .pixel_format = SG_PIXELFORMAT_RGBA8,
        .data = { .ptr = scratch, .size = (8 * 8 * 4 * 4) + (4 * 4 * 4 * 2) },
    });
    T(sg_query_image_state(img) == SG_RESOURCESTATE_VALID);
    wgpu_mock_texture_info_t info = {0};
    T(wgpu_mock_texture_info(sg_wgpu_query_image_info(img).tex, &info));
    T(info.dimension == WGPUTextureDimension_3D);
    T(info.width == 8 && info.height == 8 && info.depth_or_layers == 4);
    T(info.mip_level_count == 2);
    T(info.num_queue_writes == 2);
    T(info.last_write_mip_level == 1);
    T(info.last_write_size.width == 4 && info.last_write_size.height == 4 && info.last_write_size.depthOrArrayLayers == 2);
    T(info.last_write_layout.offset == (8 * 8 * 4 * 4));
    T(info.last_write_layout.bytesPerRow == 16);
    T(info.last_write_layout.rowsPerImage == 4);
    sg_view view = make_texture_view(img);
    T(sg_query_view_state(view) == SG_RESOURCESTATE_VALID);
    wgpu_mock_texture_view_info_t vinfo = {0};
    T(wgpu_mock_texture_view_info(sg_wgpu_query_view_info(view).view, &vinfo));
    T(vinfo.dimension == WGPUTextureViewDimension_3D);
    T(vinfo.mip_level_count == 2);
    T(vinfo.array_layer_count == 1);
    sg_destroy_view(view);
    sg_destroy_image(img);
    teardown();
}

UTEST(sokol_gfx_wgpu, image_cube) {
    setup();
    sg_image img = sg_make_image(&(sg_image_desc){
        .type = SG_IMAGETYPE_CUBE,
        .width = 8,
        .height = 8,
        .pixel_format = SG_PIXELFORMAT_RGBA8,
        .data = { .ptr = scratch, .size = 6 * 8 * 8 * 4 },
    });
    T(sg_query_image_state(img) == SG_RESOURCESTATE_VALID);
    wgpu_mock_texture_info_t info = {0};
    T(wgpu_mock_texture_info(sg_wgpu_query_image_info(img).tex, &info));
    T(info.dimension == WGPUTextureDimension_2D);
    T(info.depth_or_layers == 6);
    T(info.last_write_size.depthOrArrayLayers == 6);
    sg_view view = make_texture_view(img);
    wgpu_mock_texture_view_info_t vinfo = {0};
    T(wgpu_mock_texture_view_info(sg_wgpu_query_view_info(view).view, &vinfo));
    T(vinfo.dimension == WGPUTextureViewDimension_Cube);
    T(vinfo.array_layer_count == 6);
    T(vinfo.aspect == WGPUTextureAspect_All);
    sg_destroy_view(view);
    sg_destroy_image(img);
    teardown();
}

UTEST(sokol_gfx_wgpu, image_array_view_ranges) {
    setup();
    sg_image img = sg_make_image(&(sg_image_desc){
        .type = SG_IMAGETYPE_ARRAY,
        .width = 8,
        .height = 8,
        .num_slices = 4,
        .num_mipmaps = 3,
        .pixel_format = SG_PIXELFORMAT_RGBA8,
        .data = { .ptr = scratch, .size = (4 * 8 * 8 * 4) + (4 * 4 * 4 * 4) + (4 * 2 * 2 * 4) },
    });
    T(sg_query_image_state(img) == SG_RESOURCESTATE_VALID);
    wgpu_mock_texture_info_t info = {0};
    T(wgpu_mock_texture_info(sg_wgpu_query_image_info(img).tex, &info));
    T(info.dimension == WGPUTextureDimension_2D);
    T(info.depth_or_layers == 4);
    T(info.mip_level_count == 3);
    T(info.num_queue_writes == 3);
    // default view covers everything
    sg_view all = make_texture_view(img);
    wgpu_mock_texture_view_info_t vinfo = {0};
    T(wgpu_mock_texture_view_info(sg_wgpu_query_view_info(all).view, &vinfo));
    T(vinfo.dimension == WGPUTextureViewDimension_2DArray);
    T(vinfo.base_mip_level == 0 && vinfo.mip_level_count == 3);
    T(vinfo.base_array_layer == 0 && vinfo.array_layer_count == 4);
    // sub-range view
    sg_view sub = sg_make_view(&(sg_view_desc){
        .texture = { .image = img, .mip_levels = { .base = 1, .count = 2 }, .slices = { .base = 1, .count = 2 } },
    });
    T(sg_query_view_state(sub) == SG_RESOURCESTATE_VALID);
    T(wgpu_mock_texture_view_info(sg_wgpu_query_view_info(sub).view, &vinfo));
    T(vinfo.base_mip_level == 1 && vinfo.mip_level_count == 2);
    T(vinfo.base_array_layer == 1 && vinfo.array_layer_count == 2);
    // sub-range view with defaulted counts
    sg_view rest = sg_make_view(&(sg_view_desc){
        .texture = { .image = img, .mip_levels.base = 2, .slices.base = 3 },
    });
    T(wgpu_mock_texture_view_info(sg_wgpu_query_view_info(rest).view, &vinfo));
    T(vinfo.base_mip_level == 2 && vinfo.mip_level_count == 1);
    T(vinfo.base_array_layer == 3 && vinfo.array_layer_count == 1);
    sg_destroy_view(rest);
    sg_destroy_view(sub);
    sg_destroy_view(all);
    sg_destroy_image(img);
    teardown();
}

UTEST(sokol_gfx_wgpu, image_usage_flags) {
    setup_with((setup_desc_t){ .all_features = true });
    sg_image storage = sg_make_image(&(sg_image_desc){
        .usage.storage_image = true,
        .width = 8,
        .height = 8,
        .pixel_format = SG_PIXELFORMAT_RGBA8,
    });
    sg_image copy = sg_make_image(&(sg_image_desc){
        .usage = { .copy_src = true, .copy_dst = true },
        .width = 8,
        .height = 8,
        .pixel_format = SG_PIXELFORMAT_RGBA8,
    });
    sg_image color = make_color_image(8, 8, 1);
    wgpu_mock_texture_info_t info = {0};
    T(wgpu_mock_texture_info(sg_wgpu_query_image_info(storage).tex, &info));
    T(info.usage & WGPUTextureUsage_StorageBinding);
    T(info.usage & WGPUTextureUsage_TextureBinding);
    T(wgpu_mock_texture_info(sg_wgpu_query_image_info(copy).tex, &info));
    T(info.usage & WGPUTextureUsage_CopySrc);
    T(info.usage & WGPUTextureUsage_CopyDst);
    T(!(info.usage & WGPUTextureUsage_RenderAttachment));
    T(wgpu_mock_texture_info(sg_wgpu_query_image_info(color).tex, &info));
    T(info.usage & WGPUTextureUsage_RenderAttachment);
    T(!(info.usage & WGPUTextureUsage_CopyDst));
    sg_destroy_image(color);
    sg_destroy_image(copy);
    sg_destroy_image(storage);
    teardown();
}

//== views =====================================================================
UTEST(sokol_gfx_wgpu, view_texture_depth_aspect) {
    setup();
    sg_image depth = sg_make_image(&(sg_image_desc){
        .usage.depth_stencil_attachment = true,
        .width = 16,
        .height = 16,
        .pixel_format = SG_PIXELFORMAT_DEPTH_STENCIL,
    });
    sg_view tex_view = make_texture_view(depth);
    sg_view att_view = sg_make_view(&(sg_view_desc){ .depth_stencil_attachment.image = depth });
    T(sg_query_view_state(tex_view) == SG_RESOURCESTATE_VALID);
    T(sg_query_view_state(att_view) == SG_RESOURCESTATE_VALID);
    wgpu_mock_texture_view_info_t vinfo = {0};
    T(wgpu_mock_texture_view_info(sg_wgpu_query_view_info(tex_view).view, &vinfo));
    T(vinfo.aspect == WGPUTextureAspect_DepthOnly);
    T(wgpu_mock_texture_view_info(sg_wgpu_query_view_info(att_view).view, &vinfo));
    T(vinfo.aspect == WGPUTextureAspect_All);
    T(vinfo.format == WGPUTextureFormat_Depth32FloatStencil8);
    sg_destroy_view(att_view);
    sg_destroy_view(tex_view);
    sg_destroy_image(depth);
    teardown();
}

UTEST(sokol_gfx_wgpu, view_attachments_array_and_cube) {
    setup();
    sg_image arr = sg_make_image(&(sg_image_desc){
        .type = SG_IMAGETYPE_ARRAY,
        .usage.color_attachment = true,
        .width = 8,
        .height = 8,
        .num_slices = 4,
        .num_mipmaps = 2,
        .pixel_format = SG_PIXELFORMAT_BGRA8,
    });
    sg_image cube = sg_make_image(&(sg_image_desc){
        .type = SG_IMAGETYPE_CUBE,
        .usage.color_attachment = true,
        .width = 8,
        .height = 8,
        .pixel_format = SG_PIXELFORMAT_BGRA8,
    });
    sg_view arr_view = sg_make_view(&(sg_view_desc){ .color_attachment = { .image = arr, .mip_level = 1, .slice = 2 } });
    sg_view cube_view = sg_make_view(&(sg_view_desc){ .color_attachment = { .image = cube, .slice = 5 } });
    wgpu_mock_texture_view_info_t vinfo = {0};
    T(wgpu_mock_texture_view_info(sg_wgpu_query_view_info(arr_view).view, &vinfo));
    T(vinfo.dimension == WGPUTextureViewDimension_2DArray);
    T(vinfo.base_mip_level == 1 && vinfo.mip_level_count == 1);
    T(vinfo.base_array_layer == 2 && vinfo.array_layer_count == 1);
    T(wgpu_mock_texture_view_info(sg_wgpu_query_view_info(cube_view).view, &vinfo));
    // cube attachments are 2D-array views of a single face
    T(vinfo.dimension == WGPUTextureViewDimension_2DArray);
    T(vinfo.base_array_layer == 5 && vinfo.array_layer_count == 1);
    sg_destroy_view(cube_view);
    sg_destroy_view(arr_view);
    sg_destroy_image(cube);
    sg_destroy_image(arr);
    teardown();
}

// 3D color attachments need a 3D view and a depthSlice in the render pass
UTEST(sokol_gfx_wgpu, view_3d_attachment) {
    setup();
    sg_image img = sg_make_image(&(sg_image_desc){
        .type = SG_IMAGETYPE_3D,
        .usage.color_attachment = true,
        .width = 8,
        .height = 8,
        .num_slices = 4,
        .pixel_format = SG_PIXELFORMAT_BGRA8,
    });
    T(sg_query_image_state(img) == SG_RESOURCESTATE_VALID);
    sg_view view = sg_make_view(&(sg_view_desc){ .color_attachment = { .image = img, .slice = 0 } });
    T(sg_query_view_state(view) == SG_RESOURCESTATE_VALID);
    const void* wgpu_view = sg_wgpu_query_view_info(view).view;
    wgpu_mock_texture_view_info_t vinfo = {0};
    T(wgpu_mock_texture_view_info(wgpu_view, &vinfo));
    T(vinfo.dimension == WGPUTextureViewDimension_3D);
    T(vinfo.base_array_layer == 0 && vinfo.array_layer_count == 1);
    T(vinfo.base_mip_level == 0 && vinfo.mip_level_count == 1);
    sg_begin_pass(&(sg_pass){ .attachments.colors[0] = view });
    const wgpu_mock_render_pass_info_t* rp = wgpu_mock_last_render_pass();
    T(rp->num_color_attachments == 1);
    T(rp->color_attachments[0].view == wgpu_view);
    T(rp->color_attachments[0].depth_slice == 0);
    sg_end_pass();
    sg_commit();
    sg_destroy_view(view);
    sg_destroy_image(img);
    teardown();
}

// non-3D color attachments must leave depthSlice undefined
UTEST(sokol_gfx_wgpu, view_2d_attachment_depth_slice_undefined) {
    setup();
    sg_image img = make_color_image(16, 16, 1);
    sg_view view = sg_make_view(&(sg_view_desc){ .color_attachment.image = img });
    sg_begin_pass(&(sg_pass){ .attachments.colors[0] = view });
    T(wgpu_mock_last_render_pass()->color_attachments[0].depth_slice == WGPU_DEPTH_SLICE_UNDEFINED);
    sg_end_pass();
    sg_commit();
    sg_destroy_view(view);
    sg_destroy_image(img);
    teardown();
}

// 3D storage images need a 3D view to match the bind group layout
UTEST(sokol_gfx_wgpu, view_3d_storage_image) {
    setup();
    sg_shader shd = sg_make_shader(&(sg_shader_desc){
        .compute_func = { .source = cs_src, .entry = "cs_main" },
        .views[0].storage_image = { .stage = SG_SHADERSTAGE_COMPUTE, .image_type = SG_IMAGETYPE_3D, .access_format = SG_PIXELFORMAT_RGBA8, .writeonly = true, .wgsl_group1_binding_n = 0 },
    });
    T(sg_query_shader_state(shd) == SG_RESOURCESTATE_VALID);
    sg_pipeline pip = sg_make_pipeline(&(sg_pipeline_desc){ .compute = true, .shader = shd });
    T(sg_query_pipeline_state(pip) == SG_RESOURCESTATE_VALID);
    sg_image img = sg_make_image(&(sg_image_desc){
        .type = SG_IMAGETYPE_3D,
        .usage.storage_image = true,
        .width = 8,
        .height = 8,
        .num_slices = 4,
        .pixel_format = SG_PIXELFORMAT_RGBA8,
    });
    T(sg_query_image_state(img) == SG_RESOURCESTATE_VALID);
    sg_view view = sg_make_view(&(sg_view_desc){ .storage_image.image = img });
    T(sg_query_view_state(view) == SG_RESOURCESTATE_VALID);
    wgpu_mock_texture_view_info_t vinfo = {0};
    T(wgpu_mock_texture_view_info(sg_wgpu_query_view_info(view).view, &vinfo));
    T(vinfo.dimension == WGPUTextureViewDimension_3D);
    T(vinfo.base_array_layer == 0 && vinfo.array_layer_count == 1);
    sg_begin_pass(&(sg_pass){ .compute = true });
    sg_apply_pipeline(pip);
    sg_apply_bindings(&(sg_bindings){ .views[0] = view });
    sg_dispatch(1, 1, 1);
    T(wgpu_mock_compute_pass_encoder_state()->bind_groups[1].group != 0);
    sg_end_pass();
    sg_commit();
    sg_destroy_view(view);
    sg_destroy_image(img);
    sg_destroy_pipeline(pip);
    sg_destroy_shader(shd);
    teardown();
}

UTEST(sokol_gfx_wgpu, view_storage_buffer) {
    setup();
    sg_buffer buf = sg_make_buffer(&(sg_buffer_desc){
        .usage = { .storage_buffer = true },
        .data = { .ptr = scratch, .size = 1024 },
    });
    T(sg_query_buffer_state(buf) == SG_RESOURCESTATE_VALID);
    sg_view view = sg_make_view(&(sg_view_desc){ .storage_buffer = { .buffer = buf, .offset = 256 } });
    T(sg_query_view_state(view) == SG_RESOURCESTATE_VALID);
    // storage buffer views have no WebGPU object
    T(sg_wgpu_query_view_info(view).view == 0);
    wgpu_mock_buffer_info_t binfo = {0};
    T(wgpu_mock_buffer_info(sg_wgpu_query_buffer_info(buf).buf, &binfo));
    T(binfo.usage & WGPUBufferUsage_Storage);
    sg_destroy_view(view);
    sg_destroy_buffer(buf);
    teardown();
}

UTEST(sokol_gfx_wgpu, view_create_fail) {
    setup();
    sg_image img = make_2d_image();
    wgpu_mock_fail_next(WGPU_MOCK_OBJ_TEXTURE_VIEW, 1);
    sg_view view = make_texture_view(img);
    T(sg_query_view_state(view) == SG_RESOURCESTATE_FAILED);
    T(logged(SG_LOGITEM_WGPU_CREATE_TEXTURE_VIEW_FAILED));
    sg_destroy_view(view);
    sg_destroy_image(img);
    teardown();
}

//== shaders ===================================================================
static const WGPUBindGroupLayoutEntry* find_bgl_entry(const wgpu_mock_bind_group_layout_info_t* info, uint32_t binding) {
    for (int i = 0; i < info->num_entries; i++) {
        if (info->entries[i].binding == binding) {
            return &info->entries[i];
        }
    }
    return 0;
}

static bool get_shader_view_smp_bgl(sg_shader shd, wgpu_mock_bind_group_layout_info_t* out) {
    return wgpu_mock_bind_group_layout_info(sg_wgpu_query_shader_info(shd).bgl, out);
}

static bool get_pip_info(sg_pipeline pip, wgpu_mock_render_pipeline_info_t* out) {
    return wgpu_mock_render_pipeline_info(sg_wgpu_query_pipeline_info(pip).render_pipeline, out);
}

static bool get_pip_layout(sg_pipeline pip, wgpu_mock_pipeline_layout_info_t* out) {
    wgpu_mock_render_pipeline_info_t pi = {0};
    return get_pip_info(pip, &pi) && wgpu_mock_pipeline_layout_info(pi.layout, out);
}

UTEST(sokol_gfx_wgpu, shader_bgl_entries) {
    setup();
    sg_shader shd = sg_make_shader(&(sg_shader_desc){
        .vertex_func = { .source = vs_src, .entry = "vs_main" },
        .fragment_func = { .source = fs_src, .entry = "fs_main" },
        .views = {
            [0].texture = { .stage = SG_SHADERSTAGE_FRAGMENT, .image_type = SG_IMAGETYPE_2D, .sample_type = SG_IMAGESAMPLETYPE_FLOAT, .wgsl_group1_binding_n = 0 },
            [1].texture = { .stage = SG_SHADERSTAGE_FRAGMENT, .image_type = SG_IMAGETYPE_CUBE, .sample_type = SG_IMAGESAMPLETYPE_UNFILTERABLE_FLOAT, .wgsl_group1_binding_n = 1 },
            [2].texture = { .stage = SG_SHADERSTAGE_FRAGMENT, .image_type = SG_IMAGETYPE_ARRAY, .sample_type = SG_IMAGESAMPLETYPE_UINT, .wgsl_group1_binding_n = 2 },
            [3].texture = { .stage = SG_SHADERSTAGE_FRAGMENT, .image_type = SG_IMAGETYPE_3D, .sample_type = SG_IMAGESAMPLETYPE_SINT, .wgsl_group1_binding_n = 3 },
            [4].texture = { .stage = SG_SHADERSTAGE_FRAGMENT, .image_type = SG_IMAGETYPE_2D, .sample_type = SG_IMAGESAMPLETYPE_DEPTH, .wgsl_group1_binding_n = 4 },
            [5].texture = { .stage = SG_SHADERSTAGE_FRAGMENT, .image_type = SG_IMAGETYPE_2D, .sample_type = SG_IMAGESAMPLETYPE_FLOAT, .multisampled = true, .wgsl_group1_binding_n = 5 },
            [6].texture = { .stage = SG_SHADERSTAGE_VERTEX, .image_type = SG_IMAGETYPE_2D, .sample_type = SG_IMAGESAMPLETYPE_FLOAT, .wgsl_group1_binding_n = 6 },
        },
        .samplers = {
            [0] = { .stage = SG_SHADERSTAGE_FRAGMENT, .sampler_type = SG_SAMPLERTYPE_FILTERING, .wgsl_group1_binding_n = 8 },
            [1] = { .stage = SG_SHADERSTAGE_FRAGMENT, .sampler_type = SG_SAMPLERTYPE_COMPARISON, .wgsl_group1_binding_n = 9 },
            [2] = { .stage = SG_SHADERSTAGE_FRAGMENT, .sampler_type = SG_SAMPLERTYPE_NONFILTERING, .wgsl_group1_binding_n = 10 },
            [3] = { .stage = SG_SHADERSTAGE_VERTEX, .sampler_type = SG_SAMPLERTYPE_FILTERING, .wgsl_group1_binding_n = 11 },
        },
        .texture_sampler_pairs = {
            [0] = { .stage = SG_SHADERSTAGE_FRAGMENT, .view_slot = 0, .sampler_slot = 0, .glsl_name = "t0" },
            [1] = { .stage = SG_SHADERSTAGE_FRAGMENT, .view_slot = 1, .sampler_slot = 2, .glsl_name = "t1" },
            [2] = { .stage = SG_SHADERSTAGE_FRAGMENT, .view_slot = 2, .sampler_slot = 2, .glsl_name = "t2" },
            [3] = { .stage = SG_SHADERSTAGE_FRAGMENT, .view_slot = 3, .sampler_slot = 2, .glsl_name = "t3" },
            [4] = { .stage = SG_SHADERSTAGE_FRAGMENT, .view_slot = 4, .sampler_slot = 1, .glsl_name = "t4" },
            [5] = { .stage = SG_SHADERSTAGE_FRAGMENT, .view_slot = 5, .sampler_slot = 2, .glsl_name = "t5" },
            [6] = { .stage = SG_SHADERSTAGE_VERTEX, .view_slot = 6, .sampler_slot = 3, .glsl_name = "t6" },
        },
        .label = "bgl-shd",
    });
    T(sg_query_shader_state(shd) == SG_RESOURCESTATE_VALID);
    wgpu_mock_bind_group_layout_info_t bgl = {0};
    T(get_shader_view_smp_bgl(shd, &bgl));
    T(bgl.num_entries == 11);
    const WGPUBindGroupLayoutEntry* e;
    e = find_bgl_entry(&bgl, 0);
    T(e);
    if (e) {
        T(e->visibility == WGPUShaderStage_Fragment);
        T(e->texture.sampleType == WGPUTextureSampleType_Float);
        T(e->texture.viewDimension == WGPUTextureViewDimension_2D);
        T(!e->texture.multisampled);
    }
    e = find_bgl_entry(&bgl, 1);
    T(e);
    if (e) {
        T(e->texture.sampleType == WGPUTextureSampleType_UnfilterableFloat);
        T(e->texture.viewDimension == WGPUTextureViewDimension_Cube);
    }
    e = find_bgl_entry(&bgl, 2);
    T(e);
    if (e) {
        T(e->texture.sampleType == WGPUTextureSampleType_Uint);
        T(e->texture.viewDimension == WGPUTextureViewDimension_2DArray);
    }
    e = find_bgl_entry(&bgl, 3);
    T(e);
    if (e) {
        T(e->texture.sampleType == WGPUTextureSampleType_Sint);
        T(e->texture.viewDimension == WGPUTextureViewDimension_3D);
    }
    e = find_bgl_entry(&bgl, 4);
    T(e);
    if (e) {
        T(e->texture.sampleType == WGPUTextureSampleType_Depth);
    }
    // a multisampled float texture must be unfilterable
    e = find_bgl_entry(&bgl, 5);
    T(e);
    if (e) {
        T(e->texture.multisampled);
        T(e->texture.sampleType == WGPUTextureSampleType_UnfilterableFloat);
    }
    e = find_bgl_entry(&bgl, 6);
    T(e);
    if (e) {
        T(e->visibility == WGPUShaderStage_Vertex);
    }
    e = find_bgl_entry(&bgl, 8);
    T(e);
    if (e) {
        T(e->sampler.type == WGPUSamplerBindingType_Filtering);
    }
    e = find_bgl_entry(&bgl, 9);
    T(e);
    if (e) {
        T(e->sampler.type == WGPUSamplerBindingType_Comparison);
    }
    e = find_bgl_entry(&bgl, 10);
    T(e);
    if (e) {
        T(e->sampler.type == WGPUSamplerBindingType_NonFiltering);
    }
    e = find_bgl_entry(&bgl, 11);
    T(e);
    if (e) {
        T(e->visibility == WGPUShaderStage_Vertex);
    }
    // all shader modules carry the shader label
    const sg_wgpu_shader_info info = sg_wgpu_query_shader_info(shd);
    wgpu_mock_shader_module_info_t vs_mod = {0}, fs_mod = {0};
    T(wgpu_mock_shader_module_info(info.vs_mod, &vs_mod));
    T(wgpu_mock_shader_module_info(info.fs_mod, &fs_mod));
    T(0 == strcmp(vs_mod.code, vs_src));
    T(0 == strcmp(fs_mod.code, fs_src));
    T_LABEL(vs_mod.label, "bgl-shd");
    T_LABEL(fs_mod.label, "bgl-shd");
    sg_destroy_shader(shd);
    teardown();
}

UTEST(sokol_gfx_wgpu, shader_uniform_block_bgl) {
    setup();
    sg_shader shd = make_ub_shader();
    sg_pipeline pip = make_pipeline(shd);
    T(sg_query_pipeline_state(pip) == SG_RESOURCESTATE_VALID);
    wgpu_mock_pipeline_layout_info_t pl = {0};
    T(get_pip_layout(pip, &pl));
    T(pl.num_bind_group_layouts == 2);
    // group 1 of a shader without views and samplers is empty
    wgpu_mock_bind_group_layout_info_t bgl0 = {0}, bgl1 = {0};
    T(wgpu_mock_bind_group_layout_info(pl.bind_group_layouts[0], &bgl0));
    T(wgpu_mock_bind_group_layout_info(pl.bind_group_layouts[1], &bgl1));
    T(bgl1.num_entries == 0);
    T(pl.bind_group_layouts[1] == sg_wgpu_query_shader_info(shd).bgl);
    T(bgl0.num_entries == 2);
    T(bgl0.num_dynamic_offsets == 2);
    const WGPUBindGroupLayoutEntry* e5 = find_bgl_entry(&bgl0, 5);
    const WGPUBindGroupLayoutEntry* e2 = find_bgl_entry(&bgl0, 2);
    T(e5 && e2);
    if (e5 && e2) {
        T(e5->visibility == WGPUShaderStage_Vertex);
        T(e2->visibility == WGPUShaderStage_Fragment);
        T(e5->buffer.type == WGPUBufferBindingType_Uniform);
        T(e5->buffer.hasDynamicOffset);
        T(e2->buffer.type == WGPUBufferBindingType_Uniform);
        T(e2->buffer.hasDynamicOffset);
    }
    sg_destroy_pipeline(pip);
    sg_destroy_shader(shd);
    teardown();
}

UTEST(sokol_gfx_wgpu, shader_compute_storage_bindings) {
    setup();
    sg_shader shd = sg_make_shader(&(sg_shader_desc){
        .compute_func = { .source = cs_src, .entry = "cs_main" },
        .views = {
            [0].storage_buffer = { .stage = SG_SHADERSTAGE_COMPUTE, .readonly = true, .wgsl_group1_binding_n = 0 },
            [1].storage_buffer = { .stage = SG_SHADERSTAGE_COMPUTE, .readonly = false, .wgsl_group1_binding_n = 1 },
            [2].storage_image = { .stage = SG_SHADERSTAGE_COMPUTE, .image_type = SG_IMAGETYPE_2D, .access_format = SG_PIXELFORMAT_RGBA8, .writeonly = true, .wgsl_group1_binding_n = 2 },
            [3].storage_image = { .stage = SG_SHADERSTAGE_COMPUTE, .image_type = SG_IMAGETYPE_2D, .access_format = SG_PIXELFORMAT_R32F, .writeonly = false, .wgsl_group1_binding_n = 3 },
        },
        .uniform_blocks[0] = { .stage = SG_SHADERSTAGE_COMPUTE, .size = 16, .wgsl_group0_binding_n = 4 },
        .label = "cs-storage",
    });
    T(sg_query_shader_state(shd) == SG_RESOURCESTATE_VALID);
    wgpu_mock_bind_group_layout_info_t bgl = {0};
    T(get_shader_view_smp_bgl(shd, &bgl));
    T(bgl.num_entries == 4);
    const WGPUBindGroupLayoutEntry* e;
    e = find_bgl_entry(&bgl, 0);
    T(e);
    if (e) {
        T(e->visibility == WGPUShaderStage_Compute);
        T(e->buffer.type == WGPUBufferBindingType_ReadOnlyStorage);
    }
    e = find_bgl_entry(&bgl, 1);
    T(e);
    if (e) {
        T(e->buffer.type == WGPUBufferBindingType_Storage);
    }
    e = find_bgl_entry(&bgl, 2);
    T(e);
    if (e) {
        T(e->storageTexture.access == WGPUStorageTextureAccess_WriteOnly);
        T(e->storageTexture.format == WGPUTextureFormat_RGBA8Unorm);
        T(e->storageTexture.viewDimension == WGPUTextureViewDimension_2D);
    }
    e = find_bgl_entry(&bgl, 3);
    T(e);
    if (e) {
        T(e->storageTexture.access == WGPUStorageTextureAccess_ReadWrite);
        T(e->storageTexture.format == WGPUTextureFormat_R32Float);
    }

    // run it through a compute pass
    sg_pipeline pip = sg_make_pipeline(&(sg_pipeline_desc){ .compute = true, .shader = shd });
    T(sg_query_pipeline_state(pip) == SG_RESOURCESTATE_VALID);
    sg_buffer sbuf0 = sg_make_buffer(&(sg_buffer_desc){ .usage.storage_buffer = true, .data = { .ptr = scratch, .size = 256 } });
    sg_buffer sbuf1 = sg_make_buffer(&(sg_buffer_desc){ .usage.storage_buffer = true, .size = 256 });
    sg_image simg0 = sg_make_image(&(sg_image_desc){ .usage.storage_image = true, .width = 16, .height = 16, .pixel_format = SG_PIXELFORMAT_RGBA8 });
    sg_image simg1 = sg_make_image(&(sg_image_desc){ .usage.storage_image = true, .width = 16, .height = 16, .pixel_format = SG_PIXELFORMAT_R32F });
    sg_view v0 = sg_make_view(&(sg_view_desc){ .storage_buffer.buffer = sbuf0 });
    sg_view v1 = sg_make_view(&(sg_view_desc){ .storage_buffer.buffer = sbuf1 });
    sg_view v2 = sg_make_view(&(sg_view_desc){ .storage_image.image = simg0 });
    sg_view v3 = sg_make_view(&(sg_view_desc){ .storage_image.image = simg1 });
    T(sg_query_view_state(v0) == SG_RESOURCESTATE_VALID);
    T(sg_query_view_state(v1) == SG_RESOURCESTATE_VALID);
    T(sg_query_view_state(v2) == SG_RESOURCESTATE_VALID);
    T(sg_query_view_state(v3) == SG_RESOURCESTATE_VALID);
    sg_begin_pass(&(sg_pass){ .compute = true });
    sg_apply_pipeline(pip);
    sg_apply_bindings(&(sg_bindings){ .views = { v0, v1, v2, v3 } });
    sg_apply_uniforms(0, &(sg_range){ scratch, 16 });
    sg_dispatch(1, 1, 1);
    const wgpu_mock_compute_pass_encoder_state_t* cpe = wgpu_mock_compute_pass_encoder_state();
    T(cpe->num_dispatches == 1);
    T(cpe->bind_groups[0].group != 0);
    T(cpe->bind_groups[0].num_dynamic_offsets == 1);
    T(cpe->bind_groups[1].group != 0);
    T(cpe->bind_groups[1].num_dynamic_offsets == 0);
    wgpu_mock_bind_group_info_t bg = {0};
    T(wgpu_mock_bind_group_info(cpe->bind_groups[1].group, &bg));
    T(bg.num_entries == 4);
    for (int i = 0; i < bg.num_entries; i++) {
        if (bg.entries[i].binding < 2) {
            T(bg.entries[i].buffer != 0);
            T(bg.entries[i].size == 256);
        } else {
            T(bg.entries[i].textureView != 0);
        }
    }
    sg_end_pass();
    sg_commit();
    sg_destroy_view(v0);
    sg_destroy_view(v1);
    sg_destroy_view(v2);
    sg_destroy_view(v3);
    sg_destroy_image(simg0);
    sg_destroy_image(simg1);
    sg_destroy_buffer(sbuf0);
    sg_destroy_buffer(sbuf1);
    sg_destroy_pipeline(pip);
    sg_destroy_shader(shd);
    teardown();
}

// the release-mode range check for WGSL bindings (needs disable_validation, in
// debug builds the generic validation asserts before the backend can log)
UTEST(sokol_gfx_wgpu, shader_wgsl_binding_ranges) {
    setup_with((setup_desc_t){ .disable_validation = true });
    // sg_setup() itself owns the uniform buffer
    const int baseline = wgpu_mock_live_objects_total();
    // largest valid bindings
    sg_shader ok = sg_make_shader(&(sg_shader_desc){
        .vertex_func = { .source = vs_src, .entry = "vs_main" },
        .fragment_func = { .source = fs_src, .entry = "fs_main" },
        .uniform_blocks[0] = { .stage = SG_SHADERSTAGE_VERTEX, .size = 16, .wgsl_group0_binding_n = 15 },
        .views[0].texture = { .stage = SG_SHADERSTAGE_FRAGMENT, .image_type = SG_IMAGETYPE_2D, .sample_type = SG_IMAGESAMPLETYPE_FLOAT, .wgsl_group1_binding_n = 127 },
        .samplers[0] = { .stage = SG_SHADERSTAGE_FRAGMENT, .sampler_type = SG_SAMPLERTYPE_FILTERING, .wgsl_group1_binding_n = 126 },
    });
    T(sg_query_shader_state(ok) == SG_RESOURCESTATE_VALID);
    T(num_log_items == 0);
    sg_destroy_shader(ok);

    sg_shader bad_ub = sg_make_shader(&(sg_shader_desc){
        .vertex_func = { .source = vs_src, .entry = "vs_main" },
        .fragment_func = { .source = fs_src, .entry = "fs_main" },
        .uniform_blocks[0] = { .stage = SG_SHADERSTAGE_VERTEX, .size = 16, .wgsl_group0_binding_n = 16 },
    });
    T(sg_query_shader_state(bad_ub) == SG_RESOURCESTATE_FAILED);
    T(logged(SG_LOGITEM_WGPU_UNIFORMBLOCK_WGSL_GROUP0_BINDING_OUT_OF_RANGE));
    sg_destroy_shader(bad_ub);

    reset_log();
    sg_shader bad_tex = sg_make_shader(&(sg_shader_desc){
        .vertex_func = { .source = vs_src, .entry = "vs_main" },
        .fragment_func = { .source = fs_src, .entry = "fs_main" },
        .views[0].texture = { .stage = SG_SHADERSTAGE_FRAGMENT, .image_type = SG_IMAGETYPE_2D, .sample_type = SG_IMAGESAMPLETYPE_FLOAT, .wgsl_group1_binding_n = 128 },
    });
    T(sg_query_shader_state(bad_tex) == SG_RESOURCESTATE_FAILED);
    T(logged(SG_LOGITEM_WGPU_TEXTURE_WGSL_GROUP1_BINDING_OUT_OF_RANGE));
    sg_destroy_shader(bad_tex);

    reset_log();
    sg_shader bad_sbuf = sg_make_shader(&(sg_shader_desc){
        .compute_func = { .source = cs_src, .entry = "cs_main" },
        .views[0].storage_buffer = { .stage = SG_SHADERSTAGE_COMPUTE, .readonly = true, .wgsl_group1_binding_n = 200 },
    });
    T(sg_query_shader_state(bad_sbuf) == SG_RESOURCESTATE_FAILED);
    T(logged(SG_LOGITEM_WGPU_STORAGEBUFFER_WGSL_GROUP1_BINDING_OUT_OF_RANGE));
    sg_destroy_shader(bad_sbuf);

    reset_log();
    sg_shader bad_simg = sg_make_shader(&(sg_shader_desc){
        .compute_func = { .source = cs_src, .entry = "cs_main" },
        .views[0].storage_image = { .stage = SG_SHADERSTAGE_COMPUTE, .image_type = SG_IMAGETYPE_2D, .access_format = SG_PIXELFORMAT_RGBA8, .writeonly = true, .wgsl_group1_binding_n = 255 },
    });
    T(sg_query_shader_state(bad_simg) == SG_RESOURCESTATE_FAILED);
    T(logged(SG_LOGITEM_WGPU_STORAGEIMAGE_WGSL_GROUP1_BINDING_OUT_OF_RANGE));
    sg_destroy_shader(bad_simg);

    reset_log();
    sg_shader bad_smp = sg_make_shader(&(sg_shader_desc){
        .vertex_func = { .source = vs_src, .entry = "vs_main" },
        .fragment_func = { .source = fs_src, .entry = "fs_main" },
        .samplers[0] = { .stage = SG_SHADERSTAGE_FRAGMENT, .sampler_type = SG_SAMPLERTYPE_FILTERING, .wgsl_group1_binding_n = 128 },
    });
    T(sg_query_shader_state(bad_smp) == SG_RESOURCESTATE_FAILED);
    T(logged(SG_LOGITEM_WGPU_SAMPLER_WGSL_GROUP1_BINDING_OUT_OF_RANGE));
    sg_destroy_shader(bad_smp);
    // a rejected shader must not have created any WebGPU object
    T(wgpu_mock_live_objects_total() == baseline);
    teardown();
}

UTEST(sokol_gfx_wgpu, shader_create_fail) {
    setup();
    // vertex module fails, the fragment module must be released again
    wgpu_mock_fail_next(WGPU_MOCK_OBJ_SHADER_MODULE, 1);
    sg_shader shd = make_shader();
    T(sg_query_shader_state(shd) == SG_RESOURCESTATE_FAILED);
    T(logged(SG_LOGITEM_WGPU_CREATE_SHADER_MODULE_FAILED));
    T(wgpu_mock_live_objects(WGPU_MOCK_OBJ_SHADER_MODULE) == 0);
    sg_destroy_shader(shd);

    // fragment module fails, the vertex module must be released again
    reset_log();
    wgpu_mock_fail_next_after(WGPU_MOCK_OBJ_SHADER_MODULE, 1, 1);
    shd = make_shader();
    T(sg_query_shader_state(shd) == SG_RESOURCESTATE_FAILED);
    T(logged(SG_LOGITEM_WGPU_CREATE_SHADER_MODULE_FAILED));
    T(wgpu_mock_live_objects(WGPU_MOCK_OBJ_SHADER_MODULE) == 0);
    sg_destroy_shader(shd);

    // compute module fails
    reset_log();
    wgpu_mock_fail_next(WGPU_MOCK_OBJ_SHADER_MODULE, 1);
    shd = make_compute_shader();
    T(sg_query_shader_state(shd) == SG_RESOURCESTATE_FAILED);
    T(logged(SG_LOGITEM_WGPU_CREATE_SHADER_MODULE_FAILED));
    sg_destroy_shader(shd);

    // the view/sampler bindgroup layout (second layout of the shader) fails
    reset_log();
    wgpu_mock_fail_next_after(WGPU_MOCK_OBJ_BIND_GROUP_LAYOUT, 1, 1);
    shd = make_shader();
    T(sg_query_shader_state(shd) == SG_RESOURCESTATE_FAILED);
    T(logged(SG_LOGITEM_WGPU_SHADER_CREATE_BINDGROUP_LAYOUT_FAILED));
    sg_destroy_shader(shd);
    T(wgpu_mock_live_objects(WGPU_MOCK_OBJ_BIND_GROUP_LAYOUT) == 0);
    T(wgpu_mock_live_objects(WGPU_MOCK_OBJ_BIND_GROUP) == 0);
    teardown();
}

//== pipelines =================================================================
UTEST(sokol_gfx_wgpu, pipeline_defaults) {
    setup();
    sg_shader shd = make_shader();
    sg_pipeline pip = make_pipeline(shd);
    T(sg_query_pipeline_state(pip) == SG_RESOURCESTATE_VALID);
    wgpu_mock_render_pipeline_info_t pi = {0};
    T(get_pip_info(pip, &pi));
    const sg_wgpu_shader_info si = sg_wgpu_query_shader_info(shd);
    T(pi.vertex_module == si.vs_mod);
    T(pi.fragment_module == si.fs_mod);
    T(0 == strcmp(pi.vertex_entry_point, "vs_main"));
    T(0 == strcmp(pi.fragment_entry_point, "fs_main"));
    T_LABEL(pi.label, "pip");
    T(pi.primitive.topology == WGPUPrimitiveTopology_TriangleList);
    T(pi.primitive.stripIndexFormat == WGPUIndexFormat_Undefined);
    T(pi.primitive.frontFace == WGPUFrontFace_CW);
    T(pi.primitive.cullMode == WGPUCullMode_None);
    T(pi.multisample.count == 1);
    T(pi.multisample.mask == 0xFFFFFFFF);
    T(!pi.multisample.alphaToCoverageEnabled);
    T(pi.has_fragment);
    T(pi.num_targets == 1);
    T(pi.targets[0].format == WGPUTextureFormat_BGRA8Unorm);
    T(!pi.targets[0].has_blend);
    T(pi.targets[0].write_mask == (WGPUColorWriteMask_Red | WGPUColorWriteMask_Green | WGPUColorWriteMask_Blue | WGPUColorWriteMask_Alpha));
    T(pi.has_depth_stencil);
    T(pi.depth_stencil.format == WGPUTextureFormat_Depth32FloatStencil8);
    T(pi.depth_stencil.depthWriteEnabled == WGPUOptionalBool_False);
    T(pi.depth_stencil.depthCompare == WGPUCompareFunction_Always);
    // stencil is disabled by default, so the stencil state stays zero-initialized
    T(pi.depth_stencil.stencilFront.compare == WGPUCompareFunction_Undefined);
    T(pi.depth_stencil.stencilFront.passOp == WGPUStencilOperation_Undefined);
    T(pi.depth_stencil.stencilWriteMask == 0);
    T(pi.num_vertex_buffers == 1);
    T(pi.vertex_buffers[0].array_stride == 12);
    T(pi.vertex_buffers[0].step_mode == WGPUVertexStepMode_Vertex);
    T(pi.vertex_buffers[0].num_attributes == 1);
    T(pi.vertex_buffers[0].attributes[0].format == WGPUVertexFormat_Float32x3);
    T(pi.vertex_buffers[0].attributes[0].offset == 0);
    T(pi.vertex_buffers[0].attributes[0].shaderLocation == 0);
    wgpu_mock_pipeline_layout_info_t pl = {0};
    T(get_pip_layout(pip, &pl));
    T(pl.num_bind_group_layouts == 2);
    T(pl.bind_group_layouts[1] == si.bgl);
    // the pipeline layout is only referenced by the pipeline
    T(wgpu_mock_live_objects(WGPU_MOCK_OBJ_PIPELINE_LAYOUT) == 1);
    sg_destroy_pipeline(pip);
    T(wgpu_mock_live_objects(WGPU_MOCK_OBJ_PIPELINE_LAYOUT) == 0);
    sg_destroy_shader(shd);
    teardown();
}

UTEST(sokol_gfx_wgpu, pipeline_primitive_state) {
    setup();
    sg_shader shd = make_shader();
    static const struct {
        sg_primitive_type prim;
        sg_index_type idx;
        sg_cull_mode cull;
        sg_face_winding fw;
        WGPUPrimitiveTopology topo;
        WGPUIndexFormat strip;
        WGPUCullMode wcull;
        WGPUFrontFace wfw;
    } cases[] = {
        { SG_PRIMITIVETYPE_POINTS,         SG_INDEXTYPE_NONE,   SG_CULLMODE_NONE,  SG_FACEWINDING_CW,  WGPUPrimitiveTopology_PointList,     WGPUIndexFormat_Undefined, WGPUCullMode_None,  WGPUFrontFace_CW },
        { SG_PRIMITIVETYPE_LINES,          SG_INDEXTYPE_UINT16, SG_CULLMODE_NONE,  SG_FACEWINDING_CCW, WGPUPrimitiveTopology_LineList,      WGPUIndexFormat_Undefined, WGPUCullMode_None,  WGPUFrontFace_CCW },
        { SG_PRIMITIVETYPE_LINE_STRIP,     SG_INDEXTYPE_UINT16, SG_CULLMODE_FRONT, SG_FACEWINDING_CW,  WGPUPrimitiveTopology_LineStrip,     WGPUIndexFormat_Uint16,    WGPUCullMode_Front, WGPUFrontFace_CW },
        { SG_PRIMITIVETYPE_TRIANGLES,      SG_INDEXTYPE_UINT32, SG_CULLMODE_BACK,  SG_FACEWINDING_CCW, WGPUPrimitiveTopology_TriangleList,  WGPUIndexFormat_Undefined, WGPUCullMode_Back,  WGPUFrontFace_CCW },
        { SG_PRIMITIVETYPE_TRIANGLE_STRIP, SG_INDEXTYPE_UINT32, SG_CULLMODE_NONE,  SG_FACEWINDING_CW,  WGPUPrimitiveTopology_TriangleStrip, WGPUIndexFormat_Uint32,    WGPUCullMode_None,  WGPUFrontFace_CW },
        { SG_PRIMITIVETYPE_TRIANGLE_STRIP, SG_INDEXTYPE_NONE,   SG_CULLMODE_NONE,  SG_FACEWINDING_CW,  WGPUPrimitiveTopology_TriangleStrip, WGPUIndexFormat_Undefined, WGPUCullMode_None,  WGPUFrontFace_CW },
    };
    for (size_t i = 0; i < sizeof(cases) / sizeof(cases[0]); i++) {
        sg_pipeline pip = sg_make_pipeline(&(sg_pipeline_desc){
            .shader = shd,
            .layout.attrs[0].format = SG_VERTEXFORMAT_FLOAT3,
            .primitive_type = cases[i].prim,
            .index_type = cases[i].idx,
            .cull_mode = cases[i].cull,
            .face_winding = cases[i].fw,
            .depth.pixel_format = SG_PIXELFORMAT_NONE,
            .colors[0].pixel_format = SG_PIXELFORMAT_BGRA8,
        });
        T(sg_query_pipeline_state(pip) == SG_RESOURCESTATE_VALID);
        wgpu_mock_render_pipeline_info_t pi = {0};
        T(get_pip_info(pip, &pi));
        T(pi.primitive.topology == cases[i].topo);
        T(pi.primitive.stripIndexFormat == cases[i].strip);
        T(pi.primitive.cullMode == cases[i].wcull);
        T(pi.primitive.frontFace == cases[i].wfw);
        T(!pi.has_depth_stencil);
        sg_destroy_pipeline(pip);
    }
    sg_destroy_shader(shd);
    teardown();
}

UTEST(sokol_gfx_wgpu, pipeline_vertex_formats) {
    setup();
    sg_shader shd = make_shader();
    static const sg_vertex_format sg_fmts1[16] = {
        SG_VERTEXFORMAT_FLOAT, SG_VERTEXFORMAT_FLOAT2, SG_VERTEXFORMAT_FLOAT3, SG_VERTEXFORMAT_FLOAT4,
        SG_VERTEXFORMAT_INT, SG_VERTEXFORMAT_INT2, SG_VERTEXFORMAT_INT3, SG_VERTEXFORMAT_INT4,
        SG_VERTEXFORMAT_UINT, SG_VERTEXFORMAT_UINT2, SG_VERTEXFORMAT_UINT3, SG_VERTEXFORMAT_UINT4,
        SG_VERTEXFORMAT_BYTE4, SG_VERTEXFORMAT_BYTE4N, SG_VERTEXFORMAT_UBYTE4, SG_VERTEXFORMAT_UBYTE4N,
    };
    static const WGPUVertexFormat wgpu_fmts1[16] = {
        WGPUVertexFormat_Float32, WGPUVertexFormat_Float32x2, WGPUVertexFormat_Float32x3, WGPUVertexFormat_Float32x4,
        WGPUVertexFormat_Sint32, WGPUVertexFormat_Sint32x2, WGPUVertexFormat_Sint32x3, WGPUVertexFormat_Sint32x4,
        WGPUVertexFormat_Uint32, WGPUVertexFormat_Uint32x2, WGPUVertexFormat_Uint32x3, WGPUVertexFormat_Uint32x4,
        WGPUVertexFormat_Sint8x4, WGPUVertexFormat_Snorm8x4, WGPUVertexFormat_Uint8x4, WGPUVertexFormat_Unorm8x4,
    };
    static const sg_vertex_format sg_fmts2[11] = {
        SG_VERTEXFORMAT_SHORT2, SG_VERTEXFORMAT_SHORT2N, SG_VERTEXFORMAT_USHORT2, SG_VERTEXFORMAT_USHORT2N,
        SG_VERTEXFORMAT_SHORT4, SG_VERTEXFORMAT_SHORT4N, SG_VERTEXFORMAT_USHORT4, SG_VERTEXFORMAT_USHORT4N,
        SG_VERTEXFORMAT_UINT10_N2, SG_VERTEXFORMAT_HALF2, SG_VERTEXFORMAT_HALF4,
    };
    static const WGPUVertexFormat wgpu_fmts2[11] = {
        WGPUVertexFormat_Sint16x2, WGPUVertexFormat_Snorm16x2, WGPUVertexFormat_Uint16x2, WGPUVertexFormat_Unorm16x2,
        WGPUVertexFormat_Sint16x4, WGPUVertexFormat_Snorm16x4, WGPUVertexFormat_Uint16x4, WGPUVertexFormat_Unorm16x4,
        WGPUVertexFormat_Unorm10_10_10_2, WGPUVertexFormat_Float16x2, WGPUVertexFormat_Float16x4,
    };
    for (int round = 0; round < 2; round++) {
        const int num = (round == 0) ? 16 : 11;
        sg_pipeline_desc pd = {
            .shader = shd,
            .depth.pixel_format = SG_PIXELFORMAT_NONE,
            .colors[0].pixel_format = SG_PIXELFORMAT_BGRA8,
            .layout.buffers[0].stride = 256,
        };
        for (int i = 0; i < num; i++) {
            pd.layout.attrs[i].format = (round == 0) ? sg_fmts1[i] : sg_fmts2[i];
            pd.layout.attrs[i].offset = i * 16;
        }
        sg_pipeline pip = sg_make_pipeline(&pd);
        T(sg_query_pipeline_state(pip) == SG_RESOURCESTATE_VALID);
        wgpu_mock_render_pipeline_info_t pi = {0};
        T(get_pip_info(pip, &pi));
        T(pi.num_vertex_buffers == 1);
        T(pi.vertex_buffers[0].array_stride == 256);
        T(pi.vertex_buffers[0].num_attributes == num);
        for (int i = 0; i < num; i++) {
            T(pi.vertex_buffers[0].attributes[i].format == ((round == 0) ? wgpu_fmts1[i] : wgpu_fmts2[i]));
            T(pi.vertex_buffers[0].attributes[i].offset == (uint64_t)(i * 16));
            T(pi.vertex_buffers[0].attributes[i].shaderLocation == (uint32_t)i);
        }
        sg_destroy_pipeline(pip);
    }
    sg_destroy_shader(shd);
    teardown();
}

UTEST(sokol_gfx_wgpu, pipeline_multiple_vertex_buffers) {
    setup();
    sg_shader shd = make_shader();
    sg_pipeline pip = sg_make_pipeline(&(sg_pipeline_desc){
        .shader = shd,
        .layout = {
            .buffers = {
                [0] = { .stride = 28 },
                [1] = { .stride = 8, .step_func = SG_VERTEXSTEP_PER_INSTANCE },
            },
            .attrs = {
                [0] = { .format = SG_VERTEXFORMAT_FLOAT3, .buffer_index = 0 },
                [1] = { .format = SG_VERTEXFORMAT_FLOAT2, .buffer_index = 1 },
                [2] = { .format = SG_VERTEXFORMAT_FLOAT4, .buffer_index = 0, .offset = 12 },
            },
        },
        .depth.pixel_format = SG_PIXELFORMAT_NONE,
        .colors[0].pixel_format = SG_PIXELFORMAT_BGRA8,
    });
    T(sg_query_pipeline_state(pip) == SG_RESOURCESTATE_VALID);
    wgpu_mock_render_pipeline_info_t pi = {0};
    T(get_pip_info(pip, &pi));
    T(pi.num_vertex_buffers == 2);
    T(pi.vertex_buffers[0].array_stride == 28);
    T(pi.vertex_buffers[0].step_mode == WGPUVertexStepMode_Vertex);
    T(pi.vertex_buffers[0].num_attributes == 2);
    T(pi.vertex_buffers[0].attributes[0].shaderLocation == 0);
    T(pi.vertex_buffers[0].attributes[1].shaderLocation == 2);
    T(pi.vertex_buffers[0].attributes[1].offset == 12);
    T(pi.vertex_buffers[0].attributes[1].format == WGPUVertexFormat_Float32x4);
    T(pi.vertex_buffers[1].array_stride == 8);
    T(pi.vertex_buffers[1].step_mode == WGPUVertexStepMode_Instance);
    T(pi.vertex_buffers[1].num_attributes == 1);
    T(pi.vertex_buffers[1].attributes[0].shaderLocation == 1);
    sg_destroy_pipeline(pip);
    sg_destroy_shader(shd);
    teardown();
}

// unused vertex buffer slots between used slots must become empty layouts
UTEST(sokol_gfx_wgpu, pipeline_sparse_vertex_buffer_slots) {
    setup();
    swapchain_t sc = make_swapchain(64, 64, 1, false);
    sg_shader shd = make_shader();
    sg_pipeline pip = sg_make_pipeline(&(sg_pipeline_desc){
        .shader = shd,
        .layout = {
            .buffers = {
                [0] = { .stride = 16 },
                [2] = { .stride = 8 },
            },
            .attrs = {
                [0] = { .format = SG_VERTEXFORMAT_FLOAT4, .buffer_index = 0 },
                [1] = { .format = SG_VERTEXFORMAT_FLOAT2, .buffer_index = 2 },
            },
        },
        .depth.pixel_format = SG_PIXELFORMAT_NONE,
        .colors[0].pixel_format = SG_PIXELFORMAT_BGRA8,
    });
    T(sg_query_pipeline_state(pip) == SG_RESOURCESTATE_VALID);
    wgpu_mock_render_pipeline_info_t pi = {0};
    T(get_pip_info(pip, &pi));
    T(pi.num_vertex_buffers == 3);
    T(pi.vertex_buffers[0].array_stride == 16);
    T(pi.vertex_buffers[0].step_mode == WGPUVertexStepMode_Vertex);
    T(pi.vertex_buffers[0].num_attributes == 1);
    T(pi.vertex_buffers[0].attributes[0].shaderLocation == 0);
    // unused slot: zero stepMode and no attributes
    T(pi.vertex_buffers[1].step_mode == WGPUVertexStepMode_Undefined);
    T(pi.vertex_buffers[1].num_attributes == 0);
    T(pi.vertex_buffers[2].array_stride == 8);
    T(pi.vertex_buffers[2].step_mode == WGPUVertexStepMode_Vertex);
    T(pi.vertex_buffers[2].num_attributes == 1);
    T(pi.vertex_buffers[2].attributes[0].format == WGPUVertexFormat_Float32x2);
    T(pi.vertex_buffers[2].attributes[0].shaderLocation == 1);
    sg_buffer vbuf0 = make_vbuf();
    sg_buffer vbuf2 = make_vbuf();
    sg_begin_pass(&(sg_pass){ .swapchain = swapchain(&sc) });
    sg_apply_pipeline(pip);
    sg_apply_bindings(&(sg_bindings){ .vertex_buffers = { [0] = vbuf0, [2] = vbuf2 } });
    sg_draw(0, 3, 1);
    sg_end_pass();
    sg_commit();
    T(wgpu_mock_render_pass_encoder_state()->num_draws == 1);
    T(num_log_items == 0);
    sg_destroy_buffer(vbuf2);
    sg_destroy_buffer(vbuf0);
    sg_destroy_pipeline(pip);
    sg_destroy_shader(shd);
    discard_swapchain(&sc);
    teardown();
}

// an unused vertex buffer slot 0 must not drop the following slots
UTEST(sokol_gfx_wgpu, pipeline_vertex_buffer_slot0_unused) {
    setup();
    swapchain_t sc = make_swapchain(64, 64, 1, false);
    sg_shader shd = make_shader();
    sg_pipeline pip = sg_make_pipeline(&(sg_pipeline_desc){
        .shader = shd,
        .layout.attrs[0] = { .format = SG_VERTEXFORMAT_FLOAT3, .buffer_index = 1 },
        .depth.pixel_format = SG_PIXELFORMAT_NONE,
        .colors[0].pixel_format = SG_PIXELFORMAT_BGRA8,
    });
    T(sg_query_pipeline_state(pip) == SG_RESOURCESTATE_VALID);
    wgpu_mock_render_pipeline_info_t pi = {0};
    T(get_pip_info(pip, &pi));
    T(pi.num_vertex_buffers == 2);
    T(pi.vertex_buffers[0].step_mode == WGPUVertexStepMode_Undefined);
    T(pi.vertex_buffers[0].num_attributes == 0);
    T(pi.vertex_buffers[1].array_stride == 12);
    T(pi.vertex_buffers[1].num_attributes == 1);
    T(pi.vertex_buffers[1].attributes[0].shaderLocation == 0);
    sg_buffer vbuf = make_vbuf();
    sg_begin_pass(&(sg_pass){ .swapchain = swapchain(&sc) });
    sg_apply_pipeline(pip);
    sg_apply_bindings(&(sg_bindings){ .vertex_buffers[1] = vbuf });
    sg_draw(0, 3, 1);
    sg_end_pass();
    sg_commit();
    T(wgpu_mock_render_pass_encoder_state()->num_draws == 1);
    T(num_log_items == 0);
    sg_destroy_buffer(vbuf);
    sg_destroy_pipeline(pip);
    sg_destroy_shader(shd);
    discard_swapchain(&sc);
    teardown();
}

UTEST(sokol_gfx_wgpu, pipeline_depth_stencil_state) {
    setup();
    sg_shader shd = make_shader();
    sg_pipeline pip = sg_make_pipeline(&(sg_pipeline_desc){
        .shader = shd,
        .layout.attrs[0].format = SG_VERTEXFORMAT_FLOAT3,
        .depth = {
            .pixel_format = SG_PIXELFORMAT_DEPTH_STENCIL,
            .compare = SG_COMPAREFUNC_LESS_EQUAL,
            .write_enabled = true,
            .bias = 2.7f,
            .bias_slope_scale = 1.5f,
            .bias_clamp = 0.5f,
        },
        .stencil = {
            .enabled = true,
            .front = { .compare = SG_COMPAREFUNC_EQUAL, .fail_op = SG_STENCILOP_ZERO, .depth_fail_op = SG_STENCILOP_REPLACE, .pass_op = SG_STENCILOP_INCR_CLAMP },
            .back = { .compare = SG_COMPAREFUNC_NOT_EQUAL, .fail_op = SG_STENCILOP_DECR_CLAMP, .depth_fail_op = SG_STENCILOP_INVERT, .pass_op = SG_STENCILOP_INCR_WRAP },
            .read_mask = 0x0F,
            .write_mask = 0xF0,
            .ref = 3,
        },
        .colors[0].pixel_format = SG_PIXELFORMAT_BGRA8,
    });
    T(sg_query_pipeline_state(pip) == SG_RESOURCESTATE_VALID);
    wgpu_mock_render_pipeline_info_t pi = {0};
    T(get_pip_info(pip, &pi));
    T(pi.has_depth_stencil);
    const WGPUDepthStencilState* ds = &pi.depth_stencil;
    T(ds->format == WGPUTextureFormat_Depth32FloatStencil8);
    T(ds->depthWriteEnabled == WGPUOptionalBool_True);
    T(ds->depthCompare == WGPUCompareFunction_LessEqual);
    T(ds->stencilFront.compare == WGPUCompareFunction_Equal);
    T(ds->stencilFront.failOp == WGPUStencilOperation_Zero);
    T(ds->stencilFront.depthFailOp == WGPUStencilOperation_Replace);
    T(ds->stencilFront.passOp == WGPUStencilOperation_IncrementClamp);
    T(ds->stencilBack.compare == WGPUCompareFunction_NotEqual);
    T(ds->stencilBack.failOp == WGPUStencilOperation_DecrementClamp);
    T(ds->stencilBack.depthFailOp == WGPUStencilOperation_Invert);
    T(ds->stencilBack.passOp == WGPUStencilOperation_IncrementWrap);
    T(ds->stencilReadMask == 0x0F);
    T(ds->stencilWriteMask == 0xF0);
    // KNOWN WART: the float depth bias is truncated to an integer
    T(ds->depthBias == 2);
    T(ds->depthBiasSlopeScale == 1.5f);
    T(ds->depthBiasClamp == 0.5f);
    sg_destroy_pipeline(pip);
    sg_destroy_shader(shd);
    teardown();
}

// stencil.enabled == false must leave the stencil state zero-initialized
// (Undefined means compare Always and ops Keep, which disables the stencil test)
UTEST(sokol_gfx_wgpu, pipeline_stencil_enabled) {
    setup();
    sg_shader shd = make_shader();
    const sg_stencil_state stencil = {
        .front = { .compare = SG_COMPAREFUNC_EQUAL, .pass_op = SG_STENCILOP_REPLACE },
        .back = { .compare = SG_COMPAREFUNC_NOT_EQUAL, .fail_op = SG_STENCILOP_ZERO },
        .read_mask = 0x0F,
        .write_mask = 0xF0,
    };
    // disabled
    sg_stencil_state disabled = stencil;
    disabled.enabled = false;
    sg_pipeline pip_off = sg_make_pipeline(&(sg_pipeline_desc){
        .shader = shd,
        .layout.attrs[0].format = SG_VERTEXFORMAT_FLOAT3,
        .depth.pixel_format = SG_PIXELFORMAT_DEPTH_STENCIL,
        .stencil = disabled,
        .colors[0].pixel_format = SG_PIXELFORMAT_BGRA8,
    });
    T(sg_query_pipeline_state(pip_off) == SG_RESOURCESTATE_VALID);
    wgpu_mock_render_pipeline_info_t pi = {0};
    T(get_pip_info(pip_off, &pi));
    T(pi.depth_stencil.stencilFront.compare == WGPUCompareFunction_Undefined);
    T(pi.depth_stencil.stencilFront.failOp == WGPUStencilOperation_Undefined);
    T(pi.depth_stencil.stencilFront.depthFailOp == WGPUStencilOperation_Undefined);
    T(pi.depth_stencil.stencilFront.passOp == WGPUStencilOperation_Undefined);
    T(pi.depth_stencil.stencilBack.compare == WGPUCompareFunction_Undefined);
    T(pi.depth_stencil.stencilBack.failOp == WGPUStencilOperation_Undefined);
    T(pi.depth_stencil.stencilBack.depthFailOp == WGPUStencilOperation_Undefined);
    T(pi.depth_stencil.stencilBack.passOp == WGPUStencilOperation_Undefined);
    T(pi.depth_stencil.stencilReadMask == 0);
    T(pi.depth_stencil.stencilWriteMask == 0);
    // enabled
    sg_stencil_state enabled = stencil;
    enabled.enabled = true;
    sg_pipeline pip_on = sg_make_pipeline(&(sg_pipeline_desc){
        .shader = shd,
        .layout.attrs[0].format = SG_VERTEXFORMAT_FLOAT3,
        .depth.pixel_format = SG_PIXELFORMAT_DEPTH_STENCIL,
        .stencil = enabled,
        .colors[0].pixel_format = SG_PIXELFORMAT_BGRA8,
    });
    T(sg_query_pipeline_state(pip_on) == SG_RESOURCESTATE_VALID);
    T(get_pip_info(pip_on, &pi));
    T(pi.depth_stencil.stencilFront.compare == WGPUCompareFunction_Equal);
    T(pi.depth_stencil.stencilFront.passOp == WGPUStencilOperation_Replace);
    T(pi.depth_stencil.stencilBack.compare == WGPUCompareFunction_NotEqual);
    T(pi.depth_stencil.stencilBack.failOp == WGPUStencilOperation_Zero);
    T(pi.depth_stencil.stencilReadMask == 0x0F);
    T(pi.depth_stencil.stencilWriteMask == 0xF0);
    sg_destroy_pipeline(pip_on);
    sg_destroy_pipeline(pip_off);
    sg_destroy_shader(shd);
    teardown();
}

// stencil state in a desc with a depth-only format must not reach WebGPU
// when stencil.enabled is false (teardown() checks for mock errors)
UTEST(sokol_gfx_wgpu, pipeline_stencil_disabled_depth_only_format) {
    setup();
    sg_shader shd = make_shader();
    sg_pipeline pip = sg_make_pipeline(&(sg_pipeline_desc){
        .shader = shd,
        .layout.attrs[0].format = SG_VERTEXFORMAT_FLOAT3,
        .depth.pixel_format = SG_PIXELFORMAT_DEPTH,
        .stencil = {
            .enabled = false,
            .front = { .compare = SG_COMPAREFUNC_EQUAL, .pass_op = SG_STENCILOP_REPLACE },
            .back = { .compare = SG_COMPAREFUNC_EQUAL, .pass_op = SG_STENCILOP_REPLACE },
        },
        .colors[0].pixel_format = SG_PIXELFORMAT_BGRA8,
    });
    T(sg_query_pipeline_state(pip) == SG_RESOURCESTATE_VALID);
    wgpu_mock_render_pipeline_info_t pi = {0};
    T(get_pip_info(pip, &pi));
    T(pi.depth_stencil.format == WGPUTextureFormat_Depth32Float);
    T(pi.depth_stencil.stencilFront.compare == WGPUCompareFunction_Undefined);
    T(pi.depth_stencil.stencilBack.compare == WGPUCompareFunction_Undefined);
    T(num_log_items == 0);
    sg_destroy_pipeline(pip);
    sg_destroy_shader(shd);
    teardown();
}

UTEST(sokol_gfx_wgpu, pipeline_depth_only_and_no_depth) {
    setup();
    sg_shader shd = make_shader();
    // depth-only: the fragment state has no targets, but the fragment
    // shader must still run (for discard and frag_depth writes)
    sg_pipeline depth_only = sg_make_pipeline(&(sg_pipeline_desc){
        .shader = shd,
        .layout.attrs[0].format = SG_VERTEXFORMAT_FLOAT3,
        .depth = { .pixel_format = SG_PIXELFORMAT_DEPTH, .write_enabled = true, .compare = SG_COMPAREFUNC_LESS },
        .colors[0].pixel_format = SG_PIXELFORMAT_NONE,
    });
    T(sg_query_pipeline_state(depth_only) == SG_RESOURCESTATE_VALID);
    wgpu_mock_render_pipeline_info_t pi = {0};
    T(get_pip_info(depth_only, &pi));
    T(pi.has_fragment);
    T(pi.fragment_module != 0);
    T(0 == strcmp(pi.fragment_entry_point, "fs_main"));
    T(pi.num_targets == 0);
    T(pi.has_depth_stencil);
    T(pi.depth_stencil.format == WGPUTextureFormat_Depth32Float);
    T(pi.depth_stencil.depthWriteEnabled == WGPUOptionalBool_True);
    T(pi.depth_stencil.depthCompare == WGPUCompareFunction_Less);
    // draw in a depth-only pass
    sg_image depth = sg_make_image(&(sg_image_desc){
        .usage.depth_stencil_attachment = true,
        .width = 32,
        .height = 32,
        .pixel_format = SG_PIXELFORMAT_DEPTH,
    });
    sg_view depth_view = sg_make_view(&(sg_view_desc){ .depth_stencil_attachment.image = depth });
    sg_buffer vbuf = make_vbuf();
    sg_begin_pass(&(sg_pass){ .attachments.depth_stencil = depth_view });
    sg_apply_pipeline(depth_only);
    sg_apply_bindings(&(sg_bindings){ .vertex_buffers[0] = vbuf });
    sg_draw(0, 3, 1);
    sg_end_pass();
    sg_commit();
    T(wgpu_mock_render_pass_encoder_state()->num_draws == 1);
    T(num_log_items == 0);
    sg_destroy_buffer(vbuf);
    sg_destroy_view(depth_view);
    sg_destroy_image(depth);
    // color-only
    sg_pipeline color_only = sg_make_pipeline(&(sg_pipeline_desc){
        .shader = shd,
        .layout.attrs[0].format = SG_VERTEXFORMAT_FLOAT3,
        .depth.pixel_format = SG_PIXELFORMAT_NONE,
        .colors[0].pixel_format = SG_PIXELFORMAT_RGBA8,
    });
    T(sg_query_pipeline_state(color_only) == SG_RESOURCESTATE_VALID);
    T(get_pip_info(color_only, &pi));
    T(pi.has_fragment);
    T(!pi.has_depth_stencil);
    T(pi.targets[0].format == WGPUTextureFormat_RGBA8Unorm);
    sg_destroy_pipeline(color_only);
    sg_destroy_pipeline(depth_only);
    sg_destroy_shader(shd);
    teardown();
}

UTEST(sokol_gfx_wgpu, pipeline_blend_and_write_masks) {
    setup();
    sg_shader shd = make_shader();
    sg_pipeline pip = sg_make_pipeline(&(sg_pipeline_desc){
        .shader = shd,
        .layout.attrs[0].format = SG_VERTEXFORMAT_FLOAT3,
        .depth.pixel_format = SG_PIXELFORMAT_NONE,
        .color_count = 4,
        .colors = {
            [0] = {
                .pixel_format = SG_PIXELFORMAT_BGRA8,
                .write_mask = SG_COLORMASK_RGB,
                .blend = {
                    .enabled = true,
                    .src_factor_rgb = SG_BLENDFACTOR_SRC_ALPHA,
                    .dst_factor_rgb = SG_BLENDFACTOR_ONE_MINUS_SRC_ALPHA,
                    .op_rgb = SG_BLENDOP_ADD,
                    .src_factor_alpha = SG_BLENDFACTOR_ONE,
                    .dst_factor_alpha = SG_BLENDFACTOR_ONE_MINUS_SRC_ALPHA,
                    .op_alpha = SG_BLENDOP_ADD,
                },
            },
            [1] = { .pixel_format = SG_PIXELFORMAT_RGBA8 },
            [2] = {
                .pixel_format = SG_PIXELFORMAT_RGBA16F,
                .write_mask = SG_COLORMASK_GB,
                .blend = {
                    .enabled = true,
                    .src_factor_rgb = SG_BLENDFACTOR_DST_COLOR,
                    .dst_factor_rgb = SG_BLENDFACTOR_ONE_MINUS_DST_COLOR,
                    .op_rgb = SG_BLENDOP_SUBTRACT,
                    .src_factor_alpha = SG_BLENDFACTOR_DST_ALPHA,
                    .dst_factor_alpha = SG_BLENDFACTOR_ONE_MINUS_DST_ALPHA,
                    .op_alpha = SG_BLENDOP_REVERSE_SUBTRACT,
                },
            },
            [3] = { .pixel_format = SG_PIXELFORMAT_RGBA8, .write_mask = SG_COLORMASK_NONE },
        },
    });
    T(sg_query_pipeline_state(pip) == SG_RESOURCESTATE_VALID);
    wgpu_mock_render_pipeline_info_t pi = {0};
    T(get_pip_info(pip, &pi));
    T(pi.has_fragment);
    T(pi.num_targets == 4);
    T(pi.targets[0].format == WGPUTextureFormat_BGRA8Unorm);
    T(pi.targets[0].has_blend);
    T(pi.targets[0].write_mask == (WGPUColorWriteMask_Red | WGPUColorWriteMask_Green | WGPUColorWriteMask_Blue));
    T(pi.targets[0].blend.color.srcFactor == WGPUBlendFactor_SrcAlpha);
    T(pi.targets[0].blend.color.dstFactor == WGPUBlendFactor_OneMinusSrcAlpha);
    T(pi.targets[0].blend.color.operation == WGPUBlendOperation_Add);
    T(pi.targets[0].blend.alpha.srcFactor == WGPUBlendFactor_One);
    T(pi.targets[0].blend.alpha.dstFactor == WGPUBlendFactor_OneMinusSrcAlpha);
    T(pi.targets[0].blend.alpha.operation == WGPUBlendOperation_Add);
    T(!pi.targets[1].has_blend);
    T(pi.targets[1].write_mask == (WGPUColorWriteMask_Red | WGPUColorWriteMask_Green | WGPUColorWriteMask_Blue | WGPUColorWriteMask_Alpha));
    T(pi.targets[2].has_blend);
    T(pi.targets[2].write_mask == (WGPUColorWriteMask_Green | WGPUColorWriteMask_Blue));
    T(pi.targets[2].blend.color.srcFactor == WGPUBlendFactor_Dst);
    T(pi.targets[2].blend.color.dstFactor == WGPUBlendFactor_OneMinusDst);
    T(pi.targets[2].blend.color.operation == WGPUBlendOperation_Subtract);
    T(pi.targets[2].blend.alpha.srcFactor == WGPUBlendFactor_DstAlpha);
    T(pi.targets[2].blend.alpha.dstFactor == WGPUBlendFactor_OneMinusDstAlpha);
    T(pi.targets[2].blend.alpha.operation == WGPUBlendOperation_ReverseSubtract);
    T(pi.targets[3].write_mask == WGPUColorWriteMask_None);
    sg_destroy_pipeline(pip);
    sg_destroy_shader(shd);
    teardown();
}

UTEST(sokol_gfx_wgpu, pipeline_blend_constant_min_max) {
    setup();
    sg_shader shd = make_shader();
    sg_pipeline pip = sg_make_pipeline(&(sg_pipeline_desc){
        .shader = shd,
        .layout.attrs[0].format = SG_VERTEXFORMAT_FLOAT3,
        .depth.pixel_format = SG_PIXELFORMAT_NONE,
        .color_count = 3,
        .colors = {
            [0] = {
                .pixel_format = SG_PIXELFORMAT_BGRA8,
                .blend = {
                    .enabled = true,
                    .src_factor_rgb = SG_BLENDFACTOR_BLEND_COLOR,
                    .dst_factor_rgb = SG_BLENDFACTOR_ONE_MINUS_BLEND_COLOR,
                    .src_factor_alpha = SG_BLENDFACTOR_BLEND_ALPHA,
                    .dst_factor_alpha = SG_BLENDFACTOR_ONE_MINUS_BLEND_ALPHA,
                },
            },
            [1] = {
                .pixel_format = SG_PIXELFORMAT_RGBA8,
                .blend = {
                    .enabled = true,
                    .src_factor_rgb = SG_BLENDFACTOR_ONE,
                    .dst_factor_rgb = SG_BLENDFACTOR_ONE,
                    .op_rgb = SG_BLENDOP_MIN,
                    .src_factor_alpha = SG_BLENDFACTOR_ONE,
                    .dst_factor_alpha = SG_BLENDFACTOR_ONE,
                    .op_alpha = SG_BLENDOP_MAX,
                },
            },
            [2] = {
                .pixel_format = SG_PIXELFORMAT_RGBA8,
                .blend = {
                    .enabled = true,
                    .src_factor_rgb = SG_BLENDFACTOR_SRC_COLOR,
                    .dst_factor_rgb = SG_BLENDFACTOR_ONE_MINUS_SRC_COLOR,
                    .src_factor_alpha = SG_BLENDFACTOR_SRC_ALPHA_SATURATED,
                    .dst_factor_alpha = SG_BLENDFACTOR_ZERO,
                },
            },
        },
    });
    T(sg_query_pipeline_state(pip) == SG_RESOURCESTATE_VALID);
    wgpu_mock_render_pipeline_info_t pi = {0};
    T(get_pip_info(pip, &pi));
    T(pi.targets[0].blend.color.srcFactor == WGPUBlendFactor_Constant);
    T(pi.targets[0].blend.color.dstFactor == WGPUBlendFactor_OneMinusConstant);
    // KNOWN WART: BLEND_ALPHA maps to the constant's RGB, not to its alpha (FIXME in sokol_gfx.h)
    T(pi.targets[0].blend.alpha.srcFactor == WGPUBlendFactor_Constant);
    T(pi.targets[0].blend.alpha.dstFactor == WGPUBlendFactor_OneMinusConstant);
    T(pi.targets[1].blend.color.operation == WGPUBlendOperation_Min);
    T(pi.targets[1].blend.alpha.operation == WGPUBlendOperation_Max);
    T(pi.targets[2].blend.color.srcFactor == WGPUBlendFactor_Src);
    T(pi.targets[2].blend.color.dstFactor == WGPUBlendFactor_OneMinusSrc);
    T(pi.targets[2].blend.alpha.srcFactor == WGPUBlendFactor_SrcAlphaSaturated);
    T(pi.targets[2].blend.alpha.dstFactor == WGPUBlendFactor_Zero);
    sg_destroy_pipeline(pip);
    sg_destroy_shader(shd);
    teardown();
}

UTEST(sokol_gfx_wgpu, pipeline_dual_source_blend) {
    setup_with((setup_desc_t){ .all_features = true });
    sg_shader shd = make_shader();
    sg_pipeline pip = sg_make_pipeline(&(sg_pipeline_desc){
        .shader = shd,
        .layout.attrs[0].format = SG_VERTEXFORMAT_FLOAT3,
        .depth.pixel_format = SG_PIXELFORMAT_NONE,
        .colors[0] = {
            .pixel_format = SG_PIXELFORMAT_BGRA8,
            .blend = {
                .enabled = true,
                .src_factor_rgb = SG_BLENDFACTOR_SRC1_COLOR,
                .dst_factor_rgb = SG_BLENDFACTOR_ONE_MINUS_SRC1_COLOR,
                .src_factor_alpha = SG_BLENDFACTOR_SRC1_ALPHA,
                .dst_factor_alpha = SG_BLENDFACTOR_ONE_MINUS_SRC1_ALPHA,
            },
        },
    });
    T(sg_query_pipeline_state(pip) == SG_RESOURCESTATE_VALID);
    wgpu_mock_render_pipeline_info_t pi = {0};
    T(get_pip_info(pip, &pi));
    T(pi.targets[0].blend.color.srcFactor == WGPUBlendFactor_Src1);
    T(pi.targets[0].blend.color.dstFactor == WGPUBlendFactor_OneMinusSrc1);
    T(pi.targets[0].blend.alpha.srcFactor == WGPUBlendFactor_Src1Alpha);
    T(pi.targets[0].blend.alpha.dstFactor == WGPUBlendFactor_OneMinusSrc1Alpha);
    sg_destroy_pipeline(pip);
    sg_destroy_shader(shd);
    teardown();
}

UTEST(sokol_gfx_wgpu, pipeline_alpha_to_coverage) {
    setup();
    sg_shader shd = make_shader();
    sg_pipeline pip = sg_make_pipeline(&(sg_pipeline_desc){
        .shader = shd,
        .layout.attrs[0].format = SG_VERTEXFORMAT_FLOAT3,
        .sample_count = 4,
        .alpha_to_coverage_enabled = true,
        .depth.pixel_format = SG_PIXELFORMAT_NONE,
        .colors[0].pixel_format = SG_PIXELFORMAT_BGRA8,
    });
    T(sg_query_pipeline_state(pip) == SG_RESOURCESTATE_VALID);
    wgpu_mock_render_pipeline_info_t pi = {0};
    T(get_pip_info(pip, &pi));
    T(pi.multisample.count == 4);
    T(pi.multisample.mask == 0xFFFFFFFF);
    T(pi.multisample.alphaToCoverageEnabled);
    sg_destroy_pipeline(pip);
    sg_destroy_shader(shd);
    teardown();
}

UTEST(sokol_gfx_wgpu, pipeline_compute_info) {
    setup();
    sg_shader shd = make_compute_shader();
    sg_pipeline pip = sg_make_pipeline(&(sg_pipeline_desc){ .compute = true, .shader = shd, .label = "cpip" });
    T(sg_query_pipeline_state(pip) == SG_RESOURCESTATE_VALID);
    const sg_wgpu_pipeline_info info = sg_wgpu_query_pipeline_info(pip);
    T(info.compute_pipeline != 0);
    T(info.render_pipeline == 0);
    wgpu_mock_compute_pipeline_info_t ci = {0};
    T(wgpu_mock_compute_pipeline_info(info.compute_pipeline, &ci));
    T(0 == strcmp(ci.entry_point, "cs_main"));
    T_LABEL(ci.label, "cpip");
    wgpu_mock_shader_module_info_t mi = {0};
    T(wgpu_mock_shader_module_info(ci.module, &mi));
    T(0 == strcmp(mi.code, cs_src));
    wgpu_mock_pipeline_layout_info_t pl = {0};
    T(wgpu_mock_pipeline_layout_info(ci.layout, &pl));
    T(pl.num_bind_group_layouts == 2);
    sg_destroy_pipeline(pip);
    sg_destroy_shader(shd);
    teardown();
}

UTEST(sokol_gfx_wgpu, pipeline_create_fail) {
    setup();
    sg_shader shd = make_shader();
    sg_shader cshd = make_compute_shader();
    // pipeline layout
    wgpu_mock_fail_next(WGPU_MOCK_OBJ_PIPELINE_LAYOUT, 1);
    sg_pipeline pip = make_pipeline(shd);
    T(sg_query_pipeline_state(pip) == SG_RESOURCESTATE_FAILED);
    T(logged(SG_LOGITEM_WGPU_CREATE_PIPELINE_LAYOUT_FAILED));
    sg_destroy_pipeline(pip);
    // render pipeline, the pipeline layout must be released
    reset_log();
    wgpu_mock_fail_next(WGPU_MOCK_OBJ_RENDER_PIPELINE, 1);
    pip = make_pipeline(shd);
    T(sg_query_pipeline_state(pip) == SG_RESOURCESTATE_FAILED);
    T(logged(SG_LOGITEM_WGPU_CREATE_RENDER_PIPELINE_FAILED));
    sg_destroy_pipeline(pip);
    T(wgpu_mock_live_objects(WGPU_MOCK_OBJ_PIPELINE_LAYOUT) == 0);
    // compute pipeline
    reset_log();
    wgpu_mock_fail_next(WGPU_MOCK_OBJ_COMPUTE_PIPELINE, 1);
    pip = sg_make_pipeline(&(sg_pipeline_desc){ .compute = true, .shader = cshd });
    T(sg_query_pipeline_state(pip) == SG_RESOURCESTATE_FAILED);
    T(logged(SG_LOGITEM_WGPU_CREATE_COMPUTE_PIPELINE_FAILED));
    sg_destroy_pipeline(pip);
    T(wgpu_mock_live_objects(WGPU_MOCK_OBJ_PIPELINE_LAYOUT) == 0);
    sg_destroy_shader(cshd);
    sg_destroy_shader(shd);
    teardown();
}

UTEST(sokol_gfx_wgpu, pipeline_apply_blend_color_and_stencil_ref) {
    setup();
    swapchain_t sc = make_swapchain(64, 64, 1, true);
    sg_shader shd = make_shader();
    sg_pipeline pip0 = sg_make_pipeline(&(sg_pipeline_desc){
        .shader = shd,
        .layout.attrs[0].format = SG_VERTEXFORMAT_FLOAT3,
        .depth.pixel_format = SG_PIXELFORMAT_DEPTH_STENCIL,
        .stencil.ref = 7,
        .blend_color = { 0.25f, 0.5f, 0.75f, 1.0f },
        .colors[0].pixel_format = SG_PIXELFORMAT_BGRA8,
    });
    sg_pipeline pip1 = sg_make_pipeline(&(sg_pipeline_desc){
        .shader = shd,
        .layout.attrs[0].format = SG_VERTEXFORMAT_FLOAT3,
        .depth.pixel_format = SG_PIXELFORMAT_DEPTH_STENCIL,
        .stencil.ref = 9,
        .blend_color = { 1.0f, 0.0f, 0.5f, 0.25f },
        .colors[0].pixel_format = SG_PIXELFORMAT_BGRA8,
    });
    sg_begin_pass(&(sg_pass){ .swapchain = swapchain(&sc) });
    sg_apply_pipeline(pip0);
    const wgpu_mock_render_pass_encoder_state_t* rpe = wgpu_mock_render_pass_encoder_state();
    T(rpe->has_blend_constant);
    T(rpe->blend_constant.r == 0.25);
    T(rpe->blend_constant.g == 0.5);
    T(rpe->blend_constant.b == 0.75);
    T(rpe->blend_constant.a == 1.0);
    T(rpe->has_stencil_reference);
    T(rpe->stencil_reference == 7);
    sg_apply_pipeline(pip1);
    T(rpe->blend_constant.r == 1.0);
    T(rpe->blend_constant.g == 0.0);
    T(rpe->blend_constant.a == 0.25);
    T(rpe->stencil_reference == 9);
    T(wgpu_mock_count_calls(F(wgpuRenderPassEncoderSetBlendConstant)) == 2);
    T(wgpu_mock_count_calls(F(wgpuRenderPassEncoderSetStencilReference)) == 2);
    sg_end_pass();
    sg_commit();
    sg_destroy_pipeline(pip1);
    sg_destroy_pipeline(pip0);
    sg_destroy_shader(shd);
    discard_swapchain(&sc);
    teardown();
}

//== bindings and bindgroup cache ==============================================
// shader with one texture view at WGSL binding 0 and one sampler at binding 1
static sg_shader make_tex_shader(void) {
    return sg_make_shader(&(sg_shader_desc){
        .vertex_func = { .source = vs_src, .entry = "vs_main" },
        .fragment_func = { .source = fs_src, .entry = "fs_main" },
        .views[0].texture = { .stage = SG_SHADERSTAGE_FRAGMENT, .image_type = SG_IMAGETYPE_2D, .sample_type = SG_IMAGESAMPLETYPE_FLOAT, .wgsl_group1_binding_n = 0 },
        .samplers[0] = { .stage = SG_SHADERSTAGE_FRAGMENT, .sampler_type = SG_SAMPLERTYPE_FILTERING, .wgsl_group1_binding_n = 1 },
        .texture_sampler_pairs[0] = { .stage = SG_SHADERSTAGE_FRAGMENT, .view_slot = 0, .sampler_slot = 0, .glsl_name = "tex_smp" },
        .label = "tex-shd",
    });
}

static sg_sampler make_linear_sampler(void) {
    return sg_make_sampler(&(sg_sampler_desc){ .min_filter = SG_FILTER_LINEAR, .mag_filter = SG_FILTER_LINEAR });
}

static sg_frame_stats_wgpu_bindings bstats(void) {
    return sg_query_stats().cur_frame.wgpu.bindings;
}

UTEST(sokol_gfx_wgpu, bindgroup_cache_hits_and_redundant_skips) {
    setup();
    sg_enable_stats();
    swapchain_t sc = make_swapchain(64, 64, 1, true);
    sg_shader shd = make_tex_shader();
    sg_pipeline pip = make_pipeline(shd);
    sg_buffer vbuf = make_vbuf();
    sg_image img = make_2d_image();
    sg_view view = make_texture_view(img);
    sg_sampler smp0 = make_linear_sampler();
    sg_sampler smp1 = make_linear_sampler();
    sg_begin_pass(&(sg_pass){ .swapchain = swapchain(&sc) });
    sg_apply_pipeline(pip);
    const wgpu_mock_render_pass_encoder_state_t* rpe = wgpu_mock_render_pass_encoder_state();
    sg_apply_bindings(&(sg_bindings){ .vertex_buffers[0] = vbuf, .views[0] = view, .samplers[0] = smp0 });
    sg_draw(0, 3, 1);
    sg_frame_stats_wgpu_bindings st = bstats();
    T(st.num_bindgroup_cache_misses == 1);
    T(st.num_bindgroup_cache_hits == 0);
    T(st.num_create_bindgroup == 1);
    T(st.num_set_bindgroup == 2);
    T(st.num_set_vertex_buffer == 1);
    T(rpe->bind_groups[1].group != 0);
    // identical bindings: cache hit, bindgroup and vertex buffer are not set again
    sg_apply_bindings(&(sg_bindings){ .vertex_buffers[0] = vbuf, .views[0] = view, .samplers[0] = smp0 });
    sg_draw(0, 3, 1);
    st = bstats();
    T(st.num_bindgroup_cache_hits == 1);
    T(st.num_skip_redundant_bindgroup == 1);
    T(st.num_set_bindgroup == 2);
    T(st.num_set_vertex_buffer == 1);
    T(st.num_skip_redundant_vertex_buffer >= 1);
    T(rpe->bind_groups[1].num_set_calls == 2);
    // different sampler: new bindgroup
    sg_apply_bindings(&(sg_bindings){ .vertex_buffers[0] = vbuf, .views[0] = view, .samplers[0] = smp1 });
    sg_draw(0, 3, 1);
    st = bstats();
    T(st.num_bindgroup_cache_misses == 2);
    T(st.num_create_bindgroup == 2);
    T(st.num_set_bindgroup == 3);
    // back to the first sampler: cached bindgroup is set again
    sg_apply_bindings(&(sg_bindings){ .vertex_buffers[0] = vbuf, .views[0] = view, .samplers[0] = smp0 });
    sg_draw(0, 3, 1);
    st = bstats();
    T(st.num_bindgroup_cache_hits == 2);
    T(st.num_create_bindgroup == 2);
    T(st.num_set_bindgroup == 4);
    T(rpe->num_draws == 4);
    sg_end_pass();
    sg_commit();

    // the cache survives frames
    sg_begin_pass(&(sg_pass){ .swapchain = swapchain(&sc) });
    sg_apply_pipeline(pip);
    sg_apply_bindings(&(sg_bindings){ .vertex_buffers[0] = vbuf, .views[0] = view, .samplers[0] = smp1 });
    sg_draw(0, 3, 1);
    st = bstats();
    T(st.num_bindgroup_cache_hits == 1);
    T(st.num_bindgroup_cache_misses == 0);
    T(st.num_create_bindgroup == 0);
    sg_end_pass();
    sg_commit();
    sg_destroy_sampler(smp1);
    sg_destroy_sampler(smp0);
    sg_destroy_view(view);
    sg_destroy_image(img);
    sg_destroy_buffer(vbuf);
    sg_destroy_pipeline(pip);
    sg_destroy_shader(shd);
    discard_swapchain(&sc);
    teardown();
}

UTEST(sokol_gfx_wgpu, bindgroup_cache_disabled) {
    setup_with((setup_desc_t){ .disable_bindgroups_cache = true });
    sg_enable_stats();
    swapchain_t sc = make_swapchain(64, 64, 1, true);
    sg_shader shd = make_tex_shader();
    sg_pipeline pip = make_pipeline(shd);
    sg_buffer vbuf = make_vbuf();
    sg_image img = make_2d_image();
    sg_view view = make_texture_view(img);
    sg_sampler smp = make_linear_sampler();
    // only the uniform bindgroup of the shader exists
    const int baseline = wgpu_mock_live_objects(WGPU_MOCK_OBJ_BIND_GROUP);
    sg_begin_pass(&(sg_pass){ .swapchain = swapchain(&sc) });
    sg_apply_pipeline(pip);
    for (int i = 0; i < 2; i++) {
        sg_apply_bindings(&(sg_bindings){ .vertex_buffers[0] = vbuf, .views[0] = view, .samplers[0] = smp });
        sg_draw(0, 3, 1);
        T(wgpu_mock_live_objects(WGPU_MOCK_OBJ_BIND_GROUP) == baseline);
    }
    const sg_frame_stats_wgpu_bindings st = bstats();
    T(st.num_create_bindgroup == 2);
    T(st.num_discard_bindgroup == 2);
    T(st.num_bindgroup_cache_hits == 0);
    T(st.num_bindgroup_cache_misses == 0);
    T(st.num_set_bindgroup == 3);
    sg_end_pass();
    sg_commit();
    sg_destroy_sampler(smp);
    sg_destroy_view(view);
    sg_destroy_image(img);
    sg_destroy_buffer(vbuf);
    sg_destroy_pipeline(pip);
    sg_destroy_shader(shd);
    discard_swapchain(&sc);
    teardown();
}

UTEST(sokol_gfx_wgpu, bindgroup_cache_collisions) {
    setup_with((setup_desc_t){ .bindgroups_cache_size = 2 });
    sg_enable_stats();
    swapchain_t sc = make_swapchain(64, 64, 1, true);
    sg_shader shd = make_tex_shader();
    sg_pipeline pip = make_pipeline(shd);
    sg_buffer vbuf = make_vbuf();
    sg_image img = make_2d_image();
    sg_view view = make_texture_view(img);
    sg_sampler smps[6];
    for (int i = 0; i < 6; i++) {
        smps[i] = make_linear_sampler();
    }
    sg_begin_pass(&(sg_pass){ .swapchain = swapchain(&sc) });
    sg_apply_pipeline(pip);
    for (int i = 0; i < 6; i++) {
        sg_apply_bindings(&(sg_bindings){ .vertex_buffers[0] = vbuf, .views[0] = view, .samplers[0] = smps[i] });
        sg_draw(0, 3, 1);
    }
    // six different keys in two cache slots (pigeonhole) must collide
    const sg_frame_stats_wgpu_bindings st = bstats();
    T(st.num_create_bindgroup == 6);
    T(st.num_bindgroup_cache_hits == 0);
    T(st.num_bindgroup_cache_misses + st.num_bindgroup_cache_collisions == 6);
    T(st.num_bindgroup_cache_collisions >= 4);
    T(st.num_discard_bindgroup == st.num_bindgroup_cache_collisions);
    T(st.num_bindgroup_cache_hash_vs_key_mismatch == 0);
    // at most two cached bindgroups plus the shader's uniform bindgroup
    T(wgpu_mock_live_objects(WGPU_MOCK_OBJ_BIND_GROUP) <= 3);
    sg_end_pass();
    sg_commit();
    for (int i = 0; i < 6; i++) {
        sg_destroy_sampler(smps[i]);
    }
    sg_destroy_view(view);
    sg_destroy_image(img);
    sg_destroy_buffer(vbuf);
    sg_destroy_pipeline(pip);
    sg_destroy_shader(shd);
    discard_swapchain(&sc);
    teardown();
}

// one pass with a single draw, which puts a bindgroup into the cache
static void draw_with(swapchain_t* sc, sg_pipeline pip, sg_buffer vbuf, sg_view view, sg_sampler smp) {
    sg_begin_pass(&(sg_pass){ .swapchain = swapchain(sc) });
    sg_apply_pipeline(pip);
    sg_apply_bindings(&(sg_bindings){ .vertex_buffers[0] = vbuf, .views[0] = view, .samplers[0] = smp });
    sg_draw(0, 3, 1);
    sg_end_pass();
}

// destroying a sampler (WGSL binding 1), pipeline or view (binding 0) must
// discard the cached bindgroups which reference it
UTEST(sokol_gfx_wgpu, bindgroup_cache_invalidate) {
    setup();
    sg_enable_stats();
    swapchain_t sc = make_swapchain(64, 64, 1, true);
    sg_shader shd = make_tex_shader();
    sg_pipeline pip = make_pipeline(shd);
    sg_buffer vbuf = make_vbuf();
    sg_image img = make_2d_image();
    sg_view view = make_texture_view(img);
    sg_sampler smp = make_linear_sampler();
    const int baseline = wgpu_mock_live_objects(WGPU_MOCK_OBJ_BIND_GROUP);

    draw_with(&sc, pip, vbuf, view, smp);
    T(wgpu_mock_live_objects(WGPU_MOCK_OBJ_BIND_GROUP) == baseline + 1);
    T(wgpu_mock_live_objects(WGPU_MOCK_OBJ_SAMPLER) == 1);
    sg_destroy_sampler(smp);
    T(bstats().num_bindgroup_cache_invalidates == 1);
    T(bstats().num_discard_bindgroup == 1);
    T(wgpu_mock_live_objects(WGPU_MOCK_OBJ_BIND_GROUP) == baseline);
    T(wgpu_mock_live_objects(WGPU_MOCK_OBJ_SAMPLER) == 0);

    sg_sampler smp2 = make_linear_sampler();
    draw_with(&sc, pip, vbuf, view, smp2);
    T(wgpu_mock_live_objects(WGPU_MOCK_OBJ_BIND_GROUP) == baseline + 1);
    sg_destroy_pipeline(pip);
    T(bstats().num_bindgroup_cache_invalidates == 2);
    T(wgpu_mock_live_objects(WGPU_MOCK_OBJ_BIND_GROUP) == baseline);

    sg_pipeline pip2 = make_pipeline(shd);
    draw_with(&sc, pip2, vbuf, view, smp2);
    T(wgpu_mock_live_objects(WGPU_MOCK_OBJ_BIND_GROUP) == baseline + 1);
    sg_destroy_view(view);
    T(bstats().num_bindgroup_cache_invalidates == 3);
    T(wgpu_mock_live_objects(WGPU_MOCK_OBJ_BIND_GROUP) == baseline);

    // nothing cached anymore, destroying the rest must not invalidate again
    sg_destroy_sampler(smp2);
    T(bstats().num_bindgroup_cache_invalidates == 3);
    sg_commit();
    sg_destroy_image(img);
    sg_destroy_buffer(vbuf);
    sg_destroy_pipeline(pip2);
    sg_destroy_shader(shd);
    discard_swapchain(&sc);
    teardown();
}

// a failed bindgroup must free its pool slot (pool size is 2 here)
UTEST(sokol_gfx_wgpu, bindgroup_create_fail_frees_pool_slot) {
    setup_with((setup_desc_t){ .disable_bindgroups_cache = true, .bindgroups_cache_size = 2 });
    swapchain_t sc = make_swapchain(64, 64, 1, true);
    sg_shader shd = make_tex_shader();
    sg_pipeline pip = make_pipeline(shd);
    sg_buffer vbuf = make_vbuf();
    sg_image img = make_2d_image();
    sg_view view = make_texture_view(img);
    sg_sampler smp = make_linear_sampler();
    sg_begin_pass(&(sg_pass){ .swapchain = swapchain(&sc) });
    sg_apply_pipeline(pip);
    wgpu_mock_fail_next(WGPU_MOCK_OBJ_BIND_GROUP, 100);
    for (int i = 0; i < 6; i++) {
        reset_log();
        // a failed apply_bindings invalidates the next draw until the pipeline is applied again
        sg_apply_pipeline(pip);
        sg_apply_bindings(&(sg_bindings){ .vertex_buffers[0] = vbuf, .views[0] = view, .samplers[0] = smp });
        T(logged(SG_LOGITEM_WGPU_CREATEBINDGROUP_FAILED));
        T(!logged(SG_LOGITEM_WGPU_BINDGROUPS_POOL_EXHAUSTED));
    }
    wgpu_mock_fail_next(WGPU_MOCK_OBJ_BIND_GROUP, 0);
    reset_log();
    sg_apply_pipeline(pip);
    sg_apply_bindings(&(sg_bindings){ .vertex_buffers[0] = vbuf, .views[0] = view, .samplers[0] = smp });
    T(num_log_items == 0);
    sg_end_pass();
    sg_commit();
    sg_destroy_sampler(smp);
    sg_destroy_view(view);
    sg_destroy_image(img);
    sg_destroy_buffer(vbuf);
    sg_destroy_pipeline(pip);
    sg_destroy_shader(shd);
    discard_swapchain(&sc);
    teardown();
}

// a failed bindgroup skips the draw also with the cache disabled
UTEST(sokol_gfx_wgpu, bindgroup_create_fail_uncached) {
    setup_with((setup_desc_t){ .disable_bindgroups_cache = true });
    swapchain_t sc = make_swapchain(64, 64, 1, true);
    sg_shader shd = make_tex_shader();
    sg_pipeline pip = make_pipeline(shd);
    sg_buffer vbuf = make_vbuf();
    sg_image img = make_2d_image();
    sg_view view = make_texture_view(img);
    sg_sampler smp = make_linear_sampler();
    const int baseline = wgpu_mock_live_objects(WGPU_MOCK_OBJ_BIND_GROUP);
    sg_begin_pass(&(sg_pass){ .swapchain = swapchain(&sc) });
    sg_apply_pipeline(pip);
    wgpu_mock_fail_next(WGPU_MOCK_OBJ_BIND_GROUP, 1);
    sg_apply_bindings(&(sg_bindings){ .vertex_buffers[0] = vbuf, .views[0] = view, .samplers[0] = smp });
    T(logged(SG_LOGITEM_WGPU_CREATEBINDGROUP_FAILED));
    sg_draw(0, 3, 1);
    T(wgpu_mock_render_pass_encoder_state()->num_draws == 0);
    T(wgpu_mock_live_objects(WGPU_MOCK_OBJ_BIND_GROUP) == baseline);
    // retry works
    sg_apply_pipeline(pip);
    sg_apply_bindings(&(sg_bindings){ .vertex_buffers[0] = vbuf, .views[0] = view, .samplers[0] = smp });
    sg_draw(0, 3, 1);
    T(wgpu_mock_render_pass_encoder_state()->num_draws == 1);
    sg_end_pass();
    sg_commit();
    sg_destroy_sampler(smp);
    sg_destroy_view(view);
    sg_destroy_image(img);
    sg_destroy_buffer(vbuf);
    sg_destroy_pipeline(pip);
    sg_destroy_shader(shd);
    discard_swapchain(&sc);
    teardown();
}

// a failed bindgroup is not cached: the draw is skipped, a retry works
UTEST(sokol_gfx_wgpu, bindgroup_cache_create_fail) {
    setup();
    sg_enable_stats();
    swapchain_t sc = make_swapchain(64, 64, 1, true);
    sg_shader shd = make_tex_shader();
    sg_pipeline pip = make_pipeline(shd);
    sg_buffer vbuf = make_vbuf();
    sg_image img = make_2d_image();
    sg_view view = make_texture_view(img);
    sg_sampler smp = make_linear_sampler();
    const int baseline = wgpu_mock_live_objects(WGPU_MOCK_OBJ_BIND_GROUP);
    sg_begin_pass(&(sg_pass){ .swapchain = swapchain(&sc) });
    sg_apply_pipeline(pip);
    wgpu_mock_fail_next(WGPU_MOCK_OBJ_BIND_GROUP, 1);
    sg_apply_bindings(&(sg_bindings){ .vertex_buffers[0] = vbuf, .views[0] = view, .samplers[0] = smp });
    T(logged(SG_LOGITEM_WGPU_CREATEBINDGROUP_FAILED));
    sg_draw(0, 3, 1);
    T(wgpu_mock_render_pass_encoder_state()->num_draws == 0);
    T(wgpu_mock_live_objects(WGPU_MOCK_OBJ_BIND_GROUP) == baseline);
    // retry with the same bindings: cache miss again, bindgroup is created
    sg_apply_pipeline(pip);
    sg_apply_bindings(&(sg_bindings){ .vertex_buffers[0] = vbuf, .views[0] = view, .samplers[0] = smp });
    sg_draw(0, 3, 1);
    T(wgpu_mock_render_pass_encoder_state()->num_draws == 1);
    T(bstats().num_bindgroup_cache_misses == 2);
    T(bstats().num_bindgroup_cache_hits == 0);
    T(wgpu_mock_live_objects(WGPU_MOCK_OBJ_BIND_GROUP) == baseline + 1);
    sg_end_pass();
    sg_commit();
    sg_destroy_sampler(smp);
    sg_destroy_view(view);
    sg_destroy_image(img);
    sg_destroy_buffer(vbuf);
    sg_destroy_pipeline(pip);
    sg_destroy_shader(shd);
    discard_swapchain(&sc);
    teardown();
}

UTEST(sokol_gfx_wgpu, bindings_vertex_and_index_buffers) {
    setup_with((setup_desc_t){ .disable_validation = true });
    sg_enable_stats();
    swapchain_t sc = make_swapchain(64, 64, 1, true);
    sg_shader shd = make_shader();
    sg_pipeline pip = sg_make_pipeline(&(sg_pipeline_desc){
        .shader = shd,
        .layout.attrs[0].format = SG_VERTEXFORMAT_FLOAT3,
        .layout.attrs[1] = { .format = SG_VERTEXFORMAT_FLOAT3, .buffer_index = 1 },
        .index_type = SG_INDEXTYPE_UINT16,
        .depth.pixel_format = SG_PIXELFORMAT_DEPTH_STENCIL,
        .colors[0].pixel_format = SG_PIXELFORMAT_BGRA8,
    });
    sg_buffer vb0 = make_vbuf();
    sg_buffer vb1 = make_vbuf();
    sg_buffer ib = make_ibuf();
    T(sg_query_pipeline_state(pip) == SG_RESOURCESTATE_VALID);
    sg_begin_pass(&(sg_pass){ .swapchain = swapchain(&sc) });
    sg_apply_pipeline(pip);
    const wgpu_mock_render_pass_encoder_state_t* rpe = wgpu_mock_render_pass_encoder_state();
    sg_apply_bindings(&(sg_bindings){ .vertex_buffers = { vb0, vb1 }, .index_buffer = ib });
    sg_draw(0, 6, 1);
    sg_frame_stats_wgpu_bindings st = bstats();
    T(st.num_set_vertex_buffer == 2);
    T(st.num_set_index_buffer == 1);
    T(rpe->num_indexed_draws == 1);
    T(rpe->index_format == WGPUIndexFormat_Uint16);
    T(rpe->index_buffer.offset == 0);
    T(rpe->index_buffer.size == 64);
    T(rpe->vertex_buffers[0].size == 128);
    T(rpe->vertex_buffers[1].size == 128);
    // same bindings again: everything is skipped
    sg_apply_bindings(&(sg_bindings){ .vertex_buffers = { vb0, vb1 }, .index_buffer = ib });
    st = bstats();
    T(st.num_set_vertex_buffer == 2);
    T(st.num_set_index_buffer == 1);
    T(st.num_skip_redundant_index_buffer >= 1);
    // offsets are part of the cached state, and shrink the bound range
    sg_apply_bindings(&(sg_bindings){
        .vertex_buffers = { vb0, vb1 },
        .vertex_buffer_offsets = { 16, 0 },
        .index_buffer = ib,
        .index_buffer_offset = 32,
    });
    st = bstats();
    T(st.num_set_vertex_buffer == 3);
    T(st.num_set_index_buffer == 2);
    T(rpe->vertex_buffers[0].offset == 16);
    T(rpe->vertex_buffers[0].size == 112);
    T(rpe->index_buffer.offset == 32);
    T(rpe->index_buffer.size == 32);
    // switching to a pipeline with one vertex buffer unbinds the other slot
    sg_pipeline pip2 = sg_make_pipeline(&(sg_pipeline_desc){
        .shader = shd,
        .layout.attrs[0].format = SG_VERTEXFORMAT_FLOAT3,
        .index_type = SG_INDEXTYPE_UINT16,
        .depth.pixel_format = SG_PIXELFORMAT_DEPTH_STENCIL,
        .colors[0].pixel_format = SG_PIXELFORMAT_BGRA8,
    });
    sg_apply_pipeline(pip2);
    const int n = wgpu_mock_count_calls(F(wgpuRenderPassEncoderSetVertexBuffer));
    sg_apply_bindings(&(sg_bindings){
        .vertex_buffers = { vb0 },
        .vertex_buffer_offsets = { 16 },
        .index_buffer = ib,
        .index_buffer_offset = 32,
    });
    T(wgpu_mock_count_calls(F(wgpuRenderPassEncoderSetVertexBuffer)) == n + 1);
    const wgpu_mock_call_t* c = wgpu_mock_last_call(F(wgpuRenderPassEncoderSetVertexBuffer));
    T(c);
    if (c) {
        T(c->args[0].u == 1);
        T(c->args[1].p == 0);
    }
    sg_end_pass();
    sg_commit();
    wgpu_mock_clear_errors();
    sg_destroy_buffer(ib);
    sg_destroy_buffer(vb1);
    sg_destroy_buffer(vb0);
    sg_destroy_pipeline(pip2);
    sg_destroy_pipeline(pip);
    sg_destroy_shader(shd);
    discard_swapchain(&sc);
    teardown();
}

UTEST(sokol_gfx_wgpu, reset_state_cache) {
    setup();
    sg_enable_stats();
    swapchain_t sc = make_swapchain(64, 64, 1, true);
    sg_shader shd = make_tex_shader();
    sg_pipeline pip = make_pipeline(shd);
    sg_buffer vbuf = make_vbuf();
    sg_image img = make_2d_image();
    sg_view view = make_texture_view(img);
    sg_sampler smp = make_linear_sampler();
    sg_begin_pass(&(sg_pass){ .swapchain = swapchain(&sc) });
    sg_apply_pipeline(pip);
    sg_apply_bindings(&(sg_bindings){ .vertex_buffers[0] = vbuf, .views[0] = view, .samplers[0] = smp });
    T(bstats().num_set_vertex_buffer == 1);
    T(bstats().num_set_bindgroup == 2);
    sg_reset_state_cache();
    sg_apply_pipeline(pip);
    sg_apply_bindings(&(sg_bindings){ .vertex_buffers[0] = vbuf, .views[0] = view, .samplers[0] = smp });
    sg_draw(0, 3, 1);
    // without the reset these would have been skipped as redundant
    T(bstats().num_set_vertex_buffer == 2);
    T(bstats().num_set_bindgroup == 3);
    sg_end_pass();
    sg_commit();
    sg_destroy_sampler(smp);
    sg_destroy_view(view);
    sg_destroy_image(img);
    sg_destroy_buffer(vbuf);
    sg_destroy_pipeline(pip);
    sg_destroy_shader(shd);
    discard_swapchain(&sc);
    teardown();
}

UTEST(sokol_gfx_wgpu, commit_without_pass_and_missing_commit) {
    setup();
    // commit without any pass in the frame does nothing
    sg_commit();
    T(wgpu_mock_count_calls(F(wgpuQueueSubmit)) == 0);
    // a pass without a commit: sg_shutdown() must release the command encoder
    swapchain_t sc = make_swapchain(64, 64, 1, true);
    sg_begin_pass(&(sg_pass){ .swapchain = swapchain(&sc) });
    sg_end_pass();
    discard_swapchain(&sc);
    teardown();
}

// KNOWN WART: the uniform bindgroup window is always 64 KB, but only the
// assert on the total buffer size (uniform_buffer_size + 64 KB) is checked
// in sg_apply_uniforms(). Writing more than uniform_buffer_size per frame
// results in a dynamic offset which moves the window beyond the buffer end
UTEST(sokol_gfx_wgpu, uniforms_beyond_buffer_size_wart) {
    setup_with((setup_desc_t){ .uniform_buffer_size = 1024 });
    swapchain_t sc = make_swapchain(64, 64, 1, true);
    sg_shader shd = make_ub_shader();
    sg_pipeline pip = make_pipeline(shd);
    sg_buffer vbuf = make_vbuf();
    sg_begin_pass(&(sg_pass){ .swapchain = swapchain(&sc) });
    sg_apply_pipeline(pip);
    sg_apply_bindings(&(sg_bindings){ .vertex_buffers[0] = vbuf });
    // up to a dynamic offset of uniform_buffer_size is fine
    sg_apply_uniforms(1, &(sg_range){ .ptr = scratch, .size = 32 });
    for (int i = 0; i < 4; i++) {
        sg_apply_uniforms(0, &(sg_range){ .ptr = scratch, .size = 16 });
    }
    sg_draw(0, 3, 1);
    T(wgpu_mock_total_errors() == 0);
    // the next offset is 1280: within the buffer, but its 64 KB window is not
    sg_apply_uniforms(0, &(sg_range){ .ptr = scratch, .size = 16 });
    sg_draw(0, 3, 1);
    T(wgpu_mock_find_error("beyond the buffer size") >= 0);
    wgpu_mock_clear_errors();
    sg_end_pass();
    sg_commit();
    sg_destroy_buffer(vbuf);
    sg_destroy_pipeline(pip);
    sg_destroy_shader(shd);
    discard_swapchain(&sc);
    teardown();
}

//== depth-only attachment and remaining enum paths ============================
UTEST(sokol_gfx_wgpu, offscreen_pass_depth_only_attachment) {
    setup();
    sg_image color = make_color_image(32, 32, 1);
    sg_image depth = sg_make_image(&(sg_image_desc){
        .usage.depth_stencil_attachment = true,
        .width = 32,
        .height = 32,
        .pixel_format = SG_PIXELFORMAT_DEPTH,
    });
    sg_view color_view = sg_make_view(&(sg_view_desc){ .color_attachment.image = color });
    sg_view depth_view = sg_make_view(&(sg_view_desc){ .depth_stencil_attachment.image = depth });
    T(sg_query_view_state(depth_view) == SG_RESOURCESTATE_VALID);
    sg_begin_pass(&(sg_pass){ .attachments = { .colors[0] = color_view, .depth_stencil = depth_view } });
    const wgpu_mock_render_pass_info_t* rp = wgpu_mock_last_render_pass();
    T(rp->has_depth_stencil);
    // depth-only formats must not have stencil ops
    T(rp->stencil_load_op == WGPULoadOp_Undefined);
    T(rp->stencil_store_op == WGPUStoreOp_Undefined);
    // wart: read-only flags are always false
    T(!rp->depth_read_only);
    T(!rp->stencil_read_only);
    sg_end_pass();
    sg_commit();
    sg_destroy_view(depth_view);
    sg_destroy_view(color_view);
    sg_destroy_image(depth);
    sg_destroy_image(color);
    teardown();
}

UTEST(sokol_gfx_wgpu, pipeline_stencil_decr_wrap) {
    setup();
    sg_shader shd = make_shader();
    sg_pipeline pip = sg_make_pipeline(&(sg_pipeline_desc){
        .shader = shd,
        .layout.attrs[0].format = SG_VERTEXFORMAT_FLOAT3,
        .depth.pixel_format = SG_PIXELFORMAT_DEPTH_STENCIL,
        .stencil = {
            .enabled = true,
            .front.pass_op = SG_STENCILOP_DECR_WRAP,
            .back.fail_op = SG_STENCILOP_DECR_WRAP,
        },
        .colors[0].pixel_format = SG_PIXELFORMAT_BGRA8,
    });
    T(sg_query_pipeline_state(pip) == SG_RESOURCESTATE_VALID);
    wgpu_mock_render_pipeline_info_t pi;
    T(get_pip_info(pip, &pi));
    T(pi.depth_stencil.stencilFront.passOp == WGPUStencilOperation_DecrementWrap);
    T(pi.depth_stencil.stencilBack.failOp == WGPUStencilOperation_DecrementWrap);
    sg_destroy_pipeline(pip);
    sg_destroy_shader(shd);
    teardown();
}

UTEST_MAIN()
