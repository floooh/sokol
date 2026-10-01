//------------------------------------------------------------------------------
//  LLM maintained.
//
//  sokol_gfx_gl_test.c
//  Smoke tests for the sokol_gfx.h GL backend, running against the mock GL
//  library in tests/mocks/gl. One executable is built per supported GL
//  version, the version is selected with -DGL_MOCK_VERSION=NNN:
//
//      SOKOL_GLCORE: 410, 430
//      SOKOL_GLES3:  300, 310, 320
//
//  Only exercises paths that are backend-specific -- generic public API
//  behaviour is covered by the DUMMY-backend suite in sokol_gfx_test.c and
//  is not repeated here.
//
//  Rough coverage map (referenced sokol_gfx.h symbol on the right):
//    - Backend setup / teardown                    _sg_gl_setup_backend / _sg_gl_discard_backend
//    - Version-dependent features                  _sg_gl_init_caps_glcore / _sg_gl_init_caps_gles3
//    - Limits from glGetIntegerv                   _sg_gl_init_limits
//    - Extension-driven pixel formats              _sg_gl_init_pixelformats_*
//    - Initial state cache reset                   _sg_gl_reset_state_cache
//    - Buffer create paths (all usage flavours)    _sg_gl_create_buffer / _sg_gl_buffer_*
//    - Image create paths (2D / 3D / cube / MSAA)  _sg_gl_create_image / _sg_gl_texstorage
//    - Sampler create                              _sg_gl_create_sampler
//    - Shader create (uniform blocks, compute)     _sg_gl_create_shader / _sg_gl_compile_shader
//    - Pipeline create (raster/depth/blend/topo)   _sg_gl_create_pipeline
//    - View creation (texture / attachment / MSAA) _sg_gl_create_view
//    - Pass begin/end (swapchain, offscreen, MSAA) _sg_gl_begin_pass / _sg_gl_end_pass
//    - Draw / dispatch                             _sg_gl_draw / _sg_gl_dispatch
//    - Resource writes (transient / unsealed)      _sg_gl_write_* / _sg_gl_write_miplevel_data
//    - Resource copies                             _sg_gl_copy_buffer_to_buffer / _sg_gl_copy_buffer_to_image
//    - Compute memory barriers                     _sg_gl_handle_memory_barriers
//    - Injected native handles                     sg_*_desc.gl_buffer / .gl_texture / .gl_sampler
//    - Error paths                                 shader compile / link failure
//------------------------------------------------------------------------------
#define SOKOL_IMPL
#define SOKOL_EXTERNAL_GL_LOADER
#include "gl_mock.h"
#include "sokol_gfx.h"
#include "utest.h"
#include <string.h>

#define T(b) EXPECT_TRUE(b)
// TA() aborts the current test, use it before dereferencing a returned pointer
#define TA(b) ASSERT_TRUE(b)

#if defined(SOKOL_GLCORE)
    #define EXPECTED_BACKEND SG_BACKEND_GLCORE
#else
    #define EXPECTED_BACKEND SG_BACKEND_GLES3
#endif

// Backend feature gates, keyed on GL_MOCK_VERSION and never on sokol_gfx.h's
// private _SOKOL_GL_HAS_* macros. Those macros are the union of what the mock
// predefines (tests/mocks/gl/gl.h) and what the *host* platform branch in
// sokol_gfx.h adds on top, so they differ per host -- on a Linux host for
// example GLES3 always gets _SOKOL_GL_HAS_COMPUTE, even at GL_MOCK_VERSION
// 300. GL_MOCK_VERSION is the same on every host, and the mock declares all
// GL enums and functions regardless of version, so a version-gated test body
// compiles everywhere.
//
// The thresholds must match the feature macro block in tests/mocks/gl/gl.h.
#if defined(SOKOL_GLCORE)
    #define TEST_HAS_COMPUTE    (GL_MOCK_VERSION >= 430)
    #define TEST_HAS_TEXSTORAGE (GL_MOCK_VERSION >= 420)
#else
    #define TEST_HAS_COMPUTE    (GL_MOCK_VERSION >= 310)
    #define TEST_HAS_TEXSTORAGE (1)
#endif

//------------------------------------------------------------------------------
//  test harness
//------------------------------------------------------------------------------
#define MAX_LOG_ITEMS (64)
static sg_log_item log_items[MAX_LOG_ITEMS];
static int num_log_items;
static int num_error_logs;      // panic- and error-level log messages (incl. validation errors)

static void reset_log(void) {
    num_log_items = 0;
    num_error_logs = 0;
    memset(log_items, 0, sizeof(log_items));
}

static void capture_log(const char* tag, uint32_t log_level, uint32_t log_item_id, const char* msg, uint32_t line_nr, const char* file, void* ud) {
    (void)tag; (void)msg; (void)line_nr; (void)file; (void)ud;
    if (log_level <= 1) {
        num_error_logs++;
    }
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

static bool no_errors(void) {
    return num_error_logs == 0;
}

static void setup(void) {
    // a previous test aborted by TA() may have skipped its teardown
    if (sg_isvalid()) {
        sg_shutdown();
        gl_mock_shutdown();
    }
    reset_log();
    gl_mock_setup();
    sg_setup(&(sg_desc){
        .environment.defaults = {
            .color_format = SG_PIXELFORMAT_RGBA8,
            .depth_format = SG_PIXELFORMAT_DEPTH_STENCIL,
            .sample_count = 1,
        },
        .logger.func = capture_log,
    });
}

static void teardown(void) {
    sg_shutdown();
    gl_mock_shutdown();
}

// a minimal shader which passes GL backend validation
static sg_shader make_test_shader(void) {
    return sg_make_shader(&(sg_shader_desc){
        .vertex_func.source = "void main() { gl_Position = vec4(0); }",
        .fragment_func.source = "void main() { }",
    });
}

//------------------------------------------------------------------------------
//  backend setup and teardown
//------------------------------------------------------------------------------
UTEST(sokol_gfx_gl, backend_and_version) {
    setup();
    T(sg_isvalid());
    T(sg_query_backend() == EXPECTED_BACKEND);
    T(gl_mock_version() == GL_MOCK_VERSION);
    teardown();
}

UTEST(sokol_gfx_gl, setup_creates_vao_and_framebuffer) {
    setup();
    // setup creates one 'global' vertex array object and one framebuffer
    T(gl_mock_live_objects(GL_MOCK_OBJ_VERTEXARRAY) == 1);
    T(gl_mock_live_objects(GL_MOCK_OBJ_FRAMEBUFFER) == 1);
    T(gl_mock_bindings()->vertex_array != 0);
    T(gl_mock_count_calls(GL_MOCK_FUNC_glGenVertexArrays) == 1);
    T(gl_mock_count_calls(GL_MOCK_FUNC_glGenFramebuffers) == 1);
    // GL_UNPACK_ALIGNMENT is set to 1 for tightly packed pixel data
    T(gl_mock_render_state()->unpack_alignment == 1);
    #if defined(SOKOL_GLCORE)
    T(gl_mock_is_enabled(GL_TEXTURE_CUBE_MAP_SEAMLESS));
    #endif
    teardown();
}

UTEST(sokol_gfx_gl, setup_resets_state_cache) {
    setup();
    const gl_mock_render_state_t* rs = gl_mock_render_state();
    // the state cache reset puts the pipeline into a known default state:
    // depth test and scissor test stay enabled, their effect is switched off
    // through the depth func and the scissor rectangle instead
    T(gl_mock_is_enabled(GL_DEPTH_TEST));
    T(gl_mock_is_enabled(GL_SCISSOR_TEST));
    T(gl_mock_is_enabled(GL_DITHER));
    T(!gl_mock_is_enabled(GL_STENCIL_TEST));
    T(!gl_mock_is_enabled(GL_BLEND));
    T(!gl_mock_is_enabled(GL_CULL_FACE));
    T(!gl_mock_is_enabled(GL_POLYGON_OFFSET_FILL));
    T(!gl_mock_is_enabled(GL_SAMPLE_ALPHA_TO_COVERAGE));
    #if defined(SOKOL_GLCORE)
    T(gl_mock_is_enabled(GL_MULTISAMPLE));
    T(gl_mock_is_enabled(GL_PROGRAM_POINT_SIZE));
    #endif
    T(rs->depth_func == GL_ALWAYS);
    T(rs->depth_mask == GL_FALSE);
    T(rs->stencil_write_mask == 0);
    T(rs->stencil_front.func == GL_ALWAYS);
    T(rs->stencil_front.fail_op == GL_KEEP);
    T(rs->front_face == GL_CW);
    T(rs->cull_face == GL_BACK);
    T(rs->blend_src_rgb == GL_ONE);
    T(rs->blend_dst_rgb == GL_ZERO);
    T(rs->blend_op_rgb == GL_FUNC_ADD);
    T(rs->color_mask[0][0] && rs->color_mask[0][1] && rs->color_mask[0][2] && rs->color_mask[0][3]);
    T(gl_mock_bindings()->program == 0);
    T(gl_mock_bindings()->array_buffer == 0);
    T(gl_mock_bindings()->element_array_buffer == 0);
    teardown();
}

UTEST(sokol_gfx_gl, shutdown_releases_all_objects) {
    setup();
    T(gl_mock_live_objects_total() > 0);
    sg_shutdown();
    T(gl_mock_live_objects_total() == 0);
    gl_mock_shutdown();
}

//------------------------------------------------------------------------------
//  version dependent features
//------------------------------------------------------------------------------
UTEST(sokol_gfx_gl, features) {
    setup();
    const sg_features f = sg_query_features();
    T(!f.origin_top_left);
    #if defined(SOKOL_GLCORE)
        T(f.image_clamp_to_border);
        T(f.mrt_independent_write_mask);
        T(f.compute == (GL_MOCK_VERSION >= 430));
        T(f.gl_texture_views == (GL_MOCK_VERSION >= 430));
        T(f.draw_base_vertex);          // GLCORE is always >= 3.2
        T(f.draw_base_instance == (GL_MOCK_VERSION >= 420));
    #else
        T(!f.image_clamp_to_border);
        T(f.compute == (GL_MOCK_VERSION >= 310));
        T(!f.gl_texture_views);
        T(!f.msaa_texture_bindings);
        T(f.draw_base_vertex == (GL_MOCK_VERSION >= 320));
        T(!f.draw_base_instance);
        T(!f.dual_source_blending);
    #endif
    T(!f.mrt_independent_blend_state);
    T(f.vertexformat_int10_n2);
    teardown();
}

UTEST(sokol_gfx_gl, limits_from_getintegerv) {
    setup();
    const sg_limits l = sg_query_limits();
    // must match the default table in tests/mocks/gl/gl_mock.c
    T(l.max_image_size_2d == 16384);
    T(l.max_image_size_cube == 16384);
    T(l.max_image_size_3d == 2048);
    // the array image size limit comes from GL_MAX_TEXTURE_SIZE too
    T(l.max_image_size_array == 16384);
    T(l.max_image_array_layers == 2048);
    T(l.max_vertex_attrs == 16);
    T(l.gl_max_vertex_uniform_components == 4096);
    T(l.gl_max_combined_texture_image_units == 32);
    teardown();
}

UTEST(sokol_gfx_gl, limits_override) {
    reset_log();
    gl_mock_setup();
    // the mock answers limit queries from a table which tests can override
    gl_mock_set_int(GL_MAX_VERTEX_ATTRIBS, 8);
    gl_mock_set_int(GL_MAX_TEXTURE_SIZE, 4096);
    sg_setup(&(sg_desc){ .logger.func = capture_log });
    T(sg_query_limits().max_vertex_attrs == 8);
    T(sg_query_limits().max_image_size_2d == 4096);
    teardown();
}

UTEST(sokol_gfx_gl, pixelformats_from_extensions) {
    setup();
    // uncompressed formats are always supported
    T(sg_query_pixelformat(SG_PIXELFORMAT_RGBA8).sample);
    T(sg_query_pixelformat(SG_PIXELFORMAT_RGBA8).render);
    T(sg_query_pixelformat(SG_PIXELFORMAT_RGBA8).blend);
    T(sg_query_pixelformat(SG_PIXELFORMAT_DEPTH_STENCIL).depth);
    // the mock reports all compression extensions
    T(sg_query_pixelformat(SG_PIXELFORMAT_BC1_RGBA).sample);
    T(sg_query_pixelformat(SG_PIXELFORMAT_BC4_R).sample);
    T(sg_query_pixelformat(SG_PIXELFORMAT_BC7_RGBA).sample);
    T(sg_query_pixelformat(SG_PIXELFORMAT_ETC2_RGB8).sample);
    T(sg_query_pixelformat(SG_PIXELFORMAT_ASTC_4x4_RGBA).sample);
    // ...and the float colour buffer extensions
    T(sg_query_pixelformat(SG_PIXELFORMAT_RGBA32F).render);
    T(sg_query_pixelformat(SG_PIXELFORMAT_RGBA16F).render);
    teardown();
}

//------------------------------------------------------------------------------
//  resource create / destroy round trips
//------------------------------------------------------------------------------
UTEST(sokol_gfx_gl, buffer_create_destroy) {
    setup();
    static const float data[] = { 1.0f, 2.0f, 3.0f, 4.0f };
    sg_buffer buf = sg_make_buffer(&(sg_buffer_desc){ .data = SG_RANGE(data) });
    T(sg_query_buffer_state(buf) == SG_RESOURCESTATE_VALID);
    T(gl_mock_live_objects(GL_MOCK_OBJ_BUFFER) == 1);

    const gl_mock_call_t* call = gl_mock_last_call(GL_MOCK_FUNC_glBufferData);
    TA(call != 0);
    T(call->args[0].i == GL_ARRAY_BUFFER);
    T(call->args[1].i == (int64_t)sizeof(data));
    T(call->args[3].i == GL_STATIC_DRAW);

    sg_destroy_buffer(buf);
    T(gl_mock_live_objects(GL_MOCK_OBJ_BUFFER) == 0);
    teardown();
}

UTEST(sokol_gfx_gl, image_create_destroy) {
    setup();
    // an immutable image uses a single texture slot
    static uint32_t pixels[8 * 8];
    sg_image img = sg_make_image(&(sg_image_desc){
        .width = 8,
        .height = 8,
        .pixel_format = SG_PIXELFORMAT_RGBA8,
        .data.mip_levels[0] = SG_RANGE(pixels),
    });
    T(sg_query_image_state(img) == SG_RESOURCESTATE_VALID);
    T(gl_mock_live_objects(GL_MOCK_OBJ_TEXTURE) == 1);

    // GL 4.1 has no glTexStorage2D and must take the glTexImage2D path
    #if defined(SOKOL_GLCORE) && (GL_MOCK_VERSION < 420)
        T(gl_mock_count_calls(GL_MOCK_FUNC_glTexStorage2D) == 0);
        T(gl_mock_count_calls(GL_MOCK_FUNC_glTexImage2D) == 1);
    #else
        T(gl_mock_count_calls(GL_MOCK_FUNC_glTexStorage2D) == 1);
        T(gl_mock_count_calls(GL_MOCK_FUNC_glTexSubImage2D) == 1);
    #endif

    sg_destroy_image(img);
    T(gl_mock_live_objects(GL_MOCK_OBJ_TEXTURE) == 0);
    teardown();
}

UTEST(sokol_gfx_gl, write_transient_image_uses_renaming_slots) {
    setup();
    // a write-transient image is created with SG_NUM_INFLIGHT_FRAMES textures
    sg_image img = sg_make_image(&(sg_image_desc){
        .width = 8,
        .height = 8,
        .pixel_format = SG_PIXELFORMAT_RGBA8,
        .usage.write_transient = true,
    });
    T(sg_query_image_state(img) == SG_RESOURCESTATE_VALID);
    T(gl_mock_live_objects(GL_MOCK_OBJ_TEXTURE) == SG_NUM_INFLIGHT_FRAMES);
    sg_destroy_image(img);
    T(gl_mock_live_objects(GL_MOCK_OBJ_TEXTURE) == 0);
    teardown();
}

UTEST(sokol_gfx_gl, sampler_create_destroy) {
    setup();
    sg_sampler smp = sg_make_sampler(&(sg_sampler_desc){
        .min_filter = SG_FILTER_LINEAR,
        .mag_filter = SG_FILTER_LINEAR,
        .wrap_u = SG_WRAP_CLAMP_TO_EDGE,
        .wrap_v = SG_WRAP_CLAMP_TO_EDGE,
    });
    T(sg_query_sampler_state(smp) == SG_RESOURCESTATE_VALID);
    T(gl_mock_live_objects(GL_MOCK_OBJ_SAMPLER) == 1);

    // sampler state goes through glSamplerParameter*. With a default sampler
    // desc (compare == NEVER, max_anisotropy == 0), sokol_gfx sets 6 int
    // parameters: min/mag filter, wrap s/t/r and compare mode.
    const gl_mock_call_t* call = gl_mock_last_call(GL_MOCK_FUNC_glSamplerParameteri);
    TA(call != 0);
    T(gl_mock_count_calls(GL_MOCK_FUNC_glSamplerParameteri) == 6);
    T(gl_mock_count_calls(GL_MOCK_FUNC_glSamplerParameterf) == 2);   // min/max lod

    sg_destroy_sampler(smp);
    T(gl_mock_live_objects(GL_MOCK_OBJ_SAMPLER) == 0);
    teardown();
}

UTEST(sokol_gfx_gl, shader_create_destroy) {
    setup();
    sg_shader shd = make_test_shader();
    T(sg_query_shader_state(shd) == SG_RESOURCESTATE_VALID);
    // the two shader objects are deleted right after linking
    T(gl_mock_live_objects(GL_MOCK_OBJ_PROGRAM) == 1);
    T(gl_mock_live_objects(GL_MOCK_OBJ_SHADER) == 0);
    T(gl_mock_count_calls(GL_MOCK_FUNC_glCreateShader) == 2);
    T(gl_mock_count_calls(GL_MOCK_FUNC_glLinkProgram) == 1);

    sg_destroy_shader(shd);
    T(gl_mock_live_objects(GL_MOCK_OBJ_PROGRAM) == 0);
    teardown();
}

UTEST(sokol_gfx_gl, pipeline_create_destroy) {
    setup();
    sg_shader shd = make_test_shader();
    sg_pipeline pip = sg_make_pipeline(&(sg_pipeline_desc){
        .shader = shd,
        .layout.attrs[0].format = SG_VERTEXFORMAT_FLOAT3,
    });
    T(sg_query_pipeline_state(pip) == SG_RESOURCESTATE_VALID);
    // a GL pipeline is pure CPU-side state, no GL objects
    T(gl_mock_live_objects(GL_MOCK_OBJ_PROGRAM) == 1);
    sg_destroy_pipeline(pip);
    sg_destroy_shader(shd);
    T(gl_mock_live_objects(GL_MOCK_OBJ_PROGRAM) == 0);
    teardown();
}

UTEST(sokol_gfx_gl, full_round_trip_leaves_no_objects) {
    setup();
    const int base = gl_mock_live_objects_total();
    static const float data[] = { 1.0f, 2.0f, 3.0f, 4.0f };
    sg_buffer buf = sg_make_buffer(&(sg_buffer_desc){ .data = SG_RANGE(data) });
    sg_image img = sg_make_image(&(sg_image_desc){ .width = 4, .height = 4, .usage.write_transient = true });
    sg_sampler smp = sg_make_sampler(&(sg_sampler_desc){0});
    sg_shader shd = make_test_shader();
    sg_pipeline pip = sg_make_pipeline(&(sg_pipeline_desc){ .shader = shd });
    T(gl_mock_live_objects_total() > base);
    sg_destroy_pipeline(pip);
    sg_destroy_shader(shd);
    sg_destroy_sampler(smp);
    sg_destroy_image(img);
    sg_destroy_buffer(buf);
    T(gl_mock_live_objects_total() == base);
    teardown();
}

//------------------------------------------------------------------------------
//  error paths
//------------------------------------------------------------------------------
UTEST(sokol_gfx_gl, shader_compilation_failure) {
    setup();
    gl_mock_set_info_log("mock compile error");
    gl_mock_fail_next_compile(1);
    sg_shader shd = make_test_shader();
    T(sg_query_shader_state(shd) == SG_RESOURCESTATE_FAILED);
    T(logged(SG_LOGITEM_GL_SHADER_COMPILATION_FAILED));
    // the failed attempt must not leak GL objects
    T(gl_mock_live_objects(GL_MOCK_OBJ_SHADER) == 0);
    T(gl_mock_live_objects(GL_MOCK_OBJ_PROGRAM) == 0);
    sg_destroy_shader(shd);
    teardown();
}

UTEST(sokol_gfx_gl, shader_linking_failure) {
    setup();
    gl_mock_set_info_log("mock link error");
    gl_mock_fail_next_link(1);
    sg_shader shd = make_test_shader();
    T(sg_query_shader_state(shd) == SG_RESOURCESTATE_FAILED);
    T(logged(SG_LOGITEM_GL_SHADER_LINKING_FAILED));
    T(gl_mock_live_objects(GL_MOCK_OBJ_SHADER) == 0);
    T(gl_mock_live_objects(GL_MOCK_OBJ_PROGRAM) == 0);
    sg_destroy_shader(shd);
    teardown();
}

//------------------------------------------------------------------------------
//  call log sanity
//------------------------------------------------------------------------------
UTEST(sokol_gfx_gl, call_log) {
    setup();
    T(gl_mock_num_calls() > 0);
    T(!gl_mock_call_log_overflow());
    const int idx = gl_mock_find_call(GL_MOCK_FUNC_glBindVertexArray, 0);
    T(idx >= 0);
    T(gl_mock_call(idx)->func == GL_MOCK_FUNC_glBindVertexArray);
    T(0 == strcmp(gl_mock_func_name(GL_MOCK_FUNC_glBindVertexArray), "glBindVertexArray"));
    gl_mock_clear_calls();
    T(gl_mock_num_calls() == 0);
    teardown();
}

//------------------------------------------------------------------------------
//  buffer creation code paths
//------------------------------------------------------------------------------
UTEST(sokol_gfx_gl, buffer_copy_dst_uses_dynamic_copy) {
    setup();
    sg_buffer buf = sg_make_buffer(&(sg_buffer_desc){
        .size = 256, .usage.copy_dst = true,
    });
    T(sg_query_buffer_state(buf) == SG_RESOURCESTATE_VALID);
    // a copy-dst buffer has a single slot
    T(gl_mock_live_objects(GL_MOCK_OBJ_BUFFER) == 1);
    const gl_mock_call_t* call = gl_mock_last_call(GL_MOCK_FUNC_glBufferData);
    TA(call != 0);
    T(call->args[3].i == GL_DYNAMIC_COPY);
    sg_destroy_buffer(buf);
    T(gl_mock_live_objects(GL_MOCK_OBJ_BUFFER) == 0);
    teardown();
}

UTEST(sokol_gfx_gl, buffer_stream_uses_stream_draw) {
    setup();
    sg_buffer buf = sg_make_buffer(&(sg_buffer_desc){
        .size = 128, .usage.write_transient = true,
    });
    T(sg_query_buffer_state(buf) == SG_RESOURCESTATE_VALID);
    // write-transient buffers use SG_NUM_INFLIGHT_FRAMES renaming slots
    T(gl_mock_live_objects(GL_MOCK_OBJ_BUFFER) == SG_NUM_INFLIGHT_FRAMES);
    const gl_mock_call_t* call = gl_mock_last_call(GL_MOCK_FUNC_glBufferData);
    TA(call != 0);
    T(call->args[3].i == GL_STREAM_DRAW);
    sg_destroy_buffer(buf);
    teardown();
}

UTEST(sokol_gfx_gl, buffer_index_uses_element_array_target) {
    setup();
    static const uint16_t indices[] = { 0, 1, 2, 0, 2, 3 };
    sg_buffer buf = sg_make_buffer(&(sg_buffer_desc){
        .usage.index_buffer = true,
        .data = SG_RANGE(indices),
    });
    T(sg_query_buffer_state(buf) == SG_RESOURCESTATE_VALID);
    const gl_mock_call_t* call = gl_mock_last_call(GL_MOCK_FUNC_glBufferData);
    TA(call != 0);
    T(call->args[0].i == GL_ELEMENT_ARRAY_BUFFER);
    sg_destroy_buffer(buf);
    teardown();
}

#if TEST_HAS_COMPUTE
UTEST(sokol_gfx_gl, buffer_storage_uses_shader_storage_target) {
    setup();
    sg_buffer buf = sg_make_buffer(&(sg_buffer_desc){
        .size = 1024, .usage.storage_buffer = true,
    });
    T(sg_query_buffer_state(buf) == SG_RESOURCESTATE_VALID);
    const gl_mock_call_t* call = gl_mock_last_call(GL_MOCK_FUNC_glBufferData);
    TA(call != 0);
    T(call->args[0].i == GL_SHADER_STORAGE_BUFFER);
    sg_destroy_buffer(buf);
    teardown();
}
#endif

UTEST(sokol_gfx_gl, buffer_inject_native) {
    setup();
    // pretend the caller already created a GL buffer object
    const uint32_t injected_buf = 0xDEAD;
    sg_buffer buf = sg_make_buffer(&(sg_buffer_desc){
        .size = 64,
        .gl_buffer = injected_buf,
    });
    T(sg_query_buffer_state(buf) == SG_RESOURCESTATE_VALID);
    // sokol must not call glGenBuffers/glBufferData for injected handles
    T(gl_mock_count_calls(GL_MOCK_FUNC_glGenBuffers) == 0);
    T(gl_mock_count_calls(GL_MOCK_FUNC_glBufferData) == 0);
    sg_destroy_buffer(buf);
    // and must not call glDeleteBuffers
    T(gl_mock_count_calls(GL_MOCK_FUNC_glDeleteBuffers) == 0);
    teardown();
}

//------------------------------------------------------------------------------
//  image creation code paths -- 2D / 3D / cube / array / MSAA / storage
//------------------------------------------------------------------------------
UTEST(sokol_gfx_gl, image_3d_takes_texstorage3d_or_teximage3d) {
    setup();
    sg_image img = sg_make_image(&(sg_image_desc){
        .type = SG_IMAGETYPE_3D,
        .width = 8, .height = 8, .num_slices = 8,
        .pixel_format = SG_PIXELFORMAT_RGBA8,
        .usage.write_transient = true,
    });
    T(sg_query_image_state(img) == SG_RESOURCESTATE_VALID);
    T(gl_mock_live_objects(GL_MOCK_OBJ_TEXTURE) == SG_NUM_INFLIGHT_FRAMES);
    #if TEST_HAS_TEXSTORAGE
        T(gl_mock_count_calls(GL_MOCK_FUNC_glTexStorage3D) == SG_NUM_INFLIGHT_FRAMES);
    #else
        T(gl_mock_count_calls(GL_MOCK_FUNC_glTexImage3D) > 0);
    #endif
    // bind target must be GL_TEXTURE_3D
    const gl_mock_call_t* bind = gl_mock_last_call(GL_MOCK_FUNC_glBindTexture);
    TA(bind != 0);
    T(bind->args[0].i == GL_TEXTURE_3D);
    sg_destroy_image(img);
    teardown();
}

UTEST(sokol_gfx_gl, image_cube_uses_texture_cube_map_target) {
    setup();
    static uint32_t face_pixels[8 * 8];
    sg_image img = sg_make_image(&(sg_image_desc){
        .type = SG_IMAGETYPE_CUBE,
        .width = 8, .height = 8,
        .pixel_format = SG_PIXELFORMAT_RGBA8,
        .data.mip_levels[0].ptr = face_pixels,
        .data.mip_levels[0].size = sizeof(face_pixels) * 6,
    });
    T(sg_query_image_state(img) == SG_RESOURCESTATE_VALID);
    const gl_mock_call_t* bind = gl_mock_last_call(GL_MOCK_FUNC_glBindTexture);
    TA(bind != 0);
    T(bind->args[0].i == GL_TEXTURE_CUBE_MAP);
    sg_destroy_image(img);
    teardown();
}

UTEST(sokol_gfx_gl, image_array_uses_2d_array_target) {
    setup();
    sg_image img = sg_make_image(&(sg_image_desc){
        .type = SG_IMAGETYPE_ARRAY,
        .width = 8, .height = 8, .num_slices = 4,
        .pixel_format = SG_PIXELFORMAT_RGBA8,
        .usage.color_attachment = true,
    });
    T(sg_query_image_state(img) == SG_RESOURCESTATE_VALID);
    const gl_mock_call_t* bind = gl_mock_last_call(GL_MOCK_FUNC_glBindTexture);
    TA(bind != 0);
    T(bind->args[0].i == GL_TEXTURE_2D_ARRAY);
    sg_destroy_image(img);
    teardown();
}

UTEST(sokol_gfx_gl, image_depth_attachment) {
    setup();
    sg_image img = sg_make_image(&(sg_image_desc){
        .width = 64, .height = 64,
        .pixel_format = SG_PIXELFORMAT_DEPTH_STENCIL,
        .usage.depth_stencil_attachment = true,
    });
    T(sg_query_image_state(img) == SG_RESOURCESTATE_VALID);
    T(gl_mock_live_objects(GL_MOCK_OBJ_TEXTURE) == 1);
    sg_destroy_image(img);
    teardown();
}

UTEST(sokol_gfx_gl, image_msaa_attachment_only) {
    setup();
    // On macOS GLCORE and all GLES3, MSAA texture bindings are disabled, so
    // MSAA color attachment images have NO underlying GL texture object.
    sg_image img = sg_make_image(&(sg_image_desc){
        .width = 128, .height = 128,
        .pixel_format = SG_PIXELFORMAT_RGBA8,
        .sample_count = 4,
        .usage.color_attachment = true,
    });
    T(sg_query_image_state(img) == SG_RESOURCESTATE_VALID);
    if (!sg_query_features().msaa_texture_bindings) {
        T(gl_mock_live_objects(GL_MOCK_OBJ_TEXTURE) == 0);
    } else {
        T(gl_mock_live_objects(GL_MOCK_OBJ_TEXTURE) == 1);
    }
    sg_destroy_image(img);
    teardown();
}

#if TEST_HAS_COMPUTE
UTEST(sokol_gfx_gl, image_storage_creates_texture) {
    setup();
    sg_image img = sg_make_image(&(sg_image_desc){
        .width = 32, .height = 32,
        .pixel_format = SG_PIXELFORMAT_RGBA8,
        .usage.storage_image = true,
    });
    T(sg_query_image_state(img) == SG_RESOURCESTATE_VALID);
    T(gl_mock_live_objects(GL_MOCK_OBJ_TEXTURE) == 1);
    sg_destroy_image(img);
    teardown();
}
#endif

UTEST(sokol_gfx_gl, image_compressed_uses_compressed_upload) {
    setup();
    // BC1 has 8 bytes per 4x4 block; 8x8 pixels = 4 blocks = 32 bytes
    static uint8_t bc1[32];
    sg_image img = sg_make_image(&(sg_image_desc){
        .width = 8, .height = 8,
        .pixel_format = SG_PIXELFORMAT_BC1_RGBA,
        .data.mip_levels[0] = SG_RANGE(bc1),
    });
    T(sg_query_image_state(img) == SG_RESOURCESTATE_VALID);
    #if TEST_HAS_TEXSTORAGE
        T(gl_mock_count_calls(GL_MOCK_FUNC_glCompressedTexSubImage2D) == 1);
    #else
        T(gl_mock_count_calls(GL_MOCK_FUNC_glCompressedTexImage2D) == 1);
    #endif
    sg_destroy_image(img);
    teardown();
}

UTEST(sokol_gfx_gl, image_inject_native) {
    setup();
    const uint32_t injected_tex = 0xC0DE;
    sg_image img = sg_make_image(&(sg_image_desc){
        .width = 16, .height = 16,
        .pixel_format = SG_PIXELFORMAT_RGBA8,
        .gl_texture = injected_tex,
    });
    T(sg_query_image_state(img) == SG_RESOURCESTATE_VALID);
    // no glGenTextures / glTexStorage / glTexImage for injected handles
    T(gl_mock_count_calls(GL_MOCK_FUNC_glGenTextures) == 0);
    T(gl_mock_count_calls(GL_MOCK_FUNC_glTexStorage2D) == 0);
    T(gl_mock_count_calls(GL_MOCK_FUNC_glTexImage2D) == 0);
    sg_destroy_image(img);
    T(gl_mock_count_calls(GL_MOCK_FUNC_glDeleteTextures) == 0);
    teardown();
}

//------------------------------------------------------------------------------
//  sampler variants -- exercise filter / wrap / compare / anisotropy
//------------------------------------------------------------------------------
UTEST(sokol_gfx_gl, sampler_with_compare_sets_ref_to_texture) {
    setup();
    sg_sampler smp = sg_make_sampler(&(sg_sampler_desc){
        .compare = SG_COMPAREFUNC_LESS_EQUAL,
    });
    T(sg_query_sampler_state(smp) == SG_RESOURCESTATE_VALID);
    // enabling compare adds two extra glSamplerParameteri calls
    // (GL_TEXTURE_COMPARE_MODE + GL_TEXTURE_COMPARE_FUNC)
    T(gl_mock_count_calls(GL_MOCK_FUNC_glSamplerParameteri) == 7);
    sg_destroy_sampler(smp);
    teardown();
}

UTEST(sokol_gfx_gl, sampler_with_anisotropy) {
    setup();
    sg_sampler smp = sg_make_sampler(&(sg_sampler_desc){
        .min_filter = SG_FILTER_LINEAR,
        .mag_filter = SG_FILTER_LINEAR,
        .mipmap_filter = SG_FILTER_LINEAR,
        .max_anisotropy = 16,
    });
    T(sg_query_sampler_state(smp) == SG_RESOURCESTATE_VALID);
    // one extra glSamplerParameteri for anisotropy
    T(gl_mock_count_calls(GL_MOCK_FUNC_glSamplerParameteri) == 7);
    sg_destroy_sampler(smp);
    teardown();
}

UTEST(sokol_gfx_gl, sampler_inject_native) {
    setup();
    const uint32_t injected_smp = 0xABCD;
    sg_sampler smp = sg_make_sampler(&(sg_sampler_desc){
        .gl_sampler = injected_smp,
    });
    T(sg_query_sampler_state(smp) == SG_RESOURCESTATE_VALID);
    T(gl_mock_count_calls(GL_MOCK_FUNC_glGenSamplers) == 0);
    T(gl_mock_count_calls(GL_MOCK_FUNC_glSamplerParameteri) == 0);
    sg_destroy_sampler(smp);
    T(gl_mock_count_calls(GL_MOCK_FUNC_glDeleteSamplers) == 0);
    teardown();
}

//------------------------------------------------------------------------------
//  shader creation -- uniform blocks, texture/sampler pairs, compute
//------------------------------------------------------------------------------
UTEST(sokol_gfx_gl, shader_with_uniform_block_queries_locations) {
    setup();
    sg_shader shd = sg_make_shader(&(sg_shader_desc){
        .vertex_func.source = "vs",
        .fragment_func.source = "fs",
        .uniform_blocks[0] = {
            .stage = SG_SHADERSTAGE_VERTEX,
            .size = 64,
            .layout = SG_UNIFORMLAYOUT_NATIVE,
            .glsl_uniforms = {
                [0] = { .type = SG_UNIFORMTYPE_MAT4, .glsl_name = "mvp" },
            },
        },
    });
    T(sg_query_shader_state(shd) == SG_RESOURCESTATE_VALID);
    // sokol must look up each glsl_name via glGetUniformLocation
    T(gl_mock_count_calls(GL_MOCK_FUNC_glGetUniformLocation) >= 1);
    sg_destroy_shader(shd);
    teardown();
}

UTEST(sokol_gfx_gl, shader_with_texture_sampler_pair) {
    setup();
    sg_shader shd = sg_make_shader(&(sg_shader_desc){
        .vertex_func.source = "vs",
        .fragment_func.source = "fs",
        .views[0].texture = {
            .stage = SG_SHADERSTAGE_FRAGMENT,
            .image_type = SG_IMAGETYPE_2D,
            .sample_type = SG_IMAGESAMPLETYPE_FLOAT,
        },
        .samplers[0] = {
            .stage = SG_SHADERSTAGE_FRAGMENT,
            .sampler_type = SG_SAMPLERTYPE_FILTERING,
        },
        .texture_sampler_pairs[0] = {
            .stage = SG_SHADERSTAGE_FRAGMENT,
            .view_slot = 0,
            .sampler_slot = 0,
            .glsl_name = "tex",
        },
    });
    T(sg_query_shader_state(shd) == SG_RESOURCESTATE_VALID);
    // texture uniform slot bound via glUseProgram + glUniform1i
    T(gl_mock_count_calls(GL_MOCK_FUNC_glUniform1i) >= 1);
    sg_destroy_shader(shd);
    teardown();
}

#if TEST_HAS_COMPUTE
UTEST(sokol_gfx_gl, shader_compute) {
    setup();
    sg_shader shd = sg_make_shader(&(sg_shader_desc){
        .compute_func.source = "void main() { }",
    });
    T(sg_query_shader_state(shd) == SG_RESOURCESTATE_VALID);
    // one shader created for the compute stage
    T(gl_mock_count_calls(GL_MOCK_FUNC_glCreateShader) == 1);
    T(gl_mock_count_calls(GL_MOCK_FUNC_glLinkProgram) == 1);
    sg_destroy_shader(shd);
    teardown();
}
#endif

//------------------------------------------------------------------------------
//  pipeline creation -- render / depth / stencil / blend / primitive types
//------------------------------------------------------------------------------
UTEST(sokol_gfx_gl, pipeline_all_primitive_types) {
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
            .shader = shd,
            .primitive_type = prims[i],
            .layout.attrs[0].format = SG_VERTEXFORMAT_FLOAT3,
        });
        T(sg_query_pipeline_state(pip) == SG_RESOURCESTATE_VALID);
        sg_destroy_pipeline(pip);
    }
    sg_destroy_shader(shd);
    teardown();
}

UTEST(sokol_gfx_gl, pipeline_index_types) {
    setup();
    sg_shader shd = make_test_shader();
    sg_pipeline p16 = sg_make_pipeline(&(sg_pipeline_desc){
        .shader = shd, .index_type = SG_INDEXTYPE_UINT16,
        .layout.attrs[0].format = SG_VERTEXFORMAT_FLOAT3,
    });
    sg_pipeline p32 = sg_make_pipeline(&(sg_pipeline_desc){
        .shader = shd, .index_type = SG_INDEXTYPE_UINT32,
        .layout.attrs[0].format = SG_VERTEXFORMAT_FLOAT3,
    });
    T(sg_query_pipeline_state(p16) == SG_RESOURCESTATE_VALID);
    T(sg_query_pipeline_state(p32) == SG_RESOURCESTATE_VALID);
    sg_destroy_pipeline(p16);
    sg_destroy_pipeline(p32);
    sg_destroy_shader(shd);
    teardown();
}

#if TEST_HAS_COMPUTE
UTEST(sokol_gfx_gl, pipeline_compute) {
    setup();
    sg_shader shd = sg_make_shader(&(sg_shader_desc){
        .compute_func.source = "void main() { }",
    });
    sg_pipeline pip = sg_make_pipeline(&(sg_pipeline_desc){
        .shader = shd, .compute = true,
    });
    T(sg_query_pipeline_state(pip) == SG_RESOURCESTATE_VALID);
    sg_destroy_pipeline(pip);
    sg_destroy_shader(shd);
    teardown();
}
#endif

//------------------------------------------------------------------------------
//  view creation -- texture / attachment / MSAA resolve
//------------------------------------------------------------------------------
UTEST(sokol_gfx_gl, view_texture_no_new_gl_object) {
    setup();
    static uint32_t pixels[4 * 4];
    sg_image img = sg_make_image(&(sg_image_desc){
        .width = 4, .height = 4, .pixel_format = SG_PIXELFORMAT_RGBA8,
        .data.mip_levels[0] = SG_RANGE(pixels),
    });
    const int before = gl_mock_live_objects_total();
    sg_view v = sg_make_view(&(sg_view_desc){ .texture.image = img });
    T(sg_query_view_state(v) == SG_RESOURCESTATE_VALID);
    if (sg_query_features().gl_texture_views) {
        // GL 4.3 creates a glTextureView (one extra texture per slot)
        T(gl_mock_live_objects_total() > before);
    } else {
        // without ARB_texture_view a texture view is CPU-only
        T(gl_mock_live_objects_total() == before);
    }
    sg_destroy_view(v);
    sg_destroy_image(img);
    T(gl_mock_live_objects_total() == before - 1);
    teardown();
}

UTEST(sokol_gfx_gl, view_color_attachment_no_new_gl_object) {
    setup();
    sg_image img = sg_make_image(&(sg_image_desc){
        .width = 64, .height = 64, .pixel_format = SG_PIXELFORMAT_RGBA8,
        .usage.color_attachment = true,
    });
    const int before = gl_mock_live_objects_total();
    sg_view v = sg_make_view(&(sg_view_desc){ .color_attachment.image = img });
    T(sg_query_view_state(v) == SG_RESOURCESTATE_VALID);
    // non-MSAA color attachment views are CPU-only, no GL objects
    T(gl_mock_live_objects_total() == before);
    sg_destroy_view(v);
    sg_destroy_image(img);
    teardown();
}

UTEST(sokol_gfx_gl, view_depth_stencil_attachment) {
    setup();
    sg_image img = sg_make_image(&(sg_image_desc){
        .width = 64, .height = 64, .pixel_format = SG_PIXELFORMAT_DEPTH_STENCIL,
        .usage.depth_stencil_attachment = true,
    });
    sg_view v = sg_make_view(&(sg_view_desc){ .depth_stencil_attachment.image = img });
    T(sg_query_view_state(v) == SG_RESOURCESTATE_VALID);
    sg_destroy_view(v);
    sg_destroy_image(img);
    teardown();
}

UTEST(sokol_gfx_gl, view_msaa_color_creates_renderbuffer) {
    setup();
    // MSAA color attachment on a target without MSAA texture bindings creates
    // a helper renderbuffer via glGenRenderbuffers + glRenderbufferStorageMultisample
    sg_image img = sg_make_image(&(sg_image_desc){
        .width = 64, .height = 64,
        .pixel_format = SG_PIXELFORMAT_RGBA8,
        .sample_count = 4,
        .usage.color_attachment = true,
    });
    const int rb_before = gl_mock_count_calls(GL_MOCK_FUNC_glGenRenderbuffers);
    sg_view v = sg_make_view(&(sg_view_desc){ .color_attachment.image = img });
    T(sg_query_view_state(v) == SG_RESOURCESTATE_VALID);
    if (!sg_query_features().msaa_texture_bindings) {
        T(gl_mock_count_calls(GL_MOCK_FUNC_glGenRenderbuffers) == rb_before + 1);
        T(gl_mock_count_calls(GL_MOCK_FUNC_glRenderbufferStorageMultisample) >= 1);
    }
    sg_destroy_view(v);
    sg_destroy_image(img);
    teardown();
}

UTEST(sokol_gfx_gl, view_resolve_attachment_creates_helper_framebuffer) {
    setup();
    sg_image img = sg_make_image(&(sg_image_desc){
        .width = 64, .height = 64, .pixel_format = SG_PIXELFORMAT_RGBA8,
        .usage.resolve_attachment = true,
    });
    const int fb_before = gl_mock_live_objects(GL_MOCK_OBJ_FRAMEBUFFER);
    sg_view v = sg_make_view(&(sg_view_desc){ .resolve_attachment.image = img });
    T(sg_query_view_state(v) == SG_RESOURCESTATE_VALID);
    // resolve view builds its own MSAA-resolve framebuffer
    T(gl_mock_live_objects(GL_MOCK_OBJ_FRAMEBUFFER) == fb_before + 1);
    sg_destroy_view(v);
    T(gl_mock_live_objects(GL_MOCK_OBJ_FRAMEBUFFER) == fb_before);
    sg_destroy_image(img);
    teardown();
}

#if TEST_HAS_COMPUTE
UTEST(sokol_gfx_gl, view_storage_image_cpu_only) {
    setup();
    sg_image img = sg_make_image(&(sg_image_desc){
        .width = 32, .height = 32, .pixel_format = SG_PIXELFORMAT_RGBA8,
        .usage.storage_image = true,
    });
    const int before = gl_mock_live_objects_total();
    sg_view v = sg_make_view(&(sg_view_desc){ .storage_image.image = img });
    T(sg_query_view_state(v) == SG_RESOURCESTATE_VALID);
    T(gl_mock_live_objects_total() == before);
    sg_destroy_view(v);
    sg_destroy_image(img);
    teardown();
}
#endif

//------------------------------------------------------------------------------
//  passes -- swapchain, offscreen, MSAA resolve, compute
//------------------------------------------------------------------------------
UTEST(sokol_gfx_gl, swapchain_pass_clear_and_commit) {
    setup();
    gl_mock_clear_calls();
    sg_begin_pass(&(sg_pass){
        .action.colors[0] = {
            .load_action = SG_LOADACTION_CLEAR,
            .clear_value = { 0.1f, 0.2f, 0.3f, 1.0f },
        },
        .swapchain = {
            .width = 640, .height = 480, .sample_count = 1,
            .color_format = SG_PIXELFORMAT_RGBA8,
            .depth_format = SG_PIXELFORMAT_DEPTH_STENCIL,
        },
    });
    sg_apply_viewport(0, 0, 640, 480, false);
    sg_apply_scissor_rect(10, 10, 100, 100, false);
    sg_end_pass();
    sg_commit();
    // swapchain pass binds framebuffer 0
    T(gl_mock_count_calls(GL_MOCK_FUNC_glBindFramebuffer) >= 1);
    T(gl_mock_count_calls(GL_MOCK_FUNC_glViewport) >= 1);
    T(gl_mock_count_calls(GL_MOCK_FUNC_glScissor) >= 1);
    // clear is done via glClearBufferfv on the color buffer
    T(gl_mock_count_calls(GL_MOCK_FUNC_glClearBufferfv) >= 1);
    T(sg_isvalid());
    teardown();
}

UTEST(sokol_gfx_gl, offscreen_pass_binds_persistent_framebuffer) {
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
    gl_mock_clear_calls();
    sg_begin_pass(&(sg_pass){
        .attachments = { .colors = { color_v }, .depth_stencil = depth_v },
        .action = {
            .colors[0] = { .load_action = SG_LOADACTION_CLEAR, .clear_value = {0, 0, 0, 1} },
            .depth = { .load_action = SG_LOADACTION_CLEAR, .clear_value = 1.0f },
            .stencil = { .load_action = SG_LOADACTION_CLEAR, .clear_value = 0 },
        },
    });
    sg_end_pass();
    sg_commit();
    // offscreen pass calls glFramebufferTexture2D for the color attachment and
    // clears depth + stencil via glClearBufferfi
    T(gl_mock_count_calls(GL_MOCK_FUNC_glFramebufferTexture2D) >= 1);
    T(gl_mock_count_calls(GL_MOCK_FUNC_glClearBufferfi) >= 1);
    sg_destroy_view(color_v);
    sg_destroy_view(depth_v);
    sg_destroy_image(color_img);
    sg_destroy_image(depth_img);
    T(sg_isvalid());
    teardown();
}

UTEST(sokol_gfx_gl, offscreen_msaa_pass_blit_resolves) {
    setup();
    if (sg_query_features().msaa_texture_bindings) {
        // sokol_gfx uses glBlitFramebuffer only when the MSAA source is a
        // renderbuffer; skip this test on platforms that use MSAA textures
        teardown();
        return;
    }
    sg_image msaa = sg_make_image(&(sg_image_desc){
        .width = 64, .height = 64, .pixel_format = SG_PIXELFORMAT_RGBA8,
        .sample_count = 4, .usage.color_attachment = true,
    });
    sg_image resolve = sg_make_image(&(sg_image_desc){
        .width = 64, .height = 64, .pixel_format = SG_PIXELFORMAT_RGBA8,
        .usage.resolve_attachment = true,
    });
    sg_view color_v = sg_make_view(&(sg_view_desc){ .color_attachment.image = msaa });
    sg_view resolve_v = sg_make_view(&(sg_view_desc){ .resolve_attachment.image = resolve });
    gl_mock_clear_calls();
    sg_begin_pass(&(sg_pass){
        .attachments = { .colors = { color_v }, .resolves = { resolve_v } },
        .action.colors[0] = { .load_action = SG_LOADACTION_CLEAR, .store_action = SG_STOREACTION_STORE },
    });
    sg_end_pass();  // triggers glBlitFramebuffer to the resolve target
    sg_commit();
    T(gl_mock_count_calls(GL_MOCK_FUNC_glBlitFramebuffer) >= 1);
    sg_destroy_view(color_v);
    sg_destroy_view(resolve_v);
    sg_destroy_image(msaa);
    sg_destroy_image(resolve);
    T(sg_isvalid());
    teardown();
}

#if TEST_HAS_COMPUTE
UTEST(sokol_gfx_gl, compute_pass_no_framebuffer_bind) {
    setup();
    sg_shader shd = sg_make_shader(&(sg_shader_desc){
        .compute_func.source = "void main() { }",
    });
    sg_pipeline pip = sg_make_pipeline(&(sg_pipeline_desc){ .shader = shd, .compute = true });
    gl_mock_clear_calls();
    sg_begin_pass(&(sg_pass){ .compute = true });
    sg_apply_pipeline(pip);
    sg_dispatch(4, 4, 1);
    sg_end_pass();
    sg_commit();
    // compute pass does not touch framebuffer state
    T(gl_mock_count_calls(GL_MOCK_FUNC_glBindFramebuffer) == 0);
    T(gl_mock_count_calls(GL_MOCK_FUNC_glDispatchCompute) == 1);
    sg_destroy_pipeline(pip);
    sg_destroy_shader(shd);
    T(sg_isvalid());
    teardown();
}
#endif

//------------------------------------------------------------------------------
//  draw / dispatch code paths
//------------------------------------------------------------------------------
UTEST(sokol_gfx_gl, draw_arrays_non_indexed_and_instanced) {
    setup();
    sg_shader shd = make_test_shader();
    sg_pipeline pip = sg_make_pipeline(&(sg_pipeline_desc){
        .shader = shd,
        .layout.attrs[0].format = SG_VERTEXFORMAT_FLOAT3,
    });
    static const float verts[9] = {0};
    sg_buffer vbuf = sg_make_buffer(&(sg_buffer_desc){ .data = SG_RANGE(verts) });
    sg_begin_pass(&(sg_pass){
        .swapchain = { .width = 64, .height = 64, .sample_count = 1,
                       .color_format = SG_PIXELFORMAT_RGBA8, .depth_format = SG_PIXELFORMAT_DEPTH_STENCIL },
    });
    sg_apply_pipeline(pip);
    sg_apply_bindings(&(sg_bindings){ .vertex_buffers[0] = vbuf });
    gl_mock_clear_calls();
    sg_draw(0, 3, 1);                       // glDrawArrays
    sg_draw(0, 3, 4);                       // glDrawArraysInstanced
    sg_end_pass();
    sg_commit();
    T(gl_mock_count_calls(GL_MOCK_FUNC_glDrawArrays) == 1);
    T(gl_mock_count_calls(GL_MOCK_FUNC_glDrawArraysInstanced) == 1);
    sg_destroy_buffer(vbuf);
    sg_destroy_pipeline(pip);
    sg_destroy_shader(shd);
    T(sg_isvalid());
    teardown();
}

UTEST(sokol_gfx_gl, draw_indexed_and_instanced) {
    setup();
    sg_shader shd = make_test_shader();
    sg_pipeline pip = sg_make_pipeline(&(sg_pipeline_desc){
        .shader = shd, .index_type = SG_INDEXTYPE_UINT16,
        .layout.attrs[0].format = SG_VERTEXFORMAT_FLOAT3,
    });
    static const float verts[12] = {0};
    static const uint16_t indices[6] = { 0, 1, 2, 0, 2, 3 };
    sg_buffer vbuf = sg_make_buffer(&(sg_buffer_desc){ .data = SG_RANGE(verts) });
    sg_buffer ibuf = sg_make_buffer(&(sg_buffer_desc){
        .usage.index_buffer = true, .data = SG_RANGE(indices),
    });
    sg_begin_pass(&(sg_pass){
        .swapchain = { .width = 64, .height = 64, .sample_count = 1,
                       .color_format = SG_PIXELFORMAT_RGBA8, .depth_format = SG_PIXELFORMAT_DEPTH_STENCIL },
    });
    sg_apply_pipeline(pip);
    sg_apply_bindings(&(sg_bindings){ .vertex_buffers[0] = vbuf, .index_buffer = ibuf });
    gl_mock_clear_calls();
    sg_draw(0, 6, 1);
    sg_draw(0, 6, 4);
    sg_end_pass();
    sg_commit();
    // no base_vertex/base_instance in the bindings, so sokol_gfx picks the
    // simple GL entry points on every backend
    T(gl_mock_count_calls(GL_MOCK_FUNC_glDrawElements) == 1);
    T(gl_mock_count_calls(GL_MOCK_FUNC_glDrawElementsInstanced) == 1);
    sg_destroy_buffer(vbuf);
    sg_destroy_buffer(ibuf);
    sg_destroy_pipeline(pip);
    sg_destroy_shader(shd);
    T(sg_isvalid());
    teardown();
}

UTEST(sokol_gfx_gl, draw_ex_base_vertex_indexed) {
    setup();
    if (!sg_query_features().draw_base_vertex) {
        // GLES 3.0 and 3.1 have no ARB_draw_elements_base_vertex; sokol_gfx
        // falls back to plain glDrawElements and silently drops base_vertex
        teardown();
        return;
    }
    sg_shader shd = make_test_shader();
    sg_pipeline pip = sg_make_pipeline(&(sg_pipeline_desc){
        .shader = shd, .index_type = SG_INDEXTYPE_UINT16,
        .layout.attrs[0].format = SG_VERTEXFORMAT_FLOAT3,
    });
    static const float verts[12] = {0};
    static const uint16_t indices[6] = { 0, 1, 2, 0, 2, 3 };
    sg_buffer vbuf = sg_make_buffer(&(sg_buffer_desc){ .data = SG_RANGE(verts) });
    sg_buffer ibuf = sg_make_buffer(&(sg_buffer_desc){
        .usage.index_buffer = true, .data = SG_RANGE(indices),
    });
    sg_begin_pass(&(sg_pass){
        .swapchain = { .width = 64, .height = 64, .sample_count = 1,
                       .color_format = SG_PIXELFORMAT_RGBA8, .depth_format = SG_PIXELFORMAT_DEPTH_STENCIL },
    });
    sg_apply_pipeline(pip);
    sg_apply_bindings(&(sg_bindings){ .vertex_buffers[0] = vbuf, .index_buffer = ibuf });
    gl_mock_clear_calls();
    // base_vertex != 0, base_instance == 0
    sg_draw_ex(0, 6, 1, 1, 0);              // -> glDrawElementsBaseVertex
    sg_draw_ex(0, 6, 4, 1, 0);              // -> glDrawElementsInstancedBaseVertex
    sg_end_pass();
    sg_commit();
    T(gl_mock_count_calls(GL_MOCK_FUNC_glDrawElementsBaseVertex) == 1);
    T(gl_mock_count_calls(GL_MOCK_FUNC_glDrawElementsInstancedBaseVertex) == 1);
    // and the plain variants must NOT fire when base_vertex is non-zero
    T(gl_mock_count_calls(GL_MOCK_FUNC_glDrawElements) == 0);
    T(gl_mock_count_calls(GL_MOCK_FUNC_glDrawElementsInstanced) == 0);
    sg_destroy_buffer(vbuf);
    sg_destroy_buffer(ibuf);
    sg_destroy_pipeline(pip);
    sg_destroy_shader(shd);
    T(sg_isvalid());
    teardown();
}

UTEST(sokol_gfx_gl, draw_ex_base_instance_indexed) {
    setup();
    if (!sg_query_features().draw_base_instance) {
        // GLCORE < 4.2 and all GLES3 lack draw_base_instance; skip.
        teardown();
        return;
    }
    sg_shader shd = make_test_shader();
    sg_pipeline pip = sg_make_pipeline(&(sg_pipeline_desc){
        .shader = shd, .index_type = SG_INDEXTYPE_UINT16,
        .layout.attrs[0].format = SG_VERTEXFORMAT_FLOAT3,
    });
    static const float verts[12] = {0};
    static const uint16_t indices[6] = { 0, 1, 2, 0, 2, 3 };
    sg_buffer vbuf = sg_make_buffer(&(sg_buffer_desc){ .data = SG_RANGE(verts) });
    sg_buffer ibuf = sg_make_buffer(&(sg_buffer_desc){
        .usage.index_buffer = true, .data = SG_RANGE(indices),
    });
    sg_begin_pass(&(sg_pass){
        .swapchain = { .width = 64, .height = 64, .sample_count = 1,
                       .color_format = SG_PIXELFORMAT_RGBA8, .depth_format = SG_PIXELFORMAT_DEPTH_STENCIL },
    });
    sg_apply_pipeline(pip);
    sg_apply_bindings(&(sg_bindings){ .vertex_buffers[0] = vbuf, .index_buffer = ibuf });
    gl_mock_clear_calls();
    // base_instance != 0 always uses the *BaseVertexBaseInstance variant, even
    // when base_vertex == 0 (that branch is the sokol_gfx.h dispatch)
    sg_draw_ex(0, 6, 4, 0, 2);
    sg_end_pass();
    sg_commit();
    T(gl_mock_count_calls(GL_MOCK_FUNC_glDrawElementsInstancedBaseVertexBaseInstance) == 1);
    T(gl_mock_count_calls(GL_MOCK_FUNC_glDrawElementsInstanced) == 0);
    sg_destroy_buffer(vbuf);
    sg_destroy_buffer(ibuf);
    sg_destroy_pipeline(pip);
    sg_destroy_shader(shd);
    T(sg_isvalid());
    teardown();
}

UTEST(sokol_gfx_gl, draw_ex_base_instance_non_indexed) {
    setup();
    if (!sg_query_features().draw_base_instance) {
        teardown();
        return;
    }
    sg_shader shd = make_test_shader();
    sg_pipeline pip = sg_make_pipeline(&(sg_pipeline_desc){
        .shader = shd,
        .layout.attrs[0].format = SG_VERTEXFORMAT_FLOAT3,
    });
    static const float verts[9] = {0};
    sg_buffer vbuf = sg_make_buffer(&(sg_buffer_desc){ .data = SG_RANGE(verts) });
    sg_begin_pass(&(sg_pass){
        .swapchain = { .width = 64, .height = 64, .sample_count = 1,
                       .color_format = SG_PIXELFORMAT_RGBA8, .depth_format = SG_PIXELFORMAT_DEPTH_STENCIL },
    });
    sg_apply_pipeline(pip);
    sg_apply_bindings(&(sg_bindings){ .vertex_buffers[0] = vbuf });
    gl_mock_clear_calls();
    sg_draw_ex(0, 3, 4, 0, 2);
    sg_end_pass();
    sg_commit();
    T(gl_mock_count_calls(GL_MOCK_FUNC_glDrawArraysInstancedBaseInstance) == 1);
    T(gl_mock_count_calls(GL_MOCK_FUNC_glDrawArraysInstanced) == 0);
    sg_destroy_buffer(vbuf);
    sg_destroy_pipeline(pip);
    sg_destroy_shader(shd);
    T(sg_isvalid());
    teardown();
}

UTEST(sokol_gfx_gl, apply_uniforms_calls_glUniformXfv) {
    setup();
    sg_shader shd = sg_make_shader(&(sg_shader_desc){
        .vertex_func.source = "vs", .fragment_func.source = "fs",
        .uniform_blocks[0] = {
            .stage = SG_SHADERSTAGE_VERTEX,
            .size = 64,
            .layout = SG_UNIFORMLAYOUT_NATIVE,
            .glsl_uniforms = {
                [0] = { .type = SG_UNIFORMTYPE_MAT4, .glsl_name = "mvp" },
            },
        },
    });
    sg_pipeline pip = sg_make_pipeline(&(sg_pipeline_desc){
        .shader = shd,
        .layout.attrs[0].format = SG_VERTEXFORMAT_FLOAT3,
    });
    static const float verts[3] = {0};
    sg_buffer vbuf = sg_make_buffer(&(sg_buffer_desc){ .data = SG_RANGE(verts) });
    sg_begin_pass(&(sg_pass){
        .swapchain = { .width = 32, .height = 32, .sample_count = 1,
                       .color_format = SG_PIXELFORMAT_RGBA8, .depth_format = SG_PIXELFORMAT_DEPTH_STENCIL },
    });
    sg_apply_pipeline(pip);
    sg_apply_bindings(&(sg_bindings){ .vertex_buffers[0] = vbuf });
    float mvp[16] = {0};
    gl_mock_clear_calls();
    sg_apply_uniforms(0, &SG_RANGE(mvp));
    T(gl_mock_count_calls(GL_MOCK_FUNC_glUniformMatrix4fv) == 1);
    sg_draw(0, 3, 1);
    sg_end_pass();
    sg_commit();
    sg_destroy_buffer(vbuf);
    sg_destroy_pipeline(pip);
    sg_destroy_shader(shd);
    T(sg_isvalid());
    teardown();
}

//------------------------------------------------------------------------------
//  resource writes -- glBufferSubData, glTexSubImage2D
//------------------------------------------------------------------------------
UTEST(sokol_gfx_gl, buffer_write_transient_calls_bufferSubData) {
    setup();
    sg_buffer buf = sg_make_buffer(&(sg_buffer_desc){
        .size = 64, .usage.write_transient = true,
    });
    T(sg_query_buffer_state(buf) == SG_RESOURCESTATE_VALID);
    uint8_t bytes[64] = {0};
    gl_mock_clear_calls();
    sg_write_buffer_transient(&(sg_write_buffer_desc){ .src.data = SG_RANGE(bytes), .dst.buffer = buf });
    T(gl_mock_count_calls(GL_MOCK_FUNC_glBufferSubData) == 1);
    const gl_mock_call_t* call = gl_mock_last_call(GL_MOCK_FUNC_glBufferSubData);
    T(call->args[1].i == 0);                    // offset
    T(call->args[2].i == (int64_t)sizeof(bytes));
    sg_destroy_buffer(buf);
    teardown();
}

UTEST(sokol_gfx_gl, buffer_write_transient_uses_dst_offset) {
    setup();
    sg_buffer buf = sg_make_buffer(&(sg_buffer_desc){
        .size = 256, .usage.write_transient = true,
    });
    uint8_t bytes[32] = {0};
    sg_write_buffer_transient(&(sg_write_buffer_desc){ .src.data = SG_RANGE(bytes), .dst = { .buffer = buf, .offset = 0 } });
    sg_write_buffer_transient(&(sg_write_buffer_desc){ .src.data = SG_RANGE(bytes), .dst = { .buffer = buf, .offset = 32 } });
    // second write uses non-zero offset in glBufferSubData
    const gl_mock_call_t* call = gl_mock_last_call(GL_MOCK_FUNC_glBufferSubData);
    TA(call != 0);
    T(call->args[1].i == 32);
    sg_destroy_buffer(buf);
    teardown();
}

UTEST(sokol_gfx_gl, image_write_transient_calls_texSubImage2D) {
    setup();
    sg_image img = sg_make_image(&(sg_image_desc){
        .width = 8, .height = 8, .pixel_format = SG_PIXELFORMAT_RGBA8,
        .usage.write_transient = true,
    });
    uint32_t pixels[8 * 8] = {0};
    gl_mock_clear_calls();
    sg_write_image_transient(&(sg_write_image_desc){ .src.data = SG_RANGE(pixels), .dst.image = img });
    T(gl_mock_count_calls(GL_MOCK_FUNC_glTexSubImage2D) == 1);
    sg_destroy_image(img);
    T(sg_isvalid());
    teardown();
}

//------------------------------------------------------------------------------
//  bindings + memory barriers
//------------------------------------------------------------------------------
UTEST(sokol_gfx_gl, apply_bindings_binds_vertex_buffer) {
    setup();
    sg_shader shd = make_test_shader();
    sg_pipeline pip = sg_make_pipeline(&(sg_pipeline_desc){
        .shader = shd, .layout.attrs[0].format = SG_VERTEXFORMAT_FLOAT3,
    });
    static const float verts[3] = {0};
    sg_buffer vbuf = sg_make_buffer(&(sg_buffer_desc){ .data = SG_RANGE(verts) });
    sg_begin_pass(&(sg_pass){
        .swapchain = { .width = 32, .height = 32, .sample_count = 1,
                       .color_format = SG_PIXELFORMAT_RGBA8, .depth_format = SG_PIXELFORMAT_DEPTH_STENCIL },
    });
    sg_apply_pipeline(pip);
    gl_mock_clear_calls();
    sg_apply_bindings(&(sg_bindings){ .vertex_buffers[0] = vbuf });
    // vertex buffer binding routes through glVertexAttribPointer +
    // glEnableVertexAttribArray. glBindBuffer is cached and may be a no-op
    // if the target buffer is already bound after buffer creation.
    T(gl_mock_count_calls(GL_MOCK_FUNC_glVertexAttribPointer) >= 1);
    T(gl_mock_count_calls(GL_MOCK_FUNC_glEnableVertexAttribArray) >= 1);
    sg_end_pass();
    sg_commit();
    sg_destroy_buffer(vbuf);
    sg_destroy_pipeline(pip);
    sg_destroy_shader(shd);
    teardown();
}

#if TEST_HAS_COMPUTE
UTEST(sokol_gfx_gl, storage_image_binding_uses_bindImageTexture) {
    setup();
    sg_shader shd = sg_make_shader(&(sg_shader_desc){
        .compute_func.source = "cs",
        .views[0].storage_image = {
            .stage = SG_SHADERSTAGE_COMPUTE,
            .image_type = SG_IMAGETYPE_2D,
            .access_format = SG_PIXELFORMAT_RGBA8,
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
    gl_mock_clear_calls();
    sg_apply_bindings(&(sg_bindings){ .views[0] = uav });
    T(gl_mock_count_calls(GL_MOCK_FUNC_glBindImageTexture) == 1);
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
#endif

//------------------------------------------------------------------------------
//  buffer usage combinations -- GL target, GL usage hint, slot count
//------------------------------------------------------------------------------

// index of the first call to 'func' at or after 'start' whose first argument
// equals 'arg0', or -1
static int find_call_arg0(gl_mock_func_t func, int start, int64_t arg0) {
    int i = start;
    while ((i = gl_mock_find_call(func, i)) >= 0) {
        if (gl_mock_call(i)->args[0].i == arg0) {
            return i;
        }
        i++;
    }
    return -1;
}

typedef struct {
    sg_buffer_usage usage;
    bool with_data;
    bool needs_compute;
    GLenum gl_target;
    GLenum gl_usage;
    int num_slots;
} buffer_usage_case_t;

UTEST(sokol_gfx_gl, buffer_usage_combinations) {
    static const uint32_t data[16] = { 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16 };
    const buffer_usage_case_t cases[] = {
        { .usage = { .vertex_buffer = true }, .with_data = true, .gl_target = GL_ARRAY_BUFFER, .gl_usage = GL_STATIC_DRAW, .num_slots = 1 },
        { .usage = { .index_buffer = true }, .with_data = true, .gl_target = GL_ELEMENT_ARRAY_BUFFER, .gl_usage = GL_STATIC_DRAW, .num_slots = 1 },
        // vertex_buffer takes precedence when a buffer has several types
        { .usage = { .vertex_buffer = true, .index_buffer = true }, .with_data = true, .gl_target = GL_ARRAY_BUFFER, .gl_usage = GL_STATIC_DRAW, .num_slots = 1 },
        { .usage = { .vertex_buffer = true, .copy_src = true }, .with_data = true, .gl_target = GL_ARRAY_BUFFER, .gl_usage = GL_STATIC_DRAW, .num_slots = 1 },
        { .usage = { .vertex_buffer = true, .write_transient = true }, .gl_target = GL_ARRAY_BUFFER, .gl_usage = GL_STREAM_DRAW, .num_slots = SG_NUM_INFLIGHT_FRAMES },
        { .usage = { .index_buffer = true, .write_transient = true }, .gl_target = GL_ELEMENT_ARRAY_BUFFER, .gl_usage = GL_STREAM_DRAW, .num_slots = SG_NUM_INFLIGHT_FRAMES },
        { .usage = { .vertex_buffer = true, .write_unsealed = true }, .gl_target = GL_ARRAY_BUFFER, .gl_usage = GL_STATIC_DRAW, .num_slots = 1 },
        { .usage = { .index_buffer = true, .write_unsealed = true }, .gl_target = GL_ELEMENT_ARRAY_BUFFER, .gl_usage = GL_STATIC_DRAW, .num_slots = 1 },
        { .usage = { .vertex_buffer = true, .copy_dst = true }, .gl_target = GL_ARRAY_BUFFER, .gl_usage = GL_DYNAMIC_COPY, .num_slots = 1 },
        { .usage = { .index_buffer = true, .copy_dst = true, .copy_src = true }, .gl_target = GL_ELEMENT_ARRAY_BUFFER, .gl_usage = GL_DYNAMIC_COPY, .num_slots = 1 },
        { .usage = { .staging_buffer = true, .copy_src = true }, .gl_target = GL_ARRAY_BUFFER, .gl_usage = GL_STATIC_DRAW, .num_slots = 1 },
        { .usage = { .staging_index_buffer = true, .copy_src = true }, .gl_target = GL_ELEMENT_ARRAY_BUFFER, .gl_usage = GL_STATIC_DRAW, .num_slots = 1 },
        { .usage = { .storage_buffer = true }, .with_data = true, .needs_compute = true, .gl_target = GL_SHADER_STORAGE_BUFFER, .gl_usage = GL_STATIC_DRAW, .num_slots = 1 },
        { .usage = { .storage_buffer = true, .write_transient = true }, .needs_compute = true, .gl_target = GL_SHADER_STORAGE_BUFFER, .gl_usage = GL_STREAM_DRAW, .num_slots = SG_NUM_INFLIGHT_FRAMES },
        { .usage = { .storage_buffer = true, .copy_dst = true }, .needs_compute = true, .gl_target = GL_SHADER_STORAGE_BUFFER, .gl_usage = GL_DYNAMIC_COPY, .num_slots = 1 },
    };
    for (size_t ci = 0; ci < sizeof(cases) / sizeof(cases[0]); ci++) {
        const buffer_usage_case_t* c = &cases[ci];
        if (c->needs_compute && !TEST_HAS_COMPUTE) {
            continue;
        }
        setup();
        gl_mock_clear_calls();
        sg_buffer_desc desc = { .usage = c->usage, .size = sizeof(data) };
        if (c->with_data) {
            desc.data = (sg_range)SG_RANGE(data);
        }
        sg_buffer buf = sg_make_buffer(&desc);
        const sg_resource_state expected_state = c->usage.write_unsealed ? SG_RESOURCESTATE_UNSEALED : SG_RESOURCESTATE_VALID;
        T(sg_query_buffer_state(buf) == expected_state);
        T(sg_query_buffer_info(buf).num_slots == c->num_slots);
        T(gl_mock_live_objects(GL_MOCK_OBJ_BUFFER) == c->num_slots);
        T(gl_mock_count_calls(GL_MOCK_FUNC_glGenBuffers) == c->num_slots);
        T(gl_mock_count_calls(GL_MOCK_FUNC_glBufferData) == c->num_slots);
        T(gl_mock_count_calls(GL_MOCK_FUNC_glBufferSubData) == (c->with_data ? c->num_slots : 0));
        const sg_gl_buffer_info gl_info = sg_gl_query_buffer_info(buf);
        int call_idx = 0;
        for (int slot = 0; slot < c->num_slots; slot++) {
            const GLuint gl_buf = gl_info.buf[slot];
            T(gl_buf != 0);
            // each slot: bind, allocate with glBufferData(data=0), then upload with glBufferSubData
            const int bind_idx = find_call_arg0(GL_MOCK_FUNC_glBindBuffer, call_idx, c->gl_target);
            TA(bind_idx >= 0);
            T(gl_mock_call(bind_idx)->args[1].i == gl_buf);
            const int data_idx = gl_mock_find_call(GL_MOCK_FUNC_glBufferData, bind_idx);
            TA(data_idx > bind_idx);
            const gl_mock_call_t* bd = gl_mock_call(data_idx);
            T(bd->args[0].i == c->gl_target);
            T(bd->args[1].i == (int64_t)sizeof(data));
            T(bd->args[2].p == 0);
            T(bd->args[3].i == c->gl_usage);
            call_idx = data_idx + 1;
            if (c->with_data) {
                const int sub_idx = gl_mock_find_call(GL_MOCK_FUNC_glBufferSubData, data_idx);
                TA(sub_idx == data_idx + 1);
                const gl_mock_call_t* bsd = gl_mock_call(sub_idx);
                T(bsd->args[0].i == c->gl_target);
                T(bsd->args[1].i == 0);
                T(bsd->args[2].i == (int64_t)sizeof(data));
                T(bsd->args[3].p == data);
                call_idx = sub_idx + 1;
            }
            gl_mock_buffer_info_t mock_info;
            TA(gl_mock_buffer_info(gl_buf, &mock_info));
            T(mock_info.target == c->gl_target);
            T(mock_info.usage == c->gl_usage);
            T(mock_info.size == (GLsizeiptr)sizeof(data));
        }
        sg_destroy_buffer(buf);
        T(gl_mock_count_calls(GL_MOCK_FUNC_glDeleteBuffers) == c->num_slots);
        T(gl_mock_live_objects(GL_MOCK_OBJ_BUFFER) == 0);
        T(no_errors());
    teardown();
    }
}

UTEST(sokol_gfx_gl, buffer_create_restores_previous_binding) {
    setup();
    static const float data[4] = { 0 };
    sg_buffer a = sg_make_buffer(&(sg_buffer_desc){ .data = SG_RANGE(data) });
    const GLuint gl_a = sg_gl_query_buffer_info(a).buf[0];
    // after the first create, the new buffer stays bound in the cache
    T(gl_mock_bindings()->array_buffer == gl_a);
    gl_mock_clear_calls();
    sg_buffer b = sg_make_buffer(&(sg_buffer_desc){ .data = SG_RANGE(data) });
    const GLuint gl_b = sg_gl_query_buffer_info(b).buf[0];
    // the second create binds b, uploads, then restores a
    TA(gl_mock_count_calls(GL_MOCK_FUNC_glBindBuffer) == 2);
    T(gl_mock_call(gl_mock_find_call(GL_MOCK_FUNC_glBindBuffer, 0))->args[1].i == gl_b);
    T(gl_mock_last_call(GL_MOCK_FUNC_glBindBuffer)->args[1].i == gl_a);
    T(gl_mock_bindings()->array_buffer == gl_a);
    // index buffers have their own bind point and stored binding
    static const uint16_t idata[4] = { 0 };
    sg_buffer c = sg_make_buffer(&(sg_buffer_desc){ .usage.index_buffer = true, .data = SG_RANGE(idata) });
    sg_buffer d = sg_make_buffer(&(sg_buffer_desc){ .usage.index_buffer = true, .data = SG_RANGE(idata) });
    T(gl_mock_bindings()->element_array_buffer == sg_gl_query_buffer_info(c).buf[0]);
    T(gl_mock_bindings()->array_buffer == gl_a);
    // destroying a bound buffer drops it from the cache, so the next create
    // has nothing to restore
    sg_destroy_buffer(a);
    sg_destroy_buffer(c);
    gl_mock_clear_calls();
    sg_buffer e = sg_make_buffer(&(sg_buffer_desc){ .data = SG_RANGE(data) });
    T(gl_mock_count_calls(GL_MOCK_FUNC_glBindBuffer) == 1);
    sg_destroy_buffer(b);
    sg_destroy_buffer(d);
    sg_destroy_buffer(e);
    T(no_errors());
    teardown();
}

#if TEST_HAS_COMPUTE
UTEST(sokol_gfx_gl, storage_buffer_create_restores_previous_binding) {
    setup();
    static const uint32_t data[4] = { 0 };
    sg_buffer a = sg_make_buffer(&(sg_buffer_desc){ .usage.storage_buffer = true, .data = SG_RANGE(data) });
    sg_buffer b = sg_make_buffer(&(sg_buffer_desc){ .usage.storage_buffer = true, .data = SG_RANGE(data) });
    T(gl_mock_bindings()->shader_storage_buffer == sg_gl_query_buffer_info(a).buf[0]);
    sg_destroy_buffer(a);
    sg_destroy_buffer(b);
    T(no_errors());
    teardown();
}
#endif

//------------------------------------------------------------------------------
//  buffer writes -- write_transient (slot rotation) and write_unsealed + seal
//------------------------------------------------------------------------------
UTEST(sokol_gfx_gl, buffer_write_transient_rotates_slots) {
    setup();
    sg_buffer buf = sg_make_buffer(&(sg_buffer_desc){ .size = 64, .usage.write_transient = true });
    const sg_gl_buffer_info info0 = sg_gl_query_buffer_info(buf);
    T(info0.active_slot == 0);
    T(info0.buf[0] != info0.buf[1]);
    uint8_t bytes[32];
    for (int i = 0; i < 32; i++) { bytes[i] = (uint8_t)i; }

    // first write in a frame rotates to the next slot
    gl_mock_clear_calls();
    sg_write_buffer_transient(&(sg_write_buffer_desc){
        .src = { .data = SG_RANGE(bytes), .offset = 8 },
        .dst = { .buffer = buf, .offset = 16 },
        .size = 16,
    });
    T(sg_gl_query_buffer_info(buf).active_slot == 1);
    const int bind_idx = find_call_arg0(GL_MOCK_FUNC_glBindBuffer, 0, GL_ARRAY_BUFFER);
    TA(bind_idx >= 0);
    T(gl_mock_call(bind_idx)->args[1].i == info0.buf[1]);
    const int sub_idx = gl_mock_find_call(GL_MOCK_FUNC_glBufferSubData, bind_idx);
    TA(sub_idx > bind_idx);
    const gl_mock_call_t* sub = gl_mock_call(sub_idx);
    T(sub->args[0].i == GL_ARRAY_BUFFER);
    T(sub->args[1].i == 16);
    T(sub->args[2].i == 16);
    T(sub->args[3].p == &bytes[8]);
    gl_mock_buffer_info_t mi;
    TA(gl_mock_buffer_info(info0.buf[1], &mi));
    T(mi.num_subdata == 1);
    TA(gl_mock_buffer_info(info0.buf[0], &mi));
    T(mi.num_subdata == 0);

    // a second write in the same frame stays in the same slot, size defaults to src size
    gl_mock_clear_calls();
    sg_write_buffer_transient(&(sg_write_buffer_desc){ .src.data = SG_RANGE(bytes), .dst = { .buffer = buf, .offset = 32 } });
    T(sg_gl_query_buffer_info(buf).active_slot == 1);
    sub = gl_mock_last_call(GL_MOCK_FUNC_glBufferSubData);
    TA(sub != 0);
    T(sub->args[1].i == 32);
    T(sub->args[2].i == (int64_t)sizeof(bytes));
    T(sub->args[3].p == bytes);
    // bind slot 1 for the write, then restore the previously bound slot 0
    // (creating the 2 slots left slot 0 bound)
    TA(gl_mock_count_calls(GL_MOCK_FUNC_glBindBuffer) == 2);
    T(gl_mock_call(gl_mock_find_call(GL_MOCK_FUNC_glBindBuffer, 0))->args[1].i == info0.buf[1]);
    T(gl_mock_last_call(GL_MOCK_FUNC_glBindBuffer)->args[1].i == info0.buf[0]);
    T(gl_mock_bindings()->array_buffer == info0.buf[0]);

    // the next frame wraps around to slot 0
    sg_commit();
    gl_mock_clear_calls();
    sg_write_buffer_transient(&(sg_write_buffer_desc){ .src.data = SG_RANGE(bytes), .dst.buffer = buf });
    T(sg_gl_query_buffer_info(buf).active_slot == 0);
    T(gl_mock_bindings()->array_buffer == info0.buf[0]);
    TA(gl_mock_buffer_info(info0.buf[0], &mi));
    T(mi.num_subdata == 1);
    sg_destroy_buffer(buf);
    T(no_errors());
    teardown();
}

UTEST(sokol_gfx_gl, index_buffer_write_transient_uses_element_array_target) {
    setup();
    sg_buffer buf = sg_make_buffer(&(sg_buffer_desc){ .size = 64, .usage = { .index_buffer = true, .write_transient = true } });
    uint16_t indices[8] = { 0 };
    gl_mock_clear_calls();
    sg_write_buffer_transient(&(sg_write_buffer_desc){ .src.data = SG_RANGE(indices), .dst.buffer = buf });
    const gl_mock_call_t* sub = gl_mock_last_call(GL_MOCK_FUNC_glBufferSubData);
    TA(sub != 0);
    T(sub->args[0].i == GL_ELEMENT_ARRAY_BUFFER);
    const int bind_idx = find_call_arg0(GL_MOCK_FUNC_glBindBuffer, 0, GL_ELEMENT_ARRAY_BUFFER);
    TA(bind_idx >= 0);
    T(gl_mock_call(bind_idx)->args[1].i == sg_gl_query_buffer_info(buf).buf[1]);
    // the previous index buffer binding is restored
    T(gl_mock_bindings()->element_array_buffer == sg_gl_query_buffer_info(buf).buf[0]);
    sg_destroy_buffer(buf);
    T(no_errors());
    teardown();
}

UTEST(sokol_gfx_gl, buffer_write_unsealed_then_seal) {
    setup();
    sg_buffer buf = sg_make_buffer(&(sg_buffer_desc){ .size = 64, .usage.write_unsealed = true });
    T(sg_query_buffer_state(buf) == SG_RESOURCESTATE_UNSEALED);
    const GLuint gl_buf = sg_gl_query_buffer_info(buf).buf[0];
    uint8_t part0[32] = { 0 };
    uint8_t part1[32] = { 0 };
    gl_mock_clear_calls();
    sg_write_buffer_unsealed(&(sg_write_buffer_desc){ .src.data = SG_RANGE(part0), .dst.buffer = buf });
    sg_write_buffer_unsealed(&(sg_write_buffer_desc){ .src.data = SG_RANGE(part1), .dst = { .buffer = buf, .offset = 32 } });
    TA(gl_mock_count_calls(GL_MOCK_FUNC_glBufferSubData) == 2);
    const int first = gl_mock_find_call(GL_MOCK_FUNC_glBufferSubData, 0);
    const gl_mock_call_t* c0 = gl_mock_call(first);
    const gl_mock_call_t* c1 = gl_mock_call(gl_mock_find_call(GL_MOCK_FUNC_glBufferSubData, first + 1));
    T(c0->args[0].i == GL_ARRAY_BUFFER);
    T(c0->args[1].i == 0);
    T(c0->args[2].i == 32);
    T(c0->args[3].p == part0);
    T(c1->args[1].i == 32);
    T(c1->args[2].i == 32);
    T(c1->args[3].p == part1);
    T(gl_mock_bindings()->array_buffer == gl_buf);
    // an unsealed buffer is never renamed
    T(sg_gl_query_buffer_info(buf).active_slot == 0);

    // sealing is a pure state change in the GL backend, no GL calls
    gl_mock_clear_calls();
    sg_seal_buffer(buf);
    T(sg_query_buffer_state(buf) == SG_RESOURCESTATE_VALID);
    T(gl_mock_num_calls() == 0);
    sg_destroy_buffer(buf);
    T(no_errors());
    teardown();
}

//------------------------------------------------------------------------------
//  image writes -- write_transient (slot rotation) and write_unsealed + seal
//------------------------------------------------------------------------------
UTEST(sokol_gfx_gl, image_write_transient_rotates_slots_and_sets_unpack_state) {
    setup();
    sg_image img = sg_make_image(&(sg_image_desc){
        .width = 8, .height = 8, .num_mipmaps = 2,
        .pixel_format = SG_PIXELFORMAT_RGBA8,
        .usage.write_transient = true,
    });
    T(sg_query_image_state(img) == SG_RESOURCESTATE_VALID);
    const sg_gl_image_info info0 = sg_gl_query_image_info(img);
    T(info0.tex_target == GL_TEXTURE_2D);
    T(info0.active_slot == 0);
    T(gl_mock_live_objects(GL_MOCK_OBJ_TEXTURE) == SG_NUM_INFLIGHT_FRAMES);
    #if !TEST_HAS_TEXSTORAGE
        // without glTexStorage the max mip level is set explicitly
        T(gl_mock_count_calls(GL_MOCK_FUNC_glTexParameteri) >= SG_NUM_INFLIGHT_FRAMES);
        T(gl_mock_count_calls(GL_MOCK_FUNC_glTexImage2D) == 2 * SG_NUM_INFLIGHT_FRAMES);
    #else
        T(gl_mock_count_calls(GL_MOCK_FUNC_glTexStorage2D) == SG_NUM_INFLIGHT_FRAMES);
    #endif

    // write a 2x2 region into mip level 1 (4x4), source rows are 4 pixels wide
    uint32_t pixels[4 * 4] = { 0 };
    gl_mock_clear_calls();
    sg_write_image_transient(&(sg_write_image_desc){
        .src.data = SG_RANGE(pixels),
        .dst = { .image = img, .mip_level = 1, .x = 1, .y = 2 },
        .size = { .width = 2, .height = 2 },
    });
    T(sg_gl_query_image_info(img).active_slot == 1);
    const int tbind_idx = gl_mock_find_call(GL_MOCK_FUNC_glBindTexture, 0);
    TA(tbind_idx >= 0);
    T(gl_mock_call(tbind_idx)->args[0].i == GL_TEXTURE_2D);
    T(gl_mock_call(tbind_idx)->args[1].i == info0.tex[1]);
    const int row_idx = find_call_arg0(GL_MOCK_FUNC_glPixelStorei, 0, GL_UNPACK_ROW_LENGTH);
    const int hgt_idx = find_call_arg0(GL_MOCK_FUNC_glPixelStorei, 0, GL_UNPACK_IMAGE_HEIGHT);
    const int sub_idx = gl_mock_find_call(GL_MOCK_FUNC_glTexSubImage2D, 0);
    TA((row_idx >= 0) && (hgt_idx >= 0) && (sub_idx >= 0));
    T(row_idx < sub_idx);
    T(hgt_idx < sub_idx);
    T(gl_mock_call(row_idx)->args[1].i == 4);
    T(gl_mock_call(hgt_idx)->args[1].i == 4);
    const gl_mock_call_t* sub = gl_mock_call(sub_idx);
    T(sub->args[0].i == GL_TEXTURE_2D);
    T(sub->args[1].i == 1);     // mip level
    T(sub->args[2].i == 1);     // x
    T(sub->args[3].i == 2);     // y
    T(sub->args[4].i == 2);     // width
    T(sub->args[5].i == 2);     // height
    T(sub->args[6].i == GL_RGBA);
    T(sub->args[7].i == GL_UNSIGNED_BYTE);
    T(sub->args[8].p == pixels);
    // unpack state is reset afterwards
    T(gl_mock_render_state()->unpack_row_length == 0);
    T(gl_mock_render_state()->unpack_image_height == 0);
    gl_mock_texture_info_t ti;
    TA(gl_mock_texture_info(info0.tex[1], &ti));
    T(ti.num_subimage == 1);
    TA(gl_mock_texture_info(info0.tex[0], &ti));
    T(ti.num_subimage == 0);

    // the next frame wraps around to slot 0
    sg_commit();
    sg_write_image_transient(&(sg_write_image_desc){ .src.data = SG_RANGE(pixels), .dst = { .image = img, .mip_level = 1 } });
    T(sg_gl_query_image_info(img).active_slot == 0);
    TA(gl_mock_texture_info(info0.tex[0], &ti));
    T(ti.num_subimage == 1);
    sg_destroy_image(img);
    T(gl_mock_live_objects(GL_MOCK_OBJ_TEXTURE) == 0);
    T(no_errors());
    teardown();
}

UTEST(sokol_gfx_gl, image_write_transient_cube_writes_each_face) {
    setup();
    sg_image img = sg_make_image(&(sg_image_desc){
        .type = SG_IMAGETYPE_CUBE,
        .width = 4, .height = 4,
        .pixel_format = SG_PIXELFORMAT_RGBA8,
        .usage.write_transient = true,
    });
    T(sg_query_image_state(img) == SG_RESOURCESTATE_VALID);
    static uint8_t pixels[6 * 4 * 4 * 4];
    gl_mock_clear_calls();
    // all six faces
    sg_write_image_transient(&(sg_write_image_desc){ .src.data = SG_RANGE(pixels), .dst.image = img });
    TA(gl_mock_count_calls(GL_MOCK_FUNC_glTexSubImage2D) == 6);
    int idx = 0;
    for (int face = 0; face < 6; face++) {
        idx = gl_mock_find_call(GL_MOCK_FUNC_glTexSubImage2D, idx);
        TA(idx >= 0);
        const gl_mock_call_t* c = gl_mock_call(idx++);
        T(c->args[0].i == GL_TEXTURE_CUBE_MAP_POSITIVE_X + face);
        T(c->args[8].p == &pixels[face * 64]);
    }
    // faces 2 and 3 only
    gl_mock_clear_calls();
    sg_write_image_transient(&(sg_write_image_desc){
        .src.data = { .ptr = pixels, .size = 2 * 64 },
        .dst = { .image = img, .slice = 2 },
        .size.num_slices = 2,
    });
    TA(gl_mock_count_calls(GL_MOCK_FUNC_glTexSubImage2D) == 2);
    idx = gl_mock_find_call(GL_MOCK_FUNC_glTexSubImage2D, 0);
    T(gl_mock_call(idx)->args[0].i == GL_TEXTURE_CUBE_MAP_POSITIVE_X + 2);
    T(gl_mock_call(idx)->args[8].p == &pixels[0]);
    idx = gl_mock_find_call(GL_MOCK_FUNC_glTexSubImage2D, idx + 1);
    T(gl_mock_call(idx)->args[0].i == GL_TEXTURE_CUBE_MAP_POSITIVE_X + 3);
    T(gl_mock_call(idx)->args[8].p == &pixels[64]);
    sg_destroy_image(img);
    T(no_errors());
    teardown();
}

UTEST(sokol_gfx_gl, image_write_transient_array_and_3d_use_texsubimage3d) {
    setup();
    const sg_image_type types[2] = { SG_IMAGETYPE_ARRAY, SG_IMAGETYPE_3D };
    const GLenum targets[2] = { GL_TEXTURE_2D_ARRAY, GL_TEXTURE_3D };
    static uint8_t pixels[2 * 8 * 8 * 4];
    for (int i = 0; i < 2; i++) {
        sg_image img = sg_make_image(&(sg_image_desc){
            .type = types[i],
            .width = 4, .height = 4, .num_slices = 4,
            .pixel_format = SG_PIXELFORMAT_RGBA8,
            .usage.write_transient = true,
        });
        T(sg_query_image_state(img) == SG_RESOURCESTATE_VALID);
        T(sg_gl_query_image_info(img).tex_target == targets[i]);
        gl_mock_clear_calls();
        // write slices 1 and 2, source rows are padded to 8 pixels, source slices to 8 rows
        sg_write_image_transient(&(sg_write_image_desc){
            .src = { .data = SG_RANGE(pixels), .bytes_per_row = 32, .bytes_per_slice = 8 * 32 },
            .dst = { .image = img, .slice = 1 },
            .size = { .num_slices = 2 },
        });
        const gl_mock_call_t* sub = gl_mock_last_call(GL_MOCK_FUNC_glTexSubImage3D);
        TA(sub != 0);
        T(sub->args[0].i == targets[i]);
        T(sub->args[1].i == 0);     // mip level
        T(sub->args[4].i == 1);     // zoffset == slice
        T(sub->args[5].i == 4);     // width
        T(sub->args[6].i == 4);     // height
        T(sub->args[7].i == 2);     // depth == num_slices
        T(sub->args[10].p == pixels);
        const int row_idx = find_call_arg0(GL_MOCK_FUNC_glPixelStorei, 0, GL_UNPACK_ROW_LENGTH);
        const int hgt_idx = find_call_arg0(GL_MOCK_FUNC_glPixelStorei, 0, GL_UNPACK_IMAGE_HEIGHT);
        TA((row_idx >= 0) && (hgt_idx >= 0));
        T(gl_mock_call(row_idx)->args[1].i == 8);
        T(gl_mock_call(hgt_idx)->args[1].i == 8);
        sg_destroy_image(img);
    }
    T(no_errors());
    teardown();
}

UTEST(sokol_gfx_gl, image_write_unsealed_then_seal) {
    setup();
    sg_image img = sg_make_image(&(sg_image_desc){
        .width = 8, .height = 8,
        .pixel_format = SG_PIXELFORMAT_RGBA8,
        .usage.write_unsealed = true,
    });
    T(sg_query_image_state(img) == SG_RESOURCESTATE_UNSEALED);
    // write_unsealed implies immutable, so a single texture
    T(gl_mock_live_objects(GL_MOCK_OBJ_TEXTURE) == 1);
    // the texture is allocated without data
    #if TEST_HAS_TEXSTORAGE
        T(gl_mock_count_calls(GL_MOCK_FUNC_glTexStorage2D) == 1);
        T(gl_mock_count_calls(GL_MOCK_FUNC_glTexSubImage2D) == 0);
    #else
        const gl_mock_call_t* ti = gl_mock_last_call(GL_MOCK_FUNC_glTexImage2D);
        TA(ti != 0);
        T(ti->args[8].p == 0);
    #endif
    const GLuint tex = sg_gl_query_image_info(img).tex[0];
    uint32_t top[8 * 4] = { 0 };
    uint32_t bottom[8 * 4] = { 0 };
    gl_mock_clear_calls();
    // a partial-height write needs an explicit bytes_per_slice (default is the whole mip surface)
    sg_write_image_unsealed(&(sg_write_image_desc){ .src = { .data = SG_RANGE(top), .bytes_per_slice = sizeof(top) }, .dst.image = img, .size.height = 4 });
    sg_write_image_unsealed(&(sg_write_image_desc){ .src = { .data = SG_RANGE(bottom), .bytes_per_slice = sizeof(bottom) }, .dst = { .image = img, .y = 4 } });
    TA(gl_mock_count_calls(GL_MOCK_FUNC_glTexSubImage2D) == 2);
    const gl_mock_call_t* sub = gl_mock_last_call(GL_MOCK_FUNC_glTexSubImage2D);
    T(sub->args[3].i == 4);     // y
    T(sub->args[5].i == 4);     // height
    T(sub->args[8].p == bottom);
    gl_mock_texture_info_t mi;
    TA(gl_mock_texture_info(tex, &mi));
    T(mi.num_subimage == 2);
    T(mi.num_subimage_unpack_buffer == 0);

    gl_mock_clear_calls();
    sg_seal_image(img);
    T(sg_query_image_state(img) == SG_RESOURCESTATE_VALID);
    T(gl_mock_num_calls() == 0);
    sg_destroy_image(img);
    T(no_errors());
    teardown();
}

UTEST(sokol_gfx_gl, image_write_unsealed_compressed) {
    setup();
    // BC1: 8 bytes per 4x4 block
    static uint8_t blocks[6 * 32];
    // 2D: glCompressedTexSubImage2D with the block-compressed size of the region
    sg_image img2d = sg_make_image(&(sg_image_desc){
        .width = 8, .height = 8,
        .pixel_format = SG_PIXELFORMAT_BC1_RGBA,
        .usage.write_unsealed = true,
    });
    T(sg_query_image_state(img2d) == SG_RESOURCESTATE_UNSEALED);
    gl_mock_clear_calls();
    sg_write_image_unsealed(&(sg_write_image_desc){ .src.data = { .ptr = blocks, .size = 32 }, .dst.image = img2d });
    const gl_mock_call_t* c = gl_mock_last_call(GL_MOCK_FUNC_glCompressedTexSubImage2D);
    TA(c != 0);
    T(c->args[0].i == GL_TEXTURE_2D);
    T(c->args[4].i == 8);
    T(c->args[5].i == 8);
    T(c->args[7].i == 32);
    T(c->args[8].p == blocks);
    T(gl_mock_count_calls(GL_MOCK_FUNC_glTexSubImage2D) == 0);
    sg_seal_image(img2d);

    // array: glCompressedTexSubImage3D, size covers all written slices
    sg_image img_arr = sg_make_image(&(sg_image_desc){
        .type = SG_IMAGETYPE_ARRAY,
        .width = 8, .height = 8, .num_slices = 2,
        .pixel_format = SG_PIXELFORMAT_BC1_RGBA,
        .usage.write_unsealed = true,
    });
    gl_mock_clear_calls();
    sg_write_image_unsealed(&(sg_write_image_desc){ .src.data = { .ptr = blocks, .size = 64 }, .dst.image = img_arr });
    c = gl_mock_last_call(GL_MOCK_FUNC_glCompressedTexSubImage3D);
    TA(c != 0);
    T(c->args[0].i == GL_TEXTURE_2D_ARRAY);
    T(c->args[7].i == 2);       // depth
    T(c->args[9].i == 64);      // imageSize
    T(c->args[10].p == blocks);

    // cube: one glCompressedTexSubImage2D per face
    sg_image img_cube = sg_make_image(&(sg_image_desc){
        .type = SG_IMAGETYPE_CUBE,
        .width = 8, .height = 8,
        .pixel_format = SG_PIXELFORMAT_BC1_RGBA,
        .usage.write_unsealed = true,
    });
    gl_mock_clear_calls();
    sg_write_image_unsealed(&(sg_write_image_desc){ .src.data = SG_RANGE(blocks), .dst.image = img_cube });
    T(gl_mock_count_calls(GL_MOCK_FUNC_glCompressedTexSubImage2D) == 6);
    c = gl_mock_last_call(GL_MOCK_FUNC_glCompressedTexSubImage2D);
    TA(c != 0);
    T(c->args[0].i == GL_TEXTURE_CUBE_MAP_NEGATIVE_Z);
    T(c->args[8].p == &blocks[5 * 32]);
    sg_destroy_image(img2d);
    sg_destroy_image(img_arr);
    sg_destroy_image(img_cube);
    T(no_errors());
    teardown();
}

//------------------------------------------------------------------------------
//  sg_copy_buffer_to_buffer
//------------------------------------------------------------------------------
UTEST(sokol_gfx_gl, copy_buffer_to_buffer) {
    setup();
    static const uint32_t data[16] = { 0 };
    sg_buffer src = sg_make_buffer(&(sg_buffer_desc){ .usage = { .vertex_buffer = true, .copy_src = true }, .data = SG_RANGE(data) });
    sg_buffer dst = sg_make_buffer(&(sg_buffer_desc){ .usage = { .vertex_buffer = true, .copy_dst = true }, .size = sizeof(data) });
    const GLuint gl_src = sg_gl_query_buffer_info(src).buf[0];
    const GLuint gl_dst = sg_gl_query_buffer_info(dst).buf[0];
    gl_mock_clear_calls();
    sg_copy_buffer_to_buffer(&(sg_copy_buffer_to_buffer_desc){
        .src = { .buffer = src, .offset = 8 },
        .dst = { .buffer = dst, .offset = 16 },
        .size = 32,
    });
    // exact call sequence: bind read + write targets, copy, unbind both
    TA(gl_mock_num_calls() == 5);
    const gl_mock_call_t* c = gl_mock_call(0);
    T(c->func == GL_MOCK_FUNC_glBindBuffer);
    T(c->args[0].i == GL_COPY_READ_BUFFER);
    T(c->args[1].i == gl_src);
    c = gl_mock_call(1);
    T(c->func == GL_MOCK_FUNC_glBindBuffer);
    T(c->args[0].i == GL_COPY_WRITE_BUFFER);
    T(c->args[1].i == gl_dst);
    c = gl_mock_call(2);
    T(c->func == GL_MOCK_FUNC_glCopyBufferSubData);
    T(c->args[0].i == GL_COPY_READ_BUFFER);
    T(c->args[1].i == GL_COPY_WRITE_BUFFER);
    T(c->args[2].i == 8);
    T(c->args[3].i == 16);
    T(c->args[4].i == 32);
    c = gl_mock_call(3);
    T(c->func == GL_MOCK_FUNC_glBindBuffer);
    T(c->args[0].i == GL_COPY_READ_BUFFER);
    T(c->args[1].i == 0);
    c = gl_mock_call(4);
    T(c->func == GL_MOCK_FUNC_glBindBuffer);
    T(c->args[0].i == GL_COPY_WRITE_BUFFER);
    T(c->args[1].i == 0);
    T(gl_mock_bindings()->copy_read_buffer == 0);
    T(gl_mock_bindings()->copy_write_buffer == 0);
    gl_mock_buffer_info_t mi;
    TA(gl_mock_buffer_info(gl_src, &mi));
    T(mi.num_copy_read == 1);
    TA(gl_mock_buffer_info(gl_dst, &mi));
    T(mi.num_copy_write == 1);
    sg_destroy_buffer(src);
    sg_destroy_buffer(dst);
    T(no_errors());
    teardown();
}

UTEST(sokol_gfx_gl, copy_staging_buffers_into_vertex_and_index_buffers) {
    setup();
    sg_buffer vstage = sg_make_buffer(&(sg_buffer_desc){ .usage = { .staging_buffer = true, .copy_src = true }, .size = 64 });
    sg_buffer istage = sg_make_buffer(&(sg_buffer_desc){ .usage = { .staging_index_buffer = true, .copy_src = true }, .size = 64 });
    sg_buffer vbuf = sg_make_buffer(&(sg_buffer_desc){ .usage = { .vertex_buffer = true, .copy_dst = true }, .size = 64 });
    sg_buffer ibuf = sg_make_buffer(&(sg_buffer_desc){ .usage = { .index_buffer = true, .copy_dst = true }, .size = 64 });
    // staging buffers are filled through their regular bind point
    uint8_t bytes[64] = { 0 };
    (void)bytes;
    gl_mock_clear_calls();
    sg_copy_buffer_to_buffer(&(sg_copy_buffer_to_buffer_desc){ .src.buffer = vstage, .dst.buffer = vbuf, .size = 64 });
    sg_copy_buffer_to_buffer(&(sg_copy_buffer_to_buffer_desc){ .src.buffer = istage, .dst.buffer = ibuf, .size = 64 });
    T(gl_mock_count_calls(GL_MOCK_FUNC_glCopyBufferSubData) == 2);
    // no memory barriers for non-storage buffers
    T(gl_mock_count_calls(GL_MOCK_FUNC_glMemoryBarrier) == 0);
    const gl_mock_call_t* c = gl_mock_last_call(GL_MOCK_FUNC_glCopyBufferSubData);
    TA(c != 0);
    T(c->args[2].i == 0);
    T(c->args[3].i == 0);
    T(c->args[4].i == 64);
    gl_mock_buffer_info_t mi;
    TA(gl_mock_buffer_info(sg_gl_query_buffer_info(ibuf).buf[0], &mi));
    T(mi.num_copy_write == 1);
    TA(gl_mock_buffer_info(sg_gl_query_buffer_info(istage).buf[0], &mi));
    T(mi.num_copy_read == 1);
    sg_destroy_buffer(vstage);
    sg_destroy_buffer(istage);
    sg_destroy_buffer(vbuf);
    sg_destroy_buffer(ibuf);
    T(no_errors());
    teardown();
}

#if TEST_HAS_COMPUTE
UTEST(sokol_gfx_gl, copy_buffer_to_buffer_storage_issues_barrier) {
    setup();
    sg_buffer sbuf = sg_make_buffer(&(sg_buffer_desc){ .usage = { .storage_buffer = true, .copy_src = true, .copy_dst = true }, .size = 64 });
    sg_buffer vbuf = sg_make_buffer(&(sg_buffer_desc){ .usage = { .vertex_buffer = true, .copy_src = true, .copy_dst = true }, .size = 64 });
    // storage buffer as source
    gl_mock_clear_calls();
    sg_copy_buffer_to_buffer(&(sg_copy_buffer_to_buffer_desc){ .src.buffer = sbuf, .dst.buffer = vbuf, .size = 64 });
    TA(gl_mock_num_calls() == 6);
    T(gl_mock_call(0)->func == GL_MOCK_FUNC_glMemoryBarrier);
    T(gl_mock_call(0)->args[0].i == GL_BUFFER_UPDATE_BARRIER_BIT);
    T(gl_mock_call(3)->func == GL_MOCK_FUNC_glCopyBufferSubData);
    // storage buffer as destination
    gl_mock_clear_calls();
    sg_copy_buffer_to_buffer(&(sg_copy_buffer_to_buffer_desc){ .src.buffer = vbuf, .dst.buffer = sbuf, .size = 64 });
    TA(gl_mock_count_calls(GL_MOCK_FUNC_glMemoryBarrier) == 1);
    T(gl_mock_last_call(GL_MOCK_FUNC_glMemoryBarrier)->args[0].i == GL_BUFFER_UPDATE_BARRIER_BIT);
    sg_destroy_buffer(sbuf);
    sg_destroy_buffer(vbuf);
    T(no_errors());
    teardown();
}
#endif

//------------------------------------------------------------------------------
//  sg_copy_buffer_to_image -- staging buffer via GL_PIXEL_UNPACK_BUFFER
//------------------------------------------------------------------------------
UTEST(sokol_gfx_gl, copy_dst_image_usage) {
    setup();
    sg_image img = sg_make_image(&(sg_image_desc){
        .width = 8, .height = 8,
        .pixel_format = SG_PIXELFORMAT_RGBA8,
        .usage.copy_dst = true,
    });
    T(sg_query_image_state(img) == SG_RESOURCESTATE_VALID);
    // the image is allocated without data, copy-dst images have a single slot
    T(sg_query_image_info(img).num_slots == 1);
    T(gl_mock_live_objects(GL_MOCK_OBJ_TEXTURE) == 1);
    T(gl_mock_count_calls(GL_MOCK_FUNC_glTexSubImage2D) == 0);
    #if TEST_HAS_TEXSTORAGE
        T(gl_mock_count_calls(GL_MOCK_FUNC_glTexStorage2D) == 1);
    #else
        const gl_mock_call_t* ti = gl_mock_last_call(GL_MOCK_FUNC_glTexImage2D);
        TA(ti != 0);
        T(ti->args[8].p == 0);
    #endif
    sg_destroy_image(img);
    T(gl_mock_live_objects(GL_MOCK_OBJ_TEXTURE) == 0);
    T(no_errors());
    teardown();
}

UTEST(sokol_gfx_gl, copy_buffer_to_image_2d) {
    setup();
    sg_buffer stage = sg_make_buffer(&(sg_buffer_desc){ .usage = { .staging_buffer = true, .copy_src = true }, .size = 8 * 8 * 4 });
    sg_image img = sg_make_image(&(sg_image_desc){
        .width = 8, .height = 8,
        .pixel_format = SG_PIXELFORMAT_RGBA8,
        .usage.copy_dst = true,
    });
    const GLuint gl_stage = sg_gl_query_buffer_info(stage).buf[0];
    const sg_gl_image_info img_info = sg_gl_query_image_info(img);
    const GLuint gl_tex = img_info.tex[img_info.active_slot];
    gl_mock_clear_calls();
    sg_copy_buffer_to_image(&(sg_copy_buffer_to_image_desc){ .src.buffer = stage, .dst.image = img });
    // bind the staging buffer as pixel unpack source...
    const int bind_idx = find_call_arg0(GL_MOCK_FUNC_glBindBuffer, 0, GL_PIXEL_UNPACK_BUFFER);
    TA(bind_idx >= 0);
    T(gl_mock_call(bind_idx)->args[1].i == gl_stage);
    // ...configure the unpack layout...
    const int row_idx = find_call_arg0(GL_MOCK_FUNC_glPixelStorei, bind_idx, GL_UNPACK_ROW_LENGTH);
    TA(row_idx > bind_idx);
    T(gl_mock_call(row_idx)->args[1].i == 8);
    // ...upload from buffer offset 0 (a null data pointer)...
    const int sub_idx = gl_mock_find_call(GL_MOCK_FUNC_glTexSubImage2D, row_idx);
    TA(sub_idx > row_idx);
    const gl_mock_call_t* sub = gl_mock_call(sub_idx);
    T(sub->args[0].i == GL_TEXTURE_2D);
    T(sub->args[1].i == 0);
    T(sub->args[2].i == 0);
    T(sub->args[3].i == 0);
    T(sub->args[4].i == 8);
    T(sub->args[5].i == 8);
    T(sub->args[6].i == GL_RGBA);
    T(sub->args[7].i == GL_UNSIGNED_BYTE);
    T(sub->args[8].p == 0);
    // ...and unbind the pixel unpack buffer again
    const int unbind_idx = find_call_arg0(GL_MOCK_FUNC_glBindBuffer, sub_idx, GL_PIXEL_UNPACK_BUFFER);
    TA(unbind_idx > sub_idx);
    T(gl_mock_call(unbind_idx)->args[1].i == 0);
    T(gl_mock_bindings()->pixel_unpack_buffer == 0);
    T(gl_mock_render_state()->unpack_row_length == 0);
    // the upload went into the image texture and was sourced from the staging buffer
    gl_mock_texture_info_t ti;
    TA(gl_mock_texture_info(gl_tex, &ti));
    T(ti.num_subimage == 1);
    T(ti.num_subimage_unpack_buffer == 1);
    T(ti.last_unpack_buffer == gl_stage);
    // no barriers needed (staging source, non-storage destination)
    T(gl_mock_count_calls(GL_MOCK_FUNC_glMemoryBarrier) == 0);
    T(gl_mock_count_calls(GL_MOCK_FUNC_glCopyBufferSubData) == 0);
    T(!logged(SG_LOGITEM_GL_APPLE_PIXEL_UNPACK_OFFSET_BUG));
    sg_destroy_buffer(stage);
    sg_destroy_image(img);
    T(no_errors());
    teardown();
}

// NOTE: the Apple pixel-unpack-offset warning is a 'warn once' static inside
// sokol_gfx.h, so this must be the only test that copies with a non-zero
// source offset
UTEST(sokol_gfx_gl, copy_buffer_to_image_subregion_with_src_offset) {
    setup();
    // one row of slack in front of the source data
    sg_buffer stage = sg_make_buffer(&(sg_buffer_desc){ .usage = { .staging_buffer = true, .copy_src = true }, .size = 9 * 8 * 4 });
    sg_image img = sg_make_image(&(sg_image_desc){
        .width = 8, .height = 8, .num_mipmaps = 4,
        .pixel_format = SG_PIXELFORMAT_RGBA8,
        .usage.copy_dst = true,
    });
    gl_mock_clear_calls();
    // copy a 4x2 region at (2, 4) of mip 0, source rows are 8 pixels wide, source offset is one row
    sg_copy_buffer_to_image(&(sg_copy_buffer_to_image_desc){
        .src = { .buffer = stage, .offset = 32, .bytes_per_row = 32, .bytes_per_slice = 2 * 32 },
        .dst = { .image = img, .x = 2, .y = 4 },
        .size = { .width = 4, .height = 2 },
    });
    const gl_mock_call_t* sub = gl_mock_last_call(GL_MOCK_FUNC_glTexSubImage2D);
    TA(sub != 0);
    T(sub->args[1].i == 0);
    T(sub->args[2].i == 2);
    T(sub->args[3].i == 4);
    T(sub->args[4].i == 4);
    T(sub->args[5].i == 2);
    // with a bound pixel unpack buffer, the data pointer is the buffer offset
    T(sub->args[8].p == (const void*)(uintptr_t)32);
    const int row_idx = find_call_arg0(GL_MOCK_FUNC_glPixelStorei, 0, GL_UNPACK_ROW_LENGTH);
    const int hgt_idx = find_call_arg0(GL_MOCK_FUNC_glPixelStorei, 0, GL_UNPACK_IMAGE_HEIGHT);
    TA((row_idx >= 0) && (hgt_idx >= 0));
    T(gl_mock_call(row_idx)->args[1].i == 8);
    T(gl_mock_call(hgt_idx)->args[1].i == 2);
    // the macOS/iOS GL drivers ignore that offset, sokol_gfx warns about it
    #if defined(__APPLE__)
        T(logged(SG_LOGITEM_GL_APPLE_PIXEL_UNPACK_OFFSET_BUG));
    #else
        T(!logged(SG_LOGITEM_GL_APPLE_PIXEL_UNPACK_OFFSET_BUG));
    #endif

    // a smaller mip level: default size is the whole 2x2 mip, rows default to 2 pixels
    reset_log();
    gl_mock_clear_calls();
    sg_copy_buffer_to_image(&(sg_copy_buffer_to_image_desc){ .src.buffer = stage, .dst = { .image = img, .mip_level = 2 } });
    sub = gl_mock_last_call(GL_MOCK_FUNC_glTexSubImage2D);
    TA(sub != 0);
    T(sub->args[1].i == 2);
    T(sub->args[4].i == 2);
    T(sub->args[5].i == 2);
    T(sub->args[8].p == 0);
    T(gl_mock_call(find_call_arg0(GL_MOCK_FUNC_glPixelStorei, 0, GL_UNPACK_ROW_LENGTH))->args[1].i == 2);
    T(!logged(SG_LOGITEM_GL_APPLE_PIXEL_UNPACK_OFFSET_BUG));
    sg_destroy_buffer(stage);
    sg_destroy_image(img);
    T(no_errors());
    teardown();
}

UTEST(sokol_gfx_gl, copy_buffer_to_image_cube_array_3d) {
    setup();
    sg_buffer stage = sg_make_buffer(&(sg_buffer_desc){ .usage = { .staging_buffer = true, .copy_src = true }, .size = 6 * 4 * 4 * 4 });
    const GLuint gl_stage = sg_gl_query_buffer_info(stage).buf[0];

    // cube: one glTexSubImage2D per face, each face at its own offset into the buffer
    sg_image cube = sg_make_image(&(sg_image_desc){
        .type = SG_IMAGETYPE_CUBE, .width = 4, .height = 4,
        .pixel_format = SG_PIXELFORMAT_RGBA8, .usage.copy_dst = true,
    });
    gl_mock_clear_calls();
    sg_copy_buffer_to_image(&(sg_copy_buffer_to_image_desc){ .src.buffer = stage, .dst.image = cube });
    TA(gl_mock_count_calls(GL_MOCK_FUNC_glTexSubImage2D) == 6);
    int idx = 0;
    for (int face = 0; face < 6; face++) {
        idx = gl_mock_find_call(GL_MOCK_FUNC_glTexSubImage2D, idx);
        TA(idx >= 0);
        const gl_mock_call_t* c = gl_mock_call(idx++);
        T(c->args[0].i == GL_TEXTURE_CUBE_MAP_POSITIVE_X + face);
        T(c->args[8].p == (const void*)(uintptr_t)(face * 64));
    }
    gl_mock_texture_info_t ti;
    TA(gl_mock_texture_info(sg_gl_query_image_info(cube).tex[0], &ti));
    T(ti.num_subimage_unpack_buffer == 6);
    T(ti.last_unpack_buffer == gl_stage);

    // array and 3D: a single glTexSubImage3D
    const sg_image_type types[2] = { SG_IMAGETYPE_ARRAY, SG_IMAGETYPE_3D };
    const GLenum targets[2] = { GL_TEXTURE_2D_ARRAY, GL_TEXTURE_3D };
    for (int i = 0; i < 2; i++) {
        sg_image img = sg_make_image(&(sg_image_desc){
            .type = types[i], .width = 4, .height = 4, .num_slices = 4,
            .pixel_format = SG_PIXELFORMAT_RGBA8, .usage.copy_dst = true,
        });
        T(sg_query_image_state(img) == SG_RESOURCESTATE_VALID);
        gl_mock_clear_calls();
        sg_copy_buffer_to_image(&(sg_copy_buffer_to_image_desc){
            .src.buffer = stage,
            .dst = { .image = img, .slice = 1 },
            .size.num_slices = 3,
        });
        const gl_mock_call_t* sub = gl_mock_last_call(GL_MOCK_FUNC_glTexSubImage3D);
        TA(sub != 0);
        T(sub->args[0].i == targets[i]);
        T(sub->args[4].i == 1);     // zoffset
        T(sub->args[7].i == 3);     // depth
        T(sub->args[10].p == 0);
        T(gl_mock_call(find_call_arg0(GL_MOCK_FUNC_glPixelStorei, 0, GL_UNPACK_IMAGE_HEIGHT))->args[1].i == 4);
        T(gl_mock_bindings()->pixel_unpack_buffer == 0);
        sg_destroy_image(img);
    }
    sg_destroy_image(cube);
    sg_destroy_buffer(stage);
    T(no_errors());
    teardown();
}

//------------------------------------------------------------------------------
//  compute memory barriers (GL 4.3 / GLES 3.1+)
//------------------------------------------------------------------------------
#if TEST_HAS_COMPUTE
static int count_barriers(GLbitfield* out_bits) {
    int n = 0;
    GLbitfield bits = 0;
    for (int i = 0; i < gl_mock_num_calls(); i++) {
        const gl_mock_call_t* c = gl_mock_call(i);
        if (c->func == GL_MOCK_FUNC_glMemoryBarrier) {
            n++;
            bits |= (GLbitfield)c->args[0].i;
        }
    }
    if (out_bits) {
        *out_bits = bits;
    }
    return n;
}

UTEST(sokol_gfx_gl, memory_barriers_after_compute_writes) {
    setup();
    // a buffer that's written by compute and then read as vertex-, index- and storage-buffer
    sg_buffer buf = sg_make_buffer(&(sg_buffer_desc){
        .usage = { .vertex_buffer = true, .index_buffer = true, .storage_buffer = true },
        .size = 256,
    });
    // an image that's written by compute and then sampled and rendered to
    sg_image img = sg_make_image(&(sg_image_desc){
        .width = 16, .height = 16, .pixel_format = SG_PIXELFORMAT_RGBA8,
        .usage = { .storage_image = true, .color_attachment = true },
    });
    sg_view sbuf_view = sg_make_view(&(sg_view_desc){ .storage_buffer.buffer = buf });
    sg_view simg_view = sg_make_view(&(sg_view_desc){ .storage_image.image = img });
    sg_view tex_view = sg_make_view(&(sg_view_desc){ .texture.image = img });
    sg_view att_view = sg_make_view(&(sg_view_desc){ .color_attachment.image = img });
    sg_sampler smp = sg_make_sampler(&(sg_sampler_desc){0});
    T(sg_query_view_state(sbuf_view) == SG_RESOURCESTATE_VALID);
    T(sg_query_view_state(simg_view) == SG_RESOURCESTATE_VALID);
    T(sg_query_view_state(tex_view) == SG_RESOURCESTATE_VALID);
    T(sg_query_view_state(att_view) == SG_RESOURCESTATE_VALID);

    sg_shader cs = sg_make_shader(&(sg_shader_desc){
        .compute_func.source = "cs",
        .views[0].storage_buffer = { .stage = SG_SHADERSTAGE_COMPUTE, .readonly = false },
        .views[1].storage_image = {
            .stage = SG_SHADERSTAGE_COMPUTE,
            .image_type = SG_IMAGETYPE_2D,
            .access_format = SG_PIXELFORMAT_RGBA8,
        },
    });
    sg_pipeline cpip = sg_make_pipeline(&(sg_pipeline_desc){ .shader = cs, .compute = true });
    sg_shader rs = sg_make_shader(&(sg_shader_desc){
        .vertex_func.source = "vs",
        .fragment_func.source = "fs",
        .views[0].storage_buffer = { .stage = SG_SHADERSTAGE_FRAGMENT, .readonly = true },
        .views[1].texture = { .stage = SG_SHADERSTAGE_FRAGMENT, .image_type = SG_IMAGETYPE_2D, .sample_type = SG_IMAGESAMPLETYPE_FLOAT },
        .samplers[0] = { .stage = SG_SHADERSTAGE_FRAGMENT, .sampler_type = SG_SAMPLERTYPE_FILTERING },
        .texture_sampler_pairs[0] = { .stage = SG_SHADERSTAGE_FRAGMENT, .view_slot = 1, .sampler_slot = 0, .glsl_name = "tex" },
    });
    sg_pipeline rpip = sg_make_pipeline(&(sg_pipeline_desc){
        .shader = rs,
        .layout.attrs[0].format = SG_VERTEXFORMAT_FLOAT4,
        .index_type = SG_INDEXTYPE_UINT16,
    });
    T(sg_query_pipeline_state(cpip) == SG_RESOURCESTATE_VALID);
    T(sg_query_pipeline_state(rpip) == SG_RESOURCESTATE_VALID);
    const sg_swapchain swapchain = {
        .width = 16, .height = 16, .sample_count = 1,
        .color_format = SG_PIXELFORMAT_RGBA8, .depth_format = SG_PIXELFORMAT_DEPTH_STENCIL,
    };
    GLbitfield bits = 0;

    // 1st compute pass: nothing is dirty yet, so no barrier, but the
    // bound storage buffer and storage image become dirty
    gl_mock_clear_calls();
    sg_begin_pass(&(sg_pass){ .compute = true });
    sg_apply_pipeline(cpip);
    sg_apply_bindings(&(sg_bindings){ .views = { [0] = sbuf_view, [1] = simg_view } });
    sg_dispatch(1, 1, 1);
    sg_end_pass();
    T(count_barriers(0) == 0);

    // render pass reading everything that was written: one combined barrier
    gl_mock_clear_calls();
    sg_begin_pass(&(sg_pass){ .swapchain = swapchain });
    sg_apply_pipeline(rpip);
    const sg_bindings rbnd = {
        .vertex_buffers[0] = buf,
        .index_buffer = buf,
        .views = { [0] = sbuf_view, [1] = tex_view },
        .samplers[0] = smp,
    };
    sg_apply_bindings(&rbnd);
    T(count_barriers(&bits) == 1);
    T(bits == (GL_VERTEX_ATTRIB_ARRAY_BARRIER_BIT | GL_ELEMENT_ARRAY_BARRIER_BIT | GL_SHADER_STORAGE_BARRIER_BIT | GL_TEXTURE_FETCH_BARRIER_BIT));
    // the dirty flags are cleared, binding again needs no barrier
    gl_mock_clear_calls();
    sg_apply_bindings(&rbnd);
    T(count_barriers(0) == 0);
    sg_end_pass();

    // render into the image as color attachment: framebuffer barrier at pass begin
    gl_mock_clear_calls();
    sg_begin_pass(&(sg_pass){ .attachments.colors[0] = att_view });
    T(count_barriers(&bits) == 1);
    T(bits == GL_FRAMEBUFFER_BARRIER_BIT);
    sg_end_pass();

    // 2nd compute pass: only the storage image access is still dirty
    gl_mock_clear_calls();
    sg_begin_pass(&(sg_pass){ .compute = true });
    sg_apply_pipeline(cpip);
    sg_apply_bindings(&(sg_bindings){ .views = { [0] = sbuf_view, [1] = simg_view } });
    T(count_barriers(&bits) == 1);
    T(bits == GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
    sg_end_pass();

    // a later copy out of the compute-written storage buffer needs a buffer-update barrier
    sg_buffer dst = sg_make_buffer(&(sg_buffer_desc){ .usage = { .vertex_buffer = true, .copy_dst = true }, .size = 256 });
    sg_commit();
    T(sg_isvalid());

    sg_destroy_buffer(dst);
    sg_destroy_pipeline(rpip);
    sg_destroy_shader(rs);
    sg_destroy_pipeline(cpip);
    sg_destroy_shader(cs);
    sg_destroy_sampler(smp);
    sg_destroy_view(att_view);
    sg_destroy_view(tex_view);
    sg_destroy_view(simg_view);
    sg_destroy_view(sbuf_view);
    sg_destroy_image(img);
    sg_destroy_buffer(buf);
    T(no_errors());
    teardown();
}

UTEST(sokol_gfx_gl, memory_barriers_readonly_storage_buffer_stays_clean) {
    setup();
    sg_buffer buf = sg_make_buffer(&(sg_buffer_desc){
        .usage = { .vertex_buffer = true, .storage_buffer = true },
        .size = 256,
    });
    sg_view sbuf_view = sg_make_view(&(sg_view_desc){ .storage_buffer.buffer = buf });
    // compute shader only *reads* the storage buffer
    sg_shader cs = sg_make_shader(&(sg_shader_desc){
        .compute_func.source = "cs",
        .views[0].storage_buffer = { .stage = SG_SHADERSTAGE_COMPUTE, .readonly = true },
    });
    sg_pipeline cpip = sg_make_pipeline(&(sg_pipeline_desc){ .shader = cs, .compute = true });
    sg_begin_pass(&(sg_pass){ .compute = true });
    sg_apply_pipeline(cpip);
    sg_apply_bindings(&(sg_bindings){ .views[0] = sbuf_view });
    sg_dispatch(1, 1, 1);
    sg_end_pass();
    gl_mock_clear_calls();
    sg_begin_pass(&(sg_pass){ .compute = true });
    sg_apply_pipeline(cpip);
    sg_apply_bindings(&(sg_bindings){ .views[0] = sbuf_view });
    sg_end_pass();
    sg_commit();
    T(gl_mock_count_calls(GL_MOCK_FUNC_glMemoryBarrier) == 0);
    sg_destroy_pipeline(cpip);
    sg_destroy_shader(cs);
    sg_destroy_view(sbuf_view);
    sg_destroy_buffer(buf);
    T(no_errors());
    teardown();
}
#endif

//------------------------------------------------------------------------------
//  injected images with an explicit texture target
//------------------------------------------------------------------------------
UTEST(sokol_gfx_gl, image_inject_native_with_texture_target) {
    setup();
    sg_image img = sg_make_image(&(sg_image_desc){
        .width = 16, .height = 16,
        .pixel_format = SG_PIXELFORMAT_RGBA8,
        .gl_texture = 0xC0DE,
    });
    T(sg_gl_query_image_info(img).tex[0] == 0xC0DE);
    T(sg_gl_query_image_info(img).tex_target == GL_TEXTURE_2D);
    sg_image img2 = sg_make_image(&(sg_image_desc){
        .type = SG_IMAGETYPE_ARRAY,
        .width = 16, .height = 16, .num_slices = 2,
        .pixel_format = SG_PIXELFORMAT_RGBA8,
        .gl_texture = 0xC0DF,
        .gl_texture_target = GL_TEXTURE_3D,
    });
    T(sg_query_image_state(img2) == SG_RESOURCESTATE_VALID);
    T(sg_gl_query_image_info(img2).tex_target == GL_TEXTURE_3D);
    T(gl_mock_count_calls(GL_MOCK_FUNC_glGenTextures) == 0);
    sg_destroy_image(img);
    sg_destroy_image(img2);
    T(gl_mock_count_calls(GL_MOCK_FUNC_glDeleteTextures) == 0);
    T(no_errors());
    teardown();
}

UTEST(sokol_gfx_gl, buffer_inject_native_query) {
    setup();
    sg_buffer buf = sg_make_buffer(&(sg_buffer_desc){ .size = 64, .usage.index_buffer = true, .gl_buffer = 0xBEEF });
    T(sg_query_buffer_state(buf) == SG_RESOURCESTATE_VALID);
    T(sg_gl_query_buffer_info(buf).buf[0] == 0xBEEF);
    T(sg_query_buffer_info(buf).num_slots == 1);
    sg_destroy_buffer(buf);
    T(gl_mock_count_calls(GL_MOCK_FUNC_glDeleteBuffers) == 0);
    T(no_errors());
    teardown();
}

//------------------------------------------------------------------------------
//  render pipeline state -> GL render state
//------------------------------------------------------------------------------
static const sg_swapchain test_swapchain = {
    .width = 32, .height = 32, .sample_count = 1,
    .color_format = SG_PIXELFORMAT_RGBA8, .depth_format = SG_PIXELFORMAT_DEPTH_STENCIL,
};

// creates a pipeline, applies it in the current pass and destroys it again
static void apply_tmp_pipeline(sg_pipeline_desc desc) {
    sg_pipeline pip = sg_make_pipeline(&desc);
    sg_apply_pipeline(pip);
    sg_destroy_pipeline(pip);
}

UTEST(sokol_gfx_gl, pipeline_compare_funcs) {
    setup();
    const GLenum expected[_SG_COMPAREFUNC_NUM] = {
        [SG_COMPAREFUNC_NEVER] = GL_NEVER,
        [SG_COMPAREFUNC_LESS] = GL_LESS,
        [SG_COMPAREFUNC_EQUAL] = GL_EQUAL,
        [SG_COMPAREFUNC_LESS_EQUAL] = GL_LEQUAL,
        [SG_COMPAREFUNC_GREATER] = GL_GREATER,
        [SG_COMPAREFUNC_NOT_EQUAL] = GL_NOTEQUAL,
        [SG_COMPAREFUNC_GREATER_EQUAL] = GL_GEQUAL,
        [SG_COMPAREFUNC_ALWAYS] = GL_ALWAYS,
    };
    sg_shader shd = make_test_shader();
    sg_begin_pass(&(sg_pass){ .swapchain = test_swapchain });
    for (int i = SG_COMPAREFUNC_NEVER; i < _SG_COMPAREFUNC_NUM; i++) {
        const sg_compare_func cmp = (sg_compare_func)i;
        apply_tmp_pipeline((sg_pipeline_desc){
            .shader = shd,
            .depth.compare = cmp,
            .stencil = { .enabled = true, .front.compare = cmp, .back.compare = cmp, .ref = 3, .read_mask = 0x0F },
        });
        T(gl_mock_render_state()->depth_func == expected[i]);
        T(gl_mock_render_state()->stencil_front.func == expected[i]);
        T(gl_mock_render_state()->stencil_back.func == expected[i]);
        T(gl_mock_render_state()->stencil_front.ref == 3);
        T(gl_mock_render_state()->stencil_front.mask == 0x0F);
        T(gl_mock_is_enabled(GL_STENCIL_TEST));
    }
    // a pipeline without stencil disables the stencil test again
    apply_tmp_pipeline((sg_pipeline_desc){ .shader = shd });
    T(!gl_mock_is_enabled(GL_STENCIL_TEST));
    sg_end_pass();
    sg_commit();
    sg_destroy_shader(shd);
    T(no_errors());
    teardown();
}

UTEST(sokol_gfx_gl, pipeline_stencil_ops) {
    setup();
    const GLenum expected[_SG_STENCILOP_NUM] = {
        [SG_STENCILOP_KEEP] = GL_KEEP,
        [SG_STENCILOP_ZERO] = GL_ZERO,
        [SG_STENCILOP_REPLACE] = GL_REPLACE,
        [SG_STENCILOP_INCR_CLAMP] = GL_INCR,
        [SG_STENCILOP_DECR_CLAMP] = GL_DECR,
        [SG_STENCILOP_INVERT] = GL_INVERT,
        [SG_STENCILOP_INCR_WRAP] = GL_INCR_WRAP,
        [SG_STENCILOP_DECR_WRAP] = GL_DECR_WRAP,
    };
    sg_shader shd = make_test_shader();
    sg_begin_pass(&(sg_pass){ .swapchain = test_swapchain });
    for (int i = SG_STENCILOP_KEEP; i < _SG_STENCILOP_NUM; i++) {
        // rotate the ops so that each face/op slot sees a different value
        const sg_stencil_op op0 = (sg_stencil_op)i;
        const sg_stencil_op op1 = (sg_stencil_op)(((i - 1 + 1) % (_SG_STENCILOP_NUM - 1)) + 1);
        const sg_stencil_op op2 = (sg_stencil_op)(((i - 1 + 2) % (_SG_STENCILOP_NUM - 1)) + 1);
        apply_tmp_pipeline((sg_pipeline_desc){
            .shader = shd,
            .stencil = {
                .enabled = true,
                .front = { .fail_op = op0, .depth_fail_op = op1, .pass_op = op2 },
                .back = { .fail_op = op2, .depth_fail_op = op0, .pass_op = op1 },
                .write_mask = 0x7F,
            },
        });
        const gl_mock_render_state_t* rs = gl_mock_render_state();
        T(rs->stencil_front.fail_op == expected[op0]);
        T(rs->stencil_front.depth_fail_op == expected[op1]);
        T(rs->stencil_front.pass_op == expected[op2]);
        T(rs->stencil_back.fail_op == expected[op2]);
        T(rs->stencil_back.depth_fail_op == expected[op0]);
        T(rs->stencil_back.pass_op == expected[op1]);
        T(rs->stencil_write_mask == 0x7F);
    }
    sg_end_pass();
    sg_commit();
    sg_destroy_shader(shd);
    T(no_errors());
    teardown();
}

UTEST(sokol_gfx_gl, pipeline_blend_factors_and_ops) {
    setup();
    const GLenum factors[_SG_BLENDFACTOR_NUM] = {
        [SG_BLENDFACTOR_ZERO] = GL_ZERO,
        [SG_BLENDFACTOR_ONE] = GL_ONE,
        [SG_BLENDFACTOR_SRC_COLOR] = GL_SRC_COLOR,
        [SG_BLENDFACTOR_ONE_MINUS_SRC_COLOR] = GL_ONE_MINUS_SRC_COLOR,
        [SG_BLENDFACTOR_SRC_ALPHA] = GL_SRC_ALPHA,
        [SG_BLENDFACTOR_ONE_MINUS_SRC_ALPHA] = GL_ONE_MINUS_SRC_ALPHA,
        [SG_BLENDFACTOR_DST_COLOR] = GL_DST_COLOR,
        [SG_BLENDFACTOR_ONE_MINUS_DST_COLOR] = GL_ONE_MINUS_DST_COLOR,
        [SG_BLENDFACTOR_DST_ALPHA] = GL_DST_ALPHA,
        [SG_BLENDFACTOR_ONE_MINUS_DST_ALPHA] = GL_ONE_MINUS_DST_ALPHA,
        [SG_BLENDFACTOR_SRC_ALPHA_SATURATED] = GL_SRC_ALPHA_SATURATE,
        [SG_BLENDFACTOR_BLEND_COLOR] = GL_CONSTANT_COLOR,
        [SG_BLENDFACTOR_ONE_MINUS_BLEND_COLOR] = GL_ONE_MINUS_CONSTANT_COLOR,
        [SG_BLENDFACTOR_BLEND_ALPHA] = GL_CONSTANT_ALPHA,
        [SG_BLENDFACTOR_ONE_MINUS_BLEND_ALPHA] = GL_ONE_MINUS_CONSTANT_ALPHA,
        [SG_BLENDFACTOR_SRC1_COLOR] = GL_SRC1_COLOR,
        [SG_BLENDFACTOR_ONE_MINUS_SRC1_COLOR] = GL_ONE_MINUS_SRC1_COLOR,
        [SG_BLENDFACTOR_SRC1_ALPHA] = GL_SRC1_ALPHA,
        [SG_BLENDFACTOR_ONE_MINUS_SRC1_ALPHA] = GL_ONE_MINUS_SRC1_ALPHA,
    };
    const GLenum ops[_SG_BLENDOP_NUM] = {
        [SG_BLENDOP_ADD] = GL_FUNC_ADD,
        [SG_BLENDOP_SUBTRACT] = GL_FUNC_SUBTRACT,
        [SG_BLENDOP_REVERSE_SUBTRACT] = GL_FUNC_REVERSE_SUBTRACT,
        [SG_BLENDOP_MIN] = GL_MIN,
        [SG_BLENDOP_MAX] = GL_MAX,
    };
    // the dual-source factors are only available with dual-source blending
    const int num_factors = sg_query_features().dual_source_blending ? _SG_BLENDFACTOR_NUM : SG_BLENDFACTOR_SRC1_COLOR;
    sg_shader shd = make_test_shader();
    sg_begin_pass(&(sg_pass){ .swapchain = test_swapchain });
    for (int i = SG_BLENDFACTOR_ZERO; i < num_factors; i++) {
        const sg_blend_factor f0 = (sg_blend_factor)i;
        const sg_blend_factor f1 = (sg_blend_factor)(((i - 1 + 3) % (num_factors - 1)) + 1);
        apply_tmp_pipeline((sg_pipeline_desc){
            .shader = shd,
            .colors[0].blend = {
                .enabled = true,
                .src_factor_rgb = f0, .dst_factor_rgb = f1,
                .src_factor_alpha = f1, .dst_factor_alpha = f0,
                .op_rgb = SG_BLENDOP_ADD, .op_alpha = SG_BLENDOP_SUBTRACT,
            },
        });
        const gl_mock_render_state_t* rs = gl_mock_render_state();
        T(gl_mock_is_enabled(GL_BLEND));
        T(rs->blend_src_rgb == factors[f0]);
        T(rs->blend_dst_rgb == factors[f1]);
        T(rs->blend_src_alpha == factors[f1]);
        T(rs->blend_dst_alpha == factors[f0]);
    }
    for (int i = SG_BLENDOP_ADD; i < _SG_BLENDOP_NUM; i++) {
        const sg_blend_op op = (sg_blend_op)i;
        apply_tmp_pipeline((sg_pipeline_desc){
            .shader = shd,
            .colors[0].blend = {
                .enabled = true,
                .src_factor_rgb = SG_BLENDFACTOR_ONE, .dst_factor_rgb = SG_BLENDFACTOR_ONE,
                .src_factor_alpha = SG_BLENDFACTOR_ONE, .dst_factor_alpha = SG_BLENDFACTOR_ONE,
                .op_rgb = op, .op_alpha = op,
            },
        });
        T(gl_mock_render_state()->blend_op_rgb == ops[op]);
        T(gl_mock_render_state()->blend_op_alpha == ops[op]);
    }
    // blend disabled again, plus a blend color
    apply_tmp_pipeline((sg_pipeline_desc){ .shader = shd, .blend_color = { 0.25f, 0.5f, 0.75f, 1.0f } });
    T(!gl_mock_is_enabled(GL_BLEND));
    T(gl_mock_render_state()->blend_color[0] == 0.25f);
    T(gl_mock_render_state()->blend_color[2] == 0.75f);
    sg_end_pass();
    sg_commit();
    sg_destroy_shader(shd);
    T(no_errors());
    teardown();
}

UTEST(sokol_gfx_gl, pipeline_rasterizer_state) {
    setup();
    sg_shader shd = make_test_shader();
    sg_begin_pass(&(sg_pass){ .swapchain = test_swapchain });
    // cull modes and face winding
    apply_tmp_pipeline((sg_pipeline_desc){ .shader = shd, .cull_mode = SG_CULLMODE_FRONT, .face_winding = SG_FACEWINDING_CW });
    T(gl_mock_is_enabled(GL_CULL_FACE));
    T(gl_mock_render_state()->cull_face == GL_FRONT);
    T(gl_mock_render_state()->front_face == GL_CW);
    apply_tmp_pipeline((sg_pipeline_desc){ .shader = shd, .cull_mode = SG_CULLMODE_BACK, .face_winding = SG_FACEWINDING_CCW });
    T(gl_mock_is_enabled(GL_CULL_FACE));
    T(gl_mock_render_state()->cull_face == GL_BACK);
    T(gl_mock_render_state()->front_face == GL_CCW);
    apply_tmp_pipeline((sg_pipeline_desc){ .shader = shd, .cull_mode = SG_CULLMODE_NONE });
    T(!gl_mock_is_enabled(GL_CULL_FACE));
    // depth bias toggles GL_POLYGON_OFFSET_FILL
    apply_tmp_pipeline((sg_pipeline_desc){ .shader = shd, .depth = { .bias = 1.0f, .bias_slope_scale = 2.0f } });
    T(gl_mock_is_enabled(GL_POLYGON_OFFSET_FILL));
    T(gl_mock_render_state()->polygon_offset_factor == 2.0f);
    T(gl_mock_render_state()->polygon_offset_units == 1.0f);
    apply_tmp_pipeline((sg_pipeline_desc){ .shader = shd });
    T(!gl_mock_is_enabled(GL_POLYGON_OFFSET_FILL));
    // depth write
    apply_tmp_pipeline((sg_pipeline_desc){ .shader = shd, .depth = { .write_enabled = true, .compare = SG_COMPAREFUNC_LESS_EQUAL } });
    T(gl_mock_render_state()->depth_mask == GL_TRUE);
    // color write mask on the only color attachment
    apply_tmp_pipeline((sg_pipeline_desc){ .shader = shd, .colors[0].write_mask = SG_COLORMASK_RB });
    T(gl_mock_render_state()->color_mask[0][0] == GL_TRUE);
    T(gl_mock_render_state()->color_mask[0][1] == GL_FALSE);
    T(gl_mock_render_state()->color_mask[0][2] == GL_TRUE);
    T(gl_mock_render_state()->color_mask[0][3] == GL_FALSE);
    sg_end_pass();

    // a clearing pass must reset the color mask and the depth state for the clear...
    gl_mock_clear_calls();
    sg_begin_pass(&(sg_pass){
        .swapchain = test_swapchain,
        .action = {
            .colors[0].load_action = SG_LOADACTION_CLEAR,
            .depth.load_action = SG_LOADACTION_CLEAR,
            .stencil.load_action = SG_LOADACTION_CLEAR,
        },
    });
    const gl_mock_call_t* cm = gl_mock_last_call(GL_MOCK_FUNC_glColorMask);
    TA(cm != 0);
    T(cm->args[0].i == GL_TRUE && cm->args[1].i == GL_TRUE && cm->args[2].i == GL_TRUE && cm->args[3].i == GL_TRUE);
    T(gl_mock_render_state()->depth_func == GL_ALWAYS);
    // ...and the next pipeline must restore the color mask
    apply_tmp_pipeline((sg_pipeline_desc){ .shader = shd, .colors[0].write_mask = SG_COLORMASK_A });
    T(gl_mock_render_state()->color_mask[0][0] == GL_FALSE);
    T(gl_mock_render_state()->color_mask[0][3] == GL_TRUE);
    sg_end_pass();
    sg_commit();
    sg_destroy_shader(shd);
    T(no_errors());
    teardown();
}

UTEST(sokol_gfx_gl, pipeline_msaa_and_alpha_to_coverage) {
    setup();
    sg_shader shd = make_test_shader();
    sg_swapchain msaa_swapchain = test_swapchain;
    msaa_swapchain.sample_count = 4;
    sg_begin_pass(&(sg_pass){ .swapchain = msaa_swapchain });
    apply_tmp_pipeline((sg_pipeline_desc){ .shader = shd, .sample_count = 4, .alpha_to_coverage_enabled = true });
    T(gl_mock_is_enabled(GL_SAMPLE_ALPHA_TO_COVERAGE));
    #if defined(SOKOL_GLCORE)
    T(gl_mock_is_enabled(GL_MULTISAMPLE));
    #endif
    apply_tmp_pipeline((sg_pipeline_desc){ .shader = shd, .sample_count = 4 });
    T(!gl_mock_is_enabled(GL_SAMPLE_ALPHA_TO_COVERAGE));
    sg_end_pass();
    sg_begin_pass(&(sg_pass){ .swapchain = test_swapchain });
    apply_tmp_pipeline((sg_pipeline_desc){ .shader = shd });
    #if defined(SOKOL_GLCORE)
    T(!gl_mock_is_enabled(GL_MULTISAMPLE));
    #endif
    sg_end_pass();
    sg_commit();
    sg_destroy_shader(shd);
    T(no_errors());
    teardown();
}

UTEST(sokol_gfx_gl, pipeline_mrt_color_write_masks) {
    setup();
    sg_image imgs[2];
    sg_view views[2];
    for (int i = 0; i < 2; i++) {
        imgs[i] = sg_make_image(&(sg_image_desc){
            .width = 16, .height = 16, .pixel_format = SG_PIXELFORMAT_RGBA8, .usage.color_attachment = true,
        });
        views[i] = sg_make_view(&(sg_view_desc){ .color_attachment.image = imgs[i] });
    }
    sg_shader shd = make_test_shader();
    sg_pipeline pip = sg_make_pipeline(&(sg_pipeline_desc){
        .shader = shd,
        .color_count = 2,
        .colors = { [0].write_mask = SG_COLORMASK_R, [1].write_mask = SG_COLORMASK_GB },
        .depth.pixel_format = SG_PIXELFORMAT_NONE,
    });
    T(sg_query_pipeline_state(pip) == SG_RESOURCESTATE_VALID);
    sg_begin_pass(&(sg_pass){ .attachments.colors = { views[0], views[1] } });
    gl_mock_clear_calls();
    sg_apply_pipeline(pip);
    if (sg_query_features().mrt_independent_write_mask) {
        T(gl_mock_count_calls(GL_MOCK_FUNC_glColorMaski) == 2);
        T(gl_mock_render_state()->color_mask[1][0] == GL_FALSE);
        T(gl_mock_render_state()->color_mask[1][1] == GL_TRUE);
        T(gl_mock_render_state()->color_mask[1][2] == GL_TRUE);
    } else {
        T(gl_mock_count_calls(GL_MOCK_FUNC_glColorMask) == 1);
    }
    T(gl_mock_render_state()->color_mask[0][0] == GL_TRUE);
    T(gl_mock_render_state()->color_mask[0][1] == GL_FALSE);
    sg_end_pass();
    sg_commit();
    sg_destroy_pipeline(pip);
    sg_destroy_shader(shd);
    for (int i = 0; i < 2; i++) {
        sg_destroy_view(views[i]);
        sg_destroy_image(imgs[i]);
    }
    T(no_errors());
    teardown();
}

//------------------------------------------------------------------------------
//  vertex formats -> glVertexAttrib(I)Pointer
//------------------------------------------------------------------------------
typedef struct {
    sg_vertex_format fmt;
    GLint size;
    GLenum type;
    bool normalized;
    bool integer;
} vertex_format_case_t;

UTEST(sokol_gfx_gl, pipeline_vertex_formats) {
    const vertex_format_case_t cases[] = {
        { SG_VERTEXFORMAT_FLOAT,     1, GL_FLOAT, false, false },
        { SG_VERTEXFORMAT_FLOAT2,    2, GL_FLOAT, false, false },
        { SG_VERTEXFORMAT_FLOAT3,    3, GL_FLOAT, false, false },
        { SG_VERTEXFORMAT_FLOAT4,    4, GL_FLOAT, false, false },
        { SG_VERTEXFORMAT_INT,       1, GL_INT, false, true },
        { SG_VERTEXFORMAT_INT2,      2, GL_INT, false, true },
        { SG_VERTEXFORMAT_INT3,      3, GL_INT, false, true },
        { SG_VERTEXFORMAT_INT4,      4, GL_INT, false, true },
        { SG_VERTEXFORMAT_UINT,      1, GL_UNSIGNED_INT, false, true },
        { SG_VERTEXFORMAT_UINT2,     2, GL_UNSIGNED_INT, false, true },
        { SG_VERTEXFORMAT_UINT3,     3, GL_UNSIGNED_INT, false, true },
        { SG_VERTEXFORMAT_UINT4,     4, GL_UNSIGNED_INT, false, true },
        { SG_VERTEXFORMAT_BYTE4,     4, GL_BYTE, false, true },
        { SG_VERTEXFORMAT_BYTE4N,    4, GL_BYTE, true, false },
        { SG_VERTEXFORMAT_UBYTE4,    4, GL_UNSIGNED_BYTE, false, true },
        { SG_VERTEXFORMAT_UBYTE4N,   4, GL_UNSIGNED_BYTE, true, false },
        { SG_VERTEXFORMAT_SHORT2,    2, GL_SHORT, false, true },
        { SG_VERTEXFORMAT_SHORT2N,   2, GL_SHORT, true, false },
        { SG_VERTEXFORMAT_USHORT2,   2, GL_UNSIGNED_SHORT, false, true },
        { SG_VERTEXFORMAT_USHORT2N,  2, GL_UNSIGNED_SHORT, true, false },
        { SG_VERTEXFORMAT_SHORT4,    4, GL_SHORT, false, true },
        { SG_VERTEXFORMAT_SHORT4N,   4, GL_SHORT, true, false },
        { SG_VERTEXFORMAT_USHORT4,   4, GL_UNSIGNED_SHORT, false, true },
        { SG_VERTEXFORMAT_USHORT4N,  4, GL_UNSIGNED_SHORT, true, false },
        { SG_VERTEXFORMAT_INT10_N2,  4, GL_INT_2_10_10_10_REV, true, false },
        { SG_VERTEXFORMAT_UINT10_N2, 4, GL_UNSIGNED_INT_2_10_10_10_REV, true, false },
        { SG_VERTEXFORMAT_HALF2,     2, GL_HALF_FLOAT, false, false },
        { SG_VERTEXFORMAT_HALF4,     4, GL_HALF_FLOAT, false, false },
    };
    const int num_cases = (int)(sizeof(cases) / sizeof(cases[0]));
    T(num_cases == (_SG_VERTEXFORMAT_NUM - 1));
    setup();
    static const uint8_t vdata[256] = { 0 };
    sg_buffer vbuf = sg_make_buffer(&(sg_buffer_desc){ .data = SG_RANGE(vdata) });
    sg_shader shd = make_test_shader();
    sg_begin_pass(&(sg_pass){ .swapchain = test_swapchain });
    for (int i = 0; i < num_cases; i++) {
        const vertex_format_case_t* c = &cases[i];
        sg_pipeline pip = sg_make_pipeline(&(sg_pipeline_desc){
            .shader = shd,
            .layout = {
                .buffers[0].stride = 32,
                .attrs[0] = { .format = c->fmt, .offset = 4 },
            },
        });
        T(sg_query_pipeline_state(pip) == SG_RESOURCESTATE_VALID);
        gl_mock_clear_calls();
        sg_apply_pipeline(pip);
        sg_apply_bindings(&(sg_bindings){ .vertex_buffers[0] = vbuf, .vertex_buffer_offsets[0] = (i & 1) * 32 });
        const gl_mock_call_t* p;
        if (c->integer) {
            T(gl_mock_count_calls(GL_MOCK_FUNC_glVertexAttribPointer) == 0);
            p = gl_mock_last_call(GL_MOCK_FUNC_glVertexAttribIPointer);
            TA(p != 0);
            T(p->args[0].i == 0);
            T(p->args[1].i == c->size);
            T(p->args[2].i == c->type);
            T(p->args[3].i == 32);
            T(p->args[4].p == (const void*)(uintptr_t)(4 + (i & 1) * 32));
        } else {
            T(gl_mock_count_calls(GL_MOCK_FUNC_glVertexAttribIPointer) == 0);
            p = gl_mock_last_call(GL_MOCK_FUNC_glVertexAttribPointer);
            TA(p != 0);
            T(p->args[0].i == 0);
            T(p->args[1].i == c->size);
            T(p->args[2].i == c->type);
            T(p->args[3].i == (c->normalized ? GL_TRUE : GL_FALSE));
            T(p->args[4].i == 32);
        }
        sg_destroy_pipeline(pip);
    }
    sg_end_pass();
    sg_commit();
    sg_destroy_shader(shd);
    sg_destroy_buffer(vbuf);
    T(no_errors());
    teardown();
}

UTEST(sokol_gfx_gl, pipeline_instancing_and_disabled_attrs) {
    setup();
    static const float vdata[64] = { 0 };
    sg_buffer vbuf = sg_make_buffer(&(sg_buffer_desc){ .data = SG_RANGE(vdata) });
    sg_shader shd = make_test_shader();
    sg_pipeline pip2 = sg_make_pipeline(&(sg_pipeline_desc){
        .shader = shd,
        .layout = {
            .buffers[1] = { .step_func = SG_VERTEXSTEP_PER_INSTANCE, .step_rate = 2 },
            .attrs = {
                [0] = { .format = SG_VERTEXFORMAT_FLOAT3, .buffer_index = 0 },
                [1] = { .format = SG_VERTEXFORMAT_FLOAT4, .buffer_index = 1 },
            },
        },
    });
    sg_pipeline pip1 = sg_make_pipeline(&(sg_pipeline_desc){
        .shader = shd,
        .layout.attrs[0].format = SG_VERTEXFORMAT_FLOAT3,
    });
    sg_begin_pass(&(sg_pass){ .swapchain = test_swapchain });
    gl_mock_clear_calls();
    sg_apply_pipeline(pip2);
    sg_apply_bindings(&(sg_bindings){ .vertex_buffers = { vbuf, vbuf } });
    // per-instance attribute gets the step rate as divisor
    int idx = find_call_arg0(GL_MOCK_FUNC_glVertexAttribDivisor, 0, 1);
    TA(idx >= 0);
    T(gl_mock_call(idx)->args[1].i == 2);
    T(gl_mock_vertex_attrib_enabled(0));
    T(gl_mock_vertex_attrib_enabled(1));
    // switching to a pipeline with fewer attributes disables the unused one
    gl_mock_clear_calls();
    sg_apply_pipeline(pip1);
    sg_apply_bindings(&(sg_bindings){ .vertex_buffers[0] = vbuf });
    idx = find_call_arg0(GL_MOCK_FUNC_glDisableVertexAttribArray, 0, 1);
    T(idx >= 0);
    T(gl_mock_vertex_attrib_enabled(0));
    T(!gl_mock_vertex_attrib_enabled(1));
    sg_end_pass();
    sg_commit();
    sg_destroy_pipeline(pip1);
    sg_destroy_pipeline(pip2);
    sg_destroy_shader(shd);
    sg_destroy_buffer(vbuf);
    T(no_errors());
    teardown();
}

UTEST(sokol_gfx_gl, pipeline_attr_not_found_in_shader) {
    setup();
    sg_shader shd = sg_make_shader(&(sg_shader_desc){
        .vertex_func.source = "vs", .fragment_func.source = "fs",
        .attrs = { [0].glsl_name = "pos", [1].glsl_name = "color" },
    });
    // the first glGetAttribLocation ('pos') returns -1
    gl_mock_fail_next_attrib_location(1);
    sg_pipeline pip = sg_make_pipeline(&(sg_pipeline_desc){
        .shader = shd,
        .layout.attrs = { [0].format = SG_VERTEXFORMAT_FLOAT3, [1].format = SG_VERTEXFORMAT_FLOAT4 },
    });
    // a missing attribute is only a warning
    T(sg_query_pipeline_state(pip) == SG_RESOURCESTATE_VALID);
    T(logged(SG_LOGITEM_GL_VERTEX_ATTRIBUTE_NOT_FOUND_IN_SHADER));
    T(gl_mock_count_calls(GL_MOCK_FUNC_glGetAttribLocation) == 2);
    sg_destroy_pipeline(pip);
    sg_destroy_shader(shd);
    T(no_errors());
    teardown();
}

//------------------------------------------------------------------------------
//  uniform types -> glUniform*
//------------------------------------------------------------------------------
UTEST(sokol_gfx_gl, apply_uniforms_all_types_native_layout) {
    setup();
    sg_shader shd = sg_make_shader(&(sg_shader_desc){
        .vertex_func.source = "vs", .fragment_func.source = "fs",
        .uniform_blocks[0] = {
            .stage = SG_SHADERSTAGE_VERTEX,
            .size = 176,
            .layout = SG_UNIFORMLAYOUT_NATIVE,
            .glsl_uniforms = {
                [0] = { .type = SG_UNIFORMTYPE_FLOAT,  .glsl_name = "u_f1" },
                [1] = { .type = SG_UNIFORMTYPE_FLOAT2, .glsl_name = "u_f2" },
                [2] = { .type = SG_UNIFORMTYPE_FLOAT3, .glsl_name = "u_f3" },
                [3] = { .type = SG_UNIFORMTYPE_FLOAT4, .glsl_name = "u_f4" },
                [4] = { .type = SG_UNIFORMTYPE_INT,    .glsl_name = "u_i1" },
                [5] = { .type = SG_UNIFORMTYPE_INT2,   .glsl_name = "u_i2" },
                [6] = { .type = SG_UNIFORMTYPE_INT3,   .glsl_name = "u_i3" },
                [7] = { .type = SG_UNIFORMTYPE_INT4,   .glsl_name = "u_i4" },
                [8] = { .type = SG_UNIFORMTYPE_MAT4,   .glsl_name = "u_m4" },
                [9] = { .type = SG_UNIFORMTYPE_FLOAT4, .array_count = 2, .glsl_name = "u_f4arr" },
            },
        },
    });
    T(sg_query_shader_state(shd) == SG_RESOURCESTATE_VALID);
    sg_pipeline pip = sg_make_pipeline(&(sg_pipeline_desc){ .shader = shd });
    // fill the uniform data so that each member is recognizable
    union { float f[44]; int32_t i[44]; } ub;
    memset(&ub, 0, sizeof(ub));
    ub.f[0] = 1.0f;                     // u_f1 @ 0
    ub.f[1] = 2.0f; ub.f[2] = 2.5f;     // u_f2 @ 4
    ub.f[3] = 3.0f;                     // u_f3 @ 12
    ub.f[6] = 4.0f;                     // u_f4 @ 24
    ub.i[10] = 5;                       // u_i1 @ 40
    ub.i[11] = 6;                       // u_i2 @ 44
    ub.i[13] = 7;                       // u_i3 @ 52
    ub.i[16] = 8;                       // u_i4 @ 64
    ub.f[20] = 9.0f;                    // u_m4 @ 80
    ub.f[36] = 10.0f;                   // u_f4arr @ 144
    sg_begin_pass(&(sg_pass){ .swapchain = test_swapchain });
    sg_apply_pipeline(pip);
    gl_mock_clear_calls();
    sg_apply_uniforms(0, &(sg_range){ &ub, 176 });
    const gl_mock_call_t* c;
    TA((c = gl_mock_last_call(GL_MOCK_FUNC_glUniform1fv)) != 0);
    T(c->args[1].i == 1 && c->args[2].f == 1.0);
    TA((c = gl_mock_last_call(GL_MOCK_FUNC_glUniform2fv)) != 0);
    T(c->args[2].f == 2.0 && c->args[3].f == 2.5);
    TA((c = gl_mock_last_call(GL_MOCK_FUNC_glUniform3fv)) != 0);
    T(c->args[2].f == 3.0);
    T(gl_mock_count_calls(GL_MOCK_FUNC_glUniform4fv) == 2);
    TA((c = gl_mock_call(gl_mock_find_call(GL_MOCK_FUNC_glUniform4fv, 0))) != 0);
    T(c->args[1].i == 1 && c->args[2].f == 4.0);
    TA((c = gl_mock_last_call(GL_MOCK_FUNC_glUniform4fv)) != 0);
    T(c->args[1].i == 2 && c->args[2].f == 10.0);
    TA((c = gl_mock_last_call(GL_MOCK_FUNC_glUniform1iv)) != 0);
    T(c->args[2].i == 5);
    TA((c = gl_mock_last_call(GL_MOCK_FUNC_glUniform2iv)) != 0);
    T(c->args[2].i == 6);
    TA((c = gl_mock_last_call(GL_MOCK_FUNC_glUniform3iv)) != 0);
    T(c->args[2].i == 7);
    TA((c = gl_mock_last_call(GL_MOCK_FUNC_glUniform4iv)) != 0);
    T(c->args[2].i == 8);
    TA((c = gl_mock_last_call(GL_MOCK_FUNC_glUniformMatrix4fv)) != 0);
    T(c->args[1].i == 1 && c->args[2].i == GL_FALSE && c->args[3].f == 9.0);
    sg_end_pass();
    sg_commit();
    sg_destroy_pipeline(pip);
    sg_destroy_shader(shd);
    T(no_errors());
    teardown();
}

UTEST(sokol_gfx_gl, apply_uniforms_std140_and_missing_uniform) {
    setup();
    // the first uniform location lookup fails: a warning at creation, skipped in apply_uniforms
    gl_mock_fail_next_uniform_location(1);
    sg_shader shd = sg_make_shader(&(sg_shader_desc){
        .vertex_func.source = "vs", .fragment_func.source = "fs",
        .uniform_blocks[0] = {
            .stage = SG_SHADERSTAGE_FRAGMENT,
            .size = 48,
            .layout = SG_UNIFORMLAYOUT_STD140,
            .glsl_uniforms = {
                [0] = { .type = SG_UNIFORMTYPE_FLOAT,  .glsl_name = "missing" },  // @ 0
                [1] = { .type = SG_UNIFORMTYPE_FLOAT3, .glsl_name = "u_f3" },     // @ 16
                [2] = { .type = SG_UNIFORMTYPE_INT,    .glsl_name = "u_i1" },     // @ 28
                [3] = { .type = SG_UNIFORMTYPE_FLOAT2, .glsl_name = "u_f2" },     // @ 32
            },
        },
    });
    T(sg_query_shader_state(shd) == SG_RESOURCESTATE_VALID);
    T(logged(SG_LOGITEM_GL_UNIFORMBLOCK_NAME_NOT_FOUND_IN_SHADER));
    sg_pipeline pip = sg_make_pipeline(&(sg_pipeline_desc){ .shader = shd });
    union { float f[12]; int32_t i[12]; } ub;
    memset(&ub, 0, sizeof(ub));
    ub.f[4] = 3.0f;
    ub.i[7] = 7;
    ub.f[8] = 2.0f;
    sg_begin_pass(&(sg_pass){ .swapchain = test_swapchain });
    sg_apply_pipeline(pip);
    gl_mock_clear_calls();
    sg_apply_uniforms(0, &(sg_range){ &ub, sizeof(ub) });
    T(gl_mock_count_calls(GL_MOCK_FUNC_glUniform1fv) == 0);
    const gl_mock_call_t* c;
    TA((c = gl_mock_last_call(GL_MOCK_FUNC_glUniform3fv)) != 0);
    T(c->args[2].f == 3.0);
    TA((c = gl_mock_last_call(GL_MOCK_FUNC_glUniform1iv)) != 0);
    T(c->args[2].i == 7);
    TA((c = gl_mock_last_call(GL_MOCK_FUNC_glUniform2fv)) != 0);
    T(c->args[2].f == 2.0);
    sg_end_pass();
    sg_commit();
    sg_destroy_pipeline(pip);
    sg_destroy_shader(shd);
    T(no_errors());
    teardown();
}

UTEST(sokol_gfx_gl, shader_texture_sampler_name_not_found) {
    setup();
    gl_mock_fail_next_uniform_location(1);
    sg_shader shd = sg_make_shader(&(sg_shader_desc){
        .vertex_func.source = "vs", .fragment_func.source = "fs",
        .views[0].texture = { .stage = SG_SHADERSTAGE_FRAGMENT, .image_type = SG_IMAGETYPE_2D, .sample_type = SG_IMAGESAMPLETYPE_FLOAT },
        .samplers[0] = { .stage = SG_SHADERSTAGE_FRAGMENT, .sampler_type = SG_SAMPLERTYPE_FILTERING },
        .texture_sampler_pairs[0] = { .stage = SG_SHADERSTAGE_FRAGMENT, .view_slot = 0, .sampler_slot = 0, .glsl_name = "tex" },
    });
    T(sg_query_shader_state(shd) == SG_RESOURCESTATE_VALID);
    T(logged(SG_LOGITEM_GL_IMAGE_SAMPLER_NAME_NOT_FOUND_IN_SHADER));
    sg_destroy_shader(shd);
    T(no_errors());
    teardown();
}

//------------------------------------------------------------------------------
//  framebuffer completeness errors
//------------------------------------------------------------------------------
UTEST(sokol_gfx_gl, framebuffer_status_errors) {
    const struct { GLenum status; sg_log_item item; } cases[] = {
        { GL_FRAMEBUFFER_UNDEFINED, SG_LOGITEM_GL_FRAMEBUFFER_STATUS_UNDEFINED },
        { GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT, SG_LOGITEM_GL_FRAMEBUFFER_STATUS_INCOMPLETE_ATTACHMENT },
        { GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT, SG_LOGITEM_GL_FRAMEBUFFER_STATUS_INCOMPLETE_MISSING_ATTACHMENT },
        { GL_FRAMEBUFFER_UNSUPPORTED, SG_LOGITEM_GL_FRAMEBUFFER_STATUS_UNSUPPORTED },
        { GL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE, SG_LOGITEM_GL_FRAMEBUFFER_STATUS_INCOMPLETE_MULTISAMPLE },
        { 0x1234, SG_LOGITEM_GL_FRAMEBUFFER_STATUS_UNKNOWN },
    };
    setup();
    sg_image img = sg_make_image(&(sg_image_desc){
        .width = 16, .height = 16, .pixel_format = SG_PIXELFORMAT_RGBA8, .usage.color_attachment = true,
    });
    sg_view att = sg_make_view(&(sg_view_desc){ .color_attachment.image = img });
    for (size_t i = 0; i < sizeof(cases) / sizeof(cases[0]); i++) {
        reset_log();
        gl_mock_set_framebuffer_status(cases[i].status);
        sg_begin_pass(&(sg_pass){ .attachments.colors[0] = att });
        T(logged(cases[i].item));
        sg_end_pass();
    }
    // a complete framebuffer works again
    gl_mock_set_framebuffer_status(GL_FRAMEBUFFER_COMPLETE);
    reset_log();
    sg_begin_pass(&(sg_pass){ .attachments.colors[0] = att });
    sg_end_pass();
    sg_commit();
    T(no_errors());
    sg_destroy_view(att);
    sg_destroy_image(img);
    teardown();
}

//------------------------------------------------------------------------------
//  pass details: sRGB swapchain, depth-only and stencil-only clears,
//  cube and array color attachments
//------------------------------------------------------------------------------
UTEST(sokol_gfx_gl, swapchain_pass_srgb_and_partial_clears) {
    setup();
    sg_swapchain sc = test_swapchain;
    #if defined(SOKOL_GLCORE)
        sc.color_format = SG_PIXELFORMAT_SRGB8A8;
        sg_begin_pass(&(sg_pass){ .swapchain = sc });
        T(gl_mock_is_enabled(GL_FRAMEBUFFER_SRGB));
        sg_end_pass();
        sc.color_format = SG_PIXELFORMAT_RGBA8;
        sg_begin_pass(&(sg_pass){ .swapchain = sc });
        T(!gl_mock_is_enabled(GL_FRAMEBUFFER_SRGB));
        sg_end_pass();
    #endif
    // depth-only clear
    gl_mock_clear_calls();
    sg_begin_pass(&(sg_pass){
        .swapchain = sc,
        .action = {
            .colors[0].load_action = SG_LOADACTION_LOAD,
            .depth = { .load_action = SG_LOADACTION_CLEAR, .clear_value = 0.5f },
            .stencil.load_action = SG_LOADACTION_LOAD,
        },
    });
    int idx = find_call_arg0(GL_MOCK_FUNC_glClearBufferfv, 0, GL_DEPTH);
    T(idx >= 0);
    T(gl_mock_count_calls(GL_MOCK_FUNC_glClearBufferfi) == 0);
    sg_end_pass();
    // stencil-only clear
    gl_mock_clear_calls();
    sg_begin_pass(&(sg_pass){
        .swapchain = sc,
        .action = {
            .colors[0].load_action = SG_LOADACTION_LOAD,
            .depth.load_action = SG_LOADACTION_LOAD,
            .stencil = { .load_action = SG_LOADACTION_CLEAR, .clear_value = 7 },
        },
    });
    idx = find_call_arg0(GL_MOCK_FUNC_glClearBufferiv, 0, GL_STENCIL);
    T(idx >= 0);
    T(gl_mock_count_calls(GL_MOCK_FUNC_glClearBufferfi) == 0);
    sg_end_pass();
    sg_commit();
    T(no_errors());
    teardown();
}

UTEST(sokol_gfx_gl, cube_and_array_color_attachments) {
    setup();
    sg_image cube = sg_make_image(&(sg_image_desc){
        .type = SG_IMAGETYPE_CUBE, .width = 16, .height = 16,
        .pixel_format = SG_PIXELFORMAT_RGBA8, .usage.color_attachment = true,
    });
    sg_image arr = sg_make_image(&(sg_image_desc){
        .type = SG_IMAGETYPE_ARRAY, .width = 16, .height = 16, .num_slices = 4,
        .pixel_format = SG_PIXELFORMAT_RGBA8, .usage.color_attachment = true,
    });
    sg_view cube_att = sg_make_view(&(sg_view_desc){ .color_attachment = { .image = cube, .slice = 3 } });
    sg_view arr_att = sg_make_view(&(sg_view_desc){ .color_attachment = { .image = arr, .slice = 2 } });
    const GLuint gl_cube = sg_gl_query_image_info(cube).tex[0];
    const GLuint gl_arr = sg_gl_query_image_info(arr).tex[0];
    gl_mock_clear_calls();
    sg_begin_pass(&(sg_pass){ .attachments.colors[0] = cube_att });
    sg_end_pass();
    // find the attach call (unused attachments are explicitly detached afterwards)
    const gl_mock_call_t* c = 0;
    for (int i = gl_mock_find_call(GL_MOCK_FUNC_glFramebufferTexture2D, 0); i >= 0; i = gl_mock_find_call(GL_MOCK_FUNC_glFramebufferTexture2D, i + 1)) {
        if (gl_mock_call(i)->args[3].u == gl_cube) {
            c = gl_mock_call(i);
            break;
        }
    }
    TA(c != 0);
    T(c->args[1].i == GL_COLOR_ATTACHMENT0);
    T(c->args[2].i == GL_TEXTURE_CUBE_MAP_POSITIVE_X + 3);
    gl_mock_clear_calls();
    sg_begin_pass(&(sg_pass){ .attachments.colors[0] = arr_att });
    sg_end_pass();
    c = gl_mock_last_call(GL_MOCK_FUNC_glFramebufferTextureLayer);
    TA(c != 0);
    T(c->args[1].i == GL_COLOR_ATTACHMENT0);
    T(c->args[2].i == gl_arr);
    T(c->args[3].i == 0);
    T(c->args[4].i == 2);
    sg_commit();
    sg_destroy_view(cube_att);
    sg_destroy_view(arr_att);
    sg_destroy_image(cube);
    sg_destroy_image(arr);
    T(no_errors());
    teardown();
}

//------------------------------------------------------------------------------
//  samplers: wrap modes, filters, border colors, anisotropy clamping
//------------------------------------------------------------------------------
UTEST(sokol_gfx_gl, sampler_wrap_filter_border_aniso) {
    setup();
    #if defined(SOKOL_GLCORE)
        const GLint clamp_to_border = GL_CLAMP_TO_BORDER;
    #else
        const GLint clamp_to_border = GL_CLAMP_TO_EDGE;
    #endif
    sg_sampler s0 = sg_make_sampler(&(sg_sampler_desc){
        .wrap_u = SG_WRAP_REPEAT, .wrap_v = SG_WRAP_MIRRORED_REPEAT, .wrap_w = SG_WRAP_CLAMP_TO_BORDER,
        .min_filter = SG_FILTER_NEAREST, .mipmap_filter = SG_FILTER_LINEAR, .mag_filter = SG_FILTER_LINEAR,
        .border_color = SG_BORDERCOLOR_OPAQUE_WHITE,
    });
    gl_mock_sampler_info_t si;
    TA(gl_mock_sampler_info(sg_gl_query_sampler_info(s0).smp, &si));
    T(si.wrap_s == GL_REPEAT);
    T(si.wrap_t == GL_MIRRORED_REPEAT);
    T(si.wrap_r == clamp_to_border);
    T(si.min_filter == GL_NEAREST_MIPMAP_LINEAR);
    T(si.mag_filter == GL_LINEAR);
    #if defined(SOKOL_GLCORE)
        T(si.border_color[0] == 1.0f && si.border_color[3] == 1.0f);
    #endif
    sg_sampler s1 = sg_make_sampler(&(sg_sampler_desc){
        .min_filter = SG_FILTER_LINEAR, .mipmap_filter = SG_FILTER_NEAREST,
        .wrap_u = SG_WRAP_CLAMP_TO_BORDER,
        .border_color = SG_BORDERCOLOR_TRANSPARENT_BLACK,
    });
    TA(gl_mock_sampler_info(sg_gl_query_sampler_info(s1).smp, &si));
    T(si.min_filter == GL_LINEAR_MIPMAP_NEAREST);
    T(si.wrap_s == clamp_to_border);
    #if defined(SOKOL_GLCORE)
        T(si.border_color[0] == 0.0f && si.border_color[3] == 0.0f);
    #endif
    // anisotropy is clamped to GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT (16 in the mock)
    sg_sampler s2 = sg_make_sampler(&(sg_sampler_desc){
        .min_filter = SG_FILTER_LINEAR, .mag_filter = SG_FILTER_LINEAR, .mipmap_filter = SG_FILTER_LINEAR,
        .max_anisotropy = 32,
    });
    TA(gl_mock_sampler_info(sg_gl_query_sampler_info(s2).smp, &si));
    T(si.min_filter == GL_LINEAR_MIPMAP_LINEAR);
    T(si.max_anisotropy == 16.0f);
    sg_destroy_sampler(s0);
    sg_destroy_sampler(s1);
    sg_destroy_sampler(s2);
    T(no_errors());
    teardown();
}

//------------------------------------------------------------------------------
//  all pixel formats: texture creation with initial data / as attachment
//------------------------------------------------------------------------------
UTEST(sokol_gfx_gl, image_all_pixel_formats) {
    setup();
    static uint8_t data[4 * 4 * 16 * 2];
    int num_tested = 0;
    for (int i = SG_PIXELFORMAT_NONE + 1; i < _SG_PIXELFORMAT_NUM; i++) {
        const sg_pixel_format fmt = (sg_pixel_format)i;
        const sg_pixelformat_info info = sg_query_pixelformat(fmt);
        if (info.depth) {
            if (info.render) {
                sg_image img = sg_make_image(&(sg_image_desc){
                    .width = 4, .height = 4, .pixel_format = fmt, .usage.depth_stencil_attachment = true,
                });
                T(sg_query_image_state(img) == SG_RESOURCESTATE_VALID);
                sg_destroy_image(img);
                num_tested++;
            }
            continue;
        }
        if (!info.sample) {
            continue;
        }
        const int pitch = sg_query_surface_pitch(fmt, 4, 4, 1);
        TA((pitch > 0) && (pitch * 2 <= (int)sizeof(data)));
        gl_mock_clear_calls();
        // 2D with initial data
        sg_image img = sg_make_image(&(sg_image_desc){
            .width = 4, .height = 4, .pixel_format = fmt,
            .data.mip_levels[0] = { .ptr = data, .size = (size_t)pitch },
        });
        T(sg_query_image_state(img) == SG_RESOURCESTATE_VALID);
        gl_mock_texture_info_t ti;
        TA(gl_mock_texture_info(sg_gl_query_image_info(img).tex[0], &ti));
        T(ti.internal_format != 0);
        if (info.compressed) {
            T((gl_mock_count_calls(GL_MOCK_FUNC_glCompressedTexImage2D) + gl_mock_count_calls(GL_MOCK_FUNC_glCompressedTexSubImage2D)) == 1);
        } else {
            T((gl_mock_count_calls(GL_MOCK_FUNC_glTexImage2D) + gl_mock_count_calls(GL_MOCK_FUNC_glTexSubImage2D)) == 1);
        }
        sg_destroy_image(img);
        // 2D array with initial data
        gl_mock_clear_calls();
        img = sg_make_image(&(sg_image_desc){
            .type = SG_IMAGETYPE_ARRAY, .width = 4, .height = 4, .num_slices = 2, .pixel_format = fmt,
            .data.mip_levels[0] = { .ptr = data, .size = (size_t)(pitch * 2) },
        });
        T(sg_query_image_state(img) == SG_RESOURCESTATE_VALID);
        if (info.compressed) {
            T((gl_mock_count_calls(GL_MOCK_FUNC_glCompressedTexImage3D) + gl_mock_count_calls(GL_MOCK_FUNC_glCompressedTexSubImage3D)) == 1);
        } else {
            T((gl_mock_count_calls(GL_MOCK_FUNC_glTexImage3D) + gl_mock_count_calls(GL_MOCK_FUNC_glTexSubImage3D)) == 1);
        }
        sg_destroy_image(img);
        // renderable formats as color attachment
        if (info.render) {
            img = sg_make_image(&(sg_image_desc){
                .width = 4, .height = 4, .pixel_format = fmt, .usage.color_attachment = true,
            });
            T(sg_query_image_state(img) == SG_RESOURCESTATE_VALID);
            sg_destroy_image(img);
        }
        num_tested++;
    }
    T(num_tested > 40);
    T(gl_mock_live_objects(GL_MOCK_OBJ_TEXTURE) == 0);
    T(no_errors());
    teardown();
}

//------------------------------------------------------------------------------
//  compute-only paths: storage buffer binding invalidation, GLSL binding limits
//------------------------------------------------------------------------------
#if TEST_HAS_COMPUTE
UTEST(sokol_gfx_gl, destroy_bound_storage_buffer_unbinds_it) {
    setup();
    sg_buffer buf = sg_make_buffer(&(sg_buffer_desc){ .usage.storage_buffer = true, .size = 64 });
    sg_view view = sg_make_view(&(sg_view_desc){ .storage_buffer.buffer = buf });
    sg_shader cs = sg_make_shader(&(sg_shader_desc){
        .compute_func.source = "cs",
        .views[0].storage_buffer = { .stage = SG_SHADERSTAGE_COMPUTE, .readonly = false, .glsl_binding_n = 3 },
    });
    sg_pipeline pip = sg_make_pipeline(&(sg_pipeline_desc){ .shader = cs, .compute = true });
    sg_begin_pass(&(sg_pass){ .compute = true });
    sg_apply_pipeline(pip);
    gl_mock_clear_calls();
    sg_apply_bindings(&(sg_bindings){ .views[0] = view });
    const gl_mock_call_t* c = gl_mock_last_call(GL_MOCK_FUNC_glBindBufferRange);
    TA(c != 0);
    T(c->args[0].i == GL_SHADER_STORAGE_BUFFER);
    T(c->args[1].i == 3);
    T(c->args[2].i == sg_gl_query_buffer_info(buf).buf[0]);
    sg_dispatch(1, 1, 1);
    sg_end_pass();
    // destroy before sg_commit(), which would soft-clear the binding cache
    sg_destroy_view(view);
    gl_mock_clear_calls();
    sg_destroy_buffer(buf);
    // the indexed binding point is reset before the buffer is deleted
    const int idx = find_call_arg0(GL_MOCK_FUNC_glBindBufferBase, 0, GL_SHADER_STORAGE_BUFFER);
    TA(idx >= 0);
    T(gl_mock_call(idx)->args[1].i == 3);
    T(gl_mock_call(idx)->args[2].i == 0);
    T(idx < gl_mock_find_call(GL_MOCK_FUNC_glDeleteBuffers, 0));
    sg_commit();
    sg_destroy_pipeline(pip);
    sg_destroy_shader(cs);
    T(no_errors());
    teardown();
}

UTEST(sokol_gfx_gl, shader_glsl_binding_out_of_range) {
    if (sg_isvalid()) { sg_shutdown(); gl_mock_shutdown(); }
    reset_log();
    gl_mock_setup();
    gl_mock_set_int(GL_MAX_SHADER_STORAGE_BUFFER_BINDINGS, 4);
    gl_mock_set_int(GL_MAX_IMAGE_UNITS, 2);
    sg_setup(&(sg_desc){ .logger.func = capture_log });
    T(sg_query_limits().max_storage_buffer_bindings_per_stage == 4);
    sg_shader s0 = sg_make_shader(&(sg_shader_desc){
        .compute_func.source = "cs",
        .views[0].storage_buffer = { .stage = SG_SHADERSTAGE_COMPUTE, .glsl_binding_n = 5 },
    });
    T(sg_query_shader_state(s0) == SG_RESOURCESTATE_FAILED);
    T(logged(SG_LOGITEM_GL_STORAGEBUFFER_GLSL_BINDING_OUT_OF_RANGE));
    sg_shader s1 = sg_make_shader(&(sg_shader_desc){
        .compute_func.source = "cs",
        .views[0].storage_image = {
            .stage = SG_SHADERSTAGE_COMPUTE, .image_type = SG_IMAGETYPE_2D,
            .access_format = SG_PIXELFORMAT_RGBA8, .glsl_binding_n = 3,
        },
    });
    T(sg_query_shader_state(s1) == SG_RESOURCESTATE_FAILED);
    T(logged(SG_LOGITEM_GL_STORAGEIMAGE_GLSL_BINDING_OUT_OF_RANGE));
    // no GL program objects leak from the failed shaders
    T(gl_mock_live_objects(GL_MOCK_OBJ_PROGRAM) == 0);
    sg_destroy_shader(s0);
    sg_destroy_shader(s1);
    teardown();
}
#endif

//------------------------------------------------------------------------------
//  no-leaks full lifecycle
//------------------------------------------------------------------------------
UTEST(sokol_gfx_gl, no_leaks_full_lifecycle) {
    setup();
    sg_shader shd = make_test_shader();
    sg_pipeline pip = sg_make_pipeline(&(sg_pipeline_desc){
        .shader = shd, .layout.attrs[0].format = SG_VERTEXFORMAT_FLOAT3,
    });
    sg_buffer buf = sg_make_buffer(&(sg_buffer_desc){ .size = 64, .usage.write_transient = true });
    sg_image color = sg_make_image(&(sg_image_desc){
        .width = 32, .height = 32, .pixel_format = SG_PIXELFORMAT_RGBA8,
        .usage.color_attachment = true,
    });
    sg_view v = sg_make_view(&(sg_view_desc){ .color_attachment.image = color });
    sg_destroy_view(v);
    sg_destroy_image(color);
    sg_destroy_buffer(buf);
    sg_destroy_pipeline(pip);
    sg_destroy_shader(shd);
    teardown();
    // sokol shutdown must have released every GL object it created
    T(gl_mock_live_objects_total() == 0);
}

UTEST_MAIN()
