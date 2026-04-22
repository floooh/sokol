#define SOKOL_IMPL
#include "sokol_letterbox.h"

void use_letterbox_impl(void) {
    slbx_letterbox_desc desc = {};
    desc.content_aspect_ratio = 4.0f / 3.0f;
    const slbx_viewport vp = slbx_letterbox(256, 256, &desc);
    (void)vp;
}
