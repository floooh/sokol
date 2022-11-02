#include "sokol_gfx.h"
#define SOKOL_IMPL
#include "spine/spine.h"
#include "sokol_spine.h"

void use_sspine_impl(void) {
    sspine_setup(&(sspine_desc){0});
}
