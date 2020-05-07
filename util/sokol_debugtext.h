#ifndef SOKOL_DEBUGTEXT_INCLUDED
/*
    WIP WIP WIP

    sokol_debugtext.h   - simple ASCII debug text rendering on top of sokol_gfx.h

    Project URL: https://github.com/floooh/sokol

    Do this:
        #define SOKOL_DEBUGTEXT_IMPL
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
    SOKOL_WGPU

    Define at least one of the following fonts to use:

    SOKOL_DEBUGTEXT_FONT_KC853
    SOKOL_DEBUGTEXT_FONT_KC854
    SOKOL_DEBUGTEXT_FONT_Z1013
    SOKOL_DEBUGTEXT_FONT_CPC
    SOKOL_DEBUGTEXT_FONT_C64

    ...optionally provide the following macros to override defaults:

    SOKOL_SNPRINTF      - the function name of an alternative snprintf() function (default: snprintf)
    SOKOL_ASSERT(c)     - your own assert macro (default: assert(c))
    SOKOL_MALLOC(s)     - your own malloc function (default: malloc(s))
    SOKOL_FREE(p)       - your own free function (default: free(p))
    SOKOL_API_DECL      - public function declaration prefix (default: extern)
    SOKOL_API_IMPL      - public function implementation prefix (default: -)
    SOKOL_LOG(msg)      - your own logging function (default: puts(msg))
    SOKOL_UNREACHABLE() - a guard macro for unreachable code (default: assert(false))

    If sokol_debugtext.h is compiled as a DLL, define the following before
    including the declaration or implementation:

    SOKOL_DLL

    On Windows, SOKOL_DLL will define SOKOL_API_DECL as __declspec(dllexport)
    or __declspec(dllimport) as needed.

    Include the following headers before including sokol_debugtext.h:

        sokol_gfx.h

    FEATURE OVERVIEW
    ================
    [TODO]

    STEP BY STEP
    ============
    [TODO]


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
#define SOKOL_DEBUGTEXT_INCLUDED (1)
#include <stdint.h>
#include <stdbool.h>

#ifndef SOKOL_API_DECL
#if defined(_WIN32) && defined(SOKOL_DLL) && defined(SOKOL_IMPL)
#define SOKOL_API_DECL __declspec(dllexport)
#elif defined(_WIN32) && defined(SOKOL_DLL)
#define SOKOL_API_DECL __declspec(dllimport)
#else
#define SOKOL_API_DECL extern
#endif
#endif

#if defined(__GNUC__)
#define SOKOL_DEBUGTEXT_PRINTF_ATTR __attribute__((format(printf, 1, 2)))
#else
#define SOKOL_DEBUGTEXT_PRINTF_ATTR
#endif

#ifdef __cplusplus
extern "C" {
#endif

/*
    sdtx_font_t

    Identifies one of the built-in 'vintage fonts'. To use a specific font it
    must also be 'activated' with the matching SOKOL_DEBUGTEXT_FONT_*
    compile-time define.
*/
typedef enum sdtx_font_t {
    SDTX_FONT_KC853 = 0,
    SDTX_FONT_KC854,
    SDTX_FONT_Z1013,
    SDTX_FONT_CPC,
    SDTX_FONT_C64,
    //--- keep at end:
    SDTX_FONT_NUM
} sdtx_font_t;

/* a rendering context handle */
typedef struct sdtx_context { uint32_t id; } sdtx_context;

/* the default context handle */
static const sdtx_context sdtx_default_context = { 0 };

/*
    sdtx_context_desc_t

    Describes the initialization parameters of a rendering context. Creating
    additional rendering contexts is useful if you want to render in
    different sokol-gfx rendering passes, or when rendering several layers
    of text.
*/
typedef struct sdtx_context_desc_t {
    int char_buf_size;              // max number of characters rendered in one frame, default: 16384
    int printf_buf_size;            // size of internal buffer for snprintf()
    int canvas_width;               // the initial virtual canvas width, default: 640
    int canvas_height;              // the initial virtual canvas height, default: 400
    sdtx_font_t font;               // the default font (default is the first valid embedded font)
    sg_pixel_format color_format;   // color pixel format of target render pass
    sg_pixel_format depth_format;   // depth pixel format of target render pass
    int sample_count;               // MSAA sample count of target render pass
} sdtx_context_desc_t;

/*
    sdtx_desc_t

    Describes the sokol-debugtext API initialization parameters. Passed
    to the sdtx_setup() function.
*/
typedef struct sdtx_desc_t {
    int context_pool_size;          // max number of rendering contexts that can be created, default: 16
    sdtx_context_desc_t context;    // the default context creation parameters
} sdtx_desc_t;

/* initialization/shutdown */
SOKOL_API_DECL void sdtx_setup(const sdtx_desc_t* desc);
SOKOL_API_DECL void sdtx_shutdown(void);

/* context functions */
SOKOL_API_DECL sdtx_context sdtx_make_context(const sdtx_context_desc_t* desc);
SOKOL_API_DECL void sdtx_destroy_context(sdtx_context ctx);
SOKOL_API_DECL void sdtx_set_context(sdtx_context ctx);
SOKOL_API_DECL sdtx_context sdtx_get_context(void);

/* draw and 'rewind' the current context */
SOKOL_API_DECL void sdtx_draw(void);

/* set a new virtual canvas size in screen pixels */
SOKOL_API_DECL void sdtx_canvas(int w, int h);

/* set a new origin in virtual canvas pixels */
SOKOL_API_DECL void sdtx_origin(int x, int y);

/* cursor movement functions */
SOKOL_API_DECL void sdtx_home(void);        // move to origin
SOKOL_API_DECL void sdtx_pos(int x, int y); // move to character grid pos
SOKOL_API_DECL void sdtx_x(int x);          // move to abs x, keep y unchanged
SOKOL_API_DECL void sdtx_y(int y);          // move to abs y, keep x unchanged
SOKOL_API_DECL void sdtx_dx(int dx);        // move left/right by dx
SOKOL_API_DECL void sdtx_dy(int dy);        // move up/down by dy
SOKOL_API_DECL void sdtx_crlf(void);        // start new line, same as sdtx_x(0); sdtx_dy(1)

/* get current cursor position */
SOKOL_API_DECL int sdtx_get_x(void);
SOKOL_API_DECL int sdtx_get_y(void);

/* set the current text color */
SOKOL_API_DECL void sdtx_color3b(uint8_t r, uint8_t g, uint8_t b);              // RGB 0..255, A=255
SOKOL_API_DECL void sdtx_color3f(float r, float g, float b);                    // RGB 0..1, A=1
SOKOL_API_DECL void sdtx_color4b(uint8_t r, uint8_t g, uint8_t b, uint8_t a);   // RGBA 0..255
SOKOL_API_DECL void sdtx_color4f(float r, float g, float b, float a);           // RGBA 0..1
SOKOL_API_DECL void sdtx_color1i(uint32_t rgba);

/* text rendering */
SOKOL_API_DECL void sdtx_putc(int chr);
SOKOL_API_DECL void sdtx_puts(const char* str);             // does NOT append newline!
SOKOL_API_DECL void sdtx_putr(const char* str, int len);    // 'put range', also stops at zero-char
SOKOL_API_DECL int sdtx_printf(const char* fmt, ...) SOKOL_DEBUGTEXT_PRINTF_ATTR;

#ifdef __cplusplus
} /* extern "C" */
#endif
#endif /* SOKOL_DEBUGTEXT_INCLUDED */


/*-- IMPLEMENTATION ----------------------------------------------------------*/
#ifdef SOKOL_DEBUGTEXT_IMPL
#define SOKOL_DEBUGTEXT_IMPL_INCLUDED (1)

#include <string.h> // memset

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

#ifndef SOKOL_SNPRINTF
#define SOKOL_SNPRINTF snprintf
#endif

typedef struct {
    uint32_t id;
    sg_resource_state state;
} _sdtx_slot_t;

typedef struct {
    int size;
    int queue_top;
    uint32_t* gen_ctrs;
    int* free_queue;
} _sdtx_pool_t;

typedef struct {
    int x, y;
} _sdtx_int2_t;

typedef struct {
    uint16_t u0, v0, u1, v1;
} _sdtx_uvrect_t;

typedef struct {
    _sdtx_uvrect_t uvs[256];
} _sdtx_font_t;

typedef struct {
    uint16_t x, y;
    uint32_t color;
} _sdtx_vertex_t;

typedef struct {
    _sdtx_slot_t slot;
    sg_buffer vbuf;
    sg_pipeline pip;
    uint32_t max_vertex;
    uint32_t cur_vertex;
    _sdtx_vertex_t* vertices;
    uint32_t fmt_buf_size;
    uint8_t* fmt_buf;
    sdtx_font_t cur_font;
    _sdtx_int2_t canvas_size;
    _sdtx_int2_t origin;
    _sdtx_int2_t pos;
    uint32_t color;
} _sdtx_context_t;

typedef struct {
    _sdtx_pool_t pool;
    _sdtx_context_t* contexts;
} _sdtx_context_pool_t;

typedef struct {
    uint32_t init_cookie;
    sdtx_desc_t desc;

    sg_image font_tex;
    _sdtx_font_t fonts[SDTX_FONT_NUM];
    uint32_t default_context_id;
    _sdtx_context_pool_t context_pool;
} _sdtx_t;
static _sdtx_t _sdtx;


/*=== CONTEXT POOL ===========================================================*/
#define _SDTX_INVALID_SLOT_INDEX (0)
#define _SDTX_SLOT_SHIFT (16)
#define _SDTX_MAX_POOL_SIZE (1<<_SDTX_SLOT_SHIFT)
#define _SDTX_SLOT_MASK (_SDTX_MAX_POOL_SIZE-1)

static void _sdtx_init_pool(_sdtx_pool_t* pool, int num) {
    SOKOL_ASSERT(pool && (num >= 1));
    /* slot 0 is reserved for the 'invalid id', so bump the pool size by 1 */
    pool->size = num + 1;
    pool->queue_top = 0;
    /* generation counters indexable by pool slot index, slot 0 is reserved */
    size_t gen_ctrs_size = sizeof(uint32_t) * pool->size;
    pool->gen_ctrs = (uint32_t*) SOKOL_MALLOC(gen_ctrs_size);
    SOKOL_ASSERT(pool->gen_ctrs);
    memset(pool->gen_ctrs, 0, gen_ctrs_size);
    /* it's not a bug to only reserve 'num' here */
    pool->free_queue = (int*) SOKOL_MALLOC(sizeof(int)*num);
    SOKOL_ASSERT(pool->free_queue);
    /* never allocate the zero-th pool item since the invalid id is 0 */
    for (int i = pool->size-1; i >= 1; i--) {
        pool->free_queue[pool->queue_top++] = i;
    }
}

static void _sdtx_discard_pool(_sdtx_pool_t* pool) {
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

static int _sdtx_pool_alloc_index(_sdtx_pool_t* pool) {
    SOKOL_ASSERT(pool);
    SOKOL_ASSERT(pool->free_queue);
    if (pool->queue_top > 0) {
        int slot_index = pool->free_queue[--pool->queue_top];
        SOKOL_ASSERT((slot_index > 0) && (slot_index < pool->size));
        return slot_index;
    }
    else {
        /* pool exhausted */
        return _SDTX_INVALID_SLOT_INDEX;
    }
}

static void _sdtx_pool_free_index(_sdtx_pool_t* pool, int slot_index) {
    SOKOL_ASSERT((slot_index > _SDTX_INVALID_SLOT_INDEX) && (slot_index < pool->size));
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
    SOKOL_ASSERT(pool->queue_top <= (pool->size-1));
}

static void _sdtx_reset_context(_sdtx_context_t* ctx) {
    SOKOL_ASSERT(ctx);
    memset(ctx, 0, sizeof(_sdtx_context_t));
}

static void _sdtx_setup_context_pool(const sdtx_desc_t* desc) {
    SOKOL_ASSERT(desc);
    /* note: the pool will have an additional item, since slot 0 is reserved */
    SOKOL_ASSERT((desc->context_pool_size > 0) && (desc->context_pool_size < _SDTX_MAX_POOL_SIZE));
    _sdtx_init_pool(&_sdtx.context_pool.pool, desc->context_pool_size);
    size_t pool_byte_size = sizeof(_sdtx_context_t) * _sdtx.context_pool.pool.size;
    _sdtx.context_pool.contexts = (_sdtx_context_t*) SOKOL_MALLOC(pool_byte_size);
    SOKOL_ASSERT(_sdtx.context_pool.contexts);
    memset(_sdtx.context_pool.contexts, 0, pool_byte_size);
}

static void _sdtx_discard_pipeline_pool(void) {
    SOKOL_FREE(_sdtx.context_pool.contexts);
    _sdtx.context_pool.contexts = 0;
    _sdtx_discard_pool(&_sdtx.context_pool.pool);
}

/* allocate the slot at slot_index:
    - bump the slot's generation counter
    - create a resource id from the generation counter and slot index
    - set the slot's id to this id
    - set the slot's state to ALLOC
    - return the resource id
*/
static uint32_t _sdtx_slot_alloc(_sdtx_pool_t* pool, _sdtx_slot_t* slot, int slot_index) {
    /* FIXME: add handling for an overflowing generation counter,
       for now, just overflow (another option is to disable
       the slot)
    */
    SOKOL_ASSERT(pool && pool->gen_ctrs);
    SOKOL_ASSERT((slot_index > _SDTX_INVALID_SLOT_INDEX) && (slot_index < pool->size));
    SOKOL_ASSERT((slot->state == SG_RESOURCESTATE_INITIAL) && (slot->id == SG_INVALID_ID));
    uint32_t ctr = ++pool->gen_ctrs[slot_index];
    slot->id = (ctr<<_SDTX_SLOT_SHIFT)|(slot_index & _SDTX_SLOT_MASK);
    slot->state = SG_RESOURCESTATE_ALLOC;
    return slot->id;
}

/* extract slot index from id */
static int _sdtx_slot_index(uint32_t id) {
    int slot_index = (int) (id & _SDTX_SLOT_MASK);
    SOKOL_ASSERT(_SDTX_INVALID_SLOT_INDEX != slot_index);
    return slot_index;
}

/* get context pointer without id-check */
static _sdtx_context_t* _sdtx_context_at(uint32_t ctx_id) {
    SOKOL_ASSERT(SG_INVALID_ID != ctx_id);
    int slot_index = _sdtx_slot_index(ctx_id);
    SOKOL_ASSERT((slot_index > _SDTX_INVALID_SLOT_INDEX) && (slot_index < _sdtx.context_pool.pool.size));
    return &_sdtx.context_pool.contexts[slot_index];
}

/* get context pointer with id-check, returns 0 if no match */
static _sdtx_context_t* _sdtx_lookup_context(uint32_t ctx_id) {
    if (SG_INVALID_ID != ctx_id) {
        _sdtx_context_t* ctx = _sdtx_context_at(ctx_id);
        if (ctx->slot.id == ctx_id) {
            return ctx;
        }
    }
    return 0;
}

static sdtx_context _sdtx_alloc_context(void) {
    sdtx_context hnd;
    int slot_index = _sdtx_pool_alloc_index(&_sdtx.context_pool.pool);
    if (_SDTX_INVALID_SLOT_INDEX != slot_index) {
        hnd.id = _sdtx_slot_alloc(&_sdtx.context_pool.pool, &_sdtx.context_pool.contexts[slot_index].slot, slot_index);
    }
    else {
        /* pool is exhausted */
        hnd.id = SG_INVALID_ID;
    }
    return hnd;
}

static void _sdtx_init_context(sdtx_context ctx_id, const sdtx_context_desc_t* desc) {
    SOKOL_ASSERT((ctx_id.id != SG_INVALID_ID) && desc);
    // FIXME
    SOKOL_ASSERT(false);
}

static sdtx_context _sdtx_make_context(const sdtx_context_desc_t* desc) {
    SOKOL_ASSERT(desc);
    sdtx_context ctx_id = _sdtx_alloc_context();
    if (ctx_id.id != SG_INVALID_ID) {
        _sdtx_init_context(ctx_id, desc);
    }
    else {
        SOKOL_LOG("sokol_debugtext.h: context pool exhausted!");
    }
    return ctx_id;
}

static void _sdtx_destroy_context(sdtx_context ctx_id) {
    _sdtx_context_t* ctx = _sdtx_lookup_context(ctx_id.id);
    if (ctx) {
        // FIXME
        SOKOL_ASSERT(false);
    }
}

#endif /* SOKOL_DEBUGTEXT_IMPL */
