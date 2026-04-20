#if defined(IMPL)
#define SOKOL_NUKLEAR_IMPL
#define NK_IMPLEMENTATION
#endif
#define NK_INCLUDE_FIXED_TYPES
#define NK_INCLUDE_STANDARD_IO
#define NK_INCLUDE_DEFAULT_ALLOCATOR
#define NK_INCLUDE_VERTEX_BUFFER_OUTPUT
#define NK_INCLUDE_FONT_BAKING
#define NK_INCLUDE_DEFAULT_FONT
#define NK_INCLUDE_STANDARD_VARARGS
#include "nuklear.h"
#include "sokol_defines.h"
#include "sokol_app.h"
#include "sokol_gfx.h"
#include "sokol_nuklear.h"
