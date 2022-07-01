#include "sokol_app.h"
#include "sokol_gfx.h"
#define SOKOL_IMPL
#include "sokol_glue.h"

void use_glue_impl() {
    const sg_context_desc ctx = sapp_sgcontext();
    (void)ctx;
}
