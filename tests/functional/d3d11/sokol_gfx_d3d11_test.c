//------------------------------------------------------------------------------
//  LLM maintained.
//
//  sokol_gfx_d3d11_test.c
//  Focused tests for the sokol_gfx.h D3D11 backend, running against the mock
//  d3d11.h/d3dcompiler.h stubs in tests/mocks/d3d11. Only exercises paths
//  that are backend-specific -- generic public API behaviour is covered by
//  the DUMMY-backend suite in sokol_gfx_test.c and is not repeated here.
//
//  Rough coverage map (referenced sokol_gfx.h symbol on the right):
//    - Backend setup / feature level               _sg_d3d11_setup_backend / _sg_d3d11_init_caps
//    - Buffer create paths (all usage flavours)    _sg_d3d11_create_buffer / _sg_d3d11_buffer_*
//    - Image create paths (2D / 3D / cube / MSAA)  _sg_d3d11_create_image / _sg_d3d11_image_*
//    - Sampler create                              _sg_d3d11_create_sampler / _sg_d3d11_filter/address
//    - Shader create (source + bytecode + compute) _sg_d3d11_create_shader / _sg_d3d11_compile_shader
//    - Pipeline create (raster/depth/blend/topo)   _sg_d3d11_create_pipeline / _sg_d3d11_blend/stencil
//    - View creation (SRV / UAV / RTV / DSV)       _sg_d3d11_create_view
//    - Pass begin/end (swapchain, offscreen, MSAA) _sg_d3d11_begin_pass / _sg_d3d11_end_pass
//    - Draw / dispatch                             _sg_d3d11_draw / _sg_d3d11_dispatch
//    - Resource updates (Map / UpdateSubresource)  _sg_d3d11_update_* / _sg_d3d11_write_*
//    - Injected native handles                     sg_*_desc.d3d11_buffer / .d3d11_texture
//    - Error paths                                 CreateBuffer / CreateTexture / D3DCompile / DLL
//------------------------------------------------------------------------------
#define SOKOL_IMPL
#include "d3d11_mock.h"
#include "sokol_gfx.h"
#include "utest.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define T(b) EXPECT_TRUE(b)

//------------------------------------------------------------------------------
//  test harness
//------------------------------------------------------------------------------
#define MAX_LOG_ITEMS (64)
static sg_log_item log_items[MAX_LOG_ITEMS];
static uint32_t log_levels[MAX_LOG_ITEMS];
static int num_log_items;

static void reset_log(void) {
    num_log_items = 0;
    memset(log_items, 0, sizeof(log_items));
    memset(log_levels, 0, sizeof(log_levels));
}

static void capture_log(const char* tag, uint32_t log_level, uint32_t log_item_id, const char* msg, uint32_t line_nr, const char* file, void* ud) {
    (void)tag; (void)file; (void)ud;
    if (getenv("D3D11_TEST_VERBOSE")) {
        printf("  [log] level=%u item=%u line=%u: %s\n", log_level, log_item_id, line_nr, msg ? msg : "");
    }
    if (num_log_items < MAX_LOG_ITEMS) {
        log_levels[num_log_items] = log_level;
        log_items[num_log_items++] = (sg_log_item)log_item_id;
    }
}

static bool logged(sg_log_item item) {
    for (int i = 0; i < num_log_items; i++) {
        if (log_items[i] == item) { return true; }
    }
    return false;
}

// true if nothing was logged at panic (0) or error (1) level -- validation
// layer messages are logged at error level, so this also means 'no
// validation errors'
static bool no_errors(void) {
    for (int i = 0; i < num_log_items; i++) {
        if (log_levels[i] <= 1) { return false; }
    }
    return true;
}

static ID3D11Device* mock_dev;
static ID3D11DeviceContext* mock_ctx;

static void setup(void) {
    reset_log();
    d3d11_mock_reset();
    mock_dev = d3d11_mock_create_device();
    mock_ctx = d3d11_mock_get_device_context(mock_dev);
    sg_setup(&(sg_desc){
        .environment.d3d11.device = mock_dev,
        .environment.d3d11.device_context = mock_ctx,
        .logger.func = capture_log,
    });
}

static void teardown(void) {
    sg_shutdown();
    d3d11_mock_destroy_device(mock_dev);
    mock_dev = NULL;
    mock_ctx = NULL;
}

// native object lookup helpers
static const void* nbuf(sg_buffer b) { return sg_d3d11_query_buffer_info(b).buf; }
static const void* ntex(sg_image i) { return sg_d3d11_query_image_info(i).res; }
static const void* nsrv(sg_view v) { return sg_d3d11_query_view_info(v).srv; }
static const void* nuav(sg_view v) { return sg_d3d11_query_view_info(v).uav; }
static const void* nrtv(sg_view v) { return sg_d3d11_query_view_info(v).rtv; }
static const void* ndsv(sg_view v) { return sg_d3d11_query_view_info(v).dsv; }
static const void* nsmp(sg_sampler s) { return sg_d3d11_query_sampler_info(s).smp; }

// a 64x64 swapchain render pass without depth buffer (pipelines used inside
// must have .depth.pixel_format = SG_PIXELFORMAT_NONE)
static void begin_swapchain_pass(ID3D11RenderTargetView* rtv) {
    sg_begin_pass(&(sg_pass){
        .swapchain = { .width = 64, .height = 64, .sample_count = 1,
                       .color_format = SG_PIXELFORMAT_BGRA8, .depth_format = SG_PIXELFORMAT_NONE,
                       .d3d11 = { .render_view = rtv } },
    });
}

#define LAST(kind) d3d11_mock_last_call(D3D11_MOCK_CALL_##kind)
#define NTH(kind, n) d3d11_mock_nth_call(D3D11_MOCK_CALL_##kind, n)
#define COUNT(kind) d3d11_mock_count_calls(D3D11_MOCK_CALL_##kind)

//------------------------------------------------------------------------------
//  backend setup / features
//------------------------------------------------------------------------------
UTEST(sokol_gfx_d3d11, backend_is_d3d11) {
    setup();
    T(sg_isvalid());
    T(sg_query_backend() == SG_BACKEND_D3D11);
    // Mock reports feature level 11.1 -- storage-buffer limit takes the 11.1 branch.
    T(sg_query_limits().d3d11_max_unordered_access_views > 8);
    teardown();
}

UTEST(sokol_gfx_d3d11, format_caps_populated) {
    setup();
    // Colour formats should be sampleable and blendable.
    sg_pixelformat_info info = sg_query_pixelformat(SG_PIXELFORMAT_RGBA8);
    T(info.sample);
    T(info.filter);
    T(info.render);
    T(info.blend);
    // Depth format should report as depth, not colour-blendable.
    info = sg_query_pixelformat(SG_PIXELFORMAT_DEPTH_STENCIL);
    T(info.depth);
    T(info.render);
    // Mock never claims depth support for RGBA8.
    T(!sg_query_pixelformat(SG_PIXELFORMAT_RGBA8).depth);
    teardown();
}

//------------------------------------------------------------------------------
//  buffer creation code paths
//------------------------------------------------------------------------------
UTEST(sokol_gfx_d3d11, buffer_immutable_creates_native) {
    setup();
    const int before = d3d11_mock_live_object_count();
    static const float data[] = { 1, 2, 3, 4 };
    sg_buffer buf = sg_make_buffer(&(sg_buffer_desc){ .data = SG_RANGE(data) });
    T(sg_query_buffer_state(buf) == SG_RESOURCESTATE_VALID);
    // One CreateBuffer call -> one live mock buffer.
    T(d3d11_mock_live_object_count() == before + 1);
    sg_destroy_buffer(buf);
    T(d3d11_mock_live_object_count() == before);
    teardown();
}

UTEST(sokol_gfx_d3d11, buffer_dynamic_stream_transient) {
    setup();
    const int before = d3d11_mock_live_object_count();
    sg_buffer dyn = sg_make_buffer(&(sg_buffer_desc){ .size = 256, .usage.copy_dst = true });
    sg_buffer stm = sg_make_buffer(&(sg_buffer_desc){ .size = 256, .usage.write_unsealed = true });
    sg_buffer tra = sg_make_buffer(&(sg_buffer_desc){ .size = 256, .usage.write_transient = true });
    T(sg_query_buffer_state(dyn) == SG_RESOURCESTATE_VALID);
    T(sg_query_buffer_state(stm) == SG_RESOURCESTATE_UNSEALED);
    T(sg_query_buffer_state(tra) == SG_RESOURCESTATE_VALID);
    T(d3d11_mock_live_object_count() == before + 3);
    sg_destroy_buffer(dyn);
    sg_destroy_buffer(stm);
    sg_destroy_buffer(tra);
    T(d3d11_mock_live_object_count() == before);
    teardown();
}

UTEST(sokol_gfx_d3d11, buffer_storage_allocates_srv_and_uav) {
    setup();
    const int before = d3d11_mock_live_object_count();
    sg_buffer sb = sg_make_buffer(&(sg_buffer_desc){
        .size = 1024, .usage.storage_buffer = true,
    });
    T(sg_query_buffer_state(sb) == SG_RESOURCESTATE_VALID);
    // A storage buffer view creates SRV + UAV; without a view, only the buffer.
    T(d3d11_mock_live_object_count() == before + 1);
    sg_view sv = sg_make_view(&(sg_view_desc){ .storage_buffer.buffer = sb });
    T(sg_query_view_state(sv) == SG_RESOURCESTATE_VALID);
    T(d3d11_mock_live_object_count() == before + 3);   // buffer + SRV + UAV
    sg_destroy_view(sv);
    sg_destroy_buffer(sb);
    T(d3d11_mock_live_object_count() == before);
    teardown();
}

UTEST(sokol_gfx_d3d11, buffer_inject_native) {
    setup();
    // Sokol should adopt the caller's ID3D11Buffer via AddRef and skip the
    // CreateBuffer call. Live count grows only when we make the mock buffer.
    const int before = d3d11_mock_live_object_count();
    ID3D11Buffer* raw = NULL;
    D3D11_BUFFER_DESC bd = { .ByteWidth = 64, .BindFlags = D3D11_BIND_VERTEX_BUFFER };
    mock_dev->lpVtbl->CreateBuffer(mock_dev, &bd, NULL, &raw);
    T(d3d11_mock_live_object_count() == before + 1);
    sg_buffer buf = sg_make_buffer(&(sg_buffer_desc){
        .size = 64,
        .d3d11_buffer = raw,
    });
    T(sg_query_buffer_state(buf) == SG_RESOURCESTATE_VALID);
    // Buffer object refcount is now 2 (our ref + sokol's AddRef); live count
    // unchanged because no new object was allocated.
    T(d3d11_mock_live_object_count() == before + 1);
    sg_destroy_buffer(buf);
    T(d3d11_mock_live_object_count() == before + 1);   // our ref still holds
    raw->lpVtbl->Release(raw);
    T(d3d11_mock_live_object_count() == before);
    teardown();
}

UTEST(sokol_gfx_d3d11, buffer_create_fail_marks_state_failed) {
    setup();
    d3d11_mock_fail_next_create(1);
    sg_buffer buf = sg_make_buffer(&(sg_buffer_desc){ .size = 128, .usage.write_transient = true });
    T(sg_query_buffer_state(buf) == SG_RESOURCESTATE_FAILED);
    T(logged(SG_LOGITEM_D3D11_CREATE_BUFFER_FAILED));
    sg_destroy_buffer(buf);
    teardown();
}

//------------------------------------------------------------------------------
//  image creation code paths -- 2D / 3D / cube / array / MSAA
//------------------------------------------------------------------------------
UTEST(sokol_gfx_d3d11, image_2d_creates_texture2d) {
    setup();
    const int before = d3d11_mock_live_object_count();
    uint8_t pixels[16 * 16 * 4] = {0};
    sg_image img = sg_make_image(&(sg_image_desc){
        .width = 16, .height = 16,
        .pixel_format = SG_PIXELFORMAT_RGBA8,
        .data.mip_levels[0] = SG_RANGE(pixels),
    });
    T(sg_query_image_state(img) == SG_RESOURCESTATE_VALID);
    T(d3d11_mock_live_object_count() == before + 1);
    sg_destroy_image(img);
    T(d3d11_mock_live_object_count() == before);
    teardown();
}

UTEST(sokol_gfx_d3d11, image_3d_takes_texture3d_path) {
    setup();
    const int before = d3d11_mock_live_object_count();
    sg_image img = sg_make_image(&(sg_image_desc){
        .type = SG_IMAGETYPE_3D,
        .width = 8, .height = 8, .num_slices = 8,
        .pixel_format = SG_PIXELFORMAT_RGBA8,
        .usage.write_transient = true,
    });
    T(sg_query_image_state(img) == SG_RESOURCESTATE_VALID);
    T(d3d11_mock_live_object_count() == before + 1);
    sg_destroy_image(img);
    teardown();
}

UTEST(sokol_gfx_d3d11, image_cube_texture2d_arraysize6) {
    setup();
    sg_image img = sg_make_image(&(sg_image_desc){
        .type = SG_IMAGETYPE_CUBE,
        .width = 8, .height = 8, .num_slices = 6,
        .pixel_format = SG_PIXELFORMAT_RGBA8,
        .usage.color_attachment = true,
    });
    T(sg_query_image_state(img) == SG_RESOURCESTATE_VALID);
    sg_destroy_image(img);
    teardown();
}

UTEST(sokol_gfx_d3d11, image_2d_array_texture2d_arraysize) {
    setup();
    sg_image img = sg_make_image(&(sg_image_desc){
        .type = SG_IMAGETYPE_ARRAY,
        .width = 8, .height = 8, .num_slices = 4,
        .pixel_format = SG_PIXELFORMAT_RGBA8,
        .usage.color_attachment = true,
    });
    T(sg_query_image_state(img) == SG_RESOURCESTATE_VALID);
    sg_destroy_image(img);
    teardown();
}

UTEST(sokol_gfx_d3d11, image_msaa_color_attachment) {
    setup();
    sg_image img = sg_make_image(&(sg_image_desc){
        .width = 128, .height = 128,
        .pixel_format = SG_PIXELFORMAT_RGBA8,
        .sample_count = 4,
        .usage.color_attachment = true,
    });
    T(sg_query_image_state(img) == SG_RESOURCESTATE_VALID);
    sg_destroy_image(img);
    teardown();
}

UTEST(sokol_gfx_d3d11, image_depth_attachment) {
    setup();
    sg_image img = sg_make_image(&(sg_image_desc){
        .width = 64, .height = 64,
        .pixel_format = SG_PIXELFORMAT_DEPTH_STENCIL,
        .usage.depth_stencil_attachment = true,
    });
    T(sg_query_image_state(img) == SG_RESOURCESTATE_VALID);
    sg_destroy_image(img);
    teardown();
}

UTEST(sokol_gfx_d3d11, image_storage) {
    setup();
    sg_image img = sg_make_image(&(sg_image_desc){
        .width = 32, .height = 32,
        .pixel_format = SG_PIXELFORMAT_RGBA8,
        .usage.storage_image = true,
    });
    T(sg_query_image_state(img) == SG_RESOURCESTATE_VALID);
    sg_destroy_image(img);
    teardown();
}

UTEST(sokol_gfx_d3d11, image_compressed_bc3) {
    setup();
    uint8_t bc3[16 * 16] = {0}; // 4x4 blocks, one 16-byte block per 4x4 texels
    sg_image img = sg_make_image(&(sg_image_desc){
        .width = 16, .height = 16,
        .pixel_format = SG_PIXELFORMAT_BC3_RGBA,
        .data.mip_levels[0] = SG_RANGE(bc3),
    });
    T(sg_query_image_state(img) == SG_RESOURCESTATE_VALID);
    sg_destroy_image(img);
    teardown();
}

UTEST(sokol_gfx_d3d11, image_inject_native_texture) {
    setup();
    ID3D11Texture2D* raw = NULL;
    D3D11_TEXTURE2D_DESC td = { .Width = 16, .Height = 16, .MipLevels = 1, .ArraySize = 1,
                                .Format = DXGI_FORMAT_R8G8B8A8_UNORM, .SampleDesc = { .Count = 1 } };
    mock_dev->lpVtbl->CreateTexture2D(mock_dev, &td, NULL, &raw);
    const int before = d3d11_mock_live_object_count();
    sg_image img = sg_make_image(&(sg_image_desc){
        .width = 16, .height = 16,
        .pixel_format = SG_PIXELFORMAT_RGBA8,
        .usage.immutable = true,
        .d3d11_texture = raw,
    });
    T(sg_query_image_state(img) == SG_RESOURCESTATE_VALID);
    T(d3d11_mock_live_object_count() == before);   // no new texture allocated
    sg_destroy_image(img);
    raw->lpVtbl->Release(raw);
    teardown();
}

UTEST(sokol_gfx_d3d11, image_create_fail_marks_failed) {
    setup();
    d3d11_mock_fail_next_create(1);
    sg_image img = sg_make_image(&(sg_image_desc){
        .width = 16, .height = 16,
        .pixel_format = SG_PIXELFORMAT_RGBA8,
        .usage.write_transient = true,
    });
    T(sg_query_image_state(img) == SG_RESOURCESTATE_FAILED);
    T(logged(SG_LOGITEM_D3D11_CREATE_2D_TEXTURE_FAILED));
    sg_destroy_image(img);
    teardown();
}

UTEST(sokol_gfx_d3d11, image_3d_create_fail_marks_failed) {
    setup();
    d3d11_mock_fail_next_create(1);
    sg_image img = sg_make_image(&(sg_image_desc){
        .type = SG_IMAGETYPE_3D,
        .width = 8, .height = 8, .num_slices = 8,
        .pixel_format = SG_PIXELFORMAT_RGBA8,
        .usage.write_transient = true,
    });
    T(sg_query_image_state(img) == SG_RESOURCESTATE_FAILED);
    T(logged(SG_LOGITEM_D3D11_CREATE_3D_TEXTURE_FAILED));
    sg_destroy_image(img);
    teardown();
}

//------------------------------------------------------------------------------
//  sampler creation -- exercise filter / address / compare combinations
//------------------------------------------------------------------------------
UTEST(sokol_gfx_d3d11, sampler_variants) {
    setup();
    sg_sampler s1 = sg_make_sampler(&(sg_sampler_desc){
        .min_filter = SG_FILTER_LINEAR, .mag_filter = SG_FILTER_LINEAR, .mipmap_filter = SG_FILTER_LINEAR,
        .wrap_u = SG_WRAP_REPEAT, .wrap_v = SG_WRAP_MIRRORED_REPEAT, .wrap_w = SG_WRAP_CLAMP_TO_EDGE,
        .max_anisotropy = 16,
    });
    sg_sampler s2 = sg_make_sampler(&(sg_sampler_desc){
        .compare = SG_COMPAREFUNC_LESS_EQUAL,
        .wrap_u = SG_WRAP_CLAMP_TO_BORDER,
        .border_color = SG_BORDERCOLOR_OPAQUE_WHITE,
    });
    T(sg_query_sampler_state(s1) == SG_RESOURCESTATE_VALID);
    T(sg_query_sampler_state(s2) == SG_RESOURCESTATE_VALID);
    sg_destroy_sampler(s1);
    sg_destroy_sampler(s2);
    teardown();
}

UTEST(sokol_gfx_d3d11, sampler_create_fail) {
    setup();
    d3d11_mock_fail_next_create(1);
    sg_sampler s = sg_make_sampler(&(sg_sampler_desc){0});
    T(sg_query_sampler_state(s) == SG_RESOURCESTATE_FAILED);
    T(logged(SG_LOGITEM_D3D11_CREATE_SAMPLER_STATE_FAILED));
    sg_destroy_sampler(s);
    teardown();
}

//------------------------------------------------------------------------------
//  shader creation -- HLSL source, precompiled bytecode, compute, errors
//------------------------------------------------------------------------------
UTEST(sokol_gfx_d3d11, shader_from_hlsl_source_compiles) {
    setup();
    sg_shader shd = sg_make_shader(&(sg_shader_desc){
        .vertex_func = { .source = "vs", .d3d11_target = "vs_5_0" },
        .fragment_func = { .source = "ps", .d3d11_target = "ps_5_0" },
        .attrs = { [0] = { .hlsl_sem_name = "POSITION" } },
    });
    T(sg_query_shader_state(shd) == SG_RESOURCESTATE_VALID);
    sg_destroy_shader(shd);
    teardown();
}

UTEST(sokol_gfx_d3d11, shader_from_bytecode_skips_compiler) {
    setup();
    static const uint8_t vs_code[128] = {0};
    static const uint8_t fs_code[128] = {0};
    // Force the DLL loader to fail; with bytecode we should never reach it.
    d3d11_mock_fail_d3dcompiler_dll(true);
    sg_shader shd = sg_make_shader(&(sg_shader_desc){
        .vertex_func   = { .bytecode = SG_RANGE(vs_code) },
        .fragment_func = { .bytecode = SG_RANGE(fs_code) },
        .attrs = { [0] = { .hlsl_sem_name = "POSITION" } },
    });
    T(sg_query_shader_state(shd) == SG_RESOURCESTATE_VALID);
    T(!logged(SG_LOGITEM_D3D11_LOAD_D3DCOMPILER_47_DLL_FAILED));
    sg_destroy_shader(shd);
    teardown();
}

UTEST(sokol_gfx_d3d11, shader_source_dll_missing_fails) {
    setup();
    d3d11_mock_fail_d3dcompiler_dll(true);
    sg_shader shd = sg_make_shader(&(sg_shader_desc){
        .vertex_func = { .source = "vs", .d3d11_target = "vs_5_0" },
        .fragment_func = { .source = "ps", .d3d11_target = "ps_5_0" },
        .attrs = { [0] = { .hlsl_sem_name = "POSITION" } },
    });
    T(sg_query_shader_state(shd) == SG_RESOURCESTATE_FAILED);
    T(logged(SG_LOGITEM_D3D11_LOAD_D3DCOMPILER_47_DLL_FAILED));
    sg_destroy_shader(shd);
    teardown();
}

UTEST(sokol_gfx_d3d11, shader_source_compile_fails) {
    setup();
    d3d11_mock_fail_next_compile(2);  // both VS and FS compile calls fail
    sg_shader shd = sg_make_shader(&(sg_shader_desc){
        .vertex_func = { .source = "vs", .d3d11_target = "vs_5_0" },
        .fragment_func = { .source = "ps", .d3d11_target = "ps_5_0" },
        .attrs = { [0] = { .hlsl_sem_name = "POSITION" } },
    });
    T(sg_query_shader_state(shd) == SG_RESOURCESTATE_FAILED);
    T(logged(SG_LOGITEM_D3D11_SHADER_COMPILATION_FAILED));
    sg_destroy_shader(shd);
    teardown();
}

UTEST(sokol_gfx_d3d11, shader_compute) {
    setup();
    sg_shader shd = sg_make_shader(&(sg_shader_desc){
        .compute_func = { .source = "cs", .d3d11_target = "cs_5_0" },
    });
    T(sg_query_shader_state(shd) == SG_RESOURCESTATE_VALID);
    sg_destroy_shader(shd);
    teardown();
}

UTEST(sokol_gfx_d3d11, shader_with_uniform_blocks_creates_cbuffers) {
    setup();
    const int before = d3d11_mock_live_object_count();
    sg_shader shd = sg_make_shader(&(sg_shader_desc){
        .vertex_func = { .source = "vs", .d3d11_target = "vs_5_0" },
        .fragment_func = { .source = "ps", .d3d11_target = "ps_5_0" },
        .attrs = { [0] = { .hlsl_sem_name = "POSITION" } },
        .uniform_blocks = {
            [0] = {
                .stage = SG_SHADERSTAGE_VERTEX,
                .size = 64,
                .hlsl_register_b_n = 0,
            },
            [1] = {
                .stage = SG_SHADERSTAGE_FRAGMENT,
                .size = 32,
                .hlsl_register_b_n = 0,
            },
        },
    });
    T(sg_query_shader_state(shd) == SG_RESOURCESTATE_VALID);
    // 2 constant buffers + VS + PS = 4 new mock objects.
    T(d3d11_mock_live_object_count() == before + 4);
    sg_destroy_shader(shd);
    T(d3d11_mock_live_object_count() == before);
    teardown();
}

//------------------------------------------------------------------------------
//  pipeline creation -- input layout, rasterizer, DSS, blend, topology
//------------------------------------------------------------------------------
static sg_shader make_test_shader(void) {
    return sg_make_shader(&(sg_shader_desc){
        .vertex_func = { .source = "vs", .d3d11_target = "vs_5_0" },
        .fragment_func = { .source = "ps", .d3d11_target = "ps_5_0" },
        .attrs = { [0] = { .hlsl_sem_name = "POSITION" }, [1] = { .hlsl_sem_name = "COLOR" } },
    });
}

UTEST(sokol_gfx_d3d11, pipeline_creates_il_rs_dss_bs) {
    setup();
    sg_shader shd = make_test_shader();
    const int before = d3d11_mock_live_object_count();
    sg_pipeline pip = sg_make_pipeline(&(sg_pipeline_desc){
        .shader = shd,
        .layout = {
            .attrs = {
                [0] = { .format = SG_VERTEXFORMAT_FLOAT3 },
                [1] = { .format = SG_VERTEXFORMAT_UBYTE4N },
            },
        },
        .depth = { .compare = SG_COMPAREFUNC_LESS_EQUAL, .write_enabled = true },
        .colors = { [0] = { .blend = { .enabled = true, .src_factor_rgb = SG_BLENDFACTOR_SRC_ALPHA, .dst_factor_rgb = SG_BLENDFACTOR_ONE_MINUS_SRC_ALPHA } } },
        .cull_mode = SG_CULLMODE_BACK,
        .face_winding = SG_FACEWINDING_CCW,
    });
    T(sg_query_pipeline_state(pip) == SG_RESOURCESTATE_VALID);
    // InputLayout + RasterizerState + DepthStencilState + BlendState = 4.
    T(d3d11_mock_live_object_count() == before + 4);
    sg_destroy_pipeline(pip);
    sg_destroy_shader(shd);
    teardown();
}

UTEST(sokol_gfx_d3d11, pipeline_all_primitive_types) {
    setup();
    sg_shader shd = make_test_shader();
    const sg_primitive_type prims[] = {
        SG_PRIMITIVETYPE_POINTS,
        SG_PRIMITIVETYPE_LINES,
        SG_PRIMITIVETYPE_LINE_STRIP,
        SG_PRIMITIVETYPE_TRIANGLES,
        SG_PRIMITIVETYPE_TRIANGLE_STRIP,
    };
    for (int i = 0; i < (int)(sizeof(prims)/sizeof(prims[0])); i++) {
        sg_pipeline pip = sg_make_pipeline(&(sg_pipeline_desc){
            .shader = shd, .primitive_type = prims[i],
            .layout = { .attrs = { [0] = { .format = SG_VERTEXFORMAT_FLOAT3 }, [1] = { .format = SG_VERTEXFORMAT_UBYTE4N } } },
        });
        T(sg_query_pipeline_state(pip) == SG_RESOURCESTATE_VALID);
        sg_destroy_pipeline(pip);
    }
    sg_destroy_shader(shd);
    teardown();
}

UTEST(sokol_gfx_d3d11, pipeline_index_uint16_uint32) {
    setup();
    sg_shader shd = make_test_shader();
    sg_pipeline p16 = sg_make_pipeline(&(sg_pipeline_desc){
        .shader = shd, .index_type = SG_INDEXTYPE_UINT16,
        .layout = { .attrs = { [0] = { .format = SG_VERTEXFORMAT_FLOAT3 }, [1] = { .format = SG_VERTEXFORMAT_UBYTE4N } } },
    });
    sg_pipeline p32 = sg_make_pipeline(&(sg_pipeline_desc){
        .shader = shd, .index_type = SG_INDEXTYPE_UINT32,
        .layout = { .attrs = { [0] = { .format = SG_VERTEXFORMAT_FLOAT3 }, [1] = { .format = SG_VERTEXFORMAT_UBYTE4N } } },
    });
    T(sg_query_pipeline_state(p16) == SG_RESOURCESTATE_VALID);
    T(sg_query_pipeline_state(p32) == SG_RESOURCESTATE_VALID);
    sg_destroy_pipeline(p16);
    sg_destroy_pipeline(p32);
    sg_destroy_shader(shd);
    teardown();
}

UTEST(sokol_gfx_d3d11, pipeline_input_layout_fail) {
    setup();
    sg_shader shd = make_test_shader();
    // Fail order at pipeline create: CreateInputLayout is the 1st CreateXxx.
    d3d11_mock_fail_next_create(1);
    sg_pipeline pip = sg_make_pipeline(&(sg_pipeline_desc){
        .shader = shd,
        .layout = { .attrs = { [0] = { .format = SG_VERTEXFORMAT_FLOAT3 } } },
    });
    T(sg_query_pipeline_state(pip) == SG_RESOURCESTATE_FAILED);
    T(logged(SG_LOGITEM_D3D11_CREATE_INPUT_LAYOUT_FAILED));
    sg_destroy_pipeline(pip);
    sg_destroy_shader(shd);
    teardown();
}

UTEST(sokol_gfx_d3d11, pipeline_compute_no_input_layout) {
    setup();
    sg_shader cshd = sg_make_shader(&(sg_shader_desc){
        .compute_func = { .source = "cs", .d3d11_target = "cs_5_0" },
    });
    const int before = d3d11_mock_live_object_count();
    sg_pipeline pip = sg_make_pipeline(&(sg_pipeline_desc){
        .shader = cshd, .compute = true,
    });
    T(sg_query_pipeline_state(pip) == SG_RESOURCESTATE_VALID);
    // Compute pipeline creates no IL/RS/DSS/BS.
    T(d3d11_mock_live_object_count() == before);
    sg_destroy_pipeline(pip);
    sg_destroy_shader(cshd);
    teardown();
}

//------------------------------------------------------------------------------
//  view creation (SRV / UAV / RTV / DSV paths)
//------------------------------------------------------------------------------
UTEST(sokol_gfx_d3d11, view_texture_creates_srv) {
    setup();
    uint8_t px[4 * 4 * 4] = {0};
    sg_image img = sg_make_image(&(sg_image_desc){
        .width = 4, .height = 4, .pixel_format = SG_PIXELFORMAT_RGBA8,
        .data.mip_levels[0] = SG_RANGE(px),
    });
    const int before = d3d11_mock_live_object_count();
    sg_view v = sg_make_view(&(sg_view_desc){ .texture.image = img });
    T(sg_query_view_state(v) == SG_RESOURCESTATE_VALID);
    T(d3d11_mock_live_object_count() == before + 1);   // SRV
    sg_destroy_view(v);
    sg_destroy_image(img);
    teardown();
}

UTEST(sokol_gfx_d3d11, view_color_attachment_creates_rtv) {
    setup();
    sg_image img = sg_make_image(&(sg_image_desc){
        .width = 128, .height = 128, .pixel_format = SG_PIXELFORMAT_RGBA8,
        .usage.color_attachment = true,
    });
    const int before = d3d11_mock_live_object_count();
    sg_view v = sg_make_view(&(sg_view_desc){ .color_attachment.image = img });
    T(sg_query_view_state(v) == SG_RESOURCESTATE_VALID);
    T(d3d11_mock_live_object_count() == before + 1);   // RTV
    sg_destroy_view(v);
    sg_destroy_image(img);
    teardown();
}

UTEST(sokol_gfx_d3d11, view_depth_stencil_attachment_creates_dsv) {
    setup();
    sg_image img = sg_make_image(&(sg_image_desc){
        .width = 128, .height = 128, .pixel_format = SG_PIXELFORMAT_DEPTH_STENCIL,
        .usage.depth_stencil_attachment = true,
    });
    const int before = d3d11_mock_live_object_count();
    sg_view v = sg_make_view(&(sg_view_desc){ .depth_stencil_attachment.image = img });
    T(sg_query_view_state(v) == SG_RESOURCESTATE_VALID);
    T(d3d11_mock_live_object_count() == before + 1);   // DSV
    sg_destroy_view(v);
    sg_destroy_image(img);
    teardown();
}

UTEST(sokol_gfx_d3d11, view_storage_image_creates_uav) {
    setup();
    sg_image img = sg_make_image(&(sg_image_desc){
        .width = 32, .height = 32, .pixel_format = SG_PIXELFORMAT_RGBA8,
        .usage.storage_image = true,
    });
    const int before = d3d11_mock_live_object_count();
    sg_view v = sg_make_view(&(sg_view_desc){ .storage_image.image = img });
    T(sg_query_view_state(v) == SG_RESOURCESTATE_VALID);
    T(d3d11_mock_live_object_count() == before + 1);   // UAV
    sg_destroy_view(v);
    sg_destroy_image(img);
    teardown();
}

UTEST(sokol_gfx_d3d11, view_srv_create_fail) {
    setup();
    uint8_t px[4 * 4 * 4] = {0};
    sg_image img = sg_make_image(&(sg_image_desc){
        .width = 4, .height = 4, .pixel_format = SG_PIXELFORMAT_RGBA8,
        .data.mip_levels[0] = SG_RANGE(px),
    });
    d3d11_mock_fail_next_create(1);
    sg_view v = sg_make_view(&(sg_view_desc){ .texture.image = img });
    T(sg_query_view_state(v) == SG_RESOURCESTATE_FAILED);
    T(logged(SG_LOGITEM_D3D11_CREATE_2D_SRV_FAILED) ||
      logged(SG_LOGITEM_D3D11_CREATE_3D_SRV_FAILED));
    sg_destroy_view(v);
    sg_destroy_image(img);
    teardown();
}

//------------------------------------------------------------------------------
//  passes -- swapchain, offscreen, MSAA resolve
//------------------------------------------------------------------------------
UTEST(sokol_gfx_d3d11, swapchain_pass_clear_and_commit) {
    setup();
    ID3D11RenderTargetView* rtv = d3d11_mock_create_rtv(mock_dev);
    ID3D11DepthStencilView* dsv = d3d11_mock_create_dsv(mock_dev);
    sg_begin_pass(&(sg_pass){
        .action = { .colors[0] = { .load_action = SG_LOADACTION_CLEAR, .clear_value = {0.1f, 0.2f, 0.3f, 1.0f} } },
        .swapchain = {
            .width = 640, .height = 480,
            .sample_count = 1,
            .color_format = SG_PIXELFORMAT_BGRA8,
            .depth_format = SG_PIXELFORMAT_DEPTH_STENCIL,
            .d3d11 = { .render_view = rtv, .depth_stencil_view = dsv },
        },
    });
    sg_apply_viewport(0, 0, 640, 480, false);
    sg_apply_scissor_rect(10, 10, 100, 100, false);
    sg_end_pass();
    sg_commit();
    rtv->lpVtbl->Release(rtv);
    dsv->lpVtbl->Release(dsv);
    T(sg_isvalid());
    teardown();
}

UTEST(sokol_gfx_d3d11, swapchain_msaa_resolve_pass) {
    setup();
    ID3D11RenderTargetView* rtv = d3d11_mock_create_rtv(mock_dev);
    ID3D11RenderTargetView* resolve = d3d11_mock_create_rtv(mock_dev);
    sg_begin_pass(&(sg_pass){
        .action = { .colors[0] = { .load_action = SG_LOADACTION_CLEAR, .store_action = SG_STOREACTION_STORE } },
        .swapchain = {
            .width = 128, .height = 128,
            .sample_count = 4,
            .color_format = SG_PIXELFORMAT_BGRA8,
            .depth_format = SG_PIXELFORMAT_NONE,
            .d3d11 = { .render_view = rtv, .resolve_view = resolve },
        },
    });
    sg_end_pass();   // triggers _sg_d3d11_ResolveSubresource
    sg_commit();
    rtv->lpVtbl->Release(rtv);
    resolve->lpVtbl->Release(resolve);
    T(sg_isvalid());
    teardown();
}

UTEST(sokol_gfx_d3d11, offscreen_pass_with_color_and_depth_views) {
    setup();
    sg_image color_img = sg_make_image(&(sg_image_desc){
        .width = 64, .height = 64, .pixel_format = SG_PIXELFORMAT_RGBA8,
        .usage.color_attachment = true,
    });
    sg_image depth_img = sg_make_image(&(sg_image_desc){
        .width = 64, .height = 64, .pixel_format = SG_PIXELFORMAT_DEPTH_STENCIL,
        .usage.depth_stencil_attachment = true,
    });
    sg_view color_v = sg_make_view(&(sg_view_desc){ .color_attachment.image = color_img });
    sg_view depth_v = sg_make_view(&(sg_view_desc){ .depth_stencil_attachment.image = depth_img });
    sg_begin_pass(&(sg_pass){
        .attachments = {
            .colors = { color_v },
            .depth_stencil = depth_v,
        },
        .action = {
            .colors[0] = { .load_action = SG_LOADACTION_CLEAR, .clear_value = {0, 0, 0, 1} },
            .depth = { .load_action = SG_LOADACTION_CLEAR, .clear_value = 1.0f },
            .stencil = { .load_action = SG_LOADACTION_CLEAR, .clear_value = 0 },
        },
    });
    sg_end_pass();
    sg_commit();
    sg_destroy_view(color_v);
    sg_destroy_view(depth_v);
    sg_destroy_image(color_img);
    sg_destroy_image(depth_img);
    T(sg_isvalid());
    teardown();
}

UTEST(sokol_gfx_d3d11, offscreen_msaa_pass_with_resolve) {
    setup();
    sg_image msaa_img = sg_make_image(&(sg_image_desc){
        .width = 64, .height = 64, .pixel_format = SG_PIXELFORMAT_RGBA8,
        .sample_count = 4, .usage.color_attachment = true,
    });
    sg_image resolve_img = sg_make_image(&(sg_image_desc){
        .width = 64, .height = 64, .pixel_format = SG_PIXELFORMAT_RGBA8,
        .usage.resolve_attachment = true,
    });
    sg_view color_v = sg_make_view(&(sg_view_desc){ .color_attachment.image = msaa_img });
    sg_view resolve_v = sg_make_view(&(sg_view_desc){ .resolve_attachment.image = resolve_img });
    sg_begin_pass(&(sg_pass){
        .attachments = { .colors = { color_v }, .resolves = { resolve_v } },
        .action = { .colors[0] = { .load_action = SG_LOADACTION_CLEAR, .store_action = SG_STOREACTION_STORE } },
    });
    sg_end_pass();  // exercises image-side ResolveSubresource
    sg_commit();
    sg_destroy_view(color_v);
    sg_destroy_view(resolve_v);
    sg_destroy_image(msaa_img);
    sg_destroy_image(resolve_img);
    T(sg_isvalid());
    teardown();
}

//------------------------------------------------------------------------------
//  draw / dispatch
//------------------------------------------------------------------------------
UTEST(sokol_gfx_d3d11, draw_indexed_and_instanced) {
    setup();
    ID3D11RenderTargetView* rtv = d3d11_mock_create_rtv(mock_dev);
    sg_shader shd = make_test_shader();
    sg_pipeline pip = sg_make_pipeline(&(sg_pipeline_desc){
        .shader = shd, .index_type = SG_INDEXTYPE_UINT16,
        .layout = { .attrs = { [0] = { .format = SG_VERTEXFORMAT_FLOAT3 }, [1] = { .format = SG_VERTEXFORMAT_UBYTE4N } } },
        .depth.pixel_format = SG_PIXELFORMAT_NONE,
    });
    static const float verts[32] = {0};
    static const uint16_t indices[8] = {0, 1, 2, 0, 2, 3, 0, 0};
    sg_buffer vbuf = sg_make_buffer(&(sg_buffer_desc){ .data = SG_RANGE(verts) });
    sg_buffer ibuf = sg_make_buffer(&(sg_buffer_desc){ .usage.index_buffer = true, .data = SG_RANGE(indices) });
    d3d11_mock_clear_calls();
    begin_swapchain_pass(rtv);
    sg_apply_pipeline(pip);
    sg_apply_bindings(&(sg_bindings){
        .vertex_buffers[0] = vbuf, .vertex_buffer_offsets[0] = 16,
        .index_buffer = ibuf, .index_buffer_offset = 4,
    });
    sg_draw(0, 6, 1);
    sg_draw(1, 3, 4);
    sg_draw_ex(2, 3, 2, 7, 5);
    sg_end_pass();
    sg_commit();
    T(no_errors());
    const d3d11_mock_call_t* c = LAST(IA_SET_VERTEX_BUFFERS);
    T(c && (c->dst == nbuf(vbuf)) && (c->args[0] == 0) && (c->args[1] == SG_MAX_VERTEXBUFFER_BINDSLOTS));
    T(c && (c->args[2] == 16) && (c->args[3] == 16));    // stride (float3+ubyte4n), offset
    c = LAST(IA_SET_INDEX_BUFFER);
    T(c && (c->dst == nbuf(ibuf)) && (c->args[0] == DXGI_FORMAT_R16_UINT) && (c->args[1] == 4));
    T(COUNT(DRAW_INDEXED) == 1);
    T(COUNT(DRAW_INDEXED_INSTANCED) == 2);
    T(COUNT(DRAW) == 0);
    c = LAST(DRAW_INDEXED);
    T(c && (c->args[0] == 6) && (c->args[1] == 0) && (c->args[2] == 0));
    c = NTH(DRAW_INDEXED_INSTANCED, 0);
    T(c && (c->args[0] == 3) && (c->args[1] == 4) && (c->args[2] == 1) && (c->args[3] == 0) && (c->args[4] == 0));
    c = NTH(DRAW_INDEXED_INSTANCED, 1);
    T(c && (c->args[0] == 3) && (c->args[1] == 2) && (c->args[2] == 2) && (c->args[3] == 7) && (c->args[4] == 5));
    sg_destroy_buffer(vbuf);
    sg_destroy_buffer(ibuf);
    sg_destroy_pipeline(pip);
    sg_destroy_shader(shd);
    rtv->lpVtbl->Release(rtv);
    teardown();
}

UTEST(sokol_gfx_d3d11, draw_non_indexed_and_instanced) {
    setup();
    ID3D11RenderTargetView* rtv = d3d11_mock_create_rtv(mock_dev);
    sg_shader shd = make_test_shader();
    sg_pipeline pip = sg_make_pipeline(&(sg_pipeline_desc){
        .shader = shd,
        .layout = { .attrs = { [0] = { .format = SG_VERTEXFORMAT_FLOAT3 }, [1] = { .format = SG_VERTEXFORMAT_UBYTE4N } } },
        .depth.pixel_format = SG_PIXELFORMAT_NONE,
    });
    static const float verts[24] = {0};
    sg_buffer vbuf = sg_make_buffer(&(sg_buffer_desc){ .data = SG_RANGE(verts) });
    d3d11_mock_clear_calls();
    begin_swapchain_pass(rtv);
    sg_apply_pipeline(pip);
    sg_apply_bindings(&(sg_bindings){ .vertex_buffers[0] = vbuf });
    sg_draw(0, 3, 1);
    sg_draw(2, 3, 8);
    sg_draw_ex(1, 3, 2, 0, 3);
    sg_end_pass();
    sg_commit();
    T(no_errors());
    // non-indexed draw: index buffer slot gets cleared
    const d3d11_mock_call_t* c = LAST(IA_SET_INDEX_BUFFER);
    T(c && (c->dst == NULL));
    T(COUNT(DRAW) == 1);
    T(COUNT(DRAW_INSTANCED) == 2);
    T(COUNT(DRAW_INDEXED) == 0);
    c = LAST(DRAW);
    T(c && (c->args[0] == 3) && (c->args[1] == 0));
    c = NTH(DRAW_INSTANCED, 0);
    T(c && (c->args[0] == 3) && (c->args[1] == 8) && (c->args[2] == 2) && (c->args[3] == 0));
    c = NTH(DRAW_INSTANCED, 1);
    T(c && (c->args[0] == 3) && (c->args[1] == 2) && (c->args[2] == 1) && (c->args[3] == 3));
    sg_destroy_buffer(vbuf);
    sg_destroy_pipeline(pip);
    sg_destroy_shader(shd);
    rtv->lpVtbl->Release(rtv);
    teardown();
}

UTEST(sokol_gfx_d3d11, apply_uniforms_updates_constant_buffer) {
    setup();
    ID3D11RenderTargetView* rtv = d3d11_mock_create_rtv(mock_dev);
    sg_shader shd = sg_make_shader(&(sg_shader_desc){
        .vertex_func = { .source = "vs", .d3d11_target = "vs_5_0" },
        .fragment_func = { .source = "ps", .d3d11_target = "ps_5_0" },
        .attrs = { [0] = { .hlsl_sem_name = "POSITION" } },
        .uniform_blocks = {
            [0] = { .stage = SG_SHADERSTAGE_VERTEX, .size = 64, .hlsl_register_b_n = 0 },
            [3] = { .stage = SG_SHADERSTAGE_FRAGMENT, .size = 16, .hlsl_register_b_n = 7 },
        },
    });
    // constant buffers: 64 and 16 bytes, CONSTANT_BUFFER bind flag, DEFAULT usage
    const d3d11_mock_call_t* c = NTH(CREATE_BUFFER, 0);
    T(c && (c->buffer_desc.ByteWidth == 64) && (c->buffer_desc.BindFlags == D3D11_BIND_CONSTANT_BUFFER));
    T(c && (c->buffer_desc.Usage == D3D11_USAGE_DEFAULT) && (c->buffer_desc.CPUAccessFlags == 0));
    c = NTH(CREATE_BUFFER, 1);
    T(c && (c->buffer_desc.ByteWidth == 16));
    const void* vs_cbuf = sg_d3d11_query_shader_info(shd).cbufs[0];
    const void* fs_cbuf = sg_d3d11_query_shader_info(shd).cbufs[3];
    T(vs_cbuf && fs_cbuf && (vs_cbuf != fs_cbuf));
    sg_pipeline pip = sg_make_pipeline(&(sg_pipeline_desc){
        .shader = shd,
        .layout = { .attrs = { [0] = { .format = SG_VERTEXFORMAT_FLOAT3 } } },
        .depth.pixel_format = SG_PIXELFORMAT_NONE,
    });
    static const float verts[9] = {0};
    sg_buffer vbuf = sg_make_buffer(&(sg_buffer_desc){ .data = SG_RANGE(verts) });
    d3d11_mock_clear_calls();
    begin_swapchain_pass(rtv);
    sg_apply_pipeline(pip);
    sg_apply_bindings(&(sg_bindings){ .vertex_buffers[0] = vbuf });
    float u0[16];
    for (int i = 0; i < 16; i++) { u0[i] = (float)i; }
    const float u3[4] = { 9.0f, 8.0f, 7.0f, 6.0f };
    sg_apply_uniforms(0, &SG_RANGE(u0));
    sg_apply_uniforms(3, &SG_RANGE(u3));
    sg_draw(0, 3, 1);
    sg_end_pass();
    sg_commit();
    T(no_errors());
    T(COUNT(UPDATE_SUBRESOURCE) == 2);
    c = NTH(UPDATE_SUBRESOURCE, 0);
    T(c && (c->dst == vs_cbuf) && !c->has_box && (c->dst_subres == 0));
    T(c && (c->data_size == 64) && (0 == memcmp(c->data, u0, 64)));
    c = NTH(UPDATE_SUBRESOURCE, 1);
    T(c && (c->dst == fs_cbuf) && !c->has_box);
    T(c && (c->data_size == 16) && (0 == memcmp(c->data, u3, 16)));
    sg_destroy_buffer(vbuf);
    sg_destroy_pipeline(pip);
    sg_destroy_shader(shd);
    rtv->lpVtbl->Release(rtv);
    teardown();
}

UTEST(sokol_gfx_d3d11, dispatch_compute) {
    setup();
    sg_shader shd = sg_make_shader(&(sg_shader_desc){
        .compute_func = { .source = "cs", .d3d11_target = "cs_5_0" },
    });
    sg_pipeline pip = sg_make_pipeline(&(sg_pipeline_desc){
        .shader = shd, .compute = true,
    });
    sg_begin_pass(&(sg_pass){ .compute = true });
    sg_apply_pipeline(pip);
    sg_dispatch(4, 4, 1);
    sg_end_pass();
    sg_commit();
    sg_destroy_pipeline(pip);
    sg_destroy_shader(shd);
    T(sg_isvalid());
    teardown();
}

//------------------------------------------------------------------------------
//  resource updates -- Map/Unmap vs UpdateSubresource
//------------------------------------------------------------------------------
UTEST(sokol_gfx_d3d11, write_transient_buffer) {
    setup();
    sg_buffer buf = sg_make_buffer(&(sg_buffer_desc){ .size = 128, .usage.write_transient = true });
    T(sg_query_buffer_state(buf) == SG_RESOURCESTATE_VALID);
    uint8_t bytes[32] = {0};
    sg_write_buffer_transient(&(sg_write_buffer_desc){
        .src.data = SG_RANGE(bytes),
        .dst.buffer = buf,
    });
    sg_destroy_buffer(buf);
    teardown();
}

UTEST(sokol_gfx_d3d11, write_unsealed_buffer_seals_it) {
    setup();
    sg_buffer buf = sg_make_buffer(&(sg_buffer_desc){
        .size = 64, .usage.write_unsealed = true,
    });
    T(sg_query_buffer_state(buf) == SG_RESOURCESTATE_UNSEALED);
    uint8_t bytes[64] = {0};
    sg_write_buffer_unsealed(&(sg_write_buffer_desc){
        .src.data = SG_RANGE(bytes),
        .dst.buffer = buf,
    });
    sg_seal_buffer(buf);
    T(sg_query_buffer_state(buf) == SG_RESOURCESTATE_VALID);
    sg_destroy_buffer(buf);
    teardown();
}

//------------------------------------------------------------------------------
//  storage view bindings + dispatch (write path through CSSetUnorderedAccessViews)
//------------------------------------------------------------------------------
UTEST(sokol_gfx_d3d11, apply_bindings_with_storage_views) {
    setup();
    sg_shader shd = sg_make_shader(&(sg_shader_desc){
        .compute_func = { .source = "cs", .d3d11_target = "cs_5_0" },
        .views = {
            [0] = { .storage_image = { .stage = SG_SHADERSTAGE_COMPUTE, .image_type = SG_IMAGETYPE_2D, .access_format = SG_PIXELFORMAT_RGBA8, .hlsl_register_u_n = 0 } },
        },
    });
    sg_pipeline pip = sg_make_pipeline(&(sg_pipeline_desc){ .shader = shd, .compute = true });
    sg_image img = sg_make_image(&(sg_image_desc){
        .width = 16, .height = 16, .pixel_format = SG_PIXELFORMAT_RGBA8,
        .usage.storage_image = true,
    });
    sg_view uav = sg_make_view(&(sg_view_desc){ .storage_image.image = img });
    sg_begin_pass(&(sg_pass){ .compute = true });
    sg_apply_pipeline(pip);
    sg_apply_bindings(&(sg_bindings){ .views[0] = uav });
    sg_dispatch(2, 2, 1);
    sg_end_pass();
    sg_commit();
    sg_destroy_view(uav);
    sg_destroy_image(img);
    sg_destroy_pipeline(pip);
    sg_destroy_shader(shd);
    T(sg_isvalid());
    teardown();
}

//------------------------------------------------------------------------------
//  leak-check across a full lifecycle
//------------------------------------------------------------------------------
UTEST(sokol_gfx_d3d11, no_leaks_full_lifecycle) {
    setup();
    sg_shader shd = make_test_shader();
    sg_pipeline pip = sg_make_pipeline(&(sg_pipeline_desc){
        .shader = shd,
        .layout = { .attrs = { [0] = { .format = SG_VERTEXFORMAT_FLOAT3 }, [1] = { .format = SG_VERTEXFORMAT_UBYTE4N } } },
    });
    sg_buffer buf = sg_make_buffer(&(sg_buffer_desc){ .size = 64, .usage.write_transient = true });
    sg_image img = sg_make_image(&(sg_image_desc){
        .width = 8, .height = 8, .pixel_format = SG_PIXELFORMAT_RGBA8,
        .usage.color_attachment = true,
    });
    sg_view v = sg_make_view(&(sg_view_desc){ .color_attachment.image = img });
    sg_destroy_view(v);
    sg_destroy_image(img);
    sg_destroy_buffer(buf);
    sg_destroy_pipeline(pip);
    sg_destroy_shader(shd);
    teardown();
    // Only per-test transient allocations should remain -- reset drains them.
    T(d3d11_mock_live_object_count() == 0);
}

//------------------------------------------------------------------------------
//  shared test data
//------------------------------------------------------------------------------
static uint8_t pattern[4096];
static void fill_pattern(void) {
    for (size_t i = 0; i < sizeof(pattern); i++) {
        pattern[i] = (uint8_t)((i * 7u) ^ (i >> 8));
    }
}

//------------------------------------------------------------------------------
//  buffer usage matrix -> D3D11_BUFFER_DESC
//------------------------------------------------------------------------------
static bool check_buffer_desc(const sg_buffer_desc* desc, UINT byte_width, D3D11_USAGE usage, UINT bind, UINT cpu, UINT misc) {
    reset_log();
    d3d11_mock_clear_calls();
    sg_buffer buf = sg_make_buffer(desc);
    const sg_resource_state state = sg_query_buffer_state(buf);
    bool ok = ((state == SG_RESOURCESTATE_VALID) || (state == SG_RESOURCESTATE_UNSEALED)) && no_errors();
    ok = ok && (d3d11_mock_count_calls(D3D11_MOCK_CALL_CREATE_BUFFER) == 1);
    const d3d11_mock_call_t* c = LAST(CREATE_BUFFER);
    ok = ok && c && (c->result == S_OK) && (c->dst == nbuf(buf));
    ok = ok && c && (c->buffer_desc.ByteWidth == byte_width);
    ok = ok && c && (c->buffer_desc.Usage == usage);
    ok = ok && c && (c->buffer_desc.BindFlags == bind);
    ok = ok && c && (c->buffer_desc.CPUAccessFlags == cpu);
    ok = ok && c && (c->buffer_desc.MiscFlags == misc);
    ok = ok && c && (c->buffer_desc.StructureByteStride == 0);
    ok = ok && c && (c->has_init_data == (desc->data.ptr != 0));
    if (ok && desc->data.ptr) {
        ok = (c->src_data == desc->data.ptr) && (0 == memcmp(c->data, desc->data.ptr, c->data_size));
    }
    sg_destroy_buffer(buf);
    return ok;
}

UTEST(sokol_gfx_d3d11, buffer_usage_matrix) {
    setup();
    fill_pattern();
    const UINT VB = D3D11_BIND_VERTEX_BUFFER;
    const UINT IB = D3D11_BIND_INDEX_BUFFER;
    const UINT SRV = D3D11_BIND_SHADER_RESOURCE;
    const UINT UAV = D3D11_BIND_UNORDERED_ACCESS;
    const UINT RAW = D3D11_RESOURCE_MISC_BUFFER_ALLOW_RAW_VIEWS;
    const UINT W = D3D11_CPU_ACCESS_WRITE;
    const sg_range data = { pattern, 64 };
    // immutable (default) vertex/index buffers with init data
    T(check_buffer_desc(&(sg_buffer_desc){ .data = data }, 64, D3D11_USAGE_DEFAULT, VB, 0, 0));
    T(check_buffer_desc(&(sg_buffer_desc){ .usage.index_buffer = true, .data = data }, 64, D3D11_USAGE_DEFAULT, IB, 0, 0));
    T(check_buffer_desc(&(sg_buffer_desc){ .usage = { .vertex_buffer = true, .index_buffer = true }, .data = data }, 64, D3D11_USAGE_DEFAULT, VB|IB, 0, 0));
    T(check_buffer_desc(&(sg_buffer_desc){ .usage = { .vertex_buffer = true, .copy_src = true }, .data = data }, 64, D3D11_USAGE_DEFAULT, VB, 0, 0));
    // storage buffers: SRV+UAV and raw views, write_transient storage buffers can't have an UAV
    T(check_buffer_desc(&(sg_buffer_desc){ .usage.storage_buffer = true, .size = 256 }, 256, D3D11_USAGE_DEFAULT, SRV|UAV, 0, RAW));
    T(check_buffer_desc(&(sg_buffer_desc){ .usage.storage_buffer = true, .data = data }, 64, D3D11_USAGE_DEFAULT, SRV|UAV, 0, RAW));
    T(check_buffer_desc(&(sg_buffer_desc){ .usage = { .storage_buffer = true, .write_transient = true }, .size = 256 }, 256, D3D11_USAGE_DYNAMIC, SRV, W, RAW));
    T(check_buffer_desc(&(sg_buffer_desc){ .usage = { .storage_buffer = true, .write_unsealed = true }, .size = 256 }, 256, D3D11_USAGE_DEFAULT, SRV|UAV, 0, RAW));
    T(check_buffer_desc(&(sg_buffer_desc){ .usage = { .storage_buffer = true, .copy_src = true, .copy_dst = true }, .size = 256 }, 256, D3D11_USAGE_DEFAULT, SRV|UAV, 0, RAW));
    // write_transient => DYNAMIC + CPU write access
    T(check_buffer_desc(&(sg_buffer_desc){ .usage.write_transient = true, .size = 128 }, 128, D3D11_USAGE_DYNAMIC, VB, W, 0));
    T(check_buffer_desc(&(sg_buffer_desc){ .usage = { .index_buffer = true, .write_transient = true }, .size = 128 }, 128, D3D11_USAGE_DYNAMIC, IB, W, 0));
    T(check_buffer_desc(&(sg_buffer_desc){ .usage = { .write_transient = true, .copy_src = true }, .size = 128 }, 128, D3D11_USAGE_DYNAMIC, VB, W, 0));
    // write_unsealed and copy_dst => DEFAULT, no CPU access (UpdateSubresource / CopySubresourceRegion)
    T(check_buffer_desc(&(sg_buffer_desc){ .usage.write_unsealed = true, .size = 64 }, 64, D3D11_USAGE_DEFAULT, VB, 0, 0));
    T(check_buffer_desc(&(sg_buffer_desc){ .usage = { .index_buffer = true, .write_unsealed = true }, .size = 64 }, 64, D3D11_USAGE_DEFAULT, IB, 0, 0));
    T(check_buffer_desc(&(sg_buffer_desc){ .usage.copy_dst = true, .size = 64 }, 64, D3D11_USAGE_DEFAULT, VB, 0, 0));
    T(check_buffer_desc(&(sg_buffer_desc){ .usage = { .index_buffer = true, .copy_dst = true }, .size = 64 }, 64, D3D11_USAGE_DEFAULT, IB, 0, 0));
    teardown();
}

UTEST(sokol_gfx_d3d11, buffer_staging_has_no_native_buffer) {
    setup();
    fill_pattern();
    const int before = d3d11_mock_live_object_count();
    d3d11_mock_clear_calls();
    sg_buffer s0 = sg_make_buffer(&(sg_buffer_desc){
        .size = 256, .usage = { .staging_buffer = true, .copy_src = true, .write_transient = true },
    });
    sg_buffer s1 = sg_make_buffer(&(sg_buffer_desc){
        .size = 256, .usage = { .staging_index_buffer = true, .copy_src = true, .write_transient = true },
    });
    T(sg_query_buffer_state(s0) == SG_RESOURCESTATE_VALID);
    T(sg_query_buffer_state(s1) == SG_RESOURCESTATE_VALID);
    T(no_errors());
    // staging buffers live in CPU memory: no CreateBuffer, no native object
    T(COUNT(CREATE_BUFFER) == 0);
    T(d3d11_mock_live_object_count() == before);
    T(nbuf(s0) == NULL);
    T(nbuf(s1) == NULL);
    // ...and writing to them is a plain memcpy, no Map/Unmap/UpdateSubresource
    sg_write_buffer_transient(&(sg_write_buffer_desc){ .src.data = { pattern, 64 }, .dst.buffer = s0 });
    sg_write_buffer_transient(&(sg_write_buffer_desc){ .src.data = { pattern, 64 }, .dst = { .buffer = s1, .offset = 128 } });
    T(no_errors());
    T(d3d11_mock_num_calls() == 0);
    sg_destroy_buffer(s0);
    sg_destroy_buffer(s1);
    T(d3d11_mock_live_object_count() == before);
    teardown();
}

//------------------------------------------------------------------------------
//  sg_write_buffer_transient => Map(DISCARD / NO_OVERWRITE) + memcpy + Unmap
//------------------------------------------------------------------------------
UTEST(sokol_gfx_d3d11, write_buffer_transient_map_discard_then_nooverwrite) {
    setup();
    fill_pattern();
    sg_buffer buf = sg_make_buffer(&(sg_buffer_desc){ .size = 128, .usage.write_transient = true });
    const void* nb = nbuf(buf);
    d3d11_mock_clear_calls();
    // first write in frame => WRITE_DISCARD
    sg_write_buffer_transient(&(sg_write_buffer_desc){ .src.data = { pattern, 64 }, .dst.buffer = buf });
    T(no_errors());
    T(d3d11_mock_num_calls() == 2);
    const d3d11_mock_call_t* c = d3d11_mock_get_call(0);
    T(c && (c->kind == D3D11_MOCK_CALL_MAP) && (c->dst == nb) && (c->dst_subres == 0) && (c->map_type == D3D11_MAP_WRITE_DISCARD) && (c->result == S_OK));
    c = d3d11_mock_get_call(1);
    T(c && (c->kind == D3D11_MOCK_CALL_UNMAP) && (c->dst == nb));
    size_t mapped_size = 0;
    const uint8_t* mapped = (const uint8_t*)d3d11_mock_mapped_data(nb, &mapped_size);
    T(mapped && (mapped_size >= 128));
    T(mapped && (0 == memcmp(mapped, pattern, 64)));
    // second write in same frame => WRITE_NO_OVERWRITE, honours src/dst offsets
    sg_write_buffer_transient(&(sg_write_buffer_desc){
        .src = { .data = { pattern, 128 }, .offset = 8 },
        .dst = { .buffer = buf, .offset = 64 },
        .size = 16,
    });
    T(no_errors());
    T(COUNT(MAP) == 2);
    T(COUNT(UNMAP) == 2);
    c = NTH(MAP, 1);
    T(c && (c->dst == nb) && (c->map_type == D3D11_MAP_WRITE_NO_OVERWRITE));
    mapped = (const uint8_t*)d3d11_mock_mapped_data(nb, &mapped_size);
    T(mapped && (0 == memcmp(mapped + 64, pattern + 8, 16)));
    T(COUNT(UPDATE_SUBRESOURCE) == 0);
    // new frame => WRITE_DISCARD again
    sg_commit();
    d3d11_mock_clear_calls();
    sg_write_buffer_transient(&(sg_write_buffer_desc){ .src.data = { pattern + 100, 32 }, .dst.buffer = buf });
    T(no_errors());
    c = LAST(MAP);
    T(c && (c->map_type == D3D11_MAP_WRITE_DISCARD));
    mapped = (const uint8_t*)d3d11_mock_mapped_data(nb, &mapped_size);
    T(mapped && (0 == memcmp(mapped, pattern + 100, 32)));
    sg_destroy_buffer(buf);
    teardown();
}

UTEST(sokol_gfx_d3d11, write_buffer_transient_storage_buffer) {
    setup();
    fill_pattern();
    sg_buffer buf = sg_make_buffer(&(sg_buffer_desc){ .size = 256, .usage = { .storage_buffer = true, .write_transient = true } });
    d3d11_mock_clear_calls();
    sg_write_buffer_transient(&(sg_write_buffer_desc){ .src.data = { pattern, 256 }, .dst.buffer = buf });
    T(no_errors());
    const d3d11_mock_call_t* c = LAST(MAP);
    T(c && (c->dst == nbuf(buf)) && (c->map_type == D3D11_MAP_WRITE_DISCARD));
    size_t size = 0;
    const void* mapped = d3d11_mock_mapped_data(nbuf(buf), &size);
    T(mapped && (size >= 256) && (0 == memcmp(mapped, pattern, 256)));
    sg_destroy_buffer(buf);
    teardown();
}

UTEST(sokol_gfx_d3d11, write_buffer_transient_map_fail) {
    setup();
    fill_pattern();
    sg_buffer buf = sg_make_buffer(&(sg_buffer_desc){ .size = 128, .usage.write_transient = true });
    d3d11_mock_clear_calls();
    d3d11_mock_fail_next_map(1);
    sg_write_buffer_transient(&(sg_write_buffer_desc){ .src.data = { pattern, 64 }, .dst.buffer = buf });
    T(logged(SG_LOGITEM_D3D11_MAP_FOR_WRITE_BUFFER_TRANSIENT_FAILED));
    T(COUNT(MAP) == 1);
    T(LAST(MAP) && (LAST(MAP)->result != S_OK));
    T(COUNT(UNMAP) == 0);   // a failed Map must not be followed by Unmap
    sg_destroy_buffer(buf);
    teardown();
}

UTEST(sokol_gfx_d3d11, write_buffer_transient_validation_rejects) {
    setup();
    fill_pattern();
    sg_buffer imm = sg_make_buffer(&(sg_buffer_desc){ .data = { pattern, 64 } });
    sg_buffer tra = sg_make_buffer(&(sg_buffer_desc){ .size = 128, .usage.write_transient = true });
    d3d11_mock_clear_calls();
    // not a write_transient buffer
    sg_write_buffer_transient(&(sg_write_buffer_desc){ .src.data = { pattern, 16 }, .dst.buffer = imm });
    T(logged(SG_LOGITEM_VALIDATION_FAILED));
    // misaligned destination offset
    reset_log();
    sg_write_buffer_transient(&(sg_write_buffer_desc){ .src.data = { pattern, 16 }, .dst = { .buffer = tra, .offset = 2 } });
    T(logged(SG_LOGITEM_VALIDATION_FAILED));
    // overflow
    reset_log();
    sg_write_buffer_transient(&(sg_write_buffer_desc){ .src.data = { pattern, 64 }, .dst = { .buffer = tra, .offset = 96 } });
    T(logged(SG_LOGITEM_VALIDATION_FAILED));
    T(d3d11_mock_num_calls() == 0);
    sg_destroy_buffer(imm);
    sg_destroy_buffer(tra);
    teardown();
}

//------------------------------------------------------------------------------
//  sg_write_buffer_unsealed => UpdateSubresource with a 1D dst box, then seal
//------------------------------------------------------------------------------
UTEST(sokol_gfx_d3d11, write_buffer_unsealed_update_subresource_and_seal) {
    setup();
    fill_pattern();
    sg_buffer buf = sg_make_buffer(&(sg_buffer_desc){ .size = 64, .usage = { .storage_buffer = true, .write_unsealed = true } });
    T(sg_query_buffer_state(buf) == SG_RESOURCESTATE_UNSEALED);
    const void* nb = nbuf(buf);
    d3d11_mock_clear_calls();
    sg_write_buffer_unsealed(&(sg_write_buffer_desc){
        .src = { .data = { pattern, 32 }, .offset = 4 },
        .dst = { .buffer = buf, .offset = 16 },
        .size = 20,
    });
    sg_write_buffer_unsealed(&(sg_write_buffer_desc){ .src.data = { pattern + 200, 16 }, .dst.buffer = buf });
    T(no_errors());
    T(COUNT(UPDATE_SUBRESOURCE) == 2);
    T(COUNT(MAP) == 0);
    const d3d11_mock_call_t* c = NTH(UPDATE_SUBRESOURCE, 0);
    T(c && (c->dst == nb) && (c->dst_subres == 0) && c->has_box);
    T(c && (c->box.left == 16) && (c->box.right == 36) && (c->box.top == 0) && (c->box.bottom == 1) && (c->box.front == 0) && (c->box.back == 1));
    T(c && (c->src_data == pattern + 4) && (c->src_row_pitch == 0) && (c->src_depth_pitch == 0));
    T(c && (c->data_size == 20) && (0 == memcmp(c->data, pattern + 4, 20)));
    c = NTH(UPDATE_SUBRESOURCE, 1);
    T(c && (c->box.left == 0) && (c->box.right == 16) && (c->src_data == pattern + 200));
    // sealing is a no-op on D3D11
    d3d11_mock_clear_calls();
    sg_seal_buffer(buf);
    T(sg_query_buffer_state(buf) == SG_RESOURCESTATE_VALID);
    T(d3d11_mock_num_calls() == 0);
    // writing after sealing is rejected by validation
    sg_write_buffer_unsealed(&(sg_write_buffer_desc){ .src.data = { pattern, 16 }, .dst.buffer = buf });
    T(logged(SG_LOGITEM_VALIDATION_FAILED));
    T(d3d11_mock_num_calls() == 0);
    sg_destroy_buffer(buf);
    teardown();
}

//------------------------------------------------------------------------------
//  sg_copy_buffer_to_buffer
//------------------------------------------------------------------------------
UTEST(sokol_gfx_d3d11, copy_buffer_to_buffer_gpu_uses_copy_subresource_region) {
    setup();
    fill_pattern();
    sg_buffer src = sg_make_buffer(&(sg_buffer_desc){ .usage = { .vertex_buffer = true, .copy_src = true }, .data = { pattern, 256 } });
    sg_buffer dst = sg_make_buffer(&(sg_buffer_desc){ .usage = { .storage_buffer = true, .copy_dst = true }, .size = 256 });
    d3d11_mock_clear_calls();
    sg_copy_buffer_to_buffer(&(sg_copy_buffer_to_buffer_desc){
        .src = { .buffer = src, .offset = 64 },
        .dst = { .buffer = dst, .offset = 128 },
        .size = 32,
    });
    T(no_errors());
    T(d3d11_mock_num_calls() == 1);
    const d3d11_mock_call_t* c = LAST(COPY_SUBRESOURCE_REGION);
    T(c && (c->dst == nbuf(dst)) && (c->dst_subres == 0));
    T(c && (c->dst_x == 128) && (c->dst_y == 0) && (c->dst_z == 0));
    T(c && (c->src == nbuf(src)) && (c->src_subres == 0) && c->has_box);
    T(c && (c->box.left == 64) && (c->box.right == 96) && (c->box.top == 0) && (c->box.bottom == 1) && (c->box.front == 0) && (c->box.back == 1));
    sg_destroy_buffer(src);
    sg_destroy_buffer(dst);
    teardown();
}

UTEST(sokol_gfx_d3d11, copy_buffer_to_buffer_from_staging_uses_update_subresource) {
    setup();
    fill_pattern();
    sg_buffer stg = sg_make_buffer(&(sg_buffer_desc){
        .size = 256, .usage = { .staging_buffer = true, .copy_src = true, .write_transient = true },
    });
    sg_buffer dst = sg_make_buffer(&(sg_buffer_desc){ .usage.copy_dst = true, .size = 128 });
    d3d11_mock_clear_calls();
    sg_write_buffer_transient(&(sg_write_buffer_desc){ .src.data = { pattern, 256 }, .dst.buffer = stg });
    sg_copy_buffer_to_buffer(&(sg_copy_buffer_to_buffer_desc){
        .src = { .buffer = stg, .offset = 32 },
        .dst = { .buffer = dst, .offset = 16 },
        .size = 48,
    });
    T(no_errors());
    // no Map for the staging write, no GPU copy: a single UpdateSubresource
    T(d3d11_mock_num_calls() == 1);
    const d3d11_mock_call_t* c = LAST(UPDATE_SUBRESOURCE);
    T(c && (c->dst == nbuf(dst)) && (c->dst_subres == 0) && c->has_box);
    T(c && (c->box.left == 16) && (c->box.right == 64) && (c->box.top == 0) && (c->box.bottom == 1) && (c->box.front == 0) && (c->box.back == 1));
    T(c && (c->src_row_pitch == 0) && (c->src_depth_pitch == 0));
    T(c && (c->data_size == 48) && (0 == memcmp(c->data, pattern + 32, 48)));
    // writing the staging buffer again in the same frame after it was a copy source is invalid
    sg_write_buffer_transient(&(sg_write_buffer_desc){ .src.data = { pattern, 16 }, .dst.buffer = stg });
    T(logged(SG_LOGITEM_VALIDATION_FAILED));
    sg_destroy_buffer(stg);
    sg_destroy_buffer(dst);
    teardown();
}

UTEST(sokol_gfx_d3d11, copy_buffer_to_buffer_from_staging_index_buffer) {
    setup();
    fill_pattern();
    sg_buffer stg = sg_make_buffer(&(sg_buffer_desc){
        .size = 64, .usage = { .staging_index_buffer = true, .copy_src = true, .write_transient = true },
    });
    sg_buffer dst = sg_make_buffer(&(sg_buffer_desc){ .usage = { .index_buffer = true, .copy_dst = true }, .size = 64 });
    sg_write_buffer_transient(&(sg_write_buffer_desc){ .src.data = { pattern + 64, 64 }, .dst.buffer = stg });
    d3d11_mock_clear_calls();
    sg_copy_buffer_to_buffer(&(sg_copy_buffer_to_buffer_desc){ .src.buffer = stg, .dst.buffer = dst, .size = 64 });
    T(no_errors());
    const d3d11_mock_call_t* c = LAST(UPDATE_SUBRESOURCE);
    T(c && (c->dst == nbuf(dst)) && (c->box.left == 0) && (c->box.right == 64));
    T(c && (0 == memcmp(c->data, pattern + 64, c->data_size)));
    T(COUNT(COPY_SUBRESOURCE_REGION) == 0);
    sg_destroy_buffer(stg);
    sg_destroy_buffer(dst);
    teardown();
}

UTEST(sokol_gfx_d3d11, copy_buffer_to_buffer_validation_rejects) {
    setup();
    fill_pattern();
    sg_buffer src = sg_make_buffer(&(sg_buffer_desc){ .usage = { .vertex_buffer = true, .copy_src = true }, .data = { pattern, 64 } });
    sg_buffer nosrc = sg_make_buffer(&(sg_buffer_desc){ .data = { pattern, 64 } });
    sg_buffer dst = sg_make_buffer(&(sg_buffer_desc){ .usage.copy_dst = true, .size = 64 });
    d3d11_mock_clear_calls();
    // source not copy_src
    sg_copy_buffer_to_buffer(&(sg_copy_buffer_to_buffer_desc){ .src.buffer = nosrc, .dst.buffer = dst, .size = 16 });
    T(logged(SG_LOGITEM_VALIDATION_FAILED));
    // misaligned offset
    reset_log();
    sg_copy_buffer_to_buffer(&(sg_copy_buffer_to_buffer_desc){ .src = { .buffer = src, .offset = 2 }, .dst.buffer = dst, .size = 16 });
    T(logged(SG_LOGITEM_VALIDATION_FAILED));
    // overflow
    reset_log();
    sg_copy_buffer_to_buffer(&(sg_copy_buffer_to_buffer_desc){ .src.buffer = src, .dst = { .buffer = dst, .offset = 32 }, .size = 64 });
    T(logged(SG_LOGITEM_VALIDATION_FAILED));
    // inside a pass
    reset_log();
    sg_begin_pass(&(sg_pass){ .compute = true });
    sg_copy_buffer_to_buffer(&(sg_copy_buffer_to_buffer_desc){ .src.buffer = src, .dst.buffer = dst, .size = 16 });
    sg_end_pass();
    T(logged(SG_LOGITEM_VALIDATION_FAILED));
    T(COUNT(COPY_SUBRESOURCE_REGION) == 0);
    T(COUNT(UPDATE_SUBRESOURCE) == 0);
    sg_destroy_buffer(src);
    sg_destroy_buffer(nosrc);
    sg_destroy_buffer(dst);
    teardown();
}

//------------------------------------------------------------------------------
//  sg_copy_buffer_to_image (staging buffer source => UpdateSubresource)
//------------------------------------------------------------------------------
static sg_buffer make_filled_staging_buffer(size_t size) {
    sg_buffer stg = sg_make_buffer(&(sg_buffer_desc){
        .size = size, .usage = { .staging_buffer = true, .copy_src = true, .write_transient = true },
    });
    sg_write_buffer_transient(&(sg_write_buffer_desc){ .src.data = { pattern, size }, .dst.buffer = stg });
    return stg;
}

UTEST(sokol_gfx_d3d11, copy_buffer_to_image_2d_defaults) {
    setup();
    fill_pattern();
    sg_buffer stg = make_filled_staging_buffer(2048);
    sg_image img = sg_make_image(&(sg_image_desc){ .width = 16, .height = 16, .pixel_format = SG_PIXELFORMAT_RGBA8, .usage.copy_dst = true });
    T(sg_query_image_state(img) == SG_RESOURCESTATE_VALID);
    d3d11_mock_clear_calls();
    sg_copy_buffer_to_image(&(sg_copy_buffer_to_image_desc){ .src = { .buffer = stg, .offset = 512 }, .dst.image = img });
    T(no_errors());
    T(d3d11_mock_num_calls() == 1);
    const d3d11_mock_call_t* c = LAST(UPDATE_SUBRESOURCE);
    T(c && (c->dst == ntex(img)) && (c->dst_subres == 0) && c->has_box);
    T(c && (c->box.left == 0) && (c->box.right == 16) && (c->box.top == 0) && (c->box.bottom == 16) && (c->box.front == 0) && (c->box.back == 1));
    T(c && (c->src_row_pitch == 64) && (c->src_depth_pitch == 0));
    T(c && (c->data_size > 0) && (0 == memcmp(c->data, pattern + 512, c->data_size)));
    sg_destroy_buffer(stg);
    sg_destroy_image(img);
    teardown();
}

UTEST(sokol_gfx_d3d11, copy_buffer_to_image_2d_subrect) {
    setup();
    fill_pattern();
    sg_buffer stg = make_filled_staging_buffer(2048);
    sg_image img = sg_make_image(&(sg_image_desc){ .width = 16, .height = 16, .pixel_format = SG_PIXELFORMAT_RGBA8, .usage.copy_dst = true });
    d3d11_mock_clear_calls();
    sg_copy_buffer_to_image(&(sg_copy_buffer_to_image_desc){
        .src = { .buffer = stg, .offset = 256, .bytes_per_row = 128 },
        .dst = { .image = img, .x = 4, .y = 2 },
        .size = { .width = 8, .height = 4 },
    });
    T(no_errors());
    const d3d11_mock_call_t* c = LAST(UPDATE_SUBRESOURCE);
    T(c && (c->dst == ntex(img)) && (c->dst_subres == 0));
    T(c && (c->box.left == 4) && (c->box.right == 12) && (c->box.top == 2) && (c->box.bottom == 6) && (c->box.front == 0) && (c->box.back == 1));
    T(c && (c->src_row_pitch == 128) && (c->src_depth_pitch == 0));
    T(c && (0 == memcmp(c->data, pattern + 256, c->data_size)));
    sg_destroy_buffer(stg);
    sg_destroy_image(img);
    teardown();
}

UTEST(sokol_gfx_d3d11, copy_buffer_to_image_array_mip_slices) {
    setup();
    fill_pattern();
    sg_buffer stg = make_filled_staging_buffer(1024);
    sg_image img = sg_make_image(&(sg_image_desc){
        .type = SG_IMAGETYPE_ARRAY, .width = 8, .height = 8, .num_slices = 3, .num_mipmaps = 2,
        .pixel_format = SG_PIXELFORMAT_RGBA8, .usage.copy_dst = true,
    });
    d3d11_mock_clear_calls();
    // mip 1 is 4x4: row pitch 16, slice pitch 64
    sg_copy_buffer_to_image(&(sg_copy_buffer_to_image_desc){
        .src = { .buffer = stg, .offset = 128 },
        .dst = { .image = img, .mip_level = 1, .slice = 1 },
        .size = { .num_slices = 2 },
    });
    T(no_errors());
    T(COUNT(UPDATE_SUBRESOURCE) == 2);
    const d3d11_mock_call_t* c0 = NTH(UPDATE_SUBRESOURCE, 0);
    const d3d11_mock_call_t* c1 = NTH(UPDATE_SUBRESOURCE, 1);
    // subresource = mip + slice * num_mips
    T(c0 && (c0->dst == ntex(img)) && (c0->dst_subres == 3));
    T(c1 && (c1->dst == ntex(img)) && (c1->dst_subres == 5));
    T(c0 && (c0->box.left == 0) && (c0->box.right == 4) && (c0->box.top == 0) && (c0->box.bottom == 4) && (c0->box.front == 0) && (c0->box.back == 1));
    T(c0 && (c0->src_row_pitch == 16) && (c0->src_depth_pitch == 0));
    T(c0 && c1 && (((const uint8_t*)c1->src_data - (const uint8_t*)c0->src_data) == 64));
    T(c0 && (0 == memcmp(c0->data, pattern + 128, c0->data_size)));
    T(c1 && (0 == memcmp(c1->data, pattern + 192, c1->data_size)));
    sg_destroy_buffer(stg);
    sg_destroy_image(img);
    teardown();
}

UTEST(sokol_gfx_d3d11, copy_buffer_to_image_3d) {
    setup();
    fill_pattern();
    sg_buffer stg = make_filled_staging_buffer(1024);
    sg_image img = sg_make_image(&(sg_image_desc){
        .type = SG_IMAGETYPE_3D, .width = 8, .height = 8, .num_slices = 4,
        .pixel_format = SG_PIXELFORMAT_RGBA8, .usage.copy_dst = true,
    });
    T(sg_query_image_state(img) == SG_RESOURCESTATE_VALID);
    d3d11_mock_clear_calls();
    sg_copy_buffer_to_image(&(sg_copy_buffer_to_image_desc){
        .src = { .buffer = stg, .offset = 0 },
        .dst = { .image = img, .slice = 1 },
        .size = { .num_slices = 2 },
    });
    T(no_errors());
    // 3D textures: a single UpdateSubresource with front/back covering the depth slices
    T(COUNT(UPDATE_SUBRESOURCE) == 1);
    const d3d11_mock_call_t* c = LAST(UPDATE_SUBRESOURCE);
    T(c && (c->dst == ntex(img)) && (c->dst_subres == 0));
    T(c && (c->box.left == 0) && (c->box.right == 8) && (c->box.top == 0) && (c->box.bottom == 8) && (c->box.front == 1) && (c->box.back == 3));
    T(c && (c->src_row_pitch == 32) && (c->src_depth_pitch == 256));
    T(c && (0 == memcmp(c->data, pattern, c->data_size)));
    sg_destroy_buffer(stg);
    sg_destroy_image(img);
    teardown();
}

UTEST(sokol_gfx_d3d11, copy_buffer_to_image_validation_rejects) {
    setup();
    fill_pattern();
    sg_buffer stg = make_filled_staging_buffer(256);
    sg_buffer gpu = sg_make_buffer(&(sg_buffer_desc){ .usage = { .vertex_buffer = true, .copy_src = true }, .data = { pattern, 1024 } });
    sg_image img = sg_make_image(&(sg_image_desc){ .width = 16, .height = 16, .pixel_format = SG_PIXELFORMAT_RGBA8, .usage.copy_dst = true });
    d3d11_mock_clear_calls();
    // source must be a staging buffer
    sg_copy_buffer_to_image(&(sg_copy_buffer_to_image_desc){ .src.buffer = gpu, .dst.image = img });
    T(logged(SG_LOGITEM_VALIDATION_FAILED));
    // source too small for a full 16x16 RGBA8 surface
    reset_log();
    sg_copy_buffer_to_image(&(sg_copy_buffer_to_image_desc){ .src.buffer = stg, .dst.image = img });
    T(logged(SG_LOGITEM_VALIDATION_FAILED));
    T(d3d11_mock_num_calls() == 0);
    sg_destroy_buffer(stg);
    sg_destroy_buffer(gpu);
    sg_destroy_image(img);
    teardown();
}

//------------------------------------------------------------------------------
//  image creation => D3D11_TEXTURE2D_DESC / D3D11_TEXTURE3D_DESC
//------------------------------------------------------------------------------
static const d3d11_mock_call_t* make_image_get_create(const sg_image_desc* desc, sg_image* out_img) {
    reset_log();
    d3d11_mock_clear_calls();
    *out_img = sg_make_image(desc);
    return (desc->type == SG_IMAGETYPE_3D) ? LAST(CREATE_TEXTURE3D) : LAST(CREATE_TEXTURE2D);
}

UTEST(sokol_gfx_d3d11, image_tex2d_desc_variants) {
    setup();
    fill_pattern();
    const UINT SRV = D3D11_BIND_SHADER_RESOURCE;
    sg_image img;
    // copy_dst 2D
    const d3d11_mock_call_t* c = make_image_get_create(&(sg_image_desc){
        .width = 32, .height = 16, .num_mipmaps = 3, .pixel_format = SG_PIXELFORMAT_RGBA8, .usage.copy_dst = true,
    }, &img);
    T(no_errors() && (sg_query_image_state(img) == SG_RESOURCESTATE_VALID));
    T(c && (c->dst == ntex(img)) && (c->dst == sg_d3d11_query_image_info(img).tex2d) && !c->has_init_data);
    T(c && (c->tex2d_desc.Width == 32) && (c->tex2d_desc.Height == 16) && (c->tex2d_desc.MipLevels == 3) && (c->tex2d_desc.ArraySize == 1));
    T(c && (c->tex2d_desc.Format == DXGI_FORMAT_R8G8B8A8_UNORM) && (c->tex2d_desc.BindFlags == SRV));
    T(c && (c->tex2d_desc.Usage == D3D11_USAGE_DEFAULT) && (c->tex2d_desc.CPUAccessFlags == 0) && (c->tex2d_desc.MiscFlags == 0));
    T(c && (c->tex2d_desc.SampleDesc.Count == 1) && (c->tex2d_desc.SampleDesc.Quality == 0));
    sg_destroy_image(img);
    // write_transient 2D: still DEFAULT usage (updated via UpdateSubresource)
    c = make_image_get_create(&(sg_image_desc){ .width = 8, .height = 8, .pixel_format = SG_PIXELFORMAT_BGRA8, .usage.write_transient = true }, &img);
    T(c && (c->tex2d_desc.Format == DXGI_FORMAT_B8G8R8A8_UNORM) && (c->tex2d_desc.Usage == D3D11_USAGE_DEFAULT) && (c->tex2d_desc.CPUAccessFlags == 0));
    sg_destroy_image(img);
    // cube map
    c = make_image_get_create(&(sg_image_desc){
        .type = SG_IMAGETYPE_CUBE, .width = 8, .height = 8, .pixel_format = SG_PIXELFORMAT_RGBA8, .usage.copy_dst = true,
    }, &img);
    T(c && (c->tex2d_desc.ArraySize == 6) && (c->tex2d_desc.MiscFlags == D3D11_RESOURCE_MISC_TEXTURECUBE));
    sg_destroy_image(img);
    // array texture
    c = make_image_get_create(&(sg_image_desc){
        .type = SG_IMAGETYPE_ARRAY, .width = 8, .height = 8, .num_slices = 5, .pixel_format = SG_PIXELFORMAT_RGBA8, .usage.copy_dst = true,
    }, &img);
    T(c && (c->tex2d_desc.ArraySize == 5) && (c->tex2d_desc.MiscFlags == 0));
    sg_destroy_image(img);
    // MSAA color attachment
    c = make_image_get_create(&(sg_image_desc){
        .width = 64, .height = 64, .pixel_format = SG_PIXELFORMAT_RGBA8, .sample_count = 4, .usage.color_attachment = true,
    }, &img);
    T(c && (c->tex2d_desc.BindFlags == (SRV | D3D11_BIND_RENDER_TARGET)));
    T(c && (c->tex2d_desc.SampleDesc.Count == 4) && (c->tex2d_desc.SampleDesc.Quality == D3D11_STANDARD_MULTISAMPLE_PATTERN));
    sg_destroy_image(img);
    // resolve attachment: no render-target bind flag needed
    c = make_image_get_create(&(sg_image_desc){
        .width = 64, .height = 64, .pixel_format = SG_PIXELFORMAT_RGBA8, .usage.resolve_attachment = true,
    }, &img);
    T(c && (c->tex2d_desc.BindFlags == SRV) && (c->tex2d_desc.SampleDesc.Count == 1));
    sg_destroy_image(img);
    // depth-stencil attachment => typeless texture format
    c = make_image_get_create(&(sg_image_desc){
        .width = 64, .height = 64, .pixel_format = SG_PIXELFORMAT_DEPTH_STENCIL, .usage.depth_stencil_attachment = true,
    }, &img);
    T(c && (c->tex2d_desc.Format == DXGI_FORMAT_R32G8X24_TYPELESS) && (c->tex2d_desc.BindFlags == (SRV | D3D11_BIND_DEPTH_STENCIL)));
    sg_destroy_image(img);
    c = make_image_get_create(&(sg_image_desc){
        .width = 64, .height = 64, .pixel_format = SG_PIXELFORMAT_DEPTH, .usage.depth_stencil_attachment = true,
    }, &img);
    T(c && (c->tex2d_desc.Format == DXGI_FORMAT_R32_TYPELESS));
    sg_destroy_image(img);
    // storage image
    c = make_image_get_create(&(sg_image_desc){
        .width = 16, .height = 16, .pixel_format = SG_PIXELFORMAT_RGBA8, .usage.storage_image = true,
    }, &img);
    T(c && (c->tex2d_desc.BindFlags == (SRV | D3D11_BIND_UNORDERED_ACCESS)));
    sg_destroy_image(img);
    // BC3 compressed
    c = make_image_get_create(&(sg_image_desc){
        .width = 16, .height = 16, .pixel_format = SG_PIXELFORMAT_BC3_RGBA, .data.mip_levels[0] = { pattern, 256 },
    }, &img);
    T(c && (c->tex2d_desc.Format == DXGI_FORMAT_BC3_UNORM) && c->has_init_data && (c->src_row_pitch == 64));
    sg_destroy_image(img);
    T(no_errors());
    teardown();
}

UTEST(sokol_gfx_d3d11, image_init_data_pitches) {
    setup();
    fill_pattern();
    sg_image img;
    // 2D with 2 mips: first subresource points at mip0 data, row pitch, no slice pitch
    const d3d11_mock_call_t* c = make_image_get_create(&(sg_image_desc){
        .width = 16, .height = 16, .num_mipmaps = 2, .pixel_format = SG_PIXELFORMAT_RGBA8,
        .data.mip_levels = { [0] = { pattern, 1024 }, [1] = { pattern + 1024, 256 } },
    }, &img);
    T(no_errors() && (sg_query_image_state(img) == SG_RESOURCESTATE_VALID));
    T(c && c->has_init_data && (c->src_data == pattern) && (c->src_row_pitch == 64) && (c->src_depth_pitch == 0));
    sg_destroy_image(img);
    // array: per-slice data is split out of the mip-level data
    c = make_image_get_create(&(sg_image_desc){
        .type = SG_IMAGETYPE_ARRAY, .width = 4, .height = 4, .num_slices = 3, .pixel_format = SG_PIXELFORMAT_RGBA8,
        .data.mip_levels[0] = { pattern, 3 * 64 },
    }, &img);
    T(c && c->has_init_data && (c->src_data == pattern) && (c->src_row_pitch == 16) && (c->src_depth_pitch == 0));
    T(c && (c->tex2d_desc.ArraySize == 3));
    sg_destroy_image(img);
    // 3D: slice pitch is set
    c = make_image_get_create(&(sg_image_desc){
        .type = SG_IMAGETYPE_3D, .width = 8, .height = 8, .num_slices = 4, .pixel_format = SG_PIXELFORMAT_RGBA8,
        .data.mip_levels[0] = { pattern, 1024 },
    }, &img);
    T(no_errors() && (sg_query_image_state(img) == SG_RESOURCESTATE_VALID));
    T(c && (c->dst == sg_d3d11_query_image_info(img).tex3d) && (c->dst == ntex(img)));
    T(c && (c->tex3d_desc.Width == 8) && (c->tex3d_desc.Height == 8) && (c->tex3d_desc.Depth == 4) && (c->tex3d_desc.MipLevels == 1));
    T(c && (c->tex3d_desc.Format == DXGI_FORMAT_R8G8B8A8_UNORM) && (c->tex3d_desc.BindFlags == D3D11_BIND_SHADER_RESOURCE));
    T(c && (c->tex3d_desc.Usage == D3D11_USAGE_DEFAULT) && (c->tex3d_desc.CPUAccessFlags == 0));
    T(c && c->has_init_data && (c->src_data == pattern) && (c->src_row_pitch == 32) && (c->src_depth_pitch == 256));
    sg_destroy_image(img);
    // no data => no init data
    c = make_image_get_create(&(sg_image_desc){
        .type = SG_IMAGETYPE_3D, .width = 8, .height = 8, .num_slices = 4, .pixel_format = SG_PIXELFORMAT_RGBA8, .usage.copy_dst = true,
    }, &img);
    T(c && !c->has_init_data);
    sg_destroy_image(img);
    teardown();
}

//------------------------------------------------------------------------------
//  sg_write_image_transient / sg_write_image_unsealed
//------------------------------------------------------------------------------
UTEST(sokol_gfx_d3d11, write_image_transient_2d) {
    setup();
    fill_pattern();
    sg_image img = sg_make_image(&(sg_image_desc){ .width = 16, .height = 16, .pixel_format = SG_PIXELFORMAT_RGBA8, .usage.write_transient = true });
    d3d11_mock_clear_calls();
    sg_write_image_transient(&(sg_write_image_desc){ .src.data = { pattern, 1024 }, .dst.image = img });
    T(no_errors());
    T(d3d11_mock_num_calls() == 1);
    const d3d11_mock_call_t* c = LAST(UPDATE_SUBRESOURCE);
    T(c && (c->dst == ntex(img)) && (c->dst_subres == 0) && c->has_box);
    T(c && (c->box.left == 0) && (c->box.right == 16) && (c->box.top == 0) && (c->box.bottom == 16) && (c->box.front == 0) && (c->box.back == 1));
    T(c && (c->src_data == pattern) && (c->src_row_pitch == 64) && (c->src_depth_pitch == 0));
    T(COUNT(MAP) == 0);
    sg_commit();
    // subrect with explicit pitch and source offset
    d3d11_mock_clear_calls();
    sg_write_image_transient(&(sg_write_image_desc){
        .src = { .data = { pattern, 2048 }, .offset = 32, .bytes_per_row = 64 },
        .dst = { .image = img, .x = 8, .y = 4 },
        .size = { .width = 4, .height = 8 },
    });
    T(no_errors());
    c = LAST(UPDATE_SUBRESOURCE);
    T(c && (c->box.left == 8) && (c->box.right == 12) && (c->box.top == 4) && (c->box.bottom == 12));
    T(c && (c->src_data == pattern + 32) && (c->src_row_pitch == 64));
    sg_destroy_image(img);
    teardown();
}

UTEST(sokol_gfx_d3d11, write_image_transient_3d) {
    setup();
    fill_pattern();
    sg_image img = sg_make_image(&(sg_image_desc){
        .type = SG_IMAGETYPE_3D, .width = 8, .height = 8, .num_slices = 4,
        .pixel_format = SG_PIXELFORMAT_RGBA8, .usage.write_transient = true,
    });
    d3d11_mock_clear_calls();
    sg_write_image_transient(&(sg_write_image_desc){
        .src = { .data = { pattern, 1024 }, .offset = 256 },
        .dst = { .image = img, .slice = 1 },
        .size = { .num_slices = 2 },
    });
    T(no_errors());
    T(COUNT(UPDATE_SUBRESOURCE) == 1);
    const d3d11_mock_call_t* c = LAST(UPDATE_SUBRESOURCE);
    T(c && (c->dst == ntex(img)) && (c->dst_subres == 0));
    T(c && (c->box.left == 0) && (c->box.right == 8) && (c->box.top == 0) && (c->box.bottom == 8) && (c->box.front == 1) && (c->box.back == 3));
    T(c && (c->src_data == pattern + 256) && (c->src_row_pitch == 32) && (c->src_depth_pitch == 256));
    sg_destroy_image(img);
    teardown();
}

UTEST(sokol_gfx_d3d11, write_image_unsealed_cube_mips_and_seal) {
    setup();
    fill_pattern();
    sg_image img = sg_make_image(&(sg_image_desc){
        .type = SG_IMAGETYPE_CUBE, .width = 8, .height = 8, .num_mipmaps = 2,
        .pixel_format = SG_PIXELFORMAT_RGBA8, .usage.write_unsealed = true,
    });
    T(no_errors());
    T(sg_query_image_state(img) == SG_RESOURCESTATE_UNSEALED);
    const d3d11_mock_call_t* c = LAST(CREATE_TEXTURE2D);
    T(c && !c->has_init_data && (c->tex2d_desc.ArraySize == 6) && (c->tex2d_desc.MipLevels == 2));
    d3d11_mock_clear_calls();
    // mip 1 (4x4), faces 2..4
    sg_write_image_unsealed(&(sg_write_image_desc){
        .src.data = { pattern, 3 * 64 },
        .dst = { .image = img, .mip_level = 1, .slice = 2 },
        .size.num_slices = 3,
    });
    T(no_errors());
    T(COUNT(UPDATE_SUBRESOURCE) == 3);
    for (int i = 0; i < 3; i++) {
        c = NTH(UPDATE_SUBRESOURCE, i);
        T(c && (c->dst == ntex(img)) && (c->dst_subres == (UINT)(1 + (2 + i) * 2)));
        T(c && (c->box.right == 4) && (c->box.bottom == 4) && (c->box.front == 0) && (c->box.back == 1));
        T(c && (c->src_data == pattern + i * 64) && (c->src_row_pitch == 16) && (c->src_depth_pitch == 0));
    }
    d3d11_mock_clear_calls();
    sg_seal_image(img);
    T(sg_query_image_state(img) == SG_RESOURCESTATE_VALID);
    T(d3d11_mock_num_calls() == 0);
    sg_write_image_unsealed(&(sg_write_image_desc){ .src.data = { pattern, 256 }, .dst.image = img });
    T(logged(SG_LOGITEM_VALIDATION_FAILED));
    T(d3d11_mock_num_calls() == 0);
    sg_destroy_image(img);
    teardown();
}

UTEST(sokol_gfx_d3d11, write_image_unsealed_array) {
    setup();
    fill_pattern();
    sg_image img = sg_make_image(&(sg_image_desc){
        .type = SG_IMAGETYPE_ARRAY, .width = 4, .height = 4, .num_slices = 4,
        .pixel_format = SG_PIXELFORMAT_RGBA8, .usage.write_unsealed = true,
    });
    d3d11_mock_clear_calls();
    sg_write_image_unsealed(&(sg_write_image_desc){ .src.data = { pattern, 4 * 64 }, .dst.image = img });
    T(no_errors());
    T(COUNT(UPDATE_SUBRESOURCE) == 4);
    const d3d11_mock_call_t* c = NTH(UPDATE_SUBRESOURCE, 3);
    T(c && (c->dst_subres == 3) && (c->src_data == pattern + 192));
    sg_seal_image(img);
    T(sg_query_image_state(img) == SG_RESOURCESTATE_VALID);
    sg_destroy_image(img);
    teardown();
}

//------------------------------------------------------------------------------
//  injected native objects: no Create*, AddRef'd, returned by query functions
//------------------------------------------------------------------------------
UTEST(sokol_gfx_d3d11, inject_d3d11_buffer) {
    setup();
    ID3D11Buffer* raw = NULL;
    D3D11_BUFFER_DESC bd = { .ByteWidth = 256, .BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_UNORDERED_ACCESS, .MiscFlags = D3D11_RESOURCE_MISC_BUFFER_ALLOW_RAW_VIEWS };
    mock_dev->lpVtbl->CreateBuffer(mock_dev, &bd, NULL, &raw);
    T(raw && (d3d11_mock_refcount(raw) == 1));
    d3d11_mock_clear_calls();
    sg_buffer buf = sg_make_buffer(&(sg_buffer_desc){ .size = 256, .usage.storage_buffer = true, .d3d11_buffer = raw });
    T(no_errors() && (sg_query_buffer_state(buf) == SG_RESOURCESTATE_VALID));
    T(COUNT(CREATE_BUFFER) == 0);
    T(nbuf(buf) == raw);
    T(d3d11_mock_refcount(raw) == 2);
    // a storage-buffer view on the injected buffer references the native buffer
    sg_view v = sg_make_view(&(sg_view_desc){ .storage_buffer.buffer = buf });
    T(LAST(CREATE_SRV) && (LAST(CREATE_SRV)->src == raw));
    T(LAST(CREATE_UAV) && (LAST(CREATE_UAV)->src == raw));
    sg_destroy_view(v);
    sg_destroy_buffer(buf);
    T(d3d11_mock_refcount(raw) == 1);
    raw->lpVtbl->Release(raw);
    T(d3d11_mock_refcount(raw) == 0);
    teardown();
}

UTEST(sokol_gfx_d3d11, inject_d3d11_texture_2d) {
    setup();
    ID3D11Texture2D* raw = NULL;
    D3D11_TEXTURE2D_DESC td = { .Width = 16, .Height = 16, .MipLevels = 1, .ArraySize = 1,
                                .Format = DXGI_FORMAT_R8G8B8A8_UNORM, .SampleDesc = { .Count = 1 },
                                .BindFlags = D3D11_BIND_SHADER_RESOURCE };
    mock_dev->lpVtbl->CreateTexture2D(mock_dev, &td, NULL, &raw);
    T(raw && (d3d11_mock_refcount(raw) == 1));
    d3d11_mock_clear_calls();
    sg_image img = sg_make_image(&(sg_image_desc){ .width = 16, .height = 16, .pixel_format = SG_PIXELFORMAT_RGBA8, .d3d11_texture = raw });
    T(no_errors() && (sg_query_image_state(img) == SG_RESOURCESTATE_VALID));
    T(COUNT(CREATE_TEXTURE2D) == 0);
    T(sg_d3d11_query_image_info(img).tex2d == raw);
    T(sg_d3d11_query_image_info(img).tex3d == NULL);
    T(ntex(img) == raw);
    // one reference for the texture pointer, one for the resource pointer
    T(d3d11_mock_refcount(raw) == 3);
    sg_view v = sg_make_view(&(sg_view_desc){ .texture.image = img });
    const d3d11_mock_call_t* c = LAST(CREATE_SRV);
    T(c && (c->src == raw) && (c->dst == nsrv(v)));
    sg_destroy_view(v);
    sg_destroy_image(img);
    T(d3d11_mock_refcount(raw) == 1);
    raw->lpVtbl->Release(raw);
    teardown();
}

UTEST(sokol_gfx_d3d11, inject_d3d11_texture_3d) {
    setup();
    ID3D11Texture3D* raw = NULL;
    D3D11_TEXTURE3D_DESC td = { .Width = 8, .Height = 8, .Depth = 4, .MipLevels = 1,
                                .Format = DXGI_FORMAT_R8G8B8A8_UNORM, .BindFlags = D3D11_BIND_SHADER_RESOURCE };
    mock_dev->lpVtbl->CreateTexture3D(mock_dev, &td, NULL, &raw);
    T(raw && (d3d11_mock_refcount(raw) == 1));
    d3d11_mock_clear_calls();
    sg_image img = sg_make_image(&(sg_image_desc){
        .type = SG_IMAGETYPE_3D, .width = 8, .height = 8, .num_slices = 4,
        .pixel_format = SG_PIXELFORMAT_RGBA8, .d3d11_texture = raw,
    });
    T(no_errors() && (sg_query_image_state(img) == SG_RESOURCESTATE_VALID));
    T(COUNT(CREATE_TEXTURE3D) == 0);
    T(sg_d3d11_query_image_info(img).tex3d == raw);
    T(sg_d3d11_query_image_info(img).tex2d == NULL);
    T(d3d11_mock_refcount(raw) == 3);
    sg_destroy_image(img);
    T(d3d11_mock_refcount(raw) == 1);
    raw->lpVtbl->Release(raw);
    teardown();
}

UTEST(sokol_gfx_d3d11, inject_d3d11_sampler) {
    setup();
    ID3D11SamplerState* raw = NULL;
    D3D11_SAMPLER_DESC sd = { .Filter = D3D11_FILTER_MIN_MAG_MIP_POINT, .MaxLOD = 1000.0f };
    mock_dev->lpVtbl->CreateSamplerState(mock_dev, &sd, &raw);
    T(raw && (d3d11_mock_refcount(raw) == 1));
    d3d11_mock_clear_calls();
    sg_sampler smp = sg_make_sampler(&(sg_sampler_desc){ .d3d11_sampler = raw });
    T(no_errors() && (sg_query_sampler_state(smp) == SG_RESOURCESTATE_VALID));
    T(COUNT(CREATE_SAMPLER_STATE) == 0);
    T(nsmp(smp) == raw);
    T(d3d11_mock_refcount(raw) == 2);
    sg_destroy_sampler(smp);
    T(d3d11_mock_refcount(raw) == 1);
    raw->lpVtbl->Release(raw);
    teardown();
}

UTEST(sokol_gfx_d3d11, inject_validation_rejects_transient_and_data) {
    setup();
    fill_pattern();
    ID3D11Buffer* raw = NULL;
    D3D11_BUFFER_DESC bd = { .ByteWidth = 64, .BindFlags = D3D11_BIND_VERTEX_BUFFER };
    mock_dev->lpVtbl->CreateBuffer(mock_dev, &bd, NULL, &raw);
    sg_buffer b0 = sg_make_buffer(&(sg_buffer_desc){ .size = 64, .usage.write_transient = true, .d3d11_buffer = raw });
    T(sg_query_buffer_state(b0) == SG_RESOURCESTATE_FAILED);
    T(logged(SG_LOGITEM_VALIDATION_FAILED));
    T(d3d11_mock_refcount(raw) == 1);
    sg_destroy_buffer(b0);
    raw->lpVtbl->Release(raw);
    teardown();
}

//------------------------------------------------------------------------------
//  view creation => D3D11_*_VIEW_DESC
//------------------------------------------------------------------------------
UTEST(sokol_gfx_d3d11, view_texture_srv_descs) {
    setup();
    // 2D with mip range
    sg_image img2d = sg_make_image(&(sg_image_desc){ .width = 16, .height = 16, .num_mipmaps = 4, .pixel_format = SG_PIXELFORMAT_RGBA8, .usage.copy_dst = true });
    d3d11_mock_clear_calls();
    sg_view v = sg_make_view(&(sg_view_desc){ .texture = { .image = img2d, .mip_levels = { .base = 1, .count = 2 } } });
    T(no_errors() && (sg_query_view_state(v) == SG_RESOURCESTATE_VALID));
    const d3d11_mock_call_t* c = LAST(CREATE_SRV);
    T(c && (c->dst == nsrv(v)) && (c->src == ntex(img2d)) && c->has_view_desc);
    T(c && (c->srv_desc.Format == DXGI_FORMAT_R8G8B8A8_UNORM) && (c->srv_desc.ViewDimension == D3D11_SRV_DIMENSION_TEXTURE2D));
    T(c && (c->srv_desc.Texture2D.MostDetailedMip == 1) && (c->srv_desc.Texture2D.MipLevels == 2));
    T(nuav(v) == NULL && nrtv(v) == NULL && ndsv(v) == NULL);
    sg_destroy_view(v);
    // cube
    sg_image cube = sg_make_image(&(sg_image_desc){ .type = SG_IMAGETYPE_CUBE, .width = 8, .height = 8, .num_mipmaps = 2, .pixel_format = SG_PIXELFORMAT_RGBA8, .usage.copy_dst = true });
    v = sg_make_view(&(sg_view_desc){ .texture.image = cube });
    c = LAST(CREATE_SRV);
    T(c && (c->srv_desc.ViewDimension == D3D11_SRV_DIMENSION_TEXTURECUBE));
    T(c && (c->srv_desc.TextureCube.MostDetailedMip == 0) && (c->srv_desc.TextureCube.MipLevels == 2));
    sg_destroy_view(v);
    // array with slice range
    sg_image arr = sg_make_image(&(sg_image_desc){ .type = SG_IMAGETYPE_ARRAY, .width = 8, .height = 8, .num_slices = 4, .num_mipmaps = 2, .pixel_format = SG_PIXELFORMAT_RGBA8, .usage.copy_dst = true });
    v = sg_make_view(&(sg_view_desc){ .texture = { .image = arr, .mip_levels = { .base = 1 }, .slices = { .base = 1, .count = 2 } } });
    c = LAST(CREATE_SRV);
    T(c && (c->srv_desc.ViewDimension == D3D11_SRV_DIMENSION_TEXTURE2DARRAY));
    T(c && (c->srv_desc.Texture2DArray.MostDetailedMip == 1) && (c->srv_desc.Texture2DArray.MipLevels == 1));
    T(c && (c->srv_desc.Texture2DArray.FirstArraySlice == 1) && (c->srv_desc.Texture2DArray.ArraySize == 2));
    sg_destroy_view(v);
    // 3D
    sg_image vol = sg_make_image(&(sg_image_desc){ .type = SG_IMAGETYPE_3D, .width = 8, .height = 8, .num_slices = 8, .num_mipmaps = 3, .pixel_format = SG_PIXELFORMAT_RGBA8, .usage.copy_dst = true });
    v = sg_make_view(&(sg_view_desc){ .texture = { .image = vol, .mip_levels = { .base = 2 } } });
    c = LAST(CREATE_SRV);
    T(c && (c->src == ntex(vol)) && (c->srv_desc.ViewDimension == D3D11_SRV_DIMENSION_TEXTURE3D));
    T(c && (c->srv_desc.Texture3D.MostDetailedMip == 2) && (c->srv_desc.Texture3D.MipLevels == 1));
    sg_destroy_view(v);
    // MSAA
    sg_image msaa = sg_make_image(&(sg_image_desc){ .width = 16, .height = 16, .sample_count = 4, .pixel_format = SG_PIXELFORMAT_RGBA8, .usage.color_attachment = true });
    v = sg_make_view(&(sg_view_desc){ .texture.image = msaa });
    c = LAST(CREATE_SRV);
    T(c && (c->srv_desc.ViewDimension == D3D11_SRV_DIMENSION_TEXTURE2DMS));
    sg_destroy_view(v);
    // depth-stencil texture => readable SRV format
    sg_image ds = sg_make_image(&(sg_image_desc){ .width = 16, .height = 16, .pixel_format = SG_PIXELFORMAT_DEPTH_STENCIL, .usage.depth_stencil_attachment = true });
    v = sg_make_view(&(sg_view_desc){ .texture.image = ds });
    c = LAST(CREATE_SRV);
    T(c && (c->srv_desc.Format == DXGI_FORMAT_R32_FLOAT_X8X24_TYPELESS));
    sg_destroy_view(v);
    T(no_errors());
    sg_destroy_image(img2d);
    sg_destroy_image(cube);
    sg_destroy_image(arr);
    sg_destroy_image(vol);
    sg_destroy_image(msaa);
    sg_destroy_image(ds);
    teardown();
}

UTEST(sokol_gfx_d3d11, view_attachment_rtv_dsv_descs) {
    setup();
    sg_image col = sg_make_image(&(sg_image_desc){ .width = 16, .height = 16, .num_mipmaps = 2, .pixel_format = SG_PIXELFORMAT_RGBA8, .usage.color_attachment = true });
    d3d11_mock_clear_calls();
    sg_view v = sg_make_view(&(sg_view_desc){ .color_attachment = { .image = col, .mip_level = 1 } });
    const d3d11_mock_call_t* c = LAST(CREATE_RTV);
    T(c && (c->dst == nrtv(v)) && (c->src == ntex(col)) && c->has_view_desc);
    T(c && (c->rtv_desc.Format == DXGI_FORMAT_R8G8B8A8_UNORM) && (c->rtv_desc.ViewDimension == D3D11_RTV_DIMENSION_TEXTURE2D) && (c->rtv_desc.Texture2D.MipSlice == 1));
    sg_destroy_view(v);
    sg_image msaa = sg_make_image(&(sg_image_desc){ .width = 16, .height = 16, .sample_count = 4, .pixel_format = SG_PIXELFORMAT_RGBA8, .usage.color_attachment = true });
    v = sg_make_view(&(sg_view_desc){ .color_attachment.image = msaa });
    c = LAST(CREATE_RTV);
    T(c && (c->rtv_desc.ViewDimension == D3D11_RTV_DIMENSION_TEXTURE2DMS));
    sg_destroy_view(v);
    sg_image arr = sg_make_image(&(sg_image_desc){ .type = SG_IMAGETYPE_ARRAY, .width = 16, .height = 16, .num_slices = 4, .pixel_format = SG_PIXELFORMAT_RGBA8, .usage.color_attachment = true });
    v = sg_make_view(&(sg_view_desc){ .color_attachment = { .image = arr, .slice = 2 } });
    c = LAST(CREATE_RTV);
    T(c && (c->rtv_desc.ViewDimension == D3D11_RTV_DIMENSION_TEXTURE2DARRAY));
    T(c && (c->rtv_desc.Texture2DArray.MipSlice == 0) && (c->rtv_desc.Texture2DArray.FirstArraySlice == 2) && (c->rtv_desc.Texture2DArray.ArraySize == 1));
    sg_destroy_view(v);
    sg_image cube = sg_make_image(&(sg_image_desc){ .type = SG_IMAGETYPE_CUBE, .width = 16, .height = 16, .pixel_format = SG_PIXELFORMAT_RGBA8, .usage.color_attachment = true });
    v = sg_make_view(&(sg_view_desc){ .color_attachment = { .image = cube, .slice = 5 } });
    c = LAST(CREATE_RTV);
    T(c && (c->rtv_desc.ViewDimension == D3D11_RTV_DIMENSION_TEXTURE2DARRAY) && (c->rtv_desc.Texture2DArray.FirstArraySlice == 5));
    sg_destroy_view(v);
    sg_image vol = sg_make_image(&(sg_image_desc){ .type = SG_IMAGETYPE_3D, .width = 16, .height = 16, .num_slices = 4, .pixel_format = SG_PIXELFORMAT_RGBA8, .usage.color_attachment = true });
    v = sg_make_view(&(sg_view_desc){ .color_attachment = { .image = vol, .slice = 0 } });
    c = LAST(CREATE_RTV);
    T(c && (c->rtv_desc.ViewDimension == D3D11_RTV_DIMENSION_TEXTURE3D));
    T(c && (c->rtv_desc.Texture3D.MipSlice == 0) && (c->rtv_desc.Texture3D.FirstWSlice == 0) && (c->rtv_desc.Texture3D.WSize == 1));
    sg_destroy_view(v);
    // depth-stencil views
    sg_image ds = sg_make_image(&(sg_image_desc){ .width = 16, .height = 16, .num_mipmaps = 2, .pixel_format = SG_PIXELFORMAT_DEPTH_STENCIL, .usage.depth_stencil_attachment = true });
    v = sg_make_view(&(sg_view_desc){ .depth_stencil_attachment = { .image = ds, .mip_level = 1 } });
    c = LAST(CREATE_DSV);
    T(c && (c->dst == ndsv(v)) && (c->src == ntex(ds)));
    T(c && (c->dsv_desc.Format == DXGI_FORMAT_D32_FLOAT_S8X24_UINT) && (c->dsv_desc.ViewDimension == D3D11_DSV_DIMENSION_TEXTURE2D) && (c->dsv_desc.Texture2D.MipSlice == 1));
    sg_destroy_view(v);
    sg_image d = sg_make_image(&(sg_image_desc){ .width = 16, .height = 16, .sample_count = 4, .pixel_format = SG_PIXELFORMAT_DEPTH, .usage.depth_stencil_attachment = true });
    v = sg_make_view(&(sg_view_desc){ .depth_stencil_attachment.image = d });
    c = LAST(CREATE_DSV);
    T(c && (c->dsv_desc.Format == DXGI_FORMAT_D32_FLOAT) && (c->dsv_desc.ViewDimension == D3D11_DSV_DIMENSION_TEXTURE2DMS));
    sg_destroy_view(v);
    sg_image darr = sg_make_image(&(sg_image_desc){ .type = SG_IMAGETYPE_ARRAY, .width = 16, .height = 16, .num_slices = 3, .pixel_format = SG_PIXELFORMAT_DEPTH, .usage.depth_stencil_attachment = true });
    v = sg_make_view(&(sg_view_desc){ .depth_stencil_attachment = { .image = darr, .slice = 2 } });
    c = LAST(CREATE_DSV);
    T(c && (c->dsv_desc.ViewDimension == D3D11_DSV_DIMENSION_TEXTURE2DARRAY));
    T(c && (c->dsv_desc.Texture2DArray.MipSlice == 0) && (c->dsv_desc.Texture2DArray.FirstArraySlice == 2) && (c->dsv_desc.Texture2DArray.ArraySize == 1));
    sg_destroy_view(v);
    T(no_errors());
    sg_destroy_image(col); sg_destroy_image(msaa); sg_destroy_image(arr); sg_destroy_image(cube);
    sg_destroy_image(vol); sg_destroy_image(ds); sg_destroy_image(d); sg_destroy_image(darr);
    teardown();
}

UTEST(sokol_gfx_d3d11, view_storage_image_uav_descs) {
    setup();
    sg_image img = sg_make_image(&(sg_image_desc){ .width = 16, .height = 16, .num_mipmaps = 2, .pixel_format = SG_PIXELFORMAT_RGBA8, .usage.storage_image = true });
    d3d11_mock_clear_calls();
    sg_view v = sg_make_view(&(sg_view_desc){ .storage_image = { .image = img, .mip_level = 1 } });
    const d3d11_mock_call_t* c = LAST(CREATE_UAV);
    T(c && (c->dst == nuav(v)) && (c->src == ntex(img)) && c->has_view_desc);
    T(c && (c->uav_desc.Format == DXGI_FORMAT_R8G8B8A8_UNORM) && (c->uav_desc.ViewDimension == D3D11_UAV_DIMENSION_TEXTURE2D) && (c->uav_desc.Texture2D.MipSlice == 1));
    sg_destroy_view(v);
    sg_image arr = sg_make_image(&(sg_image_desc){ .type = SG_IMAGETYPE_ARRAY, .width = 16, .height = 16, .num_slices = 3, .pixel_format = SG_PIXELFORMAT_RGBA8, .usage.storage_image = true });
    v = sg_make_view(&(sg_view_desc){ .storage_image = { .image = arr, .slice = 1 } });
    c = LAST(CREATE_UAV);
    T(c && (c->uav_desc.ViewDimension == D3D11_UAV_DIMENSION_TEXTURE2DARRAY));
    T(c && (c->uav_desc.Texture2DArray.MipSlice == 0) && (c->uav_desc.Texture2DArray.FirstArraySlice == 1) && (c->uav_desc.Texture2DArray.ArraySize == 1));
    sg_destroy_view(v);
    sg_image vol = sg_make_image(&(sg_image_desc){ .type = SG_IMAGETYPE_3D, .width = 16, .height = 16, .num_slices = 4, .pixel_format = SG_PIXELFORMAT_RGBA8, .usage.storage_image = true });
    v = sg_make_view(&(sg_view_desc){ .storage_image = { .image = vol, .slice = 0 } });
    c = LAST(CREATE_UAV);
    T(c && (c->uav_desc.ViewDimension == D3D11_UAV_DIMENSION_TEXTURE3D));
    T(c && (c->uav_desc.Texture3D.MipSlice == 0) && (c->uav_desc.Texture3D.FirstWSlice == 0) && (c->uav_desc.Texture3D.WSize == 1));
    sg_destroy_view(v);
    T(no_errors());
    sg_destroy_image(img);
    sg_destroy_image(arr);
    sg_destroy_image(vol);
    teardown();
}

UTEST(sokol_gfx_d3d11, view_storage_buffer_srv_uav_descs) {
    setup();
    sg_buffer buf = sg_make_buffer(&(sg_buffer_desc){ .size = 1024, .usage.storage_buffer = true });
    d3d11_mock_clear_calls();
    sg_view v = sg_make_view(&(sg_view_desc){ .storage_buffer = { .buffer = buf, .offset = 256 } });
    T(no_errors() && (sg_query_view_state(v) == SG_RESOURCESTATE_VALID));
    const d3d11_mock_call_t* c = LAST(CREATE_SRV);
    T(c && (c->dst == nsrv(v)) && (c->src == nbuf(buf)) && c->has_view_desc);
    T(c && (c->srv_desc.Format == DXGI_FORMAT_R32_TYPELESS) && (c->srv_desc.ViewDimension == D3D11_SRV_DIMENSION_BUFFEREX));
    T(c && (c->srv_desc.BufferEx.FirstElement == 64) && (c->srv_desc.BufferEx.NumElements == 192) && (c->srv_desc.BufferEx.Flags == D3D11_BUFFEREX_SRV_FLAG_RAW));
    c = LAST(CREATE_UAV);
    T(c && (c->dst == nuav(v)) && (c->src == nbuf(buf)) && c->has_view_desc);
    T(c && (c->uav_desc.Format == DXGI_FORMAT_R32_TYPELESS) && (c->uav_desc.ViewDimension == D3D11_UAV_DIMENSION_BUFFER));
    T(c && (c->uav_desc.Buffer.FirstElement == 64) && (c->uav_desc.Buffer.NumElements == 192) && (c->uav_desc.Buffer.Flags == D3D11_BUFFER_UAV_FLAG_RAW));
    sg_destroy_view(v);
    // write_transient storage buffers (DYNAMIC usage) can't have an UAV
    sg_buffer tbuf = sg_make_buffer(&(sg_buffer_desc){ .size = 256, .usage = { .storage_buffer = true, .write_transient = true } });
    d3d11_mock_clear_calls();
    v = sg_make_view(&(sg_view_desc){ .storage_buffer.buffer = tbuf });
    T(no_errors() && (sg_query_view_state(v) == SG_RESOURCESTATE_VALID));
    T(COUNT(CREATE_SRV) == 1);
    T(COUNT(CREATE_UAV) == 0);
    T(nsrv(v) != NULL);
    T(nuav(v) == NULL);
    c = LAST(CREATE_SRV);
    T(c && (c->srv_desc.BufferEx.FirstElement == 0) && (c->srv_desc.BufferEx.NumElements == 64));
    sg_destroy_view(v);
    sg_destroy_buffer(buf);
    sg_destroy_buffer(tbuf);
    teardown();
}

UTEST(sokol_gfx_d3d11, view_create_failures) {
    setup();
    sg_buffer buf = sg_make_buffer(&(sg_buffer_desc){ .size = 256, .usage.storage_buffer = true });
    sg_image simg = sg_make_image(&(sg_image_desc){ .width = 8, .height = 8, .pixel_format = SG_PIXELFORMAT_RGBA8, .usage.storage_image = true });
    sg_image cimg = sg_make_image(&(sg_image_desc){ .width = 8, .height = 8, .pixel_format = SG_PIXELFORMAT_RGBA8, .usage.color_attachment = true });
    sg_image dimg = sg_make_image(&(sg_image_desc){ .width = 8, .height = 8, .pixel_format = SG_PIXELFORMAT_DEPTH, .usage.depth_stencil_attachment = true });
    const int before = d3d11_mock_live_object_count();

    d3d11_mock_fail_next_create(1);
    sg_view v = sg_make_view(&(sg_view_desc){ .storage_buffer.buffer = buf });
    T((sg_query_view_state(v) == SG_RESOURCESTATE_FAILED) && logged(SG_LOGITEM_D3D11_CREATE_BUFFER_SRV_FAILED));
    sg_destroy_view(v);
    T(d3d11_mock_live_object_count() == before);

    reset_log();
    d3d11_mock_fail_create_after(1, 1);   // SRV succeeds, UAV fails
    v = sg_make_view(&(sg_view_desc){ .storage_buffer.buffer = buf });
    T((sg_query_view_state(v) == SG_RESOURCESTATE_FAILED) && logged(SG_LOGITEM_D3D11_CREATE_BUFFER_UAV_FAILED));
    sg_destroy_view(v);
    T(d3d11_mock_live_object_count() == before);   // the SRV got released

    reset_log();
    d3d11_mock_fail_next_create(1);
    v = sg_make_view(&(sg_view_desc){ .storage_image.image = simg });
    T((sg_query_view_state(v) == SG_RESOURCESTATE_FAILED) && logged(SG_LOGITEM_D3D11_CREATE_UAV_FAILED));
    sg_destroy_view(v);

    reset_log();
    d3d11_mock_fail_next_create(1);
    v = sg_make_view(&(sg_view_desc){ .color_attachment.image = cimg });
    T((sg_query_view_state(v) == SG_RESOURCESTATE_FAILED) && logged(SG_LOGITEM_D3D11_CREATE_RTV_FAILED));
    sg_destroy_view(v);

    reset_log();
    d3d11_mock_fail_next_create(1);
    v = sg_make_view(&(sg_view_desc){ .depth_stencil_attachment.image = dimg });
    T((sg_query_view_state(v) == SG_RESOURCESTATE_FAILED) && logged(SG_LOGITEM_D3D11_CREATE_DSV_FAILED));
    sg_destroy_view(v);

    reset_log();
    d3d11_mock_fail_next_create(1);
    v = sg_make_view(&(sg_view_desc){ .texture.image = cimg });
    T((sg_query_view_state(v) == SG_RESOURCESTATE_FAILED) && logged(SG_LOGITEM_D3D11_CREATE_2D_SRV_FAILED));
    sg_destroy_view(v);
    T(d3d11_mock_live_object_count() == before);

    sg_destroy_buffer(buf);
    sg_destroy_image(simg);
    sg_destroy_image(cimg);
    sg_destroy_image(dimg);
    teardown();
}

//------------------------------------------------------------------------------
//  pass clear / resolve call arguments
//------------------------------------------------------------------------------
UTEST(sokol_gfx_d3d11, offscreen_pass_clear_args) {
    setup();
    sg_image cimg = sg_make_image(&(sg_image_desc){ .width = 32, .height = 32, .pixel_format = SG_PIXELFORMAT_RGBA8, .usage.color_attachment = true });
    sg_image cimg2 = sg_make_image(&(sg_image_desc){ .width = 32, .height = 32, .pixel_format = SG_PIXELFORMAT_RGBA8, .usage.color_attachment = true });
    sg_image dimg = sg_make_image(&(sg_image_desc){ .width = 32, .height = 32, .pixel_format = SG_PIXELFORMAT_DEPTH_STENCIL, .usage.depth_stencil_attachment = true });
    sg_view cv = sg_make_view(&(sg_view_desc){ .color_attachment.image = cimg });
    sg_view cv2 = sg_make_view(&(sg_view_desc){ .color_attachment.image = cimg2 });
    sg_view dv = sg_make_view(&(sg_view_desc){ .depth_stencil_attachment.image = dimg });
    d3d11_mock_clear_calls();
    sg_begin_pass(&(sg_pass){
        .attachments = { .colors = { [0] = cv, [1] = cv2 }, .depth_stencil = dv },
        .action = {
            .colors = {
                [0] = { .load_action = SG_LOADACTION_CLEAR, .clear_value = { 0.25f, 0.5f, 0.75f, 1.0f } },
                [1] = { .load_action = SG_LOADACTION_LOAD },
            },
            .depth = { .load_action = SG_LOADACTION_CLEAR, .clear_value = 0.5f },
            .stencil = { .load_action = SG_LOADACTION_CLEAR, .clear_value = 7 },
        },
    });
    sg_end_pass();
    T(no_errors());
    const d3d11_mock_call_t* c = LAST(OM_SET_RENDER_TARGETS);
    T(c && (c->args[0] == SG_MAX_COLOR_ATTACHMENTS) && (c->dst == nrtv(cv)) && (c->src == ndsv(dv)));
    T(COUNT(CLEAR_RENDER_TARGET_VIEW) == 1);   // only the CLEAR attachment
    c = LAST(CLEAR_RENDER_TARGET_VIEW);
    const float exp_color[4] = { 0.25f, 0.5f, 0.75f, 1.0f };
    T(c && (c->dst == nrtv(cv)) && (0 == memcmp(c->data, exp_color, sizeof(exp_color))));
    c = LAST(CLEAR_DEPTH_STENCIL_VIEW);
    T(c && (c->dst == ndsv(dv)) && (c->args[0] == (D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL)) && (c->args[1] == 7));
    float depth = 0.0f;
    if (c) { memcpy(&depth, c->data, sizeof(depth)); }
    T(depth == 0.5f);
    T(COUNT(RESOLVE_SUBRESOURCE) == 0);
    // stencil-only clear
    d3d11_mock_clear_calls();
    sg_begin_pass(&(sg_pass){
        .attachments = { .colors[0] = cv, .depth_stencil = dv },
        .action = {
            .colors[0].load_action = SG_LOADACTION_LOAD,
            .depth.load_action = SG_LOADACTION_LOAD,
            .stencil = { .load_action = SG_LOADACTION_CLEAR, .clear_value = 3 },
        },
    });
    sg_end_pass();
    T(COUNT(CLEAR_RENDER_TARGET_VIEW) == 0);
    c = LAST(CLEAR_DEPTH_STENCIL_VIEW);
    T(c && (c->args[0] == D3D11_CLEAR_STENCIL) && (c->args[1] == 3));
    // no clear at all
    d3d11_mock_clear_calls();
    sg_begin_pass(&(sg_pass){
        .attachments = { .colors[0] = cv, .depth_stencil = dv },
        .action = { .colors[0].load_action = SG_LOADACTION_LOAD, .depth.load_action = SG_LOADACTION_LOAD, .stencil.load_action = SG_LOADACTION_DONTCARE },
    });
    sg_end_pass();
    T(COUNT(CLEAR_DEPTH_STENCIL_VIEW) == 0);
    sg_commit();
    T(no_errors());
    sg_destroy_view(cv); sg_destroy_view(cv2); sg_destroy_view(dv);
    sg_destroy_image(cimg); sg_destroy_image(cimg2); sg_destroy_image(dimg);
    teardown();
}

UTEST(sokol_gfx_d3d11, offscreen_msaa_resolve_args) {
    setup();
    sg_image msaa = sg_make_image(&(sg_image_desc){ .width = 32, .height = 32, .sample_count = 4, .pixel_format = SG_PIXELFORMAT_RGBA8, .usage.color_attachment = true });
    sg_image res = sg_make_image(&(sg_image_desc){ .type = SG_IMAGETYPE_ARRAY, .width = 32, .height = 32, .num_slices = 2, .num_mipmaps = 2, .pixel_format = SG_PIXELFORMAT_RGBA8, .usage.resolve_attachment = true });
    sg_view cv = sg_make_view(&(sg_view_desc){ .color_attachment.image = msaa });
    sg_view rv = sg_make_view(&(sg_view_desc){ .resolve_attachment = { .image = res, .slice = 1 } });
    d3d11_mock_clear_calls();
    sg_begin_pass(&(sg_pass){
        .attachments = { .colors[0] = cv, .resolves[0] = rv },
        .action.colors[0] = { .load_action = SG_LOADACTION_CLEAR, .store_action = SG_STOREACTION_DONTCARE },
    });
    sg_end_pass();
    T(no_errors());
    T(COUNT(RESOLVE_SUBRESOURCE) == 1);
    const d3d11_mock_call_t* c = LAST(RESOLVE_SUBRESOURCE);
    // dst subresource = mip + slice * num_mips = 0 + 1 * 2
    T(c && (c->dst == ntex(res)) && (c->dst_subres == 2) && (c->src == ntex(msaa)) && (c->src_subres == 0));
    T(c && (c->args[0] == DXGI_FORMAT_R8G8B8A8_UNORM));
    sg_commit();
    sg_destroy_view(cv); sg_destroy_view(rv);
    sg_destroy_image(msaa); sg_destroy_image(res);
    teardown();
}

UTEST(sokol_gfx_d3d11, swapchain_pass_clear_and_resolve_args) {
    setup();
    ID3D11RenderTargetView* rtv = d3d11_mock_create_rtv(mock_dev);
    ID3D11RenderTargetView* resolve = d3d11_mock_create_rtv(mock_dev);
    ID3D11DepthStencilView* dsv = d3d11_mock_create_dsv(mock_dev);
    d3d11_mock_clear_calls();
    sg_begin_pass(&(sg_pass){
        .action.colors[0] = { .load_action = SG_LOADACTION_CLEAR, .clear_value = { 0.1f, 0.2f, 0.3f, 0.4f } },
        .swapchain = {
            .width = 64, .height = 64, .sample_count = 4,
            .color_format = SG_PIXELFORMAT_BGRA8, .depth_format = SG_PIXELFORMAT_DEPTH_STENCIL,
            .d3d11 = { .render_view = rtv, .resolve_view = resolve, .depth_stencil_view = dsv },
        },
    });
    sg_end_pass();
    sg_commit();
    T(no_errors());
    const d3d11_mock_call_t* c = LAST(OM_SET_RENDER_TARGETS);
    T(c && (c->dst == rtv) && (c->src == dsv));
    c = LAST(CLEAR_RENDER_TARGET_VIEW);
    const float exp_color[4] = { 0.1f, 0.2f, 0.3f, 0.4f };
    T(c && (c->dst == rtv) && (0 == memcmp(c->data, exp_color, sizeof(exp_color))));
    // default depth/stencil action: clear to 1.0 / 0
    c = LAST(CLEAR_DEPTH_STENCIL_VIEW);
    T(c && (c->dst == dsv) && (c->args[0] == (D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL)) && (c->args[1] == 0));
    float depth = 0.0f;
    if (c) { memcpy(&depth, c->data, sizeof(depth)); }
    T(depth == 1.0f);
    // resolve between the resources behind the swapchain views
    ID3D11Resource* rtv_res = NULL;
    ID3D11Resource* resolve_res = NULL;
    rtv->lpVtbl->GetResource(rtv, &rtv_res);
    resolve->lpVtbl->GetResource(resolve, &resolve_res);
    c = LAST(RESOLVE_SUBRESOURCE);
    T(c && (c->dst == resolve_res) && (c->dst_subres == 0) && (c->src == rtv_res) && (c->src_subres == 0));
    T(c && (c->args[0] == DXGI_FORMAT_B8G8R8A8_UNORM));
    rtv_res->lpVtbl->Release(rtv_res);
    resolve_res->lpVtbl->Release(resolve_res);
    rtv->lpVtbl->Release(rtv);
    resolve->lpVtbl->Release(resolve);
    dsv->lpVtbl->Release(dsv);
    teardown();
}

//------------------------------------------------------------------------------
//  resource bindings => *SetShaderResources / *SetSamplers / CSSetUnorderedAccessViews
//------------------------------------------------------------------------------
UTEST(sokol_gfx_d3d11, apply_bindings_render_srvs_and_samplers) {
    setup();
    fill_pattern();
    ID3D11RenderTargetView* rtv = d3d11_mock_create_rtv(mock_dev);
    sg_shader shd = sg_make_shader(&(sg_shader_desc){
        .vertex_func = { .source = "vs", .d3d11_target = "vs_5_0" },
        .fragment_func = { .source = "ps", .d3d11_target = "ps_5_0" },
        .attrs = { [0] = { .hlsl_sem_name = "POSITION" } },
        .views = {
            [0] = { .texture = { .stage = SG_SHADERSTAGE_FRAGMENT, .image_type = SG_IMAGETYPE_2D, .sample_type = SG_IMAGESAMPLETYPE_FLOAT, .hlsl_register_t_n = 3 } },
            [1] = { .storage_buffer = { .stage = SG_SHADERSTAGE_VERTEX, .readonly = true, .hlsl_register_t_n = 5 } },
        },
        .samplers = { [0] = { .stage = SG_SHADERSTAGE_FRAGMENT, .sampler_type = SG_SAMPLERTYPE_FILTERING, .hlsl_register_s_n = 2 } },
        .texture_sampler_pairs = { [0] = { .stage = SG_SHADERSTAGE_FRAGMENT, .view_slot = 0, .sampler_slot = 0 } },
    });
    T(sg_query_shader_state(shd) == SG_RESOURCESTATE_VALID);
    sg_pipeline pip = sg_make_pipeline(&(sg_pipeline_desc){
        .shader = shd,
        .layout.attrs[0].format = SG_VERTEXFORMAT_FLOAT3,
        .depth.pixel_format = SG_PIXELFORMAT_NONE,
    });
    sg_buffer vbuf = sg_make_buffer(&(sg_buffer_desc){ .data = { pattern, 36 } });
    sg_buffer sbuf = sg_make_buffer(&(sg_buffer_desc){ .usage.storage_buffer = true, .data = { pattern, 256 } });
    sg_image img = sg_make_image(&(sg_image_desc){ .width = 4, .height = 4, .pixel_format = SG_PIXELFORMAT_RGBA8, .data.mip_levels[0] = { pattern, 64 } });
    sg_view tex_view = sg_make_view(&(sg_view_desc){ .texture.image = img });
    sg_view sbuf_view = sg_make_view(&(sg_view_desc){ .storage_buffer.buffer = sbuf });
    sg_sampler smp = sg_make_sampler(&(sg_sampler_desc){ .min_filter = SG_FILTER_LINEAR, .mag_filter = SG_FILTER_LINEAR });
    d3d11_mock_clear_calls();
    begin_swapchain_pass(rtv);
    sg_apply_pipeline(pip);
    sg_apply_bindings(&(sg_bindings){
        .vertex_buffers[0] = vbuf,
        .views = { [0] = tex_view, [1] = sbuf_view },
        .samplers[0] = smp,
    });
    sg_draw(0, 3, 1);
    sg_end_pass();
    sg_commit();
    T(no_errors());
    const d3d11_mock_call_t* c = LAST(PS_SET_SHADER_RESOURCES);
    T(c && (c->args[0] == 0) && (c->args[1] == SG_MAX_VIEW_BINDSLOTS));
    T(c && (c->ptrs[3] == nsrv(tex_view)) && (c->ptrs[5] == NULL) && (c->ptrs[0] == NULL));
    c = LAST(VS_SET_SHADER_RESOURCES);
    T(c && (c->ptrs[5] == nsrv(sbuf_view)) && (c->ptrs[3] == NULL));
    c = LAST(PS_SET_SAMPLERS);
    T(c && (c->args[0] == 0) && (c->args[1] == SG_MAX_SAMPLER_BINDSLOTS) && (c->ptrs[2] == nsmp(smp)) && (c->ptrs[0] == NULL));
    c = LAST(VS_SET_SAMPLERS);
    T(c && (c->ptrs[2] == NULL));
    T(COUNT(CS_SET_UNORDERED_ACCESS_VIEWS) == 0);
    sg_destroy_view(tex_view);
    sg_destroy_view(sbuf_view);
    sg_destroy_sampler(smp);
    sg_destroy_image(img);
    sg_destroy_buffer(sbuf);
    sg_destroy_buffer(vbuf);
    sg_destroy_pipeline(pip);
    sg_destroy_shader(shd);
    rtv->lpVtbl->Release(rtv);
    teardown();
}

UTEST(sokol_gfx_d3d11, apply_bindings_compute_uavs_and_srvs) {
    setup();
    sg_shader shd = sg_make_shader(&(sg_shader_desc){
        .compute_func = { .source = "cs", .d3d11_target = "cs_5_0" },
        .views = {
            [0] = { .storage_buffer = { .stage = SG_SHADERSTAGE_COMPUTE, .readonly = false, .hlsl_register_u_n = 2 } },
            [1] = { .storage_image = { .stage = SG_SHADERSTAGE_COMPUTE, .image_type = SG_IMAGETYPE_2D, .access_format = SG_PIXELFORMAT_RGBA8, .hlsl_register_u_n = 4 } },
            [2] = { .storage_buffer = { .stage = SG_SHADERSTAGE_COMPUTE, .readonly = true, .hlsl_register_t_n = 1 } },
        },
    });
    T(sg_query_shader_state(shd) == SG_RESOURCESTATE_VALID);
    sg_pipeline pip = sg_make_pipeline(&(sg_pipeline_desc){ .shader = shd, .compute = true });
    sg_buffer rw_buf = sg_make_buffer(&(sg_buffer_desc){ .size = 256, .usage.storage_buffer = true });
    sg_buffer ro_buf = sg_make_buffer(&(sg_buffer_desc){ .size = 256, .usage.storage_buffer = true });
    sg_image simg = sg_make_image(&(sg_image_desc){ .width = 16, .height = 16, .pixel_format = SG_PIXELFORMAT_RGBA8, .usage.storage_image = true });
    sg_view rw_view = sg_make_view(&(sg_view_desc){ .storage_buffer.buffer = rw_buf });
    sg_view ro_view = sg_make_view(&(sg_view_desc){ .storage_buffer.buffer = ro_buf });
    sg_view img_view = sg_make_view(&(sg_view_desc){ .storage_image.image = simg });
    d3d11_mock_clear_calls();
    sg_begin_pass(&(sg_pass){ .compute = true });
    sg_apply_pipeline(pip);
    sg_apply_bindings(&(sg_bindings){ .views = { [0] = rw_view, [1] = img_view, [2] = ro_view } });
    sg_dispatch(4, 2, 1);
    sg_end_pass();
    sg_commit();
    T(no_errors());
    const d3d11_mock_call_t* c = LAST(CS_SET_UNORDERED_ACCESS_VIEWS);
    T(c && (c->args[0] == 0) && (c->args[1] == (UINT)sg_query_limits().d3d11_max_unordered_access_views));
    T(c && (c->ptrs[2] == nuav(rw_view)) && (c->ptrs[4] == nuav(img_view)) && (c->ptrs[0] == NULL));
    c = LAST(CS_SET_SHADER_RESOURCES);
    T(c && (c->ptrs[1] == nsrv(ro_view)) && (c->ptrs[2] == NULL));
    c = LAST(DISPATCH);
    T(c && (c->args[0] == 4) && (c->args[1] == 2) && (c->args[2] == 1));
    // render-stage setters are not touched in compute passes
    T(COUNT(PS_SET_SHADER_RESOURCES) == 0);
    T(COUNT(IA_SET_VERTEX_BUFFERS) == 0);
    sg_destroy_view(rw_view); sg_destroy_view(ro_view); sg_destroy_view(img_view);
    sg_destroy_buffer(rw_buf); sg_destroy_buffer(ro_buf); sg_destroy_image(simg);
    sg_destroy_pipeline(pip);
    sg_destroy_shader(shd);
    teardown();
}

//------------------------------------------------------------------------------
//  pipeline state => input layout / rasterizer / depth-stencil / blend descs
//------------------------------------------------------------------------------
UTEST(sokol_gfx_d3d11, pipeline_input_layout_elements) {
    setup();
    static const struct { sg_vertex_format fmt; DXGI_FORMAT dxgi; UINT size; } fmts[] = {
        { SG_VERTEXFORMAT_FLOAT, DXGI_FORMAT_R32_FLOAT, 4 },
        { SG_VERTEXFORMAT_FLOAT2, DXGI_FORMAT_R32G32_FLOAT, 8 },
        { SG_VERTEXFORMAT_FLOAT3, DXGI_FORMAT_R32G32B32_FLOAT, 12 },
        { SG_VERTEXFORMAT_FLOAT4, DXGI_FORMAT_R32G32B32A32_FLOAT, 16 },
        { SG_VERTEXFORMAT_INT, DXGI_FORMAT_R32_SINT, 4 },
        { SG_VERTEXFORMAT_INT2, DXGI_FORMAT_R32G32_SINT, 8 },
        { SG_VERTEXFORMAT_INT3, DXGI_FORMAT_R32G32B32_SINT, 12 },
        { SG_VERTEXFORMAT_INT4, DXGI_FORMAT_R32G32B32A32_SINT, 16 },
        { SG_VERTEXFORMAT_UINT, DXGI_FORMAT_R32_UINT, 4 },
        { SG_VERTEXFORMAT_UINT2, DXGI_FORMAT_R32G32_UINT, 8 },
        { SG_VERTEXFORMAT_UINT3, DXGI_FORMAT_R32G32B32_UINT, 12 },
        { SG_VERTEXFORMAT_UINT4, DXGI_FORMAT_R32G32B32A32_UINT, 16 },
        { SG_VERTEXFORMAT_BYTE4, DXGI_FORMAT_R8G8B8A8_SINT, 4 },
        { SG_VERTEXFORMAT_BYTE4N, DXGI_FORMAT_R8G8B8A8_SNORM, 4 },
        { SG_VERTEXFORMAT_UBYTE4, DXGI_FORMAT_R8G8B8A8_UINT, 4 },
        { SG_VERTEXFORMAT_UBYTE4N, DXGI_FORMAT_R8G8B8A8_UNORM, 4 },
        { SG_VERTEXFORMAT_SHORT2, DXGI_FORMAT_R16G16_SINT, 4 },
        { SG_VERTEXFORMAT_SHORT2N, DXGI_FORMAT_R16G16_SNORM, 4 },
        { SG_VERTEXFORMAT_USHORT2, DXGI_FORMAT_R16G16_UINT, 4 },
        { SG_VERTEXFORMAT_USHORT2N, DXGI_FORMAT_R16G16_UNORM, 4 },
        { SG_VERTEXFORMAT_SHORT4, DXGI_FORMAT_R16G16B16A16_SINT, 8 },
        { SG_VERTEXFORMAT_SHORT4N, DXGI_FORMAT_R16G16B16A16_SNORM, 8 },
        { SG_VERTEXFORMAT_USHORT4, DXGI_FORMAT_R16G16B16A16_UINT, 8 },
        { SG_VERTEXFORMAT_USHORT4N, DXGI_FORMAT_R16G16B16A16_UNORM, 8 },
        { SG_VERTEXFORMAT_UINT10_N2, DXGI_FORMAT_R10G10B10A2_UNORM, 4 },
        { SG_VERTEXFORMAT_HALF2, DXGI_FORMAT_R16G16_FLOAT, 4 },
        { SG_VERTEXFORMAT_HALF4, DXGI_FORMAT_R16G16B16A16_FLOAT, 8 },
    };
    const int num_fmts = (int)(sizeof(fmts) / sizeof(fmts[0]));
    sg_shader_desc shd_desc = {
        .vertex_func = { .source = "vs", .d3d11_target = "vs_5_0" },
        .fragment_func = { .source = "ps", .d3d11_target = "ps_5_0" },
    };
    for (int i = 0; i < SG_MAX_VERTEX_ATTRIBUTES; i++) {
        shd_desc.attrs[i].hlsl_sem_name = "ATTR";
        shd_desc.attrs[i].hlsl_sem_index = (uint8_t)i;
    }
    sg_shader shd = sg_make_shader(&shd_desc);
    T(sg_query_shader_state(shd) == SG_RESOURCESTATE_VALID);
    // vertex formats spread over two pipelines with 16 and 11 attributes
    for (int base = 0; base < num_fmts; base += SG_MAX_VERTEX_ATTRIBUTES) {
        int n = num_fmts - base;
        if (n > SG_MAX_VERTEX_ATTRIBUTES) { n = SG_MAX_VERTEX_ATTRIBUTES; }
        sg_pipeline_desc pip_desc = { .shader = shd };
        for (int i = 0; i < n; i++) {
            pip_desc.layout.attrs[i].format = fmts[base + i].fmt;
        }
        d3d11_mock_clear_calls();
        reset_log();
        sg_pipeline pip = sg_make_pipeline(&pip_desc);
        T(no_errors() && (sg_query_pipeline_state(pip) == SG_RESOURCESTATE_VALID));
        const d3d11_mock_call_t* c = LAST(CREATE_INPUT_LAYOUT);
        T(c && (c->dst == sg_d3d11_query_pipeline_info(pip).il) && (c->args[0] == (UINT)n));
        UINT offset = 0;
        for (int i = 0; c && (i < n); i++) {
            const D3D11_INPUT_ELEMENT_DESC* e = &c->input_elements[i];
            T(0 == strcmp(e->SemanticName, "ATTR"));
            T(e->SemanticIndex == (UINT)i);
            T(e->Format == fmts[base + i].dxgi);
            T(e->InputSlot == 0);
            T(e->AlignedByteOffset == offset);
            T(e->InputSlotClass == D3D11_INPUT_PER_VERTEX_DATA);
            T(e->InstanceDataStepRate == 0);
            offset += fmts[base + i].size;
        }
        sg_destroy_pipeline(pip);
    }
    sg_destroy_shader(shd);
    teardown();
}

UTEST(sokol_gfx_d3d11, pipeline_instancing_step_rate_and_strides) {
    setup();
    sg_shader shd = make_test_shader();
    d3d11_mock_clear_calls();
    sg_pipeline pip = sg_make_pipeline(&(sg_pipeline_desc){
        .shader = shd,
        .layout = {
            .buffers = { [0] = { .stride = 32 }, [1] = { .step_func = SG_VERTEXSTEP_PER_INSTANCE, .step_rate = 2 } },
            .attrs = {
                [0] = { .buffer_index = 0, .offset = 8, .format = SG_VERTEXFORMAT_FLOAT3 },
                [1] = { .buffer_index = 1, .format = SG_VERTEXFORMAT_UBYTE4N },
            },
        },
        .depth.pixel_format = SG_PIXELFORMAT_NONE,
    });
    T(no_errors() && (sg_query_pipeline_state(pip) == SG_RESOURCESTATE_VALID));
    const d3d11_mock_call_t* c = LAST(CREATE_INPUT_LAYOUT);
    T(c && (c->args[0] == 2));
    T(c && (0 == strcmp(c->input_elements[0].SemanticName, "POSITION")) && (c->input_elements[0].AlignedByteOffset == 8));
    T(c && (c->input_elements[0].InputSlot == 0) && (c->input_elements[0].InputSlotClass == D3D11_INPUT_PER_VERTEX_DATA));
    T(c && (0 == strcmp(c->input_elements[1].SemanticName, "COLOR")) && (c->input_elements[1].InputSlot == 1));
    T(c && (c->input_elements[1].InputSlotClass == D3D11_INPUT_PER_INSTANCE_DATA) && (c->input_elements[1].InstanceDataStepRate == 2));
    // strides are passed to IASetVertexBuffers
    ID3D11RenderTargetView* rtv = d3d11_mock_create_rtv(mock_dev);
    sg_buffer vb0 = sg_make_buffer(&(sg_buffer_desc){ .size = 128, .usage.write_transient = true });
    sg_buffer vb1 = sg_make_buffer(&(sg_buffer_desc){ .size = 64, .usage.copy_dst = true });
    sg_write_buffer_transient(&(sg_write_buffer_desc){ .src.data = { pattern, 128 }, .dst.buffer = vb0 });
    begin_swapchain_pass(rtv);
    sg_apply_pipeline(pip);
    sg_apply_bindings(&(sg_bindings){ .vertex_buffers = { vb0, vb1 } });
    sg_draw(0, 3, 2);
    sg_end_pass();
    sg_commit();
    T(no_errors());
    c = LAST(IA_SET_VERTEX_BUFFERS);
    T(c && (c->dst == nbuf(vb0)) && (c->args[2] == 32));
    T(COUNT(DRAW_INSTANCED) == 1);
    sg_destroy_buffer(vb0);
    sg_destroy_buffer(vb1);
    sg_destroy_pipeline(pip);
    sg_destroy_shader(shd);
    rtv->lpVtbl->Release(rtv);
    teardown();
}

UTEST(sokol_gfx_d3d11, pipeline_rasterizer_desc) {
    setup();
    sg_shader shd = make_test_shader();
    const sg_vertex_layout_state layout = { .attrs = { [0].format = SG_VERTEXFORMAT_FLOAT3, [1].format = SG_VERTEXFORMAT_UBYTE4N } };
    d3d11_mock_clear_calls();
    sg_pipeline pip = sg_make_pipeline(&(sg_pipeline_desc){
        .shader = shd, .layout = layout,
        .cull_mode = SG_CULLMODE_FRONT, .face_winding = SG_FACEWINDING_CCW,
        .depth = { .bias = 3.0f, .bias_slope_scale = 2.0f, .bias_clamp = 0.5f },
        .sample_count = 4,
    });
    T(no_errors() && (sg_query_pipeline_state(pip) == SG_RESOURCESTATE_VALID));
    const d3d11_mock_call_t* c = LAST(CREATE_RASTERIZER_STATE);
    T(c && (c->dst == sg_d3d11_query_pipeline_info(pip).rs));
    T(c && (c->rasterizer_desc.FillMode == D3D11_FILL_SOLID) && (c->rasterizer_desc.CullMode == D3D11_CULL_FRONT));
    T(c && (c->rasterizer_desc.FrontCounterClockwise == TRUE));
    T(c && (c->rasterizer_desc.DepthBias == 3) && (c->rasterizer_desc.SlopeScaledDepthBias == 2.0f) && (c->rasterizer_desc.DepthBiasClamp == 0.5f));
    T(c && (c->rasterizer_desc.DepthClipEnable == TRUE) && (c->rasterizer_desc.ScissorEnable == TRUE) && (c->rasterizer_desc.MultisampleEnable == TRUE));
    sg_destroy_pipeline(pip);
    pip = sg_make_pipeline(&(sg_pipeline_desc){ .shader = shd, .layout = layout, .cull_mode = SG_CULLMODE_BACK, .face_winding = SG_FACEWINDING_CW, .sample_count = 1 });
    c = LAST(CREATE_RASTERIZER_STATE);
    T(c && (c->rasterizer_desc.CullMode == D3D11_CULL_BACK) && (c->rasterizer_desc.FrontCounterClockwise == FALSE) && (c->rasterizer_desc.MultisampleEnable == FALSE));
    sg_destroy_pipeline(pip);
    pip = sg_make_pipeline(&(sg_pipeline_desc){ .shader = shd, .layout = layout });
    c = LAST(CREATE_RASTERIZER_STATE);
    T(c && (c->rasterizer_desc.CullMode == D3D11_CULL_NONE) && (c->rasterizer_desc.DepthBias == 0));
    sg_destroy_pipeline(pip);
    sg_destroy_shader(shd);
    teardown();
}

UTEST(sokol_gfx_d3d11, pipeline_depth_stencil_desc) {
    setup();
    sg_shader shd = make_test_shader();
    const sg_vertex_layout_state layout = { .attrs = { [0].format = SG_VERTEXFORMAT_FLOAT3, [1].format = SG_VERTEXFORMAT_UBYTE4N } };
    static const struct { sg_compare_func sg; D3D11_COMPARISON_FUNC d3d; } cmp[] = {
        { SG_COMPAREFUNC_NEVER, D3D11_COMPARISON_NEVER },
        { SG_COMPAREFUNC_LESS, D3D11_COMPARISON_LESS },
        { SG_COMPAREFUNC_EQUAL, D3D11_COMPARISON_EQUAL },
        { SG_COMPAREFUNC_LESS_EQUAL, D3D11_COMPARISON_LESS_EQUAL },
        { SG_COMPAREFUNC_GREATER, D3D11_COMPARISON_GREATER },
        { SG_COMPAREFUNC_NOT_EQUAL, D3D11_COMPARISON_NOT_EQUAL },
        { SG_COMPAREFUNC_GREATER_EQUAL, D3D11_COMPARISON_GREATER_EQUAL },
        { SG_COMPAREFUNC_ALWAYS, D3D11_COMPARISON_ALWAYS },
    };
    static const struct { sg_stencil_op sg; D3D11_STENCIL_OP d3d; } ops[] = {
        { SG_STENCILOP_KEEP, D3D11_STENCIL_OP_KEEP },
        { SG_STENCILOP_ZERO, D3D11_STENCIL_OP_ZERO },
        { SG_STENCILOP_REPLACE, D3D11_STENCIL_OP_REPLACE },
        { SG_STENCILOP_INCR_CLAMP, D3D11_STENCIL_OP_INCR_SAT },
        { SG_STENCILOP_DECR_CLAMP, D3D11_STENCIL_OP_DECR_SAT },
        { SG_STENCILOP_INVERT, D3D11_STENCIL_OP_INVERT },
        { SG_STENCILOP_INCR_WRAP, D3D11_STENCIL_OP_INCR },
        { SG_STENCILOP_DECR_WRAP, D3D11_STENCIL_OP_DECR },
    };
    for (int i = 0; i < 8; i++) {
        const int j = (i + 3) % 8;
        d3d11_mock_clear_calls();
        reset_log();
        sg_pipeline pip = sg_make_pipeline(&(sg_pipeline_desc){
            .shader = shd, .layout = layout,
            .depth = { .compare = cmp[i].sg, .write_enabled = (i & 1) != 0 },
            .stencil = {
                .enabled = true, .read_mask = 0xF0, .write_mask = 0x0F, .ref = 1,
                .front = { .compare = cmp[i].sg, .fail_op = ops[i].sg, .depth_fail_op = ops[(i + 1) % 8].sg, .pass_op = ops[(i + 2) % 8].sg },
                .back = { .compare = cmp[j].sg, .fail_op = ops[j].sg, .depth_fail_op = ops[(j + 1) % 8].sg, .pass_op = ops[(j + 2) % 8].sg },
            },
        });
        T(no_errors() && (sg_query_pipeline_state(pip) == SG_RESOURCESTATE_VALID));
        const d3d11_mock_call_t* c = LAST(CREATE_DEPTH_STENCIL_STATE);
        T(c && (c->dst == sg_d3d11_query_pipeline_info(pip).dss));
        if (c) {
            const D3D11_DEPTH_STENCIL_DESC* d = &c->depth_stencil_desc;
            T(d->DepthEnable == TRUE);
            T(d->DepthWriteMask == ((i & 1) ? D3D11_DEPTH_WRITE_MASK_ALL : D3D11_DEPTH_WRITE_MASK_ZERO));
            T(d->DepthFunc == cmp[i].d3d);
            T(d->StencilEnable == TRUE);
            T((d->StencilReadMask == 0xF0) && (d->StencilWriteMask == 0x0F));
            T(d->FrontFace.StencilFunc == cmp[i].d3d);
            T(d->FrontFace.StencilFailOp == ops[i].d3d);
            T(d->FrontFace.StencilDepthFailOp == ops[(i + 1) % 8].d3d);
            T(d->FrontFace.StencilPassOp == ops[(i + 2) % 8].d3d);
            T(d->BackFace.StencilFunc == cmp[j].d3d);
            T(d->BackFace.StencilFailOp == ops[j].d3d);
            T(d->BackFace.StencilDepthFailOp == ops[(j + 1) % 8].d3d);
            T(d->BackFace.StencilPassOp == ops[(j + 2) % 8].d3d);
        }
        sg_destroy_pipeline(pip);
    }
    sg_destroy_shader(shd);
    teardown();
}

UTEST(sokol_gfx_d3d11, pipeline_blend_desc) {
    setup();
    sg_shader shd = make_test_shader();
    d3d11_mock_clear_calls();
    sg_pipeline pip = sg_make_pipeline(&(sg_pipeline_desc){
        .shader = shd,
        .layout = { .attrs = { [0].format = SG_VERTEXFORMAT_FLOAT3, [1].format = SG_VERTEXFORMAT_UBYTE4N } },
        .alpha_to_coverage_enabled = true,
        .color_count = 4,
        .colors = {
            [0] = { .write_mask = SG_COLORMASK_RGB, .blend = {
                .enabled = true,
                .src_factor_rgb = SG_BLENDFACTOR_SRC_ALPHA, .dst_factor_rgb = SG_BLENDFACTOR_ONE_MINUS_SRC_ALPHA, .op_rgb = SG_BLENDOP_ADD,
                .src_factor_alpha = SG_BLENDFACTOR_ONE, .dst_factor_alpha = SG_BLENDFACTOR_ZERO, .op_alpha = SG_BLENDOP_SUBTRACT } },
            [1] = { .blend = {
                .enabled = true,
                .src_factor_rgb = SG_BLENDFACTOR_DST_COLOR, .dst_factor_rgb = SG_BLENDFACTOR_ONE_MINUS_DST_COLOR, .op_rgb = SG_BLENDOP_REVERSE_SUBTRACT,
                .src_factor_alpha = SG_BLENDFACTOR_SRC_ALPHA_SATURATED, .dst_factor_alpha = SG_BLENDFACTOR_BLEND_COLOR, .op_alpha = SG_BLENDOP_ADD } },
            [2] = { .write_mask = SG_COLORMASK_RA, .blend = {
                .enabled = true,
                .src_factor_rgb = SG_BLENDFACTOR_BLEND_ALPHA, .dst_factor_rgb = SG_BLENDFACTOR_ONE_MINUS_BLEND_COLOR, .op_rgb = SG_BLENDOP_ADD,
                .src_factor_alpha = SG_BLENDFACTOR_DST_ALPHA, .dst_factor_alpha = SG_BLENDFACTOR_ONE_MINUS_DST_ALPHA, .op_alpha = SG_BLENDOP_ADD } },
            [3] = { .write_mask = SG_COLORMASK_NONE, .blend = {
                .enabled = true,
                .src_factor_rgb = SG_BLENDFACTOR_SRC_COLOR, .dst_factor_rgb = SG_BLENDFACTOR_ONE_MINUS_SRC_COLOR, .op_rgb = SG_BLENDOP_ADD,
                .src_factor_alpha = SG_BLENDFACTOR_ONE_MINUS_BLEND_ALPHA, .dst_factor_alpha = SG_BLENDFACTOR_ONE_MINUS_SRC_ALPHA, .op_alpha = SG_BLENDOP_ADD } },
        },
    });
    T(no_errors() && (sg_query_pipeline_state(pip) == SG_RESOURCESTATE_VALID));
    const d3d11_mock_call_t* c = LAST(CREATE_BLEND_STATE);
    T(c && (c->dst == sg_d3d11_query_pipeline_info(pip).bs));
    if (c) {
        const D3D11_BLEND_DESC* b = &c->blend_desc;
        T(b->AlphaToCoverageEnable == TRUE);
        T(b->IndependentBlendEnable == TRUE);
        const D3D11_RENDER_TARGET_BLEND_DESC* rt = &b->RenderTarget[0];
        T(rt->BlendEnable == TRUE);
        T((rt->SrcBlend == D3D11_BLEND_SRC_ALPHA) && (rt->DestBlend == D3D11_BLEND_INV_SRC_ALPHA) && (rt->BlendOp == D3D11_BLEND_OP_ADD));
        T((rt->SrcBlendAlpha == D3D11_BLEND_ONE) && (rt->DestBlendAlpha == D3D11_BLEND_ZERO) && (rt->BlendOpAlpha == D3D11_BLEND_OP_SUBTRACT));
        T(rt->RenderTargetWriteMask == (D3D11_COLOR_WRITE_ENABLE_RED | D3D11_COLOR_WRITE_ENABLE_GREEN | D3D11_COLOR_WRITE_ENABLE_BLUE));
        rt = &b->RenderTarget[1];
        T((rt->SrcBlend == D3D11_BLEND_DEST_COLOR) && (rt->DestBlend == D3D11_BLEND_INV_DEST_COLOR) && (rt->BlendOp == D3D11_BLEND_OP_REV_SUBTRACT));
        T((rt->SrcBlendAlpha == D3D11_BLEND_SRC_ALPHA_SAT) && (rt->DestBlendAlpha == D3D11_BLEND_BLEND_FACTOR));
        T(rt->RenderTargetWriteMask == D3D11_COLOR_WRITE_ENABLE_ALL);
        rt = &b->RenderTarget[2];
        T((rt->SrcBlend == D3D11_BLEND_BLEND_FACTOR) && (rt->DestBlend == D3D11_BLEND_INV_BLEND_FACTOR));
        T((rt->SrcBlendAlpha == D3D11_BLEND_DEST_ALPHA) && (rt->DestBlendAlpha == D3D11_BLEND_INV_DEST_ALPHA));
        T(rt->RenderTargetWriteMask == (D3D11_COLOR_WRITE_ENABLE_RED | D3D11_COLOR_WRITE_ENABLE_ALPHA));
        rt = &b->RenderTarget[3];
        T((rt->SrcBlend == D3D11_BLEND_SRC_COLOR) && (rt->DestBlend == D3D11_BLEND_INV_SRC_COLOR));
        T((rt->SrcBlendAlpha == D3D11_BLEND_INV_BLEND_FACTOR) && (rt->DestBlendAlpha == D3D11_BLEND_INV_SRC_ALPHA));
        T(rt->RenderTargetWriteMask == 0);
        // unused color targets get default state
        for (int i = 4; i < 8; i++) {
            rt = &b->RenderTarget[i];
            T(rt->BlendEnable == FALSE);
            T((rt->SrcBlend == D3D11_BLEND_ONE) && (rt->DestBlend == D3D11_BLEND_ZERO) && (rt->BlendOp == D3D11_BLEND_OP_ADD));
            T((rt->SrcBlendAlpha == D3D11_BLEND_ONE) && (rt->DestBlendAlpha == D3D11_BLEND_ZERO) && (rt->BlendOpAlpha == D3D11_BLEND_OP_ADD));
            T(rt->RenderTargetWriteMask == D3D11_COLOR_WRITE_ENABLE_ALL);
        }
    }
    sg_destroy_pipeline(pip);
    // min/max blend ops
    pip = sg_make_pipeline(&(sg_pipeline_desc){
        .shader = shd,
        .layout = { .attrs = { [0].format = SG_VERTEXFORMAT_FLOAT3, [1].format = SG_VERTEXFORMAT_UBYTE4N } },
        .colors[0].blend = { .enabled = true, .op_rgb = SG_BLENDOP_MIN, .op_alpha = SG_BLENDOP_MAX },
    });
    c = LAST(CREATE_BLEND_STATE);
    T(c && (c->blend_desc.AlphaToCoverageEnable == FALSE));
    T(c && (c->blend_desc.RenderTarget[0].BlendOp == D3D11_BLEND_OP_MIN) && (c->blend_desc.RenderTarget[0].BlendOpAlpha == D3D11_BLEND_OP_MAX));
    sg_destroy_pipeline(pip);
    sg_destroy_shader(shd);
    teardown();
}

UTEST(sokol_gfx_d3d11, pipeline_state_object_create_failures) {
    setup();
    sg_shader shd = make_test_shader();
    const sg_pipeline_desc desc = {
        .shader = shd,
        .layout = { .attrs = { [0].format = SG_VERTEXFORMAT_FLOAT3, [1].format = SG_VERTEXFORMAT_UBYTE4N } },
    };
    const int before = d3d11_mock_live_object_count();
    static const sg_log_item items[3] = {
        SG_LOGITEM_D3D11_CREATE_RASTERIZER_STATE_FAILED,
        SG_LOGITEM_D3D11_CREATE_DEPTH_STENCIL_STATE_FAILED,
        SG_LOGITEM_D3D11_CREATE_BLEND_STATE_FAILED,
    };
    for (int i = 0; i < 3; i++) {
        reset_log();
        d3d11_mock_fail_create_after(i + 1, 1);
        sg_pipeline pip = sg_make_pipeline(&desc);
        T(sg_query_pipeline_state(pip) == SG_RESOURCESTATE_FAILED);
        T(logged(items[i]));
        sg_destroy_pipeline(pip);
        T(d3d11_mock_live_object_count() == before);
    }
    sg_destroy_shader(shd);
    teardown();
}

//------------------------------------------------------------------------------
//  sampler desc mapping
//------------------------------------------------------------------------------
UTEST(sokol_gfx_d3d11, sampler_desc_mapping) {
    setup();
    d3d11_mock_clear_calls();
    sg_sampler s = sg_make_sampler(&(sg_sampler_desc){
        .min_filter = SG_FILTER_LINEAR, .mag_filter = SG_FILTER_NEAREST, .mipmap_filter = SG_FILTER_LINEAR,
        .wrap_u = SG_WRAP_REPEAT, .wrap_v = SG_WRAP_CLAMP_TO_EDGE, .wrap_w = SG_WRAP_MIRRORED_REPEAT,
        .min_lod = 1.0f, .max_lod = 5.0f,
    });
    T(no_errors());
    const d3d11_mock_call_t* c = LAST(CREATE_SAMPLER_STATE);
    T(c && (c->dst == nsmp(s)));
    T(c && (c->sampler_desc.Filter == D3D11_FILTER_MIN_LINEAR_MAG_POINT_MIP_LINEAR));
    T(c && (c->sampler_desc.AddressU == D3D11_TEXTURE_ADDRESS_WRAP) && (c->sampler_desc.AddressV == D3D11_TEXTURE_ADDRESS_CLAMP) && (c->sampler_desc.AddressW == D3D11_TEXTURE_ADDRESS_MIRROR));
    T(c && (c->sampler_desc.MinLOD == 1.0f) && (c->sampler_desc.MaxLOD == 5.0f));
    T(c && (c->sampler_desc.ComparisonFunc == D3D11_COMPARISON_NEVER) && (c->sampler_desc.MaxAnisotropy == 1));
    sg_destroy_sampler(s);
    // point mag-linear, comparison sampler, border colors
    s = sg_make_sampler(&(sg_sampler_desc){
        .min_filter = SG_FILTER_NEAREST, .mag_filter = SG_FILTER_LINEAR, .mipmap_filter = SG_FILTER_NEAREST,
        .wrap_u = SG_WRAP_CLAMP_TO_BORDER, .border_color = SG_BORDERCOLOR_OPAQUE_WHITE,
        .compare = SG_COMPAREFUNC_LESS_EQUAL,
    });
    c = LAST(CREATE_SAMPLER_STATE);
    T(c && (c->sampler_desc.Filter == D3D11_FILTER_COMPARISON_MIN_POINT_MAG_LINEAR_MIP_POINT));
    T(c && (c->sampler_desc.AddressU == D3D11_TEXTURE_ADDRESS_BORDER) && (c->sampler_desc.ComparisonFunc == D3D11_COMPARISON_LESS_EQUAL));
    T(c && (c->sampler_desc.BorderColor[0] == 1.0f) && (c->sampler_desc.BorderColor[1] == 1.0f) && (c->sampler_desc.BorderColor[2] == 1.0f) && (c->sampler_desc.BorderColor[3] == 1.0f));
    sg_destroy_sampler(s);
    s = sg_make_sampler(&(sg_sampler_desc){ .wrap_u = SG_WRAP_CLAMP_TO_BORDER, .border_color = SG_BORDERCOLOR_TRANSPARENT_BLACK });
    c = LAST(CREATE_SAMPLER_STATE);
    T(c && (c->sampler_desc.Filter == D3D11_FILTER_MIN_MAG_MIP_POINT));
    T(c && (c->sampler_desc.BorderColor[0] == 0.0f) && (c->sampler_desc.BorderColor[3] == 0.0f));
    sg_destroy_sampler(s);
    s = sg_make_sampler(&(sg_sampler_desc){ .wrap_u = SG_WRAP_CLAMP_TO_BORDER, .border_color = SG_BORDERCOLOR_OPAQUE_BLACK });
    c = LAST(CREATE_SAMPLER_STATE);
    T(c && (c->sampler_desc.BorderColor[0] == 0.0f) && (c->sampler_desc.BorderColor[3] == 1.0f));
    sg_destroy_sampler(s);
    // anisotropic (with and without compare)
    s = sg_make_sampler(&(sg_sampler_desc){
        .min_filter = SG_FILTER_LINEAR, .mag_filter = SG_FILTER_LINEAR, .mipmap_filter = SG_FILTER_LINEAR, .max_anisotropy = 8,
    });
    c = LAST(CREATE_SAMPLER_STATE);
    T(c && (c->sampler_desc.Filter == D3D11_FILTER_ANISOTROPIC) && (c->sampler_desc.MaxAnisotropy == 8));
    sg_destroy_sampler(s);
    s = sg_make_sampler(&(sg_sampler_desc){
        .min_filter = SG_FILTER_LINEAR, .mag_filter = SG_FILTER_LINEAR, .mipmap_filter = SG_FILTER_LINEAR, .max_anisotropy = 4,
        .compare = SG_COMPAREFUNC_GREATER,
    });
    c = LAST(CREATE_SAMPLER_STATE);
    T(c && (c->sampler_desc.Filter == D3D11_FILTER_COMPARISON_ANISOTROPIC) && (c->sampler_desc.ComparisonFunc == D3D11_COMPARISON_GREATER));
    sg_destroy_sampler(s);
    T(no_errors());
    teardown();
}

//------------------------------------------------------------------------------
//  shader creation errors: HLSL register ranges, constant buffer creation
//------------------------------------------------------------------------------
UTEST(sokol_gfx_d3d11, shader_hlsl_register_out_of_range) {
    setup();
    const sg_shader_function vs = { .source = "vs", .d3d11_target = "vs_5_0" };
    const sg_shader_function fs = { .source = "ps", .d3d11_target = "ps_5_0" };
    const sg_shader_function cs = { .source = "cs", .d3d11_target = "cs_5_0" };
    sg_shader shd;

    reset_log();
    shd = sg_make_shader(&(sg_shader_desc){ .vertex_func = vs, .fragment_func = fs,
        .uniform_blocks[0] = { .stage = SG_SHADERSTAGE_VERTEX, .size = 16, .hlsl_register_b_n = 8 } });
    T((sg_query_shader_state(shd) == SG_RESOURCESTATE_FAILED) && logged(SG_LOGITEM_D3D11_UNIFORMBLOCK_HLSL_REGISTER_B_OUT_OF_RANGE));
    sg_destroy_shader(shd);

    reset_log();
    shd = sg_make_shader(&(sg_shader_desc){ .vertex_func = vs, .fragment_func = fs,
        .views[0].texture = { .stage = SG_SHADERSTAGE_FRAGMENT, .image_type = SG_IMAGETYPE_2D, .sample_type = SG_IMAGESAMPLETYPE_FLOAT, .hlsl_register_t_n = 32 },
        .samplers[0] = { .stage = SG_SHADERSTAGE_FRAGMENT, .sampler_type = SG_SAMPLERTYPE_FILTERING },
        .texture_sampler_pairs[0] = { .stage = SG_SHADERSTAGE_FRAGMENT, .view_slot = 0, .sampler_slot = 0 } });
    T((sg_query_shader_state(shd) == SG_RESOURCESTATE_FAILED) && logged(SG_LOGITEM_D3D11_IMAGE_HLSL_REGISTER_T_OUT_OF_RANGE));
    sg_destroy_shader(shd);

    reset_log();
    shd = sg_make_shader(&(sg_shader_desc){ .compute_func = cs,
        .views[0].storage_buffer = { .stage = SG_SHADERSTAGE_COMPUTE, .readonly = true, .hlsl_register_t_n = 32 } });
    T((sg_query_shader_state(shd) == SG_RESOURCESTATE_FAILED) && logged(SG_LOGITEM_D3D11_STORAGEBUFFER_HLSL_REGISTER_T_OUT_OF_RANGE));
    sg_destroy_shader(shd);

    reset_log();
    shd = sg_make_shader(&(sg_shader_desc){ .compute_func = cs,
        .views[0].storage_buffer = { .stage = SG_SHADERSTAGE_COMPUTE, .hlsl_register_u_n = 32 } });
    T((sg_query_shader_state(shd) == SG_RESOURCESTATE_FAILED) && logged(SG_LOGITEM_D3D11_STORAGEBUFFER_HLSL_REGISTER_U_OUT_OF_RANGE));
    sg_destroy_shader(shd);

    reset_log();
    shd = sg_make_shader(&(sg_shader_desc){ .compute_func = cs,
        .views[0].storage_image = { .stage = SG_SHADERSTAGE_COMPUTE, .image_type = SG_IMAGETYPE_2D, .access_format = SG_PIXELFORMAT_RGBA8, .hlsl_register_u_n = 32 } });
    T((sg_query_shader_state(shd) == SG_RESOURCESTATE_FAILED) && logged(SG_LOGITEM_D3D11_STORAGEIMAGE_HLSL_REGISTER_U_OUT_OF_RANGE));
    sg_destroy_shader(shd);

    reset_log();
    shd = sg_make_shader(&(sg_shader_desc){ .vertex_func = vs, .fragment_func = fs,
        .views[0].texture = { .stage = SG_SHADERSTAGE_FRAGMENT, .image_type = SG_IMAGETYPE_2D, .sample_type = SG_IMAGESAMPLETYPE_FLOAT },
        .samplers[0] = { .stage = SG_SHADERSTAGE_FRAGMENT, .sampler_type = SG_SAMPLERTYPE_FILTERING, .hlsl_register_s_n = 12 },
        .texture_sampler_pairs[0] = { .stage = SG_SHADERSTAGE_FRAGMENT, .view_slot = 0, .sampler_slot = 0 } });
    T((sg_query_shader_state(shd) == SG_RESOURCESTATE_FAILED) && logged(SG_LOGITEM_D3D11_SAMPLER_HLSL_REGISTER_S_OUT_OF_RANGE));
    sg_destroy_shader(shd);
    teardown();
}

UTEST(sokol_gfx_d3d11, shader_constant_buffer_create_fail) {
    setup();
    const int before = d3d11_mock_live_object_count();
    d3d11_mock_fail_next_create(1);
    sg_shader shd = sg_make_shader(&(sg_shader_desc){
        .vertex_func = { .source = "vs", .d3d11_target = "vs_5_0" },
        .fragment_func = { .source = "ps", .d3d11_target = "ps_5_0" },
        .uniform_blocks[0] = { .stage = SG_SHADERSTAGE_VERTEX, .size = 64 },
    });
    T(sg_query_shader_state(shd) == SG_RESOURCESTATE_FAILED);
    T(logged(SG_LOGITEM_D3D11_CREATE_CONSTANT_BUFFER_FAILED));
    sg_destroy_shader(shd);
    T(d3d11_mock_live_object_count() == before);
    teardown();
}

UTEST_MAIN()
