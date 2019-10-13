#ifndef SOKOL_GFX_IMGUI_INCLUDED
/*
    sokol_gfx_imgui.h -- debug-inspection UI for sokol_gfx.h using Dear ImGui

    Project URL: https://github.com/floooh/sokol

    Do this:

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
        SOKOL_MALLOC(s)     -- your own memory allocation function, default: malloc(s)
        SOKOL_FREE(p)       -- your own memory free function, default: free(p)
        SOKOL_API_DECL      - public function declaration prefix (default: extern)
        SOKOL_API_IMPL      - public function implementation prefix (default: -)

    If sokol_gfx_imgui.h is compiled as a DLL, define the following before
    including the declaration or implementation:

    SOKOL_DLL

    On Windows, SOKOL_DLL will define SOKOL_API_DECL as __declspec(dllexport)
    or __declspec(dllimport) as needed.

    STEP BY STEP:
    =============
    --- create an sg_imgui_t struct (which must be preserved between frames)
        and initialize it with:

            sg_imgui_init(&sg_imgui);

    --- somewhere in the per-frame code call:

            sg_imgui_draw(&sg_imgui)

        this won't draw anything yet, since no windows are open.

    --- open and close windows directly by setting the following public
        booleans in the sg_imgui_t struct:

            sg_imgui.buffers.open = true;
            sg_imgui.images.open = true;
            sg_imgui.shaders.open = true;
            sg_imgui.pipelines.open = true;
            sg_imgui.passes.open = true;
            sg_imgui.capture.open = true;

        ...for instance, to control the window visibility through
        menu items, the following code can be used:

            if (ImGui::BeginMainMenuBar()) {
                if (ImGui::BeginMenu("sokol-gfx")) {
                    ImGui::MenuItem("Buffers", 0, &sg_imgui.buffers.open);
                    ImGui::MenuItem("Images", 0, &sg_imgui.images.open);
                    ImGui::MenuItem("Shaders", 0, &sg_imgui.shaders.open);
                    ImGui::MenuItem("Pipelines", 0, &sg_imgui.pipelines.open);
                    ImGui::MenuItem("Passes", 0, &sg_imgui.passes.open);
                    ImGui::MenuItem("Calls", 0, &sg_imgui.capture.open);
                    ImGui::EndMenu();
                }
                ImGui::EndMainMenuBar();
            }

    --- before application shutdown, call:

            sg_imgui_discard(&sg_imgui);

        ...this is not strictly necessary because the application exits
        anyway, but not doing this may trigger memory leak detection tools.

    --- finally, your application needs an ImGui renderer, you can either
        provide your own, or drop in the sokol_imgui.h utility header

    ALTERNATIVE DRAWING FUNCTIONS:
    ==============================
    Instead of the convenient, but all-in-one sg_imgui_draw() function,
    you can also use the following granular functions which might allow
    better integration with your existing UI:

    The following functions only render the window *content* (so you
    can integrate the UI into you own windows):

        void sg_imgui_draw_buffers_content(sg_imgui_t* ctx);
        void sg_imgui_draw_images_content(sg_imgui_t* ctx);
        void sg_imgui_draw_shaders_content(sg_imgui_t* ctx);
        void sg_imgui_draw_pipelines_content(sg_imgui_t* ctx);
        void sg_imgui_draw_passes_content(sg_imgui_t* ctx);
        void sg_imgui_draw_capture_content(sg_imgui_t* ctx);

    And these are the 'full window' drawing functions:

        void sg_imgui_draw_buffers_window(sg_imgui_t* ctx);
        void sg_imgui_draw_images_window(sg_imgui_t* ctx);
        void sg_imgui_draw_shaders_window(sg_imgui_t* ctx);
        void sg_imgui_draw_pipelines_window(sg_imgui_t* ctx);
        void sg_imgui_draw_passes_window(sg_imgui_t* ctx);
        void sg_imgui_draw_capture_window(sg_imgui_t* ctx);

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

#define SG_IMGUI_STRBUF_LEN (96)
/* max number of captured calls per frame */
#define SG_IMGUI_MAX_FRAMECAPTURE_ITEMS (4096)

typedef struct {
    char buf[SG_IMGUI_STRBUF_LEN];
} sg_imgui_str_t;

typedef struct {
    sg_buffer res_id;
    sg_imgui_str_t label;
    sg_buffer_desc desc;
} sg_imgui_buffer_t;

typedef struct {
    sg_image res_id;
    float ui_scale;
    sg_imgui_str_t label;
    sg_image_desc desc;
} sg_imgui_image_t;

typedef struct {
    sg_shader res_id;
    sg_imgui_str_t label;
    sg_imgui_str_t vs_entry;
    sg_imgui_str_t vs_image_name[SG_MAX_SHADERSTAGE_IMAGES];
    sg_imgui_str_t vs_uniform_name[SG_MAX_SHADERSTAGE_UBS][SG_MAX_UB_MEMBERS];
    sg_imgui_str_t fs_entry;
    sg_imgui_str_t fs_image_name[SG_MAX_SHADERSTAGE_IMAGES];
    sg_imgui_str_t fs_uniform_name[SG_MAX_SHADERSTAGE_UBS][SG_MAX_UB_MEMBERS];
    sg_imgui_str_t attr_name[SG_MAX_VERTEX_ATTRIBUTES];
    sg_imgui_str_t attr_sem_name[SG_MAX_VERTEX_ATTRIBUTES];
    sg_shader_desc desc;
} sg_imgui_shader_t;

typedef struct {
    sg_pipeline res_id;
    sg_imgui_str_t label;
    sg_pipeline_desc desc;
} sg_imgui_pipeline_t;

typedef struct {
    sg_pass res_id;
    sg_imgui_str_t label;
    float color_image_scale[SG_MAX_COLOR_ATTACHMENTS];
    float ds_image_scale;
    sg_pass_desc desc;
} sg_imgui_pass_t;

typedef struct {
    bool open;
    int num_slots;
    sg_buffer sel_buf;
    sg_imgui_buffer_t* slots;
} sg_imgui_buffers_t;

typedef struct {
    bool open;
    int num_slots;
    sg_image sel_img;
    sg_imgui_image_t* slots;
} sg_imgui_images_t;

typedef struct {
    bool open;
    int num_slots;
    sg_shader sel_shd;
    sg_imgui_shader_t* slots;
} sg_imgui_shaders_t;

typedef struct {
    bool open;
    int num_slots;
    sg_pipeline sel_pip;
    sg_imgui_pipeline_t* slots;
} sg_imgui_pipelines_t;

typedef struct {
    bool open;
    int num_slots;
    sg_pass sel_pass;
    sg_imgui_pass_t* slots;
} sg_imgui_passes_t;

typedef enum {
    SG_IMGUI_CMD_INVALID,
    SG_IMGUI_CMD_RESET_STATE_CACHE,
    SG_IMGUI_CMD_MAKE_BUFFER,
    SG_IMGUI_CMD_MAKE_IMAGE,
    SG_IMGUI_CMD_MAKE_SHADER,
    SG_IMGUI_CMD_MAKE_PIPELINE,
    SG_IMGUI_CMD_MAKE_PASS,
    SG_IMGUI_CMD_DESTROY_BUFFER,
    SG_IMGUI_CMD_DESTROY_IMAGE,
    SG_IMGUI_CMD_DESTROY_SHADER,
    SG_IMGUI_CMD_DESTROY_PIPELINE,
    SG_IMGUI_CMD_DESTROY_PASS,
    SG_IMGUI_CMD_UPDATE_BUFFER,
    SG_IMGUI_CMD_UPDATE_IMAGE,
    SG_IMGUI_CMD_APPEND_BUFFER,
    SG_IMGUI_CMD_BEGIN_DEFAULT_PASS,
    SG_IMGUI_CMD_BEGIN_PASS,
    SG_IMGUI_CMD_APPLY_VIEWPORT,
    SG_IMGUI_CMD_APPLY_SCISSOR_RECT,
    SG_IMGUI_CMD_APPLY_PIPELINE,
    SG_IMGUI_CMD_APPLY_BINDINGS,
    SG_IMGUI_CMD_APPLY_UNIFORMS,
    SG_IMGUI_CMD_DRAW,
    SG_IMGUI_CMD_END_PASS,
    SG_IMGUI_CMD_COMMIT,
    SG_IMGUI_CMD_ALLOC_BUFFER,
    SG_IMGUI_CMD_ALLOC_IMAGE,
    SG_IMGUI_CMD_ALLOC_SHADER,
    SG_IMGUI_CMD_ALLOC_PIPELINE,
    SG_IMGUI_CMD_ALLOC_PASS,
    SG_IMGUI_CMD_INIT_BUFFER,
    SG_IMGUI_CMD_INIT_IMAGE,
    SG_IMGUI_CMD_INIT_SHADER,
    SG_IMGUI_CMD_INIT_PIPELINE,
    SG_IMGUI_CMD_INIT_PASS,
    SG_IMGUI_CMD_FAIL_BUFFER,
    SG_IMGUI_CMD_FAIL_IMAGE,
    SG_IMGUI_CMD_FAIL_SHADER,
    SG_IMGUI_CMD_FAIL_PIPELINE,
    SG_IMGUI_CMD_FAIL_PASS,
    SG_IMGUI_CMD_PUSH_DEBUG_GROUP,
    SG_IMGUI_CMD_POP_DEBUG_GROUP,
    SG_IMGUI_CMD_ERR_BUFFER_POOL_EXHAUSTED,
    SG_IMGUI_CMD_ERR_IMAGE_POOL_EXHAUSTED,
    SG_IMGUI_CMD_ERR_SHADER_POOL_EXHAUSTED,
    SG_IMGUI_CMD_ERR_PIPELINE_POOL_EXHAUSTED,
    SG_IMGUI_CMD_ERR_PASS_POOL_EXHAUSTED,
    SG_IMGUI_CMD_ERR_CONTEXT_MISMATCH,
    SG_IMGUI_CMD_ERR_PASS_INVALID,
    SG_IMGUI_CMD_ERR_DRAW_INVALID,
    SG_IMGUI_CMD_ERR_BINDINGS_INVALID,
} sg_imgui_cmd_t;

typedef struct {
    sg_buffer result;
} sg_imgui_args_make_buffer_t;

typedef struct {
    sg_image result;
} sg_imgui_args_make_image_t;

typedef struct {
    sg_shader result;
} sg_imgui_args_make_shader_t;

typedef struct {
    sg_pipeline result;
} sg_imgui_args_make_pipeline_t;

typedef struct {
    sg_pass result;
} sg_imgui_args_make_pass_t;

typedef struct {
    sg_buffer buffer;
} sg_imgui_args_destroy_buffer_t;

typedef struct {
    sg_image image;
} sg_imgui_args_destroy_image_t;

typedef struct {
    sg_shader shader;
} sg_imgui_args_destroy_shader_t;

typedef struct {
    sg_pipeline pipeline;
} sg_imgui_args_destroy_pipeline_t;

typedef struct {
    sg_pass pass;
} sg_imgui_args_destroy_pass_t;

typedef struct {
    sg_buffer buffer;
    int data_size;
} sg_imgui_args_update_buffer_t;

typedef struct {
    sg_image image;
} sg_imgui_args_update_image_t;

typedef struct {
    sg_buffer buffer;
    int data_size;
    int result;
} sg_imgui_args_append_buffer_t;

typedef struct {
    sg_pass_action action;
    int width;
    int height;
} sg_imgui_args_begin_default_pass_t;

typedef struct {
    sg_pass pass;
    sg_pass_action action;
} sg_imgui_args_begin_pass_t;

typedef struct {
    int x, y, width, height;
    bool origin_top_left;
} sg_imgui_args_apply_viewport_t;

typedef struct {
    int x, y, width, height;
    bool origin_top_left;
} sg_imgui_args_apply_scissor_rect_t;

typedef struct {
    sg_pipeline pipeline;
} sg_imgui_args_apply_pipeline_t;

typedef struct {
    sg_bindings bindings;
} sg_imgui_args_apply_bindings_t;

typedef struct {
    sg_shader_stage stage;
    int ub_index;
    const void* data;
    int num_bytes;
    sg_pipeline pipeline;   /* the pipeline which was active at this call */
    uint32_t ubuf_pos;      /* start of copied data in capture buffer */
} sg_imgui_args_apply_uniforms_t;

typedef struct {
    int base_element;
    int num_elements;
    int num_instances;
} sg_imgui_args_draw_t;

typedef struct {
    sg_buffer result;
} sg_imgui_args_alloc_buffer_t;

typedef struct {
    sg_image result;
} sg_imgui_args_alloc_image_t;

typedef struct {
    sg_shader result;
} sg_imgui_args_alloc_shader_t;

typedef struct {
    sg_pipeline result;
} sg_imgui_args_alloc_pipeline_t;

typedef struct {
    sg_pass result;
} sg_imgui_args_alloc_pass_t;

typedef struct {
    sg_buffer buffer;
} sg_imgui_args_init_buffer_t;

typedef struct {
    sg_image image;
} sg_imgui_args_init_image_t;

typedef struct {
    sg_shader shader;
} sg_imgui_args_init_shader_t;

typedef struct {
    sg_pipeline pipeline;
} sg_imgui_args_init_pipeline_t;

typedef struct {
    sg_pass pass;
} sg_imgui_args_init_pass_t;

typedef struct {
    sg_buffer buffer;
} sg_imgui_args_fail_buffer_t;

typedef struct {
    sg_image image;
} sg_imgui_args_fail_image_t;

typedef struct {
    sg_shader shader;
} sg_imgui_args_fail_shader_t;

typedef struct {
    sg_pipeline pipeline;
} sg_imgui_args_fail_pipeline_t;

typedef struct {
    sg_pass pass;
} sg_imgui_args_fail_pass_t;

typedef struct {
    sg_imgui_str_t name;
} sg_imgui_args_push_debug_group_t;

typedef union {
    sg_imgui_args_make_buffer_t make_buffer;
    sg_imgui_args_make_image_t make_image;
    sg_imgui_args_make_shader_t make_shader;
    sg_imgui_args_make_pipeline_t make_pipeline;
    sg_imgui_args_make_pass_t make_pass;
    sg_imgui_args_destroy_buffer_t destroy_buffer;
    sg_imgui_args_destroy_image_t destroy_image;
    sg_imgui_args_destroy_shader_t destroy_shader;
    sg_imgui_args_destroy_pipeline_t destroy_pipeline;
    sg_imgui_args_destroy_pass_t destroy_pass;
    sg_imgui_args_update_buffer_t update_buffer;
    sg_imgui_args_update_image_t update_image;
    sg_imgui_args_append_buffer_t append_buffer;
    sg_imgui_args_begin_default_pass_t begin_default_pass;
    sg_imgui_args_begin_pass_t begin_pass;
    sg_imgui_args_apply_viewport_t apply_viewport;
    sg_imgui_args_apply_scissor_rect_t apply_scissor_rect;
    sg_imgui_args_apply_pipeline_t apply_pipeline;
    sg_imgui_args_apply_bindings_t apply_bindings;
    sg_imgui_args_apply_uniforms_t apply_uniforms;
    sg_imgui_args_draw_t draw;
    sg_imgui_args_alloc_buffer_t alloc_buffer;
    sg_imgui_args_alloc_image_t alloc_image;
    sg_imgui_args_alloc_shader_t alloc_shader;
    sg_imgui_args_alloc_pipeline_t alloc_pipeline;
    sg_imgui_args_alloc_pass_t alloc_pass;
    sg_imgui_args_init_buffer_t init_buffer;
    sg_imgui_args_init_image_t init_image;
    sg_imgui_args_init_shader_t init_shader;
    sg_imgui_args_init_pipeline_t init_pipeline;
    sg_imgui_args_init_pass_t init_pass;
    sg_imgui_args_fail_buffer_t fail_buffer;
    sg_imgui_args_fail_image_t fail_image;
    sg_imgui_args_fail_shader_t fail_shader;
    sg_imgui_args_fail_pipeline_t fail_pipeline;
    sg_imgui_args_fail_pass_t fail_pass;
    sg_imgui_args_push_debug_group_t push_debug_group;
} sg_imgui_args_t;

typedef struct {
    sg_imgui_cmd_t cmd;
    uint32_t color;
    sg_imgui_args_t args;
} sg_imgui_capture_item_t;

typedef struct {
    uint32_t ubuf_size;     /* size of uniform capture buffer in bytes */
    uint32_t ubuf_pos;      /* current uniform buffer pos */
    uint8_t* ubuf;          /* buffer for capturing uniform updates */
    uint32_t num_items;
    sg_imgui_capture_item_t items[SG_IMGUI_MAX_FRAMECAPTURE_ITEMS];
} sg_imgui_capture_bucket_t;

/* double-buffered call-capture buckets, one bucket is currently recorded,
   the previous bucket is displayed
*/
typedef struct {
    bool open;
    uint32_t bucket_index;      /* which bucket to record to, 0 or 1 */
    uint32_t sel_item;          /* currently selected capture item by index */
    sg_imgui_capture_bucket_t bucket[2];
} sg_imgui_capture_t;

typedef struct {
    bool open;
} sg_imgui_caps_t;

typedef struct {
    uint32_t init_tag;
    sg_imgui_buffers_t buffers;
    sg_imgui_images_t images;
    sg_imgui_shaders_t shaders;
    sg_imgui_pipelines_t pipelines;
    sg_imgui_passes_t passes;
    sg_imgui_capture_t capture;
    sg_imgui_caps_t caps;
    sg_pipeline cur_pipeline;
    sg_trace_hooks hooks;
} sg_imgui_t;

SOKOL_API_DECL void sg_imgui_init(sg_imgui_t* ctx);
SOKOL_API_DECL void sg_imgui_discard(sg_imgui_t* ctx);
SOKOL_API_DECL void sg_imgui_draw(sg_imgui_t* ctx);

SOKOL_API_DECL void sg_imgui_draw_buffers_content(sg_imgui_t* ctx);
SOKOL_API_DECL void sg_imgui_draw_images_content(sg_imgui_t* ctx);
SOKOL_API_DECL void sg_imgui_draw_shaders_content(sg_imgui_t* ctx);
SOKOL_API_DECL void sg_imgui_draw_pipelines_content(sg_imgui_t* ctx);
SOKOL_API_DECL void sg_imgui_draw_passes_content(sg_imgui_t* ctx);
SOKOL_API_DECL void sg_imgui_draw_capture_content(sg_imgui_t* ctx);
SOKOL_API_DECL void sg_imgui_draw_capabilities_content(sg_imgui_t* ctx);

SOKOL_API_DECL void sg_imgui_draw_buffers_window(sg_imgui_t* ctx);
SOKOL_API_DECL void sg_imgui_draw_images_window(sg_imgui_t* ctx);
SOKOL_API_DECL void sg_imgui_draw_shaders_window(sg_imgui_t* ctx);
SOKOL_API_DECL void sg_imgui_draw_pipelines_window(sg_imgui_t* ctx);
SOKOL_API_DECL void sg_imgui_draw_passes_window(sg_imgui_t* ctx);
SOKOL_API_DECL void sg_imgui_draw_capture_window(sg_imgui_t* ctx);
SOKOL_API_DECL void sg_imgui_draw_capabilities_window(sg_imgui_t* ctx);

#if defined(__cplusplus)
} /* extern "C" */
#endif
#endif /* SOKOL_GFX_IMGUI_INCLUDED */

/*=== IMPLEMENTATION =========================================================*/
#ifdef SOKOL_GFX_IMGUI_IMPL
#define SOKOL_GFX_IMGUI_IMPL_INCLUDED (1)
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

#define _SG_IMGUI_SLOT_MASK (0xFFFF)
#define _SG_IMGUI_LIST_WIDTH (192)
#define _SG_IMGUI_COLOR_OTHER 0xFFCCCCCC
#define _SG_IMGUI_COLOR_RSRC 0xFF00FFFF
#define _SG_IMGUI_COLOR_DRAW 0xFF00FF00
#define _SG_IMGUI_COLOR_ERR 0xFF8888FF

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
_SOKOL_PRIVATE void igPushIDInt(int int_id) {
    return ImGui::PushID(int_id);
}
_SOKOL_PRIVATE void igPopID() {
    return ImGui::PopID();
}
_SOKOL_PRIVATE bool igSelectable(const char* label,bool selected,ImGuiSelectableFlags flags,const ImVec2 size) {
    return ImGui::Selectable(label,selected,flags,size);
}
_SOKOL_PRIVATE bool igSmallButton(const char* label) {
    return ImGui::SmallButton(label);
}
_SOKOL_PRIVATE bool igBeginChild(const char* str_id,const ImVec2 size,bool border,ImGuiWindowFlags flags) {
    return ImGui::BeginChild(str_id,size,border,flags);
}
_SOKOL_PRIVATE void igEndChild() {
    return ImGui::EndChild();
}
_SOKOL_PRIVATE void igPushStyleColorU32(ImGuiCol idx, ImU32 col) {
    return ImGui::PushStyleColor(idx,col);
}
_SOKOL_PRIVATE void igPopStyleColor(int count) {
    return ImGui::PopStyleColor(count);
}
_SOKOL_PRIVATE bool igTreeNodeStrStr(const char* str_id,const char* fmt,...) {
    va_list args;
    va_start(args, fmt);
    bool ret = ImGui::TreeNodeV(str_id,fmt,args);
    va_end(args);
    return ret;
}
_SOKOL_PRIVATE bool igTreeNodeStr(const char* label) {
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
_SOKOL_PRIVATE bool igSliderFloat(const char* label,float* v,float v_min,float v_max,const char* format,float power) {
    return ImGui::SliderFloat(label,v,v_min,v_max,format,power);
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
#else
#define IMVEC2(x,y) (ImVec2){x,y}
#define IMVEC4(x,y,z,w) (ImVec4){x,y,z,w}
#endif

/*--- UTILS ------------------------------------------------------------------*/
_SOKOL_PRIVATE int _sg_imgui_slot_index(uint32_t id) {
    int slot_index = (int) (id & _SG_IMGUI_SLOT_MASK);
    SOKOL_ASSERT(0 != slot_index);
    return slot_index;
}

_SOKOL_PRIVATE int _sg_imgui_uniform_size(sg_uniform_type type, int count) {
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

_SOKOL_PRIVATE void* _sg_imgui_alloc(int size) {
    SOKOL_ASSERT(size > 0);
    return SOKOL_MALLOC(size);
}

_SOKOL_PRIVATE void _sg_imgui_free(void* ptr) {
    if (ptr) {
        SOKOL_FREE(ptr);
    }
}

_SOKOL_PRIVATE void* _sg_imgui_realloc(void* old_ptr, int old_size, int new_size) {
    SOKOL_ASSERT((new_size > 0) && (new_size > old_size));
    void* new_ptr = SOKOL_MALLOC(new_size);
    SOKOL_ASSERT(new_ptr);
    if (old_ptr) {
        if (old_size > 0) {
            memcpy(new_ptr, old_ptr, old_size);
        }
        _sg_imgui_free(old_ptr);
    }
    return new_ptr;
}

_SOKOL_PRIVATE void _sg_imgui_strcpy(sg_imgui_str_t* dst, const char* src) {
    SOKOL_ASSERT(dst);
    if (src) {
        #if defined(_MSC_VER)
        strncpy_s(dst->buf, SG_IMGUI_STRBUF_LEN, src, (SG_IMGUI_STRBUF_LEN-1));
        #else
        strncpy(dst->buf, src, SG_IMGUI_STRBUF_LEN);
        #endif
        dst->buf[SG_IMGUI_STRBUF_LEN-1] = 0;
    }
    else {
        memset(dst->buf, 0, SG_IMGUI_STRBUF_LEN);
    }
}

_SOKOL_PRIVATE sg_imgui_str_t _sg_imgui_make_str(const char* str) {
    sg_imgui_str_t res;
    _sg_imgui_strcpy(&res, str);
    return res;
}

_SOKOL_PRIVATE const char* _sg_imgui_str_dup(const char* src) {
    SOKOL_ASSERT(src);
    int len = (int) strlen(src) + 1;
    char* dst = (char*) _sg_imgui_alloc(len);
    memcpy(dst, src, len);
    return (const char*) dst;
}

_SOKOL_PRIVATE const uint8_t* _sg_imgui_bin_dup(const uint8_t* src, int num_bytes) {
    SOKOL_ASSERT(src && (num_bytes > 0));
    uint8_t* dst = (uint8_t*) _sg_imgui_alloc(num_bytes);
    memcpy(dst, src, num_bytes);
    return (const uint8_t*) dst;
}

_SOKOL_PRIVATE void _sg_imgui_snprintf(sg_imgui_str_t* dst, const char* fmt, ...) {
    SOKOL_ASSERT(dst);
    va_list args;
    va_start(args, fmt);
    vsnprintf(dst->buf, sizeof(dst->buf), fmt, args);
    dst->buf[sizeof(dst->buf)-1] = 0;
    va_end(args);
}

/*--- STRING CONVERSION ------------------------------------------------------*/
_SOKOL_PRIVATE const char* _sg_imgui_resourcestate_string(sg_resource_state s) {
    switch (s) {
        case SG_RESOURCESTATE_INITIAL:  return "SG_RESOURCESTATE_INITIAL";
        case SG_RESOURCESTATE_ALLOC:    return "SG_RESOURCESTATE_ALLOC";
        case SG_RESOURCESTATE_VALID:    return "SG_RESOURCESTATE_VALID";
        case SG_RESOURCESTATE_FAILED:   return "SG_RESOURCESTATE_FAILED";
        default:                        return "SG_RESOURCESTATE_INVALID";
    }
}

_SOKOL_PRIVATE void _sg_imgui_draw_resource_slot(const sg_slot_info* slot) {
    igText("ResId: %08X", slot->res_id);
    igText("CtxId: %08X", slot->ctx_id);
    igText("State: %s", _sg_imgui_resourcestate_string(slot->state));
}

_SOKOL_PRIVATE const char* _sg_imgui_backend_string(sg_backend b) {
    switch (b) {
        case SG_BACKEND_GLCORE33:           return "SG_BACKEND_GLCORE33";
        case SG_BACKEND_GLES2:              return "SG_BACKEND_GLES2";
        case SG_BACKEND_GLES3:              return "SG_BACKEND_GLES3";
        case SG_BACKEND_D3D11:              return "SG_BACKEND_D3D11";
        case SG_BACKEND_METAL_IOS:          return "SG_BACKEND_METAL_IOS";
        case SG_BACKEND_METAL_MACOS:        return "SG_BACKEND_METAL_MACOS";
        case SG_BACKEND_METAL_SIMULATOR:    return "SG_BACKEND_METAL_SIMULATOR";
        case SG_BACKEND_DUMMY:              return "SG_BACKEND_DUMMY";
        default: return "???";
    }
}

_SOKOL_PRIVATE const char* _sg_imgui_buffertype_string(sg_buffer_type t) {
    switch (t) {
        case SG_BUFFERTYPE_VERTEXBUFFER:    return "SG_BUFFERTYPE_VERTEXBUFFER";
        case SG_BUFFERTYPE_INDEXBUFFER:     return "SG_BUFFERTYPE_INDEXBUFFER";
        default:                            return "???";
    }
}

_SOKOL_PRIVATE const char* _sg_imgui_usage_string(sg_usage u) {
    switch (u) {
        case SG_USAGE_IMMUTABLE:    return "SG_USAGE_IMMUTABLE";
        case SG_USAGE_DYNAMIC:      return "SG_USAGE_DYNAMIC";
        case SG_USAGE_STREAM:       return "SG_USAGE_STREAM";
        default:                    return "???";
    }
}

_SOKOL_PRIVATE const char* _sg_imgui_imagetype_string(sg_image_type t) {
    switch (t) {
        case SG_IMAGETYPE_2D:       return "SG_IMAGETYPE_2D";
        case SG_IMAGETYPE_CUBE:     return "SG_IMAGETYPE_CUBE";
        case SG_IMAGETYPE_3D:       return "SG_IMAGETYPE_3D";
        case SG_IMAGETYPE_ARRAY:    return "SG_IMAGETYPE_ARRAY";
        default:                    return "???";
    }
}

_SOKOL_PRIVATE const char* _sg_imgui_pixelformat_string(sg_pixel_format fmt) {
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
        default: return "???";
    }
}

_SOKOL_PRIVATE const char* _sg_imgui_filter_string(sg_filter f) {
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

_SOKOL_PRIVATE const char* _sg_imgui_wrap_string(sg_wrap w) {
    switch (w) {
        case SG_WRAP_REPEAT:            return "SG_WRAP_REPEAT";
        case SG_WRAP_CLAMP_TO_EDGE:     return "SG_WRAP_CLAMP_TO_EDGE";
        case SG_WRAP_CLAMP_TO_BORDER:   return "SG_WRAP_CLAMP_TO_BORDER";
        case SG_WRAP_MIRRORED_REPEAT:   return "SG_WRAP_MIRRORED_REPEAT";
        default:                        return "???";
    }
}

_SOKOL_PRIVATE const char* _sg_imgui_bordercolor_string(sg_border_color bc) {
    switch (bc) {
        case SG_BORDERCOLOR_TRANSPARENT_BLACK:  return "SG_BORDERCOLOR_TRANSPARENT_BLACK";
        case SG_BORDERCOLOR_OPAQUE_BLACK:       return "SG_BORDERCOLOR_OPAQUE_BLACK";
        case SG_BORDERCOLOR_OPAQUE_WHITE:       return "SG_BORDERCOLOR_OPAQUE_WHITE";
        default:                                return "???";
    }
}

_SOKOL_PRIVATE const char* _sg_imgui_uniformtype_string(sg_uniform_type t) {
    switch (t) {
        case SG_UNIFORMTYPE_FLOAT:  return "SG_UNIFORMTYPE_FLOAT";
        case SG_UNIFORMTYPE_FLOAT2: return "SG_UNIFORMTYPE_FLOAT2";
        case SG_UNIFORMTYPE_FLOAT3: return "SG_UNIFORMTYPE_FLOAT3";
        case SG_UNIFORMTYPE_FLOAT4: return "SG_UNIFORMTYPE_FLOAT4";
        case SG_UNIFORMTYPE_MAT4:   return "SG_UNIFORMTYPE_MAT4";
        default:                    return "???";
    }
}

_SOKOL_PRIVATE const char* _sg_imgui_vertexstep_string(sg_vertex_step s) {
    switch (s) {
        case SG_VERTEXSTEP_PER_VERTEX:      return "SG_VERTEXSTEP_PER_VERTEX";
        case SG_VERTEXSTEP_PER_INSTANCE:    return "SG_VERTEXSTEP_PER_INSTANCE";
        default:                            return "???";
    }
}

_SOKOL_PRIVATE const char* _sg_imgui_vertexformat_string(sg_vertex_format f) {
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

_SOKOL_PRIVATE const char* _sg_imgui_primitivetype_string(sg_primitive_type t) {
    switch (t) {
        case SG_PRIMITIVETYPE_POINTS:           return "SG_PRIMITIVETYPE_POINTS";
        case SG_PRIMITIVETYPE_LINES:            return "SG_PRIMITIVETYPE_LINES";
        case SG_PRIMITIVETYPE_LINE_STRIP:       return "SG_PRIMITIVETYPE_LINE_STRIP";
        case SG_PRIMITIVETYPE_TRIANGLES:        return "SG_PRIMITIVETYPE_TRIANGLES";
        case SG_PRIMITIVETYPE_TRIANGLE_STRIP:   return "SG_PRIMITIVETYPE_TRIANGLE_STRIP";
        default:                                return "???";
    }
}

_SOKOL_PRIVATE const char* _sg_imgui_indextype_string(sg_index_type t) {
    switch (t) {
        case SG_INDEXTYPE_NONE:     return "SG_INDEXTYPE_NONE";
        case SG_INDEXTYPE_UINT16:   return "SG_INDEXTYPE_UINT16";
        case SG_INDEXTYPE_UINT32:   return "SG_INDEXTYPE_UINT32";
        default:                    return "???";
    }
}

_SOKOL_PRIVATE const char* _sg_imgui_stencilop_string(sg_stencil_op op) {
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

_SOKOL_PRIVATE const char* _sg_imgui_comparefunc_string(sg_compare_func f) {
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

_SOKOL_PRIVATE const char* _sg_imgui_blendfactor_string(sg_blend_factor f) {
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

_SOKOL_PRIVATE const char* _sg_imgui_blendop_string(sg_blend_op op) {
    switch (op) {
        case SG_BLENDOP_ADD:                return "SG_BLENDOP_ADD";
        case SG_BLENDOP_SUBTRACT:           return "SG_BLENDOP_SUBTRACT";
        case SG_BLENDOP_REVERSE_SUBTRACT:   return "SG_BLENDOP_REVERSE_SUBTRACT";
        default:                            return "???";
    }
}

_SOKOL_PRIVATE const char* _sg_imgui_colormask_string(uint8_t m) {
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

_SOKOL_PRIVATE const char* _sg_imgui_cullmode_string(sg_cull_mode cm) {
    switch (cm) {
        case SG_CULLMODE_NONE:  return "SG_CULLMODE_NONE";
        case SG_CULLMODE_FRONT: return "SG_CULLMODE_FRONT";
        case SG_CULLMODE_BACK:  return "SG_CULLMODE_BACK";
        default:                return "???";
    }
}

_SOKOL_PRIVATE const char* _sg_imgui_facewinding_string(sg_face_winding fw) {
    switch (fw) {
        case SG_FACEWINDING_CCW:    return "SG_FACEWINDING_CCW";
        case SG_FACEWINDING_CW:     return "SG_FACEWINDING_CW";
        default:                    return "???";
    }
}

_SOKOL_PRIVATE const char* _sg_imgui_shaderstage_string(sg_shader_stage stage) {
    switch (stage) {
        case SG_SHADERSTAGE_VS:     return "SG_SHADERSTAGE_VS";
        case SG_SHADERSTAGE_FS:     return "SG_SHADERSTAGE_FS";
        default:                    return "???";
    }
}

_SOKOL_PRIVATE const char* _sg_imgui_bool_string(bool b) {
    return b ? "true" : "false";
}

_SOKOL_PRIVATE sg_imgui_str_t _sg_imgui_res_id_string(uint32_t res_id, const char* label) {
    SOKOL_ASSERT(label);
    sg_imgui_str_t res;
    if (label[0]) {
        _sg_imgui_snprintf(&res, "'%s'", label);
    }
    else {
        _sg_imgui_snprintf(&res, "0x%08X", res_id);
    }
    return res;
}

_SOKOL_PRIVATE sg_imgui_str_t _sg_imgui_buffer_id_string(sg_imgui_t* ctx, sg_buffer buf_id) {
    if (buf_id.id != SG_INVALID_ID) {
        const sg_imgui_buffer_t* buf_ui = &ctx->buffers.slots[_sg_imgui_slot_index(buf_id.id)];
        return _sg_imgui_res_id_string(buf_id.id, buf_ui->label.buf);
    }
    else {
        return _sg_imgui_make_str("<invalid>");
    }
}

_SOKOL_PRIVATE sg_imgui_str_t _sg_imgui_image_id_string(sg_imgui_t* ctx, sg_image img_id) {
    if (img_id.id != SG_INVALID_ID) {
        const sg_imgui_image_t* img_ui = &ctx->images.slots[_sg_imgui_slot_index(img_id.id)];
        return _sg_imgui_res_id_string(img_id.id, img_ui->label.buf);
    }
    else {
        return _sg_imgui_make_str("<invalid>");
    }
}

_SOKOL_PRIVATE sg_imgui_str_t _sg_imgui_shader_id_string(sg_imgui_t* ctx, sg_shader shd_id) {
    if (shd_id.id != SG_INVALID_ID) {
        const sg_imgui_shader_t* shd_ui = &ctx->shaders.slots[_sg_imgui_slot_index(shd_id.id)];
        return _sg_imgui_res_id_string(shd_id.id, shd_ui->label.buf);
    }
    else {
        return _sg_imgui_make_str("<invalid>");
    }
}

_SOKOL_PRIVATE sg_imgui_str_t _sg_imgui_pipeline_id_string(sg_imgui_t* ctx, sg_pipeline pip_id) {
    if (pip_id.id != SG_INVALID_ID) {
        const sg_imgui_pipeline_t* pip_ui = &ctx->pipelines.slots[_sg_imgui_slot_index(pip_id.id)];
        return _sg_imgui_res_id_string(pip_id.id, pip_ui->label.buf);
    }
    else {
        return _sg_imgui_make_str("<invalid>");
    }
}

_SOKOL_PRIVATE sg_imgui_str_t _sg_imgui_pass_id_string(sg_imgui_t* ctx, sg_pass pass_id) {
    if (pass_id.id != SG_INVALID_ID) {
        const sg_imgui_pass_t* pass_ui = &ctx->passes.slots[_sg_imgui_slot_index(pass_id.id)];
        return _sg_imgui_res_id_string(pass_id.id, pass_ui->label.buf);
    }
    else {
        return _sg_imgui_make_str("<invalid>");
    }
}

/*--- RESOURCE HELPERS -------------------------------------------------------*/
_SOKOL_PRIVATE void _sg_imgui_buffer_created(sg_imgui_t* ctx, sg_buffer res_id, int slot_index, const sg_buffer_desc* desc) {
    SOKOL_ASSERT((slot_index > 0) && (slot_index < ctx->buffers.num_slots));
    sg_imgui_buffer_t* buf = &ctx->buffers.slots[slot_index];
    buf->res_id = res_id;
    buf->desc = *desc;
    buf->label = _sg_imgui_make_str(desc->label);
}

_SOKOL_PRIVATE void _sg_imgui_buffer_destroyed(sg_imgui_t* ctx, int slot_index) {
    SOKOL_ASSERT((slot_index > 0) && (slot_index < ctx->buffers.num_slots));
    sg_imgui_buffer_t* buf = &ctx->buffers.slots[slot_index];
    buf->res_id.id = SG_INVALID_ID;
}

_SOKOL_PRIVATE void _sg_imgui_image_created(sg_imgui_t* ctx, sg_image res_id, int slot_index, const sg_image_desc* desc) {
    SOKOL_ASSERT((slot_index > 0) && (slot_index < ctx->images.num_slots));
    sg_imgui_image_t* img = &ctx->images.slots[slot_index];
    img->res_id = res_id;
    img->desc = *desc;
    img->ui_scale = 1.0f;
    img->label = _sg_imgui_make_str(desc->label);
}

_SOKOL_PRIVATE void _sg_imgui_image_destroyed(sg_imgui_t* ctx, int slot_index) {
    SOKOL_ASSERT((slot_index > 0) && (slot_index < ctx->images.num_slots));
    sg_imgui_image_t* img = &ctx->images.slots[slot_index];
    img->res_id.id = SG_INVALID_ID;
}

_SOKOL_PRIVATE void _sg_imgui_shader_created(sg_imgui_t* ctx, sg_shader res_id, int slot_index, const sg_shader_desc* desc) {
    SOKOL_ASSERT((slot_index > 0) && (slot_index < ctx->shaders.num_slots));
    sg_imgui_shader_t* shd = &ctx->shaders.slots[slot_index];
    shd->res_id = res_id;
    shd->desc = *desc;
    shd->label = _sg_imgui_make_str(desc->label);
    if (shd->desc.vs.entry) {
        shd->vs_entry = _sg_imgui_make_str(shd->desc.vs.entry);
        shd->desc.vs.entry = shd->vs_entry.buf;
    }
    if (shd->desc.fs.entry) {
        shd->fs_entry = _sg_imgui_make_str(shd->desc.fs.entry);
        shd->desc.fs.entry = shd->fs_entry.buf;
    }
    for (int i = 0; i < SG_MAX_SHADERSTAGE_UBS; i++) {
        for (int j = 0; j < SG_MAX_UB_MEMBERS; j++) {
            sg_shader_uniform_desc* ud = &shd->desc.vs.uniform_blocks[i].uniforms[j];
            if (ud->name) {
                shd->vs_uniform_name[i][j] = _sg_imgui_make_str(ud->name);
                ud->name = shd->vs_uniform_name[i][j].buf;
            }
        }
    }
    for (int i = 0; i < SG_MAX_SHADERSTAGE_UBS; i++) {
        for (int j = 0; j < SG_MAX_UB_MEMBERS; j++) {
            sg_shader_uniform_desc* ud = &shd->desc.fs.uniform_blocks[i].uniforms[j];
            if (ud->name) {
                shd->fs_uniform_name[i][j] = _sg_imgui_make_str(ud->name);
                ud->name = shd->fs_uniform_name[i][j].buf;
            }
        }
    }
    for (int i = 0; i < SG_MAX_SHADERSTAGE_IMAGES; i++) {
        if (shd->desc.vs.images[i].name) {
            shd->vs_image_name[i] = _sg_imgui_make_str(shd->desc.vs.images[i].name);
            shd->desc.vs.images[i].name = shd->vs_image_name[i].buf;
        }
    }
    for (int i = 0; i < SG_MAX_SHADERSTAGE_IMAGES; i++) {
        if (shd->desc.fs.images[i].name) {
            shd->fs_image_name[i] = _sg_imgui_make_str(shd->desc.fs.images[i].name);
            shd->desc.fs.images[i].name = shd->fs_image_name[i].buf;
        }
    }
    if (shd->desc.vs.source) {
        shd->desc.vs.source = _sg_imgui_str_dup(shd->desc.vs.source);
    }
    if (shd->desc.vs.byte_code) {
        shd->desc.vs.byte_code = _sg_imgui_bin_dup(shd->desc.vs.byte_code, shd->desc.vs.byte_code_size);
    }
    if (shd->desc.fs.source) {
        shd->desc.fs.source = _sg_imgui_str_dup(shd->desc.fs.source);
    }
    if (shd->desc.fs.byte_code) {
        shd->desc.fs.byte_code = _sg_imgui_bin_dup(shd->desc.fs.byte_code, shd->desc.fs.byte_code_size);
    }
    for (int i = 0; i < SG_MAX_VERTEX_ATTRIBUTES; i++) {
        sg_shader_attr_desc* ad = &shd->desc.attrs[i];
        if (ad->name) {
            shd->attr_name[i] = _sg_imgui_make_str(ad->name);
            ad->name = shd->attr_name[i].buf;
        }
        if (ad->sem_name) {
            shd->attr_sem_name[i] = _sg_imgui_make_str(ad->sem_name);
            ad->sem_name = shd->attr_sem_name[i].buf;
        }
    }
}

_SOKOL_PRIVATE void _sg_imgui_shader_destroyed(sg_imgui_t* ctx, int slot_index) {
    SOKOL_ASSERT((slot_index > 0) && (slot_index < ctx->shaders.num_slots));
    sg_imgui_shader_t* shd = &ctx->shaders.slots[slot_index];
    shd->res_id.id = SG_INVALID_ID;
    if (shd->desc.vs.source) {
        _sg_imgui_free((void*)shd->desc.vs.source);
        shd->desc.vs.source = 0;
    }
    if (shd->desc.vs.byte_code) {
        _sg_imgui_free((void*)shd->desc.vs.byte_code);
        shd->desc.vs.byte_code = 0;
    }
    if (shd->desc.fs.source) {
        _sg_imgui_free((void*)shd->desc.fs.source);
        shd->desc.fs.source = 0;
    }
    if (shd->desc.fs.byte_code) {
        _sg_imgui_free((void*)shd->desc.fs.byte_code);
        shd->desc.fs.byte_code = 0;
    }
}

_SOKOL_PRIVATE void _sg_imgui_pipeline_created(sg_imgui_t* ctx, sg_pipeline res_id, int slot_index, const sg_pipeline_desc* desc) {
    SOKOL_ASSERT((slot_index > 0) && (slot_index < ctx->pipelines.num_slots));
    sg_imgui_pipeline_t* pip = &ctx->pipelines.slots[slot_index];
    pip->res_id = res_id;
    pip->label = _sg_imgui_make_str(desc->label);
    pip->desc = *desc;

}

_SOKOL_PRIVATE void _sg_imgui_pipeline_destroyed(sg_imgui_t* ctx, int slot_index) {
    SOKOL_ASSERT((slot_index > 0) && (slot_index < ctx->pipelines.num_slots));
    sg_imgui_pipeline_t* pip = &ctx->pipelines.slots[slot_index];
    pip->res_id.id = SG_INVALID_ID;
}

_SOKOL_PRIVATE void _sg_imgui_pass_created(sg_imgui_t* ctx, sg_pass res_id, int slot_index, const sg_pass_desc* desc) {
    SOKOL_ASSERT((slot_index > 0) && (slot_index < ctx->passes.num_slots));
    sg_imgui_pass_t* pass = &ctx->passes.slots[slot_index];
    pass->res_id = res_id;
    for (int i = 0; i < SG_MAX_COLOR_ATTACHMENTS; i++) {
        pass->color_image_scale[i] = 0.25f;
    }
    pass->ds_image_scale = 0.25f;
    pass->label = _sg_imgui_make_str(desc->label);
    pass->desc = *desc;
}

_SOKOL_PRIVATE void _sg_imgui_pass_destroyed(sg_imgui_t* ctx, int slot_index) {
    SOKOL_ASSERT((slot_index > 0) && (slot_index < ctx->passes.num_slots));
    sg_imgui_pass_t* pass = &ctx->passes.slots[slot_index];
    pass->res_id.id = SG_INVALID_ID;
}

/*--- COMMAND CAPTURING ------------------------------------------------------*/
_SOKOL_PRIVATE void _sg_imgui_capture_init(sg_imgui_t* ctx) {
    const int ubuf_initial_size = 256 * 1024;
    for (int i = 0; i < 2; i++) {
        sg_imgui_capture_bucket_t* bucket = &ctx->capture.bucket[i];
        bucket->ubuf_size = ubuf_initial_size;
        bucket->ubuf = (uint8_t*) _sg_imgui_alloc(bucket->ubuf_size);
        SOKOL_ASSERT(bucket->ubuf);
    }
}

_SOKOL_PRIVATE void _sg_imgui_capture_discard(sg_imgui_t* ctx) {
    for (int i = 0; i < 2; i++) {
        sg_imgui_capture_bucket_t* bucket = &ctx->capture.bucket[i];
        SOKOL_ASSERT(bucket->ubuf);
        _sg_imgui_free(bucket->ubuf);
        bucket->ubuf = 0;
    }
}

_SOKOL_PRIVATE sg_imgui_capture_bucket_t* _sg_imgui_capture_get_write_bucket(sg_imgui_t* ctx) {
    return &ctx->capture.bucket[ctx->capture.bucket_index & 1];
}

_SOKOL_PRIVATE sg_imgui_capture_bucket_t* _sg_imgui_capture_get_read_bucket(sg_imgui_t* ctx) {
    return &ctx->capture.bucket[(ctx->capture.bucket_index + 1) & 1];
}

_SOKOL_PRIVATE void _sg_imgui_capture_next_frame(sg_imgui_t* ctx) {
    ctx->capture.bucket_index = (ctx->capture.bucket_index + 1) & 1;
    sg_imgui_capture_bucket_t* bucket = &ctx->capture.bucket[ctx->capture.bucket_index];
    bucket->num_items = 0;
    bucket->ubuf_pos = 0;
}

_SOKOL_PRIVATE void _sg_imgui_capture_grow_ubuf(sg_imgui_t* ctx, uint32_t required_size) {
    sg_imgui_capture_bucket_t* bucket = _sg_imgui_capture_get_write_bucket(ctx);
    SOKOL_ASSERT(required_size > bucket->ubuf_size);
    int old_size = bucket->ubuf_size;
    int new_size = required_size + (required_size>>1);  /* allocate a bit ahead */
    bucket->ubuf_size = new_size;
    bucket->ubuf = (uint8_t*) _sg_imgui_realloc(bucket->ubuf, old_size, new_size);
}

_SOKOL_PRIVATE sg_imgui_capture_item_t* _sg_imgui_capture_next_write_item(sg_imgui_t* ctx) {
    sg_imgui_capture_bucket_t* bucket = _sg_imgui_capture_get_write_bucket(ctx);
    if (bucket->num_items < SG_IMGUI_MAX_FRAMECAPTURE_ITEMS) {
        sg_imgui_capture_item_t* item = &bucket->items[bucket->num_items++];
        return item;
    }
    else {
        return 0;
    }
}

_SOKOL_PRIVATE uint32_t _sg_imgui_capture_num_read_items(sg_imgui_t* ctx) {
    sg_imgui_capture_bucket_t* bucket = _sg_imgui_capture_get_read_bucket(ctx);
    return bucket->num_items;
}

_SOKOL_PRIVATE sg_imgui_capture_item_t* _sg_imgui_capture_read_item_at(sg_imgui_t* ctx, uint32_t index) {
    sg_imgui_capture_bucket_t* bucket = _sg_imgui_capture_get_read_bucket(ctx);
    SOKOL_ASSERT(index < bucket->num_items);
    return &bucket->items[index];
}

_SOKOL_PRIVATE uint32_t _sg_imgui_capture_uniforms(sg_imgui_t* ctx, const void* data, int num_bytes) {
    sg_imgui_capture_bucket_t* bucket = _sg_imgui_capture_get_write_bucket(ctx);
    const uint32_t required_size = bucket->ubuf_pos + num_bytes;
    if (required_size > bucket->ubuf_size) {
        _sg_imgui_capture_grow_ubuf(ctx, required_size);
    }
    SOKOL_ASSERT(required_size <= bucket->ubuf_size);
    memcpy(bucket->ubuf + bucket->ubuf_pos, data, num_bytes);
    const uint32_t pos = bucket->ubuf_pos;
    bucket->ubuf_pos += num_bytes;
    SOKOL_ASSERT(bucket->ubuf_pos <= bucket->ubuf_size);
    return pos;
}

_SOKOL_PRIVATE sg_imgui_str_t _sg_imgui_capture_item_string(sg_imgui_t* ctx, int index, const sg_imgui_capture_item_t* item) {
    sg_imgui_str_t str = _sg_imgui_make_str(0);
    sg_imgui_str_t res_id = _sg_imgui_make_str(0);
    switch (item->cmd) {
        case SG_IMGUI_CMD_RESET_STATE_CACHE:
            _sg_imgui_snprintf(&str, "%d: sg_reset_state_cache()", index);
            break;

        case SG_IMGUI_CMD_MAKE_BUFFER:
            res_id = _sg_imgui_buffer_id_string(ctx, item->args.make_buffer.result);
            _sg_imgui_snprintf(&str, "%d: sg_make_buffer(desc=..) => %s", index, res_id.buf);
            break;

        case SG_IMGUI_CMD_MAKE_IMAGE:
            res_id = _sg_imgui_image_id_string(ctx, item->args.make_image.result);
            _sg_imgui_snprintf(&str, "%d: sg_make_image(desc=..) => %s", index, res_id.buf);
            break;

        case SG_IMGUI_CMD_MAKE_SHADER:
            res_id = _sg_imgui_shader_id_string(ctx, item->args.make_shader.result);
            _sg_imgui_snprintf(&str, "%d: sg_make_shader(desc=..) => %s", index, res_id.buf);
            break;

        case SG_IMGUI_CMD_MAKE_PIPELINE:
            res_id = _sg_imgui_pipeline_id_string(ctx, item->args.make_pipeline.result);
            _sg_imgui_snprintf(&str, "%d: sg_make_pipeline(desc=..) => %s", index, res_id.buf);
            break;

        case SG_IMGUI_CMD_MAKE_PASS:
            res_id = _sg_imgui_pass_id_string(ctx, item->args.make_pass.result);
            _sg_imgui_snprintf(&str, "%d: sg_make_pass(desc=..) => %s", index, res_id.buf);
            break;

        case SG_IMGUI_CMD_DESTROY_BUFFER:
            res_id = _sg_imgui_buffer_id_string(ctx, item->args.destroy_buffer.buffer);
            _sg_imgui_snprintf(&str, "%d: sg_destroy_buffer(buf=%s)", index, res_id.buf);
            break;

        case SG_IMGUI_CMD_DESTROY_IMAGE:
            res_id = _sg_imgui_image_id_string(ctx, item->args.destroy_image.image);
            _sg_imgui_snprintf(&str, "%d: sg_destroy_image(img=%s)", index, res_id.buf);
            break;

        case SG_IMGUI_CMD_DESTROY_SHADER:
            res_id = _sg_imgui_shader_id_string(ctx, item->args.destroy_shader.shader);
            _sg_imgui_snprintf(&str, "%d: sg_destroy_shader(shd=%s)", index, res_id.buf);
            break;

        case SG_IMGUI_CMD_DESTROY_PIPELINE:
            res_id = _sg_imgui_pipeline_id_string(ctx, item->args.destroy_pipeline.pipeline);
            _sg_imgui_snprintf(&str, "%d: sg_destroy_pipeline(pip=%s)", index, res_id.buf);
            break;

        case SG_IMGUI_CMD_DESTROY_PASS:
            res_id = _sg_imgui_pass_id_string(ctx, item->args.destroy_pass.pass);
            _sg_imgui_snprintf(&str, "%d: sg_destroy_pass(pass=%s)", index, res_id.buf);
            break;

        case SG_IMGUI_CMD_UPDATE_BUFFER:
            res_id = _sg_imgui_buffer_id_string(ctx, item->args.update_buffer.buffer);
            _sg_imgui_snprintf(&str, "%d: sg_update_buffer(buf=%s, data_ptr=.., data_size=%d)",
                index, res_id.buf,
                item->args.update_buffer.data_size);
            break;

        case SG_IMGUI_CMD_UPDATE_IMAGE:
            res_id = _sg_imgui_image_id_string(ctx, item->args.update_image.image);
            _sg_imgui_snprintf(&str, "%d: sg_update_image(img=%s, data=..)", index, res_id.buf);
            break;

        case SG_IMGUI_CMD_APPEND_BUFFER:
            res_id = _sg_imgui_buffer_id_string(ctx, item->args.append_buffer.buffer);
            _sg_imgui_snprintf(&str, "%d: sg_append_buffer(buf=%s, data_ptr=.., data_size=%d) => %d",
                index, res_id.buf,
                item->args.append_buffer.data_size,
                item->args.append_buffer.result);
            break;

        case SG_IMGUI_CMD_BEGIN_DEFAULT_PASS:
            _sg_imgui_snprintf(&str, "%d: sg_begin_default_pass(pass_action=.., width=%d, height=%d)",
                index,
                item->args.begin_default_pass.width,
                item->args.begin_default_pass.height);
            break;

        case SG_IMGUI_CMD_BEGIN_PASS:
            res_id = _sg_imgui_pass_id_string(ctx, item->args.begin_pass.pass);
            _sg_imgui_snprintf(&str, "%d: sg_begin_pass(pass=%s, pass_action=..)", index, res_id.buf);
            break;

        case SG_IMGUI_CMD_APPLY_VIEWPORT:
            _sg_imgui_snprintf(&str, "%d: sg_apply_viewport(x=%d, y=%d, width=%d, height=%d, origin_top_left=%s)",
                index,
                item->args.apply_viewport.x,
                item->args.apply_viewport.y,
                item->args.apply_viewport.width,
                item->args.apply_viewport.height,
                _sg_imgui_bool_string(item->args.apply_viewport.origin_top_left));
            break;

        case SG_IMGUI_CMD_APPLY_SCISSOR_RECT:
            _sg_imgui_snprintf(&str, "%d: sg_apply_scissor_rect(x=%d, y=%d, width=%d, height=%d, origin_top_left=%s)",
                index,
                item->args.apply_scissor_rect.x,
                item->args.apply_scissor_rect.y,
                item->args.apply_scissor_rect.width,
                item->args.apply_scissor_rect.height,
                _sg_imgui_bool_string(item->args.apply_scissor_rect.origin_top_left));
            break;

        case SG_IMGUI_CMD_APPLY_PIPELINE:
            res_id = _sg_imgui_pipeline_id_string(ctx, item->args.apply_pipeline.pipeline);
            _sg_imgui_snprintf(&str, "%d: sg_apply_pipeline(pip=%s)", index, res_id.buf);
            break;

        case SG_IMGUI_CMD_APPLY_BINDINGS:
            _sg_imgui_snprintf(&str, "%d: sg_apply_bindings(bindings=..)", index);
            break;

        case SG_IMGUI_CMD_APPLY_UNIFORMS:
            _sg_imgui_snprintf(&str, "%d: sg_apply_uniforms(stage=%s, ub_index=%d, data=.., num_bytes=%d)",
                index,
                _sg_imgui_shaderstage_string(item->args.apply_uniforms.stage),
                item->args.apply_uniforms.ub_index,
                item->args.apply_uniforms.num_bytes);
            break;

        case SG_IMGUI_CMD_DRAW:
            _sg_imgui_snprintf(&str, "%d: sg_draw(base_element=%d, num_elements=%d, num_instances=%d)",
                index,
                item->args.draw.base_element,
                item->args.draw.num_elements,
                item->args.draw.num_instances);
            break;

        case SG_IMGUI_CMD_END_PASS:
            _sg_imgui_snprintf(&str, "%d: sg_end_pass()", index);
            break;

        case SG_IMGUI_CMD_COMMIT:
            _sg_imgui_snprintf(&str, "%d: sg_commit()", index);
            break;

        case SG_IMGUI_CMD_ALLOC_BUFFER:
            res_id = _sg_imgui_buffer_id_string(ctx, item->args.alloc_buffer.result);
            _sg_imgui_snprintf(&str, "%d: sg_alloc_buffer() => %s", index, res_id.buf);
            break;

        case SG_IMGUI_CMD_ALLOC_IMAGE:
            res_id = _sg_imgui_image_id_string(ctx, item->args.alloc_image.result);
            _sg_imgui_snprintf(&str, "%d: sg_alloc_image() => %s", index, res_id.buf);
            break;

        case SG_IMGUI_CMD_ALLOC_SHADER:
            res_id = _sg_imgui_shader_id_string(ctx, item->args.alloc_shader.result);
            _sg_imgui_snprintf(&str, "%d: sg_alloc_shader() => %s", index, res_id.buf);
            break;

        case SG_IMGUI_CMD_ALLOC_PIPELINE:
            res_id = _sg_imgui_pipeline_id_string(ctx, item->args.alloc_pipeline.result);
            _sg_imgui_snprintf(&str, "%d: sg_alloc_pipeline() => %s", index, res_id.buf);
            break;

        case SG_IMGUI_CMD_ALLOC_PASS:
            res_id = _sg_imgui_pass_id_string(ctx, item->args.alloc_pass.result);
            _sg_imgui_snprintf(&str, "%d: sg_alloc_pass() => %s", index, res_id.buf);
            break;

        case SG_IMGUI_CMD_INIT_BUFFER:
            res_id = _sg_imgui_buffer_id_string(ctx, item->args.init_buffer.buffer);
            _sg_imgui_snprintf(&str, "%d: sg_init_buffer(buf=%s, desc=..)", index, res_id.buf);
            break;

        case SG_IMGUI_CMD_INIT_IMAGE:
            res_id = _sg_imgui_image_id_string(ctx, item->args.init_image.image);
            _sg_imgui_snprintf(&str, "%d: sg_init_image(img=%s, desc=..)", index, res_id.buf);
            break;

        case SG_IMGUI_CMD_INIT_SHADER:
            res_id = _sg_imgui_shader_id_string(ctx, item->args.init_shader.shader);
            _sg_imgui_snprintf(&str, "%d: sg_init_shader(shd=%s, desc=..)", index, res_id.buf);
            break;

        case SG_IMGUI_CMD_INIT_PIPELINE:
            res_id = _sg_imgui_pipeline_id_string(ctx, item->args.init_pipeline.pipeline);
            _sg_imgui_snprintf(&str, "%d: sg_init_pipeline(pip=%s, desc=..)", index, res_id.buf);
            break;

        case SG_IMGUI_CMD_INIT_PASS:
            res_id = _sg_imgui_pass_id_string(ctx, item->args.init_pass.pass);
            _sg_imgui_snprintf(&str, "%d: sg_init_pass(pass=%s, desc=..)", index, res_id.buf);
            break;

        case SG_IMGUI_CMD_FAIL_BUFFER:
            res_id = _sg_imgui_buffer_id_string(ctx, item->args.fail_buffer.buffer);
            _sg_imgui_snprintf(&str, "%d: sg_fail_buffer(buf=%s)", index, res_id.buf);
            break;

        case SG_IMGUI_CMD_FAIL_IMAGE:
            res_id = _sg_imgui_image_id_string(ctx, item->args.fail_image.image);
            _sg_imgui_snprintf(&str, "%d: sg_fail_image(img=%s)", index, res_id.buf);
            break;

        case SG_IMGUI_CMD_FAIL_SHADER:
            res_id = _sg_imgui_shader_id_string(ctx, item->args.fail_shader.shader);
            _sg_imgui_snprintf(&str, "%d: sg_fail_shader(shd=%s)", index, res_id.buf);
            break;

        case SG_IMGUI_CMD_FAIL_PIPELINE:
            res_id = _sg_imgui_pipeline_id_string(ctx, item->args.fail_pipeline.pipeline);
            _sg_imgui_snprintf(&str, "%d: sg_fail_pipeline(shd=%s)", index, res_id.buf);
            break;

        case SG_IMGUI_CMD_FAIL_PASS:
            res_id = _sg_imgui_pass_id_string(ctx, item->args.fail_pass.pass);
            _sg_imgui_snprintf(&str, "%d: sg_fail_pass(pass=%s)", index, res_id.buf);
            break;

        case SG_IMGUI_CMD_PUSH_DEBUG_GROUP:
            _sg_imgui_snprintf(&str, "%d: sg_push_debug_group(name=%s)", index,
                item->args.push_debug_group.name.buf);
            break;

        case SG_IMGUI_CMD_POP_DEBUG_GROUP:
            _sg_imgui_snprintf(&str, "%d: sg_pop_debug_group()", index);
            break;

        case SG_IMGUI_CMD_ERR_BUFFER_POOL_EXHAUSTED:
            _sg_imgui_snprintf(&str, "%d: sg_err_buffer_pool_exhausted()", index);
            break;

        case SG_IMGUI_CMD_ERR_IMAGE_POOL_EXHAUSTED:
            _sg_imgui_snprintf(&str, "%d: sg_err_image_pool_exhausted()", index);
            break;

        case SG_IMGUI_CMD_ERR_SHADER_POOL_EXHAUSTED:
            _sg_imgui_snprintf(&str, "%d: sg_err_shader_pool_exhausted()", index);
            break;

        case SG_IMGUI_CMD_ERR_PIPELINE_POOL_EXHAUSTED:
            _sg_imgui_snprintf(&str, "%d: sg_err_pipeline_pool_exhausted()", index);
            break;

        case SG_IMGUI_CMD_ERR_PASS_POOL_EXHAUSTED:
            _sg_imgui_snprintf(&str, "%d: sg_err_pass_pool_exhausted()", index);
            break;

        case SG_IMGUI_CMD_ERR_CONTEXT_MISMATCH:
            _sg_imgui_snprintf(&str, "%d: sg_err_context_mismatch()", index);
            break;

        case SG_IMGUI_CMD_ERR_PASS_INVALID:
            _sg_imgui_snprintf(&str, "%d: sg_err_pass_invalid()", index);
            break;

        case SG_IMGUI_CMD_ERR_DRAW_INVALID:
            _sg_imgui_snprintf(&str, "%d: sg_err_draw_invalid()", index);
            break;

        case SG_IMGUI_CMD_ERR_BINDINGS_INVALID:
            _sg_imgui_snprintf(&str, "%d: sg_err_bindings_invalid()", index);
            break;

        default:
            _sg_imgui_snprintf(&str, "%d: ???", index);
            break;
    }
    return str;
}

/*--- CAPTURE CALLBACKS ------------------------------------------------------*/
_SOKOL_PRIVATE void _sg_imgui_reset_state_cache(void* user_data) {
    sg_imgui_t* ctx = (sg_imgui_t*) user_data;
    SOKOL_ASSERT(ctx);
    sg_imgui_capture_item_t* item = _sg_imgui_capture_next_write_item(ctx);
    if (item) {
        item->cmd = SG_IMGUI_CMD_RESET_STATE_CACHE;
        item->color = _SG_IMGUI_COLOR_OTHER;
    }
    if (ctx->hooks.reset_state_cache) {
        ctx->hooks.reset_state_cache(ctx->hooks.user_data);
    }
}

_SOKOL_PRIVATE void _sg_imgui_make_buffer(const sg_buffer_desc* desc, sg_buffer buf_id, void* user_data) {
    sg_imgui_t* ctx = (sg_imgui_t*) user_data;
    SOKOL_ASSERT(ctx);
    sg_imgui_capture_item_t* item = _sg_imgui_capture_next_write_item(ctx);
    if (item) {
        item->cmd = SG_IMGUI_CMD_MAKE_BUFFER;
        item->color = _SG_IMGUI_COLOR_RSRC;
        item->args.make_buffer.result = buf_id;
    }
    if (ctx->hooks.make_buffer) {
        ctx->hooks.make_buffer(desc, buf_id, ctx->hooks.user_data);
    }
    if (buf_id.id != SG_INVALID_ID) {
        _sg_imgui_buffer_created(ctx, buf_id, _sg_imgui_slot_index(buf_id.id), desc);
    }
}

_SOKOL_PRIVATE void _sg_imgui_make_image(const sg_image_desc* desc, sg_image img_id, void* user_data) {
    sg_imgui_t* ctx = (sg_imgui_t*) user_data;
    SOKOL_ASSERT(ctx);
    sg_imgui_capture_item_t* item = _sg_imgui_capture_next_write_item(ctx);
    if (item) {
        item->cmd = SG_IMGUI_CMD_MAKE_IMAGE;
        item->color = _SG_IMGUI_COLOR_RSRC;
        item->args.make_image.result = img_id;
    }
    if (ctx->hooks.make_image) {
        ctx->hooks.make_image(desc, img_id, ctx->hooks.user_data);
    }
    if (img_id.id != SG_INVALID_ID) {
        _sg_imgui_image_created(ctx, img_id, _sg_imgui_slot_index(img_id.id), desc);
    }
}

_SOKOL_PRIVATE void _sg_imgui_make_shader(const sg_shader_desc* desc, sg_shader shd_id, void* user_data) {
    sg_imgui_t* ctx = (sg_imgui_t*) user_data;
    SOKOL_ASSERT(ctx);
    sg_imgui_capture_item_t* item = _sg_imgui_capture_next_write_item(ctx);
    if (item) {
        item->cmd = SG_IMGUI_CMD_MAKE_SHADER;
        item->color = _SG_IMGUI_COLOR_RSRC;
        item->args.make_shader.result = shd_id;
    }
    if (ctx->hooks.make_shader) {
        ctx->hooks.make_shader(desc, shd_id, ctx->hooks.user_data);
    }
    if (shd_id.id != SG_INVALID_ID) {
        _sg_imgui_shader_created(ctx, shd_id, _sg_imgui_slot_index(shd_id.id), desc);
    }
}

_SOKOL_PRIVATE void _sg_imgui_make_pipeline(const sg_pipeline_desc* desc, sg_pipeline pip_id, void* user_data) {
    sg_imgui_t* ctx = (sg_imgui_t*) user_data;
    SOKOL_ASSERT(ctx);
    sg_imgui_capture_item_t* item = _sg_imgui_capture_next_write_item(ctx);
    if (item) {
        item->cmd = SG_IMGUI_CMD_MAKE_PIPELINE;
        item->color = _SG_IMGUI_COLOR_RSRC;
        item->args.make_pipeline.result = pip_id;
    }
    if (ctx->hooks.make_pipeline) {
        ctx->hooks.make_pipeline(desc, pip_id, ctx->hooks.user_data);
    }
    if (pip_id.id != SG_INVALID_ID) {
        _sg_imgui_pipeline_created(ctx, pip_id, _sg_imgui_slot_index(pip_id.id), desc);
    }
}

_SOKOL_PRIVATE void _sg_imgui_make_pass(const sg_pass_desc* desc, sg_pass pass_id, void* user_data) {
    sg_imgui_t* ctx = (sg_imgui_t*) user_data;
    SOKOL_ASSERT(ctx);
    sg_imgui_capture_item_t* item = _sg_imgui_capture_next_write_item(ctx);
    if (item) {
        item->cmd = SG_IMGUI_CMD_MAKE_PASS;
        item->color = _SG_IMGUI_COLOR_RSRC;
        item->args.make_pass.result = pass_id;
    }
    if (ctx->hooks.make_pass) {
        ctx->hooks.make_pass(desc, pass_id, ctx->hooks.user_data);
    }
    if (pass_id.id != SG_INVALID_ID) {
        _sg_imgui_pass_created(ctx, pass_id, _sg_imgui_slot_index(pass_id.id), desc);
    }
}

_SOKOL_PRIVATE void _sg_imgui_destroy_buffer(sg_buffer buf, void* user_data) {
    sg_imgui_t* ctx = (sg_imgui_t*) user_data;
    SOKOL_ASSERT(ctx);
    sg_imgui_capture_item_t* item = _sg_imgui_capture_next_write_item(ctx);
    if (item) {
        item->cmd = SG_IMGUI_CMD_DESTROY_BUFFER;
        item->color = _SG_IMGUI_COLOR_RSRC;
        item->args.destroy_buffer.buffer = buf;
    }
    if (ctx->hooks.destroy_buffer) {
        ctx->hooks.destroy_buffer(buf, ctx->hooks.user_data);
    }
    if (buf.id != SG_INVALID_ID) {
        _sg_imgui_buffer_destroyed(ctx, _sg_imgui_slot_index(buf.id));
    }
}

_SOKOL_PRIVATE void _sg_imgui_destroy_image(sg_image img, void* user_data) {
    sg_imgui_t* ctx = (sg_imgui_t*) user_data;
    SOKOL_ASSERT(ctx);
    sg_imgui_capture_item_t* item = _sg_imgui_capture_next_write_item(ctx);
    if (item) {
        item->cmd = SG_IMGUI_CMD_DESTROY_IMAGE;
        item->color = _SG_IMGUI_COLOR_RSRC;
        item->args.destroy_image.image = img;
    }
    if (ctx->hooks.destroy_image) {
        ctx->hooks.destroy_image(img, ctx->hooks.user_data);
    }
    if (img.id != SG_INVALID_ID) {
        _sg_imgui_image_destroyed(ctx, _sg_imgui_slot_index(img.id));
    }
}

_SOKOL_PRIVATE void _sg_imgui_destroy_shader(sg_shader shd, void* user_data) {
    sg_imgui_t* ctx = (sg_imgui_t*) user_data;
    SOKOL_ASSERT(ctx);
    sg_imgui_capture_item_t* item = _sg_imgui_capture_next_write_item(ctx);
    if (item) {
        item->cmd = SG_IMGUI_CMD_DESTROY_SHADER;
        item->color = _SG_IMGUI_COLOR_RSRC;
        item->args.destroy_shader.shader = shd;
    }
    if (ctx->hooks.destroy_shader) {
        ctx->hooks.destroy_shader(shd, ctx->hooks.user_data);
    }
    if (shd.id != SG_INVALID_ID) {
        _sg_imgui_shader_destroyed(ctx, _sg_imgui_slot_index(shd.id));
    }
}

_SOKOL_PRIVATE void _sg_imgui_destroy_pipeline(sg_pipeline pip, void* user_data) {
    sg_imgui_t* ctx = (sg_imgui_t*) user_data;
    SOKOL_ASSERT(ctx);
    sg_imgui_capture_item_t* item = _sg_imgui_capture_next_write_item(ctx);
    if (item) {
        item->cmd = SG_IMGUI_CMD_DESTROY_PIPELINE;
        item->color = _SG_IMGUI_COLOR_RSRC;
        item->args.destroy_pipeline.pipeline = pip;
    }
    if (ctx->hooks.destroy_pipeline) {
        ctx->hooks.destroy_pipeline(pip, ctx->hooks.user_data);
    }
    if (pip.id != SG_INVALID_ID) {
        _sg_imgui_pipeline_destroyed(ctx, _sg_imgui_slot_index(pip.id));
    }
}

_SOKOL_PRIVATE void _sg_imgui_destroy_pass(sg_pass pass, void* user_data) {
    sg_imgui_t* ctx = (sg_imgui_t*) user_data;
    SOKOL_ASSERT(ctx);
    sg_imgui_capture_item_t* item = _sg_imgui_capture_next_write_item(ctx);
    if (item) {
        item->cmd = SG_IMGUI_CMD_DESTROY_PASS;
        item->color = _SG_IMGUI_COLOR_RSRC;
        item->args.destroy_pass.pass = pass;
    }
    if (ctx->hooks.destroy_pass) {
        ctx->hooks.destroy_pass(pass, ctx->hooks.user_data);
    }
    if (pass.id != SG_INVALID_ID) {
        _sg_imgui_pass_destroyed(ctx, _sg_imgui_slot_index(pass.id));
    }
}

_SOKOL_PRIVATE void _sg_imgui_update_buffer(sg_buffer buf, const void* data_ptr, int data_size, void* user_data) {
    sg_imgui_t* ctx = (sg_imgui_t*) user_data;
    SOKOL_ASSERT(ctx);
    sg_imgui_capture_item_t* item = _sg_imgui_capture_next_write_item(ctx);
    if (item) {
        item->cmd = SG_IMGUI_CMD_UPDATE_BUFFER;
        item->color = _SG_IMGUI_COLOR_RSRC;
        item->args.update_buffer.buffer = buf;
        item->args.update_buffer.data_size = data_size;
    }
    if (ctx->hooks.update_buffer) {
        ctx->hooks.update_buffer(buf, data_ptr, data_size, ctx->hooks.user_data);
    }
}

_SOKOL_PRIVATE void _sg_imgui_update_image(sg_image img, const sg_image_content* data, void* user_data) {
    sg_imgui_t* ctx = (sg_imgui_t*) user_data;
    SOKOL_ASSERT(ctx);
    sg_imgui_capture_item_t* item = _sg_imgui_capture_next_write_item(ctx);
    if (item) {
        item->cmd = SG_IMGUI_CMD_UPDATE_IMAGE;
        item->color = _SG_IMGUI_COLOR_RSRC;
        item->args.update_image.image = img;
    }
    if (ctx->hooks.update_image) {
        ctx->hooks.update_image(img, data, ctx->hooks.user_data);
    }
}

_SOKOL_PRIVATE void _sg_imgui_append_buffer(sg_buffer buf, const void* data_ptr, int data_size, int result, void* user_data) {
    sg_imgui_t* ctx = (sg_imgui_t*) user_data;
    SOKOL_ASSERT(ctx);
    sg_imgui_capture_item_t* item = _sg_imgui_capture_next_write_item(ctx);
    if (item) {
        item->cmd = SG_IMGUI_CMD_APPEND_BUFFER;
        item->color = _SG_IMGUI_COLOR_RSRC;
        item->args.append_buffer.buffer = buf;
        item->args.append_buffer.data_size = data_size;
        item->args.append_buffer.result = result;
    }
    if (ctx->hooks.append_buffer) {
        ctx->hooks.append_buffer(buf, data_ptr, data_size, result, ctx->hooks.user_data);
    }
}

_SOKOL_PRIVATE void _sg_imgui_begin_default_pass(const sg_pass_action* pass_action, int width, int height, void* user_data) {
    sg_imgui_t* ctx = (sg_imgui_t*) user_data;
    SOKOL_ASSERT(ctx);
    sg_imgui_capture_item_t* item = _sg_imgui_capture_next_write_item(ctx);
    if (item) {
        SOKOL_ASSERT(pass_action);
        item->cmd = SG_IMGUI_CMD_BEGIN_DEFAULT_PASS;
        item->color = _SG_IMGUI_COLOR_DRAW;
        item->args.begin_default_pass.action = *pass_action;
        item->args.begin_default_pass.width = width;
        item->args.begin_default_pass.height = height;
    }
    if (ctx->hooks.begin_default_pass) {
        ctx->hooks.begin_default_pass(pass_action, width, height, ctx->hooks.user_data);
    }
}

_SOKOL_PRIVATE void _sg_imgui_begin_pass(sg_pass pass, const sg_pass_action* pass_action, void* user_data) {
    sg_imgui_t* ctx = (sg_imgui_t*) user_data;
    SOKOL_ASSERT(ctx);
    sg_imgui_capture_item_t* item = _sg_imgui_capture_next_write_item(ctx);
    if (item) {
        SOKOL_ASSERT(pass_action);
        item->cmd = SG_IMGUI_CMD_BEGIN_PASS;
        item->color = _SG_IMGUI_COLOR_DRAW;
        item->args.begin_pass.pass = pass;
        item->args.begin_pass.action = *pass_action;
    }
    if (ctx->hooks.begin_pass) {
        ctx->hooks.begin_pass(pass, pass_action, ctx->hooks.user_data);
    }
}

_SOKOL_PRIVATE void _sg_imgui_apply_viewport(int x, int y, int width, int height, bool origin_top_left, void* user_data) {
    sg_imgui_t* ctx = (sg_imgui_t*) user_data;
    SOKOL_ASSERT(ctx);
    sg_imgui_capture_item_t* item = _sg_imgui_capture_next_write_item(ctx);
    if (item) {
        item->cmd = SG_IMGUI_CMD_APPLY_VIEWPORT;
        item->color = _SG_IMGUI_COLOR_DRAW;
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

_SOKOL_PRIVATE void _sg_imgui_apply_scissor_rect(int x, int y, int width, int height, bool origin_top_left, void* user_data) {
    sg_imgui_t* ctx = (sg_imgui_t*) user_data;
    SOKOL_ASSERT(ctx);
    sg_imgui_capture_item_t* item = _sg_imgui_capture_next_write_item(ctx);
    if (item) {
        item->cmd = SG_IMGUI_CMD_APPLY_SCISSOR_RECT;
        item->color = _SG_IMGUI_COLOR_DRAW;
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

_SOKOL_PRIVATE void _sg_imgui_apply_pipeline(sg_pipeline pip, void* user_data) {
    sg_imgui_t* ctx = (sg_imgui_t*) user_data;
    SOKOL_ASSERT(ctx);
    ctx->cur_pipeline = pip;    /* stored for _sg_imgui_apply_uniforms */
    sg_imgui_capture_item_t* item = _sg_imgui_capture_next_write_item(ctx);
    if (item) {
        item->cmd = SG_IMGUI_CMD_APPLY_PIPELINE;
        item->color = _SG_IMGUI_COLOR_DRAW;
        item->args.apply_pipeline.pipeline = pip;
    }
    if (ctx->hooks.apply_pipeline) {
        ctx->hooks.apply_pipeline(pip, ctx->hooks.user_data);
    }
}

_SOKOL_PRIVATE void _sg_imgui_apply_bindings(const sg_bindings* bindings, void* user_data) {
    sg_imgui_t* ctx = (sg_imgui_t*) user_data;
    SOKOL_ASSERT(ctx);
    sg_imgui_capture_item_t* item = _sg_imgui_capture_next_write_item(ctx);
    if (item) {
        SOKOL_ASSERT(bindings);
        item->cmd = SG_IMGUI_CMD_APPLY_BINDINGS;
        item->color = _SG_IMGUI_COLOR_DRAW;
        item->args.apply_bindings.bindings = *bindings;
    }
    if (ctx->hooks.apply_bindings) {
        ctx->hooks.apply_bindings(bindings, ctx->hooks.user_data);
    }
}

_SOKOL_PRIVATE void _sg_imgui_apply_uniforms(sg_shader_stage stage, int ub_index, const void* data, int num_bytes, void* user_data) {
    sg_imgui_t* ctx = (sg_imgui_t*) user_data;
    SOKOL_ASSERT(ctx);
    sg_imgui_capture_item_t* item = _sg_imgui_capture_next_write_item(ctx);
    if (item) {
        item->cmd = SG_IMGUI_CMD_APPLY_UNIFORMS;
        item->color = _SG_IMGUI_COLOR_DRAW;
        sg_imgui_args_apply_uniforms_t* args = &item->args.apply_uniforms;
        args->stage = stage;
        args->ub_index = ub_index;
        args->data = data;
        args->num_bytes = num_bytes;
        args->pipeline = ctx->cur_pipeline;
        args->ubuf_pos = _sg_imgui_capture_uniforms(ctx, data, num_bytes);
    }
    if (ctx->hooks.apply_uniforms) {
        ctx->hooks.apply_uniforms(stage, ub_index, data, num_bytes, ctx->hooks.user_data);
    }
}

_SOKOL_PRIVATE void _sg_imgui_draw(int base_element, int num_elements, int num_instances, void* user_data) {
    sg_imgui_t* ctx = (sg_imgui_t*) user_data;
    SOKOL_ASSERT(ctx);
    sg_imgui_capture_item_t* item = _sg_imgui_capture_next_write_item(ctx);
    if (item) {
        item->cmd = SG_IMGUI_CMD_DRAW;
        item->color = _SG_IMGUI_COLOR_DRAW;
        item->args.draw.base_element = base_element;
        item->args.draw.num_elements = num_elements;
        item->args.draw.num_instances = num_instances;
    }
    if (ctx->hooks.draw) {
        ctx->hooks.draw(base_element, num_elements, num_instances, ctx->hooks.user_data);
    }
}

_SOKOL_PRIVATE void _sg_imgui_end_pass(void* user_data) {
    sg_imgui_t* ctx = (sg_imgui_t*) user_data;
    SOKOL_ASSERT(ctx);
    ctx->cur_pipeline.id = SG_INVALID_ID;
    sg_imgui_capture_item_t* item = _sg_imgui_capture_next_write_item(ctx);
    if (item) {
        item->cmd = SG_IMGUI_CMD_END_PASS;
        item->color = _SG_IMGUI_COLOR_DRAW;
    }
    if (ctx->hooks.end_pass) {
        ctx->hooks.end_pass(ctx->hooks.user_data);
    }
}

_SOKOL_PRIVATE void _sg_imgui_commit(void* user_data) {
    sg_imgui_t* ctx = (sg_imgui_t*) user_data;
    SOKOL_ASSERT(ctx);
    sg_imgui_capture_item_t* item = _sg_imgui_capture_next_write_item(ctx);
    if (item) {
        item->cmd = SG_IMGUI_CMD_COMMIT;
        item->color = _SG_IMGUI_COLOR_DRAW;
    }
    _sg_imgui_capture_next_frame(ctx);
    if (ctx->hooks.commit) {
        ctx->hooks.commit(ctx->hooks.user_data);
    }
}

_SOKOL_PRIVATE void _sg_imgui_alloc_buffer(sg_buffer result, void* user_data) {
    sg_imgui_t* ctx = (sg_imgui_t*) user_data;
    SOKOL_ASSERT(ctx);
    sg_imgui_capture_item_t* item = _sg_imgui_capture_next_write_item(ctx);
    if (item) {
        item->cmd = SG_IMGUI_CMD_ALLOC_BUFFER;
        item->color = _SG_IMGUI_COLOR_RSRC;
        item->args.alloc_buffer.result = result;
    }
    if (ctx->hooks.alloc_buffer) {
        ctx->hooks.alloc_buffer(result, ctx->hooks.user_data);
    }
}

_SOKOL_PRIVATE void _sg_imgui_alloc_image(sg_image result, void* user_data) {
    sg_imgui_t* ctx = (sg_imgui_t*) user_data;
    SOKOL_ASSERT(ctx);
    sg_imgui_capture_item_t* item = _sg_imgui_capture_next_write_item(ctx);
    if (item) {
        item->cmd = SG_IMGUI_CMD_ALLOC_IMAGE;
        item->color = _SG_IMGUI_COLOR_RSRC;
        item->args.alloc_image.result = result;
    }
    if (ctx->hooks.alloc_image) {
        ctx->hooks.alloc_image(result, ctx->hooks.user_data);
    }
}

_SOKOL_PRIVATE void _sg_imgui_alloc_shader(sg_shader result, void* user_data) {
    sg_imgui_t* ctx = (sg_imgui_t*) user_data;
    SOKOL_ASSERT(ctx);
    sg_imgui_capture_item_t* item = _sg_imgui_capture_next_write_item(ctx);
    if (item) {
        item->cmd = SG_IMGUI_CMD_ALLOC_SHADER;
        item->color = _SG_IMGUI_COLOR_RSRC;
        item->args.alloc_shader.result = result;
    }
    if (ctx->hooks.alloc_shader) {
        ctx->hooks.alloc_shader(result, ctx->hooks.user_data);
    }
}

_SOKOL_PRIVATE void _sg_imgui_alloc_pipeline(sg_pipeline result, void* user_data) {
    sg_imgui_t* ctx = (sg_imgui_t*) user_data;
    SOKOL_ASSERT(ctx);
    sg_imgui_capture_item_t* item = _sg_imgui_capture_next_write_item(ctx);
    if (item) {
        item->cmd = SG_IMGUI_CMD_ALLOC_PIPELINE;
        item->color = _SG_IMGUI_COLOR_RSRC;
        item->args.alloc_pipeline.result = result;
    }
    if (ctx->hooks.alloc_pipeline) {
        ctx->hooks.alloc_pipeline(result, ctx->hooks.user_data);
    }
}

_SOKOL_PRIVATE void _sg_imgui_alloc_pass(sg_pass result, void* user_data) {
    sg_imgui_t* ctx = (sg_imgui_t*) user_data;
    SOKOL_ASSERT(ctx);
    sg_imgui_capture_item_t* item = _sg_imgui_capture_next_write_item(ctx);
    if (item) {
        item->cmd = SG_IMGUI_CMD_ALLOC_PASS;
        item->color = _SG_IMGUI_COLOR_RSRC;
        item->args.alloc_pass.result = result;
    }
    if (ctx->hooks.alloc_pass) {
        ctx->hooks.alloc_pass(result, ctx->hooks.user_data);
    }
}

_SOKOL_PRIVATE void _sg_imgui_init_buffer(sg_buffer buf_id, const sg_buffer_desc* desc, void* user_data) {
    sg_imgui_t* ctx = (sg_imgui_t*) user_data;
    SOKOL_ASSERT(ctx);
    sg_imgui_capture_item_t* item = _sg_imgui_capture_next_write_item(ctx);
    if (item) {
        item->cmd = SG_IMGUI_CMD_INIT_BUFFER;
        item->color = _SG_IMGUI_COLOR_RSRC;
        item->args.init_buffer.buffer = buf_id;
    }
    if (ctx->hooks.init_buffer) {
        ctx->hooks.init_buffer(buf_id, desc, ctx->hooks.user_data);
    }
    if (buf_id.id != SG_INVALID_ID) {
        _sg_imgui_buffer_created(ctx, buf_id, _sg_imgui_slot_index(buf_id.id), desc);
    }
}

_SOKOL_PRIVATE void _sg_imgui_init_image(sg_image img_id, const sg_image_desc* desc, void* user_data) {
    sg_imgui_t* ctx = (sg_imgui_t*) user_data;
    SOKOL_ASSERT(ctx);
    sg_imgui_capture_item_t* item = _sg_imgui_capture_next_write_item(ctx);
    if (item) {
        item->cmd = SG_IMGUI_CMD_INIT_IMAGE;
        item->color = _SG_IMGUI_COLOR_RSRC;
        item->args.init_image.image = img_id;
    }
    if (ctx->hooks.init_image) {
        ctx->hooks.init_image(img_id, desc, ctx->hooks.user_data);
    }
    if (img_id.id != SG_INVALID_ID) {
        _sg_imgui_image_created(ctx, img_id, _sg_imgui_slot_index(img_id.id), desc);
    }
}

_SOKOL_PRIVATE void _sg_imgui_init_shader(sg_shader shd_id, const sg_shader_desc* desc, void* user_data) {
    sg_imgui_t* ctx = (sg_imgui_t*) user_data;
    SOKOL_ASSERT(ctx);
    sg_imgui_capture_item_t* item = _sg_imgui_capture_next_write_item(ctx);
    if (item) {
        item->cmd = SG_IMGUI_CMD_INIT_SHADER;
        item->color = _SG_IMGUI_COLOR_RSRC;
        item->args.init_shader.shader = shd_id;
    }
    if (ctx->hooks.init_shader) {
        ctx->hooks.init_shader(shd_id, desc, ctx->hooks.user_data);
    }
    if (shd_id.id != SG_INVALID_ID) {
        _sg_imgui_shader_created(ctx, shd_id, _sg_imgui_slot_index(shd_id.id), desc);
    }
}

_SOKOL_PRIVATE void _sg_imgui_init_pipeline(sg_pipeline pip_id, const sg_pipeline_desc* desc, void* user_data) {
    sg_imgui_t* ctx = (sg_imgui_t*) user_data;
    SOKOL_ASSERT(ctx);
    sg_imgui_capture_item_t* item = _sg_imgui_capture_next_write_item(ctx);
    if (item) {
        item->cmd = SG_IMGUI_CMD_INIT_PIPELINE;
        item->color = _SG_IMGUI_COLOR_RSRC;
        item->args.init_pipeline.pipeline = pip_id;
    }
    if (ctx->hooks.init_pipeline) {
        ctx->hooks.init_pipeline(pip_id, desc, ctx->hooks.user_data);
    }
    if (pip_id.id != SG_INVALID_ID) {
        _sg_imgui_pipeline_created(ctx, pip_id, _sg_imgui_slot_index(pip_id.id), desc);
    }
}

_SOKOL_PRIVATE void _sg_imgui_init_pass(sg_pass pass_id, const sg_pass_desc* desc, void* user_data) {
    sg_imgui_t* ctx = (sg_imgui_t*) user_data;
    SOKOL_ASSERT(ctx);
    sg_imgui_capture_item_t* item = _sg_imgui_capture_next_write_item(ctx);
    if (item) {
        item->cmd = SG_IMGUI_CMD_INIT_PASS;
        item->color = _SG_IMGUI_COLOR_RSRC;
        item->args.init_pass.pass = pass_id;
    }
    if (ctx->hooks.init_pass) {
        ctx->hooks.init_pass(pass_id, desc, ctx->hooks.user_data);
    }
    if (pass_id.id != SG_INVALID_ID) {
        _sg_imgui_pass_created(ctx, pass_id, _sg_imgui_slot_index(pass_id.id), desc);
    }
}

_SOKOL_PRIVATE void _sg_imgui_fail_buffer(sg_buffer buf_id, void* user_data) {
    sg_imgui_t* ctx = (sg_imgui_t*) user_data;
    SOKOL_ASSERT(ctx);
    sg_imgui_capture_item_t* item = _sg_imgui_capture_next_write_item(ctx);
    if (item) {
        item->cmd = SG_IMGUI_CMD_FAIL_BUFFER;
        item->color = _SG_IMGUI_COLOR_RSRC;
        item->args.fail_buffer.buffer = buf_id;
    }
    if (ctx->hooks.fail_buffer) {
        ctx->hooks.fail_buffer(buf_id, ctx->hooks.user_data);
    }
}

_SOKOL_PRIVATE void _sg_imgui_fail_image(sg_image img_id, void* user_data) {
    sg_imgui_t* ctx = (sg_imgui_t*) user_data;
    SOKOL_ASSERT(ctx);
    sg_imgui_capture_item_t* item = _sg_imgui_capture_next_write_item(ctx);
    if (item) {
        item->cmd = SG_IMGUI_CMD_FAIL_IMAGE;
        item->color = _SG_IMGUI_COLOR_RSRC;
        item->args.fail_image.image = img_id;
    }
    if (ctx->hooks.fail_image) {
        ctx->hooks.fail_image(img_id, ctx->hooks.user_data);
    }
}

_SOKOL_PRIVATE void _sg_imgui_fail_shader(sg_shader shd_id, void* user_data) {
    sg_imgui_t* ctx = (sg_imgui_t*) user_data;
    SOKOL_ASSERT(ctx);
    sg_imgui_capture_item_t* item = _sg_imgui_capture_next_write_item(ctx);
    if (item) {
        item->cmd = SG_IMGUI_CMD_FAIL_SHADER;
        item->color = _SG_IMGUI_COLOR_RSRC;
        item->args.fail_shader.shader = shd_id;
    }
    if (ctx->hooks.fail_shader) {
        ctx->hooks.fail_shader(shd_id, ctx->hooks.user_data);
    }
}

_SOKOL_PRIVATE void _sg_imgui_fail_pipeline(sg_pipeline pip_id, void* user_data) {
    sg_imgui_t* ctx = (sg_imgui_t*) user_data;
    SOKOL_ASSERT(ctx);
    sg_imgui_capture_item_t* item = _sg_imgui_capture_next_write_item(ctx);
    if (item) {
        item->cmd = SG_IMGUI_CMD_FAIL_PIPELINE;
        item->color = _SG_IMGUI_COLOR_RSRC;
        item->args.fail_pipeline.pipeline = pip_id;
    }
    if (ctx->hooks.fail_pipeline) {
        ctx->hooks.fail_pipeline(pip_id, ctx->hooks.user_data);
    }
}

_SOKOL_PRIVATE void _sg_imgui_fail_pass(sg_pass pass_id, void* user_data) {
    sg_imgui_t* ctx = (sg_imgui_t*) user_data;
    SOKOL_ASSERT(ctx);
    sg_imgui_capture_item_t* item = _sg_imgui_capture_next_write_item(ctx);
    if (item) {
        item->cmd = SG_IMGUI_CMD_FAIL_PASS;
        item->color = _SG_IMGUI_COLOR_RSRC;
        item->args.fail_pass.pass = pass_id;
    }
    if (ctx->hooks.fail_pass) {
        ctx->hooks.fail_pass(pass_id, ctx->hooks.user_data);
    }
}

_SOKOL_PRIVATE void _sg_imgui_push_debug_group(const char* name, void* user_data) {
    sg_imgui_t* ctx = (sg_imgui_t*) user_data;
    SOKOL_ASSERT(ctx);
    sg_imgui_capture_item_t* item = _sg_imgui_capture_next_write_item(ctx);
    if (item) {
        item->cmd = SG_IMGUI_CMD_PUSH_DEBUG_GROUP;
        item->color = _SG_IMGUI_COLOR_OTHER;
        item->args.push_debug_group.name = _sg_imgui_make_str(name);
    }
    if (ctx->hooks.push_debug_group) {
        ctx->hooks.push_debug_group(name, ctx->hooks.user_data);
    }
}

_SOKOL_PRIVATE void _sg_imgui_pop_debug_group(void* user_data) {
    sg_imgui_t* ctx = (sg_imgui_t*) user_data;
    SOKOL_ASSERT(ctx);
    sg_imgui_capture_item_t* item = _sg_imgui_capture_next_write_item(ctx);
    if (item) {
        item->cmd = SG_IMGUI_CMD_POP_DEBUG_GROUP;
        item->color = _SG_IMGUI_COLOR_OTHER;
    }
    if (ctx->hooks.pop_debug_group) {
        ctx->hooks.pop_debug_group(ctx->hooks.user_data);
    }
}

_SOKOL_PRIVATE void _sg_imgui_err_buffer_pool_exhausted(void* user_data) {
    sg_imgui_t* ctx = (sg_imgui_t*) user_data;
    SOKOL_ASSERT(ctx);
    sg_imgui_capture_item_t* item = _sg_imgui_capture_next_write_item(ctx);
    if (item) {
        item->cmd = SG_IMGUI_CMD_ERR_BUFFER_POOL_EXHAUSTED;
        item->color = _SG_IMGUI_COLOR_ERR;
    }
    if (ctx->hooks.err_buffer_pool_exhausted) {
        ctx->hooks.err_buffer_pool_exhausted(ctx->hooks.user_data);
    }
}

_SOKOL_PRIVATE void _sg_imgui_err_image_pool_exhausted(void* user_data) {
    sg_imgui_t* ctx = (sg_imgui_t*) user_data;
    SOKOL_ASSERT(ctx);
    sg_imgui_capture_item_t* item = _sg_imgui_capture_next_write_item(ctx);
    if (item) {
        item->cmd = SG_IMGUI_CMD_ERR_IMAGE_POOL_EXHAUSTED;
        item->color = _SG_IMGUI_COLOR_ERR;
    }
    if (ctx->hooks.err_image_pool_exhausted) {
        ctx->hooks.err_image_pool_exhausted(ctx->hooks.user_data);
    }
}

_SOKOL_PRIVATE void _sg_imgui_err_shader_pool_exhausted(void* user_data) {
    sg_imgui_t* ctx = (sg_imgui_t*) user_data;
    SOKOL_ASSERT(ctx);
    sg_imgui_capture_item_t* item = _sg_imgui_capture_next_write_item(ctx);
    if (item) {
        item->cmd = SG_IMGUI_CMD_ERR_SHADER_POOL_EXHAUSTED;
        item->color = _SG_IMGUI_COLOR_ERR;
    }
    if (ctx->hooks.err_shader_pool_exhausted) {
        ctx->hooks.err_shader_pool_exhausted(ctx->hooks.user_data);
    }
}

_SOKOL_PRIVATE void _sg_imgui_err_pipeline_pool_exhausted(void* user_data) {
    sg_imgui_t* ctx = (sg_imgui_t*) user_data;
    SOKOL_ASSERT(ctx);
    sg_imgui_capture_item_t* item = _sg_imgui_capture_next_write_item(ctx);
    if (item) {
        item->cmd = SG_IMGUI_CMD_ERR_PIPELINE_POOL_EXHAUSTED;
        item->color = _SG_IMGUI_COLOR_ERR;
    }
    if (ctx->hooks.err_pipeline_pool_exhausted) {
        ctx->hooks.err_pipeline_pool_exhausted(ctx->hooks.user_data);
    }
}

_SOKOL_PRIVATE void _sg_imgui_err_pass_pool_exhausted(void* user_data) {
    sg_imgui_t* ctx = (sg_imgui_t*) user_data;
    SOKOL_ASSERT(ctx);
    sg_imgui_capture_item_t* item = _sg_imgui_capture_next_write_item(ctx);
    if (item) {
        item->cmd = SG_IMGUI_CMD_ERR_PASS_POOL_EXHAUSTED;
        item->color = _SG_IMGUI_COLOR_ERR;
    }
    if (ctx->hooks.err_pass_pool_exhausted) {
        ctx->hooks.err_pass_pool_exhausted(ctx->hooks.user_data);
    }
}

_SOKOL_PRIVATE void _sg_imgui_err_context_mismatch(void* user_data) {
    sg_imgui_t* ctx = (sg_imgui_t*) user_data;
    SOKOL_ASSERT(ctx);
    sg_imgui_capture_item_t* item = _sg_imgui_capture_next_write_item(ctx);
    if (item) {
        item->cmd = SG_IMGUI_CMD_ERR_CONTEXT_MISMATCH;
        item->color = _SG_IMGUI_COLOR_ERR;
    }
    if (ctx->hooks.err_context_mismatch) {
        ctx->hooks.err_context_mismatch(ctx->hooks.user_data);
    }
}

_SOKOL_PRIVATE void _sg_imgui_err_pass_invalid(void* user_data) {
    sg_imgui_t* ctx = (sg_imgui_t*) user_data;
    SOKOL_ASSERT(ctx);
    sg_imgui_capture_item_t* item = _sg_imgui_capture_next_write_item(ctx);
    if (item) {
        item->cmd = SG_IMGUI_CMD_ERR_PASS_INVALID;
        item->color = _SG_IMGUI_COLOR_ERR;
    }
    if (ctx->hooks.err_pass_invalid) {
        ctx->hooks.err_pass_invalid(ctx->hooks.user_data);
    }
}

_SOKOL_PRIVATE void _sg_imgui_err_draw_invalid(void* user_data) {
    sg_imgui_t* ctx = (sg_imgui_t*) user_data;
    SOKOL_ASSERT(ctx);
    sg_imgui_capture_item_t* item = _sg_imgui_capture_next_write_item(ctx);
    if (item) {
        item->cmd = SG_IMGUI_CMD_ERR_DRAW_INVALID;
        item->color = _SG_IMGUI_COLOR_ERR;
    }
    if (ctx->hooks.err_draw_invalid) {
        ctx->hooks.err_draw_invalid(ctx->hooks.user_data);
    }
}

_SOKOL_PRIVATE void _sg_imgui_err_bindings_invalid(void* user_data) {
    sg_imgui_t* ctx = (sg_imgui_t*) user_data;
    SOKOL_ASSERT(ctx);
    sg_imgui_capture_item_t* item = _sg_imgui_capture_next_write_item(ctx);
    if (item) {
        item->cmd = SG_IMGUI_CMD_ERR_BINDINGS_INVALID;
        item->color = _SG_IMGUI_COLOR_ERR;
    }
    if (ctx->hooks.err_bindings_invalid) {
        ctx->hooks.err_bindings_invalid(ctx->hooks.user_data);
    }
}

/*--- IMGUI HELPERS ----------------------------------------------------------*/
_SOKOL_PRIVATE bool _sg_imgui_draw_resid_list_item(uint32_t res_id, const char* label, bool selected) {
    igPushIDInt((int)res_id);
    bool res;
    if (label[0]) {
        res = igSelectable(label, selected, 0, IMVEC2(0,0));
    }
    else {
        sg_imgui_str_t str;
        _sg_imgui_snprintf(&str, "0x%08X", res_id);
        res = igSelectable(str.buf, selected, 0, IMVEC2(0,0));
    }
    igPopID();
    return res;
}

_SOKOL_PRIVATE bool _sg_imgui_draw_resid_link(uint32_t res_id, const char* label) {
    SOKOL_ASSERT(label);
    sg_imgui_str_t str_buf;
    const char* str;
    if (label[0]) {
        str = label;
    }
    else {
        _sg_imgui_snprintf(&str_buf, "0x%08X", res_id);
        str = str_buf.buf;
    }
    igPushIDInt((int)res_id);
    bool res = igSmallButton(str);
    igPopID();
    return res;
}

_SOKOL_PRIVATE bool _sg_imgui_draw_buffer_link(sg_imgui_t* ctx, sg_buffer buf) {
    bool retval = false;
    if (buf.id != SG_INVALID_ID) {
        const sg_imgui_buffer_t* buf_ui = &ctx->buffers.slots[_sg_imgui_slot_index(buf.id)];
        retval = _sg_imgui_draw_resid_link(buf.id, buf_ui->label.buf);
    }
    return retval;
}

_SOKOL_PRIVATE bool _sg_imgui_draw_image_link(sg_imgui_t* ctx, sg_image img) {
    bool retval = false;
    if (img.id != SG_INVALID_ID) {
        const sg_imgui_image_t* img_ui = &ctx->images.slots[_sg_imgui_slot_index(img.id)];
        retval = _sg_imgui_draw_resid_link(img.id, img_ui->label.buf);
    }
    return retval;
}

_SOKOL_PRIVATE bool _sg_imgui_draw_shader_link(sg_imgui_t* ctx, sg_shader shd) {
    bool retval = false;
    if (shd.id != SG_INVALID_ID) {
        const sg_imgui_shader_t* shd_ui = &ctx->shaders.slots[_sg_imgui_slot_index(shd.id)];
        retval = _sg_imgui_draw_resid_link(shd.id, shd_ui->label.buf);
    }
    return retval;
}

_SOKOL_PRIVATE void _sg_imgui_show_buffer(sg_imgui_t* ctx, sg_buffer buf) {
    ctx->buffers.open = true;
    ctx->buffers.sel_buf = buf;
}

_SOKOL_PRIVATE void _sg_imgui_show_image(sg_imgui_t* ctx, sg_image img) {
    ctx->images.open = true;
    ctx->images.sel_img = img;
}

_SOKOL_PRIVATE void _sg_imgui_show_shader(sg_imgui_t* ctx, sg_shader shd) {
    ctx->shaders.open = true;
    ctx->shaders.sel_shd = shd;
}

_SOKOL_PRIVATE void _sg_imgui_draw_buffer_list(sg_imgui_t* ctx) {
    igBeginChild("buffer_list", IMVEC2(_SG_IMGUI_LIST_WIDTH,0), true, 0);
    for (int i = 0; i < ctx->buffers.num_slots; i++) {
        sg_buffer buf = ctx->buffers.slots[i].res_id;
        sg_resource_state state = sg_query_buffer_state(buf);
        if ((state != SG_RESOURCESTATE_INVALID) && (state != SG_RESOURCESTATE_INITIAL)) {
            bool selected = ctx->buffers.sel_buf.id == buf.id;
            if (_sg_imgui_draw_resid_list_item(buf.id, ctx->buffers.slots[i].label.buf, selected)) {
                ctx->buffers.sel_buf.id = buf.id;
            }
        }
    }
    igEndChild();
}

_SOKOL_PRIVATE void _sg_imgui_draw_image_list(sg_imgui_t* ctx) {
    igBeginChild("image_list", IMVEC2(_SG_IMGUI_LIST_WIDTH,0), true, 0);
    for (int i = 0; i < ctx->images.num_slots; i++) {
        sg_image img = ctx->images.slots[i].res_id;
        sg_resource_state state = sg_query_image_state(img);
        if ((state != SG_RESOURCESTATE_INVALID) && (state != SG_RESOURCESTATE_INITIAL)) {
            bool selected = ctx->images.sel_img.id == img.id;
            if (_sg_imgui_draw_resid_list_item(img.id, ctx->images.slots[i].label.buf, selected)) {
                ctx->images.sel_img.id = img.id;
            }
        }
    }
    igEndChild();
}

_SOKOL_PRIVATE void _sg_imgui_draw_shader_list(sg_imgui_t* ctx) {
    igBeginChild("shader_list", IMVEC2(_SG_IMGUI_LIST_WIDTH,0), true, 0);
    for (int i = 0; i < ctx->shaders.num_slots; i++) {
        sg_shader shd = ctx->shaders.slots[i].res_id;
        sg_resource_state state = sg_query_shader_state(shd);
        if ((state != SG_RESOURCESTATE_INVALID) && (state != SG_RESOURCESTATE_INITIAL)) {
            bool selected = ctx->shaders.sel_shd.id == shd.id;
            if (_sg_imgui_draw_resid_list_item(shd.id, ctx->shaders.slots[i].label.buf, selected)) {
                ctx->shaders.sel_shd.id = shd.id;
            }
        }
    }
    igEndChild();
}

_SOKOL_PRIVATE void _sg_imgui_draw_pipeline_list(sg_imgui_t* ctx) {
    igBeginChild("pipeline_list", IMVEC2(_SG_IMGUI_LIST_WIDTH,0), true, 0);
    for (int i = 1; i < ctx->pipelines.num_slots; i++) {
        sg_pipeline pip = ctx->pipelines.slots[i].res_id;
        sg_resource_state state = sg_query_pipeline_state(pip);
        if ((state != SG_RESOURCESTATE_INVALID) && (state != SG_RESOURCESTATE_INITIAL)) {
            bool selected = ctx->pipelines.sel_pip.id == pip.id;
            if (_sg_imgui_draw_resid_list_item(pip.id, ctx->pipelines.slots[i].label.buf, selected)) {
                ctx->pipelines.sel_pip.id = pip.id;
            }
        }
    }
    igEndChild();
}

_SOKOL_PRIVATE void _sg_imgui_draw_pass_list(sg_imgui_t* ctx) {
    igBeginChild("pass_list", IMVEC2(_SG_IMGUI_LIST_WIDTH,0), true, 0);
    for (int i = 1; i < ctx->passes.num_slots; i++) {
        sg_pass pass = ctx->passes.slots[i].res_id;
        sg_resource_state state = sg_query_pass_state(pass);
        if ((state != SG_RESOURCESTATE_INVALID) && (state != SG_RESOURCESTATE_INITIAL)) {
            bool selected = ctx->passes.sel_pass.id == pass.id;
            if (_sg_imgui_draw_resid_list_item(pass.id, ctx->passes.slots[i].label.buf, selected)) {
                ctx->passes.sel_pass.id = pass.id;
            }
        }
    }
    igEndChild();
}

_SOKOL_PRIVATE void _sg_imgui_draw_capture_list(sg_imgui_t* ctx) {
    igBeginChild("capture_list", IMVEC2(_SG_IMGUI_LIST_WIDTH,0), true, 0);
    const uint32_t num_items = _sg_imgui_capture_num_read_items(ctx);
    uint64_t group_stack = 1;   /* bit set: group unfolded, cleared: folded */
    for (uint32_t i = 0; i < num_items; i++) {
        const sg_imgui_capture_item_t* item = _sg_imgui_capture_read_item_at(ctx, i);
        sg_imgui_str_t item_string = _sg_imgui_capture_item_string(ctx, i, item);
        igPushStyleColorU32(ImGuiCol_Text, item->color);
        if (item->cmd == SG_IMGUI_CMD_PUSH_DEBUG_GROUP) {
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
        else if (item->cmd == SG_IMGUI_CMD_POP_DEBUG_GROUP) {
            if (group_stack & 1) {
                igTreePop();
            }
            group_stack >>= 1;
        }
        else if (group_stack & 1) {
            igPushIDInt(i);
            if (igSelectable(item_string.buf, ctx->capture.sel_item == i, 0, IMVEC2(0,0))) {
                ctx->capture.sel_item = i;
            }
            if (igIsItemHovered(0)) {
                igSetTooltip("%s", item_string.buf);
            }
            igPopID();
        }
        igPopStyleColor(1);
    }
    igEndChild();
}

_SOKOL_PRIVATE void _sg_imgui_draw_buffer_panel(sg_imgui_t* ctx, sg_buffer buf) {
    if (buf.id != SG_INVALID_ID) {
        igBeginChild("buffer", IMVEC2(0,0), false, 0);
        sg_buffer_info info = sg_query_buffer_info(buf);
        if (info.slot.state == SG_RESOURCESTATE_VALID) {
            const sg_imgui_buffer_t* buf_ui = &ctx->buffers.slots[_sg_imgui_slot_index(buf.id)];
            igText("Label: %s", buf_ui->label.buf[0] ? buf_ui->label.buf : "---");
            _sg_imgui_draw_resource_slot(&info.slot);
            igSeparator();
            igText("Type:  %s", _sg_imgui_buffertype_string(buf_ui->desc.type));
            igText("Usage: %s", _sg_imgui_usage_string(buf_ui->desc.usage));
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

_SOKOL_PRIVATE bool _sg_imgui_image_renderable(sg_imgui_t* ctx, sg_image_type type, sg_pixel_format fmt) {
    return sg_query_pixelformat(fmt).sample && !sg_query_pixelformat(fmt).depth;
}

_SOKOL_PRIVATE void _sg_imgui_draw_embedded_image(sg_imgui_t* ctx, sg_image img, float* scale) {
    if (sg_query_image_state(img) == SG_RESOURCESTATE_VALID) {
        sg_imgui_image_t* img_ui = &ctx->images.slots[_sg_imgui_slot_index(img.id)];
        if (_sg_imgui_image_renderable(ctx, img_ui->desc.type, img_ui->desc.pixel_format)) {
            igPushIDInt((int)img.id);
            igSliderFloat("Scale", scale, 0.125f, 8.0f, "%.3f", 2.0f);
            float w = (float)img_ui->desc.width * (*scale);
            float h = (float)img_ui->desc.height * (*scale);
            igImage((ImTextureID)(intptr_t)img.id, IMVEC2(w, h), IMVEC2(0,0), IMVEC2(1,1), IMVEC4(1,1,1,1), IMVEC4(0,0,0,0));
            igPopID();
        }
        else {
            igText("Image not renderable.");
        }
    }
}

_SOKOL_PRIVATE void _sg_imgui_draw_image_panel(sg_imgui_t* ctx, sg_image img) {
    if (img.id != SG_INVALID_ID) {
        igBeginChild("image", IMVEC2(0,0), false, 0);
        sg_image_info info = sg_query_image_info(img);
        if (info.slot.state == SG_RESOURCESTATE_VALID) {
            sg_imgui_image_t* img_ui = &ctx->images.slots[_sg_imgui_slot_index(img.id)];
            const sg_image_desc* desc = &img_ui->desc;
            igText("Label: %s", img_ui->label.buf[0] ? img_ui->label.buf : "---");
            _sg_imgui_draw_resource_slot(&info.slot);
            igSeparator();
            _sg_imgui_draw_embedded_image(ctx, img, &img_ui->ui_scale);
            igSeparator();
            igText("Type:              %s", _sg_imgui_imagetype_string(desc->type));
            igText("Usage:             %s", _sg_imgui_usage_string(desc->usage));
            igText("Render Target:     %s", desc->render_target ? "YES":"NO");
            igText("Width:             %d", desc->width);
            igText("Height:            %d", desc->height);
            igText("Depth:             %d", desc->depth);
            igText("Num Mipmaps:       %d", desc->num_mipmaps);
            igText("Pixel Format:      %s", _sg_imgui_pixelformat_string(desc->pixel_format));
            igText("Sample Count:      %d", desc->sample_count);
            igText("Min Filter:        %s", _sg_imgui_filter_string(desc->min_filter));
            igText("Mag Filter:        %s", _sg_imgui_filter_string(desc->mag_filter));
            igText("Wrap U:            %s", _sg_imgui_wrap_string(desc->wrap_u));
            igText("Wrap V:            %s", _sg_imgui_wrap_string(desc->wrap_v));
            igText("Wrap W:            %s", _sg_imgui_wrap_string(desc->wrap_w));
            igText("Border Color:      %s", _sg_imgui_bordercolor_string(desc->border_color));
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

_SOKOL_PRIVATE void _sg_imgui_draw_shader_stage(sg_imgui_t* ctx, const sg_shader_stage_desc* stage) {
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
                            igText("  %s %s", _sg_imgui_uniformtype_string(u->type), u->name ? u->name : "");
                        }
                        else {
                            igText("  %s[%d] %s", _sg_imgui_uniformtype_string(u->type), u->array_count, u->name ? u->name : "");
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
                    igText("%s %s", _sg_imgui_imagetype_string(sid->type), sid->name ? sid->name : "");
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

_SOKOL_PRIVATE void _sg_imgui_draw_shader_panel(sg_imgui_t* ctx, sg_shader shd) {
    if (shd.id != SG_INVALID_ID) {
        igBeginChild("shader", IMVEC2(0,0), false, ImGuiWindowFlags_HorizontalScrollbar);
        sg_shader_info info = sg_query_shader_info(shd);
        if (info.slot.state == SG_RESOURCESTATE_VALID) {
            const sg_imgui_shader_t* shd_ui = &ctx->shaders.slots[_sg_imgui_slot_index(shd.id)];
            igText("Label: %s", shd_ui->label.buf[0] ? shd_ui->label.buf : "---");
            _sg_imgui_draw_resource_slot(&info.slot);
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
                _sg_imgui_draw_shader_stage(ctx, &shd_ui->desc.vs);
                igTreePop();
            }
            if (igTreeNodeStr("Fragment Shader Stage")) {
                _sg_imgui_draw_shader_stage(ctx, &shd_ui->desc.fs);
                igTreePop();
            }
        }
        else {
            igText("Shader 0x%08X not valid!", shd.id);
        }
        igEndChild();
    }
}

_SOKOL_PRIVATE void _sg_imgui_draw_vertex_layout(const sg_layout_desc* layout) {
    if (igTreeNodeStr("Buffers")) {
        for (int i = 0; i < SG_MAX_SHADERSTAGE_BUFFERS; i++) {
            const sg_buffer_layout_desc* l_desc = &layout->buffers[i];
            if (l_desc->stride > 0) {
                igText("#%d:", i);
                igText("  Stride:    %d", l_desc->stride);
                igText("  Step Func: %s", _sg_imgui_vertexstep_string(l_desc->step_func));
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
                igText("  Format:       %s", _sg_imgui_vertexformat_string(a_desc->format));
                igText("  Offset:       %d", a_desc->offset);
                igText("  Buffer Index: %d", a_desc->buffer_index);
            }
        }
        igTreePop();
    }
}

_SOKOL_PRIVATE void _sg_imgui_draw_stencil_state(const sg_stencil_state* ss) {
    igText("Fail Op:       %s", _sg_imgui_stencilop_string(ss->fail_op));
    igText("Depth Fail Op: %s", _sg_imgui_stencilop_string(ss->depth_fail_op));
    igText("Pass Op:       %s", _sg_imgui_stencilop_string(ss->pass_op));
    igText("Compare Func:  %s", _sg_imgui_comparefunc_string(ss->compare_func));
}

_SOKOL_PRIVATE void _sg_imgui_draw_depth_stencil_state(const sg_depth_stencil_state* dss) {
    igText("Depth Compare Func:  %s", _sg_imgui_comparefunc_string(dss->depth_compare_func));
    igText("Depth Write Enabled: %s", dss->depth_write_enabled ? "YES":"NO");
    igText("Stencil Enabled:     %s", dss->stencil_enabled ? "YES":"NO");
    igText("Stencil Read Mask:   0x%02X", dss->stencil_read_mask);
    igText("Stencil Write Mask:  0x%02X", dss->stencil_write_mask);
    igText("Stencil Ref:         0x%02X", dss->stencil_ref);
    if (igTreeNodeStr("Stencil Front")) {
        _sg_imgui_draw_stencil_state(&dss->stencil_front);
        igTreePop();
    }
    if (igTreeNodeStr("Stencil Back")) {
        _sg_imgui_draw_stencil_state(&dss->stencil_back);
        igTreePop();
    }
}

_SOKOL_PRIVATE void _sg_imgui_draw_blend_state(const sg_blend_state* bs) {
    igText("Blend Enabled:    %s", bs->enabled ? "YES":"NO");
    igText("Src Factor RGB:   %s", _sg_imgui_blendfactor_string(bs->src_factor_rgb));
    igText("Dst Factor RGB:   %s", _sg_imgui_blendfactor_string(bs->dst_factor_rgb));
    igText("Op RGB:           %s", _sg_imgui_blendop_string(bs->op_rgb));
    igText("Src Factor Alpha: %s", _sg_imgui_blendfactor_string(bs->src_factor_alpha));
    igText("Dst Factor Alpha: %s", _sg_imgui_blendfactor_string(bs->dst_factor_alpha));
    igText("Op Alpha:         %s", _sg_imgui_blendop_string(bs->op_alpha));
    igText("Color Write Mask: %s", _sg_imgui_colormask_string(bs->color_write_mask));
    igText("Attachment Count: %d", bs->color_attachment_count);
    igText("Color Format:     %s", _sg_imgui_pixelformat_string(bs->color_format));
    igText("Depth Format:     %s", _sg_imgui_pixelformat_string(bs->depth_format));
    igText("Blend Color:      %.3f %.3f %.3f %.3f", bs->blend_color[0], bs->blend_color[1], bs->blend_color[2], bs->blend_color[3]);
}

_SOKOL_PRIVATE void _sg_imgui_draw_rasterizer_state(const sg_rasterizer_state* rs) {
    igText("Alpha to Coverage: %s", rs->alpha_to_coverage_enabled ? "YES":"NO");
    igText("Cull Mode:         %s", _sg_imgui_cullmode_string(rs->cull_mode));
    igText("Face Winding:      %s", _sg_imgui_facewinding_string(rs->face_winding));
    igText("Sample Count:      %d", rs->sample_count);
    igText("Depth Bias:        %f", rs->depth_bias);
    igText("Depth Bias Slope:  %f", rs->depth_bias_slope_scale);
    igText("Depth Bias Clamp:  %f", rs->depth_bias_clamp);
}

_SOKOL_PRIVATE void _sg_imgui_draw_pipeline_panel(sg_imgui_t* ctx, sg_pipeline pip) {
    if (pip.id != SG_INVALID_ID) {
        igBeginChild("pipeline", IMVEC2(0,0), false, 0);
        sg_pipeline_info info = sg_query_pipeline_info(pip);
        if (info.slot.state == SG_RESOURCESTATE_VALID) {
            const sg_imgui_pipeline_t* pip_ui = &ctx->pipelines.slots[_sg_imgui_slot_index(pip.id)];
            igText("Label: %s", pip_ui->label.buf[0] ? pip_ui->label.buf : "---");
            _sg_imgui_draw_resource_slot(&info.slot);
            igSeparator();
            igText("Shader:    "); igSameLine(0,-1);
            if (_sg_imgui_draw_shader_link(ctx, pip_ui->desc.shader)) {
                _sg_imgui_show_shader(ctx, pip_ui->desc.shader);
            }
            igText("Prim Type:  %s", _sg_imgui_primitivetype_string(pip_ui->desc.primitive_type));
            igText("Index Type: %s", _sg_imgui_indextype_string(pip_ui->desc.index_type));
            if (igTreeNodeStr("Vertex Layout")) {
                _sg_imgui_draw_vertex_layout(&pip_ui->desc.layout);
                igTreePop();
            }
            if (igTreeNodeStr("Depth Stencil State")) {
                _sg_imgui_draw_depth_stencil_state(&pip_ui->desc.depth_stencil);
                igTreePop();
            }
            if (igTreeNodeStr("Blend State")) {
                _sg_imgui_draw_blend_state(&pip_ui->desc.blend);
                igTreePop();
            }
            if (igTreeNodeStr("Rasterizer State")) {
                _sg_imgui_draw_rasterizer_state(&pip_ui->desc.rasterizer);
                igTreePop();
            }
        }
        else {
            igText("Pipeline 0x%08X not valid.", pip.id);
        }
        igEndChild();
    }
}

_SOKOL_PRIVATE void _sg_imgui_draw_attachment(sg_imgui_t* ctx, const sg_attachment_desc* att, float* img_scale) {
    igText("  Image: "); igSameLine(0,-1);
    if (_sg_imgui_draw_image_link(ctx, att->image)) {
        _sg_imgui_show_image(ctx, att->image);
    }
    igText("  Mip Level: %d", att->mip_level);
    igText("  Face/Layer/Slice: %d", att->layer);
    _sg_imgui_draw_embedded_image(ctx, att->image, img_scale);
}

_SOKOL_PRIVATE void _sg_imgui_draw_pass_panel(sg_imgui_t* ctx, sg_pass pass) {
    if (pass.id != SG_INVALID_ID) {
        igBeginChild("pass", IMVEC2(0,0), false, 0);
        sg_pass_info info = sg_query_pass_info(pass);
        if (info.slot.state == SG_RESOURCESTATE_VALID) {
            sg_imgui_pass_t* pass_ui = &ctx->passes.slots[_sg_imgui_slot_index(pass.id)];
            igText("Label: %s", pass_ui->label.buf[0] ? pass_ui->label.buf : "---");
            _sg_imgui_draw_resource_slot(&info.slot);
            for (int i = 0; i < SG_MAX_COLOR_ATTACHMENTS; i++) {
                if (pass_ui->desc.color_attachments[i].image.id == SG_INVALID_ID) {
                    break;
                }
                igSeparator();
                igText("Color Attachment #%d:", i);
                _sg_imgui_draw_attachment(ctx, &pass_ui->desc.color_attachments[i], &pass_ui->color_image_scale[i]);
            }
            if (pass_ui->desc.depth_stencil_attachment.image.id != SG_INVALID_ID) {
                igSeparator();
                igText("Depth-Stencil Attachemnt:");
                _sg_imgui_draw_attachment(ctx, &pass_ui->desc.depth_stencil_attachment, &pass_ui->ds_image_scale);
            }
        }
        else {
            igText("Pass 0x%08X not valid.", pass.id);
        }
        igEndChild();
    }
}

_SOKOL_PRIVATE void _sg_imgui_draw_bindings_panel(sg_imgui_t* ctx, const sg_bindings* bnd) {
    for (int i = 0; i < SG_MAX_SHADERSTAGE_BUFFERS; i++) {
        sg_buffer buf = bnd->vertex_buffers[i];
        if (buf.id != SG_INVALID_ID) {
            igSeparator();
            igText("Vertex Buffer Slot #%d:", i);
            igText("  Buffer: "); igSameLine(0,-1);
            if (_sg_imgui_draw_buffer_link(ctx, buf)) {
                _sg_imgui_show_buffer(ctx, buf);
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
            igText("  Buffer: "); igSameLine(0,-1);
            if (_sg_imgui_draw_buffer_link(ctx, buf)) {
                _sg_imgui_show_buffer(ctx, buf);
            }
            igText("  Offset: %d", bnd->index_buffer_offset);
        }
    }
    for (int i = 0; i < SG_MAX_SHADERSTAGE_IMAGES; i++) {
        sg_image img = bnd->vs_images[i];
        if (img.id != SG_INVALID_ID) {
            igSeparator();
            igText("Vertex Stage Image Slot #%d:", i);
            igText("  Image: "); igSameLine(0,-1);
            if (_sg_imgui_draw_image_link(ctx, img)) {
                _sg_imgui_show_image(ctx, img);
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
            igText("  Image: "); igSameLine(0,-1);
            if (_sg_imgui_draw_image_link(ctx, img)) {
                _sg_imgui_show_image(ctx, img);
            }
        }
    }
}

_SOKOL_PRIVATE void _sg_imgui_draw_uniforms_panel(sg_imgui_t* ctx, const sg_imgui_args_apply_uniforms_t* args) {
    SOKOL_ASSERT(args->ub_index < SG_MAX_SHADERSTAGE_BUFFERS);

    /* check if all the required information for drawing the structured uniform block content
        is available, otherwise just render a generic hexdump
    */
   if (sg_query_pipeline_state(args->pipeline) != SG_RESOURCESTATE_VALID) {
        igText("Pipeline object not valid!");
        return;
   }
    sg_imgui_pipeline_t* pip_ui = &ctx->pipelines.slots[_sg_imgui_slot_index(args->pipeline.id)];
    if (sg_query_shader_state(pip_ui->desc.shader) != SG_RESOURCESTATE_VALID) {
        igText("Shader object not valid!");
        return;
    }
    sg_imgui_shader_t* shd_ui = &ctx->shaders.slots[_sg_imgui_slot_index(pip_ui->desc.shader.id)];
    SOKOL_ASSERT(shd_ui->res_id.id == pip_ui->desc.shader.id);
    const sg_shader_uniform_block_desc* ub_desc = (args->stage == SG_SHADERSTAGE_VS) ?
        &shd_ui->desc.vs.uniform_blocks[args->ub_index] :
        &shd_ui->desc.fs.uniform_blocks[args->ub_index];
    SOKOL_ASSERT(args->num_bytes <= ub_desc->size);
    bool draw_dump = false;
    if (ub_desc->uniforms[0].type == SG_UNIFORMTYPE_INVALID) {
        draw_dump = true;
    }

    sg_imgui_capture_bucket_t* bucket = _sg_imgui_capture_get_read_bucket(ctx);
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
                igText("%d: %s %s[%d] =", i, _sg_imgui_uniformtype_string(ud->type), ud->name?ud->name:"", ud->array_count);
            }
            else {
                igText("%d: %s %s =", i, _sg_imgui_uniformtype_string(ud->type), ud->name?ud->name:"");
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
                uptrf += _sg_imgui_uniform_size(ud->type, 1) / sizeof(float);
            }
        }
    }
    else {
        const uint32_t num_floats = ub_desc->size / sizeof(float);
        for (uint32_t i = 0; i < num_floats; i++) {
            igText("%.3f, ", uptrf[i]);
            if (((i + 1) % 4) != 0) {
                igSameLine(0,-1);
            }
        }
    }
}

_SOKOL_PRIVATE void _sg_imgui_draw_passaction_panel(sg_imgui_t* ctx, sg_pass pass, const sg_pass_action* action) {
    /* determine number of valid color attachments in the pass */
    int num_color_atts = 0;
    if (SG_INVALID_ID == pass.id) {
        /* default pass: one color attachment */
        num_color_atts = 1;
    }
    else {
        const sg_imgui_pass_t* pass_ui = &ctx->passes.slots[_sg_imgui_slot_index(pass.id)];
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

_SOKOL_PRIVATE void _sg_imgui_draw_capture_panel(sg_imgui_t* ctx) {
    uint32_t sel_item_index = ctx->capture.sel_item;
    if (sel_item_index >= _sg_imgui_capture_num_read_items(ctx)) {
        return;
    }
    sg_imgui_capture_item_t* item = _sg_imgui_capture_read_item_at(ctx, sel_item_index);
    igBeginChild("capture_item", IMVEC2(0, 0), false, 0);
    igPushStyleColorU32(ImGuiCol_Text, item->color);
    igText("%s", _sg_imgui_capture_item_string(ctx, sel_item_index, item).buf);
    igPopStyleColor(1);
    igSeparator();
    switch (item->cmd) {
        case SG_IMGUI_CMD_RESET_STATE_CACHE:
            break;
        case SG_IMGUI_CMD_MAKE_BUFFER:
            _sg_imgui_draw_buffer_panel(ctx, item->args.make_buffer.result);
            break;
        case SG_IMGUI_CMD_MAKE_IMAGE:
            _sg_imgui_draw_image_panel(ctx, item->args.make_image.result);
            break;
        case SG_IMGUI_CMD_MAKE_SHADER:
            _sg_imgui_draw_shader_panel(ctx, item->args.make_shader.result);
            break;
        case SG_IMGUI_CMD_MAKE_PIPELINE:
            _sg_imgui_draw_pipeline_panel(ctx, item->args.make_pipeline.result);
            break;
        case SG_IMGUI_CMD_MAKE_PASS:
            _sg_imgui_draw_pass_panel(ctx, item->args.make_pass.result);
            break;
        case SG_IMGUI_CMD_DESTROY_BUFFER:
            _sg_imgui_draw_buffer_panel(ctx, item->args.destroy_buffer.buffer);
            break;
        case SG_IMGUI_CMD_DESTROY_IMAGE:
            _sg_imgui_draw_image_panel(ctx, item->args.destroy_image.image);
            break;
        case SG_IMGUI_CMD_DESTROY_SHADER:
            _sg_imgui_draw_shader_panel(ctx, item->args.destroy_shader.shader);
            break;
        case SG_IMGUI_CMD_DESTROY_PIPELINE:
            _sg_imgui_draw_pipeline_panel(ctx, item->args.destroy_pipeline.pipeline);
            break;
        case SG_IMGUI_CMD_DESTROY_PASS:
            _sg_imgui_draw_pass_panel(ctx, item->args.destroy_pass.pass);
            break;
        case SG_IMGUI_CMD_UPDATE_BUFFER:
            _sg_imgui_draw_buffer_panel(ctx, item->args.update_buffer.buffer);
            break;
        case SG_IMGUI_CMD_UPDATE_IMAGE:
            _sg_imgui_draw_image_panel(ctx, item->args.update_image.image);
            break;
        case SG_IMGUI_CMD_APPEND_BUFFER:
            _sg_imgui_draw_buffer_panel(ctx, item->args.update_buffer.buffer);
            break;
        case SG_IMGUI_CMD_BEGIN_DEFAULT_PASS:
            {
                sg_pass inv_pass = { SG_INVALID_ID };
                _sg_imgui_draw_passaction_panel(ctx, inv_pass, &item->args.begin_default_pass.action);
            }
            break;
        case SG_IMGUI_CMD_BEGIN_PASS:
            _sg_imgui_draw_passaction_panel(ctx, item->args.begin_pass.pass, &item->args.begin_pass.action);
            igSeparator();
            _sg_imgui_draw_pass_panel(ctx, item->args.begin_pass.pass);
            break;
        case SG_IMGUI_CMD_APPLY_VIEWPORT:
        case SG_IMGUI_CMD_APPLY_SCISSOR_RECT:
            break;
        case SG_IMGUI_CMD_APPLY_PIPELINE:
            _sg_imgui_draw_pipeline_panel(ctx, item->args.apply_pipeline.pipeline);
            break;
        case SG_IMGUI_CMD_APPLY_BINDINGS:
            _sg_imgui_draw_bindings_panel(ctx, &item->args.apply_bindings.bindings);
            break;
        case SG_IMGUI_CMD_APPLY_UNIFORMS:
            _sg_imgui_draw_uniforms_panel(ctx, &item->args.apply_uniforms);
            break;
        case SG_IMGUI_CMD_DRAW:
        case SG_IMGUI_CMD_END_PASS:
        case SG_IMGUI_CMD_COMMIT:
            break;
        case SG_IMGUI_CMD_ALLOC_BUFFER:
            _sg_imgui_draw_buffer_panel(ctx, item->args.alloc_buffer.result);
            break;
        case SG_IMGUI_CMD_ALLOC_IMAGE:
            _sg_imgui_draw_image_panel(ctx, item->args.alloc_image.result);
            break;
        case SG_IMGUI_CMD_ALLOC_SHADER:
            _sg_imgui_draw_shader_panel(ctx, item->args.alloc_shader.result);
            break;
        case SG_IMGUI_CMD_ALLOC_PIPELINE:
            _sg_imgui_draw_pipeline_panel(ctx, item->args.alloc_pipeline.result);
            break;
        case SG_IMGUI_CMD_ALLOC_PASS:
            _sg_imgui_draw_pass_panel(ctx, item->args.alloc_pass.result);
            break;
        case SG_IMGUI_CMD_INIT_BUFFER:
            _sg_imgui_draw_buffer_panel(ctx, item->args.init_buffer.buffer);
            break;
        case SG_IMGUI_CMD_INIT_IMAGE:
            _sg_imgui_draw_image_panel(ctx, item->args.init_image.image);
            break;
        case SG_IMGUI_CMD_INIT_SHADER:
            _sg_imgui_draw_shader_panel(ctx, item->args.init_shader.shader);
            break;
        case SG_IMGUI_CMD_INIT_PIPELINE:
            _sg_imgui_draw_pipeline_panel(ctx, item->args.init_pipeline.pipeline);
            break;
        case SG_IMGUI_CMD_INIT_PASS:
            _sg_imgui_draw_pass_panel(ctx, item->args.init_pass.pass);
            break;
        case SG_IMGUI_CMD_FAIL_BUFFER:
            _sg_imgui_draw_buffer_panel(ctx, item->args.fail_buffer.buffer);
            break;
        case SG_IMGUI_CMD_FAIL_IMAGE:
            _sg_imgui_draw_image_panel(ctx, item->args.fail_image.image);
            break;
        case SG_IMGUI_CMD_FAIL_SHADER:
            _sg_imgui_draw_shader_panel(ctx, item->args.fail_shader.shader);
            break;
        case SG_IMGUI_CMD_FAIL_PIPELINE:
            _sg_imgui_draw_pipeline_panel(ctx, item->args.fail_pipeline.pipeline);
            break;
        case SG_IMGUI_CMD_FAIL_PASS:
            _sg_imgui_draw_pass_panel(ctx, item->args.fail_pass.pass);
            break;
        default:
            break;
    }
    igEndChild();
}

_SOKOL_PRIVATE void _sg_imgui_draw_caps_panel(sg_imgui_t* ctx) {
    igText("Backend: %s\n\n", _sg_imgui_backend_string(sg_query_backend()));
    sg_features f = sg_query_features();
    igText("Features:");
    igText("    instancing: %s", _sg_imgui_bool_string(f.instancing));
    igText("    origin_top_left: %s", _sg_imgui_bool_string(f.origin_top_left));
    igText("    multiple_render_targets: %s", _sg_imgui_bool_string(f.multiple_render_targets));
    igText("    msaa_render_targets: %s", _sg_imgui_bool_string(f.msaa_render_targets));
    igText("    imagetype_3d: %s", _sg_imgui_bool_string(f.imagetype_3d));
    igText("    imagetype_array: %s", _sg_imgui_bool_string(f.imagetype_array));
    igText("    image_clamp_to_border: %s", _sg_imgui_bool_string(f.image_clamp_to_border));
    sg_limits l = sg_query_limits();
    igText("\nLimits:\n");
    igText("    max_image_size_2d: %d", l.max_image_size_2d);
    igText("    max_image_size_cube: %d", l.max_image_size_cube);
    igText("    max_image_size_3d: %d", l.max_image_size_3d);
    igText("    max_image_size_array: %d", l.max_image_size_array);
    igText("    max_image_array_layers: %d", l.max_image_array_layers);
    igText("    max_vertex_attrs: %d", l.max_vertex_attrs);
    igText("\nUsable Pixelformats:");
    for (int i = (int)(SG_PIXELFORMAT_NONE+1); i < (int)_SG_PIXELFORMAT_NUM; i++) {
        sg_pixel_format fmt = (sg_pixel_format)i;
        sg_pixelformat_info info = sg_query_pixelformat(fmt);
        if (info.sample) {
            igText("  %s: %s%s%s%s%s%s",
                _sg_imgui_pixelformat_string(fmt),
                info.sample ? "SAMPLE ":"",
                info.filter ? "FILTER ":"",
                info.blend ? "BLEND ":"",
                info.render ? "RENDER ":"",
                info.msaa ? "MSAA ":"",
                info.depth ? "DEPTH ":"");
        }
    }
}

/*--- PUBLIC FUNCTIONS -------------------------------------------------------*/
SOKOL_API_IMPL void sg_imgui_init(sg_imgui_t* ctx) {
    SOKOL_ASSERT(ctx);
    memset(ctx, 0, sizeof(sg_imgui_t));
    ctx->init_tag = 0xABCDABCD;
    _sg_imgui_capture_init(ctx);

    /* hook into sokol_gfx functions */
    sg_trace_hooks hooks;
    memset(&hooks, 0, sizeof(hooks));
    hooks.user_data = (void*) ctx;
    hooks.reset_state_cache = _sg_imgui_reset_state_cache;
    hooks.make_buffer = _sg_imgui_make_buffer;
    hooks.make_image = _sg_imgui_make_image;
    hooks.make_shader = _sg_imgui_make_shader;
    hooks.make_pipeline = _sg_imgui_make_pipeline;
    hooks.make_pass = _sg_imgui_make_pass;
    hooks.destroy_buffer = _sg_imgui_destroy_buffer;
    hooks.destroy_image = _sg_imgui_destroy_image;
    hooks.destroy_shader = _sg_imgui_destroy_shader;
    hooks.destroy_pipeline = _sg_imgui_destroy_pipeline;
    hooks.destroy_pass = _sg_imgui_destroy_pass;
    hooks.update_buffer = _sg_imgui_update_buffer;
    hooks.update_image = _sg_imgui_update_image;
    hooks.append_buffer = _sg_imgui_append_buffer;
    hooks.begin_default_pass = _sg_imgui_begin_default_pass;
    hooks.begin_pass = _sg_imgui_begin_pass;
    hooks.apply_viewport = _sg_imgui_apply_viewport;
    hooks.apply_scissor_rect = _sg_imgui_apply_scissor_rect;
    hooks.apply_pipeline = _sg_imgui_apply_pipeline;
    hooks.apply_bindings = _sg_imgui_apply_bindings;
    hooks.apply_uniforms = _sg_imgui_apply_uniforms;
    hooks.draw = _sg_imgui_draw;
    hooks.end_pass = _sg_imgui_end_pass;
    hooks.commit = _sg_imgui_commit;
    hooks.alloc_buffer = _sg_imgui_alloc_buffer;
    hooks.alloc_image = _sg_imgui_alloc_image;
    hooks.alloc_shader = _sg_imgui_alloc_shader;
    hooks.alloc_pipeline = _sg_imgui_alloc_pipeline;
    hooks.alloc_pass = _sg_imgui_alloc_pass;
    hooks.init_buffer = _sg_imgui_init_buffer;
    hooks.init_image = _sg_imgui_init_image;
    hooks.init_shader = _sg_imgui_init_shader;
    hooks.init_pipeline = _sg_imgui_init_pipeline;
    hooks.init_pass = _sg_imgui_init_pass;
    hooks.fail_buffer = _sg_imgui_fail_buffer;
    hooks.fail_image = _sg_imgui_fail_image;
    hooks.fail_shader = _sg_imgui_fail_shader;
    hooks.fail_pipeline = _sg_imgui_fail_pipeline;
    hooks.fail_pass = _sg_imgui_fail_pass;
    hooks.push_debug_group = _sg_imgui_push_debug_group;
    hooks.pop_debug_group = _sg_imgui_pop_debug_group;
    hooks.err_buffer_pool_exhausted = _sg_imgui_err_buffer_pool_exhausted;
    hooks.err_image_pool_exhausted = _sg_imgui_err_image_pool_exhausted;
    hooks.err_shader_pool_exhausted = _sg_imgui_err_shader_pool_exhausted;
    hooks.err_pipeline_pool_exhausted = _sg_imgui_err_pipeline_pool_exhausted;
    hooks.err_pass_pool_exhausted = _sg_imgui_err_pass_pool_exhausted;
    hooks.err_context_mismatch = _sg_imgui_err_context_mismatch;
    hooks.err_pass_invalid = _sg_imgui_err_pass_invalid;
    hooks.err_draw_invalid = _sg_imgui_err_draw_invalid;
    hooks.err_bindings_invalid = _sg_imgui_err_bindings_invalid;
    ctx->hooks = sg_install_trace_hooks(&hooks);

    /* allocate resource debug-info slots */
    sg_desc desc = sg_query_desc();
    ctx->buffers.num_slots = desc.buffer_pool_size;
    ctx->images.num_slots = desc.image_pool_size;
    ctx->shaders.num_slots = desc.shader_pool_size;
    ctx->pipelines.num_slots = desc.pipeline_pool_size;
    ctx->passes.num_slots = desc.pass_pool_size;

    const int buffer_pool_size = ctx->buffers.num_slots * sizeof(sg_imgui_buffer_t);
    ctx->buffers.slots = (sg_imgui_buffer_t*) _sg_imgui_alloc(buffer_pool_size);
    SOKOL_ASSERT(ctx->buffers.slots);
    memset(ctx->buffers.slots, 0, buffer_pool_size);

    const int image_pool_size = ctx->images.num_slots * sizeof(sg_imgui_image_t);
    ctx->images.slots = (sg_imgui_image_t*) _sg_imgui_alloc(image_pool_size);
    SOKOL_ASSERT(ctx->images.slots);
    memset(ctx->images.slots, 0, image_pool_size);

    const int shader_pool_size = ctx->shaders.num_slots * sizeof(sg_imgui_shader_t);
    ctx->shaders.slots = (sg_imgui_shader_t*) _sg_imgui_alloc(shader_pool_size);
    SOKOL_ASSERT(ctx->shaders.slots);
    memset(ctx->shaders.slots, 0, shader_pool_size);

    const int pipeline_pool_size = ctx->pipelines.num_slots * sizeof(sg_imgui_pipeline_t);
    ctx->pipelines.slots = (sg_imgui_pipeline_t*) _sg_imgui_alloc(pipeline_pool_size);
    SOKOL_ASSERT(ctx->pipelines.slots);
    memset(ctx->pipelines.slots, 0, pipeline_pool_size);

    const int pass_pool_size = ctx->passes.num_slots * sizeof(sg_imgui_pass_t);
    ctx->passes.slots = (sg_imgui_pass_t*) _sg_imgui_alloc(pass_pool_size);
    SOKOL_ASSERT(ctx->passes.slots);
    memset(ctx->passes.slots, 0, pass_pool_size);
}

SOKOL_API_IMPL void sg_imgui_discard(sg_imgui_t* ctx) {
    SOKOL_ASSERT(ctx && (ctx->init_tag == 0xABCDABCD));
    /* restore original trace hooks */
    sg_install_trace_hooks(&ctx->hooks);
    ctx->init_tag = 0;
    _sg_imgui_capture_discard(ctx);
    if (ctx->buffers.slots) {
        for (int i = 0; i < ctx->buffers.num_slots; i++) {
            if (ctx->buffers.slots[i].res_id.id != SG_INVALID_ID) {
                _sg_imgui_buffer_destroyed(ctx, i);
            }
        }
        _sg_imgui_free((void*)ctx->buffers.slots);
        ctx->buffers.slots = 0;
    }
    if (ctx->images.slots) {
        for (int i = 0; i < ctx->images.num_slots; i++) {
            if (ctx->images.slots[i].res_id.id != SG_INVALID_ID) {
                _sg_imgui_image_destroyed(ctx, i);
            }
        }
        _sg_imgui_free((void*)ctx->images.slots);
        ctx->images.slots = 0;
    }
    if (ctx->shaders.slots) {
        for (int i = 0; i < ctx->shaders.num_slots; i++) {
            if (ctx->shaders.slots[i].res_id.id != SG_INVALID_ID) {
                _sg_imgui_shader_destroyed(ctx, i);
            }
        }
        _sg_imgui_free((void*)ctx->shaders.slots);
        ctx->shaders.slots = 0;
    }
    if (ctx->pipelines.slots) {
        for (int i = 0; i < ctx->pipelines.num_slots; i++) {
            if (ctx->pipelines.slots[i].res_id.id != SG_INVALID_ID) {
                _sg_imgui_pipeline_destroyed(ctx, i);
            }
        }
        _sg_imgui_free((void*)ctx->pipelines.slots);
        ctx->pipelines.slots = 0;
    }
    if (ctx->passes.slots) {
        for (int i = 0; i < ctx->passes.num_slots; i++) {
            if (ctx->passes.slots[i].res_id.id != SG_INVALID_ID) {
                _sg_imgui_pass_destroyed(ctx, i);
            }
        }
        _sg_imgui_free((void*)ctx->passes.slots);
        ctx->passes.slots = 0;
    }
}

SOKOL_API_IMPL void sg_imgui_draw(sg_imgui_t* ctx) {
    SOKOL_ASSERT(ctx && (ctx->init_tag == 0xABCDABCD));
    sg_imgui_draw_buffers_window(ctx);
    sg_imgui_draw_images_window(ctx);
    sg_imgui_draw_shaders_window(ctx);
    sg_imgui_draw_pipelines_window(ctx);
    sg_imgui_draw_passes_window(ctx);
    sg_imgui_draw_capture_window(ctx);
    sg_imgui_draw_capabilities_window(ctx);
}

SOKOL_API_IMPL void sg_imgui_draw_buffers_window(sg_imgui_t* ctx) {
    SOKOL_ASSERT(ctx && (ctx->init_tag == 0xABCDABCD));
    if (!ctx->buffers.open) {
        return;
    }
    igSetNextWindowSize(IMVEC2(440, 280), ImGuiCond_Once);
    if (igBegin("Buffers", &ctx->buffers.open, 0)) {
        sg_imgui_draw_buffers_content(ctx);
    }
    igEnd();
}

SOKOL_API_IMPL void sg_imgui_draw_images_window(sg_imgui_t* ctx) {
    SOKOL_ASSERT(ctx && (ctx->init_tag == 0xABCDABCD));
    if (!ctx->images.open) {
        return;
    }
    igSetNextWindowSize(IMVEC2(440, 400), ImGuiCond_Once);
    if (igBegin("Images", &ctx->images.open, 0)) {
        sg_imgui_draw_images_content(ctx);
    }
    igEnd();
}

SOKOL_API_IMPL void sg_imgui_draw_shaders_window(sg_imgui_t* ctx) {
    SOKOL_ASSERT(ctx && (ctx->init_tag == 0xABCDABCD));
    if (!ctx->shaders.open) {
        return;
    }
    igSetNextWindowSize(IMVEC2(440, 400), ImGuiCond_Once);
    if (igBegin("Shaders", &ctx->shaders.open, 0)) {
        sg_imgui_draw_shaders_content(ctx);
    }
    igEnd();
}

SOKOL_API_IMPL void sg_imgui_draw_pipelines_window(sg_imgui_t* ctx) {
    SOKOL_ASSERT(ctx && (ctx->init_tag == 0xABCDABCD));
    if (!ctx->pipelines.open) {
        return;
    }
    igSetNextWindowSize(IMVEC2(540, 400), ImGuiCond_Once);
    if (igBegin("Pipelines", &ctx->pipelines.open, 0)) {
        sg_imgui_draw_pipelines_content(ctx);
    }
    igEnd();
}

SOKOL_API_IMPL void sg_imgui_draw_passes_window(sg_imgui_t* ctx) {
    SOKOL_ASSERT(ctx && (ctx->init_tag == 0xABCDABCD));
    if (!ctx->passes.open) {
        return;
    }
    igSetNextWindowSize(IMVEC2(440, 400), ImGuiCond_Once);
    if (igBegin("Passes", &ctx->passes.open, 0)) {
        sg_imgui_draw_passes_content(ctx);
    }
    igEnd();
}

SOKOL_API_IMPL void sg_imgui_draw_capture_window(sg_imgui_t* ctx) {
    SOKOL_ASSERT(ctx && (ctx->init_tag == 0xABCDABCD));
    if (!ctx->capture.open) {
        return;
    }
    igSetNextWindowSize(IMVEC2(640, 400), ImGuiCond_Once);
    if (igBegin("Frame Capture", &ctx->capture.open, 0)) {
        sg_imgui_draw_capture_content(ctx);
    }
    igEnd();
}

SOKOL_API_IMPL void sg_imgui_draw_capabilities_window(sg_imgui_t* ctx) {
    SOKOL_ASSERT(ctx && (ctx->init_tag == 0xABCDABCD));
    if (!ctx->caps.open) {
        return;
    }
    igSetNextWindowSize(IMVEC2(440, 400), ImGuiCond_Once);
    if (igBegin("Capabilities", &ctx->caps.open, 0)) {
        sg_imgui_draw_capabilities_content(ctx);
    }
    igEnd();
}

SOKOL_API_IMPL void sg_imgui_draw_buffers_content(sg_imgui_t* ctx) {
    SOKOL_ASSERT(ctx && (ctx->init_tag == 0xABCDABCD));
    _sg_imgui_draw_buffer_list(ctx);
    igSameLine(0,-1);
    _sg_imgui_draw_buffer_panel(ctx, ctx->buffers.sel_buf);
}

SOKOL_API_IMPL void sg_imgui_draw_images_content(sg_imgui_t* ctx) {
    SOKOL_ASSERT(ctx && (ctx->init_tag == 0xABCDABCD));
    _sg_imgui_draw_image_list(ctx);
    igSameLine(0,-1);
    _sg_imgui_draw_image_panel(ctx, ctx->images.sel_img);
}

SOKOL_API_IMPL void sg_imgui_draw_shaders_content(sg_imgui_t* ctx) {
    SOKOL_ASSERT(ctx && (ctx->init_tag == 0xABCDABCD));
    _sg_imgui_draw_shader_list(ctx);
    igSameLine(0,-1);
    _sg_imgui_draw_shader_panel(ctx, ctx->shaders.sel_shd);
}

SOKOL_API_IMPL void sg_imgui_draw_pipelines_content(sg_imgui_t* ctx) {
    SOKOL_ASSERT(ctx && (ctx->init_tag == 0xABCDABCD));
    _sg_imgui_draw_pipeline_list(ctx);
    igSameLine(0,-1);
    _sg_imgui_draw_pipeline_panel(ctx, ctx->pipelines.sel_pip);
}

SOKOL_API_IMPL void sg_imgui_draw_passes_content(sg_imgui_t* ctx) {
    SOKOL_ASSERT(ctx && (ctx->init_tag == 0xABCDABCD));
    _sg_imgui_draw_pass_list(ctx);
    igSameLine(0,-1);
    _sg_imgui_draw_pass_panel(ctx, ctx->passes.sel_pass);
}

SOKOL_API_IMPL void sg_imgui_draw_capture_content(sg_imgui_t* ctx) {
    SOKOL_ASSERT(ctx && (ctx->init_tag == 0xABCDABCD));
    _sg_imgui_draw_capture_list(ctx);
    igSameLine(0,-1);
    _sg_imgui_draw_capture_panel(ctx);
}

SOKOL_API_IMPL void sg_imgui_draw_capabilities_content(sg_imgui_t* ctx) {
    SOKOL_ASSERT(ctx && (ctx->init_tag == 0xABCDABCD));
    _sg_imgui_draw_caps_panel(ctx);
}

#endif /* SOKOL_GFX_IMGUI_IMPL */
