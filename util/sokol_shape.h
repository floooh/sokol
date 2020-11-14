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
typedef struct sshape_buffer_item_t {
    void* buffer_ptr;          // pointer to start of output buffer
    uint32_t buffer_size;      // size in bytes of output buffer
    uint32_t data_size;     // size in bytes of valid data in buffer
    uint32_t shape_offset;  // data offset of the most recent shape
} sshape_buffer_item_t;

typedef struct sshape_buffer_t {
    bool valid;
    sshape_buffer_item_t vertices;
    sshape_buffer_item_t indices;
} sshape_buffer_t;

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
    uint32_t slices;                // default: 5
    uint32_t stacks;                // default: 4
    uint32_t color;                 // default: white
    sshape_mat4_t transform;        // default: identity matrix
} sshape_sphere_t;

typedef struct sshape_cylinder_t {
    float radius;                   // default: 0.5
    float height;                   // default: 1.0
    uint32_t slices;                // default: 5
    uint32_t stacks;                // default: 1
    uint32_t color;                 // default: white
    sshape_mat4_t transform;        // default: identity matrix
} sshape_cylinder_t;

typedef struct sshape_torus_t {
    float radius;                   // default: 0.5f
    float ring_radius;              // default: 0.2f
    uint32_t sides;                 // default: 5
    uint32_t rings;                 // default: 5
    uint32_t color;                 // default: white
    sshape_mat4_t transform;        // default: identity matrix
} sshape_torus_t;

/* shape builder functions */
SOKOL_API_DECL sshape_buffer_t sshape_build_plane(const sshape_buffer_t* buf, const sshape_plane_t* params);
SOKOL_API_DECL sshape_buffer_t sshape_build_box(const sshape_buffer_t* buf, const sshape_box_t* params);
SOKOL_API_DECL sshape_buffer_t sshape_build_sphere(const sshape_buffer_t* buf, const sshape_sphere_t* params);
SOKOL_API_DECL sshape_buffer_t sshape_build_cylinder(const sshape_buffer_t* buf, const sshape_cylinder_t* params);
SOKOL_API_DECL sshape_buffer_t sshape_build_torus(const sshape_buffer_t* buf, const sshape_torus_t* params);

/* query required vertex- and index-buffer sizes in bytes */
SOKOL_API_DECL sshape_sizes_t sshape_plane_sizes(uint32_t tiles);
SOKOL_API_DECL sshape_sizes_t sshape_box_sizes(uint32_t tiles);
SOKOL_API_DECL sshape_sizes_t sshape_sphere_sizes(uint32_t slices, uint32_t stacks);
SOKOL_API_DECL sshape_sizes_t sshape_cylinder_sizes(uint32_t slices, uint32_t stacks);
SOKOL_API_DECL sshape_sizes_t sshape_torus_sizes(uint32_t sides, uint32_t rings);

/* extract sokol-gfx desc structs and primitive ranges from build state */
SOKOL_API_DECL sg_buffer_desc sshape_vertex_buffer_desc(const sshape_buffer_t* buf);
SOKOL_API_DECL sg_buffer_desc sshape_index_buffer_desc(const sshape_buffer_t* buf);
SOKOL_API_DECL sshape_element_range_t sshape_element_range(const sshape_buffer_t* buf);
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
#include <stddef.h> // offsetof
#include <math.h>   // sinf, cosf

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

static inline sshape_vec4_t _sshape_vec4_norm(sshape_vec4_t v) {
    float l = sqrtf(v.x*v.x + v.y*v.y + v.z*v.z + v.w*v.w);
    if (l != 0.0f) {
        return (sshape_vec4_t) { v.x/l, v.y/l, v.z/l, v.w/l };
    }
    else {
        return (sshape_vec4_t) { 0.0f, 1.0f, 0.0f, 0.0f };
    }
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

static bool _sshape_validate_buffer_item(const sshape_buffer_item_t* item, uint32_t build_size) {
    if (0 == item->buffer_ptr) {
        return false;
    }
    if (0 == item->buffer_size) {
        return false;
    }
    if ((item->data_size + build_size) > item->buffer_size) {
        return false;
    }
    if (item->shape_offset > item->data_size) {
        return false;
    }
    return true;
}

static bool _sshape_validate_buffer(const sshape_buffer_t* buf, uint32_t num_vertices, uint32_t num_indices) {
    if (!_sshape_validate_buffer_item(&buf->vertices, num_vertices * sizeof(sshape_vertex_t))) {
        return false;
    }
    if (!_sshape_validate_buffer_item(&buf->indices, num_indices * sizeof(sshape_index_t))) {
        return false;
    }
    return true;
}

static void _sshape_advance_offset(sshape_buffer_item_t* item) {
    item->shape_offset = item->data_size;
}

static sshape_index_t _sshape_base_index(const sshape_buffer_t* buf) {
    return buf->vertices.shape_offset / sizeof(sshape_vertex_t);
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

static sshape_sphere_t _sshape_sphere_defaults(const sshape_sphere_t* params) {
    sshape_sphere_t res = *params;
    res.radius = _sshape_def_flt(res.radius, 0.5f);
    res.slices = _sshape_def(res.slices, 5);
    res.stacks = _sshape_def(res.stacks, 4);
    res.color = _sshape_def(res.color, _sshape_white);
    res.transform = _sshape_mat4_isnull(&res.transform) ? _sshape_mat4_identity() : res.transform;
    return res;
}

static sshape_cylinder_t _sshape_cylinder_defaults(const sshape_cylinder_t* params) {
    sshape_cylinder_t res = *params;
    res.radius = _sshape_def_flt(res.radius, 0.5f);
    res.height = _sshape_def_flt(res.height, 1.0f);
    res.slices = _sshape_def(res.slices, 5);
    res.stacks = _sshape_def(res.stacks, 1);
    res.color = _sshape_def(res.color, _sshape_white);
    res.transform = _sshape_mat4_isnull(&res.transform) ? _sshape_mat4_identity() : res.transform;
    return res;
}

static sshape_torus_t _sshape_torus_defaults(const sshape_torus_t* params) {
    sshape_torus_t res = *params;
    res.radius = _sshape_def_flt(res.radius, 0.5f);
    res.ring_radius = _sshape_def_flt(res.ring_radius, 0.2f);
    res.sides = _sshape_def_flt(res.sides, 5);
    res.rings = _sshape_def_flt(res.rings, 5);
    res.color = _sshape_def(res.color, _sshape_white);
    res.transform = _sshape_mat4_isnull(&res.transform) ? _sshape_mat4_identity() : res.transform;
    return res;
}

static void _sshape_add_vertex(sshape_buffer_t* buf, sshape_vec4_t pos, sshape_vec4_t norm, sshape_vec2_t uv, uint32_t color) {
    uint32_t offset = buf->vertices.data_size;
    SOKOL_ASSERT((offset + sizeof(sshape_vertex_t)) <= buf->vertices.buffer_size);
    buf->vertices.data_size += sizeof(sshape_vertex_t);
    sshape_vertex_t* v_ptr = (sshape_vertex_t*) ((uint8_t*)buf->vertices.buffer_ptr + offset);
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

static void _sshape_add_triangle(sshape_buffer_t* buf, sshape_index_t i0, sshape_index_t i1, sshape_index_t i2) {
    uint32_t offset = buf->indices.data_size;
    SOKOL_ASSERT((offset + 3*sizeof(sshape_index_t)) <= buf->indices.buffer_size);
    buf->indices.data_size += 3*sizeof(sshape_index_t);
    sshape_index_t* i_ptr = (sshape_index_t*) ((uint8_t*)buf->indices.buffer_ptr + offset);
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
SOKOL_API_IMPL sshape_buffer_t sshape_build_plane(const sshape_buffer_t* in_buf, const sshape_plane_t* in_params) {
    SOKOL_ASSERT(in_buf && in_params);
    const sshape_plane_t params = _sshape_plane_defaults(in_params);
    const uint32_t num_vertices = _sshape_plane_num_vertices(params.tiles);
    const uint32_t num_indices = _sshape_plane_num_indices(params.tiles);
    sshape_buffer_t buf = *in_buf;
    if (!_sshape_validate_buffer(&buf, num_vertices, num_indices)) {
        buf.valid = false;
        return buf;
    }
    buf.valid = true;
    _sshape_advance_offset(&buf.vertices);
    _sshape_advance_offset(&buf.indices);

    // write vertices
    const float x0 = -params.width * 0.5f;
    const float z0 =  params.depth * 0.5f;
    const float dx =  params.width / params.tiles;
    const float dz = -params.depth / params.tiles;
    const float duv = 1.0f / params.tiles;
    sshape_vec4_t tnorm = _sshape_mat4_mul(&params.transform, _sshape_vec4(0.0f, 1.0f, 0.0f, 0.0f));
    for (uint32_t ix = 0; ix <= params.tiles; ix++) {
        for (uint32_t iz = 0; iz <= params.tiles; iz++) {
            const sshape_vec4_t pos = _sshape_vec4(x0 + dx*ix, 0.0f, z0 + dz*iz, 1.0f);
            const sshape_vec4_t tpos = _sshape_mat4_mul(&params.transform, pos);
            const sshape_vec2_t uv = _sshape_vec2(duv*ix, duv*iz);
            _sshape_add_vertex(&buf, tpos, tnorm, uv, params.color);
        }
    }

    // write indices
    sshape_index_t start_index = _sshape_base_index(&buf);
    for (uint32_t j = 0; j < params.tiles; j++) {
        for (uint32_t i = 0; i < params.tiles; i++) {
            const sshape_index_t i0 = start_index + (j * (params.tiles + 1)) + i;
            const sshape_index_t i1 = i0 + 1;
            const sshape_index_t i2 = i0 + params.tiles + 1;
            const sshape_index_t i3 = i2 + 1;
            _sshape_add_triangle(&buf, i0, i1, i3);
            _sshape_add_triangle(&buf, i0, i3, i2);
        }
    }
    return buf;
}

SOKOL_API_IMPL sshape_buffer_t sshape_build_box(const sshape_buffer_t* in_buf, const sshape_box_t* in_params) {
    SOKOL_ASSERT(in_buf && in_params);
    const sshape_box_t params = _sshape_box_defaults(in_params);
    const uint32_t num_vertices = _sshape_box_num_vertices(params.tiles);
    const uint32_t num_indices = _sshape_box_num_indices(params.tiles);
    sshape_buffer_t buf = *in_buf;
    if (!_sshape_validate_buffer(&buf, num_vertices, num_indices)) {
        buf.valid = false;
        return buf;
    }
    buf.valid = true;
    _sshape_advance_offset(&buf.vertices);
    _sshape_advance_offset(&buf.indices);

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
        const sshape_vec4_t norm = _sshape_vec4(0.0f, (0==top_bottom) ? -1.0f:1.0f, 0.0f, 0.0f);
        const sshape_vec4_t tnorm = _sshape_mat4_mul(&params.transform, norm);
        for (uint32_t ix = 0; ix <= params.tiles; ix++) {
            pos.x = (0==top_bottom) ? (x0 + dx * ix) : (x1 - dx * ix);
            for (uint32_t iz = 0; iz <= params.tiles; iz++) {
                pos.z = z0 + dz * iz;
                const sshape_vec4_t tpos = _sshape_mat4_mul(&params.transform, pos);
                const sshape_vec2_t uv = _sshape_vec2(ix * duv, iz * duv);
                _sshape_add_vertex(&buf, tpos, tnorm, uv, params.color);
            }
        }
    }

    // left/right vertices
    for (uint32_t left_right = 0; left_right < 2; left_right++) {
        sshape_vec4_t pos = _sshape_vec4((0==left_right) ? x0:x1, 0.0f, 0.0f, 0.0f);
        const sshape_vec4_t norm = _sshape_vec4((0==left_right) ? -1.0f:1.0f, 0.0f, 0.0f, 0.0f);
        const sshape_vec4_t tnorm = _sshape_mat4_mul(&params.transform, norm);
        for (uint32_t iy = 0; iy <= params.tiles; iy++) {
            pos.y = (0==left_right) ? (y1 - dy * iy) : (y0 + dy * iy);
            for (uint32_t iz = 0; iz <= params.tiles; iz++) {
                pos.z = z0 + dz * iz;
                const sshape_vec4_t tpos = _sshape_mat4_mul(&params.transform, pos);
                const sshape_vec2_t uv = _sshape_vec2(iy * duv, iz * duv);
                _sshape_add_vertex(&buf, tpos, tnorm, uv, params.color);
            }
        }
    }

    // front/back vertices
    for (uint32_t front_back = 0; front_back < 2; front_back++) {
        sshape_vec4_t pos = _sshape_vec4(0.0f, 0.0f, (0==front_back) ? z0:z1, 0.0f);
        const sshape_vec4_t norm = _sshape_vec4(0.0f, 0.0f, (0==front_back) ? -1.0f:1.0f, 0.0f);
        const sshape_vec4_t tnorm = _sshape_mat4_mul(&params.transform, norm);
        for (uint32_t ix = 0; ix <= params.tiles; ix++) {
            pos.x = (0==front_back) ? (x1 - dx * ix) : (x0 + dx * ix);
            for (uint32_t iy = 0; iy <= params.tiles; iy++) {
                pos.y = y0 + dy * iy;
                const sshape_vec4_t tpos = _sshape_mat4_mul(&params.transform, pos);
                const sshape_vec2_t uv = _sshape_vec2(ix * duv, iy * duv);
                _sshape_add_vertex(&buf, tpos, tnorm, uv, params.color);
            }
        }
    }

    // build indices
    const sshape_index_t verts_per_face = (params.tiles + 1) * (params.tiles + 1);
    const sshape_index_t start_index = _sshape_base_index(&buf);
    for (uint32_t face = 0; face < 6; face++) {
        sshape_index_t face_start_index = start_index + face * verts_per_face;
        for (uint32_t j = 0; j < params.tiles; j++) {
            for (uint32_t i = 0; i < params.tiles; i++) {
                const sshape_index_t i0 = face_start_index + (j * (params.tiles + 1)) + i;
                const sshape_index_t i1 = i0 + 1;
                const sshape_index_t i2 = i0 + params.tiles + 1;
                const sshape_index_t i3 = i2 + 1;
                _sshape_add_triangle(&buf, i0, i1, i3);
                _sshape_add_triangle(&buf, i0, i3, i2);
            }
        }
    }
    return buf;
}

/*
    Geometry layout for spheres is as follows (for 5 slices, 4 stacks):

    +  +  +  +  +  +        north pole
    |\ |\ |\ |\ |\
    | \| \| \| \| \
    +--+--+--+--+--+        30 vertices (slices + 1) * (stacks + 1)
    |\ |\ |\ |\ |\ |        30 triangles (2 * slices * stacks) - (2 * slices)
    | \| \| \| \| \|        2 orphaned vertices
    +--+--+--+--+--+
    |\ |\ |\ |\ |\ |
    | \| \| \| \| \|
    +--+--+--+--+--+
     \ |\ |\ |\ |\ |
      \| \| \| \| \|
    +  +  +  +  +  +        south pole
*/
SOKOL_API_IMPL sshape_buffer_t sshape_build_sphere(const sshape_buffer_t* in_buf, const sshape_sphere_t* in_params) {
    SOKOL_ASSERT(in_buf && in_params);
    const sshape_sphere_t params = _sshape_sphere_defaults(in_params);
    const uint32_t num_vertices = _sshape_sphere_num_vertices(params.slices, params.stacks);
    const uint32_t num_indices = _sshape_sphere_num_indices(params.slices, params.stacks);
    sshape_buffer_t buf = *in_buf;
    if (!_sshape_validate_buffer(&buf, num_vertices, num_indices)) {
        buf.valid = false;
        return buf;
    }
    buf.valid = true;
    _sshape_advance_offset(&buf.vertices);
    _sshape_advance_offset(&buf.indices);

    const float pi = 3.14159265358979323846f;
    const float two_pi = 2.0f * pi;
    const float du = 1.0f / params.slices;
    const float dv = 1.0f / params.stacks;

    // generate vertices
    for (uint32_t stack = 0; stack <= params.stacks; stack++) {
        const float stack_angle = (pi * stack) / params.stacks;
        const float sin_stack = sinf(stack_angle);
        const float cos_stack = cosf(stack_angle);
        for (uint32_t slice = 0; slice <= params.slices; slice++) {
            const float slice_angle = (two_pi * slice) / params.slices;
            const float sin_slice = sinf(slice_angle);
            const float cos_slice = cosf(slice_angle);
            const sshape_vec4_t norm = _sshape_vec4(sin_slice * sin_stack, cos_stack, cos_slice * sin_stack, 0.0f);
            const sshape_vec4_t pos = _sshape_vec4(norm.x * params.radius, norm.y * params.radius, norm.z * params.radius, 1.0f);
            const sshape_vec4_t tnorm = _sshape_mat4_mul(&params.transform, norm);
            const sshape_vec4_t tpos = _sshape_mat4_mul(&params.transform, pos);
            const sshape_vec2_t uv = _sshape_vec2(slice * du, stack * dv);
            _sshape_add_vertex(&buf, tpos, tnorm, uv, params.color);
        }
    }

    // generate indices
    const sshape_index_t start_index = _sshape_base_index(&buf);

    // north-pole triangles
    {
        const sshape_index_t row_a = start_index;
        const sshape_index_t row_b = row_a + params.slices + 1;
        for (uint32_t slice = 0; slice < params.slices; slice++) {
            _sshape_add_triangle(&buf, row_a + slice, row_b + slice, row_b + slice + 1);
        }
    }
    // stack triangles
    for (uint32_t stack = 1; stack < params.stacks - 1; stack++) {
        const sshape_index_t row_a = start_index + stack * (params.slices + 1);
        const sshape_index_t row_b = row_a + params.slices + 1;
        for (uint32_t slice = 0; slice < params.slices; slice++) {
            _sshape_add_triangle(&buf, row_a + slice, row_b + slice + 1, row_a + slice + 1);
            _sshape_add_triangle(&buf, row_a + slice, row_b + slice, row_b + slice + 1);
        }
    }
    // south-pole triangles
    {
        const sshape_index_t row_a = start_index + (params.stacks - 1) * (params.slices + 1);
        const sshape_index_t row_b = row_a + params.slices + 1;
        for (uint32_t slice = 0; slice < params.slices; slice++) {
            _sshape_add_triangle(&buf, row_a + slice, row_b + slice + 1, row_a + slice + 1);
        }
    }
    return buf;
}

/*
    Geometry for cylinders is as follows (2 stacks, 5 slices):

    +  +  +  +  +  +
    |\ |\ |\ |\ |\
    | \| \| \| \| \
    +--+--+--+--+--+
    +--+--+--+--+--+    42 vertices (2 wasted) (slices + 1) * (stacks + 5)
    |\ |\ |\ |\ |\ |    30 triangles (2 * slices * stacks) + (2 * slices)
    | \| \| \| \| \|
    +--+--+--+--+--+
    |\ |\ |\ |\ |\ |
    | \| \| \| \| \|
    +--+--+--+--+--+
    +--+--+--+--+--+
     \ |\ |\ |\ |\ |
      \| \| \| \| \|
    +  +  +  +  +  +
*/
static void _sshape_build_cylinder_cap_pole(sshape_buffer_t* buf, const sshape_cylinder_t* params, float pos_y, float norm_y, float du, float v) {
    const sshape_vec4_t tnorm = _sshape_mat4_mul(&params->transform, _sshape_vec4(0.0f, norm_y, 0.0f, 0.0f));
    const sshape_vec4_t tpos = _sshape_mat4_mul(&params->transform, _sshape_vec4(0.0f, pos_y, 0.0f, 1.0f));
    for (uint32_t slice = 0; slice <= params->slices; slice++) {
        const sshape_vec2_t uv = _sshape_vec2(slice * du, v);
        _sshape_add_vertex(buf, tpos, tnorm, uv, params->color);
    }
}

static void _sshape_build_cylinder_cap_ring(sshape_buffer_t* buf, const sshape_cylinder_t* params, float pos_y, float norm_y, float du, float v) {
    const float two_pi = 2.0 * 3.14159265358979323846;
    const sshape_vec4_t tnorm = _sshape_mat4_mul(&params->transform, _sshape_vec4(0.0f, norm_y, 0.0f, 0.0f));
    for (uint32_t slice = 0; slice <= params->slices; slice++) {
        const float slice_angle = (two_pi * slice) / params->slices;
        const float sin_slice = sinf(slice_angle);
        const float cos_slice = cosf(slice_angle);
        const sshape_vec4_t pos = _sshape_vec4(sin_slice * params->radius, pos_y, cos_slice * params->radius, 1.0f);
        const sshape_vec4_t tpos = _sshape_mat4_mul(&params->transform, pos);
        const sshape_vec2_t uv = _sshape_vec2(slice * du, v);
        _sshape_add_vertex(buf, tpos, tnorm, uv, params->color);
    }
}

SOKOL_API_DECL sshape_buffer_t sshape_build_cylinder(const sshape_buffer_t* in_buf, const sshape_cylinder_t* in_params) {
    SOKOL_ASSERT(in_buf && in_params);
    const sshape_cylinder_t params = _sshape_cylinder_defaults(in_params);
    const uint32_t num_vertices = _sshape_cylinder_num_vertices(params.slices, params.stacks);
    const uint32_t num_indices = _sshape_cylinder_num_indices(params.slices, params.stacks);
    sshape_buffer_t buf = *in_buf;
    if (!_sshape_validate_buffer(&buf, num_vertices, num_indices)) {
        buf.valid = false;
        return buf;
    }
    buf.valid = true;
    _sshape_advance_offset(&buf.vertices);
    _sshape_advance_offset(&buf.indices);

    const float two_pi = 2.0f * 3.14159265358979323846f;
    const float du = 1.0f / params.slices;
    const float dv = 1.0f / (params.stacks + 2);
    const float y0 = params.height * 0.5f;
    const float y1 = -params.height * 0.5f;
    const float dy = params.height / params.stacks;

    // generate vertices
    _sshape_build_cylinder_cap_pole(&buf, &params, y0, 1.0f, du, 0.0f);
    _sshape_build_cylinder_cap_ring(&buf, &params, y0, 1.0f, du, dv);
    for (uint32_t stack = 0; stack <= params.stacks; stack++) {
        const float y = y0 - dy * stack;
        const float v = dv * stack + dv;
        for (uint32_t slice = 0; slice <= params.slices; slice++) {
            const float slice_angle = (two_pi * slice) / params.slices;
            const float sin_slice = sinf(slice_angle);
            const float cos_slice = cosf(slice_angle);
            const sshape_vec4_t pos = _sshape_vec4(sin_slice * params.radius, y, cos_slice * params.radius, 1.0f);
            const sshape_vec4_t tpos = _sshape_mat4_mul(&params.transform, pos);
            const sshape_vec4_t norm = _sshape_vec4(sin_slice, 0.0f, cos_slice, 0.0f);
            const sshape_vec4_t tnorm = _sshape_mat4_mul(&params.transform, norm);
            const sshape_vec2_t uv = _sshape_vec2(slice * du, v);
            _sshape_add_vertex(&buf, tpos, tnorm, uv, params.color);
        }
    }
    _sshape_build_cylinder_cap_ring(&buf, &params, y1, -1.0f, du, 1.0f - dv);
    _sshape_build_cylinder_cap_pole(&buf, &params, y1, -1.0f, du, 1.0f);

    // generate indices
    const sshape_index_t start_index = _sshape_base_index(&buf);

    // top-cap indices
    {
        const sshape_index_t row_a = start_index;
        const sshape_index_t row_b = row_a + params.slices + 1;
        for (uint32_t slice = 0; slice < params.slices; slice++) {
            _sshape_add_triangle(&buf, row_a + slice, row_b + slice + 1, row_b + slice);
        }
    }
    // shaft triangles
    for (uint32_t stack = 0; stack < params.stacks; stack++) {
        const sshape_index_t row_a = start_index + (stack + 2) * (params.slices + 1);
        const sshape_index_t row_b = row_a + params.slices + 1;
        for (uint32_t slice = 0; slice < params.slices; slice++) {
            _sshape_add_triangle(&buf, row_a + slice, row_a + slice + 1, row_b + slice + 1);
            _sshape_add_triangle(&buf, row_a + slice, row_b + slice + 1, row_b + slice);
        }
    }
    // bottom-cap indices
    {
        const sshape_index_t row_a = start_index + (params.stacks + 3) * (params.slices + 1);
        const sshape_index_t row_b = row_a + params.slices + 1;
        for (uint32_t slice = 0; slice < params.slices; slice++) {
            _sshape_add_triangle(&buf, row_a + slice, row_a + slice + 1, row_b + slice + 1);
        }
    }
    return buf;
}

/*
    Geometry layout for torus (sides = 4, rings = 5):

    +--+--+--+--+--+
    |\ |\ |\ |\ |\ |
    | \| \| \| \| \|
    +--+--+--+--+--+    30 vertices (sides + 1) * (rings + 1)
    |\ |\ |\ |\ |\ |    40 triangles (2 * sides * rings)
    | \| \| \| \| \|
    +--+--+--+--+--+
    |\ |\ |\ |\ |\ |
    | \| \| \| \| \|
    +--+--+--+--+--+
    |\ |\ |\ |\ |\ |
    | \| \| \| \| \|
    +--+--+--+--+--+
*/
SOKOL_API_IMPL sshape_buffer_t sshape_build_torus(const sshape_buffer_t* in_buf, const sshape_torus_t* in_params) {
    SOKOL_ASSERT(in_buf && in_params);
    const sshape_torus_t params = _sshape_torus_defaults(in_params);
    const uint32_t num_vertices = _sshape_torus_num_vertices(params.sides, params.rings);
    const uint32_t num_indices = _sshape_torus_num_indices(params.sides, params.rings);
    sshape_buffer_t buf = *in_buf;
    if (!_sshape_validate_buffer(&buf, num_vertices, num_indices)) {
        buf.valid = false;
        return buf;
    }
    buf.valid = true;
    _sshape_advance_offset(&buf.vertices);
    _sshape_advance_offset(&buf.indices);

    const float two_pi = 2.0f * 3.14159265358979323846f;
    const float dv = 1.0f / params.sides;
    const float du = 1.0f / params.rings;

    // generate vertices
    for (uint32_t side = 0; side <= params.sides; side++) {
        const float phi = (side * two_pi) / params.sides;
        const float sin_phi = sinf(phi);
        const float cos_phi = cosf(phi);
        for (uint32_t ring = 0; ring <= params.rings; ring++) {
            const float theta = (ring * two_pi) / params.rings;
            const float sin_theta = sinf(theta);
            const float cos_theta = cosf(theta);

            // torus surface position
            const float spx = cos_theta * (params.radius + (params.ring_radius * cos_phi));
            const float spy = sin_phi * params.ring_radius;
            const float spz = -sin_theta * (params.radius + (params.ring_radius * cos_phi));

            // torus position with ring-radius zero (for normal computation)
            const float ipx = cos_theta * params.radius;
            const float ipy = 0.0f;
            const float ipz = -sin_theta * params.radius;

            const sshape_vec4_t pos = _sshape_vec4(spx, spy, spz, 1.0f);
            const sshape_vec4_t norm = _sshape_vec4_norm(_sshape_vec4(spx - ipx, spy - ipy, spz - ipz, 0.0f));
            const sshape_vec4_t tpos = _sshape_mat4_mul(&params.transform, pos);
            const sshape_vec4_t tnorm = _sshape_mat4_mul(&params.transform, norm);
            const sshape_vec2_t uv = _sshape_vec2(side * du, ring * dv);
            _sshape_add_vertex(&buf, tpos, tnorm, uv, params.color);
        }
    }

    // generate indices
    const sshape_index_t start_index = _sshape_base_index(&buf);
    for (uint32_t side = 0; side < params.sides; side++) {
        const sshape_index_t row_a = start_index + side * (params.rings + 1);
        const sshape_index_t row_b = row_a + params.rings + 1;
        for (uint32_t ring = 0; ring < params.rings; ring++) {
            _sshape_add_triangle(&buf, row_a + ring, row_a + ring + 1, row_b + ring + 1);
            _sshape_add_triangle(&buf, row_a + ring, row_b + ring + 1, row_b + ring);
        }
    }
    return buf;
}

SOKOL_API_IMPL sg_buffer_desc sshape_vertex_buffer_desc(const sshape_buffer_t* buf) {
    SOKOL_ASSERT(buf && buf->valid);
    sg_buffer_desc desc = { 0 };
    desc.size = buf->vertices.data_size;
    desc.type = SG_BUFFERTYPE_VERTEXBUFFER;
    desc.usage = SG_USAGE_IMMUTABLE;
    desc.content = buf->vertices.buffer_ptr;
    return desc;
}

SOKOL_API_IMPL sg_buffer_desc sshape_index_buffer_desc(const sshape_buffer_t* buf) {
    SOKOL_ASSERT(buf && buf->valid);
    sg_buffer_desc desc = { 0 };
    desc.size = buf->indices.data_size;
    desc.type = SG_BUFFERTYPE_INDEXBUFFER;
    desc.usage = SG_USAGE_IMMUTABLE;
    desc.content = buf->indices.buffer_ptr;
    return desc;
}

SOKOL_API_DECL sshape_element_range_t sshape_element_range(const sshape_buffer_t* buf) {
    SOKOL_ASSERT(buf && buf->valid);
    SOKOL_ASSERT(buf->indices.shape_offset < buf->indices.data_size);
    SOKOL_ASSERT(0 == (buf->indices.shape_offset & (sizeof(sshape_index_t) - 1)));
    SOKOL_ASSERT(0 == (buf->indices.data_size & (sizeof(sshape_index_t) - 1)));
    sshape_element_range_t range = { 0 };
    range.base_element = buf->indices.shape_offset / sizeof(sshape_index_t);
    range.num_elements = (buf->indices.data_size - buf->indices.shape_offset) / sizeof(sshape_index_t);
    return range;
}

SOKOL_API_IMPL sg_buffer_layout_desc sshape_buffer_layout_desc(void) {
    sg_buffer_layout_desc desc = { 0 };
    desc.stride = sizeof(sshape_vertex_t);
    return desc;
}

SOKOL_API_IMPL sg_vertex_attr_desc sshape_position_attr_desc(void) {
    sg_vertex_attr_desc desc = { 0 };
    desc.offset = offsetof(sshape_vertex_t, position);
    desc.format = SG_VERTEXFORMAT_FLOAT3;
    return desc;
}

SOKOL_API_IMPL sg_vertex_attr_desc sshape_normal_attr_desc(void) {
    sg_vertex_attr_desc desc = { 0 };
    desc.offset = offsetof(sshape_vertex_t, normal);
    desc.format = SG_VERTEXFORMAT_FLOAT3;
    return desc;
}

SOKOL_API_IMPL sg_vertex_attr_desc sshape_texcoord_attr_desc(void) {
    sg_vertex_attr_desc desc = { 0 };
    desc.offset = offsetof(sshape_vertex_t, texcoord);
    desc.format = SG_VERTEXFORMAT_FLOAT2;
    return desc;
}

SOKOL_API_IMPL sg_vertex_attr_desc sshape_color_attr_desc(void) {
    sg_vertex_attr_desc desc = { 0 };
    desc.offset = offsetof(sshape_vertex_t, color);
    desc.format = SG_VERTEXFORMAT_UBYTE4N;
    return desc;
}

#endif // SOKOL_SHAPE_IMPL
