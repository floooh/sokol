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
//    - Resource updates                            _sg_gl_update_* / _sg_gl_append_*
//    - Injected native handles                     sg_*_desc.gl_buffers / .gl_textures / .gl_sampler
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

static void setup(void) {
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

UTEST(sokol_gfx_gl, dynamic_image_uses_renaming_slots) {
    setup();
    // a non-immutable image is created with SG_NUM_INFLIGHT_FRAMES textures
    sg_image img = sg_make_image(&(sg_image_desc){
        .width = 8,
        .height = 8,
        .pixel_format = SG_PIXELFORMAT_RGBA8,
        .usage.dynamic_update = true,
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
    sg_image img = sg_make_image(&(sg_image_desc){ .width = 4, .height = 4, .usage.dynamic_update = true });
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
UTEST(sokol_gfx_gl, buffer_dynamic_uses_dynamic_draw) {
    setup();
    sg_buffer buf = sg_make_buffer(&(sg_buffer_desc){
        .size = 256, .usage.dynamic_update = true,
    });
    T(sg_query_buffer_state(buf) == SG_RESOURCESTATE_VALID);
    // dynamic buffer uses SG_NUM_INFLIGHT_FRAMES renaming slots
    T(gl_mock_live_objects(GL_MOCK_OBJ_BUFFER) == SG_NUM_INFLIGHT_FRAMES);
    const gl_mock_call_t* call = gl_mock_last_call(GL_MOCK_FUNC_glBufferData);
    TA(call != 0);
    T(call->args[3].i == GL_DYNAMIC_DRAW);
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
        .usage.immutable = true,
        .gl_buffers = { injected_buf },
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
        .usage.dynamic_update = true,
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
        .usage.immutable = true,
        .gl_textures = { injected_tex },
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
//  resource updates -- glBufferSubData, glTexSubImage2D, append + rename
//------------------------------------------------------------------------------
UTEST(sokol_gfx_gl, buffer_update_calls_bufferSubData) {
    setup();
    sg_buffer buf = sg_make_buffer(&(sg_buffer_desc){
        .size = 64, .usage.dynamic_update = true,
    });
    T(sg_query_buffer_state(buf) == SG_RESOURCESTATE_VALID);
    uint8_t bytes[64] = {0};
    gl_mock_clear_calls();
    sg_update_buffer(buf, &SG_RANGE(bytes));
    T(gl_mock_count_calls(GL_MOCK_FUNC_glBufferSubData) == 1);
    const gl_mock_call_t* call = gl_mock_last_call(GL_MOCK_FUNC_glBufferSubData);
    T(call->args[1].i == 0);                    // offset
    T(call->args[2].i == (int64_t)sizeof(bytes));
    sg_destroy_buffer(buf);
    teardown();
}

UTEST(sokol_gfx_gl, buffer_append_uses_offset) {
    setup();
    sg_buffer buf = sg_make_buffer(&(sg_buffer_desc){
        .size = 256, .usage.dynamic_update = true,
    });
    uint8_t bytes[32] = {0};
    int off1 = sg_append_buffer(buf, &SG_RANGE(bytes));
    int off2 = sg_append_buffer(buf, &SG_RANGE(bytes));
    T(off1 == 0);
    T(off2 == 32);
    // second append uses non-zero offset in glBufferSubData
    const gl_mock_call_t* call = gl_mock_last_call(GL_MOCK_FUNC_glBufferSubData);
    TA(call != 0);
    T(call->args[1].i == 32);
    sg_destroy_buffer(buf);
    teardown();
}

UTEST(sokol_gfx_gl, image_update_calls_texSubImage2D) {
    setup();
    sg_image img = sg_make_image(&(sg_image_desc){
        .width = 8, .height = 8, .pixel_format = SG_PIXELFORMAT_RGBA8,
        .usage.dynamic_update = true,
    });
    uint32_t pixels[8 * 8] = {0};
    gl_mock_clear_calls();
    sg_update_image(img, &(sg_image_data){ .mip_levels[0] = SG_RANGE(pixels) });
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
//  no-leaks full lifecycle
//------------------------------------------------------------------------------
UTEST(sokol_gfx_gl, no_leaks_full_lifecycle) {
    setup();
    sg_shader shd = make_test_shader();
    sg_pipeline pip = sg_make_pipeline(&(sg_pipeline_desc){
        .shader = shd, .layout.attrs[0].format = SG_VERTEXFORMAT_FLOAT3,
    });
    sg_buffer buf = sg_make_buffer(&(sg_buffer_desc){ .size = 64, .usage.dynamic_update = true });
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
