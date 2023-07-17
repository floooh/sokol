//------------------------------------------------------------------------------
//  sokol-gl-test.c
//------------------------------------------------------------------------------
#include "sokol_gfx.h"
#define SOKOL_GL_IMPL
#include "sokol_gl.h"
#include "utest.h"
#include <float.h>

#define T(b) EXPECT_TRUE(b)
#define TFLT(f0,f1,epsilon) {T(fabs((f0)-(f1))<=(epsilon));}

static void init(void) {
    sg_setup(&(sg_desc){0});
    sgl_setup(&(sgl_desc_t){0});
}

static void shutdown(void) {
    sgl_shutdown();
    sg_shutdown();
}

UTEST(sokol_gl, default_init_shutdown) {
    init();
    T(_sgl.init_cookie == _SGL_INIT_COOKIE);
    T(_sgl.def_ctx_id.id == SGL_DEFAULT_CONTEXT.id);
    T(_sgl.cur_ctx_id.id == _sgl.def_ctx_id.id);
    T(_sgl.cur_ctx);
    T(_sgl.cur_ctx->vertices.cap == 65536);
    T(_sgl.cur_ctx->commands.cap == 16384);
    T(_sgl.cur_ctx->uniforms.cap == 16384);
    T(_sgl.cur_ctx->vertices.next == 0);
    T(_sgl.cur_ctx->commands.next == 0);
    T(_sgl.cur_ctx->uniforms.next == 0);
    T(_sgl.cur_ctx->vertices.ptr != 0);
    T(_sgl.cur_ctx->uniforms.ptr != 0);
    T(_sgl.cur_ctx->commands.ptr != 0);
    T(_sgl.cur_ctx->error == SGL_NO_ERROR);
    T(!_sgl.cur_ctx->in_begin);
    T(_sgl.cur_ctx->def_pip.id != SG_INVALID_ID);
    T(_sgl.pip_pool.pool.size == (_SGL_DEFAULT_PIPELINE_POOL_SIZE + 1));
    TFLT(_sgl.cur_ctx->u, 0.0f, FLT_MIN);
    TFLT(_sgl.cur_ctx->v, 0.0f, FLT_MIN);
    T(_sgl.cur_ctx->rgba == 0xFFFFFFFF);
    T(_sgl.cur_ctx->cur_img.id == _sgl.def_img.id);
    shutdown();
}

UTEST(sokol_gl, viewport) {
    init();
    sgl_viewport(1, 2, 3, 4, true);
    T(_sgl.cur_ctx->commands.next == 1);
    T(_sgl.cur_ctx->commands.ptr[0].cmd == SGL_COMMAND_VIEWPORT);
    T(_sgl.cur_ctx->commands.ptr[0].args.viewport.x == 1);
    T(_sgl.cur_ctx->commands.ptr[0].args.viewport.y == 2);
    T(_sgl.cur_ctx->commands.ptr[0].args.viewport.w == 3);
    T(_sgl.cur_ctx->commands.ptr[0].args.viewport.h == 4);
    T(_sgl.cur_ctx->commands.ptr[0].args.viewport.origin_top_left);
    sgl_viewport(5, 6, 7, 8, false);
    T(_sgl.cur_ctx->commands.next == 2);
    T(_sgl.cur_ctx->commands.ptr[1].cmd == SGL_COMMAND_VIEWPORT);
    T(_sgl.cur_ctx->commands.ptr[1].args.viewport.x == 5);
    T(_sgl.cur_ctx->commands.ptr[1].args.viewport.y == 6);
    T(_sgl.cur_ctx->commands.ptr[1].args.viewport.w == 7);
    T(_sgl.cur_ctx->commands.ptr[1].args.viewport.h == 8);
    T(!_sgl.cur_ctx->commands.ptr[1].args.viewport.origin_top_left);
    shutdown();
}

UTEST(sokol_gl, scissor_rect) {
    init();
    sgl_scissor_rect(10, 20, 30, 40, true);
    T(_sgl.cur_ctx->commands.next == 1);
    T(_sgl.cur_ctx->commands.ptr[0].cmd == SGL_COMMAND_SCISSOR_RECT);
    T(_sgl.cur_ctx->commands.ptr[0].args.scissor_rect.x == 10);
    T(_sgl.cur_ctx->commands.ptr[0].args.scissor_rect.y == 20);
    T(_sgl.cur_ctx->commands.ptr[0].args.scissor_rect.w == 30);
    T(_sgl.cur_ctx->commands.ptr[0].args.scissor_rect.h == 40);
    T(_sgl.cur_ctx->commands.ptr[0].args.scissor_rect.origin_top_left);
    sgl_scissor_rect(50, 60, 70, 80, false);
    T(_sgl.cur_ctx->commands.next == 2);
    T(_sgl.cur_ctx->commands.ptr[1].cmd == SGL_COMMAND_SCISSOR_RECT);
    T(_sgl.cur_ctx->commands.ptr[1].args.scissor_rect.x == 50);
    T(_sgl.cur_ctx->commands.ptr[1].args.scissor_rect.y == 60);
    T(_sgl.cur_ctx->commands.ptr[1].args.scissor_rect.w == 70);
    T(_sgl.cur_ctx->commands.ptr[1].args.scissor_rect.h == 80);
    T(!_sgl.cur_ctx->commands.ptr[1].args.scissor_rect.origin_top_left);
    shutdown();
}

UTEST(sokol_gl, texture) {
    init();
    T(_sgl.cur_ctx->cur_img.id == _sgl.def_img.id);
    uint32_t pixels[64] = { 0 };
    sg_image img = sg_make_image(&(sg_image_desc){
        .type = SG_IMAGETYPE_2D,
        .width = 8,
        .height = 8,
        .data.subimage[0][0] = SG_RANGE(pixels),
    });
    sg_sampler smp = sg_make_sampler(&(sg_sampler_desc){0});
    sgl_texture(img, smp);
    T(_sgl.cur_ctx->cur_img.id == img.id);
    T(_sgl.cur_ctx->cur_smp.id == smp.id);
    shutdown();
}

UTEST(sokol_gl, texture_image_nosampler) {
    init();
    T(_sgl.cur_ctx->cur_img.id == _sgl.def_img.id);
    uint32_t pixels[64] = { 0 };
    sg_image img = sg_make_image(&(sg_image_desc){
        .type = SG_IMAGETYPE_2D,
        .width = 8,
        .height = 8,
        .data.subimage[0][0] = SG_RANGE(pixels),
    });
    sgl_texture(img, (sg_sampler){0});
    T(_sgl.cur_ctx->cur_img.id == img.id);
    T(_sgl.cur_ctx->cur_smp.id == _sgl.def_smp.id);
    shutdown();
}

UTEST(sokol_gl, texture_noimage_sampler) {
    init();
    T(_sgl.cur_ctx->cur_img.id == _sgl.def_img.id);
    sg_sampler smp = sg_make_sampler(&(sg_sampler_desc){0});
    sgl_texture((sg_image){0}, smp);
    T(_sgl.cur_ctx->cur_img.id == _sgl.def_img.id);
    T(_sgl.cur_ctx->cur_smp.id == smp.id);
    shutdown();
}

UTEST(sokol_gl, texture_noimage_nosampler) {
    init();
    T(_sgl.cur_ctx->cur_img.id == _sgl.def_img.id);
    sgl_texture((sg_image){0}, (sg_sampler){0});
    T(_sgl.cur_ctx->cur_img.id == _sgl.def_img.id);
    T(_sgl.cur_ctx->cur_smp.id == _sgl.def_smp.id);
    shutdown();
}
UTEST(sokol_gl, begin_end) {
    init();
    sgl_begin_triangles();
    sgl_v3f(1.0f, 2.0f, 3.0f);
    sgl_v3f(4.0f, 5.0f, 6.0f);
    sgl_v3f(7.0f, 8.0f, 9.0f);
    sgl_end();
    T(_sgl.cur_ctx->base_vertex == 0);
    T(_sgl.cur_ctx->vertices.next == 3);
    T(_sgl.cur_ctx->commands.next == 1);
    T(_sgl.cur_ctx->uniforms.next == 1);
    T(_sgl.cur_ctx->commands.ptr[0].cmd == SGL_COMMAND_DRAW);
    T(_sgl.cur_ctx->commands.ptr[0].args.draw.pip.id == _sgl_pipeline_at(_sgl.cur_ctx->def_pip.id)->pip[SGL_PRIMITIVETYPE_TRIANGLES].id);
    T(_sgl.cur_ctx->commands.ptr[0].args.draw.base_vertex == 0);
    T(_sgl.cur_ctx->commands.ptr[0].args.draw.num_vertices == 3);
    T(_sgl.cur_ctx->commands.ptr[0].args.draw.uniform_index == 0);
    shutdown();
}

UTEST(sokol_gl, matrix_mode) {
    init();
    sgl_matrix_mode_modelview(); T(_sgl.cur_ctx->cur_matrix_mode == SGL_MATRIXMODE_MODELVIEW);
    sgl_matrix_mode_projection(); T(_sgl.cur_ctx->cur_matrix_mode == SGL_MATRIXMODE_PROJECTION);
    sgl_matrix_mode_texture(); T(_sgl.cur_ctx->cur_matrix_mode == SGL_MATRIXMODE_TEXTURE);
    shutdown();
}

UTEST(sokol_gl, load_identity) {
    init();
    sgl_load_identity();
    const _sgl_matrix_t* m = _sgl_matrix_modelview(_sgl.cur_ctx);
    TFLT(m->v[0][0], 1.0f, FLT_MIN); TFLT(m->v[0][1], 0.0f, FLT_MIN); TFLT(m->v[0][2], 0.0f, FLT_MIN); TFLT(m->v[0][3], 0.0f, FLT_MIN);
    TFLT(m->v[1][0], 0.0f, FLT_MIN); TFLT(m->v[1][1], 1.0f, FLT_MIN); TFLT(m->v[1][2], 0.0f, FLT_MIN); TFLT(m->v[1][3], 0.0f, FLT_MIN);
    TFLT(m->v[2][0], 0.0f, FLT_MIN); TFLT(m->v[2][1], 0.0f, FLT_MIN); TFLT(m->v[2][2], 1.0f, FLT_MIN); TFLT(m->v[2][3], 0.0f, FLT_MIN);
    TFLT(m->v[3][0], 0.0f, FLT_MIN); TFLT(m->v[3][1], 0.0f, FLT_MIN); TFLT(m->v[3][2], 0.0f, FLT_MIN); TFLT(m->v[3][3], 1.0f, FLT_MIN);
    shutdown();
}

UTEST(sokol_gl, load_matrix) {
    init();
    const float m[16] = {
        0.5f, 0.0f, 0.0f, 0.0f,
        0.0f, 0.5f, 0.0f, 0.0f,
        0.0f, 0.0f, 0.5f, 0.0f,
        2.0f, 3.0f, 4.0f, 1.0f
    };
    sgl_load_matrix(m);
    const _sgl_matrix_t* m0 = _sgl_matrix_modelview(_sgl.cur_ctx);
    TFLT(m0->v[0][0], 0.5f, FLT_MIN);
    TFLT(m0->v[1][1], 0.5f, FLT_MIN);
    TFLT(m0->v[2][2], 0.5f, FLT_MIN);
    TFLT(m0->v[3][0], 2.0f, FLT_MIN);
    TFLT(m0->v[3][1], 3.0f, FLT_MIN);
    TFLT(m0->v[3][2], 4.0f, FLT_MIN);
    TFLT(m0->v[3][3], 1.0f, FLT_MIN);
    sgl_load_transpose_matrix(m);
    const _sgl_matrix_t* m1 = _sgl_matrix_modelview(_sgl.cur_ctx);
    TFLT(m1->v[0][0], 0.5f, FLT_MIN);
    TFLT(m1->v[1][1], 0.5f, FLT_MIN);
    TFLT(m1->v[2][2], 0.5f, FLT_MIN);
    TFLT(m1->v[0][3], 2.0f, FLT_MIN);
    TFLT(m1->v[1][3], 3.0f, FLT_MIN);
    TFLT(m1->v[2][3], 4.0f, FLT_MIN);
    TFLT(m1->v[3][3], 1.0f, FLT_MIN);
    shutdown();
}

UTEST(sokol_gl, make_destroy_pipelines) {
    sg_setup(&(sg_desc){0});
    sgl_setup(&(sgl_desc_t){
        /* one pool slot is used by soko-gl itself */
        .pipeline_pool_size = 4
    });

    sgl_pipeline pip[3] = { {0} };
    sg_pipeline_desc desc = {
        .depth = {
            .write_enabled = true,
            .compare = SG_COMPAREFUNC_LESS_EQUAL
        }
    };
    for (int i = 0; i < 3; i++) {
        pip[i] = sgl_make_pipeline(&desc);
        T(pip[i].id != SG_INVALID_ID);
        T((2-i) == _sgl.pip_pool.pool.queue_top);
        const _sgl_pipeline_t* pip_ptr = _sgl_lookup_pipeline(pip[i].id);
        T(pip_ptr);
        T(pip_ptr->slot.id == pip[i].id);
        T(pip_ptr->slot.state == SG_RESOURCESTATE_VALID);
    }
    /* trying to create another one fails because buffer is exhausted */
    T(sgl_make_pipeline(&desc).id == SG_INVALID_ID);

    for (int i = 0; i < 3; i++) {
        sgl_destroy_pipeline(pip[i]);
        T(0 == _sgl_lookup_pipeline(pip[i].id));
        const _sgl_pipeline_t* pip_ptr = _sgl_pipeline_at(pip[i].id);
        T(pip_ptr);
        T(pip_ptr->slot.id == SG_INVALID_ID);
        T(pip_ptr->slot.state == SG_RESOURCESTATE_INITIAL);
        T((i+1) == _sgl.pip_pool.pool.queue_top);
    }
    sgl_shutdown();
    sg_shutdown();
}

UTEST(sokol_gl, make_destroy_contexts) {
    init();
    sgl_context ctx = sgl_make_context(&(sgl_context_desc_t){
        .max_vertices = 1024,
        .max_commands = 256,
        .color_format = SG_PIXELFORMAT_RG8,
        .depth_format = SG_PIXELFORMAT_NONE,
        .sample_count = 4,
    });
    T(ctx.id != SG_INVALID_ID);
    T(ctx.id != SGL_DEFAULT_CONTEXT.id);
    // creating a context should not change the current context
    T(ctx.id != _sgl.cur_ctx_id.id);
    sgl_set_context(ctx);
    T(_sgl.cur_ctx->vertices.cap == 1024);
    T(_sgl.cur_ctx->commands.cap == 256);
    T(_sgl.cur_ctx->uniforms.cap == 256);
    T(ctx.id == _sgl.cur_ctx_id.id);
    T(sgl_get_context().id == ctx.id);
    sgl_set_context(SGL_DEFAULT_CONTEXT);
    T(sgl_get_context().id == SGL_DEFAULT_CONTEXT.id);
    sgl_destroy_context(ctx);
    shutdown();
}

UTEST(sokol_gl, destroy_active_context) {
    init();
    sgl_context ctx = sgl_make_context(&(sgl_context_desc_t){
        .max_vertices = 1024,
        .max_commands = 256,
        .color_format = SG_PIXELFORMAT_RG8,
        .depth_format = SG_PIXELFORMAT_NONE,
        .sample_count = 4,
    });
    sgl_set_context(ctx);
    sgl_destroy_context(ctx);
    T(_sgl.cur_ctx == 0);
    T(sgl_error() == SGL_ERROR_NO_CONTEXT);
    shutdown();
}

UTEST(sokol_gl, context_pipeline) {
    init();
    sgl_context ctx1 = sgl_make_context(&(sgl_context_desc_t){
        .max_vertices = 1024,
        .max_commands = 256,
        .color_format = SG_PIXELFORMAT_R8,
        .depth_format = SG_PIXELFORMAT_NONE,
        .sample_count = 4,
    });
    sgl_context ctx2 = sgl_make_context(&(sgl_context_desc_t){
        .max_vertices = 1024,
        .max_commands = 256,
        .color_format = SG_PIXELFORMAT_RG8,
        .depth_format = SG_PIXELFORMAT_NONE,
        .sample_count = 2,
    });
    sgl_set_context(ctx1);
    sgl_pipeline pip1 = sgl_make_pipeline(&(sg_pipeline_desc){
        .colors[0].blend.enabled = true,
    });
    T(pip1.id != SG_INVALID_ID);
    // FIXME: currently sg_query_pipeline_info() doesn't provide enough information

    sgl_pipeline pip2 = sgl_context_make_pipeline(ctx2, &(sg_pipeline_desc){
        .alpha_to_coverage_enabled = true,
    });
    T(pip2.id != SG_INVALID_ID);
    shutdown();
}

UTEST(sokol_gl, default_context) {
    init();
    T(sgl_default_context().id == SGL_DEFAULT_CONTEXT.id);
    shutdown();
}
