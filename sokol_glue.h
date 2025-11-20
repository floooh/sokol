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

#ifndef _SOKOL_PRIVATE
    #if defined(__GNUC__) || defined(__clang__)
        #define _SOKOL_PRIVATE __attribute__((unused)) static
    #else
        #define _SOKOL_PRIVATE static
    #endif
#endif

#ifndef SOKOL_ASSERT
    #include <assert.h>
    #define SOKOL_ASSERT(c) assert(c)
#endif
#ifndef SOKOL_UNREACHABLE
    #define SOKOL_UNREACHABLE SOKOL_ASSERT(false)
#endif

_SOKOL_PRIVATE sg_pixel_format _sglue_to_sgpixelformat(sapp_pixel_format fmt) {
    switch (fmt) {
        case SAPP_PIXELFORMAT_NONE: return SG_PIXELFORMAT_NONE;
        case SAPP_PIXELFORMAT_RGBA8: return SG_PIXELFORMAT_RGBA8;
        case SAPP_PIXELFORMAT_SRGB8A8: return SG_PIXELFORMAT_SRGB8A8;
        case SAPP_PIXELFORMAT_BGRA8: return SG_PIXELFORMAT_BGRA8;
        case SAPP_PIXELFORMAT_DEPTH_STENCIL: return SG_PIXELFORMAT_DEPTH_STENCIL;
        case SAPP_PIXELFORMAT_DEPTH: return SG_PIXELFORMAT_DEPTH;
        case SAPP_PIXELFORMAT_SBGRA8: // FIXME!
        default:
            SOKOL_UNREACHABLE;
            return SG_PIXELFORMAT_NONE;
    }
}

SOKOL_API_IMPL sg_environment sglue_environment(void) {
    sg_environment res;
    memset(&res, 0, sizeof(res));
    const sapp_environment env = sapp_get_environment();
    res.defaults.color_format = _sglue_to_sgpixelformat(env.defaults.color_format);
    res.defaults.depth_format = _sglue_to_sgpixelformat(env.defaults.depth_format);
    res.defaults.sample_count = env.defaults.sample_count;
    res.metal.device = env.metal.device;
    res.d3d11.device = env.d3d11.device;
    res.d3d11.device_context = env.d3d11.device_context;
    res.wgpu.device = env.wgpu.device;
    res.vulkan.physical_device = env.vulkan.physical_device;
    res.vulkan.device = env.vulkan.device;
    res.vulkan.queue = env.vulkan.queue;
    res.vulkan.queue_family_index = env.vulkan.queue_family_index;
    return res;
}

SOKOL_API_IMPL sg_swapchain sglue_swapchain(void) {
    sg_swapchain res;
    memset(&res, 0, sizeof(res));
    const sapp_swapchain sc = sapp_get_swapchain();
    res.width = sc.width;
    res.height = sc.height;
    res.sample_count = sc.sample_count;
    res.color_format = _sglue_to_sgpixelformat(sc.color_format);
    res.depth_format = _sglue_to_sgpixelformat(sc.depth_format);
    res.metal.current_drawable = sc.metal.current_drawable;
    res.metal.depth_stencil_texture = sc.metal.depth_stencil_texture;
    res.metal.msaa_color_texture = sc.metal.msaa_color_texture;
    res.d3d11.render_view = sc.d3d11.render_view;
    res.d3d11.resolve_view = sc.d3d11.resolve_view;
    res.d3d11.depth_stencil_view = sc.d3d11.depth_stencil_view;
    res.wgpu.render_view = sc.wgpu.render_view;
    res.wgpu.resolve_view = sc.wgpu.resolve_view;
    res.wgpu.depth_stencil_view = sc.wgpu.depth_stencil_view;
    res.vulkan.render_image = sc.vulkan.render_image;
    res.vulkan.render_view = sc.vulkan.render_view;
    res.vulkan.resolve_image = sc.vulkan.resolve_image;
    res.vulkan.resolve_view = sc.vulkan.resolve_view;
    res.vulkan.depth_stencil_image = sc.vulkan.depth_stencil_image;
    res.vulkan.depth_stencil_view = sc.vulkan.depth_stencil_view;
    res.vulkan.render_finished_semaphore = sc.vulkan.render_finished_semaphore;
    res.vulkan.present_complete_semaphore = sc.vulkan.present_complete_semaphore;
    res.gl.framebuffer = sc.gl.framebuffer;
    return res;
}

#endif /* SOKOL_GLUE_IMPL */
