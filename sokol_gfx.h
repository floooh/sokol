#if defined(SOKOL_IMPL) && !defined(SOKOL_GFX_IMPL)
#define SOKOL_GFX_IMPL
#endif
#ifndef SOKOL_GFX_INCLUDED
/*
    sokol_gfx.h -- simple 3D API wrapper

    Project URL: https://github.com/floooh/sokol

    Example code: https://github.com/floooh/sokol-samples

    Do this:
        #define SOKOL_IMPL or
        #define SOKOL_GFX_IMPL
    before you include this file in *one* C or C++ file to create the
    implementation.

    In the same place define one of the following to select the rendering
    backend:
        #define SOKOL_GLCORE33
        #define SOKOL_GLES2
        #define SOKOL_GLES3
        #define SOKOL_D3D11
        #define SOKOL_METAL
        #define SOKOL_WGPU
        #define SOKOL_DUMMY_BACKEND

    I.e. for the GL 3.3 Core Profile it should look like this:

    #include ...
    #include ...
    #define SOKOL_IMPL
    #define SOKOL_GLCORE33
    #include "sokol_gfx.h"

    The dummy backend replaces the platform-specific backend code with empty
    stub functions. This is useful for writing tests that need to run on the
    command line.

    Optionally provide the following defines with your own implementations:

    SOKOL_ASSERT(c)             - your own assert macro (default: assert(c))
    SOKOL_UNREACHABLE()         - a guard macro for unreachable code (default: assert(false))
    SOKOL_GFX_API_DECL          - public function declaration prefix (default: extern)
    SOKOL_API_DECL              - same as SOKOL_GFX_API_DECL
    SOKOL_API_IMPL              - public function implementation prefix (default: -)
    SOKOL_TRACE_HOOKS           - enable trace hook callbacks (search below for TRACE HOOKS)
    SOKOL_EXTERNAL_GL_LOADER    - indicates that you're using your own GL loader, in this case
                                  sokol_gfx.h will not include any platform GL headers and disable
                                  the integrated Win32 GL loader

    If sokol_gfx.h is compiled as a DLL, define the following before
    including the declaration or implementation:

    SOKOL_DLL

    On Windows, SOKOL_DLL will define SOKOL_GFX_API_DECL as __declspec(dllexport)
    or __declspec(dllimport) as needed.

    If you want to compile without deprecated structs and functions,
    define:

    SOKOL_NO_DEPRECATED

    Optionally define the following to force debug checks and validations
    even in release mode:

    SOKOL_DEBUG         - by default this is defined if _DEBUG is defined

    sokol_gfx DOES NOT:
    ===================
    - create a window or the 3D-API context/device, you must do this
      before sokol_gfx is initialized, and pass any required information
      (like 3D device pointers) to the sokol_gfx initialization call

    - present the rendered frame, how this is done exactly usually depends
      on how the window and 3D-API context/device was created

    - provide a unified shader language, instead 3D-API-specific shader
      source-code or shader-bytecode must be provided (for the "official"
      offline shader cross-compiler, see here:
      https://github.com/floooh/sokol-tools/blob/master/docs/sokol-shdc.md)


    STEP BY STEP
    ============
    --- to initialize sokol_gfx, after creating a window and a 3D-API
        context/device, call:

            sg_setup(const sg_desc*)

    --- create resource objects (at least buffers, shaders and pipelines,
        and optionally images and passes):

            sg_buffer sg_make_buffer(const sg_buffer_desc*)
            sg_image sg_make_image(const sg_image_desc*)
            sg_shader sg_make_shader(const sg_shader_desc*)
            sg_pipeline sg_make_pipeline(const sg_pipeline_desc*)
            sg_pass sg_make_pass(const sg_pass_desc*)

    --- start rendering to the default frame buffer with:

            sg_begin_default_pass(const sg_pass_action* action, int width, int height)

        ...or alternatively with:

            sg_begin_default_passf(const sg_pass_action* action, float width, float height)

        ...which takes the framebuffer width and height as float values.

    --- or start rendering to an offscreen framebuffer with:

            sg_begin_pass(sg_pass pass, const sg_pass_action* action)

    --- set the pipeline state for the next draw call with:

            sg_apply_pipeline(sg_pipeline pip)

    --- fill an sg_bindings struct with the resource bindings for the next
        draw call (1..N vertex buffers, 0 or 1 index buffer, 0..N image objects
        to use as textures each on the vertex-shader- and fragment-shader-stage
        and then call

            sg_apply_bindings(const sg_bindings* bindings)

        to update the resource bindings

    --- optionally update shader uniform data with:

            sg_apply_uniforms(sg_shader_stage stage, int ub_index, const sg_range* data)

        Read the section 'UNIFORM DATA LAYOUT' to learn about the expected memory layout
        of the uniform data passed into sg_apply_uniforms().

    --- kick off a draw call with:

            sg_draw(int base_element, int num_elements, int num_instances)

        The sg_draw() function unifies all the different ways to render primitives
        in a single call (indexed vs non-indexed rendering, and instanced vs non-instanced
        rendering). In case of indexed rendering, base_element and num_element specify
        indices in the currently bound index buffer. In case of non-indexed rendering
        base_element and num_elements specify vertices in the currently bound
        vertex-buffer(s). To perform instanced rendering, the rendering pipeline
        must be setup for instancing (see sg_pipeline_desc below), a separate vertex buffer
        containing per-instance data must be bound, and the num_instances parameter
        must be > 1.

    --- finish the current rendering pass with:

            sg_end_pass()

    --- when done with the current frame, call

            sg_commit()

    --- at the end of your program, shutdown sokol_gfx with:

            sg_shutdown()

    --- if you need to destroy resources before sg_shutdown(), call:

            sg_destroy_buffer(sg_buffer buf)
            sg_destroy_image(sg_image img)
            sg_destroy_shader(sg_shader shd)
            sg_destroy_pipeline(sg_pipeline pip)
            sg_destroy_pass(sg_pass pass)

    --- to set a new viewport rectangle, call

            sg_apply_viewport(int x, int y, int width, int height, bool origin_top_left)

        ...or if you want to specifiy the viewport rectangle with float values:

            sg_apply_viewportf(float x, float y, float width, float height, bool origin_top_left)

    --- to set a new scissor rect, call:

            sg_apply_scissor_rect(int x, int y, int width, int height, bool origin_top_left)

        ...or with float values:

            sg_apply_scissor_rectf(float x, float y, float width, float height, bool origin_top_left)

        Both sg_apply_viewport() and sg_apply_scissor_rect() must be called
        inside a rendering pass

        Note that sg_begin_default_pass() and sg_begin_pass() will reset both the
        viewport and scissor rectangles to cover the entire framebuffer.

    --- to update (overwrite) the content of buffer and image resources, call:

            sg_update_buffer(sg_buffer buf, const sg_range* data)
            sg_update_image(sg_image img, const sg_image_data* data)

        Buffers and images to be updated must have been created with
        SG_USAGE_DYNAMIC or SG_USAGE_STREAM

        Only one update per frame is allowed for buffer and image resources when
        using the sg_update_*() functions. The rationale is to have a simple
        countermeasure to avoid the CPU scribbling over data the GPU is currently
        using, or the CPU having to wait for the GPU

        Buffer and image updates can be partial, as long as a rendering
        operation only references the valid (updated) data in the
        buffer or image.

    --- to append a chunk of data to a buffer resource, call:

            int sg_append_buffer(sg_buffer buf, const sg_range* data)

        The difference to sg_update_buffer() is that sg_append_buffer()
        can be called multiple times per frame to append new data to the
        buffer piece by piece, optionally interleaved with draw calls referencing
        the previously written data.

        sg_append_buffer() returns a byte offset to the start of the
        written data, this offset can be assigned to
        sg_bindings.vertex_buffer_offsets[n] or
        sg_bindings.index_buffer_offset

        Code example:

        for (...) {
            const void* data = ...;
            const int num_bytes = ...;
            int offset = sg_append_buffer(buf, &(sg_range) { .ptr=data, .size=num_bytes });
            bindings.vertex_buffer_offsets[0] = offset;
            sg_apply_pipeline(pip);
            sg_apply_bindings(&bindings);
            sg_apply_uniforms(...);
            sg_draw(...);
        }

        A buffer to be used with sg_append_buffer() must have been created
        with SG_USAGE_DYNAMIC or SG_USAGE_STREAM.

        If the application appends more data to the buffer then fits into
        the buffer, the buffer will go into the "overflow" state for the
        rest of the frame.

        Any draw calls attempting to render an overflown buffer will be
        silently dropped (in debug mode this will also result in a
        validation error).

        You can also check manually if a buffer is in overflow-state by calling

            bool sg_query_buffer_overflow(sg_buffer buf)

        You can manually check to see if an overflow would occur before adding
        any data to a buffer by calling

            bool sg_query_buffer_will_overflow(sg_buffer buf, size_t size)

        NOTE: Due to restrictions in underlying 3D-APIs, appended chunks of
        data will be 4-byte aligned in the destination buffer. This means
        that there will be gaps in index buffers containing 16-bit indices
        when the number of indices in a call to sg_append_buffer() is
        odd. This isn't a problem when each call to sg_append_buffer()
        is associated with one draw call, but will be problematic when
        a single indexed draw call spans several appended chunks of indices.

    --- to check at runtime for optional features, limits and pixelformat support,
        call:

            sg_features sg_query_features()
            sg_limits sg_query_limits()
            sg_pixelformat_info sg_query_pixelformat(sg_pixel_format fmt)

    --- if you need to call into the underlying 3D-API directly, you must call:

            sg_reset_state_cache()

        ...before calling sokol_gfx functions again

    --- you can inspect the original sg_desc structure handed to sg_setup()
        by calling sg_query_desc(). This will return an sg_desc struct with
        the default values patched in instead of any zero-initialized values

    --- you can inspect various internal resource attributes via:

            sg_buffer_info sg_query_buffer_info(sg_buffer buf)
            sg_image_info sg_query_image_info(sg_image img)
            sg_shader_info sg_query_shader_info(sg_shader shd)
            sg_pipeline_info sg_query_pipeline_info(sg_pipeline pip)
            sg_pass_info sg_query_pass_info(sg_pass pass)

        ...please note that the returned info-structs are tied quite closely
        to sokol_gfx.h internals, and may change more often than other
        public API functions and structs.

    --- you can ask at runtime what backend sokol_gfx.h has been compiled
        for, or whether the GLES3 backend had to fall back to GLES2 with:

            sg_backend sg_query_backend(void)

    --- you can query the default resource creation parameters through the functions

            sg_buffer_desc sg_query_buffer_defaults(const sg_buffer_desc* desc)
            sg_image_desc sg_query_image_defaults(const sg_image_desc* desc)
            sg_shader_desc sg_query_shader_defaults(const sg_shader_desc* desc)
            sg_pipeline_desc sg_query_pipeline_defaults(const sg_pipeline_desc* desc)
            sg_pass_desc sg_query_pass_defaults(const sg_pass_desc* desc)

        These functions take a pointer to a desc structure which may contain
        zero-initialized items for default values. These zero-init values
        will be replaced with their concrete values in the returned desc
        struct.


    ON INITIALIZATION:
    ==================
    When calling sg_setup(), a pointer to an sg_desc struct must be provided
    which contains initialization options. These options provide two types
    of information to sokol-gfx:

        (1) upper bounds and limits needed to allocate various internal
            data structures:
                - the max number of resources of each type that can
                  be alive at the same time, this is used for allocating
                  internal pools
                - the max overall size of uniform data that can be
                  updated per frame, including a worst-case alignment
                  per uniform update (this worst-case alignment is 256 bytes)
                - the max size of all dynamic resource updates (sg_update_buffer,
                  sg_append_buffer and sg_update_image) per frame
                - the max number of entries in the texture sampler cache
                  (how many unique texture sampler can exist at the same time)
            Not all of those limit values are used by all backends, but it is
            good practice to provide them none-the-less.

        (2) 3D-API "context information" (sometimes also called "bindings"):
            sokol_gfx.h doesn't create or initialize 3D API objects which are
            closely related to the presentation layer (this includes the "rendering
            device", the swapchain, and any objects which depend on the
            swapchain). These API objects (or callback functions to obtain
            them, if those objects might change between frames), must
            be provided in a nested sg_context_desc struct inside the
            sg_desc struct. If sokol_gfx.h is used together with
            sokol_app.h, have a look at the sokol_glue.h header which provides
            a convenience function to get a sg_context_desc struct filled out
            with context information provided by sokol_app.h

    See the documention block of the sg_desc struct below for more information.


    UNIFORM DATA LAYOUT:
    ====================
    NOTE: if you use the sokol-shdc shader compiler tool, you don't need to worry
    about the following details.

    The data that's passed into the sg_apply_uniforms() function must adhere to
    specific layout rules so that the GPU shader finds the uniform block
    items at the right offset.

    For the D3D11 and Metal backends, sokol-gfx only cares about the size of uniform
    blocks, but not about the internal layout. The data will just be copied into
    a uniform/constant buffer in a single operation and it's up you to arrange the
    CPU-side layout so that it matches the GPU side layout. This also means that with
    the D3D11 and Metal backends you are not limited to a 'cross-platform' subset
    of uniform variable types.

    If you ever only use one of the D3D11, Metal *or* WebGPU backend, you can stop reading here.

    For the GL backends, the internal layout of uniform blocks matters though,
    and you are limited to a small number of uniform variable types. This is
    because sokol-gfx must be able to locate the uniform block members in order
    to upload them to the GPU with glUniformXXX() calls.

    To describe the uniform block layout to sokol-gfx, the following information
    must be passed to the sg_make_shader() call in the sg_shader_desc struct:

        - a hint about the used packing rule (either SG_UNIFORMLAYOUT_NATIVE or
          SG_UNIFORMLAYOUT_STD140)
        - a list of the uniform block members types in the correct order they
          appear on the CPU side

    For example if the GLSL shader has the following uniform declarations:

        uniform mat4 mvp;
        uniform vec2 offset0;
        uniform vec2 offset1;
        uniform vec2 offset2;

    ...and on the CPU side, there's a similar C struct:

        typedef struct {
            float mvp[16];
            float offset0[2];
            float offset1[2];
            float offset2[2];
        } params_t;

    ...the uniform block description in the sg_shader_desc must look like this:

        sg_shader_desc desc = {
            .vs.uniform_blocks[0] = {
                .size = sizeof(params_t),
                .layout = SG_UNIFORMLAYOUT_NATIVE,  // this is the default and can be omitted
                .uniforms = {
                    // order must be the same as in 'params_t':
                    [0] = { .name = "mvp", .type = SG_UNIFORMTYPE_MAT4 },
                    [1] = { .name = "offset0", .type = SG_UNIFORMTYPE_VEC2 },
                    [2] = { .name = "offset1", .type = SG_UNIFORMTYPE_VEC2 },
                    [3] = { .name = "offset2", .type = SG_UNIFORMTYPE_VEC2 },
                }
            }
        };

    With this information sokol-gfx can now compute the correct offsets of the data items
    within the uniform block struct.

    The SG_UNIFORMLAYOUT_NATIVE packing rule works fine if only the GL backends are used,
    but for proper D3D11/Metal/GL a subset of the std140 layout must be used which is
    described in the next section:


    CROSS-BACKEND COMMON UNIFORM DATA LAYOUT
    ========================================
    For cross-platform / cross-3D-backend code it is important that the same uniform block
    layout on the CPU side can be used for all sokol-gfx backends. To achieve this,
    a common subset of the std140 layout must be used:

    - The uniform block layout hint in sg_shader_desc must be explicitely set to
      SG_UNIFORMLAYOUT_STD140.
    - Only the following GLSL uniform types can be used (with their associated sokol-gfx enums):
        - float => SG_UNIFORMTYPE_FLOAT
        - vec2  => SG_UNIFORMTYPE_FLOAT2
        - vec3  => SG_UNIFORMTYPE_FLOAT3
        - vec4  => SG_UNIFORMTYPE_FLOAT4
        - int   => SG_UNIFORMTYPE_INT
        - ivec2 => SG_UNIFORMTYPE_INT2
        - ivec3 => SG_UNIFORMTYPE_INT3
        - ivec4 => SG_UNIFORMTYPE_INT4
        - mat4  => SG_UNIFORMTYPE_MAT4
    - Alignment for those types must be as follows (in bytes):
        - float => 4
        - vec2  => 8
        - vec3  => 16
        - vec4  => 16
        - int   => 4
        - ivec2 => 8
        - ivec3 => 16
        - ivec4 => 16
        - mat4  => 16
    - Arrays are only allowed for the following types: vec4, int4, mat4.

    Note that the HLSL cbuffer layout rules are slightly different from the
    std140 layout rules, this means that the cbuffer declarations in HLSL code
    must be tweaked so that the layout is compatible with std140.

    The by far easiest way to tacke the common uniform block layout problem is
    to use the sokol-shdc shader cross-compiler tool!


    BACKEND-SPECIFIC TOPICS:
    ========================
    --- The GL backends need to know about the internal structure of uniform
        blocks, and the texture sampler-name and -type. The uniform layout details
        are  described in the UNIFORM DATA LAYOUT section above.

            // uniform block structure and texture image definition in sg_shader_desc:
            sg_shader_desc desc = {
                // uniform block description (size and internal structure)
                .vs.uniform_blocks[0] = {
                    ...
                },
                // one texture on the fragment-shader-stage, GLES2/WebGL needs name and image type
                .fs.images[0] = { .name="tex", .type=SG_IMAGETYPE_ARRAY }
                ...
            };

    --- the Metal and D3D11 backends only need to know the size of uniform blocks,
        not their internal member structure, and they only need to know
        the type of a texture sampler, not its name:

            sg_shader_desc desc = {
                .vs.uniform_blocks[0].size = sizeof(params_t),
                .fs.images[0].type = SG_IMAGETYPE_ARRAY,
                ...
            };

    --- when creating a shader object, GLES2/WebGL need to know the vertex
        attribute names as used in the vertex shader:

            sg_shader_desc desc = {
                .attrs = {
                    [0] = { .name="position" },
                    [1] = { .name="color1" }
                }
            };

        The vertex attribute names provided when creating a shader will be
        used later in sg_create_pipeline() for matching the vertex layout
        to vertex shader inputs.

    --- on D3D11 you need to provide a semantic name and semantic index in the
        shader description struct instead (see the D3D11 documentation on
        D3D11_INPUT_ELEMENT_DESC for details):

            sg_shader_desc desc = {
                .attrs = {
                    [0] = { .sem_name="POSITION", .sem_index=0 }
                    [1] = { .sem_name="COLOR", .sem_index=1 }
                }
            };

        The provided semantic information will be used later in sg_create_pipeline()
        to match the vertex layout to vertex shader inputs.

    --- on D3D11, and when passing HLSL source code (instead of byte code) to shader
        creation, you can optionally define the shader model targets on the vertex
        stage:

            sg_shader_Desc desc = {
                .vs = {
                    ...
                    .d3d11_target = "vs_5_0"
                },
                .fs = {
                    ...
                    .d3d11_target = "ps_5_0"
                }
            };

        The default targets are "ps_4_0" and "fs_4_0". Note that those target names
        are only used when compiling shaders from source. They are ignored when
        creating a shader from bytecode.

    --- on Metal, GL 3.3 or GLES3/WebGL2, you don't need to provide an attribute
        name or semantic name, since vertex attributes can be bound by their slot index
        (this is mandatory in Metal, and optional in GL):

            sg_pipeline_desc desc = {
                .layout = {
                    .attrs = {
                        [0] = { .format=SG_VERTEXFORMAT_FLOAT3 },
                        [1] = { .format=SG_VERTEXFORMAT_FLOAT4 }
                    }
                }
            };


    WORKING WITH CONTEXTS
    =====================
    sokol-gfx allows to switch between different rendering contexts and
    associate resource objects with contexts. This is useful to
    create GL applications that render into multiple windows.

    A rendering context keeps track of all resources created while
    the context is active. When the context is destroyed, all resources
    "belonging to the context" are destroyed as well.

    A default context will be created and activated implicitly in
    sg_setup(), and destroyed in sg_shutdown(). So for a typical application
    which *doesn't* use multiple contexts, nothing changes, and calling
    the context functions isn't necessary.

    Three functions have been added to work with contexts:

    --- sg_context sg_setup_context():
        This must be called once after a GL context has been created and
        made active.

    --- void sg_activate_context(sg_context ctx)
        This must be called after making a different GL context active.
        Apart from 3D-API-specific actions, the call to sg_activate_context()
        will internally call sg_reset_state_cache().

    --- void sg_discard_context(sg_context ctx)
        This must be called right before a GL context is destroyed and
        will destroy all resources associated with the context (that
        have been created while the context was active) The GL context must be
        active at the time sg_discard_context(sg_context ctx) is called.

    Also note that resources (buffers, images, shaders and pipelines) must
    only be used or destroyed while the same GL context is active that
    was also active while the resource was created (an exception is
    resource sharing on GL, such resources can be used while
    another context is active, but must still be destroyed under
    the same context that was active during creation).

    For more information, check out the multiwindow-glfw sample:

    https://github.com/floooh/sokol-samples/blob/master/glfw/multiwindow-glfw.c


    TRACE HOOKS:
    ============
    sokol_gfx.h optionally allows to install "trace hook" callbacks for
    each public API functions. When a public API function is called, and
    a trace hook callback has been installed for this function, the
    callback will be invoked with the parameters and result of the function.
    This is useful for things like debugging- and profiling-tools, or
    keeping track of resource creation and destruction.

    To use the trace hook feature:

    --- Define SOKOL_TRACE_HOOKS before including the implementation.

    --- Setup an sg_trace_hooks structure with your callback function
        pointers (keep all function pointers you're not interested
        in zero-initialized), optionally set the user_data member
        in the sg_trace_hooks struct.

    --- Install the trace hooks by calling sg_install_trace_hooks(),
        the return value of this function is another sg_trace_hooks
        struct which contains the previously set of trace hooks.
        You should keep this struct around, and call those previous
        functions pointers from your own trace callbacks for proper
        chaining.

    As an example of how trace hooks are used, have a look at the
    imgui/sokol_gfx_imgui.h header which implements a realtime
    debugging UI for sokol_gfx.h on top of Dear ImGui.


    A NOTE ON PORTABLE PACKED VERTEX FORMATS:
    =========================================
    There are two things to consider when using packed
    vertex formats like UBYTE4, SHORT2, etc which need to work
    across all backends:

    - D3D11 can only convert *normalized* vertex formats to
      floating point during vertex fetch, normalized formats
      have a trailing 'N', and are "normalized" to a range
      -1.0..+1.0 (for the signed formats) or 0.0..1.0 (for the
      unsigned formats):

        - SG_VERTEXFORMAT_BYTE4N
        - SG_VERTEXFORMAT_UBYTE4N
        - SG_VERTEXFORMAT_SHORT2N
        - SG_VERTEXFORMAT_USHORT2N
        - SG_VERTEXFORMAT_SHORT4N
        - SG_VERTEXFORMAT_USHORT4N

      D3D11 will not convert *non-normalized* vertex formats to floating point
      vertex shader inputs, those can only be uses with the *ivecn* vertex shader
      input types when D3D11 is used as backend (GL and Metal can use both formats)

        - SG_VERTEXFORMAT_BYTE4,
        - SG_VERTEXFORMAT_UBYTE4
        - SG_VERTEXFORMAT_SHORT2
        - SG_VERTEXFORMAT_SHORT4

    - WebGL/GLES2 cannot use integer vertex shader inputs (int or ivecn)

    - SG_VERTEXFORMAT_UINT10_N2 is not supported on WebGL/GLES2

    So for a vertex input layout which works on all platforms, only use the following
    vertex formats, and if needed "expand" the normalized vertex shader
    inputs in the vertex shader by multiplying with 127.0, 255.0, 32767.0 or
    65535.0:

        - SG_VERTEXFORMAT_FLOAT,
        - SG_VERTEXFORMAT_FLOAT2,
        - SG_VERTEXFORMAT_FLOAT3,
        - SG_VERTEXFORMAT_FLOAT4,
        - SG_VERTEXFORMAT_BYTE4N,
        - SG_VERTEXFORMAT_UBYTE4N,
        - SG_VERTEXFORMAT_SHORT2N,
        - SG_VERTEXFORMAT_USHORT2N
        - SG_VERTEXFORMAT_SHORT4N,
        - SG_VERTEXFORMAT_USHORT4N


    MEMORY ALLOCATION OVERRIDE
    ==========================
    You can override the memory allocation functions at initialization time
    like this:

        void* my_alloc(size_t size, void* user_data) {
            return malloc(size);
        }

        void my_free(void* ptr, void* user_data) {
            free(ptr);
        }

        ...
            sg_setup(&(sg_desc){
                // ...
                .allocator = {
                    .alloc = my_alloc,
                    .free = my_free,
                    .user_data = ...,
                }
            });
        ...

    If no overrides are provided, malloc and free will be used.

    This only affects memory allocation calls done by sokol_gfx.h
    itself though, not any allocations in OS libraries.


    LOG FUNCTION OVERRIDE
    =====================
    You can override the log function at initialization time like this:

        void my_log(const char* message, void* user_data) {
            printf("sg says: \s\n", message);
        }

        ...
            sg_setup(&(sg_desc){
                // ...
                .logger = {
                    .log_cb = my_log,
                    .user_data = ...,
                }
            });
        ...

    If no overrides are provided, puts will be used on most platforms.
    On Android, __android_log_write will be used instead.


    COMMIT LISTENERS
    ================
    It's possible to hook callback functions into sokol-gfx which are called from
    inside sg_commit() in unspecified order. This is mainly useful for libraries
    that build on top of sokol_gfx.h to be notified about the end/start of a frame.

    To add a commit listener, call:

        static void my_commit_listener(void* user_data) {
            ...
        }

        bool success = sg_add_commit_listener((sg_commit_listener){
            .func = my_commit_listener,
            .user_data = ...,
        });

    The function returns false if the internal array of commit listeners is full,
    or the same commit listener had already been added.

    If the function returns true, my_commit_listener() will be called each frame
    from inside sg_commit().

    By default, 1024 distinct commit listeners can be added, but this number
    can be tweaked in the sg_setup() call:

        sg_setup(&(sg_desc){
            .max_commit_listeners = 2048,
        });

    An sg_commit_listener item is equal to another if both the function
    pointer and user_data field are equal.

    To remove a commit listener:

        bool success = sg_remove_commit_listener((sg_commit_listener){
            .func = my_commit_listener,
            .user_data = ...,
        });

    ...where the .func and .user_data field are equal to a previous
    sg_add_commit_listener() call. The function returns true if the commit
    listener item was found and removed, and false otherwise.


    RESOURCE CREATION AND DESTRUCTION IN DETAIL
    ===========================================
    The 'vanilla' way to create resource objects is with the 'make functions':

        sg_buffer sg_make_buffer(const sg_buffer_desc* desc)
        sg_image sg_make_image(const sg_image_desc* desc)
        sg_shader sg_make_shader(const sg_shader_desc* desc)
        sg_pipeline sg_make_pipeline(const sg_pipeline_desc* desc)
        sg_pass sg_make_pass(const sg_pass_desc* desc)

    This will result in one of three cases:

        1. The returned handle is invalid. This happens when there are no more
           free slots in the resource pool for this resource type. An invalid
           handle is associated with the INVALID resource state, for instance:

                sg_buffer buf = sg_make_buffer(...)
                if (sg_query_buffer_state(buf) == SG_RESOURCESTATE_INVALID) {
                    // buffer pool is exhausted
                }

        2. The returned handle is valid, but creating the underlying resource
           has failed for some reason. This results in a resource object in the
           FAILED state. The reason *why* resource creation has failed differ
           by resource type. Look for log messages with more details. A failed
           resource state can be checked with:

                sg_buffer buf = sg_make_buffer(...)
                if (sg_query_buffer_state(buf) == SG_RESOURCESTATE_FAILED) {
                    // creating the resource has failed
                }

        3. And finally, if everything goes right, the returned resource is
           in resource state VALID and ready to use. This can be checked
           with:

                sg_buffer buf = sg_make_buffer(...)
                if (sg_query_buffer_state(buf) == SG_RESOURCESTATE_VALID) {
                    // creating the resource has failed
                }

    When calling the 'make functions', the created resource goes through a number
    of states:

        - INITIAL: the resource slot associated with the new resource is currently
          free (technically, there is no resource yet, just an empty pool slot)
        - ALLOC: a handle for the new resource has been allocated, this just means
          a pool slot has been reserved.
        - VALID or FAILED: in VALID state any 3D API backend resource objects have
          been successfully created, otherwise if anything went wrong, the resource
          will be in FAILED state.

    Sometimes it makes sense to first grab a handle, but initialize the
    underlying resource at a later time. For instance when loading data
    asynchronously from a slow data source, you may know what buffers and
    textures are needed at an early stage of the loading process, but actually
    loading the buffer or texture content can only be completed at a later time.

    For such situations, sokol-gfx resource objects can be created in two steps.
    You can allocate a handle upfront with one of the 'alloc functions':

        sg_buffer sg_alloc_buffer(void)
        sg_image sg_alloc_image(void)
        sg_shader sg_alloc_shader(void)
        sg_pipeline sg_alloc_pipeline(void)
        sg_pass sg_alloc_pass(void)

    This will return a handle with the underlying resource object in the
    ALLOC state:

        sg_image img = sg_alloc_image();
        if (sg_query_image_state(img) == SG_RESOURCESTATE_ALLOC) {
            // allocating an image handle has succeeded, otherwise
            // the image pool is full
        }

    Such an 'incomplete' handle can be used in most sokol-gfx rendering functions
    without doing any harm, sokol-gfx will simply skip any rendering operation
    that involve resources which are not in VALID state.

    At a later time (for instance once the texture has completed loading
    asynchronously), the resource creation can be completed by calling one of
    the 'init functions', those functions take an existing resource handle and
    'desc struct':

        void sg_init_buffer(sg_buffer buf, const sg_buffer_desc* desc)
        void sg_init_image(sg_image img, const sg_image_desc* desc)
        void sg_init_shader(sg_shader shd, const sg_shader_desc* desc)
        void sg_init_pipeline(sg_pipeline pip, const sg_pipeline_desc* desc)
        void sg_init_pass(sg_pass pass, const sg_pass_desc* desc)

    The init functions expect a resource in ALLOC state, and after the function
    returns, the resource will be either in VALID or FAILED state. Calling
    an 'alloc function' followed by the matching 'init function' is fully
    equivalent with calling the 'make function' alone.

    Destruction can also happen as a two-step process. The 'uninit functions'
    will put a resource object from the VALID or FAILED state back into the
    ALLOC state:

        void sg_uninit_buffer(sg_buffer buf)
        void sg_uninit_image(sg_image img)
        void sg_uninit_shader(sg_shader shd)
        void sg_uninit_pipeline(sg_pipeline pip)
        void sg_uninit_pass(sg_pass pass)

    Calling the 'uninit functions' with a resource that is not in the VALID or
    FAILED state is a no-op.

    To finally free the pool slot for recycling call the 'dealloc functions':

        void sg_dealloc_buffer(sg_buffer buf)
        void sg_dealloc_image(sg_image img)
        void sg_dealloc_shader(sg_shader shd)
        void sg_dealloc_pipeline(sg_pipeline pip)
        void sg_dealloc_pass(sg_pass pass)

    Calling the 'dealloc functions' on a resource that's not in ALLOC state is
    a no-op, but will generate a warning log message.

    Calling an 'uninit function' and 'dealloc function' in sequence is equivalent
    with calling the associated 'destroy function':

        void sg_destroy_buffer(sg_buffer buf)
        void sg_destroy_image(sg_image img)
        void sg_destroy_shader(sg_shader shd)
        void sg_destroy_pipeline(sg_pipeline pip)
        void sg_destroy_pass(sg_pass pass)

    The 'destroy functions' can be called on resources in any state and generally
    do the right thing (for instance if the resource is in ALLOC state, the destroy
    function will be equivalent to the 'dealloc function' and skip the 'uninit part').

    And finally to close the circle, the 'fail functions' can be called to manually
    put a resource in ALLOC state into the FAILED state:

        sg_fail_buffer(sg_buffer buf)
        sg_fail_image(sg_image img)
        sg_fail_shader(sg_shader shd)
        sg_fail_pipeline(sg_pipeline pip)
        sg_fail_pass(sg_pass pass)

    This is recommended if anything went wrong outside of sokol-gfx during asynchronous
    resource creation (for instance the file loading operation failed). In this case,
    the 'fail function' should be called instead of the 'init function'.

    Calling a 'fail function' on a resource that's not in ALLOC state is a no-op,
    but will generate a warning log message.

    NOTE: that two-step resource creation usually only makes sense for buffers
    and images, but not for shaders, pipelines or passes. Most notably, trying
    to create a pipeline object with a shader that's not in VALID state will
    trigger a validation layer error, or if the validation layer is disabled,
    result in a pipeline object in FAILED state. Same when trying to create
    a pass object with image invalid image objects.

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
#define SOKOL_GFX_INCLUDED (1)
#include <stddef.h>     // size_t
#include <stdint.h>
#include <stdbool.h>

#if defined(SOKOL_API_DECL) && !defined(SOKOL_GFX_API_DECL)
#define SOKOL_GFX_API_DECL SOKOL_API_DECL
#endif
#ifndef SOKOL_GFX_API_DECL
#if defined(_WIN32) && defined(SOKOL_DLL) && defined(SOKOL_GFX_IMPL)
#define SOKOL_GFX_API_DECL __declspec(dllexport)
#elif defined(_WIN32) && defined(SOKOL_DLL)
#define SOKOL_GFX_API_DECL __declspec(dllimport)
#else
#define SOKOL_GFX_API_DECL extern
#endif
#endif

#ifdef __cplusplus
extern "C" {
#endif

/*
    Resource id typedefs:

    sg_buffer:      vertex- and index-buffers
    sg_image:       textures and render targets
    sg_shader:      vertex- and fragment-shaders, uniform blocks
    sg_pipeline:    associated shader and vertex-layouts, and render states
    sg_pass:        a bundle of render targets and actions on them
    sg_context:     a 'context handle' for switching between 3D-API contexts

    Instead of pointers, resource creation functions return a 32-bit
    number which uniquely identifies the resource object.

    The 32-bit resource id is split into a 16-bit pool index in the lower bits,
    and a 16-bit 'unique counter' in the upper bits. The index allows fast
    pool lookups, and combined with the unique-mask it allows to detect
    'dangling accesses' (trying to use an object which no longer exists, and
    its pool slot has been reused for a new object)

    The resource ids are wrapped into a struct so that the compiler
    can complain when the wrong resource type is used.
*/
typedef struct sg_buffer   { uint32_t id; } sg_buffer;
typedef struct sg_image    { uint32_t id; } sg_image;
typedef struct sg_shader   { uint32_t id; } sg_shader;
typedef struct sg_pipeline { uint32_t id; } sg_pipeline;
typedef struct sg_pass     { uint32_t id; } sg_pass;
typedef struct sg_context  { uint32_t id; } sg_context;

/*
    sg_range is a pointer-size-pair struct used to pass memory blobs into
    sokol-gfx. When initialized from a value type (array or struct), you can
    use the SG_RANGE() macro to build an sg_range struct. For functions which
    take either a sg_range pointer, or a (C++) sg_range reference, use the
    SG_RANGE_REF macro as a solution which compiles both in C and C++.
*/
typedef struct sg_range {
    const void* ptr;
    size_t size;
} sg_range;

// disabling this for every includer isn't great, but the warnings are also quite pointless
#if defined(_MSC_VER)
#pragma warning(disable:4221)   /* /W4 only: nonstandard extension used: 'x': cannot be initialized using address of automatic variable 'y' */
#pragma warning(disable:4204)   /* VS2015: nonstandard extension used: non-constant aggregate initializer */
#endif
#if defined(__cplusplus)
#define SG_RANGE(x) sg_range{ &x, sizeof(x) }
#define SG_RANGE_REF(x) sg_range{ &x, sizeof(x) }
#else
#define SG_RANGE(x) (sg_range){ &x, sizeof(x) }
#define SG_RANGE_REF(x) &(sg_range){ &x, sizeof(x) }
#endif

//  various compile-time constants
enum {
    SG_INVALID_ID = 0,
    SG_NUM_SHADER_STAGES = 2,
    SG_NUM_INFLIGHT_FRAMES = 2,
    SG_MAX_COLOR_ATTACHMENTS = 4,
    SG_MAX_SHADERSTAGE_BUFFERS = 8,
    SG_MAX_SHADERSTAGE_IMAGES = 12,
    SG_MAX_SHADERSTAGE_UBS = 4,
    SG_MAX_UB_MEMBERS = 16,
    SG_MAX_VERTEX_ATTRIBUTES = 16,      /* NOTE: actual max vertex attrs can be less on GLES2, see sg_limits! */
    SG_MAX_MIPMAPS = 16,
    SG_MAX_TEXTUREARRAY_LAYERS = 128
};

/*
    sg_color

    An RGBA color value.
*/
typedef struct sg_color { float r, g, b, a; } sg_color;

/*
    sg_backend

    The active 3D-API backend, use the function sg_query_backend()
    to get the currently active backend.

    NOTE that SG_BACKEND_GLES2 will be returned if sokol-gfx was
    compiled with SOKOL_GLES3, but the runtime platform doesn't support
    GLES3/WebGL2 and sokol-gfx had to fallback to GLES2/WebGL.
*/
typedef enum sg_backend {
    SG_BACKEND_GLCORE33,
    SG_BACKEND_GLES2,
    SG_BACKEND_GLES3,
    SG_BACKEND_D3D11,
    SG_BACKEND_METAL_IOS,
    SG_BACKEND_METAL_MACOS,
    SG_BACKEND_METAL_SIMULATOR,
    SG_BACKEND_WGPU,
    SG_BACKEND_DUMMY,
} sg_backend;

/*
    sg_pixel_format

    sokol_gfx.h basically uses the same pixel formats as WebGPU, since these
    are supported on most newer GPUs. GLES2 and WebGL only supports a much
    smaller subset of actually available pixel formats. Call
    sg_query_pixelformat() to check at runtime if a pixel format supports the
    desired features.

    A pixelformat name consist of three parts:

        - components (R, RG, RGB or RGBA)
        - bit width per component (8, 16 or 32)
        - component data type:
            - unsigned normalized (no postfix)
            - signed normalized (SN postfix)
            - unsigned integer (UI postfix)
            - signed integer (SI postfix)
            - float (F postfix)

    Not all pixel formats can be used for everything, call sg_query_pixelformat()
    to inspect the capabilities of a given pixelformat. The function returns
    an sg_pixelformat_info struct with the following bool members:

        - sample: the pixelformat can be sampled as texture at least with
                  nearest filtering
        - filter: the pixelformat can be samples as texture with linear
                  filtering
        - render: the pixelformat can be used for render targets
        - blend:  blending is supported when using the pixelformat for
                  render targets
        - msaa:   multisample-antialiasing is supported when using the
                  pixelformat for render targets
        - depth:  the pixelformat can be used for depth-stencil attachments

    When targeting GLES2/WebGL, the only safe formats to use
    as texture are SG_PIXELFORMAT_R8 and SG_PIXELFORMAT_RGBA8. For rendering
    in GLES2/WebGL, only SG_PIXELFORMAT_RGBA8 is safe. All other formats
    must be checked via sg_query_pixelformats().

    The default pixel format for texture images is SG_PIXELFORMAT_RGBA8.

    The default pixel format for render target images is platform-dependent:
        - for Metal and D3D11 it is SG_PIXELFORMAT_BGRA8
        - for GL backends it is SG_PIXELFORMAT_RGBA8

    This is mainly because of the default framebuffer which is setup outside
    of sokol_gfx.h. On some backends, using BGRA for the default frame buffer
    allows more efficient frame flips. For your own offscreen-render-targets,
    use whatever renderable pixel format is convenient for you.
*/
typedef enum sg_pixel_format {
    _SG_PIXELFORMAT_DEFAULT,    /* value 0 reserved for default-init */
    SG_PIXELFORMAT_NONE,

    SG_PIXELFORMAT_R8,
    SG_PIXELFORMAT_R8SN,
    SG_PIXELFORMAT_R8UI,
    SG_PIXELFORMAT_R8SI,

    SG_PIXELFORMAT_R16,
    SG_PIXELFORMAT_R16SN,
    SG_PIXELFORMAT_R16UI,
    SG_PIXELFORMAT_R16SI,
    SG_PIXELFORMAT_R16F,
    SG_PIXELFORMAT_RG8,
    SG_PIXELFORMAT_RG8SN,
    SG_PIXELFORMAT_RG8UI,
    SG_PIXELFORMAT_RG8SI,

    SG_PIXELFORMAT_R32UI,
    SG_PIXELFORMAT_R32SI,
    SG_PIXELFORMAT_R32F,
    SG_PIXELFORMAT_RG16,
    SG_PIXELFORMAT_RG16SN,
    SG_PIXELFORMAT_RG16UI,
    SG_PIXELFORMAT_RG16SI,
    SG_PIXELFORMAT_RG16F,
    SG_PIXELFORMAT_RGBA8,
    SG_PIXELFORMAT_RGBA8SN,
    SG_PIXELFORMAT_RGBA8UI,
    SG_PIXELFORMAT_RGBA8SI,
    SG_PIXELFORMAT_BGRA8,
    SG_PIXELFORMAT_RGB10A2,
    SG_PIXELFORMAT_RG11B10F,

    SG_PIXELFORMAT_RG32UI,
    SG_PIXELFORMAT_RG32SI,
    SG_PIXELFORMAT_RG32F,
    SG_PIXELFORMAT_RGBA16,
    SG_PIXELFORMAT_RGBA16SN,
    SG_PIXELFORMAT_RGBA16UI,
    SG_PIXELFORMAT_RGBA16SI,
    SG_PIXELFORMAT_RGBA16F,

    SG_PIXELFORMAT_RGBA32UI,
    SG_PIXELFORMAT_RGBA32SI,
    SG_PIXELFORMAT_RGBA32F,

    SG_PIXELFORMAT_DEPTH,
    SG_PIXELFORMAT_DEPTH_STENCIL,

    SG_PIXELFORMAT_BC1_RGBA,
    SG_PIXELFORMAT_BC2_RGBA,
    SG_PIXELFORMAT_BC3_RGBA,
    SG_PIXELFORMAT_BC4_R,
    SG_PIXELFORMAT_BC4_RSN,
    SG_PIXELFORMAT_BC5_RG,
    SG_PIXELFORMAT_BC5_RGSN,
    SG_PIXELFORMAT_BC6H_RGBF,
    SG_PIXELFORMAT_BC6H_RGBUF,
    SG_PIXELFORMAT_BC7_RGBA,
    SG_PIXELFORMAT_PVRTC_RGB_2BPP,
    SG_PIXELFORMAT_PVRTC_RGB_4BPP,
    SG_PIXELFORMAT_PVRTC_RGBA_2BPP,
    SG_PIXELFORMAT_PVRTC_RGBA_4BPP,
    SG_PIXELFORMAT_ETC2_RGB8,
    SG_PIXELFORMAT_ETC2_RGB8A1,
    SG_PIXELFORMAT_ETC2_RGBA8,
    SG_PIXELFORMAT_ETC2_RG11,
    SG_PIXELFORMAT_ETC2_RG11SN,

    SG_PIXELFORMAT_RGB9E5,

    _SG_PIXELFORMAT_NUM,
    _SG_PIXELFORMAT_FORCE_U32 = 0x7FFFFFFF
} sg_pixel_format;

/*
    Runtime information about a pixel format, returned
    by sg_query_pixelformat().
*/
typedef struct sg_pixelformat_info {
    bool sample;        // pixel format can be sampled in shaders
    bool filter;        // pixel format can be sampled with filtering
    bool render;        // pixel format can be used as render target
    bool blend;         // alpha-blending is supported
    bool msaa;          // pixel format can be used as MSAA render target
    bool depth;         // pixel format is a depth format
    #if defined(SOKOL_ZIG_BINDINGS)
    uint32_t __pad[3];
    #endif
} sg_pixelformat_info;

/*
    Runtime information about available optional features,
    returned by sg_query_features()
*/
typedef struct sg_features {
    bool instancing;                    // hardware instancing supported
    bool origin_top_left;               // framebuffer and texture origin is in top left corner
    bool multiple_render_targets;       // offscreen render passes can have multiple render targets attached
    bool msaa_render_targets;           // offscreen render passes support MSAA antialiasing
    bool imagetype_3d;                  // creation of SG_IMAGETYPE_3D images is supported
    bool imagetype_array;               // creation of SG_IMAGETYPE_ARRAY images is supported
    bool image_clamp_to_border;         // border color and clamp-to-border UV-wrap mode is supported
    bool mrt_independent_blend_state;   // multiple-render-target rendering can use per-render-target blend state
    bool mrt_independent_write_mask;    // multiple-render-target rendering can use per-render-target color write masks
    #if defined(SOKOL_ZIG_BINDINGS)
    uint32_t __pad[3];
    #endif
} sg_features;

/*
    Runtime information about resource limits, returned by sg_query_limit()
*/
typedef struct sg_limits {
    int max_image_size_2d;          // max width/height of SG_IMAGETYPE_2D images
    int max_image_size_cube;        // max width/height of SG_IMAGETYPE_CUBE images
    int max_image_size_3d;          // max width/height/depth of SG_IMAGETYPE_3D images
    int max_image_size_array;       // max width/height of SG_IMAGETYPE_ARRAY images
    int max_image_array_layers;     // max number of layers in SG_IMAGETYPE_ARRAY images
    int max_vertex_attrs;           // <= SG_MAX_VERTEX_ATTRIBUTES or less (on some GLES2 impls)
    int gl_max_vertex_uniform_vectors;  // <= GL_MAX_VERTEX_UNIFORM_VECTORS (only on GL backends)
} sg_limits;

/*
    sg_resource_state

    The current state of a resource in its resource pool.
    Resources start in the INITIAL state, which means the
    pool slot is unoccupied and can be allocated. When a resource is
    created, first an id is allocated, and the resource pool slot
    is set to state ALLOC. After allocation, the resource is
    initialized, which may result in the VALID or FAILED state. The
    reason why allocation and initialization are separate is because
    some resource types (e.g. buffers and images) might be asynchronously
    initialized by the user application. If a resource which is not
    in the VALID state is attempted to be used for rendering, rendering
    operations will silently be dropped.

    The special INVALID state is returned in sg_query_xxx_state() if no
    resource object exists for the provided resource id.
*/
typedef enum sg_resource_state {
    SG_RESOURCESTATE_INITIAL,
    SG_RESOURCESTATE_ALLOC,
    SG_RESOURCESTATE_VALID,
    SG_RESOURCESTATE_FAILED,
    SG_RESOURCESTATE_INVALID,
    _SG_RESOURCESTATE_FORCE_U32 = 0x7FFFFFFF
} sg_resource_state;

/*
    sg_usage

    A resource usage hint describing the update strategy of
    buffers and images. This is used in the sg_buffer_desc.usage
    and sg_image_desc.usage members when creating buffers
    and images:

    SG_USAGE_IMMUTABLE:     the resource will never be updated with
                            new data, instead the content of the
                            resource must be provided on creation
    SG_USAGE_DYNAMIC:       the resource will be updated infrequently
                            with new data (this could range from "once
                            after creation", to "quite often but not
                            every frame")
    SG_USAGE_STREAM:        the resource will be updated each frame
                            with new content

    The rendering backends use this hint to prevent that the
    CPU needs to wait for the GPU when attempting to update
    a resource that might be currently accessed by the GPU.

    Resource content is updated with the functions sg_update_buffer() or
    sg_append_buffer() for buffer objects, and sg_update_image() for image
    objects. For the sg_update_*() functions, only one update is allowed per
    frame and resource object, while sg_append_buffer() can be called
    multiple times per frame on the same buffer. The application must update
    all data required for rendering (this means that the update data can be
    smaller than the resource size, if only a part of the overall resource
    size is used for rendering, you only need to make sure that the data that
    *is* used is valid).

    The default usage is SG_USAGE_IMMUTABLE.
*/
typedef enum sg_usage {
    _SG_USAGE_DEFAULT,      /* value 0 reserved for default-init */
    SG_USAGE_IMMUTABLE,
    SG_USAGE_DYNAMIC,
    SG_USAGE_STREAM,
    _SG_USAGE_NUM,
    _SG_USAGE_FORCE_U32 = 0x7FFFFFFF
} sg_usage;

/*
    sg_buffer_type

    This indicates whether a buffer contains vertex- or index-data,
    used in the sg_buffer_desc.type member when creating a buffer.

    The default value is SG_BUFFERTYPE_VERTEXBUFFER.
*/
typedef enum sg_buffer_type {
    _SG_BUFFERTYPE_DEFAULT,         /* value 0 reserved for default-init */
    SG_BUFFERTYPE_VERTEXBUFFER,
    SG_BUFFERTYPE_INDEXBUFFER,
    _SG_BUFFERTYPE_NUM,
    _SG_BUFFERTYPE_FORCE_U32 = 0x7FFFFFFF
} sg_buffer_type;

/*
    sg_index_type

    Indicates whether indexed rendering (fetching vertex-indices from an
    index buffer) is used, and if yes, the index data type (16- or 32-bits).
    This is used in the sg_pipeline_desc.index_type member when creating a
    pipeline object.

    The default index type is SG_INDEXTYPE_NONE.
*/
typedef enum sg_index_type {
    _SG_INDEXTYPE_DEFAULT,   /* value 0 reserved for default-init */
    SG_INDEXTYPE_NONE,
    SG_INDEXTYPE_UINT16,
    SG_INDEXTYPE_UINT32,
    _SG_INDEXTYPE_NUM,
    _SG_INDEXTYPE_FORCE_U32 = 0x7FFFFFFF
} sg_index_type;

/*
    sg_image_type

    Indicates the basic type of an image object (2D-texture, cubemap,
    3D-texture or 2D-array-texture). 3D- and array-textures are not supported
    on the GLES2/WebGL backend (use sg_query_features().imagetype_3d and
    sg_query_features().imagetype_array to check for support). The image type
    is used in the sg_image_desc.type member when creating an image, and
    in sg_shader_image_desc when describing a shader's texture sampler binding.

    The default image type when creating an image is SG_IMAGETYPE_2D.
*/
typedef enum sg_image_type {
    _SG_IMAGETYPE_DEFAULT,  /* value 0 reserved for default-init */
    SG_IMAGETYPE_2D,
    SG_IMAGETYPE_CUBE,
    SG_IMAGETYPE_3D,
    SG_IMAGETYPE_ARRAY,
    _SG_IMAGETYPE_NUM,
    _SG_IMAGETYPE_FORCE_U32 = 0x7FFFFFFF
} sg_image_type;

/*
    sg_sampler_type

    Indicates the basic data type of a shader's texture sampler which
    can be float , unsigned integer or signed integer. The sampler
    type is used in the sg_shader_image_desc to describe the
    sampler type of a shader's texture sampler binding.

    The default sampler type is SG_SAMPLERTYPE_FLOAT.
*/
typedef enum sg_sampler_type {
    _SG_SAMPLERTYPE_DEFAULT,  /* value 0 reserved for default-init */
    SG_SAMPLERTYPE_FLOAT,
    SG_SAMPLERTYPE_SINT,
    SG_SAMPLERTYPE_UINT,
} sg_sampler_type;

/*
    sg_cube_face

    The cubemap faces. Use these as indices in the sg_image_desc.content
    array.
*/
typedef enum sg_cube_face {
    SG_CUBEFACE_POS_X,
    SG_CUBEFACE_NEG_X,
    SG_CUBEFACE_POS_Y,
    SG_CUBEFACE_NEG_Y,
    SG_CUBEFACE_POS_Z,
    SG_CUBEFACE_NEG_Z,
    SG_CUBEFACE_NUM,
    _SG_CUBEFACE_FORCE_U32 = 0x7FFFFFFF
} sg_cube_face;

/*
    sg_shader_stage

    There are 2 shader stages: vertex- and fragment-shader-stage.
    Each shader stage consists of:

    - one slot for a shader function (provided as source- or byte-code)
    - SG_MAX_SHADERSTAGE_UBS slots for uniform blocks
    - SG_MAX_SHADERSTAGE_IMAGES slots for images used as textures by
      the shader function
*/
typedef enum sg_shader_stage {
    SG_SHADERSTAGE_VS,
    SG_SHADERSTAGE_FS,
    _SG_SHADERSTAGE_FORCE_U32 = 0x7FFFFFFF
} sg_shader_stage;

/*
    sg_primitive_type

    This is the common subset of 3D primitive types supported across all 3D
    APIs. This is used in the sg_pipeline_desc.primitive_type member when
    creating a pipeline object.

    The default primitive type is SG_PRIMITIVETYPE_TRIANGLES.
*/
typedef enum sg_primitive_type {
    _SG_PRIMITIVETYPE_DEFAULT,  /* value 0 reserved for default-init */
    SG_PRIMITIVETYPE_POINTS,
    SG_PRIMITIVETYPE_LINES,
    SG_PRIMITIVETYPE_LINE_STRIP,
    SG_PRIMITIVETYPE_TRIANGLES,
    SG_PRIMITIVETYPE_TRIANGLE_STRIP,
    _SG_PRIMITIVETYPE_NUM,
    _SG_PRIMITIVETYPE_FORCE_U32 = 0x7FFFFFFF
} sg_primitive_type;

/*
    sg_filter

    The filtering mode when sampling a texture image. This is
    used in the sg_image_desc.min_filter and sg_image_desc.mag_filter
    members when creating an image object.

    The default filter mode is SG_FILTER_NEAREST.
*/
typedef enum sg_filter {
    _SG_FILTER_DEFAULT, /* value 0 reserved for default-init */
    SG_FILTER_NEAREST,
    SG_FILTER_LINEAR,
    SG_FILTER_NEAREST_MIPMAP_NEAREST,
    SG_FILTER_NEAREST_MIPMAP_LINEAR,
    SG_FILTER_LINEAR_MIPMAP_NEAREST,
    SG_FILTER_LINEAR_MIPMAP_LINEAR,
    _SG_FILTER_NUM,
    _SG_FILTER_FORCE_U32 = 0x7FFFFFFF
} sg_filter;

/*
    sg_wrap

    The texture coordinates wrapping mode when sampling a texture
    image. This is used in the sg_image_desc.wrap_u, .wrap_v
    and .wrap_w members when creating an image.

    The default wrap mode is SG_WRAP_REPEAT.

    NOTE: SG_WRAP_CLAMP_TO_BORDER is not supported on all backends
    and platforms. To check for support, call sg_query_features()
    and check the "clamp_to_border" boolean in the returned
    sg_features struct.

    Platforms which don't support SG_WRAP_CLAMP_TO_BORDER will silently fall back
    to SG_WRAP_CLAMP_TO_EDGE without a validation error.

    Platforms which support clamp-to-border are:

        - all desktop GL platforms
        - Metal on macOS
        - D3D11

    Platforms which do not support clamp-to-border:

        - GLES2/3 and WebGL/WebGL2
        - Metal on iOS
*/
typedef enum sg_wrap {
    _SG_WRAP_DEFAULT,   /* value 0 reserved for default-init */
    SG_WRAP_REPEAT,
    SG_WRAP_CLAMP_TO_EDGE,
    SG_WRAP_CLAMP_TO_BORDER,
    SG_WRAP_MIRRORED_REPEAT,
    _SG_WRAP_NUM,
    _SG_WRAP_FORCE_U32 = 0x7FFFFFFF
} sg_wrap;

/*
    sg_border_color

    The border color to use when sampling a texture, and the UV wrap
    mode is SG_WRAP_CLAMP_TO_BORDER.

    The default border color is SG_BORDERCOLOR_OPAQUE_BLACK
*/
typedef enum sg_border_color {
    _SG_BORDERCOLOR_DEFAULT,    /* value 0 reserved for default-init */
    SG_BORDERCOLOR_TRANSPARENT_BLACK,
    SG_BORDERCOLOR_OPAQUE_BLACK,
    SG_BORDERCOLOR_OPAQUE_WHITE,
    _SG_BORDERCOLOR_NUM,
    _SG_BORDERCOLOR_FORCE_U32 = 0x7FFFFFFF
} sg_border_color;

/*
    sg_vertex_format

    The data type of a vertex component. This is used to describe
    the layout of vertex data when creating a pipeline object.
*/
typedef enum sg_vertex_format {
    SG_VERTEXFORMAT_INVALID,
    SG_VERTEXFORMAT_FLOAT,
    SG_VERTEXFORMAT_FLOAT2,
    SG_VERTEXFORMAT_FLOAT3,
    SG_VERTEXFORMAT_FLOAT4,
    SG_VERTEXFORMAT_BYTE4,
    SG_VERTEXFORMAT_BYTE4N,
    SG_VERTEXFORMAT_UBYTE4,
    SG_VERTEXFORMAT_UBYTE4N,
    SG_VERTEXFORMAT_SHORT2,
    SG_VERTEXFORMAT_SHORT2N,
    SG_VERTEXFORMAT_USHORT2N,
    SG_VERTEXFORMAT_SHORT4,
    SG_VERTEXFORMAT_SHORT4N,
    SG_VERTEXFORMAT_USHORT4N,
    SG_VERTEXFORMAT_UINT10_N2,
    _SG_VERTEXFORMAT_NUM,
    _SG_VERTEXFORMAT_FORCE_U32 = 0x7FFFFFFF
} sg_vertex_format;

/*
    sg_vertex_step

    Defines whether the input pointer of a vertex input stream is advanced
    'per vertex' or 'per instance'. The default step-func is
    SG_VERTEXSTEP_PER_VERTEX. SG_VERTEXSTEP_PER_INSTANCE is used with
    instanced-rendering.

    The vertex-step is part of the vertex-layout definition
    when creating pipeline objects.
*/
typedef enum sg_vertex_step {
    _SG_VERTEXSTEP_DEFAULT,     /* value 0 reserved for default-init */
    SG_VERTEXSTEP_PER_VERTEX,
    SG_VERTEXSTEP_PER_INSTANCE,
    _SG_VERTEXSTEP_NUM,
    _SG_VERTEXSTEP_FORCE_U32 = 0x7FFFFFFF
} sg_vertex_step;

/*
    sg_uniform_type

    The data type of a uniform block member. This is used to
    describe the internal layout of uniform blocks when creating
    a shader object.
*/
typedef enum sg_uniform_type {
    SG_UNIFORMTYPE_INVALID,
    SG_UNIFORMTYPE_FLOAT,
    SG_UNIFORMTYPE_FLOAT2,
    SG_UNIFORMTYPE_FLOAT3,
    SG_UNIFORMTYPE_FLOAT4,
    SG_UNIFORMTYPE_INT,
    SG_UNIFORMTYPE_INT2,
    SG_UNIFORMTYPE_INT3,
    SG_UNIFORMTYPE_INT4,
    SG_UNIFORMTYPE_MAT4,
    _SG_UNIFORMTYPE_NUM,
    _SG_UNIFORMTYPE_FORCE_U32 = 0x7FFFFFFF
} sg_uniform_type;

/*
    sg_uniform_layout

    A hint for the interior memory layout of uniform blocks. This is
    only really relevant for the GL backend where the internal layout
    of uniform blocks must be known to sokol-gfx. For all other backends the
    internal memory layout of uniform blocks doesn't matter, sokol-gfx
    will just pass uniform data as a single memory blob to the
    3D backend.

    SG_UNIFORMLAYOUT_NATIVE (default)
        Native layout means that a 'backend-native' memory layout
        is used. For the GL backend this means that uniforms
        are packed tightly in memory (e.g. there are no padding
        bytes).

    SG_UNIFORMLAYOUT_STD140
        The memory layout is a subset of std140. Arrays are only
        allowed for the FLOAT4, INT4 and MAT4. Alignment is as
        is as follows:

            FLOAT, INT:         4 byte alignment
            FLOAT2, INT2:       8 byte alignment
            FLOAT3, INT3:       16 byte alignment(!)
            FLOAT4, INT4:       16 byte alignment
            MAT4:               16 byte alignment
            FLOAT4[], INT4[]:   16 byte alignment

        The overall size of the uniform block must be a multiple
        of 16.

    For more information search for 'UNIFORM DATA LAYOUT' in the documentation block
    at the start of the header.
*/
typedef enum sg_uniform_layout {
    _SG_UNIFORMLAYOUT_DEFAULT,     /* value 0 reserved for default-init */
    SG_UNIFORMLAYOUT_NATIVE,       /* default: layout depends on currently active backend */
    SG_UNIFORMLAYOUT_STD140,       /* std140: memory layout according to std140 */
    _SG_UNIFORMLAYOUT_NUM,
    _SG_UNIFORMLAYOUT_FORCE_U32 = 0x7FFFFFFF
} sg_uniform_layout;

/*
    sg_cull_mode

    The face-culling mode, this is used in the
    sg_pipeline_desc.cull_mode member when creating a
    pipeline object.

    The default cull mode is SG_CULLMODE_NONE
*/
typedef enum sg_cull_mode {
    _SG_CULLMODE_DEFAULT,   /* value 0 reserved for default-init */
    SG_CULLMODE_NONE,
    SG_CULLMODE_FRONT,
    SG_CULLMODE_BACK,
    _SG_CULLMODE_NUM,
    _SG_CULLMODE_FORCE_U32 = 0x7FFFFFFF
} sg_cull_mode;

/*
    sg_face_winding

    The vertex-winding rule that determines a front-facing primitive. This
    is used in the member sg_pipeline_desc.face_winding
    when creating a pipeline object.

    The default winding is SG_FACEWINDING_CW (clockwise)
*/
typedef enum sg_face_winding {
    _SG_FACEWINDING_DEFAULT,    /* value 0 reserved for default-init */
    SG_FACEWINDING_CCW,
    SG_FACEWINDING_CW,
    _SG_FACEWINDING_NUM,
    _SG_FACEWINDING_FORCE_U32 = 0x7FFFFFFF
} sg_face_winding;

/*
    sg_compare_func

    The compare-function for depth- and stencil-ref tests.
    This is used when creating pipeline objects in the members:

    sg_pipeline_desc
        .depth
            .compare
        .stencil
            .front.compare
            .back.compar

    The default compare func for depth- and stencil-tests is
    SG_COMPAREFUNC_ALWAYS.
*/
typedef enum sg_compare_func {
    _SG_COMPAREFUNC_DEFAULT,    /* value 0 reserved for default-init */
    SG_COMPAREFUNC_NEVER,
    SG_COMPAREFUNC_LESS,
    SG_COMPAREFUNC_EQUAL,
    SG_COMPAREFUNC_LESS_EQUAL,
    SG_COMPAREFUNC_GREATER,
    SG_COMPAREFUNC_NOT_EQUAL,
    SG_COMPAREFUNC_GREATER_EQUAL,
    SG_COMPAREFUNC_ALWAYS,
    _SG_COMPAREFUNC_NUM,
    _SG_COMPAREFUNC_FORCE_U32 = 0x7FFFFFFF
} sg_compare_func;

/*
    sg_stencil_op

    The operation performed on a currently stored stencil-value when a
    comparison test passes or fails. This is used when creating a pipeline
    object in the members:

    sg_pipeline_desc
        .stencil
            .front
                .fail_op
                .depth_fail_op
                .pass_op
            .back
                .fail_op
                .depth_fail_op
                .pass_op

    The default value is SG_STENCILOP_KEEP.
*/
typedef enum sg_stencil_op {
    _SG_STENCILOP_DEFAULT,      /* value 0 reserved for default-init */
    SG_STENCILOP_KEEP,
    SG_STENCILOP_ZERO,
    SG_STENCILOP_REPLACE,
    SG_STENCILOP_INCR_CLAMP,
    SG_STENCILOP_DECR_CLAMP,
    SG_STENCILOP_INVERT,
    SG_STENCILOP_INCR_WRAP,
    SG_STENCILOP_DECR_WRAP,
    _SG_STENCILOP_NUM,
    _SG_STENCILOP_FORCE_U32 = 0x7FFFFFFF
} sg_stencil_op;

/*
    sg_blend_factor

    The source and destination factors in blending operations.
    This is used in the following members when creating a pipeline object:

    sg_pipeline_desc
        .colors[i]
            .blend
                .src_factor_rgb
                .dst_factor_rgb
                .src_factor_alpha
                .dst_factor_alpha

    The default value is SG_BLENDFACTOR_ONE for source
    factors, and SG_BLENDFACTOR_ZERO for destination factors.
*/
typedef enum sg_blend_factor {
    _SG_BLENDFACTOR_DEFAULT,    /* value 0 reserved for default-init */
    SG_BLENDFACTOR_ZERO,
    SG_BLENDFACTOR_ONE,
    SG_BLENDFACTOR_SRC_COLOR,
    SG_BLENDFACTOR_ONE_MINUS_SRC_COLOR,
    SG_BLENDFACTOR_SRC_ALPHA,
    SG_BLENDFACTOR_ONE_MINUS_SRC_ALPHA,
    SG_BLENDFACTOR_DST_COLOR,
    SG_BLENDFACTOR_ONE_MINUS_DST_COLOR,
    SG_BLENDFACTOR_DST_ALPHA,
    SG_BLENDFACTOR_ONE_MINUS_DST_ALPHA,
    SG_BLENDFACTOR_SRC_ALPHA_SATURATED,
    SG_BLENDFACTOR_BLEND_COLOR,
    SG_BLENDFACTOR_ONE_MINUS_BLEND_COLOR,
    SG_BLENDFACTOR_BLEND_ALPHA,
    SG_BLENDFACTOR_ONE_MINUS_BLEND_ALPHA,
    _SG_BLENDFACTOR_NUM,
    _SG_BLENDFACTOR_FORCE_U32 = 0x7FFFFFFF
} sg_blend_factor;

/*
    sg_blend_op

    Describes how the source and destination values are combined in the
    fragment blending operation. It is used in the following members when
    creating a pipeline object:

    sg_pipeline_desc
        .colors[i]
            .blend
                .op_rgb
                .op_alpha

    The default value is SG_BLENDOP_ADD.
*/
typedef enum sg_blend_op {
    _SG_BLENDOP_DEFAULT,    /* value 0 reserved for default-init */
    SG_BLENDOP_ADD,
    SG_BLENDOP_SUBTRACT,
    SG_BLENDOP_REVERSE_SUBTRACT,
    _SG_BLENDOP_NUM,
    _SG_BLENDOP_FORCE_U32 = 0x7FFFFFFF
} sg_blend_op;

/*
    sg_color_mask

    Selects the active color channels when writing a fragment color to the
    framebuffer. This is used in the members
    sg_pipeline_desc.colors[i].write_mask when creating a pipeline object.

    The default colormask is SG_COLORMASK_RGBA (write all colors channels)

    NOTE: since the color mask value 0 is reserved for the default value
    (SG_COLORMASK_RGBA), use SG_COLORMASK_NONE if all color channels
    should be disabled.
*/
typedef enum sg_color_mask {
    _SG_COLORMASK_DEFAULT = 0,    /* value 0 reserved for default-init */
    SG_COLORMASK_NONE   = 0x10,   /* special value for 'all channels disabled */
    SG_COLORMASK_R      = 0x1,
    SG_COLORMASK_G      = 0x2,
    SG_COLORMASK_RG     = 0x3,
    SG_COLORMASK_B      = 0x4,
    SG_COLORMASK_RB     = 0x5,
    SG_COLORMASK_GB     = 0x6,
    SG_COLORMASK_RGB    = 0x7,
    SG_COLORMASK_A      = 0x8,
    SG_COLORMASK_RA     = 0x9,
    SG_COLORMASK_GA     = 0xA,
    SG_COLORMASK_RGA    = 0xB,
    SG_COLORMASK_BA     = 0xC,
    SG_COLORMASK_RBA    = 0xD,
    SG_COLORMASK_GBA    = 0xE,
    SG_COLORMASK_RGBA   = 0xF,
    _SG_COLORMASK_FORCE_U32 = 0x7FFFFFFF
} sg_color_mask;

/*
    sg_action

    Defines what action should be performed at the start of a render pass:

    SG_ACTION_CLEAR:    clear the render target image
    SG_ACTION_LOAD:     load the previous content of the render target image
    SG_ACTION_DONTCARE: leave the render target image content undefined

    This is used in the sg_pass_action structure.

    The default action for all pass attachments is SG_ACTION_CLEAR, with the
    clear color rgba = {0.5f, 0.5f, 0.5f, 1.0f], depth=1.0 and stencil=0.

    If you want to override the default behaviour, it is important to not
    only set the clear color, but the 'action' field as well (as long as this
    is in its _SG_ACTION_DEFAULT, the value fields will be ignored).
*/
typedef enum sg_action {
    _SG_ACTION_DEFAULT,
    SG_ACTION_CLEAR,
    SG_ACTION_LOAD,
    SG_ACTION_DONTCARE,
    _SG_ACTION_NUM,
    _SG_ACTION_FORCE_U32 = 0x7FFFFFFF
} sg_action;

/*
    sg_pass_action

    The sg_pass_action struct defines the actions to be performed
    at the start of a rendering pass in the functions sg_begin_pass()
    and sg_begin_default_pass().

    A separate action and clear values can be defined for each
    color attachment, and for the depth-stencil attachment.

    The default clear values are defined by the macros:

    - SG_DEFAULT_CLEAR_RED:     0.5f
    - SG_DEFAULT_CLEAR_GREEN:   0.5f
    - SG_DEFAULT_CLEAR_BLUE:    0.5f
    - SG_DEFAULT_CLEAR_ALPHA:   1.0f
    - SG_DEFAULT_CLEAR_DEPTH:   1.0f
    - SG_DEFAULT_CLEAR_STENCIL: 0
*/
typedef struct sg_color_attachment_action {
    sg_action action;
    sg_color value;
} sg_color_attachment_action;

typedef struct sg_depth_attachment_action {
    sg_action action;
    float value;
} sg_depth_attachment_action;

typedef struct sg_stencil_attachment_action {
    sg_action action;
    uint8_t value;
} sg_stencil_attachment_action;

typedef struct sg_pass_action {
    uint32_t _start_canary;
    sg_color_attachment_action colors[SG_MAX_COLOR_ATTACHMENTS];
    sg_depth_attachment_action depth;
    sg_stencil_attachment_action stencil;
    uint32_t _end_canary;
} sg_pass_action;

/*
    sg_bindings

    The sg_bindings structure defines the resource binding slots
    of the sokol_gfx render pipeline, used as argument to the
    sg_apply_bindings() function.

    A resource binding struct contains:

    - 1..N vertex buffers
    - 0..N vertex buffer offsets
    - 0..1 index buffers
    - 0..1 index buffer offsets
    - 0..N vertex shader stage images
    - 0..N fragment shader stage images

    The max number of vertex buffer and shader stage images
    are defined by the SG_MAX_SHADERSTAGE_BUFFERS and
    SG_MAX_SHADERSTAGE_IMAGES configuration constants.

    The optional buffer offsets can be used to put different unrelated
    chunks of vertex- and/or index-data into the same buffer objects.
*/
typedef struct sg_bindings {
    uint32_t _start_canary;
    sg_buffer vertex_buffers[SG_MAX_SHADERSTAGE_BUFFERS];
    int vertex_buffer_offsets[SG_MAX_SHADERSTAGE_BUFFERS];
    sg_buffer index_buffer;
    int index_buffer_offset;
    sg_image vs_images[SG_MAX_SHADERSTAGE_IMAGES];
    sg_image fs_images[SG_MAX_SHADERSTAGE_IMAGES];
    uint32_t _end_canary;
} sg_bindings;

/*
    sg_buffer_desc

    Creation parameters for sg_buffer objects, used in the
    sg_make_buffer() call.

    The default configuration is:

    .size:      0       (*must* be >0 for buffers without data)
    .type:      SG_BUFFERTYPE_VERTEXBUFFER
    .usage:     SG_USAGE_IMMUTABLE
    .data.ptr   0       (*must* be valid for immutable buffers)
    .data.size  0       (*must* be > 0 for immutable buffers)
    .label      0       (optional string label for trace hooks)

    The label will be ignored by sokol_gfx.h, it is only useful
    when hooking into sg_make_buffer() or sg_init_buffer() via
    the sg_install_trace_hooks() function.

    For immutable buffers which are initialized with initial data,
    keep the .size item zero-initialized, and set the size together with the
    pointer to the initial data in the .data item.

    For mutable buffers without initial data, keep the .data item
    zero-initialized, and set the buffer size in the .size item instead.

    You can also set both size values, but currently both size values must
    be identical (this may change in the future when the dynamic resource
    management may become more flexible).

    ADVANCED TOPIC: Injecting native 3D-API buffers:

    The following struct members allow to inject your own GL, Metal
    or D3D11 buffers into sokol_gfx:

    .gl_buffers[SG_NUM_INFLIGHT_FRAMES]
    .mtl_buffers[SG_NUM_INFLIGHT_FRAMES]
    .d3d11_buffer

    You must still provide all other struct items except the .data item, and
    these must match the creation parameters of the native buffers you
    provide. For SG_USAGE_IMMUTABLE, only provide a single native 3D-API
    buffer, otherwise you need to provide SG_NUM_INFLIGHT_FRAMES buffers
    (only for GL and Metal, not D3D11). Providing multiple buffers for GL and
    Metal is necessary because sokol_gfx will rotate through them when
    calling sg_update_buffer() to prevent lock-stalls.

    Note that it is expected that immutable injected buffer have already been
    initialized with content, and the .content member must be 0!

    Also you need to call sg_reset_state_cache() after calling native 3D-API
    functions, and before calling any sokol_gfx function.
*/
typedef struct sg_buffer_desc {
    uint32_t _start_canary;
    size_t size;
    sg_buffer_type type;
    sg_usage usage;
    sg_range data;
    const char* label;
    /* GL specific */
    uint32_t gl_buffers[SG_NUM_INFLIGHT_FRAMES];
    /* Metal specific */
    const void* mtl_buffers[SG_NUM_INFLIGHT_FRAMES];
    /* D3D11 specific */
    const void* d3d11_buffer;
    /* WebGPU specific */
    const void* wgpu_buffer;
    uint32_t _end_canary;
} sg_buffer_desc;

/*
    sg_image_data

    Defines the content of an image through a 2D array of sg_range structs.
    The first array dimension is the cubemap face, and the second array
    dimension the mipmap level.
*/
typedef struct sg_image_data {
    sg_range subimage[SG_CUBEFACE_NUM][SG_MAX_MIPMAPS];
} sg_image_data;

/*
    sg_image_desc

    Creation parameters for sg_image objects, used in the sg_make_image()
    call.

    The default configuration is:

    .type:              SG_IMAGETYPE_2D
    .render_target:     false
    .width              0 (must be set to >0)
    .height             0 (must be set to >0)
    .num_slices         1 (3D textures: depth; array textures: number of layers)
    .num_mipmaps:       1
    .usage:             SG_USAGE_IMMUTABLE
    .pixel_format:      SG_PIXELFORMAT_RGBA8 for textures, or sg_desc.context.color_format for render targets
    .sample_count:      1 for textures, or sg_desc.context.sample_count for render targets
    .min_filter:        SG_FILTER_NEAREST
    .mag_filter:        SG_FILTER_NEAREST
    .wrap_u:            SG_WRAP_REPEAT
    .wrap_v:            SG_WRAP_REPEAT
    .wrap_w:            SG_WRAP_REPEAT (only SG_IMAGETYPE_3D)
    .border_color       SG_BORDERCOLOR_OPAQUE_BLACK
    .max_anisotropy     1 (must be 1..16)
    .min_lod            0.0f
    .max_lod            FLT_MAX
    .data               an sg_image_data struct to define the initial content
    .label              0       (optional string label for trace hooks)

    Q: Why is the default sample_count for render targets identical with the
    "default sample count" from sg_desc.context.sample_count?

    A: So that it matches the default sample count in pipeline objects. Even
    though it is a bit strange/confusing that offscreen render targets by default
    get the same sample count as the default framebuffer, but it's better that
    an offscreen render target created with default parameters matches
    a pipeline object created with default parameters.

    NOTE:

    SG_IMAGETYPE_ARRAY and SG_IMAGETYPE_3D are not supported on WebGL/GLES2,
    use sg_query_features().imagetype_array and
    sg_query_features().imagetype_3d at runtime to check if array- and
    3D-textures are supported.

    Images with usage SG_USAGE_IMMUTABLE must be fully initialized by
    providing a valid .data member which points to initialization data.

    ADVANCED TOPIC: Injecting native 3D-API textures:

    The following struct members allow to inject your own GL, Metal or D3D11
    textures into sokol_gfx:

    .gl_textures[SG_NUM_INFLIGHT_FRAMES]
    .mtl_textures[SG_NUM_INFLIGHT_FRAMES]
    .d3d11_texture
    .d3d11_shader_resource_view

    For GL, you can also specify the texture target or leave it empty to use
    the default texture target for the image type (GL_TEXTURE_2D for
    SG_IMAGETYPE_2D etc)

    For D3D11, you can provide either a D3D11 texture, or a
    shader-resource-view, or both. If only a texture is provided, a matching
    shader-resource-view will be created. If only a shader-resource-view is
    provided, the texture will be looked up from the shader-resource-view.

    The same rules apply as for injecting native buffers (see sg_buffer_desc
    documentation for more details).
*/
typedef struct sg_image_desc {
    uint32_t _start_canary;
    sg_image_type type;
    bool render_target;
    int width;
    int height;
    int num_slices;
    int num_mipmaps;
    sg_usage usage;
    sg_pixel_format pixel_format;
    int sample_count;
    sg_filter min_filter;
    sg_filter mag_filter;
    sg_wrap wrap_u;
    sg_wrap wrap_v;
    sg_wrap wrap_w;
    sg_border_color border_color;
    uint32_t max_anisotropy;
    float min_lod;
    float max_lod;
    sg_image_data data;
    const char* label;
    /* GL specific */
    uint32_t gl_textures[SG_NUM_INFLIGHT_FRAMES];
    uint32_t gl_texture_target;
    /* Metal specific */
    const void* mtl_textures[SG_NUM_INFLIGHT_FRAMES];
    /* D3D11 specific */
    const void* d3d11_texture;
    const void* d3d11_shader_resource_view;
    /* WebGPU specific */
    const void* wgpu_texture;
    uint32_t _end_canary;
} sg_image_desc;

/*
    sg_shader_desc

    The structure sg_shader_desc defines all creation parameters for shader
    programs, used as input to the sg_make_shader() function:

    - reflection information for vertex attributes (vertex shader inputs):
        - vertex attribute name (required for GLES2, optional for GLES3 and GL)
        - a semantic name and index (required for D3D11)
    - for each shader-stage (vertex and fragment):
        - the shader source or bytecode
        - an optional entry function name
        - an optional compile target (only for D3D11 when source is provided,
          defaults are "vs_4_0" and "ps_4_0")
        - reflection info for each uniform block used by the shader stage:
            - the size of the uniform block in bytes
            - a memory layout hint (native vs std140, only required for GL backends)
            - reflection info for each uniform block member (only required for GL backends):
                - member name
                - member type (SG_UNIFORMTYPE_xxx)
                - if the member is an array, the number of array items
        - reflection info for the texture images used by the shader stage:
            - the image type (SG_IMAGETYPE_xxx)
            - the sampler type (SG_SAMPLERTYPE_xxx, default is SG_SAMPLERTYPE_FLOAT)
            - the name of the texture sampler (required for GLES2, optional everywhere else)

    For all GL backends, shader source-code must be provided. For D3D11 and Metal,
    either shader source-code or byte-code can be provided.

    For D3D11, if source code is provided, the d3dcompiler_47.dll will be loaded
    on demand. If this fails, shader creation will fail. When compiling HLSL
    source code, you can provide an optional target string via
    sg_shader_stage_desc.d3d11_target, the default target is "vs_4_0" for the
    vertex shader stage and "ps_4_0" for the pixel shader stage.
*/
typedef struct sg_shader_attr_desc {
    const char* name;           // GLSL vertex attribute name (only strictly required for GLES2)
    const char* sem_name;       // HLSL semantic name
    int sem_index;              // HLSL semantic index
} sg_shader_attr_desc;

typedef struct sg_shader_uniform_desc {
    const char* name;
    sg_uniform_type type;
    int array_count;
} sg_shader_uniform_desc;

typedef struct sg_shader_uniform_block_desc {
    size_t size;
    sg_uniform_layout layout;
    sg_shader_uniform_desc uniforms[SG_MAX_UB_MEMBERS];
} sg_shader_uniform_block_desc;

typedef struct sg_shader_image_desc {
    const char* name;
    sg_image_type image_type;
    sg_sampler_type sampler_type;
} sg_shader_image_desc;

typedef struct sg_shader_stage_desc {
    const char* source;
    sg_range bytecode;
    const char* entry;
    const char* d3d11_target;
    sg_shader_uniform_block_desc uniform_blocks[SG_MAX_SHADERSTAGE_UBS];
    sg_shader_image_desc images[SG_MAX_SHADERSTAGE_IMAGES];
} sg_shader_stage_desc;

typedef struct sg_shader_desc {
    uint32_t _start_canary;
    sg_shader_attr_desc attrs[SG_MAX_VERTEX_ATTRIBUTES];
    sg_shader_stage_desc vs;
    sg_shader_stage_desc fs;
    const char* label;
    uint32_t _end_canary;
} sg_shader_desc;

/*
    sg_pipeline_desc

    The sg_pipeline_desc struct defines all creation parameters for an
    sg_pipeline object, used as argument to the sg_make_pipeline() function:

    - the vertex layout for all input vertex buffers
    - a shader object
    - the 3D primitive type (points, lines, triangles, ...)
    - the index type (none, 16- or 32-bit)
    - all the fixed-function-pipeline state (depth-, stencil-, blend-state, etc...)

    If the vertex data has no gaps between vertex components, you can omit
    the .layout.buffers[].stride and layout.attrs[].offset items (leave them
    default-initialized to 0), sokol-gfx will then compute the offsets and
    strides from the vertex component formats (.layout.attrs[].format).
    Please note that ALL vertex attribute offsets must be 0 in order for the
    automatic offset computation to kick in.

    The default configuration is as follows:

    .shader:            0 (must be initialized with a valid sg_shader id!)
    .layout:
        .buffers[]:         vertex buffer layouts
            .stride:        0 (if no stride is given it will be computed)
            .step_func      SG_VERTEXSTEP_PER_VERTEX
            .step_rate      1
        .attrs[]:           vertex attribute declarations
            .buffer_index   0 the vertex buffer bind slot
            .offset         0 (offsets can be omitted if the vertex layout has no gaps)
            .format         SG_VERTEXFORMAT_INVALID (must be initialized!)
    .depth:
        .pixel_format:      sg_desc.context.depth_format
        .compare:           SG_COMPAREFUNC_ALWAYS
        .write_enabled:     false
        .bias:              0.0f
        .bias_slope_scale:  0.0f
        .bias_clamp:        0.0f
    .stencil:
        .enabled:           false
        .front/back:
            .compare:       SG_COMPAREFUNC_ALWAYS
            .fail_op:       SG_STENCILOP_KEEP
            .depth_fail_op: SG_STENCILOP_KEEP
            .pass_op:       SG_STENCILOP_KEEP
        .read_mask:         0
        .write_mask:        0
        .ref:               0
    .color_count            1
    .colors[0..color_count]
        .pixel_format       sg_desc.context.color_format
        .write_mask:        SG_COLORMASK_RGBA
        .blend:
            .enabled:           false
            .src_factor_rgb:    SG_BLENDFACTOR_ONE
            .dst_factor_rgb:    SG_BLENDFACTOR_ZERO
            .op_rgb:            SG_BLENDOP_ADD
            .src_factor_alpha:  SG_BLENDFACTOR_ONE
            .dst_factor_alpha:  SG_BLENDFACTOR_ZERO
            .op_alpha:          SG_BLENDOP_ADD
    .primitive_type:            SG_PRIMITIVETYPE_TRIANGLES
    .index_type:                SG_INDEXTYPE_NONE
    .cull_mode:                 SG_CULLMODE_NONE
    .face_winding:              SG_FACEWINDING_CW
    .sample_count:              sg_desc.context.sample_count
    .blend_color:               (sg_color) { 0.0f, 0.0f, 0.0f, 0.0f }
    .alpha_to_coverage_enabled: false
    .label  0       (optional string label for trace hooks)
*/
typedef struct sg_buffer_layout_desc {
    int stride;
    sg_vertex_step step_func;
    int step_rate;
    #if defined(SOKOL_ZIG_BINDINGS)
    uint32_t __pad[2];
    #endif
} sg_buffer_layout_desc;

typedef struct sg_vertex_attr_desc {
    int buffer_index;
    int offset;
    sg_vertex_format format;
    #if defined(SOKOL_ZIG_BINDINGS)
    uint32_t __pad[2];
    #endif
} sg_vertex_attr_desc;

typedef struct sg_layout_desc {
    sg_buffer_layout_desc buffers[SG_MAX_SHADERSTAGE_BUFFERS];
    sg_vertex_attr_desc attrs[SG_MAX_VERTEX_ATTRIBUTES];
} sg_layout_desc;

typedef struct sg_stencil_face_state {
    sg_compare_func compare;
    sg_stencil_op fail_op;
    sg_stencil_op depth_fail_op;
    sg_stencil_op pass_op;
} sg_stencil_face_state;

typedef struct sg_stencil_state {
    bool enabled;
    sg_stencil_face_state front;
    sg_stencil_face_state back;
    uint8_t read_mask;
    uint8_t write_mask;
    uint8_t ref;
} sg_stencil_state;

typedef struct sg_depth_state {
    sg_pixel_format pixel_format;
    sg_compare_func compare;
    bool write_enabled;
    float bias;
    float bias_slope_scale;
    float bias_clamp;
} sg_depth_state;

typedef struct sg_blend_state {
    bool enabled;
    sg_blend_factor src_factor_rgb;
    sg_blend_factor dst_factor_rgb;
    sg_blend_op op_rgb;
    sg_blend_factor src_factor_alpha;
    sg_blend_factor dst_factor_alpha;
    sg_blend_op op_alpha;
} sg_blend_state;

typedef struct sg_color_state {
    sg_pixel_format pixel_format;
    sg_color_mask write_mask;
    sg_blend_state blend;
} sg_color_state;

typedef struct sg_pipeline_desc {
    uint32_t _start_canary;
    sg_shader shader;
    sg_layout_desc layout;
    sg_depth_state depth;
    sg_stencil_state stencil;
    int color_count;
    sg_color_state colors[SG_MAX_COLOR_ATTACHMENTS];
    sg_primitive_type primitive_type;
    sg_index_type index_type;
    sg_cull_mode cull_mode;
    sg_face_winding face_winding;
    int sample_count;
    sg_color blend_color;
    bool alpha_to_coverage_enabled;
    const char* label;
    uint32_t _end_canary;
} sg_pipeline_desc;

/*
    sg_pass_desc

    Creation parameters for an sg_pass object, used as argument
    to the sg_make_pass() function.

    A pass object contains 1..4 color-attachments and none, or one,
    depth-stencil-attachment. Each attachment consists of
    an image, and two additional indices describing
    which subimage the pass will render to: one mipmap index, and
    if the image is a cubemap, array-texture or 3D-texture, the
    face-index, array-layer or depth-slice.

    Pass images must fulfill the following requirements:

    All images must have:
    - been created as render target (sg_image_desc.render_target = true)
    - the same size
    - the same sample count

    In addition, all color-attachment images must have the same pixel format.
*/
typedef struct sg_pass_attachment_desc {
    sg_image image;
    int mip_level;
    int slice;      /* cube texture: face; array texture: layer; 3D texture: slice */
} sg_pass_attachment_desc;

typedef struct sg_pass_desc {
    uint32_t _start_canary;
    sg_pass_attachment_desc color_attachments[SG_MAX_COLOR_ATTACHMENTS];
    sg_pass_attachment_desc depth_stencil_attachment;
    const char* label;
    uint32_t _end_canary;
} sg_pass_desc;

/*
    sg_trace_hooks

    Installable callback functions to keep track of the sokol-gfx calls,
    this is useful for debugging, or keeping track of resource creation
    and destruction.

    Trace hooks are installed with sg_install_trace_hooks(), this returns
    another sg_trace_hooks struct with the previous set of
    trace hook function pointers. These should be invoked by the
    new trace hooks to form a proper call chain.
*/
typedef struct sg_trace_hooks {
    void* user_data;
    void (*reset_state_cache)(void* user_data);
    void (*make_buffer)(const sg_buffer_desc* desc, sg_buffer result, void* user_data);
    void (*make_image)(const sg_image_desc* desc, sg_image result, void* user_data);
    void (*make_shader)(const sg_shader_desc* desc, sg_shader result, void* user_data);
    void (*make_pipeline)(const sg_pipeline_desc* desc, sg_pipeline result, void* user_data);
    void (*make_pass)(const sg_pass_desc* desc, sg_pass result, void* user_data);
    void (*destroy_buffer)(sg_buffer buf, void* user_data);
    void (*destroy_image)(sg_image img, void* user_data);
    void (*destroy_shader)(sg_shader shd, void* user_data);
    void (*destroy_pipeline)(sg_pipeline pip, void* user_data);
    void (*destroy_pass)(sg_pass pass, void* user_data);
    void (*update_buffer)(sg_buffer buf, const sg_range* data, void* user_data);
    void (*update_image)(sg_image img, const sg_image_data* data, void* user_data);
    void (*append_buffer)(sg_buffer buf, const sg_range* data, int result, void* user_data);
    void (*begin_default_pass)(const sg_pass_action* pass_action, int width, int height, void* user_data);
    void (*begin_pass)(sg_pass pass, const sg_pass_action* pass_action, void* user_data);
    void (*apply_viewport)(int x, int y, int width, int height, bool origin_top_left, void* user_data);
    void (*apply_scissor_rect)(int x, int y, int width, int height, bool origin_top_left, void* user_data);
    void (*apply_pipeline)(sg_pipeline pip, void* user_data);
    void (*apply_bindings)(const sg_bindings* bindings, void* user_data);
    void (*apply_uniforms)(sg_shader_stage stage, int ub_index, const sg_range* data, void* user_data);
    void (*draw)(int base_element, int num_elements, int num_instances, void* user_data);
    void (*end_pass)(void* user_data);
    void (*commit)(void* user_data);
    void (*alloc_buffer)(sg_buffer result, void* user_data);
    void (*alloc_image)(sg_image result, void* user_data);
    void (*alloc_shader)(sg_shader result, void* user_data);
    void (*alloc_pipeline)(sg_pipeline result, void* user_data);
    void (*alloc_pass)(sg_pass result, void* user_data);
    void (*dealloc_buffer)(sg_buffer buf_id, void* user_data);
    void (*dealloc_image)(sg_image img_id, void* user_data);
    void (*dealloc_shader)(sg_shader shd_id, void* user_data);
    void (*dealloc_pipeline)(sg_pipeline pip_id, void* user_data);
    void (*dealloc_pass)(sg_pass pass_id, void* user_data);
    void (*init_buffer)(sg_buffer buf_id, const sg_buffer_desc* desc, void* user_data);
    void (*init_image)(sg_image img_id, const sg_image_desc* desc, void* user_data);
    void (*init_shader)(sg_shader shd_id, const sg_shader_desc* desc, void* user_data);
    void (*init_pipeline)(sg_pipeline pip_id, const sg_pipeline_desc* desc, void* user_data);
    void (*init_pass)(sg_pass pass_id, const sg_pass_desc* desc, void* user_data);
    void (*uninit_buffer)(sg_buffer buf_id, void* user_data);
    void (*uninit_image)(sg_image img_id, void* user_data);
    void (*uninit_shader)(sg_shader shd_id, void* user_data);
    void (*uninit_pipeline)(sg_pipeline pip_id, void* user_data);
    void (*uninit_pass)(sg_pass pass_id, void* user_data);
    void (*fail_buffer)(sg_buffer buf_id, void* user_data);
    void (*fail_image)(sg_image img_id, void* user_data);
    void (*fail_shader)(sg_shader shd_id, void* user_data);
    void (*fail_pipeline)(sg_pipeline pip_id, void* user_data);
    void (*fail_pass)(sg_pass pass_id, void* user_data);
    void (*push_debug_group)(const char* name, void* user_data);
    void (*pop_debug_group)(void* user_data);
    void (*err_buffer_pool_exhausted)(void* user_data);
    void (*err_image_pool_exhausted)(void* user_data);
    void (*err_shader_pool_exhausted)(void* user_data);
    void (*err_pipeline_pool_exhausted)(void* user_data);
    void (*err_pass_pool_exhausted)(void* user_data);
    void (*err_context_mismatch)(void* user_data);
    void (*err_pass_invalid)(void* user_data);
    void (*err_draw_invalid)(void* user_data);
    void (*err_bindings_invalid)(void* user_data);
} sg_trace_hooks;

/*
    sg_buffer_info
    sg_image_info
    sg_shader_info
    sg_pipeline_info
    sg_pass_info

    These structs contain various internal resource attributes which
    might be useful for debug-inspection. Please don't rely on the
    actual content of those structs too much, as they are quite closely
    tied to sokol_gfx.h internals and may change more frequently than
    the other public API elements.

    The *_info structs are used as the return values of the following functions:

    sg_query_buffer_info()
    sg_query_image_info()
    sg_query_shader_info()
    sg_query_pipeline_info()
    sg_query_pass_info()
*/
typedef struct sg_slot_info {
    sg_resource_state state;    /* the current state of this resource slot */
    uint32_t res_id;        /* type-neutral resource if (e.g. sg_buffer.id) */
    uint32_t ctx_id;        /* the context this resource belongs to */
} sg_slot_info;

typedef struct sg_buffer_info {
    sg_slot_info slot;              /* resource pool slot info */
    uint32_t update_frame_index;    /* frame index of last sg_update_buffer() */
    uint32_t append_frame_index;    /* frame index of last sg_append_buffer() */
    int append_pos;                 /* current position in buffer for sg_append_buffer() */
    bool append_overflow;           /* is buffer in overflow state (due to sg_append_buffer) */
    int num_slots;                  /* number of renaming-slots for dynamically updated buffers */
    int active_slot;                /* currently active write-slot for dynamically updated buffers */
} sg_buffer_info;

typedef struct sg_image_info {
    sg_slot_info slot;              /* resource pool slot info */
    uint32_t upd_frame_index;       /* frame index of last sg_update_image() */
    int num_slots;                  /* number of renaming-slots for dynamically updated images */
    int active_slot;                /* currently active write-slot for dynamically updated images */
    int width;                      /* image width */
    int height;                     /* image height */
} sg_image_info;

typedef struct sg_shader_info {
    sg_slot_info slot;              /* resoure pool slot info */
} sg_shader_info;

typedef struct sg_pipeline_info {
    sg_slot_info slot;              /* resource pool slot info */
} sg_pipeline_info;

typedef struct sg_pass_info {
    sg_slot_info slot;              /* resource pool slot info */
} sg_pass_info;

/*
    sg_desc

    The sg_desc struct contains configuration values for sokol_gfx,
    it is used as parameter to the sg_setup() call.

    NOTE that all callback function pointers come in two versions, one without
    a userdata pointer, and one with a userdata pointer. You would
    either initialize one or the other depending on whether you pass data
    to your callbacks.

    FIXME: explain the various configuration options

    The default configuration is:

    .buffer_pool_size       128
    .image_pool_size        128
    .shader_pool_size       32
    .pipeline_pool_size     64
    .pass_pool_size         16
    .context_pool_size      16
    .uniform_buffer_size    4 MB (4*1024*1024)
    .staging_buffer_size    8 MB (8*1024*1024)
    .sampler_cache_size     64
    .max_commit_listeners   1024
    .disable_validation     false

    .allocator.alloc        0 (in this case, malloc() will be called)
    .allocator.free         0 (in this case, free() will be called)
    .allocator.user_data    0

    .context.color_format: default value depends on selected backend:
        all GL backends:    SG_PIXELFORMAT_RGBA8
        Metal and D3D11:    SG_PIXELFORMAT_BGRA8
        WGPU:               *no default* (must be queried from WGPU swapchain)
    .context.depth_format   SG_PIXELFORMAT_DEPTH_STENCIL
    .context.sample_count   1

    GL specific:
        .context.gl.force_gles2
            if this is true the GL backend will act in "GLES2 fallback mode" even
            when compiled with SOKOL_GLES3, this is useful to fall back
            to traditional WebGL if a browser doesn't support a WebGL2 context

    Metal specific:
        (NOTE: All Objective-C object references are transferred through
        a bridged (const void*) to sokol_gfx, which will use a unretained
        bridged cast (__bridged id<xxx>) to retrieve the Objective-C
        references back. Since the bridge cast is unretained, the caller
        must hold a strong reference to the Objective-C object for the
        duration of the sokol_gfx call!

        .context.metal.device
            a pointer to the MTLDevice object
        .context.metal.renderpass_descriptor_cb
        .context.metal_renderpass_descriptor_userdata_cb
            A C callback function to obtain the MTLRenderPassDescriptor for the
            current frame when rendering to the default framebuffer, will be called
            in sg_begin_default_pass().
        .context.metal.drawable_cb
        .context.metal.drawable_userdata_cb
            a C callback function to obtain a MTLDrawable for the current
            frame when rendering to the default framebuffer, will be called in
            sg_end_pass() of the default pass
        .context.metal.user_data
            optional user data pointer passed to the userdata versions of
            callback functions

    D3D11 specific:
        .context.d3d11.device
            a pointer to the ID3D11Device object, this must have been created
            before sg_setup() is called
        .context.d3d11.device_context
            a pointer to the ID3D11DeviceContext object
        .context.d3d11.render_target_view_cb
        .context.d3d11.render_target_view_userdata_cb
            a C callback function to obtain a pointer to the current
            ID3D11RenderTargetView object of the default framebuffer,
            this function will be called in sg_begin_pass() when rendering
            to the default framebuffer
        .context.d3d11.depth_stencil_view_cb
        .context.d3d11.depth_stencil_view_userdata_cb
            a C callback function to obtain a pointer to the current
            ID3D11DepthStencilView object of the default framebuffer,
            this function will be called in sg_begin_pass() when rendering
            to the default framebuffer
        .context.metal.user_data
            optional user data pointer passed to the userdata versions of
            callback functions

    WebGPU specific:
        .context.wgpu.device
            a WGPUDevice handle
        .context.wgpu.render_format
            WGPUTextureFormat of the swap chain surface
        .context.wgpu.render_view_cb
        .context.wgpu.render_view_userdata_cb
            callback to get the current WGPUTextureView of the swapchain's
            rendering attachment (may be an MSAA surface)
        .context.wgpu.resolve_view_cb
        .context.wgpu.resolve_view_userdata_cb
            callback to get the current WGPUTextureView of the swapchain's
            MSAA-resolve-target surface, must return 0 if not MSAA rendering
        .context.wgpu.depth_stencil_view_cb
        .context.wgpu.depth_stencil_view_userdata_cb
            callback to get current default-pass depth-stencil-surface WGPUTextureView
            the pixel format of the default WGPUTextureView must be WGPUTextureFormat_Depth24Plus8
        .context.metal.user_data
            optional user data pointer passed to the userdata versions of
            callback functions

    When using sokol_gfx.h and sokol_app.h together, consider using the
    helper function sapp_sgcontext() in the sokol_glue.h header to
    initialize the sg_desc.context nested struct. sapp_sgcontext() returns
    a completely initialized sg_context_desc struct with information
    provided by sokol_app.h.
*/
typedef struct sg_gl_context_desc {
    bool force_gles2;
} sg_gl_context_desc;

typedef struct sg_metal_context_desc {
    const void* device;
    const void* (*renderpass_descriptor_cb)(void);
    const void* (*renderpass_descriptor_userdata_cb)(void*);
    const void* (*drawable_cb)(void);
    const void* (*drawable_userdata_cb)(void*);
    void* user_data;
} sg_metal_context_desc;

typedef struct sg_d3d11_context_desc {
    const void* device;
    const void* device_context;
    const void* (*render_target_view_cb)(void);
    const void* (*render_target_view_userdata_cb)(void*);
    const void* (*depth_stencil_view_cb)(void);
    const void* (*depth_stencil_view_userdata_cb)(void*);
    void* user_data;
} sg_d3d11_context_desc;

typedef struct sg_wgpu_context_desc {
    const void* device;                    /* WGPUDevice */
    const void* (*render_view_cb)(void);   /* returns WGPUTextureView */
    const void* (*render_view_userdata_cb)(void*);
    const void* (*resolve_view_cb)(void);  /* returns WGPUTextureView */
    const void* (*resolve_view_userdata_cb)(void*);
    const void* (*depth_stencil_view_cb)(void);    /* returns WGPUTextureView, must be WGPUTextureFormat_Depth24Plus8 */
    const void* (*depth_stencil_view_userdata_cb)(void*);
    void* user_data;
} sg_wgpu_context_desc;

typedef struct sg_context_desc {
    sg_pixel_format color_format;
    sg_pixel_format depth_format;
    int sample_count;
    sg_gl_context_desc gl;
    sg_metal_context_desc metal;
    sg_d3d11_context_desc d3d11;
    sg_wgpu_context_desc wgpu;
} sg_context_desc;

/*
    sg_commit_listener

    Used with function sg_add_commit_listener() to add a callback
    which will be called in sg_commit(). This is useful for libraries
    building on top of sokol-gfx to be notified about when a frame
    ends (instead of having to guess, or add a manual 'new-frame'
    function.
*/
typedef struct sg_commit_listener {
    void (*func)(void* user_data);
    void* user_data;
} sg_commit_listener;

/*
    sg_allocator

    Used in sg_desc to provide custom memory-alloc and -free functions
    to sokol_gfx.h. If memory management should be overridden, both the
    alloc and free function must be provided (e.g. it's not valid to
    override one function but not the other).
*/
typedef struct sg_allocator {
    void* (*alloc)(size_t size, void* user_data);
    void (*free)(void* ptr, void* user_data);
    void* user_data;
} sg_allocator;

/*
    sg_logger

    Used in sg_desc to provide custom log callbacks to sokol_gfx.h.
    Default behavior is SOKOL_LOG(message).
*/
typedef struct sg_logger {
    void (*log_cb)(const char* message, void* user_data);
    void* user_data;
} sg_logger;

typedef struct sg_desc {
    uint32_t _start_canary;
    int buffer_pool_size;
    int image_pool_size;
    int shader_pool_size;
    int pipeline_pool_size;
    int pass_pool_size;
    int context_pool_size;
    int uniform_buffer_size;
    int staging_buffer_size;
    int sampler_cache_size;
    int max_commit_listeners;
    bool disable_validation;    // disable validation layer even in debug mode, useful for tests
    sg_allocator allocator;
    sg_logger logger; // optional log function override
    sg_context_desc context;
    uint32_t _end_canary;
} sg_desc;

/* setup and misc functions */
SOKOL_GFX_API_DECL void sg_setup(const sg_desc* desc);
SOKOL_GFX_API_DECL void sg_shutdown(void);
SOKOL_GFX_API_DECL bool sg_isvalid(void);
SOKOL_GFX_API_DECL void sg_reset_state_cache(void);
SOKOL_GFX_API_DECL sg_trace_hooks sg_install_trace_hooks(const sg_trace_hooks* trace_hooks);
SOKOL_GFX_API_DECL void sg_push_debug_group(const char* name);
SOKOL_GFX_API_DECL void sg_pop_debug_group(void);
SOKOL_GFX_API_DECL bool sg_add_commit_listener(sg_commit_listener listener);
SOKOL_GFX_API_DECL bool sg_remove_commit_listener(sg_commit_listener listener);

/* resource creation, destruction and updating */
SOKOL_GFX_API_DECL sg_buffer sg_make_buffer(const sg_buffer_desc* desc);
SOKOL_GFX_API_DECL sg_image sg_make_image(const sg_image_desc* desc);
SOKOL_GFX_API_DECL sg_shader sg_make_shader(const sg_shader_desc* desc);
SOKOL_GFX_API_DECL sg_pipeline sg_make_pipeline(const sg_pipeline_desc* desc);
SOKOL_GFX_API_DECL sg_pass sg_make_pass(const sg_pass_desc* desc);
SOKOL_GFX_API_DECL void sg_destroy_buffer(sg_buffer buf);
SOKOL_GFX_API_DECL void sg_destroy_image(sg_image img);
SOKOL_GFX_API_DECL void sg_destroy_shader(sg_shader shd);
SOKOL_GFX_API_DECL void sg_destroy_pipeline(sg_pipeline pip);
SOKOL_GFX_API_DECL void sg_destroy_pass(sg_pass pass);
SOKOL_GFX_API_DECL void sg_update_buffer(sg_buffer buf, const sg_range* data);
SOKOL_GFX_API_DECL void sg_update_image(sg_image img, const sg_image_data* data);
SOKOL_GFX_API_DECL int sg_append_buffer(sg_buffer buf, const sg_range* data);
SOKOL_GFX_API_DECL bool sg_query_buffer_overflow(sg_buffer buf);
SOKOL_GFX_API_DECL bool sg_query_buffer_will_overflow(sg_buffer buf, size_t size);

/* rendering functions */
SOKOL_GFX_API_DECL void sg_begin_default_pass(const sg_pass_action* pass_action, int width, int height);
SOKOL_GFX_API_DECL void sg_begin_default_passf(const sg_pass_action* pass_action, float width, float height);
SOKOL_GFX_API_DECL void sg_begin_pass(sg_pass pass, const sg_pass_action* pass_action);
SOKOL_GFX_API_DECL void sg_apply_viewport(int x, int y, int width, int height, bool origin_top_left);
SOKOL_GFX_API_DECL void sg_apply_viewportf(float x, float y, float width, float height, bool origin_top_left);
SOKOL_GFX_API_DECL void sg_apply_scissor_rect(int x, int y, int width, int height, bool origin_top_left);
SOKOL_GFX_API_DECL void sg_apply_scissor_rectf(float x, float y, float width, float height, bool origin_top_left);
SOKOL_GFX_API_DECL void sg_apply_pipeline(sg_pipeline pip);
SOKOL_GFX_API_DECL void sg_apply_bindings(const sg_bindings* bindings);
SOKOL_GFX_API_DECL void sg_apply_uniforms(sg_shader_stage stage, int ub_index, const sg_range* data);
SOKOL_GFX_API_DECL void sg_draw(int base_element, int num_elements, int num_instances);
SOKOL_GFX_API_DECL void sg_end_pass(void);
SOKOL_GFX_API_DECL void sg_commit(void);

/* getting information */
SOKOL_GFX_API_DECL sg_desc sg_query_desc(void);
SOKOL_GFX_API_DECL sg_backend sg_query_backend(void);
SOKOL_GFX_API_DECL sg_features sg_query_features(void);
SOKOL_GFX_API_DECL sg_limits sg_query_limits(void);
SOKOL_GFX_API_DECL sg_pixelformat_info sg_query_pixelformat(sg_pixel_format fmt);
/* get current state of a resource (INITIAL, ALLOC, VALID, FAILED, INVALID) */
SOKOL_GFX_API_DECL sg_resource_state sg_query_buffer_state(sg_buffer buf);
SOKOL_GFX_API_DECL sg_resource_state sg_query_image_state(sg_image img);
SOKOL_GFX_API_DECL sg_resource_state sg_query_shader_state(sg_shader shd);
SOKOL_GFX_API_DECL sg_resource_state sg_query_pipeline_state(sg_pipeline pip);
SOKOL_GFX_API_DECL sg_resource_state sg_query_pass_state(sg_pass pass);
/* get runtime information about a resource */
SOKOL_GFX_API_DECL sg_buffer_info sg_query_buffer_info(sg_buffer buf);
SOKOL_GFX_API_DECL sg_image_info sg_query_image_info(sg_image img);
SOKOL_GFX_API_DECL sg_shader_info sg_query_shader_info(sg_shader shd);
SOKOL_GFX_API_DECL sg_pipeline_info sg_query_pipeline_info(sg_pipeline pip);
SOKOL_GFX_API_DECL sg_pass_info sg_query_pass_info(sg_pass pass);
/* get resource creation desc struct with their default values replaced */
SOKOL_GFX_API_DECL sg_buffer_desc sg_query_buffer_defaults(const sg_buffer_desc* desc);
SOKOL_GFX_API_DECL sg_image_desc sg_query_image_defaults(const sg_image_desc* desc);
SOKOL_GFX_API_DECL sg_shader_desc sg_query_shader_defaults(const sg_shader_desc* desc);
SOKOL_GFX_API_DECL sg_pipeline_desc sg_query_pipeline_defaults(const sg_pipeline_desc* desc);
SOKOL_GFX_API_DECL sg_pass_desc sg_query_pass_defaults(const sg_pass_desc* desc);

/* separate resource allocation and initialization (for async setup) */
SOKOL_GFX_API_DECL sg_buffer sg_alloc_buffer(void);
SOKOL_GFX_API_DECL sg_image sg_alloc_image(void);
SOKOL_GFX_API_DECL sg_shader sg_alloc_shader(void);
SOKOL_GFX_API_DECL sg_pipeline sg_alloc_pipeline(void);
SOKOL_GFX_API_DECL sg_pass sg_alloc_pass(void);
SOKOL_GFX_API_DECL void sg_dealloc_buffer(sg_buffer buf);
SOKOL_GFX_API_DECL void sg_dealloc_image(sg_image img);
SOKOL_GFX_API_DECL void sg_dealloc_shader(sg_shader shd);
SOKOL_GFX_API_DECL void sg_dealloc_pipeline(sg_pipeline pip);
SOKOL_GFX_API_DECL void sg_dealloc_pass(sg_pass pass);
SOKOL_GFX_API_DECL void sg_init_buffer(sg_buffer buf, const sg_buffer_desc* desc);
SOKOL_GFX_API_DECL void sg_init_image(sg_image img, const sg_image_desc* desc);
SOKOL_GFX_API_DECL void sg_init_shader(sg_shader shd, const sg_shader_desc* desc);
SOKOL_GFX_API_DECL void sg_init_pipeline(sg_pipeline pip, const sg_pipeline_desc* desc);
SOKOL_GFX_API_DECL void sg_init_pass(sg_pass pass, const sg_pass_desc* desc);
SOKOL_GFX_API_DECL void sg_uninit_buffer(sg_buffer buf);
SOKOL_GFX_API_DECL void sg_uninit_image(sg_image img);
SOKOL_GFX_API_DECL void sg_uninit_shader(sg_shader shd);
SOKOL_GFX_API_DECL void sg_uninit_pipeline(sg_pipeline pip);
SOKOL_GFX_API_DECL void sg_uninit_pass(sg_pass pass);
SOKOL_GFX_API_DECL void sg_fail_buffer(sg_buffer buf);
SOKOL_GFX_API_DECL void sg_fail_image(sg_image img);
SOKOL_GFX_API_DECL void sg_fail_shader(sg_shader shd);
SOKOL_GFX_API_DECL void sg_fail_pipeline(sg_pipeline pip);
SOKOL_GFX_API_DECL void sg_fail_pass(sg_pass pass);

/* rendering contexts (optional) */
SOKOL_GFX_API_DECL sg_context sg_setup_context(void);
SOKOL_GFX_API_DECL void sg_activate_context(sg_context ctx_id);
SOKOL_GFX_API_DECL void sg_discard_context(sg_context ctx_id);

/* Backend-specific helper functions, these may come in handy for mixing
   sokol-gfx rendering with 'native backend' rendering functions.

   This group of functions will be expanded as needed.
*/

/* D3D11: return ID3D11Device */
SOKOL_GFX_API_DECL const void* sg_d3d11_device(void);

/* Metal: return __bridge-casted MTLDevice */
SOKOL_GFX_API_DECL const void* sg_mtl_device(void);

/* Metal: return __bridge-casted MTLRenderCommandEncoder in current pass (or zero if outside pass) */
SOKOL_GFX_API_DECL const void* sg_mtl_render_command_encoder(void);

#ifdef __cplusplus
} /* extern "C" */

/* reference-based equivalents for c++ */
inline void sg_setup(const sg_desc& desc) { return sg_setup(&desc); }

inline sg_buffer sg_make_buffer(const sg_buffer_desc& desc) { return sg_make_buffer(&desc); }
inline sg_image sg_make_image(const sg_image_desc& desc) { return sg_make_image(&desc); }
inline sg_shader sg_make_shader(const sg_shader_desc& desc) { return sg_make_shader(&desc); }
inline sg_pipeline sg_make_pipeline(const sg_pipeline_desc& desc) { return sg_make_pipeline(&desc); }
inline sg_pass sg_make_pass(const sg_pass_desc& desc) { return sg_make_pass(&desc); }
inline void sg_update_image(sg_image img, const sg_image_data& data) { return sg_update_image(img, &data); }

inline void sg_begin_default_pass(const sg_pass_action& pass_action, int width, int height) { return sg_begin_default_pass(&pass_action, width, height); }
inline void sg_begin_default_passf(const sg_pass_action& pass_action, float width, float height) { return sg_begin_default_passf(&pass_action, width, height); }
inline void sg_begin_pass(sg_pass pass, const sg_pass_action& pass_action) { return sg_begin_pass(pass, &pass_action); }
inline void sg_apply_bindings(const sg_bindings& bindings) { return sg_apply_bindings(&bindings); }
inline void sg_apply_uniforms(sg_shader_stage stage, int ub_index, const sg_range& data) { return sg_apply_uniforms(stage, ub_index, &data); }

inline sg_buffer_desc sg_query_buffer_defaults(const sg_buffer_desc& desc) { return sg_query_buffer_defaults(&desc); }
inline sg_image_desc sg_query_image_defaults(const sg_image_desc& desc) { return sg_query_image_defaults(&desc); }
inline sg_shader_desc sg_query_shader_defaults(const sg_shader_desc& desc) { return sg_query_shader_defaults(&desc); }
inline sg_pipeline_desc sg_query_pipeline_defaults(const sg_pipeline_desc& desc) { return sg_query_pipeline_defaults(&desc); }
inline sg_pass_desc sg_query_pass_defaults(const sg_pass_desc& desc) { return sg_query_pass_defaults(&desc); }

inline void sg_init_buffer(sg_buffer buf_id, const sg_buffer_desc& desc) { return sg_init_buffer(buf_id, &desc); }
inline void sg_init_image(sg_image img_id, const sg_image_desc& desc) { return sg_init_image(img_id, &desc); }
inline void sg_init_shader(sg_shader shd_id, const sg_shader_desc& desc) { return sg_init_shader(shd_id, &desc); }
inline void sg_init_pipeline(sg_pipeline pip_id, const sg_pipeline_desc& desc) { return sg_init_pipeline(pip_id, &desc); }
inline void sg_init_pass(sg_pass pass_id, const sg_pass_desc& desc) { return sg_init_pass(pass_id, &desc); }

inline void sg_update_buffer(sg_buffer buf_id, const sg_range& data) { return sg_update_buffer(buf_id, &data); }
inline int sg_append_buffer(sg_buffer buf_id, const sg_range& data) { return sg_append_buffer(buf_id, &data); }
#endif
#endif // SOKOL_GFX_INCLUDED

/*--- IMPLEMENTATION ---------------------------------------------------------*/
#ifdef SOKOL_GFX_IMPL
#define SOKOL_GFX_IMPL_INCLUDED (1)

#if !(defined(SOKOL_GLCORE33)||defined(SOKOL_GLES2)||defined(SOKOL_GLES3)||defined(SOKOL_D3D11)||defined(SOKOL_METAL)||defined(SOKOL_WGPU)||defined(SOKOL_DUMMY_BACKEND))
#error "Please select a backend with SOKOL_GLCORE33, SOKOL_GLES2, SOKOL_GLES3, SOKOL_D3D11, SOKOL_METAL, SOKOL_WGPU or SOKOL_DUMMY_BACKEND"
#endif
#if defined(SOKOL_MALLOC) || defined(SOKOL_CALLOC) || defined(SOKOL_FREE)
#error "SOKOL_MALLOC/CALLOC/FREE macros are no longer supported, please use sg_desc.allocator to override memory allocation functions"
#endif

#include <stdlib.h> // malloc, free
#include <string.h> // memset
#include <float.h> // FLT_MAX

#ifndef SOKOL_API_IMPL
    #define SOKOL_API_IMPL
#endif
#ifndef SOKOL_DEBUG
    #ifndef NDEBUG
        #define SOKOL_DEBUG
    #endif
#endif
#ifndef SOKOL_ASSERT
    #include <assert.h>
    #define SOKOL_ASSERT(c) assert(c)
#endif
#ifndef SOKOL_VALIDATE_BEGIN
    #define SOKOL_VALIDATE_BEGIN() _sg_validate_begin()
#endif
#ifndef SOKOL_VALIDATE
    #define SOKOL_VALIDATE(cond, err) _sg_validate((cond), err)
#endif
#ifndef SOKOL_VALIDATE_END
    #define SOKOL_VALIDATE_END() _sg_validate_end()
#endif
#ifndef SOKOL_UNREACHABLE
    #define SOKOL_UNREACHABLE SOKOL_ASSERT(false)
#endif

#if !defined(SOKOL_DEBUG)
    #define SG_LOG(s)
#else
    #define SG_LOG(s) _sg_log(s)
    #ifndef SOKOL_LOG
        #if defined(__ANDROID__)
            #include <android/log.h>
            #define SOKOL_LOG(s) __android_log_write(ANDROID_LOG_INFO, "SOKOL_GFX", s)
        #else
            #include <stdio.h>
            #define SOKOL_LOG(s) puts(s)
        #endif
    #endif
#endif

#ifndef _SOKOL_PRIVATE
    #if defined(__GNUC__) || defined(__clang__)
        #define _SOKOL_PRIVATE __attribute__((unused)) static
    #else
        #define _SOKOL_PRIVATE static
    #endif
#endif

#ifndef _SOKOL_UNUSED
    #define _SOKOL_UNUSED(x) (void)(x)
#endif

#if defined(SOKOL_TRACE_HOOKS)
#define _SG_TRACE_ARGS(fn, ...) if (_sg.hooks.fn) { _sg.hooks.fn(__VA_ARGS__, _sg.hooks.user_data); }
#define _SG_TRACE_NOARGS(fn) if (_sg.hooks.fn) { _sg.hooks.fn(_sg.hooks.user_data); }
#else
#define _SG_TRACE_ARGS(fn, ...)
#define _SG_TRACE_NOARGS(fn)
#endif

/* default clear values */
#ifndef SG_DEFAULT_CLEAR_RED
#define SG_DEFAULT_CLEAR_RED (0.5f)
#endif
#ifndef SG_DEFAULT_CLEAR_GREEN
#define SG_DEFAULT_CLEAR_GREEN (0.5f)
#endif
#ifndef SG_DEFAULT_CLEAR_BLUE
#define SG_DEFAULT_CLEAR_BLUE (0.5f)
#endif
#ifndef SG_DEFAULT_CLEAR_ALPHA
#define SG_DEFAULT_CLEAR_ALPHA (1.0f)
#endif
#ifndef SG_DEFAULT_CLEAR_DEPTH
#define SG_DEFAULT_CLEAR_DEPTH (1.0f)
#endif
#ifndef SG_DEFAULT_CLEAR_STENCIL
#define SG_DEFAULT_CLEAR_STENCIL (0)
#endif

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable:4115)   /* named type definition in parentheses */
#pragma warning(disable:4505)   /* unreferenced local function has been removed */
#pragma warning(disable:4201)   /* nonstandard extension used: nameless struct/union (needed by d3d11.h) */
#pragma warning(disable:4054)   /* 'type cast': from function pointer */
#pragma warning(disable:4055)   /* 'type cast': from data pointer */
#endif

#if defined(SOKOL_D3D11)
    #ifndef D3D11_NO_HELPERS
    #define D3D11_NO_HELPERS
    #endif
    #ifndef WIN32_LEAN_AND_MEAN
    #define WIN32_LEAN_AND_MEAN
    #endif
    #ifndef NOMINMAX
    #define NOMINMAX
    #endif
    #include <d3d11.h>
    #include <d3dcompiler.h>
    #ifdef _MSC_VER
    #if (defined(WINAPI_FAMILY_PARTITION) && !WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP))
        #pragma comment (lib, "WindowsApp")
    #else
        #pragma comment (lib, "kernel32")
        #pragma comment (lib, "user32")
        #pragma comment (lib, "dxgi")
        #pragma comment (lib, "d3d11")
    #endif
    #endif
#elif defined(SOKOL_METAL)
    // see https://clang.llvm.org/docs/LanguageExtensions.html#automatic-reference-counting
    #if !defined(__cplusplus)
        #if __has_feature(objc_arc) && !__has_feature(objc_arc_fields)
            #error "sokol_gfx.h requires __has_feature(objc_arc_field) if ARC is enabled (use a more recent compiler version)"
        #endif
    #endif
    #include <TargetConditionals.h>
    #if defined(TARGET_OS_IPHONE) && !TARGET_OS_IPHONE
        #define _SG_TARGET_MACOS (1)
    #else
        #define _SG_TARGET_IOS (1)
        #if defined(TARGET_IPHONE_SIMULATOR) && TARGET_IPHONE_SIMULATOR
            #define _SG_TARGET_IOS_SIMULATOR (1)
        #endif
    #endif
    #import <Metal/Metal.h>
#elif defined(SOKOL_WGPU)
    #if defined(__EMSCRIPTEN__)
        #include <webgpu/webgpu.h>
    #else
        #include <dawn/webgpu.h>
    #endif
#elif defined(SOKOL_GLCORE33) || defined(SOKOL_GLES2) || defined(SOKOL_GLES3)
    #define _SOKOL_ANY_GL (1)

    // include platform specific GL headers (or on Win32: use an embedded GL loader)
    #if !defined(SOKOL_EXTERNAL_GL_LOADER)
        #if defined(_WIN32)
            #if defined(SOKOL_GLCORE33) && !defined(SOKOL_EXTERNAL_GL_LOADER)
                #ifndef WIN32_LEAN_AND_MEAN
                #define WIN32_LEAN_AND_MEAN
                #endif
                #ifndef NOMINMAX
                #define NOMINMAX
                #endif
                #include <windows.h>
                #define _SOKOL_USE_WIN32_GL_LOADER (1)
                #pragma comment (lib, "kernel32")   // GetProcAddress()
            #endif
        #elif defined(__APPLE__)
            #include <TargetConditionals.h>
            #ifndef GL_SILENCE_DEPRECATION
                #define GL_SILENCE_DEPRECATION
            #endif
            #if defined(TARGET_OS_IPHONE) && !TARGET_OS_IPHONE
                #include <OpenGL/gl3.h>
            #else
                #include <OpenGLES/ES3/gl.h>
                #include <OpenGLES/ES3/glext.h>
            #endif
        #elif defined(__EMSCRIPTEN__) || defined(__ANDROID__)
            #if defined(SOKOL_GLES3)
                #include <GLES3/gl3.h>
            #elif defined(SOKOL_GLES2)
                #ifndef GL_EXT_PROTOTYPES
                #define GL_GLEXT_PROTOTYPES
                #endif
                #include <GLES2/gl2.h>
                #include <GLES2/gl2ext.h>
            #endif
        #elif defined(__linux__) || defined(__unix__)
            #define GL_GLEXT_PROTOTYPES
            #include <GL/gl.h>
        #endif
    #endif

    // optional GL loader definitions (only on Win32)
    #if defined(_SOKOL_USE_WIN32_GL_LOADER)
        #define __gl_h_ 1
        #define __gl32_h_ 1
        #define __gl31_h_ 1
        #define __GL_H__ 1
        #define __glext_h_ 1
        #define __GLEXT_H_ 1
        #define __gltypes_h_ 1
        #define __glcorearb_h_ 1
        #define __gl_glcorearb_h_ 1
        #define GL_APIENTRY APIENTRY

        typedef unsigned int  GLenum;
        typedef unsigned int  GLuint;
        typedef int  GLsizei;
        typedef char  GLchar;
        typedef ptrdiff_t  GLintptr;
        typedef ptrdiff_t  GLsizeiptr;
        typedef double  GLclampd;
        typedef unsigned short  GLushort;
        typedef unsigned char  GLubyte;
        typedef unsigned char  GLboolean;
        typedef uint64_t  GLuint64;
        typedef double  GLdouble;
        typedef unsigned short  GLhalf;
        typedef float  GLclampf;
        typedef unsigned int  GLbitfield;
        typedef signed char  GLbyte;
        typedef short  GLshort;
        typedef void  GLvoid;
        typedef int64_t  GLint64;
        typedef float  GLfloat;
        typedef struct __GLsync * GLsync;
        typedef int  GLint;
        #define GL_INT_2_10_10_10_REV 0x8D9F
        #define GL_R32F 0x822E
        #define GL_PROGRAM_POINT_SIZE 0x8642
        #define GL_STENCIL_ATTACHMENT 0x8D20
        #define GL_DEPTH_ATTACHMENT 0x8D00
        #define GL_COLOR_ATTACHMENT2 0x8CE2
        #define GL_COLOR_ATTACHMENT0 0x8CE0
        #define GL_R16F 0x822D
        #define GL_COLOR_ATTACHMENT22 0x8CF6
        #define GL_DRAW_FRAMEBUFFER 0x8CA9
        #define GL_FRAMEBUFFER_COMPLETE 0x8CD5
        #define GL_NUM_EXTENSIONS 0x821D
        #define GL_INFO_LOG_LENGTH 0x8B84
        #define GL_VERTEX_SHADER 0x8B31
        #define GL_INCR 0x1E02
        #define GL_DYNAMIC_DRAW 0x88E8
        #define GL_STATIC_DRAW 0x88E4
        #define GL_TEXTURE_CUBE_MAP_POSITIVE_Z 0x8519
        #define GL_TEXTURE_CUBE_MAP 0x8513
        #define GL_FUNC_SUBTRACT 0x800A
        #define GL_FUNC_REVERSE_SUBTRACT 0x800B
        #define GL_CONSTANT_COLOR 0x8001
        #define GL_DECR_WRAP 0x8508
        #define GL_R8 0x8229
        #define GL_LINEAR_MIPMAP_LINEAR 0x2703
        #define GL_ELEMENT_ARRAY_BUFFER 0x8893
        #define GL_SHORT 0x1402
        #define GL_DEPTH_TEST 0x0B71
        #define GL_TEXTURE_CUBE_MAP_NEGATIVE_Y 0x8518
        #define GL_LINK_STATUS 0x8B82
        #define GL_TEXTURE_CUBE_MAP_POSITIVE_Y 0x8517
        #define GL_SAMPLE_ALPHA_TO_COVERAGE 0x809E
        #define GL_RGBA16F 0x881A
        #define GL_CONSTANT_ALPHA 0x8003
        #define GL_READ_FRAMEBUFFER 0x8CA8
        #define GL_TEXTURE0 0x84C0
        #define GL_TEXTURE_MIN_LOD 0x813A
        #define GL_CLAMP_TO_EDGE 0x812F
        #define GL_UNSIGNED_SHORT_5_6_5 0x8363
        #define GL_TEXTURE_WRAP_R 0x8072
        #define GL_UNSIGNED_SHORT_5_5_5_1 0x8034
        #define GL_NEAREST_MIPMAP_NEAREST 0x2700
        #define GL_UNSIGNED_SHORT_4_4_4_4 0x8033
        #define GL_SRC_ALPHA_SATURATE 0x0308
        #define GL_STREAM_DRAW 0x88E0
        #define GL_ONE 1
        #define GL_NEAREST_MIPMAP_LINEAR 0x2702
        #define GL_RGB10_A2 0x8059
        #define GL_RGBA8 0x8058
        #define GL_COLOR_ATTACHMENT1 0x8CE1
        #define GL_RGBA4 0x8056
        #define GL_RGB8 0x8051
        #define GL_ARRAY_BUFFER 0x8892
        #define GL_STENCIL 0x1802
        #define GL_TEXTURE_2D 0x0DE1
        #define GL_DEPTH 0x1801
        #define GL_FRONT 0x0404
        #define GL_STENCIL_BUFFER_BIT 0x00000400
        #define GL_REPEAT 0x2901
        #define GL_RGBA 0x1908
        #define GL_TEXTURE_CUBE_MAP_POSITIVE_X 0x8515
        #define GL_DECR 0x1E03
        #define GL_FRAGMENT_SHADER 0x8B30
        #define GL_FLOAT 0x1406
        #define GL_TEXTURE_MAX_LOD 0x813B
        #define GL_DEPTH_COMPONENT 0x1902
        #define GL_ONE_MINUS_DST_ALPHA 0x0305
        #define GL_COLOR 0x1800
        #define GL_TEXTURE_2D_ARRAY 0x8C1A
        #define GL_TRIANGLES 0x0004
        #define GL_UNSIGNED_BYTE 0x1401
        #define GL_TEXTURE_MAG_FILTER 0x2800
        #define GL_ONE_MINUS_CONSTANT_ALPHA 0x8004
        #define GL_NONE 0
        #define GL_SRC_COLOR 0x0300
        #define GL_BYTE 0x1400
        #define GL_TEXTURE_CUBE_MAP_NEGATIVE_Z 0x851A
        #define GL_LINE_STRIP 0x0003
        #define GL_TEXTURE_3D 0x806F
        #define GL_CW 0x0900
        #define GL_LINEAR 0x2601
        #define GL_RENDERBUFFER 0x8D41
        #define GL_GEQUAL 0x0206
        #define GL_COLOR_BUFFER_BIT 0x00004000
        #define GL_RGBA32F 0x8814
        #define GL_BLEND 0x0BE2
        #define GL_ONE_MINUS_SRC_ALPHA 0x0303
        #define GL_ONE_MINUS_CONSTANT_COLOR 0x8002
        #define GL_TEXTURE_WRAP_T 0x2803
        #define GL_TEXTURE_WRAP_S 0x2802
        #define GL_TEXTURE_MIN_FILTER 0x2801
        #define GL_LINEAR_MIPMAP_NEAREST 0x2701
        #define GL_EXTENSIONS 0x1F03
        #define GL_NO_ERROR 0
        #define GL_REPLACE 0x1E01
        #define GL_KEEP 0x1E00
        #define GL_CCW 0x0901
        #define GL_TEXTURE_CUBE_MAP_NEGATIVE_X 0x8516
        #define GL_RGB 0x1907
        #define GL_TRIANGLE_STRIP 0x0005
        #define GL_FALSE 0
        #define GL_ZERO 0
        #define GL_CULL_FACE 0x0B44
        #define GL_INVERT 0x150A
        #define GL_INT 0x1404
        #define GL_UNSIGNED_INT 0x1405
        #define GL_UNSIGNED_SHORT 0x1403
        #define GL_NEAREST 0x2600
        #define GL_SCISSOR_TEST 0x0C11
        #define GL_LEQUAL 0x0203
        #define GL_STENCIL_TEST 0x0B90
        #define GL_DITHER 0x0BD0
        #define GL_DEPTH_COMPONENT16 0x81A5
        #define GL_EQUAL 0x0202
        #define GL_FRAMEBUFFER 0x8D40
        #define GL_RGB5 0x8050
        #define GL_LINES 0x0001
        #define GL_DEPTH_BUFFER_BIT 0x00000100
        #define GL_SRC_ALPHA 0x0302
        #define GL_INCR_WRAP 0x8507
        #define GL_LESS 0x0201
        #define GL_MULTISAMPLE 0x809D
        #define GL_FRAMEBUFFER_BINDING 0x8CA6
        #define GL_BACK 0x0405
        #define GL_ALWAYS 0x0207
        #define GL_FUNC_ADD 0x8006
        #define GL_ONE_MINUS_DST_COLOR 0x0307
        #define GL_NOTEQUAL 0x0205
        #define GL_DST_COLOR 0x0306
        #define GL_COMPILE_STATUS 0x8B81
        #define GL_RED 0x1903
        #define GL_COLOR_ATTACHMENT3 0x8CE3
        #define GL_DST_ALPHA 0x0304
        #define GL_RGB5_A1 0x8057
        #define GL_GREATER 0x0204
        #define GL_POLYGON_OFFSET_FILL 0x8037
        #define GL_TRUE 1
        #define GL_NEVER 0x0200
        #define GL_POINTS 0x0000
        #define GL_ONE_MINUS_SRC_COLOR 0x0301
        #define GL_MIRRORED_REPEAT 0x8370
        #define GL_MAX_COMBINED_TEXTURE_IMAGE_UNITS 0x8B4D
        #define GL_R11F_G11F_B10F 0x8C3A
        #define GL_UNSIGNED_INT_10F_11F_11F_REV 0x8C3B
        #define GL_RGB9_E5 0x8C3D
        #define GL_UNSIGNED_INT_5_9_9_9_REV 0x8C3E
        #define GL_RGBA32UI 0x8D70
        #define GL_RGB32UI 0x8D71
        #define GL_RGBA16UI 0x8D76
        #define GL_RGB16UI 0x8D77
        #define GL_RGBA8UI 0x8D7C
        #define GL_RGB8UI 0x8D7D
        #define GL_RGBA32I 0x8D82
        #define GL_RGB32I 0x8D83
        #define GL_RGBA16I 0x8D88
        #define GL_RGB16I 0x8D89
        #define GL_RGBA8I 0x8D8E
        #define GL_RGB8I 0x8D8F
        #define GL_RED_INTEGER 0x8D94
        #define GL_RG 0x8227
        #define GL_RG_INTEGER 0x8228
        #define GL_R8 0x8229
        #define GL_R16 0x822A
        #define GL_RG8 0x822B
        #define GL_RG16 0x822C
        #define GL_R16F 0x822D
        #define GL_R32F 0x822E
        #define GL_RG16F 0x822F
        #define GL_RG32F 0x8230
        #define GL_R8I 0x8231
        #define GL_R8UI 0x8232
        #define GL_R16I 0x8233
        #define GL_R16UI 0x8234
        #define GL_R32I 0x8235
        #define GL_R32UI 0x8236
        #define GL_RG8I 0x8237
        #define GL_RG8UI 0x8238
        #define GL_RG16I 0x8239
        #define GL_RG16UI 0x823A
        #define GL_RG32I 0x823B
        #define GL_RG32UI 0x823C
        #define GL_RGBA_INTEGER 0x8D99
        #define GL_R8_SNORM 0x8F94
        #define GL_RG8_SNORM 0x8F95
        #define GL_RGB8_SNORM 0x8F96
        #define GL_RGBA8_SNORM 0x8F97
        #define GL_R16_SNORM 0x8F98
        #define GL_RG16_SNORM 0x8F99
        #define GL_RGB16_SNORM 0x8F9A
        #define GL_RGBA16_SNORM 0x8F9B
        #define GL_RGBA16 0x805B
        #define GL_MAX_TEXTURE_SIZE 0x0D33
        #define GL_MAX_CUBE_MAP_TEXTURE_SIZE 0x851C
        #define GL_MAX_3D_TEXTURE_SIZE 0x8073
        #define GL_MAX_ARRAY_TEXTURE_LAYERS 0x88FF
        #define GL_MAX_VERTEX_ATTRIBS 0x8869
        #define GL_CLAMP_TO_BORDER 0x812D
        #define GL_TEXTURE_BORDER_COLOR 0x1004
        #define GL_CURRENT_PROGRAM 0x8B8D
        #define GL_MAX_VERTEX_UNIFORM_VECTORS 0x8DFB
    #endif

    #ifndef GL_UNSIGNED_INT_2_10_10_10_REV
    #define GL_UNSIGNED_INT_2_10_10_10_REV 0x8368
    #endif
    #ifndef GL_UNSIGNED_INT_24_8
    #define GL_UNSIGNED_INT_24_8 0x84FA
    #endif
    #ifndef GL_TEXTURE_MAX_ANISOTROPY_EXT
    #define GL_TEXTURE_MAX_ANISOTROPY_EXT 0x84FE
    #endif
    #ifndef GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT
    #define GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT 0x84FF
    #endif
    #ifndef GL_COMPRESSED_RGBA_S3TC_DXT1_EXT
    #define GL_COMPRESSED_RGBA_S3TC_DXT1_EXT 0x83F1
    #endif
    #ifndef GL_COMPRESSED_RGBA_S3TC_DXT3_EXT
    #define GL_COMPRESSED_RGBA_S3TC_DXT3_EXT 0x83F2
    #endif
    #ifndef GL_COMPRESSED_RGBA_S3TC_DXT5_EXT
    #define GL_COMPRESSED_RGBA_S3TC_DXT5_EXT 0x83F3
    #endif
    #ifndef GL_COMPRESSED_RED_RGTC1
    #define GL_COMPRESSED_RED_RGTC1 0x8DBB
    #endif
    #ifndef GL_COMPRESSED_SIGNED_RED_RGTC1
    #define GL_COMPRESSED_SIGNED_RED_RGTC1 0x8DBC
    #endif
    #ifndef GL_COMPRESSED_RED_GREEN_RGTC2
    #define GL_COMPRESSED_RED_GREEN_RGTC2 0x8DBD
    #endif
    #ifndef GL_COMPRESSED_SIGNED_RED_GREEN_RGTC2
    #define GL_COMPRESSED_SIGNED_RED_GREEN_RGTC2 0x8DBE
    #endif
    #ifndef GL_COMPRESSED_RGBA_BPTC_UNORM_ARB
    #define GL_COMPRESSED_RGBA_BPTC_UNORM_ARB 0x8E8C
    #endif
    #ifndef GL_COMPRESSED_SRGB_ALPHA_BPTC_UNORM_ARB
    #define GL_COMPRESSED_SRGB_ALPHA_BPTC_UNORM_ARB 0x8E8D
    #endif
    #ifndef GL_COMPRESSED_RGB_BPTC_SIGNED_FLOAT_ARB
    #define GL_COMPRESSED_RGB_BPTC_SIGNED_FLOAT_ARB 0x8E8E
    #endif
    #ifndef GL_COMPRESSED_RGB_BPTC_UNSIGNED_FLOAT_ARB
    #define GL_COMPRESSED_RGB_BPTC_UNSIGNED_FLOAT_ARB 0x8E8F
    #endif
    #ifndef GL_COMPRESSED_RGB_PVRTC_2BPPV1_IMG
    #define GL_COMPRESSED_RGB_PVRTC_2BPPV1_IMG 0x8C01
    #endif
    #ifndef GL_COMPRESSED_RGB_PVRTC_4BPPV1_IMG
    #define GL_COMPRESSED_RGB_PVRTC_4BPPV1_IMG 0x8C00
    #endif
    #ifndef GL_COMPRESSED_RGBA_PVRTC_2BPPV1_IMG
    #define GL_COMPRESSED_RGBA_PVRTC_2BPPV1_IMG 0x8C03
    #endif
    #ifndef GL_COMPRESSED_RGBA_PVRTC_4BPPV1_IMG
    #define GL_COMPRESSED_RGBA_PVRTC_4BPPV1_IMG 0x8C02
    #endif
    #ifndef GL_COMPRESSED_RGB8_ETC2
    #define GL_COMPRESSED_RGB8_ETC2 0x9274
    #endif
    #ifndef GL_COMPRESSED_RGBA8_ETC2_EAC
    #define GL_COMPRESSED_RGBA8_ETC2_EAC 0x9278
    #endif
    #ifndef GL_COMPRESSED_RGB8_PUNCHTHROUGH_ALPHA1_ETC2
    #define GL_COMPRESSED_RGB8_PUNCHTHROUGH_ALPHA1_ETC2 0x9276
    #endif
    #ifndef GL_COMPRESSED_RG11_EAC
    #define GL_COMPRESSED_RG11_EAC 0x9272
    #endif
    #ifndef GL_COMPRESSED_SIGNED_RG11_EAC
    #define GL_COMPRESSED_SIGNED_RG11_EAC 0x9273
    #endif
    #ifndef GL_DEPTH24_STENCIL8
    #define GL_DEPTH24_STENCIL8 0x88F0
    #endif
    #ifndef GL_HALF_FLOAT
    #define GL_HALF_FLOAT 0x140B
    #endif
    #ifndef GL_DEPTH_STENCIL
    #define GL_DEPTH_STENCIL 0x84F9
    #endif
    #ifndef GL_LUMINANCE
    #define GL_LUMINANCE 0x1909
    #endif

    #ifdef SOKOL_GLES2
        #ifdef GL_ANGLE_instanced_arrays
            #define _SOKOL_GL_INSTANCING_ENABLED
            #define glDrawArraysInstanced(mode, first, count, instancecount)  glDrawArraysInstancedANGLE(mode, first, count, instancecount)
            #define glDrawElementsInstanced(mode, count, type, indices, instancecount) glDrawElementsInstancedANGLE(mode, count, type, indices, instancecount)
            #define glVertexAttribDivisor(index, divisor) glVertexAttribDivisorANGLE(index, divisor)
        #elif defined(GL_EXT_draw_instanced) && defined(GL_EXT_instanced_arrays)
            #define _SOKOL_GL_INSTANCING_ENABLED
            #define glDrawArraysInstanced(mode, first, count, instancecount)  glDrawArraysInstancedEXT(mode, first, count, instancecount)
            #define glDrawElementsInstanced(mode, count, type, indices, instancecount) glDrawElementsInstancedEXT(mode, count, type, indices, instancecount)
            #define glVertexAttribDivisor(index, divisor) glVertexAttribDivisorEXT(index, divisor)
        #else
            #define _SOKOL_GLES2_INSTANCING_ERROR "Select GL_ANGLE_instanced_arrays or (GL_EXT_draw_instanced & GL_EXT_instanced_arrays) to enable instancing in GLES2"
            #define glDrawArraysInstanced(mode, first, count, instancecount) SOKOL_ASSERT(0 && _SOKOL_GLES2_INSTANCING_ERROR)
            #define glDrawElementsInstanced(mode, count, type, indices, instancecount) SOKOL_ASSERT(0 && _SOKOL_GLES2_INSTANCING_ERROR)
            #define glVertexAttribDivisor(index, divisor) SOKOL_ASSERT(0 && _SOKOL_GLES2_INSTANCING_ERROR)
        #endif
    #else
        #define _SOKOL_GL_INSTANCING_ENABLED
    #endif
    #define _SG_GL_CHECK_ERROR() { SOKOL_ASSERT(glGetError() == GL_NO_ERROR); }
#endif

/*=== COMMON BACKEND STUFF ===================================================*/

/* resource pool slots */
typedef struct {
    uint32_t id;
    uint32_t ctx_id;
    sg_resource_state state;
} _sg_slot_t;

/* constants */
enum {
    _SG_STRING_SIZE = 16,
    _SG_SLOT_SHIFT = 16,
    _SG_SLOT_MASK = (1<<_SG_SLOT_SHIFT)-1,
    _SG_MAX_POOL_SIZE = (1<<_SG_SLOT_SHIFT),
    _SG_DEFAULT_BUFFER_POOL_SIZE = 128,
    _SG_DEFAULT_IMAGE_POOL_SIZE = 128,
    _SG_DEFAULT_SHADER_POOL_SIZE = 32,
    _SG_DEFAULT_PIPELINE_POOL_SIZE = 64,
    _SG_DEFAULT_PASS_POOL_SIZE = 16,
    _SG_DEFAULT_CONTEXT_POOL_SIZE = 16,
    _SG_DEFAULT_SAMPLER_CACHE_CAPACITY = 64,
    _SG_DEFAULT_UB_SIZE = 4 * 1024 * 1024,
    _SG_DEFAULT_STAGING_SIZE = 8 * 1024 * 1024,
    _SG_DEFAULT_MAX_COMMIT_LISTENERS = 1024,
};

/* fixed-size string */
typedef struct {
    char buf[_SG_STRING_SIZE];
} _sg_str_t;

/* helper macros */
#define _sg_def(val, def) (((val) == 0) ? (def) : (val))
#define _sg_def_flt(val, def) (((val) == 0.0f) ? (def) : (val))
#define _sg_min(a,b) (((a)<(b))?(a):(b))
#define _sg_max(a,b) (((a)>(b))?(a):(b))
#define _sg_clamp(v,v0,v1) (((v)<(v0))?(v0):(((v)>(v1))?(v1):(v)))
#define _sg_fequal(val,cmp,delta) ((((val)-(cmp))> -(delta))&&(((val)-(cmp))<(delta)))

_SOKOL_PRIVATE void* _sg_malloc_clear(size_t size);
_SOKOL_PRIVATE void _sg_free(void* ptr);
_SOKOL_PRIVATE void _sg_clear(void* ptr, size_t size);

typedef struct {
    int size;
    int append_pos;
    bool append_overflow;
    sg_buffer_type type;
    sg_usage usage;
    uint32_t update_frame_index;
    uint32_t append_frame_index;
    int num_slots;
    int active_slot;
} _sg_buffer_common_t;

_SOKOL_PRIVATE void _sg_buffer_common_init(_sg_buffer_common_t* cmn, const sg_buffer_desc* desc) {
    cmn->size = (int)desc->size;
    cmn->append_pos = 0;
    cmn->append_overflow = false;
    cmn->type = desc->type;
    cmn->usage = desc->usage;
    cmn->update_frame_index = 0;
    cmn->append_frame_index = 0;
    cmn->num_slots = (cmn->usage == SG_USAGE_IMMUTABLE) ? 1 : SG_NUM_INFLIGHT_FRAMES;
    cmn->active_slot = 0;
}

typedef struct {
    sg_image_type type;
    bool render_target;
    int width;
    int height;
    int num_slices;
    int num_mipmaps;
    sg_usage usage;
    sg_pixel_format pixel_format;
    int sample_count;
    sg_filter min_filter;
    sg_filter mag_filter;
    sg_wrap wrap_u;
    sg_wrap wrap_v;
    sg_wrap wrap_w;
    sg_border_color border_color;
    uint32_t max_anisotropy;
    uint32_t upd_frame_index;
    int num_slots;
    int active_slot;
} _sg_image_common_t;

_SOKOL_PRIVATE void _sg_image_common_init(_sg_image_common_t* cmn, const sg_image_desc* desc) {
    cmn->type = desc->type;
    cmn->render_target = desc->render_target;
    cmn->width = desc->width;
    cmn->height = desc->height;
    cmn->num_slices = desc->num_slices;
    cmn->num_mipmaps = desc->num_mipmaps;
    cmn->usage = desc->usage;
    cmn->pixel_format = desc->pixel_format;
    cmn->sample_count = desc->sample_count;
    cmn->min_filter = desc->min_filter;
    cmn->mag_filter = desc->mag_filter;
    cmn->wrap_u = desc->wrap_u;
    cmn->wrap_v = desc->wrap_v;
    cmn->wrap_w = desc->wrap_w;
    cmn->border_color = desc->border_color;
    cmn->max_anisotropy = desc->max_anisotropy;
    cmn->upd_frame_index = 0;
    cmn->num_slots = (cmn->usage == SG_USAGE_IMMUTABLE) ? 1 : SG_NUM_INFLIGHT_FRAMES;
    cmn->active_slot = 0;
}

typedef struct {
    size_t size;
} _sg_uniform_block_t;

typedef struct {
    sg_image_type image_type;
    sg_sampler_type sampler_type;
} _sg_shader_image_t;

typedef struct {
    int num_uniform_blocks;
    int num_images;
    _sg_uniform_block_t uniform_blocks[SG_MAX_SHADERSTAGE_UBS];
    _sg_shader_image_t images[SG_MAX_SHADERSTAGE_IMAGES];
} _sg_shader_stage_t;

typedef struct {
    _sg_shader_stage_t stage[SG_NUM_SHADER_STAGES];
} _sg_shader_common_t;

_SOKOL_PRIVATE void _sg_shader_common_init(_sg_shader_common_t* cmn, const sg_shader_desc* desc) {
    for (int stage_index = 0; stage_index < SG_NUM_SHADER_STAGES; stage_index++) {
        const sg_shader_stage_desc* stage_desc = (stage_index == SG_SHADERSTAGE_VS) ? &desc->vs : &desc->fs;
        _sg_shader_stage_t* stage = &cmn->stage[stage_index];
        SOKOL_ASSERT(stage->num_uniform_blocks == 0);
        for (int ub_index = 0; ub_index < SG_MAX_SHADERSTAGE_UBS; ub_index++) {
            const sg_shader_uniform_block_desc* ub_desc = &stage_desc->uniform_blocks[ub_index];
            if (0 == ub_desc->size) {
                break;
            }
            stage->uniform_blocks[ub_index].size = ub_desc->size;
            stage->num_uniform_blocks++;
        }
        SOKOL_ASSERT(stage->num_images == 0);
        for (int img_index = 0; img_index < SG_MAX_SHADERSTAGE_IMAGES; img_index++) {
            const sg_shader_image_desc* img_desc = &stage_desc->images[img_index];
            if (img_desc->image_type == _SG_IMAGETYPE_DEFAULT) {
                break;
            }
            stage->images[img_index].image_type = img_desc->image_type;
            stage->images[img_index].sampler_type = img_desc->sampler_type;
            stage->num_images++;
        }
    }
}

typedef struct {
    sg_shader shader_id;
    sg_index_type index_type;
    bool use_instanced_draw;
    bool vertex_layout_valid[SG_MAX_SHADERSTAGE_BUFFERS];
    int color_attachment_count;
    sg_pixel_format color_formats[SG_MAX_COLOR_ATTACHMENTS];
    sg_pixel_format depth_format;
    int sample_count;
    float depth_bias;
    float depth_bias_slope_scale;
    float depth_bias_clamp;
    sg_color blend_color;
} _sg_pipeline_common_t;

_SOKOL_PRIVATE void _sg_pipeline_common_init(_sg_pipeline_common_t* cmn, const sg_pipeline_desc* desc) {
    SOKOL_ASSERT((desc->color_count >= 1) && (desc->color_count <= SG_MAX_COLOR_ATTACHMENTS));
    cmn->shader_id = desc->shader;
    cmn->index_type = desc->index_type;
    cmn->use_instanced_draw = false;
    for (int i = 0; i < SG_MAX_SHADERSTAGE_BUFFERS; i++) {
        cmn->vertex_layout_valid[i] = false;
    }
    cmn->color_attachment_count = desc->color_count;
    for (int i = 0; i < cmn->color_attachment_count; i++) {
        cmn->color_formats[i] = desc->colors[i].pixel_format;
    }
    cmn->depth_format = desc->depth.pixel_format;
    cmn->sample_count = desc->sample_count;
    cmn->depth_bias = desc->depth.bias;
    cmn->depth_bias_slope_scale = desc->depth.bias_slope_scale;
    cmn->depth_bias_clamp = desc->depth.bias_clamp;
    cmn->blend_color = desc->blend_color;
}

typedef struct {
    sg_image image_id;
    int mip_level;
    int slice;
} _sg_pass_attachment_common_t;

typedef struct {
    int num_color_atts;
    _sg_pass_attachment_common_t color_atts[SG_MAX_COLOR_ATTACHMENTS];
    _sg_pass_attachment_common_t ds_att;
} _sg_pass_common_t;

_SOKOL_PRIVATE void _sg_pass_common_init(_sg_pass_common_t* cmn, const sg_pass_desc* desc) {
    const sg_pass_attachment_desc* att_desc;
    _sg_pass_attachment_common_t* att;
    for (int i = 0; i < SG_MAX_COLOR_ATTACHMENTS; i++) {
        att_desc = &desc->color_attachments[i];
        if (att_desc->image.id != SG_INVALID_ID) {
            cmn->num_color_atts++;
            att = &cmn->color_atts[i];
            att->image_id = att_desc->image;
            att->mip_level = att_desc->mip_level;
            att->slice = att_desc->slice;
        }
    }
    att_desc = &desc->depth_stencil_attachment;
    if (att_desc->image.id != SG_INVALID_ID) {
        att = &cmn->ds_att;
        att->image_id = att_desc->image;
        att->mip_level = att_desc->mip_level;
        att->slice = att_desc->slice;
    }
}

/*=== GENERIC SAMPLER CACHE ==================================================*/

/*
    this is used by the Metal and WGPU backends to reduce the
    number of sampler state objects created through the backend API
*/
typedef struct {
    sg_filter min_filter;
    sg_filter mag_filter;
    sg_wrap wrap_u;
    sg_wrap wrap_v;
    sg_wrap wrap_w;
    sg_border_color border_color;
    uint32_t max_anisotropy;
    int min_lod;    /* orig min/max_lod is float, this is int(min/max_lod*1000.0) */
    int max_lod;
    uintptr_t sampler_handle;
} _sg_sampler_cache_item_t;

typedef struct {
    int capacity;
    int num_items;
    _sg_sampler_cache_item_t* items;
} _sg_sampler_cache_t;

_SOKOL_PRIVATE void _sg_smpcache_init(_sg_sampler_cache_t* cache, int capacity) {
    SOKOL_ASSERT(cache && (capacity > 0));
    _sg_clear(cache, sizeof(_sg_sampler_cache_t));
    cache->capacity = capacity;
    const size_t size = (size_t)cache->capacity * sizeof(_sg_sampler_cache_item_t);
    cache->items = (_sg_sampler_cache_item_t*) _sg_malloc_clear(size);
}

_SOKOL_PRIVATE void _sg_smpcache_discard(_sg_sampler_cache_t* cache) {
    SOKOL_ASSERT(cache && cache->items);
    _sg_free(cache->items);
    cache->items = 0;
    cache->num_items = 0;
    cache->capacity = 0;
}

_SOKOL_PRIVATE int _sg_smpcache_minlod_int(float min_lod) {
    return (int) (min_lod * 1000.0f);
}

_SOKOL_PRIVATE int _sg_smpcache_maxlod_int(float max_lod) {
    return (int) (_sg_clamp(max_lod, 0.0f, 1000.0f) * 1000.0f);
}

_SOKOL_PRIVATE int _sg_smpcache_find_item(const _sg_sampler_cache_t* cache, const sg_image_desc* img_desc) {
    /* return matching sampler cache item index or -1 */
    SOKOL_ASSERT(cache && cache->items);
    SOKOL_ASSERT(img_desc);
    const int min_lod = _sg_smpcache_minlod_int(img_desc->min_lod);
    const int max_lod = _sg_smpcache_maxlod_int(img_desc->max_lod);
    for (int i = 0; i < cache->num_items; i++) {
        const _sg_sampler_cache_item_t* item = &cache->items[i];
        if ((img_desc->min_filter == item->min_filter) &&
            (img_desc->mag_filter == item->mag_filter) &&
            (img_desc->wrap_u == item->wrap_u) &&
            (img_desc->wrap_v == item->wrap_v) &&
            (img_desc->wrap_w == item->wrap_w) &&
            (img_desc->max_anisotropy == item->max_anisotropy) &&
            (img_desc->border_color == item->border_color) &&
            (min_lod == item->min_lod) &&
            (max_lod == item->max_lod))
        {
            return i;
        }
    }
    /* fallthrough: no matching cache item found */
    return -1;
}

_SOKOL_PRIVATE void _sg_smpcache_add_item(_sg_sampler_cache_t* cache, const sg_image_desc* img_desc, uintptr_t sampler_handle) {
    SOKOL_ASSERT(cache && cache->items);
    SOKOL_ASSERT(img_desc);
    SOKOL_ASSERT(cache->num_items < cache->capacity);
    const int item_index = cache->num_items++;
    _sg_sampler_cache_item_t* item = &cache->items[item_index];
    item->min_filter = img_desc->min_filter;
    item->mag_filter = img_desc->mag_filter;
    item->wrap_u = img_desc->wrap_u;
    item->wrap_v = img_desc->wrap_v;
    item->wrap_w = img_desc->wrap_w;
    item->border_color = img_desc->border_color;
    item->max_anisotropy = img_desc->max_anisotropy;
    item->min_lod = _sg_smpcache_minlod_int(img_desc->min_lod);
    item->max_lod = _sg_smpcache_maxlod_int(img_desc->max_lod);
    item->sampler_handle = sampler_handle;
}

_SOKOL_PRIVATE uintptr_t _sg_smpcache_sampler(_sg_sampler_cache_t* cache, int item_index) {
    SOKOL_ASSERT(cache && cache->items);
    SOKOL_ASSERT(item_index < cache->num_items);
    return cache->items[item_index].sampler_handle;
}

/*=== DUMMY BACKEND DECLARATIONS =============================================*/
#if defined(SOKOL_DUMMY_BACKEND)
typedef struct {
    _sg_slot_t slot;
    _sg_buffer_common_t cmn;
} _sg_dummy_buffer_t;
typedef _sg_dummy_buffer_t _sg_buffer_t;

typedef struct {
    _sg_slot_t slot;
    _sg_image_common_t cmn;
} _sg_dummy_image_t;
typedef _sg_dummy_image_t _sg_image_t;

typedef struct {
    _sg_slot_t slot;
    _sg_shader_common_t cmn;
} _sg_dummy_shader_t;
typedef _sg_dummy_shader_t _sg_shader_t;

typedef struct {
    _sg_slot_t slot;
    _sg_shader_t* shader;
    _sg_pipeline_common_t cmn;
} _sg_dummy_pipeline_t;
typedef _sg_dummy_pipeline_t _sg_pipeline_t;

typedef struct {
    _sg_image_t* image;
} _sg_dummy_attachment_t;

typedef struct {
    _sg_slot_t slot;
    _sg_pass_common_t cmn;
    struct {
        _sg_dummy_attachment_t color_atts[SG_MAX_COLOR_ATTACHMENTS];
        _sg_dummy_attachment_t ds_att;
    } dmy;
} _sg_dummy_pass_t;
typedef _sg_dummy_pass_t _sg_pass_t;
typedef _sg_pass_attachment_common_t _sg_pass_attachment_t;

typedef struct {
    _sg_slot_t slot;
} _sg_dummy_context_t;
typedef _sg_dummy_context_t _sg_context_t;

/*== GL BACKEND DECLARATIONS =================================================*/
#elif defined(_SOKOL_ANY_GL)
typedef struct {
    _sg_slot_t slot;
    _sg_buffer_common_t cmn;
    struct {
        GLuint buf[SG_NUM_INFLIGHT_FRAMES];
        bool ext_buffers;   /* if true, external buffers were injected with sg_buffer_desc.gl_buffers */
    } gl;
} _sg_gl_buffer_t;
typedef _sg_gl_buffer_t _sg_buffer_t;

typedef struct {
    _sg_slot_t slot;
    _sg_image_common_t cmn;
    struct {
        GLenum target;
        GLuint depth_render_buffer;
        GLuint msaa_render_buffer;
        GLuint tex[SG_NUM_INFLIGHT_FRAMES];
        bool ext_textures;  /* if true, external textures were injected with sg_image_desc.gl_textures */
    } gl;
} _sg_gl_image_t;
typedef _sg_gl_image_t _sg_image_t;

typedef struct {
    GLint gl_loc;
    sg_uniform_type type;
    uint16_t count;
    uint16_t offset;
} _sg_gl_uniform_t;

typedef struct {
    int num_uniforms;
    _sg_gl_uniform_t uniforms[SG_MAX_UB_MEMBERS];
} _sg_gl_uniform_block_t;

typedef struct {
    int gl_tex_slot;
} _sg_gl_shader_image_t;

typedef struct {
    _sg_str_t name;
} _sg_gl_shader_attr_t;

typedef struct {
    _sg_gl_uniform_block_t uniform_blocks[SG_MAX_SHADERSTAGE_UBS];
    _sg_gl_shader_image_t images[SG_MAX_SHADERSTAGE_IMAGES];
} _sg_gl_shader_stage_t;

typedef struct {
    _sg_slot_t slot;
    _sg_shader_common_t cmn;
    struct {
        GLuint prog;
        _sg_gl_shader_attr_t attrs[SG_MAX_VERTEX_ATTRIBUTES];
        _sg_gl_shader_stage_t stage[SG_NUM_SHADER_STAGES];
    } gl;
} _sg_gl_shader_t;
typedef _sg_gl_shader_t _sg_shader_t;

typedef struct {
    int8_t vb_index;        /* -1 if attr is not enabled */
    int8_t divisor;         /* -1 if not initialized */
    uint8_t stride;
    uint8_t size;
    uint8_t normalized;
    int offset;
    GLenum type;
} _sg_gl_attr_t;

typedef struct {
    _sg_slot_t slot;
    _sg_pipeline_common_t cmn;
    _sg_shader_t* shader;
    struct {
        _sg_gl_attr_t attrs[SG_MAX_VERTEX_ATTRIBUTES];
        sg_depth_state depth;
        sg_stencil_state stencil;
        sg_primitive_type primitive_type;
        sg_blend_state blend;
        sg_color_mask color_write_mask[SG_MAX_COLOR_ATTACHMENTS];
        sg_cull_mode cull_mode;
        sg_face_winding face_winding;
        int sample_count;
        bool alpha_to_coverage_enabled;
    } gl;
} _sg_gl_pipeline_t;
typedef _sg_gl_pipeline_t _sg_pipeline_t;

typedef struct {
    _sg_image_t* image;
    GLuint gl_msaa_resolve_buffer;
} _sg_gl_attachment_t;

typedef struct {
    _sg_slot_t slot;
    _sg_pass_common_t cmn;
    struct {
        GLuint fb;
        _sg_gl_attachment_t color_atts[SG_MAX_COLOR_ATTACHMENTS];
        _sg_gl_attachment_t ds_att;
    } gl;
} _sg_gl_pass_t;
typedef _sg_gl_pass_t _sg_pass_t;
typedef _sg_pass_attachment_common_t _sg_pass_attachment_t;

typedef struct {
    _sg_slot_t slot;
    #if !defined(SOKOL_GLES2)
    GLuint vao;
    #endif
    GLuint default_framebuffer;
} _sg_gl_context_t;
typedef _sg_gl_context_t _sg_context_t;

typedef struct {
    _sg_gl_attr_t gl_attr;
    GLuint gl_vbuf;
} _sg_gl_cache_attr_t;

typedef struct {
    GLenum target;
    GLuint texture;
} _sg_gl_texture_bind_slot;

typedef struct {
    sg_depth_state depth;
    sg_stencil_state stencil;
    sg_blend_state blend;
    sg_color_mask color_write_mask[SG_MAX_COLOR_ATTACHMENTS];
    sg_cull_mode cull_mode;
    sg_face_winding face_winding;
    bool polygon_offset_enabled;
    int sample_count;
    sg_color blend_color;
    bool alpha_to_coverage_enabled;
    _sg_gl_cache_attr_t attrs[SG_MAX_VERTEX_ATTRIBUTES];
    GLuint vertex_buffer;
    GLuint index_buffer;
    GLuint stored_vertex_buffer;
    GLuint stored_index_buffer;
    GLuint prog;
    _sg_gl_texture_bind_slot textures[SG_MAX_SHADERSTAGE_IMAGES];
    _sg_gl_texture_bind_slot stored_texture;
    int cur_ib_offset;
    GLenum cur_primitive_type;
    GLenum cur_index_type;
    GLenum cur_active_texture;
    _sg_pipeline_t* cur_pipeline;
    sg_pipeline cur_pipeline_id;
} _sg_gl_state_cache_t;

typedef struct {
    bool valid;
    bool gles2;
    bool in_pass;
    int cur_pass_width;
    int cur_pass_height;
    _sg_context_t* cur_context;
    _sg_pass_t* cur_pass;
    sg_pass cur_pass_id;
    _sg_gl_state_cache_t cache;
    bool ext_anisotropic;
    GLint max_anisotropy;
    GLint max_combined_texture_image_units;
    #if _SOKOL_USE_WIN32_GL_LOADER
    HINSTANCE opengl32_dll;
    #endif
} _sg_gl_backend_t;

/*== D3D11 BACKEND DECLARATIONS ==============================================*/
#elif defined(SOKOL_D3D11)

typedef struct {
    _sg_slot_t slot;
    _sg_buffer_common_t cmn;
    struct {
        ID3D11Buffer* buf;
    } d3d11;
} _sg_d3d11_buffer_t;
typedef _sg_d3d11_buffer_t _sg_buffer_t;

typedef struct {
    _sg_slot_t slot;
    _sg_image_common_t cmn;
    struct {
        DXGI_FORMAT format;
        ID3D11Texture2D* tex2d;
        ID3D11Texture3D* tex3d;
        ID3D11Texture2D* texds;
        ID3D11Texture2D* texmsaa;
        ID3D11ShaderResourceView* srv;
        ID3D11SamplerState* smp;
    } d3d11;
} _sg_d3d11_image_t;
typedef _sg_d3d11_image_t _sg_image_t;

typedef struct {
    _sg_str_t sem_name;
    int sem_index;
} _sg_d3d11_shader_attr_t;

typedef struct {
    ID3D11Buffer* cbufs[SG_MAX_SHADERSTAGE_UBS];
} _sg_d3d11_shader_stage_t;

typedef struct {
    _sg_slot_t slot;
    _sg_shader_common_t cmn;
    struct {
        _sg_d3d11_shader_attr_t attrs[SG_MAX_VERTEX_ATTRIBUTES];
        _sg_d3d11_shader_stage_t stage[SG_NUM_SHADER_STAGES];
        ID3D11VertexShader* vs;
        ID3D11PixelShader* fs;
        void* vs_blob;
        size_t vs_blob_length;
    } d3d11;
} _sg_d3d11_shader_t;
typedef _sg_d3d11_shader_t _sg_shader_t;

typedef struct {
    _sg_slot_t slot;
    _sg_pipeline_common_t cmn;
    _sg_shader_t* shader;
    struct {
        UINT stencil_ref;
        UINT vb_strides[SG_MAX_SHADERSTAGE_BUFFERS];
        D3D_PRIMITIVE_TOPOLOGY topology;
        DXGI_FORMAT index_format;
        ID3D11InputLayout* il;
        ID3D11RasterizerState* rs;
        ID3D11DepthStencilState* dss;
        ID3D11BlendState* bs;
    } d3d11;
} _sg_d3d11_pipeline_t;
typedef _sg_d3d11_pipeline_t _sg_pipeline_t;

typedef struct {
    _sg_image_t* image;
    ID3D11RenderTargetView* rtv;
} _sg_d3d11_color_attachment_t;

typedef struct {
    _sg_image_t* image;
    ID3D11DepthStencilView* dsv;
} _sg_d3d11_ds_attachment_t;

typedef struct {
    _sg_slot_t slot;
    _sg_pass_common_t cmn;
    struct {
        _sg_d3d11_color_attachment_t color_atts[SG_MAX_COLOR_ATTACHMENTS];
        _sg_d3d11_ds_attachment_t ds_att;
    } d3d11;
} _sg_d3d11_pass_t;
typedef _sg_d3d11_pass_t _sg_pass_t;
typedef _sg_pass_attachment_common_t _sg_pass_attachment_t;

typedef struct {
    _sg_slot_t slot;
} _sg_d3d11_context_t;
typedef _sg_d3d11_context_t _sg_context_t;

typedef struct {
    bool valid;
    ID3D11Device* dev;
    ID3D11DeviceContext* ctx;
    const void* (*rtv_cb)(void);
    const void* (*rtv_userdata_cb)(void*);
    const void* (*dsv_cb)(void);
    const void* (*dsv_userdata_cb)(void*);
    void* user_data;
    bool in_pass;
    bool use_indexed_draw;
    bool use_instanced_draw;
    int cur_width;
    int cur_height;
    int num_rtvs;
    _sg_pass_t* cur_pass;
    sg_pass cur_pass_id;
    _sg_pipeline_t* cur_pipeline;
    sg_pipeline cur_pipeline_id;
    ID3D11RenderTargetView* cur_rtvs[SG_MAX_COLOR_ATTACHMENTS];
    ID3D11DepthStencilView* cur_dsv;
    /* on-demand loaded d3dcompiler_47.dll handles */
    HINSTANCE d3dcompiler_dll;
    bool d3dcompiler_dll_load_failed;
    pD3DCompile D3DCompile_func;
    /* global subresourcedata array for texture updates */
    D3D11_SUBRESOURCE_DATA subres_data[SG_MAX_MIPMAPS * SG_MAX_TEXTUREARRAY_LAYERS];
} _sg_d3d11_backend_t;

/*=== METAL BACKEND DECLARATIONS =============================================*/
#elif defined(SOKOL_METAL)

#if defined(_SG_TARGET_MACOS) || defined(_SG_TARGET_IOS_SIMULATOR)
#define _SG_MTL_UB_ALIGN (256)
#else
#define _SG_MTL_UB_ALIGN (16)
#endif
#define _SG_MTL_INVALID_SLOT_INDEX (0)

typedef struct {
    uint32_t frame_index;   /* frame index at which it is safe to release this resource */
    int slot_index;
} _sg_mtl_release_item_t;

typedef struct {
    NSMutableArray* pool;
    int num_slots;
    int free_queue_top;
    int* free_queue;
    int release_queue_front;
    int release_queue_back;
    _sg_mtl_release_item_t* release_queue;
} _sg_mtl_idpool_t;

typedef struct {
    _sg_slot_t slot;
    _sg_buffer_common_t cmn;
    struct {
        int buf[SG_NUM_INFLIGHT_FRAMES];  /* index into _sg_mtl_pool */
    } mtl;
} _sg_mtl_buffer_t;
typedef _sg_mtl_buffer_t _sg_buffer_t;

typedef struct {
    _sg_slot_t slot;
    _sg_image_common_t cmn;
    struct {
        int tex[SG_NUM_INFLIGHT_FRAMES];
        int depth_tex;
        int msaa_tex;
        int sampler_state;
    } mtl;
} _sg_mtl_image_t;
typedef _sg_mtl_image_t _sg_image_t;

typedef struct {
    int mtl_lib;
    int mtl_func;
} _sg_mtl_shader_stage_t;

typedef struct {
    _sg_slot_t slot;
    _sg_shader_common_t cmn;
    struct {
        _sg_mtl_shader_stage_t stage[SG_NUM_SHADER_STAGES];
    } mtl;
} _sg_mtl_shader_t;
typedef _sg_mtl_shader_t _sg_shader_t;

typedef struct {
    _sg_slot_t slot;
    _sg_pipeline_common_t cmn;
    _sg_shader_t* shader;
    struct {
        MTLPrimitiveType prim_type;
        int index_size;
        MTLIndexType index_type;
        MTLCullMode cull_mode;
        MTLWinding winding;
        uint32_t stencil_ref;
        int rps;
        int dss;
    } mtl;
} _sg_mtl_pipeline_t;
typedef _sg_mtl_pipeline_t _sg_pipeline_t;

typedef struct {
    _sg_image_t* image;
} _sg_mtl_attachment_t;

typedef struct {
    _sg_slot_t slot;
    _sg_pass_common_t cmn;
    struct {
        _sg_mtl_attachment_t color_atts[SG_MAX_COLOR_ATTACHMENTS];
        _sg_mtl_attachment_t ds_att;
    } mtl;
} _sg_mtl_pass_t;
typedef _sg_mtl_pass_t _sg_pass_t;
typedef _sg_pass_attachment_common_t _sg_pass_attachment_t;

typedef struct {
    _sg_slot_t slot;
} _sg_mtl_context_t;
typedef _sg_mtl_context_t _sg_context_t;

/* resouce binding state cache */
typedef struct {
    const _sg_pipeline_t* cur_pipeline;
    sg_pipeline cur_pipeline_id;
    const _sg_buffer_t* cur_indexbuffer;
    int cur_indexbuffer_offset;
    sg_buffer cur_indexbuffer_id;
    const _sg_buffer_t* cur_vertexbuffers[SG_MAX_SHADERSTAGE_BUFFERS];
    int cur_vertexbuffer_offsets[SG_MAX_SHADERSTAGE_BUFFERS];
    sg_buffer cur_vertexbuffer_ids[SG_MAX_SHADERSTAGE_BUFFERS];
    const _sg_image_t* cur_vs_images[SG_MAX_SHADERSTAGE_IMAGES];
    sg_image cur_vs_image_ids[SG_MAX_SHADERSTAGE_IMAGES];
    const _sg_image_t* cur_fs_images[SG_MAX_SHADERSTAGE_IMAGES];
    sg_image cur_fs_image_ids[SG_MAX_SHADERSTAGE_IMAGES];
} _sg_mtl_state_cache_t;

typedef struct {
    bool valid;
    const void*(*renderpass_descriptor_cb)(void);
    const void*(*renderpass_descriptor_userdata_cb)(void*);
    const void*(*drawable_cb)(void);
    const void*(*drawable_userdata_cb)(void*);
    void* user_data;
    uint32_t frame_index;
    uint32_t cur_frame_rotate_index;
    int ub_size;
    int cur_ub_offset;
    uint8_t* cur_ub_base_ptr;
    bool in_pass;
    bool pass_valid;
    int cur_width;
    int cur_height;
    _sg_mtl_state_cache_t state_cache;
    _sg_sampler_cache_t sampler_cache;
    _sg_mtl_idpool_t idpool;
    dispatch_semaphore_t sem;
    id<MTLDevice> device;
    id<MTLCommandQueue> cmd_queue;
    id<MTLCommandBuffer> cmd_buffer;
    id<MTLCommandBuffer> present_cmd_buffer;
    id<MTLRenderCommandEncoder> cmd_encoder;
    id<MTLBuffer> uniform_buffers[SG_NUM_INFLIGHT_FRAMES];
} _sg_mtl_backend_t;

/*=== WGPU BACKEND DECLARATIONS ==============================================*/
#elif defined(SOKOL_WGPU)

#define _SG_WGPU_STAGING_ALIGN (256)
#define _SG_WGPU_STAGING_PIPELINE_SIZE (8)
#define _SG_WGPU_ROWPITCH_ALIGN (256)
#define _SG_WGPU_MAX_SHADERSTAGE_IMAGES (8)
#define _SG_WGPU_MAX_UNIFORM_UPDATE_SIZE (1<<16)

typedef struct {
    _sg_slot_t slot;
    _sg_buffer_common_t cmn;
    struct {
        WGPUBuffer buf;
    } wgpu;
} _sg_wgpu_buffer_t;
typedef _sg_wgpu_buffer_t _sg_buffer_t;

typedef struct {
    _sg_slot_t slot;
    _sg_image_common_t cmn;
    struct {
        WGPUTexture tex;
        WGPUTextureView tex_view;
        WGPUTexture msaa_tex;
        WGPUSampler sampler;
    } wgpu;
} _sg_wgpu_image_t;
typedef _sg_wgpu_image_t _sg_image_t;

typedef struct {
    WGPUShaderModule module;
    WGPUBindGroupLayout bind_group_layout;
    _sg_str_t entry;
} _sg_wgpu_shader_stage_t;

typedef struct {
    _sg_slot_t slot;
    _sg_shader_common_t cmn;
    struct {
        _sg_wgpu_shader_stage_t stage[SG_NUM_SHADER_STAGES];
    } wgpu;
} _sg_wgpu_shader_t;
typedef _sg_wgpu_shader_t _sg_shader_t;

typedef struct {
    _sg_slot_t slot;
    _sg_pipeline_common_t cmn;
    _sg_shader_t* shader;
    struct {
        WGPURenderPipeline pip;
        uint32_t stencil_ref;
    } wgpu;
} _sg_wgpu_pipeline_t;
typedef _sg_wgpu_pipeline_t _sg_pipeline_t;

typedef struct {
    _sg_image_t* image;
    WGPUTextureView render_tex_view;
    WGPUTextureView resolve_tex_view;
} _sg_wgpu_attachment_t;

typedef struct {
    _sg_slot_t slot;
    _sg_pass_common_t cmn;
    struct {
        _sg_wgpu_attachment_t color_atts[SG_MAX_COLOR_ATTACHMENTS];
        _sg_wgpu_attachment_t ds_att;
    } wgpu;
} _sg_wgpu_pass_t;
typedef _sg_wgpu_pass_t _sg_pass_t;
typedef _sg_pass_attachment_common_t _sg_pass_attachment_t;

typedef struct {
    _sg_slot_t slot;
} _sg_wgpu_context_t;
typedef _sg_wgpu_context_t _sg_context_t;

/* a pool of per-frame uniform buffers */
typedef struct {
    WGPUBindGroupLayout bindgroup_layout;
    uint32_t num_bytes;
    uint32_t offset;    /* current offset into current frame's mapped uniform buffer */
    uint32_t bind_offsets[SG_NUM_SHADER_STAGES][SG_MAX_SHADERSTAGE_UBS];
    WGPUBuffer buf;     /* the GPU-side uniform buffer */
    WGPUBindGroup bindgroup;
    struct {
        int num;
        int cur;
        WGPUBuffer buf[_SG_WGPU_STAGING_PIPELINE_SIZE]; /* CPU-side staging buffers */
        uint8_t* ptr[_SG_WGPU_STAGING_PIPELINE_SIZE];   /* if != 0, staging buffer currently mapped */
    } stage;
} _sg_wgpu_ubpool_t;

/* ...a similar pool (like uniform buffer pool) of dynamic-resource staging buffers */
typedef struct {
    uint32_t num_bytes;
    uint32_t offset;    /* current offset into current frame's staging buffer */
    int num;            /* number of staging buffers */
    int cur;            /* this frame's staging buffer */
    WGPUBuffer buf[_SG_WGPU_STAGING_PIPELINE_SIZE]; /* CPU-side staging buffers */
    uint8_t* ptr[_SG_WGPU_STAGING_PIPELINE_SIZE];   /* if != 0, staging buffer currently mapped */
} _sg_wgpu_stagingpool_t;

/* the WGPU backend state */
typedef struct {
    bool valid;
    bool in_pass;
    bool draw_indexed;
    int cur_width;
    int cur_height;
    WGPUDevice dev;
    WGPUTextureView (*render_view_cb)(void);
    WGPUTextureView (*render_view_userdata_cb)(void*);
    WGPUTextureView (*resolve_view_cb)(void);
    WGPUTextureView (*resolve_view_userdata_cb)(void*);
    WGPUTextureView (*depth_stencil_view_cb)(void);
    WGPUTextureView (*depth_stencil_view_userdata_cb)(void*);
    void* user_data;
    WGPUQueue queue;
    WGPUCommandEncoder render_cmd_enc;
    WGPUCommandEncoder staging_cmd_enc;
    WGPURenderPassEncoder pass_enc;
    WGPUBindGroup empty_bind_group;
    const _sg_pipeline_t* cur_pipeline;
    sg_pipeline cur_pipeline_id;
    _sg_sampler_cache_t sampler_cache;
    _sg_wgpu_ubpool_t ub;
    _sg_wgpu_stagingpool_t staging;
} _sg_wgpu_backend_t;
#endif

/*=== RESOURCE POOL DECLARATIONS =============================================*/

/* this *MUST* remain 0 */
#define _SG_INVALID_SLOT_INDEX (0)

typedef struct {
    int size;
    int queue_top;
    uint32_t* gen_ctrs;
    int* free_queue;
} _sg_pool_t;

typedef struct {
    _sg_pool_t buffer_pool;
    _sg_pool_t image_pool;
    _sg_pool_t shader_pool;
    _sg_pool_t pipeline_pool;
    _sg_pool_t pass_pool;
    _sg_pool_t context_pool;
    _sg_buffer_t* buffers;
    _sg_image_t* images;
    _sg_shader_t* shaders;
    _sg_pipeline_t* pipelines;
    _sg_pass_t* passes;
    _sg_context_t* contexts;
} _sg_pools_t;

/*=== VALIDATION LAYER DECLARATIONS ==========================================*/
typedef enum {
    /* special case 'validation was successful' */
    _SG_VALIDATE_SUCCESS,

    /* buffer creation */
    _SG_VALIDATE_BUFFERDESC_CANARY,
    _SG_VALIDATE_BUFFERDESC_SIZE,
    _SG_VALIDATE_BUFFERDESC_DATA,
    _SG_VALIDATE_BUFFERDESC_DATA_SIZE,
    _SG_VALIDATE_BUFFERDESC_NO_DATA,

    /* image data (for image creation and updating) */
    _SG_VALIDATE_IMAGEDATA_NODATA,
    _SG_VALIDATE_IMAGEDATA_DATA_SIZE,

    /* image creation */
    _SG_VALIDATE_IMAGEDESC_CANARY,
    _SG_VALIDATE_IMAGEDESC_WIDTH,
    _SG_VALIDATE_IMAGEDESC_HEIGHT,
    _SG_VALIDATE_IMAGEDESC_RT_PIXELFORMAT,
    _SG_VALIDATE_IMAGEDESC_NONRT_PIXELFORMAT,
    _SG_VALIDATE_IMAGEDESC_MSAA_BUT_NO_RT,
    _SG_VALIDATE_IMAGEDESC_NO_MSAA_RT_SUPPORT,
    _SG_VALIDATE_IMAGEDESC_RT_IMMUTABLE,
    _SG_VALIDATE_IMAGEDESC_RT_NO_DATA,
    _SG_VALIDATE_IMAGEDESC_INJECTED_NO_DATA,
    _SG_VALIDATE_IMAGEDESC_DYNAMIC_NO_DATA,
    _SG_VALIDATE_IMAGEDESC_COMPRESSED_IMMUTABLE,

    /* shader creation */
    _SG_VALIDATE_SHADERDESC_CANARY,
    _SG_VALIDATE_SHADERDESC_SOURCE,
    _SG_VALIDATE_SHADERDESC_BYTECODE,
    _SG_VALIDATE_SHADERDESC_SOURCE_OR_BYTECODE,
    _SG_VALIDATE_SHADERDESC_NO_BYTECODE_SIZE,
    _SG_VALIDATE_SHADERDESC_NO_CONT_UBS,
    _SG_VALIDATE_SHADERDESC_NO_CONT_IMGS,
    _SG_VALIDATE_SHADERDESC_NO_CONT_UB_MEMBERS,
    _SG_VALIDATE_SHADERDESC_NO_UB_MEMBERS,
    _SG_VALIDATE_SHADERDESC_UB_MEMBER_NAME,
    _SG_VALIDATE_SHADERDESC_UB_SIZE_MISMATCH,
    _SG_VALIDATE_SHADERDESC_UB_ARRAY_COUNT,
    _SG_VALIDATE_SHADERDESC_UB_STD140_ARRAY_TYPE,
    _SG_VALIDATE_SHADERDESC_IMG_NAME,
    _SG_VALIDATE_SHADERDESC_ATTR_NAMES,
    _SG_VALIDATE_SHADERDESC_ATTR_SEMANTICS,
    _SG_VALIDATE_SHADERDESC_ATTR_STRING_TOO_LONG,

    /* pipeline creation */
    _SG_VALIDATE_PIPELINEDESC_CANARY,
    _SG_VALIDATE_PIPELINEDESC_SHADER,
    _SG_VALIDATE_PIPELINEDESC_NO_ATTRS,
    _SG_VALIDATE_PIPELINEDESC_LAYOUT_STRIDE4,
    _SG_VALIDATE_PIPELINEDESC_ATTR_NAME,
    _SG_VALIDATE_PIPELINEDESC_ATTR_SEMANTICS,

    /* pass creation */
    _SG_VALIDATE_PASSDESC_CANARY,
    _SG_VALIDATE_PASSDESC_NO_COLOR_ATTS,
    _SG_VALIDATE_PASSDESC_NO_CONT_COLOR_ATTS,
    _SG_VALIDATE_PASSDESC_IMAGE,
    _SG_VALIDATE_PASSDESC_MIPLEVEL,
    _SG_VALIDATE_PASSDESC_FACE,
    _SG_VALIDATE_PASSDESC_LAYER,
    _SG_VALIDATE_PASSDESC_SLICE,
    _SG_VALIDATE_PASSDESC_IMAGE_NO_RT,
    _SG_VALIDATE_PASSDESC_COLOR_INV_PIXELFORMAT,
    _SG_VALIDATE_PASSDESC_DEPTH_INV_PIXELFORMAT,
    _SG_VALIDATE_PASSDESC_IMAGE_SIZES,
    _SG_VALIDATE_PASSDESC_IMAGE_SAMPLE_COUNTS,

    /* sg_begin_pass validation */
    _SG_VALIDATE_BEGINPASS_PASS,
    _SG_VALIDATE_BEGINPASS_IMAGE,

    /* sg_apply_pipeline validation */
    _SG_VALIDATE_APIP_PIPELINE_VALID_ID,
    _SG_VALIDATE_APIP_PIPELINE_EXISTS,
    _SG_VALIDATE_APIP_PIPELINE_VALID,
    _SG_VALIDATE_APIP_SHADER_EXISTS,
    _SG_VALIDATE_APIP_SHADER_VALID,
    _SG_VALIDATE_APIP_ATT_COUNT,
    _SG_VALIDATE_APIP_COLOR_FORMAT,
    _SG_VALIDATE_APIP_DEPTH_FORMAT,
    _SG_VALIDATE_APIP_SAMPLE_COUNT,

    /* sg_apply_bindings validation */
    _SG_VALIDATE_ABND_PIPELINE,
    _SG_VALIDATE_ABND_PIPELINE_EXISTS,
    _SG_VALIDATE_ABND_PIPELINE_VALID,
    _SG_VALIDATE_ABND_VBS,
    _SG_VALIDATE_ABND_VB_EXISTS,
    _SG_VALIDATE_ABND_VB_TYPE,
    _SG_VALIDATE_ABND_VB_OVERFLOW,
    _SG_VALIDATE_ABND_NO_IB,
    _SG_VALIDATE_ABND_IB,
    _SG_VALIDATE_ABND_IB_EXISTS,
    _SG_VALIDATE_ABND_IB_TYPE,
    _SG_VALIDATE_ABND_IB_OVERFLOW,
    _SG_VALIDATE_ABND_VS_IMGS,
    _SG_VALIDATE_ABND_VS_IMG_EXISTS,
    _SG_VALIDATE_ABND_VS_IMG_TYPES,
    _SG_VALIDATE_ABND_FS_IMGS,
    _SG_VALIDATE_ABND_FS_IMG_EXISTS,
    _SG_VALIDATE_ABND_FS_IMG_TYPES,

    /* sg_apply_uniforms validation */
    _SG_VALIDATE_AUB_NO_PIPELINE,
    _SG_VALIDATE_AUB_NO_UB_AT_SLOT,
    _SG_VALIDATE_AUB_SIZE,

    /* sg_update_buffer validation */
    _SG_VALIDATE_UPDATEBUF_USAGE,
    _SG_VALIDATE_UPDATEBUF_SIZE,
    _SG_VALIDATE_UPDATEBUF_ONCE,
    _SG_VALIDATE_UPDATEBUF_APPEND,

    /* sg_append_buffer validation */
    _SG_VALIDATE_APPENDBUF_USAGE,
    _SG_VALIDATE_APPENDBUF_SIZE,
    _SG_VALIDATE_APPENDBUF_UPDATE,

    /* sg_update_image validation */
    _SG_VALIDATE_UPDIMG_USAGE,
    _SG_VALIDATE_UPDIMG_NOTENOUGHDATA,
    _SG_VALIDATE_UPDIMG_ONCE
} _sg_validate_error_t;

/*=== GENERIC BACKEND STATE ==================================================*/

typedef struct {
    int num;        // number of allocated commit listener items
    int upper;      // the current upper index (no valid items past this point)
    sg_commit_listener* items;
} _sg_commit_listeners_t;

typedef struct {
    bool valid;
    sg_desc desc;       /* original desc with default values patched in */
    uint32_t frame_index;
    sg_context active_context;
    sg_pass cur_pass;
    sg_pipeline cur_pipeline;
    bool pass_valid;
    bool bindings_valid;
    bool next_draw_valid;
    #if defined(SOKOL_DEBUG)
    _sg_validate_error_t validate_error;
    #endif
    _sg_pools_t pools;
    sg_backend backend;
    sg_features features;
    sg_limits limits;
    sg_pixelformat_info formats[_SG_PIXELFORMAT_NUM];
    #if defined(_SOKOL_ANY_GL)
    _sg_gl_backend_t gl;
    #elif defined(SOKOL_METAL)
    _sg_mtl_backend_t mtl;
    #elif defined(SOKOL_D3D11)
    _sg_d3d11_backend_t d3d11;
    #elif defined(SOKOL_WGPU)
    _sg_wgpu_backend_t wgpu;
    #endif
    #if defined(SOKOL_TRACE_HOOKS)
    sg_trace_hooks hooks;
    #endif
    _sg_commit_listeners_t commit_listeners;
} _sg_state_t;
static _sg_state_t _sg;

/*-- helper functions --------------------------------------------------------*/

// a helper macro to clear a struct with potentially ARC'ed ObjC references
#if defined(SOKOL_METAL)
    #if defined(__cplusplus)
        #define _SG_CLEAR_ARC_STRUCT(type, item) { item = type(); }
    #else
        #define _SG_CLEAR_ARC_STRUCT(type, item) { item = (type) { 0 }; }
    #endif
#else
    #define _SG_CLEAR_ARC_STRUCT(type, item) { _sg_clear(&item, sizeof(item)); }
#endif

_SOKOL_PRIVATE void _sg_clear(void* ptr, size_t size) {
    SOKOL_ASSERT(ptr && (size > 0));
    memset(ptr, 0, size);
}

_SOKOL_PRIVATE void* _sg_malloc(size_t size) {
    SOKOL_ASSERT(size > 0);
    void* ptr;
    if (_sg.desc.allocator.alloc) {
        ptr = _sg.desc.allocator.alloc(size, _sg.desc.allocator.user_data);
    }
    else {
        ptr = malloc(size);
    }
    SOKOL_ASSERT(ptr);
    return ptr;
}

_SOKOL_PRIVATE void* _sg_malloc_clear(size_t size) {
    void* ptr = _sg_malloc(size);
    _sg_clear(ptr, size);
    return ptr;
}

_SOKOL_PRIVATE void _sg_free(void* ptr) {
    if (_sg.desc.allocator.free) {
        _sg.desc.allocator.free(ptr, _sg.desc.allocator.user_data);
    }
    else {
        free(ptr);
    }
}

#if defined(SOKOL_DEBUG)
_SOKOL_PRIVATE void _sg_log(const char* msg) {
    SOKOL_ASSERT(msg);
    if (_sg.desc.logger.log_cb) {
        _sg.desc.logger.log_cb(msg, _sg.desc.logger.user_data);
    } else {
        SOKOL_LOG(msg);
    }
}
#endif

_SOKOL_PRIVATE bool _sg_strempty(const _sg_str_t* str) {
    return 0 == str->buf[0];
}

_SOKOL_PRIVATE const char* _sg_strptr(const _sg_str_t* str) {
    return &str->buf[0];
}

_SOKOL_PRIVATE void _sg_strcpy(_sg_str_t* dst, const char* src) {
    SOKOL_ASSERT(dst);
    if (src) {
        #if defined(_MSC_VER)
        strncpy_s(dst->buf, _SG_STRING_SIZE, src, (_SG_STRING_SIZE-1));
        #else
        strncpy(dst->buf, src, _SG_STRING_SIZE);
        #endif
        dst->buf[_SG_STRING_SIZE-1] = 0;
    }
    else {
        _sg_clear(dst->buf, _SG_STRING_SIZE);
    }
}

_SOKOL_PRIVATE uint32_t _sg_align_u32(uint32_t val, uint32_t align) {
    SOKOL_ASSERT((align > 0) && ((align & (align - 1)) == 0));
    return (val + (align - 1)) & ~(align - 1);
}

/* return byte size of a vertex format */
_SOKOL_PRIVATE int _sg_vertexformat_bytesize(sg_vertex_format fmt) {
    switch (fmt) {
        case SG_VERTEXFORMAT_FLOAT:     return 4;
        case SG_VERTEXFORMAT_FLOAT2:    return 8;
        case SG_VERTEXFORMAT_FLOAT3:    return 12;
        case SG_VERTEXFORMAT_FLOAT4:    return 16;
        case SG_VERTEXFORMAT_BYTE4:     return 4;
        case SG_VERTEXFORMAT_BYTE4N:    return 4;
        case SG_VERTEXFORMAT_UBYTE4:    return 4;
        case SG_VERTEXFORMAT_UBYTE4N:   return 4;
        case SG_VERTEXFORMAT_SHORT2:    return 4;
        case SG_VERTEXFORMAT_SHORT2N:   return 4;
        case SG_VERTEXFORMAT_USHORT2N:  return 4;
        case SG_VERTEXFORMAT_SHORT4:    return 8;
        case SG_VERTEXFORMAT_SHORT4N:   return 8;
        case SG_VERTEXFORMAT_USHORT4N:  return 8;
        case SG_VERTEXFORMAT_UINT10_N2: return 4;
        case SG_VERTEXFORMAT_INVALID:   return 0;
        default:
            SOKOL_UNREACHABLE;
            return -1;
    }
}

_SOKOL_PRIVATE uint32_t _sg_uniform_alignment(sg_uniform_type type, int array_count, sg_uniform_layout ub_layout) {
    if (ub_layout == SG_UNIFORMLAYOUT_NATIVE) {
        return 1;
    }
    else {
        SOKOL_ASSERT(array_count > 0);
        if (array_count == 1) {
            switch (type) {
                case SG_UNIFORMTYPE_FLOAT:
                case SG_UNIFORMTYPE_INT:
                    return 4;
                case SG_UNIFORMTYPE_FLOAT2:
                case SG_UNIFORMTYPE_INT2:
                    return 8;
                case SG_UNIFORMTYPE_FLOAT3:
                case SG_UNIFORMTYPE_FLOAT4:
                case SG_UNIFORMTYPE_INT3:
                case SG_UNIFORMTYPE_INT4:
                    return 16;
                case SG_UNIFORMTYPE_MAT4:
                    return 16;
                default:
                    SOKOL_UNREACHABLE;
                    return 1;
            }
        }
        else {
            return 16;
        }
    }
}

_SOKOL_PRIVATE uint32_t _sg_uniform_size(sg_uniform_type type, int array_count, sg_uniform_layout ub_layout) {
    SOKOL_ASSERT(array_count > 0);
    if (array_count == 1) {
        switch (type) {
            case SG_UNIFORMTYPE_FLOAT:
            case SG_UNIFORMTYPE_INT:
                return 4;
            case SG_UNIFORMTYPE_FLOAT2:
            case SG_UNIFORMTYPE_INT2:
                return 8;
            case SG_UNIFORMTYPE_FLOAT3:
            case SG_UNIFORMTYPE_INT3:
                return 12;
            case SG_UNIFORMTYPE_FLOAT4:
            case SG_UNIFORMTYPE_INT4:
                return 16;
            case SG_UNIFORMTYPE_MAT4:
                return 64;
            default:
                SOKOL_UNREACHABLE;
                return 0;
        }
    }
    else {
        if (ub_layout == SG_UNIFORMLAYOUT_NATIVE) {
            switch (type) {
                case SG_UNIFORMTYPE_FLOAT:
                case SG_UNIFORMTYPE_INT:
                    return 4 * (uint32_t)array_count;
                case SG_UNIFORMTYPE_FLOAT2:
                case SG_UNIFORMTYPE_INT2:
                    return 8 * (uint32_t)array_count;
                case SG_UNIFORMTYPE_FLOAT3:
                case SG_UNIFORMTYPE_INT3:
                    return 12 * (uint32_t)array_count;
                case SG_UNIFORMTYPE_FLOAT4:
                case SG_UNIFORMTYPE_INT4:
                    return 16 * (uint32_t)array_count;
                case SG_UNIFORMTYPE_MAT4:
                    return 64 * (uint32_t)array_count;
                default:
                    SOKOL_UNREACHABLE;
                    return 0;
            }
        }
        else {
            switch (type) {
                case SG_UNIFORMTYPE_FLOAT:
                case SG_UNIFORMTYPE_FLOAT2:
                case SG_UNIFORMTYPE_FLOAT3:
                case SG_UNIFORMTYPE_FLOAT4:
                case SG_UNIFORMTYPE_INT:
                case SG_UNIFORMTYPE_INT2:
                case SG_UNIFORMTYPE_INT3:
                case SG_UNIFORMTYPE_INT4:
                    return 16 * (uint32_t)array_count;
                case SG_UNIFORMTYPE_MAT4:
                    return 64 * (uint32_t)array_count;
                default:
                    SOKOL_UNREACHABLE;
                    return 0;
            }
        }
    }
}

/* return true if pixel format is a compressed format */
_SOKOL_PRIVATE bool _sg_is_compressed_pixel_format(sg_pixel_format fmt) {
    switch (fmt) {
        case SG_PIXELFORMAT_BC1_RGBA:
        case SG_PIXELFORMAT_BC2_RGBA:
        case SG_PIXELFORMAT_BC3_RGBA:
        case SG_PIXELFORMAT_BC4_R:
        case SG_PIXELFORMAT_BC4_RSN:
        case SG_PIXELFORMAT_BC5_RG:
        case SG_PIXELFORMAT_BC5_RGSN:
        case SG_PIXELFORMAT_BC6H_RGBF:
        case SG_PIXELFORMAT_BC6H_RGBUF:
        case SG_PIXELFORMAT_BC7_RGBA:
        case SG_PIXELFORMAT_PVRTC_RGB_2BPP:
        case SG_PIXELFORMAT_PVRTC_RGB_4BPP:
        case SG_PIXELFORMAT_PVRTC_RGBA_2BPP:
        case SG_PIXELFORMAT_PVRTC_RGBA_4BPP:
        case SG_PIXELFORMAT_ETC2_RGB8:
        case SG_PIXELFORMAT_ETC2_RGB8A1:
        case SG_PIXELFORMAT_ETC2_RGBA8:
        case SG_PIXELFORMAT_ETC2_RG11:
        case SG_PIXELFORMAT_ETC2_RG11SN:
            return true;
        default:
            return false;
    }
}

/* return true if pixel format is a valid render target format */
_SOKOL_PRIVATE bool _sg_is_valid_rendertarget_color_format(sg_pixel_format fmt) {
    const int fmt_index = (int) fmt;
    SOKOL_ASSERT((fmt_index >= 0) && (fmt_index < _SG_PIXELFORMAT_NUM));
    return _sg.formats[fmt_index].render && !_sg.formats[fmt_index].depth;
}

/* return true if pixel format is a valid depth format */
_SOKOL_PRIVATE bool _sg_is_valid_rendertarget_depth_format(sg_pixel_format fmt) {
    const int fmt_index = (int) fmt;
    SOKOL_ASSERT((fmt_index >= 0) && (fmt_index < _SG_PIXELFORMAT_NUM));
    return _sg.formats[fmt_index].render && _sg.formats[fmt_index].depth;
}

/* return true if pixel format is a depth-stencil format */
_SOKOL_PRIVATE bool _sg_is_depth_stencil_format(sg_pixel_format fmt) {
    return (SG_PIXELFORMAT_DEPTH_STENCIL == fmt);
}

/* return the bytes-per-pixel for a pixel format */
_SOKOL_PRIVATE int _sg_pixelformat_bytesize(sg_pixel_format fmt) {
    switch (fmt) {
        case SG_PIXELFORMAT_R8:
        case SG_PIXELFORMAT_R8SN:
        case SG_PIXELFORMAT_R8UI:
        case SG_PIXELFORMAT_R8SI:
            return 1;

        case SG_PIXELFORMAT_R16:
        case SG_PIXELFORMAT_R16SN:
        case SG_PIXELFORMAT_R16UI:
        case SG_PIXELFORMAT_R16SI:
        case SG_PIXELFORMAT_R16F:
        case SG_PIXELFORMAT_RG8:
        case SG_PIXELFORMAT_RG8SN:
        case SG_PIXELFORMAT_RG8UI:
        case SG_PIXELFORMAT_RG8SI:
            return 2;

        case SG_PIXELFORMAT_R32UI:
        case SG_PIXELFORMAT_R32SI:
        case SG_PIXELFORMAT_R32F:
        case SG_PIXELFORMAT_RG16:
        case SG_PIXELFORMAT_RG16SN:
        case SG_PIXELFORMAT_RG16UI:
        case SG_PIXELFORMAT_RG16SI:
        case SG_PIXELFORMAT_RG16F:
        case SG_PIXELFORMAT_RGBA8:
        case SG_PIXELFORMAT_RGBA8SN:
        case SG_PIXELFORMAT_RGBA8UI:
        case SG_PIXELFORMAT_RGBA8SI:
        case SG_PIXELFORMAT_BGRA8:
        case SG_PIXELFORMAT_RGB10A2:
        case SG_PIXELFORMAT_RG11B10F:
        case SG_PIXELFORMAT_RGB9E5:
            return 4;

        case SG_PIXELFORMAT_RG32UI:
        case SG_PIXELFORMAT_RG32SI:
        case SG_PIXELFORMAT_RG32F:
        case SG_PIXELFORMAT_RGBA16:
        case SG_PIXELFORMAT_RGBA16SN:
        case SG_PIXELFORMAT_RGBA16UI:
        case SG_PIXELFORMAT_RGBA16SI:
        case SG_PIXELFORMAT_RGBA16F:
            return 8;

        case SG_PIXELFORMAT_RGBA32UI:
        case SG_PIXELFORMAT_RGBA32SI:
        case SG_PIXELFORMAT_RGBA32F:
            return 16;

        default:
            SOKOL_UNREACHABLE;
            return 0;
    }
}

_SOKOL_PRIVATE int _sg_roundup(int val, int round_to) {
    return (val+(round_to-1)) & ~(round_to-1);
}

/* return row pitch for an image

    see ComputePitch in https://github.com/microsoft/DirectXTex/blob/master/DirectXTex/DirectXTexUtil.cpp

    For the special PVRTC pitch computation, see:
    GL extension requirement (https://www.khronos.org/registry/OpenGL/extensions/IMG/IMG_texture_compression_pvrtc.txt)

    Quote:

    6) How is the imageSize argument calculated for the CompressedTexImage2D
       and CompressedTexSubImage2D functions.

       Resolution: For PVRTC 4BPP formats the imageSize is calculated as:
          ( max(width, 8) * max(height, 8) * 4 + 7) / 8
       For PVRTC 2BPP formats the imageSize is calculated as:
          ( max(width, 16) * max(height, 8) * 2 + 7) / 8
*/
_SOKOL_PRIVATE int _sg_row_pitch(sg_pixel_format fmt, int width, int row_align) {
    int pitch;
    switch (fmt) {
        case SG_PIXELFORMAT_BC1_RGBA:
        case SG_PIXELFORMAT_BC4_R:
        case SG_PIXELFORMAT_BC4_RSN:
        case SG_PIXELFORMAT_ETC2_RGB8:
        case SG_PIXELFORMAT_ETC2_RGB8A1:
            pitch = ((width + 3) / 4) * 8;
            pitch = pitch < 8 ? 8 : pitch;
            break;
        case SG_PIXELFORMAT_BC2_RGBA:
        case SG_PIXELFORMAT_BC3_RGBA:
        case SG_PIXELFORMAT_BC5_RG:
        case SG_PIXELFORMAT_BC5_RGSN:
        case SG_PIXELFORMAT_BC6H_RGBF:
        case SG_PIXELFORMAT_BC6H_RGBUF:
        case SG_PIXELFORMAT_BC7_RGBA:
        case SG_PIXELFORMAT_ETC2_RGBA8:
        case SG_PIXELFORMAT_ETC2_RG11:
        case SG_PIXELFORMAT_ETC2_RG11SN:
            pitch = ((width + 3) / 4) * 16;
            pitch = pitch < 16 ? 16 : pitch;
            break;
        case SG_PIXELFORMAT_PVRTC_RGB_4BPP:
        case SG_PIXELFORMAT_PVRTC_RGBA_4BPP:
            pitch = (_sg_max(width, 8) * 4 + 7) / 8;
            break;
        case SG_PIXELFORMAT_PVRTC_RGB_2BPP:
        case SG_PIXELFORMAT_PVRTC_RGBA_2BPP:
            pitch = (_sg_max(width, 16) * 2 + 7) / 8;
            break;
        default:
            pitch = width * _sg_pixelformat_bytesize(fmt);
            break;
    }
    pitch = _sg_roundup(pitch, row_align);
    return pitch;
}

/* compute the number of rows in a surface depending on pixel format */
_SOKOL_PRIVATE int _sg_num_rows(sg_pixel_format fmt, int height) {
    int num_rows;
    switch (fmt) {
        case SG_PIXELFORMAT_BC1_RGBA:
        case SG_PIXELFORMAT_BC4_R:
        case SG_PIXELFORMAT_BC4_RSN:
        case SG_PIXELFORMAT_ETC2_RGB8:
        case SG_PIXELFORMAT_ETC2_RGB8A1:
        case SG_PIXELFORMAT_ETC2_RGBA8:
        case SG_PIXELFORMAT_ETC2_RG11:
        case SG_PIXELFORMAT_ETC2_RG11SN:
        case SG_PIXELFORMAT_BC2_RGBA:
        case SG_PIXELFORMAT_BC3_RGBA:
        case SG_PIXELFORMAT_BC5_RG:
        case SG_PIXELFORMAT_BC5_RGSN:
        case SG_PIXELFORMAT_BC6H_RGBF:
        case SG_PIXELFORMAT_BC6H_RGBUF:
        case SG_PIXELFORMAT_BC7_RGBA:
            num_rows = ((height + 3) / 4);
            break;
        case SG_PIXELFORMAT_PVRTC_RGB_4BPP:
        case SG_PIXELFORMAT_PVRTC_RGBA_4BPP:
        case SG_PIXELFORMAT_PVRTC_RGB_2BPP:
        case SG_PIXELFORMAT_PVRTC_RGBA_2BPP:
            /* NOTE: this is most likely not correct because it ignores any
                PVCRTC block size, but multiplied with _sg_row_pitch()
                it gives the correct surface pitch.

                See: https://www.khronos.org/registry/OpenGL/extensions/IMG/IMG_texture_compression_pvrtc.txt
            */
            num_rows = ((_sg_max(height, 8) + 7) / 8) * 8;
            break;
        default:
            num_rows = height;
            break;
    }
    if (num_rows < 1) {
        num_rows = 1;
    }
    return num_rows;
}

/* return pitch of a 2D subimage / texture slice
    see ComputePitch in https://github.com/microsoft/DirectXTex/blob/master/DirectXTex/DirectXTexUtil.cpp
*/
_SOKOL_PRIVATE int _sg_surface_pitch(sg_pixel_format fmt, int width, int height, int row_align) {
    int num_rows = _sg_num_rows(fmt, height);
    return num_rows * _sg_row_pitch(fmt, width, row_align);
}

/* capability table pixel format helper functions */
_SOKOL_PRIVATE void _sg_pixelformat_all(sg_pixelformat_info* pfi) {
    pfi->sample = true;
    pfi->filter = true;
    pfi->blend = true;
    pfi->render = true;
    pfi->msaa = true;
}

_SOKOL_PRIVATE void _sg_pixelformat_s(sg_pixelformat_info* pfi) {
    pfi->sample = true;
}

_SOKOL_PRIVATE void _sg_pixelformat_sf(sg_pixelformat_info* pfi) {
    pfi->sample = true;
    pfi->filter = true;
}

_SOKOL_PRIVATE void _sg_pixelformat_sr(sg_pixelformat_info* pfi) {
    pfi->sample = true;
    pfi->render = true;
}

_SOKOL_PRIVATE void _sg_pixelformat_srmd(sg_pixelformat_info* pfi) {
    pfi->sample = true;
    pfi->render = true;
    pfi->msaa = true;
    pfi->depth = true;
}

_SOKOL_PRIVATE void _sg_pixelformat_srm(sg_pixelformat_info* pfi) {
    pfi->sample = true;
    pfi->render = true;
    pfi->msaa = true;
}

_SOKOL_PRIVATE void _sg_pixelformat_sfrm(sg_pixelformat_info* pfi) {
    pfi->sample = true;
    pfi->filter = true;
    pfi->render = true;
    pfi->msaa = true;
}
_SOKOL_PRIVATE void _sg_pixelformat_sbrm(sg_pixelformat_info* pfi) {
    pfi->sample = true;
    pfi->blend = true;
    pfi->render = true;
    pfi->msaa = true;
}

_SOKOL_PRIVATE void _sg_pixelformat_sbr(sg_pixelformat_info* pfi) {
    pfi->sample = true;
    pfi->blend = true;
    pfi->render = true;
}

_SOKOL_PRIVATE void _sg_pixelformat_sfbr(sg_pixelformat_info* pfi) {
    pfi->sample = true;
    pfi->filter = true;
    pfi->blend = true;
    pfi->render = true;
}

/* resolve pass action defaults into a new pass action struct */
_SOKOL_PRIVATE void _sg_resolve_default_pass_action(const sg_pass_action* from, sg_pass_action* to) {
    SOKOL_ASSERT(from && to);
    *to = *from;
    for (int i = 0; i < SG_MAX_COLOR_ATTACHMENTS; i++) {
        if (to->colors[i].action  == _SG_ACTION_DEFAULT) {
            to->colors[i].action = SG_ACTION_CLEAR;
            to->colors[i].value.r = SG_DEFAULT_CLEAR_RED;
            to->colors[i].value.g = SG_DEFAULT_CLEAR_GREEN;
            to->colors[i].value.b = SG_DEFAULT_CLEAR_BLUE;
            to->colors[i].value.a = SG_DEFAULT_CLEAR_ALPHA;
        }
    }
    if (to->depth.action == _SG_ACTION_DEFAULT) {
        to->depth.action = SG_ACTION_CLEAR;
        to->depth.value = SG_DEFAULT_CLEAR_DEPTH;
    }
    if (to->stencil.action == _SG_ACTION_DEFAULT) {
        to->stencil.action = SG_ACTION_CLEAR;
        to->stencil.value = SG_DEFAULT_CLEAR_STENCIL;
    }
}

/*== DUMMY BACKEND IMPL ======================================================*/
#if defined(SOKOL_DUMMY_BACKEND)

_SOKOL_PRIVATE void _sg_dummy_setup_backend(const sg_desc* desc) {
    SOKOL_ASSERT(desc);
    _SOKOL_UNUSED(desc);
    _sg.backend = SG_BACKEND_DUMMY;
    for (int i = SG_PIXELFORMAT_R8; i < SG_PIXELFORMAT_BC1_RGBA; i++) {
        _sg.formats[i].sample = true;
        _sg.formats[i].filter = true;
        _sg.formats[i].render = true;
        _sg.formats[i].blend = true;
        _sg.formats[i].msaa = true;
    }
    _sg.formats[SG_PIXELFORMAT_DEPTH].depth = true;
    _sg.formats[SG_PIXELFORMAT_DEPTH_STENCIL].depth = true;
}

_SOKOL_PRIVATE void _sg_dummy_discard_backend(void) {
    /* empty */
}

_SOKOL_PRIVATE void _sg_dummy_reset_state_cache(void) {
    /* empty*/
}

_SOKOL_PRIVATE sg_resource_state _sg_dummy_create_context(_sg_context_t* ctx) {
    SOKOL_ASSERT(ctx);
    _SOKOL_UNUSED(ctx);
    return SG_RESOURCESTATE_VALID;
}

_SOKOL_PRIVATE void _sg_dummy_discard_context(_sg_context_t* ctx) {
    SOKOL_ASSERT(ctx);
    _SOKOL_UNUSED(ctx);
}

_SOKOL_PRIVATE void _sg_dummy_activate_context(_sg_context_t* ctx) {
    SOKOL_ASSERT(ctx);
    _SOKOL_UNUSED(ctx);
}

_SOKOL_PRIVATE sg_resource_state _sg_dummy_create_buffer(_sg_buffer_t* buf, const sg_buffer_desc* desc) {
    SOKOL_ASSERT(buf && desc);
    _sg_buffer_common_init(&buf->cmn, desc);
    return SG_RESOURCESTATE_VALID;
}

_SOKOL_PRIVATE void _sg_dummy_discard_buffer(_sg_buffer_t* buf) {
    SOKOL_ASSERT(buf);
    _SOKOL_UNUSED(buf);
}

_SOKOL_PRIVATE sg_resource_state _sg_dummy_create_image(_sg_image_t* img, const sg_image_desc* desc) {
    SOKOL_ASSERT(img && desc);
    _sg_image_common_init(&img->cmn, desc);
    return SG_RESOURCESTATE_VALID;
}

_SOKOL_PRIVATE void _sg_dummy_discard_image(_sg_image_t* img) {
    SOKOL_ASSERT(img);
    _SOKOL_UNUSED(img);
}

_SOKOL_PRIVATE sg_resource_state _sg_dummy_create_shader(_sg_shader_t* shd, const sg_shader_desc* desc) {
    SOKOL_ASSERT(shd && desc);
    _sg_shader_common_init(&shd->cmn, desc);
    return SG_RESOURCESTATE_VALID;
}

_SOKOL_PRIVATE void _sg_dummy_discard_shader(_sg_shader_t* shd) {
    SOKOL_ASSERT(shd);
    _SOKOL_UNUSED(shd);
}

_SOKOL_PRIVATE sg_resource_state _sg_dummy_create_pipeline(_sg_pipeline_t* pip, _sg_shader_t* shd, const sg_pipeline_desc* desc) {
    SOKOL_ASSERT(pip && desc);
    pip->shader = shd;
    _sg_pipeline_common_init(&pip->cmn, desc);
    for (int attr_index = 0; attr_index < SG_MAX_VERTEX_ATTRIBUTES; attr_index++) {
        const sg_vertex_attr_desc* a_desc = &desc->layout.attrs[attr_index];
        if (a_desc->format == SG_VERTEXFORMAT_INVALID) {
            break;
        }
        SOKOL_ASSERT(a_desc->buffer_index < SG_MAX_SHADERSTAGE_BUFFERS);
        pip->cmn.vertex_layout_valid[a_desc->buffer_index] = true;
    }
    return SG_RESOURCESTATE_VALID;
}

_SOKOL_PRIVATE void _sg_dummy_discard_pipeline(_sg_pipeline_t* pip) {
    SOKOL_ASSERT(pip);
    _SOKOL_UNUSED(pip);
}

_SOKOL_PRIVATE sg_resource_state _sg_dummy_create_pass(_sg_pass_t* pass, _sg_image_t** att_images, const sg_pass_desc* desc) {
    SOKOL_ASSERT(pass && desc);
    SOKOL_ASSERT(att_images && att_images[0]);

    _sg_pass_common_init(&pass->cmn, desc);

    const sg_pass_attachment_desc* att_desc;
    for (int i = 0; i < pass->cmn.num_color_atts; i++) {
        att_desc = &desc->color_attachments[i];
        SOKOL_ASSERT(att_desc->image.id != SG_INVALID_ID);
        SOKOL_ASSERT(0 == pass->dmy.color_atts[i].image);
        SOKOL_ASSERT(att_images[i] && (att_images[i]->slot.id == att_desc->image.id));
        SOKOL_ASSERT(_sg_is_valid_rendertarget_color_format(att_images[i]->cmn.pixel_format));
        pass->dmy.color_atts[i].image = att_images[i];
    }

    SOKOL_ASSERT(0 == pass->dmy.ds_att.image);
    att_desc = &desc->depth_stencil_attachment;
    if (att_desc->image.id != SG_INVALID_ID) {
        const int ds_img_index = SG_MAX_COLOR_ATTACHMENTS;
        SOKOL_ASSERT(att_images[ds_img_index] && (att_images[ds_img_index]->slot.id == att_desc->image.id));
        SOKOL_ASSERT(_sg_is_valid_rendertarget_depth_format(att_images[ds_img_index]->cmn.pixel_format));
        pass->dmy.ds_att.image = att_images[ds_img_index];
    }
    return SG_RESOURCESTATE_VALID;
}

_SOKOL_PRIVATE void _sg_dummy_discard_pass(_sg_pass_t* pass) {
    SOKOL_ASSERT(pass);
    _SOKOL_UNUSED(pass);
}

_SOKOL_PRIVATE _sg_image_t* _sg_dummy_pass_color_image(const _sg_pass_t* pass, int index) {
    SOKOL_ASSERT(pass && (index >= 0) && (index < SG_MAX_COLOR_ATTACHMENTS));
    /* NOTE: may return null */
    return pass->dmy.color_atts[index].image;
}

_SOKOL_PRIVATE _sg_image_t* _sg_dummy_pass_ds_image(const _sg_pass_t* pass) {
    /* NOTE: may return null */
    SOKOL_ASSERT(pass);
    return pass->dmy.ds_att.image;
}

_SOKOL_PRIVATE void _sg_dummy_begin_pass(_sg_pass_t* pass, const sg_pass_action* action, int w, int h) {
    SOKOL_ASSERT(action);
    _SOKOL_UNUSED(pass);
    _SOKOL_UNUSED(action);
    _SOKOL_UNUSED(w);
    _SOKOL_UNUSED(h);
}

_SOKOL_PRIVATE void _sg_dummy_end_pass(void) {
    /* empty */
}

_SOKOL_PRIVATE void _sg_dummy_commit(void) {
    /* empty */
}

_SOKOL_PRIVATE void _sg_dummy_apply_viewport(int x, int y, int w, int h, bool origin_top_left) {
    _SOKOL_UNUSED(x);
    _SOKOL_UNUSED(y);
    _SOKOL_UNUSED(w);
    _SOKOL_UNUSED(h);
    _SOKOL_UNUSED(origin_top_left);
}

_SOKOL_PRIVATE void _sg_dummy_apply_scissor_rect(int x, int y, int w, int h, bool origin_top_left) {
    _SOKOL_UNUSED(x);
    _SOKOL_UNUSED(y);
    _SOKOL_UNUSED(w);
    _SOKOL_UNUSED(h);
    _SOKOL_UNUSED(origin_top_left);
}

_SOKOL_PRIVATE void _sg_dummy_apply_pipeline(_sg_pipeline_t* pip) {
    SOKOL_ASSERT(pip);
    _SOKOL_UNUSED(pip);
}

_SOKOL_PRIVATE void _sg_dummy_apply_bindings(
    _sg_pipeline_t* pip,
    _sg_buffer_t** vbs, const int* vb_offsets, int num_vbs,
    _sg_buffer_t* ib, int ib_offset,
    _sg_image_t** vs_imgs, int num_vs_imgs,
    _sg_image_t** fs_imgs, int num_fs_imgs)
{
    SOKOL_ASSERT(pip);
    SOKOL_ASSERT(vbs && vb_offsets);
    SOKOL_ASSERT(vs_imgs);
    SOKOL_ASSERT(fs_imgs);
    _SOKOL_UNUSED(pip);
    _SOKOL_UNUSED(vbs); _SOKOL_UNUSED(vb_offsets); _SOKOL_UNUSED(num_vbs);
    _SOKOL_UNUSED(ib); _SOKOL_UNUSED(ib_offset);
    _SOKOL_UNUSED(vs_imgs); _SOKOL_UNUSED(num_vs_imgs);
    _SOKOL_UNUSED(fs_imgs); _SOKOL_UNUSED(num_fs_imgs);
}

_SOKOL_PRIVATE void _sg_dummy_apply_uniforms(sg_shader_stage stage_index, int ub_index, const sg_range* data) {
    _SOKOL_UNUSED(stage_index);
    _SOKOL_UNUSED(ub_index);
    _SOKOL_UNUSED(data);
}

_SOKOL_PRIVATE void _sg_dummy_draw(int base_element, int num_elements, int num_instances) {
    _SOKOL_UNUSED(base_element);
    _SOKOL_UNUSED(num_elements);
    _SOKOL_UNUSED(num_instances);
}

_SOKOL_PRIVATE void _sg_dummy_update_buffer(_sg_buffer_t* buf, const sg_range* data) {
    SOKOL_ASSERT(buf && data && data->ptr && (data->size > 0));
    _SOKOL_UNUSED(data);
    if (++buf->cmn.active_slot >= buf->cmn.num_slots) {
        buf->cmn.active_slot = 0;
    }
}

_SOKOL_PRIVATE int _sg_dummy_append_buffer(_sg_buffer_t* buf, const sg_range* data, bool new_frame) {
    SOKOL_ASSERT(buf && data && data->ptr && (data->size > 0));
    _SOKOL_UNUSED(data);
    if (new_frame) {
        if (++buf->cmn.active_slot >= buf->cmn.num_slots) {
            buf->cmn.active_slot = 0;
        }
    }
    /* NOTE: this is a requirement from WebGPU, but we want identical behaviour across all backend */
    return _sg_roundup((int)data->size, 4);
}

_SOKOL_PRIVATE void _sg_dummy_update_image(_sg_image_t* img, const sg_image_data* data) {
    SOKOL_ASSERT(img && data);
    _SOKOL_UNUSED(data);
    if (++img->cmn.active_slot >= img->cmn.num_slots) {
        img->cmn.active_slot = 0;
    }
}

/*== GL BACKEND ==============================================================*/
#elif defined(_SOKOL_ANY_GL)

/*=== OPTIONAL GL LOADER FOR WIN32 ===========================================*/
#if defined(_SOKOL_USE_WIN32_GL_LOADER)

// X Macro list of GL function names and signatures
#define _SG_GL_FUNCS \
    _SG_XMACRO(glBindVertexArray,                 void, (GLuint array)) \
    _SG_XMACRO(glFramebufferTextureLayer,         void, (GLenum target, GLenum attachment, GLuint texture, GLint level, GLint layer)) \
    _SG_XMACRO(glGenFramebuffers,                 void, (GLsizei n, GLuint * framebuffers)) \
    _SG_XMACRO(glBindFramebuffer,                 void, (GLenum target, GLuint framebuffer)) \
    _SG_XMACRO(glBindRenderbuffer,                void, (GLenum target, GLuint renderbuffer)) \
    _SG_XMACRO(glGetStringi,                      const GLubyte *, (GLenum name, GLuint index)) \
    _SG_XMACRO(glClearBufferfi,                   void, (GLenum buffer, GLint drawbuffer, GLfloat depth, GLint stencil)) \
    _SG_XMACRO(glClearBufferfv,                   void, (GLenum buffer, GLint drawbuffer, const GLfloat * value)) \
    _SG_XMACRO(glClearBufferuiv,                  void, (GLenum buffer, GLint drawbuffer, const GLuint * value)) \
    _SG_XMACRO(glClearBufferiv,                   void, (GLenum buffer, GLint drawbuffer, const GLint * value)) \
    _SG_XMACRO(glDeleteRenderbuffers,             void, (GLsizei n, const GLuint * renderbuffers)) \
    _SG_XMACRO(glUniform1fv,                      void, (GLint location, GLsizei count, const GLfloat * value)) \
    _SG_XMACRO(glUniform2fv,                      void, (GLint location, GLsizei count, const GLfloat * value)) \
    _SG_XMACRO(glUniform3fv,                      void, (GLint location, GLsizei count, const GLfloat * value)) \
    _SG_XMACRO(glUniform4fv,                      void, (GLint location, GLsizei count, const GLfloat * value)) \
    _SG_XMACRO(glUniform1iv,                      void, (GLint location, GLsizei count, const GLint * value)) \
    _SG_XMACRO(glUniform2iv,                      void, (GLint location, GLsizei count, const GLint * value)) \
    _SG_XMACRO(glUniform3iv,                      void, (GLint location, GLsizei count, const GLint * value)) \
    _SG_XMACRO(glUniform4iv,                      void, (GLint location, GLsizei count, const GLint * value)) \
    _SG_XMACRO(glUniformMatrix4fv,                void, (GLint location, GLsizei count, GLboolean transpose, const GLfloat * value)) \
    _SG_XMACRO(glUseProgram,                      void, (GLuint program)) \
    _SG_XMACRO(glShaderSource,                    void, (GLuint shader, GLsizei count, const GLchar *const* string, const GLint * length)) \
    _SG_XMACRO(glLinkProgram,                     void, (GLuint program)) \
    _SG_XMACRO(glGetUniformLocation,              GLint, (GLuint program, const GLchar * name)) \
    _SG_XMACRO(glGetShaderiv,                     void, (GLuint shader, GLenum pname, GLint * params)) \
    _SG_XMACRO(glGetProgramInfoLog,               void, (GLuint program, GLsizei bufSize, GLsizei * length, GLchar * infoLog)) \
    _SG_XMACRO(glGetAttribLocation,               GLint, (GLuint program, const GLchar * name)) \
    _SG_XMACRO(glDisableVertexAttribArray,        void, (GLuint index)) \
    _SG_XMACRO(glDeleteShader,                    void, (GLuint shader)) \
    _SG_XMACRO(glDeleteProgram,                   void, (GLuint program)) \
    _SG_XMACRO(glCompileShader,                   void, (GLuint shader)) \
    _SG_XMACRO(glStencilFuncSeparate,             void, (GLenum face, GLenum func, GLint ref, GLuint mask)) \
    _SG_XMACRO(glStencilOpSeparate,               void, (GLenum face, GLenum sfail, GLenum dpfail, GLenum dppass)) \
    _SG_XMACRO(glRenderbufferStorageMultisample,  void, (GLenum target, GLsizei samples, GLenum internalformat, GLsizei width, GLsizei height)) \
    _SG_XMACRO(glDrawBuffers,                     void, (GLsizei n, const GLenum * bufs)) \
    _SG_XMACRO(glVertexAttribDivisor,             void, (GLuint index, GLuint divisor)) \
    _SG_XMACRO(glBufferSubData,                   void, (GLenum target, GLintptr offset, GLsizeiptr size, const void * data)) \
    _SG_XMACRO(glGenBuffers,                      void, (GLsizei n, GLuint * buffers)) \
    _SG_XMACRO(glCheckFramebufferStatus,          GLenum, (GLenum target)) \
    _SG_XMACRO(glFramebufferRenderbuffer,         void, (GLenum target, GLenum attachment, GLenum renderbuffertarget, GLuint renderbuffer)) \
    _SG_XMACRO(glCompressedTexImage2D,            void, (GLenum target, GLint level, GLenum internalformat, GLsizei width, GLsizei height, GLint border, GLsizei imageSize, const void * data)) \
    _SG_XMACRO(glCompressedTexImage3D,            void, (GLenum target, GLint level, GLenum internalformat, GLsizei width, GLsizei height, GLsizei depth, GLint border, GLsizei imageSize, const void * data)) \
    _SG_XMACRO(glActiveTexture,                   void, (GLenum texture)) \
    _SG_XMACRO(glTexSubImage3D,                   void, (GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLsizei width, GLsizei height, GLsizei depth, GLenum format, GLenum type, const void * pixels)) \
    _SG_XMACRO(glRenderbufferStorage,             void, (GLenum target, GLenum internalformat, GLsizei width, GLsizei height)) \
    _SG_XMACRO(glGenTextures,                     void, (GLsizei n, GLuint * textures)) \
    _SG_XMACRO(glPolygonOffset,                   void, (GLfloat factor, GLfloat units)) \
    _SG_XMACRO(glDrawElements,                    void, (GLenum mode, GLsizei count, GLenum type, const void * indices)) \
    _SG_XMACRO(glDeleteFramebuffers,              void, (GLsizei n, const GLuint * framebuffers)) \
    _SG_XMACRO(glBlendEquationSeparate,           void, (GLenum modeRGB, GLenum modeAlpha)) \
    _SG_XMACRO(glDeleteTextures,                  void, (GLsizei n, const GLuint * textures)) \
    _SG_XMACRO(glGetProgramiv,                    void, (GLuint program, GLenum pname, GLint * params)) \
    _SG_XMACRO(glBindTexture,                     void, (GLenum target, GLuint texture)) \
    _SG_XMACRO(glTexImage3D,                      void, (GLenum target, GLint level, GLint internalformat, GLsizei width, GLsizei height, GLsizei depth, GLint border, GLenum format, GLenum type, const void * pixels)) \
    _SG_XMACRO(glCreateShader,                    GLuint, (GLenum type)) \
    _SG_XMACRO(glTexSubImage2D,                   void, (GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLenum type, const void * pixels)) \
    _SG_XMACRO(glClearDepth,                      void, (GLdouble depth)) \
    _SG_XMACRO(glFramebufferTexture2D,            void, (GLenum target, GLenum attachment, GLenum textarget, GLuint texture, GLint level)) \
    _SG_XMACRO(glCreateProgram,                   GLuint, (void)) \
    _SG_XMACRO(glViewport,                        void, (GLint x, GLint y, GLsizei width, GLsizei height)) \
    _SG_XMACRO(glDeleteBuffers,                   void, (GLsizei n, const GLuint * buffers)) \
    _SG_XMACRO(glDrawArrays,                      void, (GLenum mode, GLint first, GLsizei count)) \
    _SG_XMACRO(glDrawElementsInstanced,           void, (GLenum mode, GLsizei count, GLenum type, const void * indices, GLsizei instancecount)) \
    _SG_XMACRO(glVertexAttribPointer,             void, (GLuint index, GLint size, GLenum type, GLboolean normalized, GLsizei stride, const void * pointer)) \
    _SG_XMACRO(glUniform1i,                       void, (GLint location, GLint v0)) \
    _SG_XMACRO(glDisable,                         void, (GLenum cap)) \
    _SG_XMACRO(glColorMask,                       void, (GLboolean red, GLboolean green, GLboolean blue, GLboolean alpha)) \
    _SG_XMACRO(glColorMaski,                      void, (GLuint buf, GLboolean red, GLboolean green, GLboolean blue, GLboolean alpha)) \
    _SG_XMACRO(glBindBuffer,                      void, (GLenum target, GLuint buffer)) \
    _SG_XMACRO(glDeleteVertexArrays,              void, (GLsizei n, const GLuint * arrays)) \
    _SG_XMACRO(glDepthMask,                       void, (GLboolean flag)) \
    _SG_XMACRO(glDrawArraysInstanced,             void, (GLenum mode, GLint first, GLsizei count, GLsizei instancecount)) \
    _SG_XMACRO(glClearStencil,                    void, (GLint s)) \
    _SG_XMACRO(glScissor,                         void, (GLint x, GLint y, GLsizei width, GLsizei height)) \
    _SG_XMACRO(glGenRenderbuffers,                void, (GLsizei n, GLuint * renderbuffers)) \
    _SG_XMACRO(glBufferData,                      void, (GLenum target, GLsizeiptr size, const void * data, GLenum usage)) \
    _SG_XMACRO(glBlendFuncSeparate,               void, (GLenum sfactorRGB, GLenum dfactorRGB, GLenum sfactorAlpha, GLenum dfactorAlpha)) \
    _SG_XMACRO(glTexParameteri,                   void, (GLenum target, GLenum pname, GLint param)) \
    _SG_XMACRO(glGetIntegerv,                     void, (GLenum pname, GLint * data)) \
    _SG_XMACRO(glEnable,                          void, (GLenum cap)) \
    _SG_XMACRO(glBlitFramebuffer,                 void, (GLint srcX0, GLint srcY0, GLint srcX1, GLint srcY1, GLint dstX0, GLint dstY0, GLint dstX1, GLint dstY1, GLbitfield mask, GLenum filter)) \
    _SG_XMACRO(glStencilMask,                     void, (GLuint mask)) \
    _SG_XMACRO(glAttachShader,                    void, (GLuint program, GLuint shader)) \
    _SG_XMACRO(glGetError,                        GLenum, (void)) \
    _SG_XMACRO(glClearColor,                      void, (GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha)) \
    _SG_XMACRO(glBlendColor,                      void, (GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha)) \
    _SG_XMACRO(glTexParameterf,                   void, (GLenum target, GLenum pname, GLfloat param)) \
    _SG_XMACRO(glTexParameterfv,                  void, (GLenum target, GLenum pname, GLfloat* params)) \
    _SG_XMACRO(glGetShaderInfoLog,                void, (GLuint shader, GLsizei bufSize, GLsizei * length, GLchar * infoLog)) \
    _SG_XMACRO(glDepthFunc,                       void, (GLenum func)) \
    _SG_XMACRO(glStencilOp ,                      void, (GLenum fail, GLenum zfail, GLenum zpass)) \
    _SG_XMACRO(glStencilFunc,                     void, (GLenum func, GLint ref, GLuint mask)) \
    _SG_XMACRO(glEnableVertexAttribArray,         void, (GLuint index)) \
    _SG_XMACRO(glBlendFunc,                       void, (GLenum sfactor, GLenum dfactor)) \
    _SG_XMACRO(glReadBuffer,                      void, (GLenum src)) \
    _SG_XMACRO(glReadPixels,                      void, (GLint x, GLint y, GLsizei width, GLsizei height, GLenum format, GLenum type, void * data)) \
    _SG_XMACRO(glClear,                           void, (GLbitfield mask)) \
    _SG_XMACRO(glTexImage2D,                      void, (GLenum target, GLint level, GLint internalformat, GLsizei width, GLsizei height, GLint border, GLenum format, GLenum type, const void * pixels)) \
    _SG_XMACRO(glGenVertexArrays,                 void, (GLsizei n, GLuint * arrays)) \
    _SG_XMACRO(glFrontFace,                       void, (GLenum mode)) \
    _SG_XMACRO(glCullFace,                        void, (GLenum mode))

// generate GL function pointer typedefs
#define _SG_XMACRO(name, ret, args) typedef ret (GL_APIENTRY* PFN_ ## name) args;
_SG_GL_FUNCS
#undef _SG_XMACRO

// generate GL function pointers
#define _SG_XMACRO(name, ret, args) static PFN_ ## name name;
_SG_GL_FUNCS
#undef _SG_XMACRO

// helper function to lookup GL functions in GL DLL
typedef PROC (WINAPI * _sg_wglGetProcAddress)(LPCSTR);
_SOKOL_PRIVATE void* _sg_gl_getprocaddr(const char* name, _sg_wglGetProcAddress wgl_getprocaddress) {
    void* proc_addr = (void*) wgl_getprocaddress(name);
    if (0 == proc_addr) {
        proc_addr = (void*) GetProcAddress(_sg.gl.opengl32_dll, name);
    }
    SOKOL_ASSERT(proc_addr);
    return proc_addr;
}

// populate GL function pointers
_SOKOL_PRIVATE  void _sg_gl_load_opengl(void) {
    SOKOL_ASSERT(0 == _sg.gl.opengl32_dll);
    _sg.gl.opengl32_dll = LoadLibraryA("opengl32.dll");
    SOKOL_ASSERT(_sg.gl.opengl32_dll);
    _sg_wglGetProcAddress wgl_getprocaddress = (_sg_wglGetProcAddress) GetProcAddress(_sg.gl.opengl32_dll, "wglGetProcAddress");
    SOKOL_ASSERT(wgl_getprocaddress);
    #define _SG_XMACRO(name, ret, args) name = (PFN_ ## name) _sg_gl_getprocaddr(#name, wgl_getprocaddress);
    _SG_GL_FUNCS
    #undef _SG_XMACRO
}

_SOKOL_PRIVATE void _sg_gl_unload_opengl(void) {
    SOKOL_ASSERT(_sg.gl.opengl32_dll);
    FreeLibrary(_sg.gl.opengl32_dll);
    _sg.gl.opengl32_dll = 0;
}
#endif // _SOKOL_USE_WIN32_GL_LOADER

/*-- type translation --------------------------------------------------------*/
_SOKOL_PRIVATE GLenum _sg_gl_buffer_target(sg_buffer_type t) {
    switch (t) {
        case SG_BUFFERTYPE_VERTEXBUFFER:    return GL_ARRAY_BUFFER;
        case SG_BUFFERTYPE_INDEXBUFFER:     return GL_ELEMENT_ARRAY_BUFFER;
        default: SOKOL_UNREACHABLE; return 0;
    }
}

_SOKOL_PRIVATE GLenum _sg_gl_texture_target(sg_image_type t) {
    switch (t) {
        case SG_IMAGETYPE_2D:   return GL_TEXTURE_2D;
        case SG_IMAGETYPE_CUBE: return GL_TEXTURE_CUBE_MAP;
        #if !defined(SOKOL_GLES2)
        case SG_IMAGETYPE_3D:       return GL_TEXTURE_3D;
        case SG_IMAGETYPE_ARRAY:    return GL_TEXTURE_2D_ARRAY;
        #endif
        default: SOKOL_UNREACHABLE; return 0;
    }
}

_SOKOL_PRIVATE GLenum _sg_gl_usage(sg_usage u) {
    switch (u) {
        case SG_USAGE_IMMUTABLE:    return GL_STATIC_DRAW;
        case SG_USAGE_DYNAMIC:      return GL_DYNAMIC_DRAW;
        case SG_USAGE_STREAM:       return GL_STREAM_DRAW;
        default: SOKOL_UNREACHABLE; return 0;
    }
}

_SOKOL_PRIVATE GLenum _sg_gl_shader_stage(sg_shader_stage stage) {
    switch (stage) {
        case SG_SHADERSTAGE_VS:     return GL_VERTEX_SHADER;
        case SG_SHADERSTAGE_FS:     return GL_FRAGMENT_SHADER;
        default: SOKOL_UNREACHABLE; return 0;
    }
}

_SOKOL_PRIVATE GLint _sg_gl_vertexformat_size(sg_vertex_format fmt) {
    switch (fmt) {
        case SG_VERTEXFORMAT_FLOAT:     return 1;
        case SG_VERTEXFORMAT_FLOAT2:    return 2;
        case SG_VERTEXFORMAT_FLOAT3:    return 3;
        case SG_VERTEXFORMAT_FLOAT4:    return 4;
        case SG_VERTEXFORMAT_BYTE4:     return 4;
        case SG_VERTEXFORMAT_BYTE4N:    return 4;
        case SG_VERTEXFORMAT_UBYTE4:    return 4;
        case SG_VERTEXFORMAT_UBYTE4N:   return 4;
        case SG_VERTEXFORMAT_SHORT2:    return 2;
        case SG_VERTEXFORMAT_SHORT2N:   return 2;
        case SG_VERTEXFORMAT_USHORT2N:  return 2;
        case SG_VERTEXFORMAT_SHORT4:    return 4;
        case SG_VERTEXFORMAT_SHORT4N:   return 4;
        case SG_VERTEXFORMAT_USHORT4N:  return 4;
        case SG_VERTEXFORMAT_UINT10_N2: return 4;
        default: SOKOL_UNREACHABLE; return 0;
    }
}

_SOKOL_PRIVATE GLenum _sg_gl_vertexformat_type(sg_vertex_format fmt) {
    switch (fmt) {
        case SG_VERTEXFORMAT_FLOAT:
        case SG_VERTEXFORMAT_FLOAT2:
        case SG_VERTEXFORMAT_FLOAT3:
        case SG_VERTEXFORMAT_FLOAT4:
            return GL_FLOAT;
        case SG_VERTEXFORMAT_BYTE4:
        case SG_VERTEXFORMAT_BYTE4N:
            return GL_BYTE;
        case SG_VERTEXFORMAT_UBYTE4:
        case SG_VERTEXFORMAT_UBYTE4N:
            return GL_UNSIGNED_BYTE;
        case SG_VERTEXFORMAT_SHORT2:
        case SG_VERTEXFORMAT_SHORT2N:
        case SG_VERTEXFORMAT_SHORT4:
        case SG_VERTEXFORMAT_SHORT4N:
            return GL_SHORT;
        case SG_VERTEXFORMAT_USHORT2N:
        case SG_VERTEXFORMAT_USHORT4N:
            return GL_UNSIGNED_SHORT;
        case SG_VERTEXFORMAT_UINT10_N2:
            return GL_UNSIGNED_INT_2_10_10_10_REV;
        default:
            SOKOL_UNREACHABLE; return 0;
    }
}

_SOKOL_PRIVATE GLboolean _sg_gl_vertexformat_normalized(sg_vertex_format fmt) {
    switch (fmt) {
        case SG_VERTEXFORMAT_BYTE4N:
        case SG_VERTEXFORMAT_UBYTE4N:
        case SG_VERTEXFORMAT_SHORT2N:
        case SG_VERTEXFORMAT_USHORT2N:
        case SG_VERTEXFORMAT_SHORT4N:
        case SG_VERTEXFORMAT_USHORT4N:
        case SG_VERTEXFORMAT_UINT10_N2:
            return GL_TRUE;
        default:
            return GL_FALSE;
    }
}

_SOKOL_PRIVATE GLenum _sg_gl_primitive_type(sg_primitive_type t) {
    switch (t) {
        case SG_PRIMITIVETYPE_POINTS:           return GL_POINTS;
        case SG_PRIMITIVETYPE_LINES:            return GL_LINES;
        case SG_PRIMITIVETYPE_LINE_STRIP:       return GL_LINE_STRIP;
        case SG_PRIMITIVETYPE_TRIANGLES:        return GL_TRIANGLES;
        case SG_PRIMITIVETYPE_TRIANGLE_STRIP:   return GL_TRIANGLE_STRIP;
        default: SOKOL_UNREACHABLE; return 0;
    }
}

_SOKOL_PRIVATE GLenum _sg_gl_index_type(sg_index_type t) {
    switch (t) {
        case SG_INDEXTYPE_NONE:     return 0;
        case SG_INDEXTYPE_UINT16:   return GL_UNSIGNED_SHORT;
        case SG_INDEXTYPE_UINT32:   return GL_UNSIGNED_INT;
        default: SOKOL_UNREACHABLE; return 0;
    }
}

_SOKOL_PRIVATE GLenum _sg_gl_compare_func(sg_compare_func cmp) {
    switch (cmp) {
        case SG_COMPAREFUNC_NEVER:          return GL_NEVER;
        case SG_COMPAREFUNC_LESS:           return GL_LESS;
        case SG_COMPAREFUNC_EQUAL:          return GL_EQUAL;
        case SG_COMPAREFUNC_LESS_EQUAL:     return GL_LEQUAL;
        case SG_COMPAREFUNC_GREATER:        return GL_GREATER;
        case SG_COMPAREFUNC_NOT_EQUAL:      return GL_NOTEQUAL;
        case SG_COMPAREFUNC_GREATER_EQUAL:  return GL_GEQUAL;
        case SG_COMPAREFUNC_ALWAYS:         return GL_ALWAYS;
        default: SOKOL_UNREACHABLE; return 0;
    }
}

_SOKOL_PRIVATE GLenum _sg_gl_stencil_op(sg_stencil_op op) {
    switch (op) {
        case SG_STENCILOP_KEEP:         return GL_KEEP;
        case SG_STENCILOP_ZERO:         return GL_ZERO;
        case SG_STENCILOP_REPLACE:      return GL_REPLACE;
        case SG_STENCILOP_INCR_CLAMP:   return GL_INCR;
        case SG_STENCILOP_DECR_CLAMP:   return GL_DECR;
        case SG_STENCILOP_INVERT:       return GL_INVERT;
        case SG_STENCILOP_INCR_WRAP:    return GL_INCR_WRAP;
        case SG_STENCILOP_DECR_WRAP:    return GL_DECR_WRAP;
        default: SOKOL_UNREACHABLE; return 0;
    }
}

_SOKOL_PRIVATE GLenum _sg_gl_blend_factor(sg_blend_factor f) {
    switch (f) {
        case SG_BLENDFACTOR_ZERO:                   return GL_ZERO;
        case SG_BLENDFACTOR_ONE:                    return GL_ONE;
        case SG_BLENDFACTOR_SRC_COLOR:              return GL_SRC_COLOR;
        case SG_BLENDFACTOR_ONE_MINUS_SRC_COLOR:    return GL_ONE_MINUS_SRC_COLOR;
        case SG_BLENDFACTOR_SRC_ALPHA:              return GL_SRC_ALPHA;
        case SG_BLENDFACTOR_ONE_MINUS_SRC_ALPHA:    return GL_ONE_MINUS_SRC_ALPHA;
        case SG_BLENDFACTOR_DST_COLOR:              return GL_DST_COLOR;
        case SG_BLENDFACTOR_ONE_MINUS_DST_COLOR:    return GL_ONE_MINUS_DST_COLOR;
        case SG_BLENDFACTOR_DST_ALPHA:              return GL_DST_ALPHA;
        case SG_BLENDFACTOR_ONE_MINUS_DST_ALPHA:    return GL_ONE_MINUS_DST_ALPHA;
        case SG_BLENDFACTOR_SRC_ALPHA_SATURATED:    return GL_SRC_ALPHA_SATURATE;
        case SG_BLENDFACTOR_BLEND_COLOR:            return GL_CONSTANT_COLOR;
        case SG_BLENDFACTOR_ONE_MINUS_BLEND_COLOR:  return GL_ONE_MINUS_CONSTANT_COLOR;
        case SG_BLENDFACTOR_BLEND_ALPHA:            return GL_CONSTANT_ALPHA;
        case SG_BLENDFACTOR_ONE_MINUS_BLEND_ALPHA:  return GL_ONE_MINUS_CONSTANT_ALPHA;
        default: SOKOL_UNREACHABLE; return 0;
    }
}

_SOKOL_PRIVATE GLenum _sg_gl_blend_op(sg_blend_op op) {
    switch (op) {
        case SG_BLENDOP_ADD:                return GL_FUNC_ADD;
        case SG_BLENDOP_SUBTRACT:           return GL_FUNC_SUBTRACT;
        case SG_BLENDOP_REVERSE_SUBTRACT:   return GL_FUNC_REVERSE_SUBTRACT;
        default: SOKOL_UNREACHABLE; return 0;
    }
}

_SOKOL_PRIVATE GLenum _sg_gl_filter(sg_filter f) {
    switch (f) {
        case SG_FILTER_NEAREST:                 return GL_NEAREST;
        case SG_FILTER_LINEAR:                  return GL_LINEAR;
        case SG_FILTER_NEAREST_MIPMAP_NEAREST:  return GL_NEAREST_MIPMAP_NEAREST;
        case SG_FILTER_NEAREST_MIPMAP_LINEAR:   return GL_NEAREST_MIPMAP_LINEAR;
        case SG_FILTER_LINEAR_MIPMAP_NEAREST:   return GL_LINEAR_MIPMAP_NEAREST;
        case SG_FILTER_LINEAR_MIPMAP_LINEAR:    return GL_LINEAR_MIPMAP_LINEAR;
        default: SOKOL_UNREACHABLE; return 0;
    }
}

_SOKOL_PRIVATE GLenum _sg_gl_wrap(sg_wrap w) {
    switch (w) {
        case SG_WRAP_CLAMP_TO_EDGE:     return GL_CLAMP_TO_EDGE;
        #if defined(SOKOL_GLCORE33)
        case SG_WRAP_CLAMP_TO_BORDER:   return GL_CLAMP_TO_BORDER;
        #else
        case SG_WRAP_CLAMP_TO_BORDER:   return GL_CLAMP_TO_EDGE;
        #endif
        case SG_WRAP_REPEAT:            return GL_REPEAT;
        case SG_WRAP_MIRRORED_REPEAT:   return GL_MIRRORED_REPEAT;
        default: SOKOL_UNREACHABLE; return 0;
    }
}

_SOKOL_PRIVATE GLenum _sg_gl_teximage_type(sg_pixel_format fmt) {
    switch (fmt) {
        case SG_PIXELFORMAT_R8:
        case SG_PIXELFORMAT_R8UI:
        case SG_PIXELFORMAT_RG8:
        case SG_PIXELFORMAT_RG8UI:
        case SG_PIXELFORMAT_RGBA8:
        case SG_PIXELFORMAT_RGBA8UI:
        case SG_PIXELFORMAT_BGRA8:
            return GL_UNSIGNED_BYTE;
        case SG_PIXELFORMAT_R8SN:
        case SG_PIXELFORMAT_R8SI:
        case SG_PIXELFORMAT_RG8SN:
        case SG_PIXELFORMAT_RG8SI:
        case SG_PIXELFORMAT_RGBA8SN:
        case SG_PIXELFORMAT_RGBA8SI:
            return GL_BYTE;
        case SG_PIXELFORMAT_R16:
        case SG_PIXELFORMAT_R16UI:
        case SG_PIXELFORMAT_RG16:
        case SG_PIXELFORMAT_RG16UI:
        case SG_PIXELFORMAT_RGBA16:
        case SG_PIXELFORMAT_RGBA16UI:
            return GL_UNSIGNED_SHORT;
        case SG_PIXELFORMAT_R16SN:
        case SG_PIXELFORMAT_R16SI:
        case SG_PIXELFORMAT_RG16SN:
        case SG_PIXELFORMAT_RG16SI:
        case SG_PIXELFORMAT_RGBA16SN:
        case SG_PIXELFORMAT_RGBA16SI:
            return GL_SHORT;
        case SG_PIXELFORMAT_R16F:
        case SG_PIXELFORMAT_RG16F:
        case SG_PIXELFORMAT_RGBA16F:
            return GL_HALF_FLOAT;
        case SG_PIXELFORMAT_R32UI:
        case SG_PIXELFORMAT_RG32UI:
        case SG_PIXELFORMAT_RGBA32UI:
            return GL_UNSIGNED_INT;
        case SG_PIXELFORMAT_R32SI:
        case SG_PIXELFORMAT_RG32SI:
        case SG_PIXELFORMAT_RGBA32SI:
            return GL_INT;
        case SG_PIXELFORMAT_R32F:
        case SG_PIXELFORMAT_RG32F:
        case SG_PIXELFORMAT_RGBA32F:
            return GL_FLOAT;
        #if !defined(SOKOL_GLES2)
        case SG_PIXELFORMAT_RGB10A2:
            return GL_UNSIGNED_INT_2_10_10_10_REV;
        case SG_PIXELFORMAT_RG11B10F:
            return GL_UNSIGNED_INT_10F_11F_11F_REV;
        case SG_PIXELFORMAT_RGB9E5:
            return GL_UNSIGNED_INT_5_9_9_9_REV;
        #endif
        case SG_PIXELFORMAT_DEPTH:
            return GL_UNSIGNED_SHORT;
        case SG_PIXELFORMAT_DEPTH_STENCIL:
            return GL_UNSIGNED_INT_24_8;
        default:
            SOKOL_UNREACHABLE; return 0;
    }
}

_SOKOL_PRIVATE GLenum _sg_gl_teximage_format(sg_pixel_format fmt) {
    switch (fmt) {
        case SG_PIXELFORMAT_R8:
        case SG_PIXELFORMAT_R8SN:
        case SG_PIXELFORMAT_R16:
        case SG_PIXELFORMAT_R16SN:
        case SG_PIXELFORMAT_R16F:
        case SG_PIXELFORMAT_R32F:
            #if defined(SOKOL_GLES2)
                return GL_LUMINANCE;
            #else
            if (_sg.gl.gles2) {
                return GL_LUMINANCE;
            }
            else {
                return GL_RED;
            }
            #endif
        #if !defined(SOKOL_GLES2)
            case SG_PIXELFORMAT_R8UI:
            case SG_PIXELFORMAT_R8SI:
            case SG_PIXELFORMAT_R16UI:
            case SG_PIXELFORMAT_R16SI:
            case SG_PIXELFORMAT_R32UI:
            case SG_PIXELFORMAT_R32SI:
                return GL_RED_INTEGER;
            case SG_PIXELFORMAT_RG8:
            case SG_PIXELFORMAT_RG8SN:
            case SG_PIXELFORMAT_RG16:
            case SG_PIXELFORMAT_RG16SN:
            case SG_PIXELFORMAT_RG16F:
            case SG_PIXELFORMAT_RG32F:
                return GL_RG;
            case SG_PIXELFORMAT_RG8UI:
            case SG_PIXELFORMAT_RG8SI:
            case SG_PIXELFORMAT_RG16UI:
            case SG_PIXELFORMAT_RG16SI:
            case SG_PIXELFORMAT_RG32UI:
            case SG_PIXELFORMAT_RG32SI:
                return GL_RG_INTEGER;
        #endif
        case SG_PIXELFORMAT_RGBA8:
        case SG_PIXELFORMAT_RGBA8SN:
        case SG_PIXELFORMAT_RGBA16:
        case SG_PIXELFORMAT_RGBA16SN:
        case SG_PIXELFORMAT_RGBA16F:
        case SG_PIXELFORMAT_RGBA32F:
        case SG_PIXELFORMAT_RGB10A2:
            return GL_RGBA;
        #if !defined(SOKOL_GLES2)
            case SG_PIXELFORMAT_RGBA8UI:
            case SG_PIXELFORMAT_RGBA8SI:
            case SG_PIXELFORMAT_RGBA16UI:
            case SG_PIXELFORMAT_RGBA16SI:
            case SG_PIXELFORMAT_RGBA32UI:
            case SG_PIXELFORMAT_RGBA32SI:
                return GL_RGBA_INTEGER;
        #endif
        case SG_PIXELFORMAT_RG11B10F:
        case SG_PIXELFORMAT_RGB9E5:
            return GL_RGB;
        case SG_PIXELFORMAT_DEPTH:
            return GL_DEPTH_COMPONENT;
        case SG_PIXELFORMAT_DEPTH_STENCIL:
            return GL_DEPTH_STENCIL;
        case SG_PIXELFORMAT_BC1_RGBA:
            return GL_COMPRESSED_RGBA_S3TC_DXT1_EXT;
        case SG_PIXELFORMAT_BC2_RGBA:
            return GL_COMPRESSED_RGBA_S3TC_DXT3_EXT;
        case SG_PIXELFORMAT_BC3_RGBA:
            return GL_COMPRESSED_RGBA_S3TC_DXT5_EXT;
        case SG_PIXELFORMAT_BC4_R:
            return GL_COMPRESSED_RED_RGTC1;
        case SG_PIXELFORMAT_BC4_RSN:
            return GL_COMPRESSED_SIGNED_RED_RGTC1;
        case SG_PIXELFORMAT_BC5_RG:
            return GL_COMPRESSED_RED_GREEN_RGTC2;
        case SG_PIXELFORMAT_BC5_RGSN:
            return GL_COMPRESSED_SIGNED_RED_GREEN_RGTC2;
        case SG_PIXELFORMAT_BC6H_RGBF:
            return GL_COMPRESSED_RGB_BPTC_SIGNED_FLOAT_ARB;
        case SG_PIXELFORMAT_BC6H_RGBUF:
            return GL_COMPRESSED_RGB_BPTC_UNSIGNED_FLOAT_ARB;
        case SG_PIXELFORMAT_BC7_RGBA:
            return GL_COMPRESSED_RGBA_BPTC_UNORM_ARB;
        case SG_PIXELFORMAT_PVRTC_RGB_2BPP:
            return GL_COMPRESSED_RGB_PVRTC_2BPPV1_IMG;
        case SG_PIXELFORMAT_PVRTC_RGB_4BPP:
            return GL_COMPRESSED_RGB_PVRTC_4BPPV1_IMG;
        case SG_PIXELFORMAT_PVRTC_RGBA_2BPP:
            return GL_COMPRESSED_RGBA_PVRTC_2BPPV1_IMG;
        case SG_PIXELFORMAT_PVRTC_RGBA_4BPP:
            return GL_COMPRESSED_RGBA_PVRTC_4BPPV1_IMG;
        case SG_PIXELFORMAT_ETC2_RGB8:
            return GL_COMPRESSED_RGB8_ETC2;
        case SG_PIXELFORMAT_ETC2_RGB8A1:
            return GL_COMPRESSED_RGB8_PUNCHTHROUGH_ALPHA1_ETC2;
        case SG_PIXELFORMAT_ETC2_RGBA8:
            return GL_COMPRESSED_RGBA8_ETC2_EAC;
        case SG_PIXELFORMAT_ETC2_RG11:
            return GL_COMPRESSED_RG11_EAC;
        case SG_PIXELFORMAT_ETC2_RG11SN:
            return GL_COMPRESSED_SIGNED_RG11_EAC;
        default:
            SOKOL_UNREACHABLE; return 0;
    }
}

_SOKOL_PRIVATE GLenum _sg_gl_teximage_internal_format(sg_pixel_format fmt) {
    #if defined(SOKOL_GLES2)
    return _sg_gl_teximage_format(fmt);
    #else
    if (_sg.gl.gles2) {
        return _sg_gl_teximage_format(fmt);
    }
    else {
        switch (fmt) {
            case SG_PIXELFORMAT_R8:         return GL_R8;
            case SG_PIXELFORMAT_R8SN:       return GL_R8_SNORM;
            case SG_PIXELFORMAT_R8UI:       return GL_R8UI;
            case SG_PIXELFORMAT_R8SI:       return GL_R8I;
            #if !defined(SOKOL_GLES3)
                case SG_PIXELFORMAT_R16:        return GL_R16;
                case SG_PIXELFORMAT_R16SN:      return GL_R16_SNORM;
            #endif
            case SG_PIXELFORMAT_R16UI:      return GL_R16UI;
            case SG_PIXELFORMAT_R16SI:      return GL_R16I;
            case SG_PIXELFORMAT_R16F:       return GL_R16F;
            case SG_PIXELFORMAT_RG8:        return GL_RG8;
            case SG_PIXELFORMAT_RG8SN:      return GL_RG8_SNORM;
            case SG_PIXELFORMAT_RG8UI:      return GL_RG8UI;
            case SG_PIXELFORMAT_RG8SI:      return GL_RG8I;
            case SG_PIXELFORMAT_R32UI:      return GL_R32UI;
            case SG_PIXELFORMAT_R32SI:      return GL_R32I;
            case SG_PIXELFORMAT_R32F:       return GL_R32F;
            #if !defined(SOKOL_GLES3)
                case SG_PIXELFORMAT_RG16:       return GL_RG16;
                case SG_PIXELFORMAT_RG16SN:     return GL_RG16_SNORM;
            #endif
            case SG_PIXELFORMAT_RG16UI:     return GL_RG16UI;
            case SG_PIXELFORMAT_RG16SI:     return GL_RG16I;
            case SG_PIXELFORMAT_RG16F:      return GL_RG16F;
            case SG_PIXELFORMAT_RGBA8:      return GL_RGBA8;
            case SG_PIXELFORMAT_RGBA8SN:    return GL_RGBA8_SNORM;
            case SG_PIXELFORMAT_RGBA8UI:    return GL_RGBA8UI;
            case SG_PIXELFORMAT_RGBA8SI:    return GL_RGBA8I;
            case SG_PIXELFORMAT_RGB10A2:    return GL_RGB10_A2;
            case SG_PIXELFORMAT_RG11B10F:   return GL_R11F_G11F_B10F;
            case SG_PIXELFORMAT_RGB9E5:     return GL_RGB9_E5;
            case SG_PIXELFORMAT_RG32UI:     return GL_RG32UI;
            case SG_PIXELFORMAT_RG32SI:     return GL_RG32I;
            case SG_PIXELFORMAT_RG32F:      return GL_RG32F;
            #if !defined(SOKOL_GLES3)
                case SG_PIXELFORMAT_RGBA16:     return GL_RGBA16;
                case SG_PIXELFORMAT_RGBA16SN:   return GL_RGBA16_SNORM;
            #endif
            case SG_PIXELFORMAT_RGBA16UI:   return GL_RGBA16UI;
            case SG_PIXELFORMAT_RGBA16SI:   return GL_RGBA16I;
            case SG_PIXELFORMAT_RGBA16F:    return GL_RGBA16F;
            case SG_PIXELFORMAT_RGBA32UI:   return GL_RGBA32UI;
            case SG_PIXELFORMAT_RGBA32SI:   return GL_RGBA32I;
            case SG_PIXELFORMAT_RGBA32F:    return GL_RGBA32F;
            case SG_PIXELFORMAT_DEPTH:      return GL_DEPTH_COMPONENT16;
            case SG_PIXELFORMAT_DEPTH_STENCIL:      return GL_DEPTH24_STENCIL8;
            case SG_PIXELFORMAT_BC1_RGBA:           return GL_COMPRESSED_RGBA_S3TC_DXT1_EXT;
            case SG_PIXELFORMAT_BC2_RGBA:           return GL_COMPRESSED_RGBA_S3TC_DXT3_EXT;
            case SG_PIXELFORMAT_BC3_RGBA:           return GL_COMPRESSED_RGBA_S3TC_DXT5_EXT;
            case SG_PIXELFORMAT_BC4_R:              return GL_COMPRESSED_RED_RGTC1;
            case SG_PIXELFORMAT_BC4_RSN:            return GL_COMPRESSED_SIGNED_RED_RGTC1;
            case SG_PIXELFORMAT_BC5_RG:             return GL_COMPRESSED_RED_GREEN_RGTC2;
            case SG_PIXELFORMAT_BC5_RGSN:           return GL_COMPRESSED_SIGNED_RED_GREEN_RGTC2;
            case SG_PIXELFORMAT_BC6H_RGBF:          return GL_COMPRESSED_RGB_BPTC_SIGNED_FLOAT_ARB;
            case SG_PIXELFORMAT_BC6H_RGBUF:         return GL_COMPRESSED_RGB_BPTC_UNSIGNED_FLOAT_ARB;
            case SG_PIXELFORMAT_BC7_RGBA:           return GL_COMPRESSED_RGBA_BPTC_UNORM_ARB;
            case SG_PIXELFORMAT_PVRTC_RGB_2BPP:     return GL_COMPRESSED_RGB_PVRTC_2BPPV1_IMG;
            case SG_PIXELFORMAT_PVRTC_RGB_4BPP:     return GL_COMPRESSED_RGB_PVRTC_4BPPV1_IMG;
            case SG_PIXELFORMAT_PVRTC_RGBA_2BPP:    return GL_COMPRESSED_RGBA_PVRTC_2BPPV1_IMG;
            case SG_PIXELFORMAT_PVRTC_RGBA_4BPP:    return GL_COMPRESSED_RGBA_PVRTC_4BPPV1_IMG;
            case SG_PIXELFORMAT_ETC2_RGB8:          return GL_COMPRESSED_RGB8_ETC2;
            case SG_PIXELFORMAT_ETC2_RGB8A1:        return GL_COMPRESSED_RGB8_PUNCHTHROUGH_ALPHA1_ETC2;
            case SG_PIXELFORMAT_ETC2_RGBA8:         return GL_COMPRESSED_RGBA8_ETC2_EAC;
            case SG_PIXELFORMAT_ETC2_RG11:          return GL_COMPRESSED_RG11_EAC;
            case SG_PIXELFORMAT_ETC2_RG11SN:        return GL_COMPRESSED_SIGNED_RG11_EAC;
            default: SOKOL_UNREACHABLE; return 0;
        }
    }
    #endif
}

_SOKOL_PRIVATE GLenum _sg_gl_cubeface_target(int face_index) {
    switch (face_index) {
        case 0: return GL_TEXTURE_CUBE_MAP_POSITIVE_X;
        case 1: return GL_TEXTURE_CUBE_MAP_NEGATIVE_X;
        case 2: return GL_TEXTURE_CUBE_MAP_POSITIVE_Y;
        case 3: return GL_TEXTURE_CUBE_MAP_NEGATIVE_Y;
        case 4: return GL_TEXTURE_CUBE_MAP_POSITIVE_Z;
        case 5: return GL_TEXTURE_CUBE_MAP_NEGATIVE_Z;
        default: SOKOL_UNREACHABLE; return 0;
    }
}

_SOKOL_PRIVATE GLenum _sg_gl_depth_attachment_format(sg_pixel_format fmt) {
    switch (fmt) {
        case SG_PIXELFORMAT_DEPTH:          return GL_DEPTH_COMPONENT16;
        case SG_PIXELFORMAT_DEPTH_STENCIL:  return GL_DEPTH24_STENCIL8;
        default: SOKOL_UNREACHABLE; return 0;
    }
}

/* see: https://www.khronos.org/registry/OpenGL-Refpages/es3.0/html/glTexImage2D.xhtml */
_SOKOL_PRIVATE void _sg_gl_init_pixelformats(bool has_bgra) {
    #if !defined(SOKOL_GLES2)
    if (!_sg.gl.gles2) {
        _sg_pixelformat_all(&_sg.formats[SG_PIXELFORMAT_R8]);
    }
    else {
        _sg_pixelformat_sf(&_sg.formats[SG_PIXELFORMAT_R8]);
    }
    #else
    _sg_pixelformat_sf(&_sg.formats[SG_PIXELFORMAT_R8]);
    #endif
    #if !defined(SOKOL_GLES2)
    if (!_sg.gl.gles2) {
        _sg_pixelformat_sf(&_sg.formats[SG_PIXELFORMAT_R8SN]);
        _sg_pixelformat_srm(&_sg.formats[SG_PIXELFORMAT_R8UI]);
        _sg_pixelformat_srm(&_sg.formats[SG_PIXELFORMAT_R8SI]);
        #if !defined(SOKOL_GLES3)
            _sg_pixelformat_all(&_sg.formats[SG_PIXELFORMAT_R16]);
            _sg_pixelformat_all(&_sg.formats[SG_PIXELFORMAT_R16SN]);
        #endif
        _sg_pixelformat_srm(&_sg.formats[SG_PIXELFORMAT_R16UI]);
        _sg_pixelformat_srm(&_sg.formats[SG_PIXELFORMAT_R16SI]);
        _sg_pixelformat_all(&_sg.formats[SG_PIXELFORMAT_RG8]);
        _sg_pixelformat_sf(&_sg.formats[SG_PIXELFORMAT_RG8SN]);
        _sg_pixelformat_srm(&_sg.formats[SG_PIXELFORMAT_RG8UI]);
        _sg_pixelformat_srm(&_sg.formats[SG_PIXELFORMAT_RG8SI]);
        _sg_pixelformat_sr(&_sg.formats[SG_PIXELFORMAT_R32UI]);
        _sg_pixelformat_sr(&_sg.formats[SG_PIXELFORMAT_R32SI]);
        #if !defined(SOKOL_GLES3)
            _sg_pixelformat_all(&_sg.formats[SG_PIXELFORMAT_RG16]);
            _sg_pixelformat_all(&_sg.formats[SG_PIXELFORMAT_RG16SN]);
        #endif
        _sg_pixelformat_srm(&_sg.formats[SG_PIXELFORMAT_RG16UI]);
        _sg_pixelformat_srm(&_sg.formats[SG_PIXELFORMAT_RG16SI]);
    }
    #endif
    _sg_pixelformat_all(&_sg.formats[SG_PIXELFORMAT_RGBA8]);
    #if !defined(SOKOL_GLES2)
    if (!_sg.gl.gles2) {
        _sg_pixelformat_sf(&_sg.formats[SG_PIXELFORMAT_RGBA8SN]);
        _sg_pixelformat_srm(&_sg.formats[SG_PIXELFORMAT_RGBA8UI]);
        _sg_pixelformat_srm(&_sg.formats[SG_PIXELFORMAT_RGBA8SI]);
    }
    #endif
    if (has_bgra) {
        _sg_pixelformat_all(&_sg.formats[SG_PIXELFORMAT_BGRA8]);
    }
    #if !defined(SOKOL_GLES2)
    if (!_sg.gl.gles2) {
        _sg_pixelformat_all(&_sg.formats[SG_PIXELFORMAT_RGB10A2]);
        _sg_pixelformat_sf(&_sg.formats[SG_PIXELFORMAT_RG11B10F]);
        _sg_pixelformat_sf(&_sg.formats[SG_PIXELFORMAT_RGB9E5]);
        _sg_pixelformat_srm(&_sg.formats[SG_PIXELFORMAT_RG32UI]);
        _sg_pixelformat_srm(&_sg.formats[SG_PIXELFORMAT_RG32SI]);
        #if !defined(SOKOL_GLES3)
            _sg_pixelformat_all(&_sg.formats[SG_PIXELFORMAT_RGBA16]);
            _sg_pixelformat_all(&_sg.formats[SG_PIXELFORMAT_RGBA16SN]);
        #endif
        _sg_pixelformat_srm(&_sg.formats[SG_PIXELFORMAT_RGBA16UI]);
        _sg_pixelformat_srm(&_sg.formats[SG_PIXELFORMAT_RGBA16SI]);
        _sg_pixelformat_srm(&_sg.formats[SG_PIXELFORMAT_RGBA32UI]);
        _sg_pixelformat_srm(&_sg.formats[SG_PIXELFORMAT_RGBA32SI]);
    }
    #endif
    // FIXME: WEBGL_depth_texture extension?
    _sg_pixelformat_srmd(&_sg.formats[SG_PIXELFORMAT_DEPTH]);
    _sg_pixelformat_srmd(&_sg.formats[SG_PIXELFORMAT_DEPTH_STENCIL]);
}

/* FIXME: OES_half_float_blend */
_SOKOL_PRIVATE void _sg_gl_init_pixelformats_half_float(bool has_colorbuffer_half_float, bool has_texture_half_float_linear) {
    #if !defined(SOKOL_GLES2)
    if (!_sg.gl.gles2) {
        if (has_texture_half_float_linear) {
            if (has_colorbuffer_half_float) {
                _sg_pixelformat_all(&_sg.formats[SG_PIXELFORMAT_R16F]);
                _sg_pixelformat_all(&_sg.formats[SG_PIXELFORMAT_RG16F]);
                _sg_pixelformat_all(&_sg.formats[SG_PIXELFORMAT_RGBA16F]);
            }
            else {
                _sg_pixelformat_sf(&_sg.formats[SG_PIXELFORMAT_R16F]);
                _sg_pixelformat_sf(&_sg.formats[SG_PIXELFORMAT_RG16F]);
                _sg_pixelformat_sf(&_sg.formats[SG_PIXELFORMAT_RGBA16F]);
            }
        }
        else {
            if (has_colorbuffer_half_float) {
                _sg_pixelformat_sbrm(&_sg.formats[SG_PIXELFORMAT_R16F]);
                _sg_pixelformat_sbrm(&_sg.formats[SG_PIXELFORMAT_RG16F]);
                _sg_pixelformat_sbrm(&_sg.formats[SG_PIXELFORMAT_RGBA16F]);
            }
            else {
                _sg_pixelformat_s(&_sg.formats[SG_PIXELFORMAT_R16F]);
                _sg_pixelformat_s(&_sg.formats[SG_PIXELFORMAT_RG16F]);
                _sg_pixelformat_s(&_sg.formats[SG_PIXELFORMAT_RGBA16F]);
            }
        }
    }
    else {
    #endif
        /* GLES2 can only render to RGBA, and there's no RG format */
        if (has_texture_half_float_linear) {
            if (has_colorbuffer_half_float) {
                _sg_pixelformat_all(&_sg.formats[SG_PIXELFORMAT_RGBA16F]);
            }
            else {
                _sg_pixelformat_sf(&_sg.formats[SG_PIXELFORMAT_RGBA16F]);
            }
            _sg_pixelformat_sf(&_sg.formats[SG_PIXELFORMAT_R16F]);
        }
        else {
            if (has_colorbuffer_half_float) {
                _sg_pixelformat_sbrm(&_sg.formats[SG_PIXELFORMAT_RGBA16F]);
            }
            else {
                _sg_pixelformat_s(&_sg.formats[SG_PIXELFORMAT_RGBA16F]);
            }
            _sg_pixelformat_s(&_sg.formats[SG_PIXELFORMAT_R16F]);
        }
    #if !defined(SOKOL_GLES2)
    }
    #endif
}

_SOKOL_PRIVATE void _sg_gl_init_pixelformats_float(bool has_colorbuffer_float, bool has_texture_float_linear, bool has_float_blend) {
    #if !defined(SOKOL_GLES2)
    if (!_sg.gl.gles2) {
        if (has_texture_float_linear) {
            if (has_colorbuffer_float) {
                if (has_float_blend) {
                    _sg_pixelformat_all(&_sg.formats[SG_PIXELFORMAT_R32F]);
                    _sg_pixelformat_all(&_sg.formats[SG_PIXELFORMAT_RG32F]);
                    _sg_pixelformat_all(&_sg.formats[SG_PIXELFORMAT_RGBA32F]);
                }
                else {
                    _sg_pixelformat_sfrm(&_sg.formats[SG_PIXELFORMAT_R32F]);
                    _sg_pixelformat_sfrm(&_sg.formats[SG_PIXELFORMAT_RG32F]);
                    _sg_pixelformat_sfrm(&_sg.formats[SG_PIXELFORMAT_RGBA32F]);
                }
            }
            else {
                _sg_pixelformat_sf(&_sg.formats[SG_PIXELFORMAT_R32F]);
                _sg_pixelformat_sf(&_sg.formats[SG_PIXELFORMAT_RG32F]);
                _sg_pixelformat_sf(&_sg.formats[SG_PIXELFORMAT_RGBA32F]);
            }
        }
        else {
            if (has_colorbuffer_float) {
                _sg_pixelformat_sbrm(&_sg.formats[SG_PIXELFORMAT_R32F]);
                _sg_pixelformat_sbrm(&_sg.formats[SG_PIXELFORMAT_RG32F]);
                _sg_pixelformat_sbrm(&_sg.formats[SG_PIXELFORMAT_RGBA32F]);
            }
            else {
                _sg_pixelformat_s(&_sg.formats[SG_PIXELFORMAT_R32F]);
                _sg_pixelformat_s(&_sg.formats[SG_PIXELFORMAT_RG32F]);
                _sg_pixelformat_s(&_sg.formats[SG_PIXELFORMAT_RGBA32F]);
            }
        }
    }
    else {
    #endif
        /* GLES2 can only render to RGBA, and there's no RG format */
        if (has_texture_float_linear) {
            if (has_colorbuffer_float) {
                if (has_float_blend) {
                    _sg_pixelformat_all(&_sg.formats[SG_PIXELFORMAT_RGBA32F]);
                }
                else {
                    _sg_pixelformat_sfrm(&_sg.formats[SG_PIXELFORMAT_RGBA32F]);
                }
            }
            else {
                _sg_pixelformat_sf(&_sg.formats[SG_PIXELFORMAT_RGBA32F]);
            }
            _sg_pixelformat_sf(&_sg.formats[SG_PIXELFORMAT_R32F]);
        }
        else {
            if (has_colorbuffer_float) {
                _sg_pixelformat_sbrm(&_sg.formats[SG_PIXELFORMAT_RGBA32F]);
            }
            else {
                _sg_pixelformat_s(&_sg.formats[SG_PIXELFORMAT_RGBA32F]);
            }
            _sg_pixelformat_s(&_sg.formats[SG_PIXELFORMAT_R32F]);
        }
    #if !defined(SOKOL_GLES2)
    }
    #endif
}

_SOKOL_PRIVATE void _sg_gl_init_pixelformats_s3tc(void) {
    _sg_pixelformat_sf(&_sg.formats[SG_PIXELFORMAT_BC1_RGBA]);
    _sg_pixelformat_sf(&_sg.formats[SG_PIXELFORMAT_BC2_RGBA]);
    _sg_pixelformat_sf(&_sg.formats[SG_PIXELFORMAT_BC3_RGBA]);
}

_SOKOL_PRIVATE void _sg_gl_init_pixelformats_rgtc(void) {
    _sg_pixelformat_sf(&_sg.formats[SG_PIXELFORMAT_BC4_R]);
    _sg_pixelformat_sf(&_sg.formats[SG_PIXELFORMAT_BC4_RSN]);
    _sg_pixelformat_sf(&_sg.formats[SG_PIXELFORMAT_BC5_RG]);
    _sg_pixelformat_sf(&_sg.formats[SG_PIXELFORMAT_BC5_RGSN]);
}

_SOKOL_PRIVATE void _sg_gl_init_pixelformats_bptc(void) {
    _sg_pixelformat_sf(&_sg.formats[SG_PIXELFORMAT_BC6H_RGBF]);
    _sg_pixelformat_sf(&_sg.formats[SG_PIXELFORMAT_BC6H_RGBUF]);
    _sg_pixelformat_sf(&_sg.formats[SG_PIXELFORMAT_BC7_RGBA]);
}

_SOKOL_PRIVATE void _sg_gl_init_pixelformats_pvrtc(void) {
    _sg_pixelformat_sf(&_sg.formats[SG_PIXELFORMAT_PVRTC_RGB_2BPP]);
    _sg_pixelformat_sf(&_sg.formats[SG_PIXELFORMAT_PVRTC_RGB_4BPP]);
    _sg_pixelformat_sf(&_sg.formats[SG_PIXELFORMAT_PVRTC_RGBA_2BPP]);
    _sg_pixelformat_sf(&_sg.formats[SG_PIXELFORMAT_PVRTC_RGBA_4BPP]);
}

_SOKOL_PRIVATE void _sg_gl_init_pixelformats_etc2(void) {
    _sg_pixelformat_sf(&_sg.formats[SG_PIXELFORMAT_ETC2_RGB8]);
    _sg_pixelformat_sf(&_sg.formats[SG_PIXELFORMAT_ETC2_RGB8A1]);
    _sg_pixelformat_sf(&_sg.formats[SG_PIXELFORMAT_ETC2_RGBA8]);
    _sg_pixelformat_sf(&_sg.formats[SG_PIXELFORMAT_ETC2_RG11]);
    _sg_pixelformat_sf(&_sg.formats[SG_PIXELFORMAT_ETC2_RG11SN]);
}

_SOKOL_PRIVATE void _sg_gl_init_limits(void) {
    _SG_GL_CHECK_ERROR();
    GLint gl_int;
    glGetIntegerv(GL_MAX_TEXTURE_SIZE, &gl_int);
    _SG_GL_CHECK_ERROR();
    _sg.limits.max_image_size_2d = gl_int;
    _sg.limits.max_image_size_array = gl_int;
    glGetIntegerv(GL_MAX_CUBE_MAP_TEXTURE_SIZE, &gl_int);
    _SG_GL_CHECK_ERROR();
    _sg.limits.max_image_size_cube = gl_int;
    glGetIntegerv(GL_MAX_VERTEX_ATTRIBS, &gl_int);
    _SG_GL_CHECK_ERROR();
    if (gl_int > SG_MAX_VERTEX_ATTRIBUTES) {
        gl_int = SG_MAX_VERTEX_ATTRIBUTES;
    }
    _sg.limits.max_vertex_attrs = gl_int;
    glGetIntegerv(GL_MAX_VERTEX_UNIFORM_VECTORS, &gl_int);
    _SG_GL_CHECK_ERROR();
    _sg.limits.gl_max_vertex_uniform_vectors = gl_int;
    #if !defined(SOKOL_GLES2)
    if (!_sg.gl.gles2) {
        glGetIntegerv(GL_MAX_3D_TEXTURE_SIZE, &gl_int);
        _SG_GL_CHECK_ERROR();
        _sg.limits.max_image_size_3d = gl_int;
        glGetIntegerv(GL_MAX_ARRAY_TEXTURE_LAYERS, &gl_int);
        _SG_GL_CHECK_ERROR();
        _sg.limits.max_image_array_layers = gl_int;
    }
    #endif
    if (_sg.gl.ext_anisotropic) {
        glGetIntegerv(GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, &gl_int);
        _SG_GL_CHECK_ERROR();
        _sg.gl.max_anisotropy = gl_int;
    }
    else {
        _sg.gl.max_anisotropy = 1;
    }
    glGetIntegerv(GL_MAX_COMBINED_TEXTURE_IMAGE_UNITS, &gl_int);
    _SG_GL_CHECK_ERROR();
    _sg.gl.max_combined_texture_image_units = gl_int;
}

#if defined(SOKOL_GLCORE33)
_SOKOL_PRIVATE void _sg_gl_init_caps_glcore33(void) {
    _sg.backend = SG_BACKEND_GLCORE33;

    _sg.features.origin_top_left = false;
    _sg.features.instancing = true;
    _sg.features.multiple_render_targets = true;
    _sg.features.msaa_render_targets = true;
    _sg.features.imagetype_3d = true;
    _sg.features.imagetype_array = true;
    _sg.features.image_clamp_to_border = true;
    _sg.features.mrt_independent_blend_state = false;
    _sg.features.mrt_independent_write_mask = true;

    /* scan extensions */
    bool has_s3tc = false;  /* BC1..BC3 */
    bool has_rgtc = false;  /* BC4 and BC5 */
    bool has_bptc = false;  /* BC6H and BC7 */
    bool has_pvrtc = false;
    bool has_etc2 = false;
    GLint num_ext = 0;
    glGetIntegerv(GL_NUM_EXTENSIONS, &num_ext);
    for (int i = 0; i < num_ext; i++) {
        const char* ext = (const char*) glGetStringi(GL_EXTENSIONS, (GLuint)i);
        if (ext) {
            if (strstr(ext, "_texture_compression_s3tc")) {
                has_s3tc = true;
            }
            else if (strstr(ext, "_texture_compression_rgtc")) {
                has_rgtc = true;
            }
            else if (strstr(ext, "_texture_compression_bptc")) {
                has_bptc = true;
            }
            else if (strstr(ext, "_texture_compression_pvrtc")) {
                has_pvrtc = true;
            }
            else if (strstr(ext, "_ES3_compatibility")) {
                has_etc2 = true;
            }
            else if (strstr(ext, "_texture_filter_anisotropic")) {
                _sg.gl.ext_anisotropic = true;
            }
        }
    }

    /* limits */
    _sg_gl_init_limits();

    /* pixel formats */
    const bool has_bgra = false;    /* not a bug */
    const bool has_colorbuffer_float = true;
    const bool has_colorbuffer_half_float = true;
    const bool has_texture_float_linear = true; /* FIXME??? */
    const bool has_texture_half_float_linear = true;
    const bool has_float_blend = true;
    _sg_gl_init_pixelformats(has_bgra);
    _sg_gl_init_pixelformats_float(has_colorbuffer_float, has_texture_float_linear, has_float_blend);
    _sg_gl_init_pixelformats_half_float(has_colorbuffer_half_float, has_texture_half_float_linear);
    if (has_s3tc) {
        _sg_gl_init_pixelformats_s3tc();
    }
    if (has_rgtc) {
        _sg_gl_init_pixelformats_rgtc();
    }
    if (has_bptc) {
        _sg_gl_init_pixelformats_bptc();
    }
    if (has_pvrtc) {
        _sg_gl_init_pixelformats_pvrtc();
    }
    if (has_etc2) {
        _sg_gl_init_pixelformats_etc2();
    }
}
#endif

#if defined(SOKOL_GLES3)
_SOKOL_PRIVATE void _sg_gl_init_caps_gles3(void) {
    _sg.backend = SG_BACKEND_GLES3;

    _sg.features.origin_top_left = false;
    _sg.features.instancing = true;
    _sg.features.multiple_render_targets = true;
    _sg.features.msaa_render_targets = true;
    _sg.features.imagetype_3d = true;
    _sg.features.imagetype_array = true;
    _sg.features.image_clamp_to_border = false;
    _sg.features.mrt_independent_blend_state = false;
    _sg.features.mrt_independent_write_mask = false;

    bool has_s3tc = false;  /* BC1..BC3 */
    bool has_rgtc = false;  /* BC4 and BC5 */
    bool has_bptc = false;  /* BC6H and BC7 */
    bool has_pvrtc = false;
    #if defined(__EMSCRIPTEN__)
        bool has_etc2 = false;
    #else
        bool has_etc2 = true;
    #endif
    bool has_colorbuffer_float = false;
    bool has_colorbuffer_half_float = false;
    bool has_texture_float_linear = false;
    bool has_float_blend = false;
    GLint num_ext = 0;
    glGetIntegerv(GL_NUM_EXTENSIONS, &num_ext);
    for (int i = 0; i < num_ext; i++) {
        const char* ext = (const char*) glGetStringi(GL_EXTENSIONS, (GLuint)i);
        if (ext) {
            if (strstr(ext, "_texture_compression_s3tc")) {
                has_s3tc = true;
            }
            else if (strstr(ext, "_compressed_texture_s3tc")) {
                has_s3tc = true;
            }
            else if (strstr(ext, "_texture_compression_rgtc")) {
                has_rgtc = true;
            }
            else if (strstr(ext, "_texture_compression_bptc")) {
                has_bptc = true;
            }
            else if (strstr(ext, "_texture_compression_pvrtc")) {
                has_pvrtc = true;
            }
            else if (strstr(ext, "_compressed_texture_pvrtc")) {
                has_pvrtc = true;
            }
            else if (strstr(ext, "_compressed_texture_etc")) {
                has_etc2 = true;
            }
            else if (strstr(ext, "_color_buffer_float")) {
                has_colorbuffer_float = true;
            }
            else if (strstr(ext, "_color_buffer_half_float")) {
                has_colorbuffer_half_float = true;
            }
            else if (strstr(ext, "_texture_float_linear")) {
                has_texture_float_linear = true;
            }
            else if (strstr(ext, "_float_blend")) {
                has_float_blend = true;
            }
            else if (strstr(ext, "_texture_filter_anisotropic")) {
                _sg.gl.ext_anisotropic = true;
            }
        }
    }

    /* on WebGL2, color_buffer_float also includes 16-bit formats
       see: https://developer.mozilla.org/en-US/docs/Web/API/EXT_color_buffer_float
    */
    #if defined(__EMSCRIPTEN__)
    has_colorbuffer_half_float = has_colorbuffer_float;
    #endif

    /* limits */
    _sg_gl_init_limits();

    /* pixel formats */
    const bool has_texture_half_float_linear = true;
    const bool has_bgra = false;    /* not a bug */
    _sg_gl_init_pixelformats(has_bgra);
    _sg_gl_init_pixelformats_float(has_colorbuffer_float, has_texture_float_linear, has_float_blend);
    _sg_gl_init_pixelformats_half_float(has_colorbuffer_half_float, has_texture_half_float_linear);
    if (has_s3tc) {
        _sg_gl_init_pixelformats_s3tc();
    }
    if (has_rgtc) {
        _sg_gl_init_pixelformats_rgtc();
    }
    if (has_bptc) {
        _sg_gl_init_pixelformats_bptc();
    }
    if (has_pvrtc) {
        _sg_gl_init_pixelformats_pvrtc();
    }
    if (has_etc2) {
        _sg_gl_init_pixelformats_etc2();
    }
}
#endif

#if defined(SOKOL_GLES3) || defined(SOKOL_GLES2)
_SOKOL_PRIVATE void _sg_gl_init_caps_gles2(void) {
    _sg.backend = SG_BACKEND_GLES2;

    bool has_s3tc = false;  /* BC1..BC3 */
    bool has_rgtc = false;  /* BC4 and BC5 */
    bool has_bptc = false;  /* BC6H and BC7 */
    bool has_pvrtc = false;
    bool has_etc2 = false;
    bool has_texture_float = false;
    bool has_texture_float_linear = false;
    bool has_colorbuffer_float = false;
    bool has_float_blend = false;
    bool has_instancing = false;
    const char* ext = (const char*) glGetString(GL_EXTENSIONS);
    if (ext) {
        has_s3tc = strstr(ext, "_texture_compression_s3tc") || strstr(ext, "_compressed_texture_s3tc");
        has_rgtc = strstr(ext, "_texture_compression_rgtc");
        has_bptc = strstr(ext, "_texture_compression_bptc");
        has_pvrtc = strstr(ext, "_texture_compression_pvrtc") || strstr(ext, "_compressed_texture_pvrtc");
        has_etc2 = strstr(ext, "_compressed_texture_etc");
        has_texture_float = strstr(ext, "_texture_float");
        has_texture_float_linear = strstr(ext, "_texture_float_linear");
        has_colorbuffer_float = strstr(ext, "_color_buffer_float");
        has_float_blend = strstr(ext, "_float_blend");
        /* don't bother with half_float support on WebGL1
            has_texture_half_float = strstr(ext, "_texture_half_float");
            has_texture_half_float_linear = strstr(ext, "_texture_half_float_linear");
            has_colorbuffer_half_float = strstr(ext, "_color_buffer_half_float");
        */
        has_instancing = strstr(ext, "_instanced_arrays");
        _sg.gl.ext_anisotropic = strstr(ext, "ext_anisotropic");
    }

    _sg.features.origin_top_left = false;
    #if defined(_SOKOL_GL_INSTANCING_ENABLED)
        _sg.features.instancing = has_instancing;
    #endif
    _sg.features.multiple_render_targets = false;
    _sg.features.msaa_render_targets = false;
    _sg.features.imagetype_3d = false;
    _sg.features.imagetype_array = false;
    _sg.features.image_clamp_to_border = false;
    _sg.features.mrt_independent_blend_state = false;
    _sg.features.mrt_independent_write_mask = false;

    /* limits */
    _sg_gl_init_limits();

    /* pixel formats */
    const bool has_bgra = false;    /* not a bug */
    const bool has_texture_half_float = false;
    const bool has_texture_half_float_linear = false;
    const bool has_colorbuffer_half_float = false;
    _sg_gl_init_pixelformats(has_bgra);
    if (has_texture_float) {
        _sg_gl_init_pixelformats_float(has_colorbuffer_float, has_texture_float_linear, has_float_blend);
    }
    if (has_texture_half_float) {
        _sg_gl_init_pixelformats_half_float(has_colorbuffer_half_float, has_texture_half_float_linear);
    }
    if (has_s3tc) {
        _sg_gl_init_pixelformats_s3tc();
    }
    if (has_rgtc) {
        _sg_gl_init_pixelformats_rgtc();
    }
    if (has_bptc) {
        _sg_gl_init_pixelformats_bptc();
    }
    if (has_pvrtc) {
        _sg_gl_init_pixelformats_pvrtc();
    }
    if (has_etc2) {
        _sg_gl_init_pixelformats_etc2();
    }
    /* GLES2 doesn't allow multi-sampled render targets at all */
    for (int i = 0; i < _SG_PIXELFORMAT_NUM; i++) {
        _sg.formats[i].msaa = false;
    }
}
#endif

/*-- state cache implementation ----------------------------------------------*/
_SOKOL_PRIVATE void _sg_gl_cache_clear_buffer_bindings(bool force) {
    if (force || (_sg.gl.cache.vertex_buffer != 0)) {
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        _sg.gl.cache.vertex_buffer = 0;
    }
    if (force || (_sg.gl.cache.index_buffer != 0)) {
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
        _sg.gl.cache.index_buffer = 0;
    }
}

_SOKOL_PRIVATE void _sg_gl_cache_bind_buffer(GLenum target, GLuint buffer) {
    SOKOL_ASSERT((GL_ARRAY_BUFFER == target) || (GL_ELEMENT_ARRAY_BUFFER == target));
    if (target == GL_ARRAY_BUFFER) {
        if (_sg.gl.cache.vertex_buffer != buffer) {
            _sg.gl.cache.vertex_buffer = buffer;
            glBindBuffer(target, buffer);
        }
    }
    else {
        if (_sg.gl.cache.index_buffer != buffer) {
            _sg.gl.cache.index_buffer = buffer;
            glBindBuffer(target, buffer);
        }
    }
}

_SOKOL_PRIVATE void _sg_gl_cache_store_buffer_binding(GLenum target) {
    if (target == GL_ARRAY_BUFFER) {
        _sg.gl.cache.stored_vertex_buffer = _sg.gl.cache.vertex_buffer;
    }
    else {
        _sg.gl.cache.stored_index_buffer = _sg.gl.cache.index_buffer;
    }
}

_SOKOL_PRIVATE void _sg_gl_cache_restore_buffer_binding(GLenum target) {
    if (target == GL_ARRAY_BUFFER) {
        if (_sg.gl.cache.stored_vertex_buffer != 0) {
            /* we only care restoring valid ids */
            _sg_gl_cache_bind_buffer(target, _sg.gl.cache.stored_vertex_buffer);
            _sg.gl.cache.stored_vertex_buffer = 0;
        }
    }
    else {
        if (_sg.gl.cache.stored_index_buffer != 0) {
            /* we only care restoring valid ids */
            _sg_gl_cache_bind_buffer(target, _sg.gl.cache.stored_index_buffer);
            _sg.gl.cache.stored_index_buffer = 0;
        }
    }
}

/* called when _sg_gl_deinit_buffer() */
_SOKOL_PRIVATE void _sg_gl_cache_invalidate_buffer(GLuint buf) {
    if (buf == _sg.gl.cache.vertex_buffer) {
        _sg.gl.cache.vertex_buffer = 0;
        glBindBuffer(GL_ARRAY_BUFFER, 0);
    }
    if (buf == _sg.gl.cache.index_buffer) {
        _sg.gl.cache.index_buffer = 0;
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    }
    if (buf == _sg.gl.cache.stored_vertex_buffer) {
        _sg.gl.cache.stored_vertex_buffer = 0;
    }
    if (buf == _sg.gl.cache.stored_index_buffer) {
        _sg.gl.cache.stored_index_buffer = 0;
    }
    for (int i = 0; i < SG_MAX_VERTEX_ATTRIBUTES; i++) {
        if (buf == _sg.gl.cache.attrs[i].gl_vbuf) {
            _sg.gl.cache.attrs[i].gl_vbuf = 0;
        }
    }
}

_SOKOL_PRIVATE void _sg_gl_cache_active_texture(GLenum texture) {
    if (_sg.gl.cache.cur_active_texture != texture) {
        _sg.gl.cache.cur_active_texture = texture;
        glActiveTexture(texture);
    }
}

_SOKOL_PRIVATE void _sg_gl_cache_clear_texture_bindings(bool force) {
    for (int i = 0; (i < SG_MAX_SHADERSTAGE_IMAGES) && (i < _sg.gl.max_combined_texture_image_units); i++) {
        if (force || (_sg.gl.cache.textures[i].texture != 0)) {
            GLenum gl_texture_slot = (GLenum) (GL_TEXTURE0 + i);
            glActiveTexture(gl_texture_slot);
            glBindTexture(GL_TEXTURE_2D, 0);
            glBindTexture(GL_TEXTURE_CUBE_MAP, 0);
            #if !defined(SOKOL_GLES2)
            if (!_sg.gl.gles2) {
                glBindTexture(GL_TEXTURE_3D, 0);
                glBindTexture(GL_TEXTURE_2D_ARRAY, 0);
            }
            #endif
            _sg.gl.cache.textures[i].target = 0;
            _sg.gl.cache.textures[i].texture = 0;
            _sg.gl.cache.cur_active_texture = gl_texture_slot;
        }
    }
}

_SOKOL_PRIVATE void _sg_gl_cache_bind_texture(int slot_index, GLenum target, GLuint texture) {
    /* it's valid to call this function with target=0 and/or texture=0
       target=0 will unbind the previous binding, texture=0 will clear
       the new binding
    */
    SOKOL_ASSERT(slot_index < SG_MAX_SHADERSTAGE_IMAGES);
    if (slot_index >= _sg.gl.max_combined_texture_image_units) {
        return;
    }
    _sg_gl_texture_bind_slot* slot = &_sg.gl.cache.textures[slot_index];
    if ((slot->target != target) || (slot->texture != texture)) {
        _sg_gl_cache_active_texture((GLenum)(GL_TEXTURE0 + slot_index));
        /* if the target has changed, clear the previous binding on that target */
        if ((target != slot->target) && (slot->target != 0)) {
            glBindTexture(slot->target, 0);
        }
        /* apply new binding (texture can be 0 to unbind) */
        if (target != 0) {
            glBindTexture(target, texture);
        }
        slot->target = target;
        slot->texture = texture;
    }
}

_SOKOL_PRIVATE void _sg_gl_cache_store_texture_binding(int slot_index) {
    SOKOL_ASSERT(slot_index < SG_MAX_SHADERSTAGE_IMAGES);
    _sg.gl.cache.stored_texture = _sg.gl.cache.textures[slot_index];
}

_SOKOL_PRIVATE void _sg_gl_cache_restore_texture_binding(int slot_index) {
    SOKOL_ASSERT(slot_index < SG_MAX_SHADERSTAGE_IMAGES);
    _sg_gl_texture_bind_slot* slot = &_sg.gl.cache.stored_texture;
    if (slot->texture != 0) {
        /* we only care restoring valid ids */
        SOKOL_ASSERT(slot->target != 0);
        _sg_gl_cache_bind_texture(slot_index, slot->target, slot->texture);
        slot->target = 0;
        slot->texture = 0;
    }
}

/* called from _sg_gl_destroy_texture() */
_SOKOL_PRIVATE void _sg_gl_cache_invalidate_texture(GLuint tex) {
    for (int i = 0; i < SG_MAX_SHADERSTAGE_IMAGES; i++) {
        _sg_gl_texture_bind_slot* slot = &_sg.gl.cache.textures[i];
        if (tex == slot->texture) {
            _sg_gl_cache_active_texture((GLenum)(GL_TEXTURE0 + i));
            glBindTexture(slot->target, 0);
            slot->target = 0;
            slot->texture = 0;
        }
    }
    if (tex == _sg.gl.cache.stored_texture.texture) {
        _sg.gl.cache.stored_texture.target = 0;
        _sg.gl.cache.stored_texture.texture = 0;
    }
}

/* called from _sg_gl_discard_shader() */
_SOKOL_PRIVATE void _sg_gl_cache_invalidate_program(GLuint prog) {
    if (prog == _sg.gl.cache.prog) {
        _sg.gl.cache.prog = 0;
        glUseProgram(0);
    }
}

/* called from _sg_gl_discard_pipeline() */
_SOKOL_PRIVATE void _sg_gl_cache_invalidate_pipeline(_sg_pipeline_t* pip) {
    if (pip == _sg.gl.cache.cur_pipeline) {
        _sg.gl.cache.cur_pipeline = 0;
        _sg.gl.cache.cur_pipeline_id.id = SG_INVALID_ID;
    }
}

_SOKOL_PRIVATE void _sg_gl_reset_state_cache(void) {
    if (_sg.gl.cur_context) {
        _SG_GL_CHECK_ERROR();
        #if !defined(SOKOL_GLES2)
        if (!_sg.gl.gles2) {
            glBindVertexArray(_sg.gl.cur_context->vao);
            _SG_GL_CHECK_ERROR();
        }
        #endif
        _sg_clear(&_sg.gl.cache, sizeof(_sg.gl.cache));
        _sg_gl_cache_clear_buffer_bindings(true);
        _SG_GL_CHECK_ERROR();
        _sg_gl_cache_clear_texture_bindings(true);
        _SG_GL_CHECK_ERROR();
        for (int i = 0; i < _sg.limits.max_vertex_attrs; i++) {
            _sg_gl_attr_t* attr = &_sg.gl.cache.attrs[i].gl_attr;
            attr->vb_index = -1;
            attr->divisor = -1;
            glDisableVertexAttribArray((GLuint)i);
            _SG_GL_CHECK_ERROR();
        }
        _sg.gl.cache.cur_primitive_type = GL_TRIANGLES;

        /* shader program */
        glGetIntegerv(GL_CURRENT_PROGRAM, (GLint*)&_sg.gl.cache.prog);
        _SG_GL_CHECK_ERROR();

        /* depth and stencil state */
        _sg.gl.cache.depth.compare = SG_COMPAREFUNC_ALWAYS;
        _sg.gl.cache.stencil.front.compare = SG_COMPAREFUNC_ALWAYS;
        _sg.gl.cache.stencil.front.fail_op = SG_STENCILOP_KEEP;
        _sg.gl.cache.stencil.front.depth_fail_op = SG_STENCILOP_KEEP;
        _sg.gl.cache.stencil.front.pass_op = SG_STENCILOP_KEEP;
        _sg.gl.cache.stencil.back.compare = SG_COMPAREFUNC_ALWAYS;
        _sg.gl.cache.stencil.back.fail_op = SG_STENCILOP_KEEP;
        _sg.gl.cache.stencil.back.depth_fail_op = SG_STENCILOP_KEEP;
        _sg.gl.cache.stencil.back.pass_op = SG_STENCILOP_KEEP;
        glEnable(GL_DEPTH_TEST);
        glDepthFunc(GL_ALWAYS);
        glDepthMask(GL_FALSE);
        glDisable(GL_STENCIL_TEST);
        glStencilFunc(GL_ALWAYS, 0, 0);
        glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);
        glStencilMask(0);

        /* blend state */
        _sg.gl.cache.blend.src_factor_rgb = SG_BLENDFACTOR_ONE;
        _sg.gl.cache.blend.dst_factor_rgb = SG_BLENDFACTOR_ZERO;
        _sg.gl.cache.blend.op_rgb = SG_BLENDOP_ADD;
        _sg.gl.cache.blend.src_factor_alpha = SG_BLENDFACTOR_ONE;
        _sg.gl.cache.blend.dst_factor_alpha = SG_BLENDFACTOR_ZERO;
        _sg.gl.cache.blend.op_alpha = SG_BLENDOP_ADD;
        glDisable(GL_BLEND);
        glBlendFuncSeparate(GL_ONE, GL_ZERO, GL_ONE, GL_ZERO);
        glBlendEquationSeparate(GL_FUNC_ADD, GL_FUNC_ADD);
        glBlendColor(0.0f, 0.0f, 0.0f, 0.0f);

        /* standalone state */
        for (int i = 0; i < SG_MAX_COLOR_ATTACHMENTS; i++) {
            _sg.gl.cache.color_write_mask[i] = SG_COLORMASK_RGBA;
        }
        _sg.gl.cache.cull_mode = SG_CULLMODE_NONE;
        _sg.gl.cache.face_winding = SG_FACEWINDING_CW;
        _sg.gl.cache.sample_count = 1;
        glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
        glPolygonOffset(0.0f, 0.0f);
        glDisable(GL_POLYGON_OFFSET_FILL);
        glDisable(GL_CULL_FACE);
        glFrontFace(GL_CW);
        glCullFace(GL_BACK);
        glEnable(GL_SCISSOR_TEST);
        glDisable(GL_SAMPLE_ALPHA_TO_COVERAGE);
        glEnable(GL_DITHER);
        glDisable(GL_POLYGON_OFFSET_FILL);
        #if defined(SOKOL_GLCORE33)
            glEnable(GL_MULTISAMPLE);
            glEnable(GL_PROGRAM_POINT_SIZE);
        #endif
    }
}

_SOKOL_PRIVATE void _sg_gl_setup_backend(const sg_desc* desc) {
    /* assumes that _sg.gl is already zero-initialized */
    _sg.gl.valid = true;
    #if defined(SOKOL_GLES2) || defined(SOKOL_GLES3)
    _sg.gl.gles2 = desc->context.gl.force_gles2;
    #else
    _SOKOL_UNUSED(desc);
    _sg.gl.gles2 = false;
    #endif

    #if defined(_SOKOL_USE_WIN32_GL_LOADER)
    _sg_gl_load_opengl();
    #endif

    /* clear initial GL error state */
    #if defined(SOKOL_DEBUG)
        while (glGetError() != GL_NO_ERROR);
    #endif
    #if defined(SOKOL_GLCORE33)
        _sg_gl_init_caps_glcore33();
    #elif defined(SOKOL_GLES3)
        if (_sg.gl.gles2) {
            _sg_gl_init_caps_gles2();
        }
        else {
            _sg_gl_init_caps_gles3();
        }
    #else
        _sg_gl_init_caps_gles2();
    #endif
}

_SOKOL_PRIVATE void _sg_gl_discard_backend(void) {
    SOKOL_ASSERT(_sg.gl.valid);
    _sg.gl.valid = false;
    #if defined(_SOKOL_USE_WIN32_GL_LOADER)
    _sg_gl_unload_opengl();
    #endif
}

_SOKOL_PRIVATE void _sg_gl_activate_context(_sg_context_t* ctx) {
    SOKOL_ASSERT(_sg.gl.valid);
    /* NOTE: ctx can be 0 to unset the current context */
    _sg.gl.cur_context = ctx;
    _sg_gl_reset_state_cache();
}

/*-- GL backend resource creation and destruction ----------------------------*/
_SOKOL_PRIVATE sg_resource_state _sg_gl_create_context(_sg_context_t* ctx) {
    SOKOL_ASSERT(ctx);
    SOKOL_ASSERT(0 == ctx->default_framebuffer);
    _SG_GL_CHECK_ERROR();
    glGetIntegerv(GL_FRAMEBUFFER_BINDING, (GLint*)&ctx->default_framebuffer);
    _SG_GL_CHECK_ERROR();
    #if !defined(SOKOL_GLES2)
    if (!_sg.gl.gles2) {
        SOKOL_ASSERT(0 == ctx->vao);
        glGenVertexArrays(1, &ctx->vao);
        glBindVertexArray(ctx->vao);
        _SG_GL_CHECK_ERROR();
    }
    #endif
    return SG_RESOURCESTATE_VALID;
}

_SOKOL_PRIVATE void _sg_gl_discard_context(_sg_context_t* ctx) {
    SOKOL_ASSERT(ctx);
    #if !defined(SOKOL_GLES2)
    if (!_sg.gl.gles2) {
        if (ctx->vao) {
            glDeleteVertexArrays(1, &ctx->vao);
        }
        _SG_GL_CHECK_ERROR();
    }
    #else
    _SOKOL_UNUSED(ctx);
    #endif
}

_SOKOL_PRIVATE sg_resource_state _sg_gl_create_buffer(_sg_buffer_t* buf, const sg_buffer_desc* desc) {
    SOKOL_ASSERT(buf && desc);
    _SG_GL_CHECK_ERROR();
    _sg_buffer_common_init(&buf->cmn, desc);
    buf->gl.ext_buffers = (0 != desc->gl_buffers[0]);
    GLenum gl_target = _sg_gl_buffer_target(buf->cmn.type);
    GLenum gl_usage  = _sg_gl_usage(buf->cmn.usage);
    for (int slot = 0; slot < buf->cmn.num_slots; slot++) {
        GLuint gl_buf = 0;
        if (buf->gl.ext_buffers) {
            SOKOL_ASSERT(desc->gl_buffers[slot]);
            gl_buf = desc->gl_buffers[slot];
        }
        else {
            glGenBuffers(1, &gl_buf);
            SOKOL_ASSERT(gl_buf);
            _sg_gl_cache_store_buffer_binding(gl_target);
            _sg_gl_cache_bind_buffer(gl_target, gl_buf);
            glBufferData(gl_target, buf->cmn.size, 0, gl_usage);
            if (buf->cmn.usage == SG_USAGE_IMMUTABLE) {
                SOKOL_ASSERT(desc->data.ptr);
                glBufferSubData(gl_target, 0, buf->cmn.size, desc->data.ptr);
            }
            _sg_gl_cache_restore_buffer_binding(gl_target);
        }
        buf->gl.buf[slot] = gl_buf;
    }
    _SG_GL_CHECK_ERROR();
    return SG_RESOURCESTATE_VALID;
}

_SOKOL_PRIVATE void _sg_gl_discard_buffer(_sg_buffer_t* buf) {
    SOKOL_ASSERT(buf);
    _SG_GL_CHECK_ERROR();
    for (int slot = 0; slot < buf->cmn.num_slots; slot++) {
        if (buf->gl.buf[slot]) {
            _sg_gl_cache_invalidate_buffer(buf->gl.buf[slot]);
            if (!buf->gl.ext_buffers) {
                glDeleteBuffers(1, &buf->gl.buf[slot]);
            }
        }
    }
    _SG_GL_CHECK_ERROR();
}

_SOKOL_PRIVATE bool _sg_gl_supported_texture_format(sg_pixel_format fmt) {
    const int fmt_index = (int) fmt;
    SOKOL_ASSERT((fmt_index > SG_PIXELFORMAT_NONE) && (fmt_index < _SG_PIXELFORMAT_NUM));
    return _sg.formats[fmt_index].sample;
}

_SOKOL_PRIVATE sg_resource_state _sg_gl_create_image(_sg_image_t* img, const sg_image_desc* desc) {
    SOKOL_ASSERT(img && desc);
    _SG_GL_CHECK_ERROR();
    _sg_image_common_init(&img->cmn, desc);
    img->gl.ext_textures = (0 != desc->gl_textures[0]);

    /* check if texture format is support */
    if (!_sg_gl_supported_texture_format(img->cmn.pixel_format)) {
        SG_LOG("texture format not supported by GL context\n");
        return SG_RESOURCESTATE_FAILED;
    }
    /* check for optional texture types */
    if ((img->cmn.type == SG_IMAGETYPE_3D) && !_sg.features.imagetype_3d) {
        SG_LOG("3D textures not supported by GL context\n");
        return SG_RESOURCESTATE_FAILED;
    }
    if ((img->cmn.type == SG_IMAGETYPE_ARRAY) && !_sg.features.imagetype_array) {
        SG_LOG("array textures not supported by GL context\n");
        return SG_RESOURCESTATE_FAILED;
    }

    #if !defined(SOKOL_GLES2)
    bool msaa = false;
    if (!_sg.gl.gles2) {
        msaa = (img->cmn.sample_count > 1) && (_sg.features.msaa_render_targets);
    }
    #endif

    if (_sg_is_valid_rendertarget_depth_format(img->cmn.pixel_format)) {
        /* special case depth-stencil-buffer? */
        SOKOL_ASSERT((img->cmn.usage == SG_USAGE_IMMUTABLE) && (img->cmn.num_slots == 1));
        SOKOL_ASSERT(!img->gl.ext_textures);   /* cannot provide external texture for depth images */
        glGenRenderbuffers(1, &img->gl.depth_render_buffer);
        glBindRenderbuffer(GL_RENDERBUFFER, img->gl.depth_render_buffer);
        GLenum gl_depth_format = _sg_gl_depth_attachment_format(img->cmn.pixel_format);
        #if !defined(SOKOL_GLES2)
        if (!_sg.gl.gles2 && msaa) {
            glRenderbufferStorageMultisample(GL_RENDERBUFFER, img->cmn.sample_count, gl_depth_format, img->cmn.width, img->cmn.height);
        }
        else
        #endif
        {
            glRenderbufferStorage(GL_RENDERBUFFER, gl_depth_format, img->cmn.width, img->cmn.height);
        }
    }
    else {
        /* regular color texture */
        img->gl.target = _sg_gl_texture_target(img->cmn.type);
        const GLenum gl_internal_format = _sg_gl_teximage_internal_format(img->cmn.pixel_format);

        /* if this is a MSAA render target, need to create a separate render buffer */
        #if !defined(SOKOL_GLES2)
        if (!_sg.gl.gles2 && img->cmn.render_target && msaa) {
            glGenRenderbuffers(1, &img->gl.msaa_render_buffer);
            glBindRenderbuffer(GL_RENDERBUFFER, img->gl.msaa_render_buffer);
            glRenderbufferStorageMultisample(GL_RENDERBUFFER, img->cmn.sample_count, gl_internal_format, img->cmn.width, img->cmn.height);
        }
        #endif

        if (img->gl.ext_textures) {
            /* inject externally GL textures */
            for (int slot = 0; slot < img->cmn.num_slots; slot++) {
                SOKOL_ASSERT(desc->gl_textures[slot]);
                img->gl.tex[slot] = desc->gl_textures[slot];
            }
            if (desc->gl_texture_target) {
                img->gl.target = (GLenum)desc->gl_texture_target;
            }
        }
        else {
            /* create our own GL texture(s) */
            const GLenum gl_format = _sg_gl_teximage_format(img->cmn.pixel_format);
            const bool is_compressed = _sg_is_compressed_pixel_format(img->cmn.pixel_format);
            for (int slot = 0; slot < img->cmn.num_slots; slot++) {
                glGenTextures(1, &img->gl.tex[slot]);
                SOKOL_ASSERT(img->gl.tex[slot]);
                _sg_gl_cache_store_texture_binding(0);
                _sg_gl_cache_bind_texture(0, img->gl.target, img->gl.tex[slot]);
                GLenum gl_min_filter = _sg_gl_filter(img->cmn.min_filter);
                GLenum gl_mag_filter = _sg_gl_filter(img->cmn.mag_filter);
                glTexParameteri(img->gl.target, GL_TEXTURE_MIN_FILTER, (GLint)gl_min_filter);
                glTexParameteri(img->gl.target, GL_TEXTURE_MAG_FILTER, (GLint)gl_mag_filter);
                if (_sg.gl.ext_anisotropic && (img->cmn.max_anisotropy > 1)) {
                    GLint max_aniso = (GLint) img->cmn.max_anisotropy;
                    if (max_aniso > _sg.gl.max_anisotropy) {
                        max_aniso = _sg.gl.max_anisotropy;
                    }
                    glTexParameteri(img->gl.target, GL_TEXTURE_MAX_ANISOTROPY_EXT, max_aniso);
                }
                if (img->cmn.type == SG_IMAGETYPE_CUBE) {
                    glTexParameteri(img->gl.target, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
                    glTexParameteri(img->gl.target, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
                }
                else {
                    glTexParameteri(img->gl.target, GL_TEXTURE_WRAP_S, (GLint)_sg_gl_wrap(img->cmn.wrap_u));
                    glTexParameteri(img->gl.target, GL_TEXTURE_WRAP_T, (GLint)_sg_gl_wrap(img->cmn.wrap_v));
                    #if !defined(SOKOL_GLES2)
                    if (!_sg.gl.gles2 && (img->cmn.type == SG_IMAGETYPE_3D)) {
                        glTexParameteri(img->gl.target, GL_TEXTURE_WRAP_R, (GLint)_sg_gl_wrap(img->cmn.wrap_w));
                    }
                    #endif
                    #if defined(SOKOL_GLCORE33)
                    float border[4];
                    switch (img->cmn.border_color) {
                        case SG_BORDERCOLOR_TRANSPARENT_BLACK:
                            border[0] = 0.0f; border[1] = 0.0f; border[2] = 0.0f; border[3] = 0.0f;
                            break;
                        case SG_BORDERCOLOR_OPAQUE_WHITE:
                            border[0] = 1.0f; border[1] = 1.0f; border[2] = 1.0f; border[3] = 1.0f;
                            break;
                        default:
                            border[0] = 0.0f; border[1] = 0.0f; border[2] = 0.0f; border[3] = 1.0f;
                            break;
                    }
                    glTexParameterfv(img->gl.target, GL_TEXTURE_BORDER_COLOR, border);
                    #endif
                }
                #if !defined(SOKOL_GLES2)
                if (!_sg.gl.gles2) {
                    /* GL spec has strange defaults for mipmap min/max lod: -1000 to +1000 */
                    const float min_lod = _sg_clamp(desc->min_lod, 0.0f, 1000.0f);
                    const float max_lod = _sg_clamp(desc->max_lod, 0.0f, 1000.0f);
                    glTexParameterf(img->gl.target, GL_TEXTURE_MIN_LOD, min_lod);
                    glTexParameterf(img->gl.target, GL_TEXTURE_MAX_LOD, max_lod);
                }
                #endif
                const int num_faces = img->cmn.type == SG_IMAGETYPE_CUBE ? 6 : 1;
                int data_index = 0;
                for (int face_index = 0; face_index < num_faces; face_index++) {
                    for (int mip_index = 0; mip_index < img->cmn.num_mipmaps; mip_index++, data_index++) {
                        GLenum gl_img_target = img->gl.target;
                        if (SG_IMAGETYPE_CUBE == img->cmn.type) {
                            gl_img_target = _sg_gl_cubeface_target(face_index);
                        }
                        const GLvoid* data_ptr = desc->data.subimage[face_index][mip_index].ptr;
                        int mip_width = img->cmn.width >> mip_index;
                        if (mip_width == 0) {
                            mip_width = 1;
                        }
                        int mip_height = img->cmn.height >> mip_index;
                        if (mip_height == 0) {
                            mip_height = 1;
                        }
                        if ((SG_IMAGETYPE_2D == img->cmn.type) || (SG_IMAGETYPE_CUBE == img->cmn.type)) {
                            if (is_compressed) {
                                const GLsizei data_size = (GLsizei) desc->data.subimage[face_index][mip_index].size;
                                glCompressedTexImage2D(gl_img_target, mip_index, gl_internal_format,
                                    mip_width, mip_height, 0, data_size, data_ptr);
                            }
                            else {
                                const GLenum gl_type = _sg_gl_teximage_type(img->cmn.pixel_format);
                                glTexImage2D(gl_img_target, mip_index, (GLint)gl_internal_format,
                                    mip_width, mip_height, 0, gl_format, gl_type, data_ptr);
                            }
                        }
                        #if !defined(SOKOL_GLES2)
                        else if (!_sg.gl.gles2 && ((SG_IMAGETYPE_3D == img->cmn.type) || (SG_IMAGETYPE_ARRAY == img->cmn.type))) {
                            int mip_depth = img->cmn.num_slices;
                            if (SG_IMAGETYPE_3D == img->cmn.type) {
                                mip_depth >>= mip_index;
                            }
                            if (mip_depth == 0) {
                                mip_depth = 1;
                            }
                            if (is_compressed) {
                                const GLsizei data_size = (GLsizei) desc->data.subimage[face_index][mip_index].size;
                                glCompressedTexImage3D(gl_img_target, mip_index, gl_internal_format,
                                    mip_width, mip_height, mip_depth, 0, data_size, data_ptr);
                            }
                            else {
                                const GLenum gl_type = _sg_gl_teximage_type(img->cmn.pixel_format);
                                glTexImage3D(gl_img_target, mip_index, (GLint)gl_internal_format,
                                    mip_width, mip_height, mip_depth, 0, gl_format, gl_type, data_ptr);
                            }
                        }
                        #endif
                    }
                }
                _sg_gl_cache_restore_texture_binding(0);
            }
        }
    }
    _SG_GL_CHECK_ERROR();
    return SG_RESOURCESTATE_VALID;
}

_SOKOL_PRIVATE void _sg_gl_discard_image(_sg_image_t* img) {
    SOKOL_ASSERT(img);
    _SG_GL_CHECK_ERROR();
    for (int slot = 0; slot < img->cmn.num_slots; slot++) {
        if (img->gl.tex[slot]) {
            _sg_gl_cache_invalidate_texture(img->gl.tex[slot]);
            if (!img->gl.ext_textures) {
                glDeleteTextures(1, &img->gl.tex[slot]);
            }
        }
    }
    if (img->gl.depth_render_buffer) {
        glDeleteRenderbuffers(1, &img->gl.depth_render_buffer);
    }
    if (img->gl.msaa_render_buffer) {
        glDeleteRenderbuffers(1, &img->gl.msaa_render_buffer);
    }
    _SG_GL_CHECK_ERROR();
}

_SOKOL_PRIVATE GLuint _sg_gl_compile_shader(sg_shader_stage stage, const char* src) {
    SOKOL_ASSERT(src);
    _SG_GL_CHECK_ERROR();
    GLuint gl_shd = glCreateShader(_sg_gl_shader_stage(stage));
    glShaderSource(gl_shd, 1, &src, 0);
    glCompileShader(gl_shd);
    GLint compile_status = 0;
    glGetShaderiv(gl_shd, GL_COMPILE_STATUS, &compile_status);
    if (!compile_status) {
        /* compilation failed, log error and delete shader */
        GLint log_len = 0;
        glGetShaderiv(gl_shd, GL_INFO_LOG_LENGTH, &log_len);
        if (log_len > 0) {
            GLchar* log_buf = (GLchar*) _sg_malloc((size_t)log_len);
            glGetShaderInfoLog(gl_shd, log_len, &log_len, log_buf);
            SG_LOG(log_buf);
            _sg_free(log_buf);
        }
        glDeleteShader(gl_shd);
        gl_shd = 0;
    }
    _SG_GL_CHECK_ERROR();
    return gl_shd;
}

_SOKOL_PRIVATE sg_resource_state _sg_gl_create_shader(_sg_shader_t* shd, const sg_shader_desc* desc) {
    SOKOL_ASSERT(shd && desc);
    SOKOL_ASSERT(!shd->gl.prog);
    _SG_GL_CHECK_ERROR();

    _sg_shader_common_init(&shd->cmn, desc);

    /* copy vertex attribute names over, these are required for GLES2, and optional for GLES3 and GL3.x */
    for (int i = 0; i < SG_MAX_VERTEX_ATTRIBUTES; i++) {
        _sg_strcpy(&shd->gl.attrs[i].name, desc->attrs[i].name);
    }

    GLuint gl_vs = _sg_gl_compile_shader(SG_SHADERSTAGE_VS, desc->vs.source);
    GLuint gl_fs = _sg_gl_compile_shader(SG_SHADERSTAGE_FS, desc->fs.source);
    if (!(gl_vs && gl_fs)) {
        return SG_RESOURCESTATE_FAILED;
    }
    GLuint gl_prog = glCreateProgram();
    glAttachShader(gl_prog, gl_vs);
    glAttachShader(gl_prog, gl_fs);
    glLinkProgram(gl_prog);
    glDeleteShader(gl_vs);
    glDeleteShader(gl_fs);
    _SG_GL_CHECK_ERROR();

    GLint link_status;
    glGetProgramiv(gl_prog, GL_LINK_STATUS, &link_status);
    if (!link_status) {
        GLint log_len = 0;
        glGetProgramiv(gl_prog, GL_INFO_LOG_LENGTH, &log_len);
        if (log_len > 0) {
            GLchar* log_buf = (GLchar*) _sg_malloc((size_t)log_len);
            glGetProgramInfoLog(gl_prog, log_len, &log_len, log_buf);
            SG_LOG(log_buf);
            _sg_free(log_buf);
        }
        glDeleteProgram(gl_prog);
        return SG_RESOURCESTATE_FAILED;
    }
    shd->gl.prog = gl_prog;

    /* resolve uniforms */
    _SG_GL_CHECK_ERROR();
    for (int stage_index = 0; stage_index < SG_NUM_SHADER_STAGES; stage_index++) {
        const sg_shader_stage_desc* stage_desc = (stage_index == SG_SHADERSTAGE_VS)? &desc->vs : &desc->fs;
        _sg_gl_shader_stage_t* gl_stage = &shd->gl.stage[stage_index];
        for (int ub_index = 0; ub_index < shd->cmn.stage[stage_index].num_uniform_blocks; ub_index++) {
            const sg_shader_uniform_block_desc* ub_desc = &stage_desc->uniform_blocks[ub_index];
            SOKOL_ASSERT(ub_desc->size > 0);
            _sg_gl_uniform_block_t* ub = &gl_stage->uniform_blocks[ub_index];
            SOKOL_ASSERT(ub->num_uniforms == 0);
            uint32_t cur_uniform_offset = 0;
            for (int u_index = 0; u_index < SG_MAX_UB_MEMBERS; u_index++) {
                const sg_shader_uniform_desc* u_desc = &ub_desc->uniforms[u_index];
                if (u_desc->type == SG_UNIFORMTYPE_INVALID) {
                    break;
                }
                const uint32_t u_align = _sg_uniform_alignment(u_desc->type, u_desc->array_count, ub_desc->layout);
                const uint32_t u_size = _sg_uniform_size(u_desc->type, u_desc->array_count, ub_desc->layout);
                cur_uniform_offset = _sg_align_u32(cur_uniform_offset, u_align);
                _sg_gl_uniform_t* u = &ub->uniforms[u_index];
                u->type = u_desc->type;
                u->count = (uint16_t) u_desc->array_count;
                u->offset = (uint16_t) cur_uniform_offset;
                cur_uniform_offset += u_size;
                if (u_desc->name) {
                    u->gl_loc = glGetUniformLocation(gl_prog, u_desc->name);
                }
                else {
                    u->gl_loc = u_index;
                }
                ub->num_uniforms++;
            }
            if (ub_desc->layout == SG_UNIFORMLAYOUT_STD140) {
                cur_uniform_offset = _sg_align_u32(cur_uniform_offset, 16);
            }
            SOKOL_ASSERT(ub_desc->size == (size_t)cur_uniform_offset);
            _SOKOL_UNUSED(cur_uniform_offset);
        }
    }

    /* resolve image locations */
    _SG_GL_CHECK_ERROR();
    GLuint cur_prog = 0;
    glGetIntegerv(GL_CURRENT_PROGRAM, (GLint*)&cur_prog);
    glUseProgram(gl_prog);
    int gl_tex_slot = 0;
    for (int stage_index = 0; stage_index < SG_NUM_SHADER_STAGES; stage_index++) {
        const sg_shader_stage_desc* stage_desc = (stage_index == SG_SHADERSTAGE_VS)? &desc->vs : &desc->fs;
        _sg_gl_shader_stage_t* gl_stage = &shd->gl.stage[stage_index];
        for (int img_index = 0; img_index < shd->cmn.stage[stage_index].num_images; img_index++) {
            const sg_shader_image_desc* img_desc = &stage_desc->images[img_index];
            SOKOL_ASSERT(img_desc->image_type != _SG_IMAGETYPE_DEFAULT);
            _sg_gl_shader_image_t* gl_img = &gl_stage->images[img_index];
            GLint gl_loc = img_index;
            if (img_desc->name) {
                gl_loc = glGetUniformLocation(gl_prog, img_desc->name);
            }
            if (gl_loc != -1) {
                gl_img->gl_tex_slot = gl_tex_slot++;
                glUniform1i(gl_loc, gl_img->gl_tex_slot);
            }
            else {
                gl_img->gl_tex_slot = -1;
            }
        }
    }
    /* it's legal to call glUseProgram with 0 */
    glUseProgram(cur_prog);
    _SG_GL_CHECK_ERROR();
    return SG_RESOURCESTATE_VALID;
}

_SOKOL_PRIVATE void _sg_gl_discard_shader(_sg_shader_t* shd) {
    SOKOL_ASSERT(shd);
    _SG_GL_CHECK_ERROR();
    if (shd->gl.prog) {
        _sg_gl_cache_invalidate_program(shd->gl.prog);
        glDeleteProgram(shd->gl.prog);
    }
    _SG_GL_CHECK_ERROR();
}

_SOKOL_PRIVATE sg_resource_state _sg_gl_create_pipeline(_sg_pipeline_t* pip, _sg_shader_t* shd, const sg_pipeline_desc* desc) {
    SOKOL_ASSERT(pip && shd && desc);
    SOKOL_ASSERT(!pip->shader && pip->cmn.shader_id.id == SG_INVALID_ID);
    SOKOL_ASSERT(desc->shader.id == shd->slot.id);
    SOKOL_ASSERT(shd->gl.prog);
    pip->shader = shd;
    _sg_pipeline_common_init(&pip->cmn, desc);
    pip->gl.primitive_type = desc->primitive_type;
    pip->gl.depth = desc->depth;
    pip->gl.stencil = desc->stencil;
    // FIXME: blend color and write mask per draw-buffer-attachment (requires GL4)
    pip->gl.blend = desc->colors[0].blend;
    for (int i = 0; i < SG_MAX_COLOR_ATTACHMENTS; i++) {
        pip->gl.color_write_mask[i] = desc->colors[i].write_mask;
    }
    pip->gl.cull_mode = desc->cull_mode;
    pip->gl.face_winding = desc->face_winding;
    pip->gl.sample_count = desc->sample_count;
    pip->gl.alpha_to_coverage_enabled = desc->alpha_to_coverage_enabled;

    /* resolve vertex attributes */
    for (int attr_index = 0; attr_index < SG_MAX_VERTEX_ATTRIBUTES; attr_index++) {
        pip->gl.attrs[attr_index].vb_index = -1;
    }
    for (int attr_index = 0; attr_index < _sg.limits.max_vertex_attrs; attr_index++) {
        const sg_vertex_attr_desc* a_desc = &desc->layout.attrs[attr_index];
        if (a_desc->format == SG_VERTEXFORMAT_INVALID) {
            break;
        }
        SOKOL_ASSERT(a_desc->buffer_index < SG_MAX_SHADERSTAGE_BUFFERS);
        const sg_buffer_layout_desc* l_desc = &desc->layout.buffers[a_desc->buffer_index];
        const sg_vertex_step step_func = l_desc->step_func;
        const int step_rate = l_desc->step_rate;
        GLint attr_loc = attr_index;
        if (!_sg_strempty(&shd->gl.attrs[attr_index].name)) {
            attr_loc = glGetAttribLocation(pip->shader->gl.prog, _sg_strptr(&shd->gl.attrs[attr_index].name));
        }
        SOKOL_ASSERT(attr_loc < (GLint)_sg.limits.max_vertex_attrs);
        if (attr_loc != -1) {
            _sg_gl_attr_t* gl_attr = &pip->gl.attrs[attr_loc];
            SOKOL_ASSERT(gl_attr->vb_index == -1);
            gl_attr->vb_index = (int8_t) a_desc->buffer_index;
            if (step_func == SG_VERTEXSTEP_PER_VERTEX) {
                gl_attr->divisor = 0;
            }
            else {
                gl_attr->divisor = (int8_t) step_rate;
                pip->cmn.use_instanced_draw = true;
            }
            SOKOL_ASSERT(l_desc->stride > 0);
            gl_attr->stride = (uint8_t) l_desc->stride;
            gl_attr->offset = a_desc->offset;
            gl_attr->size = (uint8_t) _sg_gl_vertexformat_size(a_desc->format);
            gl_attr->type = _sg_gl_vertexformat_type(a_desc->format);
            gl_attr->normalized = _sg_gl_vertexformat_normalized(a_desc->format);
            pip->cmn.vertex_layout_valid[a_desc->buffer_index] = true;
        }
        else {
            SG_LOG("Vertex attribute not found in shader: ");
            SG_LOG(_sg_strptr(&shd->gl.attrs[attr_index].name));
        }
    }
    return SG_RESOURCESTATE_VALID;
}

_SOKOL_PRIVATE void _sg_gl_discard_pipeline(_sg_pipeline_t* pip) {
    SOKOL_ASSERT(pip);
    _sg_gl_cache_invalidate_pipeline(pip);
}

/*
    _sg_create_pass

    att_imgs must point to a _sg_image* att_imgs[SG_MAX_COLOR_ATTACHMENTS+1] array,
    first entries are the color attachment images (or nullptr), last entry
    is the depth-stencil image (or nullptr).
*/
_SOKOL_PRIVATE sg_resource_state _sg_gl_create_pass(_sg_pass_t* pass, _sg_image_t** att_images, const sg_pass_desc* desc) {
    SOKOL_ASSERT(pass && att_images && desc);
    SOKOL_ASSERT(att_images && att_images[0]);
    _SG_GL_CHECK_ERROR();

    _sg_pass_common_init(&pass->cmn, desc);

    /* copy image pointers */
    const sg_pass_attachment_desc* att_desc;
    for (int i = 0; i < pass->cmn.num_color_atts; i++) {
        att_desc = &desc->color_attachments[i];
        SOKOL_ASSERT(att_desc->image.id != SG_INVALID_ID);
        SOKOL_ASSERT(0 == pass->gl.color_atts[i].image);
        SOKOL_ASSERT(att_images[i] && (att_images[i]->slot.id == att_desc->image.id));
        SOKOL_ASSERT(_sg_is_valid_rendertarget_color_format(att_images[i]->cmn.pixel_format));
        pass->gl.color_atts[i].image = att_images[i];
    }
    SOKOL_ASSERT(0 == pass->gl.ds_att.image);
    att_desc = &desc->depth_stencil_attachment;
    if (att_desc->image.id != SG_INVALID_ID) {
        const int ds_img_index = SG_MAX_COLOR_ATTACHMENTS;
        SOKOL_ASSERT(att_images[ds_img_index] && (att_images[ds_img_index]->slot.id == att_desc->image.id));
        SOKOL_ASSERT(_sg_is_valid_rendertarget_depth_format(att_images[ds_img_index]->cmn.pixel_format));
        pass->gl.ds_att.image = att_images[ds_img_index];
    }

    /* store current framebuffer binding (restored at end of function) */
    GLuint gl_orig_fb;
    glGetIntegerv(GL_FRAMEBUFFER_BINDING, (GLint*)&gl_orig_fb);

    /* create a framebuffer object */
    glGenFramebuffers(1, &pass->gl.fb);
    glBindFramebuffer(GL_FRAMEBUFFER, pass->gl.fb);

    /* attach msaa render buffer or textures */
    const bool is_msaa = (0 != att_images[0]->gl.msaa_render_buffer);
    if (is_msaa) {
        for (int i = 0; i < SG_MAX_COLOR_ATTACHMENTS; i++) {
            const _sg_image_t* att_img = pass->gl.color_atts[i].image;
            if (att_img) {
                const GLuint gl_render_buffer = att_img->gl.msaa_render_buffer;
                SOKOL_ASSERT(gl_render_buffer);
                glFramebufferRenderbuffer(GL_FRAMEBUFFER, (GLenum)(GL_COLOR_ATTACHMENT0+i), GL_RENDERBUFFER, gl_render_buffer);
            }
        }
    }
    else {
        for (int i = 0; i < SG_MAX_COLOR_ATTACHMENTS; i++) {
            const _sg_image_t* att_img = pass->gl.color_atts[i].image;
            const int mip_level = pass->cmn.color_atts[i].mip_level;
            const int slice = pass->cmn.color_atts[i].slice;
            if (att_img) {
                const GLuint gl_tex = att_img->gl.tex[0];
                SOKOL_ASSERT(gl_tex);
                const GLenum gl_att = (GLenum)(GL_COLOR_ATTACHMENT0 + i);
                switch (att_img->cmn.type) {
                    case SG_IMAGETYPE_2D:
                        glFramebufferTexture2D(GL_FRAMEBUFFER, gl_att, GL_TEXTURE_2D, gl_tex, mip_level);
                        break;
                    case SG_IMAGETYPE_CUBE:
                        glFramebufferTexture2D(GL_FRAMEBUFFER, gl_att, _sg_gl_cubeface_target(slice), gl_tex, mip_level);
                        break;
                    default:
                        /* 3D- or array-texture */
                        #if !defined(SOKOL_GLES2)
                        if (!_sg.gl.gles2) {
                            glFramebufferTextureLayer(GL_FRAMEBUFFER, gl_att, gl_tex, mip_level, slice);
                        }
                        #endif
                        break;
                }
            }
        }
    }

    /* attach depth-stencil buffer to framebuffer */
    if (pass->gl.ds_att.image) {
        const GLuint gl_render_buffer = pass->gl.ds_att.image->gl.depth_render_buffer;
        SOKOL_ASSERT(gl_render_buffer);
        glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, gl_render_buffer);
        if (_sg_is_depth_stencil_format(pass->gl.ds_att.image->cmn.pixel_format)) {
            glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_STENCIL_ATTACHMENT, GL_RENDERBUFFER, gl_render_buffer);
        }
    }

    /* check if framebuffer is complete */
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        SG_LOG("Framebuffer completeness check failed!\n");
        return SG_RESOURCESTATE_FAILED;
    }

    /* setup color attachments for the framebuffer */
    #if !defined(SOKOL_GLES2)
    if (!_sg.gl.gles2) {
        GLenum att[SG_MAX_COLOR_ATTACHMENTS] = {
            GL_COLOR_ATTACHMENT0,
            GL_COLOR_ATTACHMENT1,
            GL_COLOR_ATTACHMENT2,
            GL_COLOR_ATTACHMENT3
        };
        glDrawBuffers(pass->cmn.num_color_atts, att);
    }
    #endif

    /* create MSAA resolve framebuffers if necessary */
    if (is_msaa) {
        for (int i = 0; i < SG_MAX_COLOR_ATTACHMENTS; i++) {
            _sg_gl_attachment_t* gl_att = &pass->gl.color_atts[i];
            _sg_pass_attachment_t* cmn_att = &pass->cmn.color_atts[i];
            if (gl_att->image) {
                SOKOL_ASSERT(0 == gl_att->gl_msaa_resolve_buffer);
                glGenFramebuffers(1, &gl_att->gl_msaa_resolve_buffer);
                glBindFramebuffer(GL_FRAMEBUFFER, gl_att->gl_msaa_resolve_buffer);
                const GLuint gl_tex = gl_att->image->gl.tex[0];
                SOKOL_ASSERT(gl_tex);
                switch (gl_att->image->cmn.type) {
                    case SG_IMAGETYPE_2D:
                        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                            GL_TEXTURE_2D, gl_tex, cmn_att->mip_level);
                        break;
                    case SG_IMAGETYPE_CUBE:
                        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                            _sg_gl_cubeface_target(cmn_att->slice), gl_tex, cmn_att->mip_level);
                        break;
                    default:
                        #if !defined(SOKOL_GLES2)
                        if (!_sg.gl.gles2) {
                            glFramebufferTextureLayer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, gl_tex, cmn_att->mip_level, cmn_att->slice);
                        }
                        #endif
                        break;
                }
                /* check if framebuffer is complete */
                if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
                    SG_LOG("Framebuffer completeness check failed (msaa resolve buffer)!\n");
                    return SG_RESOURCESTATE_FAILED;
                }
                /* setup color attachments for the framebuffer */
                #if !defined(SOKOL_GLES2)
                if (!_sg.gl.gles2) {
                    const GLenum gl_draw_bufs = GL_COLOR_ATTACHMENT0;
                    glDrawBuffers(1, &gl_draw_bufs);
                }
                #endif
            }
        }
    }

    /* restore original framebuffer binding */
    glBindFramebuffer(GL_FRAMEBUFFER, gl_orig_fb);
    _SG_GL_CHECK_ERROR();
    return SG_RESOURCESTATE_VALID;
}

_SOKOL_PRIVATE void _sg_gl_discard_pass(_sg_pass_t* pass) {
    SOKOL_ASSERT(pass);
    SOKOL_ASSERT(pass != _sg.gl.cur_pass);
    _SG_GL_CHECK_ERROR();
    if (0 != pass->gl.fb) {
        glDeleteFramebuffers(1, &pass->gl.fb);
    }
    for (int i = 0; i < SG_MAX_COLOR_ATTACHMENTS; i++) {
        if (pass->gl.color_atts[i].gl_msaa_resolve_buffer) {
            glDeleteFramebuffers(1, &pass->gl.color_atts[i].gl_msaa_resolve_buffer);
        }
    }
    if (pass->gl.ds_att.gl_msaa_resolve_buffer) {
        glDeleteFramebuffers(1, &pass->gl.ds_att.gl_msaa_resolve_buffer);
    }
    _SG_GL_CHECK_ERROR();
}

_SOKOL_PRIVATE _sg_image_t* _sg_gl_pass_color_image(const _sg_pass_t* pass, int index) {
    SOKOL_ASSERT(pass && (index >= 0) && (index < SG_MAX_COLOR_ATTACHMENTS));
    /* NOTE: may return null */
    return pass->gl.color_atts[index].image;
}

_SOKOL_PRIVATE _sg_image_t* _sg_gl_pass_ds_image(const _sg_pass_t* pass) {
    /* NOTE: may return null */
    SOKOL_ASSERT(pass);
    return pass->gl.ds_att.image;
}

_SOKOL_PRIVATE void _sg_gl_begin_pass(_sg_pass_t* pass, const sg_pass_action* action, int w, int h) {
    /* FIXME: what if a texture used as render target is still bound, should we
       unbind all currently bound textures in begin pass? */
    SOKOL_ASSERT(action);
    SOKOL_ASSERT(!_sg.gl.in_pass);
    _SG_GL_CHECK_ERROR();
    _sg.gl.in_pass = true;
    _sg.gl.cur_pass = pass; /* can be 0 */
    if (pass) {
        _sg.gl.cur_pass_id.id = pass->slot.id;
    }
    else {
        _sg.gl.cur_pass_id.id = SG_INVALID_ID;
    }
    _sg.gl.cur_pass_width = w;
    _sg.gl.cur_pass_height = h;

    /* number of color attachments */
    const int num_color_atts = pass ? pass->cmn.num_color_atts : 1;

    /* bind the render pass framebuffer */
    if (pass) {
        /* offscreen pass */
        SOKOL_ASSERT(pass->gl.fb);
        glBindFramebuffer(GL_FRAMEBUFFER, pass->gl.fb);
    }
    else {
        /* default pass */
        SOKOL_ASSERT(_sg.gl.cur_context);
        glBindFramebuffer(GL_FRAMEBUFFER, _sg.gl.cur_context->default_framebuffer);
    }
    glViewport(0, 0, w, h);
    glScissor(0, 0, w, h);

    /* clear color and depth-stencil attachments if needed */
    bool clear_color = false;
    for (int i = 0; i < num_color_atts; i++) {
        if (SG_ACTION_CLEAR == action->colors[i].action) {
            clear_color = true;
            break;
        }
    }
    const bool clear_depth = (action->depth.action == SG_ACTION_CLEAR);
    const bool clear_stencil = (action->stencil.action == SG_ACTION_CLEAR);

    bool need_pip_cache_flush = false;
    if (clear_color) {
        bool need_color_mask_flush = false;
        // NOTE: not a bug to iterate over all possible color attachments
        for (int i = 0; i < SG_MAX_COLOR_ATTACHMENTS; i++) {
            if (SG_COLORMASK_RGBA != _sg.gl.cache.color_write_mask[i]) {
                need_pip_cache_flush = true;
                need_color_mask_flush = true;
                _sg.gl.cache.color_write_mask[i] = SG_COLORMASK_RGBA;
            }
        }
        if (need_color_mask_flush) {
            glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
        }
    }
    if (clear_depth) {
        if (!_sg.gl.cache.depth.write_enabled) {
            need_pip_cache_flush = true;
            _sg.gl.cache.depth.write_enabled = true;
            glDepthMask(GL_TRUE);
        }
        if (_sg.gl.cache.depth.compare != SG_COMPAREFUNC_ALWAYS) {
            need_pip_cache_flush = true;
            _sg.gl.cache.depth.compare = SG_COMPAREFUNC_ALWAYS;
            glDepthFunc(GL_ALWAYS);
        }
    }
    if (clear_stencil) {
        if (_sg.gl.cache.stencil.write_mask != 0xFF) {
            need_pip_cache_flush = true;
            _sg.gl.cache.stencil.write_mask = 0xFF;
            glStencilMask(0xFF);
        }
    }
    if (need_pip_cache_flush) {
        /* we messed with the state cache directly, need to clear cached
           pipeline to force re-evaluation in next sg_apply_pipeline() */
        _sg.gl.cache.cur_pipeline = 0;
        _sg.gl.cache.cur_pipeline_id.id = SG_INVALID_ID;
    }
    bool use_mrt_clear = (0 != pass);
    #if defined(SOKOL_GLES2)
    use_mrt_clear = false;
    #else
    if (_sg.gl.gles2) {
        use_mrt_clear = false;
    }
    #endif
    if (!use_mrt_clear) {
        GLbitfield clear_mask = 0;
        if (clear_color) {
            clear_mask |= GL_COLOR_BUFFER_BIT;
            const sg_color c = action->colors[0].value;
            glClearColor(c.r, c.g, c.b, c.a);
        }
        if (clear_depth) {
            clear_mask |= GL_DEPTH_BUFFER_BIT;
            #ifdef SOKOL_GLCORE33
            glClearDepth(action->depth.value);
            #else
            glClearDepthf(action->depth.value);
            #endif
        }
        if (clear_stencil) {
            clear_mask |= GL_STENCIL_BUFFER_BIT;
            glClearStencil(action->stencil.value);
        }
        if (0 != clear_mask) {
            glClear(clear_mask);
        }
    }
    #if !defined SOKOL_GLES2
    else {
        SOKOL_ASSERT(pass);
        for (int i = 0; i < num_color_atts; i++) {
            if (action->colors[i].action == SG_ACTION_CLEAR) {
                glClearBufferfv(GL_COLOR, i, &action->colors[i].value.r);
            }
        }
        if (pass->gl.ds_att.image) {
            if (clear_depth && clear_stencil) {
                glClearBufferfi(GL_DEPTH_STENCIL, 0, action->depth.value, action->stencil.value);
            }
            else if (clear_depth) {
                glClearBufferfv(GL_DEPTH, 0, &action->depth.value);
            }
            else if (clear_stencil) {
                GLint val = (GLint) action->stencil.value;
                glClearBufferiv(GL_STENCIL, 0, &val);
            }
        }
    }
    #endif
    _SG_GL_CHECK_ERROR();
}

_SOKOL_PRIVATE void _sg_gl_end_pass(void) {
    SOKOL_ASSERT(_sg.gl.in_pass);
    _SG_GL_CHECK_ERROR();

    /* if this was an offscreen pass, and MSAA rendering was used, need
       to resolve into the pass images */
    #if !defined(SOKOL_GLES2)
    if (!_sg.gl.gles2 && _sg.gl.cur_pass) {
        /* check if the pass object is still valid */
        const _sg_pass_t* pass = _sg.gl.cur_pass;
        SOKOL_ASSERT(pass->slot.id == _sg.gl.cur_pass_id.id);
        bool is_msaa = (0 != _sg.gl.cur_pass->gl.color_atts[0].gl_msaa_resolve_buffer);
        if (is_msaa) {
            SOKOL_ASSERT(pass->gl.fb);
            glBindFramebuffer(GL_READ_FRAMEBUFFER, pass->gl.fb);
            SOKOL_ASSERT(pass->gl.color_atts[0].image);
            const int w = pass->gl.color_atts[0].image->cmn.width;
            const int h = pass->gl.color_atts[0].image->cmn.height;
            for (int att_index = 0; att_index < SG_MAX_COLOR_ATTACHMENTS; att_index++) {
                const _sg_gl_attachment_t* gl_att = &pass->gl.color_atts[att_index];
                if (gl_att->image) {
                    SOKOL_ASSERT(gl_att->gl_msaa_resolve_buffer);
                    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, gl_att->gl_msaa_resolve_buffer);
                    glReadBuffer((GLenum)(GL_COLOR_ATTACHMENT0 + att_index));
                    glBlitFramebuffer(0, 0, w, h, 0, 0, w, h, GL_COLOR_BUFFER_BIT, GL_NEAREST);
                }
                else {
                    break;
                }
            }
        }
    }
    #endif
    _sg.gl.cur_pass = 0;
    _sg.gl.cur_pass_id.id = SG_INVALID_ID;
    _sg.gl.cur_pass_width = 0;
    _sg.gl.cur_pass_height = 0;

    SOKOL_ASSERT(_sg.gl.cur_context);
    glBindFramebuffer(GL_FRAMEBUFFER, _sg.gl.cur_context->default_framebuffer);
    _sg.gl.in_pass = false;
    _SG_GL_CHECK_ERROR();
}

_SOKOL_PRIVATE void _sg_gl_apply_viewport(int x, int y, int w, int h, bool origin_top_left) {
    SOKOL_ASSERT(_sg.gl.in_pass);
    y = origin_top_left ? (_sg.gl.cur_pass_height - (y+h)) : y;
    glViewport(x, y, w, h);
}

_SOKOL_PRIVATE void _sg_gl_apply_scissor_rect(int x, int y, int w, int h, bool origin_top_left) {
    SOKOL_ASSERT(_sg.gl.in_pass);
    y = origin_top_left ? (_sg.gl.cur_pass_height - (y+h)) : y;
    glScissor(x, y, w, h);
}

_SOKOL_PRIVATE void _sg_gl_apply_pipeline(_sg_pipeline_t* pip) {
    SOKOL_ASSERT(pip);
    SOKOL_ASSERT(pip->shader && (pip->cmn.shader_id.id == pip->shader->slot.id));
    _SG_GL_CHECK_ERROR();
    if ((_sg.gl.cache.cur_pipeline != pip) || (_sg.gl.cache.cur_pipeline_id.id != pip->slot.id)) {
        _sg.gl.cache.cur_pipeline = pip;
        _sg.gl.cache.cur_pipeline_id.id = pip->slot.id;
        _sg.gl.cache.cur_primitive_type = _sg_gl_primitive_type(pip->gl.primitive_type);
        _sg.gl.cache.cur_index_type = _sg_gl_index_type(pip->cmn.index_type);

        /* update depth state */
        {
            const sg_depth_state* state_ds = &pip->gl.depth;
            sg_depth_state* cache_ds = &_sg.gl.cache.depth;
            if (state_ds->compare != cache_ds->compare) {
                cache_ds->compare = state_ds->compare;
                glDepthFunc(_sg_gl_compare_func(state_ds->compare));
            }
            if (state_ds->write_enabled != cache_ds->write_enabled) {
                cache_ds->write_enabled = state_ds->write_enabled;
                glDepthMask(state_ds->write_enabled);
            }
            if (!_sg_fequal(state_ds->bias, cache_ds->bias, 0.000001f) ||
                !_sg_fequal(state_ds->bias_slope_scale, cache_ds->bias_slope_scale, 0.000001f))
            {
                /* according to ANGLE's D3D11 backend:
                    D3D11 SlopeScaledDepthBias ==> GL polygonOffsetFactor
                    D3D11 DepthBias ==> GL polygonOffsetUnits
                    DepthBiasClamp has no meaning on GL
                */
                cache_ds->bias = state_ds->bias;
                cache_ds->bias_slope_scale = state_ds->bias_slope_scale;
                glPolygonOffset(state_ds->bias_slope_scale, state_ds->bias);
                bool po_enabled = true;
                if (_sg_fequal(state_ds->bias, 0.0f, 0.000001f) &&
                    _sg_fequal(state_ds->bias_slope_scale, 0.0f, 0.000001f))
                {
                    po_enabled = false;
                }
                if (po_enabled != _sg.gl.cache.polygon_offset_enabled) {
                    _sg.gl.cache.polygon_offset_enabled = po_enabled;
                    if (po_enabled) {
                        glEnable(GL_POLYGON_OFFSET_FILL);
                    }
                    else {
                        glDisable(GL_POLYGON_OFFSET_FILL);
                    }
                }
            }
        }

        /* update stencil state */
        {
            const sg_stencil_state* state_ss = &pip->gl.stencil;
            sg_stencil_state* cache_ss = &_sg.gl.cache.stencil;
            if (state_ss->enabled != cache_ss->enabled) {
                cache_ss->enabled = state_ss->enabled;
                if (state_ss->enabled) {
                    glEnable(GL_STENCIL_TEST);
                }
                else {
                    glDisable(GL_STENCIL_TEST);
                }
            }
            if (state_ss->write_mask != cache_ss->write_mask) {
                cache_ss->write_mask = state_ss->write_mask;
                glStencilMask(state_ss->write_mask);
            }
            for (int i = 0; i < 2; i++) {
                const sg_stencil_face_state* state_sfs = (i==0)? &state_ss->front : &state_ss->back;
                sg_stencil_face_state* cache_sfs = (i==0)? &cache_ss->front : &cache_ss->back;
                GLenum gl_face = (i==0)? GL_FRONT : GL_BACK;
                if ((state_sfs->compare != cache_sfs->compare) ||
                    (state_ss->read_mask != cache_ss->read_mask) ||
                    (state_ss->ref != cache_ss->ref))
                {
                    cache_sfs->compare = state_sfs->compare;
                    glStencilFuncSeparate(gl_face,
                        _sg_gl_compare_func(state_sfs->compare),
                        state_ss->ref,
                        state_ss->read_mask);
                }
                if ((state_sfs->fail_op != cache_sfs->fail_op) ||
                    (state_sfs->depth_fail_op != cache_sfs->depth_fail_op) ||
                    (state_sfs->pass_op != cache_sfs->pass_op))
                {
                    cache_sfs->fail_op = state_sfs->fail_op;
                    cache_sfs->depth_fail_op = state_sfs->depth_fail_op;
                    cache_sfs->pass_op = state_sfs->pass_op;
                    glStencilOpSeparate(gl_face,
                        _sg_gl_stencil_op(state_sfs->fail_op),
                        _sg_gl_stencil_op(state_sfs->depth_fail_op),
                        _sg_gl_stencil_op(state_sfs->pass_op));
                }
            }
            cache_ss->read_mask = state_ss->read_mask;
            cache_ss->ref = state_ss->ref;
        }

        /* update blend state
            FIXME: separate blend state per color attachment not support, needs GL4
        */
        {
            const sg_blend_state* state_bs = &pip->gl.blend;
            sg_blend_state* cache_bs = &_sg.gl.cache.blend;
            if (state_bs->enabled != cache_bs->enabled) {
                cache_bs->enabled = state_bs->enabled;
                if (state_bs->enabled) {
                    glEnable(GL_BLEND);
                }
                else {
                    glDisable(GL_BLEND);
                }
            }
            if ((state_bs->src_factor_rgb != cache_bs->src_factor_rgb) ||
                (state_bs->dst_factor_rgb != cache_bs->dst_factor_rgb) ||
                (state_bs->src_factor_alpha != cache_bs->src_factor_alpha) ||
                (state_bs->dst_factor_alpha != cache_bs->dst_factor_alpha))
            {
                cache_bs->src_factor_rgb = state_bs->src_factor_rgb;
                cache_bs->dst_factor_rgb = state_bs->dst_factor_rgb;
                cache_bs->src_factor_alpha = state_bs->src_factor_alpha;
                cache_bs->dst_factor_alpha = state_bs->dst_factor_alpha;
                glBlendFuncSeparate(_sg_gl_blend_factor(state_bs->src_factor_rgb),
                    _sg_gl_blend_factor(state_bs->dst_factor_rgb),
                    _sg_gl_blend_factor(state_bs->src_factor_alpha),
                    _sg_gl_blend_factor(state_bs->dst_factor_alpha));
            }
            if ((state_bs->op_rgb != cache_bs->op_rgb) || (state_bs->op_alpha != cache_bs->op_alpha)) {
                cache_bs->op_rgb = state_bs->op_rgb;
                cache_bs->op_alpha = state_bs->op_alpha;
                glBlendEquationSeparate(_sg_gl_blend_op(state_bs->op_rgb), _sg_gl_blend_op(state_bs->op_alpha));
            }
        }

        /* standalone state */
        for (GLuint i = 0; i < (GLuint)pip->cmn.color_attachment_count; i++) {
            if (pip->gl.color_write_mask[i] != _sg.gl.cache.color_write_mask[i]) {
                const sg_color_mask cm = pip->gl.color_write_mask[i];
                _sg.gl.cache.color_write_mask[i] = cm;
                #ifdef SOKOL_GLCORE33
                    glColorMaski(i,
                                (cm & SG_COLORMASK_R) != 0,
                                (cm & SG_COLORMASK_G) != 0,
                                (cm & SG_COLORMASK_B) != 0,
                                (cm & SG_COLORMASK_A) != 0);
                #else
                    if (0 == i) {
                        glColorMask((cm & SG_COLORMASK_R) != 0,
                                    (cm & SG_COLORMASK_G) != 0,
                                    (cm & SG_COLORMASK_B) != 0,
                                    (cm & SG_COLORMASK_A) != 0);
                    }
                #endif
            }
        }

        if (!_sg_fequal(pip->cmn.blend_color.r, _sg.gl.cache.blend_color.r, 0.0001f) ||
            !_sg_fequal(pip->cmn.blend_color.g, _sg.gl.cache.blend_color.g, 0.0001f) ||
            !_sg_fequal(pip->cmn.blend_color.b, _sg.gl.cache.blend_color.b, 0.0001f) ||
            !_sg_fequal(pip->cmn.blend_color.a, _sg.gl.cache.blend_color.a, 0.0001f))
        {
            sg_color c = pip->cmn.blend_color;
            _sg.gl.cache.blend_color = c;
            glBlendColor(c.r, c.g, c.b, c.a);
        }
        if (pip->gl.cull_mode != _sg.gl.cache.cull_mode) {
            _sg.gl.cache.cull_mode = pip->gl.cull_mode;
            if (SG_CULLMODE_NONE == pip->gl.cull_mode) {
                glDisable(GL_CULL_FACE);
            }
            else {
                glEnable(GL_CULL_FACE);
                GLenum gl_mode = (SG_CULLMODE_FRONT == pip->gl.cull_mode) ? GL_FRONT : GL_BACK;
                glCullFace(gl_mode);
            }
        }
        if (pip->gl.face_winding != _sg.gl.cache.face_winding) {
            _sg.gl.cache.face_winding = pip->gl.face_winding;
            GLenum gl_winding = (SG_FACEWINDING_CW == pip->gl.face_winding) ? GL_CW : GL_CCW;
            glFrontFace(gl_winding);
        }
        if (pip->gl.alpha_to_coverage_enabled != _sg.gl.cache.alpha_to_coverage_enabled) {
            _sg.gl.cache.alpha_to_coverage_enabled = pip->gl.alpha_to_coverage_enabled;
            if (pip->gl.alpha_to_coverage_enabled) {
                glEnable(GL_SAMPLE_ALPHA_TO_COVERAGE);
            }
            else {
                glDisable(GL_SAMPLE_ALPHA_TO_COVERAGE);
            }
        }
        #ifdef SOKOL_GLCORE33
        if (pip->gl.sample_count != _sg.gl.cache.sample_count) {
            _sg.gl.cache.sample_count = pip->gl.sample_count;
            if (pip->gl.sample_count > 1) {
                glEnable(GL_MULTISAMPLE);
            }
            else {
                glDisable(GL_MULTISAMPLE);
            }
        }
        #endif

        /* bind shader program */
        if (pip->shader->gl.prog != _sg.gl.cache.prog) {
            _sg.gl.cache.prog = pip->shader->gl.prog;
            glUseProgram(pip->shader->gl.prog);
        }
    }
    _SG_GL_CHECK_ERROR();
}

_SOKOL_PRIVATE void _sg_gl_apply_bindings(
    _sg_pipeline_t* pip,
    _sg_buffer_t** vbs, const int* vb_offsets, int num_vbs,
    _sg_buffer_t* ib, int ib_offset,
    _sg_image_t** vs_imgs, int num_vs_imgs,
    _sg_image_t** fs_imgs, int num_fs_imgs)
{
    SOKOL_ASSERT(pip);
    _SOKOL_UNUSED(num_fs_imgs);
    _SOKOL_UNUSED(num_vs_imgs);
    _SOKOL_UNUSED(num_vbs);
    _SG_GL_CHECK_ERROR();

    /* bind textures */
    _SG_GL_CHECK_ERROR();
    for (int stage_index = 0; stage_index < SG_NUM_SHADER_STAGES; stage_index++) {
        const _sg_shader_stage_t* stage = &pip->shader->cmn.stage[stage_index];
        const _sg_gl_shader_stage_t* gl_stage = &pip->shader->gl.stage[stage_index];
        _sg_image_t** imgs = (stage_index == SG_SHADERSTAGE_VS)? vs_imgs : fs_imgs;
        SOKOL_ASSERT(((stage_index == SG_SHADERSTAGE_VS)? num_vs_imgs : num_fs_imgs) == stage->num_images);
        for (int img_index = 0; img_index < stage->num_images; img_index++) {
            const _sg_gl_shader_image_t* gl_shd_img = &gl_stage->images[img_index];
            if (gl_shd_img->gl_tex_slot != -1) {
                _sg_image_t* img = imgs[img_index];
                const GLuint gl_tex = img->gl.tex[img->cmn.active_slot];
                SOKOL_ASSERT(img && img->gl.target);
                SOKOL_ASSERT((gl_shd_img->gl_tex_slot != -1) && gl_tex);
                _sg_gl_cache_bind_texture(gl_shd_img->gl_tex_slot, img->gl.target, gl_tex);
            }
        }
    }
    _SG_GL_CHECK_ERROR();

    /* index buffer (can be 0) */
    const GLuint gl_ib = ib ? ib->gl.buf[ib->cmn.active_slot] : 0;
    _sg_gl_cache_bind_buffer(GL_ELEMENT_ARRAY_BUFFER, gl_ib);
    _sg.gl.cache.cur_ib_offset = ib_offset;

    /* vertex attributes */
    for (GLuint attr_index = 0; attr_index < (GLuint)_sg.limits.max_vertex_attrs; attr_index++) {
        _sg_gl_attr_t* attr = &pip->gl.attrs[attr_index];
        _sg_gl_cache_attr_t* cache_attr = &_sg.gl.cache.attrs[attr_index];
        bool cache_attr_dirty = false;
        int vb_offset = 0;
        GLuint gl_vb = 0;
        if (attr->vb_index >= 0) {
            /* attribute is enabled */
            SOKOL_ASSERT(attr->vb_index < num_vbs);
            _sg_buffer_t* vb = vbs[attr->vb_index];
            SOKOL_ASSERT(vb);
            gl_vb = vb->gl.buf[vb->cmn.active_slot];
            vb_offset = vb_offsets[attr->vb_index] + attr->offset;
            if ((gl_vb != cache_attr->gl_vbuf) ||
                (attr->size != cache_attr->gl_attr.size) ||
                (attr->type != cache_attr->gl_attr.type) ||
                (attr->normalized != cache_attr->gl_attr.normalized) ||
                (attr->stride != cache_attr->gl_attr.stride) ||
                (vb_offset != cache_attr->gl_attr.offset) ||
                (cache_attr->gl_attr.divisor != attr->divisor))
            {
                _sg_gl_cache_bind_buffer(GL_ARRAY_BUFFER, gl_vb);
                glVertexAttribPointer(attr_index, attr->size, attr->type,
                    attr->normalized, attr->stride,
                    (const GLvoid*)(GLintptr)vb_offset);
                #if defined(_SOKOL_GL_INSTANCING_ENABLED)
                    if (_sg.features.instancing) {
                        glVertexAttribDivisor(attr_index, (GLuint)attr->divisor);
                    }
                #endif
                cache_attr_dirty = true;
            }
            if (cache_attr->gl_attr.vb_index == -1) {
                glEnableVertexAttribArray(attr_index);
                cache_attr_dirty = true;
            }
        }
        else {
            /* attribute is disabled */
            if (cache_attr->gl_attr.vb_index != -1) {
                glDisableVertexAttribArray(attr_index);
                cache_attr_dirty = true;
            }
        }
        if (cache_attr_dirty) {
            cache_attr->gl_attr = *attr;
            cache_attr->gl_attr.offset = vb_offset;
            cache_attr->gl_vbuf = gl_vb;
        }
    }
    _SG_GL_CHECK_ERROR();
}

_SOKOL_PRIVATE void _sg_gl_apply_uniforms(sg_shader_stage stage_index, int ub_index, const sg_range* data) {
    SOKOL_ASSERT(_sg.gl.cache.cur_pipeline);
    SOKOL_ASSERT(_sg.gl.cache.cur_pipeline->slot.id == _sg.gl.cache.cur_pipeline_id.id);
    SOKOL_ASSERT(_sg.gl.cache.cur_pipeline->shader->slot.id == _sg.gl.cache.cur_pipeline->cmn.shader_id.id);
    SOKOL_ASSERT(_sg.gl.cache.cur_pipeline->shader->cmn.stage[stage_index].num_uniform_blocks > ub_index);
    SOKOL_ASSERT(_sg.gl.cache.cur_pipeline->shader->cmn.stage[stage_index].uniform_blocks[ub_index].size == data->size);
    const _sg_gl_shader_stage_t* gl_stage = &_sg.gl.cache.cur_pipeline->shader->gl.stage[stage_index];
    const _sg_gl_uniform_block_t* gl_ub = &gl_stage->uniform_blocks[ub_index];
    for (int u_index = 0; u_index < gl_ub->num_uniforms; u_index++) {
        const _sg_gl_uniform_t* u = &gl_ub->uniforms[u_index];
        SOKOL_ASSERT(u->type != SG_UNIFORMTYPE_INVALID);
        if (u->gl_loc == -1) {
            continue;
        }
        GLfloat* fptr = (GLfloat*) (((uint8_t*)data->ptr) + u->offset);
        GLint* iptr = (GLint*) (((uint8_t*)data->ptr) + u->offset);
        switch (u->type) {
            case SG_UNIFORMTYPE_INVALID:
                break;
            case SG_UNIFORMTYPE_FLOAT:
                glUniform1fv(u->gl_loc, u->count, fptr);
                break;
            case SG_UNIFORMTYPE_FLOAT2:
                glUniform2fv(u->gl_loc, u->count, fptr);
                break;
            case SG_UNIFORMTYPE_FLOAT3:
                glUniform3fv(u->gl_loc, u->count, fptr);
                break;
            case SG_UNIFORMTYPE_FLOAT4:
                glUniform4fv(u->gl_loc, u->count, fptr);
                break;
            case SG_UNIFORMTYPE_INT:
                glUniform1iv(u->gl_loc, u->count, iptr);
                break;
            case SG_UNIFORMTYPE_INT2:
                glUniform2iv(u->gl_loc, u->count, iptr);
                break;
            case SG_UNIFORMTYPE_INT3:
                glUniform3iv(u->gl_loc, u->count, iptr);
                break;
            case SG_UNIFORMTYPE_INT4:
                glUniform4iv(u->gl_loc, u->count, iptr);
                break;
            case SG_UNIFORMTYPE_MAT4:
                glUniformMatrix4fv(u->gl_loc, u->count, GL_FALSE, fptr);
                break;
            default:
                SOKOL_UNREACHABLE;
                break;
        }
    }
}

_SOKOL_PRIVATE void _sg_gl_draw(int base_element, int num_elements, int num_instances) {
    SOKOL_ASSERT(_sg.gl.cache.cur_pipeline);
    const GLenum i_type = _sg.gl.cache.cur_index_type;
    const GLenum p_type = _sg.gl.cache.cur_primitive_type;
    if (0 != i_type) {
        /* indexed rendering */
        const int i_size = (i_type == GL_UNSIGNED_SHORT) ? 2 : 4;
        const int ib_offset = _sg.gl.cache.cur_ib_offset;
        const GLvoid* indices = (const GLvoid*)(GLintptr)(base_element*i_size+ib_offset);
        if (_sg.gl.cache.cur_pipeline->cmn.use_instanced_draw) {
            if (_sg.features.instancing) {
                glDrawElementsInstanced(p_type, num_elements, i_type, indices, num_instances);
            }
        }
        else {
            glDrawElements(p_type, num_elements, i_type, indices);
        }
    }
    else {
        /* non-indexed rendering */
        if (_sg.gl.cache.cur_pipeline->cmn.use_instanced_draw) {
            if (_sg.features.instancing) {
                glDrawArraysInstanced(p_type, base_element, num_elements, num_instances);
            }
        }
        else {
            glDrawArrays(p_type, base_element, num_elements);
        }
    }
}

_SOKOL_PRIVATE void _sg_gl_commit(void) {
    SOKOL_ASSERT(!_sg.gl.in_pass);
    /* "soft" clear bindings (only those that are actually bound) */
    _sg_gl_cache_clear_buffer_bindings(false);
    _sg_gl_cache_clear_texture_bindings(false);
}

_SOKOL_PRIVATE void _sg_gl_update_buffer(_sg_buffer_t* buf, const sg_range* data) {
    SOKOL_ASSERT(buf && data && data->ptr && (data->size > 0));
    /* only one update per buffer per frame allowed */
    if (++buf->cmn.active_slot >= buf->cmn.num_slots) {
        buf->cmn.active_slot = 0;
    }
    GLenum gl_tgt = _sg_gl_buffer_target(buf->cmn.type);
    SOKOL_ASSERT(buf->cmn.active_slot < SG_NUM_INFLIGHT_FRAMES);
    GLuint gl_buf = buf->gl.buf[buf->cmn.active_slot];
    SOKOL_ASSERT(gl_buf);
    _SG_GL_CHECK_ERROR();
    _sg_gl_cache_store_buffer_binding(gl_tgt);
    _sg_gl_cache_bind_buffer(gl_tgt, gl_buf);
    glBufferSubData(gl_tgt, 0, (GLsizeiptr)data->size, data->ptr);
    _sg_gl_cache_restore_buffer_binding(gl_tgt);
    _SG_GL_CHECK_ERROR();
}

_SOKOL_PRIVATE int _sg_gl_append_buffer(_sg_buffer_t* buf, const sg_range* data, bool new_frame) {
    SOKOL_ASSERT(buf && data && data->ptr && (data->size > 0));
    if (new_frame) {
        if (++buf->cmn.active_slot >= buf->cmn.num_slots) {
            buf->cmn.active_slot = 0;
        }
    }
    GLenum gl_tgt = _sg_gl_buffer_target(buf->cmn.type);
    SOKOL_ASSERT(buf->cmn.active_slot < SG_NUM_INFLIGHT_FRAMES);
    GLuint gl_buf = buf->gl.buf[buf->cmn.active_slot];
    SOKOL_ASSERT(gl_buf);
    _SG_GL_CHECK_ERROR();
    _sg_gl_cache_store_buffer_binding(gl_tgt);
    _sg_gl_cache_bind_buffer(gl_tgt, gl_buf);
    glBufferSubData(gl_tgt, buf->cmn.append_pos, (GLsizeiptr)data->size, data->ptr);
    _sg_gl_cache_restore_buffer_binding(gl_tgt);
    _SG_GL_CHECK_ERROR();
    /* NOTE: this is a requirement from WebGPU, but we want identical behaviour across all backend */
    return _sg_roundup((int)data->size, 4);
}

_SOKOL_PRIVATE void _sg_gl_update_image(_sg_image_t* img, const sg_image_data* data) {
    SOKOL_ASSERT(img && data);
    /* only one update per image per frame allowed */
    if (++img->cmn.active_slot >= img->cmn.num_slots) {
        img->cmn.active_slot = 0;
    }
    SOKOL_ASSERT(img->cmn.active_slot < SG_NUM_INFLIGHT_FRAMES);
    SOKOL_ASSERT(0 != img->gl.tex[img->cmn.active_slot]);
    _sg_gl_cache_store_texture_binding(0);
    _sg_gl_cache_bind_texture(0, img->gl.target, img->gl.tex[img->cmn.active_slot]);
    const GLenum gl_img_format = _sg_gl_teximage_format(img->cmn.pixel_format);
    const GLenum gl_img_type = _sg_gl_teximage_type(img->cmn.pixel_format);
    const int num_faces = img->cmn.type == SG_IMAGETYPE_CUBE ? 6 : 1;
    const int num_mips = img->cmn.num_mipmaps;
    for (int face_index = 0; face_index < num_faces; face_index++) {
        for (int mip_index = 0; mip_index < num_mips; mip_index++) {
            GLenum gl_img_target = img->gl.target;
            if (SG_IMAGETYPE_CUBE == img->cmn.type) {
                gl_img_target = _sg_gl_cubeface_target(face_index);
            }
            const GLvoid* data_ptr = data->subimage[face_index][mip_index].ptr;
            int mip_width = img->cmn.width >> mip_index;
            if (mip_width == 0) {
                mip_width = 1;
            }
            int mip_height = img->cmn.height >> mip_index;
            if (mip_height == 0) {
                mip_height = 1;
            }
            if ((SG_IMAGETYPE_2D == img->cmn.type) || (SG_IMAGETYPE_CUBE == img->cmn.type)) {
                glTexSubImage2D(gl_img_target, mip_index,
                    0, 0,
                    mip_width, mip_height,
                    gl_img_format, gl_img_type,
                    data_ptr);
            }
            #if !defined(SOKOL_GLES2)
            else if (!_sg.gl.gles2 && ((SG_IMAGETYPE_3D == img->cmn.type) || (SG_IMAGETYPE_ARRAY == img->cmn.type))) {
                int mip_depth = img->cmn.num_slices >> mip_index;
                if (mip_depth == 0) {
                    mip_depth = 1;
                }
                glTexSubImage3D(gl_img_target, mip_index,
                    0, 0, 0,
                    mip_width, mip_height, mip_depth,
                    gl_img_format, gl_img_type,
                    data_ptr);

            }
            #endif
        }
    }
    _sg_gl_cache_restore_texture_binding(0);
}

/*== D3D11 BACKEND IMPLEMENTATION ============================================*/
#elif defined(SOKOL_D3D11)

#if defined(__cplusplus)
#define _sg_d3d11_AddRef(self) (self)->AddRef()
#else
#define _sg_d3d11_AddRef(self) (self)->lpVtbl->AddRef(self)
#endif

#if defined(__cplusplus)
#define _sg_d3d11_Release(self) (self)->Release()
#else
#define _sg_d3d11_Release(self) (self)->lpVtbl->Release(self)
#endif

/*-- D3D11 C/C++ wrappers ----------------------------------------------------*/
static inline HRESULT _sg_d3d11_CheckFormatSupport(ID3D11Device* self, DXGI_FORMAT Format, UINT* pFormatSupport) {
    #if defined(__cplusplus)
        return self->CheckFormatSupport(Format, pFormatSupport);
    #else
        return self->lpVtbl->CheckFormatSupport(self, Format, pFormatSupport);
    #endif
}

static inline void _sg_d3d11_OMSetRenderTargets(ID3D11DeviceContext* self, UINT NumViews, ID3D11RenderTargetView* const* ppRenderTargetViews, ID3D11DepthStencilView *pDepthStencilView) {
    #if defined(__cplusplus)
        self->OMSetRenderTargets(NumViews, ppRenderTargetViews, pDepthStencilView);
    #else
        self->lpVtbl->OMSetRenderTargets(self, NumViews, ppRenderTargetViews, pDepthStencilView);
    #endif
}

static inline void _sg_d3d11_RSSetState(ID3D11DeviceContext* self, ID3D11RasterizerState* pRasterizerState) {
    #if defined(__cplusplus)
        self->RSSetState(pRasterizerState);
    #else
        self->lpVtbl->RSSetState(self, pRasterizerState);
    #endif
}

static inline void _sg_d3d11_OMSetDepthStencilState(ID3D11DeviceContext* self, ID3D11DepthStencilState* pDepthStencilState, UINT StencilRef) {
    #if defined(__cplusplus)
        self->OMSetDepthStencilState(pDepthStencilState, StencilRef);
    #else
        self->lpVtbl->OMSetDepthStencilState(self, pDepthStencilState, StencilRef);
    #endif
}

static inline void _sg_d3d11_OMSetBlendState(ID3D11DeviceContext* self, ID3D11BlendState* pBlendState, const FLOAT BlendFactor[4], UINT SampleMask) {
    #if defined(__cplusplus)
        self->OMSetBlendState(pBlendState, BlendFactor, SampleMask);
    #else
        self->lpVtbl->OMSetBlendState(self, pBlendState, BlendFactor, SampleMask);
    #endif
}

static inline void _sg_d3d11_IASetVertexBuffers(ID3D11DeviceContext* self, UINT StartSlot, UINT NumBuffers, ID3D11Buffer* const* ppVertexBuffers, const UINT* pStrides, const UINT* pOffsets) {
    #if defined(__cplusplus)
        self->IASetVertexBuffers(StartSlot, NumBuffers, ppVertexBuffers, pStrides, pOffsets);
    #else
        self->lpVtbl->IASetVertexBuffers(self, StartSlot, NumBuffers, ppVertexBuffers, pStrides, pOffsets);
    #endif
}

static inline void _sg_d3d11_IASetIndexBuffer(ID3D11DeviceContext* self, ID3D11Buffer* pIndexBuffer, DXGI_FORMAT Format, UINT Offset) {
    #if defined(__cplusplus)
        self->IASetIndexBuffer(pIndexBuffer, Format, Offset);
    #else
        self->lpVtbl->IASetIndexBuffer(self, pIndexBuffer, Format, Offset);
    #endif
}

static inline void _sg_d3d11_IASetInputLayout(ID3D11DeviceContext* self, ID3D11InputLayout* pInputLayout) {
    #if defined(__cplusplus)
        self->IASetInputLayout(pInputLayout);
    #else
        self->lpVtbl->IASetInputLayout(self, pInputLayout);
    #endif
}

static inline void _sg_d3d11_VSSetShader(ID3D11DeviceContext* self, ID3D11VertexShader* pVertexShader, ID3D11ClassInstance* const* ppClassInstances, UINT NumClassInstances) {
    #if defined(__cplusplus)
        self->VSSetShader(pVertexShader, ppClassInstances, NumClassInstances);
    #else
        self->lpVtbl->VSSetShader(self, pVertexShader, ppClassInstances, NumClassInstances);
    #endif
}

static inline void _sg_d3d11_PSSetShader(ID3D11DeviceContext* self, ID3D11PixelShader* pPixelShader, ID3D11ClassInstance* const* ppClassInstances, UINT NumClassInstances) {
    #if defined(__cplusplus)
        self->PSSetShader(pPixelShader, ppClassInstances, NumClassInstances);
    #else
        self->lpVtbl->PSSetShader(self, pPixelShader, ppClassInstances, NumClassInstances);
    #endif
}

static inline void _sg_d3d11_VSSetConstantBuffers(ID3D11DeviceContext* self, UINT StartSlot, UINT NumBuffers, ID3D11Buffer* const* ppConstantBuffers) {
    #if defined(__cplusplus)
        self->VSSetConstantBuffers(StartSlot, NumBuffers, ppConstantBuffers);
    #else
        self->lpVtbl->VSSetConstantBuffers(self, StartSlot, NumBuffers, ppConstantBuffers);
    #endif
}

static inline void _sg_d3d11_PSSetConstantBuffers(ID3D11DeviceContext* self, UINT StartSlot, UINT NumBuffers, ID3D11Buffer* const* ppConstantBuffers) {
    #if defined(__cplusplus)
        self->PSSetConstantBuffers(StartSlot, NumBuffers, ppConstantBuffers);
    #else
        self->lpVtbl->PSSetConstantBuffers(self, StartSlot, NumBuffers, ppConstantBuffers);
    #endif
}

static inline void _sg_d3d11_VSSetShaderResources(ID3D11DeviceContext* self, UINT StartSlot, UINT NumViews, ID3D11ShaderResourceView* const* ppShaderResourceViews) {
    #if defined(__cplusplus)
        self->VSSetShaderResources(StartSlot, NumViews, ppShaderResourceViews);
    #else
        self->lpVtbl->VSSetShaderResources(self, StartSlot, NumViews, ppShaderResourceViews);
    #endif
}

static inline void _sg_d3d11_PSSetShaderResources(ID3D11DeviceContext* self, UINT StartSlot, UINT NumViews, ID3D11ShaderResourceView* const* ppShaderResourceViews) {
    #if defined(__cplusplus)
        self->PSSetShaderResources(StartSlot, NumViews, ppShaderResourceViews);
    #else
        self->lpVtbl->PSSetShaderResources(self, StartSlot, NumViews, ppShaderResourceViews);
    #endif
}

static inline void _sg_d3d11_VSSetSamplers(ID3D11DeviceContext* self, UINT StartSlot, UINT NumSamplers, ID3D11SamplerState* const* ppSamplers) {
    #if defined(__cplusplus)
        self->VSSetSamplers(StartSlot, NumSamplers, ppSamplers);
    #else
        self->lpVtbl->VSSetSamplers(self, StartSlot, NumSamplers, ppSamplers);
    #endif
}

static inline void _sg_d3d11_PSSetSamplers(ID3D11DeviceContext* self, UINT StartSlot, UINT NumSamplers, ID3D11SamplerState* const* ppSamplers) {
    #if defined(__cplusplus)
        self->PSSetSamplers(StartSlot, NumSamplers, ppSamplers);
    #else
        self->lpVtbl->PSSetSamplers(self, StartSlot, NumSamplers, ppSamplers);
    #endif
}

static inline HRESULT _sg_d3d11_CreateBuffer(ID3D11Device* self, const D3D11_BUFFER_DESC* pDesc, const D3D11_SUBRESOURCE_DATA* pInitialData, ID3D11Buffer** ppBuffer) {
    #if defined(__cplusplus)
        return self->CreateBuffer(pDesc, pInitialData, ppBuffer);
    #else
        return self->lpVtbl->CreateBuffer(self, pDesc, pInitialData, ppBuffer);
    #endif
}

static inline HRESULT _sg_d3d11_CreateTexture2D(ID3D11Device* self, const D3D11_TEXTURE2D_DESC* pDesc, const D3D11_SUBRESOURCE_DATA* pInitialData, ID3D11Texture2D** ppTexture2D) {
    #if defined(__cplusplus)
        return self->CreateTexture2D(pDesc, pInitialData, ppTexture2D);
    #else
        return self->lpVtbl->CreateTexture2D(self, pDesc, pInitialData, ppTexture2D);
    #endif
}

static inline HRESULT _sg_d3d11_CreateShaderResourceView(ID3D11Device* self, ID3D11Resource* pResource, const D3D11_SHADER_RESOURCE_VIEW_DESC* pDesc, ID3D11ShaderResourceView** ppSRView) {
    #if defined(__cplusplus)
        return self->CreateShaderResourceView(pResource, pDesc, ppSRView);
    #else
        return self->lpVtbl->CreateShaderResourceView(self, pResource, pDesc, ppSRView);
    #endif
}

static inline void _sg_d3d11_GetResource(ID3D11View* self, ID3D11Resource** ppResource) {
    #if defined(__cplusplus)
        self->GetResource(ppResource);
    #else
        self->lpVtbl->GetResource(self, ppResource);
    #endif
}

static inline HRESULT _sg_d3d11_CreateTexture3D(ID3D11Device* self, const D3D11_TEXTURE3D_DESC* pDesc, const D3D11_SUBRESOURCE_DATA* pInitialData, ID3D11Texture3D** ppTexture3D) {
    #if defined(__cplusplus)
        return self->CreateTexture3D(pDesc, pInitialData, ppTexture3D);
    #else
        return self->lpVtbl->CreateTexture3D(self, pDesc, pInitialData, ppTexture3D);
    #endif
}

static inline HRESULT _sg_d3d11_CreateSamplerState(ID3D11Device* self, const D3D11_SAMPLER_DESC* pSamplerDesc, ID3D11SamplerState** ppSamplerState) {
    #if defined(__cplusplus)
        return self->CreateSamplerState(pSamplerDesc, ppSamplerState);
    #else
        return self->lpVtbl->CreateSamplerState(self, pSamplerDesc, ppSamplerState);
    #endif
}

static inline LPVOID _sg_d3d11_GetBufferPointer(ID3D10Blob* self) {
    #if defined(__cplusplus)
        return self->GetBufferPointer();
    #else
        return self->lpVtbl->GetBufferPointer(self);
    #endif
}

static inline SIZE_T _sg_d3d11_GetBufferSize(ID3D10Blob* self) {
    #if defined(__cplusplus)
        return self->GetBufferSize();
    #else
        return self->lpVtbl->GetBufferSize(self);
    #endif
}

static inline HRESULT _sg_d3d11_CreateVertexShader(ID3D11Device* self, const void* pShaderBytecode, SIZE_T BytecodeLength, ID3D11ClassLinkage* pClassLinkage, ID3D11VertexShader** ppVertexShader) {
    #if defined(__cplusplus)
        return self->CreateVertexShader(pShaderBytecode, BytecodeLength, pClassLinkage, ppVertexShader);
    #else
        return self->lpVtbl->CreateVertexShader(self, pShaderBytecode, BytecodeLength, pClassLinkage, ppVertexShader);
    #endif
}

static inline HRESULT _sg_d3d11_CreatePixelShader(ID3D11Device* self, const void* pShaderBytecode, SIZE_T BytecodeLength, ID3D11ClassLinkage* pClassLinkage, ID3D11PixelShader** ppPixelShader) {
    #if defined(__cplusplus)
        return self->CreatePixelShader(pShaderBytecode, BytecodeLength, pClassLinkage, ppPixelShader);
    #else
        return self->lpVtbl->CreatePixelShader(self, pShaderBytecode, BytecodeLength, pClassLinkage, ppPixelShader);
    #endif
}

static inline HRESULT _sg_d3d11_CreateInputLayout(ID3D11Device* self, const D3D11_INPUT_ELEMENT_DESC* pInputElementDescs, UINT NumElements, const void* pShaderBytecodeWithInputSignature, SIZE_T BytecodeLength, ID3D11InputLayout **ppInputLayout) {
    #if defined(__cplusplus)
        return self->CreateInputLayout(pInputElementDescs, NumElements, pShaderBytecodeWithInputSignature, BytecodeLength, ppInputLayout);
    #else
        return self->lpVtbl->CreateInputLayout(self, pInputElementDescs, NumElements, pShaderBytecodeWithInputSignature, BytecodeLength, ppInputLayout);
    #endif
}

static inline HRESULT _sg_d3d11_CreateRasterizerState(ID3D11Device* self, const D3D11_RASTERIZER_DESC* pRasterizerDesc, ID3D11RasterizerState** ppRasterizerState) {
    #if defined(__cplusplus)
        return self->CreateRasterizerState(pRasterizerDesc, ppRasterizerState);
    #else
        return self->lpVtbl->CreateRasterizerState(self, pRasterizerDesc, ppRasterizerState);
    #endif
}

static inline HRESULT _sg_d3d11_CreateDepthStencilState(ID3D11Device* self, const D3D11_DEPTH_STENCIL_DESC* pDepthStencilDesc, ID3D11DepthStencilState** ppDepthStencilState) {
    #if defined(__cplusplus)
        return self->CreateDepthStencilState(pDepthStencilDesc, ppDepthStencilState);
    #else
        return self->lpVtbl->CreateDepthStencilState(self, pDepthStencilDesc, ppDepthStencilState);
    #endif
}

static inline HRESULT _sg_d3d11_CreateBlendState(ID3D11Device* self, const D3D11_BLEND_DESC* pBlendStateDesc, ID3D11BlendState** ppBlendState) {
    #if defined(__cplusplus)
        return self->CreateBlendState(pBlendStateDesc, ppBlendState);
    #else
        return self->lpVtbl->CreateBlendState(self, pBlendStateDesc, ppBlendState);
    #endif
}

static inline HRESULT _sg_d3d11_CreateRenderTargetView(ID3D11Device* self, ID3D11Resource *pResource, const D3D11_RENDER_TARGET_VIEW_DESC* pDesc, ID3D11RenderTargetView** ppRTView) {
    #if defined(__cplusplus)
        return self->CreateRenderTargetView(pResource, pDesc, ppRTView);
    #else
        return self->lpVtbl->CreateRenderTargetView(self, pResource, pDesc, ppRTView);
    #endif
}

static inline HRESULT _sg_d3d11_CreateDepthStencilView(ID3D11Device* self, ID3D11Resource* pResource, const D3D11_DEPTH_STENCIL_VIEW_DESC* pDesc, ID3D11DepthStencilView** ppDepthStencilView) {
    #if defined(__cplusplus)
        return self->CreateDepthStencilView(pResource, pDesc, ppDepthStencilView);
    #else
        return self->lpVtbl->CreateDepthStencilView(self, pResource, pDesc, ppDepthStencilView);
    #endif
}

static inline void _sg_d3d11_RSSetViewports(ID3D11DeviceContext* self, UINT NumViewports, const D3D11_VIEWPORT* pViewports) {
    #if defined(__cplusplus)
        self->RSSetViewports(NumViewports, pViewports);
    #else
        self->lpVtbl->RSSetViewports(self, NumViewports, pViewports);
    #endif
}

static inline void _sg_d3d11_RSSetScissorRects(ID3D11DeviceContext* self, UINT NumRects, const D3D11_RECT* pRects) {
    #if defined(__cplusplus)
        self->RSSetScissorRects(NumRects, pRects);
    #else
        self->lpVtbl->RSSetScissorRects(self, NumRects, pRects);
    #endif
}

static inline void _sg_d3d11_ClearRenderTargetView(ID3D11DeviceContext* self, ID3D11RenderTargetView* pRenderTargetView, const FLOAT ColorRGBA[4]) {
    #if defined(__cplusplus)
        self->ClearRenderTargetView(pRenderTargetView, ColorRGBA);
    #else
        self->lpVtbl->ClearRenderTargetView(self, pRenderTargetView, ColorRGBA);
    #endif
}

static inline void _sg_d3d11_ClearDepthStencilView(ID3D11DeviceContext* self, ID3D11DepthStencilView* pDepthStencilView, UINT ClearFlags, FLOAT Depth, UINT8 Stencil) {
    #if defined(__cplusplus)
        self->ClearDepthStencilView(pDepthStencilView, ClearFlags, Depth, Stencil);
    #else
        self->lpVtbl->ClearDepthStencilView(self, pDepthStencilView, ClearFlags, Depth, Stencil);
    #endif
}

static inline void _sg_d3d11_ResolveSubresource(ID3D11DeviceContext* self, ID3D11Resource* pDstResource, UINT DstSubresource, ID3D11Resource* pSrcResource, UINT SrcSubresource, DXGI_FORMAT Format) {
    #if defined(__cplusplus)
        self->ResolveSubresource(pDstResource, DstSubresource, pSrcResource, SrcSubresource, Format);
    #else
        self->lpVtbl->ResolveSubresource(self, pDstResource, DstSubresource, pSrcResource, SrcSubresource, Format);
    #endif
}

static inline void _sg_d3d11_IASetPrimitiveTopology(ID3D11DeviceContext* self, D3D11_PRIMITIVE_TOPOLOGY Topology) {
    #if defined(__cplusplus)
        self->IASetPrimitiveTopology(Topology);
    #else
        self->lpVtbl->IASetPrimitiveTopology(self, Topology);
    #endif
}

static inline void _sg_d3d11_UpdateSubresource(ID3D11DeviceContext* self, ID3D11Resource* pDstResource, UINT DstSubresource, const D3D11_BOX* pDstBox, const void* pSrcData, UINT SrcRowPitch, UINT SrcDepthPitch) {
    #if defined(__cplusplus)
        self->UpdateSubresource(pDstResource, DstSubresource, pDstBox, pSrcData, SrcRowPitch, SrcDepthPitch);
    #else
        self->lpVtbl->UpdateSubresource(self, pDstResource, DstSubresource, pDstBox, pSrcData, SrcRowPitch, SrcDepthPitch);
    #endif
}

static inline void _sg_d3d11_DrawIndexed(ID3D11DeviceContext* self, UINT IndexCount, UINT StartIndexLocation, INT  BaseVertexLocation) {
    #if defined(__cplusplus)
        self->DrawIndexed(IndexCount, StartIndexLocation, BaseVertexLocation);
    #else
        self->lpVtbl->DrawIndexed(self, IndexCount, StartIndexLocation, BaseVertexLocation);
    #endif
}

static inline void _sg_d3d11_DrawIndexedInstanced(ID3D11DeviceContext* self, UINT IndexCountPerInstance, UINT InstanceCount, UINT StartIndexLocation, INT BaseVertexLocation, UINT StartInstanceLocation) {
    #if defined(__cplusplus)
        self->DrawIndexedInstanced(IndexCountPerInstance, InstanceCount, StartIndexLocation, BaseVertexLocation, StartInstanceLocation);
    #else
        self->lpVtbl->DrawIndexedInstanced(self, IndexCountPerInstance, InstanceCount, StartIndexLocation, BaseVertexLocation, StartInstanceLocation);
    #endif
}

static inline void _sg_d3d11_Draw(ID3D11DeviceContext* self, UINT VertexCount, UINT StartVertexLocation) {
    #if defined(__cplusplus)
        self->Draw(VertexCount, StartVertexLocation);
    #else
        self->lpVtbl->Draw(self, VertexCount, StartVertexLocation);
    #endif
}

static inline void _sg_d3d11_DrawInstanced(ID3D11DeviceContext* self, UINT VertexCountPerInstance, UINT InstanceCount, UINT StartVertexLocation, UINT StartInstanceLocation) {
    #if defined(__cplusplus)
        self->DrawInstanced(VertexCountPerInstance, InstanceCount, StartVertexLocation, StartInstanceLocation);
    #else
        self->lpVtbl->DrawInstanced(self, VertexCountPerInstance, InstanceCount, StartVertexLocation, StartInstanceLocation);
    #endif
}

static inline HRESULT _sg_d3d11_Map(ID3D11DeviceContext* self, ID3D11Resource* pResource, UINT Subresource, D3D11_MAP MapType, UINT MapFlags, D3D11_MAPPED_SUBRESOURCE* pMappedResource) {
    #if defined(__cplusplus)
        return self->Map(pResource, Subresource, MapType, MapFlags, pMappedResource);
    #else
        return self->lpVtbl->Map(self, pResource, Subresource, MapType, MapFlags, pMappedResource);
    #endif
}

static inline void _sg_d3d11_Unmap(ID3D11DeviceContext* self, ID3D11Resource* pResource, UINT Subresource) {
    #if defined(__cplusplus)
        self->Unmap(pResource, Subresource);
    #else
        self->lpVtbl->Unmap(self, pResource, Subresource);
    #endif
}

static inline void _sg_d3d11_ClearState(ID3D11DeviceContext* self) {
    #if defined(__cplusplus)
        self->ClearState();
    #else
        self->lpVtbl->ClearState(self);
    #endif
}

/*-- enum translation functions ----------------------------------------------*/
_SOKOL_PRIVATE D3D11_USAGE _sg_d3d11_usage(sg_usage usg) {
    switch (usg) {
        case SG_USAGE_IMMUTABLE:
            return D3D11_USAGE_IMMUTABLE;
        case SG_USAGE_DYNAMIC:
        case SG_USAGE_STREAM:
            return D3D11_USAGE_DYNAMIC;
        default:
            SOKOL_UNREACHABLE;
            return (D3D11_USAGE) 0;
    }
}

_SOKOL_PRIVATE UINT _sg_d3d11_cpu_access_flags(sg_usage usg) {
    switch (usg) {
        case SG_USAGE_IMMUTABLE:
            return 0;
        case SG_USAGE_DYNAMIC:
        case SG_USAGE_STREAM:
            return D3D11_CPU_ACCESS_WRITE;
        default:
            SOKOL_UNREACHABLE;
            return 0;
    }
}

_SOKOL_PRIVATE DXGI_FORMAT _sg_d3d11_pixel_format(sg_pixel_format fmt) {
    switch (fmt) {
        case SG_PIXELFORMAT_R8:             return DXGI_FORMAT_R8_UNORM;
        case SG_PIXELFORMAT_R8SN:           return DXGI_FORMAT_R8_SNORM;
        case SG_PIXELFORMAT_R8UI:           return DXGI_FORMAT_R8_UINT;
        case SG_PIXELFORMAT_R8SI:           return DXGI_FORMAT_R8_SINT;
        case SG_PIXELFORMAT_R16:            return DXGI_FORMAT_R16_UNORM;
        case SG_PIXELFORMAT_R16SN:          return DXGI_FORMAT_R16_SNORM;
        case SG_PIXELFORMAT_R16UI:          return DXGI_FORMAT_R16_UINT;
        case SG_PIXELFORMAT_R16SI:          return DXGI_FORMAT_R16_SINT;
        case SG_PIXELFORMAT_R16F:           return DXGI_FORMAT_R16_FLOAT;
        case SG_PIXELFORMAT_RG8:            return DXGI_FORMAT_R8G8_UNORM;
        case SG_PIXELFORMAT_RG8SN:          return DXGI_FORMAT_R8G8_SNORM;
        case SG_PIXELFORMAT_RG8UI:          return DXGI_FORMAT_R8G8_UINT;
        case SG_PIXELFORMAT_RG8SI:          return DXGI_FORMAT_R8G8_SINT;
        case SG_PIXELFORMAT_R32UI:          return DXGI_FORMAT_R32_UINT;
        case SG_PIXELFORMAT_R32SI:          return DXGI_FORMAT_R32_SINT;
        case SG_PIXELFORMAT_R32F:           return DXGI_FORMAT_R32_FLOAT;
        case SG_PIXELFORMAT_RG16:           return DXGI_FORMAT_R16G16_UNORM;
        case SG_PIXELFORMAT_RG16SN:         return DXGI_FORMAT_R16G16_SNORM;
        case SG_PIXELFORMAT_RG16UI:         return DXGI_FORMAT_R16G16_UINT;
        case SG_PIXELFORMAT_RG16SI:         return DXGI_FORMAT_R16G16_SINT;
        case SG_PIXELFORMAT_RG16F:          return DXGI_FORMAT_R16G16_FLOAT;
        case SG_PIXELFORMAT_RGBA8:          return DXGI_FORMAT_R8G8B8A8_UNORM;
        case SG_PIXELFORMAT_RGBA8SN:        return DXGI_FORMAT_R8G8B8A8_SNORM;
        case SG_PIXELFORMAT_RGBA8UI:        return DXGI_FORMAT_R8G8B8A8_UINT;
        case SG_PIXELFORMAT_RGBA8SI:        return DXGI_FORMAT_R8G8B8A8_SINT;
        case SG_PIXELFORMAT_BGRA8:          return DXGI_FORMAT_B8G8R8A8_UNORM;
        case SG_PIXELFORMAT_RGB10A2:        return DXGI_FORMAT_R10G10B10A2_UNORM;
        case SG_PIXELFORMAT_RG11B10F:       return DXGI_FORMAT_R11G11B10_FLOAT;
        case SG_PIXELFORMAT_RGB9E5:         return DXGI_FORMAT_R9G9B9E5_SHAREDEXP;
        case SG_PIXELFORMAT_RG32UI:         return DXGI_FORMAT_R32G32_UINT;
        case SG_PIXELFORMAT_RG32SI:         return DXGI_FORMAT_R32G32_SINT;
        case SG_PIXELFORMAT_RG32F:          return DXGI_FORMAT_R32G32_FLOAT;
        case SG_PIXELFORMAT_RGBA16:         return DXGI_FORMAT_R16G16B16A16_UNORM;
        case SG_PIXELFORMAT_RGBA16SN:       return DXGI_FORMAT_R16G16B16A16_SNORM;
        case SG_PIXELFORMAT_RGBA16UI:       return DXGI_FORMAT_R16G16B16A16_UINT;
        case SG_PIXELFORMAT_RGBA16SI:       return DXGI_FORMAT_R16G16B16A16_SINT;
        case SG_PIXELFORMAT_RGBA16F:        return DXGI_FORMAT_R16G16B16A16_FLOAT;
        case SG_PIXELFORMAT_RGBA32UI:       return DXGI_FORMAT_R32G32B32A32_UINT;
        case SG_PIXELFORMAT_RGBA32SI:       return DXGI_FORMAT_R32G32B32A32_SINT;
        case SG_PIXELFORMAT_RGBA32F:        return DXGI_FORMAT_R32G32B32A32_FLOAT;
        case SG_PIXELFORMAT_DEPTH:          return DXGI_FORMAT_D32_FLOAT;
        case SG_PIXELFORMAT_DEPTH_STENCIL:  return DXGI_FORMAT_D24_UNORM_S8_UINT;
        case SG_PIXELFORMAT_BC1_RGBA:       return DXGI_FORMAT_BC1_UNORM;
        case SG_PIXELFORMAT_BC2_RGBA:       return DXGI_FORMAT_BC2_UNORM;
        case SG_PIXELFORMAT_BC3_RGBA:       return DXGI_FORMAT_BC3_UNORM;
        case SG_PIXELFORMAT_BC4_R:          return DXGI_FORMAT_BC4_UNORM;
        case SG_PIXELFORMAT_BC4_RSN:        return DXGI_FORMAT_BC4_SNORM;
        case SG_PIXELFORMAT_BC5_RG:         return DXGI_FORMAT_BC5_UNORM;
        case SG_PIXELFORMAT_BC5_RGSN:       return DXGI_FORMAT_BC5_SNORM;
        case SG_PIXELFORMAT_BC6H_RGBF:      return DXGI_FORMAT_BC6H_SF16;
        case SG_PIXELFORMAT_BC6H_RGBUF:     return DXGI_FORMAT_BC6H_UF16;
        case SG_PIXELFORMAT_BC7_RGBA:       return DXGI_FORMAT_BC7_UNORM;
        default:                            return DXGI_FORMAT_UNKNOWN;
    };
}

_SOKOL_PRIVATE D3D11_PRIMITIVE_TOPOLOGY _sg_d3d11_primitive_topology(sg_primitive_type prim_type) {
    switch (prim_type) {
        case SG_PRIMITIVETYPE_POINTS:           return D3D11_PRIMITIVE_TOPOLOGY_POINTLIST;
        case SG_PRIMITIVETYPE_LINES:            return D3D11_PRIMITIVE_TOPOLOGY_LINELIST;
        case SG_PRIMITIVETYPE_LINE_STRIP:       return D3D11_PRIMITIVE_TOPOLOGY_LINESTRIP;
        case SG_PRIMITIVETYPE_TRIANGLES:        return D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
        case SG_PRIMITIVETYPE_TRIANGLE_STRIP:   return D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP;
        default: SOKOL_UNREACHABLE; return (D3D11_PRIMITIVE_TOPOLOGY) 0;
    }
}

_SOKOL_PRIVATE DXGI_FORMAT _sg_d3d11_index_format(sg_index_type index_type) {
    switch (index_type) {
        case SG_INDEXTYPE_NONE:     return DXGI_FORMAT_UNKNOWN;
        case SG_INDEXTYPE_UINT16:   return DXGI_FORMAT_R16_UINT;
        case SG_INDEXTYPE_UINT32:   return DXGI_FORMAT_R32_UINT;
        default: SOKOL_UNREACHABLE; return (DXGI_FORMAT) 0;
    }
}

_SOKOL_PRIVATE D3D11_FILTER _sg_d3d11_filter(sg_filter min_f, sg_filter mag_f, uint32_t max_anisotropy) {
    if (max_anisotropy > 1) {
        return D3D11_FILTER_ANISOTROPIC;
    }
    else if (mag_f == SG_FILTER_NEAREST) {
        switch (min_f) {
            case SG_FILTER_NEAREST:
            case SG_FILTER_NEAREST_MIPMAP_NEAREST:
                return D3D11_FILTER_MIN_MAG_MIP_POINT;
            case SG_FILTER_LINEAR:
            case SG_FILTER_LINEAR_MIPMAP_NEAREST:
                return D3D11_FILTER_MIN_LINEAR_MAG_MIP_POINT;
            case SG_FILTER_NEAREST_MIPMAP_LINEAR:
                return D3D11_FILTER_MIN_MAG_POINT_MIP_LINEAR;
            case SG_FILTER_LINEAR_MIPMAP_LINEAR:
                return D3D11_FILTER_MIN_LINEAR_MAG_POINT_MIP_LINEAR;
            default:
                SOKOL_UNREACHABLE; break;
        }
    }
    else if (mag_f == SG_FILTER_LINEAR) {
        switch (min_f) {
            case SG_FILTER_NEAREST:
            case SG_FILTER_NEAREST_MIPMAP_NEAREST:
                return D3D11_FILTER_MIN_POINT_MAG_LINEAR_MIP_POINT;
            case SG_FILTER_LINEAR:
            case SG_FILTER_LINEAR_MIPMAP_NEAREST:
                return D3D11_FILTER_MIN_MAG_LINEAR_MIP_POINT;
            case SG_FILTER_NEAREST_MIPMAP_LINEAR:
                return D3D11_FILTER_MIN_POINT_MAG_MIP_LINEAR;
            case SG_FILTER_LINEAR_MIPMAP_LINEAR:
                return D3D11_FILTER_MIN_MAG_MIP_LINEAR;
            default:
                SOKOL_UNREACHABLE; break;
        }
    }
    /* invalid value for mag filter */
    SOKOL_UNREACHABLE;
    return D3D11_FILTER_MIN_MAG_MIP_POINT;
}

_SOKOL_PRIVATE D3D11_TEXTURE_ADDRESS_MODE _sg_d3d11_address_mode(sg_wrap m) {
    switch (m) {
        case SG_WRAP_REPEAT:            return D3D11_TEXTURE_ADDRESS_WRAP;
        case SG_WRAP_CLAMP_TO_EDGE:     return D3D11_TEXTURE_ADDRESS_CLAMP;
        case SG_WRAP_CLAMP_TO_BORDER:   return D3D11_TEXTURE_ADDRESS_BORDER;
        case SG_WRAP_MIRRORED_REPEAT:   return D3D11_TEXTURE_ADDRESS_MIRROR;
        default: SOKOL_UNREACHABLE; return (D3D11_TEXTURE_ADDRESS_MODE) 0;
    }
}

_SOKOL_PRIVATE DXGI_FORMAT _sg_d3d11_vertex_format(sg_vertex_format fmt) {
    switch (fmt) {
        case SG_VERTEXFORMAT_FLOAT:     return DXGI_FORMAT_R32_FLOAT;
        case SG_VERTEXFORMAT_FLOAT2:    return DXGI_FORMAT_R32G32_FLOAT;
        case SG_VERTEXFORMAT_FLOAT3:    return DXGI_FORMAT_R32G32B32_FLOAT;
        case SG_VERTEXFORMAT_FLOAT4:    return DXGI_FORMAT_R32G32B32A32_FLOAT;
        case SG_VERTEXFORMAT_BYTE4:     return DXGI_FORMAT_R8G8B8A8_SINT;
        case SG_VERTEXFORMAT_BYTE4N:    return DXGI_FORMAT_R8G8B8A8_SNORM;
        case SG_VERTEXFORMAT_UBYTE4:    return DXGI_FORMAT_R8G8B8A8_UINT;
        case SG_VERTEXFORMAT_UBYTE4N:   return DXGI_FORMAT_R8G8B8A8_UNORM;
        case SG_VERTEXFORMAT_SHORT2:    return DXGI_FORMAT_R16G16_SINT;
        case SG_VERTEXFORMAT_SHORT2N:   return DXGI_FORMAT_R16G16_SNORM;
        case SG_VERTEXFORMAT_USHORT2N:  return DXGI_FORMAT_R16G16_UNORM;
        case SG_VERTEXFORMAT_SHORT4:    return DXGI_FORMAT_R16G16B16A16_SINT;
        case SG_VERTEXFORMAT_SHORT4N:   return DXGI_FORMAT_R16G16B16A16_SNORM;
        case SG_VERTEXFORMAT_USHORT4N:  return DXGI_FORMAT_R16G16B16A16_UNORM;
        case SG_VERTEXFORMAT_UINT10_N2: return DXGI_FORMAT_R10G10B10A2_UNORM;
        default: SOKOL_UNREACHABLE; return (DXGI_FORMAT) 0;
    }
}

_SOKOL_PRIVATE D3D11_INPUT_CLASSIFICATION _sg_d3d11_input_classification(sg_vertex_step step) {
    switch (step) {
        case SG_VERTEXSTEP_PER_VERTEX:      return D3D11_INPUT_PER_VERTEX_DATA;
        case SG_VERTEXSTEP_PER_INSTANCE:    return D3D11_INPUT_PER_INSTANCE_DATA;
        default: SOKOL_UNREACHABLE; return (D3D11_INPUT_CLASSIFICATION) 0;
    }
}

_SOKOL_PRIVATE D3D11_CULL_MODE _sg_d3d11_cull_mode(sg_cull_mode m) {
    switch (m) {
        case SG_CULLMODE_NONE:      return D3D11_CULL_NONE;
        case SG_CULLMODE_FRONT:     return D3D11_CULL_FRONT;
        case SG_CULLMODE_BACK:      return D3D11_CULL_BACK;
        default: SOKOL_UNREACHABLE; return (D3D11_CULL_MODE) 0;
    }
}

_SOKOL_PRIVATE D3D11_COMPARISON_FUNC _sg_d3d11_compare_func(sg_compare_func f) {
    switch (f) {
        case SG_COMPAREFUNC_NEVER:          return D3D11_COMPARISON_NEVER;
        case SG_COMPAREFUNC_LESS:           return D3D11_COMPARISON_LESS;
        case SG_COMPAREFUNC_EQUAL:          return D3D11_COMPARISON_EQUAL;
        case SG_COMPAREFUNC_LESS_EQUAL:     return D3D11_COMPARISON_LESS_EQUAL;
        case SG_COMPAREFUNC_GREATER:        return D3D11_COMPARISON_GREATER;
        case SG_COMPAREFUNC_NOT_EQUAL:      return D3D11_COMPARISON_NOT_EQUAL;
        case SG_COMPAREFUNC_GREATER_EQUAL:  return D3D11_COMPARISON_GREATER_EQUAL;
        case SG_COMPAREFUNC_ALWAYS:         return D3D11_COMPARISON_ALWAYS;
        default: SOKOL_UNREACHABLE; return (D3D11_COMPARISON_FUNC) 0;
    }
}

_SOKOL_PRIVATE D3D11_STENCIL_OP _sg_d3d11_stencil_op(sg_stencil_op op) {
    switch (op) {
        case SG_STENCILOP_KEEP:         return D3D11_STENCIL_OP_KEEP;
        case SG_STENCILOP_ZERO:         return D3D11_STENCIL_OP_ZERO;
        case SG_STENCILOP_REPLACE:      return D3D11_STENCIL_OP_REPLACE;
        case SG_STENCILOP_INCR_CLAMP:   return D3D11_STENCIL_OP_INCR_SAT;
        case SG_STENCILOP_DECR_CLAMP:   return D3D11_STENCIL_OP_DECR_SAT;
        case SG_STENCILOP_INVERT:       return D3D11_STENCIL_OP_INVERT;
        case SG_STENCILOP_INCR_WRAP:    return D3D11_STENCIL_OP_INCR;
        case SG_STENCILOP_DECR_WRAP:    return D3D11_STENCIL_OP_DECR;
        default: SOKOL_UNREACHABLE; return (D3D11_STENCIL_OP) 0;
    }
}

_SOKOL_PRIVATE D3D11_BLEND _sg_d3d11_blend_factor(sg_blend_factor f) {
    switch (f) {
        case SG_BLENDFACTOR_ZERO:                   return D3D11_BLEND_ZERO;
        case SG_BLENDFACTOR_ONE:                    return D3D11_BLEND_ONE;
        case SG_BLENDFACTOR_SRC_COLOR:              return D3D11_BLEND_SRC_COLOR;
        case SG_BLENDFACTOR_ONE_MINUS_SRC_COLOR:    return D3D11_BLEND_INV_SRC_COLOR;
        case SG_BLENDFACTOR_SRC_ALPHA:              return D3D11_BLEND_SRC_ALPHA;
        case SG_BLENDFACTOR_ONE_MINUS_SRC_ALPHA:    return D3D11_BLEND_INV_SRC_ALPHA;
        case SG_BLENDFACTOR_DST_COLOR:              return D3D11_BLEND_DEST_COLOR;
        case SG_BLENDFACTOR_ONE_MINUS_DST_COLOR:    return D3D11_BLEND_INV_DEST_COLOR;
        case SG_BLENDFACTOR_DST_ALPHA:              return D3D11_BLEND_DEST_ALPHA;
        case SG_BLENDFACTOR_ONE_MINUS_DST_ALPHA:    return D3D11_BLEND_INV_DEST_ALPHA;
        case SG_BLENDFACTOR_SRC_ALPHA_SATURATED:    return D3D11_BLEND_SRC_ALPHA_SAT;
        case SG_BLENDFACTOR_BLEND_COLOR:            return D3D11_BLEND_BLEND_FACTOR;
        case SG_BLENDFACTOR_ONE_MINUS_BLEND_COLOR:  return D3D11_BLEND_INV_BLEND_FACTOR;
        case SG_BLENDFACTOR_BLEND_ALPHA:            return D3D11_BLEND_BLEND_FACTOR;
        case SG_BLENDFACTOR_ONE_MINUS_BLEND_ALPHA:  return D3D11_BLEND_INV_BLEND_FACTOR;
        default: SOKOL_UNREACHABLE; return (D3D11_BLEND) 0;
    }
}

_SOKOL_PRIVATE D3D11_BLEND_OP _sg_d3d11_blend_op(sg_blend_op op) {
    switch (op) {
        case SG_BLENDOP_ADD:                return D3D11_BLEND_OP_ADD;
        case SG_BLENDOP_SUBTRACT:           return D3D11_BLEND_OP_SUBTRACT;
        case SG_BLENDOP_REVERSE_SUBTRACT:   return D3D11_BLEND_OP_REV_SUBTRACT;
        default: SOKOL_UNREACHABLE; return (D3D11_BLEND_OP) 0;
    }
}

_SOKOL_PRIVATE UINT8 _sg_d3d11_color_write_mask(sg_color_mask m) {
    UINT8 res = 0;
    if (m & SG_COLORMASK_R) {
        res |= D3D11_COLOR_WRITE_ENABLE_RED;
    }
    if (m & SG_COLORMASK_G) {
        res |= D3D11_COLOR_WRITE_ENABLE_GREEN;
    }
    if (m & SG_COLORMASK_B) {
        res |= D3D11_COLOR_WRITE_ENABLE_BLUE;
    }
    if (m & SG_COLORMASK_A) {
        res |= D3D11_COLOR_WRITE_ENABLE_ALPHA;
    }
    return res;
}

/* see: https://docs.microsoft.com/en-us/windows/win32/direct3d11/overviews-direct3d-11-resources-limits#resource-limits-for-feature-level-11-hardware */
_SOKOL_PRIVATE void _sg_d3d11_init_caps(void) {
    _sg.backend = SG_BACKEND_D3D11;

    _sg.features.instancing = true;
    _sg.features.origin_top_left = true;
    _sg.features.multiple_render_targets = true;
    _sg.features.msaa_render_targets = true;
    _sg.features.imagetype_3d = true;
    _sg.features.imagetype_array = true;
    _sg.features.image_clamp_to_border = true;
    _sg.features.mrt_independent_blend_state = true;
    _sg.features.mrt_independent_write_mask = true;

    _sg.limits.max_image_size_2d = 16 * 1024;
    _sg.limits.max_image_size_cube = 16 * 1024;
    _sg.limits.max_image_size_3d = 2 * 1024;
    _sg.limits.max_image_size_array = 16 * 1024;
    _sg.limits.max_image_array_layers = 2 * 1024;
    _sg.limits.max_vertex_attrs = SG_MAX_VERTEX_ATTRIBUTES;

    /* see: https://docs.microsoft.com/en-us/windows/win32/api/d3d11/ne-d3d11-d3d11_format_support */
    for (int fmt = (SG_PIXELFORMAT_NONE+1); fmt < _SG_PIXELFORMAT_NUM; fmt++) {
        UINT dxgi_fmt_caps = 0;
        const DXGI_FORMAT dxgi_fmt = _sg_d3d11_pixel_format((sg_pixel_format)fmt);
        if (dxgi_fmt != DXGI_FORMAT_UNKNOWN) {
            HRESULT hr = _sg_d3d11_CheckFormatSupport(_sg.d3d11.dev, dxgi_fmt, &dxgi_fmt_caps);
            SOKOL_ASSERT(SUCCEEDED(hr) || (E_FAIL == hr));
            if (!SUCCEEDED(hr)) {
                dxgi_fmt_caps = 0;
            }
        }
        sg_pixelformat_info* info = &_sg.formats[fmt];
        info->sample = 0 != (dxgi_fmt_caps & D3D11_FORMAT_SUPPORT_TEXTURE2D);
        info->filter = 0 != (dxgi_fmt_caps & D3D11_FORMAT_SUPPORT_SHADER_SAMPLE);
        info->render = 0 != (dxgi_fmt_caps & D3D11_FORMAT_SUPPORT_RENDER_TARGET);
        info->blend  = 0 != (dxgi_fmt_caps & D3D11_FORMAT_SUPPORT_BLENDABLE);
        info->msaa   = 0 != (dxgi_fmt_caps & D3D11_FORMAT_SUPPORT_MULTISAMPLE_RENDERTARGET);
        info->depth  = 0 != (dxgi_fmt_caps & D3D11_FORMAT_SUPPORT_DEPTH_STENCIL);
        if (info->depth) {
            info->render = true;
        }
    }
}

_SOKOL_PRIVATE void _sg_d3d11_setup_backend(const sg_desc* desc) {
    /* assume _sg.d3d11 already is zero-initialized */
    SOKOL_ASSERT(desc);
    SOKOL_ASSERT(desc->context.d3d11.device);
    SOKOL_ASSERT(desc->context.d3d11.device_context);
    SOKOL_ASSERT(desc->context.d3d11.render_target_view_cb || desc->context.d3d11.render_target_view_userdata_cb);
    SOKOL_ASSERT(desc->context.d3d11.depth_stencil_view_cb || desc->context.d3d11.depth_stencil_view_userdata_cb);
    _sg.d3d11.valid = true;
    _sg.d3d11.dev = (ID3D11Device*) desc->context.d3d11.device;
    _sg.d3d11.ctx = (ID3D11DeviceContext*) desc->context.d3d11.device_context;
    _sg.d3d11.rtv_cb = desc->context.d3d11.render_target_view_cb;
    _sg.d3d11.rtv_userdata_cb = desc->context.d3d11.render_target_view_userdata_cb;
    _sg.d3d11.dsv_cb = desc->context.d3d11.depth_stencil_view_cb;
    _sg.d3d11.dsv_userdata_cb = desc->context.d3d11.depth_stencil_view_userdata_cb;
    _sg.d3d11.user_data = desc->context.d3d11.user_data;
    _sg_d3d11_init_caps();
}

_SOKOL_PRIVATE void _sg_d3d11_discard_backend(void) {
    SOKOL_ASSERT(_sg.d3d11.valid);
    _sg.d3d11.valid = false;
}

_SOKOL_PRIVATE void _sg_d3d11_clear_state(void) {
    /* clear all the device context state, so that resource refs don't keep stuck in the d3d device context */
    _sg_d3d11_ClearState(_sg.d3d11.ctx);
}

_SOKOL_PRIVATE void _sg_d3d11_reset_state_cache(void) {
    /* just clear the d3d11 device context state */
    _sg_d3d11_clear_state();
}

_SOKOL_PRIVATE void _sg_d3d11_activate_context(_sg_context_t* ctx) {
    _SOKOL_UNUSED(ctx);
    _sg_d3d11_clear_state();
}

_SOKOL_PRIVATE sg_resource_state _sg_d3d11_create_context(_sg_context_t* ctx) {
    SOKOL_ASSERT(ctx);
    _SOKOL_UNUSED(ctx);
    return SG_RESOURCESTATE_VALID;
}

_SOKOL_PRIVATE void _sg_d3d11_discard_context(_sg_context_t* ctx) {
    SOKOL_ASSERT(ctx);
    _SOKOL_UNUSED(ctx);
    /* empty */
}

_SOKOL_PRIVATE sg_resource_state _sg_d3d11_create_buffer(_sg_buffer_t* buf, const sg_buffer_desc* desc) {
    SOKOL_ASSERT(buf && desc);
    SOKOL_ASSERT(!buf->d3d11.buf);
    _sg_buffer_common_init(&buf->cmn, desc);
    const bool injected = (0 != desc->d3d11_buffer);
    if (injected) {
        buf->d3d11.buf = (ID3D11Buffer*) desc->d3d11_buffer;
        _sg_d3d11_AddRef(buf->d3d11.buf);
    }
    else {
        D3D11_BUFFER_DESC d3d11_desc;
        _sg_clear(&d3d11_desc, sizeof(d3d11_desc));
        d3d11_desc.ByteWidth = (UINT)buf->cmn.size;
        d3d11_desc.Usage = _sg_d3d11_usage(buf->cmn.usage);
        d3d11_desc.BindFlags = buf->cmn.type == SG_BUFFERTYPE_VERTEXBUFFER ? D3D11_BIND_VERTEX_BUFFER : D3D11_BIND_INDEX_BUFFER;
        d3d11_desc.CPUAccessFlags = _sg_d3d11_cpu_access_flags(buf->cmn.usage);
        D3D11_SUBRESOURCE_DATA* init_data_ptr = 0;
        D3D11_SUBRESOURCE_DATA init_data;
        _sg_clear(&init_data, sizeof(init_data));
        if (buf->cmn.usage == SG_USAGE_IMMUTABLE) {
            SOKOL_ASSERT(desc->data.ptr);
            init_data.pSysMem = desc->data.ptr;
            init_data_ptr = &init_data;
        }
        HRESULT hr = _sg_d3d11_CreateBuffer(_sg.d3d11.dev, &d3d11_desc, init_data_ptr, &buf->d3d11.buf);
        if (!(SUCCEEDED(hr) && buf->d3d11.buf)) {
            SG_LOG("failed to create D3D11 buffer\n");
            return SG_RESOURCESTATE_FAILED;
        }
    }
    return SG_RESOURCESTATE_VALID;
}

_SOKOL_PRIVATE void _sg_d3d11_discard_buffer(_sg_buffer_t* buf) {
    SOKOL_ASSERT(buf);
    if (buf->d3d11.buf) {
        _sg_d3d11_Release(buf->d3d11.buf);
    }
}

_SOKOL_PRIVATE void _sg_d3d11_fill_subres_data(const _sg_image_t* img, const sg_image_data* data) {
    const int num_faces = (img->cmn.type == SG_IMAGETYPE_CUBE) ? 6:1;
    const int num_slices = (img->cmn.type == SG_IMAGETYPE_ARRAY) ? img->cmn.num_slices:1;
    int subres_index = 0;
    for (int face_index = 0; face_index < num_faces; face_index++) {
        for (int slice_index = 0; slice_index < num_slices; slice_index++) {
            for (int mip_index = 0; mip_index < img->cmn.num_mipmaps; mip_index++, subres_index++) {
                SOKOL_ASSERT(subres_index < (SG_MAX_MIPMAPS * SG_MAX_TEXTUREARRAY_LAYERS));
                D3D11_SUBRESOURCE_DATA* subres_data = &_sg.d3d11.subres_data[subres_index];
                const int mip_width = ((img->cmn.width>>mip_index)>0) ? img->cmn.width>>mip_index : 1;
                const int mip_height = ((img->cmn.height>>mip_index)>0) ? img->cmn.height>>mip_index : 1;
                const sg_range* subimg_data = &(data->subimage[face_index][mip_index]);
                const size_t slice_size = subimg_data->size / (size_t)num_slices;
                const size_t slice_offset = slice_size * (size_t)slice_index;
                const uint8_t* ptr = (const uint8_t*) subimg_data->ptr;
                subres_data->pSysMem = ptr + slice_offset;
                subres_data->SysMemPitch = (UINT)_sg_row_pitch(img->cmn.pixel_format, mip_width, 1);
                if (img->cmn.type == SG_IMAGETYPE_3D) {
                    /* FIXME? const int mip_depth = ((img->depth>>mip_index)>0) ? img->depth>>mip_index : 1; */
                    subres_data->SysMemSlicePitch = (UINT)_sg_surface_pitch(img->cmn.pixel_format, mip_width, mip_height, 1);
                }
                else {
                    subres_data->SysMemSlicePitch = 0;
                }
            }
        }
    }
}

_SOKOL_PRIVATE sg_resource_state _sg_d3d11_create_image(_sg_image_t* img, const sg_image_desc* desc) {
    SOKOL_ASSERT(img && desc);
    SOKOL_ASSERT(!img->d3d11.tex2d && !img->d3d11.tex3d && !img->d3d11.texds && !img->d3d11.texmsaa);
    SOKOL_ASSERT(!img->d3d11.srv && !img->d3d11.smp);
    HRESULT hr;

    _sg_image_common_init(&img->cmn, desc);
    const bool injected = (0 != desc->d3d11_texture) || (0 != desc->d3d11_shader_resource_view);
    const bool msaa = (img->cmn.sample_count > 1);
    img->d3d11.format = _sg_d3d11_pixel_format(img->cmn.pixel_format);

    /* special case depth-stencil buffer? */
    if (_sg_is_valid_rendertarget_depth_format(img->cmn.pixel_format)) {
        /* create only a depth-texture */
        SOKOL_ASSERT(!injected);
        if (img->d3d11.format == DXGI_FORMAT_UNKNOWN) {
            SG_LOG("trying to create a D3D11 depth-texture with unsupported pixel format\n");
            return SG_RESOURCESTATE_FAILED;
        }
        D3D11_TEXTURE2D_DESC d3d11_desc;
        _sg_clear(&d3d11_desc, sizeof(d3d11_desc));
        d3d11_desc.Width = (UINT)img->cmn.width;
        d3d11_desc.Height = (UINT)img->cmn.height;
        d3d11_desc.MipLevels = 1;
        d3d11_desc.ArraySize = 1;
        d3d11_desc.Format = img->d3d11.format;
        d3d11_desc.Usage = D3D11_USAGE_DEFAULT;
        d3d11_desc.BindFlags = D3D11_BIND_DEPTH_STENCIL;
        d3d11_desc.SampleDesc.Count = (UINT)img->cmn.sample_count;
        d3d11_desc.SampleDesc.Quality = (UINT) (msaa ? D3D11_STANDARD_MULTISAMPLE_PATTERN : 0);
        hr = _sg_d3d11_CreateTexture2D(_sg.d3d11.dev, &d3d11_desc, NULL, &img->d3d11.texds);
        if (!(SUCCEEDED(hr) && img->d3d11.texds)) {
            SG_LOG("failed to create D3D11 texture 2D\n");
            return SG_RESOURCESTATE_FAILED;
        }
    }
    else {
        /* create (or inject) color texture and shader-resource-view */

        /* prepare initial content pointers */
        D3D11_SUBRESOURCE_DATA* init_data = 0;
        if (!injected && (img->cmn.usage == SG_USAGE_IMMUTABLE) && !img->cmn.render_target) {
            _sg_d3d11_fill_subres_data(img, &desc->data);
            init_data = _sg.d3d11.subres_data;
        }
        if (img->cmn.type != SG_IMAGETYPE_3D) {
            /* 2D-, cube- or array-texture */
            /* if this is an MSAA render target, the following texture will be the 'resolve-texture' */

            /* first check for injected texture and/or resource view */
            if (injected) {
                img->d3d11.tex2d = (ID3D11Texture2D*) desc->d3d11_texture;
                img->d3d11.srv = (ID3D11ShaderResourceView*) desc->d3d11_shader_resource_view;
                if (img->d3d11.tex2d) {
                    _sg_d3d11_AddRef(img->d3d11.tex2d);
                }
                else {
                    /* if only a shader-resource-view was provided, but no texture, lookup
                       the texture from the shader-resource-view, this also bumps the refcount
                    */
                    SOKOL_ASSERT(img->d3d11.srv);
                    _sg_d3d11_GetResource((ID3D11View*)img->d3d11.srv, (ID3D11Resource**)&img->d3d11.tex2d);
                    SOKOL_ASSERT(img->d3d11.tex2d);
                }
                if (img->d3d11.srv) {
                    _sg_d3d11_AddRef(img->d3d11.srv);
                }
            }

            /* if not injected, create texture */
            if (0 == img->d3d11.tex2d) {
                D3D11_TEXTURE2D_DESC d3d11_tex_desc;
                _sg_clear(&d3d11_tex_desc, sizeof(d3d11_tex_desc));
                d3d11_tex_desc.Width = (UINT)img->cmn.width;
                d3d11_tex_desc.Height = (UINT)img->cmn.height;
                d3d11_tex_desc.MipLevels = (UINT)img->cmn.num_mipmaps;
                switch (img->cmn.type) {
                    case SG_IMAGETYPE_ARRAY:    d3d11_tex_desc.ArraySize = (UINT)img->cmn.num_slices; break;
                    case SG_IMAGETYPE_CUBE:     d3d11_tex_desc.ArraySize = 6; break;
                    default:                    d3d11_tex_desc.ArraySize = 1; break;
                }
                d3d11_tex_desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
                d3d11_tex_desc.Format = img->d3d11.format;
                if (img->cmn.render_target) {
                    d3d11_tex_desc.Usage = D3D11_USAGE_DEFAULT;
                    if (!msaa) {
                        d3d11_tex_desc.BindFlags |= D3D11_BIND_RENDER_TARGET;
                    }
                    d3d11_tex_desc.CPUAccessFlags = 0;
                }
                else {
                    d3d11_tex_desc.Usage = _sg_d3d11_usage(img->cmn.usage);
                    d3d11_tex_desc.CPUAccessFlags = _sg_d3d11_cpu_access_flags(img->cmn.usage);
                }
                if (img->d3d11.format == DXGI_FORMAT_UNKNOWN) {
                    /* trying to create a texture format that's not supported by D3D */
                    SG_LOG("trying to create a D3D11 texture with unsupported pixel format\n");
                    return SG_RESOURCESTATE_FAILED;
                }
                d3d11_tex_desc.SampleDesc.Count = 1;
                d3d11_tex_desc.SampleDesc.Quality = 0;
                d3d11_tex_desc.MiscFlags = (img->cmn.type == SG_IMAGETYPE_CUBE) ? D3D11_RESOURCE_MISC_TEXTURECUBE : 0;

                hr = _sg_d3d11_CreateTexture2D(_sg.d3d11.dev, &d3d11_tex_desc, init_data, &img->d3d11.tex2d);
                if (!(SUCCEEDED(hr) && img->d3d11.tex2d)) {
                    SG_LOG("failed to create D3D11 texture 2D\n");
                    return SG_RESOURCESTATE_FAILED;
                }
            }

            /* ...and similar, if not injected, create shader-resource-view */
            if (0 == img->d3d11.srv) {
                D3D11_SHADER_RESOURCE_VIEW_DESC d3d11_srv_desc;
                _sg_clear(&d3d11_srv_desc, sizeof(d3d11_srv_desc));
                d3d11_srv_desc.Format = img->d3d11.format;
                switch (img->cmn.type) {
                    case SG_IMAGETYPE_2D:
                        d3d11_srv_desc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
                        d3d11_srv_desc.Texture2D.MipLevels = (UINT)img->cmn.num_mipmaps;
                        break;
                    case SG_IMAGETYPE_CUBE:
                        d3d11_srv_desc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURECUBE;
                        d3d11_srv_desc.TextureCube.MipLevels = (UINT)img->cmn.num_mipmaps;
                        break;
                    case SG_IMAGETYPE_ARRAY:
                        d3d11_srv_desc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2DARRAY;
                        d3d11_srv_desc.Texture2DArray.MipLevels = (UINT)img->cmn.num_mipmaps;
                        d3d11_srv_desc.Texture2DArray.ArraySize = (UINT)img->cmn.num_slices;
                        break;
                    default:
                        SOKOL_UNREACHABLE; break;
                }
                hr = _sg_d3d11_CreateShaderResourceView(_sg.d3d11.dev, (ID3D11Resource*)img->d3d11.tex2d, &d3d11_srv_desc, &img->d3d11.srv);
                if (!(SUCCEEDED(hr) && img->d3d11.srv)) {
                    SG_LOG("failed to create D3D11 resource view\n");
                    return SG_RESOURCESTATE_FAILED;
                }
            }
        }
        else {
            /* 3D texture - same procedure, first check if injected, than create non-injected */
            if (injected) {
                img->d3d11.tex3d = (ID3D11Texture3D*) desc->d3d11_texture;
                img->d3d11.srv = (ID3D11ShaderResourceView*) desc->d3d11_shader_resource_view;
                if (img->d3d11.tex3d) {
                    _sg_d3d11_AddRef(img->d3d11.tex3d);
                }
                else {
                    SOKOL_ASSERT(img->d3d11.srv);
                    _sg_d3d11_GetResource((ID3D11View*)img->d3d11.srv, (ID3D11Resource**)&img->d3d11.tex3d);
                    SOKOL_ASSERT(img->d3d11.tex3d);
                }
                if (img->d3d11.srv) {
                    _sg_d3d11_AddRef(img->d3d11.srv);
                }
            }

            if (0 == img->d3d11.tex3d) {
                D3D11_TEXTURE3D_DESC d3d11_tex_desc;
                _sg_clear(&d3d11_tex_desc, sizeof(d3d11_tex_desc));
                d3d11_tex_desc.Width = (UINT)img->cmn.width;
                d3d11_tex_desc.Height = (UINT)img->cmn.height;
                d3d11_tex_desc.Depth = (UINT)img->cmn.num_slices;
                d3d11_tex_desc.MipLevels = (UINT)img->cmn.num_mipmaps;
                d3d11_tex_desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
                d3d11_tex_desc.Format = img->d3d11.format;
                if (img->cmn.render_target) {
                    d3d11_tex_desc.Usage = D3D11_USAGE_DEFAULT;
                    if (!msaa) {
                        d3d11_tex_desc.BindFlags |= D3D11_BIND_RENDER_TARGET;
                    }
                    d3d11_tex_desc.CPUAccessFlags = 0;
                }
                else {
                    d3d11_tex_desc.Usage = _sg_d3d11_usage(img->cmn.usage);
                    d3d11_tex_desc.CPUAccessFlags = _sg_d3d11_cpu_access_flags(img->cmn.usage);
                }
                if (img->d3d11.format == DXGI_FORMAT_UNKNOWN) {
                    /* trying to create a texture format that's not supported by D3D */
                    SG_LOG("trying to create a D3D11 texture with unsupported pixel format\n");
                    return SG_RESOURCESTATE_FAILED;
                }
                hr = _sg_d3d11_CreateTexture3D(_sg.d3d11.dev, &d3d11_tex_desc, init_data, &img->d3d11.tex3d);
                if (!(SUCCEEDED(hr) && img->d3d11.tex3d)) {
                    SG_LOG("failed to create D3D11 texture 3D\n");
                    return SG_RESOURCESTATE_FAILED;
                }
            }

            if (0 == img->d3d11.srv) {
                D3D11_SHADER_RESOURCE_VIEW_DESC d3d11_srv_desc;
                _sg_clear(&d3d11_srv_desc, sizeof(d3d11_srv_desc));
                d3d11_srv_desc.Format = img->d3d11.format;
                d3d11_srv_desc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE3D;
                d3d11_srv_desc.Texture3D.MipLevels = (UINT)img->cmn.num_mipmaps;
                hr = _sg_d3d11_CreateShaderResourceView(_sg.d3d11.dev, (ID3D11Resource*)img->d3d11.tex3d, &d3d11_srv_desc, &img->d3d11.srv);
                if (!(SUCCEEDED(hr) && img->d3d11.srv)) {
                    SG_LOG("failed to create D3D11 resource view\n");
                    return SG_RESOURCESTATE_FAILED;
                }
            }
        }

        /* also need to create a separate MSAA render target texture? */
        if (msaa) {
            D3D11_TEXTURE2D_DESC d3d11_tex_desc;
            _sg_clear(&d3d11_tex_desc, sizeof(d3d11_tex_desc));
            d3d11_tex_desc.Width = (UINT)img->cmn.width;
            d3d11_tex_desc.Height = (UINT)img->cmn.height;
            d3d11_tex_desc.MipLevels = 1;
            d3d11_tex_desc.ArraySize = 1;
            d3d11_tex_desc.Format = img->d3d11.format;
            d3d11_tex_desc.Usage = D3D11_USAGE_DEFAULT;
            d3d11_tex_desc.BindFlags = D3D11_BIND_RENDER_TARGET;
            d3d11_tex_desc.CPUAccessFlags = 0;
            d3d11_tex_desc.SampleDesc.Count = (UINT)img->cmn.sample_count;
            d3d11_tex_desc.SampleDesc.Quality = (UINT)D3D11_STANDARD_MULTISAMPLE_PATTERN;
            hr = _sg_d3d11_CreateTexture2D(_sg.d3d11.dev, &d3d11_tex_desc, NULL, &img->d3d11.texmsaa);
            if (!(SUCCEEDED(hr) && img->d3d11.texmsaa)) {
                SG_LOG("failed to create D3D11 texture 2D\n");
                return SG_RESOURCESTATE_FAILED;
            }
        }

        /* sampler state object, note D3D11 implements an internal shared-pool for sampler objects */
        D3D11_SAMPLER_DESC d3d11_smp_desc;
        _sg_clear(&d3d11_smp_desc, sizeof(d3d11_smp_desc));
        d3d11_smp_desc.Filter = _sg_d3d11_filter(img->cmn.min_filter, img->cmn.mag_filter, img->cmn.max_anisotropy);
        d3d11_smp_desc.AddressU = _sg_d3d11_address_mode(img->cmn.wrap_u);
        d3d11_smp_desc.AddressV = _sg_d3d11_address_mode(img->cmn.wrap_v);
        d3d11_smp_desc.AddressW = _sg_d3d11_address_mode(img->cmn.wrap_w);
        switch (img->cmn.border_color) {
            case SG_BORDERCOLOR_TRANSPARENT_BLACK:
                /* all 0.0f */
                break;
            case SG_BORDERCOLOR_OPAQUE_WHITE:
                for (int i = 0; i < 4; i++) {
                    d3d11_smp_desc.BorderColor[i] = 1.0f;
                }
                break;
            default:
                /* opaque black */
                d3d11_smp_desc.BorderColor[3] = 1.0f;
                break;
        }
        d3d11_smp_desc.MaxAnisotropy = img->cmn.max_anisotropy;
        d3d11_smp_desc.ComparisonFunc = D3D11_COMPARISON_NEVER;
        d3d11_smp_desc.MinLOD = desc->min_lod;
        d3d11_smp_desc.MaxLOD = desc->max_lod;
        hr = _sg_d3d11_CreateSamplerState(_sg.d3d11.dev, &d3d11_smp_desc, &img->d3d11.smp);
        if (!(SUCCEEDED(hr) && img->d3d11.smp)) {
            SG_LOG("failed to create D3D11 sampler state\n");
            return SG_RESOURCESTATE_FAILED;
        }
    }
    return SG_RESOURCESTATE_VALID;
}

_SOKOL_PRIVATE void _sg_d3d11_discard_image(_sg_image_t* img) {
    SOKOL_ASSERT(img);
    if (img->d3d11.tex2d) {
        _sg_d3d11_Release(img->d3d11.tex2d);
    }
    if (img->d3d11.tex3d) {
        _sg_d3d11_Release(img->d3d11.tex3d);
    }
    if (img->d3d11.texds) {
        _sg_d3d11_Release(img->d3d11.texds);
    }
    if (img->d3d11.texmsaa) {
        _sg_d3d11_Release(img->d3d11.texmsaa);
    }
    if (img->d3d11.srv) {
        _sg_d3d11_Release(img->d3d11.srv);
    }
    if (img->d3d11.smp) {
        _sg_d3d11_Release(img->d3d11.smp);
    }
}

_SOKOL_PRIVATE bool _sg_d3d11_load_d3dcompiler_dll(void) {
    /* on UWP, don't do anything (not tested) */
    #if (defined(WINAPI_FAMILY_PARTITION) && !WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP))
        return true;
    #else
        /* load DLL on demand */
        if ((0 == _sg.d3d11.d3dcompiler_dll) && !_sg.d3d11.d3dcompiler_dll_load_failed) {
            _sg.d3d11.d3dcompiler_dll = LoadLibraryA("d3dcompiler_47.dll");
            if (0 == _sg.d3d11.d3dcompiler_dll) {
                /* don't attempt to load missing DLL in the future */
                SG_LOG("failed to load d3dcompiler_47.dll!\n");
                _sg.d3d11.d3dcompiler_dll_load_failed = true;
                return false;
            }
            /* look up function pointers */
            _sg.d3d11.D3DCompile_func = (pD3DCompile)(void*) GetProcAddress(_sg.d3d11.d3dcompiler_dll, "D3DCompile");
            SOKOL_ASSERT(_sg.d3d11.D3DCompile_func);
        }
        return 0 != _sg.d3d11.d3dcompiler_dll;
    #endif
}

#if (defined(WINAPI_FAMILY_PARTITION) && !WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP))
#define _sg_d3d11_D3DCompile D3DCompile
#else
#define _sg_d3d11_D3DCompile _sg.d3d11.D3DCompile_func
#endif

_SOKOL_PRIVATE ID3DBlob* _sg_d3d11_compile_shader(const sg_shader_stage_desc* stage_desc) {
    if (!_sg_d3d11_load_d3dcompiler_dll()) {
        return NULL;
    }
    SOKOL_ASSERT(stage_desc->d3d11_target);
    ID3DBlob* output = NULL;
    ID3DBlob* errors_or_warnings = NULL;
    HRESULT hr = _sg_d3d11_D3DCompile(
        stage_desc->source,             /* pSrcData */
        strlen(stage_desc->source),     /* SrcDataSize */
        NULL,                           /* pSourceName */
        NULL,                           /* pDefines */
        NULL,                           /* pInclude */
        stage_desc->entry ? stage_desc->entry : "main",     /* pEntryPoint */
        stage_desc->d3d11_target,       /* pTarget (vs_5_0 or ps_5_0) */
        D3DCOMPILE_PACK_MATRIX_COLUMN_MAJOR | D3DCOMPILE_OPTIMIZATION_LEVEL3,   /* Flags1 */
        0,          /* Flags2 */
        &output,    /* ppCode */
        &errors_or_warnings);   /* ppErrorMsgs */
    if (errors_or_warnings) {
        SG_LOG((LPCSTR)_sg_d3d11_GetBufferPointer(errors_or_warnings));
        _sg_d3d11_Release(errors_or_warnings); errors_or_warnings = NULL;
    }
    if (FAILED(hr)) {
        /* just in case, usually output is NULL here */
        if (output) {
            _sg_d3d11_Release(output);
            output = NULL;
        }
    }
    return output;
}

_SOKOL_PRIVATE sg_resource_state _sg_d3d11_create_shader(_sg_shader_t* shd, const sg_shader_desc* desc) {
    SOKOL_ASSERT(shd && desc);
    SOKOL_ASSERT(!shd->d3d11.vs && !shd->d3d11.fs && !shd->d3d11.vs_blob);
    HRESULT hr;

    _sg_shader_common_init(&shd->cmn, desc);

    /* copy vertex attribute semantic names and indices */
    for (int i = 0; i < SG_MAX_VERTEX_ATTRIBUTES; i++) {
        _sg_strcpy(&shd->d3d11.attrs[i].sem_name, desc->attrs[i].sem_name);
        shd->d3d11.attrs[i].sem_index = desc->attrs[i].sem_index;
    }

    /* shader stage uniform blocks and image slots */
    for (int stage_index = 0; stage_index < SG_NUM_SHADER_STAGES; stage_index++) {
        _sg_shader_stage_t* cmn_stage = &shd->cmn.stage[stage_index];
        _sg_d3d11_shader_stage_t* d3d11_stage = &shd->d3d11.stage[stage_index];
        for (int ub_index = 0; ub_index < cmn_stage->num_uniform_blocks; ub_index++) {
            const _sg_uniform_block_t* ub = &cmn_stage->uniform_blocks[ub_index];

            /* create a D3D constant buffer for each uniform block */
            SOKOL_ASSERT(0 == d3d11_stage->cbufs[ub_index]);
            D3D11_BUFFER_DESC cb_desc;
            _sg_clear(&cb_desc, sizeof(cb_desc));
            cb_desc.ByteWidth = (UINT)_sg_roundup((int)ub->size, 16);
            cb_desc.Usage = D3D11_USAGE_DEFAULT;
            cb_desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
            hr = _sg_d3d11_CreateBuffer(_sg.d3d11.dev, &cb_desc, NULL, &d3d11_stage->cbufs[ub_index]);
            if (!(SUCCEEDED(hr) && d3d11_stage->cbufs[ub_index])) {
                SG_LOG("failed to create D3D11 buffer\n");
                return SG_RESOURCESTATE_FAILED;
            }
        }
    }

    const void* vs_ptr = 0, *fs_ptr = 0;
    SIZE_T vs_length = 0, fs_length = 0;
    ID3DBlob* vs_blob = 0, *fs_blob = 0;
    if (desc->vs.bytecode.ptr && desc->fs.bytecode.ptr) {
        /* create from shader byte code */
        vs_ptr = desc->vs.bytecode.ptr;
        fs_ptr = desc->fs.bytecode.ptr;
        vs_length = desc->vs.bytecode.size;
        fs_length = desc->fs.bytecode.size;
    }
    else {
        /* compile from shader source code */
        vs_blob = _sg_d3d11_compile_shader(&desc->vs);
        fs_blob = _sg_d3d11_compile_shader(&desc->fs);
        if (vs_blob && fs_blob) {
            vs_ptr = _sg_d3d11_GetBufferPointer(vs_blob);
            vs_length = _sg_d3d11_GetBufferSize(vs_blob);
            fs_ptr = _sg_d3d11_GetBufferPointer(fs_blob);
            fs_length = _sg_d3d11_GetBufferSize(fs_blob);
        }
    }
    sg_resource_state result = SG_RESOURCESTATE_FAILED;
    if (vs_ptr && fs_ptr && (vs_length > 0) && (fs_length > 0)) {
        /* create the D3D vertex- and pixel-shader objects */
        hr = _sg_d3d11_CreateVertexShader(_sg.d3d11.dev, vs_ptr, vs_length, NULL, &shd->d3d11.vs);
        bool vs_succeeded = SUCCEEDED(hr) && shd->d3d11.vs;
        hr = _sg_d3d11_CreatePixelShader(_sg.d3d11.dev, fs_ptr, fs_length, NULL, &shd->d3d11.fs);
        bool fs_succeeded = SUCCEEDED(hr) && shd->d3d11.fs;

        /* need to store the vertex shader byte code, this is needed later in sg_create_pipeline */
        if (vs_succeeded && fs_succeeded) {
            shd->d3d11.vs_blob_length = vs_length;
            shd->d3d11.vs_blob = _sg_malloc((size_t)vs_length);
            SOKOL_ASSERT(shd->d3d11.vs_blob);
            memcpy(shd->d3d11.vs_blob, vs_ptr, vs_length);
            result = SG_RESOURCESTATE_VALID;
        }
    }
    if (vs_blob) {
        _sg_d3d11_Release(vs_blob); vs_blob = 0;
    }
    if (fs_blob) {
        _sg_d3d11_Release(fs_blob); fs_blob = 0;
    }
    return result;
}

_SOKOL_PRIVATE void _sg_d3d11_discard_shader(_sg_shader_t* shd) {
    SOKOL_ASSERT(shd);
    if (shd->d3d11.vs) {
        _sg_d3d11_Release(shd->d3d11.vs);
    }
    if (shd->d3d11.fs) {
        _sg_d3d11_Release(shd->d3d11.fs);
    }
    if (shd->d3d11.vs_blob) {
        _sg_free(shd->d3d11.vs_blob);
    }
    for (int stage_index = 0; stage_index < SG_NUM_SHADER_STAGES; stage_index++) {
        _sg_shader_stage_t* cmn_stage = &shd->cmn.stage[stage_index];
        _sg_d3d11_shader_stage_t* d3d11_stage = &shd->d3d11.stage[stage_index];
        for (int ub_index = 0; ub_index < cmn_stage->num_uniform_blocks; ub_index++) {
            if (d3d11_stage->cbufs[ub_index]) {
                _sg_d3d11_Release(d3d11_stage->cbufs[ub_index]);
            }
        }
    }
}

_SOKOL_PRIVATE sg_resource_state _sg_d3d11_create_pipeline(_sg_pipeline_t* pip, _sg_shader_t* shd, const sg_pipeline_desc* desc) {
    SOKOL_ASSERT(pip && shd && desc);
    SOKOL_ASSERT(desc->shader.id == shd->slot.id);
    SOKOL_ASSERT(shd->slot.state == SG_RESOURCESTATE_VALID);
    SOKOL_ASSERT(shd->d3d11.vs_blob && shd->d3d11.vs_blob_length > 0);
    SOKOL_ASSERT(!pip->d3d11.il && !pip->d3d11.rs && !pip->d3d11.dss && !pip->d3d11.bs);

    pip->shader = shd;
    _sg_pipeline_common_init(&pip->cmn, desc);
    pip->d3d11.index_format = _sg_d3d11_index_format(pip->cmn.index_type);
    pip->d3d11.topology = _sg_d3d11_primitive_topology(desc->primitive_type);
    pip->d3d11.stencil_ref = desc->stencil.ref;

    /* create input layout object */
    HRESULT hr;
    D3D11_INPUT_ELEMENT_DESC d3d11_comps[SG_MAX_VERTEX_ATTRIBUTES];
    _sg_clear(d3d11_comps, sizeof(d3d11_comps));
    int attr_index = 0;
    for (; attr_index < SG_MAX_VERTEX_ATTRIBUTES; attr_index++) {
        const sg_vertex_attr_desc* a_desc = &desc->layout.attrs[attr_index];
        if (a_desc->format == SG_VERTEXFORMAT_INVALID) {
            break;
        }
        SOKOL_ASSERT(a_desc->buffer_index < SG_MAX_SHADERSTAGE_BUFFERS);
        const sg_buffer_layout_desc* l_desc = &desc->layout.buffers[a_desc->buffer_index];
        const sg_vertex_step step_func = l_desc->step_func;
        const int step_rate = l_desc->step_rate;
        D3D11_INPUT_ELEMENT_DESC* d3d11_comp = &d3d11_comps[attr_index];
        d3d11_comp->SemanticName = _sg_strptr(&shd->d3d11.attrs[attr_index].sem_name);
        d3d11_comp->SemanticIndex = (UINT)shd->d3d11.attrs[attr_index].sem_index;
        d3d11_comp->Format = _sg_d3d11_vertex_format(a_desc->format);
        d3d11_comp->InputSlot = (UINT)a_desc->buffer_index;
        d3d11_comp->AlignedByteOffset = (UINT)a_desc->offset;
        d3d11_comp->InputSlotClass = _sg_d3d11_input_classification(step_func);
        if (SG_VERTEXSTEP_PER_INSTANCE == step_func) {
            d3d11_comp->InstanceDataStepRate = (UINT)step_rate;
            pip->cmn.use_instanced_draw = true;
        }
        pip->cmn.vertex_layout_valid[a_desc->buffer_index] = true;
    }
    for (int layout_index = 0; layout_index < SG_MAX_SHADERSTAGE_BUFFERS; layout_index++) {
        if (pip->cmn.vertex_layout_valid[layout_index]) {
            const sg_buffer_layout_desc* l_desc = &desc->layout.buffers[layout_index];
            SOKOL_ASSERT(l_desc->stride > 0);
            pip->d3d11.vb_strides[layout_index] = (UINT)l_desc->stride;
        }
        else {
            pip->d3d11.vb_strides[layout_index] = 0;
        }
    }
    hr = _sg_d3d11_CreateInputLayout(_sg.d3d11.dev,
        d3d11_comps,                /* pInputElementDesc */
        (UINT)attr_index,           /* NumElements */
        shd->d3d11.vs_blob,         /* pShaderByteCodeWithInputSignature */
        shd->d3d11.vs_blob_length,  /* BytecodeLength */
        &pip->d3d11.il);
    if (!(SUCCEEDED(hr) && pip->d3d11.il)) {
        SG_LOG("failed to create D3D11 input layout\n");
        return SG_RESOURCESTATE_FAILED;
    }

    /* create rasterizer state */
    D3D11_RASTERIZER_DESC rs_desc;
    _sg_clear(&rs_desc, sizeof(rs_desc));
    rs_desc.FillMode = D3D11_FILL_SOLID;
    rs_desc.CullMode = _sg_d3d11_cull_mode(desc->cull_mode);
    rs_desc.FrontCounterClockwise = desc->face_winding == SG_FACEWINDING_CCW;
    rs_desc.DepthBias = (INT) pip->cmn.depth_bias;
    rs_desc.DepthBiasClamp = pip->cmn.depth_bias_clamp;
    rs_desc.SlopeScaledDepthBias = pip->cmn.depth_bias_slope_scale;
    rs_desc.DepthClipEnable = TRUE;
    rs_desc.ScissorEnable = TRUE;
    rs_desc.MultisampleEnable = desc->sample_count > 1;
    rs_desc.AntialiasedLineEnable = FALSE;
    hr = _sg_d3d11_CreateRasterizerState(_sg.d3d11.dev, &rs_desc, &pip->d3d11.rs);
    if (!(SUCCEEDED(hr) && pip->d3d11.rs)) {
        SG_LOG("failed to create D3D11 rasterizer state\n");
        return SG_RESOURCESTATE_FAILED;
    }

    /* create depth-stencil state */
    D3D11_DEPTH_STENCIL_DESC dss_desc;
    _sg_clear(&dss_desc, sizeof(dss_desc));
    dss_desc.DepthEnable = TRUE;
    dss_desc.DepthWriteMask = desc->depth.write_enabled ? D3D11_DEPTH_WRITE_MASK_ALL : D3D11_DEPTH_WRITE_MASK_ZERO;
    dss_desc.DepthFunc = _sg_d3d11_compare_func(desc->depth.compare);
    dss_desc.StencilEnable = desc->stencil.enabled;
    dss_desc.StencilReadMask = desc->stencil.read_mask;
    dss_desc.StencilWriteMask = desc->stencil.write_mask;
    const sg_stencil_face_state* sf = &desc->stencil.front;
    dss_desc.FrontFace.StencilFailOp = _sg_d3d11_stencil_op(sf->fail_op);
    dss_desc.FrontFace.StencilDepthFailOp = _sg_d3d11_stencil_op(sf->depth_fail_op);
    dss_desc.FrontFace.StencilPassOp = _sg_d3d11_stencil_op(sf->pass_op);
    dss_desc.FrontFace.StencilFunc = _sg_d3d11_compare_func(sf->compare);
    const sg_stencil_face_state* sb = &desc->stencil.back;
    dss_desc.BackFace.StencilFailOp = _sg_d3d11_stencil_op(sb->fail_op);
    dss_desc.BackFace.StencilDepthFailOp = _sg_d3d11_stencil_op(sb->depth_fail_op);
    dss_desc.BackFace.StencilPassOp = _sg_d3d11_stencil_op(sb->pass_op);
    dss_desc.BackFace.StencilFunc = _sg_d3d11_compare_func(sb->compare);
    hr = _sg_d3d11_CreateDepthStencilState(_sg.d3d11.dev, &dss_desc, &pip->d3d11.dss);
    if (!(SUCCEEDED(hr) && pip->d3d11.dss)) {
        SG_LOG("failed to create D3D11 depth stencil state\n");
        return SG_RESOURCESTATE_FAILED;
    }

    /* create blend state */
    D3D11_BLEND_DESC bs_desc;
    _sg_clear(&bs_desc, sizeof(bs_desc));
    bs_desc.AlphaToCoverageEnable = desc->alpha_to_coverage_enabled;
    bs_desc.IndependentBlendEnable = TRUE;
    {
        int i = 0;
        for (i = 0; i < desc->color_count; i++) {
            const sg_blend_state* src = &desc->colors[i].blend;
            D3D11_RENDER_TARGET_BLEND_DESC* dst = &bs_desc.RenderTarget[i];
            dst->BlendEnable = src->enabled;
            dst->SrcBlend = _sg_d3d11_blend_factor(src->src_factor_rgb);
            dst->DestBlend = _sg_d3d11_blend_factor(src->dst_factor_rgb);
            dst->BlendOp = _sg_d3d11_blend_op(src->op_rgb);
            dst->SrcBlendAlpha = _sg_d3d11_blend_factor(src->src_factor_alpha);
            dst->DestBlendAlpha = _sg_d3d11_blend_factor(src->dst_factor_alpha);
            dst->BlendOpAlpha = _sg_d3d11_blend_op(src->op_alpha);
            dst->RenderTargetWriteMask = _sg_d3d11_color_write_mask(desc->colors[i].write_mask);
        }
        for (; i < 8; i++) {
            D3D11_RENDER_TARGET_BLEND_DESC* dst = &bs_desc.RenderTarget[i];
            dst->BlendEnable = FALSE;
            dst->SrcBlend = dst->SrcBlendAlpha = D3D11_BLEND_ONE;
            dst->DestBlend = dst->DestBlendAlpha = D3D11_BLEND_ZERO;
            dst->BlendOp = dst->BlendOpAlpha = D3D11_BLEND_OP_ADD;
            dst->RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
        }
    }
    hr = _sg_d3d11_CreateBlendState(_sg.d3d11.dev, &bs_desc, &pip->d3d11.bs);
    if (!(SUCCEEDED(hr) && pip->d3d11.bs)) {
        SG_LOG("failed to create D3D11 blend state\n");
        return SG_RESOURCESTATE_FAILED;
    }

    return SG_RESOURCESTATE_VALID;
}

_SOKOL_PRIVATE void _sg_d3d11_discard_pipeline(_sg_pipeline_t* pip) {
    SOKOL_ASSERT(pip);
    if (pip == _sg.d3d11.cur_pipeline) {
        _sg.d3d11.cur_pipeline = 0;
        _sg.d3d11.cur_pipeline_id.id = SG_INVALID_ID;
    }
    if (pip->d3d11.il) {
        _sg_d3d11_Release(pip->d3d11.il);
    }
    if (pip->d3d11.rs) {
        _sg_d3d11_Release(pip->d3d11.rs);
    }
    if (pip->d3d11.dss) {
        _sg_d3d11_Release(pip->d3d11.dss);
    }
    if (pip->d3d11.bs) {
        _sg_d3d11_Release(pip->d3d11.bs);
    }
}

_SOKOL_PRIVATE sg_resource_state _sg_d3d11_create_pass(_sg_pass_t* pass, _sg_image_t** att_images, const sg_pass_desc* desc) {
    SOKOL_ASSERT(pass && desc);
    SOKOL_ASSERT(att_images && att_images[0]);
    SOKOL_ASSERT(_sg.d3d11.dev);

    _sg_pass_common_init(&pass->cmn, desc);

    for (int i = 0; i < pass->cmn.num_color_atts; i++) {
        const sg_pass_attachment_desc* att_desc = &desc->color_attachments[i];
        _SOKOL_UNUSED(att_desc);
        SOKOL_ASSERT(att_desc->image.id != SG_INVALID_ID);
        _sg_image_t* att_img = att_images[i];
        SOKOL_ASSERT(att_img && (att_img->slot.id == att_desc->image.id));
        SOKOL_ASSERT(_sg_is_valid_rendertarget_color_format(att_img->cmn.pixel_format));
        SOKOL_ASSERT(0 == pass->d3d11.color_atts[i].image);
        pass->d3d11.color_atts[i].image = att_img;

        /* create D3D11 render-target-view */
        const _sg_pass_attachment_t* cmn_att = &pass->cmn.color_atts[i];
        SOKOL_ASSERT(0 == pass->d3d11.color_atts[i].rtv);
        ID3D11Resource* d3d11_res = 0;
        const bool is_msaa = att_img->cmn.sample_count > 1;
        D3D11_RENDER_TARGET_VIEW_DESC d3d11_rtv_desc;
        _sg_clear(&d3d11_rtv_desc, sizeof(d3d11_rtv_desc));
        d3d11_rtv_desc.Format = att_img->d3d11.format;
        if ((att_img->cmn.type == SG_IMAGETYPE_2D) || is_msaa) {
            if (is_msaa) {
                d3d11_res = (ID3D11Resource*) att_img->d3d11.texmsaa;
                d3d11_rtv_desc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2DMS;
            }
            else {
                d3d11_res = (ID3D11Resource*) att_img->d3d11.tex2d;
                d3d11_rtv_desc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;
                d3d11_rtv_desc.Texture2D.MipSlice = (UINT)cmn_att->mip_level;
            }
        }
        else if ((att_img->cmn.type == SG_IMAGETYPE_CUBE) || (att_img->cmn.type == SG_IMAGETYPE_ARRAY)) {
            d3d11_res = (ID3D11Resource*) att_img->d3d11.tex2d;
            d3d11_rtv_desc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2DARRAY;
            d3d11_rtv_desc.Texture2DArray.MipSlice = (UINT)cmn_att->mip_level;
            d3d11_rtv_desc.Texture2DArray.FirstArraySlice = (UINT)cmn_att->slice;
            d3d11_rtv_desc.Texture2DArray.ArraySize = 1;
        }
        else {
            SOKOL_ASSERT(att_img->cmn.type == SG_IMAGETYPE_3D);
            d3d11_res = (ID3D11Resource*) att_img->d3d11.tex3d;
            d3d11_rtv_desc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE3D;
            d3d11_rtv_desc.Texture3D.MipSlice = (UINT)cmn_att->mip_level;
            d3d11_rtv_desc.Texture3D.FirstWSlice = (UINT)cmn_att->slice;
            d3d11_rtv_desc.Texture3D.WSize = 1;
        }
        SOKOL_ASSERT(d3d11_res);
        HRESULT hr = _sg_d3d11_CreateRenderTargetView(_sg.d3d11.dev, d3d11_res, &d3d11_rtv_desc, &pass->d3d11.color_atts[i].rtv);
        if (!(SUCCEEDED(hr) && pass->d3d11.color_atts[i].rtv)) {
            SG_LOG("failed to create D3D11 render target view\n");
            return SG_RESOURCESTATE_FAILED;
        }
    }

    /* optional depth-stencil image */
    SOKOL_ASSERT(0 == pass->d3d11.ds_att.image);
    SOKOL_ASSERT(0 == pass->d3d11.ds_att.dsv);
    if (desc->depth_stencil_attachment.image.id != SG_INVALID_ID) {
        const int ds_img_index = SG_MAX_COLOR_ATTACHMENTS;
        const sg_pass_attachment_desc* att_desc = &desc->depth_stencil_attachment;
        _SOKOL_UNUSED(att_desc);
        _sg_image_t* att_img = att_images[ds_img_index];
        SOKOL_ASSERT(att_img && (att_img->slot.id == att_desc->image.id));
        SOKOL_ASSERT(_sg_is_valid_rendertarget_depth_format(att_img->cmn.pixel_format));
        SOKOL_ASSERT(0 == pass->d3d11.ds_att.image);
        pass->d3d11.ds_att.image = att_img;

        /* create D3D11 depth-stencil-view */
        D3D11_DEPTH_STENCIL_VIEW_DESC d3d11_dsv_desc;
        _sg_clear(&d3d11_dsv_desc, sizeof(d3d11_dsv_desc));
        d3d11_dsv_desc.Format = att_img->d3d11.format;
        const bool is_msaa = att_img->cmn.sample_count > 1;
        if (is_msaa) {
            d3d11_dsv_desc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2DMS;
        }
        else {
            d3d11_dsv_desc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
        }
        ID3D11Resource* d3d11_res = (ID3D11Resource*) att_img->d3d11.texds;
        SOKOL_ASSERT(d3d11_res);
        HRESULT hr = _sg_d3d11_CreateDepthStencilView(_sg.d3d11.dev, d3d11_res, &d3d11_dsv_desc, &pass->d3d11.ds_att.dsv);
        if (!(SUCCEEDED(hr) && pass->d3d11.ds_att.dsv)) {
            SG_LOG("failed to create D3D11 depth stencil view\n");
            return SG_RESOURCESTATE_FAILED;
        }
    }
    return SG_RESOURCESTATE_VALID;
}

_SOKOL_PRIVATE void _sg_d3d11_discard_pass(_sg_pass_t* pass) {
    SOKOL_ASSERT(pass);
    SOKOL_ASSERT(pass != _sg.d3d11.cur_pass);
    for (int i = 0; i < SG_MAX_COLOR_ATTACHMENTS; i++) {
        if (pass->d3d11.color_atts[i].rtv) {
            _sg_d3d11_Release(pass->d3d11.color_atts[i].rtv);
        }
    }
    if (pass->d3d11.ds_att.dsv) {
        _sg_d3d11_Release(pass->d3d11.ds_att.dsv);
    }
}

_SOKOL_PRIVATE _sg_image_t* _sg_d3d11_pass_color_image(const _sg_pass_t* pass, int index) {
    SOKOL_ASSERT(pass && (index >= 0) && (index < SG_MAX_COLOR_ATTACHMENTS));
    /* NOTE: may return null */
    return pass->d3d11.color_atts[index].image;
}

_SOKOL_PRIVATE _sg_image_t* _sg_d3d11_pass_ds_image(const _sg_pass_t* pass) {
    /* NOTE: may return null */
    SOKOL_ASSERT(pass);
    return pass->d3d11.ds_att.image;
}

_SOKOL_PRIVATE void _sg_d3d11_begin_pass(_sg_pass_t* pass, const sg_pass_action* action, int w, int h) {
    SOKOL_ASSERT(action);
    SOKOL_ASSERT(!_sg.d3d11.in_pass);
    SOKOL_ASSERT(_sg.d3d11.rtv_cb || _sg.d3d11.rtv_userdata_cb);
    SOKOL_ASSERT(_sg.d3d11.dsv_cb || _sg.d3d11.dsv_userdata_cb);
    _sg.d3d11.in_pass = true;
    _sg.d3d11.cur_width = w;
    _sg.d3d11.cur_height = h;
    if (pass) {
        _sg.d3d11.cur_pass = pass;
        _sg.d3d11.cur_pass_id.id = pass->slot.id;
        _sg.d3d11.num_rtvs = 0;
        for (int i = 0; i < SG_MAX_COLOR_ATTACHMENTS; i++) {
            _sg.d3d11.cur_rtvs[i] = pass->d3d11.color_atts[i].rtv;
            if (_sg.d3d11.cur_rtvs[i]) {
                _sg.d3d11.num_rtvs++;
            }
        }
        _sg.d3d11.cur_dsv = pass->d3d11.ds_att.dsv;
    }
    else {
        /* render to default frame buffer */
        _sg.d3d11.cur_pass = 0;
        _sg.d3d11.cur_pass_id.id = SG_INVALID_ID;
        _sg.d3d11.num_rtvs = 1;
        if (_sg.d3d11.rtv_cb) {
            _sg.d3d11.cur_rtvs[0] = (ID3D11RenderTargetView*) _sg.d3d11.rtv_cb();
        }
        else {
            _sg.d3d11.cur_rtvs[0] = (ID3D11RenderTargetView*) _sg.d3d11.rtv_userdata_cb(_sg.d3d11.user_data);
        }
        for (int i = 1; i < SG_MAX_COLOR_ATTACHMENTS; i++) {
            _sg.d3d11.cur_rtvs[i] = 0;
        }
        if (_sg.d3d11.dsv_cb) {
            _sg.d3d11.cur_dsv = (ID3D11DepthStencilView*) _sg.d3d11.dsv_cb();
        }
        else {
            _sg.d3d11.cur_dsv = (ID3D11DepthStencilView*) _sg.d3d11.dsv_userdata_cb(_sg.d3d11.user_data);
        }
        SOKOL_ASSERT(_sg.d3d11.cur_rtvs[0] && _sg.d3d11.cur_dsv);
    }
    /* apply the render-target- and depth-stencil-views */
    _sg_d3d11_OMSetRenderTargets(_sg.d3d11.ctx, SG_MAX_COLOR_ATTACHMENTS, _sg.d3d11.cur_rtvs, _sg.d3d11.cur_dsv);

    /* set viewport and scissor rect to cover whole screen */
    D3D11_VIEWPORT vp;
    _sg_clear(&vp, sizeof(vp));
    vp.Width = (FLOAT) w;
    vp.Height = (FLOAT) h;
    vp.MaxDepth = 1.0f;
    _sg_d3d11_RSSetViewports(_sg.d3d11.ctx, 1, &vp);
    D3D11_RECT rect;
    rect.left = 0;
    rect.top = 0;
    rect.right = w;
    rect.bottom = h;
    _sg_d3d11_RSSetScissorRects(_sg.d3d11.ctx, 1, &rect);

    /* perform clear action */
    for (int i = 0; i < _sg.d3d11.num_rtvs; i++) {
        if (action->colors[i].action == SG_ACTION_CLEAR) {
            _sg_d3d11_ClearRenderTargetView(_sg.d3d11.ctx, _sg.d3d11.cur_rtvs[i], &action->colors[i].value.r);
        }
    }
    UINT ds_flags = 0;
    if (action->depth.action == SG_ACTION_CLEAR) {
        ds_flags |= D3D11_CLEAR_DEPTH;
    }
    if (action->stencil.action == SG_ACTION_CLEAR) {
        ds_flags |= D3D11_CLEAR_STENCIL;
    }
    if ((0 != ds_flags) && _sg.d3d11.cur_dsv) {
        _sg_d3d11_ClearDepthStencilView(_sg.d3d11.ctx, _sg.d3d11.cur_dsv, ds_flags, action->depth.value, action->stencil.value);
    }
}

/* D3D11CalcSubresource only exists for C++ */
_SOKOL_PRIVATE UINT _sg_d3d11_calcsubresource(UINT mip_slice, UINT array_slice, UINT mip_levels) {
    return mip_slice + array_slice * mip_levels;
}

_SOKOL_PRIVATE void _sg_d3d11_end_pass(void) {
    SOKOL_ASSERT(_sg.d3d11.in_pass && _sg.d3d11.ctx);
    _sg.d3d11.in_pass = false;

    /* need to resolve MSAA render target into texture? */
    if (_sg.d3d11.cur_pass) {
        SOKOL_ASSERT(_sg.d3d11.cur_pass->slot.id == _sg.d3d11.cur_pass_id.id);
        for (int i = 0; i < _sg.d3d11.num_rtvs; i++) {
            _sg_pass_attachment_t* cmn_att = &_sg.d3d11.cur_pass->cmn.color_atts[i];
            _sg_image_t* att_img = _sg.d3d11.cur_pass->d3d11.color_atts[i].image;
            SOKOL_ASSERT(att_img && (att_img->slot.id == cmn_att->image_id.id));
            if (att_img->cmn.sample_count > 1) {
                /* FIXME: support MSAA resolve into 3D texture */
                SOKOL_ASSERT(att_img->d3d11.tex2d && att_img->d3d11.texmsaa && !att_img->d3d11.tex3d);
                SOKOL_ASSERT(DXGI_FORMAT_UNKNOWN != att_img->d3d11.format);
                UINT dst_subres = _sg_d3d11_calcsubresource((UINT)cmn_att->mip_level, (UINT)cmn_att->slice, (UINT)att_img->cmn.num_mipmaps);
                _sg_d3d11_ResolveSubresource(_sg.d3d11.ctx,
                    (ID3D11Resource*) att_img->d3d11.tex2d,     /* pDstResource */
                    dst_subres,                                 /* DstSubresource */
                    (ID3D11Resource*) att_img->d3d11.texmsaa,   /* pSrcResource */
                    0,                                          /* SrcSubresource */
                    att_img->d3d11.format);
            }
        }
    }
    _sg.d3d11.cur_pass = 0;
    _sg.d3d11.cur_pass_id.id = SG_INVALID_ID;
    _sg.d3d11.cur_pipeline = 0;
    _sg.d3d11.cur_pipeline_id.id = SG_INVALID_ID;
    for (int i = 0; i < SG_MAX_COLOR_ATTACHMENTS; i++) {
        _sg.d3d11.cur_rtvs[i] = 0;
    }
    _sg.d3d11.cur_dsv = 0;
    _sg_d3d11_clear_state();
}

_SOKOL_PRIVATE void _sg_d3d11_apply_viewport(int x, int y, int w, int h, bool origin_top_left) {
    SOKOL_ASSERT(_sg.d3d11.ctx);
    SOKOL_ASSERT(_sg.d3d11.in_pass);
    D3D11_VIEWPORT vp;
    vp.TopLeftX = (FLOAT) x;
    vp.TopLeftY = (FLOAT) (origin_top_left ? y : (_sg.d3d11.cur_height - (y + h)));
    vp.Width = (FLOAT) w;
    vp.Height = (FLOAT) h;
    vp.MinDepth = 0.0f;
    vp.MaxDepth = 1.0f;
    _sg_d3d11_RSSetViewports(_sg.d3d11.ctx, 1, &vp);
}

_SOKOL_PRIVATE void _sg_d3d11_apply_scissor_rect(int x, int y, int w, int h, bool origin_top_left) {
    SOKOL_ASSERT(_sg.d3d11.ctx);
    SOKOL_ASSERT(_sg.d3d11.in_pass);
    D3D11_RECT rect;
    rect.left = x;
    rect.top = (origin_top_left ? y : (_sg.d3d11.cur_height - (y + h)));
    rect.right = x + w;
    rect.bottom = origin_top_left ? (y + h) : (_sg.d3d11.cur_height - y);
    _sg_d3d11_RSSetScissorRects(_sg.d3d11.ctx, 1, &rect);
}

_SOKOL_PRIVATE void _sg_d3d11_apply_pipeline(_sg_pipeline_t* pip) {
    SOKOL_ASSERT(pip);
    SOKOL_ASSERT(pip->shader && (pip->cmn.shader_id.id == pip->shader->slot.id));
    SOKOL_ASSERT(_sg.d3d11.ctx);
    SOKOL_ASSERT(_sg.d3d11.in_pass);
    SOKOL_ASSERT(pip->d3d11.rs && pip->d3d11.bs && pip->d3d11.dss && pip->d3d11.il);

    _sg.d3d11.cur_pipeline = pip;
    _sg.d3d11.cur_pipeline_id.id = pip->slot.id;
    _sg.d3d11.use_indexed_draw = (pip->d3d11.index_format != DXGI_FORMAT_UNKNOWN);
    _sg.d3d11.use_instanced_draw = pip->cmn.use_instanced_draw;

    _sg_d3d11_RSSetState(_sg.d3d11.ctx, pip->d3d11.rs);
    _sg_d3d11_OMSetDepthStencilState(_sg.d3d11.ctx, pip->d3d11.dss, pip->d3d11.stencil_ref);
    _sg_d3d11_OMSetBlendState(_sg.d3d11.ctx, pip->d3d11.bs, &pip->cmn.blend_color.r, 0xFFFFFFFF);
    _sg_d3d11_IASetPrimitiveTopology(_sg.d3d11.ctx, pip->d3d11.topology);
    _sg_d3d11_IASetInputLayout(_sg.d3d11.ctx, pip->d3d11.il);
    _sg_d3d11_VSSetShader(_sg.d3d11.ctx, pip->shader->d3d11.vs, NULL, 0);
    _sg_d3d11_VSSetConstantBuffers(_sg.d3d11.ctx, 0, SG_MAX_SHADERSTAGE_UBS, pip->shader->d3d11.stage[SG_SHADERSTAGE_VS].cbufs);
    _sg_d3d11_PSSetShader(_sg.d3d11.ctx, pip->shader->d3d11.fs, NULL, 0);
    _sg_d3d11_PSSetConstantBuffers(_sg.d3d11.ctx, 0, SG_MAX_SHADERSTAGE_UBS, pip->shader->d3d11.stage[SG_SHADERSTAGE_FS].cbufs);
}

_SOKOL_PRIVATE void _sg_d3d11_apply_bindings(
    _sg_pipeline_t* pip,
    _sg_buffer_t** vbs, const int* vb_offsets, int num_vbs,
    _sg_buffer_t* ib, int ib_offset,
    _sg_image_t** vs_imgs, int num_vs_imgs,
    _sg_image_t** fs_imgs, int num_fs_imgs)
{
    SOKOL_ASSERT(pip);
    SOKOL_ASSERT(_sg.d3d11.ctx);
    SOKOL_ASSERT(_sg.d3d11.in_pass);

    /* gather all the D3D11 resources into arrays */
    ID3D11Buffer* d3d11_ib = ib ? ib->d3d11.buf : 0;
    ID3D11Buffer* d3d11_vbs[SG_MAX_SHADERSTAGE_BUFFERS];
    UINT d3d11_vb_offsets[SG_MAX_SHADERSTAGE_BUFFERS];
    ID3D11ShaderResourceView* d3d11_vs_srvs[SG_MAX_SHADERSTAGE_IMAGES];
    ID3D11SamplerState* d3d11_vs_smps[SG_MAX_SHADERSTAGE_IMAGES];
    ID3D11ShaderResourceView* d3d11_fs_srvs[SG_MAX_SHADERSTAGE_IMAGES];
    ID3D11SamplerState* d3d11_fs_smps[SG_MAX_SHADERSTAGE_IMAGES];
    int i;
    for (i = 0; i < num_vbs; i++) {
        SOKOL_ASSERT(vbs[i]->d3d11.buf);
        d3d11_vbs[i] = vbs[i]->d3d11.buf;
        d3d11_vb_offsets[i] = (UINT)vb_offsets[i];
    }
    for (; i < SG_MAX_SHADERSTAGE_BUFFERS; i++) {
        d3d11_vbs[i] = 0;
        d3d11_vb_offsets[i] = 0;
    }
    for (i = 0; i < num_vs_imgs; i++) {
        SOKOL_ASSERT(vs_imgs[i]->d3d11.srv);
        SOKOL_ASSERT(vs_imgs[i]->d3d11.smp);
        d3d11_vs_srvs[i] = vs_imgs[i]->d3d11.srv;
        d3d11_vs_smps[i] = vs_imgs[i]->d3d11.smp;
    }
    for (; i < SG_MAX_SHADERSTAGE_IMAGES; i++) {
        d3d11_vs_srvs[i] = 0;
        d3d11_vs_smps[i] = 0;
    }
    for (i = 0; i < num_fs_imgs; i++) {
        SOKOL_ASSERT(fs_imgs[i]->d3d11.srv);
        SOKOL_ASSERT(fs_imgs[i]->d3d11.smp);
        d3d11_fs_srvs[i] = fs_imgs[i]->d3d11.srv;
        d3d11_fs_smps[i] = fs_imgs[i]->d3d11.smp;
    }
    for (; i < SG_MAX_SHADERSTAGE_IMAGES; i++) {
        d3d11_fs_srvs[i] = 0;
        d3d11_fs_smps[i] = 0;
    }

    _sg_d3d11_IASetVertexBuffers(_sg.d3d11.ctx, 0, SG_MAX_SHADERSTAGE_BUFFERS, d3d11_vbs, pip->d3d11.vb_strides, d3d11_vb_offsets);
    _sg_d3d11_IASetIndexBuffer(_sg.d3d11.ctx, d3d11_ib, pip->d3d11.index_format, (UINT)ib_offset);
    _sg_d3d11_VSSetShaderResources(_sg.d3d11.ctx, 0, SG_MAX_SHADERSTAGE_IMAGES, d3d11_vs_srvs);
    _sg_d3d11_VSSetSamplers(_sg.d3d11.ctx, 0, SG_MAX_SHADERSTAGE_IMAGES, d3d11_vs_smps);
    _sg_d3d11_PSSetShaderResources(_sg.d3d11.ctx, 0, SG_MAX_SHADERSTAGE_IMAGES, d3d11_fs_srvs);
    _sg_d3d11_PSSetSamplers(_sg.d3d11.ctx, 0, SG_MAX_SHADERSTAGE_IMAGES, d3d11_fs_smps);
}

_SOKOL_PRIVATE void _sg_d3d11_apply_uniforms(sg_shader_stage stage_index, int ub_index, const sg_range* data) {
    SOKOL_ASSERT(_sg.d3d11.ctx && _sg.d3d11.in_pass);
    SOKOL_ASSERT(_sg.d3d11.cur_pipeline && _sg.d3d11.cur_pipeline->slot.id == _sg.d3d11.cur_pipeline_id.id);
    SOKOL_ASSERT(_sg.d3d11.cur_pipeline->shader && _sg.d3d11.cur_pipeline->shader->slot.id == _sg.d3d11.cur_pipeline->cmn.shader_id.id);
    SOKOL_ASSERT(ub_index < _sg.d3d11.cur_pipeline->shader->cmn.stage[stage_index].num_uniform_blocks);
    SOKOL_ASSERT(data->size == _sg.d3d11.cur_pipeline->shader->cmn.stage[stage_index].uniform_blocks[ub_index].size);
    ID3D11Buffer* cb = _sg.d3d11.cur_pipeline->shader->d3d11.stage[stage_index].cbufs[ub_index];
    SOKOL_ASSERT(cb);
    _sg_d3d11_UpdateSubresource(_sg.d3d11.ctx, (ID3D11Resource*)cb, 0, NULL, data->ptr, 0, 0);
}

_SOKOL_PRIVATE void _sg_d3d11_draw(int base_element, int num_elements, int num_instances) {
    SOKOL_ASSERT(_sg.d3d11.in_pass);
    if (_sg.d3d11.use_indexed_draw) {
        if (_sg.d3d11.use_instanced_draw) {
            _sg_d3d11_DrawIndexedInstanced(_sg.d3d11.ctx, (UINT)num_elements, (UINT)num_instances, (UINT)base_element, 0, 0);
        }
        else {
            _sg_d3d11_DrawIndexed(_sg.d3d11.ctx, (UINT)num_elements, (UINT)base_element, 0);
        }
    }
    else {
        if (_sg.d3d11.use_instanced_draw) {
            _sg_d3d11_DrawInstanced(_sg.d3d11.ctx, (UINT)num_elements, (UINT)num_instances, (UINT)base_element, 0);
        }
        else {
            _sg_d3d11_Draw(_sg.d3d11.ctx, (UINT)num_elements, (UINT)base_element);
        }
    }
}

_SOKOL_PRIVATE void _sg_d3d11_commit(void) {
    SOKOL_ASSERT(!_sg.d3d11.in_pass);
}

_SOKOL_PRIVATE void _sg_d3d11_update_buffer(_sg_buffer_t* buf, const sg_range* data) {
    SOKOL_ASSERT(buf && data && data->ptr && (data->size > 0));
    SOKOL_ASSERT(_sg.d3d11.ctx);
    SOKOL_ASSERT(buf->d3d11.buf);
    D3D11_MAPPED_SUBRESOURCE d3d11_msr;
    HRESULT hr = _sg_d3d11_Map(_sg.d3d11.ctx, (ID3D11Resource*)buf->d3d11.buf, 0, D3D11_MAP_WRITE_DISCARD, 0, &d3d11_msr);
    if (SUCCEEDED(hr)) {
        memcpy(d3d11_msr.pData, data->ptr, data->size);
        _sg_d3d11_Unmap(_sg.d3d11.ctx, (ID3D11Resource*)buf->d3d11.buf, 0);
    } else {
        SG_LOG("failed to map buffer while updating!\n");
    }
}

_SOKOL_PRIVATE int _sg_d3d11_append_buffer(_sg_buffer_t* buf, const sg_range* data, bool new_frame) {
    SOKOL_ASSERT(buf && data && data->ptr && (data->size > 0));
    SOKOL_ASSERT(_sg.d3d11.ctx);
    SOKOL_ASSERT(buf->d3d11.buf);
    D3D11_MAP map_type = new_frame ? D3D11_MAP_WRITE_DISCARD : D3D11_MAP_WRITE_NO_OVERWRITE;
    D3D11_MAPPED_SUBRESOURCE d3d11_msr;
    HRESULT hr = _sg_d3d11_Map(_sg.d3d11.ctx, (ID3D11Resource*)buf->d3d11.buf, 0, map_type, 0, &d3d11_msr);
    if (SUCCEEDED(hr)) {
        uint8_t* dst_ptr = (uint8_t*)d3d11_msr.pData + buf->cmn.append_pos;
        memcpy(dst_ptr, data->ptr, data->size);
        _sg_d3d11_Unmap(_sg.d3d11.ctx, (ID3D11Resource*)buf->d3d11.buf, 0);
    } else {
        SG_LOG("failed to map buffer while appending!\n");
    }
    /* NOTE: this alignment is a requirement from WebGPU, but we want identical behaviour across all backend */
    return _sg_roundup((int)data->size, 4);
}

_SOKOL_PRIVATE void _sg_d3d11_update_image(_sg_image_t* img, const sg_image_data* data) {
    SOKOL_ASSERT(img && data);
    SOKOL_ASSERT(_sg.d3d11.ctx);
    SOKOL_ASSERT(img->d3d11.tex2d || img->d3d11.tex3d);
    ID3D11Resource* d3d11_res = 0;
    if (img->d3d11.tex3d) {
        d3d11_res = (ID3D11Resource*) img->d3d11.tex3d;
    }
    else {
        d3d11_res = (ID3D11Resource*) img->d3d11.tex2d;
    }
    SOKOL_ASSERT(d3d11_res);
    const int num_faces = (img->cmn.type == SG_IMAGETYPE_CUBE) ? 6:1;
    const int num_slices = (img->cmn.type == SG_IMAGETYPE_ARRAY) ? img->cmn.num_slices:1;
    UINT subres_index = 0;
    HRESULT hr;
    D3D11_MAPPED_SUBRESOURCE d3d11_msr;
    for (int face_index = 0; face_index < num_faces; face_index++) {
        for (int slice_index = 0; slice_index < num_slices; slice_index++) {
            for (int mip_index = 0; mip_index < img->cmn.num_mipmaps; mip_index++, subres_index++) {
                SOKOL_ASSERT(subres_index < (SG_MAX_MIPMAPS * SG_MAX_TEXTUREARRAY_LAYERS));
                const int mip_width = ((img->cmn.width>>mip_index)>0) ? img->cmn.width>>mip_index : 1;
                const int mip_height = ((img->cmn.height>>mip_index)>0) ? img->cmn.height>>mip_index : 1;
                const int src_pitch = _sg_row_pitch(img->cmn.pixel_format, mip_width, 1);
                const sg_range* subimg_data = &(data->subimage[face_index][mip_index]);
                const size_t slice_size = subimg_data->size / (size_t)num_slices;
                const size_t slice_offset = slice_size * (size_t)slice_index;
                const uint8_t* slice_ptr = ((const uint8_t*)subimg_data->ptr) + slice_offset;
                hr = _sg_d3d11_Map(_sg.d3d11.ctx, d3d11_res, subres_index, D3D11_MAP_WRITE_DISCARD, 0, &d3d11_msr);
                if (SUCCEEDED(hr)) {
                    /* FIXME: need to handle difference in depth-pitch for 3D textures as well! */
                    if (src_pitch == (int)d3d11_msr.RowPitch) {
                        memcpy(d3d11_msr.pData, slice_ptr, slice_size);
                    }
                    else {
                        SOKOL_ASSERT(src_pitch < (int)d3d11_msr.RowPitch);
                        const uint8_t* src_ptr = slice_ptr;
                        uint8_t* dst_ptr = (uint8_t*) d3d11_msr.pData;
                        for (int row_index = 0; row_index < mip_height; row_index++) {
                            memcpy(dst_ptr, src_ptr, (size_t)src_pitch);
                            src_ptr += src_pitch;
                            dst_ptr += d3d11_msr.RowPitch;
                        }
                    }
                    _sg_d3d11_Unmap(_sg.d3d11.ctx, d3d11_res, subres_index);
                } else {
                    SG_LOG("failed to map texture!\n");
                }
            }
        }
    }
}

/*== METAL BACKEND IMPLEMENTATION ============================================*/
#elif defined(SOKOL_METAL)

#if __has_feature(objc_arc)
#define _SG_OBJC_RETAIN(obj) { }
#define _SG_OBJC_RELEASE(obj) { obj = nil; }
#else
#define _SG_OBJC_RETAIN(obj) { [obj retain]; }
#define _SG_OBJC_RELEASE(obj) { [obj release]; obj = nil; }
#endif

/*-- enum translation functions ----------------------------------------------*/
_SOKOL_PRIVATE MTLLoadAction _sg_mtl_load_action(sg_action a) {
    switch (a) {
        case SG_ACTION_CLEAR:       return MTLLoadActionClear;
        case SG_ACTION_LOAD:        return MTLLoadActionLoad;
        case SG_ACTION_DONTCARE:    return MTLLoadActionDontCare;
        default: SOKOL_UNREACHABLE; return (MTLLoadAction)0;
    }
}

_SOKOL_PRIVATE MTLResourceOptions _sg_mtl_buffer_resource_options(sg_usage usg) {
    switch (usg) {
        case SG_USAGE_IMMUTABLE:
            #if defined(_SG_TARGET_MACOS)
            return MTLResourceStorageModeManaged;
            #else
            return MTLResourceStorageModeShared;
            #endif
        case SG_USAGE_DYNAMIC:
        case SG_USAGE_STREAM:
            #if defined(_SG_TARGET_MACOS)
            return MTLResourceCPUCacheModeWriteCombined|MTLResourceStorageModeManaged;
            #else
            return MTLResourceCPUCacheModeWriteCombined|MTLResourceStorageModeShared;
            #endif
        default:
            SOKOL_UNREACHABLE;
            return 0;
    }
}

_SOKOL_PRIVATE MTLVertexStepFunction _sg_mtl_step_function(sg_vertex_step step) {
    switch (step) {
        case SG_VERTEXSTEP_PER_VERTEX:      return MTLVertexStepFunctionPerVertex;
        case SG_VERTEXSTEP_PER_INSTANCE:    return MTLVertexStepFunctionPerInstance;
        default: SOKOL_UNREACHABLE; return (MTLVertexStepFunction)0;
    }
}

_SOKOL_PRIVATE MTLVertexFormat _sg_mtl_vertex_format(sg_vertex_format fmt) {
    switch (fmt) {
        case SG_VERTEXFORMAT_FLOAT:     return MTLVertexFormatFloat;
        case SG_VERTEXFORMAT_FLOAT2:    return MTLVertexFormatFloat2;
        case SG_VERTEXFORMAT_FLOAT3:    return MTLVertexFormatFloat3;
        case SG_VERTEXFORMAT_FLOAT4:    return MTLVertexFormatFloat4;
        case SG_VERTEXFORMAT_BYTE4:     return MTLVertexFormatChar4;
        case SG_VERTEXFORMAT_BYTE4N:    return MTLVertexFormatChar4Normalized;
        case SG_VERTEXFORMAT_UBYTE4:    return MTLVertexFormatUChar4;
        case SG_VERTEXFORMAT_UBYTE4N:   return MTLVertexFormatUChar4Normalized;
        case SG_VERTEXFORMAT_SHORT2:    return MTLVertexFormatShort2;
        case SG_VERTEXFORMAT_SHORT2N:   return MTLVertexFormatShort2Normalized;
        case SG_VERTEXFORMAT_USHORT2N:  return MTLVertexFormatUShort2Normalized;
        case SG_VERTEXFORMAT_SHORT4:    return MTLVertexFormatShort4;
        case SG_VERTEXFORMAT_SHORT4N:   return MTLVertexFormatShort4Normalized;
        case SG_VERTEXFORMAT_USHORT4N:  return MTLVertexFormatUShort4Normalized;
        case SG_VERTEXFORMAT_UINT10_N2: return MTLVertexFormatUInt1010102Normalized;
        default: SOKOL_UNREACHABLE; return (MTLVertexFormat)0;
    }
}

_SOKOL_PRIVATE MTLPrimitiveType _sg_mtl_primitive_type(sg_primitive_type t) {
    switch (t) {
        case SG_PRIMITIVETYPE_POINTS:           return MTLPrimitiveTypePoint;
        case SG_PRIMITIVETYPE_LINES:            return MTLPrimitiveTypeLine;
        case SG_PRIMITIVETYPE_LINE_STRIP:       return MTLPrimitiveTypeLineStrip;
        case SG_PRIMITIVETYPE_TRIANGLES:        return MTLPrimitiveTypeTriangle;
        case SG_PRIMITIVETYPE_TRIANGLE_STRIP:   return MTLPrimitiveTypeTriangleStrip;
        default: SOKOL_UNREACHABLE; return (MTLPrimitiveType)0;
    }
}

_SOKOL_PRIVATE MTLPixelFormat _sg_mtl_pixel_format(sg_pixel_format fmt) {
    switch (fmt) {
        case SG_PIXELFORMAT_R8:                     return MTLPixelFormatR8Unorm;
        case SG_PIXELFORMAT_R8SN:                   return MTLPixelFormatR8Snorm;
        case SG_PIXELFORMAT_R8UI:                   return MTLPixelFormatR8Uint;
        case SG_PIXELFORMAT_R8SI:                   return MTLPixelFormatR8Sint;
        case SG_PIXELFORMAT_R16:                    return MTLPixelFormatR16Unorm;
        case SG_PIXELFORMAT_R16SN:                  return MTLPixelFormatR16Snorm;
        case SG_PIXELFORMAT_R16UI:                  return MTLPixelFormatR16Uint;
        case SG_PIXELFORMAT_R16SI:                  return MTLPixelFormatR16Sint;
        case SG_PIXELFORMAT_R16F:                   return MTLPixelFormatR16Float;
        case SG_PIXELFORMAT_RG8:                    return MTLPixelFormatRG8Unorm;
        case SG_PIXELFORMAT_RG8SN:                  return MTLPixelFormatRG8Snorm;
        case SG_PIXELFORMAT_RG8UI:                  return MTLPixelFormatRG8Uint;
        case SG_PIXELFORMAT_RG8SI:                  return MTLPixelFormatRG8Sint;
        case SG_PIXELFORMAT_R32UI:                  return MTLPixelFormatR32Uint;
        case SG_PIXELFORMAT_R32SI:                  return MTLPixelFormatR32Sint;
        case SG_PIXELFORMAT_R32F:                   return MTLPixelFormatR32Float;
        case SG_PIXELFORMAT_RG16:                   return MTLPixelFormatRG16Unorm;
        case SG_PIXELFORMAT_RG16SN:                 return MTLPixelFormatRG16Snorm;
        case SG_PIXELFORMAT_RG16UI:                 return MTLPixelFormatRG16Uint;
        case SG_PIXELFORMAT_RG16SI:                 return MTLPixelFormatRG16Sint;
        case SG_PIXELFORMAT_RG16F:                  return MTLPixelFormatRG16Float;
        case SG_PIXELFORMAT_RGBA8:                  return MTLPixelFormatRGBA8Unorm;
        case SG_PIXELFORMAT_RGBA8SN:                return MTLPixelFormatRGBA8Snorm;
        case SG_PIXELFORMAT_RGBA8UI:                return MTLPixelFormatRGBA8Uint;
        case SG_PIXELFORMAT_RGBA8SI:                return MTLPixelFormatRGBA8Sint;
        case SG_PIXELFORMAT_BGRA8:                  return MTLPixelFormatBGRA8Unorm;
        case SG_PIXELFORMAT_RGB10A2:                return MTLPixelFormatRGB10A2Unorm;
        case SG_PIXELFORMAT_RG11B10F:               return MTLPixelFormatRG11B10Float;
        case SG_PIXELFORMAT_RGB9E5:                 return MTLPixelFormatRGB9E5Float;
        case SG_PIXELFORMAT_RG32UI:                 return MTLPixelFormatRG32Uint;
        case SG_PIXELFORMAT_RG32SI:                 return MTLPixelFormatRG32Sint;
        case SG_PIXELFORMAT_RG32F:                  return MTLPixelFormatRG32Float;
        case SG_PIXELFORMAT_RGBA16:                 return MTLPixelFormatRGBA16Unorm;
        case SG_PIXELFORMAT_RGBA16SN:               return MTLPixelFormatRGBA16Snorm;
        case SG_PIXELFORMAT_RGBA16UI:               return MTLPixelFormatRGBA16Uint;
        case SG_PIXELFORMAT_RGBA16SI:               return MTLPixelFormatRGBA16Sint;
        case SG_PIXELFORMAT_RGBA16F:                return MTLPixelFormatRGBA16Float;
        case SG_PIXELFORMAT_RGBA32UI:               return MTLPixelFormatRGBA32Uint;
        case SG_PIXELFORMAT_RGBA32SI:               return MTLPixelFormatRGBA32Sint;
        case SG_PIXELFORMAT_RGBA32F:                return MTLPixelFormatRGBA32Float;
        case SG_PIXELFORMAT_DEPTH:                  return MTLPixelFormatDepth32Float;
        case SG_PIXELFORMAT_DEPTH_STENCIL:          return MTLPixelFormatDepth32Float_Stencil8;
        #if defined(_SG_TARGET_MACOS)
        case SG_PIXELFORMAT_BC1_RGBA:               return MTLPixelFormatBC1_RGBA;
        case SG_PIXELFORMAT_BC2_RGBA:               return MTLPixelFormatBC2_RGBA;
        case SG_PIXELFORMAT_BC3_RGBA:               return MTLPixelFormatBC3_RGBA;
        case SG_PIXELFORMAT_BC4_R:                  return MTLPixelFormatBC4_RUnorm;
        case SG_PIXELFORMAT_BC4_RSN:                return MTLPixelFormatBC4_RSnorm;
        case SG_PIXELFORMAT_BC5_RG:                 return MTLPixelFormatBC5_RGUnorm;
        case SG_PIXELFORMAT_BC5_RGSN:               return MTLPixelFormatBC5_RGSnorm;
        case SG_PIXELFORMAT_BC6H_RGBF:              return MTLPixelFormatBC6H_RGBFloat;
        case SG_PIXELFORMAT_BC6H_RGBUF:             return MTLPixelFormatBC6H_RGBUfloat;
        case SG_PIXELFORMAT_BC7_RGBA:               return MTLPixelFormatBC7_RGBAUnorm;
        #else
        case SG_PIXELFORMAT_PVRTC_RGB_2BPP:         return MTLPixelFormatPVRTC_RGB_2BPP;
        case SG_PIXELFORMAT_PVRTC_RGB_4BPP:         return MTLPixelFormatPVRTC_RGB_4BPP;
        case SG_PIXELFORMAT_PVRTC_RGBA_2BPP:        return MTLPixelFormatPVRTC_RGBA_2BPP;
        case SG_PIXELFORMAT_PVRTC_RGBA_4BPP:        return MTLPixelFormatPVRTC_RGBA_4BPP;
        case SG_PIXELFORMAT_ETC2_RGB8:              return MTLPixelFormatETC2_RGB8;
        case SG_PIXELFORMAT_ETC2_RGB8A1:            return MTLPixelFormatETC2_RGB8A1;
        case SG_PIXELFORMAT_ETC2_RGBA8:             return MTLPixelFormatEAC_RGBA8;
        case SG_PIXELFORMAT_ETC2_RG11:              return MTLPixelFormatEAC_RG11Unorm;
        case SG_PIXELFORMAT_ETC2_RG11SN:            return MTLPixelFormatEAC_RG11Snorm;
        #endif
        default: return MTLPixelFormatInvalid;
    }
}

_SOKOL_PRIVATE MTLColorWriteMask _sg_mtl_color_write_mask(sg_color_mask m) {
    MTLColorWriteMask mtl_mask = MTLColorWriteMaskNone;
    if (m & SG_COLORMASK_R) {
        mtl_mask |= MTLColorWriteMaskRed;
    }
    if (m & SG_COLORMASK_G) {
        mtl_mask |= MTLColorWriteMaskGreen;
    }
    if (m & SG_COLORMASK_B) {
        mtl_mask |= MTLColorWriteMaskBlue;
    }
    if (m & SG_COLORMASK_A) {
        mtl_mask |= MTLColorWriteMaskAlpha;
    }
    return mtl_mask;
}

_SOKOL_PRIVATE MTLBlendOperation _sg_mtl_blend_op(sg_blend_op op) {
    switch (op) {
        case SG_BLENDOP_ADD:                return MTLBlendOperationAdd;
        case SG_BLENDOP_SUBTRACT:           return MTLBlendOperationSubtract;
        case SG_BLENDOP_REVERSE_SUBTRACT:   return MTLBlendOperationReverseSubtract;
        default: SOKOL_UNREACHABLE; return (MTLBlendOperation)0;
    }
}

_SOKOL_PRIVATE MTLBlendFactor _sg_mtl_blend_factor(sg_blend_factor f) {
    switch (f) {
        case SG_BLENDFACTOR_ZERO:                   return MTLBlendFactorZero;
        case SG_BLENDFACTOR_ONE:                    return MTLBlendFactorOne;
        case SG_BLENDFACTOR_SRC_COLOR:              return MTLBlendFactorSourceColor;
        case SG_BLENDFACTOR_ONE_MINUS_SRC_COLOR:    return MTLBlendFactorOneMinusSourceColor;
        case SG_BLENDFACTOR_SRC_ALPHA:              return MTLBlendFactorSourceAlpha;
        case SG_BLENDFACTOR_ONE_MINUS_SRC_ALPHA:    return MTLBlendFactorOneMinusSourceAlpha;
        case SG_BLENDFACTOR_DST_COLOR:              return MTLBlendFactorDestinationColor;
        case SG_BLENDFACTOR_ONE_MINUS_DST_COLOR:    return MTLBlendFactorOneMinusDestinationColor;
        case SG_BLENDFACTOR_DST_ALPHA:              return MTLBlendFactorDestinationAlpha;
        case SG_BLENDFACTOR_ONE_MINUS_DST_ALPHA:    return MTLBlendFactorOneMinusDestinationAlpha;
        case SG_BLENDFACTOR_SRC_ALPHA_SATURATED:    return MTLBlendFactorSourceAlphaSaturated;
        case SG_BLENDFACTOR_BLEND_COLOR:            return MTLBlendFactorBlendColor;
        case SG_BLENDFACTOR_ONE_MINUS_BLEND_COLOR:  return MTLBlendFactorOneMinusBlendColor;
        case SG_BLENDFACTOR_BLEND_ALPHA:            return MTLBlendFactorBlendAlpha;
        case SG_BLENDFACTOR_ONE_MINUS_BLEND_ALPHA:  return MTLBlendFactorOneMinusBlendAlpha;
        default: SOKOL_UNREACHABLE; return (MTLBlendFactor)0;
    }
}

_SOKOL_PRIVATE MTLCompareFunction _sg_mtl_compare_func(sg_compare_func f) {
    switch (f) {
        case SG_COMPAREFUNC_NEVER:          return MTLCompareFunctionNever;
        case SG_COMPAREFUNC_LESS:           return MTLCompareFunctionLess;
        case SG_COMPAREFUNC_EQUAL:          return MTLCompareFunctionEqual;
        case SG_COMPAREFUNC_LESS_EQUAL:     return MTLCompareFunctionLessEqual;
        case SG_COMPAREFUNC_GREATER:        return MTLCompareFunctionGreater;
        case SG_COMPAREFUNC_NOT_EQUAL:      return MTLCompareFunctionNotEqual;
        case SG_COMPAREFUNC_GREATER_EQUAL:  return MTLCompareFunctionGreaterEqual;
        case SG_COMPAREFUNC_ALWAYS:         return MTLCompareFunctionAlways;
        default: SOKOL_UNREACHABLE; return (MTLCompareFunction)0;
    }
}

_SOKOL_PRIVATE MTLStencilOperation _sg_mtl_stencil_op(sg_stencil_op op) {
    switch (op) {
        case SG_STENCILOP_KEEP:         return MTLStencilOperationKeep;
        case SG_STENCILOP_ZERO:         return MTLStencilOperationZero;
        case SG_STENCILOP_REPLACE:      return MTLStencilOperationReplace;
        case SG_STENCILOP_INCR_CLAMP:   return MTLStencilOperationIncrementClamp;
        case SG_STENCILOP_DECR_CLAMP:   return MTLStencilOperationDecrementClamp;
        case SG_STENCILOP_INVERT:       return MTLStencilOperationInvert;
        case SG_STENCILOP_INCR_WRAP:    return MTLStencilOperationIncrementWrap;
        case SG_STENCILOP_DECR_WRAP:    return MTLStencilOperationDecrementWrap;
        default: SOKOL_UNREACHABLE; return (MTLStencilOperation)0;
    }
}

_SOKOL_PRIVATE MTLCullMode _sg_mtl_cull_mode(sg_cull_mode m) {
    switch (m) {
        case SG_CULLMODE_NONE:  return MTLCullModeNone;
        case SG_CULLMODE_FRONT: return MTLCullModeFront;
        case SG_CULLMODE_BACK:  return MTLCullModeBack;
        default: SOKOL_UNREACHABLE; return (MTLCullMode)0;
    }
}

_SOKOL_PRIVATE MTLWinding _sg_mtl_winding(sg_face_winding w) {
    switch (w) {
        case SG_FACEWINDING_CW:     return MTLWindingClockwise;
        case SG_FACEWINDING_CCW:    return MTLWindingCounterClockwise;
        default: SOKOL_UNREACHABLE; return (MTLWinding)0;
    }
}

_SOKOL_PRIVATE MTLIndexType _sg_mtl_index_type(sg_index_type t) {
    switch (t) {
        case SG_INDEXTYPE_UINT16:   return MTLIndexTypeUInt16;
        case SG_INDEXTYPE_UINT32:   return MTLIndexTypeUInt32;
        default: SOKOL_UNREACHABLE; return (MTLIndexType)0;
    }
}

_SOKOL_PRIVATE int _sg_mtl_index_size(sg_index_type t) {
    switch (t) {
        case SG_INDEXTYPE_NONE:     return 0;
        case SG_INDEXTYPE_UINT16:   return 2;
        case SG_INDEXTYPE_UINT32:   return 4;
        default: SOKOL_UNREACHABLE; return 0;
    }
}

_SOKOL_PRIVATE MTLTextureType _sg_mtl_texture_type(sg_image_type t) {
    switch (t) {
        case SG_IMAGETYPE_2D:       return MTLTextureType2D;
        case SG_IMAGETYPE_CUBE:     return MTLTextureTypeCube;
        case SG_IMAGETYPE_3D:       return MTLTextureType3D;
        case SG_IMAGETYPE_ARRAY:    return MTLTextureType2DArray;
        default: SOKOL_UNREACHABLE; return (MTLTextureType)0;
    }
}

_SOKOL_PRIVATE bool _sg_mtl_is_pvrtc(sg_pixel_format fmt) {
    switch (fmt) {
        case SG_PIXELFORMAT_PVRTC_RGB_2BPP:
        case SG_PIXELFORMAT_PVRTC_RGB_4BPP:
        case SG_PIXELFORMAT_PVRTC_RGBA_2BPP:
        case SG_PIXELFORMAT_PVRTC_RGBA_4BPP:
            return true;
        default:
            return false;
    }
}

_SOKOL_PRIVATE MTLSamplerAddressMode _sg_mtl_address_mode(sg_wrap w) {
    switch (w) {
        case SG_WRAP_REPEAT:            return MTLSamplerAddressModeRepeat;
        case SG_WRAP_CLAMP_TO_EDGE:     return MTLSamplerAddressModeClampToEdge;
        #if defined(_SG_TARGET_MACOS)
        case SG_WRAP_CLAMP_TO_BORDER:   return MTLSamplerAddressModeClampToBorderColor;
        #else
        /* clamp-to-border not supported on iOS, fall back to clamp-to-edge */
        case SG_WRAP_CLAMP_TO_BORDER:   return MTLSamplerAddressModeClampToEdge;
        #endif
        case SG_WRAP_MIRRORED_REPEAT:   return MTLSamplerAddressModeMirrorRepeat;
        default: SOKOL_UNREACHABLE; return (MTLSamplerAddressMode)0;
    }
}

#if defined(_SG_TARGET_MACOS)
_SOKOL_PRIVATE MTLSamplerBorderColor _sg_mtl_border_color(sg_border_color c) {
    switch (c) {
        case SG_BORDERCOLOR_TRANSPARENT_BLACK: return MTLSamplerBorderColorTransparentBlack;
        case SG_BORDERCOLOR_OPAQUE_BLACK: return MTLSamplerBorderColorOpaqueBlack;
        case SG_BORDERCOLOR_OPAQUE_WHITE: return MTLSamplerBorderColorOpaqueWhite;
        default: SOKOL_UNREACHABLE; return (MTLSamplerBorderColor)0;
    }
}
#endif

_SOKOL_PRIVATE MTLSamplerMinMagFilter _sg_mtl_minmag_filter(sg_filter f) {
    switch (f) {
        case SG_FILTER_NEAREST:
        case SG_FILTER_NEAREST_MIPMAP_NEAREST:
        case SG_FILTER_NEAREST_MIPMAP_LINEAR:
            return MTLSamplerMinMagFilterNearest;
        case SG_FILTER_LINEAR:
        case SG_FILTER_LINEAR_MIPMAP_NEAREST:
        case SG_FILTER_LINEAR_MIPMAP_LINEAR:
            return MTLSamplerMinMagFilterLinear;
        default:
            SOKOL_UNREACHABLE; return (MTLSamplerMinMagFilter)0;
    }
}

_SOKOL_PRIVATE MTLSamplerMipFilter _sg_mtl_mip_filter(sg_filter f) {
    switch (f) {
        case SG_FILTER_NEAREST:
        case SG_FILTER_LINEAR:
            return MTLSamplerMipFilterNotMipmapped;
        case SG_FILTER_NEAREST_MIPMAP_NEAREST:
        case SG_FILTER_LINEAR_MIPMAP_NEAREST:
            return MTLSamplerMipFilterNearest;
        case SG_FILTER_NEAREST_MIPMAP_LINEAR:
        case SG_FILTER_LINEAR_MIPMAP_LINEAR:
            return MTLSamplerMipFilterLinear;
        default:
            SOKOL_UNREACHABLE; return (MTLSamplerMipFilter)0;
    }
}

/*-- a pool for all Metal resource objects, with deferred release queue -------*/

_SOKOL_PRIVATE void _sg_mtl_init_pool(const sg_desc* desc) {
    _sg.mtl.idpool.num_slots = 2 *
        (
            2 * desc->buffer_pool_size +
            5 * desc->image_pool_size +
            4 * desc->shader_pool_size +
            2 * desc->pipeline_pool_size +
            desc->pass_pool_size
        );
    _sg.mtl.idpool.pool = [NSMutableArray arrayWithCapacity:(NSUInteger)_sg.mtl.idpool.num_slots];
    _SG_OBJC_RETAIN(_sg.mtl.idpool.pool);
    NSNull* null = [NSNull null];
    for (int i = 0; i < _sg.mtl.idpool.num_slots; i++) {
        [_sg.mtl.idpool.pool addObject:null];
    }
    SOKOL_ASSERT([_sg.mtl.idpool.pool count] == (NSUInteger)_sg.mtl.idpool.num_slots);
    /* a queue of currently free slot indices */
    _sg.mtl.idpool.free_queue_top = 0;
    _sg.mtl.idpool.free_queue = (int*)_sg_malloc_clear((size_t)_sg.mtl.idpool.num_slots * sizeof(int));
    /* pool slot 0 is reserved! */
    for (int i = _sg.mtl.idpool.num_slots-1; i >= 1; i--) {
        _sg.mtl.idpool.free_queue[_sg.mtl.idpool.free_queue_top++] = i;
    }
    /* a circular queue which holds release items (frame index
       when a resource is to be released, and the resource's
       pool index
    */
    _sg.mtl.idpool.release_queue_front = 0;
    _sg.mtl.idpool.release_queue_back = 0;
    _sg.mtl.idpool.release_queue = (_sg_mtl_release_item_t*)_sg_malloc_clear((size_t)_sg.mtl.idpool.num_slots * sizeof(_sg_mtl_release_item_t));
    for (int i = 0; i < _sg.mtl.idpool.num_slots; i++) {
        _sg.mtl.idpool.release_queue[i].frame_index = 0;
        _sg.mtl.idpool.release_queue[i].slot_index = _SG_MTL_INVALID_SLOT_INDEX;
    }
}

_SOKOL_PRIVATE void _sg_mtl_destroy_pool(void) {
    _sg_free(_sg.mtl.idpool.release_queue);  _sg.mtl.idpool.release_queue = 0;
    _sg_free(_sg.mtl.idpool.free_queue);     _sg.mtl.idpool.free_queue = 0;
    _SG_OBJC_RELEASE(_sg.mtl.idpool.pool);
}

/* get a new free resource pool slot */
_SOKOL_PRIVATE int _sg_mtl_alloc_pool_slot(void) {
    SOKOL_ASSERT(_sg.mtl.idpool.free_queue_top > 0);
    const int slot_index = _sg.mtl.idpool.free_queue[--_sg.mtl.idpool.free_queue_top];
    SOKOL_ASSERT((slot_index > 0) && (slot_index < _sg.mtl.idpool.num_slots));
    return slot_index;
}

/* put a free resource pool slot back into the free-queue */
_SOKOL_PRIVATE void _sg_mtl_free_pool_slot(int slot_index) {
    SOKOL_ASSERT(_sg.mtl.idpool.free_queue_top < _sg.mtl.idpool.num_slots);
    SOKOL_ASSERT((slot_index > 0) && (slot_index < _sg.mtl.idpool.num_slots));
    _sg.mtl.idpool.free_queue[_sg.mtl.idpool.free_queue_top++] = slot_index;
}

/*  add an MTLResource to the pool, return pool index or 0 if input was 'nil' */
_SOKOL_PRIVATE int _sg_mtl_add_resource(id res) {
    if (nil == res) {
        return _SG_MTL_INVALID_SLOT_INDEX;
    }
    const int slot_index = _sg_mtl_alloc_pool_slot();
    // NOTE: the NSMutableArray will take ownership of its items
    SOKOL_ASSERT([NSNull null] == _sg.mtl.idpool.pool[(NSUInteger)slot_index]);
    _sg.mtl.idpool.pool[(NSUInteger)slot_index] = res;
    return slot_index;
}

/*  mark an MTLResource for release, this will put the resource into the
    deferred-release queue, and the resource will then be released N frames later,
    the special pool index 0 will be ignored (this means that a nil
    value was provided to _sg_mtl_add_resource()
*/
_SOKOL_PRIVATE void _sg_mtl_release_resource(uint32_t frame_index, int slot_index) {
    if (slot_index == _SG_MTL_INVALID_SLOT_INDEX) {
        return;
    }
    SOKOL_ASSERT((slot_index > 0) && (slot_index < _sg.mtl.idpool.num_slots));
    SOKOL_ASSERT([NSNull null] != _sg.mtl.idpool.pool[(NSUInteger)slot_index]);
    int release_index = _sg.mtl.idpool.release_queue_front++;
    if (_sg.mtl.idpool.release_queue_front >= _sg.mtl.idpool.num_slots) {
        /* wrap-around */
        _sg.mtl.idpool.release_queue_front = 0;
    }
    /* release queue full? */
    SOKOL_ASSERT(_sg.mtl.idpool.release_queue_front != _sg.mtl.idpool.release_queue_back);
    SOKOL_ASSERT(0 == _sg.mtl.idpool.release_queue[release_index].frame_index);
    const uint32_t safe_to_release_frame_index = frame_index + SG_NUM_INFLIGHT_FRAMES + 1;
    _sg.mtl.idpool.release_queue[release_index].frame_index = safe_to_release_frame_index;
    _sg.mtl.idpool.release_queue[release_index].slot_index = slot_index;
}

/* run garbage-collection pass on all resources in the release-queue */
_SOKOL_PRIVATE void _sg_mtl_garbage_collect(uint32_t frame_index) {
    while (_sg.mtl.idpool.release_queue_back != _sg.mtl.idpool.release_queue_front) {
        if (frame_index < _sg.mtl.idpool.release_queue[_sg.mtl.idpool.release_queue_back].frame_index) {
            /* don't need to check further, release-items past this are too young */
            break;
        }
        /* safe to release this resource */
        const int slot_index = _sg.mtl.idpool.release_queue[_sg.mtl.idpool.release_queue_back].slot_index;
        SOKOL_ASSERT((slot_index > 0) && (slot_index < _sg.mtl.idpool.num_slots));
        /* note: the NSMutableArray takes ownership of its items, assigning an NSNull object will
           release the object, no matter if using ARC or not
        */
        SOKOL_ASSERT(_sg.mtl.idpool.pool[(NSUInteger)slot_index] != [NSNull null]);
        _sg.mtl.idpool.pool[(NSUInteger)slot_index] = [NSNull null];
        /* put the now free pool index back on the free queue */
        _sg_mtl_free_pool_slot(slot_index);
        /* reset the release queue slot and advance the back index */
        _sg.mtl.idpool.release_queue[_sg.mtl.idpool.release_queue_back].frame_index = 0;
        _sg.mtl.idpool.release_queue[_sg.mtl.idpool.release_queue_back].slot_index = _SG_MTL_INVALID_SLOT_INDEX;
        _sg.mtl.idpool.release_queue_back++;
        if (_sg.mtl.idpool.release_queue_back >= _sg.mtl.idpool.num_slots) {
            /* wrap-around */
            _sg.mtl.idpool.release_queue_back = 0;
        }
    }
}

_SOKOL_PRIVATE id _sg_mtl_id(int slot_index) {
    return _sg.mtl.idpool.pool[(NSUInteger)slot_index];
}

_SOKOL_PRIVATE void _sg_mtl_init_sampler_cache(const sg_desc* desc) {
    SOKOL_ASSERT(desc->sampler_cache_size > 0);
    _sg_smpcache_init(&_sg.mtl.sampler_cache, desc->sampler_cache_size);
}

/* destroy the sampler cache, and release all sampler objects */
_SOKOL_PRIVATE void _sg_mtl_destroy_sampler_cache(uint32_t frame_index) {
    SOKOL_ASSERT(_sg.mtl.sampler_cache.items);
    SOKOL_ASSERT(_sg.mtl.sampler_cache.num_items <= _sg.mtl.sampler_cache.capacity);
    for (int i = 0; i < _sg.mtl.sampler_cache.num_items; i++) {
        _sg_mtl_release_resource(frame_index, (int)_sg_smpcache_sampler(&_sg.mtl.sampler_cache, i));
    }
    _sg_smpcache_discard(&_sg.mtl.sampler_cache);
}

/*
    create and add an MTLSamplerStateObject and return its resource pool index,
    reuse identical sampler state if one exists
*/
_SOKOL_PRIVATE int _sg_mtl_create_sampler(id<MTLDevice> mtl_device, const sg_image_desc* img_desc) {
    SOKOL_ASSERT(img_desc);
    int index = _sg_smpcache_find_item(&_sg.mtl.sampler_cache, img_desc);
    if (index >= 0) {
        /* reuse existing sampler */
        return (int)_sg_smpcache_sampler(&_sg.mtl.sampler_cache, index);
    }
    else {
        /* create a new Metal sampler state object and add to sampler cache */
        MTLSamplerDescriptor* mtl_desc = [[MTLSamplerDescriptor alloc] init];
        mtl_desc.sAddressMode = _sg_mtl_address_mode(img_desc->wrap_u);
        mtl_desc.tAddressMode = _sg_mtl_address_mode(img_desc->wrap_v);
        if (SG_IMAGETYPE_3D == img_desc->type) {
            mtl_desc.rAddressMode = _sg_mtl_address_mode(img_desc->wrap_w);
        }
        #if defined(_SG_TARGET_MACOS)
            mtl_desc.borderColor = _sg_mtl_border_color(img_desc->border_color);
        #endif
        mtl_desc.minFilter = _sg_mtl_minmag_filter(img_desc->min_filter);
        mtl_desc.magFilter = _sg_mtl_minmag_filter(img_desc->mag_filter);
        mtl_desc.mipFilter = _sg_mtl_mip_filter(img_desc->min_filter);
        mtl_desc.lodMinClamp = img_desc->min_lod;
        mtl_desc.lodMaxClamp = img_desc->max_lod;
        mtl_desc.maxAnisotropy = img_desc->max_anisotropy;
        mtl_desc.normalizedCoordinates = YES;
        id<MTLSamplerState> mtl_sampler = [mtl_device newSamplerStateWithDescriptor:mtl_desc];
        _SG_OBJC_RELEASE(mtl_desc);
        int sampler_handle = _sg_mtl_add_resource(mtl_sampler);
        _SG_OBJC_RELEASE(mtl_sampler);
        _sg_smpcache_add_item(&_sg.mtl.sampler_cache, img_desc, (uintptr_t)sampler_handle);
        return sampler_handle;
    }
}

_SOKOL_PRIVATE void _sg_mtl_clear_state_cache(void) {
    _sg_clear(&_sg.mtl.state_cache, sizeof(_sg.mtl.state_cache));
}

/* https://developer.apple.com/metal/Metal-Feature-Set-Tables.pdf */
_SOKOL_PRIVATE void _sg_mtl_init_caps(void) {
    #if defined(_SG_TARGET_MACOS)
        _sg.backend = SG_BACKEND_METAL_MACOS;
    #elif defined(_SG_TARGET_IOS)
        #if defined(_SG_TARGET_IOS_SIMULATOR)
            _sg.backend = SG_BACKEND_METAL_SIMULATOR;
        #else
            _sg.backend = SG_BACKEND_METAL_IOS;
        #endif
    #endif
    _sg.features.instancing = true;
    _sg.features.origin_top_left = true;
    _sg.features.multiple_render_targets = true;
    _sg.features.msaa_render_targets = true;
    _sg.features.imagetype_3d = true;
    _sg.features.imagetype_array = true;
    #if defined(_SG_TARGET_MACOS)
        _sg.features.image_clamp_to_border = true;
    #else
        _sg.features.image_clamp_to_border = false;
    #endif
    _sg.features.mrt_independent_blend_state = true;
    _sg.features.mrt_independent_write_mask = true;

    #if defined(_SG_TARGET_MACOS)
        _sg.limits.max_image_size_2d = 16 * 1024;
        _sg.limits.max_image_size_cube = 16 * 1024;
        _sg.limits.max_image_size_3d = 2 * 1024;
        _sg.limits.max_image_size_array = 16 * 1024;
        _sg.limits.max_image_array_layers = 2 * 1024;
    #else
        /* newer iOS devices support 16k textures */
        _sg.limits.max_image_size_2d = 8 * 1024;
        _sg.limits.max_image_size_cube = 8 * 1024;
        _sg.limits.max_image_size_3d = 2 * 1024;
        _sg.limits.max_image_size_array = 8 * 1024;
        _sg.limits.max_image_array_layers = 2 * 1024;
    #endif
    _sg.limits.max_vertex_attrs = SG_MAX_VERTEX_ATTRIBUTES;

    _sg_pixelformat_all(&_sg.formats[SG_PIXELFORMAT_R8]);
    _sg_pixelformat_all(&_sg.formats[SG_PIXELFORMAT_R8SN]);
    _sg_pixelformat_srm(&_sg.formats[SG_PIXELFORMAT_R8UI]);
    _sg_pixelformat_srm(&_sg.formats[SG_PIXELFORMAT_R8SI]);
    #if defined(_SG_TARGET_MACOS)
        _sg_pixelformat_all(&_sg.formats[SG_PIXELFORMAT_R16]);
        _sg_pixelformat_all(&_sg.formats[SG_PIXELFORMAT_R16SN]);
    #else
        _sg_pixelformat_sfbr(&_sg.formats[SG_PIXELFORMAT_R16]);
        _sg_pixelformat_sfbr(&_sg.formats[SG_PIXELFORMAT_R16SN]);
    #endif
    _sg_pixelformat_srm(&_sg.formats[SG_PIXELFORMAT_R16UI]);
    _sg_pixelformat_srm(&_sg.formats[SG_PIXELFORMAT_R16SI]);
    _sg_pixelformat_all(&_sg.formats[SG_PIXELFORMAT_R16F]);
    _sg_pixelformat_all(&_sg.formats[SG_PIXELFORMAT_RG8]);
    _sg_pixelformat_all(&_sg.formats[SG_PIXELFORMAT_RG8SN]);
    _sg_pixelformat_srm(&_sg.formats[SG_PIXELFORMAT_RG8UI]);
    _sg_pixelformat_srm(&_sg.formats[SG_PIXELFORMAT_RG8SI]);
    _sg_pixelformat_sr(&_sg.formats[SG_PIXELFORMAT_R32UI]);
    _sg_pixelformat_sr(&_sg.formats[SG_PIXELFORMAT_R32SI]);
    #if defined(_SG_TARGET_MACOS)
        _sg_pixelformat_all(&_sg.formats[SG_PIXELFORMAT_R32F]);
    #else
        _sg_pixelformat_sbr(&_sg.formats[SG_PIXELFORMAT_R32F]);
    #endif
    #if defined(_SG_TARGET_MACOS)
        _sg_pixelformat_all(&_sg.formats[SG_PIXELFORMAT_RG16]);
        _sg_pixelformat_all(&_sg.formats[SG_PIXELFORMAT_RG16SN]);
    #else
        _sg_pixelformat_sfbr(&_sg.formats[SG_PIXELFORMAT_RG16]);
        _sg_pixelformat_sfbr(&_sg.formats[SG_PIXELFORMAT_RG16SN]);
    #endif
    _sg_pixelformat_srm(&_sg.formats[SG_PIXELFORMAT_RG16UI]);
    _sg_pixelformat_srm(&_sg.formats[SG_PIXELFORMAT_RG16SI]);
    _sg_pixelformat_all(&_sg.formats[SG_PIXELFORMAT_RG16F]);
    _sg_pixelformat_all(&_sg.formats[SG_PIXELFORMAT_RGBA8]);
    _sg_pixelformat_all(&_sg.formats[SG_PIXELFORMAT_RGBA8SN]);
    _sg_pixelformat_srm(&_sg.formats[SG_PIXELFORMAT_RGBA8UI]);
    _sg_pixelformat_srm(&_sg.formats[SG_PIXELFORMAT_RGBA8SI]);
    _sg_pixelformat_all(&_sg.formats[SG_PIXELFORMAT_BGRA8]);
    _sg_pixelformat_all(&_sg.formats[SG_PIXELFORMAT_RGB10A2]);
    _sg_pixelformat_all(&_sg.formats[SG_PIXELFORMAT_RG11B10F]);
    #if defined(_SG_TARGET_MACOS)
        _sg_pixelformat_sf(&_sg.formats[SG_PIXELFORMAT_RGB9E5]);
        _sg_pixelformat_srm(&_sg.formats[SG_PIXELFORMAT_RG32UI]);
        _sg_pixelformat_srm(&_sg.formats[SG_PIXELFORMAT_RG32SI]);
    #else
        _sg_pixelformat_all(&_sg.formats[SG_PIXELFORMAT_RGB9E5]);
        _sg_pixelformat_sr(&_sg.formats[SG_PIXELFORMAT_RG32UI]);
        _sg_pixelformat_sr(&_sg.formats[SG_PIXELFORMAT_RG32SI]);
    #endif
    #if defined(_SG_TARGET_MACOS)
        _sg_pixelformat_all(&_sg.formats[SG_PIXELFORMAT_RG32F]);
    #else
        _sg_pixelformat_sbr(&_sg.formats[SG_PIXELFORMAT_RG32F]);
    #endif
    #if defined(_SG_TARGET_MACOS)
        _sg_pixelformat_all(&_sg.formats[SG_PIXELFORMAT_RGBA16]);
        _sg_pixelformat_all(&_sg.formats[SG_PIXELFORMAT_RGBA16SN]);
    #else
        _sg_pixelformat_sfbr(&_sg.formats[SG_PIXELFORMAT_RGBA16]);
        _sg_pixelformat_sfbr(&_sg.formats[SG_PIXELFORMAT_RGBA16SN]);
    #endif
    _sg_pixelformat_srm(&_sg.formats[SG_PIXELFORMAT_RGBA16UI]);
    _sg_pixelformat_srm(&_sg.formats[SG_PIXELFORMAT_RGBA16SI]);
    _sg_pixelformat_all(&_sg.formats[SG_PIXELFORMAT_RGBA16F]);
    #if defined(_SG_TARGET_MACOS)
        _sg_pixelformat_srm(&_sg.formats[SG_PIXELFORMAT_RGBA32UI]);
        _sg_pixelformat_srm(&_sg.formats[SG_PIXELFORMAT_RGBA32SI]);
        _sg_pixelformat_all(&_sg.formats[SG_PIXELFORMAT_RGBA32F]);
    #else
        _sg_pixelformat_sr(&_sg.formats[SG_PIXELFORMAT_RGBA32UI]);
        _sg_pixelformat_sr(&_sg.formats[SG_PIXELFORMAT_RGBA32SI]);
        _sg_pixelformat_sr(&_sg.formats[SG_PIXELFORMAT_RGBA32F]);
    #endif
    _sg_pixelformat_srmd(&_sg.formats[SG_PIXELFORMAT_DEPTH]);
    _sg_pixelformat_srmd(&_sg.formats[SG_PIXELFORMAT_DEPTH_STENCIL]);
    #if defined(_SG_TARGET_MACOS)
        _sg_pixelformat_sf(&_sg.formats[SG_PIXELFORMAT_BC1_RGBA]);
        _sg_pixelformat_sf(&_sg.formats[SG_PIXELFORMAT_BC2_RGBA]);
        _sg_pixelformat_sf(&_sg.formats[SG_PIXELFORMAT_BC3_RGBA]);
        _sg_pixelformat_sf(&_sg.formats[SG_PIXELFORMAT_BC4_R]);
        _sg_pixelformat_sf(&_sg.formats[SG_PIXELFORMAT_BC4_RSN]);
        _sg_pixelformat_sf(&_sg.formats[SG_PIXELFORMAT_BC5_RG]);
        _sg_pixelformat_sf(&_sg.formats[SG_PIXELFORMAT_BC5_RGSN]);
        _sg_pixelformat_sf(&_sg.formats[SG_PIXELFORMAT_BC6H_RGBF]);
        _sg_pixelformat_sf(&_sg.formats[SG_PIXELFORMAT_BC6H_RGBUF]);
        _sg_pixelformat_sf(&_sg.formats[SG_PIXELFORMAT_BC7_RGBA]);
    #else
        _sg_pixelformat_sf(&_sg.formats[SG_PIXELFORMAT_PVRTC_RGB_2BPP]);
        _sg_pixelformat_sf(&_sg.formats[SG_PIXELFORMAT_PVRTC_RGB_4BPP]);
        _sg_pixelformat_sf(&_sg.formats[SG_PIXELFORMAT_PVRTC_RGBA_2BPP]);
        _sg_pixelformat_sf(&_sg.formats[SG_PIXELFORMAT_PVRTC_RGBA_4BPP]);
        _sg_pixelformat_sf(&_sg.formats[SG_PIXELFORMAT_ETC2_RGB8]);
        _sg_pixelformat_sf(&_sg.formats[SG_PIXELFORMAT_ETC2_RGB8A1]);
        _sg_pixelformat_sf(&_sg.formats[SG_PIXELFORMAT_ETC2_RGBA8]);
        _sg_pixelformat_sf(&_sg.formats[SG_PIXELFORMAT_ETC2_RG11]);
        _sg_pixelformat_sf(&_sg.formats[SG_PIXELFORMAT_ETC2_RG11SN]);
    #endif
}

/*-- main Metal backend state and functions ----------------------------------*/
_SOKOL_PRIVATE void _sg_mtl_setup_backend(const sg_desc* desc) {
    /* assume already zero-initialized */
    SOKOL_ASSERT(desc);
    SOKOL_ASSERT(desc->context.metal.device);
    SOKOL_ASSERT(desc->context.metal.renderpass_descriptor_cb || desc->context.metal.renderpass_descriptor_userdata_cb);
    SOKOL_ASSERT(desc->context.metal.drawable_cb || desc->context.metal.drawable_userdata_cb);
    SOKOL_ASSERT(desc->uniform_buffer_size > 0);
    _sg_mtl_init_pool(desc);
    _sg_mtl_init_sampler_cache(desc);
    _sg_mtl_clear_state_cache();
    _sg.mtl.valid = true;
    _sg.mtl.renderpass_descriptor_cb = desc->context.metal.renderpass_descriptor_cb;
    _sg.mtl.renderpass_descriptor_userdata_cb = desc->context.metal.renderpass_descriptor_userdata_cb;
    _sg.mtl.drawable_cb = desc->context.metal.drawable_cb;
    _sg.mtl.drawable_userdata_cb = desc->context.metal.drawable_userdata_cb;
    _sg.mtl.user_data = desc->context.metal.user_data;
    _sg.mtl.frame_index = 1;
    _sg.mtl.ub_size = desc->uniform_buffer_size;
    _sg.mtl.sem = dispatch_semaphore_create(SG_NUM_INFLIGHT_FRAMES);
    _sg.mtl.device = (__bridge id<MTLDevice>) desc->context.metal.device;
    _sg.mtl.cmd_queue = [_sg.mtl.device newCommandQueue];
    for (int i = 0; i < SG_NUM_INFLIGHT_FRAMES; i++) {
        _sg.mtl.uniform_buffers[i] = [_sg.mtl.device
            newBufferWithLength:(NSUInteger)_sg.mtl.ub_size
            options:MTLResourceCPUCacheModeWriteCombined|MTLResourceStorageModeShared
        ];
    }
    _sg_mtl_init_caps();
}

_SOKOL_PRIVATE void _sg_mtl_discard_backend(void) {
    SOKOL_ASSERT(_sg.mtl.valid);
    /* wait for the last frame to finish */
    for (int i = 0; i < SG_NUM_INFLIGHT_FRAMES; i++) {
        dispatch_semaphore_wait(_sg.mtl.sem, DISPATCH_TIME_FOREVER);
    }
    /* semaphore must be "relinquished" before destruction */
    for (int i = 0; i < SG_NUM_INFLIGHT_FRAMES; i++) {
        dispatch_semaphore_signal(_sg.mtl.sem);
    }
    _sg_mtl_destroy_sampler_cache(_sg.mtl.frame_index);
    _sg_mtl_garbage_collect(_sg.mtl.frame_index + SG_NUM_INFLIGHT_FRAMES + 2);
    _sg_mtl_destroy_pool();
    _sg.mtl.valid = false;

    _SG_OBJC_RELEASE(_sg.mtl.sem);
    _SG_OBJC_RELEASE(_sg.mtl.device);
    _SG_OBJC_RELEASE(_sg.mtl.cmd_queue);
    for (int i = 0; i < SG_NUM_INFLIGHT_FRAMES; i++) {
        _SG_OBJC_RELEASE(_sg.mtl.uniform_buffers[i]);
    }
    /* NOTE: MTLCommandBuffer and MTLRenderCommandEncoder are auto-released */
    _sg.mtl.cmd_buffer = nil;
    _sg.mtl.present_cmd_buffer = nil;
    _sg.mtl.cmd_encoder = nil;
}

_SOKOL_PRIVATE void _sg_mtl_bind_uniform_buffers(void) {
    SOKOL_ASSERT(nil != _sg.mtl.cmd_encoder);
    for (int slot = 0; slot < SG_MAX_SHADERSTAGE_UBS; slot++) {
        [_sg.mtl.cmd_encoder
            setVertexBuffer:_sg.mtl.uniform_buffers[_sg.mtl.cur_frame_rotate_index]
            offset:0
            atIndex:(NSUInteger)slot];
        [_sg.mtl.cmd_encoder
            setFragmentBuffer:_sg.mtl.uniform_buffers[_sg.mtl.cur_frame_rotate_index]
            offset:0
            atIndex:(NSUInteger)slot];
    }
}

_SOKOL_PRIVATE void _sg_mtl_reset_state_cache(void) {
    _sg_mtl_clear_state_cache();

    /* need to restore the uniform buffer binding (normally happens in
       _sg_mtl_begin_pass()
    */
    if (nil != _sg.mtl.cmd_encoder) {
        _sg_mtl_bind_uniform_buffers();
    }
}

_SOKOL_PRIVATE sg_resource_state _sg_mtl_create_context(_sg_context_t* ctx) {
    SOKOL_ASSERT(ctx);
    _SOKOL_UNUSED(ctx);
    return SG_RESOURCESTATE_VALID;
}

_SOKOL_PRIVATE void _sg_mtl_discard_context(_sg_context_t* ctx) {
    SOKOL_ASSERT(ctx);
    _SOKOL_UNUSED(ctx);
    /* empty */
}

_SOKOL_PRIVATE void _sg_mtl_activate_context(_sg_context_t* ctx) {
    _SOKOL_UNUSED(ctx);
    _sg_mtl_clear_state_cache();
}

_SOKOL_PRIVATE sg_resource_state _sg_mtl_create_buffer(_sg_buffer_t* buf, const sg_buffer_desc* desc) {
    SOKOL_ASSERT(buf && desc);
    _sg_buffer_common_init(&buf->cmn, desc);
    const bool injected = (0 != desc->mtl_buffers[0]);
    MTLResourceOptions mtl_options = _sg_mtl_buffer_resource_options(buf->cmn.usage);
    for (int slot = 0; slot < buf->cmn.num_slots; slot++) {
        id<MTLBuffer> mtl_buf;
        if (injected) {
            SOKOL_ASSERT(desc->mtl_buffers[slot]);
            mtl_buf = (__bridge id<MTLBuffer>) desc->mtl_buffers[slot];
        }
        else {
            if (buf->cmn.usage == SG_USAGE_IMMUTABLE) {
                SOKOL_ASSERT(desc->data.ptr);
                mtl_buf = [_sg.mtl.device newBufferWithBytes:desc->data.ptr length:(NSUInteger)buf->cmn.size options:mtl_options];
            }
            else {
                mtl_buf = [_sg.mtl.device newBufferWithLength:(NSUInteger)buf->cmn.size options:mtl_options];
            }
        }
        buf->mtl.buf[slot] = _sg_mtl_add_resource(mtl_buf);
        _SG_OBJC_RELEASE(mtl_buf);
    }
    return SG_RESOURCESTATE_VALID;
}

_SOKOL_PRIVATE void _sg_mtl_discard_buffer(_sg_buffer_t* buf) {
    SOKOL_ASSERT(buf);
    for (int slot = 0; slot < buf->cmn.num_slots; slot++) {
        /* it's valid to call release resource with '0' */
        _sg_mtl_release_resource(_sg.mtl.frame_index, buf->mtl.buf[slot]);
    }
}

_SOKOL_PRIVATE void _sg_mtl_copy_image_data(const _sg_image_t* img, __unsafe_unretained id<MTLTexture> mtl_tex, const sg_image_data* data) {
    const int num_faces = (img->cmn.type == SG_IMAGETYPE_CUBE) ? 6:1;
    const int num_slices = (img->cmn.type == SG_IMAGETYPE_ARRAY) ? img->cmn.num_slices : 1;
    for (int face_index = 0; face_index < num_faces; face_index++) {
        for (int mip_index = 0; mip_index < img->cmn.num_mipmaps; mip_index++) {
            SOKOL_ASSERT(data->subimage[face_index][mip_index].ptr);
            SOKOL_ASSERT(data->subimage[face_index][mip_index].size > 0);
            const uint8_t* data_ptr = (const uint8_t*)data->subimage[face_index][mip_index].ptr;
            const int mip_width = _sg_max(img->cmn.width >> mip_index, 1);
            const int mip_height = _sg_max(img->cmn.height >> mip_index, 1);
            /* special case PVRTC formats: bytePerRow and bytesPerImage must be 0 */
            int bytes_per_row = 0;
            int bytes_per_slice = 0;
            if (!_sg_mtl_is_pvrtc(img->cmn.pixel_format)) {
                bytes_per_row = _sg_row_pitch(img->cmn.pixel_format, mip_width, 1);
                bytes_per_slice = _sg_surface_pitch(img->cmn.pixel_format, mip_width, mip_height, 1);
            }
            /* bytesPerImage special case: https://developer.apple.com/documentation/metal/mtltexture/1515679-replaceregion

                "Supply a nonzero value only when you copy data to a MTLTextureType3D type texture"
            */
            MTLRegion region;
            int bytes_per_image;
            if (img->cmn.type == SG_IMAGETYPE_3D) {
                const int mip_depth = _sg_max(img->cmn.num_slices >> mip_index, 1);
                region = MTLRegionMake3D(0, 0, 0, (NSUInteger)mip_width, (NSUInteger)mip_height, (NSUInteger)mip_depth);
                bytes_per_image = bytes_per_slice;
                /* FIXME: apparently the minimal bytes_per_image size for 3D texture
                 is 4 KByte... somehow need to handle this */
            }
            else {
                region = MTLRegionMake2D(0, 0, (NSUInteger)mip_width, (NSUInteger)mip_height);
                bytes_per_image = 0;
            }

            for (int slice_index = 0; slice_index < num_slices; slice_index++) {
                const int mtl_slice_index = (img->cmn.type == SG_IMAGETYPE_CUBE) ? face_index : slice_index;
                const int slice_offset = slice_index * bytes_per_slice;
                SOKOL_ASSERT((slice_offset + bytes_per_slice) <= (int)data->subimage[face_index][mip_index].size);
                [mtl_tex replaceRegion:region
                    mipmapLevel:(NSUInteger)mip_index
                    slice:(NSUInteger)mtl_slice_index
                    withBytes:data_ptr + slice_offset
                    bytesPerRow:(NSUInteger)bytes_per_row
                    bytesPerImage:(NSUInteger)bytes_per_image];
            }
        }
    }
}

/*
    FIXME: METAL RESOURCE STORAGE MODE FOR macOS AND iOS

    For immutable textures on macOS, the recommended procedure is to create
    a MTLStorageModeManaged texture with the immutable content first,
    and then use the GPU to blit the content into a MTLStorageModePrivate
    texture before the first use.

    On iOS use the same one-time-blit procedure, but from a
    MTLStorageModeShared to a MTLStorageModePrivate texture.

    It probably makes sense to handle this in a separate 'resource manager'
    with a recycable pool of blit-source-textures?
*/

/* initialize MTLTextureDescritor with common attributes */
_SOKOL_PRIVATE bool _sg_mtl_init_texdesc_common(MTLTextureDescriptor* mtl_desc, _sg_image_t* img) {
    mtl_desc.textureType = _sg_mtl_texture_type(img->cmn.type);
    mtl_desc.pixelFormat = _sg_mtl_pixel_format(img->cmn.pixel_format);
    if (MTLPixelFormatInvalid == mtl_desc.pixelFormat) {
        SG_LOG("Unsupported texture pixel format!\n");
        return false;
    }
    mtl_desc.width = (NSUInteger)img->cmn.width;
    mtl_desc.height = (NSUInteger)img->cmn.height;
    if (SG_IMAGETYPE_3D == img->cmn.type) {
        mtl_desc.depth = (NSUInteger)img->cmn.num_slices;
    }
    else {
        mtl_desc.depth = 1;
    }
    mtl_desc.mipmapLevelCount = (NSUInteger)img->cmn.num_mipmaps;
    if (SG_IMAGETYPE_ARRAY == img->cmn.type) {
        mtl_desc.arrayLength = (NSUInteger)img->cmn.num_slices;
    }
    else {
        mtl_desc.arrayLength = 1;
    }
    mtl_desc.usage = MTLTextureUsageShaderRead;
    if (img->cmn.render_target) {
        mtl_desc.usage |= MTLTextureUsageRenderTarget;
    }
    MTLResourceOptions res_options = 0;
    if (img->cmn.usage != SG_USAGE_IMMUTABLE) {
        res_options |= MTLResourceCPUCacheModeWriteCombined;
    }
    #if defined(_SG_TARGET_MACOS)
        /* macOS: use managed textures */
        res_options |= MTLResourceStorageModeManaged;
    #else
        /* iOS: use CPU/GPU shared memory */
        res_options |= MTLResourceStorageModeShared;
    #endif
    mtl_desc.resourceOptions = res_options;
    return true;
}

/* initialize MTLTextureDescritor with rendertarget attributes */
_SOKOL_PRIVATE void _sg_mtl_init_texdesc_rt(MTLTextureDescriptor* mtl_desc, _sg_image_t* img) {
    SOKOL_ASSERT(img->cmn.render_target);
    _SOKOL_UNUSED(img);
    /* render targets are only visible to the GPU */
    mtl_desc.resourceOptions = MTLResourceStorageModePrivate;
    /* non-MSAA render targets are shader-readable */
    mtl_desc.usage = MTLTextureUsageShaderRead | MTLTextureUsageRenderTarget;
}

/* initialize MTLTextureDescritor with MSAA attributes */
_SOKOL_PRIVATE void _sg_mtl_init_texdesc_rt_msaa(MTLTextureDescriptor* mtl_desc, _sg_image_t* img) {
    SOKOL_ASSERT(img->cmn.sample_count > 1);
    /* render targets are only visible to the GPU */
    mtl_desc.resourceOptions = MTLResourceStorageModePrivate;
    /* MSAA render targets are not shader-readable (instead they are resolved) */
    mtl_desc.usage = MTLTextureUsageRenderTarget;
    mtl_desc.textureType = MTLTextureType2DMultisample;
    mtl_desc.depth = 1;
    mtl_desc.arrayLength = 1;
    mtl_desc.mipmapLevelCount = 1;
    mtl_desc.sampleCount = (NSUInteger)img->cmn.sample_count;
}

_SOKOL_PRIVATE sg_resource_state _sg_mtl_create_image(_sg_image_t* img, const sg_image_desc* desc) {
    SOKOL_ASSERT(img && desc);
    _sg_image_common_init(&img->cmn, desc);
    const bool injected = (0 != desc->mtl_textures[0]);
    const bool msaa = (img->cmn.sample_count > 1);

    /* first initialize all Metal resource pool slots to 'empty' */
    for (int i = 0; i < SG_NUM_INFLIGHT_FRAMES; i++) {
        img->mtl.tex[i] = _sg_mtl_add_resource(nil);
    }
    img->mtl.sampler_state = _sg_mtl_add_resource(nil);
    img->mtl.depth_tex = _sg_mtl_add_resource(nil);
    img->mtl.msaa_tex = _sg_mtl_add_resource(nil);

    /* initialize a Metal texture descriptor with common attributes */
    MTLTextureDescriptor* mtl_desc = [[MTLTextureDescriptor alloc] init];
    if (!_sg_mtl_init_texdesc_common(mtl_desc, img)) {
        _SG_OBJC_RELEASE(mtl_desc);
        return SG_RESOURCESTATE_FAILED;
    }

    /* special case depth-stencil-buffer? */
    if (_sg_is_valid_rendertarget_depth_format(img->cmn.pixel_format)) {
        /* depth-stencil buffer texture must always be a render target */
        SOKOL_ASSERT(img->cmn.render_target);
        SOKOL_ASSERT(img->cmn.type == SG_IMAGETYPE_2D);
        SOKOL_ASSERT(img->cmn.num_mipmaps == 1);
        SOKOL_ASSERT(!injected);
        if (msaa) {
            _sg_mtl_init_texdesc_rt_msaa(mtl_desc, img);
        }
        else {
            _sg_mtl_init_texdesc_rt(mtl_desc, img);
        }
        id<MTLTexture> tex = [_sg.mtl.device newTextureWithDescriptor:mtl_desc];
        SOKOL_ASSERT(nil != tex);
        img->mtl.depth_tex = _sg_mtl_add_resource(tex);
        _SG_OBJC_RELEASE(tex);
    }
    else {
        /* create the color texture
            In case this is a render target without MSAA, add the relevant
            render-target descriptor attributes.
            In case this is a render target *with* MSAA, the color texture
            will serve as MSAA-resolve target (not as render target), and rendering
            will go into a separate render target texture of type
            MTLTextureType2DMultisample.
        */
        if (img->cmn.render_target && !msaa) {
            _sg_mtl_init_texdesc_rt(mtl_desc, img);
        }
        for (int slot = 0; slot < img->cmn.num_slots; slot++) {
            id<MTLTexture> tex;
            if (injected) {
                SOKOL_ASSERT(desc->mtl_textures[slot]);
                tex = (__bridge id<MTLTexture>) desc->mtl_textures[slot];
            }
            else {
                tex = [_sg.mtl.device newTextureWithDescriptor:mtl_desc];
                if ((img->cmn.usage == SG_USAGE_IMMUTABLE) && !img->cmn.render_target) {
                    _sg_mtl_copy_image_data(img, tex, &desc->data);
                }
            }
            img->mtl.tex[slot] = _sg_mtl_add_resource(tex);
            _SG_OBJC_RELEASE(tex);
        }

        /* if MSAA color render target, create an additional MSAA render-surface texture */
        if (img->cmn.render_target && msaa) {
            _sg_mtl_init_texdesc_rt_msaa(mtl_desc, img);
            id<MTLTexture> tex = [_sg.mtl.device newTextureWithDescriptor:mtl_desc];
            img->mtl.msaa_tex = _sg_mtl_add_resource(tex);
            _SG_OBJC_RELEASE(tex);
        }

        /* create (possibly shared) sampler state */
        img->mtl.sampler_state = _sg_mtl_create_sampler(_sg.mtl.device, desc);
    }
    _SG_OBJC_RELEASE(mtl_desc);
    return SG_RESOURCESTATE_VALID;
}

_SOKOL_PRIVATE void _sg_mtl_discard_image(_sg_image_t* img) {
    SOKOL_ASSERT(img);
    /* it's valid to call release resource with a 'null resource' */
    for (int slot = 0; slot < img->cmn.num_slots; slot++) {
        _sg_mtl_release_resource(_sg.mtl.frame_index, img->mtl.tex[slot]);
    }
    _sg_mtl_release_resource(_sg.mtl.frame_index, img->mtl.depth_tex);
    _sg_mtl_release_resource(_sg.mtl.frame_index, img->mtl.msaa_tex);
    /* NOTE: sampler state objects are shared and not released until shutdown */
}

_SOKOL_PRIVATE id<MTLLibrary> _sg_mtl_compile_library(const char* src) {
    NSError* err = NULL;
    id<MTLLibrary> lib = [_sg.mtl.device
        newLibraryWithSource:[NSString stringWithUTF8String:src]
        options:nil
        error:&err
    ];
    if (err) {
        SG_LOG([err.localizedDescription UTF8String]);
    }
    return lib;
}

_SOKOL_PRIVATE id<MTLLibrary> _sg_mtl_library_from_bytecode(const void* ptr, size_t num_bytes) {
    NSError* err = NULL;
    dispatch_data_t lib_data = dispatch_data_create(ptr, num_bytes, NULL, DISPATCH_DATA_DESTRUCTOR_DEFAULT);
    id<MTLLibrary> lib = [_sg.mtl.device newLibraryWithData:lib_data error:&err];
    if (err) {
        SG_LOG([err.localizedDescription UTF8String]);
    }
    _SG_OBJC_RELEASE(lib_data);
    return lib;
}

_SOKOL_PRIVATE sg_resource_state _sg_mtl_create_shader(_sg_shader_t* shd, const sg_shader_desc* desc) {
    SOKOL_ASSERT(shd && desc);

    _sg_shader_common_init(&shd->cmn, desc);

    /* create metal libray objects and lookup entry functions */
    id<MTLLibrary> vs_lib = nil;
    id<MTLLibrary> fs_lib = nil;
    id<MTLFunction> vs_func = nil;
    id<MTLFunction> fs_func = nil;
    const char* vs_entry = desc->vs.entry;
    const char* fs_entry = desc->fs.entry;
    if (desc->vs.bytecode.ptr && desc->fs.bytecode.ptr) {
        /* separate byte code provided */
        vs_lib = _sg_mtl_library_from_bytecode(desc->vs.bytecode.ptr, desc->vs.bytecode.size);
        fs_lib = _sg_mtl_library_from_bytecode(desc->fs.bytecode.ptr, desc->fs.bytecode.size);
        if ((nil == vs_lib) || (nil == fs_lib)) {
            goto failed;
        }
        vs_func = [vs_lib newFunctionWithName:[NSString stringWithUTF8String:vs_entry]];
        fs_func = [fs_lib newFunctionWithName:[NSString stringWithUTF8String:fs_entry]];
    }
    else if (desc->vs.source && desc->fs.source) {
        /* separate sources provided */
        vs_lib = _sg_mtl_compile_library(desc->vs.source);
        fs_lib = _sg_mtl_compile_library(desc->fs.source);
        if ((nil == vs_lib) || (nil == fs_lib)) {
            goto failed;
        }
        vs_func = [vs_lib newFunctionWithName:[NSString stringWithUTF8String:vs_entry]];
        fs_func = [fs_lib newFunctionWithName:[NSString stringWithUTF8String:fs_entry]];
    }
    else {
        goto failed;
    }
    if (nil == vs_func) {
        SG_LOG("vertex shader entry function not found\n");
        goto failed;
    }
    if (nil == fs_func) {
        SG_LOG("fragment shader entry function not found\n");
        goto failed;
    }
    /* it is legal to call _sg_mtl_add_resource with a nil value, this will return a special 0xFFFFFFFF index */
    shd->mtl.stage[SG_SHADERSTAGE_VS].mtl_lib  = _sg_mtl_add_resource(vs_lib);
    _SG_OBJC_RELEASE(vs_lib);
    shd->mtl.stage[SG_SHADERSTAGE_FS].mtl_lib  = _sg_mtl_add_resource(fs_lib);
    _SG_OBJC_RELEASE(fs_lib);
    shd->mtl.stage[SG_SHADERSTAGE_VS].mtl_func = _sg_mtl_add_resource(vs_func);
    _SG_OBJC_RELEASE(vs_func);
    shd->mtl.stage[SG_SHADERSTAGE_FS].mtl_func = _sg_mtl_add_resource(fs_func);
    _SG_OBJC_RELEASE(fs_func);
    return SG_RESOURCESTATE_VALID;
failed:
    if (vs_lib != nil) {
        _SG_OBJC_RELEASE(vs_lib);
    }
    if (fs_lib != nil) {
        _SG_OBJC_RELEASE(fs_lib);
    }
    if (vs_func != nil) {
        _SG_OBJC_RELEASE(vs_func);
    }
    if (fs_func != nil) {
        _SG_OBJC_RELEASE(fs_func);
    }
    return SG_RESOURCESTATE_FAILED;
}

_SOKOL_PRIVATE void _sg_mtl_discard_shader(_sg_shader_t* shd) {
    SOKOL_ASSERT(shd);
    /* it is valid to call _sg_mtl_release_resource with a 'null resource' */
    _sg_mtl_release_resource(_sg.mtl.frame_index, shd->mtl.stage[SG_SHADERSTAGE_VS].mtl_func);
    _sg_mtl_release_resource(_sg.mtl.frame_index, shd->mtl.stage[SG_SHADERSTAGE_VS].mtl_lib);
    _sg_mtl_release_resource(_sg.mtl.frame_index, shd->mtl.stage[SG_SHADERSTAGE_FS].mtl_func);
    _sg_mtl_release_resource(_sg.mtl.frame_index, shd->mtl.stage[SG_SHADERSTAGE_FS].mtl_lib);
}

_SOKOL_PRIVATE sg_resource_state _sg_mtl_create_pipeline(_sg_pipeline_t* pip, _sg_shader_t* shd, const sg_pipeline_desc* desc) {
    SOKOL_ASSERT(pip && shd && desc);
    SOKOL_ASSERT(desc->shader.id == shd->slot.id);

    pip->shader = shd;
    _sg_pipeline_common_init(&pip->cmn, desc);

    sg_primitive_type prim_type = desc->primitive_type;
    pip->mtl.prim_type = _sg_mtl_primitive_type(prim_type);
    pip->mtl.index_size = _sg_mtl_index_size(pip->cmn.index_type);
    if (SG_INDEXTYPE_NONE != pip->cmn.index_type) {
        pip->mtl.index_type = _sg_mtl_index_type(pip->cmn.index_type);
    }
    pip->mtl.cull_mode = _sg_mtl_cull_mode(desc->cull_mode);
    pip->mtl.winding = _sg_mtl_winding(desc->face_winding);
    pip->mtl.stencil_ref = desc->stencil.ref;

    /* create vertex-descriptor */
    MTLVertexDescriptor* vtx_desc = [MTLVertexDescriptor vertexDescriptor];
    for (NSUInteger attr_index = 0; attr_index < SG_MAX_VERTEX_ATTRIBUTES; attr_index++) {
        const sg_vertex_attr_desc* a_desc = &desc->layout.attrs[attr_index];
        if (a_desc->format == SG_VERTEXFORMAT_INVALID) {
            break;
        }
        SOKOL_ASSERT(a_desc->buffer_index < SG_MAX_SHADERSTAGE_BUFFERS);
        vtx_desc.attributes[attr_index].format = _sg_mtl_vertex_format(a_desc->format);
        vtx_desc.attributes[attr_index].offset = (NSUInteger)a_desc->offset;
        vtx_desc.attributes[attr_index].bufferIndex = (NSUInteger)(a_desc->buffer_index + SG_MAX_SHADERSTAGE_UBS);
        pip->cmn.vertex_layout_valid[a_desc->buffer_index] = true;
    }
    for (NSUInteger layout_index = 0; layout_index < SG_MAX_SHADERSTAGE_BUFFERS; layout_index++) {
        if (pip->cmn.vertex_layout_valid[layout_index]) {
            const sg_buffer_layout_desc* l_desc = &desc->layout.buffers[layout_index];
            const NSUInteger mtl_vb_slot = layout_index + SG_MAX_SHADERSTAGE_UBS;
            SOKOL_ASSERT(l_desc->stride > 0);
            vtx_desc.layouts[mtl_vb_slot].stride = (NSUInteger)l_desc->stride;
            vtx_desc.layouts[mtl_vb_slot].stepFunction = _sg_mtl_step_function(l_desc->step_func);
            vtx_desc.layouts[mtl_vb_slot].stepRate = (NSUInteger)l_desc->step_rate;
            if (SG_VERTEXSTEP_PER_INSTANCE == l_desc->step_func) {
                // NOTE: not actually used in _sg_mtl_draw()
                pip->cmn.use_instanced_draw = true;
            }
        }
    }

    /* render-pipeline descriptor */
    MTLRenderPipelineDescriptor* rp_desc = [[MTLRenderPipelineDescriptor alloc] init];
    rp_desc.vertexDescriptor = vtx_desc;
    SOKOL_ASSERT(shd->mtl.stage[SG_SHADERSTAGE_VS].mtl_func != _SG_MTL_INVALID_SLOT_INDEX);
    rp_desc.vertexFunction = _sg_mtl_id(shd->mtl.stage[SG_SHADERSTAGE_VS].mtl_func);
    SOKOL_ASSERT(shd->mtl.stage[SG_SHADERSTAGE_FS].mtl_func != _SG_MTL_INVALID_SLOT_INDEX);
    rp_desc.fragmentFunction = _sg_mtl_id(shd->mtl.stage[SG_SHADERSTAGE_FS].mtl_func);
    rp_desc.rasterSampleCount = (NSUInteger)desc->sample_count;
    rp_desc.alphaToCoverageEnabled = desc->alpha_to_coverage_enabled;
    rp_desc.alphaToOneEnabled = NO;
    rp_desc.rasterizationEnabled = YES;
    rp_desc.depthAttachmentPixelFormat = _sg_mtl_pixel_format(desc->depth.pixel_format);
    if (desc->depth.pixel_format == SG_PIXELFORMAT_DEPTH_STENCIL) {
        rp_desc.stencilAttachmentPixelFormat = _sg_mtl_pixel_format(desc->depth.pixel_format);
    }
    /* FIXME: this only works on macOS 10.13!
    for (int i = 0; i < (SG_MAX_SHADERSTAGE_UBS+SG_MAX_SHADERSTAGE_BUFFERS); i++) {
        rp_desc.vertexBuffers[i].mutability = MTLMutabilityImmutable;
    }
    for (int i = 0; i < SG_MAX_SHADERSTAGE_UBS; i++) {
        rp_desc.fragmentBuffers[i].mutability = MTLMutabilityImmutable;
    }
    */
    for (NSUInteger i = 0; i < (NSUInteger)desc->color_count; i++) {
        SOKOL_ASSERT(i < SG_MAX_COLOR_ATTACHMENTS);
        const sg_color_state* cs = &desc->colors[i];
        rp_desc.colorAttachments[i].pixelFormat = _sg_mtl_pixel_format(cs->pixel_format);
        rp_desc.colorAttachments[i].writeMask = _sg_mtl_color_write_mask(cs->write_mask);
        rp_desc.colorAttachments[i].blendingEnabled = cs->blend.enabled;
        rp_desc.colorAttachments[i].alphaBlendOperation = _sg_mtl_blend_op(cs->blend.op_alpha);
        rp_desc.colorAttachments[i].rgbBlendOperation = _sg_mtl_blend_op(cs->blend.op_rgb);
        rp_desc.colorAttachments[i].destinationAlphaBlendFactor = _sg_mtl_blend_factor(cs->blend.dst_factor_alpha);
        rp_desc.colorAttachments[i].destinationRGBBlendFactor = _sg_mtl_blend_factor(cs->blend.dst_factor_rgb);
        rp_desc.colorAttachments[i].sourceAlphaBlendFactor = _sg_mtl_blend_factor(cs->blend.src_factor_alpha);
        rp_desc.colorAttachments[i].sourceRGBBlendFactor = _sg_mtl_blend_factor(cs->blend.src_factor_rgb);
    }
    NSError* err = NULL;
    id<MTLRenderPipelineState> mtl_rps = [_sg.mtl.device newRenderPipelineStateWithDescriptor:rp_desc error:&err];
    _SG_OBJC_RELEASE(rp_desc);
    if (nil == mtl_rps) {
        SOKOL_ASSERT(err);
        SG_LOG([err.localizedDescription UTF8String]);
        return SG_RESOURCESTATE_FAILED;
    }

    /* depth-stencil-state */
    MTLDepthStencilDescriptor* ds_desc = [[MTLDepthStencilDescriptor alloc] init];
    ds_desc.depthCompareFunction = _sg_mtl_compare_func(desc->depth.compare);
    ds_desc.depthWriteEnabled = desc->depth.write_enabled;
    if (desc->stencil.enabled) {
        const sg_stencil_face_state* sb = &desc->stencil.back;
        ds_desc.backFaceStencil = [[MTLStencilDescriptor alloc] init];
        ds_desc.backFaceStencil.stencilFailureOperation = _sg_mtl_stencil_op(sb->fail_op);
        ds_desc.backFaceStencil.depthFailureOperation = _sg_mtl_stencil_op(sb->depth_fail_op);
        ds_desc.backFaceStencil.depthStencilPassOperation = _sg_mtl_stencil_op(sb->pass_op);
        ds_desc.backFaceStencil.stencilCompareFunction = _sg_mtl_compare_func(sb->compare);
        ds_desc.backFaceStencil.readMask = desc->stencil.read_mask;
        ds_desc.backFaceStencil.writeMask = desc->stencil.write_mask;
        const sg_stencil_face_state* sf = &desc->stencil.front;
        ds_desc.frontFaceStencil = [[MTLStencilDescriptor alloc] init];
        ds_desc.frontFaceStencil.stencilFailureOperation = _sg_mtl_stencil_op(sf->fail_op);
        ds_desc.frontFaceStencil.depthFailureOperation = _sg_mtl_stencil_op(sf->depth_fail_op);
        ds_desc.frontFaceStencil.depthStencilPassOperation = _sg_mtl_stencil_op(sf->pass_op);
        ds_desc.frontFaceStencil.stencilCompareFunction = _sg_mtl_compare_func(sf->compare);
        ds_desc.frontFaceStencil.readMask = desc->stencil.read_mask;
        ds_desc.frontFaceStencil.writeMask = desc->stencil.write_mask;
    }
    id<MTLDepthStencilState> mtl_dss = [_sg.mtl.device newDepthStencilStateWithDescriptor:ds_desc];
    _SG_OBJC_RELEASE(ds_desc);
    pip->mtl.rps = _sg_mtl_add_resource(mtl_rps);
    _SG_OBJC_RELEASE(mtl_rps);
    pip->mtl.dss = _sg_mtl_add_resource(mtl_dss);
    _SG_OBJC_RELEASE(mtl_dss);
    return SG_RESOURCESTATE_VALID;
}

_SOKOL_PRIVATE void _sg_mtl_discard_pipeline(_sg_pipeline_t* pip) {
    SOKOL_ASSERT(pip);
    /* it's valid to call release resource with a 'null resource' */
    _sg_mtl_release_resource(_sg.mtl.frame_index, pip->mtl.rps);
    _sg_mtl_release_resource(_sg.mtl.frame_index, pip->mtl.dss);
}

_SOKOL_PRIVATE sg_resource_state _sg_mtl_create_pass(_sg_pass_t* pass, _sg_image_t** att_images, const sg_pass_desc* desc) {
    SOKOL_ASSERT(pass && desc);
    SOKOL_ASSERT(att_images && att_images[0]);

    _sg_pass_common_init(&pass->cmn, desc);

    /* copy image pointers */
    const sg_pass_attachment_desc* att_desc;
    for (int i = 0; i < pass->cmn.num_color_atts; i++) {
        att_desc = &desc->color_attachments[i];
        if (att_desc->image.id != SG_INVALID_ID) {
            SOKOL_ASSERT(att_desc->image.id != SG_INVALID_ID);
            SOKOL_ASSERT(0 == pass->mtl.color_atts[i].image);
            SOKOL_ASSERT(att_images[i] && (att_images[i]->slot.id == att_desc->image.id));
            SOKOL_ASSERT(_sg_is_valid_rendertarget_color_format(att_images[i]->cmn.pixel_format));
            pass->mtl.color_atts[i].image = att_images[i];
        }
    }
    SOKOL_ASSERT(0 == pass->mtl.ds_att.image);
    att_desc = &desc->depth_stencil_attachment;
    if (att_desc->image.id != SG_INVALID_ID) {
        const int ds_img_index = SG_MAX_COLOR_ATTACHMENTS;
        SOKOL_ASSERT(att_images[ds_img_index] && (att_images[ds_img_index]->slot.id == att_desc->image.id));
        SOKOL_ASSERT(_sg_is_valid_rendertarget_depth_format(att_images[ds_img_index]->cmn.pixel_format));
        pass->mtl.ds_att.image = att_images[ds_img_index];
    }
    return SG_RESOURCESTATE_VALID;
}

_SOKOL_PRIVATE void _sg_mtl_discard_pass(_sg_pass_t* pass) {
    SOKOL_ASSERT(pass);
    _SOKOL_UNUSED(pass);
}

_SOKOL_PRIVATE _sg_image_t* _sg_mtl_pass_color_image(const _sg_pass_t* pass, int index) {
    SOKOL_ASSERT(pass && (index >= 0) && (index < SG_MAX_COLOR_ATTACHMENTS));
    /* NOTE: may return null */
    return pass->mtl.color_atts[index].image;
}

_SOKOL_PRIVATE _sg_image_t* _sg_mtl_pass_ds_image(const _sg_pass_t* pass) {
    /* NOTE: may return null */
    SOKOL_ASSERT(pass);
    return pass->mtl.ds_att.image;
}

_SOKOL_PRIVATE void _sg_mtl_begin_pass(_sg_pass_t* pass, const sg_pass_action* action, int w, int h) {
    SOKOL_ASSERT(action);
    SOKOL_ASSERT(!_sg.mtl.in_pass);
    SOKOL_ASSERT(_sg.mtl.cmd_queue);
    SOKOL_ASSERT(nil == _sg.mtl.cmd_encoder);
    SOKOL_ASSERT(_sg.mtl.renderpass_descriptor_cb || _sg.mtl.renderpass_descriptor_userdata_cb);
    _sg.mtl.in_pass = true;
    _sg.mtl.cur_width = w;
    _sg.mtl.cur_height = h;
    _sg_mtl_clear_state_cache();

    /*
        if this is the first pass in the frame, create command buffers

        NOTE: we're creating two command buffers here, one with unretained references
        for storing the regular commands, and one with retained references for
        storing the presentDrawable call (this needs to hold on the drawable until
        presentation has happened - and the easiest way to do this is to let the
        command buffer manage the lifetime of the drawable).

        Also see: https://github.com/floooh/sokol/issues/762
    */
    if (nil == _sg.mtl.cmd_buffer) {
        SOKOL_ASSERT(nil == _sg.mtl.present_cmd_buffer);
        /* block until the oldest frame in flight has finished */
        dispatch_semaphore_wait(_sg.mtl.sem, DISPATCH_TIME_FOREVER);
        _sg.mtl.cmd_buffer = [_sg.mtl.cmd_queue commandBufferWithUnretainedReferences];
        _sg.mtl.present_cmd_buffer =  [_sg.mtl.cmd_queue commandBuffer];
        [_sg.mtl.cmd_buffer enqueue];
        [_sg.mtl.present_cmd_buffer enqueue];
        [_sg.mtl.present_cmd_buffer addCompletedHandler:^(id<MTLCommandBuffer> cmd_buf) {
            // NOTE: this code is called on a different thread!
            _SOKOL_UNUSED(cmd_buf);
            dispatch_semaphore_signal(_sg.mtl.sem);
        }];
    }

    /* if this is first pass in frame, get uniform buffer base pointer */
    if (0 == _sg.mtl.cur_ub_base_ptr) {
        _sg.mtl.cur_ub_base_ptr = (uint8_t*)[_sg.mtl.uniform_buffers[_sg.mtl.cur_frame_rotate_index] contents];
    }

    /* initialize a render pass descriptor */
    MTLRenderPassDescriptor* pass_desc = nil;
    if (pass) {
        /* offscreen render pass */
        pass_desc = [MTLRenderPassDescriptor renderPassDescriptor];
    }
    else {
        /* default render pass, call user-provided callback to provide render pass descriptor */
        if (_sg.mtl.renderpass_descriptor_cb) {
            pass_desc = (__bridge MTLRenderPassDescriptor*) _sg.mtl.renderpass_descriptor_cb();
        }
        else {
            pass_desc = (__bridge MTLRenderPassDescriptor*) _sg.mtl.renderpass_descriptor_userdata_cb(_sg.mtl.user_data);
        }

    }
    if (pass_desc) {
        _sg.mtl.pass_valid = true;
    }
    else {
        /* default pass descriptor will not be valid if window is minimized,
           don't do any rendering in this case */
        _sg.mtl.pass_valid = false;
        return;
    }
    if (pass) {
        /* setup pass descriptor for offscreen rendering */
        SOKOL_ASSERT(pass->slot.state == SG_RESOURCESTATE_VALID);
        for (NSUInteger i = 0; i < (NSUInteger)pass->cmn.num_color_atts; i++) {
            const _sg_pass_attachment_t* cmn_att = &pass->cmn.color_atts[i];
            const _sg_mtl_attachment_t* mtl_att = &pass->mtl.color_atts[i];
            const _sg_image_t* att_img = mtl_att->image;
            SOKOL_ASSERT(att_img->slot.state == SG_RESOURCESTATE_VALID);
            SOKOL_ASSERT(att_img->slot.id == cmn_att->image_id.id);
            const bool is_msaa = (att_img->cmn.sample_count > 1);
            pass_desc.colorAttachments[i].loadAction = _sg_mtl_load_action(action->colors[i].action);
            pass_desc.colorAttachments[i].storeAction = is_msaa ? MTLStoreActionMultisampleResolve : MTLStoreActionStore;
            sg_color c = action->colors[i].value;
            pass_desc.colorAttachments[i].clearColor = MTLClearColorMake(c.r, c.g, c.b, c.a);
            if (is_msaa) {
                SOKOL_ASSERT(att_img->mtl.msaa_tex != _SG_MTL_INVALID_SLOT_INDEX);
                SOKOL_ASSERT(att_img->mtl.tex[mtl_att->image->cmn.active_slot] != _SG_MTL_INVALID_SLOT_INDEX);
                pass_desc.colorAttachments[i].texture = _sg_mtl_id(att_img->mtl.msaa_tex);
                pass_desc.colorAttachments[i].resolveTexture = _sg_mtl_id(att_img->mtl.tex[att_img->cmn.active_slot]);
                pass_desc.colorAttachments[i].resolveLevel = (NSUInteger)cmn_att->mip_level;
                switch (att_img->cmn.type) {
                    case SG_IMAGETYPE_CUBE:
                    case SG_IMAGETYPE_ARRAY:
                        pass_desc.colorAttachments[i].resolveSlice = (NSUInteger)cmn_att->slice;
                        break;
                    case SG_IMAGETYPE_3D:
                        pass_desc.colorAttachments[i].resolveDepthPlane = (NSUInteger)cmn_att->slice;
                        break;
                    default: break;
                }
            }
            else {
                SOKOL_ASSERT(att_img->mtl.tex[att_img->cmn.active_slot] != _SG_MTL_INVALID_SLOT_INDEX);
                pass_desc.colorAttachments[i].texture = _sg_mtl_id(att_img->mtl.tex[att_img->cmn.active_slot]);
                pass_desc.colorAttachments[i].level = (NSUInteger)cmn_att->mip_level;
                switch (att_img->cmn.type) {
                    case SG_IMAGETYPE_CUBE:
                    case SG_IMAGETYPE_ARRAY:
                        pass_desc.colorAttachments[i].slice = (NSUInteger)cmn_att->slice;
                        break;
                    case SG_IMAGETYPE_3D:
                        pass_desc.colorAttachments[i].depthPlane = (NSUInteger)cmn_att->slice;
                        break;
                    default: break;
                }
            }
        }
        const _sg_image_t* ds_att_img = pass->mtl.ds_att.image;
        if (0 != ds_att_img) {
            SOKOL_ASSERT(ds_att_img->slot.state == SG_RESOURCESTATE_VALID);
            SOKOL_ASSERT(ds_att_img->slot.id == pass->cmn.ds_att.image_id.id);
            SOKOL_ASSERT(ds_att_img->mtl.depth_tex != _SG_MTL_INVALID_SLOT_INDEX);
            pass_desc.depthAttachment.texture = _sg_mtl_id(ds_att_img->mtl.depth_tex);
            pass_desc.depthAttachment.loadAction = _sg_mtl_load_action(action->depth.action);
            pass_desc.depthAttachment.clearDepth = action->depth.value;
            if (_sg_is_depth_stencil_format(ds_att_img->cmn.pixel_format)) {
                pass_desc.stencilAttachment.texture = _sg_mtl_id(ds_att_img->mtl.depth_tex);
                pass_desc.stencilAttachment.loadAction = _sg_mtl_load_action(action->stencil.action);
                pass_desc.stencilAttachment.clearStencil = action->stencil.value;
            }
        }
    }
    else {
        /* setup pass descriptor for default rendering */
        pass_desc.colorAttachments[0].loadAction = _sg_mtl_load_action(action->colors[0].action);
        sg_color c = action->colors[0].value;
        pass_desc.colorAttachments[0].clearColor = MTLClearColorMake(c.r, c.g, c.b, c.a);
        pass_desc.depthAttachment.loadAction = _sg_mtl_load_action(action->depth.action);
        pass_desc.depthAttachment.clearDepth = action->depth.value;
        pass_desc.stencilAttachment.loadAction = _sg_mtl_load_action(action->stencil.action);
        pass_desc.stencilAttachment.clearStencil = action->stencil.value;
    }

    /* create a render command encoder, this might return nil if window is minimized */
    _sg.mtl.cmd_encoder = [_sg.mtl.cmd_buffer renderCommandEncoderWithDescriptor:pass_desc];
    if (nil == _sg.mtl.cmd_encoder) {
        _sg.mtl.pass_valid = false;
        return;
    }

    /* bind the global uniform buffer, this only happens once per pass */
    _sg_mtl_bind_uniform_buffers();
}

_SOKOL_PRIVATE void _sg_mtl_end_pass(void) {
    SOKOL_ASSERT(_sg.mtl.in_pass);
    _sg.mtl.in_pass = false;
    _sg.mtl.pass_valid = false;
    if (nil != _sg.mtl.cmd_encoder) {
        [_sg.mtl.cmd_encoder endEncoding];
        /* NOTE: MTLRenderCommandEncoder is autoreleased */
        _sg.mtl.cmd_encoder = nil;
    }
}

_SOKOL_PRIVATE void _sg_mtl_commit(void) {
    SOKOL_ASSERT(!_sg.mtl.in_pass);
    SOKOL_ASSERT(!_sg.mtl.pass_valid);
    SOKOL_ASSERT(_sg.mtl.drawable_cb || _sg.mtl.drawable_userdata_cb);
    SOKOL_ASSERT(nil == _sg.mtl.cmd_encoder);
    SOKOL_ASSERT(nil != _sg.mtl.cmd_buffer);
    SOKOL_ASSERT(nil != _sg.mtl.present_cmd_buffer);

    /* present, commit and signal semaphore when done */
    id<MTLDrawable> cur_drawable = nil;
    if (_sg.mtl.drawable_cb) {
        cur_drawable = (__bridge id<MTLDrawable>) _sg.mtl.drawable_cb();
    }
    else {
        cur_drawable = (__bridge id<MTLDrawable>) _sg.mtl.drawable_userdata_cb(_sg.mtl.user_data);
    }
    if (nil != cur_drawable) {
        [_sg.mtl.present_cmd_buffer presentDrawable:cur_drawable];
    }
    [_sg.mtl.cmd_buffer commit];
    [_sg.mtl.present_cmd_buffer commit];

    /* garbage-collect resources pending for release */
    _sg_mtl_garbage_collect(_sg.mtl.frame_index);

    /* rotate uniform buffer slot */
    if (++_sg.mtl.cur_frame_rotate_index >= SG_NUM_INFLIGHT_FRAMES) {
        _sg.mtl.cur_frame_rotate_index = 0;
    }
    _sg.mtl.frame_index++;
    _sg.mtl.cur_ub_offset = 0;
    _sg.mtl.cur_ub_base_ptr = 0;
    /* NOTE: MTLCommandBuffer is autoreleased */
    _sg.mtl.cmd_buffer = nil;
    _sg.mtl.present_cmd_buffer = nil;
}

_SOKOL_PRIVATE void _sg_mtl_apply_viewport(int x, int y, int w, int h, bool origin_top_left) {
    SOKOL_ASSERT(_sg.mtl.in_pass);
    if (!_sg.mtl.pass_valid) {
        return;
    }
    SOKOL_ASSERT(nil != _sg.mtl.cmd_encoder);
    MTLViewport vp;
    vp.originX = (double) x;
    vp.originY = (double) (origin_top_left ? y : (_sg.mtl.cur_height - (y + h)));
    vp.width   = (double) w;
    vp.height  = (double) h;
    vp.znear   = 0.0;
    vp.zfar    = 1.0;
    [_sg.mtl.cmd_encoder setViewport:vp];
}

_SOKOL_PRIVATE void _sg_mtl_apply_scissor_rect(int x, int y, int w, int h, bool origin_top_left) {
    SOKOL_ASSERT(_sg.mtl.in_pass);
    if (!_sg.mtl.pass_valid) {
        return;
    }
    SOKOL_ASSERT(nil != _sg.mtl.cmd_encoder);
    /* clip against framebuffer rect */
    x = _sg_min(_sg_max(0, x), _sg.mtl.cur_width-1);
    y = _sg_min(_sg_max(0, y), _sg.mtl.cur_height-1);
    if ((x + w) > _sg.mtl.cur_width) {
        w = _sg.mtl.cur_width - x;
    }
    if ((y + h) > _sg.mtl.cur_height) {
        h = _sg.mtl.cur_height - y;
    }
    w = _sg_max(w, 1);
    h = _sg_max(h, 1);

    MTLScissorRect r;
    r.x = (NSUInteger)x;
    r.y = (NSUInteger) (origin_top_left ? y : (_sg.mtl.cur_height - (y + h)));
    r.width = (NSUInteger)w;
    r.height = (NSUInteger)h;
    [_sg.mtl.cmd_encoder setScissorRect:r];
}

_SOKOL_PRIVATE void _sg_mtl_apply_pipeline(_sg_pipeline_t* pip) {
    SOKOL_ASSERT(pip);
    SOKOL_ASSERT(pip->shader && (pip->cmn.shader_id.id == pip->shader->slot.id));
    SOKOL_ASSERT(_sg.mtl.in_pass);
    if (!_sg.mtl.pass_valid) {
        return;
    }
    SOKOL_ASSERT(nil != _sg.mtl.cmd_encoder);

    if ((_sg.mtl.state_cache.cur_pipeline != pip) || (_sg.mtl.state_cache.cur_pipeline_id.id != pip->slot.id)) {
        _sg.mtl.state_cache.cur_pipeline = pip;
        _sg.mtl.state_cache.cur_pipeline_id.id = pip->slot.id;
        sg_color c = pip->cmn.blend_color;
        [_sg.mtl.cmd_encoder setBlendColorRed:c.r green:c.g blue:c.b alpha:c.a];
        [_sg.mtl.cmd_encoder setCullMode:pip->mtl.cull_mode];
        [_sg.mtl.cmd_encoder setFrontFacingWinding:pip->mtl.winding];
        [_sg.mtl.cmd_encoder setStencilReferenceValue:pip->mtl.stencil_ref];
        [_sg.mtl.cmd_encoder setDepthBias:pip->cmn.depth_bias slopeScale:pip->cmn.depth_bias_slope_scale clamp:pip->cmn.depth_bias_clamp];
        SOKOL_ASSERT(pip->mtl.rps != _SG_MTL_INVALID_SLOT_INDEX);
        [_sg.mtl.cmd_encoder setRenderPipelineState:_sg_mtl_id(pip->mtl.rps)];
        SOKOL_ASSERT(pip->mtl.dss != _SG_MTL_INVALID_SLOT_INDEX);
        [_sg.mtl.cmd_encoder setDepthStencilState:_sg_mtl_id(pip->mtl.dss)];
    }
}

_SOKOL_PRIVATE void _sg_mtl_apply_bindings(
    _sg_pipeline_t* pip,
    _sg_buffer_t** vbs, const int* vb_offsets, int num_vbs,
    _sg_buffer_t* ib, int ib_offset,
    _sg_image_t** vs_imgs, int num_vs_imgs,
    _sg_image_t** fs_imgs, int num_fs_imgs)
{
    _SOKOL_UNUSED(pip);
    SOKOL_ASSERT(_sg.mtl.in_pass);
    if (!_sg.mtl.pass_valid) {
        return;
    }
    SOKOL_ASSERT(nil != _sg.mtl.cmd_encoder);

    /* store index buffer binding, this will be needed later in sg_draw() */
    _sg.mtl.state_cache.cur_indexbuffer = ib;
    _sg.mtl.state_cache.cur_indexbuffer_offset = ib_offset;
    if (ib) {
        SOKOL_ASSERT(pip->cmn.index_type != SG_INDEXTYPE_NONE);
        _sg.mtl.state_cache.cur_indexbuffer_id.id = ib->slot.id;
    }
    else {
        SOKOL_ASSERT(pip->cmn.index_type == SG_INDEXTYPE_NONE);
        _sg.mtl.state_cache.cur_indexbuffer_id.id = SG_INVALID_ID;
    }

    /* apply vertex buffers */
    NSUInteger slot;
    for (slot = 0; slot < (NSUInteger)num_vbs; slot++) {
        const _sg_buffer_t* vb = vbs[slot];
        if ((_sg.mtl.state_cache.cur_vertexbuffers[slot] != vb) ||
            (_sg.mtl.state_cache.cur_vertexbuffer_offsets[slot] != vb_offsets[slot]) ||
            (_sg.mtl.state_cache.cur_vertexbuffer_ids[slot].id != vb->slot.id))
        {
            _sg.mtl.state_cache.cur_vertexbuffers[slot] = vb;
            _sg.mtl.state_cache.cur_vertexbuffer_offsets[slot] = vb_offsets[slot];
            _sg.mtl.state_cache.cur_vertexbuffer_ids[slot].id = vb->slot.id;
            const NSUInteger mtl_slot = SG_MAX_SHADERSTAGE_UBS + slot;
            SOKOL_ASSERT(vb->mtl.buf[vb->cmn.active_slot] != _SG_MTL_INVALID_SLOT_INDEX);
            [_sg.mtl.cmd_encoder setVertexBuffer:_sg_mtl_id(vb->mtl.buf[vb->cmn.active_slot])
                offset:(NSUInteger)vb_offsets[slot]
                atIndex:mtl_slot];
        }
    }

    /* apply vertex shader images */
    for (slot = 0; slot < (NSUInteger)num_vs_imgs; slot++) {
        const _sg_image_t* img = vs_imgs[slot];
        if ((_sg.mtl.state_cache.cur_vs_images[slot] != img) || (_sg.mtl.state_cache.cur_vs_image_ids[slot].id != img->slot.id)) {
            _sg.mtl.state_cache.cur_vs_images[slot] = img;
            _sg.mtl.state_cache.cur_vs_image_ids[slot].id = img->slot.id;
            SOKOL_ASSERT(img->mtl.tex[img->cmn.active_slot] != _SG_MTL_INVALID_SLOT_INDEX);
            [_sg.mtl.cmd_encoder setVertexTexture:_sg_mtl_id(img->mtl.tex[img->cmn.active_slot]) atIndex:slot];
            SOKOL_ASSERT(img->mtl.sampler_state != _SG_MTL_INVALID_SLOT_INDEX);
            [_sg.mtl.cmd_encoder setVertexSamplerState:_sg_mtl_id(img->mtl.sampler_state) atIndex:slot];
        }
    }

    /* apply fragment shader images */
    for (slot = 0; slot < (NSUInteger)num_fs_imgs; slot++) {
        const _sg_image_t* img = fs_imgs[slot];
        if ((_sg.mtl.state_cache.cur_fs_images[slot] != img) || (_sg.mtl.state_cache.cur_fs_image_ids[slot].id != img->slot.id)) {
            _sg.mtl.state_cache.cur_fs_images[slot] = img;
            _sg.mtl.state_cache.cur_fs_image_ids[slot].id = img->slot.id;
            SOKOL_ASSERT(img->mtl.tex[img->cmn.active_slot] != _SG_MTL_INVALID_SLOT_INDEX);
            [_sg.mtl.cmd_encoder setFragmentTexture:_sg_mtl_id(img->mtl.tex[img->cmn.active_slot]) atIndex:slot];
            SOKOL_ASSERT(img->mtl.sampler_state != _SG_MTL_INVALID_SLOT_INDEX);
            [_sg.mtl.cmd_encoder setFragmentSamplerState:_sg_mtl_id(img->mtl.sampler_state) atIndex:slot];
        }
    }
}

_SOKOL_PRIVATE void _sg_mtl_apply_uniforms(sg_shader_stage stage_index, int ub_index, const sg_range* data) {
    SOKOL_ASSERT(_sg.mtl.in_pass);
    if (!_sg.mtl.pass_valid) {
        return;
    }
    SOKOL_ASSERT(nil != _sg.mtl.cmd_encoder);
    SOKOL_ASSERT(((size_t)_sg.mtl.cur_ub_offset + data->size) <= (size_t)_sg.mtl.ub_size);
    SOKOL_ASSERT((_sg.mtl.cur_ub_offset & (_SG_MTL_UB_ALIGN-1)) == 0);
    SOKOL_ASSERT(_sg.mtl.state_cache.cur_pipeline && _sg.mtl.state_cache.cur_pipeline->shader);
    SOKOL_ASSERT(_sg.mtl.state_cache.cur_pipeline->slot.id == _sg.mtl.state_cache.cur_pipeline_id.id);
    SOKOL_ASSERT(_sg.mtl.state_cache.cur_pipeline->shader->slot.id == _sg.mtl.state_cache.cur_pipeline->cmn.shader_id.id);
    SOKOL_ASSERT(ub_index < _sg.mtl.state_cache.cur_pipeline->shader->cmn.stage[stage_index].num_uniform_blocks);
    SOKOL_ASSERT(data->size <= _sg.mtl.state_cache.cur_pipeline->shader->cmn.stage[stage_index].uniform_blocks[ub_index].size);

    /* copy to global uniform buffer, record offset into cmd encoder, and advance offset */
    uint8_t* dst = &_sg.mtl.cur_ub_base_ptr[_sg.mtl.cur_ub_offset];
    memcpy(dst, data->ptr, data->size);
    if (stage_index == SG_SHADERSTAGE_VS) {
        [_sg.mtl.cmd_encoder setVertexBufferOffset:(NSUInteger)_sg.mtl.cur_ub_offset atIndex:(NSUInteger)ub_index];
    }
    else {
        [_sg.mtl.cmd_encoder setFragmentBufferOffset:(NSUInteger)_sg.mtl.cur_ub_offset atIndex:(NSUInteger)ub_index];
    }
    _sg.mtl.cur_ub_offset = _sg_roundup(_sg.mtl.cur_ub_offset + (int)data->size, _SG_MTL_UB_ALIGN);
}

_SOKOL_PRIVATE void _sg_mtl_draw(int base_element, int num_elements, int num_instances) {
    SOKOL_ASSERT(_sg.mtl.in_pass);
    if (!_sg.mtl.pass_valid) {
        return;
    }
    SOKOL_ASSERT(nil != _sg.mtl.cmd_encoder);
    SOKOL_ASSERT(_sg.mtl.state_cache.cur_pipeline && (_sg.mtl.state_cache.cur_pipeline->slot.id == _sg.mtl.state_cache.cur_pipeline_id.id));
    if (SG_INDEXTYPE_NONE != _sg.mtl.state_cache.cur_pipeline->cmn.index_type) {
        /* indexed rendering */
        SOKOL_ASSERT(_sg.mtl.state_cache.cur_indexbuffer && (_sg.mtl.state_cache.cur_indexbuffer->slot.id == _sg.mtl.state_cache.cur_indexbuffer_id.id));
        const _sg_buffer_t* ib = _sg.mtl.state_cache.cur_indexbuffer;
        SOKOL_ASSERT(ib->mtl.buf[ib->cmn.active_slot] != _SG_MTL_INVALID_SLOT_INDEX);
        const NSUInteger index_buffer_offset = (NSUInteger) (_sg.mtl.state_cache.cur_indexbuffer_offset + base_element * _sg.mtl.state_cache.cur_pipeline->mtl.index_size);
        [_sg.mtl.cmd_encoder drawIndexedPrimitives:_sg.mtl.state_cache.cur_pipeline->mtl.prim_type
            indexCount:(NSUInteger)num_elements
            indexType:_sg.mtl.state_cache.cur_pipeline->mtl.index_type
            indexBuffer:_sg_mtl_id(ib->mtl.buf[ib->cmn.active_slot])
            indexBufferOffset:index_buffer_offset
            instanceCount:(NSUInteger)num_instances];
    }
    else {
        /* non-indexed rendering */
        [_sg.mtl.cmd_encoder drawPrimitives:_sg.mtl.state_cache.cur_pipeline->mtl.prim_type
            vertexStart:(NSUInteger)base_element
            vertexCount:(NSUInteger)num_elements
            instanceCount:(NSUInteger)num_instances];
    }
}

_SOKOL_PRIVATE void _sg_mtl_update_buffer(_sg_buffer_t* buf, const sg_range* data) {
    SOKOL_ASSERT(buf && data && data->ptr && (data->size > 0));
    if (++buf->cmn.active_slot >= buf->cmn.num_slots) {
        buf->cmn.active_slot = 0;
    }
    __unsafe_unretained id<MTLBuffer> mtl_buf = _sg_mtl_id(buf->mtl.buf[buf->cmn.active_slot]);
    void* dst_ptr = [mtl_buf contents];
    memcpy(dst_ptr, data->ptr, data->size);
    #if defined(_SG_TARGET_MACOS)
    [mtl_buf didModifyRange:NSMakeRange(0, data->size)];
    #endif
}

_SOKOL_PRIVATE int _sg_mtl_append_buffer(_sg_buffer_t* buf, const sg_range* data, bool new_frame) {
    SOKOL_ASSERT(buf && data && data->ptr && (data->size > 0));
    if (new_frame) {
        if (++buf->cmn.active_slot >= buf->cmn.num_slots) {
            buf->cmn.active_slot = 0;
        }
    }
    __unsafe_unretained id<MTLBuffer> mtl_buf = _sg_mtl_id(buf->mtl.buf[buf->cmn.active_slot]);
    uint8_t* dst_ptr = (uint8_t*) [mtl_buf contents];
    dst_ptr += buf->cmn.append_pos;
    memcpy(dst_ptr, data->ptr, data->size);
    #if defined(_SG_TARGET_MACOS)
    [mtl_buf didModifyRange:NSMakeRange((NSUInteger)buf->cmn.append_pos, (NSUInteger)data->size)];
    #endif
    /* NOTE: this is a requirement from WebGPU, but we want identical behaviour across all backends */
    return _sg_roundup((int)data->size, 4);
}

_SOKOL_PRIVATE void _sg_mtl_update_image(_sg_image_t* img, const sg_image_data* data) {
    SOKOL_ASSERT(img && data);
    if (++img->cmn.active_slot >= img->cmn.num_slots) {
        img->cmn.active_slot = 0;
    }
    __unsafe_unretained id<MTLTexture> mtl_tex = _sg_mtl_id(img->mtl.tex[img->cmn.active_slot]);
    _sg_mtl_copy_image_data(img, mtl_tex, data);
}

/*== WEBGPU BACKEND IMPLEMENTATION ===========================================*/
#elif defined(SOKOL_WGPU)

_SOKOL_PRIVATE WGPUBufferUsageFlags _sg_wgpu_buffer_usage(sg_buffer_type t, sg_usage u) {
    WGPUBufferUsageFlags res = 0;
    if (SG_BUFFERTYPE_VERTEXBUFFER == t) {
        res |= WGPUBufferUsage_Vertex;
    }
    else {
        res |= WGPUBufferUsage_Index;
    }
    if (SG_USAGE_IMMUTABLE != u) {
        res |= WGPUBufferUsage_CopyDst;
    }
    return res;
}

_SOKOL_PRIVATE WGPULoadOp _sg_wgpu_load_op(sg_action a) {
    switch (a) {
        case SG_ACTION_CLEAR:
        case SG_ACTION_DONTCARE:
            return WGPULoadOp_Clear;
        case SG_ACTION_LOAD:
            return WGPULoadOp_Load;
        default:
            SOKOL_UNREACHABLE;
            return (WGPULoadOp)0;
    }
}

_SOKOL_PRIVATE WGPUTextureViewDimension _sg_wgpu_tex_viewdim(sg_image_type t) {
    switch (t) {
        case SG_IMAGETYPE_2D:       return WGPUTextureViewDimension_2D;
        case SG_IMAGETYPE_CUBE:     return WGPUTextureViewDimension_Cube;
        case SG_IMAGETYPE_3D:       return WGPUTextureViewDimension_3D;
        case SG_IMAGETYPE_ARRAY:    return WGPUTextureViewDimension_2DArray;
        default: SOKOL_UNREACHABLE; return WGPUTextureViewDimension_Force32;
    }
}

_SOKOL_PRIVATE WGPUTextureComponentType _sg_wgpu_tex_comptype(sg_sampler_type t) {
    switch (t) {
        case SG_SAMPLERTYPE_FLOAT:  return WGPUTextureComponentType_Float;
        case SG_SAMPLERTYPE_SINT:   return WGPUTextureComponentType_Sint;
        case SG_SAMPLERTYPE_UINT:   return WGPUTextureComponentType_Uint;
        default: SOKOL_UNREACHABLE; return WGPUTextureComponentType_Force32;
    }
}

_SOKOL_PRIVATE WGPUTextureDimension _sg_wgpu_tex_dim(sg_image_type t) {
    if (SG_IMAGETYPE_3D == t) {
        return WGPUTextureDimension_3D;
    }
    else {
        return WGPUTextureDimension_2D;
    }
}

_SOKOL_PRIVATE WGPUAddressMode _sg_wgpu_sampler_addrmode(sg_wrap m) {
    switch (m) {
        case SG_WRAP_REPEAT:
            return WGPUAddressMode_Repeat;
        case SG_WRAP_CLAMP_TO_EDGE:
        case SG_WRAP_CLAMP_TO_BORDER:
            return WGPUAddressMode_ClampToEdge;
        case SG_WRAP_MIRRORED_REPEAT:
            return WGPUAddressMode_MirrorRepeat;
        default:
            SOKOL_UNREACHABLE;
            return WGPUAddressMode_Force32;
    }
}

_SOKOL_PRIVATE WGPUFilterMode _sg_wgpu_sampler_minmagfilter(sg_filter f) {
    switch (f) {
        case SG_FILTER_NEAREST:
        case SG_FILTER_NEAREST_MIPMAP_NEAREST:
        case SG_FILTER_NEAREST_MIPMAP_LINEAR:
            return WGPUFilterMode_Nearest;
        case SG_FILTER_LINEAR:
        case SG_FILTER_LINEAR_MIPMAP_NEAREST:
        case SG_FILTER_LINEAR_MIPMAP_LINEAR:
            return WGPUFilterMode_Linear;
        default:
            SOKOL_UNREACHABLE;
            return WGPUFilterMode_Force32;
    }
}

_SOKOL_PRIVATE WGPUFilterMode _sg_wgpu_sampler_mipfilter(sg_filter f) {
    switch (f) {
        case SG_FILTER_NEAREST:
        case SG_FILTER_LINEAR:
        case SG_FILTER_NEAREST_MIPMAP_NEAREST:
        case SG_FILTER_LINEAR_MIPMAP_NEAREST:
            return WGPUFilterMode_Nearest;
        case SG_FILTER_NEAREST_MIPMAP_LINEAR:
        case SG_FILTER_LINEAR_MIPMAP_LINEAR:
            return WGPUFilterMode_Linear;
        default:
            SOKOL_UNREACHABLE;
            return WGPUFilterMode_Force32;
    }
}

_SOKOL_PRIVATE WGPUIndexFormat _sg_wgpu_indexformat(sg_index_type t) {
    /* NOTE: there's no WGPUIndexFormat_None */
    return (t == SG_INDEXTYPE_UINT16) ? WGPUIndexFormat_Uint16 : WGPUIndexFormat_Uint32;
}

_SOKOL_PRIVATE WGPUInputStepMode _sg_wgpu_stepmode(sg_vertex_step s) {
    return (s == SG_VERTEXSTEP_PER_VERTEX) ? WGPUInputStepMode_Vertex : WGPUInputStepMode_Instance;
}

_SOKOL_PRIVATE WGPUVertexFormat _sg_wgpu_vertexformat(sg_vertex_format f) {
    switch (f) {
        case SG_VERTEXFORMAT_FLOAT:         return WGPUVertexFormat_Float;
        case SG_VERTEXFORMAT_FLOAT2:        return WGPUVertexFormat_Float2;
        case SG_VERTEXFORMAT_FLOAT3:        return WGPUVertexFormat_Float3;
        case SG_VERTEXFORMAT_FLOAT4:        return WGPUVertexFormat_Float4;
        case SG_VERTEXFORMAT_BYTE4:         return WGPUVertexFormat_Char4;
        case SG_VERTEXFORMAT_BYTE4N:        return WGPUVertexFormat_Char4Norm;
        case SG_VERTEXFORMAT_UBYTE4:        return WGPUVertexFormat_UChar4;
        case SG_VERTEXFORMAT_UBYTE4N:       return WGPUVertexFormat_UChar4Norm;
        case SG_VERTEXFORMAT_SHORT2:        return WGPUVertexFormat_Short2;
        case SG_VERTEXFORMAT_SHORT2N:       return WGPUVertexFormat_Short2Norm;
        case SG_VERTEXFORMAT_USHORT2N:      return WGPUVertexFormat_UShort2Norm;
        case SG_VERTEXFORMAT_SHORT4:        return WGPUVertexFormat_Short4;
        case SG_VERTEXFORMAT_SHORT4N:       return WGPUVertexFormat_Short4Norm;
        case SG_VERTEXFORMAT_USHORT4N:      return WGPUVertexFormat_UShort4Norm;
        /* FIXME! UINT10_N2 */
        case SG_VERTEXFORMAT_UINT10_N2:
        default:
            SOKOL_UNREACHABLE;
            return WGPUVertexFormat_Force32;
    }
}

_SOKOL_PRIVATE WGPUPrimitiveTopology _sg_wgpu_topology(sg_primitive_type t) {
    switch (t) {
        case SG_PRIMITIVETYPE_POINTS:           return WGPUPrimitiveTopology_PointList;
        case SG_PRIMITIVETYPE_LINES:            return WGPUPrimitiveTopology_LineList;
        case SG_PRIMITIVETYPE_LINE_STRIP:       return WGPUPrimitiveTopology_LineStrip;
        case SG_PRIMITIVETYPE_TRIANGLES:        return WGPUPrimitiveTopology_TriangleList;
        case SG_PRIMITIVETYPE_TRIANGLE_STRIP:   return WGPUPrimitiveTopology_TriangleStrip;
        default: SOKOL_UNREACHABLE; return WGPUPrimitiveTopology_Force32;
    }
}

_SOKOL_PRIVATE WGPUFrontFace _sg_wgpu_frontface(sg_face_winding fw) {
    return (fw == SG_FACEWINDING_CCW) ? WGPUFrontFace_CCW : WGPUFrontFace_CW;
}

_SOKOL_PRIVATE WGPUCullMode _sg_wgpu_cullmode(sg_cull_mode cm) {
    switch (cm) {
        case SG_CULLMODE_NONE:      return WGPUCullMode_None;
        case SG_CULLMODE_FRONT:     return WGPUCullMode_Front;
        case SG_CULLMODE_BACK:      return WGPUCullMode_Back;
        default: SOKOL_UNREACHABLE; return WGPUCullMode_Force32;
    }
}

_SOKOL_PRIVATE WGPUTextureFormat _sg_wgpu_textureformat(sg_pixel_format p) {
    switch (p) {
        case SG_PIXELFORMAT_NONE:           return WGPUTextureFormat_Undefined;
        case SG_PIXELFORMAT_R8:             return WGPUTextureFormat_R8Unorm;
        case SG_PIXELFORMAT_R8SN:           return WGPUTextureFormat_R8Snorm;
        case SG_PIXELFORMAT_R8UI:           return WGPUTextureFormat_R8Uint;
        case SG_PIXELFORMAT_R8SI:           return WGPUTextureFormat_R8Sint;
        case SG_PIXELFORMAT_R16UI:          return WGPUTextureFormat_R16Uint;
        case SG_PIXELFORMAT_R16SI:          return WGPUTextureFormat_R16Sint;
        case SG_PIXELFORMAT_R16F:           return WGPUTextureFormat_R16Float;
        case SG_PIXELFORMAT_RG8:            return WGPUTextureFormat_RG8Unorm;
        case SG_PIXELFORMAT_RG8SN:          return WGPUTextureFormat_RG8Snorm;
        case SG_PIXELFORMAT_RG8UI:          return WGPUTextureFormat_RG8Uint;
        case SG_PIXELFORMAT_RG8SI:          return WGPUTextureFormat_RG8Sint;
        case SG_PIXELFORMAT_R32UI:          return WGPUTextureFormat_R32Uint;
        case SG_PIXELFORMAT_R32SI:          return WGPUTextureFormat_R32Sint;
        case SG_PIXELFORMAT_R32F:           return WGPUTextureFormat_R32Float;
        case SG_PIXELFORMAT_RG16UI:         return WGPUTextureFormat_RG16Uint;
        case SG_PIXELFORMAT_RG16SI:         return WGPUTextureFormat_RG16Sint;
        case SG_PIXELFORMAT_RG16F:          return WGPUTextureFormat_RG16Float;
        case SG_PIXELFORMAT_RGBA8:          return WGPUTextureFormat_RGBA8Unorm;
        case SG_PIXELFORMAT_RGBA8SN:        return WGPUTextureFormat_RGBA8Snorm;
        case SG_PIXELFORMAT_RGBA8UI:        return WGPUTextureFormat_RGBA8Uint;
        case SG_PIXELFORMAT_RGBA8SI:        return WGPUTextureFormat_RGBA8Sint;
        case SG_PIXELFORMAT_BGRA8:          return WGPUTextureFormat_BGRA8Unorm;
        case SG_PIXELFORMAT_RGB10A2:        return WGPUTextureFormat_RGB10A2Unorm;
        case SG_PIXELFORMAT_RG11B10F:       return WGPUTextureFormat_RG11B10Float;
        case SG_PIXELFORMAT_RG32UI:         return WGPUTextureFormat_RG32Uint;
        case SG_PIXELFORMAT_RG32SI:         return WGPUTextureFormat_RG32Sint;
        case SG_PIXELFORMAT_RG32F:          return WGPUTextureFormat_RG32Float;
        case SG_PIXELFORMAT_RGBA16UI:       return WGPUTextureFormat_RGBA16Uint;
        case SG_PIXELFORMAT_RGBA16SI:       return WGPUTextureFormat_RGBA16Sint;
        case SG_PIXELFORMAT_RGBA16F:        return WGPUTextureFormat_RGBA16Float;
        case SG_PIXELFORMAT_RGBA32UI:       return WGPUTextureFormat_RGBA32Uint;
        case SG_PIXELFORMAT_RGBA32SI:       return WGPUTextureFormat_RGBA32Sint;
        case SG_PIXELFORMAT_RGBA32F:        return WGPUTextureFormat_RGBA32Float;
        case SG_PIXELFORMAT_DEPTH:          return WGPUTextureFormat_Depth24Plus;
        case SG_PIXELFORMAT_DEPTH_STENCIL:  return WGPUTextureFormat_Depth24PlusStencil8;
        case SG_PIXELFORMAT_BC1_RGBA:       return WGPUTextureFormat_BC1RGBAUnorm;
        case SG_PIXELFORMAT_BC2_RGBA:       return WGPUTextureFormat_BC2RGBAUnorm;
        case SG_PIXELFORMAT_BC3_RGBA:       return WGPUTextureFormat_BC3RGBAUnorm;
        case SG_PIXELFORMAT_BC4_R:          return WGPUTextureFormat_BC4RUnorm;
        case SG_PIXELFORMAT_BC4_RSN:        return WGPUTextureFormat_BC4RSnorm;
        case SG_PIXELFORMAT_BC5_RG:         return WGPUTextureFormat_BC5RGUnorm;
        case SG_PIXELFORMAT_BC5_RGSN:       return WGPUTextureFormat_BC5RGSnorm;
        case SG_PIXELFORMAT_BC6H_RGBF:      return WGPUTextureFormat_BC6HRGBSfloat;
        case SG_PIXELFORMAT_BC6H_RGBUF:     return WGPUTextureFormat_BC6HRGBUfloat;
        case SG_PIXELFORMAT_BC7_RGBA:       return WGPUTextureFormat_BC7RGBAUnorm;

        /* NOT SUPPORTED */
        case SG_PIXELFORMAT_R16:
        case SG_PIXELFORMAT_R16SN:
        case SG_PIXELFORMAT_RG16:
        case SG_PIXELFORMAT_RG16SN:
        case SG_PIXELFORMAT_RGBA16:
        case SG_PIXELFORMAT_RGBA16SN:
        case SG_PIXELFORMAT_RGB9E5:
        case SG_PIXELFORMAT_PVRTC_RGB_2BPP:
        case SG_PIXELFORMAT_PVRTC_RGB_4BPP:
        case SG_PIXELFORMAT_PVRTC_RGBA_2BPP:
        case SG_PIXELFORMAT_PVRTC_RGBA_4BPP:
        case SG_PIXELFORMAT_ETC2_RGB8:
        case SG_PIXELFORMAT_ETC2_RGB8A1:
        case SG_PIXELFORMAT_ETC2_RGBA8:
        case SG_PIXELFORMAT_ETC2_RG11:
        case SG_PIXELFORMAT_ETC2_RG11SN:
        default:
            SOKOL_UNREACHABLE;
            return WGPUTextureFormat_Force32;
    }
}

/*
FIXME ??? this isn't needed anywhere?
_SOKOL_PRIVATE WGPUTextureAspect _sg_wgpu_texture_aspect(sg_pixel_format fmt) {
    if (_sg_is_valid_rendertarget_depth_format(fmt)) {
        if (!_sg_is_depth_stencil_format(fmt)) {
            return WGPUTextureAspect_DepthOnly;
        }
    }
    return WGPUTextureAspect_All;
}
*/

_SOKOL_PRIVATE WGPUCompareFunction _sg_wgpu_comparefunc(sg_compare_func f) {
    switch (f) {
        case SG_COMPAREFUNC_NEVER:          return WGPUCompareFunction_Never;
        case SG_COMPAREFUNC_LESS:           return WGPUCompareFunction_Less;
        case SG_COMPAREFUNC_EQUAL:          return WGPUCompareFunction_Equal;
        case SG_COMPAREFUNC_LESS_EQUAL:     return WGPUCompareFunction_LessEqual;
        case SG_COMPAREFUNC_GREATER:        return WGPUCompareFunction_Greater;
        case SG_COMPAREFUNC_NOT_EQUAL:      return WGPUCompareFunction_NotEqual;
        case SG_COMPAREFUNC_GREATER_EQUAL:  return WGPUCompareFunction_GreaterEqual;
        case SG_COMPAREFUNC_ALWAYS:         return WGPUCompareFunction_Always;
        default: SOKOL_UNREACHABLE; return WGPUCompareFunction_Force32;
    }
}

_SOKOL_PRIVATE WGPUStencilOperation _sg_wgpu_stencilop(sg_stencil_op op) {
    switch (op) {
        case SG_STENCILOP_KEEP:         return WGPUStencilOperation_Keep;
        case SG_STENCILOP_ZERO:         return WGPUStencilOperation_Zero;
        case SG_STENCILOP_REPLACE:      return WGPUStencilOperation_Replace;
        case SG_STENCILOP_INCR_CLAMP:   return WGPUStencilOperation_IncrementClamp;
        case SG_STENCILOP_DECR_CLAMP:   return WGPUStencilOperation_DecrementClamp;
        case SG_STENCILOP_INVERT:       return WGPUStencilOperation_Invert;
        case SG_STENCILOP_INCR_WRAP:    return WGPUStencilOperation_IncrementWrap;
        case SG_STENCILOP_DECR_WRAP:    return WGPUStencilOperation_DecrementWrap;
        default: SOKOL_UNREACHABLE; return WGPUStencilOperation_Force32;
    }
}

_SOKOL_PRIVATE WGPUBlendOperation _sg_wgpu_blendop(sg_blend_op op) {
    switch (op) {
        case SG_BLENDOP_ADD:                return WGPUBlendOperation_Add;
        case SG_BLENDOP_SUBTRACT:           return WGPUBlendOperation_Subtract;
        case SG_BLENDOP_REVERSE_SUBTRACT:   return WGPUBlendOperation_ReverseSubtract;
        default: SOKOL_UNREACHABLE; return WGPUBlendOperation_Force32;
    }
}

_SOKOL_PRIVATE WGPUBlendFactor _sg_wgpu_blendfactor(sg_blend_factor f) {
    switch (f) {
        case SG_BLENDFACTOR_ZERO:                   return WGPUBlendFactor_Zero;
        case SG_BLENDFACTOR_ONE:                    return WGPUBlendFactor_One;
        case SG_BLENDFACTOR_SRC_COLOR:              return WGPUBlendFactor_SrcColor;
        case SG_BLENDFACTOR_ONE_MINUS_SRC_COLOR:    return WGPUBlendFactor_OneMinusSrcColor;
        case SG_BLENDFACTOR_SRC_ALPHA:              return WGPUBlendFactor_SrcAlpha;
        case SG_BLENDFACTOR_ONE_MINUS_SRC_ALPHA:    return WGPUBlendFactor_OneMinusSrcAlpha;
        case SG_BLENDFACTOR_DST_COLOR:              return WGPUBlendFactor_DstColor;
        case SG_BLENDFACTOR_ONE_MINUS_DST_COLOR:    return WGPUBlendFactor_OneMinusDstColor;
        case SG_BLENDFACTOR_DST_ALPHA:              return WGPUBlendFactor_DstAlpha;
        case SG_BLENDFACTOR_ONE_MINUS_DST_ALPHA:    return WGPUBlendFactor_OneMinusDstAlpha;
        case SG_BLENDFACTOR_SRC_ALPHA_SATURATED:    return WGPUBlendFactor_SrcAlphaSaturated;
        case SG_BLENDFACTOR_BLEND_COLOR:            return WGPUBlendFactor_BlendColor;
        case SG_BLENDFACTOR_ONE_MINUS_BLEND_COLOR:  return WGPUBlendFactor_OneMinusBlendColor;
        /* FIXME: separate blend alpha value not supported? */
        case SG_BLENDFACTOR_BLEND_ALPHA:            return WGPUBlendFactor_BlendColor;
        case SG_BLENDFACTOR_ONE_MINUS_BLEND_ALPHA:  return WGPUBlendFactor_OneMinusBlendColor;
        default:
            SOKOL_UNREACHABLE; return WGPUBlendFactor_Force32;
    }
}

_SOKOL_PRIVATE WGPUColorWriteMaskFlags _sg_wgpu_colorwritemask(uint8_t m) {
    WGPUColorWriteMaskFlags res = 0;
    if (0 != (m & SG_COLORMASK_R)) {
        res |= WGPUColorWriteMask_Red;
    }
    if (0 != (m & SG_COLORMASK_G)) {
        res |= WGPUColorWriteMask_Green;
    }
    if (0 != (m & SG_COLORMASK_B)) {
        res |= WGPUColorWriteMask_Blue;
    }
    if (0 != (m & SG_COLORMASK_A)) {
        res |= WGPUColorWriteMask_Alpha;
    }
    return res;
}

_SOKOL_PRIVATE void _sg_wgpu_init_caps(void) {
    _sg.backend = SG_BACKEND_WGPU;
    _sg.features.instancing = true;
    _sg.features.origin_top_left = true;
    _sg.features.multiple_render_targets = true;
    _sg.features.msaa_render_targets = true;
    _sg.features.imagetype_3d = true;
    _sg.features.imagetype_array = true;
    _sg.features.image_clamp_to_border = false;
    _sg.features.mrt_independent_blend_state = true;
    _sg.features.mrt_independent_write_mask = true;

    /* FIXME: max images size??? */
    _sg.limits.max_image_size_2d = 8 * 1024;
    _sg.limits.max_image_size_cube = 8 * 1024;
    _sg.limits.max_image_size_3d = 2 * 1024;
    _sg.limits.max_image_size_array = 8 * 1024;
    _sg.limits.max_image_array_layers = 2 * 1024;
    _sg.limits.max_vertex_attrs = SG_MAX_VERTEX_ATTRIBUTES;

    _sg_pixelformat_all(&_sg.formats[SG_PIXELFORMAT_R8]);
    _sg_pixelformat_sf(&_sg.formats[SG_PIXELFORMAT_R8SN]);
    _sg_pixelformat_srm(&_sg.formats[SG_PIXELFORMAT_R8UI]);
    _sg_pixelformat_srm(&_sg.formats[SG_PIXELFORMAT_R8SI]);
    _sg_pixelformat_srm(&_sg.formats[SG_PIXELFORMAT_R16UI]);
    _sg_pixelformat_srm(&_sg.formats[SG_PIXELFORMAT_R16SI]);
    _sg_pixelformat_all(&_sg.formats[SG_PIXELFORMAT_R16F]);
    _sg_pixelformat_all(&_sg.formats[SG_PIXELFORMAT_RG8]);
    _sg_pixelformat_sf(&_sg.formats[SG_PIXELFORMAT_RG8SN]);
    _sg_pixelformat_srm(&_sg.formats[SG_PIXELFORMAT_RG8UI]);
    _sg_pixelformat_srm(&_sg.formats[SG_PIXELFORMAT_RG8SI]);
    _sg_pixelformat_sr(&_sg.formats[SG_PIXELFORMAT_R32UI]);
    _sg_pixelformat_sr(&_sg.formats[SG_PIXELFORMAT_R32SI]);
    _sg_pixelformat_sbr(&_sg.formats[SG_PIXELFORMAT_R32F]);
    _sg_pixelformat_srm(&_sg.formats[SG_PIXELFORMAT_RG16UI]);
    _sg_pixelformat_srm(&_sg.formats[SG_PIXELFORMAT_RG16SI]);
    _sg_pixelformat_all(&_sg.formats[SG_PIXELFORMAT_RG16F]);
    _sg_pixelformat_all(&_sg.formats[SG_PIXELFORMAT_RGBA8]);
    _sg_pixelformat_sf(&_sg.formats[SG_PIXELFORMAT_RGBA8SN]);
    _sg_pixelformat_srm(&_sg.formats[SG_PIXELFORMAT_RGBA8UI]);
    _sg_pixelformat_srm(&_sg.formats[SG_PIXELFORMAT_RGBA8SI]);
    _sg_pixelformat_all(&_sg.formats[SG_PIXELFORMAT_BGRA8]);
    _sg_pixelformat_all(&_sg.formats[SG_PIXELFORMAT_RGB10A2]);
    /* FIXME: missing SG_PIXELFORMAT_RG11B10F */
    _sg_pixelformat_sr(&_sg.formats[SG_PIXELFORMAT_RG32UI]);
    _sg_pixelformat_sr(&_sg.formats[SG_PIXELFORMAT_RG32SI]);
    _sg_pixelformat_sbr(&_sg.formats[SG_PIXELFORMAT_RG32F]);
    _sg_pixelformat_srm(&_sg.formats[SG_PIXELFORMAT_RGBA16UI]);
    _sg_pixelformat_srm(&_sg.formats[SG_PIXELFORMAT_RGBA16SI]);
    _sg_pixelformat_all(&_sg.formats[SG_PIXELFORMAT_RGBA16F]);
    _sg_pixelformat_sr(&_sg.formats[SG_PIXELFORMAT_RGBA32UI]);
    _sg_pixelformat_sr(&_sg.formats[SG_PIXELFORMAT_RGBA32SI]);
    _sg_pixelformat_sr(&_sg.formats[SG_PIXELFORMAT_RGBA32F]);
    _sg_pixelformat_srmd(&_sg.formats[SG_PIXELFORMAT_DEPTH]);
    _sg_pixelformat_srmd(&_sg.formats[SG_PIXELFORMAT_DEPTH_STENCIL]);

    /* FIXME FIXME FIXME: need to check if BC texture compression is
        actually supported, currently the WebGPU C-API doesn't allow this
    */
    _sg_pixelformat_sf(&_sg.formats[SG_PIXELFORMAT_BC1_RGBA]);
    _sg_pixelformat_sf(&_sg.formats[SG_PIXELFORMAT_BC2_RGBA]);
    _sg_pixelformat_sf(&_sg.formats[SG_PIXELFORMAT_BC3_RGBA]);
    _sg_pixelformat_sf(&_sg.formats[SG_PIXELFORMAT_BC4_R]);
    _sg_pixelformat_sf(&_sg.formats[SG_PIXELFORMAT_BC4_RSN]);
    _sg_pixelformat_sf(&_sg.formats[SG_PIXELFORMAT_BC5_RG]);
    _sg_pixelformat_sf(&_sg.formats[SG_PIXELFORMAT_BC5_RGSN]);
    _sg_pixelformat_sf(&_sg.formats[SG_PIXELFORMAT_BC6H_RGBF]);
    _sg_pixelformat_sf(&_sg.formats[SG_PIXELFORMAT_BC6H_RGBUF]);
    _sg_pixelformat_sf(&_sg.formats[SG_PIXELFORMAT_BC7_RGBA]);
}

/*
    WGPU uniform buffer pool implementation:

    At start of frame, a mapped buffer is grabbed from the pool,
    or a new buffer is created if there is no mapped buffer available.

    At end of frame, the current buffer is unmapped before queue submit,
    and async-mapped immediately again.

    UNIFORM BUFFER FIXME:

    - As per WebGPU spec, it should be possible to create a Uniform|MapWrite
      buffer, but this isn't currently allowed in Dawn.
*/
_SOKOL_PRIVATE void _sg_wgpu_ubpool_init(const sg_desc* desc) {

    /* Add the max-uniform-update size (64 KB) to the requested buffer size,
       this is to prevent validation errors in the WebGPU implementation
       if the entire buffer size is used per frame. 64 KB is the allowed
       max uniform update size on NVIDIA
    */
    _sg.wgpu.ub.num_bytes = desc->uniform_buffer_size + _SG_WGPU_MAX_UNIFORM_UPDATE_SIZE;

    WGPUBufferDescriptor ub_desc;
    _sg_clear(&ub_desc, sizeof(ub_desc));
    ub_desc.size = _sg.wgpu.ub.num_bytes;
    ub_desc.usage = WGPUBufferUsage_Uniform|WGPUBufferUsage_CopyDst;
    _sg.wgpu.ub.buf = wgpuDeviceCreateBuffer(_sg.wgpu.dev, &ub_desc);
    SOKOL_ASSERT(_sg.wgpu.ub.buf);

    WGPUBindGroupLayoutBinding ub_bglb_desc[SG_NUM_SHADER_STAGES][SG_MAX_SHADERSTAGE_UBS];
    _sg_clear(ub_bglb_desc, sizeof(ub_bglb_desc));
    for (int stage_index = 0; stage_index < SG_NUM_SHADER_STAGES; stage_index++) {
        WGPUShaderStage vis = (stage_index == SG_SHADERSTAGE_VS) ? WGPUShaderStage_Vertex : WGPUShaderStage_Fragment;
        for (int ub_index = 0; ub_index < SG_MAX_SHADERSTAGE_UBS; ub_index++) {
            int bind_index = stage_index * SG_MAX_SHADERSTAGE_UBS + ub_index;
            ub_bglb_desc[stage_index][ub_index].binding = bind_index;
            ub_bglb_desc[stage_index][ub_index].visibility = vis;
            ub_bglb_desc[stage_index][ub_index].type = WGPUBindingType_UniformBuffer;
            ub_bglb_desc[stage_index][ub_index].hasDynamicOffset = true;
        }
    }

    WGPUBindGroupLayoutDescriptor ub_bgl_desc;
    _sg_clear(&ub_bgl_desc, sizeof(ub_bgl_desc));
    ub_bgl_desc.bindingCount = SG_NUM_SHADER_STAGES * SG_MAX_SHADERSTAGE_UBS;
    ub_bgl_desc.bindings = &ub_bglb_desc[0][0];
    _sg.wgpu.ub.bindgroup_layout = wgpuDeviceCreateBindGroupLayout(_sg.wgpu.dev, &ub_bgl_desc);
    SOKOL_ASSERT(_sg.wgpu.ub.bindgroup_layout);

    WGPUBindGroupBinding ub_bgb[SG_NUM_SHADER_STAGES][SG_MAX_SHADERSTAGE_UBS];
    _sg_clear(ub_bgb, sizeof(ub_bgb));
    for (int stage_index = 0; stage_index < SG_NUM_SHADER_STAGES; stage_index++) {
        for (int ub_index = 0; ub_index < SG_MAX_SHADERSTAGE_UBS; ub_index++) {
            int bind_index = stage_index * SG_MAX_SHADERSTAGE_UBS + ub_index;
            ub_bgb[stage_index][ub_index].binding = bind_index;
            ub_bgb[stage_index][ub_index].buffer = _sg.wgpu.ub.buf;
            // FIXME FIXME FIXME FIXME: HACK FOR VALIDATION BUG IN DAWN
            ub_bgb[stage_index][ub_index].size = (1<<16);
        }
    }
    WGPUBindGroupDescriptor bg_desc;
    _sg_clear(&bg_desc, sizeof(bg_desc));
    bg_desc.layout = _sg.wgpu.ub.bindgroup_layout;
    bg_desc.bindingCount = SG_NUM_SHADER_STAGES * SG_MAX_SHADERSTAGE_UBS;
    bg_desc.bindings = &ub_bgb[0][0];
    _sg.wgpu.ub.bindgroup = wgpuDeviceCreateBindGroup(_sg.wgpu.dev, &bg_desc);
    SOKOL_ASSERT(_sg.wgpu.ub.bindgroup);
}

_SOKOL_PRIVATE void _sg_wgpu_ubpool_discard(void) {
    if (_sg.wgpu.ub.buf) {
        wgpuBufferRelease(_sg.wgpu.ub.buf);
        _sg.wgpu.ub.buf = 0;
    }
    if (_sg.wgpu.ub.bindgroup) {
        wgpuBindGroupRelease(_sg.wgpu.ub.bindgroup);
        _sg.wgpu.ub.bindgroup = 0;
    }
    if (_sg.wgpu.ub.bindgroup_layout) {
        wgpuBindGroupLayoutRelease(_sg.wgpu.ub.bindgroup_layout);
        _sg.wgpu.ub.bindgroup_layout = 0;
    }
    for (int i = 0; i < _sg.wgpu.ub.stage.num; i++) {
        if (_sg.wgpu.ub.stage.buf[i]) {
            wgpuBufferRelease(_sg.wgpu.ub.stage.buf[i]);
            _sg.wgpu.ub.stage.buf[i] = 0;
            _sg.wgpu.ub.stage.ptr[i] = 0;
        }
    }
}

_SOKOL_PRIVATE void _sg_wgpu_ubpool_mapped_callback(WGPUBufferMapAsyncStatus status, void* data, uint64_t data_len, void* user_data) {
    if (!_sg.wgpu.valid) {
        return;
    }
    /* FIXME: better handling for this */
    if (WGPUBufferMapAsyncStatus_Success != status) {
        SG_LOG("Mapping uniform buffer failed!\n");
        SOKOL_ASSERT(false);
    }
    SOKOL_ASSERT(data && (data_len == _sg.wgpu.ub.num_bytes));
    int index = (int)(intptr_t) user_data;
    SOKOL_ASSERT(index < _sg.wgpu.ub.stage.num);
    SOKOL_ASSERT(0 == _sg.wgpu.ub.stage.ptr[index]);
    _sg.wgpu.ub.stage.ptr[index] = (uint8_t*) data;
}

_SOKOL_PRIVATE void _sg_wgpu_ubpool_next_frame(bool first_frame) {

    /* immediately request a new mapping for the last frame's current staging buffer */
    if (!first_frame) {
        WGPUBuffer ub_src = _sg.wgpu.ub.stage.buf[_sg.wgpu.ub.stage.cur];
        wgpuBufferMapWriteAsync(ub_src, _sg_wgpu_ubpool_mapped_callback, (void*)(intptr_t)_sg.wgpu.ub.stage.cur);
    }

    /* rewind per-frame offsets */
    _sg.wgpu.ub.offset = 0;
    _sg_clear(&_sg.wgpu.ub.bind_offsets, sizeof(_sg.wgpu.ub.bind_offsets));

    /* check if a mapped staging buffer is available, otherwise create one */
    for (int i = 0; i < _sg.wgpu.ub.stage.num; i++) {
        if (_sg.wgpu.ub.stage.ptr[i]) {
            _sg.wgpu.ub.stage.cur = i;
            return;
        }
    }

    /* no mapped uniform buffer available, create one */
    SOKOL_ASSERT(_sg.wgpu.ub.stage.num < _SG_WGPU_STAGING_PIPELINE_SIZE);
    _sg.wgpu.ub.stage.cur = _sg.wgpu.ub.stage.num++;
    const int cur = _sg.wgpu.ub.stage.cur;

    WGPUBufferDescriptor desc;
    _sg_clear(&desc, sizeof(desc));
    desc.size = _sg.wgpu.ub.num_bytes;
    desc.usage = WGPUBufferUsage_CopySrc|WGPUBufferUsage_MapWrite;
    WGPUCreateBufferMappedResult res = wgpuDeviceCreateBufferMapped(_sg.wgpu.dev, &desc);
    _sg.wgpu.ub.stage.buf[cur] = res.buffer;
    _sg.wgpu.ub.stage.ptr[cur] = (uint8_t*) res.data;
    SOKOL_ASSERT(_sg.wgpu.ub.stage.buf[cur]);
    SOKOL_ASSERT(_sg.wgpu.ub.stage.ptr[cur]);
    SOKOL_ASSERT(res.dataLength == _sg.wgpu.ub.num_bytes);
}

_SOKOL_PRIVATE void _sg_wgpu_ubpool_flush(void) {
    /* unmap staging buffer and copy to uniform buffer */
    const int cur = _sg.wgpu.ub.stage.cur;
    SOKOL_ASSERT(_sg.wgpu.ub.stage.ptr[cur]);
    _sg.wgpu.ub.stage.ptr[cur] = 0;
    WGPUBuffer src_buf = _sg.wgpu.ub.stage.buf[cur];
    wgpuBufferUnmap(src_buf);
    if (_sg.wgpu.ub.offset > 0) {
        WGPUBuffer dst_buf = _sg.wgpu.ub.buf;
        wgpuCommandEncoderCopyBufferToBuffer(_sg.wgpu.render_cmd_enc, src_buf, 0, dst_buf, 0, _sg.wgpu.ub.offset);
    }
}

/* helper function to compute number of bytes needed in staging buffer to copy image data */
_SOKOL_PRIVATE uint32_t _sg_wgpu_image_data_buffer_size(const _sg_image_t* img) {
    uint32_t num_bytes = 0;
    const uint32_t num_faces = (img->cmn.type == SG_IMAGETYPE_CUBE) ? 6:1;
    const uint32_t num_slices = (img->cmn.type == SG_IMAGETYPE_ARRAY) ? img->cmn.num_slices : 1;
    for (int mip_index = 0; mip_index < img->cmn.num_mipmaps; mip_index++) {
        const uint32_t mip_width = _sg_max(img->cmn.width >> mip_index, 1);
        const uint32_t mip_height = _sg_max(img->cmn.height >> mip_index, 1);
        /* row-pitch must be 256-aligend */
        const uint32_t bytes_per_slice = _sg_surface_pitch(img->cmn.pixel_format, mip_width, mip_height, _SG_WGPU_ROWPITCH_ALIGN);
        num_bytes += bytes_per_slice * num_slices * num_faces;
    }
    return num_bytes;
}

/* helper function to copy image data into a texture via a staging buffer, returns number of
   bytes copied
*/
_SOKOL_PRIVATE uint32_t _sg_wgpu_copy_image_data(WGPUBuffer stg_buf, uint8_t* stg_base_ptr, uint32_t stg_base_offset, _sg_image_t* img, const sg_image_data* data) {
    SOKOL_ASSERT(_sg.wgpu.staging_cmd_enc);
    SOKOL_ASSERT(stg_buf && stg_base_ptr);
    SOKOL_ASSERT(img);
    SOKOL_ASSERT(data);
    uint32_t stg_offset = stg_base_offset;
    const uint32_t num_faces = (img->cmn.type == SG_IMAGETYPE_CUBE) ? 6:1;
    const uint32_t num_slices = (img->cmn.type == SG_IMAGETYPE_ARRAY) ? img->cmn.num_slices : 1;
    const sg_pixel_format fmt = img->cmn.pixel_format;
    WGPUBufferCopyView src_view;
    _sg_clear(&src_view, sizeof(src_view));
    src_view.buffer = stg_buf;
    WGPUTextureCopyView dst_view;
    _sg_clear(&dst_view, sizeof(dst_view));
    dst_view.texture = img->wgpu.tex;
    WGPUExtent3D extent;
    _sg_clear(&extent, sizeof(extent));

    for (uint32_t face_index = 0; face_index < num_faces; face_index++) {
        for (uint32_t mip_index = 0; mip_index < (uint32_t)img->cmn.num_mipmaps; mip_index++) {
            SOKOL_ASSERT(data->subimage[face_index][mip_index].ptr);
            SOKOL_ASSERT(data->subimage[face_index][mip_index].size > 0);
            const uint8_t* src_base_ptr = (const uint8_t*)data->subimage[face_index][mip_index].ptr;
            SOKOL_ASSERT(src_base_ptr);
            uint8_t* dst_base_ptr = stg_base_ptr + stg_offset;

            const uint32_t mip_width  = _sg_max(img->cmn.width >> mip_index, 1);
            const uint32_t mip_height = _sg_max(img->cmn.height >> mip_index, 1);
            const uint32_t mip_depth  = (img->cmn.type == SG_IMAGETYPE_3D) ? _sg_max(img->cmn.num_slices >> mip_index, 1) : 1;
            const uint32_t num_rows   = _sg_num_rows(fmt, mip_height);
            const uint32_t src_bytes_per_row   = _sg_row_pitch(fmt, mip_width, 1);
            const uint32_t dst_bytes_per_row   = _sg_row_pitch(fmt, mip_width, _SG_WGPU_ROWPITCH_ALIGN);
            const uint32_t src_bytes_per_slice = _sg_surface_pitch(fmt, mip_width, mip_height, 1);
            const uint32_t dst_bytes_per_slice = _sg_surface_pitch(fmt, mip_width, mip_height, _SG_WGPU_ROWPITCH_ALIGN);
            SOKOL_ASSERT((uint32_t)data->subimage[face_index][mip_index].size == (src_bytes_per_slice * num_slices));
            SOKOL_ASSERT(src_bytes_per_row <= dst_bytes_per_row);
            SOKOL_ASSERT(src_bytes_per_slice == (src_bytes_per_row * num_rows));
            SOKOL_ASSERT(dst_bytes_per_slice == (dst_bytes_per_row * num_rows));
            _SOKOL_UNUSED(src_bytes_per_slice);

            /* copy data into mapped staging buffer */
            if (src_bytes_per_row == dst_bytes_per_row) {
                /* can do a single memcpy */
                uint32_t num_bytes = data->subimage[face_index][mip_index].size;
                memcpy(dst_base_ptr, src_base_ptr, num_bytes);
            }
            else {
                /* src/dst pitch doesn't match, need to copy row by row */
                uint8_t* dst_ptr = dst_base_ptr;
                const uint8_t* src_ptr = src_base_ptr;
                for (uint32_t slice_index = 0; slice_index < num_slices; slice_index++) {
                    SOKOL_ASSERT(dst_ptr == dst_base_ptr + slice_index * dst_bytes_per_slice);
                    for (uint32_t row_index = 0; row_index < num_rows; row_index++) {
                        memcpy(dst_ptr, src_ptr, src_bytes_per_row);
                        src_ptr += src_bytes_per_row;
                        dst_ptr += dst_bytes_per_row;
                    }
                }
            }

            /* record the staging copy operation into command encoder */
            src_view.imageHeight = mip_height;
            src_view.rowPitch = dst_bytes_per_row;
            dst_view.mipLevel = mip_index;
            extent.width = mip_width;
            extent.height = mip_height;
            extent.depth = mip_depth;
            SOKOL_ASSERT((img->cmn.type != SG_IMAGETYPE_CUBE) || (num_slices == 1));
            for (uint32_t slice_index = 0; slice_index < num_slices; slice_index++) {
                const uint32_t layer_index = (img->cmn.type == SG_IMAGETYPE_ARRAY) ? slice_index : face_index;
                src_view.offset = stg_offset;
                dst_view.arrayLayer = layer_index;
                wgpuCommandEncoderCopyBufferToTexture(_sg.wgpu.staging_cmd_enc, &src_view, &dst_view, &extent);
                stg_offset += dst_bytes_per_slice;
                SOKOL_ASSERT(stg_offset <= _sg.wgpu.staging.num_bytes);
            }
        }
    }
    SOKOL_ASSERT(stg_offset >= stg_base_offset);
    return (stg_offset - stg_base_offset);
}

/*
    The WGPU staging buffer implementation:

    Very similar to the uniform buffer pool, there's a pool of big
    per-frame staging buffers, each must be big enough to hold
    all data uploaded to dynamic resources for one frame.

    Staging buffers are created on demand and reused, because the
    'frame pipeline depth' of WGPU isn't predictable.

    The difference to the uniform buffer system is that there isn't
    a 1:1 relationship for source- and destination for the
    data-copy operation. There's always one staging buffer as copy-source
    per frame, but many copy-destinations (regular vertex/index buffers
    or images). Instead of one big copy-operation at the end of the frame,
    multiple copy-operations will be written throughout the frame.
*/
_SOKOL_PRIVATE void _sg_wgpu_staging_init(const sg_desc* desc) {
    SOKOL_ASSERT(desc && (desc->staging_buffer_size > 0));
    _sg.wgpu.staging.num_bytes = desc->staging_buffer_size;
    /* there's actually nothing more to do here */
}

_SOKOL_PRIVATE void _sg_wgpu_staging_discard(void) {
    for (int i = 0; i < _sg.wgpu.staging.num; i++) {
        if (_sg.wgpu.staging.buf[i]) {
            wgpuBufferRelease(_sg.wgpu.staging.buf[i]);
            _sg.wgpu.staging.buf[i] = 0;
            _sg.wgpu.staging.ptr[i] = 0;
        }
    }
}

_SOKOL_PRIVATE void _sg_wgpu_staging_mapped_callback(WGPUBufferMapAsyncStatus status, void* data, uint64_t data_len, void* user_data) {
    if (!_sg.wgpu.valid) {
        return;
    }
    /* FIXME: better handling for this */
    if (WGPUBufferMapAsyncStatus_Success != status) {
        SOKOL_ASSERT("Mapping staging buffer failed!\n");
        SOKOL_ASSERT(false);
    }
    SOKOL_ASSERT(data && (data_len == _sg.wgpu.staging.num_bytes));
    int index = (int)(intptr_t) user_data;
    SOKOL_ASSERT(index < _sg.wgpu.staging.num);
    SOKOL_ASSERT(0 == _sg.wgpu.staging.ptr[index]);
    _sg.wgpu.staging.ptr[index] = (uint8_t*) data;
}

_SOKOL_PRIVATE void _sg_wgpu_staging_next_frame(bool first_frame) {

    /* immediately request a new mapping for the last frame's current staging buffer */
    if (!first_frame) {
        WGPUBuffer cur_buf = _sg.wgpu.staging.buf[_sg.wgpu.staging.cur];
        wgpuBufferMapWriteAsync(cur_buf, _sg_wgpu_staging_mapped_callback, (void*)(intptr_t)_sg.wgpu.staging.cur);
    }

    /* rewind staging-buffer offset */
    _sg.wgpu.staging.offset = 0;

    /* check if mapped staging buffer is available, otherwise create one */
    for (int i = 0; i < _sg.wgpu.staging.num; i++) {
        if (_sg.wgpu.staging.ptr[i]) {
            _sg.wgpu.staging.cur = i;
            return;
        }
    }

    /* no mapped buffer available, create one */
    SOKOL_ASSERT(_sg.wgpu.staging.num < _SG_WGPU_STAGING_PIPELINE_SIZE);
    _sg.wgpu.staging.cur = _sg.wgpu.staging.num++;
    const int cur = _sg.wgpu.staging.cur;

    WGPUBufferDescriptor desc;
    _sg_clear(&desc, sizeof(desc));
    desc.size = _sg.wgpu.staging.num_bytes;
    desc.usage = WGPUBufferUsage_CopySrc|WGPUBufferUsage_MapWrite;
    WGPUCreateBufferMappedResult res = wgpuDeviceCreateBufferMapped(_sg.wgpu.dev, &desc);
    _sg.wgpu.staging.buf[cur] = res.buffer;
    _sg.wgpu.staging.ptr[cur] = (uint8_t*) res.data;
    SOKOL_ASSERT(_sg.wgpu.staging.buf[cur]);
    SOKOL_ASSERT(_sg.wgpu.staging.ptr[cur]);
    SOKOL_ASSERT(res.dataLength == _sg.wgpu.staging.num_bytes);
}

_SOKOL_PRIVATE uint32_t _sg_wgpu_staging_copy_to_buffer(WGPUBuffer dst_buf, uint32_t dst_buf_offset, const void* data, uint32_t data_num_bytes) {
    /* Copy a chunk of data into the staging buffer, and record a blit-operation into
        the command encoder, bump the offset for the next data chunk, return 0 if there
        was not enough room in the staging buffer, return the number of actually
        copied bytes on success.

        NOTE: that the number of staging bytes to be copied must be a multiple of 4.

    */
    SOKOL_ASSERT(_sg.wgpu.staging_cmd_enc);
    SOKOL_ASSERT((dst_buf_offset & 3) == 0);
    SOKOL_ASSERT(data_num_bytes > 0);
    uint32_t copy_num_bytes = _sg_roundup(data_num_bytes, 4);
    if ((_sg.wgpu.staging.offset + copy_num_bytes) >= _sg.wgpu.staging.num_bytes) {
        SG_LOG("WGPU: Per frame staging buffer full (in _sg_wgpu_staging_copy_to_buffer())!\n");
        return false;
    }
    const int cur = _sg.wgpu.staging.cur;
    SOKOL_ASSERT(_sg.wgpu.staging.ptr[cur]);
    uint32_t stg_buf_offset = _sg.wgpu.staging.offset;
    uint8_t* stg_ptr = _sg.wgpu.staging.ptr[cur] + stg_buf_offset;
    memcpy(stg_ptr, data, data_num_bytes);
    WGPUBuffer stg_buf = _sg.wgpu.staging.buf[cur];
    wgpuCommandEncoderCopyBufferToBuffer(_sg.wgpu.staging_cmd_enc, stg_buf, stg_buf_offset, dst_buf, dst_buf_offset, copy_num_bytes);
    _sg.wgpu.staging.offset = stg_buf_offset + copy_num_bytes;
    return copy_num_bytes;
}

_SOKOL_PRIVATE bool _sg_wgpu_staging_copy_to_texture(_sg_image_t* img, const sg_image_data* data) {
    /* similar to _sg_wgpu_staging_copy_to_buffer(), but with image data instead */
    SOKOL_ASSERT(_sg.wgpu.staging_cmd_enc);
    uint32_t num_bytes = _sg_wgpu_image_data_buffer_size(img);
    if ((_sg.wgpu.staging.offset + num_bytes) >= _sg.wgpu.staging.num_bytes) {
        SG_LOG("WGPU: Per frame staging buffer full (in _sg_wgpu_staging_copy_to_texture)!\n");
        return false;
    }
    const int cur = _sg.wgpu.staging.cur;
    SOKOL_ASSERT(_sg.wgpu.staging.ptr[cur]);
    uint32_t stg_offset = _sg.wgpu.staging.offset;
    uint8_t* stg_ptr = _sg.wgpu.staging.ptr[cur];
    WGPUBuffer stg_buf = _sg.wgpu.staging.buf[cur];
    uint32_t bytes_copied = _sg_wgpu_copy_image_data(stg_buf, stg_ptr, stg_offset, img, data);
    _SOKOL_UNUSED(bytes_copied);
    SOKOL_ASSERT(bytes_copied == num_bytes);
    _sg.wgpu.staging.offset = _sg_roundup(stg_offset + num_bytes, _SG_WGPU_STAGING_ALIGN);
    return true;
}

_SOKOL_PRIVATE void _sg_wgpu_staging_unmap(void) {
    /* called at end of frame before queue-submit */
    const int cur = _sg.wgpu.staging.cur;
    SOKOL_ASSERT(_sg.wgpu.staging.ptr[cur]);
    _sg.wgpu.staging.ptr[cur] = 0;
    wgpuBufferUnmap(_sg.wgpu.staging.buf[cur]);
}

/*--- WGPU sampler cache functions ---*/
_SOKOL_PRIVATE void _sg_wgpu_init_sampler_cache(const sg_desc* desc) {
    SOKOL_ASSERT(desc->sampler_cache_size > 0);
    _sg_smpcache_init(&_sg.wgpu.sampler_cache, desc->sampler_cache_size);
}

_SOKOL_PRIVATE void _sg_wgpu_destroy_sampler_cache(void) {
    SOKOL_ASSERT(_sg.wgpu.sampler_cache.items);
    SOKOL_ASSERT(_sg.wgpu.sampler_cache.num_items <= _sg.wgpu.sampler_cache.capacity);
    for (int i = 0; i < _sg.wgpu.sampler_cache.num_items; i++) {
        wgpuSamplerRelease((WGPUSampler)_sg_smpcache_sampler(&_sg.wgpu.sampler_cache, i));
    }
    _sg_smpcache_discard(&_sg.wgpu.sampler_cache);
}

_SOKOL_PRIVATE WGPUSampler _sg_wgpu_create_sampler(const sg_image_desc* img_desc) {
    SOKOL_ASSERT(img_desc);
    int index = _sg_smpcache_find_item(&_sg.wgpu.sampler_cache, img_desc);
    if (index >= 0) {
        /* reuse existing sampler */
        return (WGPUSampler) _sg_smpcache_sampler(&_sg.wgpu.sampler_cache, index);
    }
    else {
        /* create a new WGPU sampler and add to sampler cache */
        /* FIXME: anisotropic filtering not supported? */
        WGPUSamplerDescriptor smp_desc;
        _sg_clear(&smp_desc, sizeof(smp_desc));
        smp_desc.addressModeU = _sg_wgpu_sampler_addrmode(img_desc->wrap_u);
        smp_desc.addressModeV = _sg_wgpu_sampler_addrmode(img_desc->wrap_v);
        smp_desc.addressModeW = _sg_wgpu_sampler_addrmode(img_desc->wrap_w);
        smp_desc.magFilter = _sg_wgpu_sampler_minmagfilter(img_desc->mag_filter);
        smp_desc.minFilter = _sg_wgpu_sampler_minmagfilter(img_desc->min_filter);
        smp_desc.mipmapFilter = _sg_wgpu_sampler_mipfilter(img_desc->min_filter);
        smp_desc.lodMinClamp = img_desc->min_lod;
        smp_desc.lodMaxClamp = img_desc->max_lod;
        WGPUSampler smp = wgpuDeviceCreateSampler(_sg.wgpu.dev, &smp_desc);
        SOKOL_ASSERT(smp);
        _sg_smpcache_add_item(&_sg.wgpu.sampler_cache, img_desc, (uintptr_t)smp);
        return smp;
    }
}

/*--- WGPU backend API functions ---*/
_SOKOL_PRIVATE void _sg_wgpu_setup_backend(const sg_desc* desc) {
    SOKOL_ASSERT(desc);
    SOKOL_ASSERT(desc->context.wgpu.device);
    SOKOL_ASSERT(desc->context.wgpu.render_view_cb || desc->context.wgpu.render_view_userdata_cb);
    SOKOL_ASSERT(desc->context.wgpu.resolve_view_cb || desc->context.wgpu.resolve_view_userdata_cb);
    SOKOL_ASSERT(desc->context.wgpu.depth_stencil_view_cb || desc->context.wgpu.depth_stencil_view_userdata_cb);
    SOKOL_ASSERT(desc->uniform_buffer_size > 0);
    SOKOL_ASSERT(desc->staging_buffer_size > 0);
    _sg.backend = SG_BACKEND_WGPU;
    _sg.wgpu.valid = true;
    _sg.wgpu.dev = (WGPUDevice) desc->context.wgpu.device;
    _sg.wgpu.render_view_cb = (WGPUTextureView(*)(void)) desc->context.wgpu.render_view_cb;
    _sg.wgpu.render_view_userdata_cb = (WGPUTextureView(*)(void*)) desc->context.wgpu.render_view_userdata_cb;
    _sg.wgpu.resolve_view_cb = (WGPUTextureView(*)(void)) desc->context.wgpu.resolve_view_cb;
    _sg.wgpu.resolve_view_userdata_cb = (WGPUTextureView(*)(void*)) desc->context.wgpu.resolve_view_userdata_cb;
    _sg.wgpu.depth_stencil_view_cb = (WGPUTextureView(*)(void)) desc->context.wgpu.depth_stencil_view_cb;
    _sg.wgpu.depth_stencil_view_userdata_cb = (WGPUTextureView(*)(void*)) desc->context.wgpu.depth_stencil_view_userdata_cb;
    _sg.wgpu.user_data = desc->context.wgpu.user_data;
    _sg.wgpu.queue = wgpuDeviceCreateQueue(_sg.wgpu.dev);
    SOKOL_ASSERT(_sg.wgpu.queue);

    /* setup WebGPU features and limits */
    _sg_wgpu_init_caps();

    /* setup the sampler cache, uniform and staging buffer pools */
    _sg_wgpu_init_sampler_cache(&_sg.desc);
    _sg_wgpu_ubpool_init(desc);
    _sg_wgpu_ubpool_next_frame(true);
    _sg_wgpu_staging_init(desc);
    _sg_wgpu_staging_next_frame(true);

    /* create an empty bind group for shader stages without bound images */
    WGPUBindGroupLayoutDescriptor bgl_desc;
    _sg_clear(&bgl_desc, sizeof(bgl_desc));
    WGPUBindGroupLayout empty_bgl = wgpuDeviceCreateBindGroupLayout(_sg.wgpu.dev, &bgl_desc);
    SOKOL_ASSERT(empty_bgl);
    WGPUBindGroupDescriptor bg_desc;
    _sg_clear(&bg_desc, sizeof(bg_desc));
    bg_desc.layout = empty_bgl;
    _sg.wgpu.empty_bind_group = wgpuDeviceCreateBindGroup(_sg.wgpu.dev, &bg_desc);
    SOKOL_ASSERT(_sg.wgpu.empty_bind_group);
    wgpuBindGroupLayoutRelease(empty_bgl);

    /* create initial per-frame command encoders */
    WGPUCommandEncoderDescriptor cmd_enc_desc;
    _sg_clear(&cmd_enc_desc, sizeof(cmd_enc_desc));
    _sg.wgpu.render_cmd_enc = wgpuDeviceCreateCommandEncoder(_sg.wgpu.dev, &cmd_enc_desc);
    SOKOL_ASSERT(_sg.wgpu.render_cmd_enc);
    _sg.wgpu.staging_cmd_enc = wgpuDeviceCreateCommandEncoder(_sg.wgpu.dev, &cmd_enc_desc);
    SOKOL_ASSERT(_sg.wgpu.staging_cmd_enc);
}

_SOKOL_PRIVATE void _sg_wgpu_discard_backend(void) {
    SOKOL_ASSERT(_sg.wgpu.valid);
    SOKOL_ASSERT(_sg.wgpu.render_cmd_enc);
    SOKOL_ASSERT(_sg.wgpu.staging_cmd_enc);
    _sg.wgpu.valid = false;
    _sg_wgpu_ubpool_discard();
    _sg_wgpu_staging_discard();
    _sg_wgpu_destroy_sampler_cache();
    wgpuBindGroupRelease(_sg.wgpu.empty_bind_group);
    wgpuCommandEncoderRelease(_sg.wgpu.render_cmd_enc);
    _sg.wgpu.render_cmd_enc = 0;
    wgpuCommandEncoderRelease(_sg.wgpu.staging_cmd_enc);
    _sg.wgpu.staging_cmd_enc = 0;
    if (_sg.wgpu.queue) {
        wgpuQueueRelease(_sg.wgpu.queue);
        _sg.wgpu.queue = 0;
    }
}

_SOKOL_PRIVATE void _sg_wgpu_reset_state_cache(void) {
    SG_LOG("_sg_wgpu_reset_state_cache: FIXME\n");
}

_SOKOL_PRIVATE sg_resource_state _sg_wgpu_create_context(_sg_context_t* ctx) {
    SOKOL_ASSERT(ctx);
    _SOKOL_UNUSED(ctx);
    return SG_RESOURCESTATE_VALID;
}

_SOKOL_PRIVATE void _sg_wgpu_discard_context(_sg_context_t* ctx) {
    SOKOL_ASSERT(ctx);
    _SOKOL_UNUSED(ctx);
}

_SOKOL_PRIVATE void _sg_wgpu_activate_context(_sg_context_t* ctx) {
    (void)ctx;
    SG_LOG("_sg_wgpu_activate_context: FIXME\n");
}

_SOKOL_PRIVATE sg_resource_state _sg_wgpu_create_buffer(_sg_buffer_t* buf, const sg_buffer_desc* desc) {
    SOKOL_ASSERT(buf && desc);
    const bool injected = (0 != desc->wgpu_buffer);
    _sg_buffer_common_init(&buf->cmn, desc);
    if (injected) {
        buf->wgpu.buf = (WGPUBuffer) desc->wgpu_buffer;
        wgpuBufferReference(buf->wgpu.buf);
    }
    else {
        WGPUBufferDescriptor wgpu_buf_desc;
        _sg_clear(&wgpu_buf_desc, sizeof(wgpu_buf_desc));
        wgpu_buf_desc.usage = _sg_wgpu_buffer_usage(buf->cmn.type, buf->cmn.usage);
        wgpu_buf_desc.size = buf->cmn.size;
        if (SG_USAGE_IMMUTABLE == buf->cmn.usage) {
            SOKOL_ASSERT(desc->data.ptr);
            WGPUCreateBufferMappedResult res = wgpuDeviceCreateBufferMapped(_sg.wgpu.dev, &wgpu_buf_desc);
            buf->wgpu.buf = res.buffer;
            SOKOL_ASSERT(res.data && (res.dataLength == buf->cmn.size));
            memcpy(res.data, desc->data.ptr, buf->cmn.size);
            wgpuBufferUnmap(res.buffer);
        }
        else {
            buf->wgpu.buf = wgpuDeviceCreateBuffer(_sg.wgpu.dev, &wgpu_buf_desc);
        }
    }
    return SG_RESOURCESTATE_VALID;
}

_SOKOL_PRIVATE void _sg_wgpu_discard_buffer(_sg_buffer_t* buf) {
    SOKOL_ASSERT(buf);
    WGPUBuffer wgpu_buf = buf->wgpu.buf;
    if (0 != wgpu_buf) {
        wgpuBufferRelease(wgpu_buf);
    }
}

_SOKOL_PRIVATE void _sg_wgpu_init_texdesc_common(WGPUTextureDescriptor* wgpu_tex_desc, const sg_image_desc* desc) {
    wgpu_tex_desc->usage = WGPUTextureUsage_Sampled|WGPUTextureUsage_CopyDst;
    wgpu_tex_desc->dimension = _sg_wgpu_tex_dim(desc->type);
    wgpu_tex_desc->size.width = desc->width;
    wgpu_tex_desc->size.height = desc->height;
    if (desc->type == SG_IMAGETYPE_3D) {
        wgpu_tex_desc->size.depth = desc->num_slices;
        wgpu_tex_desc->arrayLayerCount = 1;
    }
    else if (desc->type == SG_IMAGETYPE_CUBE) {
        wgpu_tex_desc->size.depth = 1;
        wgpu_tex_desc->arrayLayerCount = 6;
    }
    else {
        wgpu_tex_desc->size.depth = 1;
        wgpu_tex_desc->arrayLayerCount = desc->num_slices;
    }
    wgpu_tex_desc->format = _sg_wgpu_textureformat(desc->pixel_format);
    wgpu_tex_desc->mipLevelCount = desc->num_mipmaps;
    wgpu_tex_desc->sampleCount = 1;
}

_SOKOL_PRIVATE sg_resource_state _sg_wgpu_create_image(_sg_image_t* img, const sg_image_desc* desc) {
    SOKOL_ASSERT(img && desc);
    SOKOL_ASSERT(_sg.wgpu.dev);
    SOKOL_ASSERT(_sg.wgpu.staging_cmd_enc);

    _sg_image_common_init(&img->cmn, desc);

    const bool injected = (0 != desc->wgpu_texture);
    const bool is_msaa = desc->sample_count > 1;
    WGPUTextureDescriptor wgpu_tex_desc;
    _sg_clear(&wgpu_tex_desc, sizeof(wgpu_tex_desc));
    _sg_wgpu_init_texdesc_common(&wgpu_tex_desc, desc);
    if (_sg_is_valid_rendertarget_depth_format(img->cmn.pixel_format)) {
        SOKOL_ASSERT(img->cmn.render_target);
        SOKOL_ASSERT(img->cmn.type == SG_IMAGETYPE_2D);
        SOKOL_ASSERT(img->cmn.num_mipmaps == 1);
        SOKOL_ASSERT(!injected);
        /* NOTE: a depth-stencil texture will never be MSAA-resolved, so there
           won't be a separate MSAA- and resolve-texture
        */
        wgpu_tex_desc.usage = WGPUTextureUsage_OutputAttachment;
        wgpu_tex_desc.sampleCount = desc->sample_count;
        img->wgpu.tex = wgpuDeviceCreateTexture(_sg.wgpu.dev, &wgpu_tex_desc);
        SOKOL_ASSERT(img->wgpu.tex);
    }
    else {
        if (injected) {
            img->wgpu.tex = (WGPUTexture) desc->wgpu_texture;
            wgpuTextureReference(img->wgpu.tex);
        }
        else {
            /* NOTE: in the MSAA-rendertarget case, both the MSAA texture *and*
               the resolve texture need OutputAttachment usage
            */
            if (img->cmn.render_target) {
                wgpu_tex_desc.usage = WGPUTextureUsage_Sampled|WGPUTextureUsage_OutputAttachment;
            }
            img->wgpu.tex = wgpuDeviceCreateTexture(_sg.wgpu.dev, &wgpu_tex_desc);
            SOKOL_ASSERT(img->wgpu.tex);

            /* copy content into texture via a throw-away staging buffer */
            if (desc->usage == SG_USAGE_IMMUTABLE && !desc->render_target) {
                WGPUBufferDescriptor wgpu_buf_desc;
                _sg_clear(&wgpu_buf_desc, sizeof(wgpu_buf_desc));
                wgpu_buf_desc.size = _sg_wgpu_image_data_buffer_size(img);
                wgpu_buf_desc.usage = WGPUBufferUsage_CopySrc|WGPUBufferUsage_CopyDst;
                WGPUCreateBufferMappedResult map = wgpuDeviceCreateBufferMapped(_sg.wgpu.dev, &wgpu_buf_desc);
                SOKOL_ASSERT(map.buffer && map.data);
                uint32_t num_bytes = _sg_wgpu_copy_image_data(map.buffer, (uint8_t*)map.data, 0, img, &desc->data);
                _SOKOL_UNUSED(num_bytes);
                SOKOL_ASSERT(num_bytes == wgpu_buf_desc.size);
                wgpuBufferUnmap(map.buffer);
                wgpuBufferRelease(map.buffer);
            }
        }

        /* create texture view object */
        WGPUTextureViewDescriptor wgpu_view_desc;
        _sg_clear(&wgpu_view_desc, sizeof(wgpu_view_desc));
        wgpu_view_desc.dimension = _sg_wgpu_tex_viewdim(desc->type);
        img->wgpu.tex_view = wgpuTextureCreateView(img->wgpu.tex, &wgpu_view_desc);

        /* if render target and MSAA, then a separate texture in MSAA format is needed
           which will be resolved into the regular texture at the end of the
           offscreen-render pass
        */
        if (desc->render_target && is_msaa) {
            wgpu_tex_desc.dimension = WGPUTextureDimension_2D;
            wgpu_tex_desc.size.depth = 1;
            wgpu_tex_desc.arrayLayerCount = 1;
            wgpu_tex_desc.mipLevelCount = 1;
            wgpu_tex_desc.usage = WGPUTextureUsage_OutputAttachment;
            wgpu_tex_desc.sampleCount = desc->sample_count;
            img->wgpu.msaa_tex = wgpuDeviceCreateTexture(_sg.wgpu.dev, &wgpu_tex_desc);
            SOKOL_ASSERT(img->wgpu.msaa_tex);
        }

        /* create sampler via shared-sampler-cache */
        img->wgpu.sampler = _sg_wgpu_create_sampler(desc);
        SOKOL_ASSERT(img->wgpu.sampler);
    }
    return SG_RESOURCESTATE_VALID;
}

_SOKOL_PRIVATE void _sg_wgpu_discard_image(_sg_image_t* img) {
    SOKOL_ASSERT(img);
    if (img->wgpu.tex) {
        wgpuTextureRelease(img->wgpu.tex);
        img->wgpu.tex = 0;
    }
    if (img->wgpu.tex_view) {
        wgpuTextureViewRelease(img->wgpu.tex_view);
        img->wgpu.tex_view = 0;
    }
    if (img->wgpu.msaa_tex) {
        wgpuTextureRelease(img->wgpu.msaa_tex);
        img->wgpu.msaa_tex = 0;
    }
    /* NOTE: do *not* destroy the sampler from the shared-sampler-cache */
    img->wgpu.sampler = 0;
}

/*
    How BindGroups work in WebGPU:

    - up to 4 bind groups can be bound simultaneously
    - up to 16 bindings per bind group
    - 'binding' slots are local per bind group
    - in the shader:
        layout(set=0, binding=1) corresponds to bind group 0, binding 1

    Now how to map this to sokol-gfx's bind model:

    Reduce SG_MAX_SHADERSTAGE_IMAGES to 8, then:

        1 bind group for all 8 uniform buffers
        1 bind group for vertex shader textures + samplers
        1 bind group for fragment shader textures + samples

    Alternatively:

        1 bind group for 8 uniform buffer slots
        1 bind group for 8 vs images + 8 vs samplers
        1 bind group for 12 fs images
        1 bind group for 12 fs samplers

    I guess this means that we need to create BindGroups on the
    fly during sg_apply_bindings() :/
*/
_SOKOL_PRIVATE sg_resource_state _sg_wgpu_create_shader(_sg_shader_t* shd, const sg_shader_desc* desc) {
    SOKOL_ASSERT(shd && desc);
    SOKOL_ASSERT(desc->vs.bytecode.ptr && desc->fs.bytecode.ptr);
    _sg_shader_common_init(&shd->cmn, desc);

    bool success = true;
    for (int stage_index = 0; stage_index < SG_NUM_SHADER_STAGES; stage_index++) {
        const sg_shader_stage_desc* stage_desc = (stage_index == SG_SHADERSTAGE_VS) ? &desc->vs : &desc->fs;
        SOKOL_ASSERT((stage_desc->bytecode.size & 3) == 0);

        _sg_shader_stage_t* cmn_stage = &shd->cmn.stage[stage_index];
        _sg_wgpu_shader_stage_t* wgpu_stage = &shd->wgpu.stage[stage_index];

        _sg_strcpy(&wgpu_stage->entry, stage_desc->entry);
        WGPUShaderModuleDescriptor wgpu_shdmod_desc;
        _sg_clear(&wgpu_shdmod_desc, sizeof(wgpu_shdmod_desc));
        wgpu_shdmod_desc.codeSize = stage_desc->bytecode.size >> 2;
        wgpu_shdmod_desc.code = (const uint32_t*) stage_desc->bytecode.ptr;
        wgpu_stage->module = wgpuDeviceCreateShaderModule(_sg.wgpu.dev, &wgpu_shdmod_desc);
        if (0 == wgpu_stage->module) {
            success = false;
        }

        /* create image/sampler bind group for the shader stage */
        WGPUShaderStage vis = (stage_index == SG_SHADERSTAGE_VS) ? WGPUShaderStage_Vertex : WGPUShaderStage_Fragment;
        int num_imgs = cmn_stage->num_images;
        if (num_imgs > _SG_WGPU_MAX_SHADERSTAGE_IMAGES) {
            num_imgs = _SG_WGPU_MAX_SHADERSTAGE_IMAGES;
        }
        WGPUBindGroupLayoutBinding bglb_desc[_SG_WGPU_MAX_SHADERSTAGE_IMAGES * 2];
        _sg_clear(bglb_desc, sizeof(bglb_desc));
        for (int img_index = 0; img_index < num_imgs; img_index++) {
            /* texture- and sampler-bindings */
            WGPUBindGroupLayoutBinding* tex_desc = &bglb_desc[img_index*2 + 0];
            WGPUBindGroupLayoutBinding* smp_desc = &bglb_desc[img_index*2 + 1];

            tex_desc->binding = img_index;
            tex_desc->visibility = vis;
            tex_desc->type = WGPUBindingType_SampledTexture;
            tex_desc->textureDimension = _sg_wgpu_tex_viewdim(cmn_stage->images[img_index].image_type);
            tex_desc->textureComponentType = _sg_wgpu_tex_comptype(cmn_stage->images[img_index].sampler_type);

            smp_desc->binding = img_index + _SG_WGPU_MAX_SHADERSTAGE_IMAGES;
            smp_desc->visibility = vis;
            smp_desc->type = WGPUBindingType_Sampler;
        }
        WGPUBindGroupLayoutDescriptor img_bgl_desc;
        _sg_clear(&img_bgl_desc, sizeof(img_bgl_desc));
        img_bgl_desc.bindingCount = num_imgs * 2;
        img_bgl_desc.bindings = &bglb_desc[0];
        wgpu_stage->bind_group_layout = wgpuDeviceCreateBindGroupLayout(_sg.wgpu.dev, &img_bgl_desc);
        SOKOL_ASSERT(wgpu_stage->bind_group_layout);
    }
    return success ? SG_RESOURCESTATE_VALID : SG_RESOURCESTATE_FAILED;
}

_SOKOL_PRIVATE void _sg_wgpu_discard_shader(_sg_shader_t* shd) {
    SOKOL_ASSERT(shd);
    for (int stage_index = 0; stage_index < SG_NUM_SHADER_STAGES; stage_index++) {
        _sg_wgpu_shader_stage_t* wgpu_stage = &shd->wgpu.stage[stage_index];
        if (wgpu_stage->module) {
            wgpuShaderModuleRelease(wgpu_stage->module);
            wgpu_stage->module = 0;
        }
        if (wgpu_stage->bind_group_layout) {
            wgpuBindGroupLayoutRelease(wgpu_stage->bind_group_layout);
            wgpu_stage->bind_group_layout = 0;
        }
    }
}

_SOKOL_PRIVATE sg_resource_state _sg_wgpu_create_pipeline(_sg_pipeline_t* pip, _sg_shader_t* shd, const sg_pipeline_desc* desc) {
    SOKOL_ASSERT(pip && shd && desc);
    SOKOL_ASSERT(desc->shader.id == shd->slot.id);
    SOKOL_ASSERT(shd->wgpu.stage[SG_SHADERSTAGE_VS].bind_group_layout);
    SOKOL_ASSERT(shd->wgpu.stage[SG_SHADERSTAGE_FS].bind_group_layout);
    pip->shader = shd;
    _sg_pipeline_common_init(&pip->cmn, desc);
    pip->wgpu.stencil_ref = (uint32_t) desc->stencil.ref;

    WGPUBindGroupLayout pip_bgl[3] = {
        _sg.wgpu.ub.bindgroup_layout,
        shd->wgpu.stage[SG_SHADERSTAGE_VS].bind_group_layout,
        shd->wgpu.stage[SG_SHADERSTAGE_FS].bind_group_layout
    };
    WGPUPipelineLayoutDescriptor pl_desc;
    _sg_clear(&pl_desc, sizeof(pl_desc));
    pl_desc.bindGroupLayoutCount = 3;
    pl_desc.bindGroupLayouts = &pip_bgl[0];
    WGPUPipelineLayout pip_layout = wgpuDeviceCreatePipelineLayout(_sg.wgpu.dev, &pl_desc);

    WGPUVertexBufferLayoutDescriptor vb_desc[SG_MAX_SHADERSTAGE_BUFFERS];
    _sg_clear(&vb_desc, sizeof(vb_desc));
    WGPUVertexAttributeDescriptor va_desc[SG_MAX_SHADERSTAGE_BUFFERS][SG_MAX_VERTEX_ATTRIBUTES];
    _sg_clear(&va_desc, sizeof(va_desc));
    int vb_idx = 0;
    for (; vb_idx < SG_MAX_SHADERSTAGE_BUFFERS; vb_idx++) {
        const sg_buffer_layout_desc* src_vb_desc = &desc->layout.buffers[vb_idx];
        if (0 == src_vb_desc->stride) {
            break;
        }
        vb_desc[vb_idx].arrayStride = src_vb_desc->stride;
        vb_desc[vb_idx].stepMode = _sg_wgpu_stepmode(src_vb_desc->step_func);
        /* NOTE: WebGPU has no support for vertex step rate (because that's
           not supported by Core Vulkan
        */
        int va_idx = 0;
        for (int va_loc = 0; va_loc < SG_MAX_VERTEX_ATTRIBUTES; va_loc++) {
            const sg_vertex_attr_desc* src_va_desc = &desc->layout.attrs[va_loc];
            if (SG_VERTEXFORMAT_INVALID == src_va_desc->format) {
                break;
            }
            pip->cmn.vertex_layout_valid[src_va_desc->buffer_index] = true;
            if (vb_idx == src_va_desc->buffer_index) {
                va_desc[vb_idx][va_idx].format = _sg_wgpu_vertexformat(src_va_desc->format);
                va_desc[vb_idx][va_idx].offset = src_va_desc->offset;
                va_desc[vb_idx][va_idx].shaderLocation = va_loc;
                va_idx++;
            }
        }
        vb_desc[vb_idx].attributeCount = va_idx;
        vb_desc[vb_idx].attributes = &va_desc[vb_idx][0];
    }
    WGPUVertexStateDescriptor vx_state_desc;
    _sg_clear(&vx_state_desc, sizeof(vx_state_desc));
    vx_state_desc.indexFormat = _sg_wgpu_indexformat(desc->index_type);
    vx_state_desc.vertexBufferCount = vb_idx;
    vx_state_desc.vertexBuffers = vb_desc;

    WGPURasterizationStateDescriptor rs_desc;
    _sg_clear(&rs_desc, sizeof(rs_desc));
    rs_desc.frontFace = _sg_wgpu_frontface(desc->face_winding);
    rs_desc.cullMode = _sg_wgpu_cullmode(desc->cull_mode);
    rs_desc.depthBias = (int32_t) desc->depth.bias;
    rs_desc.depthBiasClamp = desc->depth.bias_clamp;
    rs_desc.depthBiasSlopeScale = desc->depth.bias_slope_scale;

    WGPUDepthStencilStateDescriptor ds_desc;
    _sg_clear(&ds_desc, sizeof(ds_desc));
    ds_desc.format = _sg_wgpu_textureformat(desc->depth.pixel_format);
    ds_desc.depthWriteEnabled = desc->depth.write_enabled;
    ds_desc.depthCompare = _sg_wgpu_comparefunc(desc->depth.compare);
    ds_desc.stencilReadMask = desc->stencil.read_mask;
    ds_desc.stencilWriteMask = desc->stencil.write_mask;
    ds_desc.stencilFront.compare = _sg_wgpu_comparefunc(desc->stencil.front.compare);
    ds_desc.stencilFront.failOp = _sg_wgpu_stencilop(desc->stencil.front.fail_op);
    ds_desc.stencilFront.depthFailOp = _sg_wgpu_stencilop(desc->stencil.front.depth_fail_op);
    ds_desc.stencilFront.passOp = _sg_wgpu_stencilop(desc->stencil.front.pass_op);
    ds_desc.stencilBack.compare = _sg_wgpu_comparefunc(desc->stencil.back.compare);
    ds_desc.stencilBack.failOp = _sg_wgpu_stencilop(desc->stencil.back.fail_op);
    ds_desc.stencilBack.depthFailOp = _sg_wgpu_stencilop(desc->stencil.back.depth_fail_op);
    ds_desc.stencilBack.passOp = _sg_wgpu_stencilop(desc->stencil.back.pass_op);

    WGPUProgrammableStageDescriptor fs_desc;
    _sg_clear(&fs_desc, sizeof(fs_desc));
    fs_desc.module = shd->wgpu.stage[SG_SHADERSTAGE_FS].module;
    fs_desc.entryPoint = shd->wgpu.stage[SG_SHADERSTAGE_VS].entry.buf;

    WGPUColorStateDescriptor cs_desc[SG_MAX_COLOR_ATTACHMENTS];
    _sg_clear(cs_desc, sizeof(cs_desc));
    for (uint32_t i = 0; i < desc->color_count; i++) {
        SOKOL_ASSERT(i < SG_MAX_COLOR_ATTACHMENTS);
        cs_desc[i].format = _sg_wgpu_textureformat(desc->colors[i].pixel_format);
        cs_desc[i].colorBlend.operation = _sg_wgpu_blendop(desc->colors[i].blend.op_rgb);
        cs_desc[i].colorBlend.srcFactor = _sg_wgpu_blendfactor(desc->colors[i].blend.src_factor_rgb);
        cs_desc[i].colorBlend.dstFactor = _sg_wgpu_blendfactor(desc->colors[i].blend.dst_factor_rgb);
        cs_desc[i].alphaBlend.operation = _sg_wgpu_blendop(desc->colors[i].blend.op_alpha);
        cs_desc[i].alphaBlend.srcFactor = _sg_wgpu_blendfactor(desc->colors[i].blend.src_factor_alpha);
        cs_desc[i].alphaBlend.dstFactor = _sg_wgpu_blendfactor(desc->colors[i].blend.dst_factor_alpha);
        cs_desc[i].writeMask = _sg_wgpu_colorwritemask(desc->colors[i].write_mask);
    }

    WGPURenderPipelineDescriptor pip_desc;
    _sg_clear(&pip_desc, sizeof(pip_desc));
    pip_desc.layout = pip_layout;
    pip_desc.vertexStage.module = shd->wgpu.stage[SG_SHADERSTAGE_VS].module;
    pip_desc.vertexStage.entryPoint = shd->wgpu.stage[SG_SHADERSTAGE_VS].entry.buf;
    pip_desc.fragmentStage = &fs_desc;
    pip_desc.vertexState = &vx_state_desc;
    pip_desc.primitiveTopology  = _sg_wgpu_topology(desc->primitive_type);
    pip_desc.rasterizationState = &rs_desc;
    pip_desc.sampleCount = desc->sample_count;
    if (SG_PIXELFORMAT_NONE != desc->depth.pixel_format) {
        pip_desc.depthStencilState = &ds_desc;
    }
    pip_desc.colorStateCount = desc->color_count;
    pip_desc.colorStates = cs_desc;
    pip_desc.sampleMask = 0xFFFFFFFF;   /* FIXME: ??? */
    pip->wgpu.pip = wgpuDeviceCreateRenderPipeline(_sg.wgpu.dev, &pip_desc);
    SOKOL_ASSERT(0 != pip->wgpu.pip);
    wgpuPipelineLayoutRelease(pip_layout);

    return SG_RESOURCESTATE_VALID;
}

_SOKOL_PRIVATE void _sg_wgpu_discard_pipeline(_sg_pipeline_t* pip) {
    SOKOL_ASSERT(pip);
    if (pip == _sg.wgpu.cur_pipeline) {
        _sg.wgpu.cur_pipeline = 0;
        _Sg.wgpu.cur_pipeline_id.id = SG_INVALID_ID;
    }
    if (pip->wgpu.pip) {
        wgpuRenderPipelineRelease(pip->wgpu.pip);
        pip->wgpu.pip = 0;
    }
}

_SOKOL_PRIVATE sg_resource_state _sg_wgpu_create_pass(_sg_pass_t* pass, _sg_image_t** att_images, const sg_pass_desc* desc) {
    SOKOL_ASSERT(pass && desc);
    SOKOL_ASSERT(att_images && att_images[0]);
    _sg_pass_common_init(&pass->cmn, desc);

    /* copy image pointers and create render-texture views */
    const sg_pass_attachment_desc* att_desc;
    for (uint32_t i = 0; i < pass->cmn.num_color_atts; i++) {
        att_desc = &desc->color_attachments[i];
        if (att_desc->image.id != SG_INVALID_ID) {
            SOKOL_ASSERT(att_desc->image.id != SG_INVALID_ID);
            SOKOL_ASSERT(0 == pass->wgpu.color_atts[i].image);
            _sg_image_t* img = att_images[i];
            SOKOL_ASSERT(img && (img->slot.id == att_desc->image.id));
            SOKOL_ASSERT(_sg_is_valid_rendertarget_color_format(img->cmn.pixel_format));
            pass->wgpu.color_atts[i].image = img;
            /* create a render-texture-view to render into the right sub-surface */
            const bool is_msaa = img->cmn.sample_count > 1;
            WGPUTextureViewDescriptor view_desc;
            _sg_clear(&view_desc, sizeof(view_desc));
            view_desc.baseMipLevel = is_msaa ? 0 : att_desc->mip_level;
            view_desc.mipLevelCount = 1;
            view_desc.baseArrayLayer = is_msaa ? 0 : att_desc->slice;
            view_desc.arrayLayerCount = 1;
            WGPUTexture wgpu_tex = is_msaa ? img->wgpu.msaa_tex : img->wgpu.tex;
            SOKOL_ASSERT(wgpu_tex);
            pass->wgpu.color_atts[i].render_tex_view = wgpuTextureCreateView(wgpu_tex, &view_desc);
            SOKOL_ASSERT(pass->wgpu.color_atts[i].render_tex_view);
            /* ... and if needed a separate resolve texture view */
            if (is_msaa) {
                view_desc.baseMipLevel = att_desc->mip_level;
                view_desc.baseArrayLayer = att_desc->slice;
                WGPUTexture wgpu_tex = img->wgpu.tex;
                pass->wgpu.color_atts[i].resolve_tex_view = wgpuTextureCreateView(wgpu_tex, &view_desc);
                SOKOL_ASSERT(pass->wgpu.color_atts[i].resolve_tex_view);
            }
        }
    }
    SOKOL_ASSERT(0 == pass->wgpu.ds_att.image);
    att_desc = &desc->depth_stencil_attachment;
    if (att_desc->image.id != SG_INVALID_ID) {
        const int ds_img_index = SG_MAX_COLOR_ATTACHMENTS;
        SOKOL_ASSERT(att_images[ds_img_index] && (att_images[ds_img_index]->slot.id == att_desc->image.id));
        SOKOL_ASSERT(_sg_is_valid_rendertarget_depth_format(att_images[ds_img_index]->cmn.pixel_format));
        _sg_image_t* ds_img = att_images[ds_img_index];
        pass->wgpu.ds_att.image = ds_img;
        /* create a render-texture view */
        SOKOL_ASSERT(0 == att_desc->mip_level);
        SOKOL_ASSERT(0 == att_desc->slice);
        WGPUTextureViewDescriptor view_desc;
        _sg_clear(&view_desc, sizeof(view_desc));
        WGPUTexture wgpu_tex = ds_img->wgpu.tex;
        SOKOL_ASSERT(wgpu_tex);
        pass->wgpu.ds_att.render_tex_view = wgpuTextureCreateView(wgpu_tex, &view_desc);
        SOKOL_ASSERT(pass->wgpu.ds_att.render_tex_view);
    }
    return SG_RESOURCESTATE_VALID;
}

_SOKOL_PRIVATE void _sg_wgpu_discard_pass(_sg_pass_t* pass) {
    SOKOL_ASSERT(pass);
    for (uint32_t i = 0; i < pass->cmn.num_color_atts; i++) {
        if (pass->wgpu.color_atts[i].render_tex_view) {
            wgpuTextureViewRelease(pass->wgpu.color_atts[i].render_tex_view);
            pass->wgpu.color_atts[i].render_tex_view = 0;
        }
        if (pass->wgpu.color_atts[i].resolve_tex_view) {
            wgpuTextureViewRelease(pass->wgpu.color_atts[i].resolve_tex_view);
            pass->wgpu.color_atts[i].resolve_tex_view = 0;
        }
    }
    if (pass->wgpu.ds_att.render_tex_view) {
        wgpuTextureViewRelease(pass->wgpu.ds_att.render_tex_view);
        pass->wgpu.ds_att.render_tex_view = 0;
    }
}

_SOKOL_PRIVATE _sg_image_t* _sg_wgpu_pass_color_image(const _sg_pass_t* pass, int index) {
    SOKOL_ASSERT(pass && (index >= 0) && (index < SG_MAX_COLOR_ATTACHMENTS));
    /* NOTE: may return null */
    return pass->wgpu.color_atts[index].image;
}

_SOKOL_PRIVATE _sg_image_t* _sg_wgpu_pass_ds_image(const _sg_pass_t* pass) {
    /* NOTE: may return null */
    SOKOL_ASSERT(pass);
    return pass->wgpu.ds_att.image;
}

_SOKOL_PRIVATE void _sg_wgpu_begin_pass(_sg_pass_t* pass, const sg_pass_action* action, int w, int h) {
    SOKOL_ASSERT(action);
    SOKOL_ASSERT(!_sg.wgpu.in_pass);
    SOKOL_ASSERT(_sg.wgpu.render_cmd_enc);
    SOKOL_ASSERT(_sg.wgpu.dev);
    SOKOL_ASSERT(_sg.wgpu.render_view_cb || _sg.wgpu.render_view_userdata_cb);
    SOKOL_ASSERT(_sg.wgpu.resolve_view_cb || _sg.wgpu.resolve_view_userdata_cb);
    SOKOL_ASSERT(_sg.wgpu.depth_stencil_view_cb || _sg.wgpu.depth_stencil_view_userdata_cb);
    _sg.wgpu.in_pass = true;
    _sg.wgpu.cur_width = w;
    _sg.wgpu.cur_height = h;
    _sg.wgpu.cur_pipeline = 0;
    _sg.wgpu.cur_pipeline_id.id = SG_INVALID_ID;

    SOKOL_ASSERT(_sg.wgpu.render_cmd_enc);
    if (pass) {
        WGPURenderPassDescriptor wgpu_pass_desc;
        _sg_clear(&wgpu_pass_desc, sizeof(wgpu_pass_desc));
        WGPURenderPassColorAttachmentDescriptor wgpu_color_att_desc[SG_MAX_COLOR_ATTACHMENTS];
        _sg_clear(&wgpu_color_att_desc, sizeof(wgpu_color_att_desc));
        SOKOL_ASSERT(pass->slot.state == SG_RESOURCESTATE_VALID);
        for (uint32_t i = 0; i < pass->cmn.num_color_atts; i++) {
            const _sg_wgpu_attachment_t* wgpu_att = &pass->wgpu.color_atts[i];
            wgpu_color_att_desc[i].loadOp = _sg_wgpu_load_op(action->colors[i].action);
            wgpu_color_att_desc[i].storeOp = WGPUStoreOp_Store;
            wgpu_color_att_desc[i].clearColor.r = action->colors[i].value.r;
            wgpu_color_att_desc[i].clearColor.g = action->colors[i].value.g;
            wgpu_color_att_desc[i].clearColor.b = action->colors[i].value.b;
            wgpu_color_att_desc[i].clearColor.a = action->colors[i].value.a;
            wgpu_color_att_desc[i].attachment = wgpu_att->render_tex_view;
            if (wgpu_att->image->cmn.sample_count > 1) {
                wgpu_color_att_desc[i].resolveTarget = wgpu_att->resolve_tex_view;
            }
        }
        wgpu_pass_desc.colorAttachmentCount = pass->cmn.num_color_atts;
        wgpu_pass_desc.colorAttachments = &wgpu_color_att_desc[0];
        if (pass->wgpu.ds_att.image) {
            WGPURenderPassDepthStencilAttachmentDescriptor wgpu_ds_att_desc;
            _sg_clear(&wgpu_ds_att_desc, sizeof(wgpu_ds_att_desc));
            wgpu_ds_att_desc.depthLoadOp = _sg_wgpu_load_op(action->depth.action);
            wgpu_ds_att_desc.clearDepth = action->depth.value;
            wgpu_ds_att_desc.stencilLoadOp = _sg_wgpu_load_op(action->stencil.action);
            wgpu_ds_att_desc.clearStencil = action->stencil.value;
            wgpu_ds_att_desc.attachment = pass->wgpu.ds_att.render_tex_view;
            wgpu_pass_desc.depthStencilAttachment = &wgpu_ds_att_desc;
            _sg.wgpu.pass_enc = wgpuCommandEncoderBeginRenderPass(_sg.wgpu.render_cmd_enc, &wgpu_pass_desc);
        }
    }
    else {
        /* default render pass */
        WGPUTextureView wgpu_render_view = _sg.wgpu.render_view_cb ? _sg.wgpu.render_view_cb() : _sg.wgpu.render_view_userdata_cb(_sg.wgpu.user_data);
        WGPUTextureView wgpu_resolve_view = _sg.wgpu.resolve_view_cb ? _sg.wgpu.resolve_view_cb() : _sg.wgpu.resolve_view_userdata_cb(_sg.wgpu.user_data);
        WGPUTextureView wgpu_depth_stencil_view = _sg.wgpu.depth_stencil_view_cb ? _sg.wgpu.depth_stencil_view_cb() : _sg.wgpu.depth_stencil_view_userdata_cb(_sg.wgpu.user_data);

        WGPURenderPassDescriptor pass_desc;
        _sg_clear(&pass_desc, sizeof(pass_desc));
        WGPURenderPassColorAttachmentDescriptor color_att_desc;
        _sg_clear(&color_att_desc, sizeof(color_att_desc));
        color_att_desc.loadOp = _sg_wgpu_load_op(action->colors[0].action);
        color_att_desc.clearColor.r = action->colors[0].value.r;
        color_att_desc.clearColor.g = action->colors[0].value.g;
        color_att_desc.clearColor.b = action->colors[0].value.b;
        color_att_desc.clearColor.a = action->colors[0].value.a;
        color_att_desc.attachment = wgpu_render_view;
        color_att_desc.resolveTarget = wgpu_resolve_view;   /* null if no MSAA rendering */
        pass_desc.colorAttachmentCount = 1;
        pass_desc.colorAttachments = &color_att_desc;
        WGPURenderPassDepthStencilAttachmentDescriptor ds_att_desc;
        _sg_clear(&ds_att_desc, sizeof(ds_att_desc));
        ds_att_desc.attachment = wgpu_depth_stencil_view;
        SOKOL_ASSERT(0 != ds_att_desc.attachment);
        ds_att_desc.depthLoadOp = _sg_wgpu_load_op(action->depth.action);
        ds_att_desc.clearDepth = action->depth.value;
        ds_att_desc.stencilLoadOp = _sg_wgpu_load_op(action->stencil.action);
        ds_att_desc.clearStencil = action->stencil.value;
        pass_desc.depthStencilAttachment = &ds_att_desc;
        _sg.wgpu.pass_enc = wgpuCommandEncoderBeginRenderPass(_sg.wgpu.render_cmd_enc, &pass_desc);
    }
    SOKOL_ASSERT(_sg.wgpu.pass_enc);

    /* initial uniform buffer binding (required even if no uniforms are set in the frame) */
    wgpuRenderPassEncoderSetBindGroup(_sg.wgpu.pass_enc,
                                      0, /* groupIndex 0 is reserved for uniform buffers */
                                      _sg.wgpu.ub.bindgroup,
                                      SG_NUM_SHADER_STAGES * SG_MAX_SHADERSTAGE_UBS,
                                      &_sg.wgpu.ub.bind_offsets[0][0]);
}

_SOKOL_PRIVATE void _sg_wgpu_end_pass(void) {
    SOKOL_ASSERT(_sg.wgpu.in_pass);
    SOKOL_ASSERT(_sg.wgpu.pass_enc);
    _sg.wgpu.in_pass = false;
    wgpuRenderPassEncoderEndPass(_sg.wgpu.pass_enc);
    wgpuRenderPassEncoderRelease(_sg.wgpu.pass_enc);
    _sg.wgpu.pass_enc = 0;
}

_SOKOL_PRIVATE void _sg_wgpu_commit(void) {
    SOKOL_ASSERT(!_sg.wgpu.in_pass);
    SOKOL_ASSERT(_sg.wgpu.queue);
    SOKOL_ASSERT(_sg.wgpu.render_cmd_enc);
    SOKOL_ASSERT(_sg.wgpu.staging_cmd_enc);

    /* finish and submit this frame's work */
    _sg_wgpu_ubpool_flush();
    _sg_wgpu_staging_unmap();

    WGPUCommandBuffer cmd_bufs[2];

    WGPUCommandBufferDescriptor cmd_buf_desc;
    _sg_clear(&cmd_buf_desc, sizeof(cmd_buf_desc));
    cmd_bufs[0] = wgpuCommandEncoderFinish(_sg.wgpu.staging_cmd_enc, &cmd_buf_desc);
    SOKOL_ASSERT(cmd_bufs[0]);
    wgpuCommandEncoderRelease(_sg.wgpu.staging_cmd_enc);
    _sg.wgpu.staging_cmd_enc = 0;

    cmd_bufs[1] = wgpuCommandEncoderFinish(_sg.wgpu.render_cmd_enc, &cmd_buf_desc);
    SOKOL_ASSERT(cmd_bufs[1]);
    wgpuCommandEncoderRelease(_sg.wgpu.render_cmd_enc);
    _sg.wgpu.render_cmd_enc = 0;

    wgpuQueueSubmit(_sg.wgpu.queue, 2, &cmd_bufs[0]);

    wgpuCommandBufferRelease(cmd_bufs[0]);
    wgpuCommandBufferRelease(cmd_bufs[1]);

    /* create a new render- and staging-command-encoders for next frame */
    WGPUCommandEncoderDescriptor cmd_enc_desc;
    _sg_clear(&cmd_enc_desc, sizeof(cmd_enc_desc));
    _sg.wgpu.staging_cmd_enc = wgpuDeviceCreateCommandEncoder(_sg.wgpu.dev, &cmd_enc_desc);
    _sg.wgpu.render_cmd_enc = wgpuDeviceCreateCommandEncoder(_sg.wgpu.dev, &cmd_enc_desc);

    /* grab new staging buffers for uniform- and vertex/image-updates */
    _sg_wgpu_ubpool_next_frame(false);
    _sg_wgpu_staging_next_frame(false);
}

_SOKOL_PRIVATE void _sg_wgpu_apply_viewport(int x, int y, int w, int h, bool origin_top_left) {
    SOKOL_ASSERT(_sg.wgpu.in_pass);
    SOKOL_ASSERT(_sg.wgpu.pass_enc);
    float xf = (float) x;
    float yf = (float) (origin_top_left ? y : (_sg.wgpu.cur_height - (y + h)));
    float wf = (float) w;
    float hf = (float) h;
    wgpuRenderPassEncoderSetViewport(_sg.wgpu.pass_enc, xf, yf, wf, hf, 0.0f, 1.0f);
}

_SOKOL_PRIVATE void _sg_wgpu_apply_scissor_rect(int x, int y, int w, int h, bool origin_top_left) {
    SOKOL_ASSERT(_sg.wgpu.in_pass);
    SOKOL_ASSERT(_sg.wgpu.pass_enc);
    SOKOL_ASSERT(_sg.wgpu.in_pass);
    SOKOL_ASSERT(_sg.wgpu.pass_enc);

    /* clip against framebuffer rect */
    x = _sg_min(_sg_max(0, x), _sg.wgpu.cur_width-1);
    y = _sg_min(_sg_max(0, y), _sg.wgpu.cur_height-1);
    if ((x + w) > _sg.wgpu.cur_width) {
        w = _sg.wgpu.cur_width - x;
    }
    if ((y + h) > _sg.wgpu.cur_height) {
        h = _sg.wgpu.cur_height - y;
    }
    w = _sg_max(w, 1);
    h = _sg_max(h, 1);

    uint32_t sx = (uint32_t) x;
    uint32_t sy = origin_top_left ? y : (_sg.wgpu.cur_height - (y + h));
    uint32_t sw = w;
    uint32_t sh = h;
    wgpuRenderPassEncoderSetScissorRect(_sg.wgpu.pass_enc, sx, sy, sw, sh);
}

_SOKOL_PRIVATE void _sg_wgpu_apply_pipeline(_sg_pipeline_t* pip) {
    SOKOL_ASSERT(pip);
    SOKOL_ASSERT(pip->wgpu.pip);
    SOKOL_ASSERT(_sg.wgpu.in_pass);
    SOKOL_ASSERT(_sg.wgpu.pass_enc);
    _sg.wgpu.draw_indexed = (pip->cmn.index_type != SG_INDEXTYPE_NONE);
    _sg.wgpu.cur_pipeline = pip;
    _sg.wgpu.cur_pipeline_id.id = pip->slot.id;
    wgpuRenderPassEncoderSetPipeline(_sg.wgpu.pass_enc, pip->wgpu.pip);
    wgpuRenderPassEncoderSetBlendColor(_sg.wgpu.pass_enc, (WGPUColor*)&pip->cmn.blend_color);
    wgpuRenderPassEncoderSetStencilReference(_sg.wgpu.pass_enc, pip->wgpu.stencil_ref);
}

_SOKOL_PRIVATE WGPUBindGroup _sg_wgpu_create_images_bindgroup(WGPUBindGroupLayout bgl, _sg_image_t** imgs, int num_imgs) {
    SOKOL_ASSERT(_sg.wgpu.dev);
    SOKOL_ASSERT(num_imgs <= _SG_WGPU_MAX_SHADERSTAGE_IMAGES);
    WGPUBindGroupBinding img_bgb[_SG_WGPU_MAX_SHADERSTAGE_IMAGES * 2];
    _sg_clear(&img_bgb, sizeof(img_bgb));
    for (int img_index = 0; img_index < num_imgs; img_index++) {
        WGPUBindGroupBinding* tex_bdg = &img_bgb[img_index*2 + 0];
        WGPUBindGroupBinding* smp_bdg = &img_bgb[img_index*2 + 1];
        tex_bdg->binding = img_index;
        tex_bdg->textureView = imgs[img_index]->wgpu.tex_view;
        smp_bdg->binding = img_index + _SG_WGPU_MAX_SHADERSTAGE_IMAGES;
        smp_bdg->sampler = imgs[img_index]->wgpu.sampler;
    }
    WGPUBindGroupDescriptor bg_desc;
    _sg_clear(&bg_desc, sizeof(bg_desc));
    bg_desc.layout = bgl;
    bg_desc.bindingCount = 2 * num_imgs;
    bg_desc.bindings = &img_bgb[0];
    WGPUBindGroup bg = wgpuDeviceCreateBindGroup(_sg.wgpu.dev, &bg_desc);
    SOKOL_ASSERT(bg);
    return bg;
}

_SOKOL_PRIVATE void _sg_wgpu_apply_bindings(
    _sg_pipeline_t* pip,
    _sg_buffer_t** vbs, const int* vb_offsets, int num_vbs,
    _sg_buffer_t* ib, int ib_offset,
    _sg_image_t** vs_imgs, int num_vs_imgs,
    _sg_image_t** fs_imgs, int num_fs_imgs)
{
    SOKOL_ASSERT(_sg.wgpu.in_pass);
    SOKOL_ASSERT(_sg.wgpu.pass_enc);
    SOKOL_ASSERT(pip->shader && (pip->cmn.shader_id.id == pip->shader->slot.id));

    /* index buffer */
    if (ib) {
        wgpuRenderPassEncoderSetIndexBuffer(_sg.wgpu.pass_enc, ib->wgpu.buf, ib_offset);
    }

    /* vertex buffers */
    for (uint32_t slot = 0; slot < (uint32_t)num_vbs; slot++) {
        wgpuRenderPassEncoderSetVertexBuffer(_sg.wgpu.pass_enc, slot, vbs[slot]->wgpu.buf, (uint64_t)vb_offsets[slot]);
    }

    /* need to create throw-away bind groups for images */
    if (num_vs_imgs > 0) {
        if (num_vs_imgs > _SG_WGPU_MAX_SHADERSTAGE_IMAGES) {
            num_vs_imgs = _SG_WGPU_MAX_SHADERSTAGE_IMAGES;
        }
        WGPUBindGroupLayout vs_bgl = pip->shader->wgpu.stage[SG_SHADERSTAGE_VS].bind_group_layout;
        SOKOL_ASSERT(vs_bgl);
        WGPUBindGroup vs_img_bg = _sg_wgpu_create_images_bindgroup(vs_bgl, vs_imgs, num_vs_imgs);
        wgpuRenderPassEncoderSetBindGroup(_sg.wgpu.pass_enc, 1, vs_img_bg, 0, 0);
        wgpuBindGroupRelease(vs_img_bg);
    }
    else {
        wgpuRenderPassEncoderSetBindGroup(_sg.wgpu.pass_enc, 1, _sg.wgpu.empty_bind_group, 0, 0);
    }
    if (num_fs_imgs > 0) {
        if (num_fs_imgs > _SG_WGPU_MAX_SHADERSTAGE_IMAGES) {
            num_fs_imgs = _SG_WGPU_MAX_SHADERSTAGE_IMAGES;
        }
        WGPUBindGroupLayout fs_bgl = pip->shader->wgpu.stage[SG_SHADERSTAGE_FS].bind_group_layout;
        SOKOL_ASSERT(fs_bgl);
        WGPUBindGroup fs_img_bg = _sg_wgpu_create_images_bindgroup(fs_bgl, fs_imgs, num_fs_imgs);
        wgpuRenderPassEncoderSetBindGroup(_sg.wgpu.pass_enc, 2, fs_img_bg, 0, 0);
        wgpuBindGroupRelease(fs_img_bg);
    }
    else {
        wgpuRenderPassEncoderSetBindGroup(_sg.wgpu.pass_enc, 2, _sg.wgpu.empty_bind_group, 0, 0);
    }
}

_SOKOL_PRIVATE void _sg_wgpu_apply_uniforms(sg_shader_stage stage_index, int ub_index, const sg_range* data) {
    SOKOL_ASSERT(_sg.wgpu.in_pass);
    SOKOL_ASSERT(_sg.wgpu.pass_enc);
    SOKOL_ASSERT((_sg.wgpu.ub.offset + data->size) <= _sg.wgpu.ub.num_bytes);
    SOKOL_ASSERT((_sg.wgpu.ub.offset & (_SG_WGPU_STAGING_ALIGN-1)) == 0);
    SOKOL_ASSERT(_sg.wgpu.cur_pipeline && _sg.wgpu.cur_pipeline->shader);
    SOKOL_ASSERT(_sg.wgpu.cur_pipeline->slot.id == _sg.wgpu.cur_pipeline_id.id);
    SOKOL_ASSERT(_sg.wgpu.cur_pipeline->shader->slot.id == _sg.wgpu.cur_pipeline->cmn.shader_id.id);
    SOKOL_ASSERT(ub_index < _sg.wgpu.cur_pipeline->shader->cmn.stage[stage_index].num_uniform_blocks);
    SOKOL_ASSERT(data->size <= _sg.wgpu.cur_pipeline->shader->cmn.stage[stage_index].uniform_blocks[ub_index].size);
    SOKOL_ASSERT(data->size <= _SG_WGPU_MAX_UNIFORM_UPDATE_SIZE);
    SOKOL_ASSERT(0 != _sg.wgpu.ub.stage.ptr[_sg.wgpu.ub.stage.cur]);

    uint8_t* dst_ptr = _sg.wgpu.ub.stage.ptr[_sg.wgpu.ub.stage.cur] + _sg.wgpu.ub.offset;
    memcpy(dst_ptr, data->ptr, data->size);
    _sg.wgpu.ub.bind_offsets[stage_index][ub_index] = _sg.wgpu.ub.offset;
    wgpuRenderPassEncoderSetBindGroup(_sg.wgpu.pass_enc,
                                      0, /* groupIndex 0 is reserved for uniform buffers */
                                      _sg.wgpu.ub.bindgroup,
                                      SG_NUM_SHADER_STAGES * SG_MAX_SHADERSTAGE_UBS,
                                      &_sg.wgpu.ub.bind_offsets[0][0]);
    _sg.wgpu.ub.offset = _sg_roundup(_sg.wgpu.ub.offset + data->size, _SG_WGPU_STAGING_ALIGN);
}

_SOKOL_PRIVATE void _sg_wgpu_draw(int base_element, int num_elements, int num_instances) {
    SOKOL_ASSERT(_sg.wgpu.in_pass);
    SOKOL_ASSERT(_sg.wgpu.pass_enc);
    if (_sg.wgpu.draw_indexed) {
        wgpuRenderPassEncoderDrawIndexed(_sg.wgpu.pass_enc, num_elements, num_instances, base_element, 0, 0);
    }
    else {
        wgpuRenderPassEncoderDraw(_sg.wgpu.pass_enc, num_elements, num_instances, base_element, 0);
    }
}

_SOKOL_PRIVATE void _sg_wgpu_update_buffer(_sg_buffer_t* buf, const sg_range* data) {
    SOKOL_ASSERT(buf && data && data->ptr && (data->size > 0));
    uint32_t copied_num_bytes = _sg_wgpu_staging_copy_to_buffer(buf->wgpu.buf, 0, data->ptr, data->size);
    SOKOL_ASSERT(copied_num_bytes > 0); _SOKOL_UNUSED(copied_num_bytes);
}

_SOKOL_PRIVATE int _sg_wgpu_append_buffer(_sg_buffer_t* buf, const sg_range* data, bool new_frame) {
    SOKOL_ASSERT(buf && data && data->ptr && (data->size > 0));
    _SOKOL_UNUSED(new_frame);
    uint32_t copied_num_bytes = _sg_wgpu_staging_copy_to_buffer(buf->wgpu.buf, buf->cmn.append_pos, data->ptr, data->size);
    SOKOL_ASSERT(copied_num_bytes > 0); _SOKOL_UNUSED(copied_num_bytes);
    return (int)copied_num_bytes;
}

_SOKOL_PRIVATE void _sg_wgpu_update_image(_sg_image_t* img, const sg_image_data* data) {
    SOKOL_ASSERT(img && data);
    bool success = _sg_wgpu_staging_copy_to_texture(img, data);
    SOKOL_ASSERT(success);
    _SOKOL_UNUSED(success);
}
#endif

/*== BACKEND API WRAPPERS ====================================================*/
static inline void _sg_setup_backend(const sg_desc* desc) {
    #if defined(_SOKOL_ANY_GL)
    _sg_gl_setup_backend(desc);
    #elif defined(SOKOL_METAL)
    _sg_mtl_setup_backend(desc);
    #elif defined(SOKOL_D3D11)
    _sg_d3d11_setup_backend(desc);
    #elif defined(SOKOL_WGPU)
    _sg_wgpu_setup_backend(desc);
    #elif defined(SOKOL_DUMMY_BACKEND)
    _sg_dummy_setup_backend(desc);
    #else
    #error("INVALID BACKEND");
    #endif
}

static inline void _sg_discard_backend(void) {
    #if defined(_SOKOL_ANY_GL)
    _sg_gl_discard_backend();
    #elif defined(SOKOL_METAL)
    _sg_mtl_discard_backend();
    #elif defined(SOKOL_D3D11)
    _sg_d3d11_discard_backend();
    #elif defined(SOKOL_WGPU)
    _sg_wgpu_discard_backend();
    #elif defined(SOKOL_DUMMY_BACKEND)
    _sg_dummy_discard_backend();
    #else
    #error("INVALID BACKEND");
    #endif
}

static inline void _sg_reset_state_cache(void) {
    #if defined(_SOKOL_ANY_GL)
    _sg_gl_reset_state_cache();
    #elif defined(SOKOL_METAL)
    _sg_mtl_reset_state_cache();
    #elif defined(SOKOL_D3D11)
    _sg_d3d11_reset_state_cache();
    #elif defined(SOKOL_WGPU)
    _sg_wgpu_reset_state_cache();
    #elif defined(SOKOL_DUMMY_BACKEND)
    _sg_dummy_reset_state_cache();
    #else
    #error("INVALID BACKEND");
    #endif
}

static inline void _sg_activate_context(_sg_context_t* ctx) {
    #if defined(_SOKOL_ANY_GL)
    _sg_gl_activate_context(ctx);
    #elif defined(SOKOL_METAL)
    _sg_mtl_activate_context(ctx);
    #elif defined(SOKOL_D3D11)
    _sg_d3d11_activate_context(ctx);
    #elif defined(SOKOL_WGPU)
    _sg_wgpu_activate_context(ctx);
    #elif defined(SOKOL_DUMMY_BACKEND)
    _sg_dummy_activate_context(ctx);
    #else
    #error("INVALID BACKEND");
    #endif
}

static inline sg_resource_state _sg_create_context(_sg_context_t* ctx) {
    #if defined(_SOKOL_ANY_GL)
    return _sg_gl_create_context(ctx);
    #elif defined(SOKOL_METAL)
    return _sg_mtl_create_context(ctx);
    #elif defined(SOKOL_D3D11)
    return _sg_d3d11_create_context(ctx);
    #elif defined(SOKOL_WGPU)
    return _sg_wgpu_create_context(ctx);
    #elif defined(SOKOL_DUMMY_BACKEND)
    return _sg_dummy_create_context(ctx);
    #else
    #error("INVALID BACKEND");
    #endif
}

static inline void _sg_discard_context(_sg_context_t* ctx) {
    #if defined(_SOKOL_ANY_GL)
    _sg_gl_discard_context(ctx);
    #elif defined(SOKOL_METAL)
    _sg_mtl_discard_context(ctx);
    #elif defined(SOKOL_D3D11)
    _sg_d3d11_discard_context(ctx);
    #elif defined(SOKOL_WGPU)
    _sg_wgpu_discard_context(ctx);
    #elif defined(SOKOL_DUMMY_BACKEND)
    _sg_dummy_discard_context(ctx);
    #else
    #error("INVALID BACKEND");
    #endif
}

static inline sg_resource_state _sg_create_buffer(_sg_buffer_t* buf, const sg_buffer_desc* desc) {
    #if defined(_SOKOL_ANY_GL)
    return _sg_gl_create_buffer(buf, desc);
    #elif defined(SOKOL_METAL)
    return _sg_mtl_create_buffer(buf, desc);
    #elif defined(SOKOL_D3D11)
    return _sg_d3d11_create_buffer(buf, desc);
    #elif defined(SOKOL_WGPU)
    return _sg_wgpu_create_buffer(buf, desc);
    #elif defined(SOKOL_DUMMY_BACKEND)
    return _sg_dummy_create_buffer(buf, desc);
    #else
    #error("INVALID BACKEND");
    #endif
}

static inline void _sg_discard_buffer(_sg_buffer_t* buf) {
    #if defined(_SOKOL_ANY_GL)
    _sg_gl_discard_buffer(buf);
    #elif defined(SOKOL_METAL)
    _sg_mtl_discard_buffer(buf);
    #elif defined(SOKOL_D3D11)
    _sg_d3d11_discard_buffer(buf);
    #elif defined(SOKOL_WGPU)
    _sg_wgpu_discard_buffer(buf);
    #elif defined(SOKOL_DUMMY_BACKEND)
    _sg_dummy_discard_buffer(buf);
    #else
    #error("INVALID BACKEND");
    #endif
}

static inline sg_resource_state _sg_create_image(_sg_image_t* img, const sg_image_desc* desc) {
    #if defined(_SOKOL_ANY_GL)
    return _sg_gl_create_image(img, desc);
    #elif defined(SOKOL_METAL)
    return _sg_mtl_create_image(img, desc);
    #elif defined(SOKOL_D3D11)
    return _sg_d3d11_create_image(img, desc);
    #elif defined(SOKOL_WGPU)
    return _sg_wgpu_create_image(img, desc);
    #elif defined(SOKOL_DUMMY_BACKEND)
    return _sg_dummy_create_image(img, desc);
    #else
    #error("INVALID BACKEND");
    #endif
}

static inline void _sg_discard_image(_sg_image_t* img) {
    #if defined(_SOKOL_ANY_GL)
    _sg_gl_discard_image(img);
    #elif defined(SOKOL_METAL)
    _sg_mtl_discard_image(img);
    #elif defined(SOKOL_D3D11)
    _sg_d3d11_discard_image(img);
    #elif defined(SOKOL_WGPU)
    _sg_wgpu_discard_image(img);
    #elif defined(SOKOL_DUMMY_BACKEND)
    _sg_dummy_discard_image(img);
    #else
    #error("INVALID BACKEND");
    #endif
}

static inline sg_resource_state _sg_create_shader(_sg_shader_t* shd, const sg_shader_desc* desc) {
    #if defined(_SOKOL_ANY_GL)
    return _sg_gl_create_shader(shd, desc);
    #elif defined(SOKOL_METAL)
    return _sg_mtl_create_shader(shd, desc);
    #elif defined(SOKOL_D3D11)
    return _sg_d3d11_create_shader(shd, desc);
    #elif defined(SOKOL_WGPU)
    return _sg_wgpu_create_shader(shd, desc);
    #elif defined(SOKOL_DUMMY_BACKEND)
    return _sg_dummy_create_shader(shd, desc);
    #else
    #error("INVALID BACKEND");
    #endif
}

static inline void _sg_discard_shader(_sg_shader_t* shd) {
    #if defined(_SOKOL_ANY_GL)
    _sg_gl_discard_shader(shd);
    #elif defined(SOKOL_METAL)
    _sg_mtl_discard_shader(shd);
    #elif defined(SOKOL_D3D11)
    _sg_d3d11_discard_shader(shd);
    #elif defined(SOKOL_WGPU)
    _sg_wgpu_discard_shader(shd);
    #elif defined(SOKOL_DUMMY_BACKEND)
    _sg_dummy_discard_shader(shd);
    #else
    #error("INVALID BACKEND");
    #endif
}

static inline sg_resource_state _sg_create_pipeline(_sg_pipeline_t* pip, _sg_shader_t* shd, const sg_pipeline_desc* desc) {
    #if defined(_SOKOL_ANY_GL)
    return _sg_gl_create_pipeline(pip, shd, desc);
    #elif defined(SOKOL_METAL)
    return _sg_mtl_create_pipeline(pip, shd, desc);
    #elif defined(SOKOL_D3D11)
    return _sg_d3d11_create_pipeline(pip, shd, desc);
    #elif defined(SOKOL_WGPU)
    return _sg_wgpu_create_pipeline(pip, shd, desc);
    #elif defined(SOKOL_DUMMY_BACKEND)
    return _sg_dummy_create_pipeline(pip, shd, desc);
    #else
    #error("INVALID BACKEND");
    #endif
}

static inline void _sg_discard_pipeline(_sg_pipeline_t* pip) {
    #if defined(_SOKOL_ANY_GL)
    _sg_gl_discard_pipeline(pip);
    #elif defined(SOKOL_METAL)
    _sg_mtl_discard_pipeline(pip);
    #elif defined(SOKOL_D3D11)
    _sg_d3d11_discard_pipeline(pip);
    #elif defined(SOKOL_WGPU)
    _sg_wgpu_discard_pipeline(pip);
    #elif defined(SOKOL_DUMMY_BACKEND)
    _sg_dummy_discard_pipeline(pip);
    #else
    #error("INVALID BACKEND");
    #endif
}

static inline sg_resource_state _sg_create_pass(_sg_pass_t* pass, _sg_image_t** att_images, const sg_pass_desc* desc) {
    #if defined(_SOKOL_ANY_GL)
    return _sg_gl_create_pass(pass, att_images, desc);
    #elif defined(SOKOL_METAL)
    return _sg_mtl_create_pass(pass, att_images, desc);
    #elif defined(SOKOL_D3D11)
    return _sg_d3d11_create_pass(pass, att_images, desc);
    #elif defined(SOKOL_WGPU)
    return _sg_wgpu_create_pass(pass, att_images, desc);
    #elif defined(SOKOL_DUMMY_BACKEND)
    return _sg_dummy_create_pass(pass, att_images, desc);
    #else
    #error("INVALID BACKEND");
    #endif
}

static inline void _sg_discard_pass(_sg_pass_t* pass) {
    #if defined(_SOKOL_ANY_GL)
    _sg_gl_discard_pass(pass);
    #elif defined(SOKOL_METAL)
    _sg_mtl_discard_pass(pass);
    #elif defined(SOKOL_D3D11)
    _sg_d3d11_discard_pass(pass);
    #elif defined(SOKOL_WGPU)
    return _sg_wgpu_discard_pass(pass);
    #elif defined(SOKOL_DUMMY_BACKEND)
    _sg_dummy_discard_pass(pass);
    #else
    #error("INVALID BACKEND");
    #endif
}

static inline _sg_image_t* _sg_pass_color_image(const _sg_pass_t* pass, int index) {
    #if defined(_SOKOL_ANY_GL)
    return _sg_gl_pass_color_image(pass, index);
    #elif defined(SOKOL_METAL)
    return _sg_mtl_pass_color_image(pass, index);
    #elif defined(SOKOL_D3D11)
    return _sg_d3d11_pass_color_image(pass, index);
    #elif defined(SOKOL_WGPU)
    return _sg_wgpu_pass_color_image(pass, index);
    #elif defined(SOKOL_DUMMY_BACKEND)
    return _sg_dummy_pass_color_image(pass, index);
    #else
    #error("INVALID BACKEND");
    #endif
}

static inline _sg_image_t* _sg_pass_ds_image(const _sg_pass_t* pass) {
    #if defined(_SOKOL_ANY_GL)
    return _sg_gl_pass_ds_image(pass);
    #elif defined(SOKOL_METAL)
    return _sg_mtl_pass_ds_image(pass);
    #elif defined(SOKOL_D3D11)
    return _sg_d3d11_pass_ds_image(pass);
    #elif defined(SOKOL_WGPU)
    return _sg_wgpu_pass_ds_image(pass);
    #elif defined(SOKOL_DUMMY_BACKEND)
    return _sg_dummy_pass_ds_image(pass);
    #else
    #error("INVALID BACKEND");
    #endif
}

static inline void _sg_begin_pass(_sg_pass_t* pass, const sg_pass_action* action, int w, int h) {
    #if defined(_SOKOL_ANY_GL)
    _sg_gl_begin_pass(pass, action, w, h);
    #elif defined(SOKOL_METAL)
    _sg_mtl_begin_pass(pass, action, w, h);
    #elif defined(SOKOL_D3D11)
    _sg_d3d11_begin_pass(pass, action, w, h);
    #elif defined(SOKOL_WGPU)
    _sg_wgpu_begin_pass(pass, action, w, h);
    #elif defined(SOKOL_DUMMY_BACKEND)
    _sg_dummy_begin_pass(pass, action, w, h);
    #else
    #error("INVALID BACKEND");
    #endif
}

static inline void _sg_end_pass(void) {
    #if defined(_SOKOL_ANY_GL)
    _sg_gl_end_pass();
    #elif defined(SOKOL_METAL)
    _sg_mtl_end_pass();
    #elif defined(SOKOL_D3D11)
    _sg_d3d11_end_pass();
    #elif defined(SOKOL_WGPU)
    _sg_wgpu_end_pass();
    #elif defined(SOKOL_DUMMY_BACKEND)
    _sg_dummy_end_pass();
    #else
    #error("INVALID BACKEND");
    #endif
}

static inline void _sg_apply_viewport(int x, int y, int w, int h, bool origin_top_left) {
    #if defined(_SOKOL_ANY_GL)
    _sg_gl_apply_viewport(x, y, w, h, origin_top_left);
    #elif defined(SOKOL_METAL)
    _sg_mtl_apply_viewport(x, y, w, h, origin_top_left);
    #elif defined(SOKOL_D3D11)
    _sg_d3d11_apply_viewport(x, y, w, h, origin_top_left);
    #elif defined(SOKOL_WGPU)
    _sg_wgpu_apply_viewport(x, y, w, h, origin_top_left);
    #elif defined(SOKOL_DUMMY_BACKEND)
    _sg_dummy_apply_viewport(x, y, w, h, origin_top_left);
    #else
    #error("INVALID BACKEND");
    #endif
}

static inline void _sg_apply_scissor_rect(int x, int y, int w, int h, bool origin_top_left) {
    #if defined(_SOKOL_ANY_GL)
    _sg_gl_apply_scissor_rect(x, y, w, h, origin_top_left);
    #elif defined(SOKOL_METAL)
    _sg_mtl_apply_scissor_rect(x, y, w, h, origin_top_left);
    #elif defined(SOKOL_D3D11)
    _sg_d3d11_apply_scissor_rect(x, y, w, h, origin_top_left);
    #elif defined(SOKOL_WGPU)
    _sg_wgpu_apply_scissor_rect(x, y, w, h, origin_top_left);
    #elif defined(SOKOL_DUMMY_BACKEND)
    _sg_dummy_apply_scissor_rect(x, y, w, h, origin_top_left);
    #else
    #error("INVALID BACKEND");
    #endif
}

static inline void _sg_apply_pipeline(_sg_pipeline_t* pip) {
    #if defined(_SOKOL_ANY_GL)
    _sg_gl_apply_pipeline(pip);
    #elif defined(SOKOL_METAL)
    _sg_mtl_apply_pipeline(pip);
    #elif defined(SOKOL_D3D11)
    _sg_d3d11_apply_pipeline(pip);
    #elif defined(SOKOL_WGPU)
    _sg_wgpu_apply_pipeline(pip);
    #elif defined(SOKOL_DUMMY_BACKEND)
    _sg_dummy_apply_pipeline(pip);
    #else
    #error("INVALID BACKEND");
    #endif
}

static inline void _sg_apply_bindings(
    _sg_pipeline_t* pip,
    _sg_buffer_t** vbs, const int* vb_offsets, int num_vbs,
    _sg_buffer_t* ib, int ib_offset,
    _sg_image_t** vs_imgs, int num_vs_imgs,
    _sg_image_t** fs_imgs, int num_fs_imgs)
{
    #if defined(_SOKOL_ANY_GL)
    _sg_gl_apply_bindings(pip, vbs, vb_offsets, num_vbs, ib, ib_offset, vs_imgs, num_vs_imgs, fs_imgs, num_fs_imgs);
    #elif defined(SOKOL_METAL)
    _sg_mtl_apply_bindings(pip, vbs, vb_offsets, num_vbs, ib, ib_offset, vs_imgs, num_vs_imgs, fs_imgs, num_fs_imgs);
    #elif defined(SOKOL_D3D11)
    _sg_d3d11_apply_bindings(pip, vbs, vb_offsets, num_vbs, ib, ib_offset, vs_imgs, num_vs_imgs, fs_imgs, num_fs_imgs);
    #elif defined(SOKOL_WGPU)
    _sg_wgpu_apply_bindings(pip, vbs, vb_offsets, num_vbs, ib, ib_offset, vs_imgs, num_vs_imgs, fs_imgs, num_fs_imgs);
    #elif defined(SOKOL_DUMMY_BACKEND)
    _sg_dummy_apply_bindings(pip, vbs, vb_offsets, num_vbs, ib, ib_offset, vs_imgs, num_vs_imgs, fs_imgs, num_fs_imgs);
    #else
    #error("INVALID BACKEND");
    #endif
}

static inline void _sg_apply_uniforms(sg_shader_stage stage_index, int ub_index, const sg_range* data) {
    #if defined(_SOKOL_ANY_GL)
    _sg_gl_apply_uniforms(stage_index, ub_index, data);
    #elif defined(SOKOL_METAL)
    _sg_mtl_apply_uniforms(stage_index, ub_index, data);
    #elif defined(SOKOL_D3D11)
    _sg_d3d11_apply_uniforms(stage_index, ub_index, data);
    #elif defined(SOKOL_WGPU)
    _sg_wgpu_apply_uniforms(stage_index, ub_index, data);
    #elif defined(SOKOL_DUMMY_BACKEND)
    _sg_dummy_apply_uniforms(stage_index, ub_index, data);
    #else
    #error("INVALID BACKEND");
    #endif
}

static inline void _sg_draw(int base_element, int num_elements, int num_instances) {
    #if defined(_SOKOL_ANY_GL)
    _sg_gl_draw(base_element, num_elements, num_instances);
    #elif defined(SOKOL_METAL)
    _sg_mtl_draw(base_element, num_elements, num_instances);
    #elif defined(SOKOL_D3D11)
    _sg_d3d11_draw(base_element, num_elements, num_instances);
    #elif defined(SOKOL_WGPU)
    _sg_wgpu_draw(base_element, num_elements, num_instances);
    #elif defined(SOKOL_DUMMY_BACKEND)
    _sg_dummy_draw(base_element, num_elements, num_instances);
    #else
    #error("INVALID BACKEND");
    #endif
}

static inline void _sg_commit(void) {
    #if defined(_SOKOL_ANY_GL)
    _sg_gl_commit();
    #elif defined(SOKOL_METAL)
    _sg_mtl_commit();
    #elif defined(SOKOL_D3D11)
    _sg_d3d11_commit();
    #elif defined(SOKOL_WGPU)
    _sg_wgpu_commit();
    #elif defined(SOKOL_DUMMY_BACKEND)
    _sg_dummy_commit();
    #else
    #error("INVALID BACKEND");
    #endif
}

static inline void _sg_update_buffer(_sg_buffer_t* buf, const sg_range* data) {
    #if defined(_SOKOL_ANY_GL)
    _sg_gl_update_buffer(buf, data);
    #elif defined(SOKOL_METAL)
    _sg_mtl_update_buffer(buf, data);
    #elif defined(SOKOL_D3D11)
    _sg_d3d11_update_buffer(buf, data);
    #elif defined(SOKOL_WGPU)
    _sg_wgpu_update_buffer(buf, data);
    #elif defined(SOKOL_DUMMY_BACKEND)
    _sg_dummy_update_buffer(buf, data);
    #else
    #error("INVALID BACKEND");
    #endif
}

static inline int _sg_append_buffer(_sg_buffer_t* buf, const sg_range* data, bool new_frame) {
    #if defined(_SOKOL_ANY_GL)
    return _sg_gl_append_buffer(buf, data, new_frame);
    #elif defined(SOKOL_METAL)
    return _sg_mtl_append_buffer(buf, data, new_frame);
    #elif defined(SOKOL_D3D11)
    return _sg_d3d11_append_buffer(buf, data, new_frame);
    #elif defined(SOKOL_WGPU)
    return _sg_wgpu_append_buffer(buf, data, new_frame);
    #elif defined(SOKOL_DUMMY_BACKEND)
    return _sg_dummy_append_buffer(buf, data, new_frame);
    #else
    #error("INVALID BACKEND");
    #endif
}

static inline void _sg_update_image(_sg_image_t* img, const sg_image_data* data) {
    #if defined(_SOKOL_ANY_GL)
    _sg_gl_update_image(img, data);
    #elif defined(SOKOL_METAL)
    _sg_mtl_update_image(img, data);
    #elif defined(SOKOL_D3D11)
    _sg_d3d11_update_image(img, data);
    #elif defined(SOKOL_WGPU)
    _sg_wgpu_update_image(img, data);
    #elif defined(SOKOL_DUMMY_BACKEND)
    _sg_dummy_update_image(img, data);
    #else
    #error("INVALID BACKEND");
    #endif
}

/*== RESOURCE POOLS ==========================================================*/

_SOKOL_PRIVATE void _sg_init_pool(_sg_pool_t* pool, int num) {
    SOKOL_ASSERT(pool && (num >= 1));
    /* slot 0 is reserved for the 'invalid id', so bump the pool size by 1 */
    pool->size = num + 1;
    pool->queue_top = 0;
    /* generation counters indexable by pool slot index, slot 0 is reserved */
    size_t gen_ctrs_size = sizeof(uint32_t) * (size_t)pool->size;
    pool->gen_ctrs = (uint32_t*)_sg_malloc_clear(gen_ctrs_size);
    /* it's not a bug to only reserve 'num' here */
    pool->free_queue = (int*) _sg_malloc_clear(sizeof(int) * (size_t)num);
    /* never allocate the zero-th pool item since the invalid id is 0 */
    for (int i = pool->size-1; i >= 1; i--) {
        pool->free_queue[pool->queue_top++] = i;
    }
}

_SOKOL_PRIVATE void _sg_discard_pool(_sg_pool_t* pool) {
    SOKOL_ASSERT(pool);
    SOKOL_ASSERT(pool->free_queue);
    _sg_free(pool->free_queue);
    pool->free_queue = 0;
    SOKOL_ASSERT(pool->gen_ctrs);
    _sg_free(pool->gen_ctrs);
    pool->gen_ctrs = 0;
    pool->size = 0;
    pool->queue_top = 0;
}

_SOKOL_PRIVATE int _sg_pool_alloc_index(_sg_pool_t* pool) {
    SOKOL_ASSERT(pool);
    SOKOL_ASSERT(pool->free_queue);
    if (pool->queue_top > 0) {
        int slot_index = pool->free_queue[--pool->queue_top];
        SOKOL_ASSERT((slot_index > 0) && (slot_index < pool->size));
        return slot_index;
    }
    else {
        /* pool exhausted */
        return _SG_INVALID_SLOT_INDEX;
    }
}

_SOKOL_PRIVATE void _sg_pool_free_index(_sg_pool_t* pool, int slot_index) {
    SOKOL_ASSERT((slot_index > _SG_INVALID_SLOT_INDEX) && (slot_index < pool->size));
    SOKOL_ASSERT(pool);
    SOKOL_ASSERT(pool->free_queue);
    SOKOL_ASSERT(pool->queue_top < pool->size);
    #ifdef SOKOL_DEBUG
    /* debug check against double-free */
    for (int i = 0; i < pool->queue_top; i++) {
        SOKOL_ASSERT(pool->free_queue[i] != slot_index);
    }
    #endif
    pool->free_queue[pool->queue_top++] = slot_index;
    SOKOL_ASSERT(pool->queue_top <= (pool->size-1));
}

_SOKOL_PRIVATE void _sg_reset_slot(_sg_slot_t* slot) {
    SOKOL_ASSERT(slot);
    _sg_clear(slot, sizeof(_sg_slot_t));
}

_SOKOL_PRIVATE void _sg_reset_buffer_to_alloc_state(_sg_buffer_t* buf) {
    SOKOL_ASSERT(buf);
    _sg_slot_t slot = buf->slot;
    _sg_clear(buf, sizeof(_sg_buffer_t));
    buf->slot = slot;
    buf->slot.state = SG_RESOURCESTATE_ALLOC;
}

_SOKOL_PRIVATE void _sg_reset_image_to_alloc_state(_sg_image_t* img) {
    SOKOL_ASSERT(img);
    _sg_slot_t slot = img->slot;
    _sg_clear(img, sizeof(_sg_image_t));
    img->slot = slot;
    img->slot.state = SG_RESOURCESTATE_ALLOC;
}

_SOKOL_PRIVATE void _sg_reset_shader_to_alloc_state(_sg_shader_t* shd) {
    SOKOL_ASSERT(shd);
    _sg_slot_t slot = shd->slot;
    _sg_clear(shd, sizeof(_sg_shader_t));
    shd->slot = slot;
    shd->slot.state = SG_RESOURCESTATE_ALLOC;
}

_SOKOL_PRIVATE void _sg_reset_pipeline_to_alloc_state(_sg_pipeline_t* pip) {
    SOKOL_ASSERT(pip);
    _sg_slot_t slot = pip->slot;
    _sg_clear(pip, sizeof(_sg_pipeline_t));
    pip->slot = slot;
    pip->slot.state = SG_RESOURCESTATE_ALLOC;
}

_SOKOL_PRIVATE void _sg_reset_pass_to_alloc_state(_sg_pass_t* pass) {
    SOKOL_ASSERT(pass);
    _sg_slot_t slot = pass->slot;
    _sg_clear(pass, sizeof(_sg_pass_t));
    pass->slot = slot;
    pass->slot.state = SG_RESOURCESTATE_ALLOC;
}

_SOKOL_PRIVATE void _sg_reset_context_to_alloc_state(_sg_context_t* ctx) {
    SOKOL_ASSERT(ctx);
    _sg_slot_t slot = ctx->slot;
    _sg_clear(ctx, sizeof(_sg_context_t));
    ctx->slot = slot;
    ctx->slot.state = SG_RESOURCESTATE_ALLOC;
}

_SOKOL_PRIVATE void _sg_setup_pools(_sg_pools_t* p, const sg_desc* desc) {
    SOKOL_ASSERT(p);
    SOKOL_ASSERT(desc);
    /* note: the pools here will have an additional item, since slot 0 is reserved */
    SOKOL_ASSERT((desc->buffer_pool_size > 0) && (desc->buffer_pool_size < _SG_MAX_POOL_SIZE));
    _sg_init_pool(&p->buffer_pool, desc->buffer_pool_size);
    size_t buffer_pool_byte_size = sizeof(_sg_buffer_t) * (size_t)p->buffer_pool.size;
    p->buffers = (_sg_buffer_t*) _sg_malloc_clear(buffer_pool_byte_size);

    SOKOL_ASSERT((desc->image_pool_size > 0) && (desc->image_pool_size < _SG_MAX_POOL_SIZE));
    _sg_init_pool(&p->image_pool, desc->image_pool_size);
    size_t image_pool_byte_size = sizeof(_sg_image_t) * (size_t)p->image_pool.size;
    p->images = (_sg_image_t*) _sg_malloc_clear(image_pool_byte_size);

    SOKOL_ASSERT((desc->shader_pool_size > 0) && (desc->shader_pool_size < _SG_MAX_POOL_SIZE));
    _sg_init_pool(&p->shader_pool, desc->shader_pool_size);
    size_t shader_pool_byte_size = sizeof(_sg_shader_t) * (size_t)p->shader_pool.size;
    p->shaders = (_sg_shader_t*) _sg_malloc_clear(shader_pool_byte_size);

    SOKOL_ASSERT((desc->pipeline_pool_size > 0) && (desc->pipeline_pool_size < _SG_MAX_POOL_SIZE));
    _sg_init_pool(&p->pipeline_pool, desc->pipeline_pool_size);
    size_t pipeline_pool_byte_size = sizeof(_sg_pipeline_t) * (size_t)p->pipeline_pool.size;
    p->pipelines = (_sg_pipeline_t*) _sg_malloc_clear(pipeline_pool_byte_size);

    SOKOL_ASSERT((desc->pass_pool_size > 0) && (desc->pass_pool_size < _SG_MAX_POOL_SIZE));
    _sg_init_pool(&p->pass_pool, desc->pass_pool_size);
    size_t pass_pool_byte_size = sizeof(_sg_pass_t) * (size_t)p->pass_pool.size;
    p->passes = (_sg_pass_t*) _sg_malloc_clear(pass_pool_byte_size);

    SOKOL_ASSERT((desc->context_pool_size > 0) && (desc->context_pool_size < _SG_MAX_POOL_SIZE));
    _sg_init_pool(&p->context_pool, desc->context_pool_size);
    size_t context_pool_byte_size = sizeof(_sg_context_t) * (size_t)p->context_pool.size;
    p->contexts = (_sg_context_t*) _sg_malloc_clear(context_pool_byte_size);
}

_SOKOL_PRIVATE void _sg_discard_pools(_sg_pools_t* p) {
    SOKOL_ASSERT(p);
    _sg_free(p->contexts);    p->contexts = 0;
    _sg_free(p->passes);      p->passes = 0;
    _sg_free(p->pipelines);   p->pipelines = 0;
    _sg_free(p->shaders);     p->shaders = 0;
    _sg_free(p->images);      p->images = 0;
    _sg_free(p->buffers);     p->buffers = 0;
    _sg_discard_pool(&p->context_pool);
    _sg_discard_pool(&p->pass_pool);
    _sg_discard_pool(&p->pipeline_pool);
    _sg_discard_pool(&p->shader_pool);
    _sg_discard_pool(&p->image_pool);
    _sg_discard_pool(&p->buffer_pool);
}

/* allocate the slot at slot_index:
    - bump the slot's generation counter
    - create a resource id from the generation counter and slot index
    - set the slot's id to this id
    - set the slot's state to ALLOC
    - return the resource id
*/
_SOKOL_PRIVATE uint32_t _sg_slot_alloc(_sg_pool_t* pool, _sg_slot_t* slot, int slot_index) {
    /* FIXME: add handling for an overflowing generation counter,
       for now, just overflow (another option is to disable
       the slot)
    */
    SOKOL_ASSERT(pool && pool->gen_ctrs);
    SOKOL_ASSERT((slot_index > _SG_INVALID_SLOT_INDEX) && (slot_index < pool->size));
    SOKOL_ASSERT((slot->state == SG_RESOURCESTATE_INITIAL) && (slot->id == SG_INVALID_ID));
    uint32_t ctr = ++pool->gen_ctrs[slot_index];
    slot->id = (ctr<<_SG_SLOT_SHIFT)|(slot_index & _SG_SLOT_MASK);
    slot->state = SG_RESOURCESTATE_ALLOC;
    return slot->id;
}

/* extract slot index from id */
_SOKOL_PRIVATE int _sg_slot_index(uint32_t id) {
    int slot_index = (int) (id & _SG_SLOT_MASK);
    SOKOL_ASSERT(_SG_INVALID_SLOT_INDEX != slot_index);
    return slot_index;
}

/* returns pointer to resource by id without matching id check */
_SOKOL_PRIVATE _sg_buffer_t* _sg_buffer_at(const _sg_pools_t* p, uint32_t buf_id) {
    SOKOL_ASSERT(p && (SG_INVALID_ID != buf_id));
    int slot_index = _sg_slot_index(buf_id);
    SOKOL_ASSERT((slot_index > _SG_INVALID_SLOT_INDEX) && (slot_index < p->buffer_pool.size));
    return &p->buffers[slot_index];
}

_SOKOL_PRIVATE _sg_image_t* _sg_image_at(const _sg_pools_t* p, uint32_t img_id) {
    SOKOL_ASSERT(p && (SG_INVALID_ID != img_id));
    int slot_index = _sg_slot_index(img_id);
    SOKOL_ASSERT((slot_index > _SG_INVALID_SLOT_INDEX) && (slot_index < p->image_pool.size));
    return &p->images[slot_index];
}

_SOKOL_PRIVATE _sg_shader_t* _sg_shader_at(const _sg_pools_t* p, uint32_t shd_id) {
    SOKOL_ASSERT(p && (SG_INVALID_ID != shd_id));
    int slot_index = _sg_slot_index(shd_id);
    SOKOL_ASSERT((slot_index > _SG_INVALID_SLOT_INDEX) && (slot_index < p->shader_pool.size));
    return &p->shaders[slot_index];
}

_SOKOL_PRIVATE _sg_pipeline_t* _sg_pipeline_at(const _sg_pools_t* p, uint32_t pip_id) {
    SOKOL_ASSERT(p && (SG_INVALID_ID != pip_id));
    int slot_index = _sg_slot_index(pip_id);
    SOKOL_ASSERT((slot_index > _SG_INVALID_SLOT_INDEX) && (slot_index < p->pipeline_pool.size));
    return &p->pipelines[slot_index];
}

_SOKOL_PRIVATE _sg_pass_t* _sg_pass_at(const _sg_pools_t* p, uint32_t pass_id) {
    SOKOL_ASSERT(p && (SG_INVALID_ID != pass_id));
    int slot_index = _sg_slot_index(pass_id);
    SOKOL_ASSERT((slot_index > _SG_INVALID_SLOT_INDEX) && (slot_index < p->pass_pool.size));
    return &p->passes[slot_index];
}

_SOKOL_PRIVATE _sg_context_t* _sg_context_at(const _sg_pools_t* p, uint32_t context_id) {
    SOKOL_ASSERT(p && (SG_INVALID_ID != context_id));
    int slot_index = _sg_slot_index(context_id);
    SOKOL_ASSERT((slot_index > _SG_INVALID_SLOT_INDEX) && (slot_index < p->context_pool.size));
    return &p->contexts[slot_index];
}

/* returns pointer to resource with matching id check, may return 0 */
_SOKOL_PRIVATE _sg_buffer_t* _sg_lookup_buffer(const _sg_pools_t* p, uint32_t buf_id) {
    if (SG_INVALID_ID != buf_id) {
        _sg_buffer_t* buf = _sg_buffer_at(p, buf_id);
        if (buf->slot.id == buf_id) {
            return buf;
        }
    }
    return 0;
}

_SOKOL_PRIVATE _sg_image_t* _sg_lookup_image(const _sg_pools_t* p, uint32_t img_id) {
    if (SG_INVALID_ID != img_id) {
        _sg_image_t* img = _sg_image_at(p, img_id);
        if (img->slot.id == img_id) {
            return img;
        }
    }
    return 0;
}

_SOKOL_PRIVATE _sg_shader_t* _sg_lookup_shader(const _sg_pools_t* p, uint32_t shd_id) {
    SOKOL_ASSERT(p);
    if (SG_INVALID_ID != shd_id) {
        _sg_shader_t* shd = _sg_shader_at(p, shd_id);
        if (shd->slot.id == shd_id) {
            return shd;
        }
    }
    return 0;
}

_SOKOL_PRIVATE _sg_pipeline_t* _sg_lookup_pipeline(const _sg_pools_t* p, uint32_t pip_id) {
    SOKOL_ASSERT(p);
    if (SG_INVALID_ID != pip_id) {
        _sg_pipeline_t* pip = _sg_pipeline_at(p, pip_id);
        if (pip->slot.id == pip_id) {
            return pip;
        }
    }
    return 0;
}

_SOKOL_PRIVATE _sg_pass_t* _sg_lookup_pass(const _sg_pools_t* p, uint32_t pass_id) {
    SOKOL_ASSERT(p);
    if (SG_INVALID_ID != pass_id) {
        _sg_pass_t* pass = _sg_pass_at(p, pass_id);
        if (pass->slot.id == pass_id) {
            return pass;
        }
    }
    return 0;
}

_SOKOL_PRIVATE _sg_context_t* _sg_lookup_context(const _sg_pools_t* p, uint32_t ctx_id) {
    SOKOL_ASSERT(p);
    if (SG_INVALID_ID != ctx_id) {
        _sg_context_t* ctx = _sg_context_at(p, ctx_id);
        if (ctx->slot.id == ctx_id) {
            return ctx;
        }
    }
    return 0;
}

_SOKOL_PRIVATE void _sg_discard_all_resources(_sg_pools_t* p, uint32_t ctx_id) {
    /*  this is a bit dumb since it loops over all pool slots to
        find the occupied slots, on the other hand it is only ever
        executed at shutdown
        NOTE: ONLY EXECUTE THIS AT SHUTDOWN
              ...because the free queues will not be reset
              and the resource slots not be cleared!
    */
    for (int i = 1; i < p->buffer_pool.size; i++) {
        if (p->buffers[i].slot.ctx_id == ctx_id) {
            sg_resource_state state = p->buffers[i].slot.state;
            if ((state == SG_RESOURCESTATE_VALID) || (state == SG_RESOURCESTATE_FAILED)) {
                _sg_discard_buffer(&p->buffers[i]);
            }
        }
    }
    for (int i = 1; i < p->image_pool.size; i++) {
        if (p->images[i].slot.ctx_id == ctx_id) {
            sg_resource_state state = p->images[i].slot.state;
            if ((state == SG_RESOURCESTATE_VALID) || (state == SG_RESOURCESTATE_FAILED)) {
                _sg_discard_image(&p->images[i]);
            }
        }
    }
    for (int i = 1; i < p->shader_pool.size; i++) {
        if (p->shaders[i].slot.ctx_id == ctx_id) {
            sg_resource_state state = p->shaders[i].slot.state;
            if ((state == SG_RESOURCESTATE_VALID) || (state == SG_RESOURCESTATE_FAILED)) {
                _sg_discard_shader(&p->shaders[i]);
            }
        }
    }
    for (int i = 1; i < p->pipeline_pool.size; i++) {
        if (p->pipelines[i].slot.ctx_id == ctx_id) {
            sg_resource_state state = p->pipelines[i].slot.state;
            if ((state == SG_RESOURCESTATE_VALID) || (state == SG_RESOURCESTATE_FAILED)) {
                _sg_discard_pipeline(&p->pipelines[i]);
            }
        }
    }
    for (int i = 1; i < p->pass_pool.size; i++) {
        if (p->passes[i].slot.ctx_id == ctx_id) {
            sg_resource_state state = p->passes[i].slot.state;
            if ((state == SG_RESOURCESTATE_VALID) || (state == SG_RESOURCESTATE_FAILED)) {
                _sg_discard_pass(&p->passes[i]);
            }
        }
    }
}

/*== VALIDATION LAYER ========================================================*/
#if defined(SOKOL_DEBUG)
/* return a human readable string for an _sg_validate_error */
_SOKOL_PRIVATE const char* _sg_validate_string(_sg_validate_error_t err) {
    switch (err) {
        /* buffer creation validation errors */
        case _SG_VALIDATE_BUFFERDESC_CANARY:        return "sg_buffer_desc not initialized";
        case _SG_VALIDATE_BUFFERDESC_SIZE:          return "sg_buffer_desc.size cannot be 0";
        case _SG_VALIDATE_BUFFERDESC_DATA:          return "immutable buffers must be initialized with data (sg_buffer_desc.data.ptr and sg_buffer_desc.data.size)";
        case _SG_VALIDATE_BUFFERDESC_DATA_SIZE:     return "immutable buffer data size differs from buffer size";
        case _SG_VALIDATE_BUFFERDESC_NO_DATA:       return "dynamic/stream usage buffers cannot be initialized with data";

        /* image data (in image creation and updating) */
        case _SG_VALIDATE_IMAGEDATA_NODATA:         return "sg_image_data: no data (.ptr and/or .size is zero)";
        case _SG_VALIDATE_IMAGEDATA_DATA_SIZE:      return "sg_image_data: data size doesn't match expected surface size";

        /* image creation validation errros */
        case _SG_VALIDATE_IMAGEDESC_CANARY:             return "sg_image_desc not initialized";
        case _SG_VALIDATE_IMAGEDESC_WIDTH:              return "sg_image_desc.width must be > 0";
        case _SG_VALIDATE_IMAGEDESC_HEIGHT:             return "sg_image_desc.height must be > 0";
        case _SG_VALIDATE_IMAGEDESC_RT_PIXELFORMAT:     return "invalid pixel format for render-target image";
        case _SG_VALIDATE_IMAGEDESC_NONRT_PIXELFORMAT:  return "invalid pixel format for non-render-target image";
        case _SG_VALIDATE_IMAGEDESC_MSAA_BUT_NO_RT:     return "non-render-target images cannot be multisampled";
        case _SG_VALIDATE_IMAGEDESC_NO_MSAA_RT_SUPPORT: return "MSAA not supported for this pixel format";
        case _SG_VALIDATE_IMAGEDESC_RT_IMMUTABLE:       return "render target images must be SG_USAGE_IMMUTABLE";
        case _SG_VALIDATE_IMAGEDESC_RT_NO_DATA:         return "render target images cannot be initialized with data";
        case _SG_VALIDATE_IMAGEDESC_INJECTED_NO_DATA:   return "images with injected textures cannot be initialized with data";
        case _SG_VALIDATE_IMAGEDESC_DYNAMIC_NO_DATA:    return "dynamic/stream images cannot be initialized with data";
        case _SG_VALIDATE_IMAGEDESC_COMPRESSED_IMMUTABLE:   return "compressed images must be immutable";

        /* shader creation */
        case _SG_VALIDATE_SHADERDESC_CANARY:                return "sg_shader_desc not initialized";
        case _SG_VALIDATE_SHADERDESC_SOURCE:                return "shader source code required";
        case _SG_VALIDATE_SHADERDESC_BYTECODE:              return "shader byte code required";
        case _SG_VALIDATE_SHADERDESC_SOURCE_OR_BYTECODE:    return "shader source or byte code required";
        case _SG_VALIDATE_SHADERDESC_NO_BYTECODE_SIZE:      return "shader byte code length (in bytes) required";
        case _SG_VALIDATE_SHADERDESC_NO_CONT_UBS:           return "shader uniform blocks must occupy continuous slots";
        case _SG_VALIDATE_SHADERDESC_NO_CONT_UB_MEMBERS:    return "uniform block members must occupy continuous slots";
        case _SG_VALIDATE_SHADERDESC_NO_UB_MEMBERS:         return "GL backend requires uniform block member declarations";
        case _SG_VALIDATE_SHADERDESC_UB_MEMBER_NAME:        return "uniform block member name missing";
        case _SG_VALIDATE_SHADERDESC_UB_SIZE_MISMATCH:      return "size of uniform block members doesn't match uniform block size";
        case _SG_VALIDATE_SHADERDESC_UB_ARRAY_COUNT:        return "uniform array count must be >= 1";
        case _SG_VALIDATE_SHADERDESC_UB_STD140_ARRAY_TYPE:  return "uniform arrays only allowed for FLOAT4, INT4, MAT4 in std140 layout";

        case _SG_VALIDATE_SHADERDESC_NO_CONT_IMGS:          return "shader images must occupy continuous slots";
        case _SG_VALIDATE_SHADERDESC_IMG_NAME:              return "GL backend requires uniform block member names";
        case _SG_VALIDATE_SHADERDESC_ATTR_NAMES:            return "GLES2 backend requires vertex attribute names";
        case _SG_VALIDATE_SHADERDESC_ATTR_SEMANTICS:        return "D3D11 backend requires vertex attribute semantics";
        case _SG_VALIDATE_SHADERDESC_ATTR_STRING_TOO_LONG:  return "vertex attribute name/semantic string too long (max len 16)";

        /* pipeline creation */
        case _SG_VALIDATE_PIPELINEDESC_CANARY:          return "sg_pipeline_desc not initialized";
        case _SG_VALIDATE_PIPELINEDESC_SHADER:          return "sg_pipeline_desc.shader missing or invalid";
        case _SG_VALIDATE_PIPELINEDESC_NO_ATTRS:        return "sg_pipeline_desc.layout.attrs is empty or not continuous";
        case _SG_VALIDATE_PIPELINEDESC_LAYOUT_STRIDE4:  return "sg_pipeline_desc.layout.buffers[].stride must be multiple of 4";
        case _SG_VALIDATE_PIPELINEDESC_ATTR_NAME:       return "GLES2/WebGL missing vertex attribute name in shader";
        case _SG_VALIDATE_PIPELINEDESC_ATTR_SEMANTICS:  return "D3D11 missing vertex attribute semantics in shader";

        /* pass creation */
        case _SG_VALIDATE_PASSDESC_CANARY:                  return "sg_pass_desc not initialized";
        case _SG_VALIDATE_PASSDESC_NO_COLOR_ATTS:           return "sg_pass_desc.color_attachments[0] must be valid";
        case _SG_VALIDATE_PASSDESC_NO_CONT_COLOR_ATTS:      return "color attachments must occupy continuous slots";
        case _SG_VALIDATE_PASSDESC_IMAGE:                   return "pass attachment image is not valid";
        case _SG_VALIDATE_PASSDESC_MIPLEVEL:                return "pass attachment mip level is bigger than image has mipmaps";
        case _SG_VALIDATE_PASSDESC_FACE:                    return "pass attachment image is cubemap, but face index is too big";
        case _SG_VALIDATE_PASSDESC_LAYER:                   return "pass attachment image is array texture, but layer index is too big";
        case _SG_VALIDATE_PASSDESC_SLICE:                   return "pass attachment image is 3d texture, but slice value is too big";
        case _SG_VALIDATE_PASSDESC_IMAGE_NO_RT:             return "pass attachment image must be render targets";
        case _SG_VALIDATE_PASSDESC_COLOR_INV_PIXELFORMAT:   return "pass color-attachment images must have a renderable pixel format";
        case _SG_VALIDATE_PASSDESC_DEPTH_INV_PIXELFORMAT:   return "pass depth-attachment image must have depth pixel format";
        case _SG_VALIDATE_PASSDESC_IMAGE_SIZES:             return "all pass attachments must have the same size";
        case _SG_VALIDATE_PASSDESC_IMAGE_SAMPLE_COUNTS:     return "all pass attachments must have the same sample count";

        /* sg_begin_pass */
        case _SG_VALIDATE_BEGINPASS_PASS:       return "sg_begin_pass: pass must be valid";
        case _SG_VALIDATE_BEGINPASS_IMAGE:      return "sg_begin_pass: one or more attachment images are not valid";

        /* sg_apply_pipeline */
        case _SG_VALIDATE_APIP_PIPELINE_VALID_ID:   return "sg_apply_pipeline: invalid pipeline id provided";
        case _SG_VALIDATE_APIP_PIPELINE_EXISTS:     return "sg_apply_pipeline: pipeline object no longer alive";
        case _SG_VALIDATE_APIP_PIPELINE_VALID:      return "sg_apply_pipeline: pipeline object not in valid state";
        case _SG_VALIDATE_APIP_SHADER_EXISTS:       return "sg_apply_pipeline: shader object no longer alive";
        case _SG_VALIDATE_APIP_SHADER_VALID:        return "sg_apply_pipeline: shader object not in valid state";
        case _SG_VALIDATE_APIP_ATT_COUNT:           return "sg_apply_pipeline: number of pipeline color attachments doesn't match number of pass color attachments";
        case _SG_VALIDATE_APIP_COLOR_FORMAT:        return "sg_apply_pipeline: pipeline color attachment pixel format doesn't match pass color attachment pixel format";
        case _SG_VALIDATE_APIP_DEPTH_FORMAT:        return "sg_apply_pipeline: pipeline depth pixel_format doesn't match pass depth attachment pixel format";
        case _SG_VALIDATE_APIP_SAMPLE_COUNT:        return "sg_apply_pipeline: pipeline MSAA sample count doesn't match render pass attachment sample count";

        /* sg_apply_bindings */
        case _SG_VALIDATE_ABND_PIPELINE:            return "sg_apply_bindings: must be called after sg_apply_pipeline";
        case _SG_VALIDATE_ABND_PIPELINE_EXISTS:     return "sg_apply_bindings: currently applied pipeline object no longer alive";
        case _SG_VALIDATE_ABND_PIPELINE_VALID:      return "sg_apply_bindings: currently applied pipeline object not in valid state";
        case _SG_VALIDATE_ABND_VBS:                 return "sg_apply_bindings: number of vertex buffers doesn't match number of pipeline vertex layouts";
        case _SG_VALIDATE_ABND_VB_EXISTS:           return "sg_apply_bindings: vertex buffer no longer alive";
        case _SG_VALIDATE_ABND_VB_TYPE:             return "sg_apply_bindings: buffer in vertex buffer slot is not a SG_BUFFERTYPE_VERTEXBUFFER";
        case _SG_VALIDATE_ABND_VB_OVERFLOW:         return "sg_apply_bindings: buffer in vertex buffer slot is overflown";
        case _SG_VALIDATE_ABND_NO_IB:               return "sg_apply_bindings: pipeline object defines indexed rendering, but no index buffer provided";
        case _SG_VALIDATE_ABND_IB:                  return "sg_apply_bindings: pipeline object defines non-indexed rendering, but index buffer provided";
        case _SG_VALIDATE_ABND_IB_EXISTS:           return "sg_apply_bindings: index buffer no longer alive";
        case _SG_VALIDATE_ABND_IB_TYPE:             return "sg_apply_bindings: buffer in index buffer slot is not a SG_BUFFERTYPE_INDEXBUFFER";
        case _SG_VALIDATE_ABND_IB_OVERFLOW:         return "sg_apply_bindings: buffer in index buffer slot is overflown";
        case _SG_VALIDATE_ABND_VS_IMGS:             return "sg_apply_bindings: vertex shader image count doesn't match sg_shader_desc";
        case _SG_VALIDATE_ABND_VS_IMG_EXISTS:       return "sg_apply_bindings: vertex shader image no longer alive";
        case _SG_VALIDATE_ABND_VS_IMG_TYPES:        return "sg_apply_bindings: one or more vertex shader image types don't match sg_shader_desc";
        case _SG_VALIDATE_ABND_FS_IMGS:             return "sg_apply_bindings: fragment shader image count doesn't match sg_shader_desc";
        case _SG_VALIDATE_ABND_FS_IMG_EXISTS:       return "sg_apply_bindings: fragment shader image no longer alive";
        case _SG_VALIDATE_ABND_FS_IMG_TYPES:        return "sg_apply_bindings: one or more fragment shader image types don't match sg_shader_desc";

        /* sg_apply_uniforms */
        case _SG_VALIDATE_AUB_NO_PIPELINE:      return "sg_apply_uniforms: must be called after sg_apply_pipeline()";
        case _SG_VALIDATE_AUB_NO_UB_AT_SLOT:    return "sg_apply_uniforms: no uniform block declaration at this shader stage UB slot";
        case _SG_VALIDATE_AUB_SIZE:             return "sg_apply_uniforms: data size exceeds declared uniform block size";

        /* sg_update_buffer */
        case _SG_VALIDATE_UPDATEBUF_USAGE:      return "sg_update_buffer: cannot update immutable buffer";
        case _SG_VALIDATE_UPDATEBUF_SIZE:       return "sg_update_buffer: update size is bigger than buffer size";
        case _SG_VALIDATE_UPDATEBUF_ONCE:       return "sg_update_buffer: only one update allowed per buffer and frame";
        case _SG_VALIDATE_UPDATEBUF_APPEND:     return "sg_update_buffer: cannot call sg_update_buffer and sg_append_buffer in same frame";

        /* sg_append_buffer */
        case _SG_VALIDATE_APPENDBUF_USAGE:      return "sg_append_buffer: cannot append to immutable buffer";
        case _SG_VALIDATE_APPENDBUF_SIZE:       return "sg_append_buffer: overall appended size is bigger than buffer size";
        case _SG_VALIDATE_APPENDBUF_UPDATE:     return "sg_append_buffer: cannot call sg_append_buffer and sg_update_buffer in same frame";

        /* sg_update_image */
        case _SG_VALIDATE_UPDIMG_USAGE:         return "sg_update_image: cannot update immutable image";
        case _SG_VALIDATE_UPDIMG_ONCE:          return "sg_update_image: only one update allowed per image and frame";

        default: return "unknown validation error";
    }
}
#endif /* defined(SOKOL_DEBUG) */

/*-- validation checks -------------------------------------------------------*/
#if defined(SOKOL_DEBUG)
_SOKOL_PRIVATE void _sg_validate_begin(void) {
    _sg.validate_error = _SG_VALIDATE_SUCCESS;
}

_SOKOL_PRIVATE void _sg_validate(bool cond, _sg_validate_error_t err) {
    if (!cond) {
        _sg.validate_error = err;
        SG_LOG(_sg_validate_string(err));
    }
}

_SOKOL_PRIVATE bool _sg_validate_end(void) {
    if (_sg.validate_error != _SG_VALIDATE_SUCCESS) {
        #if !defined(SOKOL_VALIDATE_NON_FATAL)
            SG_LOG("^^^^  SOKOL-GFX VALIDATION FAILED, TERMINATING ^^^^");
            SOKOL_ASSERT(false);
        #endif
        return false;
    }
    else {
        return true;
    }
}
#endif

_SOKOL_PRIVATE bool _sg_validate_buffer_desc(const sg_buffer_desc* desc) {
    #if !defined(SOKOL_DEBUG)
        _SOKOL_UNUSED(desc);
        return true;
    #else
        if (_sg.desc.disable_validation) {
            return true;
        }
        SOKOL_ASSERT(desc);
        SOKOL_VALIDATE_BEGIN();
        SOKOL_VALIDATE(desc->_start_canary == 0, _SG_VALIDATE_BUFFERDESC_CANARY);
        SOKOL_VALIDATE(desc->_end_canary == 0, _SG_VALIDATE_BUFFERDESC_CANARY);
        SOKOL_VALIDATE(desc->size > 0, _SG_VALIDATE_BUFFERDESC_SIZE);
        bool injected = (0 != desc->gl_buffers[0]) ||
                        (0 != desc->mtl_buffers[0]) ||
                        (0 != desc->d3d11_buffer) ||
                        (0 != desc->wgpu_buffer);
        if (!injected && (desc->usage == SG_USAGE_IMMUTABLE)) {
            SOKOL_VALIDATE((0 != desc->data.ptr) && (desc->data.size > 0), _SG_VALIDATE_BUFFERDESC_DATA);
            SOKOL_VALIDATE(desc->size == desc->data.size, _SG_VALIDATE_BUFFERDESC_DATA_SIZE);
        }
        else {
            SOKOL_VALIDATE(0 == desc->data.ptr, _SG_VALIDATE_BUFFERDESC_NO_DATA);
        }
        return SOKOL_VALIDATE_END();
    #endif
}

_SOKOL_PRIVATE void _sg_validate_image_data(const sg_image_data* data, sg_pixel_format fmt, int width, int height, int num_faces, int num_mips, int num_slices) {
    #if !defined(SOKOL_DEBUG)
        _SOKOL_UNUSED(data);
        _SOKOL_UNUSED(fmt);
        _SOKOL_UNUSED(width);
        _SOKOL_UNUSED(height);
        _SOKOL_UNUSED(num_faces);
        _SOKOL_UNUSED(num_mips);
        _SOKOL_UNUSED(num_slices);
    #else
        for (int face_index = 0; face_index < num_faces; face_index++) {
            for (int mip_index = 0; mip_index < num_mips; mip_index++) {
                const bool has_data = data->subimage[face_index][mip_index].ptr != 0;
                const bool has_size = data->subimage[face_index][mip_index].size > 0;
                SOKOL_VALIDATE(has_data && has_size, _SG_VALIDATE_IMAGEDATA_NODATA);
                const int mip_width = _sg_max(width >> mip_index, 1);
                const int mip_height = _sg_max(height >> mip_index, 1);
                const int bytes_per_slice = _sg_surface_pitch(fmt, mip_width, mip_height, 1);
                const int expected_size = bytes_per_slice * num_slices;
                SOKOL_VALIDATE(expected_size == (int)data->subimage[face_index][mip_index].size, _SG_VALIDATE_IMAGEDATA_DATA_SIZE);
            }
        }
    #endif
}

_SOKOL_PRIVATE bool _sg_validate_image_desc(const sg_image_desc* desc) {
    #if !defined(SOKOL_DEBUG)
        _SOKOL_UNUSED(desc);
        return true;
    #else
        if (_sg.desc.disable_validation) {
            return true;
        }
        SOKOL_ASSERT(desc);
        SOKOL_VALIDATE_BEGIN();
        SOKOL_VALIDATE(desc->_start_canary == 0, _SG_VALIDATE_IMAGEDESC_CANARY);
        SOKOL_VALIDATE(desc->_end_canary == 0, _SG_VALIDATE_IMAGEDESC_CANARY);
        SOKOL_VALIDATE(desc->width > 0, _SG_VALIDATE_IMAGEDESC_WIDTH);
        SOKOL_VALIDATE(desc->height > 0, _SG_VALIDATE_IMAGEDESC_HEIGHT);
        const sg_pixel_format fmt = desc->pixel_format;
        const sg_usage usage = desc->usage;
        const bool injected = (0 != desc->gl_textures[0]) ||
                              (0 != desc->mtl_textures[0]) ||
                              (0 != desc->d3d11_texture) ||
                              (0 != desc->wgpu_texture);
        if (desc->render_target) {
            SOKOL_ASSERT(((int)fmt >= 0) && ((int)fmt < _SG_PIXELFORMAT_NUM));
            SOKOL_VALIDATE(_sg.formats[fmt].render, _SG_VALIDATE_IMAGEDESC_RT_PIXELFORMAT);
            /* on GLES2, sample count for render targets is completely ignored */
            #if defined(SOKOL_GLES2) || defined(SOKOL_GLES3)
            if (!_sg.gl.gles2) {
            #endif
                if (desc->sample_count > 1) {
                    SOKOL_VALIDATE(_sg.features.msaa_render_targets && _sg.formats[fmt].msaa, _SG_VALIDATE_IMAGEDESC_NO_MSAA_RT_SUPPORT);
                }
            #if defined(SOKOL_GLES2) || defined(SOKOL_GLES3)
            }
            #endif
            SOKOL_VALIDATE(usage == SG_USAGE_IMMUTABLE, _SG_VALIDATE_IMAGEDESC_RT_IMMUTABLE);
            SOKOL_VALIDATE(desc->data.subimage[0][0].ptr==0, _SG_VALIDATE_IMAGEDESC_RT_NO_DATA);
        }
        else {
            SOKOL_VALIDATE(desc->sample_count <= 1, _SG_VALIDATE_IMAGEDESC_MSAA_BUT_NO_RT);
            const bool valid_nonrt_fmt = !_sg_is_valid_rendertarget_depth_format(fmt);
            SOKOL_VALIDATE(valid_nonrt_fmt, _SG_VALIDATE_IMAGEDESC_NONRT_PIXELFORMAT);
            const bool is_compressed = _sg_is_compressed_pixel_format(desc->pixel_format);
            const bool is_immutable = (usage == SG_USAGE_IMMUTABLE);
            if (is_compressed) {
                SOKOL_VALIDATE(is_immutable, _SG_VALIDATE_IMAGEDESC_COMPRESSED_IMMUTABLE);
            }
            if (!injected && is_immutable) {
                // image desc must have valid data
                _sg_validate_image_data(&desc->data,
                    desc->pixel_format,
                    desc->width,
                    desc->height,
                    (desc->type == SG_IMAGETYPE_CUBE) ? 6 : 1,
                    desc->num_mipmaps,
                    desc->num_slices);
            }
            else {
                // image desc must not have data
                for (int face_index = 0; face_index < SG_CUBEFACE_NUM; face_index++) {
                    for (int mip_index = 0; mip_index < SG_MAX_MIPMAPS; mip_index++) {
                        const bool no_data = 0 == desc->data.subimage[face_index][mip_index].ptr;
                        const bool no_size = 0 == desc->data.subimage[face_index][mip_index].size;
                        if (injected) {
                            SOKOL_VALIDATE(no_data && no_size, _SG_VALIDATE_IMAGEDESC_INJECTED_NO_DATA);
                        }
                        if (!is_immutable) {
                            SOKOL_VALIDATE(no_data && no_size, _SG_VALIDATE_IMAGEDESC_DYNAMIC_NO_DATA);
                        }
                    }
                }
            }
        }
        return SOKOL_VALIDATE_END();
    #endif
}

_SOKOL_PRIVATE bool _sg_validate_shader_desc(const sg_shader_desc* desc) {
    #if !defined(SOKOL_DEBUG)
        _SOKOL_UNUSED(desc);
        return true;
    #else
        if (_sg.desc.disable_validation) {
            return true;
        }
        SOKOL_ASSERT(desc);
        SOKOL_VALIDATE_BEGIN();
        SOKOL_VALIDATE(desc->_start_canary == 0, _SG_VALIDATE_SHADERDESC_CANARY);
        SOKOL_VALIDATE(desc->_end_canary == 0, _SG_VALIDATE_SHADERDESC_CANARY);
        #if defined(SOKOL_GLES2)
            SOKOL_VALIDATE(0 != desc->attrs[0].name, _SG_VALIDATE_SHADERDESC_ATTR_NAMES);
        #elif defined(SOKOL_D3D11)
            SOKOL_VALIDATE(0 != desc->attrs[0].sem_name, _SG_VALIDATE_SHADERDESC_ATTR_SEMANTICS);
        #endif
        #if defined(SOKOL_GLCORE33) || defined(SOKOL_GLES2) || defined(SOKOL_GLES3)
            /* on GL, must provide shader source code */
            SOKOL_VALIDATE(0 != desc->vs.source, _SG_VALIDATE_SHADERDESC_SOURCE);
            SOKOL_VALIDATE(0 != desc->fs.source, _SG_VALIDATE_SHADERDESC_SOURCE);
        #elif defined(SOKOL_METAL) || defined(SOKOL_D3D11)
            /* on Metal or D3D11, must provide shader source code or byte code */
            SOKOL_VALIDATE((0 != desc->vs.source)||(0 != desc->vs.bytecode.ptr), _SG_VALIDATE_SHADERDESC_SOURCE_OR_BYTECODE);
            SOKOL_VALIDATE((0 != desc->fs.source)||(0 != desc->fs.bytecode.ptr), _SG_VALIDATE_SHADERDESC_SOURCE_OR_BYTECODE);
        #elif defined(SOKOL_WGPU)
            /* on WGPU byte code must be provided */
            SOKOL_VALIDATE((0 != desc->vs.bytecode.ptr), _SG_VALIDATE_SHADERDESC_BYTECODE);
            SOKOL_VALIDATE((0 != desc->fs.bytecode.ptr), _SG_VALIDATE_SHADERDESC_BYTECODE);
        #else
            /* Dummy Backend, don't require source or bytecode */
        #endif
        for (int i = 0; i < SG_MAX_VERTEX_ATTRIBUTES; i++) {
            if (desc->attrs[i].name) {
                SOKOL_VALIDATE(strlen(desc->attrs[i].name) < _SG_STRING_SIZE, _SG_VALIDATE_SHADERDESC_ATTR_STRING_TOO_LONG);
            }
            if (desc->attrs[i].sem_name) {
                SOKOL_VALIDATE(strlen(desc->attrs[i].sem_name) < _SG_STRING_SIZE, _SG_VALIDATE_SHADERDESC_ATTR_STRING_TOO_LONG);
            }
        }
        /* if shader byte code, the size must also be provided */
        if (0 != desc->vs.bytecode.ptr) {
            SOKOL_VALIDATE(desc->vs.bytecode.size > 0, _SG_VALIDATE_SHADERDESC_NO_BYTECODE_SIZE);
        }
        if (0 != desc->fs.bytecode.ptr) {
            SOKOL_VALIDATE(desc->fs.bytecode.size > 0, _SG_VALIDATE_SHADERDESC_NO_BYTECODE_SIZE);
        }
        for (int stage_index = 0; stage_index < SG_NUM_SHADER_STAGES; stage_index++) {
            const sg_shader_stage_desc* stage_desc = (stage_index == 0)? &desc->vs : &desc->fs;
            bool uniform_blocks_continuous = true;
            for (int ub_index = 0; ub_index < SG_MAX_SHADERSTAGE_UBS; ub_index++) {
                const sg_shader_uniform_block_desc* ub_desc = &stage_desc->uniform_blocks[ub_index];
                if (ub_desc->size > 0) {
                    SOKOL_VALIDATE(uniform_blocks_continuous, _SG_VALIDATE_SHADERDESC_NO_CONT_UBS);
                    #if defined(_SOKOL_ANY_GL)
                    bool uniforms_continuous = true;
                    uint32_t uniform_offset = 0;
                    int num_uniforms = 0;
                    for (int u_index = 0; u_index < SG_MAX_UB_MEMBERS; u_index++) {
                        const sg_shader_uniform_desc* u_desc = &ub_desc->uniforms[u_index];
                        if (u_desc->type != SG_UNIFORMTYPE_INVALID) {
                            SOKOL_VALIDATE(uniforms_continuous, _SG_VALIDATE_SHADERDESC_NO_CONT_UB_MEMBERS);
                            #if defined(SOKOL_GLES2) || defined(SOKOL_GLES3)
                            SOKOL_VALIDATE(0 != u_desc->name, _SG_VALIDATE_SHADERDESC_UB_MEMBER_NAME);
                            #endif
                            const int array_count = u_desc->array_count;
                            SOKOL_VALIDATE(array_count > 0, _SG_VALIDATE_SHADERDESC_UB_ARRAY_COUNT);
                            const uint32_t u_align = _sg_uniform_alignment(u_desc->type, array_count, ub_desc->layout);
                            const uint32_t u_size  = _sg_uniform_size(u_desc->type, array_count, ub_desc->layout);
                            uniform_offset = _sg_align_u32(uniform_offset, u_align);
                            uniform_offset += u_size;
                            num_uniforms++;
                            // with std140, arrays are only allowed for FLOAT4, INT4, MAT4
                            if (ub_desc->layout == SG_UNIFORMLAYOUT_STD140) {
                                if (array_count > 1) {
                                    SOKOL_VALIDATE((u_desc->type == SG_UNIFORMTYPE_FLOAT4) || (u_desc->type == SG_UNIFORMTYPE_INT4) || (u_desc->type == SG_UNIFORMTYPE_MAT4), _SG_VALIDATE_SHADERDESC_UB_STD140_ARRAY_TYPE);
                                }
                            }
                        }
                        else {
                            uniforms_continuous = false;
                        }
                    }
                    if (ub_desc->layout == SG_UNIFORMLAYOUT_STD140) {
                        uniform_offset = _sg_align_u32(uniform_offset, 16);
                    }
                    SOKOL_VALIDATE((size_t)uniform_offset == ub_desc->size, _SG_VALIDATE_SHADERDESC_UB_SIZE_MISMATCH);
                    SOKOL_VALIDATE(num_uniforms > 0, _SG_VALIDATE_SHADERDESC_NO_UB_MEMBERS);
                    #endif
                }
                else {
                    uniform_blocks_continuous = false;
                }
            }
            bool images_continuous = true;
            for (int img_index = 0; img_index < SG_MAX_SHADERSTAGE_IMAGES; img_index++) {
                const sg_shader_image_desc* img_desc = &stage_desc->images[img_index];
                if (img_desc->image_type != _SG_IMAGETYPE_DEFAULT) {
                    SOKOL_VALIDATE(images_continuous, _SG_VALIDATE_SHADERDESC_NO_CONT_IMGS);
                    #if defined(SOKOL_GLES2)
                    SOKOL_VALIDATE(0 != img_desc->name, _SG_VALIDATE_SHADERDESC_IMG_NAME);
                    #endif
                }
                else {
                    images_continuous = false;
                }
            }
        }
        return SOKOL_VALIDATE_END();
    #endif
}

_SOKOL_PRIVATE bool _sg_validate_pipeline_desc(const sg_pipeline_desc* desc) {
    #if !defined(SOKOL_DEBUG)
        _SOKOL_UNUSED(desc);
        return true;
    #else
        if (_sg.desc.disable_validation) {
            return true;
        }
        SOKOL_ASSERT(desc);
        SOKOL_VALIDATE_BEGIN();
        SOKOL_VALIDATE(desc->_start_canary == 0, _SG_VALIDATE_PIPELINEDESC_CANARY);
        SOKOL_VALIDATE(desc->_end_canary == 0, _SG_VALIDATE_PIPELINEDESC_CANARY);
        SOKOL_VALIDATE(desc->shader.id != SG_INVALID_ID, _SG_VALIDATE_PIPELINEDESC_SHADER);
        for (int buf_index = 0; buf_index < SG_MAX_SHADERSTAGE_BUFFERS; buf_index++) {
            const sg_buffer_layout_desc* l_desc = &desc->layout.buffers[buf_index];
            if (l_desc->stride == 0) {
                continue;
            }
            SOKOL_VALIDATE((l_desc->stride & 3) == 0, _SG_VALIDATE_PIPELINEDESC_LAYOUT_STRIDE4);
        }
        SOKOL_VALIDATE(desc->layout.attrs[0].format != SG_VERTEXFORMAT_INVALID, _SG_VALIDATE_PIPELINEDESC_NO_ATTRS);
        const _sg_shader_t* shd = _sg_lookup_shader(&_sg.pools, desc->shader.id);
        SOKOL_VALIDATE(0 != shd, _SG_VALIDATE_PIPELINEDESC_SHADER);
        if (shd) {
            SOKOL_VALIDATE(shd->slot.state == SG_RESOURCESTATE_VALID, _SG_VALIDATE_PIPELINEDESC_SHADER);
            bool attrs_cont = true;
            for (int attr_index = 0; attr_index < SG_MAX_VERTEX_ATTRIBUTES; attr_index++) {
                const sg_vertex_attr_desc* a_desc = &desc->layout.attrs[attr_index];
                if (a_desc->format == SG_VERTEXFORMAT_INVALID) {
                    attrs_cont = false;
                    continue;
                }
                SOKOL_VALIDATE(attrs_cont, _SG_VALIDATE_PIPELINEDESC_NO_ATTRS);
                SOKOL_ASSERT(a_desc->buffer_index < SG_MAX_SHADERSTAGE_BUFFERS);
                #if defined(SOKOL_GLES2)
                /* on GLES2, vertex attribute names must be provided */
                SOKOL_VALIDATE(!_sg_strempty(&shd->gl.attrs[attr_index].name), _SG_VALIDATE_PIPELINEDESC_ATTR_NAME);
                #elif defined(SOKOL_D3D11)
                /* on D3D11, semantic names (and semantic indices) must be provided */
                SOKOL_VALIDATE(!_sg_strempty(&shd->d3d11.attrs[attr_index].sem_name), _SG_VALIDATE_PIPELINEDESC_ATTR_SEMANTICS);
                #endif
            }
        }
        return SOKOL_VALIDATE_END();
    #endif
}

_SOKOL_PRIVATE bool _sg_validate_pass_desc(const sg_pass_desc* desc) {
    #if !defined(SOKOL_DEBUG)
        _SOKOL_UNUSED(desc);
        return true;
    #else
        if (_sg.desc.disable_validation) {
            return true;
        }
        SOKOL_ASSERT(desc);
        SOKOL_VALIDATE_BEGIN();
        SOKOL_VALIDATE(desc->_start_canary == 0, _SG_VALIDATE_PASSDESC_CANARY);
        SOKOL_VALIDATE(desc->_end_canary == 0, _SG_VALIDATE_PASSDESC_CANARY);
        bool atts_cont = true;
        int width = -1, height = -1, sample_count = -1;
        for (int att_index = 0; att_index < SG_MAX_COLOR_ATTACHMENTS; att_index++) {
            const sg_pass_attachment_desc* att = &desc->color_attachments[att_index];
            if (att->image.id == SG_INVALID_ID) {
                SOKOL_VALIDATE(att_index > 0, _SG_VALIDATE_PASSDESC_NO_COLOR_ATTS);
                atts_cont = false;
                continue;
            }
            SOKOL_VALIDATE(atts_cont, _SG_VALIDATE_PASSDESC_NO_CONT_COLOR_ATTS);
            const _sg_image_t* img = _sg_lookup_image(&_sg.pools, att->image.id);
            SOKOL_ASSERT(img);
            SOKOL_VALIDATE(img->slot.state == SG_RESOURCESTATE_VALID, _SG_VALIDATE_PASSDESC_IMAGE);
            SOKOL_VALIDATE(att->mip_level < img->cmn.num_mipmaps, _SG_VALIDATE_PASSDESC_MIPLEVEL);
            if (img->cmn.type == SG_IMAGETYPE_CUBE) {
                SOKOL_VALIDATE(att->slice < 6, _SG_VALIDATE_PASSDESC_FACE);
            }
            else if (img->cmn.type == SG_IMAGETYPE_ARRAY) {
                SOKOL_VALIDATE(att->slice < img->cmn.num_slices, _SG_VALIDATE_PASSDESC_LAYER);
            }
            else if (img->cmn.type == SG_IMAGETYPE_3D) {
                SOKOL_VALIDATE(att->slice < img->cmn.num_slices, _SG_VALIDATE_PASSDESC_SLICE);
            }
            SOKOL_VALIDATE(img->cmn.render_target, _SG_VALIDATE_PASSDESC_IMAGE_NO_RT);
            if (att_index == 0) {
                width = img->cmn.width >> att->mip_level;
                height = img->cmn.height >> att->mip_level;
                sample_count = img->cmn.sample_count;
            }
            else {
                SOKOL_VALIDATE(width == img->cmn.width >> att->mip_level, _SG_VALIDATE_PASSDESC_IMAGE_SIZES);
                SOKOL_VALIDATE(height == img->cmn.height >> att->mip_level, _SG_VALIDATE_PASSDESC_IMAGE_SIZES);
                SOKOL_VALIDATE(sample_count == img->cmn.sample_count, _SG_VALIDATE_PASSDESC_IMAGE_SAMPLE_COUNTS);
            }
            SOKOL_VALIDATE(_sg_is_valid_rendertarget_color_format(img->cmn.pixel_format), _SG_VALIDATE_PASSDESC_COLOR_INV_PIXELFORMAT);
        }
        if (desc->depth_stencil_attachment.image.id != SG_INVALID_ID) {
            const sg_pass_attachment_desc* att = &desc->depth_stencil_attachment;
            const _sg_image_t* img = _sg_lookup_image(&_sg.pools, att->image.id);
            SOKOL_ASSERT(img);
            SOKOL_VALIDATE(img->slot.state == SG_RESOURCESTATE_VALID, _SG_VALIDATE_PASSDESC_IMAGE);
            SOKOL_VALIDATE(att->mip_level < img->cmn.num_mipmaps, _SG_VALIDATE_PASSDESC_MIPLEVEL);
            if (img->cmn.type == SG_IMAGETYPE_CUBE) {
                SOKOL_VALIDATE(att->slice < 6, _SG_VALIDATE_PASSDESC_FACE);
            }
            else if (img->cmn.type == SG_IMAGETYPE_ARRAY) {
                SOKOL_VALIDATE(att->slice < img->cmn.num_slices, _SG_VALIDATE_PASSDESC_LAYER);
            }
            else if (img->cmn.type == SG_IMAGETYPE_3D) {
                SOKOL_VALIDATE(att->slice < img->cmn.num_slices, _SG_VALIDATE_PASSDESC_SLICE);
            }
            SOKOL_VALIDATE(img->cmn.render_target, _SG_VALIDATE_PASSDESC_IMAGE_NO_RT);
            SOKOL_VALIDATE(width == img->cmn.width >> att->mip_level, _SG_VALIDATE_PASSDESC_IMAGE_SIZES);
            SOKOL_VALIDATE(height == img->cmn.height >> att->mip_level, _SG_VALIDATE_PASSDESC_IMAGE_SIZES);
            SOKOL_VALIDATE(sample_count == img->cmn.sample_count, _SG_VALIDATE_PASSDESC_IMAGE_SAMPLE_COUNTS);
            SOKOL_VALIDATE(_sg_is_valid_rendertarget_depth_format(img->cmn.pixel_format), _SG_VALIDATE_PASSDESC_DEPTH_INV_PIXELFORMAT);
        }
        return SOKOL_VALIDATE_END();
    #endif
}

_SOKOL_PRIVATE bool _sg_validate_begin_pass(_sg_pass_t* pass) {
    #if !defined(SOKOL_DEBUG)
        _SOKOL_UNUSED(pass);
        return true;
    #else
        if (_sg.desc.disable_validation) {
            return true;
        }
        SOKOL_VALIDATE_BEGIN();
        SOKOL_VALIDATE(pass->slot.state == SG_RESOURCESTATE_VALID, _SG_VALIDATE_BEGINPASS_PASS);

        for (int i = 0; i < SG_MAX_COLOR_ATTACHMENTS; i++) {
            const _sg_pass_attachment_t* att = &pass->cmn.color_atts[i];
            const _sg_image_t* img = _sg_pass_color_image(pass, i);
            if (img) {
                SOKOL_VALIDATE(img->slot.state == SG_RESOURCESTATE_VALID, _SG_VALIDATE_BEGINPASS_IMAGE);
                SOKOL_VALIDATE(img->slot.id == att->image_id.id, _SG_VALIDATE_BEGINPASS_IMAGE);
            }
        }
        const _sg_image_t* ds_img = _sg_pass_ds_image(pass);
        if (ds_img) {
            const _sg_pass_attachment_t* att = &pass->cmn.ds_att;
            SOKOL_VALIDATE(ds_img->slot.state == SG_RESOURCESTATE_VALID, _SG_VALIDATE_BEGINPASS_IMAGE);
            SOKOL_VALIDATE(ds_img->slot.id == att->image_id.id, _SG_VALIDATE_BEGINPASS_IMAGE);
        }
        return SOKOL_VALIDATE_END();
    #endif
}

_SOKOL_PRIVATE bool _sg_validate_apply_pipeline(sg_pipeline pip_id) {
    #if !defined(SOKOL_DEBUG)
        _SOKOL_UNUSED(pip_id);
        return true;
    #else
        if (_sg.desc.disable_validation) {
            return true;
        }
        SOKOL_VALIDATE_BEGIN();
        /* the pipeline object must be alive and valid */
        SOKOL_VALIDATE(pip_id.id != SG_INVALID_ID, _SG_VALIDATE_APIP_PIPELINE_VALID_ID);
        const _sg_pipeline_t* pip = _sg_lookup_pipeline(&_sg.pools, pip_id.id);
        SOKOL_VALIDATE(pip != 0, _SG_VALIDATE_APIP_PIPELINE_EXISTS);
        if (!pip) {
            return SOKOL_VALIDATE_END();
        }
        SOKOL_VALIDATE(pip->slot.state == SG_RESOURCESTATE_VALID, _SG_VALIDATE_APIP_PIPELINE_VALID);
        /* the pipeline's shader must be alive and valid */
        SOKOL_ASSERT(pip->shader);
        SOKOL_VALIDATE(pip->shader->slot.id == pip->cmn.shader_id.id, _SG_VALIDATE_APIP_SHADER_EXISTS);
        SOKOL_VALIDATE(pip->shader->slot.state == SG_RESOURCESTATE_VALID, _SG_VALIDATE_APIP_SHADER_VALID);
        /* check that pipeline attributes match current pass attributes */
        const _sg_pass_t* pass = _sg_lookup_pass(&_sg.pools, _sg.cur_pass.id);
        if (pass) {
            /* an offscreen pass */
            SOKOL_VALIDATE(pip->cmn.color_attachment_count == pass->cmn.num_color_atts, _SG_VALIDATE_APIP_ATT_COUNT);
            for (int i = 0; i < pip->cmn.color_attachment_count; i++) {
                const _sg_image_t* att_img = _sg_pass_color_image(pass, i);
                SOKOL_VALIDATE(pip->cmn.color_formats[i] == att_img->cmn.pixel_format, _SG_VALIDATE_APIP_COLOR_FORMAT);
                SOKOL_VALIDATE(pip->cmn.sample_count == att_img->cmn.sample_count, _SG_VALIDATE_APIP_SAMPLE_COUNT);
            }
            const _sg_image_t* att_dsimg = _sg_pass_ds_image(pass);
            if (att_dsimg) {
                SOKOL_VALIDATE(pip->cmn.depth_format == att_dsimg->cmn.pixel_format, _SG_VALIDATE_APIP_DEPTH_FORMAT);
            }
            else {
                SOKOL_VALIDATE(pip->cmn.depth_format == SG_PIXELFORMAT_NONE, _SG_VALIDATE_APIP_DEPTH_FORMAT);
            }
        }
        else {
            /* default pass */
            SOKOL_VALIDATE(pip->cmn.color_attachment_count == 1, _SG_VALIDATE_APIP_ATT_COUNT);
            SOKOL_VALIDATE(pip->cmn.color_formats[0] == _sg.desc.context.color_format, _SG_VALIDATE_APIP_COLOR_FORMAT);
            SOKOL_VALIDATE(pip->cmn.depth_format == _sg.desc.context.depth_format, _SG_VALIDATE_APIP_DEPTH_FORMAT);
            SOKOL_VALIDATE(pip->cmn.sample_count == _sg.desc.context.sample_count, _SG_VALIDATE_APIP_SAMPLE_COUNT);
        }
        return SOKOL_VALIDATE_END();
    #endif
}

_SOKOL_PRIVATE bool _sg_validate_apply_bindings(const sg_bindings* bindings) {
    #if !defined(SOKOL_DEBUG)
        _SOKOL_UNUSED(bindings);
        return true;
    #else
        if (_sg.desc.disable_validation) {
            return true;
        }
        SOKOL_VALIDATE_BEGIN();

        /* a pipeline object must have been applied */
        SOKOL_VALIDATE(_sg.cur_pipeline.id != SG_INVALID_ID, _SG_VALIDATE_ABND_PIPELINE);
        const _sg_pipeline_t* pip = _sg_lookup_pipeline(&_sg.pools, _sg.cur_pipeline.id);
        SOKOL_VALIDATE(pip != 0, _SG_VALIDATE_ABND_PIPELINE_EXISTS);
        if (!pip) {
            return SOKOL_VALIDATE_END();
        }
        SOKOL_VALIDATE(pip->slot.state == SG_RESOURCESTATE_VALID, _SG_VALIDATE_ABND_PIPELINE_VALID);
        SOKOL_ASSERT(pip->shader && (pip->cmn.shader_id.id == pip->shader->slot.id));

        /* has expected vertex buffers, and vertex buffers still exist */
        for (int i = 0; i < SG_MAX_SHADERSTAGE_BUFFERS; i++) {
            if (bindings->vertex_buffers[i].id != SG_INVALID_ID) {
                SOKOL_VALIDATE(pip->cmn.vertex_layout_valid[i], _SG_VALIDATE_ABND_VBS);
                /* buffers in vertex-buffer-slots must be of type SG_BUFFERTYPE_VERTEXBUFFER */
                const _sg_buffer_t* buf = _sg_lookup_buffer(&_sg.pools, bindings->vertex_buffers[i].id);
                SOKOL_VALIDATE(buf != 0, _SG_VALIDATE_ABND_VB_EXISTS);
                if (buf && buf->slot.state == SG_RESOURCESTATE_VALID) {
                    SOKOL_VALIDATE(SG_BUFFERTYPE_VERTEXBUFFER == buf->cmn.type, _SG_VALIDATE_ABND_VB_TYPE);
                    SOKOL_VALIDATE(!buf->cmn.append_overflow, _SG_VALIDATE_ABND_VB_OVERFLOW);
                }
            }
            else {
                /* vertex buffer provided in a slot which has no vertex layout in pipeline */
                SOKOL_VALIDATE(!pip->cmn.vertex_layout_valid[i], _SG_VALIDATE_ABND_VBS);
            }
        }

        /* index buffer expected or not, and index buffer still exists */
        if (pip->cmn.index_type == SG_INDEXTYPE_NONE) {
            /* pipeline defines non-indexed rendering, but index buffer provided */
            SOKOL_VALIDATE(bindings->index_buffer.id == SG_INVALID_ID, _SG_VALIDATE_ABND_IB);
        }
        else {
            /* pipeline defines indexed rendering, but no index buffer provided */
            SOKOL_VALIDATE(bindings->index_buffer.id != SG_INVALID_ID, _SG_VALIDATE_ABND_NO_IB);
        }
        if (bindings->index_buffer.id != SG_INVALID_ID) {
            /* buffer in index-buffer-slot must be of type SG_BUFFERTYPE_INDEXBUFFER */
            const _sg_buffer_t* buf = _sg_lookup_buffer(&_sg.pools, bindings->index_buffer.id);
            SOKOL_VALIDATE(buf != 0, _SG_VALIDATE_ABND_IB_EXISTS);
            if (buf && buf->slot.state == SG_RESOURCESTATE_VALID) {
                SOKOL_VALIDATE(SG_BUFFERTYPE_INDEXBUFFER == buf->cmn.type, _SG_VALIDATE_ABND_IB_TYPE);
                SOKOL_VALIDATE(!buf->cmn.append_overflow, _SG_VALIDATE_ABND_IB_OVERFLOW);
            }
        }

        /* has expected vertex shader images */
        for (int i = 0; i < SG_MAX_SHADERSTAGE_IMAGES; i++) {
            _sg_shader_stage_t* stage = &pip->shader->cmn.stage[SG_SHADERSTAGE_VS];
            if (bindings->vs_images[i].id != SG_INVALID_ID) {
                SOKOL_VALIDATE(i < stage->num_images, _SG_VALIDATE_ABND_VS_IMGS);
                const _sg_image_t* img = _sg_lookup_image(&_sg.pools, bindings->vs_images[i].id);
                SOKOL_VALIDATE(img != 0, _SG_VALIDATE_ABND_VS_IMG_EXISTS);
                if (img && img->slot.state == SG_RESOURCESTATE_VALID) {
                    SOKOL_VALIDATE(img->cmn.type == stage->images[i].image_type, _SG_VALIDATE_ABND_VS_IMG_TYPES);
                }
            }
            else {
                SOKOL_VALIDATE(i >= stage->num_images, _SG_VALIDATE_ABND_VS_IMGS);
            }
        }

        /* has expected fragment shader images */
        for (int i = 0; i < SG_MAX_SHADERSTAGE_IMAGES; i++) {
            _sg_shader_stage_t* stage = &pip->shader->cmn.stage[SG_SHADERSTAGE_FS];
            if (bindings->fs_images[i].id != SG_INVALID_ID) {
                SOKOL_VALIDATE(i < stage->num_images, _SG_VALIDATE_ABND_FS_IMGS);
                const _sg_image_t* img = _sg_lookup_image(&_sg.pools, bindings->fs_images[i].id);
                SOKOL_VALIDATE(img != 0, _SG_VALIDATE_ABND_FS_IMG_EXISTS);
                if (img && img->slot.state == SG_RESOURCESTATE_VALID) {
                    SOKOL_VALIDATE(img->cmn.type == stage->images[i].image_type, _SG_VALIDATE_ABND_FS_IMG_TYPES);
                }
            }
            else {
                SOKOL_VALIDATE(i >= stage->num_images, _SG_VALIDATE_ABND_FS_IMGS);
            }
        }
        return SOKOL_VALIDATE_END();
    #endif
}

_SOKOL_PRIVATE bool _sg_validate_apply_uniforms(sg_shader_stage stage_index, int ub_index, const sg_range* data) {
    #if !defined(SOKOL_DEBUG)
        _SOKOL_UNUSED(stage_index);
        _SOKOL_UNUSED(ub_index);
        _SOKOL_UNUSED(data);
        return true;
    #else
        if (_sg.desc.disable_validation) {
            return true;
        }
        SOKOL_ASSERT((stage_index == SG_SHADERSTAGE_VS) || (stage_index == SG_SHADERSTAGE_FS));
        SOKOL_ASSERT((ub_index >= 0) && (ub_index < SG_MAX_SHADERSTAGE_UBS));
        SOKOL_VALIDATE_BEGIN();
        SOKOL_VALIDATE(_sg.cur_pipeline.id != SG_INVALID_ID, _SG_VALIDATE_AUB_NO_PIPELINE);
        const _sg_pipeline_t* pip = _sg_lookup_pipeline(&_sg.pools, _sg.cur_pipeline.id);
        SOKOL_ASSERT(pip && (pip->slot.id == _sg.cur_pipeline.id));
        SOKOL_ASSERT(pip->shader && (pip->shader->slot.id == pip->cmn.shader_id.id));

        /* check that there is a uniform block at 'stage' and 'ub_index' */
        const _sg_shader_stage_t* stage = &pip->shader->cmn.stage[stage_index];
        SOKOL_VALIDATE(ub_index < stage->num_uniform_blocks, _SG_VALIDATE_AUB_NO_UB_AT_SLOT);

        /* check that the provided data size doesn't exceed the uniform block size */
        SOKOL_VALIDATE(data->size <= stage->uniform_blocks[ub_index].size, _SG_VALIDATE_AUB_SIZE);

        return SOKOL_VALIDATE_END();
    #endif
}

_SOKOL_PRIVATE bool _sg_validate_update_buffer(const _sg_buffer_t* buf, const sg_range* data) {
    #if !defined(SOKOL_DEBUG)
        _SOKOL_UNUSED(buf);
        _SOKOL_UNUSED(data);
        return true;
    #else
        if (_sg.desc.disable_validation) {
            return true;
        }
        SOKOL_ASSERT(buf && data && data->ptr);
        SOKOL_VALIDATE_BEGIN();
        SOKOL_VALIDATE(buf->cmn.usage != SG_USAGE_IMMUTABLE, _SG_VALIDATE_UPDATEBUF_USAGE);
        SOKOL_VALIDATE(buf->cmn.size >= (int)data->size, _SG_VALIDATE_UPDATEBUF_SIZE);
        SOKOL_VALIDATE(buf->cmn.update_frame_index != _sg.frame_index, _SG_VALIDATE_UPDATEBUF_ONCE);
        SOKOL_VALIDATE(buf->cmn.append_frame_index != _sg.frame_index, _SG_VALIDATE_UPDATEBUF_APPEND);
        return SOKOL_VALIDATE_END();
    #endif
}

_SOKOL_PRIVATE bool _sg_validate_append_buffer(const _sg_buffer_t* buf, const sg_range* data) {
    #if !defined(SOKOL_DEBUG)
        _SOKOL_UNUSED(buf);
        _SOKOL_UNUSED(data);
        return true;
    #else
        if (_sg.desc.disable_validation) {
            return true;
        }
        SOKOL_ASSERT(buf && data && data->ptr);
        SOKOL_VALIDATE_BEGIN();
        SOKOL_VALIDATE(buf->cmn.usage != SG_USAGE_IMMUTABLE, _SG_VALIDATE_APPENDBUF_USAGE);
        SOKOL_VALIDATE(buf->cmn.size >= (buf->cmn.append_pos + (int)data->size), _SG_VALIDATE_APPENDBUF_SIZE);
        SOKOL_VALIDATE(buf->cmn.update_frame_index != _sg.frame_index, _SG_VALIDATE_APPENDBUF_UPDATE);
        return SOKOL_VALIDATE_END();
    #endif
}

_SOKOL_PRIVATE bool _sg_validate_update_image(const _sg_image_t* img, const sg_image_data* data) {
    #if !defined(SOKOL_DEBUG)
        _SOKOL_UNUSED(img);
        _SOKOL_UNUSED(data);
        return true;
    #else
        if (_sg.desc.disable_validation) {
            return true;
        }
        SOKOL_ASSERT(img && data);
        SOKOL_VALIDATE_BEGIN();
        SOKOL_VALIDATE(img->cmn.usage != SG_USAGE_IMMUTABLE, _SG_VALIDATE_UPDIMG_USAGE);
        SOKOL_VALIDATE(img->cmn.upd_frame_index != _sg.frame_index, _SG_VALIDATE_UPDIMG_ONCE);
        _sg_validate_image_data(data,
            img->cmn.pixel_format,
            img->cmn.width,
            img->cmn.height,
            (img->cmn.type == SG_IMAGETYPE_CUBE) ? 6 : 1,
            img->cmn.num_mipmaps,
            img->cmn.num_slices);
        return SOKOL_VALIDATE_END();
    #endif
}

/*== fill in desc default values =============================================*/
_SOKOL_PRIVATE sg_buffer_desc _sg_buffer_desc_defaults(const sg_buffer_desc* desc) {
    sg_buffer_desc def = *desc;
    def.type = _sg_def(def.type, SG_BUFFERTYPE_VERTEXBUFFER);
    def.usage = _sg_def(def.usage, SG_USAGE_IMMUTABLE);
    if (def.size == 0) {
        def.size = def.data.size;
    }
    else if (def.data.size == 0) {
        def.data.size = def.size;
    }
    return def;
}

_SOKOL_PRIVATE sg_image_desc _sg_image_desc_defaults(const sg_image_desc* desc) {
    sg_image_desc def = *desc;
    def.type = _sg_def(def.type, SG_IMAGETYPE_2D);
    def.num_slices = _sg_def(def.num_slices, 1);
    def.num_mipmaps = _sg_def(def.num_mipmaps, 1);
    def.usage = _sg_def(def.usage, SG_USAGE_IMMUTABLE);
    if (desc->render_target) {
        def.pixel_format = _sg_def(def.pixel_format, _sg.desc.context.color_format);
        def.sample_count = _sg_def(def.sample_count, _sg.desc.context.sample_count);
    }
    else {
        def.pixel_format = _sg_def(def.pixel_format, SG_PIXELFORMAT_RGBA8);
        def.sample_count = _sg_def(def.sample_count, 1);
    }
    def.min_filter = _sg_def(def.min_filter, SG_FILTER_NEAREST);
    def.mag_filter = _sg_def(def.mag_filter, SG_FILTER_NEAREST);
    def.wrap_u = _sg_def(def.wrap_u, SG_WRAP_REPEAT);
    def.wrap_v = _sg_def(def.wrap_v, SG_WRAP_REPEAT);
    def.wrap_w = _sg_def(def.wrap_w, SG_WRAP_REPEAT);
    def.border_color = _sg_def(def.border_color, SG_BORDERCOLOR_OPAQUE_BLACK);
    def.max_anisotropy = _sg_def(def.max_anisotropy, 1);
    def.max_lod = _sg_def_flt(def.max_lod, FLT_MAX);
    return def;
}

_SOKOL_PRIVATE sg_shader_desc _sg_shader_desc_defaults(const sg_shader_desc* desc) {
    sg_shader_desc def = *desc;
    #if defined(SOKOL_METAL)
        def.vs.entry = _sg_def(def.vs.entry, "_main");
        def.fs.entry = _sg_def(def.fs.entry, "_main");
    #else
        def.vs.entry = _sg_def(def.vs.entry, "main");
        def.fs.entry = _sg_def(def.fs.entry, "main");
    #endif
    #if defined(SOKOL_D3D11)
        if (def.vs.source) {
            def.vs.d3d11_target = _sg_def(def.vs.d3d11_target, "vs_4_0");
        }
        if (def.fs.source) {
            def.fs.d3d11_target = _sg_def(def.fs.d3d11_target, "ps_4_0");
        }
    #endif
    for (int stage_index = 0; stage_index < SG_NUM_SHADER_STAGES; stage_index++) {
        sg_shader_stage_desc* stage_desc = (stage_index == SG_SHADERSTAGE_VS)? &def.vs : &def.fs;
        for (int ub_index = 0; ub_index < SG_MAX_SHADERSTAGE_UBS; ub_index++) {
            sg_shader_uniform_block_desc* ub_desc = &stage_desc->uniform_blocks[ub_index];
            if (0 == ub_desc->size) {
                break;
            }
            ub_desc->layout = _sg_def(ub_desc->layout, SG_UNIFORMLAYOUT_NATIVE);
            for (int u_index = 0; u_index < SG_MAX_UB_MEMBERS; u_index++) {
                sg_shader_uniform_desc* u_desc = &ub_desc->uniforms[u_index];
                if (u_desc->type == SG_UNIFORMTYPE_INVALID) {
                    break;
                }
                u_desc->array_count = _sg_def(u_desc->array_count, 1);
            }
        }
        for (int img_index = 0; img_index < SG_MAX_SHADERSTAGE_IMAGES; img_index++) {
            sg_shader_image_desc* img_desc = &stage_desc->images[img_index];
            if (img_desc->image_type == _SG_IMAGETYPE_DEFAULT) {
                break;
            }
            img_desc->sampler_type = _sg_def(img_desc->sampler_type, SG_SAMPLERTYPE_FLOAT);
        }
    }
    return def;
}

_SOKOL_PRIVATE sg_pipeline_desc _sg_pipeline_desc_defaults(const sg_pipeline_desc* desc) {
    sg_pipeline_desc def = *desc;

    def.primitive_type = _sg_def(def.primitive_type, SG_PRIMITIVETYPE_TRIANGLES);
    def.index_type = _sg_def(def.index_type, SG_INDEXTYPE_NONE);
    def.cull_mode = _sg_def(def.cull_mode, SG_CULLMODE_NONE);
    def.face_winding = _sg_def(def.face_winding, SG_FACEWINDING_CW);
    def.sample_count = _sg_def(def.sample_count, _sg.desc.context.sample_count);

    def.stencil.front.compare = _sg_def(def.stencil.front.compare, SG_COMPAREFUNC_ALWAYS);
    def.stencil.front.fail_op = _sg_def(def.stencil.front.fail_op, SG_STENCILOP_KEEP);
    def.stencil.front.depth_fail_op = _sg_def(def.stencil.front.depth_fail_op, SG_STENCILOP_KEEP);
    def.stencil.front.pass_op = _sg_def(def.stencil.front.pass_op, SG_STENCILOP_KEEP);
    def.stencil.back.compare = _sg_def(def.stencil.back.compare, SG_COMPAREFUNC_ALWAYS);
    def.stencil.back.fail_op = _sg_def(def.stencil.back.fail_op, SG_STENCILOP_KEEP);
    def.stencil.back.depth_fail_op = _sg_def(def.stencil.back.depth_fail_op, SG_STENCILOP_KEEP);
    def.stencil.back.pass_op = _sg_def(def.stencil.back.pass_op, SG_STENCILOP_KEEP);

    def.depth.compare = _sg_def(def.depth.compare, SG_COMPAREFUNC_ALWAYS);
    def.depth.pixel_format = _sg_def(def.depth.pixel_format, _sg.desc.context.depth_format);
    def.color_count = _sg_def(def.color_count, 1);
    if (def.color_count > SG_MAX_COLOR_ATTACHMENTS) {
        def.color_count = SG_MAX_COLOR_ATTACHMENTS;
    }
    for (int i = 0; i < def.color_count; i++) {
        sg_color_state* cs = &def.colors[i];
        cs->pixel_format = _sg_def(cs->pixel_format, _sg.desc.context.color_format);
        cs->write_mask = _sg_def(cs->write_mask, SG_COLORMASK_RGBA);
        sg_blend_state* bs = &def.colors[i].blend;
        bs->src_factor_rgb = _sg_def(bs->src_factor_rgb, SG_BLENDFACTOR_ONE);
        bs->dst_factor_rgb = _sg_def(bs->dst_factor_rgb, SG_BLENDFACTOR_ZERO);
        bs->op_rgb = _sg_def(bs->op_rgb, SG_BLENDOP_ADD);
        bs->src_factor_alpha = _sg_def(bs->src_factor_alpha, SG_BLENDFACTOR_ONE);
        bs->dst_factor_alpha = _sg_def(bs->dst_factor_alpha, SG_BLENDFACTOR_ZERO);
        bs->op_alpha = _sg_def(bs->op_alpha, SG_BLENDOP_ADD);
    }

    for (int attr_index = 0; attr_index < SG_MAX_VERTEX_ATTRIBUTES; attr_index++) {
        sg_vertex_attr_desc* a_desc = &def.layout.attrs[attr_index];
        if (a_desc->format == SG_VERTEXFORMAT_INVALID) {
            break;
        }
        SOKOL_ASSERT(a_desc->buffer_index < SG_MAX_SHADERSTAGE_BUFFERS);
        sg_buffer_layout_desc* b_desc = &def.layout.buffers[a_desc->buffer_index];
        b_desc->step_func = _sg_def(b_desc->step_func, SG_VERTEXSTEP_PER_VERTEX);
        b_desc->step_rate = _sg_def(b_desc->step_rate, 1);
    }

    /* resolve vertex layout strides and offsets */
    int auto_offset[SG_MAX_SHADERSTAGE_BUFFERS];
    _sg_clear(auto_offset, sizeof(auto_offset));
    bool use_auto_offset = true;
    for (int attr_index = 0; attr_index < SG_MAX_VERTEX_ATTRIBUTES; attr_index++) {
        /* to use computed offsets, *all* attr offsets must be 0 */
        if (def.layout.attrs[attr_index].offset != 0) {
            use_auto_offset = false;
        }
    }
    for (int attr_index = 0; attr_index < SG_MAX_VERTEX_ATTRIBUTES; attr_index++) {
        sg_vertex_attr_desc* a_desc = &def.layout.attrs[attr_index];
        if (a_desc->format == SG_VERTEXFORMAT_INVALID) {
            break;
        }
        SOKOL_ASSERT(a_desc->buffer_index < SG_MAX_SHADERSTAGE_BUFFERS);
        if (use_auto_offset) {
            a_desc->offset = auto_offset[a_desc->buffer_index];
        }
        auto_offset[a_desc->buffer_index] += _sg_vertexformat_bytesize(a_desc->format);
    }
    /* compute vertex strides if needed */
    for (int buf_index = 0; buf_index < SG_MAX_SHADERSTAGE_BUFFERS; buf_index++) {
        sg_buffer_layout_desc* l_desc = &def.layout.buffers[buf_index];
        if (l_desc->stride == 0) {
            l_desc->stride = auto_offset[buf_index];
        }
    }

    return def;
}

_SOKOL_PRIVATE sg_pass_desc _sg_pass_desc_defaults(const sg_pass_desc* desc) {
    /* FIXME: no values to replace in sg_pass_desc? */
    sg_pass_desc def = *desc;
    return def;
}

/*== allocate/initialize resource private functions ==========================*/
_SOKOL_PRIVATE sg_buffer _sg_alloc_buffer(void) {
    sg_buffer res;
    int slot_index = _sg_pool_alloc_index(&_sg.pools.buffer_pool);
    if (_SG_INVALID_SLOT_INDEX != slot_index) {
        res.id = _sg_slot_alloc(&_sg.pools.buffer_pool, &_sg.pools.buffers[slot_index].slot, slot_index);
    }
    else {
        /* pool is exhausted */
        res.id = SG_INVALID_ID;
    }
    return res;
}

_SOKOL_PRIVATE sg_image _sg_alloc_image(void) {
    sg_image res;
    int slot_index = _sg_pool_alloc_index(&_sg.pools.image_pool);
    if (_SG_INVALID_SLOT_INDEX != slot_index) {
        res.id = _sg_slot_alloc(&_sg.pools.image_pool, &_sg.pools.images[slot_index].slot, slot_index);
    }
    else {
        /* pool is exhausted */
        res.id = SG_INVALID_ID;
    }
    return res;
}

_SOKOL_PRIVATE sg_shader _sg_alloc_shader(void) {
    sg_shader res;
    int slot_index = _sg_pool_alloc_index(&_sg.pools.shader_pool);
    if (_SG_INVALID_SLOT_INDEX != slot_index) {
        res.id = _sg_slot_alloc(&_sg.pools.shader_pool, &_sg.pools.shaders[slot_index].slot, slot_index);
    }
    else {
        /* pool is exhausted */
        res.id = SG_INVALID_ID;
    }
    return res;
}

_SOKOL_PRIVATE sg_pipeline _sg_alloc_pipeline(void) {
    sg_pipeline res;
    int slot_index = _sg_pool_alloc_index(&_sg.pools.pipeline_pool);
    if (_SG_INVALID_SLOT_INDEX != slot_index) {
        res.id =_sg_slot_alloc(&_sg.pools.pipeline_pool, &_sg.pools.pipelines[slot_index].slot, slot_index);
    }
    else {
        /* pool is exhausted */
        res.id = SG_INVALID_ID;
    }
    return res;
}

_SOKOL_PRIVATE sg_pass _sg_alloc_pass(void) {
    sg_pass res;
    int slot_index = _sg_pool_alloc_index(&_sg.pools.pass_pool);
    if (_SG_INVALID_SLOT_INDEX != slot_index) {
        res.id = _sg_slot_alloc(&_sg.pools.pass_pool, &_sg.pools.passes[slot_index].slot, slot_index);
    }
    else {
        /* pool is exhausted */
        res.id = SG_INVALID_ID;
    }
    return res;
}

_SOKOL_PRIVATE void _sg_dealloc_buffer(_sg_buffer_t* buf) {
    SOKOL_ASSERT(buf && (buf->slot.state == SG_RESOURCESTATE_ALLOC) && (buf->slot.id != SG_INVALID_ID));
    _sg_pool_free_index(&_sg.pools.buffer_pool, _sg_slot_index(buf->slot.id));
    _sg_reset_slot(&buf->slot);
}

_SOKOL_PRIVATE void _sg_dealloc_image(_sg_image_t* img) {
    SOKOL_ASSERT(img && (img->slot.state == SG_RESOURCESTATE_ALLOC) && (img->slot.id != SG_INVALID_ID));
    _sg_pool_free_index(&_sg.pools.image_pool, _sg_slot_index(img->slot.id));
    _sg_reset_slot(&img->slot);
}

_SOKOL_PRIVATE void _sg_dealloc_shader(_sg_shader_t* shd) {
    SOKOL_ASSERT(shd && (shd->slot.state == SG_RESOURCESTATE_ALLOC) && (shd->slot.id != SG_INVALID_ID));
    _sg_pool_free_index(&_sg.pools.shader_pool, _sg_slot_index(shd->slot.id));
    _sg_reset_slot(&shd->slot);
}

_SOKOL_PRIVATE void _sg_dealloc_pipeline(_sg_pipeline_t* pip) {
    SOKOL_ASSERT(pip && (pip->slot.state == SG_RESOURCESTATE_ALLOC) && (pip->slot.id != SG_INVALID_ID));
    _sg_pool_free_index(&_sg.pools.pipeline_pool, _sg_slot_index(pip->slot.id));
    _sg_reset_slot(&pip->slot);
}

_SOKOL_PRIVATE void _sg_dealloc_pass(_sg_pass_t* pass) {
    SOKOL_ASSERT(pass && (pass->slot.state == SG_RESOURCESTATE_ALLOC) && (pass->slot.id != SG_INVALID_ID));
    _sg_pool_free_index(&_sg.pools.pass_pool, _sg_slot_index(pass->slot.id));
    _sg_reset_slot(&pass->slot);
}

_SOKOL_PRIVATE void _sg_init_buffer(_sg_buffer_t* buf, const sg_buffer_desc* desc) {
    SOKOL_ASSERT(buf && (buf->slot.state == SG_RESOURCESTATE_ALLOC));
    SOKOL_ASSERT(desc);
    buf->slot.ctx_id = _sg.active_context.id;
    if (_sg_validate_buffer_desc(desc)) {
        buf->slot.state = _sg_create_buffer(buf, desc);
    }
    else {
        buf->slot.state = SG_RESOURCESTATE_FAILED;
    }
    SOKOL_ASSERT((buf->slot.state == SG_RESOURCESTATE_VALID)||(buf->slot.state == SG_RESOURCESTATE_FAILED));
}

_SOKOL_PRIVATE void _sg_init_image(_sg_image_t* img, const sg_image_desc* desc) {
    SOKOL_ASSERT(img && (img->slot.state == SG_RESOURCESTATE_ALLOC));
    SOKOL_ASSERT(desc);
    img->slot.ctx_id = _sg.active_context.id;
    if (_sg_validate_image_desc(desc)) {
        img->slot.state = _sg_create_image(img, desc);
    }
    else {
        img->slot.state = SG_RESOURCESTATE_FAILED;
    }
    SOKOL_ASSERT((img->slot.state == SG_RESOURCESTATE_VALID)||(img->slot.state == SG_RESOURCESTATE_FAILED));
}

_SOKOL_PRIVATE void _sg_init_shader(_sg_shader_t* shd, const sg_shader_desc* desc) {
    SOKOL_ASSERT(shd && (shd->slot.state == SG_RESOURCESTATE_ALLOC));
    SOKOL_ASSERT(desc);
    shd->slot.ctx_id = _sg.active_context.id;
    if (_sg_validate_shader_desc(desc)) {
        shd->slot.state = _sg_create_shader(shd, desc);
    }
    else {
        shd->slot.state = SG_RESOURCESTATE_FAILED;
    }
    SOKOL_ASSERT((shd->slot.state == SG_RESOURCESTATE_VALID)||(shd->slot.state == SG_RESOURCESTATE_FAILED));
}

_SOKOL_PRIVATE void _sg_init_pipeline(_sg_pipeline_t* pip, const sg_pipeline_desc* desc) {
    SOKOL_ASSERT(pip && (pip->slot.state == SG_RESOURCESTATE_ALLOC));
    SOKOL_ASSERT(desc);
    pip->slot.ctx_id = _sg.active_context.id;
    if (_sg_validate_pipeline_desc(desc)) {
        _sg_shader_t* shd = _sg_lookup_shader(&_sg.pools, desc->shader.id);
        if (shd && (shd->slot.state == SG_RESOURCESTATE_VALID)) {
            pip->slot.state = _sg_create_pipeline(pip, shd, desc);
        }
        else {
            pip->slot.state = SG_RESOURCESTATE_FAILED;
        }
    }
    else {
        pip->slot.state = SG_RESOURCESTATE_FAILED;
    }
    SOKOL_ASSERT((pip->slot.state == SG_RESOURCESTATE_VALID)||(pip->slot.state == SG_RESOURCESTATE_FAILED));
}

_SOKOL_PRIVATE void _sg_init_pass(_sg_pass_t* pass, const sg_pass_desc* desc) {
    SOKOL_ASSERT(pass && pass->slot.state == SG_RESOURCESTATE_ALLOC);
    SOKOL_ASSERT(desc);
    pass->slot.ctx_id = _sg.active_context.id;
    if (_sg_validate_pass_desc(desc)) {
        /* lookup pass attachment image pointers */
        _sg_image_t* att_imgs[SG_MAX_COLOR_ATTACHMENTS + 1];
        for (int i = 0; i < SG_MAX_COLOR_ATTACHMENTS; i++) {
            if (desc->color_attachments[i].image.id) {
                att_imgs[i] = _sg_lookup_image(&_sg.pools, desc->color_attachments[i].image.id);
                if (!(att_imgs[i] && att_imgs[i]->slot.state == SG_RESOURCESTATE_VALID)) {
                    pass->slot.state = SG_RESOURCESTATE_FAILED;
                    return;
                }
            }
            else {
                att_imgs[i] = 0;
            }
        }
        const int ds_att_index = SG_MAX_COLOR_ATTACHMENTS;
        if (desc->depth_stencil_attachment.image.id) {
            att_imgs[ds_att_index] = _sg_lookup_image(&_sg.pools, desc->depth_stencil_attachment.image.id);
            if (!(att_imgs[ds_att_index] && att_imgs[ds_att_index]->slot.state == SG_RESOURCESTATE_VALID)) {
                pass->slot.state = SG_RESOURCESTATE_FAILED;
                return;
            }
        }
        else {
            att_imgs[ds_att_index] = 0;
        }
        pass->slot.state = _sg_create_pass(pass, att_imgs, desc);
    }
    else {
        pass->slot.state = SG_RESOURCESTATE_FAILED;
    }
    SOKOL_ASSERT((pass->slot.state == SG_RESOURCESTATE_VALID)||(pass->slot.state == SG_RESOURCESTATE_FAILED));
}

_SOKOL_PRIVATE void _sg_uninit_buffer(_sg_buffer_t* buf) {
    SOKOL_ASSERT(buf && ((buf->slot.state == SG_RESOURCESTATE_VALID) || (buf->slot.state == SG_RESOURCESTATE_FAILED)));
    if (buf->slot.ctx_id == _sg.active_context.id) {
        _sg_discard_buffer(buf);
        _sg_reset_buffer_to_alloc_state(buf);
    }
    else {
        SG_LOG("_sg_uninit_buffer: active context mismatch (must be same as for creation)");
        _SG_TRACE_NOARGS(err_context_mismatch);
    }
}

_SOKOL_PRIVATE void _sg_uninit_image(_sg_image_t* img) {
    SOKOL_ASSERT(img && ((img->slot.state == SG_RESOURCESTATE_VALID) || (img->slot.state == SG_RESOURCESTATE_FAILED)));
    if (img->slot.ctx_id == _sg.active_context.id) {
        _sg_discard_image(img);
        _sg_reset_image_to_alloc_state(img);
    }
    else {
        SG_LOG("_sg_uninit_image: active context mismatch (must be same as for creation)");
        _SG_TRACE_NOARGS(err_context_mismatch);
    }
}

_SOKOL_PRIVATE void _sg_uninit_shader(_sg_shader_t* shd) {
    SOKOL_ASSERT(shd && ((shd->slot.state == SG_RESOURCESTATE_VALID) || (shd->slot.state == SG_RESOURCESTATE_FAILED)));
    if (shd->slot.ctx_id == _sg.active_context.id) {
        _sg_discard_shader(shd);
        _sg_reset_shader_to_alloc_state(shd);
    }
    else {
        SG_LOG("_sg_uninit_shader: active context mismatch (must be same as for creation)");
        _SG_TRACE_NOARGS(err_context_mismatch);
    }
}

_SOKOL_PRIVATE void _sg_uninit_pipeline(_sg_pipeline_t* pip) {
    SOKOL_ASSERT(pip && ((pip->slot.state == SG_RESOURCESTATE_VALID) || (pip->slot.state == SG_RESOURCESTATE_FAILED)));
    if (pip->slot.ctx_id == _sg.active_context.id) {
        _sg_discard_pipeline(pip);
        _sg_reset_pipeline_to_alloc_state(pip);
    }
    else {
        SG_LOG("_sg_uninit_pipeline: active context mismatch (must be same as for creation)");
        _SG_TRACE_NOARGS(err_context_mismatch);
    }
}

_SOKOL_PRIVATE void _sg_uninit_pass(_sg_pass_t* pass) {
    SOKOL_ASSERT(pass && ((pass->slot.state == SG_RESOURCESTATE_VALID) || (pass->slot.state == SG_RESOURCESTATE_FAILED)));
    if (pass->slot.ctx_id == _sg.active_context.id) {
        _sg_discard_pass(pass);
        _sg_reset_pass_to_alloc_state(pass);
    }
    else {
        SG_LOG("_sg_uninit_pass: active context mismatch (must be same as for creation)");
        _SG_TRACE_NOARGS(err_context_mismatch);
    }
}

_SOKOL_PRIVATE void _sg_setup_commit_listeners(const sg_desc* desc) {
    SOKOL_ASSERT(desc->max_commit_listeners > 0);
    SOKOL_ASSERT(0 == _sg.commit_listeners.items);
    SOKOL_ASSERT(0 == _sg.commit_listeners.num);
    SOKOL_ASSERT(0 == _sg.commit_listeners.upper);
    _sg.commit_listeners.num = desc->max_commit_listeners;
    const size_t size = (size_t)_sg.commit_listeners.num * sizeof(sg_commit_listener);
    _sg.commit_listeners.items = (sg_commit_listener*)_sg_malloc_clear(size);
}

_SOKOL_PRIVATE void _sg_discard_commit_listeners(void) {
    SOKOL_ASSERT(0 != _sg.commit_listeners.items);
    _sg_free(_sg.commit_listeners.items);
    _sg.commit_listeners.items = 0;
}

_SOKOL_PRIVATE void _sg_notify_commit_listeners(void) {
    SOKOL_ASSERT(_sg.commit_listeners.items);
    for (int i = 0; i < _sg.commit_listeners.upper; i++) {
        const sg_commit_listener* listener = &_sg.commit_listeners.items[i];
        if (listener->func) {
            listener->func(listener->user_data);
        }
    }
}

_SOKOL_PRIVATE bool _sg_add_commit_listener(const sg_commit_listener* new_listener) {
    SOKOL_ASSERT(new_listener && new_listener->func);
    SOKOL_ASSERT(_sg.commit_listeners.items);
    // first check if the listener hadn't been added already
    for (int i = 0; i < _sg.commit_listeners.upper; i++) {
        const sg_commit_listener* slot = &_sg.commit_listeners.items[i];
        if ((slot->func == new_listener->func) && (slot->user_data == new_listener->user_data)) {
            SG_LOG("attempting to add identical commit listener\n");
            return false;
        }
    }
    // first try to plug a hole
    sg_commit_listener* slot = 0;
    for (int i = 0; i < _sg.commit_listeners.upper; i++) {
        if (_sg.commit_listeners.items[i].func == 0) {
            slot = &_sg.commit_listeners.items[i];
            break;
        }
    }
    if (!slot) {
        // append to end
        if (_sg.commit_listeners.upper < _sg.commit_listeners.num) {
            slot = &_sg.commit_listeners.items[_sg.commit_listeners.upper++];
        }
    }
    if (!slot) {
        SG_LOG("commit listener array full\n");
        return false;
    }
    *slot = *new_listener;
    return true;
}

_SOKOL_PRIVATE bool _sg_remove_commit_listener(const sg_commit_listener* listener) {
    SOKOL_ASSERT(listener && listener->func);
    SOKOL_ASSERT(_sg.commit_listeners.items);
    for (int i = 0; i < _sg.commit_listeners.upper; i++) {
        sg_commit_listener* slot = &_sg.commit_listeners.items[i];
        // both the function pointer and user data must match!
        if ((slot->func == listener->func) && (slot->user_data == listener->user_data)) {
            slot->func = 0;
            slot->user_data = 0;
            // NOTE: since _sg_add_commit_listener() already catches duplicates,
            // we don't need to worry about them here
            return true;
        }
    }
    return false;
}

_SOKOL_PRIVATE sg_desc _sg_desc_defaults(const sg_desc* desc) {
    /*
        NOTE: on WebGPU, the default color pixel format MUST be provided,
        cannot be a default compile-time constant.
    */
    sg_desc res = *desc;
    #if defined(SOKOL_WGPU)
        SOKOL_ASSERT(SG_PIXELFORMAT_NONE != res.context.color_format);
    #elif defined(SOKOL_METAL) || defined(SOKOL_D3D11)
        res.context.color_format = _sg_def(res.context.color_format, SG_PIXELFORMAT_BGRA8);
    #else
        res.context.color_format = _sg_def(res.context.color_format, SG_PIXELFORMAT_RGBA8);
    #endif
    res.context.depth_format = _sg_def(res.context.depth_format, SG_PIXELFORMAT_DEPTH_STENCIL);
    res.context.sample_count = _sg_def(res.context.sample_count, 1);
    res.buffer_pool_size = _sg_def(res.buffer_pool_size, _SG_DEFAULT_BUFFER_POOL_SIZE);
    res.image_pool_size = _sg_def(res.image_pool_size, _SG_DEFAULT_IMAGE_POOL_SIZE);
    res.shader_pool_size = _sg_def(res.shader_pool_size, _SG_DEFAULT_SHADER_POOL_SIZE);
    res.pipeline_pool_size = _sg_def(res.pipeline_pool_size, _SG_DEFAULT_PIPELINE_POOL_SIZE);
    res.pass_pool_size = _sg_def(res.pass_pool_size, _SG_DEFAULT_PASS_POOL_SIZE);
    res.context_pool_size = _sg_def(res.context_pool_size, _SG_DEFAULT_CONTEXT_POOL_SIZE);
    res.uniform_buffer_size = _sg_def(res.uniform_buffer_size, _SG_DEFAULT_UB_SIZE);
    res.staging_buffer_size = _sg_def(res.staging_buffer_size, _SG_DEFAULT_STAGING_SIZE);
    res.sampler_cache_size = _sg_def(res.sampler_cache_size, _SG_DEFAULT_SAMPLER_CACHE_CAPACITY);
    res.max_commit_listeners = _sg_def(res.max_commit_listeners, _SG_DEFAULT_MAX_COMMIT_LISTENERS);
    return res;
}

/*== PUBLIC API FUNCTIONS ====================================================*/

SOKOL_API_IMPL void sg_setup(const sg_desc* desc) {
    SOKOL_ASSERT(desc);
    SOKOL_ASSERT((desc->_start_canary == 0) && (desc->_end_canary == 0));
    SOKOL_ASSERT((desc->allocator.alloc && desc->allocator.free) || (!desc->allocator.alloc && !desc->allocator.free));
    _SG_CLEAR_ARC_STRUCT(_sg_state_t, _sg);
    _sg.desc = _sg_desc_defaults(desc);
    _sg_setup_pools(&_sg.pools, &_sg.desc);
    _sg_setup_commit_listeners(&_sg.desc);
    _sg.frame_index = 1;
    _sg_setup_backend(&_sg.desc);
    _sg.valid = true;
    sg_setup_context();
}

SOKOL_API_IMPL void sg_shutdown(void) {
    /* can only delete resources for the currently set context here, if multiple
    contexts are used, the app code must take care of properly releasing them
    (since only the app code can switch between 3D-API contexts)
    */
    if (_sg.active_context.id != SG_INVALID_ID) {
        _sg_context_t* ctx = _sg_lookup_context(&_sg.pools, _sg.active_context.id);
        if (ctx) {
            _sg_discard_all_resources(&_sg.pools, _sg.active_context.id);
            _sg_discard_context(ctx);
        }
    }
    _sg_discard_backend();
    _sg_discard_commit_listeners();
    _sg_discard_pools(&_sg.pools);
    _SG_CLEAR_ARC_STRUCT(_sg_state_t, _sg);
}

SOKOL_API_IMPL bool sg_isvalid(void) {
    return _sg.valid;
}

SOKOL_API_IMPL sg_desc sg_query_desc(void) {
    SOKOL_ASSERT(_sg.valid);
    return _sg.desc;
}

SOKOL_API_IMPL sg_backend sg_query_backend(void) {
    SOKOL_ASSERT(_sg.valid);
    return _sg.backend;
}

SOKOL_API_IMPL sg_features sg_query_features(void) {
    SOKOL_ASSERT(_sg.valid);
    return _sg.features;
}

SOKOL_API_IMPL sg_limits sg_query_limits(void) {
    SOKOL_ASSERT(_sg.valid);
    return _sg.limits;
}

SOKOL_API_IMPL sg_pixelformat_info sg_query_pixelformat(sg_pixel_format fmt) {
    SOKOL_ASSERT(_sg.valid);
    int fmt_index = (int) fmt;
    SOKOL_ASSERT((fmt_index > SG_PIXELFORMAT_NONE) && (fmt_index < _SG_PIXELFORMAT_NUM));
    return _sg.formats[fmt_index];
}

SOKOL_API_IMPL sg_context sg_setup_context(void) {
    SOKOL_ASSERT(_sg.valid);
    sg_context res;
    int slot_index = _sg_pool_alloc_index(&_sg.pools.context_pool);
    if (_SG_INVALID_SLOT_INDEX != slot_index) {
        res.id = _sg_slot_alloc(&_sg.pools.context_pool, &_sg.pools.contexts[slot_index].slot, slot_index);
        _sg_context_t* ctx = _sg_context_at(&_sg.pools, res.id);
        ctx->slot.state = _sg_create_context(ctx);
        SOKOL_ASSERT(ctx->slot.state == SG_RESOURCESTATE_VALID);
        _sg_activate_context(ctx);
    }
    else {
        /* pool is exhausted */
        res.id = SG_INVALID_ID;
    }
    _sg.active_context = res;
    return res;
}

SOKOL_API_IMPL void sg_discard_context(sg_context ctx_id) {
    SOKOL_ASSERT(_sg.valid);
    _sg_discard_all_resources(&_sg.pools, ctx_id.id);
    _sg_context_t* ctx = _sg_lookup_context(&_sg.pools, ctx_id.id);
    if (ctx) {
        _sg_discard_context(ctx);
        _sg_reset_context_to_alloc_state(ctx);
        _sg_reset_slot(&ctx->slot);
        _sg_pool_free_index(&_sg.pools.context_pool, _sg_slot_index(ctx_id.id));
    }
    _sg.active_context.id = SG_INVALID_ID;
    _sg_activate_context(0);
}

SOKOL_API_IMPL void sg_activate_context(sg_context ctx_id) {
    SOKOL_ASSERT(_sg.valid);
    _sg.active_context = ctx_id;
    _sg_context_t* ctx = _sg_lookup_context(&_sg.pools, ctx_id.id);
    /* NOTE: ctx can be 0 here if the context is no longer valid */
    _sg_activate_context(ctx);
}

SOKOL_API_IMPL sg_trace_hooks sg_install_trace_hooks(const sg_trace_hooks* trace_hooks) {
    SOKOL_ASSERT(_sg.valid);
    SOKOL_ASSERT(trace_hooks);
    _SOKOL_UNUSED(trace_hooks);
    #if defined(SOKOL_TRACE_HOOKS)
        sg_trace_hooks old_hooks = _sg.hooks;
        _sg.hooks = *trace_hooks;
    #else
        static sg_trace_hooks old_hooks;
        SG_LOG("sg_install_trace_hooks() called, but SG_TRACE_HOOKS is not defined!");
    #endif
    return old_hooks;
}

SOKOL_API_IMPL sg_buffer sg_alloc_buffer(void) {
    SOKOL_ASSERT(_sg.valid);
    sg_buffer res = _sg_alloc_buffer();
    _SG_TRACE_ARGS(alloc_buffer, res);
    return res;
}

SOKOL_API_IMPL sg_image sg_alloc_image(void) {
    SOKOL_ASSERT(_sg.valid);
    sg_image res = _sg_alloc_image();
    _SG_TRACE_ARGS(alloc_image, res);
    return res;
}

SOKOL_API_IMPL sg_shader sg_alloc_shader(void) {
    SOKOL_ASSERT(_sg.valid);
    sg_shader res = _sg_alloc_shader();
    _SG_TRACE_ARGS(alloc_shader, res);
    return res;
}

SOKOL_API_IMPL sg_pipeline sg_alloc_pipeline(void) {
    SOKOL_ASSERT(_sg.valid);
    sg_pipeline res = _sg_alloc_pipeline();
    _SG_TRACE_ARGS(alloc_pipeline, res);
    return res;
}

SOKOL_API_IMPL sg_pass sg_alloc_pass(void) {
    SOKOL_ASSERT(_sg.valid);
    sg_pass res = _sg_alloc_pass();
    _SG_TRACE_ARGS(alloc_pass, res);
    return res;
}

SOKOL_API_IMPL void sg_dealloc_buffer(sg_buffer buf_id) {
    SOKOL_ASSERT(_sg.valid);
    _sg_buffer_t* buf = _sg_lookup_buffer(&_sg.pools, buf_id.id);
    if (buf) {
        if (buf->slot.state == SG_RESOURCESTATE_ALLOC) {
            _sg_dealloc_buffer(buf);
        }
        else {
            SG_LOG("sg_dealloc_buffer: buffer must be in ALLOC state\n");
        }
    }
    _SG_TRACE_ARGS(dealloc_buffer, buf_id);
}

SOKOL_API_IMPL void sg_dealloc_image(sg_image img_id) {
    SOKOL_ASSERT(_sg.valid);
    _sg_image_t* img = _sg_lookup_image(&_sg.pools, img_id.id);
    if (img) {
        if (img->slot.state == SG_RESOURCESTATE_ALLOC) {
            _sg_dealloc_image(img);
        }
        else {
            SG_LOG("sg_dealloc_image: image must be in ALLOC state\n");
        }
    }
    _SG_TRACE_ARGS(dealloc_image, img_id);
}

SOKOL_API_IMPL void sg_dealloc_shader(sg_shader shd_id) {
    SOKOL_ASSERT(_sg.valid);
    _sg_shader_t* shd = _sg_lookup_shader(&_sg.pools, shd_id.id);
    if (shd) {
        if (shd->slot.state == SG_RESOURCESTATE_ALLOC) {
            _sg_dealloc_shader(shd);
        }
        else {
            SG_LOG("sg_dealloc_shader: shader must be in ALLOC state\n");
        }
    }
    _SG_TRACE_ARGS(dealloc_shader, shd_id);
}

SOKOL_API_IMPL void sg_dealloc_pipeline(sg_pipeline pip_id) {
    SOKOL_ASSERT(_sg.valid);
    _sg_pipeline_t* pip = _sg_lookup_pipeline(&_sg.pools, pip_id.id);
    if (pip) {
        if (pip->slot.state == SG_RESOURCESTATE_ALLOC) {
            _sg_dealloc_pipeline(pip);
        }
        else {
            SG_LOG("sg_dealloc_pipeline: pipeline must be in ALLOC state\n");
        }
    }
    _SG_TRACE_ARGS(dealloc_pipeline, pip_id);
}

SOKOL_API_IMPL void sg_dealloc_pass(sg_pass pass_id) {
    SOKOL_ASSERT(_sg.valid);
    _sg_pass_t* pass = _sg_lookup_pass(&_sg.pools, pass_id.id);
    if (pass) {
        if (pass->slot.state == SG_RESOURCESTATE_ALLOC) {
            _sg_dealloc_pass(pass);
        }
        else {
            SG_LOG("sg_dealloc_pass: pass must be in ALLOC state\n");
        }
    }
    _SG_TRACE_ARGS(dealloc_pass, pass_id);
}

SOKOL_API_IMPL void sg_init_buffer(sg_buffer buf_id, const sg_buffer_desc* desc) {
    SOKOL_ASSERT(_sg.valid);
    sg_buffer_desc desc_def = _sg_buffer_desc_defaults(desc);
    _sg_buffer_t* buf = _sg_lookup_buffer(&_sg.pools, buf_id.id);
    if (buf) {
        if (buf->slot.state == SG_RESOURCESTATE_ALLOC) {
            _sg_init_buffer(buf, &desc_def);
            SOKOL_ASSERT((buf->slot.state == SG_RESOURCESTATE_VALID) || (buf->slot.state == SG_RESOURCESTATE_FAILED));
        }
        else {
            SG_LOG("sg_init_buffer: buffer must be in alloc state\n");
        }
    }
    _SG_TRACE_ARGS(init_buffer, buf_id, &desc_def);
}

SOKOL_API_IMPL void sg_init_image(sg_image img_id, const sg_image_desc* desc) {
    SOKOL_ASSERT(_sg.valid);
    sg_image_desc desc_def = _sg_image_desc_defaults(desc);
    _sg_image_t* img = _sg_lookup_image(&_sg.pools, img_id.id);
    if (img) {
        if (img->slot.state == SG_RESOURCESTATE_ALLOC) {
            _sg_init_image(img, &desc_def);
            SOKOL_ASSERT((img->slot.state == SG_RESOURCESTATE_VALID) || (img->slot.state == SG_RESOURCESTATE_FAILED));
        }
        else {
            SG_LOG("sg_init_image: image must be in alloc state\n");
        }
    }
    _SG_TRACE_ARGS(init_image, img_id, &desc_def);
}

SOKOL_API_IMPL void sg_init_shader(sg_shader shd_id, const sg_shader_desc* desc) {
    SOKOL_ASSERT(_sg.valid);
    sg_shader_desc desc_def = _sg_shader_desc_defaults(desc);
    _sg_shader_t* shd = _sg_lookup_shader(&_sg.pools, shd_id.id);
    if (shd) {
        if (shd->slot.state == SG_RESOURCESTATE_ALLOC) {
            _sg_init_shader(shd, &desc_def);
            SOKOL_ASSERT((shd->slot.state == SG_RESOURCESTATE_VALID) || (shd->slot.state == SG_RESOURCESTATE_FAILED));
        }
        else {
            SG_LOG("sg_init_shader: shader must be in alloc state\n");
        }
    }
    _SG_TRACE_ARGS(init_shader, shd_id, &desc_def);
}

SOKOL_API_IMPL void sg_init_pipeline(sg_pipeline pip_id, const sg_pipeline_desc* desc) {
    SOKOL_ASSERT(_sg.valid);
    sg_pipeline_desc desc_def = _sg_pipeline_desc_defaults(desc);
    _sg_pipeline_t* pip = _sg_lookup_pipeline(&_sg.pools, pip_id.id);
    if (pip) {
        if (pip->slot.state == SG_RESOURCESTATE_ALLOC) {
            _sg_init_pipeline(pip, &desc_def);
            SOKOL_ASSERT((pip->slot.state == SG_RESOURCESTATE_VALID) || (pip->slot.state == SG_RESOURCESTATE_FAILED));
        }
        else {
            SG_LOG("sg_init_pipeline: pipeline must be in alloc state\n");
        }
    }
    _SG_TRACE_ARGS(init_pipeline, pip_id, &desc_def);
}

SOKOL_API_IMPL void sg_init_pass(sg_pass pass_id, const sg_pass_desc* desc) {
    SOKOL_ASSERT(_sg.valid);
    sg_pass_desc desc_def = _sg_pass_desc_defaults(desc);
    _sg_pass_t* pass = _sg_lookup_pass(&_sg.pools, pass_id.id);
    if (pass) {
        if (pass->slot.state == SG_RESOURCESTATE_ALLOC) {
            _sg_init_pass(pass, &desc_def);
            SOKOL_ASSERT((pass->slot.state == SG_RESOURCESTATE_VALID) || (pass->slot.state == SG_RESOURCESTATE_FAILED));
        }
        else {
            SG_LOG("sg_init_pass: pass must be in alloc state\n");
        }
    }
    _SG_TRACE_ARGS(init_pass, pass_id, &desc_def);
}

SOKOL_API_IMPL void sg_uninit_buffer(sg_buffer buf_id) {
    SOKOL_ASSERT(_sg.valid);
    _sg_buffer_t* buf = _sg_lookup_buffer(&_sg.pools, buf_id.id);
    if (buf) {
        if ((buf->slot.state == SG_RESOURCESTATE_VALID) || (buf->slot.state == SG_RESOURCESTATE_FAILED)) {
            _sg_uninit_buffer(buf);
            SOKOL_ASSERT(buf->slot.state == SG_RESOURCESTATE_ALLOC);
        }
        else {
            SG_LOG("sg_uninit_buffer: buffer must be in VALID or FAILED state\n");
        }
    }
    _SG_TRACE_ARGS(uninit_buffer, buf_id);
}

SOKOL_API_IMPL void sg_uninit_image(sg_image img_id) {
    SOKOL_ASSERT(_sg.valid);
    _sg_image_t* img = _sg_lookup_image(&_sg.pools, img_id.id);
    if (img) {
        if ((img->slot.state == SG_RESOURCESTATE_VALID) || (img->slot.state == SG_RESOURCESTATE_FAILED)) {
            _sg_uninit_image(img);
            SOKOL_ASSERT(img->slot.state == SG_RESOURCESTATE_ALLOC);
        }
        else {
            SG_LOG("sg_uninit_image: image must be in VALID or FAILED state\n");
        }
    }
    _SG_TRACE_ARGS(uninit_image, img_id);
}

SOKOL_API_IMPL void sg_uninit_shader(sg_shader shd_id) {
    SOKOL_ASSERT(_sg.valid);
    _sg_shader_t* shd = _sg_lookup_shader(&_sg.pools, shd_id.id);
    if (shd) {
        if ((shd->slot.state == SG_RESOURCESTATE_VALID) || (shd->slot.state == SG_RESOURCESTATE_FAILED)) {
            _sg_uninit_shader(shd);
            SOKOL_ASSERT(shd->slot.state == SG_RESOURCESTATE_ALLOC);
        }
        else {
            SG_LOG("sg_uninit_shader: shader must be in VALID or FAILED state\n");
        }
    }
    _SG_TRACE_ARGS(uninit_shader, shd_id);
}

SOKOL_API_IMPL void sg_uninit_pipeline(sg_pipeline pip_id) {
    SOKOL_ASSERT(_sg.valid);
    _sg_pipeline_t* pip = _sg_lookup_pipeline(&_sg.pools, pip_id.id);
    if (pip) {
        if ((pip->slot.state == SG_RESOURCESTATE_VALID) || (pip->slot.state == SG_RESOURCESTATE_FAILED)) {
            _sg_uninit_pipeline(pip);
            SOKOL_ASSERT(pip->slot.state == SG_RESOURCESTATE_ALLOC);
        }
        else {
            SG_LOG("sg_uninit_pipeline: pipeline must be in VALID or FAILED state\n");
        }
    }
    _SG_TRACE_ARGS(uninit_pipeline, pip_id);
}

SOKOL_API_IMPL void sg_uninit_pass(sg_pass pass_id) {
    SOKOL_ASSERT(_sg.valid);
    _sg_pass_t* pass = _sg_lookup_pass(&_sg.pools, pass_id.id);
    if (pass) {
        if ((pass->slot.state == SG_RESOURCESTATE_VALID) || (pass->slot.state == SG_RESOURCESTATE_FAILED)) {
            _sg_uninit_pass(pass);
            SOKOL_ASSERT(pass->slot.state == SG_RESOURCESTATE_ALLOC);
        }
        else {
            SG_LOG("sg_uninit_pass: pass must be in VALID or FAILED state\n");
        }
    }
    _SG_TRACE_ARGS(uninit_pass, pass_id);
}

/*-- set allocated resource to failed state ----------------------------------*/
SOKOL_API_IMPL void sg_fail_buffer(sg_buffer buf_id) {
    SOKOL_ASSERT(_sg.valid);
    _sg_buffer_t* buf = _sg_lookup_buffer(&_sg.pools, buf_id.id);
    if (buf) {
        if (buf->slot.state == SG_RESOURCESTATE_ALLOC) {
            buf->slot.ctx_id = _sg.active_context.id;
            buf->slot.state = SG_RESOURCESTATE_FAILED;
        }
        else {
            SG_LOG("sg_fail_buffer: buffer must be in ALLOC state\n");
        }
    }
    _SG_TRACE_ARGS(fail_buffer, buf_id);
}

SOKOL_API_IMPL void sg_fail_image(sg_image img_id) {
    SOKOL_ASSERT(_sg.valid);
    _sg_image_t* img = _sg_lookup_image(&_sg.pools, img_id.id);
    if (img) {
        if (img->slot.state == SG_RESOURCESTATE_ALLOC) {
            img->slot.ctx_id = _sg.active_context.id;
            img->slot.state = SG_RESOURCESTATE_FAILED;
        }
        else {
            SG_LOG("sg_fail_image: image must be in ALLOC state\n");
        }
    }
    _SG_TRACE_ARGS(fail_image, img_id);
}

SOKOL_API_IMPL void sg_fail_shader(sg_shader shd_id) {
    SOKOL_ASSERT(_sg.valid);
    _sg_shader_t* shd = _sg_lookup_shader(&_sg.pools, shd_id.id);
    if (shd) {
        if (shd->slot.state == SG_RESOURCESTATE_ALLOC) {
            shd->slot.ctx_id = _sg.active_context.id;
            shd->slot.state = SG_RESOURCESTATE_FAILED;
        }
        else {
            SG_LOG("sg_fail_shader: shader must be in ALLOC state\n");
        }
    }
    _SG_TRACE_ARGS(fail_shader, shd_id);
}

SOKOL_API_IMPL void sg_fail_pipeline(sg_pipeline pip_id) {
    SOKOL_ASSERT(_sg.valid);
    _sg_pipeline_t* pip = _sg_lookup_pipeline(&_sg.pools, pip_id.id);
    if (pip) {
        if (pip->slot.state == SG_RESOURCESTATE_ALLOC) {
            pip->slot.ctx_id = _sg.active_context.id;
            pip->slot.state = SG_RESOURCESTATE_FAILED;
        }
        else {
            SG_LOG("sg_fail_pipeline: pipeline must be in ALLOC state\n");
        }
    }
    _SG_TRACE_ARGS(fail_pipeline, pip_id);
}

SOKOL_API_IMPL void sg_fail_pass(sg_pass pass_id) {
    SOKOL_ASSERT(_sg.valid);
    _sg_pass_t* pass = _sg_lookup_pass(&_sg.pools, pass_id.id);
    if (pass) {
        if (pass->slot.state == SG_RESOURCESTATE_ALLOC) {
            pass->slot.ctx_id = _sg.active_context.id;
            pass->slot.state = SG_RESOURCESTATE_FAILED;
        }
        else {
            SG_LOG("sg_fail_pass: pass must be in ALLOC state\n");
        }
    }
    _SG_TRACE_ARGS(fail_pass, pass_id);
}

/*-- get resource state */
SOKOL_API_IMPL sg_resource_state sg_query_buffer_state(sg_buffer buf_id) {
    SOKOL_ASSERT(_sg.valid);
    _sg_buffer_t* buf = _sg_lookup_buffer(&_sg.pools, buf_id.id);
    sg_resource_state res = buf ? buf->slot.state : SG_RESOURCESTATE_INVALID;
    return res;
}

SOKOL_API_IMPL sg_resource_state sg_query_image_state(sg_image img_id) {
    SOKOL_ASSERT(_sg.valid);
    _sg_image_t* img = _sg_lookup_image(&_sg.pools, img_id.id);
    sg_resource_state res = img ? img->slot.state : SG_RESOURCESTATE_INVALID;
    return res;
}

SOKOL_API_IMPL sg_resource_state sg_query_shader_state(sg_shader shd_id) {
    SOKOL_ASSERT(_sg.valid);
    _sg_shader_t* shd = _sg_lookup_shader(&_sg.pools, shd_id.id);
    sg_resource_state res = shd ? shd->slot.state : SG_RESOURCESTATE_INVALID;
    return res;
}

SOKOL_API_IMPL sg_resource_state sg_query_pipeline_state(sg_pipeline pip_id) {
    SOKOL_ASSERT(_sg.valid);
    _sg_pipeline_t* pip = _sg_lookup_pipeline(&_sg.pools, pip_id.id);
    sg_resource_state res = pip ? pip->slot.state : SG_RESOURCESTATE_INVALID;
    return res;
}

SOKOL_API_IMPL sg_resource_state sg_query_pass_state(sg_pass pass_id) {
    SOKOL_ASSERT(_sg.valid);
    _sg_pass_t* pass = _sg_lookup_pass(&_sg.pools, pass_id.id);
    sg_resource_state res = pass ? pass->slot.state : SG_RESOURCESTATE_INVALID;
    return res;
}

/*-- allocate and initialize resource ----------------------------------------*/
SOKOL_API_IMPL sg_buffer sg_make_buffer(const sg_buffer_desc* desc) {
    SOKOL_ASSERT(_sg.valid);
    SOKOL_ASSERT(desc);
    sg_buffer_desc desc_def = _sg_buffer_desc_defaults(desc);
    sg_buffer buf_id = _sg_alloc_buffer();
    if (buf_id.id != SG_INVALID_ID) {
        _sg_buffer_t* buf = _sg_buffer_at(&_sg.pools, buf_id.id);
        SOKOL_ASSERT(buf && (buf->slot.state == SG_RESOURCESTATE_ALLOC));
        _sg_init_buffer(buf, &desc_def);
        SOKOL_ASSERT((buf->slot.state == SG_RESOURCESTATE_VALID) || (buf->slot.state == SG_RESOURCESTATE_FAILED));
    }
    else {
        SG_LOG("buffer pool exhausted!");
        _SG_TRACE_NOARGS(err_buffer_pool_exhausted);
    }
    _SG_TRACE_ARGS(make_buffer, &desc_def, buf_id);
    return buf_id;
}

SOKOL_API_IMPL sg_image sg_make_image(const sg_image_desc* desc) {
    SOKOL_ASSERT(_sg.valid);
    SOKOL_ASSERT(desc);
    sg_image_desc desc_def = _sg_image_desc_defaults(desc);
    sg_image img_id = _sg_alloc_image();
    if (img_id.id != SG_INVALID_ID) {
        _sg_image_t* img = _sg_image_at(&_sg.pools, img_id.id);
        SOKOL_ASSERT(img && (img->slot.state == SG_RESOURCESTATE_ALLOC));
        _sg_init_image(img, &desc_def);
        SOKOL_ASSERT((img->slot.state == SG_RESOURCESTATE_VALID) || (img->slot.state == SG_RESOURCESTATE_FAILED));
    }
    else {
        SG_LOG("image pool exhausted!");
        _SG_TRACE_NOARGS(err_image_pool_exhausted);
    }
    _SG_TRACE_ARGS(make_image, &desc_def, img_id);
    return img_id;
}

SOKOL_API_IMPL sg_shader sg_make_shader(const sg_shader_desc* desc) {
    SOKOL_ASSERT(_sg.valid);
    SOKOL_ASSERT(desc);
    sg_shader_desc desc_def = _sg_shader_desc_defaults(desc);
    sg_shader shd_id = _sg_alloc_shader();
    if (shd_id.id != SG_INVALID_ID) {
        _sg_shader_t* shd = _sg_shader_at(&_sg.pools, shd_id.id);
        SOKOL_ASSERT(shd && (shd->slot.state == SG_RESOURCESTATE_ALLOC));
        _sg_init_shader(shd, &desc_def);
        SOKOL_ASSERT((shd->slot.state == SG_RESOURCESTATE_VALID) || (shd->slot.state == SG_RESOURCESTATE_FAILED));
    }
    else {
        SG_LOG("shader pool exhausted!");
        _SG_TRACE_NOARGS(err_shader_pool_exhausted);
    }
    _SG_TRACE_ARGS(make_shader, &desc_def, shd_id);
    return shd_id;
}

SOKOL_API_IMPL sg_pipeline sg_make_pipeline(const sg_pipeline_desc* desc) {
    SOKOL_ASSERT(_sg.valid);
    SOKOL_ASSERT(desc);
    sg_pipeline_desc desc_def = _sg_pipeline_desc_defaults(desc);
    sg_pipeline pip_id = _sg_alloc_pipeline();
    if (pip_id.id != SG_INVALID_ID) {
        _sg_pipeline_t* pip = _sg_pipeline_at(&_sg.pools, pip_id.id);
        SOKOL_ASSERT(pip && (pip->slot.state == SG_RESOURCESTATE_ALLOC));
        _sg_init_pipeline(pip, &desc_def);
        SOKOL_ASSERT((pip->slot.state == SG_RESOURCESTATE_VALID) || (pip->slot.state == SG_RESOURCESTATE_FAILED));
    }
    else {
        SG_LOG("pipeline pool exhausted!");
        _SG_TRACE_NOARGS(err_pipeline_pool_exhausted);
    }
    _SG_TRACE_ARGS(make_pipeline, &desc_def, pip_id);
    return pip_id;
}

SOKOL_API_IMPL sg_pass sg_make_pass(const sg_pass_desc* desc) {
    SOKOL_ASSERT(_sg.valid);
    SOKOL_ASSERT(desc);
    sg_pass_desc desc_def = _sg_pass_desc_defaults(desc);
    sg_pass pass_id = _sg_alloc_pass();
    if (pass_id.id != SG_INVALID_ID) {
        _sg_pass_t* pass = _sg_pass_at(&_sg.pools, pass_id.id);
        SOKOL_ASSERT(pass && (pass->slot.state == SG_RESOURCESTATE_ALLOC));
        _sg_init_pass(pass, &desc_def);
        SOKOL_ASSERT((pass->slot.state == SG_RESOURCESTATE_VALID) || (pass->slot.state == SG_RESOURCESTATE_FAILED));
    }
    else {
        SG_LOG("pass pool exhausted!");
        _SG_TRACE_NOARGS(err_pass_pool_exhausted);
    }
    _SG_TRACE_ARGS(make_pass, &desc_def, pass_id);
    return pass_id;
}

/*-- destroy resource --------------------------------------------------------*/
SOKOL_API_IMPL void sg_destroy_buffer(sg_buffer buf_id) {
    SOKOL_ASSERT(_sg.valid);
    _SG_TRACE_ARGS(destroy_buffer, buf_id);
    _sg_buffer_t* buf = _sg_lookup_buffer(&_sg.pools, buf_id.id);
    if (buf) {
        if ((buf->slot.state == SG_RESOURCESTATE_VALID) || (buf->slot.state == SG_RESOURCESTATE_FAILED)) {
            _sg_uninit_buffer(buf);
            SOKOL_ASSERT(buf->slot.state == SG_RESOURCESTATE_ALLOC);
        }
        if (buf->slot.state == SG_RESOURCESTATE_ALLOC) {
            _sg_dealloc_buffer(buf);
            SOKOL_ASSERT(buf->slot.state == SG_RESOURCESTATE_INITIAL);
        }
    }
}

SOKOL_API_IMPL void sg_destroy_image(sg_image img_id) {
    SOKOL_ASSERT(_sg.valid);
    _SG_TRACE_ARGS(destroy_image, img_id);
    _sg_image_t* img = _sg_lookup_image(&_sg.pools, img_id.id);
    if (img) {
        if ((img->slot.state == SG_RESOURCESTATE_VALID) || (img->slot.state == SG_RESOURCESTATE_FAILED)) {
            _sg_uninit_image(img);
            SOKOL_ASSERT(img->slot.state == SG_RESOURCESTATE_ALLOC);
        }
        if (img->slot.state == SG_RESOURCESTATE_ALLOC) {
            _sg_dealloc_image(img);
            SOKOL_ASSERT(img->slot.state == SG_RESOURCESTATE_INITIAL);
        }
    }
}

SOKOL_API_IMPL void sg_destroy_shader(sg_shader shd_id) {
    SOKOL_ASSERT(_sg.valid);
    _SG_TRACE_ARGS(destroy_shader, shd_id);
    _sg_shader_t* shd = _sg_lookup_shader(&_sg.pools, shd_id.id);
    if (shd) {
        if ((shd->slot.state == SG_RESOURCESTATE_VALID) || (shd->slot.state == SG_RESOURCESTATE_FAILED)) {
            _sg_uninit_shader(shd);
            SOKOL_ASSERT(shd->slot.state == SG_RESOURCESTATE_ALLOC);
        }
        if (shd->slot.state == SG_RESOURCESTATE_ALLOC) {
            _sg_dealloc_shader(shd);
            SOKOL_ASSERT(shd->slot.state == SG_RESOURCESTATE_INITIAL);
        }
    }
}

SOKOL_API_IMPL void sg_destroy_pipeline(sg_pipeline pip_id) {
    SOKOL_ASSERT(_sg.valid);
    _SG_TRACE_ARGS(destroy_pipeline, pip_id);
    _sg_pipeline_t* pip = _sg_lookup_pipeline(&_sg.pools, pip_id.id);
    if (pip) {
        if ((pip->slot.state == SG_RESOURCESTATE_VALID) || (pip->slot.state == SG_RESOURCESTATE_FAILED)) {
            _sg_uninit_pipeline(pip);
            SOKOL_ASSERT(pip->slot.state == SG_RESOURCESTATE_ALLOC);
        }
        if (pip->slot.state == SG_RESOURCESTATE_ALLOC) {
            _sg_dealloc_pipeline(pip);
            SOKOL_ASSERT(pip->slot.state == SG_RESOURCESTATE_INITIAL);
        }
    }
}

SOKOL_API_IMPL void sg_destroy_pass(sg_pass pass_id) {
    SOKOL_ASSERT(_sg.valid);
    _SG_TRACE_ARGS(destroy_pass, pass_id);
    _sg_pass_t* pass = _sg_lookup_pass(&_sg.pools, pass_id.id);
    if (pass) {
        if ((pass->slot.state == SG_RESOURCESTATE_VALID) || (pass->slot.state == SG_RESOURCESTATE_FAILED)) {
            _sg_uninit_pass(pass);
            SOKOL_ASSERT(pass->slot.state == SG_RESOURCESTATE_ALLOC);
        }
        if (pass->slot.state == SG_RESOURCESTATE_ALLOC) {
            _sg_dealloc_pass(pass);
            SOKOL_ASSERT(pass->slot.state == SG_RESOURCESTATE_INITIAL);
        }
    }
}

SOKOL_API_IMPL void sg_begin_default_pass(const sg_pass_action* pass_action, int width, int height) {
    SOKOL_ASSERT(_sg.valid);
    SOKOL_ASSERT(pass_action);
    SOKOL_ASSERT((pass_action->_start_canary == 0) && (pass_action->_end_canary == 0));
    sg_pass_action pa;
    _sg_resolve_default_pass_action(pass_action, &pa);
    _sg.cur_pass.id = SG_INVALID_ID;
    _sg.pass_valid = true;
    _sg_begin_pass(0, &pa, width, height);
    _SG_TRACE_ARGS(begin_default_pass, pass_action, width, height);
}

SOKOL_API_IMPL void sg_begin_default_passf(const sg_pass_action* pass_action, float width, float height) {
    sg_begin_default_pass(pass_action, (int)width, (int)height);
}

SOKOL_API_IMPL void sg_begin_pass(sg_pass pass_id, const sg_pass_action* pass_action) {
    SOKOL_ASSERT(_sg.valid);
    SOKOL_ASSERT(pass_action);
    SOKOL_ASSERT((pass_action->_start_canary == 0) && (pass_action->_end_canary == 0));
    _sg.cur_pass = pass_id;
    _sg_pass_t* pass = _sg_lookup_pass(&_sg.pools, pass_id.id);
    if (pass && _sg_validate_begin_pass(pass)) {
        _sg.pass_valid = true;
        sg_pass_action pa;
        _sg_resolve_default_pass_action(pass_action, &pa);
        const _sg_image_t* img = _sg_pass_color_image(pass, 0);
        SOKOL_ASSERT(img);
        const int w = img->cmn.width;
        const int h = img->cmn.height;
        _sg_begin_pass(pass, &pa, w, h);
        _SG_TRACE_ARGS(begin_pass, pass_id, pass_action);
    }
    else {
        _sg.pass_valid = false;
        _SG_TRACE_NOARGS(err_pass_invalid);
    }
}

SOKOL_API_IMPL void sg_apply_viewport(int x, int y, int width, int height, bool origin_top_left) {
    SOKOL_ASSERT(_sg.valid);
    if (!_sg.pass_valid) {
        _SG_TRACE_NOARGS(err_pass_invalid);
        return;
    }
    _sg_apply_viewport(x, y, width, height, origin_top_left);
    _SG_TRACE_ARGS(apply_viewport, x, y, width, height, origin_top_left);
}

SOKOL_API_IMPL void sg_apply_viewportf(float x, float y, float width, float height, bool origin_top_left) {
    sg_apply_viewport((int)x, (int)y, (int)width, (int)height, origin_top_left);
}

SOKOL_API_IMPL void sg_apply_scissor_rect(int x, int y, int width, int height, bool origin_top_left) {
    SOKOL_ASSERT(_sg.valid);
    if (!_sg.pass_valid) {
        _SG_TRACE_NOARGS(err_pass_invalid);
        return;
    }
    _sg_apply_scissor_rect(x, y, width, height, origin_top_left);
    _SG_TRACE_ARGS(apply_scissor_rect, x, y, width, height, origin_top_left);
}

SOKOL_API_IMPL void sg_apply_scissor_rectf(float x, float y, float width, float height, bool origin_top_left) {
    sg_apply_scissor_rect((int)x, (int)y, (int)width, (int)height, origin_top_left);
}

SOKOL_API_IMPL void sg_apply_pipeline(sg_pipeline pip_id) {
    SOKOL_ASSERT(_sg.valid);
    _sg.bindings_valid = false;
    if (!_sg_validate_apply_pipeline(pip_id)) {
        _sg.next_draw_valid = false;
        _SG_TRACE_NOARGS(err_draw_invalid);
        return;
    }
    if (!_sg.pass_valid) {
        _SG_TRACE_NOARGS(err_pass_invalid);
        return;
    }
    _sg.cur_pipeline = pip_id;
    _sg_pipeline_t* pip = _sg_lookup_pipeline(&_sg.pools, pip_id.id);
    SOKOL_ASSERT(pip);
    _sg.next_draw_valid = (SG_RESOURCESTATE_VALID == pip->slot.state);
    SOKOL_ASSERT(pip->shader && (pip->shader->slot.id == pip->cmn.shader_id.id));
    _sg_apply_pipeline(pip);
    _SG_TRACE_ARGS(apply_pipeline, pip_id);
}

SOKOL_API_IMPL void sg_apply_bindings(const sg_bindings* bindings) {
    SOKOL_ASSERT(_sg.valid);
    SOKOL_ASSERT(bindings);
    SOKOL_ASSERT((bindings->_start_canary == 0) && (bindings->_end_canary==0));
    if (!_sg_validate_apply_bindings(bindings)) {
        _sg.next_draw_valid = false;
        _SG_TRACE_NOARGS(err_draw_invalid);
        return;
    }
    _sg.bindings_valid = true;

    _sg_pipeline_t* pip = _sg_lookup_pipeline(&_sg.pools, _sg.cur_pipeline.id);
    SOKOL_ASSERT(pip);

    _sg_buffer_t* vbs[SG_MAX_SHADERSTAGE_BUFFERS] = { 0 };
    int num_vbs = 0;
    for (int i = 0; i < SG_MAX_SHADERSTAGE_BUFFERS; i++, num_vbs++) {
        if (bindings->vertex_buffers[i].id) {
            vbs[i] = _sg_lookup_buffer(&_sg.pools, bindings->vertex_buffers[i].id);
            SOKOL_ASSERT(vbs[i]);
            _sg.next_draw_valid &= (SG_RESOURCESTATE_VALID == vbs[i]->slot.state);
            _sg.next_draw_valid &= !vbs[i]->cmn.append_overflow;
        }
        else {
            break;
        }
    }

    _sg_buffer_t* ib = 0;
    if (bindings->index_buffer.id) {
        ib = _sg_lookup_buffer(&_sg.pools, bindings->index_buffer.id);
        SOKOL_ASSERT(ib);
        _sg.next_draw_valid &= (SG_RESOURCESTATE_VALID == ib->slot.state);
        _sg.next_draw_valid &= !ib->cmn.append_overflow;
    }

    _sg_image_t* vs_imgs[SG_MAX_SHADERSTAGE_IMAGES] = { 0 };
    int num_vs_imgs = 0;
    for (int i = 0; i < SG_MAX_SHADERSTAGE_IMAGES; i++, num_vs_imgs++) {
        if (bindings->vs_images[i].id) {
            vs_imgs[i] = _sg_lookup_image(&_sg.pools, bindings->vs_images[i].id);
            SOKOL_ASSERT(vs_imgs[i]);
            _sg.next_draw_valid &= (SG_RESOURCESTATE_VALID == vs_imgs[i]->slot.state);
        }
        else {
            break;
        }
    }

    _sg_image_t* fs_imgs[SG_MAX_SHADERSTAGE_IMAGES] = { 0 };
    int num_fs_imgs = 0;
    for (int i = 0; i < SG_MAX_SHADERSTAGE_IMAGES; i++, num_fs_imgs++) {
        if (bindings->fs_images[i].id) {
            fs_imgs[i] = _sg_lookup_image(&_sg.pools, bindings->fs_images[i].id);
            SOKOL_ASSERT(fs_imgs[i]);
            _sg.next_draw_valid &= (SG_RESOURCESTATE_VALID == fs_imgs[i]->slot.state);
        }
        else {
            break;
        }
    }
    if (_sg.next_draw_valid) {
        const int* vb_offsets = bindings->vertex_buffer_offsets;
        int ib_offset = bindings->index_buffer_offset;
        _sg_apply_bindings(pip, vbs, vb_offsets, num_vbs, ib, ib_offset, vs_imgs, num_vs_imgs, fs_imgs, num_fs_imgs);
        _SG_TRACE_ARGS(apply_bindings, bindings);
    }
    else {
        _SG_TRACE_NOARGS(err_draw_invalid);
    }
}

SOKOL_API_IMPL void sg_apply_uniforms(sg_shader_stage stage, int ub_index, const sg_range* data) {
    SOKOL_ASSERT(_sg.valid);
    SOKOL_ASSERT((stage == SG_SHADERSTAGE_VS) || (stage == SG_SHADERSTAGE_FS));
    SOKOL_ASSERT((ub_index >= 0) && (ub_index < SG_MAX_SHADERSTAGE_UBS));
    SOKOL_ASSERT(data && data->ptr && (data->size > 0));
    if (!_sg_validate_apply_uniforms(stage, ub_index, data)) {
        _sg.next_draw_valid = false;
        _SG_TRACE_NOARGS(err_draw_invalid);
        return;
    }
    if (!_sg.pass_valid) {
        _SG_TRACE_NOARGS(err_pass_invalid);
        return;
    }
    if (!_sg.next_draw_valid) {
        _SG_TRACE_NOARGS(err_draw_invalid);
        return;
    }
    _sg_apply_uniforms(stage, ub_index, data);
    _SG_TRACE_ARGS(apply_uniforms, stage, ub_index, data);
}

SOKOL_API_IMPL void sg_draw(int base_element, int num_elements, int num_instances) {
    SOKOL_ASSERT(_sg.valid);
    SOKOL_ASSERT(base_element >= 0);
    SOKOL_ASSERT(num_elements >= 0);
    SOKOL_ASSERT(num_instances >= 0);
    #if defined(SOKOL_DEBUG)
        if (!_sg.bindings_valid) {
            SG_LOG("attempting to draw without resource bindings");
        }
    #endif
    if (!_sg.pass_valid) {
        _SG_TRACE_NOARGS(err_pass_invalid);
        return;
    }
    if (!_sg.next_draw_valid) {
        _SG_TRACE_NOARGS(err_draw_invalid);
        return;
    }
    if (!_sg.bindings_valid) {
        _SG_TRACE_NOARGS(err_bindings_invalid);
        return;
    }
    /* attempting to draw with zero elements or instances is not technically an
       error, but might be handled as an error in the backend API (e.g. on Metal)
    */
    if ((0 == num_elements) || (0 == num_instances)) {
        _SG_TRACE_NOARGS(err_draw_invalid);
        return;
    }
    _sg_draw(base_element, num_elements, num_instances);
    _SG_TRACE_ARGS(draw, base_element, num_elements, num_instances);
}

SOKOL_API_IMPL void sg_end_pass(void) {
    SOKOL_ASSERT(_sg.valid);
    if (!_sg.pass_valid) {
        _SG_TRACE_NOARGS(err_pass_invalid);
        return;
    }
    _sg_end_pass();
    _sg.cur_pass.id = SG_INVALID_ID;
    _sg.cur_pipeline.id = SG_INVALID_ID;
    _sg.pass_valid = false;
    _SG_TRACE_NOARGS(end_pass);
}

SOKOL_API_IMPL void sg_commit(void) {
    SOKOL_ASSERT(_sg.valid);
    _sg_commit();
    _sg_notify_commit_listeners();
    _SG_TRACE_NOARGS(commit);
    _sg.frame_index++;
}

SOKOL_API_IMPL void sg_reset_state_cache(void) {
    SOKOL_ASSERT(_sg.valid);
    _sg_reset_state_cache();
    _SG_TRACE_NOARGS(reset_state_cache);
}

SOKOL_API_IMPL void sg_update_buffer(sg_buffer buf_id, const sg_range* data) {
    SOKOL_ASSERT(_sg.valid);
    SOKOL_ASSERT(data && data->ptr && (data->size > 0));
    _sg_buffer_t* buf = _sg_lookup_buffer(&_sg.pools, buf_id.id);
    if ((data->size > 0) && buf && (buf->slot.state == SG_RESOURCESTATE_VALID)) {
        if (_sg_validate_update_buffer(buf, data)) {
            SOKOL_ASSERT(data->size <= (size_t)buf->cmn.size);
            /* only one update allowed per buffer and frame */
            SOKOL_ASSERT(buf->cmn.update_frame_index != _sg.frame_index);
            /* update and append on same buffer in same frame not allowed */
            SOKOL_ASSERT(buf->cmn.append_frame_index != _sg.frame_index);
            _sg_update_buffer(buf, data);
            buf->cmn.update_frame_index = _sg.frame_index;
        }
    }
    _SG_TRACE_ARGS(update_buffer, buf_id, data);
}

SOKOL_API_IMPL int sg_append_buffer(sg_buffer buf_id, const sg_range* data) {
    SOKOL_ASSERT(_sg.valid);
    SOKOL_ASSERT(data && data->ptr);
    _sg_buffer_t* buf = _sg_lookup_buffer(&_sg.pools, buf_id.id);
    int result;
    if (buf) {
        /* rewind append cursor in a new frame */
        if (buf->cmn.append_frame_index != _sg.frame_index) {
            buf->cmn.append_pos = 0;
            buf->cmn.append_overflow = false;
        }
        if ((buf->cmn.append_pos + _sg_roundup((int)data->size, 4)) > buf->cmn.size) {
            buf->cmn.append_overflow = true;
        }
        const int start_pos = buf->cmn.append_pos;
        if (buf->slot.state == SG_RESOURCESTATE_VALID) {
            if (_sg_validate_append_buffer(buf, data)) {
                if (!buf->cmn.append_overflow && (data->size > 0)) {
                    /* update and append on same buffer in same frame not allowed */
                    SOKOL_ASSERT(buf->cmn.update_frame_index != _sg.frame_index);
                    int copied_num_bytes = _sg_append_buffer(buf, data, buf->cmn.append_frame_index != _sg.frame_index);
                    buf->cmn.append_pos += copied_num_bytes;
                    buf->cmn.append_frame_index = _sg.frame_index;
                }
            }
        }
        result = start_pos;
    }
    else {
        /* FIXME: should we return -1 here? */
        result = 0;
    }
    _SG_TRACE_ARGS(append_buffer, buf_id, data, result);
    return result;
}

SOKOL_API_IMPL bool sg_query_buffer_overflow(sg_buffer buf_id) {
    SOKOL_ASSERT(_sg.valid);
    _sg_buffer_t* buf = _sg_lookup_buffer(&_sg.pools, buf_id.id);
    bool result = buf ? buf->cmn.append_overflow : false;
    return result;
}

SOKOL_API_IMPL bool sg_query_buffer_will_overflow(sg_buffer buf_id, size_t size) {
    SOKOL_ASSERT(_sg.valid);
    _sg_buffer_t* buf = _sg_lookup_buffer(&_sg.pools, buf_id.id);
    bool result = false;
    if (buf) {
        int append_pos = buf->cmn.append_pos;
        /* rewind append cursor in a new frame */
        if (buf->cmn.append_frame_index != _sg.frame_index) {
            append_pos = 0;
        }
        if ((append_pos + _sg_roundup((int)size, 4)) > buf->cmn.size) {
            result = true;
        }
    }
    return result;
}

SOKOL_API_IMPL void sg_update_image(sg_image img_id, const sg_image_data* data) {
    SOKOL_ASSERT(_sg.valid);
    _sg_image_t* img = _sg_lookup_image(&_sg.pools, img_id.id);
    if (img && img->slot.state == SG_RESOURCESTATE_VALID) {
        if (_sg_validate_update_image(img, data)) {
            SOKOL_ASSERT(img->cmn.upd_frame_index != _sg.frame_index);
            _sg_update_image(img, data);
            img->cmn.upd_frame_index = _sg.frame_index;
        }
    }
    _SG_TRACE_ARGS(update_image, img_id, data);
}

SOKOL_API_IMPL void sg_push_debug_group(const char* name) {
    SOKOL_ASSERT(_sg.valid);
    SOKOL_ASSERT(name);
    _SOKOL_UNUSED(name);
    _SG_TRACE_ARGS(push_debug_group, name);
}

SOKOL_API_IMPL void sg_pop_debug_group(void) {
    SOKOL_ASSERT(_sg.valid);
    _SG_TRACE_NOARGS(pop_debug_group);
}

SOKOL_API_IMPL bool sg_add_commit_listener(sg_commit_listener listener) {
    SOKOL_ASSERT(_sg.valid);
    return _sg_add_commit_listener(&listener);
}

SOKOL_API_IMPL bool sg_remove_commit_listener(sg_commit_listener listener) {
    SOKOL_ASSERT(_sg.valid);
    return _sg_remove_commit_listener(&listener);
}

SOKOL_API_IMPL sg_buffer_info sg_query_buffer_info(sg_buffer buf_id) {
    SOKOL_ASSERT(_sg.valid);
    sg_buffer_info info;
    _sg_clear(&info, sizeof(info));
    const _sg_buffer_t* buf = _sg_lookup_buffer(&_sg.pools, buf_id.id);
    if (buf) {
        info.slot.state = buf->slot.state;
        info.slot.res_id = buf->slot.id;
        info.slot.ctx_id = buf->slot.ctx_id;
        info.update_frame_index = buf->cmn.update_frame_index;
        info.append_frame_index = buf->cmn.append_frame_index;
        info.append_pos = buf->cmn.append_pos;
        info.append_overflow = buf->cmn.append_overflow;
        #if defined(SOKOL_D3D11)
        info.num_slots = 1;
        info.active_slot = 0;
        #else
        info.num_slots = buf->cmn.num_slots;
        info.active_slot = buf->cmn.active_slot;
        #endif
    }
    return info;
}

SOKOL_API_IMPL sg_image_info sg_query_image_info(sg_image img_id) {
    SOKOL_ASSERT(_sg.valid);
    sg_image_info info;
    _sg_clear(&info, sizeof(info));
    const _sg_image_t* img = _sg_lookup_image(&_sg.pools, img_id.id);
    if (img) {
        info.slot.state = img->slot.state;
        info.slot.res_id = img->slot.id;
        info.slot.ctx_id = img->slot.ctx_id;
        info.upd_frame_index = img->cmn.upd_frame_index;
        #if defined(SOKOL_D3D11)
        info.num_slots = 1;
        info.active_slot = 0;
        #else
        info.num_slots = img->cmn.num_slots;
        info.active_slot = img->cmn.active_slot;
        #endif
        info.width = img->cmn.width;
        info.height = img->cmn.height;
    }
    return info;
}

SOKOL_API_IMPL sg_shader_info sg_query_shader_info(sg_shader shd_id) {
    SOKOL_ASSERT(_sg.valid);
    sg_shader_info info;
    _sg_clear(&info, sizeof(info));
    const _sg_shader_t* shd = _sg_lookup_shader(&_sg.pools, shd_id.id);
    if (shd) {
        info.slot.state = shd->slot.state;
        info.slot.res_id = shd->slot.id;
        info.slot.ctx_id = shd->slot.ctx_id;
    }
    return info;
}

SOKOL_API_IMPL sg_pipeline_info sg_query_pipeline_info(sg_pipeline pip_id) {
    SOKOL_ASSERT(_sg.valid);
    sg_pipeline_info info;
    _sg_clear(&info, sizeof(info));
    const _sg_pipeline_t* pip = _sg_lookup_pipeline(&_sg.pools, pip_id.id);
    if (pip) {
        info.slot.state = pip->slot.state;
        info.slot.res_id = pip->slot.id;
        info.slot.ctx_id = pip->slot.ctx_id;
    }
    return info;
}

SOKOL_API_IMPL sg_pass_info sg_query_pass_info(sg_pass pass_id) {
    SOKOL_ASSERT(_sg.valid);
    sg_pass_info info;
    _sg_clear(&info, sizeof(info));
    const _sg_pass_t* pass = _sg_lookup_pass(&_sg.pools, pass_id.id);
    if (pass) {
        info.slot.state = pass->slot.state;
        info.slot.res_id = pass->slot.id;
        info.slot.ctx_id = pass->slot.ctx_id;
    }
    return info;
}

SOKOL_API_IMPL sg_buffer_desc sg_query_buffer_defaults(const sg_buffer_desc* desc) {
    SOKOL_ASSERT(_sg.valid && desc);
    return _sg_buffer_desc_defaults(desc);
}

SOKOL_API_IMPL sg_image_desc sg_query_image_defaults(const sg_image_desc* desc) {
    SOKOL_ASSERT(_sg.valid && desc);
    return _sg_image_desc_defaults(desc);
}

SOKOL_API_IMPL sg_shader_desc sg_query_shader_defaults(const sg_shader_desc* desc) {
    SOKOL_ASSERT(_sg.valid && desc);
    return _sg_shader_desc_defaults(desc);
}

SOKOL_API_IMPL sg_pipeline_desc sg_query_pipeline_defaults(const sg_pipeline_desc* desc) {
    SOKOL_ASSERT(_sg.valid && desc);
    return _sg_pipeline_desc_defaults(desc);
}

SOKOL_API_IMPL sg_pass_desc sg_query_pass_defaults(const sg_pass_desc* desc) {
    SOKOL_ASSERT(_sg.valid && desc);
    return _sg_pass_desc_defaults(desc);
}

SOKOL_API_IMPL const void* sg_d3d11_device(void) {
#if defined(SOKOL_D3D11)
    return (const void*) _sg.d3d11.dev;
#else
    return 0;
#endif
}

SOKOL_API_IMPL const void* sg_mtl_device(void) {
#if defined(SOKOL_METAL)
    if (nil != _sg.mtl.device) {
        return (__bridge const void*) _sg.mtl.device;
    }
    else {
        return 0;
    }
#else
    return 0;
#endif
}

SOKOL_API_IMPL const void* sg_mtl_render_command_encoder(void) {
    #if defined(SOKOL_METAL)
        if (nil != _sg.mtl.cmd_encoder) {
            return (__bridge const void*) _sg.mtl.cmd_encoder;
        }
        else {
            return 0;
        }
    #else
        return 0;
    #endif
}

#ifdef _MSC_VER
#pragma warning(pop)
#endif

#endif /* SOKOL_GFX_IMPL */
