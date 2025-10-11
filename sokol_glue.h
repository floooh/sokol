#if defined(SOKOL_IMPL) && !defined(SOKOL_GLUE_IMPL)
#define SOKOL_GLUE_IMPL
#endif
#ifndef SOKOL_GLUE_INCLUDED
/*
    sokol_glue.h -- glue helper functions for sokol headers

    Project URL: https://github.com/floooh/sokol

    Do this:
        #define SOKOL_IMPL or
        #define SOKOL_GLUE_IMPL
    before you include this file in *one* C or C++ file to create the
    implementation.

    ...optionally provide the following macros to override defaults:

    SOKOL_ASSERT(c)     - your own assert macro (default: assert(c))
    SOKOL_GLUE_API_DECL - public function declaration prefix (default: extern)
    SOKOL_API_DECL      - same as SOKOL_GLUE_API_DECL
    SOKOL_API_IMPL      - public function implementation prefix (default: -)

    If sokol_glue.h is compiled as a DLL, define the following before
    including the declaration or implementation:

    SOKOL_DLL

    On Windows, SOKOL_DLL will define SOKOL_GLUE_API_DECL as __declspec(dllexport)
    or __declspec(dllimport) as needed.

    OVERVIEW
    ========
    sokol_glue.h provides glue helper functions between sokol_gfx.h and sokol_app.h,
    so that sokol_gfx.h doesn't need to depend on sokol_app.h but can be
    used with different window system glue libraries.

    PROVIDED FUNCTIONS
    ==================

    sg_environment sglue_environment(void)

        Returns an sg_environment struct initialized by calling sokol_app.h
        functions. Use this in the sg_setup() call like this:

        sg_setup(&(sg_desc){
            .environment = sglue_environment(),
            ...
        });

    sg_swapchain sglue_swapchain(void)

        Returns an sg_swapchain struct initialized by calling sokol_app.h
        functions. Use this in sg_begin_pass() for a 'swapchain pass' like
        this:

        sg_begin_pass(&(sg_pass){ .swapchain = sglue_swapchain(), ... });

    LICENSE
    =======
    zlib/libpng license

    Copyright (c) 2018 Andre Weissflog

    This software is provided 'as-is', without any express or implied warranty.
    In no event will the authors be held liable for any damages arising from the
    use of this software.

    Permission is granted to anyone to use this software for any purpose,
    including commercial applications, and to alter it and redistribute it
    freely, subject to the following restrictions:

        1. The origin of this software must not be misrepresented; you must not
        claim that you wrote the original software. If you use this software in a
        product, an acknowledgment in the product documentation would be
        appreciated but is not required.

        2. Altered source versions must be plainly marked as such, and must not
        be misrepresented as being the original software.

        3. This notice may not be removed or altered from any source
        distribution.
*/
#define SOKOL_GLUE_INCLUDED

#if defined(SOKOL_API_DECL) && !defined(SOKOL_GLUE_API_DECL)
#define SOKOL_GLUE_API_DECL SOKOL_API_DECL
#endif
#ifndef SOKOL_GLUE_API_DECL
#if defined(_WIN32) && defined(SOKOL_DLL) && defined(SOKOL_GLUE_IMPL)
#define SOKOL_GLUE_API_DECL __declspec(dllexport)
#elif defined(_WIN32) && defined(SOKOL_DLL)
#define SOKOL_GLUE_API_DECL __declspec(dllimport)
#else
#define SOKOL_GLUE_API_DECL extern
#endif
#endif

#ifndef SOKOL_GFX_INCLUDED
#error "Please include sokol_gfx.h before sokol_glue.h"
#endif

#ifdef __cplusplus
extern "C" {
#endif

SOKOL_GLUE_API_DECL sg_environment sglue_environment(void);
SOKOL_GLUE_API_DECL sg_swapchain sglue_swapchain(void);

#ifdef __cplusplus
} /* extern "C" */
#endif
#endif /* SOKOL_GLUE_INCLUDED */

/*-- IMPLEMENTATION ----------------------------------------------------------*/
#ifdef SOKOL_GLUE_IMPL
#define SOKOL_GLUE_IMPL_INCLUDED (1)
#include <string.h> /* memset */

#ifndef SOKOL_APP_INCLUDED
#error "Please include sokol_app.h before the sokol_glue.h implementation"
#endif

#ifndef SOKOL_API_IMPL
#define SOKOL_API_IMPL
#endif


SOKOL_API_IMPL sg_environment sglue_environment(void) {
    sg_environment env;
    memset(&env, 0, sizeof(env));
    env.defaults.color_format = (sg_pixel_format) sapp_color_format();
    env.defaults.depth_format = (sg_pixel_format) sapp_depth_format();
    env.defaults.sample_count = sapp_sample_count();
    env.metal.device = sapp_metal_get_device();
    env.d3d11.device = sapp_d3d11_get_device();
    env.d3d11.device_context = sapp_d3d11_get_device_context();
    env.wgpu.device = sapp_wgpu_get_device();
    return env;
}

SOKOL_API_IMPL sg_swapchain sglue_swapchain(void) {
    sg_swapchain swapchain;
    memset(&swapchain, 0, sizeof(swapchain));
    swapchain.width = sapp_width();
    swapchain.height = sapp_height();
    swapchain.sample_count = sapp_sample_count();
    swapchain.color_format = (sg_pixel_format)sapp_color_format();
    swapchain.depth_format = (sg_pixel_format)sapp_depth_format();
    swapchain.metal.current_drawable = sapp_metal_get_current_drawable();
    swapchain.metal.depth_stencil_texture = sapp_metal_get_depth_stencil_texture();
    swapchain.metal.msaa_color_texture = sapp_metal_get_msaa_color_texture();
    swapchain.d3d11.render_view = sapp_d3d11_get_render_view();
    swapchain.d3d11.resolve_view = sapp_d3d11_get_resolve_view();
    swapchain.d3d11.depth_stencil_view = sapp_d3d11_get_depth_stencil_view();
    swapchain.wgpu.render_view = sapp_wgpu_get_render_view();
    swapchain.wgpu.resolve_view = sapp_wgpu_get_resolve_view();
    swapchain.wgpu.depth_stencil_view = sapp_wgpu_get_depth_stencil_view();
    swapchain.gl.framebuffer = sapp_gl_get_framebuffer();
    return swapchain;
}

#endif /* SOKOL_GLUE_IMPL */
