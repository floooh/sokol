#include "sokol_app.h"
#include "sokol_gfx.h"
#define CIMGUI_DEFINE_ENUMS_AND_STRUCTS
#if defined(_MSC_VER )
#pragma warning(disable:4201) // nonstandard extension used: nameless struct/union
#pragma warning(disable:4214) // nonstandard extension used: bit field types other than int
#endif
#include "cimgui/cimgui.h"
#include "sokol_imgui.h"
#define SOKOL_IMPL
#include "sokol_gfx_imgui.h"

void use_gfx_imgui_impl(void) {
    sg_imgui_t ctx = {0};
    sg_imgui_init(&ctx, &(sg_imgui_desc_t){0});
    sg_imgui_discard(&ctx);
}
