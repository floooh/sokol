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

/*
    sfb_framebuffer

    A framebuffer handle, created with sg_make_framebuffer(), destroyed
    with sg_destroy_framebuffer()
*/
typedef struct sfb_framebuffer { uint32_t id; } sfb_framebuffer;

/*
    sfb_format

    TODO: doc
*/
typedef enum sfb_format {
    _SFB_FORMAT_DEFAULT = 0,
    SFB_FORMAT_RGBA8,
    SFB_FORMAT_PALETTE8,
    _SG_FORMAT_FORCE_U32 = 0x7FFFFFFF
} sfb_format;

/*
    sfb_orientation

    TODO: doc
*/
typedef enum sfb_orientation {
    _SFB_ORIENTATION_DEFAULT = 0,
    SFB_ORIENTATION_LANDSCAPE,
    SFB_ORIENTATION_PORTRAIT,
    _SG_ORIENTATION_FORCE_U32 = 0x7FFFFFFF
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
    sfb_render_desc

    TODO: doc
    FIXME: draw filter/shader
*/
typedef struct sfb_render_desc {
    sfb_rect cliprect;  // clip rectangle in pixels, e.g. visible area inside framebuffer, default: [0, 0, width, height]
    // FIXME: CRT shader and shader params
} sfb_render_desc;

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
typedef struct sfb_logger_t {
    void (*func)(
        const char* tag,                // always "sfb"
        uint32_t log_level,             // 0=panic, 1=error, 2=warning, 3=info
        uint32_t log_item_id,           // SFB_LOGITEM_*
        const char* message_or_null,    // a message string, may be nullptr in release mode
        uint32_t line_nr,               // line number in sokol_gl.h
        const char* filename_or_null,   // source filename, may be nullptr in release mode
        void* user_data);
    void* user_data;
} sfb_logger_t;

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
SOKOL_FRAMEBUFFER_API_DECL void sfb_framebuffer sfb_destroy_framebuffer(sfb_framebuffer fb);
// resize framebuffer textures, note: discards previous content!
SOKOL_FRAMEBUFFER_API_DECL void sfb_resize(int new_width, int new_height);
// update framebuffer and/or color palette content (must be called outside any sokol-gfx pass)
SOKOL_FRAMEBUFFER_API_DECL sfb_update(sfb_framebuffer fb, const sfb_update_desc* desc);
// draw framebuffer content (must be called inside a sokol-gfx render pass)
SOKOL_FRAMEBUFFER_API_DECL sfb_render(sfb_framebuffer fb, const sfb_render_desc* desc);

// query current framebuffer properties
SOKOL_FRAMEBUFFER_API_DECL sfb_framebuffer_info sfb_query_framebuffer_info(sfb_framebuffer fb);
// helper function to create packed RGBA8 uint32_t from float r, g, b, a (0.0 .. 1.0)
SOKOL_FRAMEBUFFER_API_DECL uint32_t sfb_color_f32(float r, float g, float b, float a);
// helper function to create packed RGBA8 uint32_t from uint8_t (0 .. 255)
SOKOL_FRAMEBUFFER_API_DECL uint32_t sfb_color_u8(uint8_t r, uint8_t g, uint8_t b, uint8_t a);

#ifdef __cplusplus
} // extern "C"
#endif
#endif // SOKOL_FRAMEBUFFER_INCLUDED

/*=== IMPLEMENTATION =========================================================*/
#ifdef SOKOL_FRAMEBUFFER_IMPL
#define SOKOL_FRAMEBUFFER_IMPL_INCLUDED (1)

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

// FIXME

#endif // SOKOL_FRAMEBUFFER_IMPL
