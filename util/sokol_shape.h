#if defined(SOKOL_IMPL) && !defined(SOKOL_SHAPE_IMPL)
#define SOKOL_SHAPE_IMPL
#endif
#ifndef SOKOL_SHAPE_INCLUDED
/*
    sokol_shape.h -- create simple primitive shapes for sokol_gfx.h

    Project URL: https://github.com/floooh/sokol

    Do this:
        #define SOKOL_IMPL or
        #define SOKOL_SHAPE_IMPL
    before you include this file in *one* C or C++ file to create the
    implementation.

    Include the following headers before including sokol_shape.h:

        sokol_gfx.h

    ...optionally provide the following macros to override defaults:

    SOKOL_ASSERT(c)     - your own assert macro (default: assert(c))
    SOKOL_SHAPE_API_DECL- public function declaration prefix (default: extern)
    SOKOL_API_DECL      - same as SOKOL_SHAPE_API_DECL
    SOKOL_API_IMPL      - public function implementation prefix (default: -)

    If sokol_shape.h is compiled as a DLL, define the following before
    including the declaration or implementation:

    SOKOL_DLL

    On Windows, SOKOL_DLL will define SOKOL_SHAPE_API_DECL as __declspec(dllexport)
    or __declspec(dllimport) as needed.

    FEATURE OVERVIEW
    ================
    sokol_shape.h creates vertices and indices for simple shapes and
    builds structs which can be plugged into sokol-gfx resource
    creation functions:

    The following shape types are supported:

        - plane
        - cube
        - sphere (with poles, not geodesic)
        - cylinder
        - torus (donut)

    Generated vertices look like this:

        typedef struct sshape_vertex_t {
            float x, y, z;
            uint32_t normal;        // packed normal as BYTE4N
            uint16_t u, v;          // packed uv coords as USHORT2N
            uint32_t color;         // packed color as UBYTE4N (r,g,b,a);
        } sshape_vertex_t;

    Indices are generally 16-bits wide (SG_INDEXTYPE_UINT16) and the indices
    are written as triangle-lists (SG_PRIMITIVETYPE_TRIANGLES).

    EXAMPLES:
    =========

    Create multiple shapes into the same vertex- and index-buffer and
    render with separate draw calls:

    https://github.com/floooh/sokol-samples/blob/master/sapp/shapes-sapp.c

    Same as the above, but pre-transform shapes and merge them into a single
    shape that's rendered with a single draw call.

    https://github.com/floooh/sokol-samples/blob/master/sapp/shapes-transform-sapp.c

    STEP-BY-STEP:
    =============

    Setup an sshape_buffer_t struct with pointers to memory buffers where
    generated vertices and indices will be written to:

    ```c
    sshape_vertex_t vertices[512];
    uint16_t indices[4096];

    sshape_buffer_t buf = {
        .vertices = {
            .buffer = SSHAPE_RANGE(vertices),
        },
        .indices = {
            .buffer = SSHAPE_RANGE(indices),
        }
    };
    ```

    To find out how big those memory buffers must be (in case you want
    to allocate dynamically) call the following functions:

    ```c
    sshape_sizes_t sshape_plane_sizes(uint32_t tiles);
    sshape_sizes_t sshape_box_sizes(uint32_t tiles);
    sshape_sizes_t sshape_sphere_sizes(uint32_t slices, uint32_t stacks);
    sshape_sizes_t sshape_cylinder_sizes(uint32_t slices, uint32_t stacks);
    sshape_sizes_t sshape_torus_sizes(uint32_t sides, uint32_t rings);
    ```

    The returned sshape_sizes_t struct contains vertex- and index-counts
    as well as the equivalent buffer sizes in bytes. For instance:

    ```c
    sshape_sizes_t sizes = sshape_sphere_sizes(36, 12);
    uint32_t num_vertices = sizes.vertices.num;
    uint32_t num_indices = sizes.indices.num;
    uint32_t vertex_buffer_size = sizes.vertices.size;
    uint32_t index_buffer_size = sizes.indices.size;
    ```

    With the sshape_buffer_t struct that was setup earlier, call any
    of the shape-builder functions:

    ```c
    sshape_buffer_t sshape_build_plane(const sshape_buffer_t* buf, const sshape_plane_t* params);
    sshape_buffer_t sshape_build_box(const sshape_buffer_t* buf, const sshape_box_t* params);
    sshape_buffer_t sshape_build_sphere(const sshape_buffer_t* buf, const sshape_sphere_t* params);
    sshape_buffer_t sshape_build_cylinder(const sshape_buffer_t* buf, const sshape_cylinder_t* params);
    sshape_buffer_t sshape_build_torus(const sshape_buffer_t* buf, const sshape_torus_t* params);
    ```

    Note how the sshape_buffer_t struct is both an input value and the
    return value. This can be used to append multiple shapes into the
    same vertex- and index-buffers (more on this later).

    The second argument is a struct which holds creation parameters.

    For instance to build a sphere with radius 2, 36 "cake slices" and 12 stacks:

    ```c
    sshape_buffer_t buf = ...;
    buf = sshape_build_sphere(&buf, &(sshape_sphere_t){
        .radius = 2.0f,
        .slices = 36,
        .stacks = 12,
    });
    ```

    If the provided buffers are big enough to hold all generated vertices and
    indices, the "valid" field in the result will be true:

    ```c
    assert(buf.valid);
    ```

    The shape creation parameters have "useful defaults", refer to the
    actual C struct declarations below to look up those defaults.

    You can also provide additional creation parameters, like a common vertex
    color, a debug-helper to randomize colors, tell the shape builder function
    to merge the new shape with the previous shape into the same draw-element-range,
    or a 4x4 transform matrix to move, rotate and scale the generated vertices:

    ```c
    sshape_buffer_t buf = ...;
    buf = sshape_build_sphere(&buf, &(sshape_sphere_t){
        .radius = 2.0f,
        .slices = 36,
        .stacks = 12,
        // merge with previous shape into a single element-range
        .merge = true,
        // set vertex color to red+opaque
        .color = sshape_color_4f(1.0f, 0.0f, 0.0f, 1.0f),
        // set position to y = 2.0
        .transform = {
            .m = {
                { 1.0f, 0.0f, 0.0f, 0.0f },
                { 0.0f, 1.0f, 0.0f, 0.0f },
                { 0.0f, 0.0f, 1.0f, 0.0f },
                { 0.0f, 2.0f, 0.0f, 1.0f },
            }
        }
    });
    assert(buf.valid);
    ```

    The following helper functions can be used to build a packed
    color value or to convert from external matrix types:

    ```c
    uint32_t sshape_color_4f(float r, float g, float b, float a);
    uint32_t sshape_color_3f(float r, float g, float b);
    uint32_t sshape_color_4b(uint8_t r, uint8_t g, uint8_t b, uint8_t a);
    uint32_t sshape_color_3b(uint8_t r, uint8_t g, uint8_t b);
    sshape_mat4_t sshape_mat4(const float m[16]);
    sshape_mat4_t sshape_mat4_transpose(const float m[16]);
    ```

    After the shape builder function has been called, the following functions
    are used to extract the build result for plugging into sokol_gfx.h:

    ```c
    sshape_element_range_t sshape_element_range(const sshape_buffer_t* buf);
    sg_buffer_desc sshape_vertex_buffer_desc(const sshape_buffer_t* buf);
    sg_buffer_desc sshape_index_buffer_desc(const sshape_buffer_t* buf);
    sg_buffer_layout_desc sshape_buffer_layout_desc(void);
    sg_vertex_attr_desc sshape_position_attr_desc(void);
    sg_vertex_attr_desc sshape_normal_attr_desc(void);
    sg_vertex_attr_desc sshape_texcoord_attr_desc(void);
    sg_vertex_attr_desc sshape_color_attr_desc(void);
    ```

    The sshape_element_range_t struct contains the base-index and number of
    indices which can be plugged into the sg_draw() call:

    ```c
    sshape_element_range_t elms = sshape_element_range(&buf);
    ...
    sg_draw(elms.base_element, elms.num_elements, 1);
    ```

    To create sokol-gfx vertex- and index-buffers from the generated
    shape data:

    ```c
    // create sokol-gfx vertex buffer
    sg_buffer_desc vbuf_desc = sshape_vertex_buffer_desc(&buf);
    sg_buffer vbuf = sg_make_buffer(&vbuf_desc);

    // create sokol-gfx index buffer
    sg_buffer_desc ibuf_desc = sshape_index_buffer_desc(&buf);
    sg_buffer ibuf = sg_make_buffer(&ibuf_desc);
    ```

    The remaining functions are used to populate the vertex-layout item
    in sg_pipeline_desc, note that these functions don't depend on the
    created geometry, they always return the same result:

    ```c
    sg_pipeline pip = sg_make_pipeline(&(sg_pipeline_desc){
        .layout = {
            .buffers[0] = sshape_buffer_layout_desc(),
            .attrs = {
                [0] = sshape_position_attr_desc(),
                [1] = ssape_normal_attr_desc(),
                [2] = sshape_texcoord_attr_desc(),
                [3] = sshape_color_attr_desc()
            }
        },
        ...
    });
    ```

    Note that you don't have to use all generated vertex attributes in the
    pipeline's vertex layout, the sg_buffer_layout_desc struct returned
    by sshape_buffer_layout_desc() contains the correct vertex stride
    to skip vertex components.

    WRITING MULTIPLE SHAPES INTO THE SAME BUFFER
    ============================================
    You can merge multiple shapes into the same vertex- and
    index-buffers and either render them as a single shape, or
    in separate draw calls.

    To build a single shape made of two cubes which can be rendered
    in a single draw-call:

    ```
    sshape_vertex_t vertices[128];
    uint16_t indices[16];

    sshape_buffer_t buf = {
        .vertices.buffer = SSHAPE_RANGE(vertices),
        .indices.buffer  = SSHAPE_RANGE(indices)
    };

    // first cube at pos x=-2.0 (with default size of 1x1x1)
    buf = sshape_build_cube(&buf, &(sshape_box_t){
        .transform = {
            .m = {
                { 1.0f, 0.0f, 0.0f, 0.0f },
                { 0.0f, 1.0f, 0.0f, 0.0f },
                { 0.0f, 0.0f, 1.0f, 0.0f },
                {-2.0f, 0.0f, 0.0f, 1.0f },
            }
        }
    });
    // ...and append another cube at pos pos=+1.0
    // NOTE the .merge = true, this tells the shape builder
    // function to not advance the current shape start offset
    buf = sshape_build_cube(&buf, &(sshape_box_t){
        .merge = true,
        .transform = {
            .m = {
                { 1.0f, 0.0f, 0.0f, 0.0f },
                { 0.0f, 1.0f, 0.0f, 0.0f },
                { 0.0f, 0.0f, 1.0f, 0.0f },
                {-2.0f, 0.0f, 0.0f, 1.0f },
            }
        }
    });
    assert(buf.valid);

    // skipping buffer- and pipeline-creation...

    sshape_element_range_t elms = sshape_element_range(&buf);
    sg_draw(elms.base_element, elms.num_elements, 1);
    ```

    To render the two cubes in separate draw-calls, the element-ranges used
    in the sg_draw() calls must be captured right after calling the
    builder-functions:

    ```c
    sshape_vertex_t vertices[128];
    uint16_t indices[16];
    sshape_buffer_t buf = {
        .vertices.buffer = SSHAPE_RANGE(vertices),
        .indices.buffer = SSHAPE_RANGE(indices)
    };

    // build a red cube...
    buf = sshape_build_cube(&buf, &(sshape_box_t){
        .color = sshape_color_3b(255, 0, 0)
    });
    sshape_element_range_t red_cube = sshape_element_range(&buf);

    // append a green cube to the same vertex-/index-buffer:
    buf = sshape_build_cube(&bud, &sshape_box_t){
        .color = sshape_color_3b(0, 255, 0);
    });
    sshape_element_range_t green_cube = sshape_element_range(&buf);

    // skipping buffer- and pipeline-creation...

    sg_draw(red_cube.base_element, red_cube.num_elements, 1);
    sg_draw(green_cube.base_element, green_cube.num_elements, 1);
    ```

    ...that's about all :)

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
#include <stddef.h>     // size_t, offsetof
#include <stdint.h>
#include <stdbool.h>

#if !defined(SOKOL_GFX_INCLUDED)
#error "Please include sokol_gfx.h before sokol_shape.h"
#endif

#if defined(SOKOL_API_DECL) && !defined(SOKOL_SHAPE_API_DECL)
#define SOKOL_SHAPE_API_DECL SOKOL_API_DECL
#endif
#ifndef SOKOL_SHAPE_API_DECL
#if defined(_WIN32) && defined(SOKOL_DLL) && defined(SOKOL_SHAPE_IMPL)
#define SOKOL_SHAPE_API_DECL __declspec(dllexport)
#elif defined(_WIN32) && defined(SOKOL_DLL)
#define SOKOL_SHAPE_API_DECL __declspec(dllimport)
#else
#define SOKOL_SHAPE_API_DECL extern
#endif
#endif

#ifdef __cplusplus
extern "C" {
#endif

/*
    sshape_range is a pointer-size-pair struct used to pass memory
    blobs into sokol-shape. When initialized from a value type
    (array or struct), use the SSHAPE_RANGE() macro to build
    an sshape_range struct.
*/
typedef struct sshape_range {
    const void* ptr;
    size_t size;
} sshape_range;

// disabling this for every includer isn't great, but the warning is also quite pointless
#if defined(_MSC_VER)
#pragma warning(disable:4221)   /* /W4 only: nonstandard extension used: 'x': cannot be initialized using address of automatic variable 'y' */
#endif
#if defined(__cplusplus)
#define SSHAPE_RANGE(x) sshape_range{ &x, sizeof(x) }
#else
#define SSHAPE_RANGE(x) (sshape_range){ &x, sizeof(x) }
#endif

/* a 4x4 matrix wrapper struct */
typedef struct sshape_mat4_t { float m[4][4]; } sshape_mat4_t;

/* vertex layout of the generated geometry */
typedef struct sshape_vertex_t {
    float x, y, z;
    uint32_t normal;        // packed normal as BYTE4N
    uint16_t u, v;          // packed uv coords as USHORT2N
    uint32_t color;         // packed color as UBYTE4N (r,g,b,a);
} sshape_vertex_t;

/* a range of draw-elements (sg_draw(int base_element, int num_element, ...)) */
typedef struct sshape_element_range_t {
    int base_element;
    int num_elements;
    #if defined(SOKOL_ZIG_BINDINGS)
    uint32_t __pad[3];
    #endif
} sshape_element_range_t;

/* number of elements and byte size of build actions */
typedef struct sshape_sizes_item_t {
    uint32_t num;       // number of elements
    uint32_t size;      // the same as size in bytes
    #if defined(SOKOL_ZIG_BINDINGS)
    uint32_t __pad[3];
    #endif
} sshape_sizes_item_t;

typedef struct sshape_sizes_t {
    sshape_sizes_item_t vertices;
    sshape_sizes_item_t indices;
} sshape_sizes_t;

/* in/out struct to keep track of mesh-build state */
typedef struct sshape_buffer_item_t {
    sshape_range buffer;    // pointer/size pair of output buffer
    size_t data_size;       // size in bytes of valid data in buffer
    size_t shape_offset;    // data offset of the most recent shape
} sshape_buffer_item_t;

typedef struct sshape_buffer_t {
    bool valid;
    sshape_buffer_item_t vertices;
    sshape_buffer_item_t indices;
} sshape_buffer_t;

/* creation parameters for the different shape types */
typedef struct sshape_plane_t {
    float width, depth;             // default: 1.0
    uint16_t tiles;                 // default: 1
    uint32_t color;                 // default: white
    bool random_colors;             // default: false
    bool merge;                     // if true merge with previous shape (default: false)
    sshape_mat4_t transform;        // default: identity matrix
} sshape_plane_t;

typedef struct sshape_box_t {
    float width, height, depth;     // default: 1.0
    uint16_t tiles;                 // default: 1
    uint32_t color;                 // default: white
    bool random_colors;             // default: false
    bool merge;                     // if true merge with previous shape (default: false)
    sshape_mat4_t transform;        // default: identity matrix
} sshape_box_t;

typedef struct sshape_sphere_t {
    float radius;                   // default: 0.5
    uint16_t slices;                // default: 5
    uint16_t stacks;                // default: 4
    uint32_t color;                 // default: white
    bool random_colors;             // default: false
    bool merge;                     // if true merge with previous shape (default: false)
    sshape_mat4_t transform;        // default: identity matrix
} sshape_sphere_t;

typedef struct sshape_cylinder_t {
    float radius;                   // default: 0.5
    float height;                   // default: 1.0
    uint16_t slices;                // default: 5
    uint16_t stacks;                // default: 1
    uint32_t color;                 // default: white
    bool random_colors;             // default: false
    bool merge;                     // if true merge with previous shape (default: false)
    sshape_mat4_t transform;        // default: identity matrix
} sshape_cylinder_t;

typedef struct sshape_torus_t {
    float radius;                   // default: 0.5f
    float ring_radius;              // default: 0.2f
    uint16_t sides;                 // default: 5
    uint16_t rings;                 // default: 5
    uint32_t color;                 // default: white
    bool random_colors;             // default: false
    bool merge;                     // if true merge with previous shape (default: false)
    sshape_mat4_t transform;        // default: identity matrix
} sshape_torus_t;

/* shape builder functions */
SOKOL_SHAPE_API_DECL sshape_buffer_t sshape_build_plane(const sshape_buffer_t* buf, const sshape_plane_t* params);
SOKOL_SHAPE_API_DECL sshape_buffer_t sshape_build_box(const sshape_buffer_t* buf, const sshape_box_t* params);
SOKOL_SHAPE_API_DECL sshape_buffer_t sshape_build_sphere(const sshape_buffer_t* buf, const sshape_sphere_t* params);
SOKOL_SHAPE_API_DECL sshape_buffer_t sshape_build_cylinder(const sshape_buffer_t* buf, const sshape_cylinder_t* params);
SOKOL_SHAPE_API_DECL sshape_buffer_t sshape_build_torus(const sshape_buffer_t* buf, const sshape_torus_t* params);

/* query required vertex- and index-buffer sizes in bytes */
SOKOL_SHAPE_API_DECL sshape_sizes_t sshape_plane_sizes(uint32_t tiles);
SOKOL_SHAPE_API_DECL sshape_sizes_t sshape_box_sizes(uint32_t tiles);
SOKOL_SHAPE_API_DECL sshape_sizes_t sshape_sphere_sizes(uint32_t slices, uint32_t stacks);
SOKOL_SHAPE_API_DECL sshape_sizes_t sshape_cylinder_sizes(uint32_t slices, uint32_t stacks);
SOKOL_SHAPE_API_DECL sshape_sizes_t sshape_torus_sizes(uint32_t sides, uint32_t rings);

/* extract sokol-gfx desc structs and primitive ranges from build state */
SOKOL_SHAPE_API_DECL sshape_element_range_t sshape_element_range(const sshape_buffer_t* buf);
SOKOL_SHAPE_API_DECL sg_buffer_desc sshape_vertex_buffer_desc(const sshape_buffer_t* buf);
SOKOL_SHAPE_API_DECL sg_buffer_desc sshape_index_buffer_desc(const sshape_buffer_t* buf);
SOKOL_SHAPE_API_DECL sg_buffer_layout_desc sshape_buffer_layout_desc(void);
SOKOL_SHAPE_API_DECL sg_vertex_attr_desc sshape_position_attr_desc(void);
SOKOL_SHAPE_API_DECL sg_vertex_attr_desc sshape_normal_attr_desc(void);
SOKOL_SHAPE_API_DECL sg_vertex_attr_desc sshape_texcoord_attr_desc(void);
SOKOL_SHAPE_API_DECL sg_vertex_attr_desc sshape_color_attr_desc(void);

/* helper functions to build packed color value from floats or bytes */
SOKOL_SHAPE_API_DECL uint32_t sshape_color_4f(float r, float g, float b, float a);
SOKOL_SHAPE_API_DECL uint32_t sshape_color_3f(float r, float g, float b);
SOKOL_SHAPE_API_DECL uint32_t sshape_color_4b(uint8_t r, uint8_t g, uint8_t b, uint8_t a);
SOKOL_SHAPE_API_DECL uint32_t sshape_color_3b(uint8_t r, uint8_t g, uint8_t b);

/* adapter function for filling matrix struct from generic float[16] array */
SOKOL_SHAPE_API_DECL sshape_mat4_t sshape_mat4(const float m[16]);
SOKOL_SHAPE_API_DECL sshape_mat4_t sshape_mat4_transpose(const float m[16]);

#ifdef __cplusplus
} // extern "C"

// FIXME: C++ helper functions

#endif
#endif // SOKOL_SHAPE_INCLUDED

/*-- IMPLEMENTATION ----------------------------------------------------------*/
#ifdef SOKOL_SHAPE_IMPL
#define SOKOL_SHAPE_IMPL_INCLUDED (1)

#include <string.h> // memcpy
#include <math.h>   // sinf, cosf

#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wmissing-field-initializers"
#endif

#ifndef SOKOL_API_IMPL
    #define SOKOL_API_IMPL
#endif
#ifndef SOKOL_ASSERT
    #include <assert.h>
    #define SOKOL_ASSERT(c) assert(c)
#endif

#define _sshape_def(val, def) (((val) == 0) ? (def) : (val))
#define _sshape_def_flt(val, def) (((val) == 0.0f) ? (def) : (val))
#define _sshape_white (0xFFFFFFFF)

typedef struct { float x, y, z, w; } _sshape_vec4_t;
typedef struct { float x, y; } _sshape_vec2_t;

static inline float _sshape_clamp(float v) {
    if (v < 0.0f) return 0.0f;
    else if (v > 1.0f) return 1.0f;
    else return v;
}

static inline uint32_t _sshape_pack_ub4_ubyte4n(uint8_t x, uint8_t y, uint8_t z, uint8_t w) {
    return (uint32_t)(((uint32_t)w<<24)|((uint32_t)z<<16)|((uint32_t)y<<8)|x);
}

static inline uint32_t _sshape_pack_f4_ubyte4n(float x, float y, float z, float w) {
    uint8_t x8 = (uint8_t) (x * 255.0f);
    uint8_t y8 = (uint8_t) (y * 255.0f);
    uint8_t z8 = (uint8_t) (z * 255.0f);
    uint8_t w8 = (uint8_t) (w * 255.0f);
    return _sshape_pack_ub4_ubyte4n(x8, y8, z8, w8);
}

static inline uint32_t _sshape_pack_f4_byte4n(float x, float y, float z, float w) {
    int8_t x8 = (int8_t) (x * 127.0f);
    int8_t y8 = (int8_t) (y * 127.0f);
    int8_t z8 = (int8_t) (z * 127.0f);
    int8_t w8 = (int8_t) (w * 127.0f);
    return _sshape_pack_ub4_ubyte4n((uint8_t)x8, (uint8_t)y8, (uint8_t)z8, (uint8_t)w8);
}

static inline uint16_t _sshape_pack_f_ushortn(float x) {
    return (uint16_t) (x * 65535.0f);
}

static inline _sshape_vec4_t _sshape_vec4(float x, float y, float z, float w) {
    _sshape_vec4_t v = { x, y, z, w };
    return v;
}

static inline _sshape_vec4_t _sshape_vec4_norm(_sshape_vec4_t v) {
    float l = sqrtf(v.x*v.x + v.y*v.y + v.z*v.z + v.w*v.w);
    if (l != 0.0f) {
        return _sshape_vec4(v.x/l, v.y/l, v.z/l, v.w/l);
    }
    else {
        return _sshape_vec4(0.0f, 1.0f, 0.0f, 0.0f);
    }
}

static inline _sshape_vec2_t _sshape_vec2(float x, float y) {
    _sshape_vec2_t v = { x, y };
    return v;
}

static bool _sshape_mat4_isnull(const sshape_mat4_t* m) {
    for (int y = 0; y < 4; y++) {
        for (int x = 0; x < 4; x++) {
            if (0.0f != m->m[y][x]) {
                return false;
            }
        }
    }
    return true;
}

static sshape_mat4_t _sshape_mat4_identity(void) {
    sshape_mat4_t m = {
        {
            { 1.0f, 0.0f, 0.0f, 0.0f },
            { 0.0f, 1.0f, 0.0f, 0.0f },
            { 0.0f, 0.0f, 1.0f, 0.0f },
            { 0.0f, 0.0f, 0.0f, 1.0f }
        }
    };
    return m;
}

static _sshape_vec4_t _sshape_mat4_mul(const sshape_mat4_t* m, _sshape_vec4_t v) {
    _sshape_vec4_t res = {
        m->m[0][0]*v.x + m->m[1][0]*v.y + m->m[2][0]*v.z + m->m[3][0]*v.w,
        m->m[0][1]*v.x + m->m[1][1]*v.y + m->m[2][1]*v.z + m->m[3][1]*v.w,
        m->m[0][2]*v.x + m->m[1][2]*v.y + m->m[2][2]*v.z + m->m[3][2]*v.w,
        m->m[0][3]*v.x + m->m[1][3]*v.y + m->m[2][3]*v.z + m->m[3][3]*v.w
    };
    return res;
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
    if (0 == item->buffer.ptr) {
        return false;
    }
    if (0 == item->buffer.size) {
        return false;
    }
    if ((item->data_size + build_size) > item->buffer.size) {
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
    if (!_sshape_validate_buffer_item(&buf->indices, num_indices * sizeof(uint16_t))) {
        return false;
    }
    return true;
}

static void _sshape_advance_offset(sshape_buffer_item_t* item) {
    item->shape_offset = item->data_size;
}

static uint16_t _sshape_base_index(const sshape_buffer_t* buf) {
    return (uint16_t) (buf->vertices.data_size / sizeof(sshape_vertex_t));
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

static void _sshape_add_vertex(sshape_buffer_t* buf, _sshape_vec4_t pos, _sshape_vec4_t norm, _sshape_vec2_t uv, uint32_t color) {
    size_t offset = buf->vertices.data_size;
    SOKOL_ASSERT((offset + sizeof(sshape_vertex_t)) <= buf->vertices.buffer.size);
    buf->vertices.data_size += sizeof(sshape_vertex_t);
    sshape_vertex_t* v_ptr = (sshape_vertex_t*) ((uint8_t*)buf->vertices.buffer.ptr + offset);
    v_ptr->x = pos.x;
    v_ptr->y = pos.y;
    v_ptr->z = pos.z;
    v_ptr->normal = _sshape_pack_f4_byte4n(norm.x, norm.y, norm.z, norm.w);
    v_ptr->u = _sshape_pack_f_ushortn(uv.x);
    v_ptr->v = _sshape_pack_f_ushortn(uv.y);
    v_ptr->color = color;
}

static void _sshape_add_triangle(sshape_buffer_t* buf, uint16_t i0, uint16_t i1, uint16_t i2) {
    size_t offset = buf->indices.data_size;
    SOKOL_ASSERT((offset + 3*sizeof(uint16_t)) <= buf->indices.buffer.size);
    buf->indices.data_size += 3*sizeof(uint16_t);
    uint16_t* i_ptr = (uint16_t*) ((uint8_t*)buf->indices.buffer.ptr + offset);
    i_ptr[0] = i0;
    i_ptr[1] = i1;
    i_ptr[2] = i2;
}

static uint32_t _sshape_rand_color(uint32_t* xorshift_state) {
    // xorshift32
    uint32_t x = *xorshift_state;
    x ^= x<<13;
    x ^= x>>17;
    x ^= x<<5;
    *xorshift_state = x;

    // rand => bright color with alpha 1.0
    x |= 0xFF000000;
    return x;

}

/*=== PUBLIC API FUNCTIONS ===================================================*/
SOKOL_API_IMPL uint32_t sshape_color_4f(float r, float g, float b, float a) {
    return _sshape_pack_f4_ubyte4n(_sshape_clamp(r), _sshape_clamp(g), _sshape_clamp(b), _sshape_clamp(a));
}

SOKOL_API_IMPL uint32_t sshape_color_3f(float r, float g, float b) {
    return _sshape_pack_f4_ubyte4n(_sshape_clamp(r), _sshape_clamp(g), _sshape_clamp(b), 1.0f);
}

SOKOL_API_IMPL uint32_t sshape_color_4b(uint8_t r, uint8_t g, uint8_t b, uint8_t a) {
    return _sshape_pack_ub4_ubyte4n(r, g, b, a);
}

SOKOL_API_IMPL uint32_t sshape_color_3b(uint8_t r, uint8_t g, uint8_t b) {
    return _sshape_pack_ub4_ubyte4n(r, g, b, 255);
}

SOKOL_API_IMPL sshape_mat4_t sshape_mat4(const float m[16]) {
    sshape_mat4_t res;
    memcpy(&res.m[0][0], &m[0], 64);
    return res;
}

SOKOL_API_IMPL sshape_mat4_t sshape_mat4_transpose(const float m[16]) {
    sshape_mat4_t res;
    for (int c = 0; c < 4; c++) {
        for (int r = 0; r < 4; r++) {
            res.m[r][c] = m[c*4 + r];
        }
    }
    return res;
}

SOKOL_API_IMPL sshape_sizes_t sshape_plane_sizes(uint32_t tiles) {
    SOKOL_ASSERT(tiles >= 1);
    sshape_sizes_t res = { {0} };
    res.vertices.num = _sshape_plane_num_vertices(tiles);
    res.indices.num = _sshape_plane_num_indices(tiles);
    res.vertices.size = res.vertices.num * sizeof(sshape_vertex_t);
    res.indices.size = res.indices.num * sizeof(uint16_t);
    return res;
}

SOKOL_API_IMPL sshape_sizes_t sshape_box_sizes(uint32_t tiles) {
    SOKOL_ASSERT(tiles >= 1);
    sshape_sizes_t res = { {0} };
    res.vertices.num = _sshape_box_num_vertices(tiles);
    res.indices.num = _sshape_box_num_indices(tiles);
    res.vertices.size = res.vertices.num * sizeof(sshape_vertex_t);
    res.indices.size = res.indices.num * sizeof(uint16_t);
    return res;
}

SOKOL_API_IMPL sshape_sizes_t sshape_sphere_sizes(uint32_t slices, uint32_t stacks) {
    SOKOL_ASSERT((slices >= 3) && (stacks >= 2));
    sshape_sizes_t res = { {0} };
    res.vertices.num = _sshape_sphere_num_vertices(slices, stacks);
    res.indices.num = _sshape_sphere_num_indices(slices, stacks);
    res.vertices.size = res.vertices.num * sizeof(sshape_vertex_t);
    res.indices.size = res.indices.num * sizeof(uint16_t);
    return res;
}

SOKOL_API_IMPL sshape_sizes_t sshape_cylinder_sizes(uint32_t slices, uint32_t stacks) {
    SOKOL_ASSERT((slices >= 3) && (stacks >= 1));
    sshape_sizes_t res = { {0} };
    res.vertices.num = _sshape_cylinder_num_vertices(slices, stacks);
    res.indices.num = _sshape_cylinder_num_indices(slices, stacks);
    res.vertices.size = res.vertices.num * sizeof(sshape_vertex_t);
    res.indices.size = res.indices.num * sizeof(uint16_t);
    return res;
}

SOKOL_API_IMPL sshape_sizes_t sshape_torus_sizes(uint32_t sides, uint32_t rings) {
    SOKOL_ASSERT((sides >= 3) && (rings >= 3));
    sshape_sizes_t res = { {0} };
    res.vertices.num = _sshape_torus_num_vertices(sides, rings);
    res.indices.num = _sshape_torus_num_indices(sides, rings);
    res.vertices.size = res.vertices.num * sizeof(sshape_vertex_t);
    res.indices.size = res.indices.num * sizeof(uint16_t);
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
    const uint16_t start_index = _sshape_base_index(&buf);
    if (!params.merge) {
        _sshape_advance_offset(&buf.vertices);
        _sshape_advance_offset(&buf.indices);
    }

    // write vertices
    uint32_t rand_seed = 0x12345678;
    const float x0 = -params.width * 0.5f;
    const float z0 =  params.depth * 0.5f;
    const float dx =  params.width / params.tiles;
    const float dz = -params.depth / params.tiles;
    const float duv = 1.0f / params.tiles;
    _sshape_vec4_t tnorm = _sshape_vec4_norm(_sshape_mat4_mul(&params.transform, _sshape_vec4(0.0f, 1.0f, 0.0f, 0.0f)));
    for (uint32_t ix = 0; ix <= params.tiles; ix++) {
        for (uint32_t iz = 0; iz <= params.tiles; iz++) {
            const _sshape_vec4_t pos = _sshape_vec4(x0 + dx*ix, 0.0f, z0 + dz*iz, 1.0f);
            const _sshape_vec4_t tpos = _sshape_mat4_mul(&params.transform, pos);
            const _sshape_vec2_t uv = _sshape_vec2(duv*ix, duv*iz);
            const uint32_t color = params.random_colors ? _sshape_rand_color(&rand_seed) : params.color;
            _sshape_add_vertex(&buf, tpos, tnorm, uv, color);
        }
    }

    // write indices
    for (uint16_t j = 0; j < params.tiles; j++) {
        for (uint16_t i = 0; i < params.tiles; i++) {
            const uint16_t i0 = start_index + (j * (params.tiles + 1)) + i;
            const uint16_t i1 = i0 + 1;
            const uint16_t i2 = i0 + params.tiles + 1;
            const uint16_t i3 = i2 + 1;
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
    const uint16_t start_index = _sshape_base_index(&buf);
    if (!params.merge) {
        _sshape_advance_offset(&buf.vertices);
        _sshape_advance_offset(&buf.indices);
    }

    // build vertices
    uint32_t rand_seed = 0x12345678;
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
        _sshape_vec4_t pos = _sshape_vec4(0.0f, (0==top_bottom) ? y0:y1, 0.0f, 1.0f);
        const _sshape_vec4_t norm = _sshape_vec4(0.0f, (0==top_bottom) ? -1.0f:1.0f, 0.0f, 0.0f);
        const _sshape_vec4_t tnorm = _sshape_vec4_norm(_sshape_mat4_mul(&params.transform, norm));
        for (uint32_t ix = 0; ix <= params.tiles; ix++) {
            pos.x = (0==top_bottom) ? (x0 + dx * ix) : (x1 - dx * ix);
            for (uint32_t iz = 0; iz <= params.tiles; iz++) {
                pos.z = z0 + dz * iz;
                const _sshape_vec4_t tpos = _sshape_mat4_mul(&params.transform, pos);
                const _sshape_vec2_t uv = _sshape_vec2(ix * duv, iz * duv);
                const uint32_t color = params.random_colors ? _sshape_rand_color(&rand_seed) : params.color;
                _sshape_add_vertex(&buf, tpos, tnorm, uv, color);
            }
        }
    }

    // left/right vertices
    for (uint32_t left_right = 0; left_right < 2; left_right++) {
        _sshape_vec4_t pos = _sshape_vec4((0==left_right) ? x0:x1, 0.0f, 0.0f, 1.0f);
        const _sshape_vec4_t norm = _sshape_vec4((0==left_right) ? -1.0f:1.0f, 0.0f, 0.0f, 0.0f);
        const _sshape_vec4_t tnorm = _sshape_vec4_norm(_sshape_mat4_mul(&params.transform, norm));
        for (uint32_t iy = 0; iy <= params.tiles; iy++) {
            pos.y = (0==left_right) ? (y1 - dy * iy) : (y0 + dy * iy);
            for (uint32_t iz = 0; iz <= params.tiles; iz++) {
                pos.z = z0 + dz * iz;
                const _sshape_vec4_t tpos = _sshape_mat4_mul(&params.transform, pos);
                const _sshape_vec2_t uv = _sshape_vec2(iy * duv, iz * duv);
                const uint32_t color = params.random_colors ? _sshape_rand_color(&rand_seed) : params.color;
                _sshape_add_vertex(&buf, tpos, tnorm, uv, color);
            }
        }
    }

    // front/back vertices
    for (uint32_t front_back = 0; front_back < 2; front_back++) {
        _sshape_vec4_t pos = _sshape_vec4(0.0f, 0.0f, (0==front_back) ? z0:z1, 1.0f);
        const _sshape_vec4_t norm = _sshape_vec4(0.0f, 0.0f, (0==front_back) ? -1.0f:1.0f, 0.0f);
        const _sshape_vec4_t tnorm = _sshape_vec4_norm(_sshape_mat4_mul(&params.transform, norm));
        for (uint32_t ix = 0; ix <= params.tiles; ix++) {
            pos.x = (0==front_back) ? (x1 - dx * ix) : (x0 + dx * ix);
            for (uint32_t iy = 0; iy <= params.tiles; iy++) {
                pos.y = y0 + dy * iy;
                const _sshape_vec4_t tpos = _sshape_mat4_mul(&params.transform, pos);
                const _sshape_vec2_t uv = _sshape_vec2(ix * duv, iy * duv);
                const uint32_t color = params.random_colors ? _sshape_rand_color(&rand_seed) : params.color;
                _sshape_add_vertex(&buf, tpos, tnorm, uv, color);
            }
        }
    }

    // build indices
    const uint16_t verts_per_face = (params.tiles + 1) * (params.tiles + 1);
    for (uint16_t face = 0; face < 6; face++) {
        uint16_t face_start_index = start_index + face * verts_per_face;
        for (uint16_t j = 0; j < params.tiles; j++) {
            for (uint16_t i = 0; i < params.tiles; i++) {
                const uint16_t i0 = face_start_index + (j * (params.tiles + 1)) + i;
                const uint16_t i1 = i0 + 1;
                const uint16_t i2 = i0 + params.tiles + 1;
                const uint16_t i3 = i2 + 1;
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
    const uint16_t start_index = _sshape_base_index(&buf);
    if (!params.merge) {
        _sshape_advance_offset(&buf.vertices);
        _sshape_advance_offset(&buf.indices);
    }

    uint32_t rand_seed = 0x12345678;
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
            const _sshape_vec4_t norm = _sshape_vec4(-sin_slice * sin_stack, cos_stack, cos_slice * sin_stack, 0.0f);
            const _sshape_vec4_t pos = _sshape_vec4(norm.x * params.radius, norm.y * params.radius, norm.z * params.radius, 1.0f);
            const _sshape_vec4_t tnorm = _sshape_vec4_norm(_sshape_mat4_mul(&params.transform, norm));
            const _sshape_vec4_t tpos = _sshape_mat4_mul(&params.transform, pos);
            const _sshape_vec2_t uv = _sshape_vec2(1.0f - slice * du, 1.0f - stack * dv);
            const uint32_t color = params.random_colors ? _sshape_rand_color(&rand_seed) : params.color;
            _sshape_add_vertex(&buf, tpos, tnorm, uv, color);
        }
    }

    // generate indices
    {
        // north-pole triangles
        const uint16_t row_a = start_index;
        const uint16_t row_b = row_a + params.slices + 1;
        for (uint16_t slice = 0; slice < params.slices; slice++) {
            _sshape_add_triangle(&buf, row_a + slice, row_b + slice, row_b + slice + 1);
        }
    }
    // stack triangles
    for (uint16_t stack = 1; stack < params.stacks - 1; stack++) {
        const uint16_t row_a = start_index + stack * (params.slices + 1);
        const uint16_t row_b = row_a + params.slices + 1;
        for (uint16_t slice = 0; slice < params.slices; slice++) {
            _sshape_add_triangle(&buf, row_a + slice, row_b + slice + 1, row_a + slice + 1);
            _sshape_add_triangle(&buf, row_a + slice, row_b + slice, row_b + slice + 1);
        }
    }
    {
        // south-pole triangles
        const uint16_t row_a = start_index + (params.stacks - 1) * (params.slices + 1);
        const uint16_t row_b = row_a + params.slices + 1;
        for (uint16_t slice = 0; slice < params.slices; slice++) {
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
static void _sshape_build_cylinder_cap_pole(sshape_buffer_t* buf, const sshape_cylinder_t* params, float pos_y, float norm_y, float du, float v, uint32_t* rand_seed) {
    const _sshape_vec4_t tnorm = _sshape_vec4_norm(_sshape_mat4_mul(&params->transform, _sshape_vec4(0.0f, norm_y, 0.0f, 0.0f)));
    const _sshape_vec4_t tpos = _sshape_mat4_mul(&params->transform, _sshape_vec4(0.0f, pos_y, 0.0f, 1.0f));
    for (uint32_t slice = 0; slice <= params->slices; slice++) {
        const _sshape_vec2_t uv = _sshape_vec2(slice * du, 1.0f - v);
        const uint32_t color = params->random_colors ? _sshape_rand_color(rand_seed) : params->color;
        _sshape_add_vertex(buf, tpos, tnorm, uv, color);
    }
}

static void _sshape_build_cylinder_cap_ring(sshape_buffer_t* buf, const sshape_cylinder_t* params, float pos_y, float norm_y, float du, float v, uint32_t* rand_seed) {
    const float two_pi = 2.0f * 3.14159265358979323846f;
    const _sshape_vec4_t tnorm = _sshape_vec4_norm(_sshape_mat4_mul(&params->transform, _sshape_vec4(0.0f, norm_y, 0.0f, 0.0f)));
    for (uint32_t slice = 0; slice <= params->slices; slice++) {
        const float slice_angle = (two_pi * slice) / params->slices;
        const float sin_slice = sinf(slice_angle);
        const float cos_slice = cosf(slice_angle);
        const _sshape_vec4_t pos = _sshape_vec4(sin_slice * params->radius, pos_y, cos_slice * params->radius, 1.0f);
        const _sshape_vec4_t tpos = _sshape_mat4_mul(&params->transform, pos);
        const _sshape_vec2_t uv = _sshape_vec2(slice * du, 1.0f - v);
        const uint32_t color = params->random_colors ? _sshape_rand_color(rand_seed) : params->color;
        _sshape_add_vertex(buf, tpos, tnorm, uv, color);
    }
}

SOKOL_SHAPE_API_DECL sshape_buffer_t sshape_build_cylinder(const sshape_buffer_t* in_buf, const sshape_cylinder_t* in_params) {
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
    const uint16_t start_index = _sshape_base_index(&buf);
    if (!params.merge) {
        _sshape_advance_offset(&buf.vertices);
        _sshape_advance_offset(&buf.indices);
    }

    uint32_t rand_seed = 0x12345678;
    const float two_pi = 2.0f * 3.14159265358979323846f;
    const float du = 1.0f / params.slices;
    const float dv = 1.0f / (params.stacks + 2);
    const float y0 = params.height * 0.5f;
    const float y1 = -params.height * 0.5f;
    const float dy = params.height / params.stacks;

    // generate vertices
    _sshape_build_cylinder_cap_pole(&buf, &params, y0, 1.0f, du, 0.0f, &rand_seed);
    _sshape_build_cylinder_cap_ring(&buf, &params, y0, 1.0f, du, dv, &rand_seed);
    for (uint32_t stack = 0; stack <= params.stacks; stack++) {
        const float y = y0 - dy * stack;
        const float v = dv * stack + dv;
        for (uint32_t slice = 0; slice <= params.slices; slice++) {
            const float slice_angle = (two_pi * slice) / params.slices;
            const float sin_slice = sinf(slice_angle);
            const float cos_slice = cosf(slice_angle);
            const _sshape_vec4_t pos = _sshape_vec4(sin_slice * params.radius, y, cos_slice * params.radius, 1.0f);
            const _sshape_vec4_t tpos = _sshape_mat4_mul(&params.transform, pos);
            const _sshape_vec4_t norm = _sshape_vec4(sin_slice, 0.0f, cos_slice, 0.0f);
            const _sshape_vec4_t tnorm = _sshape_vec4_norm(_sshape_mat4_mul(&params.transform, norm));
            const _sshape_vec2_t uv = _sshape_vec2(slice * du, 1.0f - v);
            const uint32_t color = params.random_colors ? _sshape_rand_color(&rand_seed) : params.color;
            _sshape_add_vertex(&buf, tpos, tnorm, uv, color);
        }
    }
    _sshape_build_cylinder_cap_ring(&buf, &params, y1, -1.0f, du, 1.0f - dv, &rand_seed);
    _sshape_build_cylinder_cap_pole(&buf, &params, y1, -1.0f, du, 1.0f, &rand_seed);

    // generate indices
    {
        // top-cap indices
        const uint16_t row_a = start_index;
        const uint16_t row_b = row_a + params.slices + 1;
        for (uint16_t slice = 0; slice < params.slices; slice++) {
            _sshape_add_triangle(&buf, row_a + slice, row_b + slice + 1, row_b + slice);
        }
    }
    // shaft triangles
    for (uint16_t stack = 0; stack < params.stacks; stack++) {
        const uint16_t row_a = start_index + (stack + 2) * (params.slices + 1);
        const uint16_t row_b = row_a + params.slices + 1;
        for (uint16_t slice = 0; slice < params.slices; slice++) {
            _sshape_add_triangle(&buf, row_a + slice, row_a + slice + 1, row_b + slice + 1);
            _sshape_add_triangle(&buf, row_a + slice, row_b + slice + 1, row_b + slice);
        }
    }
    {
        // bottom-cap indices
        const uint16_t row_a = start_index + (params.stacks + 3) * (params.slices + 1);
        const uint16_t row_b = row_a + params.slices + 1;
        for (uint16_t slice = 0; slice < params.slices; slice++) {
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
    const uint16_t start_index = _sshape_base_index(&buf);
    if (!params.merge) {
        _sshape_advance_offset(&buf.vertices);
        _sshape_advance_offset(&buf.indices);
    }

    uint32_t rand_seed = 0x12345678;
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
            const float spx = sin_theta * (params.radius - (params.ring_radius * cos_phi));
            const float spy = sin_phi * params.ring_radius;
            const float spz = cos_theta * (params.radius - (params.ring_radius * cos_phi));

            // torus position with ring-radius zero (for normal computation)
            const float ipx = sin_theta * params.radius;
            const float ipy = 0.0f;
            const float ipz = cos_theta * params.radius;

            const _sshape_vec4_t pos = _sshape_vec4(spx, spy, spz, 1.0f);
            const _sshape_vec4_t norm = _sshape_vec4(spx - ipx, spy - ipy, spz - ipz, 0.0f);
            const _sshape_vec4_t tpos = _sshape_mat4_mul(&params.transform, pos);
            const _sshape_vec4_t tnorm = _sshape_vec4_norm(_sshape_mat4_mul(&params.transform, norm));
            const _sshape_vec2_t uv = _sshape_vec2(ring * du, 1.0f - side * dv);
            const uint32_t color = params.random_colors ? _sshape_rand_color(&rand_seed) : params.color;
            _sshape_add_vertex(&buf, tpos, tnorm, uv, color);
        }
    }

    // generate indices
    for (uint16_t side = 0; side < params.sides; side++) {
        const uint16_t row_a = start_index + side * (params.rings + 1);
        const uint16_t row_b = row_a + params.rings + 1;
        for (uint16_t ring = 0; ring < params.rings; ring++) {
            _sshape_add_triangle(&buf, row_a + ring, row_a + ring + 1, row_b + ring + 1);
            _sshape_add_triangle(&buf, row_a + ring, row_b + ring + 1, row_b + ring);
        }
    }
    return buf;
}

SOKOL_API_IMPL sg_buffer_desc sshape_vertex_buffer_desc(const sshape_buffer_t* buf) {
    SOKOL_ASSERT(buf && buf->valid);
    sg_buffer_desc desc = { 0 };
    if (buf->valid) {
        desc.type = SG_BUFFERTYPE_VERTEXBUFFER;
        desc.usage = SG_USAGE_IMMUTABLE;
        desc.data.ptr = buf->vertices.buffer.ptr;
        desc.data.size = buf->vertices.data_size;
    }
    return desc;
}

SOKOL_API_IMPL sg_buffer_desc sshape_index_buffer_desc(const sshape_buffer_t* buf) {
    SOKOL_ASSERT(buf && buf->valid);
    sg_buffer_desc desc = { 0 };
    if (buf->valid) {
        desc.type = SG_BUFFERTYPE_INDEXBUFFER;
        desc.usage = SG_USAGE_IMMUTABLE;
        desc.data.ptr = buf->indices.buffer.ptr;
        desc.data.size = buf->indices.data_size;
    }
    return desc;
}

SOKOL_SHAPE_API_DECL sshape_element_range_t sshape_element_range(const sshape_buffer_t* buf) {
    SOKOL_ASSERT(buf && buf->valid);
    SOKOL_ASSERT(buf->indices.shape_offset < buf->indices.data_size);
    SOKOL_ASSERT(0 == (buf->indices.shape_offset & (sizeof(uint16_t) - 1)));
    SOKOL_ASSERT(0 == (buf->indices.data_size & (sizeof(uint16_t) - 1)));
    sshape_element_range_t range = { 0 };
    range.base_element = (int) (buf->indices.shape_offset / sizeof(uint16_t));
    if (buf->valid) {
        range.num_elements = (int) ((buf->indices.data_size - buf->indices.shape_offset) / sizeof(uint16_t));
    }
    else {
        range.num_elements = 0;
    }
    return range;
}

SOKOL_API_IMPL sg_buffer_layout_desc sshape_buffer_layout_desc(void) {
    sg_buffer_layout_desc desc = { 0 };
    desc.stride = sizeof(sshape_vertex_t);
    return desc;
}

SOKOL_API_IMPL sg_vertex_attr_desc sshape_position_attr_desc(void) {
    sg_vertex_attr_desc desc = { 0 };
    desc.offset = offsetof(sshape_vertex_t, x);
    desc.format = SG_VERTEXFORMAT_FLOAT3;
    return desc;
}

SOKOL_API_IMPL sg_vertex_attr_desc sshape_normal_attr_desc(void) {
    sg_vertex_attr_desc desc = { 0 };
    desc.offset = offsetof(sshape_vertex_t, normal);
    desc.format = SG_VERTEXFORMAT_BYTE4N;
    return desc;
}

SOKOL_API_IMPL sg_vertex_attr_desc sshape_texcoord_attr_desc(void) {
    sg_vertex_attr_desc desc = { 0 };
    desc.offset = offsetof(sshape_vertex_t, u);
    desc.format = SG_VERTEXFORMAT_USHORT2N;
    return desc;
}

SOKOL_API_IMPL sg_vertex_attr_desc sshape_color_attr_desc(void) {
    sg_vertex_attr_desc desc = { 0 };
    desc.offset = offsetof(sshape_vertex_t, color);
    desc.format = SG_VERTEXFORMAT_UBYTE4N;
    return desc;
}

#ifdef __clang__
#pragma clang diagnostic pop
#endif
#endif // SOKOL_SHAPE_IMPL
