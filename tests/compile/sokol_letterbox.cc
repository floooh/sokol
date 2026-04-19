#define SOKOL_IMPL
#include "sokol_letterbox.h"

void use_letterbox_impl(void) {
    slbx_options opts = {};
    opts.aspect = 4.0f / 3.0f;
    const slbx_rect vp = slbx_viewport(256, 256, &opts);
    (void)vp;
}
