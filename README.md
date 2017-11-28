# Sokol

**Sokol (Сокол)**: Russian for Falcon, a smaller and more nimble 
bird of prey than the Eagle (Орёл, Oryol)

Minimalistic header-only platform abstraction libs in C:

- sokol\_gfx.h
- sokol\_time.h
- ...???

These are the internal parts of the Oryol C++ framework 
rewritten in pure C as standalone header-only libs.

Sample code is in a separate repo: https://github.com/floooh/sokol-samples

asm.js/wasm live demos: https://floooh.github.io/sokol-html5/index.html

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
    flextInit(w);

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
        .vertex_layouts[0] = {
            .stride = 28,
            .attrs = {
                [0] = { .name="position", .offset=0, .format=SG_VERTEXFORMAT_FLOAT3 },
                [1] = { .name="color0", .offset=12, .format=SG_VERTEXFORMAT_FLOAT4 }
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

See https://github.com/floooh/sokol-samples for more samples.

Enjoy!