#include "sokol_app.h"
#include "sokol_gfx.h"
#include "imgui.h"
#define SOKOL_IMPL
#define SOKOL_IMGUI_NO_SOKOL_APP
#include "sokol_imgui.h"

void use_imgui_impl() {
    simgui_setup({});
}

int main() {
    return 0;
}