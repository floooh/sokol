# Sokol

**Sokol (Сокол)**: Russian for Falcon, a smaller and more nimble 
bird of prey than the Eagle (Орёл, Oryol)

Minimalistic header-only cross-platform libs in C:

- **sokol\_gfx.h**: 3D-API wrapper (GL + Metal + D3D11)
- **sokol\_app.h**: app framework wrapper (entry + window + 3D-context + input)
- **sokol\_time.h**: time measurement
- **sokol\_audio.h**: (WIP!) minimal buffer-streaming audio playback
- ...???

These are the internal parts of the Oryol C++ framework 
rewritten in pure C as standalone header-only libs.

All headers are standalone and can be used indepedendently from each other.

Sample code is in a separate repo: https://github.com/floooh/sokol-samples

asm.js/wasm live demos: https://floooh.github.io/sokol-html5/index.html

Tiny 8-bit emulators: https://floooh.github.io/tiny8bit/

Nim bindings: https://github.com/floooh/sokol-nim

Nim samples: https://github.com/floooh/sokol-nim-samples

### Why C:

- easier integration with other languages
- easier integration into other projects
- allows even smaller program binaries than Oryol

Sokol will be a bit less convenient to use than Oryol, but that's ok since
the Sokol headers are intended to be low-level building blocks.

Eventually Oryol will just be a thin C++ layer over Sokol.

A blog post with more background info: [A Tour of sokol_gfx.h](http://floooh.github.io/2017/07/29/sokol-gfx-tour.html)


# sokol_gfx.h:

- simple, modern wrapper around GLES2/WebGL, GLES3/WebGL2, GL3.3, D3D11 and Metal
- buffers, images, shaders, pipeline-state-objects and render-passes
- does *not* handle window creation or 3D API context initialization
- does *not* provide shader dialect cross-translation

A triangle in C99 with GLFW and FlextGL:

```c
#define GLFW_INCLUDE_NONE
#include "GLFW/glfw3.h"
#include "flextgl/flextGL.h"
#define SOKOL_IMPL
#define SOKOL_GLCORE33
#include "sokol_gfx.h"

int main() {

    /* create window and GL context via GLFW */
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    GLFWwindow* w = glfwCreateWindow(640, 480, "Sokol Triangle GLFW", 0, 0);
    glfwMakeContextCurrent(w);
    glfwSwapInterval(1);
    flextInit();

    /* setup sokol_gfx */
    sg_setup(&(sg_desc){0});

    /* a vertex buffer */
    const float vertices[] = {
        // positions            // colors
         0.0f,  0.5f, 0.5f,     1.0f, 0.0f, 0.0f, 1.0f,
         0.5f, -0.5f, 0.5f,     0.0f, 1.0f, 0.0f, 1.0f,
        -0.5f, -0.5f, 0.5f,     0.0f, 0.0f, 1.0f, 1.0f 
    };
    sg_buffer vbuf = sg_make_buffer(&(sg_buffer_desc){
        .size = sizeof(vertices),
        .content = vertices, 
    });

    /* a shader */
    sg_shader shd = sg_make_shader(&(sg_shader_desc){
        .vs.source = 
            "#version 330\n"
            "in vec4 position;\n"
            "in vec4 color0;\n"
            "out vec4 color;\n"
            "void main() {\n"
            "  gl_Position = position;\n"
            "  color = color0;\n"
            "}\n",
        .fs.source =
            "#version 330\n"
            "in vec4 color;\n"
            "out vec4 frag_color;\n"
            "void main() {\n"
            "  frag_color = color;\n"
            "}\n"
    });

    /* a pipeline state object (default render states are fine for triangle) */
    sg_pipeline pip = sg_make_pipeline(&(sg_pipeline_desc){
        .shader = shd,
        .layout = {
            .attrs = {
                [0] = { .name="position", .format=SG_VERTEXFORMAT_FLOAT3 },
                [1] = { .name="color0", .format=SG_VERTEXFORMAT_FLOAT4 }
            }
        }
    });

    /* a draw state with all the resource binding */
    sg_draw_state draw_state = {
        .pipeline = pip,
        .vertex_buffers[0] = vbuf
    };

    /* default pass action (clear to grey) */
    sg_pass_action pass_action = {0};

    /* draw loop */
    while (!glfwWindowShouldClose(w)) {
        int cur_width, cur_height;
        glfwGetFramebufferSize(w, &cur_width, &cur_height);
        sg_begin_default_pass(&pass_action, cur_width, cur_height);
        sg_apply_draw_state(&draw_state);
        sg_draw(0, 3, 1);
        sg_end_pass();
        sg_commit();
        glfwSwapBuffers(w);
        glfwPollEvents();
    }

    /* cleanup */
    sg_shutdown();
    glfwTerminate();
    return 0;
}
```

# sokol_time.h:

Simple cross-platform time measurement:

```c
#include "sokol_time.h"
...
/* initialize sokol_time */
stm_setup();

/* take start timestamp */
uint64_t start = stm_now();

...some code to measure...

/* compute elapsed time */
uint64_t elapsed = stm_since(start);

/* convert to time units */
double seconds = stm_sec(elapsed);
double milliseconds = stm_ms(elapsed);
double microseconds = stm_us(elapsed);
double nanoseconds = stm_ns(elapsed);

/* difference between 2 time stamps */
uint64_t start = stm_now();
...
uint64_t end = stm_now();
uint64_t elapsed = stm_diff(end, start);

/* compute a 'lap time' (e.g. for fps) */
uint64_t last_time = 0;
while (!done) {
    ...render something...
    double frame_time_ms = stm_ms(stm_laptime(&last_time));
}
```

# sokol_app.h

A minimal cross-platform application-wrapper library:

- unified application entry
- single window or canvas for 3D rendering
- 3D context initialization
- event-based keyboard, mouse and touch input
- supported platforms: Win32, MacOS, Linux (X11), iOS, WASM/asm.js (planned: Android, RaspberryPi)
- supported 3D-APIs: GL3.3 (GLX/WGL), Metal, D3D11, GLES2/WebGL, GLES3/WebGL2

A simple clear-loop sample using sokol_app.h and sokol_gfx.h (does not include
separate sokol.c/.m implementation file which is necessary
to split the Objective-C code from the C code of the sample):

```cpp
#include "sokol_gfx.h"
#include "sokol_app.h"

sg_pass_action pass_action;

void init(void) {
    sg_setup(&(sg_desc){
        .mtl_device = sapp_metal_get_device(),
        .mtl_renderpass_descriptor_cb = sapp_metal_get_renderpass_descriptor,
        .mtl_drawable_cb = sapp_metal_get_drawable,
        .d3d11_device = sapp_d3d11_get_device(),
        .d3d11_device_context = sapp_d3d11_get_device_context(),
        .d3d11_render_target_view_cb = sapp_d3d11_get_render_target_view,
        .d3d11_depth_stencil_view_cb = sapp_d3d11_get_depth_stencil_view
    });
    pass_action = (sg_pass_action) {
        .colors[0] = { .action=SG_ACTION_CLEAR, .val={1.0f, 0.0f, 0.0f, 1.0f} }
    };
}

void frame(void) {
    float g = pass_action.colors[0].val[1] + 0.01f;
    pass_action.colors[0].val[1] = (g > 1.0f) ? 0.0f : g;
    sg_begin_default_pass(&pass_action, sapp_width(), sapp_height());
    sg_end_pass();
    sg_commit();
}

void cleanup(void) {
    sg_shutdown();
}

sapp_desc sokol_main(int argc, char* argv[]) {
    return (sapp_desc){
        .init_cb = init,
        .frame_cb = frame,
        .cleanup_cb = cleanup,
        .width = 400,
        .height = 300,
        .window_title = "Clear (sokol app)",
    };
}
```

# Updates

- **11-May-2018**: sokol_gfx.h now autodetects iOS vs MacOS in the Metal
backend during compilation using the standard define TARGET_OS_IPHONE defined
in the TargetConditionals.h system header, please replace the old
backend-selection defines SOKOL_METAL_MACOS and SOKOL_METAL_IOS with
**SOKOL_METAL**

- **20-Apr-2018**: 3 new context-switching functions have been added
to make it possible to use sokol together with applications that
use multiple GL contexts. On D3D11 and Metal, the functions are currently
empty. See the new section ```WORKING WITH CONTEXTS``` in the sokol_gfx.h
header documentation, and the new sample [multiwindow-glfw](https://github.com/floooh/sokol-samples/blob/master/glfw/multiwindow-glfw.c)

- **31-Jan-2018**: The vertex layout declaration in sg\_pipeline\_desc had
some fairly subtle flaws and has been changed to work like Metal or Vulkan.
The gist is that the vertex-buffer-layout properties (vertex stride, 
vertex-step-rate and -step-function for instancing) is now defined in a
separate array from the vertex attributes. This removes some brittle backend
code which tries to guess the right vertex attribute slot if no attribute
names are given, and which was wrong for shader-code-generation pipelines
which reorder the vertex attributes (I stumbled over this when porting the
Oryol Gfx module over to sokol-gfx). Some code samples:

```c
// a complete vertex layout declaration with a single input buffer
// with two vertex attributes
sg_pipeline pip = sg_make_pipeline(&(sg_pipeline_desc){
    .layout = {
        .buffers = {
            [0] = { 
                .stride = 20,
                .step_func = SG_VERTEXSTEP_PER_VERTEX,
                .step_rate = 1 
            }
        },
        .attrs = {
            [0] = { 
                .name = "pos", 
                .offset = 0, 
                .format = SG_VERTEXFORMAT_FLOAT3,
                .buffer_index = 0 
            },
            [1] = {
                .name = "uv",
                .offset = 12,
                .format = SG_VERTEXFORMAT_FLOAT2,
                .buffer_index = 0
            }
        }
    },
    ...
});

// if the vertex layout has no gaps, we can get rid of the strides and offsets:
sg_pipeline pip = sg_make_pipeline(&(sg_pipeline_desc){
    .layout = {
        .buffers = {
            [0] = { 
                .step_func = SG_VERTEXSTEP_PER_VERTEX,
                .step_rate=1
            }
        },
        .attrs = {
            [0] = { 
                .name = "pos", 
                .format = SG_VERTEXFORMAT_FLOAT3,
                .buffer_index = 0 
            },
            [1] = {
                .name = "uv",
                .format = SG_VERTEXFORMAT_FLOAT2,
                .buffer_index = 0
            }
        }
    },
    ...
});

// we can also get rid of the other default-values, which leaves buffers[0]
// as all-defaults, so it can disappear completely:
sg_pipeline pip = sg_make_pipeline(&(sg_pipeline_desc){
    .layout = {
        .attrs = {
            [0] = { .name = "pos", .format = SG_VERTEXFORMAT_FLOAT3 },
            [1] = { .name = "uv", .format = SG_VERTEXFORMAT_FLOAT2 }
        }
    },
    ...
});

// and finally on GL3.3 and Metal and we don't need the attribute names
// (on D3D11, a semantic name and index must be provided though)
sg_pipeline pip = sg_make_pipeline(&(sg_pipeline_desc){
    .layout = {
        .attrs = {
            [0] = { .format = SG_VERTEXFORMAT_FLOAT3 },
            [1] = { .format = SG_VERTEXFORMAT_FLOAT2 }
        }
    },
    ...
});
```

Please check the sample code in https://github.com/floooh/sokol-samples for
more examples!

Enjoy!