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
    sgl_primitive_type

    Used in sgl_begin() to start rendering this type of primitive.

    NOTE: the values are *not* identical with sg_primitive_type!
*/
typedef enum sgl_primitive_type_t {
    SGL_POINTS = 0,
    SGL_LINES,
    SGL_LINE_STRIP,
    SGL_TRIANGLES,
    SGL_TRIANGLE_STRIP,
    SGL_NUM_PRIMITIVE_TYPES,
} sgl_primitive_type_t;

/*
    sgl_matrixmode_t

    Used in sgl_matrix_mode()
*/
typedef enum sgl_matrix_mode_t {
    SGL_MODELVIEW,
    SGL_PROJECTION,
    SGL_TEXTURE,
} sgl_matrix_mode_t;

/*
    sgl_state_t

    Used in sgl_enable() / sgl_disable()
*/
typedef enum sgl_state_t {
    SGL_ORIGIN_TOP_LEFT = 0,        /* default: true */
    SGL_ALPHABLEND,                 /* default: false */
    SGL_TEXTURING,                  /* default: false */
    SGL_CULL_FACE,                  /* default: false */
    SGL_NUM_STATES,
} sgl_state_t;

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
} sgl_error_t;

/*
    sgl_texture_t 

    This is simply an alias for sg_image.
*/
typedef sg_image sgl_texture_t;

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
SOKOL_API_DECL void sgl_enable(sgl_state_t state);
SOKOL_API_DECL void sgl_disable(sgl_state_t state);
SOKOL_API_DECL bool sgl_is_enabled(sgl_state_t state);
SOKOL_API_DECL void sgl_viewport(int x, int y, int w, int h);
SOKOL_API_DECL void sgl_scissor_rect(int x, int y, int w, int h);
SOKOL_API_DECL void sgl_texture(sgl_texture_t tex);
SOKOL_API_DECL void sgl_texcoord_int_bits(int n);

/* these functions only set the internal 'current texcoord / color' (valid inside or outside begin/end) */
SOKOL_API_DECL void sgl_tex2f(float u, float v);
SOKOL_API_DECL void sgl_col4f(float r, float g, float b, float a);
SOKOL_API_DECL void sgl_col4u8(uint8_t r, uint8_t g, uint8_t b, uint8_t a);
SOKOL_API_DECL void sgl_col1u32(uint32_t rgba);

/* define primitives, each begin/end is one draw command */
SOKOL_API_DECL void sgl_begin(sgl_primitive_type_t mode);
SOKOL_API_DECL void sgl_vtx2f(float x, float y);
SOKOL_API_DECL void sgl_vtx3f(float x, float y, float z);
SOKOL_API_DECL void sgl_vtx2f_tex2f(float x, float y, float u, float v);
SOKOL_API_DECL void sgl_vtx3f_tex2f(float x, float y, float z, float u, float v);
SOKOL_API_DECL void sgl_vtx2f_col4f(float x, float y, float r, float g, float b, float a);
SOKOL_API_DECL void sgl_vtx2f_col4u8(float x, float y, uint8_t r, uint8_t g, uint8_t b, uint8_t a);
SOKOL_API_DECL void sgl_vtx2f_col1u32(float x, float y, uint32_t rgba);
SOKOL_API_DECL void sgl_vtx3f_col4f(float x, float y, float z, float r, float g, float b, float a);
SOKOL_API_DECL void sgl_vtx3f_col4u8(float x, float y, float z, uint8_t r, uint8_t g, uint8_t b, uint8_t a);
SOKOL_API_DECL void sgl_vtx3f_col1u32(float x, float y, float z, uint32_t rgba);
SOKOL_API_DECL void sgl_vtx2f_tex2f_col4f(float x, float y, float u, float v, float r, float g, float b, float a);
SOKOL_API_DECL void sgl_vtx2f_tex2f_col4u8(float x, float y, float u, float v, uint8_t r, uint8_t g, uint8_t b, uint8_t a);
SOKOL_API_DECL void sgl_vtx2f_tex2f_col1u32(float x, float y, float u, float v, uint32_t rgba);
SOKOL_API_DECL void sgl_vtx3f_tex2f_col4f(float x, float y, float z, float u, float v, float r, float g, float b, float a);
SOKOL_API_DECL void sgl_vtx3f_tex2f_col4u8(float x, float y, float z, float u, float v, uint8_t r, uint8_t g, uint8_t b, uint8_t a);
SOKOL_API_DECL void sgl_vtx3f_tex2f_col1u32(float x, float y, float z, float u, float v, uint32_t rgba);
SOKOL_API_DECL void sgl_end(void);

/* matrix stack functions (only valid outside begin end */
SOKOL_API_DECL void sgl_matrix_mode(sgl_matrix_mode_t mode);
SOKOL_API_DECL void sgl_load_matrix(float m[16]);
SOKOL_API_DECL void sgl_mult_matrix(float m[16]);
SOKOL_API_DECL void sgl_load_transpose_matrix(float m[16]);
SOKOL_API_DECL void sgl_mult_transpose_matrix(float m[16]);
SOKOL_API_DECL void sgl_load_identity(void);
SOKOL_API_DECL void sgl_rotate(float angle, float x, float y, float z);
SOKOL_API_DECL void sgl_scale(float x, float y, float z);
SOKOL_API_DECL void sgl_translate(float x, float y, float z);
SOKOL_API_DECL void sgl_frustum(float l, float r, float b, float t, float n, float f);
SOKOL_API_DECL void sgl_ortho(float l, float r, float b, float t, float n, float f);
SOKOL_API_DECL void sgl_push_matrix(void);
SOKOL_API_DECL void sgl_pop_matrix(void);

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
#ifndef _SOKOL_PRIVATE
    #if defined(__GNUC__)
        #define _SOKOL_PRIVATE __attribute__((unused)) static
    #else
        #define _SOKOL_PRIVATE static
    #endif
#endif

#define _sgl_def(val, def) (((val) == 0) ? (def) : (val))
#define _SGL_INIT_COOKIE (0xABCDABCD)

#if defined(SOKOL_GLCORE33)
static const char* _sgl_vs_src =
    "#version 330\n"
    "uniform mat4 mvp;\n"
    "uniform vec2 uv_scale;\n"
    "in vec2 position;\n"
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
    "attribute vec2 position;\n"
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
    "  float2 pos [[attribute(0)]];\n"
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
    "  out.pos = params.mvp * in.position;\n"
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

typedef struct {
    float pos[3];
    int16_t uv[2];  /* texcoords as packed fixed-point format, see sgl_texcoord_int_bits */
    uint32_t rgba;
} _sgl_vertex_t;

typedef struct {
    float mvp[16];      /* model-view-projection matrix */
    float uv_scale[2];  /* scaler for converting fixed-point texcoord to float */
} _sgl_uniform_t;

typedef enum {
    SGL_COMMAND_DRAW,
    SGL_COMMAND_VIEWPORT,
    SGL_COMMAND_SCISSOR_RECT,
} _sgl_command_type_t;

typedef struct {
    sgl_primitive_type_t type;
    sgl_texture_t texture;
    int base_vertex_index;
    int num_vertices;
    int uniforms_index;
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

    sgl_error_t error;
    bool in_begin;
    bool state[SGL_NUM_STATES];
    float u_scale, v_scale;
    int16_t u, v;
    uint32_t rgba;
    sgl_texture_t tex;

    // FIXME

    sg_buffer vbuf;
    sg_image img;   /* a default white texture */
    sg_shader shd;
    sg_pipeline pip[2][2][SGL_NUM_PRIMITIVE_TYPES];    /* blend/cull/primitive-type */
} _sgl_t;
static _sgl_t _sgl;

/*== PRIVATE FUNCTIONS =======================================================*/
_SOKOL_PRIVATE void _sgl_rewind(void) {
    _sgl.cur_vertex = 0;
    _sgl.cur_uniform = 0;
    _sgl.cur_command = 0;
    _sgl.error = SGL_NO_ERROR;
}

_SOKOL_PRIVATE _sgl_vertex_t* _sgl_next_vertex(void) {
    if (_sgl.cur_vertex < _sgl.num_vertices) {
        return &_sgl.vertices[_sgl.cur_vertex++];
    }
    else {
        _sgl.error = SGL_ERROR_VERTICES_FULL;
        return 0;
    }
}

_SOKOL_PRIVATE _sgl_uniform_t* _sgl_next_uniform(void) {
    if (_sgl.cur_uniform < _sgl.num_uniforms) {
        return &_sgl.uniforms[_sgl.cur_uniform++];
    }
    else {
        _sgl.error = SGL_ERROR_UNIFORMS_FULL;
        return 0;
    }
}

_SOKOL_PRIVATE _sgl_command_t* _sgl_next_command(void) {
    if (_sgl.cur_command < _sgl.num_commands) {
        return &_sgl.commands[_sgl.cur_command++];
    }
    else {
        _sgl.error = SGL_ERROR_COMMANDS_FULL;
        return 0;
    }
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
    _sgl.state[SGL_ORIGIN_TOP_LEFT] = true;
    _sgl.u_scale = _sgl.v_scale = 1.0f;
    _sgl.rgba = 0xFFFFFFFF;

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
    _sgl.img = sg_make_image(&img_desc);
    SOKOL_ASSERT(SG_INVALID_ID != _sgl.img.id);

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

    sg_pipeline_desc pip_desc;
    memset(&pip_desc, 0, sizeof(pip_desc));
    pip_desc.layout.buffers[0].stride = sizeof(_sgl_vertex_t);
    {
        sg_vertex_attr_desc* pos = &pip_desc.layout.attrs[0];
        pos->name = "position";
        pos->sem_name = "POSITION";
        pos->offset = offsetof(_sgl_vertex_t, pos);
        pos->format = SG_VERTEXFORMAT_FLOAT3;
    }
    {
        sg_vertex_attr_desc* uv = &pip_desc.layout.attrs[1];
        uv->name = "texcoord0";
        uv->sem_name = "TEXCOORD";
        uv->offset = offsetof(_sgl_vertex_t, uv);
        uv->format = SG_VERTEXFORMAT_SHORT2N;
    }
    {
        sg_vertex_attr_desc* rgba = &pip_desc.layout.attrs[2];
        rgba->name = "color0";
        rgba->sem_name = "TEXCOORD";
        rgba->offset = offsetof(_sgl_vertex_t, rgba);
        rgba->format = SG_VERTEXFORMAT_UBYTE4N;
    }
    pip_desc.shader = _sgl.shd;
    pip_desc.index_type = SG_INDEXTYPE_NONE;
    pip_desc.blend.src_factor_rgb = SG_BLENDFACTOR_SRC_ALPHA;
    pip_desc.blend.dst_factor_rgb = SG_BLENDFACTOR_ONE_MINUS_SRC_ALPHA;
    pip_desc.blend.color_write_mask = SG_COLORMASK_RGB;
    pip_desc.blend.color_format = desc->color_format;
    pip_desc.blend.depth_format = desc->depth_format;
    pip_desc.rasterizer.sample_count = desc->sample_count;
    for (int blend = 0; blend < 2; blend++) {
        pip_desc.blend.enabled = (blend == 0);
        for (int cull = 0; cull < 2; cull++) {
            pip_desc.rasterizer.cull_mode = (cull == 0) ? SG_CULLMODE_NONE : SG_CULLMODE_BACK;
            for (int prim_type = 0; prim_type < SGL_NUM_PRIMITIVE_TYPES; prim_type++) {
                switch (prim_type) {
                    case SGL_POINTS: pip_desc.primitive_type = SG_PRIMITIVETYPE_POINTS; break;
                    case SGL_LINES: pip_desc.primitive_type = SG_PRIMITIVETYPE_LINES; break;
                    case SGL_LINE_STRIP: pip_desc.primitive_type = SG_PRIMITIVETYPE_LINE_STRIP; break;
                    case SGL_TRIANGLES: pip_desc.primitive_type = SG_PRIMITIVETYPE_TRIANGLES; break;
                    case SGL_TRIANGLE_STRIP: pip_desc.primitive_type = SG_PRIMITIVETYPE_TRIANGLE_STRIP; break;
                    default: SOKOL_UNREACHABLE; break;
                }
                _sgl.pip[blend][cull][prim_type] = sg_make_pipeline(&pip_desc);
            }
        }
    }
    sg_pop_debug_group();
}

SOKOL_API_IMPL void sgl_shutdown(void) {
    SOKOL_ASSERT(_sgl.init_cookie == 0xABCDABCD);
    SOKOL_FREE(_sgl.vertices); _sgl.vertices = 0;
    SOKOL_FREE(_sgl.uniforms); _sgl.uniforms = 0;
    SOKOL_FREE(_sgl.commands = _sgl.commands = 0);
    sg_destroy_buffer(_sgl.vbuf);
    sg_destroy_image(_sgl.img);
    sg_destroy_shader(_sgl.shd);
    for (int blend = 0; blend < 2; blend++) {
        for (int cull = 0; cull < 2; cull++) {
            for (int prim_type = 0; prim_type < SGL_NUM_PRIMITIVE_TYPES; prim_type++) {
                sg_destroy_pipeline(_sgl.pip[blend][cull][prim_type]);
            }
        }
    }
    _sgl.init_cookie = 0;
}

SOKOL_API_IMPL sgl_error_t sgl_error(void) {
    return _sgl.error;
}

SOKOL_API_IMPL void sgl_enable(sgl_state_t state) {
    SOKOL_ASSERT(_SGL_INIT_COOKIE == _sgl.init_cookie);
    SOKOL_ASSERT((state >= 0) && (state < SGL_NUM_STATES));
    SOKOL_ASSERT(!_sgl.in_begin);
    _sgl.state[state] = true;
}

SOKOL_API_IMPL void sgl_disable(sgl_state_t state) {
    SOKOL_ASSERT(_SGL_INIT_COOKIE == _sgl.init_cookie);
    SOKOL_ASSERT((state >= 0) && (state < SGL_NUM_STATES));
    SOKOL_ASSERT(!_sgl.in_begin);
    _sgl.state[state] = false;
}

SOKOL_API_IMPL bool sgl_is_enabled(sgl_state_t state) {
    SOKOL_ASSERT((state >= 0) && (state < SGL_NUM_STATES));
    SOKOL_ASSERT(_SGL_INIT_COOKIE == _sgl.init_cookie);
    return _sgl.state[state];
}

SOKOL_API_IMPL void sgl_viewport(int x, int y, int w, int h) {
    SOKOL_ASSERT(_SGL_INIT_COOKIE == _sgl.init_cookie);
    SOKOL_ASSERT(!_sgl.in_begin);
    _sgl_command_t* cmd = _sgl_next_command();
    if (cmd) {
        cmd->cmd = SGL_COMMAND_VIEWPORT;
        cmd->args.viewport.x = x;
        cmd->args.viewport.y = y;
        cmd->args.viewport.w = w;
        cmd->args.viewport.h = h;
        cmd->args.viewport.origin_top_left = _sgl.state[SGL_ORIGIN_TOP_LEFT];
    }
}

SOKOL_API_IMPL void sgl_scissor_rect(int x, int y, int w, int h) {
    SOKOL_ASSERT(_SGL_INIT_COOKIE == _sgl.init_cookie);
    SOKOL_ASSERT(!_sgl.in_begin);
    _sgl_command_t* cmd = _sgl_next_command();
    if (cmd) {
        cmd->cmd = SGL_COMMAND_SCISSOR_RECT;
        cmd->args.scissor_rect.x = x;
        cmd->args.scissor_rect.y = y;
        cmd->args.scissor_rect.w = w;
        cmd->args.scissor_rect.h = h;
        cmd->args.scissor_rect.origin_top_left = _sgl.state[SGL_ORIGIN_TOP_LEFT];
    }
}

SOKOL_API_IMPL void sgl_texture(sgl_texture_t tex) {
    SOKOL_ASSERT(_SGL_INIT_COOKIE == _sgl.init_cookie);
    SOKOL_ASSERT(!_sgl.in_begin);
    if (SG_INVALID_ID != tex.id) {
        _sgl.tex = tex;
    }
    else {
        _sgl.tex = _sgl.img;
    }
}

SOKOL_API_IMPL void sgl_texcoord_int_bits(int n) {
    SOKOL_ASSERT(_SGL_INIT_COOKIE == _sgl.init_cookie);
    SOKOL_ASSERT(!_sgl.in_begin);
    SOKOL_ASSERT((n >= 0) && (n <= 15));
    // FIXME: separate int-bits for u and v?
    _sgl.u_scale = _sgl.v_scale = (float)(1<<n);
}


#endif /* SOKOL_GL_IMPL */
