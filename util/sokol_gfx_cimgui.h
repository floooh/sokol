#pragma once
/*
    sokol_gfx_cimgui.h -- debug-inspection UI for sokol_gfx.h using Dear ImGui

    Do this:
        #define SOKOL_GFX_CIMGUI_IMPL
    before you include this file in *one* C++ file to create the
    implementation.

    NOTE that the implementation must be compiled as C++ or Objective-C++
    because it calls into the ImGui C++ API. The sokol_gfx_imgui.h API
    itself is plain C though.

    Include the following file(s) before including sokol_gfx_imgui.h:

        sokol_gfx.h

    Additionally, include the following files(s) before including
    the implementation of sokol_gfx_imgui.h:

        imgui.h

    The sokol_gfx.h implementation must be compiled with debug trace hooks
    enabled by defining:

        SOKOL_TRACE_HOOKS

    ...before including the sokol_gfx.h implementation.

    Before including the sokol_gfx_cimgui.h implementation, optionally
    override the following macros:

        SOKOL_ASSERT(c)     -- your own assert macro, default: assert(c)
        SOKOL_UNREACHABLE   -- your own macro to annotate unreachable code,
                               default: SOKOL_ASSERT(false)
        SOKOL_MALLOC(s)     -- your own memory allocation function, default: malloc(s)
        SOKOL_FREE(p)       -- your own memory free function, default: free(p)
        SOKOL_API_DECL      - public function declaration prefix (default: extern)
        SOKOL_API_IMPL      - public function implementation prefix (default: -)

    If sokol_gfx_cimgui.h is compiled as a DLL, define the following before
    including the declaration or implementation:

    SOKOL_DLL

    On Windows, SOKOL_DLL will define SOKOL_API_DECL as __declspec(dllexport)
    or __declspec(dllimport) as needed.

    STEP BY STEP:
    =============
    --- create an sg_cimgui_t struct (which must be preserved between frames)
        and initialize it with:

            sg_cimgui_init(&sg_cimgui);

    --- somewhere in the per-frame code call:

            sg_cimgui_draw(&sg_cimgui)

        this won't draw anything yet, since no windows are open.

    --- open and close windows directly by setting the following public
        booleans in the sg_cimgui_t struct:

            sg_cimgui.buffers.open = true;
            sg_cimgui.images.open = true;
            sg_cimgui.shaders.open = true;
            sg_cimgui.pipelines.open = true;
            sg_cimgui.passes.open = true;
            sg_cimgui.capture.open = true;

        ...for instance, to control the window visibility through
        menu items, the following code can be used:

            if (igBeginMainMenuBar()) {
                if (igBeginMenu("sokol-gfx")) {
                    igMenuItem("Buffers", 0, &sg_cimgui.buffers.open);
                    igMenuItem("Images", 0, &sg_cimgui.images.open);
                    igMenuItem("Shaders", 0, &sg_cimgui.shaders.open);
                    igMenuItem("Pipelines", 0, &sg_cimgui.pipelines.open);
                    igMenuItem("Passes", 0, &sg_cimgui.passes.open);
                    igMenuItem("Calls", 0, &sg_cimgui.capture.open);
                    igEndMenu();
                }
                igEndMainMenuBar();
            }

    --- before application shutdown, call:

            sg_cimgui_discard(&sg_cimgui);

        ...this is not strictly necessary because the application exits
        anyway, but not doing this may trigger memory leak detection tools.

    --- finally, your application needs an ImGui renderer, you can either
        provide your own, or drop in the sokol_imgui.h utility header

    ALTERNATIVE DRAWING FUNCTIONS:
    ==============================
    Instead of the convenient, but all-in-one sg_cimgui_draw() function,
    you can also use the following granular functions which might allow
    better integration with your existing UI:

    The following functions only render the window *content* (so you
    can integrate the UI into you own windows):

        void sg_cimgui_draw_buffers_content(sg_cimgui_t* ctx);
        void sg_cimgui_draw_images_content(sg_cimgui_t* ctx);
        void sg_cimgui_draw_shaders_content(sg_cimgui_t* ctx);
        void sg_cimgui_draw_pipelines_content(sg_cimgui_t* ctx);
        void sg_cimgui_draw_passes_content(sg_cimgui_t* ctx);
        void sg_cimgui_draw_capture_content(sg_cimgui_t* ctx);

    And these are the 'full window' drawing functions:

        void sg_cimgui_draw_buffers_window(sg_cimgui_t* ctx);
        void sg_cimgui_draw_images_window(sg_cimgui_t* ctx);
        void sg_cimgui_draw_shaders_window(sg_cimgui_t* ctx);
        void sg_cimgui_draw_pipelines_window(sg_cimgui_t* ctx);
        void sg_cimgui_draw_passes_window(sg_cimgui_t* ctx);
        void sg_cimgui_draw_capture_window(sg_cimgui_t* ctx);

    Finer-grained drawing functions may be moved to the public API
    in the future as needed.

    LICENSE
    =======
    zlib/libpng license

    Copyright (c) 2018 Andre Weissflog

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
#include <stdint.h>
#include <stdbool.h>

#if !defined(SOKOL_GFX_INCLUDED)
#error "Please include sokol_gfx.h before sokol_gfx_imgui.h"
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

#if defined(__cplusplus)
extern "C" {
#endif

#define sg_cimgui_STRBUF_LEN (96)
/* max number of captured calls per frame */
#define sg_cimgui_MAX_FRAMECAPTURE_ITEMS (4096)

typedef struct {
    char buf[sg_cimgui_STRBUF_LEN];
} sg_cimgui_str_t;

typedef struct {
    sg_buffer res_id;
    sg_cimgui_str_t label;
    sg_buffer_desc desc;
} sg_cimgui_buffer_t;

typedef struct {
    sg_image res_id;
    float ui_scale;
    sg_cimgui_str_t label;
    sg_image_desc desc;
} sg_cimgui_image_t;

typedef struct {
    sg_shader res_id;
    sg_cimgui_str_t label;
    sg_cimgui_str_t vs_entry;
    sg_cimgui_str_t vs_image_name[SG_MAX_SHADERSTAGE_IMAGES];
    sg_cimgui_str_t vs_uniform_name[SG_MAX_SHADERSTAGE_UBS][SG_MAX_UB_MEMBERS];
    sg_cimgui_str_t fs_entry;
    sg_cimgui_str_t fs_image_name[SG_MAX_SHADERSTAGE_IMAGES];
    sg_cimgui_str_t fs_uniform_name[SG_MAX_SHADERSTAGE_UBS][SG_MAX_UB_MEMBERS];
    sg_cimgui_str_t attr_name[SG_MAX_VERTEX_ATTRIBUTES];
    sg_cimgui_str_t attr_sem_name[SG_MAX_VERTEX_ATTRIBUTES];
    sg_shader_desc desc;
} sg_cimgui_shader_t;

typedef struct {
    sg_pipeline res_id;
    sg_cimgui_str_t label;
    sg_pipeline_desc desc;
} sg_cimgui_pipeline_t;

typedef struct {
    sg_pass res_id;
    sg_cimgui_str_t label;
    float color_image_scale[SG_MAX_COLOR_ATTACHMENTS];
    float ds_image_scale;
    sg_pass_desc desc;
} sg_cimgui_pass_t;

typedef struct {
    bool open;
    int num_slots;
    sg_buffer sel_buf;
    sg_cimgui_buffer_t* slots;
} sg_cimgui_buffers_t;

typedef struct {
    bool open;
    int num_slots;
    sg_image sel_img;
    sg_cimgui_image_t* slots;
} sg_cimgui_images_t;

typedef struct {
    bool open;
    int num_slots;
    sg_shader sel_shd;
    sg_cimgui_shader_t* slots;
} sg_cimgui_shaders_t;

typedef struct {
    bool open;
    int num_slots;
    sg_pipeline sel_pip;
    sg_cimgui_pipeline_t* slots;
} sg_cimgui_pipelines_t;

typedef struct {
    bool open;
    int num_slots;
    sg_pass sel_pass;
    sg_cimgui_pass_t* slots;
} sg_cimgui_passes_t;

typedef enum {
    sg_cimgui_CMD_INVALID,
    sg_cimgui_CMD_QUERY_FEATURE,
    sg_cimgui_CMD_RESET_STATE_CACHE,
    sg_cimgui_CMD_MAKE_BUFFER,
    sg_cimgui_CMD_MAKE_IMAGE,
    sg_cimgui_CMD_MAKE_SHADER,
    sg_cimgui_CMD_MAKE_PIPELINE,
    sg_cimgui_CMD_MAKE_PASS,
    sg_cimgui_CMD_DESTROY_BUFFER,
    sg_cimgui_CMD_DESTROY_IMAGE,
    sg_cimgui_CMD_DESTROY_SHADER,
    sg_cimgui_CMD_DESTROY_PIPELINE,
    sg_cimgui_CMD_DESTROY_PASS,
    sg_cimgui_CMD_UPDATE_BUFFER,
    sg_cimgui_CMD_UPDATE_IMAGE,
    sg_cimgui_CMD_APPEND_BUFFER,
    sg_cimgui_CMD_BEGIN_DEFAULT_PASS,
    sg_cimgui_CMD_BEGIN_PASS,
    sg_cimgui_CMD_APPLY_VIEWPORT,
    sg_cimgui_CMD_APPLY_SCISSOR_RECT,
    sg_cimgui_CMD_APPLY_PIPELINE,
    sg_cimgui_CMD_APPLY_BINDINGS,
    sg_cimgui_CMD_APPLY_UNIFORMS,
    sg_cimgui_CMD_DRAW,
    sg_cimgui_CMD_END_PASS,
    sg_cimgui_CMD_COMMIT,
    sg_cimgui_CMD_ALLOC_BUFFER,
    sg_cimgui_CMD_ALLOC_IMAGE,
    sg_cimgui_CMD_ALLOC_SHADER,
    sg_cimgui_CMD_ALLOC_PIPELINE,
    sg_cimgui_CMD_ALLOC_PASS,
    sg_cimgui_CMD_INIT_BUFFER,
    sg_cimgui_CMD_INIT_IMAGE,
    sg_cimgui_CMD_INIT_SHADER,
    sg_cimgui_CMD_INIT_PIPELINE,
    sg_cimgui_CMD_INIT_PASS,
    sg_cimgui_CMD_FAIL_BUFFER,
    sg_cimgui_CMD_FAIL_IMAGE,
    sg_cimgui_CMD_FAIL_SHADER,
    sg_cimgui_CMD_FAIL_PIPELINE,
    sg_cimgui_CMD_FAIL_PASS,
    sg_cimgui_CMD_PUSH_DEBUG_GROUP,
    sg_cimgui_CMD_POP_DEBUG_GROUP,
    sg_cimgui_CMD_ERR_BUFFER_POOL_EXHAUSTED,
    sg_cimgui_CMD_ERR_IMAGE_POOL_EXHAUSTED,
    sg_cimgui_CMD_ERR_SHADER_POOL_EXHAUSTED,
    sg_cimgui_CMD_ERR_PIPELINE_POOL_EXHAUSTED,
    sg_cimgui_CMD_ERR_PASS_POOL_EXHAUSTED,
    sg_cimgui_CMD_ERR_CONTEXT_MISMATCH,
    sg_cimgui_CMD_ERR_PASS_INVALID,
    sg_cimgui_CMD_ERR_DRAW_INVALID,
    sg_cimgui_CMD_ERR_BINDINGS_INVALID,
} sg_cimgui_cmd_t;

typedef struct {
    sg_feature feature;
    bool result;
} sg_cimgui_args_query_feature_t;

typedef struct {
    sg_buffer result;
} sg_cimgui_args_make_buffer_t;

typedef struct {
    sg_image result;
} sg_cimgui_args_make_image_t;

typedef struct {
    sg_shader result;
} sg_cimgui_args_make_shader_t;

typedef struct {
    sg_pipeline result;
} sg_cimgui_args_make_pipeline_t;

typedef struct {
    sg_pass result;
} sg_cimgui_args_make_pass_t;

typedef struct {
    sg_buffer buffer;
} sg_cimgui_args_destroy_buffer_t;

typedef struct {
    sg_image image;
} sg_cimgui_args_destroy_image_t;

typedef struct {
    sg_shader shader;
} sg_cimgui_args_destroy_shader_t;

typedef struct {
    sg_pipeline pipeline;
} sg_cimgui_args_destroy_pipeline_t;

typedef struct {
    sg_pass pass;
} sg_cimgui_args_destroy_pass_t;

typedef struct {
    sg_buffer buffer;
    int data_size;
} sg_cimgui_args_update_buffer_t;

typedef struct {
    sg_image image;
} sg_cimgui_args_update_image_t;

typedef struct {
    sg_buffer buffer;
    int data_size;
    int result;
} sg_cimgui_args_append_buffer_t;

typedef struct {
    sg_pass_action action;
    int width;
    int height;
} sg_cimgui_args_begin_default_pass_t;

typedef struct {
    sg_pass pass;
    sg_pass_action action;
} sg_cimgui_args_begin_pass_t;

typedef struct {
    int x, y, width, height;
    bool origin_top_left;
} sg_cimgui_args_apply_viewport_t;

typedef struct {
    int x, y, width, height;
    bool origin_top_left;
} sg_cimgui_args_apply_scissor_rect_t;

typedef struct {
    sg_pipeline pipeline;
} sg_cimgui_args_apply_pipeline_t;

typedef struct {
    sg_bindings bindings;
} sg_cimgui_args_apply_bindings_t;

typedef struct {
    sg_shader_stage stage;
    int ub_index;
    const void* data;
    int num_bytes;
    sg_pipeline pipeline;   /* the pipeline which was active at this call */
    uint32_t ubuf_pos;      /* start of copied data in capture buffer */
} sg_cimgui_args_apply_uniforms_t;

typedef struct {
    int base_element;
    int num_elements;
    int num_instances;
} sg_cimgui_args_draw_t;

typedef struct {
    sg_buffer result;
} sg_cimgui_args_alloc_buffer_t;

typedef struct {
    sg_image result;
} sg_cimgui_args_alloc_image_t;

typedef struct {
    sg_shader result;
} sg_cimgui_args_alloc_shader_t;

typedef struct {
    sg_pipeline result;
} sg_cimgui_args_alloc_pipeline_t;

typedef struct {
    sg_pass result;
} sg_cimgui_args_alloc_pass_t;

typedef struct {
    sg_buffer buffer;
} sg_cimgui_args_init_buffer_t;

typedef struct {
    sg_image image;
} sg_cimgui_args_init_image_t;

typedef struct {
    sg_shader shader;
} sg_cimgui_args_init_shader_t;

typedef struct {
    sg_pipeline pipeline;
} sg_cimgui_args_init_pipeline_t;

typedef struct {
    sg_pass pass;
} sg_cimgui_args_init_pass_t;

typedef struct {
    sg_buffer buffer;
} sg_cimgui_args_fail_buffer_t;

typedef struct {
    sg_image image;
} sg_cimgui_args_fail_image_t;

typedef struct {
    sg_shader shader;
} sg_cimgui_args_fail_shader_t;

typedef struct {
    sg_pipeline pipeline;
} sg_cimgui_args_fail_pipeline_t;

typedef struct {
    sg_pass pass;
} sg_cimgui_args_fail_pass_t;

typedef struct {
    sg_cimgui_str_t name;
} sg_cimgui_args_push_debug_group_t;

typedef union {
    sg_cimgui_args_query_feature_t query_feature;
    sg_cimgui_args_make_buffer_t make_buffer;
    sg_cimgui_args_make_image_t make_image;
    sg_cimgui_args_make_shader_t make_shader;
    sg_cimgui_args_make_pipeline_t make_pipeline;
    sg_cimgui_args_make_pass_t make_pass;
    sg_cimgui_args_destroy_buffer_t destroy_buffer;
    sg_cimgui_args_destroy_image_t destroy_image;
    sg_cimgui_args_destroy_shader_t destroy_shader;
    sg_cimgui_args_destroy_pipeline_t destroy_pipeline;
    sg_cimgui_args_destroy_pass_t destroy_pass;
    sg_cimgui_args_update_buffer_t update_buffer;
    sg_cimgui_args_update_image_t update_image;
    sg_cimgui_args_append_buffer_t append_buffer;
    sg_cimgui_args_begin_default_pass_t begin_default_pass;
    sg_cimgui_args_begin_pass_t begin_pass;
    sg_cimgui_args_apply_viewport_t apply_viewport;
    sg_cimgui_args_apply_scissor_rect_t apply_scissor_rect;
    sg_cimgui_args_apply_pipeline_t apply_pipeline;
    sg_cimgui_args_apply_bindings_t apply_bindings;
    sg_cimgui_args_apply_uniforms_t apply_uniforms;
    sg_cimgui_args_draw_t draw;
    sg_cimgui_args_alloc_buffer_t alloc_buffer;
    sg_cimgui_args_alloc_image_t alloc_image;
    sg_cimgui_args_alloc_shader_t alloc_shader;
    sg_cimgui_args_alloc_pipeline_t alloc_pipeline;
    sg_cimgui_args_alloc_pass_t alloc_pass;
    sg_cimgui_args_init_buffer_t init_buffer;
    sg_cimgui_args_init_image_t init_image;
    sg_cimgui_args_init_shader_t init_shader;
    sg_cimgui_args_init_pipeline_t init_pipeline;
    sg_cimgui_args_init_pass_t init_pass;
    sg_cimgui_args_fail_buffer_t fail_buffer;
    sg_cimgui_args_fail_image_t fail_image;
    sg_cimgui_args_fail_shader_t fail_shader;
    sg_cimgui_args_fail_pipeline_t fail_pipeline;
    sg_cimgui_args_fail_pass_t fail_pass;
    sg_cimgui_args_push_debug_group_t push_debug_group;
} sg_cimgui_args_t;

typedef struct {
    sg_cimgui_cmd_t cmd;
    uint32_t color;
    sg_cimgui_args_t args;
} sg_cimgui_capture_item_t;

typedef struct {
    uint32_t ubuf_size;     /* size of uniform capture buffer in bytes */
    uint32_t ubuf_pos;      /* current uniform buffer pos */
    uint8_t* ubuf;          /* buffer for capturing uniform updates */
    uint32_t num_items;
    sg_cimgui_capture_item_t items[sg_cimgui_MAX_FRAMECAPTURE_ITEMS];
} sg_cimgui_capture_bucket_t;

/* double-buffered call-capture buckets, one bucket is currently recorded,
   the previous bucket is displayed
*/
typedef struct {
    bool open;
    uint32_t bucket_index;      /* which bucket to record to, 0 or 1 */
    uint32_t sel_item;          /* currently selected capture item by index */
    sg_cimgui_capture_bucket_t bucket[2];
} sg_cimgui_capture_t;

typedef struct {
    uint32_t init_tag;
    sg_cimgui_buffers_t buffers;
    sg_cimgui_images_t images;
    sg_cimgui_shaders_t shaders;
    sg_cimgui_pipelines_t pipelines;
    sg_cimgui_passes_t passes;
    sg_cimgui_capture_t capture;
    sg_pipeline cur_pipeline;
    sg_trace_hooks hooks;
} sg_cimgui_t;

SOKOL_API_DECL void sg_cimgui_init(sg_cimgui_t* ctx);
SOKOL_API_DECL void sg_cimgui_discard(sg_cimgui_t* ctx);
SOKOL_API_DECL void sg_cimgui_draw(sg_cimgui_t* ctx);

SOKOL_API_DECL void sg_cimgui_draw_buffers_content(sg_cimgui_t* ctx);
SOKOL_API_DECL void sg_cimgui_draw_images_content(sg_cimgui_t* ctx);
SOKOL_API_DECL void sg_cimgui_draw_shaders_content(sg_cimgui_t* ctx);
SOKOL_API_DECL void sg_cimgui_draw_pipelines_content(sg_cimgui_t* ctx);
SOKOL_API_DECL void sg_cimgui_draw_passes_content(sg_cimgui_t* ctx);
SOKOL_API_DECL void sg_cimgui_draw_capture_content(sg_cimgui_t* ctx);

SOKOL_API_DECL void sg_cimgui_draw_buffers_window(sg_cimgui_t* ctx);
SOKOL_API_DECL void sg_cimgui_draw_images_window(sg_cimgui_t* ctx);
SOKOL_API_DECL void sg_cimgui_draw_shaders_window(sg_cimgui_t* ctx);
SOKOL_API_DECL void sg_cimgui_draw_pipelines_window(sg_cimgui_t* ctx);
SOKOL_API_DECL void sg_cimgui_draw_passes_window(sg_cimgui_t* ctx);
SOKOL_API_DECL void sg_cimgui_draw_capture_window(sg_cimgui_t* ctx);

#if defined(__cplusplus)
} /* extern "C" */
#endif

/*=== IMPLEMENTATION =========================================================*/
#if defined SOKOL_GFX_CIMGUI_IMPL
#if !defined(CIMGUI_DEFINE_ENUMS_AND_STRUCTS)
#error "Please include cimgui.h before the sokol_gfx_cimgui.h implementation"
#endif
#ifndef SOKOL_ASSERT
    #include <assert.h>
    #define SOKOL_ASSERT(c) assert(c)
#endif
#ifndef SOKOL_UNREACHABLE
    #define SOKOL_UNREACHABLE SOKOL_ASSERT(false)
#endif
#ifndef SOKOL_MALLOC
    #include <stdlib.h>
    #define SOKOL_MALLOC(s) malloc(s)
    #define SOKOL_FREE(p) free(p)
#endif
#ifndef _SOKOL_PRIVATE
    #if defined(__GNUC__)
        #define _SOKOL_PRIVATE __attribute__((unused)) static
    #else
        #define _SOKOL_PRIVATE static
    #endif
#endif
#ifndef SOKOL_API_IMPL
#define SOKOL_API_IMPL
#endif

#include <string.h>
#include <stdio.h>      /* snprintf */

#define IM_COL32_R_SHIFT    0
#define IM_COL32_G_SHIFT    8
#define IM_COL32_B_SHIFT    16
#define IM_COL32_A_SHIFT    24
#define IM_COL32(R,G,B,A)    (((uint32_t)(A)<<IM_COL32_A_SHIFT) | ((uint32_t)(B)<<IM_COL32_B_SHIFT) | ((uint32_t)(G)<<IM_COL32_G_SHIFT) | ((uint32_t)(R)<<IM_COL32_R_SHIFT))
#define _sg_cimgui_SLOT_MASK (0xFFFF)
#define _sg_cimgui_LIST_WIDTH (192)
#define _sg_cimgui_COLOR_OTHER IM_COL32(191, 191, 191, 255)
#define _sg_cimgui_COLOR_RSRC IM_COL32(255, 255, 0, 255)
#define _sg_cimgui_COLOR_DRAW IM_COL32(0, 255, 0, 255)
#define _sg_cimgui_COLOR_ERR IM_COL32(255, 128, 0.5f, 255)
#define _sg_cimgui_VEC2_ZERO (ImVec2){0.0f, 0.0f}

/*--- UTILS ------------------------------------------------------------------*/
_SOKOL_PRIVATE int _sg_cimgui_slot_index(uint32_t id) {
    int slot_index = (int) (id & _sg_cimgui_SLOT_MASK);
    SOKOL_ASSERT(0 != slot_index);
    return slot_index;
}

_SOKOL_PRIVATE int _sg_cimgui_uniform_size(sg_uniform_type type, int count) {
    switch (type) {
        case SG_UNIFORMTYPE_INVALID:    return 0;
        case SG_UNIFORMTYPE_FLOAT:      return 4 * count;
        case SG_UNIFORMTYPE_FLOAT2:     return 8 * count;
        case SG_UNIFORMTYPE_FLOAT3:     return 12 * count; /* FIXME: std140??? */
        case SG_UNIFORMTYPE_FLOAT4:     return 16 * count;
        case SG_UNIFORMTYPE_MAT4:       return 64 * count;
        default:
            SOKOL_UNREACHABLE;
            return -1;
    }
}

_SOKOL_PRIVATE void* _sg_cimgui_alloc(int size) {
    SOKOL_ASSERT(size > 0);
    return SOKOL_MALLOC(size);
}

_SOKOL_PRIVATE void _sg_cimgui_free(void* ptr) {
    if (ptr) {
        SOKOL_FREE(ptr);
    }
}

_SOKOL_PRIVATE void* _sg_cimgui_realloc(void* old_ptr, int old_size, int new_size) {
    SOKOL_ASSERT((new_size > 0) && (new_size > old_size));
    void* new_ptr = SOKOL_MALLOC(new_size);
    SOKOL_ASSERT(new_ptr);
    if (old_ptr) {
        if (old_size > 0) {
            memcpy(new_ptr, old_ptr, old_size);
        }
        _sg_cimgui_free(old_ptr);
    }
    return new_ptr;
}

_SOKOL_PRIVATE void _sg_cimgui_strcpy(sg_cimgui_str_t* dst, const char* src) {
    SOKOL_ASSERT(dst);
    if (src) {
        #if defined(_MSC_VER)
        strncpy_s(dst->buf, sg_cimgui_STRBUF_LEN, src, (sg_cimgui_STRBUF_LEN-1));
        #else
        strncpy(dst->buf, src, sg_cimgui_STRBUF_LEN);
        #endif
        dst->buf[sg_cimgui_STRBUF_LEN-1] = 0;
    }
    else {
        memset(dst->buf, 0, sg_cimgui_STRBUF_LEN);
    }
}

_SOKOL_PRIVATE sg_cimgui_str_t _sg_cimgui_make_str(const char* str) {
    sg_cimgui_str_t res;
    _sg_cimgui_strcpy(&res, str);
    return res;
}

_SOKOL_PRIVATE const char* _sg_cimgui_str_dup(const char* src) {
    SOKOL_ASSERT(src);
    int len = (int) strlen(src) + 1;
    char* dst = (char*) _sg_cimgui_alloc(len);
    memcpy(dst, src, len);
    return (const char*) dst;
}

_SOKOL_PRIVATE const uint8_t* _sg_cimgui_bin_dup(const uint8_t* src, int num_bytes) {
    SOKOL_ASSERT(src && (num_bytes > 0));
    uint8_t* dst = (uint8_t*) _sg_cimgui_alloc(num_bytes);
    memcpy(dst, src, num_bytes);
    return (const uint8_t*) dst;
}

_SOKOL_PRIVATE void _sg_cimgui_snprintf(sg_cimgui_str_t* dst, const char* fmt, ...) {
    SOKOL_ASSERT(dst);
    va_list args;
    va_start(args, fmt);
    vsnprintf(dst->buf, sizeof(dst->buf), fmt, args);
    dst->buf[sizeof(dst->buf)-1] = 0;
    va_end(args);
}

/*--- STRING CONVERSION ------------------------------------------------------*/
_SOKOL_PRIVATE const char* _sg_cimgui_feature_string(sg_feature f) {
    switch (f) {
        case SG_FEATURE_INSTANCING:                 return "SG_FEATURE_INSTANCING";
        case SG_FEATURE_TEXTURE_COMPRESSION_DXT:    return "SG_FEATURE_TEXTURE_COMPRESSION_DXT";
        case SG_FEATURE_TEXTURE_COMPRESSION_PVRTC:  return "SG_FEATURE_TEXTURE_COMPRESSION_PVRTC";
        case SG_FEATURE_TEXTURE_COMPRESSION_ATC:    return "SG_FEATURE_TEXTURE_COMPRESSION_ATC";
        case SG_FEATURE_TEXTURE_COMPRESSION_ETC2:   return "SG_FEATURE_TEXTURE_COMPRESSION_ETC2";
        case SG_FEATURE_TEXTURE_FLOAT:              return "SG_FEATURE_TEXTURE_FLOAT";
        case SG_FEATURE_TEXTURE_HALF_FLOAT:         return "SG_FEATURE_TEXTURE_HALF_FLOAT";
        case SG_FEATURE_ORIGIN_BOTTOM_LEFT:         return "SG_FEATURE_ORIGIN_BOTTOM_LEFT";
        case SG_FEATURE_ORIGIN_TOP_LEFT:            return "SG_FEATURE_ORIGIN_TOP_LEFT";
        case SG_FEATURE_MSAA_RENDER_TARGETS:        return "SG_FEATURE_MSAA_RENDER_TARGETS";
        case SG_FEATURE_PACKED_VERTEX_FORMAT_10_2:  return "SG_FEATURE_PACKED_VERTEX_FORMAT_10_2";
        case SG_FEATURE_MULTIPLE_RENDER_TARGET:     return "SG_FEATURE_MULTIPLE_RENDER_TARGET";
        case SG_FEATURE_IMAGETYPE_3D:               return "SG_FEATURE_IMAGETYPE_3D";
        case SG_FEATURE_IMAGETYPE_ARRAY:            return "SG_FEATURE_IMAGETYPE_ARRAY";
        default:                                    return "???";
    }
}

_SOKOL_PRIVATE const char* _sg_cimgui_resourcestate_string(sg_resource_state s) {
    switch (s) {
        case SG_RESOURCESTATE_INITIAL:  return "SG_RESOURCESTATE_INITIAL";
        case SG_RESOURCESTATE_ALLOC:    return "SG_RESOURCESTATE_ALLOC";
        case SG_RESOURCESTATE_VALID:    return "SG_RESOURCESTATE_VALID";
        case SG_RESOURCESTATE_FAILED:   return "SG_RESOURCESTATE_FAILED";
        default:                        return "SG_RESOURCESTATE_INVALID";
    }
}

_SOKOL_PRIVATE void _sg_cimgui_draw_resource_slot(const sg_slot_info* slot) {
    igText("ResId: %08X", slot->res_id);
    igText("CtxId: %08X", slot->ctx_id);
    igText("State: %s", _sg_cimgui_resourcestate_string(slot->state));
}

_SOKOL_PRIVATE const char* _sg_cimgui_buffertype_string(sg_buffer_type t) {
    switch (t) {
        case SG_BUFFERTYPE_VERTEXBUFFER:    return "SG_BUFFERTYPE_VERTEXBUFFER";
        case SG_BUFFERTYPE_INDEXBUFFER:     return "SG_BUFFERTYPE_INDEXBUFFER";
        default:                            return "???";
    }
}

_SOKOL_PRIVATE const char* _sg_cimgui_usage_string(sg_usage u) {
    switch (u) {
        case SG_USAGE_IMMUTABLE:    return "SG_USAGE_IMMUTABLE";
        case SG_USAGE_DYNAMIC:      return "SG_USAGE_DYNAMIC";
        case SG_USAGE_STREAM:       return "SG_USAGE_STREAM";
        default:                    return "???";
    }
}

_SOKOL_PRIVATE const char* _sg_cimgui_imagetype_string(sg_image_type t) {
    switch (t) {
        case SG_IMAGETYPE_2D:       return "SG_IMAGETYPE_2D";
        case SG_IMAGETYPE_CUBE:     return "SG_IMAGETYPE_CUBE";
        case SG_IMAGETYPE_3D:       return "SG_IMAGETYPE_3D";
        case SG_IMAGETYPE_ARRAY:    return "SG_IMAGETYPE_ARRAY";
        default:                    return "???";
    }
}

_SOKOL_PRIVATE const char* _sg_cimgui_pixelformat_string(sg_pixel_format fmt) {
    switch (fmt) {
        case SG_PIXELFORMAT_NONE:           return "SG_PIXELFORMAT_NONE";
        case SG_PIXELFORMAT_RGBA8:          return "SG_PIXELFORMAT_RGBA8";
        case SG_PIXELFORMAT_RGB8:           return "SG_PIXELFORMAT_RGB8";
        case SG_PIXELFORMAT_RGBA4:          return "SG_PIXELFORMAT_RGBA4";
        case SG_PIXELFORMAT_R5G6B5:         return "SG_PIXELFORMAT_R5G6B5";
        case SG_PIXELFORMAT_R5G5B5A1:       return "SG_PIXELFORMAT_R5G5B5A1";
        case SG_PIXELFORMAT_R10G10B10A2:    return "SG_PIXELFORMAT_R10G10B10A2";
        case SG_PIXELFORMAT_RGBA32F:        return "SG_PIXELFORMAT_RGBA32F";
        case SG_PIXELFORMAT_RGBA16F:        return "SG_PIXELFORMAT_RGBA16F";
        case SG_PIXELFORMAT_R32F:           return "SG_PIXELFORMAT_R32F";
        case SG_PIXELFORMAT_R16F:           return "SG_PIXELFORMAT_R16F";
        case SG_PIXELFORMAT_L8:             return "SG_PIXELFORMAT_L8";
        case SG_PIXELFORMAT_DXT1:           return "SG_PIXELFORMAT_DXT1";
        case SG_PIXELFORMAT_DXT3:           return "SG_PIXELFORMAT_DXT3";
        case SG_PIXELFORMAT_DXT5:           return "SG_PIXELFORMAT_DXT5";
        case SG_PIXELFORMAT_DEPTH:          return "SG_PIXELFORMAT_DEPTH";
        case SG_PIXELFORMAT_DEPTHSTENCIL:   return "SG_PIXELFORMAT_DEPTHSTENCIL";
        case SG_PIXELFORMAT_PVRTC2_RGB:     return "SG_PIXELFORMAT_PVRTC2_RGB";
        case SG_PIXELFORMAT_PVRTC4_RGB:     return "SG_PIXELFORMAT_PVRTC4_RGB";
        case SG_PIXELFORMAT_PVRTC2_RGBA:    return "SG_PIXELFORMAT_PVRTC2_RGBA";
        case SG_PIXELFORMAT_PVRTC4_RGBA:    return "SG_PIXELFORMAT_PVRTC4_RGBA";
        case SG_PIXELFORMAT_ETC2_RGB8:      return "SG_PIXELFORMAT_ETC2_RGB8";
        case SG_PIXELFORMAT_ETC2_SRGB8:     return "SG_PIXELFORMAT_ETC2_SRGB8";
        default:                            return "???";
    }
}

_SOKOL_PRIVATE const char* _sg_cimgui_filter_string(sg_filter f) {
    switch (f) {
        case SG_FILTER_NEAREST:                 return "SG_FILTER_NEAREST";
        case SG_FILTER_LINEAR:                  return "SG_FILTER_LINEAR";
        case SG_FILTER_NEAREST_MIPMAP_NEAREST:  return "SG_FILTER_NEAREST_MIPMAP_NEAREST";
        case SG_FILTER_NEAREST_MIPMAP_LINEAR:   return "SG_FILTER_NEAREST_MIPMAP_LINEAR";
        case SG_FILTER_LINEAR_MIPMAP_NEAREST:   return "SG_FILTER_LINEAR_MIPMAP_NEAREST";
        case SG_FILTER_LINEAR_MIPMAP_LINEAR:    return "SG_FILTER_LINEAR_MIPMAP_LINEAR";
        default:                                return "???";
    }
}

_SOKOL_PRIVATE const char* _sg_cimgui_wrap_string(sg_wrap w) {
    switch (w) {
        case SG_WRAP_REPEAT:            return "SG_WRAP_REPEAT";
        case SG_WRAP_CLAMP_TO_EDGE:     return "SG_WRAP_CLAMP_TO_EDGE";
        case SG_WRAP_MIRRORED_REPEAT:   return "SG_WRAP_MIRRORED_REPEAT";
        default:                        return "???";
    }
}

_SOKOL_PRIVATE const char* _sg_cimgui_uniformtype_string(sg_uniform_type t) {
    switch (t) {
        case SG_UNIFORMTYPE_FLOAT:  return "SG_UNIFORMTYPE_FLOAT";
        case SG_UNIFORMTYPE_FLOAT2: return "SG_UNIFORMTYPE_FLOAT2";
        case SG_UNIFORMTYPE_FLOAT3: return "SG_UNIFORMTYPE_FLOAT3";
        case SG_UNIFORMTYPE_FLOAT4: return "SG_UNIFORMTYPE_FLOAT4";
        case SG_UNIFORMTYPE_MAT4:   return "SG_UNIFORMTYPE_MAT4";
        default:                    return "???";
    }
}

_SOKOL_PRIVATE const char* _sg_cimgui_vertexstep_string(sg_vertex_step s) {
    switch (s) {
        case SG_VERTEXSTEP_PER_VERTEX:      return "SG_VERTEXSTEP_PER_VERTEX";
        case SG_VERTEXSTEP_PER_INSTANCE:    return "SG_VERTEXSTEP_PER_INSTANCE";
        default:                            return "???";
    }
}

_SOKOL_PRIVATE const char* _sg_cimgui_vertexformat_string(sg_vertex_format f) {
    switch (f) {
        case SG_VERTEXFORMAT_FLOAT:     return "SG_VERTEXFORMAT_FLOAT";
        case SG_VERTEXFORMAT_FLOAT2:    return "SG_VERTEXFORMAT_FLOAT2";
        case SG_VERTEXFORMAT_FLOAT3:    return "SG_VERTEXFORMAT_FLOAT3";
        case SG_VERTEXFORMAT_FLOAT4:    return "SG_VERTEXFORMAT_FLOAT4";
        case SG_VERTEXFORMAT_BYTE4:     return "SG_VERTEXFORMAT_BYTE4";
        case SG_VERTEXFORMAT_BYTE4N:    return "SG_VERTEXFORMAT_BYTE4N";
        case SG_VERTEXFORMAT_UBYTE4:    return "SG_VERTEXFORMAT_UBYTE4";
        case SG_VERTEXFORMAT_UBYTE4N:   return "SG_VERTEXFORMAT_UBYTE4N";
        case SG_VERTEXFORMAT_SHORT2:    return "SG_VERTEXFORMAT_SHORT2";
        case SG_VERTEXFORMAT_SHORT2N:   return "SG_VERTEXFORMAT_SHORT2N";
        case SG_VERTEXFORMAT_SHORT4:    return "SG_VERTEXFORMAT_SHORT4";
        case SG_VERTEXFORMAT_SHORT4N:   return "SG_VERTEXFORMAT_SHORT4N";
        case SG_VERTEXFORMAT_UINT10_N2: return "SG_VERTEXFORMAT_UINT10_N2";
        default:                        return "???";
    }
}

_SOKOL_PRIVATE const char* _sg_cimgui_primitivetype_string(sg_primitive_type t) {
    switch (t) {
        case SG_PRIMITIVETYPE_POINTS:           return "SG_PRIMITIVETYPE_POINTS";
        case SG_PRIMITIVETYPE_LINES:            return "SG_PRIMITIVETYPE_LINES";
        case SG_PRIMITIVETYPE_LINE_STRIP:       return "SG_PRIMITIVETYPE_LINE_STRIP";
        case SG_PRIMITIVETYPE_TRIANGLES:        return "SG_PRIMITIVETYPE_TRIANGLES";
        case SG_PRIMITIVETYPE_TRIANGLE_STRIP:   return "SG_PRIMITIVETYPE_TRIANGLE_STRIP";
        default:                                return "???";
    }
}

_SOKOL_PRIVATE const char* _sg_cimgui_indextype_string(sg_index_type t) {
    switch (t) {
        case SG_INDEXTYPE_NONE:     return "SG_INDEXTYPE_NONE";
        case SG_INDEXTYPE_UINT16:   return "SG_INDEXTYPE_UINT16";
        case SG_INDEXTYPE_UINT32:   return "SG_INDEXTYPE_UINT32";
        default:                    return "???";
    }
}

_SOKOL_PRIVATE const char* _sg_cimgui_stencilop_string(sg_stencil_op op) {
    switch (op) {
        case SG_STENCILOP_KEEP:         return "SG_STENCILOP_KEEP";
        case SG_STENCILOP_ZERO:         return "SG_STENCILOP_ZERO";
        case SG_STENCILOP_REPLACE:      return "SG_STENCILOP_REPLACE";
        case SG_STENCILOP_INCR_CLAMP:   return "SG_STENCILOP_INCR_CLAMP";
        case SG_STENCILOP_DECR_CLAMP:   return "SG_STENCILOP_DECR_CLAMP";
        case SG_STENCILOP_INVERT:       return "SG_STENCILOP_INVERT";
        case SG_STENCILOP_INCR_WRAP:    return "SG_STENCILOP_INCR_WRAP";
        case SG_STENCILOP_DECR_WRAP:    return "SG_STENCILOP_DECR_WRAP";
        default:                        return "???";
    }
}

_SOKOL_PRIVATE const char* _sg_cimgui_comparefunc_string(sg_compare_func f) {
    switch (f) {
        case SG_COMPAREFUNC_NEVER:          return "SG_COMPAREFUNC_NEVER";
        case SG_COMPAREFUNC_LESS:           return "SG_COMPAREFUNC_LESS";
        case SG_COMPAREFUNC_EQUAL:          return "SG_COMPAREFUNC_EQUAL";
        case SG_COMPAREFUNC_LESS_EQUAL:     return "SG_COMPAREFUNC_LESS_EQUAL";
        case SG_COMPAREFUNC_GREATER:        return "SG_COMPAREFUNC_GREATER";
        case SG_COMPAREFUNC_NOT_EQUAL:      return "SG_COMPAREFUNC_NOT_EQUAL";
        case SG_COMPAREFUNC_GREATER_EQUAL:  return "SG_COMPAREFUNC_GREATER_EQUAL";
        case SG_COMPAREFUNC_ALWAYS:         return "SG_COMPAREFUNC_ALWAYS";
        default:                            return "???";
    }
}

_SOKOL_PRIVATE const char* _sg_cimgui_blendfactor_string(sg_blend_factor f) {
    switch (f) {
        case SG_BLENDFACTOR_ZERO:                   return "SG_BLENDFACTOR_ZERO";
        case SG_BLENDFACTOR_ONE:                    return "SG_BLENDFACTOR_ONE";
        case SG_BLENDFACTOR_SRC_COLOR:              return "SG_BLENDFACTOR_SRC_COLOR";
        case SG_BLENDFACTOR_ONE_MINUS_SRC_COLOR:    return "SG_BLENDFACTOR_ONE_MINUS_SRC_COLOR";
        case SG_BLENDFACTOR_SRC_ALPHA:              return "SG_BLENDFACTOR_SRC_ALPHA";
        case SG_BLENDFACTOR_ONE_MINUS_SRC_ALPHA:    return "SG_BLENDFACTOR_ONE_MINUS_SRC_ALPHA";
        case SG_BLENDFACTOR_DST_COLOR:              return "SG_BLENDFACTOR_DST_COLOR";
        case SG_BLENDFACTOR_ONE_MINUS_DST_COLOR:    return "SG_BLENDFACTOR_ONE_MINUS_DST_COLOR";
        case SG_BLENDFACTOR_DST_ALPHA:              return "SG_BLENDFACTOR_DST_ALPHA";
        case SG_BLENDFACTOR_ONE_MINUS_DST_ALPHA:    return "SG_BLENDFACTOR_ONE_MINUS_DST_ALPHA";
        case SG_BLENDFACTOR_SRC_ALPHA_SATURATED:    return "SG_BLENDFACTOR_SRC_ALPHA_SATURATED";
        case SG_BLENDFACTOR_BLEND_COLOR:            return "SG_BLENDFACTOR_BLEND_COLOR";
        case SG_BLENDFACTOR_ONE_MINUS_BLEND_COLOR:  return "SG_BLENDFACTOR_ONE_MINUS_BLEND_COLOR";
        case SG_BLENDFACTOR_BLEND_ALPHA:            return "SG_BLENDFACTOR_BLEND_ALPHA";
        case SG_BLENDFACTOR_ONE_MINUS_BLEND_ALPHA:  return "SG_BLENDFACTOR_ONE_MINUS_BLEND_ALPHA";
        default:                                    return "???";
    }
}

_SOKOL_PRIVATE const char* _sg_cimgui_blendop_string(sg_blend_op op) {
    switch (op) {
        case SG_BLENDOP_ADD:                return "SG_BLENDOP_ADD";
        case SG_BLENDOP_SUBTRACT:           return "SG_BLENDOP_SUBTRACT";
        case SG_BLENDOP_REVERSE_SUBTRACT:   return "SG_BLENDOP_REVERSE_SUBTRACT";
        default:                            return "???";
    }
}

_SOKOL_PRIVATE const char* _sg_cimgui_colormask_string(uint8_t m) {
    static const char* str[] = {
        "NONE",
        "R",
        "G",
        "RG",
        "B",
        "RB",
        "GB",
        "RGB",
        "A",
        "RA",
        "GA",
        "RGA",
        "BA",
        "RBA",
        "GBA",
        "RGBA",
    };
    return str[m & 0xF];
}

_SOKOL_PRIVATE const char* _sg_cimgui_cullmode_string(sg_cull_mode cm) {
    switch (cm) {
        case SG_CULLMODE_NONE:  return "SG_CULLMODE_NONE";
        case SG_CULLMODE_FRONT: return "SG_CULLMODE_FRONT";
        case SG_CULLMODE_BACK:  return "SG_CULLMODE_BACK";
        default:                return "???";
    }
}

_SOKOL_PRIVATE const char* _sg_cimgui_facewinding_string(sg_face_winding fw) {
    switch (fw) {
        case SG_FACEWINDING_CCW:    return "SG_FACEWINDING_CCW";
        case SG_FACEWINDING_CW:     return "SG_FACEWINDING_CW";
        default:                    return "???";
    }
}

_SOKOL_PRIVATE const char* _sg_cimgui_shaderstage_string(sg_shader_stage stage) {
    switch (stage) {
        case SG_SHADERSTAGE_VS:     return "SG_SHADERSTAGE_VS";
        case SG_SHADERSTAGE_FS:     return "SG_SHADERSTAGE_FS";
        default:                    return "???";
    }
}

_SOKOL_PRIVATE const char* _sg_cimgui_bool_string(bool b) {
    return b ? "true" : "false";
}

_SOKOL_PRIVATE sg_cimgui_str_t _sg_cimgui_res_id_string(uint32_t res_id, const char* label) {
    SOKOL_ASSERT(label);
    sg_cimgui_str_t res;
    if (label[0]) {
        _sg_cimgui_snprintf(&res, "'%s'", label);
    }
    else {
        _sg_cimgui_snprintf(&res, "0x%08X", res_id);
    }
    return res;
}

_SOKOL_PRIVATE sg_cimgui_str_t _sg_cimgui_buffer_id_string(sg_cimgui_t* ctx, sg_buffer buf_id) {
    if (buf_id.id != SG_INVALID_ID) {
        const sg_cimgui_buffer_t* buf_ui = &ctx->buffers.slots[_sg_cimgui_slot_index(buf_id.id)];
        return _sg_cimgui_res_id_string(buf_id.id, buf_ui->label.buf);
    }
    else {
        return _sg_cimgui_make_str("<invalid>");
    }
}

_SOKOL_PRIVATE sg_cimgui_str_t _sg_cimgui_image_id_string(sg_cimgui_t* ctx, sg_image img_id) {
    if (img_id.id != SG_INVALID_ID) {
        const sg_cimgui_image_t* img_ui = &ctx->images.slots[_sg_cimgui_slot_index(img_id.id)];
        return _sg_cimgui_res_id_string(img_id.id, img_ui->label.buf);
    }
    else {
        return _sg_cimgui_make_str("<invalid>");
    }
}

_SOKOL_PRIVATE sg_cimgui_str_t _sg_cimgui_shader_id_string(sg_cimgui_t* ctx, sg_shader shd_id) {
    if (shd_id.id != SG_INVALID_ID) {
        const sg_cimgui_shader_t* shd_ui = &ctx->shaders.slots[_sg_cimgui_slot_index(shd_id.id)];
        return _sg_cimgui_res_id_string(shd_id.id, shd_ui->label.buf);
    }
    else {
        return _sg_cimgui_make_str("<invalid>");
    }
}

_SOKOL_PRIVATE sg_cimgui_str_t _sg_cimgui_pipeline_id_string(sg_cimgui_t* ctx, sg_pipeline pip_id) {
    if (pip_id.id != SG_INVALID_ID) {
        const sg_cimgui_pipeline_t* pip_ui = &ctx->pipelines.slots[_sg_cimgui_slot_index(pip_id.id)];
        return _sg_cimgui_res_id_string(pip_id.id, pip_ui->label.buf);
    }
    else {
        return _sg_cimgui_make_str("<invalid>");
    }
}

_SOKOL_PRIVATE sg_cimgui_str_t _sg_cimgui_pass_id_string(sg_cimgui_t* ctx, sg_pass pass_id) {
    if (pass_id.id != SG_INVALID_ID) {
        const sg_cimgui_pass_t* pass_ui = &ctx->passes.slots[_sg_cimgui_slot_index(pass_id.id)];
        return _sg_cimgui_res_id_string(pass_id.id, pass_ui->label.buf);
    }
    else {
        return _sg_cimgui_make_str("<invalid>");
    }
}

/*--- RESOURCE HELPERS -------------------------------------------------------*/
_SOKOL_PRIVATE void _sg_cimgui_buffer_created(sg_cimgui_t* ctx, sg_buffer res_id, int slot_index, const sg_buffer_desc* desc) {
    SOKOL_ASSERT((slot_index > 0) && (slot_index < ctx->buffers.num_slots));
    sg_cimgui_buffer_t* buf = &ctx->buffers.slots[slot_index];
    buf->res_id = res_id;
    buf->desc = *desc;
    buf->label = _sg_cimgui_make_str(desc->label);
}

_SOKOL_PRIVATE void _sg_cimgui_buffer_destroyed(sg_cimgui_t* ctx, int slot_index) {
    SOKOL_ASSERT((slot_index > 0) && (slot_index < ctx->buffers.num_slots));
    sg_cimgui_buffer_t* buf = &ctx->buffers.slots[slot_index];
    buf->res_id.id = SG_INVALID_ID;
}

_SOKOL_PRIVATE void _sg_cimgui_image_created(sg_cimgui_t* ctx, sg_image res_id, int slot_index, const sg_image_desc* desc) {
    SOKOL_ASSERT((slot_index > 0) && (slot_index < ctx->images.num_slots));
    sg_cimgui_image_t* img = &ctx->images.slots[slot_index];
    img->res_id = res_id;
    img->desc = *desc;
    img->ui_scale = 1.0f;
    img->label = _sg_cimgui_make_str(desc->label);
}

_SOKOL_PRIVATE void _sg_cimgui_image_destroyed(sg_cimgui_t* ctx, int slot_index) {
    SOKOL_ASSERT((slot_index > 0) && (slot_index < ctx->images.num_slots));
    sg_cimgui_image_t* img = &ctx->images.slots[slot_index];
    img->res_id.id = SG_INVALID_ID;
}

_SOKOL_PRIVATE void _sg_cimgui_shader_created(sg_cimgui_t* ctx, sg_shader res_id, int slot_index, const sg_shader_desc* desc) {
    SOKOL_ASSERT((slot_index > 0) && (slot_index < ctx->shaders.num_slots));
    sg_cimgui_shader_t* shd = &ctx->shaders.slots[slot_index];
    shd->res_id = res_id;
    shd->desc = *desc;
    shd->label = _sg_cimgui_make_str(desc->label);
    if (shd->desc.vs.entry) {
        shd->vs_entry = _sg_cimgui_make_str(shd->desc.vs.entry);
        shd->desc.vs.entry = shd->vs_entry.buf;
    }
    if (shd->desc.fs.entry) {
        shd->fs_entry = _sg_cimgui_make_str(shd->desc.fs.entry);
        shd->desc.fs.entry = shd->fs_entry.buf;
    }
    for (int i = 0; i < SG_MAX_SHADERSTAGE_UBS; i++) {
        for (int j = 0; j < SG_MAX_UB_MEMBERS; j++) {
            sg_shader_uniform_desc* ud = &shd->desc.vs.uniform_blocks[i].uniforms[j];
            if (ud->name) {
                shd->vs_uniform_name[i][j] = _sg_cimgui_make_str(ud->name);
                ud->name = shd->vs_uniform_name[i][j].buf;
            }
        }
    }
    for (int i = 0; i < SG_MAX_SHADERSTAGE_UBS; i++) {
        for (int j = 0; j < SG_MAX_UB_MEMBERS; j++) {
            sg_shader_uniform_desc* ud = &shd->desc.fs.uniform_blocks[i].uniforms[j];
            if (ud->name) {
                shd->fs_uniform_name[i][j] = _sg_cimgui_make_str(ud->name);
                ud->name = shd->fs_uniform_name[i][j].buf;
            }
        }
    }
    for (int i = 0; i < SG_MAX_SHADERSTAGE_IMAGES; i++) {
        if (shd->desc.vs.images[i].name) {
            shd->vs_image_name[i] = _sg_cimgui_make_str(shd->desc.vs.images[i].name);
            shd->desc.vs.images[i].name = shd->vs_image_name[i].buf;
        }
    }
    for (int i = 0; i < SG_MAX_SHADERSTAGE_IMAGES; i++) {
        if (shd->desc.fs.images[i].name) {
            shd->fs_image_name[i] = _sg_cimgui_make_str(shd->desc.fs.images[i].name);
            shd->desc.fs.images[i].name = shd->fs_image_name[i].buf;
        }
    }
    if (shd->desc.vs.source) {
        shd->desc.vs.source = _sg_cimgui_str_dup(shd->desc.vs.source);
    }
    if (shd->desc.vs.byte_code) {
        shd->desc.vs.byte_code = _sg_cimgui_bin_dup(shd->desc.vs.byte_code, shd->desc.vs.byte_code_size);
    }
    if (shd->desc.fs.source) {
        shd->desc.fs.source = _sg_cimgui_str_dup(shd->desc.fs.source);
    }
    if (shd->desc.fs.byte_code) {
        shd->desc.fs.byte_code = _sg_cimgui_bin_dup(shd->desc.fs.byte_code, shd->desc.fs.byte_code_size);
    }
    for (int i = 0; i < SG_MAX_VERTEX_ATTRIBUTES; i++) {
        sg_shader_attr_desc* ad = &shd->desc.attrs[i];
        if (ad->name) {
            shd->attr_name[i] = _sg_cimgui_make_str(ad->name);
            ad->name = shd->attr_name[i].buf;
        }
        if (ad->sem_name) {
            shd->attr_sem_name[i] = _sg_cimgui_make_str(ad->sem_name);
            ad->sem_name = shd->attr_sem_name[i].buf;
        }
    }
}

_SOKOL_PRIVATE void _sg_cimgui_shader_destroyed(sg_cimgui_t* ctx, int slot_index) {
    SOKOL_ASSERT((slot_index > 0) && (slot_index < ctx->shaders.num_slots));
    sg_cimgui_shader_t* shd = &ctx->shaders.slots[slot_index];
    shd->res_id.id = SG_INVALID_ID;
    if (shd->desc.vs.source) {
        _sg_cimgui_free((void*)shd->desc.vs.source);
        shd->desc.vs.source = 0;
    }
    if (shd->desc.vs.byte_code) {
        _sg_cimgui_free((void*)shd->desc.vs.byte_code);
        shd->desc.vs.byte_code = 0;
    }
    if (shd->desc.fs.source) {
        _sg_cimgui_free((void*)shd->desc.fs.source);
        shd->desc.fs.source = 0;
    }
    if (shd->desc.fs.byte_code) {
        _sg_cimgui_free((void*)shd->desc.fs.byte_code);
        shd->desc.fs.byte_code = 0;
    }
}

_SOKOL_PRIVATE void _sg_cimgui_pipeline_created(sg_cimgui_t* ctx, sg_pipeline res_id, int slot_index, const sg_pipeline_desc* desc) {
    SOKOL_ASSERT((slot_index > 0) && (slot_index < ctx->pipelines.num_slots));
    sg_cimgui_pipeline_t* pip = &ctx->pipelines.slots[slot_index];
    pip->res_id = res_id;
    pip->label = _sg_cimgui_make_str(desc->label);
    pip->desc = *desc;

}

_SOKOL_PRIVATE void _sg_cimgui_pipeline_destroyed(sg_cimgui_t* ctx, int slot_index) {
    SOKOL_ASSERT((slot_index > 0) && (slot_index < ctx->pipelines.num_slots));
    sg_cimgui_pipeline_t* pip = &ctx->pipelines.slots[slot_index];
    pip->res_id.id = SG_INVALID_ID;
}

_SOKOL_PRIVATE void _sg_cimgui_pass_created(sg_cimgui_t* ctx, sg_pass res_id, int slot_index, const sg_pass_desc* desc) {
    SOKOL_ASSERT((slot_index > 0) && (slot_index < ctx->passes.num_slots));
    sg_cimgui_pass_t* pass = &ctx->passes.slots[slot_index];
    pass->res_id = res_id;
    for (int i = 0; i < SG_MAX_COLOR_ATTACHMENTS; i++) {
        pass->color_image_scale[i] = 0.25f;
    }
    pass->ds_image_scale = 0.25f;
    pass->label = _sg_cimgui_make_str(desc->label);
    pass->desc = *desc;
}

_SOKOL_PRIVATE void _sg_cimgui_pass_destroyed(sg_cimgui_t* ctx, int slot_index) {
    SOKOL_ASSERT((slot_index > 0) && (slot_index < ctx->passes.num_slots));
    sg_cimgui_pass_t* pass = &ctx->passes.slots[slot_index];
    pass->res_id.id = SG_INVALID_ID;
}

/*--- COMMAND CAPTURING ------------------------------------------------------*/
_SOKOL_PRIVATE void _sg_cimgui_capture_init(sg_cimgui_t* ctx) {
    const int ubuf_initial_size = 256 * 1024;
    for (int i = 0; i < 2; i++) {
        sg_cimgui_capture_bucket_t* bucket = &ctx->capture.bucket[i];
        bucket->ubuf_size = ubuf_initial_size;
        bucket->ubuf = (uint8_t*) _sg_cimgui_alloc(bucket->ubuf_size);
        SOKOL_ASSERT(bucket->ubuf);
    }
}

_SOKOL_PRIVATE void _sg_cimgui_capture_discard(sg_cimgui_t* ctx) {
    for (int i = 0; i < 2; i++) {
        sg_cimgui_capture_bucket_t* bucket = &ctx->capture.bucket[i];
        SOKOL_ASSERT(bucket->ubuf);
        _sg_cimgui_free(bucket->ubuf);
        bucket->ubuf = 0;
    }
}

_SOKOL_PRIVATE sg_cimgui_capture_bucket_t* _sg_cimgui_capture_get_write_bucket(sg_cimgui_t* ctx) {
    return &ctx->capture.bucket[ctx->capture.bucket_index & 1];
}

_SOKOL_PRIVATE sg_cimgui_capture_bucket_t* _sg_cimgui_capture_get_read_bucket(sg_cimgui_t* ctx) {
    return &ctx->capture.bucket[(ctx->capture.bucket_index + 1) & 1];
}

_SOKOL_PRIVATE void _sg_cimgui_capture_next_frame(sg_cimgui_t* ctx) {
    ctx->capture.bucket_index = (ctx->capture.bucket_index + 1) & 1;
    sg_cimgui_capture_bucket_t* bucket = &ctx->capture.bucket[ctx->capture.bucket_index];
    bucket->num_items = 0;
    bucket->ubuf_pos = 0;
}

_SOKOL_PRIVATE void _sg_cimgui_capture_grow_ubuf(sg_cimgui_t* ctx, uint32_t required_size) {
    sg_cimgui_capture_bucket_t* bucket = _sg_cimgui_capture_get_write_bucket(ctx);
    SOKOL_ASSERT(required_size > bucket->ubuf_size);
    int old_size = bucket->ubuf_size;
    int new_size = required_size + (required_size>>1);  /* allocate a bit ahead */
    bucket->ubuf_size = new_size;
    bucket->ubuf = (uint8_t*) _sg_cimgui_realloc(bucket->ubuf, old_size, new_size);
}

_SOKOL_PRIVATE sg_cimgui_capture_item_t* _sg_cimgui_capture_next_write_item(sg_cimgui_t* ctx) {
    sg_cimgui_capture_bucket_t* bucket = _sg_cimgui_capture_get_write_bucket(ctx);
    if (bucket->num_items < sg_cimgui_MAX_FRAMECAPTURE_ITEMS) {
        sg_cimgui_capture_item_t* item = &bucket->items[bucket->num_items++];
        return item;
    }
    else {
        return 0;
    }
}

_SOKOL_PRIVATE uint32_t _sg_cimgui_capture_num_read_items(sg_cimgui_t* ctx) {
    sg_cimgui_capture_bucket_t* bucket = _sg_cimgui_capture_get_read_bucket(ctx);
    return bucket->num_items;
}

_SOKOL_PRIVATE sg_cimgui_capture_item_t* _sg_cimgui_capture_read_item_at(sg_cimgui_t* ctx, uint32_t index) {
    sg_cimgui_capture_bucket_t* bucket = _sg_cimgui_capture_get_read_bucket(ctx);
    SOKOL_ASSERT(index < bucket->num_items);
    return &bucket->items[index];
}

_SOKOL_PRIVATE uint32_t _sg_cimgui_capture_uniforms(sg_cimgui_t* ctx, const void* data, int num_bytes) {
    sg_cimgui_capture_bucket_t* bucket = _sg_cimgui_capture_get_write_bucket(ctx);
    const uint32_t required_size = bucket->ubuf_pos + num_bytes;
    if (required_size > bucket->ubuf_size) {
        _sg_cimgui_capture_grow_ubuf(ctx, required_size);
    }
    SOKOL_ASSERT(required_size <= bucket->ubuf_size);
    memcpy(bucket->ubuf + bucket->ubuf_pos, data, num_bytes);
    const uint32_t pos = bucket->ubuf_pos;
    bucket->ubuf_pos += num_bytes;
    SOKOL_ASSERT(bucket->ubuf_pos <= bucket->ubuf_size);
    return pos;
}

_SOKOL_PRIVATE sg_cimgui_str_t _sg_cimgui_capture_item_string(sg_cimgui_t* ctx, int index, const sg_cimgui_capture_item_t* item) {
    sg_cimgui_str_t str = _sg_cimgui_make_str(0);
    sg_cimgui_str_t res_id = _sg_cimgui_make_str(0);
    switch (item->cmd) {
        case sg_cimgui_CMD_QUERY_FEATURE:
            _sg_cimgui_snprintf(&str, "%d: sg_query_feature(feature=%s) => %s",
                index,
                _sg_cimgui_feature_string(item->args.query_feature.feature),
                _sg_cimgui_bool_string(item->args.query_feature.result));
            break;

        case sg_cimgui_CMD_RESET_STATE_CACHE:
            _sg_cimgui_snprintf(&str, "%d: sg_reset_state_cache()", index);
            break;

        case sg_cimgui_CMD_MAKE_BUFFER:
            res_id = _sg_cimgui_buffer_id_string(ctx, item->args.make_buffer.result);
            _sg_cimgui_snprintf(&str, "%d: sg_make_buffer(desc=..) => %s", index, res_id.buf);
            break;

        case sg_cimgui_CMD_MAKE_IMAGE:
            res_id = _sg_cimgui_image_id_string(ctx, item->args.make_image.result);
            _sg_cimgui_snprintf(&str, "%d: sg_make_image(desc=..) => %s", index, res_id.buf);
            break;

        case sg_cimgui_CMD_MAKE_SHADER:
            res_id = _sg_cimgui_shader_id_string(ctx, item->args.make_shader.result);
            _sg_cimgui_snprintf(&str, "%d: sg_make_shader(desc=..) => %s", index, res_id.buf);
            break;

        case sg_cimgui_CMD_MAKE_PIPELINE:
            res_id = _sg_cimgui_pipeline_id_string(ctx, item->args.make_pipeline.result);
            _sg_cimgui_snprintf(&str, "%d: sg_make_pipeline(desc=..) => %s", index, res_id.buf);
            break;

        case sg_cimgui_CMD_MAKE_PASS:
            res_id = _sg_cimgui_pass_id_string(ctx, item->args.make_pass.result);
            _sg_cimgui_snprintf(&str, "%d: sg_make_pass(desc=..) => %s", index, res_id.buf);
            break;

        case sg_cimgui_CMD_DESTROY_BUFFER:
            res_id = _sg_cimgui_buffer_id_string(ctx, item->args.destroy_buffer.buffer);
            _sg_cimgui_snprintf(&str, "%d: sg_destroy_buffer(buf=%s)", index, res_id.buf);
            break;

        case sg_cimgui_CMD_DESTROY_IMAGE:
            res_id = _sg_cimgui_image_id_string(ctx, item->args.destroy_image.image);
            _sg_cimgui_snprintf(&str, "%d: sg_destroy_image(img=%s)", index, res_id.buf);
            break;

        case sg_cimgui_CMD_DESTROY_SHADER:
            res_id = _sg_cimgui_shader_id_string(ctx, item->args.destroy_shader.shader);
            _sg_cimgui_snprintf(&str, "%d: sg_destroy_shader(shd=%s)", index, res_id.buf);
            break;

        case sg_cimgui_CMD_DESTROY_PIPELINE:
            res_id = _sg_cimgui_pipeline_id_string(ctx, item->args.destroy_pipeline.pipeline);
            _sg_cimgui_snprintf(&str, "%d: sg_destroy_pipeline(pip=%s)", index, res_id.buf);
            break;

        case sg_cimgui_CMD_DESTROY_PASS:
            res_id = _sg_cimgui_pass_id_string(ctx, item->args.destroy_pass.pass);
            _sg_cimgui_snprintf(&str, "%d: sg_destroy_pass(pass=%s)", index, res_id.buf);
            break;

        case sg_cimgui_CMD_UPDATE_BUFFER:
            res_id = _sg_cimgui_buffer_id_string(ctx, item->args.update_buffer.buffer);
            _sg_cimgui_snprintf(&str, "%d: sg_update_buffer(buf=%s, data_ptr=.., data_size=%d)",
                index, res_id.buf,
                item->args.update_buffer.data_size);
            break;

        case sg_cimgui_CMD_UPDATE_IMAGE:
            res_id = _sg_cimgui_image_id_string(ctx, item->args.update_image.image);
            _sg_cimgui_snprintf(&str, "%d: sg_update_image(img=%s, data=..)", index, res_id.buf);
            break;

        case sg_cimgui_CMD_APPEND_BUFFER:
            res_id = _sg_cimgui_buffer_id_string(ctx, item->args.append_buffer.buffer);
            _sg_cimgui_snprintf(&str, "%d: sg_append_buffer(buf=%s, data_ptr=.., data_size=%d) => %d",
                index, res_id.buf,
                item->args.append_buffer.data_size,
                item->args.append_buffer.result);
            break;

        case sg_cimgui_CMD_BEGIN_DEFAULT_PASS:
            _sg_cimgui_snprintf(&str, "%d: sg_begin_default_pass(pass_action=.., width=%d, height=%d)",
                index,
                item->args.begin_default_pass.width,
                item->args.begin_default_pass.height);
            break;

        case sg_cimgui_CMD_BEGIN_PASS:
            res_id = _sg_cimgui_pass_id_string(ctx, item->args.begin_pass.pass);
            _sg_cimgui_snprintf(&str, "%d: sg_begin_pass(pass=%s, pass_action=..)", index, res_id.buf);
            break;

        case sg_cimgui_CMD_APPLY_VIEWPORT:
            _sg_cimgui_snprintf(&str, "%d: sg_apply_viewport(x=%d, y=%d, width=%d, height=%d, origin_top_left=%s)",
                index,
                item->args.apply_viewport.x,
                item->args.apply_viewport.y,
                item->args.apply_viewport.width,
                item->args.apply_viewport.height,
                _sg_cimgui_bool_string(item->args.apply_viewport.origin_top_left));
            break;

        case sg_cimgui_CMD_APPLY_SCISSOR_RECT:
            _sg_cimgui_snprintf(&str, "%d: sg_apply_scissor_rect(x=%d, y=%d, width=%d, height=%d, origin_top_left=%s)",
                index,
                item->args.apply_scissor_rect.x,
                item->args.apply_scissor_rect.y,
                item->args.apply_scissor_rect.width,
                item->args.apply_scissor_rect.height,
                _sg_cimgui_bool_string(item->args.apply_scissor_rect.origin_top_left));
            break;

        case sg_cimgui_CMD_APPLY_PIPELINE:
            res_id = _sg_cimgui_pipeline_id_string(ctx, item->args.apply_pipeline.pipeline);
            _sg_cimgui_snprintf(&str, "%d: sg_apply_pipeline(pip=%s)", index, res_id.buf);
            break;

        case sg_cimgui_CMD_APPLY_BINDINGS:
            _sg_cimgui_snprintf(&str, "%d: sg_apply_bindings(bindings=..)", index);
            break;

        case sg_cimgui_CMD_APPLY_UNIFORMS:
            _sg_cimgui_snprintf(&str, "%d: sg_apply_uniforms(stage=%s, ub_index=%d, data=.., num_bytes=%d)",
                index,
                _sg_cimgui_shaderstage_string(item->args.apply_uniforms.stage),
                item->args.apply_uniforms.ub_index,
                item->args.apply_uniforms.num_bytes);
            break;

        case sg_cimgui_CMD_DRAW:
            _sg_cimgui_snprintf(&str, "%d: sg_draw(base_element=%d, num_elements=%d, num_instances=%d)",
                index,
                item->args.draw.base_element,
                item->args.draw.num_elements,
                item->args.draw.num_instances);
            break;

        case sg_cimgui_CMD_END_PASS:
            _sg_cimgui_snprintf(&str, "%d: sg_end_pass()", index);
            break;

        case sg_cimgui_CMD_COMMIT:
            _sg_cimgui_snprintf(&str, "%d: sg_commit()", index);
            break;

        case sg_cimgui_CMD_ALLOC_BUFFER:
            res_id = _sg_cimgui_buffer_id_string(ctx, item->args.alloc_buffer.result);
            _sg_cimgui_snprintf(&str, "%d: sg_alloc_buffer() => %s", index, res_id.buf);
            break;

        case sg_cimgui_CMD_ALLOC_IMAGE:
            res_id = _sg_cimgui_image_id_string(ctx, item->args.alloc_image.result);
            _sg_cimgui_snprintf(&str, "%d: sg_alloc_image() => %s", index, res_id.buf);
            break;

        case sg_cimgui_CMD_ALLOC_SHADER:
            res_id = _sg_cimgui_shader_id_string(ctx, item->args.alloc_shader.result);
            _sg_cimgui_snprintf(&str, "%d: sg_alloc_shader() => %s", index, res_id.buf);
            break;

        case sg_cimgui_CMD_ALLOC_PIPELINE:
            res_id = _sg_cimgui_pipeline_id_string(ctx, item->args.alloc_pipeline.result);
            _sg_cimgui_snprintf(&str, "%d: sg_alloc_pipeline() => %s", index, res_id.buf);
            break;

        case sg_cimgui_CMD_ALLOC_PASS:
            res_id = _sg_cimgui_pass_id_string(ctx, item->args.alloc_pass.result);
            _sg_cimgui_snprintf(&str, "%d: sg_alloc_pass() => %s", index, res_id.buf);
            break;

        case sg_cimgui_CMD_INIT_BUFFER:
            res_id = _sg_cimgui_buffer_id_string(ctx, item->args.init_buffer.buffer);
            _sg_cimgui_snprintf(&str, "%d: sg_init_buffer(buf=%s, desc=..)", index, res_id.buf);
            break;

        case sg_cimgui_CMD_INIT_IMAGE:
            res_id = _sg_cimgui_image_id_string(ctx, item->args.init_image.image);
            _sg_cimgui_snprintf(&str, "%d: sg_init_image(img=%s, desc=..)", index, res_id.buf);
            break;

        case sg_cimgui_CMD_INIT_SHADER:
            res_id = _sg_cimgui_shader_id_string(ctx, item->args.init_shader.shader);
            _sg_cimgui_snprintf(&str, "%d: sg_init_shader(shd=%s, desc=..)", index, res_id.buf);
            break;

        case sg_cimgui_CMD_INIT_PIPELINE:
            res_id = _sg_cimgui_pipeline_id_string(ctx, item->args.init_pipeline.pipeline);
            _sg_cimgui_snprintf(&str, "%d: sg_init_pipeline(pip=%s, desc=..)", index, res_id.buf);
            break;

        case sg_cimgui_CMD_INIT_PASS:
            res_id = _sg_cimgui_pass_id_string(ctx, item->args.init_pass.pass);
            _sg_cimgui_snprintf(&str, "%d: sg_init_pass(pass=%s, desc=..)", index, res_id.buf);
            break;

        case sg_cimgui_CMD_FAIL_BUFFER:
            res_id = _sg_cimgui_buffer_id_string(ctx, item->args.fail_buffer.buffer);
            _sg_cimgui_snprintf(&str, "%d: sg_fail_buffer(buf=%s)", index, res_id.buf);
            break;

        case sg_cimgui_CMD_FAIL_IMAGE:
            res_id = _sg_cimgui_image_id_string(ctx, item->args.fail_image.image);
            _sg_cimgui_snprintf(&str, "%d: sg_fail_image(img=%s)", index, res_id.buf);
            break;

        case sg_cimgui_CMD_FAIL_SHADER:
            res_id = _sg_cimgui_shader_id_string(ctx, item->args.fail_shader.shader);
            _sg_cimgui_snprintf(&str, "%d: sg_fail_shader(shd=%s)", index, res_id.buf);
            break;

        case sg_cimgui_CMD_FAIL_PIPELINE:
            res_id = _sg_cimgui_pipeline_id_string(ctx, item->args.fail_pipeline.pipeline);
            _sg_cimgui_snprintf(&str, "%d: sg_fail_pipeline(shd=%s)", index, res_id.buf);
            break;

        case sg_cimgui_CMD_FAIL_PASS:
            res_id = _sg_cimgui_pass_id_string(ctx, item->args.fail_pass.pass);
            _sg_cimgui_snprintf(&str, "%d: sg_fail_pass(pass=%s)", index, res_id.buf);
            break;

        case sg_cimgui_CMD_PUSH_DEBUG_GROUP:
            _sg_cimgui_snprintf(&str, "%d: sg_push_debug_group(name=%s)", index,
                item->args.push_debug_group.name.buf);
            break;

        case sg_cimgui_CMD_POP_DEBUG_GROUP:
            _sg_cimgui_snprintf(&str, "%d: sg_pop_debug_group()", index);
            break;

        case sg_cimgui_CMD_ERR_BUFFER_POOL_EXHAUSTED:
            _sg_cimgui_snprintf(&str, "%d: sg_err_buffer_pool_exhausted()", index);
            break;

        case sg_cimgui_CMD_ERR_IMAGE_POOL_EXHAUSTED:
            _sg_cimgui_snprintf(&str, "%d: sg_err_image_pool_exhausted()", index);
            break;

        case sg_cimgui_CMD_ERR_SHADER_POOL_EXHAUSTED:
            _sg_cimgui_snprintf(&str, "%d: sg_err_shader_pool_exhausted()", index);
            break;

        case sg_cimgui_CMD_ERR_PIPELINE_POOL_EXHAUSTED:
            _sg_cimgui_snprintf(&str, "%d: sg_err_pipeline_pool_exhausted()", index);
            break;

        case sg_cimgui_CMD_ERR_PASS_POOL_EXHAUSTED:
            _sg_cimgui_snprintf(&str, "%d: sg_err_pass_pool_exhausted()", index);
            break;

        case sg_cimgui_CMD_ERR_CONTEXT_MISMATCH:
            _sg_cimgui_snprintf(&str, "%d: sg_err_context_mismatch()", index);
            break;

        case sg_cimgui_CMD_ERR_PASS_INVALID:
            _sg_cimgui_snprintf(&str, "%d: sg_err_pass_invalid()", index);
            break;

        case sg_cimgui_CMD_ERR_DRAW_INVALID:
            _sg_cimgui_snprintf(&str, "%d: sg_err_draw_invalid()", index);
            break;

        case sg_cimgui_CMD_ERR_BINDINGS_INVALID:
            _sg_cimgui_snprintf(&str, "%d: sg_err_bindings_invalid()", index);
            break;

        default:
            _sg_cimgui_snprintf(&str, "%d: ???", index);
            break;
    }
    return str;
}

/*--- CAPTURE CALLBACKS ------------------------------------------------------*/
_SOKOL_PRIVATE void _sg_cimgui_query_feature(sg_feature feature, bool result, void* user_data) {
    sg_cimgui_t* ctx = (sg_cimgui_t*) user_data;
    SOKOL_ASSERT(ctx);
    sg_cimgui_capture_item_t* item = _sg_cimgui_capture_next_write_item(ctx);
    if (item) {
        item->cmd = sg_cimgui_CMD_QUERY_FEATURE;
        item->color = _sg_cimgui_COLOR_OTHER;
        item->args.query_feature.feature = feature;
        item->args.query_feature.result = result;
    }
    if (ctx->hooks.query_feature) {
        ctx->hooks.query_feature(feature, result, ctx->hooks.user_data);
    }
}

_SOKOL_PRIVATE void _sg_cimgui_reset_state_cache(void* user_data) {
    sg_cimgui_t* ctx = (sg_cimgui_t*) user_data;
    SOKOL_ASSERT(ctx);
    sg_cimgui_capture_item_t* item = _sg_cimgui_capture_next_write_item(ctx);
    if (item) {
        item->cmd = sg_cimgui_CMD_RESET_STATE_CACHE;
        item->color = _sg_cimgui_COLOR_OTHER;
    }
    if (ctx->hooks.reset_state_cache) {
        ctx->hooks.reset_state_cache(ctx->hooks.user_data);
    }
}

_SOKOL_PRIVATE void _sg_cimgui_make_buffer(const sg_buffer_desc* desc, sg_buffer buf_id, void* user_data) {
    sg_cimgui_t* ctx = (sg_cimgui_t*) user_data;
    SOKOL_ASSERT(ctx);
    sg_cimgui_capture_item_t* item = _sg_cimgui_capture_next_write_item(ctx);
    if (item) {
        item->cmd = sg_cimgui_CMD_MAKE_BUFFER;
        item->color = _sg_cimgui_COLOR_RSRC;
        item->args.make_buffer.result = buf_id;
    }
    if (ctx->hooks.make_buffer) {
        ctx->hooks.make_buffer(desc, buf_id, ctx->hooks.user_data);
    }
    if (buf_id.id != SG_INVALID_ID) {
        _sg_cimgui_buffer_created(ctx, buf_id, _sg_cimgui_slot_index(buf_id.id), desc);
    }
}

_SOKOL_PRIVATE void _sg_cimgui_make_image(const sg_image_desc* desc, sg_image img_id, void* user_data) {
    sg_cimgui_t* ctx = (sg_cimgui_t*) user_data;
    SOKOL_ASSERT(ctx);
    sg_cimgui_capture_item_t* item = _sg_cimgui_capture_next_write_item(ctx);
    if (item) {
        item->cmd = sg_cimgui_CMD_MAKE_IMAGE;
        item->color = _sg_cimgui_COLOR_RSRC;
        item->args.make_image.result = img_id;
    }
    if (ctx->hooks.make_image) {
        ctx->hooks.make_image(desc, img_id, ctx->hooks.user_data);
    }
    if (img_id.id != SG_INVALID_ID) {
        _sg_cimgui_image_created(ctx, img_id, _sg_cimgui_slot_index(img_id.id), desc);
    }
}

_SOKOL_PRIVATE void _sg_cimgui_make_shader(const sg_shader_desc* desc, sg_shader shd_id, void* user_data) {
    sg_cimgui_t* ctx = (sg_cimgui_t*) user_data;
    SOKOL_ASSERT(ctx);
    sg_cimgui_capture_item_t* item = _sg_cimgui_capture_next_write_item(ctx);
    if (item) {
        item->cmd = sg_cimgui_CMD_MAKE_SHADER;
        item->color = _sg_cimgui_COLOR_RSRC;
        item->args.make_shader.result = shd_id;
    }
    if (ctx->hooks.make_shader) {
        ctx->hooks.make_shader(desc, shd_id, ctx->hooks.user_data);
    }
    if (shd_id.id != SG_INVALID_ID) {
        _sg_cimgui_shader_created(ctx, shd_id, _sg_cimgui_slot_index(shd_id.id), desc);
    }
}

_SOKOL_PRIVATE void _sg_cimgui_make_pipeline(const sg_pipeline_desc* desc, sg_pipeline pip_id, void* user_data) {
    sg_cimgui_t* ctx = (sg_cimgui_t*) user_data;
    SOKOL_ASSERT(ctx);
    sg_cimgui_capture_item_t* item = _sg_cimgui_capture_next_write_item(ctx);
    if (item) {
        item->cmd = sg_cimgui_CMD_MAKE_PIPELINE;
        item->color = _sg_cimgui_COLOR_RSRC;
        item->args.make_pipeline.result = pip_id;
    }
    if (ctx->hooks.make_pipeline) {
        ctx->hooks.make_pipeline(desc, pip_id, ctx->hooks.user_data);
    }
    if (pip_id.id != SG_INVALID_ID) {
        _sg_cimgui_pipeline_created(ctx, pip_id, _sg_cimgui_slot_index(pip_id.id), desc);
    }
}

_SOKOL_PRIVATE void _sg_cimgui_make_pass(const sg_pass_desc* desc, sg_pass pass_id, void* user_data) {
    sg_cimgui_t* ctx = (sg_cimgui_t*) user_data;
    SOKOL_ASSERT(ctx);
    sg_cimgui_capture_item_t* item = _sg_cimgui_capture_next_write_item(ctx);
    if (item) {
        item->cmd = sg_cimgui_CMD_MAKE_PASS;
        item->color = _sg_cimgui_COLOR_RSRC;
        item->args.make_pass.result = pass_id;
    }
    if (ctx->hooks.make_pass) {
        ctx->hooks.make_pass(desc, pass_id, ctx->hooks.user_data);
    }
    if (pass_id.id != SG_INVALID_ID) {
        _sg_cimgui_pass_created(ctx, pass_id, _sg_cimgui_slot_index(pass_id.id), desc);
    }
}

_SOKOL_PRIVATE void _sg_cimgui_destroy_buffer(sg_buffer buf, void* user_data) {
    sg_cimgui_t* ctx = (sg_cimgui_t*) user_data;
    SOKOL_ASSERT(ctx);
    sg_cimgui_capture_item_t* item = _sg_cimgui_capture_next_write_item(ctx);
    if (item) {
        item->cmd = sg_cimgui_CMD_DESTROY_BUFFER;
        item->color = _sg_cimgui_COLOR_RSRC;
        item->args.destroy_buffer.buffer = buf;
    }
    if (ctx->hooks.destroy_buffer) {
        ctx->hooks.destroy_buffer(buf, ctx->hooks.user_data);
    }
    if (buf.id != SG_INVALID_ID) {
        _sg_cimgui_buffer_destroyed(ctx, _sg_cimgui_slot_index(buf.id));
    }
}

_SOKOL_PRIVATE void _sg_cimgui_destroy_image(sg_image img, void* user_data) {
    sg_cimgui_t* ctx = (sg_cimgui_t*) user_data;
    SOKOL_ASSERT(ctx);
    sg_cimgui_capture_item_t* item = _sg_cimgui_capture_next_write_item(ctx);
    if (item) {
        item->cmd = sg_cimgui_CMD_DESTROY_IMAGE;
        item->color = _sg_cimgui_COLOR_RSRC;
        item->args.destroy_image.image = img;
    }
    if (ctx->hooks.destroy_image) {
        ctx->hooks.destroy_image(img, ctx->hooks.user_data);
    }
    if (img.id != SG_INVALID_ID) {
        _sg_cimgui_image_destroyed(ctx, _sg_cimgui_slot_index(img.id));
    }
}

_SOKOL_PRIVATE void _sg_cimgui_destroy_shader(sg_shader shd, void* user_data) {
    sg_cimgui_t* ctx = (sg_cimgui_t*) user_data;
    SOKOL_ASSERT(ctx);
    sg_cimgui_capture_item_t* item = _sg_cimgui_capture_next_write_item(ctx);
    if (item) {
        item->cmd = sg_cimgui_CMD_DESTROY_SHADER;
        item->color = _sg_cimgui_COLOR_RSRC;
        item->args.destroy_shader.shader = shd;
    }
    if (ctx->hooks.destroy_shader) {
        ctx->hooks.destroy_shader(shd, ctx->hooks.user_data);
    }
    if (shd.id != SG_INVALID_ID) {
        _sg_cimgui_shader_destroyed(ctx, _sg_cimgui_slot_index(shd.id));
    }
}

_SOKOL_PRIVATE void _sg_cimgui_destroy_pipeline(sg_pipeline pip, void* user_data) {
    sg_cimgui_t* ctx = (sg_cimgui_t*) user_data;
    SOKOL_ASSERT(ctx);
    sg_cimgui_capture_item_t* item = _sg_cimgui_capture_next_write_item(ctx);
    if (item) {
        item->cmd = sg_cimgui_CMD_DESTROY_PIPELINE;
        item->color = _sg_cimgui_COLOR_RSRC;
        item->args.destroy_pipeline.pipeline = pip;
    }
    if (ctx->hooks.destroy_pipeline) {
        ctx->hooks.destroy_pipeline(pip, ctx->hooks.user_data);
    }
    if (pip.id != SG_INVALID_ID) {
        _sg_cimgui_pipeline_destroyed(ctx, _sg_cimgui_slot_index(pip.id));
    }
}

_SOKOL_PRIVATE void _sg_cimgui_destroy_pass(sg_pass pass, void* user_data) {
    sg_cimgui_t* ctx = (sg_cimgui_t*) user_data;
    SOKOL_ASSERT(ctx);
    sg_cimgui_capture_item_t* item = _sg_cimgui_capture_next_write_item(ctx);
    if (item) {
        item->cmd = sg_cimgui_CMD_DESTROY_PASS;
        item->color = _sg_cimgui_COLOR_RSRC;
        item->args.destroy_pass.pass = pass;
    }
    if (ctx->hooks.destroy_pass) {
        ctx->hooks.destroy_pass(pass, ctx->hooks.user_data);
    }
    if (pass.id != SG_INVALID_ID) {
        _sg_cimgui_pass_destroyed(ctx, _sg_cimgui_slot_index(pass.id));
    }
}

_SOKOL_PRIVATE void _sg_cimgui_update_buffer(sg_buffer buf, const void* data_ptr, int data_size, void* user_data) {
    sg_cimgui_t* ctx = (sg_cimgui_t*) user_data;
    SOKOL_ASSERT(ctx);
    sg_cimgui_capture_item_t* item = _sg_cimgui_capture_next_write_item(ctx);
    if (item) {
        item->cmd = sg_cimgui_CMD_UPDATE_BUFFER;
        item->color = _sg_cimgui_COLOR_RSRC;
        item->args.update_buffer.buffer = buf;
        item->args.update_buffer.data_size = data_size;
    }
    if (ctx->hooks.update_buffer) {
        ctx->hooks.update_buffer(buf, data_ptr, data_size, ctx->hooks.user_data);
    }
}

_SOKOL_PRIVATE void _sg_cimgui_update_image(sg_image img, const sg_image_content* data, void* user_data) {
    sg_cimgui_t* ctx = (sg_cimgui_t*) user_data;
    SOKOL_ASSERT(ctx);
    sg_cimgui_capture_item_t* item = _sg_cimgui_capture_next_write_item(ctx);
    if (item) {
        item->cmd = sg_cimgui_CMD_UPDATE_IMAGE;
        item->color = _sg_cimgui_COLOR_RSRC;
        item->args.update_image.image = img;
    }
    if (ctx->hooks.update_image) {
        ctx->hooks.update_image(img, data, ctx->hooks.user_data);
    }
}

_SOKOL_PRIVATE void _sg_cimgui_append_buffer(sg_buffer buf, const void* data_ptr, int data_size, int result, void* user_data) {
    sg_cimgui_t* ctx = (sg_cimgui_t*) user_data;
    SOKOL_ASSERT(ctx);
    sg_cimgui_capture_item_t* item = _sg_cimgui_capture_next_write_item(ctx);
    if (item) {
        item->cmd = sg_cimgui_CMD_APPEND_BUFFER;
        item->color = _sg_cimgui_COLOR_RSRC;
        item->args.append_buffer.buffer = buf;
        item->args.append_buffer.data_size = data_size;
        item->args.append_buffer.result = result;
    }
    if (ctx->hooks.append_buffer) {
        ctx->hooks.append_buffer(buf, data_ptr, data_size, result, ctx->hooks.user_data);
    }
}

_SOKOL_PRIVATE void _sg_cimgui_begin_default_pass(const sg_pass_action* pass_action, int width, int height, void* user_data) {
    sg_cimgui_t* ctx = (sg_cimgui_t*) user_data;
    SOKOL_ASSERT(ctx);
    sg_cimgui_capture_item_t* item = _sg_cimgui_capture_next_write_item(ctx);
    if (item) {
        SOKOL_ASSERT(pass_action);
        item->cmd = sg_cimgui_CMD_BEGIN_DEFAULT_PASS;
        item->color = _sg_cimgui_COLOR_DRAW;
        item->args.begin_default_pass.action = *pass_action;
        item->args.begin_default_pass.width = width;
        item->args.begin_default_pass.height = height;
    }
    if (ctx->hooks.begin_default_pass) {
        ctx->hooks.begin_default_pass(pass_action, width, height, ctx->hooks.user_data);
    }
}

_SOKOL_PRIVATE void _sg_cimgui_begin_pass(sg_pass pass, const sg_pass_action* pass_action, void* user_data) {
    sg_cimgui_t* ctx = (sg_cimgui_t*) user_data;
    SOKOL_ASSERT(ctx);
    sg_cimgui_capture_item_t* item = _sg_cimgui_capture_next_write_item(ctx);
    if (item) {
        SOKOL_ASSERT(pass_action);
        item->cmd = sg_cimgui_CMD_BEGIN_PASS;
        item->color = _sg_cimgui_COLOR_DRAW;
        item->args.begin_pass.pass = pass;
        item->args.begin_pass.action = *pass_action;
    }
    if (ctx->hooks.begin_pass) {
        ctx->hooks.begin_pass(pass, pass_action, ctx->hooks.user_data);
    }
}

_SOKOL_PRIVATE void _sg_cimgui_apply_viewport(int x, int y, int width, int height, bool origin_top_left, void* user_data) {
    sg_cimgui_t* ctx = (sg_cimgui_t*) user_data;
    SOKOL_ASSERT(ctx);
    sg_cimgui_capture_item_t* item = _sg_cimgui_capture_next_write_item(ctx);
    if (item) {
        item->cmd = sg_cimgui_CMD_APPLY_VIEWPORT;
        item->color = _sg_cimgui_COLOR_DRAW;
        item->args.apply_viewport.x = x;
        item->args.apply_viewport.y = y;
        item->args.apply_viewport.width = width;
        item->args.apply_viewport.height = height;
        item->args.apply_viewport.origin_top_left = origin_top_left;
    }
    if (ctx->hooks.apply_viewport) {
        ctx->hooks.apply_viewport(x, y, width, height, origin_top_left, ctx->hooks.user_data);
    }
}

_SOKOL_PRIVATE void _sg_cimgui_apply_scissor_rect(int x, int y, int width, int height, bool origin_top_left, void* user_data) {
    sg_cimgui_t* ctx = (sg_cimgui_t*) user_data;
    SOKOL_ASSERT(ctx);
    sg_cimgui_capture_item_t* item = _sg_cimgui_capture_next_write_item(ctx);
    if (item) {
        item->cmd = sg_cimgui_CMD_APPLY_SCISSOR_RECT;
        item->color = _sg_cimgui_COLOR_DRAW;
        item->args.apply_scissor_rect.x = x;
        item->args.apply_scissor_rect.y = y;
        item->args.apply_scissor_rect.width = width;
        item->args.apply_scissor_rect.height = height;
        item->args.apply_scissor_rect.origin_top_left = origin_top_left;
    }
    if (ctx->hooks.apply_scissor_rect) {
        ctx->hooks.apply_scissor_rect(x, y, width, height, origin_top_left, ctx->hooks.user_data);
    }
}

_SOKOL_PRIVATE void _sg_cimgui_apply_pipeline(sg_pipeline pip, void* user_data) {
    sg_cimgui_t* ctx = (sg_cimgui_t*) user_data;
    SOKOL_ASSERT(ctx);
    ctx->cur_pipeline = pip;    /* stored for _sg_cimgui_apply_uniforms */
    sg_cimgui_capture_item_t* item = _sg_cimgui_capture_next_write_item(ctx);
    if (item) {
        item->cmd = sg_cimgui_CMD_APPLY_PIPELINE;
        item->color = _sg_cimgui_COLOR_DRAW;
        item->args.apply_pipeline.pipeline = pip;
    }
    if (ctx->hooks.apply_pipeline) {
        ctx->hooks.apply_pipeline(pip, ctx->hooks.user_data);
    }
}

_SOKOL_PRIVATE void _sg_cimgui_apply_bindings(const sg_bindings* bindings, void* user_data) {
    sg_cimgui_t* ctx = (sg_cimgui_t*) user_data;
    SOKOL_ASSERT(ctx);
    sg_cimgui_capture_item_t* item = _sg_cimgui_capture_next_write_item(ctx);
    if (item) {
        SOKOL_ASSERT(bindings);
        item->cmd = sg_cimgui_CMD_APPLY_BINDINGS;
        item->color = _sg_cimgui_COLOR_DRAW;
        item->args.apply_bindings.bindings = *bindings;
    }
    if (ctx->hooks.apply_bindings) {
        ctx->hooks.apply_bindings(bindings, ctx->hooks.user_data);
    }
}

_SOKOL_PRIVATE void _sg_cimgui_apply_uniforms(sg_shader_stage stage, int ub_index, const void* data, int num_bytes, void* user_data) {
    sg_cimgui_t* ctx = (sg_cimgui_t*) user_data;
    SOKOL_ASSERT(ctx);
    sg_cimgui_capture_item_t* item = _sg_cimgui_capture_next_write_item(ctx);
    if (item) {
        item->cmd = sg_cimgui_CMD_APPLY_UNIFORMS;
        item->color = _sg_cimgui_COLOR_DRAW;
        sg_cimgui_args_apply_uniforms_t* args = &item->args.apply_uniforms;
        args->stage = stage;
        args->ub_index = ub_index;
        args->data = data;
        args->num_bytes = num_bytes;
        args->pipeline = ctx->cur_pipeline;
        args->ubuf_pos = _sg_cimgui_capture_uniforms(ctx, data, num_bytes);
    }
    if (ctx->hooks.apply_uniforms) {
        ctx->hooks.apply_uniforms(stage, ub_index, data, num_bytes, ctx->hooks.user_data);
    }
}

_SOKOL_PRIVATE void _sg_cimgui_draw(int base_element, int num_elements, int num_instances, void* user_data) {
    sg_cimgui_t* ctx = (sg_cimgui_t*) user_data;
    SOKOL_ASSERT(ctx);
    sg_cimgui_capture_item_t* item = _sg_cimgui_capture_next_write_item(ctx);
    if (item) {
        item->cmd = sg_cimgui_CMD_DRAW;
        item->color = _sg_cimgui_COLOR_DRAW;
        item->args.draw.base_element = base_element;
        item->args.draw.num_elements = num_elements;
        item->args.draw.num_instances = num_instances;
    }
    if (ctx->hooks.draw) {
        ctx->hooks.draw(base_element, num_elements, num_instances, ctx->hooks.user_data);
    }
}

_SOKOL_PRIVATE void _sg_cimgui_end_pass(void* user_data) {
    sg_cimgui_t* ctx = (sg_cimgui_t*) user_data;
    SOKOL_ASSERT(ctx);
    ctx->cur_pipeline.id = SG_INVALID_ID;
    sg_cimgui_capture_item_t* item = _sg_cimgui_capture_next_write_item(ctx);
    if (item) {
        item->cmd = sg_cimgui_CMD_END_PASS;
        item->color = _sg_cimgui_COLOR_DRAW;
    }
    if (ctx->hooks.end_pass) {
        ctx->hooks.end_pass(ctx->hooks.user_data);
    }
}

_SOKOL_PRIVATE void _sg_cimgui_commit(void* user_data) {
    sg_cimgui_t* ctx = (sg_cimgui_t*) user_data;
    SOKOL_ASSERT(ctx);
    sg_cimgui_capture_item_t* item = _sg_cimgui_capture_next_write_item(ctx);
    if (item) {
        item->cmd = sg_cimgui_CMD_COMMIT;
        item->color = _sg_cimgui_COLOR_DRAW;
    }
    _sg_cimgui_capture_next_frame(ctx);
    if (ctx->hooks.commit) {
        ctx->hooks.commit(ctx->hooks.user_data);
    }
}

_SOKOL_PRIVATE void _sg_cimgui_alloc_buffer(sg_buffer result, void* user_data) {
    sg_cimgui_t* ctx = (sg_cimgui_t*) user_data;
    SOKOL_ASSERT(ctx);
    sg_cimgui_capture_item_t* item = _sg_cimgui_capture_next_write_item(ctx);
    if (item) {
        item->cmd = sg_cimgui_CMD_ALLOC_BUFFER;
        item->color = _sg_cimgui_COLOR_RSRC;
        item->args.alloc_buffer.result = result;
    }
    if (ctx->hooks.alloc_buffer) {
        ctx->hooks.alloc_buffer(result, ctx->hooks.user_data);
    }
}

_SOKOL_PRIVATE void _sg_cimgui_alloc_image(sg_image result, void* user_data) {
    sg_cimgui_t* ctx = (sg_cimgui_t*) user_data;
    SOKOL_ASSERT(ctx);
    sg_cimgui_capture_item_t* item = _sg_cimgui_capture_next_write_item(ctx);
    if (item) {
        item->cmd = sg_cimgui_CMD_ALLOC_IMAGE;
        item->color = _sg_cimgui_COLOR_RSRC;
        item->args.alloc_image.result = result;
    }
    if (ctx->hooks.alloc_image) {
        ctx->hooks.alloc_image(result, ctx->hooks.user_data);
    }
}

_SOKOL_PRIVATE void _sg_cimgui_alloc_shader(sg_shader result, void* user_data) {
    sg_cimgui_t* ctx = (sg_cimgui_t*) user_data;
    SOKOL_ASSERT(ctx);
    sg_cimgui_capture_item_t* item = _sg_cimgui_capture_next_write_item(ctx);
    if (item) {
        item->cmd = sg_cimgui_CMD_ALLOC_SHADER;
        item->color = _sg_cimgui_COLOR_RSRC;
        item->args.alloc_shader.result = result;
    }
    if (ctx->hooks.alloc_shader) {
        ctx->hooks.alloc_shader(result, ctx->hooks.user_data);
    }
}

_SOKOL_PRIVATE void _sg_cimgui_alloc_pipeline(sg_pipeline result, void* user_data) {
    sg_cimgui_t* ctx = (sg_cimgui_t*) user_data;
    SOKOL_ASSERT(ctx);
    sg_cimgui_capture_item_t* item = _sg_cimgui_capture_next_write_item(ctx);
    if (item) {
        item->cmd = sg_cimgui_CMD_ALLOC_PIPELINE;
        item->color = _sg_cimgui_COLOR_RSRC;
        item->args.alloc_pipeline.result = result;
    }
    if (ctx->hooks.alloc_pipeline) {
        ctx->hooks.alloc_pipeline(result, ctx->hooks.user_data);
    }
}

_SOKOL_PRIVATE void _sg_cimgui_alloc_pass(sg_pass result, void* user_data) {
    sg_cimgui_t* ctx = (sg_cimgui_t*) user_data;
    SOKOL_ASSERT(ctx);
    sg_cimgui_capture_item_t* item = _sg_cimgui_capture_next_write_item(ctx);
    if (item) {
        item->cmd = sg_cimgui_CMD_ALLOC_PASS;
        item->color = _sg_cimgui_COLOR_RSRC;
        item->args.alloc_pass.result = result;
    }
    if (ctx->hooks.alloc_pass) {
        ctx->hooks.alloc_pass(result, ctx->hooks.user_data);
    }
}

_SOKOL_PRIVATE void _sg_cimgui_init_buffer(sg_buffer buf_id, const sg_buffer_desc* desc, void* user_data) {
    sg_cimgui_t* ctx = (sg_cimgui_t*) user_data;
    SOKOL_ASSERT(ctx);
    sg_cimgui_capture_item_t* item = _sg_cimgui_capture_next_write_item(ctx);
    if (item) {
        item->cmd = sg_cimgui_CMD_INIT_BUFFER;
        item->color = _sg_cimgui_COLOR_RSRC;
        item->args.init_buffer.buffer = buf_id;
    }
    if (ctx->hooks.init_buffer) {
        ctx->hooks.init_buffer(buf_id, desc, ctx->hooks.user_data);
    }
    if (buf_id.id != SG_INVALID_ID) {
        _sg_cimgui_buffer_created(ctx, buf_id, _sg_cimgui_slot_index(buf_id.id), desc);
    }
}

_SOKOL_PRIVATE void _sg_cimgui_init_image(sg_image img_id, const sg_image_desc* desc, void* user_data) {
    sg_cimgui_t* ctx = (sg_cimgui_t*) user_data;
    SOKOL_ASSERT(ctx);
    sg_cimgui_capture_item_t* item = _sg_cimgui_capture_next_write_item(ctx);
    if (item) {
        item->cmd = sg_cimgui_CMD_INIT_IMAGE;
        item->color = _sg_cimgui_COLOR_RSRC;
        item->args.init_image.image = img_id;
    }
    if (ctx->hooks.init_image) {
        ctx->hooks.init_image(img_id, desc, ctx->hooks.user_data);
    }
    if (img_id.id != SG_INVALID_ID) {
        _sg_cimgui_image_created(ctx, img_id, _sg_cimgui_slot_index(img_id.id), desc);
    }
}

_SOKOL_PRIVATE void _sg_cimgui_init_shader(sg_shader shd_id, const sg_shader_desc* desc, void* user_data) {
    sg_cimgui_t* ctx = (sg_cimgui_t*) user_data;
    SOKOL_ASSERT(ctx);
    sg_cimgui_capture_item_t* item = _sg_cimgui_capture_next_write_item(ctx);
    if (item) {
        item->cmd = sg_cimgui_CMD_INIT_SHADER;
        item->color = _sg_cimgui_COLOR_RSRC;
        item->args.init_shader.shader = shd_id;
    }
    if (ctx->hooks.init_shader) {
        ctx->hooks.init_shader(shd_id, desc, ctx->hooks.user_data);
    }
    if (shd_id.id != SG_INVALID_ID) {
        _sg_cimgui_shader_created(ctx, shd_id, _sg_cimgui_slot_index(shd_id.id), desc);
    }
}

_SOKOL_PRIVATE void _sg_cimgui_init_pipeline(sg_pipeline pip_id, const sg_pipeline_desc* desc, void* user_data) {
    sg_cimgui_t* ctx = (sg_cimgui_t*) user_data;
    SOKOL_ASSERT(ctx);
    sg_cimgui_capture_item_t* item = _sg_cimgui_capture_next_write_item(ctx);
    if (item) {
        item->cmd = sg_cimgui_CMD_INIT_PIPELINE;
        item->color = _sg_cimgui_COLOR_RSRC;
        item->args.init_pipeline.pipeline = pip_id;
    }
    if (ctx->hooks.init_pipeline) {
        ctx->hooks.init_pipeline(pip_id, desc, ctx->hooks.user_data);
    }
    if (pip_id.id != SG_INVALID_ID) {
        _sg_cimgui_pipeline_created(ctx, pip_id, _sg_cimgui_slot_index(pip_id.id), desc);
    }
}

_SOKOL_PRIVATE void _sg_cimgui_init_pass(sg_pass pass_id, const sg_pass_desc* desc, void* user_data) {
    sg_cimgui_t* ctx = (sg_cimgui_t*) user_data;
    SOKOL_ASSERT(ctx);
    sg_cimgui_capture_item_t* item = _sg_cimgui_capture_next_write_item(ctx);
    if (item) {
        item->cmd = sg_cimgui_CMD_INIT_PASS;
        item->color = _sg_cimgui_COLOR_RSRC;
        item->args.init_pass.pass = pass_id;
    }
    if (ctx->hooks.init_pass) {
        ctx->hooks.init_pass(pass_id, desc, ctx->hooks.user_data);
    }
    if (pass_id.id != SG_INVALID_ID) {
        _sg_cimgui_pass_created(ctx, pass_id, _sg_cimgui_slot_index(pass_id.id), desc);
    }
}

_SOKOL_PRIVATE void _sg_cimgui_fail_buffer(sg_buffer buf_id, void* user_data) {
    sg_cimgui_t* ctx = (sg_cimgui_t*) user_data;
    SOKOL_ASSERT(ctx);
    sg_cimgui_capture_item_t* item = _sg_cimgui_capture_next_write_item(ctx);
    if (item) {
        item->cmd = sg_cimgui_CMD_FAIL_BUFFER;
        item->color = _sg_cimgui_COLOR_RSRC;
        item->args.fail_buffer.buffer = buf_id;
    }
    if (ctx->hooks.fail_buffer) {
        ctx->hooks.fail_buffer(buf_id, ctx->hooks.user_data);
    }
}

_SOKOL_PRIVATE void _sg_cimgui_fail_image(sg_image img_id, void* user_data) {
    sg_cimgui_t* ctx = (sg_cimgui_t*) user_data;
    SOKOL_ASSERT(ctx);
    sg_cimgui_capture_item_t* item = _sg_cimgui_capture_next_write_item(ctx);
    if (item) {
        item->cmd = sg_cimgui_CMD_FAIL_IMAGE;
        item->color = _sg_cimgui_COLOR_RSRC;
        item->args.fail_image.image = img_id;
    }
    if (ctx->hooks.fail_image) {
        ctx->hooks.fail_image(img_id, ctx->hooks.user_data);
    }
}

_SOKOL_PRIVATE void _sg_cimgui_fail_shader(sg_shader shd_id, void* user_data) {
    sg_cimgui_t* ctx = (sg_cimgui_t*) user_data;
    SOKOL_ASSERT(ctx);
    sg_cimgui_capture_item_t* item = _sg_cimgui_capture_next_write_item(ctx);
    if (item) {
        item->cmd = sg_cimgui_CMD_FAIL_SHADER;
        item->color = _sg_cimgui_COLOR_RSRC;
        item->args.fail_shader.shader = shd_id;
    }
    if (ctx->hooks.fail_shader) {
        ctx->hooks.fail_shader(shd_id, ctx->hooks.user_data);
    }
}

_SOKOL_PRIVATE void _sg_cimgui_fail_pipeline(sg_pipeline pip_id, void* user_data) {
    sg_cimgui_t* ctx = (sg_cimgui_t*) user_data;
    SOKOL_ASSERT(ctx);
    sg_cimgui_capture_item_t* item = _sg_cimgui_capture_next_write_item(ctx);
    if (item) {
        item->cmd = sg_cimgui_CMD_FAIL_PIPELINE;
        item->color = _sg_cimgui_COLOR_RSRC;
        item->args.fail_pipeline.pipeline = pip_id;
    }
    if (ctx->hooks.fail_pipeline) {
        ctx->hooks.fail_pipeline(pip_id, ctx->hooks.user_data);
    }
}

_SOKOL_PRIVATE void _sg_cimgui_fail_pass(sg_pass pass_id, void* user_data) {
    sg_cimgui_t* ctx = (sg_cimgui_t*) user_data;
    SOKOL_ASSERT(ctx);
    sg_cimgui_capture_item_t* item = _sg_cimgui_capture_next_write_item(ctx);
    if (item) {
        item->cmd = sg_cimgui_CMD_FAIL_PASS;
        item->color = _sg_cimgui_COLOR_RSRC;
        item->args.fail_pass.pass = pass_id;
    }
    if (ctx->hooks.fail_pass) {
        ctx->hooks.fail_pass(pass_id, ctx->hooks.user_data);
    }
}

_SOKOL_PRIVATE void _sg_cimgui_push_debug_group(const char* name, void* user_data) {
    sg_cimgui_t* ctx = (sg_cimgui_t*) user_data;
    SOKOL_ASSERT(ctx);
    sg_cimgui_capture_item_t* item = _sg_cimgui_capture_next_write_item(ctx);
    if (item) {
        item->cmd = sg_cimgui_CMD_PUSH_DEBUG_GROUP;
        item->color = _sg_cimgui_COLOR_OTHER;
        item->args.push_debug_group.name = _sg_cimgui_make_str(name);
    }
    if (ctx->hooks.push_debug_group) {
        ctx->hooks.push_debug_group(name, ctx->hooks.user_data);
    }
}

_SOKOL_PRIVATE void _sg_cimgui_pop_debug_group(void* user_data) {
    sg_cimgui_t* ctx = (sg_cimgui_t*) user_data;
    SOKOL_ASSERT(ctx);
    sg_cimgui_capture_item_t* item = _sg_cimgui_capture_next_write_item(ctx);
    if (item) {
        item->cmd = sg_cimgui_CMD_POP_DEBUG_GROUP;
        item->color = _sg_cimgui_COLOR_OTHER;
    }
    if (ctx->hooks.pop_debug_group) {
        ctx->hooks.pop_debug_group(ctx->hooks.user_data);
    }
}

_SOKOL_PRIVATE void _sg_cimgui_err_buffer_pool_exhausted(void* user_data) {
    sg_cimgui_t* ctx = (sg_cimgui_t*) user_data;
    SOKOL_ASSERT(ctx);
    sg_cimgui_capture_item_t* item = _sg_cimgui_capture_next_write_item(ctx);
    if (item) {
        item->cmd = sg_cimgui_CMD_ERR_BUFFER_POOL_EXHAUSTED;
        item->color = _sg_cimgui_COLOR_ERR;
    }
    if (ctx->hooks.err_buffer_pool_exhausted) {
        ctx->hooks.err_buffer_pool_exhausted(ctx->hooks.user_data);
    }
}

_SOKOL_PRIVATE void _sg_cimgui_err_image_pool_exhausted(void* user_data) {
    sg_cimgui_t* ctx = (sg_cimgui_t*) user_data;
    SOKOL_ASSERT(ctx);
    sg_cimgui_capture_item_t* item = _sg_cimgui_capture_next_write_item(ctx);
    if (item) {
        item->cmd = sg_cimgui_CMD_ERR_IMAGE_POOL_EXHAUSTED;
        item->color = _sg_cimgui_COLOR_ERR;
    }
    if (ctx->hooks.err_image_pool_exhausted) {
        ctx->hooks.err_image_pool_exhausted(ctx->hooks.user_data);
    }
}

_SOKOL_PRIVATE void _sg_cimgui_err_shader_pool_exhausted(void* user_data) {
    sg_cimgui_t* ctx = (sg_cimgui_t*) user_data;
    SOKOL_ASSERT(ctx);
    sg_cimgui_capture_item_t* item = _sg_cimgui_capture_next_write_item(ctx);
    if (item) {
        item->cmd = sg_cimgui_CMD_ERR_SHADER_POOL_EXHAUSTED;
        item->color = _sg_cimgui_COLOR_ERR;
    }
    if (ctx->hooks.err_shader_pool_exhausted) {
        ctx->hooks.err_shader_pool_exhausted(ctx->hooks.user_data);
    }
}

_SOKOL_PRIVATE void _sg_cimgui_err_pipeline_pool_exhausted(void* user_data) {
    sg_cimgui_t* ctx = (sg_cimgui_t*) user_data;
    SOKOL_ASSERT(ctx);
    sg_cimgui_capture_item_t* item = _sg_cimgui_capture_next_write_item(ctx);
    if (item) {
        item->cmd = sg_cimgui_CMD_ERR_PIPELINE_POOL_EXHAUSTED;
        item->color = _sg_cimgui_COLOR_ERR;
    }
    if (ctx->hooks.err_pipeline_pool_exhausted) {
        ctx->hooks.err_pipeline_pool_exhausted(ctx->hooks.user_data);
    }
}

_SOKOL_PRIVATE void _sg_cimgui_err_pass_pool_exhausted(void* user_data) {
    sg_cimgui_t* ctx = (sg_cimgui_t*) user_data;
    SOKOL_ASSERT(ctx);
    sg_cimgui_capture_item_t* item = _sg_cimgui_capture_next_write_item(ctx);
    if (item) {
        item->cmd = sg_cimgui_CMD_ERR_PASS_POOL_EXHAUSTED;
        item->color = _sg_cimgui_COLOR_ERR;
    }
    if (ctx->hooks.err_pass_pool_exhausted) {
        ctx->hooks.err_pass_pool_exhausted(ctx->hooks.user_data);
    }
}

_SOKOL_PRIVATE void _sg_cimgui_err_context_mismatch(void* user_data) {
    sg_cimgui_t* ctx = (sg_cimgui_t*) user_data;
    SOKOL_ASSERT(ctx);
    sg_cimgui_capture_item_t* item = _sg_cimgui_capture_next_write_item(ctx);
    if (item) {
        item->cmd = sg_cimgui_CMD_ERR_CONTEXT_MISMATCH;
        item->color = _sg_cimgui_COLOR_ERR;
    }
    if (ctx->hooks.err_context_mismatch) {
        ctx->hooks.err_context_mismatch(ctx->hooks.user_data);
    }
}

_SOKOL_PRIVATE void _sg_cimgui_err_pass_invalid(void* user_data) {
    sg_cimgui_t* ctx = (sg_cimgui_t*) user_data;
    SOKOL_ASSERT(ctx);
    sg_cimgui_capture_item_t* item = _sg_cimgui_capture_next_write_item(ctx);
    if (item) {
        item->cmd = sg_cimgui_CMD_ERR_PASS_INVALID;
        item->color = _sg_cimgui_COLOR_ERR;
    }
    if (ctx->hooks.err_pass_invalid) {
        ctx->hooks.err_pass_invalid(ctx->hooks.user_data);
    }
}

_SOKOL_PRIVATE void _sg_cimgui_err_draw_invalid(void* user_data) {
    sg_cimgui_t* ctx = (sg_cimgui_t*) user_data;
    SOKOL_ASSERT(ctx);
    sg_cimgui_capture_item_t* item = _sg_cimgui_capture_next_write_item(ctx);
    if (item) {
        item->cmd = sg_cimgui_CMD_ERR_DRAW_INVALID;
        item->color = _sg_cimgui_COLOR_ERR;
    }
    if (ctx->hooks.err_draw_invalid) {
        ctx->hooks.err_draw_invalid(ctx->hooks.user_data);
    }
}

_SOKOL_PRIVATE void _sg_cimgui_err_bindings_invalid(void* user_data) {
    sg_cimgui_t* ctx = (sg_cimgui_t*) user_data;
    SOKOL_ASSERT(ctx);
    sg_cimgui_capture_item_t* item = _sg_cimgui_capture_next_write_item(ctx);
    if (item) {
        item->cmd = sg_cimgui_CMD_ERR_BINDINGS_INVALID;
        item->color = _sg_cimgui_COLOR_ERR;
    }
    if (ctx->hooks.err_bindings_invalid) {
        ctx->hooks.err_bindings_invalid(ctx->hooks.user_data);
    }
}

/*--- IMGUI HELPERS ----------------------------------------------------------*/
_SOKOL_PRIVATE bool _sg_cimgui_draw_resid_list_item(uint32_t res_id, const char* label, bool selected) {
    igPushIDInt(res_id);
    bool res;
    if (label[0]) {
		res = igSelectable(label, selected, ImGuiSelectableFlags_None, _sg_cimgui_VEC2_ZERO);
    }
    else {
        sg_cimgui_str_t str;
        _sg_cimgui_snprintf(&str, "0x%08X", res_id);
        res = igSelectable(str.buf, selected, ImGuiSelectableFlags_None, _sg_cimgui_VEC2_ZERO);
    }
    igPopID();
    return res;
}

_SOKOL_PRIVATE bool _sg_cimgui_draw_resid_link(uint32_t res_id, const char* label) {
    SOKOL_ASSERT(label);
    sg_cimgui_str_t str_buf;
    const char* str;
    if (label[0]) {
        str = label;
    }
    else {
        _sg_cimgui_snprintf(&str_buf, "0x%08X", res_id);
        str = str_buf.buf;
    }
    igPushIDInt(res_id);
    bool res = igSmallButton(str);
    igPopID();
    return res;
}

_SOKOL_PRIVATE bool _sg_cimgui_draw_buffer_link(sg_cimgui_t* ctx, sg_buffer buf) {
    bool retval = false;
    if (buf.id != SG_INVALID_ID) {
        const sg_cimgui_buffer_t* buf_ui = &ctx->buffers.slots[_sg_cimgui_slot_index(buf.id)];
        retval = _sg_cimgui_draw_resid_link(buf.id, buf_ui->label.buf);
    }
    return retval;
}

_SOKOL_PRIVATE bool _sg_cimgui_draw_image_link(sg_cimgui_t* ctx, sg_image img) {
    bool retval = false;
    if (img.id != SG_INVALID_ID) {
        const sg_cimgui_image_t* img_ui = &ctx->images.slots[_sg_cimgui_slot_index(img.id)];
        retval = _sg_cimgui_draw_resid_link(img.id, img_ui->label.buf);
    }
    return retval;
}

_SOKOL_PRIVATE bool _sg_cimgui_draw_shader_link(sg_cimgui_t* ctx, sg_shader shd) {
    bool retval = false;
    if (shd.id != SG_INVALID_ID) {
        const sg_cimgui_shader_t* shd_ui = &ctx->shaders.slots[_sg_cimgui_slot_index(shd.id)];
        retval = _sg_cimgui_draw_resid_link(shd.id, shd_ui->label.buf);
    }
    return retval;
}

_SOKOL_PRIVATE void _sg_cimgui_show_buffer(sg_cimgui_t* ctx, sg_buffer buf) {
    ctx->buffers.open = true;
    ctx->buffers.sel_buf = buf;
}

_SOKOL_PRIVATE void _sg_cimgui_show_image(sg_cimgui_t* ctx, sg_image img) {
    ctx->images.open = true;
    ctx->images.sel_img = img;
}

_SOKOL_PRIVATE void _sg_cimgui_show_shader(sg_cimgui_t* ctx, sg_shader shd) {
    ctx->shaders.open = true;
    ctx->shaders.sel_shd = shd;
}

_SOKOL_PRIVATE void _sg_cimgui_draw_buffer_list(sg_cimgui_t* ctx) {
	igBeginChild("buffer_list", (ImVec2){_sg_cimgui_LIST_WIDTH,0}, true, ImGuiWindowFlags_None);
    for (int i = 0; i < ctx->buffers.num_slots; i++) {
        sg_buffer buf = ctx->buffers.slots[i].res_id;
        sg_resource_state state = sg_query_buffer_state(buf);
        if ((state != SG_RESOURCESTATE_INVALID) && (state != SG_RESOURCESTATE_INITIAL)) {
            bool selected = ctx->buffers.sel_buf.id == buf.id;
            if (_sg_cimgui_draw_resid_list_item(buf.id, ctx->buffers.slots[i].label.buf, selected)) {
                ctx->buffers.sel_buf.id = buf.id;
            }
        }
    }
    igEndChild();
}

_SOKOL_PRIVATE void _sg_cimgui_draw_image_list(sg_cimgui_t* ctx) {
	igBeginChild("image_list", (ImVec2){_sg_cimgui_LIST_WIDTH,0}, true, ImGuiWindowFlags_None);
    for (int i = 0; i < ctx->images.num_slots; i++) {
        sg_image img = ctx->images.slots[i].res_id;
        sg_resource_state state = sg_query_image_state(img);
        if ((state != SG_RESOURCESTATE_INVALID) && (state != SG_RESOURCESTATE_INITIAL)) {
            bool selected = ctx->images.sel_img.id == img.id;
            if (_sg_cimgui_draw_resid_list_item(img.id, ctx->images.slots[i].label.buf, selected)) {
                ctx->images.sel_img.id = img.id;
            }
        }
    }
    igEndChild();
}

_SOKOL_PRIVATE void _sg_cimgui_draw_shader_list(sg_cimgui_t* ctx) {
    igBeginChild("shader_list", (ImVec2){_sg_cimgui_LIST_WIDTH,0}, true, ImGuiWindowFlags_None);
    for (int i = 0; i < ctx->shaders.num_slots; i++) {
        sg_shader shd = ctx->shaders.slots[i].res_id;
        sg_resource_state state = sg_query_shader_state(shd);
        if ((state != SG_RESOURCESTATE_INVALID) && (state != SG_RESOURCESTATE_INITIAL)) {
            bool selected = ctx->shaders.sel_shd.id == shd.id;
            if (_sg_cimgui_draw_resid_list_item(shd.id, ctx->shaders.slots[i].label.buf, selected)) {
                ctx->shaders.sel_shd.id = shd.id;
            }
        }
    }
    igEndChild();
}

_SOKOL_PRIVATE void _sg_cimgui_draw_pipeline_list(sg_cimgui_t* ctx) {
    igBeginChild("pipeline_list", (ImVec2){_sg_cimgui_LIST_WIDTH,0}, true, ImGuiWindowFlags_None);
    for (int i = 1; i < ctx->pipelines.num_slots; i++) {
        sg_pipeline pip = ctx->pipelines.slots[i].res_id;
        sg_resource_state state = sg_query_pipeline_state(pip);
        if ((state != SG_RESOURCESTATE_INVALID) && (state != SG_RESOURCESTATE_INITIAL)) {
            bool selected = ctx->pipelines.sel_pip.id == pip.id;
            if (_sg_cimgui_draw_resid_list_item(pip.id, ctx->pipelines.slots[i].label.buf, selected)) {
                ctx->pipelines.sel_pip.id = pip.id;
            }
        }
    }
    igEndChild();
}

_SOKOL_PRIVATE void _sg_cimgui_draw_pass_list(sg_cimgui_t* ctx) {
    igBeginChild("pass_list", (ImVec2){_sg_cimgui_LIST_WIDTH,0}, true, ImGuiWindowFlags_None);
    for (int i = 1; i < ctx->passes.num_slots; i++) {
        sg_pass pass = ctx->passes.slots[i].res_id;
        sg_resource_state state = sg_query_pass_state(pass);
        if ((state != SG_RESOURCESTATE_INVALID) && (state != SG_RESOURCESTATE_INITIAL)) {
            bool selected = ctx->passes.sel_pass.id == pass.id;
            if (_sg_cimgui_draw_resid_list_item(pass.id, ctx->passes.slots[i].label.buf, selected)) {
                ctx->passes.sel_pass.id = pass.id;
            }
        }
    }
    igEndChild();
}

_SOKOL_PRIVATE void _sg_cimgui_draw_capture_list(sg_cimgui_t* ctx) {
    igBeginChild("capture_list", (ImVec2){_sg_cimgui_LIST_WIDTH,0}, true, ImGuiWindowFlags_None);
    const uint32_t num_items = _sg_cimgui_capture_num_read_items(ctx);
    uint64_t group_stack = 1;   /* bit set: group unfolded, cleared: folded */
    for (uint32_t i = 0; i < num_items; i++) {
        const sg_cimgui_capture_item_t* item = _sg_cimgui_capture_read_item_at(ctx, i);
        sg_cimgui_str_t item_string = _sg_cimgui_capture_item_string(ctx, i, item);
        igPushStyleColorU32(ImGuiCol_Text, item->color);
        if (item->cmd == sg_cimgui_CMD_PUSH_DEBUG_GROUP) {
            if (group_stack & 1) {
                group_stack <<= 1;
                const char* group_name = item->args.push_debug_group.name.buf;
                if (igTreeNodeStrStr(group_name, "Group: %s", group_name)) {
                    group_stack |= 1;
                }
            }
            else {
                group_stack <<= 1;
            }
        }
        else if (item->cmd == sg_cimgui_CMD_POP_DEBUG_GROUP) {
            if (group_stack & 1) {
                igTreePop();
            }
            group_stack >>= 1;
        }
        else if (group_stack & 1) {
            igPushIDInt(i);
            if (igSelectable(item_string.buf, ctx->capture.sel_item == i, ImGuiSelectableFlags_None, _sg_cimgui_VEC2_ZERO)) {
                ctx->capture.sel_item = i;
            }
            if (igIsItemHovered(ImGuiHoveredFlags_None)) {
                igSetTooltip("%s", item_string.buf);
            }
            igPopID();
        }
		igPopStyleColor(1);
    }
    igEndChild();
}

_SOKOL_PRIVATE void _sg_cimgui_draw_buffer_panel(sg_cimgui_t* ctx, sg_buffer buf) {
    if (buf.id != SG_INVALID_ID) {
		igBeginChild("buffer", _sg_cimgui_VEC2_ZERO, false, ImGuiWindowFlags_None);
        sg_buffer_info info = sg_query_buffer_info(buf);
        if (info.slot.state == SG_RESOURCESTATE_VALID) {
            const sg_cimgui_buffer_t* buf_ui = &ctx->buffers.slots[_sg_cimgui_slot_index(buf.id)];
            igText("Label: %s", buf_ui->label.buf[0] ? buf_ui->label.buf : "---");
            _sg_cimgui_draw_resource_slot(&info.slot);
            igSeparator();
            igText("Type:  %s", _sg_cimgui_buffertype_string(buf_ui->desc.type));
            igText("Usage: %s", _sg_cimgui_usage_string(buf_ui->desc.usage));
            igText("Size:  %d", buf_ui->desc.size);
            if (buf_ui->desc.usage != SG_USAGE_IMMUTABLE) {
                igSeparator();
                igText("Num Slots:     %d", info.num_slots);
                igText("Active Slot:   %d", info.active_slot);
                igText("Update Frame Index: %d", info.update_frame_index);
                igText("Append Frame Index: %d", info.append_frame_index);
                igText("Append Pos:         %d", info.append_pos);
                igText("Append Overflow:    %s", info.append_overflow ? "YES":"NO");
            }
        }
        else {
            igText("Buffer 0x%08X not valid.", buf.id);
        }
        igEndChild();
    }
}

_SOKOL_PRIVATE bool _sg_cimgui_image_renderable(sg_cimgui_t* ctx, sg_image_type type, sg_pixel_format fmt) {
    if ((SG_IMAGETYPE_2D != type) || (SG_PIXELFORMAT_DEPTH == fmt) || (SG_PIXELFORMAT_DEPTHSTENCIL == fmt)) {
        return false;
    }
    else {
        return true;
    }
}

_SOKOL_PRIVATE void _sg_cimgui_draw_embedded_image(sg_cimgui_t* ctx, sg_image img, float* scale) {
    if (sg_query_image_state(img) == SG_RESOURCESTATE_VALID) {
        sg_cimgui_image_t* img_ui = &ctx->images.slots[_sg_cimgui_slot_index(img.id)];
        if (_sg_cimgui_image_renderable(ctx, img_ui->desc.type, img_ui->desc.pixel_format)) {
            igPushIDInt(img.id);
            igSliderFloat("Scale", scale, 0.125f, 8.0f, "%.3f", 2.0f);
            float w = (float)img_ui->desc.width * (*scale);
            float h = (float)img_ui->desc.height * (*scale);
			igImage((ImTextureID)(intptr_t)img.id, (ImVec2){w, h}, _sg_cimgui_VEC2_ZERO, (ImVec2){1,1}, (ImVec4){1,1,1,1}, (ImVec4){0,0,0,0});
            igPopID();
        }
        else {
            igText("Image not renderable.");
        }
    }
}

_SOKOL_PRIVATE void _sg_cimgui_draw_image_panel(sg_cimgui_t* ctx, sg_image img) {
    if (img.id != SG_INVALID_ID) {
        igBeginChild("image", _sg_cimgui_VEC2_ZERO, false, ImGuiWindowFlags_None);
        sg_image_info info = sg_query_image_info(img);
        if (info.slot.state == SG_RESOURCESTATE_VALID) {
            sg_cimgui_image_t* img_ui = &ctx->images.slots[_sg_cimgui_slot_index(img.id)];
            const sg_image_desc* desc = &img_ui->desc;
            igText("Label: %s", img_ui->label.buf[0] ? img_ui->label.buf : "---");
            _sg_cimgui_draw_resource_slot(&info.slot);
            igSeparator();
            _sg_cimgui_draw_embedded_image(ctx, img, &img_ui->ui_scale);
            igSeparator();
            igText("Type:              %s", _sg_cimgui_imagetype_string(desc->type));
            igText("Usage:             %s", _sg_cimgui_usage_string(desc->usage));
            igText("Render Target:     %s", desc->render_target ? "YES":"NO");
            igText("Width:             %d", desc->width);
            igText("Height:            %d", desc->height);
            igText("Depth:             %d", desc->depth);
            igText("Num Mipmaps:       %d", desc->num_mipmaps);
            igText("Pixel Format:      %s", _sg_cimgui_pixelformat_string(desc->pixel_format));
            igText("Sample Count:      %d", desc->sample_count);
            igText("Min Filter:        %s", _sg_cimgui_filter_string(desc->min_filter));
            igText("Mag Filter:        %s", _sg_cimgui_filter_string(desc->mag_filter));
            igText("Wrap U:            %s", _sg_cimgui_wrap_string(desc->wrap_u));
            igText("Wrap V:            %s", _sg_cimgui_wrap_string(desc->wrap_v));
            igText("Wrap W:            %s", _sg_cimgui_wrap_string(desc->wrap_w));
            igText("Max Anisotropy:    %d", desc->max_anisotropy);
            igText("Min LOD:           %.3f", desc->min_lod);
            igText("Max LOD:           %.3f", desc->max_lod);
            if (desc->usage != SG_USAGE_IMMUTABLE) {
                igSeparator();
                igText("Num Slots:     %d", info.num_slots);
                igText("Active Slot:   %d", info.active_slot);
                igText("Update Frame Index: %d", info.upd_frame_index);
            }
        }
        else {
            igText("Image 0x%08X not valid.", img.id);
        }
        igEndChild();
    }
}

_SOKOL_PRIVATE void _sg_cimgui_draw_shader_stage(sg_cimgui_t* ctx, const sg_shader_stage_desc* stage) {
    int num_valid_ubs = 0;
    for (int i = 0; i < SG_MAX_SHADERSTAGE_UBS; i++) {
        const sg_shader_uniform_block_desc* ub = &stage->uniform_blocks[i];
        for (int j = 0; j < SG_MAX_UB_MEMBERS; j++) {
            const sg_shader_uniform_desc* u = &ub->uniforms[j];
            if (SG_UNIFORMTYPE_INVALID != u->type) {
                num_valid_ubs++;
                break;
            }
        }
    }
    int num_valid_images = 0;
    for (int i = 0; i < SG_MAX_SHADERSTAGE_IMAGES; i++) {
        if (_SG_IMAGETYPE_DEFAULT != stage->images[i].type) {
            num_valid_images++;
        }
        else {
            break;
        }
    }
    if (num_valid_ubs > 0) {
        if (igTreeNodeStr("Uniform Blocks")) {
            for (int i = 0; i < num_valid_ubs; i++) {
                igText("#%d:", i);
                const sg_shader_uniform_block_desc* ub = &stage->uniform_blocks[i];
                for (int j = 0; j < SG_MAX_UB_MEMBERS; j++) {
                    const sg_shader_uniform_desc* u = &ub->uniforms[j];
                    if (SG_UNIFORMTYPE_INVALID != u->type) {
                        if (u->array_count == 0) {
                            igText("  %s %s", _sg_cimgui_uniformtype_string(u->type), u->name ? u->name : "");
                        }
                        else {
                            igText("  %s[%d] %s", _sg_cimgui_uniformtype_string(u->type), u->array_count, u->name ? u->name : "");
                        }
                    }
                }
            }
            igTreePop();
        }
    }
    if (num_valid_images > 0) {
        if (igTreeNodeStr("Images")) {
            for (int i = 0; i < SG_MAX_SHADERSTAGE_IMAGES; i++) {
                const sg_shader_image_desc* sid = &stage->images[i];
                if (sid->type != _SG_IMAGETYPE_DEFAULT) {
                    igText("%s %s", _sg_cimgui_imagetype_string(sid->type), sid->name ? sid->name : "");
                }
                else {
                    break;
                }
            }
            igTreePop();
        }
    }
    if (stage->entry) {
        igText("Entry: %s", stage->entry);
    }
    if (stage->source) {
        if (igTreeNodeStr("Source")) {
            igText("%s", stage->source);
            igTreePop();
        }
    }
    else if (stage->byte_code) {
        if (igTreeNodeStr("Byte Code")) {
            igText("Byte-code display currently not supported.");
            igTreePop();
        }
    }
}

_SOKOL_PRIVATE void _sg_cimgui_draw_shader_panel(sg_cimgui_t* ctx, sg_shader shd) {
    if (shd.id != SG_INVALID_ID) {
        igBeginChild("shader", _sg_cimgui_VEC2_ZERO, false, ImGuiWindowFlags_HorizontalScrollbar);
        sg_shader_info info = sg_query_shader_info(shd);
        if (info.slot.state == SG_RESOURCESTATE_VALID) {
            const sg_cimgui_shader_t* shd_ui = &ctx->shaders.slots[_sg_cimgui_slot_index(shd.id)];
            igText("Label: %s", shd_ui->label.buf[0] ? shd_ui->label.buf : "---");
            _sg_cimgui_draw_resource_slot(&info.slot);
            igSeparator();
            if (igTreeNodeStr("Attrs")) {
                for (int i = 0; i < SG_MAX_VERTEX_ATTRIBUTES; i++) {
                    const sg_shader_attr_desc* a_desc = &shd_ui->desc.attrs[i];
                    if (a_desc->name || a_desc->sem_index) {
                        igText("#%d:", i);
                        igText("  Name:         %s", a_desc->name ? a_desc->name : "---");
                        igText("  Sem Name:     %s", a_desc->sem_name ? a_desc->sem_name : "---");
                        igText("  Sem Index:    %d", a_desc->sem_index);
                    }
                }
                igTreePop();
            }
            if (igTreeNodeStr("Vertex Shader Stage")) {
                _sg_cimgui_draw_shader_stage(ctx, &shd_ui->desc.vs);
                igTreePop();
            }
            if (igTreeNodeStr("Fragment Shader Stage")) {
                _sg_cimgui_draw_shader_stage(ctx, &shd_ui->desc.fs);
                igTreePop();
            }
        }
        else {
            igText("Shader 0x%08X not valid!", shd.id);
        }
        igEndChild();
    }
}

_SOKOL_PRIVATE void _sg_cimgui_draw_vertex_layout(const sg_layout_desc* layout) {
    if (igTreeNodeStr("Buffers")) {
        for (int i = 0; i < SG_MAX_SHADERSTAGE_BUFFERS; i++) {
            const sg_buffer_layout_desc* l_desc = &layout->buffers[i];
            if (l_desc->stride > 0) {
                igText("#%d:", i);
                igText("  Stride:    %d", l_desc->stride);
                igText("  Step Func: %s", _sg_cimgui_vertexstep_string(l_desc->step_func));
                igText("  Step Rate: %d", l_desc->step_rate);
            }
        }
        igTreePop();
    }
    if (igTreeNodeStr("Attrs")) {
        for (int i = 0; i < SG_MAX_VERTEX_ATTRIBUTES; i++) {
            const sg_vertex_attr_desc* a_desc = &layout->attrs[i];
            if (a_desc->format != SG_VERTEXFORMAT_INVALID) {
                igText("#%d:", i);
                igText("  Format:       %s", _sg_cimgui_vertexformat_string(a_desc->format));
                igText("  Offset:       %d", a_desc->offset);
                igText("  Buffer Index: %d", a_desc->buffer_index);
            }
        }
        igTreePop();
    }
}

_SOKOL_PRIVATE void _sg_cimgui_draw_stencil_state(const sg_stencil_state* ss) {
    igText("Fail Op:       %s", _sg_cimgui_stencilop_string(ss->fail_op));
    igText("Depth Fail Op: %s", _sg_cimgui_stencilop_string(ss->depth_fail_op));
    igText("Pass Op:       %s", _sg_cimgui_stencilop_string(ss->pass_op));
    igText("Compare Func:  %s", _sg_cimgui_comparefunc_string(ss->compare_func));
}

_SOKOL_PRIVATE void _sg_cimgui_draw_depth_stencil_state(const sg_depth_stencil_state* dss) {
    igText("Depth Compare Func:  %s", _sg_cimgui_comparefunc_string(dss->depth_compare_func));
    igText("Depth Write Enabled: %s", dss->depth_write_enabled ? "YES":"NO");
    igText("Stencil Enabled:     %s", dss->stencil_enabled ? "YES":"NO");
    igText("Stencil Read Mask:   0x%02X", dss->stencil_read_mask);
    igText("Stencil Write Mask:  0x%02X", dss->stencil_write_mask);
    igText("Stencil Ref:         0x%02X", dss->stencil_ref);
    if (igTreeNodeStr("Stencil Front")) {
        _sg_cimgui_draw_stencil_state(&dss->stencil_front);
        igTreePop();
    }
    if (igTreeNodeStr("Stencil Back")) {
        _sg_cimgui_draw_stencil_state(&dss->stencil_back);
        igTreePop();
    }
}

_SOKOL_PRIVATE void _sg_cimgui_draw_blend_state(const sg_blend_state* bs) {
    igText("Blend Enabled:    %s", bs->enabled ? "YES":"NO");
    igText("Src Factor RGB:   %s", _sg_cimgui_blendfactor_string(bs->src_factor_rgb));
    igText("Dst Factor RGB:   %s", _sg_cimgui_blendfactor_string(bs->dst_factor_rgb));
    igText("Op RGB:           %s", _sg_cimgui_blendop_string(bs->op_rgb));
    igText("Src Factor Alpha: %s", _sg_cimgui_blendfactor_string(bs->src_factor_alpha));
    igText("Dst Factor Alpha: %s", _sg_cimgui_blendfactor_string(bs->dst_factor_alpha));
    igText("Op Alpha:         %s", _sg_cimgui_blendop_string(bs->op_alpha));
    igText("Color Write Mask: %s", _sg_cimgui_colormask_string(bs->color_write_mask));
    igText("Attachment Count: %d", bs->color_attachment_count);
    igText("Color Format:     %s", _sg_cimgui_pixelformat_string(bs->color_format));
    igText("Depth Format:     %s", _sg_cimgui_pixelformat_string(bs->depth_format));
    igText("Blend Color:      %.3f %.3f %.3f %.3f", bs->blend_color[0], bs->blend_color[1], bs->blend_color[2], bs->blend_color[3]);
}

_SOKOL_PRIVATE void _sg_cimgui_draw_rasterizer_state(const sg_rasterizer_state* rs) {
    igText("Alpha to Coverage: %s", rs->alpha_to_coverage_enabled ? "YES":"NO");
    igText("Cull Mode:         %s", _sg_cimgui_cullmode_string(rs->cull_mode));
    igText("Face Winding:      %s", _sg_cimgui_facewinding_string(rs->face_winding));
    igText("Sample Count:      %d", rs->sample_count);
    igText("Depth Bias:        %f", rs->depth_bias);
    igText("Depth Bias Slope:  %f", rs->depth_bias_slope_scale);
    igText("Depth Bias Clamp:  %f", rs->depth_bias_clamp);
}

_SOKOL_PRIVATE void _sg_cimgui_draw_pipeline_panel(sg_cimgui_t* ctx, sg_pipeline pip) {
    if (pip.id != SG_INVALID_ID) {
        igBeginChild("pipeline", _sg_cimgui_VEC2_ZERO, false, ImGuiWindowFlags_None);
        sg_pipeline_info info = sg_query_pipeline_info(pip);
        if (info.slot.state == SG_RESOURCESTATE_VALID) {
            const sg_cimgui_pipeline_t* pip_ui = &ctx->pipelines.slots[_sg_cimgui_slot_index(pip.id)];
            igText("Label: %s", pip_ui->label.buf[0] ? pip_ui->label.buf : "---");
            _sg_cimgui_draw_resource_slot(&info.slot);
            igSeparator();
            igText("Shader:    "); igSameLine(0, -1);
            if (_sg_cimgui_draw_shader_link(ctx, pip_ui->desc.shader)) {
                _sg_cimgui_show_shader(ctx, pip_ui->desc.shader);
            }
            igText("Prim Type:  %s", _sg_cimgui_primitivetype_string(pip_ui->desc.primitive_type));
            igText("Index Type: %s", _sg_cimgui_indextype_string(pip_ui->desc.index_type));
            if (igTreeNodeStr("Vertex Layout")) {
                _sg_cimgui_draw_vertex_layout(&pip_ui->desc.layout);
                igTreePop();
            }
            if (igTreeNodeStr("Depth Stencil State")) {
                _sg_cimgui_draw_depth_stencil_state(&pip_ui->desc.depth_stencil);
                igTreePop();
            }
            if (igTreeNodeStr("Blend State")) {
                _sg_cimgui_draw_blend_state(&pip_ui->desc.blend);
                igTreePop();
            }
            if (igTreeNodeStr("Rasterizer State")) {
                _sg_cimgui_draw_rasterizer_state(&pip_ui->desc.rasterizer);
                igTreePop();
            }
        }
        else {
            igText("Pipeline 0x%08X not valid.", pip.id);
        }
        igEndChild();
    }
}

_SOKOL_PRIVATE void _sg_cimgui_draw_attachment(sg_cimgui_t* ctx, const sg_attachment_desc* att, float* img_scale) {
    igText("  Image: "); igSameLine(0, -1);
    if (_sg_cimgui_draw_image_link(ctx, att->image)) {
        _sg_cimgui_show_image(ctx, att->image);
    }
    igText("  Mip Level: %d", att->mip_level);
    igText("  Face/Layer/Slice: %d", att->layer);
    _sg_cimgui_draw_embedded_image(ctx, att->image, img_scale);
}

_SOKOL_PRIVATE void _sg_cimgui_draw_pass_panel(sg_cimgui_t* ctx, sg_pass pass) {
    if (pass.id != SG_INVALID_ID) {
        igBeginChild("pass", _sg_cimgui_VEC2_ZERO, false, ImGuiWindowFlags_None);
        sg_pass_info info = sg_query_pass_info(pass);
        if (info.slot.state == SG_RESOURCESTATE_VALID) {
            sg_cimgui_pass_t* pass_ui = &ctx->passes.slots[_sg_cimgui_slot_index(pass.id)];
            igText("Label: %s", pass_ui->label.buf[0] ? pass_ui->label.buf : "---");
            _sg_cimgui_draw_resource_slot(&info.slot);
            for (int i = 0; i < SG_MAX_COLOR_ATTACHMENTS; i++) {
                if (pass_ui->desc.color_attachments[i].image.id == SG_INVALID_ID) {
                    break;
                }
                igSeparator();
                igText("Color Attachment #%d:", i);
                _sg_cimgui_draw_attachment(ctx, &pass_ui->desc.color_attachments[i], &pass_ui->color_image_scale[i]);
            }
            if (pass_ui->desc.depth_stencil_attachment.image.id != SG_INVALID_ID) {
                igSeparator();
                igText("Depth-Stencil Attachemnt:");
                _sg_cimgui_draw_attachment(ctx, &pass_ui->desc.depth_stencil_attachment, &pass_ui->ds_image_scale);
            }
        }
        else {
            igText("Pass 0x%08X not valid.", pass.id);
        }
        igEndChild();
    }
}

_SOKOL_PRIVATE void _sg_cimgui_draw_bindings_panel(sg_cimgui_t* ctx, const sg_bindings* bnd) {
    for (int i = 0; i < SG_MAX_SHADERSTAGE_BUFFERS; i++) {
        sg_buffer buf = bnd->vertex_buffers[i];
        if (buf.id != SG_INVALID_ID) {
            igSeparator();
            igText("Vertex Buffer Slot #%d:", i);
            igText("  Buffer: "); igSameLine(0, -1);
            if (_sg_cimgui_draw_buffer_link(ctx, buf)) {
                _sg_cimgui_show_buffer(ctx, buf);
            }
            igText("  Offset: %d", bnd->vertex_buffer_offsets[i]);
        }
        else {
            break;
        }
    }
    if (bnd->index_buffer.id != SG_INVALID_ID) {
        sg_buffer buf = bnd->index_buffer;
        if (buf.id != SG_INVALID_ID) {
            igSeparator();
            igText("Index Buffer Slot:");
            igText("  Buffer: "); igSameLine(0, -1);
            if (_sg_cimgui_draw_buffer_link(ctx, buf)) {
                _sg_cimgui_show_buffer(ctx, buf);
            }
            igText("  Offset: %d", bnd->index_buffer_offset);
        }
    }
    for (int i = 0; i < SG_MAX_SHADERSTAGE_IMAGES; i++) {
        sg_image img = bnd->vs_images[i];
        if (img.id != SG_INVALID_ID) {
            igSeparator();
            igText("Vertex Stage Image Slot #%d:", i);
            igText("  Image: "); igSameLine(0, -1);
            if (_sg_cimgui_draw_image_link(ctx, img)) {
                _sg_cimgui_show_image(ctx, img);
            }
        }
        else {
            break;
        }
    }
    for (int i = 0; i < SG_MAX_SHADERSTAGE_IMAGES; i++) {
        sg_image img = bnd->fs_images[i];
        if (img.id != SG_INVALID_ID) {
            igSeparator();
            igText("Fragment Stage Image Slot #%d:", i);
            igText("  Image: "); igSameLine(0, -1);
            if (_sg_cimgui_draw_image_link(ctx, img)) {
                _sg_cimgui_show_image(ctx, img);
            }
        }
    }
}

_SOKOL_PRIVATE void _sg_cimgui_draw_uniforms_panel(sg_cimgui_t* ctx, const sg_cimgui_args_apply_uniforms_t* args) {
    SOKOL_ASSERT(args->ub_index < SG_MAX_SHADERSTAGE_BUFFERS);

    /* check if all the required information for drawing the structured uniform block content
        is available, otherwise just render a generic hexdump
    */
   if (sg_query_pipeline_state(args->pipeline) != SG_RESOURCESTATE_VALID) {
        igText("Pipeline object not valid!");
        return;
   }
    sg_cimgui_pipeline_t* pip_ui = &ctx->pipelines.slots[_sg_cimgui_slot_index(args->pipeline.id)];
    if (sg_query_shader_state(pip_ui->desc.shader) != SG_RESOURCESTATE_VALID) {
        igText("Shader object not valid!");
        return;
    }
    sg_cimgui_shader_t* shd_ui = &ctx->shaders.slots[_sg_cimgui_slot_index(pip_ui->desc.shader.id)];
    SOKOL_ASSERT(shd_ui->res_id.id == pip_ui->desc.shader.id);
    const sg_shader_uniform_block_desc* ub_desc = (args->stage == SG_SHADERSTAGE_VS) ?
        &shd_ui->desc.vs.uniform_blocks[args->ub_index] :
        &shd_ui->desc.fs.uniform_blocks[args->ub_index];
    SOKOL_ASSERT(args->num_bytes <= ub_desc->size);
    bool draw_dump = false;
    if (ub_desc->uniforms[0].type == SG_UNIFORMTYPE_INVALID) {
        draw_dump = true;
    }

    sg_cimgui_capture_bucket_t* bucket = _sg_cimgui_capture_get_read_bucket(ctx);
    SOKOL_ASSERT((args->ubuf_pos + args->num_bytes) <= bucket->ubuf_size);
    const float* uptrf = (const float*) (bucket->ubuf + args->ubuf_pos);
    if (!draw_dump) {
        for (int i = 0; i < SG_MAX_UB_MEMBERS; i++) {
            const sg_shader_uniform_desc* ud = &ub_desc->uniforms[i];
            if (ud->type == SG_UNIFORMTYPE_INVALID) {
                break;
            }
            int num_items = (ud->array_count > 1) ? ud->array_count : 1;
            if (num_items > 1) {
                igText("%d: %s %s[%d] =", i, _sg_cimgui_uniformtype_string(ud->type), ud->name?ud->name:"", ud->array_count);
            }
            else {
                igText("%d: %s %s =", i, _sg_cimgui_uniformtype_string(ud->type), ud->name?ud->name:"");
            }
            for (int i = 0; i < num_items; i++) {
                switch (ud->type) {
                    case SG_UNIFORMTYPE_FLOAT:
                        igText("    %.3f", *uptrf);
                        break;
                    case SG_UNIFORMTYPE_FLOAT2:
                        igText("    %.3f, %.3f", uptrf[0], uptrf[1]);
                        break;
                    case SG_UNIFORMTYPE_FLOAT3:
                        igText("    %.3f, %.3f, %.3f", uptrf[0], uptrf[1], uptrf[2]);
                        break;
                    case SG_UNIFORMTYPE_FLOAT4:
                        igText("    %.3f, %.3f, %.3f, %.3f", uptrf[0], uptrf[1], uptrf[2], uptrf[3]);
                        break;
                    case SG_UNIFORMTYPE_MAT4:
                        igText("    %.3f, %.3f, %.3f, %.3f\n"
                                    "    %.3f, %.3f, %.3f, %.3f\n"
                                    "    %.3f, %.3f, %.3f, %.3f\n"
                                    "    %.3f, %.3f, %.3f, %.3f",
                            uptrf[0],  uptrf[1],  uptrf[2],  uptrf[3],
                            uptrf[4],  uptrf[5],  uptrf[6],  uptrf[7],
                            uptrf[8],  uptrf[9],  uptrf[10], uptrf[11],
                            uptrf[12], uptrf[13], uptrf[14], uptrf[15]);
                        break;
                    default:
                        igText("???");
                        break;
                }
                uptrf += _sg_cimgui_uniform_size(ud->type, 1) / sizeof(float);
            }
        }
    }
    else {
        const uint32_t num_floats = ub_desc->size / sizeof(float);
        for (uint32_t i = 0; i < num_floats; i++) {
            igText("%.3f, ", uptrf[i]);
            if (((i + 1) % 4) != 0) {
                igSameLine(0, -1);
            }
        }
    }
}

_SOKOL_PRIVATE void _sg_cimgui_draw_passaction_panel(sg_cimgui_t* ctx, sg_pass pass, const sg_pass_action* action) {
    /* determine number of valid color attachments in the pass */
    int num_color_atts = 0;
    if (SG_INVALID_ID == pass.id) {
        /* default pass: one color attachment */
        num_color_atts = 1;
    }
    else {
        const sg_cimgui_pass_t* pass_ui = &ctx->passes.slots[_sg_cimgui_slot_index(pass.id)];
        for (int i = 0; i < SG_MAX_COLOR_ATTACHMENTS; i++) {
            if (pass_ui->desc.color_attachments[i].image.id != SG_INVALID_ID) {
                num_color_atts++;
            }
        }
    }

    igText("Pass Action: ");
    for (int i = 0; i < num_color_atts; i++) {
        const sg_color_attachment_action* c_att = &action->colors[i];
        igText("  Color Attachment %d:", i);
        switch (c_att->action) {
            case SG_ACTION_LOAD: igText("    SG_ACTION_LOAD"); break;
            case SG_ACTION_DONTCARE: igText("    SG_ACTION_DONTCARE"); break;
            default:
                igText("    SG_ACTION_CLEAR: %.3f, %.3f, %.3f, %.3f",
                    c_att->val[0],
                    c_att->val[1],
                    c_att->val[2],
                    c_att->val[3]);
                break;
        }
    }
    const sg_depth_attachment_action* d_att = &action->depth;
    igText("  Depth Attachment:");
    switch (d_att->action) {
        case SG_ACTION_LOAD: igText("    SG_ACTION_LOAD"); break;
        case SG_ACTION_DONTCARE: igText("    SG_ACTION_DONTCARE"); break;
        default: igText("    SG_ACTION_CLEAR: %.3f", d_att->val); break;
    }
    const sg_stencil_attachment_action* s_att = &action->stencil;
    igText("  Stencil Attachment");
    switch (s_att->action) {
        case SG_ACTION_LOAD: igText("    SG_ACTION_LOAD"); break;
        case SG_ACTION_DONTCARE: igText("    SG_ACTION_DONTCARE"); break;
        default: igText("    SG_ACTION_CLEAR: 0x%02X", s_att->val); break;
    }
}

_SOKOL_PRIVATE void _sg_cimgui_draw_capture_panel(sg_cimgui_t* ctx) {
    uint32_t sel_item_index = ctx->capture.sel_item;
    if (sel_item_index >= _sg_cimgui_capture_num_read_items(ctx)) {
        return;
    }
    sg_cimgui_capture_item_t* item = _sg_cimgui_capture_read_item_at(ctx, sel_item_index);
    igBeginChild("capture_item", _sg_cimgui_VEC2_ZERO, false,ImGuiWindowFlags_None );
    igPushStyleColorU32(ImGuiCol_Text, item->color);
    igText("%s", _sg_cimgui_capture_item_string(ctx, sel_item_index, item).buf);
    igPopStyleColor(1);
    igSeparator();
    switch (item->cmd) {
        case sg_cimgui_CMD_QUERY_FEATURE:
            break;
        case sg_cimgui_CMD_RESET_STATE_CACHE:
            break;
        case sg_cimgui_CMD_MAKE_BUFFER:
            _sg_cimgui_draw_buffer_panel(ctx, item->args.make_buffer.result);
            break;
        case sg_cimgui_CMD_MAKE_IMAGE:
            _sg_cimgui_draw_image_panel(ctx, item->args.make_image.result);
            break;
        case sg_cimgui_CMD_MAKE_SHADER:
            _sg_cimgui_draw_shader_panel(ctx, item->args.make_shader.result);
            break;
        case sg_cimgui_CMD_MAKE_PIPELINE:
            _sg_cimgui_draw_pipeline_panel(ctx, item->args.make_pipeline.result);
            break;
        case sg_cimgui_CMD_MAKE_PASS:
            _sg_cimgui_draw_pass_panel(ctx, item->args.make_pass.result);
            break;
        case sg_cimgui_CMD_DESTROY_BUFFER:
            _sg_cimgui_draw_buffer_panel(ctx, item->args.destroy_buffer.buffer);
            break;
        case sg_cimgui_CMD_DESTROY_IMAGE:
            _sg_cimgui_draw_image_panel(ctx, item->args.destroy_image.image);
            break;
        case sg_cimgui_CMD_DESTROY_SHADER:
            _sg_cimgui_draw_shader_panel(ctx, item->args.destroy_shader.shader);
            break;
        case sg_cimgui_CMD_DESTROY_PIPELINE:
            _sg_cimgui_draw_pipeline_panel(ctx, item->args.destroy_pipeline.pipeline);
            break;
        case sg_cimgui_CMD_DESTROY_PASS:
            _sg_cimgui_draw_pass_panel(ctx, item->args.destroy_pass.pass);
            break;
        case sg_cimgui_CMD_UPDATE_BUFFER:
            _sg_cimgui_draw_buffer_panel(ctx, item->args.update_buffer.buffer);
            break;
        case sg_cimgui_CMD_UPDATE_IMAGE:
            _sg_cimgui_draw_image_panel(ctx, item->args.update_image.image);
            break;
        case sg_cimgui_CMD_APPEND_BUFFER:
            _sg_cimgui_draw_buffer_panel(ctx, item->args.update_buffer.buffer);
            break;
        case sg_cimgui_CMD_BEGIN_DEFAULT_PASS:
            {
                sg_pass inv_pass = { SG_INVALID_ID };
                _sg_cimgui_draw_passaction_panel(ctx, inv_pass, &item->args.begin_default_pass.action);
            }
            break;
        case sg_cimgui_CMD_BEGIN_PASS:
            _sg_cimgui_draw_passaction_panel(ctx, item->args.begin_pass.pass, &item->args.begin_pass.action);
            igSeparator();
            _sg_cimgui_draw_pass_panel(ctx, item->args.begin_pass.pass);
            break;
        case sg_cimgui_CMD_APPLY_VIEWPORT:
        case sg_cimgui_CMD_APPLY_SCISSOR_RECT:
            break;
        case sg_cimgui_CMD_APPLY_PIPELINE:
            _sg_cimgui_draw_pipeline_panel(ctx, item->args.apply_pipeline.pipeline);
            break;
        case sg_cimgui_CMD_APPLY_BINDINGS:
            _sg_cimgui_draw_bindings_panel(ctx, &item->args.apply_bindings.bindings);
            break;
        case sg_cimgui_CMD_APPLY_UNIFORMS:
            _sg_cimgui_draw_uniforms_panel(ctx, &item->args.apply_uniforms);
            break;
        case sg_cimgui_CMD_DRAW:
        case sg_cimgui_CMD_END_PASS:
        case sg_cimgui_CMD_COMMIT:
            break;
        case sg_cimgui_CMD_ALLOC_BUFFER:
            _sg_cimgui_draw_buffer_panel(ctx, item->args.alloc_buffer.result);
            break;
        case sg_cimgui_CMD_ALLOC_IMAGE:
            _sg_cimgui_draw_image_panel(ctx, item->args.alloc_image.result);
            break;
        case sg_cimgui_CMD_ALLOC_SHADER:
            _sg_cimgui_draw_shader_panel(ctx, item->args.alloc_shader.result);
            break;
        case sg_cimgui_CMD_ALLOC_PIPELINE:
            _sg_cimgui_draw_pipeline_panel(ctx, item->args.alloc_pipeline.result);
            break;
        case sg_cimgui_CMD_ALLOC_PASS:
            _sg_cimgui_draw_pass_panel(ctx, item->args.alloc_pass.result);
            break;
        case sg_cimgui_CMD_INIT_BUFFER:
            _sg_cimgui_draw_buffer_panel(ctx, item->args.init_buffer.buffer);
            break;
        case sg_cimgui_CMD_INIT_IMAGE:
            _sg_cimgui_draw_image_panel(ctx, item->args.init_image.image);
            break;
        case sg_cimgui_CMD_INIT_SHADER:
            _sg_cimgui_draw_shader_panel(ctx, item->args.init_shader.shader);
            break;
        case sg_cimgui_CMD_INIT_PIPELINE:
            _sg_cimgui_draw_pipeline_panel(ctx, item->args.init_pipeline.pipeline);
            break;
        case sg_cimgui_CMD_INIT_PASS:
            _sg_cimgui_draw_pass_panel(ctx, item->args.init_pass.pass);
            break;
        case sg_cimgui_CMD_FAIL_BUFFER:
            _sg_cimgui_draw_buffer_panel(ctx, item->args.fail_buffer.buffer);
            break;
        case sg_cimgui_CMD_FAIL_IMAGE:
            _sg_cimgui_draw_image_panel(ctx, item->args.fail_image.image);
            break;
        case sg_cimgui_CMD_FAIL_SHADER:
            _sg_cimgui_draw_shader_panel(ctx, item->args.fail_shader.shader);
            break;
        case sg_cimgui_CMD_FAIL_PIPELINE:
            _sg_cimgui_draw_pipeline_panel(ctx, item->args.fail_pipeline.pipeline);
            break;
        case sg_cimgui_CMD_FAIL_PASS:
            _sg_cimgui_draw_pass_panel(ctx, item->args.fail_pass.pass);
            break;
        default:
            break;
    }
    igEndChild();
}

/*--- PUBLIC FUNCTIONS -------------------------------------------------------*/
SOKOL_API_IMPL void sg_cimgui_init(sg_cimgui_t* ctx) {
    SOKOL_ASSERT(ctx);
    memset(ctx, 0, sizeof(sg_cimgui_t));
    ctx->init_tag = 0xABCDABCD;
    _sg_cimgui_capture_init(ctx);

    /* hook into sokol_gfx functions */
    sg_trace_hooks hooks;
    memset(&hooks, 0, sizeof(hooks));
    hooks.user_data = (void*) ctx;
    hooks.query_feature = _sg_cimgui_query_feature;
    hooks.reset_state_cache = _sg_cimgui_reset_state_cache;
    hooks.make_buffer = _sg_cimgui_make_buffer;
    hooks.make_image = _sg_cimgui_make_image;
    hooks.make_shader = _sg_cimgui_make_shader;
    hooks.make_pipeline = _sg_cimgui_make_pipeline;
    hooks.make_pass = _sg_cimgui_make_pass;
    hooks.destroy_buffer = _sg_cimgui_destroy_buffer;
    hooks.destroy_image = _sg_cimgui_destroy_image;
    hooks.destroy_shader = _sg_cimgui_destroy_shader;
    hooks.destroy_pipeline = _sg_cimgui_destroy_pipeline;
    hooks.destroy_pass = _sg_cimgui_destroy_pass;
    hooks.update_buffer = _sg_cimgui_update_buffer;
    hooks.update_image = _sg_cimgui_update_image;
    hooks.append_buffer = _sg_cimgui_append_buffer;
    hooks.begin_default_pass = _sg_cimgui_begin_default_pass;
    hooks.begin_pass = _sg_cimgui_begin_pass;
    hooks.apply_viewport = _sg_cimgui_apply_viewport;
    hooks.apply_scissor_rect = _sg_cimgui_apply_scissor_rect;
    hooks.apply_pipeline = _sg_cimgui_apply_pipeline;
    hooks.apply_bindings = _sg_cimgui_apply_bindings;
    hooks.apply_uniforms = _sg_cimgui_apply_uniforms;
    hooks.draw = _sg_cimgui_draw;
    hooks.end_pass = _sg_cimgui_end_pass;
    hooks.commit = _sg_cimgui_commit;
    hooks.alloc_buffer = _sg_cimgui_alloc_buffer;
    hooks.alloc_image = _sg_cimgui_alloc_image;
    hooks.alloc_shader = _sg_cimgui_alloc_shader;
    hooks.alloc_pipeline = _sg_cimgui_alloc_pipeline;
    hooks.alloc_pass = _sg_cimgui_alloc_pass;
    hooks.init_buffer = _sg_cimgui_init_buffer;
    hooks.init_image = _sg_cimgui_init_image;
    hooks.init_shader = _sg_cimgui_init_shader;
    hooks.init_pipeline = _sg_cimgui_init_pipeline;
    hooks.init_pass = _sg_cimgui_init_pass;
    hooks.fail_buffer = _sg_cimgui_fail_buffer;
    hooks.fail_image = _sg_cimgui_fail_image;
    hooks.fail_shader = _sg_cimgui_fail_shader;
    hooks.fail_pipeline = _sg_cimgui_fail_pipeline;
    hooks.fail_pass = _sg_cimgui_fail_pass;
    hooks.push_debug_group = _sg_cimgui_push_debug_group;
    hooks.pop_debug_group = _sg_cimgui_pop_debug_group;
    hooks.err_buffer_pool_exhausted = _sg_cimgui_err_buffer_pool_exhausted;
    hooks.err_image_pool_exhausted = _sg_cimgui_err_image_pool_exhausted;
    hooks.err_shader_pool_exhausted = _sg_cimgui_err_shader_pool_exhausted;
    hooks.err_pipeline_pool_exhausted = _sg_cimgui_err_pipeline_pool_exhausted;
    hooks.err_pass_pool_exhausted = _sg_cimgui_err_pass_pool_exhausted;
    hooks.err_context_mismatch = _sg_cimgui_err_context_mismatch;
    hooks.err_pass_invalid = _sg_cimgui_err_pass_invalid;
    hooks.err_draw_invalid = _sg_cimgui_err_draw_invalid;
    hooks.err_bindings_invalid = _sg_cimgui_err_bindings_invalid;
    ctx->hooks = sg_install_trace_hooks(&hooks);

    /* allocate resource debug-info slots */
    sg_desc desc = sg_query_desc();
    ctx->buffers.num_slots = desc.buffer_pool_size;
    ctx->images.num_slots = desc.image_pool_size;
    ctx->shaders.num_slots = desc.shader_pool_size;
    ctx->pipelines.num_slots = desc.pipeline_pool_size;
    ctx->passes.num_slots = desc.pass_pool_size;

    const int buffer_pool_size = ctx->buffers.num_slots * sizeof(sg_cimgui_buffer_t);
    ctx->buffers.slots = (sg_cimgui_buffer_t*) _sg_cimgui_alloc(buffer_pool_size);
    SOKOL_ASSERT(ctx->buffers.slots);
    memset(ctx->buffers.slots, 0, buffer_pool_size);

    const int image_pool_size = ctx->images.num_slots * sizeof(sg_cimgui_image_t);
    ctx->images.slots = (sg_cimgui_image_t*) _sg_cimgui_alloc(image_pool_size);
    SOKOL_ASSERT(ctx->images.slots);
    memset(ctx->images.slots, 0, image_pool_size);

    const int shader_pool_size = ctx->shaders.num_slots * sizeof(sg_cimgui_shader_t);
    ctx->shaders.slots = (sg_cimgui_shader_t*) _sg_cimgui_alloc(shader_pool_size);
    SOKOL_ASSERT(ctx->shaders.slots);
    memset(ctx->shaders.slots, 0, shader_pool_size);

    const int pipeline_pool_size = ctx->pipelines.num_slots * sizeof(sg_cimgui_pipeline_t);
    ctx->pipelines.slots = (sg_cimgui_pipeline_t*) _sg_cimgui_alloc(pipeline_pool_size);
    SOKOL_ASSERT(ctx->pipelines.slots);
    memset(ctx->pipelines.slots, 0, pipeline_pool_size);

    const int pass_pool_size = ctx->passes.num_slots * sizeof(sg_cimgui_pass_t);
    ctx->passes.slots = (sg_cimgui_pass_t*) _sg_cimgui_alloc(pass_pool_size);
    SOKOL_ASSERT(ctx->passes.slots);
    memset(ctx->passes.slots, 0, pass_pool_size);
}

SOKOL_API_IMPL void sg_cimgui_discard(sg_cimgui_t* ctx) {
    SOKOL_ASSERT(ctx && (ctx->init_tag == 0xABCDABCD));
    /* restore original trace hooks */
    sg_install_trace_hooks(&ctx->hooks);
    ctx->init_tag = 0;
    _sg_cimgui_capture_discard(ctx);
    if (ctx->buffers.slots) {
        for (int i = 0; i < ctx->buffers.num_slots; i++) {
            if (ctx->buffers.slots[i].res_id.id != SG_INVALID_ID) {
                _sg_cimgui_buffer_destroyed(ctx, i);
            }
        }
        _sg_cimgui_free((void*)ctx->buffers.slots);
        ctx->buffers.slots = 0;
    }
    if (ctx->images.slots) {
        for (int i = 0; i < ctx->images.num_slots; i++) {
            if (ctx->images.slots[i].res_id.id != SG_INVALID_ID) {
                _sg_cimgui_image_destroyed(ctx, i);
            }
        }
        _sg_cimgui_free((void*)ctx->images.slots);
        ctx->images.slots = 0;
    }
    if (ctx->shaders.slots) {
        for (int i = 0; i < ctx->shaders.num_slots; i++) {
            if (ctx->shaders.slots[i].res_id.id != SG_INVALID_ID) {
                _sg_cimgui_shader_destroyed(ctx, i);
            }
        }
        _sg_cimgui_free((void*)ctx->shaders.slots);
        ctx->shaders.slots = 0;
    }
    if (ctx->pipelines.slots) {
        for (int i = 0; i < ctx->pipelines.num_slots; i++) {
            if (ctx->pipelines.slots[i].res_id.id != SG_INVALID_ID) {
                _sg_cimgui_pipeline_destroyed(ctx, i);
            }
        }
        _sg_cimgui_free((void*)ctx->pipelines.slots);
        ctx->pipelines.slots = 0;
    }
    if (ctx->passes.slots) {
        for (int i = 0; i < ctx->passes.num_slots; i++) {
            if (ctx->passes.slots[i].res_id.id != SG_INVALID_ID) {
                _sg_cimgui_pass_destroyed(ctx, i);
            }
        }
        _sg_cimgui_free((void*)ctx->passes.slots);
        ctx->passes.slots = 0;
    }
}

SOKOL_API_IMPL void sg_cimgui_draw(sg_cimgui_t* ctx) {
    SOKOL_ASSERT(ctx && (ctx->init_tag == 0xABCDABCD));
    sg_cimgui_draw_buffers_window(ctx);
    sg_cimgui_draw_images_window(ctx);
    sg_cimgui_draw_shaders_window(ctx);
    sg_cimgui_draw_pipelines_window(ctx);
    sg_cimgui_draw_passes_window(ctx);
    sg_cimgui_draw_capture_window(ctx);
}

SOKOL_API_IMPL void sg_cimgui_draw_buffers_window(sg_cimgui_t* ctx) {
    SOKOL_ASSERT(ctx && (ctx->init_tag == 0xABCDABCD));
    if (!ctx->buffers.open) {
        return;
    }
	igSetNextWindowSize((ImVec2){440, 280}, ImGuiCond_Once);
    if (igBegin("Buffers", &ctx->buffers.open, ImGuiWindowFlags_None)) {
        sg_cimgui_draw_buffers_content(ctx);
    }
    igEnd();
}

SOKOL_API_IMPL void sg_cimgui_draw_images_window(sg_cimgui_t* ctx) {
    SOKOL_ASSERT(ctx && (ctx->init_tag == 0xABCDABCD));
    if (!ctx->images.open) {
        return;
    }
	igSetNextWindowSize((ImVec2){440, 400}, ImGuiCond_Once);
    if (igBegin("Images", &ctx->images.open, ImGuiWindowFlags_None)) {
        sg_cimgui_draw_images_content(ctx);
    }
    igEnd();
}

SOKOL_API_IMPL void sg_cimgui_draw_shaders_window(sg_cimgui_t* ctx) {
    SOKOL_ASSERT(ctx && (ctx->init_tag == 0xABCDABCD));
    if (!ctx->shaders.open) {
        return;
    }
	igSetNextWindowSize((ImVec2){440, 400}, ImGuiCond_Once);
    if (igBegin("Shaders", &ctx->shaders.open, ImGuiWindowFlags_None)) {
        sg_cimgui_draw_shaders_content(ctx);
    }
    igEnd();
}

SOKOL_API_IMPL void sg_cimgui_draw_pipelines_window(sg_cimgui_t* ctx) {
    SOKOL_ASSERT(ctx && (ctx->init_tag == 0xABCDABCD));
    if (!ctx->pipelines.open) {
        return;
    }
	igSetNextWindowSize((ImVec2){540, 400}, ImGuiCond_Once);
    if (igBegin("Pipelines", &ctx->pipelines.open, ImGuiWindowFlags_None)) {
        sg_cimgui_draw_pipelines_content(ctx);
    }
    igEnd();
}

SOKOL_API_IMPL void sg_cimgui_draw_passes_window(sg_cimgui_t* ctx) {
    SOKOL_ASSERT(ctx && (ctx->init_tag == 0xABCDABCD));
    if (!ctx->passes.open) {
        return;
    }
	igSetNextWindowSize((ImVec2){440, 400}, ImGuiCond_Once);
    if (igBegin("Passes", &ctx->passes.open, ImGuiWindowFlags_None)) {
        sg_cimgui_draw_passes_content(ctx);
    }
    igEnd();
}

SOKOL_API_IMPL void sg_cimgui_draw_capture_window(sg_cimgui_t* ctx) {
    SOKOL_ASSERT(ctx && (ctx->init_tag == 0xABCDABCD));
    if (!ctx->capture.open) {
        return;
    }
	igSetNextWindowSize((ImVec2){640, 400}, ImGuiCond_Once);
    if (igBegin("Frame Capture", &ctx->capture.open, ImGuiWindowFlags_None)) {
        sg_cimgui_draw_capture_content(ctx);
    }
    igEnd();
}

SOKOL_API_IMPL void sg_cimgui_draw_buffers_content(sg_cimgui_t* ctx) {
    SOKOL_ASSERT(ctx && (ctx->init_tag == 0xABCDABCD));
    _sg_cimgui_draw_buffer_list(ctx);
    igSameLine(0, -1);
    _sg_cimgui_draw_buffer_panel(ctx, ctx->buffers.sel_buf);
}

SOKOL_API_IMPL void sg_cimgui_draw_images_content(sg_cimgui_t* ctx) {
    SOKOL_ASSERT(ctx && (ctx->init_tag == 0xABCDABCD));
    _sg_cimgui_draw_image_list(ctx);
    igSameLine(0, -1);
    _sg_cimgui_draw_image_panel(ctx, ctx->images.sel_img);
}

SOKOL_API_IMPL void sg_cimgui_draw_shaders_content(sg_cimgui_t* ctx) {
    SOKOL_ASSERT(ctx && (ctx->init_tag == 0xABCDABCD));
    _sg_cimgui_draw_shader_list(ctx);
    igSameLine(0, -1);
    _sg_cimgui_draw_shader_panel(ctx, ctx->shaders.sel_shd);
}

SOKOL_API_IMPL void sg_cimgui_draw_pipelines_content(sg_cimgui_t* ctx) {
    SOKOL_ASSERT(ctx && (ctx->init_tag == 0xABCDABCD));
    _sg_cimgui_draw_pipeline_list(ctx);
    igSameLine(0, -1);
    _sg_cimgui_draw_pipeline_panel(ctx, ctx->pipelines.sel_pip);
}

SOKOL_API_IMPL void sg_cimgui_draw_passes_content(sg_cimgui_t* ctx) {
    SOKOL_ASSERT(ctx && (ctx->init_tag == 0xABCDABCD));
    _sg_cimgui_draw_pass_list(ctx);
    igSameLine(0, -1);
    _sg_cimgui_draw_pass_panel(ctx, ctx->passes.sel_pass);
}

SOKOL_API_IMPL void sg_cimgui_draw_capture_content(sg_cimgui_t* ctx) {
    SOKOL_ASSERT(ctx && (ctx->init_tag == 0xABCDABCD));
    _sg_cimgui_draw_capture_list(ctx);
    igSameLine(0, -1);
    _sg_cimgui_draw_capture_panel(ctx);
}

#endif /* SOKOL_GFX_CIMGUI_IMPL */
