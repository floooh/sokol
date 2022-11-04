#include "sokol_gfx.h"
#define SOKOL_IMPL
#include "spine/spine.h"
#include "sokol_spine.h"

void use_sspine_impl(void) {
    const sspine_desc desc = {};
    sspine_setup(&desc);
}
