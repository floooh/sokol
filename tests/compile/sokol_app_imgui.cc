#include "imgui.h"
#include "sokol_app.h"
#include "sokol_gfx.h"
#include "sokol_imgui.h"
#define SOKOL_IMPL
#include "sokol_app_imgui.h"

void use_app_imgui_impl() {
    sappimgui_setup();
    sappimgui_shutdown();
}
