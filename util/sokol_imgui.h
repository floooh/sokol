#ifndef SOKOL_IMGUI_INCLUDED
/*
    sokol_imgui.h -- drop-in Dear ImGui renderer/event-handler for sokol_gfx.h

    Project URL: https://github.com/floooh/sokol

    Do this:

        #define SOKOL_IMGUI_IMPL

    before you include this file in *one* C or C++ file to create the
    implementation.

    NOTE that the implementation can be compiled either as C++ or as C.
    When compiled as C++, sokol_imgui.h will directly call into the
    Dear ImGui C++ API. When compiled as C, sokol_imgui.h will call
    cimgui.h functions instead.

    NOTE that the formerly separate header sokol_cimgui.h has been
    merged into sokol_imgui.h

    The following defines are used by the implementation to select the
    platform-specific embedded shader code (these are the same defines as
    used by sokol_gfx.h and sokol_app.h):

    SOKOL_GLCORE33
    SOKOL_GLES2
    SOKOL_GLES3
    SOKOL_D3D11
    SOKOL_METAL

    Optionally provide the following configuration defines before including the
    implementation:

    SOKOL_IMGUI_NO_SOKOL_APP    - don't depend on sokol_app.h (see below for details)

    Optionally provide the following macros before including the implementation
    to override defaults:

    SOKOL_ASSERT(c)     - your own assert macro (default: assert(c))
    SOKOL_API_DECL      - public function declaration prefix (default: extern)
    SOKOL_API_IMPL      - public function implementation prefix (default: -)

    If sokol_imgui.h is compiled as a DLL, define the following before
    including the declaration or implementation:

    SOKOL_DLL

    On Windows, SOKOL_DLL will define SOKOL_API_DECL as __declspec(dllexport)
    or __declspec(dllimport) as needed.

    Include the following headers before sokol_imgui.h (both before including
    the declaration and implementation):

        sokol_gfx.h
        sokol_app.h     (except SOKOL_IMGUI_NO_SOKOL_APP)

    Additionally, include the following headers before including the
    implementation:

    If the implementation is compiled as C++:
        imgui.h

    If the implementation is compiled as C:
        cimgui.h


    FEATURE OVERVIEW:
    =================
    sokol_imgui.h implements the initialization, rendering and event-handling
    code for Dear ImGui (https://github.com/ocornut/imgui) on top of
    sokol_gfx.h and (optionally) sokol_app.h.

    The sokol_app.h dependency is optional and used for input event handling.
    If you only use sokol_gfx.h but not sokol_app.h in your application,
    define SOKOL_IMGUI_NO_SOKOL_APP before including the implementation
    of sokol_imgui.h, this will remove any dependency to sokol_app.h, but
    you must feed input events into Dear ImGui yourself.

    sokol_imgui.h is not thread-safe, all calls must be made from the
    same thread where sokol_gfx.h is running.

    HOWTO:
    ======

    --- To initialize sokol-imgui, call:

        simgui_setup(const simgui_desc_t* desc)

        This will initialize Dear ImGui and create sokol-gfx resources
        (two buffers for vertices and indices, a font texture and a pipeline-
        state-object).

        Use the following simgui_desc_t members to configure behaviour:

            int max_vertices
                The maximum number of vertices used for UI rendering, default is 65536.
                sokol-imgui will use this to compute the size of the vertex-
                and index-buffers allocated via sokol_gfx.h

            sg_pixel_format color_format
                The color pixel format of the render pass where the UI
                will be rendered. The default is SG_PIXELFORMAT_RGBA8

            sg_pixel_format depth_format
                The depth-buffer pixel format of the render pass where
                the UI will be rendered. The default is SG_PIXELFORMAT_DEPTHSTENCIL.

            int sample_count
                The MSAA sample-count of the render pass where the UI
                will be rendered. The default is 1.

            float dpi_scale
                DPI scaling factor. Set this to the result of sapp_dpi_scale().
                To render in high resolution on a Retina Mac this would
                typically be 2.0. The default value is 1.0

            const char* ini_filename
                Use this path as ImGui::GetIO().IniFilename. By default
                this is 0, so that Dear ImGui will not do any
                filesystem calls.

            bool no_default_font
                Set this to true if you don't want to use ImGui's default
                font. In this case you need to initialize the font
                yourself after simgui_setup() is called.

    --- At the start of a frame, call:

        simgui_new_frame(int width, int height, double delta_time)

        'width' and 'height' are the dimensions of the rendering surface,
        passed to ImGui::GetIO().DisplaySize.

        'delta_time' is the frame duration passed to ImGui::GetIO().DeltaTime.

        For example, if you're using sokol_app.h and render to the
        default framebuffer:

        simgui_new_frame(sapp_width(), sapp_height(), delta_time);

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
#define SOKOL_IMGUI_INCLUDED (1)
#include <stdint.h>
#include <stdbool.h>

#if !defined(SOKOL_GFX_INCLUDED)
#error "Please include sokol_gfx.h before sokol_imgui.h"
#endif
#if !defined(SOKOL_IMGUI_NO_SOKOL_APP) && !defined(SOKOL_APP_INCLUDED)
#error "Please include sokol_app.h before sokol_imgui.h"
#endif

#ifndef SOKOL_API_DECL
#if defined(_WIN32) && defined(SOKOL_DLL) && defined(SOKOL_IMPL)
#define SOKOL_API_DECL __declspec(dllexport)
#elif defined(_WIN32) && defined(SOKOL_DLL)
#define SOKOL_API_DECL __declspec(dllimport)
#else
#define SOKOL_API_DECL extern
#endif
#endif

#ifdef __cplusplus
extern "C" {
#endif

typedef struct simgui_desc_t {
    int max_vertices;
    sg_pixel_format color_format;
    sg_pixel_format depth_format;
    int sample_count;
    float dpi_scale;
    const char* ini_filename;
    bool no_default_font;
} simgui_desc_t;

SOKOL_API_DECL void simgui_setup(const simgui_desc_t* desc);
SOKOL_API_DECL void simgui_new_frame(int width, int height, double delta_time);
SOKOL_API_DECL void simgui_render(void);
#if !defined(SOKOL_IMGUI_NO_SOKOL_APP)
SOKOL_API_DECL bool simgui_handle_event(const sapp_event* ev);
#endif
SOKOL_API_DECL void simgui_shutdown(void);

#ifdef __cplusplus
} /* extern "C" */
#endif
#endif /* SOKOL_IMGUI_INCLUDED */

/*-- IMPLEMENTATION ----------------------------------------------------------*/
#ifdef SOKOL_IMGUI_IMPL
#define SOKOL_IMGUI_IMPL_INCLUDED (1)

#if defined(__cplusplus)
    #if !defined(IMGUI_VERSION)
    #error "Please include imgui.h before the sokol_imgui.h implementation"
    #endif
#else
    #if !defined(CIMGUI_INCLUDED)
    #error "Please include cimgui.h before the sokol_imgui.h implementation"
    #endif
#endif

#include <stddef.h> /* offsetof */
#include <string.h> /* memset */

#ifndef SOKOL_API_IMPL
#define SOKOL_API_IMPL
#endif
#ifndef SOKOL_DEBUG
    #ifndef NDEBUG
        #define SOKOL_DEBUG (1)
    #endif
#endif
#ifndef SOKOL_ASSERT
    #include <assert.h>
    #define SOKOL_ASSERT(c) assert(c)
#endif
#ifndef _SOKOL_PRIVATE
    #if defined(__GNUC__)
        #define _SOKOL_PRIVATE __attribute__((unused)) static
    #else
        #define _SOKOL_PRIVATE static
    #endif
#endif

/* helper macros */
#define _simgui_def(val, def) (((val) == 0) ? (def) : (val))

typedef struct {
    ImVec2 disp_size;
} _simgui_vs_params_t;

typedef struct {
    simgui_desc_t desc;
    sg_buffer vbuf;
    sg_buffer ibuf;
    sg_image img;
    sg_shader shd;
    sg_pipeline pip;
    #if !defined(SOKOL_IMGUI_NO_SOKOL_APP)
    bool btn_down[SAPP_MAX_MOUSEBUTTONS];
    bool btn_up[SAPP_MAX_MOUSEBUTTONS];
    #endif
} _simgui_state_t;
static _simgui_state_t _simgui;

/* embedded shader sources */
#if defined(SOKOL_GLCORE33)
static const char* _simgui_vs_src =
    "#version 330\n"
    "uniform vec2 disp_size;\n"
    "in vec2 position;\n"
    "in vec2 texcoord0;\n"
    "in vec4 color0;\n"
    "out vec2 uv;\n"
    "out vec4 color;\n"
    "void main() {\n"
    "    gl_Position = vec4(((position/disp_size)-0.5)*vec2(2.0,-2.0), 0.5, 1.0);\n"
    "    uv = texcoord0;\n"
    "    color = color0;\n"
    "}\n";
static const char* _simgui_fs_src =
    "#version 330\n"
    "uniform sampler2D tex;\n"
    "in vec2 uv;\n"
    "in vec4 color;\n"
    "out vec4 frag_color;\n"
    "void main() {\n"
    "    frag_color = texture(tex, uv) * color;\n"
    "}\n";
#elif defined(SOKOL_GLES2) || defined(SOKOL_GLES3)
static const char* _simgui_vs_src =
    "uniform vec2 disp_size;\n"
    "attribute vec2 position;\n"
    "attribute vec2 texcoord0;\n"
    "attribute vec4 color0;\n"
    "varying vec2 uv;\n"
    "varying vec4 color;\n"
    "void main() {\n"
    "    gl_Position = vec4(((position/disp_size)-0.5)*vec2(2.0,-2.0), 0.5, 1.0);\n"
    "    uv = texcoord0;\n"
    "    color = color0;\n"
    "}\n";
static const char* _simgui_fs_src =
    "precision mediump float;\n"
    "uniform sampler2D tex;\n"
    "varying vec2 uv;\n"
    "varying vec4 color;\n"
    "void main() {\n"
    "    gl_FragColor = texture2D(tex, uv) * color;\n"
    "}\n";
#elif defined(SOKOL_METAL)
static const char* _simgui_vs_src =
    "#include <metal_stdlib>\n"
    "using namespace metal;\n"
    "struct params_t {\n"
    "  float2 disp_size;\n"
    "};\n"
    "struct vs_in {\n"
    "  float2 pos [[attribute(0)]];\n"
    "  float2 uv [[attribute(1)]];\n"
    "  float4 color [[attribute(2)]];\n"
    "};\n"
    "struct vs_out {\n"
    "  float4 pos [[position]];\n"
    "  float2 uv;\n"
    "  float4 color;\n"
    "};\n"
    "vertex vs_out _main(vs_in in [[stage_in]], constant params_t& params [[buffer(0)]]) {\n"
    "  vs_out out;\n"
    "  out.pos = float4(((in.pos / params.disp_size)-0.5)*float2(2.0,-2.0), 0.5, 1.0);\n"
    "  out.uv = in.uv;\n"
    "  out.color = in.color;\n"
    "  return out;\n"
    "}\n";
static const char* _simgui_fs_src =
    "#include <metal_stdlib>\n"
    "using namespace metal;\n"
    "struct fs_in {\n"
    "  float2 uv;\n"
    "  float4 color;\n"
    "};\n"
    "fragment float4 _main(fs_in in [[stage_in]], texture2d<float> tex [[texture(0)]], sampler smp [[sampler(0)]]) {\n"
    "  return tex.sample(smp, in.uv) * in.color;\n"
    "}\n";
#elif defined(SOKOL_D3D11)
/*
    Shader blobs for D3D11, compiled with:

    fxc.exe /T vs_5_0 /Fh vs.h /Gec /O3 vs.hlsl
    fxc.exe /T ps_5_0 /Fh fs.h /Gec /O3 fs.hlsl

    Vertex shader source:

        cbuffer params {
            float2 disp_size;
        };
        struct vs_in {
            float2 pos: POSITION;
            float2 uv: TEXCOORD0;
            float4 color: COLOR0;
        };
        struct vs_out {
            float2 uv: TEXCOORD0;
            float4 color: COLOR0;
            float4 pos: SV_Position;
        };
        vs_out main(vs_in inp) {
            vs_out outp;
            outp.pos = float4(((inp.pos/disp_size)-0.5)*float2(2.0,-2.0), 0.5, 1.0);
            outp.uv = inp.uv;
            outp.color = inp.color;
            return outp;
        }

    Fragment shader source:

        Texture2D<float4> tex: register(t0);
        sampler smp: register(s0);
        float4 main(float2 uv: TEXCOORD0, float4 color: COLOR0): SV_Target0 {
            return tex.Sample(smp, uv) * color;
        }
*/
static const uint8_t _simgui_vs_bin[] = {
     68,  88,  66,  67, 204, 137,
    115, 177, 245,  67, 161, 195,
     58, 224,  90,  35,  76, 123,
     88, 146,   1,   0,   0,   0,
    244,   3,   0,   0,   5,   0,
      0,   0,  52,   0,   0,   0,
     64,   1,   0,   0, 176,   1,
      0,   0,  36,   2,   0,   0,
     88,   3,   0,   0,  82,  68,
     69,  70,   4,   1,   0,   0,
      1,   0,   0,   0, 100,   0,
      0,   0,   1,   0,   0,   0,
     60,   0,   0,   0,   0,   5,
    254, 255,   0, 145,   0,   0,
    220,   0,   0,   0,  82,  68,
     49,  49,  60,   0,   0,   0,
     24,   0,   0,   0,  32,   0,
      0,   0,  40,   0,   0,   0,
     36,   0,   0,   0,  12,   0,
      0,   0,   0,   0,   0,   0,
     92,   0,   0,   0,   0,   0,
      0,   0,   0,   0,   0,   0,
      0,   0,   0,   0,   0,   0,
      0,   0,   0,   0,   0,   0,
      1,   0,   0,   0,   0,   0,
      0,   0, 112,  97, 114,  97,
    109, 115,   0, 171,  92,   0,
      0,   0,   1,   0,   0,   0,
    124,   0,   0,   0,  16,   0,
      0,   0,   0,   0,   0,   0,
      0,   0,   0,   0, 164,   0,
      0,   0,   0,   0,   0,   0,
      8,   0,   0,   0,   2,   0,
      0,   0, 184,   0,   0,   0,
      0,   0,   0,   0, 255, 255,
    255, 255,   0,   0,   0,   0,
    255, 255, 255, 255,   0,   0,
      0,   0, 100, 105, 115, 112,
     95, 115, 105, 122, 101,   0,
    102, 108, 111,  97, 116,  50,
      0, 171, 171, 171,   1,   0,
      3,   0,   1,   0,   2,   0,
      0,   0,   0,   0,   0,   0,
      0,   0,   0,   0,   0,   0,
      0,   0,   0,   0,   0,   0,
      0,   0,   0,   0,   0,   0,
    174,   0,   0,   0,  77, 105,
     99, 114, 111, 115, 111, 102,
    116,  32,  40,  82,  41,  32,
     72,  76,  83,  76,  32,  83,
    104,  97, 100, 101, 114,  32,
     67, 111, 109, 112, 105, 108,
    101, 114,  32,  49,  48,  46,
     49,   0,  73,  83,  71,  78,
    104,   0,   0,   0,   3,   0,
      0,   0,   8,   0,   0,   0,
     80,   0,   0,   0,   0,   0,
      0,   0,   0,   0,   0,   0,
      3,   0,   0,   0,   0,   0,
      0,   0,   3,   3,   0,   0,
     89,   0,   0,   0,   0,   0,
      0,   0,   0,   0,   0,   0,
      3,   0,   0,   0,   1,   0,
      0,   0,   3,   3,   0,   0,
     98,   0,   0,   0,   0,   0,
      0,   0,   0,   0,   0,   0,
      3,   0,   0,   0,   2,   0,
      0,   0,  15,  15,   0,   0,
     80,  79,  83,  73,  84,  73,
     79,  78,   0,  84,  69,  88,
     67,  79,  79,  82,  68,   0,
     67,  79,  76,  79,  82,   0,
     79,  83,  71,  78, 108,   0,
      0,   0,   3,   0,   0,   0,
      8,   0,   0,   0,  80,   0,
      0,   0,   0,   0,   0,   0,
      0,   0,   0,   0,   3,   0,
      0,   0,   0,   0,   0,   0,
      3,  12,   0,   0,  89,   0,
      0,   0,   0,   0,   0,   0,
      0,   0,   0,   0,   3,   0,
      0,   0,   1,   0,   0,   0,
     15,   0,   0,   0,  95,   0,
      0,   0,   0,   0,   0,   0,
      1,   0,   0,   0,   3,   0,
      0,   0,   2,   0,   0,   0,
     15,   0,   0,   0,  84,  69,
     88,  67,  79,  79,  82,  68,
      0,  67,  79,  76,  79,  82,
      0,  83,  86,  95,  80, 111,
    115, 105, 116, 105, 111, 110,
      0, 171,  83,  72,  69,  88,
     44,   1,   0,   0,  80,   0,
      1,   0,  75,   0,   0,   0,
    106,   8,   0,   1,  89,   0,
      0,   4,  70, 142,  32,   0,
      0,   0,   0,   0,   1,   0,
      0,   0,  95,   0,   0,   3,
     50,  16,  16,   0,   0,   0,
      0,   0,  95,   0,   0,   3,
     50,  16,  16,   0,   1,   0,
      0,   0,  95,   0,   0,   3,
    242,  16,  16,   0,   2,   0,
      0,   0, 101,   0,   0,   3,
     50,  32,  16,   0,   0,   0,
      0,   0, 101,   0,   0,   3,
    242,  32,  16,   0,   1,   0,
      0,   0, 103,   0,   0,   4,
    242,  32,  16,   0,   2,   0,
      0,   0,   1,   0,   0,   0,
    104,   0,   0,   2,   1,   0,
      0,   0,  54,   0,   0,   5,
     50,  32,  16,   0,   0,   0,
      0,   0,  70,  16,  16,   0,
      1,   0,   0,   0,  54,   0,
      0,   5, 242,  32,  16,   0,
      1,   0,   0,   0,  70,  30,
     16,   0,   2,   0,   0,   0,
     14,   0,   0,   8,  50,   0,
     16,   0,   0,   0,   0,   0,
     70,  16,  16,   0,   0,   0,
      0,   0,  70, 128,  32,   0,
      0,   0,   0,   0,   0,   0,
      0,   0,   0,   0,   0,  10,
     50,   0,  16,   0,   0,   0,
      0,   0,  70,   0,  16,   0,
      0,   0,   0,   0,   2,  64,
      0,   0,   0,   0,   0, 191,
      0,   0,   0, 191,   0,   0,
      0,   0,   0,   0,   0,   0,
     56,   0,   0,  10,  50,  32,
     16,   0,   2,   0,   0,   0,
     70,   0,  16,   0,   0,   0,
      0,   0,   2,  64,   0,   0,
      0,   0,   0,  64,   0,   0,
      0, 192,   0,   0,   0,   0,
      0,   0,   0,   0,  54,   0,
      0,   8, 194,  32,  16,   0,
      2,   0,   0,   0,   2,  64,
      0,   0,   0,   0,   0,   0,
      0,   0,   0,   0,   0,   0,
      0,  63,   0,   0, 128,  63,
     62,   0,   0,   1,  83,  84,
     65,  84, 148,   0,   0,   0,
      7,   0,   0,   0,   1,   0,
      0,   0,   0,   0,   0,   0,
      6,   0,   0,   0,   3,   0,
      0,   0,   0,   0,   0,   0,
      0,   0,   0,   0,   1,   0,
      0,   0,   0,   0,   0,   0,
      0,   0,   0,   0,   0,   0,
      0,   0,   0,   0,   0,   0,
      0,   0,   0,   0,   0,   0,
      0,   0,   0,   0,   0,   0,
      0,   0,   0,   0,   0,   0,
      0,   0,   0,   0,   0,   0,
      0,   0,   0,   0,   3,   0,
      0,   0,   0,   0,   0,   0,
      0,   0,   0,   0,   0,   0,
      0,   0,   0,   0,   0,   0,
      0,   0,   0,   0,   0,   0,
      0,   0,   0,   0,   0,   0,
      0,   0,   0,   0,   0,   0,
      0,   0,   0,   0,   0,   0,
      0,   0,   0,   0,   0,   0,
      0,   0,   0,   0,   0,   0,
      0,   0,   0,   0,   0,   0,
      0,   0,   0,   0,   0,   0,
      0,   0,   0,   0
};
static const uint8_t _simgui_fs_bin[] = {
     68,  88,  66,  67, 116,  27,
    191,   2, 170,  79,  42, 154,
     39,  13,  69, 105, 240,  12,
    136,  97,   1,   0,   0,   0,
    176,   2,   0,   0,   5,   0,
      0,   0,  52,   0,   0,   0,
    232,   0,   0,   0,  56,   1,
      0,   0, 108,   1,   0,   0,
     20,   2,   0,   0,  82,  68,
     69,  70, 172,   0,   0,   0,
      0,   0,   0,   0,   0,   0,
      0,   0,   2,   0,   0,   0,
     60,   0,   0,   0,   0,   5,
    255, 255,   0, 145,   0,   0,
    132,   0,   0,   0,  82,  68,
     49,  49,  60,   0,   0,   0,
     24,   0,   0,   0,  32,   0,
      0,   0,  40,   0,   0,   0,
     36,   0,   0,   0,  12,   0,
      0,   0,   0,   0,   0,   0,
    124,   0,   0,   0,   3,   0,
      0,   0,   0,   0,   0,   0,
      0,   0,   0,   0,   0,   0,
      0,   0,   0,   0,   0,   0,
      1,   0,   0,   0,   1,   0,
      0,   0, 128,   0,   0,   0,
      2,   0,   0,   0,   5,   0,
      0,   0,   4,   0,   0,   0,
    255, 255, 255, 255,   0,   0,
      0,   0,   1,   0,   0,   0,
     13,   0,   0,   0, 115, 109,
    112,   0, 116, 101, 120,   0,
     77, 105,  99, 114, 111, 115,
    111, 102, 116,  32,  40,  82,
     41,  32,  72,  76,  83,  76,
     32,  83, 104,  97, 100, 101,
    114,  32,  67, 111, 109, 112,
    105, 108, 101, 114,  32,  49,
     48,  46,  49,   0,  73,  83,
     71,  78,  72,   0,   0,   0,
      2,   0,   0,   0,   8,   0,
      0,   0,  56,   0,   0,   0,
      0,   0,   0,   0,   0,   0,
      0,   0,   3,   0,   0,   0,
      0,   0,   0,   0,   3,   3,
      0,   0,  65,   0,   0,   0,
      0,   0,   0,   0,   0,   0,
      0,   0,   3,   0,   0,   0,
      1,   0,   0,   0,  15,  15,
      0,   0,  84,  69,  88,  67,
     79,  79,  82,  68,   0,  67,
     79,  76,  79,  82,   0, 171,
     79,  83,  71,  78,  44,   0,
      0,   0,   1,   0,   0,   0,
      8,   0,   0,   0,  32,   0,
      0,   0,   0,   0,   0,   0,
      0,   0,   0,   0,   3,   0,
      0,   0,   0,   0,   0,   0,
     15,   0,   0,   0,  83,  86,
     95,  84,  97, 114, 103, 101,
    116,   0, 171, 171,  83,  72,
     69,  88, 160,   0,   0,   0,
     80,   0,   0,   0,  40,   0,
      0,   0, 106,   8,   0,   1,
     90,   0,   0,   3,   0,  96,
     16,   0,   0,   0,   0,   0,
     88,  24,   0,   4,   0, 112,
     16,   0,   0,   0,   0,   0,
     85,  85,   0,   0,  98,  16,
      0,   3,  50,  16,  16,   0,
      0,   0,   0,   0,  98,  16,
      0,   3, 242,  16,  16,   0,
      1,   0,   0,   0, 101,   0,
      0,   3, 242,  32,  16,   0,
      0,   0,   0,   0, 104,   0,
      0,   2,   1,   0,   0,   0,
     69,   0,   0, 139, 194,   0,
      0, 128,  67,  85,  21,   0,
    242,   0,  16,   0,   0,   0,
      0,   0,  70,  16,  16,   0,
      0,   0,   0,   0,  70, 126,
     16,   0,   0,   0,   0,   0,
      0,  96,  16,   0,   0,   0,
      0,   0,  56,   0,   0,   7,
    242,  32,  16,   0,   0,   0,
      0,   0,  70,  14,  16,   0,
      0,   0,   0,   0,  70,  30,
     16,   0,   1,   0,   0,   0,
     62,   0,   0,   1,  83,  84,
     65,  84, 148,   0,   0,   0,
      3,   0,   0,   0,   1,   0,
      0,   0,   0,   0,   0,   0,
      3,   0,   0,   0,   1,   0,
      0,   0,   0,   0,   0,   0,
      0,   0,   0,   0,   1,   0,
      0,   0,   0,   0,   0,   0,
      0,   0,   0,   0,   0,   0,
      0,   0,   0,   0,   0,   0,
      0,   0,   0,   0,   0,   0,
      0,   0,   1,   0,   0,   0,
      0,   0,   0,   0,   0,   0,
      0,   0,   0,   0,   0,   0,
      0,   0,   0,   0,   0,   0,
      0,   0,   0,   0,   0,   0,
      0,   0,   0,   0,   0,   0,
      0,   0,   0,   0,   0,   0,
      0,   0,   0,   0,   0,   0,
      0,   0,   0,   0,   0,   0,
      0,   0,   0,   0,   0,   0,
      0,   0,   0,   0,   0,   0,
      0,   0,   0,   0,   0,   0,
      0,   0,   0,   0,   0,   0,
      0,   0,   0,   0,   0,   0,
      0,   0,   0,   0,   0,   0,
      0,   0,   0,   0
};
#else
#error "sokol_imgui.h: No sokol_gfx.h backend selected (SOKOL_GLCORE33, SOKOL_GLES2, SOKOL_GLES3, SOKOL_D3D11 or SOKOL_METAL)"
#endif

SOKOL_API_IMPL void simgui_setup(const simgui_desc_t* desc) {
    SOKOL_ASSERT(desc);
    memset(&_simgui, 0, sizeof(_simgui));
    _simgui.desc = *desc;
    _simgui.desc.max_vertices = _simgui_def(_simgui.desc.max_vertices, 65536);
    _simgui.desc.dpi_scale = _simgui_def(_simgui.desc.dpi_scale, 1.0f);
    /* can keep color_format, depth_format and sample_count as is,
       since sokol_gfx.h will do its own default-value handling
    */

    /* initialize Dear ImGui */
    #if defined(__cplusplus)
        ImGui::CreateContext();
        ImGui::StyleColorsDark();
        ImGuiIO* io = &ImGui::GetIO();
        if (!_simgui.desc.no_default_font) {
            io->Fonts->AddFontDefault();
        }
        io->IniFilename = _simgui.desc.ini_filename;
    #else
        igCreateContext(NULL);
        igStyleColorsDark(igGetStyle());
        ImGuiIO* io = igGetIO();
        if (!_simgui.desc.no_default_font) {
            ImFontAtlas_AddFontDefault(io->Fonts, NULL);
        }
        io->IniFilename = _simgui.desc.ini_filename;
    #endif
    #if !defined(SOKOL_IMGUI_NO_SOKOL_APP)
        io->KeyMap[ImGuiKey_Tab] = SAPP_KEYCODE_TAB;
        io->KeyMap[ImGuiKey_LeftArrow] = SAPP_KEYCODE_LEFT;
        io->KeyMap[ImGuiKey_RightArrow] = SAPP_KEYCODE_RIGHT;
        io->KeyMap[ImGuiKey_UpArrow] = SAPP_KEYCODE_UP;
        io->KeyMap[ImGuiKey_DownArrow] = SAPP_KEYCODE_DOWN;
        io->KeyMap[ImGuiKey_PageUp] = SAPP_KEYCODE_PAGE_UP;
        io->KeyMap[ImGuiKey_PageDown] = SAPP_KEYCODE_PAGE_DOWN;
        io->KeyMap[ImGuiKey_Home] = SAPP_KEYCODE_HOME;
        io->KeyMap[ImGuiKey_End] = SAPP_KEYCODE_END;
        io->KeyMap[ImGuiKey_Delete] = SAPP_KEYCODE_DELETE;
        io->KeyMap[ImGuiKey_Backspace] = SAPP_KEYCODE_BACKSPACE;
        io->KeyMap[ImGuiKey_Space] = SAPP_KEYCODE_SPACE;
        io->KeyMap[ImGuiKey_Enter] = SAPP_KEYCODE_ENTER;
        io->KeyMap[ImGuiKey_Escape] = SAPP_KEYCODE_ESCAPE;
        io->KeyMap[ImGuiKey_A] = SAPP_KEYCODE_A;
        io->KeyMap[ImGuiKey_C] = SAPP_KEYCODE_C;
        io->KeyMap[ImGuiKey_V] = SAPP_KEYCODE_V;
        io->KeyMap[ImGuiKey_X] = SAPP_KEYCODE_X;
        io->KeyMap[ImGuiKey_Y] = SAPP_KEYCODE_Y;
        io->KeyMap[ImGuiKey_Z] = SAPP_KEYCODE_Z;
    #endif

    /* create sokol-gfx resources */
    sg_push_debug_group("sokol-imgui");

    /* NOTE: since we're in C++ mode here we can't use C99 designated init */
    sg_buffer_desc vb_desc;
    memset(&vb_desc, 0, sizeof(vb_desc));
    vb_desc.usage = SG_USAGE_STREAM;
    vb_desc.size = _simgui.desc.max_vertices * sizeof(ImDrawVert);
    vb_desc.label = "sokol-imgui-vertices";
    _simgui.vbuf = sg_make_buffer(&vb_desc);

    sg_buffer_desc ib_desc;
    memset(&ib_desc, 0, sizeof(ib_desc));
    ib_desc.type = SG_BUFFERTYPE_INDEXBUFFER;
    ib_desc.usage = SG_USAGE_STREAM;
    ib_desc.size = _simgui.desc.max_vertices * 3 * sizeof(uint16_t);
    ib_desc.label = "sokol-imgui-indices";
    _simgui.ibuf = sg_make_buffer(&ib_desc);

    /* default font texture */
    if (!_simgui.desc.no_default_font) {
        unsigned char* font_pixels;
        int font_width, font_height;
        #if defined(__cplusplus)
            io->Fonts->GetTexDataAsRGBA32(&font_pixels, &font_width, &font_height);
        #else
            int bytes_per_pixel;
            ImFontAtlas_GetTexDataAsRGBA32(io->Fonts, &font_pixels, &font_width, &font_height, &bytes_per_pixel);
        #endif
        sg_image_desc img_desc;
        memset(&img_desc, 0, sizeof(img_desc));
        img_desc.width = font_width;
        img_desc.height = font_height;
        img_desc.pixel_format = SG_PIXELFORMAT_RGBA8;
        img_desc.wrap_u = SG_WRAP_CLAMP_TO_EDGE;
        img_desc.wrap_v = SG_WRAP_CLAMP_TO_EDGE;
        img_desc.min_filter = SG_FILTER_LINEAR;
        img_desc.mag_filter = SG_FILTER_LINEAR;
        img_desc.content.subimage[0][0].ptr = font_pixels;
        img_desc.content.subimage[0][0].size = font_width * font_height * sizeof(uint32_t);
        img_desc.label = "sokol-imgui-font";
        _simgui.img = sg_make_image(&img_desc);
        io->Fonts->TexID = (ImTextureID)(uintptr_t) _simgui.img.id;
    }

    /* shader object for using the embedded shader source (or bytecode) */
    sg_shader_desc shd_desc;
    memset(&shd_desc, 0, sizeof(shd_desc));
    sg_shader_uniform_block_desc* ub = &shd_desc.vs.uniform_blocks[0];
    ub->size = sizeof(_simgui_vs_params_t);
    ub->uniforms[0].name = "disp_size";
    ub->uniforms[0].type = SG_UNIFORMTYPE_FLOAT2;
    shd_desc.attrs[0].name = "position";
    shd_desc.attrs[0].sem_name = "POSITION";
    shd_desc.attrs[1].name = "texcoord0";
    shd_desc.attrs[1].sem_name  = "TEXCOORD";
    shd_desc.attrs[2].name  = "color0";
    shd_desc.attrs[2].sem_name  = "COLOR";
    shd_desc.fs.images[0].name = "tex";
    shd_desc.fs.images[0].type = SG_IMAGETYPE_2D;
    #if defined(SOKOL_D3D11)
        shd_desc.vs.byte_code = _simgui_vs_bin;
        shd_desc.vs.byte_code_size = sizeof(_simgui_vs_bin);
        shd_desc.fs.byte_code = _simgui_fs_bin;
        shd_desc.fs.byte_code_size = sizeof(_simgui_fs_bin);
    #else
        shd_desc.vs.source = _simgui_vs_src;
        shd_desc.fs.source = _simgui_fs_src;
    #endif
    shd_desc.label = "sokol-imgui-shader";
    _simgui.shd = sg_make_shader(&shd_desc);

    /* pipeline object for imgui rendering */
    sg_pipeline_desc pip_desc;
    memset(&pip_desc, 0, sizeof(pip_desc));
    pip_desc.layout.buffers[0].stride = sizeof(ImDrawVert);
    {
        sg_vertex_attr_desc* attr = &pip_desc.layout.attrs[0];
        attr->offset = offsetof(ImDrawVert, pos);
        attr->format = SG_VERTEXFORMAT_FLOAT2;
    }
    {
        sg_vertex_attr_desc* attr = &pip_desc.layout.attrs[1];
        attr->offset = offsetof(ImDrawVert, uv);
        attr->format = SG_VERTEXFORMAT_FLOAT2;
    }
    {
        sg_vertex_attr_desc* attr = &pip_desc.layout.attrs[2];
        attr->offset = offsetof(ImDrawVert, col);
        attr->format = SG_VERTEXFORMAT_UBYTE4N;
    }
    pip_desc.shader = _simgui.shd;
    pip_desc.index_type = SG_INDEXTYPE_UINT16;
    pip_desc.blend.enabled = true;
    pip_desc.blend.src_factor_rgb = SG_BLENDFACTOR_SRC_ALPHA;
    pip_desc.blend.dst_factor_rgb = SG_BLENDFACTOR_ONE_MINUS_SRC_ALPHA;
    pip_desc.blend.color_write_mask = SG_COLORMASK_RGB;
    pip_desc.blend.color_format = _simgui.desc.color_format;
    pip_desc.blend.depth_format = _simgui.desc.depth_format;
    pip_desc.rasterizer.sample_count = _simgui.desc.sample_count;
    pip_desc.label = "sokol-imgui-pipeline";
    _simgui.pip = sg_make_pipeline(&pip_desc);

    sg_pop_debug_group();
}

SOKOL_API_IMPL void simgui_shutdown(void) {
    #if defined(__cplusplus)
        ImGui::DestroyContext();
    #else
        igDestroyContext(0);
    #endif
    /* NOTE: it's valid to call the destroy funcs with SG_INVALID_ID */
    sg_destroy_pipeline(_simgui.pip);
    sg_destroy_shader(_simgui.shd);
    sg_destroy_image(_simgui.img);
    sg_destroy_buffer(_simgui.ibuf);
    sg_destroy_buffer(_simgui.vbuf);
}

SOKOL_API_IMPL void simgui_new_frame(int width, int height, double delta_time) {
    #if defined(__cplusplus)
        ImGuiIO* io = &ImGui::GetIO();
    #else
        ImGuiIO* io = igGetIO();
    #endif
    io->DisplaySize.x = ((float) width) / _simgui.desc.dpi_scale;
    io->DisplaySize.y = ((float) height) / _simgui.desc.dpi_scale;
    io->DeltaTime = (float) delta_time;
    #if !defined(SOKOL_IMGUI_NO_SOKOL_APP)
    for (int i = 0; i < SAPP_MAX_MOUSEBUTTONS; i++) {
        if (_simgui.btn_down[i]) {
            _simgui.btn_down[i] = false;
            io->MouseDown[i] = true;
        }
        else if (_simgui.btn_up[i]) {
            _simgui.btn_up[i] = false;
            io->MouseDown[i] = false;
        }
    }
    if (io->WantTextInput && !sapp_keyboard_shown()) {
        sapp_show_keyboard(true);
    }
    if (!io->WantTextInput && sapp_keyboard_shown()) {
        sapp_show_keyboard(false);
    }
    #endif
    #if defined(__cplusplus)
        ImGui::NewFrame();
    #else
        igNewFrame();
    #endif
}

SOKOL_API_IMPL void simgui_render(void) {
    #if defined(__cplusplus)
        ImGui::Render();
        ImDrawData* draw_data = ImGui::GetDrawData();
        ImGuiIO* io = &ImGui::GetIO();
    #else
        igRender();
        ImDrawData* draw_data = igGetDrawData();
        ImGuiIO* io = igGetIO();
    #endif
    if (0 == draw_data) {
        return;
    }
    if (draw_data->CmdListsCount == 0) {
        return;
    }

    /* render the ImGui command list */
    sg_push_debug_group("sokol-imgui");

    const float dpi_scale = _simgui.desc.dpi_scale;
    const int fb_width = (const int) (io->DisplaySize.x * dpi_scale);
    const int fb_height = (const int) (io->DisplaySize.y * dpi_scale);
    sg_apply_viewport(0, 0, fb_width, fb_height, true);
    sg_apply_scissor_rect(0, 0, fb_width, fb_height, true);

    sg_apply_pipeline(_simgui.pip);
    _simgui_vs_params_t vs_params;
    vs_params.disp_size.x = io->DisplaySize.x;
    vs_params.disp_size.y = io->DisplaySize.y;
    sg_apply_uniforms(SG_SHADERSTAGE_VS, 0, &vs_params, sizeof(vs_params));
    sg_bindings bind;
    memset(&bind, 0, sizeof(bind));
    bind.vertex_buffers[0] = _simgui.vbuf;
    bind.index_buffer = _simgui.ibuf;
    ImTextureID tex_id = io->Fonts->TexID;
    bind.fs_images[0].id = (uint32_t)(uintptr_t)tex_id;
    for (int cl_index = 0; cl_index < draw_data->CmdListsCount; cl_index++) {
        ImDrawList* cl = draw_data->CmdLists[cl_index];

        /* append vertices and indices to buffers, record start offsets in draw state */
        #if defined(__cplusplus)
            const int vtx_size = cl->VtxBuffer.size() * sizeof(ImDrawVert);
            const int idx_size = cl->IdxBuffer.size() * sizeof(ImDrawIdx);
            const ImDrawVert* vtx_ptr = &cl->VtxBuffer.front();
            const ImDrawIdx* idx_ptr = &cl->IdxBuffer.front();
        #else
            const int vtx_size = cl->VtxBuffer.Size * sizeof(ImDrawVert);
            const int idx_size = cl->IdxBuffer.Size * sizeof(ImDrawIdx);
            const ImDrawVert* vtx_ptr = cl->VtxBuffer.Data;
            const ImDrawIdx* idx_ptr = cl->IdxBuffer.Data;
        #endif
        const int vb_offset = sg_append_buffer(bind.vertex_buffers[0], vtx_ptr, vtx_size);
        const int ib_offset = sg_append_buffer(bind.index_buffer, idx_ptr, idx_size);
        /* don't render anything if the buffer is in overflow state (this is also
            checked internally in sokol_gfx, draw calls that attempt to draw with
            overflowed buffers will be silently dropped)
        */
        if (sg_query_buffer_overflow(bind.vertex_buffers[0]) ||
            sg_query_buffer_overflow(bind.index_buffer))
        {
            break;
        }
        bind.vertex_buffer_offsets[0] = vb_offset;
        bind.index_buffer_offset = ib_offset;
        sg_apply_bindings(&bind);

        int base_element = 0;
        #if defined(__cplusplus)
            const int num_cmds = cl->CmdBuffer.size();
        #else
            const int num_cmds = cl->CmdBuffer.Size;
        #endif
        for (int cmd_index = 0; cmd_index < num_cmds; cmd_index++) {
            ImDrawCmd* pcmd = &cl->CmdBuffer.Data[cmd_index];
            if (pcmd->UserCallback) {
                pcmd->UserCallback(cl, pcmd);
            }
            else {
                if (tex_id != pcmd->TextureId) {
                    tex_id = pcmd->TextureId;
                    bind.fs_images[0].id = (uint32_t)(uintptr_t)tex_id;
                    sg_apply_bindings(&bind);
                }
                const int scissor_x = (int) (pcmd->ClipRect.x * dpi_scale);
                const int scissor_y = (int) (pcmd->ClipRect.y * dpi_scale);
                const int scissor_w = (int) ((pcmd->ClipRect.z - pcmd->ClipRect.x) * dpi_scale);
                const int scissor_h = (int) ((pcmd->ClipRect.w - pcmd->ClipRect.y) * dpi_scale);
                sg_apply_scissor_rect(scissor_x, scissor_y, scissor_w, scissor_h, true);
                sg_draw(base_element, pcmd->ElemCount, 1);
            }
            base_element += pcmd->ElemCount;
        }
    }
    sg_apply_viewport(0, 0, fb_width, fb_height, true);
    sg_apply_scissor_rect(0, 0, fb_width, fb_height, true);
    sg_pop_debug_group();
}

#if !defined(SOKOL_IMGUI_NO_SOKOL_APP)
SOKOL_API_IMPL bool simgui_handle_event(const sapp_event* ev) {
    const float dpi_scale = _simgui.desc.dpi_scale;
    #if defined(__cplusplus)
        ImGuiIO* io = &ImGui::GetIO();
    #else
        ImGuiIO* io = igGetIO();
    #endif
    io->KeyAlt = (ev->modifiers & SAPP_MODIFIER_ALT) != 0;
    io->KeyCtrl = (ev->modifiers & SAPP_MODIFIER_CTRL) != 0;
    io->KeyShift = (ev->modifiers & SAPP_MODIFIER_SHIFT) != 0;
    io->KeySuper = (ev->modifiers & SAPP_MODIFIER_SUPER) != 0;
    switch (ev->type) {
        case SAPP_EVENTTYPE_MOUSE_DOWN:
            io->MousePos.x = ev->mouse_x / dpi_scale;
            io->MousePos.y = ev->mouse_y / dpi_scale;
            if (ev->mouse_button < 3) {
                _simgui.btn_down[ev->mouse_button] = true;
            }
            break;
        case SAPP_EVENTTYPE_MOUSE_UP:
            io->MousePos.x = ev->mouse_x / dpi_scale;
            io->MousePos.y = ev->mouse_y / dpi_scale;
            if (ev->mouse_button < 3) {
                _simgui.btn_up[ev->mouse_button] = true;
            }
            break;
        case SAPP_EVENTTYPE_MOUSE_MOVE:
            io->MousePos.x = ev->mouse_x / dpi_scale;
            io->MousePos.y = ev->mouse_y / dpi_scale;
            break;
        case SAPP_EVENTTYPE_MOUSE_ENTER:
        case SAPP_EVENTTYPE_MOUSE_LEAVE:
            for (int i = 0; i < 3; i++) {
                _simgui.btn_down[i] = false;
                _simgui.btn_up[i] = false;
                io->MouseDown[i] = false;
            }
            break;
        case SAPP_EVENTTYPE_MOUSE_SCROLL:
            io->MouseWheelH = ev->scroll_x;
            io->MouseWheel = ev->scroll_y;
            break;
        case SAPP_EVENTTYPE_TOUCHES_BEGAN:
            _simgui.btn_down[0] = true;
            io->MousePos.x = ev->touches[0].pos_x / dpi_scale;
            io->MousePos.y = ev->touches[0].pos_y / dpi_scale;
            break;
        case SAPP_EVENTTYPE_TOUCHES_MOVED:
            io->MousePos.x = ev->touches[0].pos_x / dpi_scale;
            io->MousePos.y = ev->touches[0].pos_y / dpi_scale;
            break;
        case SAPP_EVENTTYPE_TOUCHES_ENDED:
            _simgui.btn_up[0] = true;
            io->MousePos.x = ev->touches[0].pos_x / dpi_scale;
            io->MousePos.y = ev->touches[0].pos_y / dpi_scale;
            break;
        case SAPP_EVENTTYPE_TOUCHES_CANCELLED:
            _simgui.btn_up[0] = _simgui.btn_down[0] = false;
            break;
        case SAPP_EVENTTYPE_KEY_DOWN:
            io->KeysDown[ev->key_code] = true;
            break;
        case SAPP_EVENTTYPE_KEY_UP:
            io->KeysDown[ev->key_code] = false;
            break;
        case SAPP_EVENTTYPE_CHAR:
            #if defined(__cplusplus)
                io->AddInputCharacter((ImWchar)ev->char_code);
            #else
                ImGuiIO_AddInputCharacter(io, (ImWchar)ev->char_code);
            #endif
            break;
        default:
            break;
    }
    return io->WantCaptureKeyboard;
}
#endif

#endif /* SOKOL_IMPL */
