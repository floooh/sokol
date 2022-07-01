#include "sokol_app.h"
#include "sokol_gfx.h"
#include "imgui.h"
#define SOKOL_IMPL
#if defined(SOKOL_DUMMY_BACKEND)
#define SOKOL_IMGUI_NO_SOKOL_APP
#endif
#include "sokol_imgui.h"

void use_imgui_impl() {
    simgui_setup({});
}
