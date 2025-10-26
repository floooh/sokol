//------------------------------------------------------------------------------
//  triangle-bufferless-sapp.c
//
//  Rendering a triangle without buffers (instead define the vertex data
//  as constants in the shader).
//------------------------------------------------------------------------------
#include "sokol_app_turbo.h"
#include "sokol_gfx.h"
#include "sokol_time.h"
#include "sokol_log.h"
#include "sokol_glue.h"

#include "cimgui.h"
#define SOKOL_IMGUI_IMPL
#include "util/sokol_imgui.h"
// #include "dbgui/dbgui.h"
#include "triangle-bufferless-sapp.glsl.h"

#include <stdio.h>

static struct {
    sg_pipeline pip;
    sg_pass_action pass_action;
    uint64_t last_time;
    double min_raw_frame_time;
    double max_raw_frame_time;
    double min_rounded_frame_time;
    double max_rounded_frame_time;
} state;

static void reset_minmax_frametimes(void) {
    state.max_raw_frame_time = 0;
    state.min_raw_frame_time = 1000.0;
    state.max_rounded_frame_time = 0;
    state.min_rounded_frame_time = 1000.0;
}

static void init(void) {
    sg_setup(&(sg_desc){
        .environment = sglue_environment(),
        .logger.func = slog_func,
    });
    stm_setup();
    simgui_setup(&(simgui_desc_t){
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

    // List all displays
    int display_count = sapp_display_get_count();
    printf("\nTotal Displays: %d\n", display_count);
    for (int i = 0; i < display_count; i++) {
        const sapp_display* display = sapp_display_get_at_index(i);
        if (display) {
            printf("  Display %d: %s (%dx%d @ %d Hz @ %f DPI scale)\n",
                   i + 1, display->name ? display->name : "Unknown",
                   display->width_px, display->height_px, display->refresh_rate, display->dpi_scale);
        }
    }
    printf("\n");

    reset_minmax_frametimes();
}

static void frame(void) {
    sapp_begin_tick();

    const int width = sapp_width();
    const int height = sapp_height();
    const float fwidth = (float)width;
    const float fheight = (float)height;
    double raw_frame_time = stm_sec(stm_laptime(&state.last_time));
    double rounded_frame_time = sapp_frame_duration();
    if (raw_frame_time > 0) {
        if (raw_frame_time < state.min_raw_frame_time) {
            state.min_raw_frame_time = raw_frame_time;
        }
        if (raw_frame_time > state.max_raw_frame_time) {
            state.max_raw_frame_time = raw_frame_time;
        }
    }
    if (rounded_frame_time > 0) {
        if (rounded_frame_time < state.min_rounded_frame_time) {
            state.min_rounded_frame_time = rounded_frame_time;
        }
        if (rounded_frame_time > state.max_rounded_frame_time) {
            state.max_rounded_frame_time = rounded_frame_time;
        }
    }

    simgui_new_frame(&(simgui_frame_desc_t){
        .width = width,
        .height = height,
        .delta_time = rounded_frame_time,
        .dpi_scale = sapp_dpi_scale()
    });

    // controls window
    igSetNextWindowPos((ImVec2){ 10, 10 }, ImGuiCond_Once);
    igSetNextWindowSize((ImVec2){ 450, 0 }, ImGuiCond_Once);
    igBegin("Controls", 0, ImGuiWindowFlags_NoResize|ImGuiWindowFlags_NoScrollbar);
    igText("Raw frame time:     %.3fms (min: %.3f, max: %.3f)",
        raw_frame_time * 1000.0,
        state.min_raw_frame_time * 1000.0,
        state.max_raw_frame_time * 1000.0);
    igText("Rounded frame time: %.3fms (min: %.3f, max: %.3f)",
        rounded_frame_time * 1000.0,
        state.min_rounded_frame_time * 1000.0,
        state.max_rounded_frame_time * 1000.0);
    if (igButton("Reset min/max times")) {
        reset_minmax_frametimes();
    }
    igEnd();

    sg_begin_pass(&(sg_pass){ .action = state.pass_action, .swapchain = sglue_swapchain() });
    sg_apply_pipeline(state.pip);
    sg_draw(0, 3, 1);
    // __dbgui_draw();
    simgui_render();
    sg_end_pass();
    sg_commit();

    sapp_end_tick();
}

static void cleanup(void) {
    // __dbgui_shutdown();
    simgui_shutdown();
    sg_shutdown();
}

static void event(const sapp_event* e) {
    simgui_handle_event(e);
}


int main(int argc, char* argv[]) {
    (void)argc; (void)argv;
    
    const sapp_desc desc =  (sapp_desc){
        .init_cb = init,
        .cleanup_cb = cleanup,
        .event_cb = event,
        .width = 1280,
        .height = 720,
        .window_title = "triangle-bufferless-sapp.c",
        .icon.sokol_default = true,
        .logger.func = slog_func,
    };

    sapp_setup(&desc);

    while(!sapp_should_close()) {
        frame();
    }

    sapp_shutdown();

    return 0;
}