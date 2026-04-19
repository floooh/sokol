#define SOKOL_IMPL
#include "sokol_letterbox.h"

void use_letterbox_impl(void) {
    const slbx_rect vp = slbx_viewport(256, 256, &(slbx_options){ .aspect = 4.0f / 3.0f });
    (void)vp;
}
