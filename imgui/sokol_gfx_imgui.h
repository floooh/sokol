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

#define SG_IMGUI_STRBUF_LEN (32)

/* a small string buffer to store strings coming into sokol_gfx.h via
    the desc structures, these are not guaranteed to be static, so
    we need to copy them
*/
typedef struct {
    char buf[SG_IMGUI_STRBUF_LEN];
} sg_imgui_str_t;

typedef struct {
    sg_buffer res_id;
    sg_imgui_str_t label;
} sg_imgui_buffer_t;

typedef struct {
    sg_image res_id;
    sg_imgui_str_t label;
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
} sg_imgui_pass_t;

typedef struct sg_imgui_buffers_t {
    bool open;
    int num_slots;
    uint32_t sel_id;
    sg_imgui_buffer_t* slots;
} sg_imgui_buffers_t;

typedef struct sg_imgui_images_t {
    bool open;
    int num_slots;
    uint32_t sel_id;
    sg_imgui_image_t* slots;
} sg_imgui_images_t;

typedef struct sg_imgui_shaders_t {
    bool open;
    int num_slots;
    uint32_t sel_id;
    sg_imgui_shader_t* slots;
} sg_imgui_shaders_t;

typedef struct sg_imgui_pipelines_t {
    bool open;
    int num_slots;
    uint32_t sel_id;
    sg_imgui_pipeline_t* slots;
} sg_imgui_pipelines_t;

typedef struct sg_imgui_passes_t {
    bool open;
    int num_slots;
    uint32_t sel_id;
    sg_imgui_pass_t* slots;
} sg_imgui_passes_t;

typedef struct sg_imgui_t {
    uint32_t init_tag;
    sg_imgui_buffers_t buffers;
    sg_imgui_images_t images;
    sg_imgui_shaders_t shaders;
    sg_imgui_pipelines_t pipelines;
    sg_imgui_passes_t passes;
    sg_trace_hooks hooks;
} sg_imgui_t;

void sg_imgui_init(sg_imgui_t* ctx);
void sg_imgui_discard(sg_imgui_t* ctx);
void sg_imgui_draw(sg_imgui_t* ctx);

void sg_imgui_draw_buffers_content(sg_imgui_t* ctx);
void sg_imgui_draw_images_content(sg_imgui_t* ctx);
void sg_imgui_draw_shaders_content(sg_imgui_t* ctx);
void sg_imgui_draw_pipelines_content(sg_imgui_t* ctx);
void sg_imgui_draw_passes_content(sg_imgui_t* ctx);

void sg_imgui_draw_buffers_window(sg_imgui_t* ctx);
void sg_imgui_draw_images_window(sg_imgui_t* ctx);
void sg_imgui_draw_shaders_window(sg_imgui_t* ctx);
void sg_imgui_draw_pipelines_window(sg_imgui_t* ctx);
void sg_imgui_draw_passes_window(sg_imgui_t* ctx);

#if defined(__cplusplus)
} /* extern "C" */
#endif

/*=== IMPLEMENTATION =========================================================*/
#if defined SOKOL_IMPL
#include <string.h>

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

_SOKOL_PRIVATE const char* _sg_imgui_str_dup(const char* src) {
    SOKOL_ASSERT(src);
    char* dst = (char*) SOKOL_MALLOC(strlen(src) + 1);
    strcpy(dst, src);
    return (const char*) dst;
}

_SOKOL_PRIVATE const uint8_t* _sg_imgui_bin_dup(const uint8_t* src, int num_bytes) {
    SOKOL_ASSERT(src && (num_bytes > 0));
    uint8_t* dst = (uint8_t*) SOKOL_MALLOC(num_bytes);
    memcpy(dst, src, num_bytes);
    return (const uint8_t*) dst;
}

_SOKOL_PRIVATE void _sg_imgui_buffer_created(sg_imgui_t* ctx, sg_buffer res_id, int slot_index, const sg_buffer_desc* desc) {
    SOKOL_ASSERT((slot_index > 0) && (slot_index < ctx->buffers.num_slots));
    sg_imgui_buffer_t* buf = &ctx->buffers.slots[slot_index];
    buf->res_id = res_id;
    _sg_imgui_strcpy(&buf->label, desc->label);
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
    _sg_imgui_strcpy(&img->label, desc->label);
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
    _sg_imgui_strcpy(&shd->label, desc->label);
    if (shd->desc.vs.entry) {
        _sg_imgui_strcpy(&shd->vs_entry, shd->desc.vs.entry);
        shd->desc.vs.entry = shd->vs_entry.buf;
    }
    if (shd->desc.fs.entry) {
        _sg_imgui_strcpy(&shd->fs_entry, shd->desc.fs.entry);
        shd->desc.fs.entry = shd->fs_entry.buf;
    }
    for (int i = 0; i < SG_MAX_SHADERSTAGE_UBS; i++) {
        for (int j = 0; j < SG_MAX_UB_MEMBERS; j++) {
            sg_shader_uniform_desc* ud = &shd->desc.vs.uniform_blocks[i].uniforms[j];
            if (ud->name) {
                _sg_imgui_strcpy(&shd->vs_uniform_name[i][j], ud->name);
                ud->name = shd->vs_uniform_name[i][j].buf;
            }
        }
    }
    for (int i = 0; i < SG_MAX_SHADERSTAGE_UBS; i++) {
        for (int j = 0; j < SG_MAX_UB_MEMBERS; j++) {
            sg_shader_uniform_desc* ud = &shd->desc.fs.uniform_blocks[i].uniforms[j];
            if (ud->name) {
                _sg_imgui_strcpy(&shd->fs_uniform_name[i][j], ud->name);
                ud->name = shd->fs_uniform_name[i][j].buf;
            }
        }
    }
    for (int i = 0; i < SG_MAX_SHADERSTAGE_IMAGES; i++) {
        if (shd->desc.vs.images[i].name) {
            _sg_imgui_strcpy(&shd->vs_image_name[i], shd->desc.vs.images[i].name);
            shd->desc.vs.images[i].name = shd->vs_image_name[i].buf;
        }
    }
    for (int i = 0; i < SG_MAX_SHADERSTAGE_IMAGES; i++) {
        if (shd->desc.fs.images[i].name) {
            _sg_imgui_strcpy(&shd->fs_image_name[i], shd->desc.fs.images[i].name);
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
        SOKOL_FREE((void*)shd->desc.vs.source);
        shd->desc.vs.source = 0;
    }
    if (shd->desc.vs.byte_code) {
        SOKOL_FREE((void*)shd->desc.vs.byte_code);
        shd->desc.vs.byte_code = 0;
    }
    if (shd->desc.fs.source) {
        SOKOL_FREE((void*)shd->desc.fs.source);
        shd->desc.fs.source = 0;
    }
    if (shd->desc.fs.byte_code) {
        SOKOL_FREE((void*)shd->desc.fs.byte_code);
        shd->desc.fs.byte_code = 0;
    }
}

_SOKOL_PRIVATE void _sg_imgui_pipeline_created(sg_imgui_t* ctx, sg_pipeline res_id, int slot_index, const sg_pipeline_desc* desc) {
    SOKOL_ASSERT((slot_index > 0) && (slot_index < ctx->pipelines.num_slots));
    sg_imgui_pipeline_t* pip = &ctx->pipelines.slots[slot_index];
    pip->res_id = res_id;
    _sg_imgui_strcpy(&pip->label, desc->label);
    pip->desc = *desc;

    /* copy strings in vertex layout to persistent location */
    for (int i = 0; i < SG_MAX_VERTEX_ATTRIBUTES; i++) {
        sg_vertex_attr_desc* ad = &pip->desc.layout.attrs[i];
        if (ad->name) {
            _sg_imgui_strcpy(&pip->attr_name[i], ad->name);
            ad->name = pip->attr_name[i].buf;
        }
        if (ad->sem_name) {
            _sg_imgui_strcpy(&pip->attr_sem_name[i], ad->sem_name);
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
    _sg_imgui_strcpy(&pass->label, desc->label);
}

_SOKOL_PRIVATE void _sg_imgui_pass_destroyed(sg_imgui_t* ctx, int slot_index) {
    SOKOL_ASSERT((slot_index > 0) && (slot_index < ctx->passes.num_slots));
    sg_imgui_pass_t* pass = &ctx->passes.slots[slot_index];
    pass->res_id.id = SG_INVALID_ID;
}

/*--- sokol-gfx trace hook functions -----------------------------------------*/
_SOKOL_PRIVATE void _sg_imgui_query_feature(sg_feature feature, bool result, void* user_data) {
    sg_imgui_t* ctx = (sg_imgui_t*) user_data;
    SOKOL_ASSERT(ctx);
    if (ctx->hooks.query_feature) {
        ctx->hooks.query_feature(feature, result, ctx->hooks.user_data);
    }
}

_SOKOL_PRIVATE void _sg_imgui_reset_state_cache(void* user_data) {
    sg_imgui_t* ctx = (sg_imgui_t*) user_data;
    SOKOL_ASSERT(ctx);
    if (ctx->hooks.reset_state_cache) {
        ctx->hooks.reset_state_cache(ctx->hooks.user_data);
    }
}

_SOKOL_PRIVATE void _sg_imgui_make_buffer(const sg_buffer_desc* desc, sg_buffer buf_id, void* user_data) {
    sg_imgui_t* ctx = (sg_imgui_t*) user_data;
    SOKOL_ASSERT(ctx);
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
    if (ctx->hooks.destroy_pass) {
        ctx->hooks.destroy_pass(pass, ctx->hooks.user_data);
    }
    if (pass.id != SG_INVALID_ID) {
        _sg_imgui_pass_destroyed(ctx, _sg_slot_index(pass.id));
    }
}

_SOKOL_PRIVATE void _sg_imgui_update_buffer(sg_buffer buf, const void* data_ptr, int data_size, void* user_data) {
    const sg_imgui_t* ctx = (const sg_imgui_t*) user_data;
    SOKOL_ASSERT(ctx);
    if (ctx->hooks.update_buffer) {
        ctx->hooks.update_buffer(buf, data_ptr, data_size, ctx->hooks.user_data);
    }
}

_SOKOL_PRIVATE void _sg_imgui_update_image(sg_image img, const sg_image_content* data, void* user_data) {
    const sg_imgui_t* ctx = (const sg_imgui_t*) user_data;
    SOKOL_ASSERT(ctx);
    if (ctx->hooks.update_image) {
        ctx->hooks.update_image(img, data, ctx->hooks.user_data);
    }
}

_SOKOL_PRIVATE void _sg_imgui_append_buffer(sg_buffer buf, const void* data_ptr, int data_size, int result, void* user_data) {
    const sg_imgui_t* ctx = (const sg_imgui_t*) user_data;
    SOKOL_ASSERT(ctx);
    if (ctx->hooks.append_buffer) {
        ctx->hooks.append_buffer(buf, data_ptr, data_size, result, ctx->hooks.user_data);
    }
}

_SOKOL_PRIVATE void _sg_imgui_query_buffer_overflow(sg_buffer buf, bool result, void* user_data) {
    const sg_imgui_t* ctx = (const sg_imgui_t*) user_data;
    SOKOL_ASSERT(ctx);
    if (ctx->hooks.query_buffer_overflow) {
        ctx->hooks.query_buffer_overflow(buf, result, ctx->hooks.user_data);
    }
}

_SOKOL_PRIVATE void _sg_imgui_query_buffer_state(sg_buffer buf, sg_resource_state result, void* user_data) {
    const sg_imgui_t* ctx = (const sg_imgui_t*) user_data;
    SOKOL_ASSERT(ctx);
    if (ctx->hooks.query_buffer_state) {
        ctx->hooks.query_buffer_state(buf, result, ctx->hooks.user_data);
    }
}

_SOKOL_PRIVATE void _sg_imgui_query_image_state(sg_image img, sg_resource_state result, void* user_data) {
    const sg_imgui_t* ctx = (const sg_imgui_t*) user_data;
    SOKOL_ASSERT(ctx);
    if (ctx->hooks.query_image_state) {
        ctx->hooks.query_image_state(img, result, ctx->hooks.user_data);
    }
}

_SOKOL_PRIVATE void _sg_imgui_query_shader_state(sg_shader shd, sg_resource_state result, void* user_data) {
    const sg_imgui_t* ctx = (const sg_imgui_t*) user_data;
    SOKOL_ASSERT(ctx);
    if (ctx->hooks.query_shader_state) {
        ctx->hooks.query_shader_state(shd, result, ctx->hooks.user_data);
    }
}

_SOKOL_PRIVATE void _sg_imgui_query_pipeline_state(sg_pipeline pip, sg_resource_state result, void* user_data) {
    const sg_imgui_t* ctx = (const sg_imgui_t*) user_data;
    SOKOL_ASSERT(ctx);
    if (ctx->hooks.query_pipeline_state) {
        ctx->hooks.query_pipeline_state(pip, result, ctx->hooks.user_data);
    }
}

_SOKOL_PRIVATE void _sg_imgui_query_pass_state(sg_pass pass, sg_resource_state result, void* user_data) {
    const sg_imgui_t* ctx = (const sg_imgui_t*) user_data;
    SOKOL_ASSERT(ctx);
    if (ctx->hooks.query_pass_state) {
        ctx->hooks.query_pass_state(pass, result, ctx->hooks.user_data);
    }
}

_SOKOL_PRIVATE void _sg_imgui_begin_default_pass(const sg_pass_action* pass_action, int width, int height, void* user_data) {
    const sg_imgui_t* ctx = (const sg_imgui_t*) user_data;
    SOKOL_ASSERT(ctx);
    if (ctx->hooks.begin_default_pass) {
        ctx->hooks.begin_default_pass(pass_action, width, height, ctx->hooks.user_data);
    }
}

_SOKOL_PRIVATE void _sg_imgui_begin_pass(sg_pass pass, const sg_pass_action* pass_action, void* user_data) {
    const sg_imgui_t* ctx = (const sg_imgui_t*) user_data;
    SOKOL_ASSERT(ctx);
    if (ctx->hooks.begin_pass) {
        ctx->hooks.begin_pass(pass, pass_action, ctx->hooks.user_data);
    }
}

_SOKOL_PRIVATE void _sg_imgui_apply_viewport(int x, int y, int width, int height, bool origin_top_left, void* user_data) {
    const sg_imgui_t* ctx = (const sg_imgui_t*) user_data;
    SOKOL_ASSERT(ctx);
    if (ctx->hooks.apply_viewport) {
        ctx->hooks.apply_viewport(x, y, width, height, origin_top_left, ctx->hooks.user_data);
    }
}

_SOKOL_PRIVATE void _sg_imgui_apply_scissor_rect(int x, int y, int width, int height, bool origin_top_left, void* user_data) {
    const sg_imgui_t* ctx = (const sg_imgui_t*) user_data;
    SOKOL_ASSERT(ctx);
    if (ctx->hooks.apply_scissor_rect) {
        ctx->hooks.apply_scissor_rect(x, y, width, height, origin_top_left, ctx->hooks.user_data);
    }
}

_SOKOL_PRIVATE void _sg_imgui_apply_pipeline(sg_pipeline pip, void* user_data) {
    const sg_imgui_t* ctx = (const sg_imgui_t*) user_data;
    SOKOL_ASSERT(ctx);
    if (ctx->hooks.apply_pipeline) {
        ctx->hooks.apply_pipeline(pip, ctx->hooks.user_data);
    }
}

_SOKOL_PRIVATE void _sg_imgui_apply_bindings(const sg_bindings* bindings, void* user_data) {
    const sg_imgui_t* ctx = (const sg_imgui_t*) user_data;
    SOKOL_ASSERT(ctx);
    if (ctx->hooks.apply_bindings) {
        ctx->hooks.apply_bindings(bindings, ctx->hooks.user_data);
    }
}

_SOKOL_PRIVATE void _sg_imgui_apply_uniforms(sg_shader_stage stage, int ub_index, const void* data, int num_bytes, void* user_data) {
    const sg_imgui_t* ctx = (const sg_imgui_t*) user_data;
    SOKOL_ASSERT(ctx);
    if (ctx->hooks.apply_uniforms) {
        ctx->hooks.apply_uniforms(stage, ub_index, data, num_bytes, ctx->hooks.user_data);
    }
}

_SOKOL_PRIVATE void _sg_imgui_draw(int base_element, int num_elements, int num_instances, void* user_data) {
    const sg_imgui_t* ctx = (const sg_imgui_t*) user_data;
    SOKOL_ASSERT(ctx);
    if (ctx->hooks.draw) {
        ctx->hooks.draw(base_element, num_elements, num_instances, ctx->hooks.user_data);
    }
}

_SOKOL_PRIVATE void _sg_imgui_end_pass(void* user_data) {
    const sg_imgui_t* ctx = (const sg_imgui_t*) user_data;
    SOKOL_ASSERT(ctx);
    if (ctx->hooks.end_pass) {
        ctx->hooks.end_pass(ctx->hooks.user_data);
    }
}

_SOKOL_PRIVATE void _sg_imgui_commit(void* user_data) {
    const sg_imgui_t* ctx = (const sg_imgui_t*) user_data;
    SOKOL_ASSERT(ctx);
    if (ctx->hooks.commit) {
        ctx->hooks.commit(ctx->hooks.user_data);
    }
}

_SOKOL_PRIVATE void _sg_imgui_alloc_buffer(sg_buffer result, void* user_data) {
    const sg_imgui_t* ctx = (const sg_imgui_t*) user_data;
    SOKOL_ASSERT(ctx);
    if (ctx->hooks.alloc_buffer) {
        ctx->hooks.alloc_buffer(result, ctx->hooks.user_data);
    }
}

_SOKOL_PRIVATE void _sg_imgui_alloc_image(sg_image result, void* user_data) {
    const sg_imgui_t* ctx = (const sg_imgui_t*) user_data;
    SOKOL_ASSERT(ctx);
    if (ctx->hooks.alloc_image) {
        ctx->hooks.alloc_image(result, ctx->hooks.user_data);
    }
}

_SOKOL_PRIVATE void _sg_imgui_alloc_shader(sg_shader result, void* user_data) {
    const sg_imgui_t* ctx = (const sg_imgui_t*) user_data;
    SOKOL_ASSERT(ctx);
    if (ctx->hooks.alloc_shader) {
        ctx->hooks.alloc_shader(result, ctx->hooks.user_data);
    }
}

_SOKOL_PRIVATE void _sg_imgui_alloc_pipeline(sg_pipeline result, void* user_data) {
    const sg_imgui_t* ctx = (const sg_imgui_t*) user_data;
    SOKOL_ASSERT(ctx);
    if (ctx->hooks.alloc_pipeline) {
        ctx->hooks.alloc_pipeline(result, ctx->hooks.user_data);
    }
}

_SOKOL_PRIVATE void _sg_imgui_alloc_pass(sg_pass result, void* user_data) {
    const sg_imgui_t* ctx = (const sg_imgui_t*) user_data;
    SOKOL_ASSERT(ctx);
    if (ctx->hooks.alloc_pass) {
        ctx->hooks.alloc_pass(result, ctx->hooks.user_data);
    }
}

_SOKOL_PRIVATE void _sg_imgui_init_buffer(sg_buffer buf_id, const sg_buffer_desc* desc, void* user_data) {
    sg_imgui_t* ctx = (sg_imgui_t*) user_data;
    SOKOL_ASSERT(ctx);
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
    if (ctx->hooks.init_pass) {
        ctx->hooks.init_pass(pass_id, desc, ctx->hooks.user_data);
    }
    if (pass_id.id != SG_INVALID_ID) {
        _sg_imgui_pass_created(ctx, pass_id, _sg_slot_index(pass_id.id), desc);
    }
}

_SOKOL_PRIVATE void _sg_imgui_fail_buffer(sg_buffer buf_id, void* user_data) {
    const sg_imgui_t* ctx = (const sg_imgui_t*) user_data;
    SOKOL_ASSERT(ctx);
    if (ctx->hooks.fail_buffer) {
        ctx->hooks.fail_buffer(buf_id, ctx->hooks.user_data);
    }
}

_SOKOL_PRIVATE void _sg_imgui_fail_image(sg_image img_id, void* user_data) {
    const sg_imgui_t* ctx = (const sg_imgui_t*) user_data;
    SOKOL_ASSERT(ctx);
    if (ctx->hooks.fail_image) {
        ctx->hooks.fail_image(img_id, ctx->hooks.user_data);
    }
}

_SOKOL_PRIVATE void _sg_imgui_fail_shader(sg_shader shd_id, void* user_data) {
    const sg_imgui_t* ctx = (const sg_imgui_t*) user_data;
    SOKOL_ASSERT(ctx);
    if (ctx->hooks.fail_shader) {
        ctx->hooks.fail_shader(shd_id, ctx->hooks.user_data);
    }
}

_SOKOL_PRIVATE void _sg_imgui_fail_pipeline(sg_pipeline pip_id, void* user_data) {
    const sg_imgui_t* ctx = (const sg_imgui_t*) user_data;
    SOKOL_ASSERT(ctx);
    if (ctx->hooks.fail_pipeline) {
        ctx->hooks.fail_pipeline(pip_id, ctx->hooks.user_data);
    }
}

_SOKOL_PRIVATE void _sg_imgui_fail_pass(sg_pass pass_id, void* user_data) {
    const sg_imgui_t* ctx = (const sg_imgui_t*) user_data;
    SOKOL_ASSERT(ctx);
    if (ctx->hooks.fail_pass) {
        ctx->hooks.fail_pass(pass_id, ctx->hooks.user_data);
    }
}

_SOKOL_PRIVATE void _sg_imgui_err_buffer_pool_exhausted(void* user_data) {
    const sg_imgui_t* ctx = (const sg_imgui_t*) user_data;
    SOKOL_ASSERT(ctx);
    if (ctx->hooks.err_buffer_pool_exhausted) {
        ctx->hooks.err_buffer_pool_exhausted(ctx->hooks.user_data);
    }
}

_SOKOL_PRIVATE void _sg_imgui_err_image_pool_exhausted(void* user_data) {
    const sg_imgui_t* ctx = (const sg_imgui_t*) user_data;
    SOKOL_ASSERT(ctx);
    if (ctx->hooks.err_image_pool_exhausted) {
        ctx->hooks.err_image_pool_exhausted(ctx->hooks.user_data);
    }
}

_SOKOL_PRIVATE void _sg_imgui_err_shader_pool_exhausted(void* user_data) {
    const sg_imgui_t* ctx = (const sg_imgui_t*) user_data;
    SOKOL_ASSERT(ctx);
    if (ctx->hooks.err_shader_pool_exhausted) {
        ctx->hooks.err_shader_pool_exhausted(ctx->hooks.user_data);
    }
}

_SOKOL_PRIVATE void _sg_imgui_err_pipeline_pool_exhausted(void* user_data) {
    const sg_imgui_t* ctx = (const sg_imgui_t*) user_data;
    SOKOL_ASSERT(ctx);
    if (ctx->hooks.err_pipeline_pool_exhausted) {
        ctx->hooks.err_pipeline_pool_exhausted(ctx->hooks.user_data);
    }
}

_SOKOL_PRIVATE void _sg_imgui_err_pass_pool_exhausted(void* user_data) {
    const sg_imgui_t* ctx = (const sg_imgui_t*) user_data;
    SOKOL_ASSERT(ctx);
    if (ctx->hooks.err_pass_pool_exhausted) {
        ctx->hooks.err_pass_pool_exhausted(ctx->hooks.user_data);
    }
}

_SOKOL_PRIVATE void _sg_imgui_err_context_mismatch(void* user_data) {
    const sg_imgui_t* ctx = (const sg_imgui_t*) user_data;
    SOKOL_ASSERT(ctx);
    if (ctx->hooks.err_context_mismatch) {
        ctx->hooks.err_context_mismatch(ctx->hooks.user_data);
    }
}

_SOKOL_PRIVATE void _sg_imgui_err_pass_invalid(void* user_data) {
    const sg_imgui_t* ctx = (const sg_imgui_t*) user_data;
    SOKOL_ASSERT(ctx);
    if (ctx->hooks.err_pass_invalid) {
        ctx->hooks.err_pass_invalid(ctx->hooks.user_data);
    }
}

_SOKOL_PRIVATE void _sg_imgui_err_draw_invalid(void* user_data) {
    const sg_imgui_t* ctx = (const sg_imgui_t*) user_data;
    SOKOL_ASSERT(ctx);
    if (ctx->hooks.err_draw_invalid) {
        ctx->hooks.err_draw_invalid(ctx->hooks.user_data);
    }
}

_SOKOL_PRIVATE void _sg_imgui_err_bindings_invalid(void* user_data) {
    const sg_imgui_t* ctx = (const sg_imgui_t*) user_data;
    SOKOL_ASSERT(ctx);
    if (ctx->hooks.err_bindings_invalid) {
        ctx->hooks.err_bindings_invalid(ctx->hooks.user_data);
    }
}

_SOKOL_PRIVATE bool _sg_imgui_draw_resid_list(uint32_t res_id, const char* label, bool selected) {
    if (label[0]) {
        return ImGui::Selectable(label, selected);
    }
    else {
        char buf[32];
        snprintf(buf, sizeof(buf), "0x%08X", res_id);
        return ImGui::Selectable(buf, selected);
    }
}

_SOKOL_PRIVATE bool _sg_imgui_draw_resid_link(uint32_t res_id, const char* label) {
    char buf[32];
    const char* str;
    if (label[0]) {
        str = label;
    }
    else {
        snprintf(buf, sizeof(buf), "0x%08X", res_id);
        str = buf;
    }
    return ImGui::SmallButton(str);
}

_SOKOL_PRIVATE bool _sg_imgui_draw_shader_link(sg_imgui_t* ctx, uint32_t shd_id) {
    bool retval = false;
    if (shd_id != SG_INVALID_ID) {
        const sg_imgui_shader_t* shd_ui = &ctx->shaders.slots[_sg_slot_index(shd_id)];
        retval = _sg_imgui_draw_resid_link(shd_id, shd_ui->label.buf);
    }
    return retval;
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

_SOKOL_PRIVATE void _sg_imgui_show_image(sg_imgui_t* ctx, uint32_t img_id) {
    ctx->images.open = true;
    ctx->images.sel_id = img_id;
}

_SOKOL_PRIVATE void _sg_imgui_show_shader(sg_imgui_t* ctx, uint32_t shd_id) {
    ctx->shaders.open = true;
    ctx->shaders.sel_id = shd_id;
}

_SOKOL_PRIVATE void _sg_imgui_draw_buffer_list(sg_imgui_t* ctx) {
    ImGui::BeginChild("buffer_list", ImVec2(128,0), true);
    for (int i = 1; i < _sg.pools.buffer_pool.size; i++) {
        const _sg_buffer_t* buf = &_sg.pools.buffers[i];
        if (buf->slot.state != SG_RESOURCESTATE_INITIAL) {
            bool selected = ctx->buffers.sel_id == buf->slot.id;
            if (_sg_imgui_draw_resid_list(buf->slot.id, ctx->buffers.slots[i].label.buf, selected)) {
                ctx->buffers.sel_id = buf->slot.id;
            }
        }
    }
    ImGui::EndChild();
}

_SOKOL_PRIVATE void _sg_imgui_draw_image_list(sg_imgui_t* ctx) {
    ImGui::BeginChild("image_list", ImVec2(128,0), true);
    for (int i = 1; i < _sg.pools.image_pool.size; i++) {
        const _sg_image_t* img = &_sg.pools.images[i];
        if (img->slot.state != SG_RESOURCESTATE_INITIAL) {
            bool selected = ctx->images.sel_id == img->slot.id;
            if (_sg_imgui_draw_resid_list(img->slot.id, ctx->images.slots[i].label.buf, selected)) {
                ctx->images.sel_id = img->slot.id;
            }
        }
    }
    ImGui::EndChild();
}

_SOKOL_PRIVATE void _sg_imgui_draw_shader_list(sg_imgui_t* ctx) {
    ImGui::BeginChild("shader_list", ImVec2(128,0), true);
    for (int i = 1; i < _sg.pools.shader_pool.size; i++) {
        const _sg_shader_t* shd = &_sg.pools.shaders[i];
        if (shd->slot.state != SG_RESOURCESTATE_INITIAL) {
            bool selected = ctx->shaders.sel_id == shd->slot.id;
            if (_sg_imgui_draw_resid_list(shd->slot.id, ctx->shaders.slots[i].label.buf, selected)) {
                ctx->shaders.sel_id = shd->slot.id;
            }
        }
    }
    ImGui::EndChild();
}

_SOKOL_PRIVATE void _sg_imgui_draw_pipeline_list(sg_imgui_t* ctx) {
    ImGui::BeginChild("pipeline_list", ImVec2(128,0), true);
    for (int i = 1; i < _sg.pools.pipeline_pool.size; i++) {
        const _sg_pipeline_t* pip = &_sg.pools.pipelines[i];
        if (pip->slot.state != SG_RESOURCESTATE_INITIAL) {
            bool selected = ctx->pipelines.sel_id == pip->slot.id;
            if (_sg_imgui_draw_resid_list(pip->slot.id, ctx->pipelines.slots[i].label.buf, selected)) {
                ctx->pipelines.sel_id = pip->slot.id;
            }
        }
    }
    ImGui::EndChild();
}

_SOKOL_PRIVATE void _sg_imgui_draw_pass_list(sg_imgui_t* ctx) {
    ImGui::BeginChild("pass_list", ImVec2(128,0), true);
    for (int i = 1; i < _sg.pools.pass_pool.size; i++) {
        const _sg_pass_t* pass = &_sg.pools.passes[i];
        if (pass->slot.state != SG_RESOURCESTATE_INITIAL) {
            bool selected = ctx->passes.sel_id == pass->slot.id;
            if (_sg_imgui_draw_resid_list(pass->slot.id, ctx->passes.slots[i].label.buf, selected)) {
                ctx->passes.sel_id = pass->slot.id;
            }
        }
    }
    ImGui::EndChild();
}

_SOKOL_PRIVATE void _sg_imgui_draw_buffer_panel(sg_imgui_t* ctx, uint32_t sel_id) {
    if (sel_id != SG_INVALID_ID) {
        const _sg_buffer_t* buf = _sg_buffer_at(&_sg.pools, sel_id);
        const sg_imgui_buffer_t* buf_ui = &ctx->buffers.slots[_sg_slot_index(sel_id)];
        ImGui::SameLine();
        ImGui::BeginChild("buffer", ImVec2(0,0), false);
        ImGui::Text("Label: %s", buf_ui->label.buf[0] ? buf_ui->label.buf : "---");
        _sg_imgui_draw_resource_slot(&buf->slot);
        ImGui::Separator();
        ImGui::Text("Type:  %s", _sg_imgui_buffertype_string(buf->type));
        ImGui::Text("Usage: %s", _sg_imgui_usage_string(buf->usage));
        ImGui::Text("Size:  %d", buf->size);
        if (buf->usage != SG_USAGE_IMMUTABLE) {
            ImGui::Separator();
            ImGui::Text("Num Slots:     %d", buf->num_slots);
            ImGui::Text("Active Slot:/s   %d", buf->active_slot);
            ImGui::Text("Update Frame Index: %d", buf->update_frame_index);
            ImGui::Text("Append Frame Index: %d", buf->append_frame_index);
            ImGui::Text("Append Pos:         %d", buf->append_pos);
            ImGui::Text("Append Overflow:    %s", buf->append_overflow ? "YES":"NO");
        }
        ImGui::EndChild();
    }
}

_SOKOL_PRIVATE void _sg_imgui_draw_image_panel(sg_imgui_t* ctx, uint32_t sel_id) {
    if (sel_id != SG_INVALID_ID) {
        const _sg_image_t* img = _sg_image_at(&_sg.pools, sel_id);
        const sg_imgui_image_t* img_ui = &ctx->images.slots[_sg_slot_index(sel_id)];
        ImGui::SameLine();
        ImGui::BeginChild("image", ImVec2(0,0), false);
        ImGui::Text("Label: %s", img_ui->label.buf[0] ? img_ui->label.buf : "---");
        _sg_imgui_draw_resource_slot(&img->slot);
        ImGui::Separator();
        ImGui::Text("Type:              %s", _sg_imgui_imagetype_string(img->type));
        ImGui::Text("Usage:             %s", _sg_imgui_usage_string(img->usage));
        ImGui::Text("Render Target:     %s", img->render_target ? "YES":"NO");
        ImGui::Text("Width:             %d", img->width);
        ImGui::Text("Height:            %d", img->height);
        ImGui::Text("Depth:             %d", img->depth);
        ImGui::Text("Num Mipmaps:       %d", img->num_mipmaps);
        ImGui::Text("Pixel Format:      %s", _sg_imgui_pixelformat_string(img->pixel_format));
        ImGui::Text("Sample Count:      %d", img->sample_count);
        ImGui::Text("Min Filter:        %s", _sg_imgui_filter_string(img->min_filter));
        ImGui::Text("Mag Filter:        %s", _sg_imgui_filter_string(img->mag_filter));
        ImGui::Text("Wrap U:            %s", _sg_imgui_wrap_string(img->wrap_u));
        ImGui::Text("Wrap V:            %s", _sg_imgui_wrap_string(img->wrap_v));
        ImGui::Text("Wrap W:            %s", _sg_imgui_wrap_string(img->wrap_w));
        ImGui::Text("Max Anisotropy:    %d", img->max_anisotropy);
        if (img->usage != SG_USAGE_IMMUTABLE) {
            ImGui::Separator();
            ImGui::Text("Num Slots:     %d", img->num_slots);
            ImGui::Text("Active Slot:   %d", img->active_slot);
            ImGui::Text("Update Frame Index: %d", img->upd_frame_index);
        }
        if ((SG_IMAGETYPE_2D == img->type) && !_sg_is_valid_rendertarget_depth_format(img->pixel_format)) {
            ImGui::BeginChild("texture", ImVec2(0, 0), true, ImGuiWindowFlags_HorizontalScrollbar);
            ImGui::Image((ImTextureID)(intptr_t)sel_id, ImVec2(2*img->width, 2*img->height));
            ImGui::EndChild();
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
            ImGui::Text("FIXME!");
        }
    }
}

_SOKOL_PRIVATE void _sg_imgui_draw_shader_panel(sg_imgui_t* ctx, uint32_t sel_id) {
    if (sel_id != SG_INVALID_ID) {
        const _sg_shader_t* shd = _sg_shader_at(&_sg.pools, sel_id);
        const sg_imgui_shader_t* shd_ui = &ctx->shaders.slots[_sg_slot_index(sel_id)];
        ImGui::SameLine();
        ImGui::BeginChild("shader", ImVec2(0,0), false, ImGuiWindowFlags_HorizontalScrollbar);
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

_SOKOL_PRIVATE void _sg_imgui_draw_pipeline_panel(sg_imgui_t* ctx, uint32_t sel_id) {
    if (sel_id != SG_INVALID_ID) {
        const _sg_pipeline_t* pip = _sg_pipeline_at(&_sg.pools, sel_id);
        const sg_imgui_pipeline_t* pip_ui = &ctx->pipelines.slots[_sg_slot_index(sel_id)];
        ImGui::SameLine();
        ImGui::BeginChild("pipeline", ImVec2(0,0), false);
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
        ImGui::EndChild();
    }
}

_SOKOL_PRIVATE void _sg_imgui_draw_pass_panel(sg_imgui_t* ctx, uint32_t sel_id) {
    if (sel_id != SG_INVALID_ID) {
        const _sg_pass_t* pass = _sg_pass_at(&_sg.pools, sel_id);
        const sg_imgui_pass_t* pass_ui = &ctx->passes.slots[_sg_slot_index(sel_id)];
        ImGui::SameLine();
        ImGui::BeginChild("pass", ImVec2(0,0), false);
        ImGui::Text("Label: %s", pass_ui->label.buf[0] ? pass_ui->label.buf : "---");
        _sg_imgui_draw_resource_slot(&pass->slot);
        ImGui::EndChild();
    }
}

void sg_imgui_init(sg_imgui_t* ctx) {
    SOKOL_ASSERT(ctx);
    memset(ctx, 0, sizeof(sg_imgui_t));
    ctx->init_tag = 0xABCDABCD;

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
    ctx->buffers.slots = (sg_imgui_buffer_t*) SOKOL_MALLOC(buffer_pool_size);
    SOKOL_ASSERT(ctx->buffers.slots);
    memset(ctx->buffers.slots, 0, buffer_pool_size);

    const int image_pool_size = ctx->images.num_slots * sizeof(sg_imgui_image_t);
    ctx->images.slots = (sg_imgui_image_t*) SOKOL_MALLOC(image_pool_size);
    SOKOL_ASSERT(ctx->images.slots);
    memset(ctx->images.slots, 0, image_pool_size);

    const int shader_pool_size = ctx->shaders.num_slots * sizeof(sg_imgui_shader_t);
    ctx->shaders.slots = (sg_imgui_shader_t*) SOKOL_MALLOC(shader_pool_size);
    SOKOL_ASSERT(ctx->shaders.slots);
    memset(ctx->shaders.slots, 0, shader_pool_size);

    const int pipeline_pool_size = ctx->pipelines.num_slots * sizeof(sg_imgui_pipeline_t);
    ctx->pipelines.slots = (sg_imgui_pipeline_t*) SOKOL_MALLOC(pipeline_pool_size);
    SOKOL_ASSERT(ctx->pipelines.slots);
    memset(ctx->pipelines.slots, 0, pipeline_pool_size);

    const int pass_pool_size = ctx->passes.num_slots * sizeof(sg_imgui_pass_t);
    ctx->passes.slots = (sg_imgui_pass_t*) SOKOL_MALLOC(pass_pool_size);
    SOKOL_ASSERT(ctx->passes.slots);
    memset(ctx->passes.slots, 0, pass_pool_size);
}

void sg_imgui_discard(sg_imgui_t* ctx) {
    SOKOL_ASSERT(ctx && (ctx->init_tag == 0xABCDABCD));
    ctx->init_tag = 0;
    if (ctx->buffers.slots) {
        for (int i = 0; i < ctx->buffers.num_slots; i++) {
            if (ctx->buffers.slots[i].res_id.id != SG_INVALID_ID) {
                _sg_imgui_buffer_destroyed(ctx, i);
            }
        }
        SOKOL_FREE((void*)ctx->buffers.slots);
        ctx->buffers.slots = 0;
    }
    if (ctx->images.slots) {
        for (int i = 0; i < ctx->images.num_slots; i++) {
            if (ctx->images.slots[i].res_id.id != SG_INVALID_ID) {
                _sg_imgui_image_destroyed(ctx, i);
            }
        }
        SOKOL_FREE((void*)ctx->images.slots);
        ctx->images.slots = 0;
    }
    if (ctx->shaders.slots) {
        for (int i = 0; i < ctx->shaders.num_slots; i++) {
            if (ctx->shaders.slots[i].res_id.id != SG_INVALID_ID) {
                _sg_imgui_shader_destroyed(ctx, i);
            }
        }
        SOKOL_FREE((void*)ctx->shaders.slots);
        ctx->shaders.slots = 0;
    }
    if (ctx->pipelines.slots) {
        for (int i = 0; i < ctx->pipelines.num_slots; i++) {
            if (ctx->pipelines.slots[i].res_id.id != SG_INVALID_ID) {
                _sg_imgui_pipeline_destroyed(ctx, i);
            }
        }
        SOKOL_FREE((void*)ctx->pipelines.slots);
        ctx->pipelines.slots = 0;
    }
    if (ctx->passes.slots) {
        for (int i = 0; i < ctx->passes.num_slots; i++) {
            if (ctx->passes.slots[i].res_id.id != SG_INVALID_ID) {
                _sg_imgui_pass_destroyed(ctx, i);
            }
        }
        SOKOL_FREE((void*)ctx->passes.slots);
        ctx->passes.slots = 0;
    }
}

void sg_imgui_draw(sg_imgui_t* ctx) {
    SOKOL_ASSERT(ctx && (ctx->init_tag == 0xABCDABCD));
    sg_imgui_draw_buffers_window(ctx);
    sg_imgui_draw_images_window(ctx);
    sg_imgui_draw_shaders_window(ctx);
    sg_imgui_draw_pipelines_window(ctx);
    sg_imgui_draw_passes_window(ctx);
}

void sg_imgui_draw_buffers_window(sg_imgui_t* ctx) {
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

void sg_imgui_draw_images_window(sg_imgui_t* ctx) {
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

void sg_imgui_draw_shaders_window(sg_imgui_t* ctx) {
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

void sg_imgui_draw_pipelines_window(sg_imgui_t* ctx) {
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

void sg_imgui_draw_passes_window(sg_imgui_t* ctx) {
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

void sg_imgui_draw_buffers_content(sg_imgui_t* ctx) {
    SOKOL_ASSERT(ctx && (ctx->init_tag == 0xABCDABCD));
    _sg_imgui_draw_buffer_list(ctx);
    _sg_imgui_draw_buffer_panel(ctx, ctx->buffers.sel_id);
}

void sg_imgui_draw_images_content(sg_imgui_t* ctx) {
    SOKOL_ASSERT(ctx && (ctx->init_tag == 0xABCDABCD));
    _sg_imgui_draw_image_list(ctx);
    _sg_imgui_draw_image_panel(ctx, ctx->images.sel_id);
}

void sg_imgui_draw_shaders_content(sg_imgui_t* ctx) {
    SOKOL_ASSERT(ctx && (ctx->init_tag == 0xABCDABCD));
    _sg_imgui_draw_shader_list(ctx);
    _sg_imgui_draw_shader_panel(ctx, ctx->shaders.sel_id);
}

void sg_imgui_draw_pipelines_content(sg_imgui_t* ctx) {
    SOKOL_ASSERT(ctx && (ctx->init_tag == 0xABCDABCD));
    _sg_imgui_draw_pipeline_list(ctx);
    _sg_imgui_draw_pipeline_panel(ctx, ctx->pipelines.sel_id);
}

void sg_imgui_draw_passes_content(sg_imgui_t* ctx) {
    SOKOL_ASSERT(ctx && (ctx->init_tag == 0xABCDABCD));
    _sg_imgui_draw_pass_list(ctx);
    _sg_imgui_draw_pass_panel(ctx, ctx->passes.sel_id);
}

#endif /* SOKOL_IMPL */
