#pragma once
/*
    sokol_gfx_imgui.h -- debug-inspection UI for sokol_gfx.h using Dear ImGui

    This is an extension header for sokol_gfx.h, this means sokol_gfx_imgui.h
    must be included *after* sokol_gfx.h both for the implementation
    and declaration.

    You also need to include imgui.h before including the implementation
    of sokol_gfx_imgui.h.

    The implementation must be compiled as C++ or Objective-C++, the
    declaration can be included in plain C sources.
    
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
    sg_shader_desc desc;
} sg_imgui_shader_t;

typedef struct {
    sg_pipeline res_id;
    sg_imgui_str_t label;
    sg_imgui_str_t attr_name[SG_MAX_VERTEX_ATTRIBUTES];
    sg_imgui_str_t attr_sem_name[SG_MAX_VERTEX_ATTRIBUTES];
    sg_pipeline_desc desc;
} sg_imgui_pipeline_t;

typedef struct {
    sg_pass res_id;
    sg_imgui_str_t label;
    float color_image_scale[SG_MAX_COLOR_ATTACHMENTS];
    float ds_image_scale;
} sg_imgui_pass_t;

typedef struct {
    bool open;
    int num_slots;
    uint32_t sel_id;
    sg_imgui_buffer_t* slots;
} sg_imgui_buffers_t;

typedef struct {
    bool open;
    int num_slots;
    uint32_t sel_id;
    sg_imgui_image_t* slots;
} sg_imgui_images_t;

typedef struct {
    bool open;
    int num_slots;
    uint32_t sel_id;
    sg_imgui_shader_t* slots;
} sg_imgui_shaders_t;

typedef struct {
    bool open;
    int num_slots;
    uint32_t sel_id;
    sg_imgui_pipeline_t* slots;
} sg_imgui_pipelines_t;

typedef struct {
    bool open;
    int num_slots;
    uint32_t sel_id;
    sg_imgui_pass_t* slots;
} sg_imgui_passes_t;

typedef enum {
    SG_IMGUI_CMD_INVALID,
    SG_IMGUI_CMD_QUERY_FEATURE,
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
    SG_IMGUI_CMD_QUERY_BUFFER_OVERFLOW,
    SG_IMGUI_CMD_QUERY_BUFFER_STATE,
    SG_IMGUI_CMD_QUERY_IMAGE_STATE,
    SG_IMGUI_CMD_QUERY_SHADER_STATE,
    SG_IMGUI_CMD_QUERY_PIPELINE_STATE,
    SG_IMGUI_CMD_QUERY_PASS_STATE,
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
    sg_feature feature;
    bool result;
} sg_imgui_args_query_feature_t;

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
    sg_buffer buffer;
    bool result;
} sg_imgui_args_query_buffer_overflow_t;

typedef struct {
    sg_buffer buffer;
    sg_resource_state result;
} sg_imgui_args_query_buffer_state_t;

typedef struct {
    sg_image image;
    sg_resource_state result;
} sg_imgui_args_query_image_state_t;

typedef struct {
    sg_shader shader;
    sg_resource_state result;
} sg_imgui_args_query_shader_state_t;

typedef struct {
    sg_pipeline pipeline;
    sg_resource_state result;
} sg_imgui_args_query_pipeline_state_t;

typedef struct {
    sg_pass pass;
    sg_resource_state result;
} sg_imgui_args_query_pass_state_t;

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
    sg_imgui_args_query_feature_t query_feature;
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
    sg_imgui_args_query_buffer_overflow_t query_buffer_overflow;
    sg_imgui_args_query_buffer_state_t query_buffer_state;
    sg_imgui_args_query_image_state_t query_image_state;
    sg_imgui_args_query_shader_state_t query_shader_state;
    sg_imgui_args_query_pipeline_state_t query_pipeline_state;
    sg_imgui_args_query_pass_state_t query_pass_state;
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
    uint32_t init_tag;
    sg_imgui_buffers_t buffers;
    sg_imgui_images_t images;
    sg_imgui_shaders_t shaders;
    sg_imgui_pipelines_t pipelines;
    sg_imgui_passes_t passes;
    sg_imgui_capture_t capture;
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

SOKOL_API_DECL void sg_imgui_draw_buffers_window(sg_imgui_t* ctx);
SOKOL_API_DECL void sg_imgui_draw_images_window(sg_imgui_t* ctx);
SOKOL_API_DECL void sg_imgui_draw_shaders_window(sg_imgui_t* ctx);
SOKOL_API_DECL void sg_imgui_draw_pipelines_window(sg_imgui_t* ctx);
SOKOL_API_DECL void sg_imgui_draw_passes_window(sg_imgui_t* ctx);
SOKOL_API_DECL void sg_imgui_draw_capture_window(sg_imgui_t* ctx);

#if defined(__cplusplus)
} /* extern "C" */
#endif

/*=== IMPLEMENTATION =========================================================*/
#if defined SOKOL_IMPL
#include <string.h>
#include <stdio.h>      /* snprintf */

#define _SG_IMGUI_LIST_WIDTH (192)
#define _SG_IMGUI_COLOR_OTHER ImColor(0.75f, 0.75f, 0.75f, 1.0f)
#define _SG_IMGUI_COLOR_RSRC ImColor(1.0f, 1.0f, 0.0f, 1.0f)
#define _SG_IMGUI_COLOR_DRAW ImColor(0.0f, 1.0f, 0.0f, 1.0f)
#define _SG_IMGUI_COLOR_ERR ImColor(1.0f, 0.5f, 0.5f, 1.0f)

/*--- UTILS ------------------------------------------------------------------*/
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
        strncpy(dst->buf, src, SG_IMGUI_STRBUF_LEN);
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
    char* dst = (char*) _sg_imgui_alloc(strlen(src) + 1);
    strcpy(dst, src);
    return (const char*) dst;
}

_SOKOL_PRIVATE const uint8_t* _sg_imgui_bin_dup(const uint8_t* src, int num_bytes) {
    SOKOL_ASSERT(src && (num_bytes > 0));
    uint8_t* dst = (uint8_t*) _sg_imgui_alloc(num_bytes);
    memcpy(dst, src, num_bytes);
    return (const uint8_t*) dst;
}

/*--- STRING CONVERSION ------------------------------------------------------*/
_SOKOL_PRIVATE const char* _sg_imgui_feature_string(sg_feature f) {
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

_SOKOL_PRIVATE const char* _sg_imgui_resourcestate_string(sg_resource_state s) {
    switch (s) {
        case SG_RESOURCESTATE_INITIAL:  return "SG_RESOURCESTATE_INITIAL";
        case SG_RESOURCESTATE_ALLOC:    return "SG_RESOURCESTATE_ALLOC";
        case SG_RESOURCESTATE_VALID:    return "SG_RESOURCESTATE_VALID";
        case SG_RESOURCESTATE_FAILED:   return "SG_RESOURCESTATE_FAILED";
        default:                        return "SG_RESOURCESTATE_INVALID";
    }
}

_SOKOL_PRIVATE void _sg_imgui_draw_resource_slot(const _sg_slot_t* slot) {
    ImGui::Text("ResId: %08X", slot->id);
    ImGui::Text("CtxId: %08X", slot->ctx_id);
    ImGui::Text("State: %s", _sg_imgui_resourcestate_string(slot->state));
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
        case SG_WRAP_MIRRORED_REPEAT:   return "SG_WRAP_MIRRORED_REPEAT";
        default:                        return "???";
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
        snprintf(res.buf, sizeof(res.buf), "'%s'", label);
    }
    else {
        snprintf(res.buf, sizeof(res.buf), "0x%08X", res_id);
    }
    return res;
}

_SOKOL_PRIVATE sg_imgui_str_t _sg_imgui_buffer_id_string(sg_imgui_t* ctx, sg_buffer buf_id) {
    if (buf_id.id != SG_INVALID_ID) {
        const sg_imgui_buffer_t* buf_ui = &ctx->buffers.slots[_sg_slot_index(buf_id.id)];
        return _sg_imgui_res_id_string(buf_id.id, buf_ui->label.buf);
    }
    else {
        return _sg_imgui_make_str("<invalid>");
    }
}

_SOKOL_PRIVATE sg_imgui_str_t _sg_imgui_image_id_string(sg_imgui_t* ctx, sg_image img_id) {
    if (img_id.id != SG_INVALID_ID) {
        const sg_imgui_image_t* img_ui = &ctx->images.slots[_sg_slot_index(img_id.id)];
        return _sg_imgui_res_id_string(img_id.id, img_ui->label.buf);
    }
    else {
        return _sg_imgui_make_str("<invalid>");
    }
}

_SOKOL_PRIVATE sg_imgui_str_t _sg_imgui_shader_id_string(sg_imgui_t* ctx, sg_shader shd_id) {
    if (shd_id.id != SG_INVALID_ID) {
        const sg_imgui_shader_t* shd_ui = &ctx->shaders.slots[_sg_slot_index(shd_id.id)];
        return _sg_imgui_res_id_string(shd_id.id, shd_ui->label.buf);
    }
    else {
        return _sg_imgui_make_str("<invalid>");
    }
}

_SOKOL_PRIVATE sg_imgui_str_t _sg_imgui_pipeline_id_string(sg_imgui_t* ctx, sg_pipeline pip_id) {
    if (pip_id.id != SG_INVALID_ID) {
        const sg_imgui_pipeline_t* pip_ui = &ctx->pipelines.slots[_sg_slot_index(pip_id.id)];
        return _sg_imgui_res_id_string(pip_id.id, pip_ui->label.buf);
    }
    else {
        return _sg_imgui_make_str("<invalid>");
    }
}

_SOKOL_PRIVATE sg_imgui_str_t _sg_imgui_pass_id_string(sg_imgui_t* ctx, sg_pass pass_id) {
    if (pass_id.id != SG_INVALID_ID) {
        const sg_imgui_pass_t* pass_ui = &ctx->passes.slots[_sg_slot_index(pass_id.id)];
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

    /* resolve default state */
    buf->desc.type = _sg_def(buf->desc.type, SG_BUFFERTYPE_VERTEXBUFFER);
    buf->desc.usage = _sg_def(buf->desc.usage, SG_USAGE_IMMUTABLE);
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

    /* resolve default state */
    img->desc.type = _sg_def(img->desc.type, SG_IMAGETYPE_2D);
    img->desc.depth = _sg_def(img->desc.depth, 1);
    img->desc.num_mipmaps = _sg_def(img->desc.num_mipmaps, 1);
    img->desc.usage = _sg_def(img->desc.usage, SG_USAGE_IMMUTABLE);
    img->desc.pixel_format = _sg_def(img->desc.pixel_format, SG_PIXELFORMAT_RGBA8);
    img->desc.sample_count = _sg_def(img->desc.sample_count, 1);
    img->desc.min_filter = _sg_def(img->desc.min_filter, SG_FILTER_NEAREST);
    img->desc.mag_filter = _sg_def(img->desc.mag_filter, SG_FILTER_NEAREST);
    img->desc.wrap_u = _sg_def(img->desc.wrap_u, SG_WRAP_REPEAT);
    img->desc.wrap_v = _sg_def(img->desc.wrap_v, SG_WRAP_REPEAT);
    img->desc.wrap_w = _sg_def(img->desc.wrap_w, SG_WRAP_REPEAT);
    img->desc.max_anisotropy = _sg_def(img->desc.max_anisotropy, 1);
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

    /* copy strings in vertex layout to persistent location */
    for (int i = 0; i < SG_MAX_VERTEX_ATTRIBUTES; i++) {
        sg_vertex_attr_desc* ad = &pip->desc.layout.attrs[i];
        if (ad->name) {
            pip->attr_name[i] = _sg_imgui_make_str(ad->name);
            ad->name = pip->attr_name[i].buf;
        }
        if (ad->sem_name) {
            pip->attr_sem_name[i] = _sg_imgui_make_str(ad->sem_name);
            ad->sem_name = pip->attr_sem_name[i].buf;
        }
    }
    
    /* need to resolve default values */
    sg_depth_stencil_state* ds = &pip->desc.depth_stencil;
    sg_stencil_state* dsf = &ds->stencil_front;
    sg_stencil_state* dsb = &ds->stencil_back;
    sg_blend_state* blend = &pip->desc.blend;
    sg_rasterizer_state* rs = &pip->desc.rasterizer;
    pip->desc.primitive_type = _sg_def(desc->primitive_type, SG_PRIMITIVETYPE_TRIANGLES);
    pip->desc.index_type = _sg_def(desc->index_type, SG_INDEXTYPE_NONE);
    dsf->fail_op = _sg_def(dsf->fail_op, SG_STENCILOP_KEEP);
    dsf->depth_fail_op = _sg_def(dsf->depth_fail_op, SG_STENCILOP_KEEP);
    dsf->pass_op = _sg_def(dsf->pass_op, SG_STENCILOP_KEEP);
    dsf->compare_func = _sg_def(dsf->compare_func, SG_COMPAREFUNC_ALWAYS);
    dsb->fail_op = _sg_def(dsb->fail_op, SG_STENCILOP_KEEP);
    dsb->depth_fail_op = _sg_def(dsb->depth_fail_op, SG_STENCILOP_KEEP);
    dsb->pass_op = _sg_def(dsb->pass_op, SG_STENCILOP_KEEP);
    dsb->compare_func = _sg_def(dsb->compare_func, SG_COMPAREFUNC_ALWAYS);
    ds->depth_compare_func = _sg_def(ds->depth_compare_func, SG_COMPAREFUNC_ALWAYS);
    blend->src_factor_rgb = _sg_def(blend->src_factor_rgb, SG_BLENDFACTOR_ONE);
    blend->dst_factor_rgb = _sg_def(blend->dst_factor_rgb, SG_BLENDFACTOR_ZERO);
    blend->op_rgb = _sg_def(blend->op_rgb, SG_BLENDOP_ADD);
    blend->src_factor_alpha = _sg_def(blend->src_factor_alpha, SG_BLENDFACTOR_ONE);
    blend->dst_factor_alpha = _sg_def(blend->dst_factor_alpha, SG_BLENDFACTOR_ZERO);
    blend->op_alpha = _sg_def(blend->op_alpha, SG_BLENDOP_ADD);
    if (blend->color_write_mask == SG_COLORMASK_NONE) {
        blend->color_write_mask = 0;
    }
    else {
        blend->color_write_mask = (uint8_t) _sg_def((sg_color_mask)blend->color_write_mask, SG_COLORMASK_RGBA);
    }
    blend->color_attachment_count = _sg_def(blend->color_attachment_count, 1);
    blend->color_format = _sg_def(blend->color_format, SG_PIXELFORMAT_RGBA8);
    blend->depth_format = _sg_def(blend->depth_format, SG_PIXELFORMAT_DEPTHSTENCIL);
    rs->cull_mode = _sg_def(rs->cull_mode, SG_CULLMODE_NONE);
    rs->face_winding = _sg_def(rs->face_winding, SG_FACEWINDING_CW);
    rs->sample_count = _sg_def(rs->sample_count, 1);
    
    /* resolve vertex layout strides and offsets */
    int auto_offset[SG_MAX_SHADERSTAGE_BUFFERS];
    for (int layout_index = 0; layout_index < SG_MAX_SHADERSTAGE_BUFFERS; layout_index++) {
        auto_offset[layout_index] = 0;
    }
    bool use_auto_offset = true;
    for (int attr_index = 0; attr_index < SG_MAX_VERTEX_ATTRIBUTES; attr_index++) {
        /* to use computed offsets, *all* attr offsets must be 0 */
        if (pip->desc.layout.attrs[attr_index].offset != 0) {
            use_auto_offset = false;
        }
    }
    for (int attr_index = 0; attr_index < SG_MAX_VERTEX_ATTRIBUTES; attr_index++) {
        sg_vertex_attr_desc* a_desc = &pip->desc.layout.attrs[attr_index];
        if (a_desc->format == SG_VERTEXFORMAT_INVALID) {
            break;
        }
        SOKOL_ASSERT((a_desc->buffer_index >= 0) && (a_desc->buffer_index < SG_MAX_SHADERSTAGE_BUFFERS));
        if (use_auto_offset) {
            a_desc->offset = auto_offset[a_desc->buffer_index];
        }
        auto_offset[a_desc->buffer_index] += _sg_vertexformat_bytesize(a_desc->format);
    }
    /* compute vertex strides if needed, and default-resolve step_func and rate */
    for (int buf_index = 0; buf_index < SG_MAX_SHADERSTAGE_BUFFERS; buf_index++) {
        sg_buffer_layout_desc* l_desc = &pip->desc.layout.buffers[buf_index];
        l_desc->step_func = _sg_def(l_desc->step_func, SG_VERTEXSTEP_PER_VERTEX);
        l_desc->step_rate = _sg_def(l_desc->step_rate, 1);
        if (l_desc->stride == 0) {
            l_desc->stride = auto_offset[buf_index];
        }
    }
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
    const int len = sizeof(str.buf);
    switch (item->cmd) {
        case SG_IMGUI_CMD_QUERY_FEATURE: 
            snprintf(str.buf, len, "%d: sg_query_feature(feature=%s) => %s",
                index,
                _sg_imgui_feature_string(item->args.query_feature.feature),
                _sg_imgui_bool_string(item->args.query_feature.result));
            break;
        
        case SG_IMGUI_CMD_RESET_STATE_CACHE:
            snprintf(str.buf, len, "%d: sg_reset_state_cache()", index);
            break;

        case SG_IMGUI_CMD_MAKE_BUFFER:
            res_id = _sg_imgui_buffer_id_string(ctx, item->args.make_buffer.result);
            snprintf(str.buf, len, "%d: sg_make_buffer(desc=..) => %s", index, res_id.buf);
            break;

        case SG_IMGUI_CMD_MAKE_IMAGE:
            res_id = _sg_imgui_image_id_string(ctx, item->args.make_image.result);
            snprintf(str.buf, len, "%d: sg_make_image(desc=..) => %s", index, res_id.buf);
            break;

        case SG_IMGUI_CMD_MAKE_SHADER:
            res_id = _sg_imgui_shader_id_string(ctx, item->args.make_shader.result);
            snprintf(str.buf, len, "%d: sg_make_shader(desc=..) => %s", index, res_id.buf);
            break;

        case SG_IMGUI_CMD_MAKE_PIPELINE:
            res_id = _sg_imgui_pipeline_id_string(ctx, item->args.make_pipeline.result);
            snprintf(str.buf, len, "%d: sg_make_pipeline(desc=..) => %s", index, res_id.buf);
            break;

        case SG_IMGUI_CMD_MAKE_PASS:
            res_id = _sg_imgui_pass_id_string(ctx, item->args.make_pass.result);
            snprintf(str.buf, len, "%d: sg_make_pass(desc=..) => %s", index, res_id.buf);
            break;

        case SG_IMGUI_CMD_DESTROY_BUFFER:
            res_id = _sg_imgui_buffer_id_string(ctx, item->args.destroy_buffer.buffer);
            snprintf(str.buf, len, "%d: sg_destroy_buffer(buf=%s)", index, res_id.buf);
            break;

        case SG_IMGUI_CMD_DESTROY_IMAGE:
            res_id = _sg_imgui_image_id_string(ctx, item->args.destroy_image.image);
            snprintf(str.buf, len, "%d: sg_destroy_image(img=%s)", index, res_id.buf);
            break;

        case SG_IMGUI_CMD_DESTROY_SHADER:
            res_id = _sg_imgui_shader_id_string(ctx, item->args.destroy_shader.shader);
            snprintf(str.buf, len, "%d: sg_destroy_shader(shd=%s)", index, res_id.buf);
            break;

        case SG_IMGUI_CMD_DESTROY_PIPELINE:
            res_id = _sg_imgui_pipeline_id_string(ctx, item->args.destroy_pipeline.pipeline);
            snprintf(str.buf, len, "%d: sg_destroy_pipeline(pip=%s)", index, res_id.buf);
            break;

        case SG_IMGUI_CMD_DESTROY_PASS:
            res_id = _sg_imgui_pass_id_string(ctx, item->args.destroy_pass.pass);
            snprintf(str.buf, len, "%d: sg_destroy_pass(pass=%s)", index, res_id.buf);
            break;

        case SG_IMGUI_CMD_UPDATE_BUFFER:
            res_id = _sg_imgui_buffer_id_string(ctx, item->args.update_buffer.buffer);
            snprintf(str.buf, len, "%d: sg_update_buffer(buf=%s, data_ptr=.., data_size=%d)", 
                index, res_id.buf,
                item->args.update_buffer.data_size);
            break;

        case SG_IMGUI_CMD_UPDATE_IMAGE:
            res_id = _sg_imgui_image_id_string(ctx, item->args.update_image.image);
            snprintf(str.buf, len, "%d: sg_update_image(img=%s, data=..)", index, res_id.buf);
            break;

        case SG_IMGUI_CMD_APPEND_BUFFER:
            res_id = _sg_imgui_buffer_id_string(ctx, item->args.append_buffer.buffer);
            snprintf(str.buf, len, "%d: sg_append_buffer(buf=%s, data_ptr=.., data_size=%d) => %d", 
                index, res_id.buf,
                item->args.append_buffer.data_size,
                item->args.append_buffer.result);
            break;

        case SG_IMGUI_CMD_QUERY_BUFFER_OVERFLOW:
            res_id = _sg_imgui_buffer_id_string(ctx, item->args.query_buffer_overflow.buffer);
            snprintf(str.buf, len, "%d: sg_query_buffer_overflow(buf=%s) => %s", 
                index, res_id.buf,
                _sg_imgui_bool_string(item->args.query_buffer_overflow.result));
            break;

        case SG_IMGUI_CMD_QUERY_BUFFER_STATE:
            res_id = _sg_imgui_buffer_id_string(ctx, item->args.query_buffer_state.buffer);
            snprintf(str.buf, len, "%d: sg_query_buffer_state(buf=%s) => %s",
                index, res_id.buf,
                _sg_imgui_resourcestate_string(item->args.query_buffer_state.result));
            break;

        case SG_IMGUI_CMD_QUERY_IMAGE_STATE:
            res_id = _sg_imgui_image_id_string(ctx, item->args.query_image_state.image);
            snprintf(str.buf, len, "%d: sg_query_image_state(img=%s) => %s",
                index, res_id.buf,
                _sg_imgui_resourcestate_string(item->args.query_image_state.result));
            break;

        case SG_IMGUI_CMD_QUERY_SHADER_STATE:
            res_id = _sg_imgui_shader_id_string(ctx, item->args.query_shader_state.shader);
            snprintf(str.buf, len, "%d: sg_query_shader_state(shd=%s) => %s",
                index, res_id.buf,
                _sg_imgui_resourcestate_string(item->args.query_shader_state.result));
            break;

        case SG_IMGUI_CMD_QUERY_PIPELINE_STATE:
            res_id = _sg_imgui_pipeline_id_string(ctx, item->args.query_pipeline_state.pipeline);
            snprintf(str.buf, len, "%d: sg_query_pipeline_state(pip=%s) => %s",
                index, res_id.buf,
                _sg_imgui_resourcestate_string(item->args.query_pipeline_state.result));
            break;

        case SG_IMGUI_CMD_QUERY_PASS_STATE:
            res_id = _sg_imgui_pass_id_string(ctx, item->args.query_pass_state.pass);
            snprintf(str.buf, len, "%d: sg_query_pass_state(pass=%s) => %s",
                index, res_id.buf,
                _sg_imgui_resourcestate_string(item->args.query_pass_state.result));
            break;

        case SG_IMGUI_CMD_BEGIN_DEFAULT_PASS:
            snprintf(str.buf, len, "%d: sg_begin_default_pass(pass_action=.., width=%d, height=%d)",
                index,
                item->args.begin_default_pass.width,
                item->args.begin_default_pass.height);
            break;

        case SG_IMGUI_CMD_BEGIN_PASS:
            res_id = _sg_imgui_pass_id_string(ctx, item->args.begin_pass.pass);
            snprintf(str.buf, len, "%d: sg_begin_pass(pass=%s, pass_action=..)", index, res_id.buf);
            break;

        case SG_IMGUI_CMD_APPLY_VIEWPORT:
            snprintf(str.buf, len, "%d: sg_apply_viewport(x=%d, y=%d, width=%d, height=%d, origin_top_left=%s)",
                index,
                item->args.apply_viewport.x,
                item->args.apply_viewport.y,
                item->args.apply_viewport.width,
                item->args.apply_viewport.height,
                _sg_imgui_bool_string(item->args.apply_viewport.origin_top_left));
            break;

        case SG_IMGUI_CMD_APPLY_SCISSOR_RECT:
            snprintf(str.buf, len, "%d: sg_apply_scissor_rect(x=%d, y=%d, width=%d, height=%d, origin_top_left=%s)",
                index,
                item->args.apply_scissor_rect.x,
                item->args.apply_scissor_rect.y,
                item->args.apply_scissor_rect.width,
                item->args.apply_scissor_rect.height,
                _sg_imgui_bool_string(item->args.apply_scissor_rect.origin_top_left));
            break;

        case SG_IMGUI_CMD_APPLY_PIPELINE:
            res_id = _sg_imgui_pipeline_id_string(ctx, item->args.apply_pipeline.pipeline);
            snprintf(str.buf, len, "%d: sg_apply_pipeline(pip=%s)", index, res_id.buf);
            break;

        case SG_IMGUI_CMD_APPLY_BINDINGS:
            snprintf(str.buf, len, "%d: sg_apply_bindings(bindings=..)", index);
            break;

        case SG_IMGUI_CMD_APPLY_UNIFORMS:
            snprintf(str.buf, len, "%d: sg_apply_uniforms(stage=%s, ub_index=%d, data=.., num_bytes=%d)",
                index,
                _sg_imgui_shaderstage_string(item->args.apply_uniforms.stage),
                item->args.apply_uniforms.ub_index,
                item->args.apply_uniforms.num_bytes);
            break;

        case SG_IMGUI_CMD_DRAW:
            snprintf(str.buf, len, "%d: sg_draw(base_element=%d, num_elements=%d, num_instances=%d)",
                index,
                item->args.draw.base_element,
                item->args.draw.num_elements,
                item->args.draw.num_instances);
            break;

        case SG_IMGUI_CMD_END_PASS:
            snprintf(str.buf, len, "%d: sg_end_pass()", index);
            break;

        case SG_IMGUI_CMD_COMMIT:
            snprintf(str.buf, len, "%d: sg_commit()", index);
            break;

        case SG_IMGUI_CMD_ALLOC_BUFFER:
            res_id = _sg_imgui_buffer_id_string(ctx, item->args.alloc_buffer.result);
            snprintf(str.buf, len, "%d: sg_alloc_buffer() => %s", index, res_id.buf);
            break;

        case SG_IMGUI_CMD_ALLOC_IMAGE:
            res_id = _sg_imgui_image_id_string(ctx, item->args.alloc_image.result);
            snprintf(str.buf, len, "%d: sg_alloc_image() => %s", index, res_id.buf);
            break;

        case SG_IMGUI_CMD_ALLOC_SHADER:
            res_id = _sg_imgui_shader_id_string(ctx, item->args.alloc_shader.result);
            snprintf(str.buf, len, "%d: sg_alloc_shader() => %s", index, res_id.buf);
            break;

        case SG_IMGUI_CMD_ALLOC_PIPELINE:
            res_id = _sg_imgui_pipeline_id_string(ctx, item->args.alloc_pipeline.result);
            snprintf(str.buf, len, "%d: sg_alloc_pipeline() => %s", index, res_id.buf);
            break;

        case SG_IMGUI_CMD_ALLOC_PASS:
            res_id = _sg_imgui_pass_id_string(ctx, item->args.alloc_pass.result);
            snprintf(str.buf, len, "%d: sg_alloc_pass() => %s", index, res_id.buf);
            break;

        case SG_IMGUI_CMD_INIT_BUFFER:
            res_id = _sg_imgui_buffer_id_string(ctx, item->args.init_buffer.buffer);
            snprintf(str.buf, len, "%d: sg_init_buffer(buf=%s, desc=..)", index, res_id.buf);
            break;

        case SG_IMGUI_CMD_INIT_IMAGE:
            res_id = _sg_imgui_image_id_string(ctx, item->args.init_image.image);
            snprintf(str.buf, len, "%d: sg_init_image(img=%s, desc=..)", index, res_id.buf);
            break;

        case SG_IMGUI_CMD_INIT_SHADER:
            res_id = _sg_imgui_shader_id_string(ctx, item->args.init_shader.shader);
            snprintf(str.buf, len, "%d: sg_init_shader(shd=%s, desc=..)", index, res_id.buf);
            break;

        case SG_IMGUI_CMD_INIT_PIPELINE:
            res_id = _sg_imgui_pipeline_id_string(ctx, item->args.init_pipeline.pipeline);
            snprintf(str.buf, len, "%d: sg_init_pipeline(pip=%s, desc=..)", index, res_id.buf);
            break;

        case SG_IMGUI_CMD_INIT_PASS:
            res_id = _sg_imgui_pass_id_string(ctx, item->args.init_pass.pass);
            snprintf(str.buf, len, "%d: sg_init_pass(pass=%s, desc=..)", index, res_id.buf);
            break;

        case SG_IMGUI_CMD_FAIL_BUFFER:
            res_id = _sg_imgui_buffer_id_string(ctx, item->args.fail_buffer.buffer);
            snprintf(str.buf, len, "%d: sg_fail_buffer(buf=%s)", index, res_id.buf);
            break;

        case SG_IMGUI_CMD_FAIL_IMAGE:
            res_id = _sg_imgui_image_id_string(ctx, item->args.fail_image.image);
            snprintf(str.buf, len, "%d: sg_fail_image(img=%s)", index, res_id.buf);
            break;

        case SG_IMGUI_CMD_FAIL_SHADER:
            res_id = _sg_imgui_shader_id_string(ctx, item->args.fail_shader.shader);
            snprintf(str.buf, len, "%d: sg_fail_shader(shd=%s)", index, res_id.buf);
            break;

        case SG_IMGUI_CMD_FAIL_PIPELINE:
            res_id = _sg_imgui_pipeline_id_string(ctx, item->args.fail_pipeline.pipeline);
            snprintf(str.buf, len, "%d: sg_fail_pipeline(shd=%s)", index, res_id.buf);
            break;

        case SG_IMGUI_CMD_FAIL_PASS:
            res_id = _sg_imgui_pass_id_string(ctx, item->args.fail_pass.pass);
            snprintf(str.buf, len, "%d: sg_fail_pass(pass=%s)", index, res_id.buf);
            break;

        case SG_IMGUI_CMD_PUSH_DEBUG_GROUP:
            snprintf(str.buf, len, "%d: sg_push_debug_group(name=%s)", index,
                item->args.push_debug_group.name.buf);
            break;

        case SG_IMGUI_CMD_POP_DEBUG_GROUP:
            snprintf(str.buf, len, "%d: sg_pop_debug_group()", index);
            break;

        case SG_IMGUI_CMD_ERR_BUFFER_POOL_EXHAUSTED:
            snprintf(str.buf, len, "%d: sg_err_buffer_pool_exhausted()", index);
            break;

        case SG_IMGUI_CMD_ERR_IMAGE_POOL_EXHAUSTED:
            snprintf(str.buf, len, "%d: sg_err_image_pool_exhausted()", index);
            break;

        case SG_IMGUI_CMD_ERR_SHADER_POOL_EXHAUSTED:
            snprintf(str.buf, len, "%d: sg_err_shader_pool_exhausted()", index);
            break;

        case SG_IMGUI_CMD_ERR_PIPELINE_POOL_EXHAUSTED:
            snprintf(str.buf, len, "%d: sg_err_pipeline_pool_exhausted()", index);
            break;

        case SG_IMGUI_CMD_ERR_PASS_POOL_EXHAUSTED:
            snprintf(str.buf, len, "%d: sg_err_pass_pool_exhausted()", index);
            break;

        case SG_IMGUI_CMD_ERR_CONTEXT_MISMATCH:
            snprintf(str.buf, len, "%d: sg_err_context_mismatch()", index);
            break;

        case SG_IMGUI_CMD_ERR_PASS_INVALID:
            snprintf(str.buf, len, "%d: sg_err_pass_invalid()", index);
            break;

        case SG_IMGUI_CMD_ERR_DRAW_INVALID:
            snprintf(str.buf, len, "%d: sg_err_draw_invalid()", index);
            break;

        case SG_IMGUI_CMD_ERR_BINDINGS_INVALID:
            snprintf(str.buf, len, "%d: sg_err_bindings_invalid()", index);
            break;

        default:
            snprintf(str.buf, len, "%d: ???", index);
            break;
    }
    str.buf[len-1] = 0;
    return str;
}

/*--- CAPTURE CALLBACKS ------------------------------------------------------*/
_SOKOL_PRIVATE void _sg_imgui_query_feature(sg_feature feature, bool result, void* user_data) {
    sg_imgui_t* ctx = (sg_imgui_t*) user_data;
    SOKOL_ASSERT(ctx);
    sg_imgui_capture_item_t* item = _sg_imgui_capture_next_write_item(ctx);
    if (item) {
        item->cmd = SG_IMGUI_CMD_QUERY_FEATURE;
        item->color = _SG_IMGUI_COLOR_OTHER;
        item->args.query_feature.feature = feature;
        item->args.query_feature.result = result;
    }
    if (ctx->hooks.query_feature) {
        ctx->hooks.query_feature(feature, result, ctx->hooks.user_data);
    }
}

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
        _sg_imgui_buffer_created(ctx, buf_id, _sg_slot_index(buf_id.id), desc);
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
        _sg_imgui_image_created(ctx, img_id, _sg_slot_index(img_id.id), desc);
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
        _sg_imgui_shader_created(ctx, shd_id, _sg_slot_index(shd_id.id), desc);
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
        _sg_imgui_pipeline_created(ctx, pip_id, _sg_slot_index(pip_id.id), desc);
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
        _sg_imgui_pass_created(ctx, pass_id, _sg_slot_index(pass_id.id), desc);
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
        _sg_imgui_buffer_destroyed(ctx, _sg_slot_index(buf.id));
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
        _sg_imgui_image_destroyed(ctx, _sg_slot_index(img.id));
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
        _sg_imgui_shader_destroyed(ctx, _sg_slot_index(shd.id));
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
        _sg_imgui_pipeline_destroyed(ctx, _sg_slot_index(pip.id));
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
        _sg_imgui_pass_destroyed(ctx, _sg_slot_index(pass.id));
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

_SOKOL_PRIVATE void _sg_imgui_query_buffer_overflow(sg_buffer buf, bool result, void* user_data) {
    sg_imgui_t* ctx = (sg_imgui_t*) user_data;
    SOKOL_ASSERT(ctx);
    sg_imgui_capture_item_t* item = _sg_imgui_capture_next_write_item(ctx);
    if (item) {
        item->cmd = SG_IMGUI_CMD_QUERY_BUFFER_OVERFLOW;
        item->color = _SG_IMGUI_COLOR_RSRC;
        item->args.query_buffer_overflow.buffer = buf;
        item->args.query_buffer_overflow.result = result;
    }
    if (ctx->hooks.query_buffer_overflow) {
        ctx->hooks.query_buffer_overflow(buf, result, ctx->hooks.user_data);
    }
}

_SOKOL_PRIVATE void _sg_imgui_query_buffer_state(sg_buffer buf, sg_resource_state result, void* user_data) {
    sg_imgui_t* ctx = (sg_imgui_t*) user_data;
    SOKOL_ASSERT(ctx);
    sg_imgui_capture_item_t* item = _sg_imgui_capture_next_write_item(ctx);
    if (item) {
        item->cmd = SG_IMGUI_CMD_QUERY_BUFFER_STATE;
        item->color = _SG_IMGUI_COLOR_RSRC;
        item->args.query_buffer_state.buffer = buf;
        item->args.query_buffer_state.result = result;
    }
    if (ctx->hooks.query_buffer_state) {
        ctx->hooks.query_buffer_state(buf, result, ctx->hooks.user_data);
    }
}

_SOKOL_PRIVATE void _sg_imgui_query_image_state(sg_image img, sg_resource_state result, void* user_data) {
    sg_imgui_t* ctx = (sg_imgui_t*) user_data;
    SOKOL_ASSERT(ctx);
    sg_imgui_capture_item_t* item = _sg_imgui_capture_next_write_item(ctx);
    if (item) {
        item->cmd = SG_IMGUI_CMD_QUERY_IMAGE_STATE;
        item->color = _SG_IMGUI_COLOR_RSRC;
        item->args.query_image_state.image = img;
        item->args.query_image_state.result = result;
    }
    if (ctx->hooks.query_image_state) {
        ctx->hooks.query_image_state(img, result, ctx->hooks.user_data);
    }
}

_SOKOL_PRIVATE void _sg_imgui_query_shader_state(sg_shader shd, sg_resource_state result, void* user_data) {
    sg_imgui_t* ctx = (sg_imgui_t*) user_data;
    SOKOL_ASSERT(ctx);
    sg_imgui_capture_item_t* item = _sg_imgui_capture_next_write_item(ctx);
    if (item) {
        item->cmd = SG_IMGUI_CMD_QUERY_SHADER_STATE;
        item->color = _SG_IMGUI_COLOR_RSRC;
        item->args.query_shader_state.shader = shd;
        item->args.query_shader_state.result = result;
    }
    if (ctx->hooks.query_shader_state) {
        ctx->hooks.query_shader_state(shd, result, ctx->hooks.user_data);
    }
}

_SOKOL_PRIVATE void _sg_imgui_query_pipeline_state(sg_pipeline pip, sg_resource_state result, void* user_data) {
    sg_imgui_t* ctx = (sg_imgui_t*) user_data;
    SOKOL_ASSERT(ctx);
    sg_imgui_capture_item_t* item = _sg_imgui_capture_next_write_item(ctx);
    if (item) {
        item->cmd = SG_IMGUI_CMD_QUERY_PIPELINE_STATE;
        item->color = _SG_IMGUI_COLOR_RSRC;
        item->args.query_pipeline_state.pipeline = pip;
        item->args.query_pipeline_state.result = result;
    }
    if (ctx->hooks.query_pipeline_state) {
        ctx->hooks.query_pipeline_state(pip, result, ctx->hooks.user_data);
    }
}

_SOKOL_PRIVATE void _sg_imgui_query_pass_state(sg_pass pass, sg_resource_state result, void* user_data) {
    sg_imgui_t* ctx = (sg_imgui_t*) user_data;
    SOKOL_ASSERT(ctx);
    sg_imgui_capture_item_t* item = _sg_imgui_capture_next_write_item(ctx);
    if (item) {
        item->cmd = SG_IMGUI_CMD_QUERY_PASS_STATE;
        item->color = _SG_IMGUI_COLOR_RSRC;
        item->args.query_pass_state.pass = pass;
        item->args.query_pass_state.result = result;
    }
    if (ctx->hooks.query_pass_state) {
        ctx->hooks.query_pass_state(pass, result, ctx->hooks.user_data);
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
        args->pipeline = _sg.cur_pipeline;
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
        _sg_imgui_buffer_created(ctx, buf_id, _sg_slot_index(buf_id.id), desc);
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
        _sg_imgui_image_created(ctx, img_id, _sg_slot_index(img_id.id), desc);
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
        _sg_imgui_shader_created(ctx, shd_id, _sg_slot_index(shd_id.id), desc);
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
        _sg_imgui_pipeline_created(ctx, pip_id, _sg_slot_index(pip_id.id), desc);
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
        _sg_imgui_pass_created(ctx, pass_id, _sg_slot_index(pass_id.id), desc);
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
    ImGui::PushID((int)res_id);
    bool res;
    if (label[0]) {
        res = ImGui::Selectable(label, selected);
    }
    else {
        char buf[32];
        snprintf(buf, sizeof(buf), "0x%08X", res_id);
        res = ImGui::Selectable(buf, selected);
    }
    ImGui::PopID();
    return res;
}

_SOKOL_PRIVATE bool _sg_imgui_draw_resid_link(uint32_t res_id, const char* label) {
    SOKOL_ASSERT(label);
    char buf[32];
    const char* str;
    if (label[0]) {
        str = label;
    }
    else {
        snprintf(buf, sizeof(buf), "0x%08X", res_id);
        str = buf;
    }
    ImGui::PushID((int)res_id);
    bool res = ImGui::SmallButton(str);
    ImGui::PopID();
    return res;
}

_SOKOL_PRIVATE bool _sg_imgui_draw_buffer_link(sg_imgui_t* ctx, uint32_t buf_id) {
    bool retval = false;
    if (buf_id != SG_INVALID_ID) {
        const sg_imgui_buffer_t* buf_ui = &ctx->buffers.slots[_sg_slot_index(buf_id)];
        retval = _sg_imgui_draw_resid_link(buf_id, buf_ui->label.buf);
    }
    return retval;
}

_SOKOL_PRIVATE bool _sg_imgui_draw_image_link(sg_imgui_t* ctx, uint32_t img_id) {
    bool retval = false;
    if (img_id != SG_INVALID_ID) {
        const sg_imgui_image_t* img_ui = &ctx->images.slots[_sg_slot_index(img_id)];
        retval = _sg_imgui_draw_resid_link(img_id, img_ui->label.buf);
    }
    return retval;
}

_SOKOL_PRIVATE bool _sg_imgui_draw_shader_link(sg_imgui_t* ctx, uint32_t shd_id) {
    bool retval = false;
    if (shd_id != SG_INVALID_ID) {
        const sg_imgui_shader_t* shd_ui = &ctx->shaders.slots[_sg_slot_index(shd_id)];
        retval = _sg_imgui_draw_resid_link(shd_id, shd_ui->label.buf);
    }
    return retval;
}

_SOKOL_PRIVATE void _sg_imgui_show_buffer(sg_imgui_t* ctx, uint32_t buf_id) {
    ctx->buffers.open = true;
    ctx->buffers.sel_id = buf_id;
}

_SOKOL_PRIVATE void _sg_imgui_show_image(sg_imgui_t* ctx, uint32_t img_id) {
    ctx->images.open = true;
    ctx->images.sel_id = img_id;
}

_SOKOL_PRIVATE void _sg_imgui_show_shader(sg_imgui_t* ctx, uint32_t shd_id) {
    ctx->shaders.open = true;
    ctx->shaders.sel_id = shd_id;
}

_SOKOL_PRIVATE void _sg_imgui_draw_buffer_list(sg_imgui_t* ctx) {
    ImGui::BeginChild("buffer_list", ImVec2(_SG_IMGUI_LIST_WIDTH,0), true);
    for (int i = 1; i < _sg.pools.buffer_pool.size; i++) {
        const _sg_buffer_t* buf = &_sg.pools.buffers[i];
        if (buf->slot.state != SG_RESOURCESTATE_INITIAL) {
            bool selected = ctx->buffers.sel_id == buf->slot.id;
            if (_sg_imgui_draw_resid_list_item(buf->slot.id, ctx->buffers.slots[i].label.buf, selected)) {
                ctx->buffers.sel_id = buf->slot.id;
            }
        }
    }
    ImGui::EndChild();
}

_SOKOL_PRIVATE void _sg_imgui_draw_image_list(sg_imgui_t* ctx) {
    ImGui::BeginChild("image_list", ImVec2(_SG_IMGUI_LIST_WIDTH,0), true);
    for (int i = 1; i < _sg.pools.image_pool.size; i++) {
        const _sg_image_t* img = &_sg.pools.images[i];
        if (img->slot.state != SG_RESOURCESTATE_INITIAL) {
            bool selected = ctx->images.sel_id == img->slot.id;
            if (_sg_imgui_draw_resid_list_item(img->slot.id, ctx->images.slots[i].label.buf, selected)) {
                ctx->images.sel_id = img->slot.id;
            }
        }
    }
    ImGui::EndChild();
}

_SOKOL_PRIVATE void _sg_imgui_draw_shader_list(sg_imgui_t* ctx) {
    ImGui::BeginChild("shader_list", ImVec2(_SG_IMGUI_LIST_WIDTH,0), true);
    for (int i = 1; i < _sg.pools.shader_pool.size; i++) {
        const _sg_shader_t* shd = &_sg.pools.shaders[i];
        if (shd->slot.state != SG_RESOURCESTATE_INITIAL) {
            bool selected = ctx->shaders.sel_id == shd->slot.id;
            if (_sg_imgui_draw_resid_list_item(shd->slot.id, ctx->shaders.slots[i].label.buf, selected)) {
                ctx->shaders.sel_id = shd->slot.id;
            }
        }
    }
    ImGui::EndChild();
}

_SOKOL_PRIVATE void _sg_imgui_draw_pipeline_list(sg_imgui_t* ctx) {
    ImGui::BeginChild("pipeline_list", ImVec2(_SG_IMGUI_LIST_WIDTH,0), true);
    for (int i = 1; i < _sg.pools.pipeline_pool.size; i++) {
        const _sg_pipeline_t* pip = &_sg.pools.pipelines[i];
        if (pip->slot.state != SG_RESOURCESTATE_INITIAL) {
            bool selected = ctx->pipelines.sel_id == pip->slot.id;
            if (_sg_imgui_draw_resid_list_item(pip->slot.id, ctx->pipelines.slots[i].label.buf, selected)) {
                ctx->pipelines.sel_id = pip->slot.id;
            }
        }
    }
    ImGui::EndChild();
}

_SOKOL_PRIVATE void _sg_imgui_draw_pass_list(sg_imgui_t* ctx) {
    ImGui::BeginChild("pass_list", ImVec2(_SG_IMGUI_LIST_WIDTH,0), true);
    for (int i = 1; i < _sg.pools.pass_pool.size; i++) {
        const _sg_pass_t* pass = &_sg.pools.passes[i];
        if (pass->slot.state != SG_RESOURCESTATE_INITIAL) {
            bool selected = ctx->passes.sel_id == pass->slot.id;
            if (_sg_imgui_draw_resid_list_item(pass->slot.id, ctx->passes.slots[i].label.buf, selected)) {
                ctx->passes.sel_id = pass->slot.id;
            }
        }
    }
    ImGui::EndChild();
}

_SOKOL_PRIVATE void _sg_imgui_draw_capture_list(sg_imgui_t* ctx) {
    ImGui::BeginChild("capture_list", ImVec2(_SG_IMGUI_LIST_WIDTH,0), true);
    const uint32_t num_items = _sg_imgui_capture_num_read_items(ctx);
    uint64_t group_stack = 1;   /* bit set: group unfolded, cleared: folded */
    for (uint32_t i = 0; i < num_items; i++) {
        const sg_imgui_capture_item_t* item = _sg_imgui_capture_read_item_at(ctx, i);
        sg_imgui_str_t item_string = _sg_imgui_capture_item_string(ctx, i, item);
        ImGui::PushStyleColor(ImGuiCol_Text, item->color);
        if (item->cmd == SG_IMGUI_CMD_PUSH_DEBUG_GROUP) {
            if (group_stack & 1) {
                group_stack <<= 1;
                const char* group_name = item->args.push_debug_group.name.buf;
                if (ImGui::TreeNode(group_name, "Group: %s", group_name)) {
                    group_stack |= 1;
                }
            }
            else {
                group_stack <<= 1;
            }
        }
        else if (item->cmd == SG_IMGUI_CMD_POP_DEBUG_GROUP) {
            if (group_stack & 1) {
                ImGui::TreePop();
            }
            group_stack >>= 1;
        }
        else if (group_stack & 1) {
            ImGui::PushID(i);
            if (ImGui::Selectable(item_string.buf, ctx->capture.sel_item == i)) {
                ctx->capture.sel_item = i;
            }
            if (ImGui::IsItemHovered()) {
                ImGui::SetTooltip("%s", item_string.buf);
            }
            ImGui::PopID();
        }
        ImGui::PopStyleColor();
    }
    ImGui::EndChild();
}

_SOKOL_PRIVATE void _sg_imgui_draw_buffer_panel(sg_imgui_t* ctx, uint32_t buf_id) {
    if (buf_id != SG_INVALID_ID) {
        ImGui::BeginChild("buffer", ImVec2(0,0), false);
        const _sg_buffer_t* buf = _sg_lookup_buffer(&_sg.pools, buf_id);
        if (buf) {
            const sg_imgui_buffer_t* buf_ui = &ctx->buffers.slots[_sg_slot_index(buf_id)];
            ImGui::Text("Label: %s", buf_ui->label.buf[0] ? buf_ui->label.buf : "---");
            _sg_imgui_draw_resource_slot(&buf->slot);
            ImGui::Separator();
            ImGui::Text("Type:  %s", _sg_imgui_buffertype_string(buf_ui->desc.type));
            ImGui::Text("Usage: %s", _sg_imgui_usage_string(buf_ui->desc.usage));
            ImGui::Text("Size:  %d", buf_ui->desc.size);
            if (buf_ui->desc.usage != SG_USAGE_IMMUTABLE) {
                ImGui::Separator();
                ImGui::Text("Num Slots:     %d", buf->num_slots);
                ImGui::Text("Active Slot:/s   %d", buf->active_slot);
                ImGui::Text("Update Frame Index: %d", buf->update_frame_index);
                ImGui::Text("Append Frame Index: %d", buf->append_frame_index);
                ImGui::Text("Append Pos:         %d", buf->append_pos);
                ImGui::Text("Append Overflow:    %s", buf->append_overflow ? "YES":"NO");
            }
        }
        else {
            ImGui::Text("Buffer 0x%08X no longer alive", buf_id);
        }
        ImGui::EndChild();
    }
}

_SOKOL_PRIVATE void _sg_imgui_draw_embedded_image(sg_imgui_t* ctx, uint32_t img_id, float* scale) {
    const _sg_image_t* img = _sg_lookup_image(&_sg.pools, img_id);
    if (img) {
        if ((SG_IMAGETYPE_2D == img->type) && !_sg_is_valid_rendertarget_depth_format(img->pixel_format)) {
            ImGui::PushID((int)img_id);
            ImGui::SliderFloat("Scale", scale, 0.125f, 8.0f, "%.3f", 2.0f);
            float w = (float)img->width * (*scale);
            float h = (float)img->height * (*scale);
            ImGui::Image((ImTextureID)(intptr_t)img_id, ImVec2(w, h));
            ImGui::PopID();
        }
        else {
            ImGui::Text("Image not renderable.");
        }
    }
    else {
        ImGui::Text("Image 0x%08X no longer alive", img_id);
    }
}

_SOKOL_PRIVATE void _sg_imgui_draw_image_panel(sg_imgui_t* ctx, uint32_t img_id) {
    if (img_id != SG_INVALID_ID) {
        ImGui::BeginChild("image", ImVec2(0,0), false);
        const _sg_image_t* img = _sg_lookup_image(&_sg.pools, img_id);
        if (img) {
            sg_imgui_image_t* img_ui = &ctx->images.slots[_sg_slot_index(img_id)];
            const sg_image_desc* desc = &img_ui->desc;
            ImGui::Text("Label: %s", img_ui->label.buf[0] ? img_ui->label.buf : "---");
            _sg_imgui_draw_resource_slot(&img->slot);
            ImGui::Separator();
            _sg_imgui_draw_embedded_image(ctx, img_id, &img_ui->ui_scale);
            ImGui::Separator();
            ImGui::Text("Type:              %s", _sg_imgui_imagetype_string(desc->type));
            ImGui::Text("Usage:             %s", _sg_imgui_usage_string(desc->usage));
            ImGui::Text("Render Target:     %s", desc->render_target ? "YES":"NO");
            ImGui::Text("Width:             %d", desc->width);
            ImGui::Text("Height:            %d", desc->height);
            ImGui::Text("Depth:             %d", desc->depth);
            ImGui::Text("Num Mipmaps:       %d", desc->num_mipmaps);
            ImGui::Text("Pixel Format:      %s", _sg_imgui_pixelformat_string(desc->pixel_format));
            ImGui::Text("Sample Count:      %d", desc->sample_count);
            ImGui::Text("Min Filter:        %s", _sg_imgui_filter_string(desc->min_filter));
            ImGui::Text("Mag Filter:        %s", _sg_imgui_filter_string(desc->mag_filter));
            ImGui::Text("Wrap U:            %s", _sg_imgui_wrap_string(desc->wrap_u));
            ImGui::Text("Wrap V:            %s", _sg_imgui_wrap_string(desc->wrap_v));
            ImGui::Text("Wrap W:            %s", _sg_imgui_wrap_string(desc->wrap_w));
            ImGui::Text("Max Anisotropy:    %d", desc->max_anisotropy);
            ImGui::Text("Min LOD:           %.3f", desc->min_lod);
            ImGui::Text("Max LOD:           %.3f", desc->max_lod);
            if (img->usage != SG_USAGE_IMMUTABLE) {
                ImGui::Separator();
                ImGui::Text("Num Slots:     %d", img->num_slots);
                ImGui::Text("Active Slot:   %d", img->active_slot);
                ImGui::Text("Update Frame Index: %d", img->upd_frame_index);
            }
        }
        else {
            ImGui::Text("Image 0x%08X no longer alive", img_id);
        }
        ImGui::EndChild();
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
        if (ImGui::TreeNode("Uniform Blocks")) {
            for (int i = 0; i < num_valid_ubs; i++) {
                ImGui::Text("#%d:", i);
                const sg_shader_uniform_block_desc* ub = &stage->uniform_blocks[i];
                for (int j = 0; j < SG_MAX_UB_MEMBERS; j++) {
                    const sg_shader_uniform_desc* u = &ub->uniforms[j];
                    if (SG_UNIFORMTYPE_INVALID != u->type) {
                        if (u->array_count == 0) {
                            ImGui::Text("  %s %s", _sg_imgui_uniformtype_string(u->type), u->name ? u->name : "");
                        }
                        else {
                            ImGui::Text("  %s[%d] %s", _sg_imgui_uniformtype_string(u->type), u->array_count, u->name ? u->name : "");
                        }
                    }
                }
            }
            ImGui::TreePop();
        }
    }
    if (num_valid_images > 0) {
        if (ImGui::TreeNode("Images")) {
            for (int i = 0; i < SG_MAX_SHADERSTAGE_IMAGES; i++) {
                const sg_shader_image_desc* sid = &stage->images[i];
                if (sid->type != _SG_IMAGETYPE_DEFAULT) {
                    ImGui::Text("%s %s", _sg_imgui_imagetype_string(sid->type), sid->name ? sid->name : "");
                }
                else {
                    break;
                }
            }
            ImGui::TreePop();
        }
    }
    if (stage->entry) {
        ImGui::Text("Entry: %s", stage->entry);
    }
    if (stage->source) {
        if (ImGui::TreeNode("Source")) {
            ImGui::Text("%s", stage->source);
            ImGui::TreePop();
        }
    }
    else if (stage->byte_code) {
        if (ImGui::TreeNode("Byte Code")) {
            ImGui::Text("Byte-code display currently not supported.");
        }
    }
}

_SOKOL_PRIVATE void _sg_imgui_draw_shader_panel(sg_imgui_t* ctx, uint32_t shd_id) {
    if (shd_id != SG_INVALID_ID) {
        ImGui::BeginChild("shader", ImVec2(0,0), false, ImGuiWindowFlags_HorizontalScrollbar);
        const _sg_shader_t* shd = _sg_lookup_shader(&_sg.pools, shd_id);
        if (shd) {
            const sg_imgui_shader_t* shd_ui = &ctx->shaders.slots[_sg_slot_index(shd_id)];
            ImGui::Text("Label: %s", shd_ui->label.buf[0] ? shd_ui->label.buf : "---");
            _sg_imgui_draw_resource_slot(&shd->slot);
            ImGui::Separator();
            if (ImGui::TreeNode("Vertex Shader Stage")) {
                _sg_imgui_draw_shader_stage(ctx, &shd_ui->desc.vs);
                ImGui::TreePop();
            }
            if (ImGui::TreeNode("Fragment Shader Stage")) {
                _sg_imgui_draw_shader_stage(ctx, &shd_ui->desc.fs);
                ImGui::TreePop();
            }
        }
        else {
            ImGui::Text("Shader 0x%08X no longer alive", shd_id);
        }
        ImGui::EndChild();
    }
}

_SOKOL_PRIVATE void _sg_imgui_draw_vertex_layout(const sg_layout_desc* layout) {
    if (ImGui::TreeNode("Buffers")) {
        for (int i = 0; i < SG_MAX_SHADERSTAGE_BUFFERS; i++) {
            const sg_buffer_layout_desc* l_desc = &layout->buffers[i];
            if (l_desc->stride > 0) {
                ImGui::Text("#%d:", i);
                ImGui::Text("  Stride:    %d", l_desc->stride);
                ImGui::Text("  Step Func: %s", _sg_imgui_vertexstep_string(l_desc->step_func));
                ImGui::Text("  Step Rate: %d", l_desc->step_rate);
            }
        }
        ImGui::TreePop();
    }
    if (ImGui::TreeNode("Attrs")) {
        for (int i = 0; i < SG_MAX_VERTEX_ATTRIBUTES; i++) {
            const sg_vertex_attr_desc* a_desc = &layout->attrs[i];
            if (a_desc->format != SG_VERTEXFORMAT_INVALID) {
                ImGui::Text("#%d:", i);
                ImGui::Text("  Format:       %s", _sg_imgui_vertexformat_string(a_desc->format));
                ImGui::Text("  Name:         %s", a_desc->name ? a_desc->name : "---");
                ImGui::Text("  Sem Name:     %s", a_desc->sem_name ? a_desc->sem_name : "---");
                ImGui::Text("  Sem Index:    %d", a_desc->sem_index);
                ImGui::Text("  Offset:       %d", a_desc->offset);
                ImGui::Text("  Buffer Index: %d", a_desc->buffer_index);
            }
        }
        ImGui::TreePop();
    }
}

_SOKOL_PRIVATE void _sg_imgui_draw_stencil_state(const sg_stencil_state* ss) {
    ImGui::Text("Fail Op:       %s", _sg_imgui_stencilop_string(ss->fail_op));
    ImGui::Text("Depth Fail Op: %s", _sg_imgui_stencilop_string(ss->depth_fail_op));
    ImGui::Text("Pass Op:       %s", _sg_imgui_stencilop_string(ss->pass_op));
    ImGui::Text("Compare Func:  %s", _sg_imgui_comparefunc_string(ss->compare_func));
}

_SOKOL_PRIVATE void _sg_imgui_draw_depth_stencil_state(const sg_depth_stencil_state* dss) {
    ImGui::Text("Depth Compare Func:  %s", _sg_imgui_comparefunc_string(dss->depth_compare_func));
    ImGui::Text("Depth Write Enabled: %s", dss->depth_write_enabled ? "YES":"NO");
    ImGui::Text("Stencil Enabled:     %s", dss->stencil_enabled ? "YES":"NO");
    ImGui::Text("Stencil Read Mask:   0x%02X", dss->stencil_read_mask);
    ImGui::Text("Stencil Write Mask:  0x%02X", dss->stencil_write_mask);
    ImGui::Text("Stencil Ref:         0x%02X", dss->stencil_ref);
    if (ImGui::TreeNode("Stencil Front")) {
        _sg_imgui_draw_stencil_state(&dss->stencil_front);
        ImGui::TreePop();
    }
    if (ImGui::TreeNode("Stencil Back")) {
        _sg_imgui_draw_stencil_state(&dss->stencil_back);
        ImGui::TreePop();
    }
}

_SOKOL_PRIVATE void _sg_imgui_draw_blend_state(const sg_blend_state* bs) {
    ImGui::Text("Blend Enabled:    %s", bs->enabled ? "YES":"NO");
    ImGui::Text("Src Factor RGB:   %s", _sg_imgui_blendfactor_string(bs->src_factor_rgb));
    ImGui::Text("Dst Factor RGB:   %s", _sg_imgui_blendfactor_string(bs->dst_factor_rgb));
    ImGui::Text("Op RGB:           %s", _sg_imgui_blendop_string(bs->op_rgb));
    ImGui::Text("Src Factor Alpha: %s", _sg_imgui_blendfactor_string(bs->src_factor_alpha));
    ImGui::Text("Dst Factor Alpha: %s", _sg_imgui_blendfactor_string(bs->dst_factor_alpha));
    ImGui::Text("Op Alpha:         %s", _sg_imgui_blendop_string(bs->op_alpha));
    ImGui::Text("Color Write Mask: %s", _sg_imgui_colormask_string(bs->color_write_mask));
    ImGui::Text("Attachment Count: %d", bs->color_attachment_count);
    ImGui::Text("Color Format:     %s", _sg_imgui_pixelformat_string(bs->color_format));
    ImGui::Text("Depth Format:     %s", _sg_imgui_pixelformat_string(bs->depth_format));
    ImGui::Text("Blend Color:      %.3f %.3f %.3f %.3f", bs->blend_color[0], bs->blend_color[1], bs->blend_color[2], bs->blend_color[3]);
}

_SOKOL_PRIVATE void _sg_imgui_draw_rasterizer_state(const sg_rasterizer_state* rs) {
    ImGui::Text("Alpha to Coverage: %s", rs->alpha_to_coverage_enabled ? "YES":"NO");
    ImGui::Text("Cull Mode:         %s", _sg_imgui_cullmode_string(rs->cull_mode));
    ImGui::Text("Face Winding:      %s", _sg_imgui_facewinding_string(rs->face_winding));
    ImGui::Text("Sample Count:      %d", rs->sample_count);
    ImGui::Text("Depth Bias:        %f", rs->depth_bias);
    ImGui::Text("Depth Bias Slope:  %f", rs->depth_bias_slope_scale);
    ImGui::Text("Depth Bias Clamp:  %f", rs->depth_bias_clamp);
}

_SOKOL_PRIVATE void _sg_imgui_draw_pipeline_panel(sg_imgui_t* ctx, uint32_t pip_id) {
    if (pip_id != SG_INVALID_ID) {
        ImGui::BeginChild("pipeline", ImVec2(0,0), false);
        const _sg_pipeline_t* pip = _sg_lookup_pipeline(&_sg.pools, pip_id);
        if (pip) {
            const sg_imgui_pipeline_t* pip_ui = &ctx->pipelines.slots[_sg_slot_index(pip_id)];
            ImGui::Text("Label: %s", pip_ui->label.buf[0] ? pip_ui->label.buf : "---");
            _sg_imgui_draw_resource_slot(&pip->slot);
            ImGui::Separator();
            ImGui::Text("Shader:    "); ImGui::SameLine();
            if (_sg_imgui_draw_shader_link(ctx, pip->shader_id.id)) {
                _sg_imgui_show_shader(ctx, pip->shader_id.id);
            }
            ImGui::Text("Prim Type:  %s", _sg_imgui_primitivetype_string(pip_ui->desc.primitive_type));
            ImGui::Text("Index Type: %s", _sg_imgui_indextype_string(pip_ui->desc.index_type));
            if (ImGui::TreeNode("Vertex Layout")) {
                _sg_imgui_draw_vertex_layout(&pip_ui->desc.layout);
                ImGui::TreePop();
            } 
            if (ImGui::TreeNode("Depth Stencil State")) {
                _sg_imgui_draw_depth_stencil_state(&pip_ui->desc.depth_stencil);
                ImGui::TreePop();
            }
            if (ImGui::TreeNode("Blend State")) {
                _sg_imgui_draw_blend_state(&pip_ui->desc.blend);
                ImGui::TreePop();
            }
            if (ImGui::TreeNode("Rasterizer State")) {
                _sg_imgui_draw_rasterizer_state(&pip_ui->desc.rasterizer);
                ImGui::TreePop();
            }
        }
        else {
            ImGui::Text("Pipeline 0x%08X no longer alive.", pip_id);
        }
        ImGui::EndChild();
    }
}

_SOKOL_PRIVATE void _sg_imgui_draw_attachment(sg_imgui_t* ctx, const _sg_attachment_t* att, float* img_scale) {
    SOKOL_ASSERT(att->image && (att->image->slot.id == att->image_id.id));
    ImGui::Text("  Image: "); ImGui::SameLine();
    if (_sg_imgui_draw_image_link(ctx, att->image_id.id)) {
        _sg_imgui_show_image(ctx, att->image_id.id);
    }
    ImGui::Text("  Mip Level: %d", att->mip_level);
    ImGui::Text("  Slice: %d", att->slice);
    _sg_imgui_draw_embedded_image(ctx, att->image_id.id, img_scale);
}

_SOKOL_PRIVATE void _sg_imgui_draw_pass_panel(sg_imgui_t* ctx, uint32_t pass_id) {
    if (pass_id != SG_INVALID_ID) {
        ImGui::BeginChild("pass", ImVec2(0,0), false);
        const _sg_pass_t* pass = _sg_lookup_pass(&_sg.pools, pass_id);
        if (pass) {
            sg_imgui_pass_t* pass_ui = &ctx->passes.slots[_sg_slot_index(pass_id)];
            ImGui::Text("Label: %s", pass_ui->label.buf[0] ? pass_ui->label.buf : "---");
            _sg_imgui_draw_resource_slot(&pass->slot);
            for (int i = 0; i < pass->num_color_atts; i++) {
                ImGui::Separator();
                ImGui::Text("Color Attachment #%d:", i);
                _sg_imgui_draw_attachment(ctx, &pass->color_atts[i], &pass_ui->color_image_scale[i]);
            }
            if (pass->ds_att.image_id.id != SG_INVALID_ID) {
                ImGui::Separator();
                ImGui::Text("Depth-Stencil Attachemnt:");
                _sg_imgui_draw_attachment(ctx, &pass->ds_att, &pass_ui->ds_image_scale);
            }
        }
        else {
            ImGui::Text("Pass 0x%08X no longer alive.", pass_id);
        }
        ImGui::EndChild();
    }
}

_SOKOL_PRIVATE void _sg_imgui_draw_bindings_panel(sg_imgui_t* ctx, const sg_bindings* bnd) {
    for (int i = 0; i < SG_MAX_SHADERSTAGE_BUFFERS; i++) {
        uint32_t buf_id = bnd->vertex_buffers[i].id;
        if (buf_id != SG_INVALID_ID) {
            ImGui::Separator();
            ImGui::Text("Vertex Buffer Slot #%d:", i);
            ImGui::Text("  Buffer: "); ImGui::SameLine();
            if (_sg_imgui_draw_buffer_link(ctx, buf_id)) {
                _sg_imgui_show_buffer(ctx, buf_id);
            }
            ImGui::Text("  Offset: %d", bnd->vertex_buffer_offsets[i]);
        }
        else {
            break;
        }
    }
    if (bnd->index_buffer.id != SG_INVALID_ID) {
        uint32_t buf_id = bnd->index_buffer.id;
        if (buf_id != SG_INVALID_ID) {
            ImGui::Separator();
            ImGui::Text("Index Buffer Slot:");
            ImGui::Text("  Buffer: "); ImGui::SameLine();
            if (_sg_imgui_draw_buffer_link(ctx, buf_id)) {
                _sg_imgui_show_buffer(ctx, buf_id);
            }
            ImGui::Text("  Offset: %d", bnd->index_buffer_offset);
        }
    }
    for (int i = 0; i < SG_MAX_SHADERSTAGE_IMAGES; i++) {
        uint32_t img_id = bnd->vs_images[i].id;
        if (img_id != SG_INVALID_ID) {
            ImGui::Separator();
            ImGui::Text("Vertex Stage Image Slot #%d:", i);
            ImGui::Text("  Image: "); ImGui::SameLine();
            if (_sg_imgui_draw_image_link(ctx, img_id)) {
                _sg_imgui_show_image(ctx, img_id);
            }
        }
        else {
            break;
        }
    }
    for (int i = 0; i < SG_MAX_SHADERSTAGE_IMAGES; i++) {
        uint32_t img_id = bnd->fs_images[i].id;
        if (img_id != SG_INVALID_ID) {
            ImGui::Separator();
            ImGui::Text("Fragment Stage Image Slot #%d:", i);
            ImGui::Text("  Image: "); ImGui::SameLine();
            if (_sg_imgui_draw_image_link(ctx, img_id)) {
                _sg_imgui_show_image(ctx, img_id);
            }
        }
    }
}

_SOKOL_PRIVATE void _sg_imgui_draw_uniforms_panel(sg_imgui_t* ctx, const sg_imgui_args_apply_uniforms_t* args) {
    SOKOL_ASSERT(args->ub_index < SG_MAX_SHADERSTAGE_BUFFERS);

    /* check if all the required information for drawing the structured uniform block content
        is available, otherwise just render a generic hexdump
    */
    bool draw_dump = false;
    _sg_pipeline_t* pip = _sg_lookup_pipeline(&_sg.pools, args->pipeline.id);
    if (!pip) {
        ImGui::Text("Pipeline object no longer alive!");
        draw_dump = true;
    }
    _sg_shader_t* shd = _sg_lookup_shader(&_sg.pools, pip->shader_id.id);
    if (!shd) {
        ImGui::Text("Shader object no longer alive!");
        draw_dump = true;
    }
    const sg_imgui_shader_t* shd_ui = &ctx->shaders.slots[_sg_slot_index(pip->shader_id.id)];
    SOKOL_ASSERT(shd_ui->res_id.id == pip->shader_id.id);
    const sg_shader_uniform_block_desc* ub_desc = (args->stage == SG_SHADERSTAGE_VS) ?
        ub_desc = &shd_ui->desc.vs.uniform_blocks[args->ub_index] :
        ub_desc = &shd_ui->desc.fs.uniform_blocks[args->ub_index];
    SOKOL_ASSERT(args->num_bytes <= ub_desc->size);
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
                ImGui::Text("%d: %s %s[%d] =", i, _sg_imgui_uniformtype_string(ud->type), ud->name?ud->name:"", ud->array_count);
            }
            else {
                ImGui::Text("%d: %s %s =", i, _sg_imgui_uniformtype_string(ud->type), ud->name?ud->name:"");
            }
            for (int i = 0; i < num_items; i++) {
                switch (ud->type) {
                    case SG_UNIFORMTYPE_FLOAT:
                        ImGui::Text("    %.3f", *uptrf);
                        break;
                    case SG_UNIFORMTYPE_FLOAT2:
                        ImGui::Text("    %.3f, %.3f", uptrf[0], uptrf[1]);
                        break;
                    case SG_UNIFORMTYPE_FLOAT3:
                        ImGui::Text("    %.3f, %.3f, %.3f", uptrf[0], uptrf[1], uptrf[2]);
                        break;
                    case SG_UNIFORMTYPE_FLOAT4:
                        ImGui::Text("    %.3f, %.3f, %.3f, %.3f", uptrf[0], uptrf[1], uptrf[2], uptrf[3]);
                        break;
                    case SG_UNIFORMTYPE_MAT4:
                        ImGui::Text("    %.3f, %.3f, %.3f, %.3f\n"
                                    "    %.3f, %.3f, %.3f, %.3f\n"
                                    "    %.3f, %.3f, %.3f, %.3f\n"
                                    "    %.3f, %.3f, %.3f, %.3f",
                            uptrf[0],  uptrf[1],  uptrf[2],  uptrf[3],
                            uptrf[4],  uptrf[5],  uptrf[6],  uptrf[7],
                            uptrf[8],  uptrf[9],  uptrf[10], uptrf[11],
                            uptrf[12], uptrf[13], uptrf[14], uptrf[15]);
                        break;
                    default:
                        ImGui::Text("???");
                        break;
                }
                uptrf += _sg_uniform_size(ud->type, 1) / sizeof(float);
            }
        }
    }
    else {
        const uint32_t num_floats = ub_desc->size / sizeof(float);
        for (uint32_t i = 0; i < num_floats; i++) {
            ImGui::Text("%.3f, ", uptrf[i]);
            if (((i + 1) % 4) != 0) {
                ImGui::SameLine();
            }
        }
    }
}

_SOKOL_PRIVATE void _sg_imgui_draw_passaction_panel(sg_imgui_t* ctx, uint32_t pass_id, const sg_pass_action* action) {
    int num_color_atts = 1;
    if (SG_INVALID_ID != pass_id) {
        const _sg_pass_t* pass = _sg_lookup_pass(&_sg.pools, pass_id);
        if (pass) {
            num_color_atts = pass->num_color_atts;
        }
    }

    ImGui::Text("Pass Action: ");
    for (int i = 0; i < num_color_atts; i++) {
        const sg_color_attachment_action* c_att = &action->colors[i];
        ImGui::Text("  Color Attachment %d:", i);
        switch (c_att->action) {
            case SG_ACTION_LOAD: ImGui::Text("    SG_ACTION_LOAD"); break;
            case SG_ACTION_DONTCARE: ImGui::Text("    SG_ACTION_DONTCARE"); break;
            default:
                ImGui::Text("    SG_ACTION_CLEAR: %.3f, %.3f, %.3f, %.3f",
                    c_att->val[0],
                    c_att->val[1],
                    c_att->val[2],
                    c_att->val[3]);
                break;
        }
    }
    const sg_depth_attachment_action* d_att = &action->depth;
    ImGui::Text("  Depth Attachment:");
    switch (d_att->action) {
        case SG_ACTION_LOAD: ImGui::Text("    SG_ACTION_LOAD"); break;
        case SG_ACTION_DONTCARE: ImGui::Text("    SG_ACTION_DONTCARE"); break;
        default: ImGui::Text("    SG_ACTION_CLEAR: %.3f", d_att->val); break;
    }
    const sg_stencil_attachment_action* s_att = &action->stencil;
    ImGui::Text("  Stencil Attachment");
    switch (s_att->action) {
        case SG_ACTION_LOAD: ImGui::Text("    SG_ACTION_LOAD"); break;
        case SG_ACTION_DONTCARE: ImGui::Text("    SG_ACTION_DONTCARE"); break;
        default: ImGui::Text("    SG_ACTION_CLEAR: 0x%02X", s_att->val); break;
    }
}

_SOKOL_PRIVATE void _sg_imgui_draw_capture_panel(sg_imgui_t* ctx) {
    uint32_t sel_item_index = ctx->capture.sel_item;
    if (sel_item_index >= _sg_imgui_capture_num_read_items(ctx)) {
        return;
    }
    sg_imgui_capture_item_t* item = _sg_imgui_capture_read_item_at(ctx, sel_item_index);
    ImGui::BeginChild("capture_item", ImVec2(0, 0), false);
    ImGui::PushStyleColor(ImGuiCol_Text, item->color);
    ImGui::Text("%s", _sg_imgui_capture_item_string(ctx, sel_item_index, item).buf);
    ImGui::PopStyleColor();
    ImGui::Separator();
    switch (item->cmd) {
        case SG_IMGUI_CMD_QUERY_FEATURE:
            break;
        case SG_IMGUI_CMD_RESET_STATE_CACHE:
            break;
        case SG_IMGUI_CMD_MAKE_BUFFER:
            _sg_imgui_draw_buffer_panel(ctx, item->args.make_buffer.result.id);
            break;
        case SG_IMGUI_CMD_MAKE_IMAGE:
            _sg_imgui_draw_image_panel(ctx, item->args.make_image.result.id);
            break;
        case SG_IMGUI_CMD_MAKE_SHADER:
            _sg_imgui_draw_shader_panel(ctx, item->args.make_shader.result.id);
            break;
        case SG_IMGUI_CMD_MAKE_PIPELINE:
            _sg_imgui_draw_pipeline_panel(ctx, item->args.make_pipeline.result.id);
            break;
        case SG_IMGUI_CMD_MAKE_PASS:
            _sg_imgui_draw_pass_panel(ctx, item->args.make_pass.result.id);
            break;
        case SG_IMGUI_CMD_DESTROY_BUFFER:
            _sg_imgui_draw_buffer_panel(ctx, item->args.destroy_buffer.buffer.id);
            break;
        case SG_IMGUI_CMD_DESTROY_IMAGE:
            _sg_imgui_draw_image_panel(ctx, item->args.destroy_image.image.id);
            break;
        case SG_IMGUI_CMD_DESTROY_SHADER:
            _sg_imgui_draw_shader_panel(ctx, item->args.destroy_shader.shader.id);
            break;
        case SG_IMGUI_CMD_DESTROY_PIPELINE:
            _sg_imgui_draw_pipeline_panel(ctx, item->args.destroy_pipeline.pipeline.id);
            break;
        case SG_IMGUI_CMD_DESTROY_PASS:
            _sg_imgui_draw_pass_panel(ctx, item->args.destroy_pass.pass.id);
            break;
        case SG_IMGUI_CMD_UPDATE_BUFFER:
            _sg_imgui_draw_buffer_panel(ctx, item->args.update_buffer.buffer.id);
            break;
        case SG_IMGUI_CMD_UPDATE_IMAGE:
            _sg_imgui_draw_image_panel(ctx, item->args.update_image.image.id);
            break;
        case SG_IMGUI_CMD_APPEND_BUFFER:
            _sg_imgui_draw_buffer_panel(ctx, item->args.update_buffer.buffer.id);
            break;
        case SG_IMGUI_CMD_QUERY_BUFFER_OVERFLOW:
            _sg_imgui_draw_buffer_panel(ctx, item->args.query_buffer_overflow.buffer.id);
            break;
        case SG_IMGUI_CMD_QUERY_BUFFER_STATE:
            _sg_imgui_draw_buffer_panel(ctx, item->args.query_buffer_state.buffer.id);
            break;
        case SG_IMGUI_CMD_QUERY_IMAGE_STATE:
            _sg_imgui_draw_image_panel(ctx, item->args.query_image_state.image.id);
            break;
        case SG_IMGUI_CMD_QUERY_SHADER_STATE:
            _sg_imgui_draw_shader_panel(ctx, item->args.query_shader_state.shader.id);
            break;
        case SG_IMGUI_CMD_QUERY_PIPELINE_STATE:
            _sg_imgui_draw_pipeline_panel(ctx, item->args.query_pipeline_state.pipeline.id);
            break;
        case SG_IMGUI_CMD_QUERY_PASS_STATE:
            _sg_imgui_draw_pass_panel(ctx, item->args.query_pass_state.pass.id);
            break;
        case SG_IMGUI_CMD_BEGIN_DEFAULT_PASS:
            _sg_imgui_draw_passaction_panel(ctx, SG_INVALID_ID, &item->args.begin_default_pass.action);
            break;
        case SG_IMGUI_CMD_BEGIN_PASS:
            _sg_imgui_draw_passaction_panel(ctx, item->args.begin_pass.pass.id, &item->args.begin_pass.action);
            ImGui::Separator();
            _sg_imgui_draw_pass_panel(ctx, item->args.begin_pass.pass.id);
            break;
        case SG_IMGUI_CMD_APPLY_VIEWPORT:
        case SG_IMGUI_CMD_APPLY_SCISSOR_RECT:
            break;
        case SG_IMGUI_CMD_APPLY_PIPELINE:
            _sg_imgui_draw_pipeline_panel(ctx, item->args.apply_pipeline.pipeline.id);
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
            _sg_imgui_draw_buffer_panel(ctx, item->args.alloc_buffer.result.id);
            break;
        case SG_IMGUI_CMD_ALLOC_IMAGE:
            _sg_imgui_draw_image_panel(ctx, item->args.alloc_image.result.id);
            break;
        case SG_IMGUI_CMD_ALLOC_SHADER:
            _sg_imgui_draw_shader_panel(ctx, item->args.alloc_shader.result.id);
            break;
        case SG_IMGUI_CMD_ALLOC_PIPELINE:
            _sg_imgui_draw_pipeline_panel(ctx, item->args.alloc_pipeline.result.id);
            break;
        case SG_IMGUI_CMD_ALLOC_PASS:
            _sg_imgui_draw_pass_panel(ctx, item->args.alloc_pass.result.id);
            break;
        case SG_IMGUI_CMD_INIT_BUFFER:
            _sg_imgui_draw_buffer_panel(ctx, item->args.init_buffer.buffer.id);
            break;
        case SG_IMGUI_CMD_INIT_IMAGE:
            _sg_imgui_draw_image_panel(ctx, item->args.init_image.image.id);
            break;
        case SG_IMGUI_CMD_INIT_SHADER:
            _sg_imgui_draw_shader_panel(ctx, item->args.init_shader.shader.id);
            break;
        case SG_IMGUI_CMD_INIT_PIPELINE:
            _sg_imgui_draw_pipeline_panel(ctx, item->args.init_pipeline.pipeline.id);
            break;
        case SG_IMGUI_CMD_INIT_PASS:
            _sg_imgui_draw_pass_panel(ctx, item->args.init_pass.pass.id);
            break;
        case SG_IMGUI_CMD_FAIL_BUFFER:
            _sg_imgui_draw_buffer_panel(ctx, item->args.fail_buffer.buffer.id);
            break;
        case SG_IMGUI_CMD_FAIL_IMAGE:
            _sg_imgui_draw_image_panel(ctx, item->args.fail_image.image.id);
            break;
        case SG_IMGUI_CMD_FAIL_SHADER:
            _sg_imgui_draw_shader_panel(ctx, item->args.fail_shader.shader.id);
            break;
        case SG_IMGUI_CMD_FAIL_PIPELINE:
            _sg_imgui_draw_pipeline_panel(ctx, item->args.fail_pipeline.pipeline.id);
            break;
        case SG_IMGUI_CMD_FAIL_PASS:
            _sg_imgui_draw_pass_panel(ctx, item->args.fail_pass.pass.id);
            break;
        default:
            break;
    }
    ImGui::EndChild();
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
    hooks.query_feature = _sg_imgui_query_feature;
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
    hooks.query_buffer_overflow = _sg_imgui_query_buffer_overflow;
    hooks.query_buffer_state = _sg_imgui_query_buffer_state;
    hooks.query_image_state = _sg_imgui_query_image_state;
    hooks.query_shader_state = _sg_imgui_query_shader_state;
    hooks.query_pipeline_state = _sg_imgui_query_pipeline_state;
    hooks.query_pass_state = _sg_imgui_query_pass_state;
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
    ctx->buffers.num_slots = _sg.pools.buffer_pool.size;
    ctx->images.num_slots = _sg.pools.image_pool.size;
    ctx->shaders.num_slots = _sg.pools.shader_pool.size;
    ctx->pipelines.num_slots = _sg.pools.pipeline_pool.size;
    ctx->passes.num_slots = _sg.pools.pass_pool.size;

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
}

SOKOL_API_IMPL void sg_imgui_draw_buffers_window(sg_imgui_t* ctx) {
    SOKOL_ASSERT(ctx && (ctx->init_tag == 0xABCDABCD));
    if (!ctx->buffers.open) {
        return;
    }
    ImGui::SetNextWindowSize(ImVec2(440, 280), ImGuiSetCond_Once);
    if (ImGui::Begin("Buffers", &ctx->buffers.open)) {
        sg_imgui_draw_buffers_content(ctx);
    }
    ImGui::End();
}

SOKOL_API_IMPL void sg_imgui_draw_images_window(sg_imgui_t* ctx) {
    SOKOL_ASSERT(ctx && (ctx->init_tag == 0xABCDABCD));
    if (!ctx->images.open) {
        return;
    }
    ImGui::SetNextWindowSize(ImVec2(440, 400), ImGuiSetCond_Once);
    if (ImGui::Begin("Images", &ctx->images.open)) {
        sg_imgui_draw_images_content(ctx);
    }
    ImGui::End();
}

SOKOL_API_IMPL void sg_imgui_draw_shaders_window(sg_imgui_t* ctx) {
    SOKOL_ASSERT(ctx && (ctx->init_tag == 0xABCDABCD));
    if (!ctx->shaders.open) {
        return;
    }
    ImGui::SetNextWindowSize(ImVec2(440, 400), ImGuiSetCond_Once);
    if (ImGui::Begin("Shaders", &ctx->shaders.open)) {
        sg_imgui_draw_shaders_content(ctx);
    }
    ImGui::End();
}

SOKOL_API_IMPL void sg_imgui_draw_pipelines_window(sg_imgui_t* ctx) {
    SOKOL_ASSERT(ctx && (ctx->init_tag == 0xABCDABCD));
    if (!ctx->pipelines.open) {
        return;
    }
    ImGui::SetNextWindowSize(ImVec2(540, 400), ImGuiSetCond_Once);
    if (ImGui::Begin("Pipelines", &ctx->pipelines.open)) {
        sg_imgui_draw_pipelines_content(ctx);
    }
    ImGui::End();
}

SOKOL_API_IMPL void sg_imgui_draw_passes_window(sg_imgui_t* ctx) {
    SOKOL_ASSERT(ctx && (ctx->init_tag == 0xABCDABCD));
    if (!ctx->passes.open) {
        return;
    }
    ImGui::SetNextWindowSize(ImVec2(440, 400), ImGuiSetCond_Once);
    if (ImGui::Begin("Passes", &ctx->passes.open)) {
        sg_imgui_draw_passes_content(ctx);
    }
    ImGui::End();
}

SOKOL_API_IMPL void sg_imgui_draw_capture_window(sg_imgui_t* ctx) {
    SOKOL_ASSERT(ctx && (ctx->init_tag == 0xABCDABCD));
    if (!ctx->capture.open) {
        return;
    }
    ImGui::SetNextWindowSize(ImVec2(640, 400), ImGuiSetCond_Once);
    if (ImGui::Begin("Frame Capture", &ctx->capture.open)) {
        sg_imgui_draw_capture_content(ctx);
    }
    ImGui::End();
}

SOKOL_API_IMPL void sg_imgui_draw_buffers_content(sg_imgui_t* ctx) {
    SOKOL_ASSERT(ctx && (ctx->init_tag == 0xABCDABCD));
    _sg_imgui_draw_buffer_list(ctx);
    ImGui::SameLine();
    _sg_imgui_draw_buffer_panel(ctx, ctx->buffers.sel_id);
}

SOKOL_API_IMPL void sg_imgui_draw_images_content(sg_imgui_t* ctx) {
    SOKOL_ASSERT(ctx && (ctx->init_tag == 0xABCDABCD));
    _sg_imgui_draw_image_list(ctx);
    ImGui::SameLine();
    _sg_imgui_draw_image_panel(ctx, ctx->images.sel_id);
}

SOKOL_API_IMPL void sg_imgui_draw_shaders_content(sg_imgui_t* ctx) {
    SOKOL_ASSERT(ctx && (ctx->init_tag == 0xABCDABCD));
    _sg_imgui_draw_shader_list(ctx);
    ImGui::SameLine();
    _sg_imgui_draw_shader_panel(ctx, ctx->shaders.sel_id);
}

SOKOL_API_IMPL void sg_imgui_draw_pipelines_content(sg_imgui_t* ctx) {
    SOKOL_ASSERT(ctx && (ctx->init_tag == 0xABCDABCD));
    _sg_imgui_draw_pipeline_list(ctx);
    ImGui::SameLine();
    _sg_imgui_draw_pipeline_panel(ctx, ctx->pipelines.sel_id);
}

SOKOL_API_IMPL void sg_imgui_draw_passes_content(sg_imgui_t* ctx) {
    SOKOL_ASSERT(ctx && (ctx->init_tag == 0xABCDABCD));
    _sg_imgui_draw_pass_list(ctx);
    ImGui::SameLine();
    _sg_imgui_draw_pass_panel(ctx, ctx->passes.sel_id);
}

SOKOL_API_IMPL void sg_imgui_draw_capture_content(sg_imgui_t* ctx) {
    SOKOL_ASSERT(ctx && (ctx->init_tag == 0xABCDABCD));
    _sg_imgui_draw_capture_list(ctx);
    ImGui::SameLine();
    _sg_imgui_draw_capture_panel(ctx);
}

#endif /* SOKOL_IMPL */
