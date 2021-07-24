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

#ifdef __cplusplus
extern "C" {
#endif

    typedef enum sbatch_sprite_flags {
        SBATCH_FLIP_NONE = 0,
        SBATCH_FLIP_X = 1 << 0,
        SBATCH_FLIP_Y = 1 << 1,
        SBATCH_FLIP_BOTH = SBATCH_FLIP_Y | SBATCH_FLIP_X
    } sbatch_sprite_flags;

    typedef struct sbatch_float2 {
        float x;
        float y;
    } sbatch_float2;

    typedef struct sbatch_rect {
        float x;
        float y;
        float width;
        float height;
    } sbatch_rect;

    typedef struct sbatch_sprite {
        sg_image        image;
        sbatch_float2   position;
        sbatch_rect     source;
        const sg_color* color;
        float           rotation;
        sbatch_float2   origin;
        sbatch_float2   scale;
        uint32_t        flags;
        float           depth;
    } sbatch_sprite;

    typedef struct sbatch_sprite_rect {
        sg_image        image;
        sbatch_rect     destination;
        sbatch_rect     source;
        const sg_color* color;
        float           rotation;
        sbatch_float2   origin;
        uint32_t        flags;
        float           depth;
    } sbatch_sprite_rect;

    typedef struct sbatch_desc {
        int context_pool_size;
        sg_pixel_format color_format;
        sg_pixel_format depth_format;
        int sample_count;
    } sbatch_desc;

    typedef struct sbatch_context {
        uint32_t id;
    } sbatch_context;

    enum
    {
        SBATCH_MAX_SPRITES = (1 << 16) / 4,
        SBATCH_DEFAULT_SPRITES = SBATCH_MAX_SPRITES / 4
    };

    typedef struct sbatch_context_desc {
        int canvas_width;
        int canvas_height;
        int max_sprites;
        sg_pipeline pipeline;
        const char* label;
    } sbatch_context_desc;

    SOKOL_SPRITEBATCH_API_DECL void sbatch_setup(const sbatch_desc* desc);
    SOKOL_SPRITEBATCH_API_DECL void sbatch_shutdown(void);
    SOKOL_SPRITEBATCH_API_DECL int sbatch_frame();

    SOKOL_SPRITEBATCH_API_DECL sbatch_context sbatch_make_context(const sbatch_context_desc* desc);
    SOKOL_SPRITEBATCH_API_DECL void sbatch_destroy_context(sbatch_context context);

    SOKOL_SPRITEBATCH_API_DECL void sbatch_begin(sbatch_context context);
    SOKOL_SPRITEBATCH_API_DECL void sbatch_push_sprite(const sbatch_sprite* sprite);
    SOKOL_SPRITEBATCH_API_DECL void sbatch_push_sprite_rect(const sbatch_sprite_rect* sprite);
    SOKOL_SPRITEBATCH_API_DECL void sbatch_end(void);

    SOKOL_SPRITEBATCH_API_DECL void sbatch_apply_fs_uniforms(int ub_index, const sg_range* data);

    SOKOL_SPRITEBATCH_API_DECL void sbatch_premultiply_alpha_rgba8(uint8_t* pixels, int pixel_count);

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

#ifndef SOKOL_DEBUG
#ifndef NDEBUG
#define SOKOL_DEBUG (1)
#endif
#endif

#ifndef SOKOL_LOG
#ifdef SOKOL_DEBUG
#include <stdio.h>
#define SOKOL_LOG(s) { SOKOL_ASSERT(s); puts(s); }
#else
#define SOKOL_LOG(s)
#endif
#endif

#define _SBATCH_IMAGE_SLOT_MASK (0xFFFF)

#define _SBATCH_DEFAULT(val, def) (((val) == 0) ? (def) : (val))

#define _SBATCH_MAX_VERTICES (1 << 16)
#define _SBATCH_MAX_QUADS (_SBATCH_MAX_VERTICES / 4)
#define _SBATCH_MAX_INDICES (_SBATCH_MAX_QUADS * 6)

#define _SBATCH_INITIAL_BATCH_CAPACITY (32)

#define _SBATCH_INVALID_SLOT_INDEX (0)
#define _SBATCH_SLOT_SHIFT (16)
#define _SBATCH_MAX_POOL_SIZE (1<<_SBATCH_SLOT_SHIFT)
#define _SBATCH_SLOT_MASK (_SBATCH_MAX_POOL_SIZE-1)
#define _SBATCH_STRBUF_LEN (96)

typedef struct {
    float    x;
    float    y;
    float    z;
    float    u;
    float    v;
    uint32_t rgba;
} _sbatch_vertex;

typedef struct {
    uint32_t id;
    sg_resource_state state;
} _sbatch_slot;

typedef struct {
    char buf[_SBATCH_STRBUF_LEN];
} _sbatch_str;

typedef struct _sbatch_fs_uniform_state {
    int ub_index;
    const sg_range* data;
} _sbatch_fs_uniform_state;

typedef struct {
    _sbatch_str label;
    _sbatch_slot slot;
    sbatch_context_desc desc;
    int sprite_count;
    _sbatch_vertex* vertices;
    sg_image* images;
    sg_buffer vertex_buffer;
    sg_pipeline pipeline;
    int update_frame_index;
    _sbatch_fs_uniform_state fs_uniform_state;
} _sbatch_context;

typedef struct {
    int size;
    int queue_top;
    uint32_t* gen_ctrs;
    int* free_queue;
} _sbatch_pool;

typedef struct {
    _sbatch_pool pool;
    _sbatch_context* contexts;
} _sbatch_context_pool;

typedef struct {
    sg_image image;
    int      width;
    int      height;
    float    texel_width;
    float    texel_height;
} _sbatch_sprite_data;

typedef struct {
    _sbatch_sprite_data* data;
    size_t           size;
} _sbatch_sprite_pool;

typedef struct {
    bool begin_called;
    _sbatch_slot slot;
    sg_bindings bindings;
    sg_shader shader;
    sg_pipeline pipeline;
    sbatch_context ctx_id;
    _sbatch_context_pool context_pool;
    _sbatch_sprite_pool sprite_pool;
    sg_buffer index_buffer;
    int frame_index;
} _sbatch_t;

typedef struct {
    float m[4][4];
} _sbatch_mat4x4;

static _sbatch_t _sbatch;

#if defined(SOKOL_D3D11)
static const uint8_t vs_bytecode[884] = {
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

static const uint8_t fs_bytecode[620] = {
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

static const sg_shader_desc* spritebatch_shader_desc(sg_backend backend) {
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
            desc.vs.bytecode.ptr = vs_bytecode;
            desc.vs.bytecode.size = 884;
            desc.vs.entry = "main";
            desc.vs.uniform_blocks[0].size = 64;
            desc.fs.bytecode.ptr = fs_bytecode;
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

static void _sbatch_strcpy(_sbatch_str* dst, const char* src) {
    SOKOL_ASSERT(dst);
    if (src) {
#if defined(_MSC_VER)
        strncpy_s(dst->buf, _SBATCH_STRBUF_LEN, src, (_SBATCH_STRBUF_LEN - 1));
#else
        strncpy(dst->buf, src, SG_IMGUI_STRBUF_LEN);
#endif
        dst->buf[_SBATCH_STRBUF_LEN - 1] = 0;
    }
    else {
        memset(dst->buf, 0, _SBATCH_STRBUF_LEN);
    }
}

static _sbatch_str _sbatch_make_str(const char* str) {
    _sbatch_str res;
    _sbatch_strcpy(&res, str);
    return res;
}

/*=== CONTEXT POOL ===========================================================*/
static void _sbatch_init_pool(_sbatch_pool* pool, int num) {
    SOKOL_ASSERT(pool && (num >= 1));
    /* slot 0 is reserved for the 'invalid id', so bump the pool size by 1 */
    pool->size = num + 1;
    pool->queue_top = 0;
    /* generation counters indexable by pool slot index, slot 0 is reserved */
    const size_t gen_ctrs_size = sizeof(uint32_t) * (size_t)pool->size;
    pool->gen_ctrs = (uint32_t*)SOKOL_MALLOC(gen_ctrs_size);
    SOKOL_ASSERT(pool->gen_ctrs);
    memset(pool->gen_ctrs, 0, gen_ctrs_size);
    /* it's not a bug to only reserve 'num' here */
    pool->free_queue = (int*)SOKOL_MALLOC(sizeof(int) * (size_t)num);
    SOKOL_ASSERT(pool->free_queue);
    /* never allocate the zero-th pool item since the invalid id is 0 */
    for (int i = pool->size - 1; i >= 1; i--) {
        pool->free_queue[pool->queue_top++] = i;
    }
}

static void _sbatch_discard_pool(_sbatch_pool* pool) {
    SOKOL_ASSERT(pool);
    SOKOL_ASSERT(pool->free_queue);
    SOKOL_FREE(pool->free_queue);
    pool->free_queue = 0;
    SOKOL_ASSERT(pool->gen_ctrs);
    SOKOL_FREE(pool->gen_ctrs);
    pool->gen_ctrs = 0;
    pool->size = 0;
    pool->queue_top = 0;
}

static int _sbatch_pool_alloc_index(_sbatch_pool* pool) {
    SOKOL_ASSERT(pool);
    SOKOL_ASSERT(pool->free_queue);
    if (pool->queue_top > 0) {
        const int slot_index = pool->free_queue[--pool->queue_top];
        SOKOL_ASSERT((slot_index > 0) && (slot_index < pool->size));
        return slot_index;
    }
    /* pool exhausted */
    return _SBATCH_INVALID_SLOT_INDEX;
}

static void _sbatch_pool_free_index(_sbatch_pool* pool, int slot_index) {
    SOKOL_ASSERT((slot_index > _SBATCH_INVALID_SLOT_INDEX) && (slot_index < pool->size));
    SOKOL_ASSERT(pool);
    SOKOL_ASSERT(pool->free_queue);
    SOKOL_ASSERT(pool->queue_top < pool->size);
#ifdef SOKOL_DEBUG
    /* debug check against double-free */
    for (int i = 0; i < pool->queue_top; i++) {
        SOKOL_ASSERT(pool->free_queue[i] != slot_index);
    }
#endif
    pool->free_queue[pool->queue_top++] = slot_index;
    SOKOL_ASSERT(pool->queue_top <= (pool->size - 1));
}

static void _sbatch_setup_context_pool(const sbatch_desc* desc) {
    SOKOL_ASSERT(desc);
    /* note: the pool will have an additional item, since slot 0 is reserved */
    SOKOL_ASSERT((desc->context_pool_size > 0) && (desc->context_pool_size < _SBATCH_MAX_POOL_SIZE));
    _sbatch_init_pool(&_sbatch.context_pool.pool, desc->context_pool_size);
    const size_t pool_byte_size = sizeof(_sbatch_context) * (size_t)_sbatch.context_pool.pool.size;
    _sbatch.context_pool.contexts = (_sbatch_context*)SOKOL_MALLOC(pool_byte_size);
    SOKOL_ASSERT(_sbatch.context_pool.contexts);
    memset(_sbatch.context_pool.contexts, 0, pool_byte_size);
}

static void _sbatch_discard_context_pool(void) {
    SOKOL_ASSERT(_sbatch.context_pool.contexts);
    SOKOL_FREE(_sbatch.context_pool.contexts);
    _sbatch.context_pool.contexts = 0;
    _sbatch_discard_pool(&_sbatch.context_pool.pool);
}

/* allocate the slot at slot_index:
    - bump the slot's generation counter
    - create a resource id from the generation counter and slot index
    - set the slot's id to this id
    - set the slot's state to ALLOC
    - return the resource id
*/
static uint32_t _sbatch_slot_alloc(_sbatch_pool* pool, _sbatch_slot* slot, int slot_index) {
    /* FIXME: add handling for an overflowing generation counter,
       for now, just overflow (another option is to disable
       the slot)
    */
    SOKOL_ASSERT(pool && pool->gen_ctrs);
    SOKOL_ASSERT((slot_index > _SBATCH_INVALID_SLOT_INDEX) && (slot_index < pool->size));
    SOKOL_ASSERT((slot->state == SG_RESOURCESTATE_INITIAL) && (slot->id == SG_INVALID_ID));
    const uint32_t ctr = ++pool->gen_ctrs[slot_index];
    slot->id = (ctr << _SBATCH_SLOT_SHIFT) | (slot_index & _SBATCH_SLOT_MASK);
    slot->state = SG_RESOURCESTATE_ALLOC;
    return slot->id;
}

/* extract slot index from id */
static int _sbatch_slot_index(uint32_t id) {
    const int slot_index = (int)(id & _SBATCH_SLOT_MASK);
    SOKOL_ASSERT(_SBATCH_INVALID_SLOT_INDEX != slot_index);
    return slot_index;
}

/* get context pointer without id-check */
static _sbatch_context* _sbatch_context_at(uint32_t ctx_id) {
    SOKOL_ASSERT(SG_INVALID_ID != ctx_id);
    const int slot_index = _sbatch_slot_index(ctx_id);
    SOKOL_ASSERT((slot_index > _SBATCH_INVALID_SLOT_INDEX) && (slot_index < _sbatch.context_pool.pool.size));
    return &_sbatch.context_pool.contexts[slot_index];
}

/* get context pointer with id-check, returns 0 if no match */
static _sbatch_context* _sbatch_lookup_context(uint32_t ctx_id) {
    if (SG_INVALID_ID != ctx_id) {
        _sbatch_context* ctx = _sbatch_context_at(ctx_id);
        if (ctx->slot.id == ctx_id) {
            return ctx;
        }
    }
    return NULL;
}

/* make context handle from raw uint32_t id */
static sbatch_context _sbatch_make_ctx_id(uint32_t ctx_id) {
    sbatch_context ctx;
    ctx.id = ctx_id;
    return ctx;
}

static sbatch_context _sbatch_alloc_context(void) {
    sbatch_context ctx_id;
    const int slot_index = _sbatch_pool_alloc_index(&_sbatch.context_pool.pool);
    if (_SBATCH_INVALID_SLOT_INDEX != slot_index) {
        ctx_id = _sbatch_make_ctx_id(_sbatch_slot_alloc(&_sbatch.context_pool.pool, &_sbatch.context_pool.contexts[slot_index].slot, slot_index));
    }
    else {
        /* pool is exhausted */
        ctx_id = _sbatch_make_ctx_id(SG_INVALID_ID);
    }
    return ctx_id;
}

static sbatch_context_desc _sbatch_context_desc_defaults(const sbatch_context_desc* desc) {
    sbatch_context_desc res = *desc;
    res.max_sprites = _SBATCH_DEFAULT(res.max_sprites, 4096);
    res.canvas_height = _SBATCH_DEFAULT(res.canvas_height, 480);
    res.canvas_width = _SBATCH_DEFAULT(res.canvas_width, 640);
    return res;
}

static void _sbatch_init_context(sbatch_context ctx_id, const sbatch_context_desc* in_desc) {
    sg_push_debug_group("sokol-spritebatch");

    SOKOL_ASSERT((ctx_id.id != SG_INVALID_ID) && in_desc);

    _sbatch_context* ctx = _sbatch_lookup_context(ctx_id.id);
    SOKOL_ASSERT(ctx);

    ctx->update_frame_index = -1;

    ctx->desc = _sbatch_context_desc_defaults(in_desc);

    ctx->label = _sbatch_make_str(ctx->desc.label);

    ctx->desc.label = NULL;

    const int max_vertices = 4 * ctx->desc.max_sprites;
    const size_t vbuf_size = (size_t)max_vertices * sizeof(_sbatch_vertex);

    ctx->vertices = (_sbatch_vertex*)SOKOL_MALLOC(vbuf_size);
    SOKOL_ASSERT(ctx->vertices);

    ctx->images = (sg_image*)SOKOL_MALLOC(ctx->desc.max_sprites * sizeof(sg_image));
    SOKOL_ASSERT(ctx->images);

    ctx->pipeline.id = _SBATCH_DEFAULT(in_desc->pipeline.id, _sbatch.pipeline.id);

    ctx->sprite_count = 0;

    sg_buffer_desc vbuf_desc;
    memset(&vbuf_desc, 0, sizeof(vbuf_desc));
    vbuf_desc.size = vbuf_size;
    vbuf_desc.type = SG_BUFFERTYPE_VERTEXBUFFER;
    vbuf_desc.usage = SG_USAGE_STREAM;
    vbuf_desc.label = "sokol-spritebatch-vertices";
    ctx->vertex_buffer = sg_make_buffer(&vbuf_desc);
    SOKOL_ASSERT(SG_INVALID_ID != ctx->vertex_buffer.id);

    sg_pop_debug_group();
}

static void _sbatch_destroy_context(sbatch_context ctx_id) {
    _sbatch_context* ctx = _sbatch_lookup_context(ctx_id.id);
    if (ctx) {
        ctx->sprite_count = 0;
        if (ctx->vertices) {
            SOKOL_FREE(ctx->vertices);
        }
        if (ctx->images) {
            SOKOL_FREE(ctx->images);
        }
        sg_push_debug_group("sokol-spritebatch");
        sg_destroy_buffer(ctx->vertex_buffer);
        sg_pop_debug_group();
        memset(ctx, 0, sizeof(*ctx));
        _sbatch_pool_free_index(&_sbatch.context_pool.pool, _sbatch_slot_index(ctx_id.id));
    }
}

static uint32_t _sbatch_pack_color_bytes(uint8_t r, uint8_t g, uint8_t b, uint8_t a) {
    return (uint32_t)a << 24 | (uint32_t)b << 16 | (uint32_t)g << 8 | (uint32_t)r;
}

static float _sbatch_clampf(float v, float low, float high) {
    if (v < low) {
        return low;
    }
    if (v > high) {
        return high;
    }
    return v;
}

static uint32_t _sbatch_pack_color(const sg_color* color) {
    const uint8_t r = (uint8_t)(_sbatch_clampf(color->r, 0.0f, 1.0f) * 255.0f);
    const uint8_t g = (uint8_t)(_sbatch_clampf(color->g, 0.0f, 1.0f) * 255.0f);
    const uint8_t b = (uint8_t)(_sbatch_clampf(color->b, 0.0f, 1.0f) * 255.0f);
    const uint8_t a = (uint8_t)(_sbatch_clampf(color->a, 0.0f, 1.0f) * 255.0f);
    return _sbatch_pack_color_bytes(r, g, b, a);
}

static int _sg_image_slot_index(uint32_t id) {
    int slot_index = (int)(id & _SBATCH_IMAGE_SLOT_MASK);
    SOKOL_ASSERT(0 != slot_index);
    return slot_index;
}

static void _sbatch_setup_sprite_pool(void) {
    sg_desc sg_desc = sg_query_desc();
    const int num_slots = sg_desc.image_pool_size;
    _sbatch.sprite_pool.size = (size_t)num_slots * sizeof(_sbatch_sprite_data);
    _sbatch.sprite_pool.data = (_sbatch_sprite_data*)SOKOL_MALLOC(_sbatch.sprite_pool.size);
    SOKOL_ASSERT(_sbatch.sprite_pool.data);
    memset(_sbatch.sprite_pool.data, 0, _sbatch.sprite_pool.size);
}

static void _sbatch_init_quad_rotated(
    _sbatch_vertex* vertices, float x, float y, float dx, float dy,
    float w, float h, float sin, float cos, uint32_t rgba,
    sbatch_float2 top_left, sbatch_float2 bottom_right, float depth) {

    _sbatch_vertex* top_left_vertex = vertices;
    top_left_vertex->x = x + dx * cos - dy * sin;
    top_left_vertex->y = y + dx * sin + dy * cos;
    top_left_vertex->z = depth;
    top_left_vertex->rgba = rgba;
    top_left_vertex->u = top_left.x;
    top_left_vertex->v = top_left.y;

    _sbatch_vertex* top_right_vertex = vertices + 1;
    top_right_vertex->x = x + (dx + w) * cos - dy * sin;
    top_right_vertex->y = y + (dx + w) * sin + dy * cos;
    top_right_vertex->z = depth;
    top_right_vertex->rgba = rgba;
    top_right_vertex->u = bottom_right.x;
    top_right_vertex->v = top_left.y;

    _sbatch_vertex* bottom_left_vertex = vertices + 2;
    bottom_left_vertex->x = x + dx * cos - (dy + h) * sin;
    bottom_left_vertex->y = y + dx * sin + (dy + h) * cos;
    bottom_left_vertex->z = depth;
    bottom_left_vertex->rgba = rgba;
    bottom_left_vertex->u = top_left.x;
    bottom_left_vertex->v = bottom_right.y;

    _sbatch_vertex* bottom_right_vertex = vertices + 3;
    bottom_right_vertex->x = x + (dx + w) * cos - (dy + h) * sin;
    bottom_right_vertex->y = y + (dx + w) * sin + (dy + h) * cos;
    bottom_right_vertex->z = depth;
    bottom_right_vertex->rgba = rgba;
    bottom_right_vertex->u = bottom_right.x;
    bottom_right_vertex->v = bottom_right.y;
}

static void _sbatch_init_quad(
    _sbatch_vertex* vertices, float x, float y, float w, float h, uint32_t rgba,
    sbatch_float2 top_left, sbatch_float2 bottom_right, float depth) {

    _sbatch_vertex* top_left_vertex = vertices;
    top_left_vertex->x = x;
    top_left_vertex->y = y;
    top_left_vertex->z = depth;
    top_left_vertex->rgba = rgba;
    top_left_vertex->u = top_left.x;
    top_left_vertex->v = top_left.y;

    _sbatch_vertex* top_right_vertex = vertices + 1;
    top_right_vertex->x = x + w;
    top_right_vertex->y = y;
    top_right_vertex->z = depth;
    top_right_vertex->rgba = rgba;
    top_right_vertex->u = bottom_right.x;
    top_right_vertex->v = top_left.y;

    _sbatch_vertex* bottom_left_vertex = vertices + 2;
    bottom_left_vertex->x = x;
    bottom_left_vertex->y = y + h;
    bottom_left_vertex->z = depth;
    bottom_left_vertex->rgba = rgba;
    bottom_left_vertex->u = top_left.x;
    bottom_left_vertex->v = bottom_right.y;

    _sbatch_vertex* bottom_right_vertex = vertices + 3;
    bottom_right_vertex->x = x + w;
    bottom_right_vertex->y = y + h;
    bottom_right_vertex->z = depth;
    bottom_right_vertex->rgba = rgba;
    bottom_right_vertex->u = bottom_right.x;
    bottom_right_vertex->v = bottom_right.y;
}

static void _sbatch_init_index_buffer(void) {
    uint16_t* index_buffer = (uint16_t*)SOKOL_MALLOC(_SBATCH_MAX_INDICES * sizeof(uint16_t));
    SOKOL_ASSERT(index_buffer);

    uint16_t* index_ptr = index_buffer;
    for (uint32_t i = 0; i < _SBATCH_MAX_QUADS; i++, index_ptr += 6) {
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
    index_buffer_desc.size = _SBATCH_MAX_INDICES * sizeof(uint16_t);
    index_buffer_desc.type = SG_BUFFERTYPE_INDEXBUFFER;
    index_buffer_desc.usage = SG_USAGE_IMMUTABLE;
    index_buffer_desc.label = "sokol-spritebatch-indices";
    index_buffer_desc.data.size = _SBATCH_MAX_INDICES * sizeof(uint16_t);
    index_buffer_desc.data.ptr = index_buffer;
    _sbatch.index_buffer = sg_make_buffer(&index_buffer_desc);
    SOKOL_ASSERT(SG_INVALID_ID != _sbatch.index_buffer.id);

    SOKOL_FREE(index_buffer);

    _sbatch.bindings.index_buffer = _sbatch.index_buffer;
}

static bool _sbatch_rect_is_valid(const sbatch_rect* rect) {
    if (rect->width != 0.0f && rect->height != 0.0f) {
        return true;
    }
    return false;
}

SOKOL_SPRITEBATCH_API_DECL int sbatch_frame() {
    return ++_sbatch.frame_index;
}

SOKOL_API_IMPL void sbatch_setup(const sbatch_desc* desc) {

    memset(&_sbatch, 0, sizeof _sbatch);

    sbatch_desc batch_desc = *desc;
    batch_desc.context_pool_size = _SBATCH_DEFAULT(batch_desc.context_pool_size, 32);
    _sbatch_setup_context_pool(&batch_desc);
    _sbatch_setup_sprite_pool();

    _sbatch.shader = sg_make_shader(spritebatch_shader_desc(sg_query_backend()));

    sg_pipeline_desc pipeline_desc;
    memset(&pipeline_desc, 0, sizeof(sg_pipeline_desc));
    pipeline_desc.color_count = 1;
    pipeline_desc.colors[0].blend.enabled = true;
    pipeline_desc.colors[0].blend.src_factor_rgb = SG_BLENDFACTOR_ONE;
    pipeline_desc.colors[0].blend.src_factor_alpha = SG_BLENDFACTOR_ONE;
    pipeline_desc.colors[0].blend.dst_factor_rgb = SG_BLENDFACTOR_ONE_MINUS_SRC_ALPHA;
    pipeline_desc.colors[0].blend.dst_factor_alpha = SG_BLENDFACTOR_ONE_MINUS_SRC_ALPHA;
    pipeline_desc.shader = _sbatch.shader;
    pipeline_desc.index_type = SG_INDEXTYPE_UINT16;
    pipeline_desc.layout.attrs[0].format = SG_VERTEXFORMAT_FLOAT3;
    pipeline_desc.layout.attrs[1].format = SG_VERTEXFORMAT_FLOAT2;
    pipeline_desc.layout.attrs[2].format = SG_VERTEXFORMAT_UBYTE4N;
    pipeline_desc.label = "spritebatch-default-pipeline";

    _sbatch.pipeline = sg_make_pipeline(&pipeline_desc);

    _sbatch_init_index_buffer();
}

SOKOL_API_IMPL void sbatch_shutdown(void) {
    sg_destroy_buffer(_sbatch.index_buffer);
    sg_destroy_pipeline(_sbatch.pipeline);
    sg_destroy_shader(_sbatch.shader);
    _sbatch_discard_context_pool();
    SOKOL_FREE(_sbatch.sprite_pool.data);
}

SOKOL_API_IMPL sbatch_context sbatch_make_context(const sbatch_context_desc* desc) {
    SOKOL_ASSERT(desc);
    const sbatch_context result = _sbatch_alloc_context();
    _sbatch_init_context(result, desc);
    return result;
}

SOKOL_API_IMPL void sbatch_destroy_context(sbatch_context context) {
    SOKOL_ASSERT(context.id != SG_INVALID_ID);
    _sbatch_destroy_context(context);
}

SOKOL_API_IMPL void sbatch_begin(sbatch_context context) {
    SOKOL_ASSERT(context.id != SG_INVALID_ID);
    _sbatch.ctx_id = context;
    SOKOL_ASSERT(!_sbatch.begin_called);
    _sbatch.begin_called = true;

    _sbatch_context* ctx = _sbatch_context_at(_sbatch.ctx_id.id);

    // a sbatch_context object can only be used in one sbatch_begin call per frame.
    SOKOL_ASSERT(_sbatch.frame_index != ctx->update_frame_index);
    ctx->update_frame_index = _sbatch.frame_index;

    SOKOL_ASSERT(ctx);
    _sbatch.bindings.vertex_buffers[0] = ctx->vertex_buffer;
}

SOKOL_API_IMPL void sbatch_push_sprite(const sbatch_sprite* sprite) {
    SOKOL_ASSERT(sprite);
    SOKOL_ASSERT(sprite->image.id != SG_INVALID_ID);

    _sbatch_context* ctx = _sbatch_context_at(_sbatch.ctx_id.id);
    SOKOL_ASSERT(ctx);

    if (ctx->sprite_count < ctx->desc.max_sprites) {

        const int sprite_index = ctx->sprite_count++;

        _sbatch_sprite_data* cached_sprite_data = &_sbatch.sprite_pool.data[_sg_image_slot_index(sprite->image.id)];

        if (cached_sprite_data->image.id != sprite->image.id) {
            const sg_image_info info = sg_query_image_info(sprite->image);
            cached_sprite_data->height = info.height;
            cached_sprite_data->width = info.width;
            cached_sprite_data->texel_height = 1.0f / (float)info.height;
            cached_sprite_data->texel_width = 1.0f / (float)info.width;
            cached_sprite_data->image = sprite->image;
        }

        const sbatch_float2 scale = { _SBATCH_DEFAULT(sprite->scale.x, 1.0f),  _SBATCH_DEFAULT(sprite->scale.y, 1.0f) };
        const sbatch_float2 scaled_origin = { scale.x * sprite->origin.x, scale.y * sprite->origin.y };
        float width, height;
        sbatch_float2 tex_coord_top_left;
        sbatch_float2 tex_coord_bottom_right;

        if (_sbatch_rect_is_valid(&sprite->source)) {
            width = sprite->source.width * scale.x;
            height = sprite->source.height * scale.y;
            tex_coord_top_left.x = sprite->source.x * cached_sprite_data->texel_width;
            tex_coord_top_left.y = sprite->source.y * cached_sprite_data->texel_height;
            tex_coord_bottom_right.x = (sprite->source.x + sprite->source.width) * cached_sprite_data->texel_width;
            tex_coord_bottom_right.y = (sprite->source.y + sprite->source.height) * cached_sprite_data->texel_height;
        } else {
            width = (float)cached_sprite_data->width * scale.x;
            height = (float)cached_sprite_data->height * scale.y;
            tex_coord_top_left.x = 0.0f;
            tex_coord_top_left.y = 0.0f;
            tex_coord_bottom_right.x = 1.0f;
            tex_coord_bottom_right.y = 1.0f;
        }

        if ((sprite->flags & SBATCH_FLIP_Y) != SBATCH_FLIP_NONE) {
            const float temp = tex_coord_bottom_right.y;
            tex_coord_bottom_right.y = tex_coord_top_left.y;
            tex_coord_top_left.y = temp;
        }

        if ((sprite->flags & SBATCH_FLIP_X) != SBATCH_FLIP_NONE) {
            const float temp = tex_coord_bottom_right.x;
            tex_coord_bottom_right.x = tex_coord_top_left.x;
            tex_coord_top_left.x = temp;
        }

        uint32_t packed_color = sprite->color ? _sbatch_pack_color(sprite->color) : 0xFFFFFFFF;

        ctx->images[sprite_index] = sprite->image;

        const int base_vertex_index = sprite_index * 4;
        _sbatch_vertex* vertices = ctx->vertices + base_vertex_index;

        if (sprite->rotation == 0.0f) {
            _sbatch_init_quad(vertices,
                sprite->position.x - scaled_origin.x,
                sprite->position.y - scaled_origin.y,
                width,
                height,
                packed_color,
                tex_coord_top_left,
                tex_coord_bottom_right,
                sprite->depth);
        }
        else {
            _sbatch_init_quad_rotated(vertices,
                sprite->position.x,
                sprite->position.y,
                -scaled_origin.x,
                -scaled_origin.y,
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
    else {
        if (ctx->label.buf[0] != '\0') {
            SOKOL_LOG("sokol_spritebatch.h: dropped sprites, increase max_sprites of sbatch_context:");
            SOKOL_LOG(ctx->label.buf);
        }
        else {
            SOKOL_LOG("sokol_spritebatch.h: dropped sprites, increase max_sprites");
        }
    }
}

SOKOL_API_IMPL void sbatch_push_sprite_rect(const sbatch_sprite_rect* sprite) {
    SOKOL_ASSERT(sprite);
    SOKOL_ASSERT(sprite->image.id != SG_INVALID_ID);

    _sbatch_context* ctx = _sbatch_context_at(_sbatch.ctx_id.id);
    SOKOL_ASSERT(ctx);

    if (ctx->sprite_count < ctx->desc.max_sprites) {

        const int sprite_index = ctx->sprite_count++;

        _sbatch_sprite_data* cached_sprite_data = &_sbatch.sprite_pool.data[_sg_image_slot_index(sprite->image.id)];

        if (cached_sprite_data->image.id != sprite->image.id) {
            const sg_image_info info = sg_query_image_info(sprite->image);
            cached_sprite_data->height = info.height;
            cached_sprite_data->width = info.width;
            cached_sprite_data->texel_height = 1.0f / (float)info.height;
            cached_sprite_data->texel_width = 1.0f / (float)info.width;
            cached_sprite_data->image = sprite->image;
        }

        sbatch_float2 scaled_origin;
        sbatch_float2 tex_coord_top_left;
        sbatch_float2 tex_coord_bottom_right;

        if (_sbatch_rect_is_valid(&sprite->source)) {
            tex_coord_top_left.x = sprite->source.x * cached_sprite_data->texel_width;
            tex_coord_top_left.y = sprite->source.y * cached_sprite_data->texel_height;
            tex_coord_bottom_right.x = (sprite->source.x + sprite->source.width) * cached_sprite_data->texel_width;
            tex_coord_bottom_right.y = (sprite->source.y + sprite->source.height) * cached_sprite_data->texel_height;
            scaled_origin.x = sprite->origin.x * sprite->destination.width / sprite->source.width;
            scaled_origin.y = sprite->origin.y * sprite->destination.height / sprite->source.height;
        } else {
            tex_coord_top_left.x = 0.0f;
            tex_coord_top_left.y = 0.0f;
            tex_coord_bottom_right.x = 1.0f;
            tex_coord_bottom_right.y = 1.0f;
            scaled_origin.x = sprite->origin.x * sprite->destination.width * cached_sprite_data->texel_width;
            scaled_origin.y = sprite->origin.y * sprite->destination.height * cached_sprite_data->texel_height;
        }

        if ((sprite->flags & SBATCH_FLIP_Y) != SBATCH_FLIP_NONE) {
            const float temp = tex_coord_bottom_right.y;
            tex_coord_bottom_right.y = tex_coord_top_left.y;
            tex_coord_top_left.y = temp;
        }

        if ((sprite->flags & SBATCH_FLIP_X) != SBATCH_FLIP_NONE) {
            const float temp = tex_coord_bottom_right.x;
            tex_coord_bottom_right.x = tex_coord_top_left.x;
            tex_coord_top_left.x = temp;
        }

        uint32_t packed_color = sprite->color ? _sbatch_pack_color(sprite->color) : 0xFFFFFFFF;

        ctx->images[sprite_index] = sprite->image;

        const int base_vertex_index = sprite_index * 4;
        _sbatch_vertex* vertices = ctx->vertices + base_vertex_index;

        if (sprite->rotation == 0.0f) {
            _sbatch_init_quad(vertices,
                sprite->destination.x - scaled_origin.x,
                sprite->destination.y - scaled_origin.y,
                sprite->destination.width,
                sprite->destination.height,
                packed_color,
                tex_coord_top_left,
                tex_coord_bottom_right,
                sprite->depth);
        } else {
            _sbatch_init_quad_rotated(vertices,
                sprite->destination.x,
                sprite->destination.y,
                -scaled_origin.x,
                -scaled_origin.y,
                sprite->destination.width,
                sprite->destination.height,
                sinf(sprite->rotation),
                cosf(sprite->rotation),
                packed_color,
                tex_coord_top_left,
                tex_coord_bottom_right,
                sprite->depth);
        }
    }
    else {
        if (ctx->label.buf[0] != '\0') {
            SOKOL_LOG("sokol_spritebatch.h: dropped sprites, increase max_sprites of sbatch_context:");
            SOKOL_LOG(ctx->label.buf);
        } else {
            SOKOL_LOG("sokol_spritebatch.h: dropped sprites, increase max_sprites");
        }
    }
}

static void _sbatch_draw(int base_element, sg_image current_image, int num_elements) {
    _sbatch.bindings.fs_images[0] = current_image;
    sg_apply_bindings(&_sbatch.bindings);
    sg_draw(base_element, num_elements, 1);
}

static _sbatch_mat4x4 _sbatch_orthographic_off_center(float left, float right, float bottom, float top, float z_near, float z_far) {
    _sbatch_mat4x4 result;

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
    result.m[2][2] = 1.0f / (z_near - z_far);
    result.m[2][3] = 0.0f;

    result.m[3][0] = (left + right) / (left - right);
    result.m[3][1] = (bottom + top) / (bottom - top);
    result.m[3][2] = z_near / (z_near - z_far);
    result.m[3][3] = 1.0f;

    return result;
}

void sbatch_end(void) {
    SOKOL_ASSERT(_sbatch.begin_called);
    _sbatch.begin_called = false;

    _sbatch_context* ctx = _sbatch_context_at(_sbatch.ctx_id.id);
    SOKOL_ASSERT(ctx);

    if (ctx->sprite_count == 0) {
        return;
    }

    const int max_vertices = 4 * ctx->sprite_count;
    const size_t vbuf_size = (size_t)max_vertices * sizeof(_sbatch_vertex);
    const sg_range range = { ctx->vertices, vbuf_size };
    sg_update_buffer(ctx->vertex_buffer, &range);

    int batch_size = 0;
    int base_element = 0;
    sg_image current_image = ctx->images[0];

    sg_apply_pipeline(ctx->pipeline);

    _sbatch_mat4x4 matrix =
        _sbatch_orthographic_off_center(0.0f, (float)ctx->desc.canvas_width, (float)ctx->desc.canvas_height, 0.0f, 0.0f, 1000.0f);

    const sg_range matrix_range = { &matrix, sizeof matrix };
    sg_apply_uniforms(SG_SHADERSTAGE_VS, 0, &matrix_range);

    if(ctx->fs_uniform_state.data) {
        sg_apply_uniforms(SG_SHADERSTAGE_FS, ctx->fs_uniform_state.ub_index, ctx->fs_uniform_state.data);
    }

    for (int i = 0; i < ctx->sprite_count; ++i, ++batch_size) {
        if (ctx->images[i].id != current_image.id) {
            const int num_elements = batch_size * 6;
            _sbatch_draw(base_element, current_image, num_elements);
            batch_size = 0;
            base_element += num_elements;
            current_image = ctx->images[i];
        }
    }

    const int num_elements = batch_size * 6;
    _sbatch_draw(base_element, current_image, num_elements);

    ctx->sprite_count = 0;
}

SOKOL_API_IMPL void sbatch_apply_fs_uniforms(int ub_index, const sg_range* data) {
    SOKOL_ASSERT(data);
    SOKOL_ASSERT(data->ptr);
    _sbatch_context* ctx = _sbatch_context_at(_sbatch.ctx_id.id);
    ctx->fs_uniform_state.ub_index = ub_index;
    ctx->fs_uniform_state.data = data;
}

SOKOL_API_IMPL void sbatch_premultiply_alpha_rgba8(uint8_t* pixels, int pixel_count) {
    SOKOL_ASSERT(pixels);
    for (int i = 0; i < pixel_count; ++i) {
        pixels[0] = pixels[0] * pixels[3] / 255;
        pixels[1] = pixels[1] * pixels[3] / 255;
        pixels[2] = pixels[2] * pixels[3] / 255;
        pixels += 4;
    }
}

#endif /* SOKOL_SPRITEBATCH_IMPL */
