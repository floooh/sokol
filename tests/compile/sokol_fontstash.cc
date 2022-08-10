#include "sokol_gfx.h"
#include "sokol_gl.h"

#define FONTSTASH_IMPLEMENTATION
#if defined(_MSC_VER )
#pragma warning(disable:4996)   // strncpy use in fontstash.h
#pragma warning(disable:4505)   // unreferenced local function has been removed
#pragma warning(disable:4100)   // unreferenced formal parameter
#endif
#if defined(__GNUC__) || defined(__clang__)
#pragma GCC diagnostic ignored "-Wunused-function"
#pragma GCC diagnostic ignored "-Wsign-conversion"
#endif
#include <stdlib.h>     // malloc/free
#include "fontstash.h"
#define SOKOL_IMPL
#include "sokol_fontstash.h"

void use_fontstash_impl() {
    const sfons_desc_t desc = { };
    FONScontext* ctx = sfons_create(&desc);
    sfons_destroy(ctx);
}
