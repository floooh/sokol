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
#include <string.h>

#define T(b) EXPECT_TRUE(b)

//------------------------------------------------------------------------------
//  test harness
//------------------------------------------------------------------------------
#define MAX_LOG_ITEMS (64)
static sg_log_item log_items[MAX_LOG_ITEMS];
static int num_log_items;

static void reset_log(void) {
    num_log_items = 0;
    memset(log_items, 0, sizeof(log_items));
}

static void capture_log(const char* tag, uint32_t log_level, uint32_t log_item_id, const char* msg, uint32_t line_nr, const char* file, void* ud) {
    (void)tag; (void)log_level; (void)msg; (void)line_nr; (void)file; (void)ud;
    if (num_log_items < MAX_LOG_ITEMS) {
        log_items[num_log_items++] = (sg_log_item)log_item_id;
    }
}

static bool logged(sg_log_item item) {
    for (int i = 0; i < num_log_items; i++) {
        if (log_items[i] == item) { return true; }
    }
    return false;
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
    sg_buffer dyn = sg_make_buffer(&(sg_buffer_desc){ .size = 256, .usage.dynamic_update = true });
    sg_buffer stm = sg_make_buffer(&(sg_buffer_desc){ .size = 256, .usage.write_unsealed = true, .usage.immutable = true });
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
        .usage.immutable = true,
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
    sg_buffer buf = sg_make_buffer(&(sg_buffer_desc){ .size = 128, .usage.dynamic_update = true });
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
        .usage.dynamic_update = true,
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
        .usage.dynamic_update = true,
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
        .usage.dynamic_update = true,
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
    });
    static const float verts[24] = {0};
    static const uint16_t indices[6] = {0, 1, 2, 0, 2, 3};
    sg_buffer vbuf = sg_make_buffer(&(sg_buffer_desc){ .data = SG_RANGE(verts) });
    sg_buffer ibuf = sg_make_buffer(&(sg_buffer_desc){ .usage.index_buffer = true, .data = SG_RANGE(indices) });

    sg_begin_pass(&(sg_pass){
        .swapchain = { .width = 64, .height = 64, .sample_count = 1,
                       .color_format = SG_PIXELFORMAT_BGRA8, .depth_format = SG_PIXELFORMAT_NONE,
                       .d3d11 = { .render_view = rtv } },
    });
    sg_apply_pipeline(pip);
    sg_apply_bindings(&(sg_bindings){
        .vertex_buffers[0] = vbuf,
        .index_buffer = ibuf,
    });
    sg_draw(0, 6, 1);              // exercises DrawIndexed
    sg_draw(0, 6, 4);              // DrawIndexedInstanced
    sg_end_pass();
    sg_commit();

    sg_destroy_buffer(vbuf);
    sg_destroy_buffer(ibuf);
    sg_destroy_pipeline(pip);
    sg_destroy_shader(shd);
    rtv->lpVtbl->Release(rtv);
    T(sg_isvalid());
    teardown();
}

UTEST(sokol_gfx_d3d11, draw_non_indexed_and_instanced) {
    setup();
    ID3D11RenderTargetView* rtv = d3d11_mock_create_rtv(mock_dev);
    sg_shader shd = make_test_shader();
    sg_pipeline pip = sg_make_pipeline(&(sg_pipeline_desc){
        .shader = shd,
        .layout = { .attrs = { [0] = { .format = SG_VERTEXFORMAT_FLOAT3 }, [1] = { .format = SG_VERTEXFORMAT_UBYTE4N } } },
    });
    static const float verts[24] = {0};
    sg_buffer vbuf = sg_make_buffer(&(sg_buffer_desc){ .data = SG_RANGE(verts) });
    sg_begin_pass(&(sg_pass){
        .swapchain = { .width = 64, .height = 64, .sample_count = 1,
                       .color_format = SG_PIXELFORMAT_BGRA8, .depth_format = SG_PIXELFORMAT_NONE,
                       .d3d11 = { .render_view = rtv } },
    });
    sg_apply_pipeline(pip);
    sg_apply_bindings(&(sg_bindings){ .vertex_buffers[0] = vbuf });
    sg_draw(0, 3, 1);              // Draw
    sg_draw(0, 3, 8);              // DrawInstanced
    sg_end_pass();
    sg_commit();
    sg_destroy_buffer(vbuf);
    sg_destroy_pipeline(pip);
    sg_destroy_shader(shd);
    rtv->lpVtbl->Release(rtv);
    T(sg_isvalid());
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
        },
    });
    sg_pipeline pip = sg_make_pipeline(&(sg_pipeline_desc){
        .shader = shd,
        .layout = { .attrs = { [0] = { .format = SG_VERTEXFORMAT_FLOAT3 } } },
    });
    static const float verts[3] = {0};
    sg_buffer vbuf = sg_make_buffer(&(sg_buffer_desc){ .data = SG_RANGE(verts) });
    sg_begin_pass(&(sg_pass){
        .swapchain = { .width = 32, .height = 32, .sample_count = 1,
                       .color_format = SG_PIXELFORMAT_BGRA8, .depth_format = SG_PIXELFORMAT_NONE,
                       .d3d11 = { .render_view = rtv } },
    });
    sg_apply_pipeline(pip);
    sg_apply_bindings(&(sg_bindings){ .vertex_buffers[0] = vbuf });
    float u[16] = {0};
    sg_apply_uniforms(0, &SG_RANGE(u));    // exercises UpdateSubresource on cbuffer
    sg_draw(0, 3, 1);
    sg_end_pass();
    sg_commit();
    sg_destroy_buffer(vbuf);
    sg_destroy_pipeline(pip);
    sg_destroy_shader(shd);
    rtv->lpVtbl->Release(rtv);
    T(sg_isvalid());
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
UTEST(sokol_gfx_d3d11, buffer_update_via_map) {
    setup();
    sg_buffer buf = sg_make_buffer(&(sg_buffer_desc){ .size = 64, .usage.dynamic_update = true });
    T(sg_query_buffer_state(buf) == SG_RESOURCESTATE_VALID);
    uint8_t bytes[64] = {0};
    sg_update_buffer(buf, &SG_RANGE(bytes));   // Map(WRITE_DISCARD) + Unmap
    sg_destroy_buffer(buf);
    teardown();
}

UTEST(sokol_gfx_d3d11, buffer_append_via_map_noverwrite) {
    setup();
    sg_buffer buf = sg_make_buffer(&(sg_buffer_desc){ .size = 128, .usage.dynamic_update = true });
    uint8_t bytes[32] = {0};
    int off1 = sg_append_buffer(buf, &SG_RANGE(bytes));   // WRITE_DISCARD (first-in-frame)
    int off2 = sg_append_buffer(buf, &SG_RANGE(bytes));   // WRITE_NO_OVERWRITE
    T(off1 == 0);
    T(off2 == 32);
    sg_destroy_buffer(buf);
    teardown();
}

UTEST(sokol_gfx_d3d11, image_update_via_map) {
    setup();
    sg_image img = sg_make_image(&(sg_image_desc){
        .width = 8, .height = 8, .pixel_format = SG_PIXELFORMAT_RGBA8,
        .usage.dynamic_update = true,
    });
    uint8_t px[8 * 8 * 4] = {0};
    sg_update_image(img, &(sg_image_data){ .mip_levels[0] = SG_RANGE(px) });
    sg_destroy_image(img);
    T(sg_isvalid());
    teardown();
}

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
        .size = 64, .usage.immutable = true, .usage.write_unsealed = true,
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
    sg_buffer buf = sg_make_buffer(&(sg_buffer_desc){ .size = 64, .usage.dynamic_update = true });
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

UTEST_MAIN()
