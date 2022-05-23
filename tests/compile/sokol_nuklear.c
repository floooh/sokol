#include "sokol_app.h"
#include "sokol_gfx.h"

// include nuklear.h before the sokol_nuklear.h implementation
#define NK_INCLUDE_FIXED_TYPES
#define NK_INCLUDE_STANDARD_IO
#define NK_INCLUDE_DEFAULT_ALLOCATOR
#define NK_INCLUDE_VERTEX_BUFFER_OUTPUT
#define NK_INCLUDE_FONT_BAKING
#define NK_INCLUDE_DEFAULT_FONT
#define NK_INCLUDE_STANDARD_VARARGS
#define NK_IMPLEMENTATION

#if defined(__clang__)
#pragma GCC diagnostic ignored "-Wunknown-warning-option"
#pragma GCC diagnostic ignored "-Wunused-parameter"
#pragma GCC diagnostic ignored "-Wsign-conversion"
#pragma GCC diagnostic ignored "-Wnull-pointer-subtraction"
#pragma GCC diagnostic ignored "-Wunused-but-set-variable"
#endif
#if defined(__GNUC__) && !defined(__clang__)
#pragma GCC diagnostic ignored "-Wmaybe-uninitialized"
#pragma GCC diagnostic ignored "-Wsign-conversion"
#pragma GCC diagnostic ignored "-Wunused-parameter"
#endif
#if defined(_MSC_VER)
#pragma warning(push)
#pragma warning(disable:4996)   // sprintf,fopen,localtime: This function or variable may be unsafe
#pragma warning(disable:4127)   // conditional expression is constant
#pragma warning(disable:4100)   // unreferenced formal parameter
#pragma warning(disable:4701)   // potentially uninitialized local variable used
#endif
#include "nuklear.h"
#if defined(_MSC_VER)
#pragma warning(pop)
#endif

#define SOKOL_IMPL
#if defined(SOKOL_DUMMY_BACKEND)
#define SOKOL_NUKLEAR_NO_SOKOL_APP
#endif
#include "sokol_nuklear.h"

void use_nuklear_impl(void) {
    snk_setup(&(snk_desc_t){0});
}
