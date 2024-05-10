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
        SOKOL_GFX_IMGUI_API_DECL      - public function declaration prefix (default: extern)
        SOKOL_API_DECL      - same as SOKOL_GFX_IMGUI_API_DECL
        SOKOL_API_IMPL      - public function implementation prefix (default: -)

    If sokol_gfx_imgui.h is compiled as a DLL, define the following before
    including the declaration or implementation:

    SOKOL_DLL

    On Windows, SOKOL_DLL will define SOKOL_GFX_IMGUI_API_DECL as __declspec(dllexport)
    or __declspec(dllimport) as needed.

    STEP BY STEP:
    =============
    --- create an sgimgui_t struct (which must be preserved between frames)
        and initialize it with:

            sgimgui_init(&sgimgui, &(sgimgui_desc_t){ 0 });

        Note that from C++ you can't inline the desc structure initialization:

            const sgimgui_desc_t desc = { };
            sgimgui_init(&sgimgui, &desc);

        Provide optional memory allocator override functions (compatible with malloc/free) like this:

            sgimgui_init(&sgimgui, &(sgimgui_desc_t){
                .allocator = {
                    .alloc_fn = my_malloc,
                    .free_fn = my_free,
                }
            });

    --- somewhere in the per-frame code call:

            sgimgui_draw(&sgimgui)

        this won't draw anything yet, since no windows are open.

    --- call the convenience function sgimgui_draw_menu(ctx, title)
        to render a menu which allows to open/close the provided debug windows

            sgimgui_draw_menu(&sgimgui, "sokol-gfx");

    --- alternative, open and close windows directly by setting the following public
        booleans in the sgimgui_t struct:

            sgimgui.caps_window.open = true;
            sgimgui.frame_stats_window.open = true;
            sgimgui.buffer_window.open = true;
            sgimgui.image_window.open = true;
            sgimgui.sampler_window.open = true;
            sgimgui.shader_window.open = true;
            sgimgui.pipeline_window.open = true;
            sgimgui.attachments_window.open = true;
            sgimgui.capture_window.open = true;
            sgimgui.frame_stats_window.open = true;

        ...for instance, to control the window visibility through
        menu items, the following code can be used:

            if (ImGui::BeginMainMenuBar()) {
                if (ImGui::BeginMenu("sokol-gfx")) {
                    ImGui::MenuItem("Capabilities", 0, &sgimgui.caps_window.open);
                    ImGui::MenuItem("Frame Stats", 0, &sgimgui.frame_stats_window.open);
                    ImGui::MenuItem("Buffers", 0, &sgimgui.buffer_window.open);
                    ImGui::MenuItem("Images", 0, &sgimgui.image_window.open);
                    ImGui::MenuItem("Samplers", 0, &sgimgui.sampler_window.open);
                    ImGui::MenuItem("Shaders", 0, &sgimgui.shader_window.open);
                    ImGui::MenuItem("Pipelines", 0, &sgimgui.pipeline_window.open);
                    ImGui::MenuItem("Attachments", 0, &sgimgui.attachments_window.open);
                    ImGui::MenuItem("Calls", 0, &sgimgui.capture_window.open);
                    ImGui::EndMenu();
                }
                ImGui::EndMainMenuBar();
            }

    --- before application shutdown, call:

            sgimgui_discard(&sgimgui);

        ...this is not strictly necessary because the application exits
        anyway, but not doing this may trigger memory leak detection tools.

    --- finally, your application needs an ImGui renderer, you can either
        provide your own, or drop in the sokol_imgui.h utility header

    ALTERNATIVE DRAWING FUNCTIONS:
    ==============================
    Instead of the convenient, but all-in-one sgimgui_draw() function,
    you can also use the following granular functions which might allow
    better integration with your existing UI:

    The following functions only render the window *content* (so you
    can integrate the UI into you own windows):

        void sgimgui_draw_buffer_window_content(sgimgui_t* ctx);
        void sgimgui_draw_image_window_content(sgimgui_t* ctx);
        void sgimgui_draw_sampler_window_content(sgimgui_t* ctx);
        void sgimgui_draw_shader_window_content(sgimgui_t* ctx);
        void sgimgui_draw_pipeline_window_content(sgimgui_t* ctx);
        void sgimgui_draw_attachments_window_content(sgimgui_t* ctx);
        void sgimgui_draw_capture_window_content(sgimgui_t* ctx);

    And these are the 'full window' drawing functions:

        void sgimgui_draw_buffer_window(sgimgui_t* ctx);
        void sgimgui_draw_image_window(sgimgui_t* ctx);
        void sgimgui_draw_sampler_window(sgimgui_t* ctx);
        void sgimgui_draw_shader_window(sgimgui_t* ctx);
        void sgimgui_draw_pipeline_window(sgimgui_t* ctx);
        void sgimgui_draw_attachments_window(sgimgui_t* ctx);
        void sgimgui_draw_capture_window(sgimgui_t* ctx);

    Finer-grained drawing functions may be moved to the public API
    in the future as needed.

    MEMORY ALLOCATION OVERRIDE
    ==========================
    You can override the memory allocation functions at initialization time
    like this:

        void* my_alloc(size_t size, void* user_data) {
            return malloc(size);
        }

        void my_free(void* ptr, void* user_data) {
            free(ptr);
        }

        ...
            sgimgui_init(&(&ctx, &(sgimgui_desc_t){
                // ...
                .allocator = {
                    .alloc_fn = my_alloc,
                    .free_fn = my_free,
                    .user_data = ...;
                }
            });
        ...

    This only affects memory allocation calls done by sokol_gfx_imgui.h
    itself though, not any allocations in OS libraries.


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

#define sgimgui_STRBUF_LEN (96)
/* max number of captured calls per frame */
#define sgimgui_MAX_FRAMECAPTURE_ITEMS (4096)

typedef struct sgimgui_str_t {
    char buf[sgimgui_STRBUF_LEN];
} sgimgui_str_t;

typedef struct sgimgui_buffer_t {
    sg_buffer res_id;
    sgimgui_str_t label;
    sg_buffer_desc desc;
} sgimgui_buffer_t;

typedef struct sgimgui_image_t {
    sg_image res_id;
    float ui_scale;
    sgimgui_str_t label;
    sg_image_desc desc;
    simgui_image_t simgui_img;
} sgimgui_image_t;

typedef struct sgimgui_sampler_t {
    sg_sampler res_id;
    sgimgui_str_t label;
    sg_sampler_desc desc;
} sgimgui_sampler_t;

typedef struct sgimgui_shader_t {
    sg_shader res_id;
    sgimgui_str_t label;
    sgimgui_str_t vs_entry;
    sgimgui_str_t vs_d3d11_target;
    sgimgui_str_t vs_image_sampler_name[SG_MAX_SHADERSTAGE_IMAGESAMPLERPAIRS];
    sgimgui_str_t vs_uniform_name[SG_MAX_SHADERSTAGE_UBS][SG_MAX_UB_MEMBERS];
    sgimgui_str_t fs_entry;
    sgimgui_str_t fs_d3d11_target;
    sgimgui_str_t fs_image_sampler_name[SG_MAX_SHADERSTAGE_IMAGESAMPLERPAIRS];
    sgimgui_str_t fs_uniform_name[SG_MAX_SHADERSTAGE_UBS][SG_MAX_UB_MEMBERS];
    sgimgui_str_t attr_name[SG_MAX_VERTEX_ATTRIBUTES];
    sgimgui_str_t attr_sem_name[SG_MAX_VERTEX_ATTRIBUTES];
    sg_shader_desc desc;
} sgimgui_shader_t;

typedef struct sgimgui_pipeline_t {
    sg_pipeline res_id;
    sgimgui_str_t label;
    sg_pipeline_desc desc;
} sgimgui_pipeline_t;

typedef struct sgimgui_attachments_t {
    sg_attachments res_id;
    sgimgui_str_t label;
    float color_image_scale[SG_MAX_COLOR_ATTACHMENTS];
    float resolve_image_scale[SG_MAX_COLOR_ATTACHMENTS];
    float ds_image_scale;
    sg_attachments_desc desc;
} sgimgui_attachments_t;

typedef struct sgimgui_buffer_window_t {
    bool open;
    int num_slots;
    sg_buffer sel_buf;
    sgimgui_buffer_t* slots;
} sgimgui_buffer_window_t;

typedef struct sgimgui_image_window_t {
    bool open;
    int num_slots;
    sg_image sel_img;
    sgimgui_image_t* slots;
} sgimgui_image_window_t;

typedef struct sgimgui_sampler_window_t {
    bool open;
    int num_slots;
    sg_sampler sel_smp;
    sgimgui_sampler_t* slots;
} sgimgui_sampler_window_t;

typedef struct sgimgui_shader_window_t {
    bool open;
    int num_slots;
    sg_shader sel_shd;
    sgimgui_shader_t* slots;
} sgimgui_shader_window_t;

typedef struct sgimgui_pipeline_window_t {
    bool open;
    int num_slots;
    sg_pipeline sel_pip;
    sgimgui_pipeline_t* slots;
} sgimgui_pipeline_window_t;

typedef struct sgimgui_attachments_window_t {
    bool open;
    int num_slots;
    sg_attachments sel_atts;
    sgimgui_attachments_t* slots;
} sgimgui_attachments_window_t;

typedef enum sgimgui_cmd_t {
    SGIMGUI_CMD_INVALID,
    SGIMGUI_CMD_RESET_STATE_CACHE,
    SGIMGUI_CMD_MAKE_BUFFER,
    SGIMGUI_CMD_MAKE_IMAGE,
    SGIMGUI_CMD_MAKE_SAMPLER,
    SGIMGUI_CMD_MAKE_SHADER,
    SGIMGUI_CMD_MAKE_PIPELINE,
    SGIMGUI_CMD_MAKE_ATTACHMENTS,
    SGIMGUI_CMD_DESTROY_BUFFER,
    SGIMGUI_CMD_DESTROY_IMAGE,
    SGIMGUI_CMD_DESTROY_SAMPLER,
    SGIMGUI_CMD_DESTROY_SHADER,
    SGIMGUI_CMD_DESTROY_PIPELINE,
    SGIMGUI_CMD_DESTROY_ATTACHMENTS,
    SGIMGUI_CMD_UPDATE_BUFFER,
    SGIMGUI_CMD_UPDATE_IMAGE,
    SGIMGUI_CMD_APPEND_BUFFER,
    SGIMGUI_CMD_BEGIN_PASS,
    SGIMGUI_CMD_APPLY_VIEWPORT,
    SGIMGUI_CMD_APPLY_SCISSOR_RECT,
    SGIMGUI_CMD_APPLY_PIPELINE,
    SGIMGUI_CMD_APPLY_BINDINGS,
    SGIMGUI_CMD_APPLY_UNIFORMS,
    SGIMGUI_CMD_DRAW,
    SGIMGUI_CMD_END_PASS,
    SGIMGUI_CMD_COMMIT,
    SGIMGUI_CMD_ALLOC_BUFFER,
    SGIMGUI_CMD_ALLOC_IMAGE,
    SGIMGUI_CMD_ALLOC_SAMPLER,
    SGIMGUI_CMD_ALLOC_SHADER,
    SGIMGUI_CMD_ALLOC_PIPELINE,
    SGIMGUI_CMD_ALLOC_ATTACHMENTS,
    SGIMGUI_CMD_DEALLOC_BUFFER,
    SGIMGUI_CMD_DEALLOC_IMAGE,
    SGIMGUI_CMD_DEALLOC_SAMPLER,
    SGIMGUI_CMD_DEALLOC_SHADER,
    SGIMGUI_CMD_DEALLOC_PIPELINE,
    SGIMGUI_CMD_DEALLOC_ATTACHMENTS,
    SGIMGUI_CMD_INIT_BUFFER,
    SGIMGUI_CMD_INIT_IMAGE,
    SGIMGUI_CMD_INIT_SAMPLER,
    SGIMGUI_CMD_INIT_SHADER,
    SGIMGUI_CMD_INIT_PIPELINE,
    SGIMGUI_CMD_INIT_ATTACHMENTS,
    SGIMGUI_CMD_UNINIT_BUFFER,
    SGIMGUI_CMD_UNINIT_IMAGE,
    SGIMGUI_CMD_UNINIT_SAMPLER,
    SGIMGUI_CMD_UNINIT_SHADER,
    SGIMGUI_CMD_UNINIT_PIPELINE,
    SGIMGUI_CMD_UNINIT_ATTACHMENTS,
    SGIMGUI_CMD_FAIL_BUFFER,
    SGIMGUI_CMD_FAIL_IMAGE,
    SGIMGUI_CMD_FAIL_SAMPLER,
    SGIMGUI_CMD_FAIL_SHADER,
    SGIMGUI_CMD_FAIL_PIPELINE,
    SGIMGUI_CMD_FAIL_ATTACHMENTS,
    SGIMGUI_CMD_PUSH_DEBUG_GROUP,
    SGIMGUI_CMD_POP_DEBUG_GROUP,
} sgimgui_cmd_t;

typedef struct sgimgui_args_make_buffer_t {
    sg_buffer result;
} sgimgui_args_make_buffer_t;

typedef struct sgimgui_args_make_image_t {
    sg_image result;
} sgimgui_args_make_image_t;

typedef struct sgimgui_args_make_sampler_t {
    sg_sampler result;
} sgimgui_args_make_sampler_t;

typedef struct sgimgui_args_make_shader_t {
    sg_shader result;
} sgimgui_args_make_shader_t;

typedef struct sgimgui_args_make_pipeline_t {
    sg_pipeline result;
} sgimgui_args_make_pipeline_t;

typedef struct sgimgui_args_make_attachments_t {
    sg_attachments result;
} sgimgui_args_make_attachments_t;

typedef struct sgimgui_args_destroy_buffer_t {
    sg_buffer buffer;
} sgimgui_args_destroy_buffer_t;

typedef struct sgimgui_args_destroy_image_t {
    sg_image image;
} sgimgui_args_destroy_image_t;

typedef struct sgimgui_args_destroy_sampler_t {
    sg_sampler sampler;
} sgimgui_args_destroy_sampler_t;

typedef struct sgimgui_args_destroy_shader_t {
    sg_shader shader;
} sgimgui_args_destroy_shader_t;

typedef struct sgimgui_args_destroy_pipeline_t {
    sg_pipeline pipeline;
} sgimgui_args_destroy_pipeline_t;

typedef struct sgimgui_args_destroy_attachments_t {
    sg_attachments attachments;
} sgimgui_args_destroy_attachments_t;

typedef struct sgimgui_args_update_buffer_t {
    sg_buffer buffer;
    size_t data_size;
} sgimgui_args_update_buffer_t;

typedef struct sgimgui_args_update_image_t {
    sg_image image;
} sgimgui_args_update_image_t;

typedef struct sgimgui_args_append_buffer_t {
    sg_buffer buffer;
    size_t data_size;
    int result;
} sgimgui_args_append_buffer_t;

typedef struct sgimgui_args_begin_pass_t {
    sg_pass pass;
} sgimgui_args_begin_pass_t;

typedef struct sgimgui_args_apply_viewport_t {
    int x, y, width, height;
    bool origin_top_left;
} sgimgui_args_apply_viewport_t;

typedef struct sgimgui_args_apply_scissor_rect_t {
    int x, y, width, height;
    bool origin_top_left;
} sgimgui_args_apply_scissor_rect_t;

typedef struct sgimgui_args_apply_pipeline_t {
    sg_pipeline pipeline;
} sgimgui_args_apply_pipeline_t;

typedef struct sgimgui_args_apply_bindings_t {
    sg_bindings bindings;
} sgimgui_args_apply_bindings_t;

typedef struct sgimgui_args_apply_uniforms_t {
    sg_shader_stage stage;
    int ub_index;
    size_t data_size;
    sg_pipeline pipeline;   /* the pipeline which was active at this call */
    size_t ubuf_pos;        /* start of copied data in capture buffer */
} sgimgui_args_apply_uniforms_t;

typedef struct sgimgui_args_draw_t {
    int base_element;
    int num_elements;
    int num_instances;
} sgimgui_args_draw_t;

typedef struct sgimgui_args_alloc_buffer_t {
    sg_buffer result;
} sgimgui_args_alloc_buffer_t;

typedef struct sgimgui_args_alloc_image_t {
    sg_image result;
} sgimgui_args_alloc_image_t;

typedef struct sgimgui_args_alloc_sampler_t {
    sg_sampler result;
} sgimgui_args_alloc_sampler_t;

typedef struct sgimgui_args_alloc_shader_t {
    sg_shader result;
} sgimgui_args_alloc_shader_t;

typedef struct sgimgui_args_alloc_pipeline_t {
    sg_pipeline result;
} sgimgui_args_alloc_pipeline_t;

typedef struct sgimgui_args_alloc_attachments_t {
    sg_attachments result;
} sgimgui_args_alloc_attachments_t;

typedef struct sgimgui_args_dealloc_buffer_t {
    sg_buffer buffer;
} sgimgui_args_dealloc_buffer_t;

typedef struct sgimgui_args_dealloc_image_t {
    sg_image image;
} sgimgui_args_dealloc_image_t;

typedef struct sgimgui_args_dealloc_sampler_t {
    sg_sampler sampler;
} sgimgui_args_dealloc_sampler_t;

typedef struct sgimgui_args_dealloc_shader_t {
    sg_shader shader;
} sgimgui_args_dealloc_shader_t;

typedef struct sgimgui_args_dealloc_pipeline_t {
    sg_pipeline pipeline;
} sgimgui_args_dealloc_pipeline_t;

typedef struct sgimgui_args_dealloc_attachments_t {
    sg_attachments attachments;
} sgimgui_args_dealloc_attachments_t;

typedef struct sgimgui_args_init_buffer_t {
    sg_buffer buffer;
} sgimgui_args_init_buffer_t;

typedef struct sgimgui_args_init_image_t {
    sg_image image;
} sgimgui_args_init_image_t;

typedef struct sgimgui_args_init_sampler_t {
    sg_sampler sampler;
} sgimgui_args_init_sampler_t;

typedef struct sgimgui_args_init_shader_t {
    sg_shader shader;
} sgimgui_args_init_shader_t;

typedef struct sgimgui_args_init_pipeline_t {
    sg_pipeline pipeline;
} sgimgui_args_init_pipeline_t;

typedef struct sgimgui_args_init_attachments_t {
    sg_attachments attachments;
} sgimgui_args_init_attachments_t;

typedef struct sgimgui_args_uninit_buffer_t {
    sg_buffer buffer;
} sgimgui_args_uninit_buffer_t;

typedef struct sgimgui_args_uninit_image_t {
    sg_image image;
} sgimgui_args_uninit_image_t;

typedef struct sgimgui_args_uninit_sampler_t {
    sg_sampler sampler;
} sgimgui_args_uninit_sampler_t;

typedef struct sgimgui_args_uninit_shader_t {
    sg_shader shader;
} sgimgui_args_uninit_shader_t;

typedef struct sgimgui_args_uninit_pipeline_t {
    sg_pipeline pipeline;
} sgimgui_args_uninit_pipeline_t;

typedef struct sgimgui_args_uninit_attachments_t {
    sg_attachments attachments;
} sgimgui_args_uninit_attachments_t;

typedef struct sgimgui_args_fail_buffer_t {
    sg_buffer buffer;
} sgimgui_args_fail_buffer_t;

typedef struct sgimgui_args_fail_image_t {
    sg_image image;
} sgimgui_args_fail_image_t;

typedef struct sgimgui_args_fail_sampler_t {
    sg_sampler sampler;
} sgimgui_args_fail_sampler_t;

typedef struct sgimgui_args_fail_shader_t {
    sg_shader shader;
} sgimgui_args_fail_shader_t;

typedef struct sgimgui_args_fail_pipeline_t {
    sg_pipeline pipeline;
} sgimgui_args_fail_pipeline_t;

typedef struct sgimgui_args_fail_attachments_t {
    sg_attachments attachments;
} sgimgui_args_fail_attachments_t;

typedef struct sgimgui_args_push_debug_group_t {
    sgimgui_str_t name;
} sgimgui_args_push_debug_group_t;

typedef union sgimgui_args_t {
    sgimgui_args_make_buffer_t make_buffer;
    sgimgui_args_make_image_t make_image;
    sgimgui_args_make_sampler_t make_sampler;
    sgimgui_args_make_shader_t make_shader;
    sgimgui_args_make_pipeline_t make_pipeline;
    sgimgui_args_make_attachments_t make_attachments;
    sgimgui_args_destroy_buffer_t destroy_buffer;
    sgimgui_args_destroy_image_t destroy_image;
    sgimgui_args_destroy_sampler_t destroy_sampler;
    sgimgui_args_destroy_shader_t destroy_shader;
    sgimgui_args_destroy_pipeline_t destroy_pipeline;
    sgimgui_args_destroy_attachments_t destroy_attachments;
    sgimgui_args_update_buffer_t update_buffer;
    sgimgui_args_update_image_t update_image;
    sgimgui_args_append_buffer_t append_buffer;
    sgimgui_args_begin_pass_t begin_pass;
    sgimgui_args_apply_viewport_t apply_viewport;
    sgimgui_args_apply_scissor_rect_t apply_scissor_rect;
    sgimgui_args_apply_pipeline_t apply_pipeline;
    sgimgui_args_apply_bindings_t apply_bindings;
    sgimgui_args_apply_uniforms_t apply_uniforms;
    sgimgui_args_draw_t draw;
    sgimgui_args_alloc_buffer_t alloc_buffer;
    sgimgui_args_alloc_image_t alloc_image;
    sgimgui_args_alloc_sampler_t alloc_sampler;
    sgimgui_args_alloc_shader_t alloc_shader;
    sgimgui_args_alloc_pipeline_t alloc_pipeline;
    sgimgui_args_alloc_attachments_t alloc_attachments;
    sgimgui_args_dealloc_buffer_t dealloc_buffer;
    sgimgui_args_dealloc_image_t dealloc_image;
    sgimgui_args_dealloc_sampler_t dealloc_sampler;
    sgimgui_args_dealloc_shader_t dealloc_shader;
    sgimgui_args_dealloc_pipeline_t dealloc_pipeline;
    sgimgui_args_dealloc_attachments_t dealloc_attachments;
    sgimgui_args_init_buffer_t init_buffer;
    sgimgui_args_init_image_t init_image;
    sgimgui_args_init_sampler_t init_sampler;
    sgimgui_args_init_shader_t init_shader;
    sgimgui_args_init_pipeline_t init_pipeline;
    sgimgui_args_init_attachments_t init_attachments;
    sgimgui_args_uninit_buffer_t uninit_buffer;
    sgimgui_args_uninit_image_t uninit_image;
    sgimgui_args_uninit_sampler_t uninit_sampler;
    sgimgui_args_uninit_shader_t uninit_shader;
    sgimgui_args_uninit_pipeline_t uninit_pipeline;
    sgimgui_args_uninit_attachments_t uninit_attachments;
    sgimgui_args_fail_buffer_t fail_buffer;
    sgimgui_args_fail_image_t fail_image;
    sgimgui_args_fail_sampler_t fail_sampler;
    sgimgui_args_fail_shader_t fail_shader;
    sgimgui_args_fail_pipeline_t fail_pipeline;
    sgimgui_args_fail_attachments_t fail_attachments;
    sgimgui_args_push_debug_group_t push_debug_group;
} sgimgui_args_t;

typedef struct sgimgui_capture_item_t {
    sgimgui_cmd_t cmd;
    uint32_t color;
    sgimgui_args_t args;
} sgimgui_capture_item_t;

typedef struct sgimgui_capture_bucket_t {
    size_t ubuf_size;       /* size of uniform capture buffer in bytes */
    size_t ubuf_pos;        /* current uniform buffer pos */
    uint8_t* ubuf;          /* buffer for capturing uniform updates */
    int num_items;
    sgimgui_capture_item_t items[sgimgui_MAX_FRAMECAPTURE_ITEMS];
} sgimgui_capture_bucket_t;

/* double-buffered call-capture buckets, one bucket is currently recorded,
   the previous bucket is displayed
*/
typedef struct sgimgui_capture_window_t {
    bool open;
    int bucket_index;      /* which bucket to record to, 0 or 1 */
    int sel_item;          /* currently selected capture item by index */
    sgimgui_capture_bucket_t bucket[2];
} sgimgui_capture_window_t;

typedef struct sgimgui_caps_window_t {
    bool open;
} sgimgui_caps_window_t;

typedef struct sgimgui_frame_stats_window_t {
    bool open;
    bool disable_sokol_imgui_stats;
    bool in_sokol_imgui;
    sg_frame_stats stats;
    // FIXME: add a ringbuffer for a stats history here
} sgimgui_frame_stats_window_t;

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

typedef struct sgimgui_t {
    uint32_t init_tag;
    sgimgui_desc_t desc;
    sgimgui_buffer_window_t buffer_window;
    sgimgui_image_window_t image_window;
    sgimgui_sampler_window_t sampler_window;
    sgimgui_shader_window_t shader_window;
    sgimgui_pipeline_window_t pipeline_window;
    sgimgui_attachments_window_t attachments_window;
    sgimgui_capture_window_t capture_window;
    sgimgui_caps_window_t caps_window;
    sgimgui_frame_stats_window_t frame_stats_window;
    sg_pipeline cur_pipeline;
    sg_trace_hooks hooks;
} sgimgui_t;

SOKOL_GFX_IMGUI_API_DECL void sgimgui_init(sgimgui_t* ctx, const sgimgui_desc_t* desc);
SOKOL_GFX_IMGUI_API_DECL void sgimgui_discard(sgimgui_t* ctx);
SOKOL_GFX_IMGUI_API_DECL void sgimgui_draw(sgimgui_t* ctx);

SOKOL_GFX_IMGUI_API_DECL void sgimgui_draw_menu(sgimgui_t* ctx, const char* title);

SOKOL_GFX_IMGUI_API_DECL void sgimgui_draw_buffer_window_content(sgimgui_t* ctx);
SOKOL_GFX_IMGUI_API_DECL void sgimgui_draw_image_window_content(sgimgui_t* ctx);
SOKOL_GFX_IMGUI_API_DECL void sgimgui_draw_sampler_window_content(sgimgui_t* ctx);
SOKOL_GFX_IMGUI_API_DECL void sgimgui_draw_shader_window_content(sgimgui_t* ctx);
SOKOL_GFX_IMGUI_API_DECL void sgimgui_draw_pipeline_window_content(sgimgui_t* ctx);
SOKOL_GFX_IMGUI_API_DECL void sgimgui_draw_attachments_window_content(sgimgui_t* ctx);
SOKOL_GFX_IMGUI_API_DECL void sgimgui_draw_capture_window_content(sgimgui_t* ctx);
SOKOL_GFX_IMGUI_API_DECL void sgimgui_draw_capabilities_window_content(sgimgui_t* ctx);
SOKOL_GFX_IMGUI_API_DECL void sgimgui_draw_frame_stats_window_content(sgimgui_t* ctx);

SOKOL_GFX_IMGUI_API_DECL void sgimgui_draw_buffer_window(sgimgui_t* ctx);
SOKOL_GFX_IMGUI_API_DECL void sgimgui_draw_image_window(sgimgui_t* ctx);
SOKOL_GFX_IMGUI_API_DECL void sgimgui_draw_sampler_window(sgimgui_t* ctx);
SOKOL_GFX_IMGUI_API_DECL void sgimgui_draw_shader_window(sgimgui_t* ctx);
SOKOL_GFX_IMGUI_API_DECL void sgimgui_draw_pipeline_window(sgimgui_t* ctx);
SOKOL_GFX_IMGUI_API_DECL void sgimgui_draw_attachments_window(sgimgui_t* ctx);
SOKOL_GFX_IMGUI_API_DECL void sgimgui_draw_capture_window(sgimgui_t* ctx);
SOKOL_GFX_IMGUI_API_DECL void sgimgui_draw_capabilities_window(sgimgui_t* ctx);
SOKOL_GFX_IMGUI_API_DECL void sgimgui_draw_frame_stats_window(sgimgui_t* ctx);

#if defined(__cplusplus)
} /* extern "C" */
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
    #if !defined(CIMGUI_INCLUDED)
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
#define IMVEC2(x,y) ImVec2(x,y)
#define IMVEC4(x,y,z,w) ImVec4(x,y,z,w)
_SOKOL_PRIVATE void igText(const char* fmt,...) {
    va_list args;
    va_start(args, fmt);
    ImGui::TextV(fmt, args);
    va_end(args);
}
_SOKOL_PRIVATE void igSeparator() {
    return ImGui::Separator();
}
_SOKOL_PRIVATE void igSameLine(float offset_from_start_x, float spacing) {
    return ImGui::SameLine(offset_from_start_x,spacing);
}
_SOKOL_PRIVATE void igPushID_Int(int int_id) {
    return ImGui::PushID(int_id);
}
_SOKOL_PRIVATE void igPopID() {
    return ImGui::PopID();
}
_SOKOL_PRIVATE bool igSelectable_Bool(const char* label,bool selected,ImGuiSelectableFlags flags,const ImVec2 size) {
    return ImGui::Selectable(label,selected,flags,size);
}
_SOKOL_PRIVATE bool igSmallButton(const char* label) {
    return ImGui::SmallButton(label);
}
_SOKOL_PRIVATE bool igBeginChild_Str(const char* str_id,const ImVec2 size,bool border,ImGuiWindowFlags flags) {
    return ImGui::BeginChild(str_id,size,border,flags);
}
_SOKOL_PRIVATE void igEndChild() {
    return ImGui::EndChild();
}
_SOKOL_PRIVATE void igPushStyleColor_U32(ImGuiCol idx, ImU32 col) {
    return ImGui::PushStyleColor(idx,col);
}
_SOKOL_PRIVATE void igPopStyleColor(int count) {
    return ImGui::PopStyleColor(count);
}
_SOKOL_PRIVATE bool igTreeNode_StrStr(const char* str_id,const char* fmt,...) {
    va_list args;
    va_start(args, fmt);
    bool ret = ImGui::TreeNodeV(str_id,fmt,args);
    va_end(args);
    return ret;
}
_SOKOL_PRIVATE bool igTreeNode_Str(const char* label) {
    return ImGui::TreeNode(label);
}
_SOKOL_PRIVATE void igTreePop() {
    return ImGui::TreePop();
}
_SOKOL_PRIVATE bool igIsItemHovered(ImGuiHoveredFlags flags) {
    return ImGui::IsItemHovered(flags);
}
_SOKOL_PRIVATE void igSetTooltip(const char* fmt,...) {
    va_list args;
    va_start(args, fmt);
    ImGui::SetTooltipV(fmt,args);
    va_end(args);
}
_SOKOL_PRIVATE bool igSliderFloat(const char* label,float* v,float v_min,float v_max,const char* format,ImGuiSliderFlags flags) {
    return ImGui::SliderFloat(label,v,v_min,v_max,format,flags);
}
_SOKOL_PRIVATE void igImage(ImTextureID user_texture_id,const ImVec2 size,const ImVec2 uv0,const ImVec2 uv1,const ImVec4 tint_col,const ImVec4 border_col) {
    return ImGui::Image(user_texture_id,size,uv0,uv1,tint_col,border_col);
}
_SOKOL_PRIVATE void igSetNextWindowSize(const ImVec2 size,ImGuiCond cond) {
    return ImGui::SetNextWindowSize(size,cond);
}
_SOKOL_PRIVATE bool igBegin(const char* name,bool* p_open,ImGuiWindowFlags flags) {
    return ImGui::Begin(name,p_open,flags);
}
_SOKOL_PRIVATE void igEnd() {
    return ImGui::End();
}
_SOKOL_PRIVATE bool igBeginMenu(const char* label, bool enabled) {
    return ImGui::BeginMenu(label, enabled);
}
_SOKOL_PRIVATE void igEndMenu(void) {
    ImGui::EndMenu();
}
_SOKOL_PRIVATE bool igMenuItem_BoolPtr(const char* label, const char* shortcut, bool* p_selected, bool enabled) {
    return ImGui::MenuItem(label, shortcut, p_selected, enabled);
}
_SOKOL_PRIVATE bool igBeginTable(const char* str_id, int column, ImGuiTableFlags flags, const ImVec2 outer_size, float inner_width) {
    return ImGui::BeginTable(str_id, column, flags, outer_size, inner_width);
}
_SOKOL_PRIVATE void igEndTable(void) {
    ImGui::EndTable();
}
_SOKOL_PRIVATE void igTableSetupScrollFreeze(int cols, int rows) {
    ImGui::TableSetupScrollFreeze(cols, rows);
}
_SOKOL_PRIVATE void igTableSetupColumn(const char* label, ImGuiTableColumnFlags flags, float init_width_or_weight, ImGuiID user_id) {
    ImGui::TableSetupColumn(label, flags, init_width_or_weight, user_id);
}
_SOKOL_PRIVATE void igTableHeadersRow(void) {
    ImGui::TableHeadersRow();
}
_SOKOL_PRIVATE void igTableNextRow(ImGuiTableRowFlags row_flags, float min_row_height) {
    ImGui::TableNextRow(row_flags, min_row_height);
}
_SOKOL_PRIVATE bool igTableSetColumnIndex(int column_n) {
    return ImGui::TableSetColumnIndex(column_n);
}
_SOKOL_PRIVATE bool igCheckbox(const char* label, bool* v) {
    return ImGui::Checkbox(label, v);
}
#else
#define IMVEC2(x,y) (ImVec2){x,y}
#define IMVEC4(x,y,z,w) (ImVec4){x,y,z,w}
#endif

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

_SOKOL_PRIVATE void _sgimgui_strcpy(sgimgui_str_t* dst, const char* src) {
    SOKOL_ASSERT(dst);
    if (src) {
        #if defined(_MSC_VER)
        strncpy_s(dst->buf, sgimgui_STRBUF_LEN, src, (sgimgui_STRBUF_LEN-1));
        #else
        strncpy(dst->buf, src, sgimgui_STRBUF_LEN);
        #endif
        dst->buf[sgimgui_STRBUF_LEN-1] = 0;
    } else {
        _sgimgui_clear(dst->buf, sgimgui_STRBUF_LEN);
    }
}

_SOKOL_PRIVATE sgimgui_str_t _sgimgui_make_str(const char* str) {
    sgimgui_str_t res;
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

_SOKOL_PRIVATE void _sgimgui_snprintf(sgimgui_str_t* dst, const char* fmt, ...) {
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
    igText("ResId: %08X", slot->res_id);
    igText("State: %s", _sgimgui_resourcestate_string(slot->state));
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
        case SG_BACKEND_DUMMY:              return "SG_BACKEND_DUMMY";
        default: return "???";
    }
}

_SOKOL_PRIVATE const char* _sgimgui_buffertype_string(sg_buffer_type t) {
    switch (t) {
        case SG_BUFFERTYPE_VERTEXBUFFER:    return "SG_BUFFERTYPE_VERTEXBUFFER";
        case SG_BUFFERTYPE_INDEXBUFFER:     return "SG_BUFFERTYPE_INDEXBUFFER";
        case SG_BUFFERTYPE_STORAGEBUFFER:   return "SG_BUFFERTYPE_STORAGEBUFFER";
        default:                            return "???";
    }
}

_SOKOL_PRIVATE const char* _sgimgui_usage_string(sg_usage u) {
    switch (u) {
        case SG_USAGE_IMMUTABLE:    return "SG_USAGE_IMMUTABLE";
        case SG_USAGE_DYNAMIC:      return "SG_USAGE_DYNAMIC";
        case SG_USAGE_STREAM:       return "SG_USAGE_STREAM";
        default:                    return "???";
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
        case SG_PIXELFORMAT_PVRTC_RGB_2BPP: return "SG_PIXELFORMAT_PVRTC_RGB_2BPP";
        case SG_PIXELFORMAT_PVRTC_RGB_4BPP: return "SG_PIXELFORMAT_PVRTC_RGB_4BPP";
        case SG_PIXELFORMAT_PVRTC_RGBA_2BPP: return "SG_PIXELFORMAT_PVRTC_RGBA_2BPP";
        case SG_PIXELFORMAT_PVRTC_RGBA_4BPP: return "SG_PIXELFORMAT_PVRTC_RGBA_4BPP";
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
        case SG_FILTER_NONE:    return "SG_FILTER_NONE";
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
        case SG_VERTEXFORMAT_BYTE4:     return "SG_VERTEXFORMAT_BYTE4";
        case SG_VERTEXFORMAT_BYTE4N:    return "SG_VERTEXFORMAT_BYTE4N";
        case SG_VERTEXFORMAT_UBYTE4:    return "SG_VERTEXFORMAT_UBYTE4";
        case SG_VERTEXFORMAT_UBYTE4N:   return "SG_VERTEXFORMAT_UBYTE4N";
        case SG_VERTEXFORMAT_SHORT2:    return "SG_VERTEXFORMAT_SHORT2";
        case SG_VERTEXFORMAT_SHORT2N:   return "SG_VERTEXFORMAT_SHORT2N";
        case SG_VERTEXFORMAT_USHORT2N:  return "SG_VERTEXFORMAT_USHORT2N";
        case SG_VERTEXFORMAT_SHORT4:    return "SG_VERTEXFORMAT_SHORT4";
        case SG_VERTEXFORMAT_SHORT4N:   return "SG_VERTEXFORMAT_SHORT4N";
        case SG_VERTEXFORMAT_USHORT4N:  return "SG_VERTEXFORMAT_USHORT4N";
        case SG_VERTEXFORMAT_UINT10_N2: return "SG_VERTEXFORMAT_UINT10_N2";
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
        case SG_SHADERSTAGE_VS:     return "SG_SHADERSTAGE_VS";
        case SG_SHADERSTAGE_FS:     return "SG_SHADERSTAGE_FS";
        default:                    return "???";
    }
}

_SOKOL_PRIVATE const char* _sgimgui_bool_string(bool b) {
    return b ? "true" : "false";
}

_SOKOL_PRIVATE const char* _sgimgui_color_string(sgimgui_str_t* dst_str, sg_color color) {
    _sgimgui_snprintf(dst_str, "%.3f %.3f %.3f %.3f", color.r, color.g, color.b, color.a);
    return dst_str->buf;
}

_SOKOL_PRIVATE sgimgui_str_t _sgimgui_res_id_string(uint32_t res_id, const char* label) {
    SOKOL_ASSERT(label);
    sgimgui_str_t res;
    if (label[0]) {
        _sgimgui_snprintf(&res, "'%s'", label);
    } else {
        _sgimgui_snprintf(&res, "0x%08X", res_id);
    }
    return res;
}

_SOKOL_PRIVATE sgimgui_str_t _sgimgui_buffer_id_string(sgimgui_t* ctx, sg_buffer buf_id) {
    if (buf_id.id != SG_INVALID_ID) {
        const sgimgui_buffer_t* buf_ui = &ctx->buffer_window.slots[_sgimgui_slot_index(buf_id.id)];
        return _sgimgui_res_id_string(buf_id.id, buf_ui->label.buf);
    } else {
        return _sgimgui_make_str("<invalid>");
    }
}

_SOKOL_PRIVATE sgimgui_str_t _sgimgui_image_id_string(sgimgui_t* ctx, sg_image img_id) {
    if (img_id.id != SG_INVALID_ID) {
        const sgimgui_image_t* img_ui = &ctx->image_window.slots[_sgimgui_slot_index(img_id.id)];
        return _sgimgui_res_id_string(img_id.id, img_ui->label.buf);
    } else {
        return _sgimgui_make_str("<invalid>");
    }
}

_SOKOL_PRIVATE sgimgui_str_t _sgimgui_sampler_id_string(sgimgui_t* ctx, sg_sampler smp_id) {
    if (smp_id.id != SG_INVALID_ID) {
        const sgimgui_sampler_t* smp_ui = &ctx->sampler_window.slots[_sgimgui_slot_index(smp_id.id)];
        return _sgimgui_res_id_string(smp_id.id, smp_ui->label.buf);
    } else {
        return _sgimgui_make_str("<invalid>");
    }
}

_SOKOL_PRIVATE sgimgui_str_t _sgimgui_shader_id_string(sgimgui_t* ctx, sg_shader shd_id) {
    if (shd_id.id != SG_INVALID_ID) {
        const sgimgui_shader_t* shd_ui = &ctx->shader_window.slots[_sgimgui_slot_index(shd_id.id)];
        return _sgimgui_res_id_string(shd_id.id, shd_ui->label.buf);
    } else {
        return _sgimgui_make_str("<invalid>");
    }
}

_SOKOL_PRIVATE sgimgui_str_t _sgimgui_pipeline_id_string(sgimgui_t* ctx, sg_pipeline pip_id) {
    if (pip_id.id != SG_INVALID_ID) {
        const sgimgui_pipeline_t* pip_ui = &ctx->pipeline_window.slots[_sgimgui_slot_index(pip_id.id)];
        return _sgimgui_res_id_string(pip_id.id, pip_ui->label.buf);
    } else {
        return _sgimgui_make_str("<invalid>");
    }
}

_SOKOL_PRIVATE sgimgui_str_t _sgimgui_attachments_id_string(sgimgui_t* ctx, sg_attachments atts_id) {
    if (atts_id.id != SG_INVALID_ID) {
        const sgimgui_attachments_t* atts_ui = &ctx->attachments_window.slots[_sgimgui_slot_index(atts_id.id)];
        return _sgimgui_res_id_string(atts_id.id, atts_ui->label.buf);
    } else {
        return _sgimgui_make_str("<invalid>");
    }
}

/*--- RESOURCE HELPERS -------------------------------------------------------*/
_SOKOL_PRIVATE void _sgimgui_buffer_created(sgimgui_t* ctx, sg_buffer res_id, int slot_index, const sg_buffer_desc* desc) {
    SOKOL_ASSERT((slot_index > 0) && (slot_index < ctx->buffer_window.num_slots));
    sgimgui_buffer_t* buf = &ctx->buffer_window.slots[slot_index];
    buf->res_id = res_id;
    buf->desc = *desc;
    buf->label = _sgimgui_make_str(desc->label);
}

_SOKOL_PRIVATE void _sgimgui_buffer_destroyed(sgimgui_t* ctx, int slot_index) {
    SOKOL_ASSERT((slot_index > 0) && (slot_index < ctx->buffer_window.num_slots));
    sgimgui_buffer_t* buf = &ctx->buffer_window.slots[slot_index];
    buf->res_id.id = SG_INVALID_ID;
}

_SOKOL_PRIVATE void _sgimgui_image_created(sgimgui_t* ctx, sg_image res_id, int slot_index, const sg_image_desc* desc) {
    SOKOL_ASSERT((slot_index > 0) && (slot_index < ctx->image_window.num_slots));
    sgimgui_image_t* img = &ctx->image_window.slots[slot_index];
    img->res_id = res_id;
    img->desc = *desc;
    img->ui_scale = 1.0f;
    img->label = _sgimgui_make_str(desc->label);
    simgui_image_desc_t simgui_img_desc;
    _sgimgui_clear(&simgui_img_desc, sizeof(simgui_img_desc));
    simgui_img_desc.image = res_id;
    // keep sampler at default, which will use sokol_imgui.h's default nearest-filtering sampler
    img->simgui_img = simgui_make_image(&simgui_img_desc);
}

_SOKOL_PRIVATE void _sgimgui_image_destroyed(sgimgui_t* ctx, int slot_index) {
    SOKOL_ASSERT((slot_index > 0) && (slot_index < ctx->image_window.num_slots));
    sgimgui_image_t* img = &ctx->image_window.slots[slot_index];
    img->res_id.id = SG_INVALID_ID;
    simgui_destroy_image(img->simgui_img);
}

_SOKOL_PRIVATE void _sgimgui_sampler_created(sgimgui_t* ctx, sg_sampler res_id, int slot_index, const sg_sampler_desc* desc) {
    SOKOL_ASSERT((slot_index > 0) && (slot_index < ctx->sampler_window.num_slots));
    sgimgui_sampler_t* smp = &ctx->sampler_window.slots[slot_index];
    smp->res_id = res_id;
    smp->desc = *desc;
    smp->label = _sgimgui_make_str(desc->label);
}

_SOKOL_PRIVATE void _sgimgui_sampler_destroyed(sgimgui_t* ctx, int slot_index) {
    SOKOL_ASSERT((slot_index > 0) && (slot_index < ctx->sampler_window.num_slots));
    sgimgui_sampler_t* smp = &ctx->sampler_window.slots[slot_index];
    smp->res_id.id = SG_INVALID_ID;
}

_SOKOL_PRIVATE void _sgimgui_shader_created(sgimgui_t* ctx, sg_shader res_id, int slot_index, const sg_shader_desc* desc) {
    SOKOL_ASSERT((slot_index > 0) && (slot_index < ctx->shader_window.num_slots));
    sgimgui_shader_t* shd = &ctx->shader_window.slots[slot_index];
    shd->res_id = res_id;
    shd->desc = *desc;
    shd->label = _sgimgui_make_str(desc->label);
    if (shd->desc.vs.entry) {
        shd->vs_entry = _sgimgui_make_str(shd->desc.vs.entry);
        shd->desc.vs.entry = shd->vs_entry.buf;
    }
    if (shd->desc.fs.entry) {
        shd->fs_entry = _sgimgui_make_str(shd->desc.fs.entry);
        shd->desc.fs.entry = shd->fs_entry.buf;
    }
    if (shd->desc.vs.d3d11_target) {
        shd->vs_d3d11_target = _sgimgui_make_str(shd->desc.vs.d3d11_target);
        shd->desc.fs.d3d11_target = shd->vs_d3d11_target.buf;
    }
    if (shd->desc.fs.d3d11_target) {
        shd->fs_d3d11_target = _sgimgui_make_str(shd->desc.fs.d3d11_target);
        shd->desc.fs.d3d11_target = shd->fs_d3d11_target.buf;
    }
    for (int i = 0; i < SG_MAX_SHADERSTAGE_UBS; i++) {
        for (int j = 0; j < SG_MAX_UB_MEMBERS; j++) {
            sg_shader_uniform_desc* ud = &shd->desc.vs.uniform_blocks[i].uniforms[j];
            if (ud->name) {
                shd->vs_uniform_name[i][j] = _sgimgui_make_str(ud->name);
                ud->name = shd->vs_uniform_name[i][j].buf;
            }
        }
    }
    for (int i = 0; i < SG_MAX_SHADERSTAGE_UBS; i++) {
        for (int j = 0; j < SG_MAX_UB_MEMBERS; j++) {
            sg_shader_uniform_desc* ud = &shd->desc.fs.uniform_blocks[i].uniforms[j];
            if (ud->name) {
                shd->fs_uniform_name[i][j] = _sgimgui_make_str(ud->name);
                ud->name = shd->fs_uniform_name[i][j].buf;
            }
        }
    }
    for (int i = 0; i < SG_MAX_SHADERSTAGE_IMAGESAMPLERPAIRS; i++) {
        if (shd->desc.vs.image_sampler_pairs[i].glsl_name) {
            shd->vs_image_sampler_name[i] = _sgimgui_make_str(shd->desc.vs.image_sampler_pairs[i].glsl_name);
            shd->desc.vs.image_sampler_pairs[i].glsl_name = shd->vs_image_sampler_name[i].buf;
        }
    }
    for (int i = 0; i < SG_MAX_SHADERSTAGE_IMAGESAMPLERPAIRS; i++) {
        if (shd->desc.fs.image_sampler_pairs[i].glsl_name) {
            shd->fs_image_sampler_name[i] = _sgimgui_make_str(shd->desc.fs.image_sampler_pairs[i].glsl_name);
            shd->desc.fs.image_sampler_pairs[i].glsl_name = shd->fs_image_sampler_name[i].buf;
        }
    }
    if (shd->desc.vs.source) {
        shd->desc.vs.source = _sgimgui_str_dup(&ctx->desc.allocator, shd->desc.vs.source);
    }
    if (shd->desc.vs.bytecode.ptr) {
        shd->desc.vs.bytecode.ptr = _sgimgui_bin_dup(&ctx->desc.allocator, shd->desc.vs.bytecode.ptr, shd->desc.vs.bytecode.size);
    }
    if (shd->desc.fs.source) {
        shd->desc.fs.source = _sgimgui_str_dup(&ctx->desc.allocator, shd->desc.fs.source);
    }
    if (shd->desc.fs.bytecode.ptr) {
        shd->desc.fs.bytecode.ptr = _sgimgui_bin_dup(&ctx->desc.allocator, shd->desc.fs.bytecode.ptr, shd->desc.fs.bytecode.size);
    }
    for (int i = 0; i < SG_MAX_VERTEX_ATTRIBUTES; i++) {
        sg_shader_attr_desc* ad = &shd->desc.attrs[i];
        if (ad->name) {
            shd->attr_name[i] = _sgimgui_make_str(ad->name);
            ad->name = shd->attr_name[i].buf;
        }
        if (ad->sem_name) {
            shd->attr_sem_name[i] = _sgimgui_make_str(ad->sem_name);
            ad->sem_name = shd->attr_sem_name[i].buf;
        }
    }
}

_SOKOL_PRIVATE void _sgimgui_shader_destroyed(sgimgui_t* ctx, int slot_index) {
    SOKOL_ASSERT((slot_index > 0) && (slot_index < ctx->shader_window.num_slots));
    sgimgui_shader_t* shd = &ctx->shader_window.slots[slot_index];
    shd->res_id.id = SG_INVALID_ID;
    if (shd->desc.vs.source) {
        _sgimgui_free(&ctx->desc.allocator, (void*)shd->desc.vs.source);
        shd->desc.vs.source = 0;
    }
    if (shd->desc.vs.bytecode.ptr) {
        _sgimgui_free(&ctx->desc.allocator, (void*)shd->desc.vs.bytecode.ptr);
        shd->desc.vs.bytecode.ptr = 0;
    }
    if (shd->desc.fs.source) {
        _sgimgui_free(&ctx->desc.allocator, (void*)shd->desc.fs.source);
        shd->desc.fs.source = 0;
    }
    if (shd->desc.fs.bytecode.ptr) {
        _sgimgui_free(&ctx->desc.allocator, (void*)shd->desc.fs.bytecode.ptr);
        shd->desc.fs.bytecode.ptr = 0;
    }
}

_SOKOL_PRIVATE void _sgimgui_pipeline_created(sgimgui_t* ctx, sg_pipeline res_id, int slot_index, const sg_pipeline_desc* desc) {
    SOKOL_ASSERT((slot_index > 0) && (slot_index < ctx->pipeline_window.num_slots));
    sgimgui_pipeline_t* pip = &ctx->pipeline_window.slots[slot_index];
    pip->res_id = res_id;
    pip->label = _sgimgui_make_str(desc->label);
    pip->desc = *desc;

}

_SOKOL_PRIVATE void _sgimgui_pipeline_destroyed(sgimgui_t* ctx, int slot_index) {
    SOKOL_ASSERT((slot_index > 0) && (slot_index < ctx->pipeline_window.num_slots));
    sgimgui_pipeline_t* pip = &ctx->pipeline_window.slots[slot_index];
    pip->res_id.id = SG_INVALID_ID;
}

_SOKOL_PRIVATE void _sgimgui_attachments_created(sgimgui_t* ctx, sg_attachments res_id, int slot_index, const sg_attachments_desc* desc) {
    SOKOL_ASSERT((slot_index > 0) && (slot_index < ctx->attachments_window.num_slots));
    sgimgui_attachments_t* atts = &ctx->attachments_window.slots[slot_index];
    atts->res_id = res_id;
    for (int i = 0; i < SG_MAX_COLOR_ATTACHMENTS; i++) {
        atts->color_image_scale[i] = 0.25f;
        atts->resolve_image_scale[i] = 0.25f;
    }
    atts->ds_image_scale = 0.25f;
    atts->label = _sgimgui_make_str(desc->label);
    atts->desc = *desc;
}

_SOKOL_PRIVATE void _sgimgui_attachments_destroyed(sgimgui_t* ctx, int slot_index) {
    SOKOL_ASSERT((slot_index > 0) && (slot_index < ctx->attachments_window.num_slots));
    sgimgui_attachments_t* atts = &ctx->attachments_window.slots[slot_index];
    atts->res_id.id = SG_INVALID_ID;
}

/*--- COMMAND CAPTURING ------------------------------------------------------*/
_SOKOL_PRIVATE void _sgimgui_capture_init(sgimgui_t* ctx) {
    const size_t ubuf_initial_size = 256 * 1024;
    for (int i = 0; i < 2; i++) {
        sgimgui_capture_bucket_t* bucket = &ctx->capture_window.bucket[i];
        bucket->ubuf_size = ubuf_initial_size;
        bucket->ubuf = (uint8_t*) _sgimgui_malloc(&ctx->desc.allocator, bucket->ubuf_size);
    }
}

_SOKOL_PRIVATE void _sgimgui_capture_discard(sgimgui_t* ctx) {
    for (int i = 0; i < 2; i++) {
        sgimgui_capture_bucket_t* bucket = &ctx->capture_window.bucket[i];
        SOKOL_ASSERT(bucket->ubuf);
        _sgimgui_free(&ctx->desc.allocator, bucket->ubuf);
        bucket->ubuf = 0;
    }
}

_SOKOL_PRIVATE sgimgui_capture_bucket_t* _sgimgui_capture_get_write_bucket(sgimgui_t* ctx) {
    return &ctx->capture_window.bucket[ctx->capture_window.bucket_index & 1];
}

_SOKOL_PRIVATE sgimgui_capture_bucket_t* _sgimgui_capture_get_read_bucket(sgimgui_t* ctx) {
    return &ctx->capture_window.bucket[(ctx->capture_window.bucket_index + 1) & 1];
}

_SOKOL_PRIVATE void _sgimgui_capture_next_frame(sgimgui_t* ctx) {
    ctx->capture_window.bucket_index = (ctx->capture_window.bucket_index + 1) & 1;
    sgimgui_capture_bucket_t* bucket = &ctx->capture_window.bucket[ctx->capture_window.bucket_index];
    bucket->num_items = 0;
    bucket->ubuf_pos = 0;
}

_SOKOL_PRIVATE void _sgimgui_capture_grow_ubuf(sgimgui_t* ctx, size_t required_size) {
    sgimgui_capture_bucket_t* bucket = _sgimgui_capture_get_write_bucket(ctx);
    SOKOL_ASSERT(required_size > bucket->ubuf_size);
    size_t old_size = bucket->ubuf_size;
    size_t new_size = required_size + (required_size>>1);  /* allocate a bit ahead */
    bucket->ubuf_size = new_size;
    bucket->ubuf = (uint8_t*) _sgimgui_realloc(&ctx->desc.allocator, bucket->ubuf, old_size, new_size);
}

_SOKOL_PRIVATE sgimgui_capture_item_t* _sgimgui_capture_next_write_item(sgimgui_t* ctx) {
    sgimgui_capture_bucket_t* bucket = _sgimgui_capture_get_write_bucket(ctx);
    if (bucket->num_items < sgimgui_MAX_FRAMECAPTURE_ITEMS) {
        sgimgui_capture_item_t* item = &bucket->items[bucket->num_items++];
        return item;
    } else {
        return 0;
    }
}

_SOKOL_PRIVATE int _sgimgui_capture_num_read_items(sgimgui_t* ctx) {
    sgimgui_capture_bucket_t* bucket = _sgimgui_capture_get_read_bucket(ctx);
    return bucket->num_items;
}

_SOKOL_PRIVATE sgimgui_capture_item_t* _sgimgui_capture_read_item_at(sgimgui_t* ctx, int index) {
    sgimgui_capture_bucket_t* bucket = _sgimgui_capture_get_read_bucket(ctx);
    SOKOL_ASSERT(index < bucket->num_items);
    return &bucket->items[index];
}

_SOKOL_PRIVATE size_t _sgimgui_capture_uniforms(sgimgui_t* ctx, const sg_range* data) {
    sgimgui_capture_bucket_t* bucket = _sgimgui_capture_get_write_bucket(ctx);
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

_SOKOL_PRIVATE sgimgui_str_t _sgimgui_capture_item_string(sgimgui_t* ctx, int index, const sgimgui_capture_item_t* item) {
    sgimgui_str_t str = _sgimgui_make_str(0);
    switch (item->cmd) {
        case SGIMGUI_CMD_RESET_STATE_CACHE:
            _sgimgui_snprintf(&str, "%d: sg_reset_state_cache()", index);
            break;

        case SGIMGUI_CMD_MAKE_BUFFER:
            {
                sgimgui_str_t res_id = _sgimgui_buffer_id_string(ctx, item->args.make_buffer.result);
                _sgimgui_snprintf(&str, "%d: sg_make_buffer(desc=..) => %s", index, res_id.buf);
            }
            break;

        case SGIMGUI_CMD_MAKE_IMAGE:
            {
                sgimgui_str_t res_id = _sgimgui_image_id_string(ctx, item->args.make_image.result);
                _sgimgui_snprintf(&str, "%d: sg_make_image(desc=..) => %s", index, res_id.buf);
            }
            break;

        case SGIMGUI_CMD_MAKE_SAMPLER:
            {
                sgimgui_str_t res_id = _sgimgui_sampler_id_string(ctx, item->args.make_sampler.result);
                _sgimgui_snprintf(&str, "%d: sg_make_sampler(desc=..) => %s", index, res_id.buf);
            }
            break;
        case SGIMGUI_CMD_MAKE_SHADER:
            {
                sgimgui_str_t res_id = _sgimgui_shader_id_string(ctx, item->args.make_shader.result);
                _sgimgui_snprintf(&str, "%d: sg_make_shader(desc=..) => %s", index, res_id.buf);
            }
            break;

        case SGIMGUI_CMD_MAKE_PIPELINE:
            {
                sgimgui_str_t res_id = _sgimgui_pipeline_id_string(ctx, item->args.make_pipeline.result);
                _sgimgui_snprintf(&str, "%d: sg_make_pipeline(desc=..) => %s", index, res_id.buf);
            }
            break;

        case SGIMGUI_CMD_MAKE_ATTACHMENTS:
            {
                sgimgui_str_t res_id = _sgimgui_attachments_id_string(ctx, item->args.make_attachments.result);
                _sgimgui_snprintf(&str, "%d: sg_make_attachments(desc=..) => %s", index, res_id.buf);
            }
            break;

        case SGIMGUI_CMD_DESTROY_BUFFER:
            {
                sgimgui_str_t res_id = _sgimgui_buffer_id_string(ctx, item->args.destroy_buffer.buffer);
                _sgimgui_snprintf(&str, "%d: sg_destroy_buffer(buf=%s)", index, res_id.buf);
            }
            break;

        case SGIMGUI_CMD_DESTROY_IMAGE:
            {
                sgimgui_str_t res_id = _sgimgui_image_id_string(ctx, item->args.destroy_image.image);
                _sgimgui_snprintf(&str, "%d: sg_destroy_image(img=%s)", index, res_id.buf);
            }
            break;

        case SGIMGUI_CMD_DESTROY_SAMPLER:
            {
                sgimgui_str_t res_id = _sgimgui_sampler_id_string(ctx, item->args.destroy_sampler.sampler);
                _sgimgui_snprintf(&str, "%d: sg_destroy_sampler(smp=%s)", index, res_id.buf);
            }
            break;

        case SGIMGUI_CMD_DESTROY_SHADER:
            {
                sgimgui_str_t res_id = _sgimgui_shader_id_string(ctx, item->args.destroy_shader.shader);
                _sgimgui_snprintf(&str, "%d: sg_destroy_shader(shd=%s)", index, res_id.buf);
            }
            break;

        case SGIMGUI_CMD_DESTROY_PIPELINE:
            {
                sgimgui_str_t res_id = _sgimgui_pipeline_id_string(ctx, item->args.destroy_pipeline.pipeline);
                _sgimgui_snprintf(&str, "%d: sg_destroy_pipeline(pip=%s)", index, res_id.buf);
            }
            break;

        case SGIMGUI_CMD_DESTROY_ATTACHMENTS:
            {
                sgimgui_str_t res_id = _sgimgui_attachments_id_string(ctx, item->args.destroy_attachments.attachments);
                _sgimgui_snprintf(&str, "%d: sg_destroy_attachments(atts=%s)", index, res_id.buf);
            }
            break;

        case SGIMGUI_CMD_UPDATE_BUFFER:
            {
                sgimgui_str_t res_id = _sgimgui_buffer_id_string(ctx, item->args.update_buffer.buffer);
                _sgimgui_snprintf(&str, "%d: sg_update_buffer(buf=%s, data.size=%d)",
                    index, res_id.buf,
                    item->args.update_buffer.data_size);
            }
            break;

        case SGIMGUI_CMD_UPDATE_IMAGE:
            {
                sgimgui_str_t res_id = _sgimgui_image_id_string(ctx, item->args.update_image.image);
                _sgimgui_snprintf(&str, "%d: sg_update_image(img=%s, data=..)", index, res_id.buf);
            }
            break;

        case SGIMGUI_CMD_APPEND_BUFFER:
            {
                sgimgui_str_t res_id = _sgimgui_buffer_id_string(ctx, item->args.append_buffer.buffer);
                _sgimgui_snprintf(&str, "%d: sg_append_buffer(buf=%s, data.size=%d) => %d",
                    index, res_id.buf,
                    item->args.append_buffer.data_size,
                    item->args.append_buffer.result);
            }
            break;

        case SGIMGUI_CMD_BEGIN_PASS:
            {
                _sgimgui_snprintf(&str, "%d: sg_begin_pass(pass=...)", index);
            }
            break;

        case SGIMGUI_CMD_APPLY_VIEWPORT:
            _sgimgui_snprintf(&str, "%d: sg_apply_viewport(x=%d, y=%d, width=%d, height=%d, origin_top_left=%s)",
                index,
                item->args.apply_viewport.x,
                item->args.apply_viewport.y,
                item->args.apply_viewport.width,
                item->args.apply_viewport.height,
                _sgimgui_bool_string(item->args.apply_viewport.origin_top_left));
            break;

        case SGIMGUI_CMD_APPLY_SCISSOR_RECT:
            _sgimgui_snprintf(&str, "%d: sg_apply_scissor_rect(x=%d, y=%d, width=%d, height=%d, origin_top_left=%s)",
                index,
                item->args.apply_scissor_rect.x,
                item->args.apply_scissor_rect.y,
                item->args.apply_scissor_rect.width,
                item->args.apply_scissor_rect.height,
                _sgimgui_bool_string(item->args.apply_scissor_rect.origin_top_left));
            break;

        case SGIMGUI_CMD_APPLY_PIPELINE:
            {
                sgimgui_str_t res_id = _sgimgui_pipeline_id_string(ctx, item->args.apply_pipeline.pipeline);
                _sgimgui_snprintf(&str, "%d: sg_apply_pipeline(pip=%s)", index, res_id.buf);
            }
            break;

        case SGIMGUI_CMD_APPLY_BINDINGS:
            _sgimgui_snprintf(&str, "%d: sg_apply_bindings(bindings=..)", index);
            break;

        case SGIMGUI_CMD_APPLY_UNIFORMS:
            _sgimgui_snprintf(&str, "%d: sg_apply_uniforms(stage=%s, ub_index=%d, data.size=%d)",
                index,
                _sgimgui_shaderstage_string(item->args.apply_uniforms.stage),
                item->args.apply_uniforms.ub_index,
                item->args.apply_uniforms.data_size);
            break;

        case SGIMGUI_CMD_DRAW:
            _sgimgui_snprintf(&str, "%d: sg_draw(base_element=%d, num_elements=%d, num_instances=%d)",
                index,
                item->args.draw.base_element,
                item->args.draw.num_elements,
                item->args.draw.num_instances);
            break;

        case SGIMGUI_CMD_END_PASS:
            _sgimgui_snprintf(&str, "%d: sg_end_pass()", index);
            break;

        case SGIMGUI_CMD_COMMIT:
            _sgimgui_snprintf(&str, "%d: sg_commit()", index);
            break;

        case SGIMGUI_CMD_ALLOC_BUFFER:
            {
                sgimgui_str_t res_id = _sgimgui_buffer_id_string(ctx, item->args.alloc_buffer.result);
                _sgimgui_snprintf(&str, "%d: sg_alloc_buffer() => %s", index, res_id.buf);
            }
            break;

        case SGIMGUI_CMD_ALLOC_IMAGE:
            {
                sgimgui_str_t res_id = _sgimgui_image_id_string(ctx, item->args.alloc_image.result);
                _sgimgui_snprintf(&str, "%d: sg_alloc_image() => %s", index, res_id.buf);
            }
            break;

        case SGIMGUI_CMD_ALLOC_SAMPLER:
            {
                sgimgui_str_t res_id = _sgimgui_sampler_id_string(ctx, item->args.alloc_sampler.result);
                _sgimgui_snprintf(&str, "%d: sg_alloc_sampler() => %s", index, res_id.buf);
            }
            break;

        case SGIMGUI_CMD_ALLOC_SHADER:
            {
                sgimgui_str_t res_id = _sgimgui_shader_id_string(ctx, item->args.alloc_shader.result);
                _sgimgui_snprintf(&str, "%d: sg_alloc_shader() => %s", index, res_id.buf);
            }
            break;

        case SGIMGUI_CMD_ALLOC_PIPELINE:
            {
                sgimgui_str_t res_id = _sgimgui_pipeline_id_string(ctx, item->args.alloc_pipeline.result);
                _sgimgui_snprintf(&str, "%d: sg_alloc_pipeline() => %s", index, res_id.buf);
            }
            break;

        case SGIMGUI_CMD_ALLOC_ATTACHMENTS:
            {
                sgimgui_str_t res_id = _sgimgui_attachments_id_string(ctx, item->args.alloc_attachments.result);
                _sgimgui_snprintf(&str, "%d: sg_alloc_attachments() => %s", index, res_id.buf);
            }
            break;

        case SGIMGUI_CMD_DEALLOC_BUFFER:
            {
                sgimgui_str_t res_id = _sgimgui_buffer_id_string(ctx, item->args.dealloc_buffer.buffer);
                _sgimgui_snprintf(&str, "%d: sg_dealloc_buffer(buf=%s)", index, res_id.buf);
            }
            break;

        case SGIMGUI_CMD_DEALLOC_IMAGE:
            {
                sgimgui_str_t res_id = _sgimgui_image_id_string(ctx, item->args.dealloc_image.image);
                _sgimgui_snprintf(&str, "%d: sg_dealloc_image(img=%d)", index, res_id.buf);
            }
            break;

        case SGIMGUI_CMD_DEALLOC_SAMPLER:
            {
                sgimgui_str_t res_id = _sgimgui_sampler_id_string(ctx, item->args.dealloc_sampler.sampler);
                _sgimgui_snprintf(&str, "%d: sg_dealloc_sampler(smp=%s)", index, res_id.buf);
            }
            break;

        case SGIMGUI_CMD_DEALLOC_SHADER:
            {
                sgimgui_str_t res_id = _sgimgui_shader_id_string(ctx, item->args.dealloc_shader.shader);
                _sgimgui_snprintf(&str, "%d: sg_dealloc_shader(shd=%s)", index, res_id.buf);
            }
            break;

        case SGIMGUI_CMD_DEALLOC_PIPELINE:
            {
                sgimgui_str_t res_id = _sgimgui_pipeline_id_string(ctx, item->args.dealloc_pipeline.pipeline);
                _sgimgui_snprintf(&str, "%d: sg_dealloc_pipeline(pip=%s)", index, res_id.buf);
            }
            break;

        case SGIMGUI_CMD_DEALLOC_ATTACHMENTS:
            {
                sgimgui_str_t res_id = _sgimgui_attachments_id_string(ctx, item->args.dealloc_attachments.attachments);
                _sgimgui_snprintf(&str, "%d: sg_dealloc_attachments(atts=%s)", index, res_id.buf);
            }
            break;

        case SGIMGUI_CMD_INIT_BUFFER:
            {
                sgimgui_str_t res_id = _sgimgui_buffer_id_string(ctx, item->args.init_buffer.buffer);
                _sgimgui_snprintf(&str, "%d: sg_init_buffer(buf=%s, desc=..)", index, res_id.buf);
            }
            break;

        case SGIMGUI_CMD_INIT_IMAGE:
            {
                sgimgui_str_t res_id = _sgimgui_image_id_string(ctx, item->args.init_image.image);
                _sgimgui_snprintf(&str, "%d: sg_init_image(img=%s, desc=..)", index, res_id.buf);
            }
            break;

        case SGIMGUI_CMD_INIT_SAMPLER:
            {
                sgimgui_str_t res_id = _sgimgui_sampler_id_string(ctx, item->args.init_sampler.sampler);
                _sgimgui_snprintf(&str, "%d: sg_init_sampler(smp=%s, desc=..)", index, res_id.buf);
            }
            break;

        case SGIMGUI_CMD_INIT_SHADER:
            {
                sgimgui_str_t res_id = _sgimgui_shader_id_string(ctx, item->args.init_shader.shader);
                _sgimgui_snprintf(&str, "%d: sg_init_shader(shd=%s, desc=..)", index, res_id.buf);
            }
            break;

        case SGIMGUI_CMD_INIT_PIPELINE:
            {
                sgimgui_str_t res_id = _sgimgui_pipeline_id_string(ctx, item->args.init_pipeline.pipeline);
                _sgimgui_snprintf(&str, "%d: sg_init_pipeline(pip=%s, desc=..)", index, res_id.buf);
            }
            break;

        case SGIMGUI_CMD_INIT_ATTACHMENTS:
            {
                sgimgui_str_t res_id = _sgimgui_attachments_id_string(ctx, item->args.init_attachments.attachments);
                _sgimgui_snprintf(&str, "%d: sg_init_attachments(atts=%s, desc=..)", index, res_id.buf);
            }
            break;

        case SGIMGUI_CMD_UNINIT_BUFFER:
            {
                sgimgui_str_t res_id = _sgimgui_buffer_id_string(ctx, item->args.uninit_buffer.buffer);
                _sgimgui_snprintf(&str, "%d: sg_uninit_buffer(buf=%s)", index, res_id.buf);
            }
            break;

        case SGIMGUI_CMD_UNINIT_IMAGE:
            {
                sgimgui_str_t res_id = _sgimgui_image_id_string(ctx, item->args.uninit_image.image);
                _sgimgui_snprintf(&str, "%d: sg_uninit_image(img=%s)", index, res_id.buf);
            }
            break;

        case SGIMGUI_CMD_UNINIT_SAMPLER:
            {
                sgimgui_str_t res_id = _sgimgui_sampler_id_string(ctx, item->args.uninit_sampler.sampler);
                _sgimgui_snprintf(&str, "%d: sg_uninit_sampler(smp=%s)", index, res_id.buf);
            }
            break;

        case SGIMGUI_CMD_UNINIT_SHADER:
            {
                sgimgui_str_t res_id = _sgimgui_shader_id_string(ctx, item->args.uninit_shader.shader);
                _sgimgui_snprintf(&str, "%d: sg_uninit_shader(shd=%s)", index, res_id.buf);
            }
            break;

        case SGIMGUI_CMD_UNINIT_PIPELINE:
            {
                sgimgui_str_t res_id = _sgimgui_pipeline_id_string(ctx, item->args.uninit_pipeline.pipeline);
                _sgimgui_snprintf(&str, "%d: sg_uninit_pipeline(pip=%s)", index, res_id.buf);
            }
            break;

        case SGIMGUI_CMD_UNINIT_ATTACHMENTS:
            {
                sgimgui_str_t res_id = _sgimgui_attachments_id_string(ctx, item->args.uninit_attachments.attachments);
                _sgimgui_snprintf(&str, "%d: sg_uninit_attachments(atts=%s)", index, res_id.buf);
            }
            break;

        case SGIMGUI_CMD_FAIL_BUFFER:
            {
                sgimgui_str_t res_id = _sgimgui_buffer_id_string(ctx, item->args.fail_buffer.buffer);
                _sgimgui_snprintf(&str, "%d: sg_fail_buffer(buf=%s)", index, res_id.buf);
            }
            break;

        case SGIMGUI_CMD_FAIL_IMAGE:
            {
                sgimgui_str_t res_id = _sgimgui_image_id_string(ctx, item->args.fail_image.image);
                _sgimgui_snprintf(&str, "%d: sg_fail_image(img=%s)", index, res_id.buf);
            }
            break;

        case SGIMGUI_CMD_FAIL_SAMPLER:
            {
                sgimgui_str_t res_id = _sgimgui_sampler_id_string(ctx, item->args.fail_sampler.sampler);
                _sgimgui_snprintf(&str, "%d: sg_fail_sampler(smp=%s)", index, res_id.buf);
            }
            break;

        case SGIMGUI_CMD_FAIL_SHADER:
            {
                sgimgui_str_t res_id = _sgimgui_shader_id_string(ctx, item->args.fail_shader.shader);
                _sgimgui_snprintf(&str, "%d: sg_fail_shader(shd=%s)", index, res_id.buf);
            }
            break;

        case SGIMGUI_CMD_FAIL_PIPELINE:
            {
                sgimgui_str_t res_id = _sgimgui_pipeline_id_string(ctx, item->args.fail_pipeline.pipeline);
                _sgimgui_snprintf(&str, "%d: sg_fail_pipeline(shd=%s)", index, res_id.buf);
            }
            break;

        case SGIMGUI_CMD_FAIL_ATTACHMENTS:
            {
                sgimgui_str_t res_id = _sgimgui_attachments_id_string(ctx, item->args.fail_attachments.attachments);
                _sgimgui_snprintf(&str, "%d: sg_fail_attachments(atts=%s)", index, res_id.buf);
            }
            break;

        case SGIMGUI_CMD_PUSH_DEBUG_GROUP:
            _sgimgui_snprintf(&str, "%d: sg_push_debug_group(name=%s)", index,
                item->args.push_debug_group.name.buf);
            break;

        case SGIMGUI_CMD_POP_DEBUG_GROUP:
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
    sgimgui_t* ctx = (sgimgui_t*) user_data;
    SOKOL_ASSERT(ctx);
    sgimgui_capture_item_t* item = _sgimgui_capture_next_write_item(ctx);
    if (item) {
        item->cmd = SGIMGUI_CMD_RESET_STATE_CACHE;
        item->color = _SGIMGUI_COLOR_OTHER;
    }
    if (ctx->hooks.reset_state_cache) {
        ctx->hooks.reset_state_cache(ctx->hooks.user_data);
    }
}

_SOKOL_PRIVATE void _sgimgui_make_buffer(const sg_buffer_desc* desc, sg_buffer buf_id, void* user_data) {
    sgimgui_t* ctx = (sgimgui_t*) user_data;
    SOKOL_ASSERT(ctx);
    sgimgui_capture_item_t* item = _sgimgui_capture_next_write_item(ctx);
    if (item) {
        item->cmd = SGIMGUI_CMD_MAKE_BUFFER;
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
    sgimgui_t* ctx = (sgimgui_t*) user_data;
    SOKOL_ASSERT(ctx);
    sgimgui_capture_item_t* item = _sgimgui_capture_next_write_item(ctx);
    if (item) {
        item->cmd = SGIMGUI_CMD_MAKE_IMAGE;
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
    sgimgui_t* ctx = (sgimgui_t*) user_data;
    SOKOL_ASSERT(ctx);
    sgimgui_capture_item_t* item = _sgimgui_capture_next_write_item(ctx);
    if (item) {
        item->cmd = SGIMGUI_CMD_MAKE_SAMPLER;
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
    sgimgui_t* ctx = (sgimgui_t*) user_data;
    SOKOL_ASSERT(ctx);
    sgimgui_capture_item_t* item = _sgimgui_capture_next_write_item(ctx);
    if (item) {
        item->cmd = SGIMGUI_CMD_MAKE_SHADER;
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
    sgimgui_t* ctx = (sgimgui_t*) user_data;
    SOKOL_ASSERT(ctx);
    sgimgui_capture_item_t* item = _sgimgui_capture_next_write_item(ctx);
    if (item) {
        item->cmd = SGIMGUI_CMD_MAKE_PIPELINE;
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

_SOKOL_PRIVATE void _sgimgui_make_attachments(const sg_attachments_desc* desc, sg_attachments atts_id, void* user_data) {
    sgimgui_t* ctx = (sgimgui_t*) user_data;
    SOKOL_ASSERT(ctx);
    sgimgui_capture_item_t* item = _sgimgui_capture_next_write_item(ctx);
    if (item) {
        item->cmd = SGIMGUI_CMD_MAKE_ATTACHMENTS;
        item->color = _SGIMGUI_COLOR_RSRC;
        item->args.make_attachments.result = atts_id;
    }
    if (ctx->hooks.make_attachments) {
        ctx->hooks.make_attachments(desc, atts_id, ctx->hooks.user_data);
    }
    if (atts_id.id != SG_INVALID_ID) {
        _sgimgui_attachments_created(ctx, atts_id, _sgimgui_slot_index(atts_id.id), desc);
    }
}

_SOKOL_PRIVATE void _sgimgui_destroy_buffer(sg_buffer buf, void* user_data) {
    sgimgui_t* ctx = (sgimgui_t*) user_data;
    SOKOL_ASSERT(ctx);
    sgimgui_capture_item_t* item = _sgimgui_capture_next_write_item(ctx);
    if (item) {
        item->cmd = SGIMGUI_CMD_DESTROY_BUFFER;
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
    sgimgui_t* ctx = (sgimgui_t*) user_data;
    SOKOL_ASSERT(ctx);
    sgimgui_capture_item_t* item = _sgimgui_capture_next_write_item(ctx);
    if (item) {
        item->cmd = SGIMGUI_CMD_DESTROY_IMAGE;
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
    sgimgui_t* ctx = (sgimgui_t*) user_data;
    SOKOL_ASSERT(ctx);
    sgimgui_capture_item_t* item = _sgimgui_capture_next_write_item(ctx);
    if (item) {
        item->cmd = SGIMGUI_CMD_DESTROY_SAMPLER;
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
    sgimgui_t* ctx = (sgimgui_t*) user_data;
    SOKOL_ASSERT(ctx);
    sgimgui_capture_item_t* item = _sgimgui_capture_next_write_item(ctx);
    if (item) {
        item->cmd = SGIMGUI_CMD_DESTROY_SHADER;
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
    sgimgui_t* ctx = (sgimgui_t*) user_data;
    SOKOL_ASSERT(ctx);
    sgimgui_capture_item_t* item = _sgimgui_capture_next_write_item(ctx);
    if (item) {
        item->cmd = SGIMGUI_CMD_DESTROY_PIPELINE;
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

_SOKOL_PRIVATE void _sgimgui_destroy_attachments(sg_attachments atts, void* user_data) {
    sgimgui_t* ctx = (sgimgui_t*) user_data;
    SOKOL_ASSERT(ctx);
    sgimgui_capture_item_t* item = _sgimgui_capture_next_write_item(ctx);
    if (item) {
        item->cmd = SGIMGUI_CMD_DESTROY_ATTACHMENTS;
        item->color = _SGIMGUI_COLOR_RSRC;
        item->args.destroy_attachments.attachments = atts;
    }
    if (ctx->hooks.destroy_attachments) {
        ctx->hooks.destroy_attachments(atts, ctx->hooks.user_data);
    }
    if (atts.id != SG_INVALID_ID) {
        _sgimgui_attachments_destroyed(ctx, _sgimgui_slot_index(atts.id));
    }
}

_SOKOL_PRIVATE void _sgimgui_update_buffer(sg_buffer buf, const sg_range* data, void* user_data) {
    sgimgui_t* ctx = (sgimgui_t*) user_data;
    SOKOL_ASSERT(ctx);
    sgimgui_capture_item_t* item = _sgimgui_capture_next_write_item(ctx);
    if (item) {
        item->cmd = SGIMGUI_CMD_UPDATE_BUFFER;
        item->color = _SGIMGUI_COLOR_RSRC;
        item->args.update_buffer.buffer = buf;
        item->args.update_buffer.data_size = data->size;
    }
    if (ctx->hooks.update_buffer) {
        ctx->hooks.update_buffer(buf, data, ctx->hooks.user_data);
    }
}

_SOKOL_PRIVATE void _sgimgui_update_image(sg_image img, const sg_image_data* data, void* user_data) {
    sgimgui_t* ctx = (sgimgui_t*) user_data;
    SOKOL_ASSERT(ctx);
    sgimgui_capture_item_t* item = _sgimgui_capture_next_write_item(ctx);
    if (item) {
        item->cmd = SGIMGUI_CMD_UPDATE_IMAGE;
        item->color = _SGIMGUI_COLOR_RSRC;
        item->args.update_image.image = img;
    }
    if (ctx->hooks.update_image) {
        ctx->hooks.update_image(img, data, ctx->hooks.user_data);
    }
}

_SOKOL_PRIVATE void _sgimgui_append_buffer(sg_buffer buf, const sg_range* data, int result, void* user_data) {
    sgimgui_t* ctx = (sgimgui_t*) user_data;
    SOKOL_ASSERT(ctx);
    sgimgui_capture_item_t* item = _sgimgui_capture_next_write_item(ctx);
    if (item) {
        item->cmd = SGIMGUI_CMD_APPEND_BUFFER;
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
    sgimgui_t* ctx = (sgimgui_t*) user_data;
    SOKOL_ASSERT(ctx);
    sgimgui_capture_item_t* item = _sgimgui_capture_next_write_item(ctx);
    if (item) {
        SOKOL_ASSERT(pass);
        item->cmd = SGIMGUI_CMD_BEGIN_PASS;
        item->color = _SGIMGUI_COLOR_PASS;
        item->args.begin_pass.pass = *pass;
    }
    if (ctx->hooks.begin_pass) {
        ctx->hooks.begin_pass(pass, ctx->hooks.user_data);
    }
}

_SOKOL_PRIVATE void _sgimgui_apply_viewport(int x, int y, int width, int height, bool origin_top_left, void* user_data) {
    sgimgui_t* ctx = (sgimgui_t*) user_data;
    SOKOL_ASSERT(ctx);
    sgimgui_capture_item_t* item = _sgimgui_capture_next_write_item(ctx);
    if (item) {
        item->cmd = SGIMGUI_CMD_APPLY_VIEWPORT;
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
    sgimgui_t* ctx = (sgimgui_t*) user_data;
    SOKOL_ASSERT(ctx);
    sgimgui_capture_item_t* item = _sgimgui_capture_next_write_item(ctx);
    if (item) {
        item->cmd = SGIMGUI_CMD_APPLY_SCISSOR_RECT;
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
    sgimgui_t* ctx = (sgimgui_t*) user_data;
    SOKOL_ASSERT(ctx);
    ctx->cur_pipeline = pip;    /* stored for _sgimgui_apply_uniforms */
    sgimgui_capture_item_t* item = _sgimgui_capture_next_write_item(ctx);
    if (item) {
        item->cmd = SGIMGUI_CMD_APPLY_PIPELINE;
        item->color = _SGIMGUI_COLOR_APPLY;
        item->args.apply_pipeline.pipeline = pip;
    }
    if (ctx->hooks.apply_pipeline) {
        ctx->hooks.apply_pipeline(pip, ctx->hooks.user_data);
    }
}

_SOKOL_PRIVATE void _sgimgui_apply_bindings(const sg_bindings* bindings, void* user_data) {
    sgimgui_t* ctx = (sgimgui_t*) user_data;
    SOKOL_ASSERT(ctx);
    sgimgui_capture_item_t* item = _sgimgui_capture_next_write_item(ctx);
    if (item) {
        SOKOL_ASSERT(bindings);
        item->cmd = SGIMGUI_CMD_APPLY_BINDINGS;
        item->color = _SGIMGUI_COLOR_APPLY;
        item->args.apply_bindings.bindings = *bindings;
    }
    if (ctx->hooks.apply_bindings) {
        ctx->hooks.apply_bindings(bindings, ctx->hooks.user_data);
    }
}

_SOKOL_PRIVATE void _sgimgui_apply_uniforms(sg_shader_stage stage, int ub_index, const sg_range* data, void* user_data) {
    sgimgui_t* ctx = (sgimgui_t*) user_data;
    SOKOL_ASSERT(ctx);
    SOKOL_ASSERT(data);
    sgimgui_capture_item_t* item = _sgimgui_capture_next_write_item(ctx);
    if (item) {
        item->cmd = SGIMGUI_CMD_APPLY_UNIFORMS;
        item->color = _SGIMGUI_COLOR_APPLY;
        sgimgui_args_apply_uniforms_t* args = &item->args.apply_uniforms;
        args->stage = stage;
        args->ub_index = ub_index;
        args->data_size = data->size;
        args->pipeline = ctx->cur_pipeline;
        args->ubuf_pos = _sgimgui_capture_uniforms(ctx, data);
    }
    if (ctx->hooks.apply_uniforms) {
        ctx->hooks.apply_uniforms(stage, ub_index, data, ctx->hooks.user_data);
    }
}

_SOKOL_PRIVATE void _sgimgui_draw(int base_element, int num_elements, int num_instances, void* user_data) {
    sgimgui_t* ctx = (sgimgui_t*) user_data;
    SOKOL_ASSERT(ctx);
    sgimgui_capture_item_t* item = _sgimgui_capture_next_write_item(ctx);
    if (item) {
        item->cmd = SGIMGUI_CMD_DRAW;
        item->color = _SGIMGUI_COLOR_DRAW;
        item->args.draw.base_element = base_element;
        item->args.draw.num_elements = num_elements;
        item->args.draw.num_instances = num_instances;
    }
    if (ctx->hooks.draw) {
        ctx->hooks.draw(base_element, num_elements, num_instances, ctx->hooks.user_data);
    }
}

_SOKOL_PRIVATE void _sgimgui_end_pass(void* user_data) {
    sgimgui_t* ctx = (sgimgui_t*) user_data;
    SOKOL_ASSERT(ctx);
    ctx->cur_pipeline.id = SG_INVALID_ID;
    sgimgui_capture_item_t* item = _sgimgui_capture_next_write_item(ctx);
    if (item) {
        item->cmd = SGIMGUI_CMD_END_PASS;
        item->color = _SGIMGUI_COLOR_PASS;
    }
    if (ctx->hooks.end_pass) {
        ctx->hooks.end_pass(ctx->hooks.user_data);
    }
}

_SOKOL_PRIVATE void _sgimgui_commit(void* user_data) {
    sgimgui_t* ctx = (sgimgui_t*) user_data;
    SOKOL_ASSERT(ctx);
    sgimgui_capture_item_t* item = _sgimgui_capture_next_write_item(ctx);
    if (item) {
        item->cmd = SGIMGUI_CMD_COMMIT;
        item->color = _SGIMGUI_COLOR_OTHER;
    }
    _sgimgui_capture_next_frame(ctx);
    if (ctx->hooks.commit) {
        ctx->hooks.commit(ctx->hooks.user_data);
    }
}

_SOKOL_PRIVATE void _sgimgui_alloc_buffer(sg_buffer result, void* user_data) {
    sgimgui_t* ctx = (sgimgui_t*) user_data;
    SOKOL_ASSERT(ctx);
    sgimgui_capture_item_t* item = _sgimgui_capture_next_write_item(ctx);
    if (item) {
        item->cmd = SGIMGUI_CMD_ALLOC_BUFFER;
        item->color = _SGIMGUI_COLOR_RSRC;
        item->args.alloc_buffer.result = result;
    }
    if (ctx->hooks.alloc_buffer) {
        ctx->hooks.alloc_buffer(result, ctx->hooks.user_data);
    }
}

_SOKOL_PRIVATE void _sgimgui_alloc_image(sg_image result, void* user_data) {
    sgimgui_t* ctx = (sgimgui_t*) user_data;
    SOKOL_ASSERT(ctx);
    sgimgui_capture_item_t* item = _sgimgui_capture_next_write_item(ctx);
    if (item) {
        item->cmd = SGIMGUI_CMD_ALLOC_IMAGE;
        item->color = _SGIMGUI_COLOR_RSRC;
        item->args.alloc_image.result = result;
    }
    if (ctx->hooks.alloc_image) {
        ctx->hooks.alloc_image(result, ctx->hooks.user_data);
    }
}

_SOKOL_PRIVATE void _sgimgui_alloc_sampler(sg_sampler result, void* user_data) {
    sgimgui_t* ctx = (sgimgui_t*) user_data;
    SOKOL_ASSERT(ctx);
    sgimgui_capture_item_t* item = _sgimgui_capture_next_write_item(ctx);
    if (item) {
        item->cmd = SGIMGUI_CMD_ALLOC_SAMPLER;
        item->color = _SGIMGUI_COLOR_RSRC;
        item->args.alloc_sampler.result = result;
    }
    if (ctx->hooks.alloc_sampler) {
        ctx->hooks.alloc_sampler(result, ctx->hooks.user_data);
    }
}

_SOKOL_PRIVATE void _sgimgui_alloc_shader(sg_shader result, void* user_data) {
    sgimgui_t* ctx = (sgimgui_t*) user_data;
    SOKOL_ASSERT(ctx);
    sgimgui_capture_item_t* item = _sgimgui_capture_next_write_item(ctx);
    if (item) {
        item->cmd = SGIMGUI_CMD_ALLOC_SHADER;
        item->color = _SGIMGUI_COLOR_RSRC;
        item->args.alloc_shader.result = result;
    }
    if (ctx->hooks.alloc_shader) {
        ctx->hooks.alloc_shader(result, ctx->hooks.user_data);
    }
}

_SOKOL_PRIVATE void _sgimgui_alloc_pipeline(sg_pipeline result, void* user_data) {
    sgimgui_t* ctx = (sgimgui_t*) user_data;
    SOKOL_ASSERT(ctx);
    sgimgui_capture_item_t* item = _sgimgui_capture_next_write_item(ctx);
    if (item) {
        item->cmd = SGIMGUI_CMD_ALLOC_PIPELINE;
        item->color = _SGIMGUI_COLOR_RSRC;
        item->args.alloc_pipeline.result = result;
    }
    if (ctx->hooks.alloc_pipeline) {
        ctx->hooks.alloc_pipeline(result, ctx->hooks.user_data);
    }
}

_SOKOL_PRIVATE void _sgimgui_alloc_attachments(sg_attachments result, void* user_data) {
    sgimgui_t* ctx = (sgimgui_t*) user_data;
    SOKOL_ASSERT(ctx);
    sgimgui_capture_item_t* item = _sgimgui_capture_next_write_item(ctx);
    if (item) {
        item->cmd = SGIMGUI_CMD_ALLOC_ATTACHMENTS;
        item->color = _SGIMGUI_COLOR_RSRC;
        item->args.alloc_attachments.result = result;
    }
    if (ctx->hooks.alloc_attachments) {
        ctx->hooks.alloc_attachments(result, ctx->hooks.user_data);
    }
}

_SOKOL_PRIVATE void _sgimgui_dealloc_buffer(sg_buffer buf_id, void* user_data) {
    sgimgui_t* ctx = (sgimgui_t*) user_data;
    SOKOL_ASSERT(ctx);
    sgimgui_capture_item_t* item = _sgimgui_capture_next_write_item(ctx);
    if (item) {
        item->cmd = SGIMGUI_CMD_DEALLOC_BUFFER;
        item->color = _SGIMGUI_COLOR_RSRC;
        item->args.dealloc_buffer.buffer = buf_id;
    }
    if (ctx->hooks.dealloc_buffer) {
        ctx->hooks.dealloc_buffer(buf_id, ctx->hooks.user_data);
    }
}

_SOKOL_PRIVATE void _sgimgui_dealloc_image(sg_image img_id, void* user_data) {
    sgimgui_t* ctx = (sgimgui_t*) user_data;
    SOKOL_ASSERT(ctx);
    sgimgui_capture_item_t* item = _sgimgui_capture_next_write_item(ctx);
    if (item) {
        item->cmd = SGIMGUI_CMD_DEALLOC_IMAGE;
        item->color = _SGIMGUI_COLOR_RSRC;
        item->args.dealloc_image.image = img_id;
    }
    if (ctx->hooks.dealloc_image) {
        ctx->hooks.dealloc_image(img_id, ctx->hooks.user_data);
    }
}

_SOKOL_PRIVATE void _sgimgui_dealloc_sampler(sg_sampler smp_id, void* user_data) {
    sgimgui_t* ctx = (sgimgui_t*) user_data;
    SOKOL_ASSERT(ctx);
    sgimgui_capture_item_t* item = _sgimgui_capture_next_write_item(ctx);
    if (item) {
        item->cmd = SGIMGUI_CMD_DEALLOC_SAMPLER;
        item->color = _SGIMGUI_COLOR_RSRC;
        item->args.dealloc_sampler.sampler = smp_id;
    }
    if (ctx->hooks.dealloc_sampler) {
        ctx->hooks.dealloc_sampler(smp_id, ctx->hooks.user_data);
    }
}

_SOKOL_PRIVATE void _sgimgui_dealloc_shader(sg_shader shd_id, void* user_data) {
    sgimgui_t* ctx = (sgimgui_t*) user_data;
    SOKOL_ASSERT(ctx);
    sgimgui_capture_item_t* item = _sgimgui_capture_next_write_item(ctx);
    if (item) {
        item->cmd = SGIMGUI_CMD_DEALLOC_SHADER;
        item->color = _SGIMGUI_COLOR_RSRC;
        item->args.dealloc_shader.shader = shd_id;
    }
    if (ctx->hooks.dealloc_shader) {
        ctx->hooks.dealloc_shader(shd_id, ctx->hooks.user_data);
    }
}

_SOKOL_PRIVATE void _sgimgui_dealloc_pipeline(sg_pipeline pip_id, void* user_data) {
    sgimgui_t* ctx = (sgimgui_t*) user_data;
    SOKOL_ASSERT(ctx);
    sgimgui_capture_item_t* item = _sgimgui_capture_next_write_item(ctx);
    if (item) {
        item->cmd = SGIMGUI_CMD_DEALLOC_PIPELINE;
        item->color = _SGIMGUI_COLOR_RSRC;
        item->args.dealloc_pipeline.pipeline = pip_id;
    }
    if (ctx->hooks.dealloc_pipeline) {
        ctx->hooks.dealloc_pipeline(pip_id, ctx->hooks.user_data);
    }
}

_SOKOL_PRIVATE void _sgimgui_dealloc_attachments(sg_attachments atts_id, void* user_data) {
    sgimgui_t* ctx = (sgimgui_t*) user_data;
    SOKOL_ASSERT(ctx);
    sgimgui_capture_item_t* item = _sgimgui_capture_next_write_item(ctx);
    if (item) {
        item->cmd = SGIMGUI_CMD_DEALLOC_ATTACHMENTS;
        item->color = _SGIMGUI_COLOR_RSRC;
        item->args.dealloc_attachments.attachments = atts_id;
    }
    if (ctx->hooks.dealloc_attachments) {
        ctx->hooks.dealloc_attachments(atts_id, ctx->hooks.user_data);
    }
}

_SOKOL_PRIVATE void _sgimgui_init_buffer(sg_buffer buf_id, const sg_buffer_desc* desc, void* user_data) {
    sgimgui_t* ctx = (sgimgui_t*) user_data;
    SOKOL_ASSERT(ctx);
    sgimgui_capture_item_t* item = _sgimgui_capture_next_write_item(ctx);
    if (item) {
        item->cmd = SGIMGUI_CMD_INIT_BUFFER;
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
    sgimgui_t* ctx = (sgimgui_t*) user_data;
    SOKOL_ASSERT(ctx);
    sgimgui_capture_item_t* item = _sgimgui_capture_next_write_item(ctx);
    if (item) {
        item->cmd = SGIMGUI_CMD_INIT_IMAGE;
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
    sgimgui_t* ctx = (sgimgui_t*) user_data;
    SOKOL_ASSERT(ctx);
    sgimgui_capture_item_t* item = _sgimgui_capture_next_write_item(ctx);
    if (item) {
        item->cmd = SGIMGUI_CMD_INIT_SAMPLER;
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
    sgimgui_t* ctx = (sgimgui_t*) user_data;
    SOKOL_ASSERT(ctx);
    sgimgui_capture_item_t* item = _sgimgui_capture_next_write_item(ctx);
    if (item) {
        item->cmd = SGIMGUI_CMD_INIT_SHADER;
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
    sgimgui_t* ctx = (sgimgui_t*) user_data;
    SOKOL_ASSERT(ctx);
    sgimgui_capture_item_t* item = _sgimgui_capture_next_write_item(ctx);
    if (item) {
        item->cmd = SGIMGUI_CMD_INIT_PIPELINE;
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

_SOKOL_PRIVATE void _sgimgui_init_attachments(sg_attachments atts_id, const sg_attachments_desc* desc, void* user_data) {
    sgimgui_t* ctx = (sgimgui_t*) user_data;
    SOKOL_ASSERT(ctx);
    sgimgui_capture_item_t* item = _sgimgui_capture_next_write_item(ctx);
    if (item) {
        item->cmd = SGIMGUI_CMD_INIT_ATTACHMENTS;
        item->color = _SGIMGUI_COLOR_RSRC;
        item->args.init_attachments.attachments = atts_id;
    }
    if (ctx->hooks.init_attachments) {
        ctx->hooks.init_attachments(atts_id, desc, ctx->hooks.user_data);
    }
    if (atts_id.id != SG_INVALID_ID) {
        _sgimgui_attachments_created(ctx, atts_id, _sgimgui_slot_index(atts_id.id), desc);
    }
}

_SOKOL_PRIVATE void _sgimgui_uninit_buffer(sg_buffer buf, void* user_data) {
    sgimgui_t* ctx = (sgimgui_t*) user_data;
    SOKOL_ASSERT(ctx);
    sgimgui_capture_item_t* item = _sgimgui_capture_next_write_item(ctx);
    if (item) {
        item->cmd = SGIMGUI_CMD_UNINIT_BUFFER;
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
    sgimgui_t* ctx = (sgimgui_t*) user_data;
    SOKOL_ASSERT(ctx);
    sgimgui_capture_item_t* item = _sgimgui_capture_next_write_item(ctx);
    if (item) {
        item->cmd = SGIMGUI_CMD_UNINIT_IMAGE;
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
    sgimgui_t* ctx = (sgimgui_t*) user_data;
    SOKOL_ASSERT(ctx);
    sgimgui_capture_item_t* item = _sgimgui_capture_next_write_item(ctx);
    if (item) {
        item->cmd = SGIMGUI_CMD_UNINIT_SAMPLER;
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
    sgimgui_t* ctx = (sgimgui_t*) user_data;
    SOKOL_ASSERT(ctx);
    sgimgui_capture_item_t* item = _sgimgui_capture_next_write_item(ctx);
    if (item) {
        item->cmd = SGIMGUI_CMD_UNINIT_SHADER;
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
    sgimgui_t* ctx = (sgimgui_t*) user_data;
    SOKOL_ASSERT(ctx);
    sgimgui_capture_item_t* item = _sgimgui_capture_next_write_item(ctx);
    if (item) {
        item->cmd = SGIMGUI_CMD_UNINIT_PIPELINE;
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

_SOKOL_PRIVATE void _sgimgui_uninit_attachments(sg_attachments atts, void* user_data) {
    sgimgui_t* ctx = (sgimgui_t*) user_data;
    SOKOL_ASSERT(ctx);
    sgimgui_capture_item_t* item = _sgimgui_capture_next_write_item(ctx);
    if (item) {
        item->cmd = SGIMGUI_CMD_UNINIT_PIPELINE;
        item->color = _SGIMGUI_COLOR_RSRC;
        item->args.uninit_attachments.attachments = atts;
    }
    if (ctx->hooks.uninit_attachments) {
        ctx->hooks.uninit_attachments(atts, ctx->hooks.user_data);
    }
    if (atts.id != SG_INVALID_ID) {
        _sgimgui_attachments_destroyed(ctx, _sgimgui_slot_index(atts.id));
    }
}

_SOKOL_PRIVATE void _sgimgui_fail_buffer(sg_buffer buf_id, void* user_data) {
    sgimgui_t* ctx = (sgimgui_t*) user_data;
    SOKOL_ASSERT(ctx);
    sgimgui_capture_item_t* item = _sgimgui_capture_next_write_item(ctx);
    if (item) {
        item->cmd = SGIMGUI_CMD_FAIL_BUFFER;
        item->color = _SGIMGUI_COLOR_RSRC;
        item->args.fail_buffer.buffer = buf_id;
    }
    if (ctx->hooks.fail_buffer) {
        ctx->hooks.fail_buffer(buf_id, ctx->hooks.user_data);
    }
}

_SOKOL_PRIVATE void _sgimgui_fail_image(sg_image img_id, void* user_data) {
    sgimgui_t* ctx = (sgimgui_t*) user_data;
    SOKOL_ASSERT(ctx);
    sgimgui_capture_item_t* item = _sgimgui_capture_next_write_item(ctx);
    if (item) {
        item->cmd = SGIMGUI_CMD_FAIL_IMAGE;
        item->color = _SGIMGUI_COLOR_RSRC;
        item->args.fail_image.image = img_id;
    }
    if (ctx->hooks.fail_image) {
        ctx->hooks.fail_image(img_id, ctx->hooks.user_data);
    }
}

_SOKOL_PRIVATE void _sgimgui_fail_sampler(sg_sampler smp_id, void* user_data) {
    sgimgui_t* ctx = (sgimgui_t*) user_data;
    SOKOL_ASSERT(ctx);
    sgimgui_capture_item_t* item = _sgimgui_capture_next_write_item(ctx);
    if (item) {
        item->cmd = SGIMGUI_CMD_FAIL_SAMPLER;
        item->color = _SGIMGUI_COLOR_RSRC;
        item->args.fail_sampler.sampler = smp_id;
    }
    if (ctx->hooks.fail_sampler) {
        ctx->hooks.fail_sampler(smp_id, ctx->hooks.user_data);
    }
}

_SOKOL_PRIVATE void _sgimgui_fail_shader(sg_shader shd_id, void* user_data) {
    sgimgui_t* ctx = (sgimgui_t*) user_data;
    SOKOL_ASSERT(ctx);
    sgimgui_capture_item_t* item = _sgimgui_capture_next_write_item(ctx);
    if (item) {
        item->cmd = SGIMGUI_CMD_FAIL_SHADER;
        item->color = _SGIMGUI_COLOR_RSRC;
        item->args.fail_shader.shader = shd_id;
    }
    if (ctx->hooks.fail_shader) {
        ctx->hooks.fail_shader(shd_id, ctx->hooks.user_data);
    }
}

_SOKOL_PRIVATE void _sgimgui_fail_pipeline(sg_pipeline pip_id, void* user_data) {
    sgimgui_t* ctx = (sgimgui_t*) user_data;
    SOKOL_ASSERT(ctx);
    sgimgui_capture_item_t* item = _sgimgui_capture_next_write_item(ctx);
    if (item) {
        item->cmd = SGIMGUI_CMD_FAIL_PIPELINE;
        item->color = _SGIMGUI_COLOR_RSRC;
        item->args.fail_pipeline.pipeline = pip_id;
    }
    if (ctx->hooks.fail_pipeline) {
        ctx->hooks.fail_pipeline(pip_id, ctx->hooks.user_data);
    }
}

_SOKOL_PRIVATE void _sgimgui_fail_attachments(sg_attachments atts_id, void* user_data) {
    sgimgui_t* ctx = (sgimgui_t*) user_data;
    SOKOL_ASSERT(ctx);
    sgimgui_capture_item_t* item = _sgimgui_capture_next_write_item(ctx);
    if (item) {
        item->cmd = SGIMGUI_CMD_FAIL_ATTACHMENTS;
        item->color = _SGIMGUI_COLOR_RSRC;
        item->args.fail_attachments.attachments = atts_id;
    }
    if (ctx->hooks.fail_attachments) {
        ctx->hooks.fail_attachments(atts_id, ctx->hooks.user_data);
    }
}

_SOKOL_PRIVATE void _sgimgui_push_debug_group(const char* name, void* user_data) {
    sgimgui_t* ctx = (sgimgui_t*) user_data;
    SOKOL_ASSERT(ctx);
    if (0 == strcmp(name, "sokol-imgui")) {
        ctx->frame_stats_window.in_sokol_imgui = true;
        if (ctx->frame_stats_window.disable_sokol_imgui_stats) {
            sg_disable_frame_stats();
        }
    }
    sgimgui_capture_item_t* item = _sgimgui_capture_next_write_item(ctx);
    if (item) {
        item->cmd = SGIMGUI_CMD_PUSH_DEBUG_GROUP;
        item->color = _SGIMGUI_COLOR_OTHER;
        item->args.push_debug_group.name = _sgimgui_make_str(name);
    }
    if (ctx->hooks.push_debug_group) {
        ctx->hooks.push_debug_group(name, ctx->hooks.user_data);
    }
}

_SOKOL_PRIVATE void _sgimgui_pop_debug_group(void* user_data) {
    sgimgui_t* ctx = (sgimgui_t*) user_data;
    SOKOL_ASSERT(ctx);
    if (ctx->frame_stats_window.in_sokol_imgui) {
        ctx->frame_stats_window.in_sokol_imgui = false;
        if (ctx->frame_stats_window.disable_sokol_imgui_stats) {
            sg_enable_frame_stats();
        }
    }
    sgimgui_capture_item_t* item = _sgimgui_capture_next_write_item(ctx);
    if (item) {
        item->cmd = SGIMGUI_CMD_POP_DEBUG_GROUP;
        item->color = _SGIMGUI_COLOR_OTHER;
    }
    if (ctx->hooks.pop_debug_group) {
        ctx->hooks.pop_debug_group(ctx->hooks.user_data);
    }
}

/*--- IMGUI HELPERS ----------------------------------------------------------*/
_SOKOL_PRIVATE bool _sgimgui_draw_resid_list_item(uint32_t res_id, const char* label, bool selected) {
    igPushID_Int((int)res_id);
    bool res;
    if (label[0]) {
        res = igSelectable_Bool(label, selected, 0, IMVEC2(0,0));
    } else {
        sgimgui_str_t str;
        _sgimgui_snprintf(&str, "0x%08X", res_id);
        res = igSelectable_Bool(str.buf, selected, 0, IMVEC2(0,0));
    }
    igPopID();
    return res;
}

_SOKOL_PRIVATE bool _sgimgui_draw_resid_link(uint32_t res_type, uint32_t res_id, const char* label) {
    SOKOL_ASSERT(label);
    sgimgui_str_t str_buf;
    const char* str;
    if (label[0]) {
        str = label;
    } else {
        _sgimgui_snprintf(&str_buf, "0x%08X", res_id);
        str = str_buf.buf;
    }
    igPushID_Int((int)((res_type<<24)|res_id));
    bool res = igSmallButton(str);
    igPopID();
    return res;
}

_SOKOL_PRIVATE bool _sgimgui_draw_buffer_link(sgimgui_t* ctx, sg_buffer buf) {
    bool retval = false;
    if (buf.id != SG_INVALID_ID) {
        const sgimgui_buffer_t* buf_ui = &ctx->buffer_window.slots[_sgimgui_slot_index(buf.id)];
        retval = _sgimgui_draw_resid_link(1, buf.id, buf_ui->label.buf);
    }
    return retval;
}

_SOKOL_PRIVATE bool _sgimgui_draw_image_link(sgimgui_t* ctx, sg_image img) {
    bool retval = false;
    if (img.id != SG_INVALID_ID) {
        const sgimgui_image_t* img_ui = &ctx->image_window.slots[_sgimgui_slot_index(img.id)];
        retval = _sgimgui_draw_resid_link(2, img.id, img_ui->label.buf);
    }
    return retval;
}

_SOKOL_PRIVATE bool _sgimgui_draw_sampler_link(sgimgui_t* ctx, sg_sampler smp) {
    bool retval = false;
    if (smp.id != SG_INVALID_ID) {
        const sgimgui_sampler_t* smp_ui = &ctx->sampler_window.slots[_sgimgui_slot_index(smp.id)];
        retval = _sgimgui_draw_resid_link(2, smp.id, smp_ui->label.buf);
    }
    return retval;
}

_SOKOL_PRIVATE bool _sgimgui_draw_shader_link(sgimgui_t* ctx, sg_shader shd) {
    bool retval = false;
    if (shd.id != SG_INVALID_ID) {
        const sgimgui_shader_t* shd_ui = &ctx->shader_window.slots[_sgimgui_slot_index(shd.id)];
        retval = _sgimgui_draw_resid_link(3, shd.id, shd_ui->label.buf);
    }
    return retval;
}

_SOKOL_PRIVATE void _sgimgui_show_buffer(sgimgui_t* ctx, sg_buffer buf) {
    ctx->buffer_window.open = true;
    ctx->buffer_window.sel_buf = buf;
}

_SOKOL_PRIVATE void _sgimgui_show_image(sgimgui_t* ctx, sg_image img) {
    ctx->image_window.open = true;
    ctx->image_window.sel_img = img;
}

_SOKOL_PRIVATE void _sgimgui_show_sampler(sgimgui_t* ctx, sg_sampler smp) {
    ctx->sampler_window.open = true;
    ctx->sampler_window.sel_smp = smp;
}

_SOKOL_PRIVATE void _sgimgui_show_shader(sgimgui_t* ctx, sg_shader shd) {
    ctx->shader_window.open = true;
    ctx->shader_window.sel_shd = shd;
}

_SOKOL_PRIVATE void _sgimgui_draw_buffer_list(sgimgui_t* ctx) {
    igBeginChild_Str("buffer_list", IMVEC2(_SGIMGUI_LIST_WIDTH,0), true, 0);
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
    igEndChild();
}

_SOKOL_PRIVATE void _sgimgui_draw_image_list(sgimgui_t* ctx) {
    igBeginChild_Str("image_list", IMVEC2(_SGIMGUI_LIST_WIDTH,0), true, 0);
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
    igEndChild();
}

_SOKOL_PRIVATE void _sgimgui_draw_sampler_list(sgimgui_t* ctx) {
    igBeginChild_Str("sampler_list", IMVEC2(_SGIMGUI_LIST_WIDTH,0), true, 0);
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
    igEndChild();
}

_SOKOL_PRIVATE void _sgimgui_draw_shader_list(sgimgui_t* ctx) {
    igBeginChild_Str("shader_list", IMVEC2(_SGIMGUI_LIST_WIDTH,0), true, 0);
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
    igEndChild();
}

_SOKOL_PRIVATE void _sgimgui_draw_pipeline_list(sgimgui_t* ctx) {
    igBeginChild_Str("pipeline_list", IMVEC2(_SGIMGUI_LIST_WIDTH,0), true, 0);
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
    igEndChild();
}

_SOKOL_PRIVATE void _sgimgui_draw_attachments_list(sgimgui_t* ctx) {
    igBeginChild_Str("pass_list", IMVEC2(_SGIMGUI_LIST_WIDTH,0), true, 0);
    for (int i = 1; i < ctx->attachments_window.num_slots; i++) {
        sg_attachments atts = ctx->attachments_window.slots[i].res_id;
        sg_resource_state state = sg_query_attachments_state(atts);
        if ((state != SG_RESOURCESTATE_INVALID) && (state != SG_RESOURCESTATE_INITIAL)) {
            bool selected = ctx->attachments_window.sel_atts.id == atts.id;
            if (_sgimgui_draw_resid_list_item(atts.id, ctx->attachments_window.slots[i].label.buf, selected)) {
                ctx->attachments_window.sel_atts.id = atts.id;
            }
        }
    }
    igEndChild();
}

_SOKOL_PRIVATE void _sgimgui_draw_capture_list(sgimgui_t* ctx) {
    igBeginChild_Str("capture_list", IMVEC2(_SGIMGUI_LIST_WIDTH,0), true, 0);
    const int num_items = _sgimgui_capture_num_read_items(ctx);
    uint64_t group_stack = 1;   /* bit set: group unfolded, cleared: folded */
    for (int i = 0; i < num_items; i++) {
        const sgimgui_capture_item_t* item = _sgimgui_capture_read_item_at(ctx, i);
        sgimgui_str_t item_string = _sgimgui_capture_item_string(ctx, i, item);
        igPushStyleColor_U32(ImGuiCol_Text, item->color);
        igPushID_Int(i);
        if (item->cmd == SGIMGUI_CMD_PUSH_DEBUG_GROUP) {
            if (group_stack & 1) {
                group_stack <<= 1;
                const char* group_name = item->args.push_debug_group.name.buf;
                if (igTreeNode_StrStr(group_name, "Group: %s", group_name)) {
                    group_stack |= 1;
                }
            } else {
                group_stack <<= 1;
            }
        } else if (item->cmd == SGIMGUI_CMD_POP_DEBUG_GROUP) {
            if (group_stack & 1) {
                igTreePop();
            }
            group_stack >>= 1;
        } else if (group_stack & 1) {
            if (igSelectable_Bool(item_string.buf, ctx->capture_window.sel_item == i, 0, IMVEC2(0,0))) {
                ctx->capture_window.sel_item = i;
            }
            if (igIsItemHovered(0)) {
                igSetTooltip("%s", item_string.buf);
            }
        }
        igPopID();
        igPopStyleColor(1);
    }
    igEndChild();
}

_SOKOL_PRIVATE void _sgimgui_draw_buffer_panel(sgimgui_t* ctx, sg_buffer buf) {
    if (buf.id != SG_INVALID_ID) {
        igBeginChild_Str("buffer", IMVEC2(0,0), false, 0);
        sg_buffer_info info = sg_query_buffer_info(buf);
        if (info.slot.state == SG_RESOURCESTATE_VALID) {
            const sgimgui_buffer_t* buf_ui = &ctx->buffer_window.slots[_sgimgui_slot_index(buf.id)];
            igText("Label: %s", buf_ui->label.buf[0] ? buf_ui->label.buf : "---");
            _sgimgui_draw_resource_slot(&info.slot);
            igSeparator();
            igText("Type:  %s", _sgimgui_buffertype_string(buf_ui->desc.type));
            igText("Usage: %s", _sgimgui_usage_string(buf_ui->desc.usage));
            igText("Size:  %d", buf_ui->desc.size);
            if (buf_ui->desc.usage != SG_USAGE_IMMUTABLE) {
                igSeparator();
                igText("Num Slots:     %d", info.num_slots);
                igText("Active Slot:   %d", info.active_slot);
                igText("Update Frame Index: %d", info.update_frame_index);
                igText("Append Frame Index: %d", info.append_frame_index);
                igText("Append Pos:         %d", info.append_pos);
                igText("Append Overflow:    %s", _sgimgui_bool_string(info.append_overflow));
            }
        } else {
            igText("Buffer 0x%08X not valid.", buf.id);
        }
        igEndChild();
    }
}

_SOKOL_PRIVATE bool _sgimgui_image_renderable(sg_image_type type, sg_pixel_format fmt, int sample_count) {
    return (type == SG_IMAGETYPE_2D)
        && sg_query_pixelformat(fmt).sample
        && sample_count == 1;
}

_SOKOL_PRIVATE void _sgimgui_draw_embedded_image(sgimgui_t* ctx, sg_image img, float* scale) {
    if (sg_query_image_state(img) == SG_RESOURCESTATE_VALID) {
        sgimgui_image_t* img_ui = &ctx->image_window.slots[_sgimgui_slot_index(img.id)];
        if (_sgimgui_image_renderable(img_ui->desc.type, img_ui->desc.pixel_format, img_ui->desc.sample_count)) {
            igPushID_Int((int)img.id);
            igSliderFloat("Scale", scale, 0.125f, 8.0f, "%.3f", ImGuiSliderFlags_Logarithmic);
            float w = (float)img_ui->desc.width * (*scale);
            float h = (float)img_ui->desc.height * (*scale);
            igImage(simgui_imtextureid(img_ui->simgui_img), IMVEC2(w, h), IMVEC2(0,0), IMVEC2(1,1), IMVEC4(1,1,1,1), IMVEC4(0,0,0,0));
            igPopID();
        } else {
            igText("Image not renderable.");
        }
    }
}

_SOKOL_PRIVATE void _sgimgui_draw_image_panel(sgimgui_t* ctx, sg_image img) {
    if (img.id != SG_INVALID_ID) {
        igBeginChild_Str("image", IMVEC2(0,0), false, 0);
        sg_image_info info = sg_query_image_info(img);
        if (info.slot.state == SG_RESOURCESTATE_VALID) {
            sgimgui_image_t* img_ui = &ctx->image_window.slots[_sgimgui_slot_index(img.id)];
            const sg_image_desc* desc = &img_ui->desc;
            igText("Label: %s", img_ui->label.buf[0] ? img_ui->label.buf : "---");
            _sgimgui_draw_resource_slot(&info.slot);
            igSeparator();
            _sgimgui_draw_embedded_image(ctx, img, &img_ui->ui_scale);
            igSeparator();
            igText("Type:           %s", _sgimgui_imagetype_string(desc->type));
            igText("Usage:          %s", _sgimgui_usage_string(desc->usage));
            igText("Render Target:  %s", _sgimgui_bool_string(desc->render_target));
            igText("Width:          %d", desc->width);
            igText("Height:         %d", desc->height);
            igText("Num Slices:     %d", desc->num_slices);
            igText("Num Mipmaps:    %d", desc->num_mipmaps);
            igText("Pixel Format:   %s", _sgimgui_pixelformat_string(desc->pixel_format));
            igText("Sample Count:   %d", desc->sample_count);
            if (desc->usage != SG_USAGE_IMMUTABLE) {
                igSeparator();
                igText("Num Slots:     %d", info.num_slots);
                igText("Active Slot:   %d", info.active_slot);
                igText("Update Frame Index: %d", info.upd_frame_index);
            }
        } else {
            igText("Image 0x%08X not valid.", img.id);
        }
        igEndChild();
    }
}

_SOKOL_PRIVATE void _sgimgui_draw_sampler_panel(sgimgui_t* ctx, sg_sampler smp) {
    if (smp.id != SG_INVALID_ID) {
        igBeginChild_Str("sampler", IMVEC2(0,0), false, 0);
        sg_sampler_info info = sg_query_sampler_info(smp);
        if (info.slot.state == SG_RESOURCESTATE_VALID) {
            sgimgui_sampler_t* smp_ui = &ctx->sampler_window.slots[_sgimgui_slot_index(smp.id)];
            const sg_sampler_desc* desc = &smp_ui->desc;
            igText("Label: %s", smp_ui->label.buf[0] ? smp_ui->label.buf : "---");
            _sgimgui_draw_resource_slot(&info.slot);
            igSeparator();
            igText("Min Filter:     %s", _sgimgui_filter_string(desc->min_filter));
            igText("Mag Filter:     %s", _sgimgui_filter_string(desc->mag_filter));
            igText("Mipmap Filter:  %s", _sgimgui_filter_string(desc->mipmap_filter));
            igText("Wrap U:         %s", _sgimgui_wrap_string(desc->wrap_u));
            igText("Wrap V:         %s", _sgimgui_wrap_string(desc->wrap_v));
            igText("Wrap W:         %s", _sgimgui_wrap_string(desc->wrap_w));
            igText("Min LOD:        %.3f", desc->min_lod);
            igText("Max LOD:        %.3f", desc->max_lod);
            igText("Border Color:   %s", _sgimgui_bordercolor_string(desc->border_color));
            igText("Compare:        %s", _sgimgui_comparefunc_string(desc->compare));
            igText("Max Anisotropy: %d", desc->max_anisotropy);
        } else {
            igText("Sampler 0x%08X not valid.", smp.id);
        }
        igEndChild();
    }
}

_SOKOL_PRIVATE void _sgimgui_draw_shader_stage(const sg_shader_stage_desc* stage) {
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
        if (stage->images[i].used) {
            num_valid_images++;
        } else {
            break;
        }
    }
    int num_valid_samplers = 0;
    for (int i = 0; i < SG_MAX_SHADERSTAGE_SAMPLERS; i++) {
        if (stage->samplers[i].used) {
            num_valid_samplers++;
        } else {
            break;
        }
    }
    int num_valid_image_sampler_pairs = 0;
    for (int i = 0; i < SG_MAX_SHADERSTAGE_IMAGESAMPLERPAIRS; i++) {
        if (stage->image_sampler_pairs[i].used) {
            num_valid_image_sampler_pairs++;
        } else {
            break;
        }
    }
    int num_valid_storage_buffers = 0;
    for (int i = 0; i < SG_MAX_SHADERSTAGE_STORAGEBUFFERS; i++) {
        if (stage->storage_buffers[i].used) {
            num_valid_storage_buffers++;
        } else {
            break;
        }
    }

    if (num_valid_ubs > 0) {
        if (igTreeNode_Str("Uniform Blocks")) {
            for (int i = 0; i < num_valid_ubs; i++) {
                const sg_shader_uniform_block_desc* ub = &stage->uniform_blocks[i];
                igText("#%d: (size: %d layout: %s)\n", i, ub->size, _sgimgui_uniformlayout_string(ub->layout));
                for (int j = 0; j < SG_MAX_UB_MEMBERS; j++) {
                    const sg_shader_uniform_desc* u = &ub->uniforms[j];
                    if (SG_UNIFORMTYPE_INVALID != u->type) {
                        if (u->array_count <= 1) {
                            igText("  %s %s", _sgimgui_uniformtype_string(u->type), u->name ? u->name : "");
                        } else {
                            igText("  %s[%d] %s", _sgimgui_uniformtype_string(u->type), u->array_count, u->name ? u->name : "");
                        }
                    }
                }
            }
            igTreePop();
        }
    }
    if (num_valid_images > 0) {
        if (igTreeNode_Str("Images")) {
            for (int i = 0; i < num_valid_images; i++) {
                const sg_shader_image_desc* sid = &stage->images[i];
                igText("slot: %d\n  multisampled: %s\n  image_type: %s\n  sample_type: %s",
                    i,
                    sid->multisampled ? "true" : "false",
                    _sgimgui_imagetype_string(sid->image_type),
                    _sgimgui_imagesampletype_string(sid->sample_type));
            }
            igTreePop();
        }
    }
    if (num_valid_samplers > 0) {
        if (igTreeNode_Str("Samplers")) {
            for (int i = 0; i < num_valid_samplers; i++) {
                const sg_shader_sampler_desc* ssd = &stage->samplers[i];
                igText("slot: %d\n  sampler_type: %s", i, _sgimgui_samplertype_string(ssd->sampler_type));
            }
            igTreePop();
        }
    }
    if (num_valid_image_sampler_pairs > 0) {
        if (igTreeNode_Str("Image Sampler Pairs")) {
            for (int i = 0; i < num_valid_image_sampler_pairs; i++) {
                const sg_shader_image_sampler_pair_desc* sispd = &stage->image_sampler_pairs[i];
                igText("slot: %d\n  image_slot: %d\n  sampler_slot: %d\n  glsl_name: %s\n",
                    i,
                    sispd->image_slot,
                    sispd->sampler_slot,
                    sispd->glsl_name ? sispd->glsl_name : "---");
            }
            igTreePop();
        }
    }
    if (num_valid_storage_buffers > 0) {
        if (igTreeNode_Str("Storage Buffers")) {
            for (int i = 0; i < num_valid_storage_buffers; i++) {
                const sg_shader_storage_buffer_desc* sbuf_desc = &stage->storage_buffers[i];
                igText("slot: %d\n  readonly: %s\n", i, sbuf_desc->readonly ? "true" : "false");
            }
            igTreePop();
        }
    }
    if (stage->entry) {
        igText("Entry: %s", stage->entry);
    }
    if (stage->d3d11_target) {
        igText("D3D11 Target: %s", stage->d3d11_target);
    }
    if (stage->source) {
        if (igTreeNode_Str("Source")) {
            igText("%s", stage->source);
            igTreePop();
        }
    } else if (stage->bytecode.ptr) {
        if (igTreeNode_Str("Byte Code")) {
            igText("Byte-code display currently not supported.");
            igTreePop();
        }
    }
}

_SOKOL_PRIVATE void _sgimgui_draw_shader_panel(sgimgui_t* ctx, sg_shader shd) {
    if (shd.id != SG_INVALID_ID) {
        igBeginChild_Str("shader", IMVEC2(0,0), false, ImGuiWindowFlags_HorizontalScrollbar);
        sg_shader_info info = sg_query_shader_info(shd);
        if (info.slot.state == SG_RESOURCESTATE_VALID) {
            const sgimgui_shader_t* shd_ui = &ctx->shader_window.slots[_sgimgui_slot_index(shd.id)];
            igText("Label: %s", shd_ui->label.buf[0] ? shd_ui->label.buf : "---");
            _sgimgui_draw_resource_slot(&info.slot);
            igSeparator();
            if (igTreeNode_Str("Attrs")) {
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
            if (igTreeNode_Str("Vertex Shader Stage")) {
                _sgimgui_draw_shader_stage(&shd_ui->desc.vs);
                igTreePop();
            }
            if (igTreeNode_Str("Fragment Shader Stage")) {
                _sgimgui_draw_shader_stage(&shd_ui->desc.fs);
                igTreePop();
            }
        } else {
            igText("Shader 0x%08X not valid!", shd.id);
        }
        igEndChild();
    }
}

_SOKOL_PRIVATE void _sgimgui_draw_vertex_layout_state(const sg_vertex_layout_state* layout) {
    if (igTreeNode_Str("Buffers")) {
        for (int i = 0; i < SG_MAX_VERTEX_BUFFERS; i++) {
            const sg_vertex_buffer_layout_state* l_state = &layout->buffers[i];
            if (l_state->stride > 0) {
                igText("#%d:", i);
                igText("  Stride:    %d", l_state->stride);
                igText("  Step Func: %s", _sgimgui_vertexstep_string(l_state->step_func));
                igText("  Step Rate: %d", l_state->step_rate);
            }
        }
        igTreePop();
    }
    if (igTreeNode_Str("Attrs")) {
        for (int i = 0; i < SG_MAX_VERTEX_ATTRIBUTES; i++) {
            const sg_vertex_attr_state* a_state = &layout->attrs[i];
            if (a_state->format != SG_VERTEXFORMAT_INVALID) {
                igText("#%d:", i);
                igText("  Format:       %s", _sgimgui_vertexformat_string(a_state->format));
                igText("  Offset:       %d", a_state->offset);
                igText("  Buffer Index: %d", a_state->buffer_index);
            }
        }
        igTreePop();
    }
}

_SOKOL_PRIVATE void _sgimgui_draw_stencil_face_state(const sg_stencil_face_state* sfs) {
    igText("Fail Op:       %s", _sgimgui_stencilop_string(sfs->fail_op));
    igText("Depth Fail Op: %s", _sgimgui_stencilop_string(sfs->depth_fail_op));
    igText("Pass Op:       %s", _sgimgui_stencilop_string(sfs->pass_op));
    igText("Compare:       %s", _sgimgui_comparefunc_string(sfs->compare));
}

_SOKOL_PRIVATE void _sgimgui_draw_stencil_state(const sg_stencil_state* ss) {
    igText("Enabled:    %s", _sgimgui_bool_string(ss->enabled));
    igText("Read Mask:  0x%02X", ss->read_mask);
    igText("Write Mask: 0x%02X", ss->write_mask);
    igText("Ref:        0x%02X", ss->ref);
    if (igTreeNode_Str("Front")) {
        _sgimgui_draw_stencil_face_state(&ss->front);
        igTreePop();
    }
    if (igTreeNode_Str("Back")) {
        _sgimgui_draw_stencil_face_state(&ss->back);
        igTreePop();
    }
}

_SOKOL_PRIVATE void _sgimgui_draw_depth_state(const sg_depth_state* ds) {
    igText("Pixel Format:  %s", _sgimgui_pixelformat_string(ds->pixel_format));
    igText("Compare:       %s", _sgimgui_comparefunc_string(ds->compare));
    igText("Write Enabled: %s", _sgimgui_bool_string(ds->write_enabled));
    igText("Bias:          %f", ds->bias);
    igText("Bias Slope:    %f", ds->bias_slope_scale);
    igText("Bias Clamp:    %f", ds->bias_clamp);
}

_SOKOL_PRIVATE void _sgimgui_draw_blend_state(const sg_blend_state* bs) {
    igText("Blend Enabled:    %s", _sgimgui_bool_string(bs->enabled));
    igText("Src Factor RGB:   %s", _sgimgui_blendfactor_string(bs->src_factor_rgb));
    igText("Dst Factor RGB:   %s", _sgimgui_blendfactor_string(bs->dst_factor_rgb));
    igText("Op RGB:           %s", _sgimgui_blendop_string(bs->op_rgb));
    igText("Src Factor Alpha: %s", _sgimgui_blendfactor_string(bs->src_factor_alpha));
    igText("Dst Factor Alpha: %s", _sgimgui_blendfactor_string(bs->dst_factor_alpha));
    igText("Op Alpha:         %s", _sgimgui_blendop_string(bs->op_alpha));
}

_SOKOL_PRIVATE void _sgimgui_draw_color_target_state(const sg_color_target_state* cs) {
    igText("Pixel Format:     %s", _sgimgui_pixelformat_string(cs->pixel_format));
    igText("Write Mask:       %s", _sgimgui_colormask_string(cs->write_mask));
    if (igTreeNode_Str("Blend State:")) {
        _sgimgui_draw_blend_state(&cs->blend);
        igTreePop();
    }
}

_SOKOL_PRIVATE void _sgimgui_draw_pipeline_panel(sgimgui_t* ctx, sg_pipeline pip) {
    if (pip.id != SG_INVALID_ID) {
        igBeginChild_Str("pipeline", IMVEC2(0,0), false, 0);
        sg_pipeline_info info = sg_query_pipeline_info(pip);
        if (info.slot.state == SG_RESOURCESTATE_VALID) {
            const sgimgui_pipeline_t* pip_ui = &ctx->pipeline_window.slots[_sgimgui_slot_index(pip.id)];
            igText("Label: %s", pip_ui->label.buf[0] ? pip_ui->label.buf : "---");
            _sgimgui_draw_resource_slot(&info.slot);
            igSeparator();
            igText("Shader:    "); igSameLine(0,-1);
            if (_sgimgui_draw_shader_link(ctx, pip_ui->desc.shader)) {
                _sgimgui_show_shader(ctx, pip_ui->desc.shader);
            }
            if (igTreeNode_Str("Vertex Layout State")) {
                _sgimgui_draw_vertex_layout_state(&pip_ui->desc.layout);
                igTreePop();
            }
            if (igTreeNode_Str("Depth State")) {
                _sgimgui_draw_depth_state(&pip_ui->desc.depth);
                igTreePop();
            }
            if (igTreeNode_Str("Stencil State")) {
                _sgimgui_draw_stencil_state(&pip_ui->desc.stencil);
                igTreePop();
            }
            igText("Color Count: %d", pip_ui->desc.color_count);
            for (int i = 0; i < pip_ui->desc.color_count; i++) {
                sgimgui_str_t str;
                _sgimgui_snprintf(&str, "Color Target %d", i);
                if (igTreeNode_Str(str.buf)) {
                    _sgimgui_draw_color_target_state(&pip_ui->desc.colors[i]);
                    igTreePop();
                }
            }
            igText("Prim Type:      %s", _sgimgui_primitivetype_string(pip_ui->desc.primitive_type));
            igText("Index Type:     %s", _sgimgui_indextype_string(pip_ui->desc.index_type));
            igText("Cull Mode:      %s", _sgimgui_cullmode_string(pip_ui->desc.cull_mode));
            igText("Face Winding:   %s", _sgimgui_facewinding_string(pip_ui->desc.face_winding));
            igText("Sample Count:   %d", pip_ui->desc.sample_count);
            sgimgui_str_t blend_color_str;
            igText("Blend Color:    %.3f %.3f %.3f %.3f", _sgimgui_color_string(&blend_color_str, pip_ui->desc.blend_color));
            igText("Alpha To Coverage: %s", _sgimgui_bool_string(pip_ui->desc.alpha_to_coverage_enabled));
        } else {
            igText("Pipeline 0x%08X not valid.", pip.id);
        }
        igEndChild();
    }
}

_SOKOL_PRIVATE void _sgimgui_draw_attachment(sgimgui_t* ctx, const sg_attachment_desc* att, float* img_scale) {
    igText("  Image: "); igSameLine(0,-1);
    if (_sgimgui_draw_image_link(ctx, att->image)) {
        _sgimgui_show_image(ctx, att->image);
    }
    igText("  Mip Level: %d", att->mip_level);
    igText("  Slice: %d", att->slice);
    _sgimgui_draw_embedded_image(ctx, att->image, img_scale);
}

_SOKOL_PRIVATE void _sgimgui_draw_attachments_panel(sgimgui_t* ctx, sg_attachments atts) {
    if (atts.id != SG_INVALID_ID) {
        igBeginChild_Str("attachments", IMVEC2(0,0), false, 0);
        sg_attachments_info info = sg_query_attachments_info(atts);
        if (info.slot.state == SG_RESOURCESTATE_VALID) {
            sgimgui_attachments_t* atts_ui = &ctx->attachments_window.slots[_sgimgui_slot_index(atts.id)];
            igText("Label: %s", atts_ui->label.buf[0] ? atts_ui->label.buf : "---");
            _sgimgui_draw_resource_slot(&info.slot);
            for (int i = 0; i < SG_MAX_COLOR_ATTACHMENTS; i++) {
                if (atts_ui->desc.colors[i].image.id == SG_INVALID_ID) {
                    break;
                }
                igSeparator();
                igText("Color Image #%d:", i);
                _sgimgui_draw_attachment(ctx, &atts_ui->desc.colors[i], &atts_ui->color_image_scale[i]);
            }
            for (int i = 0; i < SG_MAX_COLOR_ATTACHMENTS; i++) {
                if (atts_ui->desc.resolves[i].image.id == SG_INVALID_ID) {
                    break;
                }
                igSeparator();
                igText("Resolve Image #%d:", i);
                _sgimgui_draw_attachment(ctx, &atts_ui->desc.resolves[i], &atts_ui->resolve_image_scale[i]);
            }
            if (atts_ui->desc.depth_stencil.image.id != SG_INVALID_ID) {
                igSeparator();
                igText("Depth-Stencil Image:");
                _sgimgui_draw_attachment(ctx, &atts_ui->desc.depth_stencil, &atts_ui->ds_image_scale);
            }
        } else {
            igText("Attachments 0x%08X not valid.", atts.id);
        }
        igEndChild();
    }
}

_SOKOL_PRIVATE void _sgimgui_draw_bindings_panel(sgimgui_t* ctx, const sg_bindings* bnd) {
    for (int i = 0; i < SG_MAX_VERTEX_BUFFERS; i++) {
        sg_buffer buf = bnd->vertex_buffers[i];
        if (buf.id != SG_INVALID_ID) {
            igSeparator();
            igText("Vertex Buffer Slot #%d:", i);
            igText("  Buffer: "); igSameLine(0,-1);
            if (_sgimgui_draw_buffer_link(ctx, buf)) {
                _sgimgui_show_buffer(ctx, buf);
            }
            igText("  Offset: %d", bnd->vertex_buffer_offsets[i]);
        } else {
            break;
        }
    }
    if (bnd->index_buffer.id != SG_INVALID_ID) {
        sg_buffer buf = bnd->index_buffer;
        if (buf.id != SG_INVALID_ID) {
            igSeparator();
            igText("Index Buffer Slot:");
            igText("  Buffer: "); igSameLine(0,-1);
            if (_sgimgui_draw_buffer_link(ctx, buf)) {
                _sgimgui_show_buffer(ctx, buf);
            }
            igText("  Offset: %d", bnd->index_buffer_offset);
        }
    }
    for (int i = 0; i < SG_MAX_SHADERSTAGE_IMAGES; i++) {
        sg_image img = bnd->vs.images[i];
        if (img.id != SG_INVALID_ID) {
            igSeparator();
            igText("Vertex Stage Image Slot #%d:", i);
            igText("  Image: "); igSameLine(0,-1);
            if (_sgimgui_draw_image_link(ctx, img)) {
                _sgimgui_show_image(ctx, img);
            }
        } else {
            break;
        }
    }
    for (int i = 0; i < SG_MAX_SHADERSTAGE_SAMPLERS; i++) {
        sg_sampler smp = bnd->vs.samplers[i];
        if (smp.id != SG_INVALID_ID) {
            igSeparator();
            igText("Vertex Stage Sampler Slot #%d:", i);
            igText("  Sampler: "); igSameLine(0,-1);
            if (_sgimgui_draw_sampler_link(ctx, smp)) {
                _sgimgui_show_sampler(ctx, smp);
            }
        } else {
            break;
        }
    }
    for (int i = 0; i < SG_MAX_SHADERSTAGE_STORAGEBUFFERS; i++) {
        sg_buffer buf = bnd->vs.storage_buffers[i];
        if (buf.id != SG_INVALID_ID) {
            igSeparator();
            igText("Vertex Stage Storage Buffer Slot #%d:", i);
            igText("  Buffer: "); igSameLine(0,-1);
            if (_sgimgui_draw_buffer_link(ctx, buf)) {
                _sgimgui_show_buffer(ctx, buf);
            }
        }
    }
    for (int i = 0; i < SG_MAX_SHADERSTAGE_IMAGES; i++) {
        sg_image img = bnd->fs.images[i];
        if (img.id != SG_INVALID_ID) {
            igSeparator();
            igText("Fragment Stage Image Slot #%d:", i);
            igText("  Image: "); igSameLine(0,-1);
            if (_sgimgui_draw_image_link(ctx, img)) {
                _sgimgui_show_image(ctx, img);
            }
        }
    }
    for (int i = 0; i < SG_MAX_SHADERSTAGE_SAMPLERS; i++) {
        sg_sampler smp = bnd->fs.samplers[i];
        if (smp.id != SG_INVALID_ID) {
            igSeparator();
            igText("Fragment Stage Sampler Slot #%d:", i);
            igText("  Sampler: "); igSameLine(0,-1);
            if (_sgimgui_draw_sampler_link(ctx, smp)) {
                _sgimgui_show_sampler(ctx, smp);
            }
        }
    }
    for (int i = 0; i < SG_MAX_SHADERSTAGE_STORAGEBUFFERS; i++) {
        sg_buffer buf = bnd->fs.storage_buffers[i];
        if (buf.id != SG_INVALID_ID) {
            igSeparator();
            igText("Fragment Stage Storage Buffer Slot #%d:", i);
            igText("  Buffer: "); igSameLine(0,-1);
            if (_sgimgui_draw_buffer_link(ctx, buf)) {
                _sgimgui_show_buffer(ctx, buf);
            }
        }
    }
}

_SOKOL_PRIVATE void _sgimgui_draw_uniforms_panel(sgimgui_t* ctx, const sgimgui_args_apply_uniforms_t* args) {
    SOKOL_ASSERT(args->ub_index < SG_MAX_VERTEX_BUFFERS);

    /* check if all the required information for drawing the structured uniform block content
        is available, otherwise just render a generic hexdump
    */
   if (sg_query_pipeline_state(args->pipeline) != SG_RESOURCESTATE_VALID) {
        igText("Pipeline object not valid!");
        return;
   }
    sgimgui_pipeline_t* pip_ui = &ctx->pipeline_window.slots[_sgimgui_slot_index(args->pipeline.id)];
    if (sg_query_shader_state(pip_ui->desc.shader) != SG_RESOURCESTATE_VALID) {
        igText("Shader object not valid!");
        return;
    }
    sgimgui_shader_t* shd_ui = &ctx->shader_window.slots[_sgimgui_slot_index(pip_ui->desc.shader.id)];
    SOKOL_ASSERT(shd_ui->res_id.id == pip_ui->desc.shader.id);
    const sg_shader_uniform_block_desc* ub_desc = (args->stage == SG_SHADERSTAGE_VS) ?
        &shd_ui->desc.vs.uniform_blocks[args->ub_index] :
        &shd_ui->desc.fs.uniform_blocks[args->ub_index];
    SOKOL_ASSERT(args->data_size <= ub_desc->size);
    bool draw_dump = false;
    if (ub_desc->uniforms[0].type == SG_UNIFORMTYPE_INVALID) {
        draw_dump = true;
    }

    sgimgui_capture_bucket_t* bucket = _sgimgui_capture_get_read_bucket(ctx);
    SOKOL_ASSERT((args->ubuf_pos + args->data_size) <= bucket->ubuf_size);
    const float* uptrf = (const float*) (bucket->ubuf + args->ubuf_pos);
    const int32_t* uptri32 = (const int32_t*) uptrf;
    if (!draw_dump) {
        uint32_t u_off = 0;
        for (int i = 0; i < SG_MAX_UB_MEMBERS; i++) {
            const sg_shader_uniform_desc* ud = &ub_desc->uniforms[i];
            if (ud->type == SG_UNIFORMTYPE_INVALID) {
                break;
            }
            int num_items = (ud->array_count > 1) ? ud->array_count : 1;
            if (num_items > 1) {
                igText("%d: %s %s[%d] =", i, _sgimgui_uniformtype_string(ud->type), ud->name?ud->name:"", ud->array_count);
            } else {
                igText("%d: %s %s =", i, _sgimgui_uniformtype_string(ud->type), ud->name?ud->name:"");
            }
            for (int item_index = 0; item_index < num_items; item_index++) {
                const uint32_t u_size = _sgimgui_std140_uniform_size(ud->type, ud->array_count) / 4;
                const uint32_t u_align = _sgimgui_std140_uniform_alignment(ud->type, ud->array_count) / 4;
                u_off = _sgimgui_align_u32(u_off, u_align);
                switch (ud->type) {
                    case SG_UNIFORMTYPE_FLOAT:
                        igText("    %.3f", uptrf[u_off]);
                        break;
                    case SG_UNIFORMTYPE_INT:
                        igText("    %d", uptri32[u_off]);
                        break;
                    case SG_UNIFORMTYPE_FLOAT2:
                        igText("    %.3f, %.3f", uptrf[u_off], uptrf[u_off+1]);
                        break;
                    case SG_UNIFORMTYPE_INT2:
                        igText("    %d, %d", uptri32[u_off], uptri32[u_off+1]);
                        break;
                    case SG_UNIFORMTYPE_FLOAT3:
                        igText("    %.3f, %.3f, %.3f", uptrf[u_off], uptrf[u_off+1], uptrf[u_off+2]);
                        break;
                    case SG_UNIFORMTYPE_INT3:
                        igText("    %d, %d, %d", uptri32[u_off], uptri32[u_off+1], uptri32[u_off+2]);
                        break;
                    case SG_UNIFORMTYPE_FLOAT4:
                        igText("    %.3f, %.3f, %.3f, %.3f", uptrf[u_off], uptrf[u_off+1], uptrf[u_off+2], uptrf[u_off+3]);
                        break;
                    case SG_UNIFORMTYPE_INT4:
                        igText("    %d, %d, %d, %d", uptri32[u_off], uptri32[u_off+1], uptri32[u_off+2], uptri32[u_off+3]);
                        break;
                    case SG_UNIFORMTYPE_MAT4:
                        igText("    %.3f, %.3f, %.3f, %.3f\n"
                               "    %.3f, %.3f, %.3f, %.3f\n"
                               "    %.3f, %.3f, %.3f, %.3f\n"
                               "    %.3f, %.3f, %.3f, %.3f",
                            uptrf[u_off+0],  uptrf[u_off+1],  uptrf[u_off+2],  uptrf[u_off+3],
                            uptrf[u_off+4],  uptrf[u_off+5],  uptrf[u_off+6],  uptrf[u_off+7],
                            uptrf[u_off+8],  uptrf[u_off+9],  uptrf[u_off+10], uptrf[u_off+11],
                            uptrf[u_off+12], uptrf[u_off+13], uptrf[u_off+14], uptrf[u_off+15]);
                        break;
                    default:
                        igText("???");
                        break;
                }
                u_off += u_size;
            }
        }
    } else {
        // FIXME: float vs int
        const size_t num_floats = ub_desc->size / sizeof(float);
        for (uint32_t i = 0; i < num_floats; i++) {
            igText("%.3f, ", uptrf[i]);
            if (((i + 1) % 4) != 0) {
                igSameLine(0,-1);
            }
        }
    }
}

_SOKOL_PRIVATE void _sgimgui_draw_passaction_panel(sgimgui_t* ctx, sg_attachments atts, const sg_pass_action* action) {
    /* determine number of valid color attachments */
    int num_color_atts = 0;
    if (SG_INVALID_ID == atts.id) {
        /* a swapchain pass: one color attachment */
        num_color_atts = 1;
    } else {
        const sgimgui_attachments_t* atts_ui = &ctx->attachments_window.slots[_sgimgui_slot_index(atts.id)];
        for (int i = 0; i < SG_MAX_COLOR_ATTACHMENTS; i++) {
            if (atts_ui->desc.colors[i].image.id != SG_INVALID_ID) {
                num_color_atts++;
            }
        }
    }

    igText("Pass Action: ");
    for (int i = 0; i < num_color_atts; i++) {
        const sg_color_attachment_action* c_att = &action->colors[i];
        igText("  Color Attachment %d:", i);
        sgimgui_str_t color_str;
        switch (c_att->load_action) {
            case SG_LOADACTION_LOAD: igText("    SG_LOADACTION_LOAD"); break;
            case SG_LOADACTION_DONTCARE: igText("    SG_LOADACTION_DONTCARE"); break;
            case SG_LOADACTION_CLEAR:
                igText("    SG_LOADACTION_CLEAR: %s", _sgimgui_color_string(&color_str, c_att->clear_value));
                break;
            default: igText("    ???"); break;
        }
        switch (c_att->store_action) {
            case SG_STOREACTION_STORE: igText("    SG_STOREACTION_STORE"); break;
            case SG_STOREACTION_DONTCARE: igText("    SG_STOREACTION_DONTCARE"); break;
            default: igText("    ???"); break;
        }
    }
    const sg_depth_attachment_action* d_att = &action->depth;
    igText("  Depth Attachment:");
    switch (d_att->load_action) {
        case SG_LOADACTION_LOAD: igText("    SG_LOADACTION_LOAD"); break;
        case SG_LOADACTION_DONTCARE: igText("    SG_LOADACTION_DONTCARE"); break;
        case SG_LOADACTION_CLEAR: igText("    SG_LOADACTION_CLEAR: %.3f", d_att->clear_value); break;
        default: igText("    ???"); break;
    }
    switch (d_att->store_action) {
        case SG_STOREACTION_STORE: igText("    SG_STOREACTION_STORE"); break;
        case SG_STOREACTION_DONTCARE: igText("    SG_STOREACTION_DONTCARE"); break;
        default: igText("    ???"); break;
    }
    const sg_stencil_attachment_action* s_att = &action->stencil;
    igText("  Stencil Attachment");
    switch (s_att->load_action) {
        case SG_LOADACTION_LOAD: igText("    SG_LOADACTION_LOAD"); break;
        case SG_LOADACTION_DONTCARE: igText("    SG_LOADACTION_DONTCARE"); break;
        case SG_LOADACTION_CLEAR: igText("    SG_LOADACTION_CLEAR: 0x%02X", s_att->clear_value); break;
        default: igText("    ???"); break;
    }
    switch (d_att->store_action) {
        case SG_STOREACTION_STORE: igText("    SG_STOREACTION_STORE"); break;
        case SG_STOREACTION_DONTCARE: igText("    SG_STOREACTION_DONTCARE"); break;
        default: igText("    ???"); break;
    }
}

_SOKOL_PRIVATE void _sgimgui_draw_swapchain_panel(sg_swapchain* swapchain) {
    igText("Swapchain");
    igText("  Width: %d", swapchain->width);
    igText("  Height: %d", swapchain->height);
    igText("  Sample Count: %d", swapchain->sample_count);
    igText("  Color Format: %s", _sgimgui_pixelformat_string(swapchain->color_format));
    igText("  Depth Format: %s", _sgimgui_pixelformat_string(swapchain->depth_format));
    igSeparator();
    switch (sg_query_backend()) {
        case SG_BACKEND_D3D11:
            igText("D3D11 Objects:");
            igText("  Render View: %p", swapchain->d3d11.render_view);
            igText("  Resolve View: %p", swapchain->d3d11.resolve_view);
            igText("  Depth Stencil View: %p", swapchain->d3d11.depth_stencil_view);
            break;
        case SG_BACKEND_WGPU:
            igText("WGPU Objects:");
            igText("  Render View: %p", swapchain->wgpu.render_view);
            igText("  Resolve View: %p", swapchain->wgpu.resolve_view);
            igText("  Depth Stencil View: %p", swapchain->wgpu.depth_stencil_view);
            break;
        case SG_BACKEND_METAL_MACOS:
        case SG_BACKEND_METAL_IOS:
        case SG_BACKEND_METAL_SIMULATOR:
            igText("Metal Objects:");
            igText("  Current Drawable: %p", swapchain->metal.current_drawable);
            igText("  Depth Stencil Texture: %p", swapchain->metal.depth_stencil_texture);
            igText("  MSAA Color Texture: %p", swapchain->metal.msaa_color_texture);
            break;
        case SG_BACKEND_GLCORE:
        case SG_BACKEND_GLES3:
            igText("GL Objects:");
            igText("  Framebuffer: %d", swapchain->gl.framebuffer);
            break;
        default:
            igText("  UNKNOWN BACKEND!");
            break;
    }
}

_SOKOL_PRIVATE void _sgimgui_draw_capture_panel(sgimgui_t* ctx) {
    int sel_item_index = ctx->capture_window.sel_item;
    if (sel_item_index >= _sgimgui_capture_num_read_items(ctx)) {
        return;
    }
    sgimgui_capture_item_t* item = _sgimgui_capture_read_item_at(ctx, sel_item_index);
    igBeginChild_Str("capture_item", IMVEC2(0, 0), false, 0);
    igPushStyleColor_U32(ImGuiCol_Text, item->color);
    igText("%s", _sgimgui_capture_item_string(ctx, sel_item_index, item).buf);
    igPopStyleColor(1);
    igSeparator();
    switch (item->cmd) {
        case SGIMGUI_CMD_RESET_STATE_CACHE:
            break;
        case SGIMGUI_CMD_MAKE_BUFFER:
            _sgimgui_draw_buffer_panel(ctx, item->args.make_buffer.result);
            break;
        case SGIMGUI_CMD_MAKE_IMAGE:
            _sgimgui_draw_image_panel(ctx, item->args.make_image.result);
            break;
        case SGIMGUI_CMD_MAKE_SAMPLER:
            _sgimgui_draw_sampler_panel(ctx, item->args.make_sampler.result);
            break;
        case SGIMGUI_CMD_MAKE_SHADER:
            _sgimgui_draw_shader_panel(ctx, item->args.make_shader.result);
            break;
        case SGIMGUI_CMD_MAKE_PIPELINE:
            _sgimgui_draw_pipeline_panel(ctx, item->args.make_pipeline.result);
            break;
        case SGIMGUI_CMD_MAKE_ATTACHMENTS:
            _sgimgui_draw_attachments_panel(ctx, item->args.make_attachments.result);
            break;
        case SGIMGUI_CMD_DESTROY_BUFFER:
            _sgimgui_draw_buffer_panel(ctx, item->args.destroy_buffer.buffer);
            break;
        case SGIMGUI_CMD_DESTROY_IMAGE:
            _sgimgui_draw_image_panel(ctx, item->args.destroy_image.image);
            break;
        case SGIMGUI_CMD_DESTROY_SAMPLER:
            _sgimgui_draw_sampler_panel(ctx, item->args.destroy_sampler.sampler);
            break;
        case SGIMGUI_CMD_DESTROY_SHADER:
            _sgimgui_draw_shader_panel(ctx, item->args.destroy_shader.shader);
            break;
        case SGIMGUI_CMD_DESTROY_PIPELINE:
            _sgimgui_draw_pipeline_panel(ctx, item->args.destroy_pipeline.pipeline);
            break;
        case SGIMGUI_CMD_DESTROY_ATTACHMENTS:
            _sgimgui_draw_attachments_panel(ctx, item->args.destroy_attachments.attachments);
            break;
        case SGIMGUI_CMD_UPDATE_BUFFER:
            _sgimgui_draw_buffer_panel(ctx, item->args.update_buffer.buffer);
            break;
        case SGIMGUI_CMD_UPDATE_IMAGE:
            _sgimgui_draw_image_panel(ctx, item->args.update_image.image);
            break;
        case SGIMGUI_CMD_APPEND_BUFFER:
            _sgimgui_draw_buffer_panel(ctx, item->args.update_buffer.buffer);
            break;
        case SGIMGUI_CMD_BEGIN_PASS:
            _sgimgui_draw_passaction_panel(ctx, item->args.begin_pass.pass.attachments, &item->args.begin_pass.pass.action);
            igSeparator();
            if (item->args.begin_pass.pass.attachments.id != SG_INVALID_ID) {
                _sgimgui_draw_attachments_panel(ctx, item->args.begin_pass.pass.attachments);
            } else {
                _sgimgui_draw_swapchain_panel(&item->args.begin_pass.pass.swapchain);
            }
            break;
        case SGIMGUI_CMD_APPLY_VIEWPORT:
        case SGIMGUI_CMD_APPLY_SCISSOR_RECT:
            break;
        case SGIMGUI_CMD_APPLY_PIPELINE:
            _sgimgui_draw_pipeline_panel(ctx, item->args.apply_pipeline.pipeline);
            break;
        case SGIMGUI_CMD_APPLY_BINDINGS:
            _sgimgui_draw_bindings_panel(ctx, &item->args.apply_bindings.bindings);
            break;
        case SGIMGUI_CMD_APPLY_UNIFORMS:
            _sgimgui_draw_uniforms_panel(ctx, &item->args.apply_uniforms);
            break;
        case SGIMGUI_CMD_DRAW:
        case SGIMGUI_CMD_END_PASS:
        case SGIMGUI_CMD_COMMIT:
            break;
        case SGIMGUI_CMD_ALLOC_BUFFER:
            _sgimgui_draw_buffer_panel(ctx, item->args.alloc_buffer.result);
            break;
        case SGIMGUI_CMD_ALLOC_IMAGE:
            _sgimgui_draw_image_panel(ctx, item->args.alloc_image.result);
            break;
        case SGIMGUI_CMD_ALLOC_SAMPLER:
            _sgimgui_draw_sampler_panel(ctx, item->args.alloc_sampler.result);
            break;
        case SGIMGUI_CMD_ALLOC_SHADER:
            _sgimgui_draw_shader_panel(ctx, item->args.alloc_shader.result);
            break;
        case SGIMGUI_CMD_ALLOC_PIPELINE:
            _sgimgui_draw_pipeline_panel(ctx, item->args.alloc_pipeline.result);
            break;
        case SGIMGUI_CMD_ALLOC_ATTACHMENTS:
            _sgimgui_draw_attachments_panel(ctx, item->args.alloc_attachments.result);
            break;
        case SGIMGUI_CMD_INIT_BUFFER:
            _sgimgui_draw_buffer_panel(ctx, item->args.init_buffer.buffer);
            break;
        case SGIMGUI_CMD_INIT_IMAGE:
            _sgimgui_draw_image_panel(ctx, item->args.init_image.image);
            break;
        case SGIMGUI_CMD_INIT_SAMPLER:
            _sgimgui_draw_sampler_panel(ctx, item->args.init_sampler.sampler);
            break;
        case SGIMGUI_CMD_INIT_SHADER:
            _sgimgui_draw_shader_panel(ctx, item->args.init_shader.shader);
            break;
        case SGIMGUI_CMD_INIT_PIPELINE:
            _sgimgui_draw_pipeline_panel(ctx, item->args.init_pipeline.pipeline);
            break;
        case SGIMGUI_CMD_INIT_ATTACHMENTS:
            _sgimgui_draw_attachments_panel(ctx, item->args.init_attachments.attachments);
            break;
        case SGIMGUI_CMD_FAIL_BUFFER:
            _sgimgui_draw_buffer_panel(ctx, item->args.fail_buffer.buffer);
            break;
        case SGIMGUI_CMD_FAIL_IMAGE:
            _sgimgui_draw_image_panel(ctx, item->args.fail_image.image);
            break;
        case SGIMGUI_CMD_FAIL_SAMPLER:
            _sgimgui_draw_sampler_panel(ctx, item->args.fail_sampler.sampler);
            break;
        case SGIMGUI_CMD_FAIL_SHADER:
            _sgimgui_draw_shader_panel(ctx, item->args.fail_shader.shader);
            break;
        case SGIMGUI_CMD_FAIL_PIPELINE:
            _sgimgui_draw_pipeline_panel(ctx, item->args.fail_pipeline.pipeline);
            break;
        case SGIMGUI_CMD_FAIL_ATTACHMENTS:
            _sgimgui_draw_attachments_panel(ctx, item->args.fail_attachments.attachments);
            break;
        default:
            break;
    }
    igEndChild();
}

_SOKOL_PRIVATE void _sgimgui_draw_caps_panel(void) {
    igText("Backend: %s\n\n", _sgimgui_backend_string(sg_query_backend()));
    sg_features f = sg_query_features();
    igText("Features:");
    igText("    origin_top_left: %s", _sgimgui_bool_string(f.origin_top_left));
    igText("    image_clamp_to_border: %s", _sgimgui_bool_string(f.image_clamp_to_border));
    igText("    mrt_independent_blend_state: %s", _sgimgui_bool_string(f.mrt_independent_blend_state));
    igText("    mrt_independent_write_mask: %s", _sgimgui_bool_string(f.mrt_independent_write_mask));
    igText("    storage_buffer: %s", _sgimgui_bool_string(f.storage_buffer));
    sg_limits l = sg_query_limits();
    igText("\nLimits:\n");
    igText("    max_image_size_2d: %d", l.max_image_size_2d);
    igText("    max_image_size_cube: %d", l.max_image_size_cube);
    igText("    max_image_size_3d: %d", l.max_image_size_3d);
    igText("    max_image_size_array: %d", l.max_image_size_array);
    igText("    max_image_array_layers: %d", l.max_image_array_layers);
    igText("    max_vertex_attrs: %d", l.max_vertex_attrs);
    igText("    gl_max_vertex_uniform_components: %d", l.gl_max_vertex_uniform_components);
    igText("    gl_max_combined_texture_image_units: %d", l.gl_max_combined_texture_image_units);
    igText("\nUsable Pixelformats:");
    for (int i = (int)(SG_PIXELFORMAT_NONE+1); i < (int)_SG_PIXELFORMAT_NUM; i++) {
        sg_pixel_format fmt = (sg_pixel_format)i;
        sg_pixelformat_info info = sg_query_pixelformat(fmt);
        if (info.sample) {
            igText("  %s: %s%s%s%s%s%s",
                _sgimgui_pixelformat_string(fmt),
                info.sample ? "SAMPLE ":"",
                info.filter ? "FILTER ":"",
                info.blend ? "BLEND ":"",
                info.render ? "RENDER ":"",
                info.msaa ? "MSAA ":"",
                info.depth ? "DEPTH ":"");
        }
    }
}

_SOKOL_PRIVATE void _sgimgui_frame_add_stats_row(const char* key, uint32_t value) {
    igTableNextRow(0, 0.0f);
    igTableSetColumnIndex(0);
    igText(key);
    igTableSetColumnIndex(1);
    igText("%d", value);
}

#define _sgimgui_frame_stats(key) _sgimgui_frame_add_stats_row(#key, stats->key)

_SOKOL_PRIVATE void _sgimgui_draw_frame_stats_panel(sgimgui_t* ctx) {
    _SOKOL_UNUSED(ctx);
    igCheckbox("Ignore sokol_imgui.h", &ctx->frame_stats_window.disable_sokol_imgui_stats);
    const sg_frame_stats* stats = &ctx->frame_stats_window.stats;
    const ImGuiTableFlags flags =
        ImGuiTableFlags_Resizable |
        ImGuiTableFlags_ScrollY |
        ImGuiTableFlags_SizingFixedFit |
        ImGuiTableFlags_Borders;
    if (igBeginTable("##frame_stats_table", 2, flags, IMVEC2(0, 0), 0)) {
        igTableSetupScrollFreeze(0, 2);
        igTableSetupColumn("key", ImGuiTableColumnFlags_None, 0, 0);
        igTableSetupColumn("value", ImGuiTableColumnFlags_None, 0, 0);
        igTableHeadersRow();
        _sgimgui_frame_stats(frame_index);
        _sgimgui_frame_stats(num_passes);
        _sgimgui_frame_stats(num_apply_viewport);
        _sgimgui_frame_stats(num_apply_scissor_rect);
        _sgimgui_frame_stats(num_apply_pipeline);
        _sgimgui_frame_stats(num_apply_bindings);
        _sgimgui_frame_stats(num_apply_uniforms);
        _sgimgui_frame_stats(num_draw);
        _sgimgui_frame_stats(num_update_buffer);
        _sgimgui_frame_stats(num_append_buffer);
        _sgimgui_frame_stats(num_update_image);
        _sgimgui_frame_stats(size_apply_uniforms);
        _sgimgui_frame_stats(size_update_buffer);
        _sgimgui_frame_stats(size_append_buffer);
        _sgimgui_frame_stats(size_update_image);
        switch (sg_query_backend()) {
            case SG_BACKEND_GLCORE:
            case SG_BACKEND_GLES3:
                _sgimgui_frame_stats(gl.num_bind_buffer);
                _sgimgui_frame_stats(gl.num_active_texture);
                _sgimgui_frame_stats(gl.num_bind_texture);
                _sgimgui_frame_stats(gl.num_bind_sampler);
                _sgimgui_frame_stats(gl.num_use_program);
                _sgimgui_frame_stats(gl.num_render_state);
                _sgimgui_frame_stats(gl.num_vertex_attrib_pointer);
                _sgimgui_frame_stats(gl.num_vertex_attrib_divisor);
                _sgimgui_frame_stats(gl.num_enable_vertex_attrib_array);
                _sgimgui_frame_stats(gl.num_disable_vertex_attrib_array);
                _sgimgui_frame_stats(gl.num_uniform);
                break;
            case SG_BACKEND_WGPU:
                _sgimgui_frame_stats(wgpu.uniforms.num_set_bindgroup);
                _sgimgui_frame_stats(wgpu.uniforms.size_write_buffer);
                _sgimgui_frame_stats(wgpu.bindings.num_set_vertex_buffer);
                _sgimgui_frame_stats(wgpu.bindings.num_skip_redundant_vertex_buffer);
                _sgimgui_frame_stats(wgpu.bindings.num_set_index_buffer);
                _sgimgui_frame_stats(wgpu.bindings.num_skip_redundant_index_buffer);
                _sgimgui_frame_stats(wgpu.bindings.num_create_bindgroup);
                _sgimgui_frame_stats(wgpu.bindings.num_discard_bindgroup);
                _sgimgui_frame_stats(wgpu.bindings.num_set_bindgroup);
                _sgimgui_frame_stats(wgpu.bindings.num_skip_redundant_bindgroup);
                _sgimgui_frame_stats(wgpu.bindings.num_bindgroup_cache_hits);
                _sgimgui_frame_stats(wgpu.bindings.num_bindgroup_cache_misses);
                _sgimgui_frame_stats(wgpu.bindings.num_bindgroup_cache_collisions);
                _sgimgui_frame_stats(wgpu.bindings.num_bindgroup_cache_hash_vs_key_mismatch);
                break;
            case SG_BACKEND_METAL_MACOS:
            case SG_BACKEND_METAL_IOS:
            case SG_BACKEND_METAL_SIMULATOR:
                _sgimgui_frame_stats(metal.idpool.num_added);
                _sgimgui_frame_stats(metal.idpool.num_released);
                _sgimgui_frame_stats(metal.idpool.num_garbage_collected);
                _sgimgui_frame_stats(metal.pipeline.num_set_blend_color);
                _sgimgui_frame_stats(metal.pipeline.num_set_cull_mode);
                _sgimgui_frame_stats(metal.pipeline.num_set_front_facing_winding);
                _sgimgui_frame_stats(metal.pipeline.num_set_stencil_reference_value);
                _sgimgui_frame_stats(metal.pipeline.num_set_depth_bias);
                _sgimgui_frame_stats(metal.pipeline.num_set_render_pipeline_state);
                _sgimgui_frame_stats(metal.pipeline.num_set_depth_stencil_state);
                _sgimgui_frame_stats(metal.bindings.num_set_vertex_buffer);
                _sgimgui_frame_stats(metal.bindings.num_set_vertex_texture);
                _sgimgui_frame_stats(metal.bindings.num_set_vertex_sampler_state);
                _sgimgui_frame_stats(metal.bindings.num_set_fragment_buffer);
                _sgimgui_frame_stats(metal.bindings.num_set_fragment_texture);
                _sgimgui_frame_stats(metal.bindings.num_set_fragment_sampler_state);
                _sgimgui_frame_stats(metal.uniforms.num_set_vertex_buffer_offset);
                _sgimgui_frame_stats(metal.uniforms.num_set_fragment_buffer_offset);
                break;
            case SG_BACKEND_D3D11:
                _sgimgui_frame_stats(d3d11.pass.num_om_set_render_targets);
                _sgimgui_frame_stats(d3d11.pass.num_clear_render_target_view);
                _sgimgui_frame_stats(d3d11.pass.num_clear_depth_stencil_view);
                _sgimgui_frame_stats(d3d11.pass.num_resolve_subresource);
                _sgimgui_frame_stats(d3d11.pipeline.num_rs_set_state);
                _sgimgui_frame_stats(d3d11.pipeline.num_om_set_depth_stencil_state);
                _sgimgui_frame_stats(d3d11.pipeline.num_om_set_blend_state);
                _sgimgui_frame_stats(d3d11.pipeline.num_ia_set_primitive_topology);
                _sgimgui_frame_stats(d3d11.pipeline.num_ia_set_input_layout);
                _sgimgui_frame_stats(d3d11.pipeline.num_vs_set_shader);
                _sgimgui_frame_stats(d3d11.pipeline.num_vs_set_constant_buffers);
                _sgimgui_frame_stats(d3d11.pipeline.num_ps_set_shader);
                _sgimgui_frame_stats(d3d11.pipeline.num_ps_set_constant_buffers);
                _sgimgui_frame_stats(d3d11.bindings.num_ia_set_vertex_buffers);
                _sgimgui_frame_stats(d3d11.bindings.num_ia_set_index_buffer);
                _sgimgui_frame_stats(d3d11.bindings.num_vs_set_shader_resources);
                _sgimgui_frame_stats(d3d11.bindings.num_ps_set_shader_resources);
                _sgimgui_frame_stats(d3d11.bindings.num_vs_set_samplers);
                _sgimgui_frame_stats(d3d11.bindings.num_ps_set_samplers);
                _sgimgui_frame_stats(d3d11.uniforms.num_update_subresource);
                _sgimgui_frame_stats(d3d11.draw.num_draw_indexed_instanced);
                _sgimgui_frame_stats(d3d11.draw.num_draw_indexed);
                _sgimgui_frame_stats(d3d11.draw.num_draw_instanced);
                _sgimgui_frame_stats(d3d11.draw.num_draw);
                _sgimgui_frame_stats(d3d11.num_map);
                _sgimgui_frame_stats(d3d11.num_unmap);
                break;
            default: break;
        }
        igEndTable();
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
SOKOL_API_IMPL void sgimgui_init(sgimgui_t* ctx, const sgimgui_desc_t* desc) {
    SOKOL_ASSERT(ctx && desc);
    _sgimgui_clear(ctx, sizeof(sgimgui_t));
    ctx->init_tag = 0xABCDABCD;
    ctx->desc = _sgimgui_desc_defaults(desc);
    _sgimgui_capture_init(ctx);

    /* hook into sokol_gfx functions */
    sg_trace_hooks hooks;
    _sgimgui_clear(&hooks, sizeof(hooks));
    hooks.user_data = (void*) ctx;
    hooks.reset_state_cache = _sgimgui_reset_state_cache;
    hooks.make_buffer = _sgimgui_make_buffer;
    hooks.make_image = _sgimgui_make_image;
    hooks.make_sampler = _sgimgui_make_sampler;
    hooks.make_shader = _sgimgui_make_shader;
    hooks.make_pipeline = _sgimgui_make_pipeline;
    hooks.make_attachments = _sgimgui_make_attachments;
    hooks.destroy_buffer = _sgimgui_destroy_buffer;
    hooks.destroy_image = _sgimgui_destroy_image;
    hooks.destroy_sampler = _sgimgui_destroy_sampler;
    hooks.destroy_shader = _sgimgui_destroy_shader;
    hooks.destroy_pipeline = _sgimgui_destroy_pipeline;
    hooks.destroy_attachments = _sgimgui_destroy_attachments;
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
    hooks.end_pass = _sgimgui_end_pass;
    hooks.commit = _sgimgui_commit;
    hooks.alloc_buffer = _sgimgui_alloc_buffer;
    hooks.alloc_image = _sgimgui_alloc_image;
    hooks.alloc_sampler = _sgimgui_alloc_sampler;
    hooks.alloc_shader = _sgimgui_alloc_shader;
    hooks.alloc_pipeline = _sgimgui_alloc_pipeline;
    hooks.alloc_attachments = _sgimgui_alloc_attachments;
    hooks.dealloc_buffer = _sgimgui_dealloc_buffer;
    hooks.dealloc_image = _sgimgui_dealloc_image;
    hooks.dealloc_sampler = _sgimgui_dealloc_sampler;
    hooks.dealloc_shader = _sgimgui_dealloc_shader;
    hooks.dealloc_pipeline = _sgimgui_dealloc_pipeline;
    hooks.dealloc_attachments = _sgimgui_dealloc_attachments;
    hooks.init_buffer = _sgimgui_init_buffer;
    hooks.init_image = _sgimgui_init_image;
    hooks.init_sampler = _sgimgui_init_sampler;
    hooks.init_shader = _sgimgui_init_shader;
    hooks.init_pipeline = _sgimgui_init_pipeline;
    hooks.init_attachments = _sgimgui_init_attachments;
    hooks.uninit_buffer = _sgimgui_uninit_buffer;
    hooks.uninit_image = _sgimgui_uninit_image;
    hooks.uninit_sampler = _sgimgui_uninit_sampler;
    hooks.uninit_shader = _sgimgui_uninit_shader;
    hooks.uninit_pipeline = _sgimgui_uninit_pipeline;
    hooks.uninit_attachments = _sgimgui_uninit_attachments;
    hooks.fail_buffer = _sgimgui_fail_buffer;
    hooks.fail_image = _sgimgui_fail_image;
    hooks.fail_sampler = _sgimgui_fail_sampler;
    hooks.fail_shader = _sgimgui_fail_shader;
    hooks.fail_pipeline = _sgimgui_fail_pipeline;
    hooks.fail_attachments = _sgimgui_fail_attachments;
    hooks.push_debug_group = _sgimgui_push_debug_group;
    hooks.pop_debug_group = _sgimgui_pop_debug_group;
    ctx->hooks = sg_install_trace_hooks(&hooks);

    /* allocate resource debug-info slots */
    const sg_desc sgdesc = sg_query_desc();
    ctx->buffer_window.num_slots = sgdesc.buffer_pool_size;
    ctx->image_window.num_slots = sgdesc.image_pool_size;
    ctx->sampler_window.num_slots = sgdesc.sampler_pool_size;
    ctx->shader_window.num_slots = sgdesc.shader_pool_size;
    ctx->pipeline_window.num_slots = sgdesc.pipeline_pool_size;
    ctx->attachments_window.num_slots = sgdesc.attachments_pool_size;

    const size_t buffer_pool_size = (size_t)ctx->buffer_window.num_slots * sizeof(sgimgui_buffer_t);
    ctx->buffer_window.slots = (sgimgui_buffer_t*) _sgimgui_malloc_clear(&ctx->desc.allocator, buffer_pool_size);

    const size_t image_pool_size = (size_t)ctx->image_window.num_slots * sizeof(sgimgui_image_t);
    ctx->image_window.slots = (sgimgui_image_t*) _sgimgui_malloc_clear(&ctx->desc.allocator, image_pool_size);

    const size_t sampler_pool_size = (size_t)ctx->sampler_window.num_slots * sizeof(sgimgui_sampler_t);
    ctx->sampler_window.slots = (sgimgui_sampler_t*) _sgimgui_malloc_clear(&ctx->desc.allocator, sampler_pool_size);

    const size_t shader_pool_size = (size_t)ctx->shader_window.num_slots * sizeof(sgimgui_shader_t);
    ctx->shader_window.slots = (sgimgui_shader_t*) _sgimgui_malloc_clear(&ctx->desc.allocator, shader_pool_size);

    const size_t pipeline_pool_size = (size_t)ctx->pipeline_window.num_slots * sizeof(sgimgui_pipeline_t);
    ctx->pipeline_window.slots = (sgimgui_pipeline_t*) _sgimgui_malloc_clear(&ctx->desc.allocator, pipeline_pool_size);

    const size_t attachments_pool_size = (size_t)ctx->attachments_window.num_slots * sizeof(sgimgui_attachments_t);
    ctx->attachments_window.slots = (sgimgui_attachments_t*) _sgimgui_malloc_clear(&ctx->desc.allocator, attachments_pool_size);
}

SOKOL_API_IMPL void sgimgui_discard(sgimgui_t* ctx) {
    SOKOL_ASSERT(ctx && (ctx->init_tag == 0xABCDABCD));
    /* restore original trace hooks */
    sg_install_trace_hooks(&ctx->hooks);
    ctx->init_tag = 0;
    _sgimgui_capture_discard(ctx);
    if (ctx->buffer_window.slots) {
        for (int i = 0; i < ctx->buffer_window.num_slots; i++) {
            if (ctx->buffer_window.slots[i].res_id.id != SG_INVALID_ID) {
                _sgimgui_buffer_destroyed(ctx, i);
            }
        }
        _sgimgui_free(&ctx->desc.allocator, (void*)ctx->buffer_window.slots);
        ctx->buffer_window.slots = 0;
    }
    if (ctx->image_window.slots) {
        for (int i = 0; i < ctx->image_window.num_slots; i++) {
            if (ctx->image_window.slots[i].res_id.id != SG_INVALID_ID) {
                _sgimgui_image_destroyed(ctx, i);
            }
        }
        _sgimgui_free(&ctx->desc.allocator, (void*)ctx->image_window.slots);
        ctx->image_window.slots = 0;
    }
    if (ctx->sampler_window.slots) {
        for (int i = 0; i < ctx->sampler_window.num_slots; i++) {
            if (ctx->sampler_window.slots[i].res_id.id != SG_INVALID_ID) {
                _sgimgui_sampler_destroyed(ctx, i);
            }
        }
        _sgimgui_free(&ctx->desc.allocator, (void*)ctx->sampler_window.slots);
        ctx->sampler_window.slots = 0;
    }
    if (ctx->shader_window.slots) {
        for (int i = 0; i < ctx->shader_window.num_slots; i++) {
            if (ctx->shader_window.slots[i].res_id.id != SG_INVALID_ID) {
                _sgimgui_shader_destroyed(ctx, i);
            }
        }
        _sgimgui_free(&ctx->desc.allocator, (void*)ctx->shader_window.slots);
        ctx->shader_window.slots = 0;
    }
    if (ctx->pipeline_window.slots) {
        for (int i = 0; i < ctx->pipeline_window.num_slots; i++) {
            if (ctx->pipeline_window.slots[i].res_id.id != SG_INVALID_ID) {
                _sgimgui_pipeline_destroyed(ctx, i);
            }
        }
        _sgimgui_free(&ctx->desc.allocator, (void*)ctx->pipeline_window.slots);
        ctx->pipeline_window.slots = 0;
    }
    if (ctx->attachments_window.slots) {
        for (int i = 0; i < ctx->attachments_window.num_slots; i++) {
            if (ctx->attachments_window.slots[i].res_id.id != SG_INVALID_ID) {
                _sgimgui_attachments_destroyed(ctx, i);
            }
        }
        _sgimgui_free(&ctx->desc.allocator, (void*)ctx->attachments_window.slots);
        ctx->attachments_window.slots = 0;
    }
}

SOKOL_API_IMPL void sgimgui_draw(sgimgui_t* ctx) {
    SOKOL_ASSERT(ctx && (ctx->init_tag == 0xABCDABCD));
    sgimgui_draw_buffer_window(ctx);
    sgimgui_draw_image_window(ctx);
    sgimgui_draw_sampler_window(ctx);
    sgimgui_draw_shader_window(ctx);
    sgimgui_draw_pipeline_window(ctx);
    sgimgui_draw_attachments_window(ctx);
    sgimgui_draw_capture_window(ctx);
    sgimgui_draw_capabilities_window(ctx);
    sgimgui_draw_frame_stats_window(ctx);
}

SOKOL_API_IMPL void sgimgui_draw_menu(sgimgui_t* ctx, const char* title) {
    SOKOL_ASSERT(ctx && (ctx->init_tag == 0xABCDABCD));
    SOKOL_ASSERT(title);
    if (igBeginMenu(title, true)) {
        igMenuItem_BoolPtr("Capabilities", 0, &ctx->caps_window.open, true);
        igMenuItem_BoolPtr("Frame Stats", 0, &ctx->frame_stats_window.open, true);
        igMenuItem_BoolPtr("Buffers", 0, &ctx->buffer_window.open, true);
        igMenuItem_BoolPtr("Images", 0, &ctx->image_window.open, true);
        igMenuItem_BoolPtr("Samplers", 0, &ctx->sampler_window.open, true);
        igMenuItem_BoolPtr("Shaders", 0, &ctx->shader_window.open, true);
        igMenuItem_BoolPtr("Pipelines", 0, &ctx->pipeline_window.open, true);
        igMenuItem_BoolPtr("Attachments", 0, &ctx->attachments_window.open, true);
        igMenuItem_BoolPtr("Calls", 0, &ctx->capture_window.open, true);
        igEndMenu();
    }
}

SOKOL_API_IMPL void sgimgui_draw_buffer_window(sgimgui_t* ctx) {
    SOKOL_ASSERT(ctx && (ctx->init_tag == 0xABCDABCD));
    if (!ctx->buffer_window.open) {
        return;
    }
    igSetNextWindowSize(IMVEC2(440, 280), ImGuiCond_Once);
    if (igBegin("Buffers", &ctx->buffer_window.open, 0)) {
        sgimgui_draw_buffer_window_content(ctx);
    }
    igEnd();
}

SOKOL_API_IMPL void sgimgui_draw_image_window(sgimgui_t* ctx) {
    SOKOL_ASSERT(ctx && (ctx->init_tag == 0xABCDABCD));
    if (!ctx->image_window.open) {
        return;
    }
    igSetNextWindowSize(IMVEC2(440, 400), ImGuiCond_Once);
    if (igBegin("Images", &ctx->image_window.open, 0)) {
        sgimgui_draw_image_window_content(ctx);
    }
    igEnd();
}

SOKOL_API_IMPL void sgimgui_draw_sampler_window(sgimgui_t* ctx) {
    SOKOL_ASSERT(ctx && (ctx->init_tag == 0xABCDABCD));
    if (!ctx->sampler_window.open) {
        return;
    }
    igSetNextWindowSize(IMVEC2(440, 400), ImGuiCond_Once);
    if (igBegin("Samplers", &ctx->sampler_window.open, 0)) {
        sgimgui_draw_sampler_window_content(ctx);
    }
    igEnd();
}

SOKOL_API_IMPL void sgimgui_draw_shader_window(sgimgui_t* ctx) {
    SOKOL_ASSERT(ctx && (ctx->init_tag == 0xABCDABCD));
    if (!ctx->shader_window.open) {
        return;
    }
    igSetNextWindowSize(IMVEC2(440, 400), ImGuiCond_Once);
    if (igBegin("Shaders", &ctx->shader_window.open, 0)) {
        sgimgui_draw_shader_window_content(ctx);
    }
    igEnd();
}

SOKOL_API_IMPL void sgimgui_draw_pipeline_window(sgimgui_t* ctx) {
    SOKOL_ASSERT(ctx && (ctx->init_tag == 0xABCDABCD));
    if (!ctx->pipeline_window.open) {
        return;
    }
    igSetNextWindowSize(IMVEC2(540, 400), ImGuiCond_Once);
    if (igBegin("Pipelines", &ctx->pipeline_window.open, 0)) {
        sgimgui_draw_pipeline_window_content(ctx);
    }
    igEnd();
}

SOKOL_API_IMPL void sgimgui_draw_attachments_window(sgimgui_t* ctx) {
    SOKOL_ASSERT(ctx && (ctx->init_tag == 0xABCDABCD));
    if (!ctx->attachments_window.open) {
        return;
    }
    igSetNextWindowSize(IMVEC2(440, 400), ImGuiCond_Once);
    if (igBegin("Attachments", &ctx->attachments_window.open, 0)) {
        sgimgui_draw_attachments_window_content(ctx);
    }
    igEnd();
}

SOKOL_API_IMPL void sgimgui_draw_capture_window(sgimgui_t* ctx) {
    SOKOL_ASSERT(ctx && (ctx->init_tag == 0xABCDABCD));
    if (!ctx->capture_window.open) {
        return;
    }
    igSetNextWindowSize(IMVEC2(640, 400), ImGuiCond_Once);
    if (igBegin("Frame Capture", &ctx->capture_window.open, 0)) {
        sgimgui_draw_capture_window_content(ctx);
    }
    igEnd();
}

SOKOL_API_IMPL void sgimgui_draw_capabilities_window(sgimgui_t* ctx) {
    SOKOL_ASSERT(ctx && (ctx->init_tag == 0xABCDABCD));
    if (!ctx->caps_window.open) {
        return;
    }
    igSetNextWindowSize(IMVEC2(440, 400), ImGuiCond_Once);
    if (igBegin("Capabilities", &ctx->caps_window.open, 0)) {
        sgimgui_draw_capabilities_window_content(ctx);
    }
    igEnd();
}

SOKOL_API_IMPL void sgimgui_draw_frame_stats_window(sgimgui_t* ctx) {
    SOKOL_ASSERT(ctx && (ctx->init_tag == 0xABCDABCD));
    if (!ctx->frame_stats_window.open) {
        return;
    }
    igSetNextWindowSize(IMVEC2(512, 400), ImGuiCond_Once);
    if (igBegin("Frame Stats", &ctx->frame_stats_window.open, 0)) {
        sgimgui_draw_frame_stats_window_content(ctx);
    }
    igEnd();
}

SOKOL_API_IMPL void sgimgui_draw_buffer_window_content(sgimgui_t* ctx) {
    SOKOL_ASSERT(ctx && (ctx->init_tag == 0xABCDABCD));
    _sgimgui_draw_buffer_list(ctx);
    igSameLine(0,-1);
    _sgimgui_draw_buffer_panel(ctx, ctx->buffer_window.sel_buf);
}

SOKOL_API_IMPL void sgimgui_draw_image_window_content(sgimgui_t* ctx) {
    SOKOL_ASSERT(ctx && (ctx->init_tag == 0xABCDABCD));
    _sgimgui_draw_image_list(ctx);
    igSameLine(0,-1);
    _sgimgui_draw_image_panel(ctx, ctx->image_window.sel_img);
}

SOKOL_API_IMPL void sgimgui_draw_sampler_window_content(sgimgui_t* ctx) {
    SOKOL_ASSERT(ctx && (ctx->init_tag == 0xABCDABCD));
    _sgimgui_draw_sampler_list(ctx);
    igSameLine(0,-1);
    _sgimgui_draw_sampler_panel(ctx, ctx->sampler_window.sel_smp);
}

SOKOL_API_IMPL void sgimgui_draw_shader_window_content(sgimgui_t* ctx) {
    SOKOL_ASSERT(ctx && (ctx->init_tag == 0xABCDABCD));
    _sgimgui_draw_shader_list(ctx);
    igSameLine(0,-1);
    _sgimgui_draw_shader_panel(ctx, ctx->shader_window.sel_shd);
}

SOKOL_API_IMPL void sgimgui_draw_pipeline_window_content(sgimgui_t* ctx) {
    SOKOL_ASSERT(ctx && (ctx->init_tag == 0xABCDABCD));
    _sgimgui_draw_pipeline_list(ctx);
    igSameLine(0,-1);
    _sgimgui_draw_pipeline_panel(ctx, ctx->pipeline_window.sel_pip);
}

SOKOL_API_IMPL void sgimgui_draw_attachments_window_content(sgimgui_t* ctx) {
    SOKOL_ASSERT(ctx && (ctx->init_tag == 0xABCDABCD));
    _sgimgui_draw_attachments_list(ctx);
    igSameLine(0,-1);
    _sgimgui_draw_attachments_panel(ctx, ctx->attachments_window.sel_atts);
}

SOKOL_API_IMPL void sgimgui_draw_capture_window_content(sgimgui_t* ctx) {
    SOKOL_ASSERT(ctx && (ctx->init_tag == 0xABCDABCD));
    _sgimgui_draw_capture_list(ctx);
    igSameLine(0,-1);
    _sgimgui_draw_capture_panel(ctx);
}

SOKOL_API_IMPL void sgimgui_draw_capabilities_window_content(sgimgui_t* ctx) {
    SOKOL_ASSERT(ctx && (ctx->init_tag == 0xABCDABCD));
    _SOKOL_UNUSED(ctx);
    _sgimgui_draw_caps_panel();
}

SOKOL_API_IMPL void sgimgui_draw_frame_stats_window_content(sgimgui_t* ctx) {
    SOKOL_ASSERT(ctx && (ctx->init_tag == 0xABCDABCD));
    ctx->frame_stats_window.stats = sg_query_frame_stats();
    _sgimgui_draw_frame_stats_panel(ctx);
}

#endif /* SOKOL_GFX_IMGUI_IMPL */
