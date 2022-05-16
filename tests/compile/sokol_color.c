#include "sokol_gfx.h"
#define SOKOL_IMPL
#include "sokol_color.h"

void use_color_impl(void) {
    sg_color c = sg_make_color_4b(255, 0, 0, 255);
    (void)c;
}
