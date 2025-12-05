#if defined(SOKOL_IMPL) && !defined(SOKOL_GFX_IMGUI_IMPL)
#define SOKOL_GFX_IMGUI_IMPL
#endif
#ifndef SOKOL_GFX_IMGUI_INCLUDED
/*
    sokol_gfx_imgui.h -- debug-inspection UI for sokol_gfx.h using Dear ImGui

    Project URL: https://github.com/floooh/sokol

    Do this:
        #define SOKOL_IMPL or
        #define SOKOL_GFX_IMGUI_IMPL

    before you include this file in *one* C or C++ file to create the
    implementation.

    NOTE that the implementation can be compiled either as C++ or as C.
    When compiled as C++, sokol_gfx_imgui.h will directly call into the
    Dear ImGui C++ API. When compiled as C, sokol_gfx_imgui.h will call
    cimgui.h functions instead.

    Include the following file(s) before including sokol_gfx_imgui.h:

        sokol_gfx.h

    Additionally, include the following headers before including the
    implementation:

    If the implementation is compiled as C++:
        imgui.h

    If the implementation is compiled as C:
        cimgui.h

    The sokol_gfx.h implementation must be compiled with debug trace hooks
    enabled by defining:

        SOKOL_TRACE_HOOKS

    ...before including the sokol_gfx.h implementation.

    Before including the sokol_gfx_imgui.h implementation, optionally
    override the following macros:

        SOKOL_ASSERT(c)     -- your own assert macro, default: assert(c)
        SOKOL_UNREACHABLE   -- your own macro to annotate unreachable code,
                               default: SOKOL_ASSERT(false)
        SOKOL_GFX_IMGUI_API_DECL    - public function declaration prefix (default: extern)
        SOKOL_GFX_IMGUI_CPREFIX     - defines the function prefix for the Dear ImGui C bindings (default: ig)
        SOKOL_API_DECL      - same as SOKOL_GFX_IMGUI_API_DECL
        SOKOL_API_IMPL      - public function implementation prefix (default: -)

    If sokol_gfx_imgui.h is compiled as a DLL, define the following before
    including the declaration or implementation:

    SOKOL_DLL

    On Windows, SOKOL_DLL will define SOKOL_GFX_IMGUI_API_DECL as __declspec(dllexport)
    or __declspec(dllimport) as needed.

    STEP BY STEP:
    =============
    --- call sgimgui_init() with optional allocator overrides:

            sgimgui_init(&(sgimgui_desc_t){
                .allocator = {
                    .alloc_fn = my_malloc,
                    .free_fn = my_free,
                }
            });

    --- somewhere in the per-frame code call:

            sgimgui_draw()

        this won't draw anything yet, since no windows are open.

    --- call the convenience function sgimgui_draw_menu(ctx, title)
        to render a menu which allows to open/close the provided debug windows

            sgimgui_draw_menu("sokol-gfx");

    --- alternatively the individual single menu items via:

        if (ImGui::BeginMainMenuBar()) {
            if (ImGui::BeginMenu("sokol-gfx")) {
                sgimgui_draw_buffer_window_menu_item("Buffers");
                sgimgui_draw_image_window_menu_item("Images");
                sgimgui_draw_sampler_window_menu_item("Samplers");
                sgimgui_draw_shader_window_menu_item("Shaders");
                sgimgui_draw_pipeline_window_menu_item("Pipelines");
                sgimgui_draw_view_window_menu_item("Views");
                sgimgui_draw_capture_window_menu_item("Calls");
                sgimgui_draw_capabilities_window_menu_item("Capabilities");
                sgimgui_draw_frame_stats_window_menu_item("Frame Stats");
                ImGui::EndMenu();
            }
            ImGui::EndMainMenuBar();
        }

    --- before application shutdown, call:

            sgimgui_discard();

        ...this is not strictly necessary because the application exits
        anyway, but not doing this may trigger memory leak detection tools.

    --- finally, your application needs an ImGui renderer, you can either
        provide your own, or drop in the sokol_imgui.h utility header

    ALTERNATIVE DRAWING FUNCTIONS:
    ==============================
    Instead of the convenient but all-in-one sgimgui_draw() function,
    you can also use the following granular functions which might allow
    better integration with your existing UI:

    The following functions only render the window *content* (so you
    can integrate the UI into you own windows):

        void sgimgui_draw_buffer_window_content();
        void sgimgui_draw_image_window_content();
        void sgimgui_draw_sampler_window_content();
        void sgimgui_draw_shader_window_content();
        void sgimgui_draw_pipeline_window_content();
        void sgimgui_draw_view_window_content();
        void sgimgui_draw_capture_window_content();
        void sgimgui_draw_capabilities_window_content();
        void sgimgui_draw_frame_stats_window_content();

    And these are the 'full window' drawing functions:

        void sgimgui_draw_buffer_window("Buffers");
        void sgimgui_draw_image_window("Images");
        void sgimgui_draw_sampler_window("Samplers");
        void sgimgui_draw_shader_window("Shaders");
        void sgimgui_draw_pipeline_window("Pipelines");
        void sgimgui_draw_view_window("Views");
        void sgimgui_draw_capture_window("Frame Capture");
        void sgimgui_draw_capabilities_window("Capabilities");
        void sgimgui_draw_frame_stats_window("Frame Stats");

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
#define SOKOL_GFX_IMGUI_INCLUDED (1)
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h> // size_t

#if !defined(SOKOL_GFX_INCLUDED)
#error "Please include sokol_gfx.h before sokol_gfx_imgui.h"
#endif

#if defined(SOKOL_API_DECL) && !defined(SOKOL_GFX_IMGUI_API_DECL)
#define SOKOL_GFX_IMGUI_API_DECL SOKOL_API_DECL
#endif
#ifndef SOKOL_GFX_IMGUI_API_DECL
#if defined(_WIN32) && defined(SOKOL_DLL) && defined(SOKOL_GFX_IMGUI_IMPL)
#define SOKOL_GFX_IMGUI_API_DECL __declspec(dllexport)
#elif defined(_WIN32) && defined(SOKOL_DLL)
#define SOKOL_GFX_IMGUI_API_DECL __declspec(dllimport)
#else
#define SOKOL_GFX_IMGUI_API_DECL extern
#endif
#endif

#if defined(__cplusplus)
extern "C" {
#endif

/*
    sgimgui_allocator_t

    Used in sgimgui_desc_t to provide custom memory-alloc and -free functions
    to sokol_gfx_imgui.h. If memory management should be overridden, both the
    alloc and free function must be provided (e.g. it's not valid to
    override one function but not the other).
*/
typedef struct sgimgui_allocator_t {
    void* (*alloc_fn)(size_t size, void* user_data);
    void (*free_fn)(void* ptr, void* user_data);
    void* user_data;
} sgimgui_allocator_t;

/*
    sgimgui_desc_t

    Initialization options for sgimgui_init().
*/
typedef struct sgimgui_desc_t {
    sgimgui_allocator_t allocator; // optional memory allocation overrides (default: malloc/free)
} sgimgui_desc_t;

SOKOL_GFX_IMGUI_API_DECL void sgimgui_setup(const sgimgui_desc_t* desc);
SOKOL_GFX_IMGUI_API_DECL void sgimgui_shutdown(void);
SOKOL_GFX_IMGUI_API_DECL void sgimgui_draw(void);
SOKOL_GFX_IMGUI_API_DECL void sgimgui_draw_menu(const char* title);

SOKOL_GFX_IMGUI_API_DECL void sgimgui_draw_buffer_window_content(void);
SOKOL_GFX_IMGUI_API_DECL void sgimgui_draw_image_window_content(void);
SOKOL_GFX_IMGUI_API_DECL void sgimgui_draw_sampler_window_content(void);
SOKOL_GFX_IMGUI_API_DECL void sgimgui_draw_shader_window_content(void);
SOKOL_GFX_IMGUI_API_DECL void sgimgui_draw_pipeline_window_content(void);
SOKOL_GFX_IMGUI_API_DECL void sgimgui_draw_view_window_content(void);
SOKOL_GFX_IMGUI_API_DECL void sgimgui_draw_capture_window_content(void);
SOKOL_GFX_IMGUI_API_DECL void sgimgui_draw_capabilities_window_content(void);
SOKOL_GFX_IMGUI_API_DECL void sgimgui_draw_frame_stats_window_content(void);

SOKOL_GFX_IMGUI_API_DECL void sgimgui_draw_buffer_window(const char* title);
SOKOL_GFX_IMGUI_API_DECL void sgimgui_draw_image_window(const char* title);
SOKOL_GFX_IMGUI_API_DECL void sgimgui_draw_sampler_window(const char* title);
SOKOL_GFX_IMGUI_API_DECL void sgimgui_draw_shader_window(const char* title);
SOKOL_GFX_IMGUI_API_DECL void sgimgui_draw_pipeline_window(const char* title);
SOKOL_GFX_IMGUI_API_DECL void sgimgui_draw_view_window(const char* title);
SOKOL_GFX_IMGUI_API_DECL void sgimgui_draw_capture_window(const char* title);
SOKOL_GFX_IMGUI_API_DECL void sgimgui_draw_capabilities_window(const char* title);
SOKOL_GFX_IMGUI_API_DECL void sgimgui_draw_frame_stats_window(const char* title);

SOKOL_GFX_IMGUI_API_DECL void sgimgui_draw_buffer_menu_item(const char* label);
SOKOL_GFX_IMGUI_API_DECL void sgimgui_draw_image_menu_item(const char* label);
SOKOL_GFX_IMGUI_API_DECL void sgimgui_draw_sampler_menu_item(const char* label);
SOKOL_GFX_IMGUI_API_DECL void sgimgui_draw_shader_menu_item(const char* label);
SOKOL_GFX_IMGUI_API_DECL void sgimgui_draw_pipeline_menu_item(const char* label);
SOKOL_GFX_IMGUI_API_DECL void sgimgui_draw_view_menu_item(const char* label);
SOKOL_GFX_IMGUI_API_DECL void sgimgui_draw_capture_menu_item(const char* label);
SOKOL_GFX_IMGUI_API_DECL void sgimgui_draw_capabilities_menu_item(const char* label);
SOKOL_GFX_IMGUI_API_DECL void sgimgui_draw_frame_stats_menu_item(const char* label);

#if defined(__cplusplus)
} // extern "C"

// reference-based equivalents for c++
inline void sgimgui_setup(const sgimgui_desc_t& desc) { return sgimgui_setup(&desc); }

#endif
#endif /* SOKOL_GFX_IMGUI_INCLUDED */

/*=== IMPLEMENTATION =========================================================*/
#ifdef SOKOL_GFX_IMGUI_IMPL
#define SOKOL_GFX_IMGUI_IMPL_INCLUDED (1)

#if defined(SOKOL_MALLOC) || defined(SOKOL_CALLOC) || defined(SOKOL_FREE)
#error "SOKOL_MALLOC/CALLOC/FREE macros are no longer supported, please use sgimgui_desc_t.allocator to override memory allocation functions"
#endif

#if defined(__cplusplus)
    #if !defined(IMGUI_VERSION)
    #error "Please include imgui.h before the sokol_imgui.h implementation"
    #endif
#else
    #if !defined(CIMGUI_API)
    #error "Please include cimgui.h before the sokol_imgui.h implementation"
    #endif
#endif
#ifndef SOKOL_ASSERT
    #include <assert.h>
    #define SOKOL_ASSERT(c) assert(c)
#endif
#ifndef SOKOL_UNREACHABLE
    #define SOKOL_UNREACHABLE SOKOL_ASSERT(false)
#endif
#ifndef _SOKOL_PRIVATE
    #if defined(__GNUC__) || defined(__clang__)
        #define _SOKOL_PRIVATE __attribute__((unused)) static
    #else
        #define _SOKOL_PRIVATE static
    #endif
#endif
#ifndef _SOKOL_UNUSED
#define _SOKOL_UNUSED(x) (void)(x)
#endif
#ifndef SOKOL_API_IMPL
#define SOKOL_API_IMPL
#endif

#include <string.h>
#include <stdio.h>      // snprintf
#include <stdlib.h>     // malloc, free

#define _SGIMGUI_STRBUF_LEN (96)
/* max number of captured calls per frame */
#define _SGIMGUI_MAX_FRAMECAPTURE_ITEMS (4096)

typedef struct {
    char buf[_SGIMGUI_STRBUF_LEN];
} _sgimgui_str_t;

typedef struct {
    sg_buffer res_id;
    _sgimgui_str_t label;
    sg_buffer_desc desc;
} _sgimgui_buffer_t;

typedef struct {
    sg_image res_id;
    float ui_scale;
    _sgimgui_str_t label;
    sg_image_desc desc;
} _sgimgui_image_t;

typedef struct {
    sg_sampler res_id;
    _sgimgui_str_t label;
    sg_sampler_desc desc;
} _sgimgui_sampler_t;

typedef struct {
    sg_shader res_id;
    _sgimgui_str_t label;
    _sgimgui_str_t vs_entry;
    _sgimgui_str_t vs_d3d11_target;
    _sgimgui_str_t fs_entry;
    _sgimgui_str_t fs_d3d11_target;
    _sgimgui_str_t glsl_texture_sampler_name[SG_MAX_TEXTURE_SAMPLER_PAIRS];
    _sgimgui_str_t glsl_uniform_name[SG_MAX_UNIFORMBLOCK_BINDSLOTS][SG_MAX_UNIFORMBLOCK_MEMBERS];
    _sgimgui_str_t attr_glsl_name[SG_MAX_VERTEX_ATTRIBUTES];
    _sgimgui_str_t attr_hlsl_sem_name[SG_MAX_VERTEX_ATTRIBUTES];
    sg_shader_desc desc;
} _sgimgui_shader_t;

typedef struct {
    sg_pipeline res_id;
    _sgimgui_str_t label;
    sg_pipeline_desc desc;
} _sgimgui_pipeline_t;

typedef struct {
    sg_view res_id;
    float ui_scale;
    _sgimgui_str_t label;
    sg_view_desc desc;
} _sgimgui_view_t;

typedef struct {
    bool open;
    sg_buffer sel_buf;
    int num_slots;
    _sgimgui_buffer_t* slots;
} _sgimgui_buffer_window_t;

typedef struct {
    bool open;
    sg_image sel_img;
    int num_slots;
    _sgimgui_image_t* slots;
} _sgimgui_image_window_t;

typedef struct {
    bool open;
    sg_sampler sel_smp;
    int num_slots;
    _sgimgui_sampler_t* slots;
} _sgimgui_sampler_window_t;

typedef struct {
    bool open;
    sg_shader sel_shd;
    int num_slots;
    _sgimgui_shader_t* slots;
} _sgimgui_shader_window_t;

typedef struct {
    bool open;
    sg_pipeline sel_pip;
    int num_slots;
    _sgimgui_pipeline_t* slots;
} _sgimgui_pipeline_window_t;

typedef struct {
    bool open;
    sg_view sel_view;
    int num_slots;
    _sgimgui_view_t* slots;
} _sgimgui_view_window_t;

typedef enum {
    _SGIMGUI_CMD_INVALID,
    _SGIMGUI_CMD_RESET_STATE_CACHE,
    _SGIMGUI_CMD_MAKE_BUFFER,
    _SGIMGUI_CMD_MAKE_IMAGE,
    _SGIMGUI_CMD_MAKE_SAMPLER,
    _SGIMGUI_CMD_MAKE_SHADER,
    _SGIMGUI_CMD_MAKE_PIPELINE,
    _SGIMGUI_CMD_MAKE_VIEW,
    _SGIMGUI_CMD_DESTROY_BUFFER,
    _SGIMGUI_CMD_DESTROY_IMAGE,
    _SGIMGUI_CMD_DESTROY_SAMPLER,
    _SGIMGUI_CMD_DESTROY_SHADER,
    _SGIMGUI_CMD_DESTROY_PIPELINE,
    _SGIMGUI_CMD_DESTROY_VIEW,
    _SGIMGUI_CMD_UPDATE_BUFFER,
    _SGIMGUI_CMD_UPDATE_IMAGE,
    _SGIMGUI_CMD_APPEND_BUFFER,
    _SGIMGUI_CMD_BEGIN_PASS,
    _SGIMGUI_CMD_APPLY_VIEWPORT,
    _SGIMGUI_CMD_APPLY_SCISSOR_RECT,
    _SGIMGUI_CMD_APPLY_PIPELINE,
    _SGIMGUI_CMD_APPLY_BINDINGS,
    _SGIMGUI_CMD_APPLY_UNIFORMS,
    _SGIMGUI_CMD_DRAW,
    _SGIMGUI_CMD_DRAW_EX,
    _SGIMGUI_CMD_DISPATCH,
    _SGIMGUI_CMD_END_PASS,
    _SGIMGUI_CMD_COMMIT,
    _SGIMGUI_CMD_ALLOC_BUFFER,
    _SGIMGUI_CMD_ALLOC_IMAGE,
    _SGIMGUI_CMD_ALLOC_SAMPLER,
    _SGIMGUI_CMD_ALLOC_SHADER,
    _SGIMGUI_CMD_ALLOC_PIPELINE,
    _SGIMGUI_CMD_ALLOC_VIEW,
    _SGIMGUI_CMD_DEALLOC_BUFFER,
    _SGIMGUI_CMD_DEALLOC_IMAGE,
    _SGIMGUI_CMD_DEALLOC_SAMPLER,
    _SGIMGUI_CMD_DEALLOC_SHADER,
    _SGIMGUI_CMD_DEALLOC_PIPELINE,
    _SGIMGUI_CMD_DEALLOC_VIEW,
    _SGIMGUI_CMD_INIT_BUFFER,
    _SGIMGUI_CMD_INIT_IMAGE,
    _SGIMGUI_CMD_INIT_SAMPLER,
    _SGIMGUI_CMD_INIT_SHADER,
    _SGIMGUI_CMD_INIT_PIPELINE,
    _SGIMGUI_CMD_INIT_VIEW,
    _SGIMGUI_CMD_UNINIT_BUFFER,
    _SGIMGUI_CMD_UNINIT_IMAGE,
    _SGIMGUI_CMD_UNINIT_SAMPLER,
    _SGIMGUI_CMD_UNINIT_SHADER,
    _SGIMGUI_CMD_UNINIT_PIPELINE,
    _SGIMGUI_CMD_UNINIT_VIEW,
    _SGIMGUI_CMD_FAIL_BUFFER,
    _SGIMGUI_CMD_FAIL_IMAGE,
    _SGIMGUI_CMD_FAIL_SAMPLER,
    _SGIMGUI_CMD_FAIL_SHADER,
    _SGIMGUI_CMD_FAIL_PIPELINE,
    _SGIMGUI_CMD_FAIL_VIEW,
    _SGIMGUI_CMD_PUSH_DEBUG_GROUP,
    _SGIMGUI_CMD_POP_DEBUG_GROUP,
} _sgimgui_cmd_t;

typedef struct {
    sg_buffer result;
} _sgimgui_args_make_buffer_t;

typedef struct {
    sg_image result;
} _sgimgui_args_make_image_t;

typedef struct {
    sg_sampler result;
} _sgimgui_args_make_sampler_t;

typedef struct {
    sg_shader result;
} _sgimgui_args_make_shader_t;

typedef struct {
    sg_pipeline result;
} _sgimgui_args_make_pipeline_t;

typedef struct {
    sg_view result;
} _sgimgui_args_make_view_t;

typedef struct {
    sg_buffer buffer;
} _sgimgui_args_destroy_buffer_t;

typedef struct {
    sg_image image;
} _sgimgui_args_destroy_image_t;

typedef struct {
    sg_sampler sampler;
} _sgimgui_args_destroy_sampler_t;

typedef struct {
    sg_shader shader;
} _sgimgui_args_destroy_shader_t;

typedef struct {
    sg_pipeline pipeline;
} _sgimgui_args_destroy_pipeline_t;

typedef struct {
    sg_view view;
} _sgimgui_args_destroy_view_t;

typedef struct {
    sg_buffer buffer;
    size_t data_size;
} _sgimgui_args_update_buffer_t;

typedef struct {
    sg_image image;
} _sgimgui_args_update_image_t;

typedef struct {
    sg_buffer buffer;
    size_t data_size;
    int result;
} _sgimgui_args_append_buffer_t;

typedef struct {
    sg_pass pass;
} _sgimgui_args_begin_pass_t;

typedef struct {
    int x, y, width, height;
    bool origin_top_left;
} _sgimgui_args_apply_viewport_t;

typedef struct {
    int x, y, width, height;
    bool origin_top_left;
} _sgimgui_args_apply_scissor_rect_t;

typedef struct {
    sg_pipeline pipeline;
} _sgimgui_args_apply_pipeline_t;

typedef struct {
    sg_bindings bindings;
} _sgimgui_args_apply_bindings_t;

typedef struct {
    int ub_slot;
    size_t data_size;
    sg_pipeline pipeline;   /* the pipeline which was active at this call */
    size_t ubuf_pos;        /* start of copied data in capture buffer */
} _sgimgui_args_apply_uniforms_t;

typedef struct {
    int base_element;
    int num_elements;
    int num_instances;
} _sgimgui_args_draw_t;

typedef struct {
    int base_element;
    int num_elements;
    int num_instances;
    int base_vertex;
    int base_instance;
} _sgimgui_args_draw_ex_t;

typedef struct {
    int num_groups_x;
    int num_groups_y;
    int num_groups_z;
} _sgimgui_args_dispatch_t;

typedef struct {
    sg_buffer result;
} _sgimgui_args_alloc_buffer_t;

typedef struct {
    sg_image result;
} _sgimgui_args_alloc_image_t;

typedef struct {
    sg_sampler result;
} _sgimgui_args_alloc_sampler_t;

typedef struct {
    sg_shader result;
} _sgimgui_args_alloc_shader_t;

typedef struct {
    sg_pipeline result;
} _sgimgui_args_alloc_pipeline_t;

typedef struct {
    sg_view result;
} _sgimgui_args_alloc_view_t;

typedef struct {
    sg_buffer buffer;
} _sgimgui_args_dealloc_buffer_t;

typedef struct {
    sg_image image;
} _sgimgui_args_dealloc_image_t;

typedef struct {
    sg_sampler sampler;
} _sgimgui_args_dealloc_sampler_t;

typedef struct {
    sg_shader shader;
} _sgimgui_args_dealloc_shader_t;

typedef struct {
    sg_pipeline pipeline;
} _sgimgui_args_dealloc_pipeline_t;

typedef struct {
    sg_view view;
} _sgimgui_args_dealloc_view_t;

typedef struct {
    sg_buffer buffer;
} _sgimgui_args_init_buffer_t;

typedef struct {
    sg_image image;
} _sgimgui_args_init_image_t;

typedef struct {
    sg_sampler sampler;
} _sgimgui_args_init_sampler_t;

typedef struct {
    sg_shader shader;
} _sgimgui_args_init_shader_t;

typedef struct {
    sg_pipeline pipeline;
} _sgimgui_args_init_pipeline_t;

typedef struct {
    sg_view view;
} _sgimgui_args_init_view_t;

typedef struct {
    sg_buffer buffer;
} _sgimgui_args_uninit_buffer_t;

typedef struct {
    sg_image image;
} _sgimgui_args_uninit_image_t;

typedef struct {
    sg_sampler sampler;
} _sgimgui_args_uninit_sampler_t;

typedef struct {
    sg_shader shader;
} _sgimgui_args_uninit_shader_t;

typedef struct {
    sg_pipeline pipeline;
} _sgimgui_args_uninit_pipeline_t;

typedef struct {
    sg_view view;
} _sgimgui_args_uninit_view_t;

typedef struct {
    sg_buffer buffer;
} _sgimgui_args_fail_buffer_t;

typedef struct {
    sg_image image;
} _sgimgui_args_fail_image_t;

typedef struct {
    sg_sampler sampler;
} _sgimgui_args_fail_sampler_t;

typedef struct {
    sg_shader shader;
} _sgimgui_args_fail_shader_t;

typedef struct {
    sg_pipeline pipeline;
} _sgimgui_args_fail_pipeline_t;

typedef struct {
    sg_view view;
} _sgimgui_args_fail_view_t;

typedef struct {
    _sgimgui_str_t name;
} _sgimgui_args_push_debug_group_t;

typedef union {
    _sgimgui_args_make_buffer_t make_buffer;
    _sgimgui_args_make_image_t make_image;
    _sgimgui_args_make_sampler_t make_sampler;
    _sgimgui_args_make_shader_t make_shader;
    _sgimgui_args_make_pipeline_t make_pipeline;
    _sgimgui_args_make_view_t make_view;
    _sgimgui_args_destroy_buffer_t destroy_buffer;
    _sgimgui_args_destroy_image_t destroy_image;
    _sgimgui_args_destroy_sampler_t destroy_sampler;
    _sgimgui_args_destroy_shader_t destroy_shader;
    _sgimgui_args_destroy_pipeline_t destroy_pipeline;
    _sgimgui_args_destroy_view_t destroy_view;
    _sgimgui_args_update_buffer_t update_buffer;
    _sgimgui_args_update_image_t update_image;
    _sgimgui_args_append_buffer_t append_buffer;
    _sgimgui_args_begin_pass_t begin_pass;
    _sgimgui_args_apply_viewport_t apply_viewport;
    _sgimgui_args_apply_scissor_rect_t apply_scissor_rect;
    _sgimgui_args_apply_pipeline_t apply_pipeline;
    _sgimgui_args_apply_bindings_t apply_bindings;
    _sgimgui_args_apply_uniforms_t apply_uniforms;
    _sgimgui_args_draw_t draw;
    _sgimgui_args_draw_ex_t draw_ex;
    _sgimgui_args_dispatch_t dispatch;
    _sgimgui_args_alloc_buffer_t alloc_buffer;
    _sgimgui_args_alloc_image_t alloc_image;
    _sgimgui_args_alloc_sampler_t alloc_sampler;
    _sgimgui_args_alloc_shader_t alloc_shader;
    _sgimgui_args_alloc_pipeline_t alloc_pipeline;
    _sgimgui_args_alloc_view_t alloc_view;
    _sgimgui_args_dealloc_buffer_t dealloc_buffer;
    _sgimgui_args_dealloc_image_t dealloc_image;
    _sgimgui_args_dealloc_sampler_t dealloc_sampler;
    _sgimgui_args_dealloc_shader_t dealloc_shader;
    _sgimgui_args_dealloc_pipeline_t dealloc_pipeline;
    _sgimgui_args_dealloc_view_t dealloc_view;
    _sgimgui_args_init_buffer_t init_buffer;
    _sgimgui_args_init_image_t init_image;
    _sgimgui_args_init_sampler_t init_sampler;
    _sgimgui_args_init_shader_t init_shader;
    _sgimgui_args_init_pipeline_t init_pipeline;
    _sgimgui_args_init_view_t init_view;
    _sgimgui_args_uninit_buffer_t uninit_buffer;
    _sgimgui_args_uninit_image_t uninit_image;
    _sgimgui_args_uninit_sampler_t uninit_sampler;
    _sgimgui_args_uninit_shader_t uninit_shader;
    _sgimgui_args_uninit_pipeline_t uninit_pipeline;
    _sgimgui_args_uninit_view_t uninit_view;
    _sgimgui_args_fail_buffer_t fail_buffer;
    _sgimgui_args_fail_image_t fail_image;
    _sgimgui_args_fail_sampler_t fail_sampler;
    _sgimgui_args_fail_shader_t fail_shader;
    _sgimgui_args_fail_pipeline_t fail_pipeline;
    _sgimgui_args_fail_view_t fail_view;
    _sgimgui_args_push_debug_group_t push_debug_group;
} _sgimgui_args_t;

typedef struct {
    _sgimgui_cmd_t cmd;
    uint32_t color;
    _sgimgui_args_t args;
} _sgimgui_capture_item_t;

typedef struct {
    size_t ubuf_size;       /* size of uniform capture buffer in bytes */
    size_t ubuf_pos;        /* current uniform buffer pos */
    uint8_t* ubuf;          /* buffer for capturing uniform updates */
    int num_items;
    _sgimgui_capture_item_t items[_SGIMGUI_MAX_FRAMECAPTURE_ITEMS];
} _sgimgui_capture_bucket_t;

/* double-buffered call-capture buckets, one bucket is currently recorded,
   the previous bucket is displayed
*/
typedef struct {
    bool open;
    int bucket_index;      /* which bucket to record to, 0 or 1 */
    int sel_item;          /* currently selected capture item by index */
    _sgimgui_capture_bucket_t bucket[2];
} _sgimgui_capture_window_t;

typedef struct {
    bool open;
} _sgimgui_caps_window_t;

typedef struct {
    bool open;
    bool disable_sokol_imgui_stats;
    bool in_sokol_imgui;
    sg_stats stats;
    // FIXME: add a ringbuffer for a stats history here
} _sgimgui_frame_stats_window_t;

typedef struct {
    uint32_t init_tag;
    sgimgui_desc_t desc;
    _sgimgui_buffer_window_t buffer_window;
    _sgimgui_image_window_t image_window;
    _sgimgui_sampler_window_t sampler_window;
    _sgimgui_shader_window_t shader_window;
    _sgimgui_pipeline_window_t pipeline_window;
    _sgimgui_view_window_t view_window;
    _sgimgui_capture_window_t capture_window;
    _sgimgui_caps_window_t caps_window;
    _sgimgui_frame_stats_window_t frame_stats_window;
    sg_pipeline cur_pipeline;
    sg_trace_hooks hooks;
} _sgimgui_t;
static _sgimgui_t _sgimgui;

#define _SGIMGUI_SLOT_MASK (0xFFFF)
#define _SGIMGUI_LIST_WIDTH (192)
#define _SGIMGUI_COLOR_OTHER 0xFFCCCCCC
#define _SGIMGUI_COLOR_RSRC 0xFF00FFFF
#define _SGIMGUI_COLOR_PASS 0xFFFFFF00
#define _SGIMGUI_COLOR_APPLY 0xFFCCCC00
#define _SGIMGUI_COLOR_DRAW 0xFF00FF00
#define _SGIMGUI_COLOR_ERR 0xFF8888FF

/*--- C => C++ layer ---------------------------------------------------------*/

#if defined(__cplusplus)
#define _SGIMGUI_IMGUI_FUNC(name) ImGui::name
#else
#ifndef SOKOL_GFX_IMGUI_CPREFIX
#define SOKOL_GFX_IMGUI_CPREFIX ig
#endif
#define _SGIMGUI_CONCAT2(prefix, name) prefix ## name
#define _SGIMGUI_CONCAT(prefix, name) _SGIMGUI_CONCAT2(prefix, name)
#define _SGIMGUI_IMGUI_FUNC(name) _SGIMGUI_CONCAT(SOKOL_GFX_IMGUI_CPREFIX, name)
#endif

#if defined(__cplusplus)
#define IMVEC2(x,y) ImVec2(x,y)
#define IMVEC4(x,y,z,w) ImVec4(x,y,z,w)
#else
#define IMVEC2(x,y) (ImVec2){x,y}
#define IMVEC4(x,y,z,w) (ImVec4){x,y,z,w}
#endif

_SOKOL_PRIVATE void _sgimgui_igtext(const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);
    _SGIMGUI_IMGUI_FUNC(TextV)(fmt, args);
    va_end(args);
}

_SOKOL_PRIVATE void _sgimgui_igseparator(void) {
    _SGIMGUI_IMGUI_FUNC(Separator)();
}

_SOKOL_PRIVATE void _sgimgui_igsameline(void) {
    _SGIMGUI_IMGUI_FUNC(SameLine)();
}

_SOKOL_PRIVATE void _sgimgui_igpushidint(int int_id) {
    #if defined(__cplusplus)
        ImGui::PushID(int_id);
    #else
        _SGIMGUI_IMGUI_FUNC(PushIDInt)(int_id);
    #endif
}

_SOKOL_PRIVATE void _sgimgui_igpushid(const char* str_id) {
    _SGIMGUI_IMGUI_FUNC(PushID)(str_id);
}

_SOKOL_PRIVATE void _sgimgui_igpopid(void) {
    _SGIMGUI_IMGUI_FUNC(PopID)();
}

_SOKOL_PRIVATE bool _sgimgui_igselectableex(const char* label, bool selected, ImGuiSelectableFlags flags, const ImVec2 size) {
    #if defined(__cplusplus)
        return ImGui::Selectable(label, selected, flags, size);
    #else
        return _SGIMGUI_IMGUI_FUNC(SelectableEx)(label, selected, flags, size);
    #endif
}

_SOKOL_PRIVATE bool _sgimgui_igsmallbutton(const char* label) {
    return _SGIMGUI_IMGUI_FUNC(SmallButton)(label);
}

_SOKOL_PRIVATE bool _sgimgui_igbeginchild(const char* str_id, const ImVec2 size, bool border, ImGuiWindowFlags flags) {
    return _SGIMGUI_IMGUI_FUNC(BeginChild)(str_id, size, border, flags);
}

_SOKOL_PRIVATE void _sgimgui_igendchild(void) {
    _SGIMGUI_IMGUI_FUNC(EndChild)();
}

_SOKOL_PRIVATE void _sgimgui_igpushstylecolor(ImGuiCol idx, ImU32 col) {
    _SGIMGUI_IMGUI_FUNC(PushStyleColor)(idx, col);
}

_SOKOL_PRIVATE void _sgimgui_igpopstylecolor(void) {
    _SGIMGUI_IMGUI_FUNC(PopStyleColor)();
}

_SOKOL_PRIVATE bool _sgimgui_igtreenodestr(const char* str_id, const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);
    bool ret = _SGIMGUI_IMGUI_FUNC(TreeNodeV)(str_id, fmt, args);
    va_end(args);
    return ret;
}

_SOKOL_PRIVATE bool _sgimgui_igtreenode(const char* label) {
    return _SGIMGUI_IMGUI_FUNC(TreeNode)(label);
}

_SOKOL_PRIVATE void _sgimgui_igtreepop(void) {
    _SGIMGUI_IMGUI_FUNC(TreePop)();
}

_SOKOL_PRIVATE bool _sgimgui_igisitemhovered(ImGuiHoveredFlags flags) {
    return _SGIMGUI_IMGUI_FUNC(IsItemHovered)(flags);
}

_SOKOL_PRIVATE void _sgimgui_igsettooltip(const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);
    _SGIMGUI_IMGUI_FUNC(SetTooltipV)(fmt, args);
    va_end(args);
}

_SOKOL_PRIVATE bool _sgimgui_igbegintooltip(void) {
    return _SGIMGUI_IMGUI_FUNC(BeginTooltip)();
}

_SOKOL_PRIVATE void _sgimgui_igendtooltip(void) {
    _SGIMGUI_IMGUI_FUNC(EndTooltip)();
}

_SOKOL_PRIVATE bool _sgimgui_igsliderfloatex(const char* label, float* v, float v_min, float v_max, const char* format, ImGuiSliderFlags flags) {
    #if defined(__cplusplus)
        return ImGui::SliderFloat(label, v, v_min, v_max, format, flags);
    #else
        return _SGIMGUI_IMGUI_FUNC(SliderFloatEx)(label, v, v_min, v_max, format, flags);
    #endif
}

_SOKOL_PRIVATE void _sgimgui_igsetnextwindowsize(const ImVec2 size, ImGuiCond cond) {
    _SGIMGUI_IMGUI_FUNC(SetNextWindowSize)(size, cond);
}

_SOKOL_PRIVATE bool _sgimgui_igbegin(const char* name, bool* p_open, ImGuiWindowFlags flags) {
    return _SGIMGUI_IMGUI_FUNC(Begin)(name, p_open, flags);
}

_SOKOL_PRIVATE void _sgimgui_igend(void) {
    _SGIMGUI_IMGUI_FUNC(End)();
}

_SOKOL_PRIVATE bool _sgimgui_igbeginmenu(const char* label) {
    return _SGIMGUI_IMGUI_FUNC(BeginMenu)(label);
}

_SOKOL_PRIVATE void _sgimgui_igendmenu(void) {
    _SGIMGUI_IMGUI_FUNC(EndMenu)();
}

_SOKOL_PRIVATE bool _sgimgui_igmenuitemboolptr(const char* label, const char* shortcut, bool* p_selected, bool enabled) {
    #if defined(__cplusplus)
        return ImGui::MenuItem(label, shortcut, p_selected, enabled);
    #else
        return _SGIMGUI_IMGUI_FUNC(MenuItemBoolPtr)(label, shortcut, p_selected, enabled);
    #endif
}

_SOKOL_PRIVATE bool _sgimgui_igbegintable(const char* str_id, int column, ImGuiTableFlags flags) {
    return _SGIMGUI_IMGUI_FUNC(BeginTable)(str_id, column, flags);
}

_SOKOL_PRIVATE void _sgimgui_igendtable(void) {
    _SGIMGUI_IMGUI_FUNC(EndTable)();
}

_SOKOL_PRIVATE void _sgimgui_igtablesetupscrollfreeze(int cols, int rows) {
    _SGIMGUI_IMGUI_FUNC(TableSetupScrollFreeze)(cols, rows);
}

_SOKOL_PRIVATE void _sgimgui_igtablesetupcolumn(const char* label, ImGuiTableColumnFlags flags) {
    _SGIMGUI_IMGUI_FUNC(TableSetupColumn)(label, flags);
}

_SOKOL_PRIVATE void _sgimgui_igtableheadersrow(void) {
    _SGIMGUI_IMGUI_FUNC(TableHeadersRow)();
}

_SOKOL_PRIVATE void _sgimgui_igtablenextrow(void) {
    _SGIMGUI_IMGUI_FUNC(TableNextRow)();
}

_SOKOL_PRIVATE bool _sgimgui_igtablesetcolumnindex(int column_n) {
    return _SGIMGUI_IMGUI_FUNC(TableSetColumnIndex)(column_n);
}

_SOKOL_PRIVATE bool _sgimgui_igcheckbox(const char* label, bool* v) {
    return _SGIMGUI_IMGUI_FUNC(Checkbox)(label, v);
}

_SOKOL_PRIVATE void _sgimgui_igimage(ImTextureID user_texture_id, const ImVec2 size) {
    #if defined(__cplusplus)
        ImGui::Image(ImTextureRef(user_texture_id), size);
    #else
        // FIXME: Dear Bindings is currently missing a constructor wrapper for ImTextureRef
        ImTextureRef tex_ref = {0};
        tex_ref._TexID = user_texture_id;
        _SGIMGUI_IMGUI_FUNC(Image)(tex_ref, size);
    #endif
}

/*--- UTILS ------------------------------------------------------------------*/
_SOKOL_PRIVATE void _sgimgui_clear(void* ptr, size_t size) {
    SOKOL_ASSERT(ptr && (size > 0));
    memset(ptr, 0, size);
}

_SOKOL_PRIVATE void* _sgimgui_malloc(const sgimgui_allocator_t* allocator, size_t size) {
    SOKOL_ASSERT(allocator && (size > 0));
    void* ptr;
    if (allocator->alloc_fn) {
        ptr = allocator->alloc_fn(size, allocator->user_data);
    } else {
        ptr = malloc(size);
    }
    SOKOL_ASSERT(ptr);
    return ptr;
}

_SOKOL_PRIVATE void* _sgimgui_malloc_clear(const sgimgui_allocator_t* allocator, size_t size) {
    void* ptr = _sgimgui_malloc(allocator, size);
    _sgimgui_clear(ptr, size);
    return ptr;
}

_SOKOL_PRIVATE void _sgimgui_free(const sgimgui_allocator_t* allocator, void* ptr) {
    SOKOL_ASSERT(allocator);
    if (allocator->free_fn) {
        allocator->free_fn(ptr, allocator->user_data);
    } else {
        free(ptr);
    }
}

 _SOKOL_PRIVATE void* _sgimgui_realloc(const sgimgui_allocator_t* allocator, void* old_ptr, size_t old_size, size_t new_size) {
    SOKOL_ASSERT(allocator && (new_size > 0) && (new_size > old_size));
    void* new_ptr = _sgimgui_malloc(allocator, new_size);
    if (old_ptr) {
        if (old_size > 0) {
            memcpy(new_ptr, old_ptr, old_size);
        }
        _sgimgui_free(allocator, old_ptr);
    }
    return new_ptr;
}

_SOKOL_PRIVATE int _sgimgui_slot_index(uint32_t id) {
    int slot_index = (int) (id & _SGIMGUI_SLOT_MASK);
    SOKOL_ASSERT(0 != slot_index);
    return slot_index;
}

_SOKOL_PRIVATE uint32_t _sgimgui_align_u32(uint32_t val, uint32_t align) {
    SOKOL_ASSERT((align > 0) && ((align & (align - 1)) == 0));
    return (val + (align - 1)) & ~(align - 1);
}

_SOKOL_PRIVATE uint32_t _sgimgui_std140_uniform_alignment(sg_uniform_type type, int array_count) {
    SOKOL_ASSERT(array_count > 0);
    if (array_count == 1) {
        switch (type) {
            case SG_UNIFORMTYPE_FLOAT:
            case SG_UNIFORMTYPE_INT:
                return 4;
            case SG_UNIFORMTYPE_FLOAT2:
            case SG_UNIFORMTYPE_INT2:
                return 8;
            case SG_UNIFORMTYPE_FLOAT3:
            case SG_UNIFORMTYPE_FLOAT4:
            case SG_UNIFORMTYPE_INT3:
            case SG_UNIFORMTYPE_INT4:
                return 16;
            case SG_UNIFORMTYPE_MAT4:
                return 16;
            default:
                SOKOL_UNREACHABLE;
                return 1;
        }
    } else {
        return 16;
    }
}

_SOKOL_PRIVATE uint32_t _sgimgui_std140_uniform_size(sg_uniform_type type, int array_count) {
    SOKOL_ASSERT(array_count > 0);
    if (array_count == 1) {
        switch (type) {
            case SG_UNIFORMTYPE_FLOAT:
            case SG_UNIFORMTYPE_INT:
                return 4;
            case SG_UNIFORMTYPE_FLOAT2:
            case SG_UNIFORMTYPE_INT2:
                return 8;
            case SG_UNIFORMTYPE_FLOAT3:
            case SG_UNIFORMTYPE_INT3:
                return 12;
            case SG_UNIFORMTYPE_FLOAT4:
            case SG_UNIFORMTYPE_INT4:
                return 16;
            case SG_UNIFORMTYPE_MAT4:
                return 64;
            default:
                SOKOL_UNREACHABLE;
                return 0;
        }
    } else {
        switch (type) {
            case SG_UNIFORMTYPE_FLOAT:
            case SG_UNIFORMTYPE_FLOAT2:
            case SG_UNIFORMTYPE_FLOAT3:
            case SG_UNIFORMTYPE_FLOAT4:
            case SG_UNIFORMTYPE_INT:
            case SG_UNIFORMTYPE_INT2:
            case SG_UNIFORMTYPE_INT3:
            case SG_UNIFORMTYPE_INT4:
                return 16 * (uint32_t)array_count;
            case SG_UNIFORMTYPE_MAT4:
                return 64 * (uint32_t)array_count;
            default:
                SOKOL_UNREACHABLE;
                return 0;
        }
    }
}

_SOKOL_PRIVATE void _sgimgui_strcpy(_sgimgui_str_t* dst, const char* src) {
    SOKOL_ASSERT(dst);
    if (src) {
        #if defined(_MSC_VER)
        strncpy_s(dst->buf, _SGIMGUI_STRBUF_LEN, src, (_SGIMGUI_STRBUF_LEN-1));
        #else
        strncpy(dst->buf, src, _SGIMGUI_STRBUF_LEN);
        #endif
        dst->buf[_SGIMGUI_STRBUF_LEN-1] = 0;
    } else {
        _sgimgui_clear(dst->buf, _SGIMGUI_STRBUF_LEN);
    }
}

_SOKOL_PRIVATE _sgimgui_str_t _sgimgui_make_str(const char* str) {
    _sgimgui_str_t res;
    _sgimgui_strcpy(&res, str);
    return res;
}

_SOKOL_PRIVATE const char* _sgimgui_str_dup(const sgimgui_allocator_t* allocator, const char* src) {
    SOKOL_ASSERT(allocator && src);
    size_t len = strlen(src) + 1;
    char* dst = (char*) _sgimgui_malloc(allocator, len);
    memcpy(dst, src, len);
    return (const char*) dst;
}

_SOKOL_PRIVATE const void* _sgimgui_bin_dup(const sgimgui_allocator_t* allocator, const void* src, size_t num_bytes) {
    SOKOL_ASSERT(allocator && src && (num_bytes > 0));
    void* dst = _sgimgui_malloc(allocator, num_bytes);
    memcpy(dst, src, num_bytes);
    return (const void*) dst;
}

_SOKOL_PRIVATE void _sgimgui_snprintf(_sgimgui_str_t* dst, const char* fmt, ...) {
    SOKOL_ASSERT(dst);
    va_list args;
    va_start(args, fmt);
    vsnprintf(dst->buf, sizeof(dst->buf), fmt, args);
    dst->buf[sizeof(dst->buf)-1] = 0;
    va_end(args);
}

/*--- STRING CONVERSION ------------------------------------------------------*/
_SOKOL_PRIVATE const char* _sgimgui_resourcestate_string(sg_resource_state s) {
    switch (s) {
        case SG_RESOURCESTATE_INITIAL:  return "SG_RESOURCESTATE_INITIAL";
        case SG_RESOURCESTATE_ALLOC:    return "SG_RESOURCESTATE_ALLOC";
        case SG_RESOURCESTATE_VALID:    return "SG_RESOURCESTATE_VALID";
        case SG_RESOURCESTATE_FAILED:   return "SG_RESOURCESTATE_FAILED";
        default:                        return "SG_RESOURCESTATE_INVALID";
    }
}

_SOKOL_PRIVATE void _sgimgui_draw_resource_slot(const sg_slot_info* slot) {
    _sgimgui_igtext("ResId: %08X", slot->res_id);
    _sgimgui_igtext("State: %s", _sgimgui_resourcestate_string(slot->state));
    _sgimgui_igtext("Uninit Count: %d", slot->uninit_count);
}

_SOKOL_PRIVATE const char* _sgimgui_backend_string(sg_backend b) {
    switch (b) {
        case SG_BACKEND_GLCORE:             return "SG_BACKEND_GLCORE";
        case SG_BACKEND_GLES3:              return "SG_BACKEND_GLES3";
        case SG_BACKEND_D3D11:              return "SG_BACKEND_D3D11";
        case SG_BACKEND_METAL_IOS:          return "SG_BACKEND_METAL_IOS";
        case SG_BACKEND_METAL_MACOS:        return "SG_BACKEND_METAL_MACOS";
        case SG_BACKEND_METAL_SIMULATOR:    return "SG_BACKEND_METAL_SIMULATOR";
        case SG_BACKEND_WGPU:               return "SG_BACKEND_WGPU";
        case SG_BACKEND_VULKAN:             return "SG_BACKEND_VULKAN";
        case SG_BACKEND_DUMMY:              return "SG_BACKEND_DUMMY";
        default: return "???";
    }
}

_SOKOL_PRIVATE const char* _sgimgui_imagetype_string(sg_image_type t) {
    switch (t) {
        case SG_IMAGETYPE_2D:       return "SG_IMAGETYPE_2D";
        case SG_IMAGETYPE_CUBE:     return "SG_IMAGETYPE_CUBE";
        case SG_IMAGETYPE_3D:       return "SG_IMAGETYPE_3D";
        case SG_IMAGETYPE_ARRAY:    return "SG_IMAGETYPE_ARRAY";
        default:                    return "???";
    }
}

_SOKOL_PRIVATE const char* _sgimgui_imagesampletype_string(sg_image_sample_type t) {
    switch (t) {
        case SG_IMAGESAMPLETYPE_FLOAT:  return "SG_IMAGESAMPLETYPE_FLOAT";
        case SG_IMAGESAMPLETYPE_DEPTH:  return "SG_IMAGESAMPLETYPE_DEPTH";
        case SG_IMAGESAMPLETYPE_SINT:   return "SG_IMAGESAMPLETYPE_SINT";
        case SG_IMAGESAMPLETYPE_UINT:   return "SG_IMAGESAMPLETYPE_UINT";
        case SG_IMAGESAMPLETYPE_UNFILTERABLE_FLOAT: return "SG_IMAGESAMPLETYPE_UNFILTERABLE_FLOAT";
        default:                        return "???";
    }
}

_SOKOL_PRIVATE const char* _sgimgui_samplertype_string(sg_sampler_type t) {
    switch (t) {
        case SG_SAMPLERTYPE_FILTERING:      return "SG_SAMPLERTYPE_FILTERING";
        case SG_SAMPLERTYPE_COMPARISON:     return "SG_SAMPLERTYPE_COMPARISON";
        case SG_SAMPLERTYPE_NONFILTERING:   return "SG_SAMPLERTYPE_NONFILTERING";
        default:                            return "???";
    }
}

_SOKOL_PRIVATE const char* _sgimgui_uniformlayout_string(sg_uniform_layout l) {
    switch (l) {
        case SG_UNIFORMLAYOUT_NATIVE:   return "SG_UNIFORMLAYOUT_NATIVE";
        case SG_UNIFORMLAYOUT_STD140:   return "SG_UNIFORMLAYOUT_STD140";
        default:                        return "???";
    }
}

_SOKOL_PRIVATE const char* _sgimgui_pixelformat_string(sg_pixel_format fmt) {
    switch (fmt) {
        case SG_PIXELFORMAT_NONE: return "SG_PIXELFORMAT_NONE";
        case SG_PIXELFORMAT_R8: return "SG_PIXELFORMAT_R8";
        case SG_PIXELFORMAT_R8SN: return "SG_PIXELFORMAT_R8SN";
        case SG_PIXELFORMAT_R8UI: return "SG_PIXELFORMAT_R8UI";
        case SG_PIXELFORMAT_R8SI: return "SG_PIXELFORMAT_R8SI";
        case SG_PIXELFORMAT_R16: return "SG_PIXELFORMAT_R16";
        case SG_PIXELFORMAT_R16SN: return "SG_PIXELFORMAT_R16SN";
        case SG_PIXELFORMAT_R16UI: return "SG_PIXELFORMAT_R16UI";
        case SG_PIXELFORMAT_R16SI: return "SG_PIXELFORMAT_R16SI";
        case SG_PIXELFORMAT_R16F: return "SG_PIXELFORMAT_R16F";
        case SG_PIXELFORMAT_RG8: return "SG_PIXELFORMAT_RG8";
        case SG_PIXELFORMAT_RG8SN: return "SG_PIXELFORMAT_RG8SN";
        case SG_PIXELFORMAT_RG8UI: return "SG_PIXELFORMAT_RG8UI";
        case SG_PIXELFORMAT_RG8SI: return "SG_PIXELFORMAT_RG8SI";
        case SG_PIXELFORMAT_R32UI: return "SG_PIXELFORMAT_R32UI";
        case SG_PIXELFORMAT_R32SI: return "SG_PIXELFORMAT_R32SI";
        case SG_PIXELFORMAT_R32F: return "SG_PIXELFORMAT_R32F";
        case SG_PIXELFORMAT_RG16: return "SG_PIXELFORMAT_RG16";
        case SG_PIXELFORMAT_RG16SN: return "SG_PIXELFORMAT_RG16SN";
        case SG_PIXELFORMAT_RG16UI: return "SG_PIXELFORMAT_RG16UI";
        case SG_PIXELFORMAT_RG16SI: return "SG_PIXELFORMAT_RG16SI";
        case SG_PIXELFORMAT_RG16F: return "SG_PIXELFORMAT_RG16F";
        case SG_PIXELFORMAT_RGBA8: return "SG_PIXELFORMAT_RGBA8";
        case SG_PIXELFORMAT_SRGB8A8: return "SG_PIXELFORMAT_SRGB8A8";
        case SG_PIXELFORMAT_RGBA8SN: return "SG_PIXELFORMAT_RGBA8SN";
        case SG_PIXELFORMAT_RGBA8UI: return "SG_PIXELFORMAT_RGBA8UI";
        case SG_PIXELFORMAT_RGBA8SI: return "SG_PIXELFORMAT_RGBA8SI";
        case SG_PIXELFORMAT_BGRA8: return "SG_PIXELFORMAT_BGRA8";
        case SG_PIXELFORMAT_RGB10A2: return "SG_PIXELFORMAT_RGB10A2";
        case SG_PIXELFORMAT_RG11B10F: return "SG_PIXELFORMAT_RG11B10F";
        case SG_PIXELFORMAT_RG32UI: return "SG_PIXELFORMAT_RG32UI";
        case SG_PIXELFORMAT_RG32SI: return "SG_PIXELFORMAT_RG32SI";
        case SG_PIXELFORMAT_RG32F: return "SG_PIXELFORMAT_RG32F";
        case SG_PIXELFORMAT_RGBA16: return "SG_PIXELFORMAT_RGBA16";
        case SG_PIXELFORMAT_RGBA16SN: return "SG_PIXELFORMAT_RGBA16SN";
        case SG_PIXELFORMAT_RGBA16UI: return "SG_PIXELFORMAT_RGBA16UI";
        case SG_PIXELFORMAT_RGBA16SI: return "SG_PIXELFORMAT_RGBA16SI";
        case SG_PIXELFORMAT_RGBA16F: return "SG_PIXELFORMAT_RGBA16F";
        case SG_PIXELFORMAT_RGBA32UI: return "SG_PIXELFORMAT_RGBA32UI";
        case SG_PIXELFORMAT_RGBA32SI: return "SG_PIXELFORMAT_RGBA32SI";
        case SG_PIXELFORMAT_RGBA32F: return "SG_PIXELFORMAT_RGBA32F";
        case SG_PIXELFORMAT_DEPTH: return "SG_PIXELFORMAT_DEPTH";
        case SG_PIXELFORMAT_DEPTH_STENCIL: return "SG_PIXELFORMAT_DEPTH_STENCIL";
        case SG_PIXELFORMAT_BC1_RGBA: return "SG_PIXELFORMAT_BC1_RGBA";
        case SG_PIXELFORMAT_BC2_RGBA: return "SG_PIXELFORMAT_BC2_RGBA";
        case SG_PIXELFORMAT_BC3_RGBA: return "SG_PIXELFORMAT_BC3_RGBA";
        case SG_PIXELFORMAT_BC4_R: return "SG_PIXELFORMAT_BC4_R";
        case SG_PIXELFORMAT_BC4_RSN: return "SG_PIXELFORMAT_BC4_RSN";
        case SG_PIXELFORMAT_BC5_RG: return "SG_PIXELFORMAT_BC5_RG";
        case SG_PIXELFORMAT_BC5_RGSN: return "SG_PIXELFORMAT_BC5_RGSN";
        case SG_PIXELFORMAT_BC6H_RGBF: return "SG_PIXELFORMAT_BC6H_RGBF";
        case SG_PIXELFORMAT_BC6H_RGBUF: return "SG_PIXELFORMAT_BC6H_RGBUF";
        case SG_PIXELFORMAT_BC7_RGBA: return "SG_PIXELFORMAT_BC7_RGBA";
        case SG_PIXELFORMAT_ETC2_RGB8: return "SG_PIXELFORMAT_ETC2_RGB8";
        case SG_PIXELFORMAT_ETC2_RGB8A1: return "SG_PIXELFORMAT_ETC2_RGB8A1";
        case SG_PIXELFORMAT_ETC2_RGBA8: return "SG_PIXELFORMAT_ETC2_RGBA8";
        case SG_PIXELFORMAT_EAC_R11: return "SG_PIXELFORMAT_EAC_R11";
        case SG_PIXELFORMAT_EAC_R11SN: return "SG_PIXELFORMAT_EAC_R11SN";
        case SG_PIXELFORMAT_EAC_RG11: return "SG_PIXELFORMAT_EAC_RG11";
        case SG_PIXELFORMAT_EAC_RG11SN: return "SG_PIXELFORMAT_EAC_RG11SN";
        case SG_PIXELFORMAT_RGB9E5: return "SG_PIXELFORMAT_RGB9E5";
        case SG_PIXELFORMAT_BC3_SRGBA: return "SG_PIXELFORMAT_BC3_SRGBA";
        case SG_PIXELFORMAT_BC7_SRGBA: return "SG_PIXELFORMAT_BC7_SRGBA";
        case SG_PIXELFORMAT_ETC2_SRGB8: return "SG_PIXELFORMAT_ETC2_SRGB8";
        case SG_PIXELFORMAT_ETC2_SRGB8A8: return "SG_PIXELFORMAT_ETC2_SRGB8A8";
        case SG_PIXELFORMAT_ASTC_4x4_RGBA: return "SG_PIXELFORMAT_ASTC_4x4_RGBA";
        case SG_PIXELFORMAT_ASTC_4x4_SRGBA: return "SG_PIXELFORMAT_ASTC_4x4_SRGBA";
        default: return "???";
    }
}

_SOKOL_PRIVATE const char* _sgimgui_filter_string(sg_filter f) {
    switch (f) {
        case SG_FILTER_NEAREST: return "SG_FILTER_NEAREST";
        case SG_FILTER_LINEAR:  return "SG_FILTER_LINEAR";
        default:                return "???";
    }
}

_SOKOL_PRIVATE const char* _sgimgui_wrap_string(sg_wrap w) {
    switch (w) {
        case SG_WRAP_REPEAT:            return "SG_WRAP_REPEAT";
        case SG_WRAP_CLAMP_TO_EDGE:     return "SG_WRAP_CLAMP_TO_EDGE";
        case SG_WRAP_CLAMP_TO_BORDER:   return "SG_WRAP_CLAMP_TO_BORDER";
        case SG_WRAP_MIRRORED_REPEAT:   return "SG_WRAP_MIRRORED_REPEAT";
        default:                        return "???";
    }
}

_SOKOL_PRIVATE const char* _sgimgui_bordercolor_string(sg_border_color bc) {
    switch (bc) {
        case SG_BORDERCOLOR_TRANSPARENT_BLACK:  return "SG_BORDERCOLOR_TRANSPARENT_BLACK";
        case SG_BORDERCOLOR_OPAQUE_BLACK:       return "SG_BORDERCOLOR_OPAQUE_BLACK";
        case SG_BORDERCOLOR_OPAQUE_WHITE:       return "SG_BORDERCOLOR_OPAQUE_WHITE";
        default:                                return "???";
    }
}

_SOKOL_PRIVATE const char* _sgimgui_uniformtype_string(sg_uniform_type t) {
    switch (t) {
        case SG_UNIFORMTYPE_FLOAT:  return "SG_UNIFORMTYPE_FLOAT";
        case SG_UNIFORMTYPE_FLOAT2: return "SG_UNIFORMTYPE_FLOAT2";
        case SG_UNIFORMTYPE_FLOAT3: return "SG_UNIFORMTYPE_FLOAT3";
        case SG_UNIFORMTYPE_FLOAT4: return "SG_UNIFORMTYPE_FLOAT4";
        case SG_UNIFORMTYPE_INT:    return "SG_UNIFORMTYPE_INT";
        case SG_UNIFORMTYPE_INT2:   return "SG_UNIFORMTYPE_INT2";
        case SG_UNIFORMTYPE_INT3:   return "SG_UNIFORMTYPE_INT3";
        case SG_UNIFORMTYPE_INT4:   return "SG_UNIFORMTYPE_INT4";
        case SG_UNIFORMTYPE_MAT4:   return "SG_UNIFORMTYPE_MAT4";
        default:                    return "???";
    }
}

_SOKOL_PRIVATE const char* _sgimgui_vertexstep_string(sg_vertex_step s) {
    switch (s) {
        case SG_VERTEXSTEP_PER_VERTEX:      return "SG_VERTEXSTEP_PER_VERTEX";
        case SG_VERTEXSTEP_PER_INSTANCE:    return "SG_VERTEXSTEP_PER_INSTANCE";
        default:                            return "???";
    }
}

_SOKOL_PRIVATE const char* _sgimgui_vertexformat_string(sg_vertex_format f) {
    switch (f) {
        case SG_VERTEXFORMAT_FLOAT:     return "SG_VERTEXFORMAT_FLOAT";
        case SG_VERTEXFORMAT_FLOAT2:    return "SG_VERTEXFORMAT_FLOAT2";
        case SG_VERTEXFORMAT_FLOAT3:    return "SG_VERTEXFORMAT_FLOAT3";
        case SG_VERTEXFORMAT_FLOAT4:    return "SG_VERTEXFORMAT_FLOAT4";
        case SG_VERTEXFORMAT_INT:       return "SG_VERTEXFORMAT_INT";
        case SG_VERTEXFORMAT_INT2:      return "SG_VERTEXFORMAT_INT2";
        case SG_VERTEXFORMAT_INT3:      return "SG_VERTEXFORMAT_INT3";
        case SG_VERTEXFORMAT_INT4:      return "SG_VERTEXFORMAT_INT4";
        case SG_VERTEXFORMAT_UINT:      return "SG_VERTEXFORMAT_UINT";
        case SG_VERTEXFORMAT_UINT2:     return "SG_VERTEXFORMAT_UINT2";
        case SG_VERTEXFORMAT_UINT3:     return "SG_VERTEXFORMAT_UINT3";
        case SG_VERTEXFORMAT_UINT4:     return "SG_VERTEXFORMAT_UINT4";
        case SG_VERTEXFORMAT_BYTE4:     return "SG_VERTEXFORMAT_BYTE4";
        case SG_VERTEXFORMAT_BYTE4N:    return "SG_VERTEXFORMAT_BYTE4N";
        case SG_VERTEXFORMAT_UBYTE4:    return "SG_VERTEXFORMAT_UBYTE4";
        case SG_VERTEXFORMAT_UBYTE4N:   return "SG_VERTEXFORMAT_UBYTE4N";
        case SG_VERTEXFORMAT_SHORT2:    return "SG_VERTEXFORMAT_SHORT2";
        case SG_VERTEXFORMAT_SHORT2N:   return "SG_VERTEXFORMAT_SHORT2N";
        case SG_VERTEXFORMAT_USHORT2:   return "SG_VERTEXFORMAT_USHORT2";
        case SG_VERTEXFORMAT_USHORT2N:  return "SG_VERTEXFORMAT_USHORT2N";
        case SG_VERTEXFORMAT_SHORT4:    return "SG_VERTEXFORMAT_SHORT4";
        case SG_VERTEXFORMAT_SHORT4N:   return "SG_VERTEXFORMAT_SHORT4N";
        case SG_VERTEXFORMAT_USHORT4:   return "SG_VERTEXFORMAT_USHORT4";
        case SG_VERTEXFORMAT_USHORT4N:  return "SG_VERTEXFORMAT_USHORT4N";
        case SG_VERTEXFORMAT_UINT10_N2: return "SG_VERTEXFORMAT_UINT10_N2";
        case SG_VERTEXFORMAT_HALF2:     return "SG_VERTEXFORMAT_HALF2";
        case SG_VERTEXFORMAT_HALF4:     return "SG_VERTEXFORMAT_HALF4";
        default:                        return "???";
    }
}

_SOKOL_PRIVATE const char* _sgimgui_primitivetype_string(sg_primitive_type t) {
    switch (t) {
        case SG_PRIMITIVETYPE_POINTS:           return "SG_PRIMITIVETYPE_POINTS";
        case SG_PRIMITIVETYPE_LINES:            return "SG_PRIMITIVETYPE_LINES";
        case SG_PRIMITIVETYPE_LINE_STRIP:       return "SG_PRIMITIVETYPE_LINE_STRIP";
        case SG_PRIMITIVETYPE_TRIANGLES:        return "SG_PRIMITIVETYPE_TRIANGLES";
        case SG_PRIMITIVETYPE_TRIANGLE_STRIP:   return "SG_PRIMITIVETYPE_TRIANGLE_STRIP";
        default:                                return "???";
    }
}

_SOKOL_PRIVATE const char* _sgimgui_indextype_string(sg_index_type t) {
    switch (t) {
        case SG_INDEXTYPE_NONE:     return "SG_INDEXTYPE_NONE";
        case SG_INDEXTYPE_UINT16:   return "SG_INDEXTYPE_UINT16";
        case SG_INDEXTYPE_UINT32:   return "SG_INDEXTYPE_UINT32";
        default:                    return "???";
    }
}

_SOKOL_PRIVATE const char* _sgimgui_stencilop_string(sg_stencil_op op) {
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

_SOKOL_PRIVATE const char* _sgimgui_comparefunc_string(sg_compare_func f) {
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

_SOKOL_PRIVATE const char* _sgimgui_blendfactor_string(sg_blend_factor f) {
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

_SOKOL_PRIVATE const char* _sgimgui_blendop_string(sg_blend_op op) {
    switch (op) {
        case SG_BLENDOP_ADD:                return "SG_BLENDOP_ADD";
        case SG_BLENDOP_SUBTRACT:           return "SG_BLENDOP_SUBTRACT";
        case SG_BLENDOP_REVERSE_SUBTRACT:   return "SG_BLENDOP_REVERSE_SUBTRACT";
        case SG_BLENDOP_MIN:                return "SG_BLENDOP_MIN";
        case SG_BLENDOP_MAX:                return "SG_BLENDOP_MAX";
        default:                            return "???";
    }
}

_SOKOL_PRIVATE const char* _sgimgui_colormask_string(sg_color_mask m) {
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

_SOKOL_PRIVATE const char* _sgimgui_cullmode_string(sg_cull_mode cm) {
    switch (cm) {
        case SG_CULLMODE_NONE:  return "SG_CULLMODE_NONE";
        case SG_CULLMODE_FRONT: return "SG_CULLMODE_FRONT";
        case SG_CULLMODE_BACK:  return "SG_CULLMODE_BACK";
        default:                return "???";
    }
}

_SOKOL_PRIVATE const char* _sgimgui_facewinding_string(sg_face_winding fw) {
    switch (fw) {
        case SG_FACEWINDING_CCW:    return "SG_FACEWINDING_CCW";
        case SG_FACEWINDING_CW:     return "SG_FACEWINDING_CW";
        default:                    return "???";
    }
}

_SOKOL_PRIVATE const char* _sgimgui_shaderstage_string(sg_shader_stage stage) {
    switch (stage) {
        case SG_SHADERSTAGE_VERTEX:     return "SG_SHADERSTAGE_VERTEX";
        case SG_SHADERSTAGE_FRAGMENT:   return "SG_SHADERSTAGE_FRAGMENT";
        case SG_SHADERSTAGE_COMPUTE:    return "SG_SHADERSTAGE_COMPUTE";
        default:                        return "???";
    }
}

_SOKOL_PRIVATE const char* _sgimgui_shaderattrbasetype_string(sg_shader_attr_base_type b) {
    switch (b) {
        case SG_SHADERATTRBASETYPE_UNDEFINED:   return "SG_SHADERATTRBASETYPE_UNDEFINED";
        case SG_SHADERATTRBASETYPE_FLOAT:       return "SG_SHADERATTRBASETYPE_FLOAT";
        case SG_SHADERATTRBASETYPE_SINT:        return "SG_SHADERATTRBASETYPE_SINT";
        case SG_SHADERATTRBASETYPE_UINT:        return "SG_SHADERATTRBASETYPE_UINT";
        default:                                return "???";
    }
}

_SOKOL_PRIVATE const char* _sgimgui_bool_string(bool b) {
    return b ? "true" : "false";
}

_SOKOL_PRIVATE const char* _sgimgui_color_string(_sgimgui_str_t* dst_str, sg_color color) {
    _sgimgui_snprintf(dst_str, "%.3f %.3f %.3f %.3f", color.r, color.g, color.b, color.a);
    return dst_str->buf;
}

_SOKOL_PRIVATE _sgimgui_str_t _sgimgui_res_id_string(uint32_t res_id, const char* label) {
    SOKOL_ASSERT(label);
    _sgimgui_str_t res;
    if (label[0]) {
        _sgimgui_snprintf(&res, "'%s'", label);
    } else {
        _sgimgui_snprintf(&res, "0x%08X", res_id);
    }
    return res;
}

_SOKOL_PRIVATE _sgimgui_str_t _sgimgui_buffer_id_string(_sgimgui_t* ctx, sg_buffer buf_id) {
    if (buf_id.id != SG_INVALID_ID) {
        const _sgimgui_buffer_t* buf_ui = &ctx->buffer_window.slots[_sgimgui_slot_index(buf_id.id)];
        return _sgimgui_res_id_string(buf_id.id, buf_ui->label.buf);
    } else {
        return _sgimgui_make_str("<invalid>");
    }
}

_SOKOL_PRIVATE _sgimgui_str_t _sgimgui_image_id_string(_sgimgui_t* ctx, sg_image img_id) {
    if (img_id.id != SG_INVALID_ID) {
        const _sgimgui_image_t* img_ui = &ctx->image_window.slots[_sgimgui_slot_index(img_id.id)];
        return _sgimgui_res_id_string(img_id.id, img_ui->label.buf);
    } else {
        return _sgimgui_make_str("<invalid>");
    }
}

_SOKOL_PRIVATE _sgimgui_str_t _sgimgui_sampler_id_string(_sgimgui_t* ctx, sg_sampler smp_id) {
    if (smp_id.id != SG_INVALID_ID) {
        const _sgimgui_sampler_t* smp_ui = &ctx->sampler_window.slots[_sgimgui_slot_index(smp_id.id)];
        return _sgimgui_res_id_string(smp_id.id, smp_ui->label.buf);
    } else {
        return _sgimgui_make_str("<invalid>");
    }
}

_SOKOL_PRIVATE _sgimgui_str_t _sgimgui_shader_id_string(_sgimgui_t* ctx, sg_shader shd_id) {
    if (shd_id.id != SG_INVALID_ID) {
        const _sgimgui_shader_t* shd_ui = &ctx->shader_window.slots[_sgimgui_slot_index(shd_id.id)];
        return _sgimgui_res_id_string(shd_id.id, shd_ui->label.buf);
    } else {
        return _sgimgui_make_str("<invalid>");
    }
}

_SOKOL_PRIVATE _sgimgui_str_t _sgimgui_pipeline_id_string(_sgimgui_t* ctx, sg_pipeline pip_id) {
    if (pip_id.id != SG_INVALID_ID) {
        const _sgimgui_pipeline_t* pip_ui = &ctx->pipeline_window.slots[_sgimgui_slot_index(pip_id.id)];
        return _sgimgui_res_id_string(pip_id.id, pip_ui->label.buf);
    } else {
        return _sgimgui_make_str("<invalid>");
    }
}

_SOKOL_PRIVATE _sgimgui_str_t _sgimgui_view_id_string(_sgimgui_t* ctx, sg_view view_id) {
    if (view_id.id != SG_INVALID_ID) {
        const _sgimgui_view_t* view_ui = &ctx->view_window.slots[_sgimgui_slot_index(view_id.id)];
        return _sgimgui_res_id_string(view_id.id, view_ui->label.buf);
    } else {
        return _sgimgui_make_str("<invalid>");
    }
}

/*--- RESOURCE HELPERS -------------------------------------------------------*/
_SOKOL_PRIVATE void _sgimgui_buffer_created(_sgimgui_t* ctx, sg_buffer res_id, int slot_index, const sg_buffer_desc* desc) {
    SOKOL_ASSERT((slot_index > 0) && (slot_index < ctx->buffer_window.num_slots));
    _sgimgui_buffer_t* buf = &ctx->buffer_window.slots[slot_index];
    buf->res_id = res_id;
    buf->desc = *desc;
    buf->label = _sgimgui_make_str(desc->label);
}

_SOKOL_PRIVATE void _sgimgui_buffer_destroyed(_sgimgui_t* ctx, int slot_index) {
    SOKOL_ASSERT((slot_index > 0) && (slot_index < ctx->buffer_window.num_slots));
    _sgimgui_buffer_t* buf = &ctx->buffer_window.slots[slot_index];
    buf->res_id.id = SG_INVALID_ID;
}

_SOKOL_PRIVATE void _sgimgui_image_created(_sgimgui_t* ctx, sg_image res_id, int slot_index, const sg_image_desc* desc) {
    SOKOL_ASSERT((slot_index > 0) && (slot_index < ctx->image_window.num_slots));
    _sgimgui_image_t* img = &ctx->image_window.slots[slot_index];
    img->res_id = res_id;
    img->desc = *desc;
    img->ui_scale = 1.0f;
    img->label = _sgimgui_make_str(desc->label);
}

_SOKOL_PRIVATE void _sgimgui_image_destroyed(_sgimgui_t* ctx, int slot_index) {
    SOKOL_ASSERT((slot_index > 0) && (slot_index < ctx->image_window.num_slots));
    _sgimgui_image_t* img = &ctx->image_window.slots[slot_index];
    img->res_id.id = SG_INVALID_ID;
}

_SOKOL_PRIVATE void _sgimgui_sampler_created(_sgimgui_t* ctx, sg_sampler res_id, int slot_index, const sg_sampler_desc* desc) {
    SOKOL_ASSERT((slot_index > 0) && (slot_index < ctx->sampler_window.num_slots));
    _sgimgui_sampler_t* smp = &ctx->sampler_window.slots[slot_index];
    smp->res_id = res_id;
    smp->desc = *desc;
    smp->label = _sgimgui_make_str(desc->label);
}

_SOKOL_PRIVATE void _sgimgui_sampler_destroyed(_sgimgui_t* ctx, int slot_index) {
    SOKOL_ASSERT((slot_index > 0) && (slot_index < ctx->sampler_window.num_slots));
    _sgimgui_sampler_t* smp = &ctx->sampler_window.slots[slot_index];
    smp->res_id.id = SG_INVALID_ID;
}

_SOKOL_PRIVATE void _sgimgui_shader_created(_sgimgui_t* ctx, sg_shader res_id, int slot_index, const sg_shader_desc* desc) {
    SOKOL_ASSERT((slot_index > 0) && (slot_index < ctx->shader_window.num_slots));
    _sgimgui_shader_t* shd = &ctx->shader_window.slots[slot_index];
    shd->res_id = res_id;
    shd->desc = *desc;
    shd->label = _sgimgui_make_str(desc->label);
    if (shd->desc.vertex_func.entry) {
        shd->vs_entry = _sgimgui_make_str(shd->desc.vertex_func.entry);
        shd->desc.vertex_func.entry = shd->vs_entry.buf;
    }
    if (shd->desc.fragment_func.entry) {
        shd->fs_entry = _sgimgui_make_str(shd->desc.fragment_func.entry);
        shd->desc.fragment_func.entry = shd->fs_entry.buf;
    }
    if (shd->desc.vertex_func.d3d11_target) {
        shd->vs_d3d11_target = _sgimgui_make_str(shd->desc.vertex_func.d3d11_target);
        shd->desc.vertex_func.d3d11_target = shd->vs_d3d11_target.buf;
    }
    if (shd->desc.fragment_func.d3d11_target) {
        shd->fs_d3d11_target = _sgimgui_make_str(shd->desc.fragment_func.d3d11_target);
        shd->desc.fragment_func.d3d11_target = shd->fs_d3d11_target.buf;
    }
    for (int i = 0; i < SG_MAX_UNIFORMBLOCK_BINDSLOTS; i++) {
        for (int j = 0; j < SG_MAX_UNIFORMBLOCK_MEMBERS; j++) {
            sg_glsl_shader_uniform* su = &shd->desc.uniform_blocks[i].glsl_uniforms[j];
            if (su->glsl_name) {
                shd->glsl_uniform_name[i][j] = _sgimgui_make_str(su->glsl_name);
                su->glsl_name = shd->glsl_uniform_name[i][j].buf;
            }
        }
    }
    for (int i = 0; i < SG_MAX_TEXTURE_SAMPLER_PAIRS; i++) {
        if (shd->desc.texture_sampler_pairs[i].glsl_name) {
            shd->glsl_texture_sampler_name[i] = _sgimgui_make_str(shd->desc.texture_sampler_pairs[i].glsl_name);
            shd->desc.texture_sampler_pairs[i].glsl_name = shd->glsl_texture_sampler_name[i].buf;
        }
    }
    if (shd->desc.vertex_func.source) {
        shd->desc.vertex_func.source = _sgimgui_str_dup(&ctx->desc.allocator, shd->desc.vertex_func.source);
    }
    if (shd->desc.vertex_func.bytecode.ptr) {
        shd->desc.vertex_func.bytecode.ptr = _sgimgui_bin_dup(&ctx->desc.allocator, shd->desc.vertex_func.bytecode.ptr, shd->desc.vertex_func.bytecode.size);
    }
    if (shd->desc.fragment_func.source) {
        shd->desc.fragment_func.source = _sgimgui_str_dup(&ctx->desc.allocator, shd->desc.fragment_func.source);
    }
    if (shd->desc.fragment_func.bytecode.ptr) {
        shd->desc.fragment_func.bytecode.ptr = _sgimgui_bin_dup(&ctx->desc.allocator, shd->desc.fragment_func.bytecode.ptr, shd->desc.fragment_func.bytecode.size);
    }
    for (int i = 0; i < SG_MAX_VERTEX_ATTRIBUTES; i++) {
        sg_shader_vertex_attr* va = &shd->desc.attrs[i];
        if (va->glsl_name) {
            shd->attr_glsl_name[i] = _sgimgui_make_str(va->glsl_name);
            va->glsl_name = shd->attr_glsl_name[i].buf;
        }
        if (va->hlsl_sem_name) {
            shd->attr_hlsl_sem_name[i] = _sgimgui_make_str(va->hlsl_sem_name);
            va->hlsl_sem_name = shd->attr_hlsl_sem_name[i].buf;
        }
    }
}

_SOKOL_PRIVATE void _sgimgui_shader_destroyed(_sgimgui_t* ctx, int slot_index) {
    SOKOL_ASSERT((slot_index > 0) && (slot_index < ctx->shader_window.num_slots));
    _sgimgui_shader_t* shd = &ctx->shader_window.slots[slot_index];
    shd->res_id.id = SG_INVALID_ID;
    if (shd->desc.vertex_func.source) {
        _sgimgui_free(&ctx->desc.allocator, (void*)shd->desc.vertex_func.source);
        shd->desc.vertex_func.source = 0;
    }
    if (shd->desc.vertex_func.bytecode.ptr) {
        _sgimgui_free(&ctx->desc.allocator, (void*)shd->desc.vertex_func.bytecode.ptr);
        shd->desc.vertex_func.bytecode.ptr = 0;
    }
    if (shd->desc.fragment_func.source) {
        _sgimgui_free(&ctx->desc.allocator, (void*)shd->desc.fragment_func.source);
        shd->desc.fragment_func.source = 0;
    }
    if (shd->desc.fragment_func.bytecode.ptr) {
        _sgimgui_free(&ctx->desc.allocator, (void*)shd->desc.fragment_func.bytecode.ptr);
        shd->desc.fragment_func.bytecode.ptr = 0;
    }
}

_SOKOL_PRIVATE void _sgimgui_pipeline_created(_sgimgui_t* ctx, sg_pipeline res_id, int slot_index, const sg_pipeline_desc* desc) {
    SOKOL_ASSERT((slot_index > 0) && (slot_index < ctx->pipeline_window.num_slots));
    _sgimgui_pipeline_t* pip = &ctx->pipeline_window.slots[slot_index];
    pip->res_id = res_id;
    pip->label = _sgimgui_make_str(desc->label);
    pip->desc = *desc;

}

_SOKOL_PRIVATE void _sgimgui_pipeline_destroyed(_sgimgui_t* ctx, int slot_index) {
    SOKOL_ASSERT((slot_index > 0) && (slot_index < ctx->pipeline_window.num_slots));
    _sgimgui_pipeline_t* pip = &ctx->pipeline_window.slots[slot_index];
    pip->res_id.id = SG_INVALID_ID;
}

_SOKOL_PRIVATE void _sgimgui_view_created(_sgimgui_t* ctx, sg_view res_id, int slot_index, const sg_view_desc* desc) {
    SOKOL_ASSERT((slot_index > 0) && (slot_index < ctx->view_window.num_slots));
    _sgimgui_view_t* view = &ctx->view_window.slots[slot_index];
    view->res_id = res_id;
    view->ui_scale = 1.0f;
    view->label = _sgimgui_make_str(desc->label);
    view->desc = *desc;
}

_SOKOL_PRIVATE void _sgimgui_view_destroyed(_sgimgui_t* ctx, int slot_index) {
    SOKOL_ASSERT((slot_index > 0) && (slot_index < ctx->view_window.num_slots));
    _sgimgui_view_t* view = &ctx->view_window.slots[slot_index];
    view->res_id.id = SG_INVALID_ID;
}

/*--- COMMAND CAPTURING ------------------------------------------------------*/
_SOKOL_PRIVATE void _sgimgui_capture_init(_sgimgui_t* ctx) {
    const size_t ubuf_initial_size = 256 * 1024;
    for (int i = 0; i < 2; i++) {
        _sgimgui_capture_bucket_t* bucket = &ctx->capture_window.bucket[i];
        bucket->ubuf_size = ubuf_initial_size;
        bucket->ubuf = (uint8_t*) _sgimgui_malloc(&ctx->desc.allocator, bucket->ubuf_size);
    }
}

_SOKOL_PRIVATE void _sgimgui_capture_discard(_sgimgui_t* ctx) {
    for (int i = 0; i < 2; i++) {
        _sgimgui_capture_bucket_t* bucket = &ctx->capture_window.bucket[i];
        SOKOL_ASSERT(bucket->ubuf);
        _sgimgui_free(&ctx->desc.allocator, bucket->ubuf);
        bucket->ubuf = 0;
    }
}

_SOKOL_PRIVATE _sgimgui_capture_bucket_t* _sgimgui_capture_get_write_bucket(_sgimgui_t* ctx) {
    return &ctx->capture_window.bucket[ctx->capture_window.bucket_index & 1];
}

_SOKOL_PRIVATE _sgimgui_capture_bucket_t* _sgimgui_capture_get_read_bucket(_sgimgui_t* ctx) {
    return &ctx->capture_window.bucket[(ctx->capture_window.bucket_index + 1) & 1];
}

_SOKOL_PRIVATE void _sgimgui_capture_next_frame(_sgimgui_t* ctx) {
    ctx->capture_window.bucket_index = (ctx->capture_window.bucket_index + 1) & 1;
    _sgimgui_capture_bucket_t* bucket = &ctx->capture_window.bucket[ctx->capture_window.bucket_index];
    bucket->num_items = 0;
    bucket->ubuf_pos = 0;
}

_SOKOL_PRIVATE void _sgimgui_capture_grow_ubuf(_sgimgui_t* ctx, size_t required_size) {
    _sgimgui_capture_bucket_t* bucket = _sgimgui_capture_get_write_bucket(ctx);
    SOKOL_ASSERT(required_size > bucket->ubuf_size);
    size_t old_size = bucket->ubuf_size;
    size_t new_size = required_size + (required_size>>1);  /* allocate a bit ahead */
    bucket->ubuf_size = new_size;
    bucket->ubuf = (uint8_t*) _sgimgui_realloc(&ctx->desc.allocator, bucket->ubuf, old_size, new_size);
}

_SOKOL_PRIVATE _sgimgui_capture_item_t* _sgimgui_capture_next_write_item(_sgimgui_t* ctx) {
    _sgimgui_capture_bucket_t* bucket = _sgimgui_capture_get_write_bucket(ctx);
    if (bucket->num_items < _SGIMGUI_MAX_FRAMECAPTURE_ITEMS) {
        _sgimgui_capture_item_t* item = &bucket->items[bucket->num_items++];
        return item;
    } else {
        return 0;
    }
}

_SOKOL_PRIVATE int _sgimgui_capture_num_read_items(_sgimgui_t* ctx) {
    _sgimgui_capture_bucket_t* bucket = _sgimgui_capture_get_read_bucket(ctx);
    return bucket->num_items;
}

_SOKOL_PRIVATE _sgimgui_capture_item_t* _sgimgui_capture_read_item_at(_sgimgui_t* ctx, int index) {
    _sgimgui_capture_bucket_t* bucket = _sgimgui_capture_get_read_bucket(ctx);
    SOKOL_ASSERT(index < bucket->num_items);
    return &bucket->items[index];
}

_SOKOL_PRIVATE size_t _sgimgui_capture_uniforms(_sgimgui_t* ctx, const sg_range* data) {
    _sgimgui_capture_bucket_t* bucket = _sgimgui_capture_get_write_bucket(ctx);
    const size_t required_size = bucket->ubuf_pos + data->size;
    if (required_size > bucket->ubuf_size) {
        _sgimgui_capture_grow_ubuf(ctx, required_size);
    }
    SOKOL_ASSERT(required_size <= bucket->ubuf_size);
    memcpy(bucket->ubuf + bucket->ubuf_pos, data->ptr, data->size);
    const size_t pos = bucket->ubuf_pos;
    bucket->ubuf_pos += data->size;
    SOKOL_ASSERT(bucket->ubuf_pos <= bucket->ubuf_size);
    return pos;
}

_SOKOL_PRIVATE _sgimgui_str_t _sgimgui_capture_item_string(_sgimgui_t* ctx, int index, const _sgimgui_capture_item_t* item) {
    _sgimgui_str_t str = _sgimgui_make_str(0);
    switch (item->cmd) {
        case _SGIMGUI_CMD_RESET_STATE_CACHE:
            _sgimgui_snprintf(&str, "%d: sg_reset_state_cache()", index);
            break;

        case _SGIMGUI_CMD_MAKE_BUFFER:
            {
                _sgimgui_str_t res_id = _sgimgui_buffer_id_string(ctx, item->args.make_buffer.result);
                _sgimgui_snprintf(&str, "%d: sg_make_buffer(desc=..) => %s", index, res_id.buf);
            }
            break;

        case _SGIMGUI_CMD_MAKE_IMAGE:
            {
                _sgimgui_str_t res_id = _sgimgui_image_id_string(ctx, item->args.make_image.result);
                _sgimgui_snprintf(&str, "%d: sg_make_image(desc=..) => %s", index, res_id.buf);
            }
            break;

        case _SGIMGUI_CMD_MAKE_SAMPLER:
            {
                _sgimgui_str_t res_id = _sgimgui_sampler_id_string(ctx, item->args.make_sampler.result);
                _sgimgui_snprintf(&str, "%d: sg_make_sampler(desc=..) => %s", index, res_id.buf);
            }
            break;
        case _SGIMGUI_CMD_MAKE_SHADER:
            {
                _sgimgui_str_t res_id = _sgimgui_shader_id_string(ctx, item->args.make_shader.result);
                _sgimgui_snprintf(&str, "%d: sg_make_shader(desc=..) => %s", index, res_id.buf);
            }
            break;

        case _SGIMGUI_CMD_MAKE_PIPELINE:
            {
                _sgimgui_str_t res_id = _sgimgui_pipeline_id_string(ctx, item->args.make_pipeline.result);
                _sgimgui_snprintf(&str, "%d: sg_make_pipeline(desc=..) => %s", index, res_id.buf);
            }
            break;

        case _SGIMGUI_CMD_MAKE_VIEW:
            {
                _sgimgui_str_t res_id = _sgimgui_view_id_string(ctx, item->args.make_view.result);
                _sgimgui_snprintf(&str, "%d: sg_make_views(desc=..) => %s", index, res_id.buf);
            }
            break;

        case _SGIMGUI_CMD_DESTROY_BUFFER:
            {
                _sgimgui_str_t res_id = _sgimgui_buffer_id_string(ctx, item->args.destroy_buffer.buffer);
                _sgimgui_snprintf(&str, "%d: sg_destroy_buffer(buf=%s)", index, res_id.buf);
            }
            break;

        case _SGIMGUI_CMD_DESTROY_IMAGE:
            {
                _sgimgui_str_t res_id = _sgimgui_image_id_string(ctx, item->args.destroy_image.image);
                _sgimgui_snprintf(&str, "%d: sg_destroy_image(img=%s)", index, res_id.buf);
            }
            break;

        case _SGIMGUI_CMD_DESTROY_SAMPLER:
            {
                _sgimgui_str_t res_id = _sgimgui_sampler_id_string(ctx, item->args.destroy_sampler.sampler);
                _sgimgui_snprintf(&str, "%d: sg_destroy_sampler(smp=%s)", index, res_id.buf);
            }
            break;

        case _SGIMGUI_CMD_DESTROY_SHADER:
            {
                _sgimgui_str_t res_id = _sgimgui_shader_id_string(ctx, item->args.destroy_shader.shader);
                _sgimgui_snprintf(&str, "%d: sg_destroy_shader(shd=%s)", index, res_id.buf);
            }
            break;

        case _SGIMGUI_CMD_DESTROY_PIPELINE:
            {
                _sgimgui_str_t res_id = _sgimgui_pipeline_id_string(ctx, item->args.destroy_pipeline.pipeline);
                _sgimgui_snprintf(&str, "%d: sg_destroy_pipeline(pip=%s)", index, res_id.buf);
            }
            break;

        case _SGIMGUI_CMD_DESTROY_VIEW:
            {
                _sgimgui_str_t res_id = _sgimgui_view_id_string(ctx, item->args.destroy_view.view);
                _sgimgui_snprintf(&str, "%d: sg_destroy_view(view=%s)", index, res_id.buf);
            }
            break;

        case _SGIMGUI_CMD_UPDATE_BUFFER:
            {
                _sgimgui_str_t res_id = _sgimgui_buffer_id_string(ctx, item->args.update_buffer.buffer);
                _sgimgui_snprintf(&str, "%d: sg_update_buffer(buf=%s, data.size=%d)",
                    index, res_id.buf,
                    item->args.update_buffer.data_size);
            }
            break;

        case _SGIMGUI_CMD_UPDATE_IMAGE:
            {
                _sgimgui_str_t res_id = _sgimgui_image_id_string(ctx, item->args.update_image.image);
                _sgimgui_snprintf(&str, "%d: sg_update_image(img=%s, data=..)", index, res_id.buf);
            }
            break;

        case _SGIMGUI_CMD_APPEND_BUFFER:
            {
                _sgimgui_str_t res_id = _sgimgui_buffer_id_string(ctx, item->args.append_buffer.buffer);
                _sgimgui_snprintf(&str, "%d: sg_append_buffer(buf=%s, data.size=%d) => %d",
                    index, res_id.buf,
                    item->args.append_buffer.data_size,
                    item->args.append_buffer.result);
            }
            break;

        case _SGIMGUI_CMD_BEGIN_PASS:
            {
                _sgimgui_snprintf(&str, "%d: sg_begin_pass(pass=...)", index);
            }
            break;

        case _SGIMGUI_CMD_APPLY_VIEWPORT:
            _sgimgui_snprintf(&str, "%d: sg_apply_viewport(x=%d, y=%d, width=%d, height=%d, origin_top_left=%s)",
                index,
                item->args.apply_viewport.x,
                item->args.apply_viewport.y,
                item->args.apply_viewport.width,
                item->args.apply_viewport.height,
                _sgimgui_bool_string(item->args.apply_viewport.origin_top_left));
            break;

        case _SGIMGUI_CMD_APPLY_SCISSOR_RECT:
            _sgimgui_snprintf(&str, "%d: sg_apply_scissor_rect(x=%d, y=%d, width=%d, height=%d, origin_top_left=%s)",
                index,
                item->args.apply_scissor_rect.x,
                item->args.apply_scissor_rect.y,
                item->args.apply_scissor_rect.width,
                item->args.apply_scissor_rect.height,
                _sgimgui_bool_string(item->args.apply_scissor_rect.origin_top_left));
            break;

        case _SGIMGUI_CMD_APPLY_PIPELINE:
            {
                _sgimgui_str_t res_id = _sgimgui_pipeline_id_string(ctx, item->args.apply_pipeline.pipeline);
                _sgimgui_snprintf(&str, "%d: sg_apply_pipeline(pip=%s)", index, res_id.buf);
            }
            break;

        case _SGIMGUI_CMD_APPLY_BINDINGS:
            _sgimgui_snprintf(&str, "%d: sg_apply_bindings(bindings=..)", index);
            break;

        case _SGIMGUI_CMD_APPLY_UNIFORMS:
            _sgimgui_snprintf(&str, "%d: sg_apply_uniforms(ub_slot=%d, data.size=%d)",
                index,
                item->args.apply_uniforms.ub_slot,
                item->args.apply_uniforms.data_size);
            break;

        case _SGIMGUI_CMD_DRAW:
            _sgimgui_snprintf(&str, "%d: sg_draw(base_element=%d, num_elements=%d, num_instances=%d)",
                index,
                item->args.draw.base_element,
                item->args.draw.num_elements,
                item->args.draw.num_instances);
            break;

        case _SGIMGUI_CMD_DRAW_EX:
            _sgimgui_snprintf(&str, "%d: sg_draw_ex(base_element=%d, num_elements=%d, num_instances=%d, base_vertex=%d, base_instance=%d)",
                index,
                item->args.draw_ex.base_element,
                item->args.draw_ex.num_elements,
                item->args.draw_ex.num_instances,
                item->args.draw_ex.base_vertex,
                item->args.draw_ex.base_instance);
            break;

        case _SGIMGUI_CMD_DISPATCH:
            _sgimgui_snprintf(&str, "%d: sg_dispatch(num_groups_x=%d, num_groups_y=%d, num_groups_z=%d)",
                index,
                item->args.dispatch.num_groups_x,
                item->args.dispatch.num_groups_y,
                item->args.dispatch.num_groups_z);
            break;

        case _SGIMGUI_CMD_END_PASS:
            _sgimgui_snprintf(&str, "%d: sg_end_pass()", index);
            break;

        case _SGIMGUI_CMD_COMMIT:
            _sgimgui_snprintf(&str, "%d: sg_commit()", index);
            break;

        case _SGIMGUI_CMD_ALLOC_BUFFER:
            {
                _sgimgui_str_t res_id = _sgimgui_buffer_id_string(ctx, item->args.alloc_buffer.result);
                _sgimgui_snprintf(&str, "%d: sg_alloc_buffer() => %s", index, res_id.buf);
            }
            break;

        case _SGIMGUI_CMD_ALLOC_IMAGE:
            {
                _sgimgui_str_t res_id = _sgimgui_image_id_string(ctx, item->args.alloc_image.result);
                _sgimgui_snprintf(&str, "%d: sg_alloc_image() => %s", index, res_id.buf);
            }
            break;

        case _SGIMGUI_CMD_ALLOC_SAMPLER:
            {
                _sgimgui_str_t res_id = _sgimgui_sampler_id_string(ctx, item->args.alloc_sampler.result);
                _sgimgui_snprintf(&str, "%d: sg_alloc_sampler() => %s", index, res_id.buf);
            }
            break;

        case _SGIMGUI_CMD_ALLOC_SHADER:
            {
                _sgimgui_str_t res_id = _sgimgui_shader_id_string(ctx, item->args.alloc_shader.result);
                _sgimgui_snprintf(&str, "%d: sg_alloc_shader() => %s", index, res_id.buf);
            }
            break;

        case _SGIMGUI_CMD_ALLOC_PIPELINE:
            {
                _sgimgui_str_t res_id = _sgimgui_pipeline_id_string(ctx, item->args.alloc_pipeline.result);
                _sgimgui_snprintf(&str, "%d: sg_alloc_pipeline() => %s", index, res_id.buf);
            }
            break;

        case _SGIMGUI_CMD_ALLOC_VIEW:
            {
                _sgimgui_str_t res_id = _sgimgui_view_id_string(ctx, item->args.alloc_view.result);
                _sgimgui_snprintf(&str, "%d: sg_alloc_view() => %s", index, res_id.buf);
            }
            break;

        case _SGIMGUI_CMD_DEALLOC_BUFFER:
            {
                _sgimgui_str_t res_id = _sgimgui_buffer_id_string(ctx, item->args.dealloc_buffer.buffer);
                _sgimgui_snprintf(&str, "%d: sg_dealloc_buffer(buf=%s)", index, res_id.buf);
            }
            break;

        case _SGIMGUI_CMD_DEALLOC_IMAGE:
            {
                _sgimgui_str_t res_id = _sgimgui_image_id_string(ctx, item->args.dealloc_image.image);
                _sgimgui_snprintf(&str, "%d: sg_dealloc_image(img=%d)", index, res_id.buf);
            }
            break;

        case _SGIMGUI_CMD_DEALLOC_SAMPLER:
            {
                _sgimgui_str_t res_id = _sgimgui_sampler_id_string(ctx, item->args.dealloc_sampler.sampler);
                _sgimgui_snprintf(&str, "%d: sg_dealloc_sampler(smp=%s)", index, res_id.buf);
            }
            break;

        case _SGIMGUI_CMD_DEALLOC_SHADER:
            {
                _sgimgui_str_t res_id = _sgimgui_shader_id_string(ctx, item->args.dealloc_shader.shader);
                _sgimgui_snprintf(&str, "%d: sg_dealloc_shader(shd=%s)", index, res_id.buf);
            }
            break;

        case _SGIMGUI_CMD_DEALLOC_PIPELINE:
            {
                _sgimgui_str_t res_id = _sgimgui_pipeline_id_string(ctx, item->args.dealloc_pipeline.pipeline);
                _sgimgui_snprintf(&str, "%d: sg_dealloc_pipeline(pip=%s)", index, res_id.buf);
            }
            break;

        case _SGIMGUI_CMD_DEALLOC_VIEW:
            {
                _sgimgui_str_t res_id = _sgimgui_view_id_string(ctx, item->args.dealloc_view.view);
                _sgimgui_snprintf(&str, "%d: sg_dealloc_view(view=%s)", index, res_id.buf);
            }
            break;

        case _SGIMGUI_CMD_INIT_BUFFER:
            {
                _sgimgui_str_t res_id = _sgimgui_buffer_id_string(ctx, item->args.init_buffer.buffer);
                _sgimgui_snprintf(&str, "%d: sg_init_buffer(buf=%s, desc=..)", index, res_id.buf);
            }
            break;

        case _SGIMGUI_CMD_INIT_IMAGE:
            {
                _sgimgui_str_t res_id = _sgimgui_image_id_string(ctx, item->args.init_image.image);
                _sgimgui_snprintf(&str, "%d: sg_init_image(img=%s, desc=..)", index, res_id.buf);
            }
            break;

        case _SGIMGUI_CMD_INIT_SAMPLER:
            {
                _sgimgui_str_t res_id = _sgimgui_sampler_id_string(ctx, item->args.init_sampler.sampler);
                _sgimgui_snprintf(&str, "%d: sg_init_sampler(smp=%s, desc=..)", index, res_id.buf);
            }
            break;

        case _SGIMGUI_CMD_INIT_SHADER:
            {
                _sgimgui_str_t res_id = _sgimgui_shader_id_string(ctx, item->args.init_shader.shader);
                _sgimgui_snprintf(&str, "%d: sg_init_shader(shd=%s, desc=..)", index, res_id.buf);
            }
            break;

        case _SGIMGUI_CMD_INIT_PIPELINE:
            {
                _sgimgui_str_t res_id = _sgimgui_pipeline_id_string(ctx, item->args.init_pipeline.pipeline);
                _sgimgui_snprintf(&str, "%d: sg_init_pipeline(pip=%s, desc=..)", index, res_id.buf);
            }
            break;

        case _SGIMGUI_CMD_INIT_VIEW:
            {
                _sgimgui_str_t res_id = _sgimgui_view_id_string(ctx, item->args.init_view.view);
                _sgimgui_snprintf(&str, "%d: sg_init_view(view=%s, desc=..)", index, res_id.buf);
            }
            break;

        case _SGIMGUI_CMD_UNINIT_BUFFER:
            {
                _sgimgui_str_t res_id = _sgimgui_buffer_id_string(ctx, item->args.uninit_buffer.buffer);
                _sgimgui_snprintf(&str, "%d: sg_uninit_buffer(buf=%s)", index, res_id.buf);
            }
            break;

        case _SGIMGUI_CMD_UNINIT_IMAGE:
            {
                _sgimgui_str_t res_id = _sgimgui_image_id_string(ctx, item->args.uninit_image.image);
                _sgimgui_snprintf(&str, "%d: sg_uninit_image(img=%s)", index, res_id.buf);
            }
            break;

        case _SGIMGUI_CMD_UNINIT_SAMPLER:
            {
                _sgimgui_str_t res_id = _sgimgui_sampler_id_string(ctx, item->args.uninit_sampler.sampler);
                _sgimgui_snprintf(&str, "%d: sg_uninit_sampler(smp=%s)", index, res_id.buf);
            }
            break;

        case _SGIMGUI_CMD_UNINIT_SHADER:
            {
                _sgimgui_str_t res_id = _sgimgui_shader_id_string(ctx, item->args.uninit_shader.shader);
                _sgimgui_snprintf(&str, "%d: sg_uninit_shader(shd=%s)", index, res_id.buf);
            }
            break;

        case _SGIMGUI_CMD_UNINIT_PIPELINE:
            {
                _sgimgui_str_t res_id = _sgimgui_pipeline_id_string(ctx, item->args.uninit_pipeline.pipeline);
                _sgimgui_snprintf(&str, "%d: sg_uninit_pipeline(pip=%s)", index, res_id.buf);
            }
            break;

        case _SGIMGUI_CMD_UNINIT_VIEW:
            {
                _sgimgui_str_t res_id = _sgimgui_view_id_string(ctx, item->args.uninit_view.view);
                _sgimgui_snprintf(&str, "%d: sg_uninit_view(view=%s)", index, res_id.buf);
            }
            break;

        case _SGIMGUI_CMD_FAIL_BUFFER:
            {
                _sgimgui_str_t res_id = _sgimgui_buffer_id_string(ctx, item->args.fail_buffer.buffer);
                _sgimgui_snprintf(&str, "%d: sg_fail_buffer(buf=%s)", index, res_id.buf);
            }
            break;

        case _SGIMGUI_CMD_FAIL_IMAGE:
            {
                _sgimgui_str_t res_id = _sgimgui_image_id_string(ctx, item->args.fail_image.image);
                _sgimgui_snprintf(&str, "%d: sg_fail_image(img=%s)", index, res_id.buf);
            }
            break;

        case _SGIMGUI_CMD_FAIL_SAMPLER:
            {
                _sgimgui_str_t res_id = _sgimgui_sampler_id_string(ctx, item->args.fail_sampler.sampler);
                _sgimgui_snprintf(&str, "%d: sg_fail_sampler(smp=%s)", index, res_id.buf);
            }
            break;

        case _SGIMGUI_CMD_FAIL_SHADER:
            {
                _sgimgui_str_t res_id = _sgimgui_shader_id_string(ctx, item->args.fail_shader.shader);
                _sgimgui_snprintf(&str, "%d: sg_fail_shader(shd=%s)", index, res_id.buf);
            }
            break;

        case _SGIMGUI_CMD_FAIL_PIPELINE:
            {
                _sgimgui_str_t res_id = _sgimgui_pipeline_id_string(ctx, item->args.fail_pipeline.pipeline);
                _sgimgui_snprintf(&str, "%d: sg_fail_pipeline(shd=%s)", index, res_id.buf);
            }
            break;

        case _SGIMGUI_CMD_FAIL_VIEW:
            {
                _sgimgui_str_t res_id = _sgimgui_view_id_string(ctx, item->args.fail_view.view);
                _sgimgui_snprintf(&str, "%d: sg_fail_view(view=%s)", index, res_id.buf);
            }
            break;

        case _SGIMGUI_CMD_PUSH_DEBUG_GROUP:
            _sgimgui_snprintf(&str, "%d: sg_push_debug_group(name=%s)", index,
                item->args.push_debug_group.name.buf);
            break;

        case _SGIMGUI_CMD_POP_DEBUG_GROUP:
            _sgimgui_snprintf(&str, "%d: sg_pop_debug_group()", index);
            break;

        default:
            _sgimgui_snprintf(&str, "%d: ???", index);
            break;
    }
    return str;
}

/*--- CAPTURE CALLBACKS ------------------------------------------------------*/
_SOKOL_PRIVATE void _sgimgui_reset_state_cache(void* user_data) {
    _sgimgui_t* ctx = (_sgimgui_t*) user_data;
    SOKOL_ASSERT(ctx);
    _sgimgui_capture_item_t* item = _sgimgui_capture_next_write_item(ctx);
    if (item) {
        item->cmd = _SGIMGUI_CMD_RESET_STATE_CACHE;
        item->color = _SGIMGUI_COLOR_OTHER;
    }
    if (ctx->hooks.reset_state_cache) {
        ctx->hooks.reset_state_cache(ctx->hooks.user_data);
    }
}

_SOKOL_PRIVATE void _sgimgui_make_buffer(const sg_buffer_desc* desc, sg_buffer buf_id, void* user_data) {
    _sgimgui_t* ctx = (_sgimgui_t*) user_data;
    SOKOL_ASSERT(ctx);
    _sgimgui_capture_item_t* item = _sgimgui_capture_next_write_item(ctx);
    if (item) {
        item->cmd = _SGIMGUI_CMD_MAKE_BUFFER;
        item->color = _SGIMGUI_COLOR_RSRC;
        item->args.make_buffer.result = buf_id;
    }
    if (ctx->hooks.make_buffer) {
        ctx->hooks.make_buffer(desc, buf_id, ctx->hooks.user_data);
    }
    if (buf_id.id != SG_INVALID_ID) {
        _sgimgui_buffer_created(ctx, buf_id, _sgimgui_slot_index(buf_id.id), desc);
    }
}

_SOKOL_PRIVATE void _sgimgui_make_image(const sg_image_desc* desc, sg_image img_id, void* user_data) {
    _sgimgui_t* ctx = (_sgimgui_t*) user_data;
    SOKOL_ASSERT(ctx);
    _sgimgui_capture_item_t* item = _sgimgui_capture_next_write_item(ctx);
    if (item) {
        item->cmd = _SGIMGUI_CMD_MAKE_IMAGE;
        item->color = _SGIMGUI_COLOR_RSRC;
        item->args.make_image.result = img_id;
    }
    if (ctx->hooks.make_image) {
        ctx->hooks.make_image(desc, img_id, ctx->hooks.user_data);
    }
    if (img_id.id != SG_INVALID_ID) {
        _sgimgui_image_created(ctx, img_id, _sgimgui_slot_index(img_id.id), desc);
    }
}

_SOKOL_PRIVATE void _sgimgui_make_sampler(const sg_sampler_desc* desc, sg_sampler smp_id, void* user_data) {
    _sgimgui_t* ctx = (_sgimgui_t*) user_data;
    SOKOL_ASSERT(ctx);
    _sgimgui_capture_item_t* item = _sgimgui_capture_next_write_item(ctx);
    if (item) {
        item->cmd = _SGIMGUI_CMD_MAKE_SAMPLER;
        item->color = _SGIMGUI_COLOR_RSRC;
        item->args.make_sampler.result = smp_id;
    }
    if (ctx->hooks.make_sampler) {
        ctx->hooks.make_sampler(desc, smp_id, ctx->hooks.user_data);
    }
    if (smp_id.id != SG_INVALID_ID) {
        _sgimgui_sampler_created(ctx, smp_id, _sgimgui_slot_index(smp_id.id), desc);
    }
}

_SOKOL_PRIVATE void _sgimgui_make_shader(const sg_shader_desc* desc, sg_shader shd_id, void* user_data) {
    _sgimgui_t* ctx = (_sgimgui_t*) user_data;
    SOKOL_ASSERT(ctx);
    _sgimgui_capture_item_t* item = _sgimgui_capture_next_write_item(ctx);
    if (item) {
        item->cmd = _SGIMGUI_CMD_MAKE_SHADER;
        item->color = _SGIMGUI_COLOR_RSRC;
        item->args.make_shader.result = shd_id;
    }
    if (ctx->hooks.make_shader) {
        ctx->hooks.make_shader(desc, shd_id, ctx->hooks.user_data);
    }
    if (shd_id.id != SG_INVALID_ID) {
        _sgimgui_shader_created(ctx, shd_id, _sgimgui_slot_index(shd_id.id), desc);
    }
}

_SOKOL_PRIVATE void _sgimgui_make_pipeline(const sg_pipeline_desc* desc, sg_pipeline pip_id, void* user_data) {
    _sgimgui_t* ctx = (_sgimgui_t*) user_data;
    SOKOL_ASSERT(ctx);
    _sgimgui_capture_item_t* item = _sgimgui_capture_next_write_item(ctx);
    if (item) {
        item->cmd = _SGIMGUI_CMD_MAKE_PIPELINE;
        item->color = _SGIMGUI_COLOR_RSRC;
        item->args.make_pipeline.result = pip_id;
    }
    if (ctx->hooks.make_pipeline) {
        ctx->hooks.make_pipeline(desc, pip_id, ctx->hooks.user_data);
    }
    if (pip_id.id != SG_INVALID_ID) {
        _sgimgui_pipeline_created(ctx, pip_id, _sgimgui_slot_index(pip_id.id), desc);
    }
}

_SOKOL_PRIVATE void _sgimgui_make_view(const sg_view_desc* desc, sg_view view_id, void* user_data) {
    _sgimgui_t* ctx = (_sgimgui_t*) user_data;
    SOKOL_ASSERT(ctx);
    _sgimgui_capture_item_t* item = _sgimgui_capture_next_write_item(ctx);
    if (item) {
        item->cmd = _SGIMGUI_CMD_MAKE_VIEW;
        item->color = _SGIMGUI_COLOR_RSRC;
        item->args.make_view.result = view_id;
    }
    if (ctx->hooks.make_view) {
        ctx->hooks.make_view(desc, view_id, ctx->hooks.user_data);
    }
    if (view_id.id != SG_INVALID_ID) {
        _sgimgui_view_created(ctx, view_id, _sgimgui_slot_index(view_id.id), desc);
    }
}

_SOKOL_PRIVATE void _sgimgui_destroy_buffer(sg_buffer buf, void* user_data) {
    _sgimgui_t* ctx = (_sgimgui_t*) user_data;
    SOKOL_ASSERT(ctx);
    _sgimgui_capture_item_t* item = _sgimgui_capture_next_write_item(ctx);
    if (item) {
        item->cmd = _SGIMGUI_CMD_DESTROY_BUFFER;
        item->color = _SGIMGUI_COLOR_RSRC;
        item->args.destroy_buffer.buffer = buf;
    }
    if (ctx->hooks.destroy_buffer) {
        ctx->hooks.destroy_buffer(buf, ctx->hooks.user_data);
    }
    if (buf.id != SG_INVALID_ID) {
        _sgimgui_buffer_destroyed(ctx, _sgimgui_slot_index(buf.id));
    }
}

_SOKOL_PRIVATE void _sgimgui_destroy_image(sg_image img, void* user_data) {
    _sgimgui_t* ctx = (_sgimgui_t*) user_data;
    SOKOL_ASSERT(ctx);
    _sgimgui_capture_item_t* item = _sgimgui_capture_next_write_item(ctx);
    if (item) {
        item->cmd = _SGIMGUI_CMD_DESTROY_IMAGE;
        item->color = _SGIMGUI_COLOR_RSRC;
        item->args.destroy_image.image = img;
    }
    if (ctx->hooks.destroy_image) {
        ctx->hooks.destroy_image(img, ctx->hooks.user_data);
    }
    if (img.id != SG_INVALID_ID) {
        _sgimgui_image_destroyed(ctx, _sgimgui_slot_index(img.id));
    }
}

_SOKOL_PRIVATE void _sgimgui_destroy_sampler(sg_sampler smp, void* user_data) {
    _sgimgui_t* ctx = (_sgimgui_t*) user_data;
    SOKOL_ASSERT(ctx);
    _sgimgui_capture_item_t* item = _sgimgui_capture_next_write_item(ctx);
    if (item) {
        item->cmd = _SGIMGUI_CMD_DESTROY_SAMPLER;
        item->color = _SGIMGUI_COLOR_RSRC;
        item->args.destroy_sampler.sampler = smp;
    }
    if (ctx->hooks.destroy_sampler) {
        ctx->hooks.destroy_sampler(smp, ctx->hooks.user_data);
    }
    if (smp.id != SG_INVALID_ID) {
        _sgimgui_sampler_destroyed(ctx, _sgimgui_slot_index(smp.id));
    }
}

_SOKOL_PRIVATE void _sgimgui_destroy_shader(sg_shader shd, void* user_data) {
    _sgimgui_t* ctx = (_sgimgui_t*) user_data;
    SOKOL_ASSERT(ctx);
    _sgimgui_capture_item_t* item = _sgimgui_capture_next_write_item(ctx);
    if (item) {
        item->cmd = _SGIMGUI_CMD_DESTROY_SHADER;
        item->color = _SGIMGUI_COLOR_RSRC;
        item->args.destroy_shader.shader = shd;
    }
    if (ctx->hooks.destroy_shader) {
        ctx->hooks.destroy_shader(shd, ctx->hooks.user_data);
    }
    if (shd.id != SG_INVALID_ID) {
        _sgimgui_shader_destroyed(ctx, _sgimgui_slot_index(shd.id));
    }
}

_SOKOL_PRIVATE void _sgimgui_destroy_pipeline(sg_pipeline pip, void* user_data) {
    _sgimgui_t* ctx = (_sgimgui_t*) user_data;
    SOKOL_ASSERT(ctx);
    _sgimgui_capture_item_t* item = _sgimgui_capture_next_write_item(ctx);
    if (item) {
        item->cmd = _SGIMGUI_CMD_DESTROY_PIPELINE;
        item->color = _SGIMGUI_COLOR_RSRC;
        item->args.destroy_pipeline.pipeline = pip;
    }
    if (ctx->hooks.destroy_pipeline) {
        ctx->hooks.destroy_pipeline(pip, ctx->hooks.user_data);
    }
    if (pip.id != SG_INVALID_ID) {
        _sgimgui_pipeline_destroyed(ctx, _sgimgui_slot_index(pip.id));
    }
}

_SOKOL_PRIVATE void _sgimgui_destroy_view(sg_view view, void* user_data) {
    _sgimgui_t* ctx = (_sgimgui_t*) user_data;
    SOKOL_ASSERT(ctx);
    _sgimgui_capture_item_t* item = _sgimgui_capture_next_write_item(ctx);
    if (item) {
        item->cmd = _SGIMGUI_CMD_DESTROY_VIEW;
        item->color = _SGIMGUI_COLOR_RSRC;
        item->args.destroy_view.view = view;
    }
    if (ctx->hooks.destroy_view) {
        ctx->hooks.destroy_view(view, ctx->hooks.user_data);
    }
    if (view.id != SG_INVALID_ID) {
        _sgimgui_view_destroyed(ctx, _sgimgui_slot_index(view.id));
    }
}

_SOKOL_PRIVATE void _sgimgui_update_buffer(sg_buffer buf, const sg_range* data, void* user_data) {
    _sgimgui_t* ctx = (_sgimgui_t*) user_data;
    SOKOL_ASSERT(ctx);
    _sgimgui_capture_item_t* item = _sgimgui_capture_next_write_item(ctx);
    if (item) {
        item->cmd = _SGIMGUI_CMD_UPDATE_BUFFER;
        item->color = _SGIMGUI_COLOR_RSRC;
        item->args.update_buffer.buffer = buf;
        item->args.update_buffer.data_size = data->size;
    }
    if (ctx->hooks.update_buffer) {
        ctx->hooks.update_buffer(buf, data, ctx->hooks.user_data);
    }
}

_SOKOL_PRIVATE void _sgimgui_update_image(sg_image img, const sg_image_data* data, void* user_data) {
    _sgimgui_t* ctx = (_sgimgui_t*) user_data;
    SOKOL_ASSERT(ctx);
    _sgimgui_capture_item_t* item = _sgimgui_capture_next_write_item(ctx);
    if (item) {
        item->cmd = _SGIMGUI_CMD_UPDATE_IMAGE;
        item->color = _SGIMGUI_COLOR_RSRC;
        item->args.update_image.image = img;
    }
    if (ctx->hooks.update_image) {
        ctx->hooks.update_image(img, data, ctx->hooks.user_data);
    }
}

_SOKOL_PRIVATE void _sgimgui_append_buffer(sg_buffer buf, const sg_range* data, int result, void* user_data) {
    _sgimgui_t* ctx = (_sgimgui_t*) user_data;
    SOKOL_ASSERT(ctx);
    _sgimgui_capture_item_t* item = _sgimgui_capture_next_write_item(ctx);
    if (item) {
        item->cmd = _SGIMGUI_CMD_APPEND_BUFFER;
        item->color = _SGIMGUI_COLOR_RSRC;
        item->args.append_buffer.buffer = buf;
        item->args.append_buffer.data_size = data->size;
        item->args.append_buffer.result = result;
    }
    if (ctx->hooks.append_buffer) {
        ctx->hooks.append_buffer(buf, data, result, ctx->hooks.user_data);
    }
}

_SOKOL_PRIVATE void _sgimgui_begin_pass(const sg_pass* pass, void* user_data) {
    _sgimgui_t* ctx = (_sgimgui_t*) user_data;
    SOKOL_ASSERT(ctx);
    _sgimgui_capture_item_t* item = _sgimgui_capture_next_write_item(ctx);
    if (item) {
        SOKOL_ASSERT(pass);
        item->cmd = _SGIMGUI_CMD_BEGIN_PASS;
        item->color = _SGIMGUI_COLOR_PASS;
        item->args.begin_pass.pass = *pass;
    }
    if (ctx->hooks.begin_pass) {
        ctx->hooks.begin_pass(pass, ctx->hooks.user_data);
    }
}

_SOKOL_PRIVATE void _sgimgui_apply_viewport(int x, int y, int width, int height, bool origin_top_left, void* user_data) {
    _sgimgui_t* ctx = (_sgimgui_t*) user_data;
    SOKOL_ASSERT(ctx);
    _sgimgui_capture_item_t* item = _sgimgui_capture_next_write_item(ctx);
    if (item) {
        item->cmd = _SGIMGUI_CMD_APPLY_VIEWPORT;
        item->color = _SGIMGUI_COLOR_APPLY;
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

_SOKOL_PRIVATE void _sgimgui_apply_scissor_rect(int x, int y, int width, int height, bool origin_top_left, void* user_data) {
    _sgimgui_t* ctx = (_sgimgui_t*) user_data;
    SOKOL_ASSERT(ctx);
    _sgimgui_capture_item_t* item = _sgimgui_capture_next_write_item(ctx);
    if (item) {
        item->cmd = _SGIMGUI_CMD_APPLY_SCISSOR_RECT;
        item->color = _SGIMGUI_COLOR_APPLY;
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

_SOKOL_PRIVATE void _sgimgui_apply_pipeline(sg_pipeline pip, void* user_data) {
    _sgimgui_t* ctx = (_sgimgui_t*) user_data;
    SOKOL_ASSERT(ctx);
    ctx->cur_pipeline = pip;    /* stored for _sgimgui_apply_uniforms */
    _sgimgui_capture_item_t* item = _sgimgui_capture_next_write_item(ctx);
    if (item) {
        item->cmd = _SGIMGUI_CMD_APPLY_PIPELINE;
        item->color = _SGIMGUI_COLOR_APPLY;
        item->args.apply_pipeline.pipeline = pip;
    }
    if (ctx->hooks.apply_pipeline) {
        ctx->hooks.apply_pipeline(pip, ctx->hooks.user_data);
    }
}

_SOKOL_PRIVATE void _sgimgui_apply_bindings(const sg_bindings* bindings, void* user_data) {
    _sgimgui_t* ctx = (_sgimgui_t*) user_data;
    SOKOL_ASSERT(ctx);
    _sgimgui_capture_item_t* item = _sgimgui_capture_next_write_item(ctx);
    if (item) {
        SOKOL_ASSERT(bindings);
        item->cmd = _SGIMGUI_CMD_APPLY_BINDINGS;
        item->color = _SGIMGUI_COLOR_APPLY;
        item->args.apply_bindings.bindings = *bindings;
    }
    if (ctx->hooks.apply_bindings) {
        ctx->hooks.apply_bindings(bindings, ctx->hooks.user_data);
    }
}

_SOKOL_PRIVATE void _sgimgui_apply_uniforms(int ub_slot, const sg_range* data, void* user_data) {
    _sgimgui_t* ctx = (_sgimgui_t*) user_data;
    SOKOL_ASSERT(ctx);
    SOKOL_ASSERT(data);
    _sgimgui_capture_item_t* item = _sgimgui_capture_next_write_item(ctx);
    if (item) {
        item->cmd = _SGIMGUI_CMD_APPLY_UNIFORMS;
        item->color = _SGIMGUI_COLOR_APPLY;
        _sgimgui_args_apply_uniforms_t* args = &item->args.apply_uniforms;
        args->ub_slot = ub_slot;
        args->data_size = data->size;
        args->pipeline = ctx->cur_pipeline;
        args->ubuf_pos = _sgimgui_capture_uniforms(ctx, data);
    }
    if (ctx->hooks.apply_uniforms) {
        ctx->hooks.apply_uniforms(ub_slot, data, ctx->hooks.user_data);
    }
}

_SOKOL_PRIVATE void _sgimgui_draw(int base_element, int num_elements, int num_instances, void* user_data) {
    _sgimgui_t* ctx = (_sgimgui_t*) user_data;
    SOKOL_ASSERT(ctx);
    _sgimgui_capture_item_t* item = _sgimgui_capture_next_write_item(ctx);
    if (item) {
        item->cmd = _SGIMGUI_CMD_DRAW;
        item->color = _SGIMGUI_COLOR_DRAW;
        item->args.draw.base_element = base_element;
        item->args.draw.num_elements = num_elements;
        item->args.draw.num_instances = num_instances;
    }
    if (ctx->hooks.draw) {
        ctx->hooks.draw(base_element, num_elements, num_instances, ctx->hooks.user_data);
    }
}

_SOKOL_PRIVATE void _sgimgui_draw_ex(int base_element, int num_elements, int num_instances, int base_vertex, int base_instance, void* user_data) {
    _sgimgui_t* ctx = (_sgimgui_t*)user_data;
    SOKOL_ASSERT(ctx);
    _sgimgui_capture_item_t* item = _sgimgui_capture_next_write_item(ctx);
    if (item) {
        item->cmd = _SGIMGUI_CMD_DRAW_EX;
        item->color = _SGIMGUI_COLOR_DRAW;
        item->args.draw_ex.base_element = base_element;
        item->args.draw_ex.num_elements = num_elements;
        item->args.draw_ex.num_instances = num_instances;
        item->args.draw_ex.base_vertex = base_vertex;
        item->args.draw_ex.base_instance = base_instance;
    }
    if (ctx->hooks.draw_ex) {
        ctx->hooks.draw_ex(base_element, num_elements, num_instances, base_vertex, base_instance, ctx->hooks.user_data);
    }
}

_SOKOL_PRIVATE void _sgimgui_dispatch(int num_groups_x, int num_groups_y, int num_groups_z, void* user_data) {
    _sgimgui_t* ctx = (_sgimgui_t*) user_data;
    SOKOL_ASSERT(ctx);
    _sgimgui_capture_item_t* item = _sgimgui_capture_next_write_item(ctx);
    if (item) {
        item->cmd = _SGIMGUI_CMD_DISPATCH;
        item->color = _SGIMGUI_COLOR_DRAW;
        item->args.dispatch.num_groups_x = num_groups_x;
        item->args.dispatch.num_groups_y = num_groups_y;
        item->args.dispatch.num_groups_z = num_groups_z;
    }
    if (ctx->hooks.dispatch) {
        ctx->hooks.dispatch(num_groups_x, num_groups_y, num_groups_z, ctx->hooks.user_data);
    }
}

_SOKOL_PRIVATE void _sgimgui_end_pass(void* user_data) {
    _sgimgui_t* ctx = (_sgimgui_t*) user_data;
    SOKOL_ASSERT(ctx);
    ctx->cur_pipeline.id = SG_INVALID_ID;
    _sgimgui_capture_item_t* item = _sgimgui_capture_next_write_item(ctx);
    if (item) {
        item->cmd = _SGIMGUI_CMD_END_PASS;
        item->color = _SGIMGUI_COLOR_PASS;
    }
    if (ctx->hooks.end_pass) {
        ctx->hooks.end_pass(ctx->hooks.user_data);
    }
}

_SOKOL_PRIVATE void _sgimgui_commit(void* user_data) {
    _sgimgui_t* ctx = (_sgimgui_t*) user_data;
    SOKOL_ASSERT(ctx);
    _sgimgui_capture_item_t* item = _sgimgui_capture_next_write_item(ctx);
    if (item) {
        item->cmd = _SGIMGUI_CMD_COMMIT;
        item->color = _SGIMGUI_COLOR_OTHER;
    }
    _sgimgui_capture_next_frame(ctx);
    if (ctx->hooks.commit) {
        ctx->hooks.commit(ctx->hooks.user_data);
    }
}

_SOKOL_PRIVATE void _sgimgui_alloc_buffer(sg_buffer result, void* user_data) {
    _sgimgui_t* ctx = (_sgimgui_t*) user_data;
    SOKOL_ASSERT(ctx);
    _sgimgui_capture_item_t* item = _sgimgui_capture_next_write_item(ctx);
    if (item) {
        item->cmd = _SGIMGUI_CMD_ALLOC_BUFFER;
        item->color = _SGIMGUI_COLOR_RSRC;
        item->args.alloc_buffer.result = result;
    }
    if (ctx->hooks.alloc_buffer) {
        ctx->hooks.alloc_buffer(result, ctx->hooks.user_data);
    }
}

_SOKOL_PRIVATE void _sgimgui_alloc_image(sg_image result, void* user_data) {
    _sgimgui_t* ctx = (_sgimgui_t*) user_data;
    SOKOL_ASSERT(ctx);
    _sgimgui_capture_item_t* item = _sgimgui_capture_next_write_item(ctx);
    if (item) {
        item->cmd = _SGIMGUI_CMD_ALLOC_IMAGE;
        item->color = _SGIMGUI_COLOR_RSRC;
        item->args.alloc_image.result = result;
    }
    if (ctx->hooks.alloc_image) {
        ctx->hooks.alloc_image(result, ctx->hooks.user_data);
    }
}

_SOKOL_PRIVATE void _sgimgui_alloc_sampler(sg_sampler result, void* user_data) {
    _sgimgui_t* ctx = (_sgimgui_t*) user_data;
    SOKOL_ASSERT(ctx);
    _sgimgui_capture_item_t* item = _sgimgui_capture_next_write_item(ctx);
    if (item) {
        item->cmd = _SGIMGUI_CMD_ALLOC_SAMPLER;
        item->color = _SGIMGUI_COLOR_RSRC;
        item->args.alloc_sampler.result = result;
    }
    if (ctx->hooks.alloc_sampler) {
        ctx->hooks.alloc_sampler(result, ctx->hooks.user_data);
    }
}

_SOKOL_PRIVATE void _sgimgui_alloc_shader(sg_shader result, void* user_data) {
    _sgimgui_t* ctx = (_sgimgui_t*) user_data;
    SOKOL_ASSERT(ctx);
    _sgimgui_capture_item_t* item = _sgimgui_capture_next_write_item(ctx);
    if (item) {
        item->cmd = _SGIMGUI_CMD_ALLOC_SHADER;
        item->color = _SGIMGUI_COLOR_RSRC;
        item->args.alloc_shader.result = result;
    }
    if (ctx->hooks.alloc_shader) {
        ctx->hooks.alloc_shader(result, ctx->hooks.user_data);
    }
}

_SOKOL_PRIVATE void _sgimgui_alloc_pipeline(sg_pipeline result, void* user_data) {
    _sgimgui_t* ctx = (_sgimgui_t*) user_data;
    SOKOL_ASSERT(ctx);
    _sgimgui_capture_item_t* item = _sgimgui_capture_next_write_item(ctx);
    if (item) {
        item->cmd = _SGIMGUI_CMD_ALLOC_PIPELINE;
        item->color = _SGIMGUI_COLOR_RSRC;
        item->args.alloc_pipeline.result = result;
    }
    if (ctx->hooks.alloc_pipeline) {
        ctx->hooks.alloc_pipeline(result, ctx->hooks.user_data);
    }
}

_SOKOL_PRIVATE void _sgimgui_alloc_view(sg_view result, void* user_data) {
    _sgimgui_t* ctx = (_sgimgui_t*) user_data;
    SOKOL_ASSERT(ctx);
    _sgimgui_capture_item_t* item = _sgimgui_capture_next_write_item(ctx);
    if (item) {
        item->cmd = _SGIMGUI_CMD_ALLOC_VIEW;
        item->color = _SGIMGUI_COLOR_RSRC;
        item->args.alloc_view.result = result;
    }
    if (ctx->hooks.alloc_view) {
        ctx->hooks.alloc_view(result, ctx->hooks.user_data);
    }
}

_SOKOL_PRIVATE void _sgimgui_dealloc_buffer(sg_buffer buf_id, void* user_data) {
    _sgimgui_t* ctx = (_sgimgui_t*) user_data;
    SOKOL_ASSERT(ctx);
    _sgimgui_capture_item_t* item = _sgimgui_capture_next_write_item(ctx);
    if (item) {
        item->cmd = _SGIMGUI_CMD_DEALLOC_BUFFER;
        item->color = _SGIMGUI_COLOR_RSRC;
        item->args.dealloc_buffer.buffer = buf_id;
    }
    if (ctx->hooks.dealloc_buffer) {
        ctx->hooks.dealloc_buffer(buf_id, ctx->hooks.user_data);
    }
}

_SOKOL_PRIVATE void _sgimgui_dealloc_image(sg_image img_id, void* user_data) {
    _sgimgui_t* ctx = (_sgimgui_t*) user_data;
    SOKOL_ASSERT(ctx);
    _sgimgui_capture_item_t* item = _sgimgui_capture_next_write_item(ctx);
    if (item) {
        item->cmd = _SGIMGUI_CMD_DEALLOC_IMAGE;
        item->color = _SGIMGUI_COLOR_RSRC;
        item->args.dealloc_image.image = img_id;
    }
    if (ctx->hooks.dealloc_image) {
        ctx->hooks.dealloc_image(img_id, ctx->hooks.user_data);
    }
}

_SOKOL_PRIVATE void _sgimgui_dealloc_sampler(sg_sampler smp_id, void* user_data) {
    _sgimgui_t* ctx = (_sgimgui_t*) user_data;
    SOKOL_ASSERT(ctx);
    _sgimgui_capture_item_t* item = _sgimgui_capture_next_write_item(ctx);
    if (item) {
        item->cmd = _SGIMGUI_CMD_DEALLOC_SAMPLER;
        item->color = _SGIMGUI_COLOR_RSRC;
        item->args.dealloc_sampler.sampler = smp_id;
    }
    if (ctx->hooks.dealloc_sampler) {
        ctx->hooks.dealloc_sampler(smp_id, ctx->hooks.user_data);
    }
}

_SOKOL_PRIVATE void _sgimgui_dealloc_shader(sg_shader shd_id, void* user_data) {
    _sgimgui_t* ctx = (_sgimgui_t*) user_data;
    SOKOL_ASSERT(ctx);
    _sgimgui_capture_item_t* item = _sgimgui_capture_next_write_item(ctx);
    if (item) {
        item->cmd = _SGIMGUI_CMD_DEALLOC_SHADER;
        item->color = _SGIMGUI_COLOR_RSRC;
        item->args.dealloc_shader.shader = shd_id;
    }
    if (ctx->hooks.dealloc_shader) {
        ctx->hooks.dealloc_shader(shd_id, ctx->hooks.user_data);
    }
}

_SOKOL_PRIVATE void _sgimgui_dealloc_pipeline(sg_pipeline pip_id, void* user_data) {
    _sgimgui_t* ctx = (_sgimgui_t*) user_data;
    SOKOL_ASSERT(ctx);
    _sgimgui_capture_item_t* item = _sgimgui_capture_next_write_item(ctx);
    if (item) {
        item->cmd = _SGIMGUI_CMD_DEALLOC_PIPELINE;
        item->color = _SGIMGUI_COLOR_RSRC;
        item->args.dealloc_pipeline.pipeline = pip_id;
    }
    if (ctx->hooks.dealloc_pipeline) {
        ctx->hooks.dealloc_pipeline(pip_id, ctx->hooks.user_data);
    }
}

_SOKOL_PRIVATE void _sgimgui_dealloc_view(sg_view view_id, void* user_data) {
    _sgimgui_t* ctx = (_sgimgui_t*) user_data;
    SOKOL_ASSERT(ctx);
    _sgimgui_capture_item_t* item = _sgimgui_capture_next_write_item(ctx);
    if (item) {
        item->cmd = _SGIMGUI_CMD_DEALLOC_VIEW;
        item->color = _SGIMGUI_COLOR_RSRC;
        item->args.dealloc_view.view = view_id;
    }
    if (ctx->hooks.dealloc_view) {
        ctx->hooks.dealloc_view(view_id, ctx->hooks.user_data);
    }
}

_SOKOL_PRIVATE void _sgimgui_init_buffer(sg_buffer buf_id, const sg_buffer_desc* desc, void* user_data) {
    _sgimgui_t* ctx = (_sgimgui_t*) user_data;
    SOKOL_ASSERT(ctx);
    _sgimgui_capture_item_t* item = _sgimgui_capture_next_write_item(ctx);
    if (item) {
        item->cmd = _SGIMGUI_CMD_INIT_BUFFER;
        item->color = _SGIMGUI_COLOR_RSRC;
        item->args.init_buffer.buffer = buf_id;
    }
    if (ctx->hooks.init_buffer) {
        ctx->hooks.init_buffer(buf_id, desc, ctx->hooks.user_data);
    }
    if (buf_id.id != SG_INVALID_ID) {
        _sgimgui_buffer_created(ctx, buf_id, _sgimgui_slot_index(buf_id.id), desc);
    }
}

_SOKOL_PRIVATE void _sgimgui_init_image(sg_image img_id, const sg_image_desc* desc, void* user_data) {
    _sgimgui_t* ctx = (_sgimgui_t*) user_data;
    SOKOL_ASSERT(ctx);
    _sgimgui_capture_item_t* item = _sgimgui_capture_next_write_item(ctx);
    if (item) {
        item->cmd = _SGIMGUI_CMD_INIT_IMAGE;
        item->color = _SGIMGUI_COLOR_RSRC;
        item->args.init_image.image = img_id;
    }
    if (ctx->hooks.init_image) {
        ctx->hooks.init_image(img_id, desc, ctx->hooks.user_data);
    }
    if (img_id.id != SG_INVALID_ID) {
        _sgimgui_image_created(ctx, img_id, _sgimgui_slot_index(img_id.id), desc);
    }
}

_SOKOL_PRIVATE void _sgimgui_init_sampler(sg_sampler smp_id, const sg_sampler_desc* desc, void* user_data) {
    _sgimgui_t* ctx = (_sgimgui_t*) user_data;
    SOKOL_ASSERT(ctx);
    _sgimgui_capture_item_t* item = _sgimgui_capture_next_write_item(ctx);
    if (item) {
        item->cmd = _SGIMGUI_CMD_INIT_SAMPLER;
        item->color = _SGIMGUI_COLOR_RSRC;
        item->args.init_sampler.sampler = smp_id;
    }
    if (ctx->hooks.init_sampler) {
        ctx->hooks.init_sampler(smp_id, desc, ctx->hooks.user_data);
    }
    if (smp_id.id != SG_INVALID_ID) {
        _sgimgui_sampler_created(ctx, smp_id, _sgimgui_slot_index(smp_id.id), desc);
    }
}

_SOKOL_PRIVATE void _sgimgui_init_shader(sg_shader shd_id, const sg_shader_desc* desc, void* user_data) {
    _sgimgui_t* ctx = (_sgimgui_t*) user_data;
    SOKOL_ASSERT(ctx);
    _sgimgui_capture_item_t* item = _sgimgui_capture_next_write_item(ctx);
    if (item) {
        item->cmd = _SGIMGUI_CMD_INIT_SHADER;
        item->color = _SGIMGUI_COLOR_RSRC;
        item->args.init_shader.shader = shd_id;
    }
    if (ctx->hooks.init_shader) {
        ctx->hooks.init_shader(shd_id, desc, ctx->hooks.user_data);
    }
    if (shd_id.id != SG_INVALID_ID) {
        _sgimgui_shader_created(ctx, shd_id, _sgimgui_slot_index(shd_id.id), desc);
    }
}

_SOKOL_PRIVATE void _sgimgui_init_pipeline(sg_pipeline pip_id, const sg_pipeline_desc* desc, void* user_data) {
    _sgimgui_t* ctx = (_sgimgui_t*) user_data;
    SOKOL_ASSERT(ctx);
    _sgimgui_capture_item_t* item = _sgimgui_capture_next_write_item(ctx);
    if (item) {
        item->cmd = _SGIMGUI_CMD_INIT_PIPELINE;
        item->color = _SGIMGUI_COLOR_RSRC;
        item->args.init_pipeline.pipeline = pip_id;
    }
    if (ctx->hooks.init_pipeline) {
        ctx->hooks.init_pipeline(pip_id, desc, ctx->hooks.user_data);
    }
    if (pip_id.id != SG_INVALID_ID) {
        _sgimgui_pipeline_created(ctx, pip_id, _sgimgui_slot_index(pip_id.id), desc);
    }
}

_SOKOL_PRIVATE void _sgimgui_init_view(sg_view view_id, const sg_view_desc* desc, void* user_data) {
    _sgimgui_t* ctx = (_sgimgui_t*) user_data;
    SOKOL_ASSERT(ctx);
    _sgimgui_capture_item_t* item = _sgimgui_capture_next_write_item(ctx);
    if (item) {
        item->cmd = _SGIMGUI_CMD_INIT_VIEW;
        item->color = _SGIMGUI_COLOR_RSRC;
        item->args.init_view.view = view_id;
    }
    if (ctx->hooks.init_view) {
        ctx->hooks.init_view(view_id, desc, ctx->hooks.user_data);
    }
    if (view_id.id != SG_INVALID_ID) {
        _sgimgui_view_created(ctx, view_id, _sgimgui_slot_index(view_id.id), desc);
    }
}

_SOKOL_PRIVATE void _sgimgui_uninit_buffer(sg_buffer buf, void* user_data) {
    _sgimgui_t* ctx = (_sgimgui_t*) user_data;
    SOKOL_ASSERT(ctx);
    _sgimgui_capture_item_t* item = _sgimgui_capture_next_write_item(ctx);
    if (item) {
        item->cmd = _SGIMGUI_CMD_UNINIT_BUFFER;
        item->color = _SGIMGUI_COLOR_RSRC;
        item->args.uninit_buffer.buffer = buf;
    }
    if (ctx->hooks.uninit_buffer) {
        ctx->hooks.uninit_buffer(buf, ctx->hooks.user_data);
    }
    if (buf.id != SG_INVALID_ID) {
        _sgimgui_buffer_destroyed(ctx, _sgimgui_slot_index(buf.id));
    }
}

_SOKOL_PRIVATE void _sgimgui_uninit_image(sg_image img, void* user_data) {
    _sgimgui_t* ctx = (_sgimgui_t*) user_data;
    SOKOL_ASSERT(ctx);
    _sgimgui_capture_item_t* item = _sgimgui_capture_next_write_item(ctx);
    if (item) {
        item->cmd = _SGIMGUI_CMD_UNINIT_IMAGE;
        item->color = _SGIMGUI_COLOR_RSRC;
        item->args.uninit_image.image = img;
    }
    if (ctx->hooks.uninit_image) {
        ctx->hooks.uninit_image(img, ctx->hooks.user_data);
    }
    if (img.id != SG_INVALID_ID) {
        _sgimgui_image_destroyed(ctx, _sgimgui_slot_index(img.id));
    }
}

_SOKOL_PRIVATE void _sgimgui_uninit_sampler(sg_sampler smp, void* user_data) {
    _sgimgui_t* ctx = (_sgimgui_t*) user_data;
    SOKOL_ASSERT(ctx);
    _sgimgui_capture_item_t* item = _sgimgui_capture_next_write_item(ctx);
    if (item) {
        item->cmd = _SGIMGUI_CMD_UNINIT_SAMPLER;
        item->color = _SGIMGUI_COLOR_RSRC;
        item->args.uninit_sampler.sampler = smp;
    }
    if (ctx->hooks.uninit_sampler) {
        ctx->hooks.uninit_sampler(smp, ctx->hooks.user_data);
    }
    if (smp.id != SG_INVALID_ID) {
        _sgimgui_sampler_destroyed(ctx, _sgimgui_slot_index(smp.id));
    }
}

_SOKOL_PRIVATE void _sgimgui_uninit_shader(sg_shader shd, void* user_data) {
    _sgimgui_t* ctx = (_sgimgui_t*) user_data;
    SOKOL_ASSERT(ctx);
    _sgimgui_capture_item_t* item = _sgimgui_capture_next_write_item(ctx);
    if (item) {
        item->cmd = _SGIMGUI_CMD_UNINIT_SHADER;
        item->color = _SGIMGUI_COLOR_RSRC;
        item->args.uninit_shader.shader = shd;
    }
    if (ctx->hooks.uninit_shader) {
        ctx->hooks.uninit_shader(shd, ctx->hooks.user_data);
    }
    if (shd.id != SG_INVALID_ID) {
        _sgimgui_shader_destroyed(ctx, _sgimgui_slot_index(shd.id));
    }
}

_SOKOL_PRIVATE void _sgimgui_uninit_pipeline(sg_pipeline pip, void* user_data) {
    _sgimgui_t* ctx = (_sgimgui_t*) user_data;
    SOKOL_ASSERT(ctx);
    _sgimgui_capture_item_t* item = _sgimgui_capture_next_write_item(ctx);
    if (item) {
        item->cmd = _SGIMGUI_CMD_UNINIT_PIPELINE;
        item->color = _SGIMGUI_COLOR_RSRC;
        item->args.uninit_pipeline.pipeline = pip;
    }
    if (ctx->hooks.uninit_pipeline) {
        ctx->hooks.uninit_pipeline(pip, ctx->hooks.user_data);
    }
    if (pip.id != SG_INVALID_ID) {
        _sgimgui_pipeline_destroyed(ctx, _sgimgui_slot_index(pip.id));
    }
}

_SOKOL_PRIVATE void _sgimgui_uninit_view(sg_view view, void* user_data) {
    _sgimgui_t* ctx = (_sgimgui_t*) user_data;
    SOKOL_ASSERT(ctx);
    _sgimgui_capture_item_t* item = _sgimgui_capture_next_write_item(ctx);
    if (item) {
        item->cmd = _SGIMGUI_CMD_UNINIT_VIEW;
        item->color = _SGIMGUI_COLOR_RSRC;
        item->args.uninit_view.view = view;
    }
    if (ctx->hooks.uninit_view) {
        ctx->hooks.uninit_view(view, ctx->hooks.user_data);
    }
    if (view.id != SG_INVALID_ID) {
        _sgimgui_view_destroyed(ctx, _sgimgui_slot_index(view.id));
    }
}

_SOKOL_PRIVATE void _sgimgui_fail_buffer(sg_buffer buf_id, void* user_data) {
    _sgimgui_t* ctx = (_sgimgui_t*) user_data;
    SOKOL_ASSERT(ctx);
    _sgimgui_capture_item_t* item = _sgimgui_capture_next_write_item(ctx);
    if (item) {
        item->cmd = _SGIMGUI_CMD_FAIL_BUFFER;
        item->color = _SGIMGUI_COLOR_RSRC;
        item->args.fail_buffer.buffer = buf_id;
    }
    if (ctx->hooks.fail_buffer) {
        ctx->hooks.fail_buffer(buf_id, ctx->hooks.user_data);
    }
}

_SOKOL_PRIVATE void _sgimgui_fail_image(sg_image img_id, void* user_data) {
    _sgimgui_t* ctx = (_sgimgui_t*) user_data;
    SOKOL_ASSERT(ctx);
    _sgimgui_capture_item_t* item = _sgimgui_capture_next_write_item(ctx);
    if (item) {
        item->cmd = _SGIMGUI_CMD_FAIL_IMAGE;
        item->color = _SGIMGUI_COLOR_RSRC;
        item->args.fail_image.image = img_id;
    }
    if (ctx->hooks.fail_image) {
        ctx->hooks.fail_image(img_id, ctx->hooks.user_data);
    }
}

_SOKOL_PRIVATE void _sgimgui_fail_sampler(sg_sampler smp_id, void* user_data) {
    _sgimgui_t* ctx = (_sgimgui_t*) user_data;
    SOKOL_ASSERT(ctx);
    _sgimgui_capture_item_t* item = _sgimgui_capture_next_write_item(ctx);
    if (item) {
        item->cmd = _SGIMGUI_CMD_FAIL_SAMPLER;
        item->color = _SGIMGUI_COLOR_RSRC;
        item->args.fail_sampler.sampler = smp_id;
    }
    if (ctx->hooks.fail_sampler) {
        ctx->hooks.fail_sampler(smp_id, ctx->hooks.user_data);
    }
}

_SOKOL_PRIVATE void _sgimgui_fail_shader(sg_shader shd_id, void* user_data) {
    _sgimgui_t* ctx = (_sgimgui_t*) user_data;
    SOKOL_ASSERT(ctx);
    _sgimgui_capture_item_t* item = _sgimgui_capture_next_write_item(ctx);
    if (item) {
        item->cmd = _SGIMGUI_CMD_FAIL_SHADER;
        item->color = _SGIMGUI_COLOR_RSRC;
        item->args.fail_shader.shader = shd_id;
    }
    if (ctx->hooks.fail_shader) {
        ctx->hooks.fail_shader(shd_id, ctx->hooks.user_data);
    }
}

_SOKOL_PRIVATE void _sgimgui_fail_pipeline(sg_pipeline pip_id, void* user_data) {
    _sgimgui_t* ctx = (_sgimgui_t*) user_data;
    SOKOL_ASSERT(ctx);
    _sgimgui_capture_item_t* item = _sgimgui_capture_next_write_item(ctx);
    if (item) {
        item->cmd = _SGIMGUI_CMD_FAIL_PIPELINE;
        item->color = _SGIMGUI_COLOR_RSRC;
        item->args.fail_pipeline.pipeline = pip_id;
    }
    if (ctx->hooks.fail_pipeline) {
        ctx->hooks.fail_pipeline(pip_id, ctx->hooks.user_data);
    }
}

_SOKOL_PRIVATE void _sgimgui_fail_view(sg_view view_id, void* user_data) {
    _sgimgui_t* ctx = (_sgimgui_t*) user_data;
    SOKOL_ASSERT(ctx);
    _sgimgui_capture_item_t* item = _sgimgui_capture_next_write_item(ctx);
    if (item) {
        item->cmd = _SGIMGUI_CMD_FAIL_VIEW;
        item->color = _SGIMGUI_COLOR_RSRC;
        item->args.fail_view.view = view_id;
    }
    if (ctx->hooks.fail_view) {
        ctx->hooks.fail_view(view_id, ctx->hooks.user_data);
    }
}

_SOKOL_PRIVATE void _sgimgui_push_debug_group(const char* name, void* user_data) {
    _sgimgui_t* ctx = (_sgimgui_t*) user_data;
    SOKOL_ASSERT(ctx);
    if (0 == strcmp(name, "sokol-imgui")) {
        ctx->frame_stats_window.in_sokol_imgui = true;
        if (ctx->frame_stats_window.disable_sokol_imgui_stats) {
            sg_disable_stats();
        }
    }
    _sgimgui_capture_item_t* item = _sgimgui_capture_next_write_item(ctx);
    if (item) {
        item->cmd = _SGIMGUI_CMD_PUSH_DEBUG_GROUP;
        item->color = _SGIMGUI_COLOR_OTHER;
        item->args.push_debug_group.name = _sgimgui_make_str(name);
    }
    if (ctx->hooks.push_debug_group) {
        ctx->hooks.push_debug_group(name, ctx->hooks.user_data);
    }
}

_SOKOL_PRIVATE void _sgimgui_pop_debug_group(void* user_data) {
    _sgimgui_t* ctx = (_sgimgui_t*) user_data;
    SOKOL_ASSERT(ctx);
    if (ctx->frame_stats_window.in_sokol_imgui) {
        ctx->frame_stats_window.in_sokol_imgui = false;
        if (ctx->frame_stats_window.disable_sokol_imgui_stats) {
            sg_enable_stats();
        }
    }
    _sgimgui_capture_item_t* item = _sgimgui_capture_next_write_item(ctx);
    if (item) {
        item->cmd = _SGIMGUI_CMD_POP_DEBUG_GROUP;
        item->color = _SGIMGUI_COLOR_OTHER;
    }
    if (ctx->hooks.pop_debug_group) {
        ctx->hooks.pop_debug_group(ctx->hooks.user_data);
    }
}

/*--- IMGUI HELPERS ----------------------------------------------------------*/
_SOKOL_PRIVATE void _sgimgui_draw_image(_sgimgui_t* ctx, sg_image img, float* opt_scale_ptr, float max_width) {
    if (sg_query_image_state(img) != SG_RESOURCESTATE_VALID) {
        _sgimgui_igtext("Image not in valid state.");
        return;
    }
    // try to find a texture view for the image
    sg_view view = {SG_INVALID_ID};
    for (int i = 0; i < ctx->view_window.num_slots; i++) {
        const _sgimgui_view_t* view_ui = &ctx->view_window.slots[i];
        view = view_ui->res_id;
        if (sg_query_view_type(view) == SG_VIEWTYPE_TEXTURE) {
            sg_image view_img = sg_query_view_image(view);
            if (view_img.id == img.id) {
                // FIXME: once texture views can have a separate image type, check this instead
                const bool image_renderable = (sg_query_image_type(view_img) == SG_IMAGETYPE_2D) && (sg_query_image_sample_count(view_img) == 1);
                if (image_renderable) {
                    break;
                }
            }
        }
    }
    if (view.id != SG_INVALID_ID) {
        _sgimgui_igpushidint((int)view.id);
        float scale = 1.0f;
        if (opt_scale_ptr) {
            _sgimgui_igsliderfloatex("Scale", opt_scale_ptr, 0.125f, 8.0f, "%.3f", ImGuiSliderFlags_Logarithmic);
            scale = *opt_scale_ptr;
        }
        float w = (float)sg_query_image_width(img) * scale;
        float h = (float)sg_query_image_height(img) * scale;
        if ((max_width > 1.0f) && (w > max_width)) {
            h *= max_width / w;
            w = max_width;
        }
        _sgimgui_igimage(simgui_imtextureid(view), IMVEC2(w, h));
        _sgimgui_igpopid();
    } else {
        _sgimgui_igtext("Image has no renderable texture view.", img.id);
    }
}

_SOKOL_PRIVATE bool _sgimgui_draw_resid_list_item(uint32_t res_id, const char* label, bool selected) {
    _sgimgui_igpushidint((int)res_id);
    bool res;
    if (label[0]) {
        res = _sgimgui_igselectableex(label, selected, 0, IMVEC2(0,0));
    } else {
        _sgimgui_str_t str;
        _sgimgui_snprintf(&str, "0x%08X", res_id);
        res = _sgimgui_igselectableex(str.buf, selected, 0, IMVEC2(0,0));
    }
    _sgimgui_igpopid();
    return res;
}

_SOKOL_PRIVATE bool _sgimgui_draw_resid_link(uint32_t res_type, uint32_t res_id, const char* label) {
    SOKOL_ASSERT(label);
    _sgimgui_str_t str_buf;
    const char* str;
    if (label[0]) {
        str = label;
    } else {
        _sgimgui_snprintf(&str_buf, "0x%08X", res_id);
        str = str_buf.buf;
    }
    _sgimgui_igpushidint((int)((res_type<<24)|res_id));
    bool res = _sgimgui_igsmallbutton(str);
    _sgimgui_igpopid();
    return res;
}

_SOKOL_PRIVATE bool _sgimgui_draw_buffer_link(_sgimgui_t* ctx, sg_buffer buf) {
    bool retval = false;
    if (buf.id != SG_INVALID_ID) {
        const _sgimgui_buffer_t* buf_ui = &ctx->buffer_window.slots[_sgimgui_slot_index(buf.id)];
        retval = _sgimgui_draw_resid_link(1, buf.id, buf_ui->label.buf);
    }
    return retval;
}

_SOKOL_PRIVATE bool _sgimgui_draw_image_link(_sgimgui_t* ctx, sg_image img) {
    bool retval = false;
    if (img.id != SG_INVALID_ID) {
        const _sgimgui_image_t* img_ui = &ctx->image_window.slots[_sgimgui_slot_index(img.id)];
        retval = _sgimgui_draw_resid_link(2, img.id, img_ui->label.buf);
    }
    return retval;
}

_SOKOL_PRIVATE bool _sgimgui_draw_sampler_link(_sgimgui_t* ctx, sg_sampler smp) {
    bool retval = false;
    if (smp.id != SG_INVALID_ID) {
        const _sgimgui_sampler_t* smp_ui = &ctx->sampler_window.slots[_sgimgui_slot_index(smp.id)];
        retval = _sgimgui_draw_resid_link(3, smp.id, smp_ui->label.buf);
    }
    return retval;
}

_SOKOL_PRIVATE bool _sgimgui_draw_shader_link(_sgimgui_t* ctx, sg_shader shd) {
    bool retval = false;
    if (shd.id != SG_INVALID_ID) {
        const _sgimgui_shader_t* shd_ui = &ctx->shader_window.slots[_sgimgui_slot_index(shd.id)];
        retval = _sgimgui_draw_resid_link(4, shd.id, shd_ui->label.buf);
    }
    return retval;
}

_SOKOL_PRIVATE bool _sgimgui_draw_view_link(_sgimgui_t* ctx, sg_view view) {
    bool retval = false;
    if (view.id != SG_INVALID_ID) {
        const _sgimgui_view_t* view_ui = &ctx->view_window.slots[_sgimgui_slot_index(view.id)];
        retval = _sgimgui_draw_resid_link(5, view.id, view_ui->label.buf);
        if (_sgimgui_igisitemhovered(0)) {
            sg_image img = sg_query_view_image(view);
            if (img.id != SG_INVALID_ID) {
                if (_sgimgui_igbegintooltip()) {
                    _sgimgui_draw_image(ctx, img, 0, 128.0f);
                    _sgimgui_igendtooltip();
                }
            }
        }
    }
    return retval;
}

_SOKOL_PRIVATE void _sgimgui_show_buffer(_sgimgui_t* ctx, sg_buffer buf) {
    ctx->buffer_window.open = true;
    ctx->buffer_window.sel_buf = buf;
}

_SOKOL_PRIVATE void _sgimgui_show_image(_sgimgui_t* ctx, sg_image img) {
    ctx->image_window.open = true;
    ctx->image_window.sel_img = img;
}

_SOKOL_PRIVATE void _sgimgui_show_sampler(_sgimgui_t* ctx, sg_sampler smp) {
    ctx->sampler_window.open = true;
    ctx->sampler_window.sel_smp = smp;
}

_SOKOL_PRIVATE void _sgimgui_show_shader(_sgimgui_t* ctx, sg_shader shd) {
    ctx->shader_window.open = true;
    ctx->shader_window.sel_shd = shd;
}

_SOKOL_PRIVATE void _sgimgui_show_view(_sgimgui_t* ctx, sg_view view) {
    ctx->view_window.open = true;
    ctx->view_window.sel_view = view;
}

_SOKOL_PRIVATE void _sgimgui_draw_buffer_list(_sgimgui_t* ctx) {
    _sgimgui_igbeginchild("buffer_list", IMVEC2(_SGIMGUI_LIST_WIDTH,0), true, 0);
    for (int i = 0; i < ctx->buffer_window.num_slots; i++) {
        sg_buffer buf = ctx->buffer_window.slots[i].res_id;
        sg_resource_state state = sg_query_buffer_state(buf);
        if ((state != SG_RESOURCESTATE_INVALID) && (state != SG_RESOURCESTATE_INITIAL)) {
            bool selected = ctx->buffer_window.sel_buf.id == buf.id;
            if (_sgimgui_draw_resid_list_item(buf.id, ctx->buffer_window.slots[i].label.buf, selected)) {
                ctx->buffer_window.sel_buf.id = buf.id;
            }
        }
    }
    _sgimgui_igendchild();
}

_SOKOL_PRIVATE void _sgimgui_draw_image_list(_sgimgui_t* ctx) {
    _sgimgui_igbeginchild("image_list", IMVEC2(_SGIMGUI_LIST_WIDTH,0), true, 0);
    for (int i = 0; i < ctx->image_window.num_slots; i++) {
        sg_image img = ctx->image_window.slots[i].res_id;
        sg_resource_state state = sg_query_image_state(img);
        if ((state != SG_RESOURCESTATE_INVALID) && (state != SG_RESOURCESTATE_INITIAL)) {
            bool selected = ctx->image_window.sel_img.id == img.id;
            if (_sgimgui_draw_resid_list_item(img.id, ctx->image_window.slots[i].label.buf, selected)) {
                ctx->image_window.sel_img.id = img.id;
            }
        }
    }
    _sgimgui_igendchild();
}

_SOKOL_PRIVATE void _sgimgui_draw_sampler_list(_sgimgui_t* ctx) {
    _sgimgui_igbeginchild("sampler_list", IMVEC2(_SGIMGUI_LIST_WIDTH,0), true, 0);
    for (int i = 0; i < ctx->sampler_window.num_slots; i++) {
        sg_sampler smp = ctx->sampler_window.slots[i].res_id;
        sg_resource_state state = sg_query_sampler_state(smp);
        if ((state != SG_RESOURCESTATE_INVALID) && (state != SG_RESOURCESTATE_INITIAL)) {
            bool selected = ctx->sampler_window.sel_smp.id == smp.id;
            if (_sgimgui_draw_resid_list_item(smp.id, ctx->sampler_window.slots[i].label.buf, selected)) {
                ctx->sampler_window.sel_smp.id = smp.id;
            }
        }
    }
    _sgimgui_igendchild();
}

_SOKOL_PRIVATE void _sgimgui_draw_shader_list(_sgimgui_t* ctx) {
    _sgimgui_igbeginchild("shader_list", IMVEC2(_SGIMGUI_LIST_WIDTH,0), true, 0);
    for (int i = 0; i < ctx->shader_window.num_slots; i++) {
        sg_shader shd = ctx->shader_window.slots[i].res_id;
        sg_resource_state state = sg_query_shader_state(shd);
        if ((state != SG_RESOURCESTATE_INVALID) && (state != SG_RESOURCESTATE_INITIAL)) {
            bool selected = ctx->shader_window.sel_shd.id == shd.id;
            if (_sgimgui_draw_resid_list_item(shd.id, ctx->shader_window.slots[i].label.buf, selected)) {
                ctx->shader_window.sel_shd.id = shd.id;
            }
        }
    }
    _sgimgui_igendchild();
}

_SOKOL_PRIVATE void _sgimgui_draw_pipeline_list(_sgimgui_t* ctx) {
    _sgimgui_igbeginchild("pipeline_list", IMVEC2(_SGIMGUI_LIST_WIDTH,0), true, 0);
    for (int i = 1; i < ctx->pipeline_window.num_slots; i++) {
        sg_pipeline pip = ctx->pipeline_window.slots[i].res_id;
        sg_resource_state state = sg_query_pipeline_state(pip);
        if ((state != SG_RESOURCESTATE_INVALID) && (state != SG_RESOURCESTATE_INITIAL)) {
            bool selected = ctx->pipeline_window.sel_pip.id == pip.id;
            if (_sgimgui_draw_resid_list_item(pip.id, ctx->pipeline_window.slots[i].label.buf, selected)) {
                ctx->pipeline_window.sel_pip.id = pip.id;
            }
        }
    }
    _sgimgui_igendchild();
}

_SOKOL_PRIVATE void _sgimgui_draw_view_list(_sgimgui_t* ctx) {
    _sgimgui_igbeginchild("view_list", IMVEC2(_SGIMGUI_LIST_WIDTH,0), true, 0);
    for (int i = 1; i < ctx->view_window.num_slots; i++) {
        sg_view view = ctx->view_window.slots[i].res_id;
        sg_resource_state state = sg_query_view_state(view);
        if ((state != SG_RESOURCESTATE_INVALID) && (state != SG_RESOURCESTATE_INITIAL)) {
            bool selected = ctx->view_window.sel_view.id == view.id;
            if (_sgimgui_draw_resid_list_item(view.id, ctx->view_window.slots[i].label.buf, selected)) {
                ctx->view_window.sel_view.id = view.id;
            }
        }
    }
    _sgimgui_igendchild();
}

_SOKOL_PRIVATE void _sgimgui_draw_capture_list(_sgimgui_t* ctx) {
    _sgimgui_igbeginchild("capture_list", IMVEC2(_SGIMGUI_LIST_WIDTH,0), true, 0);
    const int num_items = _sgimgui_capture_num_read_items(ctx);
    uint64_t group_stack = 1;   /* bit set: group unfolded, cleared: folded */
    for (int i = 0; i < num_items; i++) {
        const _sgimgui_capture_item_t* item = _sgimgui_capture_read_item_at(ctx, i);
        _sgimgui_str_t item_string = _sgimgui_capture_item_string(ctx, i, item);
        _sgimgui_igpushstylecolor(ImGuiCol_Text, item->color);
        _sgimgui_igpushidint(i);
        if (item->cmd == _SGIMGUI_CMD_PUSH_DEBUG_GROUP) {
            if (group_stack & 1) {
                group_stack <<= 1;
                const char* group_name = item->args.push_debug_group.name.buf;
                if (_sgimgui_igtreenodestr(group_name, "Group: %s", group_name)) {
                    group_stack |= 1;
                }
            } else {
                group_stack <<= 1;
            }
        } else if (item->cmd == _SGIMGUI_CMD_POP_DEBUG_GROUP) {
            if (group_stack & 1) {
                _sgimgui_igtreepop();
            }
            group_stack >>= 1;
        } else if (group_stack & 1) {
            if (_sgimgui_igselectableex(item_string.buf, ctx->capture_window.sel_item == i, 0, IMVEC2(0,0))) {
                ctx->capture_window.sel_item = i;
            }
            if (_sgimgui_igisitemhovered(0)) {
                _sgimgui_igsettooltip("%s", item_string.buf);
            }
        }
        _sgimgui_igpopid();
        _sgimgui_igpopstylecolor();
    }
    _sgimgui_igendchild();
}

_SOKOL_PRIVATE void _sgimgui_draw_buffer_panel(_sgimgui_t* ctx, sg_buffer buf) {
    if (buf.id != SG_INVALID_ID) {
        _sgimgui_igbeginchild("buffer", IMVEC2(0,0), false, 0);
        sg_buffer_info info = sg_query_buffer_info(buf);
        if (info.slot.state == SG_RESOURCESTATE_VALID) {
            const _sgimgui_buffer_t* buf_ui = &ctx->buffer_window.slots[_sgimgui_slot_index(buf.id)];
            _sgimgui_igtext("Label: %s", buf_ui->label.buf[0] ? buf_ui->label.buf : "---");
            _sgimgui_draw_resource_slot(&info.slot);
            _sgimgui_igseparator();
            _sgimgui_igtext("Usage:\n");
            _sgimgui_igtext("  vertex_buffer: %s", _sgimgui_bool_string(buf_ui->desc.usage.vertex_buffer));
            _sgimgui_igtext("  index_buffer: %s", _sgimgui_bool_string(buf_ui->desc.usage.index_buffer));
            _sgimgui_igtext("  storage_buffer: %s", _sgimgui_bool_string(buf_ui->desc.usage.storage_buffer));
            _sgimgui_igtext("  immutable: %s", _sgimgui_bool_string(buf_ui->desc.usage.immutable));
            _sgimgui_igtext("  dynamic_update: %s", _sgimgui_bool_string(buf_ui->desc.usage.dynamic_update));
            _sgimgui_igtext("  stream_update: %s", _sgimgui_bool_string(buf_ui->desc.usage.stream_update));
            _sgimgui_igtext("Size:  %d", (int)buf_ui->desc.size);
            if (!buf_ui->desc.usage.immutable) {
                _sgimgui_igseparator();
                _sgimgui_igtext("Num Slots:     %d", info.num_slots);
                _sgimgui_igtext("Active Slot:   %d", info.active_slot);
                _sgimgui_igtext("Update Frame Index: %d", info.update_frame_index);
                _sgimgui_igtext("Append Frame Index: %d", info.append_frame_index);
                _sgimgui_igtext("Append Pos:         %d", info.append_pos);
                _sgimgui_igtext("Append Overflow:    %s", _sgimgui_bool_string(info.append_overflow));
            }
        } else {
            _sgimgui_igtext("Buffer 0x%08X not valid.", buf.id);
        }
        _sgimgui_igendchild();
    }
}

_SOKOL_PRIVATE void _sgimgui_draw_image_panel(_sgimgui_t* ctx, sg_image img) {
    if (img.id != SG_INVALID_ID) {
        _sgimgui_igbeginchild("image", IMVEC2(0,0), false, 0);
        sg_image_info info = sg_query_image_info(img);
        if (info.slot.state == SG_RESOURCESTATE_VALID) {
            _sgimgui_image_t* img_ui = &ctx->image_window.slots[_sgimgui_slot_index(img.id)];
            const sg_image_desc* desc = &img_ui->desc;
            _sgimgui_igtext("Label: %s", img_ui->label.buf[0] ? img_ui->label.buf : "---");
            _sgimgui_draw_resource_slot(&info.slot);
            _sgimgui_igseparator();
            _sgimgui_draw_image(ctx, img, &img_ui->ui_scale, 4096.0f);
            _sgimgui_igseparator();
            _sgimgui_igtext("Type:           %s", _sgimgui_imagetype_string(desc->type));
            _sgimgui_igtext("Usage:\n");
            _sgimgui_igtext("  storage_image: %s", _sgimgui_bool_string(desc->usage.storage_image));
            _sgimgui_igtext("  color_attachment: %s", _sgimgui_bool_string(desc->usage.color_attachment));
            _sgimgui_igtext("  resolve_attachment: %s", _sgimgui_bool_string(desc->usage.resolve_attachment));
            _sgimgui_igtext("  depth_stencil_attachment: %s", _sgimgui_bool_string(desc->usage.depth_stencil_attachment));
            _sgimgui_igtext("  immutable: %s", _sgimgui_bool_string(desc->usage.immutable));
            _sgimgui_igtext("  dynamic_update: %s", _sgimgui_bool_string(desc->usage.dynamic_update));
            _sgimgui_igtext("  stream_update: %s", _sgimgui_bool_string(desc->usage.stream_update));
            _sgimgui_igtext("Width:          %d", desc->width);
            _sgimgui_igtext("Height:         %d", desc->height);
            _sgimgui_igtext("Num Slices:     %d", desc->num_slices);
            _sgimgui_igtext("Num Mipmaps:    %d", desc->num_mipmaps);
            _sgimgui_igtext("Pixel Format:   %s", _sgimgui_pixelformat_string(desc->pixel_format));
            _sgimgui_igtext("Sample Count:   %d", desc->sample_count);
            if (!desc->usage.immutable) {
                _sgimgui_igseparator();
                _sgimgui_igtext("Num Slots:     %d", info.num_slots);
                _sgimgui_igtext("Active Slot:   %d", info.active_slot);
                _sgimgui_igtext("Update Frame Index: %d", info.upd_frame_index);
            }
        } else {
            _sgimgui_igtext("Image 0x%08X not valid.", img.id);
        }
        _sgimgui_igendchild();
    }
}

_SOKOL_PRIVATE void _sgimgui_draw_sampler_panel(_sgimgui_t* ctx, sg_sampler smp) {
    if (smp.id != SG_INVALID_ID) {
        _sgimgui_igbeginchild("sampler", IMVEC2(0,0), false, 0);
        sg_sampler_info info = sg_query_sampler_info(smp);
        if (info.slot.state == SG_RESOURCESTATE_VALID) {
            _sgimgui_sampler_t* smp_ui = &ctx->sampler_window.slots[_sgimgui_slot_index(smp.id)];
            const sg_sampler_desc* desc = &smp_ui->desc;
            _sgimgui_igtext("Label: %s", smp_ui->label.buf[0] ? smp_ui->label.buf : "---");
            _sgimgui_draw_resource_slot(&info.slot);
            _sgimgui_igseparator();
            _sgimgui_igtext("Min Filter:     %s", _sgimgui_filter_string(desc->min_filter));
            _sgimgui_igtext("Mag Filter:     %s", _sgimgui_filter_string(desc->mag_filter));
            _sgimgui_igtext("Mipmap Filter:  %s", _sgimgui_filter_string(desc->mipmap_filter));
            _sgimgui_igtext("Wrap U:         %s", _sgimgui_wrap_string(desc->wrap_u));
            _sgimgui_igtext("Wrap V:         %s", _sgimgui_wrap_string(desc->wrap_v));
            _sgimgui_igtext("Wrap W:         %s", _sgimgui_wrap_string(desc->wrap_w));
            _sgimgui_igtext("Min LOD:        %.3f", desc->min_lod);
            _sgimgui_igtext("Max LOD:        %.3f", desc->max_lod);
            _sgimgui_igtext("Border Color:   %s", _sgimgui_bordercolor_string(desc->border_color));
            _sgimgui_igtext("Compare:        %s", _sgimgui_comparefunc_string(desc->compare));
            _sgimgui_igtext("Max Anisotropy: %d", desc->max_anisotropy);
        } else {
            _sgimgui_igtext("Sampler 0x%08X not valid.", smp.id);
        }
        _sgimgui_igendchild();
    }
}

_SOKOL_PRIVATE void _sgimgui_draw_shader_func(const char* title, const sg_shader_function* func) {
    SOKOL_ASSERT(func);
    if ((func->source == 0) && (func->bytecode.ptr == 0)) {
        return;
    }
    _sgimgui_igpushid(title);
    _sgimgui_igtext("%s", title);
    if (func->entry) {
        _sgimgui_igtext("  entry: %s", func->entry);
    }
    if (func->d3d11_target) {
        _sgimgui_igtext("  d3d11_target: %s", func->d3d11_target);
    }
    if (func->source) {
        if (_sgimgui_igtreenode("source:")) {
            _sgimgui_igtext("%s", func->source);
            _sgimgui_igtreepop();
        }
    } else if (func->bytecode.ptr) {
        if (_sgimgui_igtreenode("bytecode")) {
            _sgimgui_igtext("Byte-code display currently not supported.");
            _sgimgui_igtreepop();
        }
    }
    _sgimgui_igpopid();
}

_SOKOL_PRIVATE void _sgimgui_draw_shader_panel(_sgimgui_t* ctx, sg_shader shd) {
    if (shd.id != SG_INVALID_ID) {
        _sgimgui_igbeginchild("shader", IMVEC2(0,0), false, ImGuiWindowFlags_HorizontalScrollbar);
        sg_shader_info info = sg_query_shader_info(shd);
        if (info.slot.state == SG_RESOURCESTATE_VALID) {
            const _sgimgui_shader_t* shd_ui = &ctx->shader_window.slots[_sgimgui_slot_index(shd.id)];
            _sgimgui_igtext("Label: %s", shd_ui->label.buf[0] ? shd_ui->label.buf : "---");
            _sgimgui_draw_resource_slot(&info.slot);
            _sgimgui_igseparator();
            if (_sgimgui_igtreenode("Attrs")) {
                for (int i = 0; i < SG_MAX_VERTEX_ATTRIBUTES; i++) {
                    const sg_shader_vertex_attr* a_desc = &shd_ui->desc.attrs[i];
                    if ((a_desc->base_type != SG_SHADERATTRBASETYPE_UNDEFINED) || a_desc->glsl_name || a_desc->hlsl_sem_name) {
                        _sgimgui_igtext("#%d:", i);
                        if (a_desc->base_type != SG_SHADERATTRBASETYPE_UNDEFINED) {
                            _sgimgui_igtext("  Base Type: %s", _sgimgui_shaderattrbasetype_string(a_desc->base_type));
                        }
                        if (a_desc->glsl_name) {
                            _sgimgui_igtext("  GLSL Name: %s", a_desc->glsl_name);
                        }
                        if (a_desc->hlsl_sem_name) {
                            _sgimgui_igtext("  HLSL Sem Name:  %s", a_desc->hlsl_sem_name);
                            _sgimgui_igtext("  HLSL Sem Index: %d", a_desc->hlsl_sem_index);
                        }
                    }
                }
                _sgimgui_igtreepop();
            }
            int num_valid_ubs = 0;
            for (int i = 0; i < SG_MAX_UNIFORMBLOCK_BINDSLOTS; i++) {
                const sg_shader_uniform_block* ub = &shd_ui->desc.uniform_blocks[i];
                if (ub->stage != SG_SHADERSTAGE_NONE) {
                    num_valid_ubs++;
                }
            }
            int num_valid_views = 0;
            for (int i = 0; i < SG_MAX_VIEW_BINDSLOTS; i++) {
                const sg_shader_view* view = &shd_ui->desc.views[i];
                if ((view->texture.stage != SG_SHADERSTAGE_NONE) ||
                    (view->storage_buffer.stage != SG_SHADERSTAGE_NONE) ||
                    (view->storage_image.stage != SG_SHADERSTAGE_NONE))
                {
                    num_valid_views++;
                }
            }
            int num_valid_samplers = 0;
            for (int i = 0; i < SG_MAX_SAMPLER_BINDSLOTS; i++) {
                if (shd_ui->desc.samplers[i].stage != SG_SHADERSTAGE_NONE) {
                    num_valid_samplers++;
                }
            }
            int num_valid_texture_sampler_pairs = 0;
            for (int i = 0; i < SG_MAX_TEXTURE_SAMPLER_PAIRS; i++) {
                if (shd_ui->desc.texture_sampler_pairs[i].stage != SG_SHADERSTAGE_NONE) {
                    num_valid_texture_sampler_pairs++;
                }
            }
            if (num_valid_ubs > 0) {
                if (_sgimgui_igtreenode("Uniform Blocks")) {
                    for (int i = 0; i < SG_MAX_UNIFORMBLOCK_BINDSLOTS; i++) {
                        const sg_shader_uniform_block* ub = &shd_ui->desc.uniform_blocks[i];
                        if (ub->stage == SG_SHADERSTAGE_NONE) {
                            continue;
                        }
                        _sgimgui_igtext("- slot: %d", i);
                        _sgimgui_igtext("  stage: %s", _sgimgui_shaderstage_string(ub->stage));
                        _sgimgui_igtext("  size: %d", ub->size);
                        _sgimgui_igtext("  layout: %s", _sgimgui_uniformlayout_string(ub->layout));
                        _sgimgui_igtext("  hlsl_register_b_n: %d", ub->hlsl_register_b_n);
                        _sgimgui_igtext("  msl_buffer_n: %d", ub->msl_buffer_n);
                        _sgimgui_igtext("  wgsl_group0_binding_n: %d", ub->wgsl_group0_binding_n);
                        _sgimgui_igtext("  spirv_set0_binding_n: %d", ub->spirv_set0_binding_n);
                        _sgimgui_igtext("  glsl_uniforms:");
                        for (int j = 0; j < SG_MAX_UNIFORMBLOCK_MEMBERS; j++) {
                            const sg_glsl_shader_uniform* u = &ub->glsl_uniforms[j];
                            if (SG_UNIFORMTYPE_INVALID != u->type) {
                                if (u->array_count <= 1) {
                                    _sgimgui_igtext("    %s %s", _sgimgui_uniformtype_string(u->type), u->glsl_name ? u->glsl_name : "");
                                } else {
                                    _sgimgui_igtext("    %s[%d] %s", _sgimgui_uniformtype_string(u->type), u->array_count, u->glsl_name ? u->glsl_name : "");
                                }
                            }
                        }
                    }
                    _sgimgui_igtreepop();
                }
            }
            if (num_valid_views > 0) {
                if (_sgimgui_igtreenode("Views")) {
                    for (int i = 0; i < SG_MAX_VIEW_BINDSLOTS; i++) {
                        const sg_shader_view* view = &shd_ui->desc.views[i];
                        if (view->texture.stage != SG_SHADERSTAGE_NONE) {
                            const sg_shader_texture_view* tex = &view->texture;
                            _sgimgui_igtext("- slot: %d", i);
                            _sgimgui_igtext("  stage: %s", _sgimgui_shaderstage_string(tex->stage));
                            _sgimgui_igtext("  type: SG_VIEWTYPE_TEXTURE");
                            _sgimgui_igtext("  image_type: %s", _sgimgui_imagetype_string(tex->image_type));
                            _sgimgui_igtext("  sample_type: %s", _sgimgui_imagesampletype_string(tex->sample_type));
                            _sgimgui_igtext("  multisampled: %s", _sgimgui_bool_string(tex->multisampled));
                            _sgimgui_igtext("  hlsl_register_t_n: %d", tex->hlsl_register_t_n);
                            _sgimgui_igtext("  msl_texture_n: %d", tex->msl_texture_n);
                            _sgimgui_igtext("  wgsl_group1_binding_n: %d", tex->wgsl_group1_binding_n);
                            _sgimgui_igtext("  spirv_set1_binding_n: %d", tex->spirv_set1_binding_n);
                        } else if (view->storage_buffer.stage != SG_SHADERSTAGE_NONE) {
                            const sg_shader_storage_buffer_view* sbuf = &view->storage_buffer;
                            _sgimgui_igtext("- slot: %d", i);
                            _sgimgui_igtext("  stage: %s", _sgimgui_shaderstage_string(sbuf->stage));
                            _sgimgui_igtext("  type: SG_VIEWTYPE_STORAGEBUFFER");
                            _sgimgui_igtext("  readonly: %s", _sgimgui_bool_string(sbuf->readonly));
                            if (sbuf->readonly) {
                                _sgimgui_igtext("  hlsl_register_t_n: %d", sbuf->hlsl_register_t_n);
                            } else {
                                _sgimgui_igtext("  hlsl_register_u_n: %d", sbuf->hlsl_register_u_n);
                            }
                            _sgimgui_igtext("  msl_buffer_n: %d", sbuf->msl_buffer_n);
                            _sgimgui_igtext("  wgsl_group1_binding_n: %d", sbuf->wgsl_group1_binding_n);
                            _sgimgui_igtext("  spirv_group1_binding_n: %d\n", sbuf->spirv_set1_binding_n);
                            _sgimgui_igtext("  glsl_binding_n: %d", sbuf->glsl_binding_n);
                        } else if (view->storage_image.stage != SG_SHADERSTAGE_NONE) {
                            const sg_shader_storage_image_view* simg = &view->storage_image;
                            _sgimgui_igtext("- slot: %d", i);
                            _sgimgui_igtext("  stage: %s", _sgimgui_shaderstage_string(simg->stage));
                            _sgimgui_igtext("  type: SG_VIEWTYPE_STORAGEIMAGE");
                            _sgimgui_igtext("  image_type: %s", _sgimgui_imagetype_string(simg->image_type));
                            _sgimgui_igtext("  access_format: %s", _sgimgui_pixelformat_string(simg->access_format));
                            _sgimgui_igtext("  writeonly: %s", _sgimgui_bool_string(simg->writeonly));
                            _sgimgui_igtext("  hlsl_register_u_n: %d", simg->hlsl_register_u_n);
                            _sgimgui_igtext("  msl_texture_n: %d", simg->msl_texture_n);
                            _sgimgui_igtext("  wgsl_group2_binding_n: %d", simg->wgsl_group1_binding_n);
                            _sgimgui_igtext("  spirv_set1_binding_n: %d", simg->spirv_set1_binding_n);
                            _sgimgui_igtext("  glsl_binding_n: %d", simg->glsl_binding_n);
                        }
                    }
                    _sgimgui_igtreepop();
                }
            }
            if (num_valid_samplers > 0) {
                if (_sgimgui_igtreenode("Samplers")) {
                    for (int i = 0; i < SG_MAX_SAMPLER_BINDSLOTS; i++) {
                        const sg_shader_sampler* ssd = &shd_ui->desc.samplers[i];
                        if (ssd->stage == SG_SHADERSTAGE_NONE) {
                            continue;
                        }
                        _sgimgui_igtext("- slot: %d", i);
                        _sgimgui_igtext("  stage: %s", _sgimgui_shaderstage_string(ssd->stage));
                        _sgimgui_igtext("  sampler_type: %s", _sgimgui_samplertype_string(ssd->sampler_type));
                        _sgimgui_igtext("  hlsl_register_s_n: %d", ssd->hlsl_register_s_n);
                        _sgimgui_igtext("  msl_sampler_n: %d", ssd->msl_sampler_n);
                        _sgimgui_igtext("  wgsl_group1_binding_n: %d", ssd->wgsl_group1_binding_n);
                        _sgimgui_igtext("  spirv_set1_binding_1: %d", ssd->spirv_set1_binding_n);
                    }
                    _sgimgui_igtreepop();
                }
            }
            if (num_valid_texture_sampler_pairs > 0) {
                if (_sgimgui_igtreenode("Texture Sampler Pairs")) {
                    for (int i = 0; i < SG_MAX_TEXTURE_SAMPLER_PAIRS; i++) {
                        const sg_shader_texture_sampler_pair* stspd = &shd_ui->desc.texture_sampler_pairs[i];
                        if (stspd->stage == SG_SHADERSTAGE_NONE) {
                            continue;
                        }
                        _sgimgui_igtext("- slot: %d", i);
                        _sgimgui_igtext("  stage: %s", _sgimgui_shaderstage_string(stspd->stage));
                        _sgimgui_igtext("  view_slot: %d", stspd->view_slot);
                        _sgimgui_igtext("  sampler_slot: %d", stspd->sampler_slot);
                        _sgimgui_igtext("  glsl_name: %s", stspd->glsl_name ? stspd->glsl_name : "---");
                    }
                    _sgimgui_igtreepop();
                }
            }
            _sgimgui_draw_shader_func("Vertex Function", &shd_ui->desc.vertex_func);
            _sgimgui_draw_shader_func("Fragment Function", &shd_ui->desc.fragment_func);
            _sgimgui_draw_shader_func("Compute Function", &shd_ui->desc.compute_func);
        } else {
            _sgimgui_igtext("Shader 0x%08X not valid!", shd.id);
        }
        _sgimgui_igendchild();
    }
}

_SOKOL_PRIVATE void _sgimgui_draw_vertex_layout_state(const sg_vertex_layout_state* layout) {
    if (_sgimgui_igtreenode("Buffers")) {
        for (int i = 0; i < SG_MAX_VERTEXBUFFER_BINDSLOTS; i++) {
            const sg_vertex_buffer_layout_state* l_state = &layout->buffers[i];
            if (l_state->stride > 0) {
                _sgimgui_igtext("#%d:", i);
                _sgimgui_igtext("  Stride:    %d", l_state->stride);
                _sgimgui_igtext("  Step Func: %s", _sgimgui_vertexstep_string(l_state->step_func));
                _sgimgui_igtext("  Step Rate: %d", l_state->step_rate);
            }
        }
        _sgimgui_igtreepop();
    }
    if (_sgimgui_igtreenode("Attrs")) {
        for (int i = 0; i < SG_MAX_VERTEX_ATTRIBUTES; i++) {
            const sg_vertex_attr_state* a_state = &layout->attrs[i];
            if (a_state->format != SG_VERTEXFORMAT_INVALID) {
                _sgimgui_igtext("#%d:", i);
                _sgimgui_igtext("  Format:       %s", _sgimgui_vertexformat_string(a_state->format));
                _sgimgui_igtext("  Offset:       %d", a_state->offset);
                _sgimgui_igtext("  Buffer Index: %d", a_state->buffer_index);
            }
        }
        _sgimgui_igtreepop();
    }
}

_SOKOL_PRIVATE void _sgimgui_draw_stencil_face_state(const sg_stencil_face_state* sfs) {
    _sgimgui_igtext("Fail Op:       %s", _sgimgui_stencilop_string(sfs->fail_op));
    _sgimgui_igtext("Depth Fail Op: %s", _sgimgui_stencilop_string(sfs->depth_fail_op));
    _sgimgui_igtext("Pass Op:       %s", _sgimgui_stencilop_string(sfs->pass_op));
    _sgimgui_igtext("Compare:       %s", _sgimgui_comparefunc_string(sfs->compare));
}

_SOKOL_PRIVATE void _sgimgui_draw_stencil_state(const sg_stencil_state* ss) {
    _sgimgui_igtext("Enabled:    %s", _sgimgui_bool_string(ss->enabled));
    _sgimgui_igtext("Read Mask:  0x%02X", ss->read_mask);
    _sgimgui_igtext("Write Mask: 0x%02X", ss->write_mask);
    _sgimgui_igtext("Ref:        0x%02X", ss->ref);
    if (_sgimgui_igtreenode("Front")) {
        _sgimgui_draw_stencil_face_state(&ss->front);
        _sgimgui_igtreepop();
    }
    if (_sgimgui_igtreenode("Back")) {
        _sgimgui_draw_stencil_face_state(&ss->back);
        _sgimgui_igtreepop();
    }
}

_SOKOL_PRIVATE void _sgimgui_draw_depth_state(const sg_depth_state* ds) {
    _sgimgui_igtext("Pixel Format:  %s", _sgimgui_pixelformat_string(ds->pixel_format));
    _sgimgui_igtext("Compare:       %s", _sgimgui_comparefunc_string(ds->compare));
    _sgimgui_igtext("Write Enabled: %s", _sgimgui_bool_string(ds->write_enabled));
    _sgimgui_igtext("Bias:          %f", ds->bias);
    _sgimgui_igtext("Bias Slope:    %f", ds->bias_slope_scale);
    _sgimgui_igtext("Bias Clamp:    %f", ds->bias_clamp);
}

_SOKOL_PRIVATE void _sgimgui_draw_blend_state(const sg_blend_state* bs) {
    _sgimgui_igtext("Blend Enabled:    %s", _sgimgui_bool_string(bs->enabled));
    _sgimgui_igtext("Src Factor RGB:   %s", _sgimgui_blendfactor_string(bs->src_factor_rgb));
    _sgimgui_igtext("Dst Factor RGB:   %s", _sgimgui_blendfactor_string(bs->dst_factor_rgb));
    _sgimgui_igtext("Op RGB:           %s", _sgimgui_blendop_string(bs->op_rgb));
    _sgimgui_igtext("Src Factor Alpha: %s", _sgimgui_blendfactor_string(bs->src_factor_alpha));
    _sgimgui_igtext("Dst Factor Alpha: %s", _sgimgui_blendfactor_string(bs->dst_factor_alpha));
    _sgimgui_igtext("Op Alpha:         %s", _sgimgui_blendop_string(bs->op_alpha));
}

_SOKOL_PRIVATE void _sgimgui_draw_color_target_state(const sg_color_target_state* cs) {
    _sgimgui_igtext("Pixel Format:     %s", _sgimgui_pixelformat_string(cs->pixel_format));
    _sgimgui_igtext("Write Mask:       %s", _sgimgui_colormask_string(cs->write_mask));
    if (_sgimgui_igtreenode("Blend State:")) {
        _sgimgui_draw_blend_state(&cs->blend);
        _sgimgui_igtreepop();
    }
}

_SOKOL_PRIVATE void _sgimgui_draw_pipeline_panel(_sgimgui_t* ctx, sg_pipeline pip) {
    if (pip.id != SG_INVALID_ID) {
        _sgimgui_igbeginchild("pipeline", IMVEC2(0,0), false, 0);
        sg_pipeline_info info = sg_query_pipeline_info(pip);
        if (info.slot.state == SG_RESOURCESTATE_VALID) {
            const _sgimgui_pipeline_t* pip_ui = &ctx->pipeline_window.slots[_sgimgui_slot_index(pip.id)];
            _sgimgui_igtext("Label: %s", pip_ui->label.buf[0] ? pip_ui->label.buf : "---");
            _sgimgui_draw_resource_slot(&info.slot);
            _sgimgui_igseparator();
            _sgimgui_igtext("Compute: %s", _sgimgui_bool_string(pip_ui->desc.compute));
            _sgimgui_igtext("Shader: "); _sgimgui_igsameline();
            if (_sgimgui_draw_shader_link(ctx, pip_ui->desc.shader)) {
                _sgimgui_show_shader(ctx, pip_ui->desc.shader);
            }
            if (!pip_ui->desc.compute) {
                if (_sgimgui_igtreenode("Vertex Layout State")) {
                    _sgimgui_draw_vertex_layout_state(&pip_ui->desc.layout);
                    _sgimgui_igtreepop();
                }
                if (_sgimgui_igtreenode("Depth State")) {
                    _sgimgui_draw_depth_state(&pip_ui->desc.depth);
                    _sgimgui_igtreepop();
                }
                if (_sgimgui_igtreenode("Stencil State")) {
                    _sgimgui_draw_stencil_state(&pip_ui->desc.stencil);
                    _sgimgui_igtreepop();
                }
                _sgimgui_igtext("Color Count: %d", pip_ui->desc.color_count);
                for (int i = 0; i < pip_ui->desc.color_count; i++) {
                    _sgimgui_str_t str;
                    _sgimgui_snprintf(&str, "Color Target %d", i);
                    if (_sgimgui_igtreenode(str.buf)) {
                        _sgimgui_draw_color_target_state(&pip_ui->desc.colors[i]);
                        _sgimgui_igtreepop();
                    }
                }
                _sgimgui_igtext("Prim Type:      %s", _sgimgui_primitivetype_string(pip_ui->desc.primitive_type));
                _sgimgui_igtext("Index Type:     %s", _sgimgui_indextype_string(pip_ui->desc.index_type));
                _sgimgui_igtext("Cull Mode:      %s", _sgimgui_cullmode_string(pip_ui->desc.cull_mode));
                _sgimgui_igtext("Face Winding:   %s", _sgimgui_facewinding_string(pip_ui->desc.face_winding));
                _sgimgui_igtext("Sample Count:   %d", pip_ui->desc.sample_count);
                _sgimgui_str_t blend_color_str;
                _sgimgui_igtext("Blend Color:    %s", _sgimgui_color_string(&blend_color_str, pip_ui->desc.blend_color));
                _sgimgui_igtext("Alpha To Coverage: %s", _sgimgui_bool_string(pip_ui->desc.alpha_to_coverage_enabled));
            }
        } else {
            _sgimgui_igtext("Pipeline 0x%08X not valid.", pip.id);
        }
        _sgimgui_igendchild();
    }
}

_SOKOL_PRIVATE void _sgimgui_draw_buffer_view(_sgimgui_t* ctx, const char* title, const sg_buffer_view_desc* desc) {
    _sgimgui_igtext("%s: ", title);
    _sgimgui_igtext("  Buffer: "); _sgimgui_igsameline();
    if (_sgimgui_draw_buffer_link(ctx, desc->buffer)) {
        _sgimgui_show_buffer(ctx, desc->buffer);
    }
    _sgimgui_igtext("  Offset: %d", desc->offset);
}

_SOKOL_PRIVATE void _sgimgui_draw_image_view(_sgimgui_t* ctx, const char* title, sg_view view, const sg_image_view_desc* desc) {
    _sgimgui_igtext("%s: ", title);
    _sgimgui_igtext("  Image: "); _sgimgui_igsameline();
    if (_sgimgui_draw_image_link(ctx, desc->image)) {
        _sgimgui_show_image(ctx, desc->image);
    }
    _sgimgui_igtext("  Mip Level: %d", desc->mip_level);
    _sgimgui_igtext("  Slice: %d", desc->slice);
    _sgimgui_igseparator();
    _sgimgui_view_t* view_ui = &ctx->view_window.slots[_sgimgui_slot_index(view.id)];
    _sgimgui_draw_image(ctx, desc->image, &view_ui->ui_scale, 4096.0f);
}

_SOKOL_PRIVATE void _sgimgui_draw_texture_view(_sgimgui_t* ctx, const char* title, sg_view view, const sg_texture_view_desc* desc) {
    _sgimgui_igtext("%s: ", title);
    _sgimgui_igtext("  Image: "); _sgimgui_igsameline();
    if (_sgimgui_draw_image_link(ctx, desc->image)) {
        _sgimgui_show_image(ctx, desc->image);
    }
    _sgimgui_igtext("  Mip Levels Base:  %d", desc->mip_levels.base);
    _sgimgui_igtext("  Mip Levels Count: %d", desc->mip_levels.count);
    _sgimgui_igtext("  Slices Base: %d", desc->slices.base);
    _sgimgui_igtext("  Slices Count: %d", desc->slices.count);
    _sgimgui_igseparator();
    _sgimgui_view_t* view_ui = &ctx->view_window.slots[_sgimgui_slot_index(view.id)];
    _sgimgui_draw_image(ctx, desc->image, &view_ui->ui_scale, 4096.0f);
}

_SOKOL_PRIVATE void _sgimgui_draw_view_panel(_sgimgui_t* ctx, sg_view view) {
    if (view.id != SG_INVALID_ID) {
        _sgimgui_igbeginchild("view", IMVEC2(0,0), false, 0);
        sg_view_info info = sg_query_view_info(view);
        if (info.slot.state == SG_RESOURCESTATE_VALID) {
            _sgimgui_view_t* view_ui = &ctx->view_window.slots[_sgimgui_slot_index(view.id)];
            _sgimgui_igtext("Label: %s", view_ui->label.buf[0] ? view_ui->label.buf : "---");
            _sgimgui_draw_resource_slot(&info.slot);
            _sgimgui_igseparator();
            const sg_view_desc desc = sg_query_view_desc(view);
            const sg_view_type type = sg_query_view_type(view);
            switch (type) {
                case SG_VIEWTYPE_STORAGEBUFFER:
                    _sgimgui_draw_buffer_view(ctx, "Storage Buffer", &desc.storage_buffer);
                    break;
                case SG_VIEWTYPE_STORAGEIMAGE:
                    _sgimgui_draw_image_view(ctx, "Storage Image", view, &desc.storage_image);
                    break;
                case SG_VIEWTYPE_TEXTURE:
                    _sgimgui_draw_texture_view(ctx, "Texture", view, &desc.texture);
                    break;
                case SG_VIEWTYPE_COLORATTACHMENT:
                    _sgimgui_draw_image_view(ctx, "Color Attachment", view, &desc.color_attachment);
                    break;
                case SG_VIEWTYPE_RESOLVEATTACHMENT:
                    _sgimgui_draw_image_view(ctx, "Resolve Attachment", view, &desc.resolve_attachment);
                    break;
                case SG_VIEWTYPE_DEPTHSTENCILATTACHMENT:
                    _sgimgui_draw_image_view(ctx, "Depth Stencil Attachment", view, &desc.depth_stencil_attachment);
                    break;
                default:
                    break;
            }
        } else {
            _sgimgui_igtext("View 0x%08X not valid.", view.id);
        }
        _sgimgui_igendchild();
    }
}

_SOKOL_PRIVATE void _sgimgui_draw_bindings_panel(_sgimgui_t* ctx, const sg_bindings* bnd) {
    _sgimgui_igpushid("bnd_vbufs");
    for (int i = 0; i < SG_MAX_VERTEXBUFFER_BINDSLOTS; i++) {
        sg_buffer buf = bnd->vertex_buffers[i];
        if (buf.id != SG_INVALID_ID) {
            _sgimgui_igtext("Vertex Buffer #%d:", i); _sgimgui_igsameline();
            if (_sgimgui_draw_buffer_link(ctx, buf)) {
                _sgimgui_show_buffer(ctx, buf);
            }
            _sgimgui_igsameline();
            _sgimgui_igtext("offset: %d", bnd->vertex_buffer_offsets[i]);
        }
    }
    _sgimgui_igpopid();
    _sgimgui_igpushid("bnd_ibuf");
    if (bnd->index_buffer.id != SG_INVALID_ID) {
        sg_buffer buf = bnd->index_buffer;
        if (buf.id != SG_INVALID_ID) {
            _sgimgui_igtext("Index Buffer:"); _sgimgui_igsameline();
            if (_sgimgui_draw_buffer_link(ctx, buf)) {
                _sgimgui_show_buffer(ctx, buf);
            }
            _sgimgui_igsameline();
            _sgimgui_igtext("offset: %d", bnd->index_buffer_offset);
        }
    }
    _sgimgui_igpopid();
    _sgimgui_igpushid("bnd_views");
    for (int i = 0; i < SG_MAX_VIEW_BINDSLOTS; i++) {
        sg_view view = bnd->views[i];
        if (view.id != SG_INVALID_ID) {
            _sgimgui_igtext("View #%d:", i); _sgimgui_igsameline();
            if (_sgimgui_draw_view_link(ctx, view)) {
                _sgimgui_show_view(ctx, view);
            }
        }
    }
    _sgimgui_igpopid();
    _sgimgui_igpushid("bnd_smps");
    for (int i = 0; i < SG_MAX_SAMPLER_BINDSLOTS; i++) {
        sg_sampler smp = bnd->samplers[i];
        if (smp.id != SG_INVALID_ID) {
            _sgimgui_igtext("Sampler Slot #%d:", i); _sgimgui_igsameline();
            if (_sgimgui_draw_sampler_link(ctx, smp)) {
                _sgimgui_show_sampler(ctx, smp);
            }
        }
    }
    _sgimgui_igpopid();
}

_SOKOL_PRIVATE void _sgimgui_draw_uniforms_panel(_sgimgui_t* ctx, const _sgimgui_args_apply_uniforms_t* args) {
    SOKOL_ASSERT(args->ub_slot < SG_MAX_UNIFORMBLOCK_BINDSLOTS);

    /* check if all the required information for drawing the structured uniform block content
        is available, otherwise just render a generic hexdump
    */
    if (sg_query_pipeline_state(args->pipeline) != SG_RESOURCESTATE_VALID) {
        _sgimgui_igtext("Pipeline object not valid!");
        return;
    }
    _sgimgui_pipeline_t* pip_ui = &ctx->pipeline_window.slots[_sgimgui_slot_index(args->pipeline.id)];
    if (sg_query_shader_state(pip_ui->desc.shader) != SG_RESOURCESTATE_VALID) {
        _sgimgui_igtext("Shader object not valid!");
        return;
    }
    _sgimgui_shader_t* shd_ui = &ctx->shader_window.slots[_sgimgui_slot_index(pip_ui->desc.shader.id)];
    SOKOL_ASSERT(shd_ui->res_id.id == pip_ui->desc.shader.id);
    const sg_shader_uniform_block* ub_desc = &shd_ui->desc.uniform_blocks[args->ub_slot];
    SOKOL_ASSERT(args->data_size <= ub_desc->size);
    bool draw_dump = false;
    if (ub_desc->glsl_uniforms[0].type == SG_UNIFORMTYPE_INVALID) {
        draw_dump = true;
    }

    _sgimgui_capture_bucket_t* bucket = _sgimgui_capture_get_read_bucket(ctx);
    SOKOL_ASSERT((args->ubuf_pos + args->data_size) <= bucket->ubuf_size);
    const float* uptrf = (const float*) (bucket->ubuf + args->ubuf_pos);
    const int32_t* uptri32 = (const int32_t*) uptrf;
    if (!draw_dump) {
        uint32_t u_off = 0;
        for (int i = 0; i < SG_MAX_UNIFORMBLOCK_MEMBERS; i++) {
            const sg_glsl_shader_uniform* ud = &ub_desc->glsl_uniforms[i];
            if (ud->type == SG_UNIFORMTYPE_INVALID) {
                break;
            }
            int num_items = (ud->array_count > 1) ? ud->array_count : 1;
            if (num_items > 1) {
                _sgimgui_igtext("%d: %s %s[%d] =", i, _sgimgui_uniformtype_string(ud->type), ud->glsl_name?ud->glsl_name:"", ud->array_count);
            } else {
                _sgimgui_igtext("%d: %s %s =", i, _sgimgui_uniformtype_string(ud->type), ud->glsl_name?ud->glsl_name:"");
            }
            for (int item_index = 0; item_index < num_items; item_index++) {
                const uint32_t u_size = _sgimgui_std140_uniform_size(ud->type, ud->array_count) / 4;
                const uint32_t u_align = _sgimgui_std140_uniform_alignment(ud->type, ud->array_count) / 4;
                u_off = _sgimgui_align_u32(u_off, u_align);
                switch (ud->type) {
                    case SG_UNIFORMTYPE_FLOAT:
                        _sgimgui_igtext("    %.3f", uptrf[u_off]);
                        break;
                    case SG_UNIFORMTYPE_INT:
                        _sgimgui_igtext("    %d", uptri32[u_off]);
                        break;
                    case SG_UNIFORMTYPE_FLOAT2:
                        _sgimgui_igtext("    %.3f, %.3f", uptrf[u_off], uptrf[u_off+1]);
                        break;
                    case SG_UNIFORMTYPE_INT2:
                        _sgimgui_igtext("    %d, %d", uptri32[u_off], uptri32[u_off+1]);
                        break;
                    case SG_UNIFORMTYPE_FLOAT3:
                        _sgimgui_igtext("    %.3f, %.3f, %.3f", uptrf[u_off], uptrf[u_off+1], uptrf[u_off+2]);
                        break;
                    case SG_UNIFORMTYPE_INT3:
                        _sgimgui_igtext("    %d, %d, %d", uptri32[u_off], uptri32[u_off+1], uptri32[u_off+2]);
                        break;
                    case SG_UNIFORMTYPE_FLOAT4:
                        _sgimgui_igtext("    %.3f, %.3f, %.3f, %.3f", uptrf[u_off], uptrf[u_off+1], uptrf[u_off+2], uptrf[u_off+3]);
                        break;
                    case SG_UNIFORMTYPE_INT4:
                        _sgimgui_igtext("    %d, %d, %d, %d", uptri32[u_off], uptri32[u_off+1], uptri32[u_off+2], uptri32[u_off+3]);
                        break;
                    case SG_UNIFORMTYPE_MAT4:
                        _sgimgui_igtext("    %.3f, %.3f, %.3f, %.3f\n"
                               "    %.3f, %.3f, %.3f, %.3f\n"
                               "    %.3f, %.3f, %.3f, %.3f\n"
                               "    %.3f, %.3f, %.3f, %.3f",
                            uptrf[u_off+0],  uptrf[u_off+1],  uptrf[u_off+2],  uptrf[u_off+3],
                            uptrf[u_off+4],  uptrf[u_off+5],  uptrf[u_off+6],  uptrf[u_off+7],
                            uptrf[u_off+8],  uptrf[u_off+9],  uptrf[u_off+10], uptrf[u_off+11],
                            uptrf[u_off+12], uptrf[u_off+13], uptrf[u_off+14], uptrf[u_off+15]);
                        break;
                    default:
                        _sgimgui_igtext("???");
                        break;
                }
                u_off += u_size;
            }
        }
    } else {
        // FIXME: float vs int
        const size_t num_floats = ub_desc->size / sizeof(float);
        for (uint32_t i = 0; i < num_floats; i++) {
            _sgimgui_igtext("%.3f, ", uptrf[i]);
            if (((i + 1) % 4) != 0) {
                _sgimgui_igsameline();
            }
        }
    }
}

_SOKOL_PRIVATE void _sgimgui_draw_passaction_panel(const sg_pass_action* action, int num_color_atts) {
    _sgimgui_igtext("Pass Action:");
    for (int i = 0; i < num_color_atts; i++) {
        const sg_color_attachment_action* c_att = &action->colors[i];
        _sgimgui_igtext("  Color Attachment %d:", i);
        _sgimgui_str_t color_str;
        switch (c_att->load_action) {
            case SG_LOADACTION_LOAD: _sgimgui_igtext("    SG_LOADACTION_LOAD"); break;
            case SG_LOADACTION_DONTCARE: _sgimgui_igtext("    SG_LOADACTION_DONTCARE"); break;
            case SG_LOADACTION_CLEAR:
                _sgimgui_igtext("    SG_LOADACTION_CLEAR: %s", _sgimgui_color_string(&color_str, c_att->clear_value));
                break;
            default: _sgimgui_igtext("    ???"); break;
        }
        switch (c_att->store_action) {
            case SG_STOREACTION_STORE: _sgimgui_igtext("    SG_STOREACTION_STORE"); break;
            case SG_STOREACTION_DONTCARE: _sgimgui_igtext("    SG_STOREACTION_DONTCARE"); break;
            default: _sgimgui_igtext("    ???"); break;
        }
    }
    const sg_depth_attachment_action* d_att = &action->depth;
    _sgimgui_igtext("  Depth Attachment:");
    switch (d_att->load_action) {
        case SG_LOADACTION_LOAD: _sgimgui_igtext("    SG_LOADACTION_LOAD"); break;
        case SG_LOADACTION_DONTCARE: _sgimgui_igtext("    SG_LOADACTION_DONTCARE"); break;
        case SG_LOADACTION_CLEAR: _sgimgui_igtext("    SG_LOADACTION_CLEAR: %.3f", d_att->clear_value); break;
        default: _sgimgui_igtext("    ???"); break;
    }
    switch (d_att->store_action) {
        case SG_STOREACTION_STORE: _sgimgui_igtext("    SG_STOREACTION_STORE"); break;
        case SG_STOREACTION_DONTCARE: _sgimgui_igtext("    SG_STOREACTION_DONTCARE"); break;
        default: _sgimgui_igtext("    ???"); break;
    }
    const sg_stencil_attachment_action* s_att = &action->stencil;
    _sgimgui_igtext("  Stencil Attachment");
    switch (s_att->load_action) {
        case SG_LOADACTION_LOAD: _sgimgui_igtext("    SG_LOADACTION_LOAD"); break;
        case SG_LOADACTION_DONTCARE: _sgimgui_igtext("    SG_LOADACTION_DONTCARE"); break;
        case SG_LOADACTION_CLEAR: _sgimgui_igtext("    SG_LOADACTION_CLEAR: 0x%02X", s_att->clear_value); break;
        default: _sgimgui_igtext("    ???"); break;
    }
    switch (d_att->store_action) {
        case SG_STOREACTION_STORE: _sgimgui_igtext("    SG_STOREACTION_STORE"); break;
        case SG_STOREACTION_DONTCARE: _sgimgui_igtext("    SG_STOREACTION_DONTCARE"); break;
        default: _sgimgui_igtext("    ???"); break;
    }
}

_SOKOL_PRIVATE void _sgimgui_draw_attachments_panel(_sgimgui_t* ctx, const sg_attachments* atts, int num_color_atts) {
    _sgimgui_igtext("Attachments:");
    for (int i = 0; i < num_color_atts; i++) {
        if (atts->colors[i].id!= SG_INVALID_ID) {
            sg_view view = atts->colors[i];
            _sgimgui_igtext("  Color Attachment #%d:", i); _sgimgui_igsameline();
            if (_sgimgui_draw_view_link(ctx, view)) {
                _sgimgui_show_view(ctx, view);
            }
        }
    }
    for (int i = 0; i < num_color_atts; i++) {
        if (atts->resolves[i].id != SG_INVALID_ID) {
            sg_view view = atts->resolves[i];
            _sgimgui_igtext("  Resolve Attachment #%d:", i); _sgimgui_igsameline();
            if (_sgimgui_draw_view_link(ctx, view)) {
                _sgimgui_show_view(ctx, view);
            }
        }
    }
    if (atts->depth_stencil.id != SG_INVALID_ID) {
        sg_view view = atts->depth_stencil;
        _sgimgui_igtext("  Depth Stencil Attachment:"); _sgimgui_igsameline();
        if (_sgimgui_draw_view_link(ctx, view)) {
            _sgimgui_show_view(ctx, view);
        }
    }
}

_SOKOL_PRIVATE void _sgimgui_draw_swapchain_panel(sg_swapchain* swapchain) {
    _sgimgui_igtext("Swapchain:");
    _sgimgui_igtext("  Width: %d", swapchain->width);
    _sgimgui_igtext("  Height: %d", swapchain->height);
    _sgimgui_igtext("  Sample Count: %d", swapchain->sample_count);
    _sgimgui_igtext("  Color Format: %s", _sgimgui_pixelformat_string(swapchain->color_format));
    _sgimgui_igtext("  Depth Format: %s", _sgimgui_pixelformat_string(swapchain->depth_format));
    _sgimgui_igseparator();
    switch (sg_query_backend()) {
        case SG_BACKEND_D3D11:
            _sgimgui_igtext("D3D11 Objects:");
            _sgimgui_igtext("  Render View: %p", swapchain->d3d11.render_view);
            _sgimgui_igtext("  Resolve View: %p", swapchain->d3d11.resolve_view);
            _sgimgui_igtext("  Depth Stencil View: %p", swapchain->d3d11.depth_stencil_view);
            break;
        case SG_BACKEND_WGPU:
            _sgimgui_igtext("WGPU Objects:");
            _sgimgui_igtext("  Render View: %p", swapchain->wgpu.render_view);
            _sgimgui_igtext("  Resolve View: %p", swapchain->wgpu.resolve_view);
            _sgimgui_igtext("  Depth Stencil View: %p", swapchain->wgpu.depth_stencil_view);
            break;
        case SG_BACKEND_METAL_MACOS:
        case SG_BACKEND_METAL_IOS:
        case SG_BACKEND_METAL_SIMULATOR:
            _sgimgui_igtext("Metal Objects:");
            _sgimgui_igtext("  Current Drawable: %p", swapchain->metal.current_drawable);
            _sgimgui_igtext("  Depth Stencil Texture: %p", swapchain->metal.depth_stencil_texture);
            _sgimgui_igtext("  MSAA Color Texture: %p", swapchain->metal.msaa_color_texture);
            break;
        case SG_BACKEND_GLCORE:
        case SG_BACKEND_GLES3:
            _sgimgui_igtext("GL Objects:");
            _sgimgui_igtext("  Framebuffer: %d", swapchain->gl.framebuffer);
            break;
        case SG_BACKEND_VULKAN:
            _sgimgui_igtext("Vulkan Objects:");
            _sgimgui_igtext("  Render Image: %p", swapchain->vulkan.render_image);
            _sgimgui_igtext("  Render View: %p", swapchain->vulkan.render_view);
            _sgimgui_igtext("  Resolve Image: %p", swapchain->vulkan.resolve_image);
            _sgimgui_igtext("  Resolve View: %p", swapchain->vulkan.resolve_view);
            _sgimgui_igtext("  Depth Stencil Image: %p", swapchain->vulkan.depth_stencil_image);
            _sgimgui_igtext("  Depth Stencil View: %p", swapchain->vulkan.depth_stencil_view);
            _sgimgui_igtext("  Render Finished Semaphore: %p", swapchain->vulkan.render_finished_semaphore);
            _sgimgui_igtext("  Present Complete Semaphore: %p", swapchain->vulkan.present_complete_semaphore);
            break;
        default:
            _sgimgui_igtext("  UNKNOWN BACKEND!");
            break;
    }
}

_SOKOL_PRIVATE void _sgimgui_draw_pass_panel(_sgimgui_t* ctx, sg_pass* pass) {
    bool is_compute_pass = pass->compute;
    bool is_attachments_pass = false;
    bool is_swapchain_pass = false;
    int num_color_atts = 0;
    if (!is_compute_pass) {
        for (int i = 0; i < SG_MAX_COLOR_ATTACHMENTS; i++) {
            if (pass->attachments.colors[i].id != SG_INVALID_ID) {
                num_color_atts++;
                is_attachments_pass = true;
            }
        }
        if (pass->attachments.depth_stencil.id != SG_INVALID_ID) {
            is_attachments_pass = true;
        }
        if (!is_attachments_pass) {
            num_color_atts = 1;
            is_swapchain_pass = true;
        }
    }
    _sgimgui_igtext("Compute: %s", _sgimgui_bool_string(is_compute_pass));
    _sgimgui_igseparator();
    if (!is_compute_pass) {
        _sgimgui_draw_passaction_panel(&pass->action, num_color_atts);
        _sgimgui_igseparator();
        if (is_attachments_pass) {
            _sgimgui_draw_attachments_panel(ctx, &pass->attachments, num_color_atts);
        } else if (is_swapchain_pass) {
            _sgimgui_draw_swapchain_panel(&pass->swapchain);
        }
    }
}

_SOKOL_PRIVATE void _sgimgui_draw_capture_panel(_sgimgui_t* ctx) {
    int sel_item_index = ctx->capture_window.sel_item;
    if (sel_item_index >= _sgimgui_capture_num_read_items(ctx)) {
        return;
    }
    _sgimgui_capture_item_t* item = _sgimgui_capture_read_item_at(ctx, sel_item_index);
    _sgimgui_igbeginchild("capture_item", IMVEC2(0, 0), false, 0);
    _sgimgui_igpushstylecolor(ImGuiCol_Text, item->color);
    _sgimgui_igtext("%s", _sgimgui_capture_item_string(ctx, sel_item_index, item).buf);
    _sgimgui_igpopstylecolor();
    _sgimgui_igseparator();
    switch (item->cmd) {
        case _SGIMGUI_CMD_RESET_STATE_CACHE:
            break;
        case _SGIMGUI_CMD_MAKE_BUFFER:
            _sgimgui_draw_buffer_panel(ctx, item->args.make_buffer.result);
            break;
        case _SGIMGUI_CMD_MAKE_IMAGE:
            _sgimgui_draw_image_panel(ctx, item->args.make_image.result);
            break;
        case _SGIMGUI_CMD_MAKE_SAMPLER:
            _sgimgui_draw_sampler_panel(ctx, item->args.make_sampler.result);
            break;
        case _SGIMGUI_CMD_MAKE_SHADER:
            _sgimgui_draw_shader_panel(ctx, item->args.make_shader.result);
            break;
        case _SGIMGUI_CMD_MAKE_PIPELINE:
            _sgimgui_draw_pipeline_panel(ctx, item->args.make_pipeline.result);
            break;
        case _SGIMGUI_CMD_MAKE_VIEW:
            _sgimgui_draw_view_panel(ctx, item->args.make_view.result);
            break;
        case _SGIMGUI_CMD_DESTROY_BUFFER:
            _sgimgui_draw_buffer_panel(ctx, item->args.destroy_buffer.buffer);
            break;
        case _SGIMGUI_CMD_DESTROY_IMAGE:
            _sgimgui_draw_image_panel(ctx, item->args.destroy_image.image);
            break;
        case _SGIMGUI_CMD_DESTROY_SAMPLER:
            _sgimgui_draw_sampler_panel(ctx, item->args.destroy_sampler.sampler);
            break;
        case _SGIMGUI_CMD_DESTROY_SHADER:
            _sgimgui_draw_shader_panel(ctx, item->args.destroy_shader.shader);
            break;
        case _SGIMGUI_CMD_DESTROY_PIPELINE:
            _sgimgui_draw_pipeline_panel(ctx, item->args.destroy_pipeline.pipeline);
            break;
        case _SGIMGUI_CMD_DESTROY_VIEW:
            _sgimgui_draw_view_panel(ctx, item->args.destroy_view.view);
            break;
        case _SGIMGUI_CMD_UPDATE_BUFFER:
            _sgimgui_draw_buffer_panel(ctx, item->args.update_buffer.buffer);
            break;
        case _SGIMGUI_CMD_UPDATE_IMAGE:
            _sgimgui_draw_image_panel(ctx, item->args.update_image.image);
            break;
        case _SGIMGUI_CMD_APPEND_BUFFER:
            _sgimgui_draw_buffer_panel(ctx, item->args.update_buffer.buffer);
            break;
        case _SGIMGUI_CMD_BEGIN_PASS:
            _sgimgui_draw_pass_panel(ctx, &item->args.begin_pass.pass);
            break;
        case _SGIMGUI_CMD_APPLY_VIEWPORT:
        case _SGIMGUI_CMD_APPLY_SCISSOR_RECT:
            break;
        case _SGIMGUI_CMD_APPLY_PIPELINE:
            _sgimgui_draw_pipeline_panel(ctx, item->args.apply_pipeline.pipeline);
            break;
        case _SGIMGUI_CMD_APPLY_BINDINGS:
            _sgimgui_draw_bindings_panel(ctx, &item->args.apply_bindings.bindings);
            break;
        case _SGIMGUI_CMD_APPLY_UNIFORMS:
            _sgimgui_draw_uniforms_panel(ctx, &item->args.apply_uniforms);
            break;
        case _SGIMGUI_CMD_DRAW:
        case _SGIMGUI_CMD_DRAW_EX:
        case _SGIMGUI_CMD_DISPATCH:
        case _SGIMGUI_CMD_END_PASS:
        case _SGIMGUI_CMD_COMMIT:
            break;
        case _SGIMGUI_CMD_ALLOC_BUFFER:
            _sgimgui_draw_buffer_panel(ctx, item->args.alloc_buffer.result);
            break;
        case _SGIMGUI_CMD_ALLOC_IMAGE:
            _sgimgui_draw_image_panel(ctx, item->args.alloc_image.result);
            break;
        case _SGIMGUI_CMD_ALLOC_SAMPLER:
            _sgimgui_draw_sampler_panel(ctx, item->args.alloc_sampler.result);
            break;
        case _SGIMGUI_CMD_ALLOC_SHADER:
            _sgimgui_draw_shader_panel(ctx, item->args.alloc_shader.result);
            break;
        case _SGIMGUI_CMD_ALLOC_PIPELINE:
            _sgimgui_draw_pipeline_panel(ctx, item->args.alloc_pipeline.result);
            break;
        case _SGIMGUI_CMD_ALLOC_VIEW:
            _sgimgui_draw_view_panel(ctx, item->args.alloc_view.result);
            break;
        case _SGIMGUI_CMD_INIT_BUFFER:
            _sgimgui_draw_buffer_panel(ctx, item->args.init_buffer.buffer);
            break;
        case _SGIMGUI_CMD_INIT_IMAGE:
            _sgimgui_draw_image_panel(ctx, item->args.init_image.image);
            break;
        case _SGIMGUI_CMD_INIT_SAMPLER:
            _sgimgui_draw_sampler_panel(ctx, item->args.init_sampler.sampler);
            break;
        case _SGIMGUI_CMD_INIT_SHADER:
            _sgimgui_draw_shader_panel(ctx, item->args.init_shader.shader);
            break;
        case _SGIMGUI_CMD_INIT_PIPELINE:
            _sgimgui_draw_pipeline_panel(ctx, item->args.init_pipeline.pipeline);
            break;
        case _SGIMGUI_CMD_INIT_VIEW:
            _sgimgui_draw_view_panel(ctx, item->args.init_view.view);
            break;
        case _SGIMGUI_CMD_FAIL_BUFFER:
            _sgimgui_draw_buffer_panel(ctx, item->args.fail_buffer.buffer);
            break;
        case _SGIMGUI_CMD_FAIL_IMAGE:
            _sgimgui_draw_image_panel(ctx, item->args.fail_image.image);
            break;
        case _SGIMGUI_CMD_FAIL_SAMPLER:
            _sgimgui_draw_sampler_panel(ctx, item->args.fail_sampler.sampler);
            break;
        case _SGIMGUI_CMD_FAIL_SHADER:
            _sgimgui_draw_shader_panel(ctx, item->args.fail_shader.shader);
            break;
        case _SGIMGUI_CMD_FAIL_PIPELINE:
            _sgimgui_draw_pipeline_panel(ctx, item->args.fail_pipeline.pipeline);
            break;
        case _SGIMGUI_CMD_FAIL_VIEW:
            _sgimgui_draw_view_panel(ctx, item->args.fail_view.view);
            break;
        default:
            break;
    }
    _sgimgui_igendchild();
}

_SOKOL_PRIVATE void _sgimgui_draw_caps_panel(void) {
    _sgimgui_igtext("Backend: %s\n", _sgimgui_backend_string(sg_query_backend()));
    _sgimgui_igtext("Dear ImGui Version: %s\n\n", IMGUI_VERSION);
    sg_features f = sg_query_features();
    _sgimgui_igtext("Features:");
    _sgimgui_igtext("    origin_top_left: %s", _sgimgui_bool_string(f.origin_top_left));
    _sgimgui_igtext("    image_clamp_to_border: %s", _sgimgui_bool_string(f.image_clamp_to_border));
    _sgimgui_igtext("    mrt_independent_blend_state: %s", _sgimgui_bool_string(f.mrt_independent_blend_state));
    _sgimgui_igtext("    mrt_independent_write_mask: %s", _sgimgui_bool_string(f.mrt_independent_write_mask));
    _sgimgui_igtext("    compute: %s", _sgimgui_bool_string(f.compute));
    _sgimgui_igtext("    msaa_texture_bindings: %s", _sgimgui_bool_string(f.msaa_texture_bindings));
    _sgimgui_igtext("    separate_buffer_types: %s", _sgimgui_bool_string(f.separate_buffer_types));
    _sgimgui_igtext("    draw_base_vertex: %s", _sgimgui_bool_string(f.draw_base_vertex));
    _sgimgui_igtext("    draw_base_instance: %s", _sgimgui_bool_string(f.draw_base_instance));
    _sgimgui_igtext("    gl_texture_views: %s", _sgimgui_bool_string(f.gl_texture_views));
    sg_limits l = sg_query_limits();
    _sgimgui_igtext("\nLimits:\n");
    _sgimgui_igtext("    max_image_size_2d: %d", l.max_image_size_2d);
    _sgimgui_igtext("    max_image_size_cube: %d", l.max_image_size_cube);
    _sgimgui_igtext("    max_image_size_3d: %d", l.max_image_size_3d);
    _sgimgui_igtext("    max_image_size_array: %d", l.max_image_size_array);
    _sgimgui_igtext("    max_image_array_layers: %d", l.max_image_array_layers);
    _sgimgui_igtext("    max_vertex_attrs: %d", l.max_vertex_attrs);
    _sgimgui_igtext("    max_color_attachments: %d", l.max_color_attachments);
    _sgimgui_igtext("    max_texture_bindings_per_stage: %d", l.max_texture_bindings_per_stage);
    _sgimgui_igtext("    max_storage_buffer_bindings_per_stage: %d", l.max_storage_buffer_bindings_per_stage);
    _sgimgui_igtext("    max_storage_image_bindings_per_stage: %d", l.max_storage_image_bindings_per_stage);
    _sgimgui_igtext("    gl_max_vertex_uniform_components: %d", l.gl_max_vertex_uniform_components);
    _sgimgui_igtext("    gl_max_combined_texture_image_units: %d", l.gl_max_combined_texture_image_units);
    _sgimgui_igtext("    d3d11_max_unordered_access_views: %d", l.d3d11_max_unordered_access_views);
    _sgimgui_igtext("    vk_min_uniform_buffer_offset_alignment: %d", l.vk_min_uniform_buffer_offset_alignment);
    _sgimgui_igtext("\nStruct Sizes:\n");
    _sgimgui_igtext("    sg_desc:           %d bytes\n", (int)sizeof(sg_desc));
    _sgimgui_igtext("    sg_buffer_desc:    %d bytes\n", (int)sizeof(sg_buffer_desc));
    _sgimgui_igtext("    sg_image_desc:     %d bytes\n", (int)sizeof(sg_image_desc));
    _sgimgui_igtext("    sg_view_desc:      %d bytes\n", (int)sizeof(sg_view_desc));
    _sgimgui_igtext("    sg_sampler_desc:   %d bytes\n", (int)sizeof(sg_sampler_desc));
    _sgimgui_igtext("    sg_shader_desc:    %d bytes\n", (int)sizeof(sg_shader_desc));
    _sgimgui_igtext("    sg_pipeline_desc:  %d bytes\n", (int)sizeof(sg_pipeline_desc));
    _sgimgui_igtext("    sg_pass:           %d bytes\n", (int)sizeof(sg_pass));
    _sgimgui_igtext("    sg_bindings:       %d bytes\n", (int)sizeof(sg_bindings));
    _sgimgui_igtext("\nUsable Pixelformats:");
    for (int i = (int)(SG_PIXELFORMAT_NONE+1); i < (int)_SG_PIXELFORMAT_NUM; i++) {
        sg_pixel_format fmt = (sg_pixel_format)i;
        sg_pixelformat_info info = sg_query_pixelformat(fmt);
        if (info.sample) {
            _sgimgui_igtext("  %s: %s%s%s%s%s%s%s%s%s",
                _sgimgui_pixelformat_string(fmt),
                info.sample ? "SAMPLE ":"",
                info.filter ? "FILTER ":"",
                info.blend ? "BLEND ":"",
                info.render ? "RENDER ":"",
                info.msaa ? "MSAA ":"",
                info.depth ? "DEPTH ":"",
                info.compressed ? "COMPRESSED ":"",
                info.read ? "READ ":"",
                info.write ? "WRITE ":"");
        }
    }
}

_SOKOL_PRIVATE void _sgimgui_frame_add_stats_row(const char* key, uint32_t value) {
    _sgimgui_igtablenextrow();
    _sgimgui_igtablesetcolumnindex(0);
    _sgimgui_igtext("%s", key);
    _sgimgui_igtablesetcolumnindex(1);
    _sgimgui_igtext("%d", value);
}

#define _sgimgui_frame_stats(key) _sgimgui_frame_add_stats_row(#key, stats->key)

_SOKOL_PRIVATE void _sgimgui_draw_frame_stats_panel(_sgimgui_t* ctx) {
    _SOKOL_UNUSED(ctx);
    _sgimgui_igcheckbox("Ignore sokol_imgui.h", &ctx->frame_stats_window.disable_sokol_imgui_stats);
    const sg_stats* stats = &ctx->frame_stats_window.stats;
    const ImGuiTableFlags flags =
        ImGuiTableFlags_Resizable |
        ImGuiTableFlags_ScrollY |
        ImGuiTableFlags_SizingFixedFit |
        ImGuiTableFlags_Borders;
    if (_sgimgui_igbegintable("#frame_stats_table", 2, flags)) {
        _sgimgui_igtablesetupscrollfreeze(0, 1);
        _sgimgui_igtablesetupcolumn("key", ImGuiTableColumnFlags_None);
        _sgimgui_igtablesetupcolumn("value", ImGuiTableColumnFlags_None);
        _sgimgui_igtableheadersrow();
        _sgimgui_frame_stats(prev_frame.frame_index);
        _sgimgui_frame_stats(prev_frame.num_passes);
        _sgimgui_frame_stats(prev_frame.num_apply_viewport);
        _sgimgui_frame_stats(prev_frame.num_apply_scissor_rect);
        _sgimgui_frame_stats(prev_frame.num_apply_pipeline);
        _sgimgui_frame_stats(prev_frame.num_apply_bindings);
        _sgimgui_frame_stats(prev_frame.num_apply_uniforms);
        _sgimgui_frame_stats(prev_frame.num_draw);
        _sgimgui_frame_stats(prev_frame.num_draw_ex);
        _sgimgui_frame_stats(prev_frame.num_dispatch);
        _sgimgui_frame_stats(prev_frame.num_update_buffer);
        _sgimgui_frame_stats(prev_frame.num_append_buffer);
        _sgimgui_frame_stats(prev_frame.num_update_image);
        _sgimgui_frame_stats(prev_frame.size_apply_uniforms);
        _sgimgui_frame_stats(prev_frame.size_update_buffer);
        _sgimgui_frame_stats(prev_frame.size_append_buffer);
        _sgimgui_frame_stats(prev_frame.size_update_image);
        _sgimgui_frame_stats(prev_frame.buffers.allocated);
        _sgimgui_frame_stats(prev_frame.buffers.deallocated);
        _sgimgui_frame_stats(prev_frame.buffers.inited);
        _sgimgui_frame_stats(prev_frame.buffers.uninited);
        _sgimgui_frame_stats(prev_frame.images.allocated);
        _sgimgui_frame_stats(prev_frame.images.deallocated);
        _sgimgui_frame_stats(prev_frame.images.inited);
        _sgimgui_frame_stats(prev_frame.images.uninited);
        _sgimgui_frame_stats(prev_frame.views.allocated);
        _sgimgui_frame_stats(prev_frame.views.deallocated);
        _sgimgui_frame_stats(prev_frame.views.inited);
        _sgimgui_frame_stats(prev_frame.views.uninited);
        _sgimgui_frame_stats(prev_frame.shaders.allocated);
        _sgimgui_frame_stats(prev_frame.shaders.deallocated);
        _sgimgui_frame_stats(prev_frame.shaders.inited);
        _sgimgui_frame_stats(prev_frame.shaders.uninited);
        _sgimgui_frame_stats(prev_frame.pipelines.allocated);
        _sgimgui_frame_stats(prev_frame.pipelines.deallocated);
        _sgimgui_frame_stats(prev_frame.pipelines.inited);
        _sgimgui_frame_stats(prev_frame.pipelines.uninited);
        switch (sg_query_backend()) {
            case SG_BACKEND_GLCORE:
            case SG_BACKEND_GLES3:
                _sgimgui_frame_stats(prev_frame.gl.num_bind_buffer);
                _sgimgui_frame_stats(prev_frame.gl.num_active_texture);
                _sgimgui_frame_stats(prev_frame.gl.num_bind_texture);
                _sgimgui_frame_stats(prev_frame.gl.num_bind_image_texture);
                _sgimgui_frame_stats(prev_frame.gl.num_bind_sampler);
                _sgimgui_frame_stats(prev_frame.gl.num_use_program);
                _sgimgui_frame_stats(prev_frame.gl.num_render_state);
                _sgimgui_frame_stats(prev_frame.gl.num_vertex_attrib_pointer);
                _sgimgui_frame_stats(prev_frame.gl.num_vertex_attrib_divisor);
                _sgimgui_frame_stats(prev_frame.gl.num_enable_vertex_attrib_array);
                _sgimgui_frame_stats(prev_frame.gl.num_disable_vertex_attrib_array);
                _sgimgui_frame_stats(prev_frame.gl.num_uniform);
                _sgimgui_frame_stats(prev_frame.gl.num_memory_barriers);
                break;
            case SG_BACKEND_WGPU:
                _sgimgui_frame_stats(prev_frame.wgpu.uniforms.num_set_bindgroup);
                _sgimgui_frame_stats(prev_frame.wgpu.uniforms.size_write_buffer);
                _sgimgui_frame_stats(prev_frame.wgpu.bindings.num_set_vertex_buffer);
                _sgimgui_frame_stats(prev_frame.wgpu.bindings.num_skip_redundant_vertex_buffer);
                _sgimgui_frame_stats(prev_frame.wgpu.bindings.num_set_index_buffer);
                _sgimgui_frame_stats(prev_frame.wgpu.bindings.num_skip_redundant_index_buffer);
                _sgimgui_frame_stats(prev_frame.wgpu.bindings.num_create_bindgroup);
                _sgimgui_frame_stats(prev_frame.wgpu.bindings.num_discard_bindgroup);
                _sgimgui_frame_stats(prev_frame.wgpu.bindings.num_set_bindgroup);
                _sgimgui_frame_stats(prev_frame.wgpu.bindings.num_skip_redundant_bindgroup);
                _sgimgui_frame_stats(prev_frame.wgpu.bindings.num_bindgroup_cache_hits);
                _sgimgui_frame_stats(prev_frame.wgpu.bindings.num_bindgroup_cache_misses);
                _sgimgui_frame_stats(prev_frame.wgpu.bindings.num_bindgroup_cache_collisions);
                _sgimgui_frame_stats(prev_frame.wgpu.bindings.num_bindgroup_cache_invalidates);
                _sgimgui_frame_stats(prev_frame.wgpu.bindings.num_bindgroup_cache_hash_vs_key_mismatch);
                break;
            case SG_BACKEND_METAL_MACOS:
            case SG_BACKEND_METAL_IOS:
            case SG_BACKEND_METAL_SIMULATOR:
                _sgimgui_frame_stats(prev_frame.metal.idpool.num_added);
                _sgimgui_frame_stats(prev_frame.metal.idpool.num_released);
                _sgimgui_frame_stats(prev_frame.metal.idpool.num_garbage_collected);
                _sgimgui_frame_stats(prev_frame.metal.pipeline.num_set_blend_color);
                _sgimgui_frame_stats(prev_frame.metal.pipeline.num_set_cull_mode);
                _sgimgui_frame_stats(prev_frame.metal.pipeline.num_set_front_facing_winding);
                _sgimgui_frame_stats(prev_frame.metal.pipeline.num_set_stencil_reference_value);
                _sgimgui_frame_stats(prev_frame.metal.pipeline.num_set_depth_bias);
                _sgimgui_frame_stats(prev_frame.metal.pipeline.num_set_render_pipeline_state);
                _sgimgui_frame_stats(prev_frame.metal.pipeline.num_set_depth_stencil_state);
                _sgimgui_frame_stats(prev_frame.metal.bindings.num_set_vertex_buffer);
                _sgimgui_frame_stats(prev_frame.metal.bindings.num_set_fragment_buffer);
                _sgimgui_frame_stats(prev_frame.metal.bindings.num_set_compute_buffer);
                _sgimgui_frame_stats(prev_frame.metal.bindings.num_set_vertex_buffer_offset);
                _sgimgui_frame_stats(prev_frame.metal.bindings.num_set_fragment_buffer_offset);
                _sgimgui_frame_stats(prev_frame.metal.bindings.num_set_compute_buffer_offset);
                _sgimgui_frame_stats(prev_frame.metal.bindings.num_set_vertex_texture);
                _sgimgui_frame_stats(prev_frame.metal.bindings.num_set_fragment_texture);
                _sgimgui_frame_stats(prev_frame.metal.bindings.num_set_compute_texture);
                _sgimgui_frame_stats(prev_frame.metal.bindings.num_set_vertex_sampler_state);
                _sgimgui_frame_stats(prev_frame.metal.bindings.num_set_fragment_sampler_state);
                _sgimgui_frame_stats(prev_frame.metal.bindings.num_set_compute_sampler_state);
                _sgimgui_frame_stats(prev_frame.metal.bindings.num_skip_redundant_vertex_buffer);
                _sgimgui_frame_stats(prev_frame.metal.bindings.num_skip_redundant_fragment_buffer);
                _sgimgui_frame_stats(prev_frame.metal.bindings.num_skip_redundant_compute_buffer);
                _sgimgui_frame_stats(prev_frame.metal.bindings.num_skip_redundant_vertex_texture);
                _sgimgui_frame_stats(prev_frame.metal.bindings.num_skip_redundant_fragment_texture);
                _sgimgui_frame_stats(prev_frame.metal.bindings.num_skip_redundant_compute_texture);
                _sgimgui_frame_stats(prev_frame.metal.bindings.num_skip_redundant_vertex_sampler_state);
                _sgimgui_frame_stats(prev_frame.metal.bindings.num_skip_redundant_fragment_sampler_state);
                _sgimgui_frame_stats(prev_frame.metal.bindings.num_skip_redundant_compute_sampler_state);
                _sgimgui_frame_stats(prev_frame.metal.uniforms.num_set_vertex_buffer_offset);
                _sgimgui_frame_stats(prev_frame.metal.uniforms.num_set_fragment_buffer_offset);
                _sgimgui_frame_stats(prev_frame.metal.uniforms.num_set_compute_buffer_offset);
                break;
            case SG_BACKEND_D3D11:
                _sgimgui_frame_stats(prev_frame.d3d11.pass.num_om_set_render_targets);
                _sgimgui_frame_stats(prev_frame.d3d11.pass.num_clear_render_target_view);
                _sgimgui_frame_stats(prev_frame.d3d11.pass.num_clear_depth_stencil_view);
                _sgimgui_frame_stats(prev_frame.d3d11.pass.num_resolve_subresource);
                _sgimgui_frame_stats(prev_frame.d3d11.pipeline.num_rs_set_state);
                _sgimgui_frame_stats(prev_frame.d3d11.pipeline.num_om_set_depth_stencil_state);
                _sgimgui_frame_stats(prev_frame.d3d11.pipeline.num_om_set_blend_state);
                _sgimgui_frame_stats(prev_frame.d3d11.pipeline.num_ia_set_primitive_topology);
                _sgimgui_frame_stats(prev_frame.d3d11.pipeline.num_ia_set_input_layout);
                _sgimgui_frame_stats(prev_frame.d3d11.pipeline.num_vs_set_shader);
                _sgimgui_frame_stats(prev_frame.d3d11.pipeline.num_vs_set_constant_buffers);
                _sgimgui_frame_stats(prev_frame.d3d11.pipeline.num_ps_set_shader);
                _sgimgui_frame_stats(prev_frame.d3d11.pipeline.num_ps_set_constant_buffers);
                _sgimgui_frame_stats(prev_frame.d3d11.pipeline.num_cs_set_shader);
                _sgimgui_frame_stats(prev_frame.d3d11.pipeline.num_cs_set_constant_buffers);
                _sgimgui_frame_stats(prev_frame.d3d11.bindings.num_ia_set_vertex_buffers);
                _sgimgui_frame_stats(prev_frame.d3d11.bindings.num_ia_set_index_buffer);
                _sgimgui_frame_stats(prev_frame.d3d11.bindings.num_vs_set_shader_resources);
                _sgimgui_frame_stats(prev_frame.d3d11.bindings.num_ps_set_shader_resources);
                _sgimgui_frame_stats(prev_frame.d3d11.bindings.num_cs_set_shader_resources);
                _sgimgui_frame_stats(prev_frame.d3d11.bindings.num_vs_set_samplers);
                _sgimgui_frame_stats(prev_frame.d3d11.bindings.num_ps_set_samplers);
                _sgimgui_frame_stats(prev_frame.d3d11.bindings.num_cs_set_samplers);
                _sgimgui_frame_stats(prev_frame.d3d11.bindings.num_cs_set_unordered_access_views);
                _sgimgui_frame_stats(prev_frame.d3d11.uniforms.num_update_subresource);
                _sgimgui_frame_stats(prev_frame.d3d11.draw.num_draw_indexed_instanced);
                _sgimgui_frame_stats(prev_frame.d3d11.draw.num_draw_indexed);
                _sgimgui_frame_stats(prev_frame.d3d11.draw.num_draw_instanced);
                _sgimgui_frame_stats(prev_frame.d3d11.draw.num_draw);
                _sgimgui_frame_stats(prev_frame.d3d11.num_map);
                _sgimgui_frame_stats(prev_frame.d3d11.num_unmap);
                break;
            case SG_BACKEND_VULKAN:
                _sgimgui_frame_stats(prev_frame.vk.num_cmd_pipeline_barrier);
                _sgimgui_frame_stats(prev_frame.vk.num_allocate_memory);
                _sgimgui_frame_stats(prev_frame.vk.num_free_memory);
                _sgimgui_frame_stats(prev_frame.vk.size_allocate_memory);
                _sgimgui_frame_stats(prev_frame.vk.num_delete_queue_added);
                _sgimgui_frame_stats(prev_frame.vk.num_delete_queue_collected);
                _sgimgui_frame_stats(prev_frame.vk.num_cmd_copy_buffer);
                _sgimgui_frame_stats(prev_frame.vk.num_cmd_copy_buffer_to_image);
                _sgimgui_frame_stats(prev_frame.vk.num_cmd_set_descriptor_buffer_offsets);
                _sgimgui_frame_stats(prev_frame.vk.size_descriptor_buffer_writes);
                break;
            default: break;
        }
        _sgimgui_frame_stats(total.buffers.alive);
        _sgimgui_frame_stats(total.buffers.free);
        _sgimgui_frame_stats(total.buffers.allocated);
        _sgimgui_frame_stats(total.buffers.deallocated);
        _sgimgui_frame_stats(total.buffers.inited);
        _sgimgui_frame_stats(total.buffers.uninited);
        _sgimgui_frame_stats(total.images.alive);
        _sgimgui_frame_stats(total.images.free);
        _sgimgui_frame_stats(total.images.allocated);
        _sgimgui_frame_stats(total.images.deallocated);
        _sgimgui_frame_stats(total.images.inited);
        _sgimgui_frame_stats(total.images.uninited);
        _sgimgui_frame_stats(total.samplers.alive);
        _sgimgui_frame_stats(total.samplers.free);
        _sgimgui_frame_stats(total.samplers.allocated);
        _sgimgui_frame_stats(total.samplers.deallocated);
        _sgimgui_frame_stats(total.samplers.inited);
        _sgimgui_frame_stats(total.samplers.uninited);
        _sgimgui_frame_stats(total.views.alive);
        _sgimgui_frame_stats(total.views.free);
        _sgimgui_frame_stats(total.views.allocated);
        _sgimgui_frame_stats(total.views.deallocated);
        _sgimgui_frame_stats(total.views.inited);
        _sgimgui_frame_stats(total.views.uninited);
        _sgimgui_frame_stats(total.pipelines.alive);
        _sgimgui_frame_stats(total.pipelines.free);
        _sgimgui_frame_stats(total.pipelines.allocated);
        _sgimgui_frame_stats(total.pipelines.deallocated);
        _sgimgui_frame_stats(total.pipelines.inited);
        _sgimgui_frame_stats(total.pipelines.uninited);
        _sgimgui_igendtable();
    }
}

#define _sgimgui_def(val, def) (((val) == 0) ? (def) : (val))

_SOKOL_PRIVATE sgimgui_desc_t _sgimgui_desc_defaults(const sgimgui_desc_t* desc) {
    SOKOL_ASSERT((desc->allocator.alloc_fn && desc->allocator.free_fn) || (!desc->allocator.alloc_fn && !desc->allocator.free_fn));
    sgimgui_desc_t res = *desc;
    // FIXME: any additional default overrides would go here
    return res;
}

/*--- PUBLIC FUNCTIONS -------------------------------------------------------*/
SOKOL_API_IMPL void sgimgui_setup(const sgimgui_desc_t* desc) {
    SOKOL_ASSERT(desc);
    _sgimgui_clear(&_sgimgui, sizeof(_sgimgui_t));
    _sgimgui.init_tag = 0xABCDABCD;
    _sgimgui.desc = _sgimgui_desc_defaults(desc);
    _sgimgui_capture_init(&_sgimgui);

    /* hook into sokol_gfx functions */
    sg_trace_hooks hooks;
    _sgimgui_clear(&hooks, sizeof(hooks));
    hooks.user_data = (void*)&_sgimgui;
    hooks.reset_state_cache = _sgimgui_reset_state_cache;
    hooks.make_buffer = _sgimgui_make_buffer;
    hooks.make_image = _sgimgui_make_image;
    hooks.make_sampler = _sgimgui_make_sampler;
    hooks.make_shader = _sgimgui_make_shader;
    hooks.make_pipeline = _sgimgui_make_pipeline;
    hooks.make_view = _sgimgui_make_view;
    hooks.destroy_buffer = _sgimgui_destroy_buffer;
    hooks.destroy_image = _sgimgui_destroy_image;
    hooks.destroy_sampler = _sgimgui_destroy_sampler;
    hooks.destroy_shader = _sgimgui_destroy_shader;
    hooks.destroy_pipeline = _sgimgui_destroy_pipeline;
    hooks.destroy_view = _sgimgui_destroy_view;
    hooks.update_buffer = _sgimgui_update_buffer;
    hooks.update_image = _sgimgui_update_image;
    hooks.append_buffer = _sgimgui_append_buffer;
    hooks.begin_pass = _sgimgui_begin_pass;
    hooks.apply_viewport = _sgimgui_apply_viewport;
    hooks.apply_scissor_rect = _sgimgui_apply_scissor_rect;
    hooks.apply_pipeline = _sgimgui_apply_pipeline;
    hooks.apply_bindings = _sgimgui_apply_bindings;
    hooks.apply_uniforms = _sgimgui_apply_uniforms;
    hooks.draw = _sgimgui_draw;
    hooks.draw_ex = _sgimgui_draw_ex;
    hooks.dispatch = _sgimgui_dispatch;
    hooks.end_pass = _sgimgui_end_pass;
    hooks.commit = _sgimgui_commit;
    hooks.alloc_buffer = _sgimgui_alloc_buffer;
    hooks.alloc_image = _sgimgui_alloc_image;
    hooks.alloc_sampler = _sgimgui_alloc_sampler;
    hooks.alloc_shader = _sgimgui_alloc_shader;
    hooks.alloc_pipeline = _sgimgui_alloc_pipeline;
    hooks.alloc_view = _sgimgui_alloc_view;
    hooks.dealloc_buffer = _sgimgui_dealloc_buffer;
    hooks.dealloc_image = _sgimgui_dealloc_image;
    hooks.dealloc_sampler = _sgimgui_dealloc_sampler;
    hooks.dealloc_shader = _sgimgui_dealloc_shader;
    hooks.dealloc_pipeline = _sgimgui_dealloc_pipeline;
    hooks.dealloc_view = _sgimgui_dealloc_view;
    hooks.init_buffer = _sgimgui_init_buffer;
    hooks.init_image = _sgimgui_init_image;
    hooks.init_sampler = _sgimgui_init_sampler;
    hooks.init_shader = _sgimgui_init_shader;
    hooks.init_pipeline = _sgimgui_init_pipeline;
    hooks.init_view = _sgimgui_init_view;
    hooks.uninit_buffer = _sgimgui_uninit_buffer;
    hooks.uninit_image = _sgimgui_uninit_image;
    hooks.uninit_sampler = _sgimgui_uninit_sampler;
    hooks.uninit_shader = _sgimgui_uninit_shader;
    hooks.uninit_pipeline = _sgimgui_uninit_pipeline;
    hooks.uninit_view = _sgimgui_uninit_view;
    hooks.fail_buffer = _sgimgui_fail_buffer;
    hooks.fail_image = _sgimgui_fail_image;
    hooks.fail_sampler = _sgimgui_fail_sampler;
    hooks.fail_shader = _sgimgui_fail_shader;
    hooks.fail_pipeline = _sgimgui_fail_pipeline;
    hooks.fail_view = _sgimgui_fail_view;
    hooks.push_debug_group = _sgimgui_push_debug_group;
    hooks.pop_debug_group = _sgimgui_pop_debug_group;
    _sgimgui.hooks = sg_install_trace_hooks(&hooks);

    /* allocate resource debug-info slots */
    const sg_desc sgdesc = sg_query_desc();
    _sgimgui.buffer_window.num_slots = sgdesc.buffer_pool_size;
    _sgimgui.image_window.num_slots = sgdesc.image_pool_size;
    _sgimgui.sampler_window.num_slots = sgdesc.sampler_pool_size;
    _sgimgui.shader_window.num_slots = sgdesc.shader_pool_size;
    _sgimgui.pipeline_window.num_slots = sgdesc.pipeline_pool_size;
    _sgimgui.view_window.num_slots = sgdesc.view_pool_size;

    const size_t buffer_pool_size = (size_t)_sgimgui.buffer_window.num_slots * sizeof(_sgimgui_buffer_t);
    _sgimgui.buffer_window.slots = (_sgimgui_buffer_t*) _sgimgui_malloc_clear(&_sgimgui.desc.allocator, buffer_pool_size);

    const size_t image_pool_size = (size_t)_sgimgui.image_window.num_slots * sizeof(_sgimgui_image_t);
    _sgimgui.image_window.slots = (_sgimgui_image_t*) _sgimgui_malloc_clear(&_sgimgui.desc.allocator, image_pool_size);

    const size_t sampler_pool_size = (size_t)_sgimgui.sampler_window.num_slots * sizeof(_sgimgui_sampler_t);
    _sgimgui.sampler_window.slots = (_sgimgui_sampler_t*) _sgimgui_malloc_clear(&_sgimgui.desc.allocator, sampler_pool_size);

    const size_t shader_pool_size = (size_t)_sgimgui.shader_window.num_slots * sizeof(_sgimgui_shader_t);
    _sgimgui.shader_window.slots = (_sgimgui_shader_t*) _sgimgui_malloc_clear(&_sgimgui.desc.allocator, shader_pool_size);

    const size_t pipeline_pool_size = (size_t)_sgimgui.pipeline_window.num_slots * sizeof(_sgimgui_pipeline_t);
    _sgimgui.pipeline_window.slots = (_sgimgui_pipeline_t*) _sgimgui_malloc_clear(&_sgimgui.desc.allocator, pipeline_pool_size);

    const size_t view_pool_size = (size_t)_sgimgui.view_window.num_slots * sizeof(_sgimgui_view_t);
    _sgimgui.view_window.slots = (_sgimgui_view_t*) _sgimgui_malloc_clear(&_sgimgui.desc.allocator, view_pool_size);
}

SOKOL_API_IMPL void sgimgui_shutdown(void) {
    SOKOL_ASSERT(_sgimgui.init_tag == 0xABCDABCD);
    /* restore original trace hooks */
    sg_install_trace_hooks(&_sgimgui.hooks);
    _sgimgui.init_tag = 0;
    _sgimgui_capture_discard(&_sgimgui);
    if (_sgimgui.buffer_window.slots) {
        for (int i = 0; i < _sgimgui.buffer_window.num_slots; i++) {
            if (_sgimgui.buffer_window.slots[i].res_id.id != SG_INVALID_ID) {
                _sgimgui_buffer_destroyed(&_sgimgui, i);
            }
        }
        _sgimgui_free(&_sgimgui.desc.allocator, (void*)_sgimgui.buffer_window.slots);
        _sgimgui.buffer_window.slots = 0;
    }
    if (_sgimgui.image_window.slots) {
        for (int i = 0; i < _sgimgui.image_window.num_slots; i++) {
            if (_sgimgui.image_window.slots[i].res_id.id != SG_INVALID_ID) {
                _sgimgui_image_destroyed(&_sgimgui, i);
            }
        }
        _sgimgui_free(&_sgimgui.desc.allocator, (void*)_sgimgui.image_window.slots);
        _sgimgui.image_window.slots = 0;
    }
    if (_sgimgui.sampler_window.slots) {
        for (int i = 0; i < _sgimgui.sampler_window.num_slots; i++) {
            if (_sgimgui.sampler_window.slots[i].res_id.id != SG_INVALID_ID) {
                _sgimgui_sampler_destroyed(&_sgimgui, i);
            }
        }
        _sgimgui_free(&_sgimgui.desc.allocator, (void*)_sgimgui.sampler_window.slots);
        _sgimgui.sampler_window.slots = 0;
    }
    if (_sgimgui.shader_window.slots) {
        for (int i = 0; i < _sgimgui.shader_window.num_slots; i++) {
            if (_sgimgui.shader_window.slots[i].res_id.id != SG_INVALID_ID) {
                _sgimgui_shader_destroyed(&_sgimgui, i);
            }
        }
        _sgimgui_free(&_sgimgui.desc.allocator, (void*)_sgimgui.shader_window.slots);
        _sgimgui.shader_window.slots = 0;
    }
    if (_sgimgui.pipeline_window.slots) {
        for (int i = 0; i < _sgimgui.pipeline_window.num_slots; i++) {
            if (_sgimgui.pipeline_window.slots[i].res_id.id != SG_INVALID_ID) {
                _sgimgui_pipeline_destroyed(&_sgimgui, i);
            }
        }
        _sgimgui_free(&_sgimgui.desc.allocator, (void*)_sgimgui.pipeline_window.slots);
        _sgimgui.pipeline_window.slots = 0;
    }
    if (_sgimgui.view_window.slots) {
        for (int i = 0; i < _sgimgui.view_window.num_slots; i++) {
            if (_sgimgui.view_window.slots[i].res_id.id != SG_INVALID_ID) {
                _sgimgui_view_destroyed(&_sgimgui, i);
            }
        }
        _sgimgui_free(&_sgimgui.desc.allocator, (void*)_sgimgui.view_window.slots);
        _sgimgui.view_window.slots = 0;
    }
}

SOKOL_API_IMPL void sgimgui_draw(void) {
    sgimgui_draw_buffer_window("Buffers");
    sgimgui_draw_image_window("Images");
    sgimgui_draw_sampler_window("Samplers");
    sgimgui_draw_shader_window("Shaders");
    sgimgui_draw_pipeline_window("Pipelines");
    sgimgui_draw_view_window("Views");
    sgimgui_draw_capture_window("Frame Capture");
    sgimgui_draw_capabilities_window("Capabilities");
    sgimgui_draw_frame_stats_window("Frame Stats");
}

SOKOL_API_IMPL void sgimgui_draw_menu(const char* title) {
    SOKOL_ASSERT(_sgimgui.init_tag == 0xABCDABCD);
    SOKOL_ASSERT(title);
    if (_sgimgui_igbeginmenu(title)) {
        sgimgui_draw_capabilities_menu_item("Capabilities");
        sgimgui_draw_frame_stats_menu_item("Frame Stats");
        sgimgui_draw_buffer_menu_item("Buffers");
        sgimgui_draw_image_menu_item("Images");
        sgimgui_draw_view_menu_item("Views");
        sgimgui_draw_sampler_menu_item("Samplers");
        sgimgui_draw_shader_menu_item("Shaders");
        sgimgui_draw_pipeline_menu_item("Pipelines");
        sgimgui_draw_capture_menu_item("Calls");
        _sgimgui_igendmenu();
    }
}

SOKOL_API_IMPL void sgimgui_draw_buffer_menu_item(const char* label) {
    SOKOL_ASSERT(_sgimgui.init_tag == 0xABCDABCD);
    SOKOL_ASSERT(label);
    _sgimgui_igmenuitemboolptr(label, 0, &_sgimgui.buffer_window.open, true);
}

SOKOL_API_IMPL void sgimgui_draw_image_menu_item(const char* label) {
    SOKOL_ASSERT(_sgimgui.init_tag == 0xABCDABCD);
    SOKOL_ASSERT(label);
    _sgimgui_igmenuitemboolptr(label, 0, &_sgimgui.image_window.open, true);
}

SOKOL_API_IMPL void sgimgui_draw_sampler_menu_item(const char* label) {
    SOKOL_ASSERT(_sgimgui.init_tag == 0xABCDABCD);
    SOKOL_ASSERT(label);
    _sgimgui_igmenuitemboolptr(label, 0, &_sgimgui.sampler_window.open, true);
}

SOKOL_API_IMPL void sgimgui_draw_shader_menu_item(const char* label) {
    SOKOL_ASSERT(_sgimgui.init_tag == 0xABCDABCD);
    SOKOL_ASSERT(label);
    _sgimgui_igmenuitemboolptr(label, 0, &_sgimgui.shader_window.open, true);
}

SOKOL_API_IMPL void sgimgui_draw_pipeline_menu_item(const char* label) {
    SOKOL_ASSERT(_sgimgui.init_tag == 0xABCDABCD);
    SOKOL_ASSERT(label);
    _sgimgui_igmenuitemboolptr(label, 0, &_sgimgui.pipeline_window.open, true);
}

SOKOL_API_IMPL void sgimgui_draw_view_menu_item(const char* label) {
    SOKOL_ASSERT(_sgimgui.init_tag == 0xABCDABCD);
    SOKOL_ASSERT(label);
    _sgimgui_igmenuitemboolptr(label, 0, &_sgimgui.view_window.open, true);
}

SOKOL_API_IMPL void sgimgui_draw_capture_menu_item(const char* label) {
    SOKOL_ASSERT(_sgimgui.init_tag == 0xABCDABCD);
    SOKOL_ASSERT(label);
    _sgimgui_igmenuitemboolptr(label, 0, &_sgimgui.capture_window.open, true);
}

SOKOL_API_IMPL void sgimgui_draw_capabilities_menu_item(const char* label) {
    SOKOL_ASSERT(_sgimgui.init_tag == 0xABCDABCD);
    SOKOL_ASSERT(label);
    _sgimgui_igmenuitemboolptr(label, 0, &_sgimgui.caps_window.open, true);
}

SOKOL_API_IMPL void sgimgui_draw_frame_stats_menu_item(const char* label) {
    SOKOL_ASSERT(_sgimgui.init_tag == 0xABCDABCD);
    SOKOL_ASSERT(label);
    _sgimgui_igmenuitemboolptr(label, 0, &_sgimgui.frame_stats_window.open, true);
}

SOKOL_API_IMPL void sgimgui_draw_buffer_window(const char* title) {
    SOKOL_ASSERT(_sgimgui.init_tag == 0xABCDABCD);
    SOKOL_ASSERT(title);
    if (!_sgimgui.buffer_window.open) {
        return;
    }
    _sgimgui_igsetnextwindowsize(IMVEC2(440, 280), ImGuiCond_Once);
    if (_sgimgui_igbegin(title, &_sgimgui.buffer_window.open, 0)) {
        sgimgui_draw_buffer_window_content();
    }
    _sgimgui_igend();
}

SOKOL_API_IMPL void sgimgui_draw_image_window(const char* title) {
    SOKOL_ASSERT(_sgimgui.init_tag == 0xABCDABCD);
    SOKOL_ASSERT(title);
    if (!_sgimgui.image_window.open) {
        return;
    }
    _sgimgui_igsetnextwindowsize(IMVEC2(440, 400), ImGuiCond_Once);
    if (_sgimgui_igbegin(title, &_sgimgui.image_window.open, 0)) {
        sgimgui_draw_image_window_content();
    }
    _sgimgui_igend();
}

SOKOL_API_IMPL void sgimgui_draw_sampler_window(const char* title) {
    SOKOL_ASSERT(_sgimgui.init_tag == 0xABCDABCD);
    SOKOL_ASSERT(title);
    if (!_sgimgui.sampler_window.open) {
        return;
    }
    _sgimgui_igsetnextwindowsize(IMVEC2(440, 400), ImGuiCond_Once);
    if (_sgimgui_igbegin(title, &_sgimgui.sampler_window.open, 0)) {
        sgimgui_draw_sampler_window_content();
    }
    _sgimgui_igend();
}

SOKOL_API_IMPL void sgimgui_draw_shader_window(const char* title) {
    SOKOL_ASSERT(_sgimgui.init_tag == 0xABCDABCD);
    SOKOL_ASSERT(title);
    if (!_sgimgui.shader_window.open) {
        return;
    }
    _sgimgui_igsetnextwindowsize(IMVEC2(440, 400), ImGuiCond_Once);
    if (_sgimgui_igbegin(title, &_sgimgui.shader_window.open, 0)) {
        sgimgui_draw_shader_window_content();
    }
    _sgimgui_igend();
}

SOKOL_API_IMPL void sgimgui_draw_pipeline_window(const char* title) {
    SOKOL_ASSERT(_sgimgui.init_tag == 0xABCDABCD);
    SOKOL_ASSERT(title);
    if (!_sgimgui.pipeline_window.open) {
        return;
    }
    _sgimgui_igsetnextwindowsize(IMVEC2(540, 400), ImGuiCond_Once);
    if (_sgimgui_igbegin(title, &_sgimgui.pipeline_window.open, 0)) {
        sgimgui_draw_pipeline_window_content();
    }
    _sgimgui_igend();
}

SOKOL_API_IMPL void sgimgui_draw_view_window(const char* title) {
    SOKOL_ASSERT(_sgimgui.init_tag == 0xABCDABCD);
    SOKOL_ASSERT(title);
    if (!_sgimgui.view_window.open) {
        return;
    }
    _sgimgui_igsetnextwindowsize(IMVEC2(440, 400), ImGuiCond_Once);
    if (_sgimgui_igbegin(title, &_sgimgui.view_window.open, 0)) {
        sgimgui_draw_view_window_content();
    }
    _sgimgui_igend();
}

SOKOL_API_IMPL void sgimgui_draw_capture_window(const char* title) {
    SOKOL_ASSERT(_sgimgui.init_tag == 0xABCDABCD);
    SOKOL_ASSERT(title);
    if (!_sgimgui.capture_window.open) {
        return;
    }
    _sgimgui_igsetnextwindowsize(IMVEC2(640, 400), ImGuiCond_Once);
    if (_sgimgui_igbegin(title, &_sgimgui.capture_window.open, 0)) {
        sgimgui_draw_capture_window_content();
    }
    _sgimgui_igend();
}

SOKOL_API_IMPL void sgimgui_draw_capabilities_window(const char* title) {
    SOKOL_ASSERT(_sgimgui.init_tag == 0xABCDABCD);
    SOKOL_ASSERT(title);
    if (!_sgimgui.caps_window.open) {
        return;
    }
    _sgimgui_igsetnextwindowsize(IMVEC2(440, 400), ImGuiCond_Once);
    if (_sgimgui_igbegin(title, &_sgimgui.caps_window.open, 0)) {
        sgimgui_draw_capabilities_window_content();
    }
    _sgimgui_igend();
}

SOKOL_API_IMPL void sgimgui_draw_frame_stats_window(const char* title) {
    SOKOL_ASSERT(_sgimgui.init_tag == 0xABCDABCD);
    SOKOL_ASSERT(title);
    if (!_sgimgui.frame_stats_window.open) {
        return;
    }
    _sgimgui_igsetnextwindowsize(IMVEC2(640, 400), ImGuiCond_Once);
    if (_sgimgui_igbegin(title, &_sgimgui.frame_stats_window.open, 0)) {
        sgimgui_draw_frame_stats_window_content();
    }
    _sgimgui_igend();
}

SOKOL_API_IMPL void sgimgui_draw_buffer_window_content(void) {
    SOKOL_ASSERT(_sgimgui.init_tag == 0xABCDABCD);
    _sgimgui_draw_buffer_list(&_sgimgui);
    _sgimgui_igsameline();
    _sgimgui_draw_buffer_panel(&_sgimgui, _sgimgui.buffer_window.sel_buf);
}

SOKOL_API_IMPL void sgimgui_draw_image_window_content(void) {
    SOKOL_ASSERT(_sgimgui.init_tag == 0xABCDABCD);
    _sgimgui_draw_image_list(&_sgimgui);
    _sgimgui_igsameline();
    _sgimgui_draw_image_panel(&_sgimgui, _sgimgui.image_window.sel_img);
}

SOKOL_API_IMPL void sgimgui_draw_sampler_window_content(void) {
    SOKOL_ASSERT(_sgimgui.init_tag == 0xABCDABCD);
    _sgimgui_draw_sampler_list(&_sgimgui);
    _sgimgui_igsameline();
    _sgimgui_draw_sampler_panel(&_sgimgui, _sgimgui.sampler_window.sel_smp);
}

SOKOL_API_IMPL void sgimgui_draw_shader_window_content(void) {
    SOKOL_ASSERT(_sgimgui.init_tag == 0xABCDABCD);
    _sgimgui_draw_shader_list(&_sgimgui);
    _sgimgui_igsameline();
    _sgimgui_draw_shader_panel(&_sgimgui, _sgimgui.shader_window.sel_shd);
}

SOKOL_API_IMPL void sgimgui_draw_pipeline_window_content(void) {
    SOKOL_ASSERT(_sgimgui.init_tag == 0xABCDABCD);
    _sgimgui_draw_pipeline_list(&_sgimgui);
    _sgimgui_igsameline();
    _sgimgui_draw_pipeline_panel(&_sgimgui, _sgimgui.pipeline_window.sel_pip);
}

SOKOL_API_IMPL void sgimgui_draw_view_window_content(void) {
    SOKOL_ASSERT(_sgimgui.init_tag == 0xABCDABCD);
    _sgimgui_draw_view_list(&_sgimgui);
    _sgimgui_igsameline();
    _sgimgui_draw_view_panel(&_sgimgui, _sgimgui.view_window.sel_view);
}

SOKOL_API_IMPL void sgimgui_draw_capture_window_content(void) {
    SOKOL_ASSERT(_sgimgui.init_tag == 0xABCDABCD);
    _sgimgui_draw_capture_list(&_sgimgui);
    _sgimgui_igsameline();
    _sgimgui_draw_capture_panel(&_sgimgui);
}

SOKOL_API_IMPL void sgimgui_draw_capabilities_window_content(void) {
    SOKOL_ASSERT(_sgimgui.init_tag == 0xABCDABCD);
    _sgimgui_draw_caps_panel();
}

SOKOL_API_IMPL void sgimgui_draw_frame_stats_window_content(void) {
    SOKOL_ASSERT(_sgimgui.init_tag == 0xABCDABCD);
    _sgimgui.frame_stats_window.stats = sg_query_stats();
    _sgimgui_draw_frame_stats_panel(&_sgimgui);
}

#endif /* SOKOL_GFX_IMGUI_IMPL */
