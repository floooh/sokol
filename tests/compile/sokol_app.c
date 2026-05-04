#define SOKOL_IMPL
#include "sokol_app.h"

void use_app_impl(void) {
    sapp_run(&(sapp_desc){
        .color_format = SAPP_PIXELFORMAT_RGBA16F
    });
    sapp_set_color_format(SAPP_PIXELFORMAT_BGRA8);
}
