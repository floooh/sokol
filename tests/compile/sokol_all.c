#define SOKOL_IMPL
// core
#include "sokol_app.h"
#include "sokol_gfx.h"
#include "sokol_log.h"
#include "sokol_audio.h"
#include "sokol_fetch.h"
#include "sokol_time.h"
#include "sokol_glue.h"
// util
#include "sokol_color.h"
#include "sokol_debugtext.h"
#include "sokol_gl.h"
#include "sokol_letterbox.h"
#include "sokol_memtrack.h"
#include "sokol_shape.h"

#if defined(_MSC_VER )
#pragma warning(disable:4201) // nonstandard extension used: nameless struct/union
#pragma warning(disable:4214) // nonstandard extension used: bit field types other than int
#endif
#include "cimgui.h"
#define SOKOL_IMPL
#if defined(SOKOL_DUMMY_BACKEND)
#define SOKOL_IMGUI_NO_SOKOL_APP
#endif
#include "sokol_imgui.h"
#include "sokol_gfx_imgui.h"
#include "sokol_app_imgui.h"

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
#include "sokol_fontstash.h"

#include "spine/spine.h"
#include "sokol_spine.h"

// include nuklear.h before the sokol_nuklear.h implementation
#define NK_INCLUDE_FIXED_TYPES
#define NK_INCLUDE_STANDARD_IO
#define NK_INCLUDE_DEFAULT_ALLOCATOR
#define NK_INCLUDE_VERTEX_BUFFER_OUTPUT
#define NK_INCLUDE_FONT_BAKING
#define NK_INCLUDE_DEFAULT_FONT
#define NK_INCLUDE_STANDARD_VARARGS
#include "nuklear.h"

#define SOKOL_IMPL
#if defined(SOKOL_DUMMY_BACKEND)
#define SOKOL_NUKLEAR_NO_SOKOL_APP
#endif
#include "sokol_nuklear.h"

void use_all(void) {
    sapp_run(&(sapp_desc){0});
    sg_setup(&(sg_desc){0});
    slog_func("bla", 1, 123, "123", 42, "bla.c", 0);
    saudio_setup(&(saudio_desc){0});
    sfetch_setup(&(sfetch_desc_t){0});
    stm_setup();
    const sg_environment env = sglue_environment(); (void)env;
    sg_color c = sg_make_color_4b(255, 0, 0, 255); (void)c;
    sdtx_setup(&(sdtx_desc_t){0});
    sgl_setup(&(sgl_desc_t){0});
    const slbx_viewport vp = slbx_letterbox(256, 256, &(slbx_letterbox_desc){0}); (void)vp;
    void* p = smemtrack_alloc(8, 0); (void)p;
    sshape_plane_sizes(10);
    simgui_setup(&(simgui_desc_t){0});
    sgimgui_setup(&(sgimgui_desc_t){0});
    sappimgui_setup();
    FONScontext* ctx = sfons_create(&(sfons_desc_t){ 0 }); (void)ctx;
    sspine_setup(&(sspine_desc){0});
    snk_setup(&(snk_desc_t){0});
}
