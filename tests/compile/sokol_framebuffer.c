#include "sokol_gfx.h"
#define SOKOL_IMPL
#include "sokol_framebuffer.h"

void use_sokol_framebuffer(void) {
    sfb_setup(&(sfb_desc){0});
}
