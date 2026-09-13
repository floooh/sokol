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
//    - Resource create / destroy round trip        _sg_gl_create_* / _sg_gl_discard_*
//    - Shader compile and link failure             _sg_gl_compile_shader / _sg_gl_create_shader
//------------------------------------------------------------------------------
#define SOKOL_IMPL
#define SOKOL_EXTERNAL_GL_LOADER
#include "gl_mock.h"
#include "sokol_gfx.h"
#include "utest.h"
#include <string.h>

#define T(b) EXPECT_TRUE(b)

#if defined(SOKOL_GLCORE)
    #define EXPECTED_BACKEND SG_BACKEND_GLCORE
#else
    #define EXPECTED_BACKEND SG_BACKEND_GLES3
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
    T(call != 0);
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
    T(call != 0);
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

UTEST_MAIN()
