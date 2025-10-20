#define SOKOL_IMPL
#include "sokol_app_turbo.h"

void use_app_impl(void) {
    sapp_setup(&(sapp_desc){0});
    sapp_shutdown();
}
