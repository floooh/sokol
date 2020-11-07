#ifndef SOKOL_SHAPES_INCLUDED
/*
    sokol_shapes.h -- create simple primitive shapes for sokol_gfx.h

    Project URL: https://github.com/floooh/sokol

    Do this:
        #define SOKOL_SHAPES_IMPL
    before you include this file in *one* C or C++ file to create the
    implementation.

    ...optionally provide the following macros to override defaults:

    SOKOL_ASSERT(c)     - your own assert macro (default: assert(c))
    SOKOL_API_DECL      - public function declaration prefix (default: extern)
    SOKOL_API_IMPL      - public function implementation prefix (default: -)
    SOKOL_LOG(msg)      - your own logging function (default: puts(msg))
    SOKOL_UNREACHABLE() - a guard macro for unreachable code (default: assert(false))

    If sokol_shapes.h is compiled as a DLL, define the following before
    including the declaration or implementation:

    SOKOL_DLL

    On Windows, SOKOL_DLL will define SOKOL_API_DECL as __declspec(dllexport)
    or __declspec(dllimport) as needed.

    Include the following headers before including sokol_shapes.h:

        sokol_gfx.h

    [TOOD: DOCS]

    LICENSE
    =======
    zlib/libpng license

    Copyright (c) 2020 Andre Weissflog

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
#define SOKOL_SHAPES_INCLUDED
#include <stdint.h>
#include <stdbool.h>

#if !defined(SOKOL_GFX_INCLUDED)
#error "Please include sokol_gfx.h before sokol_shapes.h"
#endif

#ifndef SOKOL_API_DECL
#if defined(_WIN32) && defined(SOKOL_DLL) && defined(SOKOL_IMPL)
#define SOKOL_API_DECL __declspec(dllexport)
#elif defined(_WIN32) && defined(SOKOL_DLL)
#define SOKOL_API_DECL __declspec(dllimport)
#else
#define SOKOL_API_DECL extern
#endif
#endif

#ifdef __cplusplus
extern "C" {
#endif

typedef struct sshape_vec2_t { float x, y; } sshape_vec2_t;
typedef struct sshape_vec3_t { float x, y, z; } sshape_vec3_t;
typedef struct sshape_vec4_t { float x, y, z, w; } sshape_vec4_t;
typedef struct sshape_mat4_t { float m[4][4]; } sshape_mat4_t;

typedef struct sshape_vertex_t {
    sshape_vec3_t pos;
    sshape_vec3_t normal;
    sshape_vec2_t uv;
    uint32_t color;
} sshape_vertex_t;

/* a pointer / byte-size pair to describe a memory buffer */
typedef struct sshape_buffer_t {
    void* ptr;
    uint32_t size;
} sshape_buffer_t;

/* result struct of shape builder functions */
typedef struct sshape_mesh_t {
    bool success;
    // ready-to-use sokol-gfx desc structs
    struct {
        sg_buffer_desc vbuf;
        sg_buffer_desc ibuf;
        sg_vertex_attr_desc position;
        sg_vertex_attr_desc normal;
        sg_vertex_attr_desc uv;
        sg_vertex_attr_desc color;
    } desc;
    // ...or as vertex- and index-pointer/num pairs (same as the input buffer pointers)
    struct {
        sshape_vertex_t* ptr;
        uint32_t num;
    } vertices;
    struct {
        uint16_t* ptr;
        uint32_t num;
    } indices;
} sshape_mesh_t;

/* result of the ssahape_*_buffer_size() functions */
typedef struct sshape_buffer_sizes_t {
    uint32_t vertex_buffer_size;    // required vertex buffer size in bytes
    uint32_t index_buffer_size;     // required index buffer size
    uint32_t num_vertices;          // ...the same in number of vertices
    uint32_t num_indices;           // ...the same in number of indices
} sshape_buffer_sizes_t;

/* shape build-parameter structs */
typedef struct sshape_plane_desc_t {
    float width, depth;             // default: 1.0
    uint32_t tiles;                 // default: 1
    uint32_t color;                 // default: white
    sshape_mat4_t transform;        // default: identity matrix
    sshape_buffer_t vertices;       // memory buffer to write vertices to
    sshape_buffer_t indices;        // memory buffer to write indices to
} sshape_plane_t;

typedef struct sshape_box_desc_t {
    float width, height, depth;     // default: 1.0
    uint32_t tiles;                 // default: 1
    uint32_t color;                 // default: white
    sshape_mat4_t transform;        // default: identity matrix
    sshape_buffer_t vertices;       // memory buffer to write vertices to
    sshape_buffer_t indices;        // memory buffer to write indices to
} sshape_box_desc_t;

typedef struct sshape_sphere_desc_t {
    float radius;                   // default: 0.5
    uint32_t slices;                // default: FIXME
    uint32_t stacks;                // default: FIXME
    uint32_t color;                 // default: white
    sshape_mat4_t transform;        // default: identity matrix
    sshape_buffer_t vertices;       // memory buffer to write vertices to
    sshape_buffer_t indices;        // memory buffer to write indices to
} sshape_sphere_desc_t;

typedef struct sshape_cylinder_desc_t {
    float radius;                   // default: 0.5
    float length;                   // default: 1.0
    uint32_t slices;                // default: FIXME
    uint32_t stacks;                // default: FIXME
    uint32_t color;                 // default: white
    sshape_mat4_t transform;        // default: identity matrix
    sshape_buffer_t vertices;       // memory buffer to write vertices to
    sshape_buffer_t indices;        // memory buffer to write indices to
} sshape_cylinder_desc_t;

typedef struct sshape_torus_desc_t {
    float ring_radius;              // default: ???
    float radius;                   // default: ???
    uint32_t sides;                 // default: ???
    uint32_t rings;                 // default: ???
    sshape_mat4_t transform;        // default: identity matrix
    sshape_buffer_t vertices;       // memory buffer to write vertices to
    sshape_buffer_t indices;        // memory buffer to write indices to
} sshape_torus_desc_t;

/* shape builder functions */
SOKOL_API_DECL sshape_mesh_t sshape_build_plane(const sshape_plane_desc_t* desc);
SOKOL_API_DECL sshape_mesh_t sshape_build_box(const sshape_box_desc_t* desc);
SOKOL_API_DECL sshape_mesh_t sshape_build_sphere(const sshape_sphere_desc_t* desc);
SOKOL_API_DECL sshape_mesh_t sshape_build_cylinder(const sshape_cylinder_desc_t* desc);
SOKOL_API_DECL sshape_mesh_t sshape_build_torus(const sshape_torus_desc_t* desc);

/* query required vertex- and index-buffer sizes in bytes */
SOKOL_API_DECL sshape_buffer_sizes_t sshape_query_plane_buffer_sizes(const sshape_plane_desc_t* desc);
SOKOL_API_DECL sshape_buffer_sizes_t sshape_query_box_buffer_sizes(const sshape_box_desc_t* desc);
SOKOL_API_DECL sshape_buffer_sizes_t sshape_query_sphere_buffer_sizes(const sshape_sphere_desc_t* desc);
SOKOL_API_DECL sshape_buffer_sizes_t sshape_query_cylinder_buffer_sizes(const sshape_cylinder_desc_t* desc);
SOKOL_API_DECL sshape_buffer_sizes_t sshape_query_torus_buffer_sizes(const sshape_torus_desc_t* desc);

/* helper functions to build packed color value from floats or bytes */
SOKOL_API_DECL uint32_t sshape_color_4f(float r, float g, float b, float a);
SOKOL_API_DECL uint32_t sshape_color_3f(float r, float g, float b);
SOKOL_API_DECL uint32_t sshape_color_4b(uint8_t r, uint8_t g, uint8_t b, uint8_t a);
SOKOL_API_DECL uint32_t sshape_color_3b(uint8_t r, uint8_t g, uint8_t b);

/* adapter function for filling matrix from generic float[16] array */
SOKOL_API_DECL sshape_mat4_t sshape_mat4(const float[16] m);
SOKOL_API_DECL sshape_mat4_t sshape_mat4_transpose(const float[16] m);

#ifdef __cplusplus
} // extern "C"

// FIXME: C++ helper functions

#endif
#endif // SOKOL_SHAPES_INCLUDED

/*-- IMPLEMENTATION ----------------------------------------------------------*/
#ifdef SOKOL_SHAPES_IMPL
#define SOKOL_SHAPES_IMPL_INCLUDED (1)

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

#ifndef SOKOL_LOG
    #ifdef SOKOL_DEBUG
        #include <stdio.h>
        #define SOKOL_LOG(s) { SOKOL_ASSERT(s); puts(s); }
    #else
        #define SOKOL_LOG(s)
    #endif
#endif
#ifndef SOKOL_UNREACHABLE
    #define SOKOL_UNREACHABLE SOKOL_ASSERT(false)
#endif
#ifndef _SOKOL_UNUSED
    #define _SOKOL_UNUSED(x) (void)(x)
#endif

/*=== PRIVATE FUNCTIONS ======================================================*/
static inline uint32_t _sshape_pack_rgbab(uint8_t r, uint8_t g, uint8_t b, uint8_t a) {
    return (uint32_t)(((uint32_t)a<<24)|((uint32_t)b<<16)|((uint32_t)g<<8)|r);
}

static inline float _sshape_clamp(float v, float lo, float hi) {
    if (v < lo) return lo;
    else if (v > hi) return hi;
    else return v;
}

static inline uint32_t _sshape_pack_rgbaf(float r, float g, float b, float a) {
    uint8_t r_u8 = (uint8_t) (_sgl_clamp(r, 0.0f, 1.0f) * 255.0f);
    uint8_t g_u8 = (uint8_t) (_sgl_clamp(g, 0.0f, 1.0f) * 255.0f);
    uint8_t b_u8 = (uint8_t) (_sgl_clamp(b, 0.0f, 1.0f) * 255.0f);
    uint8_t a_u8 = (uint8_t) (_sgl_clamp(a, 0.0f, 1.0f) * 255.0f);
    return _sshape_pack_rgbab(r_u8, g_u8, b_u8, a_u8);
}

/*=== PUBLIC API FUNCTIONS ===================================================*/
SOKOL_API_IMPL uint32_t sshape_color_4f(float r, float g, float b, float a) {
    return _sshape_pack_rgbaf(r, g, b, a);
}

SOKOL_API_IMPL uint32_t sshape_color_3f(float r, float g, float b) {
    return _sshape_pack_rgbaf(r, g, b, 1.0f);
}

SOKOL_API_IMPL uint32_t sshape_color_4b(uint8_t r, uint8_t g, uint8_t b, uint8_t a) {
    return _sshape_pack_rgbab(r, g, b, a);
}

SOKOL_API_IMPL uint32_t sshape_color_3b(uint8_t r, uint8_t g, uint8_t b) {
    return _sshape_pack_rgbab(r, g, b, 255);
}

SOKOL_API_IMPL sshape_mat4_t sshape_mat4(const float[16] m) {
    sshape_mat4_t res;
    memcpy(&res.m[0][0], &m[0], 64);
    return res;
}

SOKOL_API_IMPL sshape_mat4_t sshape_mat4_transpose(const float[16] m) {
    sshape_mat4_t res;
    for (int c = 0; c < 4; c++) {
        for (int r = 0; r < 4; r++) {
            res.m[r][c] = m[c*4 + r];
        }
    }
    return res;
}


#endif // SOKOL_SHAPES_IMPL
