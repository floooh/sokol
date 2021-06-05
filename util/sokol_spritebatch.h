#if defined(SOKOL_IMPL) && !defined(SOKOL_SPRITEBATCH_IMPL)
#define SOKOL_SPRITEBATCH_IMPL
#endif

#ifndef SOKOL_SPRITEBATCH_INCLUDED
#define SOKOL_SPRITEBATCH_INCLUDED (1)

#if !defined(SOKOL_GFX_INCLUDED)
#error "Please include sokol_gfx.h before sokol_spritebatch.h"
#endif

#if defined(SOKOL_API_DECL) && !defined(SOKOL_SPRITEBATCH_API_DECL)
#define SOKOL_SPRITEBATCH_API_DECL SOKOL_API_DECL
#endif
#ifndef SOKOL_SPRITEBATCH_API_DECL
#if defined(_WIN32) && defined(SOKOL_DLL) && defined(SOKOL_SPRITEBATCH_IMPL)
#define SOKOL_SPRITEBATCH_API_DECL __declspec(dllexport)
#elif defined(_WIN32) && defined(SOKOL_DLL)
#define SOKOL_SPRITEBATCH_API_DECL __declspec(dllimport)
#else
#define SOKOL_SPRITEBATCH_API_DECL extern
#endif
#endif

typedef enum sb_sprite_flags {
    SB_FLIP_NONE = 0,
    SB_FLIP_X    = 1 << 0,
    SB_FLIP_Y    = 1 << 1,
    SB_FLIP_BOTH = SB_FLIP_Y | SB_FLIP_X,
    SB_Z_TILT    = 1<< 2
} sb_sprite_flags;

typedef enum sb_sort_mode {
    SB_SORT_MODE_DEFERRED,
    SB_SORT_MODE_TEXTURE,
    SB_SORT_MODE_BACK_TO_FRONT,
    SB_SORT_MODE_FRONT_TO_BACK
} sb_sort_mode;

typedef struct sb_float2 {
    float x;
    float y;
} sb_float2;

typedef struct sb_sprite_info {
    sg_image  image;
    float     width;
    float     height;
    sb_float2 position;
    float     depth;
    float     rotation;
    sb_float2 origin;
    sb_float2 scale;
    uint32_t  flags;
    sb_float2 source;
    sg_color  color;
} sb_sprite_info;

typedef struct sb_desc {
    int max_quads;
} sb_desc;

typedef struct sb_matrix {
  float m[4][4];
} sb_matrix;

typedef struct sb_viewport {
    int x, y, width, height;
    bool origin_top_left;
} sb_viewport;

typedef struct sb_render_state {
    sb_sort_mode sort_mode;
    sg_pipeline  pipeline;
    sb_matrix    transform_matrix;
    sb_viewport  viewport;
} sb_render_state;

SOKOL_SPRITEBATCH_API_DECL void sb_setup(const sb_desc* desc);
SOKOL_SPRITEBATCH_API_DECL void sb_shutdown(void);

SOKOL_SPRITEBATCH_API_DECL void sb_begin(const sb_render_state* render_state);
SOKOL_SPRITEBATCH_API_DECL void sb_sprite(const sb_sprite_info* sprite);
SOKOL_SPRITEBATCH_API_DECL void sb_end(void);

SOKOL_SPRITEBATCH_API_DECL void sb_draw(void);

SOKOL_SPRITEBATCH_API_DECL void sb_premultiply_alpha(uint8_t* pixels, int pixel_count);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* SOKOL_SPRITEBATCH_INCLUDED */

/*-- IMPLEMENTATION ----------------------------------------------------------*/
#ifdef SOKOL_SPRITEBATCH_IMPL
#define SOKOL_SPRITEBATCH_IMPL_INCLUDED (1)

#include <string.h> /* memset */
#include <math.h>   /* sinf, cosf */

#ifndef SOKOL_API_IMPL
    #define SOKOL_API_IMPL
#endif

#ifndef SOKOL_ASSERT
    #include <assert.h>
    #define SOKOL_ASSERT(c) assert(c)
#endif

#ifndef SOKOL_MALLOC
    #include <stdlib.h>
    #define SOKOL_MALLOC(s) malloc(s)
    #define SOKOL_FREE(result) free(result)
#endif

#define _SB_IMAGE_SLOT_MASK (0xFFFF)
#define _SB_DEFAULT(val, def) (((val) == 0) ? (def) : (val))
#define _SB_MAX_VERTICES (1 << 16)
#define _SB_MAX_QUADS (_SB_MAX_VERTICES / 4)
#define _SB_MAX_INDICES (_SB_MAX_QUADS * 6)
#define _SB_INITIAL_BATCH_CAPACITY 32

#ifndef SB_MAX_DEPTH
#define SB_MAX_DEPTH 1000.0f
#endif

typedef struct {
    sg_image image;
    int      width;
    int      height;
} _sb_sprite_data;

typedef struct {
    float    x;
    float    y;
    float    z;
    float    u;
    float    v;
    uint32_t rgba;
} _sb_vertex;

typedef struct {
    _sb_vertex top_left;
    _sb_vertex top_right;
    _sb_vertex bottom_left;
    _sb_vertex bottom_right;
    sg_image   image;
    uint64_t   sort_key;
} _sb_quad;

typedef struct {
    _sb_sprite_data* data;
    size_t           size;
} _sb_sprite_pool;

typedef struct {
    sg_image  image;
    int       base_element;
    int       num_elements;
    sb_matrix matrix;
} _sb_batch;

typedef struct {
    size_t     batch_size;
    size_t     batch_capacity;
    _sb_batch* batches;
} _sb_batch_data;

typedef struct {
    _sb_sprite_pool sprite_pool;
    _sb_quad*       quads;
    size_t          quad_count;
    _sb_vertex*     vertex_buffer_data;
    sg_buffer       vertex_buffer;
    sg_buffer       index_buffer;
    _sb_batch_data  batch_data;
    sg_bindings     bindings;
    bool            begin_called;
    sb_render_state render_state;
    sg_shader       default_shader;
    sg_pipeline     default_pipeline;
    sb_matrix       projection_matrix;
} _sb_t;

static _sb_t _sb;

#if defined(SOKOL_D3D11)
static const uint8_t vs_bytecode_hlsl4[884] = {
    0x44,0x58,0x42,0x43,0x5f,0x8c,0xaf,0xe1,0x5e,0x2d,0xba,0x0e,0x85,0xba,0xeb,0xc5,
    0x0c,0x64,0x6d,0x0c,0x01,0x00,0x00,0x00,0x74,0x03,0x00,0x00,0x05,0x00,0x00,0x00,
    0x34,0x00,0x00,0x00,0xf4,0x00,0x00,0x00,0x58,0x01,0x00,0x00,0xc8,0x01,0x00,0x00,
    0xf8,0x02,0x00,0x00,0x52,0x44,0x45,0x46,0xb8,0x00,0x00,0x00,0x01,0x00,0x00,0x00,
    0x48,0x00,0x00,0x00,0x01,0x00,0x00,0x00,0x1c,0x00,0x00,0x00,0x00,0x04,0xfe,0xff,
    0x10,0x81,0x00,0x00,0x90,0x00,0x00,0x00,0x3c,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
    0x01,0x00,0x00,0x00,0x01,0x00,0x00,0x00,0x76,0x73,0x5f,0x70,0x61,0x72,0x61,0x6d,
    0x73,0x00,0xab,0xab,0x3c,0x00,0x00,0x00,0x01,0x00,0x00,0x00,0x60,0x00,0x00,0x00,
    0x40,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x78,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,0x40,0x00,0x00,0x00,0x02,0x00,0x00,0x00,0x80,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,0x5f,0x32,0x31,0x5f,0x6d,0x76,0x70,0x00,0x02,0x00,0x03,0x00,
    0x04,0x00,0x04,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x4d,0x69,0x63,0x72,
    0x6f,0x73,0x6f,0x66,0x74,0x20,0x28,0x52,0x29,0x20,0x48,0x4c,0x53,0x4c,0x20,0x53,
    0x68,0x61,0x64,0x65,0x72,0x20,0x43,0x6f,0x6d,0x70,0x69,0x6c,0x65,0x72,0x20,0x31,
    0x30,0x2e,0x31,0x00,0x49,0x53,0x47,0x4e,0x5c,0x00,0x00,0x00,0x03,0x00,0x00,0x00,
    0x08,0x00,0x00,0x00,0x50,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
    0x03,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x07,0x07,0x00,0x00,0x50,0x00,0x00,0x00,
    0x01,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x03,0x00,0x00,0x00,0x01,0x00,0x00,0x00,
    0x03,0x03,0x00,0x00,0x50,0x00,0x00,0x00,0x02,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
    0x03,0x00,0x00,0x00,0x02,0x00,0x00,0x00,0x0f,0x0f,0x00,0x00,0x54,0x45,0x58,0x43,
    0x4f,0x4f,0x52,0x44,0x00,0xab,0xab,0xab,0x4f,0x53,0x47,0x4e,0x68,0x00,0x00,0x00,
    0x03,0x00,0x00,0x00,0x08,0x00,0x00,0x00,0x50,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,0x03,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x03,0x0c,0x00,0x00,
    0x50,0x00,0x00,0x00,0x01,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x03,0x00,0x00,0x00,
    0x01,0x00,0x00,0x00,0x0f,0x00,0x00,0x00,0x59,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
    0x01,0x00,0x00,0x00,0x03,0x00,0x00,0x00,0x02,0x00,0x00,0x00,0x0f,0x00,0x00,0x00,
    0x54,0x45,0x58,0x43,0x4f,0x4f,0x52,0x44,0x00,0x53,0x56,0x5f,0x50,0x6f,0x73,0x69,
    0x74,0x69,0x6f,0x6e,0x00,0xab,0xab,0xab,0x53,0x48,0x44,0x52,0x28,0x01,0x00,0x00,
    0x40,0x00,0x01,0x00,0x4a,0x00,0x00,0x00,0x59,0x00,0x00,0x04,0x46,0x8e,0x20,0x00,
    0x00,0x00,0x00,0x00,0x04,0x00,0x00,0x00,0x5f,0x00,0x00,0x03,0x72,0x10,0x10,0x00,
    0x00,0x00,0x00,0x00,0x5f,0x00,0x00,0x03,0x32,0x10,0x10,0x00,0x01,0x00,0x00,0x00,
    0x5f,0x00,0x00,0x03,0xf2,0x10,0x10,0x00,0x02,0x00,0x00,0x00,0x65,0x00,0x00,0x03,
    0x32,0x20,0x10,0x00,0x00,0x00,0x00,0x00,0x65,0x00,0x00,0x03,0xf2,0x20,0x10,0x00,
    0x01,0x00,0x00,0x00,0x67,0x00,0x00,0x04,0xf2,0x20,0x10,0x00,0x02,0x00,0x00,0x00,
    0x01,0x00,0x00,0x00,0x68,0x00,0x00,0x02,0x01,0x00,0x00,0x00,0x36,0x00,0x00,0x05,
    0x32,0x20,0x10,0x00,0x00,0x00,0x00,0x00,0x46,0x10,0x10,0x00,0x01,0x00,0x00,0x00,
    0x36,0x00,0x00,0x05,0xf2,0x20,0x10,0x00,0x01,0x00,0x00,0x00,0x46,0x1e,0x10,0x00,
    0x02,0x00,0x00,0x00,0x38,0x00,0x00,0x08,0xf2,0x00,0x10,0x00,0x00,0x00,0x00,0x00,
    0x56,0x15,0x10,0x00,0x00,0x00,0x00,0x00,0x46,0x8e,0x20,0x00,0x00,0x00,0x00,0x00,
    0x01,0x00,0x00,0x00,0x32,0x00,0x00,0x0a,0xf2,0x00,0x10,0x00,0x00,0x00,0x00,0x00,
    0x06,0x10,0x10,0x00,0x00,0x00,0x00,0x00,0x46,0x8e,0x20,0x00,0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,0x46,0x0e,0x10,0x00,0x00,0x00,0x00,0x00,0x32,0x00,0x00,0x0a,
    0xf2,0x00,0x10,0x00,0x00,0x00,0x00,0x00,0xa6,0x1a,0x10,0x00,0x00,0x00,0x00,0x00,
    0x46,0x8e,0x20,0x00,0x00,0x00,0x00,0x00,0x02,0x00,0x00,0x00,0x46,0x0e,0x10,0x00,
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x08,0xf2,0x20,0x10,0x00,0x02,0x00,0x00,0x00,
    0x46,0x0e,0x10,0x00,0x00,0x00,0x00,0x00,0x46,0x8e,0x20,0x00,0x00,0x00,0x00,0x00,
    0x03,0x00,0x00,0x00,0x3e,0x00,0x00,0x01,0x53,0x54,0x41,0x54,0x74,0x00,0x00,0x00,
    0x07,0x00,0x00,0x00,0x01,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x06,0x00,0x00,0x00,
    0x04,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x01,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x02,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,
};

static const uint8_t fs_bytecode_hlsl4[620] = {
    0x44,0x58,0x42,0x43,0xd1,0x93,0x1f,0x1b,0x9d,0x70,0x90,0xeb,0xc2,0x7c,0x26,0x07,
    0xdf,0x52,0xda,0x49,0x01,0x00,0x00,0x00,0x6c,0x02,0x00,0x00,0x05,0x00,0x00,0x00,
    0x34,0x00,0x00,0x00,0xd4,0x00,0x00,0x00,0x20,0x01,0x00,0x00,0x54,0x01,0x00,0x00,
    0xf0,0x01,0x00,0x00,0x52,0x44,0x45,0x46,0x98,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,0x02,0x00,0x00,0x00,0x1c,0x00,0x00,0x00,0x00,0x04,0xff,0xff,
    0x10,0x81,0x00,0x00,0x6d,0x00,0x00,0x00,0x5c,0x00,0x00,0x00,0x03,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
    0x01,0x00,0x00,0x00,0x01,0x00,0x00,0x00,0x69,0x00,0x00,0x00,0x02,0x00,0x00,0x00,
    0x05,0x00,0x00,0x00,0x04,0x00,0x00,0x00,0xff,0xff,0xff,0xff,0x00,0x00,0x00,0x00,
    0x01,0x00,0x00,0x00,0x0d,0x00,0x00,0x00,0x5f,0x74,0x65,0x78,0x5f,0x73,0x61,0x6d,
    0x70,0x6c,0x65,0x72,0x00,0x74,0x65,0x78,0x00,0x4d,0x69,0x63,0x72,0x6f,0x73,0x6f,
    0x66,0x74,0x20,0x28,0x52,0x29,0x20,0x48,0x4c,0x53,0x4c,0x20,0x53,0x68,0x61,0x64,
    0x65,0x72,0x20,0x43,0x6f,0x6d,0x70,0x69,0x6c,0x65,0x72,0x20,0x31,0x30,0x2e,0x31,
    0x00,0xab,0xab,0xab,0x49,0x53,0x47,0x4e,0x44,0x00,0x00,0x00,0x02,0x00,0x00,0x00,
    0x08,0x00,0x00,0x00,0x38,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
    0x03,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x03,0x03,0x00,0x00,0x38,0x00,0x00,0x00,
    0x01,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x03,0x00,0x00,0x00,0x01,0x00,0x00,0x00,
    0x0f,0x0f,0x00,0x00,0x54,0x45,0x58,0x43,0x4f,0x4f,0x52,0x44,0x00,0xab,0xab,0xab,
    0x4f,0x53,0x47,0x4e,0x2c,0x00,0x00,0x00,0x01,0x00,0x00,0x00,0x08,0x00,0x00,0x00,
    0x20,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x03,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,0x0f,0x00,0x00,0x00,0x53,0x56,0x5f,0x54,0x61,0x72,0x67,0x65,
    0x74,0x00,0xab,0xab,0x53,0x48,0x44,0x52,0x94,0x00,0x00,0x00,0x40,0x00,0x00,0x00,
    0x25,0x00,0x00,0x00,0x5a,0x00,0x00,0x03,0x00,0x60,0x10,0x00,0x00,0x00,0x00,0x00,
    0x58,0x18,0x00,0x04,0x00,0x70,0x10,0x00,0x00,0x00,0x00,0x00,0x55,0x55,0x00,0x00,
    0x62,0x10,0x00,0x03,0x32,0x10,0x10,0x00,0x00,0x00,0x00,0x00,0x62,0x10,0x00,0x03,
    0xf2,0x10,0x10,0x00,0x01,0x00,0x00,0x00,0x65,0x00,0x00,0x03,0xf2,0x20,0x10,0x00,
    0x00,0x00,0x00,0x00,0x68,0x00,0x00,0x02,0x01,0x00,0x00,0x00,0x45,0x00,0x00,0x09,
    0xf2,0x00,0x10,0x00,0x00,0x00,0x00,0x00,0x46,0x10,0x10,0x00,0x00,0x00,0x00,0x00,
    0x46,0x7e,0x10,0x00,0x00,0x00,0x00,0x00,0x00,0x60,0x10,0x00,0x00,0x00,0x00,0x00,
    0x38,0x00,0x00,0x07,0xf2,0x20,0x10,0x00,0x00,0x00,0x00,0x00,0x46,0x0e,0x10,0x00,
    0x00,0x00,0x00,0x00,0x46,0x1e,0x10,0x00,0x01,0x00,0x00,0x00,0x3e,0x00,0x00,0x01,
    0x53,0x54,0x41,0x54,0x74,0x00,0x00,0x00,0x03,0x00,0x00,0x00,0x01,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,0x03,0x00,0x00,0x00,0x01,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,0x01,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
    0x01,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
};
#if !defined(SOKOL_GFX_INCLUDED)
#error "Please include sokol_gfx.h before spritebatch-sapp.glsl.h"
#endif

static inline const sg_shader_desc* spritebatch_shader_desc(sg_backend backend) {
    if (backend == SG_BACKEND_D3D11) {
        static sg_shader_desc desc;
        static bool valid;
        if (!valid) {
            valid = true;
            desc.attrs[0].sem_name = "TEXCOORD";
            desc.attrs[0].sem_index = 0;
            desc.attrs[1].sem_name = "TEXCOORD";
            desc.attrs[1].sem_index = 1;
            desc.attrs[2].sem_name = "TEXCOORD";
            desc.attrs[2].sem_index = 2;
            desc.vs.bytecode.ptr = vs_bytecode_hlsl4;
            desc.vs.bytecode.size = 884;
            desc.vs.entry = "main";
            desc.vs.uniform_blocks[0].size = 64;
            desc.fs.bytecode.ptr = fs_bytecode_hlsl4;
            desc.fs.bytecode.size = 620;
            desc.fs.entry = "main";
            desc.fs.images[0].name = "tex";
            desc.fs.images[0].image_type = SG_IMAGETYPE_2D;
            desc.fs.images[0].sampler_type = SG_SAMPLERTYPE_FLOAT;
            desc.label = "spritebatch_shader";
        };
        return &desc;
    }
    return 0;
}

#endif

static inline bool _sb_matrix_is_null(const sb_matrix* m) {
    for (int y = 0; y < 4; y++) {
        for (int x = 0; x < 4; x++) {
            if (0.0f != m->m[y][x]) {
                return false;
            }
        }
    }
    return true;
}

static inline sb_matrix _sb_matrix_identity(void) {
    sb_matrix m = {
        {
            { 1.0f, 0.0f, 0.0f, 0.0f },
            { 0.0f, 1.0f, 0.0f, 0.0f },
            { 0.0f, 0.0f, 1.0f, 0.0f },
            { 0.0f, 0.0f, 0.0f, 1.0f }
        }
    };
    return m;
}

static inline sb_matrix _sb_orthographic_off_center(float left, float right, float bottom, float top, float near, float far) {
    sb_matrix result;

    result.m[0][0] = 2.0f / (right - left);
    result.m[0][1] = 0.0f;
    result.m[0][2] = 0.0f;
    result.m[0][3] = 0.0f;

    result.m[1][0] = 0.0f;
    result.m[1][1] = 2.0f / (top - bottom);
    result.m[1][2] = 0.0f;
    result.m[1][3] = 0.0f;

    result.m[2][0] = 0.0f;
    result.m[2][1] = 0.0f;
    result.m[2][2] = 1.0f / (near - far);
    result.m[2][3] = 0.0f;

    result.m[3][0] = (left + right) / (left - right);
    result.m[3][1] = (bottom + top) / (bottom - top);
    result.m[3][2] = near / (near - far);
    result.m[3][3] = 1.0f;

    return result;
}

static inline int _sg_image_slot_index(uint32_t id) {
    int slot_index = (int)(id & _SB_IMAGE_SLOT_MASK);
    SOKOL_ASSERT(0 != slot_index);
    return slot_index;
}

static inline void _sb_init_sprite_pool(void) {
    sg_desc sg_desc = sg_query_desc();
    int num_slots = sg_desc.image_pool_size;
    _sb.sprite_pool.size = (size_t)num_slots * sizeof(_sb_sprite_data);
    _sb.sprite_pool.data = (_sb_sprite_data*)SOKOL_MALLOC(_sb.sprite_pool.size);
    SOKOL_ASSERT(_sb.sprite_pool.data);
    memset(_sb.sprite_pool.data, 0, _sb.sprite_pool.size);
}

static inline void _sb_init_batch_data(void) {
    _sb.batch_data.batch_capacity = _SB_INITIAL_BATCH_CAPACITY;
    _sb.batch_data.batches = (_sb_batch*)SOKOL_MALLOC(_SB_INITIAL_BATCH_CAPACITY * sizeof(_sb_batch));
}

static inline void _sb_init_index_buffer(void) {
    uint16_t* index_buffer = (uint16_t*)SOKOL_MALLOC(_SB_MAX_INDICES * sizeof(uint16_t));
    SOKOL_ASSERT(index_buffer);

    uint16_t* index_ptr = index_buffer;
    for (uint32_t i = 0; i < _SB_MAX_QUADS; i++, index_ptr += 6) {
        // Triangle 1
        *(index_ptr + 0) = (uint16_t)(i * 4);
        *(index_ptr + 1) = (uint16_t)(i * 4 + 1);
        *(index_ptr + 2) = (uint16_t)(i * 4 + 2);
        // Triangle 2
        *(index_ptr + 3) = (uint16_t)(i * 4 + 1);
        *(index_ptr + 4) = (uint16_t)(i * 4 + 3);
        *(index_ptr + 5) = (uint16_t)(i * 4 + 2);
    }

    sg_buffer_desc index_buffer_desc;
    memset(&index_buffer_desc, 0, sizeof(index_buffer_desc));
    index_buffer_desc.size      = _SB_MAX_INDICES * sizeof(uint16_t);
    index_buffer_desc.type      = SG_BUFFERTYPE_INDEXBUFFER;
    index_buffer_desc.usage     = SG_USAGE_IMMUTABLE;
    index_buffer_desc.label     = "sokol-spritebatch-indices";
    index_buffer_desc.data.size = _SB_MAX_INDICES * sizeof(uint16_t);
    index_buffer_desc.data.ptr  = index_buffer;
    _sb.index_buffer = sg_make_buffer(&index_buffer_desc);
    SOKOL_ASSERT(SG_INVALID_ID != _sb.index_buffer.id);

    SOKOL_FREE(index_buffer);

    _sb.bindings.index_buffer = _sb.index_buffer;
}

static inline void _sb_init_quad_rotated(
    _sb_quad* quad, float x, float y, float dx, float dy,
    float w, float h, float sin, float cos, uint32_t rgba,
    sb_float2 top_left, sb_float2 bottom_right, float depth) {

    /* TODO: Z-tilt: what the hell do we do? */

    quad->top_left.x    = x + dx * cos - dy * sin;
    quad->top_left.y    = y + dx * sin + dy * cos;
    quad->top_left.z    = depth;
    quad->top_left.rgba = rgba;
    quad->top_left.u    = top_left.x;
    quad->top_left.v    = top_left.y;

    quad->top_right.x    = x + (dx + w) * cos - dy * sin;
    quad->top_right.y    = y + (dx + w) * sin + dy * cos;
    quad->top_right.z    = depth;
    quad->top_right.rgba = rgba;
    quad->top_right.u    = bottom_right.x;
    quad->top_right.v    = top_left.y;

    quad->bottom_left.x    = x + dx * cos - (dy + h) * sin;
    quad->bottom_left.y    = y + dx * sin + (dy + h) * cos;
    quad->bottom_left.z    = depth;
    quad->bottom_left.rgba = rgba;
    quad->bottom_left.u    = top_left.x;
    quad->bottom_left.v    = bottom_right.y;

    quad->bottom_right.x    = x + (dx + w) * cos - (dy + h) * sin;
    quad->bottom_right.y    = y + (dx + w) * sin + (dy + h) * cos;
    quad->bottom_right.z    = depth;
    quad->bottom_right.rgba = rgba;
    quad->bottom_right.u    = bottom_right.x;
    quad->bottom_right.v    = bottom_right.y;
}

static inline void _sb_init_quad(
    _sb_quad* quad, uint32_t flags, float x, float y, float w, float h, uint32_t rgba,
    sb_float2 top_left, sb_float2 bottom_right, float depth) {

    quad->top_left.x = x;
    quad->top_left.y = y;
    quad->top_left.z = depth;
    quad->top_left.rgba = rgba;
    quad->top_left.u = top_left.x;
    quad->top_left.v = top_left.y;

    quad->top_right.x = x + w;
    quad->top_right.y = y;
    quad->top_right.z = depth;
    quad->top_right.rgba = rgba;
    quad->top_right.u = bottom_right.x;
    quad->top_right.v = top_left.y;

    if ((flags & SB_Z_TILT) != SB_FLIP_NONE) {
        // move the topmost vertices further out to enable z-tilting
        const float angle = 0.785398f; // 45 degrees
        const float depth = h * tanf(angle);
        quad->top_left.z  -= depth;
        quad->top_right.z -= depth;
    }

    quad->bottom_left.x    = x;
    quad->bottom_left.y    = y + h;
    quad->bottom_left.z    = depth;
    quad->bottom_left.rgba = rgba;
    quad->bottom_left.u    = top_left.x;
    quad->bottom_left.v    = bottom_right.y;

    quad->bottom_right.x    = x + w;
    quad->bottom_right.y    = y + h;
    quad->bottom_right.z    = depth;
    quad->bottom_right.rgba = rgba;
    quad->bottom_right.u    = bottom_right.x;
    quad->bottom_right.v    = bottom_right.y;
}

static inline void _sb_init_vertex_buffer(void) {
    sg_buffer_desc vertex_buffer_desc;
    memset(&vertex_buffer_desc, 0, sizeof(vertex_buffer_desc));
    vertex_buffer_desc.size = _SB_MAX_VERTICES * sizeof(_sb_vertex);
    vertex_buffer_desc.type = SG_BUFFERTYPE_VERTEXBUFFER;
    vertex_buffer_desc.usage = SG_USAGE_STREAM;
    vertex_buffer_desc.label = "sokol-spritebatch-vertices";
    _sb.vertex_buffer = sg_make_buffer(&vertex_buffer_desc);
    SOKOL_ASSERT(SG_INVALID_ID != _sb.vertex_buffer.id);

    _sb.vertex_buffer_data = (_sb_vertex*)SOKOL_MALLOC(_SB_MAX_VERTICES * sizeof(_sb_vertex));
    SOKOL_ASSERT(_sb.vertex_buffer_data);

    _sb.bindings.vertex_buffers[0] = _sb.vertex_buffer;
}

static inline uint32_t _sb_float_flip(uint32_t f) {
    uint32_t mask = -(int32_t)(f >> 31) | 0x80000000;
    return f ^ mask;
}

static inline uint32_t _sb_depth_to_bits(float value) {
    // https://aras-p.info/blog/2014/01/16/rough-sorting-by-depth/
    // Taking highest 10 bits for rough sort of positive floats.
    // Sign is always zero, so only 9 bits in the result are used.
    // 0.01 maps to 240; 0.1 to 247; 1.0 to 254; 10.0 to 260;
    // 100.0 to 267; 1000.0 to 273 etc.
    uint32_t i;
    memcpy(&i, &value, sizeof(value));
    i = _sb_float_flip(i);
    return i >> 22; // take highest 10 bits
}

static inline uint64_t _sb_make_sort_key(const sb_sprite_info* sprite) {
    switch (_sb.render_state.sort_mode) {
    case SB_SORT_MODE_TEXTURE: {
        return (uint64_t)sprite->image.id;
    }
    case SB_SORT_MODE_BACK_TO_FRONT: {
        return (uint64_t)_sb_depth_to_bits(-sprite->depth) << 32 | sprite->image.id;
    }
    case SB_SORT_MODE_FRONT_TO_BACK: {
        return (uint64_t)_sb_depth_to_bits(sprite->depth) << 32 | sprite->image.id;
    }
    }
    return 0;
}

static inline uint32_t _sb_pack_color_bytes(uint8_t r, uint8_t g, uint8_t b, uint8_t a) {
    return (uint32_t)(((uint32_t)a << 24) | ((uint32_t)b << 16) | ((uint32_t)g << 8) | r);
}

static inline float _sb_clampf(float v, float low, float high) {
    if (v < low) {
        return low;
    }
    else if (v > high) {
        return high;
    }
    return v;
}

static inline uint32_t _sb_pack_color(const sg_color* color) {
    const uint8_t r = (uint8_t)(_sb_clampf(color->r, 0.0f, 1.0f) * 255.0f);
    const uint8_t g = (uint8_t)(_sb_clampf(color->g, 0.0f, 1.0f) * 255.0f);
    const uint8_t b = (uint8_t)(_sb_clampf(color->b, 0.0f, 1.0f) * 255.0f);
    const uint8_t a = (uint8_t)(_sb_clampf(color->a, 0.0f, 1.0f) * 255.0f);
    return _sb_pack_color_bytes(r, g, b, a);
}

static inline int _sb_quad_compare(const void* a, const void* b) {
    uint64_t key_a = ((const _sb_quad*)a)->sort_key;
    uint64_t key_b = ((const _sb_quad*)b)->sort_key;
    if (key_a < key_b) return -1;
    if (key_a > key_b) return  1;
    return 0;
}

static inline void* _sb_realloc(void* old_ptr, size_t old_size, size_t new_size) {
    SOKOL_ASSERT((new_size > 0) && (new_size > old_size));
    void* new_ptr = SOKOL_MALLOC(new_size);
    SOKOL_ASSERT(new_ptr);
    if (old_ptr) {
        if (old_size > 0) {
            memcpy(new_ptr, old_ptr, old_size);
        }
        SOKOL_FREE(old_ptr);
    }
    return new_ptr;
}

static inline void _sb_grow_batch_buffer(void) {
    const size_t new_capacity = _sb.batch_data.batch_capacity * 2;
    _sb.batch_data.batches = (_sb_batch*)_sb_realloc(_sb.batch_data.batches, _sb.batch_data.batch_capacity, new_capacity);
    _sb.batch_data.batch_capacity = new_capacity;
}

static inline _sb_batch* _sb_create_batch(void) {
    if (_sb.batch_data.batch_size >= _sb.batch_data.batch_capacity) {
        _sb_grow_batch_buffer();
    }
    return &_sb.batch_data.batches[_sb.batch_data.batch_size++];
}

static inline void _sb_init_batch(sg_image image, int num_elements, int base_element) {
    _sb_batch* batch    = _sb_create_batch();
    batch->image        = image;
    batch->num_elements = num_elements;
    batch->base_element = base_element;
    batch->matrix       = _sb.projection_matrix;
}

static inline void _sb_init_batches(void) {
    int batch_size = 0;
    int base_element = 0;
    sg_image current_image = { SG_INVALID_ID };

    for (int i = 0; i < _sb.quad_count; ++i, ++batch_size) {
        if (_sb.quads[i].image.id != current_image.id) {
            const int num_elements = batch_size * 6;
            _sb_init_batch(current_image, num_elements, base_element);
            current_image = _sb.quads[i].image;
            batch_size = 0;
            base_element += num_elements;
        }
    }

    const int num_elements = batch_size * 6;
    _sb_init_batch(current_image, num_elements, base_element);
}

SOKOL_API_IMPL void sb_premultiply_alpha(uint8_t* pixels, int pixel_count) {
    /*
        http://www.realtimerendering.com/blog/gpus-prefer-premultiplication/
        https://shawnhargreaves.com/blog/premultiplied-alpha.html
    */
    SOKOL_ASSERT(pixels);
    for (int i = 0; i < pixel_count; ++i) {
        pixels[0] = pixels[0] * pixels[3] / 255;
        pixels[1] = pixels[1] * pixels[3] / 255;
        pixels[2] = pixels[2] * pixels[3] / 255;
        pixels += 4;
    }
}

SOKOL_API_IMPL void sb_setup(const sb_desc* desc) {
    SOKOL_ASSERT(desc);
    memset(&_sb, 0, sizeof(_sb_t));
    _sb_init_sprite_pool();
    _sb.quads = (_sb_quad*)SOKOL_MALLOC(_SB_MAX_QUADS * sizeof(_sb_quad));
    SOKOL_ASSERT(_sb.quads);
    _sb_init_vertex_buffer();
    _sb_init_index_buffer();
    _sb_init_batch_data();

    _sb.default_shader = sg_make_shader(spritebatch_shader_desc(sg_query_backend()));

    sg_pipeline_desc pipeline_desc;
    memset(&pipeline_desc, 0, sizeof(sg_pipeline_desc));
    pipeline_desc.color_count                      = 1;
    pipeline_desc.colors[0].blend.enabled          = true;
    pipeline_desc.colors[0].blend.src_factor_rgb   = SG_BLENDFACTOR_ONE;
    pipeline_desc.colors[0].blend.src_factor_alpha = SG_BLENDFACTOR_ONE;
    pipeline_desc.colors[0].blend.dst_factor_rgb   = SG_BLENDFACTOR_ONE_MINUS_SRC_ALPHA;
    pipeline_desc.colors[0].blend.dst_factor_alpha = SG_BLENDFACTOR_ONE_MINUS_SRC_ALPHA;
    pipeline_desc.shader                           = _sb.default_shader;
    pipeline_desc.index_type                       = SG_INDEXTYPE_UINT16;
    pipeline_desc.layout.attrs[0].format           = SG_VERTEXFORMAT_FLOAT3;
    pipeline_desc.layout.attrs[1].format           = SG_VERTEXFORMAT_FLOAT2;
    pipeline_desc.layout.attrs[2].format           = SG_VERTEXFORMAT_UBYTE4N;
    pipeline_desc.label                            = "spritebatch-default-pipeline";

    _sb.default_pipeline = sg_make_pipeline(&pipeline_desc);
}

SOKOL_API_IMPL void sb_shutdown(void) {
    sg_destroy_pipeline(_sb.default_pipeline);
    sg_destroy_shader(_sb.default_shader);
    SOKOL_FREE(_sb.batch_data.batches);
    sg_destroy_buffer(_sb.index_buffer);
    sg_destroy_buffer(_sb.vertex_buffer);
    SOKOL_FREE(_sb.vertex_buffer_data);
    SOKOL_FREE(_sb.quads);
    SOKOL_FREE(_sb.sprite_pool.data);
}

SOKOL_API_IMPL void sb_begin(const sb_render_state* render_state) {
    SOKOL_ASSERT(render_state);
    SOKOL_ASSERT(!_sb.begin_called);
    _sb.begin_called = true;

    _sb.render_state.viewport = render_state->viewport;
    _sb.render_state.sort_mode = render_state->sort_mode;
    _sb.render_state.pipeline.id
        = _SB_DEFAULT(render_state->pipeline.id, _sb.default_pipeline.id);

    const float width = (float)_sb.render_state.viewport.width;
    const float height = (float)_sb.render_state.viewport.height;

    _sb.projection_matrix
        = _sb_orthographic_off_center(0.0f, width, height, 0.0f, 0.0f, SB_MAX_DEPTH);

    _sb.render_state.transform_matrix
        = _sb_matrix_is_null(&render_state->transform_matrix)
            ? _sb_matrix_identity()
            : render_state->transform_matrix;
}

SOKOL_API_IMPL void sb_sprite(const sb_sprite_info* sprite) {
    /* TODO: maybe take an array of sprite info? */
    SOKOL_ASSERT(sprite);
    SOKOL_ASSERT(sprite->image.id != SG_INVALID_ID);
    SOKOL_ASSERT(_sb.quad_count < _SB_MAX_QUADS);

    _sb_sprite_data* cached_sprite_data = &_sb.sprite_pool.data[_sg_image_slot_index(sprite->image.id)];
    if (cached_sprite_data->image.id != sprite->image.id)
    {
        sg_image_info info = sg_query_image_info(sprite->image);
        cached_sprite_data->height = info.height;
        cached_sprite_data->width  = info.width;
        cached_sprite_data->image  = sprite->image;
    }

    const float scale_x = _SB_DEFAULT(sprite->scale.x, 1.0f);
    const float scale_y = _SB_DEFAULT(sprite->scale.y, 1.0f);

    const float sprite_width  = _SB_DEFAULT(sprite->width, (float)cached_sprite_data->width);
    const float sprite_height = _SB_DEFAULT(sprite->height, (float)cached_sprite_data->height);

    const float width  = sprite_width * scale_x;
    const float height = sprite_height * scale_y;

    const float texel_width  = (1.0f / cached_sprite_data->width);
    const float texel_height = (1.0f / cached_sprite_data->height);

    sb_float2 tex_coord_top_left = {
        sprite->source.x * texel_width,
        sprite->source.y * texel_height
    };

    sb_float2 tex_coord_bottom_right = {
        (sprite->source.x + sprite_width)  * texel_width,
        (sprite->source.y + sprite_height) * texel_height
    };

    if ((sprite->flags & SB_FLIP_Y) != SB_FLIP_NONE)
    {
        const float temp         = tex_coord_bottom_right.y;
        tex_coord_bottom_right.y = tex_coord_top_left.y;
        tex_coord_top_left.y     = temp;
    }

    if ((sprite->flags & SB_FLIP_X) != SB_FLIP_NONE)
    {
        const float temp         = tex_coord_bottom_right.x;
        tex_coord_bottom_right.x = tex_coord_top_left.x;
        tex_coord_top_left.x     = temp;
    }

    const float scaled_origin_x = scale_x * sprite->origin.x;
    const float scaled_origin_y = scale_y * sprite->origin.y;

    _sb_quad* quad = &_sb.quads[_sb.quad_count++];
    quad->sort_key = _sb_make_sort_key(sprite);
    quad->image = sprite->image;

    uint32_t packed_color = _sb_pack_color(&sprite->color);
    packed_color = packed_color == 0 ? 0xFFFFFFFF : packed_color;

    if (sprite->rotation == 0.0f)
    {
        _sb_init_quad(quad,
            sprite->flags,
            sprite->position.x - scaled_origin_x,
            sprite->position.y - scaled_origin_y,
            width,
            height,
            packed_color,
            tex_coord_top_left,
            tex_coord_bottom_right,
            sprite->depth);
    }
    else
    {
        _sb_init_quad_rotated(quad,
            sprite->position.x,
            sprite->position.y,
            -scaled_origin_x,
            -scaled_origin_y,
            width,
            height,
            sinf(sprite->rotation),
            cosf(sprite->rotation),
            packed_color,
            tex_coord_top_left,
            tex_coord_bottom_right,
            sprite->depth);
    }
}

SOKOL_API_IMPL void sb_end(void) {

    SOKOL_ASSERT(_sb.begin_called);
    _sb.begin_called = false;

    if (_sb.quad_count == 0) {
        return;
    }

    if (_sb.render_state.sort_mode != SB_SORT_MODE_DEFERRED) {
        qsort(_sb.quads, _sb.quad_count, sizeof(_sb_quad), _sb_quad_compare);
    }

    /*
        If this is moved to storing the quad data SOA, we would be able to use the array that stores all the
        vertex data verbatim without copying vertex data into an auxiliary buffer. This would significantly
        complicate the code though, especially sorting the quads before submission. Can't use a standard sorting
        algorithm to sort SOA data. Would need to implement a bespoke solution and that'd be a bit of a pain to
        profile and maintain. It is something to think about though.
    */
    for (size_t i = 0; i < _sb.quad_count; i++) {
        memcpy(_sb.vertex_buffer_data + (i * 4), &_sb.quads[i], 4 * sizeof(_sb_vertex));
    }

    _sb_init_batches();
}

SOKOL_API_IMPL void sb_draw(void) {

    if (_sb.batch_data.batch_size == 0) {
        return;
    }

    const sg_range range = { _sb.vertex_buffer_data, _sb.quad_count * 4 * sizeof(_sb_vertex) };
    sg_update_buffer(_sb.vertex_buffer, &range);

    for (size_t i = 1; i < _sb.batch_data.batch_size; i++) {
        _sb_batch* batch = _sb.batch_data.batches + i;
        sg_apply_pipeline(_sb.default_pipeline);
        sg_apply_uniforms(SG_SHADERSTAGE_VS, 0, &SG_RANGE(batch->matrix));
        _sb.bindings.fs_images[0] = batch->image;
        sg_apply_bindings(&_sb.bindings);
        sg_draw(batch->base_element, batch->num_elements, 1);
    }

    _sb.quad_count = 0;
    _sb.batch_data.batch_size = 0;
}

#endif /* SOKOL_SPRITEBATCH_IMPL */
