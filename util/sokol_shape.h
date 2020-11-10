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
typedef struct sshape_vec4_t { float x, y, z, w; } sshape_vec4_t;
typedef struct sshape_mat4_t { float v[4][4]; } sshape_mat4_t;
typedef uint16_t sshape_index_t;
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
    bool valid;
    sshape_build_item_t vertices;
    sshape_build_item_t indices;
} sshape_build_t;

/* TODO: documentation on build-parameter structs */
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
#define _sshape_def(val, def) (((val) == 0) ? (def) : (val))
#define _sshape_def_flt(val, def) (((val) == 0.0f) ? (def) : (val))
#define _sshape_white (0xFFFFFFFF)

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

static inline sshape_vec4_t _sshape_vec4(float x, float y, float z, float w) {
    return (sshape_vec4_t) { x, y, z, w };
}

static inline sshape_vec2_t _sshape_vec2(float x, float y) {
    return (sshape_vec2_t) { x, y };
}

static bool _sshape_mat4_isnull(const sshape_mat4_t* m) {
    for (int y = 0; y < 4; y++) {
        for (int x = 0; x < 4; x++) {
            if (0.0f != m->v[y][x]) {
                return false;
            }
        }
    }
    return true;
}

static sshape_mat4_t _sshape_mat4_identity(void) {
    return (sshape_mat4_t) {
        {
            { 1.0f, 0.0f, 0.0f, 0.0f },
            { 0.0f, 1.0f, 0.0f, 0.0f },
            { 0.0f, 0.0f, 1.0f, 0.0f },
            { 0.0f, 0.0f, 0.0f, 1.0f }
        }
    };
}

static sshape_vec4_t _sshape_mat4_mul(const sshape_mat4_t* m, sshape_vec4_t v) {
    return (sshape_vec4_t) {
        m->v[0][0]*v.x + m->v[1][0]*v.y + m->v[2][0]*v.z + m->v[3][0]*v.w,
        m->v[0][1]*v.x + m->v[1][1]*v.y + m->v[2][1]*v.z + m->v[3][1]*v.w,
        m->v[0][2]*v.x + m->v[1][2]*v.y + m->v[2][2]*v.z + m->v[3][2]*v.w,
        m->v[0][3]*v.x + m->v[1][3]*v.y + m->v[2][3]*v.z + m->v[3][3]*v.w
    };
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

static bool _sshape_validate_build_item(const sshape_build_item_t* item, uint32_t build_size) {
    if (0 == item->buf_ptr) {
        return false;
    }
    if (0 == item->buf_size) {
        return false;
    }
    if ((item->data_offset + item->data_size + build_size) > item->buf_size) {
        return false;
    }
    return true;
}

static bool _sshape_validate_build_state(const sshape_build_t* state, uint32_t num_vertices, uint32_t num_indices) {
    if (!_sshape_validate_build_item(&state->vertices, num_vertices * sizeof(sshape_vertex_t))) {
        return false;
    }
    if (!_sshape_validate_build_item(&state->indices, num_indices * sizeof(sshape_index_t))) {
        return false;
    }
    return true;
}

static void _sshape_advance_offset(sshape_build_item_t* item) {
    item->data_offset += item->data_size;
    item->data_size = 0;
}

static sshape_index_t _sshape_base_index(const sshape_build_t* state) {
    return state->indices.data_offset / sizeof(sshape_index_t);
}

static sshape_plane_t _sshape_plane_defaults(const sshape_plane_t* params) {
    sshape_plane_t res = *params;
    res.width = _sshape_def_flt(res.width, 1.0f);
    res.depth = _sshape_def_flt(res.depth, 1.0f);
    res.tiles = _sshape_def(res.tiles, 1);
    res.color = _sshape_def(res.color, _sshape_white);
    res.transform = _sshape_mat4_isnull(&res.transform) ? _sshape_mat4_identity() : res.transform;
    return res;
}

static sshape_box_t _sshape_box_defaults(const sshape_box_t* params) {
    sshape_box_t res = *params;
    res.width = _sshape_def_flt(res.width, 1.0f);
    res.height = _sshape_def_flt(res.height, 1.0f);
    res.depth = _sshape_def_flt(res.depth, 1.0f);
    res.tiles = _sshape_def(res.tiles, 1);
    res.color = _sshape_def(res.color, _sshape_white);
    res.transform = _sshape_mat4_isnull(&res.transform) ? _sshape_mat4_identity() : res.transform;
    return res;
}

static void _sshape_add_vertex(sshape_build_t* state, sshape_vec4_t pos, sshape_vec4_t norm, sshape_vec2_t uv, uint32_t color) {
    uint32_t offset = state->vertices.data_offset + state->vertices.data_size;
    SOKOL_ASSERT((offset + sizeof(sshape_vertex_t)) <= state->vertices.buf_size);
    state->vertices.data_size += sizeof(sshape_vertex_t);
    sshape_vertex_t* v_ptr = (sshape_vertex_t*) ((uint8_t*)state->vertices.buf_ptr + offset);
    v_ptr->position.x = pos.x;
    v_ptr->position.y = pos.y;
    v_ptr->position.z = pos.z;
    v_ptr->normal.x = norm.x;
    v_ptr->normal.y = norm.y;
    v_ptr->normal.z = norm.z;
    v_ptr->texcoord.x = uv.x;
    v_ptr->texcoord.y = uv.y;
    v_ptr->color = color;
}

static void _sshape_add_triangle(sshape_build_t* state, sshape_index_t i0, sshape_index_t i1, sshape_index_t i2) {
    uint32_t offset = state->indices.data_offset + state->indices.data_size;
    SOKOL_ASSERT((offset + 3*sizeof(sshape_index_t)) <= state->indices.buf_size);
    state->indices.data_size += 3*sizeof(sshape_index_t);
    sshape_index_t* i_ptr = (sshape_index_t*) ((uint8_t*)state->indices.buf_ptr + offset);
    i_ptr[0] = i0;
    i_ptr[1] = i1;
    i_ptr[2] = i2;
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
    res.indices.size = res.indices.num * sizeof(sshape_index_t);
    return res;
}

SOKOL_API_IMPL sshape_sizes_t sshape_box_buffer_sizes(uint32_t tiles) {
    SOKOL_ASSERT(tiles >= 1);
    sshape_sizes_t res = { 0 };
    res.vertices.num = _sshape_box_num_vertices(tiles);
    res.indices.num = _sshape_box_num_indices(tiles);
    res.vertices.size = res.vertices.num * sizeof(sshape_vertex_t);
    res.indices.size = res.indices.num * sizeof(sshape_index_t);
    return res;
}

SOKOL_API_IMPL sshape_sizes_t sshape_sphere_buffer_sizes(uint32_t slices, uint32_t stacks) {
    SOKOL_ASSERT((slices >= 3) && (stacks >= 2));
    sshape_sizes_t res = { 0 };
    res.vertices.num = _sshape_sphere_num_vertices(slices, stacks);
    res.indices.num = _sshape_sphere_num_indices(slices, stacks);
    res.vertices.size = res.vertices.num * sizeof(sshape_vertex_t);
    res.indices.size = res.indices.num * sizeof(sshape_index_t);
    return res;
}

SOKOL_API_IMPL sshape_sizes_t sshape_cylinder_buffer_sizes(uint32_t slices, uint32_t stacks) {
    SOKOL_ASSERT((slices >= 3) && (stacks >= 1));
    sshape_sizes_t res = { 0 };
    res.vertices.num = _sshape_cylinder_num_vertices(slices, stacks);
    res.indices.num = _sshape_cylinder_num_indices(slices, stacks);
    res.vertices.size = res.vertices.num * sizeof(sshape_vertex_t);
    res.indices.size = res.indices.num * sizeof(sshape_index_t);
    return res;
}

SOKOL_API_IMPL sshape_sizes_t sshape_torus_buffer_sizes(uint32_t sides, uint32_t rings) {
    SOKOL_ASSERT((sides >= 3) && (rings >= 3));
    sshape_sizes_t res = { 0 };
    res.vertices.num = _sshape_torus_num_vertices(sides, rings);
    res.indices.num = _sshape_torus_num_indices(sides, rings);
    res.vertices.size = res.vertices.num * sizeof(sshape_vertex_t);
    res.indices.size = res.indices.num * sizeof(sshape_index_t);
    return res;
}

/*
    Geometry layout for plane (4 tiles):
    +--+--+--+--+
    |\ |\ |\ |\ |
    | \| \| \| \|
    +--+--+--+--+    25 vertices (tiles + 1) * (tiles + 1)
    |\ |\ |\ |\ |    32 triangles (tiles + 1) * (tiles + 1) * 2
    | \| \| \| \|
    +--+--+--+--+
    |\ |\ |\ |\ |
    | \| \| \| \|
    +--+--+--+--+
    |\ |\ |\ |\ |
    | \| \| \| \|
    +--+--+--+--+
*/
SOKOL_API_IMPL sshape_build_t sshape_build_plane(const sshape_build_t* in_state, const sshape_plane_t* in_params) {
    SOKOL_ASSERT(in_state && in_params);
    const sshape_plane_t params = _sshape_plane_defaults(in_params);
    const uint32_t num_vertices = _sshape_plane_num_vertices(params.tiles);
    const uint32_t num_indices = _sshape_plane_num_indices(params.tiles);
    sshape_build_t state = *in_state;
    if (!_sshape_validate_build_state(&state, num_vertices, num_indices)) {
        state.valid = false;
        return state;
    }
    state.valid = true;
    _sshape_advance_offset(&state.vertices);
    _sshape_advance_offset(&state.indices);

    // write vertices
    const float x0 = -params.width * 0.5f;
    const float z0 =  params.depth * 0.5f;
    const float dx =  params.width / params.tiles;
    const float dz = -params.depth / params.tiles;
    const float duv = 1.0f / params.tiles;
    sshape_vec4_t tnorm = _sshape_mat4_mul(&params.transform, _sshape_vec4(0.0f, 1.0f, 0.0f, 0.0f));
    for (uint32_t ix = 0; ix <= params.tiles; ix++) {
        for (uint32_t iz = 0; iz <= params.tiles; iz++) {
            sshape_vec4_t pos = _sshape_vec4(x0 + dx*ix, z0 + dz*iz, 0.0f, 1.0f);
            sshape_vec4_t tpos = _sshape_mat4_mul(&params.transform, pos);
            sshape_vec2_t uv = _sshape_vec2(duv*ix, duv*iz);
            _sshape_add_vertex(&state, tpos, tnorm, uv, params.color);
        }
    }

    // write indices
    sshape_index_t start_index = _sshape_base_index(&state);
    for (uint32_t j = 0; j < params.tiles; j++) {
        for (uint32_t i = 0; i < params.tiles; i++) {
            sshape_index_t i0 = start_index + (j * (params.tiles + 1)) + i;
            sshape_index_t i1 = i0 + 1;
            sshape_index_t i2 = i0 + params.tiles + 1;
            sshape_index_t i3 = i2 + 1;
            _sshape_add_triangle(&state, i0, i1, i2);
            _sshape_add_triangle(&state, i0, i3, i2);
        }
    }
    return state;
}

SOKOL_API_IMPL sshape_build_t sshape_build_box(const sshape_build_t* in_state, const sshape_box_t* in_params) {
    SOKOL_ASSERT(in_state && in_params);
    const sshape_box_t params = _sshape_box_defaults(in_params);
    const uint32_t num_vertices = _sshape_box_num_vertices(params.tiles);
    const uint32_t num_indices = _sshape_box_num_indices(params.tiles);
    sshape_build_t state = *in_state;
    if (!_sshape_validate_build_state(&state, num_vertices, num_indices)) {
        state.valid = false;
        return state;
    }
    state.valid = true;
    _sshape_advance_offset(&state.vertices);
    _sshape_advance_offset(&state.indices);

    // build vertices
    const float x0 = -params.width * 0.5f;
    const float x1 =  params.width * 0.5f;
    const float y0 = -params.height * 0.5f;
    const float y1 =  params.height * 0.5f;
    const float z0 = -params.depth * 0.5f;
    const float z1 =  params.depth * 0.5f;
    const float dx = params.width / params.tiles;
    const float dy = params.height / params.tiles;
    const float dz = params.depth / params.tiles;
    const float duv = 1.0f / params.tiles;

    // bottom/top vertices
    for (uint32_t top_bottom = 0; top_bottom < 2; top_bottom++) {
        sshape_vec4_t pos = _sshape_vec4(0.0f, (0==top_bottom) ? y0:y1, 0.0f, 0.0f);
        sshape_vec4_t norm = _sshape_vec4(0.0f, (0==top_bottom) ? -1.0f:1.0f, 0.0f, 0.0f);
        sshape_vec4_t tnorm = _sshape_mat4_mul(&params.transform, norm);
        for (uint32_t ix = 0; ix <= params.tiles; ix++) {
            pos.x = (0==top_bottom) ? (x0 + dx * ix) : (x1 - dx * ix);
            for (uint32_t iz = 0; iz <= params.tiles; iz++) {
                pos.z = z0 + dz * iz;
                sshape_vec4_t tpos = _sshape_mat4_mul(&params.transform, pos);
                sshape_vec2_t uv = _sshape_vec2(ix * duv, iz * duv);
                _sshape_add_vertex(&state, tpos, tnorm, uv, params.color);
            }
        }
    }

    // left/right vertices
    for (uint32_t left_right = 0; left_right < 2; left_right++) {
        sshape_vec4_t pos = _sshape_vec4((0==left_right) ? x0:x1, 0.0f, 0.0f, 0.0f);
        sshape_vec4_t norm = _sshape_vec4((0==left_right) ? -1.0f:1.0f, 0.0f, 0.0f, 0.0f);
        sshape_vec4_t tnorm = _sshape_mat4_mul(&params.transform, norm);
        for (uint32_t iy = 0; iy <= params.tiles; iy++) {
            pos.y = (0==left_right) ? (y1 - dy * iy) : (y1 + dy * iy);
            for (uint32_t iz = 0; iz <= params.tiles; iz++) {
                pos.z = z0 + dz * iz;
                sshape_vec4_t tpos = _sshape_mat4_mul(&params.transform, pos);
                sshape_vec2_t uv = _sshape_vec2(iy * duv, iz * duv);
                _sshape_add_vertex(&state, tpos, tnorm, uv, params.color);
            }
        }
    }

    // front/back vertices
    for (uint32_t front_back = 0; front_back < 2; front_back++) {
        sshape_vec4_t pos = _sshape_vec4(0.0f, 0.0f, (0==front_back) ? z0:z1, 0.0f);
        sshape_vec4_t norm = _sshape_vec4(0.0f, 0.0f, (0==front_back) ? -1.0f:1.0f, 0.0f);
        sshape_vec4_t tnorm = _sshape_mat4_mul(&params.transform, norm);
        for (uint32_t ix = 0; ix <= params.tiles; ix++) {
            pos.x = (0==front_back) ? (x1 - dx * ix) : (x0 + dx * ix);
            for (uint32_t iy = 0; iy <= params.tiles; iy++) {
                pos.y = y0 + dy * iy;
                sshape_vec4_t tpos = _sshape_mat4_mul(&params.transform, pos);
                sshape_vec2_t uv = _sshape_vec2(ix * duv, iy * duv);
                _sshape_add_vertex(&state, tpos, tnorm, uv, params.color);
            }
        }
    }

    // build indices
    const sshape_index_t verts_per_face = (params.tiles + 1) * (params.tiles + 1);
    sshape_index_t start_index = _sshape_base_index(&state);
    for (uint32_t face = 0; face < 6; face++) {
        sshape_index_t face_start_index = start_index + face * verts_per_face;
        for (uint32_t j = 0; j < params.tiles; j++) {
            for (uint32_t i = 0; i < params.tiles; i++) {
                sshape_index_t i0 = face_start_index + (j * (params.tiles + 1)) + i;
                sshape_index_t i1 = i0 + 1;
                sshape_index_t i2 = i0 + params.tiles + 1;
                sshape_index_t i3 = i2 + 1;
                _sshape_add_triangle(&state, i0, i1, i3);
                _sshape_add_triangle(&state, i0, i3, i2);
            }
        }
    }
    return state;
}


#endif // SOKOL_SHAPE_IMPL
