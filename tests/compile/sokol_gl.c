#include "sokol_gfx.h"
#define SOKOL_IMPL
#include "sokol_gl.h"

void use_gl_impl(void) {
    sgl_setup(&(sgl_desc_t){0});
}
