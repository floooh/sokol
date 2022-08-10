#include "sokol_gfx.h"
#include "sokol_gl.h"

#define FONTSTASH_IMPLEMENTATION
#if defined(_MSC_VER )
#pragma warning(disable:4996)   // strncpy use in fontstash.h
#endif
#if defined(__GNUC__) || defined(__clang__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-function"
#pragma GCC diagnostic ignored "-Wsign-conversion"
#endif
#include <stdlib.h>     // malloc/free
#include "fontstash.h"
#define SOKOL_IMPL
#include "sokol_fontstash.h"

void use_fontstash_impl(void) {
    FONScontext* ctx = sfons_create(&(sfons_desc_t){ 0 });
    sfons_destroy(ctx);
}
