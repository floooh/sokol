#include "sokol_app.h"
#include "sokol_gfx.h"
#define CIMGUI_DEFINE_ENUMS_AND_STRUCTS
#if defined(_MSC_VER )
#pragma warning(disable:4201) // nonstandard extension used: nameless struct/union
#pragma warning(disable:4214) // nonstandard extension used: bit field types other than int
#endif
#include "cimgui/cimgui.h"
#define SOKOL_IMPL
#if defined(SOKOL_DUMMY_BACKEND)
#define SOKOL_IMGUI_NO_SOKOL_APP
#endif
#include "sokol_imgui.h"

void use_imgui_impl(void) {
    simgui_setup(&(simgui_desc_t){0});
}

