#if defined(IMPL)
#ifndef CIMGUI_HEADER_PATH
#define CIMGUI_HEADER_PATH "cimgui.h"
#endif
// NOTE: this is only needed for the old cimgui.h bindings
#define CIMGUI_DEFINE_ENUMS_AND_STRUCTS
#define SOKOL_IMGUI_IMPL
#include CIMGUI_HEADER_PATH
#endif
#include "sokol_defines.h"
#include "sokol_app.h"
#include "sokol_gfx.h"
#include "sokol_imgui.h"
