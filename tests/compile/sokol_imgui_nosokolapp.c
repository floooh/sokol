#include "sokol_app.h"
#include "sokol_gfx.h"
#if defined(_MSC_VER )
#pragma warning(disable:4201) // nonstandard extension used: nameless struct/union
#pragma warning(disable:4214) // nonstandard extension used: bit field types other than int
#endif
#include "cimgui.h"
#define SOKOL_IMPL
#define SOKOL_IMGUI_NO_SOKOL_APP
#include "sokol_imgui.h"

void use_imgui_impl(void) {
    simgui_setup(&(simgui_desc_t){0});
}

int main(void) {
    return 0;
}
