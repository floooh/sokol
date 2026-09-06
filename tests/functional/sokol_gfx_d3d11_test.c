//------------------------------------------------------------------------------
//  LLM maintained.
//
//  sokol_gfx_d3d11_test.c
//  Smoke test that exercises the sokol_gfx.h D3D11 backend against the mocked
//  d3d11.h/d3dcompiler.h library in tests/mocks/. Only runs on non-Windows
//  hosts; on Windows the real SDK is used and this executable is not built.
//------------------------------------------------------------------------------
#define SOKOL_D3D11
#define SOKOL_IMPL
#include "d3d11_mock.h"
#include "sokol_gfx.h"
#include "utest.h"
#include <stdio.h>

#define T(b) EXPECT_TRUE(b)

static ID3D11Device* mock_dev;

static void log_stderr(const char* tag, uint32_t log_level, uint32_t log_item_id, const char* message_or_null, uint32_t line_nr, const char* filename_or_null, void* user_data) {
    (void)tag; (void)log_level; (void)log_item_id; (void)line_nr; (void)filename_or_null; (void)user_data;
    if (message_or_null) { fprintf(stderr, "[sokol] %s\n", message_or_null); }
}

static void setup(void) {
    mock_dev = d3d11_mock_create_device();
    sg_setup(&(sg_desc){
        .environment.d3d11.device = mock_dev,
        .environment.d3d11.device_context = d3d11_mock_get_device_context(mock_dev),
        .logger.func = log_stderr,
    });
}

static void teardown(void) {
    sg_shutdown();
    d3d11_mock_destroy_device(mock_dev);
    mock_dev = NULL;
}

UTEST(sokol_gfx_d3d11, setup_shutdown) {
    setup();
    T(sg_isvalid());
    T(sg_query_backend() == SG_BACKEND_D3D11);
    teardown();
    T(!sg_isvalid());
}

UTEST(sokol_gfx_d3d11, make_destroy_buffer) {
    setup();
    static const float verts[] = { 0, 0, 0, 1, 0, 0, 0, 1, 0 };
    sg_buffer buf = sg_make_buffer(&(sg_buffer_desc){ .data = SG_RANGE(verts) });
    T(sg_query_buffer_state(buf) == SG_RESOURCESTATE_VALID);
    sg_destroy_buffer(buf);
    T(sg_query_buffer_state(buf) == SG_RESOURCESTATE_INVALID);
    teardown();
}

UTEST(sokol_gfx_d3d11, make_destroy_image) {
    setup();
    static uint8_t pixels[4 * 4 * 4] = {0};
    sg_image img = sg_make_image(&(sg_image_desc){
        .width = 4,
        .height = 4,
        .pixel_format = SG_PIXELFORMAT_RGBA8,
        .data.mip_levels[0] = SG_RANGE(pixels),
    });
    T(sg_query_image_state(img) == SG_RESOURCESTATE_VALID);
    sg_destroy_image(img);
    T(sg_query_image_state(img) == SG_RESOURCESTATE_INVALID);
    teardown();
}

UTEST(sokol_gfx_d3d11, make_destroy_sampler) {
    setup();
    sg_sampler smp = sg_make_sampler(&(sg_sampler_desc){0});
    T(sg_query_sampler_state(smp) == SG_RESOURCESTATE_VALID);
    sg_destroy_sampler(smp);
    T(sg_query_sampler_state(smp) == SG_RESOURCESTATE_INVALID);
    teardown();
}

UTEST(sokol_gfx_d3d11, make_shader_and_pipeline) {
    setup();
    sg_shader shd = sg_make_shader(&(sg_shader_desc){
        .vertex_func   = { .source = "vs_source_placeholder", .d3d11_target = "vs_5_0" },
        .fragment_func = { .source = "fs_source_placeholder", .d3d11_target = "ps_5_0" },
        .attrs = { [0] = { .hlsl_sem_name = "POSITION" } },
    });
    T(sg_query_shader_state(shd) == SG_RESOURCESTATE_VALID);
    sg_pipeline pip = sg_make_pipeline(&(sg_pipeline_desc){
        .shader = shd,
        .layout = { .attrs = { [0] = { .format = SG_VERTEXFORMAT_FLOAT3 } } },
    });
    T(sg_query_pipeline_state(pip) == SG_RESOURCESTATE_VALID);
    sg_destroy_pipeline(pip);
    sg_destroy_shader(shd);
    teardown();
}

UTEST(sokol_gfx_d3d11, swapchain_pass) {
    setup();
    (void)utest_result;
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
    sg_end_pass();
    sg_commit();
    ((ID3D11RenderTargetView*)rtv)->lpVtbl->Release(rtv);
    ((ID3D11DepthStencilView*)dsv)->lpVtbl->Release(dsv);
    teardown();
}

UTEST(sokol_gfx_d3d11, no_leaks_after_shutdown) {
    setup();
    /* Create then destroy a bunch of resources, expect only the device+ctx
       to remain live after teardown. */
    sg_buffer buf = sg_make_buffer(&(sg_buffer_desc){
        .size = 64, .usage.dynamic_update = true,
    });
    sg_image img = sg_make_image(&(sg_image_desc){
        .width = 8, .height = 8, .pixel_format = SG_PIXELFORMAT_RGBA8,
        .usage.color_attachment = true,
    });
    sg_destroy_buffer(buf);
    sg_destroy_image(img);
    teardown();
    T(d3d11_mock_live_object_count() == 0);
}

UTEST_MAIN()
