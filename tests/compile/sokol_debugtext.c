#include "sokol_gfx.h"
#define SOKOL_IMPL
#include "sokol_debugtext.h"

void use_debugtext_impl(void) {
    sdtx_setup(&(sdtx_desc_t){0});
}
