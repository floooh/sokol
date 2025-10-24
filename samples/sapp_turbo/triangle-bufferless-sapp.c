//------------------------------------------------------------------------------
//  triangle-bufferless-sapp.c
//
//  Rendering a triangle without buffers (instead define the vertex data
//  as constants in the shader).
//------------------------------------------------------------------------------
#include "sokol_app_turbo.h"
#include "sokol_gfx.h"
#include "sokol_log.h"
#include "sokol_glue.h"
// #include "dbgui/dbgui.h"
#include "triangle-bufferless-sapp.glsl.h"

static struct {
    sg_pipeline pip;
    sg_pass_action pass_action;
} state;

static void init(void) {
    sg_setup(&(sg_desc){
        .environment = sglue_environment(),
        .logger.func = slog_func,
    });
    // __dbgui_setup(sapp_sample_count());

    // look ma, no vertex buffer!

    // create a shader object
    sg_shader shd = sg_make_shader(triangle_shader_desc(sg_query_backend()));

    // ...and a pipeline object, note that there's no vertex layout since there's
    // no vertex data passed into the shader.
    // All other pipeline attributes can be left to their defaults for a 2D triangle
    // to show up.
    state.pip = sg_make_pipeline(&(sg_pipeline_desc){ .shader = shd });

    // setup pass action to clear to black
    state.pass_action = (sg_pass_action){
        .colors[0] = { .load_action = SG_LOADACTION_CLEAR, .clear_value = { 0, 0, 0, 1} },
    };
}

static void frame(void) {
    sg_begin_pass(&(sg_pass){ .action = state.pass_action, .swapchain = sglue_swapchain() });
    sg_apply_pipeline(state.pip);
    sg_draw(0, 3, 1);
    // __dbgui_draw();
    sg_end_pass();
    sg_commit();
}

static void cleanup(void) {
    // __dbgui_shutdown();
    sg_shutdown();
}

static void event(const sapp_event* e) {
    (void)e;
    // simgui_handle_event(e);
}


int main(int argc, char* argv[]) {
    (void)argc; (void)argv;
    
    const sapp_desc desc =  (sapp_desc){
        .init_cb = init,
        .frame_cb = frame,
        .cleanup_cb = cleanup,
        .event_cb = event,
        .width = 640,
        .height = 480,
        .window_title = "triangle-bufferless-sapp.c",
        .icon.sokol_default = true,
        .logger.func = slog_func,
    };

    sapp_setup(&desc);

    while(!sapp_should_close()) {
        sapp_poll_events();
    }

    sapp_shutdown();

    return 0;
}