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

typedef struct sg_imgui_buffers_t {
    bool open;
} sg_imgui_buffers_t;

typedef struct sg_imgui_images_t {
    bool open;
} sg_imgui_images_t;

typedef struct sg_imgui_shaders_t {
    bool open;
} sg_imgui_shaders_t;

typedef struct sg_imgui_pipelines_t {
    bool open;
} sg_imgui_pipelines_t;

typedef struct sg_imgui_passes_t {
    bool open;
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

/*--- sokol-gfx trace hook functions -----------------------------------------*/
_SOKOL_PRIVATE void _sg_imgui_query_feature(sg_feature feature, bool result, void* user_data) {
    const sg_imgui_t* ctx = (const sg_imgui_t*) user_data;
    SOKOL_ASSERT(ctx);
    if (ctx->hooks.query_feature) {
        ctx->hooks.query_feature(feature, result, ctx->hooks.user_data);
    }
}

_SOKOL_PRIVATE void _sg_imgui_reset_state_cache(void* user_data) {
    const sg_imgui_t* ctx = (const sg_imgui_t*) user_data;
    SOKOL_ASSERT(ctx);
    if (ctx->hooks.reset_state_cache) {
        ctx->hooks.reset_state_cache(ctx->hooks.user_data);
    }
}

_SOKOL_PRIVATE void _sg_imgui_make_buffer(const sg_buffer_desc* desc, sg_buffer result, void* user_data) {
    const sg_imgui_t* ctx = (const sg_imgui_t*) user_data;
    SOKOL_ASSERT(ctx);
    if (ctx->hooks.make_buffer) {
        ctx->hooks.make_buffer(desc, result, ctx->hooks.user_data);
    }
}

_SOKOL_PRIVATE void _sg_imgui_make_image(const sg_image_desc* desc, sg_image result, void* user_data) {
    const sg_imgui_t* ctx = (const sg_imgui_t*) user_data;
    SOKOL_ASSERT(ctx);
    if (ctx->hooks.make_image) {
        ctx->hooks.make_image(desc, result, ctx->hooks.user_data);
    }
}

_SOKOL_PRIVATE void _sg_imgui_make_shader(const sg_shader_desc* desc, sg_shader result, void* user_data) {
    const sg_imgui_t* ctx = (const sg_imgui_t*) user_data;
    SOKOL_ASSERT(ctx);
    if (ctx->hooks.make_shader) {
        ctx->hooks.make_shader(desc, result, ctx->hooks.user_data);
    }
}

_SOKOL_PRIVATE void _sg_imgui_make_pipeline(const sg_pipeline_desc* desc, sg_pipeline result, void* user_data) {
    const sg_imgui_t* ctx = (const sg_imgui_t*) user_data;
    SOKOL_ASSERT(ctx);
    if (ctx->hooks.make_pipeline) {
        ctx->hooks.make_pipeline(desc, result, ctx->hooks.user_data);
    }
}

_SOKOL_PRIVATE void _sg_imgui_make_pass(const sg_pass_desc* desc, sg_pass result, void* user_data) {
    const sg_imgui_t* ctx = (const sg_imgui_t*) user_data;
    SOKOL_ASSERT(ctx);
    if (ctx->hooks.make_pass) {
        ctx->hooks.make_pass(desc, result, ctx->hooks.user_data);
    }
}

_SOKOL_PRIVATE void _sg_imgui_destroy_buffer(sg_buffer buf, void* user_data) {
    const sg_imgui_t* ctx = (const sg_imgui_t*) user_data;
    SOKOL_ASSERT(ctx);
    if (ctx->hooks.destroy_buffer) {
        ctx->hooks.destroy_buffer(buf, ctx->hooks.user_data);
    }
}

_SOKOL_PRIVATE void _sg_imgui_destroy_image(sg_image img, void* user_data) {
    const sg_imgui_t* ctx = (const sg_imgui_t*) user_data;
    SOKOL_ASSERT(ctx);
    if (ctx->hooks.destroy_image) {
        ctx->hooks.destroy_image(img, ctx->hooks.user_data);
    }
}

_SOKOL_PRIVATE void _sg_imgui_destroy_shader(sg_shader shd, void* user_data) {
    const sg_imgui_t* ctx = (const sg_imgui_t*) user_data;
    SOKOL_ASSERT(ctx);
    if (ctx->hooks.destroy_shader) {
        ctx->hooks.destroy_shader(shd, ctx->hooks.user_data);
    }
}

_SOKOL_PRIVATE void _sg_imgui_destroy_pipeline(sg_pipeline pip, void* user_data) {
    const sg_imgui_t* ctx = (const sg_imgui_t*) user_data;
    SOKOL_ASSERT(ctx);
    if (ctx->hooks.destroy_pipeline) {
        ctx->hooks.destroy_pipeline(pip, ctx->hooks.user_data);
    }
}

_SOKOL_PRIVATE void _sg_imgui_destroy_pass(sg_pass pass, void* user_data) {
    const sg_imgui_t* ctx = (const sg_imgui_t*) user_data;
    SOKOL_ASSERT(ctx);
    if (ctx->hooks.destroy_pass) {
        ctx->hooks.destroy_pass(pass, ctx->hooks.user_data);
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
    const sg_imgui_t* ctx = (const sg_imgui_t*) user_data;
    SOKOL_ASSERT(ctx);
    if (ctx->hooks.init_buffer) {
        ctx->hooks.init_buffer(buf_id, desc, ctx->hooks.user_data);
    }
}

_SOKOL_PRIVATE void _sg_imgui_init_image(sg_image img_id, const sg_image_desc* desc, void* user_data) {
    const sg_imgui_t* ctx = (const sg_imgui_t*) user_data;
    SOKOL_ASSERT(ctx);
    if (ctx->hooks.init_image) {
        ctx->hooks.init_image(img_id, desc, ctx->hooks.user_data);
    }
}

_SOKOL_PRIVATE void _sg_imgui_init_shader(sg_shader shd_id, const sg_shader_desc* desc, void* user_data) {
    const sg_imgui_t* ctx = (const sg_imgui_t*) user_data;
    SOKOL_ASSERT(ctx);
    if (ctx->hooks.init_shader) {
        ctx->hooks.init_shader(shd_id, desc, ctx->hooks.user_data);
    }
}

_SOKOL_PRIVATE void _sg_imgui_init_pipeline(sg_pipeline pip_id, const sg_pipeline_desc* desc, void* user_data) {
    const sg_imgui_t* ctx = (const sg_imgui_t*) user_data;
    SOKOL_ASSERT(ctx);
    if (ctx->hooks.init_pipeline) {
        ctx->hooks.init_pipeline(pip_id, desc, ctx->hooks.user_data);
    }
}

_SOKOL_PRIVATE void _sg_imgui_init_pass(sg_pass pass_id, const sg_pass_desc* desc, void* user_data) {
    const sg_imgui_t* ctx = (const sg_imgui_t*) user_data;
    SOKOL_ASSERT(ctx);
    if (ctx->hooks.init_pass) {
        ctx->hooks.init_pass(pass_id, desc, ctx->hooks.user_data);
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
}

void sg_imgui_discard(sg_imgui_t* ctx) {
    SOKOL_ASSERT(ctx && (ctx->init_tag == 0xABCDABCD));
    ctx->init_tag = 0;
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
    ImGui::SetNextWindowSize(ImVec2(400, 400), ImGuiSetCond_Once);
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
    ImGui::SetNextWindowSize(ImVec2(400, 400), ImGuiSetCond_Once);
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
    ImGui::SetNextWindowSize(ImVec2(400, 400), ImGuiSetCond_Once);
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
    ImGui::SetNextWindowSize(ImVec2(400, 400), ImGuiSetCond_Once);
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
    ImGui::SetNextWindowSize(ImVec2(400, 400), ImGuiSetCond_Once);
    if (ImGui::Begin("Passes", &ctx->passes.open)) {
        sg_imgui_draw_passes_content(ctx);
    }
    ImGui::End();
}

void sg_imgui_draw_buffers_content(sg_imgui_t* ctx) {
    SOKOL_ASSERT(ctx && (ctx->init_tag == 0xABCDABCD));
    ImGui::Text("FIXME!");
}

void sg_imgui_draw_images_content(sg_imgui_t* ctx) {
    SOKOL_ASSERT(ctx && (ctx->init_tag == 0xABCDABCD));
    ImGui::Text("FIXME!");
}

void sg_imgui_draw_shaders_content(sg_imgui_t* ctx) {
    SOKOL_ASSERT(ctx && (ctx->init_tag == 0xABCDABCD));
    ImGui::Text("FIXME!");
}

void sg_imgui_draw_pipelines_content(sg_imgui_t* ctx) {
    SOKOL_ASSERT(ctx && (ctx->init_tag == 0xABCDABCD));
    ImGui::Text("FIXME!");
}

void sg_imgui_draw_passes_content(sg_imgui_t* ctx) {
    SOKOL_ASSERT(ctx && (ctx->init_tag == 0xABCDABCD));
    ImGui::Text("FIXME!");
}

#endif /* SOKOL_IMPL */
