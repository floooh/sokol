#pragma once
/*
    sokol_gl.h -- OpenGL 1.x style rendering on top of sokol_gfx.h

    WIP! Progress will be slow.

    Do this:
        #define SOKOL_GL_IMPL
    before you include this file in *one* C or C++ file to create the
    implementation.

    The following defines are used by the implementation to select the
    platform-specific embedded shader code (these are the same defines as
    used by sokol_gfx.h and sokol_app.h):

    SOKOL_GLCORE33
    SOKOL_GLES2
    SOKOL_GLES3
    SOKOL_D3D11
    SOKOL_METAL

    ...optionally provide the following macros to override defaults:

    SOKOL_ASSERT(c)     - your own assert macro (default: assert(c))
    SOKOL_MALLOC(s)     - your own malloc function (default: malloc(s))
    SOKOL_FREE(p)       - your own free function (default: free(p))
    SOKOL_API_DECL      - public function declaration prefix (default: extern)
    SOKOL_API_IMPL      - public function implementation prefix (default: -)
    SOKOL_UNREACHABLE() - a guard macro for unreachable code (default: assert(false))

    Include the following headers before including sokol_gl.h:

        sokol_gfx.h

    Matrix functions are taken from MESA and Regal.

    FEATURE OVERVIEW:
    =================
    FIXME

    HOWTO:
    ======
    FIXME

    LICENSE
    =======

    zlib/libpng license

    Copyright (c) 2018 Andre Weissflog

    This software is provided 'as-is', without any express or implied warranty.
    In no event will the authors be held liable for any damages arising from the
    use of this software.

    Permission is granted to anyone to use this software for any purpose,
    including commercial applications, and to alter it and redistribute it
    freely, subject to the following restrictions:

        1. The origin of this software must not be misrepresented; you must not
        claim that you wrote the original software. If you use this software in a
        product, an acknowledgment in the product documentation would be
        appreciated but is not required.

        2. Altered source versions must be plainly marked as such, and must not
        be misrepresented as being the original software.

        3. This notice may not be removed or altered from any source
        distribution.
*/
#define SOKOL_GL_INCLUDED (1)
#include <stdint.h>
#include <stdbool.h>

#if !defined(SOKOL_GFX_INCLUDED)
#error "Please include sokol_gfx.h before sokol_gl.h"
#endif

#ifndef SOKOL_API_DECL
    #define SOKOL_API_DECL extern
#endif

/*
    sgl_error_t

    Errors are reset each frame after calling sgl_draw(),
    get the last error code with sgl_error()
*/
typedef enum sgl_error_t {
    SGL_NO_ERROR = 0,
    SGL_ERROR_VERTICES_FULL,
    SGL_ERROR_UNIFORMS_FULL,
    SGL_ERROR_COMMANDS_FULL,
    SGL_ERROR_STACK_OVERFLOW,
    SGL_ERROR_STACK_UNDERFLOW,
} sgl_error_t;

typedef struct sgl_desc_t {
    int max_vertices;   /* size for vertex buffer */
    int max_commands;   /* size of uniform- and command-buffers */
    sg_pixel_format color_format;
    sg_pixel_format depth_format;
    int sample_count;
} sgl_desc_t;

/* setup/shutdown */
SOKOL_API_DECL void sgl_setup(const sgl_desc_t* desc);
SOKOL_API_DECL void sgl_shutdown(void);
SOKOL_API_DECL sgl_error_t sgl_error(void);

/* render state functions (only valid outside begin/end) */
SOKOL_API_DECL void sgl_default_state(void);
SOKOL_API_DECL void sgl_enable_depth_test(void);
SOKOL_API_DECL void sgl_enable_blend(void);
SOKOL_API_DECL void sgl_enable_cull_face(void);
SOKOL_API_DECL void sgl_enable_texture(void);
SOKOL_API_DECL void sgl_disable_depth_test(void);
SOKOL_API_DECL void sgl_disable_blend(void);
SOKOL_API_DECL void sgl_disable_cull_face(void);
SOKOL_API_DECL void sgl_disable_texture(void);
SOKOL_API_DECL void sgl_viewport(int x, int y, int w, int h, bool origin_top_left);
SOKOL_API_DECL void sgl_scissor_rect(int x, int y, int w, int h, bool origin_top_left);
SOKOL_API_DECL void sgl_texture(sg_image img);
SOKOL_API_DECL void sgl_texcoord_int_bits(int u_bits, int v_bits);

/* degree-radian conversion */
SOKOL_API_DECL float sgl_rad(float deg);
SOKOL_API_DECL float sgl_deg(float rad);

/* these functions only set the internal 'current texcoord / color' (valid inside or outside begin/end) */
SOKOL_API_DECL void sgl_t2f(float u, float v);
SOKOL_API_DECL void sgl_c3f(float r, float g, float b);
SOKOL_API_DECL void sgl_c4f(float r, float g, float b, float a);
SOKOL_API_DECL void sgl_c3b(uint8_t r, uint8_t g, uint8_t b);
SOKOL_API_DECL void sgl_c4b(uint8_t r, uint8_t g, uint8_t b, uint8_t a);
SOKOL_API_DECL void sgl_c1i(uint32_t rgba);

/* define primitives, each begin/end is one draw command */
SOKOL_API_DECL void sgl_begin_points(void);
SOKOL_API_DECL void sgl_begin_lines(void);
SOKOL_API_DECL void sgl_begin_line_strip(void);
SOKOL_API_DECL void sgl_begin_triangles(void);
SOKOL_API_DECL void sgl_begin_triangle_strip(void);
SOKOL_API_DECL void sgl_begin_quads(void);
SOKOL_API_DECL void sgl_v2f(float x, float y);
SOKOL_API_DECL void sgl_v3f(float x, float y, float z);
SOKOL_API_DECL void sgl_v2f_t2f(float x, float y, float u, float v);
SOKOL_API_DECL void sgl_v3f_t2f(float x, float y, float z, float u, float v);
SOKOL_API_DECL void sgl_v2f_c3f(float x, float y, float r, float g, float b);
SOKOL_API_DECL void sgl_v2f_c3b(float x, float y, uint8_t r, uint8_t g, uint8_t b);
SOKOL_API_DECL void sgl_v2f_c4f(float x, float y, float r, float g, float b, float a);
SOKOL_API_DECL void sgl_v2f_c4b(float x, float y, uint8_t r, uint8_t g, uint8_t b, uint8_t a);
SOKOL_API_DECL void sgl_v2f_c1i(float x, float y, uint32_t rgba);
SOKOL_API_DECL void sgl_v3f_c3f(float x, float y, float z, float r, float g, float b);
SOKOL_API_DECL void sgl_v3f_c3b(float x, float y, float z, uint8_t r, uint8_t g, uint8_t b);
SOKOL_API_DECL void sgl_v3f_c4f(float x, float y, float z, float r, float g, float b, float a);
SOKOL_API_DECL void sgl_v3f_c4b(float x, float y, float z, uint8_t r, uint8_t g, uint8_t b, uint8_t a);
SOKOL_API_DECL void sgl_v3f_c1i(float x, float y, float z, uint32_t rgba);
SOKOL_API_DECL void sgl_v2f_t2f_c3f(float x, float y, float u, float v, float r, float g, float b);
SOKOL_API_DECL void sgl_v2f_t2f_c3b(float x, float y, float u, float v, uint8_t r, uint8_t g, uint8_t b);
SOKOL_API_DECL void sgl_v2f_t2f_c4f(float x, float y, float u, float v, float r, float g, float b, float a);
SOKOL_API_DECL void sgl_v2f_t2f_c4b(float x, float y, float u, float v, uint8_t r, uint8_t g, uint8_t b, uint8_t a);
SOKOL_API_DECL void sgl_v2f_t2f_c1i(float x, float y, float u, float v, uint32_t rgba);
SOKOL_API_DECL void sgl_v3f_t2f_c3f(float x, float y, float z, float u, float v, float r, float g, float b);
SOKOL_API_DECL void sgl_v3f_t2f_c3b(float x, float y, float z, float u, float v, uint8_t r, uint8_t g, uint8_t b);
SOKOL_API_DECL void sgl_v3f_t2f_c4f(float x, float y, float z, float u, float v, float r, float g, float b, float a);
SOKOL_API_DECL void sgl_v3f_t2f_c4b(float x, float y, float z, float u, float v, uint8_t r, uint8_t g, uint8_t b, uint8_t a);
SOKOL_API_DECL void sgl_v3f_t2f_c1i(float x, float y, float z, float u, float v, uint32_t rgba);
SOKOL_API_DECL void sgl_end(void);

/* matrix stack functions (only valid outside begin end) */
SOKOL_API_DECL void sgl_matrix_mode_modelview(void);
SOKOL_API_DECL void sgl_matrix_mode_projection(void);
SOKOL_API_DECL void sgl_matrix_mode_texture(void);
SOKOL_API_DECL void sgl_load_identity(void);
SOKOL_API_DECL void sgl_load_matrix(const float m[16]);
SOKOL_API_DECL void sgl_load_transpose_matrix(const float m[16]);
SOKOL_API_DECL void sgl_mult_matrix(const float m[16]);
SOKOL_API_DECL void sgl_mult_transpose_matrix(const float m[16]);
SOKOL_API_DECL void sgl_rotate(float angle_rad, float x, float y, float z);
SOKOL_API_DECL void sgl_scale(float x, float y, float z);
SOKOL_API_DECL void sgl_translate(float x, float y, float z);
SOKOL_API_DECL void sgl_frustum(float l, float r, float b, float t, float n, float f);
SOKOL_API_DECL void sgl_ortho(float l, float r, float b, float t, float n, float f);
SOKOL_API_DECL void sgl_perspective(float fov_y, float aspect, float z_near, float z_far);
SOKOL_API_DECL void sgl_lookat(float eye_x, float eye_y, float eye_z, float center_x, float center_y, float center_z, float up_x, float up_y, float up_z);
SOKOL_API_DECL void sgl_push_matrix(void);
SOKOL_API_DECL void sgl_pop_matrix(void);

/* render everything */
SOKOL_API_DECL void sgl_draw(void);

#ifdef __cplusplus
extern "C" {
#endif

#ifdef __cplusplus
} /* extern "C" */
#endif

/*-- IMPLEMENTATION ----------------------------------------------------------*/
#ifdef SOKOL_GL_IMPL
#define SOKOL_GL_IMPL_INCLUDED (1)

#include <stddef.h> /* offsetof */
#include <string.h> /* memset */
#include <math.h> /* M_PI, sqrtf, sinf, cosf */

#ifndef M_PI
#define M_PI 3.14159265358979323846264338327
#endif

#ifndef SOKOL_API_IMPL
    #define SOKOL_API_IMPL
#endif
#ifndef SOKOL_DEBUG
    #ifndef NDEBUG
        #define SOKOL_DEBUG (1)
    #endif
#endif
#ifndef SOKOL_ASSERT
    #include <assert.h>
    #define SOKOL_ASSERT(c) assert(c)
#endif
#ifndef SOKOL_MALLOC
    #include <stdlib.h>
    #define SOKOL_MALLOC(s) malloc(s)
    #define SOKOL_FREE(p) free(p)
#endif
#ifndef SOKOL_UNREACHABLE
    #define SOKOL_UNREACHABLE SOKOL_ASSERT(false)
#endif

#define _sgl_def(val, def) (((val) == 0) ? (def) : (val))
#define _SGL_INIT_COOKIE (0xABCDABCD)

#if defined(SOKOL_GLCORE33)
static const char* _sgl_vs_src =
    "#version 330\n"
    "uniform mat4 mvp;\n"
    "uniform vec2 uv_scale;\n"
    "in vec4 position;\n"
    "in vec2 texcoord0;\n"
    "in vec4 color0;\n"
    "out vec2 uv;\n"
    "out vec4 color;\n"
    "void main() {\n"
    "    gl_Position = mvp * position;\n"
    "    uv = uv_scale * texcoord0;\n"
    "    color = color0;\n"
    "}\n";
static const char* _sgl_fs_src =
    "#version 330\n"
    "uniform sampler2D tex;\n"
    "in vec2 uv;\n"
    "in vec4 color;\n"
    "out vec4 frag_color;\n"
    "void main() {\n"
    "    frag_color = texture(tex, uv) * color;\n"
    "}\n";
#elif defined(SOKOL_GLES2) || defined(SOKOL_GLES3)
static const char* _sgl_vs_src =
    "uniform mat4 mvp;\n"
    "uniform vec2 uv_scale;\n"
    "attribute vec4 position;\n"
    "attribute vec2 texcoord0;\n"
    "attribute vec4 color0;\n"
    "varying vec2 uv;\n"
    "varying vec4 color;\n"
    "void main() {\n"
    "    gl_Position = mvp * position;\n"
    "    uv = uv_scale * texcoord0;\n"
    "    color = color0;\n"
    "}\n";
static const char* _sgl_fs_src =
    "precision mediump float;\n"
    "uniform sampler2D tex;\n"
    "varying vec2 uv;\n"
    "varying vec4 color;\n"
    "void main() {\n"
    "    gl_FragColor = texture2D(tex, uv) * color;\n"
    "}\n";
#elif defined(SOKOL_METAL)
static const char* _sgl_vs_src =
    "#include <metal_stdlib>\n"
    "using namespace metal;\n"
    "struct params_t {\n"
    "  float4x4 mvp;\n"
    "  float2 uv_scale;\n"
    "};\n"
    "struct vs_in {\n"
    "  float4 pos [[attribute(0)]];\n"
    "  float2 uv [[attribute(1)]];\n"
    "  float4 color [[attribute(2)]];\n"
    "};\n"
    "struct vs_out {\n"
    "  float4 pos [[position]];\n"
    "  float2 uv;\n"
    "  float4 color;\n"
    "};\n"
    "vertex vs_out _main(vs_in in [[stage_in]], constant params_t& params [[buffer(0)]]) {\n"
    "  vs_out out;\n"
    "  out.pos = params.mvp * in.pos;\n"
    "  out.uv = params.uv_scale * in.uv;\n"
    "  out.color = in.color;\n"
    "  return out;\n"
    "}\n";
static const char* _sgl_fs_src =
    "#include <metal_stdlib>\n"
    "using namespace metal;\n"
    "struct fs_in {\n"
    "  float2 uv;\n"
    "  float4 color;\n"
    "};\n"
    "fragment float4 _main(fs_in in [[stage_in]], texture2d<float> tex [[texture(0)]], sampler smp [[sampler(0)]]) {\n"
    "  return tex.sample(smp, in.uv) * in.color;\n"
    "}\n";
#elif defined(SOKOL_D3D11)
static const char* _sgl_vs_src =
    "cbuffer params: register(b0) {\n"
    "  float4x4 mvp;\n"
    "  float2 uv_scale;\n"
    "};\n"
    "struct vs_in {\n"
    "  float4 pos: POS;\n"
    "  float2 uv: TEXCOORD0;\n"
    "  float4 color: COLOR0;\n"
    "};\n"
    "struct vs_out {\n"
    "  float2 uv: TEXCOORD0;\n"
    "  float4 color: COLOR0;\n"
    "  float4 pos: SV_Position;\n"
    "};\n"
    "vs_out main(vs_in inp) {\n"
    "  vs_out outp;\n"
    "  outp.pos = mul(mvp, inp.pos);\n"
    "  outp.uv = uv_scale * inp.uv;\n"
    "  outp.color = inp.color;\n"
    "  return outp;\n"
    "};\n";
static const char* _sgl_fs_src =
    "Texture2D<float4> tex: register(t0);\n"
    "sampler smp: register(s0);\n"
    "float4 main(float2 uv: TEXCOORD0, float4 color: COLOR0): SV_Target0 {\n"
    "  return tex.Sample(smp, uv) * color;\n"
    "}\n";
#elif defined(SOKOL_DUMMY_BACKEND)
static const char* _sgl_vs_src = "";
static const char* _sgl_fs_src = "";
#endif

typedef enum {
    SGL_PRIMITIVETYPE_POINTS = 0,
    SGL_PRIMITIVETYPE_LINES,
    SGL_PRIMITIVETYPE_LINE_STRIP,
    SGL_PRIMITIVETYPE_TRIANGLES,
    SGL_PRIMITIVETYPE_TRIANGLE_STRIP,
    SGL_PRIMITIVETYPE_QUADS,
    SGL_NUM_PRIMITIVE_TYPES,
} _sgl_primitive_type_t;

typedef enum {
    SGL_STATE_DEPTHTEST,
    SGL_STATE_BLEND,
    SGL_STATE_CULLFACE,
    SGL_STATE_TEXTURE,
    SGL_NUM_STATES
} _sgl_state_t;

typedef enum {
    SGL_MATRIXMODE_MODELVIEW,
    SGL_MATRIXMODE_PROJECTION,
    SGL_MATRIXMODE_TEXTURE,
    SGL_NUM_MATRIXMODES
} _sgl_matrix_mode_t;


typedef struct {
    float pos[3];
    int16_t uv[2];  /* texcoords as packed fixed-point format, see sgl_texcoord_int_bits */
    uint32_t rgba;
} _sgl_vertex_t;

typedef struct {
    float v[4][4];
} _sgl_matrix_t;

typedef struct {
    _sgl_matrix_t mvp;  /* model-view-projection matrix */
    float uv_scale[2];  /* scaler for converting fixed-point texcoord to float */
} _sgl_uniform_t;

typedef enum {
    SGL_COMMAND_DRAW,
    SGL_COMMAND_VIEWPORT,
    SGL_COMMAND_SCISSOR_RECT,
} _sgl_command_type_t;

typedef struct {
    sg_image img;
    uint16_t state_bits;    /* bit mask with primitive type and render states */
    int base_vertex;
    int num_vertices;
    int uniform_index;
} _sgl_draw_args_t;

typedef struct {
    int x, y, w, h;
    bool origin_top_left;
} _sgl_viewport_args_t;

typedef struct {
    int x, y, w, h;
    bool origin_top_left;
} _sgl_scissor_rect_args_t;

typedef union {
    _sgl_draw_args_t draw;
    _sgl_viewport_args_t viewport;
    _sgl_scissor_rect_args_t scissor_rect;
} _sgl_args_t;

typedef struct {
    _sgl_command_type_t cmd;
    _sgl_args_t args;
} _sgl_command_t;

/* number of pipelines: 3 bits for primitive type, 3 relevant render state bits */
#define _SGL_MAX_PIPELINES (64)
/* matrix stack depth */
#define _SGL_MAX_STACK_DEPTH (64)

typedef struct {
    uint32_t init_cookie;

    int num_vertices;
    int num_uniforms;
    int num_commands;
    int cur_vertex;
    int cur_uniform;
    int cur_command;
    _sgl_vertex_t* vertices;
    _sgl_uniform_t* uniforms;
    _sgl_command_t* commands;

    /* state tracking */
    int base_vertex;
    int vtx_count;      /* number of times vtx function has been called, used for non-triangle primitives */
    sgl_error_t error;
    bool in_begin;
    uint16_t state_bits;        /* bitmask with primitive type and render states */
    float u_scale, v_scale;
    int16_t u, v;
    uint32_t rgba;
    sg_image cur_img;

    /* sokol-gfx resources */
    sg_buffer vbuf;
    sg_image def_img;   /* a default white texture */
    sg_shader shd;
    sg_bindings bind;
    sg_pipeline pip[_SGL_MAX_PIPELINES];
    sg_pipeline_desc pip_desc;  /* template for lazy pipeline creation */

    /* matrix stacks */
    _sgl_matrix_mode_t cur_matrix_mode;
    int top_of_stack[SGL_NUM_MATRIXMODES];
    _sgl_matrix_t matrix_stack[SGL_NUM_MATRIXMODES][_SGL_MAX_STACK_DEPTH];
} _sgl_t;
static _sgl_t _sgl;

/*== PRIVATE FUNCTIONS =======================================================*/
/* set primitive type in 16-bit merged state */
static inline uint16_t _sgl_set_prim_type(_sgl_primitive_type_t type, uint16_t bits) {
    SOKOL_ASSERT(((int)type) < 8);
    return (bits & ~7) | (type & 7);
}

/* extract primitive type from 16-bit merged state */
static inline _sgl_primitive_type_t _sgl_prim_type(uint16_t bits) {
    return (_sgl_primitive_type_t) (bits & 7);
}

static inline void _sgl_begin(_sgl_primitive_type_t mode) {
    _sgl.in_begin = true;
    _sgl.base_vertex = _sgl.cur_vertex;
    _sgl.vtx_count = 0;
    _sgl.state_bits = _sgl_set_prim_type(mode, _sgl.state_bits);
}

static void _sgl_rewind(void) {
    _sgl.base_vertex = 0;
    _sgl.cur_vertex = 0;
    _sgl.cur_uniform = 0;
    _sgl.cur_command = 0;
    _sgl.error = SGL_NO_ERROR;
}

static inline _sgl_vertex_t* _sgl_next_vertex(void) {
    if (_sgl.cur_vertex < _sgl.num_vertices) {
        return &_sgl.vertices[_sgl.cur_vertex++];
    }
    else {
        _sgl.error = SGL_ERROR_VERTICES_FULL;
        return 0;
    }
}

static inline _sgl_uniform_t* _sgl_next_uniform(void) {
    if (_sgl.cur_uniform < _sgl.num_uniforms) {
        return &_sgl.uniforms[_sgl.cur_uniform++];
    }
    else {
        _sgl.error = SGL_ERROR_UNIFORMS_FULL;
        return 0;
    }
}

static inline _sgl_command_t* _sgl_next_command(void) {
    if (_sgl.cur_command < _sgl.num_commands) {
        return &_sgl.commands[_sgl.cur_command++];
    }
    else {
        _sgl.error = SGL_ERROR_COMMANDS_FULL;
        return 0;
    }
}

static inline int16_t _sgl_pack_u(float u) {
    return (int16_t) ((u * (1<<15)) / _sgl.u_scale);
}

static inline int16_t _sgl_pack_v(float v) {
    return (int16_t) ((v * (1<<15)) / _sgl.v_scale);
}

static inline uint32_t _sgl_pack_rgbab(uint8_t r, uint8_t g, uint8_t b, uint8_t a) {
    return (uint32_t)((a<<24)|(b<<16)|(g<<8)|r);
}

static inline uint32_t _sgl_pack_rgbaf(float r, float g, float b, float a) {
    uint8_t r_u8 = (uint8_t) (r * 255.0f);
    uint8_t g_u8 = (uint8_t) (g * 255.0f);
    uint8_t b_u8 = (uint8_t) (b * 255.0f);
    uint8_t a_u8 = (uint8_t) (a * 255.0f);
    return _sgl_pack_rgbab(r_u8, g_u8, b_u8, a_u8);
}

static inline void _sgl_vtx(float x, float y, float z, int16_t u, int16_t v, uint32_t rgba) {
    SOKOL_ASSERT(_sgl.in_begin);
    _sgl_vertex_t* vtx;
    /* handle non-native primitive types */
    if ((_sgl_prim_type(_sgl.state_bits) == SGL_PRIMITIVETYPE_QUADS) && ((_sgl.vtx_count & 3) == 3)) {
        /* for quads, before writing the last quad vertex, reuse 
           the first and third vertex to start the second triangle in the quad
        */
        vtx = _sgl_next_vertex();
        if (vtx) { *vtx = *(vtx - 3); }
        vtx = _sgl_next_vertex();
        if (vtx) { *vtx = *(vtx - 2); }
    }
    vtx = _sgl_next_vertex();
    if (vtx) {
        vtx->pos[0] = x; vtx->pos[1] = y; vtx->pos[2] = z;
        vtx->uv[0] = u; vtx->uv[1] = v;
        vtx->rgba = rgba;
    }
    _sgl.vtx_count++;
}

/* set render state bit in 16-bit merged state */
static inline uint16_t _sgl_enable_state(_sgl_state_t state, uint16_t bits) {
    /* first 3 bits are used by the primitive type */
    return bits | (8<<state);
}

/* clear render state bit in 16-bit merged state */
static inline uint16_t _sgl_disable_state(_sgl_state_t state, uint16_t bits) {
    /* first 3 bits are used by the primitive type */
    return bits & ~(8<<state);
}

/* get render state from merged state bit mask */
static inline bool _sgl_state(_sgl_state_t state, uint16_t bits) {
    return 0 != (bits & (8<<state));
}

/* get pipeline index from merged primitive-type / render state bit mask */
static inline int _sgl_pipeline_index(uint16_t state) {
    /* replace emulated primitive types */
    if (_sgl_prim_type(state) == SGL_PRIMITIVETYPE_QUADS) {
        state = _sgl_set_prim_type(SGL_PRIMITIVETYPE_TRIANGLES, state);
    }
    return (int) (state & (_SGL_MAX_PIPELINES-1));
}

/* lookup or lazy-create pipeline object */
static sg_pipeline _sgl_pipeline(uint16_t state_bits) {
    /* NOTE: emulated primitive types like QUADS are 'redirected' to 
       a native primitive type in _sgl_pipeline_index
    */
    int pip_index = _sgl_pipeline_index(state_bits);
    if (SG_INVALID_ID == _sgl.pip[pip_index].id) {
        _sgl.pip_desc.blend.enabled = _sgl_state(SGL_STATE_BLEND, state_bits);
        _sgl.pip_desc.rasterizer.cull_mode = _sgl_state(SGL_STATE_CULLFACE, state_bits) ? SG_CULLMODE_BACK : SG_CULLMODE_NONE;
        _sgl.pip_desc.depth_stencil.depth_compare_func = _sgl_state(SGL_STATE_DEPTHTEST, state_bits) ? SG_COMPAREFUNC_LESS_EQUAL : SG_COMPAREFUNC_ALWAYS;
        switch (_sgl_prim_type(state_bits)) {
            case SGL_PRIMITIVETYPE_POINTS: _sgl.pip_desc.primitive_type = SG_PRIMITIVETYPE_POINTS; break;
            case SGL_PRIMITIVETYPE_LINES: _sgl.pip_desc.primitive_type = SG_PRIMITIVETYPE_LINES; break;
            case SGL_PRIMITIVETYPE_LINE_STRIP: _sgl.pip_desc.primitive_type = SG_PRIMITIVETYPE_LINE_STRIP; break;
            case SGL_PRIMITIVETYPE_TRIANGLES: _sgl.pip_desc.primitive_type = SG_PRIMITIVETYPE_TRIANGLES; break;
            case SGL_PRIMITIVETYPE_TRIANGLE_STRIP: _sgl.pip_desc.primitive_type = SG_PRIMITIVETYPE_TRIANGLE_STRIP; break;
            case SGL_PRIMITIVETYPE_QUADS: _sgl.pip_desc.primitive_type = SG_PRIMITIVETYPE_TRIANGLES; break;
            default: SOKOL_UNREACHABLE; break;
        }
        _sgl.pip[pip_index] = sg_make_pipeline(&_sgl.pip_desc);
    }
    return _sgl.pip[pip_index];
}

/* initialize identity matrix */
static void _sgl_identity(_sgl_matrix_t* m) {
    for (int c = 0; c < 4; c++) {
        for (int r = 0; r < 4; r++) {
            m->v[c][r] = (r == c) ? 1.0f : 0.0f;
        }
    }
}

/* multiply matrices, can be in-place if p and a are identical */
static void _sgl_matmul4(_sgl_matrix_t* p, const _sgl_matrix_t* a, const _sgl_matrix_t* b) {
    for (int r = 0; r < 4; r++) {
        float ai0=a->v[0][r], ai1=a->v[1][r], ai2=a->v[2][r], ai3=a->v[3][r];
        p->v[0][r] = ai0*b->v[0][0] + ai1*b->v[0][1] + ai2*b->v[0][2] + ai3*b->v[0][3];
        p->v[1][r] = ai0*b->v[1][0] + ai1*b->v[1][1] + ai2*b->v[1][2] + ai3*b->v[1][3];
        p->v[2][r] = ai0*b->v[2][0] + ai1*b->v[2][1] + ai2*b->v[2][2] + ai3*b->v[2][3];
        p->v[3][r] = ai0*b->v[3][0] + ai1*b->v[3][1] + ai2*b->v[3][2] + ai3*b->v[3][3];
    }
}

/* in-place matrix multiplication */
static void _sgl_mul(_sgl_matrix_t* dst, const _sgl_matrix_t* m) {
    _sgl_matmul4(dst, dst, m);
}

/* return transposed matrix */
static void _sgl_transpose(_sgl_matrix_t* dst, const _sgl_matrix_t* m) {
    SOKOL_ASSERT(dst != m);
    for (int c = 0; c < 4; c++) {
        for (int r = 0; r < 4; r++) {
            dst->v[r][c] = m->v[c][r];
        }
    }
}

static void _sgl_rotate(_sgl_matrix_t* dst, float a, float x, float y, float z) {
    
    float s = sinf(a);
    float c = cosf(a);

    float xx = x * x;
    float yy = y * y;
    float zz = z * z;
    float mag = sqrtf(xx + yy + zz);
    if (mag < 1.0e-4F) {
        return;
    }
    x /= mag;
    y /= mag;
    z /= mag;

    float xy = x * y;
    float yz = y * z;
    float zx = z * x;
    float xs = x * s;
    float ys = y * s;
    float zs = z * s;
    float one_c = 1.0f - c;

    _sgl_matrix_t m;
    m.v[0][0] = (one_c * xx) + c;
    m.v[1][0] = (one_c * xy) - zs;
    m.v[2][0] = (one_c * zx) + ys;
    m.v[3][0] = 0.0f;
    m.v[0][1] = (one_c * xy) + zs;
    m.v[1][1] = (one_c * yy) + c;
    m.v[2][1] = (one_c * yz) - xs;
    m.v[3][1] = 0.0f;
    m.v[0][2] = (one_c * zx) - ys;
    m.v[1][2] = (one_c * yz) + xs;
    m.v[2][2] = (one_c * zz) + c;
    m.v[3][2] = 0.0f;
    m.v[0][3] = 0.0f;
    m.v[1][3] = 0.0f;
    m.v[2][3] = 0.0f;
    m.v[3][3] = 1.0f;
    _sgl_matmul4(dst, dst, &m);
}

static void _sgl_scale(_sgl_matrix_t* dst, float x, float y, float z) {
    for (int r = 0; r < 4; r++) {
        dst->v[0][r] *= x;
        dst->v[1][r] *= y;
        dst->v[2][r] *= z;
    }
}

static void _sgl_translate(_sgl_matrix_t* dst, float x, float y, float z) {
    for (int r = 0; r < 4; r++) {
        dst->v[3][r] = dst->v[0][r]*x + dst->v[1][r]*y + dst->v[2][r]*z + dst->v[3][r];
    }
}

static void _sgl_frustum(_sgl_matrix_t* dst, float left, float right, float bottom, float top, float near, float far) {
    float x = (2.0f * near) / (right - left);
    float y = (2.0f * near) / (top - bottom);
    float a = (right + left) / (right - left);
    float b = (top + bottom) / (top - bottom);
    float c = -(far + near) / (far - near);
    float d = -(2.0f * far * near) / (far - near);
    _sgl_matrix_t m;
    m.v[0][0] = x;    m.v[0][1] = 0.0f; m.v[0][2] = 0.0f; m.v[0][3] = 0.0f;
    m.v[1][0] = 0.0f; m.v[1][1] = y;    m.v[1][2] = 0.0f; m.v[1][3] = 0.0f;
    m.v[2][0] = a;    m.v[2][1] = b;    m.v[2][2] = c;    m.v[2][3] = -1.0f;
    m.v[3][0] = 0.0f; m.v[3][1] = 0.0f; m.v[3][2] = d;    m.v[3][3] = 0.0f;
    _sgl_matmul4(dst, dst, &m);
}

static void _sgl_ortho(_sgl_matrix_t* dst, float left, float right, float bottom, float top, float near, float far) {
    _sgl_matrix_t m;
    m.v[0][0] = 2.0f / (right - left);
    m.v[1][0] = 0.0f;
    m.v[2][0] = 0.0f;
    m.v[3][0] = -(right + left) / (right - left);
    m.v[0][1] = 0.0f;
    m.v[1][1] = 2.0f / (top - bottom);
    m.v[2][1] = 0.0f;
    m.v[3][1] = -(top + bottom) / (top - bottom);
    m.v[0][2] = 0.0f;
    m.v[1][2] = 0.0f;
    m.v[2][2] = -2.0f / (far - near);
    m.v[3][2] = -(far + near) / (far - near);
    m.v[0][3] = 0.0f;
    m.v[1][3] = 0.0f;
    m.v[2][3] = 0.0f;
    m.v[3][3] = 1.0f;

    _sgl_matmul4(dst, dst, &m);
}

static void _sgl_perspective(_sgl_matrix_t* dst, float fovy, float aspect, float near, float far) {
    float sine = sinf(fovy / 2.0f);
    float delta_z = far - near;
    if ((delta_z == 0.0f) || (sine == 0.0f) || (aspect == 0.0f)) {
        return;
    }
    float cotan = cosf(fovy / 2.0f) / sine;
    _sgl_matrix_t m;
    _sgl_identity(&m);
    m.v[0][0] = cotan / aspect;
    m.v[1][1] = cotan;
    m.v[2][2] = -(far + near) / delta_z;
    m.v[2][3] = -1.0f;
    m.v[3][2] = -2.0f * near * far / delta_z;
    m.v[3][3] = 0.0f;
    _sgl_matmul4(dst, dst, &m);
}

/* current top-of-stack projection matrix */
static inline _sgl_matrix_t* _sgl_matrix_projection(void) {
    return &_sgl.matrix_stack[SGL_MATRIXMODE_PROJECTION][_sgl.top_of_stack[SGL_MATRIXMODE_PROJECTION]];
}

/* get top-of-stack modelview matrix */
static inline _sgl_matrix_t* _sgl_matrix_modelview(void) {
    return &_sgl.matrix_stack[SGL_MATRIXMODE_MODELVIEW][_sgl.top_of_stack[SGL_MATRIXMODE_MODELVIEW]];
}

/* get top-of-stack texture matrix */
static inline _sgl_matrix_t* _sgl_matrix_texture(void) {
    return &_sgl.matrix_stack[SGL_MATRIXMODE_TEXTURE][_sgl.top_of_stack[SGL_MATRIXMODE_TEXTURE]];
}

/* get pointer to current top-of-stack of current matrix mode */
static inline _sgl_matrix_t* _sgl_matrix(void) {
    return &_sgl.matrix_stack[_sgl.cur_matrix_mode][_sgl.top_of_stack[_sgl.cur_matrix_mode]];
}

/*== PUBLIC FUNCTIONS ========================================================*/
SOKOL_API_IMPL void sgl_setup(const sgl_desc_t* desc) {
    SOKOL_ASSERT(desc);
    memset(&_sgl, 0, sizeof(_sgl));
    _sgl.init_cookie = _SGL_INIT_COOKIE;

    /* allocate buffers */
    _sgl.num_vertices = _sgl_def(desc->max_vertices, (1<<16));
    _sgl.num_uniforms = _sgl_def(desc->max_commands, (1<<14));
    _sgl.num_commands = _sgl.num_uniforms;
    _sgl.vertices = (_sgl_vertex_t*) SOKOL_MALLOC(_sgl.num_vertices * sizeof(_sgl_vertex_t));
    SOKOL_ASSERT(_sgl.vertices);
    _sgl.uniforms = (_sgl_uniform_t*) SOKOL_MALLOC(_sgl.num_uniforms * sizeof(_sgl_uniform_t));
    SOKOL_ASSERT(_sgl.uniforms);
    _sgl.commands = (_sgl_command_t*) SOKOL_MALLOC(_sgl.num_commands * sizeof(_sgl_command_t));
    SOKOL_ASSERT(_sgl.commands);

    /* default state */
    _sgl.u_scale = _sgl.v_scale = 1.0f;
    _sgl.rgba = 0xFFFFFFFF;
    for (int i = 0; i < SGL_NUM_MATRIXMODES; i++) {
        _sgl_identity(&_sgl.matrix_stack[i][0]);
    }

    /* create sokol-gfx resource objects */
    sg_push_debug_group("sokol-gl");

    sg_buffer_desc vbuf_desc;
    memset(&vbuf_desc, 0, sizeof(vbuf_desc));
    vbuf_desc.size = _sgl.num_vertices * sizeof(_sgl_vertex_t);
    vbuf_desc.type = SG_BUFFERTYPE_VERTEXBUFFER;
    vbuf_desc.usage = SG_USAGE_STREAM;
    vbuf_desc.label = "sgl-vertex-buffer";
    _sgl.vbuf = sg_make_buffer(&vbuf_desc);
    SOKOL_ASSERT(SG_INVALID_ID != _sgl.vbuf.id);

    uint32_t pixels[64];
    for (int i = 0; i < 64; i++) {
        pixels[i] = 0xFFFFFFFF;
    }
    sg_image_desc img_desc;
    memset(&img_desc, 0, sizeof(img_desc));
    img_desc.type = SG_IMAGETYPE_2D;
    img_desc.width = 8;
    img_desc.height = 8;
    img_desc.num_mipmaps = 1;
    img_desc.pixel_format = SG_PIXELFORMAT_RGBA8;
    img_desc.min_filter = SG_FILTER_NEAREST;
    img_desc.mag_filter = SG_FILTER_NEAREST;
    img_desc.content.subimage[0][0].ptr = pixels;
    img_desc.content.subimage[0][0].size = sizeof(pixels);
    img_desc.label = "sgl-default-texture";
    _sgl.def_img = sg_make_image(&img_desc);
    SOKOL_ASSERT(SG_INVALID_ID != _sgl.def_img.id);
    _sgl.cur_img = _sgl.def_img;

    sg_shader_desc shd_desc;
    memset(&shd_desc, 0, sizeof(shd_desc));
    sg_shader_uniform_block_desc* ub = &shd_desc.vs.uniform_blocks[0];
    ub->size = sizeof(_sgl_uniform_t);
    ub->uniforms[0].name = "mvp";
    ub->uniforms[0].type = SG_UNIFORMTYPE_MAT4;
    ub->uniforms[1].name = "uv_scale";
    ub->uniforms[1].type = SG_UNIFORMTYPE_FLOAT2;
    shd_desc.fs.images[0].name = "tex";
    shd_desc.fs.images[0].type = SG_IMAGETYPE_2D;
    shd_desc.vs.source = _sgl_vs_src;
    shd_desc.fs.source = _sgl_fs_src;
    shd_desc.label = "sgl-shader";
    _sgl.shd = sg_make_shader(&shd_desc);
    sg_pop_debug_group();

    /* template desc for lazy pipeline creation */
    _sgl.pip_desc.layout.buffers[0].stride = sizeof(_sgl_vertex_t);
    {
        sg_vertex_attr_desc* pos = &_sgl.pip_desc.layout.attrs[0];
        pos->name = "position";
        pos->sem_name = "POSITION";
        pos->offset = offsetof(_sgl_vertex_t, pos);
        pos->format = SG_VERTEXFORMAT_FLOAT3;
    }
    {
        sg_vertex_attr_desc* uv = &_sgl.pip_desc.layout.attrs[1];
        uv->name = "texcoord0";
        uv->sem_name = "TEXCOORD";
        uv->offset = offsetof(_sgl_vertex_t, uv);
        uv->format = SG_VERTEXFORMAT_SHORT2N;
    }
    {
        sg_vertex_attr_desc* rgba = &_sgl.pip_desc.layout.attrs[2];
        rgba->name = "color0";
        rgba->sem_name = "TEXCOORD";
        rgba->offset = offsetof(_sgl_vertex_t, rgba);
        rgba->format = SG_VERTEXFORMAT_UBYTE4N;
    }
    _sgl.pip_desc.shader = _sgl.shd;
    _sgl.pip_desc.index_type = SG_INDEXTYPE_NONE;
    _sgl.pip_desc.depth_stencil.depth_write_enabled = true;
    _sgl.pip_desc.blend.src_factor_rgb = SG_BLENDFACTOR_SRC_ALPHA;
    _sgl.pip_desc.blend.dst_factor_rgb = SG_BLENDFACTOR_ONE_MINUS_SRC_ALPHA;
    _sgl.pip_desc.blend.color_write_mask = SG_COLORMASK_RGB;
    _sgl.pip_desc.blend.color_format = desc->color_format;
    _sgl.pip_desc.blend.depth_format = desc->depth_format;
    _sgl.pip_desc.rasterizer.sample_count = desc->sample_count;
}

SOKOL_API_IMPL void sgl_shutdown(void) {
    SOKOL_ASSERT(_sgl.init_cookie == 0xABCDABCD);
    SOKOL_FREE(_sgl.vertices); _sgl.vertices = 0;
    SOKOL_FREE(_sgl.uniforms); _sgl.uniforms = 0;
    SOKOL_FREE(_sgl.commands); _sgl.commands = 0;
    sg_destroy_buffer(_sgl.vbuf);
    sg_destroy_image(_sgl.def_img);
    sg_destroy_shader(_sgl.shd);
    /* NOTE: calling sg_destroy_*() with an invalid id is valid */
    for (int i = 0; i < _SGL_MAX_PIPELINES; i++) {
        sg_destroy_pipeline(_sgl.pip[i]);
    }
    _sgl.init_cookie = 0;
}

SOKOL_API_IMPL sgl_error_t sgl_error(void) {
    return _sgl.error;
}

SOKOL_API_IMPL void sgl_default_state(void) {
    SOKOL_ASSERT(_SGL_INIT_COOKIE == _sgl.init_cookie);
    SOKOL_ASSERT(!_sgl.in_begin);
    _sgl.state_bits = 0;
    _sgl.u_scale = _sgl.v_scale = 1.0f;
    _sgl.u = 0; _sgl.v = 0;
    _sgl.rgba = 0xFFFFFFFF;
    _sgl.cur_img = _sgl.def_img;
    for (int i = 0; i < SGL_NUM_MATRIXMODES; i++) {
        _sgl.cur_matrix_mode = (_sgl_matrix_mode_t)i;
        _sgl_identity(_sgl_matrix());
    }
    _sgl.cur_matrix_mode = 0;
}

SOKOL_API_IMPL void sgl_enable_depth_test(void) {
    SOKOL_ASSERT(_SGL_INIT_COOKIE == _sgl.init_cookie);
    SOKOL_ASSERT(!_sgl.in_begin);
    _sgl.state_bits = _sgl_enable_state(SGL_STATE_DEPTHTEST, _sgl.state_bits);
}

SOKOL_API_IMPL void sgl_enable_blend(void) {
    SOKOL_ASSERT(_SGL_INIT_COOKIE == _sgl.init_cookie);
    SOKOL_ASSERT(!_sgl.in_begin);
    _sgl.state_bits = _sgl_enable_state(SGL_STATE_BLEND, _sgl.state_bits);
}

SOKOL_API_IMPL void sgl_enable_cull_face(void) {
    SOKOL_ASSERT(_SGL_INIT_COOKIE == _sgl.init_cookie);
    SOKOL_ASSERT(!_sgl.in_begin);
    _sgl.state_bits = _sgl_enable_state(SGL_STATE_CULLFACE, _sgl.state_bits);
}

SOKOL_API_IMPL void sgl_enable_texture(void) {
    SOKOL_ASSERT(_SGL_INIT_COOKIE == _sgl.init_cookie);
    SOKOL_ASSERT(!_sgl.in_begin);
    _sgl.state_bits = _sgl_enable_state(SGL_STATE_TEXTURE, _sgl.state_bits);
}

SOKOL_API_IMPL void sgl_disable_depth_test(void) {
    SOKOL_ASSERT(_SGL_INIT_COOKIE == _sgl.init_cookie);
    SOKOL_ASSERT(!_sgl.in_begin);
    _sgl.state_bits = _sgl_disable_state(SGL_STATE_DEPTHTEST, _sgl.state_bits);
}

SOKOL_API_IMPL void sgl_disable_blend(void) {
    SOKOL_ASSERT(_SGL_INIT_COOKIE == _sgl.init_cookie);
    SOKOL_ASSERT(!_sgl.in_begin);
    _sgl.state_bits = _sgl_disable_state(SGL_STATE_BLEND, _sgl.state_bits);
}

SOKOL_API_IMPL void sgl_disable_cull_face(void) {
    SOKOL_ASSERT(_SGL_INIT_COOKIE == _sgl.init_cookie);
    SOKOL_ASSERT(!_sgl.in_begin);
    _sgl.state_bits = _sgl_disable_state(SGL_STATE_CULLFACE, _sgl.state_bits);
}

SOKOL_API_IMPL void sgl_disable_texture(void) {
    SOKOL_ASSERT(_SGL_INIT_COOKIE == _sgl.init_cookie);
    SOKOL_ASSERT(!_sgl.in_begin);
    _sgl.state_bits = _sgl_disable_state(SGL_STATE_TEXTURE, _sgl.state_bits);
}

SOKOL_API_IMPL void sgl_viewport(int x, int y, int w, int h, bool origin_top_left) {
    SOKOL_ASSERT(_SGL_INIT_COOKIE == _sgl.init_cookie);
    SOKOL_ASSERT(!_sgl.in_begin);
    _sgl_command_t* cmd = _sgl_next_command();
    if (cmd) {
        cmd->cmd = SGL_COMMAND_VIEWPORT;
        cmd->args.viewport.x = x;
        cmd->args.viewport.y = y;
        cmd->args.viewport.w = w;
        cmd->args.viewport.h = h;
        cmd->args.viewport.origin_top_left = origin_top_left;
    }
}

SOKOL_API_IMPL void sgl_scissor_rect(int x, int y, int w, int h, bool origin_top_left) {
    SOKOL_ASSERT(_SGL_INIT_COOKIE == _sgl.init_cookie);
    SOKOL_ASSERT(!_sgl.in_begin);
    _sgl_command_t* cmd = _sgl_next_command();
    if (cmd) {
        cmd->cmd = SGL_COMMAND_SCISSOR_RECT;
        cmd->args.scissor_rect.x = x;
        cmd->args.scissor_rect.y = y;
        cmd->args.scissor_rect.w = w;
        cmd->args.scissor_rect.h = h;
        cmd->args.scissor_rect.origin_top_left = origin_top_left;
    }
}

SOKOL_API_IMPL void sgl_texture(sg_image img) {
    SOKOL_ASSERT(_SGL_INIT_COOKIE == _sgl.init_cookie);
    SOKOL_ASSERT(!_sgl.in_begin);
    if (SG_INVALID_ID != img.id) {
        _sgl.cur_img = img;
    }
    else {
        _sgl.cur_img = _sgl.def_img;
    }
}

SOKOL_API_IMPL void sgl_texcoord_int_bits(int u_bits, int v_bits) {
    SOKOL_ASSERT(_SGL_INIT_COOKIE == _sgl.init_cookie);
    SOKOL_ASSERT(!_sgl.in_begin);
    SOKOL_ASSERT((u_bits >= 0) && (u_bits <= 15));
    SOKOL_ASSERT((v_bits >= 0) && (v_bits <= 15));
    _sgl.u_scale = (float)(1<<u_bits);
    _sgl.v_scale = (float)(1<<v_bits);
}

SOKOL_API_IMPL void sgl_begin_points(void) {
    SOKOL_ASSERT(_SGL_INIT_COOKIE == _sgl.init_cookie);
    SOKOL_ASSERT(!_sgl.in_begin);
    _sgl_begin(SGL_PRIMITIVETYPE_POINTS);
}

SOKOL_API_IMPL void sgl_begin_lines(void) {
    SOKOL_ASSERT(_SGL_INIT_COOKIE == _sgl.init_cookie);
    SOKOL_ASSERT(!_sgl.in_begin);
    _sgl_begin(SGL_PRIMITIVETYPE_LINES);
}

SOKOL_API_IMPL void sgl_begin_line_strip(void) {
    SOKOL_ASSERT(_SGL_INIT_COOKIE == _sgl.init_cookie);
    SOKOL_ASSERT(!_sgl.in_begin);
    _sgl_begin(SGL_PRIMITIVETYPE_LINE_STRIP);
}

SOKOL_API_IMPL void sgl_begin_triangles(void) {
    SOKOL_ASSERT(_SGL_INIT_COOKIE == _sgl.init_cookie);
    SOKOL_ASSERT(!_sgl.in_begin);
    _sgl_begin(SGL_PRIMITIVETYPE_TRIANGLES);
}

SOKOL_API_IMPL void sgl_begin_triangle_strip(void) {
    SOKOL_ASSERT(_SGL_INIT_COOKIE == _sgl.init_cookie);
    SOKOL_ASSERT(!_sgl.in_begin);
    _sgl_begin(SGL_PRIMITIVETYPE_TRIANGLE_STRIP);
}

SOKOL_API_IMPL void sgl_begin_quads(void) {
    SOKOL_ASSERT(_SGL_INIT_COOKIE == _sgl.init_cookie);
    SOKOL_ASSERT(!_sgl.in_begin);
    _sgl_begin(SGL_PRIMITIVETYPE_QUADS);
}

SOKOL_API_IMPL void sgl_end(void) {
    SOKOL_ASSERT(_SGL_INIT_COOKIE == _sgl.init_cookie);
    SOKOL_ASSERT(_sgl.in_begin);
    _sgl.in_begin = false;
    if (_sgl.base_vertex == _sgl.cur_vertex) {
        return;
    }
    _sgl_command_t* cmd = _sgl_next_command();
    if (cmd) {
        cmd->cmd = SGL_COMMAND_DRAW;
        cmd->args.draw.img = _sgl.cur_img;
        cmd->args.draw.state_bits = _sgl.state_bits;
        cmd->args.draw.base_vertex = _sgl.base_vertex;
        cmd->args.draw.num_vertices = _sgl.cur_vertex - _sgl.base_vertex;
        cmd->args.draw.uniform_index = _sgl.cur_uniform;
    }
    _sgl_uniform_t* uni = _sgl_next_uniform();
    if (uni) {
        /* FIXME: update the model-view-proj matrix lazily */
        _sgl_matmul4(&uni->mvp, _sgl_matrix_projection(), _sgl_matrix_modelview());
        uni->uv_scale[0] = _sgl.u_scale;
        uni->uv_scale[1] = _sgl.v_scale;
    }
}

SOKOL_API_IMPL float sgl_rad(float deg) {
    return (deg * (float)M_PI) / 180.0f;
}

SOKOL_API_IMPL float sgl_deg(float rad) {
    return (rad * 180.0f) / (float)M_PI;
}

SOKOL_API_IMPL void sgl_t2f(float u, float v) {
    _sgl.u = _sgl_pack_u(u);
    _sgl.v = _sgl_pack_v(v);
}

SOKOL_API_IMPL void sgl_c3f(float r, float g, float b) {
    _sgl.rgba = _sgl_pack_rgbaf(r, g, b, 1.0f);
}

SOKOL_API_IMPL void sgl_c4f(float r, float g, float b, float a) {
    _sgl.rgba = _sgl_pack_rgbaf(r, g, b, a);
}

SOKOL_API_IMPL void sgl_c3b(uint8_t r, uint8_t g, uint8_t b) {
    _sgl.rgba = _sgl_pack_rgbab(r, g, b, 255);
}

SOKOL_API_IMPL void sgl_c4b(uint8_t r, uint8_t g, uint8_t b, uint8_t a) {
    _sgl.rgba = _sgl_pack_rgbab(r, g, b, a);
}

SOKOL_API_IMPL void sgl_c1i(uint32_t rgba) {
    _sgl.rgba = rgba;
}

SOKOL_API_IMPL void sgl_v2f(float x, float y) {
    _sgl_vtx(x, y, 0.0f, _sgl.u, _sgl.v, _sgl.rgba);
}

SOKOL_API_IMPL void sgl_v3f(float x, float y, float z) {
    _sgl_vtx(x, y, z, _sgl.u, _sgl.v, _sgl.rgba);
}

SOKOL_API_IMPL void sgl_v2f_t2f(float x, float y, float u, float v) {
    _sgl_vtx(x, y, 0.0f, _sgl_pack_u(u), _sgl_pack_v(v), _sgl.rgba);
}

SOKOL_API_IMPL void sgl_v3f_t2f(float x, float y, float z, float u, float v) {
    _sgl_vtx(x, y, z, _sgl_pack_u(u), _sgl_pack_v(v), _sgl.rgba);
}

SOKOL_API_IMPL void sgl_v2f_c3f(float x, float y, float r, float g, float b) {
    _sgl_vtx(x, y, 0.0f, _sgl.u, _sgl.v, _sgl_pack_rgbaf(r, g, b, 1.0f));
}

SOKOL_API_IMPL void sgl_v2f_c3b(float x, float y, uint8_t r, uint8_t g, uint8_t b) {
    _sgl_vtx(x, y, 0.0f, _sgl.u, _sgl.v, _sgl_pack_rgbab(r, g, b, 255));
}

SOKOL_API_IMPL void sgl_v2f_c4f(float x, float y, float r, float g, float b, float a) {
    _sgl_vtx(x, y, 0.0f, _sgl.u, _sgl.v, _sgl_pack_rgbaf(r, g, b, a));
}

SOKOL_API_IMPL void sgl_v2f_c4b(float x, float y, uint8_t r, uint8_t g, uint8_t b, uint8_t a) {
    _sgl_vtx(x, y, 0.0f, _sgl.u, _sgl.v, _sgl_pack_rgbab(r, g, b, a));
}

SOKOL_API_IMPL void sgl_v2f_c1i(float x, float y, uint32_t rgba) {
    _sgl_vtx(x, y, 0.0f, _sgl.u, _sgl.v, rgba);
}

SOKOL_API_IMPL void sgl_v3f_c3f(float x, float y, float z, float r, float g, float b) {
    _sgl_vtx(x, y, z, _sgl.u, _sgl.v, _sgl_pack_rgbaf(r, g, b, 1.0f));
}

SOKOL_API_IMPL void sgl_v3f_c3b(float x, float y, float z, uint8_t r, uint8_t g, uint8_t b) {
    _sgl_vtx(x, y, z, _sgl.u, _sgl.v, _sgl_pack_rgbab(r, g, b, 255));
}

SOKOL_API_IMPL void sgl_v3f_c4f(float x, float y, float z, float r, float g, float b, float a) {
    _sgl_vtx(x, y, z, _sgl.u, _sgl.v, _sgl_pack_rgbaf(r, g, b, a));
}

SOKOL_API_IMPL void sgl_v3f_c4b(float x, float y, float z, uint8_t r, uint8_t g, uint8_t b, uint8_t a) {
    _sgl_vtx(x, y, z, _sgl.u, _sgl.v, _sgl_pack_rgbab(r, g, b, a));
}

SOKOL_API_IMPL void sgl_v3f_c1i(float x, float y, float z, uint32_t rgba) {
    _sgl_vtx(x, y, z, _sgl.u, _sgl.v, rgba);
}

SOKOL_API_IMPL void sgl_v2f_t2f_c3f(float x, float y, float u, float v, float r, float g, float b) {
    _sgl_vtx(x, y, 0.0f, _sgl_pack_u(u), _sgl_pack_v(v), _sgl_pack_rgbaf(r, g, b, 1.0f));
}

SOKOL_API_IMPL void sgl_v2f_t2f_c3b(float x, float y, float u, float v, uint8_t r, uint8_t g, uint8_t b) {
    _sgl_vtx(x, y, 0.0f, _sgl_pack_u(u), _sgl_pack_v(v), _sgl_pack_rgbab(r, g, b, 255));
}

SOKOL_API_IMPL void sgl_v2f_t2f_c4f(float x, float y, float u, float v, float r, float g, float b, float a) {
    _sgl_vtx(x, y, 0.0f, _sgl_pack_u(u), _sgl_pack_v(v), _sgl_pack_rgbaf(r, g, b, a));
}

SOKOL_API_IMPL void sgl_v2f_t2f_c4b(float x, float y, float u, float v, uint8_t r, uint8_t g, uint8_t b, uint8_t a) {
    _sgl_vtx(x, y, 0.0f, _sgl_pack_u(u), _sgl_pack_v(v), _sgl_pack_rgbab(r, g, b, a));
}

SOKOL_API_IMPL void sgl_v2f_t2f_c1i(float x, float y, float u, float v, uint32_t rgba) {
    _sgl_vtx(x, y, 0.0f, _sgl_pack_u(u), _sgl_pack_v(v), rgba);
}

SOKOL_API_IMPL void sgl_v3f_t2f_c3f(float x, float y, float z, float u, float v, float r, float g, float b) {
    _sgl_vtx(x, y, z, _sgl_pack_u(u), _sgl_pack_v(v), _sgl_pack_rgbaf(r, g, b, 1.0f));
}

SOKOL_API_IMPL void sgl_v3f_t2f_c3b(float x, float y, float z, float u, float v, uint8_t r, uint8_t g, uint8_t b) {
    _sgl_vtx(x, y, z, _sgl_pack_u(u), _sgl_pack_v(v), _sgl_pack_rgbab(r, g, b, 255));
}

SOKOL_API_IMPL void sgl_v3f_t2f_c4f(float x, float y, float z, float u, float v, float r, float g, float b, float a) {
    _sgl_vtx(x, y, z, _sgl_pack_u(u), _sgl_pack_v(v), _sgl_pack_rgbaf(r, g, b, a));
}

SOKOL_API_IMPL void sgl_v3f_t2f_c4b(float x, float y, float z, float u, float v, uint8_t r, uint8_t g, uint8_t b, uint8_t a) {
    _sgl_vtx(x, y, z, _sgl_pack_u(u), _sgl_pack_v(v), _sgl_pack_rgbab(r, g, b, a));
}

SOKOL_API_IMPL void sgl_v3f_t2f_c1i(float x, float y, float z, float u, float v, uint32_t rgba) {
    _sgl_vtx(x, y, z, _sgl_pack_u(u), _sgl_pack_v(v), rgba);
}

SOKOL_API_IMPL void sgl_matrix_mode_modelview(void) {
    SOKOL_ASSERT(_SGL_INIT_COOKIE == _sgl.init_cookie);
    _sgl.cur_matrix_mode = SGL_MATRIXMODE_MODELVIEW;
}

SOKOL_API_IMPL void sgl_matrix_mode_projection(void) {
    SOKOL_ASSERT(_SGL_INIT_COOKIE == _sgl.init_cookie);
    _sgl.cur_matrix_mode = SGL_MATRIXMODE_PROJECTION;
}

SOKOL_API_IMPL void sgl_matrix_mode_texture(void) {
    SOKOL_ASSERT(_SGL_INIT_COOKIE == _sgl.init_cookie);
    _sgl.cur_matrix_mode = SGL_MATRIXMODE_TEXTURE;
}

SOKOL_API_IMPL void sgl_load_identity(void) {
    SOKOL_ASSERT(_SGL_INIT_COOKIE == _sgl.init_cookie);
    _sgl_identity(_sgl_matrix());
}

SOKOL_API_IMPL void sgl_load_matrix(const float m[16]) {
    SOKOL_ASSERT(_SGL_INIT_COOKIE == _sgl.init_cookie);
    _sgl_transpose(_sgl_matrix(), (const _sgl_matrix_t*) &m[0]);
}

SOKOL_API_IMPL void sgl_load_transpose_matrix(const float m[16]) {
    SOKOL_ASSERT(_SGL_INIT_COOKIE == _sgl.init_cookie);
    memcpy(&_sgl_matrix()->v[0][0], &m[0], 64);
}

SOKOL_API_IMPL void sgl_mult_matrix(const float m[16]) {
    SOKOL_ASSERT(_SGL_INIT_COOKIE == _sgl.init_cookie);
    _sgl_matrix_t m0;
    _sgl_transpose(&m0, (const _sgl_matrix_t*) &m[0]);
    /* order? */
    _sgl_mul(_sgl_matrix(), &m0);
}

SOKOL_API_IMPL void sgl_mult_transpose_matrix_matrix(const float m[16]) {
    SOKOL_ASSERT(_SGL_INIT_COOKIE == _sgl.init_cookie);
    const _sgl_matrix_t* m0  = (const _sgl_matrix_t*) &m[0];
    /* order? */
    _sgl_mul(_sgl_matrix(), m0);
}

SOKOL_API_IMPL void sgl_rotate(float angle_rad, float x, float y, float z) {
    SOKOL_ASSERT(_SGL_INIT_COOKIE == _sgl.init_cookie);
    _sgl_rotate(_sgl_matrix(), angle_rad, x, y, z);
}

SOKOL_API_IMPL void sgl_scale(float x, float y, float z) {
    SOKOL_ASSERT(_SGL_INIT_COOKIE == _sgl.init_cookie);
    _sgl_scale(_sgl_matrix(), x, y, z);
}

SOKOL_API_IMPL void sgl_translate(float x, float y, float z) {
    SOKOL_ASSERT(_SGL_INIT_COOKIE == _sgl.init_cookie);
    _sgl_translate(_sgl_matrix(), x, y, z);
}

SOKOL_API_IMPL void sgl_frustum(float l, float r, float b, float t, float n, float f) {
    SOKOL_ASSERT(_SGL_INIT_COOKIE == _sgl.init_cookie);
    _sgl_frustum(_sgl_matrix(), l, r, b, t, n, f);
}

SOKOL_API_IMPL void sgl_ortho(float l, float r, float b, float t, float n, float f) {
    SOKOL_ASSERT(_SGL_INIT_COOKIE == _sgl.init_cookie);
    _sgl_ortho(_sgl_matrix(), l, r, b, t, n, f);
}

SOKOL_API_IMPL void sgl_perspective(float fov_y, float aspect, float z_near, float z_far) {
    SOKOL_ASSERT(_SGL_INIT_COOKIE == _sgl.init_cookie);
    _sgl_perspective(_sgl_matrix(), fov_y, aspect, z_near, z_far);
}

/* this draw the accumulated draw commands via sokol-gfx */
SOKOL_API_IMPL void sgl_draw(void) {
    SOKOL_ASSERT(_SGL_INIT_COOKIE == _sgl.init_cookie);
    if ((_sgl.error == SGL_NO_ERROR) && (_sgl.cur_vertex > 0) && (_sgl.cur_command > 0)) {
        sg_push_debug_group("sokol-gl");
        sg_update_buffer(_sgl.vbuf, _sgl.vertices, _sgl.cur_vertex * sizeof(_sgl_vertex_t));
        _sgl.bind.vertex_buffers[0] = _sgl.vbuf;
        for (int i = 0; i < _sgl.cur_command; i++) {
            const _sgl_command_t* cmd = &_sgl.commands[i];
            switch (cmd->cmd) {
                case SGL_COMMAND_VIEWPORT:
                    { 
                        const _sgl_viewport_args_t* args = &cmd->args.viewport;
                        sg_apply_viewport(args->x, args->y, args->w, args->h, args->origin_top_left);
                    }
                    break;
                case SGL_COMMAND_SCISSOR_RECT:
                    {
                        const _sgl_scissor_rect_args_t* args = &cmd->args.scissor_rect;
                        sg_apply_scissor_rect(args->x, args->y, args->w, args->h, args->origin_top_left);
                    }
                    break;
                case SGL_COMMAND_DRAW:
                    {
                        /* FIXME: don't apply redundant state */
                        const _sgl_draw_args_t* args = &cmd->args.draw;
                        sg_pipeline pip = _sgl_pipeline(args->state_bits);
                        sg_apply_pipeline(pip);
                        _sgl.bind.fs_images[0] = args->img;
                        sg_apply_bindings(&_sgl.bind);
                        sg_apply_uniforms(SG_SHADERSTAGE_VS, 0, &_sgl.uniforms[args->uniform_index], sizeof(_sgl_uniform_t));
                        /* FIXME: what if number of vertices doesn't match the primitive type? */
                        sg_draw(args->base_vertex, args->num_vertices, 1);
                    }
                    break;
            }
        }
        sg_pop_debug_group();
    }
    _sgl_rewind();
}
#endif /* SOKOL_GL_IMPL */
