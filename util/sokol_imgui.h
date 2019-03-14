#pragma once
/*
    sokol_imgui.h -- drop-in Dear ImGui renderer/event-handler for sokol_gfx.h

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

    ...and finally, optionally provide the following macros to
    override defaults:

    SOKOL_ASSERT(c)     - your own assert macro (default: assert(c))
    SOKOL_API_DECL      - public function declaration prefix (default: extern)
    SOKOL_API_IMPL      - public function implementation prefix (default: -)

    Include the following headers before sokol_imgui.h:

        sokol_gfx.h
        sokol_app.h     (except SOKOL_IMGUI_NO_SOKOL_APP)

    Additionally, include the following headers before the sokol_imgui.h
    implementation:

        imgui.h

    Note that the sokol_imgui.h *implementation* must be compiled as C++
    (since Dear ImGui has a C++ API).

    FIXME: replace embedded Metal and D3D11 shader sources with
    precompiled bytecode blobs.

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

        simgui_setup(const simgui_desc_t* desc)

        This will initialize Dear ImGui and create sokol-gfx resources
        (two buffers for vertices and indices, a font texture and a pipeline-
        state-object).

        Use the following simgui_desc members to configure behaviour:

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
                The default value is 1.0

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
    #define SOKOL_API_DECL extern
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

/*-- IMPLEMENTATION ----------------------------------------------------------*/
#ifdef SOKOL_IMGUI_IMPL
#define SOKOL_IMGUI_IMPL_INCLUDED (1)

#if !defined(IMGUI_VERSION)
#error "Please include imgui.h before the sokol_imgui.h implementation"
#endif
#if !defined(__cplusplus)
#error "The sokol_imgui.h implementation must be compiled as C++"
#endif

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
static const char* _simgui_vs_src =
    "cbuffer params {\n"
    "  float2 disp_size;\n"
    "};\n"
    "struct vs_in {\n"
    "  float2 pos: POSITION;\n"
    "  float2 uv: TEXCOORD0;\n"
    "  float4 color: COLOR0;\n"
    "};\n"
    "struct vs_out {\n"
    "  float2 uv: TEXCOORD0;\n"
    "  float4 color: COLOR0;\n"
    "  float4 pos: SV_Position;\n"
    "};\n"
    "vs_out main(vs_in inp) {\n"
    "  vs_out outp;\n"
    "  outp.pos = float4(((inp.pos/disp_size)-0.5)*float2(2.0,-2.0), 0.5, 1.0);\n"
    "  outp.uv = inp.uv;\n"
    "  outp.color = inp.color;\n"
    "  return outp;\n"
    "}\n";
static const char* _simgui_fs_src =
    "Texture2D<float4> tex: register(t0);\n"
    "sampler smp: register(s0);\n"
    "float4 main(float2 uv: TEXCOORD0, float4 color: COLOR0): SV_Target0 {\n"
    "  return tex.Sample(smp, uv) * color;\n"
    "}\n";
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
    ImGui::CreateContext();
    ImGui::StyleColorsDark();
    auto& io = ImGui::GetIO();
    if (!_simgui.desc.no_default_font) {
        io.Fonts->AddFontDefault();
    }
    io.IniFilename = _simgui.desc.ini_filename;
    #if !defined(SOKOL_IMGUI_NO_SOKOL_APP)
        io.KeyMap[ImGuiKey_Tab] = SAPP_KEYCODE_TAB;
        io.KeyMap[ImGuiKey_LeftArrow] = SAPP_KEYCODE_LEFT;
        io.KeyMap[ImGuiKey_RightArrow] = SAPP_KEYCODE_RIGHT;
        io.KeyMap[ImGuiKey_UpArrow] = SAPP_KEYCODE_UP;
        io.KeyMap[ImGuiKey_DownArrow] = SAPP_KEYCODE_DOWN;
        io.KeyMap[ImGuiKey_PageUp] = SAPP_KEYCODE_PAGE_UP;
        io.KeyMap[ImGuiKey_PageDown] = SAPP_KEYCODE_PAGE_DOWN;
        io.KeyMap[ImGuiKey_Home] = SAPP_KEYCODE_HOME;
        io.KeyMap[ImGuiKey_End] = SAPP_KEYCODE_END;
        io.KeyMap[ImGuiKey_Delete] = SAPP_KEYCODE_DELETE;
        io.KeyMap[ImGuiKey_Backspace] = SAPP_KEYCODE_BACKSPACE;
        io.KeyMap[ImGuiKey_Space] = SAPP_KEYCODE_SPACE;
        io.KeyMap[ImGuiKey_Enter] = SAPP_KEYCODE_ENTER;
        io.KeyMap[ImGuiKey_Escape] = SAPP_KEYCODE_ESCAPE;
        io.KeyMap[ImGuiKey_A] = SAPP_KEYCODE_A;
        io.KeyMap[ImGuiKey_C] = SAPP_KEYCODE_C;
        io.KeyMap[ImGuiKey_V] = SAPP_KEYCODE_V;
        io.KeyMap[ImGuiKey_X] = SAPP_KEYCODE_X;
        io.KeyMap[ImGuiKey_Y] = SAPP_KEYCODE_Y;
        io.KeyMap[ImGuiKey_Z] = SAPP_KEYCODE_Z;
    #endif

    /* create sokol-gfx resources */
    sg_push_debug_group("sokol-imgui");

    /* NOTE: since we're in C++ mode here we can't use C99 designated init */
    sg_buffer_desc vb_desc = { };
    vb_desc.usage = SG_USAGE_STREAM;
    vb_desc.size = _simgui.desc.max_vertices * sizeof(ImDrawVert);
    vb_desc.label = "sokol-imgui-vertices";
    _simgui.vbuf = sg_make_buffer(&vb_desc);

    sg_buffer_desc ib_desc = { };
    ib_desc.type = SG_BUFFERTYPE_INDEXBUFFER;
    ib_desc.usage = SG_USAGE_STREAM;
    ib_desc.size = _simgui.desc.max_vertices * 3 * sizeof(uint16_t);
    ib_desc.label = "sokol-imgui-indices";
    _simgui.ibuf = sg_make_buffer(&ib_desc);

    /* default font texture */
    if (!_simgui.desc.no_default_font) {
        unsigned char* font_pixels;
        int font_width, font_height;
        io.Fonts->GetTexDataAsRGBA32(&font_pixels, &font_width, &font_height);
        sg_image_desc img_desc = { };
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
        io.Fonts->TexID = (ImTextureID)(uintptr_t) _simgui.img.id;
    }

    /* shader object for imgui rendering */
    sg_shader_desc shd_desc = { };
    auto& ub = shd_desc.vs.uniform_blocks[0];
    ub.size = sizeof(_simgui_vs_params_t);
    ub.uniforms[0].name = "disp_size";
    ub.uniforms[0].type = SG_UNIFORMTYPE_FLOAT2;
    shd_desc.fs.images[0].name = "tex";
    shd_desc.fs.images[0].type = SG_IMAGETYPE_2D;
    shd_desc.vs.source = _simgui_vs_src;
    shd_desc.fs.source = _simgui_fs_src;
    shd_desc.label = "sokol-imgui-shader";
    _simgui.shd = sg_make_shader(&shd_desc);

    /* pipeline object for imgui rendering */
    sg_pipeline_desc pip_desc = { };
    pip_desc.layout.buffers[0].stride = sizeof(ImDrawVert);
    {
        auto& attr = pip_desc.layout.attrs[0];
        attr.name       = "position";
        attr.sem_name   = "POSITION";
        attr.offset     = offsetof(ImDrawVert, pos);
        attr.format     = SG_VERTEXFORMAT_FLOAT2;
    }
    {
        auto& attr = pip_desc.layout.attrs[1];
        attr.name       = "texcoord0";
        attr.sem_name   = "TEXCOORD";
        attr.offset     = offsetof(ImDrawVert, uv);
        attr.format     = SG_VERTEXFORMAT_FLOAT2;
    }
    {
        auto& attr = pip_desc.layout.attrs[2];
        attr.name       = "color0";
        attr.sem_name   = "COLOR";
        attr.offset     = offsetof(ImDrawVert, col);
        attr.format     = SG_VERTEXFORMAT_UBYTE4N;
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
    /* NOTE: it's valid to call the destroy funcs with SG_INVALID_ID */
    sg_destroy_pipeline(_simgui.pip);
    sg_destroy_shader(_simgui.shd);
    sg_destroy_image(_simgui.img);
    sg_destroy_buffer(_simgui.ibuf);
    sg_destroy_buffer(_simgui.vbuf);
}

SOKOL_API_IMPL void simgui_new_frame(int width, int height, double delta_time) {
    ImGuiIO& io = ImGui::GetIO();
    io.DisplaySize.x = (float) width;
    io.DisplaySize.y = (float) height;
    io.DeltaTime = (float) delta_time;
    #if !defined(SOKOL_IMGUI_NO_SOKOL_APP)
    for (int i = 0; i < SAPP_MAX_MOUSEBUTTONS; i++) {
        if (_simgui.btn_down[i]) {
            _simgui.btn_down[i] = false;
            io.MouseDown[i] = true;
        }
        else if (_simgui.btn_up[i]) {
            _simgui.btn_up[i] = false;
            io.MouseDown[i] = false;
        }
    }
    if (io.WantTextInput && !sapp_keyboard_shown()) {
        sapp_show_keyboard(true);
    }
    if (!io.WantTextInput && sapp_keyboard_shown()) {
        sapp_show_keyboard(false);
    }
    #endif
    ImGui::NewFrame();
}

SOKOL_API_IMPL void simgui_render(void) {
    ImGui::Render();
    ImDrawData* draw_data = ImGui::GetDrawData();
    if (nullptr == draw_data) {
        return;
    }
    if (draw_data->CmdListsCount == 0) {
        return;
    }
    const float dpi_scale = _simgui.desc.dpi_scale;

    /* render the ImGui command list */
    sg_push_debug_group("sokol-imgui");
    sg_apply_pipeline(_simgui.pip);
    _simgui_vs_params_t vs_params;
    vs_params.disp_size.x = ImGui::GetIO().DisplaySize.x / dpi_scale;
    vs_params.disp_size.y = ImGui::GetIO().DisplaySize.y / dpi_scale;
    sg_apply_uniforms(SG_SHADERSTAGE_VS, 0, &vs_params, sizeof(vs_params));
    sg_bindings bind = { };
    bind.vertex_buffers[0] = _simgui.vbuf;
    bind.index_buffer = _simgui.ibuf;
    ImTextureID tex_id = ImGui::GetIO().Fonts->TexID;
    bind.fs_images[0].id = (uint32_t)(uintptr_t)tex_id;
    for (int cl_index = 0; cl_index < draw_data->CmdListsCount; cl_index++) {
        const ImDrawList* cl = draw_data->CmdLists[cl_index];

        /* append vertices and indices to buffers, record start offsets in draw state */
        const int vtx_size = cl->VtxBuffer.size() * sizeof(ImDrawVert);
        const int idx_size = cl->IdxBuffer.size() * sizeof(ImDrawIdx);
        const int vb_offset = sg_append_buffer(bind.vertex_buffers[0], &cl->VtxBuffer.front(), vtx_size);
        const int ib_offset = sg_append_buffer(bind.index_buffer, &cl->IdxBuffer.front(), idx_size);
        /* don't render anything if the buffer is in overflow state (this is also
            checked internally in sokol_gfx, draw calls that attempt from
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
        for (const ImDrawCmd& pcmd : cl->CmdBuffer) {
            if (pcmd.UserCallback) {
                pcmd.UserCallback(cl, &pcmd);
            }
            else {
                if (tex_id != pcmd.TextureId) {
                    tex_id = pcmd.TextureId;
                    bind.fs_images[0].id = (uint32_t)(uintptr_t)tex_id;
                    sg_apply_bindings(&bind);
                }
                const int scissor_x = (int) (pcmd.ClipRect.x * dpi_scale);
                const int scissor_y = (int) (pcmd.ClipRect.y * dpi_scale);
                const int scissor_w = (int) ((pcmd.ClipRect.z - pcmd.ClipRect.x) * dpi_scale);
                const int scissor_h = (int) ((pcmd.ClipRect.w - pcmd.ClipRect.y) * dpi_scale);
                sg_apply_scissor_rect(scissor_x, scissor_y, scissor_w, scissor_h, true);
                sg_draw(base_element, pcmd.ElemCount, 1);
            }
            base_element += pcmd.ElemCount;
        }
    }
    sg_pop_debug_group();
}

#if !defined(SOKOL_IMGUI_NO_SOKOL_APP)
SOKOL_API_IMPL bool simgui_handle_event(const sapp_event* ev) {
    const float dpi_scale = _simgui.desc.dpi_scale;
    auto& io = ImGui::GetIO();
    io.KeyAlt = (ev->modifiers & SAPP_MODIFIER_ALT) != 0;
    io.KeyCtrl = (ev->modifiers & SAPP_MODIFIER_CTRL) != 0;
    io.KeyShift = (ev->modifiers & SAPP_MODIFIER_SHIFT) != 0;
    io.KeySuper = (ev->modifiers & SAPP_MODIFIER_SUPER) != 0;
    switch (ev->type) {
        case SAPP_EVENTTYPE_MOUSE_DOWN:
            io.MousePos.x = ev->mouse_x / dpi_scale;
            io.MousePos.y = ev->mouse_y / dpi_scale;
            if (ev->mouse_button < 3) {
                _simgui.btn_down[ev->mouse_button] = true;
            }
            break;
        case SAPP_EVENTTYPE_MOUSE_UP:
            io.MousePos.x = ev->mouse_x / dpi_scale;
            io.MousePos.y = ev->mouse_y / dpi_scale;
            if (ev->mouse_button < 3) {
                _simgui.btn_up[ev->mouse_button] = true;
            }
            break;
        case SAPP_EVENTTYPE_MOUSE_MOVE:
            io.MousePos.x = ev->mouse_x / dpi_scale;
            io.MousePos.y = ev->mouse_y / dpi_scale;
            break;
        case SAPP_EVENTTYPE_MOUSE_ENTER:
        case SAPP_EVENTTYPE_MOUSE_LEAVE:
            for (int i = 0; i < 3; i++) {
                _simgui.btn_down[i] = false;
                _simgui.btn_up[i] = false;
                io.MouseDown[i] = false;
            }
            break;
        case SAPP_EVENTTYPE_MOUSE_SCROLL:
            io.MouseWheelH = ev->scroll_x;
            io.MouseWheel = ev->scroll_y;
            break;
        case SAPP_EVENTTYPE_TOUCHES_BEGAN:
            _simgui.btn_down[0] = true;
            io.MousePos.x = ev->touches[0].pos_x / dpi_scale;
            io.MousePos.y = ev->touches[0].pos_y / dpi_scale;
            break;
        case SAPP_EVENTTYPE_TOUCHES_MOVED:
            io.MousePos.x = ev->touches[0].pos_x / dpi_scale;
            io.MousePos.y = ev->touches[0].pos_y / dpi_scale;
            break;
        case SAPP_EVENTTYPE_TOUCHES_ENDED:
            _simgui.btn_up[0] = true;
            io.MousePos.x = ev->touches[0].pos_x / dpi_scale;
            io.MousePos.y = ev->touches[0].pos_y / dpi_scale;
            break;
        case SAPP_EVENTTYPE_TOUCHES_CANCELLED:
            _simgui.btn_up[0] = _simgui.btn_down[0] = false;
            break;
        case SAPP_EVENTTYPE_KEY_DOWN:
            io.KeysDown[ev->key_code] = true;
            break;
        case SAPP_EVENTTYPE_KEY_UP:
            io.KeysDown[ev->key_code] = false;
            break;
        case SAPP_EVENTTYPE_CHAR:
            io.AddInputCharacter((ImWchar)ev->char_code);
            break;
        default:
            break;
    }
    return io.WantCaptureKeyboard;
}
#endif

#endif /* SOKOL_IMPL */
