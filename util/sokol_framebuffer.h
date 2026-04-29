#if defined(SOKOL_IMPL) && !defined(SOKOL_FRAMEBUFFER_IMPL)
#define SOKOL_FRAMEBUFFER_IMPL
#endif
#ifndef SOKOL_FRAMEBUFFER_INCLUDED
/*
    sokol_framebuffer.h -- pixel framebuffer for CPU rendering

    Project URL: https://github.com/floooh/sokol

    Optionally provide the following defines with your own implementations:

    SOKOL_ASSERT(c)     - your own assert macro (default: assert(c))
    SOKOL_FRAMEBUFFER_API_DECL - public function declaration prefix (default: extern)
    SOKOL_API_DECL      - same as SOKOL_FRAMEBUFFER_API_DECL
    SOKOL_API_IMPL      - public function implementation prefix (default: -)

    If sokol_framebuffer.h is compiled as a DLL, define the following before
    including the declaration or implementation:

    SOKOL_DLL

    NOTE: the implementation is written in C99 and cannot be compiled in C++ mode,
    the declaration can be used from C++ though.

    [TODO: documentation]

    LICENSE
    =======

    zlib/libpng license

    Copyright (c) 2026 Andre Weissflog

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
#define SOKOL_FRAMEBUFFER_INCLUDED (1)
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#if !defined(SOKOL_GFX_INCLUDED)
#error "Please include sokol_gfx.h before sokol_framebuffer.h"
#endif

#if defined(SOKOL_API_DECL) && !defined(SOKOL_FRAMEBUFFER_API_DECL)
#define SOKOL_FRAMEBUFFER_API_DECL SOKOL_API_DECL
#endif
#ifndef SOKOL_FRAMEBUFFER_API_DECL
#if defined(_WIN32) && defined(SOKOL_DLL) && defined(SOKOL_FRAMEBUFFER_IMPL)
#define SOKOL_FRAMEBUFFER_API_DECL __declspec(dllexport)
#elif defined(_WIN32) && defined(SOKOL_DLL)
#define SOKOL_FRAMEBUFFER_API_DECL __declspec(dllimport)
#else
#define SOKOL_FRAMEBUFFER_API_DECL extern
#endif
#endif

#ifdef __cplusplus
extern "C" {
#endif

enum {
    SFB_INVALID_ID = 0,
};

/*
    sfb_framebuffer

    A framebuffer handle, created with sfb_make_framebuffer(), destroyed
    with sfb_destroy_framebuffer()
*/
typedef struct sfb_framebuffer { uint32_t id; } sfb_framebuffer;

/*
    sfb_resource_state

    TODO: docs
*/
typedef enum sfb_resource_state {
    SFB_RESOURCESTATE_INITIAL,
    SFB_RESOURCESTATE_ALLOC,
    SFB_RESOURCESTATE_VALID,
    SFB_RESOURCESTATE_FAILED,
    SFB_RESOURCESTATE_INVALID,
    _SFB_RESOURCESTATE_FORCE_U32 = 0x7FFFFFFF
} sfb_resource_state;

/*
    sfb_format

    TODO: doc
*/
typedef enum sfb_format {
    _SFB_FORMAT_DEFAULT = 0,
    SFB_FORMAT_RGBA8,
    SFB_FORMAT_PALETTE8,
    _SFB_FORMAT_FORCE_U32 = 0x7FFFFFFF
} sfb_format;

/*
    sfb_orientation

    TODO: doc
*/
typedef enum sfb_orientation {
    _SFB_ORIENTATION_DEFAULT = 0,
    SFB_ORIENTATION_LANDSCAPE,
    SFB_ORIENTATION_PORTRAIT,
    _SFB_ORIENTATION_FORCE_U32 = 0x7FFFFFFF
} sfb_orientation;

/*
    sfb_rect

    TODO doc:
*/
typedef struct sfb_rect {
    int x;
    int y;
    int width;
    int height;
} sfb_rect;

/*
    sfb_pass_desc

    TODO: doc
*/
typedef struct sfb_render_pass_desc {
    sg_pixel_format color_format;
    sg_pixel_format depth_format;
    int sample_count;
} sfb_render_pass_desc;

/*
    sfb_framebuffer_desc

    TODO: doc
*/
typedef struct sfb_framebuffer_desc {
    int width;                      // width in pixels, must be provided
    int height;                     // height in pixels, must be provided
    int prescale;                   // bilinear-prefiltered prescale factor, default: 1
    sfb_format format;              // framebuffer pixel format, default: SFB_FORMAT_RGBA8
    sfb_orientation orientation;    // framebuffer orientation, default: SFB_ORIENTATION_LANDSCAPE
    sfb_render_pass_desc render_pass;   // pixel formats and sample count of sokol-gfx render pass (defaults: use sokol-gfx defaults)
} sfb_framebuffer_desc;

/*
    sfb_range

    TODO: doc
*/
typedef struct sfb_range {
    void* ptr;
    size_t size;
} sfb_range;

/*
    sfb_update_desc

    TODO: doc
*/
typedef struct sfb_update_desc {
    sfb_range pixels;
    sfb_range palette;
} sfb_update_desc;

/*
    sfb_render_overrides

    TODO doc
*/
typedef struct sfb_render_overrides {
    sg_pipeline pip;
    sg_bindings bindings;
    sg_range uniforms[SG_MAX_UNIFORMBLOCK_BINDSLOTS];
} sfb_render_overrides;

/*
    sfb_render_desc

    TODO: doc
*/
typedef struct sfb_render_desc {
    sfb_rect cliprect;  // clip rectangle in pixels, e.g. visible area inside framebuffer, default: [0, 0, width, height]
    sfb_render_overrides overrides; // for plugging in CRT shaders etc...
} sfb_render_desc;

/*
    sfb_framebuffer_info

    TODO doc
*/
typedef struct sfb_framebuffer_info {
    sg_image update_image;
    sg_view update_texture_view;
    sg_image prescale_image;            // only valid when sfb_framebuffer_desc.prescale > 1
    sg_view prescale_texture_view;      // only valid when sfb_framebuffer_desc.prescale > 1
    sg_image palette_image;             // only valid when sfb_framebuffer_desc.format == SFB_FORMAT_PALETTE8
    sg_view palette_texture_view;       // only valid when sfb_framebuffer_desc.format == SFB_FORMAT_PALETTE8
} sfb_framebuffer_info;

/*
    sfb_allocator

    Used in sfb_desc to provide custom memory-alloc and -free functions
    to sokol_framebuffer.h. If memory management should be overridden, both the
    alloc and free function must be provided (e.g. it's not valid to
    override one function but not the other).
*/
typedef struct sfb_allocator {
    void* (*alloc_fn)(size_t size, void* user_data);
    void (*free_fn)(void* ptr, void* user_data);
    void* user_data;
} sfb_allocator;

/*
    sfb_logger

    Used in sfb_desc to provide a custom logging and error reporting
    callback to sokol_framebuffer.h.
*/
typedef struct sfb_logger {
    void (*func)(
        const char* tag,                // always "sfb"
        uint32_t log_level,             // 0=panic, 1=error, 2=warning, 3=info
        uint32_t log_item_id,           // SFB_LOGITEM_*
        const char* message_or_null,    // a message string, may be nullptr in release mode
        uint32_t line_nr,               // line number in sokol_gl.h
        const char* filename_or_null,   // source filename, may be nullptr in release mode
        void* user_data);
    void* user_data;
} sfb_logger;

/*
    TODO doc
*/
typedef struct sfb_desc {
    int framebuffer_pool_size;      // default: 8
    sfb_allocator allocator;
    sfb_logger logger;
} sfb_desc;

// setup sokol-framebuffer
SOKOL_FRAMEBUFFER_API_DECL void sfb_setup(const sfb_desc* desc);
// shutdown sokol-framebuffer
SOKOL_FRAMEBUFFER_API_DECL void sfb_shutdown(void);

// create a framebuffer object
SOKOL_FRAMEBUFFER_API_DECL sfb_framebuffer sfb_make_framebuffer(const sfb_framebuffer_desc* desc);
// destroy framebuffer object
SOKOL_FRAMEBUFFER_API_DECL void sfb_destroy_framebuffer(sfb_framebuffer fb);
// query framebuffer resource state (valid or failed)
SOKOL_FRAMEBUFFER_API_DECL sfb_resource_state sfb_query_framebuffer_state(sfb_framebuffer fb);
// update framebuffer and/or color palette content (must be called outside any sokol-gfx pass)
SOKOL_FRAMEBUFFER_API_DECL void sfb_update(sfb_framebuffer fb, const sfb_update_desc* desc);
// draw framebuffer content (must be called inside a sokol-gfx render pass)
SOKOL_FRAMEBUFFER_API_DECL void sfb_render(sfb_framebuffer fb, const sfb_render_desc* desc);

// query current framebuffer properties
SOKOL_FRAMEBUFFER_API_DECL sfb_framebuffer_info sfb_query_framebuffer_info(sfb_framebuffer fb);
// helper function to create packed RGBA8 uint32_t from float r, g, b, a (0.0 .. 1.0)
SOKOL_FRAMEBUFFER_API_DECL uint32_t sfb_color_f32(float r, float g, float b, float a);
// helper function to create packed RGBA8 uint32_t from uint8_t (0 .. 255)
SOKOL_FRAMEBUFFER_API_DECL uint32_t sfb_color_u8(uint8_t r, uint8_t g, uint8_t b, uint8_t a);

// FIXME: C++ overloads

#ifdef __cplusplus
} // extern "C"
#endif
#endif // SOKOL_FRAMEBUFFER_INCLUDED

/*=== IMPLEMENTATION =========================================================*/
#ifdef SOKOL_FRAMEBUFFER_IMPL
#define SOKOL_FRAMEBUFFER_IMPL_INCLUDED (1)

#include <stdlib.h> // malloc, free, abort
#include <string.h> // memset

#ifndef SOKOL_API_IMPL
    #define SOKOL_API_IMPL
#endif
#ifndef SOKOL_DEBUG
    #ifndef NDEBUG
        #define SOKOL_DEBUG
    #endif
#endif
#ifndef SOKOL_ASSERT
    #include <assert.h>
    #define SOKOL_ASSERT(c) assert(c)
#endif

#define _sfb_def(val, def) (((val) == 0) ? (def) : (val))

// >>structs
enum {
    _SFB_SLOT_SHIFT = 16,
    _SFB_SLOT_MASK = (1<<_SFB_SLOT_SHIFT)-1,
    _SFB_MAX_POOL_SIZE = (1<<_SFB_SLOT_SHIFT),
    _SFB_DEFAULT_FRAMEBUFFER_POOL_SIZE = 8,
};

#define _SFB_INVALID_SLOT_INDEX (0)

typedef struct {
    uint32_t id;
    sfb_resource_state state;
} _sfb_slot_t;

typedef struct {
    _sfb_slot_t slot;
    sfb_framebuffer_desc desc;
    struct {
        sg_image img;
        sg_view tex_view;
    } update;
    struct {
        sg_image img;
        sg_view tex_view;
        sg_view att_view;
    } prescale;
    struct {
        sg_image img;
        sg_view tex_view;
    } palette;
} _sfb_framebuffer_t;

// resource pool housekeeping struct
typedef struct {
    int size;
    int queue_top;
    uint32_t* gen_ctrs;
    int* free_queue;
} _sfb_pool_t;

#define _SFB_INVALID_SLOT_INDEX (0)
typedef struct {
    _sfb_pool_t framebuffer_pool;
    _sfb_framebuffer_t* framebuffers;
} _sfb_pools_t;

typedef struct {
    uint32_t init_tag;
    sfb_desc desc;
    _sfb_pools_t pools;
} _sfb_state_t;
static _sfb_state_t _sfb;

// >>logging
#define _SFB_LOG_ITEMS \
    _SFB_LOGITEM_XMACRO(OK, "Ok") \
    _SFB_LOGITEM_XMACRO(MALLOC_FAILED, "memory allocation failed") \
    _SFB_LOGITEM_XMACRO(FRAMEBUFFER_POOL_EXHAUSTED, "framebuffer pool exhausted (sfb_desc.framebuffer_pool_size)") \
    _SFB_LOGITEM_XMACRO(INVALID_FRAMEBUFFER_WIDTH, "sfb_framebuffer_desc.width must be > 0") \
    _SFB_LOGITEM_XMACRO(INVALID_FRAMEBUFFER_HEIGHT, "sfb_framebuffer_desc.height must be > 0") \

#define _SFB_LOGITEM_XMACRO(item,msg) _SFB_LOGITEM_##item,
typedef enum {
    _SFB_LOG_ITEMS
} _sfb_log_item_t;
#undef _SFB_LOGITEM_XMACRO

#if defined(SOKOL_DEBUG)
#define _SFB_LOGITEM_XMACRO(item,msg) #item ": " msg,
static const char* _sfb_log_messages[] = {
    _SFB_LOG_ITEMS
};
#undef _SFB_LOGITEM_XMACRO
#endif // SOKOL_DEBUG

#define _SFB_PANIC(code) _sfb_log(_SFB_LOGITEM_ ##code, 0, 0, __LINE__)
#define _SFB_ERROR(code) _sfb_log(_SFB_LOGITEM_ ##code, 1, 0, __LINE__)
#define _SFB_WARN(code) _sfb_log(_SFB_LOGITEM_ ##code, 2, 0, __LINE__)
#define _SFB_INFO(code) _sfb_log(_SFB_LOGITEM_ ##code, 3, 0, __LINE__)
#define _SFB_LOGMSG(code,msg) _sfb_log(_SFB_LOGITEM_ ##code, 3, msg, __LINE__)

static void _sfb_log(_sfb_log_item_t log_item, uint32_t log_level, const char* msg, uint32_t line_nr) {
    if (_sfb.desc.logger.func) {
        const char* filename = 0;
        #if defined(SOKOL_DEBUG)
            filename = __FILE__;
            if (0 == msg) {
                msg = _sfb_log_messages[log_item];
            }
        #endif
        _sfb.desc.logger.func("sfb", log_level, (uint32_t)log_item, msg, line_nr, filename, _sfb.desc.logger.user_data);
    } else {
        // for log level PANIC it would be 'undefined behaviour' to continue
        if (log_level == 0) {
            abort();
        }
    }
}

// >>memory
static void _sfb_clear(void* ptr, size_t size) {
    SOKOL_ASSERT(ptr && (size > 0));
    memset(ptr, 0, size);
}

static void* _sfb_malloc(size_t size) {
    SOKOL_ASSERT(size > 0);
    void* ptr;
    if (_sfb.desc.allocator.alloc_fn) {
        ptr = _sfb.desc.allocator.alloc_fn(size, _sfb.desc.allocator.user_data);
    } else {
        ptr = malloc(size);
    }
    if (0 == ptr) {
        _SFB_PANIC(MALLOC_FAILED);
    }
    return ptr;
}

static void* _sfb_malloc_clear(size_t size) {
    void* ptr = _sfb_malloc(size);
    _sfb_clear(ptr, size);
    return ptr;
}

static void _sfb_free(void* ptr) {
    if (_sfb.desc.allocator.free_fn) {
        _sfb.desc.allocator.free_fn(ptr, _sfb.desc.allocator.user_data);
    } else {
        free(ptr);
    }
}

// >>pool
static void _sfb_pool_init(_sfb_pool_t* pool, int num) {
    SOKOL_ASSERT(pool && (num >= 1));
    // slot 0 is reserved for the 'invalid id', so bump the pool size by 1
    pool->size = num + 1;
    pool->queue_top = 0;
    // generation counters indexable by pool slot index, slot 0 is reserved
    size_t gen_ctrs_size = sizeof(uint32_t) * (size_t)pool->size;
    pool->gen_ctrs = (uint32_t*)_sfb_malloc_clear(gen_ctrs_size);
    // it's not a bug to only reserve 'num' here
    pool->free_queue = (int*) _sfb_malloc_clear(sizeof(int) * (size_t)num);
    // never allocate the zero-th pool item since the invalid id is 0
    for (int i = pool->size-1; i >= 1; i--) {
        pool->free_queue[pool->queue_top++] = i;
    }
}

static void _sfb_pool_discard(_sfb_pool_t* pool) {
    SOKOL_ASSERT(pool);
    SOKOL_ASSERT(pool->free_queue);
    _sfb_free(pool->free_queue);
    pool->free_queue = 0;
    SOKOL_ASSERT(pool->gen_ctrs);
    _sfb_free(pool->gen_ctrs);
    pool->gen_ctrs = 0;
    pool->size = 0;
    pool->queue_top = 0;
}

static int _sfb_pool_alloc_index(_sfb_pool_t* pool) {
    SOKOL_ASSERT(pool);
    SOKOL_ASSERT(pool->free_queue);
    if (pool->queue_top > 0) {
        int slot_index = pool->free_queue[--pool->queue_top];
        SOKOL_ASSERT((slot_index > 0) && (slot_index < pool->size));
        return slot_index;
    } else {
        // pool exhausted
        return _SFB_INVALID_SLOT_INDEX;
    }
}

static void _sfb_pool_free_index(_sfb_pool_t* pool, int slot_index) {
    SOKOL_ASSERT((slot_index > _SFB_INVALID_SLOT_INDEX) && (slot_index < pool->size));
    SOKOL_ASSERT(pool);
    SOKOL_ASSERT(pool->free_queue);
    SOKOL_ASSERT(pool->queue_top < pool->size);
    #ifdef SOKOL_DEBUG
    // debug check against double-free
    for (int i = 0; i < pool->queue_top; i++) {
        SOKOL_ASSERT(pool->free_queue[i] != slot_index);
    }
    #endif
    pool->free_queue[pool->queue_top++] = slot_index;
    SOKOL_ASSERT(pool->queue_top <= (pool->size-1));
}

static void _sfb_setup_pools(_sfb_pools_t* p, const sfb_desc* desc) {
    SOKOL_ASSERT(p);
    SOKOL_ASSERT(desc);
    // note: the pools here will have an additional item, since slot 0 is reserved
    SOKOL_ASSERT((desc->framebuffer_pool_size > 0) && (desc->framebuffer_pool_size < _SFB_MAX_POOL_SIZE));
    _sfb_pool_init(&p->framebuffer_pool, desc->framebuffer_pool_size);
    size_t fb_pool_byte_size = sizeof(_sfb_framebuffer_t) * (size_t)p->framebuffer_pool.size;
    p->framebuffers = (_sfb_framebuffer_t*) _sfb_malloc_clear(fb_pool_byte_size);
}

static void _sfb_discard_pools(_sfb_pools_t* p) {
    SOKOL_ASSERT(p);
    _sfb_free(p->framebuffers); p->framebuffers = 0;
    _sfb_pool_discard(&p->framebuffer_pool);
}

/* allocate the slot at slot_index:
    - bump the slot's generation counter
    - create a resource id from the generation counter and slot index
    - set the slot's id to this id
    - set the slot's state to ALLOC
    - return the resource id
*/
static uint32_t _sfb_slot_alloc(_sfb_pool_t* pool, _sfb_slot_t* slot, int slot_index) {
    /* FIXME: add handling for an overflowing generation counter,
       for now, just overflow (another option is to disable
       the slot)
    */
    SOKOL_ASSERT(pool && pool->gen_ctrs);
    SOKOL_ASSERT((slot_index > _SFB_INVALID_SLOT_INDEX) && (slot_index < pool->size));
    SOKOL_ASSERT(slot->id == SFB_INVALID_ID);
    SOKOL_ASSERT(slot->state == SFB_RESOURCESTATE_INITIAL);
    uint32_t ctr = ++pool->gen_ctrs[slot_index];
    slot->id = (ctr<<_SFB_SLOT_SHIFT)|(slot_index & _SFB_SLOT_MASK);
    slot->state = SFB_RESOURCESTATE_ALLOC;
    return slot->id;
}

// extract slot index from id
static int _sfb_slot_index(uint32_t id) {
    int slot_index = (int) (id & _SFB_SLOT_MASK);
    SOKOL_ASSERT(_SFB_INVALID_SLOT_INDEX != slot_index);
    return slot_index;
}

// returns pointer to resource by id without matching id check
static _sfb_framebuffer_t* _sfb_framebuffer_at(uint32_t fb_id) {
    SOKOL_ASSERT(SFB_INVALID_ID != fb_id);
    int slot_index = _sfb_slot_index(fb_id);
    SOKOL_ASSERT((slot_index > _SFB_INVALID_SLOT_INDEX) && (slot_index < _sfb.pools.framebuffer_pool.size));
    return &_sfb.pools.framebuffers[slot_index];
}

// returns pointer to resource with matching id check, may return 0
static _sfb_framebuffer_t* _sfb_lookup_framebuffer(uint32_t fb_id) {
    if (SFB_INVALID_ID != fb_id) {
        _sfb_framebuffer_t* fb = _sfb_framebuffer_at(fb_id);
        if (fb->slot.id == fb_id) {
            return fb;
        }
    }
    return 0;
}

static sfb_framebuffer _sfb_alloc_framebuffer(void) {
    sfb_framebuffer res;
    int slot_index = _sfb_pool_alloc_index(&_sfb.pools.framebuffer_pool);
    if (_SFB_INVALID_SLOT_INDEX != slot_index) {
        res.id = _sfb_slot_alloc(&_sfb.pools.framebuffer_pool, &_sfb.pools.framebuffers[slot_index].slot, slot_index);
    } else {
        res.id = SFB_INVALID_ID;
        _SFB_ERROR(FRAMEBUFFER_POOL_EXHAUSTED);
    }
    return res;
}

static void _sfb_dealloc_framebuffer(_sfb_framebuffer_t* fb) {
    SOKOL_ASSERT(fb && (fb->slot.state == SFB_RESOURCESTATE_ALLOC) && (fb->slot.id != SFB_INVALID_ID));
    _sfb_pool_free_index(&_sfb.pools.framebuffer_pool, _sfb_slot_index(fb->slot.id));
    _sfb_clear(fb, sizeof(_sfb_framebuffer_t));
}

static sfb_desc _sfb_desc_defaults(const sfb_desc* desc) {
    SOKOL_ASSERT(desc);
    sfb_desc res = *desc;
    res.framebuffer_pool_size = _sfb_def(res.framebuffer_pool_size, _SFB_DEFAULT_FRAMEBUFFER_POOL_SIZE);
    return res;
}

static sfb_framebuffer_desc _sfb_framebuffer_desc_defaults(const sfb_framebuffer_desc* desc) {
    SOKOL_ASSERT(desc);
    sfb_framebuffer_desc res = *desc;
    res.prescale = _sfb_def(res.prescale, 1);
    res.format = _sfb_def(res.format, SFB_FORMAT_RGBA8);
    res.orientation = _sfb_def(res.orientation, SFB_ORIENTATION_LANDSCAPE);
    return res;
}

static void _sfb_init_framebuffer(_sfb_framebuffer_t* fb, const sfb_framebuffer_desc* desc) {
    SOKOL_ASSERT(fb && (fb->slot.state == SFB_RESOURCESTATE_ALLOC));
    SOKOL_ASSERT(desc);
    if (desc->width <= 0) {
        _SFB_ERROR(INVALID_FRAMEBUFFER_WIDTH);
        fb->slot.state = SFB_RESOURCESTATE_FAILED;
        return;
    }
    if (desc->height <= 0) {
        _SFB_ERROR(INVALID_FRAMEBUFFER_HEIGHT);
        fb->slot.state = SFB_RESOURCESTATE_FAILED;
        return;
    }
    fb->desc = *desc;

    bool valid = true;
    fb->update.img = sg_make_image(&(sg_image_desc){
        .usage.stream_update = true,
        .width = fb->desc.width,
        .height = fb->desc.height,
        .pixel_format = fb->desc.format == SFB_FORMAT_RGBA8 ? SG_PIXELFORMAT_RGBA8 : SG_PIXELFORMAT_R8,
        .label = "sfb-update-image",
    });
    valid &= sg_query_image_state(fb->update.img) == SG_RESOURCESTATE_VALID;
    fb->update.tex_view = sg_make_view(&(sg_view_desc){
        .texture.image = fb->update.img,
        .label = "sfb-update-tex-view",
    });
    valid &= sg_query_view_state(fb->update.tex_view) == SG_RESOURCESTATE_VALID;
    if (fb->desc.prescale > 1) {
        fb->prescale.img = sg_make_image(&(sg_image_desc){
            .usage.color_attachment = true,
            .width = fb->desc.width * fb->desc.prescale,
            .height = fb->desc.height * fb->desc.prescale,
            .pixel_format = SG_PIXELFORMAT_RGBA8,
            .label = "sfb-prescale-image",
        });
        valid &= sg_query_image_state(fb->prescale.img) == SG_RESOURCESTATE_VALID;
        fb->prescale.tex_view = sg_make_view(&(sg_view_desc){
            .texture.image = fb->prescale.img,
            .label = "sfb-prescale-texture-view",
        });
        valid &= sg_query_view_state(fb->prescale.tex_view) == SG_RESOURCESTATE_VALID;
        fb->prescale.att_view = sg_make_view(&(sg_view_desc){
            .color_attachment.image = fb->prescale.img,
            .label = "sfb-prescale-attachment-view",
        });
        valid &= sg_query_view_state(fb->prescale.att_view) == SG_RESOURCESTATE_VALID;
    }
    if (fb->desc.format == SFB_FORMAT_PALETTE8) {
        fb->palette.img = sg_make_image(&(sg_image_desc){
            .width = 256,
            .height = 1,
            .pixel_format = SG_PIXELFORMAT_RGBA8,
            .label = "sfb-palette-img",
        });
        valid &= sg_query_image_state(fb->palette.img) == SG_RESOURCESTATE_VALID;
        fb->palette.tex_view = sg_make_view(&(sg_view_desc){
            .texture.image = fb->palette.img,
        });
        valid &= sg_query_view_state(fb->palette.tex_view) == SG_RESOURCESTATE_VALID;
    }
    fb->slot.state = valid ? SFB_RESOURCESTATE_VALID : SFB_RESOURCESTATE_FAILED;
}

static void _sfb_uninit_framebuffer(_sfb_framebuffer_t* fb) {
    SOKOL_ASSERT(fb && (fb->slot.state == SFB_RESOURCESTATE_VALID) || (fb->slot.state == SFB_RESOURCESTATE_FAILED));
    // it's ok to call the destroy funcs with invalid id or in failed state
    sg_destroy_image(fb->update.img);
    sg_destroy_view(fb->update.tex_view);
    sg_destroy_image(fb->prescale.img);
    sg_destroy_view(fb->prescale.tex_view);
    sg_destroy_view(fb->prescale.att_view);
    sg_destroy_image(fb->palette.img);
    sg_destroy_view(fb->palette.tex_view);
    fb->slot.state = SFB_RESOURCESTATE_ALLOC;
}

static void _sfb_discard_all_resources(void) {
    for (int i = 1; i < _sfb.pools.framebuffer_pool.size; i++) {
        sfb_resource_state state = _sfb.pools.framebuffers[i].slot.state;
        if ((state == SFB_RESOURCESTATE_VALID) || (state == SFB_RESOURCESTATE_FAILED)) {
            _sfb_uninit_framebuffer(&_sfb.pools.framebuffers[i]);
        }
    }
}

// >>public
#define _SFB_INIT_TAG (0xDCBADCBA)

SOKOL_API_IMPL void sfb_setup(const sfb_desc* desc) {
    SOKOL_ASSERT(desc);
    SOKOL_ASSERT((desc->allocator.alloc_fn && desc->allocator.free_fn) || (!desc->allocator.alloc_fn && !desc->allocator.free_fn));
    _sfb_clear(&_sfb, sizeof(_sfb));
    _sfb.init_tag = _SFB_INIT_TAG;
    _sfb.desc = _sfb_desc_defaults(desc);
    _sfb_setup_pools(&_sfb.pools, &_sfb.desc);
}

SOKOL_API_IMPL void sfb_shutdown(void) {
    SOKOL_ASSERT(_SFB_INIT_TAG == _sfb.init_tag);
    _sfb_discard_all_resources();
    _sfb_discard_pools(&_sfb.pools);
    _sfb_clear(&_sfb, sizeof(_sfb));
}

SOKOL_API_IMPL sfb_framebuffer sfb_make_framebuffer(const sfb_framebuffer_desc* desc) {
    SOKOL_ASSERT(_SFB_INIT_TAG == _sfb.init_tag);
    SOKOL_ASSERT(desc);
    sfb_framebuffer_desc desc_def = _sfb_framebuffer_desc_defaults(desc);
    sfb_framebuffer fb_id = _sfb_alloc_framebuffer();
    if (fb_id.id != SFB_INVALID_ID) {
        _sfb_framebuffer_t* fb = _sfb_framebuffer_at(fb_id.id);
        SOKOL_ASSERT(fb && (fb->slot.state == SFB_RESOURCESTATE_ALLOC));
        _sfb_init_framebuffer(fb, &desc_def);
        SOKOL_ASSERT((fb->slot.state == SFB_RESOURCESTATE_VALID) || (fb->slot.state == SFB_RESOURCESTATE_FAILED));
    }
    return fb_id;
}

SOKOL_API_IMPL void sfb_destroy_framebuffer(sfb_framebuffer fb_id) {
    SOKOL_ASSERT(_SFB_INIT_TAG == _sfb.init_tag);
    _sfb_framebuffer_t* fb = _sfb_lookup_framebuffer(fb_id.id);
    if (fb) {
        if ((fb->slot.state == SFB_RESOURCESTATE_VALID) || (fb->slot.state == SFB_RESOURCESTATE_FAILED)) {
            _sfb_uninit_framebuffer(fb);
            SOKOL_ASSERT(fb->slot.state == SFB_RESOURCESTATE_ALLOC);
        }
        if (fb->slot.state == SFB_RESOURCESTATE_ALLOC) {
            _sfb_dealloc_framebuffer(fb);
            SOKOL_ASSERT(fb->slot.state == SFB_RESOURCESTATE_INITIAL);
        }
    }
}

SOKOL_API_IMPL sfb_resource_state sfb_query_framebuffer_state(sfb_framebuffer fb_id) {
    SOKOL_ASSERT(_SFB_INIT_TAG == _sfb.init_tag);
    _sfb_framebuffer_t* fb = _sfb_lookup_framebuffer(fb_id.id);
    return fb ? fb->slot.state : SFB_RESOURCESTATE_INVALID;
}

#endif // SOKOL_FRAMEBUFFER_IMPL
