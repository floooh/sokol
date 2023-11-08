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
    - create a window, swapchain or the 3D-API context/device, you must do this
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

        Depending on the selected 3D backend, sokol-gfx requires some
        information, like a device pointer framebuffer pixel formats
        and so on. If you are using sokol_app.h for the window system
        glue, you can use a helper function provided in the sokol_glue.h
        header:

            #include "sokol_gfx.h"
            #include "sokol_app.h"
            #include "sokol_glue.h"
            //...
            sg_setup(&(sg_desc){
                .context = sapp_sgcontext(),
            });

        To get any logging output for errors and from the validation layer, you
        need to provide a logging callback. Easiest way is through sokol_log.h:

            #include "sokol_log.h"
            //...
            sg_setup(&(sg_desc){
                //...
                .logger.func = slog_func,
            });

    --- create resource objects (at least buffers, shaders and pipelines,
        and optionally images, samplers and passes):

            sg_buffer sg_make_buffer(const sg_buffer_desc*)
            sg_image sg_make_image(const sg_image_desc*)
            sg_sampler sg_make_sampler(const sg_sampler_desc*)
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
        draw call (1..N vertex buffers, 0 or 1 index buffer, 0..N image objects and
        0..N sampler objects on the vertex-shader- and fragment-shader-stage
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
            sg_destroy_sampler(sg_sampler smp)
            sg_destroy_shader(sg_shader shd)
            sg_destroy_pipeline(sg_pipeline pip)
            sg_destroy_pass(sg_pass pass)

    --- to set a new viewport rectangle, call

            sg_apply_viewport(int x, int y, int width, int height, bool origin_top_left)

        ...or if you want to specify the viewport rectangle with float values:

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

    --- you can get a desc struct matching the creation attributes of a
        specific resource object via:

            sg_buffer_desc sg_query_buffer_desc(sg_buffer buf)
            sg_image_desc sg_query_image_desc(sg_image img)
            sg_sampler_desc sg_query_sampler_desc(sg_sampler smp)
            sg_shader_desc sq_query_shader_desc(sg_shader shd)
            sg_pipeline_desc sg_query_pipeline_desc(sg_pipeline pip)
            sg_pass_desc sg_query_pass_desc(sg_pass pass)

        ...but NOTE that the returned desc structs may be incomplete, only
        creation attributes that are kept around internally after resource
        creation will be filled in, and in some cases (like shaders) that's
        very little. Any missing attributes will be set to zero. The returned
        desc structs might still be useful as partial blueprint for creating
        similar resources if filled up with the missing attributes.

        Calling the query-desc functions on an invalid resource will return
        completely zeroed structs (it makes sense to check  the resource state
        with sg_query_*_state() first)

    --- you can query the default resource creation parameters through the functions

            sg_buffer_desc sg_query_buffer_defaults(const sg_buffer_desc* desc)
            sg_image_desc sg_query_image_defaults(const sg_image_desc* desc)
            sg_sampler_desc sg_query_sampler_defaults(const sg_sampler_desc* desc)
            sg_shader_desc sg_query_shader_defaults(const sg_shader_desc* desc)
            sg_pipeline_desc sg_query_pipeline_defaults(const sg_pipeline_desc* desc)
            sg_pass_desc sg_query_pass_defaults(const sg_pass_desc* desc)

        These functions take a pointer to a desc structure which may contain
        zero-initialized items for default values. These zero-init values
        will be replaced with their concrete values in the returned desc
        struct.

    --- you can inspect various internal resource runtime values via:

            sg_buffer_info sg_query_buffer_info(sg_buffer buf)
            sg_image_info sg_query_image_info(sg_image img)
            sg_sampler_info sg_query_sampler_info(sg_sampler smp)
            sg_shader_info sg_query_shader_info(sg_shader shd)
            sg_pipeline_info sg_query_pipeline_info(sg_pipeline pip)
            sg_pass_info sg_query_pass_info(sg_pass pass)

        ...please note that the returned info-structs are tied quite closely
        to sokol_gfx.h internals, and may change more often than other
        public API functions and structs.

    --- you can query frame stats and control stats collection via:

            sg_query_frame_stats()
            sg_enable_frame_stats()
            sg_disable_frame_stats()
            sg_frame_stats_enabled()

    --- you can ask at runtime what backend sokol_gfx.h has been compiled for:

            sg_backend sg_query_backend(void)


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

    See the documentation block of the sg_desc struct below for more information.


    ON RENDER PASSES
    ================
    Relevant samples:
        - https://floooh.github.io/sokol-html5/offscreen-sapp.html
        - https://floooh.github.io/sokol-html5/offscreen-msaa-sapp.html
        - https://floooh.github.io/sokol-html5/mrt-sapp.html
        - https://floooh.github.io/sokol-html5/mrt-pixelformats-sapp.html

    A render pass groups rendering commands into a set of render target images
    (called 'pass attachments'). Render target images can be used in subsequent
    passes as textures (it is invalid to use the same image both as render target
    and as texture in the same pass).

    The following sokol-gfx functions must be called inside a render pass:

        sg_apply_viewport(f)
        sg_apply_scissor_rect(f)
        sg_apply_pipeline
        sg_apply_bindings
        sg_apply_uniforms
        sg_draw

    A frame must have at least one render pass, and this must be the 'default
    pass' which renders into the 'default' (swapchain) framebuffer. The default
    pass must always be the last pass in the frame before the sg_commit()
    call.

    The default and offscreen passes form a dependency tree with the default
    pass at the root, offscreen passes as nodes, and render target images as
    dependencies between passes.

    For offscreen render passes, the render target images used in a render pass
    are baked into an immutable sg_pass object (for the default pass, the
    pass-state is managed internally instead).

    For a simple offscreen scenario with one color-, one depth-stencil-render
    target and without multisampling, creating a pass object looks like this:

    First create two render target images, one with a color pixel format,
    and one with the depth- or depth-stencil pixel format. Both images
    must have the same dimensions:

        const sg_image color_img = sg_make_image(&(sg_image_desc){
            .render_target = true,
            .width = 256,
            .height = 256,
            .pixel_format = SG_PIXELFORMAT_RGBA8,
            .sample_count = 1,
        });
        const sg_image depth_img = sg_make_image(&(sg_image_desc){
            .render_target = true,
            .width = 256,
            .height = 256,
            .pixel_format = SG_PIXELFORMAT_DEPTH,
            .sample_count = 1,
        });

    NOTE: when creating render target images, have in mind that some default values
    are aligned with the default framebuffer attributes, this is sometimes not
    what you want:

        - the default values for .pixel_format and .sample_count are the same
          as the default framebuffer
        - the default value for .num_mipmaps is always 1

    Next create a pass object:

        const sg_pass pass = sg_make_pass(&(sg_pass_desc){
            .color_attachments[0].image = color_img,
            .depth_stencil_attachment.image = depth_img,
        });

    When using the sg_pass object in a render pass you also need to define
    what actions should happen at the start and end of the render pass
    in an sg_pass_action struct (for instance whether the render target should
    be cleared).

    A typical sg_pass_action object which clears the color attachment to black
    might look like this:

        const sg_pass_action = {
            .colors[0] = {
                .load_action = SG_LOADACTION_CLEAR,
                .clear_value = { 0.0f, 0.0f, 0.0f, 1.0f }
            }
        };

    This omits the defaults for the color attachment store action, and
    the depth-stencil-attachments actions. The same pass action with the
    defaults explicitly filled in would look like this:

        const sg_pass_action = {
            .colors[0] = {
                .load_action = SG_LOADACTION_CLEAR,
                .store_action = SG_STOREACTION_STORE,
                .clear_value = { 0.0f, 0.0f, 0.0f, 1.0f }
            },
            .depth = = {
                .load_action = SG_LOADACTION_CLEAR,
                .store_action = SG_STOREACTION_DONTCARE,
                .clear_value = 1.0f,
            },
            .stencil = {
                .load_action = SG_LOADACTION_CLEAR,
                .store_action = SG_STOREACTION_DONTCARE,
                .clear_value = 0
            }
        };

    With the sg_pass object and sg_pass_action struct in place everything
    is ready now for the actual render pass:

        sg_begin_pass(pass, &pass_action);
        ...
        sg_end_pass();

    Offscreen rendering can also go into a mipmap, or a slice/face of
    a cube-, array- or 3d-image (which some restrictions, for instance
    it's not possible to create a 3D image with a depth/stencil pixel format,
    these exceptions are generally caught by the sokol-gfx validation layer).

    The mipmap/slice selection happens at pass creation time, for instance
    to render into mipmap 2 of slice 3 of an array texture:

        const sg_pass pass = sg_make_pass(&(sg_pass_desc){
            .color_attachments[0] = {
                .image = color_img,
                .mip_level = 2,
                .slice = 3,
            },
            .depth_stencil_attachment.image = depth_img,
        });

    If MSAA offscreen rendering is desired, the multi-sample rendering result
    must be 'resolved' into a separate 'resolve image', before that image can
    be used as texture.

    NOTE: currently multisample-images cannot be bound as textures.

    Creating a simple pass object for multisampled rendering requires
    3 attachment images: the color attachment image which has a sample
    count > 1, a resolve attachment image of the same size and pixel format
    but a sample count == 1, and a depth/stencil attachment image with
    the same size and sample count as the color attachment image:

        const sg_image color_img = sg_make_image(&(sg_image_desc){
            .render_target = true,
            .width = 256,
            .height = 256,
            .pixel_format = SG_PIXELFORMAT_RGBA8,
            .sample_count = 4,
        });
        const sg_image resolve_img = sg_make_image(&(sg_image_desc){
            .render_target = true,
            .width = 256,
            .height = 256,
            .pixel_format = SG_PIXELFORMAT_RGBA8,
            .sample_count = 1,
        });
        const sg_image depth_img = sg_make_image(&(sg_image_desc){
            .render_target = true,
            .width = 256,
            .height = 256,
            .pixel_format = SG_PIXELFORMAT_DEPTH,
            .sample_count = 4,
        });

    ...create the pass object:

        const sg_pass pass = sg_make_pass(&(sg_pass_desc){
            .color_attachments[0].image = color_img,
            .resolve_attachments[0].image = resolve_img,
            .depth_stencil_attachment.image = depth_img,
        });

    If a pass object defines a resolve image in a specific resolve attachment slot,
    an 'msaa resolve operation' will happen in sg_end_pass().

    In this scenario, the content of the MSAA color attachment doesn't need to be
    preserved (since it's only needed inside sg_end_pass for the msaa-resolve), so
    the .store_action should be set to "don't care":

        const sg_pass_action = {
            .colors[0] = {
                .load_action = SG_LOADACTION_CLEAR,
                .store_action = SG_STOREACTION_DONTCARE,
                .clear_value = { 0.0f, 0.0f, 0.0f, 1.0f }
            }
        };

    The actual render pass looks as usual:

        sg_begin_pass(pass, &pass_action);
        ...
        sg_end_pass();

    ...after sg_end_pass() the only difference to the non-msaa scenario is that the
    rendering result which is going to be used as texture in a followup pass is
    in 'resolve_img', not in 'color_img' (in fact, trying to bind color_img as a
    texture would result in a validation error).


    ON SHADER CREATION
    ==================
    sokol-gfx doesn't come with an integrated shader cross-compiler, instead
    backend-specific shader sources or binary blobs need to be provided when
    creating a shader object, along with information about the shader resource
    binding interface needed in the sokol-gfx validation layer and to properly
    bind shader resources on the CPU-side to be consumable by the GPU-side.

    The easiest way to provide all this shader creation data is to use the
    sokol-shdc shader compiler tool to compile shaders from a common
    GLSL syntax into backend-specific sources or binary blobs, along with
    shader interface information and uniform blocks mapped to C structs.

    To create a shader using a C header which has been code-generated by sokol-shdc:

        // include the C header code-generated by sokol-shdc:
        #include "myshader.glsl.h"
        ...

        // create shader using a code-generated helper function from the C header:
        sg_shader shd = sg_make_shader(myshader_shader_desc(sg_query_backend()));

    The samples in the 'sapp' subdirectory of the sokol-samples project
    also use the sokol-shdc approach:

        https://github.com/floooh/sokol-samples/tree/master/sapp

    If you're planning to use sokol-shdc, you can stop reading here, instead
    continue with the sokol-shdc documentation:

        https://github.com/floooh/sokol-tools/blob/master/docs/sokol-shdc.md

    To create shaders with backend-specific shader code or binary blobs,
    the sg_make_shader() function requires the following information:

    - Shader code or shader binary blobs for the vertex- and fragment- shader-stage:
        - for the desktop GL backend, source code must be provided in '#version 330' syntax
        - for the GLES3 backend, source code must be provided in '#version 300 es' syntax
        - for the D3D11 backend, shaders can be provided as source or binary blobs, the
          source code should be in HLSL4.0 (for best compatibility) or alternatively
          in HLSL5.0 syntax (other versions may work but are not tested), NOTE: when
          shader source code is provided for the D3D11 backend, sokol-gfx will dynamically
          load 'd3dcompiler_47.dll'
        - for the Metal backends, shaders can be provided as source or binary blobs, the
          MSL version should be in 'metal-1.1' (other versions may work but are not tested)
        - for the WebGPU backend, shader must be provided as WGSL source code
        - optionally the following shader-code related attributes can be provided:
            - an entry function name (only on D3D11 or Metal, but not OpenGL)
            - on D3D11 only, a compilation target (default is "vs_4_0" and "ps_4_0")

    - Depending on backend, information about the input vertex attributes used by the
      vertex shader:
        - Metal: no information needed since vertex attributes are always bound
          by their attribute location defined in the shader via '[[attribute(N)]]'
        - WebGPU: no information needed since vertex attributes are always
          bound by their attribute location defined in the shader via `@location(N)`
        - GLSL: vertex attribute names can be optionally provided, in that case their
          location will be looked up by name, otherwise, the vertex attribute location
          can be defined with 'layout(location = N)', PLEASE NOTE that the name-lookup method
          may be removed at some point
        - D3D11: a 'semantic name' and 'semantic index' must be provided for each vertex
          attribute, e.g. if the vertex attribute is defined as 'TEXCOORD1' in the shader,
          the semantic name would be 'TEXCOORD', and the semantic index would be '1'

    - Information about each uniform block used in the shader:
        - The size of the uniform block in number of bytes.
        - A memory layout hint (currently 'native' or 'std140') where 'native' defines a
          backend-specific memory layout which shouldn't be used for cross-platform code.
          Only std140 guarantees a backend-agnostic memory layout.
        - For GLSL only: a description of the internal uniform block layout, which maps
          member types and their offsets on the CPU side to uniform variable names
          in the GLSL shader
        - please also NOTE the documentation sections about UNIFORM DATA LAYOUT
          and CROSS-BACKEND COMMON UNIFORM DATA LAYOUT below!

    - A description of each texture/image used in the shader:
        - the expected image type:
            - SG_IMAGETYPE_2D
            - SG_IMAGETYPE_CUBE
            - SG_IMAGETYPE_3D
            - SG_IMAGETYPE_ARRAY
        - the expected 'image sample type':
            - SG_IMAGESAMPLETYPE_FLOAT
            - SG_IMAGESAMPLETYPE_DEPTH
            - SG_IMAGESAMPLETYPE_SINT
            - SG_IMAGESAMPLETYPE_UINT
            - SG_IMAGESAMPLETYPE_UNFILTERABLE_FLOAT
        - a flag whether the texture is expected to be multisampled
          (currently it's not supported to fetch data from multisampled
          textures in shaders, but this is planned for a later time)

    - A description of each sampler used in the shader:
        - SG_SAMPLERTYPE_FILTERING,
        - SG_SAMPLERTYPE_NONFILTERING,
        - SG_SAMPLERTYPE_COMPARISON,

    - An array of 'image-sampler-pairs' used by the shader to sample textures,
      for D3D11, Metal and WebGPU this is used for validation purposes to check
      whether the texture and sampler are compatible with each other (especially
      WebGPU is very picky about combining the correct
      texture-sample-type with the correct sampler-type). For GLSL an
      additional 'combined-image-sampler name' must be provided because 'OpenGL
      style GLSL' cannot handle separate texture and sampler objects, but still
      groups them into a tradtional GLSL 'sampler object'.

    Compatibility rules for image-sample-type vs sampler-type are as follows:

        - SG_IMAGESAMPLETYPE_FLOAT => (SG_SAMPLERTYPE_FILTERING or SG_SAMPLERTYPE_NONFILTERING)
        - SG_IMAGESAMPLETYPE_UNFILTERABLE_FLOAT => SG_SAMPLERTYPE_NONFILTERING
        - SG_IMAGESAMPLETYPE_SINT => SG_SAMPLERTYPE_NONFILTERING
        - SG_IMAGESAMPLETYPE_UINT => SG_SAMPLERTYPE_NONFILTERING
        - SG_IMAGESAMPLETYPE_DEPTH => SG_SAMPLERTYPE_COMPARISON

    For example code of how to create backend-specific shader objects,
    please refer to the following samples:

        - for D3D11:    https://github.com/floooh/sokol-samples/tree/master/d3d11
        - for Metal:    https://github.com/floooh/sokol-samples/tree/master/metal
        - for OpenGL:   https://github.com/floooh/sokol-samples/tree/master/glfw
        - for GLES3:    https://github.com/floooh/sokol-samples/tree/master/html5
        - for WebGPI:   https://github.com/floooh/sokol-samples/tree/master/wgpu


    ON SG_IMAGESAMPLETYPE_UNFILTERABLE_FLOAT AND SG_SAMPLERTYPE_NONFILTERING
    ========================================================================
    The WebGPU backend introduces the concept of 'unfilterable-float' textures,
    which can only be combined with 'nonfiltering' samplers (this is a restriction
    specific to WebGPU, but since the same sokol-gfx code should work across
    all backend, the sokol-gfx validation layer also enforces this restriction
    - the alternative would be undefined behaviour in some backend APIs on
    some devices).

    The background is that some mobile devices (most notably iOS devices) can
    not perform linear filtering when sampling textures with certain pixel
    formats, most notable the 32F formats:

        - SG_PIXELFORMAT_R32F
        - SG_PIXELFORMAT_RG32F
        - SG_PIXELFORMAT_RGBA32F

    The information of whether a shader is going to be used with such an
    unfilterable-float texture must already be provided in the sg_shader_desc
    struct when creating the shader (see the above section "ON SHADER CREATION").

    If you are using the sokol-shdc shader compiler, the information whether a
    texture/sampler binding expects an 'unfilterable-float/nonfiltering'
    texture/sampler combination cannot be inferred from the shader source
    alone, you'll need to provide this hint via annotation-tags. For instance
    here is an example from the ozz-skin-sapp.c sample shader which samples an
    RGBA32F texture with skinning matrices in the vertex shader:

    ```glsl
    @image_sample_type joint_tex unfilterable_float
    uniform texture2D joint_tex;
    @sampler_type smp nonfiltering
    uniform sampler smp;
    ```

    This will result in SG_IMAGESAMPLETYPE_UNFILTERABLE_FLOAT and
    SG_SAMPLERTYPE_NONFILTERING being written to the code-generated
    sg_shader_desc struct.


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

    - The uniform block layout hint in sg_shader_desc must be explicitly set to
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

    The by far easiest way to tackle the common uniform block layout problem is
    to use the sokol-shdc shader cross-compiler tool!


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

    For a vertex input layout which works on all platforms, only use the following
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
        - SG_VERTEXFORMAT_UINT10_N2
        - SG_VERTEXFORMAT_HALF2
        - SG_VERTEXFORMAT_HALF4


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
                    .alloc_fn = my_alloc,
                    .free_fn = my_free,
                    .user_data = ...,
                }
            });
        ...

    If no overrides are provided, malloc and free will be used.

    This only affects memory allocation calls done by sokol_gfx.h
    itself though, not any allocations in OS libraries.


    ERROR REPORTING AND LOGGING
    ===========================
    To get any logging information at all you need to provide a logging callback in the setup call
    the easiest way is to use sokol_log.h:

        #include "sokol_log.h"

        sg_setup(&(sg_desc){ .logger.func = slog_func });

    To override logging with your own callback, first write a logging function like this:

        void my_log(const char* tag,                // e.g. 'sg'
                    uint32_t log_level,             // 0=panic, 1=error, 2=warn, 3=info
                    uint32_t log_item_id,           // SG_LOGITEM_*
                    const char* message_or_null,    // a message string, may be nullptr in release mode
                    uint32_t line_nr,               // line number in sokol_gfx.h
                    const char* filename_or_null,   // source filename, may be nullptr in release mode
                    void* user_data)
        {
            ...
        }

    ...and then setup sokol-gfx like this:

        sg_setup(&(sg_desc){
            .logger = {
                .func = my_log,
                .user_data = my_user_data,
            }
        });

    The provided logging function must be reentrant (e.g. be callable from
    different threads).

    If you don't want to provide your own custom logger it is highly recommended to use
    the standard logger in sokol_log.h instead, otherwise you won't see any warnings or
    errors.


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
        sg_sampler sg_make_sampler(const sg_sampler_desc* desc)
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
        sg_sampler sg_alloc_sampler(void)
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
        void sg_init_sampler(sg_sampler smp, const sg_sampler_desc* desc)
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
        void sg_uninit_sampler(sg_sampler smp)
        void sg_uninit_shader(sg_shader shd)
        void sg_uninit_pipeline(sg_pipeline pip)
        void sg_uninit_pass(sg_pass pass)

    Calling the 'uninit functions' with a resource that is not in the VALID or
    FAILED state is a no-op.

    To finally free the pool slot for recycling call the 'dealloc functions':

        void sg_dealloc_buffer(sg_buffer buf)
        void sg_dealloc_image(sg_image img)
        void sg_dealloc_sampler(sg_sampler smp)
        void sg_dealloc_shader(sg_shader shd)
        void sg_dealloc_pipeline(sg_pipeline pip)
        void sg_dealloc_pass(sg_pass pass)

    Calling the 'dealloc functions' on a resource that's not in ALLOC state is
    a no-op, but will generate a warning log message.

    Calling an 'uninit function' and 'dealloc function' in sequence is equivalent
    with calling the associated 'destroy function':

        void sg_destroy_buffer(sg_buffer buf)
        void sg_destroy_image(sg_image img)
        void sg_destroy_sampler(sg_sampler smp)
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
        sg_fail_sampler(sg_sampler smp)
        sg_fail_shader(sg_shader shd)
        sg_fail_pipeline(sg_pipeline pip)
        sg_fail_pass(sg_pass pass)

    This is recommended if anything went wrong outside of sokol-gfx during asynchronous
    resource creation (for instance a file loading operation failed). In this case,
    the 'fail function' should be called instead of the 'init function'.

    Calling a 'fail function' on a resource that's not in ALLOC state is a no-op,
    but will generate a warning log message.

    NOTE: that two-step resource creation usually only makes sense for buffers
    and images, but not for samplers, shaders, pipelines or passes. Most notably, trying
    to create a pipeline object with a shader that's not in VALID state will
    trigger a validation layer error, or if the validation layer is disabled,
    result in a pipeline object in FAILED state. Same when trying to create
    a pass object with invalid image objects.


    WEBGPU CAVEATS
    ==============
    For a general overview and design notes of the WebGPU backend see:

        https://floooh.github.io/2023/10/16/sokol-webgpu.html

    In general, don't expect an automatic speedup when switching from the WebGL2
    backend to the WebGPU backend. Some WebGPU functions currently actually
    have a higher CPU overhead than similar WebGL2 functions, leading to the
    paradoxical situation that WebGPU code is slower than similar WebGL2
    code.

    - when writing WGSL shader code by hand, a specific bind-slot convention
      must be used:

      All uniform block structs must use `@group(0)`, with up to
      4 uniform blocks per shader stage.
        - Vertex shader uniform block bindings must start at `@group(0) @binding(0)`
        - Fragment shader uniform blocks bindings must start at `@group(0) @binding(4)`

      All textures and samplers must use `@group(1)` and start at specific
      offsets depending on resource type and shader stage.
        - Vertex shader textures must start at `@group(1) @binding(0)`
        - Vertex shader samplers must start at `@group(1) @binding(16)`
        - Fragment shader textures must start at `@group(1) @binding(32)`
        - Fragment shader samplers must start at `@group(1) @binding(48)`

      Note that the actual number of allowed per-stage texture- and sampler-bindings
      in sokol-gfx is currently lower than the above ranges (currently only up to
      12 textures and 8 samplers per shader stage are allowed).

      If you use sokol-shdc to generate WGSL shader code, you don't need to worry
      about the above binding convention since sokol-shdc assigns bind slots
      automatically.

    - The sokol-gfx WebGPU backend uses the sg_desc.uniform_buffer_size item
      to allocate a single per-frame uniform buffer which must be big enough
      to hold all data written by sg_apply_uniforms() during a single frame,
      including a worst-case 256-byte alignment (e.g. each sg_apply_uniform
      call will cost 256 bytes of uniform buffer size). The default size
      is 4 MB, which is enough for 16384 sg_apply_uniform() calls per
      frame (assuming the uniform data 'payload' is less than 256 bytes
      per call). These rules are the same as for the Metal backend, so if
      you are already using the Metal backend you'll be fine.

    - sg_apply_bindings(): the sokol-gfx WebGPU backend implements a bindgroup
      cache to prevent excessive creation and destruction of BindGroup objects
      when calling sg_apply_bindings(). The number of slots in the bindgroups
      cache is defined in sg_desc.wgpu_bindgroups_cache_size when calling
      sg_setup. The cache size must be a power-of-2 numbers, with the default being
      1024. The bindgroups cache behaviour can be observed by calling the new
      function sg_query_frame_stats(), where the following struct items are
      of interest:

        .wgpu.num_bindgroup_cache_hits
        .wgpu.num_bindgroup_cache_misses
        .wgpu.num_bindgroup_cache_collisions
        .wgpu.num_bindgroup_cache_vs_hash_key_mismatch

      The value to pay attention to is `.wgpu.num_bindgroup_cache_collisions`,
      if this number if consistently higher than a few percent of the
      .wgpu.num_set_bindgroup value, it might be a good idea to bump the
      bindgroups cache size to the next power-of-2.

    - sg_apply_viewport(): WebGPU currently has a unique restriction that viewport
      rectangles must be contained entirely within the framebuffer. As a shitty
      workaround sokol_gfx.h will clip incoming viewport rectangles against
      the framebuffer, but this will distort the clipspace-to-screenspace mapping.
      There's no proper way to handle this inside sokol_gfx.h, this must be fixed
      in a future WebGPU update.

    - The sokol shader compiler generally adds `diagnostic(off, derivative_uniformity);`
      into the WGSL output. Currently only the Chrome WebGPU implementation seems
      to accept this.

    - The vertex format SG_VERTEXFORMAT_UINT10_N2 is currently not supported because
      WebGPU lacks a matching vertex format (this is currently being worked on though,
      as soon as the vertex format shows up in webgpu.h, sokol_gfx.h will add support.

    - Likewise, the following sokol-gfx vertex formats are not supported in WebGPU:
      R16, R16SN, RG16, RG16SN, RGBA16, RGBA16SN and all PVRTC compressed format.
      Unlike unsupported vertex formats, unsupported pixel formats can be queried
      in cross-backend code via sg_query_pixel_format() though.

    - The Emscripten WebGPU shim currently doesn't support the Closure minification
      post-link-step (e.g. currently the emcc argument '--closure 1' or '--closure 2'
      will generate broken Javascript code.

    - sokol-gfx requires the WebGPU device feature `depth32float-stencil8` to be enabled
      (this should be supported widely supported)

    - sokol-gfx expects that the WebGPU device feature `float32-filterable` to *not* be
      enabled (this would exclude all iOS devices)


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
    sg_image:       images used as textures and render targets
    sg_sampler      sampler object describing how a texture is sampled in a shader
    sg_shader:      vertex- and fragment-shaders and shader interface information
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
typedef struct sg_sampler  { uint32_t id; } sg_sampler;
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
#pragma warning(disable:4221)   // /W4 only: nonstandard extension used: 'x': cannot be initialized using address of automatic variable 'y'
#pragma warning(disable:4204)   // VS2015: nonstandard extension used: non-constant aggregate initializer
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
    SG_MAX_VERTEX_BUFFERS = 8,
    SG_MAX_SHADERSTAGE_IMAGES = 12,
    SG_MAX_SHADERSTAGE_SAMPLERS = 8,
    SG_MAX_SHADERSTAGE_IMAGESAMPLERPAIRS = 12,
    SG_MAX_SHADERSTAGE_UBS = 4,
    SG_MAX_UB_MEMBERS = 16,
    SG_MAX_VERTEX_ATTRIBUTES = 16,
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
*/
typedef enum sg_backend {
    SG_BACKEND_GLCORE33,
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
    are supported on most newer GPUs.

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
    _SG_PIXELFORMAT_DEFAULT,    // value 0 reserved for default-init
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
    SG_PIXELFORMAT_SRGB8A8,
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
    bool sample;        // pixel format can be sampled in shaders at least with nearest filtering
    bool filter;        // pixel format can be sampled with linear filtering
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
    bool origin_top_left;               // framebuffer and texture origin is in top left corner
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
    int max_vertex_attrs;           // max number of vertex attributes, clamped to SG_MAX_VERTEX_ATTRIBUTES
    int gl_max_vertex_uniform_vectors;  // <= GL_MAX_VERTEX_UNIFORM_VECTORS (only on GL backends)
    int gl_max_combined_texture_image_units; // <= GL_MAX_COMBINED_TEXTURE_IMAGE_UNITS (only on GL backends)
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
    _SG_USAGE_DEFAULT,      // value 0 reserved for default-init
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
    _SG_BUFFERTYPE_DEFAULT,         // value 0 reserved for default-init
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
    _SG_INDEXTYPE_DEFAULT,   // value 0 reserved for default-init
    SG_INDEXTYPE_NONE,
    SG_INDEXTYPE_UINT16,
    SG_INDEXTYPE_UINT32,
    _SG_INDEXTYPE_NUM,
    _SG_INDEXTYPE_FORCE_U32 = 0x7FFFFFFF
} sg_index_type;

/*
    sg_image_type

    Indicates the basic type of an image object (2D-texture, cubemap,
    3D-texture or 2D-array-texture). Used in the sg_image_desc.type member when
    creating an image, and in sg_shader_image_desc to describe a sampled texture
    in the shader (both must match and will be checked in the validation layer
    when calling sg_apply_bindings).

    The default image type when creating an image is SG_IMAGETYPE_2D.
*/
typedef enum sg_image_type {
    _SG_IMAGETYPE_DEFAULT,  // value 0 reserved for default-init
    SG_IMAGETYPE_2D,
    SG_IMAGETYPE_CUBE,
    SG_IMAGETYPE_3D,
    SG_IMAGETYPE_ARRAY,
    _SG_IMAGETYPE_NUM,
    _SG_IMAGETYPE_FORCE_U32 = 0x7FFFFFFF
} sg_image_type;

/*
    sg_image_sample_type

    The basic data type of a texture sample as expected by a shader.
    Must be provided in sg_shader_image_desc and used by the validation
    layer in sg_apply_bindings() to check if the provided image object
    is compatible with what the shader expects, and also required by the
    WebGPU backend.

    NOTE that the following texture pixel formats require the use
    of SG_IMAGESAMPLETYPE_UNFILTERABLE_FLOAT, combined with a sampler
    of type SG_SAMPLERTYPE_NONFILTERING:

    - SG_PIXELFORMAT_R32F
    - SG_PIXELFORMAT_RG32F
    - SG_PIXELFORMAT_RGBA32F

    (when using sokol-shdc, also check out the tags `@image_sample_type`
    and `@sampler_type`)
*/
typedef enum sg_image_sample_type {
    _SG_IMAGESAMPLETYPE_DEFAULT,  // value 0 reserved for default-init
    SG_IMAGESAMPLETYPE_FLOAT,
    SG_IMAGESAMPLETYPE_DEPTH,
    SG_IMAGESAMPLETYPE_SINT,
    SG_IMAGESAMPLETYPE_UINT,
    SG_IMAGESAMPLETYPE_UNFILTERABLE_FLOAT,
    _SG_IMAGESAMPLETYPE_NUM,
    _SG_IMAGESAMPLETYPE_FORCE_U32 = 0x7FFFFFFF
} sg_image_sample_type;

/*
    sg_sampler_type

    The basic type of a texture sampler (sampling vs comparison) as
    defined in a shader. Must be provided in sg_shader_sampler_desc.

    sg_image_sample_type and sg_sampler_type for a texture/sampler
    pair must be compatible with each other, specifically only
    the following pairs are allowed:

    - SG_IMAGESAMPLETYPE_FLOAT => (SG_SAMPLERTYPE_FILTERING or SG_SAMPLERTYPE_NONFILTERING)
    - SG_IMAGESAMPLETYPE_UNFILTERABLE_FLOAT => SG_SAMPLERTYPE_NONFILTERING
    - SG_IMAGESAMPLETYPE_SINT => SG_SAMPLERTYPE_NONFILTERING
    - SG_IMAGESAMPLETYPE_UINT => SG_SAMPLERTYPE_NONFILTERING
    - SG_IMAGESAMPLETYPE_DEPTH => SG_SAMPLERTYPE_COMPARISON
*/
typedef enum sg_sampler_type {
    _SG_SAMPLERTYPE_DEFAULT,
    SG_SAMPLERTYPE_FILTERING,
    SG_SAMPLERTYPE_NONFILTERING,
    SG_SAMPLERTYPE_COMPARISON,
    _SG_SAMPLERTYPE_NUM,
    _SG_SAMPLERTYPE_FORCE_U32,
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
    _SG_PRIMITIVETYPE_DEFAULT,  // value 0 reserved for default-init
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
    used in the sg_sampler_desc.min_filter, sg_sampler_desc.mag_filter
    and sg_sampler_desc.mipmap_filter members when creating a sampler object.

    For min_filter and mag_filter the default is SG_FILTER_NEAREST.

    For mipmap_filter the default is SG_FILTER_NONE.
*/
typedef enum sg_filter {
    _SG_FILTER_DEFAULT, // value 0 reserved for default-init
    SG_FILTER_NONE,
    SG_FILTER_NEAREST,
    SG_FILTER_LINEAR,
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
*/
typedef enum sg_wrap {
    _SG_WRAP_DEFAULT,   // value 0 reserved for default-init
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
    _SG_BORDERCOLOR_DEFAULT,    // value 0 reserved for default-init
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
    SG_VERTEXFORMAT_HALF2,
    SG_VERTEXFORMAT_HALF4,
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
    _SG_VERTEXSTEP_DEFAULT,     // value 0 reserved for default-init
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
    _SG_UNIFORMLAYOUT_DEFAULT,     // value 0 reserved for default-init
    SG_UNIFORMLAYOUT_NATIVE,       // default: layout depends on currently active backend
    SG_UNIFORMLAYOUT_STD140,       // std140: memory layout according to std140
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
    _SG_CULLMODE_DEFAULT,   // value 0 reserved for default-init
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
    _SG_FACEWINDING_DEFAULT,    // value 0 reserved for default-init
    SG_FACEWINDING_CCW,
    SG_FACEWINDING_CW,
    _SG_FACEWINDING_NUM,
    _SG_FACEWINDING_FORCE_U32 = 0x7FFFFFFF
} sg_face_winding;

/*
    sg_compare_func

    The compare-function for configuring depth- and stencil-ref tests
    in pipeline objects, and for texture samplers which perform a comparison
    instead of regular sampling operation.

    sg_pipeline_desc
        .depth
            .compare
        .stencil
            .front.compare
            .back.compar

    sg_sampler_desc
        .compare

    The default compare func for depth- and stencil-tests is
    SG_COMPAREFUNC_ALWAYS.

    The default compare func for sampler is SG_COMPAREFUNC_NEVER.
*/
typedef enum sg_compare_func {
    _SG_COMPAREFUNC_DEFAULT,    // value 0 reserved for default-init
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
    _SG_STENCILOP_DEFAULT,      // value 0 reserved for default-init
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
    _SG_BLENDFACTOR_DEFAULT,    // value 0 reserved for default-init
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
    _SG_BLENDOP_DEFAULT,    // value 0 reserved for default-init
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
    _SG_COLORMASK_DEFAULT = 0,    // value 0 reserved for default-init
    SG_COLORMASK_NONE   = 0x10,   // special value for 'all channels disabled
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
    sg_load_action

    Defines the load action that should be performed at the start of a render pass:

    SG_LOADACTION_CLEAR:        clear the render target
    SG_LOADACTION_LOAD:         load the previous content of the render target
    SG_LOADACTION_DONTCARE:     leave the render target in an undefined state

    This is used in the sg_pass_action structure.

    The default load action for all pass attachments is SG_LOADACTION_CLEAR,
    with the values rgba = { 0.5f, 0.5f, 0.5f, 1.0f }, depth=1.0f and stencil=0.

    If you want to override the default behaviour, it is important to not
    only set the clear color, but the 'action' field as well (as long as this
    is _SG_LOADACTION_DEFAULT, the value fields will be ignored).
*/
typedef enum sg_load_action {
    _SG_LOADACTION_DEFAULT,
    SG_LOADACTION_CLEAR,
    SG_LOADACTION_LOAD,
    SG_LOADACTION_DONTCARE,
    _SG_LOADACTION_FORCE_U32 = 0x7FFFFFFF
} sg_load_action;

/*
    sg_store_action

    Defines the store action that be performed at the end of a render pass:

    SG_STOREACTION_STORE:       store the rendered content to the color attachment image
    SG_STOREACTION_DONTCARE:    allows the GPU to discard the rendered content
*/
typedef enum sg_store_action {
    _SG_STOREACTION_DEFAULT,
    SG_STOREACTION_STORE,
    SG_STOREACTION_DONTCARE,
    _SG_STOREACTION_FORCE_U32 = 0x7FFFFFFF
} sg_store_action;


/*
    sg_pass_action

    The sg_pass_action struct defines the actions to be performed
    at the start of and end of a render pass.

    - at the start of the pass whether the render targets should be cleared
      loaded with their previous content, or start in an undefined state
    - for clear operations: the clear value (color, depth, or stencil values)
    - at the end of the pass, whether the rendering result should be
      stored back into the render target, or discarded
*/
typedef struct sg_color_attachment_action {
    sg_load_action load_action;         // default: SG_LOADACTION_CLEAR
    sg_store_action store_action;       // default: SG_STOREACTION_STORE
    sg_color clear_value;               // default: { 0.5f, 0.5f, 0.5f, 1.0f }
} sg_color_attachment_action;

typedef struct sg_depth_attachment_action {
    sg_load_action load_action;         // default: SG_LOADACTION_CLEAR
    sg_store_action store_action;       // default: SG_STOREACTION_DONTCARE
    float clear_value;                  // default: 1.0
} sg_depth_attachment_action;

typedef struct sg_stencil_attachment_action {
    sg_load_action load_action;         // default: SG_LOADACTION_CLEAR
    sg_store_action store_action;       // default: SG_STOREACTION_DONTCARE
    uint8_t clear_value;                // default: 0
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
    - 0..N vertex shader stage samplers
    - 0..N fragment shader stage images
    - 0..N fragment shader stage samplers

    The max number of vertex buffer and shader stage images
    are defined by the SG_MAX_VERTEX_BUFFERS and
    SG_MAX_SHADERSTAGE_IMAGES configuration constants.

    The optional buffer offsets can be used to put different unrelated
    chunks of vertex- and/or index-data into the same buffer objects.
*/
typedef struct sg_stage_bindings {
    sg_image images[SG_MAX_SHADERSTAGE_IMAGES];
    sg_sampler samplers[SG_MAX_SHADERSTAGE_SAMPLERS];
} sg_stage_bindings;

typedef struct sg_bindings {
    uint32_t _start_canary;
    sg_buffer vertex_buffers[SG_MAX_VERTEX_BUFFERS];
    int vertex_buffer_offsets[SG_MAX_VERTEX_BUFFERS];
    sg_buffer index_buffer;
    int index_buffer_offset;
    sg_stage_bindings vs;
    sg_stage_bindings fs;
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
    // optionally inject backend-specific resources
    uint32_t gl_buffers[SG_NUM_INFLIGHT_FRAMES];
    const void* mtl_buffers[SG_NUM_INFLIGHT_FRAMES];
    const void* d3d11_buffer;
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

    Creation parameters for sg_image objects, used in the sg_make_image() call.

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
    .data               an sg_image_data struct to define the initial content
    .label              0 (optional string label for trace hooks)

    Q: Why is the default sample_count for render targets identical with the
    "default sample count" from sg_desc.context.sample_count?

    A: So that it matches the default sample count in pipeline objects. Even
    though it is a bit strange/confusing that offscreen render targets by default
    get the same sample count as the default framebuffer, but it's better that
    an offscreen render target created with default parameters matches
    a pipeline object created with default parameters.

    NOTE:

    Images with usage SG_USAGE_IMMUTABLE must be fully initialized by
    providing a valid .data member which points to initialization data.

    ADVANCED TOPIC: Injecting native 3D-API textures:

    The following struct members allow to inject your own GL, Metal or D3D11
    textures into sokol_gfx:

    .gl_textures[SG_NUM_INFLIGHT_FRAMES]
    .mtl_textures[SG_NUM_INFLIGHT_FRAMES]
    .d3d11_texture
    .d3d11_shader_resource_view
    .wgpu_texture
    .wgpu_texture_view

    For GL, you can also specify the texture target or leave it empty to use
    the default texture target for the image type (GL_TEXTURE_2D for
    SG_IMAGETYPE_2D etc)

    For D3D11 and WebGPU, either only provide a texture, or both a texture and
    shader-resource-view / texture-view object. If you want to use access the
    injected texture in a shader you *must* provide a shader-resource-view.

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
    sg_image_data data;
    const char* label;
    // optionally inject backend-specific resources
    uint32_t gl_textures[SG_NUM_INFLIGHT_FRAMES];
    uint32_t gl_texture_target;
    const void* mtl_textures[SG_NUM_INFLIGHT_FRAMES];
    const void* d3d11_texture;
    const void* d3d11_shader_resource_view;
    const void* wgpu_texture;
    const void* wgpu_texture_view;
    uint32_t _end_canary;
} sg_image_desc;

/*
    sg_sampler_desc

    Creation parameters for sg_sampler objects, used in the sg_make_sampler() call

    .min_filter:        SG_FILTER_NEAREST
    .mag_filter:        SG_FILTER_NEAREST
    .mipmap_filter      SG_FILTER_NONE
    .wrap_u:            SG_WRAP_REPEAT
    .wrap_v:            SG_WRAP_REPEAT
    .wrap_w:            SG_WRAP_REPEAT (only SG_IMAGETYPE_3D)
    .min_lod            0.0f
    .max_lod            FLT_MAX
    .border_color       SG_BORDERCOLOR_OPAQUE_BLACK
    .compare            SG_COMPAREFUNC_NEVER
    .max_anisotropy     1 (must be 1..16)

*/
typedef struct sg_sampler_desc {
    uint32_t _start_canary;
    sg_filter min_filter;
    sg_filter mag_filter;
    sg_filter mipmap_filter;
    sg_wrap wrap_u;
    sg_wrap wrap_v;
    sg_wrap wrap_w;
    float min_lod;
    float max_lod;
    sg_border_color border_color;
    sg_compare_func compare;
    uint32_t max_anisotropy;
    const char* label;
    // optionally inject backend-specific resources
    uint32_t gl_sampler;
    const void* mtl_sampler;
    const void* d3d11_sampler;
    const void* wgpu_sampler;
    uint32_t _end_canary;
} sg_sampler_desc;

/*
    sg_shader_desc

    The structure sg_shader_desc defines all creation parameters for shader
    programs, used as input to the sg_make_shader() function:

    - reflection information for vertex attributes (vertex shader inputs):
        - vertex attribute name (only optionally used by GLES3 and GL)
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
        - reflection info for textures used in the shader stage:
            - the image type (SG_IMAGETYPE_xxx)
            - the image-sample type (SG_IMAGESAMPLETYPE_xxx, default is SG_IMAGESAMPLETYPE_FLOAT)
            - whether the shader expects a multisampled texture
        - reflection info for samplers used in the shader stage:
            - the sampler type (SG_SAMPLERTYPE_xxx)
        - reflection info for each image-sampler-pair used by the shader:
            - the texture slot of the involved texture
            - the sampler slot of the involved sampler
            - for GLSL only: the name of the combined image-sampler object

    For all GL backends, shader source-code must be provided. For D3D11 and Metal,
    either shader source-code or byte-code can be provided.

    For D3D11, if source code is provided, the d3dcompiler_47.dll will be loaded
    on demand. If this fails, shader creation will fail. When compiling HLSL
    source code, you can provide an optional target string via
    sg_shader_stage_desc.d3d11_target, the default target is "vs_4_0" for the
    vertex shader stage and "ps_4_0" for the pixel shader stage.
*/
typedef struct sg_shader_attr_desc {
    const char* name;           // GLSL vertex attribute name (optional)
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
    bool used;
    bool multisampled;
    sg_image_type image_type;
    sg_image_sample_type sample_type;
} sg_shader_image_desc;

typedef struct sg_shader_sampler_desc {
    bool used;
    sg_sampler_type sampler_type;
} sg_shader_sampler_desc;

typedef struct sg_shader_image_sampler_pair_desc {
    bool used;
    int image_slot;
    int sampler_slot;
    const char* glsl_name;
} sg_shader_image_sampler_pair_desc;

typedef struct sg_shader_stage_desc {
    const char* source;
    sg_range bytecode;
    const char* entry;
    const char* d3d11_target;
    sg_shader_uniform_block_desc uniform_blocks[SG_MAX_SHADERSTAGE_UBS];
    sg_shader_image_desc images[SG_MAX_SHADERSTAGE_IMAGES];
    sg_shader_sampler_desc samplers[SG_MAX_SHADERSTAGE_SAMPLERS];
    sg_shader_image_sampler_pair_desc image_sampler_pairs[SG_MAX_SHADERSTAGE_IMAGESAMPLERPAIRS];
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
typedef struct sg_vertex_buffer_layout_state {
    int stride;
    sg_vertex_step step_func;
    int step_rate;
    #if defined(SOKOL_ZIG_BINDINGS)
    uint32_t __pad[2];
    #endif
} sg_vertex_buffer_layout_state;

typedef struct sg_vertex_attr_state {
    int buffer_index;
    int offset;
    sg_vertex_format format;
    #if defined(SOKOL_ZIG_BINDINGS)
    uint32_t __pad[2];
    #endif
} sg_vertex_attr_state;

typedef struct sg_vertex_layout_state {
    sg_vertex_buffer_layout_state buffers[SG_MAX_VERTEX_BUFFERS];
    sg_vertex_attr_state attrs[SG_MAX_VERTEX_ATTRIBUTES];
} sg_vertex_layout_state;

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

typedef struct sg_color_target_state {
    sg_pixel_format pixel_format;
    sg_color_mask write_mask;
    sg_blend_state blend;
} sg_color_target_state;

typedef struct sg_pipeline_desc {
    uint32_t _start_canary;
    sg_shader shader;
    sg_vertex_layout_state layout;
    sg_depth_state depth;
    sg_stencil_state stencil;
    int color_count;
    sg_color_target_state colors[SG_MAX_COLOR_ATTACHMENTS];
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

    Creation parameters for an sg_pass object, used as argument to the
    sg_make_pass() function.

    A pass object contains 1..4 color attachments, 0..4 msaa-resolve
    attachemnts, and none or one depth-stencil attachment.

    Each attachment consists of an image, and two additional indices describing
    which subimage the pass will render into: one mipmap index, and if the image
    is a cubemap, array-texture or 3D-texture, the face-index, array-layer or
    depth-slice.

    All attachments must have the same width and height.

    All color attachments and the depth-stencil attachment must have the
    same sample count.

    If a resolve attachment is set, an MSAA-resolve operation from the
    associated color attachment into the resolve attachment image will take
    place in the sg_end_pass() function. In this case, the color attachment
    must have a (sample_count>1), and the resolve attachment a
    (sample_count==1). The resolve attachment also must have the same pixel
    format as the color attachment.

    NOTE that MSAA depth-stencil attachments cannot be msaa-resolved!
*/
typedef struct sg_pass_attachment_desc {
    sg_image image;
    int mip_level;
    int slice;      // cube texture: face; array texture: layer; 3D texture: slice
} sg_pass_attachment_desc;

typedef struct sg_pass_desc {
    uint32_t _start_canary;
    sg_pass_attachment_desc color_attachments[SG_MAX_COLOR_ATTACHMENTS];
    sg_pass_attachment_desc resolve_attachments[SG_MAX_COLOR_ATTACHMENTS];
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
    void (*make_sampler)(const sg_sampler_desc* desc, sg_sampler result, void* user_data);
    void (*make_shader)(const sg_shader_desc* desc, sg_shader result, void* user_data);
    void (*make_pipeline)(const sg_pipeline_desc* desc, sg_pipeline result, void* user_data);
    void (*make_pass)(const sg_pass_desc* desc, sg_pass result, void* user_data);
    void (*destroy_buffer)(sg_buffer buf, void* user_data);
    void (*destroy_image)(sg_image img, void* user_data);
    void (*destroy_sampler)(sg_sampler smp, void* user_data);
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
    void (*alloc_sampler)(sg_sampler result, void* user_data);
    void (*alloc_shader)(sg_shader result, void* user_data);
    void (*alloc_pipeline)(sg_pipeline result, void* user_data);
    void (*alloc_pass)(sg_pass result, void* user_data);
    void (*dealloc_buffer)(sg_buffer buf_id, void* user_data);
    void (*dealloc_image)(sg_image img_id, void* user_data);
    void (*dealloc_sampler)(sg_sampler smp_id, void* user_data);
    void (*dealloc_shader)(sg_shader shd_id, void* user_data);
    void (*dealloc_pipeline)(sg_pipeline pip_id, void* user_data);
    void (*dealloc_pass)(sg_pass pass_id, void* user_data);
    void (*init_buffer)(sg_buffer buf_id, const sg_buffer_desc* desc, void* user_data);
    void (*init_image)(sg_image img_id, const sg_image_desc* desc, void* user_data);
    void (*init_sampler)(sg_sampler smp_id, const sg_sampler_desc* desc, void* user_data);
    void (*init_shader)(sg_shader shd_id, const sg_shader_desc* desc, void* user_data);
    void (*init_pipeline)(sg_pipeline pip_id, const sg_pipeline_desc* desc, void* user_data);
    void (*init_pass)(sg_pass pass_id, const sg_pass_desc* desc, void* user_data);
    void (*uninit_buffer)(sg_buffer buf_id, void* user_data);
    void (*uninit_image)(sg_image img_id, void* user_data);
    void (*uninit_sampler)(sg_sampler smp_id, void* user_data);
    void (*uninit_shader)(sg_shader shd_id, void* user_data);
    void (*uninit_pipeline)(sg_pipeline pip_id, void* user_data);
    void (*uninit_pass)(sg_pass pass_id, void* user_data);
    void (*fail_buffer)(sg_buffer buf_id, void* user_data);
    void (*fail_image)(sg_image img_id, void* user_data);
    void (*fail_sampler)(sg_sampler smp_id, void* user_data);
    void (*fail_shader)(sg_shader shd_id, void* user_data);
    void (*fail_pipeline)(sg_pipeline pip_id, void* user_data);
    void (*fail_pass)(sg_pass pass_id, void* user_data);
    void (*push_debug_group)(const char* name, void* user_data);
    void (*pop_debug_group)(void* user_data);
} sg_trace_hooks;

/*
    sg_buffer_info
    sg_image_info
    sg_sampler_info
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
    sg_query_sampler_info()
    sg_query_shader_info()
    sg_query_pipeline_info()
    sg_query_pass_info()
*/
typedef struct sg_slot_info {
    sg_resource_state state;    // the current state of this resource slot
    uint32_t res_id;            // type-neutral resource if (e.g. sg_buffer.id)
    uint32_t ctx_id;            // the context this resource belongs to
} sg_slot_info;

typedef struct sg_buffer_info {
    sg_slot_info slot;              // resource pool slot info
    uint32_t update_frame_index;    // frame index of last sg_update_buffer()
    uint32_t append_frame_index;    // frame index of last sg_append_buffer()
    int append_pos;                 // current position in buffer for sg_append_buffer()
    bool append_overflow;           // is buffer in overflow state (due to sg_append_buffer)
    int num_slots;                  // number of renaming-slots for dynamically updated buffers
    int active_slot;                // currently active write-slot for dynamically updated buffers
} sg_buffer_info;

typedef struct sg_image_info {
    sg_slot_info slot;              // resource pool slot info
    uint32_t upd_frame_index;       // frame index of last sg_update_image()
    int num_slots;                  // number of renaming-slots for dynamically updated images
    int active_slot;                // currently active write-slot for dynamically updated images
} sg_image_info;

typedef struct sg_sampler_info {
    sg_slot_info slot;              // resource pool slot info
} sg_sampler_info;

typedef struct sg_shader_info {
    sg_slot_info slot;              // resource pool slot info
} sg_shader_info;

typedef struct sg_pipeline_info {
    sg_slot_info slot;              // resource pool slot info
} sg_pipeline_info;

typedef struct sg_pass_info {
    sg_slot_info slot;              // resource pool slot info
} sg_pass_info;

/*
    sg_frame_stats

    Allows to track generic and backend-specific tracking stats about a
    render frame. Obtained by calling sg_query_frame_stats(). The returned
    struct will contains information about the *previous* frame.
*/
typedef struct sg_frame_stats_gl {
    uint32_t num_bind_buffer;
    uint32_t num_active_texture;
    uint32_t num_bind_texture;
    uint32_t num_bind_sampler;
    uint32_t num_use_program;
    uint32_t num_render_state;
    uint32_t num_vertex_attrib_pointer;
    uint32_t num_vertex_attrib_divisor;
    uint32_t num_enable_vertex_attrib_array;
    uint32_t num_disable_vertex_attrib_array;
    uint32_t num_uniform;
} sg_frame_stats_gl;

typedef struct sg_frame_stats_d3d11_pass {
    uint32_t num_om_set_render_targets;
    uint32_t num_clear_render_target_view;
    uint32_t num_clear_depth_stencil_view;
    uint32_t num_resolve_subresource;
} sg_frame_stats_d3d11_pass;

typedef struct sg_frame_stats_d3d11_pipeline {
    uint32_t num_rs_set_state;
    uint32_t num_om_set_depth_stencil_state;
    uint32_t num_om_set_blend_state;
    uint32_t num_ia_set_primitive_topology;
    uint32_t num_ia_set_input_layout;
    uint32_t num_vs_set_shader;
    uint32_t num_vs_set_constant_buffers;
    uint32_t num_ps_set_shader;
    uint32_t num_ps_set_constant_buffers;
} sg_frame_stats_d3d11_pipeline;

typedef struct sg_frame_stats_d3d11_bindings {
    uint32_t num_ia_set_vertex_buffers;
    uint32_t num_ia_set_index_buffer;
    uint32_t num_vs_set_shader_resources;
    uint32_t num_ps_set_shader_resources;
    uint32_t num_vs_set_samplers;
    uint32_t num_ps_set_samplers;
} sg_frame_stats_d3d11_bindings;

typedef struct sg_frame_stats_d3d11_uniforms {
    uint32_t num_update_subresource;
} sg_frame_stats_d3d11_uniforms;

typedef struct sg_frame_stats_d3d11_draw {
    uint32_t num_draw_indexed_instanced;
    uint32_t num_draw_indexed;
    uint32_t num_draw_instanced;
    uint32_t num_draw;
} sg_frame_stats_d3d11_draw;

typedef struct sg_frame_stats_d3d11 {
    sg_frame_stats_d3d11_pass pass;
    sg_frame_stats_d3d11_pipeline pipeline;
    sg_frame_stats_d3d11_bindings bindings;
    sg_frame_stats_d3d11_uniforms uniforms;
    sg_frame_stats_d3d11_draw draw;
    uint32_t num_map;
    uint32_t num_unmap;
} sg_frame_stats_d3d11;

typedef struct sg_frame_stats_metal_idpool {
    uint32_t num_added;
    uint32_t num_released;
    uint32_t num_garbage_collected;
} sg_frame_stats_metal_idpool;

typedef struct sg_frame_stats_metal_pipeline {
    uint32_t num_set_blend_color;
    uint32_t num_set_cull_mode;
    uint32_t num_set_front_facing_winding;
    uint32_t num_set_stencil_reference_value;
    uint32_t num_set_depth_bias;
    uint32_t num_set_render_pipeline_state;
    uint32_t num_set_depth_stencil_state;
} sg_frame_stats_metal_pipeline;

typedef struct sg_frame_stats_metal_bindings {
    uint32_t num_set_vertex_buffer;
    uint32_t num_set_vertex_texture;
    uint32_t num_set_vertex_sampler_state;
    uint32_t num_set_fragment_texture;
    uint32_t num_set_fragment_sampler_state;
} sg_frame_stats_metal_bindings;

typedef struct sg_frame_stats_metal_uniforms {
    uint32_t num_set_vertex_buffer_offset;
    uint32_t num_set_fragment_buffer_offset;
} sg_frame_stats_metal_uniforms;

typedef struct sg_frame_stats_metal {
    sg_frame_stats_metal_idpool idpool;
    sg_frame_stats_metal_pipeline pipeline;
    sg_frame_stats_metal_bindings bindings;
    sg_frame_stats_metal_uniforms uniforms;
} sg_frame_stats_metal;

typedef struct sg_frame_stats_wgpu_uniforms {
    uint32_t num_set_bindgroup;
    uint32_t size_write_buffer;
} sg_frame_stats_wgpu_uniforms;

typedef struct sg_frame_stats_wgpu_bindings {
    uint32_t num_set_vertex_buffer;
    uint32_t num_skip_redundant_vertex_buffer;
    uint32_t num_set_index_buffer;
    uint32_t num_skip_redundant_index_buffer;
    uint32_t num_create_bindgroup;
    uint32_t num_discard_bindgroup;
    uint32_t num_set_bindgroup;
    uint32_t num_skip_redundant_bindgroup;
    uint32_t num_bindgroup_cache_hits;
    uint32_t num_bindgroup_cache_misses;
    uint32_t num_bindgroup_cache_collisions;
    uint32_t num_bindgroup_cache_hash_vs_key_mismatch;
} sg_frame_stats_wgpu_bindings;

typedef struct sg_frame_stats_wgpu {
    sg_frame_stats_wgpu_uniforms uniforms;
    sg_frame_stats_wgpu_bindings bindings;
} sg_frame_stats_wgpu;

typedef struct sg_frame_stats {
    uint32_t frame_index;   // current frame counter, starts at 0

    uint32_t num_passes;
    uint32_t num_apply_viewport;
    uint32_t num_apply_scissor_rect;
    uint32_t num_apply_pipeline;
    uint32_t num_apply_bindings;
    uint32_t num_apply_uniforms;
    uint32_t num_draw;
    uint32_t num_update_buffer;
    uint32_t num_append_buffer;
    uint32_t num_update_image;

    uint32_t size_apply_uniforms;
    uint32_t size_update_buffer;
    uint32_t size_append_buffer;
    uint32_t size_update_image;

    sg_frame_stats_gl gl;
    sg_frame_stats_d3d11 d3d11;
    sg_frame_stats_metal metal;
    sg_frame_stats_wgpu wgpu;
} sg_frame_stats;

/*
    sg_log_item

    An enum with a unique item for each log message, warning, error
    and validation layer message.
*/
#define _SG_LOG_ITEMS \
    _SG_LOGITEM_XMACRO(OK, "Ok") \
    _SG_LOGITEM_XMACRO(MALLOC_FAILED, "memory allocation failed") \
    _SG_LOGITEM_XMACRO(GL_TEXTURE_FORMAT_NOT_SUPPORTED, "pixel format not supported for texture (gl)") \
    _SG_LOGITEM_XMACRO(GL_3D_TEXTURES_NOT_SUPPORTED, "3d textures not supported (gl)") \
    _SG_LOGITEM_XMACRO(GL_ARRAY_TEXTURES_NOT_SUPPORTED, "array textures not supported (gl)") \
    _SG_LOGITEM_XMACRO(GL_SHADER_COMPILATION_FAILED, "shader compilation failed (gl)") \
    _SG_LOGITEM_XMACRO(GL_SHADER_LINKING_FAILED, "shader linking failed (gl)") \
    _SG_LOGITEM_XMACRO(GL_VERTEX_ATTRIBUTE_NOT_FOUND_IN_SHADER, "vertex attribute not found in shader (gl)") \
    _SG_LOGITEM_XMACRO(GL_TEXTURE_NAME_NOT_FOUND_IN_SHADER, "texture name not found in shader (gl)") \
    _SG_LOGITEM_XMACRO(GL_FRAMEBUFFER_STATUS_UNDEFINED, "framebuffer completeness check failed with GL_FRAMEBUFFER_UNDEFINED (gl)") \
    _SG_LOGITEM_XMACRO(GL_FRAMEBUFFER_STATUS_INCOMPLETE_ATTACHMENT, "framebuffer completeness check failed with GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT (gl)") \
    _SG_LOGITEM_XMACRO(GL_FRAMEBUFFER_STATUS_INCOMPLETE_MISSING_ATTACHMENT, "framebuffer completeness check failed with GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT (gl)") \
    _SG_LOGITEM_XMACRO(GL_FRAMEBUFFER_STATUS_UNSUPPORTED, "framebuffer completeness check failed with GL_FRAMEBUFFER_UNSUPPORTED (gl)") \
    _SG_LOGITEM_XMACRO(GL_FRAMEBUFFER_STATUS_INCOMPLETE_MULTISAMPLE, "framebuffer completeness check failed with GL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE (gl)") \
    _SG_LOGITEM_XMACRO(GL_FRAMEBUFFER_STATUS_UNKNOWN, "framebuffer completeness check failed (unknown reason) (gl)") \
    _SG_LOGITEM_XMACRO(D3D11_CREATE_BUFFER_FAILED, "CreateBuffer() failed (d3d11)") \
    _SG_LOGITEM_XMACRO(D3D11_CREATE_DEPTH_TEXTURE_UNSUPPORTED_PIXEL_FORMAT, "pixel format not supported for depth-stencil texture (d3d11)") \
    _SG_LOGITEM_XMACRO(D3D11_CREATE_DEPTH_TEXTURE_FAILED, "CreateTexture2D() failed for depth-stencil texture (d3d11)") \
    _SG_LOGITEM_XMACRO(D3D11_CREATE_2D_TEXTURE_UNSUPPORTED_PIXEL_FORMAT, "pixel format not supported for 2d-, cube- or array-texture (d3d11)") \
    _SG_LOGITEM_XMACRO(D3D11_CREATE_2D_TEXTURE_FAILED, "CreateTexture2D() failed for 2d-, cube- or array-texture (d3d11)") \
    _SG_LOGITEM_XMACRO(D3D11_CREATE_2D_SRV_FAILED, "CreateShaderResourceView() failed for 2d-, cube- or array-texture (d3d11)") \
    _SG_LOGITEM_XMACRO(D3D11_CREATE_3D_TEXTURE_UNSUPPORTED_PIXEL_FORMAT, "pixel format not supported for 3D texture (d3d11)") \
    _SG_LOGITEM_XMACRO(D3D11_CREATE_3D_TEXTURE_FAILED, "CreateTexture3D() failed (d3d11)") \
    _SG_LOGITEM_XMACRO(D3D11_CREATE_3D_SRV_FAILED, "CreateShaderResourceView() failed for 3d texture (d3d11)") \
    _SG_LOGITEM_XMACRO(D3D11_CREATE_MSAA_TEXTURE_FAILED, "CreateTexture2D() failed for MSAA render target texture (d3d11)") \
    _SG_LOGITEM_XMACRO(D3D11_CREATE_SAMPLER_STATE_FAILED, "CreateSamplerState() failed (d3d11)") \
    _SG_LOGITEM_XMACRO(D3D11_LOAD_D3DCOMPILER_47_DLL_FAILED, "loading d3dcompiler_47.dll failed (d3d11)") \
    _SG_LOGITEM_XMACRO(D3D11_SHADER_COMPILATION_FAILED, "shader compilation failed (d3d11)") \
    _SG_LOGITEM_XMACRO(D3D11_SHADER_COMPILATION_OUTPUT, "") \
    _SG_LOGITEM_XMACRO(D3D11_CREATE_CONSTANT_BUFFER_FAILED, "CreateBuffer() failed for uniform constant buffer (d3d11)") \
    _SG_LOGITEM_XMACRO(D3D11_CREATE_INPUT_LAYOUT_FAILED, "CreateInputLayout() failed (d3d11)") \
    _SG_LOGITEM_XMACRO(D3D11_CREATE_RASTERIZER_STATE_FAILED, "CreateRasterizerState() failed (d3d11)") \
    _SG_LOGITEM_XMACRO(D3D11_CREATE_DEPTH_STENCIL_STATE_FAILED, "CreateDepthStencilState() failed (d3d11)") \
    _SG_LOGITEM_XMACRO(D3D11_CREATE_BLEND_STATE_FAILED, "CreateBlendState() failed (d3d11)") \
    _SG_LOGITEM_XMACRO(D3D11_CREATE_RTV_FAILED, "CreateRenderTargetView() failed (d3d11)") \
    _SG_LOGITEM_XMACRO(D3D11_CREATE_DSV_FAILED, "CreateDepthStencilView() failed (d3d11)") \
    _SG_LOGITEM_XMACRO(D3D11_MAP_FOR_UPDATE_BUFFER_FAILED, "Map() failed when updating buffer (d3d11)") \
    _SG_LOGITEM_XMACRO(D3D11_MAP_FOR_APPEND_BUFFER_FAILED, "Map() failed when appending to buffer (d3d11)") \
    _SG_LOGITEM_XMACRO(D3D11_MAP_FOR_UPDATE_IMAGE_FAILED, "Map() failed when updating image (d3d11)") \
    _SG_LOGITEM_XMACRO(METAL_CREATE_BUFFER_FAILED, "failed to create buffer object (metal)") \
    _SG_LOGITEM_XMACRO(METAL_TEXTURE_FORMAT_NOT_SUPPORTED, "pixel format not supported for texture (metal)") \
    _SG_LOGITEM_XMACRO(METAL_CREATE_TEXTURE_FAILED, "failed to create texture object (metal)") \
    _SG_LOGITEM_XMACRO(METAL_CREATE_SAMPLER_FAILED, "failed to create sampler object (metal)") \
    _SG_LOGITEM_XMACRO(METAL_SHADER_COMPILATION_FAILED, "shader compilation failed (metal)") \
    _SG_LOGITEM_XMACRO(METAL_SHADER_CREATION_FAILED, "shader creation failed (metal)") \
    _SG_LOGITEM_XMACRO(METAL_SHADER_COMPILATION_OUTPUT, "") \
    _SG_LOGITEM_XMACRO(METAL_VERTEX_SHADER_ENTRY_NOT_FOUND, "vertex shader entry function not found (metal)") \
    _SG_LOGITEM_XMACRO(METAL_FRAGMENT_SHADER_ENTRY_NOT_FOUND, "fragment shader entry not found (metal)") \
    _SG_LOGITEM_XMACRO(METAL_CREATE_RPS_FAILED, "failed to create render pipeline state (metal)") \
    _SG_LOGITEM_XMACRO(METAL_CREATE_RPS_OUTPUT, "") \
    _SG_LOGITEM_XMACRO(METAL_CREATE_DSS_FAILED, "failed to create depth stencil state (metal)") \
    _SG_LOGITEM_XMACRO(WGPU_BINDGROUPS_POOL_EXHAUSTED, "bindgroups pool exhausted (increase sg_desc.bindgroups_cache_size) (wgpu)") \
    _SG_LOGITEM_XMACRO(WGPU_BINDGROUPSCACHE_SIZE_GREATER_ONE, "sg_desc.wgpu_bindgroups_cache_size must be > 1 (wgpu)") \
    _SG_LOGITEM_XMACRO(WGPU_BINDGROUPSCACHE_SIZE_POW2, "sg_desc.wgpu_bindgroups_cache_size must be a power of 2 (wgpu)") \
    _SG_LOGITEM_XMACRO(WGPU_CREATEBINDGROUP_FAILED, "wgpuDeviceCreateBindGroup failed") \
    _SG_LOGITEM_XMACRO(WGPU_CREATE_BUFFER_FAILED, "wgpuDeviceCreateBuffer() failed") \
    _SG_LOGITEM_XMACRO(WGPU_CREATE_TEXTURE_FAILED, "wgpuDeviceCreateTexture() failed") \
    _SG_LOGITEM_XMACRO(WGPU_CREATE_TEXTURE_VIEW_FAILED, "wgpuTextureCreateView() failed") \
    _SG_LOGITEM_XMACRO(WGPU_CREATE_SAMPLER_FAILED, "wgpuDeviceCreateSampler() failed") \
    _SG_LOGITEM_XMACRO(WGPU_CREATE_SHADER_MODULE_FAILED, "wgpuDeviceCreateShaderModule() failed") \
    _SG_LOGITEM_XMACRO(WGPU_SHADER_TOO_MANY_IMAGES, "shader uses too many sampled images on shader stage (wgpu)") \
    _SG_LOGITEM_XMACRO(WGPU_SHADER_TOO_MANY_SAMPLERS, "shader uses too many samplers on shader stage (wgpu)") \
    _SG_LOGITEM_XMACRO(WGPU_SHADER_CREATE_BINDGROUP_LAYOUT_FAILED, "wgpuDeviceCreateBindGroupLayout() for shader stage failed") \
    _SG_LOGITEM_XMACRO(WGPU_CREATE_PIPELINE_LAYOUT_FAILED, "wgpuDeviceCreatePipelineLayout() failed") \
    _SG_LOGITEM_XMACRO(WGPU_CREATE_RENDER_PIPELINE_FAILED, "wgpuDeviceCreateRenderPipeline() failed") \
    _SG_LOGITEM_XMACRO(WGPU_PASS_CREATE_TEXTURE_VIEW_FAILED, "wgpuTextureCreateView() failed in create pass") \
    _SG_LOGITEM_XMACRO(UNINIT_BUFFER_ACTIVE_CONTEXT_MISMATCH, "active context mismatch in buffer uninit (must be same as for creation)") \
    _SG_LOGITEM_XMACRO(UNINIT_IMAGE_ACTIVE_CONTEXT_MISMATCH, "active context mismatch in image uninit (must be same as for creation)") \
    _SG_LOGITEM_XMACRO(UNINIT_SAMPLER_ACTIVE_CONTEXT_MISMATCH, "active context mismatch in sampler uninit (must be same as for creation)") \
    _SG_LOGITEM_XMACRO(UNINIT_SHADER_ACTIVE_CONTEXT_MISMATCH, "active context mismatch in shader uninit (must be same as for creation)") \
    _SG_LOGITEM_XMACRO(UNINIT_PIPELINE_ACTIVE_CONTEXT_MISMATCH, "active context mismatch in pipeline uninit (must be same as for creation)") \
    _SG_LOGITEM_XMACRO(UNINIT_PASS_ACTIVE_CONTEXT_MISMATCH, "active context mismatch in pass uninit (must be same as for creation)") \
    _SG_LOGITEM_XMACRO(IDENTICAL_COMMIT_LISTENER, "attempting to add identical commit listener") \
    _SG_LOGITEM_XMACRO(COMMIT_LISTENER_ARRAY_FULL, "commit listener array full") \
    _SG_LOGITEM_XMACRO(TRACE_HOOKS_NOT_ENABLED, "sg_install_trace_hooks() called, but SG_TRACE_HOOKS is not defined") \
    _SG_LOGITEM_XMACRO(DEALLOC_BUFFER_INVALID_STATE, "sg_dealloc_buffer(): buffer must be in ALLOC state") \
    _SG_LOGITEM_XMACRO(DEALLOC_IMAGE_INVALID_STATE, "sg_dealloc_image(): image must be in alloc state") \
    _SG_LOGITEM_XMACRO(DEALLOC_SAMPLER_INVALID_STATE, "sg_dealloc_sampler(): sampler must be in alloc state") \
    _SG_LOGITEM_XMACRO(DEALLOC_SHADER_INVALID_STATE, "sg_dealloc_shader(): shader must be in ALLOC state") \
    _SG_LOGITEM_XMACRO(DEALLOC_PIPELINE_INVALID_STATE, "sg_dealloc_pipeline(): pipeline must be in ALLOC state") \
    _SG_LOGITEM_XMACRO(DEALLOC_PASS_INVALID_STATE, "sg_dealloc_pass(): pass must be in ALLOC state") \
    _SG_LOGITEM_XMACRO(INIT_BUFFER_INVALID_STATE, "sg_init_buffer(): buffer must be in ALLOC state") \
    _SG_LOGITEM_XMACRO(INIT_IMAGE_INVALID_STATE, "sg_init_image(): image must be in ALLOC state") \
    _SG_LOGITEM_XMACRO(INIT_SAMPLER_INVALID_STATE, "sg_init_sampler(): sampler must be in ALLOC state") \
    _SG_LOGITEM_XMACRO(INIT_SHADER_INVALID_STATE, "sg_init_shader(): shader must be in ALLOC state") \
    _SG_LOGITEM_XMACRO(INIT_PIPELINE_INVALID_STATE, "sg_init_pipeline(): pipeline must be in ALLOC state") \
    _SG_LOGITEM_XMACRO(INIT_PASS_INVALID_STATE, "sg_init_pass(): pass must be in ALLOC state") \
    _SG_LOGITEM_XMACRO(UNINIT_BUFFER_INVALID_STATE, "sg_uninit_buffer(): buffer must be in VALID or FAILED state") \
    _SG_LOGITEM_XMACRO(UNINIT_IMAGE_INVALID_STATE, "sg_uninit_image(): image must be in VALID or FAILED state") \
    _SG_LOGITEM_XMACRO(UNINIT_SAMPLER_INVALID_STATE, "sg_uninit_sampler(): sampler must be in VALID or FAILED state") \
    _SG_LOGITEM_XMACRO(UNINIT_SHADER_INVALID_STATE, "sg_uninit_shader(): shader must be in VALID or FAILED state") \
    _SG_LOGITEM_XMACRO(UNINIT_PIPELINE_INVALID_STATE, "sg_uninit_pipeline(): pipeline must be in VALID or FAILED state") \
    _SG_LOGITEM_XMACRO(UNINIT_PASS_INVALID_STATE, "sg_uninit_pass(): pass must be in VALID or FAILED state") \
    _SG_LOGITEM_XMACRO(FAIL_BUFFER_INVALID_STATE, "sg_fail_buffer(): buffer must be in ALLOC state") \
    _SG_LOGITEM_XMACRO(FAIL_IMAGE_INVALID_STATE, "sg_fail_image(): image must be in ALLOC state") \
    _SG_LOGITEM_XMACRO(FAIL_SAMPLER_INVALID_STATE, "sg_fail_sampler(): sampler must be in ALLOC state") \
    _SG_LOGITEM_XMACRO(FAIL_SHADER_INVALID_STATE, "sg_fail_shader(): shader must be in ALLOC state") \
    _SG_LOGITEM_XMACRO(FAIL_PIPELINE_INVALID_STATE, "sg_fail_pipeline(): pipeline must be in ALLOC state") \
    _SG_LOGITEM_XMACRO(FAIL_PASS_INVALID_STATE, "sg_fail_pass(): pass must be in ALLOC state") \
    _SG_LOGITEM_XMACRO(BUFFER_POOL_EXHAUSTED, "buffer pool exhausted") \
    _SG_LOGITEM_XMACRO(IMAGE_POOL_EXHAUSTED, "image pool exhausted") \
    _SG_LOGITEM_XMACRO(SAMPLER_POOL_EXHAUSTED, "sampler pool exhausted") \
    _SG_LOGITEM_XMACRO(SHADER_POOL_EXHAUSTED, "shader pool exhausted") \
    _SG_LOGITEM_XMACRO(PIPELINE_POOL_EXHAUSTED, "pipeline pool exhausted") \
    _SG_LOGITEM_XMACRO(PASS_POOL_EXHAUSTED, "pass pool exhausted") \
    _SG_LOGITEM_XMACRO(DRAW_WITHOUT_BINDINGS, "attempting to draw without resource bindings") \
    _SG_LOGITEM_XMACRO(VALIDATE_BUFFERDESC_CANARY, "sg_buffer_desc not initialized") \
    _SG_LOGITEM_XMACRO(VALIDATE_BUFFERDESC_SIZE, "sg_buffer_desc.size and .data.size cannot both be 0") \
    _SG_LOGITEM_XMACRO(VALIDATE_BUFFERDESC_DATA, "immutable buffers must be initialized with data (sg_buffer_desc.data.ptr and sg_buffer_desc.data.size)") \
    _SG_LOGITEM_XMACRO(VALIDATE_BUFFERDESC_DATA_SIZE, "immutable buffer data size differs from buffer size") \
    _SG_LOGITEM_XMACRO(VALIDATE_BUFFERDESC_NO_DATA, "dynamic/stream usage buffers cannot be initialized with data") \
    _SG_LOGITEM_XMACRO(VALIDATE_IMAGEDATA_NODATA, "sg_image_data: no data (.ptr and/or .size is zero)") \
    _SG_LOGITEM_XMACRO(VALIDATE_IMAGEDATA_DATA_SIZE, "sg_image_data: data size doesn't match expected surface size") \
    _SG_LOGITEM_XMACRO(VALIDATE_IMAGEDESC_CANARY, "sg_image_desc not initialized") \
    _SG_LOGITEM_XMACRO(VALIDATE_IMAGEDESC_WIDTH, "sg_image_desc.width must be > 0") \
    _SG_LOGITEM_XMACRO(VALIDATE_IMAGEDESC_HEIGHT, "sg_image_desc.height must be > 0") \
    _SG_LOGITEM_XMACRO(VALIDATE_IMAGEDESC_RT_PIXELFORMAT, "invalid pixel format for render-target image") \
    _SG_LOGITEM_XMACRO(VALIDATE_IMAGEDESC_NONRT_PIXELFORMAT, "invalid pixel format for non-render-target image") \
    _SG_LOGITEM_XMACRO(VALIDATE_IMAGEDESC_MSAA_BUT_NO_RT, "non-render-target images cannot be multisampled") \
    _SG_LOGITEM_XMACRO(VALIDATE_IMAGEDESC_NO_MSAA_RT_SUPPORT, "MSAA not supported for this pixel format") \
    _SG_LOGITEM_XMACRO(VALIDATE_IMAGEDESC_MSAA_NUM_MIPMAPS, "MSAA images must have num_mipmaps == 1") \
    _SG_LOGITEM_XMACRO(VALIDATE_IMAGEDESC_MSAA_3D_IMAGE, "3D images cannot have a sample_count > 1") \
    _SG_LOGITEM_XMACRO(VALIDATE_IMAGEDESC_DEPTH_3D_IMAGE, "3D images cannot have a depth/stencil image format") \
    _SG_LOGITEM_XMACRO(VALIDATE_IMAGEDESC_RT_IMMUTABLE, "render target images must be SG_USAGE_IMMUTABLE") \
    _SG_LOGITEM_XMACRO(VALIDATE_IMAGEDESC_RT_NO_DATA, "render target images cannot be initialized with data") \
    _SG_LOGITEM_XMACRO(VALIDATE_IMAGEDESC_INJECTED_NO_DATA, "images with injected textures cannot be initialized with data") \
    _SG_LOGITEM_XMACRO(VALIDATE_IMAGEDESC_DYNAMIC_NO_DATA, "dynamic/stream images cannot be initialized with data") \
    _SG_LOGITEM_XMACRO(VALIDATE_IMAGEDESC_COMPRESSED_IMMUTABLE, "compressed images must be immutable") \
    _SG_LOGITEM_XMACRO(VALIDATE_SAMPLERDESC_CANARY, "sg_sampler_desc not initialized") \
    _SG_LOGITEM_XMACRO(VALIDATE_SAMPLERDESC_MINFILTER_NONE, "sg_sampler_desc.min_filter cannot be SG_FILTER_NONE") \
    _SG_LOGITEM_XMACRO(VALIDATE_SAMPLERDESC_MAGFILTER_NONE, "sg_sampler_desc.mag_filter cannot be SG_FILTER_NONE") \
    _SG_LOGITEM_XMACRO(VALIDATE_SAMPLERDESC_ANISTROPIC_REQUIRES_LINEAR_FILTERING, "sg_sampler_desc.max_anisotropy > 1 requires min/mag/mipmap_filter to be SG_FILTER_LINEAR") \
    _SG_LOGITEM_XMACRO(VALIDATE_SHADERDESC_CANARY, "sg_shader_desc not initialized") \
    _SG_LOGITEM_XMACRO(VALIDATE_SHADERDESC_SOURCE, "shader source code required") \
    _SG_LOGITEM_XMACRO(VALIDATE_SHADERDESC_BYTECODE, "shader byte code required") \
    _SG_LOGITEM_XMACRO(VALIDATE_SHADERDESC_SOURCE_OR_BYTECODE, "shader source or byte code required") \
    _SG_LOGITEM_XMACRO(VALIDATE_SHADERDESC_NO_BYTECODE_SIZE, "shader byte code length (in bytes) required") \
    _SG_LOGITEM_XMACRO(VALIDATE_SHADERDESC_NO_CONT_UBS, "shader uniform blocks must occupy continuous slots") \
    _SG_LOGITEM_XMACRO(VALIDATE_SHADERDESC_NO_CONT_UB_MEMBERS, "uniform block members must occupy continuous slots") \
    _SG_LOGITEM_XMACRO(VALIDATE_SHADERDESC_NO_UB_MEMBERS, "GL backend requires uniform block member declarations") \
    _SG_LOGITEM_XMACRO(VALIDATE_SHADERDESC_UB_MEMBER_NAME, "uniform block member name missing") \
    _SG_LOGITEM_XMACRO(VALIDATE_SHADERDESC_UB_SIZE_MISMATCH, "size of uniform block members doesn't match uniform block size") \
    _SG_LOGITEM_XMACRO(VALIDATE_SHADERDESC_UB_ARRAY_COUNT, "uniform array count must be >= 1") \
    _SG_LOGITEM_XMACRO(VALIDATE_SHADERDESC_UB_STD140_ARRAY_TYPE, "uniform arrays only allowed for FLOAT4, INT4, MAT4 in std140 layout") \
    _SG_LOGITEM_XMACRO(VALIDATE_SHADERDESC_NO_CONT_IMAGES, "shader stage images must occupy continuous slots (sg_shader_desc.vs|fs.images[])") \
    _SG_LOGITEM_XMACRO(VALIDATE_SHADERDESC_NO_CONT_SAMPLERS, "shader stage samplers must occupy continuous slots (sg_shader_desc.vs|fs.samplers[])") \
    _SG_LOGITEM_XMACRO(VALIDATE_SHADERDESC_IMAGE_SAMPLER_PAIR_IMAGE_SLOT_OUT_OF_RANGE, "shader stage: image-sampler-pair image slot index is out of range (sg_shader_desc.vs|fs.image_sampler_pairs[].image_slot)") \
    _SG_LOGITEM_XMACRO(VALIDATE_SHADERDESC_IMAGE_SAMPLER_PAIR_SAMPLER_SLOT_OUT_OF_RANGE, "shader stage: image-sampler-pair image slot index is out of range (sg_shader_desc.vs|fs.image_sampler_pairs[].sampler_slot)") \
    _SG_LOGITEM_XMACRO(VALIDATE_SHADERDESC_IMAGE_SAMPLER_PAIR_NAME_REQUIRED_FOR_GL, "shader stage: image-sampler-pairs must be named in GL (sg_shader_desc.vs|fs.image_sampler_pairs[].name)") \
    _SG_LOGITEM_XMACRO(VALIDATE_SHADERDESC_IMAGE_SAMPLER_PAIR_HAS_NAME_BUT_NOT_USED, "shader stage: image-sampler-pair has name but .used field not true") \
    _SG_LOGITEM_XMACRO(VALIDATE_SHADERDESC_IMAGE_SAMPLER_PAIR_HAS_IMAGE_BUT_NOT_USED, "shader stage: image-sampler-pair has .image_slot != 0 but .used field not true") \
    _SG_LOGITEM_XMACRO(VALIDATE_SHADERDESC_IMAGE_SAMPLER_PAIR_HAS_SAMPLER_BUT_NOT_USED, "shader stage: image-sampler-pair .sampler_slot != 0 but .used field not true") \
    _SG_LOGITEM_XMACRO(VALIDATE_SHADERDESC_NONFILTERING_SAMPLER_REQUIRED, "shader stage: image sample type UNFILTERABLE_FLOAT, UINT, SINT can only be used with NONFILTERING sampler") \
    _SG_LOGITEM_XMACRO(VALIDATE_SHADERDESC_COMPARISON_SAMPLER_REQUIRED, "shader stage: image sample type DEPTH can only be used with COMPARISON sampler") \
    _SG_LOGITEM_XMACRO(VALIDATE_SHADERDESC_IMAGE_NOT_REFERENCED_BY_IMAGE_SAMPLER_PAIRS, "shader stage: one or more images are note referenced by  (sg_shader_desc.vs|fs.image_sampler_pairs[].image_slot)") \
    _SG_LOGITEM_XMACRO(VALIDATE_SHADERDESC_SAMPLER_NOT_REFERENCED_BY_IMAGE_SAMPLER_PAIRS, "shader stage: one or more samplers are not referenced by image-sampler-pairs (sg_shader_desc.vs|fs.image_sampler_pairs[].sampler_slot)") \
    _SG_LOGITEM_XMACRO(VALIDATE_SHADERDESC_NO_CONT_IMAGE_SAMPLER_PAIRS, "shader stage image-sampler-pairs must occupy continuous slots (sg_shader_desc.vs|fs.image_samplers[])") \
    _SG_LOGITEM_XMACRO(VALIDATE_SHADERDESC_ATTR_SEMANTICS, "D3D11 backend requires vertex attribute semantics") \
    _SG_LOGITEM_XMACRO(VALIDATE_SHADERDESC_ATTR_STRING_TOO_LONG, "vertex attribute name/semantic string too long (max len 16)") \
    _SG_LOGITEM_XMACRO(VALIDATE_PIPELINEDESC_CANARY, "sg_pipeline_desc not initialized") \
    _SG_LOGITEM_XMACRO(VALIDATE_PIPELINEDESC_SHADER, "sg_pipeline_desc.shader missing or invalid") \
    _SG_LOGITEM_XMACRO(VALIDATE_PIPELINEDESC_NO_ATTRS, "sg_pipeline_desc.layout.attrs is empty or not continuous") \
    _SG_LOGITEM_XMACRO(VALIDATE_PIPELINEDESC_LAYOUT_STRIDE4, "sg_pipeline_desc.layout.buffers[].stride must be multiple of 4") \
    _SG_LOGITEM_XMACRO(VALIDATE_PIPELINEDESC_ATTR_SEMANTICS, "D3D11 missing vertex attribute semantics in shader") \
    _SG_LOGITEM_XMACRO(VALIDATE_PASSDESC_CANARY, "sg_pass_desc not initialized") \
    _SG_LOGITEM_XMACRO(VALIDATE_PASSDESC_NO_ATTACHMENTS, "sg_pass_desc no color or depth-stencil attachments") \
    _SG_LOGITEM_XMACRO(VALIDATE_PASSDESC_NO_CONT_COLOR_ATTS, "color attachments must occupy continuous slots") \
    _SG_LOGITEM_XMACRO(VALIDATE_PASSDESC_IMAGE, "pass attachment image is not valid") \
    _SG_LOGITEM_XMACRO(VALIDATE_PASSDESC_MIPLEVEL, "pass attachment mip level is bigger than image has mipmaps") \
    _SG_LOGITEM_XMACRO(VALIDATE_PASSDESC_FACE, "pass attachment image is cubemap, but face index is too big") \
    _SG_LOGITEM_XMACRO(VALIDATE_PASSDESC_LAYER, "pass attachment image is array texture, but layer index is too big") \
    _SG_LOGITEM_XMACRO(VALIDATE_PASSDESC_SLICE, "pass attachment image is 3d texture, but slice value is too big") \
    _SG_LOGITEM_XMACRO(VALIDATE_PASSDESC_IMAGE_NO_RT, "pass attachment image must be have render_target=true") \
    _SG_LOGITEM_XMACRO(VALIDATE_PASSDESC_COLOR_INV_PIXELFORMAT, "pass color-attachment images must be renderable color pixel format") \
    _SG_LOGITEM_XMACRO(VALIDATE_PASSDESC_DEPTH_INV_PIXELFORMAT, "pass depth-attachment image must be depth or depth-stencil pixel format") \
    _SG_LOGITEM_XMACRO(VALIDATE_PASSDESC_IMAGE_SIZES, "all pass attachments must have the same size") \
    _SG_LOGITEM_XMACRO(VALIDATE_PASSDESC_IMAGE_SAMPLE_COUNTS, "all pass attachments must have the same sample count") \
    _SG_LOGITEM_XMACRO(VALIDATE_PASSDESC_RESOLVE_COLOR_IMAGE_MSAA, "pass resolve attachments must have a color attachment image with sample count > 1") \
    _SG_LOGITEM_XMACRO(VALIDATE_PASSDESC_RESOLVE_IMAGE, "pass resolve attachment image not valid") \
    _SG_LOGITEM_XMACRO(VALIDATE_PASSDESC_RESOLVE_SAMPLE_COUNT, "pass resolve attachment image sample count must be 1") \
    _SG_LOGITEM_XMACRO(VALIDATE_PASSDESC_RESOLVE_MIPLEVEL, "pass resolve attachment mip level is bigger than image has mipmaps") \
    _SG_LOGITEM_XMACRO(VALIDATE_PASSDESC_RESOLVE_FACE, "pass resolve attachment is cubemap, but face index is too big") \
    _SG_LOGITEM_XMACRO(VALIDATE_PASSDESC_RESOLVE_LAYER, "pass resolve attachment is array texture, but layer index is too big") \
    _SG_LOGITEM_XMACRO(VALIDATE_PASSDESC_RESOLVE_SLICE, "pass resolve attachment is 3d texture, but slice value is too big") \
    _SG_LOGITEM_XMACRO(VALIDATE_PASSDESC_RESOLVE_IMAGE_NO_RT, "pass resolve attachment image must have render_target=true") \
    _SG_LOGITEM_XMACRO(VALIDATE_PASSDESC_RESOLVE_IMAGE_SIZES, "pass resolve attachment size must match color attachment image size") \
    _SG_LOGITEM_XMACRO(VALIDATE_PASSDESC_RESOLVE_IMAGE_FORMAT, "pass resolve attachment pixel format must match color attachment pixel format") \
    _SG_LOGITEM_XMACRO(VALIDATE_PASSDESC_DEPTH_IMAGE, "pass depth attachment image is not valid") \
    _SG_LOGITEM_XMACRO(VALIDATE_PASSDESC_DEPTH_MIPLEVEL, "pass depth attachment mip level is bigger than image has mipmaps") \
    _SG_LOGITEM_XMACRO(VALIDATE_PASSDESC_DEPTH_FACE, "pass depth attachment image is cubemap, but face index is too big") \
    _SG_LOGITEM_XMACRO(VALIDATE_PASSDESC_DEPTH_LAYER, "pass depth attachment image is array texture, but layer index is too big") \
    _SG_LOGITEM_XMACRO(VALIDATE_PASSDESC_DEPTH_SLICE, "pass depth attachment image is 3d texture, but slice value is too big") \
    _SG_LOGITEM_XMACRO(VALIDATE_PASSDESC_DEPTH_IMAGE_NO_RT, "pass depth attachment image must be have render_target=true") \
    _SG_LOGITEM_XMACRO(VALIDATE_PASSDESC_DEPTH_IMAGE_SIZES, "pass depth attachment image size must match color attachment image size") \
    _SG_LOGITEM_XMACRO(VALIDATE_PASSDESC_DEPTH_IMAGE_SAMPLE_COUNT, "pass depth attachment sample count must match color attachment sample count") \
    _SG_LOGITEM_XMACRO(VALIDATE_BEGINPASS_PASS, "sg_begin_pass: pass must be valid") \
    _SG_LOGITEM_XMACRO(VALIDATE_BEGINPASS_COLOR_ATTACHMENT_IMAGE, "sg_begin_pass: one or more color attachment images are not valid") \
    _SG_LOGITEM_XMACRO(VALIDATE_BEGINPASS_RESOLVE_ATTACHMENT_IMAGE, "sg_begin_pass: one or more resolve attachment images are not valid") \
    _SG_LOGITEM_XMACRO(VALIDATE_BEGINPASS_DEPTHSTENCIL_ATTACHMENT_IMAGE, "sg_begin_pass: one or more depth-stencil attachment images are not valid") \
    _SG_LOGITEM_XMACRO(VALIDATE_APIP_PIPELINE_VALID_ID, "sg_apply_pipeline: invalid pipeline id provided") \
    _SG_LOGITEM_XMACRO(VALIDATE_APIP_PIPELINE_EXISTS, "sg_apply_pipeline: pipeline object no longer alive") \
    _SG_LOGITEM_XMACRO(VALIDATE_APIP_PIPELINE_VALID, "sg_apply_pipeline: pipeline object not in valid state") \
    _SG_LOGITEM_XMACRO(VALIDATE_APIP_SHADER_EXISTS, "sg_apply_pipeline: shader object no longer alive") \
    _SG_LOGITEM_XMACRO(VALIDATE_APIP_SHADER_VALID, "sg_apply_pipeline: shader object not in valid state") \
    _SG_LOGITEM_XMACRO(VALIDATE_APIP_ATT_COUNT, "sg_apply_pipeline: number of pipeline color attachments doesn't match number of pass color attachments") \
    _SG_LOGITEM_XMACRO(VALIDATE_APIP_COLOR_FORMAT, "sg_apply_pipeline: pipeline color attachment pixel format doesn't match pass color attachment pixel format") \
    _SG_LOGITEM_XMACRO(VALIDATE_APIP_DEPTH_FORMAT, "sg_apply_pipeline: pipeline depth pixel_format doesn't match pass depth attachment pixel format") \
    _SG_LOGITEM_XMACRO(VALIDATE_APIP_SAMPLE_COUNT, "sg_apply_pipeline: pipeline MSAA sample count doesn't match render pass attachment sample count") \
    _SG_LOGITEM_XMACRO(VALIDATE_ABND_PIPELINE, "sg_apply_bindings: must be called after sg_apply_pipeline") \
    _SG_LOGITEM_XMACRO(VALIDATE_ABND_PIPELINE_EXISTS, "sg_apply_bindings: currently applied pipeline object no longer alive") \
    _SG_LOGITEM_XMACRO(VALIDATE_ABND_PIPELINE_VALID, "sg_apply_bindings: currently applied pipeline object not in valid state") \
    _SG_LOGITEM_XMACRO(VALIDATE_ABND_VBS, "sg_apply_bindings: number of vertex buffers doesn't match number of pipeline vertex layouts") \
    _SG_LOGITEM_XMACRO(VALIDATE_ABND_VB_EXISTS, "sg_apply_bindings: vertex buffer no longer alive") \
    _SG_LOGITEM_XMACRO(VALIDATE_ABND_VB_TYPE, "sg_apply_bindings: buffer in vertex buffer slot is not a SG_BUFFERTYPE_VERTEXBUFFER") \
    _SG_LOGITEM_XMACRO(VALIDATE_ABND_VB_OVERFLOW, "sg_apply_bindings: buffer in vertex buffer slot is overflown") \
    _SG_LOGITEM_XMACRO(VALIDATE_ABND_NO_IB, "sg_apply_bindings: pipeline object defines indexed rendering, but no index buffer provided") \
    _SG_LOGITEM_XMACRO(VALIDATE_ABND_IB, "sg_apply_bindings: pipeline object defines non-indexed rendering, but index buffer provided") \
    _SG_LOGITEM_XMACRO(VALIDATE_ABND_IB_EXISTS, "sg_apply_bindings: index buffer no longer alive") \
    _SG_LOGITEM_XMACRO(VALIDATE_ABND_IB_TYPE, "sg_apply_bindings: buffer in index buffer slot is not a SG_BUFFERTYPE_INDEXBUFFER") \
    _SG_LOGITEM_XMACRO(VALIDATE_ABND_IB_OVERFLOW, "sg_apply_bindings: buffer in index buffer slot is overflown") \
    _SG_LOGITEM_XMACRO(VALIDATE_ABND_VS_EXPECTED_IMAGE_BINDING, "sg_apply_bindings: image binding on vertex stage is missing or the image handle is invalid") \
    _SG_LOGITEM_XMACRO(VALIDATE_ABND_VS_IMG_EXISTS, "sg_apply_bindings: image bound to vertex stage no longer alive") \
    _SG_LOGITEM_XMACRO(VALIDATE_ABND_VS_IMAGE_TYPE_MISMATCH, "sg_apply_bindings: type of image bound to vertex stage doesn't match shader desc") \
    _SG_LOGITEM_XMACRO(VALIDATE_ABND_VS_IMAGE_MSAA, "sg_apply_bindings: cannot bind image with sample_count>1 to vertex stage") \
    _SG_LOGITEM_XMACRO(VALIDATE_ABND_VS_EXPECTED_FILTERABLE_IMAGE, "sg_apply_bindings: filterable image expected on vertex stage") \
    _SG_LOGITEM_XMACRO(VALIDATE_ABND_VS_EXPECTED_DEPTH_IMAGE, "sg_apply_bindings: depth image expected on vertex stage") \
    _SG_LOGITEM_XMACRO(VALIDATE_ABND_VS_UNEXPECTED_IMAGE_BINDING, "sg_apply_bindings: unexpected image binding on vertex stage") \
    _SG_LOGITEM_XMACRO(VALIDATE_ABND_VS_EXPECTED_SAMPLER_BINDING, "sg_apply_bindings: sampler binding on vertex stage is missing or the sampler handle is invalid") \
    _SG_LOGITEM_XMACRO(VALIDATE_ABND_VS_UNEXPECTED_SAMPLER_COMPARE_NEVER, "sg_apply_bindings: shader expects SG_SAMPLERTYPE_COMPARISON on vertex stage but sampler has SG_COMPAREFUNC_NEVER") \
    _SG_LOGITEM_XMACRO(VALIDATE_ABND_VS_EXPECTED_SAMPLER_COMPARE_NEVER, "sg_apply_bindings: shader expects SG_SAMPLERTYPE_FILTERING or SG_SAMPLERTYPE_NONFILTERING on vertex stage but sampler doesn't have SG_COMPAREFUNC_NEVER") \
    _SG_LOGITEM_XMACRO(VALIDATE_ABND_VS_EXPECTED_NONFILTERING_SAMPLER, "sg_apply_bindings: shader expected SG_SAMPLERTYPE_NONFILTERING on vertex stage, but sampler has SG_FILTER_LINEAR filters") \
    _SG_LOGITEM_XMACRO(VALIDATE_ABND_VS_UNEXPECTED_SAMPLER_BINDING, "sg_apply_bindings: unexpected sampler binding on vertex stage") \
    _SG_LOGITEM_XMACRO(VALIDATE_ABND_VS_SMP_EXISTS, "sg_apply_bindings: sampler bound to vertex stage no longer alive") \
    _SG_LOGITEM_XMACRO(VALIDATE_ABND_FS_EXPECTED_IMAGE_BINDING, "sg_apply_bindings: image binding on fragment stage is missing or the image handle is invalid") \
    _SG_LOGITEM_XMACRO(VALIDATE_ABND_FS_IMG_EXISTS, "sg_apply_bindings: image bound to fragment stage no longer alive") \
    _SG_LOGITEM_XMACRO(VALIDATE_ABND_FS_IMAGE_TYPE_MISMATCH, "sg_apply_bindings: type of image bound to fragment stage doesn't match shader desc") \
    _SG_LOGITEM_XMACRO(VALIDATE_ABND_FS_IMAGE_MSAA, "sg_apply_bindings: cannot bind image with sample_count>1 to fragment stage") \
    _SG_LOGITEM_XMACRO(VALIDATE_ABND_FS_EXPECTED_FILTERABLE_IMAGE, "sg_apply_bindings: filterable image expected on fragment stage") \
    _SG_LOGITEM_XMACRO(VALIDATE_ABND_FS_EXPECTED_DEPTH_IMAGE, "sg_apply_bindings: depth image expected on fragment stage") \
    _SG_LOGITEM_XMACRO(VALIDATE_ABND_FS_UNEXPECTED_IMAGE_BINDING, "sg_apply_bindings: unexpected image binding on fragment stage") \
    _SG_LOGITEM_XMACRO(VALIDATE_ABND_FS_EXPECTED_SAMPLER_BINDING, "sg_apply_bindings: sampler binding on fragment stage is missing or the sampler handle is invalid") \
    _SG_LOGITEM_XMACRO(VALIDATE_ABND_FS_UNEXPECTED_SAMPLER_COMPARE_NEVER, "sg_apply_bindings: shader expects SG_SAMPLERTYPE_COMPARISON on fragment stage but sampler has SG_COMPAREFUNC_NEVER") \
    _SG_LOGITEM_XMACRO(VALIDATE_ABND_FS_EXPECTED_SAMPLER_COMPARE_NEVER, "sg_apply_bindings: shader expects SG_SAMPLERTYPE_FILTERING on fragment stage but sampler doesn't have SG_COMPAREFUNC_NEVER") \
    _SG_LOGITEM_XMACRO(VALIDATE_ABND_FS_EXPECTED_NONFILTERING_SAMPLER, "sg_apply_bindings: shader expected SG_SAMPLERTYPE_NONFILTERING on fragment stage, but sampler has SG_FILTER_LINEAR filters") \
    _SG_LOGITEM_XMACRO(VALIDATE_ABND_FS_UNEXPECTED_SAMPLER_BINDING, "sg_apply_bindings: unexpected sampler binding on fragment stage") \
    _SG_LOGITEM_XMACRO(VALIDATE_ABND_FS_SMP_EXISTS, "sg_apply_bindings: sampler bound to fragment stage no longer alive") \
    _SG_LOGITEM_XMACRO(VALIDATE_AUB_NO_PIPELINE, "sg_apply_uniforms: must be called after sg_apply_pipeline()") \
    _SG_LOGITEM_XMACRO(VALIDATE_AUB_NO_UB_AT_SLOT, "sg_apply_uniforms: no uniform block declaration at this shader stage UB slot") \
    _SG_LOGITEM_XMACRO(VALIDATE_AUB_SIZE, "sg_apply_uniforms: data size doesn't match declared uniform block size") \
    _SG_LOGITEM_XMACRO(VALIDATE_UPDATEBUF_USAGE, "sg_update_buffer: cannot update immutable buffer") \
    _SG_LOGITEM_XMACRO(VALIDATE_UPDATEBUF_SIZE, "sg_update_buffer: update size is bigger than buffer size") \
    _SG_LOGITEM_XMACRO(VALIDATE_UPDATEBUF_ONCE, "sg_update_buffer: only one update allowed per buffer and frame") \
    _SG_LOGITEM_XMACRO(VALIDATE_UPDATEBUF_APPEND, "sg_update_buffer: cannot call sg_update_buffer and sg_append_buffer in same frame") \
    _SG_LOGITEM_XMACRO(VALIDATE_APPENDBUF_USAGE, "sg_append_buffer: cannot append to immutable buffer") \
    _SG_LOGITEM_XMACRO(VALIDATE_APPENDBUF_SIZE, "sg_append_buffer: overall appended size is bigger than buffer size") \
    _SG_LOGITEM_XMACRO(VALIDATE_APPENDBUF_UPDATE, "sg_append_buffer: cannot call sg_append_buffer and sg_update_buffer in same frame") \
    _SG_LOGITEM_XMACRO(VALIDATE_UPDIMG_USAGE, "sg_update_image: cannot update immutable image") \
    _SG_LOGITEM_XMACRO(VALIDATE_UPDIMG_ONCE, "sg_update_image: only one update allowed per image and frame") \
    _SG_LOGITEM_XMACRO(VALIDATION_FAILED, "validation layer checks failed") \

#define _SG_LOGITEM_XMACRO(item,msg) SG_LOGITEM_##item,
typedef enum sg_log_item {
    _SG_LOG_ITEMS
} sg_log_item;
#undef _SG_LOGITEM_XMACRO

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
    .sampler_pool_size      64
    .shader_pool_size       32
    .pipeline_pool_size     64
    .pass_pool_size         16
    .context_pool_size      16
    .uniform_buffer_size    4 MB (4*1024*1024)
    .max_commit_listeners   1024
    .disable_validation     false
    .mtl_force_managed_storage_mode false
    .wgpu_disable_bindgroups_cache  false
    .wgpu_bindgroups_cache_size     1024

    .allocator.alloc_fn     0 (in this case, malloc() will be called)
    .allocator.free_fn      0 (in this case, free() will be called)
    .allocator.user_data    0

    .context.color_format: default value depends on selected backend:
        all GL backends:    SG_PIXELFORMAT_RGBA8
        Metal and D3D11:    SG_PIXELFORMAT_BGRA8
        WebGPU:             *no default* (must be queried from swapchain)
    .context.depth_format   SG_PIXELFORMAT_DEPTH_STENCIL
    .context.sample_count   1

    Metal specific:
        (NOTE: All Objective-C object references are transferred through
        a bridged (const void*) to sokol_gfx, which will use a unretained
        bridged cast (__bridged id<xxx>) to retrieve the Objective-C
        references back. Since the bridge cast is unretained, the caller
        must hold a strong reference to the Objective-C object for the
        duration of the sokol_gfx call!

        .mtl_force_managed_storage_mode
            when enabled, Metal buffers and texture resources are created in managed storage
            mode, otherwise sokol-gfx will decide whether to create buffers and
            textures in managed or shared storage mode (this is mainly a debugging option)
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
        .context.d3d11.user_data
            optional user data pointer passed to the userdata versions of
            callback functions

    WebGPU specific:
        .wgpu_disable_bindgroups_cache
            When this is true, the WebGPU backend will create and immediately
            release a BindGroup object in the sg_apply_bindings() call, only
            use this for debugging purposes.
        .wgpu_bindgroups_cache_size
            The size of the bindgroups cache for re-using BindGroup objects
            between sg_apply_bindings() calls. The smaller the cache size,
            the more likely are cache slot collisions which will cause
            a BindGroups object to be destroyed and a new one created.
            Use the information returned by sg_query_stats() to check
            if this is a frequent occurence, and increase the cache size as
            needed (the default is 1024).
            NOTE: wgpu_bindgroups_cache_size must be a power-of-2 number!
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
            the pixel format of the default WGPUTextureView must be WGPUTextureFormat_Depth32FloatStencil8
        .context.wgpu.user_data
            optional user data pointer passed to the userdata versions of
            callback functions

    When using sokol_gfx.h and sokol_app.h together, consider using the
    helper function sapp_sgcontext() in the sokol_glue.h header to
    initialize the sg_desc.context nested struct. sapp_sgcontext() returns
    a completely initialized sg_context_desc struct with information
    provided by sokol_app.h.
*/
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
    const void* device;                    // WGPUDevice
    const void* (*render_view_cb)(void);   // returns WGPUTextureView
    const void* (*render_view_userdata_cb)(void*);
    const void* (*resolve_view_cb)(void);  // returns WGPUTextureView
    const void* (*resolve_view_userdata_cb)(void*);
    const void* (*depth_stencil_view_cb)(void);    // returns WGPUTextureView
    const void* (*depth_stencil_view_userdata_cb)(void*);
    void* user_data;
} sg_wgpu_context_desc;

typedef struct sg_gl_context_desc {
    uint32_t (*default_framebuffer_cb)(void);
    uint32_t (*default_framebuffer_userdata_cb)(void*);
    void* user_data;
} sg_gl_context_desc;

typedef struct sg_context_desc {
    sg_pixel_format color_format;
    sg_pixel_format depth_format;
    int sample_count;
    sg_metal_context_desc metal;
    sg_d3d11_context_desc d3d11;
    sg_wgpu_context_desc wgpu;
    sg_gl_context_desc gl;
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
    alloc_fn and free_fn function must be provided (e.g. it's not valid to
    override one function but not the other).
*/
typedef struct sg_allocator {
    void* (*alloc_fn)(size_t size, void* user_data);
    void (*free_fn)(void* ptr, void* user_data);
    void* user_data;
} sg_allocator;

/*
    sg_logger

    Used in sg_desc to provide a logging function. Please be aware
    that without logging function, sokol-gfx will be completely
    silent, e.g. it will not report errors, warnings and
    validation layer messages. For maximum error verbosity,
    compile in debug mode (e.g. NDEBUG *not* defined) and install
    a logger (for instance the standard logging function from sokol_log.h).
*/
typedef struct sg_logger {
    void (*func)(
        const char* tag,                // always "sg"
        uint32_t log_level,             // 0=panic, 1=error, 2=warning, 3=info
        uint32_t log_item_id,           // SG_LOGITEM_*
        const char* message_or_null,    // a message string, may be nullptr in release mode
        uint32_t line_nr,               // line number in sokol_gfx.h
        const char* filename_or_null,   // source filename, may be nullptr in release mode
        void* user_data);
    void* user_data;
} sg_logger;

typedef struct sg_desc {
    uint32_t _start_canary;
    int buffer_pool_size;
    int image_pool_size;
    int sampler_pool_size;
    int shader_pool_size;
    int pipeline_pool_size;
    int pass_pool_size;
    int context_pool_size;
    int uniform_buffer_size;
    int max_commit_listeners;
    bool disable_validation;    // disable validation layer even in debug mode, useful for tests
    bool mtl_force_managed_storage_mode; // for debugging: use Metal managed storage mode for resources even with UMA
    bool wgpu_disable_bindgroups_cache;  // set to true to disable the WebGPU backend BindGroup cache
    int wgpu_bindgroups_cache_size;      // number of slots in the WebGPU bindgroup cache (must be 2^N)
    sg_allocator allocator;
    sg_logger logger; // optional log function override
    sg_context_desc context;
    uint32_t _end_canary;
} sg_desc;

// setup and misc functions
SOKOL_GFX_API_DECL void sg_setup(const sg_desc* desc);
SOKOL_GFX_API_DECL void sg_shutdown(void);
SOKOL_GFX_API_DECL bool sg_isvalid(void);
SOKOL_GFX_API_DECL void sg_reset_state_cache(void);
SOKOL_GFX_API_DECL sg_trace_hooks sg_install_trace_hooks(const sg_trace_hooks* trace_hooks);
SOKOL_GFX_API_DECL void sg_push_debug_group(const char* name);
SOKOL_GFX_API_DECL void sg_pop_debug_group(void);
SOKOL_GFX_API_DECL bool sg_add_commit_listener(sg_commit_listener listener);
SOKOL_GFX_API_DECL bool sg_remove_commit_listener(sg_commit_listener listener);

// resource creation, destruction and updating
SOKOL_GFX_API_DECL sg_buffer sg_make_buffer(const sg_buffer_desc* desc);
SOKOL_GFX_API_DECL sg_image sg_make_image(const sg_image_desc* desc);
SOKOL_GFX_API_DECL sg_sampler sg_make_sampler(const sg_sampler_desc* desc);
SOKOL_GFX_API_DECL sg_shader sg_make_shader(const sg_shader_desc* desc);
SOKOL_GFX_API_DECL sg_pipeline sg_make_pipeline(const sg_pipeline_desc* desc);
SOKOL_GFX_API_DECL sg_pass sg_make_pass(const sg_pass_desc* desc);
SOKOL_GFX_API_DECL void sg_destroy_buffer(sg_buffer buf);
SOKOL_GFX_API_DECL void sg_destroy_image(sg_image img);
SOKOL_GFX_API_DECL void sg_destroy_sampler(sg_sampler smp);
SOKOL_GFX_API_DECL void sg_destroy_shader(sg_shader shd);
SOKOL_GFX_API_DECL void sg_destroy_pipeline(sg_pipeline pip);
SOKOL_GFX_API_DECL void sg_destroy_pass(sg_pass pass);
SOKOL_GFX_API_DECL void sg_update_buffer(sg_buffer buf, const sg_range* data);
SOKOL_GFX_API_DECL void sg_update_image(sg_image img, const sg_image_data* data);
SOKOL_GFX_API_DECL int sg_append_buffer(sg_buffer buf, const sg_range* data);
SOKOL_GFX_API_DECL bool sg_query_buffer_overflow(sg_buffer buf);
SOKOL_GFX_API_DECL bool sg_query_buffer_will_overflow(sg_buffer buf, size_t size);

// rendering functions
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

// getting information
SOKOL_GFX_API_DECL sg_desc sg_query_desc(void);
SOKOL_GFX_API_DECL sg_backend sg_query_backend(void);
SOKOL_GFX_API_DECL sg_features sg_query_features(void);
SOKOL_GFX_API_DECL sg_limits sg_query_limits(void);
SOKOL_GFX_API_DECL sg_pixelformat_info sg_query_pixelformat(sg_pixel_format fmt);
// get current state of a resource (INITIAL, ALLOC, VALID, FAILED, INVALID)
SOKOL_GFX_API_DECL sg_resource_state sg_query_buffer_state(sg_buffer buf);
SOKOL_GFX_API_DECL sg_resource_state sg_query_image_state(sg_image img);
SOKOL_GFX_API_DECL sg_resource_state sg_query_sampler_state(sg_sampler smp);
SOKOL_GFX_API_DECL sg_resource_state sg_query_shader_state(sg_shader shd);
SOKOL_GFX_API_DECL sg_resource_state sg_query_pipeline_state(sg_pipeline pip);
SOKOL_GFX_API_DECL sg_resource_state sg_query_pass_state(sg_pass pass);
// get runtime information about a resource
SOKOL_GFX_API_DECL sg_buffer_info sg_query_buffer_info(sg_buffer buf);
SOKOL_GFX_API_DECL sg_image_info sg_query_image_info(sg_image img);
SOKOL_GFX_API_DECL sg_sampler_info sg_query_sampler_info(sg_sampler smp);
SOKOL_GFX_API_DECL sg_shader_info sg_query_shader_info(sg_shader shd);
SOKOL_GFX_API_DECL sg_pipeline_info sg_query_pipeline_info(sg_pipeline pip);
SOKOL_GFX_API_DECL sg_pass_info sg_query_pass_info(sg_pass pass);
// get desc structs matching a specific resource (NOTE that not all creation attributes may be provided)
SOKOL_GFX_API_DECL sg_buffer_desc sg_query_buffer_desc(sg_buffer buf);
SOKOL_GFX_API_DECL sg_image_desc sg_query_image_desc(sg_image img);
SOKOL_GFX_API_DECL sg_sampler_desc sg_query_sampler_desc(sg_sampler smp);
SOKOL_GFX_API_DECL sg_shader_desc sg_query_shader_desc(sg_shader shd);
SOKOL_GFX_API_DECL sg_pipeline_desc sg_query_pipeline_desc(sg_pipeline pip);
SOKOL_GFX_API_DECL sg_pass_desc sg_query_pass_desc(sg_pass pass);
// get resource creation desc struct with their default values replaced
SOKOL_GFX_API_DECL sg_buffer_desc sg_query_buffer_defaults(const sg_buffer_desc* desc);
SOKOL_GFX_API_DECL sg_image_desc sg_query_image_defaults(const sg_image_desc* desc);
SOKOL_GFX_API_DECL sg_sampler_desc sg_query_sampler_defaults(const sg_sampler_desc* desc);
SOKOL_GFX_API_DECL sg_shader_desc sg_query_shader_defaults(const sg_shader_desc* desc);
SOKOL_GFX_API_DECL sg_pipeline_desc sg_query_pipeline_defaults(const sg_pipeline_desc* desc);
SOKOL_GFX_API_DECL sg_pass_desc sg_query_pass_defaults(const sg_pass_desc* desc);

// separate resource allocation and initialization (for async setup)
SOKOL_GFX_API_DECL sg_buffer sg_alloc_buffer(void);
SOKOL_GFX_API_DECL sg_image sg_alloc_image(void);
SOKOL_GFX_API_DECL sg_sampler sg_alloc_sampler(void);
SOKOL_GFX_API_DECL sg_shader sg_alloc_shader(void);
SOKOL_GFX_API_DECL sg_pipeline sg_alloc_pipeline(void);
SOKOL_GFX_API_DECL sg_pass sg_alloc_pass(void);
SOKOL_GFX_API_DECL void sg_dealloc_buffer(sg_buffer buf);
SOKOL_GFX_API_DECL void sg_dealloc_image(sg_image img);
SOKOL_GFX_API_DECL void sg_dealloc_sampler(sg_sampler smp);
SOKOL_GFX_API_DECL void sg_dealloc_shader(sg_shader shd);
SOKOL_GFX_API_DECL void sg_dealloc_pipeline(sg_pipeline pip);
SOKOL_GFX_API_DECL void sg_dealloc_pass(sg_pass pass);
SOKOL_GFX_API_DECL void sg_init_buffer(sg_buffer buf, const sg_buffer_desc* desc);
SOKOL_GFX_API_DECL void sg_init_image(sg_image img, const sg_image_desc* desc);
SOKOL_GFX_API_DECL void sg_init_sampler(sg_sampler smg, const sg_sampler_desc* desc);
SOKOL_GFX_API_DECL void sg_init_shader(sg_shader shd, const sg_shader_desc* desc);
SOKOL_GFX_API_DECL void sg_init_pipeline(sg_pipeline pip, const sg_pipeline_desc* desc);
SOKOL_GFX_API_DECL void sg_init_pass(sg_pass pass, const sg_pass_desc* desc);
SOKOL_GFX_API_DECL void sg_uninit_buffer(sg_buffer buf);
SOKOL_GFX_API_DECL void sg_uninit_image(sg_image img);
SOKOL_GFX_API_DECL void sg_uninit_sampler(sg_sampler smp);
SOKOL_GFX_API_DECL void sg_uninit_shader(sg_shader shd);
SOKOL_GFX_API_DECL void sg_uninit_pipeline(sg_pipeline pip);
SOKOL_GFX_API_DECL void sg_uninit_pass(sg_pass pass);
SOKOL_GFX_API_DECL void sg_fail_buffer(sg_buffer buf);
SOKOL_GFX_API_DECL void sg_fail_image(sg_image img);
SOKOL_GFX_API_DECL void sg_fail_sampler(sg_sampler smp);
SOKOL_GFX_API_DECL void sg_fail_shader(sg_shader shd);
SOKOL_GFX_API_DECL void sg_fail_pipeline(sg_pipeline pip);
SOKOL_GFX_API_DECL void sg_fail_pass(sg_pass pass);

// frame stats
SOKOL_GFX_API_DECL void sg_enable_frame_stats(void);
SOKOL_GFX_API_DECL void sg_disable_frame_stats(void);
SOKOL_GFX_API_DECL bool sg_frame_stats_enabled(void);
SOKOL_GFX_API_DECL sg_frame_stats sg_query_frame_stats(void);

// rendering contexts (optional)
SOKOL_GFX_API_DECL sg_context sg_setup_context(void);
SOKOL_GFX_API_DECL void sg_activate_context(sg_context ctx_id);
SOKOL_GFX_API_DECL void sg_discard_context(sg_context ctx_id);

/* Backend-specific structs and functions, these may come in handy for mixing
   sokol-gfx rendering with 'native backend' rendering functions.

   This group of functions will be expanded as needed.
*/

typedef struct sg_d3d11_buffer_info {
    const void* buf;      // ID3D11Buffer*
} sg_d3d11_buffer_info;

typedef struct sg_d3d11_image_info {
    const void* tex2d;    // ID3D11Texture2D*
    const void* tex3d;    // ID3D11Texture3D*
    const void* res;      // ID3D11Resource* (either tex2d or tex3d)
    const void* srv;      // ID3D11ShaderResourceView*
} sg_d3d11_image_info;

typedef struct sg_d3d11_sampler_info {
    const void* smp;      // ID3D11SamplerState*
} sg_d3d11_sampler_info;

typedef struct sg_d3d11_shader_info {
    const void* vs_cbufs[SG_MAX_SHADERSTAGE_UBS]; // ID3D11Buffer* (vertex stage constant buffers)
    const void* fs_cbufs[SG_MAX_SHADERSTAGE_UBS]; // ID3D11BUffer* (fragment stage constant buffers)
    const void* vs;   // ID3D11VertexShader*
    const void* fs;   // ID3D11PixelShader*
} sg_d3d11_shader_info;

typedef struct sg_d3d11_pipeline_info {
    const void* il;   // ID3D11InputLayout*
    const void* rs;   // ID3D11RasterizerState*
    const void* dss;  // ID3D11DepthStencilState*
    const void* bs;   // ID3D11BlendState*
} sg_d3d11_pipeline_info;

typedef struct sg_d3d11_pass_info {
    const void* color_rtv[SG_MAX_COLOR_ATTACHMENTS];      // ID3D11RenderTargetView
    const void* resolve_rtv[SG_MAX_COLOR_ATTACHMENTS];    // ID3D11RenderTargetView
    const void* dsv;  // ID3D11DepthStencilView
} sg_d3d11_pass_info;

typedef struct sg_mtl_buffer_info {
    const void* buf[SG_NUM_INFLIGHT_FRAMES];  // id<MTLBuffer>
    int active_slot;
} sg_mtl_buffer_info;

typedef struct sg_mtl_image_info {
    const void* tex[SG_NUM_INFLIGHT_FRAMES]; // id<MTLTexture>
    int active_slot;
} sg_mtl_image_info;

typedef struct sg_mtl_sampler_info {
    const void* smp;  // id<MTLSamplerState>
} sg_mtl_sampler_info;

typedef struct sg_mtl_shader_info {
    const void* vs_lib;   // id<MTLLibrary>
    const void* fs_lib;   // id<MTLLibrary>
    const void* vs_func;  // id<MTLFunction>
    const void* fs_func;  // id<MTLFunction>
} sg_mtl_shader_info;

typedef struct sg_mtl_pipeline_info {
    const void* rps;      // id<MTLRenderPipelineState>
    const void* dss;      // id<MTLDepthStencilState>
} sg_mtl_pipeline_info;

typedef struct sg_wgpu_buffer_info {
    const void* buf;  // WGPUBuffer
} sg_wgpu_buffer_info;

typedef struct sg_wgpu_image_info {
    const void* tex;  // WGPUTexture
    const void* view; // WGPUTextureView
} sg_wgpu_image_info;

typedef struct sg_wgpu_sampler_info {
    const void* smp;  // WGPUSampler
} sg_wgpu_sampler_info;

typedef struct sg_wgpu_shader_info {
    const void* vs_mod;   // WGPUShaderModule
    const void* fs_mod;   // WGPUShaderModule
    const void* bgl;      // WGPUBindGroupLayout;
} sg_wgpu_shader_info;

typedef struct sg_wgpu_pipeline_info {
    const void* pip;      // WGPURenderPipeline
} sg_wgpu_pipeline_info;

typedef struct sg_wgpu_pass_info {
    const void* color_view[SG_MAX_COLOR_ATTACHMENTS];     // WGPUTextureView
    const void* resolve_view[SG_MAX_COLOR_ATTACHMENTS];    // WGPUTextureView
    const void* ds_view;  // WGPUTextureView
} sg_wgpu_pass_info;

typedef struct sg_gl_buffer_info {
    uint32_t buf[SG_NUM_INFLIGHT_FRAMES];
    int active_slot;
} sg_gl_buffer_info;

typedef struct sg_gl_image_info {
    uint32_t tex[SG_NUM_INFLIGHT_FRAMES];
    uint32_t tex_target;
    uint32_t msaa_render_buffer;
    int active_slot;
} sg_gl_image_info;

typedef struct sg_gl_sampler_info {
    uint32_t smp;
} sg_gl_sampler_info;

typedef struct sg_gl_shader_info {
    uint32_t prog;
} sg_gl_shader_info;

typedef struct sg_gl_pass_info {
    uint32_t frame_buffer;
    uint32_t msaa_resolve_framebuffer[SG_MAX_COLOR_ATTACHMENTS];
} sg_gl_pass_info;

// D3D11: return ID3D11Device
SOKOL_GFX_API_DECL const void* sg_d3d11_device(void);
// D3D11: return ID3D11DeviceContext
SOKOL_GFX_API_DECL const void* sg_d3d11_device_context(void);
// D3D11: get internal buffer resource objects
SOKOL_GFX_API_DECL sg_d3d11_buffer_info sg_d3d11_query_buffer_info(sg_buffer buf);
// D3D11: get internal image resource objects
SOKOL_GFX_API_DECL sg_d3d11_image_info sg_d3d11_query_image_info(sg_image img);
// D3D11: get internal sampler resource objects
SOKOL_GFX_API_DECL sg_d3d11_sampler_info sg_d3d11_query_sampler_info(sg_sampler smp);
// D3D11: get internal shader resource objects
SOKOL_GFX_API_DECL sg_d3d11_shader_info sg_d3d11_query_shader_info(sg_shader shd);
// D3D11: get internal pipeline resource objects
SOKOL_GFX_API_DECL sg_d3d11_pipeline_info sg_d3d11_query_pipeline_info(sg_pipeline pip);
// D3D11: get internal pass resource objects
SOKOL_GFX_API_DECL sg_d3d11_pass_info sg_d3d11_query_pass_info(sg_pass pass);

// Metal: return __bridge-casted MTLDevice
SOKOL_GFX_API_DECL const void* sg_mtl_device(void);
// Metal: return __bridge-casted MTLRenderCommandEncoder in current pass (or zero if outside pass)
SOKOL_GFX_API_DECL const void* sg_mtl_render_command_encoder(void);
// Metal: get internal __bridge-casted buffer resource objects
SOKOL_GFX_API_DECL sg_mtl_buffer_info sg_mtl_query_buffer_info(sg_buffer buf);
// Metal: get internal __bridge-casted image resource objects
SOKOL_GFX_API_DECL sg_mtl_image_info sg_mtl_query_image_info(sg_image img);
// Metal: get internal __bridge-casted sampler resource objects
SOKOL_GFX_API_DECL sg_mtl_sampler_info sg_mtl_query_sampler_info(sg_sampler smp);
// Metal: get internal __bridge-casted shader resource objects
SOKOL_GFX_API_DECL sg_mtl_shader_info sg_mtl_query_shader_info(sg_shader shd);
// Metal: get internal __bridge-casted pipeline resource objects
SOKOL_GFX_API_DECL sg_mtl_pipeline_info sg_mtl_query_pipeline_info(sg_pipeline pip);

// WebGPU: return WGPUDevice object
SOKOL_GFX_API_DECL const void* sg_wgpu_device(void);
// WebGPU: return WGPUQueue object
SOKOL_GFX_API_DECL const void* sg_wgpu_queue(void);
// WebGPU: return this frame's WGPUCommandEncoder
SOKOL_GFX_API_DECL const void* sg_wgpu_command_encoder(void);
// WebGPU: return WGPURenderPassEncoder of currrent pass
SOKOL_GFX_API_DECL const void* sg_wgpu_render_pass_encoder(void);
// WebGPU: get internal buffer resource objects
SOKOL_GFX_API_DECL sg_wgpu_buffer_info sg_wgpu_query_buffer_info(sg_buffer buf);
// WebGPU: get internal image resource objects
SOKOL_GFX_API_DECL sg_wgpu_image_info sg_wgpu_query_image_info(sg_image img);
// WebGPU: get internal sampler resource objects
SOKOL_GFX_API_DECL sg_wgpu_sampler_info sg_wgpu_query_sampler_info(sg_sampler smp);
// WebGPU: get internal shader resource objects
SOKOL_GFX_API_DECL sg_wgpu_shader_info sg_wgpu_query_shader_info(sg_shader shd);
// WebGPU: get internal pipeline resource objects
SOKOL_GFX_API_DECL sg_wgpu_pipeline_info sg_wgpu_query_pipeline_info(sg_pipeline pip);
// WebGPU: get internal pass resource objects
SOKOL_GFX_API_DECL sg_wgpu_pass_info sg_wgpu_query_pass_info(sg_pass pass);

// GL: get internal buffer resource objects
SOKOL_GFX_API_DECL sg_gl_buffer_info sg_gl_query_buffer_info(sg_buffer buf);
// GL: get internal image resource objects
SOKOL_GFX_API_DECL sg_gl_image_info sg_gl_query_image_info(sg_image img);
// GL: get internal sampler resource objects
SOKOL_GFX_API_DECL sg_gl_sampler_info sg_gl_query_sampler_info(sg_sampler smp);
// GL: get internal shader resource objects
SOKOL_GFX_API_DECL sg_gl_shader_info sg_gl_query_shader_info(sg_shader shd);
// GL: get internal pass resource objects
SOKOL_GFX_API_DECL sg_gl_pass_info sg_gl_query_pass_info(sg_pass pass);

#ifdef __cplusplus
} // extern "C"

// reference-based equivalents for c++
inline void sg_setup(const sg_desc& desc) { return sg_setup(&desc); }

inline sg_buffer sg_make_buffer(const sg_buffer_desc& desc) { return sg_make_buffer(&desc); }
inline sg_image sg_make_image(const sg_image_desc& desc) { return sg_make_image(&desc); }
inline sg_sampler sg_make_sampler(const sg_sampler_desc& desc) { return sg_make_sampler(&desc); }
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
inline sg_sampler_desc sg_query_sampler_defaults(const sg_sampler_desc& desc) { return sg_query_sampler_defaults(&desc); }
inline sg_shader_desc sg_query_shader_defaults(const sg_shader_desc& desc) { return sg_query_shader_defaults(&desc); }
inline sg_pipeline_desc sg_query_pipeline_defaults(const sg_pipeline_desc& desc) { return sg_query_pipeline_defaults(&desc); }
inline sg_pass_desc sg_query_pass_defaults(const sg_pass_desc& desc) { return sg_query_pass_defaults(&desc); }

inline void sg_init_buffer(sg_buffer buf, const sg_buffer_desc& desc) { return sg_init_buffer(buf, &desc); }
inline void sg_init_image(sg_image img, const sg_image_desc& desc) { return sg_init_image(img, &desc); }
inline void sg_init_sampler(sg_sampler smp, const sg_sampler_desc& desc) { return sg_init_sampler(smp, &desc); }
inline void sg_init_shader(sg_shader shd, const sg_shader_desc& desc) { return sg_init_shader(shd, &desc); }
inline void sg_init_pipeline(sg_pipeline pip, const sg_pipeline_desc& desc) { return sg_init_pipeline(pip, &desc); }
inline void sg_init_pass(sg_pass pass, const sg_pass_desc& desc) { return sg_init_pass(pass, &desc); }

inline void sg_update_buffer(sg_buffer buf_id, const sg_range& data) { return sg_update_buffer(buf_id, &data); }
inline int sg_append_buffer(sg_buffer buf_id, const sg_range& data) { return sg_append_buffer(buf_id, &data); }
#endif
#endif // SOKOL_GFX_INCLUDED

// ██ ███    ███ ██████  ██      ███████ ███    ███ ███████ ███    ██ ████████  █████  ████████ ██  ██████  ███    ██
// ██ ████  ████ ██   ██ ██      ██      ████  ████ ██      ████   ██    ██    ██   ██    ██    ██ ██    ██ ████   ██
// ██ ██ ████ ██ ██████  ██      █████   ██ ████ ██ █████   ██ ██  ██    ██    ███████    ██    ██ ██    ██ ██ ██  ██
// ██ ██  ██  ██ ██      ██      ██      ██  ██  ██ ██      ██  ██ ██    ██    ██   ██    ██    ██ ██    ██ ██  ██ ██
// ██ ██      ██ ██      ███████ ███████ ██      ██ ███████ ██   ████    ██    ██   ██    ██    ██  ██████  ██   ████
//
// >>implementation
#ifdef SOKOL_GFX_IMPL
#define SOKOL_GFX_IMPL_INCLUDED (1)

#if !(defined(SOKOL_GLCORE33)||defined(SOKOL_GLES3)||defined(SOKOL_D3D11)||defined(SOKOL_METAL)||defined(SOKOL_WGPU)||defined(SOKOL_DUMMY_BACKEND))
#error "Please select a backend with SOKOL_GLCORE33, SOKOL_GLES3, SOKOL_D3D11, SOKOL_METAL, SOKOL_WGPU or SOKOL_DUMMY_BACKEND"
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
#ifndef SOKOL_UNREACHABLE
    #define SOKOL_UNREACHABLE SOKOL_ASSERT(false)
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

// default clear values
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
#pragma warning(disable:4115)   // named type definition in parentheses
#pragma warning(disable:4505)   // unreferenced local function has been removed
#pragma warning(disable:4201)   // nonstandard extension used: nameless struct/union (needed by d3d11.h)
#pragma warning(disable:4054)   // 'type cast': from function pointer
#pragma warning(disable:4055)   // 'type cast': from data pointer
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
    #pragma comment (lib, "kernel32")
    #pragma comment (lib, "user32")
    #pragma comment (lib, "dxgi")
    #pragma comment (lib, "d3d11")
    #endif
#elif defined(SOKOL_METAL)
    // see https://clang.llvm.org/docs/LanguageExtensions.html#automatic-reference-counting
    #if !defined(__cplusplus)
        #if __has_feature(objc_arc) && !__has_feature(objc_arc_fields)
            #error "sokol_gfx.h requires __has_feature(objc_arc_field) if ARC is enabled (use a more recent compiler version)"
        #endif
    #endif
    #include <TargetConditionals.h>
    #include <AvailabilityMacros.h>
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
    #include <webgpu/webgpu.h>
    #if defined(__EMSCRIPTEN__)
        #include <emscripten/emscripten.h>
    #endif
#elif defined(SOKOL_GLCORE33) || defined(SOKOL_GLES3)
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
        typedef int  GLint;
        #define GL_INT_2_10_10_10_REV 0x8D9F
        #define GL_R32F 0x822E
        #define GL_PROGRAM_POINT_SIZE 0x8642
        #define GL_DEPTH_ATTACHMENT 0x8D00
        #define GL_DEPTH_STENCIL_ATTACHMENT 0x821A
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
        #define GL_SRGB8_ALPHA8 0x8C43
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
        #define GL_DEPTH_COMPONENT32F 0x8CAC
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
        #define GL_UNPACK_ALIGNMENT 0x0CF5
        #define GL_FRAMEBUFFER_SRGB 0x8DB9
        #define GL_TEXTURE_COMPARE_MODE 0x884C
        #define GL_TEXTURE_COMPARE_FUNC 0x884D
        #define GL_COMPARE_REF_TO_TEXTURE 0x884E
        #define GL_TEXTURE_CUBE_MAP_SEAMLESS 0x884F
        #define GL_TEXTURE_MAX_LEVEL 0x813D
        #define GL_FRAMEBUFFER_UNDEFINED 0x8219
        #define GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT 0x8CD6
        #define GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT 0x8CD7
        #define GL_FRAMEBUFFER_UNSUPPORTED 0x8CDD
        #define GL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE 0x8D56
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
    #ifndef _SG_GL_CHECK_ERROR
    #define _SG_GL_CHECK_ERROR() { SOKOL_ASSERT(glGetError() == GL_NO_ERROR); }
    #endif
#endif

// ███████ ████████ ██████  ██    ██  ██████ ████████ ███████
// ██         ██    ██   ██ ██    ██ ██         ██    ██
// ███████    ██    ██████  ██    ██ ██         ██    ███████
//      ██    ██    ██   ██ ██    ██ ██         ██         ██
// ███████    ██    ██   ██  ██████   ██████    ██    ███████
//
// >>structs
// resource pool slots
typedef struct {
    uint32_t id;
    uint32_t ctx_id;
    sg_resource_state state;
} _sg_slot_t;

// resource pool housekeeping struct
typedef struct {
    int size;
    int queue_top;
    uint32_t* gen_ctrs;
    int* free_queue;
} _sg_pool_t;

_SOKOL_PRIVATE void _sg_init_pool(_sg_pool_t* pool, int num);
_SOKOL_PRIVATE void _sg_discard_pool(_sg_pool_t* pool);
_SOKOL_PRIVATE int _sg_pool_alloc_index(_sg_pool_t* pool);
_SOKOL_PRIVATE void _sg_pool_free_index(_sg_pool_t* pool, int slot_index);
_SOKOL_PRIVATE void _sg_reset_slot(_sg_slot_t* slot);
_SOKOL_PRIVATE uint32_t _sg_slot_alloc(_sg_pool_t* pool, _sg_slot_t* slot, int slot_index);
_SOKOL_PRIVATE int _sg_slot_index(uint32_t id);

// constants
enum {
    _SG_STRING_SIZE = 16,
    _SG_SLOT_SHIFT = 16,
    _SG_SLOT_MASK = (1<<_SG_SLOT_SHIFT)-1,
    _SG_MAX_POOL_SIZE = (1<<_SG_SLOT_SHIFT),
    _SG_DEFAULT_BUFFER_POOL_SIZE = 128,
    _SG_DEFAULT_IMAGE_POOL_SIZE = 128,
    _SG_DEFAULT_SAMPLER_POOL_SIZE = 64,
    _SG_DEFAULT_SHADER_POOL_SIZE = 32,
    _SG_DEFAULT_PIPELINE_POOL_SIZE = 64,
    _SG_DEFAULT_PASS_POOL_SIZE = 16,
    _SG_DEFAULT_CONTEXT_POOL_SIZE = 16,
    _SG_DEFAULT_UB_SIZE = 4 * 1024 * 1024,
    _SG_DEFAULT_MAX_COMMIT_LISTENERS = 1024,
    _SG_DEFAULT_WGPU_BINDGROUP_CACHE_SIZE = 1024,
};

// fixed-size string
typedef struct {
    char buf[_SG_STRING_SIZE];
} _sg_str_t;

// helper macros
#define _sg_def(val, def) (((val) == 0) ? (def) : (val))
#define _sg_def_flt(val, def) (((val) == 0.0f) ? (def) : (val))
#define _sg_min(a,b) (((a)<(b))?(a):(b))
#define _sg_max(a,b) (((a)>(b))?(a):(b))
#define _sg_clamp(v,v0,v1) (((v)<(v0))?(v0):(((v)>(v1))?(v1):(v)))
#define _sg_fequal(val,cmp,delta) ((((val)-(cmp))> -(delta))&&(((val)-(cmp))<(delta)))
#define _sg_ispow2(val) ((val&(val-1))==0)
#define _sg_stats_add(key,val) {if(_sg.stats_enabled){ _sg.stats.key+=val;}}

_SOKOL_PRIVATE void* _sg_malloc_clear(size_t size);
_SOKOL_PRIVATE void _sg_free(void* ptr);
_SOKOL_PRIVATE void _sg_clear(void* ptr, size_t size);

typedef struct {
    int size;
    int append_pos;
    bool append_overflow;
    uint32_t update_frame_index;
    uint32_t append_frame_index;
    int num_slots;
    int active_slot;
    sg_buffer_type type;
    sg_usage usage;
} _sg_buffer_common_t;

_SOKOL_PRIVATE void _sg_buffer_common_init(_sg_buffer_common_t* cmn, const sg_buffer_desc* desc) {
    cmn->size = (int)desc->size;
    cmn->append_pos = 0;
    cmn->append_overflow = false;
    cmn->update_frame_index = 0;
    cmn->append_frame_index = 0;
    cmn->num_slots = (desc->usage == SG_USAGE_IMMUTABLE) ? 1 : SG_NUM_INFLIGHT_FRAMES;
    cmn->active_slot = 0;
    cmn->type = desc->type;
    cmn->usage = desc->usage;
}

typedef struct {
    uint32_t upd_frame_index;
    int num_slots;
    int active_slot;
    sg_image_type type;
    bool render_target;
    int width;
    int height;
    int num_slices;
    int num_mipmaps;
    sg_usage usage;
    sg_pixel_format pixel_format;
    int sample_count;
} _sg_image_common_t;

_SOKOL_PRIVATE void _sg_image_common_init(_sg_image_common_t* cmn, const sg_image_desc* desc) {
    cmn->upd_frame_index = 0;
    cmn->num_slots = (desc->usage == SG_USAGE_IMMUTABLE) ? 1 : SG_NUM_INFLIGHT_FRAMES;
    cmn->active_slot = 0;
    cmn->type = desc->type;
    cmn->render_target = desc->render_target;
    cmn->width = desc->width;
    cmn->height = desc->height;
    cmn->num_slices = desc->num_slices;
    cmn->num_mipmaps = desc->num_mipmaps;
    cmn->usage = desc->usage;
    cmn->pixel_format = desc->pixel_format;
    cmn->sample_count = desc->sample_count;
}

typedef struct {
    sg_filter min_filter;
    sg_filter mag_filter;
    sg_filter mipmap_filter;
    sg_wrap wrap_u;
    sg_wrap wrap_v;
    sg_wrap wrap_w;
    float min_lod;
    float max_lod;
    sg_border_color border_color;
    sg_compare_func compare;
    uint32_t max_anisotropy;
} _sg_sampler_common_t;

_SOKOL_PRIVATE void _sg_sampler_common_init(_sg_sampler_common_t* cmn, const sg_sampler_desc* desc) {
    cmn->min_filter = desc->min_filter;
    cmn->mag_filter = desc->mag_filter;
    cmn->mipmap_filter = desc->mipmap_filter;
    cmn->wrap_u = desc->wrap_u;
    cmn->wrap_v = desc->wrap_v;
    cmn->wrap_w = desc->wrap_w;
    cmn->min_lod = desc->min_lod;
    cmn->max_lod = desc->max_lod;
    cmn->border_color = desc->border_color;
    cmn->compare = desc->compare;
    cmn->max_anisotropy = desc->max_anisotropy;
}

typedef struct {
    size_t size;
} _sg_shader_uniform_block_t;

typedef struct {
    sg_image_type image_type;
    sg_image_sample_type sample_type;
    bool multisampled;
} _sg_shader_image_t;

typedef struct {
    sg_sampler_type sampler_type;
} _sg_shader_sampler_t;

// combined image sampler mappings, only needed on GL
typedef struct {
    int image_slot;
    int sampler_slot;
} _sg_shader_image_sampler_t;

typedef struct {
    int num_uniform_blocks;
    int num_images;
    int num_samplers;
    int num_image_samplers;
    _sg_shader_uniform_block_t uniform_blocks[SG_MAX_SHADERSTAGE_UBS];
    _sg_shader_image_t images[SG_MAX_SHADERSTAGE_IMAGES];
    _sg_shader_sampler_t samplers[SG_MAX_SHADERSTAGE_SAMPLERS];
    _sg_shader_image_sampler_t image_samplers[SG_MAX_SHADERSTAGE_IMAGESAMPLERPAIRS];
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
            if (!img_desc->used) {
                break;
            }
            stage->images[img_index].multisampled = img_desc->multisampled;
            stage->images[img_index].image_type = img_desc->image_type;
            stage->images[img_index].sample_type = img_desc->sample_type;
            stage->num_images++;
        }
        SOKOL_ASSERT(stage->num_samplers == 0);
        for (int smp_index = 0; smp_index < SG_MAX_SHADERSTAGE_SAMPLERS; smp_index++) {
            const sg_shader_sampler_desc* smp_desc = &stage_desc->samplers[smp_index];
            if (!smp_desc->used) {
                break;
            }
            stage->samplers[smp_index].sampler_type = smp_desc->sampler_type;
            stage->num_samplers++;
        }
        SOKOL_ASSERT(stage->num_image_samplers == 0);
        for (int img_smp_index = 0; img_smp_index < SG_MAX_SHADERSTAGE_IMAGESAMPLERPAIRS; img_smp_index++) {
            const sg_shader_image_sampler_pair_desc* img_smp_desc = &stage_desc->image_sampler_pairs[img_smp_index];
            if (!img_smp_desc->used) {
                break;
            }
            SOKOL_ASSERT((img_smp_desc->image_slot >= 0) && (img_smp_desc->image_slot < stage->num_images));
            stage->image_samplers[img_smp_index].image_slot = img_smp_desc->image_slot;
            SOKOL_ASSERT((img_smp_desc->sampler_slot >= 0) && (img_smp_desc->sampler_slot < stage->num_samplers));
            stage->image_samplers[img_smp_index].sampler_slot = img_smp_desc->sampler_slot;
            stage->num_image_samplers++;
        }
    }
}

typedef struct {
    bool vertex_buffer_layout_active[SG_MAX_VERTEX_BUFFERS];
    bool use_instanced_draw;
    sg_shader shader_id;
    sg_vertex_layout_state layout;
    sg_depth_state depth;
    sg_stencil_state stencil;
    int color_count;
    sg_color_target_state colors[SG_MAX_COLOR_ATTACHMENTS];
    sg_primitive_type primitive_type;
    sg_index_type index_type;
    sg_cull_mode cull_mode;
    sg_face_winding face_winding;
    int sample_count;
    sg_color blend_color;
    bool alpha_to_coverage_enabled;
} _sg_pipeline_common_t;

_SOKOL_PRIVATE void _sg_pipeline_common_init(_sg_pipeline_common_t* cmn, const sg_pipeline_desc* desc) {
    SOKOL_ASSERT((desc->color_count >= 0) && (desc->color_count <= SG_MAX_COLOR_ATTACHMENTS));
    for (int i = 0; i < SG_MAX_VERTEX_BUFFERS; i++) {
        cmn->vertex_buffer_layout_active[i] = false;
    }
    cmn->use_instanced_draw = false;
    cmn->shader_id = desc->shader;
    cmn->layout = desc->layout;
    cmn->depth = desc->depth;
    cmn->stencil = desc->stencil;
    cmn->color_count = desc->color_count;
    for (int i = 0; i < desc->color_count; i++) {
        cmn->colors[i] = desc->colors[i];
    }
    cmn->primitive_type = desc->primitive_type;
    cmn->index_type = desc->index_type;
    cmn->cull_mode = desc->cull_mode;
    cmn->face_winding = desc->face_winding;
    cmn->sample_count = desc->sample_count;
    cmn->blend_color = desc->blend_color;
    cmn->alpha_to_coverage_enabled = desc->alpha_to_coverage_enabled;
}

typedef struct {
    sg_image image_id;
    int mip_level;
    int slice;
} _sg_pass_attachment_common_t;

typedef struct {
    int width;
    int height;
    int num_color_atts;
    _sg_pass_attachment_common_t color_atts[SG_MAX_COLOR_ATTACHMENTS];
    _sg_pass_attachment_common_t resolve_atts[SG_MAX_COLOR_ATTACHMENTS];
    _sg_pass_attachment_common_t ds_att;
} _sg_pass_common_t;

_SOKOL_PRIVATE void _sg_pass_attachment_common_init(_sg_pass_attachment_common_t* cmn, const sg_pass_attachment_desc* desc) {
    cmn->image_id = desc->image;
    cmn->mip_level = desc->mip_level;
    cmn->slice = desc->slice;
}

_SOKOL_PRIVATE void _sg_pass_common_init(_sg_pass_common_t* cmn, const sg_pass_desc* desc, int width, int height) {
    SOKOL_ASSERT((width > 0) && (height > 0));
    cmn->width = width;
    cmn->height = height;
    for (int i = 0; i < SG_MAX_COLOR_ATTACHMENTS; i++) {
        if (desc->color_attachments[i].image.id != SG_INVALID_ID) {
            cmn->num_color_atts++;
            _sg_pass_attachment_common_init(&cmn->color_atts[i], &desc->color_attachments[i]);
            _sg_pass_attachment_common_init(&cmn->resolve_atts[i], &desc->resolve_attachments[i]);
        }
    }
    if (desc->depth_stencil_attachment.image.id != SG_INVALID_ID) {
        _sg_pass_attachment_common_init(&cmn->ds_att, &desc->depth_stencil_attachment);
    }
}

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
    _sg_sampler_common_t cmn;
} _sg_dummy_sampler_t;
typedef _sg_dummy_sampler_t _sg_sampler_t;

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
        _sg_dummy_attachment_t resolve_atts[SG_MAX_COLOR_ATTACHMENTS];
        _sg_dummy_attachment_t ds_att;
    } dmy;
} _sg_dummy_pass_t;
typedef _sg_dummy_pass_t _sg_pass_t;
typedef _sg_pass_attachment_common_t _sg_pass_attachment_t;

typedef struct {
    _sg_slot_t slot;
} _sg_dummy_context_t;
typedef _sg_dummy_context_t _sg_context_t;

#elif defined(_SOKOL_ANY_GL)
typedef struct {
    _sg_slot_t slot;
    _sg_buffer_common_t cmn;
    struct {
        GLuint buf[SG_NUM_INFLIGHT_FRAMES];
        bool injected;  // if true, external buffers were injected with sg_buffer_desc.gl_buffers
    } gl;
} _sg_gl_buffer_t;
typedef _sg_gl_buffer_t _sg_buffer_t;

typedef struct {
    _sg_slot_t slot;
    _sg_image_common_t cmn;
    struct {
        GLenum target;
        GLuint msaa_render_buffer;
        GLuint tex[SG_NUM_INFLIGHT_FRAMES];
        bool injected;  // if true, external textures were injected with sg_image_desc.gl_textures
    } gl;
} _sg_gl_image_t;
typedef _sg_gl_image_t _sg_image_t;

typedef struct {
    _sg_slot_t slot;
    _sg_sampler_common_t cmn;
    struct {
        GLuint smp;
        bool injected;  // true if external sampler was injects in sg_sampler_desc.gl_sampler
    } gl;
} _sg_gl_sampler_t;
typedef _sg_gl_sampler_t _sg_sampler_t;

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
} _sg_gl_shader_image_sampler_t;

typedef struct {
    _sg_str_t name;
} _sg_gl_shader_attr_t;

typedef struct {
    _sg_gl_uniform_block_t uniform_blocks[SG_MAX_SHADERSTAGE_UBS];
    _sg_gl_shader_image_sampler_t image_samplers[SG_MAX_SHADERSTAGE_IMAGESAMPLERPAIRS];
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
    int8_t vb_index;        // -1 if attr is not enabled
    int8_t divisor;         // -1 if not initialized
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
} _sg_gl_attachment_t;

typedef struct {
    _sg_slot_t slot;
    _sg_pass_common_t cmn;
    struct {
        GLuint fb;
        _sg_gl_attachment_t color_atts[SG_MAX_COLOR_ATTACHMENTS];
        _sg_gl_attachment_t resolve_atts[SG_MAX_COLOR_ATTACHMENTS];
        _sg_gl_attachment_t ds_att;
        GLuint msaa_resolve_framebuffer[SG_MAX_COLOR_ATTACHMENTS];
    } gl;
} _sg_gl_pass_t;
typedef _sg_gl_pass_t _sg_pass_t;
typedef _sg_pass_attachment_common_t _sg_pass_attachment_t;

typedef struct {
    _sg_slot_t slot;
    GLuint vao;
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
    GLuint sampler;
} _sg_gl_cache_texture_sampler_bind_slot;

#define _SG_GL_TEXTURE_SAMPLER_CACHE_SIZE (SG_MAX_SHADERSTAGE_IMAGESAMPLERPAIRS * SG_NUM_SHADER_STAGES)

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
    _sg_gl_cache_texture_sampler_bind_slot texture_samplers[_SG_GL_TEXTURE_SAMPLER_CACHE_SIZE];
    _sg_gl_cache_texture_sampler_bind_slot stored_texture_sampler;
    int cur_ib_offset;
    GLenum cur_primitive_type;
    GLenum cur_index_type;
    GLenum cur_active_texture;
    _sg_pipeline_t* cur_pipeline;
    sg_pipeline cur_pipeline_id;
} _sg_gl_state_cache_t;

typedef struct {
    bool valid;
    bool in_pass;
    int cur_pass_width;
    int cur_pass_height;
    _sg_context_t* cur_context;
    _sg_pass_t* cur_pass;
    sg_pass cur_pass_id;
    _sg_gl_state_cache_t cache;
    bool ext_anisotropic;
    GLint max_anisotropy;
    sg_store_action color_store_actions[SG_MAX_COLOR_ATTACHMENTS];
    sg_store_action depth_store_action;
    sg_store_action stencil_store_action;
    #if _SOKOL_USE_WIN32_GL_LOADER
    HINSTANCE opengl32_dll;
    #endif
} _sg_gl_backend_t;

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
        ID3D11Resource* res;    // either tex2d or tex3d
        ID3D11ShaderResourceView* srv;
    } d3d11;
} _sg_d3d11_image_t;
typedef _sg_d3d11_image_t _sg_image_t;

typedef struct {
    _sg_slot_t slot;
    _sg_sampler_common_t cmn;
    struct {
        ID3D11SamplerState* smp;
    } d3d11;
} _sg_d3d11_sampler_t;
typedef _sg_d3d11_sampler_t _sg_sampler_t;

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
        UINT vb_strides[SG_MAX_VERTEX_BUFFERS];
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
    union {
        ID3D11RenderTargetView* rtv;
        ID3D11DepthStencilView* dsv;
    } view;
} _sg_d3d11_attachment_t;

typedef struct {
    _sg_slot_t slot;
    _sg_pass_common_t cmn;
    struct {
        _sg_d3d11_attachment_t color_atts[SG_MAX_COLOR_ATTACHMENTS];
        _sg_d3d11_attachment_t resolve_atts[SG_MAX_COLOR_ATTACHMENTS];
        _sg_d3d11_attachment_t ds_att;
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
    // on-demand loaded d3dcompiler_47.dll handles
    HINSTANCE d3dcompiler_dll;
    bool d3dcompiler_dll_load_failed;
    pD3DCompile D3DCompile_func;
    // global subresourcedata array for texture updates
    D3D11_SUBRESOURCE_DATA subres_data[SG_MAX_MIPMAPS * SG_MAX_TEXTUREARRAY_LAYERS];
} _sg_d3d11_backend_t;

#elif defined(SOKOL_METAL)

#if defined(_SG_TARGET_MACOS) || defined(_SG_TARGET_IOS_SIMULATOR)
#define _SG_MTL_UB_ALIGN (256)
#else
#define _SG_MTL_UB_ALIGN (16)
#endif
#define _SG_MTL_INVALID_SLOT_INDEX (0)

typedef struct {
    uint32_t frame_index;   // frame index at which it is safe to release this resource
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
        int buf[SG_NUM_INFLIGHT_FRAMES];  // index into _sg_mtl_pool
    } mtl;
} _sg_mtl_buffer_t;
typedef _sg_mtl_buffer_t _sg_buffer_t;

typedef struct {
    _sg_slot_t slot;
    _sg_image_common_t cmn;
    struct {
        int tex[SG_NUM_INFLIGHT_FRAMES];
    } mtl;
} _sg_mtl_image_t;
typedef _sg_mtl_image_t _sg_image_t;

typedef struct {
    _sg_slot_t slot;
    _sg_sampler_common_t cmn;
    struct {
        int sampler_state;
    } mtl;
} _sg_mtl_sampler_t;
typedef _sg_mtl_sampler_t _sg_sampler_t;

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
        _sg_mtl_attachment_t resolve_atts[SG_MAX_COLOR_ATTACHMENTS];
        _sg_mtl_attachment_t ds_att;
    } mtl;
} _sg_mtl_pass_t;
typedef _sg_mtl_pass_t _sg_pass_t;
typedef _sg_pass_attachment_common_t _sg_pass_attachment_t;

typedef struct {
    _sg_slot_t slot;
} _sg_mtl_context_t;
typedef _sg_mtl_context_t _sg_context_t;

// resouce binding state cache
typedef struct {
    const _sg_pipeline_t* cur_pipeline;
    sg_pipeline cur_pipeline_id;
    const _sg_buffer_t* cur_indexbuffer;
    int cur_indexbuffer_offset;
    sg_buffer cur_indexbuffer_id;
    const _sg_buffer_t* cur_vertexbuffers[SG_MAX_VERTEX_BUFFERS];
    int cur_vertexbuffer_offsets[SG_MAX_VERTEX_BUFFERS];
    sg_buffer cur_vertexbuffer_ids[SG_MAX_VERTEX_BUFFERS];
    const _sg_image_t* cur_vs_images[SG_MAX_SHADERSTAGE_IMAGES];
    sg_image cur_vs_image_ids[SG_MAX_SHADERSTAGE_IMAGES];
    const _sg_image_t* cur_fs_images[SG_MAX_SHADERSTAGE_IMAGES];
    sg_image cur_fs_image_ids[SG_MAX_SHADERSTAGE_IMAGES];
    const _sg_sampler_t* cur_vs_samplers[SG_MAX_SHADERSTAGE_SAMPLERS];
    sg_sampler cur_vs_sampler_ids[SG_MAX_SHADERSTAGE_SAMPLERS];
    const _sg_sampler_t* cur_fs_samplers[SG_MAX_SHADERSTAGE_SAMPLERS];
    sg_sampler cur_fs_sampler_ids[SG_MAX_SHADERSTAGE_SAMPLERS];
} _sg_mtl_state_cache_t;

typedef struct {
    bool valid;
    bool use_shared_storage_mode;
    const void*(*renderpass_descriptor_cb)(void);
    const void*(*renderpass_descriptor_userdata_cb)(void*);
    const void*(*drawable_cb)(void);
    const void*(*drawable_userdata_cb)(void*);
    void* user_data;
    uint32_t cur_frame_rotate_index;
    int ub_size;
    int cur_ub_offset;
    uint8_t* cur_ub_base_ptr;
    bool in_pass;
    bool pass_valid;
    int cur_width;
    int cur_height;
    _sg_mtl_state_cache_t state_cache;
    _sg_mtl_idpool_t idpool;
    dispatch_semaphore_t sem;
    id<MTLDevice> device;
    id<MTLCommandQueue> cmd_queue;
    id<MTLCommandBuffer> cmd_buffer;
    id<MTLRenderCommandEncoder> cmd_encoder;
    id<MTLBuffer> uniform_buffers[SG_NUM_INFLIGHT_FRAMES];
} _sg_mtl_backend_t;

#elif defined(SOKOL_WGPU)

#define _SG_WGPU_ROWPITCH_ALIGN (256)
#define _SG_WGPU_MAX_UNIFORM_UPDATE_SIZE (1<<16) // also see WGPULimits.maxUniformBufferBindingSize
#define _SG_WGPU_NUM_BINDGROUPS (2) // 0: uniforms, 1: images and sampler on both shader stages
#define _SG_WGPU_UNIFORM_BINDGROUP_INDEX (0)
#define _SG_WGPU_IMAGE_SAMPLER_BINDGROUP_INDEX (1)

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
        WGPUTextureView view;
    } wgpu;
} _sg_wgpu_image_t;
typedef _sg_wgpu_image_t _sg_image_t;

typedef struct {
    _sg_slot_t slot;
    _sg_sampler_common_t cmn;
    struct {
        WGPUSampler smp;
    } wgpu;
} _sg_wgpu_sampler_t;
typedef _sg_wgpu_sampler_t _sg_sampler_t;

typedef struct {
    WGPUShaderModule module;
    _sg_str_t entry;
} _sg_wgpu_shader_stage_t;

typedef struct {
    _sg_slot_t slot;
    _sg_shader_common_t cmn;
    struct {
        _sg_wgpu_shader_stage_t stage[SG_NUM_SHADER_STAGES];
        WGPUBindGroupLayout bind_group_layout;
    } wgpu;
} _sg_wgpu_shader_t;
typedef _sg_wgpu_shader_t _sg_shader_t;

typedef struct {
    _sg_slot_t slot;
    _sg_pipeline_common_t cmn;
    _sg_shader_t* shader;
    struct {
        WGPURenderPipeline pip;
        WGPUColor blend_color;
    } wgpu;
} _sg_wgpu_pipeline_t;
typedef _sg_wgpu_pipeline_t _sg_pipeline_t;

typedef struct {
    _sg_image_t* image;
    WGPUTextureView view;
} _sg_wgpu_attachment_t;

typedef struct {
    _sg_slot_t slot;
    _sg_pass_common_t cmn;
    struct {
        _sg_wgpu_attachment_t color_atts[SG_MAX_COLOR_ATTACHMENTS];
        _sg_wgpu_attachment_t resolve_atts[SG_MAX_COLOR_ATTACHMENTS];
        _sg_wgpu_attachment_t ds_att;
    } wgpu;
} _sg_wgpu_pass_t;
typedef _sg_wgpu_pass_t _sg_pass_t;
typedef _sg_pass_attachment_common_t _sg_pass_attachment_t;

typedef struct {
    _sg_slot_t slot;
} _sg_wgpu_context_t;
typedef _sg_wgpu_context_t _sg_context_t;

// a pool of per-frame uniform buffers
typedef struct {
    uint32_t num_bytes;
    uint32_t offset;    // current offset into buf
    uint8_t* staging;   // intermediate buffer for uniform data updates
    WGPUBuffer buf;     // the GPU-side uniform buffer
    struct {
        WGPUBindGroupLayout group_layout;
        WGPUBindGroup group;
        uint32_t offsets[SG_NUM_SHADER_STAGES][SG_MAX_SHADERSTAGE_UBS];
    } bind;
} _sg_wgpu_uniform_buffer_t;

typedef struct {
    uint32_t id;
} _sg_wgpu_bindgroup_handle_t;

#define _SG_WGPU_BINDGROUPSCACHE_NUM_ITEMS (1 + SG_NUM_SHADER_STAGES * (SG_MAX_SHADERSTAGE_IMAGES + SG_MAX_SHADERSTAGE_SAMPLERS))
typedef struct {
    uint64_t hash;
    uint32_t items[_SG_WGPU_BINDGROUPSCACHE_NUM_ITEMS];
} _sg_wgpu_bindgroups_cache_key_t;

typedef struct {
    uint32_t num;           // must be 2^n
    uint32_t index_mask;    // mask to turn hash into valid index
    _sg_wgpu_bindgroup_handle_t* items;
} _sg_wgpu_bindgroups_cache_t;

typedef struct {
    _sg_slot_t slot;
    WGPUBindGroup bindgroup;
    _sg_wgpu_bindgroups_cache_key_t key;
} _sg_wgpu_bindgroup_t;

typedef struct {
    _sg_pool_t pool;
    _sg_wgpu_bindgroup_t* bindgroups;
} _sg_wgpu_bindgroups_pool_t;

typedef struct {
    struct {
        sg_buffer buffer;
        int offset;
    } vbs[SG_MAX_VERTEX_BUFFERS];
    struct {
        sg_buffer buffer;
        int offset;
    } ib;
    _sg_wgpu_bindgroup_handle_t bg;
} _sg_wgpu_bindings_cache_t;

// the WGPU backend state
typedef struct {
    bool valid;
    WGPUDevice dev;
    WGPUTextureView (*render_view_cb)(void);
    WGPUTextureView (*render_view_userdata_cb)(void*);
    WGPUTextureView (*resolve_view_cb)(void);
    WGPUTextureView (*resolve_view_userdata_cb)(void*);
    WGPUTextureView (*depth_stencil_view_cb)(void);
    WGPUTextureView (*depth_stencil_view_userdata_cb)(void*);
    void* user_data;
    bool in_pass;
    bool use_indexed_draw;
    int cur_width;
    int cur_height;
    WGPUSupportedLimits limits;
    WGPUQueue queue;
    WGPUCommandEncoder cmd_enc;
    WGPURenderPassEncoder pass_enc;
    WGPUBindGroup empty_bind_group;
    const _sg_pipeline_t* cur_pipeline;
    sg_pipeline cur_pipeline_id;
    _sg_wgpu_uniform_buffer_t uniform;
    _sg_wgpu_bindings_cache_t bindings_cache;
    _sg_wgpu_bindgroups_cache_t bindgroups_cache;
    _sg_wgpu_bindgroups_pool_t bindgroups_pool;
} _sg_wgpu_backend_t;
#endif

// POOL STRUCTS

// this *MUST* remain 0
#define _SG_INVALID_SLOT_INDEX (0)

typedef struct {
    _sg_pool_t buffer_pool;
    _sg_pool_t image_pool;
    _sg_pool_t sampler_pool;
    _sg_pool_t shader_pool;
    _sg_pool_t pipeline_pool;
    _sg_pool_t pass_pool;
    _sg_pool_t context_pool;
    _sg_buffer_t* buffers;
    _sg_image_t* images;
    _sg_sampler_t* samplers;
    _sg_shader_t* shaders;
    _sg_pipeline_t* pipelines;
    _sg_pass_t* passes;
    _sg_context_t* contexts;
} _sg_pools_t;

typedef struct {
    int num;        // number of allocated commit listener items
    int upper;      // the current upper index (no valid items past this point)
    sg_commit_listener* items;
} _sg_commit_listeners_t;

// resolved resource bindings struct
typedef struct {
    _sg_pipeline_t* pip;
    int num_vbs;
    int num_vs_imgs;
    int num_vs_smps;
    int num_fs_imgs;
    int num_fs_smps;
    int vb_offsets[SG_MAX_VERTEX_BUFFERS];
    int ib_offset;
    _sg_buffer_t* vbs[SG_MAX_VERTEX_BUFFERS];
    _sg_buffer_t* ib;
    _sg_image_t* vs_imgs[SG_MAX_SHADERSTAGE_IMAGES];
    _sg_sampler_t* vs_smps[SG_MAX_SHADERSTAGE_SAMPLERS];
    _sg_image_t* fs_imgs[SG_MAX_SHADERSTAGE_IMAGES];
    _sg_sampler_t* fs_smps[SG_MAX_SHADERSTAGE_SAMPLERS];
} _sg_bindings_t;

typedef struct {
    bool valid;
    sg_desc desc;       // original desc with default values patched in
    uint32_t frame_index;
    sg_context active_context;
    sg_pass cur_pass;
    sg_pipeline cur_pipeline;
    bool pass_valid;
    bool bindings_applied;
    bool next_draw_valid;
    #if defined(SOKOL_DEBUG)
    sg_log_item validate_error;
    #endif
    _sg_pools_t pools;
    sg_backend backend;
    sg_features features;
    sg_limits limits;
    sg_pixelformat_info formats[_SG_PIXELFORMAT_NUM];
    bool stats_enabled;
    sg_frame_stats stats;
    sg_frame_stats prev_stats;
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

// ██       ██████   ██████   ██████  ██ ███    ██  ██████
// ██      ██    ██ ██       ██       ██ ████   ██ ██
// ██      ██    ██ ██   ███ ██   ███ ██ ██ ██  ██ ██   ███
// ██      ██    ██ ██    ██ ██    ██ ██ ██  ██ ██ ██    ██
// ███████  ██████   ██████   ██████  ██ ██   ████  ██████
//
// >>logging
#if defined(SOKOL_DEBUG)
#define _SG_LOGITEM_XMACRO(item,msg) #item ": " msg,
static const char* _sg_log_messages[] = {
    _SG_LOG_ITEMS
};
#undef _SG_LOGITEM_XMACRO
#endif // SOKOL_DEBUG

#define _SG_PANIC(code) _sg_log(SG_LOGITEM_ ##code, 0, 0, __LINE__)
#define _SG_ERROR(code) _sg_log(SG_LOGITEM_ ##code, 1, 0, __LINE__)
#define _SG_WARN(code) _sg_log(SG_LOGITEM_ ##code, 2, 0, __LINE__)
#define _SG_INFO(code) _sg_log(SG_LOGITEM_ ##code, 3, 0, __LINE__)
#define _SG_LOGMSG(code,msg) _sg_log(SG_LOGITEM_ ##code, 3, msg, __LINE__)
#define _SG_VALIDATE(cond,code) if (!(cond)){ _sg.validate_error = SG_LOGITEM_ ##code; _sg_log(SG_LOGITEM_ ##code, 1, 0, __LINE__); }

static void _sg_log(sg_log_item log_item, uint32_t log_level, const char* msg, uint32_t line_nr) {
    if (_sg.desc.logger.func) {
        const char* filename = 0;
        #if defined(SOKOL_DEBUG)
            filename = __FILE__;
            if (0 == msg) {
                msg = _sg_log_messages[log_item];
            }
        #endif
        _sg.desc.logger.func("sg", log_level, log_item, msg, line_nr, filename, _sg.desc.logger.user_data);
    } else {
        // for log level PANIC it would be 'undefined behaviour' to continue
        if (log_level == 0) {
            abort();
        }
    }
}

// ███    ███ ███████ ███    ███  ██████  ██████  ██    ██
// ████  ████ ██      ████  ████ ██    ██ ██   ██  ██  ██
// ██ ████ ██ █████   ██ ████ ██ ██    ██ ██████    ████
// ██  ██  ██ ██      ██  ██  ██ ██    ██ ██   ██    ██
// ██      ██ ███████ ██      ██  ██████  ██   ██    ██
//
// >>memory

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
    if (_sg.desc.allocator.alloc_fn) {
        ptr = _sg.desc.allocator.alloc_fn(size, _sg.desc.allocator.user_data);
    } else {
        ptr = malloc(size);
    }
    if (0 == ptr) {
        _SG_PANIC(MALLOC_FAILED);
    }
    return ptr;
}

_SOKOL_PRIVATE void* _sg_malloc_clear(size_t size) {
    void* ptr = _sg_malloc(size);
    _sg_clear(ptr, size);
    return ptr;
}

_SOKOL_PRIVATE void _sg_free(void* ptr) {
    if (_sg.desc.allocator.free_fn) {
        _sg.desc.allocator.free_fn(ptr, _sg.desc.allocator.user_data);
    } else {
        free(ptr);
    }
}

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
    } else {
        _sg_clear(dst->buf, _SG_STRING_SIZE);
    }
}

// ██   ██ ███████ ██      ██████  ███████ ██████  ███████
// ██   ██ ██      ██      ██   ██ ██      ██   ██ ██
// ███████ █████   ██      ██████  █████   ██████  ███████
// ██   ██ ██      ██      ██      ██      ██   ██      ██
// ██   ██ ███████ ███████ ██      ███████ ██   ██ ███████
//
// >>helpers
_SOKOL_PRIVATE uint32_t _sg_align_u32(uint32_t val, uint32_t align) {
    SOKOL_ASSERT((align > 0) && ((align & (align - 1)) == 0));
    return (val + (align - 1)) & ~(align - 1);
}

typedef struct { int x, y, w, h; } _sg_recti_t;

_SOKOL_PRIVATE _sg_recti_t _sg_clipi(int x, int y, int w, int h, int clip_width, int clip_height) {
    x = _sg_min(_sg_max(0, x), clip_width-1);
    y = _sg_min(_sg_max(0, y), clip_height-1);
    if ((x + w) > clip_width) {
        w = clip_width - x;
    }
    if ((y + h) > clip_height) {
        h = clip_height - y;
    }
    w = _sg_max(w, 1);
    h = _sg_max(h, 1);
    const _sg_recti_t res = { x, y, w, h };
    return res;
}

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
        case SG_VERTEXFORMAT_HALF2:     return 4;
        case SG_VERTEXFORMAT_HALF4:     return 8;
        case SG_VERTEXFORMAT_INVALID:   return 0;
        default:
            SOKOL_UNREACHABLE;
            return -1;
    }
}

_SOKOL_PRIVATE uint32_t _sg_uniform_alignment(sg_uniform_type type, int array_count, sg_uniform_layout ub_layout) {
    if (ub_layout == SG_UNIFORMLAYOUT_NATIVE) {
        return 1;
    } else {
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
        } else {
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
    } else {
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
        } else {
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

_SOKOL_PRIVATE bool _sg_is_valid_rendertarget_color_format(sg_pixel_format fmt) {
    const int fmt_index = (int) fmt;
    SOKOL_ASSERT((fmt_index >= 0) && (fmt_index < _SG_PIXELFORMAT_NUM));
    return _sg.formats[fmt_index].render && !_sg.formats[fmt_index].depth;
}

_SOKOL_PRIVATE bool _sg_is_valid_rendertarget_depth_format(sg_pixel_format fmt) {
    const int fmt_index = (int) fmt;
    SOKOL_ASSERT((fmt_index >= 0) && (fmt_index < _SG_PIXELFORMAT_NUM));
    return _sg.formats[fmt_index].render && _sg.formats[fmt_index].depth;
}

_SOKOL_PRIVATE bool _sg_is_depth_or_depth_stencil_format(sg_pixel_format fmt) {
    return (SG_PIXELFORMAT_DEPTH == fmt) || (SG_PIXELFORMAT_DEPTH_STENCIL == fmt);
}

_SOKOL_PRIVATE bool _sg_is_depth_stencil_format(sg_pixel_format fmt) {
    return (SG_PIXELFORMAT_DEPTH_STENCIL == fmt);
}

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
        case SG_PIXELFORMAT_SRGB8A8:
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

_SOKOL_PRIVATE uint32_t _sg_roundup_u32(uint32_t val, uint32_t round_to) {
    return (val+(round_to-1)) & ~(round_to-1);
}

_SOKOL_PRIVATE uint64_t _sg_roundup_u64(uint64_t val, uint64_t round_to) {
    return (val+(round_to-1)) & ~(round_to-1);
}

_SOKOL_PRIVATE bool _sg_multiple_u64(uint64_t val, uint64_t of) {
    return (val & (of-1)) == 0;
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

// compute the number of rows in a surface depending on pixel format
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

// return size of a mipmap level
_SOKOL_PRIVATE int _sg_miplevel_dim(int base_dim, int mip_level) {
    return _sg_max(base_dim >> mip_level, 1);
}

/* return pitch of a 2D subimage / texture slice
    see ComputePitch in https://github.com/microsoft/DirectXTex/blob/master/DirectXTex/DirectXTexUtil.cpp
*/
_SOKOL_PRIVATE int _sg_surface_pitch(sg_pixel_format fmt, int width, int height, int row_align) {
    int num_rows = _sg_num_rows(fmt, height);
    return num_rows * _sg_row_pitch(fmt, width, row_align);
}

// capability table pixel format helper functions
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

_SOKOL_PRIVATE void _sg_resolve_default_pass_action(const sg_pass_action* from, sg_pass_action* to) {
    SOKOL_ASSERT(from && to);
    *to = *from;
    for (int i = 0; i < SG_MAX_COLOR_ATTACHMENTS; i++) {
        if (to->colors[i].load_action == _SG_LOADACTION_DEFAULT) {
            to->colors[i].load_action = SG_LOADACTION_CLEAR;
            to->colors[i].clear_value.r = SG_DEFAULT_CLEAR_RED;
            to->colors[i].clear_value.g = SG_DEFAULT_CLEAR_GREEN;
            to->colors[i].clear_value.b = SG_DEFAULT_CLEAR_BLUE;
            to->colors[i].clear_value.a = SG_DEFAULT_CLEAR_ALPHA;
        }
        if (to->colors[i].store_action == _SG_STOREACTION_DEFAULT) {
            to->colors[i].store_action = SG_STOREACTION_STORE;
        }
    }
    if (to->depth.load_action == _SG_LOADACTION_DEFAULT) {
        to->depth.load_action = SG_LOADACTION_CLEAR;
        to->depth.clear_value = SG_DEFAULT_CLEAR_DEPTH;
    }
    if (to->depth.store_action == _SG_STOREACTION_DEFAULT) {
        to->depth.store_action = SG_STOREACTION_DONTCARE;
    }
    if (to->stencil.load_action == _SG_LOADACTION_DEFAULT) {
        to->stencil.load_action = SG_LOADACTION_CLEAR;
        to->stencil.clear_value = SG_DEFAULT_CLEAR_STENCIL;
    }
    if (to->stencil.store_action == _SG_STOREACTION_DEFAULT) {
        to->stencil.store_action = SG_STOREACTION_DONTCARE;
    }
}

// ██████  ██    ██ ███    ███ ███    ███ ██    ██     ██████   █████   ██████ ██   ██ ███████ ███    ██ ██████
// ██   ██ ██    ██ ████  ████ ████  ████  ██  ██      ██   ██ ██   ██ ██      ██  ██  ██      ████   ██ ██   ██
// ██   ██ ██    ██ ██ ████ ██ ██ ████ ██   ████       ██████  ███████ ██      █████   █████   ██ ██  ██ ██   ██
// ██   ██ ██    ██ ██  ██  ██ ██  ██  ██    ██        ██   ██ ██   ██ ██      ██  ██  ██      ██  ██ ██ ██   ██
// ██████   ██████  ██      ██ ██      ██    ██        ██████  ██   ██  ██████ ██   ██ ███████ ██   ████ ██████
//
// >>dummy backend
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
    // empty
}

_SOKOL_PRIVATE void _sg_dummy_reset_state_cache(void) {
    // empty
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
    _SOKOL_UNUSED(buf);
    _SOKOL_UNUSED(desc);
    return SG_RESOURCESTATE_VALID;
}

_SOKOL_PRIVATE void _sg_dummy_discard_buffer(_sg_buffer_t* buf) {
    SOKOL_ASSERT(buf);
    _SOKOL_UNUSED(buf);
}

_SOKOL_PRIVATE sg_resource_state _sg_dummy_create_image(_sg_image_t* img, const sg_image_desc* desc) {
    SOKOL_ASSERT(img && desc);
    _SOKOL_UNUSED(img);
    _SOKOL_UNUSED(desc);
    return SG_RESOURCESTATE_VALID;
}

_SOKOL_PRIVATE void _sg_dummy_discard_image(_sg_image_t* img) {
    SOKOL_ASSERT(img);
    _SOKOL_UNUSED(img);
}

_SOKOL_PRIVATE sg_resource_state _sg_dummy_create_sampler(_sg_sampler_t* smp, const sg_sampler_desc* desc) {
    SOKOL_ASSERT(smp && desc);
    _SOKOL_UNUSED(smp);
    _SOKOL_UNUSED(desc);
    return SG_RESOURCESTATE_VALID;
}

_SOKOL_PRIVATE void _sg_dummy_discard_sampler(_sg_sampler_t* smp) {
    SOKOL_ASSERT(smp);
    _SOKOL_UNUSED(smp);
}

_SOKOL_PRIVATE sg_resource_state _sg_dummy_create_shader(_sg_shader_t* shd, const sg_shader_desc* desc) {
    SOKOL_ASSERT(shd && desc);
    _SOKOL_UNUSED(shd);
    _SOKOL_UNUSED(desc);
    return SG_RESOURCESTATE_VALID;
}

_SOKOL_PRIVATE void _sg_dummy_discard_shader(_sg_shader_t* shd) {
    SOKOL_ASSERT(shd);
    _SOKOL_UNUSED(shd);
}

_SOKOL_PRIVATE sg_resource_state _sg_dummy_create_pipeline(_sg_pipeline_t* pip, _sg_shader_t* shd, const sg_pipeline_desc* desc) {
    SOKOL_ASSERT(pip && desc);
    pip->shader = shd;
    for (int attr_index = 0; attr_index < SG_MAX_VERTEX_ATTRIBUTES; attr_index++) {
        const sg_vertex_attr_state* a_state = &desc->layout.attrs[attr_index];
        if (a_state->format == SG_VERTEXFORMAT_INVALID) {
            break;
        }
        SOKOL_ASSERT(a_state->buffer_index < SG_MAX_VERTEX_BUFFERS);
        pip->cmn.vertex_buffer_layout_active[a_state->buffer_index] = true;
    }
    return SG_RESOURCESTATE_VALID;
}

_SOKOL_PRIVATE void _sg_dummy_discard_pipeline(_sg_pipeline_t* pip) {
    SOKOL_ASSERT(pip);
    _SOKOL_UNUSED(pip);
}

_SOKOL_PRIVATE sg_resource_state _sg_dummy_create_pass(_sg_pass_t* pass, _sg_image_t** color_images, _sg_image_t** resolve_images, _sg_image_t* ds_img, const sg_pass_desc* desc) {
    SOKOL_ASSERT(pass && desc);
    SOKOL_ASSERT(color_images && resolve_images);

    for (int i = 0; i < pass->cmn.num_color_atts; i++) {
        const sg_pass_attachment_desc* color_desc = &desc->color_attachments[i];
        _SOKOL_UNUSED(color_desc);
        SOKOL_ASSERT(color_desc->image.id != SG_INVALID_ID);
        SOKOL_ASSERT(0 == pass->dmy.color_atts[i].image);
        SOKOL_ASSERT(color_images[i] && (color_images[i]->slot.id == color_desc->image.id));
        SOKOL_ASSERT(_sg_is_valid_rendertarget_color_format(color_images[i]->cmn.pixel_format));
        pass->dmy.color_atts[i].image = color_images[i];

        const sg_pass_attachment_desc* resolve_desc = &desc->resolve_attachments[i];
        if (resolve_desc->image.id != SG_INVALID_ID) {
            SOKOL_ASSERT(0 == pass->dmy.resolve_atts[i].image);
            SOKOL_ASSERT(resolve_images[i] && (resolve_images[i]->slot.id == resolve_desc->image.id));
            SOKOL_ASSERT(color_images[i] && (color_images[i]->cmn.pixel_format == resolve_images[i]->cmn.pixel_format));
            pass->dmy.resolve_atts[i].image = resolve_images[i];
        }
    }

    SOKOL_ASSERT(0 == pass->dmy.ds_att.image);
    const sg_pass_attachment_desc* ds_desc = &desc->depth_stencil_attachment;
    if (ds_desc->image.id != SG_INVALID_ID) {
        SOKOL_ASSERT(ds_img && (ds_img->slot.id == ds_desc->image.id));
        SOKOL_ASSERT(_sg_is_valid_rendertarget_depth_format(ds_img->cmn.pixel_format));
        pass->dmy.ds_att.image = ds_img;
    }
    return SG_RESOURCESTATE_VALID;
}

_SOKOL_PRIVATE void _sg_dummy_discard_pass(_sg_pass_t* pass) {
    SOKOL_ASSERT(pass);
    _SOKOL_UNUSED(pass);
}

_SOKOL_PRIVATE _sg_image_t* _sg_dummy_pass_color_image(const _sg_pass_t* pass, int index) {
    SOKOL_ASSERT(pass && (index >= 0) && (index < SG_MAX_COLOR_ATTACHMENTS));
    return pass->dmy.color_atts[index].image;
}

_SOKOL_PRIVATE _sg_image_t* _sg_dummy_pass_resolve_image(const _sg_pass_t* pass, int index) {
    SOKOL_ASSERT(pass && (index >= 0) && (index < SG_MAX_COLOR_ATTACHMENTS));
    return pass->dmy.resolve_atts[index].image;
}

_SOKOL_PRIVATE _sg_image_t* _sg_dummy_pass_ds_image(const _sg_pass_t* pass) {
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
    // empty
}

_SOKOL_PRIVATE void _sg_dummy_commit(void) {
    // empty
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

_SOKOL_PRIVATE bool _sg_dummy_apply_bindings(_sg_bindings_t* bnd) {
    SOKOL_ASSERT(bnd);
    SOKOL_ASSERT(bnd->pip);
    _SOKOL_UNUSED(bnd);
    return true;
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

_SOKOL_PRIVATE bool _sg_dummy_append_buffer(_sg_buffer_t* buf, const sg_range* data, bool new_frame) {
    SOKOL_ASSERT(buf && data && data->ptr && (data->size > 0));
    _SOKOL_UNUSED(data);
    if (new_frame) {
        if (++buf->cmn.active_slot >= buf->cmn.num_slots) {
            buf->cmn.active_slot = 0;
        }
    }
    return true;
}

_SOKOL_PRIVATE void _sg_dummy_update_image(_sg_image_t* img, const sg_image_data* data) {
    SOKOL_ASSERT(img && data);
    _SOKOL_UNUSED(data);
    if (++img->cmn.active_slot >= img->cmn.num_slots) {
        img->cmn.active_slot = 0;
    }
}

//  ██████  ██████  ███████ ███    ██  ██████  ██          ██████   █████   ██████ ██   ██ ███████ ███    ██ ██████
// ██    ██ ██   ██ ██      ████   ██ ██       ██          ██   ██ ██   ██ ██      ██  ██  ██      ████   ██ ██   ██
// ██    ██ ██████  █████   ██ ██  ██ ██   ███ ██          ██████  ███████ ██      █████   █████   ██ ██  ██ ██   ██
// ██    ██ ██      ██      ██  ██ ██ ██    ██ ██          ██   ██ ██   ██ ██      ██  ██  ██      ██  ██ ██ ██   ██
//  ██████  ██      ███████ ██   ████  ██████  ███████     ██████  ██   ██  ██████ ██   ██ ███████ ██   ████ ██████
//
// >>opengl backend
#elif defined(_SOKOL_ANY_GL)

// optional GL loader for win32
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
    _SG_XMACRO(glBlendColor,                      void, (GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha)) \
    _SG_XMACRO(glTexParameterf,                   void, (GLenum target, GLenum pname, GLfloat param)) \
    _SG_XMACRO(glTexParameterfv,                  void, (GLenum target, GLenum pname, const GLfloat* params)) \
    _SG_XMACRO(glGetShaderInfoLog,                void, (GLuint shader, GLsizei bufSize, GLsizei * length, GLchar * infoLog)) \
    _SG_XMACRO(glDepthFunc,                       void, (GLenum func)) \
    _SG_XMACRO(glStencilOp ,                      void, (GLenum fail, GLenum zfail, GLenum zpass)) \
    _SG_XMACRO(glStencilFunc,                     void, (GLenum func, GLint ref, GLuint mask)) \
    _SG_XMACRO(glEnableVertexAttribArray,         void, (GLuint index)) \
    _SG_XMACRO(glBlendFunc,                       void, (GLenum sfactor, GLenum dfactor)) \
    _SG_XMACRO(glReadBuffer,                      void, (GLenum src)) \
    _SG_XMACRO(glTexImage2D,                      void, (GLenum target, GLint level, GLint internalformat, GLsizei width, GLsizei height, GLint border, GLenum format, GLenum type, const void * pixels)) \
    _SG_XMACRO(glGenVertexArrays,                 void, (GLsizei n, GLuint * arrays)) \
    _SG_XMACRO(glFrontFace,                       void, (GLenum mode)) \
    _SG_XMACRO(glCullFace,                        void, (GLenum mode)) \
    _SG_XMACRO(glPixelStorei,                     void, (GLenum pname, GLint param)) \
    _SG_XMACRO(glBindSampler,                     void, (GLuint unit, GLuint sampler)) \
    _SG_XMACRO(glGenSamplers,                     void, (GLsizei n, GLuint* samplers)) \
    _SG_XMACRO(glSamplerParameteri,               void, (GLuint sampler, GLenum pname, GLint param)) \
    _SG_XMACRO(glSamplerParameterf,               void, (GLuint sampler, GLenum pname, GLfloat param)) \
    _SG_XMACRO(glSamplerParameterfv,              void, (GLuint sampler, GLenum pname, const GLfloat* params)) \
    _SG_XMACRO(glDeleteSamplers,                  void, (GLsizei n, const GLuint* samplers))

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

//-- type translation ----------------------------------------------------------
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
        case SG_IMAGETYPE_3D:       return GL_TEXTURE_3D;
        case SG_IMAGETYPE_ARRAY:    return GL_TEXTURE_2D_ARRAY;
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
        case SG_VERTEXFORMAT_HALF2:     return 2;
        case SG_VERTEXFORMAT_HALF4:     return 4;
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
        case SG_VERTEXFORMAT_HALF2:
        case SG_VERTEXFORMAT_HALF4:
            return GL_HALF_FLOAT;
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

_SOKOL_PRIVATE GLenum _sg_gl_min_filter(sg_filter min_f, sg_filter mipmap_f) {
    if (min_f == SG_FILTER_NEAREST) {
        switch (mipmap_f) {
            case SG_FILTER_NONE:    return GL_NEAREST;
            case SG_FILTER_NEAREST: return GL_NEAREST_MIPMAP_NEAREST;
            case SG_FILTER_LINEAR:  return GL_NEAREST_MIPMAP_LINEAR;
            default: SOKOL_UNREACHABLE; return (GLenum)0;
        }
    } else if (min_f == SG_FILTER_LINEAR) {
        switch (mipmap_f) {
            case SG_FILTER_NONE:    return GL_LINEAR;
            case SG_FILTER_NEAREST: return GL_LINEAR_MIPMAP_NEAREST;
            case SG_FILTER_LINEAR:  return GL_LINEAR_MIPMAP_LINEAR;
            default: SOKOL_UNREACHABLE; return (GLenum)0;
        }
    } else {
        SOKOL_UNREACHABLE; return (GLenum)0;
    }
}

_SOKOL_PRIVATE GLenum _sg_gl_mag_filter(sg_filter mag_f) {
    if (mag_f == SG_FILTER_NEAREST) {
        return GL_NEAREST;
    } else {
        return GL_LINEAR;
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
        case SG_PIXELFORMAT_SRGB8A8:
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
        case SG_PIXELFORMAT_RGB10A2:
            return GL_UNSIGNED_INT_2_10_10_10_REV;
        case SG_PIXELFORMAT_RG11B10F:
            return GL_UNSIGNED_INT_10F_11F_11F_REV;
        case SG_PIXELFORMAT_RGB9E5:
            return GL_UNSIGNED_INT_5_9_9_9_REV;
        case SG_PIXELFORMAT_DEPTH:
            return GL_FLOAT;
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
            return GL_RED;
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
        case SG_PIXELFORMAT_RGBA8:
        case SG_PIXELFORMAT_SRGB8A8:
        case SG_PIXELFORMAT_RGBA8SN:
        case SG_PIXELFORMAT_RGBA16:
        case SG_PIXELFORMAT_RGBA16SN:
        case SG_PIXELFORMAT_RGBA16F:
        case SG_PIXELFORMAT_RGBA32F:
        case SG_PIXELFORMAT_RGB10A2:
            return GL_RGBA;
        case SG_PIXELFORMAT_RGBA8UI:
        case SG_PIXELFORMAT_RGBA8SI:
        case SG_PIXELFORMAT_RGBA16UI:
        case SG_PIXELFORMAT_RGBA16SI:
        case SG_PIXELFORMAT_RGBA32UI:
        case SG_PIXELFORMAT_RGBA32SI:
            return GL_RGBA_INTEGER;
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
        case SG_PIXELFORMAT_SRGB8A8:    return GL_SRGB8_ALPHA8;
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
        case SG_PIXELFORMAT_DEPTH:      return GL_DEPTH_COMPONENT32F;
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

// see: https://www.khronos.org/registry/OpenGL-Refpages/es3.0/html/glTexImage2D.xhtml
_SOKOL_PRIVATE void _sg_gl_init_pixelformats(bool has_bgra) {
    _sg_pixelformat_all(&_sg.formats[SG_PIXELFORMAT_R8]);
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
    _sg_pixelformat_all(&_sg.formats[SG_PIXELFORMAT_RGBA8]);
    _sg_pixelformat_all(&_sg.formats[SG_PIXELFORMAT_SRGB8A8]);
    _sg_pixelformat_sf(&_sg.formats[SG_PIXELFORMAT_RGBA8SN]);
    _sg_pixelformat_srm(&_sg.formats[SG_PIXELFORMAT_RGBA8UI]);
    _sg_pixelformat_srm(&_sg.formats[SG_PIXELFORMAT_RGBA8SI]);
    if (has_bgra) {
        _sg_pixelformat_all(&_sg.formats[SG_PIXELFORMAT_BGRA8]);
    }
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
    _sg_pixelformat_srmd(&_sg.formats[SG_PIXELFORMAT_DEPTH]);
    _sg_pixelformat_srmd(&_sg.formats[SG_PIXELFORMAT_DEPTH_STENCIL]);
}

// FIXME: OES_half_float_blend
_SOKOL_PRIVATE void _sg_gl_init_pixelformats_half_float(bool has_colorbuffer_half_float) {
    if (has_colorbuffer_half_float) {
        _sg_pixelformat_all(&_sg.formats[SG_PIXELFORMAT_R16F]);
        _sg_pixelformat_all(&_sg.formats[SG_PIXELFORMAT_RG16F]);
        _sg_pixelformat_all(&_sg.formats[SG_PIXELFORMAT_RGBA16F]);
    } else {
        _sg_pixelformat_sf(&_sg.formats[SG_PIXELFORMAT_R16F]);
        _sg_pixelformat_sf(&_sg.formats[SG_PIXELFORMAT_RG16F]);
        _sg_pixelformat_sf(&_sg.formats[SG_PIXELFORMAT_RGBA16F]);
    }
}

_SOKOL_PRIVATE void _sg_gl_init_pixelformats_float(bool has_colorbuffer_float, bool has_texture_float_linear, bool has_float_blend) {
    if (has_texture_float_linear) {
        if (has_colorbuffer_float) {
            if (has_float_blend) {
                _sg_pixelformat_all(&_sg.formats[SG_PIXELFORMAT_R32F]);
                _sg_pixelformat_all(&_sg.formats[SG_PIXELFORMAT_RG32F]);
                _sg_pixelformat_all(&_sg.formats[SG_PIXELFORMAT_RGBA32F]);
            } else {
                _sg_pixelformat_sfrm(&_sg.formats[SG_PIXELFORMAT_R32F]);
                _sg_pixelformat_sfrm(&_sg.formats[SG_PIXELFORMAT_RG32F]);
                _sg_pixelformat_sfrm(&_sg.formats[SG_PIXELFORMAT_RGBA32F]);
            }
        } else {
            _sg_pixelformat_sf(&_sg.formats[SG_PIXELFORMAT_R32F]);
            _sg_pixelformat_sf(&_sg.formats[SG_PIXELFORMAT_RG32F]);
            _sg_pixelformat_sf(&_sg.formats[SG_PIXELFORMAT_RGBA32F]);
        }
    } else {
        if (has_colorbuffer_float) {
            _sg_pixelformat_sbrm(&_sg.formats[SG_PIXELFORMAT_R32F]);
            _sg_pixelformat_sbrm(&_sg.formats[SG_PIXELFORMAT_RG32F]);
            _sg_pixelformat_sbrm(&_sg.formats[SG_PIXELFORMAT_RGBA32F]);
        } else {
            _sg_pixelformat_s(&_sg.formats[SG_PIXELFORMAT_R32F]);
            _sg_pixelformat_s(&_sg.formats[SG_PIXELFORMAT_RG32F]);
            _sg_pixelformat_s(&_sg.formats[SG_PIXELFORMAT_RGBA32F]);
        }
    }
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
    glGetIntegerv(GL_MAX_3D_TEXTURE_SIZE, &gl_int);
    _SG_GL_CHECK_ERROR();
    _sg.limits.max_image_size_3d = gl_int;
    glGetIntegerv(GL_MAX_ARRAY_TEXTURE_LAYERS, &gl_int);
    _SG_GL_CHECK_ERROR();
    _sg.limits.max_image_array_layers = gl_int;
    if (_sg.gl.ext_anisotropic) {
        glGetIntegerv(GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, &gl_int);
        _SG_GL_CHECK_ERROR();
        _sg.gl.max_anisotropy = gl_int;
    } else {
        _sg.gl.max_anisotropy = 1;
    }
    glGetIntegerv(GL_MAX_COMBINED_TEXTURE_IMAGE_UNITS, &gl_int);
    _SG_GL_CHECK_ERROR();
    _sg.limits.gl_max_combined_texture_image_units = gl_int;
}

#if defined(SOKOL_GLCORE33)
_SOKOL_PRIVATE void _sg_gl_init_caps_glcore33(void) {
    _sg.backend = SG_BACKEND_GLCORE33;

    _sg.features.origin_top_left = false;
    _sg.features.image_clamp_to_border = true;
    _sg.features.mrt_independent_blend_state = false;
    _sg.features.mrt_independent_write_mask = true;

    // scan extensions
    bool has_s3tc = false;  // BC1..BC3
    bool has_rgtc = false;  // BC4 and BC5
    bool has_bptc = false;  // BC6H and BC7
    bool has_pvrtc = false;
    bool has_etc2 = false;
    GLint num_ext = 0;
    glGetIntegerv(GL_NUM_EXTENSIONS, &num_ext);
    for (int i = 0; i < num_ext; i++) {
        const char* ext = (const char*) glGetStringi(GL_EXTENSIONS, (GLuint)i);
        if (ext) {
            if (strstr(ext, "_texture_compression_s3tc")) {
                has_s3tc = true;
            } else if (strstr(ext, "_texture_compression_rgtc")) {
                has_rgtc = true;
            } else if (strstr(ext, "_texture_compression_bptc")) {
                has_bptc = true;
            } else if (strstr(ext, "_texture_compression_pvrtc")) {
                has_pvrtc = true;
            } else if (strstr(ext, "_ES3_compatibility")) {
                has_etc2 = true;
            } else if (strstr(ext, "_texture_filter_anisotropic")) {
                _sg.gl.ext_anisotropic = true;
            }
        }
    }

    // limits
    _sg_gl_init_limits();

    // pixel formats
    const bool has_bgra = false;    // not a bug
    const bool has_colorbuffer_float = true;
    const bool has_colorbuffer_half_float = true;
    const bool has_texture_float_linear = true; // FIXME???
    const bool has_float_blend = true;
    _sg_gl_init_pixelformats(has_bgra);
    _sg_gl_init_pixelformats_float(has_colorbuffer_float, has_texture_float_linear, has_float_blend);
    _sg_gl_init_pixelformats_half_float(has_colorbuffer_half_float);
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
    _sg.features.image_clamp_to_border = false;
    _sg.features.mrt_independent_blend_state = false;
    _sg.features.mrt_independent_write_mask = false;

    bool has_s3tc = false;  // BC1..BC3
    bool has_rgtc = false;  // BC4 and BC5
    bool has_bptc = false;  // BC6H and BC7
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
            } else if (strstr(ext, "_compressed_texture_s3tc")) {
                has_s3tc = true;
            } else if (strstr(ext, "_texture_compression_rgtc")) {
                has_rgtc = true;
            } else if (strstr(ext, "_texture_compression_bptc")) {
                has_bptc = true;
            } else if (strstr(ext, "_texture_compression_pvrtc")) {
                has_pvrtc = true;
            } else if (strstr(ext, "_compressed_texture_pvrtc")) {
                has_pvrtc = true;
            } else if (strstr(ext, "_compressed_texture_etc")) {
                has_etc2 = true;
            } else if (strstr(ext, "_color_buffer_float")) {
                has_colorbuffer_float = true;
            } else if (strstr(ext, "_color_buffer_half_float")) {
                has_colorbuffer_half_float = true;
            } else if (strstr(ext, "_texture_float_linear")) {
                has_texture_float_linear = true;
            } else if (strstr(ext, "_float_blend")) {
                has_float_blend = true;
            } else if (strstr(ext, "_texture_filter_anisotropic")) {
                _sg.gl.ext_anisotropic = true;
            }
        }
    }

    /* on WebGL2, color_buffer_float also includes 16-bit formats
       see: https://developer.mozilla.org/en-US/docs/Web/API/EXT_color_buffer_float
    */
    #if defined(__EMSCRIPTEN__)
    if (!has_colorbuffer_half_float && has_colorbuffer_float) {
        has_colorbuffer_half_float = has_colorbuffer_float;
    }
    #endif

    // limits
    _sg_gl_init_limits();

    // pixel formats
    const bool has_bgra = false;    // not a bug
    _sg_gl_init_pixelformats(has_bgra);
    _sg_gl_init_pixelformats_float(has_colorbuffer_float, has_texture_float_linear, has_float_blend);
    _sg_gl_init_pixelformats_half_float(has_colorbuffer_half_float);
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

//-- state cache implementation ------------------------------------------------
_SOKOL_PRIVATE void _sg_gl_cache_clear_buffer_bindings(bool force) {
    if (force || (_sg.gl.cache.vertex_buffer != 0)) {
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        _sg.gl.cache.vertex_buffer = 0;
        _sg_stats_add(gl.num_bind_buffer, 1);
    }
    if (force || (_sg.gl.cache.index_buffer != 0)) {
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
        _sg.gl.cache.index_buffer = 0;
        _sg_stats_add(gl.num_bind_buffer, 1);
    }
}

_SOKOL_PRIVATE void _sg_gl_cache_bind_buffer(GLenum target, GLuint buffer) {
    SOKOL_ASSERT((GL_ARRAY_BUFFER == target) || (GL_ELEMENT_ARRAY_BUFFER == target));
    if (target == GL_ARRAY_BUFFER) {
        if (_sg.gl.cache.vertex_buffer != buffer) {
            _sg.gl.cache.vertex_buffer = buffer;
            glBindBuffer(target, buffer);
            _sg_stats_add(gl.num_bind_buffer, 1);
        }
    } else {
        if (_sg.gl.cache.index_buffer != buffer) {
            _sg.gl.cache.index_buffer = buffer;
            glBindBuffer(target, buffer);
            _sg_stats_add(gl.num_bind_buffer, 1);
        }
    }
}

_SOKOL_PRIVATE void _sg_gl_cache_store_buffer_binding(GLenum target) {
    if (target == GL_ARRAY_BUFFER) {
        _sg.gl.cache.stored_vertex_buffer = _sg.gl.cache.vertex_buffer;
    } else {
        _sg.gl.cache.stored_index_buffer = _sg.gl.cache.index_buffer;
    }
}

_SOKOL_PRIVATE void _sg_gl_cache_restore_buffer_binding(GLenum target) {
    if (target == GL_ARRAY_BUFFER) {
        if (_sg.gl.cache.stored_vertex_buffer != 0) {
            // we only care about restoring valid ids
            _sg_gl_cache_bind_buffer(target, _sg.gl.cache.stored_vertex_buffer);
            _sg.gl.cache.stored_vertex_buffer = 0;
        }
    } else {
        if (_sg.gl.cache.stored_index_buffer != 0) {
            // we only care about restoring valid ids
            _sg_gl_cache_bind_buffer(target, _sg.gl.cache.stored_index_buffer);
            _sg.gl.cache.stored_index_buffer = 0;
        }
    }
}

// called when _sg_gl_discard_buffer()
_SOKOL_PRIVATE void _sg_gl_cache_invalidate_buffer(GLuint buf) {
    if (buf == _sg.gl.cache.vertex_buffer) {
        _sg.gl.cache.vertex_buffer = 0;
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        _sg_stats_add(gl.num_bind_buffer, 1);
    }
    if (buf == _sg.gl.cache.index_buffer) {
        _sg.gl.cache.index_buffer = 0;
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
        _sg_stats_add(gl.num_bind_buffer, 1);
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
    _SG_GL_CHECK_ERROR();
    if (_sg.gl.cache.cur_active_texture != texture) {
        _sg.gl.cache.cur_active_texture = texture;
        glActiveTexture(texture);
        _sg_stats_add(gl.num_active_texture, 1);
    }
    _SG_GL_CHECK_ERROR();
}

_SOKOL_PRIVATE void _sg_gl_cache_clear_texture_sampler_bindings(bool force) {
    _SG_GL_CHECK_ERROR();
    for (int i = 0; (i < _SG_GL_TEXTURE_SAMPLER_CACHE_SIZE) && (i < _sg.limits.gl_max_combined_texture_image_units); i++) {
        if (force || (_sg.gl.cache.texture_samplers[i].texture != 0)) {
            GLenum gl_texture_unit = (GLenum) (GL_TEXTURE0 + i);
            glActiveTexture(gl_texture_unit);
            _sg_stats_add(gl.num_active_texture, 1);
            glBindTexture(GL_TEXTURE_2D, 0);
            glBindTexture(GL_TEXTURE_CUBE_MAP, 0);
            glBindTexture(GL_TEXTURE_3D, 0);
            glBindTexture(GL_TEXTURE_2D_ARRAY, 0);
            _sg_stats_add(gl.num_bind_texture, 4);
            glBindSampler((GLuint)i, 0);
            _sg_stats_add(gl.num_bind_sampler, 1);
            _sg.gl.cache.texture_samplers[i].target = 0;
            _sg.gl.cache.texture_samplers[i].texture = 0;
            _sg.gl.cache.texture_samplers[i].sampler = 0;
            _sg.gl.cache.cur_active_texture = gl_texture_unit;
        }
    }
    _SG_GL_CHECK_ERROR();
}

_SOKOL_PRIVATE void _sg_gl_cache_bind_texture_sampler(int slot_index, GLenum target, GLuint texture, GLuint sampler) {
    /* it's valid to call this function with target=0 and/or texture=0
       target=0 will unbind the previous binding, texture=0 will clear
       the new binding
    */
    SOKOL_ASSERT((slot_index >= 0) && (slot_index < _SG_GL_TEXTURE_SAMPLER_CACHE_SIZE));
    if (slot_index >= _sg.limits.gl_max_combined_texture_image_units) {
        return;
    }
    _SG_GL_CHECK_ERROR();
    _sg_gl_cache_texture_sampler_bind_slot* slot = &_sg.gl.cache.texture_samplers[slot_index];
    if ((slot->target != target) || (slot->texture != texture) || (slot->sampler != sampler)) {
        _sg_gl_cache_active_texture((GLenum)(GL_TEXTURE0 + slot_index));
        // if the target has changed, clear the previous binding on that target
        if ((target != slot->target) && (slot->target != 0)) {
            glBindTexture(slot->target, 0);
            _SG_GL_CHECK_ERROR();
            _sg_stats_add(gl.num_bind_texture, 1);
        }
        // apply new binding (can be 0 to unbind)
        if (target != 0) {
            glBindTexture(target, texture);
            _SG_GL_CHECK_ERROR();
            _sg_stats_add(gl.num_bind_texture, 1);
        }
        // apply new sampler (can be 0 to unbind)
        glBindSampler((GLuint)slot_index, sampler);
        _SG_GL_CHECK_ERROR();
        _sg_stats_add(gl.num_bind_sampler, 1);

        slot->target = target;
        slot->texture = texture;
        slot->sampler = sampler;
    }
}

_SOKOL_PRIVATE void _sg_gl_cache_store_texture_sampler_binding(int slot_index) {
    SOKOL_ASSERT((slot_index >= 0) && (slot_index < _SG_GL_TEXTURE_SAMPLER_CACHE_SIZE));
    _sg.gl.cache.stored_texture_sampler = _sg.gl.cache.texture_samplers[slot_index];
}

_SOKOL_PRIVATE void _sg_gl_cache_restore_texture_sampler_binding(int slot_index) {
    SOKOL_ASSERT((slot_index >= 0) && (slot_index < _SG_GL_TEXTURE_SAMPLER_CACHE_SIZE));
    _sg_gl_cache_texture_sampler_bind_slot* slot = &_sg.gl.cache.stored_texture_sampler;
    if (slot->texture != 0) {
        // we only care about restoring valid ids
        SOKOL_ASSERT(slot->target != 0);
        _sg_gl_cache_bind_texture_sampler(slot_index, slot->target, slot->texture, slot->sampler);
        slot->target = 0;
        slot->texture = 0;
        slot->sampler = 0;
    }
}

// called from _sg_gl_discard_texture() and _sg_gl_discard_sampler()
_SOKOL_PRIVATE void _sg_gl_cache_invalidate_texture_sampler(GLuint tex, GLuint smp) {
    _SG_GL_CHECK_ERROR();
    for (int i = 0; i < _SG_GL_TEXTURE_SAMPLER_CACHE_SIZE; i++) {
        _sg_gl_cache_texture_sampler_bind_slot* slot = &_sg.gl.cache.texture_samplers[i];
        if ((0 != slot->target) && ((tex == slot->texture) || (smp == slot->sampler))) {
            _sg_gl_cache_active_texture((GLenum)(GL_TEXTURE0 + i));
            glBindTexture(slot->target, 0);
            _SG_GL_CHECK_ERROR();
            _sg_stats_add(gl.num_bind_texture, 1);
            glBindSampler((GLuint)i, 0);
            _SG_GL_CHECK_ERROR();
            _sg_stats_add(gl.num_bind_sampler, 1);
            slot->target = 0;
            slot->texture = 0;
            slot->sampler = 0;
        }
    }
    if ((tex == _sg.gl.cache.stored_texture_sampler.texture) || (smp == _sg.gl.cache.stored_texture_sampler.sampler)) {
        _sg.gl.cache.stored_texture_sampler.target = 0;
        _sg.gl.cache.stored_texture_sampler.texture = 0;
        _sg.gl.cache.stored_texture_sampler.sampler = 0;
    }
}

// called from _sg_gl_discard_shader()
_SOKOL_PRIVATE void _sg_gl_cache_invalidate_program(GLuint prog) {
    if (prog == _sg.gl.cache.prog) {
        _sg.gl.cache.prog = 0;
        glUseProgram(0);
        _sg_stats_add(gl.num_use_program, 1);
    }
}

// called from _sg_gl_discard_pipeline()
_SOKOL_PRIVATE void _sg_gl_cache_invalidate_pipeline(_sg_pipeline_t* pip) {
    if (pip == _sg.gl.cache.cur_pipeline) {
        _sg.gl.cache.cur_pipeline = 0;
        _sg.gl.cache.cur_pipeline_id.id = SG_INVALID_ID;
    }
}

_SOKOL_PRIVATE void _sg_gl_reset_state_cache(void) {
    if (_sg.gl.cur_context) {
        _SG_GL_CHECK_ERROR();
        glBindVertexArray(_sg.gl.cur_context->vao);
        _SG_GL_CHECK_ERROR();
        _sg_clear(&_sg.gl.cache, sizeof(_sg.gl.cache));
        _sg_gl_cache_clear_buffer_bindings(true);
        _SG_GL_CHECK_ERROR();
        _sg_gl_cache_clear_texture_sampler_bindings(true);
        _SG_GL_CHECK_ERROR();
        for (int i = 0; i < _sg.limits.max_vertex_attrs; i++) {
            _sg_gl_attr_t* attr = &_sg.gl.cache.attrs[i].gl_attr;
            attr->vb_index = -1;
            attr->divisor = -1;
            glDisableVertexAttribArray((GLuint)i);
            _SG_GL_CHECK_ERROR();
            _sg_stats_add(gl.num_disable_vertex_attrib_array, 1);
        }
        _sg.gl.cache.cur_primitive_type = GL_TRIANGLES;

        // shader program
        glGetIntegerv(GL_CURRENT_PROGRAM, (GLint*)&_sg.gl.cache.prog);
        _SG_GL_CHECK_ERROR();

        // depth and stencil state
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
        _sg_stats_add(gl.num_render_state, 7);

        // blend state
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
        _sg_stats_add(gl.num_render_state, 4);

        // standalone state
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
        _sg_stats_add(gl.num_render_state, 10);
        #if defined(SOKOL_GLCORE33)
            glEnable(GL_MULTISAMPLE);
            glEnable(GL_PROGRAM_POINT_SIZE);
            _sg_stats_add(gl.num_render_state, 2);
        #endif
    }
}

_SOKOL_PRIVATE void _sg_gl_setup_backend(const sg_desc* desc) {
    _SOKOL_UNUSED(desc);
    SOKOL_ASSERT(desc->context.gl.default_framebuffer_cb == 0 || desc->context.gl.default_framebuffer_userdata_cb == 0);

    // assumes that _sg.gl is already zero-initialized
    _sg.gl.valid = true;

    #if defined(_SOKOL_USE_WIN32_GL_LOADER)
    _sg_gl_load_opengl();
    #endif

    // clear initial GL error state
    #if defined(SOKOL_DEBUG)
        while (glGetError() != GL_NO_ERROR);
    #endif
    #if defined(SOKOL_GLCORE33)
        _sg_gl_init_caps_glcore33();
    #elif defined(SOKOL_GLES3)
        _sg_gl_init_caps_gles3();
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
    // NOTE: ctx can be 0 to unset the current context
    _sg.gl.cur_context = ctx;
    _sg_gl_reset_state_cache();
}

//-- GL backend resource creation and destruction ------------------------------
_SOKOL_PRIVATE sg_resource_state _sg_gl_create_context(_sg_context_t* ctx) {
    SOKOL_ASSERT(ctx);
    SOKOL_ASSERT(0 == ctx->default_framebuffer);
    _SG_GL_CHECK_ERROR();
    glGetIntegerv(GL_FRAMEBUFFER_BINDING, (GLint*)&ctx->default_framebuffer);
    _SG_GL_CHECK_ERROR();
    SOKOL_ASSERT(0 == ctx->vao);
    glGenVertexArrays(1, &ctx->vao);
    glBindVertexArray(ctx->vao);
    _SG_GL_CHECK_ERROR();
    // incoming texture data is generally expected to be packed tightly
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    #if defined(SOKOL_GLCORE33)
        // enable seamless cubemap sampling (only desktop GL)
        glEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS);
    #endif
    return SG_RESOURCESTATE_VALID;
}

_SOKOL_PRIVATE void _sg_gl_discard_context(_sg_context_t* ctx) {
    SOKOL_ASSERT(ctx);
    if (ctx->vao) {
        glDeleteVertexArrays(1, &ctx->vao);
    }
    _SG_GL_CHECK_ERROR();
}

_SOKOL_PRIVATE sg_resource_state _sg_gl_create_buffer(_sg_buffer_t* buf, const sg_buffer_desc* desc) {
    SOKOL_ASSERT(buf && desc);
    _SG_GL_CHECK_ERROR();
    buf->gl.injected = (0 != desc->gl_buffers[0]);
    const GLenum gl_target = _sg_gl_buffer_target(buf->cmn.type);
    const GLenum gl_usage  = _sg_gl_usage(buf->cmn.usage);
    for (int slot = 0; slot < buf->cmn.num_slots; slot++) {
        GLuint gl_buf = 0;
        if (buf->gl.injected) {
            SOKOL_ASSERT(desc->gl_buffers[slot]);
            gl_buf = desc->gl_buffers[slot];
        } else {
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
            if (!buf->gl.injected) {
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
    img->gl.injected = (0 != desc->gl_textures[0]);

    // check if texture format is support
    if (!_sg_gl_supported_texture_format(img->cmn.pixel_format)) {
        _SG_ERROR(GL_TEXTURE_FORMAT_NOT_SUPPORTED);
        return SG_RESOURCESTATE_FAILED;
    }
    const GLenum gl_internal_format = _sg_gl_teximage_internal_format(img->cmn.pixel_format);

    // if this is a MSAA render target, a render buffer object will be created instead of a regulat texture
    // (since GLES3 has no multisampled texture objects)
    if (img->cmn.render_target && (img->cmn.sample_count > 1)) {
        glGenRenderbuffers(1, &img->gl.msaa_render_buffer);
        glBindRenderbuffer(GL_RENDERBUFFER, img->gl.msaa_render_buffer);
        glRenderbufferStorageMultisample(GL_RENDERBUFFER, img->cmn.sample_count, gl_internal_format, img->cmn.width, img->cmn.height);
    } else if (img->gl.injected) {
        img->gl.target = _sg_gl_texture_target(img->cmn.type);
        // inject externally GL textures
        for (int slot = 0; slot < img->cmn.num_slots; slot++) {
            SOKOL_ASSERT(desc->gl_textures[slot]);
            img->gl.tex[slot] = desc->gl_textures[slot];
        }
        if (desc->gl_texture_target) {
            img->gl.target = (GLenum)desc->gl_texture_target;
        }
    } else {
        // create our own GL texture(s)
        img->gl.target = _sg_gl_texture_target(img->cmn.type);
        const GLenum gl_format = _sg_gl_teximage_format(img->cmn.pixel_format);
        const bool is_compressed = _sg_is_compressed_pixel_format(img->cmn.pixel_format);
        for (int slot = 0; slot < img->cmn.num_slots; slot++) {
            glGenTextures(1, &img->gl.tex[slot]);
            SOKOL_ASSERT(img->gl.tex[slot]);
            _sg_gl_cache_store_texture_sampler_binding(0);
            _sg_gl_cache_bind_texture_sampler(0, img->gl.target, img->gl.tex[slot], 0);
            glTexParameteri(img->gl.target, GL_TEXTURE_MAX_LEVEL, img->cmn.num_mipmaps - 1);
            const int num_faces = img->cmn.type == SG_IMAGETYPE_CUBE ? 6 : 1;
            int data_index = 0;
            for (int face_index = 0; face_index < num_faces; face_index++) {
                for (int mip_index = 0; mip_index < img->cmn.num_mipmaps; mip_index++, data_index++) {
                    GLenum gl_img_target = img->gl.target;
                    if (SG_IMAGETYPE_CUBE == img->cmn.type) {
                        gl_img_target = _sg_gl_cubeface_target(face_index);
                    }
                    const GLvoid* data_ptr = desc->data.subimage[face_index][mip_index].ptr;
                    const int mip_width = _sg_miplevel_dim(img->cmn.width, mip_index);
                    const int mip_height = _sg_miplevel_dim(img->cmn.height, mip_index);
                    if ((SG_IMAGETYPE_2D == img->cmn.type) || (SG_IMAGETYPE_CUBE == img->cmn.type)) {
                        if (is_compressed) {
                            const GLsizei data_size = (GLsizei) desc->data.subimage[face_index][mip_index].size;
                            glCompressedTexImage2D(gl_img_target, mip_index, gl_internal_format,
                                mip_width, mip_height, 0, data_size, data_ptr);
                        } else {
                            const GLenum gl_type = _sg_gl_teximage_type(img->cmn.pixel_format);
                            glTexImage2D(gl_img_target, mip_index, (GLint)gl_internal_format,
                                mip_width, mip_height, 0, gl_format, gl_type, data_ptr);
                        }
                    } else if ((SG_IMAGETYPE_3D == img->cmn.type) || (SG_IMAGETYPE_ARRAY == img->cmn.type)) {
                        int mip_depth = img->cmn.num_slices;
                        if (SG_IMAGETYPE_3D == img->cmn.type) {
                            mip_depth = _sg_miplevel_dim(mip_depth, mip_index);
                        }
                        if (is_compressed) {
                            const GLsizei data_size = (GLsizei) desc->data.subimage[face_index][mip_index].size;
                            glCompressedTexImage3D(gl_img_target, mip_index, gl_internal_format,
                                mip_width, mip_height, mip_depth, 0, data_size, data_ptr);
                        } else {
                            const GLenum gl_type = _sg_gl_teximage_type(img->cmn.pixel_format);
                            glTexImage3D(gl_img_target, mip_index, (GLint)gl_internal_format,
                                mip_width, mip_height, mip_depth, 0, gl_format, gl_type, data_ptr);
                        }
                    }
                }
            }
            _sg_gl_cache_restore_texture_sampler_binding(0);
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
            _sg_gl_cache_invalidate_texture_sampler(img->gl.tex[slot], 0);
            if (!img->gl.injected) {
                glDeleteTextures(1, &img->gl.tex[slot]);
            }
        }
    }
    if (img->gl.msaa_render_buffer) {
        glDeleteRenderbuffers(1, &img->gl.msaa_render_buffer);
    }
    _SG_GL_CHECK_ERROR();
}

_SOKOL_PRIVATE sg_resource_state _sg_gl_create_sampler(_sg_sampler_t* smp, const sg_sampler_desc* desc) {
    SOKOL_ASSERT(smp && desc);
    _SG_GL_CHECK_ERROR();
    smp->gl.injected = (0 != desc->gl_sampler);
    if (smp->gl.injected) {
        smp->gl.smp = (GLuint) desc->gl_sampler;
    } else {
        glGenSamplers(1, &smp->gl.smp);
        SOKOL_ASSERT(smp->gl.smp);

        const GLenum gl_min_filter = _sg_gl_min_filter(smp->cmn.min_filter, smp->cmn.mipmap_filter);
        const GLenum gl_mag_filter = _sg_gl_mag_filter(smp->cmn.mag_filter);
        glSamplerParameteri(smp->gl.smp, GL_TEXTURE_MIN_FILTER, (GLint)gl_min_filter);
        glSamplerParameteri(smp->gl.smp, GL_TEXTURE_MAG_FILTER, (GLint)gl_mag_filter);
        // GL spec has strange defaults for mipmap min/max lod: -1000 to +1000
        const float min_lod = _sg_clamp(desc->min_lod, 0.0f, 1000.0f);
        const float max_lod = _sg_clamp(desc->max_lod, 0.0f, 1000.0f);
        glSamplerParameterf(smp->gl.smp, GL_TEXTURE_MIN_LOD, min_lod);
        glSamplerParameterf(smp->gl.smp, GL_TEXTURE_MAX_LOD, max_lod);
        glSamplerParameteri(smp->gl.smp, GL_TEXTURE_WRAP_S, (GLint)_sg_gl_wrap(smp->cmn.wrap_u));
        glSamplerParameteri(smp->gl.smp, GL_TEXTURE_WRAP_T, (GLint)_sg_gl_wrap(smp->cmn.wrap_v));
        glSamplerParameteri(smp->gl.smp, GL_TEXTURE_WRAP_R, (GLint)_sg_gl_wrap(smp->cmn.wrap_w));
        #if defined(SOKOL_GLCORE33)
        float border[4];
        switch (smp->cmn.border_color) {
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
        glSamplerParameterfv(smp->gl.smp, GL_TEXTURE_BORDER_COLOR, border);
        #endif
        if (smp->cmn.compare != SG_COMPAREFUNC_NEVER) {
            glSamplerParameteri(smp->gl.smp, GL_TEXTURE_COMPARE_MODE, GL_COMPARE_REF_TO_TEXTURE);
            glSamplerParameteri(smp->gl.smp, GL_TEXTURE_COMPARE_FUNC, (GLint)_sg_gl_compare_func(smp->cmn.compare));
        } else {
            glSamplerParameteri(smp->gl.smp, GL_TEXTURE_COMPARE_MODE, GL_NONE);
        }
        if (_sg.gl.ext_anisotropic && (smp->cmn.max_anisotropy > 1)) {
            GLint max_aniso = (GLint) smp->cmn.max_anisotropy;
            if (max_aniso > _sg.gl.max_anisotropy) {
                max_aniso = _sg.gl.max_anisotropy;
            }
            glSamplerParameteri(smp->gl.smp, GL_TEXTURE_MAX_ANISOTROPY_EXT, max_aniso);
        }
    }
    _SG_GL_CHECK_ERROR();
    return SG_RESOURCESTATE_VALID;
}

_SOKOL_PRIVATE void _sg_gl_discard_sampler(_sg_sampler_t* smp) {
    SOKOL_ASSERT(smp);
    _SG_GL_CHECK_ERROR();
    _sg_gl_cache_invalidate_texture_sampler(0, smp->gl.smp);
    if (!smp->gl.injected) {
        glDeleteSamplers(1, &smp->gl.smp);
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
        // compilation failed, log error and delete shader
        GLint log_len = 0;
        glGetShaderiv(gl_shd, GL_INFO_LOG_LENGTH, &log_len);
        if (log_len > 0) {
            GLchar* log_buf = (GLchar*) _sg_malloc((size_t)log_len);
            glGetShaderInfoLog(gl_shd, log_len, &log_len, log_buf);
            _SG_ERROR(GL_SHADER_COMPILATION_FAILED);
            _SG_LOGMSG(GL_SHADER_COMPILATION_FAILED, log_buf);
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

    // copy the optional vertex attribute names over
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
            _SG_ERROR(GL_SHADER_LINKING_FAILED);
            _SG_LOGMSG(GL_SHADER_LINKING_FAILED, log_buf);
            _sg_free(log_buf);
        }
        glDeleteProgram(gl_prog);
        return SG_RESOURCESTATE_FAILED;
    }
    shd->gl.prog = gl_prog;

    // resolve uniforms
    _SG_GL_CHECK_ERROR();
    for (int stage_index = 0; stage_index < SG_NUM_SHADER_STAGES; stage_index++) {
        const sg_shader_stage_desc* stage_desc = (stage_index == SG_SHADERSTAGE_VS)? &desc->vs : &desc->fs;
        const _sg_shader_stage_t* stage = &shd->cmn.stage[stage_index];
        _sg_gl_shader_stage_t* gl_stage = &shd->gl.stage[stage_index];
        for (int ub_index = 0; ub_index < stage->num_uniform_blocks; ub_index++) {
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
                } else {
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

    // resolve combined image samplers
    _SG_GL_CHECK_ERROR();
    GLuint cur_prog = 0;
    glGetIntegerv(GL_CURRENT_PROGRAM, (GLint*)&cur_prog);
    glUseProgram(gl_prog);
    int gl_tex_slot = 0;
    for (int stage_index = 0; stage_index < SG_NUM_SHADER_STAGES; stage_index++) {
        const sg_shader_stage_desc* stage_desc = (stage_index == SG_SHADERSTAGE_VS)? &desc->vs : &desc->fs;
        const _sg_shader_stage_t* stage = &shd->cmn.stage[stage_index];
        _sg_gl_shader_stage_t* gl_stage = &shd->gl.stage[stage_index];
        for (int img_smp_index = 0; img_smp_index < stage->num_image_samplers; img_smp_index++) {
            const sg_shader_image_sampler_pair_desc* img_smp_desc = &stage_desc->image_sampler_pairs[img_smp_index];
            _sg_gl_shader_image_sampler_t* gl_img_smp = &gl_stage->image_samplers[img_smp_index];
            SOKOL_ASSERT(img_smp_desc->glsl_name);
            GLint gl_loc = glGetUniformLocation(gl_prog, img_smp_desc->glsl_name);
            if (gl_loc != -1) {
                gl_img_smp->gl_tex_slot = gl_tex_slot++;
                glUniform1i(gl_loc, gl_img_smp->gl_tex_slot);
            } else {
                gl_img_smp->gl_tex_slot = -1;
                _SG_ERROR(GL_TEXTURE_NAME_NOT_FOUND_IN_SHADER);
                _SG_LOGMSG(GL_TEXTURE_NAME_NOT_FOUND_IN_SHADER, img_smp_desc->glsl_name);
            }
        }
    }
    // it's legal to call glUseProgram with 0
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
    SOKOL_ASSERT((pip->shader == 0) && (pip->cmn.shader_id.id != SG_INVALID_ID));
    SOKOL_ASSERT(desc->shader.id == shd->slot.id);
    SOKOL_ASSERT(shd->gl.prog);
    pip->shader = shd;
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

    // resolve vertex attributes
    for (int attr_index = 0; attr_index < SG_MAX_VERTEX_ATTRIBUTES; attr_index++) {
        pip->gl.attrs[attr_index].vb_index = -1;
    }
    for (int attr_index = 0; attr_index < _sg.limits.max_vertex_attrs; attr_index++) {
        const sg_vertex_attr_state* a_state = &desc->layout.attrs[attr_index];
        if (a_state->format == SG_VERTEXFORMAT_INVALID) {
            break;
        }
        SOKOL_ASSERT(a_state->buffer_index < SG_MAX_VERTEX_BUFFERS);
        const sg_vertex_buffer_layout_state* l_state = &desc->layout.buffers[a_state->buffer_index];
        const sg_vertex_step step_func = l_state->step_func;
        const int step_rate = l_state->step_rate;
        GLint attr_loc = attr_index;
        if (!_sg_strempty(&shd->gl.attrs[attr_index].name)) {
            attr_loc = glGetAttribLocation(pip->shader->gl.prog, _sg_strptr(&shd->gl.attrs[attr_index].name));
        }
        SOKOL_ASSERT(attr_loc < (GLint)_sg.limits.max_vertex_attrs);
        if (attr_loc != -1) {
            _sg_gl_attr_t* gl_attr = &pip->gl.attrs[attr_loc];
            SOKOL_ASSERT(gl_attr->vb_index == -1);
            gl_attr->vb_index = (int8_t) a_state->buffer_index;
            if (step_func == SG_VERTEXSTEP_PER_VERTEX) {
                gl_attr->divisor = 0;
            } else {
                gl_attr->divisor = (int8_t) step_rate;
                pip->cmn.use_instanced_draw = true;
            }
            SOKOL_ASSERT(l_state->stride > 0);
            gl_attr->stride = (uint8_t) l_state->stride;
            gl_attr->offset = a_state->offset;
            gl_attr->size = (uint8_t) _sg_gl_vertexformat_size(a_state->format);
            gl_attr->type = _sg_gl_vertexformat_type(a_state->format);
            gl_attr->normalized = _sg_gl_vertexformat_normalized(a_state->format);
            pip->cmn.vertex_buffer_layout_active[a_state->buffer_index] = true;
        } else {
            _SG_ERROR(GL_VERTEX_ATTRIBUTE_NOT_FOUND_IN_SHADER);
            _SG_LOGMSG(GL_VERTEX_ATTRIBUTE_NOT_FOUND_IN_SHADER, _sg_strptr(&shd->gl.attrs[attr_index].name));
        }
    }
    return SG_RESOURCESTATE_VALID;
}

_SOKOL_PRIVATE void _sg_gl_discard_pipeline(_sg_pipeline_t* pip) {
    SOKOL_ASSERT(pip);
    _sg_gl_cache_invalidate_pipeline(pip);
}

_SOKOL_PRIVATE void _sg_gl_fb_attach_texture(const _sg_gl_attachment_t* gl_att, const _sg_pass_attachment_common_t* cmn_att, GLenum gl_att_type) {
    const _sg_image_t* img = gl_att->image;
    SOKOL_ASSERT(img);
    const GLuint gl_tex = img->gl.tex[0];
    SOKOL_ASSERT(gl_tex);
    const GLuint gl_target = img->gl.target;
    SOKOL_ASSERT(gl_target);
    const int mip_level = cmn_att->mip_level;
    const int slice = cmn_att->slice;
    switch (img->cmn.type) {
        case SG_IMAGETYPE_2D:
            glFramebufferTexture2D(GL_FRAMEBUFFER, gl_att_type, gl_target, gl_tex, mip_level);
            break;
        case SG_IMAGETYPE_CUBE:
            glFramebufferTexture2D(GL_FRAMEBUFFER, gl_att_type, _sg_gl_cubeface_target(slice), gl_tex, mip_level);
            break;
        default:
            glFramebufferTextureLayer(GL_FRAMEBUFFER, gl_att_type, gl_tex, mip_level, slice);
            break;
    }
}

_SOKOL_PRIVATE GLenum _sg_gl_depth_stencil_attachment_type(const _sg_gl_attachment_t* ds_att) {
    const _sg_image_t* img = ds_att->image;
    SOKOL_ASSERT(img);
    if (_sg_is_depth_stencil_format(img->cmn.pixel_format)) {
        return GL_DEPTH_STENCIL_ATTACHMENT;
    } else {
        return GL_DEPTH_ATTACHMENT;
    }
}

_SOKOL_PRIVATE sg_resource_state _sg_gl_create_pass(_sg_pass_t* pass, _sg_image_t** color_images, _sg_image_t** resolve_images, _sg_image_t* ds_image, const sg_pass_desc* desc) {
    SOKOL_ASSERT(pass && desc);
    SOKOL_ASSERT(color_images && resolve_images);
    _SG_GL_CHECK_ERROR();

    // copy image pointers
    for (int i = 0; i < pass->cmn.num_color_atts; i++) {
        const sg_pass_attachment_desc* color_desc = &desc->color_attachments[i];
        _SOKOL_UNUSED(color_desc);
        SOKOL_ASSERT(color_desc->image.id != SG_INVALID_ID);
        SOKOL_ASSERT(0 == pass->gl.color_atts[i].image);
        SOKOL_ASSERT(color_images[i] && (color_images[i]->slot.id == color_desc->image.id));
        SOKOL_ASSERT(_sg_is_valid_rendertarget_color_format(color_images[i]->cmn.pixel_format));
        pass->gl.color_atts[i].image = color_images[i];

        const sg_pass_attachment_desc* resolve_desc = &desc->resolve_attachments[i];
        if (resolve_desc->image.id != SG_INVALID_ID) {
            SOKOL_ASSERT(0 == pass->gl.resolve_atts[i].image);
            SOKOL_ASSERT(resolve_images[i] && (resolve_images[i]->slot.id == resolve_desc->image.id));
            SOKOL_ASSERT(color_images[i] && (color_images[i]->cmn.pixel_format == resolve_images[i]->cmn.pixel_format));
            pass->gl.resolve_atts[i].image = resolve_images[i];
        }
    }
    SOKOL_ASSERT(0 == pass->gl.ds_att.image);
    const sg_pass_attachment_desc* ds_desc = &desc->depth_stencil_attachment;
    if (ds_desc->image.id != SG_INVALID_ID) {
        SOKOL_ASSERT(ds_image && (ds_image->slot.id == ds_desc->image.id));
        SOKOL_ASSERT(_sg_is_valid_rendertarget_depth_format(ds_image->cmn.pixel_format));
        pass->gl.ds_att.image = ds_image;
    }

    // store current framebuffer binding (restored at end of function)
    GLuint gl_orig_fb;
    glGetIntegerv(GL_FRAMEBUFFER_BINDING, (GLint*)&gl_orig_fb);

    // create a framebuffer object
    glGenFramebuffers(1, &pass->gl.fb);
    glBindFramebuffer(GL_FRAMEBUFFER, pass->gl.fb);

    // attach color attachments to framebuffer
    for (int i = 0; i < pass->cmn.num_color_atts; i++) {
        const _sg_image_t* color_img = pass->gl.color_atts[i].image;
        SOKOL_ASSERT(color_img);
        const GLuint gl_msaa_render_buffer = color_img->gl.msaa_render_buffer;
        if (gl_msaa_render_buffer) {
            glFramebufferRenderbuffer(GL_FRAMEBUFFER, (GLenum)(GL_COLOR_ATTACHMENT0+i), GL_RENDERBUFFER, gl_msaa_render_buffer);
        } else {
            const GLenum gl_att_type = (GLenum)(GL_COLOR_ATTACHMENT0 + i);
            _sg_gl_fb_attach_texture(&pass->gl.color_atts[i], &pass->cmn.color_atts[i], gl_att_type);
        }
    }
    // attach depth-stencil attachement
    if (pass->gl.ds_att.image) {
        const GLenum gl_att = _sg_gl_depth_stencil_attachment_type(&pass->gl.ds_att);
        const _sg_image_t* ds_img = pass->gl.ds_att.image;
        const GLuint gl_msaa_render_buffer = ds_img->gl.msaa_render_buffer;
        if (gl_msaa_render_buffer) {
            glFramebufferRenderbuffer(GL_FRAMEBUFFER, gl_att, GL_RENDERBUFFER, gl_msaa_render_buffer);
        } else {
            const GLenum gl_att_type = _sg_gl_depth_stencil_attachment_type(&pass->gl.ds_att);
            _sg_gl_fb_attach_texture(&pass->gl.ds_att, &pass->cmn.ds_att, gl_att_type);
        }
    }

    // check if framebuffer is complete
    {
        const GLenum fb_status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
        if (fb_status != GL_FRAMEBUFFER_COMPLETE) {
            switch (fb_status) {
                case GL_FRAMEBUFFER_UNDEFINED:
                    _SG_ERROR(GL_FRAMEBUFFER_STATUS_UNDEFINED);
                    break;
                case GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT:
                    _SG_ERROR(GL_FRAMEBUFFER_STATUS_INCOMPLETE_ATTACHMENT);
                    break;
                case GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT:
                    _SG_ERROR(GL_FRAMEBUFFER_STATUS_INCOMPLETE_MISSING_ATTACHMENT);
                    break;
                case GL_FRAMEBUFFER_UNSUPPORTED:
                    _SG_ERROR(GL_FRAMEBUFFER_STATUS_UNSUPPORTED);
                    break;
                case GL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE:
                    _SG_ERROR(GL_FRAMEBUFFER_STATUS_INCOMPLETE_MULTISAMPLE);
                    break;
                default:
                    _SG_ERROR(GL_FRAMEBUFFER_STATUS_UNKNOWN);
                    break;
            }
            return SG_RESOURCESTATE_FAILED;
        }
    }

    // setup color attachments for the framebuffer
    static const GLenum gl_draw_bufs[SG_MAX_COLOR_ATTACHMENTS] = {
        GL_COLOR_ATTACHMENT0,
        GL_COLOR_ATTACHMENT1,
        GL_COLOR_ATTACHMENT2,
        GL_COLOR_ATTACHMENT3
    };
    glDrawBuffers(pass->cmn.num_color_atts, gl_draw_bufs);

    // create MSAA resolve framebuffers if necessary
    for (int i = 0; i < pass->cmn.num_color_atts; i++) {
        _sg_gl_attachment_t* gl_resolve_att = &pass->gl.resolve_atts[i];
        if (gl_resolve_att->image) {
            _sg_pass_attachment_t* cmn_resolve_att = &pass->cmn.resolve_atts[i];
            SOKOL_ASSERT(0 == pass->gl.msaa_resolve_framebuffer[i]);
            glGenFramebuffers(1, &pass->gl.msaa_resolve_framebuffer[i]);
            glBindFramebuffer(GL_FRAMEBUFFER, pass->gl.msaa_resolve_framebuffer[i]);
            _sg_gl_fb_attach_texture(gl_resolve_att, cmn_resolve_att, GL_COLOR_ATTACHMENT0);
            // check if framebuffer is complete
            const GLenum fb_status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
            if (fb_status != GL_FRAMEBUFFER_COMPLETE) {
                switch (fb_status) {
                    case GL_FRAMEBUFFER_UNDEFINED:
                        _SG_ERROR(GL_FRAMEBUFFER_STATUS_UNDEFINED);
                        break;
                    case GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT:
                        _SG_ERROR(GL_FRAMEBUFFER_STATUS_INCOMPLETE_ATTACHMENT);
                        break;
                    case GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT:
                        _SG_ERROR(GL_FRAMEBUFFER_STATUS_INCOMPLETE_MISSING_ATTACHMENT);
                        break;
                    case GL_FRAMEBUFFER_UNSUPPORTED:
                        _SG_ERROR(GL_FRAMEBUFFER_STATUS_UNSUPPORTED);
                        break;
                    case GL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE:
                        _SG_ERROR(GL_FRAMEBUFFER_STATUS_INCOMPLETE_MULTISAMPLE);
                        break;
                    default:
                        _SG_ERROR(GL_FRAMEBUFFER_STATUS_UNKNOWN);
                        break;
                }
                return SG_RESOURCESTATE_FAILED;
            }
            // setup color attachments for the framebuffer
            glDrawBuffers(1, &gl_draw_bufs[0]);
        }
    }

    // restore original framebuffer binding
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
        if (pass->gl.msaa_resolve_framebuffer[i]) {
            glDeleteFramebuffers(1, &pass->gl.msaa_resolve_framebuffer[i]);
        }
    }
    _SG_GL_CHECK_ERROR();
}

_SOKOL_PRIVATE _sg_image_t* _sg_gl_pass_color_image(const _sg_pass_t* pass, int index) {
    SOKOL_ASSERT(pass && (index >= 0) && (index < SG_MAX_COLOR_ATTACHMENTS));
    return pass->gl.color_atts[index].image;
}

_SOKOL_PRIVATE _sg_image_t* _sg_gl_pass_resolve_image(const _sg_pass_t* pass, int index) {
    SOKOL_ASSERT(pass && (index >= 0) && (index < SG_MAX_COLOR_ATTACHMENTS));
    return pass->gl.resolve_atts[index].image;
}

_SOKOL_PRIVATE _sg_image_t* _sg_gl_pass_ds_image(const _sg_pass_t* pass) {
    SOKOL_ASSERT(pass);
    return pass->gl.ds_att.image;
}

_SOKOL_PRIVATE void _sg_gl_begin_pass(_sg_pass_t* pass, const sg_pass_action* action, int w, int h) {
    // FIXME: what if a texture used as render target is still bound, should we
    // unbind all currently bound textures in begin pass?
    SOKOL_ASSERT(action);
    SOKOL_ASSERT(!_sg.gl.in_pass);
    _SG_GL_CHECK_ERROR();
    _sg.gl.in_pass = true;
    _sg.gl.cur_pass = pass; // can be 0
    if (pass) {
        _sg.gl.cur_pass_id.id = pass->slot.id;
    } else {
        _sg.gl.cur_pass_id.id = SG_INVALID_ID;
    }
    _sg.gl.cur_pass_width = w;
    _sg.gl.cur_pass_height = h;

    // bind the render pass framebuffer
    //
    // FIXME: Disabling SRGB conversion for the default framebuffer is
    // a crude hack to make behaviour for sRGB render target textures
    // identical with the Metal and D3D11 swapchains created by sokol-app.
    //
    // This will need a cleaner solution (e.g. allowing to configure
    // sokol_app.h with an sRGB or RGB framebuffer.
    if (pass) {
        // offscreen pass
        SOKOL_ASSERT(pass->gl.fb);
        #if defined(SOKOL_GLCORE33)
        glEnable(GL_FRAMEBUFFER_SRGB);
        #endif
        glBindFramebuffer(GL_FRAMEBUFFER, pass->gl.fb);
    } else {
        // default pass
        SOKOL_ASSERT(_sg.gl.cur_context);
        #if defined(SOKOL_GLCORE33)
        glDisable(GL_FRAMEBUFFER_SRGB);
        #endif
        if (_sg.desc.context.gl.default_framebuffer_userdata_cb) {
            _sg.gl.cur_context->default_framebuffer = _sg.desc.context.gl.default_framebuffer_userdata_cb(_sg.desc.context.gl.user_data);
        } else if (_sg.desc.context.gl.default_framebuffer_cb) {
            _sg.gl.cur_context->default_framebuffer = _sg.desc.context.gl.default_framebuffer_cb();
        }

        glBindFramebuffer(GL_FRAMEBUFFER, _sg.gl.cur_context->default_framebuffer);
    }
    glViewport(0, 0, w, h);
    glScissor(0, 0, w, h);

    // number of color attachments
    const int num_color_atts = pass ? pass->cmn.num_color_atts : 1;

    // clear color and depth-stencil attachments if needed
    bool clear_any_color = false;
    for (int i = 0; i < num_color_atts; i++) {
        if (SG_LOADACTION_CLEAR == action->colors[i].load_action) {
            clear_any_color = true;
            break;
        }
    }
    const bool clear_depth = (action->depth.load_action == SG_LOADACTION_CLEAR);
    const bool clear_stencil = (action->stencil.load_action == SG_LOADACTION_CLEAR);

    bool need_pip_cache_flush = false;
    if (clear_any_color) {
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
        // we messed with the state cache directly, need to clear cached
        // pipeline to force re-evaluation in next sg_apply_pipeline()
        _sg.gl.cache.cur_pipeline = 0;
        _sg.gl.cache.cur_pipeline_id.id = SG_INVALID_ID;
    }
    for (int i = 0; i < num_color_atts; i++) {
        if (action->colors[i].load_action == SG_LOADACTION_CLEAR) {
            glClearBufferfv(GL_COLOR, i, &action->colors[i].clear_value.r);
        }
    }
    if ((pass == 0) || (pass->gl.ds_att.image)) {
        if (clear_depth && clear_stencil) {
            glClearBufferfi(GL_DEPTH_STENCIL, 0, action->depth.clear_value, action->stencil.clear_value);
        } else if (clear_depth) {
            glClearBufferfv(GL_DEPTH, 0, &action->depth.clear_value);
        } else if (clear_stencil) {
            GLint val = (GLint) action->stencil.clear_value;
            glClearBufferiv(GL_STENCIL, 0, &val);
        }
    }
    // keep store actions for end-pass
    for (int i = 0; i < SG_MAX_COLOR_ATTACHMENTS; i++) {
        _sg.gl.color_store_actions[i] = action->colors[i].store_action;
    }
    _sg.gl.depth_store_action = action->depth.store_action;
    _sg.gl.stencil_store_action = action->stencil.store_action;

    _SG_GL_CHECK_ERROR();
}

_SOKOL_PRIVATE void _sg_gl_end_pass(void) {
    SOKOL_ASSERT(_sg.gl.in_pass);
    _SG_GL_CHECK_ERROR();

    if (_sg.gl.cur_pass) {
        const _sg_pass_t* pass = _sg.gl.cur_pass;
        SOKOL_ASSERT(pass->slot.id == _sg.gl.cur_pass_id.id);
        bool fb_read_bound = false;
        bool fb_draw_bound = false;
        const int num_atts = pass->cmn.num_color_atts;
        for (int i = 0; i < num_atts; i++) {
            // perform MSAA resolve if needed
            if (pass->gl.msaa_resolve_framebuffer[i] != 0) {
                if (!fb_read_bound) {
                    SOKOL_ASSERT(pass->gl.fb);
                    glBindFramebuffer(GL_READ_FRAMEBUFFER, pass->gl.fb);
                    fb_read_bound = true;
                }
                const int w = pass->gl.color_atts[i].image->cmn.width;
                const int h = pass->gl.color_atts[i].image->cmn.height;
                glBindFramebuffer(GL_DRAW_FRAMEBUFFER, pass->gl.msaa_resolve_framebuffer[i]);
                glReadBuffer((GLenum)(GL_COLOR_ATTACHMENT0 + i));
                glBlitFramebuffer(0, 0, w, h, 0, 0, w, h, GL_COLOR_BUFFER_BIT, GL_NEAREST);
                fb_draw_bound = true;
            }
        }

        // invalidate framebuffers
        _SOKOL_UNUSED(fb_draw_bound);
        #if defined(SOKOL_GLES3)
        // need to restore framebuffer binding before invalidate if the MSAA resolve had changed the binding
        if (fb_draw_bound) {
            glBindFramebuffer(GL_FRAMEBUFFER, pass->gl.fb);
        }
        GLenum invalidate_atts[SG_MAX_COLOR_ATTACHMENTS + 2] = { 0 };
        int att_index = 0;
        for (int i = 0; i < num_atts; i++) {
            if (_sg.gl.color_store_actions[i] == SG_STOREACTION_DONTCARE) {
                invalidate_atts[att_index++] = (GLenum)(GL_COLOR_ATTACHMENT0 + i);
            }
        }
        if (_sg.gl.depth_store_action == SG_STOREACTION_DONTCARE) {
            invalidate_atts[att_index++] = GL_DEPTH_ATTACHMENT;
        }
        if (_sg.gl.stencil_store_action == SG_STOREACTION_DONTCARE) {
            invalidate_atts[att_index++] = GL_STENCIL_ATTACHMENT;
        }
        if (att_index > 0) {
            glInvalidateFramebuffer(GL_DRAW_FRAMEBUFFER, att_index, invalidate_atts);
        }
        #endif
    }

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

        // update depth state
        {
            const sg_depth_state* state_ds = &pip->gl.depth;
            sg_depth_state* cache_ds = &_sg.gl.cache.depth;
            if (state_ds->compare != cache_ds->compare) {
                cache_ds->compare = state_ds->compare;
                glDepthFunc(_sg_gl_compare_func(state_ds->compare));
                _sg_stats_add(gl.num_render_state, 1);
            }
            if (state_ds->write_enabled != cache_ds->write_enabled) {
                cache_ds->write_enabled = state_ds->write_enabled;
                glDepthMask(state_ds->write_enabled);
                _sg_stats_add(gl.num_render_state, 1);
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
                _sg_stats_add(gl.num_render_state, 1);
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
                    } else {
                        glDisable(GL_POLYGON_OFFSET_FILL);
                    }
                    _sg_stats_add(gl.num_render_state, 1);
                }
            }
        }

        // update stencil state
        {
            const sg_stencil_state* state_ss = &pip->gl.stencil;
            sg_stencil_state* cache_ss = &_sg.gl.cache.stencil;
            if (state_ss->enabled != cache_ss->enabled) {
                cache_ss->enabled = state_ss->enabled;
                if (state_ss->enabled) {
                    glEnable(GL_STENCIL_TEST);
                } else {
                    glDisable(GL_STENCIL_TEST);
                }
                _sg_stats_add(gl.num_render_state, 1);
            }
            if (state_ss->write_mask != cache_ss->write_mask) {
                cache_ss->write_mask = state_ss->write_mask;
                glStencilMask(state_ss->write_mask);
                _sg_stats_add(gl.num_render_state, 1);
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
                    _sg_stats_add(gl.num_render_state, 1);
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
                    _sg_stats_add(gl.num_render_state, 1);
                }
            }
            cache_ss->read_mask = state_ss->read_mask;
            cache_ss->ref = state_ss->ref;
        }

        if (pip->cmn.color_count > 0) {
            // update blend state
            // FIXME: separate blend state per color attachment not support, needs GL4
            const sg_blend_state* state_bs = &pip->gl.blend;
            sg_blend_state* cache_bs = &_sg.gl.cache.blend;
            if (state_bs->enabled != cache_bs->enabled) {
                cache_bs->enabled = state_bs->enabled;
                if (state_bs->enabled) {
                    glEnable(GL_BLEND);
                } else {
                    glDisable(GL_BLEND);
                }
                _sg_stats_add(gl.num_render_state, 1);
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
                _sg_stats_add(gl.num_render_state, 1);
            }
            if ((state_bs->op_rgb != cache_bs->op_rgb) || (state_bs->op_alpha != cache_bs->op_alpha)) {
                cache_bs->op_rgb = state_bs->op_rgb;
                cache_bs->op_alpha = state_bs->op_alpha;
                glBlendEquationSeparate(_sg_gl_blend_op(state_bs->op_rgb), _sg_gl_blend_op(state_bs->op_alpha));
                _sg_stats_add(gl.num_render_state, 1);
            }

            // standalone color target state
            for (GLuint i = 0; i < (GLuint)pip->cmn.color_count; i++) {
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
                    _sg_stats_add(gl.num_render_state, 1);
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
                _sg_stats_add(gl.num_render_state, 1);
            }
        } // pip->cmn.color_count > 0

        if (pip->gl.cull_mode != _sg.gl.cache.cull_mode) {
            _sg.gl.cache.cull_mode = pip->gl.cull_mode;
            if (SG_CULLMODE_NONE == pip->gl.cull_mode) {
                glDisable(GL_CULL_FACE);
                _sg_stats_add(gl.num_render_state, 1);
            } else {
                glEnable(GL_CULL_FACE);
                GLenum gl_mode = (SG_CULLMODE_FRONT == pip->gl.cull_mode) ? GL_FRONT : GL_BACK;
                glCullFace(gl_mode);
                _sg_stats_add(gl.num_render_state, 2);
            }
        }
        if (pip->gl.face_winding != _sg.gl.cache.face_winding) {
            _sg.gl.cache.face_winding = pip->gl.face_winding;
            GLenum gl_winding = (SG_FACEWINDING_CW == pip->gl.face_winding) ? GL_CW : GL_CCW;
            glFrontFace(gl_winding);
            _sg_stats_add(gl.num_render_state, 1);
        }
        if (pip->gl.alpha_to_coverage_enabled != _sg.gl.cache.alpha_to_coverage_enabled) {
            _sg.gl.cache.alpha_to_coverage_enabled = pip->gl.alpha_to_coverage_enabled;
            if (pip->gl.alpha_to_coverage_enabled) {
                glEnable(GL_SAMPLE_ALPHA_TO_COVERAGE);
            } else {
                glDisable(GL_SAMPLE_ALPHA_TO_COVERAGE);
            }
            _sg_stats_add(gl.num_render_state, 1);
        }
        #ifdef SOKOL_GLCORE33
        if (pip->gl.sample_count != _sg.gl.cache.sample_count) {
            _sg.gl.cache.sample_count = pip->gl.sample_count;
            if (pip->gl.sample_count > 1) {
                glEnable(GL_MULTISAMPLE);
            } else {
                glDisable(GL_MULTISAMPLE);
            }
            _sg_stats_add(gl.num_render_state, 1);
        }
        #endif

        // bind shader program
        if (pip->shader->gl.prog != _sg.gl.cache.prog) {
            _sg.gl.cache.prog = pip->shader->gl.prog;
            glUseProgram(pip->shader->gl.prog);
            _sg_stats_add(gl.num_use_program, 1);
        }
    }
    _SG_GL_CHECK_ERROR();
}

_SOKOL_PRIVATE bool _sg_gl_apply_bindings(_sg_bindings_t* bnd) {
    SOKOL_ASSERT(bnd);
    SOKOL_ASSERT(bnd->pip);
    _SG_GL_CHECK_ERROR();

    // bind combined image-samplers
    _SG_GL_CHECK_ERROR();
    for (int stage_index = 0; stage_index < SG_NUM_SHADER_STAGES; stage_index++) {
        const _sg_shader_stage_t* stage = &bnd->pip->shader->cmn.stage[stage_index];
        const _sg_gl_shader_stage_t* gl_stage = &bnd->pip->shader->gl.stage[stage_index];
        _sg_image_t** imgs = (stage_index == SG_SHADERSTAGE_VS) ? bnd->vs_imgs : bnd->fs_imgs;
        _sg_sampler_t** smps = (stage_index == SG_SHADERSTAGE_VS) ? bnd->vs_smps : bnd->fs_smps;
        const int num_imgs = (stage_index == SG_SHADERSTAGE_VS) ? bnd->num_vs_imgs : bnd->num_fs_imgs;
        const int num_smps = (stage_index == SG_SHADERSTAGE_VS) ? bnd->num_vs_smps : bnd->num_fs_smps;
        SOKOL_ASSERT(num_imgs == stage->num_images); _SOKOL_UNUSED(num_imgs);
        SOKOL_ASSERT(num_smps == stage->num_samplers); _SOKOL_UNUSED(num_smps);
        for (int img_smp_index = 0; img_smp_index < stage->num_image_samplers; img_smp_index++) {
            const int gl_tex_slot = gl_stage->image_samplers[img_smp_index].gl_tex_slot;
            if (gl_tex_slot != -1) {
                const int img_index = stage->image_samplers[img_smp_index].image_slot;
                const int smp_index = stage->image_samplers[img_smp_index].sampler_slot;
                SOKOL_ASSERT(img_index < num_imgs);
                SOKOL_ASSERT(smp_index < num_smps);
                _sg_image_t* img = imgs[img_index];
                _sg_sampler_t* smp = smps[smp_index];
                const GLenum gl_tgt = img->gl.target;
                const GLuint gl_tex = img->gl.tex[img->cmn.active_slot];
                const GLuint gl_smp = smp->gl.smp;
                _sg_gl_cache_bind_texture_sampler(gl_tex_slot, gl_tgt, gl_tex, gl_smp);
            }
        }
    }
    _SG_GL_CHECK_ERROR();

    // index buffer (can be 0)
    const GLuint gl_ib = bnd->ib ? bnd->ib->gl.buf[bnd->ib->cmn.active_slot] : 0;
    _sg_gl_cache_bind_buffer(GL_ELEMENT_ARRAY_BUFFER, gl_ib);
    _sg.gl.cache.cur_ib_offset = bnd->ib_offset;

    // vertex attributes
    for (GLuint attr_index = 0; attr_index < (GLuint)_sg.limits.max_vertex_attrs; attr_index++) {
        _sg_gl_attr_t* attr = &bnd->pip->gl.attrs[attr_index];
        _sg_gl_cache_attr_t* cache_attr = &_sg.gl.cache.attrs[attr_index];
        bool cache_attr_dirty = false;
        int vb_offset = 0;
        GLuint gl_vb = 0;
        if (attr->vb_index >= 0) {
            // attribute is enabled
            SOKOL_ASSERT(attr->vb_index < bnd->num_vbs);
            _sg_buffer_t* vb = bnd->vbs[attr->vb_index];
            SOKOL_ASSERT(vb);
            gl_vb = vb->gl.buf[vb->cmn.active_slot];
            vb_offset = bnd->vb_offsets[attr->vb_index] + attr->offset;
            if ((gl_vb != cache_attr->gl_vbuf) ||
                (attr->size != cache_attr->gl_attr.size) ||
                (attr->type != cache_attr->gl_attr.type) ||
                (attr->normalized != cache_attr->gl_attr.normalized) ||
                (attr->stride != cache_attr->gl_attr.stride) ||
                (vb_offset != cache_attr->gl_attr.offset) ||
                (cache_attr->gl_attr.divisor != attr->divisor))
            {
                _sg_gl_cache_bind_buffer(GL_ARRAY_BUFFER, gl_vb);
                glVertexAttribPointer(attr_index, attr->size, attr->type, attr->normalized, attr->stride, (const GLvoid*)(GLintptr)vb_offset);
                _sg_stats_add(gl.num_vertex_attrib_pointer, 1);
                glVertexAttribDivisor(attr_index, (GLuint)attr->divisor);
                _sg_stats_add(gl.num_vertex_attrib_divisor, 1);
                cache_attr_dirty = true;
            }
            if (cache_attr->gl_attr.vb_index == -1) {
                glEnableVertexAttribArray(attr_index);
                _sg_stats_add(gl.num_enable_vertex_attrib_array, 1);
                cache_attr_dirty = true;
            }
        } else {
            // attribute is disabled
            if (cache_attr->gl_attr.vb_index != -1) {
                glDisableVertexAttribArray(attr_index);
                _sg_stats_add(gl.num_disable_vertex_attrib_array, 1);
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
    return true;
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
        _sg_stats_add(gl.num_uniform, 1);
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
        // indexed rendering
        const int i_size = (i_type == GL_UNSIGNED_SHORT) ? 2 : 4;
        const int ib_offset = _sg.gl.cache.cur_ib_offset;
        const GLvoid* indices = (const GLvoid*)(GLintptr)(base_element*i_size+ib_offset);
        if (_sg.gl.cache.cur_pipeline->cmn.use_instanced_draw) {
            glDrawElementsInstanced(p_type, num_elements, i_type, indices, num_instances);
        } else {
            glDrawElements(p_type, num_elements, i_type, indices);
        }
    } else {
        // non-indexed rendering
        if (_sg.gl.cache.cur_pipeline->cmn.use_instanced_draw) {
            glDrawArraysInstanced(p_type, base_element, num_elements, num_instances);
        } else {
            glDrawArrays(p_type, base_element, num_elements);
        }
    }
}

_SOKOL_PRIVATE void _sg_gl_commit(void) {
    SOKOL_ASSERT(!_sg.gl.in_pass);
    // "soft" clear bindings (only those that are actually bound)
    _sg_gl_cache_clear_buffer_bindings(false);
    _sg_gl_cache_clear_texture_sampler_bindings(false);
}

_SOKOL_PRIVATE void _sg_gl_update_buffer(_sg_buffer_t* buf, const sg_range* data) {
    SOKOL_ASSERT(buf && data && data->ptr && (data->size > 0));
    // only one update per buffer per frame allowed
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

_SOKOL_PRIVATE void _sg_gl_append_buffer(_sg_buffer_t* buf, const sg_range* data, bool new_frame) {
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
}

_SOKOL_PRIVATE void _sg_gl_update_image(_sg_image_t* img, const sg_image_data* data) {
    SOKOL_ASSERT(img && data);
    // only one update per image per frame allowed
    if (++img->cmn.active_slot >= img->cmn.num_slots) {
        img->cmn.active_slot = 0;
    }
    SOKOL_ASSERT(img->cmn.active_slot < SG_NUM_INFLIGHT_FRAMES);
    SOKOL_ASSERT(0 != img->gl.tex[img->cmn.active_slot]);
    _sg_gl_cache_store_texture_sampler_binding(0);
    _sg_gl_cache_bind_texture_sampler(0, img->gl.target, img->gl.tex[img->cmn.active_slot], 0);
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
            int mip_width = _sg_miplevel_dim(img->cmn.width, mip_index);
            int mip_height = _sg_miplevel_dim(img->cmn.height, mip_index);
            if ((SG_IMAGETYPE_2D == img->cmn.type) || (SG_IMAGETYPE_CUBE == img->cmn.type)) {
                glTexSubImage2D(gl_img_target, mip_index,
                    0, 0,
                    mip_width, mip_height,
                    gl_img_format, gl_img_type,
                    data_ptr);
            } else if ((SG_IMAGETYPE_3D == img->cmn.type) || (SG_IMAGETYPE_ARRAY == img->cmn.type)) {
                int mip_depth = img->cmn.num_slices;
                if (SG_IMAGETYPE_3D == img->cmn.type) {
                    mip_depth = _sg_miplevel_dim(img->cmn.num_slices, mip_index);
                }
                glTexSubImage3D(gl_img_target, mip_index,
                    0, 0, 0,
                    mip_width, mip_height, mip_depth,
                    gl_img_format, gl_img_type,
                    data_ptr);

            }
        }
    }
    _sg_gl_cache_restore_texture_sampler_binding(0);
}

// ██████  ██████  ██████   ██  ██     ██████   █████   ██████ ██   ██ ███████ ███    ██ ██████
// ██   ██      ██ ██   ██ ███ ███     ██   ██ ██   ██ ██      ██  ██  ██      ████   ██ ██   ██
// ██   ██  █████  ██   ██  ██  ██     ██████  ███████ ██      █████   █████   ██ ██  ██ ██   ██
// ██   ██      ██ ██   ██  ██  ██     ██   ██ ██   ██ ██      ██  ██  ██      ██  ██ ██ ██   ██
// ██████  ██████  ██████   ██  ██     ██████  ██   ██  ██████ ██   ██ ███████ ██   ████ ██████
//
// >>d3d11 backend
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

//-- D3D11 C/C++ wrappers ------------------------------------------------------
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

//-- enum translation functions ------------------------------------------------
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

_SOKOL_PRIVATE DXGI_FORMAT _sg_d3d11_texture_pixel_format(sg_pixel_format fmt) {
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
        case SG_PIXELFORMAT_SRGB8A8:        return DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
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
        case SG_PIXELFORMAT_DEPTH:          return DXGI_FORMAT_R32_TYPELESS;
        case SG_PIXELFORMAT_DEPTH_STENCIL:  return DXGI_FORMAT_R24G8_TYPELESS;
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

_SOKOL_PRIVATE DXGI_FORMAT _sg_d3d11_srv_pixel_format(sg_pixel_format fmt) {
    if (fmt == SG_PIXELFORMAT_DEPTH) {
        return DXGI_FORMAT_R32_FLOAT;
    } else if (fmt == SG_PIXELFORMAT_DEPTH_STENCIL) {
        return DXGI_FORMAT_R24_UNORM_X8_TYPELESS;
    } else {
        return _sg_d3d11_texture_pixel_format(fmt);
    }
}

_SOKOL_PRIVATE DXGI_FORMAT _sg_d3d11_dsv_pixel_format(sg_pixel_format fmt) {
    if (fmt == SG_PIXELFORMAT_DEPTH) {
        return DXGI_FORMAT_D32_FLOAT;
    } else if (fmt == SG_PIXELFORMAT_DEPTH_STENCIL) {
        return DXGI_FORMAT_D24_UNORM_S8_UINT;
    } else {
        return _sg_d3d11_texture_pixel_format(fmt);
    }
}

_SOKOL_PRIVATE DXGI_FORMAT _sg_d3d11_rtv_pixel_format(sg_pixel_format fmt) {
    if (fmt == SG_PIXELFORMAT_DEPTH) {
        return DXGI_FORMAT_R32_FLOAT;
    } else if (fmt == SG_PIXELFORMAT_DEPTH_STENCIL) {
        return DXGI_FORMAT_R24_UNORM_X8_TYPELESS;
    } else {
        return _sg_d3d11_texture_pixel_format(fmt);
    }
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

_SOKOL_PRIVATE D3D11_FILTER _sg_d3d11_filter(sg_filter min_f, sg_filter mag_f, sg_filter mipmap_f, bool comparison, uint32_t max_anisotropy) {
    uint32_t d3d11_filter = 0;
    if (max_anisotropy > 1) {
        // D3D11_FILTER_ANISOTROPIC = 0x55,
        d3d11_filter |= 0x55;
    } else {
        // D3D11_FILTER_MIN_MAG_MIP_POINT = 0,
        // D3D11_FILTER_MIN_MAG_POINT_MIP_LINEAR = 0x1,
        // D3D11_FILTER_MIN_POINT_MAG_LINEAR_MIP_POINT = 0x4,
        // D3D11_FILTER_MIN_POINT_MAG_MIP_LINEAR = 0x5,
        // D3D11_FILTER_MIN_LINEAR_MAG_MIP_POINT = 0x10,
        // D3D11_FILTER_MIN_LINEAR_MAG_POINT_MIP_LINEAR = 0x11,
        // D3D11_FILTER_MIN_MAG_LINEAR_MIP_POINT = 0x14,
        // D3D11_FILTER_MIN_MAG_MIP_LINEAR = 0x15,
        if (mipmap_f == SG_FILTER_LINEAR) {
            d3d11_filter |= 0x01;
        }
        if (mag_f == SG_FILTER_LINEAR) {
            d3d11_filter |= 0x04;
        }
        if (min_f == SG_FILTER_LINEAR) {
            d3d11_filter |= 0x10;
        }
    }
    // D3D11_FILTER_COMPARISON_MIN_MAG_MIP_POINT = 0x80,
    // D3D11_FILTER_COMPARISON_MIN_MAG_POINT_MIP_LINEAR = 0x81,
    // D3D11_FILTER_COMPARISON_MIN_POINT_MAG_LINEAR_MIP_POINT = 0x84,
    // D3D11_FILTER_COMPARISON_MIN_POINT_MAG_MIP_LINEAR = 0x85,
    // D3D11_FILTER_COMPARISON_MIN_LINEAR_MAG_MIP_POINT = 0x90,
    // D3D11_FILTER_COMPARISON_MIN_LINEAR_MAG_POINT_MIP_LINEAR = 0x91,
    // D3D11_FILTER_COMPARISON_MIN_MAG_LINEAR_MIP_POINT = 0x94,
    // D3D11_FILTER_COMPARISON_MIN_MAG_MIP_LINEAR = 0x95,
    // D3D11_FILTER_COMPARISON_ANISOTROPIC = 0xd5,
    if (comparison) {
        d3d11_filter |= 0x80;
    }
    return (D3D11_FILTER)d3d11_filter;
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
        case SG_VERTEXFORMAT_HALF2:     return DXGI_FORMAT_R16G16_FLOAT;
        case SG_VERTEXFORMAT_HALF4:     return DXGI_FORMAT_R16G16B16A16_FLOAT;
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

_SOKOL_PRIVATE UINT _sg_d3d11_dxgi_fmt_caps(DXGI_FORMAT dxgi_fmt) {
    UINT dxgi_fmt_caps = 0;
    if (dxgi_fmt != DXGI_FORMAT_UNKNOWN) {
        HRESULT hr = _sg_d3d11_CheckFormatSupport(_sg.d3d11.dev, dxgi_fmt, &dxgi_fmt_caps);
        SOKOL_ASSERT(SUCCEEDED(hr) || (E_FAIL == hr));
        if (!SUCCEEDED(hr)) {
            dxgi_fmt_caps = 0;
        }
    }
    return dxgi_fmt_caps;
}

// see: https://docs.microsoft.com/en-us/windows/win32/direct3d11/overviews-direct3d-11-resources-limits#resource-limits-for-feature-level-11-hardware
_SOKOL_PRIVATE void _sg_d3d11_init_caps(void) {
    _sg.backend = SG_BACKEND_D3D11;

    _sg.features.origin_top_left = true;
    _sg.features.image_clamp_to_border = true;
    _sg.features.mrt_independent_blend_state = true;
    _sg.features.mrt_independent_write_mask = true;

    _sg.limits.max_image_size_2d = 16 * 1024;
    _sg.limits.max_image_size_cube = 16 * 1024;
    _sg.limits.max_image_size_3d = 2 * 1024;
    _sg.limits.max_image_size_array = 16 * 1024;
    _sg.limits.max_image_array_layers = 2 * 1024;
    _sg.limits.max_vertex_attrs = SG_MAX_VERTEX_ATTRIBUTES;

    // see: https://docs.microsoft.com/en-us/windows/win32/api/d3d11/ne-d3d11-d3d11_format_support
    for (int fmt = (SG_PIXELFORMAT_NONE+1); fmt < _SG_PIXELFORMAT_NUM; fmt++) {
        const UINT srv_dxgi_fmt_caps = _sg_d3d11_dxgi_fmt_caps(_sg_d3d11_srv_pixel_format((sg_pixel_format)fmt));
        const UINT rtv_dxgi_fmt_caps = _sg_d3d11_dxgi_fmt_caps(_sg_d3d11_rtv_pixel_format((sg_pixel_format)fmt));
        const UINT dsv_dxgi_fmt_caps = _sg_d3d11_dxgi_fmt_caps(_sg_d3d11_dsv_pixel_format((sg_pixel_format)fmt));
        sg_pixelformat_info* info = &_sg.formats[fmt];
        const bool render = 0 != (rtv_dxgi_fmt_caps & D3D11_FORMAT_SUPPORT_RENDER_TARGET);
        const bool depth  = 0 != (dsv_dxgi_fmt_caps & D3D11_FORMAT_SUPPORT_DEPTH_STENCIL);
        info->sample = 0 != (srv_dxgi_fmt_caps & D3D11_FORMAT_SUPPORT_TEXTURE2D);
        info->filter = 0 != (srv_dxgi_fmt_caps & D3D11_FORMAT_SUPPORT_SHADER_SAMPLE);
        info->render = render || depth;
        info->blend  = 0 != (rtv_dxgi_fmt_caps & D3D11_FORMAT_SUPPORT_BLENDABLE);
        info->msaa   = 0 != (rtv_dxgi_fmt_caps & D3D11_FORMAT_SUPPORT_MULTISAMPLE_RENDERTARGET);
        info->depth  = depth;
    }
}

_SOKOL_PRIVATE void _sg_d3d11_setup_backend(const sg_desc* desc) {
    // assume _sg.d3d11 already is zero-initialized
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
    // clear all the device context state, so that resource refs don't keep stuck in the d3d device context
    _sg_d3d11_ClearState(_sg.d3d11.ctx);
}

_SOKOL_PRIVATE void _sg_d3d11_reset_state_cache(void) {
    // just clear the d3d11 device context state
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
    // empty
}

_SOKOL_PRIVATE sg_resource_state _sg_d3d11_create_buffer(_sg_buffer_t* buf, const sg_buffer_desc* desc) {
    SOKOL_ASSERT(buf && desc);
    SOKOL_ASSERT(!buf->d3d11.buf);
    const bool injected = (0 != desc->d3d11_buffer);
    if (injected) {
        buf->d3d11.buf = (ID3D11Buffer*) desc->d3d11_buffer;
        _sg_d3d11_AddRef(buf->d3d11.buf);
    } else {
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
            _SG_ERROR(D3D11_CREATE_BUFFER_FAILED);
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
                const int mip_width = _sg_miplevel_dim(img->cmn.width, mip_index);
                const int mip_height = _sg_miplevel_dim(img->cmn.height, mip_index);
                const sg_range* subimg_data = &(data->subimage[face_index][mip_index]);
                const size_t slice_size = subimg_data->size / (size_t)num_slices;
                const size_t slice_offset = slice_size * (size_t)slice_index;
                const uint8_t* ptr = (const uint8_t*) subimg_data->ptr;
                subres_data->pSysMem = ptr + slice_offset;
                subres_data->SysMemPitch = (UINT)_sg_row_pitch(img->cmn.pixel_format, mip_width, 1);
                if (img->cmn.type == SG_IMAGETYPE_3D) {
                    // FIXME? const int mip_depth = _sg_miplevel_dim(img->depth, mip_index);
                    subres_data->SysMemSlicePitch = (UINT)_sg_surface_pitch(img->cmn.pixel_format, mip_width, mip_height, 1);
                } else {
                    subres_data->SysMemSlicePitch = 0;
                }
            }
        }
    }
}

_SOKOL_PRIVATE sg_resource_state _sg_d3d11_create_image(_sg_image_t* img, const sg_image_desc* desc) {
    SOKOL_ASSERT(img && desc);
    SOKOL_ASSERT((0 == img->d3d11.tex2d) && (0 == img->d3d11.tex3d) && (0 == img->d3d11.res) && (0 == img->d3d11.srv));
    HRESULT hr;

    const bool injected = (0 != desc->d3d11_texture);
    const bool msaa = (img->cmn.sample_count > 1);
    img->d3d11.format = _sg_d3d11_texture_pixel_format(img->cmn.pixel_format);
    if (img->d3d11.format == DXGI_FORMAT_UNKNOWN) {
        _SG_ERROR(D3D11_CREATE_2D_TEXTURE_UNSUPPORTED_PIXEL_FORMAT);
        return SG_RESOURCESTATE_FAILED;
    }

    // prepare initial content pointers
    D3D11_SUBRESOURCE_DATA* init_data = 0;
    if (!injected && (img->cmn.usage == SG_USAGE_IMMUTABLE) && !img->cmn.render_target) {
        _sg_d3d11_fill_subres_data(img, &desc->data);
        init_data = _sg.d3d11.subres_data;
    }
    if (img->cmn.type != SG_IMAGETYPE_3D) {
        // 2D-, cube- or array-texture
        // first check for injected texture and/or resource view
        if (injected) {
            img->d3d11.tex2d = (ID3D11Texture2D*) desc->d3d11_texture;
            _sg_d3d11_AddRef(img->d3d11.tex2d);
            img->d3d11.srv = (ID3D11ShaderResourceView*) desc->d3d11_shader_resource_view;
            if (img->d3d11.srv) {
                _sg_d3d11_AddRef(img->d3d11.srv);
            }
        } else {
            // if not injected, create 2D texture
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
            d3d11_tex_desc.Format = img->d3d11.format;
            if (img->cmn.render_target) {
                d3d11_tex_desc.Usage = D3D11_USAGE_DEFAULT;
                if (_sg_is_depth_or_depth_stencil_format(img->cmn.pixel_format)) {
                    d3d11_tex_desc.BindFlags = D3D11_BIND_DEPTH_STENCIL;
                } else {
                    d3d11_tex_desc.BindFlags = D3D11_BIND_RENDER_TARGET;
                }
                if (!msaa) {
                    d3d11_tex_desc.BindFlags |= D3D11_BIND_SHADER_RESOURCE;
                }
                d3d11_tex_desc.CPUAccessFlags = 0;
            } else {
                d3d11_tex_desc.Usage = _sg_d3d11_usage(img->cmn.usage);
                d3d11_tex_desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
                d3d11_tex_desc.CPUAccessFlags = _sg_d3d11_cpu_access_flags(img->cmn.usage);
            }
            d3d11_tex_desc.SampleDesc.Count = (UINT)img->cmn.sample_count;
            d3d11_tex_desc.SampleDesc.Quality = (UINT) (msaa ? D3D11_STANDARD_MULTISAMPLE_PATTERN : 0);
            d3d11_tex_desc.MiscFlags = (img->cmn.type == SG_IMAGETYPE_CUBE) ? D3D11_RESOURCE_MISC_TEXTURECUBE : 0;

            hr = _sg_d3d11_CreateTexture2D(_sg.d3d11.dev, &d3d11_tex_desc, init_data, &img->d3d11.tex2d);
            if (!(SUCCEEDED(hr) && img->d3d11.tex2d)) {
                _SG_ERROR(D3D11_CREATE_2D_TEXTURE_FAILED);
                return SG_RESOURCESTATE_FAILED;
            }

            // create shader-resource-view for 2D texture
            // FIXME: currently we don't support setting MSAA texture as shader resource
            if (!msaa) {
                D3D11_SHADER_RESOURCE_VIEW_DESC d3d11_srv_desc;
                _sg_clear(&d3d11_srv_desc, sizeof(d3d11_srv_desc));
                d3d11_srv_desc.Format = _sg_d3d11_srv_pixel_format(img->cmn.pixel_format);
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
                    _SG_ERROR(D3D11_CREATE_2D_SRV_FAILED);
                    return SG_RESOURCESTATE_FAILED;
                }
            }
        }
        SOKOL_ASSERT(img->d3d11.tex2d);
        img->d3d11.res = (ID3D11Resource*)img->d3d11.tex2d;
        _sg_d3d11_AddRef(img->d3d11.res);
    } else {
        // 3D texture - same procedure, first check if injected, than create non-injected
        if (injected) {
            img->d3d11.tex3d = (ID3D11Texture3D*) desc->d3d11_texture;
            _sg_d3d11_AddRef(img->d3d11.tex3d);
            img->d3d11.srv = (ID3D11ShaderResourceView*) desc->d3d11_shader_resource_view;
            if (img->d3d11.srv) {
                _sg_d3d11_AddRef(img->d3d11.srv);
            }
        } else {
            // not injected, create 3d texture
            D3D11_TEXTURE3D_DESC d3d11_tex_desc;
            _sg_clear(&d3d11_tex_desc, sizeof(d3d11_tex_desc));
            d3d11_tex_desc.Width = (UINT)img->cmn.width;
            d3d11_tex_desc.Height = (UINT)img->cmn.height;
            d3d11_tex_desc.Depth = (UINT)img->cmn.num_slices;
            d3d11_tex_desc.MipLevels = (UINT)img->cmn.num_mipmaps;
            d3d11_tex_desc.Format = img->d3d11.format;
            if (img->cmn.render_target) {
                d3d11_tex_desc.Usage = D3D11_USAGE_DEFAULT;
                d3d11_tex_desc.BindFlags = D3D11_BIND_RENDER_TARGET;
                d3d11_tex_desc.CPUAccessFlags = 0;
            } else {
                d3d11_tex_desc.Usage = _sg_d3d11_usage(img->cmn.usage);
                d3d11_tex_desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
                d3d11_tex_desc.CPUAccessFlags = _sg_d3d11_cpu_access_flags(img->cmn.usage);
            }
            if (img->d3d11.format == DXGI_FORMAT_UNKNOWN) {
                _SG_ERROR(D3D11_CREATE_3D_TEXTURE_UNSUPPORTED_PIXEL_FORMAT);
                return SG_RESOURCESTATE_FAILED;
            }
            hr = _sg_d3d11_CreateTexture3D(_sg.d3d11.dev, &d3d11_tex_desc, init_data, &img->d3d11.tex3d);
            if (!(SUCCEEDED(hr) && img->d3d11.tex3d)) {
                _SG_ERROR(D3D11_CREATE_3D_TEXTURE_FAILED);
                return SG_RESOURCESTATE_FAILED;
            }

            // create shader-resource-view for 3D texture
            if (!msaa) {
                D3D11_SHADER_RESOURCE_VIEW_DESC d3d11_srv_desc;
                _sg_clear(&d3d11_srv_desc, sizeof(d3d11_srv_desc));
                d3d11_srv_desc.Format = _sg_d3d11_srv_pixel_format(img->cmn.pixel_format);
                d3d11_srv_desc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE3D;
                d3d11_srv_desc.Texture3D.MipLevels = (UINT)img->cmn.num_mipmaps;
                hr = _sg_d3d11_CreateShaderResourceView(_sg.d3d11.dev, (ID3D11Resource*)img->d3d11.tex3d, &d3d11_srv_desc, &img->d3d11.srv);
                if (!(SUCCEEDED(hr) && img->d3d11.srv)) {
                    _SG_ERROR(D3D11_CREATE_3D_SRV_FAILED);
                    return SG_RESOURCESTATE_FAILED;
                }
            }
        }
        SOKOL_ASSERT(img->d3d11.tex3d);
        img->d3d11.res = (ID3D11Resource*)img->d3d11.tex3d;
        _sg_d3d11_AddRef(img->d3d11.res);
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
    if (img->d3d11.res) {
        _sg_d3d11_Release(img->d3d11.res);
    }
    if (img->d3d11.srv) {
        _sg_d3d11_Release(img->d3d11.srv);
    }
}

_SOKOL_PRIVATE sg_resource_state _sg_d3d11_create_sampler(_sg_sampler_t* smp, const sg_sampler_desc* desc) {
    SOKOL_ASSERT(smp && desc);
    SOKOL_ASSERT(0 == smp->d3d11.smp);
    const bool injected = (0 != desc->d3d11_sampler);
    if (injected) {
        smp->d3d11.smp = (ID3D11SamplerState*)desc->d3d11_sampler;
        _sg_d3d11_AddRef(smp->d3d11.smp);
    } else {
        D3D11_SAMPLER_DESC d3d11_smp_desc;
        _sg_clear(&d3d11_smp_desc, sizeof(d3d11_smp_desc));
        d3d11_smp_desc.Filter = _sg_d3d11_filter(desc->min_filter, desc->mag_filter, desc->mipmap_filter, desc->compare != SG_COMPAREFUNC_NEVER, desc->max_anisotropy);
        d3d11_smp_desc.AddressU = _sg_d3d11_address_mode(desc->wrap_u);
        d3d11_smp_desc.AddressV = _sg_d3d11_address_mode(desc->wrap_v);
        d3d11_smp_desc.AddressW = _sg_d3d11_address_mode(desc->wrap_w);
        d3d11_smp_desc.MipLODBias = 0.0f; // FIXME?
        switch (desc->border_color) {
            case SG_BORDERCOLOR_TRANSPARENT_BLACK:
                // all 0.0f
                break;
            case SG_BORDERCOLOR_OPAQUE_WHITE:
                for (int i = 0; i < 4; i++) {
                    d3d11_smp_desc.BorderColor[i] = 1.0f;
                }
                break;
            default:
                // opaque black
                d3d11_smp_desc.BorderColor[3] = 1.0f;
                break;
        }
        d3d11_smp_desc.MaxAnisotropy = desc->max_anisotropy;
        d3d11_smp_desc.ComparisonFunc = _sg_d3d11_compare_func(desc->compare);
        d3d11_smp_desc.MinLOD = desc->min_lod;
        d3d11_smp_desc.MaxLOD = desc->max_lod;
        HRESULT hr = _sg_d3d11_CreateSamplerState(_sg.d3d11.dev, &d3d11_smp_desc, &smp->d3d11.smp);
        if (!(SUCCEEDED(hr) && smp->d3d11.smp)) {
            _SG_ERROR(D3D11_CREATE_SAMPLER_STATE_FAILED);
            return SG_RESOURCESTATE_FAILED;
        }
    }
    return SG_RESOURCESTATE_VALID;
}

_SOKOL_PRIVATE void _sg_d3d11_discard_sampler(_sg_sampler_t* smp) {
    SOKOL_ASSERT(smp);
    if (smp->d3d11.smp) {
        _sg_d3d11_Release(smp->d3d11.smp);
    }
}

_SOKOL_PRIVATE bool _sg_d3d11_load_d3dcompiler_dll(void) {
    if ((0 == _sg.d3d11.d3dcompiler_dll) && !_sg.d3d11.d3dcompiler_dll_load_failed) {
        _sg.d3d11.d3dcompiler_dll = LoadLibraryA("d3dcompiler_47.dll");
        if (0 == _sg.d3d11.d3dcompiler_dll) {
            // don't attempt to load missing DLL in the future
            _SG_ERROR(D3D11_LOAD_D3DCOMPILER_47_DLL_FAILED);
            _sg.d3d11.d3dcompiler_dll_load_failed = true;
            return false;
        }
        // look up function pointers
        _sg.d3d11.D3DCompile_func = (pD3DCompile)(void*) GetProcAddress(_sg.d3d11.d3dcompiler_dll, "D3DCompile");
        SOKOL_ASSERT(_sg.d3d11.D3DCompile_func);
    }
    return 0 != _sg.d3d11.d3dcompiler_dll;
}

_SOKOL_PRIVATE ID3DBlob* _sg_d3d11_compile_shader(const sg_shader_stage_desc* stage_desc) {
    if (!_sg_d3d11_load_d3dcompiler_dll()) {
        return NULL;
    }
    SOKOL_ASSERT(stage_desc->d3d11_target);
    ID3DBlob* output = NULL;
    ID3DBlob* errors_or_warnings = NULL;
    HRESULT hr = _sg.d3d11.D3DCompile_func(
        stage_desc->source,             // pSrcData
        strlen(stage_desc->source),     // SrcDataSize
        NULL,                           // pSourceName
        NULL,                           // pDefines
        NULL,                           // pInclude
        stage_desc->entry ? stage_desc->entry : "main",     // pEntryPoint
        stage_desc->d3d11_target,       // pTarget
        D3DCOMPILE_PACK_MATRIX_COLUMN_MAJOR | D3DCOMPILE_OPTIMIZATION_LEVEL3,   // Flags1
        0,          // Flags2
        &output,    // ppCode
        &errors_or_warnings);   // ppErrorMsgs
    if (FAILED(hr)) {
        _SG_ERROR(D3D11_SHADER_COMPILATION_FAILED);
    }
    if (errors_or_warnings) {
        _SG_WARN(D3D11_SHADER_COMPILATION_OUTPUT);
        _SG_LOGMSG(D3D11_SHADER_COMPILATION_OUTPUT, (LPCSTR)_sg_d3d11_GetBufferPointer(errors_or_warnings));
        _sg_d3d11_Release(errors_or_warnings); errors_or_warnings = NULL;
    }
    if (FAILED(hr)) {
        // just in case, usually output is NULL here
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

    // copy vertex attribute semantic names and indices
    for (int i = 0; i < SG_MAX_VERTEX_ATTRIBUTES; i++) {
        _sg_strcpy(&shd->d3d11.attrs[i].sem_name, desc->attrs[i].sem_name);
        shd->d3d11.attrs[i].sem_index = desc->attrs[i].sem_index;
    }

    // shader stage uniform blocks and image slots
    for (int stage_index = 0; stage_index < SG_NUM_SHADER_STAGES; stage_index++) {
        _sg_shader_stage_t* cmn_stage = &shd->cmn.stage[stage_index];
        _sg_d3d11_shader_stage_t* d3d11_stage = &shd->d3d11.stage[stage_index];
        for (int ub_index = 0; ub_index < cmn_stage->num_uniform_blocks; ub_index++) {
            const _sg_shader_uniform_block_t* ub = &cmn_stage->uniform_blocks[ub_index];

            // create a D3D constant buffer for each uniform block
            SOKOL_ASSERT(0 == d3d11_stage->cbufs[ub_index]);
            D3D11_BUFFER_DESC cb_desc;
            _sg_clear(&cb_desc, sizeof(cb_desc));
            cb_desc.ByteWidth = (UINT)_sg_roundup((int)ub->size, 16);
            cb_desc.Usage = D3D11_USAGE_DEFAULT;
            cb_desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
            hr = _sg_d3d11_CreateBuffer(_sg.d3d11.dev, &cb_desc, NULL, &d3d11_stage->cbufs[ub_index]);
            if (!(SUCCEEDED(hr) && d3d11_stage->cbufs[ub_index])) {
                _SG_ERROR(D3D11_CREATE_CONSTANT_BUFFER_FAILED);
                return SG_RESOURCESTATE_FAILED;
            }
        }
    }

    const void* vs_ptr = 0, *fs_ptr = 0;
    SIZE_T vs_length = 0, fs_length = 0;
    ID3DBlob* vs_blob = 0, *fs_blob = 0;
    if (desc->vs.bytecode.ptr && desc->fs.bytecode.ptr) {
        // create from shader byte code
        vs_ptr = desc->vs.bytecode.ptr;
        fs_ptr = desc->fs.bytecode.ptr;
        vs_length = desc->vs.bytecode.size;
        fs_length = desc->fs.bytecode.size;
    } else {
        // compile from shader source code
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
        // create the D3D vertex- and pixel-shader objects
        hr = _sg_d3d11_CreateVertexShader(_sg.d3d11.dev, vs_ptr, vs_length, NULL, &shd->d3d11.vs);
        bool vs_succeeded = SUCCEEDED(hr) && shd->d3d11.vs;
        hr = _sg_d3d11_CreatePixelShader(_sg.d3d11.dev, fs_ptr, fs_length, NULL, &shd->d3d11.fs);
        bool fs_succeeded = SUCCEEDED(hr) && shd->d3d11.fs;

        // need to store the vertex shader byte code, this is needed later in sg_create_pipeline
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
    pip->d3d11.index_format = _sg_d3d11_index_format(pip->cmn.index_type);
    pip->d3d11.topology = _sg_d3d11_primitive_topology(desc->primitive_type);
    pip->d3d11.stencil_ref = desc->stencil.ref;

    // create input layout object
    HRESULT hr;
    D3D11_INPUT_ELEMENT_DESC d3d11_comps[SG_MAX_VERTEX_ATTRIBUTES];
    _sg_clear(d3d11_comps, sizeof(d3d11_comps));
    int attr_index = 0;
    for (; attr_index < SG_MAX_VERTEX_ATTRIBUTES; attr_index++) {
        const sg_vertex_attr_state* a_state = &desc->layout.attrs[attr_index];
        if (a_state->format == SG_VERTEXFORMAT_INVALID) {
            break;
        }
        SOKOL_ASSERT(a_state->buffer_index < SG_MAX_VERTEX_BUFFERS);
        const sg_vertex_buffer_layout_state* l_state = &desc->layout.buffers[a_state->buffer_index];
        const sg_vertex_step step_func = l_state->step_func;
        const int step_rate = l_state->step_rate;
        D3D11_INPUT_ELEMENT_DESC* d3d11_comp = &d3d11_comps[attr_index];
        d3d11_comp->SemanticName = _sg_strptr(&shd->d3d11.attrs[attr_index].sem_name);
        d3d11_comp->SemanticIndex = (UINT)shd->d3d11.attrs[attr_index].sem_index;
        d3d11_comp->Format = _sg_d3d11_vertex_format(a_state->format);
        d3d11_comp->InputSlot = (UINT)a_state->buffer_index;
        d3d11_comp->AlignedByteOffset = (UINT)a_state->offset;
        d3d11_comp->InputSlotClass = _sg_d3d11_input_classification(step_func);
        if (SG_VERTEXSTEP_PER_INSTANCE == step_func) {
            d3d11_comp->InstanceDataStepRate = (UINT)step_rate;
            pip->cmn.use_instanced_draw = true;
        }
        pip->cmn.vertex_buffer_layout_active[a_state->buffer_index] = true;
    }
    for (int layout_index = 0; layout_index < SG_MAX_VERTEX_BUFFERS; layout_index++) {
        if (pip->cmn.vertex_buffer_layout_active[layout_index]) {
            const sg_vertex_buffer_layout_state* l_state = &desc->layout.buffers[layout_index];
            SOKOL_ASSERT(l_state->stride > 0);
            pip->d3d11.vb_strides[layout_index] = (UINT)l_state->stride;
        } else {
            pip->d3d11.vb_strides[layout_index] = 0;
        }
    }
    hr = _sg_d3d11_CreateInputLayout(_sg.d3d11.dev,
        d3d11_comps,                // pInputElementDesc
        (UINT)attr_index,           // NumElements
        shd->d3d11.vs_blob,         // pShaderByteCodeWithInputSignature
        shd->d3d11.vs_blob_length,  // BytecodeLength
        &pip->d3d11.il);
    if (!(SUCCEEDED(hr) && pip->d3d11.il)) {
        _SG_ERROR(D3D11_CREATE_INPUT_LAYOUT_FAILED);
        return SG_RESOURCESTATE_FAILED;
    }

    // create rasterizer state
    D3D11_RASTERIZER_DESC rs_desc;
    _sg_clear(&rs_desc, sizeof(rs_desc));
    rs_desc.FillMode = D3D11_FILL_SOLID;
    rs_desc.CullMode = _sg_d3d11_cull_mode(desc->cull_mode);
    rs_desc.FrontCounterClockwise = desc->face_winding == SG_FACEWINDING_CCW;
    rs_desc.DepthBias = (INT) pip->cmn.depth.bias;
    rs_desc.DepthBiasClamp = pip->cmn.depth.bias_clamp;
    rs_desc.SlopeScaledDepthBias = pip->cmn.depth.bias_slope_scale;
    rs_desc.DepthClipEnable = TRUE;
    rs_desc.ScissorEnable = TRUE;
    rs_desc.MultisampleEnable = desc->sample_count > 1;
    rs_desc.AntialiasedLineEnable = FALSE;
    hr = _sg_d3d11_CreateRasterizerState(_sg.d3d11.dev, &rs_desc, &pip->d3d11.rs);
    if (!(SUCCEEDED(hr) && pip->d3d11.rs)) {
        _SG_ERROR(D3D11_CREATE_RASTERIZER_STATE_FAILED);
        return SG_RESOURCESTATE_FAILED;
    }

    // create depth-stencil state
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
        _SG_ERROR(D3D11_CREATE_DEPTH_STENCIL_STATE_FAILED);
        return SG_RESOURCESTATE_FAILED;
    }

    // create blend state
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
        _SG_ERROR(D3D11_CREATE_BLEND_STATE_FAILED);
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

_SOKOL_PRIVATE sg_resource_state _sg_d3d11_create_pass(_sg_pass_t* pass, _sg_image_t** color_images, _sg_image_t** resolve_images, _sg_image_t* ds_img, const sg_pass_desc* desc) {
    SOKOL_ASSERT(pass && desc);
    SOKOL_ASSERT(color_images && resolve_images);
    SOKOL_ASSERT(_sg.d3d11.dev);

    // copy image pointers
    for (int i = 0; i < pass->cmn.num_color_atts; i++) {
        const sg_pass_attachment_desc* color_desc = &desc->color_attachments[i];
        _SOKOL_UNUSED(color_desc);
        SOKOL_ASSERT(color_desc->image.id != SG_INVALID_ID);
        SOKOL_ASSERT(0 == pass->d3d11.color_atts[i].image);
        SOKOL_ASSERT(color_images[i] && (color_images[i]->slot.id == color_desc->image.id));
        SOKOL_ASSERT(_sg_is_valid_rendertarget_color_format(color_images[i]->cmn.pixel_format));
        pass->d3d11.color_atts[i].image = color_images[i];

        const sg_pass_attachment_desc* resolve_desc = &desc->resolve_attachments[i];
        if (resolve_desc->image.id != SG_INVALID_ID) {
            SOKOL_ASSERT(0 == pass->d3d11.resolve_atts[i].image);
            SOKOL_ASSERT(resolve_images[i] && (resolve_images[i]->slot.id == resolve_desc->image.id));
            SOKOL_ASSERT(color_images[i] && (color_images[i]->cmn.pixel_format == resolve_images[i]->cmn.pixel_format));
            pass->d3d11.resolve_atts[i].image = resolve_images[i];
        }
    }
    SOKOL_ASSERT(0 == pass->d3d11.ds_att.image);
    const sg_pass_attachment_desc* ds_desc = &desc->depth_stencil_attachment;
    if (ds_desc->image.id != SG_INVALID_ID) {
        SOKOL_ASSERT(ds_img && (ds_img->slot.id == ds_desc->image.id));
        SOKOL_ASSERT(_sg_is_valid_rendertarget_depth_format(ds_img->cmn.pixel_format));
        pass->d3d11.ds_att.image = ds_img;
    }

    // create render-target views
    for (int i = 0; i < pass->cmn.num_color_atts; i++) {
        const _sg_pass_attachment_t* cmn_color_att = &pass->cmn.color_atts[i];
        const _sg_image_t* color_img = color_images[i];
        SOKOL_ASSERT(0 == pass->d3d11.color_atts[i].view.rtv);
        const bool msaa = color_img->cmn.sample_count > 1;
        D3D11_RENDER_TARGET_VIEW_DESC d3d11_rtv_desc;
        _sg_clear(&d3d11_rtv_desc, sizeof(d3d11_rtv_desc));
        d3d11_rtv_desc.Format = _sg_d3d11_rtv_pixel_format(color_img->cmn.pixel_format);
        if (color_img->cmn.type == SG_IMAGETYPE_2D) {
            if (msaa) {
                d3d11_rtv_desc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2DMS;
            } else {
                d3d11_rtv_desc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;
                d3d11_rtv_desc.Texture2D.MipSlice = (UINT)cmn_color_att->mip_level;
            }
        } else if ((color_img->cmn.type == SG_IMAGETYPE_CUBE) || (color_img->cmn.type == SG_IMAGETYPE_ARRAY)) {
            if (msaa) {
                d3d11_rtv_desc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2DMSARRAY;
                d3d11_rtv_desc.Texture2DMSArray.FirstArraySlice = (UINT)cmn_color_att->slice;
                d3d11_rtv_desc.Texture2DMSArray.ArraySize = 1;
            } else {
                d3d11_rtv_desc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2DARRAY;
                d3d11_rtv_desc.Texture2DArray.MipSlice = (UINT)cmn_color_att->mip_level;
                d3d11_rtv_desc.Texture2DArray.FirstArraySlice = (UINT)cmn_color_att->slice;
                d3d11_rtv_desc.Texture2DArray.ArraySize = 1;
            }
        } else {
            SOKOL_ASSERT(color_img->cmn.type == SG_IMAGETYPE_3D);
            SOKOL_ASSERT(!msaa);
            d3d11_rtv_desc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE3D;
            d3d11_rtv_desc.Texture3D.MipSlice = (UINT)cmn_color_att->mip_level;
            d3d11_rtv_desc.Texture3D.FirstWSlice = (UINT)cmn_color_att->slice;
            d3d11_rtv_desc.Texture3D.WSize = 1;
        }
        SOKOL_ASSERT(color_img->d3d11.res);
        HRESULT hr = _sg_d3d11_CreateRenderTargetView(_sg.d3d11.dev, color_img->d3d11.res, &d3d11_rtv_desc, &pass->d3d11.color_atts[i].view.rtv);
        if (!(SUCCEEDED(hr) && pass->d3d11.color_atts[i].view.rtv)) {
            _SG_ERROR(D3D11_CREATE_RTV_FAILED);
            return SG_RESOURCESTATE_FAILED;
        }
    }
    SOKOL_ASSERT(0 == pass->d3d11.ds_att.view.dsv);
    if (ds_desc->image.id != SG_INVALID_ID) {
        const _sg_pass_attachment_t* cmn_ds_att = &pass->cmn.ds_att;
        const bool msaa = ds_img->cmn.sample_count > 1;
        D3D11_DEPTH_STENCIL_VIEW_DESC d3d11_dsv_desc;
        _sg_clear(&d3d11_dsv_desc, sizeof(d3d11_dsv_desc));
        d3d11_dsv_desc.Format = _sg_d3d11_dsv_pixel_format(ds_img->cmn.pixel_format);
        SOKOL_ASSERT(ds_img && ds_img->cmn.type != SG_IMAGETYPE_3D);
        if (ds_img->cmn.type == SG_IMAGETYPE_2D) {
            if (msaa) {
                d3d11_dsv_desc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2DMS;
            } else {
                d3d11_dsv_desc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
                d3d11_dsv_desc.Texture2D.MipSlice = (UINT)cmn_ds_att->mip_level;
            }
        } else if ((ds_img->cmn.type == SG_IMAGETYPE_CUBE) || (ds_img->cmn.type == SG_IMAGETYPE_ARRAY)) {
            if (msaa) {
                d3d11_dsv_desc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2DMSARRAY;
                d3d11_dsv_desc.Texture2DMSArray.FirstArraySlice = (UINT)cmn_ds_att->slice;
                d3d11_dsv_desc.Texture2DMSArray.ArraySize = 1;
            } else {
                d3d11_dsv_desc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2DARRAY;
                d3d11_dsv_desc.Texture2DArray.MipSlice = (UINT)cmn_ds_att->mip_level;
                d3d11_dsv_desc.Texture2DArray.FirstArraySlice = (UINT)cmn_ds_att->slice;
                d3d11_dsv_desc.Texture2DArray.ArraySize = 1;
            }
        }
        SOKOL_ASSERT(ds_img->d3d11.res);
        HRESULT hr = _sg_d3d11_CreateDepthStencilView(_sg.d3d11.dev, ds_img->d3d11.res, &d3d11_dsv_desc, &pass->d3d11.ds_att.view.dsv);
        if (!(SUCCEEDED(hr) && pass->d3d11.ds_att.view.dsv)) {
            _SG_ERROR(D3D11_CREATE_DSV_FAILED);
            return SG_RESOURCESTATE_FAILED;
        }
    }
    return SG_RESOURCESTATE_VALID;
}

_SOKOL_PRIVATE void _sg_d3d11_discard_pass(_sg_pass_t* pass) {
    SOKOL_ASSERT(pass);
    SOKOL_ASSERT(pass != _sg.d3d11.cur_pass);
    for (int i = 0; i < SG_MAX_COLOR_ATTACHMENTS; i++) {
        if (pass->d3d11.color_atts[i].view.rtv) {
            _sg_d3d11_Release(pass->d3d11.color_atts[i].view.rtv);
        }
        if (pass->d3d11.resolve_atts[i].view.rtv) {
            _sg_d3d11_Release(pass->d3d11.resolve_atts[i].view.rtv);
        }
    }
    if (pass->d3d11.ds_att.view.dsv) {
        _sg_d3d11_Release(pass->d3d11.ds_att.view.dsv);
    }
}

_SOKOL_PRIVATE _sg_image_t* _sg_d3d11_pass_color_image(const _sg_pass_t* pass, int index) {
    SOKOL_ASSERT(pass && (index >= 0) && (index < SG_MAX_COLOR_ATTACHMENTS));
    return pass->d3d11.color_atts[index].image;
}

_SOKOL_PRIVATE _sg_image_t* _sg_d3d11_pass_resolve_image(const _sg_pass_t* pass, int index) {
    SOKOL_ASSERT(pass && (index >= 0) && (index < SG_MAX_COLOR_ATTACHMENTS));
    return pass->d3d11.resolve_atts[index].image;
}

_SOKOL_PRIVATE _sg_image_t* _sg_d3d11_pass_ds_image(const _sg_pass_t* pass) {
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
            _sg.d3d11.cur_rtvs[i] = pass->d3d11.color_atts[i].view.rtv;
            if (_sg.d3d11.cur_rtvs[i]) {
                _sg.d3d11.num_rtvs++;
            }
        }
        _sg.d3d11.cur_dsv = pass->d3d11.ds_att.view.dsv;
    } else {
        // render to default frame buffer
        _sg.d3d11.cur_pass = 0;
        _sg.d3d11.cur_pass_id.id = SG_INVALID_ID;
        _sg.d3d11.num_rtvs = 1;
        if (_sg.d3d11.rtv_cb) {
            _sg.d3d11.cur_rtvs[0] = (ID3D11RenderTargetView*) _sg.d3d11.rtv_cb();
        } else {
            _sg.d3d11.cur_rtvs[0] = (ID3D11RenderTargetView*) _sg.d3d11.rtv_userdata_cb(_sg.d3d11.user_data);
        }
        for (int i = 1; i < SG_MAX_COLOR_ATTACHMENTS; i++) {
            _sg.d3d11.cur_rtvs[i] = 0;
        }
        if (_sg.d3d11.dsv_cb) {
            _sg.d3d11.cur_dsv = (ID3D11DepthStencilView*) _sg.d3d11.dsv_cb();
        } else {
            _sg.d3d11.cur_dsv = (ID3D11DepthStencilView*) _sg.d3d11.dsv_userdata_cb(_sg.d3d11.user_data);
        }
        SOKOL_ASSERT(_sg.d3d11.cur_rtvs[0] && _sg.d3d11.cur_dsv);
    }
    // apply the render-target- and depth-stencil-views
    _sg_d3d11_OMSetRenderTargets(_sg.d3d11.ctx, SG_MAX_COLOR_ATTACHMENTS, _sg.d3d11.cur_rtvs, _sg.d3d11.cur_dsv);
    _sg_stats_add(d3d11.pass.num_om_set_render_targets, 1);

    // set viewport and scissor rect to cover whole screen
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

    // perform clear action
    for (int i = 0; i < _sg.d3d11.num_rtvs; i++) {
        if (action->colors[i].load_action == SG_LOADACTION_CLEAR) {
            _sg_d3d11_ClearRenderTargetView(_sg.d3d11.ctx, _sg.d3d11.cur_rtvs[i], &action->colors[i].clear_value.r);
            _sg_stats_add(d3d11.pass.num_clear_render_target_view, 1);
        }
    }
    UINT ds_flags = 0;
    if (action->depth.load_action == SG_LOADACTION_CLEAR) {
        ds_flags |= D3D11_CLEAR_DEPTH;
    }
    if (action->stencil.load_action == SG_LOADACTION_CLEAR) {
        ds_flags |= D3D11_CLEAR_STENCIL;
    }
    if ((0 != ds_flags) && _sg.d3d11.cur_dsv) {
        _sg_d3d11_ClearDepthStencilView(_sg.d3d11.ctx, _sg.d3d11.cur_dsv, ds_flags, action->depth.clear_value, action->stencil.clear_value);
        _sg_stats_add(d3d11.pass.num_clear_depth_stencil_view, 1);
    }
}

// D3D11CalcSubresource only exists for C++
_SOKOL_PRIVATE UINT _sg_d3d11_calcsubresource(UINT mip_slice, UINT array_slice, UINT mip_levels) {
    return mip_slice + array_slice * mip_levels;
}

_SOKOL_PRIVATE void _sg_d3d11_end_pass(void) {
    SOKOL_ASSERT(_sg.d3d11.in_pass && _sg.d3d11.ctx);
    _sg.d3d11.in_pass = false;

    // need to resolve MSAA render attachments into texture?
    if (_sg.d3d11.cur_pass) {
        SOKOL_ASSERT(_sg.d3d11.cur_pass->slot.id == _sg.d3d11.cur_pass_id.id);
        for (int i = 0; i < _sg.d3d11.num_rtvs; i++) {
            const _sg_image_t* resolve_img = _sg.d3d11.cur_pass->d3d11.resolve_atts[i].image;
            if (resolve_img) {
                const _sg_image_t* color_img = _sg.d3d11.cur_pass->d3d11.color_atts[i].image;
                const _sg_pass_attachment_t* cmn_color_att = &_sg.d3d11.cur_pass->cmn.color_atts[i];
                const _sg_pass_attachment_t* cmn_resolve_att = &_sg.d3d11.cur_pass->cmn.resolve_atts[i];
                SOKOL_ASSERT(resolve_img->slot.id == cmn_resolve_att->image_id.id);
                SOKOL_ASSERT(color_img && (color_img->slot.id == cmn_color_att->image_id.id));
                SOKOL_ASSERT(color_img->cmn.sample_count > 1);
                SOKOL_ASSERT(resolve_img->cmn.sample_count == 1);
                const UINT src_subres = _sg_d3d11_calcsubresource(
                    (UINT)cmn_color_att->mip_level,
                    (UINT)cmn_color_att->slice,
                    (UINT)color_img->cmn.num_mipmaps);
                const UINT dst_subres = _sg_d3d11_calcsubresource(
                    (UINT)cmn_resolve_att->mip_level,
                    (UINT)cmn_resolve_att->slice,
                    (UINT)resolve_img->cmn.num_mipmaps);
                _sg_d3d11_ResolveSubresource(_sg.d3d11.ctx,
                    resolve_img->d3d11.res,
                    dst_subres,
                    color_img->d3d11.res,
                    src_subres,
                    color_img->d3d11.format);
                _sg_stats_add(d3d11.pass.num_resolve_subresource, 1);
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
    _sg_stats_add(d3d11.pipeline.num_rs_set_state, 1);
    _sg_stats_add(d3d11.pipeline.num_om_set_depth_stencil_state, 1);
    _sg_stats_add(d3d11.pipeline.num_om_set_blend_state, 1);
    _sg_stats_add(d3d11.pipeline.num_ia_set_primitive_topology, 1);
    _sg_stats_add(d3d11.pipeline.num_ia_set_input_layout, 1);
    _sg_stats_add(d3d11.pipeline.num_vs_set_shader, 1);
    _sg_stats_add(d3d11.pipeline.num_vs_set_constant_buffers, 1);
    _sg_stats_add(d3d11.pipeline.num_ps_set_shader, 1);
    _sg_stats_add(d3d11.pipeline.num_ps_set_constant_buffers, 1);
}

_SOKOL_PRIVATE bool _sg_d3d11_apply_bindings(_sg_bindings_t* bnd) {
    SOKOL_ASSERT(bnd);
    SOKOL_ASSERT(bnd->pip);
    SOKOL_ASSERT(_sg.d3d11.ctx);
    SOKOL_ASSERT(_sg.d3d11.in_pass);

    // gather all the D3D11 resources into arrays
    ID3D11Buffer* d3d11_ib = bnd->ib ? bnd->ib->d3d11.buf : 0;
    ID3D11Buffer* d3d11_vbs[SG_MAX_VERTEX_BUFFERS] = {0};
    UINT d3d11_vb_offsets[SG_MAX_VERTEX_BUFFERS] = {0};
    ID3D11ShaderResourceView* d3d11_vs_srvs[SG_MAX_SHADERSTAGE_IMAGES] = {0};
    ID3D11ShaderResourceView* d3d11_fs_srvs[SG_MAX_SHADERSTAGE_IMAGES] = {0};
    ID3D11SamplerState* d3d11_vs_smps[SG_MAX_SHADERSTAGE_SAMPLERS] = {0};
    ID3D11SamplerState* d3d11_fs_smps[SG_MAX_SHADERSTAGE_SAMPLERS] = {0};
    for (int i = 0; i < bnd->num_vbs; i++) {
        SOKOL_ASSERT(bnd->vbs[i]->d3d11.buf);
        d3d11_vbs[i] = bnd->vbs[i]->d3d11.buf;
        d3d11_vb_offsets[i] = (UINT)bnd->vb_offsets[i];
    }
    for (int i = 0; i < bnd->num_vs_imgs; i++) {
        SOKOL_ASSERT(bnd->vs_imgs[i]->d3d11.srv);
        d3d11_vs_srvs[i] = bnd->vs_imgs[i]->d3d11.srv;
    }
    for (int i = 0; i < bnd->num_fs_imgs; i++) {
        SOKOL_ASSERT(bnd->fs_imgs[i]->d3d11.srv);
        d3d11_fs_srvs[i] = bnd->fs_imgs[i]->d3d11.srv;
    }
    for (int i = 0; i < bnd->num_vs_smps; i++) {
        SOKOL_ASSERT(bnd->vs_smps[i]->d3d11.smp);
        d3d11_vs_smps[i] = bnd->vs_smps[i]->d3d11.smp;
    }
    for (int i = 0; i < bnd->num_fs_smps; i++) {
        SOKOL_ASSERT(bnd->fs_smps[i]->d3d11.smp);
        d3d11_fs_smps[i] = bnd->fs_smps[i]->d3d11.smp;
    }
    _sg_d3d11_IASetVertexBuffers(_sg.d3d11.ctx, 0, SG_MAX_VERTEX_BUFFERS, d3d11_vbs, bnd->pip->d3d11.vb_strides, d3d11_vb_offsets);
    _sg_d3d11_IASetIndexBuffer(_sg.d3d11.ctx, d3d11_ib, bnd->pip->d3d11.index_format, (UINT)bnd->ib_offset);
    _sg_d3d11_VSSetShaderResources(_sg.d3d11.ctx, 0, SG_MAX_SHADERSTAGE_IMAGES, d3d11_vs_srvs);
    _sg_d3d11_PSSetShaderResources(_sg.d3d11.ctx, 0, SG_MAX_SHADERSTAGE_IMAGES, d3d11_fs_srvs);
    _sg_d3d11_VSSetSamplers(_sg.d3d11.ctx, 0, SG_MAX_SHADERSTAGE_SAMPLERS, d3d11_vs_smps);
    _sg_d3d11_PSSetSamplers(_sg.d3d11.ctx, 0, SG_MAX_SHADERSTAGE_SAMPLERS, d3d11_fs_smps);
    _sg_stats_add(d3d11.bindings.num_ia_set_vertex_buffers, 1);
    _sg_stats_add(d3d11.bindings.num_ia_set_index_buffer, 1);
    _sg_stats_add(d3d11.bindings.num_vs_set_shader_resources, 1);
    _sg_stats_add(d3d11.bindings.num_ps_set_shader_resources, 1);
    _sg_stats_add(d3d11.bindings.num_vs_set_samplers, 1);
    _sg_stats_add(d3d11.bindings.num_ps_set_samplers, 1);
    return true;
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
    _sg_stats_add(d3d11.uniforms.num_update_subresource, 1);
}

_SOKOL_PRIVATE void _sg_d3d11_draw(int base_element, int num_elements, int num_instances) {
    SOKOL_ASSERT(_sg.d3d11.in_pass);
    if (_sg.d3d11.use_indexed_draw) {
        if (_sg.d3d11.use_instanced_draw) {
            _sg_d3d11_DrawIndexedInstanced(_sg.d3d11.ctx, (UINT)num_elements, (UINT)num_instances, (UINT)base_element, 0, 0);
            _sg_stats_add(d3d11.draw.num_draw_indexed_instanced, 1);
        } else {
            _sg_d3d11_DrawIndexed(_sg.d3d11.ctx, (UINT)num_elements, (UINT)base_element, 0);
            _sg_stats_add(d3d11.draw.num_draw_indexed, 1);
        }
    } else {
        if (_sg.d3d11.use_instanced_draw) {
            _sg_d3d11_DrawInstanced(_sg.d3d11.ctx, (UINT)num_elements, (UINT)num_instances, (UINT)base_element, 0);
            _sg_stats_add(d3d11.draw.num_draw_instanced, 1);
        } else {
            _sg_d3d11_Draw(_sg.d3d11.ctx, (UINT)num_elements, (UINT)base_element);
            _sg_stats_add(d3d11.draw.num_draw, 1);
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
    _sg_stats_add(d3d11.num_map, 1);
    if (SUCCEEDED(hr)) {
        memcpy(d3d11_msr.pData, data->ptr, data->size);
        _sg_d3d11_Unmap(_sg.d3d11.ctx, (ID3D11Resource*)buf->d3d11.buf, 0);
        _sg_stats_add(d3d11.num_unmap, 1);
    } else {
        _SG_ERROR(D3D11_MAP_FOR_UPDATE_BUFFER_FAILED);
    }
}

_SOKOL_PRIVATE void _sg_d3d11_append_buffer(_sg_buffer_t* buf, const sg_range* data, bool new_frame) {
    SOKOL_ASSERT(buf && data && data->ptr && (data->size > 0));
    SOKOL_ASSERT(_sg.d3d11.ctx);
    SOKOL_ASSERT(buf->d3d11.buf);
    D3D11_MAP map_type = new_frame ? D3D11_MAP_WRITE_DISCARD : D3D11_MAP_WRITE_NO_OVERWRITE;
    D3D11_MAPPED_SUBRESOURCE d3d11_msr;
    HRESULT hr = _sg_d3d11_Map(_sg.d3d11.ctx, (ID3D11Resource*)buf->d3d11.buf, 0, map_type, 0, &d3d11_msr);
    _sg_stats_add(d3d11.num_map, 1);
    if (SUCCEEDED(hr)) {
        uint8_t* dst_ptr = (uint8_t*)d3d11_msr.pData + buf->cmn.append_pos;
        memcpy(dst_ptr, data->ptr, data->size);
        _sg_d3d11_Unmap(_sg.d3d11.ctx, (ID3D11Resource*)buf->d3d11.buf, 0);
        _sg_stats_add(d3d11.num_unmap, 1);
    } else {
        _SG_ERROR(D3D11_MAP_FOR_APPEND_BUFFER_FAILED);
    }
}

_SOKOL_PRIVATE void _sg_d3d11_update_image(_sg_image_t* img, const sg_image_data* data) {
    SOKOL_ASSERT(img && data);
    SOKOL_ASSERT(_sg.d3d11.ctx);
    SOKOL_ASSERT(img->d3d11.res);
    const int num_faces = (img->cmn.type == SG_IMAGETYPE_CUBE) ? 6:1;
    const int num_slices = (img->cmn.type == SG_IMAGETYPE_ARRAY) ? img->cmn.num_slices:1;
    UINT subres_index = 0;
    HRESULT hr;
    D3D11_MAPPED_SUBRESOURCE d3d11_msr;
    for (int face_index = 0; face_index < num_faces; face_index++) {
        for (int slice_index = 0; slice_index < num_slices; slice_index++) {
            for (int mip_index = 0; mip_index < img->cmn.num_mipmaps; mip_index++, subres_index++) {
                SOKOL_ASSERT(subres_index < (SG_MAX_MIPMAPS * SG_MAX_TEXTUREARRAY_LAYERS));
                const int mip_width = _sg_miplevel_dim(img->cmn.width, mip_index);
                const int mip_height = _sg_miplevel_dim(img->cmn.height, mip_index);
                const int src_pitch = _sg_row_pitch(img->cmn.pixel_format, mip_width, 1);
                const sg_range* subimg_data = &(data->subimage[face_index][mip_index]);
                const size_t slice_size = subimg_data->size / (size_t)num_slices;
                const size_t slice_offset = slice_size * (size_t)slice_index;
                const uint8_t* slice_ptr = ((const uint8_t*)subimg_data->ptr) + slice_offset;
                hr = _sg_d3d11_Map(_sg.d3d11.ctx, img->d3d11.res, subres_index, D3D11_MAP_WRITE_DISCARD, 0, &d3d11_msr);
                _sg_stats_add(d3d11.num_map, 1);
                if (SUCCEEDED(hr)) {
                    // FIXME: need to handle difference in depth-pitch for 3D textures as well!
                    if (src_pitch == (int)d3d11_msr.RowPitch) {
                        memcpy(d3d11_msr.pData, slice_ptr, slice_size);
                    } else {
                        SOKOL_ASSERT(src_pitch < (int)d3d11_msr.RowPitch);
                        const uint8_t* src_ptr = slice_ptr;
                        uint8_t* dst_ptr = (uint8_t*) d3d11_msr.pData;
                        for (int row_index = 0; row_index < mip_height; row_index++) {
                            memcpy(dst_ptr, src_ptr, (size_t)src_pitch);
                            src_ptr += src_pitch;
                            dst_ptr += d3d11_msr.RowPitch;
                        }
                    }
                    _sg_d3d11_Unmap(_sg.d3d11.ctx, img->d3d11.res, subres_index);
                    _sg_stats_add(d3d11.num_unmap, 1);
                } else {
                    _SG_ERROR(D3D11_MAP_FOR_UPDATE_IMAGE_FAILED);
                }
            }
        }
    }
}

// ███    ███ ███████ ████████  █████  ██          ██████   █████   ██████ ██   ██ ███████ ███    ██ ██████
// ████  ████ ██         ██    ██   ██ ██          ██   ██ ██   ██ ██      ██  ██  ██      ████   ██ ██   ██
// ██ ████ ██ █████      ██    ███████ ██          ██████  ███████ ██      █████   █████   ██ ██  ██ ██   ██
// ██  ██  ██ ██         ██    ██   ██ ██          ██   ██ ██   ██ ██      ██  ██  ██      ██  ██ ██ ██   ██
// ██      ██ ███████    ██    ██   ██ ███████     ██████  ██   ██  ██████ ██   ██ ███████ ██   ████ ██████
//
// >>metal backend
#elif defined(SOKOL_METAL)

#if __has_feature(objc_arc)
#define _SG_OBJC_RETAIN(obj) { }
#define _SG_OBJC_RELEASE(obj) { obj = nil; }
#else
#define _SG_OBJC_RETAIN(obj) { [obj retain]; }
#define _SG_OBJC_RELEASE(obj) { [obj release]; obj = nil; }
#endif

//-- enum translation functions ------------------------------------------------
_SOKOL_PRIVATE MTLLoadAction _sg_mtl_load_action(sg_load_action a) {
    switch (a) {
        case SG_LOADACTION_CLEAR:       return MTLLoadActionClear;
        case SG_LOADACTION_LOAD:        return MTLLoadActionLoad;
        case SG_LOADACTION_DONTCARE:    return MTLLoadActionDontCare;
        default: SOKOL_UNREACHABLE;     return (MTLLoadAction)0;
    }
}

_SOKOL_PRIVATE MTLStoreAction _sg_mtl_store_action(sg_store_action a, bool resolve) {
    switch (a) {
        case SG_STOREACTION_STORE:
            if (resolve) {
                return MTLStoreActionStoreAndMultisampleResolve;
            } else {
                return MTLStoreActionStore;
            }
            break;
        case SG_STOREACTION_DONTCARE:
            if (resolve) {
                return MTLStoreActionMultisampleResolve;
            } else {
                return MTLStoreActionDontCare;
            }
            break;
        default: SOKOL_UNREACHABLE; return (MTLStoreAction)0;
    }
}

_SOKOL_PRIVATE MTLResourceOptions _sg_mtl_resource_options_storage_mode_managed_or_shared(void) {
    #if defined(_SG_TARGET_MACOS)
    if (_sg.mtl.use_shared_storage_mode) {
        return MTLResourceStorageModeShared;
    } else {
        return MTLResourceStorageModeManaged;
    }
    #else
        // MTLResourceStorageModeManaged is not even defined on iOS SDK
        return MTLResourceStorageModeShared;
    #endif
}

_SOKOL_PRIVATE MTLResourceOptions _sg_mtl_buffer_resource_options(sg_usage usg) {
    switch (usg) {
        case SG_USAGE_IMMUTABLE:
            return _sg_mtl_resource_options_storage_mode_managed_or_shared();
        case SG_USAGE_DYNAMIC:
        case SG_USAGE_STREAM:
            return MTLResourceCPUCacheModeWriteCombined | _sg_mtl_resource_options_storage_mode_managed_or_shared();
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
        case SG_VERTEXFORMAT_HALF2:     return MTLVertexFormatHalf2;
        case SG_VERTEXFORMAT_HALF4:     return MTLVertexFormatHalf4;
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
        case SG_PIXELFORMAT_SRGB8A8:                return MTLPixelFormatRGBA8Unorm_sRGB;
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
    if (_sg.features.image_clamp_to_border) {
        if (@available(macOS 12.0, iOS 14.0, *)) {
            // border color feature available
            switch (w) {
                case SG_WRAP_REPEAT:            return MTLSamplerAddressModeRepeat;
                case SG_WRAP_CLAMP_TO_EDGE:     return MTLSamplerAddressModeClampToEdge;
                case SG_WRAP_CLAMP_TO_BORDER:   return MTLSamplerAddressModeClampToBorderColor;
                case SG_WRAP_MIRRORED_REPEAT:   return MTLSamplerAddressModeMirrorRepeat;
                default: SOKOL_UNREACHABLE; return (MTLSamplerAddressMode)0;
            }
        }
    }
    // fallthrough: clamp to border no supported
    switch (w) {
        case SG_WRAP_REPEAT:            return MTLSamplerAddressModeRepeat;
        case SG_WRAP_CLAMP_TO_EDGE:     return MTLSamplerAddressModeClampToEdge;
        case SG_WRAP_CLAMP_TO_BORDER:   return MTLSamplerAddressModeClampToEdge;
        case SG_WRAP_MIRRORED_REPEAT:   return MTLSamplerAddressModeMirrorRepeat;
        default: SOKOL_UNREACHABLE; return (MTLSamplerAddressMode)0;
    }
}

_SOKOL_PRIVATE API_AVAILABLE(ios(14.0), macos(12.0)) MTLSamplerBorderColor _sg_mtl_border_color(sg_border_color c) {
    switch (c) {
        case SG_BORDERCOLOR_TRANSPARENT_BLACK: return MTLSamplerBorderColorTransparentBlack;
        case SG_BORDERCOLOR_OPAQUE_BLACK: return MTLSamplerBorderColorOpaqueBlack;
        case SG_BORDERCOLOR_OPAQUE_WHITE: return MTLSamplerBorderColorOpaqueWhite;
        default: SOKOL_UNREACHABLE; return (MTLSamplerBorderColor)0;
    }
}

_SOKOL_PRIVATE MTLSamplerMinMagFilter _sg_mtl_minmag_filter(sg_filter f) {
    switch (f) {
        case SG_FILTER_NEAREST:
            return MTLSamplerMinMagFilterNearest;
        case SG_FILTER_LINEAR:
            return MTLSamplerMinMagFilterLinear;
        default:
            SOKOL_UNREACHABLE; return (MTLSamplerMinMagFilter)0;
    }
}

_SOKOL_PRIVATE MTLSamplerMipFilter _sg_mtl_mipmap_filter(sg_filter f) {
    switch (f) {
        case SG_FILTER_NONE:
            return MTLSamplerMipFilterNotMipmapped;
        case SG_FILTER_NEAREST:
            return MTLSamplerMipFilterNearest;
        case SG_FILTER_LINEAR:
            return MTLSamplerMipFilterLinear;
        default:
            SOKOL_UNREACHABLE; return (MTLSamplerMipFilter)0;
    }
}

//-- a pool for all Metal resource objects, with deferred release queue ---------
_SOKOL_PRIVATE void _sg_mtl_init_pool(const sg_desc* desc) {
    _sg.mtl.idpool.num_slots = 2 *
        (
            2 * desc->buffer_pool_size +
            4 * desc->image_pool_size +
            1 * desc->sampler_pool_size +
            4 * desc->shader_pool_size +
            2 * desc->pipeline_pool_size +
            desc->pass_pool_size +
            128
        );
    _sg.mtl.idpool.pool = [NSMutableArray arrayWithCapacity:(NSUInteger)_sg.mtl.idpool.num_slots];
    _SG_OBJC_RETAIN(_sg.mtl.idpool.pool);
    NSNull* null = [NSNull null];
    for (int i = 0; i < _sg.mtl.idpool.num_slots; i++) {
        [_sg.mtl.idpool.pool addObject:null];
    }
    SOKOL_ASSERT([_sg.mtl.idpool.pool count] == (NSUInteger)_sg.mtl.idpool.num_slots);
    // a queue of currently free slot indices
    _sg.mtl.idpool.free_queue_top = 0;
    _sg.mtl.idpool.free_queue = (int*)_sg_malloc_clear((size_t)_sg.mtl.idpool.num_slots * sizeof(int));
    // pool slot 0 is reserved!
    for (int i = _sg.mtl.idpool.num_slots-1; i >= 1; i--) {
        _sg.mtl.idpool.free_queue[_sg.mtl.idpool.free_queue_top++] = i;
    }
    // a circular queue which holds release items (frame index when a resource is to be released, and the resource's pool index
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

// get a new free resource pool slot
_SOKOL_PRIVATE int _sg_mtl_alloc_pool_slot(void) {
    SOKOL_ASSERT(_sg.mtl.idpool.free_queue_top > 0);
    const int slot_index = _sg.mtl.idpool.free_queue[--_sg.mtl.idpool.free_queue_top];
    SOKOL_ASSERT((slot_index > 0) && (slot_index < _sg.mtl.idpool.num_slots));
    return slot_index;
}

// put a free resource pool slot back into the free-queue
_SOKOL_PRIVATE void _sg_mtl_free_pool_slot(int slot_index) {
    SOKOL_ASSERT(_sg.mtl.idpool.free_queue_top < _sg.mtl.idpool.num_slots);
    SOKOL_ASSERT((slot_index > 0) && (slot_index < _sg.mtl.idpool.num_slots));
    _sg.mtl.idpool.free_queue[_sg.mtl.idpool.free_queue_top++] = slot_index;
}

// add an MTLResource to the pool, return pool index or 0 if input was 'nil'
_SOKOL_PRIVATE int _sg_mtl_add_resource(id res) {
    if (nil == res) {
        return _SG_MTL_INVALID_SLOT_INDEX;
    }
    _sg_stats_add(metal.idpool.num_added, 1);
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
    _sg_stats_add(metal.idpool.num_released, 1);
    SOKOL_ASSERT((slot_index > 0) && (slot_index < _sg.mtl.idpool.num_slots));
    SOKOL_ASSERT([NSNull null] != _sg.mtl.idpool.pool[(NSUInteger)slot_index]);
    int release_index = _sg.mtl.idpool.release_queue_front++;
    if (_sg.mtl.idpool.release_queue_front >= _sg.mtl.idpool.num_slots) {
        // wrap-around
        _sg.mtl.idpool.release_queue_front = 0;
    }
    // release queue full?
    SOKOL_ASSERT(_sg.mtl.idpool.release_queue_front != _sg.mtl.idpool.release_queue_back);
    SOKOL_ASSERT(0 == _sg.mtl.idpool.release_queue[release_index].frame_index);
    const uint32_t safe_to_release_frame_index = frame_index + SG_NUM_INFLIGHT_FRAMES + 1;
    _sg.mtl.idpool.release_queue[release_index].frame_index = safe_to_release_frame_index;
    _sg.mtl.idpool.release_queue[release_index].slot_index = slot_index;
}

// run garbage-collection pass on all resources in the release-queue
_SOKOL_PRIVATE void _sg_mtl_garbage_collect(uint32_t frame_index) {
    while (_sg.mtl.idpool.release_queue_back != _sg.mtl.idpool.release_queue_front) {
        if (frame_index < _sg.mtl.idpool.release_queue[_sg.mtl.idpool.release_queue_back].frame_index) {
            // don't need to check further, release-items past this are too young
            break;
        }
        _sg_stats_add(metal.idpool.num_garbage_collected, 1);
        // safe to release this resource
        const int slot_index = _sg.mtl.idpool.release_queue[_sg.mtl.idpool.release_queue_back].slot_index;
        SOKOL_ASSERT((slot_index > 0) && (slot_index < _sg.mtl.idpool.num_slots));
        // note: the NSMutableArray takes ownership of its items, assigning an NSNull object will
        // release the object, no matter if using ARC or not
        SOKOL_ASSERT(_sg.mtl.idpool.pool[(NSUInteger)slot_index] != [NSNull null]);
        _sg.mtl.idpool.pool[(NSUInteger)slot_index] = [NSNull null];
        // put the now free pool index back on the free queue
        _sg_mtl_free_pool_slot(slot_index);
        // reset the release queue slot and advance the back index
        _sg.mtl.idpool.release_queue[_sg.mtl.idpool.release_queue_back].frame_index = 0;
        _sg.mtl.idpool.release_queue[_sg.mtl.idpool.release_queue_back].slot_index = _SG_MTL_INVALID_SLOT_INDEX;
        _sg.mtl.idpool.release_queue_back++;
        if (_sg.mtl.idpool.release_queue_back >= _sg.mtl.idpool.num_slots) {
            // wrap-around
            _sg.mtl.idpool.release_queue_back = 0;
        }
    }
}

_SOKOL_PRIVATE id _sg_mtl_id(int slot_index) {
    return _sg.mtl.idpool.pool[(NSUInteger)slot_index];
}

_SOKOL_PRIVATE void _sg_mtl_clear_state_cache(void) {
    _sg_clear(&_sg.mtl.state_cache, sizeof(_sg.mtl.state_cache));
}

// https://developer.apple.com/metal/Metal-Feature-Set-Tables.pdf
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
    _sg.features.origin_top_left = true;
    _sg.features.mrt_independent_blend_state = true;
    _sg.features.mrt_independent_write_mask = true;

    _sg.features.image_clamp_to_border = false;
    #if (MAC_OS_X_VERSION_MAX_ALLOWED >= 120000) || (__IPHONE_OS_VERSION_MAX_ALLOWED >= 140000)
    if (@available(macOS 12.0, iOS 14.0, *)) {
        _sg.features.image_clamp_to_border = [_sg.mtl.device supportsFamily:MTLGPUFamilyApple7]
                                             || [_sg.mtl.device supportsFamily:MTLGPUFamilyMac2];
        #if (MAC_OS_X_VERSION_MAX_ALLOWED >= 130000) || (__IPHONE_OS_VERSION_MAX_ALLOWED >= 160000)
        if (!_sg.features.image_clamp_to_border) {
            if (@available(macOS 13.0, iOS 16.0, *)) {
                _sg.features.image_clamp_to_border = [_sg.mtl.device supportsFamily:MTLGPUFamilyMetal3];
            }
        }
        #endif
    }
    #endif

    #if defined(_SG_TARGET_MACOS)
        _sg.limits.max_image_size_2d = 16 * 1024;
        _sg.limits.max_image_size_cube = 16 * 1024;
        _sg.limits.max_image_size_3d = 2 * 1024;
        _sg.limits.max_image_size_array = 16 * 1024;
        _sg.limits.max_image_array_layers = 2 * 1024;
    #else
        // FIXME: newer iOS devices support 16k textures
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
    _sg_pixelformat_all(&_sg.formats[SG_PIXELFORMAT_SRGB8A8]);
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

//-- main Metal backend state and functions ------------------------------------
_SOKOL_PRIVATE void _sg_mtl_setup_backend(const sg_desc* desc) {
    // assume already zero-initialized
    SOKOL_ASSERT(desc);
    SOKOL_ASSERT(desc->context.metal.device);
    SOKOL_ASSERT(desc->context.metal.renderpass_descriptor_cb || desc->context.metal.renderpass_descriptor_userdata_cb);
    SOKOL_ASSERT(desc->context.metal.drawable_cb || desc->context.metal.drawable_userdata_cb);
    SOKOL_ASSERT(desc->uniform_buffer_size > 0);
    _sg_mtl_init_pool(desc);
    _sg_mtl_clear_state_cache();
    _sg.mtl.valid = true;
    _sg.mtl.renderpass_descriptor_cb = desc->context.metal.renderpass_descriptor_cb;
    _sg.mtl.renderpass_descriptor_userdata_cb = desc->context.metal.renderpass_descriptor_userdata_cb;
    _sg.mtl.drawable_cb = desc->context.metal.drawable_cb;
    _sg.mtl.drawable_userdata_cb = desc->context.metal.drawable_userdata_cb;
    _sg.mtl.user_data = desc->context.metal.user_data;
    _sg.mtl.ub_size = desc->uniform_buffer_size;
    _sg.mtl.sem = dispatch_semaphore_create(SG_NUM_INFLIGHT_FRAMES);
    _sg.mtl.device = (__bridge id<MTLDevice>) desc->context.metal.device;
    _sg.mtl.cmd_queue = [_sg.mtl.device newCommandQueue];

    for (int i = 0; i < SG_NUM_INFLIGHT_FRAMES; i++) {
        _sg.mtl.uniform_buffers[i] = [_sg.mtl.device
            newBufferWithLength:(NSUInteger)_sg.mtl.ub_size
            options:MTLResourceCPUCacheModeWriteCombined|MTLResourceStorageModeShared
        ];
        #if defined(SOKOL_DEBUG)
            _sg.mtl.uniform_buffers[i].label = [NSString stringWithFormat:@"sg-uniform-buffer.%d", i];
        #endif
    }

    if (desc->mtl_force_managed_storage_mode) {
        _sg.mtl.use_shared_storage_mode = false;
    } else if (@available(macOS 10.15, iOS 13.0, *)) {
        // on Intel Macs, always use managed resources even though the
        // device says it supports unified memory (because of texture restrictions)
        const bool is_apple_gpu = [_sg.mtl.device supportsFamily:MTLGPUFamilyApple1];
        if (!is_apple_gpu) {
            _sg.mtl.use_shared_storage_mode = false;
        } else {
            _sg.mtl.use_shared_storage_mode = true;
        }
    } else {
        #if defined(_SG_TARGET_MACOS)
            _sg.mtl.use_shared_storage_mode = false;
        #else
            _sg.mtl.use_shared_storage_mode = true;
        #endif
    }
    _sg_mtl_init_caps();
}

_SOKOL_PRIVATE void _sg_mtl_discard_backend(void) {
    SOKOL_ASSERT(_sg.mtl.valid);
    // wait for the last frame to finish
    for (int i = 0; i < SG_NUM_INFLIGHT_FRAMES; i++) {
        dispatch_semaphore_wait(_sg.mtl.sem, DISPATCH_TIME_FOREVER);
    }
    // semaphore must be "relinquished" before destruction
    for (int i = 0; i < SG_NUM_INFLIGHT_FRAMES; i++) {
        dispatch_semaphore_signal(_sg.mtl.sem);
    }
    _sg_mtl_garbage_collect(_sg.frame_index + SG_NUM_INFLIGHT_FRAMES + 2);
    _sg_mtl_destroy_pool();
    _sg.mtl.valid = false;

    _SG_OBJC_RELEASE(_sg.mtl.sem);
    _SG_OBJC_RELEASE(_sg.mtl.device);
    _SG_OBJC_RELEASE(_sg.mtl.cmd_queue);
    for (int i = 0; i < SG_NUM_INFLIGHT_FRAMES; i++) {
        _SG_OBJC_RELEASE(_sg.mtl.uniform_buffers[i]);
    }
    // NOTE: MTLCommandBuffer and MTLRenderCommandEncoder are auto-released
    _sg.mtl.cmd_buffer = nil;
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
    // need to restore the uniform buffer binding (normally happens in _sg_mtl_begin_pass()
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
    // empty
}

_SOKOL_PRIVATE void _sg_mtl_activate_context(_sg_context_t* ctx) {
    _SOKOL_UNUSED(ctx);
    _sg_mtl_clear_state_cache();
}

_SOKOL_PRIVATE sg_resource_state _sg_mtl_create_buffer(_sg_buffer_t* buf, const sg_buffer_desc* desc) {
    SOKOL_ASSERT(buf && desc);
    const bool injected = (0 != desc->mtl_buffers[0]);
    MTLResourceOptions mtl_options = _sg_mtl_buffer_resource_options(buf->cmn.usage);
    for (int slot = 0; slot < buf->cmn.num_slots; slot++) {
        id<MTLBuffer> mtl_buf;
        if (injected) {
            SOKOL_ASSERT(desc->mtl_buffers[slot]);
            mtl_buf = (__bridge id<MTLBuffer>) desc->mtl_buffers[slot];
        } else {
            if (buf->cmn.usage == SG_USAGE_IMMUTABLE) {
                SOKOL_ASSERT(desc->data.ptr);
                mtl_buf = [_sg.mtl.device newBufferWithBytes:desc->data.ptr length:(NSUInteger)buf->cmn.size options:mtl_options];
            } else {
                mtl_buf = [_sg.mtl.device newBufferWithLength:(NSUInteger)buf->cmn.size options:mtl_options];
            }
            if (nil == mtl_buf) {
                _SG_ERROR(METAL_CREATE_BUFFER_FAILED);
                return SG_RESOURCESTATE_FAILED;
            }
        }
        #if defined(SOKOL_DEBUG)
            if (desc->label) {
                mtl_buf.label = [NSString stringWithFormat:@"%s.%d", desc->label, slot];
            }
        #endif
        buf->mtl.buf[slot] = _sg_mtl_add_resource(mtl_buf);
        _SG_OBJC_RELEASE(mtl_buf);
    }
    return SG_RESOURCESTATE_VALID;
}

_SOKOL_PRIVATE void _sg_mtl_discard_buffer(_sg_buffer_t* buf) {
    SOKOL_ASSERT(buf);
    for (int slot = 0; slot < buf->cmn.num_slots; slot++) {
        // it's valid to call release resource with '0'
        _sg_mtl_release_resource(_sg.frame_index, buf->mtl.buf[slot]);
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
            const int mip_width = _sg_miplevel_dim(img->cmn.width, mip_index);
            const int mip_height = _sg_miplevel_dim(img->cmn.height, mip_index);
            // special case PVRTC formats: bytePerRow and bytesPerImage must be 0
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
                const int mip_depth = _sg_miplevel_dim(img->cmn.num_slices, mip_index);
                region = MTLRegionMake3D(0, 0, 0, (NSUInteger)mip_width, (NSUInteger)mip_height, (NSUInteger)mip_depth);
                bytes_per_image = bytes_per_slice;
                // FIXME: apparently the minimal bytes_per_image size for 3D texture is 4 KByte... somehow need to handle this
            } else {
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

// initialize MTLTextureDescritor with common attributes
_SOKOL_PRIVATE bool _sg_mtl_init_texdesc_common(MTLTextureDescriptor* mtl_desc, _sg_image_t* img) {
    mtl_desc.textureType = _sg_mtl_texture_type(img->cmn.type);
    mtl_desc.pixelFormat = _sg_mtl_pixel_format(img->cmn.pixel_format);
    if (MTLPixelFormatInvalid == mtl_desc.pixelFormat) {
        _SG_ERROR(METAL_TEXTURE_FORMAT_NOT_SUPPORTED);
        return false;
    }
    mtl_desc.width = (NSUInteger)img->cmn.width;
    mtl_desc.height = (NSUInteger)img->cmn.height;
    if (SG_IMAGETYPE_3D == img->cmn.type) {
        mtl_desc.depth = (NSUInteger)img->cmn.num_slices;
    } else {
        mtl_desc.depth = 1;
    }
    mtl_desc.mipmapLevelCount = (NSUInteger)img->cmn.num_mipmaps;
    if (SG_IMAGETYPE_ARRAY == img->cmn.type) {
        mtl_desc.arrayLength = (NSUInteger)img->cmn.num_slices;
    } else {
        mtl_desc.arrayLength = 1;
    }
    mtl_desc.usage = MTLTextureUsageShaderRead;
    MTLResourceOptions res_options = 0;
    if (img->cmn.usage != SG_USAGE_IMMUTABLE) {
        res_options |= MTLResourceCPUCacheModeWriteCombined;
    }
    res_options |= _sg_mtl_resource_options_storage_mode_managed_or_shared();
    mtl_desc.resourceOptions = res_options;
    return true;
}

// initialize MTLTextureDescritor with rendertarget attributes
_SOKOL_PRIVATE void _sg_mtl_init_texdesc_rt(MTLTextureDescriptor* mtl_desc, _sg_image_t* img) {
    SOKOL_ASSERT(img->cmn.render_target);
    _SOKOL_UNUSED(img);
    mtl_desc.usage = MTLTextureUsageShaderRead | MTLTextureUsageRenderTarget;
    mtl_desc.resourceOptions = MTLResourceStorageModePrivate;
}

// initialize MTLTextureDescritor with MSAA attributes
_SOKOL_PRIVATE void _sg_mtl_init_texdesc_rt_msaa(MTLTextureDescriptor* mtl_desc, _sg_image_t* img) {
    SOKOL_ASSERT(img->cmn.sample_count > 1);
    mtl_desc.usage = MTLTextureUsageRenderTarget;
    mtl_desc.resourceOptions = MTLResourceStorageModePrivate;
    mtl_desc.textureType = MTLTextureType2DMultisample;
    mtl_desc.sampleCount = (NSUInteger)img->cmn.sample_count;
}

_SOKOL_PRIVATE sg_resource_state _sg_mtl_create_image(_sg_image_t* img, const sg_image_desc* desc) {
    SOKOL_ASSERT(img && desc);
    const bool injected = (0 != desc->mtl_textures[0]);

    // first initialize all Metal resource pool slots to 'empty'
    for (int i = 0; i < SG_NUM_INFLIGHT_FRAMES; i++) {
        img->mtl.tex[i] = _sg_mtl_add_resource(nil);
    }

    // initialize a Metal texture descriptor
    MTLTextureDescriptor* mtl_desc = [[MTLTextureDescriptor alloc] init];
    if (!_sg_mtl_init_texdesc_common(mtl_desc, img)) {
        _SG_OBJC_RELEASE(mtl_desc);
        return SG_RESOURCESTATE_FAILED;
    }
    if (img->cmn.render_target) {
        if (img->cmn.sample_count > 1) {
            _sg_mtl_init_texdesc_rt_msaa(mtl_desc, img);
        } else {
            _sg_mtl_init_texdesc_rt(mtl_desc, img);
        }
    }
    for (int slot = 0; slot < img->cmn.num_slots; slot++) {
        id<MTLTexture> mtl_tex;
        if (injected) {
            SOKOL_ASSERT(desc->mtl_textures[slot]);
            mtl_tex = (__bridge id<MTLTexture>) desc->mtl_textures[slot];
        } else {
            mtl_tex = [_sg.mtl.device newTextureWithDescriptor:mtl_desc];
            if (nil == mtl_tex) {
                _SG_OBJC_RELEASE(mtl_desc);
                _SG_ERROR(METAL_CREATE_TEXTURE_FAILED);
                return SG_RESOURCESTATE_FAILED;
            }
            if ((img->cmn.usage == SG_USAGE_IMMUTABLE) && !img->cmn.render_target) {
                _sg_mtl_copy_image_data(img, mtl_tex, &desc->data);
            }
        }
        #if defined(SOKOL_DEBUG)
            if (desc->label) {
                mtl_tex.label = [NSString stringWithFormat:@"%s.%d", desc->label, slot];
            }
        #endif
        img->mtl.tex[slot] = _sg_mtl_add_resource(mtl_tex);
        _SG_OBJC_RELEASE(mtl_tex);
    }
    _SG_OBJC_RELEASE(mtl_desc);
    return SG_RESOURCESTATE_VALID;
}

_SOKOL_PRIVATE void _sg_mtl_discard_image(_sg_image_t* img) {
    SOKOL_ASSERT(img);
    // it's valid to call release resource with a 'null resource'
    for (int slot = 0; slot < img->cmn.num_slots; slot++) {
        _sg_mtl_release_resource(_sg.frame_index, img->mtl.tex[slot]);
    }
}

_SOKOL_PRIVATE sg_resource_state _sg_mtl_create_sampler(_sg_sampler_t* smp, const sg_sampler_desc* desc) {
    SOKOL_ASSERT(smp && desc);
    id<MTLSamplerState> mtl_smp;
    const bool injected = (0 != desc->mtl_sampler);
    if (injected) {
        SOKOL_ASSERT(desc->mtl_sampler);
        mtl_smp = (__bridge id<MTLSamplerState>) desc->mtl_sampler;
    } else {
        MTLSamplerDescriptor* mtl_desc = [[MTLSamplerDescriptor alloc] init];
        mtl_desc.sAddressMode = _sg_mtl_address_mode(desc->wrap_u);
        mtl_desc.tAddressMode = _sg_mtl_address_mode(desc->wrap_v);
        mtl_desc.rAddressMode = _sg_mtl_address_mode(desc->wrap_w);
        if (_sg.features.image_clamp_to_border) {
            if (@available(macOS 12.0, iOS 14.0, *)) {
                mtl_desc.borderColor  = _sg_mtl_border_color(desc->border_color);
            }
        }
        mtl_desc.minFilter = _sg_mtl_minmag_filter(desc->min_filter);
        mtl_desc.magFilter = _sg_mtl_minmag_filter(desc->mag_filter);
        mtl_desc.mipFilter = _sg_mtl_mipmap_filter(desc->mipmap_filter);
        mtl_desc.lodMinClamp = desc->min_lod;
        mtl_desc.lodMaxClamp = desc->max_lod;
        // FIXME: lodAverage?
        mtl_desc.maxAnisotropy = desc->max_anisotropy;
        mtl_desc.normalizedCoordinates = YES;
        mtl_desc.compareFunction = _sg_mtl_compare_func(desc->compare);
        #if defined(SOKOL_DEBUG)
            if (desc->label) {
                mtl_desc.label = [NSString stringWithUTF8String:desc->label];
            }
        #endif
        mtl_smp = [_sg.mtl.device newSamplerStateWithDescriptor:mtl_desc];
        _SG_OBJC_RELEASE(mtl_desc);
        if (nil == mtl_smp) {
            _SG_ERROR(METAL_CREATE_SAMPLER_FAILED);
            return SG_RESOURCESTATE_FAILED;
        }
    }
    smp->mtl.sampler_state = _sg_mtl_add_resource(mtl_smp);
    _SG_OBJC_RELEASE(mtl_smp);
    return SG_RESOURCESTATE_VALID;
}

_SOKOL_PRIVATE void _sg_mtl_discard_sampler(_sg_sampler_t* smp) {
    SOKOL_ASSERT(smp);
    // it's valid to call release resource with a 'null resource'
    _sg_mtl_release_resource(_sg.frame_index, smp->mtl.sampler_state);
}

_SOKOL_PRIVATE id<MTLLibrary> _sg_mtl_compile_library(const char* src) {
    NSError* err = NULL;
    id<MTLLibrary> lib = [_sg.mtl.device
        newLibraryWithSource:[NSString stringWithUTF8String:src]
        options:nil
        error:&err
    ];
    if (err) {
        _SG_ERROR(METAL_SHADER_COMPILATION_FAILED);
        _SG_LOGMSG(METAL_SHADER_COMPILATION_OUTPUT, [err.localizedDescription UTF8String]);
    }
    return lib;
}

_SOKOL_PRIVATE id<MTLLibrary> _sg_mtl_library_from_bytecode(const void* ptr, size_t num_bytes) {
    NSError* err = NULL;
    dispatch_data_t lib_data = dispatch_data_create(ptr, num_bytes, NULL, DISPATCH_DATA_DESTRUCTOR_DEFAULT);
    id<MTLLibrary> lib = [_sg.mtl.device newLibraryWithData:lib_data error:&err];
    if (err) {
        _SG_ERROR(METAL_SHADER_CREATION_FAILED);
        _SG_LOGMSG(METAL_SHADER_COMPILATION_OUTPUT, [err.localizedDescription UTF8String]);
    }
    _SG_OBJC_RELEASE(lib_data);
    return lib;
}

_SOKOL_PRIVATE sg_resource_state _sg_mtl_create_shader(_sg_shader_t* shd, const sg_shader_desc* desc) {
    SOKOL_ASSERT(shd && desc);

    // create metal library objects and lookup entry functions
    id<MTLLibrary> vs_lib = nil;
    id<MTLLibrary> fs_lib = nil;
    id<MTLFunction> vs_func = nil;
    id<MTLFunction> fs_func = nil;
    const char* vs_entry = desc->vs.entry;
    const char* fs_entry = desc->fs.entry;
    if (desc->vs.bytecode.ptr && desc->fs.bytecode.ptr) {
        // separate byte code provided
        vs_lib = _sg_mtl_library_from_bytecode(desc->vs.bytecode.ptr, desc->vs.bytecode.size);
        fs_lib = _sg_mtl_library_from_bytecode(desc->fs.bytecode.ptr, desc->fs.bytecode.size);
        if ((nil == vs_lib) || (nil == fs_lib)) {
            goto failed;
        }
        vs_func = [vs_lib newFunctionWithName:[NSString stringWithUTF8String:vs_entry]];
        fs_func = [fs_lib newFunctionWithName:[NSString stringWithUTF8String:fs_entry]];
    } else if (desc->vs.source && desc->fs.source) {
        // separate sources provided
        vs_lib = _sg_mtl_compile_library(desc->vs.source);
        fs_lib = _sg_mtl_compile_library(desc->fs.source);
        if ((nil == vs_lib) || (nil == fs_lib)) {
            goto failed;
        }
        vs_func = [vs_lib newFunctionWithName:[NSString stringWithUTF8String:vs_entry]];
        fs_func = [fs_lib newFunctionWithName:[NSString stringWithUTF8String:fs_entry]];
    } else {
        goto failed;
    }
    if (nil == vs_func) {
        _SG_ERROR(METAL_VERTEX_SHADER_ENTRY_NOT_FOUND);
        goto failed;
    }
    if (nil == fs_func) {
        _SG_ERROR(METAL_FRAGMENT_SHADER_ENTRY_NOT_FOUND);
        goto failed;
    }
    #if defined(SOKOL_DEBUG)
        if (desc->label) {
            vs_lib.label = [NSString stringWithFormat:@"%s.vs", desc->label];
            fs_lib.label = [NSString stringWithFormat:@"%s.fs", desc->label];
        }
    #endif
    // it is legal to call _sg_mtl_add_resource with a nil value, this will return a special 0xFFFFFFFF index
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
    // it is valid to call _sg_mtl_release_resource with a 'null resource'
    _sg_mtl_release_resource(_sg.frame_index, shd->mtl.stage[SG_SHADERSTAGE_VS].mtl_func);
    _sg_mtl_release_resource(_sg.frame_index, shd->mtl.stage[SG_SHADERSTAGE_VS].mtl_lib);
    _sg_mtl_release_resource(_sg.frame_index, shd->mtl.stage[SG_SHADERSTAGE_FS].mtl_func);
    _sg_mtl_release_resource(_sg.frame_index, shd->mtl.stage[SG_SHADERSTAGE_FS].mtl_lib);
}

_SOKOL_PRIVATE sg_resource_state _sg_mtl_create_pipeline(_sg_pipeline_t* pip, _sg_shader_t* shd, const sg_pipeline_desc* desc) {
    SOKOL_ASSERT(pip && shd && desc);
    SOKOL_ASSERT(desc->shader.id == shd->slot.id);

    pip->shader = shd;

    sg_primitive_type prim_type = desc->primitive_type;
    pip->mtl.prim_type = _sg_mtl_primitive_type(prim_type);
    pip->mtl.index_size = _sg_mtl_index_size(pip->cmn.index_type);
    if (SG_INDEXTYPE_NONE != pip->cmn.index_type) {
        pip->mtl.index_type = _sg_mtl_index_type(pip->cmn.index_type);
    }
    pip->mtl.cull_mode = _sg_mtl_cull_mode(desc->cull_mode);
    pip->mtl.winding = _sg_mtl_winding(desc->face_winding);
    pip->mtl.stencil_ref = desc->stencil.ref;

    // create vertex-descriptor
    MTLVertexDescriptor* vtx_desc = [MTLVertexDescriptor vertexDescriptor];
    for (NSUInteger attr_index = 0; attr_index < SG_MAX_VERTEX_ATTRIBUTES; attr_index++) {
        const sg_vertex_attr_state* a_state = &desc->layout.attrs[attr_index];
        if (a_state->format == SG_VERTEXFORMAT_INVALID) {
            break;
        }
        SOKOL_ASSERT(a_state->buffer_index < SG_MAX_VERTEX_BUFFERS);
        vtx_desc.attributes[attr_index].format = _sg_mtl_vertex_format(a_state->format);
        vtx_desc.attributes[attr_index].offset = (NSUInteger)a_state->offset;
        vtx_desc.attributes[attr_index].bufferIndex = (NSUInteger)(a_state->buffer_index + SG_MAX_SHADERSTAGE_UBS);
        pip->cmn.vertex_buffer_layout_active[a_state->buffer_index] = true;
    }
    for (NSUInteger layout_index = 0; layout_index < SG_MAX_VERTEX_BUFFERS; layout_index++) {
        if (pip->cmn.vertex_buffer_layout_active[layout_index]) {
            const sg_vertex_buffer_layout_state* l_state = &desc->layout.buffers[layout_index];
            const NSUInteger mtl_vb_slot = layout_index + SG_MAX_SHADERSTAGE_UBS;
            SOKOL_ASSERT(l_state->stride > 0);
            vtx_desc.layouts[mtl_vb_slot].stride = (NSUInteger)l_state->stride;
            vtx_desc.layouts[mtl_vb_slot].stepFunction = _sg_mtl_step_function(l_state->step_func);
            vtx_desc.layouts[mtl_vb_slot].stepRate = (NSUInteger)l_state->step_rate;
            if (SG_VERTEXSTEP_PER_INSTANCE == l_state->step_func) {
                // NOTE: not actually used in _sg_mtl_draw()
                pip->cmn.use_instanced_draw = true;
            }
        }
    }

    // render-pipeline descriptor
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
    if (@available(macOS 10.13, iOS 11.0, *)) {
        for (NSUInteger i = 0; i < (SG_MAX_SHADERSTAGE_UBS+SG_MAX_VERTEX_BUFFERS); i++) {
            rp_desc.vertexBuffers[i].mutability = MTLMutabilityImmutable;
        }
        for (NSUInteger i = 0; i < SG_MAX_SHADERSTAGE_UBS; i++) {
            rp_desc.fragmentBuffers[i].mutability = MTLMutabilityImmutable;
        }
    }
    for (NSUInteger i = 0; i < (NSUInteger)desc->color_count; i++) {
        SOKOL_ASSERT(i < SG_MAX_COLOR_ATTACHMENTS);
        const sg_color_target_state* cs = &desc->colors[i];
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
    #if defined(SOKOL_DEBUG)
        if (desc->label) {
            rp_desc.label = [NSString stringWithFormat:@"%s", desc->label];
        }
    #endif
    NSError* err = NULL;
    id<MTLRenderPipelineState> mtl_rps = [_sg.mtl.device newRenderPipelineStateWithDescriptor:rp_desc error:&err];
    _SG_OBJC_RELEASE(rp_desc);
    if (nil == mtl_rps) {
        SOKOL_ASSERT(err);
        _SG_ERROR(METAL_CREATE_RPS_FAILED);
        _SG_LOGMSG(METAL_CREATE_RPS_OUTPUT, [err.localizedDescription UTF8String]);
        return SG_RESOURCESTATE_FAILED;
    }
    pip->mtl.rps = _sg_mtl_add_resource(mtl_rps);
    _SG_OBJC_RELEASE(mtl_rps);

    // depth-stencil-state
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
    #if defined(SOKOL_DEBUG)
        if (desc->label) {
            ds_desc.label = [NSString stringWithFormat:@"%s.dss", desc->label];
        }
    #endif
    id<MTLDepthStencilState> mtl_dss = [_sg.mtl.device newDepthStencilStateWithDescriptor:ds_desc];
    _SG_OBJC_RELEASE(ds_desc);
    if (nil == mtl_dss) {
        _SG_ERROR(METAL_CREATE_DSS_FAILED);
        return SG_RESOURCESTATE_FAILED;
    }
    pip->mtl.dss = _sg_mtl_add_resource(mtl_dss);
    _SG_OBJC_RELEASE(mtl_dss);
    return SG_RESOURCESTATE_VALID;
}

_SOKOL_PRIVATE void _sg_mtl_discard_pipeline(_sg_pipeline_t* pip) {
    SOKOL_ASSERT(pip);
    // it's valid to call release resource with a 'null resource'
    _sg_mtl_release_resource(_sg.frame_index, pip->mtl.rps);
    _sg_mtl_release_resource(_sg.frame_index, pip->mtl.dss);
}

_SOKOL_PRIVATE sg_resource_state _sg_mtl_create_pass(_sg_pass_t* pass, _sg_image_t** color_images, _sg_image_t** resolve_images, _sg_image_t* ds_img, const sg_pass_desc* desc) {
    SOKOL_ASSERT(pass && desc);
    SOKOL_ASSERT(color_images && resolve_images);

    // copy image pointers
    for (int i = 0; i < pass->cmn.num_color_atts; i++) {
        const sg_pass_attachment_desc* color_desc = &desc->color_attachments[i];
        _SOKOL_UNUSED(color_desc);
        SOKOL_ASSERT(color_desc->image.id != SG_INVALID_ID);
        SOKOL_ASSERT(0 == pass->mtl.color_atts[i].image);
        SOKOL_ASSERT(color_images[i] && (color_images[i]->slot.id == color_desc->image.id));
        SOKOL_ASSERT(_sg_is_valid_rendertarget_color_format(color_images[i]->cmn.pixel_format));
        pass->mtl.color_atts[i].image = color_images[i];

        const sg_pass_attachment_desc* resolve_desc = &desc->resolve_attachments[i];
        if (resolve_desc->image.id != SG_INVALID_ID) {
            SOKOL_ASSERT(0 == pass->mtl.resolve_atts[i].image);
            SOKOL_ASSERT(resolve_images[i] && (resolve_images[i]->slot.id == resolve_desc->image.id));
            SOKOL_ASSERT(color_images[i] && (color_images[i]->cmn.pixel_format == resolve_images[i]->cmn.pixel_format));
            pass->mtl.resolve_atts[i].image = resolve_images[i];
        }
    }
    SOKOL_ASSERT(0 == pass->mtl.ds_att.image);
    const sg_pass_attachment_desc* ds_desc = &desc->depth_stencil_attachment;
    if (ds_desc->image.id != SG_INVALID_ID) {
        SOKOL_ASSERT(ds_img && (ds_img->slot.id == ds_desc->image.id));
        SOKOL_ASSERT(_sg_is_valid_rendertarget_depth_format(ds_img->cmn.pixel_format));
        pass->mtl.ds_att.image = ds_img;
    }
    return SG_RESOURCESTATE_VALID;
}

_SOKOL_PRIVATE void _sg_mtl_discard_pass(_sg_pass_t* pass) {
    SOKOL_ASSERT(pass);
    _SOKOL_UNUSED(pass);
}

_SOKOL_PRIVATE _sg_image_t* _sg_mtl_pass_color_image(const _sg_pass_t* pass, int index) {
    // NOTE: may return null
    SOKOL_ASSERT(pass && (index >= 0) && (index < SG_MAX_COLOR_ATTACHMENTS));
    return pass->mtl.color_atts[index].image;
}

_SOKOL_PRIVATE _sg_image_t* _sg_mtl_pass_resolve_image(const _sg_pass_t* pass, int index) {
    // NOTE: may return null
    SOKOL_ASSERT(pass && (index >= 0) && (index < SG_MAX_COLOR_ATTACHMENTS));
    return pass->mtl.resolve_atts[index].image;
}

_SOKOL_PRIVATE _sg_image_t* _sg_mtl_pass_ds_image(const _sg_pass_t* pass) {
    // NOTE: may return null
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
        // block until the oldest frame in flight has finished
        dispatch_semaphore_wait(_sg.mtl.sem, DISPATCH_TIME_FOREVER);
        _sg.mtl.cmd_buffer = [_sg.mtl.cmd_queue commandBufferWithUnretainedReferences];
        [_sg.mtl.cmd_buffer enqueue];
        [_sg.mtl.cmd_buffer addCompletedHandler:^(id<MTLCommandBuffer> cmd_buf) {
            // NOTE: this code is called on a different thread!
            _SOKOL_UNUSED(cmd_buf);
            dispatch_semaphore_signal(_sg.mtl.sem);
        }];
    }

    // if this is first pass in frame, get uniform buffer base pointer
    if (0 == _sg.mtl.cur_ub_base_ptr) {
        _sg.mtl.cur_ub_base_ptr = (uint8_t*)[_sg.mtl.uniform_buffers[_sg.mtl.cur_frame_rotate_index] contents];
    }

    // initialize a render pass descriptor
    MTLRenderPassDescriptor* pass_desc = nil;
    if (pass) {
        // offscreen render pass
        pass_desc = [MTLRenderPassDescriptor renderPassDescriptor];
    } else {
        // default render pass, call user-provided callback to provide render pass descriptor
        if (_sg.mtl.renderpass_descriptor_cb) {
            pass_desc = (__bridge MTLRenderPassDescriptor*) _sg.mtl.renderpass_descriptor_cb();
        } else {
            pass_desc = (__bridge MTLRenderPassDescriptor*) _sg.mtl.renderpass_descriptor_userdata_cb(_sg.mtl.user_data);
        }
        // pin the swapchain resources into memory so that they outlive their command buffer
        // (this is necessary because the command buffer doesn't retain references)
        int default_pass_desc_ref = _sg_mtl_add_resource(pass_desc);
        _sg_mtl_release_resource(_sg.frame_index, default_pass_desc_ref);
    }
    if (pass_desc) {
        _sg.mtl.pass_valid = true;
    } else {
        // default pass descriptor will not be valid if window is minimized, don't do any rendering in this case
        _sg.mtl.pass_valid = false;
        return;
    }
    if (pass) {
        // setup pass descriptor for offscreen rendering
        SOKOL_ASSERT(pass->slot.state == SG_RESOURCESTATE_VALID);
        for (NSUInteger i = 0; i < (NSUInteger)pass->cmn.num_color_atts; i++) {
            const _sg_pass_attachment_t* cmn_color_att = &pass->cmn.color_atts[i];
            const _sg_mtl_attachment_t* mtl_color_att = &pass->mtl.color_atts[i];
            const _sg_image_t* color_att_img = mtl_color_att->image;
            const _sg_pass_attachment_t* cmn_resolve_att = &pass->cmn.resolve_atts[i];
            const _sg_mtl_attachment_t* mtl_resolve_att = &pass->mtl.resolve_atts[i];
            const _sg_image_t* resolve_att_img = mtl_resolve_att->image;
            SOKOL_ASSERT(color_att_img->slot.state == SG_RESOURCESTATE_VALID);
            SOKOL_ASSERT(color_att_img->slot.id == cmn_color_att->image_id.id);
            SOKOL_ASSERT(color_att_img->mtl.tex[color_att_img->cmn.active_slot] != _SG_MTL_INVALID_SLOT_INDEX);
            pass_desc.colorAttachments[i].loadAction = _sg_mtl_load_action(action->colors[i].load_action);
            pass_desc.colorAttachments[i].storeAction = _sg_mtl_store_action(action->colors[i].store_action, resolve_att_img != 0);
            sg_color c = action->colors[i].clear_value;
            pass_desc.colorAttachments[i].clearColor = MTLClearColorMake(c.r, c.g, c.b, c.a);
            pass_desc.colorAttachments[i].texture = _sg_mtl_id(color_att_img->mtl.tex[color_att_img->cmn.active_slot]);
            pass_desc.colorAttachments[i].level = (NSUInteger)cmn_color_att->mip_level;
            switch (color_att_img->cmn.type) {
                case SG_IMAGETYPE_CUBE:
                case SG_IMAGETYPE_ARRAY:
                    pass_desc.colorAttachments[i].slice = (NSUInteger)cmn_color_att->slice;
                    break;
                case SG_IMAGETYPE_3D:
                    pass_desc.colorAttachments[i].depthPlane = (NSUInteger)cmn_color_att->slice;
                    break;
                default: break;
            }
            if (resolve_att_img) {
                SOKOL_ASSERT(resolve_att_img->slot.state == SG_RESOURCESTATE_VALID);
                SOKOL_ASSERT(resolve_att_img->slot.id == cmn_resolve_att->image_id.id);
                SOKOL_ASSERT(resolve_att_img->mtl.tex[resolve_att_img->cmn.active_slot] != _SG_MTL_INVALID_SLOT_INDEX);
                pass_desc.colorAttachments[i].resolveTexture = _sg_mtl_id(resolve_att_img->mtl.tex[resolve_att_img->cmn.active_slot]);
                pass_desc.colorAttachments[i].resolveLevel = (NSUInteger)cmn_resolve_att->mip_level;
                switch (resolve_att_img->cmn.type) {
                    case SG_IMAGETYPE_CUBE:
                    case SG_IMAGETYPE_ARRAY:
                        pass_desc.colorAttachments[i].resolveSlice = (NSUInteger)cmn_resolve_att->slice;
                        break;
                    case SG_IMAGETYPE_3D:
                        pass_desc.colorAttachments[i].resolveDepthPlane = (NSUInteger)cmn_resolve_att->slice;
                        break;
                    default: break;
                }
            }
        }
        const _sg_image_t* ds_att_img = pass->mtl.ds_att.image;
        if (0 != ds_att_img) {
            SOKOL_ASSERT(ds_att_img->slot.state == SG_RESOURCESTATE_VALID);
            SOKOL_ASSERT(ds_att_img->slot.id == pass->cmn.ds_att.image_id.id);
            SOKOL_ASSERT(ds_att_img->mtl.tex[ds_att_img->cmn.active_slot] != _SG_MTL_INVALID_SLOT_INDEX);
            pass_desc.depthAttachment.texture = _sg_mtl_id(ds_att_img->mtl.tex[ds_att_img->cmn.active_slot]);
            pass_desc.depthAttachment.loadAction = _sg_mtl_load_action(action->depth.load_action);
            pass_desc.depthAttachment.storeAction = _sg_mtl_store_action(action->depth.store_action, false);
            pass_desc.depthAttachment.clearDepth = action->depth.clear_value;
            const _sg_pass_attachment_t* cmn_ds_att = &pass->cmn.ds_att;
            switch (ds_att_img->cmn.type) {
                case SG_IMAGETYPE_CUBE:
                case SG_IMAGETYPE_ARRAY:
                    pass_desc.depthAttachment.slice = (NSUInteger)cmn_ds_att->slice;
                    break;
                case SG_IMAGETYPE_3D:
                    pass_desc.depthAttachment.resolveDepthPlane = (NSUInteger)cmn_ds_att->slice;
                    break;
                default: break;
            }
            if (_sg_is_depth_stencil_format(ds_att_img->cmn.pixel_format)) {
                pass_desc.stencilAttachment.texture = _sg_mtl_id(ds_att_img->mtl.tex[ds_att_img->cmn.active_slot]);
                pass_desc.stencilAttachment.loadAction = _sg_mtl_load_action(action->stencil.load_action);
                pass_desc.stencilAttachment.storeAction = _sg_mtl_store_action(action->depth.store_action, false);
                pass_desc.stencilAttachment.clearStencil = action->stencil.clear_value;
                switch (ds_att_img->cmn.type) {
                    case SG_IMAGETYPE_CUBE:
                    case SG_IMAGETYPE_ARRAY:
                        pass_desc.stencilAttachment.slice = (NSUInteger)cmn_ds_att->slice;
                        break;
                    case SG_IMAGETYPE_3D:
                        pass_desc.stencilAttachment.resolveDepthPlane = (NSUInteger)cmn_ds_att->slice;
                        break;
                    default: break;
                }
            }
        }
    } else {
        // setup pass descriptor for default rendering
        pass_desc.colorAttachments[0].loadAction = _sg_mtl_load_action(action->colors[0].load_action);
        sg_color c = action->colors[0].clear_value;
        pass_desc.colorAttachments[0].clearColor = MTLClearColorMake(c.r, c.g, c.b, c.a);
        pass_desc.depthAttachment.loadAction = _sg_mtl_load_action(action->depth.load_action);
        pass_desc.depthAttachment.clearDepth = action->depth.clear_value;
        pass_desc.stencilAttachment.loadAction = _sg_mtl_load_action(action->stencil.load_action);
        pass_desc.stencilAttachment.clearStencil = action->stencil.clear_value;
    }

    // create a render command encoder, this might return nil if window is minimized
    _sg.mtl.cmd_encoder = [_sg.mtl.cmd_buffer renderCommandEncoderWithDescriptor:pass_desc];
    if (nil == _sg.mtl.cmd_encoder) {
        _sg.mtl.pass_valid = false;
        return;
    }

    // bind the global uniform buffer, this only happens once per pass
    _sg_mtl_bind_uniform_buffers();
}

_SOKOL_PRIVATE void _sg_mtl_end_pass(void) {
    SOKOL_ASSERT(_sg.mtl.in_pass);
    _sg.mtl.in_pass = false;
    _sg.mtl.pass_valid = false;
    if (nil != _sg.mtl.cmd_encoder) {
        [_sg.mtl.cmd_encoder endEncoding];
        // NOTE: MTLRenderCommandEncoder is autoreleased
        _sg.mtl.cmd_encoder = nil;
    }
}

_SOKOL_PRIVATE void _sg_mtl_commit(void) {
    SOKOL_ASSERT(!_sg.mtl.in_pass);
    SOKOL_ASSERT(!_sg.mtl.pass_valid);
    SOKOL_ASSERT(_sg.mtl.drawable_cb || _sg.mtl.drawable_userdata_cb);
    SOKOL_ASSERT(nil == _sg.mtl.cmd_encoder);
    SOKOL_ASSERT(nil != _sg.mtl.cmd_buffer);

    // present, commit and signal semaphore when done
    id<MTLDrawable> cur_drawable = nil;
    if (_sg.mtl.drawable_cb) {
        cur_drawable = (__bridge id<MTLDrawable>) _sg.mtl.drawable_cb();
    } else {
        cur_drawable = (__bridge id<MTLDrawable>) _sg.mtl.drawable_userdata_cb(_sg.mtl.user_data);
    }
    if (nil != cur_drawable) {
        [_sg.mtl.cmd_buffer presentDrawable:cur_drawable];
    }
    [_sg.mtl.cmd_buffer commit];

    // garbage-collect resources pending for release
    _sg_mtl_garbage_collect(_sg.frame_index);

    // rotate uniform buffer slot
    if (++_sg.mtl.cur_frame_rotate_index >= SG_NUM_INFLIGHT_FRAMES) {
        _sg.mtl.cur_frame_rotate_index = 0;
    }
    _sg.mtl.cur_ub_offset = 0;
    _sg.mtl.cur_ub_base_ptr = 0;
    // NOTE: MTLCommandBuffer is autoreleased
    _sg.mtl.cmd_buffer = nil;
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
    // clip against framebuffer rect
    const _sg_recti_t clip = _sg_clipi(x, y, w, h, _sg.mtl.cur_width, _sg.mtl.cur_height);
    MTLScissorRect r;
    r.x = (NSUInteger)clip.x;
    r.y = (NSUInteger) (origin_top_left ? clip.y : (_sg.mtl.cur_height - (clip.y + clip.h)));
    r.width = (NSUInteger)clip.w;
    r.height = (NSUInteger)clip.h;
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

    if (_sg.mtl.state_cache.cur_pipeline_id.id != pip->slot.id) {
        _sg.mtl.state_cache.cur_pipeline = pip;
        _sg.mtl.state_cache.cur_pipeline_id.id = pip->slot.id;
        sg_color c = pip->cmn.blend_color;
        [_sg.mtl.cmd_encoder setBlendColorRed:c.r green:c.g blue:c.b alpha:c.a];
        _sg_stats_add(metal.pipeline.num_set_blend_color, 1);
        [_sg.mtl.cmd_encoder setCullMode:pip->mtl.cull_mode];
        _sg_stats_add(metal.pipeline.num_set_cull_mode, 1);
        [_sg.mtl.cmd_encoder setFrontFacingWinding:pip->mtl.winding];
        _sg_stats_add(metal.pipeline.num_set_front_facing_winding, 1);
        [_sg.mtl.cmd_encoder setStencilReferenceValue:pip->mtl.stencil_ref];
        _sg_stats_add(metal.pipeline.num_set_stencil_reference_value, 1);
        [_sg.mtl.cmd_encoder setDepthBias:pip->cmn.depth.bias slopeScale:pip->cmn.depth.bias_slope_scale clamp:pip->cmn.depth.bias_clamp];
        _sg_stats_add(metal.pipeline.num_set_depth_bias, 1);
        SOKOL_ASSERT(pip->mtl.rps != _SG_MTL_INVALID_SLOT_INDEX);
        [_sg.mtl.cmd_encoder setRenderPipelineState:_sg_mtl_id(pip->mtl.rps)];
        _sg_stats_add(metal.pipeline.num_set_render_pipeline_state, 1);
        SOKOL_ASSERT(pip->mtl.dss != _SG_MTL_INVALID_SLOT_INDEX);
        [_sg.mtl.cmd_encoder setDepthStencilState:_sg_mtl_id(pip->mtl.dss)];
        _sg_stats_add(metal.pipeline.num_set_depth_stencil_state, 1);
    }
}

_SOKOL_PRIVATE bool _sg_mtl_apply_bindings(_sg_bindings_t* bnd) {
    SOKOL_ASSERT(bnd);
    SOKOL_ASSERT(bnd->pip);
    SOKOL_ASSERT(_sg.mtl.in_pass);
    if (!_sg.mtl.pass_valid) {
        return false;
    }
    SOKOL_ASSERT(nil != _sg.mtl.cmd_encoder);

    // store index buffer binding, this will be needed later in sg_draw()
    _sg.mtl.state_cache.cur_indexbuffer = bnd->ib;
    _sg.mtl.state_cache.cur_indexbuffer_offset = bnd->ib_offset;
    if (bnd->ib) {
        SOKOL_ASSERT(bnd->pip->cmn.index_type != SG_INDEXTYPE_NONE);
        _sg.mtl.state_cache.cur_indexbuffer_id.id = bnd->ib->slot.id;
    } else {
        SOKOL_ASSERT(bnd->pip->cmn.index_type == SG_INDEXTYPE_NONE);
        _sg.mtl.state_cache.cur_indexbuffer_id.id = SG_INVALID_ID;
    }

    // apply vertex buffers
    for (NSUInteger slot = 0; slot < (NSUInteger)bnd->num_vbs; slot++) {
        const _sg_buffer_t* vb = bnd->vbs[slot];
        const int vb_offset = bnd->vb_offsets[slot];
        if ((_sg.mtl.state_cache.cur_vertexbuffer_ids[slot].id != vb->slot.id) ||
            (_sg.mtl.state_cache.cur_vertexbuffer_offsets[slot] != vb_offset))
        {
            _sg.mtl.state_cache.cur_vertexbuffers[slot] = vb;
            _sg.mtl.state_cache.cur_vertexbuffer_offsets[slot] = vb_offset;
            _sg.mtl.state_cache.cur_vertexbuffer_ids[slot].id = vb->slot.id;
            const NSUInteger mtl_slot = SG_MAX_SHADERSTAGE_UBS + slot;
            SOKOL_ASSERT(vb->mtl.buf[vb->cmn.active_slot] != _SG_MTL_INVALID_SLOT_INDEX);
            [_sg.mtl.cmd_encoder setVertexBuffer:_sg_mtl_id(vb->mtl.buf[vb->cmn.active_slot])
                offset:(NSUInteger)vb_offset
                atIndex:mtl_slot];
            _sg_stats_add(metal.bindings.num_set_vertex_buffer, 1);
        }
    }

    // apply vertex shader images
    for (NSUInteger slot = 0; slot < (NSUInteger)bnd->num_vs_imgs; slot++) {
        const _sg_image_t* img = bnd->vs_imgs[slot];
        if (_sg.mtl.state_cache.cur_vs_image_ids[slot].id != img->slot.id) {
            _sg.mtl.state_cache.cur_vs_images[slot] = img;
            _sg.mtl.state_cache.cur_vs_image_ids[slot].id = img->slot.id;
            SOKOL_ASSERT(img->mtl.tex[img->cmn.active_slot] != _SG_MTL_INVALID_SLOT_INDEX);
            [_sg.mtl.cmd_encoder setVertexTexture:_sg_mtl_id(img->mtl.tex[img->cmn.active_slot]) atIndex:slot];
            _sg_stats_add(metal.bindings.num_set_vertex_texture, 1);
        }
    }

    // apply vertex shader samplers
    for (NSUInteger slot = 0; slot < (NSUInteger)bnd->num_vs_smps; slot++) {
        const _sg_sampler_t* smp = bnd->vs_smps[slot];
        if (_sg.mtl.state_cache.cur_vs_sampler_ids[slot].id != smp->slot.id) {
            _sg.mtl.state_cache.cur_vs_samplers[slot] = smp;
            _sg.mtl.state_cache.cur_vs_sampler_ids[slot].id = smp->slot.id;
            SOKOL_ASSERT(smp->mtl.sampler_state != _SG_MTL_INVALID_SLOT_INDEX);
            [_sg.mtl.cmd_encoder setVertexSamplerState:_sg_mtl_id(smp->mtl.sampler_state) atIndex:slot];
            _sg_stats_add(metal.bindings.num_set_vertex_sampler_state, 1);
        }
    }

    // apply fragment shader images
    for (NSUInteger slot = 0; slot < (NSUInteger)bnd->num_fs_imgs; slot++) {
        const _sg_image_t* img = bnd->fs_imgs[slot];
        if (_sg.mtl.state_cache.cur_fs_image_ids[slot].id != img->slot.id) {
            _sg.mtl.state_cache.cur_fs_images[slot] = img;
            _sg.mtl.state_cache.cur_fs_image_ids[slot].id = img->slot.id;
            SOKOL_ASSERT(img->mtl.tex[img->cmn.active_slot] != _SG_MTL_INVALID_SLOT_INDEX);
            [_sg.mtl.cmd_encoder setFragmentTexture:_sg_mtl_id(img->mtl.tex[img->cmn.active_slot]) atIndex:slot];
            _sg_stats_add(metal.bindings.num_set_fragment_texture, 1);
        }
    }

    // apply fragment shader samplers
    for (NSUInteger slot = 0; slot < (NSUInteger)bnd->num_fs_smps; slot++) {
        const _sg_sampler_t* smp = bnd->fs_smps[slot];
        if (_sg.mtl.state_cache.cur_fs_sampler_ids[slot].id != smp->slot.id) {
            _sg.mtl.state_cache.cur_fs_samplers[slot] = smp;
            _sg.mtl.state_cache.cur_fs_sampler_ids[slot].id = smp->slot.id;
            SOKOL_ASSERT(smp->mtl.sampler_state != _SG_MTL_INVALID_SLOT_INDEX);
            [_sg.mtl.cmd_encoder setFragmentSamplerState:_sg_mtl_id(smp->mtl.sampler_state) atIndex:slot];
            _sg_stats_add(metal.bindings.num_set_fragment_sampler_state, 1);
        }
    }
    return true;
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
    SOKOL_ASSERT(data->size == _sg.mtl.state_cache.cur_pipeline->shader->cmn.stage[stage_index].uniform_blocks[ub_index].size);

    // copy to global uniform buffer, record offset into cmd encoder, and advance offset
    uint8_t* dst = &_sg.mtl.cur_ub_base_ptr[_sg.mtl.cur_ub_offset];
    memcpy(dst, data->ptr, data->size);
    if (stage_index == SG_SHADERSTAGE_VS) {
        [_sg.mtl.cmd_encoder setVertexBufferOffset:(NSUInteger)_sg.mtl.cur_ub_offset atIndex:(NSUInteger)ub_index];
        _sg_stats_add(metal.uniforms.num_set_vertex_buffer_offset, 1);
    } else {
        [_sg.mtl.cmd_encoder setFragmentBufferOffset:(NSUInteger)_sg.mtl.cur_ub_offset atIndex:(NSUInteger)ub_index];
        _sg_stats_add(metal.uniforms.num_set_fragment_buffer_offset, 1);
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
        // indexed rendering
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
    } else {
        // non-indexed rendering
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
    if (_sg_mtl_resource_options_storage_mode_managed_or_shared() == MTLStorageModeManaged) {
        [mtl_buf didModifyRange:NSMakeRange(0, data->size)];
    }
    #endif
}

_SOKOL_PRIVATE void _sg_mtl_append_buffer(_sg_buffer_t* buf, const sg_range* data, bool new_frame) {
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
    if (_sg_mtl_resource_options_storage_mode_managed_or_shared() == MTLStorageModeManaged) {
        [mtl_buf didModifyRange:NSMakeRange((NSUInteger)buf->cmn.append_pos, (NSUInteger)data->size)];
    }
    #endif
}

_SOKOL_PRIVATE void _sg_mtl_update_image(_sg_image_t* img, const sg_image_data* data) {
    SOKOL_ASSERT(img && data);
    if (++img->cmn.active_slot >= img->cmn.num_slots) {
        img->cmn.active_slot = 0;
    }
    __unsafe_unretained id<MTLTexture> mtl_tex = _sg_mtl_id(img->mtl.tex[img->cmn.active_slot]);
    _sg_mtl_copy_image_data(img, mtl_tex, data);
}

_SOKOL_PRIVATE void _sg_mtl_push_debug_group(const char* name) {
    SOKOL_ASSERT(name);
    if (_sg.mtl.cmd_encoder) {
        [_sg.mtl.cmd_encoder pushDebugGroup:[NSString stringWithUTF8String:name]];
    }
}

_SOKOL_PRIVATE void _sg_mtl_pop_debug_group(void) {
    if (_sg.mtl.cmd_encoder) {
        [_sg.mtl.cmd_encoder popDebugGroup];
    }
}

// ██     ██ ███████ ██████   ██████  ██████  ██    ██     ██████   █████   ██████ ██   ██ ███████ ███    ██ ██████
// ██     ██ ██      ██   ██ ██       ██   ██ ██    ██     ██   ██ ██   ██ ██      ██  ██  ██      ████   ██ ██   ██
// ██  █  ██ █████   ██████  ██   ███ ██████  ██    ██     ██████  ███████ ██      █████   █████   ██ ██  ██ ██   ██
// ██ ███ ██ ██      ██   ██ ██    ██ ██      ██    ██     ██   ██ ██   ██ ██      ██  ██  ██      ██  ██ ██ ██   ██
//  ███ ███  ███████ ██████   ██████  ██       ██████      ██████  ██   ██  ██████ ██   ██ ███████ ██   ████ ██████
//
// >>webgpu
// >>wgpu
#elif defined(SOKOL_WGPU)

_SOKOL_PRIVATE WGPUBufferUsageFlags _sg_wgpu_buffer_usage(sg_buffer_type t, sg_usage u) {
    WGPUBufferUsageFlags res = 0;
    if (SG_BUFFERTYPE_VERTEXBUFFER == t) {
        res |= WGPUBufferUsage_Vertex;
    } else {
        res |= WGPUBufferUsage_Index;
    }
    if (SG_USAGE_IMMUTABLE != u) {
        res |= WGPUBufferUsage_CopyDst;
    }
    return res;
}

_SOKOL_PRIVATE WGPULoadOp _sg_wgpu_load_op(WGPUTextureView view, sg_load_action a) {
    if (0 == view) {
        return WGPULoadOp_Undefined;
    } else switch (a) {
        case SG_LOADACTION_CLEAR:
        case SG_LOADACTION_DONTCARE:
            return WGPULoadOp_Clear;
        case SG_LOADACTION_LOAD:
            return WGPULoadOp_Load;
        default:
            SOKOL_UNREACHABLE;
            return WGPULoadOp_Force32;
    }
}

_SOKOL_PRIVATE WGPUStoreOp _sg_wgpu_store_op(WGPUTextureView view, sg_store_action a) {
    if (0 == view) {
        return WGPUStoreOp_Undefined;
    } else switch (a) {
        case SG_STOREACTION_STORE:
            return WGPUStoreOp_Store;
        case SG_STOREACTION_DONTCARE:
            return WGPUStoreOp_Discard;
        default:
            SOKOL_UNREACHABLE;
            return WGPUStoreOp_Force32;
    }
}

_SOKOL_PRIVATE WGPUTextureViewDimension _sg_wgpu_texture_view_dimension(sg_image_type t) {
    switch (t) {
        case SG_IMAGETYPE_2D:       return WGPUTextureViewDimension_2D;
        case SG_IMAGETYPE_CUBE:     return WGPUTextureViewDimension_Cube;
        case SG_IMAGETYPE_3D:       return WGPUTextureViewDimension_3D;
        case SG_IMAGETYPE_ARRAY:    return WGPUTextureViewDimension_2DArray;
        default: SOKOL_UNREACHABLE; return WGPUTextureViewDimension_Force32;
    }
}

_SOKOL_PRIVATE WGPUTextureDimension _sg_wgpu_texture_dimension(sg_image_type t) {
    if (SG_IMAGETYPE_3D == t) {
        return WGPUTextureDimension_3D;
    } else {
        return WGPUTextureDimension_2D;
    }
}

_SOKOL_PRIVATE WGPUTextureSampleType _sg_wgpu_texture_sample_type(sg_image_sample_type t) {
    switch (t) {
        case SG_IMAGESAMPLETYPE_FLOAT:  return WGPUTextureSampleType_Float;
        case SG_IMAGESAMPLETYPE_DEPTH:  return WGPUTextureSampleType_Depth;
        case SG_IMAGESAMPLETYPE_SINT:   return WGPUTextureSampleType_Sint;
        case SG_IMAGESAMPLETYPE_UINT:   return WGPUTextureSampleType_Uint;
        case SG_IMAGESAMPLETYPE_UNFILTERABLE_FLOAT: return WGPUTextureSampleType_UnfilterableFloat;
        default: SOKOL_UNREACHABLE;     return WGPUTextureSampleType_Force32;
    }
}

_SOKOL_PRIVATE WGPUSamplerBindingType _sg_wgpu_sampler_binding_type(sg_sampler_type t) {
    switch (t) {
        case SG_SAMPLERTYPE_FILTERING: return WGPUSamplerBindingType_Filtering;
        case SG_SAMPLERTYPE_COMPARISON: return WGPUSamplerBindingType_Comparison;
        case SG_SAMPLERTYPE_NONFILTERING: return WGPUSamplerBindingType_NonFiltering;
        default: SOKOL_UNREACHABLE; return WGPUSamplerBindingType_Force32;
    }
}

_SOKOL_PRIVATE WGPUAddressMode _sg_wgpu_sampler_address_mode(sg_wrap m) {
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

_SOKOL_PRIVATE WGPUFilterMode _sg_wgpu_sampler_minmag_filter(sg_filter f) {
    switch (f) {
        case SG_FILTER_NEAREST:
            return WGPUFilterMode_Nearest;
        case SG_FILTER_LINEAR:
            return WGPUFilterMode_Linear;
        default:
            SOKOL_UNREACHABLE;
            return WGPUFilterMode_Force32;
    }
}

_SOKOL_PRIVATE WGPUMipmapFilterMode _sg_wgpu_sampler_mipmap_filter(sg_filter f) {
    switch (f) {
        case SG_FILTER_NONE:
        case SG_FILTER_NEAREST:
            return WGPUMipmapFilterMode_Nearest;
        case SG_FILTER_LINEAR:
            return WGPUMipmapFilterMode_Linear;
        default:
            SOKOL_UNREACHABLE;
            return WGPUMipmapFilterMode_Force32;
    }
}

_SOKOL_PRIVATE WGPUIndexFormat _sg_wgpu_indexformat(sg_index_type t) {
    // NOTE: there's no WGPUIndexFormat_None
    return (t == SG_INDEXTYPE_UINT16) ? WGPUIndexFormat_Uint16 : WGPUIndexFormat_Uint32;
}

_SOKOL_PRIVATE WGPUIndexFormat _sg_wgpu_stripindexformat(sg_primitive_type prim_type, sg_index_type idx_type) {
    if (idx_type == SG_INDEXTYPE_NONE) {
        return WGPUIndexFormat_Undefined;
    } else if ((prim_type == SG_PRIMITIVETYPE_LINE_STRIP) || (prim_type == SG_PRIMITIVETYPE_TRIANGLE_STRIP)) {
        return _sg_wgpu_indexformat(idx_type);
    } else {
        return WGPUIndexFormat_Undefined;
    }
}

_SOKOL_PRIVATE WGPUVertexStepMode _sg_wgpu_stepmode(sg_vertex_step s) {
    return (s == SG_VERTEXSTEP_PER_VERTEX) ? WGPUVertexStepMode_Vertex : WGPUVertexStepMode_Instance;
}

_SOKOL_PRIVATE WGPUVertexFormat _sg_wgpu_vertexformat(sg_vertex_format f) {
    switch (f) {
        case SG_VERTEXFORMAT_FLOAT:         return WGPUVertexFormat_Float32;
        case SG_VERTEXFORMAT_FLOAT2:        return WGPUVertexFormat_Float32x2;
        case SG_VERTEXFORMAT_FLOAT3:        return WGPUVertexFormat_Float32x3;
        case SG_VERTEXFORMAT_FLOAT4:        return WGPUVertexFormat_Float32x4;
        case SG_VERTEXFORMAT_BYTE4:         return WGPUVertexFormat_Sint8x4;
        case SG_VERTEXFORMAT_BYTE4N:        return WGPUVertexFormat_Snorm8x4;
        case SG_VERTEXFORMAT_UBYTE4:        return WGPUVertexFormat_Uint8x4;
        case SG_VERTEXFORMAT_UBYTE4N:       return WGPUVertexFormat_Unorm8x4;
        case SG_VERTEXFORMAT_SHORT2:        return WGPUVertexFormat_Sint16x2;
        case SG_VERTEXFORMAT_SHORT2N:       return WGPUVertexFormat_Snorm16x2;
        case SG_VERTEXFORMAT_USHORT2N:      return WGPUVertexFormat_Unorm16x2;
        case SG_VERTEXFORMAT_SHORT4:        return WGPUVertexFormat_Sint16x4;
        case SG_VERTEXFORMAT_SHORT4N:       return WGPUVertexFormat_Snorm16x4;
        case SG_VERTEXFORMAT_USHORT4N:      return WGPUVertexFormat_Unorm16x4;
        case SG_VERTEXFORMAT_HALF2:         return WGPUVertexFormat_Float16x2;
        case SG_VERTEXFORMAT_HALF4:         return WGPUVertexFormat_Float16x4;
        // FIXME! UINT10_N2 (see https://github.com/gpuweb/gpuweb/issues/4275)
        case SG_VERTEXFORMAT_UINT10_N2:     return WGPUVertexFormat_Undefined;
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
        default:
            SOKOL_UNREACHABLE;
            return WGPUPrimitiveTopology_Force32;
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
        default:
            SOKOL_UNREACHABLE;
            return WGPUCullMode_Force32;
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
        case SG_PIXELFORMAT_SRGB8A8:        return WGPUTextureFormat_RGBA8UnormSrgb;
        case SG_PIXELFORMAT_RGBA8SN:        return WGPUTextureFormat_RGBA8Snorm;
        case SG_PIXELFORMAT_RGBA8UI:        return WGPUTextureFormat_RGBA8Uint;
        case SG_PIXELFORMAT_RGBA8SI:        return WGPUTextureFormat_RGBA8Sint;
        case SG_PIXELFORMAT_BGRA8:          return WGPUTextureFormat_BGRA8Unorm;
        case SG_PIXELFORMAT_RGB10A2:        return WGPUTextureFormat_RGB10A2Unorm;
        case SG_PIXELFORMAT_RG11B10F:       return WGPUTextureFormat_RG11B10Ufloat;
        case SG_PIXELFORMAT_RG32UI:         return WGPUTextureFormat_RG32Uint;
        case SG_PIXELFORMAT_RG32SI:         return WGPUTextureFormat_RG32Sint;
        case SG_PIXELFORMAT_RG32F:          return WGPUTextureFormat_RG32Float;
        case SG_PIXELFORMAT_RGBA16UI:       return WGPUTextureFormat_RGBA16Uint;
        case SG_PIXELFORMAT_RGBA16SI:       return WGPUTextureFormat_RGBA16Sint;
        case SG_PIXELFORMAT_RGBA16F:        return WGPUTextureFormat_RGBA16Float;
        case SG_PIXELFORMAT_RGBA32UI:       return WGPUTextureFormat_RGBA32Uint;
        case SG_PIXELFORMAT_RGBA32SI:       return WGPUTextureFormat_RGBA32Sint;
        case SG_PIXELFORMAT_RGBA32F:        return WGPUTextureFormat_RGBA32Float;
        case SG_PIXELFORMAT_DEPTH:          return WGPUTextureFormat_Depth32Float;
        case SG_PIXELFORMAT_DEPTH_STENCIL:  return WGPUTextureFormat_Depth32FloatStencil8;
        case SG_PIXELFORMAT_BC1_RGBA:       return WGPUTextureFormat_BC1RGBAUnorm;
        case SG_PIXELFORMAT_BC2_RGBA:       return WGPUTextureFormat_BC2RGBAUnorm;
        case SG_PIXELFORMAT_BC3_RGBA:       return WGPUTextureFormat_BC3RGBAUnorm;
        case SG_PIXELFORMAT_BC4_R:          return WGPUTextureFormat_BC4RUnorm;
        case SG_PIXELFORMAT_BC4_RSN:        return WGPUTextureFormat_BC4RSnorm;
        case SG_PIXELFORMAT_BC5_RG:         return WGPUTextureFormat_BC5RGUnorm;
        case SG_PIXELFORMAT_BC5_RGSN:       return WGPUTextureFormat_BC5RGSnorm;
        case SG_PIXELFORMAT_BC6H_RGBF:      return WGPUTextureFormat_BC6HRGBFloat;
        case SG_PIXELFORMAT_BC6H_RGBUF:     return WGPUTextureFormat_BC6HRGBUfloat;
        case SG_PIXELFORMAT_BC7_RGBA:       return WGPUTextureFormat_BC7RGBAUnorm;
        case SG_PIXELFORMAT_ETC2_RGB8:      return WGPUTextureFormat_ETC2RGB8Unorm;
        case SG_PIXELFORMAT_ETC2_RGB8A1:    return WGPUTextureFormat_ETC2RGB8A1Unorm;
        case SG_PIXELFORMAT_ETC2_RGBA8:     return WGPUTextureFormat_ETC2RGBA8Unorm;
        case SG_PIXELFORMAT_ETC2_RG11:      return WGPUTextureFormat_EACR11Unorm;
        case SG_PIXELFORMAT_ETC2_RG11SN:    return WGPUTextureFormat_EACR11Snorm;
        case SG_PIXELFORMAT_RGB9E5:         return WGPUTextureFormat_RGB9E5Ufloat;

        // NOT SUPPORTED
        case SG_PIXELFORMAT_R16:
        case SG_PIXELFORMAT_R16SN:
        case SG_PIXELFORMAT_RG16:
        case SG_PIXELFORMAT_RG16SN:
        case SG_PIXELFORMAT_RGBA16:
        case SG_PIXELFORMAT_RGBA16SN:
        case SG_PIXELFORMAT_PVRTC_RGB_2BPP:
        case SG_PIXELFORMAT_PVRTC_RGB_4BPP:
        case SG_PIXELFORMAT_PVRTC_RGBA_2BPP:
        case SG_PIXELFORMAT_PVRTC_RGBA_4BPP:
            return WGPUTextureFormat_Undefined;

        default:
            SOKOL_UNREACHABLE;
            return WGPUTextureFormat_Force32;
    }
}

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
        default:
            SOKOL_UNREACHABLE;
            return WGPUCompareFunction_Force32;
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
        default:
            SOKOL_UNREACHABLE;
            return WGPUStencilOperation_Force32;
    }
}

_SOKOL_PRIVATE WGPUBlendOperation _sg_wgpu_blendop(sg_blend_op op) {
    switch (op) {
        case SG_BLENDOP_ADD:                return WGPUBlendOperation_Add;
        case SG_BLENDOP_SUBTRACT:           return WGPUBlendOperation_Subtract;
        case SG_BLENDOP_REVERSE_SUBTRACT:   return WGPUBlendOperation_ReverseSubtract;
        default:
            SOKOL_UNREACHABLE;
            return WGPUBlendOperation_Force32;
    }
}

_SOKOL_PRIVATE WGPUBlendFactor _sg_wgpu_blendfactor(sg_blend_factor f) {
    switch (f) {
        case SG_BLENDFACTOR_ZERO:                   return WGPUBlendFactor_Zero;
        case SG_BLENDFACTOR_ONE:                    return WGPUBlendFactor_One;
        case SG_BLENDFACTOR_SRC_COLOR:              return WGPUBlendFactor_Src;
        case SG_BLENDFACTOR_ONE_MINUS_SRC_COLOR:    return WGPUBlendFactor_OneMinusSrc;
        case SG_BLENDFACTOR_SRC_ALPHA:              return WGPUBlendFactor_SrcAlpha;
        case SG_BLENDFACTOR_ONE_MINUS_SRC_ALPHA:    return WGPUBlendFactor_OneMinusSrcAlpha;
        case SG_BLENDFACTOR_DST_COLOR:              return WGPUBlendFactor_Dst;
        case SG_BLENDFACTOR_ONE_MINUS_DST_COLOR:    return WGPUBlendFactor_OneMinusDst;
        case SG_BLENDFACTOR_DST_ALPHA:              return WGPUBlendFactor_DstAlpha;
        case SG_BLENDFACTOR_ONE_MINUS_DST_ALPHA:    return WGPUBlendFactor_OneMinusDstAlpha;
        case SG_BLENDFACTOR_SRC_ALPHA_SATURATED:    return WGPUBlendFactor_SrcAlphaSaturated;
        case SG_BLENDFACTOR_BLEND_COLOR:            return WGPUBlendFactor_Constant;
        case SG_BLENDFACTOR_ONE_MINUS_BLEND_COLOR:  return WGPUBlendFactor_OneMinusConstant;
        // FIXME: separate blend alpha value not supported?
        case SG_BLENDFACTOR_BLEND_ALPHA:            return WGPUBlendFactor_Constant;
        case SG_BLENDFACTOR_ONE_MINUS_BLEND_ALPHA:  return WGPUBlendFactor_OneMinusConstant;
        default:
            SOKOL_UNREACHABLE;
            return WGPUBlendFactor_Force32;
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

// image/sampler binding on wgpu follows this convention:
//
//  - all images and sampler are in @group(1)
//  - vertex stage images start at @binding(0)
//  - vertex stage samplers start at @binding(16)
//  - fragment stage images start at @binding(32)
//  - fragment stage samplers start at @binding(48)
//
_SOKOL_PRIVATE uint32_t _sg_wgpu_image_binding(sg_shader_stage stage, int img_slot) {
    SOKOL_ASSERT((img_slot >= 0) && (img_slot < 16));
    if (SG_SHADERSTAGE_VS == stage) {
        return 0 + (uint32_t)img_slot;
    } else {
        return 32 + (uint32_t)img_slot;
    }
}

_SOKOL_PRIVATE uint32_t _sg_wgpu_sampler_binding(sg_shader_stage stage, int smp_slot) {
    SOKOL_ASSERT((smp_slot >= 0) && (smp_slot < 16));
    if (SG_SHADERSTAGE_VS == stage) {
        return 16 + (uint32_t)smp_slot;
    } else {
        return 48 + (uint32_t)smp_slot;
    }
}

_SOKOL_PRIVATE WGPUShaderStage _sg_wgpu_shader_stage(sg_shader_stage stage) {
    switch (stage) {
        case SG_SHADERSTAGE_VS: return WGPUShaderStage_Vertex;
        case SG_SHADERSTAGE_FS: return WGPUShaderStage_Fragment;
        default: SOKOL_UNREACHABLE; return WGPUShaderStage_None;
    }
}

_SOKOL_PRIVATE void _sg_wgpu_init_caps(void) {
    _sg.backend = SG_BACKEND_WGPU;
    _sg.features.origin_top_left = true;
    _sg.features.image_clamp_to_border = false;
    _sg.features.mrt_independent_blend_state = true;
    _sg.features.mrt_independent_write_mask = true;

    // FIXME: in webgpu.h, wgpuDeviceGetLimits() has a boolean return value, but
    // the JS shim doesn't actually return anything
    // (see: https://github.com/emscripten-core/emscripten/issues/20278)
    wgpuDeviceGetLimits(_sg.wgpu.dev, &_sg.wgpu.limits);

    const WGPULimits* l = &_sg.wgpu.limits.limits;
    _sg.limits.max_image_size_2d = (int) l->maxTextureDimension2D;
    _sg.limits.max_image_size_cube = (int) l->maxTextureDimension2D; // not a bug, see: https://github.com/gpuweb/gpuweb/issues/1327
    _sg.limits.max_image_size_3d = (int) l->maxTextureDimension3D;
    _sg.limits.max_image_size_array = (int) l->maxTextureDimension2D;
    _sg.limits.max_image_array_layers = (int) l->maxTextureArrayLayers;
    _sg.limits.max_vertex_attrs = SG_MAX_VERTEX_ATTRIBUTES;

    // NOTE: no WGPUTextureFormat_R16Unorm
    _sg_pixelformat_all(&_sg.formats[SG_PIXELFORMAT_R8]);
    _sg_pixelformat_all(&_sg.formats[SG_PIXELFORMAT_RG8]);
    _sg_pixelformat_all(&_sg.formats[SG_PIXELFORMAT_RGBA8]);
    _sg_pixelformat_all(&_sg.formats[SG_PIXELFORMAT_SRGB8A8]);
    _sg_pixelformat_all(&_sg.formats[SG_PIXELFORMAT_BGRA8]);
    _sg_pixelformat_all(&_sg.formats[SG_PIXELFORMAT_R16F]);
    _sg_pixelformat_all(&_sg.formats[SG_PIXELFORMAT_RG16F]);
    _sg_pixelformat_all(&_sg.formats[SG_PIXELFORMAT_RGBA16F]);
    _sg_pixelformat_all(&_sg.formats[SG_PIXELFORMAT_RGB10A2]);

    _sg_pixelformat_sf(&_sg.formats[SG_PIXELFORMAT_R8SN]);
    _sg_pixelformat_sf(&_sg.formats[SG_PIXELFORMAT_RG8SN]);
    _sg_pixelformat_sf(&_sg.formats[SG_PIXELFORMAT_RGBA8SN]);

    // FIXME: can be made renderable via extension
    _sg_pixelformat_sf(&_sg.formats[SG_PIXELFORMAT_RG11B10F]);

    // NOTE: msaa rendering is possible in WebGPU, but no resolve
    // which is a combination that's not currently supported in sokol-gfx
    _sg_pixelformat_sr(&_sg.formats[SG_PIXELFORMAT_R8UI]);
    _sg_pixelformat_sr(&_sg.formats[SG_PIXELFORMAT_R8SI]);
    _sg_pixelformat_sr(&_sg.formats[SG_PIXELFORMAT_RG8UI]);
    _sg_pixelformat_sr(&_sg.formats[SG_PIXELFORMAT_RG8SI]);
    _sg_pixelformat_sr(&_sg.formats[SG_PIXELFORMAT_RGBA8UI]);
    _sg_pixelformat_sr(&_sg.formats[SG_PIXELFORMAT_RGBA8SI]);
    _sg_pixelformat_sr(&_sg.formats[SG_PIXELFORMAT_R16UI]);
    _sg_pixelformat_sr(&_sg.formats[SG_PIXELFORMAT_R16SI]);
    _sg_pixelformat_sr(&_sg.formats[SG_PIXELFORMAT_RG16UI]);
    _sg_pixelformat_sr(&_sg.formats[SG_PIXELFORMAT_RG16SI]);
    _sg_pixelformat_sr(&_sg.formats[SG_PIXELFORMAT_RGBA16UI]);
    _sg_pixelformat_sr(&_sg.formats[SG_PIXELFORMAT_RGBA16SI]);
    _sg_pixelformat_sr(&_sg.formats[SG_PIXELFORMAT_R32UI]);
    _sg_pixelformat_sr(&_sg.formats[SG_PIXELFORMAT_R32SI]);
    _sg_pixelformat_sr(&_sg.formats[SG_PIXELFORMAT_RG32UI]);
    _sg_pixelformat_sr(&_sg.formats[SG_PIXELFORMAT_RG32SI]);
    _sg_pixelformat_sr(&_sg.formats[SG_PIXELFORMAT_RGBA32UI]);
    _sg_pixelformat_sr(&_sg.formats[SG_PIXELFORMAT_RGBA32SI]);

    // FIXME: can be made filterable with extension
    _sg_pixelformat_sr(&_sg.formats[SG_PIXELFORMAT_R32F]);
    _sg_pixelformat_sr(&_sg.formats[SG_PIXELFORMAT_RG32F]);
    _sg_pixelformat_sr(&_sg.formats[SG_PIXELFORMAT_RGBA32F]);

    _sg_pixelformat_srmd(&_sg.formats[SG_PIXELFORMAT_DEPTH]);
    _sg_pixelformat_srmd(&_sg.formats[SG_PIXELFORMAT_DEPTH_STENCIL]);

    _sg_pixelformat_sf(&_sg.formats[SG_PIXELFORMAT_RGB9E5]);

    if (wgpuDeviceHasFeature(_sg.wgpu.dev, WGPUFeatureName_TextureCompressionBC)) {
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
    if (wgpuDeviceHasFeature(_sg.wgpu.dev, WGPUFeatureName_TextureCompressionETC2)) {
        _sg_pixelformat_sf(&_sg.formats[SG_PIXELFORMAT_ETC2_RGB8]);
        _sg_pixelformat_sf(&_sg.formats[SG_PIXELFORMAT_ETC2_RGB8A1]);
        _sg_pixelformat_sf(&_sg.formats[SG_PIXELFORMAT_ETC2_RGBA8]);
        _sg_pixelformat_sf(&_sg.formats[SG_PIXELFORMAT_ETC2_RG11]);
        _sg_pixelformat_sf(&_sg.formats[SG_PIXELFORMAT_ETC2_RG11SN]);
    }
}

_SOKOL_PRIVATE void _sg_wgpu_uniform_buffer_init(const sg_desc* desc) {
    SOKOL_ASSERT(0 == _sg.wgpu.uniform.staging);
    SOKOL_ASSERT(0 == _sg.wgpu.uniform.buf);
    SOKOL_ASSERT(0 == _sg.wgpu.uniform.bind.group_layout);
    SOKOL_ASSERT(0 == _sg.wgpu.uniform.bind.group);

    // Add the max-uniform-update size (64 KB) to the requested buffer size,
    // this is to prevent validation errors in the WebGPU implementation
    // if the entire buffer size is used per frame. 64 KB is the allowed
    // max uniform update size on NVIDIA
    //
    // FIXME: is this still needed?
    _sg.wgpu.uniform.num_bytes = (uint32_t)(desc->uniform_buffer_size + _SG_WGPU_MAX_UNIFORM_UPDATE_SIZE);
    _sg.wgpu.uniform.staging = (uint8_t*)_sg_malloc(_sg.wgpu.uniform.num_bytes);

    WGPUBufferDescriptor ub_desc;
    _sg_clear(&ub_desc, sizeof(ub_desc));
    ub_desc.size = _sg.wgpu.uniform.num_bytes;
    ub_desc.usage = WGPUBufferUsage_Uniform|WGPUBufferUsage_CopyDst;
    _sg.wgpu.uniform.buf = wgpuDeviceCreateBuffer(_sg.wgpu.dev, &ub_desc);
    SOKOL_ASSERT(_sg.wgpu.uniform.buf);

    WGPUBindGroupLayoutEntry ub_bgle_desc[SG_NUM_SHADER_STAGES][SG_MAX_SHADERSTAGE_UBS];
    _sg_clear(ub_bgle_desc, sizeof(ub_bgle_desc));
    for (uint32_t stage_index = 0; stage_index < SG_NUM_SHADER_STAGES; stage_index++) {
        WGPUShaderStage vis = (stage_index == SG_SHADERSTAGE_VS) ? WGPUShaderStage_Vertex : WGPUShaderStage_Fragment;
        for (uint32_t ub_index = 0; ub_index < SG_MAX_SHADERSTAGE_UBS; ub_index++) {
            uint32_t bind_index = stage_index * SG_MAX_SHADERSTAGE_UBS + ub_index;
            ub_bgle_desc[stage_index][ub_index].binding = bind_index;
            ub_bgle_desc[stage_index][ub_index].visibility = vis;
            ub_bgle_desc[stage_index][ub_index].buffer.type = WGPUBufferBindingType_Uniform;
            ub_bgle_desc[stage_index][ub_index].buffer.hasDynamicOffset = true;
        }
    }

    WGPUBindGroupLayoutDescriptor ub_bgl_desc;
    _sg_clear(&ub_bgl_desc, sizeof(ub_bgl_desc));
    ub_bgl_desc.entryCount = SG_NUM_SHADER_STAGES * SG_MAX_SHADERSTAGE_UBS;
    ub_bgl_desc.entries = &ub_bgle_desc[0][0];
    _sg.wgpu.uniform.bind.group_layout = wgpuDeviceCreateBindGroupLayout(_sg.wgpu.dev, &ub_bgl_desc);
    SOKOL_ASSERT(_sg.wgpu.uniform.bind.group_layout);

    WGPUBindGroupEntry ub_bge[SG_NUM_SHADER_STAGES][SG_MAX_SHADERSTAGE_UBS];
    _sg_clear(ub_bge, sizeof(ub_bge));
    for (uint32_t stage_index = 0; stage_index < SG_NUM_SHADER_STAGES; stage_index++) {
        for (uint32_t ub_index = 0; ub_index < SG_MAX_SHADERSTAGE_UBS; ub_index++) {
            uint32_t bind_index = stage_index * SG_MAX_SHADERSTAGE_UBS + ub_index;
            ub_bge[stage_index][ub_index].binding = bind_index;
            ub_bge[stage_index][ub_index].buffer = _sg.wgpu.uniform.buf;
            ub_bge[stage_index][ub_index].size = _SG_WGPU_MAX_UNIFORM_UPDATE_SIZE;
        }
    }
    WGPUBindGroupDescriptor bg_desc;
    _sg_clear(&bg_desc, sizeof(bg_desc));
    bg_desc.layout = _sg.wgpu.uniform.bind.group_layout;
    bg_desc.entryCount = SG_NUM_SHADER_STAGES * SG_MAX_SHADERSTAGE_UBS;
    bg_desc.entries = &ub_bge[0][0];
    _sg.wgpu.uniform.bind.group = wgpuDeviceCreateBindGroup(_sg.wgpu.dev, &bg_desc);
    SOKOL_ASSERT(_sg.wgpu.uniform.bind.group);
}

_SOKOL_PRIVATE void _sg_wgpu_uniform_buffer_discard(void) {
    if (_sg.wgpu.uniform.buf) {
        wgpuBufferRelease(_sg.wgpu.uniform.buf);
        _sg.wgpu.uniform.buf = 0;
    }
    if (_sg.wgpu.uniform.bind.group) {
        wgpuBindGroupRelease(_sg.wgpu.uniform.bind.group);
        _sg.wgpu.uniform.bind.group = 0;
    }
    if (_sg.wgpu.uniform.bind.group_layout) {
        wgpuBindGroupLayoutRelease(_sg.wgpu.uniform.bind.group_layout);
        _sg.wgpu.uniform.bind.group_layout = 0;
    }
    if (_sg.wgpu.uniform.staging) {
        _sg_free(_sg.wgpu.uniform.staging);
        _sg.wgpu.uniform.staging = 0;
    }
}

_SOKOL_PRIVATE void _sg_wgpu_uniform_buffer_on_begin_pass(void) {
    wgpuRenderPassEncoderSetBindGroup(_sg.wgpu.pass_enc,
                                      0, // groupIndex 0 is reserved for uniform buffers
                                      _sg.wgpu.uniform.bind.group,
                                      SG_NUM_SHADER_STAGES * SG_MAX_SHADERSTAGE_UBS,
                                      &_sg.wgpu.uniform.bind.offsets[0][0]);
}

_SOKOL_PRIVATE void _sg_wgpu_uniform_buffer_on_commit(void) {
    wgpuQueueWriteBuffer(_sg.wgpu.queue, _sg.wgpu.uniform.buf, 0, _sg.wgpu.uniform.staging, _sg.wgpu.uniform.offset);
    _sg_stats_add(wgpu.uniforms.size_write_buffer, _sg.wgpu.uniform.offset);
    _sg.wgpu.uniform.offset = 0;
    _sg_clear(&_sg.wgpu.uniform.bind.offsets[0][0], sizeof(_sg.wgpu.uniform.bind.offsets));
}

_SOKOL_PRIVATE void _sg_wgpu_bindgroups_pool_init(const sg_desc* desc) {
    SOKOL_ASSERT((desc->wgpu_bindgroups_cache_size > 0) && (desc->wgpu_bindgroups_cache_size < _SG_MAX_POOL_SIZE));
    _sg_wgpu_bindgroups_pool_t* p = &_sg.wgpu.bindgroups_pool;
    SOKOL_ASSERT(0 == p->bindgroups);
    const int pool_size = desc->wgpu_bindgroups_cache_size;
    _sg_init_pool(&p->pool, pool_size);
    size_t pool_byte_size = sizeof(_sg_wgpu_bindgroup_t) * (size_t)p->pool.size;
    p->bindgroups = (_sg_wgpu_bindgroup_t*) _sg_malloc_clear(pool_byte_size);
}

_SOKOL_PRIVATE void _sg_wgpu_bindgroups_pool_discard(void) {
    _sg_wgpu_bindgroups_pool_t* p = &_sg.wgpu.bindgroups_pool;
    SOKOL_ASSERT(p->bindgroups);
    _sg_free(p->bindgroups); p->bindgroups = 0;
    _sg_discard_pool(&p->pool);
}

_SOKOL_PRIVATE _sg_wgpu_bindgroup_t* _sg_wgpu_bindgroup_at(uint32_t bg_id) {
    SOKOL_ASSERT(SG_INVALID_ID != bg_id);
    _sg_wgpu_bindgroups_pool_t* p = &_sg.wgpu.bindgroups_pool;
    int slot_index = _sg_slot_index(bg_id);
    SOKOL_ASSERT((slot_index > _SG_INVALID_SLOT_INDEX) && (slot_index < p->pool.size));
    return &p->bindgroups[slot_index];
}

_SOKOL_PRIVATE _sg_wgpu_bindgroup_t* _sg_wgpu_lookup_bindgroup(uint32_t bg_id) {
    if (SG_INVALID_ID != bg_id) {
        _sg_wgpu_bindgroup_t* bg = _sg_wgpu_bindgroup_at(bg_id);
        if (bg->slot.id == bg_id) {
            return bg;
        }
    }
    return 0;
}

_SOKOL_PRIVATE _sg_wgpu_bindgroup_handle_t _sg_wgpu_alloc_bindgroup(void) {
    _sg_wgpu_bindgroups_pool_t* p = &_sg.wgpu.bindgroups_pool;
    _sg_wgpu_bindgroup_handle_t res;
    int slot_index = _sg_pool_alloc_index(&p->pool);
    if (_SG_INVALID_SLOT_INDEX != slot_index) {
        res.id = _sg_slot_alloc(&p->pool, &p->bindgroups[slot_index].slot, slot_index);
    } else {
        res.id = SG_INVALID_ID;
        _SG_ERROR(WGPU_BINDGROUPS_POOL_EXHAUSTED);
    }
    return res;
}

_SOKOL_PRIVATE void _sg_wgpu_dealloc_bindgroup(_sg_wgpu_bindgroup_t* bg) {
    SOKOL_ASSERT(bg && (bg->slot.state == SG_RESOURCESTATE_ALLOC) && (bg->slot.id != SG_INVALID_ID));
    _sg_wgpu_bindgroups_pool_t* p = &_sg.wgpu.bindgroups_pool;
    _sg_pool_free_index(&p->pool, _sg_slot_index(bg->slot.id));
    _sg_reset_slot(&bg->slot);
}

_SOKOL_PRIVATE void _sg_wgpu_reset_bindgroup_to_alloc_state(_sg_wgpu_bindgroup_t* bg) {
    SOKOL_ASSERT(bg);
    _sg_slot_t slot = bg->slot;
    _sg_clear(bg, sizeof(_sg_wgpu_bindgroup_t));
    bg->slot = slot;
    bg->slot.state = SG_RESOURCESTATE_ALLOC;
}

// MurmurHash64B (see: https://github.com/aappleby/smhasher/blob/61a0530f28277f2e850bfc39600ce61d02b518de/src/MurmurHash2.cpp#L142)
_SOKOL_PRIVATE uint64_t _sg_wgpu_hash(const void* key, int len, uint64_t seed) {
    const uint32_t m = 0x5bd1e995;
    const int r = 24;
    uint32_t h1 = (uint32_t)seed ^ (uint32_t)len;
    uint32_t h2 = (uint32_t)(seed >> 32);
    const uint32_t * data = (const uint32_t *)key;
    while (len >= 8) {
        uint32_t k1 = *data++;
        k1 *= m; k1 ^= k1 >> r; k1 *= m;
        h1 *= m; h1 ^= k1;
        len -= 4;
        uint32_t k2 = *data++;
        k2 *= m; k2 ^= k2 >> r; k2 *= m;
        h2 *= m; h2 ^= k2;
        len -= 4;
    }
    if (len >= 4) {
        uint32_t k1 = *data++;
        k1 *= m; k1 ^= k1 >> r; k1 *= m;
        h1 *= m; h1 ^= k1;
        len -= 4;
    }
    switch(len) {
        case 3: h2 ^= (uint32_t)(((unsigned char*)data)[2] << 16);
        case 2: h2 ^= (uint32_t)(((unsigned char*)data)[1] << 8);
        case 1: h2 ^= ((unsigned char*)data)[0];
        h2 *= m;
    };
    h1 ^= h2 >> 18; h1 *= m;
    h2 ^= h1 >> 22; h2 *= m;
    h1 ^= h2 >> 17; h1 *= m;
    h2 ^= h1 >> 19; h2 *= m;
    uint64_t h = h1;
    h = (h << 32) | h2;
    return h;
}

_SOKOL_PRIVATE void _sg_wgpu_init_bindgroups_cache_key(_sg_wgpu_bindgroups_cache_key_t* key, const _sg_bindings_t* bnd) {
    SOKOL_ASSERT(bnd);
    SOKOL_ASSERT(bnd->pip);
    SOKOL_ASSERT(bnd->num_vs_imgs <= SG_MAX_SHADERSTAGE_IMAGES);
    SOKOL_ASSERT(bnd->num_vs_smps <= SG_MAX_SHADERSTAGE_SAMPLERS);
    SOKOL_ASSERT(bnd->num_fs_imgs <= SG_MAX_SHADERSTAGE_IMAGES);
    SOKOL_ASSERT(bnd->num_fs_smps <= SG_MAX_SHADERSTAGE_SAMPLERS);

    _sg_clear(key->items, sizeof(key->items));
    key->items[0] = bnd->pip->slot.id;
    const int vs_imgs_offset = 1;
    const int vs_smps_offset = vs_imgs_offset + SG_MAX_SHADERSTAGE_IMAGES;
    const int fs_imgs_offset = vs_smps_offset + SG_MAX_SHADERSTAGE_SAMPLERS;
    const int fs_smps_offset = fs_imgs_offset + SG_MAX_SHADERSTAGE_IMAGES;
    SOKOL_ASSERT((fs_smps_offset + SG_MAX_SHADERSTAGE_SAMPLERS) == _SG_WGPU_BINDGROUPSCACHE_NUM_ITEMS);
    for (int i = 0; i < bnd->num_vs_imgs; i++) {
        SOKOL_ASSERT(bnd->vs_imgs[i]);
        key->items[vs_imgs_offset + i] = bnd->vs_imgs[i]->slot.id;
    }
    for (int i = 0; i < bnd->num_vs_smps; i++) {
        SOKOL_ASSERT(bnd->vs_smps[i]);
        key->items[vs_smps_offset + i] = bnd->vs_smps[i]->slot.id;
    }
    for (int i = 0; i < bnd->num_fs_imgs; i++) {
        SOKOL_ASSERT(bnd->fs_imgs[i]);
        key->items[fs_imgs_offset + i] = bnd->fs_imgs[i]->slot.id;
    }
    for (int i = 0; i < bnd->num_fs_smps; i++) {
        SOKOL_ASSERT(bnd->fs_smps[i]);
        key->items[fs_smps_offset + i] = bnd->fs_smps[i]->slot.id;
    }
    key->hash = _sg_wgpu_hash(&key->items, (int)sizeof(key->items), 0x1234567887654321);
}

_SOKOL_PRIVATE bool _sg_wgpu_compare_bindgroups_cache_key(_sg_wgpu_bindgroups_cache_key_t* k0, _sg_wgpu_bindgroups_cache_key_t* k1) {
    SOKOL_ASSERT(k0 && k1);
    if (k0->hash != k1->hash) {
        return false;
    }
    if (memcmp(&k0->items, &k1->items, sizeof(k0->items)) != 0) {
        _sg_stats_add(wgpu.bindings.num_bindgroup_cache_hash_vs_key_mismatch, 1);
        return false;
    }
    return true;
}

_SOKOL_PRIVATE _sg_wgpu_bindgroup_t* _sg_wgpu_create_bindgroup(_sg_bindings_t* bnd) {
    SOKOL_ASSERT(_sg.wgpu.dev);
    SOKOL_ASSERT(bnd->pip);
    SOKOL_ASSERT(bnd->pip->shader && (bnd->pip->cmn.shader_id.id == bnd->pip->shader->slot.id));
    _sg_stats_add(wgpu.bindings.num_create_bindgroup, 1);
    _sg_wgpu_bindgroup_handle_t bg_id = _sg_wgpu_alloc_bindgroup();
    if (bg_id.id == SG_INVALID_ID) {
        return 0;
    }
    _sg_wgpu_bindgroup_t* bg = _sg_wgpu_bindgroup_at(bg_id.id);
    SOKOL_ASSERT(bg && (bg->slot.state == SG_RESOURCESTATE_ALLOC));

    // create wgpu bindgroup object
    WGPUBindGroupLayout bgl = bnd->pip->shader->wgpu.bind_group_layout;
    SOKOL_ASSERT(bgl);
    WGPUBindGroupEntry wgpu_entries[SG_NUM_SHADER_STAGES * SG_MAX_SHADERSTAGE_IMAGES + SG_MAX_SHADERSTAGE_SAMPLERS];
    _sg_clear(&wgpu_entries, sizeof(wgpu_entries));
    int bge_index = 0;
    for (int i = 0; i < bnd->num_vs_imgs; i++) {
        WGPUBindGroupEntry* wgpu_entry = &wgpu_entries[bge_index++];
        wgpu_entry->binding = _sg_wgpu_image_binding(SG_SHADERSTAGE_VS, i);
        wgpu_entry->textureView = bnd->vs_imgs[i]->wgpu.view;
    }
    for (int i = 0; i < bnd->num_vs_smps; i++) {
        WGPUBindGroupEntry* wgpu_entry = &wgpu_entries[bge_index++];
        wgpu_entry->binding = _sg_wgpu_sampler_binding(SG_SHADERSTAGE_VS, i);
        wgpu_entry->sampler = bnd->vs_smps[i]->wgpu.smp;
    }
    for (int i = 0; i < bnd->num_fs_imgs; i++) {
        WGPUBindGroupEntry* wgpu_entry = &wgpu_entries[bge_index++];
        wgpu_entry->binding = _sg_wgpu_image_binding(SG_SHADERSTAGE_FS, i);
        wgpu_entry->textureView = bnd->fs_imgs[i]->wgpu.view;
    }
    for (int i = 0; i < bnd->num_fs_smps; i++) {
        WGPUBindGroupEntry* wgpu_entry = &wgpu_entries[bge_index++];
        wgpu_entry->binding = _sg_wgpu_sampler_binding(SG_SHADERSTAGE_FS, i);
        wgpu_entry->sampler = bnd->fs_smps[i]->wgpu.smp;
    }
    WGPUBindGroupDescriptor bg_desc;
    _sg_clear(&bg_desc, sizeof(bg_desc));
    bg_desc.layout = bgl;
    bg_desc.entryCount = (size_t)bge_index;
    bg_desc.entries = &wgpu_entries[0];
    bg->bindgroup = wgpuDeviceCreateBindGroup(_sg.wgpu.dev, &bg_desc);
    if (bg->bindgroup == 0) {
        _SG_ERROR(WGPU_CREATEBINDGROUP_FAILED);
        bg->slot.state = SG_RESOURCESTATE_FAILED;
        return bg;
    }

    _sg_wgpu_init_bindgroups_cache_key(&bg->key, bnd);

    bg->slot.state = SG_RESOURCESTATE_VALID;
    return bg;
}

_SOKOL_PRIVATE void _sg_wgpu_discard_bindgroup(_sg_wgpu_bindgroup_t* bg) {
    SOKOL_ASSERT(bg);
    _sg_stats_add(wgpu.bindings.num_discard_bindgroup, 1);
    if (bg->slot.state == SG_RESOURCESTATE_VALID) {
        if (bg->bindgroup) {
            wgpuBindGroupRelease(bg->bindgroup);
            bg->bindgroup = 0;
        }
        _sg_wgpu_reset_bindgroup_to_alloc_state(bg);
        SOKOL_ASSERT(bg->slot.state == SG_RESOURCESTATE_ALLOC);
    }
    if (bg->slot.state == SG_RESOURCESTATE_ALLOC) {
        _sg_wgpu_dealloc_bindgroup(bg);
        SOKOL_ASSERT(bg->slot.state == SG_RESOURCESTATE_INITIAL);
    }
}

_SOKOL_PRIVATE void _sg_wgpu_discard_all_bindgroups(void) {
    _sg_wgpu_bindgroups_pool_t* p = &_sg.wgpu.bindgroups_pool;
    for (int i = 0; i < p->pool.size; i++) {
        sg_resource_state state = p->bindgroups[i].slot.state;
        if ((state == SG_RESOURCESTATE_VALID) || (state == SG_RESOURCESTATE_FAILED)) {
            _sg_wgpu_discard_bindgroup(&p->bindgroups[i]);
        }
    }
}

_SOKOL_PRIVATE void _sg_wgpu_bindgroups_cache_init(const sg_desc* desc) {
    SOKOL_ASSERT(desc);
    SOKOL_ASSERT(_sg.wgpu.bindgroups_cache.num == 0);
    SOKOL_ASSERT(_sg.wgpu.bindgroups_cache.index_mask == 0);
    SOKOL_ASSERT(_sg.wgpu.bindgroups_cache.items == 0);
    const int num = desc->wgpu_bindgroups_cache_size;
    if (num <= 1) {
        _SG_PANIC(WGPU_BINDGROUPSCACHE_SIZE_GREATER_ONE);
    }
    if (!_sg_ispow2(num)) {
        _SG_PANIC(WGPU_BINDGROUPSCACHE_SIZE_POW2);
    }
    _sg.wgpu.bindgroups_cache.num = (uint32_t)desc->wgpu_bindgroups_cache_size;
    _sg.wgpu.bindgroups_cache.index_mask = _sg.wgpu.bindgroups_cache.num - 1;
    size_t size_in_bytes = sizeof(_sg_wgpu_bindgroup_handle_t) * (size_t)num;
    _sg.wgpu.bindgroups_cache.items = (_sg_wgpu_bindgroup_handle_t*)_sg_malloc_clear(size_in_bytes);
}

_SOKOL_PRIVATE void _sg_wgpu_bindgroups_cache_discard(void) {
    if (_sg.wgpu.bindgroups_cache.items) {
        _sg_free(_sg.wgpu.bindgroups_cache.items);
        _sg.wgpu.bindgroups_cache.items = 0;
    }
    _sg.wgpu.bindgroups_cache.num = 0;
    _sg.wgpu.bindgroups_cache.index_mask = 0;
}

_SOKOL_PRIVATE void _sg_wgpu_bindgroups_cache_set(uint64_t hash, uint32_t bg_id) {
    uint32_t index = hash & _sg.wgpu.bindgroups_cache.index_mask;
    SOKOL_ASSERT(index < _sg.wgpu.bindgroups_cache.num);
    SOKOL_ASSERT(_sg.wgpu.bindgroups_cache.items);
    _sg.wgpu.bindgroups_cache.items[index].id = bg_id;
}

_SOKOL_PRIVATE uint32_t _sg_wgpu_bindgroups_cache_get(uint64_t hash) {
    uint32_t index = hash & _sg.wgpu.bindgroups_cache.index_mask;
    SOKOL_ASSERT(index < _sg.wgpu.bindgroups_cache.num);
    SOKOL_ASSERT(_sg.wgpu.bindgroups_cache.items);
    return _sg.wgpu.bindgroups_cache.items[index].id;
}

_SOKOL_PRIVATE void _sg_wgpu_bindings_cache_clear(void) {
    memset(&_sg.wgpu.bindings_cache, 0, sizeof(_sg.wgpu.bindings_cache));
}

_SOKOL_PRIVATE bool _sg_wgpu_bindings_cache_vb_dirty(int index, const _sg_buffer_t* vb, int offset) {
    SOKOL_ASSERT((index >= 0) && (index < SG_MAX_VERTEX_BUFFERS));
    if (vb) {
        return (_sg.wgpu.bindings_cache.vbs[index].buffer.id != vb->slot.id)
            || (_sg.wgpu.bindings_cache.vbs[index].offset != offset);
    } else {
        return _sg.wgpu.bindings_cache.vbs[index].buffer.id != SG_INVALID_ID;
    }
}

_SOKOL_PRIVATE void _sg_wgpu_bindings_cache_vb_update(int index, const _sg_buffer_t* vb, int offset) {
    SOKOL_ASSERT((index >= 0) && (index < SG_MAX_VERTEX_BUFFERS));
    if (vb) {
        _sg.wgpu.bindings_cache.vbs[index].buffer.id = vb->slot.id;
        _sg.wgpu.bindings_cache.vbs[index].offset = offset;
    } else {
        _sg.wgpu.bindings_cache.vbs[index].buffer.id = SG_INVALID_ID;
        _sg.wgpu.bindings_cache.vbs[index].offset = 0;
    }
}

_SOKOL_PRIVATE bool _sg_wgpu_bindings_cache_ib_dirty(const _sg_buffer_t* ib, int offset) {
    if (ib) {
        return (_sg.wgpu.bindings_cache.ib.buffer.id != ib->slot.id)
            || (_sg.wgpu.bindings_cache.ib.offset != offset);
    } else {
        return _sg.wgpu.bindings_cache.ib.buffer.id != SG_INVALID_ID;
    }
}

_SOKOL_PRIVATE void _sg_wgpu_bindings_cache_ib_update(const _sg_buffer_t* ib, int offset) {
    if (ib) {
        _sg.wgpu.bindings_cache.ib.buffer.id = ib->slot.id;
        _sg.wgpu.bindings_cache.ib.offset = offset;
    } else {
        _sg.wgpu.bindings_cache.ib.buffer.id = SG_INVALID_ID;
        _sg.wgpu.bindings_cache.ib.offset = 0;
    }
}

_SOKOL_PRIVATE bool _sg_wgpu_bindings_cache_bg_dirty(const _sg_wgpu_bindgroup_t* bg) {
    if (bg) {
        return _sg.wgpu.bindings_cache.bg.id != bg->slot.id;
    } else {
        return _sg.wgpu.bindings_cache.bg.id != SG_INVALID_ID;
    }
}

_SOKOL_PRIVATE void _sg_wgpu_bindings_cache_bg_update(const _sg_wgpu_bindgroup_t* bg) {
    if (bg) {
        _sg.wgpu.bindings_cache.bg.id = bg->slot.id;
    } else {
        _sg.wgpu.bindings_cache.bg.id = SG_INVALID_ID;
    }
}

_SOKOL_PRIVATE void _sg_wgpu_set_image_sampler_bindgroup(_sg_wgpu_bindgroup_t* bg) {
    if (_sg_wgpu_bindings_cache_bg_dirty(bg)) {
        _sg_wgpu_bindings_cache_bg_update(bg);
        _sg_stats_add(wgpu.bindings.num_set_bindgroup, 1);
        if (bg) {
            SOKOL_ASSERT(bg->slot.state == SG_RESOURCESTATE_VALID);
            SOKOL_ASSERT(bg->bindgroup);
            wgpuRenderPassEncoderSetBindGroup(_sg.wgpu.pass_enc, _SG_WGPU_IMAGE_SAMPLER_BINDGROUP_INDEX, bg->bindgroup, 0, 0);
        } else {
            // a nullptr bindgroup means setting the empty bindgroup
            wgpuRenderPassEncoderSetBindGroup(_sg.wgpu.pass_enc, _SG_WGPU_IMAGE_SAMPLER_BINDGROUP_INDEX, _sg.wgpu.empty_bind_group, 0, 0);
        }
    } else {
        _sg_stats_add(wgpu.bindings.num_skip_redundant_bindgroup, 1);
    }
}

_SOKOL_PRIVATE bool _sg_wgpu_apply_bindgroup(_sg_bindings_t* bnd) {
    if ((bnd->num_vs_imgs + bnd->num_vs_smps + bnd->num_fs_imgs + bnd->num_fs_smps) > 0) {
        if (!_sg.desc.wgpu_disable_bindgroups_cache) {
            _sg_wgpu_bindgroup_t* bg = 0;
            _sg_wgpu_bindgroups_cache_key_t key;
            _sg_wgpu_init_bindgroups_cache_key(&key, bnd);
            uint32_t bg_id = _sg_wgpu_bindgroups_cache_get(key.hash);
            if (bg_id != SG_INVALID_ID) {
                // potential cache hit
                bg = _sg_wgpu_lookup_bindgroup(bg_id);
                SOKOL_ASSERT(bg && (bg->slot.state == SG_RESOURCESTATE_VALID));
                if (!_sg_wgpu_compare_bindgroups_cache_key(&key, &bg->key)) {
                    // cache collision, need to delete cached bindgroup
                    _sg_stats_add(wgpu.bindings.num_bindgroup_cache_collisions, 1);
                    _sg_wgpu_discard_bindgroup(bg);
                    _sg_wgpu_bindgroups_cache_set(key.hash, SG_INVALID_ID);
                    bg = 0;
                } else {
                    _sg_stats_add(wgpu.bindings.num_bindgroup_cache_hits, 1);
                }
            } else {
                _sg_stats_add(wgpu.bindings.num_bindgroup_cache_misses, 1);
            }
            if (bg == 0) {
                // either no cache entry yet, or cache collision, create new bindgroup and store in cache
                bg = _sg_wgpu_create_bindgroup(bnd);
                _sg_wgpu_bindgroups_cache_set(key.hash, bg->slot.id);
            }
            if (bg && bg->slot.state == SG_RESOURCESTATE_VALID) {
                _sg_wgpu_set_image_sampler_bindgroup(bg);
            } else {
                return false;
            }
        } else {
            // bindgroups cache disabled, create and destroy bindgroup on the fly (expensive!)
            _sg_wgpu_bindgroup_t* bg = _sg_wgpu_create_bindgroup(bnd);
            if (bg) {
                if (bg->slot.state == SG_RESOURCESTATE_VALID) {
                    _sg_wgpu_set_image_sampler_bindgroup(bg);
                }
                _sg_wgpu_discard_bindgroup(bg);
            } else {
                return false;
            }
        }
    } else {
        _sg_wgpu_set_image_sampler_bindgroup(0);
    }
    return true;
}
_SOKOL_PRIVATE bool _sg_wgpu_apply_index_buffer(_sg_bindings_t* bnd) {
    if (_sg_wgpu_bindings_cache_ib_dirty(bnd->ib, bnd->ib_offset)) {
        _sg_wgpu_bindings_cache_ib_update(bnd->ib, bnd->ib_offset);
        if (bnd->ib) {
            const WGPUIndexFormat format = _sg_wgpu_indexformat(bnd->pip->cmn.index_type);
            const uint64_t buf_size = (uint64_t)bnd->ib->cmn.size;
            const uint64_t offset = (uint64_t)bnd->ib_offset;
            SOKOL_ASSERT(buf_size > offset);
            const uint64_t max_bytes = buf_size - offset;
            wgpuRenderPassEncoderSetIndexBuffer(_sg.wgpu.pass_enc, bnd->ib->wgpu.buf, format, offset, max_bytes);
            _sg_stats_add(wgpu.bindings.num_set_index_buffer, 1);
        }
        // FIXME: else-path should actually set a null index buffer (this was just recently implemented in WebGPU)
    } else {
        _sg_stats_add(wgpu.bindings.num_skip_redundant_index_buffer, 1);
    }
    return true;
}

_SOKOL_PRIVATE bool _sg_wgpu_apply_vertex_buffers(_sg_bindings_t* bnd) {
    for (int slot = 0; slot < bnd->num_vbs; slot++) {
        if (_sg_wgpu_bindings_cache_vb_dirty(slot, bnd->vbs[slot], bnd->vb_offsets[slot])) {
            _sg_wgpu_bindings_cache_vb_update(slot, bnd->vbs[slot], bnd->vb_offsets[slot]);
            const uint64_t buf_size = (uint64_t)bnd->vbs[slot]->cmn.size;
            const uint64_t offset = (uint64_t)bnd->vb_offsets[slot];
            SOKOL_ASSERT(buf_size > offset);
            const uint64_t max_bytes = buf_size - offset;
            wgpuRenderPassEncoderSetVertexBuffer(_sg.wgpu.pass_enc, (uint32_t)slot, bnd->vbs[slot]->wgpu.buf, offset, max_bytes);
            _sg_stats_add(wgpu.bindings.num_set_vertex_buffer, 1);
        } else {
            _sg_stats_add(wgpu.bindings.num_skip_redundant_vertex_buffer, 1);
        }
    }
    // FIXME: remaining vb slots should actually set a null vertex buffer (this was just recently implemented in WebGPU)
    return true;
}

_SOKOL_PRIVATE void _sg_wgpu_setup_backend(const sg_desc* desc) {
    SOKOL_ASSERT(desc);
    SOKOL_ASSERT(desc->context.wgpu.device);
    SOKOL_ASSERT(desc->context.wgpu.render_view_cb || desc->context.wgpu.render_view_userdata_cb);
    SOKOL_ASSERT(desc->context.wgpu.resolve_view_cb || desc->context.wgpu.resolve_view_userdata_cb);
    SOKOL_ASSERT(desc->context.wgpu.depth_stencil_view_cb || desc->context.wgpu.depth_stencil_view_userdata_cb);
    SOKOL_ASSERT(desc->uniform_buffer_size > 0);
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
    _sg.wgpu.queue = wgpuDeviceGetQueue(_sg.wgpu.dev);
    SOKOL_ASSERT(_sg.wgpu.queue);

    _sg_wgpu_init_caps();
    _sg_wgpu_uniform_buffer_init(desc);
    _sg_wgpu_bindgroups_pool_init(desc);
    _sg_wgpu_bindgroups_cache_init(desc);
    _sg_wgpu_bindings_cache_clear();

    // create an empty bind group for shader stages without bound images
    // FIXME: once WebGPU supports setting null objects, this can be removed
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

    // create initial per-frame command encoder
    WGPUCommandEncoderDescriptor cmd_enc_desc;
    _sg_clear(&cmd_enc_desc, sizeof(cmd_enc_desc));
    _sg.wgpu.cmd_enc = wgpuDeviceCreateCommandEncoder(_sg.wgpu.dev, &cmd_enc_desc);
    SOKOL_ASSERT(_sg.wgpu.cmd_enc);
}

_SOKOL_PRIVATE void _sg_wgpu_discard_backend(void) {
    SOKOL_ASSERT(_sg.wgpu.valid);
    SOKOL_ASSERT(_sg.wgpu.cmd_enc);
    _sg.wgpu.valid = false;
    _sg_wgpu_discard_all_bindgroups();
    _sg_wgpu_bindgroups_cache_discard();
    _sg_wgpu_bindgroups_pool_discard();
    _sg_wgpu_uniform_buffer_discard();
    wgpuBindGroupRelease(_sg.wgpu.empty_bind_group); _sg.wgpu.empty_bind_group = 0;
    wgpuCommandEncoderRelease(_sg.wgpu.cmd_enc); _sg.wgpu.cmd_enc = 0;
    wgpuQueueRelease(_sg.wgpu.queue); _sg.wgpu.queue = 0;
}

_SOKOL_PRIVATE void _sg_wgpu_reset_state_cache(void) {
    _sg_wgpu_bindings_cache_clear();
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
    // FIXME
}

_SOKOL_PRIVATE sg_resource_state _sg_wgpu_create_buffer(_sg_buffer_t* buf, const sg_buffer_desc* desc) {
    SOKOL_ASSERT(buf && desc);
    const bool injected = (0 != desc->wgpu_buffer);
    if (injected) {
        buf->wgpu.buf = (WGPUBuffer) desc->wgpu_buffer;
        wgpuBufferReference(buf->wgpu.buf);
    } else {
        // buffer mapping size must be multiple of 4, so round up buffer size (only a problem
        // with index buffers containing odd number of indices)
        const uint64_t wgpu_buf_size = _sg_roundup_u64((uint64_t)buf->cmn.size, 4);
        const bool map_at_creation = (SG_USAGE_IMMUTABLE == buf->cmn.usage);

        WGPUBufferDescriptor wgpu_buf_desc;
        _sg_clear(&wgpu_buf_desc, sizeof(wgpu_buf_desc));
        wgpu_buf_desc.usage = _sg_wgpu_buffer_usage(buf->cmn.type, buf->cmn.usage);
        wgpu_buf_desc.size = wgpu_buf_size;
        wgpu_buf_desc.mappedAtCreation = map_at_creation;
        wgpu_buf_desc.label = desc->label;
        buf->wgpu.buf = wgpuDeviceCreateBuffer(_sg.wgpu.dev, &wgpu_buf_desc);
        if (0 == buf->wgpu.buf) {
            _SG_ERROR(WGPU_CREATE_BUFFER_FAILED);
            return SG_RESOURCESTATE_FAILED;
        }
        if (map_at_creation) {
            SOKOL_ASSERT(desc->data.ptr && (desc->data.size > 0));
            SOKOL_ASSERT(desc->data.size <= (size_t)buf->cmn.size);
            // FIXME: inefficient on WASM
            void* ptr = wgpuBufferGetMappedRange(buf->wgpu.buf, 0, wgpu_buf_size);
            SOKOL_ASSERT(ptr);
            memcpy(ptr, desc->data.ptr, desc->data.size);
            wgpuBufferUnmap(buf->wgpu.buf);
        }
    }
    return SG_RESOURCESTATE_VALID;
}

_SOKOL_PRIVATE void _sg_wgpu_discard_buffer(_sg_buffer_t* buf) {
    SOKOL_ASSERT(buf);
    if (buf->wgpu.buf) {
        wgpuBufferDestroy(buf->wgpu.buf);
        wgpuBufferRelease(buf->wgpu.buf);
    }
}

_SOKOL_PRIVATE void _sg_wgpu_copy_buffer_data(const _sg_buffer_t* buf, uint64_t offset, const sg_range* data) {
    SOKOL_ASSERT((offset + data->size) <= (size_t)buf->cmn.size);
    // WebGPU's write-buffer requires the size to be a multiple of four, so we may need to split the copy
    // operation into two writeBuffer calls
    uint64_t clamped_size = data->size & ~3UL;
    uint64_t extra_size = data->size & 3UL;
    SOKOL_ASSERT(extra_size < 4);
    wgpuQueueWriteBuffer(_sg.wgpu.queue, buf->wgpu.buf, offset, data->ptr, clamped_size);
    if (extra_size > 0) {
        const uint64_t extra_src_offset = clamped_size;
        const uint64_t extra_dst_offset = offset + clamped_size;
        uint8_t extra_data[4] = { 0 };
        uint8_t* extra_src_ptr = ((uint8_t*)data->ptr) + extra_src_offset;
        for (size_t i = 0; i < extra_size; i++) {
            extra_data[i] = extra_src_ptr[i];
        }
        wgpuQueueWriteBuffer(_sg.wgpu.queue, buf->wgpu.buf, extra_dst_offset, extra_src_ptr, 4);
    }
}

_SOKOL_PRIVATE void _sg_wgpu_copy_image_data(const _sg_image_t* img, WGPUTexture wgpu_tex, const sg_image_data* data) {
    WGPUTextureDataLayout wgpu_layout;
    _sg_clear(&wgpu_layout, sizeof(wgpu_layout));
    WGPUImageCopyTexture wgpu_copy_tex;
    _sg_clear(&wgpu_copy_tex, sizeof(wgpu_copy_tex));
    wgpu_copy_tex.texture = wgpu_tex;
    wgpu_copy_tex.aspect = WGPUTextureAspect_All;
    WGPUExtent3D wgpu_extent;
    _sg_clear(&wgpu_extent, sizeof(wgpu_extent));
    const int num_faces = (img->cmn.type == SG_IMAGETYPE_CUBE) ? 6 : 1;
    for (int face_index = 0; face_index < num_faces; face_index++) {
        for (int mip_index = 0; mip_index < img->cmn.num_mipmaps; mip_index++) {
            wgpu_copy_tex.mipLevel = (uint32_t)mip_index;
            wgpu_copy_tex.origin.z = (uint32_t)face_index;
            int mip_width = _sg_miplevel_dim(img->cmn.width, mip_index);
            int mip_height = _sg_miplevel_dim(img->cmn.height, mip_index);
            int mip_slices;
            switch (img->cmn.type) {
                case SG_IMAGETYPE_CUBE:
                    mip_slices = 1;
                    break;
                case SG_IMAGETYPE_3D:
                    mip_slices = _sg_miplevel_dim(img->cmn.num_slices, mip_index);
                    break;
                default:
                    mip_slices = img->cmn.num_slices;
                    break;
            }
            const int row_pitch = _sg_row_pitch(img->cmn.pixel_format, mip_width, 1);
            const int num_rows = _sg_num_rows(img->cmn.pixel_format, mip_height);
            if (_sg_is_compressed_pixel_format(img->cmn.pixel_format)) {
                mip_width = _sg_roundup(mip_width, 4);
                mip_height = _sg_roundup(mip_height, 4);
            }
            wgpu_layout.offset = 0;
            wgpu_layout.bytesPerRow = (uint32_t)row_pitch;
            wgpu_layout.rowsPerImage = (uint32_t)num_rows;
            wgpu_extent.width = (uint32_t)mip_width;
            wgpu_extent.height = (uint32_t)mip_height;
            wgpu_extent.depthOrArrayLayers = (uint32_t)mip_slices;
            const sg_range* mip_data = &data->subimage[face_index][mip_index];
            wgpuQueueWriteTexture(_sg.wgpu.queue, &wgpu_copy_tex, mip_data->ptr, mip_data->size, &wgpu_layout, &wgpu_extent);
        }
    }
}

_SOKOL_PRIVATE sg_resource_state _sg_wgpu_create_image(_sg_image_t* img, const sg_image_desc* desc) {
    SOKOL_ASSERT(img && desc);
    const bool injected = (0 != desc->wgpu_texture);
    if (injected) {
        img->wgpu.tex = (WGPUTexture)desc->wgpu_texture;
        wgpuTextureReference(img->wgpu.tex);
        img->wgpu.view = (WGPUTextureView)desc->wgpu_texture_view;
        if (img->wgpu.view) {
            wgpuTextureViewReference(img->wgpu.view);
        }
    } else {
        WGPUTextureDescriptor wgpu_tex_desc;
        _sg_clear(&wgpu_tex_desc, sizeof(wgpu_tex_desc));
        wgpu_tex_desc.label = desc->label;
        wgpu_tex_desc.usage = WGPUTextureUsage_TextureBinding|WGPUTextureUsage_CopyDst;
        if (desc->render_target) {
            wgpu_tex_desc.usage |= WGPUTextureUsage_RenderAttachment;
        }
        wgpu_tex_desc.dimension = _sg_wgpu_texture_dimension(img->cmn.type);
        wgpu_tex_desc.size.width = (uint32_t) img->cmn.width;
        wgpu_tex_desc.size.height = (uint32_t) img->cmn.height;
        if (desc->type == SG_IMAGETYPE_CUBE) {
            wgpu_tex_desc.size.depthOrArrayLayers = 6;
        } else {
            wgpu_tex_desc.size.depthOrArrayLayers = (uint32_t) img->cmn.num_slices;
        }
        wgpu_tex_desc.format = _sg_wgpu_textureformat(img->cmn.pixel_format);
        wgpu_tex_desc.mipLevelCount = (uint32_t) img->cmn.num_mipmaps;
        wgpu_tex_desc.sampleCount = (uint32_t) img->cmn.sample_count;
        img->wgpu.tex = wgpuDeviceCreateTexture(_sg.wgpu.dev, &wgpu_tex_desc);
        if (0 == img->wgpu.tex) {
            _SG_ERROR(WGPU_CREATE_TEXTURE_FAILED);
            return SG_RESOURCESTATE_FAILED;
        }
        if ((img->cmn.usage == SG_USAGE_IMMUTABLE) && !img->cmn.render_target) {
            _sg_wgpu_copy_image_data(img, img->wgpu.tex, &desc->data);
        }
        WGPUTextureViewDescriptor wgpu_texview_desc;
        _sg_clear(&wgpu_texview_desc, sizeof(wgpu_texview_desc));
        wgpu_texview_desc.label = desc->label;
        wgpu_texview_desc.dimension = _sg_wgpu_texture_view_dimension(img->cmn.type);
        wgpu_texview_desc.mipLevelCount = (uint32_t)img->cmn.num_mipmaps;
        if (img->cmn.type == SG_IMAGETYPE_CUBE) {
            wgpu_texview_desc.arrayLayerCount = 6;
        } else if (img->cmn.type == SG_IMAGETYPE_ARRAY) {
            wgpu_texview_desc.arrayLayerCount = (uint32_t)img->cmn.num_slices;
        } else {
            wgpu_texview_desc.arrayLayerCount = 1;
        }
        if (_sg_is_depth_or_depth_stencil_format(img->cmn.pixel_format)) {
            wgpu_texview_desc.aspect = WGPUTextureAspect_DepthOnly;
        } else {
            wgpu_texview_desc.aspect = WGPUTextureAspect_All;
        }
        img->wgpu.view = wgpuTextureCreateView(img->wgpu.tex, &wgpu_texview_desc);
        if (0 == img->wgpu.view) {
            _SG_ERROR(WGPU_CREATE_TEXTURE_VIEW_FAILED);
            return SG_RESOURCESTATE_FAILED;
        }
    }
    return SG_RESOURCESTATE_VALID;
}

_SOKOL_PRIVATE void _sg_wgpu_discard_image(_sg_image_t* img) {
    SOKOL_ASSERT(img);
    if (img->wgpu.view) {
        wgpuTextureViewRelease(img->wgpu.view);
        img->wgpu.view = 0;
    }
    if (img->wgpu.tex) {
        wgpuTextureDestroy(img->wgpu.tex);
        wgpuTextureRelease(img->wgpu.tex);
        img->wgpu.tex = 0;
    }
}

_SOKOL_PRIVATE sg_resource_state _sg_wgpu_create_sampler(_sg_sampler_t* smp, const sg_sampler_desc* desc) {
    SOKOL_ASSERT(smp && desc);
    SOKOL_ASSERT(_sg.wgpu.dev);
    const bool injected = (0 != desc->wgpu_sampler);
    if (injected) {
        smp->wgpu.smp = (WGPUSampler) desc->wgpu_sampler;
        wgpuSamplerReference(smp->wgpu.smp);
    } else {
        WGPUSamplerDescriptor wgpu_desc;
        _sg_clear(&wgpu_desc, sizeof(wgpu_desc));
        wgpu_desc.label = desc->label;
        wgpu_desc.addressModeU = _sg_wgpu_sampler_address_mode(desc->wrap_u);
        wgpu_desc.addressModeV = _sg_wgpu_sampler_address_mode(desc->wrap_v);
        wgpu_desc.addressModeW = _sg_wgpu_sampler_address_mode(desc->wrap_w);
        wgpu_desc.magFilter = _sg_wgpu_sampler_minmag_filter(desc->mag_filter);
        wgpu_desc.minFilter = _sg_wgpu_sampler_minmag_filter(desc->min_filter);
        wgpu_desc.mipmapFilter = _sg_wgpu_sampler_mipmap_filter(desc->mipmap_filter);
        wgpu_desc.lodMinClamp = desc->min_lod;
        wgpu_desc.lodMaxClamp = desc->max_lod;
        wgpu_desc.compare = _sg_wgpu_comparefunc(desc->compare);
        if (wgpu_desc.compare == WGPUCompareFunction_Never) {
            wgpu_desc.compare = WGPUCompareFunction_Undefined;
        }
        wgpu_desc.maxAnisotropy = (uint16_t)desc->max_anisotropy;
        smp->wgpu.smp = wgpuDeviceCreateSampler(_sg.wgpu.dev, &wgpu_desc);
        if (0 == smp->wgpu.smp) {
            _SG_ERROR(WGPU_CREATE_SAMPLER_FAILED);
            return SG_RESOURCESTATE_FAILED;
        }
    }
    return SG_RESOURCESTATE_VALID;
}

_SOKOL_PRIVATE void _sg_wgpu_discard_sampler(_sg_sampler_t* smp) {
    SOKOL_ASSERT(smp);
    if (smp->wgpu.smp) {
        wgpuSamplerRelease(smp->wgpu.smp);
        smp->wgpu.smp = 0;
    }
}

_SOKOL_PRIVATE sg_resource_state _sg_wgpu_create_shader(_sg_shader_t* shd, const sg_shader_desc* desc) {
    SOKOL_ASSERT(shd && desc);
    SOKOL_ASSERT(desc->vs.source && desc->fs.source);

    #define _sg_wgpu_create_shader_max_bgl_entries (SG_NUM_SHADER_STAGES * (SG_MAX_SHADERSTAGE_IMAGES + SG_MAX_SHADERSTAGE_SAMPLERS))
    WGPUBindGroupLayoutEntry wgpu_bgl_entries[_sg_wgpu_create_shader_max_bgl_entries];
    _sg_clear(wgpu_bgl_entries, sizeof(wgpu_bgl_entries));
    int bgl_index = 0;
    for (int stage_index = 0; stage_index < SG_NUM_SHADER_STAGES; stage_index++) {
        const sg_shader_stage_desc* stage_desc = (stage_index == SG_SHADERSTAGE_VS) ? &desc->vs : &desc->fs;

        _sg_shader_stage_t* cmn_stage = &shd->cmn.stage[stage_index];
        _sg_wgpu_shader_stage_t* wgpu_stage = &shd->wgpu.stage[stage_index];
        _sg_strcpy(&wgpu_stage->entry, stage_desc->entry);

        WGPUShaderModuleWGSLDescriptor wgpu_shdmod_wgsl_desc;
        _sg_clear(&wgpu_shdmod_wgsl_desc, sizeof(wgpu_shdmod_wgsl_desc));
        wgpu_shdmod_wgsl_desc.chain.sType = WGPUSType_ShaderModuleWGSLDescriptor;
        wgpu_shdmod_wgsl_desc.code = stage_desc->source;

        WGPUShaderModuleDescriptor wgpu_shdmod_desc;
        _sg_clear(&wgpu_shdmod_desc, sizeof(wgpu_shdmod_desc));
        wgpu_shdmod_desc.nextInChain = &wgpu_shdmod_wgsl_desc.chain;
        wgpu_shdmod_desc.label = desc->label;

        wgpu_stage->module = wgpuDeviceCreateShaderModule(_sg.wgpu.dev, &wgpu_shdmod_desc);
        if (0 == wgpu_stage->module) {
            _SG_ERROR(WGPU_CREATE_SHADER_MODULE_FAILED);
            return SG_RESOURCESTATE_FAILED;
        }

        const int num_images = cmn_stage->num_images;
        if (num_images > (int)_sg.wgpu.limits.limits.maxSampledTexturesPerShaderStage) {
            _SG_ERROR(WGPU_SHADER_TOO_MANY_IMAGES);
            return SG_RESOURCESTATE_FAILED;
        }
        const int num_samplers = cmn_stage->num_samplers;
        if (num_samplers > (int)_sg.wgpu.limits.limits.maxSamplersPerShaderStage) {
            _SG_ERROR(WGPU_SHADER_TOO_MANY_SAMPLERS);
            return SG_RESOURCESTATE_FAILED;
        }
        for (int img_index = 0; img_index < num_images; img_index++) {
            SOKOL_ASSERT(bgl_index < _sg_wgpu_create_shader_max_bgl_entries);
            WGPUBindGroupLayoutEntry* wgpu_bgl_entry = &wgpu_bgl_entries[bgl_index++];
            const sg_shader_image_desc* img_desc = &stage_desc->images[img_index];
            wgpu_bgl_entry->binding = _sg_wgpu_image_binding((sg_shader_stage)stage_index, img_index);
            wgpu_bgl_entry->visibility = _sg_wgpu_shader_stage((sg_shader_stage)stage_index);
            wgpu_bgl_entry->texture.viewDimension = _sg_wgpu_texture_view_dimension(img_desc->image_type);
            wgpu_bgl_entry->texture.sampleType = _sg_wgpu_texture_sample_type(img_desc->sample_type);
            wgpu_bgl_entry->texture.multisampled = img_desc->multisampled;
        }
        for (int smp_index = 0; smp_index < num_samplers; smp_index++) {
            SOKOL_ASSERT(bgl_index < _sg_wgpu_create_shader_max_bgl_entries);
            WGPUBindGroupLayoutEntry* wgpu_bgl_entry = &wgpu_bgl_entries[bgl_index++];
            const sg_shader_sampler_desc* smp_desc = &stage_desc->samplers[smp_index];
            wgpu_bgl_entry->binding =_sg_wgpu_sampler_binding((sg_shader_stage)stage_index, smp_index);
            wgpu_bgl_entry->visibility = _sg_wgpu_shader_stage((sg_shader_stage)stage_index);
            wgpu_bgl_entry->sampler.type = _sg_wgpu_sampler_binding_type(smp_desc->sampler_type);
        }
    }

    WGPUBindGroupLayoutDescriptor wgpu_bgl_desc;
    _sg_clear(&wgpu_bgl_desc, sizeof(wgpu_bgl_desc));
    wgpu_bgl_desc.entryCount = (size_t)bgl_index;
    wgpu_bgl_desc.entries = &wgpu_bgl_entries[0];
    shd->wgpu.bind_group_layout = wgpuDeviceCreateBindGroupLayout(_sg.wgpu.dev, &wgpu_bgl_desc);
    if (shd->wgpu.bind_group_layout == 0) {
        _SG_ERROR(WGPU_SHADER_CREATE_BINDGROUP_LAYOUT_FAILED);
        return SG_RESOURCESTATE_FAILED;
    }

    #undef _sg_wgpu_create_shader_max_bgl_entries
    return SG_RESOURCESTATE_VALID;
}

_SOKOL_PRIVATE void _sg_wgpu_discard_shader(_sg_shader_t* shd) {
    SOKOL_ASSERT(shd);
    if (shd->wgpu.bind_group_layout) {
        wgpuBindGroupLayoutRelease(shd->wgpu.bind_group_layout);
        shd->wgpu.bind_group_layout = 0;
    }
    for (int stage_index = 0; stage_index < SG_NUM_SHADER_STAGES; stage_index++) {
        _sg_wgpu_shader_stage_t* wgpu_stage = &shd->wgpu.stage[stage_index];
        if (wgpu_stage->module) {
            wgpuShaderModuleRelease(wgpu_stage->module);
            wgpu_stage->module = 0;
        }
    }
}

_SOKOL_PRIVATE sg_resource_state _sg_wgpu_create_pipeline(_sg_pipeline_t* pip, _sg_shader_t* shd, const sg_pipeline_desc* desc) {
    SOKOL_ASSERT(pip && shd && desc);
    SOKOL_ASSERT(desc->shader.id == shd->slot.id);
    SOKOL_ASSERT(shd->wgpu.bind_group_layout);
    pip->shader = shd;

    pip->wgpu.blend_color.r = (double) desc->blend_color.r;
    pip->wgpu.blend_color.g = (double) desc->blend_color.g;
    pip->wgpu.blend_color.b = (double) desc->blend_color.b;
    pip->wgpu.blend_color.a = (double) desc->blend_color.a;

    // - @group(0) for uniform blocks
    // - @group(1) for all image and sampler resources
    WGPUBindGroupLayout wgpu_bgl[_SG_WGPU_NUM_BINDGROUPS];
    _sg_clear(&wgpu_bgl, sizeof(wgpu_bgl));
    wgpu_bgl[_SG_WGPU_UNIFORM_BINDGROUP_INDEX] = _sg.wgpu.uniform.bind.group_layout;
    wgpu_bgl[_SG_WGPU_IMAGE_SAMPLER_BINDGROUP_INDEX] = shd->wgpu.bind_group_layout;
    WGPUPipelineLayoutDescriptor wgpu_pl_desc;
    _sg_clear(&wgpu_pl_desc, sizeof(wgpu_pl_desc));
    wgpu_pl_desc.bindGroupLayoutCount = _SG_WGPU_NUM_BINDGROUPS;
    wgpu_pl_desc.bindGroupLayouts = &wgpu_bgl[0];
    const WGPUPipelineLayout wgpu_pip_layout = wgpuDeviceCreatePipelineLayout(_sg.wgpu.dev, &wgpu_pl_desc);
    if (0 == wgpu_pip_layout) {
        _SG_ERROR(WGPU_CREATE_PIPELINE_LAYOUT_FAILED);
        return SG_RESOURCESTATE_FAILED;
    }
    SOKOL_ASSERT(wgpu_pip_layout);

    WGPUVertexBufferLayout wgpu_vb_layouts[SG_MAX_VERTEX_BUFFERS];
    _sg_clear(wgpu_vb_layouts, sizeof(wgpu_vb_layouts));
    WGPUVertexAttribute wgpu_vtx_attrs[SG_MAX_VERTEX_BUFFERS][SG_MAX_VERTEX_ATTRIBUTES];
    _sg_clear(wgpu_vtx_attrs, sizeof(wgpu_vtx_attrs));
    int wgpu_vb_num = 0;
    for (int vb_idx = 0; vb_idx < SG_MAX_VERTEX_BUFFERS; vb_idx++, wgpu_vb_num++) {
        const sg_vertex_buffer_layout_state* vbl_state = &desc->layout.buffers[vb_idx];
        if (0 == vbl_state->stride) {
            break;
        }
        wgpu_vb_layouts[vb_idx].arrayStride = (uint64_t)vbl_state->stride;
        wgpu_vb_layouts[vb_idx].stepMode = _sg_wgpu_stepmode(vbl_state->step_func);
        wgpu_vb_layouts[vb_idx].attributes = &wgpu_vtx_attrs[vb_idx][0];
    }
    for (int va_idx = 0; va_idx < SG_MAX_VERTEX_ATTRIBUTES; va_idx++) {
        const sg_vertex_attr_state* va_state = &desc->layout.attrs[va_idx];
        if (SG_VERTEXFORMAT_INVALID == va_state->format) {
            break;
        }
        const int vb_idx = va_state->buffer_index;
        SOKOL_ASSERT(vb_idx < SG_MAX_VERTEX_BUFFERS);
        pip->cmn.vertex_buffer_layout_active[vb_idx] = true;
        const size_t wgpu_attr_idx = wgpu_vb_layouts[vb_idx].attributeCount;
        wgpu_vb_layouts[vb_idx].attributeCount += 1;
        wgpu_vtx_attrs[vb_idx][wgpu_attr_idx].format = _sg_wgpu_vertexformat(va_state->format);
        wgpu_vtx_attrs[vb_idx][wgpu_attr_idx].offset = (uint64_t)va_state->offset;
        wgpu_vtx_attrs[vb_idx][wgpu_attr_idx].shaderLocation = (uint32_t)va_idx;
    }

    WGPURenderPipelineDescriptor wgpu_pip_desc;
    _sg_clear(&wgpu_pip_desc, sizeof(wgpu_pip_desc));
    WGPUDepthStencilState wgpu_ds_state;
    _sg_clear(&wgpu_ds_state, sizeof(wgpu_ds_state));
    WGPUFragmentState wgpu_frag_state;
    _sg_clear(&wgpu_frag_state, sizeof(wgpu_frag_state));
    WGPUColorTargetState wgpu_ctgt_state[SG_MAX_COLOR_ATTACHMENTS];
    _sg_clear(&wgpu_ctgt_state, sizeof(wgpu_ctgt_state));
    WGPUBlendState wgpu_blend_state[SG_MAX_COLOR_ATTACHMENTS];
    _sg_clear(&wgpu_blend_state, sizeof(wgpu_blend_state));
    wgpu_pip_desc.label = desc->label;
    wgpu_pip_desc.layout = wgpu_pip_layout;
    wgpu_pip_desc.vertex.module = shd->wgpu.stage[SG_SHADERSTAGE_VS].module;
    wgpu_pip_desc.vertex.entryPoint = shd->wgpu.stage[SG_SHADERSTAGE_VS].entry.buf;
    wgpu_pip_desc.vertex.bufferCount = (size_t)wgpu_vb_num;
    wgpu_pip_desc.vertex.buffers = &wgpu_vb_layouts[0];
    wgpu_pip_desc.primitive.topology = _sg_wgpu_topology(desc->primitive_type);
    wgpu_pip_desc.primitive.stripIndexFormat = _sg_wgpu_stripindexformat(desc->primitive_type, desc->index_type);
    wgpu_pip_desc.primitive.frontFace = _sg_wgpu_frontface(desc->face_winding);
    wgpu_pip_desc.primitive.cullMode = _sg_wgpu_cullmode(desc->cull_mode);
    if (SG_PIXELFORMAT_NONE != desc->depth.pixel_format) {
        wgpu_ds_state.format = _sg_wgpu_textureformat(desc->depth.pixel_format);
        wgpu_ds_state.depthWriteEnabled = desc->depth.write_enabled;
        wgpu_ds_state.depthCompare = _sg_wgpu_comparefunc(desc->depth.compare);
        wgpu_ds_state.stencilFront.compare = _sg_wgpu_comparefunc(desc->stencil.front.compare);
        wgpu_ds_state.stencilFront.failOp = _sg_wgpu_stencilop(desc->stencil.front.fail_op);
        wgpu_ds_state.stencilFront.depthFailOp = _sg_wgpu_stencilop(desc->stencil.front.depth_fail_op);
        wgpu_ds_state.stencilFront.passOp = _sg_wgpu_stencilop(desc->stencil.front.pass_op);
        wgpu_ds_state.stencilBack.compare = _sg_wgpu_comparefunc(desc->stencil.back.compare);
        wgpu_ds_state.stencilBack.failOp = _sg_wgpu_stencilop(desc->stencil.back.fail_op);
        wgpu_ds_state.stencilBack.depthFailOp = _sg_wgpu_stencilop(desc->stencil.back.depth_fail_op);
        wgpu_ds_state.stencilBack.passOp = _sg_wgpu_stencilop(desc->stencil.back.pass_op);
        wgpu_ds_state.stencilReadMask = desc->stencil.read_mask;
        wgpu_ds_state.stencilWriteMask = desc->stencil.write_mask;
        wgpu_ds_state.depthBias = (int32_t)desc->depth.bias;
        wgpu_ds_state.depthBiasSlopeScale = desc->depth.bias_slope_scale;
        wgpu_ds_state.depthBiasClamp = desc->depth.bias_clamp;
        wgpu_pip_desc.depthStencil = &wgpu_ds_state;
    }
    wgpu_pip_desc.multisample.count = (uint32_t)desc->sample_count;
    wgpu_pip_desc.multisample.mask = 0xFFFFFFFF;
    wgpu_pip_desc.multisample.alphaToCoverageEnabled = desc->alpha_to_coverage_enabled;
    if (desc->color_count > 0) {
        wgpu_frag_state.module = shd->wgpu.stage[SG_SHADERSTAGE_FS].module;
        wgpu_frag_state.entryPoint = shd->wgpu.stage[SG_SHADERSTAGE_FS].entry.buf;
        wgpu_frag_state.targetCount = (size_t)desc->color_count;
        wgpu_frag_state.targets = &wgpu_ctgt_state[0];
        for (int i = 0; i < desc->color_count; i++) {
            SOKOL_ASSERT(i < SG_MAX_COLOR_ATTACHMENTS);
            wgpu_ctgt_state[i].format = _sg_wgpu_textureformat(desc->colors[i].pixel_format);
            wgpu_ctgt_state[i].writeMask = _sg_wgpu_colorwritemask(desc->colors[i].write_mask);
            if (desc->colors[i].blend.enabled) {
                wgpu_ctgt_state[i].blend = &wgpu_blend_state[i];
                wgpu_blend_state[i].color.operation = _sg_wgpu_blendop(desc->colors[i].blend.op_rgb);
                wgpu_blend_state[i].color.srcFactor = _sg_wgpu_blendfactor(desc->colors[i].blend.src_factor_rgb);
                wgpu_blend_state[i].color.dstFactor = _sg_wgpu_blendfactor(desc->colors[i].blend.dst_factor_rgb);
                wgpu_blend_state[i].alpha.operation = _sg_wgpu_blendop(desc->colors[i].blend.op_alpha);
                wgpu_blend_state[i].alpha.srcFactor = _sg_wgpu_blendfactor(desc->colors[i].blend.src_factor_alpha);
                wgpu_blend_state[i].alpha.dstFactor = _sg_wgpu_blendfactor(desc->colors[i].blend.dst_factor_alpha);
            }
        }
        wgpu_pip_desc.fragment = &wgpu_frag_state;
    }
    pip->wgpu.pip = wgpuDeviceCreateRenderPipeline(_sg.wgpu.dev, &wgpu_pip_desc);
    wgpuPipelineLayoutRelease(wgpu_pip_layout);
    if (0 == pip->wgpu.pip) {
        _SG_ERROR(WGPU_CREATE_RENDER_PIPELINE_FAILED);
        return SG_RESOURCESTATE_FAILED;
    }
    return SG_RESOURCESTATE_VALID;
}

_SOKOL_PRIVATE void _sg_wgpu_discard_pipeline(_sg_pipeline_t* pip) {
    SOKOL_ASSERT(pip);
    if (pip == _sg.wgpu.cur_pipeline) {
        _sg.wgpu.cur_pipeline = 0;
        _sg.wgpu.cur_pipeline_id.id = SG_INVALID_ID;
    }
    if (pip->wgpu.pip) {
        wgpuRenderPipelineRelease(pip->wgpu.pip);
        pip->wgpu.pip = 0;
    }
}

_SOKOL_PRIVATE sg_resource_state _sg_wgpu_create_pass(_sg_pass_t* pass, _sg_image_t** color_images, _sg_image_t** resolve_images, _sg_image_t* ds_img, const sg_pass_desc* desc) {
    SOKOL_ASSERT(pass && desc);
    SOKOL_ASSERT(color_images && resolve_images);

    // copy image pointers and create renderable wgpu texture views
    for (int i = 0; i < pass->cmn.num_color_atts; i++) {
        const sg_pass_attachment_desc* color_desc = &desc->color_attachments[i];
        _SOKOL_UNUSED(color_desc);
        SOKOL_ASSERT(color_desc->image.id != SG_INVALID_ID);
        SOKOL_ASSERT(0 == pass->wgpu.color_atts[i].image);
        SOKOL_ASSERT(color_images[i] && (color_images[i]->slot.id == color_desc->image.id));
        SOKOL_ASSERT(_sg_is_valid_rendertarget_color_format(color_images[i]->cmn.pixel_format));
        SOKOL_ASSERT(color_images[i]->wgpu.tex);
        pass->wgpu.color_atts[i].image = color_images[i];

        WGPUTextureViewDescriptor wgpu_color_view_desc;
        _sg_clear(&wgpu_color_view_desc, sizeof(wgpu_color_view_desc));
        wgpu_color_view_desc.baseMipLevel = (uint32_t) color_desc->mip_level;
        wgpu_color_view_desc.mipLevelCount = 1;
        wgpu_color_view_desc.baseArrayLayer = (uint32_t) color_desc->slice;
        wgpu_color_view_desc.arrayLayerCount = 1;
        pass->wgpu.color_atts[i].view = wgpuTextureCreateView(color_images[i]->wgpu.tex, &wgpu_color_view_desc);
        if (0 == pass->wgpu.color_atts[i].view) {
            _SG_ERROR(WGPU_PASS_CREATE_TEXTURE_VIEW_FAILED);
            return SG_RESOURCESTATE_FAILED;
        }

        const sg_pass_attachment_desc* resolve_desc = &desc->resolve_attachments[i];
        if (resolve_desc->image.id != SG_INVALID_ID) {
            SOKOL_ASSERT(0 == pass->wgpu.resolve_atts[i].image);
            SOKOL_ASSERT(resolve_images[i] && (resolve_images[i]->slot.id == resolve_desc->image.id));
            SOKOL_ASSERT(color_images[i] && (color_images[i]->cmn.pixel_format == resolve_images[i]->cmn.pixel_format));
            SOKOL_ASSERT(resolve_images[i]->wgpu.tex);
            pass->wgpu.resolve_atts[i].image = resolve_images[i];

            WGPUTextureViewDescriptor wgpu_resolve_view_desc;
            _sg_clear(&wgpu_resolve_view_desc, sizeof(wgpu_resolve_view_desc));
            wgpu_resolve_view_desc.baseMipLevel = (uint32_t) resolve_desc->mip_level;
            wgpu_resolve_view_desc.mipLevelCount = 1;
            wgpu_resolve_view_desc.baseArrayLayer = (uint32_t) resolve_desc->slice;
            wgpu_resolve_view_desc.arrayLayerCount = 1;
            pass->wgpu.resolve_atts[i].view = wgpuTextureCreateView(resolve_images[i]->wgpu.tex, &wgpu_resolve_view_desc);
            if (0 == pass->wgpu.resolve_atts[i].view) {
                _SG_ERROR(WGPU_PASS_CREATE_TEXTURE_VIEW_FAILED);
                return SG_RESOURCESTATE_FAILED;
            }
        }
    }
    SOKOL_ASSERT(0 == pass->wgpu.ds_att.image);
    const sg_pass_attachment_desc* ds_desc = &desc->depth_stencil_attachment;
    if (ds_desc->image.id != SG_INVALID_ID) {
        SOKOL_ASSERT(ds_img && (ds_img->slot.id == ds_desc->image.id));
        SOKOL_ASSERT(_sg_is_valid_rendertarget_depth_format(ds_img->cmn.pixel_format));
        SOKOL_ASSERT(ds_img->wgpu.tex);
        pass->wgpu.ds_att.image = ds_img;

        WGPUTextureViewDescriptor wgpu_ds_view_desc;
        _sg_clear(&wgpu_ds_view_desc, sizeof(wgpu_ds_view_desc));
        wgpu_ds_view_desc.baseMipLevel = (uint32_t) ds_desc->mip_level;
        wgpu_ds_view_desc.mipLevelCount = 1;
        wgpu_ds_view_desc.baseArrayLayer = (uint32_t) ds_desc->slice;
        wgpu_ds_view_desc.arrayLayerCount = 1;
        pass->wgpu.ds_att.view = wgpuTextureCreateView(ds_img->wgpu.tex, &wgpu_ds_view_desc);
        if (0 == pass->wgpu.ds_att.view) {
            _SG_ERROR(WGPU_PASS_CREATE_TEXTURE_VIEW_FAILED);
            return SG_RESOURCESTATE_FAILED;
        }
    }
    return SG_RESOURCESTATE_VALID;
}

_SOKOL_PRIVATE void _sg_wgpu_discard_pass(_sg_pass_t* pass) {
    SOKOL_ASSERT(pass);
    for (int i = 0; i < pass->cmn.num_color_atts; i++) {
        if (pass->wgpu.color_atts[i].view) {
            wgpuTextureViewRelease(pass->wgpu.color_atts[i].view);
            pass->wgpu.color_atts[i].view = 0;
        }
        if (pass->wgpu.resolve_atts[i].view) {
            wgpuTextureViewRelease(pass->wgpu.resolve_atts[i].view);
            pass->wgpu.resolve_atts[i].view = 0;
        }
    }
    if (pass->wgpu.ds_att.view) {
        wgpuTextureViewRelease(pass->wgpu.ds_att.view);
        pass->wgpu.ds_att.view = 0;
    }
}

_SOKOL_PRIVATE _sg_image_t* _sg_wgpu_pass_color_image(const _sg_pass_t* pass, int index) {
    SOKOL_ASSERT(pass && (index >= 0) && (index < SG_MAX_COLOR_ATTACHMENTS));
    // NOTE: may return null
    return pass->wgpu.color_atts[index].image;
}

_SOKOL_PRIVATE _sg_image_t* _sg_wgpu_pass_resolve_image(const _sg_pass_t* pass, int index) {
    SOKOL_ASSERT(pass && (index >= 0) && (index < SG_MAX_COLOR_ATTACHMENTS));
    // NOTE: may return null
    return pass->wgpu.resolve_atts[index].image;
}

_SOKOL_PRIVATE _sg_image_t* _sg_wgpu_pass_ds_image(const _sg_pass_t* pass) {
    // NOTE: may return null
    SOKOL_ASSERT(pass);
    return pass->wgpu.ds_att.image;
}

_SOKOL_PRIVATE void _sg_wgpu_init_color_att(WGPURenderPassColorAttachment* wgpu_att, const sg_color_attachment_action* action, WGPUTextureView color_view, WGPUTextureView resolve_view) {
    wgpu_att->view = color_view;
    wgpu_att->resolveTarget = resolve_view;
    wgpu_att->loadOp = _sg_wgpu_load_op(color_view, action->load_action);
    wgpu_att->storeOp = _sg_wgpu_store_op(color_view, action->store_action);
    wgpu_att->clearValue.r = action->clear_value.r;
    wgpu_att->clearValue.g = action->clear_value.g;
    wgpu_att->clearValue.b = action->clear_value.b;
    wgpu_att->clearValue.a = action->clear_value.a;
}

_SOKOL_PRIVATE void _sg_wgpu_init_ds_att(WGPURenderPassDepthStencilAttachment* wgpu_att, const sg_pass_action* action, sg_pixel_format fmt, WGPUTextureView view) {
    wgpu_att->view = view;
    wgpu_att->depthLoadOp = _sg_wgpu_load_op(view, action->depth.load_action);
    wgpu_att->depthStoreOp = _sg_wgpu_store_op(view, action->depth.store_action);
    wgpu_att->depthClearValue = action->depth.clear_value;
    wgpu_att->depthReadOnly = false;
    if (_sg_is_depth_stencil_format(fmt)) {
        wgpu_att->stencilLoadOp = _sg_wgpu_load_op(view, action->stencil.load_action);
        wgpu_att->stencilStoreOp = _sg_wgpu_store_op(view, action->stencil.store_action);
    } else {
        wgpu_att->stencilLoadOp = WGPULoadOp_Undefined;
        wgpu_att->stencilStoreOp = WGPUStoreOp_Undefined;
    }
    wgpu_att->stencilClearValue = action->stencil.clear_value;
    wgpu_att->stencilReadOnly = false;
}

_SOKOL_PRIVATE void _sg_wgpu_begin_pass(_sg_pass_t* pass, const sg_pass_action* action, int w, int h) {
    SOKOL_ASSERT(action);
    SOKOL_ASSERT(!_sg.wgpu.in_pass);
    SOKOL_ASSERT(_sg.wgpu.cmd_enc);
    SOKOL_ASSERT(_sg.wgpu.dev);
    SOKOL_ASSERT(_sg.wgpu.render_view_cb || _sg.wgpu.render_view_userdata_cb);
    SOKOL_ASSERT(_sg.wgpu.resolve_view_cb || _sg.wgpu.resolve_view_userdata_cb);
    SOKOL_ASSERT(_sg.wgpu.depth_stencil_view_cb || _sg.wgpu.depth_stencil_view_userdata_cb);
    _sg.wgpu.in_pass = true;
    _sg.wgpu.cur_width = w;
    _sg.wgpu.cur_height = h;
    _sg.wgpu.cur_pipeline = 0;
    _sg.wgpu.cur_pipeline_id.id = SG_INVALID_ID;

    WGPURenderPassDescriptor wgpu_pass_desc;
    WGPURenderPassColorAttachment wgpu_color_att[SG_MAX_COLOR_ATTACHMENTS];
    WGPURenderPassDepthStencilAttachment wgpu_ds_att;
    _sg_clear(&wgpu_pass_desc, sizeof(wgpu_pass_desc));
    _sg_clear(&wgpu_color_att, sizeof(wgpu_color_att));
    _sg_clear(&wgpu_ds_att, sizeof(wgpu_ds_att));
    if (pass) {
        SOKOL_ASSERT(pass->slot.state == SG_RESOURCESTATE_VALID);
        for (int i = 0; i < pass->cmn.num_color_atts; i++) {
            _sg_wgpu_init_color_att(&wgpu_color_att[i], &action->colors[i], pass->wgpu.color_atts[i].view, pass->wgpu.resolve_atts[i].view);
        }
        wgpu_pass_desc.colorAttachmentCount = (size_t)pass->cmn.num_color_atts;
        wgpu_pass_desc.colorAttachments = &wgpu_color_att[0];
        if (pass->wgpu.ds_att.image) {
            _sg_wgpu_init_ds_att(&wgpu_ds_att, action, pass->wgpu.ds_att.image->cmn.pixel_format, pass->wgpu.ds_att.view);
            wgpu_pass_desc.depthStencilAttachment = &wgpu_ds_att;
        }
    } else {
        WGPUTextureView wgpu_color_view = _sg.wgpu.render_view_cb ? _sg.wgpu.render_view_cb() : _sg.wgpu.render_view_userdata_cb(_sg.wgpu.user_data);
        WGPUTextureView wgpu_resolve_view = _sg.wgpu.resolve_view_cb ? _sg.wgpu.resolve_view_cb() : _sg.wgpu.resolve_view_userdata_cb(_sg.wgpu.user_data);
        WGPUTextureView wgpu_depth_stencil_view = _sg.wgpu.depth_stencil_view_cb ? _sg.wgpu.depth_stencil_view_cb() : _sg.wgpu.depth_stencil_view_userdata_cb(_sg.wgpu.user_data);
        _sg_wgpu_init_color_att(&wgpu_color_att[0], &action->colors[0], wgpu_color_view, wgpu_resolve_view);
        wgpu_pass_desc.colorAttachmentCount = 1;
        wgpu_pass_desc.colorAttachments = &wgpu_color_att[0];
        if (wgpu_depth_stencil_view) {
            _sg_wgpu_init_ds_att(&wgpu_ds_att, action, _sg.desc.context.depth_format, wgpu_depth_stencil_view);
        }
        wgpu_pass_desc.depthStencilAttachment = &wgpu_ds_att;
    }
    _sg.wgpu.pass_enc = wgpuCommandEncoderBeginRenderPass(_sg.wgpu.cmd_enc, &wgpu_pass_desc);
    SOKOL_ASSERT(_sg.wgpu.pass_enc);

    // clear bindings cache and apply an empty image-sampler bindgroup
    _sg_wgpu_bindings_cache_clear();
    wgpuRenderPassEncoderSetBindGroup(_sg.wgpu.pass_enc, _SG_WGPU_IMAGE_SAMPLER_BINDGROUP_INDEX, _sg.wgpu.empty_bind_group, 0, 0);
    _sg_stats_add(wgpu.bindings.num_set_bindgroup, 1);

    // initial uniform buffer binding (required even if no uniforms are set in the frame)
    _sg_wgpu_uniform_buffer_on_begin_pass();
}

_SOKOL_PRIVATE void _sg_wgpu_end_pass(void) {
    SOKOL_ASSERT(_sg.wgpu.in_pass);
    SOKOL_ASSERT(_sg.wgpu.pass_enc);
    _sg.wgpu.in_pass = false;
    wgpuRenderPassEncoderEnd(_sg.wgpu.pass_enc);
    wgpuRenderPassEncoderRelease(_sg.wgpu.pass_enc);
    _sg.wgpu.pass_enc = 0;
}

_SOKOL_PRIVATE void _sg_wgpu_commit(void) {
    SOKOL_ASSERT(!_sg.wgpu.in_pass);
    SOKOL_ASSERT(_sg.wgpu.cmd_enc);

    _sg_wgpu_uniform_buffer_on_commit();

    WGPUCommandBufferDescriptor cmd_buf_desc;
    _sg_clear(&cmd_buf_desc, sizeof(cmd_buf_desc));
    WGPUCommandBuffer wgpu_cmd_buf = wgpuCommandEncoderFinish(_sg.wgpu.cmd_enc, &cmd_buf_desc);
    SOKOL_ASSERT(wgpu_cmd_buf);
    wgpuCommandEncoderRelease(_sg.wgpu.cmd_enc);
    _sg.wgpu.cmd_enc = 0;

    wgpuQueueSubmit(_sg.wgpu.queue, 1, &wgpu_cmd_buf);
    wgpuCommandBufferRelease(wgpu_cmd_buf);

    // create a new render-command-encoder for next frame
    WGPUCommandEncoderDescriptor cmd_enc_desc;
    _sg_clear(&cmd_enc_desc, sizeof(cmd_enc_desc));
    _sg.wgpu.cmd_enc = wgpuDeviceCreateCommandEncoder(_sg.wgpu.dev, &cmd_enc_desc);
}

_SOKOL_PRIVATE void _sg_wgpu_apply_viewport(int x, int y, int w, int h, bool origin_top_left) {
    SOKOL_ASSERT(_sg.wgpu.in_pass);
    SOKOL_ASSERT(_sg.wgpu.pass_enc);
    // FIXME FIXME FIXME: CLIPPING THE VIEWPORT HERE IS WRONG!!!
    // (but currently required because WebGPU insists that the viewport rectangle must be
    // fully contained inside the framebuffer, but this doesn't make any sense, and also
    // isn't required by the backend APIs)
    const _sg_recti_t clip = _sg_clipi(x, y, w, h, _sg.wgpu.cur_width, _sg.wgpu.cur_height);
    float xf = (float) clip.x;
    float yf = (float) (origin_top_left ? clip.y : (_sg.wgpu.cur_height - (clip.y + clip.h)));
    float wf = (float) clip.w;
    float hf = (float) clip.h;
    wgpuRenderPassEncoderSetViewport(_sg.wgpu.pass_enc, xf, yf, wf, hf, 0.0f, 1.0f);
}

_SOKOL_PRIVATE void _sg_wgpu_apply_scissor_rect(int x, int y, int w, int h, bool origin_top_left) {
    SOKOL_ASSERT(_sg.wgpu.in_pass);
    SOKOL_ASSERT(_sg.wgpu.pass_enc);
    const _sg_recti_t clip = _sg_clipi(x, y, w, h, _sg.wgpu.cur_width, _sg.wgpu.cur_height);
    uint32_t sx = (uint32_t) clip.x;
    uint32_t sy = (uint32_t) (origin_top_left ? clip.y : (_sg.wgpu.cur_height - (clip.y + clip.h)));
    uint32_t sw = (uint32_t) clip.w;
    uint32_t sh = (uint32_t) clip.h;
    wgpuRenderPassEncoderSetScissorRect(_sg.wgpu.pass_enc, sx, sy, sw, sh);
}

_SOKOL_PRIVATE void _sg_wgpu_apply_pipeline(_sg_pipeline_t* pip) {
    SOKOL_ASSERT(pip);
    SOKOL_ASSERT(pip->wgpu.pip);
    SOKOL_ASSERT(_sg.wgpu.in_pass);
    SOKOL_ASSERT(_sg.wgpu.pass_enc);
    _sg.wgpu.use_indexed_draw = (pip->cmn.index_type != SG_INDEXTYPE_NONE);
    _sg.wgpu.cur_pipeline = pip;
    _sg.wgpu.cur_pipeline_id.id = pip->slot.id;
    wgpuRenderPassEncoderSetPipeline(_sg.wgpu.pass_enc, pip->wgpu.pip);
    wgpuRenderPassEncoderSetBlendConstant(_sg.wgpu.pass_enc, &pip->wgpu.blend_color);
    wgpuRenderPassEncoderSetStencilReference(_sg.wgpu.pass_enc, pip->cmn.stencil.ref);
}

_SOKOL_PRIVATE bool _sg_wgpu_apply_bindings(_sg_bindings_t* bnd) {
    SOKOL_ASSERT(_sg.wgpu.in_pass);
    SOKOL_ASSERT(_sg.wgpu.pass_enc);
    SOKOL_ASSERT(bnd);
    SOKOL_ASSERT(bnd->pip->shader && (bnd->pip->cmn.shader_id.id == bnd->pip->shader->slot.id));
    bool retval = true;
    retval &= _sg_wgpu_apply_index_buffer(bnd);
    retval &= _sg_wgpu_apply_vertex_buffers(bnd);
    retval &= _sg_wgpu_apply_bindgroup(bnd);
    return retval;
}

_SOKOL_PRIVATE void _sg_wgpu_apply_uniforms(sg_shader_stage stage_index, int ub_index, const sg_range* data) {
    const uint32_t alignment = _sg.wgpu.limits.limits.minUniformBufferOffsetAlignment;
    SOKOL_ASSERT(_sg.wgpu.in_pass);
    SOKOL_ASSERT(_sg.wgpu.pass_enc);
    SOKOL_ASSERT(_sg.wgpu.uniform.staging);
    SOKOL_ASSERT((_sg.wgpu.uniform.offset + data->size) <= _sg.wgpu.uniform.num_bytes);
    SOKOL_ASSERT((_sg.wgpu.uniform.offset & (alignment - 1)) == 0);
    SOKOL_ASSERT(_sg.wgpu.cur_pipeline && _sg.wgpu.cur_pipeline->shader);
    SOKOL_ASSERT(_sg.wgpu.cur_pipeline->slot.id == _sg.wgpu.cur_pipeline_id.id);
    SOKOL_ASSERT(_sg.wgpu.cur_pipeline->shader->slot.id == _sg.wgpu.cur_pipeline->cmn.shader_id.id);
    SOKOL_ASSERT(ub_index < _sg.wgpu.cur_pipeline->shader->cmn.stage[stage_index].num_uniform_blocks);
    SOKOL_ASSERT(data->size <= _sg.wgpu.cur_pipeline->shader->cmn.stage[stage_index].uniform_blocks[ub_index].size);
    SOKOL_ASSERT(data->size <= _SG_WGPU_MAX_UNIFORM_UPDATE_SIZE);

    _sg_stats_add(wgpu.uniforms.num_set_bindgroup, 1);
    memcpy(_sg.wgpu.uniform.staging + _sg.wgpu.uniform.offset, data->ptr, data->size);
    _sg.wgpu.uniform.bind.offsets[stage_index][ub_index] = _sg.wgpu.uniform.offset;
    _sg.wgpu.uniform.offset = _sg_roundup_u32(_sg.wgpu.uniform.offset + (uint32_t)data->size, alignment);
    wgpuRenderPassEncoderSetBindGroup(_sg.wgpu.pass_enc,
                                      _SG_WGPU_UNIFORM_BINDGROUP_INDEX,
                                      _sg.wgpu.uniform.bind.group,
                                      SG_NUM_SHADER_STAGES * SG_MAX_SHADERSTAGE_UBS,
                                      &_sg.wgpu.uniform.bind.offsets[0][0]);
}

_SOKOL_PRIVATE void _sg_wgpu_draw(int base_element, int num_elements, int num_instances) {
    SOKOL_ASSERT(_sg.wgpu.in_pass);
    SOKOL_ASSERT(_sg.wgpu.pass_enc);
    SOKOL_ASSERT(_sg.wgpu.cur_pipeline && (_sg.wgpu.cur_pipeline->slot.id == _sg.wgpu.cur_pipeline_id.id));
    if (SG_INDEXTYPE_NONE != _sg.wgpu.cur_pipeline->cmn.index_type) {
        wgpuRenderPassEncoderDrawIndexed(_sg.wgpu.pass_enc, (uint32_t)num_elements, (uint32_t)num_instances, (uint32_t)base_element, 0, 0);
    } else {
        wgpuRenderPassEncoderDraw(_sg.wgpu.pass_enc, (uint32_t)num_elements, (uint32_t)num_instances, (uint32_t)base_element, 0);
    }
}

_SOKOL_PRIVATE void _sg_wgpu_update_buffer(_sg_buffer_t* buf, const sg_range* data) {
    SOKOL_ASSERT(data && data->ptr && (data->size > 0));
    SOKOL_ASSERT(buf);
    _sg_wgpu_copy_buffer_data(buf, 0, data);
}

_SOKOL_PRIVATE void _sg_wgpu_append_buffer(_sg_buffer_t* buf, const sg_range* data, bool new_frame) {
    SOKOL_ASSERT(data && data->ptr && (data->size > 0));
    _SOKOL_UNUSED(new_frame);
    _sg_wgpu_copy_buffer_data(buf, (uint64_t)buf->cmn.append_pos, data);
}

_SOKOL_PRIVATE void _sg_wgpu_update_image(_sg_image_t* img, const sg_image_data* data) {
    SOKOL_ASSERT(img && data);
    _sg_wgpu_copy_image_data(img, img->wgpu.tex, data);
}
#endif

//  ██████  ███████ ███    ██ ███████ ██████  ██  ██████     ██████   █████   ██████ ██   ██ ███████ ███    ██ ██████
// ██       ██      ████   ██ ██      ██   ██ ██ ██          ██   ██ ██   ██ ██      ██  ██  ██      ████   ██ ██   ██
// ██   ███ █████   ██ ██  ██ █████   ██████  ██ ██          ██████  ███████ ██      █████   █████   ██ ██  ██ ██   ██
// ██    ██ ██      ██  ██ ██ ██      ██   ██ ██ ██          ██   ██ ██   ██ ██      ██  ██  ██      ██  ██ ██ ██   ██
//  ██████  ███████ ██   ████ ███████ ██   ██ ██  ██████     ██████  ██   ██  ██████ ██   ██ ███████ ██   ████ ██████
//
// >>generic backend
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

static inline sg_resource_state _sg_create_sampler(_sg_sampler_t* smp, const sg_sampler_desc* desc) {
    #if defined(_SOKOL_ANY_GL)
    return _sg_gl_create_sampler(smp, desc);
    #elif defined(SOKOL_METAL)
    return _sg_mtl_create_sampler(smp, desc);
    #elif defined(SOKOL_D3D11)
    return _sg_d3d11_create_sampler(smp, desc);
    #elif defined(SOKOL_WGPU)
    return _sg_wgpu_create_sampler(smp, desc);
    #elif defined(SOKOL_DUMMY_BACKEND)
    return _sg_dummy_create_sampler(smp, desc);
    #else
    #error("INVALID BACKEND");
    #endif
}

static inline void _sg_discard_sampler(_sg_sampler_t* smp) {
    #if defined(_SOKOL_ANY_GL)
    _sg_gl_discard_sampler(smp);
    #elif defined(SOKOL_METAL)
    _sg_mtl_discard_sampler(smp);
    #elif defined(SOKOL_D3D11)
    _sg_d3d11_discard_sampler(smp);
    #elif defined(SOKOL_WGPU)
    _sg_wgpu_discard_sampler(smp);
    #elif defined(SOKOL_DUMMY_BACKEND)
    _sg_dummy_discard_sampler(smp);
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

static inline sg_resource_state _sg_create_pass(_sg_pass_t* pass, _sg_image_t** color_images, _sg_image_t** resolve_images, _sg_image_t* ds_image, const sg_pass_desc* desc) {
    #if defined(_SOKOL_ANY_GL)
    return _sg_gl_create_pass(pass, color_images, resolve_images, ds_image, desc);
    #elif defined(SOKOL_METAL)
    return _sg_mtl_create_pass(pass, color_images, resolve_images, ds_image, desc);
    #elif defined(SOKOL_D3D11)
    return _sg_d3d11_create_pass(pass, color_images, resolve_images, ds_image, desc);
    #elif defined(SOKOL_WGPU)
    return _sg_wgpu_create_pass(pass, color_images, resolve_images, ds_image, desc);
    #elif defined(SOKOL_DUMMY_BACKEND)
    return _sg_dummy_create_pass(pass, color_images, resolve_images, ds_image, desc);
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

static inline _sg_image_t* _sg_pass_resolve_image(const _sg_pass_t* pass, int index) {
    #if defined(_SOKOL_ANY_GL)
    return _sg_gl_pass_resolve_image(pass, index);
    #elif defined(SOKOL_METAL)
    return _sg_mtl_pass_resolve_image(pass, index);
    #elif defined(SOKOL_D3D11)
    return _sg_d3d11_pass_resolve_image(pass, index);
    #elif defined(SOKOL_WGPU)
    return _sg_wgpu_pass_resolve_image(pass, index);
    #elif defined(SOKOL_DUMMY_BACKEND)
    return _sg_dummy_pass_resolve_image(pass, index);
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

static inline bool _sg_apply_bindings(_sg_bindings_t* bnd) {
    #if defined(_SOKOL_ANY_GL)
    return _sg_gl_apply_bindings(bnd);
    #elif defined(SOKOL_METAL)
    return _sg_mtl_apply_bindings(bnd);
    #elif defined(SOKOL_D3D11)
    return _sg_d3d11_apply_bindings(bnd);
    #elif defined(SOKOL_WGPU)
    return _sg_wgpu_apply_bindings(bnd);
    #elif defined(SOKOL_DUMMY_BACKEND)
    return _sg_dummy_apply_bindings(bnd);
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

static inline void _sg_append_buffer(_sg_buffer_t* buf, const sg_range* data, bool new_frame) {
    #if defined(_SOKOL_ANY_GL)
    _sg_gl_append_buffer(buf, data, new_frame);
    #elif defined(SOKOL_METAL)
    _sg_mtl_append_buffer(buf, data, new_frame);
    #elif defined(SOKOL_D3D11)
    _sg_d3d11_append_buffer(buf, data, new_frame);
    #elif defined(SOKOL_WGPU)
    _sg_wgpu_append_buffer(buf, data, new_frame);
    #elif defined(SOKOL_DUMMY_BACKEND)
    _sg_dummy_append_buffer(buf, data, new_frame);
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

static inline void _sg_push_debug_group(const char* name) {
    #if defined(SOKOL_METAL)
    _sg_mtl_push_debug_group(name);
    #else
    _SOKOL_UNUSED(name);
    #endif
}

static inline void _sg_pop_debug_group(void) {
    #if defined(SOKOL_METAL)
    _sg_mtl_pop_debug_group();
    #endif
}

// ██████   ██████   ██████  ██
// ██   ██ ██    ██ ██    ██ ██
// ██████  ██    ██ ██    ██ ██
// ██      ██    ██ ██    ██ ██
// ██       ██████   ██████  ███████
//
// >>pool
_SOKOL_PRIVATE void _sg_init_pool(_sg_pool_t* pool, int num) {
    SOKOL_ASSERT(pool && (num >= 1));
    // slot 0 is reserved for the 'invalid id', so bump the pool size by 1
    pool->size = num + 1;
    pool->queue_top = 0;
    // generation counters indexable by pool slot index, slot 0 is reserved
    size_t gen_ctrs_size = sizeof(uint32_t) * (size_t)pool->size;
    pool->gen_ctrs = (uint32_t*)_sg_malloc_clear(gen_ctrs_size);
    // it's not a bug to only reserve 'num' here
    pool->free_queue = (int*) _sg_malloc_clear(sizeof(int) * (size_t)num);
    // never allocate the zero-th pool item since the invalid id is 0
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
    } else {
        // pool exhausted
        return _SG_INVALID_SLOT_INDEX;
    }
}

_SOKOL_PRIVATE void _sg_pool_free_index(_sg_pool_t* pool, int slot_index) {
    SOKOL_ASSERT((slot_index > _SG_INVALID_SLOT_INDEX) && (slot_index < pool->size));
    SOKOL_ASSERT(pool);
    SOKOL_ASSERT(pool->free_queue);
    SOKOL_ASSERT(pool->queue_top < pool->size);
    #ifdef SOKOL_DEBUG
    // debug check against double-free
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

_SOKOL_PRIVATE void _sg_reset_sampler_to_alloc_state(_sg_sampler_t* smp) {
    SOKOL_ASSERT(smp);
    _sg_slot_t slot = smp->slot;
    _sg_clear(smp, sizeof(_sg_sampler_t));
    smp->slot = slot;
    smp->slot.state = SG_RESOURCESTATE_ALLOC;
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
    // note: the pools here will have an additional item, since slot 0 is reserved
    SOKOL_ASSERT((desc->buffer_pool_size > 0) && (desc->buffer_pool_size < _SG_MAX_POOL_SIZE));
    _sg_init_pool(&p->buffer_pool, desc->buffer_pool_size);
    size_t buffer_pool_byte_size = sizeof(_sg_buffer_t) * (size_t)p->buffer_pool.size;
    p->buffers = (_sg_buffer_t*) _sg_malloc_clear(buffer_pool_byte_size);

    SOKOL_ASSERT((desc->image_pool_size > 0) && (desc->image_pool_size < _SG_MAX_POOL_SIZE));
    _sg_init_pool(&p->image_pool, desc->image_pool_size);
    size_t image_pool_byte_size = sizeof(_sg_image_t) * (size_t)p->image_pool.size;
    p->images = (_sg_image_t*) _sg_malloc_clear(image_pool_byte_size);

    SOKOL_ASSERT((desc->sampler_pool_size > 0) && (desc->sampler_pool_size < _SG_MAX_POOL_SIZE));
    _sg_init_pool(&p->sampler_pool, desc->sampler_pool_size);
    size_t sampler_pool_byte_size = sizeof(_sg_sampler_t) * (size_t)p->sampler_pool.size;
    p->samplers = (_sg_sampler_t*) _sg_malloc_clear(sampler_pool_byte_size);

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
    _sg_free(p->samplers);    p->samplers = 0;
    _sg_free(p->images);      p->images = 0;
    _sg_free(p->buffers);     p->buffers = 0;
    _sg_discard_pool(&p->context_pool);
    _sg_discard_pool(&p->pass_pool);
    _sg_discard_pool(&p->pipeline_pool);
    _sg_discard_pool(&p->shader_pool);
    _sg_discard_pool(&p->sampler_pool);
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
    SOKOL_ASSERT(slot->id == SG_INVALID_ID);
    SOKOL_ASSERT(slot->state == SG_RESOURCESTATE_INITIAL);
    uint32_t ctr = ++pool->gen_ctrs[slot_index];
    slot->id = (ctr<<_SG_SLOT_SHIFT)|(slot_index & _SG_SLOT_MASK);
    slot->state = SG_RESOURCESTATE_ALLOC;
    return slot->id;
}

// extract slot index from id
_SOKOL_PRIVATE int _sg_slot_index(uint32_t id) {
    int slot_index = (int) (id & _SG_SLOT_MASK);
    SOKOL_ASSERT(_SG_INVALID_SLOT_INDEX != slot_index);
    return slot_index;
}

// returns pointer to resource by id without matching id check
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

_SOKOL_PRIVATE _sg_sampler_t* _sg_sampler_at(const _sg_pools_t* p, uint32_t smp_id) {
    SOKOL_ASSERT(p && (SG_INVALID_ID != smp_id));
    int slot_index = _sg_slot_index(smp_id);
    SOKOL_ASSERT((slot_index > _SG_INVALID_SLOT_INDEX) && (slot_index < p->sampler_pool.size));
    return &p->samplers[slot_index];
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

// returns pointer to resource with matching id check, may return 0
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

_SOKOL_PRIVATE _sg_sampler_t* _sg_lookup_sampler(const _sg_pools_t* p, uint32_t smp_id) {
    if (SG_INVALID_ID != smp_id) {
        _sg_sampler_t* smp = _sg_sampler_at(p, smp_id);
        if (smp->slot.id == smp_id) {
            return smp;
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
    for (int i = 1; i < p->sampler_pool.size; i++) {
        if (p->samplers[i].slot.ctx_id == ctx_id) {
            sg_resource_state state = p->samplers[i].slot.state;
            if ((state == SG_RESOURCESTATE_VALID) || (state == SG_RESOURCESTATE_FAILED)) {
                _sg_discard_sampler(&p->samplers[i]);
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

// ██    ██  █████  ██      ██ ██████   █████  ████████ ██  ██████  ███    ██
// ██    ██ ██   ██ ██      ██ ██   ██ ██   ██    ██    ██ ██    ██ ████   ██
// ██    ██ ███████ ██      ██ ██   ██ ███████    ██    ██ ██    ██ ██ ██  ██
//  ██  ██  ██   ██ ██      ██ ██   ██ ██   ██    ██    ██ ██    ██ ██  ██ ██
//   ████   ██   ██ ███████ ██ ██████  ██   ██    ██    ██  ██████  ██   ████
//
// >>validation
#if defined(SOKOL_DEBUG)
_SOKOL_PRIVATE void _sg_validate_begin(void) {
    _sg.validate_error = SG_LOGITEM_OK;
}

_SOKOL_PRIVATE bool _sg_validate_end(void) {
    if (_sg.validate_error != SG_LOGITEM_OK) {
        #if !defined(SOKOL_VALIDATE_NON_FATAL)
            _SG_PANIC(VALIDATION_FAILED);
            return false;
        #else
            return false;
        #endif
    } else {
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
        _sg_validate_begin();
        _SG_VALIDATE(desc->_start_canary == 0, VALIDATE_BUFFERDESC_CANARY);
        _SG_VALIDATE(desc->_end_canary == 0, VALIDATE_BUFFERDESC_CANARY);
        _SG_VALIDATE(desc->size > 0, VALIDATE_BUFFERDESC_SIZE);
        bool injected = (0 != desc->gl_buffers[0]) ||
                        (0 != desc->mtl_buffers[0]) ||
                        (0 != desc->d3d11_buffer) ||
                        (0 != desc->wgpu_buffer);
        if (!injected && (desc->usage == SG_USAGE_IMMUTABLE)) {
            _SG_VALIDATE((0 != desc->data.ptr) && (desc->data.size > 0), VALIDATE_BUFFERDESC_DATA);
            _SG_VALIDATE(desc->size == desc->data.size, VALIDATE_BUFFERDESC_DATA_SIZE);
        } else {
            _SG_VALIDATE(0 == desc->data.ptr, VALIDATE_BUFFERDESC_NO_DATA);
        }
        return _sg_validate_end();
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
                _SG_VALIDATE(has_data && has_size, VALIDATE_IMAGEDATA_NODATA);
                const int mip_width = _sg_miplevel_dim(width, mip_index);
                const int mip_height = _sg_miplevel_dim(height, mip_index);
                const int bytes_per_slice = _sg_surface_pitch(fmt, mip_width, mip_height, 1);
                const int expected_size = bytes_per_slice * num_slices;
                _SG_VALIDATE(expected_size == (int)data->subimage[face_index][mip_index].size, VALIDATE_IMAGEDATA_DATA_SIZE);
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
        _sg_validate_begin();
        _SG_VALIDATE(desc->_start_canary == 0, VALIDATE_IMAGEDESC_CANARY);
        _SG_VALIDATE(desc->_end_canary == 0, VALIDATE_IMAGEDESC_CANARY);
        _SG_VALIDATE(desc->width > 0, VALIDATE_IMAGEDESC_WIDTH);
        _SG_VALIDATE(desc->height > 0, VALIDATE_IMAGEDESC_HEIGHT);
        const sg_pixel_format fmt = desc->pixel_format;
        const sg_usage usage = desc->usage;
        const bool injected = (0 != desc->gl_textures[0]) ||
                              (0 != desc->mtl_textures[0]) ||
                              (0 != desc->d3d11_texture) ||
                              (0 != desc->wgpu_texture);
        if (_sg_is_depth_or_depth_stencil_format(fmt)) {
            _SG_VALIDATE(desc->type != SG_IMAGETYPE_3D, VALIDATE_IMAGEDESC_DEPTH_3D_IMAGE);
        }
        if (desc->render_target) {
            SOKOL_ASSERT(((int)fmt >= 0) && ((int)fmt < _SG_PIXELFORMAT_NUM));
            _SG_VALIDATE(_sg.formats[fmt].render, VALIDATE_IMAGEDESC_RT_PIXELFORMAT);
            _SG_VALIDATE(usage == SG_USAGE_IMMUTABLE, VALIDATE_IMAGEDESC_RT_IMMUTABLE);
            _SG_VALIDATE(desc->data.subimage[0][0].ptr==0, VALIDATE_IMAGEDESC_RT_NO_DATA);
            if (desc->sample_count > 1) {
                _SG_VALIDATE(_sg.formats[fmt].msaa, VALIDATE_IMAGEDESC_NO_MSAA_RT_SUPPORT);
                _SG_VALIDATE(desc->num_mipmaps == 1, VALIDATE_IMAGEDESC_MSAA_NUM_MIPMAPS);
                _SG_VALIDATE(desc->type != SG_IMAGETYPE_3D, VALIDATE_IMAGEDESC_MSAA_3D_IMAGE);
            }
        } else {
            _SG_VALIDATE(desc->sample_count == 1, VALIDATE_IMAGEDESC_MSAA_BUT_NO_RT);
            const bool valid_nonrt_fmt = !_sg_is_valid_rendertarget_depth_format(fmt);
            _SG_VALIDATE(valid_nonrt_fmt, VALIDATE_IMAGEDESC_NONRT_PIXELFORMAT);
            const bool is_compressed = _sg_is_compressed_pixel_format(desc->pixel_format);
            const bool is_immutable = (usage == SG_USAGE_IMMUTABLE);
            if (is_compressed) {
                _SG_VALIDATE(is_immutable, VALIDATE_IMAGEDESC_COMPRESSED_IMMUTABLE);
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
            } else {
                // image desc must not have data
                for (int face_index = 0; face_index < SG_CUBEFACE_NUM; face_index++) {
                    for (int mip_index = 0; mip_index < SG_MAX_MIPMAPS; mip_index++) {
                        const bool no_data = 0 == desc->data.subimage[face_index][mip_index].ptr;
                        const bool no_size = 0 == desc->data.subimage[face_index][mip_index].size;
                        if (injected) {
                            _SG_VALIDATE(no_data && no_size, VALIDATE_IMAGEDESC_INJECTED_NO_DATA);
                        }
                        if (!is_immutable) {
                            _SG_VALIDATE(no_data && no_size, VALIDATE_IMAGEDESC_DYNAMIC_NO_DATA);
                        }
                    }
                }
            }
        }
        return _sg_validate_end();
    #endif
}

_SOKOL_PRIVATE bool _sg_validate_sampler_desc(const sg_sampler_desc* desc) {
    #if !defined(SOKOL_DEBUG)
        _SOKOL_UNUSED(desc);
        return true;
    #else
        if (_sg.desc.disable_validation) {
            return true;
        }
        SOKOL_ASSERT(desc);
        _sg_validate_begin();
        _SG_VALIDATE(desc->_start_canary == 0, VALIDATE_SAMPLERDESC_CANARY);
        _SG_VALIDATE(desc->_end_canary == 0, VALIDATE_SAMPLERDESC_CANARY);
        _SG_VALIDATE(desc->min_filter != SG_FILTER_NONE, VALIDATE_SAMPLERDESC_MINFILTER_NONE);
        _SG_VALIDATE(desc->mag_filter != SG_FILTER_NONE, VALIDATE_SAMPLERDESC_MAGFILTER_NONE);
        // restriction from WebGPU: when anisotropy > 1, all filters must be linear
        if (desc->max_anisotropy > 1) {
            _SG_VALIDATE((desc->min_filter == SG_FILTER_LINEAR)
                      && (desc->mag_filter == SG_FILTER_LINEAR)
                      && (desc->mipmap_filter == SG_FILTER_LINEAR),
                      VALIDATE_SAMPLERDESC_ANISTROPIC_REQUIRES_LINEAR_FILTERING);
        }
        return _sg_validate_end();
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
        _sg_validate_begin();
        _SG_VALIDATE(desc->_start_canary == 0, VALIDATE_SHADERDESC_CANARY);
        _SG_VALIDATE(desc->_end_canary == 0, VALIDATE_SHADERDESC_CANARY);
        #if defined(SOKOL_D3D11)
            _SG_VALIDATE(0 != desc->attrs[0].sem_name, VALIDATE_SHADERDESC_ATTR_SEMANTICS);
        #endif
        #if defined(SOKOL_GLCORE33) || defined(SOKOL_GLES3) || defined(SOKOL_WGPU)
            // on GL or WebGPU, must provide shader source code
            _SG_VALIDATE(0 != desc->vs.source, VALIDATE_SHADERDESC_SOURCE);
            _SG_VALIDATE(0 != desc->fs.source, VALIDATE_SHADERDESC_SOURCE);
        #elif defined(SOKOL_METAL) || defined(SOKOL_D3D11)
            // on Metal or D3D11, must provide shader source code or byte code
            _SG_VALIDATE((0 != desc->vs.source)||(0 != desc->vs.bytecode.ptr), VALIDATE_SHADERDESC_SOURCE_OR_BYTECODE);
            _SG_VALIDATE((0 != desc->fs.source)||(0 != desc->fs.bytecode.ptr), VALIDATE_SHADERDESC_SOURCE_OR_BYTECODE);
        #else
            // Dummy Backend, don't require source or bytecode
        #endif
        for (int i = 0; i < SG_MAX_VERTEX_ATTRIBUTES; i++) {
            if (desc->attrs[i].name) {
                _SG_VALIDATE(strlen(desc->attrs[i].name) < _SG_STRING_SIZE, VALIDATE_SHADERDESC_ATTR_STRING_TOO_LONG);
            }
            if (desc->attrs[i].sem_name) {
                _SG_VALIDATE(strlen(desc->attrs[i].sem_name) < _SG_STRING_SIZE, VALIDATE_SHADERDESC_ATTR_STRING_TOO_LONG);
            }
        }
        // if shader byte code, the size must also be provided
        if (0 != desc->vs.bytecode.ptr) {
            _SG_VALIDATE(desc->vs.bytecode.size > 0, VALIDATE_SHADERDESC_NO_BYTECODE_SIZE);
        }
        if (0 != desc->fs.bytecode.ptr) {
            _SG_VALIDATE(desc->fs.bytecode.size > 0, VALIDATE_SHADERDESC_NO_BYTECODE_SIZE);
        }
        for (int stage_index = 0; stage_index < SG_NUM_SHADER_STAGES; stage_index++) {
            const sg_shader_stage_desc* stage_desc = (stage_index == 0)? &desc->vs : &desc->fs;
            bool uniform_blocks_continuous = true;
            for (int ub_index = 0; ub_index < SG_MAX_SHADERSTAGE_UBS; ub_index++) {
                const sg_shader_uniform_block_desc* ub_desc = &stage_desc->uniform_blocks[ub_index];
                if (ub_desc->size > 0) {
                    _SG_VALIDATE(uniform_blocks_continuous, VALIDATE_SHADERDESC_NO_CONT_UBS);
                    #if defined(_SOKOL_ANY_GL)
                    bool uniforms_continuous = true;
                    uint32_t uniform_offset = 0;
                    int num_uniforms = 0;
                    for (int u_index = 0; u_index < SG_MAX_UB_MEMBERS; u_index++) {
                        const sg_shader_uniform_desc* u_desc = &ub_desc->uniforms[u_index];
                        if (u_desc->type != SG_UNIFORMTYPE_INVALID) {
                            _SG_VALIDATE(uniforms_continuous, VALIDATE_SHADERDESC_NO_CONT_UB_MEMBERS);
                            #if defined(SOKOL_GLES3)
                            _SG_VALIDATE(0 != u_desc->name, VALIDATE_SHADERDESC_UB_MEMBER_NAME);
                            #endif
                            const int array_count = u_desc->array_count;
                            _SG_VALIDATE(array_count > 0, VALIDATE_SHADERDESC_UB_ARRAY_COUNT);
                            const uint32_t u_align = _sg_uniform_alignment(u_desc->type, array_count, ub_desc->layout);
                            const uint32_t u_size  = _sg_uniform_size(u_desc->type, array_count, ub_desc->layout);
                            uniform_offset = _sg_align_u32(uniform_offset, u_align);
                            uniform_offset += u_size;
                            num_uniforms++;
                            // with std140, arrays are only allowed for FLOAT4, INT4, MAT4
                            if (ub_desc->layout == SG_UNIFORMLAYOUT_STD140) {
                                if (array_count > 1) {
                                    _SG_VALIDATE((u_desc->type == SG_UNIFORMTYPE_FLOAT4) || (u_desc->type == SG_UNIFORMTYPE_INT4) || (u_desc->type == SG_UNIFORMTYPE_MAT4), VALIDATE_SHADERDESC_UB_STD140_ARRAY_TYPE);
                                }
                            }
                        } else {
                            uniforms_continuous = false;
                        }
                    }
                    if (ub_desc->layout == SG_UNIFORMLAYOUT_STD140) {
                        uniform_offset = _sg_align_u32(uniform_offset, 16);
                    }
                    _SG_VALIDATE((size_t)uniform_offset == ub_desc->size, VALIDATE_SHADERDESC_UB_SIZE_MISMATCH);
                    _SG_VALIDATE(num_uniforms > 0, VALIDATE_SHADERDESC_NO_UB_MEMBERS);
                    #endif
                } else {
                    uniform_blocks_continuous = false;
                }
            }
            bool images_continuous = true;
            int num_images = 0;
            for (int img_index = 0; img_index < SG_MAX_SHADERSTAGE_IMAGES; img_index++) {
                const sg_shader_image_desc* img_desc = &stage_desc->images[img_index];
                if (img_desc->used) {
                    _SG_VALIDATE(images_continuous, VALIDATE_SHADERDESC_NO_CONT_IMAGES);
                    num_images++;
                } else {
                    images_continuous = false;
                }
            }
            bool samplers_continuous = true;
            int num_samplers = 0;
            for (int smp_index = 0; smp_index < SG_MAX_SHADERSTAGE_SAMPLERS; smp_index++) {
                const sg_shader_sampler_desc* smp_desc = &stage_desc->samplers[smp_index];
                if (smp_desc->used) {
                    _SG_VALIDATE(samplers_continuous, VALIDATE_SHADERDESC_NO_CONT_SAMPLERS);
                    num_samplers++;
                } else {
                    samplers_continuous = false;
                }
            }
            bool image_samplers_continuous = true;
            int num_image_samplers = 0;
            for (int img_smp_index = 0; img_smp_index < SG_MAX_SHADERSTAGE_IMAGESAMPLERPAIRS; img_smp_index++) {
                const sg_shader_image_sampler_pair_desc* img_smp_desc = &stage_desc->image_sampler_pairs[img_smp_index];
                if (img_smp_desc->used) {
                    _SG_VALIDATE(image_samplers_continuous, VALIDATE_SHADERDESC_NO_CONT_IMAGE_SAMPLER_PAIRS);
                    num_image_samplers++;
                    const bool img_slot_in_range = (img_smp_desc->image_slot >= 0) && (img_smp_desc->image_slot < SG_MAX_SHADERSTAGE_IMAGES);
                    const bool smp_slot_in_range = (img_smp_desc->sampler_slot >= 0) && (img_smp_desc->sampler_slot < SG_MAX_SHADERSTAGE_SAMPLERS);
                    _SG_VALIDATE(img_slot_in_range && (img_smp_desc->image_slot < num_images), VALIDATE_SHADERDESC_IMAGE_SAMPLER_PAIR_IMAGE_SLOT_OUT_OF_RANGE);
                    _SG_VALIDATE(smp_slot_in_range && (img_smp_desc->sampler_slot < num_samplers), VALIDATE_SHADERDESC_IMAGE_SAMPLER_PAIR_IMAGE_SLOT_OUT_OF_RANGE);
                    #if defined(_SOKOL_ANY_GL)
                    _SG_VALIDATE(img_smp_desc->glsl_name != 0, VALIDATE_SHADERDESC_IMAGE_SAMPLER_PAIR_NAME_REQUIRED_FOR_GL);
                    #endif
                    if (img_slot_in_range && smp_slot_in_range) {
                        const sg_shader_image_desc* img_desc = &stage_desc->images[img_smp_desc->image_slot];
                        const sg_shader_sampler_desc* smp_desc = &stage_desc->samplers[img_smp_desc->sampler_slot];
                        const bool needs_nonfiltering = (img_desc->sample_type == SG_IMAGESAMPLETYPE_UINT)
                                                     || (img_desc->sample_type == SG_IMAGESAMPLETYPE_SINT)
                                                     || (img_desc->sample_type == SG_IMAGESAMPLETYPE_UNFILTERABLE_FLOAT);
                        const bool needs_comparison = img_desc->sample_type == SG_IMAGESAMPLETYPE_DEPTH;
                        if (needs_nonfiltering) {
                            _SG_VALIDATE(needs_nonfiltering && (smp_desc->sampler_type == SG_SAMPLERTYPE_NONFILTERING), VALIDATE_SHADERDESC_NONFILTERING_SAMPLER_REQUIRED);
                        }
                        if (needs_comparison) {
                            _SG_VALIDATE(needs_comparison && (smp_desc->sampler_type == SG_SAMPLERTYPE_COMPARISON), VALIDATE_SHADERDESC_COMPARISON_SAMPLER_REQUIRED);
                        }
                    }
                } else {
                    _SG_VALIDATE(img_smp_desc->glsl_name == 0, VALIDATE_SHADERDESC_IMAGE_SAMPLER_PAIR_HAS_NAME_BUT_NOT_USED);
                    _SG_VALIDATE(img_smp_desc->image_slot == 0, VALIDATE_SHADERDESC_IMAGE_SAMPLER_PAIR_HAS_IMAGE_BUT_NOT_USED);
                    _SG_VALIDATE(img_smp_desc->sampler_slot == 0, VALIDATE_SHADERDESC_IMAGE_SAMPLER_PAIR_HAS_SAMPLER_BUT_NOT_USED);
                    image_samplers_continuous = false;
                }
            }
            // each image and sampler must be referenced by an image sampler
            const uint32_t expected_img_slot_mask = (uint32_t)((1 << num_images) - 1);
            const uint32_t expected_smp_slot_mask = (uint32_t)((1 << num_samplers) - 1);
            uint32_t actual_img_slot_mask = 0;
            uint32_t actual_smp_slot_mask = 0;
            for (int img_smp_index = 0; img_smp_index < num_image_samplers; img_smp_index++) {
                const sg_shader_image_sampler_pair_desc* img_smp_desc = &stage_desc->image_sampler_pairs[img_smp_index];
                actual_img_slot_mask |= (1 << ((uint32_t)img_smp_desc->image_slot & 31));
                actual_smp_slot_mask |= (1 << ((uint32_t)img_smp_desc->sampler_slot & 31));
            }
            _SG_VALIDATE(expected_img_slot_mask == actual_img_slot_mask, VALIDATE_SHADERDESC_IMAGE_NOT_REFERENCED_BY_IMAGE_SAMPLER_PAIRS);
            _SG_VALIDATE(expected_smp_slot_mask == actual_smp_slot_mask, VALIDATE_SHADERDESC_SAMPLER_NOT_REFERENCED_BY_IMAGE_SAMPLER_PAIRS);
        }
        return _sg_validate_end();
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
        _sg_validate_begin();
        _SG_VALIDATE(desc->_start_canary == 0, VALIDATE_PIPELINEDESC_CANARY);
        _SG_VALIDATE(desc->_end_canary == 0, VALIDATE_PIPELINEDESC_CANARY);
        _SG_VALIDATE(desc->shader.id != SG_INVALID_ID, VALIDATE_PIPELINEDESC_SHADER);
        for (int buf_index = 0; buf_index < SG_MAX_VERTEX_BUFFERS; buf_index++) {
            const sg_vertex_buffer_layout_state* l_state = &desc->layout.buffers[buf_index];
            if (l_state->stride == 0) {
                continue;
            }
            _SG_VALIDATE(_sg_multiple_u64((uint64_t)l_state->stride, 4), VALIDATE_PIPELINEDESC_LAYOUT_STRIDE4);
        }
        _SG_VALIDATE(desc->layout.attrs[0].format != SG_VERTEXFORMAT_INVALID, VALIDATE_PIPELINEDESC_NO_ATTRS);
        const _sg_shader_t* shd = _sg_lookup_shader(&_sg.pools, desc->shader.id);
        _SG_VALIDATE(0 != shd, VALIDATE_PIPELINEDESC_SHADER);
        if (shd) {
            _SG_VALIDATE(shd->slot.state == SG_RESOURCESTATE_VALID, VALIDATE_PIPELINEDESC_SHADER);
            bool attrs_cont = true;
            for (int attr_index = 0; attr_index < SG_MAX_VERTEX_ATTRIBUTES; attr_index++) {
                const sg_vertex_attr_state* a_state = &desc->layout.attrs[attr_index];
                if (a_state->format == SG_VERTEXFORMAT_INVALID) {
                    attrs_cont = false;
                    continue;
                }
                _SG_VALIDATE(attrs_cont, VALIDATE_PIPELINEDESC_NO_ATTRS);
                SOKOL_ASSERT(a_state->buffer_index < SG_MAX_VERTEX_BUFFERS);
                #if defined(SOKOL_D3D11)
                // on D3D11, semantic names (and semantic indices) must be provided
                _SG_VALIDATE(!_sg_strempty(&shd->d3d11.attrs[attr_index].sem_name), VALIDATE_PIPELINEDESC_ATTR_SEMANTICS);
                #endif
            }
        }
        return _sg_validate_end();
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
        _sg_validate_begin();
        _SG_VALIDATE(desc->_start_canary == 0, VALIDATE_PASSDESC_CANARY);
        _SG_VALIDATE(desc->_end_canary == 0, VALIDATE_PASSDESC_CANARY);
        bool atts_cont = true;
        int color_width = -1, color_height = -1, color_sample_count = -1;
        bool has_color_atts = false;
        for (int att_index = 0; att_index < SG_MAX_COLOR_ATTACHMENTS; att_index++) {
            const sg_pass_attachment_desc* att = &desc->color_attachments[att_index];
            if (att->image.id == SG_INVALID_ID) {
                atts_cont = false;
                continue;
            }
            _SG_VALIDATE(atts_cont, VALIDATE_PASSDESC_NO_CONT_COLOR_ATTS);
            has_color_atts = true;
            const _sg_image_t* img = _sg_lookup_image(&_sg.pools, att->image.id);
            _SG_VALIDATE(img, VALIDATE_PASSDESC_IMAGE);
            if (0 != img) {
                _SG_VALIDATE(img->slot.state == SG_RESOURCESTATE_VALID, VALIDATE_PASSDESC_IMAGE);
                _SG_VALIDATE(img->cmn.render_target, VALIDATE_PASSDESC_IMAGE_NO_RT);
                _SG_VALIDATE(att->mip_level < img->cmn.num_mipmaps, VALIDATE_PASSDESC_MIPLEVEL);
                if (img->cmn.type == SG_IMAGETYPE_CUBE) {
                    _SG_VALIDATE(att->slice < 6, VALIDATE_PASSDESC_FACE);
                } else if (img->cmn.type == SG_IMAGETYPE_ARRAY) {
                    _SG_VALIDATE(att->slice < img->cmn.num_slices, VALIDATE_PASSDESC_LAYER);
                } else if (img->cmn.type == SG_IMAGETYPE_3D) {
                    _SG_VALIDATE(att->slice < img->cmn.num_slices, VALIDATE_PASSDESC_SLICE);
                }
                if (att_index == 0) {
                    color_width = _sg_miplevel_dim(img->cmn.width, att->mip_level);
                    color_height = _sg_miplevel_dim(img->cmn.height, att->mip_level);
                    color_sample_count = img->cmn.sample_count;
                } else {
                    _SG_VALIDATE(color_width == _sg_miplevel_dim(img->cmn.width, att->mip_level), VALIDATE_PASSDESC_IMAGE_SIZES);
                    _SG_VALIDATE(color_height == _sg_miplevel_dim(img->cmn.height, att->mip_level), VALIDATE_PASSDESC_IMAGE_SIZES);
                    _SG_VALIDATE(color_sample_count == img->cmn.sample_count, VALIDATE_PASSDESC_IMAGE_SAMPLE_COUNTS);
                }
                _SG_VALIDATE(_sg_is_valid_rendertarget_color_format(img->cmn.pixel_format), VALIDATE_PASSDESC_COLOR_INV_PIXELFORMAT);

                // check resolve attachment
                const sg_pass_attachment_desc* res_att = &desc->resolve_attachments[att_index];
                if (res_att->image.id != SG_INVALID_ID) {
                    // associated color attachment must be MSAA
                    _SG_VALIDATE(img->cmn.sample_count > 1, VALIDATE_PASSDESC_RESOLVE_COLOR_IMAGE_MSAA);
                    const _sg_image_t* res_img = _sg_lookup_image(&_sg.pools, res_att->image.id);
                    _SG_VALIDATE(res_img, VALIDATE_PASSDESC_RESOLVE_IMAGE);
                    if (res_img != 0) {
                        _SG_VALIDATE(res_img->slot.state == SG_RESOURCESTATE_VALID, VALIDATE_PASSDESC_RESOLVE_IMAGE);
                        _SG_VALIDATE(res_img->cmn.render_target, VALIDATE_PASSDESC_RESOLVE_IMAGE_NO_RT);
                        _SG_VALIDATE(res_img->cmn.sample_count == 1, VALIDATE_PASSDESC_RESOLVE_SAMPLE_COUNT);
                        _SG_VALIDATE(res_att->mip_level < res_img->cmn.num_mipmaps, VALIDATE_PASSDESC_RESOLVE_MIPLEVEL);
                        if (res_img->cmn.type == SG_IMAGETYPE_CUBE) {
                            _SG_VALIDATE(res_att->slice < 6, VALIDATE_PASSDESC_RESOLVE_FACE);
                        } else if (res_img->cmn.type == SG_IMAGETYPE_ARRAY) {
                            _SG_VALIDATE(res_att->slice < res_img->cmn.num_slices, VALIDATE_PASSDESC_RESOLVE_LAYER);
                        } else if (res_img->cmn.type == SG_IMAGETYPE_3D) {
                            _SG_VALIDATE(res_att->slice < res_img->cmn.num_slices, VALIDATE_PASSDESC_RESOLVE_SLICE);
                        }
                        _SG_VALIDATE(img->cmn.pixel_format == res_img->cmn.pixel_format, VALIDATE_PASSDESC_RESOLVE_IMAGE_FORMAT);
                        _SG_VALIDATE(color_width == _sg_miplevel_dim(res_img->cmn.width, res_att->mip_level), VALIDATE_PASSDESC_RESOLVE_IMAGE_SIZES);
                        _SG_VALIDATE(color_height == _sg_miplevel_dim(res_img->cmn.height, res_att->mip_level), VALIDATE_PASSDESC_RESOLVE_IMAGE_SIZES);
                    }
                }
            }
        }
        bool has_depth_stencil_att = false;
        if (desc->depth_stencil_attachment.image.id != SG_INVALID_ID) {
            const sg_pass_attachment_desc* att = &desc->depth_stencil_attachment;
            const _sg_image_t* img = _sg_lookup_image(&_sg.pools, att->image.id);
            _SG_VALIDATE(img, VALIDATE_PASSDESC_DEPTH_IMAGE);
            has_depth_stencil_att = true;
            if (img) {
                _SG_VALIDATE(img->slot.state == SG_RESOURCESTATE_VALID, VALIDATE_PASSDESC_DEPTH_IMAGE);
                _SG_VALIDATE(att->mip_level < img->cmn.num_mipmaps, VALIDATE_PASSDESC_DEPTH_MIPLEVEL);
                if (img->cmn.type == SG_IMAGETYPE_CUBE) {
                    _SG_VALIDATE(att->slice < 6, VALIDATE_PASSDESC_DEPTH_FACE);
                } else if (img->cmn.type == SG_IMAGETYPE_ARRAY) {
                    _SG_VALIDATE(att->slice < img->cmn.num_slices, VALIDATE_PASSDESC_DEPTH_LAYER);
                } else if (img->cmn.type == SG_IMAGETYPE_3D) {
                    // NOTE: this can't actually happen because of VALIDATE_IMAGEDESC_DEPTH_3D_IMAGE
                    _SG_VALIDATE(att->slice < img->cmn.num_slices, VALIDATE_PASSDESC_DEPTH_SLICE);
                }
                _SG_VALIDATE(img->cmn.render_target, VALIDATE_PASSDESC_DEPTH_IMAGE_NO_RT);
                _SG_VALIDATE((color_width == -1) || (color_width == _sg_miplevel_dim(img->cmn.width, att->mip_level)), VALIDATE_PASSDESC_DEPTH_IMAGE_SIZES);
                _SG_VALIDATE((color_height == -1) || (color_height == _sg_miplevel_dim(img->cmn.height, att->mip_level)), VALIDATE_PASSDESC_DEPTH_IMAGE_SIZES);
                _SG_VALIDATE((color_sample_count == -1) || (color_sample_count == img->cmn.sample_count), VALIDATE_PASSDESC_DEPTH_IMAGE_SAMPLE_COUNT);
                _SG_VALIDATE(_sg_is_valid_rendertarget_depth_format(img->cmn.pixel_format), VALIDATE_PASSDESC_DEPTH_INV_PIXELFORMAT);
            }
        }
        _SG_VALIDATE(has_color_atts || has_depth_stencil_att, VALIDATE_PASSDESC_NO_ATTACHMENTS);
        return _sg_validate_end();
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
        _sg_validate_begin();
        _SG_VALIDATE(pass->slot.state == SG_RESOURCESTATE_VALID, VALIDATE_BEGINPASS_PASS);

        for (int i = 0; i < SG_MAX_COLOR_ATTACHMENTS; i++) {
            const _sg_pass_attachment_t* color_att = &pass->cmn.color_atts[i];
            const _sg_image_t* color_img = _sg_pass_color_image(pass, i);
            if (color_img) {
                _SG_VALIDATE(color_img->slot.state == SG_RESOURCESTATE_VALID, VALIDATE_BEGINPASS_COLOR_ATTACHMENT_IMAGE);
                _SG_VALIDATE(color_img->slot.id == color_att->image_id.id, VALIDATE_BEGINPASS_COLOR_ATTACHMENT_IMAGE);
            }
            const _sg_pass_attachment_t* resolve_att = &pass->cmn.resolve_atts[i];
            const _sg_image_t* resolve_img = _sg_pass_resolve_image(pass, i);
            if (resolve_img) {
                _SG_VALIDATE(resolve_img->slot.state == SG_RESOURCESTATE_VALID, VALIDATE_BEGINPASS_RESOLVE_ATTACHMENT_IMAGE);
                _SG_VALIDATE(resolve_img->slot.id == resolve_att->image_id.id, VALIDATE_BEGINPASS_RESOLVE_ATTACHMENT_IMAGE);
            }
        }
        const _sg_image_t* ds_img = _sg_pass_ds_image(pass);
        if (ds_img) {
            const _sg_pass_attachment_t* att = &pass->cmn.ds_att;
            _SG_VALIDATE(ds_img->slot.state == SG_RESOURCESTATE_VALID, VALIDATE_BEGINPASS_DEPTHSTENCIL_ATTACHMENT_IMAGE);
            _SG_VALIDATE(ds_img->slot.id == att->image_id.id, VALIDATE_BEGINPASS_DEPTHSTENCIL_ATTACHMENT_IMAGE);
        }
        return _sg_validate_end();
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
        _sg_validate_begin();
        // the pipeline object must be alive and valid
        _SG_VALIDATE(pip_id.id != SG_INVALID_ID, VALIDATE_APIP_PIPELINE_VALID_ID);
        const _sg_pipeline_t* pip = _sg_lookup_pipeline(&_sg.pools, pip_id.id);
        _SG_VALIDATE(pip != 0, VALIDATE_APIP_PIPELINE_EXISTS);
        if (!pip) {
            return _sg_validate_end();
        }
        _SG_VALIDATE(pip->slot.state == SG_RESOURCESTATE_VALID, VALIDATE_APIP_PIPELINE_VALID);
        // the pipeline's shader must be alive and valid
        SOKOL_ASSERT(pip->shader);
        _SG_VALIDATE(pip->shader->slot.id == pip->cmn.shader_id.id, VALIDATE_APIP_SHADER_EXISTS);
        _SG_VALIDATE(pip->shader->slot.state == SG_RESOURCESTATE_VALID, VALIDATE_APIP_SHADER_VALID);
        // check that pipeline attributes match current pass attributes
        const _sg_pass_t* pass = _sg_lookup_pass(&_sg.pools, _sg.cur_pass.id);
        if (pass) {
            // an offscreen pass
            _SG_VALIDATE(pip->cmn.color_count == pass->cmn.num_color_atts, VALIDATE_APIP_ATT_COUNT);
            for (int i = 0; i < pip->cmn.color_count; i++) {
                const _sg_image_t* att_img = _sg_pass_color_image(pass, i);
                _SG_VALIDATE(pip->cmn.colors[i].pixel_format == att_img->cmn.pixel_format, VALIDATE_APIP_COLOR_FORMAT);
                _SG_VALIDATE(pip->cmn.sample_count == att_img->cmn.sample_count, VALIDATE_APIP_SAMPLE_COUNT);
            }
            const _sg_image_t* att_dsimg = _sg_pass_ds_image(pass);
            if (att_dsimg) {
                _SG_VALIDATE(pip->cmn.depth.pixel_format == att_dsimg->cmn.pixel_format, VALIDATE_APIP_DEPTH_FORMAT);
            } else {
                _SG_VALIDATE(pip->cmn.depth.pixel_format == SG_PIXELFORMAT_NONE, VALIDATE_APIP_DEPTH_FORMAT);
            }
        } else {
            // default pass
            _SG_VALIDATE(pip->cmn.color_count == 1, VALIDATE_APIP_ATT_COUNT);
            _SG_VALIDATE(pip->cmn.colors[0].pixel_format == _sg.desc.context.color_format, VALIDATE_APIP_COLOR_FORMAT);
            _SG_VALIDATE(pip->cmn.depth.pixel_format == _sg.desc.context.depth_format, VALIDATE_APIP_DEPTH_FORMAT);
            _SG_VALIDATE(pip->cmn.sample_count == _sg.desc.context.sample_count, VALIDATE_APIP_SAMPLE_COUNT);
        }
        return _sg_validate_end();
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
        _sg_validate_begin();

        // a pipeline object must have been applied
        _SG_VALIDATE(_sg.cur_pipeline.id != SG_INVALID_ID, VALIDATE_ABND_PIPELINE);
        const _sg_pipeline_t* pip = _sg_lookup_pipeline(&_sg.pools, _sg.cur_pipeline.id);
        _SG_VALIDATE(pip != 0, VALIDATE_ABND_PIPELINE_EXISTS);
        if (!pip) {
            return _sg_validate_end();
        }
        _SG_VALIDATE(pip->slot.state == SG_RESOURCESTATE_VALID, VALIDATE_ABND_PIPELINE_VALID);
        SOKOL_ASSERT(pip->shader && (pip->cmn.shader_id.id == pip->shader->slot.id));

        // has expected vertex buffers, and vertex buffers still exist
        for (int i = 0; i < SG_MAX_VERTEX_BUFFERS; i++) {
            if (bindings->vertex_buffers[i].id != SG_INVALID_ID) {
                _SG_VALIDATE(pip->cmn.vertex_buffer_layout_active[i], VALIDATE_ABND_VBS);
                // buffers in vertex-buffer-slots must be of type SG_BUFFERTYPE_VERTEXBUFFER
                const _sg_buffer_t* buf = _sg_lookup_buffer(&_sg.pools, bindings->vertex_buffers[i].id);
                _SG_VALIDATE(buf != 0, VALIDATE_ABND_VB_EXISTS);
                if (buf && buf->slot.state == SG_RESOURCESTATE_VALID) {
                    _SG_VALIDATE(SG_BUFFERTYPE_VERTEXBUFFER == buf->cmn.type, VALIDATE_ABND_VB_TYPE);
                    _SG_VALIDATE(!buf->cmn.append_overflow, VALIDATE_ABND_VB_OVERFLOW);
                }
            } else {
                // vertex buffer provided in a slot which has no vertex layout in pipeline
                _SG_VALIDATE(!pip->cmn.vertex_buffer_layout_active[i], VALIDATE_ABND_VBS);
            }
        }

        // index buffer expected or not, and index buffer still exists
        if (pip->cmn.index_type == SG_INDEXTYPE_NONE) {
            // pipeline defines non-indexed rendering, but index buffer provided
            _SG_VALIDATE(bindings->index_buffer.id == SG_INVALID_ID, VALIDATE_ABND_IB);
        } else {
            // pipeline defines indexed rendering, but no index buffer provided
            _SG_VALIDATE(bindings->index_buffer.id != SG_INVALID_ID, VALIDATE_ABND_NO_IB);
        }
        if (bindings->index_buffer.id != SG_INVALID_ID) {
            // buffer in index-buffer-slot must be of type SG_BUFFERTYPE_INDEXBUFFER
            const _sg_buffer_t* buf = _sg_lookup_buffer(&_sg.pools, bindings->index_buffer.id);
            _SG_VALIDATE(buf != 0, VALIDATE_ABND_IB_EXISTS);
            if (buf && buf->slot.state == SG_RESOURCESTATE_VALID) {
                _SG_VALIDATE(SG_BUFFERTYPE_INDEXBUFFER == buf->cmn.type, VALIDATE_ABND_IB_TYPE);
                _SG_VALIDATE(!buf->cmn.append_overflow, VALIDATE_ABND_IB_OVERFLOW);
            }
        }

        // has expected vertex shader images
        for (int i = 0; i < SG_MAX_SHADERSTAGE_IMAGES; i++) {
            const _sg_shader_stage_t* stage = &pip->shader->cmn.stage[SG_SHADERSTAGE_VS];
            if (stage->images[i].image_type != _SG_IMAGETYPE_DEFAULT) {
                _SG_VALIDATE(bindings->vs.images[i].id != SG_INVALID_ID, VALIDATE_ABND_VS_EXPECTED_IMAGE_BINDING);
                if (bindings->vs.images[i].id != SG_INVALID_ID) {
                    const _sg_image_t* img = _sg_lookup_image(&_sg.pools, bindings->vs.images[i].id);
                    _SG_VALIDATE(img != 0, VALIDATE_ABND_VS_IMG_EXISTS);
                    if (img && img->slot.state == SG_RESOURCESTATE_VALID) {
                        _SG_VALIDATE(img->cmn.type == stage->images[i].image_type, VALIDATE_ABND_VS_IMAGE_TYPE_MISMATCH);
                        _SG_VALIDATE(img->cmn.sample_count == 1, VALIDATE_ABND_VS_IMAGE_MSAA);
                        const sg_pixelformat_info* info = &_sg.formats[img->cmn.pixel_format];
                        switch (stage->images[i].sample_type) {
                            case SG_IMAGESAMPLETYPE_FLOAT:
                                _SG_VALIDATE(info->filter, VALIDATE_ABND_VS_EXPECTED_FILTERABLE_IMAGE);
                                break;
                            case SG_IMAGESAMPLETYPE_DEPTH:
                                _SG_VALIDATE(info->depth, VALIDATE_ABND_VS_EXPECTED_DEPTH_IMAGE);
                                break;
                            default:
                                break;
                        }
                    }
                }
            } else {
                _SG_VALIDATE(bindings->vs.images[i].id == SG_INVALID_ID, VALIDATE_ABND_VS_UNEXPECTED_IMAGE_BINDING);
            }
        }

        // has expected vertex shader image samplers
        for (int i = 0; i < SG_MAX_SHADERSTAGE_SAMPLERS; i++) {
            const _sg_shader_stage_t* stage = &pip->shader->cmn.stage[SG_SHADERSTAGE_VS];
            if (stage->samplers[i].sampler_type != _SG_SAMPLERTYPE_DEFAULT) {
                _SG_VALIDATE(bindings->vs.samplers[i].id != SG_INVALID_ID, VALIDATE_ABND_VS_EXPECTED_SAMPLER_BINDING);
                if (bindings->vs.samplers[i].id != SG_INVALID_ID) {
                    const _sg_sampler_t* smp = _sg_lookup_sampler(&_sg.pools, bindings->vs.samplers[i].id);
                    _SG_VALIDATE(smp != 0, VALIDATE_ABND_VS_SMP_EXISTS);
                    if (smp) {
                        if (stage->samplers[i].sampler_type == SG_SAMPLERTYPE_COMPARISON) {
                            _SG_VALIDATE(smp->cmn.compare != SG_COMPAREFUNC_NEVER, VALIDATE_ABND_VS_UNEXPECTED_SAMPLER_COMPARE_NEVER);
                        } else {
                            _SG_VALIDATE(smp->cmn.compare == SG_COMPAREFUNC_NEVER, VALIDATE_ABND_VS_EXPECTED_SAMPLER_COMPARE_NEVER);
                        }
                        if (stage->samplers[i].sampler_type == SG_SAMPLERTYPE_NONFILTERING) {
                            const bool nonfiltering = (smp->cmn.min_filter != SG_FILTER_LINEAR)
                                                   && (smp->cmn.mag_filter != SG_FILTER_LINEAR)
                                                   && (smp->cmn.mipmap_filter != SG_FILTER_LINEAR);
                            _SG_VALIDATE(nonfiltering, VALIDATE_ABND_VS_EXPECTED_NONFILTERING_SAMPLER);
                        }
                    }
                }
            } else {
                _SG_VALIDATE(bindings->vs.samplers[i].id == SG_INVALID_ID, VALIDATE_ABND_VS_UNEXPECTED_SAMPLER_BINDING);
            }
        }

        // has expected fragment shader images
        for (int i = 0; i < SG_MAX_SHADERSTAGE_IMAGES; i++) {
            const _sg_shader_stage_t* stage = &pip->shader->cmn.stage[SG_SHADERSTAGE_FS];
            if (stage->images[i].image_type != _SG_IMAGETYPE_DEFAULT) {
                _SG_VALIDATE(bindings->fs.images[i].id != SG_INVALID_ID, VALIDATE_ABND_FS_EXPECTED_IMAGE_BINDING);
                if (bindings->fs.images[i].id != SG_INVALID_ID) {
                    const _sg_image_t* img = _sg_lookup_image(&_sg.pools, bindings->fs.images[i].id);
                    _SG_VALIDATE(img != 0, VALIDATE_ABND_FS_IMG_EXISTS);
                    if (img && img->slot.state == SG_RESOURCESTATE_VALID) {
                        _SG_VALIDATE(img->cmn.type == stage->images[i].image_type, VALIDATE_ABND_FS_IMAGE_TYPE_MISMATCH);
                        _SG_VALIDATE(img->cmn.sample_count == 1, VALIDATE_ABND_FS_IMAGE_MSAA);
                        const sg_pixelformat_info* info = &_sg.formats[img->cmn.pixel_format];
                        switch (stage->images[i].sample_type) {
                            case SG_IMAGESAMPLETYPE_FLOAT:
                                _SG_VALIDATE(info->filter, VALIDATE_ABND_FS_EXPECTED_FILTERABLE_IMAGE);
                                break;
                            case SG_IMAGESAMPLETYPE_DEPTH:
                                _SG_VALIDATE(info->depth, VALIDATE_ABND_FS_EXPECTED_DEPTH_IMAGE);
                                break;
                            default:
                                break;
                        }
                    }
                }
            } else {
                _SG_VALIDATE(bindings->fs.images[i].id == SG_INVALID_ID, VALIDATE_ABND_FS_UNEXPECTED_IMAGE_BINDING);
            }
        }

        // has expected fragment shader samplers
        for (int i = 0; i < SG_MAX_SHADERSTAGE_SAMPLERS; i++) {
            const _sg_shader_stage_t* stage = &pip->shader->cmn.stage[SG_SHADERSTAGE_FS];
            if (stage->samplers[i].sampler_type != _SG_SAMPLERTYPE_DEFAULT) {
                _SG_VALIDATE(bindings->fs.samplers[i].id != SG_INVALID_ID, VALIDATE_ABND_FS_EXPECTED_SAMPLER_BINDING);
                if (bindings->fs.samplers[i].id != SG_INVALID_ID) {
                    const _sg_sampler_t* smp = _sg_lookup_sampler(&_sg.pools, bindings->fs.samplers[i].id);
                    _SG_VALIDATE(smp != 0, VALIDATE_ABND_FS_SMP_EXISTS);
                    if (smp) {
                        if (stage->samplers[i].sampler_type == SG_SAMPLERTYPE_COMPARISON) {
                            _SG_VALIDATE(smp->cmn.compare != SG_COMPAREFUNC_NEVER, VALIDATE_ABND_FS_UNEXPECTED_SAMPLER_COMPARE_NEVER);
                        } else {
                            _SG_VALIDATE(smp->cmn.compare == SG_COMPAREFUNC_NEVER, VALIDATE_ABND_FS_EXPECTED_SAMPLER_COMPARE_NEVER);
                        }
                        if (stage->samplers[i].sampler_type == SG_SAMPLERTYPE_NONFILTERING) {
                            const bool nonfiltering = (smp->cmn.min_filter != SG_FILTER_LINEAR)
                                                   && (smp->cmn.mag_filter != SG_FILTER_LINEAR)
                                                   && (smp->cmn.mipmap_filter != SG_FILTER_LINEAR);
                            _SG_VALIDATE(nonfiltering, VALIDATE_ABND_FS_EXPECTED_NONFILTERING_SAMPLER);
                        }
                    }
                }
            } else {
                _SG_VALIDATE(bindings->fs.samplers[i].id == SG_INVALID_ID, VALIDATE_ABND_FS_UNEXPECTED_SAMPLER_BINDING);
            }
        }
        return _sg_validate_end();
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
        _sg_validate_begin();
        _SG_VALIDATE(_sg.cur_pipeline.id != SG_INVALID_ID, VALIDATE_AUB_NO_PIPELINE);
        const _sg_pipeline_t* pip = _sg_lookup_pipeline(&_sg.pools, _sg.cur_pipeline.id);
        SOKOL_ASSERT(pip && (pip->slot.id == _sg.cur_pipeline.id));
        SOKOL_ASSERT(pip->shader && (pip->shader->slot.id == pip->cmn.shader_id.id));

        // check that there is a uniform block at 'stage' and 'ub_index'
        const _sg_shader_stage_t* stage = &pip->shader->cmn.stage[stage_index];
        _SG_VALIDATE(ub_index < stage->num_uniform_blocks, VALIDATE_AUB_NO_UB_AT_SLOT);

        // check that the provided data size matches the uniform block size
        _SG_VALIDATE(data->size == stage->uniform_blocks[ub_index].size, VALIDATE_AUB_SIZE);

        return _sg_validate_end();
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
        _sg_validate_begin();
        _SG_VALIDATE(buf->cmn.usage != SG_USAGE_IMMUTABLE, VALIDATE_UPDATEBUF_USAGE);
        _SG_VALIDATE(buf->cmn.size >= (int)data->size, VALIDATE_UPDATEBUF_SIZE);
        _SG_VALIDATE(buf->cmn.update_frame_index != _sg.frame_index, VALIDATE_UPDATEBUF_ONCE);
        _SG_VALIDATE(buf->cmn.append_frame_index != _sg.frame_index, VALIDATE_UPDATEBUF_APPEND);
        return _sg_validate_end();
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
        _sg_validate_begin();
        _SG_VALIDATE(buf->cmn.usage != SG_USAGE_IMMUTABLE, VALIDATE_APPENDBUF_USAGE);
        _SG_VALIDATE(buf->cmn.size >= (buf->cmn.append_pos + (int)data->size), VALIDATE_APPENDBUF_SIZE);
        _SG_VALIDATE(buf->cmn.update_frame_index != _sg.frame_index, VALIDATE_APPENDBUF_UPDATE);
        return _sg_validate_end();
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
        _sg_validate_begin();
        _SG_VALIDATE(img->cmn.usage != SG_USAGE_IMMUTABLE, VALIDATE_UPDIMG_USAGE);
        _SG_VALIDATE(img->cmn.upd_frame_index != _sg.frame_index, VALIDATE_UPDIMG_ONCE);
        _sg_validate_image_data(data,
            img->cmn.pixel_format,
            img->cmn.width,
            img->cmn.height,
            (img->cmn.type == SG_IMAGETYPE_CUBE) ? 6 : 1,
            img->cmn.num_mipmaps,
            img->cmn.num_slices);
        return _sg_validate_end();
    #endif
}

// ██████  ███████ ███████  ██████  ██    ██ ██████   ██████ ███████ ███████
// ██   ██ ██      ██      ██    ██ ██    ██ ██   ██ ██      ██      ██
// ██████  █████   ███████ ██    ██ ██    ██ ██████  ██      █████   ███████
// ██   ██ ██           ██ ██    ██ ██    ██ ██   ██ ██      ██           ██
// ██   ██ ███████ ███████  ██████   ██████  ██   ██  ██████ ███████ ███████
//
// >>resources
_SOKOL_PRIVATE sg_buffer_desc _sg_buffer_desc_defaults(const sg_buffer_desc* desc) {
    sg_buffer_desc def = *desc;
    def.type = _sg_def(def.type, SG_BUFFERTYPE_VERTEXBUFFER);
    def.usage = _sg_def(def.usage, SG_USAGE_IMMUTABLE);
    if (def.size == 0) {
        def.size = def.data.size;
    } else if (def.data.size == 0) {
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
    } else {
        def.pixel_format = _sg_def(def.pixel_format, SG_PIXELFORMAT_RGBA8);
        def.sample_count = _sg_def(def.sample_count, 1);
    }
    return def;
}

_SOKOL_PRIVATE sg_sampler_desc _sg_sampler_desc_defaults(const sg_sampler_desc* desc) {
    sg_sampler_desc def = *desc;
    def.min_filter = _sg_def(def.min_filter, SG_FILTER_NEAREST);
    def.mag_filter = _sg_def(def.mag_filter, SG_FILTER_NEAREST);
    def.mipmap_filter = _sg_def(def.mipmap_filter, SG_FILTER_NONE);
    def.wrap_u = _sg_def(def.wrap_u, SG_WRAP_REPEAT);
    def.wrap_v = _sg_def(def.wrap_v, SG_WRAP_REPEAT);
    def.wrap_w = _sg_def(def.wrap_w, SG_WRAP_REPEAT);
    def.max_lod = _sg_def_flt(def.max_lod, FLT_MAX);
    def.border_color = _sg_def(def.border_color, SG_BORDERCOLOR_OPAQUE_BLACK);
    def.compare = _sg_def(def.compare, SG_COMPAREFUNC_NEVER);
    def.max_anisotropy = _sg_def(def.max_anisotropy, 1);
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
            if (!img_desc->used) {
                break;
            }
            img_desc->image_type = _sg_def(img_desc->image_type, SG_IMAGETYPE_2D);
            img_desc->sample_type = _sg_def(img_desc->sample_type, SG_IMAGESAMPLETYPE_FLOAT);
        }
        for (int smp_index = 0; smp_index < SG_MAX_SHADERSTAGE_SAMPLERS; smp_index++) {
            sg_shader_sampler_desc* smp_desc = &stage_desc->samplers[smp_index];
            if (!smp_desc->used) {
                break;
            }
            smp_desc->sampler_type = _sg_def(smp_desc->sampler_type, SG_SAMPLERTYPE_FILTERING);
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
    if (def.colors[0].pixel_format == SG_PIXELFORMAT_NONE) {
        // special case depth-only rendering, enforce a color count of 0
        def.color_count = 0;
    } else {
        def.color_count = _sg_def(def.color_count, 1);
    }
    if (def.color_count > SG_MAX_COLOR_ATTACHMENTS) {
        def.color_count = SG_MAX_COLOR_ATTACHMENTS;
    }
    for (int i = 0; i < def.color_count; i++) {
        sg_color_target_state* cs = &def.colors[i];
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
        sg_vertex_attr_state* a_state = &def.layout.attrs[attr_index];
        if (a_state->format == SG_VERTEXFORMAT_INVALID) {
            break;
        }
        SOKOL_ASSERT(a_state->buffer_index < SG_MAX_VERTEX_BUFFERS);
        sg_vertex_buffer_layout_state* l_state = &def.layout.buffers[a_state->buffer_index];
        l_state->step_func = _sg_def(l_state->step_func, SG_VERTEXSTEP_PER_VERTEX);
        l_state->step_rate = _sg_def(l_state->step_rate, 1);
    }

    // resolve vertex layout strides and offsets
    int auto_offset[SG_MAX_VERTEX_BUFFERS];
    _sg_clear(auto_offset, sizeof(auto_offset));
    bool use_auto_offset = true;
    for (int attr_index = 0; attr_index < SG_MAX_VERTEX_ATTRIBUTES; attr_index++) {
        // to use computed offsets, *all* attr offsets must be 0
        if (def.layout.attrs[attr_index].offset != 0) {
            use_auto_offset = false;
        }
    }
    for (int attr_index = 0; attr_index < SG_MAX_VERTEX_ATTRIBUTES; attr_index++) {
        sg_vertex_attr_state* a_state = &def.layout.attrs[attr_index];
        if (a_state->format == SG_VERTEXFORMAT_INVALID) {
            break;
        }
        SOKOL_ASSERT(a_state->buffer_index < SG_MAX_VERTEX_BUFFERS);
        if (use_auto_offset) {
            a_state->offset = auto_offset[a_state->buffer_index];
        }
        auto_offset[a_state->buffer_index] += _sg_vertexformat_bytesize(a_state->format);
    }
    // compute vertex strides if needed
    for (int buf_index = 0; buf_index < SG_MAX_VERTEX_BUFFERS; buf_index++) {
        sg_vertex_buffer_layout_state* l_state = &def.layout.buffers[buf_index];
        if (l_state->stride == 0) {
            l_state->stride = auto_offset[buf_index];
        }
    }

    return def;
}

_SOKOL_PRIVATE sg_pass_desc _sg_pass_desc_defaults(const sg_pass_desc* desc) {
    // FIXME: no values to replace in sg_pass_desc?
    sg_pass_desc def = *desc;
    return def;
}

_SOKOL_PRIVATE sg_buffer _sg_alloc_buffer(void) {
    sg_buffer res;
    int slot_index = _sg_pool_alloc_index(&_sg.pools.buffer_pool);
    if (_SG_INVALID_SLOT_INDEX != slot_index) {
        res.id = _sg_slot_alloc(&_sg.pools.buffer_pool, &_sg.pools.buffers[slot_index].slot, slot_index);
    } else {
        res.id = SG_INVALID_ID;
        _SG_ERROR(BUFFER_POOL_EXHAUSTED);
    }
    return res;
}

_SOKOL_PRIVATE sg_image _sg_alloc_image(void) {
    sg_image res;
    int slot_index = _sg_pool_alloc_index(&_sg.pools.image_pool);
    if (_SG_INVALID_SLOT_INDEX != slot_index) {
        res.id = _sg_slot_alloc(&_sg.pools.image_pool, &_sg.pools.images[slot_index].slot, slot_index);
    } else {
        res.id = SG_INVALID_ID;
        _SG_ERROR(IMAGE_POOL_EXHAUSTED);
    }
    return res;
}

_SOKOL_PRIVATE sg_sampler _sg_alloc_sampler(void) {
    sg_sampler res;
    int slot_index = _sg_pool_alloc_index(&_sg.pools.sampler_pool);
    if (_SG_INVALID_SLOT_INDEX != slot_index) {
        res.id = _sg_slot_alloc(&_sg.pools.sampler_pool, &_sg.pools.samplers[slot_index].slot, slot_index);
    } else {
        res.id = SG_INVALID_ID;
        _SG_ERROR(SAMPLER_POOL_EXHAUSTED);
    }
    return res;
}

_SOKOL_PRIVATE sg_shader _sg_alloc_shader(void) {
    sg_shader res;
    int slot_index = _sg_pool_alloc_index(&_sg.pools.shader_pool);
    if (_SG_INVALID_SLOT_INDEX != slot_index) {
        res.id = _sg_slot_alloc(&_sg.pools.shader_pool, &_sg.pools.shaders[slot_index].slot, slot_index);
    } else {
        res.id = SG_INVALID_ID;
        _SG_ERROR(SHADER_POOL_EXHAUSTED);
    }
    return res;
}

_SOKOL_PRIVATE sg_pipeline _sg_alloc_pipeline(void) {
    sg_pipeline res;
    int slot_index = _sg_pool_alloc_index(&_sg.pools.pipeline_pool);
    if (_SG_INVALID_SLOT_INDEX != slot_index) {
        res.id =_sg_slot_alloc(&_sg.pools.pipeline_pool, &_sg.pools.pipelines[slot_index].slot, slot_index);
    } else {
        res.id = SG_INVALID_ID;
        _SG_ERROR(PIPELINE_POOL_EXHAUSTED);
    }
    return res;
}

_SOKOL_PRIVATE sg_pass _sg_alloc_pass(void) {
    sg_pass res;
    int slot_index = _sg_pool_alloc_index(&_sg.pools.pass_pool);
    if (_SG_INVALID_SLOT_INDEX != slot_index) {
        res.id = _sg_slot_alloc(&_sg.pools.pass_pool, &_sg.pools.passes[slot_index].slot, slot_index);
    } else {
        res.id = SG_INVALID_ID;
        _SG_ERROR(PASS_POOL_EXHAUSTED);
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

_SOKOL_PRIVATE void _sg_dealloc_sampler(_sg_sampler_t* smp) {
    SOKOL_ASSERT(smp && (smp->slot.state == SG_RESOURCESTATE_ALLOC) && (smp->slot.id != SG_INVALID_ID));
    _sg_pool_free_index(&_sg.pools.sampler_pool, _sg_slot_index(smp->slot.id));
    _sg_reset_slot(&smp->slot);
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
        _sg_buffer_common_init(&buf->cmn, desc);
        buf->slot.state = _sg_create_buffer(buf, desc);
    } else {
        buf->slot.state = SG_RESOURCESTATE_FAILED;
    }
    SOKOL_ASSERT((buf->slot.state == SG_RESOURCESTATE_VALID)||(buf->slot.state == SG_RESOURCESTATE_FAILED));
}

_SOKOL_PRIVATE void _sg_init_image(_sg_image_t* img, const sg_image_desc* desc) {
    SOKOL_ASSERT(img && (img->slot.state == SG_RESOURCESTATE_ALLOC));
    SOKOL_ASSERT(desc);
    img->slot.ctx_id = _sg.active_context.id;
    if (_sg_validate_image_desc(desc)) {
        _sg_image_common_init(&img->cmn, desc);
        img->slot.state = _sg_create_image(img, desc);
    } else {
        img->slot.state = SG_RESOURCESTATE_FAILED;
    }
    SOKOL_ASSERT((img->slot.state == SG_RESOURCESTATE_VALID)||(img->slot.state == SG_RESOURCESTATE_FAILED));
}

_SOKOL_PRIVATE void _sg_init_sampler(_sg_sampler_t* smp, const sg_sampler_desc* desc) {
    SOKOL_ASSERT(smp && (smp->slot.state == SG_RESOURCESTATE_ALLOC));
    SOKOL_ASSERT(desc);
    smp->slot.ctx_id = _sg.active_context.id;
    if (_sg_validate_sampler_desc(desc)) {
        _sg_sampler_common_init(&smp->cmn, desc);
        smp->slot.state = _sg_create_sampler(smp, desc);
    } else {
        smp->slot.state = SG_RESOURCESTATE_FAILED;
    }
    SOKOL_ASSERT((smp->slot.state == SG_RESOURCESTATE_VALID)||(smp->slot.state == SG_RESOURCESTATE_FAILED));
}

_SOKOL_PRIVATE void _sg_init_shader(_sg_shader_t* shd, const sg_shader_desc* desc) {
    SOKOL_ASSERT(shd && (shd->slot.state == SG_RESOURCESTATE_ALLOC));
    SOKOL_ASSERT(desc);
    shd->slot.ctx_id = _sg.active_context.id;
    if (_sg_validate_shader_desc(desc)) {
        _sg_shader_common_init(&shd->cmn, desc);
        shd->slot.state = _sg_create_shader(shd, desc);
    } else {
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
            _sg_pipeline_common_init(&pip->cmn, desc);
            pip->slot.state = _sg_create_pipeline(pip, shd, desc);
        } else {
            pip->slot.state = SG_RESOURCESTATE_FAILED;
        }
    } else {
        pip->slot.state = SG_RESOURCESTATE_FAILED;
    }
    SOKOL_ASSERT((pip->slot.state == SG_RESOURCESTATE_VALID)||(pip->slot.state == SG_RESOURCESTATE_FAILED));
}

_SOKOL_PRIVATE void _sg_init_pass(_sg_pass_t* pass, const sg_pass_desc* desc) {
    SOKOL_ASSERT(pass && pass->slot.state == SG_RESOURCESTATE_ALLOC);
    SOKOL_ASSERT(desc);
    pass->slot.ctx_id = _sg.active_context.id;
    if (_sg_validate_pass_desc(desc)) {
        // lookup pass attachment image pointers
        _sg_image_t* color_images[SG_MAX_COLOR_ATTACHMENTS] = { 0 };
        _sg_image_t* resolve_images[SG_MAX_COLOR_ATTACHMENTS] = { 0 };
        _sg_image_t* ds_image = 0;
        // NOTE: validation already checked that all surfaces are same width/height
        int width = 0;
        int height = 0;
        for (int i = 0; i < SG_MAX_COLOR_ATTACHMENTS; i++) {
            if (desc->color_attachments[i].image.id) {
                color_images[i] = _sg_lookup_image(&_sg.pools, desc->color_attachments[i].image.id);
                if (!(color_images[i] && color_images[i]->slot.state == SG_RESOURCESTATE_VALID)) {
                    pass->slot.state = SG_RESOURCESTATE_FAILED;
                    return;
                }
                const int mip_level = desc->color_attachments[i].mip_level;
                width = _sg_miplevel_dim(color_images[i]->cmn.width, mip_level);
                height = _sg_miplevel_dim(color_images[i]->cmn.height, mip_level);
            }
            if (desc->resolve_attachments[i].image.id) {
                resolve_images[i] = _sg_lookup_image(&_sg.pools, desc->resolve_attachments[i].image.id);
                if (!(resolve_images[i] && resolve_images[i]->slot.state == SG_RESOURCESTATE_VALID)) {
                    pass->slot.state = SG_RESOURCESTATE_FAILED;
                    return;
                }
            }
        }
        if (desc->depth_stencil_attachment.image.id) {
            ds_image = _sg_lookup_image(&_sg.pools, desc->depth_stencil_attachment.image.id);
            if (!(ds_image && ds_image->slot.state == SG_RESOURCESTATE_VALID)) {
                pass->slot.state = SG_RESOURCESTATE_FAILED;
                return;
            }
            const int mip_level = desc->depth_stencil_attachment.mip_level;
            width = _sg_miplevel_dim(ds_image->cmn.width, mip_level);
            height = _sg_miplevel_dim(ds_image->cmn.height, mip_level);
        }
        _sg_pass_common_init(&pass->cmn, desc, width, height);
        pass->slot.state = _sg_create_pass(pass, color_images, resolve_images, ds_image, desc);
    } else {
        pass->slot.state = SG_RESOURCESTATE_FAILED;
    }
    SOKOL_ASSERT((pass->slot.state == SG_RESOURCESTATE_VALID)||(pass->slot.state == SG_RESOURCESTATE_FAILED));
}

_SOKOL_PRIVATE void _sg_uninit_buffer(_sg_buffer_t* buf) {
    SOKOL_ASSERT(buf && ((buf->slot.state == SG_RESOURCESTATE_VALID) || (buf->slot.state == SG_RESOURCESTATE_FAILED)));
    if (buf->slot.ctx_id == _sg.active_context.id) {
        _sg_discard_buffer(buf);
        _sg_reset_buffer_to_alloc_state(buf);
    } else {
        _SG_WARN(UNINIT_BUFFER_ACTIVE_CONTEXT_MISMATCH);
    }
}

_SOKOL_PRIVATE void _sg_uninit_image(_sg_image_t* img) {
    SOKOL_ASSERT(img && ((img->slot.state == SG_RESOURCESTATE_VALID) || (img->slot.state == SG_RESOURCESTATE_FAILED)));
    if (img->slot.ctx_id == _sg.active_context.id) {
        _sg_discard_image(img);
        _sg_reset_image_to_alloc_state(img);
    } else {
        _SG_WARN(UNINIT_IMAGE_ACTIVE_CONTEXT_MISMATCH);
    }
}

_SOKOL_PRIVATE void _sg_uninit_sampler(_sg_sampler_t* smp) {
    SOKOL_ASSERT(smp && ((smp->slot.state == SG_RESOURCESTATE_VALID) || (smp->slot.state == SG_RESOURCESTATE_FAILED)));
    if (smp->slot.ctx_id == _sg.active_context.id) {
        _sg_discard_sampler(smp);
        _sg_reset_sampler_to_alloc_state(smp);
    } else {
        _SG_WARN(UNINIT_SAMPLER_ACTIVE_CONTEXT_MISMATCH);
    }
}

_SOKOL_PRIVATE void _sg_uninit_shader(_sg_shader_t* shd) {
    SOKOL_ASSERT(shd && ((shd->slot.state == SG_RESOURCESTATE_VALID) || (shd->slot.state == SG_RESOURCESTATE_FAILED)));
    if (shd->slot.ctx_id == _sg.active_context.id) {
        _sg_discard_shader(shd);
        _sg_reset_shader_to_alloc_state(shd);
    } else {
        _SG_WARN(UNINIT_SHADER_ACTIVE_CONTEXT_MISMATCH);
    }
}

_SOKOL_PRIVATE void _sg_uninit_pipeline(_sg_pipeline_t* pip) {
    SOKOL_ASSERT(pip && ((pip->slot.state == SG_RESOURCESTATE_VALID) || (pip->slot.state == SG_RESOURCESTATE_FAILED)));
    if (pip->slot.ctx_id == _sg.active_context.id) {
        _sg_discard_pipeline(pip);
        _sg_reset_pipeline_to_alloc_state(pip);
    } else {
        _SG_WARN(UNINIT_PIPELINE_ACTIVE_CONTEXT_MISMATCH);
    }
}

_SOKOL_PRIVATE void _sg_uninit_pass(_sg_pass_t* pass) {
    SOKOL_ASSERT(pass && ((pass->slot.state == SG_RESOURCESTATE_VALID) || (pass->slot.state == SG_RESOURCESTATE_FAILED)));
    if (pass->slot.ctx_id == _sg.active_context.id) {
        _sg_discard_pass(pass);
        _sg_reset_pass_to_alloc_state(pass);
    } else {
        _SG_WARN(UNINIT_PASS_ACTIVE_CONTEXT_MISMATCH);
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
            _SG_ERROR(IDENTICAL_COMMIT_LISTENER);
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
        _SG_ERROR(COMMIT_LISTENER_ARRAY_FULL);
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
    res.sampler_pool_size = _sg_def(res.sampler_pool_size, _SG_DEFAULT_SAMPLER_POOL_SIZE);
    res.shader_pool_size = _sg_def(res.shader_pool_size, _SG_DEFAULT_SHADER_POOL_SIZE);
    res.pipeline_pool_size = _sg_def(res.pipeline_pool_size, _SG_DEFAULT_PIPELINE_POOL_SIZE);
    res.pass_pool_size = _sg_def(res.pass_pool_size, _SG_DEFAULT_PASS_POOL_SIZE);
    res.context_pool_size = _sg_def(res.context_pool_size, _SG_DEFAULT_CONTEXT_POOL_SIZE);
    res.uniform_buffer_size = _sg_def(res.uniform_buffer_size, _SG_DEFAULT_UB_SIZE);
    res.max_commit_listeners = _sg_def(res.max_commit_listeners, _SG_DEFAULT_MAX_COMMIT_LISTENERS);
    res.wgpu_bindgroups_cache_size = _sg_def(res.wgpu_bindgroups_cache_size, _SG_DEFAULT_WGPU_BINDGROUP_CACHE_SIZE);
    return res;
}

// ██████  ██    ██ ██████  ██      ██  ██████
// ██   ██ ██    ██ ██   ██ ██      ██ ██
// ██████  ██    ██ ██████  ██      ██ ██
// ██      ██    ██ ██   ██ ██      ██ ██
// ██       ██████  ██████  ███████ ██  ██████
//
// >>public
SOKOL_API_IMPL void sg_setup(const sg_desc* desc) {
    SOKOL_ASSERT(desc);
    SOKOL_ASSERT((desc->_start_canary == 0) && (desc->_end_canary == 0));
    SOKOL_ASSERT((desc->allocator.alloc_fn && desc->allocator.free_fn) || (!desc->allocator.alloc_fn && !desc->allocator.free_fn));
    _SG_CLEAR_ARC_STRUCT(_sg_state_t, _sg);
    _sg.desc = _sg_desc_defaults(desc);
    _sg_setup_pools(&_sg.pools, &_sg.desc);
    _sg_setup_commit_listeners(&_sg.desc);
    _sg.frame_index = 1;
    _sg.stats_enabled = true;
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

SOKOL_API_IMPL sg_frame_stats sg_query_frame_stats(void) {
    SOKOL_ASSERT(_sg.valid);
    return _sg.prev_stats;
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
    } else {
        // pool is exhausted
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
    // NOTE: ctx can be 0 here if the context is no longer valid
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
        _SG_WARN(TRACE_HOOKS_NOT_ENABLED);
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

SOKOL_API_IMPL sg_sampler sg_alloc_sampler(void) {
    SOKOL_ASSERT(_sg.valid);
    sg_sampler res = _sg_alloc_sampler();
    _SG_TRACE_ARGS(alloc_sampler, res);
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
        } else {
            _SG_ERROR(DEALLOC_BUFFER_INVALID_STATE);
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
        } else {
            _SG_ERROR(DEALLOC_IMAGE_INVALID_STATE);
        }
    }
    _SG_TRACE_ARGS(dealloc_image, img_id);
}

SOKOL_API_IMPL void sg_dealloc_sampler(sg_sampler smp_id) {
    SOKOL_ASSERT(_sg.valid);
    _sg_sampler_t* smp = _sg_lookup_sampler(&_sg.pools, smp_id.id);
    if (smp) {
        if (smp->slot.state == SG_RESOURCESTATE_ALLOC) {
            _sg_dealloc_sampler(smp);
        } else {
            _SG_ERROR(DEALLOC_SAMPLER_INVALID_STATE);
        }
    }
    _SG_TRACE_ARGS(dealloc_sampler, smp_id);
}

SOKOL_API_IMPL void sg_dealloc_shader(sg_shader shd_id) {
    SOKOL_ASSERT(_sg.valid);
    _sg_shader_t* shd = _sg_lookup_shader(&_sg.pools, shd_id.id);
    if (shd) {
        if (shd->slot.state == SG_RESOURCESTATE_ALLOC) {
            _sg_dealloc_shader(shd);
        } else {
            _SG_ERROR(DEALLOC_SHADER_INVALID_STATE);
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
        } else {
            _SG_ERROR(DEALLOC_PIPELINE_INVALID_STATE);
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
        } else {
            _SG_ERROR(DEALLOC_PASS_INVALID_STATE);
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
        } else {
            _SG_ERROR(INIT_BUFFER_INVALID_STATE);
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
        } else {
            _SG_ERROR(INIT_IMAGE_INVALID_STATE);
        }
    }
    _SG_TRACE_ARGS(init_image, img_id, &desc_def);
}

SOKOL_API_IMPL void sg_init_sampler(sg_sampler smp_id, const sg_sampler_desc* desc) {
    SOKOL_ASSERT(_sg.valid);
    sg_sampler_desc desc_def = _sg_sampler_desc_defaults(desc);
    _sg_sampler_t* smp = _sg_lookup_sampler(&_sg.pools, smp_id.id);
    if (smp) {
        if (smp->slot.state == SG_RESOURCESTATE_ALLOC) {
            _sg_init_sampler(smp, &desc_def);
            SOKOL_ASSERT((smp->slot.state == SG_RESOURCESTATE_VALID) || (smp->slot.state == SG_RESOURCESTATE_FAILED));
        } else {
            _SG_ERROR(INIT_SAMPLER_INVALID_STATE);
        }
    }
    _SG_TRACE_ARGS(init_sampler, smp_id, &desc_def);
}

SOKOL_API_IMPL void sg_init_shader(sg_shader shd_id, const sg_shader_desc* desc) {
    SOKOL_ASSERT(_sg.valid);
    sg_shader_desc desc_def = _sg_shader_desc_defaults(desc);
    _sg_shader_t* shd = _sg_lookup_shader(&_sg.pools, shd_id.id);
    if (shd) {
        if (shd->slot.state == SG_RESOURCESTATE_ALLOC) {
            _sg_init_shader(shd, &desc_def);
            SOKOL_ASSERT((shd->slot.state == SG_RESOURCESTATE_VALID) || (shd->slot.state == SG_RESOURCESTATE_FAILED));
        } else {
            _SG_ERROR(INIT_SHADER_INVALID_STATE);
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
        } else {
            _SG_ERROR(INIT_PIPELINE_INVALID_STATE);
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
        } else {
            _SG_ERROR(INIT_PASS_INVALID_STATE);
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
        } else {
            _SG_ERROR(UNINIT_BUFFER_INVALID_STATE);
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
        } else {
            _SG_ERROR(UNINIT_IMAGE_INVALID_STATE);
        }
    }
    _SG_TRACE_ARGS(uninit_image, img_id);
}

SOKOL_API_IMPL void sg_uninit_sampler(sg_sampler smp_id) {
    SOKOL_ASSERT(_sg.valid);
    _sg_sampler_t* smp = _sg_lookup_sampler(&_sg.pools, smp_id.id);
    if (smp) {
        if ((smp->slot.state == SG_RESOURCESTATE_VALID) || (smp->slot.state == SG_RESOURCESTATE_FAILED)) {
            _sg_uninit_sampler(smp);
            SOKOL_ASSERT(smp->slot.state == SG_RESOURCESTATE_ALLOC);
        } else {
            _SG_ERROR(UNINIT_SAMPLER_INVALID_STATE);
        }
    }
    _SG_TRACE_ARGS(uninit_sampler, smp_id);
}

SOKOL_API_IMPL void sg_uninit_shader(sg_shader shd_id) {
    SOKOL_ASSERT(_sg.valid);
    _sg_shader_t* shd = _sg_lookup_shader(&_sg.pools, shd_id.id);
    if (shd) {
        if ((shd->slot.state == SG_RESOURCESTATE_VALID) || (shd->slot.state == SG_RESOURCESTATE_FAILED)) {
            _sg_uninit_shader(shd);
            SOKOL_ASSERT(shd->slot.state == SG_RESOURCESTATE_ALLOC);
        } else {
            _SG_ERROR(UNINIT_SHADER_INVALID_STATE);
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
        } else {
            _SG_ERROR(UNINIT_PIPELINE_INVALID_STATE);
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
        } else {
            _SG_ERROR(UNINIT_PASS_INVALID_STATE);
        }
    }
    _SG_TRACE_ARGS(uninit_pass, pass_id);
}

SOKOL_API_IMPL void sg_fail_buffer(sg_buffer buf_id) {
    SOKOL_ASSERT(_sg.valid);
    _sg_buffer_t* buf = _sg_lookup_buffer(&_sg.pools, buf_id.id);
    if (buf) {
        if (buf->slot.state == SG_RESOURCESTATE_ALLOC) {
            buf->slot.ctx_id = _sg.active_context.id;
            buf->slot.state = SG_RESOURCESTATE_FAILED;
        } else {
            _SG_ERROR(FAIL_BUFFER_INVALID_STATE);
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
        } else {
            _SG_ERROR(FAIL_IMAGE_INVALID_STATE);
        }
    }
    _SG_TRACE_ARGS(fail_image, img_id);
}

SOKOL_API_IMPL void sg_fail_sampler(sg_sampler smp_id) {
    SOKOL_ASSERT(_sg.valid);
    _sg_sampler_t* smp = _sg_lookup_sampler(&_sg.pools, smp_id.id);
    if (smp) {
        if (smp->slot.state == SG_RESOURCESTATE_ALLOC) {
            smp->slot.ctx_id = _sg.active_context.id;
            smp->slot.state = SG_RESOURCESTATE_FAILED;
        } else {
            _SG_ERROR(FAIL_SAMPLER_INVALID_STATE);
        }
    }
    _SG_TRACE_ARGS(fail_sampler, smp_id);
}

SOKOL_API_IMPL void sg_fail_shader(sg_shader shd_id) {
    SOKOL_ASSERT(_sg.valid);
    _sg_shader_t* shd = _sg_lookup_shader(&_sg.pools, shd_id.id);
    if (shd) {
        if (shd->slot.state == SG_RESOURCESTATE_ALLOC) {
            shd->slot.ctx_id = _sg.active_context.id;
            shd->slot.state = SG_RESOURCESTATE_FAILED;
        } else {
            _SG_ERROR(FAIL_SHADER_INVALID_STATE);
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
        } else {
            _SG_ERROR(FAIL_PIPELINE_INVALID_STATE);
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
        } else {
            _SG_ERROR(FAIL_PASS_INVALID_STATE);
        }
    }
    _SG_TRACE_ARGS(fail_pass, pass_id);
}

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

SOKOL_API_IMPL sg_resource_state sg_query_sampler_state(sg_sampler smp_id) {
    SOKOL_ASSERT(_sg.valid);
    _sg_sampler_t* smp = _sg_lookup_sampler(&_sg.pools, smp_id.id);
    sg_resource_state res = smp ? smp->slot.state : SG_RESOURCESTATE_INVALID;
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
    _SG_TRACE_ARGS(make_image, &desc_def, img_id);
    return img_id;
}

SOKOL_API_IMPL sg_sampler sg_make_sampler(const sg_sampler_desc* desc) {
    SOKOL_ASSERT(_sg.valid);
    SOKOL_ASSERT(desc);
    sg_sampler_desc desc_def = _sg_sampler_desc_defaults(desc);
    sg_sampler smp_id = _sg_alloc_sampler();
    if (smp_id.id != SG_INVALID_ID) {
        _sg_sampler_t* smp = _sg_sampler_at(&_sg.pools, smp_id.id);
        SOKOL_ASSERT(smp && (smp->slot.state == SG_RESOURCESTATE_ALLOC));
        _sg_init_sampler(smp, &desc_def);
        SOKOL_ASSERT((smp->slot.state == SG_RESOURCESTATE_VALID) || (smp->slot.state == SG_RESOURCESTATE_FAILED));
    }
    _SG_TRACE_ARGS(make_sampler, &desc_def, smp_id);
    return smp_id;
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
    _SG_TRACE_ARGS(make_pass, &desc_def, pass_id);
    return pass_id;
}

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

SOKOL_API_IMPL void sg_destroy_sampler(sg_sampler smp_id) {
    SOKOL_ASSERT(_sg.valid);
    _SG_TRACE_ARGS(destroy_sampler, smp_id);
    _sg_sampler_t* smp = _sg_lookup_sampler(&_sg.pools, smp_id.id);
    if (smp) {
        if ((smp->slot.state == SG_RESOURCESTATE_VALID) || (smp->slot.state == SG_RESOURCESTATE_FAILED)) {
            _sg_uninit_sampler(smp);
            SOKOL_ASSERT(smp->slot.state == SG_RESOURCESTATE_ALLOC);
        }
        if (smp->slot.state == SG_RESOURCESTATE_ALLOC) {
            _sg_dealloc_sampler(smp);
            SOKOL_ASSERT(smp->slot.state == SG_RESOURCESTATE_INITIAL);
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
    _SG_TRACE_ARGS(begin_default_pass, &pa, width, height);
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
        _sg_begin_pass(pass, &pa, pass->cmn.width, pass->cmn.height);
        _SG_TRACE_ARGS(begin_pass, pass_id, &pa);
    } else {
        _sg.pass_valid = false;
    }
}

SOKOL_API_IMPL void sg_apply_viewport(int x, int y, int width, int height, bool origin_top_left) {
    SOKOL_ASSERT(_sg.valid);
    _sg_stats_add(num_apply_viewport, 1);
    if (!_sg.pass_valid) {
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
    _sg_stats_add(num_apply_scissor_rect, 1);
    if (!_sg.pass_valid) {
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
    _sg_stats_add(num_apply_pipeline, 1);
    _sg.bindings_applied = false;
    if (!_sg_validate_apply_pipeline(pip_id)) {
        _sg.next_draw_valid = false;
        return;
    }
    if (!_sg.pass_valid) {
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
    _sg_stats_add(num_apply_bindings, 1);
    if (!_sg_validate_apply_bindings(bindings)) {
        _sg.next_draw_valid = false;
        return;
    }
    _sg.bindings_applied = true;

    _sg_bindings_t bnd;
    _sg_clear(&bnd, sizeof(bnd));
    bnd.pip = _sg_lookup_pipeline(&_sg.pools, _sg.cur_pipeline.id);
    if (0 == bnd.pip) {
        _sg.next_draw_valid = false;
    }

    for (int i = 0; i < SG_MAX_VERTEX_BUFFERS; i++, bnd.num_vbs++) {
        if (bindings->vertex_buffers[i].id) {
            bnd.vbs[i] = _sg_lookup_buffer(&_sg.pools, bindings->vertex_buffers[i].id);
            bnd.vb_offsets[i] = bindings->vertex_buffer_offsets[i];
            if (bnd.vbs[i]) {
                _sg.next_draw_valid &= (SG_RESOURCESTATE_VALID == bnd.vbs[i]->slot.state);
                _sg.next_draw_valid &= !bnd.vbs[i]->cmn.append_overflow;
            } else {
                _sg.next_draw_valid = false;
            }
        } else {
            break;
        }
    }

    if (bindings->index_buffer.id) {
        bnd.ib = _sg_lookup_buffer(&_sg.pools, bindings->index_buffer.id);
        bnd.ib_offset = bindings->index_buffer_offset;
        if (bnd.ib) {
            _sg.next_draw_valid &= (SG_RESOURCESTATE_VALID == bnd.ib->slot.state);
            _sg.next_draw_valid &= !bnd.ib->cmn.append_overflow;
        } else {
            _sg.next_draw_valid = false;
        }
    }

    for (int i = 0; i < SG_MAX_SHADERSTAGE_IMAGES; i++, bnd.num_vs_imgs++) {
        if (bindings->vs.images[i].id) {
            bnd.vs_imgs[i] = _sg_lookup_image(&_sg.pools, bindings->vs.images[i].id);
            if (bnd.vs_imgs[i]) {
                _sg.next_draw_valid &= (SG_RESOURCESTATE_VALID == bnd.vs_imgs[i]->slot.state);
            } else {
                _sg.next_draw_valid = false;
            }
        } else {
            break;
        }
    }

    for (int i = 0; i < SG_MAX_SHADERSTAGE_SAMPLERS; i++, bnd.num_vs_smps++) {
        if (bindings->vs.samplers[i].id) {
            bnd.vs_smps[i] = _sg_lookup_sampler(&_sg.pools, bindings->vs.samplers[i].id);
            if (bnd.vs_smps[i]) {
                _sg.next_draw_valid &= (SG_RESOURCESTATE_VALID == bnd.vs_smps[i]->slot.state);
            } else {
                _sg.next_draw_valid = false;
            }
        } else {
            break;
        }
    }

    for (int i = 0; i < SG_MAX_SHADERSTAGE_IMAGES; i++, bnd.num_fs_imgs++) {
        if (bindings->fs.images[i].id) {
            bnd.fs_imgs[i] = _sg_lookup_image(&_sg.pools, bindings->fs.images[i].id);
            if (bnd.fs_imgs[i]) {
                _sg.next_draw_valid &= (SG_RESOURCESTATE_VALID == bnd.fs_imgs[i]->slot.state);
            } else {
                _sg.next_draw_valid = false;
            }
        } else {
            break;
        }
    }

    for (int i = 0; i < SG_MAX_SHADERSTAGE_SAMPLERS; i++, bnd.num_fs_smps++) {
        if (bindings->fs.samplers[i].id) {
            bnd.fs_smps[i] = _sg_lookup_sampler(&_sg.pools, bindings->fs.samplers[i].id);
            if (bnd.fs_smps[i]) {
                _sg.next_draw_valid &= (SG_RESOURCESTATE_VALID == bnd.fs_smps[i]->slot.state);
            } else {
                _sg.next_draw_valid = false;
            }
        } else {
            break;
        }
    }

    if (_sg.next_draw_valid) {
        _sg.next_draw_valid &= _sg_apply_bindings(&bnd);
        _SG_TRACE_ARGS(apply_bindings, bindings);
    }
}

SOKOL_API_IMPL void sg_apply_uniforms(sg_shader_stage stage, int ub_index, const sg_range* data) {
    SOKOL_ASSERT(_sg.valid);
    SOKOL_ASSERT((stage == SG_SHADERSTAGE_VS) || (stage == SG_SHADERSTAGE_FS));
    SOKOL_ASSERT((ub_index >= 0) && (ub_index < SG_MAX_SHADERSTAGE_UBS));
    SOKOL_ASSERT(data && data->ptr && (data->size > 0));
    _sg_stats_add(num_apply_uniforms, 1);
    _sg_stats_add(size_apply_uniforms, (uint32_t)data->size);
    if (!_sg_validate_apply_uniforms(stage, ub_index, data)) {
        _sg.next_draw_valid = false;
        return;
    }
    if (!_sg.pass_valid) {
        return;
    }
    if (!_sg.next_draw_valid) {
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
    _sg_stats_add(num_draw, 1);
    #if defined(SOKOL_DEBUG)
        if (!_sg.bindings_applied) {
            _SG_WARN(DRAW_WITHOUT_BINDINGS);
        }
    #endif
    if (!_sg.pass_valid) {
        return;
    }
    if (!_sg.next_draw_valid) {
        return;
    }
    if (!_sg.bindings_applied) {
        return;
    }
    /* attempting to draw with zero elements or instances is not technically an
       error, but might be handled as an error in the backend API (e.g. on Metal)
    */
    if ((0 == num_elements) || (0 == num_instances)) {
        return;
    }
    _sg_draw(base_element, num_elements, num_instances);
    _SG_TRACE_ARGS(draw, base_element, num_elements, num_instances);
}

SOKOL_API_IMPL void sg_end_pass(void) {
    SOKOL_ASSERT(_sg.valid);
    _sg_stats_add(num_passes, 1);
    if (!_sg.pass_valid) {
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
    _sg.stats.frame_index = _sg.frame_index;
    _sg.prev_stats = _sg.stats;
    _sg_clear(&_sg.stats, sizeof(_sg.stats));
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
    _sg_stats_add(num_update_buffer, 1);
    _sg_stats_add(size_update_buffer, (uint32_t)data->size);
    _sg_buffer_t* buf = _sg_lookup_buffer(&_sg.pools, buf_id.id);
    if ((data->size > 0) && buf && (buf->slot.state == SG_RESOURCESTATE_VALID)) {
        if (_sg_validate_update_buffer(buf, data)) {
            SOKOL_ASSERT(data->size <= (size_t)buf->cmn.size);
            // only one update allowed per buffer and frame
            SOKOL_ASSERT(buf->cmn.update_frame_index != _sg.frame_index);
            // update and append on same buffer in same frame not allowed
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
    _sg_stats_add(num_append_buffer, 1);
    _sg_stats_add(size_append_buffer, (uint32_t)data->size);
    _sg_buffer_t* buf = _sg_lookup_buffer(&_sg.pools, buf_id.id);
    int result;
    if (buf) {
        // rewind append cursor in a new frame
        if (buf->cmn.append_frame_index != _sg.frame_index) {
            buf->cmn.append_pos = 0;
            buf->cmn.append_overflow = false;
        }
        if (((size_t)buf->cmn.append_pos + data->size) > (size_t)buf->cmn.size) {
            buf->cmn.append_overflow = true;
        }
        const int start_pos = buf->cmn.append_pos;
        // NOTE: the multiple-of-4 requirement for the buffer offset is coming
        // from WebGPU, but we want identical behaviour between backends
        SOKOL_ASSERT(_sg_multiple_u64((uint64_t)start_pos, 4));
        if (buf->slot.state == SG_RESOURCESTATE_VALID) {
            if (_sg_validate_append_buffer(buf, data)) {
                if (!buf->cmn.append_overflow && (data->size > 0)) {
                    // update and append on same buffer in same frame not allowed
                    SOKOL_ASSERT(buf->cmn.update_frame_index != _sg.frame_index);
                    _sg_append_buffer(buf, data, buf->cmn.append_frame_index != _sg.frame_index);
                    buf->cmn.append_pos += (int) _sg_roundup_u64(data->size, 4);
                    buf->cmn.append_frame_index = _sg.frame_index;
                }
            }
        }
        result = start_pos;
    } else {
        // FIXME: should we return -1 here?
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
        // rewind append cursor in a new frame
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
    _sg_stats_add(num_update_image, 1);
    for (int face_index = 0; face_index < SG_CUBEFACE_NUM; face_index++) {
        for (int mip_index = 0; mip_index < SG_MAX_MIPMAPS; mip_index++) {
            if (data->subimage[face_index][mip_index].size == 0) {
                break;
            }
            _sg_stats_add(size_update_image, (uint32_t)data->subimage[face_index][mip_index].size);
        }
    }
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
    _sg_push_debug_group(name);
    _SG_TRACE_ARGS(push_debug_group, name);
}

SOKOL_API_IMPL void sg_pop_debug_group(void) {
    SOKOL_ASSERT(_sg.valid);
    _sg_pop_debug_group();
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

SOKOL_API_IMPL void sg_enable_frame_stats(void) {
    SOKOL_ASSERT(_sg.valid);
    _sg.stats_enabled = true;
}

SOKOL_API_IMPL void sg_disable_frame_stats(void) {
    SOKOL_ASSERT(_sg.valid);
    _sg.stats_enabled = false;
}

SOKOL_API_IMPL bool sg_frame_stats_enabled(void) {
    return _sg.stats_enabled;
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
    }
    return info;
}

SOKOL_API_IMPL sg_sampler_info sg_query_sampler_info(sg_sampler smp_id) {
    SOKOL_ASSERT(_sg.valid);
    sg_sampler_info info;
    _sg_clear(&info, sizeof(info));
    const _sg_sampler_t* smp = _sg_lookup_sampler(&_sg.pools, smp_id.id);
    if (smp) {
        info.slot.state = smp->slot.state;
        info.slot.res_id = smp->slot.id;
        info.slot.ctx_id = smp->slot.ctx_id;
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

SOKOL_API_IMPL sg_buffer_desc sg_query_buffer_desc(sg_buffer buf_id) {
    SOKOL_ASSERT(_sg.valid);
    sg_buffer_desc desc;
    _sg_clear(&desc, sizeof(desc));
    const _sg_buffer_t* buf = _sg_lookup_buffer(&_sg.pools, buf_id.id);
    if (buf) {
        desc.size = (size_t)buf->cmn.size;
        desc.type = buf->cmn.type;
        desc.usage = buf->cmn.usage;
    }
    return desc;
}

SOKOL_API_IMPL sg_image_desc sg_query_image_desc(sg_image img_id) {
    SOKOL_ASSERT(_sg.valid);
    sg_image_desc desc;
    _sg_clear(&desc, sizeof(desc));
    const _sg_image_t* img = _sg_lookup_image(&_sg.pools, img_id.id);
    if (img) {
        desc.type = img->cmn.type;
        desc.render_target = img->cmn.render_target;
        desc.width = img->cmn.width;
        desc.height = img->cmn.height;
        desc.num_slices = img->cmn.num_slices;
        desc.num_mipmaps = img->cmn.num_mipmaps;
        desc.usage = img->cmn.usage;
        desc.pixel_format = img->cmn.pixel_format;
        desc.sample_count = img->cmn.sample_count;
    }
    return desc;
}

SOKOL_API_IMPL sg_sampler_desc sg_query_sampler_desc(sg_sampler smp_id) {
    SOKOL_ASSERT(_sg.valid);
    sg_sampler_desc desc;
    _sg_clear(&desc, sizeof(desc));
    const _sg_sampler_t* smp = _sg_lookup_sampler(&_sg.pools, smp_id.id);
    if (smp) {
        desc.min_filter = smp->cmn.min_filter;
        desc.mag_filter = smp->cmn.mag_filter;
        desc.mipmap_filter = smp->cmn.mipmap_filter;
        desc.wrap_u = smp->cmn.wrap_u;
        desc.wrap_v = smp->cmn.wrap_v;
        desc.wrap_w = smp->cmn.wrap_w;
        desc.min_lod = smp->cmn.min_lod;
        desc.max_lod = smp->cmn.max_lod;
        desc.border_color = smp->cmn.border_color;
        desc.compare = smp->cmn.compare;
        desc.max_anisotropy = smp->cmn.max_anisotropy;
    }
    return desc;
}

SOKOL_API_IMPL sg_shader_desc sg_query_shader_desc(sg_shader shd_id) {
    SOKOL_ASSERT(_sg.valid);
    sg_shader_desc desc;
    _sg_clear(&desc, sizeof(desc));
    const _sg_shader_t* shd = _sg_lookup_shader(&_sg.pools, shd_id.id);
    if (shd) {
        for (int stage_idx = 0; stage_idx < SG_NUM_SHADER_STAGES; stage_idx++) {
            sg_shader_stage_desc* stage_desc = (stage_idx == 0) ? &desc.vs : &desc.fs;
            const _sg_shader_stage_t* stage = &shd->cmn.stage[stage_idx];
            for (int ub_idx = 0; ub_idx < stage->num_uniform_blocks; ub_idx++) {
                sg_shader_uniform_block_desc* ub_desc = &stage_desc->uniform_blocks[ub_idx];
                const _sg_shader_uniform_block_t* ub = &stage->uniform_blocks[ub_idx];
                ub_desc->size = ub->size;
            }
            for (int img_idx = 0; img_idx < stage->num_images; img_idx++) {
                sg_shader_image_desc* img_desc = &stage_desc->images[img_idx];
                const _sg_shader_image_t* img = &stage->images[img_idx];
                img_desc->used = true;
                img_desc->image_type = img->image_type;
                img_desc->sample_type = img->sample_type;
                img_desc->multisampled = img->multisampled;
            }
            for (int smp_idx = 0; smp_idx < stage->num_samplers; smp_idx++) {
                sg_shader_sampler_desc* smp_desc = &stage_desc->samplers[smp_idx];
                const _sg_shader_sampler_t* smp = &stage->samplers[smp_idx];
                smp_desc->used = true;
                smp_desc->sampler_type = smp->sampler_type;
            }
            for (int img_smp_idx = 0; img_smp_idx < stage->num_image_samplers; img_smp_idx++) {
                sg_shader_image_sampler_pair_desc* img_smp_desc = &stage_desc->image_sampler_pairs[img_smp_idx];
                const _sg_shader_image_sampler_t* img_smp = &stage->image_samplers[img_smp_idx];
                img_smp_desc->used = true;
                img_smp_desc->image_slot = img_smp->image_slot;
                img_smp_desc->sampler_slot = img_smp->sampler_slot;
                img_smp_desc->glsl_name = 0;
            }
        }
    }
    return desc;
}

SOKOL_API_IMPL sg_pipeline_desc sg_query_pipeline_desc(sg_pipeline pip_id) {
    SOKOL_ASSERT(_sg.valid);
    sg_pipeline_desc desc;
    _sg_clear(&desc, sizeof(desc));
    const _sg_pipeline_t* pip = _sg_lookup_pipeline(&_sg.pools, pip_id.id);
    if (pip) {
        desc.shader = pip->cmn.shader_id;
        desc.layout = pip->cmn.layout;
        desc.depth = pip->cmn.depth;
        desc.stencil = pip->cmn.stencil;
        desc.color_count = pip->cmn.color_count;
        for (int i = 0; i < pip->cmn.color_count; i++) {
            desc.colors[i] = pip->cmn.colors[i];
        }
        desc.primitive_type = pip->cmn.primitive_type;
        desc.index_type = pip->cmn.index_type;
        desc.cull_mode = pip->cmn.cull_mode;
        desc.face_winding = pip->cmn.face_winding;
        desc.sample_count = pip->cmn.sample_count;
        desc.blend_color = pip->cmn.blend_color;
        desc.alpha_to_coverage_enabled = pip->cmn.alpha_to_coverage_enabled;
    }
    return desc;
}

SOKOL_API_IMPL sg_pass_desc sg_query_pass_desc(sg_pass pass_id) {
    SOKOL_ASSERT(_sg.valid);
    sg_pass_desc desc;
    _sg_clear(&desc, sizeof(desc));
    const _sg_pass_t* pass = _sg_lookup_pass(&_sg.pools, pass_id.id);
    if (pass) {
        for (int i = 0; i < pass->cmn.num_color_atts; i++) {
            desc.color_attachments[i].image = pass->cmn.color_atts[i].image_id;
            desc.color_attachments[i].mip_level = pass->cmn.color_atts[i].mip_level;
            desc.color_attachments[i].slice = pass->cmn.color_atts[i].slice;
        }
        desc.depth_stencil_attachment.image = pass->cmn.ds_att.image_id;
        desc.depth_stencil_attachment.mip_level = pass->cmn.ds_att.mip_level;
        desc.depth_stencil_attachment.slice = pass->cmn.ds_att.slice;
    }
    return desc;
}

SOKOL_API_IMPL sg_buffer_desc sg_query_buffer_defaults(const sg_buffer_desc* desc) {
    SOKOL_ASSERT(_sg.valid && desc);
    return _sg_buffer_desc_defaults(desc);
}

SOKOL_API_IMPL sg_image_desc sg_query_image_defaults(const sg_image_desc* desc) {
    SOKOL_ASSERT(_sg.valid && desc);
    return _sg_image_desc_defaults(desc);
}

SOKOL_API_IMPL sg_sampler_desc sg_query_sampler_defaults(const sg_sampler_desc* desc) {
    SOKOL_ASSERT(_sg.valid && desc);
    return _sg_sampler_desc_defaults(desc);
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

SOKOL_API_IMPL const void* sg_d3d11_device_context(void) {
    #if defined(SOKOL_D3D11)
        return (const void*) _sg.d3d11.ctx;
    #else
        return 0;
    #endif
}

SOKOL_API_IMPL sg_d3d11_buffer_info sg_d3d11_query_buffer_info(sg_buffer buf_id) {
    SOKOL_ASSERT(_sg.valid);
    sg_d3d11_buffer_info res;
    _sg_clear(&res, sizeof(res));
    #if defined(SOKOL_D3D11)
        const _sg_buffer_t* buf = _sg_lookup_buffer(&_sg.pools, buf_id.id);
        if (buf) {
            res.buf = (const void*) buf->d3d11.buf;
        }
    #else
        _SOKOL_UNUSED(buf_id);
    #endif
    return res;
}

SOKOL_API_IMPL sg_d3d11_image_info sg_d3d11_query_image_info(sg_image img_id) {
    SOKOL_ASSERT(_sg.valid);
    sg_d3d11_image_info res;
    _sg_clear(&res, sizeof(res));
    #if defined(SOKOL_D3D11)
        const _sg_image_t* img = _sg_lookup_image(&_sg.pools, img_id.id);
        if (img) {
            res.tex2d = (const void*) img->d3d11.tex2d;
            res.tex3d = (const void*) img->d3d11.tex3d;
            res.res = (const void*) img->d3d11.res;
            res.srv = (const void*) img->d3d11.srv;
        }
    #else
        _SOKOL_UNUSED(img_id);
    #endif
    return res;
}

SOKOL_API_IMPL sg_d3d11_sampler_info sg_d3d11_query_sampler_info(sg_sampler smp_id) {
    SOKOL_ASSERT(_sg.valid);
    sg_d3d11_sampler_info res;
    _sg_clear(&res, sizeof(res));
    #if defined(SOKOL_D3D11)
        const _sg_sampler_t* smp = _sg_lookup_sampler(&_sg.pools, smp_id.id);
        if (smp) {
            res.smp = (const void*) smp->d3d11.smp;
        }
    #else
        _SOKOL_UNUSED(smp_id);
    #endif
    return res;
}

SOKOL_API_IMPL sg_d3d11_shader_info sg_d3d11_query_shader_info(sg_shader shd_id) {
    SOKOL_ASSERT(_sg.valid);
    sg_d3d11_shader_info res;
    _sg_clear(&res, sizeof(res));
    #if defined(SOKOL_D3D11)
        const _sg_shader_t* shd = _sg_lookup_shader(&_sg.pools, shd_id.id);
        if (shd) {
            for (int i = 0; i < SG_MAX_SHADERSTAGE_UBS; i++) {
                res.vs_cbufs[i] = (const void*) shd->d3d11.stage[SG_SHADERSTAGE_VS].cbufs[i];
                res.fs_cbufs[i] = (const void*) shd->d3d11.stage[SG_SHADERSTAGE_FS].cbufs[i];
            }
            res.vs = (const void*) shd->d3d11.vs;
            res.fs = (const void*) shd->d3d11.fs;
        }
    #else
        _SOKOL_UNUSED(shd_id);
    #endif
    return res;
}

SOKOL_API_IMPL sg_d3d11_pipeline_info sg_d3d11_query_pipeline_info(sg_pipeline pip_id) {
    SOKOL_ASSERT(_sg.valid);
    sg_d3d11_pipeline_info res;
    _sg_clear(&res, sizeof(res));
    #if defined(SOKOL_D3D11)
        const _sg_pipeline_t* pip = _sg_lookup_pipeline(&_sg.pools, pip_id.id);
        if (pip) {
            res.il = (const void*) pip->d3d11.il;
            res.rs = (const void*) pip->d3d11.rs;
            res.dss = (const void*) pip->d3d11.dss;
            res.bs = (const void*) pip->d3d11.bs;
        }
    #else
        _SOKOL_UNUSED(pip_id);
    #endif
    return res;
}

SOKOL_API_IMPL sg_d3d11_pass_info sg_d3d11_query_pass_info(sg_pass pass_id) {
    SOKOL_ASSERT(_sg.valid);
    sg_d3d11_pass_info res;
    _sg_clear(&res, sizeof(res));
    #if defined(SOKOL_D3D11)
        const _sg_pass_t* pass = _sg_lookup_pass(&_sg.pools, pass_id.id);
        if (pass) {
            for (int i = 0; i < SG_MAX_COLOR_ATTACHMENTS; i++) {
                res.color_rtv[i] = (const void*) pass->d3d11.color_atts[i].view.rtv;
                res.resolve_rtv[i] = (const void*) pass->d3d11.resolve_atts[i].view.rtv;
            }
            res.dsv = (const void*) pass->d3d11.ds_att.view.dsv;
        }
    #else
        _SOKOL_UNUSED(pass_id);
    #endif
    return res;
}

SOKOL_API_IMPL const void* sg_mtl_device(void) {
    #if defined(SOKOL_METAL)
        if (nil != _sg.mtl.device) {
            return (__bridge const void*) _sg.mtl.device;
        } else {
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
        } else {
            return 0;
        }
    #else
        return 0;
    #endif
}

SOKOL_API_IMPL sg_mtl_buffer_info sg_mtl_query_buffer_info(sg_buffer buf_id) {
    SOKOL_ASSERT(_sg.valid);
    sg_mtl_buffer_info res;
    _sg_clear(&res, sizeof(res));
    #if defined(SOKOL_METAL)
        const _sg_buffer_t* buf = _sg_lookup_buffer(&_sg.pools, buf_id.id);
        if (buf) {
            for (int i = 0; i < SG_NUM_INFLIGHT_FRAMES; i++) {
                if (buf->mtl.buf[i] != 0) {
                    res.buf[i] = (__bridge void*) _sg_mtl_id(buf->mtl.buf[i]);
                }
            }
            res.active_slot = buf->cmn.active_slot;
        }
    #else
        _SOKOL_UNUSED(buf_id);
    #endif
    return res;
}

SOKOL_API_IMPL sg_mtl_image_info sg_mtl_query_image_info(sg_image img_id) {
    SOKOL_ASSERT(_sg.valid);
    sg_mtl_image_info res;
    _sg_clear(&res, sizeof(res));
    #if defined(SOKOL_METAL)
        const _sg_image_t* img = _sg_lookup_image(&_sg.pools, img_id.id);
        if (img) {
            for (int i = 0; i < SG_NUM_INFLIGHT_FRAMES; i++) {
                if (img->mtl.tex[i] != 0) {
                    res.tex[i] = (__bridge void*) _sg_mtl_id(img->mtl.tex[i]);
                }
            }
            res.active_slot = img->cmn.active_slot;
        }
    #else
        _SOKOL_UNUSED(img_id);
    #endif
    return res;
}

SOKOL_API_IMPL sg_mtl_sampler_info sg_mtl_query_sampler_info(sg_sampler smp_id) {
    SOKOL_ASSERT(_sg.valid);
    sg_mtl_sampler_info res;
    _sg_clear(&res, sizeof(res));
    #if defined(SOKOL_METAL)
        const _sg_sampler_t* smp = _sg_lookup_sampler(&_sg.pools, smp_id.id);
        if (smp) {
            if (smp->mtl.sampler_state != 0) {
                res.smp = (__bridge void*) _sg_mtl_id(smp->mtl.sampler_state);
            }
        }
    #else
        _SOKOL_UNUSED(smp_id);
    #endif
    return res;
}

SOKOL_API_IMPL sg_mtl_shader_info sg_mtl_query_shader_info(sg_shader shd_id) {
    SOKOL_ASSERT(_sg.valid);
    sg_mtl_shader_info res;
    _sg_clear(&res, sizeof(res));
    #if defined(SOKOL_METAL)
        const _sg_shader_t* shd = _sg_lookup_shader(&_sg.pools, shd_id.id);
        if (shd) {
            const int vs_lib  = shd->mtl.stage[SG_SHADERSTAGE_VS].mtl_lib;
            const int vs_func = shd->mtl.stage[SG_SHADERSTAGE_VS].mtl_func;
            const int fs_lib  = shd->mtl.stage[SG_SHADERSTAGE_FS].mtl_lib;
            const int fs_func = shd->mtl.stage[SG_SHADERSTAGE_FS].mtl_func;
            if (vs_lib != 0) {
                res.vs_lib = (__bridge void*) _sg_mtl_id(vs_lib);
            }
            if (fs_lib != 0) {
                res.fs_lib = (__bridge void*) _sg_mtl_id(fs_lib);
            }
            if (vs_func != 0) {
                res.vs_func = (__bridge void*) _sg_mtl_id(vs_func);
            }
            if (fs_func != 0) {
                res.fs_func = (__bridge void*) _sg_mtl_id(fs_func);
            }
        }
    #else
        _SOKOL_UNUSED(shd_id);
    #endif
    return res;
}

SOKOL_API_IMPL sg_mtl_pipeline_info sg_mtl_query_pipeline_info(sg_pipeline pip_id) {
    SOKOL_ASSERT(_sg.valid);
    sg_mtl_pipeline_info res;
    _sg_clear(&res, sizeof(res));
    #if defined(SOKOL_METAL)
        const _sg_pipeline_t* pip = _sg_lookup_pipeline(&_sg.pools, pip_id.id);
        if (pip) {
            if (pip->mtl.rps != 0) {
                res.rps = (__bridge void*) _sg_mtl_id(pip->mtl.rps);
            }
            if (pip->mtl.dss != 0) {
                res.dss = (__bridge void*) _sg_mtl_id(pip->mtl.dss);
            }
        }
    #else
        _SOKOL_UNUSED(pip_id);
    #endif
    return res;
}

SOKOL_API_IMPL const void* sg_wgpu_device(void) {
    #if defined(SOKOL_WGPU)
        return (const void*) _sg.wgpu.dev;
    #else
        return 0;
    #endif
}

SOKOL_API_IMPL const void* sg_wgpu_queue(void) {
    #if defined(SOKOL_WGPU)
        return (const void*) _sg.wgpu.queue;
    #else
        return 0;
    #endif
}

SOKOL_API_IMPL const void* sg_wgpu_command_encoder(void) {
    #if defined(SOKOL_WGPU)
        return (const void*) _sg.wgpu.cmd_enc;
    #else
        return 0;
    #endif
}

SOKOL_API_IMPL const void* sg_wgpu_render_pass_encoder(void) {
    #if defined(SOKOL_WGPU)
        return (const void*) _sg.wgpu.pass_enc;
    #else
        return 0;
    #endif
}

SOKOL_API_IMPL sg_wgpu_buffer_info sg_wgpu_query_buffer_info(sg_buffer buf_id) {
    SOKOL_ASSERT(_sg.valid);
    sg_wgpu_buffer_info res;
    _sg_clear(&res, sizeof(res));
    #if defined(SOKOL_WGPU)
        const _sg_buffer_t* buf = _sg_lookup_buffer(&_sg.pools, buf_id.id);
        if (buf) {
            res.buf = (const void*) buf->wgpu.buf;
        }
    #else
        _SOKOL_UNUSED(buf_id);
    #endif
    return res;
}

SOKOL_API_IMPL sg_wgpu_image_info sg_wgpu_query_image_info(sg_image img_id) {
    SOKOL_ASSERT(_sg.valid);
    sg_wgpu_image_info res;
    _sg_clear(&res, sizeof(res));
    #if defined(SOKOL_WGPU)
        const _sg_image_t* img = _sg_lookup_image(&_sg.pools, img_id.id);
        if (img) {
            res.tex = (const void*) img->wgpu.tex;
            res.view = (const void*) img->wgpu.view;
        }
    #else
        _SOKOL_UNUSED(img_id);
    #endif
    return res;
}

SOKOL_API_IMPL sg_wgpu_sampler_info sg_wgpu_query_sampler_info(sg_sampler smp_id) {
    SOKOL_ASSERT(_sg.valid);
    sg_wgpu_sampler_info res;
    _sg_clear(&res, sizeof(res));
    #if defined(SOKOL_WGPU)
        const _sg_sampler_t* smp = _sg_lookup_sampler(&_sg.pools, smp_id.id);
        if (smp) {
            res.smp = (const void*) smp->wgpu.smp;
        }
    #else
        _SOKOL_UNUSED(smp_id);
    #endif
    return res;
}

SOKOL_API_IMPL sg_wgpu_shader_info sg_wgpu_query_shader_info(sg_shader shd_id) {
    SOKOL_ASSERT(_sg.valid);
    sg_wgpu_shader_info res;
    _sg_clear(&res, sizeof(res));
    #if defined(SOKOL_WGPU)
        const _sg_shader_t* shd = _sg_lookup_shader(&_sg.pools, shd_id.id);
        if (shd) {
            res.vs_mod = (const void*) shd->wgpu.stage[SG_SHADERSTAGE_VS].module;
            res.fs_mod = (const void*) shd->wgpu.stage[SG_SHADERSTAGE_FS].module;
            res.bgl = (const void*) shd->wgpu.bind_group_layout;
        }
    #else
        _SOKOL_UNUSED(shd_id);
    #endif
    return res;
}

SOKOL_API_IMPL sg_wgpu_pipeline_info sg_wgpu_query_pipeline_info(sg_pipeline pip_id) {
    SOKOL_ASSERT(_sg.valid);
    sg_wgpu_pipeline_info res;
    _sg_clear(&res, sizeof(res));
    #if defined(SOKOL_WGPU)
        const _sg_pipeline_t* pip = _sg_lookup_pipeline(&_sg.pools, pip_id.id);
        if (pip) {
            res.pip = (const void*) pip->wgpu.pip;
        }
    #else
        _SOKOL_UNUSED(pip_id);
    #endif
    return res;
}

SOKOL_API_IMPL sg_wgpu_pass_info sg_wgpu_query_pass_info(sg_pass pass_id) {
    SOKOL_ASSERT(_sg.valid);
    sg_wgpu_pass_info res;
    _sg_clear(&res, sizeof(res));
    #if defined(SOKOL_WGPU)
        const _sg_pass_t* pass = _sg_lookup_pass(&_sg.pools, pass_id.id);
        if (pass) {
            for (int i = 0; i < SG_MAX_COLOR_ATTACHMENTS; i++) {
                res.color_view[i] = (const void*) pass->wgpu.color_atts[i].view;
                res.resolve_view[i] = (const void*) pass->wgpu.resolve_atts[i].view;
            }
            res.ds_view = (const void*) pass->wgpu.ds_att.view;
        }
    #else
        _SOKOL_UNUSED(pass_id);
    #endif
    return res;
}

SOKOL_API_IMPL sg_gl_buffer_info sg_gl_query_buffer_info(sg_buffer buf_id) {
    SOKOL_ASSERT(_sg.valid);
    sg_gl_buffer_info res;
    _sg_clear(&res, sizeof(res));
    #if defined(_SOKOL_ANY_GL)
        const _sg_buffer_t* buf = _sg_lookup_buffer(&_sg.pools, buf_id.id);
        if (buf) {
            for (int i = 0; i < SG_NUM_INFLIGHT_FRAMES; i++) {
                res.buf[i] = buf->gl.buf[i];
            }
            res.active_slot = buf->cmn.active_slot;
        }
    #else
        _SOKOL_UNUSED(buf_id);
    #endif
    return res;
}

SOKOL_API_IMPL sg_gl_image_info sg_gl_query_image_info(sg_image img_id) {
    SOKOL_ASSERT(_sg.valid);
    sg_gl_image_info res;
    _sg_clear(&res, sizeof(res));
    #if defined(_SOKOL_ANY_GL)
        const _sg_image_t* img = _sg_lookup_image(&_sg.pools, img_id.id);
        if (img) {
            for (int i = 0; i < SG_NUM_INFLIGHT_FRAMES; i++) {
                res.tex[i] = img->gl.tex[i];
            }
            res.tex_target = img->gl.target;
            res.msaa_render_buffer = img->gl.msaa_render_buffer;
            res.active_slot = img->cmn.active_slot;
        }
    #else
        _SOKOL_UNUSED(img_id);
    #endif
    return res;
}

SOKOL_API_IMPL sg_gl_sampler_info sg_gl_query_sampler_info(sg_sampler smp_id) {
    SOKOL_ASSERT(_sg.valid);
    sg_gl_sampler_info res;
    _sg_clear(&res, sizeof(res));
    #if defined(_SOKOL_ANY_GL)
        const _sg_sampler_t* smp = _sg_lookup_sampler(&_sg.pools, smp_id.id);
        if (smp) {
            res.smp = smp->gl.smp;
        }
    #else
        _SOKOL_UNUSED(smp_id);
    #endif
    return res;
}

SOKOL_API_IMPL sg_gl_shader_info sg_gl_query_shader_info(sg_shader shd_id) {
    SOKOL_ASSERT(_sg.valid);
    sg_gl_shader_info res;
    _sg_clear(&res, sizeof(res));
    #if defined(_SOKOL_ANY_GL)
        const _sg_shader_t* shd = _sg_lookup_shader(&_sg.pools, shd_id.id);
        if (shd) {
            res.prog = shd->gl.prog;
        }
    #else
        _SOKOL_UNUSED(shd_id);
    #endif
    return res;
}

SOKOL_API_IMPL sg_gl_pass_info sg_gl_query_pass_info(sg_pass pass_id) {
    SOKOL_ASSERT(_sg.valid);
    sg_gl_pass_info res;
    _sg_clear(&res, sizeof(res));
    #if defined(_SOKOL_ANY_GL)
        const _sg_pass_t* pass = _sg_lookup_pass(&_sg.pools, pass_id.id);
        if (pass) {
            res.frame_buffer = pass->gl.fb;
            for (int i = 0; i < SG_MAX_COLOR_ATTACHMENTS; i++) {
                res.msaa_resolve_framebuffer[i] = pass->gl.msaa_resolve_framebuffer[i];
            }
        }
    #else
        _SOKOL_UNUSED(pass_id);
    #endif
    return res;
}

#ifdef _MSC_VER
#pragma warning(pop)
#endif

#endif // SOKOL_GFX_IMPL
