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
    SGL_TEXTURE,
    SGL_CULL_FACE,
} sgl_state_t;

/*
    sgl_texture_t 

    This is simply an alias for sg_image.
*/
typedef sg_image sgl_texture_t;

typedef struct sgl_desc_t {
    int max_vertices;
    sg_pixel_format color_format;
    sg_pixel_format depth_format;
    int sample_count;
} sgl_desc_t;

SOKOL_API_DECL void sgl_setup(const sgl_desc_t* desc);
SOKOL_API_DECL void sgl_shutdown(void);

SOKOL_API_DECL void sgl_enable(sgl_state_t state);
SOKOL_API_DECL void sgl_disable(sgl_state_t state);
SOKOL_API_DECL void sgl_viewport(int x, int y, int w, int h, bool origin_top_left);
SOKOL_API_DECL void sgl_scissor_rect(int x, int y, int w, int h, bool origin_top_left);
SOKOL_API_DECL void sgl_bind_texture(sgl_texture_t tex);

SOKOL_API_DECL void sgl_begin(sgl_primitive_type_t mode);
SOKOL_API_DECL void sgl_vtx2f(float x, float y);
SOKOL_API_DECL void sgl_vtx3f(float x, float y, float z);
SOKOL_API_DECL void sgl_tex2f(float u, float v);
SOKOL_API_DECL void sgl_col4f(float r, float g, float b, float a);
SOKOL_API_DECL void sgl_col4u8(uint8_t r, uint8_t g, uint8_t b, uint8_t a);
SOKOL_API_DECL void sgl_col1u32(uint32_t rgba);
SOKOL_API_DECL void sgl_end(void);

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

#endif /* SOKOL_GL_IMPL */
