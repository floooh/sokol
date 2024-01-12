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

#define MAX_LOGITEMS (32)
static int num_log_called = 0;
static sg_log_item log_items[MAX_LOGITEMS];

static void test_logger(const char* tag, uint32_t log_level, uint32_t log_item_id, const char* message_or_null, uint32_t line_nr, const char* filename_or_null, void* user_data) {
    (void)tag;
    (void)log_level;
    (void)message_or_null;
    (void)line_nr;
    (void)filename_or_null;
    (void)user_data;
    if (num_log_called < MAX_LOGITEMS) {
        log_items[num_log_called++] = log_item_id;
    }
    if (message_or_null) {
        printf("%s\n", message_or_null);
    }
}

static void reset_log_items(void) {
    num_log_called = 0;
    memset(log_items, 0, sizeof(log_items));
}

static void setup(const sg_desc* desc) {
    reset_log_items();
    sg_desc desc_with_logger = *desc;
    desc_with_logger.logger.func = test_logger;
    sg_setup(&desc_with_logger);
}

static sg_buffer create_buffer(void) {
    static const float data[] = { 1, 2, 3, 4 };
    return sg_make_buffer(&(sg_buffer_desc){ .data = SG_RANGE(data) });
}

static sg_image create_image(void) {
    return sg_make_image(&(sg_image_desc){
        .render_target = true,
        .width = 256,
        .height = 128
    });
}

static sg_shader create_shader(void) {
    return sg_make_shader(&(sg_shader_desc){0});
}

static sg_pipeline create_pipeline(void) {
    return sg_make_pipeline(&(sg_pipeline_desc){
        .layout = {
            .attrs[0].format = SG_VERTEXFORMAT_FLOAT3
        },
        .shader = sg_make_shader(&(sg_shader_desc){0})
    });
}

static sg_pass create_pass(void) {
    sg_image_desc img_desc = {
        .render_target = true,
        .width = 128,
        .height = 128,
    };
    return sg_make_pass(&(sg_pass_desc){
        .color_attachments = {
            [0].image = sg_make_image(&img_desc),
            [1].image = sg_make_image(&img_desc),
            [2].image = sg_make_image(&img_desc)
        },
    });
}

UTEST(sokol_gfx, init_shutdown) {
    setup(&(sg_desc){0});
    T(sg_isvalid());
    sg_shutdown();
    T(!sg_isvalid());
}

UTEST(sokol_gfx, query_desc) {
    setup(&(sg_desc){
        .buffer_pool_size = 1024,
        .sampler_pool_size = 8,
        .shader_pool_size = 128,
        .pass_pool_size = 64,
    });
    const sg_desc desc = sg_query_desc();
    T(desc.buffer_pool_size == 1024);
    T(desc.image_pool_size == _SG_DEFAULT_IMAGE_POOL_SIZE);
    T(desc.sampler_pool_size == 8);
    T(desc.shader_pool_size == 128);
    T(desc.pipeline_pool_size == _SG_DEFAULT_PIPELINE_POOL_SIZE);
    T(desc.pass_pool_size == 64);
    T(desc.context_pool_size == _SG_DEFAULT_CONTEXT_POOL_SIZE);
    T(desc.uniform_buffer_size == _SG_DEFAULT_UB_SIZE);
    sg_shutdown();
}

UTEST(sokol_gfx, query_backend) {
    setup(&(sg_desc){0});
    T(sg_query_backend() == SG_BACKEND_DUMMY);
    sg_shutdown();
}

UTEST(sokol_gfx, pool_size) {
    setup(&(sg_desc){
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
    setup(&(sg_desc){
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
    setup(&(sg_desc){
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

UTEST(sokol_gfx, alloc_fail_destroy_samplers) {
    setup(&(sg_desc){
        .sampler_pool_size = 3,
    });

    sg_sampler smp[3] = { {0} };
    for (int i = 0; i < 3; i++) {
        smp[i] = sg_alloc_sampler();
        T(smp[i].id != SG_INVALID_ID);
        T((2-i) == _sg.pools.sampler_pool.queue_top);
        T(sg_query_sampler_state(smp[i]) == SG_RESOURCESTATE_ALLOC);
    }
    // the next alloc will fail because the pool is exhausted
    sg_sampler s3 = sg_alloc_sampler();
    T(s3.id == SG_INVALID_ID);
    T(sg_query_sampler_state(s3) == SG_RESOURCESTATE_INVALID);

    // before destroying, the resources must be either in valid or failed state
    for (int i = 0; i < 3; i++) {
        sg_fail_sampler(smp[i]);
        T(sg_query_sampler_state(smp[i]) == SG_RESOURCESTATE_FAILED);
    }
    for (int i = 0; i < 3; i++) {
        sg_destroy_sampler(smp[i]);
        T(sg_query_sampler_state(smp[i]) == SG_RESOURCESTATE_INVALID);
        T((i+1) == _sg.pools.sampler_pool.queue_top);
    }
    sg_shutdown();
}

UTEST(sokol_gfx, alloc_fail_destroy_shaders) {
    setup(&(sg_desc){
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
    setup(&(sg_desc){
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
    setup(&(sg_desc){
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
    setup(&(sg_desc){
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
    /* trying to create another one fails because pool is exhausted */
    T(sg_make_buffer(&desc).id == SG_INVALID_ID);

    for (int i = 0; i < 3; i++) {
        sg_destroy_buffer(buf[i]);
        T(sg_query_buffer_state(buf[i]) == SG_RESOURCESTATE_INVALID);
        T((i+1) == _sg.pools.buffer_pool.queue_top);
    }
    sg_shutdown();
}

UTEST(sokol_gfx, make_destroy_images) {
    setup(&(sg_desc){
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
        T(imgptr->cmn.upd_frame_index == 0);
        T(imgptr->cmn.num_slots == 1);
        T(imgptr->cmn.active_slot == 0);
    }
    // trying to create another one fails because pool is exhausted
    T(sg_make_image(&desc).id == SG_INVALID_ID);

    for (int i = 0; i < 3; i++) {
        sg_destroy_image(img[i]);
        T(sg_query_image_state(img[i]) == SG_RESOURCESTATE_INVALID);
        T((i+1) == _sg.pools.image_pool.queue_top);
    }
    sg_shutdown();
}

UTEST(sokol_gfx, make_destroy_samplers) {
    setup(&(sg_desc){
        .sampler_pool_size = 3
    });
    T(sg_isvalid());

    sg_sampler smp[3] = { {0} };
    sg_sampler_desc desc = { 0 };
    for (int i = 0; i < 3; i++) {
        smp[i] = sg_make_sampler(&desc);
        T(smp[i].id != SG_INVALID_ID);
        T((2-i) == _sg.pools.sampler_pool.queue_top);
        T(sg_query_sampler_state(smp[i]) == SG_RESOURCESTATE_VALID);
        const _sg_sampler_t* smpptr = _sg_lookup_sampler(&_sg.pools, smp[i].id);
        T(smpptr);
        T(smpptr->slot.id == smp[i].id);
        T(smpptr->slot.ctx_id == _sg.active_context.id);
        T(smpptr->slot.state == SG_RESOURCESTATE_VALID);
        T(smpptr->cmn.min_filter == SG_FILTER_NEAREST);
        T(smpptr->cmn.mag_filter == SG_FILTER_NEAREST);
        T(smpptr->cmn.mipmap_filter == SG_FILTER_NONE);
        T(smpptr->cmn.wrap_u == SG_WRAP_REPEAT);
        T(smpptr->cmn.wrap_v == SG_WRAP_REPEAT);
        T(smpptr->cmn.wrap_w == SG_WRAP_REPEAT);
        T(smpptr->cmn.min_lod == 0.0f);
        T(smpptr->cmn.max_lod == FLT_MAX);
        T(smpptr->cmn.border_color == SG_BORDERCOLOR_OPAQUE_BLACK);
        T(smpptr->cmn.compare == SG_COMPAREFUNC_NEVER);
        T(smpptr->cmn.max_anisotropy == 1);
    }
    // trying to create another one fails because pool is exhausted
    T(sg_make_sampler(&desc).id == SG_INVALID_ID);

    for (int i = 0; i < 3; i++) {
        sg_destroy_sampler(smp[i]);
        T(sg_query_sampler_state(smp[i]) == SG_RESOURCESTATE_INVALID);
        T((i+1) == _sg.pools.sampler_pool.queue_top);
    }
    sg_shutdown();
}

UTEST(sokol_gfx, make_destroy_shaders) {
    setup(&(sg_desc){
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
    /* trying to create another one fails because pool is exhausted */
    T(sg_make_shader(&desc).id == SG_INVALID_ID);

    for (int i = 0; i < 3; i++) {
        sg_destroy_shader(shd[i]);
        T(sg_query_shader_state(shd[i]) == SG_RESOURCESTATE_INVALID);
        T((i+1) == _sg.pools.shader_pool.queue_top);
    }
    sg_shutdown();
}

UTEST(sokol_gfx, make_destroy_pipelines) {
    setup(&(sg_desc){
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
        T(pipptr->cmn.color_count == 1);
        T(pipptr->cmn.colors[0].pixel_format == SG_PIXELFORMAT_RGBA8);
        T(pipptr->cmn.depth.pixel_format == SG_PIXELFORMAT_DEPTH_STENCIL);
        T(pipptr->cmn.sample_count == 1);
        T(pipptr->cmn.index_type == SG_INDEXTYPE_NONE);
        T(pipptr->cmn.vertex_buffer_layout_active[0]);
        T(!pipptr->cmn.vertex_buffer_layout_active[1]);
    }
    /* trying to create another one fails because pool is exhausted */
    T(sg_make_pipeline(&desc).id == SG_INVALID_ID);

    for (int i = 0; i < 3; i++) {
        sg_destroy_pipeline(pip[i]);
        T(sg_query_pipeline_state(pip[i]) == SG_RESOURCESTATE_INVALID);
        T((i+1) == _sg.pools.pipeline_pool.queue_top);
    }
    sg_shutdown();
}

UTEST(sokol_gfx, make_destroy_passes) {
    setup(&(sg_desc){
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
    /* trying to create another one fails because pool is exhausted */
    T(sg_make_pass(&pass_desc).id == SG_INVALID_ID);

    for (int i = 0; i < 3; i++) {
        sg_destroy_pass(pass[i]);
        T(sg_query_pass_state(pass[i]) == SG_RESOURCESTATE_INVALID);
        T((i+1) == _sg.pools.pass_pool.queue_top);
    }
    sg_shutdown();
}

UTEST(sokol_gfx, generation_counter) {
    setup(&(sg_desc){
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
    setup(&(sg_desc){0});
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
    setup(&(sg_desc){0});
    const sg_image_desc desc = sg_query_image_defaults(&(sg_image_desc){0});
    T(desc.type == SG_IMAGETYPE_2D);
    T(!desc.render_target);
    T(desc.num_mipmaps == 1);
    T(desc.usage == SG_USAGE_IMMUTABLE);
    T(desc.pixel_format == SG_PIXELFORMAT_RGBA8);
    T(desc.sample_count == 1);
    sg_shutdown();
}

UTEST(sokol_gfx, query_sampler_defaults) {
    setup(&(sg_desc){0});
    const sg_sampler_desc desc = sg_query_sampler_defaults(&(sg_sampler_desc){0});
    T(desc.min_filter == SG_FILTER_NEAREST);
    T(desc.mag_filter == SG_FILTER_NEAREST);
    T(desc.mipmap_filter == SG_FILTER_NONE);
    T(desc.wrap_u == SG_WRAP_REPEAT);
    T(desc.wrap_v == SG_WRAP_REPEAT);
    T(desc.wrap_w == SG_WRAP_REPEAT);
    T(desc.min_lod == 0.0f);
    T(desc.max_lod == FLT_MAX);
    T(desc.border_color == SG_BORDERCOLOR_OPAQUE_BLACK);
    T(desc.compare == SG_COMPAREFUNC_NEVER);
    T(desc.max_anisotropy == 1);
    sg_shutdown();
}

UTEST(sokol_gfx, query_shader_defaults) {
    setup(&(sg_desc){0});
    const sg_shader_desc desc = sg_query_shader_defaults(&(sg_shader_desc){0});
    T(0 == strcmp(desc.vs.entry, "main"));
    T(0 == strcmp(desc.fs.entry, "main"));
    sg_shutdown();
}

UTEST(sokol_gfx, query_pipeline_defaults) {
    setup(&(sg_desc){0});
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
    setup(&(sg_desc){0});
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
    setup(&(sg_desc){0});
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
    setup(&(sg_desc){0});
    /* sg_pass_desc doesn't actually have any meaningful default values */
    const sg_pass_desc desc = sg_query_pass_defaults(&(sg_pass_desc){0});
    T(desc.color_attachments[0].image.id == SG_INVALID_ID);
    T(desc.color_attachments[0].mip_level == 0);
    sg_shutdown();
}

UTEST(sokol_gfx, query_buffer_info) {
    setup(&(sg_desc){0});
    sg_buffer buf = sg_make_buffer(&(sg_buffer_desc){
        .size = 256,
        .type = SG_BUFFERTYPE_VERTEXBUFFER,
        .usage = SG_USAGE_STREAM
    });
    T(buf.id != SG_INVALID_ID);
    const sg_buffer_info info = sg_query_buffer_info(buf);
    T(info.slot.state == SG_RESOURCESTATE_VALID);
    T(info.slot.res_id == buf.id);
    sg_shutdown();
}

UTEST(sokol_gfx, query_image_info) {
    setup(&(sg_desc){0});
    sg_image img = sg_make_image(&(sg_image_desc){
        .render_target = true,
        .width = 256,
        .height = 128
    });
    T(img.id != SG_INVALID_ID);
    const sg_image_info info = sg_query_image_info(img);
    T(info.slot.state == SG_RESOURCESTATE_VALID);
    T(info.slot.res_id == img.id);
    T(info.num_slots == 1);
    sg_shutdown();
}

UTEST(sokoL_gfx, query_sampler_info) {
    setup(&(sg_desc){0});
    sg_sampler smp = sg_make_sampler(&(sg_sampler_desc){ 0 });
    T(smp.id != SG_INVALID_ID);
    const sg_sampler_info info = sg_query_sampler_info(smp);
    T(info.slot.state == SG_RESOURCESTATE_VALID);
    T(info.slot.res_id == smp.id);
    sg_shutdown();
}

UTEST(sokol_gfx, query_shader_info) {
    setup(&(sg_desc){0});
    sg_shader shd = sg_make_shader(&(sg_shader_desc){
        .attrs = {
            [0] = { .name = "pos" }
        },
        .vs.source = "bla",
        .fs.source = "blub"
    });
    const sg_shader_info info = sg_query_shader_info(shd);
    T(info.slot.state == SG_RESOURCESTATE_VALID);
    T(info.slot.res_id == shd.id);
    sg_shutdown();
}

UTEST(sokol_gfx, query_pipeline_info) {
    setup(&(sg_desc){0});
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
    T(info.slot.res_id == pip.id);
    sg_shutdown();
}

UTEST(sokol_gfx, query_pass_info) {
    setup(&(sg_desc){0});
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
    T(info.slot.res_id == pass.id);
    sg_shutdown();
}

UTEST(sokol_gfx, query_buffer_desc) {
    setup(&(sg_desc){0});

    sg_buffer b0 = sg_make_buffer(&(sg_buffer_desc){
        .size = 32,
        .usage = SG_USAGE_STREAM,
        .label = "bla",
    });
    const sg_buffer_desc b0_desc = sg_query_buffer_desc(b0);
    T(b0_desc.size == 32);
    T(b0_desc.type == SG_BUFFERTYPE_VERTEXBUFFER);
    T(b0_desc.usage == SG_USAGE_STREAM);
    T(b0_desc.data.ptr == 0);
    T(b0_desc.data.size == 0);
    T(b0_desc.gl_buffers[0] == 0);
    T(b0_desc.mtl_buffers[0] == 0);
    T(b0_desc.d3d11_buffer == 0);
    T(b0_desc.wgpu_buffer == 0);

    float vtx_data[16];
    sg_buffer b1 = sg_make_buffer(&(sg_buffer_desc){
        .data = SG_RANGE(vtx_data)
    });
    const sg_buffer_desc b1_desc = sg_query_buffer_desc(b1);
    T(b1_desc.size == sizeof(vtx_data));
    T(b1_desc.type == SG_BUFFERTYPE_VERTEXBUFFER);
    T(b1_desc.usage == SG_USAGE_IMMUTABLE);
    T(b1_desc.data.ptr == 0);
    T(b1_desc.data.size == 0);

    uint16_t idx_data[8];
    sg_buffer b2 = sg_make_buffer(&(sg_buffer_desc){
        .type = SG_BUFFERTYPE_INDEXBUFFER,
        .data = SG_RANGE(idx_data),
    });
    const sg_buffer_desc b2_desc = sg_query_buffer_desc(b2);
    T(b2_desc.size == sizeof(idx_data));
    T(b2_desc.type == SG_BUFFERTYPE_INDEXBUFFER);
    T(b2_desc.usage == SG_USAGE_IMMUTABLE);
    T(b2_desc.data.ptr == 0);
    T(b2_desc.data.size == 0);

    // invalid buffer (returns zeroed desc)
    sg_buffer b3 = sg_make_buffer(&(sg_buffer_desc){
        .size = 32,
        .usage = SG_USAGE_STREAM,
        .label = "bla",
    });
    sg_destroy_buffer(b3);
    const sg_buffer_desc b3_desc = sg_query_buffer_desc(b3);
    T(b3_desc.size == 0);
    T(b3_desc.type == 0);
    T(b3_desc.usage == 0);

    sg_shutdown();
}

UTEST(sokol_gfx, query_image_desc) {
    setup(&(sg_desc){0});

    sg_image i0 = sg_make_image(&(sg_image_desc){
        .width = 256,
        .height = 512,
        .pixel_format = SG_PIXELFORMAT_R8,
        .usage = SG_USAGE_DYNAMIC,
    });
    const sg_image_desc i0_desc = sg_query_image_desc(i0);
    T(i0_desc.type == SG_IMAGETYPE_2D);
    T(i0_desc.render_target == false);
    T(i0_desc.width == 256);
    T(i0_desc.height == 512);
    T(i0_desc.num_slices == 1);
    T(i0_desc.num_mipmaps == 1);
    T(i0_desc.usage == SG_USAGE_DYNAMIC);
    T(i0_desc.pixel_format == SG_PIXELFORMAT_R8);
    T(i0_desc.sample_count == 1);
    T(i0_desc.data.subimage[0][0].ptr == 0);
    T(i0_desc.data.subimage[0][0].size == 0);
    T(i0_desc.gl_textures[0] == 0);
    T(i0_desc.gl_texture_target == 0);
    T(i0_desc.mtl_textures[0] == 0);
    T(i0_desc.d3d11_texture == 0);
    T(i0_desc.d3d11_shader_resource_view == 0);
    T(i0_desc.wgpu_texture == 0);

    sg_destroy_image(i0);
    const sg_image_desc i0_desc_x = sg_query_image_desc(i0);
    T(i0_desc_x.type == 0);
    T(i0_desc_x.render_target == false);
    T(i0_desc_x.width == 0);
    T(i0_desc_x.height == 0);
    T(i0_desc_x.num_slices == 0);
    T(i0_desc_x.num_mipmaps == 0);
    T(i0_desc_x.usage == 0);
    T(i0_desc_x.pixel_format == 0);
    T(i0_desc_x.sample_count == 0);

    sg_shutdown();
}

UTEST(sokol_gfx, query_sampler_desc) {
    setup(&(sg_desc){0});
    sg_sampler s0 = sg_make_sampler(&(sg_sampler_desc){
        .min_filter = SG_FILTER_LINEAR,
        .mag_filter = SG_FILTER_LINEAR,
        .mipmap_filter = SG_FILTER_LINEAR,
        .wrap_v = SG_WRAP_MIRRORED_REPEAT,
        .max_anisotropy = 8,
        .border_color = SG_BORDERCOLOR_TRANSPARENT_BLACK,
        .compare = SG_COMPAREFUNC_GREATER,
    });
    const sg_sampler_desc s0_desc = sg_query_sampler_desc(s0);
    T(s0_desc.min_filter == SG_FILTER_LINEAR);
    T(s0_desc.mag_filter == SG_FILTER_LINEAR);
    T(s0_desc.mipmap_filter == SG_FILTER_LINEAR);
    T(s0_desc.wrap_u == SG_WRAP_REPEAT);
    T(s0_desc.wrap_v == SG_WRAP_MIRRORED_REPEAT);
    T(s0_desc.wrap_w == SG_WRAP_REPEAT);
    T(s0_desc.min_lod == 0.0f);
    T(s0_desc.max_lod == FLT_MAX);
    T(s0_desc.border_color == SG_BORDERCOLOR_TRANSPARENT_BLACK);
    T(s0_desc.compare == SG_COMPAREFUNC_GREATER);
    T(s0_desc.max_anisotropy == 8);

    sg_destroy_sampler(s0);
    const sg_sampler_desc s0_desc_x = sg_query_sampler_desc(s0);
    T(s0_desc_x.min_filter == 0);
    T(s0_desc_x.compare == 0);

    sg_shutdown();
}

UTEST(sokol_gfx, query_shader_desc) {
    setup(&(sg_desc){0});

    sg_shader s0 = sg_make_shader(&(sg_shader_desc){
        .attrs = {
            [0] = { .name = "pos", .sem_name = "POS", .sem_index = 1 },
        },
        .vs = {
            .source = "vs_source",
            .uniform_blocks = {
                [0] = {
                    .size = 128,
                    .layout = SG_UNIFORMLAYOUT_STD140,
                    .uniforms = {
                        [0] = { .name = "blub", .type = SG_UNIFORMTYPE_FLOAT4, .array_count = 1 },
                        [1] = { .name = "blob", .type = SG_UNIFORMTYPE_FLOAT2, .array_count = 1 },
                    }
                }
            },
            .images[0] = { .used = true, .image_type = SG_IMAGETYPE_2D, .sample_type = SG_IMAGESAMPLETYPE_FLOAT, .multisampled = true },
            .images[1] = { .used = true, .image_type = SG_IMAGETYPE_3D, .sample_type = SG_IMAGESAMPLETYPE_DEPTH },
            .samplers[0] = { .used = true, .sampler_type = SG_SAMPLERTYPE_FILTERING },
            .samplers[1] = { .used = true, .sampler_type = SG_SAMPLERTYPE_COMPARISON },
            .image_sampler_pairs[0] = { .used = true, .image_slot = 0, .sampler_slot = 0, .glsl_name = "img0" },
            .image_sampler_pairs[1] = { .used = true, .image_slot = 1, .sampler_slot = 1, .glsl_name = "img1" },
        },
        .fs = {
            .source = "fs_source",
            .images[0] = { .used = true, .image_type = SG_IMAGETYPE_ARRAY, .sample_type = SG_IMAGESAMPLETYPE_DEPTH },
            .images[1] = { .used = true, .image_type = SG_IMAGETYPE_CUBE, .sample_type = SG_IMAGESAMPLETYPE_UNFILTERABLE_FLOAT },
            .samplers[0] = { .used = true, .sampler_type = SG_SAMPLERTYPE_COMPARISON },
            .samplers[1] = { .used = true, .sampler_type = SG_SAMPLERTYPE_NONFILTERING },
            .image_sampler_pairs[0] = { .used = true, .image_slot = 0, .sampler_slot = 0, .glsl_name = "img3" },
            .image_sampler_pairs[1] = { .used = true, .image_slot = 1, .sampler_slot = 1, .glsl_name = "img4" },
        },
        .label = "label",
    });
    const sg_shader_desc s0_desc = sg_query_shader_desc(s0);
    T(s0_desc.attrs[0].name == 0);
    T(s0_desc.attrs[0].sem_name == 0);
    T(s0_desc.attrs[0].sem_index == 0);
    T(s0_desc.vs.source == 0);
    T(s0_desc.vs.uniform_blocks[0].size == 128);
    T(s0_desc.vs.uniform_blocks[0].layout == 0);
    T(s0_desc.vs.uniform_blocks[0].uniforms[0].name == 0);
    T(s0_desc.vs.uniform_blocks[0].uniforms[0].type == 0);
    T(s0_desc.vs.uniform_blocks[0].uniforms[0].array_count == 0);
    T(s0_desc.vs.images[0].used);
    T(s0_desc.vs.images[0].image_type == SG_IMAGETYPE_2D);
    T(s0_desc.vs.images[0].sample_type == SG_IMAGESAMPLETYPE_FLOAT);
    T(s0_desc.vs.images[0].multisampled);
    T(s0_desc.vs.images[1].used);
    T(s0_desc.vs.images[1].image_type == SG_IMAGETYPE_3D);
    T(s0_desc.vs.images[1].sample_type == SG_IMAGESAMPLETYPE_DEPTH);
    T(s0_desc.vs.images[1].multisampled == false);
    T(s0_desc.vs.samplers[0].used);
    T(s0_desc.vs.samplers[0].sampler_type == SG_SAMPLERTYPE_FILTERING);
    T(s0_desc.vs.samplers[1].used);
    T(s0_desc.vs.samplers[1].sampler_type == SG_SAMPLERTYPE_COMPARISON);
    T(s0_desc.vs.image_sampler_pairs[0].used);
    T(s0_desc.vs.image_sampler_pairs[0].image_slot == 0);
    T(s0_desc.vs.image_sampler_pairs[0].sampler_slot == 0);
    T(s0_desc.vs.image_sampler_pairs[0].glsl_name == 0);
    T(s0_desc.vs.image_sampler_pairs[1].used);
    T(s0_desc.vs.image_sampler_pairs[1].image_slot == 1);
    T(s0_desc.vs.image_sampler_pairs[1].sampler_slot == 1);
    T(s0_desc.vs.image_sampler_pairs[1].glsl_name == 0);
    T(s0_desc.fs.source == 0);
    T(s0_desc.fs.uniform_blocks[0].size == 0);
    T(s0_desc.fs.uniform_blocks[0].layout == 0);
    T(s0_desc.fs.uniform_blocks[0].uniforms[0].name == 0);
    T(s0_desc.fs.uniform_blocks[0].uniforms[0].type == 0);
    T(s0_desc.fs.uniform_blocks[0].uniforms[0].array_count == 0);
    T(s0_desc.fs.images[0].used);
    T(s0_desc.fs.images[0].image_type == SG_IMAGETYPE_ARRAY);
    T(s0_desc.fs.images[0].sample_type == SG_IMAGESAMPLETYPE_DEPTH);
    T(s0_desc.fs.images[0].multisampled == false);
    T(s0_desc.fs.images[1].used);
    T(s0_desc.fs.images[1].image_type == SG_IMAGETYPE_CUBE);
    T(s0_desc.fs.images[1].sample_type == SG_IMAGESAMPLETYPE_UNFILTERABLE_FLOAT);
    T(s0_desc.fs.images[1].multisampled == false);
    T(s0_desc.fs.samplers[0].used);
    T(s0_desc.fs.samplers[0].sampler_type == SG_SAMPLERTYPE_COMPARISON);
    T(s0_desc.fs.samplers[1].used);
    T(s0_desc.fs.samplers[1].sampler_type == SG_SAMPLERTYPE_NONFILTERING);
    T(s0_desc.fs.image_sampler_pairs[0].used);
    T(s0_desc.fs.image_sampler_pairs[0].image_slot == 0);
    T(s0_desc.fs.image_sampler_pairs[0].sampler_slot == 0);
    T(s0_desc.fs.image_sampler_pairs[0].glsl_name == 0);
    T(s0_desc.fs.image_sampler_pairs[1].used);
    T(s0_desc.fs.image_sampler_pairs[1].image_slot == 1);
    T(s0_desc.fs.image_sampler_pairs[1].sampler_slot == 1);
    T(s0_desc.fs.image_sampler_pairs[1].glsl_name == 0);

    sg_shutdown();
}

UTEST(sokol_gfx, query_pipeline_desc) {
    setup(&(sg_desc){0});

    sg_shader shd = sg_make_shader(&(sg_shader_desc){0});
    sg_pipeline p0 = sg_make_pipeline(&(sg_pipeline_desc){
        .shader = shd,
        .layout = {
            .attrs = {
                [0] = { .format = SG_VERTEXFORMAT_FLOAT4 },
                [1] = { .format = SG_VERTEXFORMAT_FLOAT2 },
            }
        },
        .label = "p0",
    });

    const sg_pipeline_desc p0_desc = sg_query_pipeline_desc(p0);
    T(p0_desc.shader.id == shd.id);
    T(p0_desc.layout.buffers[0].stride == 24);
    T(p0_desc.layout.buffers[0].step_func == SG_VERTEXSTEP_PER_VERTEX);
    T(p0_desc.layout.buffers[0].step_rate == 1);
    T(p0_desc.layout.buffers[1].stride == 0);
    T(p0_desc.layout.buffers[1].step_func == 0);
    T(p0_desc.layout.buffers[1].step_rate == 0);
    T(p0_desc.layout.attrs[0].format == SG_VERTEXFORMAT_FLOAT4);
    T(p0_desc.layout.attrs[0].offset == 0);
    T(p0_desc.layout.attrs[0].buffer_index == 0);
    T(p0_desc.layout.attrs[1].format == SG_VERTEXFORMAT_FLOAT2);
    T(p0_desc.layout.attrs[1].offset == 16);
    T(p0_desc.layout.attrs[1].buffer_index == 0);
    T(p0_desc.layout.attrs[2].format == 0);
    T(p0_desc.layout.attrs[2].offset == 0);
    T(p0_desc.layout.attrs[2].buffer_index == 0);
    T(p0_desc.depth.pixel_format == SG_PIXELFORMAT_DEPTH_STENCIL);
    T(p0_desc.depth.compare == SG_COMPAREFUNC_ALWAYS);
    T(p0_desc.depth.write_enabled == false);
    T(p0_desc.depth.bias == 0.0f);
    T(p0_desc.depth.bias_slope_scale == 0.0f);
    T(p0_desc.depth.bias_clamp == 0.0f);
    T(p0_desc.stencil.enabled == false);
    T(p0_desc.stencil.front.compare == SG_COMPAREFUNC_ALWAYS);
    T(p0_desc.stencil.front.fail_op == SG_STENCILOP_KEEP);
    T(p0_desc.stencil.front.depth_fail_op == SG_STENCILOP_KEEP);
    T(p0_desc.stencil.front.pass_op == SG_STENCILOP_KEEP);
    T(p0_desc.stencil.back.compare == SG_COMPAREFUNC_ALWAYS);
    T(p0_desc.stencil.back.fail_op == SG_STENCILOP_KEEP);
    T(p0_desc.stencil.back.depth_fail_op == SG_STENCILOP_KEEP);
    T(p0_desc.stencil.back.pass_op == SG_STENCILOP_KEEP);
    T(p0_desc.stencil.read_mask == 0);
    T(p0_desc.stencil.write_mask == 0);
    T(p0_desc.stencil.ref == 0);
    T(p0_desc.color_count == 1);
    T(p0_desc.colors[0].pixel_format == SG_PIXELFORMAT_RGBA8);
    T(p0_desc.colors[0].write_mask == SG_COLORMASK_RGBA);
    T(p0_desc.colors[0].blend.enabled == false);
    T(p0_desc.colors[0].blend.src_factor_rgb == SG_BLENDFACTOR_ONE);
    T(p0_desc.colors[0].blend.dst_factor_rgb == SG_BLENDFACTOR_ZERO);
    T(p0_desc.colors[0].blend.op_rgb == SG_BLENDOP_ADD);
    T(p0_desc.colors[0].blend.src_factor_alpha == SG_BLENDFACTOR_ONE);
    T(p0_desc.colors[0].blend.dst_factor_alpha == SG_BLENDFACTOR_ZERO);
    T(p0_desc.colors[0].blend.op_alpha == SG_BLENDOP_ADD);
    T(p0_desc.primitive_type == SG_PRIMITIVETYPE_TRIANGLES);
    T(p0_desc.index_type == SG_INDEXTYPE_NONE);
    T(p0_desc.cull_mode == SG_CULLMODE_NONE);
    T(p0_desc.face_winding == SG_FACEWINDING_CW);
    T(p0_desc.sample_count == 1);
    T(p0_desc.blend_color.r == 0.0f);
    T(p0_desc.blend_color.g == 0.0f);
    T(p0_desc.blend_color.b == 0.0f);
    T(p0_desc.blend_color.a == 0.0f);
    T(p0_desc.alpha_to_coverage_enabled == false);
    T(p0_desc.label == 0);
    sg_shutdown();
}

UTEST(sokol_gfx, query_pass_desc) {
    setup(&(sg_desc){0});

    const sg_image_desc color_img_desc = {
        .render_target = true,
        .width = 128,
        .height = 128,
    };
    const sg_image_desc depth_img_desc = {
        .render_target = true,
        .width = 128,
        .height = 128,
        .pixel_format = SG_PIXELFORMAT_DEPTH,
    };
    sg_image color_img_0 = sg_make_image(&color_img_desc);
    sg_image color_img_1 = sg_make_image(&color_img_desc);
    sg_image color_img_2 = sg_make_image(&color_img_desc);
    sg_image depth_img = sg_make_image(&depth_img_desc);

    sg_pass p0 = sg_make_pass(&(sg_pass_desc){
        .color_attachments = {
            [0].image = color_img_0,
            [1].image = color_img_1,
            [2].image = color_img_2,
        },
        .depth_stencil_attachment.image = depth_img,
    });
    const sg_pass_desc p0_desc = sg_query_pass_desc(p0);
    T(p0_desc.color_attachments[0].image.id == color_img_0.id);
    T(p0_desc.color_attachments[0].mip_level == 0);
    T(p0_desc.color_attachments[0].slice == 0);
    T(p0_desc.color_attachments[1].image.id == color_img_1.id);
    T(p0_desc.color_attachments[1].mip_level == 0);
    T(p0_desc.color_attachments[1].slice == 0);
    T(p0_desc.color_attachments[2].image.id == color_img_2.id);
    T(p0_desc.color_attachments[2].mip_level == 0);
    T(p0_desc.color_attachments[2].slice == 0);
    T(p0_desc.depth_stencil_attachment.image.id == depth_img.id);
    T(p0_desc.depth_stencil_attachment.mip_level == 0);
    T(p0_desc.depth_stencil_attachment.slice == 0);

    sg_shutdown();
}

UTEST(sokol_gfx, buffer_resource_states) {
    setup(&(sg_desc){0});
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
    setup(&(sg_desc){0});
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

UTEST(sokol_gfx, sampler_resource_states) {
    setup(&(sg_desc){0});
    sg_sampler smp = sg_alloc_sampler();
    T(sg_query_sampler_state(smp) == SG_RESOURCESTATE_ALLOC);
    sg_init_sampler(smp, &(sg_sampler_desc){ .min_filter = SG_FILTER_LINEAR, .mag_filter = SG_FILTER_LINEAR });
    T(sg_query_sampler_state(smp) == SG_RESOURCESTATE_VALID);
    sg_uninit_sampler(smp);
    T(sg_query_sampler_state(smp) == SG_RESOURCESTATE_ALLOC);
    sg_dealloc_sampler(smp);
    T(sg_query_sampler_state(smp) == SG_RESOURCESTATE_INVALID);
    sg_shutdown();
}

UTEST(sokol_gfx, shader_resource_states) {
    setup(&(sg_desc){0});
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
    setup(&(sg_desc){0});
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
    setup(&(sg_desc){0});
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

UTEST(sokol_gfx, query_buffer_will_overflow) {
    setup(&(sg_desc){0});
    sg_buffer buf = sg_make_buffer(&(sg_buffer_desc){
        .size = 64,
        .usage = SG_USAGE_STREAM
    });
    T(!sg_query_buffer_will_overflow(buf, 32));
    T(!sg_query_buffer_will_overflow(buf, 64));
    T(sg_query_buffer_will_overflow(buf, 65));
    static const uint8_t data[32] = {0};
    sg_append_buffer(buf, &SG_RANGE(data));
    T(!sg_query_buffer_will_overflow(buf, 32));
    T(sg_query_buffer_will_overflow(buf, 33));
    sg_shutdown();
}

static struct {
    uintptr_t userdata;
    int num_called;
} commit_listener;
static void reset_commit_listener(void) {
    commit_listener.userdata = 0;
    commit_listener.num_called = 0;
}
static void commit_listener_func(void* ud) {
    commit_listener.userdata = (uintptr_t)ud;
    commit_listener.num_called++;
}

UTEST(sokol_gfx, commit_listener_called) {
    reset_commit_listener();
    setup(&(sg_desc){0});
    const bool added = sg_add_commit_listener((sg_commit_listener){
        .func = commit_listener_func,
        .user_data = (void*)23,
    });
    T(added);
    T(_sg.commit_listeners.upper == 1);
    sg_commit();
    T(23 == commit_listener.userdata);
    T(1 == commit_listener.num_called);
    sg_shutdown();
}

UTEST(sokol_gfx, commit_listener_add_twice) {
    reset_commit_listener();
    setup(&(sg_desc){0});
    const sg_commit_listener listener = {
        .func = commit_listener_func,
        .user_data = (void*)23,
    };
    T(sg_add_commit_listener(listener));
    T(_sg.commit_listeners.upper == 1);
    T(!sg_add_commit_listener(listener));
    T(_sg.commit_listeners.upper == 1);
    sg_commit();
    T(23 == commit_listener.userdata);
    T(1 == commit_listener.num_called);
    sg_shutdown();
}

UTEST(sokol_gfx, commit_listener_same_func_diff_ud) {
    reset_commit_listener();
    setup(&(sg_desc){0});
    T(sg_add_commit_listener((sg_commit_listener){
        .func = commit_listener_func,
        .user_data = (void*)23,
    }));
    T(_sg.commit_listeners.upper == 1);
    T(sg_add_commit_listener((sg_commit_listener){
        .func = commit_listener_func,
        .user_data = (void*)25,
    }));
    T(_sg.commit_listeners.upper == 2);
    sg_commit();
    T(2 == commit_listener.num_called);
    sg_shutdown();
}

UTEST(sokol_gfx, commit_listener_add_remove_add) {
    reset_commit_listener();
    setup(&(sg_desc){0});
    const sg_commit_listener listener = {
        .func = commit_listener_func,
        .user_data = (void*)23,
    };
    T(sg_add_commit_listener(listener));
    T(_sg.commit_listeners.upper == 1);
    T(sg_remove_commit_listener(listener));
    T(_sg.commit_listeners.upper == 1);
    sg_commit();
    T(0 == commit_listener.num_called);
    T(sg_add_commit_listener(listener));
    T(_sg.commit_listeners.upper == 1);
    sg_commit();
    T(1 == commit_listener.num_called);
    T(23 == commit_listener.userdata);
    sg_shutdown();
}

UTEST(sokol_gfx, commit_listener_remove_non_existant) {
    reset_commit_listener();
    setup(&(sg_desc){0});
    const sg_commit_listener l0 = {
        .func = commit_listener_func,
        .user_data = (void*)23,
    };
    const sg_commit_listener l1 = {
        .func = commit_listener_func,
        .user_data = (void*)46,
    };
    const sg_commit_listener l2 = {
        .func = commit_listener_func,
        .user_data = (void*)256,
    };
    T(sg_add_commit_listener(l0));
    T(sg_add_commit_listener(l1));
    T(_sg.commit_listeners.upper == 2);
    T(!sg_remove_commit_listener(l2));
    T(_sg.commit_listeners.upper == 2);
    sg_shutdown();
}

UTEST(sokol_gfx, commit_listener_multi_add_remove) {
    reset_commit_listener();
    setup(&(sg_desc){0});
    const sg_commit_listener l0 = {
        .func = commit_listener_func,
        .user_data = (void*)23,
    };
    const sg_commit_listener l1 = {
        .func = commit_listener_func,
        .user_data = (void*)46,
    };
    T(sg_add_commit_listener(l0));
    T(sg_add_commit_listener(l1));
    T(_sg.commit_listeners.upper == 2);
    // removing the first listener will just clear its slot
    T(sg_remove_commit_listener(l0));
    T(_sg.commit_listeners.upper == 2);
    sg_commit();
    T(commit_listener.num_called == 1);
    T(commit_listener.userdata == 46);
    commit_listener.num_called = 0;
    // adding the first listener back will fill that same slot again
    T(sg_add_commit_listener(l0));
    T(_sg.commit_listeners.upper == 2);
    sg_commit();
    T(commit_listener.num_called == 2);
    T(commit_listener.userdata == 46);
    commit_listener.num_called = 0;
    // removing the second listener will decrement the upper bound
    T(sg_remove_commit_listener(l1));
    T(_sg.commit_listeners.upper == 2);
    sg_commit();
    T(commit_listener.num_called == 1);
    T(commit_listener.userdata == 23);
    commit_listener.num_called = 0;
    // and finally remove the first listener too
    T(sg_remove_commit_listener(l0));
    T(_sg.commit_listeners.upper == 2);
    sg_commit();
    T(commit_listener.num_called == 0);
    // removing the same listener twice just returns false
    T(!sg_remove_commit_listener(l0));
    T(!sg_remove_commit_listener(l1));
    sg_shutdown();
}

UTEST(sokol_gfx, commit_listener_array_full) {
    reset_commit_listener();
    setup(&(sg_desc){
        .max_commit_listeners = 3,
    });
    const sg_commit_listener l0 = {
        .func = commit_listener_func,
        .user_data = (void*)23,
    };
    const sg_commit_listener l1 = {
        .func = commit_listener_func,
        .user_data = (void*)46,
    };
    const sg_commit_listener l2 = {
        .func = commit_listener_func,
        .user_data = (void*)128,
    };
    const sg_commit_listener l3 = {
        .func = commit_listener_func,
        .user_data = (void*)256,
    };
    T(sg_add_commit_listener(l0));
    T(sg_add_commit_listener(l1));
    T(sg_add_commit_listener(l2));
    T(_sg.commit_listeners.upper == 3);
    // overflow!
    T(!sg_add_commit_listener(l3));
    T(_sg.commit_listeners.upper == 3);
    sg_commit();
    T(commit_listener.num_called == 3);
    T(commit_listener.userdata == 128);
    sg_shutdown();
}

UTEST(sokol_gfx, buffer_double_destroy_is_ok) {
    setup(&(sg_desc){0});
    sg_buffer buf = create_buffer();
    T(sg_query_buffer_state(buf) == SG_RESOURCESTATE_VALID);
    sg_destroy_buffer(buf);
    T(sg_query_buffer_state(buf) == SG_RESOURCESTATE_INVALID);
    sg_destroy_buffer(buf);
    T(sg_query_buffer_state(buf) == SG_RESOURCESTATE_INVALID);
    sg_shutdown();
}

UTEST(sokol_gfx, image_double_destroy_is_ok) {
    setup(&(sg_desc){0});
    sg_image img = create_image();
    T(sg_query_image_state(img) == SG_RESOURCESTATE_VALID);
    sg_destroy_image(img);
    T(sg_query_image_state(img) == SG_RESOURCESTATE_INVALID);
    sg_destroy_image(img);
    T(sg_query_image_state(img) == SG_RESOURCESTATE_INVALID);
    sg_shutdown();
}

UTEST(sokol_gfx, sampler_double_destroy_is_ok) {
    setup(&(sg_desc){0});
    sg_sampler smp = sg_make_sampler(&(sg_sampler_desc){0});
    T(sg_query_sampler_state(smp) == SG_RESOURCESTATE_VALID);
    sg_destroy_sampler(smp);
    T(sg_query_sampler_state(smp) == SG_RESOURCESTATE_INVALID);
    sg_destroy_sampler(smp);
    T(sg_query_sampler_state(smp) == SG_RESOURCESTATE_INVALID);
    sg_shutdown();
}

UTEST(sokol_gfx, shader_double_destroy_is_ok) {
    setup(&(sg_desc){0});
    sg_shader shd = create_shader();
    T(sg_query_shader_state(shd) == SG_RESOURCESTATE_VALID);
    sg_destroy_shader(shd);
    T(sg_query_shader_state(shd) == SG_RESOURCESTATE_INVALID);
    sg_destroy_shader(shd);
    T(sg_query_shader_state(shd) == SG_RESOURCESTATE_INVALID);
    sg_shutdown();
}

UTEST(sokol_gfx, pipeline_double_destroy_is_ok) {
    setup(&(sg_desc){0});
    sg_pipeline pip = create_pipeline();
    T(sg_query_pipeline_state(pip) == SG_RESOURCESTATE_VALID);
    sg_destroy_pipeline(pip);
    T(sg_query_pipeline_state(pip) == SG_RESOURCESTATE_INVALID);
    sg_destroy_pipeline(pip);
    T(sg_query_pipeline_state(pip) == SG_RESOURCESTATE_INVALID);
    sg_shutdown();
}

UTEST(sokoL_gfx, pass_double_destroy_is_ok) {
    setup(&(sg_desc){0});
    sg_pass pass = create_pass();
    T(sg_query_pass_state(pass) == SG_RESOURCESTATE_VALID);
    sg_destroy_pass(pass);
    T(sg_query_pass_state(pass) == SG_RESOURCESTATE_INVALID);
    sg_destroy_pass(pass);
    T(sg_query_pass_state(pass) == SG_RESOURCESTATE_INVALID);
    sg_shutdown();
}

UTEST(sokol_gfx, make_dealloc_buffer_warns) {
    setup(&(sg_desc){0});
    sg_buffer buf = create_buffer();
    T(sg_query_buffer_state(buf) == SG_RESOURCESTATE_VALID);
    sg_dealloc_buffer(buf);
    T(log_items[0] == SG_LOGITEM_DEALLOC_BUFFER_INVALID_STATE);
    T(num_log_called == 1);
    T(sg_query_buffer_state(buf) == SG_RESOURCESTATE_VALID);
    sg_destroy_buffer(buf);
    T(sg_query_buffer_state(buf) == SG_RESOURCESTATE_INVALID);
    sg_shutdown();
}

UTEST(sokol_gfx, make_dealloc_image_warns) {
    setup(&(sg_desc){0});
    sg_image img = create_image();
    T(sg_query_image_state(img) == SG_RESOURCESTATE_VALID);
    sg_dealloc_image(img);
    T(log_items[0] == SG_LOGITEM_DEALLOC_IMAGE_INVALID_STATE);
    T(num_log_called == 1);
    T(sg_query_image_state(img) == SG_RESOURCESTATE_VALID);
    sg_destroy_image(img);
    T(sg_query_image_state(img) == SG_RESOURCESTATE_INVALID);
    sg_shutdown();
}

UTEST(sokol_gfx, make_dealloc_sampler_warns) {
    setup(&(sg_desc){0});
    sg_sampler smp = sg_make_sampler(&(sg_sampler_desc){0});
    T(sg_query_sampler_state(smp) == SG_RESOURCESTATE_VALID);
    sg_dealloc_sampler(smp);
    T(log_items[0] == SG_LOGITEM_DEALLOC_SAMPLER_INVALID_STATE);
    T(num_log_called == 1);
    T(sg_query_sampler_state(smp) == SG_RESOURCESTATE_VALID);
    sg_destroy_sampler(smp);
    T(sg_query_sampler_state(smp) == SG_RESOURCESTATE_INVALID);
    sg_shutdown();
}

UTEST(sokol_gfx, make_dealloc_shader_warns) {
    setup(&(sg_desc){0});
    sg_shader shd = create_shader();
    T(sg_query_shader_state(shd) == SG_RESOURCESTATE_VALID);
    sg_dealloc_shader(shd);
    T(log_items[0] == SG_LOGITEM_DEALLOC_SHADER_INVALID_STATE);
    T(num_log_called == 1);
    T(sg_query_shader_state(shd) == SG_RESOURCESTATE_VALID);
    sg_destroy_shader(shd);
    T(sg_query_shader_state(shd) == SG_RESOURCESTATE_INVALID);
    sg_shutdown();
}

UTEST(sokol_gfx, make_dealloc_pipeline_warns) {
    setup(&(sg_desc){0});
    sg_pipeline pip = create_pipeline();
    T(sg_query_pipeline_state(pip) == SG_RESOURCESTATE_VALID);
    sg_dealloc_pipeline(pip);
    T(log_items[0] == SG_LOGITEM_DEALLOC_PIPELINE_INVALID_STATE);
    T(num_log_called == 1);
    T(sg_query_pipeline_state(pip) == SG_RESOURCESTATE_VALID);
    sg_destroy_pipeline(pip);
    T(sg_query_pipeline_state(pip) == SG_RESOURCESTATE_INVALID);
    sg_shutdown();
}

UTEST(sokol_gfx, make_dealloc_pass_warns) {
    setup(&(sg_desc){0});
    sg_pass pass = create_pass();
    T(sg_query_pass_state(pass) == SG_RESOURCESTATE_VALID);
    sg_dealloc_pass(pass);
    T(log_items[0] == SG_LOGITEM_DEALLOC_PASS_INVALID_STATE);
    T(num_log_called == 1);
    T(sg_query_pass_state(pass) == SG_RESOURCESTATE_VALID);
    sg_destroy_pass(pass);
    T(sg_query_pass_state(pass) == SG_RESOURCESTATE_INVALID);
    sg_shutdown();
}

UTEST(sokol_gfx, alloc_uninit_buffer_warns) {
    setup(&(sg_desc){0});
    sg_buffer buf = sg_alloc_buffer();
    T(sg_query_buffer_state(buf) == SG_RESOURCESTATE_ALLOC);
    sg_uninit_buffer(buf);
    T(log_items[0] == SG_LOGITEM_UNINIT_BUFFER_INVALID_STATE);
    T(num_log_called == 1);
    T(sg_query_buffer_state(buf) == SG_RESOURCESTATE_ALLOC);
    sg_shutdown();
}

UTEST(sokol_gfx, alloc_uninit_image_warns) {
    setup(&(sg_desc){0});
    sg_image img = sg_alloc_image();
    T(sg_query_image_state(img) == SG_RESOURCESTATE_ALLOC);
    sg_uninit_image(img);
    T(log_items[0] == SG_LOGITEM_UNINIT_IMAGE_INVALID_STATE);
    T(num_log_called == 1);
    T(sg_query_image_state(img) == SG_RESOURCESTATE_ALLOC);
    sg_shutdown();
}

UTEST(sokol_gfx, alloc_uninit_sampler_warns) {
    setup(&(sg_desc){0});
    sg_sampler smp = sg_alloc_sampler();
    T(sg_query_sampler_state(smp) == SG_RESOURCESTATE_ALLOC);
    sg_uninit_sampler(smp);
    T(log_items[0] == SG_LOGITEM_UNINIT_SAMPLER_INVALID_STATE);
    T(num_log_called == 1);
    T(sg_query_sampler_state(smp) == SG_RESOURCESTATE_ALLOC);
    sg_shutdown();
}

UTEST(sokol_gfx, alloc_uninit_shader_warns) {
    setup(&(sg_desc){0});
    sg_shader shd = sg_alloc_shader();
    T(sg_query_shader_state(shd) == SG_RESOURCESTATE_ALLOC);
    sg_uninit_shader(shd);
    T(log_items[0] == SG_LOGITEM_UNINIT_SHADER_INVALID_STATE);
    T(num_log_called == 1);
    T(sg_query_shader_state(shd) == SG_RESOURCESTATE_ALLOC);
    sg_shutdown();
}

UTEST(sokol_gfx, alloc_uninit_pipeline_warns) {
    setup(&(sg_desc){0});
    sg_pipeline pip = sg_alloc_pipeline();
    T(sg_query_pipeline_state(pip) == SG_RESOURCESTATE_ALLOC);
    sg_uninit_pipeline(pip);
    T(log_items[0] == SG_LOGITEM_UNINIT_PIPELINE_INVALID_STATE);
    T(num_log_called == 1);
    T(sg_query_pipeline_state(pip) == SG_RESOURCESTATE_ALLOC);
    sg_shutdown();
}

UTEST(sokol_gfx, alloc_uninit_pass_warns) {
    setup(&(sg_desc){0});
    sg_pass pass = sg_alloc_pass();
    T(sg_query_pass_state(pass) == SG_RESOURCESTATE_ALLOC);
    sg_uninit_pass(pass);
    T(log_items[0] == SG_LOGITEM_UNINIT_PASS_INVALID_STATE);
    T(num_log_called == 1);
    T(sg_query_pass_state(pass) == SG_RESOURCESTATE_ALLOC);
    sg_shutdown();
}

UTEST(sokol_gfx, alloc_destroy_buffer_is_ok) {
    setup(&(sg_desc){0});
    sg_buffer buf = sg_alloc_buffer();
    T(sg_query_buffer_state(buf) == SG_RESOURCESTATE_ALLOC);
    sg_destroy_buffer(buf);
    T(num_log_called == 0);
    T(sg_query_buffer_state(buf) == SG_RESOURCESTATE_INVALID);
    sg_shutdown();
}

UTEST(sokol_gfx, alloc_destroy_image_is_ok) {
    setup(&(sg_desc){0});
    sg_image img = sg_alloc_image();
    T(sg_query_image_state(img) == SG_RESOURCESTATE_ALLOC);
    sg_destroy_image(img);
    T(num_log_called == 0);
    T(sg_query_image_state(img) == SG_RESOURCESTATE_INVALID);
    sg_shutdown();
}

UTEST(sokol_gfx, alloc_destroy_sampler_is_ok) {
    setup(&(sg_desc){0});
    sg_sampler smp = sg_alloc_sampler();
    T(sg_query_sampler_state(smp) == SG_RESOURCESTATE_ALLOC);
    sg_destroy_sampler(smp);
    T(num_log_called == 0);
    T(sg_query_sampler_state(smp) == SG_RESOURCESTATE_INVALID);
    sg_shutdown();

}

UTEST(sokol_gfx, alloc_destroy_shader_is_ok) {
    setup(&(sg_desc){0});
    sg_shader shd = sg_alloc_shader();
    T(sg_query_shader_state(shd) == SG_RESOURCESTATE_ALLOC);
    sg_destroy_shader(shd);
    T(num_log_called == 0);
    T(sg_query_shader_state(shd) == SG_RESOURCESTATE_INVALID);
    sg_shutdown();
}

UTEST(sokol_gfx, alloc_destroy_pipeline_is_ok) {
    setup(&(sg_desc){0});
    sg_pipeline pip = sg_alloc_pipeline();
    T(sg_query_pipeline_state(pip) == SG_RESOURCESTATE_ALLOC);
    sg_destroy_pipeline(pip);
    T(num_log_called == 0);
    T(sg_query_pipeline_state(pip) == SG_RESOURCESTATE_INVALID);
    sg_shutdown();
}

UTEST(sokol_gfx, alloc_destroy_pass_is_ok) {
    setup(&(sg_desc){0});
    sg_pass pass = sg_alloc_pass();
    T(sg_query_pass_state(pass) == SG_RESOURCESTATE_ALLOC);
    sg_destroy_pass(pass);
    T(num_log_called == 0);
    T(sg_query_pass_state(pass) == SG_RESOURCESTATE_INVALID);
    sg_shutdown();
}

UTEST(sokol_gfx, make_pipeline_with_nonvalid_shader) {
    setup(&(sg_desc){
        .disable_validation = true,
    });
    sg_shader shd = sg_alloc_shader();
    T(sg_query_shader_state(shd) == SG_RESOURCESTATE_ALLOC);
    sg_pipeline pip = sg_make_pipeline(&(sg_pipeline_desc){
        .shader = shd,
        .layout = {
            .attrs[0].format = SG_VERTEXFORMAT_FLOAT3
        },
    });
    T(sg_query_pipeline_state(pip) == SG_RESOURCESTATE_FAILED);
    sg_shutdown();
}

UTEST(sokol_gfx, make_pass_with_nonvalid_color_images) {
    setup(&(sg_desc){
        .disable_validation = true,
    });
    sg_pass pass = sg_make_pass(&(sg_pass_desc){
        .color_attachments = {
            [0].image = sg_alloc_image(),
            [1].image = sg_alloc_image(),
        },
        .depth_stencil_attachment = {
            .image = sg_make_image(&(sg_image_desc){
                .render_target = true,
                .width = 128,
                .height = 128
            })
        }
    });
    T(sg_query_pass_state(pass) == SG_RESOURCESTATE_FAILED);
    sg_destroy_pass(pass);
    T(sg_query_pass_state(pass) == SG_RESOURCESTATE_INVALID);
    sg_shutdown();
}

UTEST(sokol_gfx, make_pass_without_color_attachments) {
    setup(&(sg_desc){0});
    sg_pass pass = sg_make_pass(&(sg_pass_desc){
        .depth_stencil_attachment.image = sg_make_image(&(sg_image_desc){
            .render_target = true,
            .width = 64,
            .height = 64,
            .pixel_format = SG_PIXELFORMAT_DEPTH,
        })
    });
    T(sg_query_pass_state(pass) == SG_RESOURCESTATE_VALID);
    sg_shutdown();
}

UTEST(sokol_gfx, make_buffer_validate_start_canary) {
    setup(&(sg_desc){0});
    const uint32_t data[32] = {0};
    sg_buffer buf = sg_make_buffer(&(sg_buffer_desc){
        ._start_canary = 1234,
        .data = SG_RANGE(data),
    });
    T(sg_query_buffer_state(buf) == SG_RESOURCESTATE_FAILED);
    T(log_items[0] == SG_LOGITEM_VALIDATE_BUFFERDESC_CANARY);
    T(log_items[1] == SG_LOGITEM_VALIDATION_FAILED);
    sg_shutdown();
}

UTEST(sokol_gfx, make_buffer_validate_end_canary) {
    setup(&(sg_desc){0});
    const uint32_t data[32] = {0};
    sg_buffer buf = sg_make_buffer(&(sg_buffer_desc){
        .data = SG_RANGE(data),
        ._end_canary = 1234,
    });
    T(sg_query_buffer_state(buf) == SG_RESOURCESTATE_FAILED);
    T(log_items[0] == SG_LOGITEM_VALIDATE_BUFFERDESC_CANARY);
    T(log_items[1] == SG_LOGITEM_VALIDATION_FAILED);
    sg_shutdown();
}

UTEST(sokol_gfx, make_buffer_validate_immutable_nodata) {
    setup(&(sg_desc){0});
    sg_buffer buf = sg_make_buffer(&(sg_buffer_desc){ 0 });
    T(sg_query_buffer_state(buf) == SG_RESOURCESTATE_FAILED);
    T(log_items[0] == SG_LOGITEM_VALIDATE_BUFFERDESC_SIZE);
    T(log_items[1] == SG_LOGITEM_VALIDATE_BUFFERDESC_DATA);
    T(log_items[2] == SG_LOGITEM_VALIDATION_FAILED);
    sg_shutdown();
}

UTEST(sokol_gfx, make_buffer_validate_size_mismatch) {
    setup(&(sg_desc){0});
    const uint32_t data[16] = { 0 };
    sg_buffer buf = sg_make_buffer(&(sg_buffer_desc){
        .size = 15 * sizeof(uint32_t),
        .data = SG_RANGE(data),
    });
    T(sg_query_buffer_state(buf) == SG_RESOURCESTATE_FAILED);
    T(log_items[0] == SG_LOGITEM_VALIDATE_BUFFERDESC_DATA_SIZE);
    T(log_items[1] == SG_LOGITEM_VALIDATION_FAILED);
    sg_shutdown();
}

UTEST(sokol_gfx, make_buffer_validate_data_ptr_but_no_size) {
    setup(&(sg_desc){0});
    const uint32_t data[16] = {0};
    sg_buffer buf = sg_make_buffer(&(sg_buffer_desc){
        .data.ptr = data,
    });
    T(sg_query_buffer_state(buf) == SG_RESOURCESTATE_FAILED);
    T(log_items[0] == SG_LOGITEM_VALIDATE_BUFFERDESC_SIZE);
    T(log_items[1] == SG_LOGITEM_VALIDATE_BUFFERDESC_DATA);
    T(log_items[2] == SG_LOGITEM_VALIDATION_FAILED);
    sg_shutdown();
}

UTEST(sokol_gfx, make_image_validate_start_canary) {
    setup(&(sg_desc){0});
    const uint32_t pixels[8][8] = {0};
    sg_image img = sg_make_image(&(sg_image_desc){
        ._start_canary = 1234,
        .width = 8,
        .height = 8,
        .data.subimage[0][0] = SG_RANGE(pixels),
    });
    T(sg_query_image_state(img) == SG_RESOURCESTATE_FAILED);
    T(log_items[0] == SG_LOGITEM_VALIDATE_IMAGEDESC_CANARY);
    T(log_items[1] == SG_LOGITEM_VALIDATION_FAILED);
    sg_shutdown();
}

UTEST(sokol_gfx, make_image_validate_end_canary) {
    setup(&(sg_desc){0});
    const uint32_t pixels[8][8] = {0};
    sg_image img = sg_make_image(&(sg_image_desc){
        .width = 8,
        .height = 8,
        .data.subimage[0][0] = SG_RANGE(pixels),
        ._end_canary = 1234,
    });
    T(sg_query_image_state(img) == SG_RESOURCESTATE_FAILED);
    T(log_items[0] == SG_LOGITEM_VALIDATE_IMAGEDESC_CANARY);
    T(log_items[1] == SG_LOGITEM_VALIDATION_FAILED);
    sg_shutdown();
}

UTEST(sokol_gfx, make_image_zero_width_height) {
    setup(&(sg_desc){0});
    const uint32_t pixels[8][8] = {0};
    sg_image img = sg_make_image(&(sg_image_desc){
        .width = 0,
        .height = 0,
        .data.subimage[0][0] = SG_RANGE(pixels),
    });
    T(sg_query_image_state(img) == SG_RESOURCESTATE_FAILED);
    T(log_items[0] == SG_LOGITEM_VALIDATE_IMAGEDESC_WIDTH);
    T(log_items[1] == SG_LOGITEM_VALIDATE_IMAGEDESC_HEIGHT);
    T(log_items[2] == SG_LOGITEM_VALIDATE_IMAGEDATA_DATA_SIZE);
    T(log_items[3] == SG_LOGITEM_VALIDATION_FAILED);
    sg_shutdown();
}

UTEST(sokol_gfx, make_image_validate_msaa_no_rt) {
    setup(&(sg_desc){0});
    const uint32_t pixels[8][8] = {0};
    sg_image img = sg_make_image(&(sg_image_desc){
        .width = 8,
        .height = 8,
        .sample_count = 4,
        .data.subimage[0][0] = SG_RANGE(pixels),
    });
    T(sg_query_image_state(img) == SG_RESOURCESTATE_FAILED);
    T(log_items[0] == SG_LOGITEM_VALIDATE_IMAGEDESC_MSAA_BUT_NO_RT);
    T(log_items[1] == SG_LOGITEM_VALIDATION_FAILED);
    sg_shutdown();
}

UTEST(sokol_gfx, make_image_validate_msaa_num_mipmaps) {
    setup(&(sg_desc){0});
    sg_image img = sg_make_image(&(sg_image_desc){
        .render_target = true,
        .width = 64,
        .height = 64,
        .sample_count = 4,
        .num_mipmaps = 2,
    });
    T(sg_query_image_state(img) == SG_RESOURCESTATE_FAILED);
    T(log_items[0] == SG_LOGITEM_VALIDATE_IMAGEDESC_MSAA_NUM_MIPMAPS);
    T(log_items[1] == SG_LOGITEM_VALIDATION_FAILED);
    sg_shutdown();
}

UTEST(sokol_gfx, make_image_validate_msaa_3d_image) {
    setup(&(sg_desc){0});
    sg_image img = sg_make_image(&(sg_image_desc){
        .render_target = true,
        .type = SG_IMAGETYPE_3D,
        .width = 32,
        .height = 32,
        .num_slices = 32,
        .sample_count = 4,
    });
    T(sg_query_image_state(img) == SG_RESOURCESTATE_FAILED);
    T(log_items[0] == SG_LOGITEM_VALIDATE_IMAGEDESC_MSAA_3D_IMAGE);
    T(log_items[1] == SG_LOGITEM_VALIDATION_FAILED);
    sg_shutdown();
}

UTEST(sokol_gfx, make_image_validate_depth_3d_image_with_depth_format) {
    setup(&(sg_desc){0});
    sg_image img = sg_make_image(&(sg_image_desc){
        .render_target = true,
        .type = SG_IMAGETYPE_3D,
        .width = 8,
        .height = 8,
        .num_slices = 8,
        .pixel_format = SG_PIXELFORMAT_DEPTH,
    });
    T(sg_query_image_state(img) == SG_RESOURCESTATE_FAILED);
    T(log_items[0] == SG_LOGITEM_VALIDATE_IMAGEDESC_DEPTH_3D_IMAGE);
    T(log_items[1] == SG_LOGITEM_VALIDATION_FAILED);
    sg_shutdown();
}

UTEST(sokol_gfx, make_image_validate_rt_immutable) {
    setup(&(sg_desc){0});
    sg_image img = sg_make_image(&(sg_image_desc){
        .render_target = true,
        .usage = SG_USAGE_DYNAMIC,
        .width = 8,
        .height = 8,
    });
    T(sg_query_image_state(img) == SG_RESOURCESTATE_FAILED);
    T(log_items[0] == SG_LOGITEM_VALIDATE_IMAGEDESC_RT_IMMUTABLE);
    T(log_items[1] == SG_LOGITEM_VALIDATION_FAILED);
    sg_shutdown();
}

UTEST(sokol_gfx, make_image_validate_dynamic_no_data) {
    setup(&(sg_desc){0});
    uint32_t pixels[8][8] = {0};
    sg_image img = sg_make_image(&(sg_image_desc){
        .width = 8,
        .height = 8,
        .usage = SG_USAGE_DYNAMIC,
        .data.subimage[0][0] = SG_RANGE(pixels),
    });
    T(sg_query_image_state(img) == SG_RESOURCESTATE_FAILED);
    T(log_items[0] == SG_LOGITEM_VALIDATE_IMAGEDESC_DYNAMIC_NO_DATA);
    T(log_items[1] == SG_LOGITEM_VALIDATION_FAILED);
    sg_shutdown();
}

UTEST(sokol_gfx, make_image_valiate_compressed_immutable) {
    setup(&(sg_desc){0});
    sg_image img = sg_make_image(&(sg_image_desc){
        .width = 8,
        .height = 8,
        .pixel_format = SG_PIXELFORMAT_BC1_RGBA,
        .usage = SG_USAGE_DYNAMIC,
    });
    T(sg_query_image_state(img) == SG_RESOURCESTATE_FAILED);
    T(log_items[0] == SG_LOGITEM_VALIDATE_IMAGEDESC_COMPRESSED_IMMUTABLE);
    T(log_items[1] == SG_LOGITEM_VALIDATION_FAILED);
    sg_shutdown();
}

UTEST(sokol_gfx, make_image_validate_nodata) {
    setup(&(sg_desc){0});
    sg_image img = sg_make_image(&(sg_image_desc){
        .width = 8,
        .height = 8,
    });
    T(sg_query_image_state(img) == SG_RESOURCESTATE_FAILED);
    T(log_items[0] == SG_LOGITEM_VALIDATE_IMAGEDATA_NODATA);
    T(log_items[1] == SG_LOGITEM_VALIDATE_IMAGEDATA_DATA_SIZE);
    T(log_items[2] == SG_LOGITEM_VALIDATION_FAILED);
    sg_shutdown();
}

UTEST(sokol_gfx, make_image_validate_data_size) {
    setup(&(sg_desc){0});
    uint32_t pixels[4][4] = {0};
    sg_image img = sg_make_image(&(sg_image_desc){
        .width = 8,
        .height = 8,
        .data.subimage[0][0] = SG_RANGE(pixels),
    });
    T(sg_query_image_state(img) == SG_RESOURCESTATE_FAILED);
    T(log_items[0] == SG_LOGITEM_VALIDATE_IMAGEDATA_DATA_SIZE);
    T(log_items[1] == SG_LOGITEM_VALIDATION_FAILED);
    sg_shutdown();
}

UTEST(sokol_gfx, make_image_validate_missing_mipdata) {
    setup(&(sg_desc){0});
    uint32_t mip0[8][8] = {0};
    uint32_t mip1[4][4] = {0};
    uint32_t mip2[2][2] = {0};
    sg_image img = sg_make_image(&(sg_image_desc){
        .width = 8,
        .height = 8,
        .num_mipmaps = 4,
        .data.subimage[0][0] = SG_RANGE(mip0),
        .data.subimage[0][1] = SG_RANGE(mip1),
        .data.subimage[0][2] = SG_RANGE(mip2),
    });
    T(sg_query_image_state(img) == SG_RESOURCESTATE_FAILED);
    T(log_items[0] == SG_LOGITEM_VALIDATE_IMAGEDATA_NODATA);
    T(log_items[1] == SG_LOGITEM_VALIDATE_IMAGEDATA_DATA_SIZE);
    T(log_items[2] == SG_LOGITEM_VALIDATION_FAILED);
    sg_shutdown();
}

UTEST(sokol_gfx, make_image_validate_wrong_mipsize) {
    setup(&(sg_desc){0});
    uint32_t mip0[8][8] = {0};
    uint32_t mip1[4][4] = {0};
    uint32_t mip2[2][2] = {0};
    uint32_t mip3[1][1] = {0};
    sg_image img = sg_make_image(&(sg_image_desc){
        .width = 8,
        .height = 8,
        .num_mipmaps = 4,
        .data.subimage[0][0] = SG_RANGE(mip0),
        .data.subimage[0][1] = SG_RANGE(mip2),
        .data.subimage[0][2] = SG_RANGE(mip1),
        .data.subimage[0][3] = SG_RANGE(mip3)
    });
    T(sg_query_image_state(img) == SG_RESOURCESTATE_FAILED);
    T(log_items[0] == SG_LOGITEM_VALIDATE_IMAGEDATA_DATA_SIZE);
    T(log_items[1] == SG_LOGITEM_VALIDATE_IMAGEDATA_DATA_SIZE);
    T(log_items[2] == SG_LOGITEM_VALIDATION_FAILED);
    sg_shutdown();
}

UTEST(sokol_gfx, make_sampler_validate_start_canary) {
    setup(&(sg_desc){0});
    sg_sampler smp = sg_make_sampler(&(sg_sampler_desc){
        ._start_canary = 1234,
    });
    T(sg_query_sampler_state(smp) == SG_RESOURCESTATE_FAILED);
    T(log_items[0] == SG_LOGITEM_VALIDATE_SAMPLERDESC_CANARY);
    sg_shutdown();
}

UTEST(sokol_gfx, make_sampler_validate_minfilter_none) {
    setup(&(sg_desc){0});
    sg_sampler smp = sg_make_sampler(&(sg_sampler_desc){
        .min_filter = SG_FILTER_NONE,
    });
    T(sg_query_sampler_state(smp) == SG_RESOURCESTATE_FAILED);
    T(log_items[0] == SG_LOGITEM_VALIDATE_SAMPLERDESC_MINFILTER_NONE);
    sg_shutdown();
}

UTEST(sokol_gfx, make_sampler_validate_magfilter_none) {
    setup(&(sg_desc){0});
    sg_sampler smp = sg_make_sampler(&(sg_sampler_desc){
        .mag_filter = SG_FILTER_NONE,
    });
    T(sg_query_sampler_state(smp) == SG_RESOURCESTATE_FAILED);
    T(log_items[0] == SG_LOGITEM_VALIDATE_SAMPLERDESC_MAGFILTER_NONE);
    sg_shutdown();
}

UTEST(sokol_gfx, make_sampler_validate_anistropic_requires_linear_filtering) {
    setup(&(sg_desc){0});
    sg_sampler smp;

    smp = sg_make_sampler(&(sg_sampler_desc){
        .max_anisotropy = 2,
        .min_filter = SG_FILTER_LINEAR,
        .mag_filter = SG_FILTER_LINEAR,
        .mipmap_filter = SG_FILTER_NONE,
    });
    T(sg_query_sampler_state(smp) == SG_RESOURCESTATE_FAILED);
    T(log_items[0] == SG_LOGITEM_VALIDATE_SAMPLERDESC_ANISTROPIC_REQUIRES_LINEAR_FILTERING);

    reset_log_items();
    smp = sg_make_sampler(&(sg_sampler_desc){
        .max_anisotropy = 2,
        .min_filter = SG_FILTER_LINEAR,
        .mag_filter = SG_FILTER_LINEAR,
        .mipmap_filter = SG_FILTER_NEAREST,
    });
    T(sg_query_sampler_state(smp) == SG_RESOURCESTATE_FAILED);
    T(log_items[0] == SG_LOGITEM_VALIDATE_SAMPLERDESC_ANISTROPIC_REQUIRES_LINEAR_FILTERING);

    reset_log_items();
    smp = sg_make_sampler(&(sg_sampler_desc){
        .max_anisotropy = 2,
        .min_filter = SG_FILTER_NEAREST,
        .mag_filter = SG_FILTER_LINEAR,
        .mipmap_filter = SG_FILTER_LINEAR,
    });
    T(sg_query_sampler_state(smp) == SG_RESOURCESTATE_FAILED);
    T(log_items[0] == SG_LOGITEM_VALIDATE_SAMPLERDESC_ANISTROPIC_REQUIRES_LINEAR_FILTERING);

    reset_log_items();
    smp = sg_make_sampler(&(sg_sampler_desc){
        .max_anisotropy = 2,
        .min_filter = SG_FILTER_LINEAR,
        .mag_filter = SG_FILTER_NEAREST,
        .mipmap_filter = SG_FILTER_LINEAR,
    });
    T(sg_query_sampler_state(smp) == SG_RESOURCESTATE_FAILED);
    T(log_items[0] == SG_LOGITEM_VALIDATE_SAMPLERDESC_ANISTROPIC_REQUIRES_LINEAR_FILTERING);

    reset_log_items();
    smp = sg_make_sampler(&(sg_sampler_desc){
        .max_anisotropy = 2,
        .min_filter = SG_FILTER_LINEAR,
        .mag_filter = SG_FILTER_LINEAR,
        .mipmap_filter = SG_FILTER_LINEAR,
    });
    T(sg_query_sampler_state(smp) == SG_RESOURCESTATE_VALID);

    sg_shutdown();
}

UTEST(sokol_gfx, make_pass_validate_start_canary) {
    setup(&(sg_desc){0});
    sg_pass pass = sg_make_pass(&(sg_pass_desc){
        ._start_canary = 1234,
        .color_attachments[0].image = sg_make_image(&(sg_image_desc){
            .render_target = true,
            .width = 64,
            .height = 64,
        }),
    });
    T(sg_query_pass_state(pass) == SG_RESOURCESTATE_FAILED);
    T(log_items[0] == SG_LOGITEM_VALIDATE_PASSDESC_CANARY);
    sg_shutdown();
}

UTEST(sokol_gfx, make_pass_validate_end_canary) {
    setup(&(sg_desc){0});
    sg_pass pass = sg_make_pass(&(sg_pass_desc){
        .color_attachments[0].image = sg_make_image(&(sg_image_desc){
            .render_target = true,
            .width = 64,
            .height = 64,
        }),
        ._end_canary = 1234,
    });
    T(sg_query_pass_state(pass) == SG_RESOURCESTATE_FAILED);
    T(log_items[0] == SG_LOGITEM_VALIDATE_PASSDESC_CANARY);
    T(log_items[1] == SG_LOGITEM_VALIDATION_FAILED);
    sg_shutdown();
}

UTEST(sokol_gfx, make_pass_validate_no_cont_color_atts1) {
    setup(&(sg_desc){0});
    const sg_image_desc img_desc = { .render_target = true, .width = 64, .height = 64 };
    sg_pass pass = sg_make_pass(&(sg_pass_desc){
        .color_attachments = {
            [0].image = sg_make_image(&img_desc),
            [2].image = sg_make_image(&img_desc),
        }
    });
    T(sg_query_pass_state(pass) == SG_RESOURCESTATE_FAILED);
    T(log_items[0] == SG_LOGITEM_VALIDATE_PASSDESC_NO_CONT_COLOR_ATTS);
    T(log_items[1] == SG_LOGITEM_VALIDATION_FAILED);
    sg_shutdown();
}

UTEST(sokol_gfx, make_pass_validate_image) {
    setup(&(sg_desc){0});
    const sg_image_desc img_desc = { .render_target = true, .width = 64, .height = 64 };
    const sg_image img0 = sg_make_image(&img_desc);
    const sg_image img1 = sg_make_image(&img_desc);
    sg_destroy_image(img1);
    const sg_pass pass = sg_make_pass(&(sg_pass_desc){
        .color_attachments = {
            [0].image = img0,
            [1].image = img1,
        }
    });
    T(sg_query_pass_state(pass) == SG_RESOURCESTATE_FAILED);
    T(log_items[0] == SG_LOGITEM_VALIDATE_PASSDESC_IMAGE);
    T(log_items[1] == SG_LOGITEM_VALIDATION_FAILED);
    sg_shutdown();
}

UTEST(sokol_gfx, make_pass_validate_miplevel) {
    setup(&(sg_desc){0});
    const sg_image img = sg_make_image(&(sg_image_desc){
        .render_target = true,
        .width = 16,
        .height = 16,
        .num_mipmaps = 4,
    });
    const sg_pass pass = sg_make_pass(&(sg_pass_desc){
        .color_attachments[0] = { .image = img, .mip_level = 4 }
    });
    T(sg_query_pass_state(pass) == SG_RESOURCESTATE_FAILED);
    T(log_items[0] == SG_LOGITEM_VALIDATE_PASSDESC_MIPLEVEL);
    T(log_items[1] == SG_LOGITEM_VALIDATION_FAILED);
    sg_shutdown();
}

UTEST(sokol_gfx, make_pass_validate_face) {
    setup(&(sg_desc){0});
    const sg_image img = sg_make_image(&(sg_image_desc){
        .render_target = true,
        .type = SG_IMAGETYPE_CUBE,
        .width = 64,
        .height = 64,
    });
    const sg_pass pass = sg_make_pass(&(sg_pass_desc){
        .color_attachments[0] = { .image = img, .slice = 6 }
    });
    T(sg_query_pass_state(pass) == SG_RESOURCESTATE_FAILED);
    T(log_items[0] == SG_LOGITEM_VALIDATE_PASSDESC_FACE);
    T(log_items[1] == SG_LOGITEM_VALIDATION_FAILED);
    sg_shutdown();
}

UTEST(sokol_gfx, make_pass_validate_layer) {
    setup(&(sg_desc){0});
    const sg_image img = sg_make_image(&(sg_image_desc){
        .render_target = true,
        .type = SG_IMAGETYPE_ARRAY,
        .width = 64,
        .height = 64,
        .num_slices = 4,
    });
    const sg_pass pass = sg_make_pass(&(sg_pass_desc){
        .color_attachments[0] = { .image = img, .slice = 5 },
    });
    T(sg_query_pass_state(pass) == SG_RESOURCESTATE_FAILED);
    T(log_items[0] == SG_LOGITEM_VALIDATE_PASSDESC_LAYER);
    T(log_items[1] == SG_LOGITEM_VALIDATION_FAILED);
    sg_shutdown();
}

UTEST(sokol_gfx, make_pass_validate_slice) {
    setup(&(sg_desc){0});
    const sg_image img = sg_make_image(&(sg_image_desc){
        .render_target = true,
        .type = SG_IMAGETYPE_3D,
        .width = 64,
        .height = 64,
        .num_slices = 4,
    });
    const sg_pass pass = sg_make_pass(&(sg_pass_desc){
        .color_attachments[0] = { .image = img, .slice = 5 },
    });
    T(sg_query_pass_state(pass) == SG_RESOURCESTATE_FAILED);
    T(log_items[0] == SG_LOGITEM_VALIDATE_PASSDESC_SLICE);
    T(log_items[1] == SG_LOGITEM_VALIDATION_FAILED);
    sg_shutdown();
}

UTEST(sokol_gfx, make_pass_validate_image_no_rt) {
    setup(&(sg_desc){0});
    const sg_image img = sg_make_image(&(sg_image_desc){
        .width = 8,
        .height = 8,
        .usage = SG_USAGE_DYNAMIC,
    });
    const sg_pass pass = sg_make_pass(&(sg_pass_desc){
        .color_attachments[0].image = img,
    });
    T(sg_query_pass_state(pass) == SG_RESOURCESTATE_FAILED);
    T(log_items[0] == SG_LOGITEM_VALIDATE_PASSDESC_IMAGE_NO_RT);
    T(log_items[1] == SG_LOGITEM_VALIDATION_FAILED);
    sg_shutdown();
}

UTEST(sokol_gfx, make_pass_validate_color_inv_pixelformat) {
    setup(&(sg_desc){0});
    const sg_image_desc img_desc = {
        .render_target = true,
        .width = 8,
        .height = 8,
        .pixel_format = SG_PIXELFORMAT_DEPTH,
    };
    reset_log_items();
    const sg_pass pass = sg_make_pass(&(sg_pass_desc){
        .color_attachments[0].image = sg_make_image(&img_desc),
        .depth_stencil_attachment.image = sg_make_image(&img_desc),
    });
    T(sg_query_pass_state(pass) == SG_RESOURCESTATE_FAILED);
    T(log_items[0] == SG_LOGITEM_VALIDATE_PASSDESC_COLOR_INV_PIXELFORMAT);
    T(log_items[1] == SG_LOGITEM_VALIDATION_FAILED);
    sg_shutdown();
}

UTEST(sokol_gfx, make_pass_validate_depth_inv_pixelformat) {
    setup(&(sg_desc){0});
    const sg_image_desc img_desc = {
        .render_target = true,
        .width = 8,
        .height = 8,
    };
    const sg_pass pass = sg_make_pass(&(sg_pass_desc){
        .color_attachments[0].image = sg_make_image(&img_desc),
        .depth_stencil_attachment.image = sg_make_image(&img_desc),
    });
    T(sg_query_pass_state(pass) == SG_RESOURCESTATE_FAILED);
    T(log_items[0] == SG_LOGITEM_VALIDATE_PASSDESC_DEPTH_INV_PIXELFORMAT);
    T(log_items[1] == SG_LOGITEM_VALIDATION_FAILED);
    sg_shutdown();
}

UTEST(sokol_gfx, make_pass_validate_image_sizes) {
    setup(&(sg_desc){0});
    const sg_image img0 = sg_make_image(&(sg_image_desc){
        .render_target = true,
        .width = 64,
        .height = 64,
    });
    const sg_image img1 = sg_make_image(&(sg_image_desc){
        .render_target = true,
        .width = 32,
        .height = 32,
    });
    const sg_pass pass = sg_make_pass(&(sg_pass_desc){
        .color_attachments = {
            [0].image = img0,
            [1].image = img1,
        }
    });
    T(sg_query_pass_state(pass) == SG_RESOURCESTATE_FAILED);
    T(log_items[0] == SG_LOGITEM_VALIDATE_PASSDESC_IMAGE_SIZES);
    T(log_items[1] == SG_LOGITEM_VALIDATE_PASSDESC_IMAGE_SIZES);
    T(log_items[2] == SG_LOGITEM_VALIDATION_FAILED);
    sg_shutdown();
}

UTEST(sokol_gfx, make_pass_validate_image_sample_counts) {
    setup(&(sg_desc){0});
    const sg_image img0 = sg_make_image(&(sg_image_desc){
        .render_target = true,
        .width = 64,
        .height = 64,
        .sample_count = 4,
    });
    const sg_image img1 = sg_make_image(&(sg_image_desc){
        .render_target = true,
        .width = 64,
        .height = 64,
        .sample_count = 2,
    });
    const sg_pass pass = sg_make_pass(&(sg_pass_desc){
        .color_attachments = {
            [0].image = img0,
            [1].image = img1,
        }
    });
    T(sg_query_pass_state(pass) == SG_RESOURCESTATE_FAILED);
    T(log_items[0] == SG_LOGITEM_VALIDATE_PASSDESC_IMAGE_SAMPLE_COUNTS);
    T(log_items[1] == SG_LOGITEM_VALIDATION_FAILED);
    sg_shutdown();
}

UTEST(sokol_gfx, make_pass_validate_resolve_color_image_msaa) {
    setup(&(sg_desc){0});
    const sg_image color_img = sg_make_image(&(sg_image_desc){
        .render_target = true,
        .width = 64,
        .height = 64,
        .sample_count = 1,
    });
    const sg_image resolve_img = sg_make_image(&(sg_image_desc){
        .render_target = true,
        .width = 64,
        .height = 64,
        .sample_count = 1,
    });
    const sg_pass pass = sg_make_pass(&(sg_pass_desc){
        .color_attachments[0].image = color_img,
        .resolve_attachments[0].image = resolve_img,
    });
    T(sg_query_pass_state(pass) == SG_RESOURCESTATE_FAILED);
    T(log_items[0] == SG_LOGITEM_VALIDATE_PASSDESC_RESOLVE_COLOR_IMAGE_MSAA);
    T(log_items[1] == SG_LOGITEM_VALIDATION_FAILED);
    sg_shutdown();
}

UTEST(sokol_gfx, make_pass_validate_resolve_image) {
    setup(&(sg_desc){0});
    const sg_image color_img = sg_make_image(&(sg_image_desc){
        .render_target = true,
        .width = 64,
        .height = 64,
        .sample_count = 4,
    });
    const sg_image resolve_img = sg_make_image(&(sg_image_desc){
        .render_target = true,
        .width = 64,
        .height = 64,
        .sample_count = 1,
    });
    sg_destroy_image(resolve_img);
    const sg_pass pass = sg_make_pass(&(sg_pass_desc){
        .color_attachments[0].image = color_img,
        .resolve_attachments[0].image = resolve_img,
    });
    T(sg_query_pass_state(pass) == SG_RESOURCESTATE_FAILED);
    T(log_items[0] == SG_LOGITEM_VALIDATE_PASSDESC_RESOLVE_IMAGE);
    T(log_items[1] == SG_LOGITEM_VALIDATION_FAILED);
    sg_shutdown();
}

UTEST(sokol_gfx, make_pass_validate_resolve_sample_count) {
    setup(&(sg_desc){0});
    const sg_image color_img = sg_make_image(&(sg_image_desc){
        .render_target = true,
        .width = 64,
        .height = 64,
        .sample_count = 4,
    });
    const sg_image resolve_img = sg_make_image(&(sg_image_desc){
        .render_target = true,
        .width = 64,
        .height = 64,
        .sample_count = 4,
    });
    const sg_pass pass = sg_make_pass(&(sg_pass_desc){
        .color_attachments[0].image = color_img,
        .resolve_attachments[0].image = resolve_img,
    });
    T(sg_query_pass_state(pass) == SG_RESOURCESTATE_FAILED);
    T(log_items[0] == SG_LOGITEM_VALIDATE_PASSDESC_RESOLVE_SAMPLE_COUNT);
    T(log_items[1] == SG_LOGITEM_VALIDATION_FAILED);
    sg_shutdown();
}

UTEST(sokol_gfx, make_pass_validate_resolve_miplevel) {
    setup(&(sg_desc){0});
    const sg_image color_img = sg_make_image(&(sg_image_desc){
        .render_target = true,
        .width = 64,
        .height = 64,
        .sample_count = 4,
    });
    const sg_image resolve_img = sg_make_image(&(sg_image_desc){
        .render_target = true,
        .width = 64,
        .height = 64,
        .sample_count = 1,
    });
    const sg_pass pass = sg_make_pass(&(sg_pass_desc){
        .color_attachments[0] = { .image = color_img },
        .resolve_attachments[0] = { .image = resolve_img, .mip_level = 1 },
    });
    T(sg_query_pass_state(pass) == SG_RESOURCESTATE_FAILED);
    T(log_items[0] == SG_LOGITEM_VALIDATE_PASSDESC_RESOLVE_MIPLEVEL);
    // FIXME: these are confusing
    T(log_items[1] == SG_LOGITEM_VALIDATE_PASSDESC_RESOLVE_IMAGE_SIZES);
    T(log_items[2] == SG_LOGITEM_VALIDATE_PASSDESC_RESOLVE_IMAGE_SIZES);
    T(log_items[3] == SG_LOGITEM_VALIDATION_FAILED);
    sg_shutdown();
}

UTEST(sokol_gfx, make_pass_validate_resolve_face) {
    setup(&(sg_desc){0});
    const sg_image color_img = sg_make_image(&(sg_image_desc){
        .render_target = true,
        .width = 64,
        .height = 64,
        .sample_count = 4,
    });
    const sg_image resolve_img = sg_make_image(&(sg_image_desc){
        .render_target = true,
        .type = SG_IMAGETYPE_CUBE,
        .width = 64,
        .height = 64,
        .sample_count = 1,
    });
    const sg_pass pass = sg_make_pass(&(sg_pass_desc){
        .color_attachments[0] = { .image = color_img },
        .resolve_attachments[0] = { .image = resolve_img, .slice = 6 },
    });
    T(sg_query_pass_state(pass) == SG_RESOURCESTATE_FAILED);
    T(log_items[0] == SG_LOGITEM_VALIDATE_PASSDESC_RESOLVE_FACE);
    T(log_items[1] == SG_LOGITEM_VALIDATION_FAILED);
    sg_shutdown();
}

UTEST(sokol_gfx, make_pass_validate_resolve_layer) {
    setup(&(sg_desc){0});
    const sg_image color_img = sg_make_image(&(sg_image_desc){
        .render_target = true,
        .width = 64,
        .height = 64,
        .sample_count = 4,
    });
    const sg_image resolve_img = sg_make_image(&(sg_image_desc){
        .render_target = true,
        .type = SG_IMAGETYPE_ARRAY,
        .width = 64,
        .height = 64,
        .num_slices = 4,
        .sample_count = 1,
    });
    const sg_pass pass = sg_make_pass(&(sg_pass_desc){
        .color_attachments[0] = { .image = color_img },
        .resolve_attachments[0] = { .image = resolve_img, .slice = 4 },
    });
    T(sg_query_pass_state(pass) == SG_RESOURCESTATE_FAILED);
    T(log_items[0] == SG_LOGITEM_VALIDATE_PASSDESC_RESOLVE_LAYER);
    T(log_items[1] == SG_LOGITEM_VALIDATION_FAILED);
    sg_shutdown();
}

UTEST(sokol_gfx, make_pass_validate_resolve_slice) {
    setup(&(sg_desc){0});
    const sg_image color_img = sg_make_image(&(sg_image_desc){
        .render_target = true,
        .width = 64,
        .height = 64,
        .sample_count = 4,
    });
    const sg_image resolve_img = sg_make_image(&(sg_image_desc){
        .render_target = true,
        .type = SG_IMAGETYPE_3D,
        .width = 64,
        .height = 64,
        .num_slices = 4,
        .sample_count = 1,
    });
    const sg_pass pass = sg_make_pass(&(sg_pass_desc){
        .color_attachments[0] = { .image = color_img },
        .resolve_attachments[0] = { .image = resolve_img, .slice = 4 },
    });
    T(sg_query_pass_state(pass) == SG_RESOURCESTATE_FAILED);
    T(log_items[0] == SG_LOGITEM_VALIDATE_PASSDESC_RESOLVE_SLICE);
    T(log_items[1] == SG_LOGITEM_VALIDATION_FAILED);
    sg_shutdown();
}

UTEST(sokol_gfx, make_pass_validate_resolve_image_no_rt) {
    setup(&(sg_desc){0});
    const sg_image color_img = sg_make_image(&(sg_image_desc){
        .render_target = true,
        .width = 64,
        .height = 64,
        .sample_count = 4,
    });
    const sg_image resolve_img = sg_make_image(&(sg_image_desc){
        .width = 64,
        .height = 64,
        .usage = SG_USAGE_DYNAMIC,
        .sample_count = 1,
    });
    const sg_pass pass = sg_make_pass(&(sg_pass_desc){
        .color_attachments[0] = { .image = color_img },
        .resolve_attachments[0] = { .image = resolve_img },
    });
    T(sg_query_pass_state(pass) == SG_RESOURCESTATE_FAILED);
    T(log_items[0] == SG_LOGITEM_VALIDATE_PASSDESC_RESOLVE_IMAGE_NO_RT);
    T(log_items[1] == SG_LOGITEM_VALIDATION_FAILED);
    sg_shutdown();
}

UTEST(sokol_gfx, make_pass_validate_resolve_image_sizes) {
    setup(&(sg_desc){0});
    const sg_image color_img = sg_make_image(&(sg_image_desc){
        .render_target = true,
        .width = 64,
        .height = 64,
        .sample_count = 4,
    });
    const sg_image resolve_img = sg_make_image(&(sg_image_desc){
        .render_target = true,
        .width = 32,
        .height = 32,
        .sample_count = 1,
    });
    const sg_pass pass = sg_make_pass(&(sg_pass_desc){
        .color_attachments[0] = { .image = color_img },
        .resolve_attachments[0] = { .image = resolve_img },
    });
    T(sg_query_pass_state(pass) == SG_RESOURCESTATE_FAILED);
    T(log_items[0] == SG_LOGITEM_VALIDATE_PASSDESC_RESOLVE_IMAGE_SIZES);
    T(log_items[1] == SG_LOGITEM_VALIDATE_PASSDESC_RESOLVE_IMAGE_SIZES);
    T(log_items[2] == SG_LOGITEM_VALIDATION_FAILED);
    sg_shutdown();
}

UTEST(sokol_gfx, make_pass_validate_resolve_image_format) {
    setup(&(sg_desc){0});
    const sg_image color_img = sg_make_image(&(sg_image_desc){
        .render_target = true,
        .width = 64,
        .height = 64,
        .sample_count = 4,
    });
    const sg_image resolve_img = sg_make_image(&(sg_image_desc){
        .render_target = true,
        .width = 64,
        .height = 64,
        .pixel_format = SG_PIXELFORMAT_R8,
        .sample_count = 1,
    });
    const sg_pass pass = sg_make_pass(&(sg_pass_desc){
        .color_attachments[0] = { .image = color_img },
        .resolve_attachments[0] = { .image = resolve_img },
    });
    T(sg_query_pass_state(pass) == SG_RESOURCESTATE_FAILED);
    T(log_items[0] == SG_LOGITEM_VALIDATE_PASSDESC_RESOLVE_IMAGE_FORMAT);
    T(log_items[1] == SG_LOGITEM_VALIDATION_FAILED);
    sg_shutdown();
}

UTEST(sokol_gfx, make_pass_validate_depth_image) {
    setup(&(sg_desc){0});
    const sg_image color_img = sg_make_image(&(sg_image_desc){
        .render_target = true,
        .width = 64,
        .height = 64,
    });
    const sg_image depth_img = sg_make_image(&(sg_image_desc){
        .render_target = true,
        .width = 64,
        .height = 64,
        .pixel_format = SG_PIXELFORMAT_DEPTH,
    });
    sg_destroy_image(depth_img);
    const sg_pass pass = sg_make_pass(&(sg_pass_desc){
        .color_attachments[0] = { .image = color_img },
        .depth_stencil_attachment.image = depth_img,
    });
    T(sg_query_pass_state(pass) == SG_RESOURCESTATE_FAILED);
    T(log_items[0] == SG_LOGITEM_VALIDATE_PASSDESC_DEPTH_IMAGE);
    T(log_items[1] == SG_LOGITEM_VALIDATION_FAILED);
    sg_shutdown();
}

UTEST(sokol_gfx, make_pass_validate_depth_miplevel) {
    setup(&(sg_desc){0});
    const sg_image color_img = sg_make_image(&(sg_image_desc){
        .render_target = true,
        .width = 64,
        .height = 64,
    });
    const sg_image depth_img = sg_make_image(&(sg_image_desc){
        .render_target = true,
        .width = 64,
        .height = 64,
        .pixel_format = SG_PIXELFORMAT_DEPTH,
    });
    const sg_pass pass = sg_make_pass(&(sg_pass_desc){
        .color_attachments[0] = { .image = color_img },
        .depth_stencil_attachment = { .image = depth_img, .mip_level = 1 },
    });
    T(sg_query_pass_state(pass) == SG_RESOURCESTATE_FAILED);
    T(log_items[0] == SG_LOGITEM_VALIDATE_PASSDESC_DEPTH_MIPLEVEL);
    // FIXME: these additional validation errors are confusing
    T(log_items[1] == SG_LOGITEM_VALIDATE_PASSDESC_DEPTH_IMAGE_SIZES);
    T(log_items[2] == SG_LOGITEM_VALIDATE_PASSDESC_DEPTH_IMAGE_SIZES);
    T(log_items[3] == SG_LOGITEM_VALIDATION_FAILED);
    sg_shutdown();
}

UTEST(sokol_gfx, make_pass_validate_depth_face) {
    setup(&(sg_desc){0});
    const sg_image color_img = sg_make_image(&(sg_image_desc){
        .render_target = true,
        .width = 64,
        .height = 64,
    });
    const sg_image depth_img = sg_make_image(&(sg_image_desc){
        .render_target = true,
        .type = SG_IMAGETYPE_CUBE,
        .width = 64,
        .height = 64,
        .pixel_format = SG_PIXELFORMAT_DEPTH,
    });
    const sg_pass pass = sg_make_pass(&(sg_pass_desc){
        .color_attachments[0] = { .image = color_img },
        .depth_stencil_attachment = { .image = depth_img, .slice = 6 },
    });
    T(sg_query_pass_state(pass) == SG_RESOURCESTATE_FAILED);
    T(log_items[0] == SG_LOGITEM_VALIDATE_PASSDESC_DEPTH_FACE);
    T(log_items[1] == SG_LOGITEM_VALIDATION_FAILED);
    sg_shutdown();
}

UTEST(sokol_gfx, make_pass_validate_depth_layer) {
    setup(&(sg_desc){0});
    const sg_image color_img = sg_make_image(&(sg_image_desc){
        .render_target = true,
        .width = 64,
        .height = 64,
    });
    const sg_image depth_img = sg_make_image(&(sg_image_desc){
        .render_target = true,
        .type = SG_IMAGETYPE_ARRAY,
        .width = 64,
        .height = 64,
        .num_slices = 4,
        .pixel_format = SG_PIXELFORMAT_DEPTH,
    });
    const sg_pass pass = sg_make_pass(&(sg_pass_desc){
        .color_attachments[0] = { .image = color_img },
        .depth_stencil_attachment = { .image = depth_img, .slice = 4 },
    });
    T(sg_query_pass_state(pass) == SG_RESOURCESTATE_FAILED);
    T(log_items[0] == SG_LOGITEM_VALIDATE_PASSDESC_DEPTH_LAYER);
    T(log_items[1] == SG_LOGITEM_VALIDATION_FAILED);
    sg_shutdown();
}

// NOTE: VALIDATE_PASSDESC_DEPTH_SLICE can't actually happen because VALIDATE_IMAGEDESC_DEPTH_3D_IMAGE

// NOTE: VALIDATE_DEPTH_IMAGE_NO_RT can't actually happen because VALIDATE_IMAGEDESC_NONRT_PIXELFORMAT

UTEST(sokol_gfx, make_pass_validate_depth_image_sizes) {
    setup(&(sg_desc){0});
    const sg_image color_img = sg_make_image(&(sg_image_desc){
        .render_target = true,
        .width = 64,
        .height = 64,
    });
    const sg_image depth_img = sg_make_image(&(sg_image_desc){
        .render_target = true,
        .width = 32,
        .height = 32,
        .pixel_format = SG_PIXELFORMAT_DEPTH,
    });
    const sg_pass pass = sg_make_pass(&(sg_pass_desc){
        .color_attachments[0] = { .image = color_img },
        .depth_stencil_attachment = { .image = depth_img },
    });
    T(sg_query_pass_state(pass) == SG_RESOURCESTATE_FAILED);
    T(log_items[0] == SG_LOGITEM_VALIDATE_PASSDESC_DEPTH_IMAGE_SIZES);
    T(log_items[1] == SG_LOGITEM_VALIDATE_PASSDESC_DEPTH_IMAGE_SIZES);
    T(log_items[2] == SG_LOGITEM_VALIDATION_FAILED);
    sg_shutdown();
}

UTEST(sokol_gfx, make_pass_validate_depth_image_sample_count) {
    setup(&(sg_desc){0});
    const sg_image color_img = sg_make_image(&(sg_image_desc){
        .render_target = true,
        .width = 64,
        .height = 64,
        .sample_count = 4,
    });
    const sg_image depth_img = sg_make_image(&(sg_image_desc){
        .render_target = true,
        .width = 64,
        .height = 64,
        .pixel_format = SG_PIXELFORMAT_DEPTH,
        .sample_count = 2,
    });
    const sg_pass pass = sg_make_pass(&(sg_pass_desc){
        .color_attachments[0] = { .image = color_img },
        .depth_stencil_attachment = { .image = depth_img },
    });
    T(sg_query_pass_state(pass) == SG_RESOURCESTATE_FAILED);
    T(log_items[0] == SG_LOGITEM_VALIDATE_PASSDESC_DEPTH_IMAGE_SAMPLE_COUNT);
    T(log_items[1] == SG_LOGITEM_VALIDATION_FAILED);
    sg_shutdown();
}

UTEST(sokol_gfx, query_pixelformat_bytesperpixel) {
    setup(&(sg_desc){0});
    T(sg_query_pixelformat(SG_PIXELFORMAT_R8).bytes_per_pixel == 1);
    T(sg_query_pixelformat(SG_PIXELFORMAT_R8SN).bytes_per_pixel == 1);
    T(sg_query_pixelformat(SG_PIXELFORMAT_R8UI).bytes_per_pixel == 1);
    T(sg_query_pixelformat(SG_PIXELFORMAT_R8SI).bytes_per_pixel == 1);
    T(sg_query_pixelformat(SG_PIXELFORMAT_R16).bytes_per_pixel == 2);
    T(sg_query_pixelformat(SG_PIXELFORMAT_R16SN).bytes_per_pixel == 2);
    T(sg_query_pixelformat(SG_PIXELFORMAT_R16UI).bytes_per_pixel == 2);
    T(sg_query_pixelformat(SG_PIXELFORMAT_R16SI).bytes_per_pixel == 2);
    T(sg_query_pixelformat(SG_PIXELFORMAT_R16F).bytes_per_pixel == 2);
    T(sg_query_pixelformat(SG_PIXELFORMAT_RG8).bytes_per_pixel == 2);
    T(sg_query_pixelformat(SG_PIXELFORMAT_RG8SN).bytes_per_pixel == 2);
    T(sg_query_pixelformat(SG_PIXELFORMAT_RG8UI).bytes_per_pixel == 2);
    T(sg_query_pixelformat(SG_PIXELFORMAT_RG8SI).bytes_per_pixel == 2);
    T(sg_query_pixelformat(SG_PIXELFORMAT_R32UI).bytes_per_pixel == 4);
    T(sg_query_pixelformat(SG_PIXELFORMAT_R32SI).bytes_per_pixel == 4);
    T(sg_query_pixelformat(SG_PIXELFORMAT_R32F).bytes_per_pixel == 4);
    T(sg_query_pixelformat(SG_PIXELFORMAT_RG16).bytes_per_pixel == 4);
    T(sg_query_pixelformat(SG_PIXELFORMAT_RG16SN).bytes_per_pixel == 4);
    T(sg_query_pixelformat(SG_PIXELFORMAT_RG16UI).bytes_per_pixel == 4);
    T(sg_query_pixelformat(SG_PIXELFORMAT_RG16SI).bytes_per_pixel == 4);
    T(sg_query_pixelformat(SG_PIXELFORMAT_RG16F).bytes_per_pixel == 4);
    T(sg_query_pixelformat(SG_PIXELFORMAT_RGBA8).bytes_per_pixel == 4);
    T(sg_query_pixelformat(SG_PIXELFORMAT_SRGB8A8).bytes_per_pixel == 4);
    T(sg_query_pixelformat(SG_PIXELFORMAT_RGBA8SN).bytes_per_pixel == 4);
    T(sg_query_pixelformat(SG_PIXELFORMAT_RGBA8UI).bytes_per_pixel == 4);
    T(sg_query_pixelformat(SG_PIXELFORMAT_RGBA8SI).bytes_per_pixel == 4);
    T(sg_query_pixelformat(SG_PIXELFORMAT_BGRA8).bytes_per_pixel == 4);
    T(sg_query_pixelformat(SG_PIXELFORMAT_RGB10A2).bytes_per_pixel == 4);
    T(sg_query_pixelformat(SG_PIXELFORMAT_RG11B10F).bytes_per_pixel == 4);
    T(sg_query_pixelformat(SG_PIXELFORMAT_RGB9E5).bytes_per_pixel == 4);
    T(sg_query_pixelformat(SG_PIXELFORMAT_RG32UI).bytes_per_pixel == 8);
    T(sg_query_pixelformat(SG_PIXELFORMAT_RG32SI).bytes_per_pixel == 8);
    T(sg_query_pixelformat(SG_PIXELFORMAT_RG32F).bytes_per_pixel == 8);
    T(sg_query_pixelformat(SG_PIXELFORMAT_RGBA16).bytes_per_pixel == 8);
    T(sg_query_pixelformat(SG_PIXELFORMAT_RGBA16SN).bytes_per_pixel == 8);
    T(sg_query_pixelformat(SG_PIXELFORMAT_RGBA16UI).bytes_per_pixel == 8);
    T(sg_query_pixelformat(SG_PIXELFORMAT_RGBA16SI).bytes_per_pixel == 8);
    T(sg_query_pixelformat(SG_PIXELFORMAT_RGBA16F).bytes_per_pixel == 8);
    T(sg_query_pixelformat(SG_PIXELFORMAT_RGBA32UI).bytes_per_pixel == 16);
    T(sg_query_pixelformat(SG_PIXELFORMAT_RGBA32SI).bytes_per_pixel == 16);
    T(sg_query_pixelformat(SG_PIXELFORMAT_RGBA32F).bytes_per_pixel == 16);
    T(sg_query_pixelformat(SG_PIXELFORMAT_DEPTH).bytes_per_pixel == 4);
    T(sg_query_pixelformat(SG_PIXELFORMAT_DEPTH_STENCIL).bytes_per_pixel == 4);
    T(sg_query_pixelformat(SG_PIXELFORMAT_BC1_RGBA).bytes_per_pixel == 0);
    T(sg_query_pixelformat(SG_PIXELFORMAT_BC2_RGBA).bytes_per_pixel == 0);
    T(sg_query_pixelformat(SG_PIXELFORMAT_BC3_RGBA).bytes_per_pixel == 0);
    T(sg_query_pixelformat(SG_PIXELFORMAT_BC4_R).bytes_per_pixel == 0);
    T(sg_query_pixelformat(SG_PIXELFORMAT_BC4_RSN).bytes_per_pixel == 0);
    T(sg_query_pixelformat(SG_PIXELFORMAT_BC5_RG).bytes_per_pixel == 0);
    T(sg_query_pixelformat(SG_PIXELFORMAT_BC5_RGSN).bytes_per_pixel == 0);
    T(sg_query_pixelformat(SG_PIXELFORMAT_BC6H_RGBF).bytes_per_pixel == 0);
    T(sg_query_pixelformat(SG_PIXELFORMAT_BC6H_RGBUF).bytes_per_pixel == 0);
    T(sg_query_pixelformat(SG_PIXELFORMAT_BC7_RGBA).bytes_per_pixel == 0);
    T(sg_query_pixelformat(SG_PIXELFORMAT_PVRTC_RGB_2BPP).bytes_per_pixel == 0);
    T(sg_query_pixelformat(SG_PIXELFORMAT_PVRTC_RGB_4BPP).bytes_per_pixel == 0);
    T(sg_query_pixelformat(SG_PIXELFORMAT_PVRTC_RGBA_2BPP).bytes_per_pixel == 0);
    T(sg_query_pixelformat(SG_PIXELFORMAT_PVRTC_RGBA_4BPP).bytes_per_pixel == 0);
    T(sg_query_pixelformat(SG_PIXELFORMAT_ETC2_RGB8).bytes_per_pixel == 0);
    T(sg_query_pixelformat(SG_PIXELFORMAT_ETC2_RGB8A1).bytes_per_pixel == 0);
    T(sg_query_pixelformat(SG_PIXELFORMAT_ETC2_RGBA8).bytes_per_pixel == 0);
    T(sg_query_pixelformat(SG_PIXELFORMAT_ETC2_RG11).bytes_per_pixel == 0);
    T(sg_query_pixelformat(SG_PIXELFORMAT_ETC2_RG11SN).bytes_per_pixel == 0);
    sg_shutdown();
}

UTEST(sokol_gfx, query_pixelformat_compressed) {
    setup(&(sg_desc){0});
    int i = SG_PIXELFORMAT_NONE + 1;
    for (; i < SG_PIXELFORMAT_BC1_RGBA; i++) {
        T(sg_query_pixelformat((sg_pixel_format)i).compressed == false);
    }
    for (; i < _SG_PIXELFORMAT_NUM; i++) {
        T(sg_query_pixelformat((sg_pixel_format)i).compressed == true);
    }
    sg_shutdown();
}

UTEST(sokol_gfx, query_row_pitch) {
    setup(&(sg_desc){0});
    T(sg_query_row_pitch(SG_PIXELFORMAT_R8, 13, 1) == 13);
    T(sg_query_row_pitch(SG_PIXELFORMAT_R8, 13, 32) == 32);
    T(sg_query_row_pitch(SG_PIXELFORMAT_RG8SN, 256, 16) == 512);
    T(sg_query_row_pitch(SG_PIXELFORMAT_RGBA8, 256, 16) == 1024);
    T(sg_query_row_pitch(SG_PIXELFORMAT_BC1_RGBA, 1024, 1) == 2048);
    T(sg_query_row_pitch(SG_PIXELFORMAT_BC1_RGBA, 1, 1) == 8);
    T(sg_query_row_pitch(SG_PIXELFORMAT_DEPTH, 256, 4) == 1024);
    T(sg_query_row_pitch(SG_PIXELFORMAT_DEPTH_STENCIL, 256, 4) == 1024);
    sg_shutdown();
}

UTEST(sokol_gfx, sg_query_surface_pitch) {
    setup(&(sg_desc){0});
    T(sg_query_surface_pitch(SG_PIXELFORMAT_R8, 256, 256, 1) == (256 * 256));
    T(sg_query_surface_pitch(SG_PIXELFORMAT_R8, 256, 256, 1024) == (256 * 1024));
    T(sg_query_surface_pitch(SG_PIXELFORMAT_RG8, 1, 1, 1) == 2);
    T(sg_query_surface_pitch(SG_PIXELFORMAT_RG8, 256, 256, 4) == (256 * 256 * 2));
    T(sg_query_surface_pitch(SG_PIXELFORMAT_RGBA32F, 256, 256, 1) == (256 * 256 * 16));
    T(sg_query_surface_pitch(SG_PIXELFORMAT_BC1_RGBA, 256, 256, 1) == (256 * 2 * 64));
    T(sg_query_surface_pitch(SG_PIXELFORMAT_BC1_RGBA, 256, 1, 1) == (256 * 2));
    T(sg_query_surface_pitch(SG_PIXELFORMAT_BC1_RGBA, 256, 2, 1) == (256 * 2));
    T(sg_query_surface_pitch(SG_PIXELFORMAT_BC1_RGBA, 256, 3, 1) == (256 * 2));
    T(sg_query_surface_pitch(SG_PIXELFORMAT_BC1_RGBA, 256, 4, 1) == (256 * 2));
    T(sg_query_surface_pitch(SG_PIXELFORMAT_BC1_RGBA, 256, 5, 1) == (256 * 2 * 2));
    sg_shutdown();
}
