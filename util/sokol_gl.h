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

    The values are identical with sokol-gfx's sg_primitive_type.
*/
typedef enum sgl_primitive_type_t {
    SGL_POINTS = 1,
    SGL_LINES,
    SGL_LINE_STRIP,
    SGL_TRIANGLES,
    SGL_TRIANGLE_STRIP,
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
    SGL_TEXTURING,                  /* default: false */
    SGL_CULL_FACE,                  /* default: false */
    SGL_NUM_STATES,
} sgl_state_t;

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
#ifndef _SOKOL_PRIVATE
    #if defined(__GNUC__)
        #define _SOKOL_PRIVATE __attribute__((unused)) static
    #else
        #define _SOKOL_PRIVATE static
    #endif
#endif

typedef struct {
    float x, y, z;
    int16_t u, v;       /* texcoords as packed fixed-point format, see sgl_texcoord_int_bits */
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
} _sgl_viewport_args_t;

typedef struct {
    int x, y, w, h;
} _sgl_scissor_rect_args_t;

typedef union {
    _sgl_draw_args_t draw;
    _sgl_viewport_args_t viewport;
    _sgl_scissor_rect_args_t scissor_rect;
} _sgl_args_t;

typedef struct {
    _sgl_command_t cmd;
    _sgl_args_t args;
} _sgl_command_t;

typedef struct {
    /* per-frame draw data */
    int num_vertices;
    int num_uniforms;
    int num_commands;
    int cur_vertex;
    int cur_uniform;
    int cur_command;
    _sgl_vertex_t* vertices;
    _sgl_uniform_t* uniforms;
    _sgl_command_t* commands;

    /* current render state */
    bool sgl_state_t state[SGL_NUM_STATES];
    float u_scale, v_scale;
    int16_t u, v;
    uint32_t rgba;
    sgl_texture tex;

    /* matrix stacks */
    // FIXME
} _sgl_state_t;

static _sgl_state_t _sgl;

#endif /* SOKOL_GL_IMPL */
