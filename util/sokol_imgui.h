#pragma once
/*
    sokol_imgui.h -- drop-in Dear ImGui renderer for sokol_gfx.h

    Do this:
        #define SOKOL_IMGUI_IMPL
    before you include this file in *one* C or C++ file to create the
    implementation.

    The following defines are used by the implementation to select the
    platform-specific embedded shader code (these are the same defines as
    used by sokol_gfx.h and sokol_app.h):

    SOKOL_GLCORE33
    SOKOL_GLES2
    SOKOL_GLES3
    SOKOL_D3D11
    SOKOL_METAL

    Optionally provide the following defines to further configure 

    SOKOL_IMGUI_NO_SOKOL_APP    - don't depend on sokol_app.h (see below for details)
    SOKOL_IMGUI_NO_INCLUDES     - don't include imgui.h, sokol_gfx.h and sokol_app.h
                                  (instead include those yourself)

    ...and finally, optionally provide the following macros to
    override defaults:

    SOKOL_ASSERT(c)     - your own assert macro (default: assert(c))
    SOKOL_MALLOC(s)     - your own malloc function (default: malloc(s))
    SOKOL_FREE(p)       - your own free function (default: free(p))
    SOKOL_API_DECL      - public function declaration prefix (default: extern)
    SOKOL_API_IMPL      - public function implementation prefix (default: -)

    FEATURE OVERVIEW:
    =================
    sokol_imgui.h implements the initialization, rendering and event-handling
    code for Dear ImGui (https://github.com/ocornut/imgui) on top of
    sokol_gfx.h and (optionally) sokol_app.h.

    The sokol_app.h dependency is optional and used for input event handling.
    If you only use sokol_gfx.h but not sokol_app.h in your application,
    just define SOKOL_IMGUI_NO_SOKOL_APP before including the implementation
    of sokol_imgui.h, this will remove any dependency on sokol_app.h, and
    you must feed input events into Dear ImGui yourself.

    sokol_imgui.h is not thread-safe, all calls must be made from the
    same thread where sokol_gfx.h is running.

    HOWTO:
    ======

    --- To initialize sokol-imgui call:

        simgui_setup(const simgui_desc* desc)

        This will initialize Dear ImGui and create sokol-gfx resources
        (two buffers for vertices and indices, a font texture and a pipeline-
        state-object).

        Use the following simgui_desc members to configure behaviour:

            int max_vertices
                The maximum number of vertices used for UI rendering, default is 65536.
                sokol-imgui will do 2 memory allocations, one for vertices
                of the size (max_vertices * sizeof(ImDrawVert)), and one
                for indices of the size (3 * max_vertices * sizeof(uint16_t)),
                and it will create two dynamic sokol-gfx buffers.

            sg_pixel_format color_format
                The color pixel format of the render pass where the UI
                will be rendered. The default is SG_PIXELFORMAT_RGBA8

            sg_pixel_format depth_format
                The depth-buffer pixel format of the render pass where
                the UI will be rendered. The default is SG_PIXELFORMAT_DEPTHSTENCIL.

            int sample_count
                The MSAA sample-count of the render pass where the UI
                will be rendered. The default is 1.

            bool no_default_font
                Set this to true if you don't want to use ImGui's default
                font. In this case you need to initialize the font
                yourself after simgui_setup() is called.

    --- At the start of a frame, call:

        simgui_newframe(int width, int height, double delta_time)

        'width' and 'height' are the dimensions of the rendering surface,
        passed to ImGui::GetIO().DisplaySize.

        'delta_time' is the frame duration passed to ImGui::GetIO().DeltaTime.
        
        For example, if you're using sokol_app.h and render to the
        default framebuffer:

        simgui_newframe(sapp_width(), sapp_height(), delta_time);

    --- at the end of the frame, before the sg_end_pass() where you
        want to render the UI, call:

        simgui_render()

        This will first call ImGui::Render(), and then render ImGui's draw list
        through sokol_gfx.h

    --- if you're using sokol_app.h, from inside the sokol_app.h event callback,
        call:

        bool simgui_handle_event(const sapp_event* ev);

        The return value is the value of ImGui::GetIO().WantCaptureKeyboard,
        if this is true, you might want to skip keyboard input handling
        in your own event handler.

    --- finally, on application shutdown, call

        simgui_shutdown()

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