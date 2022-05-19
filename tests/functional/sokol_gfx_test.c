//------------------------------------------------------------------------------
//  sokol-gfx-test.c
//  NOTE: this is not only testing the public API behaviour, but also
//  accesses private functions and data. It may make sense to split
//  these into two separate tests.
//------------------------------------------------------------------------------
#include "force_dummy_backend.h"
#define SOKOL_IMPL
#include "sokol_gfx.h"
#include "utest.h"

#define T(b) EXPECT_TRUE(b)

UTEST(sokol_gfx, init_shutdown) {
    sg_setup(&(sg_desc){0});
    T(sg_isvalid());
    sg_shutdown();
    T(!sg_isvalid());
}

UTEST(sokol_gfx, query_desc) {
    sg_setup(&(sg_desc){
        .buffer_pool_size = 1024,
        .shader_pool_size = 128,
        .pass_pool_size = 64,
    });
    const sg_desc desc = sg_query_desc();
    T(desc.buffer_pool_size == 1024);
    T(desc.image_pool_size == _SG_DEFAULT_IMAGE_POOL_SIZE);
    T(desc.shader_pool_size == 128);
    T(desc.pipeline_pool_size == _SG_DEFAULT_PIPELINE_POOL_SIZE);
    T(desc.pass_pool_size == 64);
    T(desc.context_pool_size == _SG_DEFAULT_CONTEXT_POOL_SIZE);
    T(desc.uniform_buffer_size == _SG_DEFAULT_UB_SIZE);
    T(desc.sampler_cache_size == _SG_DEFAULT_SAMPLER_CACHE_CAPACITY);
    sg_shutdown();
}

UTEST(sokol_gfx, query_backend) {
    sg_setup(&(sg_desc){0});
    T(sg_query_backend() == SG_BACKEND_DUMMY);
    sg_shutdown();
}

UTEST(sokol_gfx, pool_size) {
    sg_setup(&(sg_desc){
        .buffer_pool_size = 1024,
        .image_pool_size = 2048,
        .shader_pool_size = 128,
        .pipeline_pool_size = 256,
        .pass_pool_size = 64,
        .context_pool_size = 64
    });
    T(sg_isvalid());
    /* pool slot 0 is reserved (this is the "invalid slot") */
    T(_sg.pools.buffer_pool.size == 1025);
    T(_sg.pools.image_pool.size == 2049);
    T(_sg.pools.shader_pool.size == 129);
    T(_sg.pools.pipeline_pool.size == 257);
    T(_sg.pools.pass_pool.size == 65);
    T(_sg.pools.context_pool.size == 65);
    T(_sg.pools.buffer_pool.queue_top == 1024);
    T(_sg.pools.image_pool.queue_top == 2048);
    T(_sg.pools.shader_pool.queue_top == 128);
    T(_sg.pools.pipeline_pool.queue_top == 256);
    T(_sg.pools.pass_pool.queue_top == 64);
    T(_sg.pools.context_pool.queue_top == 63);  /* default context has been created already */
    sg_shutdown();
}

UTEST(sokol_gfx, alloc_fail_destroy_buffers) {
    sg_setup(&(sg_desc){
        .buffer_pool_size = 3
    });
    T(sg_isvalid());

    sg_buffer buf[3] = { {0} };
    for (int i = 0; i < 3; i++) {
        buf[i] = sg_alloc_buffer();
        T(buf[i].id != SG_INVALID_ID);
        T((2-i) == _sg.pools.buffer_pool.queue_top);
        T(sg_query_buffer_state(buf[i]) == SG_RESOURCESTATE_ALLOC);
    }
    /* the next alloc will fail because the pool is exhausted */
    sg_buffer b3 = sg_alloc_buffer();
    T(b3.id == SG_INVALID_ID);
    T(sg_query_buffer_state(b3) == SG_RESOURCESTATE_INVALID);

    /* before destroying, the resources must be either in valid or failed state */
    for (int i = 0; i < 3; i++) {
        sg_fail_buffer(buf[i]);
        T(sg_query_buffer_state(buf[i]) == SG_RESOURCESTATE_FAILED);
    }
    for (int i = 0; i < 3; i++) {
        sg_destroy_buffer(buf[i]);
        T(sg_query_buffer_state(buf[i]) == SG_RESOURCESTATE_INVALID);
        T((i+1) == _sg.pools.buffer_pool.queue_top);
    }
    sg_shutdown();
}

UTEST(sokol_gfx, alloc_fail_destroy_images) {
    sg_setup(&(sg_desc){
        .image_pool_size = 3
    });
    T(sg_isvalid());

    sg_image img[3] = { {0} };
    for (int i = 0; i < 3; i++) {
        img[i] = sg_alloc_image();
        T(img[i].id != SG_INVALID_ID);
        T((2-i) == _sg.pools.image_pool.queue_top);
        T(sg_query_image_state(img[i]) == SG_RESOURCESTATE_ALLOC);
    }
    /* the next alloc will fail because the pool is exhausted */
    sg_image i3 = sg_alloc_image();
    T(i3.id == SG_INVALID_ID);
    T(sg_query_image_state(i3) == SG_RESOURCESTATE_INVALID);

    /* before destroying, the resources must be either in valid or failed state */
    for (int i = 0; i < 3; i++) {
        sg_fail_image(img[i]);
        T(sg_query_image_state(img[i]) == SG_RESOURCESTATE_FAILED);
    }
    for (int i = 0; i < 3; i++) {
        sg_destroy_image(img[i]);
        T(sg_query_image_state(img[i]) == SG_RESOURCESTATE_INVALID);
        T((i+1) == _sg.pools.image_pool.queue_top);
    }
    sg_shutdown();
}

UTEST(sokol_gfx, alloc_fail_destroy_shaders) {
    sg_setup(&(sg_desc){
        .shader_pool_size = 3
    });
    T(sg_isvalid());

    sg_shader shd[3] = { {0} };
    for (int i = 0; i < 3; i++) {
        shd[i] = sg_alloc_shader();
        T(shd[i].id != SG_INVALID_ID);
        T((2-i) == _sg.pools.shader_pool.queue_top);
        T(sg_query_shader_state(shd[i]) == SG_RESOURCESTATE_ALLOC);
    }
    /* the next alloc will fail because the pool is exhausted */
    sg_shader s3 = sg_alloc_shader();
    T(s3.id == SG_INVALID_ID);
    T(sg_query_shader_state(s3) == SG_RESOURCESTATE_INVALID);

    /* before destroying, the resources must be either in valid or failed state */
    for (int i = 0; i < 3; i++) {
        sg_fail_shader(shd[i]);
        T(sg_query_shader_state(shd[i]) == SG_RESOURCESTATE_FAILED);
    }
    for (int i = 0; i < 3; i++) {
        sg_destroy_shader(shd[i]);
        T(sg_query_shader_state(shd[i]) == SG_RESOURCESTATE_INVALID);
        T((i+1) == _sg.pools.shader_pool.queue_top);
    }
    sg_shutdown();
}

UTEST(sokol_gfx, alloc_fail_destroy_pipelines) {
    sg_setup(&(sg_desc){
        .pipeline_pool_size = 3
    });
    T(sg_isvalid());

    sg_pipeline pip[3] = { {0} };
    for (int i = 0; i < 3; i++) {
        pip[i] = sg_alloc_pipeline();
        T(pip[i].id != SG_INVALID_ID);
        T((2-i) == _sg.pools.pipeline_pool.queue_top);
        T(sg_query_pipeline_state(pip[i]) == SG_RESOURCESTATE_ALLOC);
    }

    /* the next alloc will fail because the pool is exhausted */
    sg_pipeline p3 = sg_alloc_pipeline();
    T(p3.id == SG_INVALID_ID);
    T(sg_query_pipeline_state(p3) == SG_RESOURCESTATE_INVALID);

    /* before destroying, the resources must be either in valid or failed state */
    for (int i = 0; i < 3; i++) {
        sg_fail_pipeline(pip[i]);
        T(sg_query_pipeline_state(pip[i]) == SG_RESOURCESTATE_FAILED);
    }
    for (int i = 0; i < 3; i++) {
        sg_destroy_pipeline(pip[i]);
        T(sg_query_pipeline_state(pip[i]) == SG_RESOURCESTATE_INVALID);
        T((i+1) == _sg.pools.pipeline_pool.queue_top);
    }
    sg_shutdown();
}

UTEST(sokol_gfx, alloc_fail_destroy_passes) {
    sg_setup(&(sg_desc){
        .pass_pool_size = 3
    });
    T(sg_isvalid());

    sg_pass pass[3] = { {0} };
    for (int i = 0; i < 3; i++) {
        pass[i] = sg_alloc_pass();
        T(pass[i].id != SG_INVALID_ID);
        T((2-i) == _sg.pools.pass_pool.queue_top);
        T(sg_query_pass_state(pass[i]) == SG_RESOURCESTATE_ALLOC);
    }
    /* the next alloc will fail because the pool is exhausted */
    sg_pass p3 = sg_alloc_pass();
    T(p3.id == SG_INVALID_ID);
    T(sg_query_pass_state(p3) == SG_RESOURCESTATE_INVALID);

    /* before destroying, the resources must be either in valid or failed state */
    for (int i = 0; i < 3; i++) {
        sg_fail_pass(pass[i]);
        T(sg_query_pass_state(pass[i]) == SG_RESOURCESTATE_FAILED);
    }
    for (int i = 0; i < 3; i++) {
        sg_destroy_pass(pass[i]);
        T(sg_query_pass_state(pass[i]) == SG_RESOURCESTATE_INVALID);
        T((i+1) == _sg.pools.pass_pool.queue_top);
    }
    sg_shutdown();
}

UTEST(sokol_gfx, make_destroy_buffers) {
    sg_setup(&(sg_desc){
        .buffer_pool_size = 3
    });
    T(sg_isvalid());

    float data[] = { 1.0f, 2.0f, 3.0f, 4.0f };

    sg_buffer buf[3] = { {0} };
    sg_buffer_desc desc = { .data = SG_RANGE(data) };
    for (int i = 0; i < 3; i++) {
        buf[i] = sg_make_buffer(&desc);
        T(buf[i].id != SG_INVALID_ID);
        T((2-i) == _sg.pools.buffer_pool.queue_top);
        T(sg_query_buffer_state(buf[i]) == SG_RESOURCESTATE_VALID);
        const _sg_buffer_t* bufptr = _sg_lookup_buffer(&_sg.pools, buf[i].id);
        T(bufptr);
        T(bufptr->slot.id == buf[i].id);
        T(bufptr->slot.ctx_id == _sg.active_context.id);
        T(bufptr->slot.state == SG_RESOURCESTATE_VALID);
        T(bufptr->cmn.size == sizeof(data));
        T(bufptr->cmn.append_pos == 0);
        T(!bufptr->cmn.append_overflow);
        T(bufptr->cmn.type == SG_BUFFERTYPE_VERTEXBUFFER);
        T(bufptr->cmn.usage == SG_USAGE_IMMUTABLE);
        T(bufptr->cmn.update_frame_index == 0);
        T(bufptr->cmn.append_frame_index == 0);
        T(bufptr->cmn.num_slots == 1);
        T(bufptr->cmn.active_slot == 0);
    }
    /* trying to create another one fails because buffer is exhausted */
    T(sg_make_buffer(&desc).id == SG_INVALID_ID);

    for (int i = 0; i < 3; i++) {
        sg_destroy_buffer(buf[i]);
        T(sg_query_buffer_state(buf[i]) == SG_RESOURCESTATE_INVALID);
        T((i+1) == _sg.pools.buffer_pool.queue_top);
    }
    sg_shutdown();
}

UTEST(sokol_gfx, make_destroy_images) {
    sg_setup(&(sg_desc){
        .image_pool_size = 3
    });
    T(sg_isvalid());

    uint32_t data[8*8] = { 0 };

    sg_image img[3] = { {0} };
    sg_image_desc desc = {
        .width = 8,
        .height = 8,
        .data.subimage[0][0] = SG_RANGE(data)
    };
    for (int i = 0; i < 3; i++) {
        img[i] = sg_make_image(&desc);
        T(img[i].id != SG_INVALID_ID);
        T((2-i) == _sg.pools.image_pool.queue_top);
        T(sg_query_image_state(img[i]) == SG_RESOURCESTATE_VALID);
        const _sg_image_t* imgptr = _sg_lookup_image(&_sg.pools, img[i].id);
        T(imgptr);
        T(imgptr->slot.id == img[i].id);
        T(imgptr->slot.ctx_id == _sg.active_context.id);
        T(imgptr->slot.state == SG_RESOURCESTATE_VALID);
        T(imgptr->cmn.type == SG_IMAGETYPE_2D);
        T(!imgptr->cmn.render_target);
        T(imgptr->cmn.width == 8);
        T(imgptr->cmn.height == 8);
        T(imgptr->cmn.num_slices == 1);
        T(imgptr->cmn.num_mipmaps == 1);
        T(imgptr->cmn.usage == SG_USAGE_IMMUTABLE);
        T(imgptr->cmn.pixel_format == SG_PIXELFORMAT_RGBA8);
        T(imgptr->cmn.sample_count == 1);
        T(imgptr->cmn.min_filter == SG_FILTER_NEAREST);
        T(imgptr->cmn.mag_filter == SG_FILTER_NEAREST);
        T(imgptr->cmn.wrap_u == SG_WRAP_REPEAT);
        T(imgptr->cmn.wrap_v == SG_WRAP_REPEAT);
        T(imgptr->cmn.wrap_w == SG_WRAP_REPEAT);
        T(imgptr->cmn.max_anisotropy == 1);
        T(imgptr->cmn.upd_frame_index == 0);
        T(imgptr->cmn.num_slots == 1);
        T(imgptr->cmn.active_slot == 0);
    }
    /* trying to create another one fails because buffer is exhausted */
    T(sg_make_image(&desc).id == SG_INVALID_ID);

    for (int i = 0; i < 3; i++) {
        sg_destroy_image(img[i]);
        T(sg_query_image_state(img[i]) == SG_RESOURCESTATE_INVALID);
        T((i+1) == _sg.pools.image_pool.queue_top);
    }
    sg_shutdown();
}

UTEST(sokol_gfx, make_destroy_shaders) {
    sg_setup(&(sg_desc){
        .shader_pool_size = 3
    });
    T(sg_isvalid());

    sg_shader shd[3] = { {0} };
    sg_shader_desc desc = {
        .vs.uniform_blocks[0] = {
            .size = 16
        }
    };
    for (int i = 0; i < 3; i++) {
        shd[i] = sg_make_shader(&desc);
        T(shd[i].id != SG_INVALID_ID);
        T((2-i) == _sg.pools.shader_pool.queue_top);
        T(sg_query_shader_state(shd[i]) == SG_RESOURCESTATE_VALID);
        const _sg_shader_t* shdptr = _sg_lookup_shader(&_sg.pools, shd[i].id);
        T(shdptr);
        T(shdptr->slot.id == shd[i].id);
        T(shdptr->slot.ctx_id == _sg.active_context.id);
        T(shdptr->slot.state == SG_RESOURCESTATE_VALID);
        T(shdptr->cmn.stage[SG_SHADERSTAGE_VS].num_uniform_blocks == 1);
        T(shdptr->cmn.stage[SG_SHADERSTAGE_VS].num_images == 0);
        T(shdptr->cmn.stage[SG_SHADERSTAGE_VS].uniform_blocks[0].size == 16);
        T(shdptr->cmn.stage[SG_SHADERSTAGE_FS].num_uniform_blocks == 0);
        T(shdptr->cmn.stage[SG_SHADERSTAGE_FS].num_images == 0);
    }
    /* trying to create another one fails because buffer is exhausted */
    T(sg_make_shader(&desc).id == SG_INVALID_ID);

    for (int i = 0; i < 3; i++) {
        sg_destroy_shader(shd[i]);
        T(sg_query_shader_state(shd[i]) == SG_RESOURCESTATE_INVALID);
        T((i+1) == _sg.pools.shader_pool.queue_top);
    }
    sg_shutdown();
}

UTEST(sokol_gfx, make_destroy_pipelines) {
    sg_setup(&(sg_desc){
        .pipeline_pool_size = 3
    });
    T(sg_isvalid());

    sg_pipeline pip[3] = { {0} };
    sg_pipeline_desc desc = {
        .shader = sg_make_shader(&(sg_shader_desc){ 0 }),
        .layout = {
            .attrs = {
                [0] = { .format=SG_VERTEXFORMAT_FLOAT3 },
                [1] = { .format=SG_VERTEXFORMAT_FLOAT4 }
            }
        },
    };
    for (int i = 0; i < 3; i++) {
        pip[i] = sg_make_pipeline(&desc);
        T(pip[i].id != SG_INVALID_ID);
        T((2-i) == _sg.pools.pipeline_pool.queue_top);
        T(sg_query_pipeline_state(pip[i]) == SG_RESOURCESTATE_VALID);
        const _sg_pipeline_t* pipptr = _sg_lookup_pipeline(&_sg.pools, pip[i].id);
        T(pipptr);
        T(pipptr->slot.id == pip[i].id);
        T(pipptr->slot.ctx_id == _sg.active_context.id);
        T(pipptr->slot.state == SG_RESOURCESTATE_VALID);
        T(pipptr->shader == _sg_lookup_shader(&_sg.pools, desc.shader.id));
        T(pipptr->cmn.shader_id.id == desc.shader.id);
        T(pipptr->cmn.color_attachment_count == 1);
        T(pipptr->cmn.color_formats[0] == SG_PIXELFORMAT_RGBA8);
        T(pipptr->cmn.depth_format == SG_PIXELFORMAT_DEPTH_STENCIL);
        T(pipptr->cmn.sample_count == 1);
        T(pipptr->cmn.index_type == SG_INDEXTYPE_NONE);
        T(pipptr->cmn.vertex_layout_valid[0]);
        T(!pipptr->cmn.vertex_layout_valid[1]);
    }
    /* trying to create another one fails because buffer is exhausted */
    T(sg_make_pipeline(&desc).id == SG_INVALID_ID);

    for (int i = 0; i < 3; i++) {
        sg_destroy_pipeline(pip[i]);
        T(sg_query_pipeline_state(pip[i]) == SG_RESOURCESTATE_INVALID);
        T((i+1) == _sg.pools.pipeline_pool.queue_top);
    }
    sg_shutdown();
}

UTEST(sokol_gfx, make_destroy_passes) {
    sg_setup(&(sg_desc){
        .pass_pool_size = 3
    });
    T(sg_isvalid());

    sg_pass pass[3] = { {0} };

    sg_image_desc img_desc = {
        .render_target = true,
        .width = 128,
        .height = 128,
    };
    sg_pass_desc pass_desc = (sg_pass_desc){
        .color_attachments = {
            [0].image = sg_make_image(&img_desc),
            [1].image = sg_make_image(&img_desc),
            [2].image = sg_make_image(&img_desc)
        },
    };
    for (int i = 0; i < 3; i++) {
        pass[i] = sg_make_pass(&pass_desc);
        T(pass[i].id != SG_INVALID_ID);
        T((2-i) == _sg.pools.pass_pool.queue_top);
        T(sg_query_pass_state(pass[i]) == SG_RESOURCESTATE_VALID);
        const _sg_pass_t* passptr = _sg_lookup_pass(&_sg.pools, pass[i].id);
        T(passptr);
        T(passptr->slot.id == pass[i].id);
        T(passptr->slot.ctx_id == _sg.active_context.id);
        T(passptr->slot.state == SG_RESOURCESTATE_VALID);
        T(passptr->cmn.num_color_atts == 3);
        for (int ai = 0; ai < 3; ai++) {
            const _sg_image_t* img = _sg_pass_color_image(passptr, ai);
            T(img == _sg_lookup_image(&_sg.pools, pass_desc.color_attachments[ai].image.id));
            T(passptr->cmn.color_atts[ai].image_id.id == pass_desc.color_attachments[ai].image.id);
        }
    }
    /* trying to create another one fails because buffer is exhausted */
    T(sg_make_pass(&pass_desc).id == SG_INVALID_ID);

    for (int i = 0; i < 3; i++) {
        sg_destroy_pass(pass[i]);
        T(sg_query_pass_state(pass[i]) == SG_RESOURCESTATE_INVALID);
        T((i+1) == _sg.pools.pass_pool.queue_top);
    }
    sg_shutdown();
}

UTEST(sokol_gfx, generation_counter) {
    sg_setup(&(sg_desc){
        .buffer_pool_size = 1,
    });

    static float data[] = { 1.0f, 2.0f, 3.0f, 4.0f };
    for (int i = 0; i < 64; i++) {
        sg_buffer buf = sg_make_buffer(&(sg_buffer_desc){ .data = SG_RANGE(data) });
        T(buf.id != SG_INVALID_ID);
        T(sg_query_buffer_state(buf) == SG_RESOURCESTATE_VALID);
        T((buf.id >> 16) == (uint32_t)(i + 1));   /* this is the generation counter */
        T(_sg_slot_index(buf.id) == 1); /* slot index should remain the same */
        sg_destroy_buffer(buf);
        T(sg_query_buffer_state(buf) == SG_RESOURCESTATE_INVALID);
    }
    sg_shutdown();
}

UTEST(sokol_gfx, query_buffer_defaults) {
    sg_setup(&(sg_desc){0});
    sg_buffer_desc desc;
    desc = sg_query_buffer_defaults(&(sg_buffer_desc){0});
    T(desc.type == SG_BUFFERTYPE_VERTEXBUFFER);
    T(desc.usage == SG_USAGE_IMMUTABLE);
    desc = sg_query_buffer_defaults(&(sg_buffer_desc){
        .type = SG_BUFFERTYPE_INDEXBUFFER,
    });
    T(desc.type == SG_BUFFERTYPE_INDEXBUFFER);
    T(desc.usage == SG_USAGE_IMMUTABLE);
    desc = sg_query_buffer_defaults(&(sg_buffer_desc){
        .usage = SG_USAGE_DYNAMIC
    });
    T(desc.type == SG_BUFFERTYPE_VERTEXBUFFER);
    T(desc.usage == SG_USAGE_DYNAMIC);
    sg_shutdown();
}

UTEST(sokol_gfx, query_image_defaults) {
    sg_setup(&(sg_desc){0});
    const sg_image_desc desc = sg_query_image_defaults(&(sg_image_desc){0});
    T(desc.type == SG_IMAGETYPE_2D);
    T(!desc.render_target);
    T(desc.num_mipmaps == 1);
    T(desc.usage == SG_USAGE_IMMUTABLE);
    T(desc.pixel_format == SG_PIXELFORMAT_RGBA8);
    T(desc.sample_count == 1);
    T(desc.min_filter == SG_FILTER_NEAREST);
    T(desc.mag_filter == SG_FILTER_NEAREST);
    T(desc.wrap_u == SG_WRAP_REPEAT);
    T(desc.wrap_v == SG_WRAP_REPEAT);
    T(desc.wrap_w == SG_WRAP_REPEAT);
    T(desc.max_anisotropy == 1);
    T(desc.max_lod >= FLT_MAX);
    sg_shutdown();
}

UTEST(sokol_gfx, query_shader_defaults) {
    sg_setup(&(sg_desc){0});
    const sg_shader_desc desc = sg_query_shader_defaults(&(sg_shader_desc){0});
    T(0 == strcmp(desc.vs.entry, "main"));
    T(0 == strcmp(desc.fs.entry, "main"));
    sg_shutdown();
}

UTEST(sokol_gfx, query_pipeline_defaults) {
    sg_setup(&(sg_desc){0});
    const sg_pipeline_desc desc = sg_query_pipeline_defaults(&(sg_pipeline_desc){
        .layout.attrs = {
            [0].format = SG_VERTEXFORMAT_FLOAT3,
            [1].format = SG_VERTEXFORMAT_FLOAT4
        }
    });
    T(desc.layout.buffers[0].stride == 28);
    T(desc.layout.buffers[0].step_rate == 1);
    T(desc.layout.buffers[0].step_func == SG_VERTEXSTEP_PER_VERTEX);
    T(desc.layout.attrs[0].offset == 0);
    T(desc.layout.attrs[0].buffer_index == 0);
    T(desc.layout.attrs[0].format == SG_VERTEXFORMAT_FLOAT3);
    T(desc.layout.attrs[1].offset == 12);
    T(desc.layout.attrs[1].buffer_index == 0);
    T(desc.layout.attrs[1].format == SG_VERTEXFORMAT_FLOAT4);
    T(desc.stencil.front.fail_op == SG_STENCILOP_KEEP);
    T(desc.stencil.front.depth_fail_op == SG_STENCILOP_KEEP);
    T(desc.stencil.front.pass_op == SG_STENCILOP_KEEP);
    T(desc.stencil.front.compare == SG_COMPAREFUNC_ALWAYS);
    T(desc.stencil.back.fail_op == SG_STENCILOP_KEEP);
    T(desc.stencil.back.depth_fail_op == SG_STENCILOP_KEEP);
    T(desc.stencil.back.pass_op == SG_STENCILOP_KEEP);
    T(desc.stencil.back.compare == SG_COMPAREFUNC_ALWAYS);
    T(desc.stencil.enabled == false);
    T(desc.stencil.read_mask == 0);
    T(desc.stencil.write_mask == 0);
    T(desc.stencil.ref == 0);
    T(desc.depth.pixel_format == SG_PIXELFORMAT_DEPTH_STENCIL);
    T(desc.depth.compare == SG_COMPAREFUNC_ALWAYS);
    T(desc.depth.write_enabled == false);
    T(desc.depth.bias == 0);
    T(desc.depth.bias_slope_scale == 0);
    T(desc.depth.bias_clamp == 0);
    T(desc.color_count == 1);
    T(desc.colors[0].pixel_format == SG_PIXELFORMAT_RGBA8);
    T(desc.colors[0].write_mask == 0xF);
    T(desc.colors[0].blend.enabled == false);
    T(desc.colors[0].blend.src_factor_rgb == SG_BLENDFACTOR_ONE);
    T(desc.colors[0].blend.dst_factor_rgb == SG_BLENDFACTOR_ZERO);
    T(desc.colors[0].blend.op_rgb == SG_BLENDOP_ADD);
    T(desc.colors[0].blend.src_factor_alpha == SG_BLENDFACTOR_ONE);
    T(desc.colors[0].blend.dst_factor_alpha == SG_BLENDFACTOR_ZERO);
    T(desc.colors[0].blend.op_alpha == SG_BLENDOP_ADD);
    T(desc.alpha_to_coverage_enabled == false);
    T(desc.primitive_type == SG_PRIMITIVETYPE_TRIANGLES);
    T(desc.index_type == SG_INDEXTYPE_NONE);
    T(desc.cull_mode == SG_CULLMODE_NONE);
    T(desc.face_winding == SG_FACEWINDING_CW);
    T(desc.sample_count == 1);
    sg_shutdown();
}

// test that color attachment defaults are set in all attachments
UTEST(sokol_gfx, query_mrt_pipeline_defaults) {
    sg_setup(&(sg_desc){0});
    const sg_pipeline_desc desc = sg_query_pipeline_defaults(&(sg_pipeline_desc){
        .color_count = 3,
    });
    T(desc.color_count == 3);
    for (int i = 0; i < desc.color_count; i++) {
        T(desc.colors[i].pixel_format == SG_PIXELFORMAT_RGBA8);
        T(desc.colors[i].write_mask == 0xF);
        T(desc.colors[i].blend.enabled == false);
        T(desc.colors[i].blend.src_factor_rgb == SG_BLENDFACTOR_ONE);
        T(desc.colors[i].blend.dst_factor_rgb == SG_BLENDFACTOR_ZERO);
        T(desc.colors[i].blend.op_rgb == SG_BLENDOP_ADD);
        T(desc.colors[i].blend.src_factor_alpha == SG_BLENDFACTOR_ONE);
        T(desc.colors[i].blend.dst_factor_alpha == SG_BLENDFACTOR_ZERO);
        T(desc.colors[i].blend.op_alpha == SG_BLENDOP_ADD);
    };
    sg_shutdown();
}

// test that first color attachment values are duplicated to other attachments
UTEST(sokol_gfx, multiple_color_state) {
    sg_setup(&(sg_desc){0});
    const sg_pipeline_desc desc = sg_query_pipeline_defaults(&(sg_pipeline_desc){
        .color_count = 3,
        .colors = {
            [0] = {
                .pixel_format = SG_PIXELFORMAT_R8,
                .write_mask = SG_COLORMASK_BA,
                .blend = {
                    .enabled = true,
                    .src_factor_rgb = SG_BLENDFACTOR_SRC_COLOR,
                    .dst_factor_rgb = SG_BLENDFACTOR_DST_COLOR,
                    .op_rgb = SG_BLENDOP_SUBTRACT,
                    .src_factor_alpha = SG_BLENDFACTOR_SRC_ALPHA,
                    .dst_factor_alpha = SG_BLENDFACTOR_DST_ALPHA,
                    .op_alpha = SG_BLENDOP_REVERSE_SUBTRACT
                }
            },
            [2] = {
                .pixel_format = SG_PIXELFORMAT_RG8,
                .write_mask = SG_COLORMASK_GA,
                .blend = {
                    .enabled = true,
                    .src_factor_rgb = SG_BLENDFACTOR_DST_COLOR,
                    .dst_factor_rgb = SG_BLENDFACTOR_SRC_COLOR,
                    .op_rgb = SG_BLENDOP_REVERSE_SUBTRACT,
                    .src_factor_alpha = SG_BLENDFACTOR_DST_ALPHA,
                    .dst_factor_alpha = SG_BLENDFACTOR_SRC_ALPHA,
                    .op_alpha = SG_BLENDOP_SUBTRACT
                }
            },
        }
    });
    T(desc.color_count == 3);

    T(desc.colors[0].pixel_format == SG_PIXELFORMAT_R8);
    T(desc.colors[0].write_mask == SG_COLORMASK_BA);
    T(desc.colors[0].blend.enabled == true);
    T(desc.colors[0].blend.src_factor_rgb == SG_BLENDFACTOR_SRC_COLOR);
    T(desc.colors[0].blend.dst_factor_rgb == SG_BLENDFACTOR_DST_COLOR);
    T(desc.colors[0].blend.op_rgb == SG_BLENDOP_SUBTRACT);
    T(desc.colors[0].blend.src_factor_alpha == SG_BLENDFACTOR_SRC_ALPHA);
    T(desc.colors[0].blend.dst_factor_alpha == SG_BLENDFACTOR_DST_ALPHA);
    T(desc.colors[0].blend.op_alpha == SG_BLENDOP_REVERSE_SUBTRACT);

    T(desc.colors[1].pixel_format == SG_PIXELFORMAT_RGBA8);
    T(desc.colors[1].write_mask == SG_COLORMASK_RGBA);
    T(desc.colors[1].blend.enabled == false);
    T(desc.colors[1].blend.src_factor_rgb == SG_BLENDFACTOR_ONE);
    T(desc.colors[1].blend.dst_factor_rgb == SG_BLENDFACTOR_ZERO);
    T(desc.colors[1].blend.op_rgb == SG_BLENDOP_ADD);
    T(desc.colors[1].blend.src_factor_alpha == SG_BLENDFACTOR_ONE);
    T(desc.colors[1].blend.dst_factor_alpha == SG_BLENDFACTOR_ZERO);
    T(desc.colors[1].blend.op_alpha == SG_BLENDOP_ADD);

    T(desc.colors[2].pixel_format == SG_PIXELFORMAT_RG8);
    T(desc.colors[2].write_mask == SG_COLORMASK_GA);
    T(desc.colors[2].blend.enabled == true);
    T(desc.colors[2].blend.src_factor_rgb == SG_BLENDFACTOR_DST_COLOR);
    T(desc.colors[2].blend.dst_factor_rgb == SG_BLENDFACTOR_SRC_COLOR);
    T(desc.colors[2].blend.op_rgb == SG_BLENDOP_REVERSE_SUBTRACT);
    T(desc.colors[2].blend.src_factor_alpha == SG_BLENDFACTOR_DST_ALPHA);
    T(desc.colors[2].blend.dst_factor_alpha == SG_BLENDFACTOR_SRC_ALPHA);
    T(desc.colors[2].blend.op_alpha == SG_BLENDOP_SUBTRACT);

    sg_shutdown();
}

UTEST(sokol_gfx, query_pass_defaults) {
    sg_setup(&(sg_desc){0});
    /* sg_pass_desc doesn't actually have any meaningful default values */
    const sg_pass_desc desc = sg_query_pass_defaults(&(sg_pass_desc){0});
    T(desc.color_attachments[0].image.id == SG_INVALID_ID);
    T(desc.color_attachments[0].mip_level == 0);
    sg_shutdown();
}

UTEST(sokol_gfx, query_buffer_info) {
    sg_setup(&(sg_desc){0});
    sg_buffer buf = sg_make_buffer(&(sg_buffer_desc){
        .size = 256,
        .type = SG_BUFFERTYPE_VERTEXBUFFER,
        .usage = SG_USAGE_STREAM
    });
    T(buf.id != SG_INVALID_ID);
    const sg_buffer_info info = sg_query_buffer_info(buf);
    T(info.slot.state == SG_RESOURCESTATE_VALID);
    sg_shutdown();
}

UTEST(sokol_gfx, query_image_info) {
    sg_setup(&(sg_desc){0});
    sg_image img = sg_make_image(&(sg_image_desc){
        .render_target = true,
        .width = 256,
        .height = 128
    });
    T(img.id != SG_INVALID_ID);
    const sg_image_info info = sg_query_image_info(img);
    T(info.slot.state == SG_RESOURCESTATE_VALID);
    T(info.num_slots == 1);
    T(info.width == 256);
    T(info.height == 128);
    sg_shutdown();
}

UTEST(sokol_gfx, query_shader_info) {
    sg_setup(&(sg_desc){0});
    sg_shader shd = sg_make_shader(&(sg_shader_desc){
        .attrs = {
            [0] = { .name = "pos" }
        },
        .vs.source = "bla",
        .fs.source = "blub"
    });
    const sg_shader_info info = sg_query_shader_info(shd);
    T(info.slot.state == SG_RESOURCESTATE_VALID);
    sg_shutdown();
}

UTEST(sokol_gfx, query_pipeline_info) {
    sg_setup(&(sg_desc){0});
    sg_pipeline pip = sg_make_pipeline(&(sg_pipeline_desc){
        .layout = {
            .attrs[0].format = SG_VERTEXFORMAT_FLOAT3
        },
        .shader = sg_make_shader(&(sg_shader_desc){
            .attrs = {
                [0] = { .name = "pos" }
            },
            .vs.source = "bla",
            .fs.source = "blub"
        })
    });
    const sg_pipeline_info info = sg_query_pipeline_info(pip);
    T(info.slot.state == SG_RESOURCESTATE_VALID);
    sg_shutdown();
}

UTEST(sokol_gfx, query_pass_info) {
    sg_setup(&(sg_desc){0});
    sg_image_desc img_desc = {
        .render_target = true,
        .width = 128,
        .height = 128,
    };
    sg_pass pass = sg_make_pass(&(sg_pass_desc){
        .color_attachments = {
            [0].image = sg_make_image(&img_desc),
            [1].image = sg_make_image(&img_desc),
            [2].image = sg_make_image(&img_desc)
        },
    });
    const sg_pass_info info = sg_query_pass_info(pass);
    T(info.slot.state == SG_RESOURCESTATE_VALID);
    sg_shutdown();
}

UTEST(sokol_gfx, buffer_resource_states) {
    sg_setup(&(sg_desc){0});
    sg_buffer buf = sg_alloc_buffer();
    T(sg_query_buffer_state(buf) == SG_RESOURCESTATE_ALLOC);
    sg_init_buffer(buf, &(sg_buffer_desc){ .usage = SG_USAGE_STREAM, .size = 128 });
    T(sg_query_buffer_state(buf) == SG_RESOURCESTATE_VALID);
    sg_uninit_buffer(buf);
    T(sg_query_buffer_state(buf) == SG_RESOURCESTATE_ALLOC);
    sg_dealloc_buffer(buf);
    T(sg_query_buffer_state(buf) == SG_RESOURCESTATE_INVALID);
    sg_shutdown();
}

UTEST(sokol_gfx, image_resource_states) {
    sg_setup(&(sg_desc){0});
    sg_image img = sg_alloc_image();
    T(sg_query_image_state(img) == SG_RESOURCESTATE_ALLOC);
    sg_init_image(img, &(sg_image_desc){ .render_target = true, .width = 16, .height = 16 });
    T(sg_query_image_state(img) == SG_RESOURCESTATE_VALID);
    sg_uninit_image(img);
    T(sg_query_image_state(img) == SG_RESOURCESTATE_ALLOC);
    sg_dealloc_image(img);
    T(sg_query_image_state(img) == SG_RESOURCESTATE_INVALID);
    sg_shutdown();
}

UTEST(sokol_gfx, shader_resource_states) {
    sg_setup(&(sg_desc){0});
    sg_shader shd = sg_alloc_shader();
    T(sg_query_shader_state(shd) == SG_RESOURCESTATE_ALLOC);
    sg_init_shader(shd, &(sg_shader_desc){0});
    T(sg_query_shader_state(shd) == SG_RESOURCESTATE_VALID);
    sg_uninit_shader(shd);
    T(sg_query_shader_state(shd) == SG_RESOURCESTATE_ALLOC);
    sg_dealloc_shader(shd);
    T(sg_query_shader_state(shd) == SG_RESOURCESTATE_INVALID);
    sg_shutdown();
}

UTEST(sokol_gfx, pipeline_resource_states) {
    sg_setup(&(sg_desc){0});
    sg_pipeline pip = sg_alloc_pipeline();
    T(sg_query_pipeline_state(pip) == SG_RESOURCESTATE_ALLOC);
    sg_init_pipeline(pip, &(sg_pipeline_desc){
        .shader = sg_make_shader(&(sg_shader_desc){0}),
        .layout.attrs[0].format = SG_VERTEXFORMAT_FLOAT3
    });
    T(sg_query_pipeline_state(pip) == SG_RESOURCESTATE_VALID);
    sg_uninit_pipeline(pip);
    T(sg_query_pipeline_state(pip) == SG_RESOURCESTATE_ALLOC);
    sg_dealloc_pipeline(pip);
    T(sg_query_pipeline_state(pip) == SG_RESOURCESTATE_INVALID);
    sg_shutdown();
}

UTEST(sokol_gfx, pass_resource_states) {
    sg_setup(&(sg_desc){0});
    sg_pass pass = sg_alloc_pass();
    T(sg_query_pass_state(pass) == SG_RESOURCESTATE_ALLOC);
    sg_init_pass(pass, &(sg_pass_desc){
        .color_attachments[0].image = sg_make_image(&(sg_image_desc){ .render_target=true, .width=16, .height=16})
    });
    T(sg_query_pass_state(pass) == SG_RESOURCESTATE_VALID);
    sg_uninit_pass(pass);
    T(sg_query_pass_state(pass) == SG_RESOURCESTATE_ALLOC);
    sg_dealloc_pass(pass);
    T(sg_query_pass_state(pass) == SG_RESOURCESTATE_INVALID);
    sg_shutdown();
}
