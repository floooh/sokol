#include "sokol_app.h"
#include "sokol_gfx.h"

// include nuklear.h before the sokol_nuklear.h implementation
#define NK_INCLUDE_FIXED_TYPES
#define NK_INCLUDE_STANDARD_IO
#define NK_INCLUDE_DEFAULT_ALLOCATOR
#define NK_INCLUDE_VERTEX_BUFFER_OUTPUT
#define NK_INCLUDE_FONT_BAKING
#define NK_INCLUDE_DEFAULT_FONT
#define NK_INCLUDE_STANDARD_VARARGS
#include "nuklear.h"

#define SOKOL_IMPL
#if defined(SOKOL_DUMMY_BACKEND)
#define SOKOL_NUKLEAR_NO_SOKOL_APP
#endif
#include "sokol_nuklear.h"

void use_nuklear_impl(void) {
    snk_setup(&(snk_desc_t){0});
}
