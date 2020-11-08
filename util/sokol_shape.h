#ifndef SOKOL_SHAPE_INCLUDED
/*
    sokol_shape.h -- create simple primitive shapes for sokol_gfx.h

    Project URL: https://github.com/floooh/sokol

    Do this:
        #define SOKOL_SHAPE_IMPL
    before you include this file in *one* C or C++ file to create the
    implementation.

    ...optionally provide the following macros to override defaults:

    SOKOL_ASSERT(c)     - your own assert macro (default: assert(c))
    SOKOL_API_DECL      - public function declaration prefix (default: extern)
    SOKOL_API_IMPL      - public function implementation prefix (default: -)
    SOKOL_LOG(msg)      - your own logging function (default: puts(msg))
    SOKOL_UNREACHABLE() - a guard macro for unreachable code (default: assert(false))

    If sokol_shape.h is compiled as a DLL, define the following before
    including the declaration or implementation:

    SOKOL_DLL

    On Windows, SOKOL_DLL will define SOKOL_API_DECL as __declspec(dllexport)
    or __declspec(dllimport) as needed.

    Include the following headers before including sokol_shape.h:

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
#define SOKOL_SHAPE_INCLUDED
#include <stdint.h>
#include <stdbool.h>

#if !defined(SOKOL_GFX_INCLUDED)
#error "Please include sokol_gfx.h before sokol_shape.h"
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

/* helper structs */
typedef struct sshape_vec2_t { float x, y; } sshape_vec2_t;
typedef struct sshape_vec3_t { float x, y, z; } sshape_vec3_t;
typedef struct sshape_mat4_t { float v[4][4]; } sshape_mat4_t;

typedef struct sshape_vertex_t {
    sshape_vec3_t position;
    sshape_vec3_t normal;
    sshape_vec2_t texcoord;
    uint32_t color;
} sshape_vertex_t;

/* a range of draw-elements (sg_draw(int base_element, int num_element, ...)) */
typedef struct sshape_element_range_t {
    int base_element;
    int num_elements;
} sshape_element_range_t;

/* number of elements and byte size of build actions */
typedef struct sshape_sizes_item_t {
    uint32_t num;       // number of elements
    uint32_t size;      // the same as size in bytes
} sshape_sizes_item_t;

typedef struct sshape_sizes_t {
    sshape_sizes_item_t vertices;
    sshape_sizes_item_t indices;
} sshape_sizes_t;

/* in/out struct to keep track of mesh-build state */
typedef struct sshape_build_item_t {
    void* buf_ptr;          // pointer to start of output buffer
    uint32_t buf_size;      // size in bytes of output buffer
    uint32_t data_offset;   // current data offset from buffer start
    uint32_t data_size;     // current length of data in bytes
} sshape_build_item_t;

typedef struct sshape_build_t {
    sshape_build_item_t vertices;
    sshape_build_item_t indices;
} sshape_build_t;

/* shape build-parameter structs */
typedef struct sshape_plane_t {
    float width, depth;             // default: 1.0
    uint32_t tiles;                 // default: 1
    uint32_t color;                 // default: white
    sshape_mat4_t transform;        // default: identity matrix
} sshape_plane_t;

typedef struct sshape_box_t {
    float width, height, depth;     // default: 1.0
    uint32_t tiles;                 // default: 1
    uint32_t color;                 // default: white
    sshape_mat4_t transform;        // default: identity matrix
} sshape_box_t;

typedef struct sshape_sphere_t {
    float radius;                   // default: 0.5
    uint32_t slices;                // default: FIXME
    uint32_t stacks;                // default: FIXME
    uint32_t color;                 // default: white
    sshape_mat4_t transform;        // default: identity matrix
} sshape_sphere_t;

typedef struct sshape_cylinder_t {
    float radius;                   // default: 0.5
    float length;                   // default: 1.0
    uint32_t slices;                // default: FIXME
    uint32_t stacks;                // default: FIXME
    uint32_t color;                 // default: white
    sshape_mat4_t transform;        // default: identity matrix
} sshape_cylinder_t;

typedef struct sshape_torus_t {
    float ring_radius;              // default: ???
    float radius;                   // default: ???
    uint32_t sides;                 // default: ???
    uint32_t rings;                 // default: ???
    sshape_mat4_t transform;        // default: identity matrix
} sshape_torus_t;

/* shape builder functions */
SOKOL_API_DECL sshape_build_t sshape_build_plane(const sshape_build_t* state, const sshape_plane_t* params);
SOKOL_API_DECL sshape_build_t sshape_build_box(const sshape_build_t* state, const sshape_box_t* params);
SOKOL_API_DECL sshape_build_t sshape_build_sphere(const sshape_build_t* state, const sshape_sphere_t* params);
SOKOL_API_DECL sshape_build_t sshape_build_cylinder(const sshape_build_t* state, const sshape_cylinder_t* params);
SOKOL_API_DECL sshape_build_t sshape_build_torus(const sshape_build_t* state, const sshape_torus_t* params);

/* query required vertex- and index-buffer sizes in bytes */
SOKOL_API_DECL sshape_sizes_t sshape_plane_sizes(uint32_t tiles);
SOKOL_API_DECL sshape_sizes_t sshape_box_sizes(uint32_t tiles);
SOKOL_API_DECL sshape_sizes_t sshape_sphere_sizes(uint32_t slices, uint32_t stacks);
SOKOL_API_DECL sshape_sizes_t sshape_cylinder_sizes(uint32_t slices, uint32_t stacks);
SOKOL_API_DECL sshape_sizes_t sshape_torus_sizes(uint32_t sides, uint32_t rings);

/* extract sokol-gfx desc structs and primitive ranges from build state */
SOKOL_API_DECL sg_buffer_desc sshape_vertex_buffer_desc(const sshape_build_t* state);
SOKOL_API_DECL sg_buffer_desc sshape_index_buffer_desc(const sshape_build_t* state);
SOKOL_API_DECL sshape_element_range_t sshape_element_range(const sshape_build_t* state);
SOKOL_API_DECL sg_buffer_layout_desc sshape_buffer_layout_desc(void);
SOKOL_API_DECL sg_vertex_attr_desc sshape_position_attr_desc(void);
SOKOL_API_DECL sg_vertex_attr_desc sshape_normal_attr_desc(void);
SOKOL_API_DECL sg_vertex_attr_desc sshape_texcoord_attr_desc(void);
SOKOL_API_DECL sg_vertex_attr_desc sshape_color_attr_desc(void);

/* helper functions to build packed color value from floats or bytes */
SOKOL_API_DECL uint32_t sshape_color_4f(float r, float g, float b, float a);
SOKOL_API_DECL uint32_t sshape_color_3f(float r, float g, float b);
SOKOL_API_DECL uint32_t sshape_color_4b(uint8_t r, uint8_t g, uint8_t b, uint8_t a);
SOKOL_API_DECL uint32_t sshape_color_3b(uint8_t r, uint8_t g, uint8_t b);

/* adapter function for filling matrix struct from generic float[16] array */
SOKOL_API_DECL sshape_mat4_t sshape_mat4(const float m[16]);
SOKOL_API_DECL sshape_mat4_t sshape_mat4_transpose(const float m[16]);

#ifdef __cplusplus
} // extern "C"

// FIXME: C++ helper functions

#endif
#endif // SOKOL_SHAPE_INCLUDED

/*-- IMPLEMENTATION ----------------------------------------------------------*/
#ifdef SOKOL_SHAPE_IMPL
#define SOKOL_SHAPE_IMPL_INCLUDED (1)

#include <string.h> // memcpy

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
    uint8_t r_u8 = (uint8_t) (_sshape_clamp(r, 0.0f, 1.0f) * 255.0f);
    uint8_t g_u8 = (uint8_t) (_sshape_clamp(g, 0.0f, 1.0f) * 255.0f);
    uint8_t b_u8 = (uint8_t) (_sshape_clamp(b, 0.0f, 1.0f) * 255.0f);
    uint8_t a_u8 = (uint8_t) (_sshape_clamp(a, 0.0f, 1.0f) * 255.0f);
    return _sshape_pack_rgbab(r_u8, g_u8, b_u8, a_u8);
}

static uint32_t _sshape_plane_num_vertices(uint32_t tiles) {
    return (tiles + 1) * (tiles + 1);
}

static uint32_t _sshape_plane_num_indices(uint32_t tiles) {
    return tiles * tiles * 2 * 3;
}

static uint32_t _sshape_box_num_vertices(uint32_t tiles) {
    return (tiles + 1) * (tiles + 1) * 6;
}

static uint32_t _sshape_box_num_indices(uint32_t tiles) {
    return tiles * tiles * 2 * 6 * 3;
}

static uint32_t _sshape_sphere_num_vertices(uint32_t slices, uint32_t stacks) {
    return (slices + 1) * (stacks + 1);
}

static uint32_t _sshape_sphere_num_indices(uint32_t slices, uint32_t stacks) {
    return ((2 * slices * stacks) - (2 * slices)) * 3;
}

static uint32_t _sshape_cylinder_num_vertices(uint32_t slices, uint32_t stacks) {
    return (slices + 1) * (stacks + 5);
}

static uint32_t _sshape_cylinder_num_indices(uint32_t slices, uint32_t stacks) {
    return ((2 * slices * stacks) + (2 * slices)) * 3;
}

static uint32_t _sshape_torus_num_vertices(uint32_t sides, uint32_t rings) {
    return (sides + 1) * (rings + 1);
}

static uint32_t _sshape_torus_num_indices(uint32_t sides, uint32_t rings) {
    return sides * rings * 2 * 3;
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

SOKOL_API_IMPL sshape_mat4_t sshape_mat4(const float m[16]) {
    sshape_mat4_t res;
    memcpy(&res.v[0][0], &m[0], 64);
    return res;
}

SOKOL_API_IMPL sshape_mat4_t sshape_mat4_transpose(const float m[16]) {
    sshape_mat4_t res;
    for (int c = 0; c < 4; c++) {
        for (int r = 0; r < 4; r++) {
            res.v[r][c] = m[c*4 + r];
        }
    }
    return res;
}

SOKOL_API_IMPL sshape_sizes_t sshape_plane_buffer_sizes(uint32_t tiles) {
    SOKOL_ASSERT(tiles >= 1);
    sshape_sizes_t res = { 0 };
    res.vertices.num = _sshape_plane_num_vertices(tiles);
    res.indices.num = _sshape_plane_num_indices(tiles);
    res.vertices.size = res.vertices.num * sizeof(sshape_vertex_t);
    res.indices.size = res.indices.num * sizeof(uint16_t);
    return res;
}

SOKOL_API_IMPL sshape_sizes_t sshape_box_buffer_sizes(uint32_t tiles) {
    SOKOL_ASSERT(tiles >= 1);
    sshape_sizes_t res = { 0 };
    res.vertices.num = _sshape_box_num_vertices(tiles);
    res.indices.num = _sshape_box_num_indices(tiles);
    res.vertices.size = res.vertices.num * sizeof(sshape_vertex_t);
    res.indices.size = res.indices.num * sizeof(uint16_t);
    return res;
}

SOKOL_API_IMPL sshape_sizes_t sshape_sphere_buffer_sizes(uint32_t slices, uint32_t stacks) {
    SOKOL_ASSERT((slices >= 3) && (stacks >= 2));
    sshape_sizes_t res = { 0 };
    res.vertices.num = _sshape_sphere_num_vertices(slices, stacks);
    res.indices.num = _sshape_sphere_num_indices(slices, stacks);
    res.vertices.size = res.vertices.num * sizeof(sshape_vertex_t);
    res.indices.size = res.indices.num * sizeof(uint16_t);
    return res;
}

SOKOL_API_IMPL sshape_sizes_t sshape_cylinder_buffer_sizes(uint32_t slices, uint32_t stacks) {
    SOKOL_ASSERT((slices >= 3) && (stacks >= 1));
    sshape_sizes_t res = { 0 };
    res.vertices.num = _sshape_cylinder_num_vertices(slices, stacks);
    res.indices.num = _sshape_cylinder_num_indices(slices, stacks);
    res.vertices.size = res.vertices.num * sizeof(sshape_vertex_t);
    res.indices.size = res.indices.num * sizeof(uint16_t);
    return res;
}

SOKOL_API_IMPL sshape_sizes_t sshape_torus_buffer_sizes(uint32_t sides, uint32_t rings) {
    SOKOL_ASSERT((sides >= 3) && (rings >= 3));
    sshape_sizes_t res = { 0 };
    res.vertices.num = _sshape_torus_num_vertices(sides, rings);
    res.indices.num = _sshape_torus_num_indices(sides, rings);
    res.vertices.size = res.vertices.num * sizeof(sshape_vertex_t);
    res.indices.size = res.indices.num * sizeof(uint16_t);
    return res;
}

#endif // SOKOL_SHAPE_IMPL
