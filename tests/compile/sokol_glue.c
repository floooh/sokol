#include "sokol_app.h"
#include "sokol_gfx.h"
#define SOKOL_IMPL
#include "sokol_glue.h"

void use_glue_impl(void) {
    const sg_environment env = sglue_environment();
    (void)env;
}
