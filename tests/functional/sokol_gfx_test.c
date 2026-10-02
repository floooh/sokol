//------------------------------------------------------------------------------
//  sokol-gfx-test.c
//
//  LLM maintained.
//
//  NOTE: this is not only testing the public API behaviour, but also
//  accesses private functions and data. It may make sense to split
//  these into two separate tests.
//------------------------------------------------------------------------------
#include "force_dummy_backend.h"
#define SOKOL_IMPL
#define SOKOL_TRACE_HOOKS
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

// for checks where the exact set of reported validation errors doesn't matter
static bool log_item_called(sg_log_item item) {
    for (int i = 0; i < num_log_called; i++) {
        if (log_items[i] == item) {
            return true;
        }
    }
    return false;
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
        .usage.color_attachment = true,
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

static sg_view create_view(void) {
    return sg_make_view(&(sg_view_desc){
        .color_attachment.image = sg_make_image(&(sg_image_desc){
            .usage.color_attachment = true,
            .width = 128,
            .height = 128,
        })
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
        .view_pool_size = 64,
    });
    const sg_desc desc = sg_query_desc();
    T(desc.buffer_pool_size == 1024);
    T(desc.image_pool_size == _SG_DEFAULT_IMAGE_POOL_SIZE);
    T(desc.sampler_pool_size == 8);
    T(desc.shader_pool_size == 128);
    T(desc.pipeline_pool_size == _SG_DEFAULT_PIPELINE_POOL_SIZE);
    T(desc.view_pool_size == 64);
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
        .view_pool_size = 64,
    });
    T(sg_isvalid());
    /* pool slot 0 is reserved (this is the "invalid slot") */
    T(_sg.pools.buffer_pool.size == 1025);
    T(_sg.pools.image_pool.size == 2049);
    T(_sg.pools.shader_pool.size == 129);
    T(_sg.pools.pipeline_pool.size == 257);
    T(_sg.pools.view_pool.size == 65);
    T(_sg.pools.buffer_pool.queue_top == 1024);
    T(_sg.pools.image_pool.queue_top == 2048);
    T(_sg.pools.shader_pool.queue_top == 128);
    T(_sg.pools.pipeline_pool.queue_top == 256);
    T(_sg.pools.view_pool.queue_top == 64);
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

UTEST(sokol_gfx, alloc_fail_destroy_views) {
    setup(&(sg_desc){
        .view_pool_size = 3
    });
    T(sg_isvalid());

    sg_view views[3] = { {0} };
    for (int i = 0; i < 3; i++) {
        views[i] = sg_alloc_view();
        T(views[i].id != SG_INVALID_ID);
        T((2-i) == _sg.pools.view_pool.queue_top);
        T(sg_query_view_state(views[i]) == SG_RESOURCESTATE_ALLOC);
    }
    // the next alloc will fail because the pool is exhausted
    sg_view v3 = sg_alloc_view();
    T(v3.id == SG_INVALID_ID);
    T(sg_query_view_state(v3) == SG_RESOURCESTATE_INVALID);

    // before destroying, the resources must be either in valid or failed state
    for (int i = 0; i < 3; i++) {
        sg_fail_view(views[i]);
        T(sg_query_view_state(views[i]) == SG_RESOURCESTATE_FAILED);
    }
    for (int i = 0; i < 3; i++) {
        sg_destroy_view(views[i]);
        T(sg_query_view_state(views[i]) == SG_RESOURCESTATE_INVALID);
        T((i+1) == _sg.pools.view_pool.queue_top);
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
        const _sg_buffer_t* bufptr = _sg_lookup_buffer(buf[i].id);
        T(bufptr);
        T(bufptr->slot.id == buf[i].id);
        T(bufptr->slot.state == SG_RESOURCESTATE_VALID);
        T(bufptr->cmn.size == sizeof(data));
        T(bufptr->cmn.usage.vertex_buffer);
        T(!bufptr->cmn.usage.write_transient);
        T(!bufptr->cmn.usage.copy_dst);
        T(bufptr->cmn.bind_frame_index == 0);
        T(bufptr->cmn.copy_src_frame_index == 0);
        T(bufptr->cmn.write_transient_frame_index == 0);
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
        .data.mip_levels[0] = SG_RANGE(data)
    };
    for (int i = 0; i < 3; i++) {
        img[i] = sg_make_image(&desc);
        T(img[i].id != SG_INVALID_ID);
        T((2-i) == _sg.pools.image_pool.queue_top);
        T(sg_query_image_state(img[i]) == SG_RESOURCESTATE_VALID);
        const _sg_image_t* imgptr = _sg_lookup_image(img[i].id);
        T(imgptr);
        T(imgptr->slot.id == img[i].id);
        T(imgptr->slot.state == SG_RESOURCESTATE_VALID);
        T(imgptr->cmn.type == SG_IMAGETYPE_2D);
        T(!imgptr->cmn.usage.color_attachment);
        T(imgptr->cmn.width == 8);
        T(imgptr->cmn.height == 8);
        T(imgptr->cmn.num_slices == 1);
        T(imgptr->cmn.num_mipmaps == 1);
        T(imgptr->cmn.usage.immutable);
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
        const _sg_sampler_t* smpptr = _sg_lookup_sampler(smp[i].id);
        T(smpptr);
        T(smpptr->slot.id == smp[i].id);
        T(smpptr->slot.state == SG_RESOURCESTATE_VALID);
        T(smpptr->cmn.min_filter == SG_FILTER_NEAREST);
        T(smpptr->cmn.mag_filter == SG_FILTER_NEAREST);
        T(smpptr->cmn.mipmap_filter == SG_FILTER_NEAREST);
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
        .uniform_blocks[0] = {
            .stage = SG_SHADERSTAGE_VERTEX,
            .size = 16
        }
    };
    for (int i = 0; i < 3; i++) {
        shd[i] = sg_make_shader(&desc);
        T(shd[i].id != SG_INVALID_ID);
        T((2-i) == _sg.pools.shader_pool.queue_top);
        T(sg_query_shader_state(shd[i]) == SG_RESOURCESTATE_VALID);
        const _sg_shader_t* shdptr = _sg_lookup_shader(shd[i].id);
        T(shdptr);
        T(shdptr->slot.id == shd[i].id);
        T(shdptr->slot.state == SG_RESOURCESTATE_VALID);
        T(shdptr->cmn.uniform_blocks[0].stage == SG_SHADERSTAGE_VERTEX);
        T(shdptr->cmn.uniform_blocks[0].size == 16);
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
        const _sg_pipeline_t* pipptr = _sg_lookup_pipeline(pip[i].id);
        T(pipptr);
        T(pipptr->slot.id == pip[i].id);
        T(pipptr->slot.state == SG_RESOURCESTATE_VALID);
        T(pipptr->cmn.shader.sref.id == desc.shader.id);
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

UTEST(sokol_gfx, make_destroy_views) {
    setup(&(sg_desc){
        .view_pool_size = 3
    });
    T(sg_isvalid());

    sg_view views[3] = { {0} };

    sg_image img = sg_make_image(&(sg_image_desc){
        .usage.color_attachment = true,
        .width = 128,
        .height = 128,
    });
    const sg_view_desc view_desc = { .color_attachment.image = img };
    for (int i = 0; i < 3; i++) {
        views[i] = sg_make_view(&view_desc);
        T(views[i].id != SG_INVALID_ID);
        T((2-i) == _sg.pools.view_pool.queue_top);
        T(sg_query_view_state(views[i]) == SG_RESOURCESTATE_VALID);
        const _sg_view_t* viewptr = _sg_lookup_view(views[i].id);
        T(viewptr);
        T(viewptr->slot.id == views[i].id);
        T(viewptr->slot.state == SG_RESOURCESTATE_VALID);
    }
    // trying to create another one fails because pool is exhausted
    T(sg_make_view(&view_desc).id == SG_INVALID_ID);

    for (int i = 0; i < 3; i++) {
        sg_destroy_view(views[i]);
        T(sg_query_view_state(views[i]) == SG_RESOURCESTATE_INVALID);
        T((i+1) == _sg.pools.view_pool.queue_top);
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
    T(desc.usage.vertex_buffer);
    T(!desc.usage.write_transient);
    T(!desc.usage.copy_dst);
    desc = sg_query_buffer_defaults(&(sg_buffer_desc){ .usage.index_buffer = true });
    T(desc.usage.index_buffer);
    T(!desc.usage.vertex_buffer);
    desc = sg_query_buffer_defaults(&(sg_buffer_desc){ .usage = { .staging_buffer = true, .copy_src = true } });
    T(desc.usage.staging_buffer);
    T(desc.usage.copy_src);
    T(!desc.usage.vertex_buffer);
    desc = sg_query_buffer_defaults(&(sg_buffer_desc){ .usage.write_transient = true });
    T(desc.usage.vertex_buffer);
    T(desc.usage.write_transient);
    sg_shutdown();
}

UTEST(sokol_gfx, query_image_defaults) {
    setup(&(sg_desc){0});
    const sg_image_desc desc = sg_query_image_defaults(&(sg_image_desc){0});
    T(desc.type == SG_IMAGETYPE_2D);
    T(!desc.usage.color_attachment);
    T(!desc.usage.resolve_attachment);
    T(!desc.usage.depth_stencil_attachment);
    T(desc.usage.immutable);
    T(desc.num_mipmaps == 1);
    T(desc.pixel_format == SG_PIXELFORMAT_RGBA8);
    T(desc.sample_count == 1);
    sg_shutdown();
}

UTEST(sokol_gfx, query_sampler_defaults) {
    setup(&(sg_desc){0});
    const sg_sampler_desc desc = sg_query_sampler_defaults(&(sg_sampler_desc){0});
    T(desc.min_filter == SG_FILTER_NEAREST);
    T(desc.mag_filter == SG_FILTER_NEAREST);
    T(desc.mipmap_filter == SG_FILTER_NEAREST);
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
    T(0 == strcmp(desc.vertex_func.entry, "main"));
    T(0 == strcmp(desc.fragment_func.entry, "main"));
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

UTEST(sokol_gfx, query_view_defaults) {
    setup(&(sg_desc){0});
    const sg_view_desc desc = sg_query_view_defaults(&(sg_view_desc){0});
    T(desc.texture.image.id == SG_INVALID_ID);
    T(desc.texture.mip_levels.count == 0);
    sg_shutdown();
}

UTEST(sokol_gfx, query_buffer_info) {
    setup(&(sg_desc){0});
    sg_buffer buf = sg_make_buffer(&(sg_buffer_desc){
        .size = 256,
        .usage = {
            .vertex_buffer = true,
            .write_transient = true,
        },
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
        .usage.color_attachment = true,
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
            [0] = { .glsl_name = "pos" }
        },
        .vertex_func.source = "bla",
        .fragment_func.source = "blub"
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
                [0] = { .glsl_name = "pos" }
            },
            .vertex_func.source = "bla",
            .fragment_func.source = "blub"
        })
    });
    const sg_pipeline_info info = sg_query_pipeline_info(pip);
    T(info.slot.state == SG_RESOURCESTATE_VALID);
    T(info.slot.res_id == pip.id);
    sg_shutdown();
}

UTEST(sokol_gfx, query_view_info) {
    setup(&(sg_desc){0});
    const sg_image img = sg_make_image(&(sg_image_desc){
        .usage.color_attachment = true,
        .width = 128,
        .height = 128,
    });
    sg_view view = sg_make_view(&(sg_view_desc){
        .color_attachment.image = img,
    });
    const sg_view_info info = sg_query_view_info(view);
    T(info.slot.state == SG_RESOURCESTATE_VALID);
    T(info.slot.res_id == view.id);
    sg_shutdown();
}

UTEST(sokol_gfx, query_buffer_desc) {
    setup(&(sg_desc){0});

    sg_buffer b0 = sg_make_buffer(&(sg_buffer_desc){
        .size = 32,
        .usage.write_transient = true,
        .label = "bla",
    });
    const sg_buffer_desc b0_desc = sg_query_buffer_desc(b0);
    T(b0_desc.size == 32);
    T(b0_desc.usage.vertex_buffer);
    T(b0_desc.usage.write_transient);
    T(b0_desc.data.ptr == 0);
    T(b0_desc.data.size == 0);
    T(b0_desc.gl_buffer == 0);
    T(b0_desc.mtl_buffer == 0);
    T(b0_desc.d3d11_buffer == 0);
    T(b0_desc.wgpu_buffer == 0);
    T(sg_query_buffer_size(b0) == 32);
    T(sg_query_buffer_usage(b0).write_transient);

    float vtx_data[16];
    sg_buffer b1 = sg_make_buffer(&(sg_buffer_desc){
        .data = SG_RANGE(vtx_data)
    });
    const sg_buffer_desc b1_desc = sg_query_buffer_desc(b1);
    T(b1_desc.size == sizeof(vtx_data));
    T(b1_desc.usage.vertex_buffer);
    T(!b1_desc.usage.write_transient);
    T(!b1_desc.usage.copy_dst);
    T(b1_desc.data.ptr == 0);
    T(b1_desc.data.size == 0);
    T(sg_query_buffer_size(b1) == sizeof(vtx_data));
    T(sg_query_buffer_usage(b1).vertex_buffer);
    T(!sg_query_buffer_usage(b1).write_transient);

    uint16_t idx_data[8];
    sg_buffer b2 = sg_make_buffer(&(sg_buffer_desc){
        .usage.index_buffer = true,
        .data = SG_RANGE(idx_data),
    });
    const sg_buffer_desc b2_desc = sg_query_buffer_desc(b2);
    T(b2_desc.size == sizeof(idx_data));
    T(b2_desc.usage.index_buffer);
    T(!b2_desc.usage.write_transient);
    T(!b2_desc.usage.copy_dst);
    T(b2_desc.data.ptr == 0);
    T(b2_desc.data.size == 0);
    T(sg_query_buffer_size(b2) == sizeof(idx_data));
    T(sg_query_buffer_usage(b2).index_buffer);
    T(!sg_query_buffer_usage(b2).write_transient);

    // invalid buffer (returns zeroed desc)
    sg_buffer b3 = sg_make_buffer(&(sg_buffer_desc){
        .size = 32,
        .usage.write_transient = true,
        .label = "bla",
    });
    sg_destroy_buffer(b3);
    const sg_buffer_desc b3_desc = sg_query_buffer_desc(b3);
    T(b3_desc.size == 0);
    T(!b3_desc.usage.write_transient);
    T(sg_query_buffer_size(b3) == 0);
    T(!sg_query_buffer_usage(b3).write_transient);
    sg_shutdown();
}

UTEST(sokol_gfx, query_image_desc) {
    setup(&(sg_desc){0});

    sg_image i0 = sg_make_image(&(sg_image_desc){
        .width = 256,
        .height = 512,
        .pixel_format = SG_PIXELFORMAT_R8,
        .usage.copy_dst = true,
    });
    const sg_image_desc i0_desc = sg_query_image_desc(i0);
    T(i0_desc.type == SG_IMAGETYPE_2D);
    T(!i0_desc.usage.color_attachment);
    T(!i0_desc.usage.resolve_attachment);
    T(!i0_desc.usage.depth_stencil_attachment);
    T(i0_desc.usage.copy_dst);
    T(!i0_desc.usage.immutable);
    T(i0_desc.width == 256);
    T(i0_desc.height == 512);
    T(i0_desc.num_slices == 1);
    T(i0_desc.num_mipmaps == 1);
    T(i0_desc.pixel_format == SG_PIXELFORMAT_R8);
    T(i0_desc.sample_count == 1);
    T(i0_desc.data.mip_levels[0].ptr == 0);
    T(i0_desc.data.mip_levels[0].size == 0);
    T(i0_desc.gl_texture == 0);
    T(i0_desc.gl_texture_target == 0);
    T(i0_desc.mtl_texture == 0);
    T(i0_desc.d3d11_texture == 0);
    T(i0_desc.wgpu_texture == 0);
    T(sg_query_image_type(i0) == SG_IMAGETYPE_2D);
    T(sg_query_image_width(i0) == 256);
    T(sg_query_image_height(i0) == 512);
    T(sg_query_image_num_slices(i0) == 1);
    T(sg_query_image_num_mipmaps(i0) == 1);
    T(sg_query_image_pixelformat(i0) == SG_PIXELFORMAT_R8);
    T(sg_query_image_sample_count(i0) == 1);
    sg_destroy_image(i0);
    const sg_image_desc i0_desc_x = sg_query_image_desc(i0);
    T(i0_desc_x.type == 0);
    T(!i0_desc_x.usage.color_attachment);
    T(!i0_desc_x.usage.resolve_attachment);
    T(!i0_desc_x.usage.depth_stencil_attachment);
    T(!i0_desc_x.usage.copy_dst);
    T(i0_desc_x.width == 0);
    T(i0_desc_x.height == 0);
    T(i0_desc_x.num_slices == 0);
    T(i0_desc_x.num_mipmaps == 0);
    T(i0_desc_x.pixel_format == 0);
    T(i0_desc_x.sample_count == 0);
    T(sg_query_image_type(i0) == _SG_IMAGETYPE_DEFAULT);
    T(sg_query_image_width(i0) == 0);
    T(sg_query_image_height(i0) == 0);
    T(sg_query_image_num_slices(i0) == 0);
    T(sg_query_image_num_mipmaps(i0) == 0);
    T(sg_query_image_pixelformat(i0) == _SG_PIXELFORMAT_DEFAULT);
    T(sg_query_image_sample_count(i0) == 0);

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
            [0] = { .glsl_name = "pos", .hlsl_sem_name = "POS", .hlsl_sem_index = 1 },
        },
        .vertex_func.source = "vs_source",
        .fragment_func.source = "fs_source",
        .uniform_blocks = {
            [0] = {
                .stage = SG_SHADERSTAGE_VERTEX,
                .size = 128,
                .layout = SG_UNIFORMLAYOUT_STD140,
                .glsl_uniforms = {
                    [0] = { .glsl_name = "blub", .type = SG_UNIFORMTYPE_FLOAT4, .array_count = 1 },
                    [1] = { .glsl_name = "blob", .type = SG_UNIFORMTYPE_FLOAT2, .array_count = 1 },
                }
            }
        },
        .views[0].texture = { .stage = SG_SHADERSTAGE_VERTEX, .image_type = SG_IMAGETYPE_2D, .sample_type = SG_IMAGESAMPLETYPE_FLOAT, .multisampled = true },
        .views[1].texture = { .stage = SG_SHADERSTAGE_VERTEX, .image_type = SG_IMAGETYPE_3D, .sample_type = SG_IMAGESAMPLETYPE_DEPTH },
        .views[2].texture = { .stage = SG_SHADERSTAGE_FRAGMENT, .image_type = SG_IMAGETYPE_ARRAY, .sample_type = SG_IMAGESAMPLETYPE_DEPTH },
        .views[3].texture = { .stage = SG_SHADERSTAGE_FRAGMENT, .image_type = SG_IMAGETYPE_CUBE, .sample_type = SG_IMAGESAMPLETYPE_UNFILTERABLE_FLOAT },
        .samplers[0] = { .stage = SG_SHADERSTAGE_VERTEX, .sampler_type = SG_SAMPLERTYPE_FILTERING },
        .samplers[1] = { .stage = SG_SHADERSTAGE_VERTEX, .sampler_type = SG_SAMPLERTYPE_COMPARISON },
        .samplers[2] = { .stage = SG_SHADERSTAGE_FRAGMENT, .sampler_type = SG_SAMPLERTYPE_COMPARISON },
        .samplers[3] = { .stage = SG_SHADERSTAGE_FRAGMENT, .sampler_type = SG_SAMPLERTYPE_NONFILTERING },
        .texture_sampler_pairs[0] = { .stage = SG_SHADERSTAGE_VERTEX, .view_slot = 0, .sampler_slot = 0, .glsl_name = "img0" },
        .texture_sampler_pairs[1] = { .stage = SG_SHADERSTAGE_VERTEX, .view_slot = 1, .sampler_slot = 1, .glsl_name = "img1" },
        .texture_sampler_pairs[2] = { .stage = SG_SHADERSTAGE_FRAGMENT, .view_slot = 2, .sampler_slot = 2, .glsl_name = "img3" },
        .texture_sampler_pairs[3] = { .stage = SG_SHADERSTAGE_FRAGMENT, .view_slot = 3, .sampler_slot = 3, .glsl_name = "img4" },
        .label = "label",
    });
    const sg_shader_desc s0_desc = sg_query_shader_desc(s0);
    T(s0_desc.attrs[0].glsl_name == 0);
    T(s0_desc.attrs[0].hlsl_sem_name == 0);
    T(s0_desc.attrs[0].hlsl_sem_index == 0);
    T(s0_desc.vertex_func.source == 0);
    T(s0_desc.fragment_func.source == 0);
    T(s0_desc.uniform_blocks[0].size == 128);
    T(s0_desc.uniform_blocks[0].layout == 0);
    T(s0_desc.uniform_blocks[0].glsl_uniforms[0].glsl_name == 0);
    T(s0_desc.uniform_blocks[0].glsl_uniforms[0].type == 0);
    T(s0_desc.uniform_blocks[0].glsl_uniforms[0].array_count == 0);
    T(s0_desc.views[0].texture.stage == SG_SHADERSTAGE_VERTEX);
    T(s0_desc.views[0].texture.image_type == SG_IMAGETYPE_2D);
    T(s0_desc.views[0].texture.sample_type == SG_IMAGESAMPLETYPE_FLOAT);
    T(s0_desc.views[0].texture.multisampled);
    T(s0_desc.views[1].texture.stage == SG_SHADERSTAGE_VERTEX);
    T(s0_desc.views[1].texture.image_type == SG_IMAGETYPE_3D);
    T(s0_desc.views[1].texture.sample_type == SG_IMAGESAMPLETYPE_DEPTH);
    T(s0_desc.views[1].texture.multisampled == false);
    T(s0_desc.views[2].texture.stage == SG_SHADERSTAGE_FRAGMENT);
    T(s0_desc.views[2].texture.image_type == SG_IMAGETYPE_ARRAY);
    T(s0_desc.views[2].texture.sample_type == SG_IMAGESAMPLETYPE_DEPTH);
    T(s0_desc.views[2].texture.multisampled == false);
    T(s0_desc.views[3].texture.stage == SG_SHADERSTAGE_FRAGMENT);
    T(s0_desc.views[3].texture.image_type == SG_IMAGETYPE_CUBE);
    T(s0_desc.views[3].texture.sample_type == SG_IMAGESAMPLETYPE_UNFILTERABLE_FLOAT);
    T(s0_desc.views[3].texture.multisampled == false);
    T(s0_desc.samplers[0].stage == SG_SHADERSTAGE_VERTEX);
    T(s0_desc.samplers[0].sampler_type == SG_SAMPLERTYPE_FILTERING);
    T(s0_desc.samplers[1].stage == SG_SHADERSTAGE_VERTEX);
    T(s0_desc.samplers[1].sampler_type == SG_SAMPLERTYPE_COMPARISON);
    T(s0_desc.samplers[2].stage == SG_SHADERSTAGE_FRAGMENT);
    T(s0_desc.samplers[2].sampler_type == SG_SAMPLERTYPE_COMPARISON);
    T(s0_desc.samplers[3].stage == SG_SHADERSTAGE_FRAGMENT);
    T(s0_desc.samplers[3].sampler_type == SG_SAMPLERTYPE_NONFILTERING);
    T(s0_desc.texture_sampler_pairs[0].stage == SG_SHADERSTAGE_VERTEX);
    T(s0_desc.texture_sampler_pairs[0].view_slot == 0);
    T(s0_desc.texture_sampler_pairs[0].sampler_slot == 0);
    T(s0_desc.texture_sampler_pairs[0].glsl_name == 0);
    T(s0_desc.texture_sampler_pairs[1].stage == SG_SHADERSTAGE_VERTEX);
    T(s0_desc.texture_sampler_pairs[1].view_slot == 1);
    T(s0_desc.texture_sampler_pairs[1].sampler_slot == 1);
    T(s0_desc.texture_sampler_pairs[1].glsl_name == 0);
    T(s0_desc.texture_sampler_pairs[2].stage == SG_SHADERSTAGE_FRAGMENT);
    T(s0_desc.texture_sampler_pairs[2].view_slot == 2);
    T(s0_desc.texture_sampler_pairs[2].sampler_slot == 2);
    T(s0_desc.texture_sampler_pairs[2].glsl_name == 0);
    T(s0_desc.texture_sampler_pairs[3].stage == SG_SHADERSTAGE_FRAGMENT);
    T(s0_desc.texture_sampler_pairs[3].view_slot == 3);
    T(s0_desc.texture_sampler_pairs[3].sampler_slot == 3);
    T(s0_desc.texture_sampler_pairs[3].glsl_name == 0);

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

UTEST(sokol_gfx, query_view_desc) {
    setup(&(sg_desc){0});

    const sg_image_desc color_img_desc = {
        .usage.color_attachment = true,
        .width = 128,
        .height = 128,
    };
    const sg_image_desc depth_img_desc = {
        .usage.depth_stencil_attachment = true,
        .width = 128,
        .height = 128,
        .pixel_format = SG_PIXELFORMAT_DEPTH,
    };
    sg_image color_img = sg_make_image(&color_img_desc);
    sg_image depth_img = sg_make_image(&depth_img_desc);
    sg_view color_view = sg_make_view(&(sg_view_desc){ .color_attachment.image = color_img });
    sg_view depth_view = sg_make_view(&(sg_view_desc){ .depth_stencil_attachment.image = depth_img });

    const sg_view_desc color_view_desc = sg_query_view_desc(color_view);
    T(color_view_desc.color_attachment.image.id == color_img.id);
    T(color_view_desc.color_attachment.mip_level == 0);
    T(color_view_desc.color_attachment.slice == 0);
    const sg_view_desc depth_view_desc = sg_query_view_desc(depth_view);
    T(depth_view_desc.depth_stencil_attachment.image.id == depth_img.id);
    T(depth_view_desc.depth_stencil_attachment.mip_level == 0);
    T(depth_view_desc.depth_stencil_attachment.slice == 0);

    sg_shutdown();
}

UTEST(sokol_gfx, buffer_resource_states) {
    setup(&(sg_desc){0});
    sg_buffer buf = sg_alloc_buffer();
    T(sg_query_buffer_state(buf) == SG_RESOURCESTATE_ALLOC);
    sg_init_buffer(buf, &(sg_buffer_desc){ .usage.write_transient = true, .size = 128 });
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
    sg_init_image(img, &(sg_image_desc){ .usage.color_attachment = true, .width = 16, .height = 16 });
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

UTEST(sokol_gfx, view_resource_states) {
    setup(&(sg_desc){0});
    sg_view view = sg_alloc_view();
    T(sg_query_view_state(view) == SG_RESOURCESTATE_ALLOC);
    sg_init_view(view, &(sg_view_desc){
        .color_attachment.image = sg_make_image(&(sg_image_desc){ .usage.color_attachment = true, .width = 16, .height = 16 }),
    });
    T(sg_query_view_state(view) == SG_RESOURCESTATE_VALID);
    sg_uninit_view(view);
    T(sg_query_view_state(view) == SG_RESOURCESTATE_ALLOC);
    sg_dealloc_view(view);
    T(sg_query_view_state(view) == SG_RESOURCESTATE_INVALID);
    sg_shutdown();
}

UTEST(sokol_gfx, buffer_uninit_count) {
    setup(&(sg_desc){0});
    const sg_buffer_desc desc = { .usage.write_transient = true, .size = 128 };
    sg_buffer buf = sg_make_buffer(&desc);
    T(sg_query_buffer_info(buf).slot.uninit_count == 0);
    sg_uninit_buffer(buf);
    T(sg_query_buffer_info(buf).slot.uninit_count == 1);
    sg_init_buffer(buf, &desc);
    T(sg_query_buffer_info(buf).slot.uninit_count == 1);
    sg_uninit_buffer(buf);
    T(sg_query_buffer_info(buf).slot.uninit_count == 2);
    sg_dealloc_buffer(buf);
    T(sg_query_buffer_info(buf).slot.uninit_count == 0);
    sg_shutdown();
}

UTEST(sokol_gfx, image_uninit_count) {
    setup(&(sg_desc){0});
    const sg_image_desc desc = { .usage.color_attachment = true, .width = 128, .height = 128 };
    sg_image img = sg_make_image(&desc);
    T(sg_query_image_info(img).slot.uninit_count == 0);
    sg_uninit_image(img);
    T(sg_query_image_info(img).slot.uninit_count == 1);
    sg_init_image(img, &desc);
    T(sg_query_image_info(img).slot.uninit_count == 1);
    sg_uninit_image(img);
    T(sg_query_image_info(img).slot.uninit_count == 2);
    sg_dealloc_image(img);
    T(sg_query_image_info(img).slot.uninit_count == 0);
    sg_shutdown();
}

UTEST(sokol_gfx, shader_uninit_count) {
    setup(&(sg_desc){0});
    const sg_shader_desc desc = {0};
    sg_shader shd = sg_make_shader(&desc);
    T(sg_query_shader_info(shd).slot.uninit_count == 0);
    sg_uninit_shader(shd);
    T(sg_query_shader_info(shd).slot.uninit_count == 1);
    sg_init_shader(shd, &desc);
    T(sg_query_shader_info(shd).slot.uninit_count == 1);
    sg_uninit_shader(shd);
    T(sg_query_shader_info(shd).slot.uninit_count == 2);
    sg_dealloc_shader(shd);
    T(sg_query_shader_info(shd).slot.uninit_count == 0);
    sg_shutdown();
}

UTEST(sokol_gfx, pipeline_uninit_count) {
    setup(&(sg_desc){0});
    const sg_pipeline_desc desc = { .shader = sg_make_shader(&(sg_shader_desc){0}) };
    sg_pipeline pip = sg_make_pipeline(&desc);
    T(sg_query_pipeline_info(pip).slot.uninit_count == 0);
    sg_uninit_pipeline(pip);
    T(sg_query_pipeline_info(pip).slot.uninit_count == 1);
    sg_init_pipeline(pip, &desc);
    T(sg_query_pipeline_info(pip).slot.uninit_count == 1);
    sg_uninit_pipeline(pip);
    T(sg_query_pipeline_info(pip).slot.uninit_count == 2);
    sg_dealloc_pipeline(pip);
    T(sg_query_pipeline_info(pip).slot.uninit_count == 0);
    sg_shutdown();
}

UTEST(sokol_gfx, view_uninit_count) {
    setup(&(sg_desc){0});
    const sg_image img = sg_make_image(&(sg_image_desc){
        .width = 128,
        .height = 128,
    });
    const sg_view_desc desc = { .texture.image = img };
    sg_view view = sg_make_view(&desc);
    T(sg_query_view_info(view).slot.uninit_count == 0);
    sg_uninit_view(view);
    T(sg_query_view_info(view).slot.uninit_count == 1);
    sg_init_view(view, &desc);
    T(sg_query_view_info(view).slot.uninit_count == 1);
    sg_uninit_view(view);
    T(sg_query_view_info(view).slot.uninit_count == 2);
    sg_dealloc_view(view);
    T(sg_query_view_info(view).slot.uninit_count == 0);
    sg_shutdown();
}

static sg_buffer create_staging_buffer(size_t size) {
    return sg_make_buffer(&(sg_buffer_desc){
        .size = size,
        .usage = { .staging_buffer = true, .copy_src = true },
    });
}

static sg_buffer create_copy_dst_buffer(size_t size) {
    return sg_make_buffer(&(sg_buffer_desc){
        .size = size,
        .usage = { .vertex_buffer = true, .copy_dst = true },
    });
}

UTEST(sokol_gfx, copy_buffer_to_buffer_ok) {
    setup(&(sg_desc){0});
    sg_buffer src = create_staging_buffer(64);
    sg_buffer dst = create_copy_dst_buffer(64);
    T(sg_query_buffer_state(src) == SG_RESOURCESTATE_VALID);
    T(sg_query_buffer_state(dst) == SG_RESOURCESTATE_VALID);
    reset_log_items();
    sg_copy_buffer_to_buffer(&(sg_copy_buffer_to_buffer_desc){
        .src = { .buffer = src, .offset = 16 },
        .dst = { .buffer = dst, .offset = 32 },
        .size = 32,
    });
    T(num_log_called == 0);
    T(_sg_lookup_buffer(src.id)->cmn.copy_src_frame_index == _sg.frame_index);
    sg_shutdown();
}

UTEST(sokol_gfx, copy_buffer_to_buffer_validate_usage) {
    setup(&(sg_desc){0});
    sg_buffer src = create_buffer();
    sg_buffer dst = create_buffer();
    T(sg_query_buffer_state(src) == SG_RESOURCESTATE_VALID);
    T(sg_query_buffer_state(dst) == SG_RESOURCESTATE_VALID);
    reset_log_items();
    sg_copy_buffer_to_buffer(&(sg_copy_buffer_to_buffer_desc){
        .src.buffer = src,
        .dst.buffer = dst,
        .size = 16,
    });
    T(log_items[0] == SG_LOGITEM_VALIDATE_COPYBUFFERTOBUFFER_COPY_SRC);
    T(log_items[1] == SG_LOGITEM_VALIDATE_COPYBUFFERTOBUFFER_COPY_DST);
    T(log_items[2] == SG_LOGITEM_VALIDATION_FAILED);
    T(num_log_called == 3);
    sg_shutdown();
}

UTEST(sokol_gfx, copy_buffer_to_buffer_validate_same_buffer) {
    setup(&(sg_desc){0});
    sg_buffer buf = sg_make_buffer(&(sg_buffer_desc){
        .size = 64,
        .usage = { .vertex_buffer = true, .copy_src = true, .copy_dst = true },
    });
    reset_log_items();
    sg_copy_buffer_to_buffer(&(sg_copy_buffer_to_buffer_desc){
        .src = { .buffer = buf, .offset = 0 },
        .dst = { .buffer = buf, .offset = 32 },
        .size = 16,
    });
    T(log_items[0] == SG_LOGITEM_VALIDATE_COPYBUFFERTOBUFFER_SRC_VS_DST_BUFFER);
    T(log_items[1] == SG_LOGITEM_VALIDATION_FAILED);
    T(num_log_called == 2);
    sg_shutdown();
}

UTEST(sokol_gfx, copy_buffer_to_buffer_validate_size_and_offsets) {
    setup(&(sg_desc){0});
    sg_buffer src = create_staging_buffer(64);
    sg_buffer dst = create_copy_dst_buffer(32);
    reset_log_items();
    sg_copy_buffer_to_buffer(&(sg_copy_buffer_to_buffer_desc){
        .src = { .buffer = src, .offset = 2 },
        .dst = { .buffer = dst, .offset = 1 },
        .size = 0,
    });
    T(log_items[0] == SG_LOGITEM_VALIDATE_COPYBUFFERTOBUFFER_ZERO_SIZE);
    T(log_items[1] == SG_LOGITEM_VALIDATE_COPYBUFFERTOBUFFER_SRC_OFFSET_ALIGNMENT);
    T(log_items[2] == SG_LOGITEM_VALIDATE_COPYBUFFERTOBUFFER_DST_OFFSET_ALIGNMENT);
    T(log_items[3] == SG_LOGITEM_VALIDATION_FAILED);
    T(num_log_called == 4);
    reset_log_items();
    sg_copy_buffer_to_buffer(&(sg_copy_buffer_to_buffer_desc){
        .src = { .buffer = src, .offset = 32 },
        .dst = { .buffer = dst, .offset = 16 },
        .size = 32,
    });
    T(log_items[0] == SG_LOGITEM_VALIDATE_COPYBUFFERTOBUFFER_DST_OVERFLOW);
    T(log_items[1] == SG_LOGITEM_VALIDATION_FAILED);
    T(num_log_called == 2);
    reset_log_items();
    sg_copy_buffer_to_buffer(&(sg_copy_buffer_to_buffer_desc){
        .src = { .buffer = src, .offset = 48 },
        .dst = { .buffer = dst, .offset = 0 },
        .size = 32,
    });
    T(log_items[0] == SG_LOGITEM_VALIDATE_COPYBUFFERTOBUFFER_SRC_OVERFLOW);
    T(log_items[1] == SG_LOGITEM_VALIDATION_FAILED);
    T(num_log_called == 2);
    sg_shutdown();
}

static sg_buffer create_copy_src_buffer(sg_buffer_usage usage, size_t size) {
    usage.copy_src = true;
    return sg_make_buffer(&(sg_buffer_desc){ .size = size, .usage = usage });
}

static sg_image create_copy_dst_image(int width, int height) {
    return sg_make_image(&(sg_image_desc){
        .width = width,
        .height = height,
        .pixel_format = SG_PIXELFORMAT_RGBA8,
        .usage.copy_dst = true,
    });
}

// override backend features after resource creation to test the validation rules in isolation
static void set_copy_buffer_to_image_features(bool relaxed_buffer_type, bool relaxed_bytes_per_row, bool separate_buffer_types) {
    _sg.features.copy_buffer_to_image_relaxed_buffer_type = relaxed_buffer_type;
    _sg.features.copy_buffer_to_image_relaxed_bytes_per_row = relaxed_bytes_per_row;
    _sg.features.separate_buffer_types = separate_buffer_types;
}

UTEST(sokol_gfx, copy_buffer_to_image_ok) {
    setup(&(sg_desc){0});
    sg_buffer src = create_staging_buffer(1024);
    sg_image dst = create_copy_dst_image(16, 16);
    T(sg_query_buffer_state(src) == SG_RESOURCESTATE_VALID);
    T(sg_query_image_state(dst) == SG_RESOURCESTATE_VALID);
    // staging source must pass with all restrictive features, 16x16 RGBA8 has a 64 byte row pitch
    set_copy_buffer_to_image_features(false, false, true);
    sg_copy_buffer_to_image(&(sg_copy_buffer_to_image_desc){ .src.buffer = src, .dst.image = dst });
    T(num_log_called == 0);
    sg_shutdown();
}

UTEST(sokol_gfx, copy_buffer_to_image_src_staging_index_buffer) {
    setup(&(sg_desc){0});
    sg_buffer src = create_copy_src_buffer((sg_buffer_usage){ .staging_index_buffer = true }, 1024);
    sg_image dst = create_copy_dst_image(16, 16);
    T(sg_query_buffer_state(src) == SG_RESOURCESTATE_VALID);
    T(sg_query_image_state(dst) == SG_RESOURCESTATE_VALID);
    // rejected independent of any backend feature
    for (int i = 0; i < 2; i++) {
        const bool relaxed = (i == 1);
        set_copy_buffer_to_image_features(relaxed, relaxed, false);
        reset_log_items();
        sg_copy_buffer_to_image(&(sg_copy_buffer_to_image_desc){ .src.buffer = src, .dst.image = dst });
        T(log_items[0] == SG_LOGITEM_VALIDATE_COPYBUFFERTOIMAGE_SRC_STAGING_INDEX_BUFFER);
        if (relaxed) {
            T(log_items[1] == SG_LOGITEM_VALIDATION_FAILED);
            T(num_log_called == 2);
        } else {
            // additionally fails the staging-buffer-only and 256-byte-row checks
            T(log_items[1] == SG_LOGITEM_VALIDATE_COPYBUFFERTOIMAGE_SRC_STAGING_BUFFER);
            T(log_items[2] == SG_LOGITEM_VALIDATE_COPYBUFFERTOIMAGE_BYTESPERROW_MULTIPLE_256);
            T(log_items[3] == SG_LOGITEM_VALIDATION_FAILED);
            T(num_log_called == 4);
        }
    }
    sg_shutdown();
}

UTEST(sokol_gfx, copy_buffer_to_image_relaxed_buffer_type) {
    setup(&(sg_desc){0});
    sg_buffer vtx = create_copy_src_buffer((sg_buffer_usage){ .vertex_buffer = true }, 1024);
    sg_buffer idx = create_copy_src_buffer((sg_buffer_usage){ .index_buffer = true }, 1024);
    sg_image dst = create_copy_dst_image(16, 16);
    T(sg_query_buffer_state(vtx) == SG_RESOURCESTATE_VALID);
    T(sg_query_buffer_state(idx) == SG_RESOURCESTATE_VALID);
    T(sg_query_image_state(dst) == SG_RESOURCESTATE_VALID);

    // relaxed: any buffer type is accepted as source
    set_copy_buffer_to_image_features(true, true, false);
    sg_copy_buffer_to_image(&(sg_copy_buffer_to_image_desc){ .src.buffer = vtx, .dst.image = dst });
    sg_copy_buffer_to_image(&(sg_copy_buffer_to_image_desc){ .src.buffer = idx, .dst.image = dst });
    T(num_log_called == 0);

    // not relaxed (D3D11): only .staging_buffer is accepted
    set_copy_buffer_to_image_features(false, true, false);
    sg_copy_buffer_to_image(&(sg_copy_buffer_to_image_desc){ .src.buffer = vtx, .dst.image = dst });
    T(log_items[0] == SG_LOGITEM_VALIDATE_COPYBUFFERTOIMAGE_SRC_STAGING_BUFFER);
    T(log_items[1] == SG_LOGITEM_VALIDATION_FAILED);
    T(num_log_called == 2);
    reset_log_items();
    sg_copy_buffer_to_image(&(sg_copy_buffer_to_image_desc){ .src.buffer = idx, .dst.image = dst });
    T(log_items[0] == SG_LOGITEM_VALIDATE_COPYBUFFERTOIMAGE_SRC_STAGING_BUFFER);
    T(log_items[1] == SG_LOGITEM_VALIDATION_FAILED);
    T(num_log_called == 2);
    sg_shutdown();
}

UTEST(sokol_gfx, copy_buffer_to_image_relaxed_bytes_per_row) {
    setup(&(sg_desc){0});
    sg_buffer vtx = create_copy_src_buffer((sg_buffer_usage){ .vertex_buffer = true }, 8192);
    sg_buffer stg = create_staging_buffer(4096);
    sg_image small = create_copy_dst_image(16, 16);     // 64 byte row pitch
    sg_image wide = create_copy_dst_image(64, 16);      // 256 byte row pitch
    T(sg_query_buffer_state(vtx) == SG_RESOURCESTATE_VALID);
    T(sg_query_buffer_state(stg) == SG_RESOURCESTATE_VALID);
    T(sg_query_image_state(small) == SG_RESOURCESTATE_VALID);
    T(sg_query_image_state(wide) == SG_RESOURCESTATE_VALID);

    // relaxed: any row pitch is fine
    set_copy_buffer_to_image_features(true, true, false);
    sg_copy_buffer_to_image(&(sg_copy_buffer_to_image_desc){ .src.buffer = vtx, .dst.image = small });
    T(num_log_called == 0);

    // not relaxed (WebGPU): non-staging source needs a 256 byte multiple row pitch
    set_copy_buffer_to_image_features(true, false, false);
    sg_copy_buffer_to_image(&(sg_copy_buffer_to_image_desc){ .src.buffer = vtx, .dst.image = small });
    T(log_items[0] == SG_LOGITEM_VALIDATE_COPYBUFFERTOIMAGE_BYTESPERROW_MULTIPLE_256);
    T(log_items[1] == SG_LOGITEM_VALIDATION_FAILED);
    T(num_log_called == 2);
    reset_log_items();
    // explicit bytes_per_row which is a multiple of the pixel size but not of 256
    sg_copy_buffer_to_image(&(sg_copy_buffer_to_image_desc){
        .src = { .buffer = vtx, .bytes_per_row = 320, .bytes_per_slice = 320 * 16 },
        .dst.image = wide,
    });
    T(log_items[0] == SG_LOGITEM_VALIDATE_COPYBUFFERTOIMAGE_BYTESPERROW_MULTIPLE_256);
    T(log_items[1] == SG_LOGITEM_VALIDATION_FAILED);
    T(num_log_called == 2);
    reset_log_items();
    // 256 byte multiple row pitch passes
    sg_copy_buffer_to_image(&(sg_copy_buffer_to_image_desc){ .src.buffer = vtx, .dst.image = wide });
    T(num_log_called == 0);
    // staging source is exempt from the 256 byte rule
    sg_copy_buffer_to_image(&(sg_copy_buffer_to_image_desc){ .src.buffer = stg, .dst.image = small });
    T(num_log_called == 0);
    sg_shutdown();
}

UTEST(sokol_gfx, copy_buffer_to_image_webgl2_index_buffer) {
    setup(&(sg_desc){0});
    sg_buffer idx = create_copy_src_buffer((sg_buffer_usage){ .index_buffer = true }, 1024);
    sg_buffer vtx = create_copy_src_buffer((sg_buffer_usage){ .vertex_buffer = true }, 1024);
    sg_buffer stg = create_staging_buffer(1024);
    sg_image dst = create_copy_dst_image(16, 16);
    T(sg_query_buffer_state(idx) == SG_RESOURCESTATE_VALID);
    T(sg_query_buffer_state(vtx) == SG_RESOURCESTATE_VALID);
    T(sg_query_buffer_state(stg) == SG_RESOURCESTATE_VALID);
    T(sg_query_image_state(dst) == SG_RESOURCESTATE_VALID);

    // separate buffer types (WebGL2): index buffers can't be a source
    set_copy_buffer_to_image_features(true, true, true);
    sg_copy_buffer_to_image(&(sg_copy_buffer_to_image_desc){ .src.buffer = idx, .dst.image = dst });
    T(log_items[0] == SG_LOGITEM_VALIDATE_COPYBUFFERTOIMAGE_WEBGL2_INDEX_BUFFER);
    T(log_items[1] == SG_LOGITEM_VALIDATION_FAILED);
    T(num_log_called == 2);
    reset_log_items();
    // ...but vertex and staging buffers are fine
    sg_copy_buffer_to_image(&(sg_copy_buffer_to_image_desc){ .src.buffer = vtx, .dst.image = dst });
    sg_copy_buffer_to_image(&(sg_copy_buffer_to_image_desc){ .src.buffer = stg, .dst.image = dst });
    T(num_log_called == 0);

    // without separate buffer types an index buffer source is fine
    set_copy_buffer_to_image_features(true, true, false);
    sg_copy_buffer_to_image(&(sg_copy_buffer_to_image_desc){ .src.buffer = idx, .dst.image = dst });
    T(num_log_called == 0);
    sg_shutdown();
}

UTEST(sokol_gfx, copy_buffer_to_image_src_overflow) {
    setup(&(sg_desc){0});
    sg_buffer full_ok = create_staging_buffer(1024);
    sg_buffer full_small = create_staging_buffer(1020);
    sg_buffer sub_ok = create_staging_buffer(144);
    sg_buffer sub_small = create_staging_buffer(140);
    sg_image dst = create_copy_dst_image(16, 16);   // 64 byte row pitch, 1024 byte slice pitch
    T(sg_query_buffer_state(full_ok) == SG_RESOURCESTATE_VALID);
    T(sg_query_buffer_state(full_small) == SG_RESOURCESTATE_VALID);
    T(sg_query_buffer_state(sub_ok) == SG_RESOURCESTATE_VALID);
    T(sg_query_buffer_state(sub_small) == SG_RESOURCESTATE_VALID);
    T(sg_query_image_state(dst) == SG_RESOURCESTATE_VALID);

    // whole image with default pitches requires exactly 1024 bytes
    sg_copy_buffer_to_image(&(sg_copy_buffer_to_image_desc){ .src.buffer = full_ok, .dst.image = dst });
    T(num_log_called == 0);
    sg_copy_buffer_to_image(&(sg_copy_buffer_to_image_desc){ .src.buffer = full_small, .dst.image = dst });
    T(log_items[0] == SG_LOGITEM_VALIDATE_COPYBUFFERTOIMAGE_SRC_OVERFLOW);
    T(log_items[1] == SG_LOGITEM_VALIDATION_FAILED);
    T(num_log_called == 2);
    reset_log_items();

    // 4x2 sub-region with a one-row source offset: only the bytes actually
    // read are required, e.g. no padding after the last row or slice:
    //  64 (offset) + 64 (first row incl. padding) + 16 (last row) = 144
    const sg_copy_buffer_to_image_desc sub_desc = {
        .src = { .offset = 64, .bytes_per_row = 64, .bytes_per_slice = 128 },
        .dst = { .image = dst, .x = 2, .y = 4 },
        .size = { .width = 4, .height = 2 },
    };
    sg_copy_buffer_to_image_desc desc = sub_desc;
    desc.src.buffer = sub_ok;
    sg_copy_buffer_to_image(&desc);
    T(num_log_called == 0);
    desc.src.buffer = sub_small;
    sg_copy_buffer_to_image(&desc);
    T(log_items[0] == SG_LOGITEM_VALIDATE_COPYBUFFERTOIMAGE_SRC_OVERFLOW);
    T(log_items[1] == SG_LOGITEM_VALIDATION_FAILED);
    T(num_log_called == 2);
    reset_log_items();
    // a bigger offset pushes the same copy over the end
    desc.src.buffer = sub_ok;
    desc.src.offset = 68;
    sg_copy_buffer_to_image(&desc);
    T(log_items[0] == SG_LOGITEM_VALIDATE_COPYBUFFERTOIMAGE_SRC_OVERFLOW);
    T(log_items[1] == SG_LOGITEM_VALIDATION_FAILED);
    T(num_log_called == 2);
    sg_shutdown();
}

UTEST(sokol_gfx, copy_buffer_to_image_src_overflow_array) {
    setup(&(sg_desc){0});
    sg_buffer src_ok = create_staging_buffer(1104);
    sg_buffer src_small = create_staging_buffer(1100);
    sg_image dst = sg_make_image(&(sg_image_desc){
        .type = SG_IMAGETYPE_ARRAY,
        .width = 16,
        .height = 16,
        .num_slices = 3,
        .pixel_format = SG_PIXELFORMAT_RGBA8,
        .usage.copy_dst = true,
    });
    T(sg_query_buffer_state(src_ok) == SG_RESOURCESTATE_VALID);
    T(sg_query_buffer_state(src_small) == SG_RESOURCESTATE_VALID);
    T(sg_query_image_state(dst) == SG_RESOURCESTATE_VALID);

    // 4x2 region into slices 1 and 2 with default pitches (64 and 1024 bytes):
    //  1024 (first slice incl. padding) + 64 (first row of last slice) + 16 (last row) = 1104
    sg_copy_buffer_to_image(&(sg_copy_buffer_to_image_desc){
        .src.buffer = src_ok,
        .dst = { .image = dst, .slice = 1 },
        .size = { .width = 4, .height = 2, .num_slices = 2 },
    });
    T(num_log_called == 0);
    sg_copy_buffer_to_image(&(sg_copy_buffer_to_image_desc){
        .src.buffer = src_small,
        .dst = { .image = dst, .slice = 1 },
        .size = { .width = 4, .height = 2, .num_slices = 2 },
    });
    T(log_items[0] == SG_LOGITEM_VALIDATE_COPYBUFFERTOIMAGE_SRC_OVERFLOW);
    T(log_items[1] == SG_LOGITEM_VALIDATION_FAILED);
    T(num_log_called == 2);
    sg_shutdown();
}

UTEST(sokol_gfx, copy_buffer_to_image_bytes_per_row_too_small) {
    setup(&(sg_desc){0});
    sg_buffer src = create_staging_buffer(1024);
    sg_image dst = create_copy_dst_image(16, 16);
    T(sg_query_buffer_state(src) == SG_RESOURCESTATE_VALID);
    T(sg_query_image_state(dst) == SG_RESOURCESTATE_VALID);

    // 16 pixel wide copy needs at least 64 bytes per row
    sg_copy_buffer_to_image(&(sg_copy_buffer_to_image_desc){
        .src = { .buffer = src, .bytes_per_row = 32, .bytes_per_slice = 32 * 16 },
        .dst.image = dst,
        .size = { .width = 16, .height = 16 },
    });
    T(log_items[0] == SG_LOGITEM_VALIDATE_COPYBUFFERTOIMAGE_BYTESPERROW_TOO_SMALL);
    T(log_items[1] == SG_LOGITEM_VALIDATION_FAILED);
    T(num_log_called == 2);
    reset_log_items();
    // ...but a 32 byte row pitch is enough for an 8 pixel wide copy
    sg_copy_buffer_to_image(&(sg_copy_buffer_to_image_desc){
        .src = { .buffer = src, .bytes_per_row = 32, .bytes_per_slice = 32 * 16 },
        .dst.image = dst,
        .size = { .width = 8, .height = 16 },
    });
    T(num_log_called == 0);
    sg_shutdown();
}

UTEST(sokol_gfx, copy_buffer_to_image_bytes_per_slice_too_small) {
    setup(&(sg_desc){0});
    sg_buffer src = create_staging_buffer(1024);
    sg_image dst = create_copy_dst_image(16, 16);
    T(sg_query_buffer_state(src) == SG_RESOURCESTATE_VALID);
    T(sg_query_image_state(dst) == SG_RESOURCESTATE_VALID);

    // 16 rows with 64 bytes per row need at least 1024 bytes per slice
    sg_copy_buffer_to_image(&(sg_copy_buffer_to_image_desc){
        .src = { .buffer = src, .bytes_per_row = 64, .bytes_per_slice = 64 * 8 },
        .dst.image = dst,
        .size = { .width = 16, .height = 16 },
    });
    T(log_items[0] == SG_LOGITEM_VALIDATE_COPYBUFFERTOIMAGE_BYTESPERSLICE_TOO_SMALL);
    T(log_items[1] == SG_LOGITEM_VALIDATION_FAILED);
    T(num_log_called == 2);
    reset_log_items();
    // ...but 512 bytes per slice are enough for an 8 row copy
    sg_copy_buffer_to_image(&(sg_copy_buffer_to_image_desc){
        .src = { .buffer = src, .bytes_per_row = 64, .bytes_per_slice = 64 * 8 },
        .dst.image = dst,
        .size = { .width = 16, .height = 8 },
    });
    T(num_log_called == 0);
    sg_shutdown();
}

UTEST(sokol_gfx, copy_buffer_to_image_compressed_sizes) {
    setup(&(sg_desc){0});
    // 16x16 BC1: 4x4 blocks of 8 bytes, 32 byte row pitch, 128 byte slice pitch
    sg_buffer src_ok = create_staging_buffer(48);
    sg_buffer src_small = create_staging_buffer(40);
    sg_image dst = sg_make_image(&(sg_image_desc){
        .width = 16,
        .height = 16,
        .pixel_format = SG_PIXELFORMAT_BC1_RGBA,
        .usage.copy_dst = true,
    });
    T(sg_query_buffer_state(src_ok) == SG_RESOURCESTATE_VALID);
    T(sg_query_buffer_state(src_small) == SG_RESOURCESTATE_VALID);
    T(sg_query_image_state(dst) == SG_RESOURCESTATE_VALID);

    // 8x8 pixel region is 2x2 blocks: 32 (first block-row incl. padding) + 16 (last block-row) = 48
    sg_copy_buffer_to_image_desc desc = {
        .src = { .buffer = src_ok, .bytes_per_row = 32, .bytes_per_slice = 128 },
        .dst = { .image = dst, .x = 4, .y = 4 },
        .size = { .width = 8, .height = 8 },
    };
    sg_copy_buffer_to_image(&desc);
    T(num_log_called == 0);
    desc.src.buffer = src_small;
    sg_copy_buffer_to_image(&desc);
    T(log_items[0] == SG_LOGITEM_VALIDATE_COPYBUFFERTOIMAGE_SRC_OVERFLOW);
    T(log_items[1] == SG_LOGITEM_VALIDATION_FAILED);
    T(num_log_called == 2);
    reset_log_items();

    // one block (8 bytes) per row is too small for a 2 block wide copy
    desc.src.buffer = src_ok;
    desc.src.bytes_per_row = 8;
    sg_copy_buffer_to_image(&desc);
    T(log_items[0] == SG_LOGITEM_VALIDATE_COPYBUFFERTOIMAGE_BYTESPERROW_TOO_SMALL);
    T(log_items[1] == SG_LOGITEM_VALIDATION_FAILED);
    T(num_log_called == 2);
    reset_log_items();

    // a slice pitch of one block-row is too small for a 2 block-row copy
    desc.src.bytes_per_row = 32;
    desc.src.bytes_per_slice = 32;
    sg_copy_buffer_to_image(&desc);
    T(log_items[0] == SG_LOGITEM_VALIDATE_COPYBUFFERTOIMAGE_BYTESPERSLICE_TOO_SMALL);
    T(log_items[1] == SG_LOGITEM_VALIDATION_FAILED);
    T(num_log_called == 2);
    reset_log_items();

    // copy size must be a multiple of the block dimensions
    desc.src.bytes_per_slice = 128;
    desc.size.width = 6;
    sg_copy_buffer_to_image(&desc);
    T(log_items[0] == SG_LOGITEM_VALIDATE_COPYBUFFERTOIMAGE_DST_WIDTH_MULTIPLE);
    T(log_items[1] == SG_LOGITEM_VALIDATION_FAILED);
    T(num_log_called == 2);
    reset_log_items();
    desc.size.width = 8;
    desc.size.height = 6;
    sg_copy_buffer_to_image(&desc);
    T(log_items[0] == SG_LOGITEM_VALIDATE_COPYBUFFERTOIMAGE_DST_HEIGHT_MULTIPLE);
    T(log_items[1] == SG_LOGITEM_VALIDATION_FAILED);
    T(num_log_called == 2);
    reset_log_items();

    // source offset must be a multiple of the block size (single block copy)
    desc.size.width = 4;
    desc.size.height = 4;
    desc.src.offset = 4;
    sg_copy_buffer_to_image(&desc);
    T(log_items[0] == SG_LOGITEM_VALIDATE_COPYBUFFERTOIMAGE_SRC_OFFSET_ALIGNMENT);
    T(log_items[1] == SG_LOGITEM_VALIDATION_FAILED);
    T(num_log_called == 2);
    reset_log_items();
    desc.src.offset = 8;
    sg_copy_buffer_to_image(&desc);
    T(num_log_called == 0);
    sg_shutdown();
}

UTEST(sokol_gfx, copy_buffer_to_image_dst_range_with_default_size) {
    setup(&(sg_desc){0});
    sg_buffer src = create_staging_buffer(1024);
    sg_image dst = create_copy_dst_image(16, 16);
    T(sg_query_buffer_state(src) == SG_RESOURCESTATE_VALID);
    T(sg_query_image_state(dst) == SG_RESOURCESTATE_VALID);

    // a destination offset at the edge of the miplevel defaults the copy size to zero,
    // this must only trigger the size and range checks (and not any of the source size checks)
    sg_copy_buffer_to_image(&(sg_copy_buffer_to_image_desc){ .src.buffer = src, .dst = { .image = dst, .x = 16 } });
    T(log_items[0] == SG_LOGITEM_VALIDATE_COPYBUFFERTOIMAGE_DST_WIDTH);
    T(log_items[1] == SG_LOGITEM_VALIDATE_COPYBUFFERTOIMAGE_DST_X_RANGE);
    T(log_items[2] == SG_LOGITEM_VALIDATION_FAILED);
    T(num_log_called == 3);
    reset_log_items();
    sg_copy_buffer_to_image(&(sg_copy_buffer_to_image_desc){ .src.buffer = src, .dst = { .image = dst, .y = 16 } });
    T(log_items[0] == SG_LOGITEM_VALIDATE_COPYBUFFERTOIMAGE_DST_HEIGHT);
    T(log_items[1] == SG_LOGITEM_VALIDATE_COPYBUFFERTOIMAGE_DST_Y_RANGE);
    T(log_items[2] == SG_LOGITEM_VALIDATION_FAILED);
    T(num_log_called == 3);
    reset_log_items();
    sg_copy_buffer_to_image(&(sg_copy_buffer_to_image_desc){ .src.buffer = src, .dst = { .image = dst, .slice = 1 } });
    T(log_items[0] == SG_LOGITEM_VALIDATE_COPYBUFFERTOIMAGE_DST_NUMSLICES);
    T(log_items[1] == SG_LOGITEM_VALIDATE_COPYBUFFERTOIMAGE_DST_SLICE_RANGE);
    T(log_items[2] == SG_LOGITEM_VALIDATION_FAILED);
    T(num_log_called == 3);
    reset_log_items();

    // a destination offset beyond the edge defaults to a negative copy size (must not assert)
    sg_copy_buffer_to_image(&(sg_copy_buffer_to_image_desc){ .src.buffer = src, .dst = { .image = dst, .x = 20 } });
    T(log_item_called(SG_LOGITEM_VALIDATE_COPYBUFFERTOIMAGE_DST_WIDTH));
    T(log_item_called(SG_LOGITEM_VALIDATE_COPYBUFFERTOIMAGE_DST_X_RANGE));
    T(log_items[num_log_called - 1] == SG_LOGITEM_VALIDATION_FAILED);
    reset_log_items();
    sg_copy_buffer_to_image(&(sg_copy_buffer_to_image_desc){ .src.buffer = src, .dst = { .image = dst, .y = 20 } });
    T(log_item_called(SG_LOGITEM_VALIDATE_COPYBUFFERTOIMAGE_DST_HEIGHT));
    T(log_item_called(SG_LOGITEM_VALIDATE_COPYBUFFERTOIMAGE_DST_Y_RANGE));
    T(log_items[num_log_called - 1] == SG_LOGITEM_VALIDATION_FAILED);
    reset_log_items();
    sg_copy_buffer_to_image(&(sg_copy_buffer_to_image_desc){ .src.buffer = src, .dst = { .image = dst, .slice = 2 } });
    T(log_item_called(SG_LOGITEM_VALIDATE_COPYBUFFERTOIMAGE_DST_NUMSLICES));
    T(log_item_called(SG_LOGITEM_VALIDATE_COPYBUFFERTOIMAGE_DST_SLICE_RANGE));
    T(log_items[num_log_called - 1] == SG_LOGITEM_VALIDATION_FAILED);
    reset_log_items();

    // the default copy size covers the rest of the miplevel and the source is sized for that
    sg_copy_buffer_to_image(&(sg_copy_buffer_to_image_desc){ .src.buffer = src, .dst = { .image = dst, .x = 15, .y = 15 } });
    T(num_log_called == 0);
    sg_shutdown();
}

UTEST(sokol_gfx, copy_buffer_to_image_stats) {
    setup(&(sg_desc){0});
    sg_buffer src = create_staging_buffer(1024);
    sg_buffer src_small = create_staging_buffer(1020);
    sg_image dst = create_copy_dst_image(16, 16);
    T(sg_query_buffer_state(src) == SG_RESOURCESTATE_VALID);
    T(sg_query_buffer_state(src_small) == SG_RESOURCESTATE_VALID);
    T(sg_query_image_state(dst) == SG_RESOURCESTATE_VALID);
    sg_enable_stats();

    // whole image with default pitches
    sg_copy_buffer_to_image(&(sg_copy_buffer_to_image_desc){ .src.buffer = src, .dst.image = dst });
    T(num_log_called == 0);
    sg_stats stats = sg_query_stats();
    T(stats.cur_frame.num_copy_buffer_to_image == 1);
    T(stats.cur_frame.size_copy_buffer_to_image == 1024);

    // 4x2 sub-region only counts the bytes actually read: 64 (first row incl. padding) + 16 (last row)
    sg_copy_buffer_to_image(&(sg_copy_buffer_to_image_desc){
        .src = { .buffer = src, .bytes_per_row = 64, .bytes_per_slice = 128 },
        .dst = { .image = dst, .x = 2, .y = 4 },
        .size = { .width = 4, .height = 2 },
    });
    T(num_log_called == 0);
    stats = sg_query_stats();
    T(stats.cur_frame.num_copy_buffer_to_image == 2);
    T(stats.cur_frame.size_copy_buffer_to_image == 1104);

    // a copy rejected by validation isn't counted
    sg_copy_buffer_to_image(&(sg_copy_buffer_to_image_desc){ .src.buffer = src_small, .dst.image = dst });
    T(log_items[0] == SG_LOGITEM_VALIDATE_COPYBUFFERTOIMAGE_SRC_OVERFLOW);
    T(log_items[1] == SG_LOGITEM_VALIDATION_FAILED);
    T(num_log_called == 2);
    stats = sg_query_stats();
    T(stats.cur_frame.num_copy_buffer_to_image == 2);
    T(stats.cur_frame.size_copy_buffer_to_image == 1104);
    sg_shutdown();
}

UTEST(sokol_gfx, copy_buffer_to_image_negative_bytes_per_row) {
    setup(&(sg_desc){0});
    sg_buffer src = create_staging_buffer(1024);
    sg_image dst = create_copy_dst_image(16, 16);
    T(sg_query_buffer_state(src) == SG_RESOURCESTATE_VALID);
    T(sg_query_image_state(dst) == SG_RESOURCESTATE_VALID);
    // must be rejected by validation, and not assert in the bytes-per-slice multiple check
    sg_copy_buffer_to_image(&(sg_copy_buffer_to_image_desc){
        .src = { .buffer = src, .bytes_per_row = -64, .bytes_per_slice = 1024 },
        .dst.image = dst,
    });
    T(log_item_called(SG_LOGITEM_VALIDATE_COPYBUFFERTOIMAGE_BYTESPERROW_MULTIPLE_BLOCKSIZE));
    T(log_item_called(SG_LOGITEM_VALIDATE_COPYBUFFERTOIMAGE_BYTESPERSLICE));
    T(log_items[num_log_called - 1] == SG_LOGITEM_VALIDATION_FAILED);
    sg_shutdown();
}

UTEST(sokol_gfx, copy_buffer_to_buffer_stats) {
    setup(&(sg_desc){0});
    sg_buffer src = create_staging_buffer(64);
    sg_buffer dst = create_copy_dst_buffer(64);
    T(sg_query_buffer_state(src) == SG_RESOURCESTATE_VALID);
    T(sg_query_buffer_state(dst) == SG_RESOURCESTATE_VALID);
    sg_enable_stats();
    sg_copy_buffer_to_buffer(&(sg_copy_buffer_to_buffer_desc){ .src.buffer = src, .dst.buffer = dst, .size = 32 });
    T(num_log_called == 0);
    sg_stats stats = sg_query_stats();
    T(stats.cur_frame.num_copy_buffer_to_buffer == 1);
    T(stats.cur_frame.size_copy_buffer_to_buffer == 32);
    // a copy rejected by validation isn't counted
    sg_copy_buffer_to_buffer(&(sg_copy_buffer_to_buffer_desc){ .src = { .buffer = src, .offset = 48 }, .dst.buffer = dst, .size = 32 });
    T(log_items[0] == SG_LOGITEM_VALIDATE_COPYBUFFERTOBUFFER_SRC_OVERFLOW);
    T(log_items[1] == SG_LOGITEM_VALIDATION_FAILED);
    T(num_log_called == 2);
    stats = sg_query_stats();
    T(stats.cur_frame.num_copy_buffer_to_buffer == 1);
    T(stats.cur_frame.size_copy_buffer_to_buffer == 32);
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

UTEST(sokol_gfx, commit_listener_remove_non_existent) {
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

UTEST(sokoL_gfx, view_double_destroy_is_ok) {
    setup(&(sg_desc){0});
    sg_view view = create_view();
    T(sg_query_view_state(view) == SG_RESOURCESTATE_VALID);
    sg_destroy_view(view);
    T(sg_query_view_state(view) == SG_RESOURCESTATE_INVALID);
    sg_destroy_view(view);
    T(sg_query_view_state(view) == SG_RESOURCESTATE_INVALID);
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

UTEST(sokol_gfx, make_dealloc_view_warns) {
    setup(&(sg_desc){0});
    sg_view view = create_view();
    T(sg_query_view_state(view) == SG_RESOURCESTATE_VALID);
    sg_dealloc_view(view);
    T(log_items[0] == SG_LOGITEM_DEALLOC_VIEW_INVALID_STATE);
    T(num_log_called == 1);
    T(sg_query_view_state(view) == SG_RESOURCESTATE_VALID);
    sg_destroy_view(view);
    T(sg_query_view_state(view) == SG_RESOURCESTATE_INVALID);
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

UTEST(sokol_gfx, alloc_destroy_view_is_ok) {
    setup(&(sg_desc){0});
    sg_view view = sg_alloc_view();
    T(sg_query_view_state(view) == SG_RESOURCESTATE_ALLOC);
    sg_destroy_view(view);
    T(num_log_called == 0);
    T(sg_query_view_state(view) == SG_RESOURCESTATE_INVALID);
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

UTEST(sokol_gfx, make_view_with_nonvalid_image) {
    setup(&(sg_desc){
        .disable_validation = true,
    });
    sg_view view = sg_make_view(&(sg_view_desc){
        .texture.image = sg_alloc_image(),
    });
    T(sg_query_view_state(view) == SG_RESOURCESTATE_FAILED);
    sg_destroy_view(view);
    T(sg_query_view_state(view) == SG_RESOURCESTATE_INVALID);
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
    T(log_items[0] == SG_LOGITEM_VALIDATE_BUFFERDESC_EXPECT_NONZERO_SIZE);
    T(log_items[1] == SG_LOGITEM_VALIDATION_FAILED);
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
    T(log_items[0] == SG_LOGITEM_VALIDATE_BUFFERDESC_EXPECT_MATCHING_DATA_SIZE);
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
    T(log_items[0] == SG_LOGITEM_VALIDATE_BUFFERDESC_EXPECT_NONZERO_SIZE);
    T(log_items[1] == SG_LOGITEM_VALIDATION_FAILED);
    sg_shutdown();
}

UTEST(sokol_gfx, make_buffer_validate_data_ptr_but_no_data_size) {
    setup(&(sg_desc){0});
    const uint32_t data[16] = {0};
    sg_buffer buf = sg_make_buffer(&(sg_buffer_desc){
        .size = sizeof(data),
        .data.ptr = data,
    });
    T(sg_query_buffer_state(buf) == SG_RESOURCESTATE_FAILED);
    T(log_items[0] == SG_LOGITEM_VALIDATE_BUFFERDESC_EXPECT_MATCHING_DATA_SIZE);
    T(log_items[1] == SG_LOGITEM_VALIDATION_FAILED);
    sg_shutdown();
}

UTEST(sokol_gfx, make_buffer_validate_no_data_ptr_but_data_size) {
    setup(&(sg_desc){0});
    const uint32_t data[16] = {0};
    sg_buffer buf = sg_make_buffer(&(sg_buffer_desc){
        .size = sizeof(data),
        .data.size = sizeof(data),
    });
    T(sg_query_buffer_state(buf) == SG_RESOURCESTATE_FAILED);
    T(log_items[0] == SG_LOGITEM_VALIDATE_BUFFERDESC_EXPECT_ZERO_DATA_SIZE);
    T(log_items[1] == SG_LOGITEM_VALIDATION_FAILED);
    sg_shutdown();
}

UTEST(sokol_gfx, make_buffer_usage_write_transient_expect_no_data) {
    setup(&(sg_desc){0});
    const uint32_t data[16] = {0};
    sg_buffer buf = sg_make_buffer(&(sg_buffer_desc){
        .usage.write_transient = true,
        .data = SG_RANGE(data),
    });
    T(sg_query_buffer_state(buf) == SG_RESOURCESTATE_FAILED);
    T(log_items[0] == SG_LOGITEM_VALIDATE_BUFFERDESC_WRITETRANSIENT_VS_INITIALDATA);
    T(log_items[1] == SG_LOGITEM_VALIDATION_FAILED);
    sg_shutdown();
}

UTEST(sokol_gfx, make_buffer_usage_write_unsealed_expect_no_data) {
    setup(&(sg_desc){0});
    const uint32_t data[16] = {0};
    sg_buffer buf = sg_make_buffer(&(sg_buffer_desc){
        .usage.write_unsealed = true,
        .data = SG_RANGE(data),
    });
    T(sg_query_buffer_state(buf) == SG_RESOURCESTATE_FAILED);
    T(log_items[0] == SG_LOGITEM_VALIDATE_BUFFERDESC_WRITEUNSEALED_VS_INITIALDATA);
    T(log_items[1] == SG_LOGITEM_VALIDATION_FAILED);
    sg_shutdown();
}

UTEST(sokol_gfx, make_buffer_usage_copy_dst_expect_no_data) {
    setup(&(sg_desc){0});
    const uint32_t data[16] = {0};
    sg_buffer buf = sg_make_buffer(&(sg_buffer_desc){
        .usage.copy_dst = true,
        .data = SG_RANGE(data),
    });
    T(sg_query_buffer_state(buf) == SG_RESOURCESTATE_FAILED);
    T(log_items[0] == SG_LOGITEM_VALIDATE_BUFFERDESC_COPYDST_VS_INITIALDATA);
    T(log_items[1] == SG_LOGITEM_VALIDATION_FAILED);
    sg_shutdown();
}

UTEST(sokol_gfx, make_buffer_usage_staging_validation) {
    setup(&(sg_desc){0});
    const uint32_t data[16] = {0};
    sg_buffer buf = sg_make_buffer(&(sg_buffer_desc){
        .usage = { .staging_buffer = true, .vertex_buffer = true, .copy_dst = true },
        .data = SG_RANGE(data),
    });
    T(sg_query_buffer_state(buf) == SG_RESOURCESTATE_FAILED);
    T(log_items[0] == SG_LOGITEM_VALIDATE_BUFFERDESC_STAGING_VS_VERTEXBUFFER);
    T(log_items[1] == SG_LOGITEM_VALIDATE_BUFFERDESC_STAGING_VS_COPYDST);
    T(log_items[2] == SG_LOGITEM_VALIDATE_BUFFERDESC_STAGING_COPYSRC);
    T(log_items[3] == SG_LOGITEM_VALIDATE_BUFFERDESC_STAGING_VS_INITIALDATA);
    T(log_items[4] == SG_LOGITEM_VALIDATE_BUFFERDESC_COPYDST_VS_INITIALDATA);
    T(log_items[5] == SG_LOGITEM_VALIDATION_FAILED);
    sg_shutdown();
}

UTEST(sokol_gfx, make_buffer_storagebuffer_not_supported_and_size) {
    setup(&(sg_desc){0});
    const uint8_t data[10] = {0};
    sg_buffer buf = sg_make_buffer(&(sg_buffer_desc){
        .usage.storage_buffer = true,
        .data = SG_RANGE(data),
    });
    T(sg_query_buffer_state(buf) == SG_RESOURCESTATE_FAILED);
    T(log_items[0] == SG_LOGITEM_VALIDATE_BUFFERDESC_STORAGEBUFFER_SUPPORTED);
    T(log_items[1] == SG_LOGITEM_VALIDATE_BUFFERDESC_STORAGEBUFFER_SIZE_MULTIPLE_4);
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
        .data.mip_levels[0] = SG_RANGE(pixels),
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
        .data.mip_levels[0] = SG_RANGE(pixels),
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
        .data.mip_levels[0] = SG_RANGE(pixels),
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
        .data.mip_levels[0] = SG_RANGE(pixels),
    });
    T(sg_query_image_state(img) == SG_RESOURCESTATE_FAILED);
    T(log_items[0] == SG_LOGITEM_VALIDATE_IMAGEDESC_MSAA_BUT_NO_ATTACHMENT);
    T(log_items[1] == SG_LOGITEM_VALIDATION_FAILED);
    sg_shutdown();
}

UTEST(sokol_gfx, make_image_validate_msaa_num_mipmaps) {
    setup(&(sg_desc){0});
    sg_image img = sg_make_image(&(sg_image_desc){
        .usage.color_attachment = true,
        .width = 64,
        .height = 64,
        .sample_count = 4,
        .num_mipmaps = 2,
    });
    T(sg_query_image_state(img) == SG_RESOURCESTATE_FAILED);
    T(log_items[0] == SG_LOGITEM_VALIDATE_IMAGEDESC_ATTACHMENT_MSAA_NUM_MIPMAPS);
    T(log_items[1] == SG_LOGITEM_VALIDATION_FAILED);
    sg_shutdown();
}

UTEST(sokol_gfx, make_image_validate_msaa_3d_image) {
    setup(&(sg_desc){0});
    sg_image img = sg_make_image(&(sg_image_desc){
        .usage.color_attachment = true,
        .type = SG_IMAGETYPE_3D,
        .width = 32,
        .height = 32,
        .num_slices = 32,
        .sample_count = 4,
    });
    T(sg_query_image_state(img) == SG_RESOURCESTATE_FAILED);
    T(log_items[0] == SG_LOGITEM_VALIDATE_IMAGEDESC_ATTACHMENT_MSAA_3D_IMAGE);
    T(log_items[1] == SG_LOGITEM_VALIDATION_FAILED);
    sg_shutdown();
}

UTEST(sokol_gfx, make_image_validate_depth_3d_image_with_depth_format) {
    setup(&(sg_desc){0});
    sg_image img = sg_make_image(&(sg_image_desc){
        .usage.depth_stencil_attachment = true,
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
        .usage = {
            .color_attachment = true,
            .copy_dst = true,
        },
        .width = 8,
        .height = 8,
    });
    T(sg_query_image_state(img) == SG_RESOURCESTATE_FAILED);
    T(log_items[0] == SG_LOGITEM_VALIDATE_IMAGEDESC_ATTACHMENT_EXPECT_IMMUTABLE);
    T(log_items[1] == SG_LOGITEM_VALIDATE_IMAGEDESC_COPYDST_VS_ATTACHMENT);
    T(log_items[2] == SG_LOGITEM_VALIDATION_FAILED);
    sg_shutdown();
}

UTEST(sokol_gfx, make_image_validate_copy_dst_no_data) {
    setup(&(sg_desc){0});
    uint32_t pixels[8][8] = {0};
    sg_image img = sg_make_image(&(sg_image_desc){
        .usage.copy_dst = true,
        .width = 8,
        .height = 8,
        .data.mip_levels[0] = SG_RANGE(pixels),
    });
    T(sg_query_image_state(img) == SG_RESOURCESTATE_FAILED);
    T(log_items[0] == SG_LOGITEM_VALIDATE_IMAGEDESC_WRITABLE_NO_DATA);
    T(log_items[1] == SG_LOGITEM_VALIDATION_FAILED);
    sg_shutdown();
}

UTEST(sokol_gfx, make_image_compressed_writable) {
    setup(&(sg_desc){0});
    // compressed images don't need to be immutable
    sg_image img0 = sg_make_image(&(sg_image_desc){
        .usage.copy_dst = true,
        .width = 8,
        .height = 8,
        .pixel_format = SG_PIXELFORMAT_BC1_RGBA,
    });
    sg_image img1 = sg_make_image(&(sg_image_desc){
        .usage.write_transient = true,
        .width = 8,
        .height = 8,
        .pixel_format = SG_PIXELFORMAT_BC1_RGBA,
    });
    T(sg_query_image_state(img0) == SG_RESOURCESTATE_VALID);
    T(sg_query_image_state(img1) == SG_RESOURCESTATE_VALID);
    T(num_log_called == 0);
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
        .data.mip_levels[0] = SG_RANGE(pixels),
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
        .data.mip_levels[0] = SG_RANGE(mip0),
        .data.mip_levels[1] = SG_RANGE(mip1),
        .data.mip_levels[2] = SG_RANGE(mip2),
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
        .data.mip_levels[0] = SG_RANGE(mip0),
        .data.mip_levels[1] = SG_RANGE(mip2),
        .data.mip_levels[2] = SG_RANGE(mip1),
        .data.mip_levels[3] = SG_RANGE(mip3)
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

UTEST(sokol_gfx, make_sampler_validate_anistropic_requires_linear_filtering) {
    setup(&(sg_desc){0});
    sg_sampler smp;

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

UTEST(sokol_gfx, make_pipeline_depth_only) {
    setup(&(sg_desc){0});
    // a NONE color format in colors[0] selects depth-only rendering, the desc
    // defaults then force color_count to zero, which must pass validation
    sg_pipeline pip = sg_make_pipeline(&(sg_pipeline_desc){
        .shader = create_shader(),
        .colors[0].pixel_format = SG_PIXELFORMAT_NONE,
        .depth = {
            .pixel_format = SG_PIXELFORMAT_DEPTH,
            .write_enabled = true,
            .compare = SG_COMPAREFUNC_LESS_EQUAL,
        },
    });
    T(sg_query_pipeline_state(pip) == SG_RESOURCESTATE_VALID);
    T(sg_query_pipeline_desc(pip).color_count == 0);
    T(num_log_called == 0);
    sg_shutdown();
}

UTEST(sokol_gfx, make_pipeline_validate_color_count) {
    setup(&(sg_desc){0});
    sg_shader shd = create_shader();
    sg_pipeline pip;

    // a negative color count is rejected
    pip = sg_make_pipeline(&(sg_pipeline_desc){
        .shader = shd,
        .color_count = -1,
    });
    T(sg_query_pipeline_state(pip) == SG_RESOURCESTATE_FAILED);
    T(log_items[0] == SG_LOGITEM_VALIDATE_PIPELINEDESC_COLOR_COUNT);
    T(log_items[1] == SG_LOGITEM_VALIDATION_FAILED);

    // the maximum color count is valid
    reset_log_items();
    pip = sg_make_pipeline(&(sg_pipeline_desc){
        .shader = shd,
        .color_count = SG_MAX_COLOR_ATTACHMENTS,
    });
    T(sg_query_pipeline_state(pip) == SG_RESOURCESTATE_VALID);
    T(sg_query_pipeline_desc(pip).color_count == SG_MAX_COLOR_ATTACHMENTS);
    T(num_log_called == 0);

    // too many color attachments are clamped by the desc defaults, not rejected
    reset_log_items();
    pip = sg_make_pipeline(&(sg_pipeline_desc){
        .shader = shd,
        .color_count = SG_MAX_COLOR_ATTACHMENTS + 1,
    });
    T(sg_query_pipeline_state(pip) == SG_RESOURCESTATE_VALID);
    T(sg_query_pipeline_desc(pip).color_count == SG_MAX_COLOR_ATTACHMENTS);
    T(num_log_called == 0);

    sg_shutdown();
}

UTEST(sokol_gfx, make_view_validate_start_canary) {
    setup(&(sg_desc){0});
    sg_view view = sg_make_view(&(sg_view_desc){
        ._start_canary = 1234,
        .color_attachment.image = sg_make_image(&(sg_image_desc){
            .usage.color_attachment = true,
            .width = 64,
            .height = 64,
        }),
    });
    T(sg_query_view_state(view) == SG_RESOURCESTATE_FAILED);
    T(log_items[0] == SG_LOGITEM_VALIDATE_VIEWDESC_CANARY);
    sg_shutdown();
}

UTEST(sokol_gfx, make_view_validate_end_canary) {
    setup(&(sg_desc){0});
    sg_view atts = sg_make_view(&(sg_view_desc){
        .color_attachment.image = sg_make_image(&(sg_image_desc){
            .usage.color_attachment = true,
            .width = 64,
            .height = 64,
        }),
        ._end_canary = 1234,
    });
    T(sg_query_view_state(atts) == SG_RESOURCESTATE_FAILED);
    T(log_items[0] == SG_LOGITEM_VALIDATE_VIEWDESC_CANARY);
    T(log_items[1] == SG_LOGITEM_VALIDATION_FAILED);
    sg_shutdown();
}

UTEST(sokol_gfx, make_view_validate_image) {
    setup(&(sg_desc){0});
    const sg_image img = sg_make_image(&(sg_image_desc){
        .usage.color_attachment = true,
        .width = 64,
        .height = 65
    });
    sg_destroy_image(img);
    const sg_view view = sg_make_view(&(sg_view_desc){
        .color_attachment.image = img,
    });
    T(sg_query_view_state(view) == SG_RESOURCESTATE_FAILED);
    T(log_items[0] == SG_LOGITEM_VALIDATE_VIEWDESC_RESOURCE_ALIVE);
    T(log_items[1] == SG_LOGITEM_VALIDATION_FAILED);
    sg_shutdown();
}

UTEST(sokol_gfx, make_view_validate_miplevel) {
    setup(&(sg_desc){0});
    const sg_image img = sg_make_image(&(sg_image_desc){
        .usage.color_attachment = 4,
        .width = 16,
        .height = 16,
        .num_mipmaps = 4,
    });
    const sg_view view = sg_make_view(&(sg_view_desc){
        .color_attachment = { .image = img, .mip_level = 4 }
    });
    T(sg_query_view_state(view) == SG_RESOURCESTATE_FAILED);
    T(log_items[0] == SG_LOGITEM_VALIDATE_VIEWDESC_IMAGE_MIPLEVEL);
    T(log_items[1] == SG_LOGITEM_VALIDATION_FAILED);
    sg_shutdown();
}

UTEST(sokol_gfx, make_view_validate_face) {
    setup(&(sg_desc){0});
    const sg_image img = sg_make_image(&(sg_image_desc){
        .usage.color_attachment = true,
        .type = SG_IMAGETYPE_CUBE,
        .width = 64,
        .height = 64,
    });
    const sg_view view = sg_make_view(&(sg_view_desc){
        .color_attachment = { .image = img, .slice = 6 }
    });
    T(sg_query_view_state(view) == SG_RESOURCESTATE_FAILED);
    T(log_items[0] == SG_LOGITEM_VALIDATE_VIEWDESC_IMAGE_CUBEMAP_SLICE);
    T(log_items[1] == SG_LOGITEM_VALIDATION_FAILED);
    sg_shutdown();
}

UTEST(sokol_gfx, make_view_validate_array_slice) {
    setup(&(sg_desc){0});
    const sg_image img = sg_make_image(&(sg_image_desc){
        .usage.color_attachment = true,
        .type = SG_IMAGETYPE_ARRAY,
        .width = 64,
        .height = 64,
        .num_slices = 4,
    });
    const sg_view view = sg_make_view(&(sg_view_desc){
        .color_attachment = { .image = img, .slice = 5 },
    });
    T(sg_query_view_state(view) == SG_RESOURCESTATE_FAILED);
    T(log_items[0] == SG_LOGITEM_VALIDATE_VIEWDESC_IMAGE_ARRAY_SLICE);
    T(log_items[1] == SG_LOGITEM_VALIDATION_FAILED);
    sg_shutdown();
}

UTEST(sokol_gfx, make_view_validate_3dslice) {
    setup(&(sg_desc){0});
    const sg_image img = sg_make_image(&(sg_image_desc){
        .usage.color_attachment = true,
        .type = SG_IMAGETYPE_3D,
        .width = 64,
        .height = 64,
        .num_slices = 4,
    });
    const sg_view view = sg_make_view(&(sg_view_desc){
        .color_attachment = { .image = img, .slice = 5 },
    });
    T(sg_query_view_state(view) == SG_RESOURCESTATE_FAILED);
    T(log_items[0] == SG_LOGITEM_VALIDATE_VIEWDESC_IMAGE_3D_SLICE);
    T(log_items[1] == SG_LOGITEM_VALIDATION_FAILED);
    sg_shutdown();
}

UTEST(sokol_gfx, make_view_validate_image_no_usage) {
    setup(&(sg_desc){0});
    const sg_image img = sg_make_image(&(sg_image_desc){
        .usage.copy_dst = true,
        .width = 8,
        .height = 8,
    });
    const sg_view view = sg_make_view(&(sg_view_desc){
        .color_attachment.image = img,
    });
    T(sg_query_view_state(view) == SG_RESOURCESTATE_FAILED);
    T(log_items[0] == SG_LOGITEM_VALIDATE_VIEWDESC_COLORATTACHMENT_USAGE);
    T(log_items[1] == SG_LOGITEM_VALIDATION_FAILED);
    sg_shutdown();
}

UTEST(sokol_gfx, make_view_validate_color_inv_pixelformat) {
    setup(&(sg_desc){0});
    const sg_image_desc img_desc = {
        .usage.color_attachment = true,
        .width = 8,
        .height = 8,
        .pixel_format = SG_PIXELFORMAT_DEPTH,
    };
    reset_log_items();
    const sg_view view = sg_make_view(&(sg_view_desc){
        .color_attachment.image = sg_make_image(&img_desc),
    });
    T(sg_query_view_state(view) == SG_RESOURCESTATE_FAILED);
    T(log_items[0] == SG_LOGITEM_VALIDATE_VIEWDESC_COLORATTACHMENT_PIXELFORMAT);
    T(log_items[1] == SG_LOGITEM_VALIDATION_FAILED);
    sg_shutdown();
}

UTEST(sokol_gfx, make_view_validate_depth_inv_pixelformat) {
    setup(&(sg_desc){0});
    const sg_image_desc img_desc = {
        .usage.depth_stencil_attachment = true,
        .width = 8,
        .height = 8,
        // need to explicitly use a non-depth pixel format, otherwise
        // the default depth format will be picked
        .pixel_format = SG_PIXELFORMAT_RGBA8,
    };
    const sg_view view = sg_make_view(&(sg_view_desc){
        .depth_stencil_attachment.image = sg_make_image(&img_desc),
    });
    T(sg_query_view_state(view) == SG_RESOURCESTATE_FAILED);
    T(log_items[0] == SG_LOGITEM_VALIDATE_VIEWDESC_DEPTHSTENCILATTACHMENT_PIXELFORMAT);
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
    T(sg_query_pixelformat(SG_PIXELFORMAT_ETC2_RGB8).bytes_per_pixel == 0);
    T(sg_query_pixelformat(SG_PIXELFORMAT_ETC2_RGB8A1).bytes_per_pixel == 0);
    T(sg_query_pixelformat(SG_PIXELFORMAT_ETC2_RGBA8).bytes_per_pixel == 0);
    T(sg_query_pixelformat(SG_PIXELFORMAT_EAC_R11).bytes_per_pixel == 0);
    T(sg_query_pixelformat(SG_PIXELFORMAT_EAC_R11SN).bytes_per_pixel == 0);
    T(sg_query_pixelformat(SG_PIXELFORMAT_EAC_R11).bytes_per_pixel == 0);
    T(sg_query_pixelformat(SG_PIXELFORMAT_EAC_R11SN).bytes_per_pixel == 0);
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

UTEST(sokol_gfx, max_texture_bindings_per_stage_vs) {
    setup(&(sg_desc){
        .disable_validation = true,
    });
    sg_shader_desc desc = {0};
    for (int i = 0; i < SG_MAX_VIEW_BINDSLOTS; i++) {
        desc.views[i].texture.stage = SG_SHADERSTAGE_VERTEX;
    }
    sg_shader shd = sg_make_shader(&desc);
    T(sg_query_shader_state(shd) == SG_RESOURCESTATE_FAILED);
    T(log_items[0] == SG_LOGITEM_SHADERDESC_TOO_MANY_VERTEXSTAGE_TEXTURES);
    sg_shutdown();
}

UTEST(sokol_gfx, max_texture_bindings_per_stage_fs) {
    setup(&(sg_desc){
        .disable_validation = true,
    });
    sg_shader_desc desc = {0};
    for (int i = 0; i < SG_MAX_VIEW_BINDSLOTS; i++) {
        desc.views[i].texture.stage = SG_SHADERSTAGE_FRAGMENT;
    }
    sg_shader shd = sg_make_shader(&desc);
    T(sg_query_shader_state(shd) == SG_RESOURCESTATE_FAILED);
    T(log_items[0] == SG_LOGITEM_SHADERDESC_TOO_MANY_FRAGMENTSTAGE_TEXTURES);
    sg_shutdown();
}

UTEST(sokol_gfx, max_texture_bindings_per_stage_cs) {
    setup(&(sg_desc){
        .disable_validation = true,
    });
    sg_shader_desc desc = {0};
    for (int i = 0; i < SG_MAX_VIEW_BINDSLOTS; i++) {
        desc.views[i].texture.stage = SG_SHADERSTAGE_COMPUTE;
    }
    sg_shader shd = sg_make_shader(&desc);
    T(sg_query_shader_state(shd) == SG_RESOURCESTATE_FAILED);
    T(log_items[0] == SG_LOGITEM_SHADERDESC_TOO_MANY_COMPUTESTAGE_TEXTURES);
    sg_shutdown();
}

UTEST(sokol_gfx, max_storagebuffer_bindings_per_stage_vs) {
    setup(&(sg_desc){0});
    sg_shader_desc desc = {0};
    for (int i = 0; i < SG_MAX_VIEW_BINDSLOTS; i++) {
        desc.views[i].storage_buffer.stage = SG_SHADERSTAGE_VERTEX;
    }
    sg_shader shd = sg_make_shader(&desc);
    T(sg_query_shader_state(shd) == SG_RESOURCESTATE_FAILED);
    T(log_items[0] == SG_LOGITEM_SHADERDESC_TOO_MANY_VERTEXSTAGE_STORAGEBUFFERS);
    sg_shutdown();
}

UTEST(sokol_gfx, max_storagebuffer_bindings_per_stage_fs) {
    setup(&(sg_desc){
        .disable_validation = true,
    });
    sg_shader_desc desc = {0};
    for (int i = 0; i < SG_MAX_VIEW_BINDSLOTS; i++) {
        desc.views[i].storage_buffer.stage = SG_SHADERSTAGE_FRAGMENT;
    }
    sg_shader shd = sg_make_shader(&desc);
    T(sg_query_shader_state(shd) == SG_RESOURCESTATE_FAILED);
    T(log_items[0] == SG_LOGITEM_SHADERDESC_TOO_MANY_FRAGMENTSTAGE_STORAGEBUFFERS);
    sg_shutdown();
}

UTEST(sokol_gfx, max_writable_storagebuffer_bindings_per_stage_cs) {
    setup(&(sg_desc){
        .disable_validation = true,
    });
    sg_shader_desc desc = {0};
    for (int i = 0; i < SG_MAX_VIEW_BINDSLOTS; i++) {
        desc.views[i].storage_buffer.stage = SG_SHADERSTAGE_COMPUTE;
    }
    sg_shader shd = sg_make_shader(&desc);
    T(sg_query_shader_state(shd) == SG_RESOURCESTATE_FAILED);
    T(log_items[0] == SG_LOGITEM_SHADERDESC_TOO_MANY_COMPUTESTAGE_STORAGEBUFFERS);
    sg_shutdown();
}

UTEST(sokol_gfx, max_readonly_storagebuffer_bindings_per_stage_cs) {
    setup(&(sg_desc){0});
    sg_shader_desc desc = {0};
    for (int i = 0; i < SG_MAX_VIEW_BINDSLOTS; i++) {
        desc.views[i].storage_buffer.stage = SG_SHADERSTAGE_COMPUTE;
        desc.views[i].storage_buffer.readonly = true;
    }
    sg_shader shd = sg_make_shader(&desc);
    T(sg_query_shader_state(shd) == SG_RESOURCESTATE_FAILED);
    T(log_items[0] == SG_LOGITEM_SHADERDESC_TOO_MANY_COMPUTESTAGE_STORAGEBUFFERS);
    sg_shutdown();
}

UTEST(sokol_gfx, max_storageimage_bindings_per_stage_vs) {
    setup(&(sg_desc){
        .disable_validation = true,
    });
    sg_shader_desc desc = {0};
    for (int i = 0; i < SG_MAX_VIEW_BINDSLOTS; i++) {
        desc.views[i].storage_image.stage = SG_SHADERSTAGE_VERTEX;
    }
    sg_shader shd = sg_make_shader(&desc);
    T(sg_query_shader_state(shd) == SG_RESOURCESTATE_FAILED);
    T(log_items[0] == SG_LOGITEM_SHADERDESC_TOO_MANY_VERTEXSTAGE_STORAGEIMAGES);
    sg_shutdown();
}

UTEST(sokol_gfx, max_storageimage_bindings_per_stage_fs) {
    setup(&(sg_desc){
        .disable_validation = true,
    });
    sg_shader_desc desc = {0};
    for (int i = 0; i < SG_MAX_VIEW_BINDSLOTS; i++) {
        desc.views[i].storage_image.stage = SG_SHADERSTAGE_FRAGMENT;
    }
    sg_shader shd = sg_make_shader(&desc);
    T(sg_query_shader_state(shd) == SG_RESOURCESTATE_FAILED);
    T(log_items[0] == SG_LOGITEM_SHADERDESC_TOO_MANY_FRAGMENTSTAGE_STORAGEIMAGES);
    sg_shutdown();
}

UTEST(sokol_gfx, max_storageimage_bindings_per_stage_cs) {
    setup(&(sg_desc){
        .disable_validation = true,
    });
    sg_shader_desc desc = {0};
    for (int i = 0; i < SG_MAX_VIEW_BINDSLOTS; i++) {
        desc.views[i].storage_image.stage = SG_SHADERSTAGE_COMPUTE;
    }
    sg_shader shd = sg_make_shader(&desc);
    T(sg_query_shader_state(shd) == SG_RESOURCESTATE_FAILED);
    T(log_items[0] == SG_LOGITEM_SHADERDESC_TOO_MANY_COMPUTESTAGE_STORAGEIMAGES);
    sg_shutdown();
}

UTEST(sokol_gfx, max_texturesamplerpairs_per_stage_vs) {
    setup(&(sg_desc){
        .disable_validation = true,
    });
    sg_shader_desc desc = {0};
    for (int i = 0; i < SG_MAX_VIEW_BINDSLOTS; i++) {
        desc.texture_sampler_pairs[i].stage = SG_SHADERSTAGE_VERTEX;
    }
    sg_shader shd = sg_make_shader(&desc);
    T(sg_query_shader_state(shd) == SG_RESOURCESTATE_FAILED);
    T(log_items[0] == SG_LOGITEM_SHADERDESC_TOO_MANY_VERTEXSTAGE_TEXTURESAMPLERPAIRS);
    sg_shutdown();
}

UTEST(sokol_gfx, max_texturesamplerpairs_per_stage_fs) {
    setup(&(sg_desc){
        .disable_validation = true,
    });
    sg_shader_desc desc = {0};
    for (int i = 0; i < SG_MAX_VIEW_BINDSLOTS; i++) {
        desc.texture_sampler_pairs[i].stage = SG_SHADERSTAGE_FRAGMENT;
    }
    sg_shader shd = sg_make_shader(&desc);
    T(sg_query_shader_state(shd) == SG_RESOURCESTATE_FAILED);
    T(log_items[0] == SG_LOGITEM_SHADERDESC_TOO_MANY_FRAGMENTSTAGE_TEXTURESAMPLERPAIRS);
    sg_shutdown();
}

UTEST(sokol_gfx, max_texturesamplerpairs_per_stage_cs) {
    setup(&(sg_desc){
        .disable_validation = true,
    });
    sg_shader_desc desc = {0};
    for (int i = 0; i < SG_MAX_VIEW_BINDSLOTS; i++) {
        desc.texture_sampler_pairs[i].stage = SG_SHADERSTAGE_COMPUTE;
    }
    sg_shader shd = sg_make_shader(&desc);
    T(sg_query_shader_state(shd) == SG_RESOURCESTATE_FAILED);
    T(log_items[0] == SG_LOGITEM_SHADERDESC_TOO_MANY_COMPUTESTAGE_TEXTURESAMPLERPAIRS);
    sg_shutdown();
}

UTEST(sokol_gfx, max_pass_attachments) {
    setup(&(sg_desc){0});
    sg_pass pass = {0};
    for (int i = 0; i < SG_MAX_COLOR_ATTACHMENTS; i++) {
        sg_image color_img = sg_make_image(&(sg_image_desc){
            .usage.color_attachment = true,
            .width = 64,
            .height = 64,
            .sample_count = 4,
        });
        sg_image resolve_img = sg_make_image(&(sg_image_desc){
            .usage.resolve_attachment = true,
            .width = 64,
            .height = 64,
        });
        pass.attachments.colors[i] = sg_make_view(&(sg_view_desc){
            .color_attachment.image = color_img,
        });
        pass.attachments.resolves[i] = sg_make_view(&(sg_view_desc){
        .resolve_attachment.image = resolve_img,
        });
    }
    sg_begin_pass(&pass);
    T(log_items[0] == SG_LOGITEM_BEGINPASS_TOO_MANY_COLOR_ATTACHMENTS);
    T(log_items[1] == SG_LOGITEM_BEGINPASS_TOO_MANY_RESOLVE_ATTACHMENTS);
    sg_end_pass();
    sg_commit();
    sg_shutdown();
}

// write-transient deferred-validation tests

static sg_pipeline create_write_transient_test_pipeline(void) {
    return sg_make_pipeline(&(sg_pipeline_desc){
        .shader = sg_make_shader(&(sg_shader_desc){0}),
        .layout.attrs[0].format = SG_VERTEXFORMAT_FLOAT3,
    });
}

static sg_buffer create_write_transient_test_buffer(void) {
    return sg_make_buffer(&(sg_buffer_desc){
        .usage = { .vertex_buffer = true, .write_transient = true },
        .size = 128,
    });
}

// pipeline + shader with one write_transient texture view for the image tests
typedef struct {
    sg_pipeline pip;
    sg_buffer vbuf;
    sg_image img;
    sg_view view;
    sg_sampler smp;
} write_transient_image_setup_t;

static write_transient_image_setup_t create_write_transient_image_setup(void) {
    write_transient_image_setup_t s = {0};
    // non-transient vertex buffer to keep the buffer-side check silent
    const float verts[8] = {0};
    s.vbuf = sg_make_buffer(&(sg_buffer_desc){
        .usage.vertex_buffer = true,
        .data = SG_RANGE(verts),
    });
    s.img = sg_make_image(&(sg_image_desc){
        .usage.write_transient = true,
        .width = 8,
        .height = 8,
        .pixel_format = SG_PIXELFORMAT_RGBA8,
    });
    s.view = sg_make_view(&(sg_view_desc){ .texture.image = s.img });
    s.smp = sg_make_sampler(&(sg_sampler_desc){0});
    sg_shader shd = sg_make_shader(&(sg_shader_desc){
        .views[0].texture = {
            .stage = SG_SHADERSTAGE_FRAGMENT,
            .image_type = SG_IMAGETYPE_2D,
            .sample_type = SG_IMAGESAMPLETYPE_FLOAT,
        },
        .samplers[0] = {
            .stage = SG_SHADERSTAGE_FRAGMENT,
            .sampler_type = SG_SAMPLERTYPE_FILTERING,
        },
        .texture_sampler_pairs[0] = {
            .stage = SG_SHADERSTAGE_FRAGMENT,
            .view_slot = 0,
            .sampler_slot = 0,
        },
    });
    s.pip = sg_make_pipeline(&(sg_pipeline_desc){
        .shader = shd,
        .layout.attrs[0].format = SG_VERTEXFORMAT_FLOAT3,
    });
    return s;
}

#define WRITE_TRANSIENT_TEST_PASS &(sg_pass){ \
    .swapchain = { \
        .width = 64, \
        .height = 64, \
        .sample_count = 1, \
        .color_format = SG_PIXELFORMAT_RGBA8, \
    } \
}

// missing sg_write_buffer_transient() must be reported at sg_draw() time,
// not at sg_apply_bindings() time
UTEST(sokol_gfx, draw_write_buffer_transient_missing) {
    setup(&(sg_desc){0});
    sg_buffer buf = create_write_transient_test_buffer();
    sg_pipeline pip = create_write_transient_test_pipeline();
    sg_begin_pass(WRITE_TRANSIENT_TEST_PASS);
    sg_apply_pipeline(pip);
    sg_apply_bindings(&(sg_bindings){ .vertex_buffers[0] = buf });
    reset_log_items();
    sg_draw(0, 3, 1);
    T(log_items[0] == SG_LOGITEM_VALIDATE_DRAW_WRITE_BUFFER_TRANSIENT_MISSING);
    T(log_items[1] == SG_LOGITEM_VALIDATION_FAILED);
    T(num_log_called == 2);
    sg_end_pass();
    sg_commit();
    sg_shutdown();
}

// write -> bind -> draw is the correct sequence and must not error
UTEST(sokol_gfx, draw_write_buffer_transient_ok) {
    setup(&(sg_desc){0});
    sg_buffer buf = create_write_transient_test_buffer();
    sg_pipeline pip = create_write_transient_test_pipeline();
    sg_begin_pass(WRITE_TRANSIENT_TEST_PASS);
    const float data[8] = {0};
    sg_write_buffer_transient(&(sg_write_buffer_desc){
        .src.data = SG_RANGE(data),
        .dst.buffer = buf,
        .size = sizeof(data),
    });
    sg_apply_pipeline(pip);
    sg_apply_bindings(&(sg_bindings){ .vertex_buffers[0] = buf });
    reset_log_items();
    sg_draw(0, 3, 1);
    T(num_log_called == 0);
    sg_end_pass();
    sg_commit();
    sg_shutdown();
}

// regression test: sg_apply_pipeline() must reset the write_transient_missing
// flags so an apply_pipeline -> draw sequence (bindings skipped) only reports
// the real REQUIRED_BINDINGS_OR_UNIFORMS_MISSING error and doesn't leak a
// stale flag from an earlier sg_apply_bindings()
UTEST(sokol_gfx, apply_pipeline_resets_write_transient_flag) {
    setup(&(sg_desc){0});
    sg_buffer buf = create_write_transient_test_buffer();
    sg_pipeline pip = create_write_transient_test_pipeline();
    sg_begin_pass(WRITE_TRANSIENT_TEST_PASS);
    // set the stale flag by binding an unwritten write_transient buffer
    sg_apply_pipeline(pip);
    sg_apply_bindings(&(sg_bindings){ .vertex_buffers[0] = buf });
    // fresh apply_pipeline must clear the flag
    sg_apply_pipeline(pip);
    reset_log_items();
    sg_draw(0, 3, 1);
    T(log_items[0] == SG_LOGITEM_VALIDATE_DRAW_REQUIRED_BINDINGS_OR_UNIFORMS_MISSING);
    T(log_items[1] == SG_LOGITEM_VALIDATION_FAILED);
    T(num_log_called == 2);
    sg_end_pass();
    sg_commit();
    sg_shutdown();
}

// missing sg_write_image_transient() must be reported at sg_draw() time,
// not at sg_apply_bindings() time
UTEST(sokol_gfx, draw_write_image_transient_missing) {
    setup(&(sg_desc){0});
    write_transient_image_setup_t s = create_write_transient_image_setup();
    sg_begin_pass(WRITE_TRANSIENT_TEST_PASS);
    sg_apply_pipeline(s.pip);
    sg_apply_bindings(&(sg_bindings){
        .vertex_buffers[0] = s.vbuf,
        .views[0] = s.view,
        .samplers[0] = s.smp,
    });
    reset_log_items();
    sg_draw(0, 3, 1);
    T(log_items[0] == SG_LOGITEM_VALIDATE_DRAW_WRITE_IMAGE_TRANSIENT_MISSING);
    T(log_items[1] == SG_LOGITEM_VALIDATION_FAILED);
    T(num_log_called == 2);
    sg_end_pass();
    sg_commit();
    sg_shutdown();
}

// write -> bind -> draw for a write_transient image must not error
UTEST(sokol_gfx, draw_write_image_transient_ok) {
    setup(&(sg_desc){0});
    write_transient_image_setup_t s = create_write_transient_image_setup();
    sg_begin_pass(WRITE_TRANSIENT_TEST_PASS);
    const uint32_t pixels[8*8] = {0};
    sg_write_image_transient(&(sg_write_image_desc){
        .src = {
            .data = SG_RANGE(pixels),
            .bytes_per_row = 8 * 4,
            .bytes_per_slice = 8 * 8 * 4,
        },
        .dst.image = s.img,
        .size = { .width = 8, .height = 8, .num_slices = 1 },
    });
    sg_apply_pipeline(s.pip);
    sg_apply_bindings(&(sg_bindings){
        .vertex_buffers[0] = s.vbuf,
        .views[0] = s.view,
        .samplers[0] = s.smp,
    });
    reset_log_items();
    sg_draw(0, 3, 1);
    T(num_log_called == 0);
    sg_end_pass();
    sg_commit();
    sg_shutdown();
}

// multiple sg_write_buffer_transient() calls are allowed until the buffer is used for the first time in a frame
UTEST(sokol_gfx, write_buffer_transient_multiple_before_bind) {
    setup(&(sg_desc){0});
    sg_buffer buf = create_write_transient_test_buffer();
    sg_pipeline pip = create_write_transient_test_pipeline();
    const float data[8] = {0};
    for (int frame = 0; frame < 2; frame++) {
        reset_log_items();
        sg_begin_pass(WRITE_TRANSIENT_TEST_PASS);
        sg_write_buffer_transient(&(sg_write_buffer_desc){ .src.data = SG_RANGE(data), .dst = { .buffer = buf, .offset = 0 } });
        sg_write_buffer_transient(&(sg_write_buffer_desc){ .src.data = SG_RANGE(data), .dst = { .buffer = buf, .offset = 32 } });
        sg_write_buffer_transient(&(sg_write_buffer_desc){ .src.data = SG_RANGE(data), .dst = { .buffer = buf, .offset = 64 } });
        T(num_log_called == 0);
        sg_apply_pipeline(pip);
        sg_apply_bindings(&(sg_bindings){ .vertex_buffers[0] = buf });
        sg_draw(0, 3, 1);
        T(num_log_called == 0);
        sg_end_pass();
        sg_commit();
    }
    sg_shutdown();
}

// sg_write_buffer_transient() after the buffer has been bound in the same frame is an error, but ok again in the next frame
UTEST(sokol_gfx, write_buffer_transient_after_bind) {
    setup(&(sg_desc){0});
    sg_buffer buf = create_write_transient_test_buffer();
    sg_pipeline pip = create_write_transient_test_pipeline();
    const float data[8] = {0};
    const sg_write_buffer_desc wr = { .src.data = SG_RANGE(data), .dst.buffer = buf };
    sg_begin_pass(WRITE_TRANSIENT_TEST_PASS);
    sg_write_buffer_transient(&wr);
    sg_apply_pipeline(pip);
    sg_apply_bindings(&(sg_bindings){ .vertex_buffers[0] = buf });
    reset_log_items();
    sg_write_buffer_transient(&wr);
    T(log_items[0] == SG_LOGITEM_VALIDATE_WRITEBUFFERTRANSIENT_WRITE_BEFORE_BIND);
    T(log_items[1] == SG_LOGITEM_VALIDATION_FAILED);
    T(num_log_called == 2);
    sg_end_pass();
    sg_commit();
    reset_log_items();
    sg_write_buffer_transient(&wr);
    T(num_log_called == 0);
    sg_shutdown();
}

// multiple copies from a write-transient buffer are allowed, but not interleaved with sg_write_buffer_transient()
UTEST(sokol_gfx, write_buffer_transient_vs_copy) {
    setup(&(sg_desc){0});
    set_copy_buffer_to_image_features(true, true, false);
    sg_buffer src = sg_make_buffer(&(sg_buffer_desc){
        .usage = { .vertex_buffer = true, .write_transient = true, .copy_src = true },
        .size = 1024,
    });
    sg_buffer dst = sg_make_buffer(&(sg_buffer_desc){ .usage = { .vertex_buffer = true, .copy_dst = true }, .size = 1024 });
    sg_image img = create_copy_dst_image(16, 16);
    T(sg_query_buffer_state(src) == SG_RESOURCESTATE_VALID);
    T(sg_query_buffer_state(dst) == SG_RESOURCESTATE_VALID);
    T(sg_query_image_state(img) == SG_RESOURCESTATE_VALID);
    const uint8_t data[64] = {0};
    const sg_write_buffer_desc wr = { .src.data = SG_RANGE(data), .dst.buffer = src };
    for (int frame = 0; frame < 2; frame++) {
        reset_log_items();
        sg_write_buffer_transient(&wr);
        sg_write_buffer_transient(&wr);
        // any mix of copies to buffers and images
        sg_copy_buffer_to_buffer(&(sg_copy_buffer_to_buffer_desc){ .src.buffer = src, .dst.buffer = dst, .size = 64 });
        sg_copy_buffer_to_image(&(sg_copy_buffer_to_image_desc){ .src.buffer = src, .dst.image = img });
        sg_copy_buffer_to_buffer(&(sg_copy_buffer_to_buffer_desc){ .src.buffer = src, .dst.buffer = dst, .size = 32 });
        T(num_log_called == 0);
        // a write after the first copy is an error
        sg_write_buffer_transient(&wr);
        T(log_items[0] == SG_LOGITEM_VALIDATE_WRITEBUFFERTRANSIENT_WRITE_BEFORE_COPY);
        T(log_items[1] == SG_LOGITEM_VALIDATION_FAILED);
        T(num_log_called == 2);
        // ...and more copies are still ok
        reset_log_items();
        sg_copy_buffer_to_buffer(&(sg_copy_buffer_to_buffer_desc){ .src.buffer = src, .dst.buffer = dst, .size = 16 });
        T(num_log_called == 0);
        sg_commit();
    }
    sg_shutdown();
}

// the copy-before-write rule also applies when the first copy goes to an image
UTEST(sokol_gfx, write_buffer_transient_after_copy_to_image) {
    setup(&(sg_desc){0});
    set_copy_buffer_to_image_features(true, true, false);
    sg_buffer src = sg_make_buffer(&(sg_buffer_desc){
        .usage = { .vertex_buffer = true, .write_transient = true, .copy_src = true },
        .size = 1024,
    });
    sg_image img = create_copy_dst_image(16, 16);
    const uint8_t data[64] = {0};
    const sg_write_buffer_desc wr = { .src.data = SG_RANGE(data), .dst.buffer = src };
    sg_write_buffer_transient(&wr);
    sg_copy_buffer_to_image(&(sg_copy_buffer_to_image_desc){ .src.buffer = src, .dst.image = img });
    T(num_log_called == 0);
    sg_write_buffer_transient(&wr);
    T(log_items[0] == SG_LOGITEM_VALIDATE_WRITEBUFFERTRANSIENT_WRITE_BEFORE_COPY);
    T(log_items[1] == SG_LOGITEM_VALIDATION_FAILED);
    T(num_log_called == 2);
    sg_shutdown();
}

// same rules for write-transient images: multiple writes before first bind, none after
UTEST(sokol_gfx, write_image_transient_multiple_before_bind) {
    setup(&(sg_desc){0});
    write_transient_image_setup_t s = create_write_transient_image_setup();
    const uint32_t pixels[8*4] = {0};
    // write the upper and lower half of the 8x8 image separately
    const sg_write_image_desc wr_top = {
        .src = { .data = SG_RANGE(pixels), .bytes_per_row = 8 * 4, .bytes_per_slice = 8 * 4 * 4 },
        .dst = { .image = s.img, .y = 0 },
        .size = { .width = 8, .height = 4, .num_slices = 1 },
    };
    sg_write_image_desc wr_bottom = wr_top;
    wr_bottom.dst.y = 4;
    for (int frame = 0; frame < 2; frame++) {
        reset_log_items();
        sg_begin_pass(WRITE_TRANSIENT_TEST_PASS);
        sg_write_image_transient(&wr_top);
        sg_write_image_transient(&wr_bottom);
        sg_write_image_transient(&wr_top);
        T(num_log_called == 0);
        sg_apply_pipeline(s.pip);
        sg_apply_bindings(&(sg_bindings){ .vertex_buffers[0] = s.vbuf, .views[0] = s.view, .samplers[0] = s.smp });
        sg_draw(0, 3, 1);
        T(num_log_called == 0);
        // write after bind is an error
        sg_write_image_transient(&wr_bottom);
        T(log_items[0] == SG_LOGITEM_VALIDATE_WRITEIMAGETRANSIENT_WRITE_BEFORE_BIND);
        T(log_items[1] == SG_LOGITEM_VALIDATION_FAILED);
        T(num_log_called == 2);
        sg_end_pass();
        sg_commit();
    }
    sg_shutdown();
}

// multiple sg_write_buffer_unsealed() calls are allowed while the buffer is unsealed, but not after sg_seal_buffer()
UTEST(sokol_gfx, write_buffer_unsealed_only_while_unsealed) {
    setup(&(sg_desc){0});
    sg_buffer buf = sg_make_buffer(&(sg_buffer_desc){ .usage = { .vertex_buffer = true, .write_unsealed = true }, .size = 128 });
    T(sg_query_buffer_state(buf) == SG_RESOURCESTATE_UNSEALED);
    const uint8_t data[64] = {0};
    sg_write_buffer_unsealed(&(sg_write_buffer_desc){ .src.data = SG_RANGE(data), .dst = { .buffer = buf, .offset = 0 } });
    sg_write_buffer_unsealed(&(sg_write_buffer_desc){ .src.data = SG_RANGE(data), .dst = { .buffer = buf, .offset = 64 } });
    T(num_log_called == 0);
    T(sg_query_buffer_state(buf) == SG_RESOURCESTATE_UNSEALED);
    sg_seal_buffer(buf);
    T(sg_query_buffer_state(buf) == SG_RESOURCESTATE_VALID);
    T(num_log_called == 0);
    sg_write_buffer_unsealed(&(sg_write_buffer_desc){ .src.data = SG_RANGE(data), .dst.buffer = buf });
    T(log_items[0] == SG_LOGITEM_VALIDATE_WRITEBUFFERUNSEALED_RESOURCESTATE);
    T(log_items[1] == SG_LOGITEM_VALIDATION_FAILED);
    T(num_log_called == 2);
    sg_shutdown();
}

UTEST(sokol_gfx, write_image_dst_range_with_default_size) {
    setup(&(sg_desc){0});
    sg_image img = sg_make_image(&(sg_image_desc){
        .usage.write_unsealed = true,
        .width = 8,
        .height = 8,
        .pixel_format = SG_PIXELFORMAT_RGBA8,
    });
    T(sg_query_image_state(img) == SG_RESOURCESTATE_UNSEALED);
    const uint32_t pixels[8*8] = {0};

    // a destination offset at the edge of the miplevel defaults the write size to zero
    sg_write_image_unsealed(&(sg_write_image_desc){ .src.data = SG_RANGE(pixels), .dst = { .image = img, .x = 8 } });
    T(log_items[0] == SG_LOGITEM_VALIDATE_WRITEIMAGE_WIDTH);
    T(log_items[1] == SG_LOGITEM_VALIDATE_WRITEIMAGE_DST_X_RANGE);
    T(log_items[2] == SG_LOGITEM_VALIDATION_FAILED);
    T(num_log_called == 3);
    reset_log_items();
    sg_write_image_unsealed(&(sg_write_image_desc){ .src.data = SG_RANGE(pixels), .dst = { .image = img, .y = 8 } });
    T(log_items[0] == SG_LOGITEM_VALIDATE_WRITEIMAGE_HEIGHT);
    T(log_items[1] == SG_LOGITEM_VALIDATE_WRITEIMAGE_DST_Y_RANGE);
    T(log_items[2] == SG_LOGITEM_VALIDATION_FAILED);
    T(num_log_called == 3);
    reset_log_items();
    sg_write_image_unsealed(&(sg_write_image_desc){ .src.data = SG_RANGE(pixels), .dst = { .image = img, .slice = 1 } });
    T(log_items[0] == SG_LOGITEM_VALIDATE_WRITEIMAGE_NUMSLICES);
    T(log_items[1] == SG_LOGITEM_VALIDATE_WRITEIMAGE_DST_SLICE_RANGE);
    T(log_items[2] == SG_LOGITEM_VALIDATION_FAILED);
    T(num_log_called == 3);
    reset_log_items();

    // negative bytes-per-row must be rejected by validation and not assert in the bytes-per-slice multiple check
    sg_write_image_unsealed(&(sg_write_image_desc){
        .src = { .data = SG_RANGE(pixels), .bytes_per_row = -32, .bytes_per_slice = 256 },
        .dst.image = img,
    });
    T(log_items[0] == SG_LOGITEM_VALIDATE_WRITEIMAGE_BYTESPERROW);
    T(log_items[1] == SG_LOGITEM_VALIDATE_WRITEIMAGE_BYTESPERSLICE);
    T(log_items[2] == SG_LOGITEM_VALIDATION_FAILED);
    T(num_log_called == 3);
    reset_log_items();

    // the default write size covers the rest of the miplevel
    sg_write_image_unsealed(&(sg_write_image_desc){ .src.data = SG_RANGE(pixels), .dst = { .image = img, .x = 4, .y = 4 } });
    T(num_log_called == 0);
    sg_shutdown();
}

UTEST(sokol_gfx, write_buffer_unsealed_wrong_usage) {
    setup(&(sg_desc){0});
    sg_buffer buf = sg_make_buffer(&(sg_buffer_desc){ .usage.vertex_buffer = true, .size = 128 });
    T(sg_query_buffer_state(buf) == SG_RESOURCESTATE_VALID);
    const uint8_t data[64] = {0};
    sg_write_buffer_unsealed(&(sg_write_buffer_desc){ .src.data = SG_RANGE(data), .dst.buffer = buf });
    T(log_items[0] == SG_LOGITEM_VALIDATE_WRITEBUFFERUNSEALED_USAGE);
    T(log_items[1] == SG_LOGITEM_VALIDATE_WRITEBUFFERUNSEALED_RESOURCESTATE);
    T(log_items[2] == SG_LOGITEM_VALIDATION_FAILED);
    T(num_log_called == 3);
    sg_shutdown();
}

// same for images, writing is done per region
UTEST(sokol_gfx, write_image_unsealed_only_while_unsealed) {
    setup(&(sg_desc){0});
    sg_image img = sg_make_image(&(sg_image_desc){
        .usage.write_unsealed = true,
        .width = 8,
        .height = 8,
        .pixel_format = SG_PIXELFORMAT_RGBA8,
    });
    T(sg_query_image_state(img) == SG_RESOURCESTATE_UNSEALED);
    const uint32_t pixels[8*4] = {0};
    const sg_write_image_desc wr_top = {
        .src = { .data = SG_RANGE(pixels), .bytes_per_row = 8 * 4, .bytes_per_slice = 8 * 4 * 4 },
        .dst = { .image = img, .y = 0 },
        .size = { .width = 8, .height = 4, .num_slices = 1 },
    };
    sg_write_image_desc wr_bottom = wr_top;
    wr_bottom.dst.y = 4;
    sg_write_image_unsealed(&wr_top);
    sg_write_image_unsealed(&wr_bottom);
    T(num_log_called == 0);
    T(sg_query_image_state(img) == SG_RESOURCESTATE_UNSEALED);
    sg_seal_image(img);
    T(sg_query_image_state(img) == SG_RESOURCESTATE_VALID);
    T(num_log_called == 0);
    sg_write_image_unsealed(&wr_top);
    T(log_items[0] == SG_LOGITEM_VALIDATE_WRITEIMAGEUNSEALED_RESOURCESTATE);
    T(log_items[1] == SG_LOGITEM_VALIDATION_FAILED);
    T(num_log_called == 2);
    sg_shutdown();
}
