#pragma once
/*
    sokol_gfx.h -- simple 3D API wrapper

    Do this:
        #define SOKOL_IMPL
    before you include this file in *one* C or C++ file to create the 
    implementation.

    In the same place define one of the following to select the rendering
    backend:
        #define SOKOL_GLCORE33
        #define SOKOL_GLES2
        #define SOKOL_GLES3
        #define SOKOL_D3D11
        #define SOKOL_METAL

    I.e. for the GL 3.3 Core Profile it should look like this:

    #include ...
    #include ...
    #define SOKOL_IMPL
    #define SOKOL_GLCORE33
    #include "sokol_gfx.h"

    To enable shader compilation support in the D3D11 backend:
        #define SOKOL_D3D11_SHADER_COMPILER

    If SOKOL_D3D11_SHADER_COMPILER is enabled, the executable will link against 
    d3dcompiler.lib (d3dcompiler_47.dll).

    Optionally provide the following defines with your own implementations:

    SOKOL_ASSERT(c)     - your own assert macro (default: assert(c))
    SOKOL_MALLOC(s)     - your own malloc function (default: malloc(s))
    SOKOL_FREE(p)       - your own free function (default: free(p))
    SOKOL_LOG(msg)      - your own logging function (default: puts(msg))
    SOKOL_UNREACHABLE() - a guard macro for unreachable code (default: assert(false))

    API usage validation macros:

    SOKOL_VALIDATE_BEGIN()      - begin a validation block (default:_sg_validate_begin())
    SOKOL_VALIDATE(cond, err)   - like assert but for API validation (default: _sg_validate(cond, err)) 
    SOKOL_VALIDATE_END()        - end a validation block, return true if all checks in block passed (default: bool _sg_validate())

    If you don't want validation errors to be fatal, define SOKOL_VALIDATE_NON_FATAL,
    be aware though that this may spam SOKOL_LOG messages.

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
      source-code or shader-bytecode must be provided

    For complete code examples using the various backend 3D-APIs, see:

        https://github.com/floooh/sokol-samples


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

            sg_begin_default_pass(const sg_pass_action* actions, int width, int height)

    --- or start rendering to an offscreen framebuffer with:

            sg_begin_pass(sg_pass pass, const sg_pass_action* actions)

    --- fill an sg_draw_state struct with the resource bindings for the next 
        draw call (one pipeline object, 1..N vertex buffers, 0 or 1
        index buffer, 0..N image objects to use as textures each on 
        the vertex-shader- and fragment-shader-stage and then call
            
            sg_apply_draw_state(const sg_draw_state* draw_state)

        to update the resource bindings

    --- optionally update shader uniform data with:

            sg_apply_uniform_block(sg_shader_stage stage, int ub_index, const void* data, int num_bytes)

    --- kick off a draw call with:
    
            sg_draw(int base_element, int num_elements, int num_instances)

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

    --- to set a new scissor rect, call:

            sg_apply_scissor_rect(int x, int y, int width, int height, bool origin_top_left)

        both sg_apply_viewport() and sg_apply_scissor_rect() must be called
        inside a rendering pass
        
        beginning a pass will reset the viewport to the size of the framebuffer used 
        in the new pass, 

    --- to update the content of buffer and image resources, call:

            sg_update_buffer(sg_buffer buf, const void* ptr, int num_bytes)
            sg_update_image(sg_image img, const sg_image_content* content)
        
        buffers and images to be updated must have been created with 
        SG_USAGE_DYNAMIC or SG_USAGE_STREAM
        
    --- to check for support of optional features:
        
            bool sg_query_feature(sg_feature feature)

    --- if you need to call into the underlying 3D-API directly, you must call:

            sg_reset_state_cache()

        ...before calling sokol_gfx functions again

    BACKEND-SPECIFIC TOPICS:
    ========================
    --- the GL backends need to know about the internal structure of uniform 
        blocks, and the texture sampler-name and -type:

            typedef struct {
                float mvp[16];      // model-view-projection matrix
                float offset0[2];   // some 2D vectors
                float offset1[2];
                float offset2[2];
            } params_t;
             
            // uniform block structure and texture image definition in sg_shader_desc:
            sg_shader_desc desc = {
                // uniform block description (size and internal structure)
                .vs.uniform_blocks[0] = {
                    .size = sizeof(params_t),
                    .uniforms = {
                        [0] = { .name="mvp", .type=SG_UNIFORMTYPE_MAT4 },
                        [1] = { .name="offset0", .type=SG_UNIFORMTYPE_VEC2 },
                        ...
                    }
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

    --- when creating a pipeline object, GLES2/WebGL need to know the vertex
        attribute names as used in the vertex shader when describing vertex 
        layouts:

            sg_pipeline_desc desc = {
                .layout = {
                    .attrs = {
                        [0] = { .name="position", .format=SG_VERTEXFORMAT_FLOAT3 },
                        [1] = { .name="color1", .format=SG_VERTEXFORMAT_FLOAT4 }
                    }
                }
            };

    --- on D3D11 you need to provide a semantic name and semantic index in the
        vertex attribute definition instead (see the D3D11 documentation on
        D3D11_INPUT_ELEMENT_DESC for details):

            sg_pipeline_desc desc = {
                .layout = {
                    .attrs = {
                        [0] = { .sem_name="POSITION", .sem_index=0, .format=SG_VERTEXFORMAT_FLOAT3 },
                        [1] = { .sem_name="COLOR", .sem_index=1, .format=SG_VERTEXFORMAT_FLOAT4 }
                    }
                }
            };

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

    A default context will be created and activated implicitely in
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

    For more information, check out the the multiwindow-glfw sample:

    https://github.com/floooh/sokol-samples/blob/master/glfw/multiwindow-glfw.c

    TODO:
    ====
    - talk about asynchronous resource creation
    
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
#include <stdint.h>
#include <stdbool.h>

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
typedef struct { uint32_t id; } sg_buffer;
typedef struct { uint32_t id; } sg_image;
typedef struct { uint32_t id; } sg_shader;
typedef struct { uint32_t id; } sg_pipeline;
typedef struct { uint32_t id; } sg_pass;
typedef struct { uint32_t id; } sg_context;

/* 
    various compile-time constants

    FIXME: it may make sense to convert some of those into defines so
    that the user code can override them.
*/
enum {
    SG_INVALID_ID = 0,
    SG_NUM_SHADER_STAGES = 2,
    SG_NUM_INFLIGHT_FRAMES = 2,
    SG_MAX_COLOR_ATTACHMENTS = 4,
    SG_MAX_SHADERSTAGE_BUFFERS = 4,
    SG_MAX_SHADERSTAGE_IMAGES = 12,
    SG_MAX_SHADERSTAGE_UBS = 4,
    SG_MAX_UB_MEMBERS = 16,
    SG_MAX_VERTEX_ATTRIBUTES = 16,
    SG_MAX_MIPMAPS = 16,
    SG_MAX_TEXTUREARRAY_LAYERS = 128
};

/*
    sg_feature

    These are optional features, use the function
    sg_query_feature() to check whether the feature is supported.
*/
typedef enum {
    SG_FEATURE_INSTANCING,
    SG_FEATURE_TEXTURE_COMPRESSION_DXT,
    SG_FEATURE_TEXTURE_COMPRESSION_PVRTC,
    SG_FEATURE_TEXTURE_COMPRESSION_ATC,
    SG_FEATURE_TEXTURE_COMPRESSION_ETC2,
    SG_FEATURE_TEXTURE_FLOAT,
    SG_FEATURE_TEXTURE_HALF_FLOAT,
    SG_FEATURE_ORIGIN_BOTTOM_LEFT,
    SG_FEATURE_ORIGIN_TOP_LEFT,
    SG_FEATURE_MSAA_RENDER_TARGETS,
    SG_FEATURE_PACKED_VERTEX_FORMAT_10_2,
    SG_FEATURE_MULTIPLE_RENDER_TARGET,
    SG_FEATURE_IMAGETYPE_3D,
    SG_FEATURE_IMAGETYPE_ARRAY,

    SG_NUM_FEATURES
} sg_feature;

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
typedef enum {
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
                            new data, instead the data content of the
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

    Resource content is updated with the function sg_update_buffer() for
    buffer objects, and sg_update_image() for image objects. Only
    one update is allowed per frame and resource object. The
    application must update all data required for rendering (this
    means that the update data can be smaller than the resource size,
    if only a part of the overall resource size is used for rendering,
    you only need to make sure that the data that *is* used is valid.

    The default usage is SG_USAGE_IMMUTABLE.
*/
typedef enum {
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
typedef enum {
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
typedef enum {
    _SG_INDEXTYPE_DEFAULT,   /* value 0 reserved for default-init */
    SG_INDEXTYPE_NONE,
    SG_INDEXTYPE_UINT16,
    SG_INDEXTYPE_UINT32,
    _SG_INDEXTYPE_NUM,
    _SG_INDEXTYPE_FORCE_U32 = 0x7FFFFFFF
} sg_index_type;

/*
    sg_image_type

    Indicates the basic image type (2D-texture, cubemap, 3D-texture
    or 2D-array-texture). 3D- and array-textures are not supported
    on the GLES2/WebGL backend. The image type is used in the
    sg_image_desc.type member when creating an image.

    The default image type when creating an image is SG_IMAGETYPE_2D.
*/
typedef enum {
    _SG_IMAGETYPE_DEFAULT,  /* value 0 reserved for default-init */
    SG_IMAGETYPE_2D,
    SG_IMAGETYPE_CUBE,
    SG_IMAGETYPE_3D,
    SG_IMAGETYPE_ARRAY,
    _SG_IMAGETYPE_NUM,
    _SG_IMAGETYPE_FORCE_U32 = 0x7FFFFFFF
} sg_image_type;

/*
    sg_cube_face

    The cubemap faces. Use these as indices in the sg_image_desc.content
    array.
*/
typedef enum {
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
typedef enum {
    SG_SHADERSTAGE_VS,
    SG_SHADERSTAGE_FS,
    _SG_SHADERSTAGE_FORCE_U32 = 0x7FFFFFFF
} sg_shader_stage;

/*
    sg_pixel_format

    This is a common subset of useful and widely supported pixel formats. The
    pixel format enum is mainly used when creating an image object in the
    sg_image_desc.pixel_format member.

    The default pixel format when creating an image is SG_PIXELFORMAT_RGBA8.
*/
typedef enum {
    _SG_PIXELFORMAT_DEFAULT,    /* value 0 reserved for default-init */
    SG_PIXELFORMAT_NONE,
    SG_PIXELFORMAT_RGBA8,
    SG_PIXELFORMAT_RGB8,
    SG_PIXELFORMAT_RGBA4,
    SG_PIXELFORMAT_R5G6B5,
    SG_PIXELFORMAT_R5G5B5A1,
    SG_PIXELFORMAT_R10G10B10A2,
    SG_PIXELFORMAT_RGBA32F,
    SG_PIXELFORMAT_RGBA16F,
    SG_PIXELFORMAT_R32F,
    SG_PIXELFORMAT_R16F,
    SG_PIXELFORMAT_L8,
    SG_PIXELFORMAT_DXT1,
    SG_PIXELFORMAT_DXT3,
    SG_PIXELFORMAT_DXT5,
    SG_PIXELFORMAT_DEPTH,
    SG_PIXELFORMAT_DEPTHSTENCIL,
    SG_PIXELFORMAT_PVRTC2_RGB,
    SG_PIXELFORMAT_PVRTC4_RGB,
    SG_PIXELFORMAT_PVRTC2_RGBA,
    SG_PIXELFORMAT_PVRTC4_RGBA,
    SG_PIXELFORMAT_ETC2_RGB8,
    SG_PIXELFORMAT_ETC2_SRGB8,
    _SG_PIXELFORMAT_NUM,
    _SG_PIXELFORMAT_FORCE_U32 = 0x7FFFFFFF
} sg_pixel_format;

/*
    sg_primitive_type

    This is the common subset of 3D primitive types supported across all 3D
    APIs. This is used in the sg_pipeline_desc.primitive_type member when
    creating a pipeline object.

    The default primitive type is SG_PRIMITIVETYPE_TRIANGLES.
*/
typedef enum {
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
typedef enum {
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
*/
typedef enum {
    _SG_WRAP_DEFAULT,   /* value 0 reserved for default-init */
    SG_WRAP_REPEAT,
    SG_WRAP_CLAMP_TO_EDGE,
    SG_WRAP_MIRRORED_REPEAT,
    _SG_WRAP_NUM,
    _SG_WRAP_FORCE_U32 = 0x7FFFFFFF
} sg_wrap;

/*
    sg_vertex_format

    The data type of a vertex component. This is used to describe
    the layout of vertex data when creating a pipeline object.
*/
typedef enum {
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
    SG_VERTEXFORMAT_SHORT4,
    SG_VERTEXFORMAT_SHORT4N,
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
typedef enum {
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
typedef enum {
    SG_UNIFORMTYPE_INVALID,
    SG_UNIFORMTYPE_FLOAT,
    SG_UNIFORMTYPE_FLOAT2,
    SG_UNIFORMTYPE_FLOAT3,
    SG_UNIFORMTYPE_FLOAT4,
    SG_UNIFORMTYPE_MAT4,
    _SG_UNIFORMTYPE_NUM,
    _SG_UNIFORMTYPE_FORCE_U32 = 0x7FFFFFFF
} sg_uniform_type;

/*
    sg_cull_mode

    The face-culling mode, this is used in the
    sg_pipeline_desc.rasterizer.cull_mode member when creating a 
    pipeline object.

    The default cull mode is SG_CULLMODE_NONE
*/
typedef enum {
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
    is used in the member sg_pipeline_desc.rasterizer.face_winding
    when creating a pipeline object.

    The default winding is SG_FACEWINDING_CW (clockwise)
*/
typedef enum {
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
        .depth_stencil
            .depth_compare_func
            .stencil_front.compare_func
            .stencil_back.compare_func

    The default compare func for depth- and stencil-tests is
    SG_COMPAREFUNC_ALWAYS.
*/
typedef enum {
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
        .depth_stencil
            .stencil_front
                .fail_op
                .depth_fail_op
                .pass_op
            .stencil_back
                .fail_op
                .depth_fail_op
                .pass_op

    The default value is SG_STENCILOP_KEEP.
*/
typedef enum {
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
        .blend
            .src_factor_rgb
            .dst_factor_rgb
            .src_factor_alpha
            .dst_factor_alpha

    The default value is SG_BLENDFACTOR_ONE for source
    factors, and SG_BLENDFACTOR_ZERO for destination factors.
*/
typedef enum {
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
        .blend
            .op_rgb
            .op_alpha

    The default value is SG_BLENDOP_ADD.
*/
typedef enum {
    _SG_BLENDOP_DEFAULT,    /* value 0 reserved for default-init */
    SG_BLENDOP_ADD,
    SG_BLENDOP_SUBTRACT,
    SG_BLENDOP_REVERSE_SUBTRACT,
    _SG_BLENDOP_NUM,
    _SG_BLENDOP_FORCE_U32 = 0x7FFFFFFF
} sg_blend_op;

/*
    sg_color_mask

    Selects the color channels when writing a fragment color to the
    framebuffer. This is used in the members
    sg_pipeline_desc.blend.color_write_mask when creating a pipeline object.

    The default colormask is SG_COLORMASK_RGBA (write all colors channels)
*/
typedef enum {
    _SG_COLORMASK_DEFAULT = 0,      /* value 0 reserved for default-init */
    SG_COLORMASK_NONE = (0x10),     /* special value for 'all channels disabled */
    SG_COLORMASK_R = (1<<0),
    SG_COLORMASK_G = (1<<1),
    SG_COLORMASK_B = (1<<2),
    SG_COLORMASK_A = (1<<3),
    SG_COLORMASK_RGB = 0x7,
    SG_COLORMASK_RGBA = 0xF,
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
typedef enum {
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
typedef struct {
    sg_action action;
    float val[4];
} sg_color_attachment_action;

typedef struct {
    sg_action action;
    float val;
} sg_depth_attachment_action;

typedef struct {
    sg_action action;
    uint8_t val;
} sg_stencil_attachment_action;

typedef struct {
    uint32_t _start_canary;
    sg_color_attachment_action colors[SG_MAX_COLOR_ATTACHMENTS];
    sg_depth_attachment_action depth;
    sg_stencil_attachment_action stencil;
    uint32_t _end_canary;
} sg_pass_action;

/*
    sg_draw_state

    The sg_draw_state structure defines the resource binding slots
    of the sokol_gfx render pipeline, used as argument to the
    sg_apply_draw_state() function.

    A draw state contains:

    - 1 pipeline object
    - 1..N vertex buffers
    - 0..N vertex buffer offsets
    - 0..1 index buffers
    - 0..1 index buffer offsets
    - 0..N vertex shader stage images
    - 0..N fragment shader stage images

    The max number of vertex buffer and shader stage images
    are defined by the SG_MAX_SHADERSTAGE_BUFFERS and
    SG_MAX_SHADERSTAGE_IMAGES configuration constants.

    The optional buffer offsets can be used to group different chunks
    of vertex- and/or index-data into the same buffer objects.
*/
typedef struct {
    uint32_t _start_canary;
    sg_pipeline pipeline;
    sg_buffer vertex_buffers[SG_MAX_SHADERSTAGE_BUFFERS];
    uint32_t vertex_buffer_offsets[SG_MAX_SHADERSTAGE_BUFFERS];
    sg_buffer index_buffer;
    uint32_t index_buffer_offset;
    sg_image vs_images[SG_MAX_SHADERSTAGE_IMAGES];
    sg_image fs_images[SG_MAX_SHADERSTAGE_IMAGES];
    uint32_t _end_canary;
} sg_draw_state;

/*
    sg_desc

    The sg_desc struct contains configuration values for sokol_gfx,
    it is used as parameter to the sg_setup() call.

    The default configuration is:

    .buffer_pool_size:      128
    .image_pool_size:       128
    .shader_pool_size:      32
    .pipeline_pool_size:    64
    .pass_pool_size:        16 
    .context_pool_size:     16
    
    GL specific:
    .gl_force_gles2
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

    .mtl_device     
        a pointer to the MTLDevice object
    .mtl_renderpass_descriptor_cb
        a C callback function to obtain the MTLRenderPassDescriptor for the 
        current frame when rendering to the default framebuffer, will be called
        in sg_begin_default_pass()
    .mtl_drawable_cb
        a C callback function to obtain a MTLDrawable for the current
        frame when rendering to the default framebuffer, will be called in
        sg_end_pass() of the default pass
    .mtl_global_uniform_buffer_size
        the size of the global uniform buffer in bytes, this must be big
        enough to hold all uniform block updates for a single frame,
        the default value is 4 MByte (4 * 1024 * 1024)
    .mtl_sampler_cache_size
        the number of slots in the sampler cache, the Metal backend
        will share texture samplers with the same state in this 
        cache, the default value is 64

    D3D11 specific:
    .d3d11_device
        a pointer to the ID3D11Device object, this must have been created 
        before sg_setup() is called
    .d3d11_device_context
        a pointer to the ID3D11DeviceContext object
    .d3d11_render_target_view_cb
        a C callback function to obtain a pointer to the current
        ID3D11RenderTargetView object of the default framebuffer,
        this function will be called in sg_begin_pass() when rendering
        to the default framebuffer
    .d3d11_depth_stencil_view_cb
        a C callback function to obtain a pointer to the current
        ID3D11DepthStencilView object of the default framebuffer,
        this function will be called in sg_begin_pass() when rendering
        to the default framebuffer
*/
typedef struct {
    uint32_t _start_canary;
    int buffer_pool_size;
    int image_pool_size;
    int shader_pool_size;
    int pipeline_pool_size;
    int pass_pool_size;
    int context_pool_size;
    /* GL specific */
    bool gl_force_gles2;
    /* Metal-specific */
    const void* mtl_device;
    const void* (*mtl_renderpass_descriptor_cb)(void);
    const void* (*mtl_drawable_cb)(void);
    int mtl_global_uniform_buffer_size;
    int mtl_sampler_cache_size;
    /* D3D11-specific */
    const void* d3d11_device;
    const void* d3d11_device_context;
    const void* (*d3d11_render_target_view_cb)(void);
    const void* (*d3d11_depth_stencil_view_cb)(void);
    uint32_t _end_canary;
} sg_desc;

/*
    sg_buffer_desc

    Creation parameters for sg_buffer objects, used in the
    sg_make_buffer() call.

    The default configuration is:

    .size:      0       (this *must* be set to a valid size in bytes)
    .type:      SG_BUFFERTYPE_VERTEXBUFFER
    .usage:     SG_USAGE_IMMUTABLE
    .content    0

    Buffers with the SG_USAGE_IMMUTABLE usage *must* fill the buffer
    with initial data (.content must point to a data chunk with
    exactly .size bytes).

    ADVANCED TOPIC: Injecting native 3D-API buffers:

    The following struct members allow to inject your own GL, Metal
    or D3D11 buffers into sokol_gfx:
    
    .gl_buffers[SG_NUM_INFLIGHT_FRAMES]
    .mtl_buffers[SG_NUM_INFLIGHT_FRAMES]
    .d3d11_buffer

    You must still provide all other members except the .content member, and
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
typedef struct {
    uint32_t _start_canary;
    int size;
    sg_buffer_type type;
    sg_usage usage;
    const void* content;
    /* GL specific */
    uint32_t gl_buffers[SG_NUM_INFLIGHT_FRAMES];
    /* Metal specific */
    const void* mtl_buffers[SG_NUM_INFLIGHT_FRAMES];
    /* D3D11 specific */
    const void* d3d11_buffer;
    uint32_t _end_canary;
} sg_buffer_desc;

/*
    sg_subimage_content

    Pointer to and size of a subimage-surface data, this is
    used to describe the initial content of immutable-usage images,
    or for updating a dynamic- or stream-usage images.

    For 3D- or array-textures, one sg_subimage_content item
    describes an entire mipmap level consisting of all array- or
    3D-slices of the mipmap level. It is only possible to update
    an entire mipmap level, not parts of it.
*/
typedef struct {
    const void* ptr;    /* pointer to subimage data */
    int size;           /* size in bytes of pointed-to subimage data */
} sg_subimage_content;

/*
    sg_image_content

    Defines the content of an image through a 2D array
    of sg_subimage_content structs. The first array dimension
    is the cubemap face, and the second array dimension the
    mipmap level.
*/
typedef struct {
    sg_subimage_content subimage[SG_CUBEFACE_NUM][SG_MAX_MIPMAPS];
} sg_image_content;

/*
    sg_image_desc

    Creation parameters for sg_image objects, used in the 
    sg_make_image() call.

    The default configuration is:

    .type:              SG_IMAGETYPE_2D
    .render_target:     false
    .width              0 (must be set to >0)
    .height             0 (must be set to >0)
    .depth/.layers:     1
    .num_mipmaps:       1
    .usage:             SG_USAGE_IMMUTABLE
    .pixel_format:      SG_PIXELFORMAT_RGBA8
    .sample_count:      1 (only used in render_targets)
    .min_filter:        SG_FILTER_NEAREST
    .mag_filter:        SG_FILTER_NEAREST
    .wrap_u:            SG_WRAP_REPEAT
    .wrap_v:            SG_WRAP_REPEAT
    .wrap_w:            SG_WRAP_REPEAT (only SG_IMAGETYPE_3D)
    .max_anisotropy     1 (must be 1..16)
    .min_lod            0.0f
    .max_lod            FLT_MAX
    .content            an sg_image_content struct to define the initial content 

    SG_IMAGETYPE_ARRAY and SG_IMAGETYPE_3D are not supported on
    WebGL/GLES2, use sg_query_feature(SG_FEATURE_IMAGETYPE_ARRAY) and
    sg_query_feature(SG_FEATURE_IMAGETYPE_3D) at runtime to check
    if array- and 3D-textures are supported.

    Images with usage SG_USAGE_IMMUTABLE must be fully initialized by
    providing a valid .content member which points to
    initialization data.
    
    ADVANCED TOPIC: Injecting native 3D-API textures:

    The following struct members allow to inject your own GL, Metal
    or D3D11 textures into sokol_gfx:
    
    .gl_textures[SG_NUM_INFLIGHT_FRAMES]
    .mtl_textures[SG_NUM_INFLIGHT_FRAMES]
    .d3d11_texture

    The same rules apply as for injecting native buffers
    (see sg_buffer_desc documentation for more details).
*/
typedef struct {
    uint32_t _start_canary;
    sg_image_type type;
    bool render_target;
    int width;
    int height;
    union {
        int depth;
        int layers;
    };
    int num_mipmaps;
    sg_usage usage;
    sg_pixel_format pixel_format;
    int sample_count;
    sg_filter min_filter;
    sg_filter mag_filter;
    sg_wrap wrap_u;
    sg_wrap wrap_v;
    sg_wrap wrap_w;
    uint32_t max_anisotropy;
    float min_lod;
    float max_lod;
    sg_image_content content;
    /* GL specific */
    uint32_t gl_textures[SG_NUM_INFLIGHT_FRAMES];
    /* Metal specific */
    const void* mtl_textures[SG_NUM_INFLIGHT_FRAMES];
    /* D3D11 specific */
    const void* d3d11_texture;
    uint32_t _end_canary;
} sg_image_desc;

/*
    sg_shader_desc

    The structure sg_shader_desc describes the shaders, uniform blocks
    and texture images on the vertex- and fragment-shader stage.

    TODO: source code vs byte code, 3D backend API specifics.
*/
typedef struct {
    const char* name;
    sg_uniform_type type;
    int array_count; 
} sg_shader_uniform_desc;

typedef struct {
    int size;
    sg_shader_uniform_desc uniforms[SG_MAX_UB_MEMBERS];
} sg_shader_uniform_block_desc;

typedef struct {
    const char* name;
    sg_image_type type;
} sg_shader_image_desc;

typedef struct {
    const char* source;
    const uint8_t* byte_code;
    int byte_code_size;
    const char* entry;
    sg_shader_uniform_block_desc uniform_blocks[SG_MAX_SHADERSTAGE_UBS];
    sg_shader_image_desc images[SG_MAX_SHADERSTAGE_IMAGES];
} sg_shader_stage_desc;

typedef struct {
    uint32_t _start_canary;
    sg_shader_stage_desc vs;
    sg_shader_stage_desc fs;
    uint32_t _end_canary;
} sg_shader_desc;

/*
    sg_pipeline_desc

    The sg_pipeline_desc struct defines all creation parameters
    for an sg_pipeline object, used as argument to the
    sg_make_pipeline() function:

    - the complete vertex layout for all input vertex buffers
    - a shader object
    - the 3D primitive type (points, lines, triangles, ...)
    - the index type (none, 16- or 32-bit)
    - depth-stencil state
    - alpha-blending state
    - rasterizer state

    If the vertex data has no gaps between vertex components, you can omit
    the .layout.buffers[].stride and layout.attrs[].offset items (leave them 
    default-initialized to 0), sokol will then compute the offsets and strides
    from the vertex component formats (.layout.attrs[].offset). Please note
    that ALL vertex attribute offsets must be 0 in order for the the
    automatic offset computation to kick in.

    The default configuration is as follows:

    .layout:
        .buffers[]:         vertex buffer layouts
            .stride:        0 (if no stride is given it will be computed)
            .step_func      SG_VERTEXSTEP_PER_VERTEX
            .step_rate      1
        .attrs[]:           vertex attribute declarations
            .buffer_index   0 the vertex buffer bind slot  
            .offset         0 (offsets can be omitted if the vertex layout has no gaps)
            .format         SG_VERTEXFORMAT_INVALID (must be initialized!)
            .name           0 (GLES2 requires an attribute name here)
            .sem_name       0 (D3D11 requires a semantic name here)
            .sem_index      0 (D3D11 requires a semantic index here)
    .shader:            0 (must be intilized with a valid sg_shader id!)
    .primitive_type:    SG_PRIMITIVETYPE_TRIANGLES
    .index_type:        SG_INDEXTYPE_NONE
    .depth_stencil:
        .stencil_front, .stencil_back:
            .fail_op:               SG_STENCILOP_KEEP
            .depth_fail_op:         SG_STENCILOP_KEEP
            .pass_op:               SG_STENCILOP_KEEP
            .compare_func           SG_COMPAREFUNC_ALWAYS
        .depth_compare_func:    SG_COMPAREFUNC_ALWAYS
        .depth_write_enabled:   false
        .stencil_enabled:       false
        .stencil_read_mask:     0
        .stencil_write_mask:    0
        .stencil_ref:           0
    .blend:
        .enabled:               false
        .src_factor_rgb:        SG_BLENDFACTOR_ONE
        .dst_factor_rgb:        SG_BLENDFACTOR_ZERO
        .op_rgb:                SG_BLENDOP_ADD
        .src_factor_alpha:      SG_BLENDFACTOR_ONE
        .dst_factor_alpha:      SG_BLENDFACTOR_ZERO
        .op_alpha:              SG_BLENDOP_ADD
        .color_write_mask:      SG_COLORMASK_RGBA
        .color_attachment_count 1
        .color_format           SG_PIXELFORMAT_RGBA8
        .depth_format           SG_PIXELFORMAT_DEPTHSTENCIL
        .blend_color:           { 0.0f, 0.0f, 0.0f, 0.0f }
    .rasterizer:
        .alpha_to_coverage_enabled:     false
        .cull_mode:                     SG_CULLMODE_NONE
        .face_winding:                  SG_FACEWINDING_CW
        .sample_count:                  1
        .depth_bias:                    0.0f
        .depth_bias_slope_scale:        0.0f
        .depth_bias_clamp:              0.0f
*/
typedef struct {
    int stride;
    sg_vertex_step step_func;
    int step_rate;
} sg_buffer_layout_desc;

typedef struct {
    const char* name;
    const char* sem_name;
    int sem_index;
    int buffer_index;
    int offset;
    sg_vertex_format format;
} sg_vertex_attr_desc;

typedef struct {
    sg_buffer_layout_desc buffers[SG_MAX_SHADERSTAGE_BUFFERS];
    sg_vertex_attr_desc attrs[SG_MAX_VERTEX_ATTRIBUTES];
} sg_layout_desc;

typedef struct {
    sg_stencil_op fail_op;
    sg_stencil_op depth_fail_op;
    sg_stencil_op pass_op;
    sg_compare_func compare_func;
} sg_stencil_state;

typedef struct {
    sg_stencil_state stencil_front;
    sg_stencil_state stencil_back;
    sg_compare_func depth_compare_func;
    bool depth_write_enabled;
    bool stencil_enabled;
    uint8_t stencil_read_mask;
    uint8_t stencil_write_mask;
    uint8_t stencil_ref;
} sg_depth_stencil_state;

typedef struct {
    bool enabled;
    sg_blend_factor src_factor_rgb;
    sg_blend_factor dst_factor_rgb;
    sg_blend_op op_rgb;
    sg_blend_factor src_factor_alpha;
    sg_blend_factor dst_factor_alpha;
    sg_blend_op op_alpha;
    uint8_t color_write_mask;
    int color_attachment_count;
    sg_pixel_format color_format;
    sg_pixel_format depth_format;
    float blend_color[4];
} sg_blend_state;

typedef struct {
    bool alpha_to_coverage_enabled;
    sg_cull_mode cull_mode;
    sg_face_winding face_winding;
    int sample_count;
    float depth_bias;
    float depth_bias_slope_scale;
    float depth_bias_clamp;
} sg_rasterizer_state;

typedef struct {
    uint32_t _start_canary;
    sg_layout_desc layout;
    sg_shader shader;
    sg_primitive_type primitive_type;
    sg_index_type index_type;
    sg_depth_stencil_state depth_stencil;
    sg_blend_state blend;
    sg_rasterizer_state rasterizer;
    uint32_t _end_canary;
} sg_pipeline_desc;

/*
    sg_pass_desc

    Creation parameters for an sg_pass object, used as argument
    to the sg_make_pass() function.

    A pass object contains 1..4 color-attachments and none, or one,
    depth-stencil-attachment. Each attachment consists of
    an image, and two additional indices describing
    which subimage the pass will render: one mipmap index, and
    if the image is a cubemap, array-texture or 3D-texture, the
    face-index, array-layer or depth-slice.

    Pass images must fulfill the following requirements:

    All images must have:
    - been created as render target (sg_image_desc.render_target = true)
    - the same size
    - the same sample count
    
    In addition, all color-attachment images must have the same
    pixel format.
*/
typedef struct {
    sg_image image;
    int mip_level;
    union {
        int face;
        int layer;
        int slice;
    };
} sg_attachment_desc;

typedef struct {
    uint32_t _start_canary;
    sg_attachment_desc color_attachments[SG_MAX_COLOR_ATTACHMENTS];
    sg_attachment_desc depth_stencil_attachment;
    uint32_t _end_canary;
} sg_pass_desc;

/* setup and misc functions */
extern void sg_setup(const sg_desc* desc);
extern void sg_shutdown(void);
extern bool sg_isvalid(void);
extern bool sg_query_feature(sg_feature feature);
extern void sg_reset_state_cache(void);

/* resource creation, destruction and updating */
extern sg_buffer sg_make_buffer(const sg_buffer_desc* desc);
extern sg_image sg_make_image(const sg_image_desc* desc);
extern sg_shader sg_make_shader(const sg_shader_desc* desc);
extern sg_pipeline sg_make_pipeline(const sg_pipeline_desc* desc);
extern sg_pass sg_make_pass(const sg_pass_desc* desc);
extern void sg_destroy_buffer(sg_buffer buf);
extern void sg_destroy_image(sg_image img);
extern void sg_destroy_shader(sg_shader shd);
extern void sg_destroy_pipeline(sg_pipeline pip);
extern void sg_destroy_pass(sg_pass pass);
extern void sg_update_buffer(sg_buffer buf, const void* data_ptr, int data_size);
extern void sg_update_image(sg_image img, const sg_image_content* data); 

/* get resource state (initial, alloc, valid, failed) */
extern sg_resource_state sg_query_buffer_state(sg_buffer buf);
extern sg_resource_state sg_query_image_state(sg_image img);
extern sg_resource_state sg_query_shader_state(sg_shader shd);
extern sg_resource_state sg_query_pipeline_state(sg_pipeline pip);
extern sg_resource_state sg_query_pass_state(sg_pass pass);

/* rendering functions */
extern void sg_begin_default_pass(const sg_pass_action* pass_action, int width, int height);
extern void sg_begin_pass(sg_pass pass, const sg_pass_action* pass_action);
extern void sg_apply_viewport(int x, int y, int width, int height, bool origin_top_left);
extern void sg_apply_scissor_rect(int x, int y, int width, int height, bool origin_top_left);
extern void sg_apply_draw_state(const sg_draw_state* ds);
extern void sg_apply_uniform_block(sg_shader_stage stage, int ub_index, const void* data, int num_bytes);
extern void sg_draw(int base_element, int num_elements, int num_instances);
extern void sg_end_pass(void);
extern void sg_commit(void);

/* separate resource allocation and initialization (for async setup) */ 
extern sg_buffer sg_alloc_buffer(void);
extern sg_image sg_alloc_image(void);
extern sg_shader sg_alloc_shader(void);
extern sg_pipeline sg_alloc_pipeline(void);
extern sg_pass sg_alloc_pass(void);
extern void sg_init_buffer(sg_buffer buf_id, const sg_buffer_desc* desc);
extern void sg_init_image(sg_image img_id, const sg_image_desc* desc);
extern void sg_init_shader(sg_shader shd_id, const sg_shader_desc* desc);
extern void sg_init_pipeline(sg_pipeline pip_id, const sg_pipeline_desc* desc);
extern void sg_init_pass(sg_pass pass_id, const sg_pass_desc* desc);
extern void sg_fail_buffer(sg_buffer buf_id);
extern void sg_fail_image(sg_image img_id);
extern void sg_fail_shader(sg_shader shd_id);
extern void sg_fail_pipeline(sg_pipeline pip_id);
extern void sg_fail_pass(sg_pass pass_id);

/* rendering contexts (optional) */
extern sg_context sg_setup_context(void);
extern void sg_activate_context(sg_context ctx_id);
extern void sg_discard_context(sg_context ctx_id);

#ifdef __cplusplus
} /* extern "C" */
#endif

/*--- IMPLEMENTATION ---------------------------------------------------------*/
#ifdef SOKOL_IMPL

#ifndef SOKOL_DEBUG
    #ifdef _DEBUG
        #define SOKOL_DEBUG (1)
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
    #define SOKOL_VALIDATE(cond, err) _sg_validate(cond, err)
#endif
#ifndef SOKOL_VALIDATE_END
    #define SOKOL_VALIDATE_END() _sg_validate_end()
#endif
#ifndef SOKOL_UNREACHABLE
    #define SOKOL_UNREACHABLE SOKOL_ASSERT(false)
#endif
#ifndef SOKOL_MALLOC
    #include <stdlib.h>
    #define SOKOL_MALLOC(s) malloc(s)
    #define SOKOL_FREE(p) free(p)
#endif
#ifndef SOKOL_LOG
    #ifdef SOKOL_DEBUG 
        #include <stdio.h>
        #define SOKOL_LOG(s) { SOKOL_ASSERT(s); puts(s); }
    #else
        #define SOKOL_LOG(s)
    #endif
#endif
#if !(defined(SOKOL_GLCORE33)||defined(SOKOL_GLES2)||defined(SOKOL_GLES3)||defined(SOKOL_D3D11)||defined(SOKOL_METAL))
#error "Please select a backend with SOKOL_GLCORE33, SOKOL_GLES2, SOKOL_GLES3, SOKOL_D3D11 or SOKOL_METAL"
#endif

#ifndef _SOKOL_PRIVATE
    #if defined(__GNUC__)
        #define _SOKOL_PRIVATE __attribute__((unused)) static
    #else
        #define _SOKOL_PRIVATE static
    #endif
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

#ifdef __cplusplus
extern "C" {
#endif

enum {
    _SG_SLOT_SHIFT = 16,
    _SG_SLOT_MASK = (1<<_SG_SLOT_SHIFT)-1,
    _SG_MAX_POOL_SIZE = (1<<_SG_SLOT_SHIFT),
    _SG_DEFAULT_BUFFER_POOL_SIZE = 128,
    _SG_DEFAULT_IMAGE_POOL_SIZE = 128,
    _SG_DEFAULT_SHADER_POOL_SIZE = 32,
    _SG_DEFAULT_PIPELINE_POOL_SIZE = 64,
    _SG_DEFAULT_PASS_POOL_SIZE = 16,
    _SG_DEFAULT_CONTEXT_POOL_SIZE = 16
};

/* helper macros */
#define _sg_def(val, def) (((val) == 0) ? (def) : (val))
#define _sg_def_flt(val, def) (((val) == 0.0f) ? (def) : (val))
#define _sg_min(a,b) ((a<b)?a:b)
#define _sg_max(a,b) ((a>b)?a:b)
#define _sg_clamp(v,v0,v1) ((v<v0)?(v0):((v>v1)?(v1):(v)))
#define _sg_fequal(val,cmp,delta) (((val-cmp)> -delta)&&((val-cmp)<delta))


/*-- helper functions --------------------------------------------------------*/

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
        case SG_VERTEXFORMAT_SHORT4:    return 8;
        case SG_VERTEXFORMAT_SHORT4N:   return 8;
        case SG_VERTEXFORMAT_UINT10_N2: return 4;
        case SG_VERTEXFORMAT_INVALID:   return 0;
        default:
            SOKOL_UNREACHABLE;
            return -1;
    }
}

/* return the byte size of a shader uniform */
_SOKOL_PRIVATE int _sg_uniform_size(sg_uniform_type type, int count) {
    switch (type) {
        case SG_UNIFORMTYPE_INVALID:    return 0;
        case SG_UNIFORMTYPE_FLOAT:      return 4 * count;
        case SG_UNIFORMTYPE_FLOAT2:     return 8 * count;
        case SG_UNIFORMTYPE_FLOAT3:     return 12 * count; /* FIXME: std140??? */
        case SG_UNIFORMTYPE_FLOAT4:     return 16 * count;
        case SG_UNIFORMTYPE_MAT4:       return 64 * count;
        default:
            SOKOL_UNREACHABLE;
            return -1;
    }
}

/* return true if pixel format is a compressed format */
_SOKOL_PRIVATE bool _sg_is_compressed_pixel_format(sg_pixel_format fmt) {
    switch (fmt) {
        case SG_PIXELFORMAT_DXT1:
        case SG_PIXELFORMAT_DXT3:
        case SG_PIXELFORMAT_DXT5:
        case SG_PIXELFORMAT_PVRTC2_RGB:
        case SG_PIXELFORMAT_PVRTC4_RGB:
        case SG_PIXELFORMAT_PVRTC2_RGBA:
        case SG_PIXELFORMAT_PVRTC4_RGBA:
        case SG_PIXELFORMAT_ETC2_RGB8:
        case SG_PIXELFORMAT_ETC2_SRGB8:
            return true;
        default:
            return false;
    }
}

/* return true if pixel format is a valid render target format */
_SOKOL_PRIVATE bool _sg_is_valid_rendertarget_color_format(sg_pixel_format fmt) {
    switch (fmt) {
        case SG_PIXELFORMAT_RGBA8:
        case SG_PIXELFORMAT_R10G10B10A2:
        case SG_PIXELFORMAT_RGBA32F:
        case SG_PIXELFORMAT_RGBA16F:
            return true;
        default:
            return false;
    }
}

/* return true if pixel format is a valid depth format */
_SOKOL_PRIVATE bool _sg_is_valid_rendertarget_depth_format(sg_pixel_format fmt) {
    switch (fmt) {
        case SG_PIXELFORMAT_DEPTH:
        case SG_PIXELFORMAT_DEPTHSTENCIL:
            return true;
        default:
            return false;
    }
}

/* return true if pixel format is a depth-stencil format */
_SOKOL_PRIVATE bool _sg_is_depth_stencil_format(sg_pixel_format fmt) {
    /* FIXME: more depth stencil formats? */
    return (SG_PIXELFORMAT_DEPTHSTENCIL == fmt);
}

/* return the bytes-per-pixel for a pixel format */
_SOKOL_PRIVATE int _sg_pixelformat_bytesize(sg_pixel_format fmt) {
    switch (fmt) {
        case SG_PIXELFORMAT_RGBA32F:
            return 16;
        case SG_PIXELFORMAT_RGBA16F:
            return 8;
        case SG_PIXELFORMAT_RGBA8:
        case SG_PIXELFORMAT_R10G10B10A2:
        case SG_PIXELFORMAT_R32F:
            return 4;
        case SG_PIXELFORMAT_RGB8:
            return 3;
        case SG_PIXELFORMAT_R5G5B5A1:
        case SG_PIXELFORMAT_R5G6B5:
        case SG_PIXELFORMAT_RGBA4:
        case SG_PIXELFORMAT_R16F:
            return 2;
        case SG_PIXELFORMAT_L8:
            return 1;
        default:
            SOKOL_UNREACHABLE;
            return 0;
    }
}

/* return row pitch for an image */
_SOKOL_PRIVATE int _sg_row_pitch(sg_pixel_format fmt, int width) {
    int pitch;
    switch (fmt) {
        case SG_PIXELFORMAT_DXT1:
        case SG_PIXELFORMAT_ETC2_RGB8:
        case SG_PIXELFORMAT_ETC2_SRGB8:
            pitch = ((width + 3) / 4) * 8;
            pitch = pitch < 8 ? 8 : pitch;
            break;
        case SG_PIXELFORMAT_DXT3:
        case SG_PIXELFORMAT_DXT5:
            pitch = ((width + 3) / 4) * 16;
            pitch = pitch < 16 ? 16 : pitch;
            break;
        case SG_PIXELFORMAT_PVRTC4_RGB:
        case SG_PIXELFORMAT_PVRTC4_RGBA:
            {
                const int block_size = 4*4;
                const int bpp = 4;
                int width_blocks = width / 4;
                width_blocks = width_blocks < 2 ? 2 : width_blocks;
                pitch = width_blocks * ((block_size * bpp) / 8);
            }
            break;
        case SG_PIXELFORMAT_PVRTC2_RGB:
        case SG_PIXELFORMAT_PVRTC2_RGBA:
            {
                const int block_size = 8*4;
                const int bpp = 2;
                int width_blocks = width / 4;
                width_blocks = width_blocks < 2 ? 2 : width_blocks;
                pitch = width_blocks * ((block_size * bpp) / 8);
            }
            break;
        default:
            pitch = width * _sg_pixelformat_bytesize(fmt);
            break;
    }
    return pitch;
}

/* return pitch of a 2D subimage / texture slice */
_SOKOL_PRIVATE int _sg_surface_pitch(sg_pixel_format fmt, int width, int height) {
    int num_rows = 0;
    switch (fmt) {
        case SG_PIXELFORMAT_DXT1:
        case SG_PIXELFORMAT_DXT3:
        case SG_PIXELFORMAT_DXT5:
        case SG_PIXELFORMAT_ETC2_RGB8:
        case SG_PIXELFORMAT_ETC2_SRGB8:
        case SG_PIXELFORMAT_PVRTC2_RGB:
        case SG_PIXELFORMAT_PVRTC2_RGBA:
        case SG_PIXELFORMAT_PVRTC4_RGB:
        case SG_PIXELFORMAT_PVRTC4_RGBA:
            num_rows = ((height + 3) / 4);
            break;
        default:
            num_rows = height;
            break;
    }
    if (num_rows < 1) {
        num_rows = 1;
    }
    return num_rows * _sg_row_pitch(fmt, width);
}

/* resolve pass action defaults into a new pass action struct */
_SOKOL_PRIVATE void _sg_resolve_default_pass_action(const sg_pass_action* from, sg_pass_action* to) {
    SOKOL_ASSERT(from && to);
    *to = *from;
    for (int i = 0; i < SG_MAX_COLOR_ATTACHMENTS; i++) {
        if (to->colors[i].action  == _SG_ACTION_DEFAULT) {
            to->colors[i].action = SG_ACTION_CLEAR;
            to->colors[i].val[0] = SG_DEFAULT_CLEAR_RED;
            to->colors[i].val[1] = SG_DEFAULT_CLEAR_GREEN;
            to->colors[i].val[2] = SG_DEFAULT_CLEAR_BLUE;
            to->colors[i].val[3] = SG_DEFAULT_CLEAR_ALPHA;
        }
    }
    if (to->depth.action == _SG_ACTION_DEFAULT) {
        to->depth.action = SG_ACTION_CLEAR;
        to->depth.val = SG_DEFAULT_CLEAR_DEPTH;
    }
    if (to->stencil.action == _SG_ACTION_DEFAULT) {
        to->stencil.action = SG_ACTION_CLEAR;
        to->stencil.val = SG_DEFAULT_CLEAR_STENCIL;
    }
}

/*-- resource pool slots (must be defined before rendering backend) ----------*/
typedef struct {
    uint32_t id;
    uint32_t ctx_id;
    sg_resource_state state;
} _sg_slot;

_SOKOL_PRIVATE int _sg_slot_index(uint32_t id) {
    return id & _SG_SLOT_MASK;
}

#ifdef __cplusplus
} /* extern "C" */
#endif

/*== GL BACKEND ==============================================================*/
#if defined(SOKOL_GLCORE33) || defined(SOKOL_GLES2) || defined(SOKOL_GLES3)
/* strstr(), memset() */
#include <string.h>

#ifdef __cplusplus
extern "C" {
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
#ifndef GL_COMPRESSED_SRGB8_ETC2
#define GL_COMPRESSED_SRGB8_ETC2 0x9275
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
#define glVertexAttribDivisor(index, divisor) glVertexAttribDivisorEXT(index, divisor)
#define glDrawArraysInstanced(mode, first, count, instancecount) glDrawArraysInstancedEXT(mode, first, count, instancecount)
#define glDrawElementsInstanced(mode, count, type, indices, instancecount) glDrawElementsInstancedEXT(mode, count, type, indices, instancecount)
#endif
#define _SG_GL_CHECK_ERROR() { SOKOL_ASSERT(glGetError() == GL_NO_ERROR); } 

/* true if runnin in GLES2-fallback mode */
static bool _sg_gl_gles2;

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
        case SG_VERTEXFORMAT_SHORT4:    return 4;
        case SG_VERTEXFORMAT_SHORT4N:   return 4;
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
        case SG_VERTEXFORMAT_SHORT4N:
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
        case SG_WRAP_REPEAT:            return GL_REPEAT;
        case SG_WRAP_MIRRORED_REPEAT:   return GL_MIRRORED_REPEAT;
        default: SOKOL_UNREACHABLE; return 0;
    }
}

_SOKOL_PRIVATE GLenum _sg_gl_teximage_type(sg_pixel_format fmt) {
    switch (fmt) {
        case SG_PIXELFORMAT_RGBA32F:
        case SG_PIXELFORMAT_R32F:
            return GL_FLOAT;
        case SG_PIXELFORMAT_RGBA16F:
        case SG_PIXELFORMAT_R16F:
            return GL_HALF_FLOAT;
        case SG_PIXELFORMAT_RGBA8:
        case SG_PIXELFORMAT_RGB8:
        case SG_PIXELFORMAT_L8:
            return GL_UNSIGNED_BYTE;
        case SG_PIXELFORMAT_R10G10B10A2:
            return GL_UNSIGNED_INT_2_10_10_10_REV;
        case SG_PIXELFORMAT_R5G5B5A1:
            return GL_UNSIGNED_SHORT_5_5_5_1;
        case SG_PIXELFORMAT_R5G6B5:
            return GL_UNSIGNED_SHORT_5_6_5;
        case SG_PIXELFORMAT_RGBA4:
            return GL_UNSIGNED_SHORT_4_4_4_4;
        case SG_PIXELFORMAT_DEPTH:
            /* FIXME */
            return GL_UNSIGNED_SHORT;
        case SG_PIXELFORMAT_DEPTHSTENCIL:
            /* FIXME */
            return GL_UNSIGNED_INT_24_8;
        default:
            SOKOL_UNREACHABLE; return 0;
    }
}

_SOKOL_PRIVATE GLenum _sg_gl_teximage_format(sg_pixel_format fmt) {
    switch (fmt) {
        case SG_PIXELFORMAT_NONE:
            return 0;
        case SG_PIXELFORMAT_RGBA8:
        case SG_PIXELFORMAT_R5G5B5A1:
        case SG_PIXELFORMAT_RGBA4:
        case SG_PIXELFORMAT_RGBA32F:
        case SG_PIXELFORMAT_RGBA16F:
        case SG_PIXELFORMAT_R10G10B10A2:
            return GL_RGBA;
        case SG_PIXELFORMAT_RGB8:
        case SG_PIXELFORMAT_R5G6B5:
            return GL_RGB;
        case SG_PIXELFORMAT_L8:
        case SG_PIXELFORMAT_R32F:
        case SG_PIXELFORMAT_R16F:
            #if defined(SOKOL_GLES2)
            return GL_LUMINANCE;
            #else
            if (_sg_gl_gles2) {
                return GL_LUMINANCE;
            }
            else {
                return GL_RED;
            }
            #endif
        case SG_PIXELFORMAT_DEPTH:
            return GL_DEPTH_COMPONENT;
        case SG_PIXELFORMAT_DEPTHSTENCIL:
            return GL_DEPTH_STENCIL;
        case SG_PIXELFORMAT_DXT1:
            return GL_COMPRESSED_RGBA_S3TC_DXT1_EXT;
        case SG_PIXELFORMAT_DXT3:
            return GL_COMPRESSED_RGBA_S3TC_DXT3_EXT;
        case SG_PIXELFORMAT_DXT5:
            return GL_COMPRESSED_RGBA_S3TC_DXT5_EXT;
        case SG_PIXELFORMAT_PVRTC2_RGB:
            return GL_COMPRESSED_RGB_PVRTC_2BPPV1_IMG;
        case SG_PIXELFORMAT_PVRTC4_RGB:
            return GL_COMPRESSED_RGB_PVRTC_4BPPV1_IMG; 
        case SG_PIXELFORMAT_PVRTC2_RGBA:
            return GL_COMPRESSED_RGBA_PVRTC_2BPPV1_IMG;
        case SG_PIXELFORMAT_PVRTC4_RGBA:
            return GL_COMPRESSED_RGBA_PVRTC_4BPPV1_IMG;
        case SG_PIXELFORMAT_ETC2_RGB8:
            return GL_COMPRESSED_RGB8_ETC2;
        case SG_PIXELFORMAT_ETC2_SRGB8:
            return GL_COMPRESSED_SRGB8_ETC2;
        default:
            SOKOL_UNREACHABLE; return 0;
    }
}

_SOKOL_PRIVATE GLenum _sg_gl_teximage_internal_format(sg_pixel_format fmt) {
    #if defined(SOKOL_GLES2)
    return _sg_gl_teximage_format(fmt);
    #else
    if (_sg_gl_gles2) {
        return _sg_gl_teximage_format(fmt);
    }
    else {
        switch (fmt) {
            case SG_PIXELFORMAT_NONE:
                return 0;
            case SG_PIXELFORMAT_RGBA8:
                return GL_RGBA8;
            case SG_PIXELFORMAT_RGB8:
                return GL_RGB8;
            case SG_PIXELFORMAT_RGBA4:
                return GL_RGBA4;
            case SG_PIXELFORMAT_R5G6B5:
                #if defined(SOKOL_GLES3)
                    return GL_RGB565;
                #else
                    return GL_RGB5;
                #endif
            case SG_PIXELFORMAT_R5G5B5A1:
                return GL_RGB5_A1;
            case SG_PIXELFORMAT_R10G10B10A2:
                return GL_RGB10_A2;
            case SG_PIXELFORMAT_RGBA32F:
                return GL_RGBA32F;
            case SG_PIXELFORMAT_RGBA16F:
                return GL_RGBA16F;
            case SG_PIXELFORMAT_R32F:
                return GL_R32F;
            case SG_PIXELFORMAT_R16F:
                return GL_R16F;
            case SG_PIXELFORMAT_L8:
                return GL_R8;
            case SG_PIXELFORMAT_DEPTH:
                /* FIXME */
                return GL_DEPTH_COMPONENT16;
            case SG_PIXELFORMAT_DEPTHSTENCIL:
                return GL_DEPTH24_STENCIL8;
            case SG_PIXELFORMAT_DXT1:
                return GL_COMPRESSED_RGBA_S3TC_DXT1_EXT;
            case SG_PIXELFORMAT_DXT3:
                return GL_COMPRESSED_RGBA_S3TC_DXT3_EXT;
            case SG_PIXELFORMAT_DXT5:
                return GL_COMPRESSED_RGBA_S3TC_DXT5_EXT;
            case SG_PIXELFORMAT_PVRTC2_RGB:
                return GL_COMPRESSED_RGB_PVRTC_2BPPV1_IMG;
            case SG_PIXELFORMAT_PVRTC4_RGB:
                return GL_COMPRESSED_RGB_PVRTC_4BPPV1_IMG; 
            case SG_PIXELFORMAT_PVRTC2_RGBA:
                return GL_COMPRESSED_RGBA_PVRTC_2BPPV1_IMG;
            case SG_PIXELFORMAT_PVRTC4_RGBA:
                return GL_COMPRESSED_RGBA_PVRTC_4BPPV1_IMG;
            case SG_PIXELFORMAT_ETC2_RGB8:
                return GL_COMPRESSED_RGB8_ETC2;
            case SG_PIXELFORMAT_ETC2_SRGB8:
                return GL_COMPRESSED_SRGB8_ETC2;
            default:
                SOKOL_UNREACHABLE; return 0;
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
        case SG_PIXELFORMAT_DEPTHSTENCIL:   return GL_DEPTH24_STENCIL8;
        default: SOKOL_UNREACHABLE; return 0;
    }
}

/*-- GL backend resource declarations ----------------------------------------*/
typedef struct {
    _sg_slot slot;
    int size;
    sg_buffer_type type;
    sg_usage usage;
    uint32_t upd_frame_index;
    int num_slots;
    int active_slot;
    GLuint gl_buf[SG_NUM_INFLIGHT_FRAMES];
    bool ext_buffers;   /* if true, external buffers were injected with sg_buffer_desc.gl_buffers */
} _sg_buffer;

_SOKOL_PRIVATE void _sg_init_buffer_slot(_sg_buffer* buf) {
    SOKOL_ASSERT(buf);
    memset(buf, 0, sizeof(_sg_buffer));
}

typedef struct {
    _sg_slot slot;
    sg_image_type type;
    bool render_target;
    int width;
    int height;
    int depth;
    int num_mipmaps;
    sg_usage usage;
    sg_pixel_format pixel_format;
    int sample_count;
    sg_filter min_filter;
    sg_filter mag_filter;
    sg_wrap wrap_u;
    sg_wrap wrap_v;
    sg_wrap wrap_w;
    uint32_t max_anisotropy;
    GLenum gl_target;
    GLuint gl_depth_render_buffer;
    GLuint gl_msaa_render_buffer;
    uint32_t upd_frame_index;
    int num_slots;
    int active_slot;
    GLuint gl_tex[SG_NUM_INFLIGHT_FRAMES];
    bool ext_textures;  /* if true, external textures were injected with sg_image_desc.gl_textures */
} _sg_image;

_SOKOL_PRIVATE void _sg_init_image_slot(_sg_image* img) {
    SOKOL_ASSERT(img);
    memset(img, 0, sizeof(_sg_image));
}

typedef struct {
    GLint gl_loc;
    sg_uniform_type type;
    uint8_t count;
    uint16_t offset;
} _sg_uniform;

typedef struct {
    int size;
    int num_uniforms;
    _sg_uniform uniforms[SG_MAX_UB_MEMBERS];
} _sg_uniform_block;

typedef struct {
    sg_image_type type;
    GLint gl_loc;
    int gl_tex_slot;
} _sg_shader_image;

typedef struct {
    int num_uniform_blocks;
    int num_images;
    _sg_uniform_block uniform_blocks[SG_MAX_SHADERSTAGE_UBS];
    _sg_shader_image images[SG_MAX_SHADERSTAGE_IMAGES];
} _sg_shader_stage;

typedef struct {
    _sg_slot slot;
    GLuint gl_prog;
    _sg_shader_stage stage[SG_NUM_SHADER_STAGES];
} _sg_shader;

_SOKOL_PRIVATE void _sg_init_shader_slot(_sg_shader* shd) {
    SOKOL_ASSERT(shd);
    memset(shd, 0, sizeof(_sg_shader));
}

typedef struct {
    int8_t vb_index;        /* -1 if attr is not enabled */
    int8_t divisor;         /* -1 if not initialized */
    uint8_t stride;
    uint8_t size;
    uint8_t normalized;
    uint32_t offset;
    GLenum type;
} _sg_gl_attr;

_SOKOL_PRIVATE void _sg_gl_init_attr(_sg_gl_attr* attr) {
    attr->vb_index = -1;
    attr->divisor = -1;
    attr->stride = 0;
    attr->size = 0;
    attr->normalized = 0;
    attr->offset = 0;
    attr->type = 0;
}

typedef struct {
    _sg_slot slot;
    _sg_shader* shader;
    sg_shader shader_id;
    sg_primitive_type primitive_type;
    sg_index_type index_type;
    bool vertex_layout_valid[SG_MAX_SHADERSTAGE_BUFFERS];
    int color_attachment_count;
    sg_pixel_format color_format;
    sg_pixel_format depth_format;
    int sample_count;
    _sg_gl_attr gl_attrs[SG_MAX_VERTEX_ATTRIBUTES];
    sg_depth_stencil_state depth_stencil;
    sg_blend_state blend;
    sg_rasterizer_state rast;
} _sg_pipeline;

_SOKOL_PRIVATE void _sg_init_pipeline_slot(_sg_pipeline* pip) {
    SOKOL_ASSERT(pip);
    memset(pip, 0, sizeof(_sg_pipeline));
}

typedef struct {
    _sg_image* image;
    sg_image image_id;
    int mip_level;
    int slice;
    GLuint gl_msaa_resolve_buffer;
} _sg_attachment;

typedef struct {
    _sg_slot slot;
    GLuint gl_fb;
    int num_color_atts;
    _sg_attachment color_atts[SG_MAX_COLOR_ATTACHMENTS];
    _sg_attachment ds_att;
} _sg_pass;

_SOKOL_PRIVATE void _sg_init_pass_slot(_sg_pass* pass) {
    SOKOL_ASSERT(pass);
    memset(pass, 0, sizeof(_sg_pass));
}

typedef struct {
    _sg_slot slot;
    #if !defined(SOKOL_GLES2)
    GLuint vao; 
    #endif
    GLuint default_framebuffer;
} _sg_context;

_SOKOL_PRIVATE void _sg_init_context_slot(_sg_context* ctx) {
    SOKOL_ASSERT(ctx);
    memset(ctx, 0, sizeof(_sg_context));
}

_SOKOL_PRIVATE void _sg_gl_init_stencil_state(sg_stencil_state* s) {
    SOKOL_ASSERT(s);
    s->fail_op = SG_STENCILOP_KEEP;
    s->depth_fail_op = SG_STENCILOP_KEEP;
    s->pass_op = SG_STENCILOP_KEEP;
    s->compare_func = SG_COMPAREFUNC_ALWAYS;
}

_SOKOL_PRIVATE void _sg_gl_init_depth_stencil_state(sg_depth_stencil_state* s) {
    SOKOL_ASSERT(s);
    _sg_gl_init_stencil_state(&s->stencil_front);
    _sg_gl_init_stencil_state(&s->stencil_back);
    s->depth_compare_func = SG_COMPAREFUNC_ALWAYS;
    s->depth_write_enabled = false;
    s->stencil_enabled = false;
    s->stencil_read_mask = 0;
    s->stencil_write_mask = 0;
    s->stencil_ref = 0;
}

_SOKOL_PRIVATE void _sg_gl_init_blend_state(sg_blend_state* s) {
    SOKOL_ASSERT(s);
    s->enabled = false;
    s->src_factor_rgb = SG_BLENDFACTOR_ONE;
    s->dst_factor_rgb = SG_BLENDFACTOR_ZERO;
    s->op_rgb = SG_BLENDOP_ADD;
    s->src_factor_alpha = SG_BLENDFACTOR_ONE;
    s->dst_factor_alpha = SG_BLENDFACTOR_ZERO;
    s->op_alpha = SG_BLENDOP_ADD;
    s->color_write_mask = SG_COLORMASK_RGBA;
    for (int i = 0; i < 4; i++) {
        s->blend_color[i] = 0.0f;
    }
}

_SOKOL_PRIVATE void _sg_gl_init_rasterizer_state(sg_rasterizer_state* s) {
    SOKOL_ASSERT(s);
    s->alpha_to_coverage_enabled = false;
    s->cull_mode = SG_CULLMODE_NONE;
    s->face_winding = SG_FACEWINDING_CW;
    s->sample_count = 1;
    s->depth_bias = 0.0f;
    s->depth_bias_slope_scale = 0.0f;
    s->depth_bias_clamp = 0.0f;
}

/*-- state cache implementation ----------------------------------------------*/
typedef struct {
    _sg_gl_attr gl_attr;
    GLuint gl_vbuf;
} _sg_gl_cache_attr;

typedef struct {
    sg_depth_stencil_state ds;
    sg_blend_state blend;
    sg_rasterizer_state rast;
    bool polygon_offset_enabled;
    _sg_gl_cache_attr attrs[SG_MAX_VERTEX_ATTRIBUTES];
    GLuint cur_gl_ib;
    uint32_t cur_ib_offset;
    GLenum cur_primitive_type;
    GLenum cur_index_type;
    _sg_pipeline* cur_pipeline;
    sg_pipeline cur_pipeline_id; 
} _sg_state_cache;

_SOKOL_PRIVATE void _sg_gl_reset_state_cache(_sg_state_cache* cache) {
    SOKOL_ASSERT(cache);
    _SG_GL_CHECK_ERROR();
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    _SG_GL_CHECK_ERROR();
    for (int i = 0; i < SG_MAX_VERTEX_ATTRIBUTES; i++) {
        _sg_gl_init_attr(&cache->attrs[i].gl_attr);
        cache->attrs[i].gl_vbuf = 0;
        glDisableVertexAttribArray(i);
        _SG_GL_CHECK_ERROR();
    }
    cache->cur_gl_ib = 0;
    cache->cur_ib_offset = 0;
    cache->cur_primitive_type = GL_TRIANGLES;
    cache->cur_index_type = 0;

    /* resource bindings */
    cache->cur_pipeline = 0;
    cache->cur_pipeline_id.id = SG_INVALID_ID;

    /* depth-stencil state */
    _sg_gl_init_depth_stencil_state(&cache->ds);
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_ALWAYS);
    glDepthMask(GL_FALSE);
    glDisable(GL_STENCIL_TEST);
    glStencilFunc(GL_ALWAYS, 0, 0);
    glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);
    glStencilMask(0);

    /* blend state */
    _sg_gl_init_blend_state(&cache->blend);
    glDisable(GL_BLEND);
    glBlendFuncSeparate(GL_ONE, GL_ZERO, GL_ONE, GL_ZERO);
    glBlendEquationSeparate(GL_FUNC_ADD, GL_FUNC_ADD);
    glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
    glBlendColor(0.0f, 0.0f, 0.0f, 0.0f);

    /* rasterizer state */
    _sg_gl_init_rasterizer_state(&cache->rast);
    cache->polygon_offset_enabled = false;
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

/*-- main GL backend state and functions -------------------------------------*/
typedef struct {
    bool valid;
    bool in_pass;
    int cur_pass_width;
    int cur_pass_height;
    _sg_context* cur_context;
    _sg_pass* cur_pass;
    sg_pass cur_pass_id;
    _sg_state_cache cache;
    bool features[SG_NUM_FEATURES];
    bool ext_anisotropic;
    GLint max_anisotropy;
} _sg_backend;

static _sg_backend _sg_gl;

_SOKOL_PRIVATE void _sg_setup_backend(const sg_desc* desc) {
    _sg_gl_gles2 = desc->gl_force_gles2;
    memset(&_sg_gl, 0, sizeof(_sg_gl));
    _sg_gl.valid = true;
    _sg_gl.in_pass = false;
    _sg_gl.cur_pass_width = 0;
    _sg_gl.cur_pass_height = 0;
    _sg_gl.cur_pass = 0;
    _sg_gl.cur_pass_id.id = SG_INVALID_ID;
    
    /* initialize feature flags */
    for (int i = 0; i < SG_NUM_FEATURES; i++) {
        _sg_gl.features[i] = false;
    }
    _sg_gl.ext_anisotropic = false;
    _sg_gl.features[SG_FEATURE_ORIGIN_BOTTOM_LEFT] = true;
    #if defined(SOKOL_GLCORE33)
        _sg_gl.features[SG_FEATURE_INSTANCING] = true;
        _sg_gl.features[SG_FEATURE_TEXTURE_FLOAT] = true;
        _sg_gl.features[SG_FEATURE_TEXTURE_HALF_FLOAT] = true;
        _sg_gl.features[SG_FEATURE_MSAA_RENDER_TARGETS] = true;
        _sg_gl.features[SG_FEATURE_PACKED_VERTEX_FORMAT_10_2] = true;
        _sg_gl.features[SG_FEATURE_MULTIPLE_RENDER_TARGET] = true;
        _sg_gl.features[SG_FEATURE_IMAGETYPE_3D] = true;
        _sg_gl.features[SG_FEATURE_IMAGETYPE_ARRAY] = true;
        GLint num_ext = 0;
        glGetIntegerv(GL_NUM_EXTENSIONS, &num_ext);
        for (int i = 0; i < num_ext; i++) {
            const char* ext = (const char*) glGetStringi(GL_EXTENSIONS, i);
            if (strstr(ext, "_texture_compression_s3tc")) {
                _sg_gl.features[SG_FEATURE_TEXTURE_COMPRESSION_DXT] = true;
                continue;
            }
            else if (strstr(ext, "_texture_filter_anisotropic")) {
                _sg_gl.ext_anisotropic = true;
                continue;
            }
        }
    #elif defined(SOKOL_GLES3)
        const char* ext = (const char*) glGetString(GL_EXTENSIONS);
        if (!_sg_gl_gles2) {
            _sg_gl.features[SG_FEATURE_INSTANCING] = true;
            _sg_gl.features[SG_FEATURE_TEXTURE_FLOAT] = true;
            _sg_gl.features[SG_FEATURE_TEXTURE_HALF_FLOAT] = true;
            _sg_gl.features[SG_FEATURE_IMAGETYPE_3D] = true;
            _sg_gl.features[SG_FEATURE_IMAGETYPE_ARRAY] = true;
            _sg_gl.features[SG_FEATURE_MSAA_RENDER_TARGETS] = true;
            _sg_gl.features[SG_FEATURE_PACKED_VERTEX_FORMAT_10_2] = true;
            _sg_gl.features[SG_FEATURE_MULTIPLE_RENDER_TARGET] = true;
        }
        else {
            _sg_gl.features[SG_FEATURE_INSTANCING] = strstr(ext, "_instanced_arrays");
            _sg_gl.features[SG_FEATURE_TEXTURE_FLOAT] = strstr(ext, "_texture_float");
            _sg_gl.features[SG_FEATURE_TEXTURE_HALF_FLOAT] = strstr(ext, "_texture_half_float");
        }
        _sg_gl.features[SG_FEATURE_TEXTURE_COMPRESSION_DXT] =
            strstr(ext, "_texture_compression_s3tc") ||
            strstr(ext, "_compressed_texture_s3tc") ||
            strstr(ext, "texture_compression_dxt1");
        _sg_gl.features[SG_FEATURE_TEXTURE_COMPRESSION_PVRTC] =
            strstr(ext, "_texture_compression_pvrtc") ||
            strstr(ext, "_compressed_texture_pvrtc");
        _sg_gl.features[SG_FEATURE_TEXTURE_COMPRESSION_ATC] =
            strstr(ext, "_compressed_texture_atc");
        _sg_gl.ext_anisotropic =
            strstr(ext, "_texture_filter_anisotropic");
    #elif defined(SOKOL_GLES2)
        const char* ext = (const char*) glGetString(GL_EXTENSIONS);
        _sg_gl.features[SG_FEATURE_INSTANCING] =
            strstr(ext, "_instanced_arrays");
        _sg_gl.features[SG_FEATURE_TEXTURE_FLOAT] =
            strstr(ext, "_texture_float");
        _sg_gl.features[SG_FEATURE_TEXTURE_HALF_FLOAT] =
            strstr(ext, "_texture_half_float");
        _sg_gl.features[SG_FEATURE_TEXTURE_COMPRESSION_DXT] =
            strstr(ext, "_texture_compression_s3tc") ||
            strstr(ext, "_compressed_texture_s3tc") ||
            strstr(ext, "texture_compression_dxt1");
        _sg_gl.features[SG_FEATURE_TEXTURE_COMPRESSION_PVRTC] =
            strstr(ext, "_texture_compression_pvrtc") ||
            strstr(ext, "_compressed_texture_pvrtc");
        _sg_gl.features[SG_FEATURE_TEXTURE_COMPRESSION_ATC] =
            strstr(ext, "_compressed_texture_atc");
        _sg_gl.ext_anisotropic = 
            strstr(ext, "_texture_filter_anisotropic");
    #endif
    _sg_gl.max_anisotropy = 1;
    if (_sg_gl.ext_anisotropic) {
        glGetIntegerv(GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, &_sg_gl.max_anisotropy);
    }
}

_SOKOL_PRIVATE void _sg_discard_backend() {
    SOKOL_ASSERT(_sg_gl.valid);
    _sg_gl.valid = false;
}

_SOKOL_PRIVATE void _sg_reset_state_cache() {
    if (_sg_gl.cur_context) {
        #if !defined(SOKOL_GLES2)
        if (!_sg_gl_gles2) {
            _SG_GL_CHECK_ERROR();
            glBindVertexArray(_sg_gl.cur_context->vao);
            _SG_GL_CHECK_ERROR();
        }
        #endif
        _sg_gl_reset_state_cache(&_sg_gl.cache);
    }
}

_SOKOL_PRIVATE bool _sg_query_feature(sg_feature f) {
    SOKOL_ASSERT((f>=0) && (f<SG_NUM_FEATURES));
    return _sg_gl.features[f];
}

_SOKOL_PRIVATE void _sg_activate_context(_sg_context* ctx) {
    SOKOL_ASSERT(_sg_gl.valid);
    /* NOTE: ctx can be 0 to unset the current context */
    _sg_gl.cur_context = ctx;
    _sg_reset_state_cache();
}

/*-- GL backend resource creation and destruction ----------------------------*/
_SOKOL_PRIVATE void _sg_create_context(_sg_context* ctx) {
    SOKOL_ASSERT(ctx);
    SOKOL_ASSERT(ctx->slot.state == SG_RESOURCESTATE_ALLOC);
    SOKOL_ASSERT(0 == ctx->default_framebuffer);
    _SG_GL_CHECK_ERROR();
    glGetIntegerv(GL_FRAMEBUFFER_BINDING, (GLint*)&ctx->default_framebuffer);
    _SG_GL_CHECK_ERROR();
    #if !defined(SOKOL_GLES2)
    if (!_sg_gl_gles2) {
        SOKOL_ASSERT(0 == ctx->vao);
        glGenVertexArrays(1, &ctx->vao);
        glBindVertexArray(ctx->vao);
        _SG_GL_CHECK_ERROR();
    }
    #endif
    ctx->slot.state = SG_RESOURCESTATE_VALID;
}

_SOKOL_PRIVATE void _sg_destroy_context(_sg_context* ctx) {
    SOKOL_ASSERT(ctx);
    #if !defined(SOKOL_GLES2)
    if (!_sg_gl_gles2) {
        if (ctx->vao) {
            glDeleteVertexArrays(1, &ctx->vao);
        }
        _SG_GL_CHECK_ERROR();
    }
    #endif
    _sg_init_context_slot(ctx);
}

_SOKOL_PRIVATE void _sg_create_buffer(_sg_buffer* buf, const sg_buffer_desc* desc) {
    SOKOL_ASSERT(buf && desc);
    SOKOL_ASSERT(buf->slot.state == SG_RESOURCESTATE_ALLOC);
    _SG_GL_CHECK_ERROR();
    buf->size = desc->size;
    buf->type = _sg_def(desc->type, SG_BUFFERTYPE_VERTEXBUFFER);
    buf->usage = _sg_def(desc->usage, SG_USAGE_IMMUTABLE);
    buf->upd_frame_index = 0;
    buf->num_slots = (buf->usage == SG_USAGE_IMMUTABLE) ? 1 : SG_NUM_INFLIGHT_FRAMES;
    buf->active_slot = 0;
    buf->ext_buffers = (0 != desc->gl_buffers[0]);
    GLenum gl_target = _sg_gl_buffer_target(buf->type);
    GLenum gl_usage  = _sg_gl_usage(buf->usage);
    for (int slot = 0; slot < buf->num_slots; slot++) {
        GLuint gl_buf = 0;
        if (buf->ext_buffers) {
            SOKOL_ASSERT(desc->gl_buffers[slot]);
            gl_buf = desc->gl_buffers[slot];
        }
        else {
            glGenBuffers(1, &gl_buf);
            glBindBuffer(gl_target, gl_buf);
            glBufferData(gl_target, buf->size, 0, gl_usage);
            if (buf->usage == SG_USAGE_IMMUTABLE) {
                SOKOL_ASSERT(desc->content);
                glBufferSubData(gl_target, 0, buf->size, desc->content);
            }
        }
        buf->gl_buf[slot] = gl_buf;
    }
    _SG_GL_CHECK_ERROR();
    buf->slot.state = SG_RESOURCESTATE_VALID;
}

_SOKOL_PRIVATE void _sg_destroy_buffer(_sg_buffer* buf) {
    SOKOL_ASSERT(buf);
    _SG_GL_CHECK_ERROR();
    if (!buf->ext_buffers) {
        for (int slot = 0; slot < buf->num_slots; slot++) {
            if (buf->gl_buf[slot]) {
                glDeleteBuffers(1, &buf->gl_buf[slot]);
            }
        }
        _SG_GL_CHECK_ERROR();
    }
    _sg_init_buffer_slot(buf);
}

_SOKOL_PRIVATE bool _sg_gl_supported_texture_format(sg_pixel_format fmt) {
    switch (fmt) {
        case SG_PIXELFORMAT_DXT1:
        case SG_PIXELFORMAT_DXT3:
        case SG_PIXELFORMAT_DXT5:
            return _sg_gl.features[SG_FEATURE_TEXTURE_COMPRESSION_DXT];
        case SG_PIXELFORMAT_PVRTC2_RGB:
        case SG_PIXELFORMAT_PVRTC4_RGB:
        case SG_PIXELFORMAT_PVRTC2_RGBA:
        case SG_PIXELFORMAT_PVRTC4_RGBA:
            return _sg_gl.features[SG_FEATURE_TEXTURE_COMPRESSION_PVRTC];
        case SG_PIXELFORMAT_ETC2_RGB8:
        case SG_PIXELFORMAT_ETC2_SRGB8:
            return _sg_gl.features[SG_FEATURE_TEXTURE_COMPRESSION_ETC2];
        default:
            return true;
    }
}

_SOKOL_PRIVATE void _sg_create_image(_sg_image* img, const sg_image_desc* desc) {
    SOKOL_ASSERT(img && desc);
    SOKOL_ASSERT(img->slot.state == SG_RESOURCESTATE_ALLOC);
    _SG_GL_CHECK_ERROR();
    img->type = _sg_def(desc->type, SG_IMAGETYPE_2D);
    img->render_target = desc->render_target;
    img->width = desc->width;
    img->height = desc->height;
    img->depth = _sg_def(desc->depth, 1);
    img->num_mipmaps = _sg_def(desc->num_mipmaps, 1);
    img->usage = _sg_def(desc->usage, SG_USAGE_IMMUTABLE);
    img->pixel_format = _sg_def(desc->pixel_format, SG_PIXELFORMAT_RGBA8);
    img->sample_count = _sg_def(desc->sample_count, 1);
    img->min_filter = _sg_def(desc->min_filter, SG_FILTER_NEAREST);
    img->mag_filter = _sg_def(desc->mag_filter, SG_FILTER_NEAREST);
    img->wrap_u = _sg_def(desc->wrap_u, SG_WRAP_REPEAT);
    img->wrap_v = _sg_def(desc->wrap_v, SG_WRAP_REPEAT);
    img->wrap_w = _sg_def(desc->wrap_w, SG_WRAP_REPEAT);
    img->max_anisotropy = _sg_def(desc->max_anisotropy, 1);
    img->upd_frame_index = 0;

    /* check if texture format is support */
    if (!_sg_gl_supported_texture_format(img->pixel_format)) {
        SOKOL_LOG("compressed texture format not supported by GL context\n");
        img->slot.state = SG_RESOURCESTATE_FAILED;
        return;
    }
    /* check for optional texture types */
    if ((img->type == SG_IMAGETYPE_3D) && !_sg_gl.features[SG_FEATURE_IMAGETYPE_3D]) {
        SOKOL_LOG("3D textures not supported by GL context\n");
        img->slot.state = SG_RESOURCESTATE_FAILED;
        return;
    }
    if ((img->type == SG_IMAGETYPE_ARRAY) && !_sg_gl.features[SG_FEATURE_IMAGETYPE_ARRAY]) {
        SOKOL_LOG("array textures not supported by GL context\n");
        img->slot.state = SG_RESOURCESTATE_FAILED;
        return;
    }

    /* create 1 or 2 GL textures, depending on requested update strategy */
    img->num_slots = (img->usage == SG_USAGE_IMMUTABLE) ? 1 : SG_NUM_INFLIGHT_FRAMES;
    img->active_slot = 0;
    img->ext_textures = (0 != desc->gl_textures[0]);

    #if !defined(SOKOL_GLES2)
    bool msaa = false;
    if (!_sg_gl_gles2) {
        msaa = (img->sample_count > 1) && (_sg_gl.features[SG_FEATURE_MSAA_RENDER_TARGETS]);
    }
    #endif
    
    if (_sg_is_valid_rendertarget_depth_format(img->pixel_format)) {
        /* special case depth-stencil-buffer? */
        SOKOL_ASSERT((img->usage == SG_USAGE_IMMUTABLE) && (img->num_slots == 1));
        SOKOL_ASSERT(!img->ext_textures);   /* cannot provide external texture for depth images */
        glGenRenderbuffers(1, &img->gl_depth_render_buffer);
        glBindRenderbuffer(GL_RENDERBUFFER, img->gl_depth_render_buffer);
        GLenum gl_depth_format = _sg_gl_depth_attachment_format(img->pixel_format);
        #if !defined(SOKOL_GLES2)
        if (!_sg_gl_gles2 && msaa) {
            glRenderbufferStorageMultisample(GL_RENDERBUFFER, img->sample_count, gl_depth_format, img->width, img->height);
        }
        else
        #endif
        {
            glRenderbufferStorage(GL_RENDERBUFFER, gl_depth_format, img->width, img->height);
        }
    }
    else {
        /* regular color texture */
        img->gl_target = _sg_gl_texture_target(img->type);
        const GLenum gl_internal_format = _sg_gl_teximage_internal_format(img->pixel_format);

        /* if this is a MSAA render target, need to create a separate render buffer */
        #if !defined(SOKOL_GLES2)
        if (!_sg_gl_gles2 && img->render_target && msaa) {
            glGenRenderbuffers(1, &img->gl_msaa_render_buffer);
            glBindRenderbuffer(GL_RENDERBUFFER, img->gl_msaa_render_buffer);
            glRenderbufferStorageMultisample(GL_RENDERBUFFER, img->sample_count, gl_internal_format, img->width, img->height);
        }
        #endif

        if (img->ext_textures) {
            /* inject externally GL textures */
            for (int slot = 0; slot < img->num_slots; slot++) {
                SOKOL_ASSERT(desc->gl_textures[slot]);
                img->gl_tex[slot] = desc->gl_textures[slot];
            } 
        }
        else {
            /* create our own GL texture(s) */
            const GLenum gl_format = _sg_gl_teximage_format(img->pixel_format);
            const bool is_compressed = _sg_is_compressed_pixel_format(img->pixel_format);
            for (int slot = 0; slot < img->num_slots; slot++) {
                glGenTextures(1, &img->gl_tex[slot]);
                glActiveTexture(GL_TEXTURE0);
                glBindTexture(img->gl_target, img->gl_tex[slot]);
                GLenum gl_min_filter = _sg_gl_filter(img->min_filter);
                GLenum gl_mag_filter = _sg_gl_filter(img->mag_filter);
                glTexParameteri(img->gl_target, GL_TEXTURE_MIN_FILTER, gl_min_filter);
                glTexParameteri(img->gl_target, GL_TEXTURE_MAG_FILTER, gl_mag_filter);
                if (_sg_gl.ext_anisotropic && (img->max_anisotropy > 1)) {
                    GLint max_aniso = (GLint) img->max_anisotropy;
                    if (max_aniso > _sg_gl.max_anisotropy) {
                        max_aniso = _sg_gl.max_anisotropy;
                    }
                    glTexParameteri(img->gl_target, GL_TEXTURE_MAX_ANISOTROPY_EXT, max_aniso);
                }
                if (img->type == SG_IMAGETYPE_CUBE) {
                    glTexParameteri(img->gl_target, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
                    glTexParameteri(img->gl_target, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
                }
                else {
                    glTexParameteri(img->gl_target, GL_TEXTURE_WRAP_S, _sg_gl_wrap(img->wrap_u));
                    glTexParameteri(img->gl_target, GL_TEXTURE_WRAP_T, _sg_gl_wrap(img->wrap_v));
                    #if !defined(SOKOL_GLES2)
                    if (!_sg_gl_gles2 && (img->type == SG_IMAGETYPE_3D)) {
                        glTexParameteri(img->gl_target, GL_TEXTURE_WRAP_R, _sg_gl_wrap(img->wrap_w));
                    }
                    #endif
                }
                #if !defined(SOKOL_GLES2)
                    if (!_sg_gl_gles2) {
                        /* GL spec has strange defaults for mipmap min/max lod: -1000 to +1000 */
                        const float min_lod = _sg_clamp(desc->min_lod, 0.0f, 1000.0f);
                        const float max_lod = _sg_clamp(_sg_def_flt(desc->max_lod, 1000.0f), 0.0f, 1000.0f);
                        glTexParameterf(img->gl_target, GL_TEXTURE_MIN_LOD, min_lod);
                        glTexParameterf(img->gl_target, GL_TEXTURE_MAX_LOD, max_lod);
                    }
                #endif
                const int num_faces = img->type == SG_IMAGETYPE_CUBE ? 6 : 1;
                int data_index = 0;
                for (int face_index = 0; face_index < num_faces; face_index++) {
                    for (int mip_index = 0; mip_index < img->num_mipmaps; mip_index++, data_index++) {
                        GLenum gl_img_target = img->gl_target;
                        if (SG_IMAGETYPE_CUBE == img->type) {
                            gl_img_target = _sg_gl_cubeface_target(face_index);
                        }
                        const GLvoid* data_ptr = desc->content.subimage[face_index][mip_index].ptr;
                        const int data_size = desc->content.subimage[face_index][mip_index].size;
                        int mip_width = img->width >> mip_index;
                        if (mip_width == 0) {
                            mip_width = 1;
                        }
                        int mip_height = img->height >> mip_index;
                        if (mip_height == 0) {
                            mip_height = 1;
                        }
                        if ((SG_IMAGETYPE_2D == img->type) || (SG_IMAGETYPE_CUBE == img->type)) {
                            if (is_compressed) {
                                glCompressedTexImage2D(gl_img_target, mip_index, gl_internal_format,
                                    mip_width, mip_height, 0, data_size, data_ptr);
                            }
                            else {
                                const GLenum gl_type = _sg_gl_teximage_type(img->pixel_format);
                                glTexImage2D(gl_img_target, mip_index, gl_internal_format,
                                    mip_width, mip_height, 0, gl_format, gl_type, data_ptr);
                            }
                        }
                        #if !defined(SOKOL_GLES2)
                        else if (!_sg_gl_gles2 && ((SG_IMAGETYPE_3D == img->type) || (SG_IMAGETYPE_ARRAY == img->type))) {
                            int mip_depth = img->depth >> mip_index;
                            if (mip_depth == 0) {
                                mip_depth = 1;
                            }
                            if (is_compressed) {
                                glCompressedTexImage3D(gl_img_target, mip_index, gl_internal_format,
                                    mip_width, mip_height, mip_depth, 0, data_size, data_ptr);
                            }
                            else {
                                const GLenum gl_type = _sg_gl_teximage_type(img->pixel_format);
                                glTexImage3D(gl_img_target, mip_index, gl_internal_format,
                                    mip_width, mip_height, mip_depth, 0, gl_format, gl_type, data_ptr);
                            }
                        }
                        #endif
                    }
                }
            }
        }
    }
    _SG_GL_CHECK_ERROR();
    img->slot.state = SG_RESOURCESTATE_VALID;
}

_SOKOL_PRIVATE void _sg_destroy_image(_sg_image* img) {
    SOKOL_ASSERT(img);
    _SG_GL_CHECK_ERROR();
    if (!img->ext_textures) {
        for (int slot = 0; slot < img->num_slots; slot++) {
            if (img->gl_tex[slot]) {
                glDeleteTextures(1, &img->gl_tex[slot]);
            }
        }
    }
    if (img->gl_depth_render_buffer) {
        glDeleteRenderbuffers(1, &img->gl_depth_render_buffer);
    }
    if (img->gl_msaa_render_buffer) {
        glDeleteRenderbuffers(1, &img->gl_msaa_render_buffer);
    }
    _SG_GL_CHECK_ERROR();
    _sg_init_image_slot(img);
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
            GLchar* log_buf = (GLchar*) SOKOL_MALLOC(log_len);
            glGetShaderInfoLog(gl_shd, log_len, &log_len, log_buf);
            SOKOL_LOG(log_buf);
            SOKOL_FREE(log_buf);
        }
        glDeleteShader(gl_shd);
        gl_shd = 0;
    }
    _SG_GL_CHECK_ERROR();
    return gl_shd;
}

_SOKOL_PRIVATE void _sg_create_shader(_sg_shader* shd, const sg_shader_desc* desc) {
    SOKOL_ASSERT(shd && desc);
    SOKOL_ASSERT(shd->slot.state == SG_RESOURCESTATE_ALLOC);
    SOKOL_ASSERT(!shd->gl_prog);
    _SG_GL_CHECK_ERROR();
    GLuint gl_vs = _sg_gl_compile_shader(SG_SHADERSTAGE_VS, desc->vs.source);
    GLuint gl_fs = _sg_gl_compile_shader(SG_SHADERSTAGE_FS, desc->fs.source);
    if (!(gl_vs && gl_fs)) {
        shd->slot.state = SG_RESOURCESTATE_FAILED;
        return;
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
            GLchar* log_buf = (GLchar*) SOKOL_MALLOC(log_len);
            glGetProgramInfoLog(gl_prog, log_len, &log_len, log_buf);
            SOKOL_LOG(log_buf);
            SOKOL_FREE(log_buf);
        }
        glDeleteProgram(gl_prog);
        shd->slot.state = SG_RESOURCESTATE_FAILED;
        return;
    }
    shd->gl_prog = gl_prog;

    /* resolve uniforms */
    _SG_GL_CHECK_ERROR();
    for (int stage_index = 0; stage_index < SG_NUM_SHADER_STAGES; stage_index++) {
        const sg_shader_stage_desc* stage_desc = (stage_index == SG_SHADERSTAGE_VS)? &desc->vs : &desc->fs;
        _sg_shader_stage* stage = &shd->stage[stage_index];
        SOKOL_ASSERT(stage->num_uniform_blocks == 0);
        for (int ub_index = 0; ub_index < SG_MAX_SHADERSTAGE_UBS; ub_index++) {
            const sg_shader_uniform_block_desc* ub_desc = &stage_desc->uniform_blocks[ub_index];
            if (0 == ub_desc->size) {
                break;
            }
            _sg_uniform_block* ub = &stage->uniform_blocks[ub_index];
            ub->size = ub_desc->size;
            SOKOL_ASSERT(ub->num_uniforms == 0);
            int cur_uniform_offset = 0;
            for (int u_index = 0; u_index < SG_MAX_UB_MEMBERS; u_index++) {
                const sg_shader_uniform_desc* u_desc = &ub_desc->uniforms[u_index];
                if (u_desc->type == SG_UNIFORMTYPE_INVALID) {
                    break;
                }
                _sg_uniform* u = &ub->uniforms[u_index];
                u->type = u_desc->type;
                u->count = _sg_def(u_desc->array_count, 1);
                u->offset = cur_uniform_offset;
                cur_uniform_offset += _sg_uniform_size(u->type, u->count);
                if (u_desc->name) {
                    u->gl_loc = glGetUniformLocation(gl_prog, u_desc->name);
                }
                else {
                    u->gl_loc = u_index;
                }
                ub->num_uniforms++;
            }
            SOKOL_ASSERT(ub_desc->size == cur_uniform_offset);
            stage->num_uniform_blocks++;
        }
    }

    /* resolve image locations */
    _SG_GL_CHECK_ERROR();
    int gl_tex_slot = 0;
    for (int stage_index = 0; stage_index < SG_NUM_SHADER_STAGES; stage_index++) {
        const sg_shader_stage_desc* stage_desc = (stage_index == SG_SHADERSTAGE_VS)? &desc->vs : &desc->fs;
        _sg_shader_stage* stage = &shd->stage[stage_index];
        SOKOL_ASSERT(stage->num_images == 0);
        for (int img_index = 0; img_index < SG_MAX_SHADERSTAGE_IMAGES; img_index++) {
            const sg_shader_image_desc* img_desc = &stage_desc->images[img_index];
            if (img_desc->type == _SG_IMAGETYPE_DEFAULT) {
                break;
            }
            _sg_shader_image* img = &stage->images[img_index];
            img->type = img_desc->type;
            img->gl_loc = img_index;
            if (img_desc->name) {
                img->gl_loc = glGetUniformLocation(gl_prog, img_desc->name);
            }
            if (img->gl_loc != -1) {
                img->gl_tex_slot = gl_tex_slot++;
            }
            else {
                img->gl_tex_slot = -1;
            }
            stage->num_images++;
        }
    }
    _SG_GL_CHECK_ERROR();
    shd->slot.state = SG_RESOURCESTATE_VALID;
}

_SOKOL_PRIVATE void _sg_destroy_shader(_sg_shader* shd) {
    SOKOL_ASSERT(shd);
    _SG_GL_CHECK_ERROR();
    if (shd->gl_prog) {
        glDeleteProgram(shd->gl_prog);
    }
    _SG_GL_CHECK_ERROR();
    _sg_init_shader_slot(shd);
}

_SOKOL_PRIVATE void _sg_gl_load_stencil(const sg_stencil_state* src, sg_stencil_state* dst) {
    dst->fail_op = _sg_def(src->fail_op, SG_STENCILOP_KEEP);
    dst->depth_fail_op = _sg_def(src->depth_fail_op, SG_STENCILOP_KEEP);
    dst->pass_op = _sg_def(src->pass_op, SG_STENCILOP_KEEP);
    dst->compare_func = _sg_def(src->compare_func, SG_COMPAREFUNC_ALWAYS);
}

_SOKOL_PRIVATE void _sg_gl_load_depth_stencil(const sg_depth_stencil_state* src, sg_depth_stencil_state* dst) {
    _sg_gl_load_stencil(&src->stencil_front, &dst->stencil_front);
    _sg_gl_load_stencil(&src->stencil_back, &dst->stencil_back);
    dst->depth_compare_func = _sg_def(src->depth_compare_func, SG_COMPAREFUNC_ALWAYS);
    dst->depth_write_enabled = src->depth_write_enabled;
    dst->stencil_enabled = src->stencil_enabled;
    dst->stencil_read_mask = src->stencil_read_mask;
    dst->stencil_write_mask = src->stencil_write_mask;
    dst->stencil_ref = src->stencil_ref; 
}

_SOKOL_PRIVATE void _sg_gl_load_blend(const sg_blend_state* src, sg_blend_state* dst) {
    dst->enabled = src->enabled;
    dst->src_factor_rgb = _sg_def(src->src_factor_rgb, SG_BLENDFACTOR_ONE);
    dst->dst_factor_rgb = _sg_def(src->dst_factor_rgb, SG_BLENDFACTOR_ZERO);
    dst->op_rgb = _sg_def(src->op_rgb, SG_BLENDOP_ADD);
    dst->src_factor_alpha = _sg_def(src->src_factor_alpha, SG_BLENDFACTOR_ONE);
    dst->dst_factor_alpha = _sg_def(src->dst_factor_alpha, SG_BLENDFACTOR_ZERO);
    dst->op_alpha = _sg_def(src->op_alpha, SG_BLENDOP_ADD);
    if (src->color_write_mask == SG_COLORMASK_NONE) {
        dst->color_write_mask = 0;
    }
    else {
        dst->color_write_mask = _sg_def((sg_color_mask)src->color_write_mask, SG_COLORMASK_RGBA);
    }
    for (int i = 0; i < 4; i++) {
        dst->blend_color[i] = src->blend_color[i];
    }
}

_SOKOL_PRIVATE void _sg_gl_load_rasterizer(const sg_rasterizer_state* src, sg_rasterizer_state* dst) {
    dst->alpha_to_coverage_enabled = src->alpha_to_coverage_enabled;
    dst->cull_mode = _sg_def(src->cull_mode, SG_CULLMODE_NONE);
    dst->face_winding = _sg_def(src->face_winding, SG_FACEWINDING_CW);
    dst->sample_count = _sg_def(src->sample_count, 1);
    dst->depth_bias = src->depth_bias;
    dst->depth_bias_slope_scale = src->depth_bias_slope_scale;
    dst->depth_bias_clamp = src->depth_bias_clamp;
}

_SOKOL_PRIVATE void _sg_create_pipeline(_sg_pipeline* pip, _sg_shader* shd, const sg_pipeline_desc* desc) {
    SOKOL_ASSERT(pip && shd && desc);
    SOKOL_ASSERT(pip->slot.state == SG_RESOURCESTATE_ALLOC);
    SOKOL_ASSERT(!pip->shader && pip->shader_id.id == SG_INVALID_ID);
    SOKOL_ASSERT(desc->shader.id == shd->slot.id);
    SOKOL_ASSERT(shd->gl_prog);
    pip->shader = shd;
    pip->shader_id = desc->shader;
    pip->primitive_type = _sg_def(desc->primitive_type, SG_PRIMITIVETYPE_TRIANGLES);
    pip->index_type = _sg_def(desc->index_type, SG_INDEXTYPE_NONE);
    pip->color_attachment_count = _sg_def(desc->blend.color_attachment_count, 1);
    pip->color_format = _sg_def(desc->blend.color_format, SG_PIXELFORMAT_RGBA8);
    pip->depth_format = _sg_def(desc->blend.depth_format, SG_PIXELFORMAT_DEPTHSTENCIL);
    pip->sample_count = _sg_def(desc->rasterizer.sample_count, 1);
    _sg_gl_load_depth_stencil(&desc->depth_stencil, &pip->depth_stencil);
    _sg_gl_load_blend(&desc->blend, &pip->blend);
    _sg_gl_load_rasterizer(&desc->rasterizer, &pip->rast);

    /* resolve vertex attributes */
    int auto_offset[SG_MAX_SHADERSTAGE_BUFFERS];
    for (int layout_index = 0; layout_index < SG_MAX_SHADERSTAGE_BUFFERS; layout_index++) {
        auto_offset[layout_index] = 0;
    }
    bool use_auto_offset = true;
    for (int attr_index = 0; attr_index < SG_MAX_VERTEX_ATTRIBUTES; attr_index++) {
        pip->gl_attrs[attr_index].vb_index = -1;
        /* to use computed offsets, *all* attr offsets must be 0 */
        if (desc->layout.attrs[attr_index].offset != 0) {
            use_auto_offset = false;
        }
    }
    for (int attr_index = 0; attr_index < SG_MAX_VERTEX_ATTRIBUTES; attr_index++) {
        const sg_vertex_attr_desc* a_desc = &desc->layout.attrs[attr_index];
        if (a_desc->format == SG_VERTEXFORMAT_INVALID) {
            break;
        }
        SOKOL_ASSERT((a_desc->buffer_index >= 0) && (a_desc->buffer_index < SG_MAX_SHADERSTAGE_BUFFERS));
        const sg_buffer_layout_desc* l_desc = &desc->layout.buffers[a_desc->buffer_index];
        const sg_vertex_step step_func = _sg_def(l_desc->step_func, SG_VERTEXSTEP_PER_VERTEX);
        const int step_rate = _sg_def(l_desc->step_rate, 1);
        GLint attr_loc = attr_index;
        if (a_desc->name) {
            attr_loc = glGetAttribLocation(pip->shader->gl_prog, a_desc->name);
        }
        SOKOL_ASSERT(attr_loc < SG_MAX_VERTEX_ATTRIBUTES);
        if (attr_loc != -1) {
            _sg_gl_attr* gl_attr = &pip->gl_attrs[attr_loc];
            SOKOL_ASSERT(gl_attr->vb_index == -1);
            gl_attr->vb_index = a_desc->buffer_index;
            if (step_func == SG_VERTEXSTEP_PER_VERTEX) {
                gl_attr->divisor = 0;
            }
            else {
                gl_attr->divisor = step_rate;
            }
            gl_attr->stride = l_desc->stride;
            gl_attr->offset = use_auto_offset ? auto_offset[a_desc->buffer_index] : a_desc->offset;
            gl_attr->size = _sg_gl_vertexformat_size(a_desc->format);
            gl_attr->type = _sg_gl_vertexformat_type(a_desc->format);
            gl_attr->normalized = _sg_gl_vertexformat_normalized(a_desc->format);
            pip->vertex_layout_valid[a_desc->buffer_index] = true;
        }
        else {
            SOKOL_LOG("Vertex attribute not found in shader: ");
            SOKOL_LOG(a_desc->name);
        }
        auto_offset[a_desc->buffer_index] += _sg_vertexformat_bytesize(a_desc->format);
    }
    /* fill computed vertex strides that haven't been explicitely provided */
    for (int attr_index = 0; attr_index < SG_MAX_VERTEX_ATTRIBUTES; attr_index++) {
        _sg_gl_attr* gl_attr = &pip->gl_attrs[attr_index];
        if ((gl_attr->vb_index != -1) && (0 == gl_attr->stride)) {
            gl_attr->stride = auto_offset[gl_attr->vb_index];
        }
    }
    pip->slot.state = SG_RESOURCESTATE_VALID;
}

_SOKOL_PRIVATE void _sg_destroy_pipeline(_sg_pipeline* pip) {
    SOKOL_ASSERT(pip);
    _sg_init_pipeline_slot(pip);
}

/*
    _sg_create_pass

    att_imgs must point to a _sg_image* att_imgs[SG_MAX_COLOR_ATTACHMENTS+1] array,
    first entries are the color attachment images (or nullptr), last entry
    is the depth-stencil image (or nullptr).
*/
_SOKOL_PRIVATE void _sg_create_pass(_sg_pass* pass, _sg_image** att_images, const sg_pass_desc* desc) {
    SOKOL_ASSERT(pass && att_images && desc);
    SOKOL_ASSERT(pass->slot.state == SG_RESOURCESTATE_ALLOC);
    SOKOL_ASSERT(att_images && att_images[0]);
    _SG_GL_CHECK_ERROR();

    /* copy image pointers and desc attributes */
    const sg_attachment_desc* att_desc;
    _sg_attachment* att;
    for (int i = 0; i < SG_MAX_COLOR_ATTACHMENTS; i++) {
        SOKOL_ASSERT(0 == pass->color_atts[i].image);
        att_desc = &desc->color_attachments[i];
        if (att_desc->image.id != SG_INVALID_ID) {
            pass->num_color_atts++;
            SOKOL_ASSERT(att_images[i] && (att_images[i]->slot.id == att_desc->image.id));
            SOKOL_ASSERT(_sg_is_valid_rendertarget_color_format(att_images[i]->pixel_format));
            att = &pass->color_atts[i];
            SOKOL_ASSERT((att->image == 0) && (att->image_id.id == SG_INVALID_ID));
            att->image = att_images[i];
            att->image_id = att_desc->image;
            att->mip_level = att_desc->mip_level;
            att->slice = att_desc->slice;
        }
    }
    SOKOL_ASSERT(0 == pass->ds_att.image);
    att_desc = &desc->depth_stencil_attachment;
    const int ds_img_index = SG_MAX_COLOR_ATTACHMENTS;
    if (att_desc->image.id != SG_INVALID_ID) {
        SOKOL_ASSERT(att_images[ds_img_index] && (att_images[ds_img_index]->slot.id == att_desc->image.id));
        SOKOL_ASSERT(_sg_is_valid_rendertarget_depth_format(att_images[ds_img_index]->pixel_format));
        att = &pass->ds_att;
        SOKOL_ASSERT((att->image == 0) && (att->image_id.id == SG_INVALID_ID));
        att->image = att_images[ds_img_index];
        att->image_id = att_desc->image;
        att->mip_level = att_desc->mip_level;
        att->slice = att_desc->slice;
    }

    /* store current framebuffer binding (restored at end of function) */
    GLuint gl_orig_fb;
    glGetIntegerv(GL_FRAMEBUFFER_BINDING, (GLint*)&gl_orig_fb);

    /* create a framebuffer object */
    glGenFramebuffers(1, &pass->gl_fb);
    glBindFramebuffer(GL_FRAMEBUFFER, pass->gl_fb);

    /* attach msaa render buffer or textures */
    const bool is_msaa = (0 != att_images[0]->gl_msaa_render_buffer);
    if (is_msaa) {
        for (int i = 0; i < SG_MAX_COLOR_ATTACHMENTS; i++) {
            const _sg_image* att_img = pass->color_atts[i].image;
            if (att_img) {
                const GLuint gl_render_buffer = att_img->gl_msaa_render_buffer;
                SOKOL_ASSERT(gl_render_buffer);
                glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0+i, GL_RENDERBUFFER, gl_render_buffer);
            }
        }
    }
    else {
        for (int i = 0; i < SG_MAX_COLOR_ATTACHMENTS; i++) {
            const _sg_image* att_img = pass->color_atts[i].image;
            const int mip_level = pass->color_atts[i].mip_level;
            const int slice = pass->color_atts[i].slice;
            if (att_img) {
                const GLuint gl_tex = att_img->gl_tex[0];
                SOKOL_ASSERT(gl_tex);
                const GLenum gl_att = GL_COLOR_ATTACHMENT0 + i;
                switch (att_img->type) {
                    case SG_IMAGETYPE_2D:
                        glFramebufferTexture2D(GL_FRAMEBUFFER, gl_att, GL_TEXTURE_2D, gl_tex, mip_level);
                        break;
                    case SG_IMAGETYPE_CUBE:
                        glFramebufferTexture2D(GL_FRAMEBUFFER, gl_att, _sg_gl_cubeface_target(slice), gl_tex, mip_level);
                        break;
                    default:
                        /* 3D- or array-texture */
                        #if !defined(SOKOL_GLES2)
                        if (!_sg_gl_gles2) {
                            glFramebufferTextureLayer(GL_FRAMEBUFFER, gl_att, gl_tex, mip_level, slice);
                        }
                        #endif
                        break;
                }
            }
        }
    }

    /* attach depth-stencil buffer to framebuffer */
    if (pass->ds_att.image) {
        const GLuint gl_render_buffer = pass->ds_att.image->gl_depth_render_buffer;
        SOKOL_ASSERT(gl_render_buffer);
        glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, gl_render_buffer);
        if (_sg_is_depth_stencil_format(pass->ds_att.image->pixel_format)) {
            glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_STENCIL_ATTACHMENT, GL_RENDERBUFFER, gl_render_buffer);
        }
    }

    /* check if framebuffer is complete */
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        SOKOL_LOG("Framebuffer completeness check failed!\n");
        pass->slot.state = SG_RESOURCESTATE_FAILED;
        return;
    }
    
    /* create MSAA resolve framebuffers if necessary */
    if (is_msaa) {
        for (int i = 0; i < SG_MAX_COLOR_ATTACHMENTS; i++) {
            _sg_attachment* att = &pass->color_atts[i];
            if (att->image) {
                SOKOL_ASSERT(0 == att->gl_msaa_resolve_buffer);
                glGenFramebuffers(1, &att->gl_msaa_resolve_buffer);
                glBindFramebuffer(GL_FRAMEBUFFER, att->gl_msaa_resolve_buffer);
                const GLuint gl_tex = att->image->gl_tex[0];
                SOKOL_ASSERT(gl_tex);
                switch (att->image->type) {
                    case SG_IMAGETYPE_2D:
                        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, 
                            GL_TEXTURE_2D, gl_tex, att->mip_level);
                        break;
                    case SG_IMAGETYPE_CUBE:
                        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, 
                            _sg_gl_cubeface_target(att->slice), gl_tex, att->mip_level);
                        break;
                    default:
                        #if !defined(SOKOL_GLES2)
                        if (!_sg_gl_gles2) {
                            glFramebufferTextureLayer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, gl_tex, att->mip_level, att->slice);
                        }
                        #endif
                        break;
                }
                /* check if framebuffer is complete */
                if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
                    SOKOL_LOG("Framebuffer completeness check failed (msaa resolve buffer)!\n");
                    pass->slot.state = SG_RESOURCESTATE_FAILED;
                    return;
                }
            }
        }
    }

    /* restore original framebuffer binding */
    glBindFramebuffer(GL_FRAMEBUFFER, gl_orig_fb);
    _SG_GL_CHECK_ERROR();
    pass->slot.state = SG_RESOURCESTATE_VALID;
}

_SOKOL_PRIVATE void _sg_destroy_pass(_sg_pass* pass) {
    SOKOL_ASSERT(pass);
    _SG_GL_CHECK_ERROR();
    if (0 != pass->gl_fb) {
        glDeleteFramebuffers(1, &pass->gl_fb);
    }
    for (int i = 0; i < SG_MAX_COLOR_ATTACHMENTS; i++) {
        if (pass->color_atts[i].gl_msaa_resolve_buffer) {
            glDeleteFramebuffers(1, &pass->color_atts[i].gl_msaa_resolve_buffer);
        }
    }
    if (pass->ds_att.gl_msaa_resolve_buffer) {
        glDeleteFramebuffers(1, &pass->ds_att.gl_msaa_resolve_buffer);
    }
    _SG_GL_CHECK_ERROR();
    _sg_init_pass_slot(pass);
}

/*-- GL backend rendering functions ------------------------------------------*/
_SOKOL_PRIVATE void _sg_begin_pass(_sg_pass* pass, const sg_pass_action* action, int w, int h) {
    /* FIXME: what if a texture used as render target is still bound, should we
       unbind all currently bound textures in begin pass? */
    SOKOL_ASSERT(action);
    SOKOL_ASSERT(!_sg_gl.in_pass);
    _SG_GL_CHECK_ERROR();
    _sg_gl.in_pass = true;
    _sg_gl.cur_pass = pass; /* can be 0 */
    if (pass) {
        _sg_gl.cur_pass_id.id = pass->slot.id;
    }
    else {
        _sg_gl.cur_pass_id.id = SG_INVALID_ID;
    }
    _sg_gl.cur_pass_width = w;
    _sg_gl.cur_pass_height = h;
    if (pass) {
        /* offscreen pass */
        SOKOL_ASSERT(pass->gl_fb);
        glBindFramebuffer(GL_FRAMEBUFFER, pass->gl_fb);
        #if !defined(SOKOL_GLES2)
        if (!_sg_gl_gles2) {
            GLenum att[SG_MAX_COLOR_ATTACHMENTS] = {
                GL_COLOR_ATTACHMENT0,
                GL_COLOR_ATTACHMENT1,
                GL_COLOR_ATTACHMENT2,
                GL_COLOR_ATTACHMENT3
            };
            int num_attrs = 0;
            for (int i = 0; i < SG_MAX_COLOR_ATTACHMENTS; i++) {
                if (pass->color_atts[num_attrs].image) {
                    num_attrs++;
                }
                else {
                    break;
                }
            }
            glDrawBuffers(num_attrs, att);
        }
        #endif
    }
    else {
        /* default pass */
        SOKOL_ASSERT(_sg_gl.cur_context);
        glBindFramebuffer(GL_FRAMEBUFFER, _sg_gl.cur_context->default_framebuffer);
    }
    glViewport(0, 0, w, h);
    glScissor(0, 0, w, h);
    bool need_pip_cache_flush = false;
    if (_sg_gl.cache.blend.color_write_mask != SG_COLORMASK_RGBA) {
        need_pip_cache_flush = true;
        _sg_gl.cache.blend.color_write_mask = SG_COLORMASK_RGBA;
        glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
    }
    if (!_sg_gl.cache.ds.depth_write_enabled) {
        need_pip_cache_flush = true;
        _sg_gl.cache.ds.depth_write_enabled = true;
        glDepthMask(GL_TRUE);
    }
    if (_sg_gl.cache.ds.depth_compare_func != SG_COMPAREFUNC_ALWAYS) {
        need_pip_cache_flush = true;
        _sg_gl.cache.ds.depth_compare_func = SG_COMPAREFUNC_ALWAYS;
        glDepthFunc(GL_ALWAYS);
    }
    if (_sg_gl.cache.ds.stencil_write_mask != 0xFF) {
        need_pip_cache_flush = true;
        _sg_gl.cache.ds.stencil_write_mask = 0xFF;
        glStencilMask(0xFF);
    }
    if (need_pip_cache_flush) {
        /* we messed with the state cache directly, need to clear cached 
           pipeline to force re-evaluation in next sg_apply_draw_state() */
        _sg_gl.cache.cur_pipeline = 0;
        _sg_gl.cache.cur_pipeline_id.id = SG_INVALID_ID;
    }
    bool use_mrt_clear = (0 != pass);
    #if defined(SOKOL_GLES2)
    use_mrt_clear = false;
    #else
    if (_sg_gl_gles2) {
        use_mrt_clear = false;
    }
    #endif
    if (!use_mrt_clear) {
        GLbitfield clear_mask = 0;
        if (action->colors[0].action == SG_ACTION_CLEAR) {
            clear_mask |= GL_COLOR_BUFFER_BIT;
            const float* c = action->colors[0].val;
            glClearColor(c[0], c[1], c[2], c[3]);
        }
        if (action->depth.action == SG_ACTION_CLEAR) {
            clear_mask |= GL_DEPTH_BUFFER_BIT;
            #ifdef SOKOL_GLCORE33
            glClearDepth(action->depth.val);
            #else
            glClearDepthf(action->depth.val);
            #endif
        }
        if (action->stencil.action == SG_ACTION_CLEAR) {
            clear_mask |= GL_STENCIL_BUFFER_BIT;
            glClearStencil(action->stencil.val);
        }
        if (0 != clear_mask) {
            glClear(clear_mask);
        }
    }
    #if !defined SOKOL_GLES2
    else {
        SOKOL_ASSERT(pass);
        for (int i = 0; i < SG_MAX_COLOR_ATTACHMENTS; i++) {
            if (pass->color_atts[i].image) {
                if (action->colors[i].action == SG_ACTION_CLEAR) {
                    glClearBufferfv(GL_COLOR, i, action->colors[i].val);
                }
            }
            else {
                break;
            }
        }
        if (pass->ds_att.image) {
            if ((action->depth.action == SG_ACTION_CLEAR) && (action->stencil.action == SG_ACTION_CLEAR)) {
                glClearBufferfi(GL_DEPTH_STENCIL, 0, action->depth.val, action->stencil.val);
            }
            else if (action->depth.action == SG_ACTION_CLEAR) {
                glClearBufferfv(GL_DEPTH, 0, &action->depth.val);
            }
            else if (action->stencil.action == SG_ACTION_CLEAR) {
                GLuint val = action->stencil.val;
                glClearBufferuiv(GL_STENCIL, 0, &val);
            }
        }
    }
    #endif
    _SG_GL_CHECK_ERROR();
}

_SOKOL_PRIVATE void _sg_end_pass() {
    SOKOL_ASSERT(_sg_gl.in_pass);
    _SG_GL_CHECK_ERROR();

    /* if this was an offscreen pass, and MSAA rendering was used, need 
       to resolve into the pass images */
    #if !defined(SOKOL_GLES2)
    if (!_sg_gl_gles2 && _sg_gl.cur_pass) {
        /* check if the pass object is still valid */
        const _sg_pass* pass = _sg_gl.cur_pass;
        SOKOL_ASSERT(pass->slot.id == _sg_gl.cur_pass_id.id);
        bool is_msaa = (0 != _sg_gl.cur_pass->color_atts[0].gl_msaa_resolve_buffer);
        if (is_msaa) {
            SOKOL_ASSERT(pass->gl_fb);
            glBindFramebuffer(GL_READ_FRAMEBUFFER, pass->gl_fb);
            SOKOL_ASSERT(pass->color_atts[0].image);
            const int w = pass->color_atts[0].image->width;
            const int h = pass->color_atts[0].image->height;
            for (int att_index = 0; att_index < SG_MAX_COLOR_ATTACHMENTS; att_index++) {
                const _sg_attachment* att = &pass->color_atts[att_index];
                if (att->image) {
                    SOKOL_ASSERT(att->gl_msaa_resolve_buffer);
                    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, att->gl_msaa_resolve_buffer);
                    glReadBuffer(GL_COLOR_ATTACHMENT0 + att_index);
                    const GLenum gl_att = GL_COLOR_ATTACHMENT0;
                    glDrawBuffers(1, &gl_att);
                    glBlitFramebuffer(0, 0, w, h, 0, 0, w, h, GL_COLOR_BUFFER_BIT, GL_NEAREST);
                }
                else {
                    break;
                }
            }
        }
    }
    #endif
    _sg_gl.cur_pass = 0;
    _sg_gl.cur_pass_id.id = SG_INVALID_ID;
    _sg_gl.cur_pass_width = 0;
    _sg_gl.cur_pass_height = 0;

    SOKOL_ASSERT(_sg_gl.cur_context);
    glBindFramebuffer(GL_FRAMEBUFFER, _sg_gl.cur_context->default_framebuffer);
    _sg_gl.in_pass = false;
    _SG_GL_CHECK_ERROR();
}

_SOKOL_PRIVATE void _sg_apply_viewport(int x, int y, int w, int h, bool origin_top_left) {
    SOKOL_ASSERT(_sg_gl.in_pass);
    y = origin_top_left ? (_sg_gl.cur_pass_height - (y+h)) : y;
    glViewport(x, y, w, h);
}

_SOKOL_PRIVATE void _sg_apply_scissor_rect(int x, int y, int w, int h, bool origin_top_left) {
    SOKOL_ASSERT(_sg_gl.in_pass);
    y = origin_top_left ? (_sg_gl.cur_pass_height - (y+h)) : y;
    glScissor(x, y, w, h);
}

_SOKOL_PRIVATE void _sg_apply_draw_state( 
    _sg_pipeline* pip, 
    _sg_buffer** vbs, const uint32_t* vb_offsets, int num_vbs,
    _sg_buffer* ib, uint32_t ib_offset,
    _sg_image** vs_imgs, int num_vs_imgs,
    _sg_image** fs_imgs, int num_fs_imgs)
{
    SOKOL_ASSERT(pip);
    SOKOL_ASSERT(pip->shader);
    _SG_GL_CHECK_ERROR();

    /* need to apply pipeline state? */
    if ((_sg_gl.cache.cur_pipeline != pip) || (_sg_gl.cache.cur_pipeline_id.id != pip->slot.id)) {
        _sg_gl.cache.cur_pipeline = pip;
        _sg_gl.cache.cur_pipeline_id.id = pip->slot.id;
        _sg_gl.cache.cur_primitive_type = _sg_gl_primitive_type(pip->primitive_type);
        _sg_gl.cache.cur_index_type = _sg_gl_index_type(pip->index_type);

        /* update depth-stencil state */
        const sg_depth_stencil_state* new_ds = &pip->depth_stencil;
        sg_depth_stencil_state* cache_ds = &_sg_gl.cache.ds;
        if (new_ds->depth_compare_func != cache_ds->depth_compare_func) {
            cache_ds->depth_compare_func = new_ds->depth_compare_func;
            glDepthFunc(_sg_gl_compare_func(new_ds->depth_compare_func));
        }
        if (new_ds->depth_write_enabled != cache_ds->depth_write_enabled) {
            cache_ds->depth_write_enabled = new_ds->depth_write_enabled;
            glDepthMask(new_ds->depth_write_enabled);
        }
        if (new_ds->stencil_enabled != cache_ds->stencil_enabled) {
            cache_ds->stencil_enabled = new_ds->stencil_enabled;
            if (new_ds->stencil_enabled) glEnable(GL_STENCIL_TEST); 
            else glDisable(GL_STENCIL_TEST);
        }
        if (new_ds->stencil_write_mask != cache_ds->stencil_write_mask) {
            cache_ds->stencil_write_mask = new_ds->stencil_write_mask;
            glStencilMask(new_ds->stencil_write_mask);
        }
        for (int i = 0; i < 2; i++) {
            const sg_stencil_state* new_ss = (i==0)? &new_ds->stencil_front : &new_ds->stencil_back;
            sg_stencil_state* cache_ss = (i==0)? &cache_ds->stencil_front : &cache_ds->stencil_back;
            GLenum gl_face = (i==0)? GL_FRONT : GL_BACK;
            if ((new_ss->compare_func != cache_ss->compare_func) ||
                (new_ds->stencil_read_mask != cache_ds->stencil_read_mask) ||
                (new_ds->stencil_ref != cache_ds->stencil_ref))
            {
                cache_ss->compare_func = new_ss->compare_func;
                glStencilFuncSeparate(gl_face, 
                    _sg_gl_compare_func(new_ss->compare_func), 
                    new_ds->stencil_ref, 
                    new_ds->stencil_read_mask);
            }
            if ((new_ss->fail_op != cache_ss->fail_op) ||
                (new_ss->depth_fail_op != cache_ss->depth_fail_op) ||
                (new_ss->pass_op != cache_ss->pass_op))
            {
                cache_ss->fail_op = new_ss->fail_op;
                cache_ss->depth_fail_op = new_ss->depth_fail_op;
                cache_ss->pass_op = new_ss->pass_op;
                glStencilOpSeparate(gl_face,
                    _sg_gl_stencil_op(new_ss->fail_op),
                    _sg_gl_stencil_op(new_ss->depth_fail_op),
                    _sg_gl_stencil_op(new_ss->pass_op));
            }
        }
        cache_ds->stencil_read_mask = new_ds->stencil_read_mask;
        cache_ds->stencil_ref = new_ds->stencil_ref;

        /* update blend state */
        const sg_blend_state* new_b = &pip->blend;
        sg_blend_state* cache_b = &_sg_gl.cache.blend;
        if (new_b->enabled != cache_b->enabled) {
            cache_b->enabled = new_b->enabled;
            if (new_b->enabled) glEnable(GL_BLEND);
            else glDisable(GL_BLEND);
        }
        if ((new_b->src_factor_rgb != cache_b->src_factor_rgb) ||
            (new_b->dst_factor_rgb != cache_b->dst_factor_rgb) ||
            (new_b->src_factor_alpha != cache_b->src_factor_alpha) ||
            (new_b->dst_factor_alpha != cache_b->dst_factor_alpha))
        {
            cache_b->src_factor_rgb = new_b->src_factor_rgb;
            cache_b->dst_factor_rgb = new_b->dst_factor_rgb;
            cache_b->src_factor_alpha = new_b->src_factor_alpha;
            cache_b->dst_factor_alpha = new_b->dst_factor_alpha;
            glBlendFuncSeparate(_sg_gl_blend_factor(new_b->src_factor_rgb),
                _sg_gl_blend_factor(new_b->dst_factor_rgb),
                _sg_gl_blend_factor(new_b->src_factor_alpha),
                _sg_gl_blend_factor(new_b->dst_factor_alpha));
        }
        if ((new_b->op_rgb != cache_b->op_rgb) || (new_b->op_alpha != cache_b->op_alpha)) {
            cache_b->op_rgb = new_b->op_rgb;
            cache_b->op_alpha = new_b->op_alpha;
            glBlendEquationSeparate(_sg_gl_blend_op(new_b->op_rgb), _sg_gl_blend_op(new_b->op_alpha));
        }
        if (new_b->color_write_mask != cache_b->color_write_mask) {
            cache_b->color_write_mask = new_b->color_write_mask;
            glColorMask((new_b->color_write_mask & SG_COLORMASK_R) != 0,
                        (new_b->color_write_mask & SG_COLORMASK_G) != 0,
                        (new_b->color_write_mask & SG_COLORMASK_B) != 0,
                        (new_b->color_write_mask & SG_COLORMASK_A) != 0);
        }
        if (!_sg_fequal(new_b->blend_color[0], cache_b->blend_color[0], 0.0001f) ||
            !_sg_fequal(new_b->blend_color[1], cache_b->blend_color[1], 0.0001f) ||
            !_sg_fequal(new_b->blend_color[2], cache_b->blend_color[2], 0.0001f) ||
            !_sg_fequal(new_b->blend_color[3], cache_b->blend_color[3], 0.0001f))
        {
            const float* bc = new_b->blend_color;
            for (int i=0; i<4; i++) {
                cache_b->blend_color[i] = bc[i];
            }
            glBlendColor(bc[0], bc[1], bc[2], bc[3]);
        }

        /* update rasterizer state */
        const sg_rasterizer_state* new_r = &pip->rast;
        sg_rasterizer_state* cache_r = &_sg_gl.cache.rast;
        if (new_r->cull_mode != cache_r->cull_mode) {
            cache_r->cull_mode = new_r->cull_mode;
            if (SG_CULLMODE_NONE == new_r->cull_mode) {
                glDisable(GL_CULL_FACE);
            }
            else {
                glEnable(GL_CULL_FACE);
                GLenum gl_mode = (SG_CULLMODE_FRONT == new_r->cull_mode) ? GL_FRONT : GL_BACK;
                glCullFace(gl_mode);
            }
        }
        if (new_r->face_winding != cache_r->face_winding) {
            cache_r->face_winding = new_r->face_winding;
            GLenum gl_winding = (SG_FACEWINDING_CW == new_r->face_winding) ? GL_CW : GL_CCW;
            glFrontFace(gl_winding);
        }
        if (new_r->alpha_to_coverage_enabled != cache_r->alpha_to_coverage_enabled) {
            cache_r->alpha_to_coverage_enabled = new_r->alpha_to_coverage_enabled;
            if (new_r->alpha_to_coverage_enabled) glEnable(GL_SAMPLE_ALPHA_TO_COVERAGE);
            else glDisable(GL_SAMPLE_ALPHA_TO_COVERAGE);
        }
        #ifdef SOKOL_GLCORE33
        if (new_r->sample_count != cache_r->sample_count) {
            cache_r->sample_count = new_r->sample_count;
            if (new_r->sample_count > 1) glEnable(GL_MULTISAMPLE);
            else glDisable(GL_MULTISAMPLE);
        }
        #endif
        if (!_sg_fequal(new_r->depth_bias, cache_r->depth_bias, 0.000001f) ||
            !_sg_fequal(new_r->depth_bias_slope_scale, cache_r->depth_bias_slope_scale, 0.000001f))
        {
            /* according to ANGLE's D3D11 backend:
                D3D11 SlopeScaledDepthBias ==> GL polygonOffsetFactor
                D3D11 DepthBias ==> GL polygonOffsetUnits
                DepthBiasClamp has no meaning on GL
            */
            cache_r->depth_bias = new_r->depth_bias;
            cache_r->depth_bias_slope_scale = new_r->depth_bias_slope_scale;
            glPolygonOffset(new_r->depth_bias_slope_scale, new_r->depth_bias);
            bool po_enabled = true;
            if (_sg_fequal(new_r->depth_bias, 0.0f, 0.000001f) &&
                _sg_fequal(new_r->depth_bias_slope_scale, 0.0f, 0.000001f))
            {
                po_enabled = false;
            }
            if (po_enabled != _sg_gl.cache.polygon_offset_enabled) {
                _sg_gl.cache.polygon_offset_enabled = po_enabled;
                if (po_enabled) glEnable(GL_POLYGON_OFFSET_FILL);
                else glDisable(GL_POLYGON_OFFSET_FILL);
            }
        }

        /* bind shader program */
        glUseProgram(pip->shader->gl_prog);
    }

    /* bind textures */
    _SG_GL_CHECK_ERROR();
    for (int stage_index = 0; stage_index < SG_NUM_SHADER_STAGES; stage_index++) {
        const _sg_shader_stage* stage = &pip->shader->stage[stage_index];
        _sg_image** imgs = (stage_index == SG_SHADERSTAGE_VS)? vs_imgs : fs_imgs;
        SOKOL_ASSERT(((stage_index == SG_SHADERSTAGE_VS)? num_vs_imgs : num_fs_imgs) == stage->num_images);
        for (int img_index = 0; img_index < stage->num_images; img_index++) {
            const _sg_shader_image* shd_img = &stage->images[img_index];
            if (shd_img->gl_loc != -1) {
                _sg_image* img = imgs[img_index];
                const GLuint gl_tex = img->gl_tex[img->active_slot];
                SOKOL_ASSERT(img && img->gl_target);
                SOKOL_ASSERT((shd_img->gl_tex_slot != -1) && gl_tex);
                glUniform1i(shd_img->gl_loc, shd_img->gl_tex_slot);
                glActiveTexture(GL_TEXTURE0+shd_img->gl_tex_slot);
                glBindTexture(img->gl_target, gl_tex);
            }
        }
    }
    _SG_GL_CHECK_ERROR();

    /* index buffer (can be 0) */
    const GLuint gl_ib = ib ? ib->gl_buf[ib->active_slot] : 0;
    if (gl_ib != _sg_gl.cache.cur_gl_ib) {
        _sg_gl.cache.cur_gl_ib = gl_ib;
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, gl_ib);
    }
    _sg_gl.cache.cur_ib_offset = ib_offset;

    /* vertex attributes */
    GLuint gl_vb = 0;
    for (int attr_index = 0; attr_index < SG_MAX_VERTEX_ATTRIBUTES; attr_index++) {
        _sg_gl_attr* attr = &pip->gl_attrs[attr_index];
        _sg_gl_cache_attr* cache_attr = &_sg_gl.cache.attrs[attr_index];
        bool cache_attr_dirty = false;
        uint32_t vb_offset = 0;
        if (attr->vb_index >= 0) {
            /* attribute is enabled */
            SOKOL_ASSERT(attr->vb_index < num_vbs);
            _sg_buffer* vb = vbs[attr->vb_index];
            SOKOL_ASSERT(vb);
            vb_offset = vb_offsets[attr->vb_index] + attr->offset;
            if ((vb->gl_buf[vb->active_slot] != cache_attr->gl_vbuf) ||
                (attr->size != cache_attr->gl_attr.size) ||
                (attr->type != cache_attr->gl_attr.type) ||
                (attr->normalized != cache_attr->gl_attr.normalized) ||
                (attr->stride != cache_attr->gl_attr.stride) ||
                (vb_offset != cache_attr->gl_attr.offset) ||
                (cache_attr->gl_attr.divisor != attr->divisor))
            {
                if (gl_vb != vb->gl_buf[vb->active_slot]) {
                    gl_vb = vb->gl_buf[vb->active_slot];
                    glBindBuffer(GL_ARRAY_BUFFER, gl_vb);
                }
                glVertexAttribPointer(attr_index, attr->size, attr->type, 
                    attr->normalized, attr->stride, 
                    (const GLvoid*)(GLintptr)vb_offset);
                if (_sg_gl.features[SG_FEATURE_INSTANCING]) {
                    glVertexAttribDivisor(attr_index, attr->divisor);
                }
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

_SOKOL_PRIVATE void _sg_apply_uniform_block(sg_shader_stage stage_index, int ub_index, const void* data, int num_bytes) {
    SOKOL_ASSERT(data && (num_bytes > 0));
    SOKOL_ASSERT((stage_index >= 0) && ((int)stage_index < SG_NUM_SHADER_STAGES));
    SOKOL_ASSERT(_sg_gl.cache.cur_pipeline);
    SOKOL_ASSERT(_sg_gl.cache.cur_pipeline->slot.id == _sg_gl.cache.cur_pipeline_id.id);
    SOKOL_ASSERT(_sg_gl.cache.cur_pipeline->shader->slot.id == _sg_gl.cache.cur_pipeline->shader_id.id);
    _sg_shader_stage* stage = &_sg_gl.cache.cur_pipeline->shader->stage[stage_index];
    SOKOL_ASSERT(ub_index < stage->num_uniform_blocks);
    _sg_uniform_block* ub = &stage->uniform_blocks[ub_index];
    SOKOL_ASSERT(ub->size == num_bytes);
    for (int u_index = 0; u_index < ub->num_uniforms; u_index++) {
        _sg_uniform* u = &ub->uniforms[u_index];
        SOKOL_ASSERT(u->type != SG_UNIFORMTYPE_INVALID);
        if (u->gl_loc == -1) {
            continue;
        }
        GLfloat* ptr = (GLfloat*) (((uint8_t*)data) + u->offset);
        switch (u->type) {
            case SG_UNIFORMTYPE_INVALID:
                break;
            case SG_UNIFORMTYPE_FLOAT:
                glUniform1fv(u->gl_loc, u->count, ptr);
                break;
            case SG_UNIFORMTYPE_FLOAT2:
                glUniform2fv(u->gl_loc, u->count, ptr);
                break;
            case SG_UNIFORMTYPE_FLOAT3:
                glUniform3fv(u->gl_loc, u->count, ptr);
                break;
            case SG_UNIFORMTYPE_FLOAT4:
                glUniform4fv(u->gl_loc, u->count, ptr);
                break;
            case SG_UNIFORMTYPE_MAT4:
                glUniformMatrix4fv(u->gl_loc, u->count, GL_FALSE, ptr);
                break;
            default:
                SOKOL_UNREACHABLE;
                break;
        }
    }
}

_SOKOL_PRIVATE void _sg_draw(int base_element, int num_elements, int num_instances) {
    const GLenum i_type = _sg_gl.cache.cur_index_type;
    const GLenum p_type = _sg_gl.cache.cur_primitive_type;
    if (0 != i_type) {
        /* indexed rendering */
        const int i_size = (i_type == GL_UNSIGNED_SHORT) ? 2 : 4;
        const uint32_t ib_offset = _sg_gl.cache.cur_ib_offset;
        const GLvoid* indices = (const GLvoid*)(GLintptr)(base_element*i_size+ib_offset);
        if (num_instances == 1) {
            glDrawElements(p_type, num_elements, i_type, indices);
        }
        else {
            if (_sg_gl.features[SG_FEATURE_INSTANCING]) {
                glDrawElementsInstanced(p_type, num_elements, i_type, indices, num_instances);
            }
        }
    }
    else {
        /* non-indexed rendering */
        if (num_instances == 1) {
            glDrawArrays(p_type, base_element, num_elements);
        }
        else {
            if (_sg_gl.features[SG_FEATURE_INSTANCING]) {
                glDrawArraysInstanced(p_type, base_element, num_elements, num_instances);
            }
        }
    }
}

_SOKOL_PRIVATE void _sg_commit() {
    SOKOL_ASSERT(!_sg_gl.in_pass);
}

_SOKOL_PRIVATE void _sg_update_buffer(_sg_buffer* buf, const void* data_ptr, int data_size) {
    SOKOL_ASSERT(buf && data_ptr && (data_size > 0));
    /* only one update per buffer per frame allowed */
    if (++buf->active_slot >= buf->num_slots) {
        buf->active_slot = 0;
    }
    GLenum gl_tgt = _sg_gl_buffer_target(buf->type);
    SOKOL_ASSERT(buf->active_slot < SG_NUM_INFLIGHT_FRAMES);
    GLuint gl_buf = buf->gl_buf[buf->active_slot];
    SOKOL_ASSERT(gl_buf);
    _SG_GL_CHECK_ERROR();
    glBindBuffer(gl_tgt, gl_buf);
    glBufferSubData(gl_tgt, 0, data_size, data_ptr);
    _SG_GL_CHECK_ERROR();
}

_SOKOL_PRIVATE void _sg_update_image(_sg_image* img, const sg_image_content* data) {
    SOKOL_ASSERT(img && data);
    /* only one update per image per frame allowed */
    if (++img->active_slot >= img->num_slots) {
        img->active_slot = 0;
    }
    SOKOL_ASSERT(img->active_slot < SG_NUM_INFLIGHT_FRAMES);
    SOKOL_ASSERT(0 != img->gl_tex[img->active_slot]);
    glBindTexture(img->gl_target, img->gl_tex[img->active_slot]);
    const GLenum gl_img_format = _sg_gl_teximage_format(img->pixel_format);
    const GLenum gl_img_type = _sg_gl_teximage_type(img->pixel_format);
    const int num_faces = img->type == SG_IMAGETYPE_CUBE ? 6 : 1;
    const int num_mips = img->num_mipmaps;
    for (int face_index = 0; face_index < num_faces; face_index++) {
        for (int mip_index = 0; mip_index < num_mips; mip_index++) {
            GLenum gl_img_target = img->gl_target;
            if (SG_IMAGETYPE_CUBE == img->type) {
                gl_img_target = _sg_gl_cubeface_target(face_index);
            }
            const GLvoid* data_ptr = data->subimage[face_index][mip_index].ptr;
            int mip_width = img->width >> mip_index;
            if (mip_width == 0) {
                mip_width = 1;
            }
            int mip_height = img->height >> mip_index;
            if (mip_height == 0) {
                mip_height = 1;
            }
            if ((SG_IMAGETYPE_2D == img->type) || (SG_IMAGETYPE_CUBE == img->type)) {
                glTexSubImage2D(gl_img_target, mip_index,
                    0, 0,
                    mip_width, mip_height,
                    gl_img_format, gl_img_type,
                    data_ptr);
            }
            #if !defined(SOKOL_GLES2)
            else if (!_sg_gl_gles2 && ((SG_IMAGETYPE_3D == img->type) || (SG_IMAGETYPE_ARRAY == img->type))) {
                int mip_depth = img->depth >> mip_index;
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
}

#ifdef __cplusplus
} /* extern "C" */
#endif

/*== D3D11 BACKEND ===========================================================*/
#elif defined(SOKOL_D3D11)

#ifndef D3D11_NO_HELPERS
#define D3D11_NO_HELPERS
#endif
#ifndef CINTERFACE
#define CINTERFACE
#endif
#ifndef COBJMACROS
#define COBJMACROS
#endif
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <d3d11.h>

#if (defined(WINAPI_FAMILY_PARTITION) && !WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP))
#pragma comment (lib, "WindowsApp.lib")
#else
#pragma comment (lib, "user32.lib")
#pragma comment (lib, "dxgi.lib")
#pragma comment (lib, "d3d11.lib")
#pragma comment (lib, "dxguid.lib")
#endif
#if defined(SOKOL_D3D11_SHADER_COMPILER)
#include <d3dcompiler.h>
#if !(defined(WINAPI_FAMILY_PARTITION) && !WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP))
#pragma comment (lib, "d3dcompiler.lib")
#endif
#endif

#ifdef __cplusplus
extern "C" {
#endif

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

_SOKOL_PRIVATE DXGI_FORMAT _sg_d3d11_texture_format(sg_pixel_format fmt) {
    /*
        NOTE: the following pixel formats are only supported on D3D11.1 
        (we're running on D3D11.0):
            DXGI_FORMAT_B4G4R4A4_UNORM
            DXGI_FORMAT_B5G6R5_UNORM
            DXGI_FORMAT_B5G5R5A1_UNORM
    */
    switch (fmt) {
        case SG_PIXELFORMAT_RGBA8:          return DXGI_FORMAT_R8G8B8A8_UNORM;
        case SG_PIXELFORMAT_R10G10B10A2:    return DXGI_FORMAT_R10G10B10A2_UNORM;
        case SG_PIXELFORMAT_RGBA32F:        return DXGI_FORMAT_R32G32B32A32_FLOAT;
        case SG_PIXELFORMAT_RGBA16F:        return DXGI_FORMAT_R16G16B16A16_FLOAT;
        case SG_PIXELFORMAT_R32F:           return DXGI_FORMAT_R32_FLOAT;
        case SG_PIXELFORMAT_R16F:           return DXGI_FORMAT_R16_FLOAT;
        case SG_PIXELFORMAT_L8:             return DXGI_FORMAT_R8_UNORM;
        case SG_PIXELFORMAT_DXT1:           return DXGI_FORMAT_BC1_UNORM;
        case SG_PIXELFORMAT_DXT3:           return DXGI_FORMAT_BC2_UNORM;
        case SG_PIXELFORMAT_DXT5:           return DXGI_FORMAT_BC3_UNORM;
        default:                            return DXGI_FORMAT_UNKNOWN;
    };
}

_SOKOL_PRIVATE DXGI_FORMAT _sg_d3d11_rendertarget_color_format(sg_pixel_format fmt) {
    switch (fmt) {
        case SG_PIXELFORMAT_RGBA8:          return DXGI_FORMAT_R8G8B8A8_UNORM;
        case SG_PIXELFORMAT_RGBA32F:        return DXGI_FORMAT_R32G32B32A32_FLOAT;
        case SG_PIXELFORMAT_RGBA16F:        return DXGI_FORMAT_R16G16B16A16_FLOAT;
        case SG_PIXELFORMAT_R32F:           return DXGI_FORMAT_R32_FLOAT;
        case SG_PIXELFORMAT_R16F:           return DXGI_FORMAT_R16_FLOAT;
        case SG_PIXELFORMAT_L8:             return DXGI_FORMAT_R8_UNORM;
        default:                            return DXGI_FORMAT_UNKNOWN;
    }
}

_SOKOL_PRIVATE DXGI_FORMAT _sg_d3d11_rendertarget_depth_format(sg_pixel_format fmt) {
    switch (fmt) {
        case SG_PIXELFORMAT_DEPTH:          return DXGI_FORMAT_D16_UNORM;
        case SG_PIXELFORMAT_DEPTHSTENCIL:   return DXGI_FORMAT_D24_UNORM_S8_UINT;
        default:                            return DXGI_FORMAT_UNKNOWN;
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
        case SG_VERTEXFORMAT_SHORT4:    return DXGI_FORMAT_R16G16B16A16_SINT;
        case SG_VERTEXFORMAT_SHORT4N:   return DXGI_FORMAT_R16G16B16A16_SNORM;
        /* FIXME: signed 10-10-10-2 vertex format not supported on d3d11 (only unsigned) */
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

/*-- backend resource structures ---------------------------------------------*/
typedef struct {
    _sg_slot slot;
    int size;
    sg_buffer_type type;
    sg_usage usage;
    uint32_t upd_frame_index;
    ID3D11Buffer* d3d11_buf;
} _sg_buffer;

_SOKOL_PRIVATE void _sg_init_buffer_slot(_sg_buffer* buf) {
    SOKOL_ASSERT(buf);
    memset(buf, 0, sizeof(_sg_buffer));
}

typedef struct {
    _sg_slot slot;
    sg_image_type type;
    bool render_target;
    int width;
    int height;
    int depth;
    int num_mipmaps;
    sg_usage usage;
    sg_pixel_format pixel_format;
    int sample_count;
    sg_filter min_filter;
    sg_filter mag_filter;
    sg_wrap wrap_u;
    sg_wrap wrap_v;
    sg_wrap wrap_w;
    uint32_t max_anisotropy;
    int upd_frame_index;
    DXGI_FORMAT d3d11_format;
    ID3D11Texture2D* d3d11_tex2d;
    ID3D11Texture3D* d3d11_tex3d;
    ID3D11Texture2D* d3d11_texds;
    ID3D11Texture2D* d3d11_texmsaa;
    ID3D11ShaderResourceView* d3d11_srv;
    ID3D11SamplerState* d3d11_smp;
} _sg_image;

_SOKOL_PRIVATE void _sg_init_image_slot(_sg_image* img) {
    SOKOL_ASSERT(img);
    memset(img, 0, sizeof(_sg_image));
}

typedef struct {
    int size;
} _sg_uniform_block;

typedef struct {
    sg_image_type type;
} _sg_shader_image;

typedef struct {
    int num_uniform_blocks;
    int num_images;
    _sg_uniform_block uniform_blocks[SG_MAX_SHADERSTAGE_UBS];
    _sg_shader_image images[SG_MAX_SHADERSTAGE_IMAGES];
    ID3D11Buffer* d3d11_cbs[SG_MAX_SHADERSTAGE_UBS];
} _sg_shader_stage;

typedef struct {
    _sg_slot slot;
    _sg_shader_stage stage[SG_NUM_SHADER_STAGES];
    ID3D11VertexShader* d3d11_vs;
    ID3D11PixelShader* d3d11_fs;
    void* d3d11_vs_blob;
    int d3d11_vs_blob_length;
} _sg_shader;

_SOKOL_PRIVATE void _sg_init_shader_slot(_sg_shader* shd) {
    SOKOL_ASSERT(shd);
    memset(shd, 0, sizeof(_sg_shader));
}

typedef struct {
    _sg_slot slot;
    _sg_shader* shader;
    sg_shader shader_id;
    sg_index_type index_type;
    bool vertex_layout_valid[SG_MAX_SHADERSTAGE_BUFFERS];
    int color_attachment_count;
    sg_pixel_format color_format;
    sg_pixel_format depth_format;
    int sample_count;
    float blend_color[4];
    UINT d3d11_stencil_ref;
    UINT d3d11_vb_strides[SG_MAX_SHADERSTAGE_BUFFERS];
    D3D_PRIMITIVE_TOPOLOGY d3d11_topology;
    DXGI_FORMAT d3d11_index_format;
    ID3D11InputLayout* d3d11_il;
    ID3D11RasterizerState* d3d11_rs;
    ID3D11DepthStencilState* d3d11_dss;
    ID3D11BlendState* d3d11_bs;
} _sg_pipeline;

_SOKOL_PRIVATE void _sg_init_pipeline_slot(_sg_pipeline* pip) {
    SOKOL_ASSERT(pip);
    memset(pip, 0, sizeof(_sg_pipeline));
}

typedef struct {
    _sg_image* image;
    sg_image image_id;
    int mip_level;
    int slice;
} _sg_attachment;

typedef struct {
    _sg_slot slot;
    int num_color_atts;
    _sg_attachment color_atts[SG_MAX_COLOR_ATTACHMENTS];
    _sg_attachment ds_att;
    ID3D11RenderTargetView* d3d11_rtvs[SG_MAX_COLOR_ATTACHMENTS];
    ID3D11DepthStencilView* d3d11_dsv;
} _sg_pass;

_SOKOL_PRIVATE void _sg_init_pass_slot(_sg_pass* pass) {
    SOKOL_ASSERT(pass);
    memset(pass, 0, sizeof(_sg_pass));
} 

typedef struct {
    _sg_slot slot;
} _sg_context;

_SOKOL_PRIVATE void _sg_init_context_slot(_sg_context* context) {
    SOKOL_ASSERT(context);
    memset(context, 0, sizeof(_sg_context));
}

/*-- main D3D11 backend state and functions ----------------------------------*/
typedef struct {
    bool valid;
    ID3D11Device* dev;
    ID3D11DeviceContext* ctx;
    const void* (*rtv_cb)(void);
    const void* (*dsv_cb)(void);
    bool in_pass;
    bool use_indexed_draw;
    int cur_width;
    int cur_height;
    int num_rtvs;
    _sg_pass* cur_pass;
    sg_pass cur_pass_id;
    _sg_pipeline* cur_pipeline;
    sg_pipeline cur_pipeline_id;
    ID3D11RenderTargetView* cur_rtvs[SG_MAX_COLOR_ATTACHMENTS];
    ID3D11DepthStencilView* cur_dsv;
    /* the following arrays are used for unbinding resources, they will always contain zeroes */ 
    ID3D11RenderTargetView* zero_rtvs[SG_MAX_COLOR_ATTACHMENTS];
    ID3D11Buffer* zero_vbs[SG_MAX_SHADERSTAGE_BUFFERS];
    UINT zero_vb_offsets[SG_MAX_SHADERSTAGE_BUFFERS];
    UINT zero_vb_strides[SG_MAX_SHADERSTAGE_BUFFERS];
    ID3D11Buffer* zero_cbs[SG_MAX_SHADERSTAGE_UBS];
    ID3D11ShaderResourceView* zero_srvs[SG_MAX_SHADERSTAGE_IMAGES];
    ID3D11SamplerState* zero_smps[SG_MAX_SHADERSTAGE_IMAGES];
    /* global subresourcedata array for texture updates */
    D3D11_SUBRESOURCE_DATA subres_data[SG_MAX_MIPMAPS * SG_MAX_TEXTUREARRAY_LAYERS];
} _sg_backend;
static _sg_backend _sg_d3d11;

_SOKOL_PRIVATE void _sg_setup_backend(const sg_desc* desc) {
    SOKOL_ASSERT(desc);
    SOKOL_ASSERT(desc->d3d11_device);
    SOKOL_ASSERT(desc->d3d11_device_context);
    SOKOL_ASSERT(desc->d3d11_render_target_view_cb);
    SOKOL_ASSERT(desc->d3d11_depth_stencil_view_cb);
    SOKOL_ASSERT(desc->d3d11_render_target_view_cb != desc->d3d11_depth_stencil_view_cb);
    memset(&_sg_d3d11, 0, sizeof(_sg_d3d11));
    _sg_d3d11.valid = true;
    _sg_d3d11.dev = (ID3D11Device*) desc->d3d11_device;
    _sg_d3d11.ctx = (ID3D11DeviceContext*) desc->d3d11_device_context;
    _sg_d3d11.rtv_cb = desc->d3d11_render_target_view_cb;
    _sg_d3d11.dsv_cb = desc->d3d11_depth_stencil_view_cb;
}

_SOKOL_PRIVATE void _sg_discard_backend() {
    SOKOL_ASSERT(_sg_d3d11.valid);
    memset(&_sg_d3d11, 0, sizeof(_sg_d3d11));
}

_SOKOL_PRIVATE bool _sg_query_feature(sg_feature f) {
    switch (f) {
        case SG_FEATURE_INSTANCING:
        case SG_FEATURE_TEXTURE_COMPRESSION_DXT:
        case SG_FEATURE_TEXTURE_FLOAT:
        case SG_FEATURE_TEXTURE_HALF_FLOAT:
        case SG_FEATURE_ORIGIN_TOP_LEFT:
        case SG_FEATURE_MSAA_RENDER_TARGETS:
        case SG_FEATURE_MULTIPLE_RENDER_TARGET:
        case SG_FEATURE_IMAGETYPE_3D:
        case SG_FEATURE_IMAGETYPE_ARRAY:
            return true;
        default:
            return false;
    }
}

_SOKOL_PRIVATE void _sg_d3d11_clear_state() {
    /* clear all the device context state, so that resource refs don't keep stuck in the d3d device context */
    ID3D11DeviceContext_OMSetRenderTargets(_sg_d3d11.ctx, SG_MAX_COLOR_ATTACHMENTS, _sg_d3d11.zero_rtvs, NULL);
    ID3D11DeviceContext_RSSetState(_sg_d3d11.ctx, NULL);
    ID3D11DeviceContext_OMSetDepthStencilState(_sg_d3d11.ctx, NULL, 0);
    ID3D11DeviceContext_OMSetBlendState(_sg_d3d11.ctx, NULL, NULL, 0xFFFFFFFF);
    ID3D11DeviceContext_IASetVertexBuffers(_sg_d3d11.ctx, 0, SG_MAX_SHADERSTAGE_BUFFERS, _sg_d3d11.zero_vbs, _sg_d3d11.zero_vb_strides, _sg_d3d11.zero_vb_offsets);
    ID3D11DeviceContext_IASetIndexBuffer(_sg_d3d11.ctx, NULL, DXGI_FORMAT_UNKNOWN, 0); 
    ID3D11DeviceContext_IASetInputLayout(_sg_d3d11.ctx, NULL);
    ID3D11DeviceContext_VSSetShader(_sg_d3d11.ctx, NULL, NULL, 0);
    ID3D11DeviceContext_PSSetShader(_sg_d3d11.ctx, NULL, NULL, 0);
    ID3D11DeviceContext_VSSetConstantBuffers(_sg_d3d11.ctx, 0, SG_MAX_SHADERSTAGE_UBS, _sg_d3d11.zero_cbs);
    ID3D11DeviceContext_PSSetConstantBuffers(_sg_d3d11.ctx, 0, SG_MAX_SHADERSTAGE_UBS, _sg_d3d11.zero_cbs);
    ID3D11DeviceContext_VSSetShaderResources(_sg_d3d11.ctx, 0, SG_MAX_SHADERSTAGE_IMAGES, _sg_d3d11.zero_srvs);
    ID3D11DeviceContext_PSSetShaderResources(_sg_d3d11.ctx, 0, SG_MAX_SHADERSTAGE_IMAGES, _sg_d3d11.zero_srvs);
    ID3D11DeviceContext_VSSetSamplers(_sg_d3d11.ctx, 0, SG_MAX_SHADERSTAGE_IMAGES, _sg_d3d11.zero_smps);
    ID3D11DeviceContext_PSSetSamplers(_sg_d3d11.ctx, 0, SG_MAX_SHADERSTAGE_IMAGES, _sg_d3d11.zero_smps);
}

_SOKOL_PRIVATE void _sg_reset_state_cache() {
    /* just clear the d3d11 device context state */
    _sg_d3d11_clear_state();
}

_SOKOL_PRIVATE void _sg_activate_context(_sg_context* ctx) {
    _sg_reset_state_cache();
}

_SOKOL_PRIVATE void _sg_create_context(_sg_context* ctx) {
    SOKOL_ASSERT(ctx);
    SOKOL_ASSERT(ctx->slot.state == SG_RESOURCESTATE_ALLOC);
    ctx->slot.state = SG_RESOURCESTATE_VALID;
}

_SOKOL_PRIVATE void _sg_destroy_context(_sg_context* ctx) {
    SOKOL_ASSERT(ctx);
    _sg_init_context_slot(ctx);
}

_SOKOL_PRIVATE void _sg_create_buffer(_sg_buffer* buf, const sg_buffer_desc* desc) {
    SOKOL_ASSERT(buf && desc);
    SOKOL_ASSERT(buf->slot.state == SG_RESOURCESTATE_ALLOC);
    SOKOL_ASSERT(!buf->d3d11_buf);
    buf->size = desc->size;
    buf->type = _sg_def(desc->type, SG_BUFFERTYPE_VERTEXBUFFER);
    buf->usage = _sg_def(desc->usage, SG_USAGE_IMMUTABLE);
    buf->upd_frame_index = 0;
    const bool injected = (0 != desc->d3d11_buffer);
    if (injected) {
        buf->d3d11_buf = (ID3D11Buffer*) desc->d3d11_buffer;
        ID3D11Buffer_AddRef(buf->d3d11_buf);
    }
    else {
        D3D11_BUFFER_DESC d3d11_desc;
        memset(&d3d11_desc, 0, sizeof(d3d11_desc));
        d3d11_desc.ByteWidth = buf->size;
        d3d11_desc.Usage = _sg_d3d11_usage(buf->usage);
        d3d11_desc.BindFlags = buf->type == SG_BUFFERTYPE_VERTEXBUFFER ? D3D11_BIND_VERTEX_BUFFER : D3D11_BIND_INDEX_BUFFER;
        d3d11_desc.CPUAccessFlags = _sg_d3d11_cpu_access_flags(buf->usage);
        D3D11_SUBRESOURCE_DATA* init_data_ptr = 0;
        D3D11_SUBRESOURCE_DATA init_data;
        memset(&init_data, 0, sizeof(init_data));
        if (buf->usage == SG_USAGE_IMMUTABLE) {
            SOKOL_ASSERT(desc->content);
            init_data.pSysMem = desc->content;
            init_data_ptr = &init_data;
        }
        HRESULT hr = ID3D11Device_CreateBuffer(_sg_d3d11.dev, &d3d11_desc, init_data_ptr, &buf->d3d11_buf);
        SOKOL_ASSERT(SUCCEEDED(hr) && buf->d3d11_buf);
    }
    buf->slot.state = SG_RESOURCESTATE_VALID;
}

_SOKOL_PRIVATE void _sg_destroy_buffer(_sg_buffer* buf) {
    SOKOL_ASSERT(buf);
    if (buf->d3d11_buf) {
        ID3D11Buffer_Release(buf->d3d11_buf);
    }
    _sg_init_buffer_slot(buf);
}

_SOKOL_PRIVATE void _sg_d3d11_fill_subres_data(const _sg_image* img, const sg_image_content* content) {
    const int num_faces = (img->type == SG_IMAGETYPE_CUBE) ? 6:1;
    const int num_slices = (img->type == SG_IMAGETYPE_ARRAY) ? img->depth:1;
    int subres_index = 0;
    for (int face_index = 0; face_index < num_faces; face_index++) {
        for (int slice_index = 0; slice_index < num_slices; slice_index++) {
            for (int mip_index = 0; mip_index < img->num_mipmaps; mip_index++, subres_index++) {
                SOKOL_ASSERT(subres_index < (SG_MAX_MIPMAPS * SG_MAX_TEXTUREARRAY_LAYERS));
                D3D11_SUBRESOURCE_DATA* subres_data = &_sg_d3d11.subres_data[subres_index];
                const int mip_width = ((img->width>>mip_index)>0) ? img->width>>mip_index : 1;
                const int mip_height = ((img->height>>mip_index)>0) ? img->height>>mip_index : 1;
                const sg_subimage_content* subimg_content = &(content->subimage[face_index][mip_index]);
                const int slice_size = subimg_content->size / num_slices;
                const int slice_offset = slice_size * slice_index;
                const uint8_t* ptr = (const uint8_t*) subimg_content->ptr;
                subres_data->pSysMem = ptr + slice_offset;
                subres_data->SysMemPitch = _sg_row_pitch(img->pixel_format, mip_width);
                if (img->type == SG_IMAGETYPE_3D) {
                    const int mip_depth = ((img->depth>>mip_index)>0) ? img->depth>>mip_index : 1;
                    subres_data->SysMemSlicePitch = _sg_surface_pitch(img->pixel_format, mip_width, mip_height);
                }
                else {
                    subres_data->SysMemSlicePitch = 0;
                }
            }
        }
    }
}

_SOKOL_PRIVATE void _sg_create_image(_sg_image* img, const sg_image_desc* desc) {
    SOKOL_ASSERT(img && desc);
    SOKOL_ASSERT(img->slot.state == SG_RESOURCESTATE_ALLOC);
    SOKOL_ASSERT(!img->d3d11_tex2d && !img->d3d11_tex3d && !img->d3d11_texds && !img->d3d11_texmsaa);
    SOKOL_ASSERT(!img->d3d11_srv && !img->d3d11_smp);
    HRESULT hr;
    
    img->type = _sg_def(desc->type, SG_IMAGETYPE_2D);
    img->render_target = desc->render_target;
    img->width = desc->width;
    img->height = desc->height;
    img->depth = _sg_def(desc->depth, 1);
    img->num_mipmaps = _sg_def(desc->num_mipmaps, 1);
    img->usage = _sg_def(desc->usage, SG_USAGE_IMMUTABLE);
    img->pixel_format = _sg_def(desc->pixel_format, SG_PIXELFORMAT_RGBA8);
    img->sample_count = _sg_def(desc->sample_count, 1);
    img->min_filter = _sg_def(desc->min_filter, SG_FILTER_NEAREST);
    img->mag_filter = _sg_def(desc->mag_filter, SG_FILTER_NEAREST);
    img->wrap_u = _sg_def(desc->wrap_u, SG_WRAP_REPEAT);
    img->wrap_v = _sg_def(desc->wrap_v, SG_WRAP_REPEAT);
    img->wrap_w = _sg_def(desc->wrap_w, SG_WRAP_REPEAT);
    img->max_anisotropy = _sg_def(desc->max_anisotropy, 1);
    img->upd_frame_index = 0;
    const bool injected = (0 != desc->d3d11_texture);

    /* special case depth-stencil buffer? */
    if (_sg_is_valid_rendertarget_depth_format(img->pixel_format)) {
        /* create only a depth-texture */
        SOKOL_ASSERT(!injected);
        img->d3d11_format = _sg_d3d11_rendertarget_depth_format(img->pixel_format);
        if (img->d3d11_format == DXGI_FORMAT_UNKNOWN) {
            /* trying to create a texture format that's not supported by D3D */
            SOKOL_LOG("trying to create a D3D11 depth-texture with unsupported pixel format\n");
            img->slot.state = SG_RESOURCESTATE_FAILED;
            return;
        }
        D3D11_TEXTURE2D_DESC d3d11_desc;
        memset(&d3d11_desc, 0, sizeof(d3d11_desc));
        d3d11_desc.Width = img->width;
        d3d11_desc.Height = img->height;
        d3d11_desc.MipLevels = 1;
        d3d11_desc.ArraySize = 1;
        d3d11_desc.Format = img->d3d11_format;
        d3d11_desc.Usage = D3D11_USAGE_DEFAULT;
        d3d11_desc.BindFlags = D3D11_BIND_DEPTH_STENCIL;
        d3d11_desc.SampleDesc.Count = img->sample_count;
        d3d11_desc.SampleDesc.Quality = (img->sample_count > 1) ? D3D11_STANDARD_MULTISAMPLE_PATTERN : 0;
        hr = ID3D11Device_CreateTexture2D(_sg_d3d11.dev, &d3d11_desc, NULL, &img->d3d11_texds);
        SOKOL_ASSERT(SUCCEEDED(hr) && img->d3d11_texds);
    }
    else {
        /* create (or inject) color texture */

        /* prepare initial content pointers */
        D3D11_SUBRESOURCE_DATA* init_data = 0;
        if (!injected && (img->usage == SG_USAGE_IMMUTABLE) && !img->render_target) {
            _sg_d3d11_fill_subres_data(img, &desc->content);
            init_data = _sg_d3d11.subres_data;
        }
        if (img->type != SG_IMAGETYPE_3D) {
            /* 2D-, cube- or array-texture */
            /* if this is an MSAA render target, the following texture will be the 'resolve-texture' */
            D3D11_TEXTURE2D_DESC d3d11_tex_desc;
            memset(&d3d11_tex_desc, 0, sizeof(d3d11_tex_desc));
            d3d11_tex_desc.Width = img->width;
            d3d11_tex_desc.Height = img->height;
            d3d11_tex_desc.MipLevels = img->num_mipmaps;
            switch (img->type) {
                case SG_IMAGETYPE_ARRAY:    d3d11_tex_desc.ArraySize = img->depth; break;
                case SG_IMAGETYPE_CUBE:     d3d11_tex_desc.ArraySize = 6; break;
                default:                    d3d11_tex_desc.ArraySize = 1; break;
            }
            d3d11_tex_desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
            if (img->render_target) {
                img->d3d11_format = _sg_d3d11_rendertarget_color_format(img->pixel_format);
                d3d11_tex_desc.Format = img->d3d11_format;
                d3d11_tex_desc.Usage = D3D11_USAGE_DEFAULT;
                if (img->sample_count == 1) {
                    d3d11_tex_desc.BindFlags |= D3D11_BIND_RENDER_TARGET;
                }
                d3d11_tex_desc.CPUAccessFlags = 0;
            }
            else {
                img->d3d11_format = _sg_d3d11_texture_format(img->pixel_format);
                d3d11_tex_desc.Format = img->d3d11_format;
                d3d11_tex_desc.Usage = _sg_d3d11_usage(img->usage);
                d3d11_tex_desc.CPUAccessFlags = _sg_d3d11_cpu_access_flags(img->usage);
            }
            if (img->d3d11_format == DXGI_FORMAT_UNKNOWN) {
                /* trying to create a texture format that's not supported by D3D */
                SOKOL_LOG("trying to create a D3D11 texture with unsupported pixel format\n");
                img->slot.state = SG_RESOURCESTATE_FAILED;
                return;
            }
            d3d11_tex_desc.SampleDesc.Count = 1;
            d3d11_tex_desc.SampleDesc.Quality = 0;
            d3d11_tex_desc.MiscFlags = (img->type == SG_IMAGETYPE_CUBE) ? D3D11_RESOURCE_MISC_TEXTURECUBE : 0;
            if (injected) {
                img->d3d11_tex2d = (ID3D11Texture2D*) desc->d3d11_texture;
                ID3D11Texture2D_AddRef(img->d3d11_tex2d);
            }
            else {
                hr = ID3D11Device_CreateTexture2D(_sg_d3d11.dev, &d3d11_tex_desc, init_data, &img->d3d11_tex2d);
                SOKOL_ASSERT(SUCCEEDED(hr) && img->d3d11_tex2d);
            }

            /* also need to create a separate MSAA render target texture? */
            if (img->sample_count > 1) {
                d3d11_tex_desc.BindFlags |= D3D11_BIND_RENDER_TARGET;
                d3d11_tex_desc.SampleDesc.Count = img->sample_count;
                d3d11_tex_desc.SampleDesc.Quality = D3D11_STANDARD_MULTISAMPLE_PATTERN;
                hr = ID3D11Device_CreateTexture2D(_sg_d3d11.dev, &d3d11_tex_desc, NULL, &img->d3d11_texmsaa);
                SOKOL_ASSERT(SUCCEEDED(hr) && img->d3d11_texmsaa);
            }

            /* shader-resource-view */
            D3D11_SHADER_RESOURCE_VIEW_DESC d3d11_srv_desc;
            memset(&d3d11_srv_desc, 0, sizeof(d3d11_srv_desc));
            d3d11_srv_desc.Format = d3d11_tex_desc.Format;
            switch (img->type) {
                case SG_IMAGETYPE_2D:
                    d3d11_srv_desc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
                    d3d11_srv_desc.Texture2D.MipLevels = img->num_mipmaps;
                    break;
                case SG_IMAGETYPE_CUBE:
                    d3d11_srv_desc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURECUBE;
                    d3d11_srv_desc.TextureCube.MipLevels = img->num_mipmaps;
                    break;
                case SG_IMAGETYPE_ARRAY:
                    d3d11_srv_desc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2DARRAY;
                    d3d11_srv_desc.Texture2DArray.MipLevels = img->num_mipmaps;
                    d3d11_srv_desc.Texture2DArray.ArraySize = img->depth;
                    break;
                default:
                    SOKOL_UNREACHABLE; break;
            }
            hr = ID3D11Device_CreateShaderResourceView(_sg_d3d11.dev, (ID3D11Resource*)img->d3d11_tex2d, &d3d11_srv_desc, &img->d3d11_srv);
            SOKOL_ASSERT(SUCCEEDED(hr) && img->d3d11_srv);
        }
        else {
            /* 3D texture */
            D3D11_TEXTURE3D_DESC d3d11_tex_desc;
            memset(&d3d11_tex_desc, 0, sizeof(d3d11_tex_desc));
            d3d11_tex_desc.Width = img->width;
            d3d11_tex_desc.Height = img->height;
            d3d11_tex_desc.Depth = img->depth;
            d3d11_tex_desc.MipLevels = img->num_mipmaps;
            if (img->render_target) {
                img->d3d11_format = _sg_d3d11_rendertarget_color_format(img->pixel_format);
                d3d11_tex_desc.Format = img->d3d11_format;
                d3d11_tex_desc.Usage = D3D11_USAGE_DEFAULT;
                d3d11_tex_desc.BindFlags = D3D11_BIND_SHADER_RESOURCE|D3D11_BIND_RENDER_TARGET;
                d3d11_tex_desc.CPUAccessFlags = 0;
            }
            else {
                img->d3d11_format = _sg_d3d11_texture_format(img->pixel_format);
                d3d11_tex_desc.Format = img->d3d11_format;
                d3d11_tex_desc.Usage = _sg_d3d11_usage(img->usage);
                d3d11_tex_desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
                d3d11_tex_desc.CPUAccessFlags = _sg_d3d11_cpu_access_flags(img->usage);
            }
            if (img->d3d11_format == DXGI_FORMAT_UNKNOWN) {
                /* trying to create a texture format that's not supported by D3D */
                SOKOL_LOG("trying to create a D3D11 texture with unsupported pixel format\n");
                img->slot.state = SG_RESOURCESTATE_FAILED;
                return;
            }
            if (injected) {
                img->d3d11_tex3d = (ID3D11Texture3D*) desc->d3d11_texture;
                ID3D11Texture3D_AddRef(img->d3d11_tex3d);
            }
            else {
                hr = ID3D11Device_CreateTexture3D(_sg_d3d11.dev, &d3d11_tex_desc, init_data, &img->d3d11_tex3d);
                SOKOL_ASSERT(SUCCEEDED(hr) && img->d3d11_tex3d);
            }

            /* shader resource view for 3d texture */
            D3D11_SHADER_RESOURCE_VIEW_DESC d3d11_srv_desc;
            memset(&d3d11_srv_desc, 0, sizeof(d3d11_srv_desc));
            d3d11_srv_desc.Format = d3d11_tex_desc.Format;
            d3d11_srv_desc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE3D;
            d3d11_srv_desc.Texture3D.MipLevels = img->num_mipmaps;
            hr = ID3D11Device_CreateShaderResourceView(_sg_d3d11.dev, (ID3D11Resource*)img->d3d11_tex3d, &d3d11_srv_desc, &img->d3d11_srv);
            SOKOL_ASSERT(SUCCEEDED(hr) && img->d3d11_srv);
        }

        /* sampler state object, note D3D11 implements an internal shared-pool for sampler objects */
        D3D11_SAMPLER_DESC d3d11_smp_desc;
        memset(&d3d11_smp_desc, 0, sizeof(d3d11_smp_desc));
        d3d11_smp_desc.Filter = _sg_d3d11_filter(img->min_filter, img->mag_filter, img->max_anisotropy);
        d3d11_smp_desc.AddressU = _sg_d3d11_address_mode(img->wrap_u);
        d3d11_smp_desc.AddressV = _sg_d3d11_address_mode(img->wrap_v);
        d3d11_smp_desc.AddressW = _sg_d3d11_address_mode(img->wrap_w);
        d3d11_smp_desc.MaxAnisotropy = img->max_anisotropy;
        d3d11_smp_desc.ComparisonFunc = D3D11_COMPARISON_NEVER;
        d3d11_smp_desc.MinLOD = desc->min_lod;
        d3d11_smp_desc.MaxLOD = _sg_def_flt(desc->max_lod, D3D11_FLOAT32_MAX);
        hr = ID3D11Device_CreateSamplerState(_sg_d3d11.dev, &d3d11_smp_desc, &img->d3d11_smp);
        SOKOL_ASSERT(SUCCEEDED(hr) && img->d3d11_smp);
    }
    img->slot.state = SG_RESOURCESTATE_VALID;
}

_SOKOL_PRIVATE void _sg_destroy_image(_sg_image* img) {
    SOKOL_ASSERT(img);
    if (img->d3d11_tex2d) {
        ID3D11Texture2D_Release(img->d3d11_tex2d);
    }
    if (img->d3d11_tex3d) {
        ID3D11Texture3D_Release(img->d3d11_tex3d);
    }
    if (img->d3d11_texds) {
        ID3D11Texture2D_Release(img->d3d11_texds);
    }
    if (img->d3d11_texmsaa) {
        ID3D11Texture2D_Release(img->d3d11_texmsaa);
    }
    if (img->d3d11_srv) {
        ID3D11ShaderResourceView_Release(img->d3d11_srv);
    }
    if (img->d3d11_smp) {
        ID3D11SamplerState_Release(img->d3d11_smp);
    }
    _sg_init_image_slot(img);
}

#if defined(SOKOL_D3D11_SHADER_COMPILER)
_SOKOL_PRIVATE ID3DBlob* _sg_d3d11_compile_shader(const sg_shader_stage_desc* stage_desc, const char* target) {
    ID3DBlob* output = NULL;
    ID3DBlob* errors = NULL;
    HRESULT hr = D3DCompile(
        stage_desc->source,             /* pSrcData */
        strlen(stage_desc->source),     /* SrcDataSize */
        NULL,                           /* pSourceName */
        NULL,                           /* pDefines */
        NULL,                           /* pInclude */
        stage_desc->entry ? stage_desc->entry : "main",     /* pEntryPoint */
        target,     /* pTarget (vs_5_0 or ps_5_0) */
        D3DCOMPILE_PACK_MATRIX_COLUMN_MAJOR | D3DCOMPILE_OPTIMIZATION_LEVEL3,   /* Flags1 */
        0,          /* Flags2 */
        &output,    /* ppCode */
        &errors);   /* ppErrorMsgs */
    if (errors) {
        SOKOL_LOG((LPCSTR)ID3D10Blob_GetBufferPointer(errors));
        ID3D10Blob_Release(errors); errors = NULL;
    }
    return output;
}
#endif

#define _sg_d3d11_roundup(val, round_to) (((val)+((round_to)-1))&~((round_to)-1))

_SOKOL_PRIVATE void _sg_create_shader(_sg_shader* shd, const sg_shader_desc* desc) {
    SOKOL_ASSERT(shd && desc);
    SOKOL_ASSERT(shd->slot.state == SG_RESOURCESTATE_ALLOC);
    SOKOL_ASSERT(!shd->d3d11_vs && !shd->d3d11_fs && !shd->d3d11_vs_blob);
    HRESULT hr;

    /* shader stage uniform blocks and image slots */
    for (int stage_index = 0; stage_index < SG_NUM_SHADER_STAGES; stage_index++) {
        const sg_shader_stage_desc* stage_desc = (stage_index == SG_SHADERSTAGE_VS) ? &desc->vs : &desc->fs;
        _sg_shader_stage* stage = &shd->stage[stage_index];
        SOKOL_ASSERT(stage->num_uniform_blocks == 0);
        for (int ub_index = 0; ub_index < SG_MAX_SHADERSTAGE_UBS; ub_index++) {
            const sg_shader_uniform_block_desc* ub_desc = &stage_desc->uniform_blocks[ub_index];
            if (0 == ub_desc->size) {
                break;
            }
            _sg_uniform_block* ub = &stage->uniform_blocks[ub_index];
            ub->size = ub_desc->size;

            /* create a D3D constant buffer */
            SOKOL_ASSERT(!stage->d3d11_cbs[ub_index]);
            D3D11_BUFFER_DESC cb_desc;
            memset(&cb_desc, 0, sizeof(cb_desc));
            cb_desc.ByteWidth = _sg_d3d11_roundup(ub->size, 16);
            cb_desc.Usage = D3D11_USAGE_DEFAULT;
            cb_desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
            hr = ID3D11Device_CreateBuffer(_sg_d3d11.dev, &cb_desc, NULL, &stage->d3d11_cbs[ub_index]);
            SOKOL_ASSERT(SUCCEEDED(hr) && stage->d3d11_cbs[ub_index]);

            stage->num_uniform_blocks++;
        }
        SOKOL_ASSERT(stage->num_images == 0);
        for (int img_index = 0; img_index < SG_MAX_SHADERSTAGE_IMAGES; img_index++) {
            const sg_shader_image_desc* img_desc = &stage_desc->images[img_index];
            if (img_desc->type == _SG_IMAGETYPE_DEFAULT) {
                break;
            }
            stage->images[img_index].type = img_desc->type;
            stage->num_images++;
        }
    }

    const void* vs_ptr = 0, *fs_ptr = 0;
    SIZE_T vs_length = 0, fs_length = 0;
    #if defined(SOKOL_D3D11_SHADER_COMPILER)
    ID3DBlob* vs_blob = 0, *fs_blob = 0;
    #endif
    if (desc->vs.byte_code && desc->fs.byte_code) {
        /* create from byte code */
        vs_ptr = desc->vs.byte_code;
        fs_ptr = desc->fs.byte_code;
        vs_length = desc->vs.byte_code_size;
        fs_length = desc->fs.byte_code_size;
    }
    else {
        /* compile shader code */
        #if defined(SOKOL_D3D11_SHADER_COMPILER)
        vs_blob = _sg_d3d11_compile_shader(&desc->vs, "vs_5_0");
        fs_blob = _sg_d3d11_compile_shader(&desc->fs, "ps_5_0");
        if (vs_blob && fs_blob) {
            vs_ptr = ID3D10Blob_GetBufferPointer(vs_blob);
            vs_length = ID3D10Blob_GetBufferSize(vs_blob);
            fs_ptr = ID3D10Blob_GetBufferPointer(fs_blob);
            fs_length = ID3D10Blob_GetBufferSize(fs_blob);
        }
        #endif
    }
    if (vs_ptr && fs_ptr && (vs_length > 0) && (fs_length > 0)) {
        /* create the D3D vertex- and pixel-shader objects */
        hr = ID3D11Device_CreateVertexShader(_sg_d3d11.dev, vs_ptr, vs_length, NULL, &shd->d3d11_vs);
        SOKOL_ASSERT(SUCCEEDED(hr) && shd->d3d11_vs);
        hr = ID3D11Device_CreatePixelShader(_sg_d3d11.dev, fs_ptr, fs_length, NULL, &shd->d3d11_fs);
        SOKOL_ASSERT(SUCCEEDED(hr) && shd->d3d11_fs);

        /* need to store the vertex shader byte code, this is needed later in sg_create_pipeline */
        shd->d3d11_vs_blob_length = (int)vs_length;
        shd->d3d11_vs_blob = SOKOL_MALLOC((int)vs_length);
        SOKOL_ASSERT(shd->d3d11_vs_blob);
        memcpy(shd->d3d11_vs_blob, vs_ptr, vs_length);

        shd->slot.state = SG_RESOURCESTATE_VALID;
    }
    else {
        shd->slot.state = SG_RESOURCESTATE_FAILED;
    }
    #if defined(SOKOL_D3D11_SHADER_COMPILER)
    if (vs_blob) {
        ID3D10Blob_Release(vs_blob); vs_blob = 0;
    }
    if (fs_blob) {
        ID3D10Blob_Release(fs_blob); fs_blob = 0;
    }
    #endif
}

_SOKOL_PRIVATE void _sg_destroy_shader(_sg_shader* shd) {
    SOKOL_ASSERT(shd);
    if (shd->d3d11_vs) {
        ID3D11VertexShader_Release(shd->d3d11_vs);
    }
    if (shd->d3d11_fs) {
        ID3D11PixelShader_Release(shd->d3d11_fs);
    }
    if (shd->d3d11_vs_blob) {
        SOKOL_FREE(shd->d3d11_vs_blob);
    }
    for (int stage_index = 0; stage_index < SG_NUM_SHADER_STAGES; stage_index++) {
        _sg_shader_stage* stage = &shd->stage[stage_index];
        for (int ub_index = 0; ub_index < stage->num_uniform_blocks; ub_index++) {
            if (stage->d3d11_cbs[ub_index]) {
                ID3D11Buffer_Release(stage->d3d11_cbs[ub_index]);
            }
        }
    }
    _sg_init_shader_slot(shd);
}

_SOKOL_PRIVATE void _sg_create_pipeline(_sg_pipeline* pip, _sg_shader* shd, const sg_pipeline_desc* desc) {
    SOKOL_ASSERT(pip && shd && desc);
    SOKOL_ASSERT(pip->slot.state == SG_RESOURCESTATE_ALLOC);
    SOKOL_ASSERT(desc->shader.id == shd->slot.id);
    SOKOL_ASSERT(shd->slot.state == SG_RESOURCESTATE_VALID);
    SOKOL_ASSERT(shd->d3d11_vs_blob && shd->d3d11_vs_blob_length > 0);
    SOKOL_ASSERT(!pip->d3d11_il && !pip->d3d11_rs && !pip->d3d11_dss && !pip->d3d11_bs);
    HRESULT hr;

    pip->shader = shd;
    pip->shader_id = desc->shader;
    pip->index_type = _sg_def(desc->index_type, SG_INDEXTYPE_NONE);
    pip->color_attachment_count = _sg_def(desc->blend.color_attachment_count, 1);
    pip->color_format = _sg_def(desc->blend.color_format, SG_PIXELFORMAT_RGBA8);
    pip->depth_format = _sg_def(desc->blend.depth_format, SG_PIXELFORMAT_DEPTHSTENCIL);
    pip->sample_count = _sg_def(desc->rasterizer.sample_count, 1);
    pip->d3d11_index_format = _sg_d3d11_index_format(pip->index_type);
    pip->d3d11_topology = _sg_d3d11_primitive_topology(_sg_def(desc->primitive_type, SG_PRIMITIVETYPE_TRIANGLES));
    for (int i = 0; i < 4; i++) {
        pip->blend_color[i] = desc->blend.blend_color[i];
    }
    pip->d3d11_stencil_ref = desc->depth_stencil.stencil_ref;

    /* create input layout object */
    int auto_offset[SG_MAX_SHADERSTAGE_BUFFERS];
    for (int layout_index = 0; layout_index < SG_MAX_SHADERSTAGE_BUFFERS; layout_index++) {
        auto_offset[layout_index] = 0;
    }
    bool use_auto_offset = true;
    for (int attr_index = 0; attr_index < SG_MAX_VERTEX_ATTRIBUTES; attr_index++) {
        /* to use computed offsets, all attr offsets must be 0 */
        if (desc->layout.attrs[attr_index].offset != 0) {
            use_auto_offset = false;
        }
    }
    D3D11_INPUT_ELEMENT_DESC d3d11_comps[SG_MAX_VERTEX_ATTRIBUTES];
    memset(d3d11_comps, 0, sizeof(d3d11_comps));
    int attr_index = 0;
    for (; attr_index < SG_MAX_VERTEX_ATTRIBUTES; attr_index++) {
        const sg_vertex_attr_desc* a_desc = &desc->layout.attrs[attr_index];
        if (a_desc->format == SG_VERTEXFORMAT_INVALID) {
            break;
        }
        SOKOL_ASSERT((a_desc->buffer_index >= 0) && (a_desc->buffer_index < SG_MAX_SHADERSTAGE_BUFFERS));
        const sg_buffer_layout_desc* l_desc = &desc->layout.buffers[a_desc->buffer_index];
        const sg_vertex_step step_func = _sg_def(l_desc->step_func, SG_VERTEXSTEP_PER_VERTEX);
        const int step_rate = _sg_def(l_desc->step_rate, 1);
        D3D11_INPUT_ELEMENT_DESC* d3d11_comp = &d3d11_comps[attr_index];
        d3d11_comp->SemanticName = a_desc->sem_name;
        d3d11_comp->SemanticIndex = a_desc->sem_index;
        d3d11_comp->Format = _sg_d3d11_vertex_format(a_desc->format);
        d3d11_comp->InputSlot = a_desc->buffer_index;
        d3d11_comp->AlignedByteOffset = use_auto_offset ? auto_offset[a_desc->buffer_index] : a_desc->offset;
        d3d11_comp->InputSlotClass = _sg_d3d11_input_classification(step_func);
        if (SG_VERTEXSTEP_PER_INSTANCE == step_func) {
            d3d11_comp->InstanceDataStepRate = step_rate;
        }
        auto_offset[a_desc->buffer_index] += _sg_vertexformat_bytesize(a_desc->format);
        pip->vertex_layout_valid[a_desc->buffer_index] = true;
    }
    for (int layout_index = 0; layout_index < SG_MAX_SHADERSTAGE_BUFFERS; layout_index++) {
        if (pip->vertex_layout_valid[layout_index]) {
            const sg_buffer_layout_desc* l_desc = &desc->layout.buffers[layout_index];
            const int stride = l_desc->stride ? l_desc->stride : auto_offset[layout_index];
            SOKOL_ASSERT(stride > 0);
            pip->d3d11_vb_strides[layout_index] = stride;
        }
        else {
            pip->d3d11_vb_strides[layout_index] = 0;
        }
    }
    hr = ID3D11Device_CreateInputLayout(_sg_d3d11.dev,
        d3d11_comps,                /* pInputElementDesc */
        attr_index,                 /* NumElements */
        shd->d3d11_vs_blob,         /* pShaderByteCodeWithInputSignature */
        shd->d3d11_vs_blob_length,  /* BytecodeLength */
        &pip->d3d11_il);
    SOKOL_ASSERT(SUCCEEDED(hr) && pip->d3d11_il);

    /* create rasterizer state */
    D3D11_RASTERIZER_DESC rs_desc;
    memset(&rs_desc, 0, sizeof(rs_desc));
    rs_desc.FillMode = D3D11_FILL_SOLID;
    rs_desc.CullMode = _sg_d3d11_cull_mode(_sg_def(desc->rasterizer.cull_mode, SG_CULLMODE_NONE));
    rs_desc.FrontCounterClockwise = _sg_def(desc->rasterizer.face_winding, SG_FACEWINDING_CW) == SG_FACEWINDING_CCW;
    rs_desc.DepthBias = (INT) desc->rasterizer.depth_bias;
    rs_desc.DepthBiasClamp = desc->rasterizer.depth_bias_clamp;
    rs_desc.SlopeScaledDepthBias = desc->rasterizer.depth_bias_slope_scale;
    rs_desc.DepthClipEnable = TRUE;
    rs_desc.ScissorEnable = TRUE;
    rs_desc.MultisampleEnable = _sg_def(desc->rasterizer.sample_count, 1) > 1;
    rs_desc.AntialiasedLineEnable = FALSE;
    hr = ID3D11Device_CreateRasterizerState(_sg_d3d11.dev, &rs_desc, &pip->d3d11_rs);
    SOKOL_ASSERT(SUCCEEDED(hr) && pip->d3d11_rs);

    /* create depth-stencil state */
    D3D11_DEPTH_STENCIL_DESC dss_desc;
    memset(&dss_desc, 0, sizeof(dss_desc));
    dss_desc.DepthEnable = TRUE;
    dss_desc.DepthWriteMask = desc->depth_stencil.depth_write_enabled ? D3D11_DEPTH_WRITE_MASK_ALL : D3D11_DEPTH_WRITE_MASK_ZERO;
    dss_desc.DepthFunc = _sg_d3d11_compare_func(_sg_def(desc->depth_stencil.depth_compare_func, SG_COMPAREFUNC_ALWAYS));
    dss_desc.StencilEnable = desc->depth_stencil.stencil_enabled;
    dss_desc.StencilReadMask = desc->depth_stencil.stencil_read_mask;
    dss_desc.StencilWriteMask = desc->depth_stencil.stencil_write_mask;
    const sg_stencil_state* sf = &desc->depth_stencil.stencil_front;
    dss_desc.FrontFace.StencilFailOp = _sg_d3d11_stencil_op(_sg_def(sf->fail_op, SG_STENCILOP_KEEP));
    dss_desc.FrontFace.StencilDepthFailOp = _sg_d3d11_stencil_op(_sg_def(sf->depth_fail_op, SG_STENCILOP_KEEP));
    dss_desc.FrontFace.StencilPassOp = _sg_d3d11_stencil_op(_sg_def(sf->pass_op, SG_STENCILOP_KEEP));
    dss_desc.FrontFace.StencilFunc = _sg_d3d11_compare_func(_sg_def(sf->compare_func, SG_COMPAREFUNC_ALWAYS));
    const sg_stencil_state* sb = &desc->depth_stencil.stencil_back;
    dss_desc.BackFace.StencilFailOp = _sg_d3d11_stencil_op(_sg_def(sb->fail_op, SG_STENCILOP_KEEP));
    dss_desc.BackFace.StencilDepthFailOp = _sg_d3d11_stencil_op(_sg_def(sb->depth_fail_op, SG_STENCILOP_KEEP));
    dss_desc.BackFace.StencilPassOp = _sg_d3d11_stencil_op(_sg_def(sb->pass_op, SG_STENCILOP_KEEP));
    dss_desc.BackFace.StencilFunc = _sg_d3d11_compare_func(_sg_def(sb->compare_func, SG_COMPAREFUNC_ALWAYS));
    hr = ID3D11Device_CreateDepthStencilState(_sg_d3d11.dev, &dss_desc, &pip->d3d11_dss);
    SOKOL_ASSERT(SUCCEEDED(hr) && pip->d3d11_dss);
    
    /* create blend state */
    D3D11_BLEND_DESC bs_desc;
    memset(&bs_desc, 0, sizeof(bs_desc));
    bs_desc.AlphaToCoverageEnable = desc->rasterizer.alpha_to_coverage_enabled;
    bs_desc.IndependentBlendEnable = FALSE;
    bs_desc.RenderTarget[0].BlendEnable = desc->blend.enabled;
    bs_desc.RenderTarget[0].SrcBlend = _sg_d3d11_blend_factor(_sg_def(desc->blend.src_factor_rgb, SG_BLENDFACTOR_ONE));
    bs_desc.RenderTarget[0].DestBlend = _sg_d3d11_blend_factor(_sg_def(desc->blend.dst_factor_rgb, SG_BLENDFACTOR_ZERO));
    bs_desc.RenderTarget[0].BlendOp = _sg_d3d11_blend_op(_sg_def(desc->blend.op_rgb, SG_BLENDOP_ADD));
    bs_desc.RenderTarget[0].SrcBlendAlpha = _sg_d3d11_blend_factor(_sg_def(desc->blend.src_factor_alpha, SG_BLENDFACTOR_ONE));
    bs_desc.RenderTarget[0].DestBlendAlpha = _sg_d3d11_blend_factor(_sg_def(desc->blend.dst_factor_alpha, SG_BLENDFACTOR_ZERO));
    bs_desc.RenderTarget[0].BlendOpAlpha = _sg_d3d11_blend_op(_sg_def(desc->blend.op_alpha, SG_BLENDOP_ADD));
    bs_desc.RenderTarget[0].RenderTargetWriteMask = _sg_d3d11_color_write_mask(_sg_def((sg_color_mask)desc->blend.color_write_mask, SG_COLORMASK_RGBA));
    hr = ID3D11Device_CreateBlendState(_sg_d3d11.dev, &bs_desc, &pip->d3d11_bs);
    SOKOL_ASSERT(SUCCEEDED(hr) && pip->d3d11_bs);

    pip->slot.state = SG_RESOURCESTATE_VALID;
}

_SOKOL_PRIVATE void _sg_destroy_pipeline(_sg_pipeline* pip) {
    SOKOL_ASSERT(pip);
    if (pip->d3d11_il) {
        ID3D11InputLayout_Release(pip->d3d11_il);
    }
    if (pip->d3d11_rs) {
        ID3D11RasterizerState_Release(pip->d3d11_rs);
    }
    if (pip->d3d11_dss) {
        ID3D11DepthStencilState_Release(pip->d3d11_dss);
    }
    if (pip->d3d11_bs) {
        ID3D11BlendState_Release(pip->d3d11_bs);
    }
    _sg_init_pipeline_slot(pip);
}

_SOKOL_PRIVATE void _sg_create_pass(_sg_pass* pass, _sg_image** att_images, const sg_pass_desc* desc) {
    SOKOL_ASSERT(pass && desc);
    SOKOL_ASSERT(pass->slot.state == SG_RESOURCESTATE_ALLOC);
    SOKOL_ASSERT(att_images && att_images[0]);
    SOKOL_ASSERT(_sg_d3d11.dev);

    const sg_attachment_desc* att_desc;
    _sg_attachment* att;
    for (int i = 0; i < SG_MAX_COLOR_ATTACHMENTS; i++) {
        SOKOL_ASSERT(0 == pass->color_atts[i].image);
        SOKOL_ASSERT(pass->d3d11_rtvs[i] == 0);
        att_desc = &desc->color_attachments[i];
        if (att_desc->image.id != SG_INVALID_ID) {
            pass->num_color_atts++;
            SOKOL_ASSERT(att_images[i] && (att_images[i]->slot.id == att_desc->image.id));
            SOKOL_ASSERT(_sg_is_valid_rendertarget_color_format(att_images[i]->pixel_format));
            att = &pass->color_atts[i];
            SOKOL_ASSERT((att->image == 0) && (att->image_id.id == SG_INVALID_ID));
            att->image = att_images[i];
            att->image_id = att_desc->image;
            att->mip_level = att_desc->mip_level;
            att->slice = att_desc->slice;

            /* create D3D11 render-target-view */
            ID3D11Resource* d3d11_res = 0;
            const bool is_msaa = att->image->sample_count > 1;
            D3D11_RENDER_TARGET_VIEW_DESC d3d11_rtv_desc;
            memset(&d3d11_rtv_desc, 0, sizeof(d3d11_rtv_desc));
            d3d11_rtv_desc.Format = att->image->d3d11_format;
            switch (att->image->type) {
                case SG_IMAGETYPE_2D:
                    if (is_msaa) {
                        d3d11_res = (ID3D11Resource*) att->image->d3d11_texmsaa;
                        d3d11_rtv_desc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2DMS;
                    }
                    else {
                        d3d11_res = (ID3D11Resource*) att->image->d3d11_tex2d;
                        d3d11_rtv_desc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;
                        d3d11_rtv_desc.Texture2D.MipSlice = att->mip_level;
                    }
                    break;
                case SG_IMAGETYPE_CUBE:
                case SG_IMAGETYPE_ARRAY:
                    if (is_msaa) {
                        d3d11_res = (ID3D11Resource*) att->image->d3d11_texmsaa;
                        d3d11_rtv_desc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2DMSARRAY;
                        d3d11_rtv_desc.Texture2DMSArray.FirstArraySlice = att->slice;
                        d3d11_rtv_desc.Texture2DMSArray.ArraySize = 1;
                    }
                    else {
                        d3d11_res = (ID3D11Resource*) att->image->d3d11_tex2d;
                        d3d11_rtv_desc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2DARRAY;
                        d3d11_rtv_desc.Texture2DArray.MipSlice = att->mip_level;
                        d3d11_rtv_desc.Texture2DArray.FirstArraySlice = att->slice;
                        d3d11_rtv_desc.Texture2DArray.ArraySize = 1;
                    }
                    break;
                case SG_IMAGETYPE_3D:
                    SOKOL_ASSERT(!is_msaa);
                    d3d11_res = (ID3D11Resource*) att->image->d3d11_tex3d;
                    d3d11_rtv_desc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE3D;
                    d3d11_rtv_desc.Texture3D.MipSlice = att->mip_level;
                    d3d11_rtv_desc.Texture3D.FirstWSlice = att->slice;
                    d3d11_rtv_desc.Texture3D.WSize = 1;
                    break;
                default:
                    SOKOL_UNREACHABLE; break;
            }
            SOKOL_ASSERT(d3d11_res);
            HRESULT hr = ID3D11Device_CreateRenderTargetView(_sg_d3d11.dev, d3d11_res, &d3d11_rtv_desc, &pass->d3d11_rtvs[i]);
            SOKOL_ASSERT(SUCCEEDED(hr) && pass->d3d11_rtvs[i]);
        }
    }

    /* optional depth-stencil image */
    SOKOL_ASSERT(0 == pass->ds_att.image);
    SOKOL_ASSERT(pass->d3d11_dsv == 0);
    att_desc = &desc->depth_stencil_attachment;
    const int ds_img_index = SG_MAX_COLOR_ATTACHMENTS;
    if (att_desc->image.id != SG_INVALID_ID) {
        SOKOL_ASSERT(att_images[ds_img_index] && (att_images[ds_img_index]->slot.id == att_desc->image.id));
        SOKOL_ASSERT(_sg_is_valid_rendertarget_depth_format(att_images[ds_img_index]->pixel_format));
        att = &pass->ds_att;
        SOKOL_ASSERT((att->image == 0) && (att->image_id.id == SG_INVALID_ID));
        att->image = att_images[ds_img_index];
        att->image_id = att_desc->image;
        att->mip_level = att_desc->mip_level;
        att->slice = att_desc->slice;

        /* create D3D11 depth-stencil-view */
        D3D11_DEPTH_STENCIL_VIEW_DESC d3d11_dsv_desc;
        memset(&d3d11_dsv_desc, 0, sizeof(d3d11_dsv_desc));
        d3d11_dsv_desc.Format = att->image->d3d11_format;
        const bool is_msaa = att->image->sample_count > 1;
        if (is_msaa) {
            d3d11_dsv_desc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2DMS;
        }
        else {
            d3d11_dsv_desc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
        }
        ID3D11Resource* d3d11_res = (ID3D11Resource*) att->image->d3d11_texds;
        SOKOL_ASSERT(d3d11_res);
        HRESULT hr = ID3D11Device_CreateDepthStencilView(_sg_d3d11.dev, d3d11_res, &d3d11_dsv_desc, &pass->d3d11_dsv);
        SOKOL_ASSERT(SUCCEEDED(hr) && pass->d3d11_dsv);
    }
    pass->slot.state = SG_RESOURCESTATE_VALID;
}

_SOKOL_PRIVATE void _sg_destroy_pass(_sg_pass* pass) {
    SOKOL_ASSERT(pass);
    for (int i = 0; i < SG_MAX_COLOR_ATTACHMENTS; i++) {
        if (pass->d3d11_rtvs[i]) {
            ID3D11RenderTargetView_Release(pass->d3d11_rtvs[i]);
        }
    }
    if (pass->d3d11_dsv) {
        ID3D11DepthStencilView_Release(pass->d3d11_dsv);
    }
    _sg_init_pass_slot(pass);
}

_SOKOL_PRIVATE void _sg_begin_pass(_sg_pass* pass, const sg_pass_action* action, int w, int h) {
    SOKOL_ASSERT(action);
    SOKOL_ASSERT(!_sg_d3d11.in_pass);
    _sg_d3d11.in_pass = true;
    _sg_d3d11.cur_width = w;
    _sg_d3d11.cur_height = h;
    if (pass) {
        _sg_d3d11.cur_pass = pass;
        _sg_d3d11.cur_pass_id.id = pass->slot.id;
        _sg_d3d11.num_rtvs = 0;
        for (int i = 0; i < SG_MAX_COLOR_ATTACHMENTS; i++) {
            _sg_d3d11.cur_rtvs[i] = pass->d3d11_rtvs[i];
            if (_sg_d3d11.cur_rtvs[i]) {
                _sg_d3d11.num_rtvs++;
            }
        }
        _sg_d3d11.cur_dsv = pass->d3d11_dsv;
    }
    else {
        /* render to default frame buffer */
        _sg_d3d11.cur_pass = 0;
        _sg_d3d11.cur_pass_id.id = SG_INVALID_ID;
        _sg_d3d11.num_rtvs = 1;
        _sg_d3d11.cur_rtvs[0] = (ID3D11RenderTargetView*) _sg_d3d11.rtv_cb();
        for (int i = 1; i < SG_MAX_COLOR_ATTACHMENTS; i++) {
            _sg_d3d11.cur_rtvs[i] = 0;
        }
        _sg_d3d11.cur_dsv = (ID3D11DepthStencilView*) _sg_d3d11.dsv_cb();
        SOKOL_ASSERT(_sg_d3d11.cur_rtvs[0] && _sg_d3d11.cur_dsv);
    }
    /* apply the render-target- and depth-stencil-views */
    ID3D11DeviceContext_OMSetRenderTargets(_sg_d3d11.ctx, SG_MAX_COLOR_ATTACHMENTS, _sg_d3d11.cur_rtvs, _sg_d3d11.cur_dsv);

    /* set viewport and scissor rect to cover whole screen */
    D3D11_VIEWPORT vp;
    memset(&vp, 0, sizeof(vp));
    vp.Width = (FLOAT) w;
    vp.Height = (FLOAT) h;
    vp.MaxDepth = 1.0f;
    ID3D11DeviceContext_RSSetViewports(_sg_d3d11.ctx, 1, &vp);
    D3D11_RECT rect;
    rect.left = 0;
    rect.top = 0;
    rect.right = w;
    rect.bottom = h;
    ID3D11DeviceContext_RSSetScissorRects(_sg_d3d11.ctx, 1, &rect);

    /* perform clear action */
    for (int i = 0; i < _sg_d3d11.num_rtvs; i++) {
        if (action->colors[i].action == SG_ACTION_CLEAR) {
            ID3D11DeviceContext_ClearRenderTargetView(_sg_d3d11.ctx, _sg_d3d11.cur_rtvs[i], action->colors[i].val);
        }
    }
    UINT ds_flags = 0;
    if (action->depth.action == SG_ACTION_CLEAR) {
        ds_flags |= D3D11_CLEAR_DEPTH;
    }
    if (action->stencil.action == SG_ACTION_CLEAR) {
        ds_flags |= D3D11_CLEAR_STENCIL;
    }
    if ((0 != ds_flags) && _sg_d3d11.cur_dsv) {
        ID3D11DeviceContext_ClearDepthStencilView(_sg_d3d11.ctx, _sg_d3d11.cur_dsv, ds_flags, action->depth.val, action->stencil.val);
    }
}

/* D3D11CalcSubresource only exists for C++ */
_SOKOL_PRIVATE UINT _sg_d3d11_calcsubresource(UINT mip_slice, UINT array_slice, UINT mip_levels) {
    return mip_slice + array_slice * mip_levels;
}

_SOKOL_PRIVATE void _sg_end_pass() {
    SOKOL_ASSERT(_sg_d3d11.in_pass && _sg_d3d11.ctx);
    _sg_d3d11.in_pass = false;

    /* need to resolve MSAA render target into texture? */
    if (_sg_d3d11.cur_pass) {
        SOKOL_ASSERT(_sg_d3d11.cur_pass->slot.id == _sg_d3d11.cur_pass_id.id);
        for (int i = 0; i < _sg_d3d11.num_rtvs; i++) {
            _sg_attachment* att = &_sg_d3d11.cur_pass->color_atts[i];
            SOKOL_ASSERT(att->image && (att->image->slot.id == att->image_id.id));
            if (att->image->sample_count > 1) {
                SOKOL_ASSERT(att->image->d3d11_tex2d && att->image->d3d11_texmsaa && !att->image->d3d11_tex3d);
                SOKOL_ASSERT(DXGI_FORMAT_UNKNOWN != att->image->d3d11_format);
                const _sg_image* img = att->image;
                UINT subres = _sg_d3d11_calcsubresource(att->mip_level, att->slice, img->num_mipmaps);
                ID3D11DeviceContext_ResolveSubresource(_sg_d3d11.ctx,
                    (ID3D11Resource*) img->d3d11_tex2d,     /* pDstResource */
                    subres,                                 /* DstSubresource */
                    (ID3D11Resource*) img->d3d11_texmsaa,   /* pSrcResource */
                    subres,                                 /* SrcSubresource */
                    img->d3d11_format);
            }
        }
    }
    _sg_d3d11.cur_pass = 0;
    _sg_d3d11.cur_pass_id.id = SG_INVALID_ID;
    _sg_d3d11.cur_pipeline = 0;
    _sg_d3d11.cur_pipeline_id.id = SG_INVALID_ID;
    for (int i = 0; i < SG_MAX_COLOR_ATTACHMENTS; i++) {
        _sg_d3d11.cur_rtvs[i] = 0;
    }
    _sg_d3d11.cur_dsv = 0;
    _sg_d3d11_clear_state();
}

_SOKOL_PRIVATE void _sg_apply_viewport(int x, int y, int w, int h, bool origin_top_left) {
    SOKOL_ASSERT(_sg_d3d11.ctx);
    SOKOL_ASSERT(_sg_d3d11.in_pass);
    D3D11_VIEWPORT vp;
    vp.TopLeftX = (FLOAT) x;
    vp.TopLeftY = (FLOAT) (origin_top_left ? y : (_sg_d3d11.cur_height - (y + h)));
    vp.Width = (FLOAT) w;
    vp.Height = (FLOAT) h;
    vp.MinDepth = 0.0f;
    vp.MaxDepth = 1.0f;
    ID3D11DeviceContext_RSSetViewports(_sg_d3d11.ctx, 1, &vp);
}

_SOKOL_PRIVATE void _sg_apply_scissor_rect(int x, int y, int w, int h, bool origin_top_left) {
    SOKOL_ASSERT(_sg_d3d11.ctx);
    SOKOL_ASSERT(_sg_d3d11.in_pass);
    D3D11_RECT rect;
    rect.left = x;
    rect.top = (origin_top_left ? y : (_sg_d3d11.cur_height - (y + h)));
    rect.right = x + w;
    rect.bottom = origin_top_left ? (y + h) : (_sg_d3d11.cur_height - y);
    ID3D11DeviceContext_RSSetScissorRects(_sg_d3d11.ctx, 1, &rect);
}

_SOKOL_PRIVATE void _sg_apply_draw_state(
    _sg_pipeline* pip,
    _sg_buffer** vbs, const uint32_t* vb_offsets, int num_vbs, 
    _sg_buffer* ib, uint32_t ib_offset,
    _sg_image** vs_imgs, int num_vs_imgs,
    _sg_image** fs_imgs, int num_fs_imgs)
{
    SOKOL_ASSERT(pip);
    SOKOL_ASSERT(pip->shader);
    SOKOL_ASSERT(_sg_d3d11.ctx);
    SOKOL_ASSERT(_sg_d3d11.in_pass);
    SOKOL_ASSERT(pip->d3d11_rs && pip->d3d11_bs && pip->d3d11_dss && pip->d3d11_il);

    _sg_d3d11.cur_pipeline = pip;
    _sg_d3d11.cur_pipeline_id.id = pip->slot.id;
    _sg_d3d11.use_indexed_draw = (pip->d3d11_index_format != DXGI_FORMAT_UNKNOWN);

    /* gather all the D3D11 resources into arrays */
    ID3D11Buffer* d3d11_ib = ib ? ib->d3d11_buf : 0;
    ID3D11Buffer* d3d11_vbs[SG_MAX_SHADERSTAGE_BUFFERS];
    UINT d3d11_vb_offsets[SG_MAX_SHADERSTAGE_BUFFERS];
    ID3D11ShaderResourceView* d3d11_vs_srvs[SG_MAX_SHADERSTAGE_IMAGES];
    ID3D11SamplerState* d3d11_vs_smps[SG_MAX_SHADERSTAGE_IMAGES];
    ID3D11ShaderResourceView* d3d11_fs_srvs[SG_MAX_SHADERSTAGE_IMAGES];
    ID3D11SamplerState* d3d11_fs_smps[SG_MAX_SHADERSTAGE_IMAGES];
    int i;
    for (i = 0; i < num_vbs; i++) {
        SOKOL_ASSERT(vbs[i]->d3d11_buf);
        d3d11_vbs[i] = vbs[i]->d3d11_buf;
        d3d11_vb_offsets[i] = vb_offsets[i];
    }
    for (; i < SG_MAX_SHADERSTAGE_BUFFERS; i++) {
        d3d11_vbs[i] = 0; 
        d3d11_vb_offsets[i] = 0;
    }
    for (i = 0; i < num_vs_imgs; i++) {
        SOKOL_ASSERT(vs_imgs[i]->d3d11_srv);
        SOKOL_ASSERT(vs_imgs[i]->d3d11_smp);
        d3d11_vs_srvs[i] = vs_imgs[i]->d3d11_srv;
        d3d11_vs_smps[i] = vs_imgs[i]->d3d11_smp;
    }
    for (; i < SG_MAX_SHADERSTAGE_IMAGES; i++) {
        d3d11_vs_srvs[i] = 0;
        d3d11_vs_smps[i] = 0;
    }
    for (i = 0; i < num_fs_imgs; i++) {
        SOKOL_ASSERT(fs_imgs[i]->d3d11_srv);
        SOKOL_ASSERT(fs_imgs[i]->d3d11_smp);
        d3d11_fs_srvs[i] = fs_imgs[i]->d3d11_srv;
        d3d11_fs_smps[i] = fs_imgs[i]->d3d11_smp;
    }
    for (; i < SG_MAX_SHADERSTAGE_IMAGES; i++) {
        d3d11_fs_srvs[i] = 0;
        d3d11_fs_smps[i] = 0;
    }

    /* FIXME: is it worth it to implement a state cache here? measure! */
    ID3D11DeviceContext_RSSetState(_sg_d3d11.ctx, pip->d3d11_rs);
    ID3D11DeviceContext_OMSetDepthStencilState(_sg_d3d11.ctx, pip->d3d11_dss, pip->d3d11_stencil_ref);
    ID3D11DeviceContext_OMSetBlendState(_sg_d3d11.ctx, pip->d3d11_bs, pip->blend_color, 0xFFFFFFFF);

    ID3D11DeviceContext_IASetVertexBuffers(_sg_d3d11.ctx, 0, SG_MAX_SHADERSTAGE_BUFFERS, d3d11_vbs, pip->d3d11_vb_strides, d3d11_vb_offsets);
    ID3D11DeviceContext_IASetPrimitiveTopology(_sg_d3d11.ctx, pip->d3d11_topology);
    ID3D11DeviceContext_IASetIndexBuffer(_sg_d3d11.ctx, d3d11_ib, pip->d3d11_index_format, ib_offset); 
    ID3D11DeviceContext_IASetInputLayout(_sg_d3d11.ctx, pip->d3d11_il);
    
    ID3D11DeviceContext_VSSetShader(_sg_d3d11.ctx, pip->shader->d3d11_vs, NULL, 0);
    ID3D11DeviceContext_VSSetConstantBuffers(_sg_d3d11.ctx, 0, SG_MAX_SHADERSTAGE_UBS, pip->shader->stage[SG_SHADERSTAGE_VS].d3d11_cbs);
    ID3D11DeviceContext_VSSetShaderResources(_sg_d3d11.ctx, 0, SG_MAX_SHADERSTAGE_IMAGES, d3d11_vs_srvs);
    ID3D11DeviceContext_VSSetSamplers(_sg_d3d11.ctx, 0, SG_MAX_SHADERSTAGE_IMAGES, d3d11_vs_smps);
    
    ID3D11DeviceContext_PSSetShader(_sg_d3d11.ctx, pip->shader->d3d11_fs, NULL, 0);
    ID3D11DeviceContext_PSSetConstantBuffers(_sg_d3d11.ctx, 0, SG_MAX_SHADERSTAGE_UBS, pip->shader->stage[SG_SHADERSTAGE_FS].d3d11_cbs);
    ID3D11DeviceContext_PSSetShaderResources(_sg_d3d11.ctx, 0, SG_MAX_SHADERSTAGE_IMAGES, d3d11_fs_srvs);
    ID3D11DeviceContext_PSSetSamplers(_sg_d3d11.ctx, 0, SG_MAX_SHADERSTAGE_IMAGES, d3d11_fs_smps);
}

_SOKOL_PRIVATE void _sg_apply_uniform_block(sg_shader_stage stage_index, int ub_index, const void* data, int num_bytes) {
    SOKOL_ASSERT(_sg_d3d11.ctx && _sg_d3d11.in_pass);
    SOKOL_ASSERT(data && (num_bytes > 0));
    SOKOL_ASSERT((stage_index >= 0) && (stage_index < SG_NUM_SHADER_STAGES));
    SOKOL_ASSERT((ub_index >= 0) && (ub_index < SG_MAX_SHADERSTAGE_UBS));
    SOKOL_ASSERT(_sg_d3d11.cur_pipeline && _sg_d3d11.cur_pipeline->slot.id == _sg_d3d11.cur_pipeline_id.id);
    SOKOL_ASSERT(_sg_d3d11.cur_pipeline->shader && _sg_d3d11.cur_pipeline->shader->slot.id == _sg_d3d11.cur_pipeline->shader_id.id);
    SOKOL_ASSERT(ub_index < _sg_d3d11.cur_pipeline->shader->stage[stage_index].num_uniform_blocks);
    SOKOL_ASSERT(num_bytes == _sg_d3d11.cur_pipeline->shader->stage[stage_index].uniform_blocks[ub_index].size);
    ID3D11Buffer* cb = _sg_d3d11.cur_pipeline->shader->stage[stage_index].d3d11_cbs[ub_index];
    SOKOL_ASSERT(cb);
    ID3D11DeviceContext_UpdateSubresource(_sg_d3d11.ctx, (ID3D11Resource*)cb, 0, NULL, data, 0, 0);
}

_SOKOL_PRIVATE void _sg_draw(int base_element, int num_elements, int num_instances) {
    SOKOL_ASSERT(_sg_d3d11.in_pass);
    if (_sg_d3d11.use_indexed_draw) {
        if (1 == num_instances) {
            ID3D11DeviceContext_DrawIndexed(_sg_d3d11.ctx, num_elements, base_element, 0);
        }
        else {
            ID3D11DeviceContext_DrawIndexedInstanced(_sg_d3d11.ctx, num_elements, num_instances, base_element, 0, 0);
        }
    }
    else {
        if (1 == num_instances) {
            ID3D11DeviceContext_Draw(_sg_d3d11.ctx, num_elements, base_element);
        }
        else {
            ID3D11DeviceContext_DrawInstanced(_sg_d3d11.ctx, num_elements, num_instances, base_element, 0);
        }
    }
}

_SOKOL_PRIVATE void _sg_commit() {
    SOKOL_ASSERT(!_sg_d3d11.in_pass);
}

_SOKOL_PRIVATE void _sg_update_buffer(_sg_buffer* buf, const void* data_ptr, int data_size) {
    SOKOL_ASSERT(buf && data_ptr && data_size);
    SOKOL_ASSERT(_sg_d3d11.ctx);
    SOKOL_ASSERT(buf->d3d11_buf);
    D3D11_MAPPED_SUBRESOURCE d3d11_msr;
    HRESULT hr = ID3D11DeviceContext_Map(_sg_d3d11.ctx, (ID3D11Resource*)buf->d3d11_buf, 0, D3D11_MAP_WRITE_DISCARD, 0, &d3d11_msr);
    SOKOL_ASSERT(SUCCEEDED(hr));
    memcpy(d3d11_msr.pData, data_ptr, data_size);
    ID3D11DeviceContext_Unmap(_sg_d3d11.ctx, (ID3D11Resource*)buf->d3d11_buf, 0);
}

_SOKOL_PRIVATE void _sg_update_image(_sg_image* img, const sg_image_content* data) {
    SOKOL_ASSERT(img && data);
    SOKOL_ASSERT(_sg_d3d11.ctx);
    SOKOL_ASSERT(img->d3d11_tex2d || img->d3d11_tex3d);
    ID3D11Resource* d3d11_res = 0;
    if (img->d3d11_tex3d) {
        d3d11_res = (ID3D11Resource*) img->d3d11_tex3d;
    }
    else {
        d3d11_res = (ID3D11Resource*) img->d3d11_tex2d;
    }
    SOKOL_ASSERT(d3d11_res);
    const int num_faces = (img->type == SG_IMAGETYPE_CUBE) ? 6:1;
    const int num_slices = (img->type == SG_IMAGETYPE_ARRAY) ? img->depth:1;
    int subres_index = 0;
    HRESULT hr;
    D3D11_MAPPED_SUBRESOURCE d3d11_msr;
    for (int face_index = 0; face_index < num_faces; face_index++) {
        for (int slice_index = 0; slice_index < num_slices; slice_index++) {
            for (int mip_index = 0; mip_index < img->num_mipmaps; mip_index++, subres_index++) {
                SOKOL_ASSERT(subres_index < (SG_MAX_MIPMAPS * SG_MAX_TEXTUREARRAY_LAYERS));
                const int mip_width = ((img->width>>mip_index)>0) ? img->width>>mip_index : 1;
                const int mip_height = ((img->height>>mip_index)>0) ? img->height>>mip_index : 1;
                const sg_subimage_content* subimg_content = &(data->subimage[face_index][mip_index]);
                const int slice_size = subimg_content->size / num_slices;
                const int slice_offset = slice_size * slice_index;
                const uint8_t* slice_ptr = ((const uint8_t*)subimg_content->ptr) + slice_offset;
                hr = ID3D11DeviceContext_Map(_sg_d3d11.ctx, d3d11_res, subres_index, D3D11_MAP_WRITE_DISCARD, 0, &d3d11_msr);
                SOKOL_ASSERT(SUCCEEDED(hr));
                memcpy(d3d11_msr.pData, slice_ptr, slice_size);
                ID3D11DeviceContext_Unmap(_sg_d3d11.ctx, d3d11_res, subres_index);
            }
        }
    }
}

#ifdef __cplusplus
} // extern "C"
#endif

/*== METAL BACKEND ===========================================================*/
#elif defined(SOKOL_METAL)

#if !__has_feature(objc_arc)
#error "Please enable ARC when using the Metal backend"
#endif

/* memset() */
#include <string.h>
#include <TargetConditionals.h>
#import <Metal/Metal.h>

#ifdef __cplusplus
extern "C" {
#endif
    
enum {
    _SG_MTL_DEFAULT_UB_SIZE = 4 * 1024 * 1024,
    #if !TARGET_OS_IPHONE
    _SG_MTL_UB_ALIGN = 256,
    #else
    _SG_MTL_UB_ALIGN = 16,
    #endif
    _SG_MTL_DEFAULT_SAMPLER_CACHE_CAPACITY = 64,
    _SG_MTL_INVALID_POOL_INDEX = 0xFFFFFFFF
};

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
            return MTLResourceStorageModeShared;
        case SG_USAGE_DYNAMIC:
        case SG_USAGE_STREAM:
            #if !TARGET_OS_IPHONE
            return MTLCPUCacheModeWriteCombined|MTLResourceStorageModeManaged;
            #else
            return MTLCPUCacheModeWriteCombined;
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
        case SG_VERTEXFORMAT_SHORT4:    return MTLVertexFormatShort4;
        case SG_VERTEXFORMAT_SHORT4N:   return MTLVertexFormatShort4Normalized;
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

_SOKOL_PRIVATE MTLPixelFormat _sg_mtl_texture_format(sg_pixel_format fmt) {
    switch (fmt) {
        case SG_PIXELFORMAT_RGBA8:          return MTLPixelFormatRGBA8Unorm;
        case SG_PIXELFORMAT_R10G10B10A2:    return MTLPixelFormatRGB10A2Unorm;
        case SG_PIXELFORMAT_RGBA32F:        return MTLPixelFormatRGBA32Float;
        case SG_PIXELFORMAT_RGBA16F:        return MTLPixelFormatRGBA16Float;
        case SG_PIXELFORMAT_R32F:           return MTLPixelFormatR32Float;
        case SG_PIXELFORMAT_R16F:           return MTLPixelFormatR16Float;
        case SG_PIXELFORMAT_L8:             return MTLPixelFormatR8Unorm;
        #if !TARGET_OS_IPHONE
        case SG_PIXELFORMAT_DXT1:           return MTLPixelFormatBC1_RGBA;
        case SG_PIXELFORMAT_DXT3:           return MTLPixelFormatBC2_RGBA;
        case SG_PIXELFORMAT_DXT5:           return MTLPixelFormatBC3_RGBA;
        #else
        case SG_PIXELFORMAT_PVRTC2_RGB:     return MTLPixelFormatPVRTC_RGB_2BPP;
        case SG_PIXELFORMAT_PVRTC4_RGB:     return MTLPixelFormatPVRTC_RGB_4BPP;
        case SG_PIXELFORMAT_PVRTC2_RGBA:    return MTLPixelFormatPVRTC_RGBA_2BPP;
        case SG_PIXELFORMAT_PVRTC4_RGBA:    return MTLPixelFormatPVRTC_RGBA_4BPP;
        case SG_PIXELFORMAT_ETC2_RGB8:      return MTLPixelFormatETC2_RGB8;
        case SG_PIXELFORMAT_ETC2_SRGB8:     return MTLPixelFormatETC2_RGB8_sRGB;
        #endif
        default:                            return MTLPixelFormatInvalid;
    }
}

_SOKOL_PRIVATE MTLPixelFormat _sg_mtl_rendertarget_color_format(sg_pixel_format fmt) {
    switch (fmt) {
        case SG_PIXELFORMAT_RGBA8:          return MTLPixelFormatBGRA8Unorm;    /* not a bug */
        case SG_PIXELFORMAT_RGBA32F:        return MTLPixelFormatRGBA32Float;
        case SG_PIXELFORMAT_RGBA16F:        return MTLPixelFormatRGBA16Float;
        case SG_PIXELFORMAT_R10G10B10A2:    return MTLPixelFormatRGB10A2Unorm;
        default:                            return MTLPixelFormatInvalid;
    }
}

_SOKOL_PRIVATE MTLPixelFormat _sg_mtl_rendertarget_depth_format(sg_pixel_format fmt) {
    switch (fmt) {
        case SG_PIXELFORMAT_DEPTH:
            return MTLPixelFormatDepth32Float;
        case SG_PIXELFORMAT_DEPTHSTENCIL:
            /* NOTE: Depth24_Stencil8 isn't universally supported! */
            return MTLPixelFormatDepth32Float_Stencil8;
        default:
            return MTLPixelFormatInvalid;
    }
}

_SOKOL_PRIVATE MTLPixelFormat _sg_mtl_rendertarget_stencil_format(sg_pixel_format fmt) {
    switch (fmt) {
        case SG_PIXELFORMAT_DEPTHSTENCIL:
            return MTLPixelFormatDepth32Float_Stencil8;
        default:
            return MTLPixelFormatInvalid;
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

_SOKOL_PRIVATE NSUInteger _sg_mtl_index_size(sg_index_type t) {
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
        case SG_PIXELFORMAT_PVRTC2_RGB:
        case SG_PIXELFORMAT_PVRTC2_RGBA:
        case SG_PIXELFORMAT_PVRTC4_RGB:
        case SG_PIXELFORMAT_PVRTC4_RGBA:
            return true;
        default:
            return false;
    }
}

_SOKOL_PRIVATE MTLSamplerAddressMode _sg_mtl_address_mode(sg_wrap w) {
    switch (w) {
        case SG_WRAP_REPEAT:            return MTLSamplerAddressModeRepeat;
        case SG_WRAP_CLAMP_TO_EDGE:     return MTLSamplerAddressModeClampToEdge;
        case SG_WRAP_MIRRORED_REPEAT:   return MTLSamplerAddressModeMirrorRepeat;
        default: SOKOL_UNREACHABLE; return (MTLSamplerAddressMode)0;
    }
}

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
static uint32_t _sg_mtl_pool_size;
static NSMutableArray* _sg_mtl_pool;
static uint32_t _sg_mtl_free_queue_top;
static uint32_t* _sg_mtl_free_queue;
static uint32_t _sg_mtl_release_queue_front;
static uint32_t _sg_mtl_release_queue_back;
typedef struct {
    uint32_t frame_index;   /* frame index at which it is safe to release this resource */
    uint32_t pool_index;
} _sg_mtl_release_item;
static _sg_mtl_release_item* _sg_mtl_release_queue;

_SOKOL_PRIVATE void _sg_mtl_init_pool(const sg_desc* desc) {
    _sg_mtl_pool_size = 2 *
        2 * _sg_def(desc->buffer_pool_size, _SG_DEFAULT_BUFFER_POOL_SIZE) +
        5 * _sg_def(desc->image_pool_size, _SG_DEFAULT_IMAGE_POOL_SIZE) +
        4 * _sg_def(desc->shader_pool_size, _SG_DEFAULT_SHADER_POOL_SIZE) +
        2 * _sg_def(desc->pipeline_pool_size, _SG_DEFAULT_PIPELINE_POOL_SIZE) +
        _sg_def(desc->pass_pool_size, _SG_DEFAULT_PASS_POOL_SIZE);
    _sg_mtl_pool = [NSMutableArray arrayWithCapacity:_sg_mtl_pool_size];
    NSNull* null = [NSNull null];
    for (uint32_t i = 0; i < _sg_mtl_pool_size; i++) {
        [_sg_mtl_pool addObject:null];
    }
    SOKOL_ASSERT([_sg_mtl_pool count] == _sg_mtl_pool_size);
    /* a queue of currently free slot indices */
    _sg_mtl_free_queue_top = 0;
    _sg_mtl_free_queue = (uint32_t*)SOKOL_MALLOC(_sg_mtl_pool_size * sizeof(uint32_t));
    for (int i = _sg_mtl_pool_size-1; i >= 0; i--) {
        _sg_mtl_free_queue[_sg_mtl_free_queue_top++] = (uint32_t)i;
    }
    /* a circular queue which holds release items (frame index
       when a resource is to be released, and the resource's
       pool index
    */
    _sg_mtl_release_queue_front = 0;
    _sg_mtl_release_queue_back = 0;
    _sg_mtl_release_queue = (_sg_mtl_release_item*)SOKOL_MALLOC(_sg_mtl_pool_size * sizeof(_sg_mtl_release_item));
    for (uint32_t i = 0; i < _sg_mtl_pool_size; i++) {
        _sg_mtl_release_queue[i].frame_index = 0;
        _sg_mtl_release_queue[i].pool_index = _SG_MTL_INVALID_POOL_INDEX;
    }
}

_SOKOL_PRIVATE void _sg_mtl_destroy_pool() {
    SOKOL_FREE(_sg_mtl_release_queue);  _sg_mtl_release_queue = 0;
    SOKOL_FREE(_sg_mtl_free_queue);     _sg_mtl_free_queue = 0;
    _sg_mtl_pool = nil;
}

/* get a new free resource pool slot */
_SOKOL_PRIVATE uint32_t _sg_mtl_alloc_pool_slot() {
    SOKOL_ASSERT(_sg_mtl_free_queue_top > 0);
    const uint32_t pool_index = _sg_mtl_free_queue[--_sg_mtl_free_queue_top];
    return pool_index;
}

/* put a free resource pool slot back into the free-queue */
_SOKOL_PRIVATE void _sg_mtl_free_pool_slot(uint32_t pool_index) {
    SOKOL_ASSERT(_sg_mtl_free_queue_top < _sg_mtl_pool_size);
    _sg_mtl_free_queue[_sg_mtl_free_queue_top++] = pool_index;
}

/*  add an MTLResource to the pool, return pool index or 0xFFFFFFFF if input was 'nil' */
_SOKOL_PRIVATE uint32_t _sg_mtl_add_resource(id res) {
    if (nil == res) {
        return _SG_MTL_INVALID_POOL_INDEX;
    }
    const uint32_t pool_index = _sg_mtl_alloc_pool_slot();
    SOKOL_ASSERT([NSNull null] == _sg_mtl_pool[pool_index]);
    _sg_mtl_pool[pool_index] = res;
    return pool_index;
}

/*  mark an MTLResource for release, this will put the resource into the
    deferred-release queue, and the resource will then be released N frames later,
    the special pool index 0xFFFFFFFF will be ignored (this means that a nil
    value was provided to _sg_mtl_add_resource()
*/
_SOKOL_PRIVATE void _sg_mtl_release_resource(uint32_t frame_index, uint32_t pool_index) {
    if (pool_index == _SG_MTL_INVALID_POOL_INDEX) {
        return;
    }
    SOKOL_ASSERT((pool_index >= 0) && (pool_index < _sg_mtl_pool_size));
    SOKOL_ASSERT([NSNull null] != _sg_mtl_pool[pool_index]);
    int slot_index = _sg_mtl_release_queue_front++;
    if (_sg_mtl_release_queue_front >= _sg_mtl_pool_size) {
        /* wrap-around */
        _sg_mtl_release_queue_front = 0;
    }
    /* release queue full? */
    SOKOL_ASSERT(_sg_mtl_release_queue_front != _sg_mtl_release_queue_back);
    SOKOL_ASSERT(0 == _sg_mtl_release_queue[slot_index].frame_index);
    const uint32_t safe_to_release_frame_index = frame_index + SG_NUM_INFLIGHT_FRAMES + 1;
    _sg_mtl_release_queue[slot_index].frame_index = safe_to_release_frame_index;
    _sg_mtl_release_queue[slot_index].pool_index = pool_index;
}

/* run garbage-collection pass on all resources in the release-queue */
_SOKOL_PRIVATE void _sg_mtl_garbage_collect(uint32_t frame_index) {
    while (_sg_mtl_release_queue_back != _sg_mtl_release_queue_front) {
        if (frame_index < _sg_mtl_release_queue[_sg_mtl_release_queue_back].frame_index) {
            /* don't need to check further, release-items past this are too young */
            break;
        }
        /* safe to release this resource */
        const uint32_t pool_index = _sg_mtl_release_queue[_sg_mtl_release_queue_back].pool_index;
        SOKOL_ASSERT(pool_index < _sg_mtl_pool_size);
        SOKOL_ASSERT(_sg_mtl_pool[pool_index] != [NSNull null]);
        _sg_mtl_pool[pool_index] = [NSNull null];
        /* put the now free pool index back on the free queue */
        _sg_mtl_free_pool_slot(pool_index);
        /* reset the release queue slot and advance the back index */
        _sg_mtl_release_queue[_sg_mtl_release_queue_back].frame_index = 0;
        _sg_mtl_release_queue[_sg_mtl_release_queue_back].pool_index = _SG_MTL_INVALID_POOL_INDEX;
        _sg_mtl_release_queue_back++;
        if (_sg_mtl_release_queue_back >= _sg_mtl_pool_size) {
            /* wrap-around */
            _sg_mtl_release_queue_back = 0;
        }
    }
}

/*-- a very simple sampler cache -----------------------------------------------

    since there's only a small number of different samplers, sampler objects
    will never be deleted (except on shutdown), and searching an identical
    sampler is a simple linear search
*/
typedef struct {
    sg_filter min_filter;
    sg_filter mag_filter;
    sg_wrap wrap_u;
    sg_wrap wrap_v;
    sg_wrap wrap_w;
    uint32_t max_anisotropy;
    int min_lod;    /* orig min/max_lod is float, this is int(min/max_lod*1000.0) */
    int max_lod;
    uint32_t mtl_sampler_state;
} _sg_mtl_sampler_cache_item;
static int _sg_mtl_sampler_cache_capacity;
static int _sg_mtl_sampler_cache_size;
static _sg_mtl_sampler_cache_item* _sg_mtl_sampler_cache;

/* initialize the sampler cache */
_SOKOL_PRIVATE void _sg_mtl_init_sampler_cache(const sg_desc* desc) {
    _sg_mtl_sampler_cache_capacity = _sg_def(desc->mtl_sampler_cache_size, _SG_MTL_DEFAULT_SAMPLER_CACHE_CAPACITY);
    _sg_mtl_sampler_cache_size = 0;
    const int size = _sg_mtl_sampler_cache_capacity * sizeof(_sg_mtl_sampler_cache_item);
    _sg_mtl_sampler_cache = (_sg_mtl_sampler_cache_item*)SOKOL_MALLOC(size);
    memset(_sg_mtl_sampler_cache, 0, size);
}

/* destroy the sampler cache, and release all sampler objects */
_SOKOL_PRIVATE void _sg_mtl_destroy_sampler_cache(uint32_t frame_index) {
    SOKOL_ASSERT(_sg_mtl_sampler_cache);
    SOKOL_ASSERT(_sg_mtl_sampler_cache_size <= _sg_mtl_sampler_cache_capacity);
    for (int i = 0; i < _sg_mtl_sampler_cache_size; i++) {
        _sg_mtl_release_resource(frame_index, _sg_mtl_sampler_cache[i].mtl_sampler_state);
    }
    SOKOL_FREE(_sg_mtl_sampler_cache); _sg_mtl_sampler_cache = 0;
    _sg_mtl_sampler_cache_size = 0;
    _sg_mtl_sampler_cache_capacity = 0;
}

/* 
    create and add an MTLSamplerStateObject and return its resource pool index,
    reuse identical sampler state if one exists
*/
_SOKOL_PRIVATE uint32_t _sg_mtl_create_sampler(id<MTLDevice> mtl_device, const sg_image_desc* img_desc) {
    SOKOL_ASSERT(img_desc);
    SOKOL_ASSERT(_sg_mtl_sampler_cache);
    /* sampler state cache is full */
    const sg_filter min_filter = _sg_def(img_desc->min_filter, SG_FILTER_NEAREST);
    const sg_filter mag_filter = _sg_def(img_desc->mag_filter, SG_FILTER_NEAREST);
    const sg_wrap wrap_u = _sg_def(img_desc->wrap_u, SG_WRAP_REPEAT);
    const sg_wrap wrap_v = _sg_def(img_desc->wrap_v, SG_WRAP_REPEAT);
    const sg_wrap wrap_w = _sg_def(img_desc->wrap_w, SG_WRAP_REPEAT);
    const uint32_t max_anisotropy = _sg_def(img_desc->max_anisotropy, 1);
    /* convert floats to valid int for proper comparison */
    const int min_lod = (int)(img_desc->min_lod * 1000.0f);
    const int max_lod = (int)(_sg_def_flt(img_desc->max_lod, 1000.0f) * 1000.0f);
    /* first try to find identical sampler, number of samplers will be small, so linear search is ok */
    for (int i = 0; i < _sg_mtl_sampler_cache_size; i++) {
        _sg_mtl_sampler_cache_item* item = &_sg_mtl_sampler_cache[i];
        if ((min_filter == item->min_filter) &&
            (mag_filter == item->mag_filter) &&
            (wrap_u == item->wrap_u) &&
            (wrap_v == item->wrap_v) &&
            (wrap_w == item->wrap_w) &&
            (max_anisotropy == item->max_anisotropy) &&
            (min_lod == item->min_lod) &&
            (max_lod == item->max_lod))
        {
            return item->mtl_sampler_state;
        }
    }
    /* fallthrough: need to create a new MTLSamplerState object */
    SOKOL_ASSERT(_sg_mtl_sampler_cache_size < _sg_mtl_sampler_cache_capacity);
    _sg_mtl_sampler_cache_item* new_item = &_sg_mtl_sampler_cache[_sg_mtl_sampler_cache_size++];
    new_item->min_filter = min_filter;
    new_item->mag_filter = mag_filter;
    new_item->wrap_u = wrap_u;
    new_item->wrap_v = wrap_v;
    new_item->wrap_w = wrap_w;
    new_item->min_lod = min_lod;
    new_item->max_lod = max_lod;
    new_item->max_anisotropy = max_anisotropy;
    MTLSamplerDescriptor* mtl_desc = [[MTLSamplerDescriptor alloc] init];
    mtl_desc.sAddressMode = _sg_mtl_address_mode(wrap_u);
    mtl_desc.tAddressMode = _sg_mtl_address_mode(wrap_v);
    if (SG_IMAGETYPE_3D == img_desc->type) {
        mtl_desc.rAddressMode = _sg_mtl_address_mode(wrap_w);
    }
    mtl_desc.minFilter = _sg_mtl_minmag_filter(min_filter);
    mtl_desc.magFilter = _sg_mtl_minmag_filter(mag_filter);
    mtl_desc.mipFilter = _sg_mtl_mip_filter(min_filter);
    mtl_desc.lodMinClamp = img_desc->min_lod;
    mtl_desc.lodMaxClamp = _sg_def_flt(img_desc->max_lod, FLT_MAX);
    mtl_desc.maxAnisotropy = max_anisotropy;
    mtl_desc.normalizedCoordinates = YES;
    id<MTLSamplerState> mtl_sampler = [mtl_device newSamplerStateWithDescriptor:mtl_desc];
    new_item->mtl_sampler_state = _sg_mtl_add_resource(mtl_sampler);
    return new_item->mtl_sampler_state;
}

/*-- Metal backend resource structs ------------------------------------------*/
typedef struct {
    _sg_slot slot;
    int size;
    sg_buffer_type type;
    sg_usage usage;
    uint32_t upd_frame_index;
    int num_slots;
    int active_slot;
    uint32_t mtl_buf[SG_NUM_INFLIGHT_FRAMES];  /* index intp _sg_mtl_pool */
} _sg_buffer;

_SOKOL_PRIVATE void _sg_init_buffer_slot(_sg_buffer* buf) {
    SOKOL_ASSERT(buf);
    memset(buf, 0, sizeof(_sg_buffer));
}

typedef struct {
    _sg_slot slot;
    sg_image_type type;
    bool render_target;
    int width;
    int height;
    int depth;
    int num_mipmaps;
    sg_usage usage;
    sg_pixel_format pixel_format;
    int sample_count;
    sg_filter min_filter;
    sg_filter mag_filter;
    sg_wrap wrap_u;
    sg_wrap wrap_v;
    sg_wrap wrap_w;
    uint32_t max_anisotropy;
    uint32_t upd_frame_index;
    int num_slots;
    int active_slot;
    uint32_t mtl_tex[SG_NUM_INFLIGHT_FRAMES];
    uint32_t mtl_depth_tex;
    uint32_t mtl_msaa_tex;
    uint32_t mtl_sampler_state;
} _sg_image;

_SOKOL_PRIVATE void _sg_init_image_slot(_sg_image* img) {
    SOKOL_ASSERT(img);
    memset(img, 0, sizeof(_sg_image));
}

typedef struct {
    int size;
} _sg_uniform_block;

typedef struct {
    sg_image_type type;
} _sg_shader_image;

typedef struct {
    int num_uniform_blocks;
    int num_images;
    _sg_uniform_block uniform_blocks[SG_MAX_SHADERSTAGE_UBS];
    _sg_shader_image images[SG_MAX_SHADERSTAGE_IMAGES];
    uint32_t mtl_lib;
    uint32_t mtl_func;
} _sg_shader_stage;

typedef struct {
    _sg_slot slot;
    _sg_shader_stage stage[SG_NUM_SHADER_STAGES];
} _sg_shader;

_SOKOL_PRIVATE void _sg_init_shader_slot(_sg_shader* shd) {
    SOKOL_ASSERT(shd);
    memset(shd, 0, sizeof(_sg_shader));
}

typedef struct {
    _sg_slot slot;
    _sg_shader* shader;
    sg_shader shader_id;
    bool vertex_layout_valid[SG_MAX_SHADERSTAGE_BUFFERS];
    int color_attachment_count;
    sg_pixel_format color_format;
    sg_pixel_format depth_format;
    int sample_count;
    float depth_bias;
    float depth_bias_slope_scale;
    float depth_bias_clamp;
    MTLPrimitiveType mtl_prim_type;
    sg_index_type index_type;
    NSUInteger mtl_index_size;
    MTLIndexType mtl_index_type;
    MTLCullMode mtl_cull_mode;
    MTLWinding mtl_winding;
    float blend_color[4];
    uint32_t mtl_stencil_ref;
    uint32_t mtl_rps;
    uint32_t mtl_dss;
} _sg_pipeline;

_SOKOL_PRIVATE void _sg_init_pipeline_slot(_sg_pipeline* pip) {
    SOKOL_ASSERT(pip);
    memset(pip, 0, sizeof(_sg_pipeline));
}

typedef struct {
    _sg_image* image;
    sg_image image_id;
    int mip_level;
    int slice;
} _sg_attachment;

typedef struct {
    _sg_slot slot;
    int num_color_atts;
    _sg_attachment color_atts[SG_MAX_COLOR_ATTACHMENTS];
    _sg_attachment ds_att;
} _sg_pass;

_SOKOL_PRIVATE void _sg_init_pass_slot(_sg_pass* pass) {
    SOKOL_ASSERT(pass);
    memset(pass, 0, sizeof(_sg_pass));
}

typedef struct {
    _sg_slot slot;
} _sg_context;

_SOKOL_PRIVATE void _sg_init_context_slot(_sg_context* ctx) {
    SOKOL_ASSERT(ctx);
    memset(ctx, 0, sizeof(_sg_context));
}

/*-- a simple state cache for the resource bindings --------------------------*/
static const _sg_pipeline* _sg_mtl_cur_pipeline;
static sg_pipeline _sg_mtl_cur_pipeline_id;
static const _sg_buffer* _sg_mtl_cur_indexbuffer;
static uint32_t _sg_mtl_cur_indexbuffer_offset;
static sg_buffer _sg_mtl_cur_indexbuffer_id;
static const _sg_buffer* _sg_mtl_cur_vertexbuffers[SG_MAX_SHADERSTAGE_BUFFERS];
static uint32_t _sg_mtl_cur_vertexbuffer_offsets[SG_MAX_SHADERSTAGE_BUFFERS];
static sg_buffer _sg_mtl_cur_vertexbuffer_ids[SG_MAX_SHADERSTAGE_BUFFERS];
static const _sg_image* _sg_mtl_cur_vs_images[SG_MAX_SHADERSTAGE_IMAGES];
static sg_image _sg_mtl_cur_vs_image_ids[SG_MAX_SHADERSTAGE_IMAGES];
static const _sg_image* _sg_mtl_cur_fs_images[SG_MAX_SHADERSTAGE_IMAGES];
static sg_image _sg_mtl_cur_fs_image_ids[SG_MAX_SHADERSTAGE_IMAGES];

_SOKOL_PRIVATE void _sg_mtl_clear_state_cache() {
    _sg_mtl_cur_pipeline = 0;
    _sg_mtl_cur_pipeline_id.id = SG_INVALID_ID;
    _sg_mtl_cur_indexbuffer = 0;
    _sg_mtl_cur_indexbuffer_offset = 0;
    _sg_mtl_cur_indexbuffer_id.id = SG_INVALID_ID;
    for (int i = 0; i < SG_MAX_SHADERSTAGE_BUFFERS; i++) {
        _sg_mtl_cur_vertexbuffers[i] = 0;
        _sg_mtl_cur_vertexbuffer_offsets[i] = 0;
        _sg_mtl_cur_vertexbuffer_ids[i].id = SG_INVALID_ID;
    }
    for (int i = 0; i < SG_MAX_SHADERSTAGE_IMAGES; i++) {
        _sg_mtl_cur_vs_images[i] = 0;
        _sg_mtl_cur_vs_image_ids[i].id = SG_INVALID_ID;
        _sg_mtl_cur_fs_images[i] = 0;
        _sg_mtl_cur_fs_image_ids[i].id = SG_INVALID_ID;
    }
}

/*-- main Metal backend state and functions ----------------------------------*/
static bool _sg_mtl_valid;
static const void*(*_sg_mtl_renderpass_descriptor_cb)(void);
static const void*(*_sg_mtl_drawable_cb)(void);
static id<MTLDevice> _sg_mtl_device;
static id<MTLCommandQueue> _sg_mtl_cmd_queue;
static id<MTLCommandBuffer> _sg_mtl_cmd_buffer;
static id<MTLRenderCommandEncoder> _sg_mtl_cmd_encoder;
static uint32_t _sg_mtl_frame_index;
static uint32_t _sg_mtl_cur_frame_rotate_index;
static uint32_t _sg_mtl_ub_size;
static uint32_t _sg_mtl_cur_ub_offset;
static uint8_t* _sg_mtl_cur_ub_base_ptr;
static id<MTLBuffer> _sg_mtl_uniform_buffers[SG_NUM_INFLIGHT_FRAMES];
static dispatch_semaphore_t _sg_mtl_sem;
static bool _sg_mtl_in_pass;
static bool _sg_mtl_pass_valid;
static int _sg_mtl_cur_width;
static int _sg_mtl_cur_height;

_SOKOL_PRIVATE void _sg_setup_backend(const sg_desc* desc) {
    SOKOL_ASSERT(desc);
    SOKOL_ASSERT(desc->mtl_device);
    SOKOL_ASSERT(desc->mtl_renderpass_descriptor_cb);
    SOKOL_ASSERT(desc->mtl_drawable_cb);
    _sg_mtl_init_pool(desc);
    _sg_mtl_init_sampler_cache(desc);
    _sg_mtl_clear_state_cache();
    _sg_mtl_valid = true;
    _sg_mtl_renderpass_descriptor_cb = desc->mtl_renderpass_descriptor_cb;
    _sg_mtl_drawable_cb = desc->mtl_drawable_cb;
    _sg_mtl_in_pass = false;
    _sg_mtl_pass_valid = false;
    _sg_mtl_cur_width = 0;
    _sg_mtl_cur_height = 0;
    _sg_mtl_frame_index = 1;
    _sg_mtl_cur_frame_rotate_index = 0;
    _sg_mtl_cur_ub_offset = 0;
    _sg_mtl_cur_ub_base_ptr = 0;
    _sg_mtl_device = (__bridge id<MTLDevice>) desc->mtl_device;
    _sg_mtl_sem = dispatch_semaphore_create(SG_NUM_INFLIGHT_FRAMES);
    _sg_mtl_cmd_queue = [_sg_mtl_device newCommandQueue];
    _sg_mtl_ub_size = _sg_def(desc->mtl_global_uniform_buffer_size, _SG_MTL_DEFAULT_UB_SIZE);
    MTLResourceOptions res_opts = MTLResourceCPUCacheModeWriteCombined;
    #if !TARGET_OS_IPHONE
    res_opts |= MTLResourceStorageModeManaged;
    #endif
    for (int i = 0; i < SG_NUM_INFLIGHT_FRAMES; i++) {
        _sg_mtl_uniform_buffers[i] = [_sg_mtl_device
            newBufferWithLength:_sg_mtl_ub_size
            options:res_opts
        ];
    }
}

_SOKOL_PRIVATE void _sg_discard_backend() {
    SOKOL_ASSERT(_sg_mtl_valid);
    /* wait for the last frame to finish */
    for (int i = 0; i < SG_NUM_INFLIGHT_FRAMES; i++) {
        dispatch_semaphore_wait(_sg_mtl_sem, DISPATCH_TIME_FOREVER);
    }
    _sg_mtl_destroy_sampler_cache(_sg_mtl_frame_index);
    _sg_mtl_garbage_collect(_sg_mtl_frame_index + SG_NUM_INFLIGHT_FRAMES + 2);
    _sg_mtl_destroy_pool();
    _sg_mtl_valid = false;
    _sg_mtl_cmd_encoder = nil;
    _sg_mtl_cmd_buffer = nil;
    _sg_mtl_cmd_queue = nil;
    for (int i = 0; i < SG_NUM_INFLIGHT_FRAMES; i++) {
        _sg_mtl_uniform_buffers[i] = nil;
    }
    _sg_mtl_device = nil;
}

_SOKOL_PRIVATE bool _sg_query_feature(sg_feature f) {
    switch (f) {
        case SG_FEATURE_INSTANCING:
        #if !TARGET_OS_IPHONE 
        case SG_FEATURE_TEXTURE_COMPRESSION_DXT:
        #else
        case SG_FEATURE_TEXTURE_COMPRESSION_PVRTC:
        case SG_FEATURE_TEXTURE_COMPRESSION_ETC2:
        #endif
        case SG_FEATURE_TEXTURE_FLOAT:
        case SG_FEATURE_ORIGIN_TOP_LEFT:
        case SG_FEATURE_MSAA_RENDER_TARGETS:
        case SG_FEATURE_PACKED_VERTEX_FORMAT_10_2:
        case SG_FEATURE_MULTIPLE_RENDER_TARGET:
        case SG_FEATURE_IMAGETYPE_3D:
        case SG_FEATURE_IMAGETYPE_ARRAY:
            return true;
        default:
            return false;
    }
}

_SOKOL_PRIVATE void _sg_reset_state_cache() {
    _sg_mtl_clear_state_cache();
}

_SOKOL_PRIVATE void _sg_create_context(_sg_context* ctx) {
    SOKOL_ASSERT(ctx);
    SOKOL_ASSERT(ctx->slot.state == SG_RESOURCESTATE_ALLOC);
    ctx->slot.state = SG_RESOURCESTATE_VALID;
}

_SOKOL_PRIVATE void _sg_destroy_context(_sg_context* ctx) {
    SOKOL_ASSERT(ctx);
    _sg_init_context_slot(ctx);
}

_SOKOL_PRIVATE void _sg_activate_context(_sg_context* ctx) {
    _sg_reset_state_cache();
}

_SOKOL_PRIVATE void _sg_create_buffer(_sg_buffer* buf, const sg_buffer_desc* desc) {
    SOKOL_ASSERT(buf && desc);
    SOKOL_ASSERT(buf->slot.state == SG_RESOURCESTATE_ALLOC);
    buf->size = desc->size;
    buf->type = _sg_def(desc->type, SG_BUFFERTYPE_VERTEXBUFFER);
    buf->usage = _sg_def(desc->usage, SG_USAGE_IMMUTABLE);
    buf->upd_frame_index = 0;
    buf->num_slots = (buf->usage == SG_USAGE_IMMUTABLE) ? 1 : SG_NUM_INFLIGHT_FRAMES;
    buf->active_slot = 0;
    const bool injected = (0 != desc->mtl_buffers[0]);
    MTLResourceOptions mtl_options = _sg_mtl_buffer_resource_options(buf->usage);
    for (int slot = 0; slot < buf->num_slots; slot++) {
        id<MTLBuffer> mtl_buf;
        if (injected) {
            SOKOL_ASSERT(desc->mtl_buffers[slot]);
            mtl_buf = (__bridge id<MTLBuffer>) desc->mtl_buffers[slot];
        }
        else {
            if (buf->usage == SG_USAGE_IMMUTABLE) {
                SOKOL_ASSERT(desc->content);
                mtl_buf = [_sg_mtl_device newBufferWithBytes:desc->content length:buf->size options:mtl_options];
            }
            else {
                mtl_buf = [_sg_mtl_device newBufferWithLength:buf->size options:mtl_options];
            }
        }
        buf->mtl_buf[slot] = _sg_mtl_add_resource(mtl_buf);
    }
    buf->slot.state = SG_RESOURCESTATE_VALID;
}

_SOKOL_PRIVATE void _sg_destroy_buffer(_sg_buffer* buf) {
    SOKOL_ASSERT(buf);
    if (buf->slot.state == SG_RESOURCESTATE_VALID) {
        for (int slot = 0; slot < buf->num_slots; slot++) {
            _sg_mtl_release_resource(_sg_mtl_frame_index, buf->mtl_buf[slot]);
        }
    }
    _sg_init_buffer_slot(buf);
}

_SOKOL_PRIVATE void _sg_mtl_copy_image_content(const _sg_image* img, __unsafe_unretained id<MTLTexture> mtl_tex, const sg_image_content* content) {
    const int num_faces = (img->type == SG_IMAGETYPE_CUBE) ? 6:1;
    const int num_slices = (img->type == SG_IMAGETYPE_ARRAY) ? img->depth : 1;
    for (int face_index = 0; face_index < num_faces; face_index++) {
        for (int mip_index = 0; mip_index < img->num_mipmaps; mip_index++) {
            SOKOL_ASSERT(content->subimage[face_index][mip_index].ptr);
            SOKOL_ASSERT(content->subimage[face_index][mip_index].size > 0);
            const uint8_t* data_ptr = (const uint8_t*)content->subimage[face_index][mip_index].ptr;
            const int mip_width = _sg_max(img->width >> mip_index, 1);
            const int mip_height = _sg_max(img->height >> mip_index, 1);
            /* special case PVRTC formats: bytePerRow must be 0 */
            int bytes_per_row = 0;
            int bytes_per_slice = _sg_surface_pitch(img->pixel_format, mip_width, mip_height);
            if (!_sg_mtl_is_pvrtc(img->pixel_format)) {
                bytes_per_row = _sg_row_pitch(img->pixel_format, mip_width);
            }
            MTLRegion region;
            if (img->type == SG_IMAGETYPE_3D) {
                const int mip_depth = _sg_max(img->depth >> mip_index, 1);
                region = MTLRegionMake3D(0, 0, 0, mip_width, mip_height, mip_depth);
                /* FIXME: apparently the minimal bytes_per_image size for 3D texture
                 is 4 KByte... somehow need to handle this */
            }
            else {
                region = MTLRegionMake2D(0, 0, mip_width, mip_height);
            }
            for (int slice_index = 0; slice_index < num_slices; slice_index++) {
                const int mtl_slice_index = (img->type == SG_IMAGETYPE_CUBE) ? face_index : slice_index;
                const int slice_offset = slice_index * bytes_per_slice;
                SOKOL_ASSERT((slice_offset + bytes_per_slice) <= (int)content->subimage[face_index][mip_index].size);
                [mtl_tex replaceRegion:region
                    mipmapLevel:mip_index
                    slice:mtl_slice_index
                    withBytes:data_ptr + slice_offset
                    bytesPerRow:bytes_per_row
                    bytesPerImage:bytes_per_slice];
            }
        }
    }
}

_SOKOL_PRIVATE void _sg_create_image(_sg_image* img, const sg_image_desc* desc) {
    SOKOL_ASSERT(img && desc);
    SOKOL_ASSERT(img->slot.state == SG_RESOURCESTATE_ALLOC);
    img->type = _sg_def(desc->type, SG_IMAGETYPE_2D);
    img->render_target = desc->render_target;
    img->width = desc->width;
    img->height = desc->height;
    img->depth = _sg_def(desc->depth, 1);
    img->num_mipmaps = _sg_def(desc->num_mipmaps, 1);
    img->usage = _sg_def(desc->usage, SG_USAGE_IMMUTABLE);
    img->pixel_format = _sg_def(desc->pixel_format, SG_PIXELFORMAT_RGBA8);
    img->sample_count = _sg_def(desc->sample_count, 1);
    img->min_filter = _sg_def(desc->min_filter, SG_FILTER_NEAREST);
    img->mag_filter = _sg_def(desc->mag_filter, SG_FILTER_NEAREST);
    img->wrap_u = _sg_def(desc->wrap_u, SG_WRAP_REPEAT);
    img->wrap_v = _sg_def(desc->wrap_v, SG_WRAP_REPEAT);
    img->wrap_w = _sg_def(desc->wrap_w, SG_WRAP_REPEAT);
    img->max_anisotropy = _sg_def(desc->max_anisotropy, 1);
    img->upd_frame_index = 0;
    img->num_slots = (img->usage == SG_USAGE_IMMUTABLE) ? 1 :SG_NUM_INFLIGHT_FRAMES;
    img->active_slot = 0;
    const bool injected = (0 != desc->mtl_textures[0]);

    /* first initialize all Metal resource pool slots to 'empty' */
    for (int i = 0; i < SG_NUM_INFLIGHT_FRAMES; i++) {
        img->mtl_tex[i] = _sg_mtl_add_resource(nil);
    }
    img->mtl_sampler_state = _sg_mtl_add_resource(nil);
    img->mtl_depth_tex = _sg_mtl_add_resource(nil);
    img->mtl_msaa_tex = _sg_mtl_add_resource(nil);

    /* initialize a Metal texture descriptor with common attributes */
    MTLTextureDescriptor* mtl_desc = [[MTLTextureDescriptor alloc] init];
    mtl_desc.textureType = _sg_mtl_texture_type(img->type);
    if (img->render_target) {
        if (_sg_is_valid_rendertarget_color_format(img->pixel_format)) {
            mtl_desc.pixelFormat = _sg_mtl_rendertarget_color_format(img->pixel_format);
        }
        else {
            mtl_desc.pixelFormat = _sg_mtl_rendertarget_depth_format(img->pixel_format);
        }
    }
    else {
        mtl_desc.pixelFormat = _sg_mtl_texture_format(img->pixel_format);
    }
    if (MTLPixelFormatInvalid == mtl_desc.pixelFormat) {
        SOKOL_LOG("Unsupported texture pixel format!\n");
        img->slot.state = SG_RESOURCESTATE_FAILED;
        return;
    }
    mtl_desc.width = img->width;
    mtl_desc.height = img->height;
    if (SG_IMAGETYPE_3D == img->type) {
        mtl_desc.depth = img->depth;
    }
    else {
        mtl_desc.depth = 1;
    }
    mtl_desc.mipmapLevelCount = img->num_mipmaps;
    if (SG_IMAGETYPE_ARRAY == img->type) {
        mtl_desc.arrayLength = img->depth;
    }
    else {
        mtl_desc.arrayLength = 1;
    }
    if (img->render_target) {
        mtl_desc.resourceOptions = MTLResourceStorageModePrivate;
        mtl_desc.cpuCacheMode = MTLCPUCacheModeDefaultCache;
        mtl_desc.storageMode = MTLStorageModePrivate;
        mtl_desc.usage |= MTLTextureUsageRenderTarget;
    }

    /* special case depth-stencil-buffer? */
    if (_sg_is_valid_rendertarget_depth_format(img->pixel_format)) {
        /* create only a depth texture */
        SOKOL_ASSERT(img->render_target);
        SOKOL_ASSERT(img->type == SG_IMAGETYPE_2D);
        SOKOL_ASSERT(img->num_mipmaps == 1);
        SOKOL_ASSERT(!injected);
        if (img->sample_count > 1) {
            mtl_desc.textureType = MTLTextureType2DMultisample;
            mtl_desc.sampleCount = img->sample_count;
        }
        id<MTLTexture> tex = [_sg_mtl_device newTextureWithDescriptor:mtl_desc];
        SOKOL_ASSERT(nil != tex);
        img->mtl_depth_tex = _sg_mtl_add_resource(tex);
    }
    else {
        /* create the color texture(s) */
        for (int slot = 0; slot < img->num_slots; slot++) {
            id<MTLTexture> tex;
            if (injected) {
                SOKOL_ASSERT(desc->mtl_textures[slot]);
                tex = (__bridge id<MTLTexture>) desc->mtl_textures[slot];
            }
            else {
                tex = [_sg_mtl_device newTextureWithDescriptor:mtl_desc];
                if ((img->usage == SG_USAGE_IMMUTABLE) && !img->render_target) {
                    _sg_mtl_copy_image_content(img, tex, &desc->content);
                }
            }
            img->mtl_tex[slot] = _sg_mtl_add_resource(tex);
        }

        /* if MSAA color render target, create an additional MSAA render-surface texture */
        if (img->render_target && (img->sample_count > 1)) {
            mtl_desc.textureType = MTLTextureType2DMultisample;
            mtl_desc.depth = 1;
            mtl_desc.arrayLength = 1;
            mtl_desc.mipmapLevelCount = 1;
            mtl_desc.sampleCount = img->sample_count;
            id<MTLTexture> tex = [_sg_mtl_device newTextureWithDescriptor:mtl_desc];
            img->mtl_msaa_tex = _sg_mtl_add_resource(tex);
        }

        /* create (possibly shared) sampler state */
        img->mtl_sampler_state = _sg_mtl_create_sampler(_sg_mtl_device, desc);
    }
    img->slot.state = SG_RESOURCESTATE_VALID;
}

_SOKOL_PRIVATE void _sg_destroy_image(_sg_image* img) {
    SOKOL_ASSERT(img);
    if (img->slot.state == SG_RESOURCESTATE_VALID) {
        for (int slot = 0; slot < img->num_slots; slot++) {
            _sg_mtl_release_resource(_sg_mtl_frame_index, img->mtl_tex[slot]);
        }
        _sg_mtl_release_resource(_sg_mtl_frame_index, img->mtl_depth_tex);
        _sg_mtl_release_resource(_sg_mtl_frame_index, img->mtl_msaa_tex);
        /* NOTE: sampler state objects are shared and not released until shutdown */
    }
    _sg_init_image_slot(img);
}

_SOKOL_PRIVATE id<MTLLibrary> _sg_mtl_compile_library(const char* src) {
    NSError* err = NULL;
    id<MTLLibrary> lib = [_sg_mtl_device
        newLibraryWithSource:[NSString stringWithUTF8String:src]
        options:nil
        error:&err
    ];
    if (err) {
        SOKOL_LOG([err.localizedDescription UTF8String]);
    }
    return lib;
}

_SOKOL_PRIVATE id<MTLLibrary> _sg_mtl_library_from_bytecode(const uint8_t* ptr, int num_bytes) {
    NSError* err = NULL;
    dispatch_data_t lib_data = dispatch_data_create(ptr, num_bytes, NULL, DISPATCH_DATA_DESTRUCTOR_DEFAULT);
    id<MTLLibrary> lib = [_sg_mtl_device newLibraryWithData:lib_data error:&err];
    if (err) {
        SOKOL_LOG([err.localizedDescription UTF8String]);
    }
    return lib;
}

_SOKOL_PRIVATE void _sg_create_shader(_sg_shader* shd, const sg_shader_desc* desc) {
    SOKOL_ASSERT(shd && desc);
    SOKOL_ASSERT(shd->slot.state == SG_RESOURCESTATE_ALLOC);

    /* uniform block sizes and image types */
    for (int stage_index = 0; stage_index < SG_NUM_SHADER_STAGES; stage_index++) {
        const sg_shader_stage_desc* stage_desc = (stage_index == SG_SHADERSTAGE_VS) ? &desc->vs : &desc->fs;
        _sg_shader_stage* stage = &shd->stage[stage_index];
        SOKOL_ASSERT(stage->num_uniform_blocks == 0);
        for (int ub_index = 0; ub_index < SG_MAX_SHADERSTAGE_UBS; ub_index++) {
            const sg_shader_uniform_block_desc* ub_desc = &stage_desc->uniform_blocks[ub_index];
            if (0 == ub_desc->size) {
                break;
            }
            _sg_uniform_block* ub = &stage->uniform_blocks[ub_index];
            ub->size = ub_desc->size;
            stage->num_uniform_blocks++;
        }
        SOKOL_ASSERT(stage->num_images == 0);
        for (int img_index = 0; img_index < SG_MAX_SHADERSTAGE_IMAGES; img_index++) {
            const sg_shader_image_desc* img_desc = &stage_desc->images[img_index];
            if (img_desc->type == _SG_IMAGETYPE_DEFAULT) {
                break;
            }
            stage->images[img_index].type = img_desc->type;
            stage->num_images++;
        }
    }

    /* create metal libray objects and lookup entry functions */
    id<MTLLibrary> vs_lib;
    id<MTLLibrary> fs_lib;
    id<MTLFunction> vs_func;
    id<MTLFunction> fs_func;
    const char* vs_entry = _sg_def(desc->vs.entry, "_main");
    const char* fs_entry = _sg_def(desc->fs.entry, "_main");
    if (desc->vs.byte_code && desc->fs.byte_code) {
        /* separate byte code provided */
        vs_lib = _sg_mtl_library_from_bytecode(desc->vs.byte_code, desc->vs.byte_code_size);
        fs_lib = _sg_mtl_library_from_bytecode(desc->fs.byte_code, desc->fs.byte_code_size);
        if (nil == vs_lib || nil == fs_lib) {
            shd->slot.state = SG_RESOURCESTATE_FAILED;
            return;
        }
        vs_func = [vs_lib newFunctionWithName:[NSString stringWithUTF8String:vs_entry]];
        fs_func = [fs_lib newFunctionWithName:[NSString stringWithUTF8String:fs_entry]];
    }
    else if (desc->vs.source && desc->fs.source) {
        /* separate sources provided */
        vs_lib = _sg_mtl_compile_library(desc->vs.source);
        fs_lib = _sg_mtl_compile_library(desc->fs.source);
        if (nil == vs_lib || nil == fs_lib) {
            shd->slot.state = SG_RESOURCESTATE_FAILED;
            return;
        }
        vs_func = [vs_lib newFunctionWithName:[NSString stringWithUTF8String:vs_entry]];
        fs_func = [fs_lib newFunctionWithName:[NSString stringWithUTF8String:fs_entry]];
    }
    else {
        shd->slot.state = SG_RESOURCESTATE_FAILED;
        return;
    }
    if (nil == vs_func) {
        SOKOL_LOG("vertex shader entry function not found\n");
        shd->slot.state = SG_RESOURCESTATE_FAILED;
        return;
    }
    if (nil == fs_func) {
        SOKOL_LOG("fragment shader entry function not found\n");
        shd->slot.state = SG_RESOURCESTATE_FAILED;
        return;
    }
    /* it is legal to call _sg_mtl_add_resource with a nil value, this will return a special 0xFFFFFFFF index */
    shd->stage[SG_SHADERSTAGE_VS].mtl_lib  = _sg_mtl_add_resource(vs_lib);
    shd->stage[SG_SHADERSTAGE_FS].mtl_lib  = _sg_mtl_add_resource(fs_lib);
    shd->stage[SG_SHADERSTAGE_VS].mtl_func = _sg_mtl_add_resource(vs_func);
    shd->stage[SG_SHADERSTAGE_FS].mtl_func = _sg_mtl_add_resource(fs_func);
    shd->slot.state = SG_RESOURCESTATE_VALID;
}

_SOKOL_PRIVATE void _sg_destroy_shader(_sg_shader* shd) {
    SOKOL_ASSERT(shd);
    if (shd->slot.state == SG_RESOURCESTATE_VALID) {
        /* it is valid to call _sg_mtl_release_resource with the special 0xFFFFFFFF index */
        _sg_mtl_release_resource(_sg_mtl_frame_index, shd->stage[SG_SHADERSTAGE_VS].mtl_func);
        _sg_mtl_release_resource(_sg_mtl_frame_index, shd->stage[SG_SHADERSTAGE_VS].mtl_lib);
        _sg_mtl_release_resource(_sg_mtl_frame_index, shd->stage[SG_SHADERSTAGE_FS].mtl_func);
        _sg_mtl_release_resource(_sg_mtl_frame_index, shd->stage[SG_SHADERSTAGE_FS].mtl_lib);
    }
    _sg_init_shader_slot(shd);
}

_SOKOL_PRIVATE void _sg_create_pipeline(_sg_pipeline* pip, _sg_shader* shd, const sg_pipeline_desc* desc) {
    SOKOL_ASSERT(pip && shd && desc);
    SOKOL_ASSERT(pip->slot.state == SG_RESOURCESTATE_ALLOC);
    SOKOL_ASSERT(desc->shader.id == shd->slot.id);
    SOKOL_ASSERT(shd->slot.state == SG_RESOURCESTATE_VALID);

    pip->shader = shd;
    pip->shader_id = desc->shader;
    pip->color_attachment_count = _sg_def(desc->blend.color_attachment_count, 1);
    pip->color_format = _sg_def(desc->blend.color_format, SG_PIXELFORMAT_RGBA8);
    pip->depth_format = _sg_def(desc->blend.depth_format, SG_PIXELFORMAT_DEPTHSTENCIL);
    pip->sample_count = _sg_def(desc->rasterizer.sample_count, 1);
    pip->depth_bias = desc->rasterizer.depth_bias;
    pip->depth_bias_slope_scale = desc->rasterizer.depth_bias_slope_scale;
    pip->depth_bias_clamp = desc->rasterizer.depth_bias_clamp;
    sg_primitive_type prim_type = _sg_def(desc->primitive_type, SG_PRIMITIVETYPE_TRIANGLES);
    pip->mtl_prim_type = _sg_mtl_primitive_type(prim_type);
    pip->index_type = _sg_def(desc->index_type, SG_INDEXTYPE_NONE);
    pip->mtl_index_size = _sg_mtl_index_size(pip->index_type);
    if (SG_INDEXTYPE_NONE != pip->index_type) {
        pip->mtl_index_type = _sg_mtl_index_type(pip->index_type);
    }
    pip->mtl_cull_mode = _sg_mtl_cull_mode(_sg_def(desc->rasterizer.cull_mode, SG_CULLMODE_NONE));
    pip->mtl_winding = _sg_mtl_winding(_sg_def(desc->rasterizer.face_winding, SG_FACEWINDING_CW));
    pip->mtl_stencil_ref = desc->depth_stencil.stencil_ref;
    for (int i = 0; i < 4; i++) {
        pip->blend_color[i] = desc->blend.blend_color[i];
    }

    /* create vertex-descriptor */
    MTLVertexDescriptor* vtx_desc = [MTLVertexDescriptor vertexDescriptor];
    int auto_offset[SG_MAX_SHADERSTAGE_BUFFERS];
    for (int layout_index = 0; layout_index < SG_MAX_SHADERSTAGE_BUFFERS; layout_index++) {
        auto_offset[layout_index] = 0;
    }
    /* to use computed offsets, *all* attr offsets must be 0 */
    bool use_auto_offset = true;
    for (int attr_index = 0; attr_index < SG_MAX_VERTEX_ATTRIBUTES; attr_index++) {
        if (desc->layout.attrs[attr_index].offset != 0) {
            use_auto_offset = false;
            break;
        }
    }
    for (int attr_index = 0; attr_index < SG_MAX_VERTEX_ATTRIBUTES; attr_index++) {
        const sg_vertex_attr_desc* a_desc = &desc->layout.attrs[attr_index];
        if (a_desc->format == SG_VERTEXFORMAT_INVALID) {
            break;
        }
        SOKOL_ASSERT((a_desc->buffer_index >= 0) && (a_desc->buffer_index < SG_MAX_SHADERSTAGE_BUFFERS));
        vtx_desc.attributes[attr_index].format = _sg_mtl_vertex_format(a_desc->format);
        vtx_desc.attributes[attr_index].offset = use_auto_offset ? auto_offset[a_desc->buffer_index] : a_desc->offset;
        vtx_desc.attributes[attr_index].bufferIndex = a_desc->buffer_index + SG_MAX_SHADERSTAGE_UBS;
        auto_offset[a_desc->buffer_index] += _sg_vertexformat_bytesize(a_desc->format);
        pip->vertex_layout_valid[a_desc->buffer_index] = true;
    }
    for (int layout_index = 0; layout_index < SG_MAX_SHADERSTAGE_BUFFERS; layout_index++) {
        if (pip->vertex_layout_valid[layout_index]) {
            const sg_buffer_layout_desc* l_desc = &desc->layout.buffers[layout_index];
            const int mtl_vb_slot = layout_index + SG_MAX_SHADERSTAGE_UBS;
            const int stride = l_desc->stride ? l_desc->stride : auto_offset[layout_index];
            SOKOL_ASSERT(stride > 0);
            vtx_desc.layouts[mtl_vb_slot].stride = stride;
            vtx_desc.layouts[mtl_vb_slot].stepFunction = _sg_mtl_step_function(_sg_def(l_desc->step_func, SG_VERTEXSTEP_PER_VERTEX));
            vtx_desc.layouts[mtl_vb_slot].stepRate = _sg_def(l_desc->step_rate, 1);
        }
    }

    /* render-pipeline descriptor */
    MTLRenderPipelineDescriptor* rp_desc = [[MTLRenderPipelineDescriptor alloc] init];
    rp_desc.vertexDescriptor = vtx_desc;
    SOKOL_ASSERT(shd->stage[SG_SHADERSTAGE_VS].mtl_func != _SG_MTL_INVALID_POOL_INDEX);
    rp_desc.vertexFunction = _sg_mtl_pool[shd->stage[SG_SHADERSTAGE_VS].mtl_func];
    SOKOL_ASSERT(shd->stage[SG_SHADERSTAGE_FS].mtl_func != _SG_MTL_INVALID_POOL_INDEX);
    rp_desc.fragmentFunction = _sg_mtl_pool[shd->stage[SG_SHADERSTAGE_FS].mtl_func];
    rp_desc.sampleCount = _sg_def(desc->rasterizer.sample_count, 1);
    rp_desc.alphaToCoverageEnabled = desc->rasterizer.alpha_to_coverage_enabled;
    rp_desc.alphaToOneEnabled = NO;
    rp_desc.rasterizationEnabled = YES;
    rp_desc.depthAttachmentPixelFormat = _sg_mtl_rendertarget_depth_format(_sg_def(desc->blend.depth_format, SG_PIXELFORMAT_DEPTHSTENCIL));
    rp_desc.stencilAttachmentPixelFormat = _sg_mtl_rendertarget_stencil_format(_sg_def(desc->blend.depth_format, SG_PIXELFORMAT_DEPTHSTENCIL));
    /* FIXME: this only works on macOS 10.13!
    for (int i = 0; i < (SG_MAX_SHADERSTAGE_UBS+SG_MAX_SHADERSTAGE_BUFFERS); i++) {
        rp_desc.vertexBuffers[i].mutability = MTLMutabilityImmutable;
    }
    for (int i = 0; i < SG_MAX_SHADERSTAGE_UBS; i++) {
        rp_desc.fragmentBuffers[i].mutability = MTLMutabilityImmutable;
    }
    */
    const int att_count = _sg_def(desc->blend.color_attachment_count, 1);
    for (int i = 0; i < att_count; i++) {
        rp_desc.colorAttachments[i].pixelFormat = _sg_mtl_rendertarget_color_format(_sg_def(desc->blend.color_format, SG_PIXELFORMAT_RGBA8));
        rp_desc.colorAttachments[i].writeMask = _sg_mtl_color_write_mask((sg_color_mask)_sg_def(desc->blend.color_write_mask, SG_COLORMASK_RGBA));
        rp_desc.colorAttachments[i].blendingEnabled = desc->blend.enabled;
        rp_desc.colorAttachments[i].alphaBlendOperation = _sg_mtl_blend_op(_sg_def(desc->blend.op_alpha, SG_BLENDOP_ADD));
        rp_desc.colorAttachments[i].rgbBlendOperation = _sg_mtl_blend_op(_sg_def(desc->blend.op_rgb, SG_BLENDOP_ADD));
        rp_desc.colorAttachments[i].destinationAlphaBlendFactor = _sg_mtl_blend_factor(_sg_def(desc->blend.dst_factor_alpha, SG_BLENDFACTOR_ZERO));
        rp_desc.colorAttachments[i].destinationRGBBlendFactor = _sg_mtl_blend_factor(_sg_def(desc->blend.dst_factor_rgb, SG_BLENDFACTOR_ZERO));
        rp_desc.colorAttachments[i].sourceAlphaBlendFactor = _sg_mtl_blend_factor(_sg_def(desc->blend.src_factor_alpha, SG_BLENDFACTOR_ONE));
        rp_desc.colorAttachments[i].sourceRGBBlendFactor = _sg_mtl_blend_factor(_sg_def(desc->blend.src_factor_rgb, SG_BLENDFACTOR_ONE));
    }
    NSError* err = NULL;
    id<MTLRenderPipelineState> mtl_rps = [_sg_mtl_device newRenderPipelineStateWithDescriptor:rp_desc error:&err];
    if (nil == mtl_rps) {
        SOKOL_ASSERT(err);
        SOKOL_LOG([err.localizedDescription UTF8String]);
        pip->slot.state = SG_RESOURCESTATE_FAILED;
        return;
    }

    /* depth-stencil-state */
    MTLDepthStencilDescriptor* ds_desc = [[MTLDepthStencilDescriptor alloc] init];
    ds_desc.depthCompareFunction = _sg_mtl_compare_func(_sg_def(desc->depth_stencil.depth_compare_func, SG_COMPAREFUNC_ALWAYS));
    ds_desc.depthWriteEnabled = desc->depth_stencil.depth_write_enabled;
    if (desc->depth_stencil.stencil_enabled) {
        const sg_stencil_state* sb = &desc->depth_stencil.stencil_back;
        ds_desc.backFaceStencil = [[MTLStencilDescriptor alloc] init];
        ds_desc.backFaceStencil.stencilFailureOperation = _sg_mtl_stencil_op(_sg_def(sb->fail_op, SG_STENCILOP_KEEP));
        ds_desc.backFaceStencil.depthFailureOperation = _sg_mtl_stencil_op(_sg_def(sb->depth_fail_op, SG_STENCILOP_KEEP));
        ds_desc.backFaceStencil.depthStencilPassOperation = _sg_mtl_stencil_op(_sg_def(sb->pass_op, SG_STENCILOP_KEEP));
        ds_desc.backFaceStencil.stencilCompareFunction = _sg_mtl_compare_func(_sg_def(sb->compare_func, SG_COMPAREFUNC_ALWAYS));
        ds_desc.backFaceStencil.readMask = desc->depth_stencil.stencil_read_mask;
        ds_desc.backFaceStencil.writeMask = desc->depth_stencil.stencil_write_mask;
        const sg_stencil_state* sf = &desc->depth_stencil.stencil_front;
        ds_desc.frontFaceStencil = [[MTLStencilDescriptor alloc] init];
        ds_desc.frontFaceStencil.stencilFailureOperation = _sg_mtl_stencil_op(_sg_def(sf->fail_op, SG_STENCILOP_KEEP));
        ds_desc.frontFaceStencil.depthFailureOperation = _sg_mtl_stencil_op(_sg_def(sf->depth_fail_op, SG_STENCILOP_KEEP));
        ds_desc.frontFaceStencil.depthStencilPassOperation = _sg_mtl_stencil_op(_sg_def(sf->pass_op, SG_STENCILOP_KEEP));
        ds_desc.frontFaceStencil.stencilCompareFunction = _sg_mtl_compare_func(_sg_def(sf->compare_func, SG_COMPAREFUNC_ALWAYS));
        ds_desc.frontFaceStencil.readMask = desc->depth_stencil.stencil_read_mask;
        ds_desc.frontFaceStencil.writeMask = desc->depth_stencil.stencil_write_mask;
    }
    id<MTLDepthStencilState> mtl_dss = [_sg_mtl_device newDepthStencilStateWithDescriptor:ds_desc];

    pip->mtl_rps = _sg_mtl_add_resource(mtl_rps);
    pip->mtl_dss = _sg_mtl_add_resource(mtl_dss);
    pip->slot.state = SG_RESOURCESTATE_VALID;
}

_SOKOL_PRIVATE void _sg_destroy_pipeline(_sg_pipeline* pip) {
    SOKOL_ASSERT(pip);
    if (pip->slot.state == SG_RESOURCESTATE_VALID) {
        _sg_mtl_release_resource(_sg_mtl_frame_index, pip->mtl_rps);
        _sg_mtl_release_resource(_sg_mtl_frame_index, pip->mtl_dss);
    }
    _sg_init_pipeline_slot(pip);
}

_SOKOL_PRIVATE void _sg_create_pass(_sg_pass* pass, _sg_image** att_images, const sg_pass_desc* desc) {
    SOKOL_ASSERT(pass && desc);
    SOKOL_ASSERT(pass->slot.state == SG_RESOURCESTATE_ALLOC);
    SOKOL_ASSERT(att_images && att_images[0]);

    /* copy image pointers and desc attributes */
    const sg_attachment_desc* att_desc;
    _sg_attachment* att;
    for (int i = 0; i < SG_MAX_COLOR_ATTACHMENTS; i++) {
        SOKOL_ASSERT(0 == pass->color_atts[i].image);
        att_desc = &desc->color_attachments[i];
        if (att_desc->image.id != SG_INVALID_ID) {
            pass->num_color_atts++;
            SOKOL_ASSERT(att_images[i] && (att_images[i]->slot.id == att_desc->image.id));
            SOKOL_ASSERT(_sg_is_valid_rendertarget_color_format(att_images[i]->pixel_format));
            att = &pass->color_atts[i];
            SOKOL_ASSERT((att->image == 0) && (att->image_id.id == SG_INVALID_ID));
            att->image = att_images[i];
            att->image_id = att_desc->image;
            att->mip_level = att_desc->mip_level;
            att->slice = att_desc->slice;
        }
    }
    SOKOL_ASSERT(0 == pass->ds_att.image);
    att_desc = &desc->depth_stencil_attachment;
    const int ds_img_index = SG_MAX_COLOR_ATTACHMENTS;
    if (att_desc->image.id != SG_INVALID_ID) {
        SOKOL_ASSERT(att_images[ds_img_index] && (att_images[ds_img_index]->slot.id == att_desc->image.id));
        SOKOL_ASSERT(_sg_is_valid_rendertarget_depth_format(att_images[ds_img_index]->pixel_format));
        att = &pass->ds_att;
        SOKOL_ASSERT((att->image == 0) && (att->image_id.id == SG_INVALID_ID));
        att->image = att_images[ds_img_index];
        att->image_id = att_desc->image;
        att->mip_level = att_desc->mip_level;
        att->slice = att_desc->slice;
    }
    pass->slot.state = SG_RESOURCESTATE_VALID;
}

_SOKOL_PRIVATE void _sg_destroy_pass(_sg_pass* pass) {
    SOKOL_ASSERT(pass);
    _sg_init_pass_slot(pass);
}

_SOKOL_PRIVATE void _sg_begin_pass(_sg_pass* pass, const sg_pass_action* action, int w, int h) {
    SOKOL_ASSERT(action);
    SOKOL_ASSERT(!_sg_mtl_in_pass);
    SOKOL_ASSERT(_sg_mtl_cmd_queue);
    SOKOL_ASSERT(!_sg_mtl_cmd_encoder);
    SOKOL_ASSERT(_sg_mtl_renderpass_descriptor_cb);
    _sg_mtl_in_pass = true;
    _sg_mtl_cur_width = w;
    _sg_mtl_cur_height = h;
    _sg_mtl_clear_state_cache();

    /* if this is the first pass in the frame, create a command buffer */
    if (nil == _sg_mtl_cmd_buffer) {
        /* block until the oldest frame in flight has finished */
        dispatch_semaphore_wait(_sg_mtl_sem, DISPATCH_TIME_FOREVER);
        _sg_mtl_cmd_buffer = [_sg_mtl_cmd_queue commandBufferWithUnretainedReferences];
    }

    /* if this is first pass in frame, get uniform buffer base pointer */
    if (0 == _sg_mtl_cur_ub_base_ptr) {
        _sg_mtl_cur_ub_base_ptr = (uint8_t*)[_sg_mtl_uniform_buffers[_sg_mtl_cur_frame_rotate_index] contents];
    }

    /* initialize a render pass descriptor */
    MTLRenderPassDescriptor* pass_desc = nil;
    if (pass) {
        /* offscreen render pass */
        pass_desc = [MTLRenderPassDescriptor renderPassDescriptor];
    }
    else {
        /* default render pass, call user-provided callback to provide render pass descriptor */
        pass_desc = (__bridge MTLRenderPassDescriptor*) _sg_mtl_renderpass_descriptor_cb();

    }
    if (pass_desc) {
        _sg_mtl_pass_valid = true;
    }
    else {
        /* default pass descriptor will not be valid if window is minized,
           don't do any rendering in this case */
        _sg_mtl_pass_valid = false;
        return;
    }
    if (pass) {
        /* setup pass descriptor for offscreen rendering */
        SOKOL_ASSERT(pass->slot.state == SG_RESOURCESTATE_VALID);
        for (int i = 0; i < SG_MAX_COLOR_ATTACHMENTS; i++) {
            const _sg_attachment* att = &pass->color_atts[i];
            if (0 == att->image) {
                break;
            }
            SOKOL_ASSERT(att->image->slot.state == SG_RESOURCESTATE_VALID);
            SOKOL_ASSERT(att->image->slot.id == att->image_id.id);
            const bool is_msaa = (att->image->sample_count > 1);
            pass_desc.colorAttachments[i].loadAction = _sg_mtl_load_action(action->colors[i].action);
            pass_desc.colorAttachments[i].storeAction = is_msaa ? MTLStoreActionMultisampleResolve : MTLStoreActionStore;
            const float* c = &(action->colors[i].val[0]);
            pass_desc.colorAttachments[i].clearColor = MTLClearColorMake(c[0], c[1], c[2], c[3]);
            if (is_msaa) {
                SOKOL_ASSERT(att->image->mtl_msaa_tex != _SG_MTL_INVALID_POOL_INDEX);
                SOKOL_ASSERT(att->image->mtl_tex[att->image->active_slot] != _SG_MTL_INVALID_POOL_INDEX);
                pass_desc.colorAttachments[i].texture = _sg_mtl_pool[att->image->mtl_msaa_tex];
                pass_desc.colorAttachments[i].resolveTexture = _sg_mtl_pool[att->image->mtl_tex[att->image->active_slot]];
                pass_desc.colorAttachments[i].resolveLevel = att->mip_level;
                switch (att->image->type) {
                    case SG_IMAGETYPE_CUBE:
                    case SG_IMAGETYPE_ARRAY:
                        pass_desc.colorAttachments[i].resolveSlice = att->slice;
                        break;
                    case SG_IMAGETYPE_3D:
                        pass_desc.colorAttachments[i].resolveDepthPlane = att->slice;
                        break;
                    default: break;
                }
            }
            else {
                SOKOL_ASSERT(att->image->mtl_tex[att->image->active_slot] != _SG_MTL_INVALID_POOL_INDEX);
                pass_desc.colorAttachments[i].texture = _sg_mtl_pool[att->image->mtl_tex[att->image->active_slot]];
                pass_desc.colorAttachments[i].level = att->mip_level;
                switch (att->image->type) {
                    case SG_IMAGETYPE_CUBE:
                    case SG_IMAGETYPE_ARRAY:
                        pass_desc.colorAttachments[i].slice = att->slice;
                        break;
                    case SG_IMAGETYPE_3D:
                        pass_desc.colorAttachments[i].depthPlane = att->slice;
                        break;
                    default: break;
                }
            }
        }
        if (0 != pass->ds_att.image) {
            const _sg_attachment* att = &pass->ds_att;
            SOKOL_ASSERT(att->image->slot.state == SG_RESOURCESTATE_VALID);
            SOKOL_ASSERT(att->image->slot.id == att->image_id.id);
            SOKOL_ASSERT(att->image->mtl_depth_tex != _SG_MTL_INVALID_POOL_INDEX);
            pass_desc.depthAttachment.texture = _sg_mtl_pool[att->image->mtl_depth_tex];
            pass_desc.depthAttachment.loadAction = _sg_mtl_load_action(action->depth.action);
            pass_desc.depthAttachment.clearDepth = action->depth.val;
            if (_sg_is_depth_stencil_format(att->image->pixel_format)) {
                pass_desc.stencilAttachment.texture = _sg_mtl_pool[att->image->mtl_depth_tex];
                pass_desc.stencilAttachment.loadAction = _sg_mtl_load_action(action->stencil.action);
                pass_desc.stencilAttachment.clearStencil = action->stencil.val;
            }
        }
    }
    else {
        /* setup pass descriptor for default rendering */
        pass_desc.colorAttachments[0].loadAction = _sg_mtl_load_action(action->colors[0].action);
        const float* c = &(action->colors[0].val[0]);
        pass_desc.colorAttachments[0].clearColor = MTLClearColorMake(c[0], c[1], c[2], c[3]);
        pass_desc.depthAttachment.loadAction = _sg_mtl_load_action(action->depth.action);
        pass_desc.depthAttachment.clearDepth = action->depth.val;
        pass_desc.stencilAttachment.loadAction = _sg_mtl_load_action(action->stencil.action);
        pass_desc.stencilAttachment.clearStencil = action->stencil.val;
    }

    /* create a render command encoder, this might return nil if window is minimized */
    _sg_mtl_cmd_encoder = [_sg_mtl_cmd_buffer renderCommandEncoderWithDescriptor:pass_desc];
    if (_sg_mtl_cmd_encoder == nil) {
        _sg_mtl_pass_valid = false;
        return;
    }

    /* bind the global uniform buffer, this only happens once per pass */
    for (int slot = 0; slot < SG_MAX_SHADERSTAGE_UBS; slot++) {
        [_sg_mtl_cmd_encoder
            setVertexBuffer:_sg_mtl_uniform_buffers[_sg_mtl_cur_frame_rotate_index]
            offset:0
            atIndex:slot];
        [_sg_mtl_cmd_encoder
            setFragmentBuffer:_sg_mtl_uniform_buffers[_sg_mtl_cur_frame_rotate_index]
            offset:0
            atIndex:slot];
    }
}

_SOKOL_PRIVATE void _sg_end_pass() {
    SOKOL_ASSERT(_sg_mtl_in_pass);
    _sg_mtl_in_pass = false;
    _sg_mtl_pass_valid = false;
    if (nil != _sg_mtl_cmd_encoder) {
        [_sg_mtl_cmd_encoder endEncoding];
        _sg_mtl_cmd_encoder = nil;
    }
}

_SOKOL_PRIVATE void _sg_commit() {
    SOKOL_ASSERT(!_sg_mtl_in_pass);
    SOKOL_ASSERT(!_sg_mtl_pass_valid);
    SOKOL_ASSERT(_sg_mtl_drawable_cb);
    SOKOL_ASSERT(nil == _sg_mtl_cmd_encoder);
    SOKOL_ASSERT(nil != _sg_mtl_cmd_buffer);

    #if !TARGET_OS_IPHONE
    [_sg_mtl_uniform_buffers[_sg_mtl_cur_frame_rotate_index] didModifyRange:NSMakeRange(0, _sg_mtl_cur_ub_offset)];
    #endif

    /* present, commit and signal semaphore when done */
    id<MTLDrawable> cur_drawable = (__bridge id<MTLDrawable>) _sg_mtl_drawable_cb();
    [_sg_mtl_cmd_buffer presentDrawable:cur_drawable];
    __block dispatch_semaphore_t sem = _sg_mtl_sem;
    [_sg_mtl_cmd_buffer addCompletedHandler:^(id<MTLCommandBuffer> cmd_buffer) {
        dispatch_semaphore_signal(sem);
    }];
    [_sg_mtl_cmd_buffer commit];

    /* garbage-collect resources pending for release */
    _sg_mtl_garbage_collect(_sg_mtl_frame_index);

    /* rotate uniform buffer slot */
    if (++_sg_mtl_cur_frame_rotate_index >= SG_NUM_INFLIGHT_FRAMES) {
        _sg_mtl_cur_frame_rotate_index = 0;
    }
    _sg_mtl_frame_index++;
    _sg_mtl_cur_ub_offset = 0;
    _sg_mtl_cur_ub_base_ptr = 0;
    _sg_mtl_cmd_buffer = nil;
}

_SOKOL_PRIVATE void _sg_apply_viewport(int x, int y, int w, int h, bool origin_top_left) {
    SOKOL_ASSERT(_sg_mtl_in_pass);
    if (!_sg_mtl_pass_valid) {
        return;
    }
    SOKOL_ASSERT(_sg_mtl_cmd_encoder);
    MTLViewport vp;
    vp.originX = (double) x;
    vp.originY = (double) (origin_top_left ? y : (_sg_mtl_cur_height - (y + h)));
    vp.width   = (double) w;
    vp.height  = (double) h;
    vp.znear   = 0.0;
    vp.zfar    = 1.0;
    [_sg_mtl_cmd_encoder setViewport:vp];
}

_SOKOL_PRIVATE void _sg_apply_scissor_rect(int x, int y, int w, int h, bool origin_top_left) {
    SOKOL_ASSERT(_sg_mtl_in_pass);
    if (!_sg_mtl_pass_valid) {
        return;
    }
    SOKOL_ASSERT(_sg_mtl_cmd_encoder);
    /* clip against framebuffer rect */
    x = _sg_min(_sg_max(0, x), _sg_mtl_cur_width-1);
    y = _sg_min(_sg_max(0, y), _sg_mtl_cur_height-1);
    if ((x + w) > _sg_mtl_cur_width) {
        w = _sg_mtl_cur_width - x;
    }
    if ((y + h) > _sg_mtl_cur_height) {
        h = _sg_mtl_cur_height - y;
    }
    w = _sg_max(w, 1);
    h = _sg_max(h, 1);

    MTLScissorRect r;
    r.x = x;
    r.y = origin_top_left ? y : (_sg_mtl_cur_height - (y + h));
    r.width = w;
    r.height = h;
    [_sg_mtl_cmd_encoder setScissorRect:r];
}

_SOKOL_PRIVATE void _sg_apply_draw_state(
    _sg_pipeline* pip,
    _sg_buffer** vbs, const uint32_t* vb_offsets, int num_vbs,
    _sg_buffer* ib, uint32_t ib_offset,
    _sg_image** vs_imgs, int num_vs_imgs,
    _sg_image** fs_imgs, int num_fs_imgs)
{
    SOKOL_ASSERT(pip);
    SOKOL_ASSERT(pip->shader);
    SOKOL_ASSERT(_sg_mtl_in_pass);
    if (!_sg_mtl_pass_valid) {
        return;
    }
    SOKOL_ASSERT(_sg_mtl_cmd_encoder);

    /* store index buffer binding, this will be needed later in sg_draw() */
    _sg_mtl_cur_indexbuffer = ib;
    _sg_mtl_cur_indexbuffer_offset = ib_offset;
    if (ib) {
        SOKOL_ASSERT(pip->index_type != SG_INDEXTYPE_NONE);
        _sg_mtl_cur_indexbuffer_id.id = ib->slot.id;
    }
    else {
        SOKOL_ASSERT(pip->index_type == SG_INDEXTYPE_NONE);
        _sg_mtl_cur_indexbuffer_id.id = SG_INVALID_ID;
    }

    /* apply pipeline state */
    if ((_sg_mtl_cur_pipeline != pip) || (_sg_mtl_cur_pipeline_id.id != pip->slot.id)) {
        _sg_mtl_cur_pipeline = pip;
        _sg_mtl_cur_pipeline_id.id = pip->slot.id;
        const float* c = pip->blend_color;
        /* FIXME: those should be filtered through a simple state cache */
        [_sg_mtl_cmd_encoder setBlendColorRed:c[0] green:c[1] blue:c[2] alpha:c[3]];
        [_sg_mtl_cmd_encoder setCullMode:pip->mtl_cull_mode];
        [_sg_mtl_cmd_encoder setFrontFacingWinding:pip->mtl_winding];
        [_sg_mtl_cmd_encoder setStencilReferenceValue:pip->mtl_stencil_ref];
        [_sg_mtl_cmd_encoder setDepthBias:pip->depth_bias slopeScale:pip->depth_bias_slope_scale clamp:pip->depth_bias_clamp];
        SOKOL_ASSERT(pip->mtl_rps != _SG_MTL_INVALID_POOL_INDEX);
        [_sg_mtl_cmd_encoder setRenderPipelineState:_sg_mtl_pool[pip->mtl_rps]];
        SOKOL_ASSERT(pip->mtl_dss != _SG_MTL_INVALID_POOL_INDEX);
        [_sg_mtl_cmd_encoder setDepthStencilState:_sg_mtl_pool[pip->mtl_dss]];
    }

    /* apply vertex buffers */
    int slot;
    for (slot = 0; slot < num_vbs; slot++) {
        const _sg_buffer* vb = vbs[slot];
        if ((_sg_mtl_cur_vertexbuffers[slot] != vb) ||
            (_sg_mtl_cur_vertexbuffer_offsets[slot] != vb_offsets[slot]) ||
            (_sg_mtl_cur_vertexbuffer_ids[slot].id != vb->slot.id))
        {
            _sg_mtl_cur_vertexbuffers[slot] = vb;
            _sg_mtl_cur_vertexbuffer_offsets[slot] = vb_offsets[slot];
            _sg_mtl_cur_vertexbuffer_ids[slot].id = vb->slot.id;
            const NSUInteger mtl_slot = SG_MAX_SHADERSTAGE_UBS + slot;
            SOKOL_ASSERT(vb->mtl_buf[vb->active_slot] != _SG_MTL_INVALID_POOL_INDEX);
            [_sg_mtl_cmd_encoder setVertexBuffer:_sg_mtl_pool[vb->mtl_buf[vb->active_slot]] 
                offset:vb_offsets[slot]
                atIndex:mtl_slot];
        }
    }

    /* apply vertex shader images */
    for (slot = 0; slot < num_vs_imgs; slot++) {
        const _sg_image* img = vs_imgs[slot];
        if ((_sg_mtl_cur_vs_images[slot] != img) || (_sg_mtl_cur_vs_image_ids[slot].id != img->slot.id)) {
            _sg_mtl_cur_vs_images[slot] = img;
            _sg_mtl_cur_vs_image_ids[slot].id = img->slot.id;
            SOKOL_ASSERT(img->mtl_tex[img->active_slot] != _SG_MTL_INVALID_POOL_INDEX);
            [_sg_mtl_cmd_encoder setVertexTexture:_sg_mtl_pool[img->mtl_tex[img->active_slot]] atIndex:slot];
            SOKOL_ASSERT(img->mtl_sampler_state != _SG_MTL_INVALID_POOL_INDEX);
            [_sg_mtl_cmd_encoder setVertexSamplerState:_sg_mtl_pool[img->mtl_sampler_state] atIndex:slot];
        }
    }

    /* apply fragment shader images */
    for (slot = 0; slot < num_fs_imgs; slot++) {
        const _sg_image* img = fs_imgs[slot];
        if ((_sg_mtl_cur_fs_images[slot] != img) || (_sg_mtl_cur_fs_image_ids[slot].id != img->slot.id)) {
            _sg_mtl_cur_fs_images[slot] = img;
            _sg_mtl_cur_fs_image_ids[slot].id = img->slot.id;
            SOKOL_ASSERT(img->mtl_tex[img->active_slot] != _SG_MTL_INVALID_POOL_INDEX);
            [_sg_mtl_cmd_encoder setFragmentTexture:_sg_mtl_pool[img->mtl_tex[img->active_slot]] atIndex:slot];
            SOKOL_ASSERT(img->mtl_sampler_state != _SG_MTL_INVALID_POOL_INDEX);
            [_sg_mtl_cmd_encoder setFragmentSamplerState:_sg_mtl_pool[img->mtl_sampler_state] atIndex:slot];
        }
    }
}

#define _sg_mtl_roundup(val, round_to) (((val)+((round_to)-1))&~((round_to)-1))

_SOKOL_PRIVATE void _sg_apply_uniform_block(sg_shader_stage stage_index, int ub_index, const void* data, int num_bytes) {
    SOKOL_ASSERT(_sg_mtl_in_pass);
    if (!_sg_mtl_pass_valid) {
        return;
    }
    SOKOL_ASSERT(_sg_mtl_cmd_encoder);
    SOKOL_ASSERT(data && (num_bytes > 0));
    SOKOL_ASSERT((stage_index >= 0) && ((int)stage_index < SG_NUM_SHADER_STAGES));
    SOKOL_ASSERT((ub_index >= 0) && (ub_index < SG_MAX_SHADERSTAGE_UBS));
    SOKOL_ASSERT((_sg_mtl_cur_ub_offset + num_bytes) <= _sg_mtl_ub_size);
    SOKOL_ASSERT((_sg_mtl_cur_ub_offset & (_SG_MTL_UB_ALIGN-1)) == 0);
    SOKOL_ASSERT(_sg_mtl_cur_pipeline && _sg_mtl_cur_pipeline->shader);
    SOKOL_ASSERT(_sg_mtl_cur_pipeline->slot.id == _sg_mtl_cur_pipeline_id.id);
    SOKOL_ASSERT(_sg_mtl_cur_pipeline->shader->slot.id == _sg_mtl_cur_pipeline->shader_id.id);
    SOKOL_ASSERT(ub_index < _sg_mtl_cur_pipeline->shader->stage[stage_index].num_uniform_blocks);
    SOKOL_ASSERT(num_bytes <= _sg_mtl_cur_pipeline->shader->stage[stage_index].uniform_blocks[ub_index].size);

    /* copy to global uniform buffer, record offset into cmd encoder, and advance offset */
    uint8_t* dst = &_sg_mtl_cur_ub_base_ptr[_sg_mtl_cur_ub_offset];
    memcpy(dst, data, num_bytes);
    if (stage_index == SG_SHADERSTAGE_VS) {
        [_sg_mtl_cmd_encoder setVertexBufferOffset:_sg_mtl_cur_ub_offset atIndex:ub_index];
    }
    else {
        [_sg_mtl_cmd_encoder setFragmentBufferOffset:_sg_mtl_cur_ub_offset atIndex:ub_index];
    }
    _sg_mtl_cur_ub_offset = _sg_mtl_roundup(_sg_mtl_cur_ub_offset + num_bytes, _SG_MTL_UB_ALIGN);
}

_SOKOL_PRIVATE void _sg_draw(int base_element, int num_elements, int num_instances) {
    SOKOL_ASSERT(_sg_mtl_in_pass);
    if (!_sg_mtl_pass_valid) {
        return;
    }
    SOKOL_ASSERT(_sg_mtl_cmd_encoder);
    SOKOL_ASSERT(_sg_mtl_cur_pipeline && (_sg_mtl_cur_pipeline->slot.id == _sg_mtl_cur_pipeline_id.id));
    if (SG_INDEXTYPE_NONE != _sg_mtl_cur_pipeline->index_type) {
        /* indexed rendering */
        SOKOL_ASSERT(_sg_mtl_cur_indexbuffer && (_sg_mtl_cur_indexbuffer->slot.id == _sg_mtl_cur_indexbuffer_id.id));
        const _sg_buffer* ib = _sg_mtl_cur_indexbuffer;
        SOKOL_ASSERT(ib->mtl_buf[ib->active_slot] != _SG_MTL_INVALID_POOL_INDEX);
        const NSUInteger index_buffer_offset = _sg_mtl_cur_indexbuffer_offset + 
            base_element * _sg_mtl_cur_pipeline->mtl_index_size;
        [_sg_mtl_cmd_encoder drawIndexedPrimitives:_sg_mtl_cur_pipeline->mtl_prim_type
            indexCount:num_elements
            indexType:_sg_mtl_cur_pipeline->mtl_index_type
            indexBuffer:_sg_mtl_pool[ib->mtl_buf[ib->active_slot]]
            indexBufferOffset:index_buffer_offset
            instanceCount:num_instances];
    }
    else {
        /* non-indexed rendering */
        [_sg_mtl_cmd_encoder drawPrimitives:_sg_mtl_cur_pipeline->mtl_prim_type
            vertexStart:base_element
            vertexCount:num_elements
            instanceCount:num_instances];
    }
}

_SOKOL_PRIVATE void _sg_update_buffer(_sg_buffer* buf, const void* data, int data_size) {
    SOKOL_ASSERT(buf && data && (data_size > 0));
    if (++buf->active_slot >= buf->num_slots) {
        buf->active_slot = 0;
    }
    __unsafe_unretained id<MTLBuffer> mtl_buf = _sg_mtl_pool[buf->mtl_buf[buf->active_slot]];
    void* dst_ptr = [mtl_buf contents];
    memcpy(dst_ptr, data, data_size);
    #if !TARGET_OS_IPHONE
    [mtl_buf didModifyRange:NSMakeRange(0, data_size)];
    #endif
}

_SOKOL_PRIVATE void _sg_update_image(_sg_image* img, const sg_image_content* data) {
    SOKOL_ASSERT(img && data);
    if (++img->active_slot >= img->num_slots) {
        img->active_slot = 0;
    }
    __unsafe_unretained id<MTLTexture> mtl_tex = _sg_mtl_pool[img->mtl_tex[img->active_slot]];
    _sg_mtl_copy_image_content(img, mtl_tex, data);
}

#ifdef __cplusplus
} /* extern "C" */
#endif

#else
#error "No rendering backend selected"
#endif
#ifdef __cplusplus
extern "C" {
#endif

/*== RESOURCE POOLS ==========================================================*/
typedef struct {
    int size;
    uint32_t unique_counter;
    int queue_top;
    int* free_queue;
} _sg_pool;

_SOKOL_PRIVATE void _sg_init_pool(_sg_pool* pool, int num) {
    SOKOL_ASSERT(pool && (num > 1));
    /* slot 0 is reserved for the 'invalid id', so bump the pool size by 1 */
    pool->size = num + 1;
    pool->queue_top = 0;
    pool->unique_counter = 0;
    /* it's not a bug to only reserve 'num' here */
    pool->free_queue = (int*) SOKOL_MALLOC(sizeof(int)*num);
    SOKOL_ASSERT(pool->free_queue);
    /* never allocate the zero-th pool item since the invalid id is 0 */
    for (int i = pool->size-1; i >= 1; i--) {
        pool->free_queue[pool->queue_top++] = i;
    }
}

_SOKOL_PRIVATE void _sg_discard_pool(_sg_pool* pool) {
    SOKOL_ASSERT(pool);
    SOKOL_FREE(pool->free_queue);
    pool->free_queue = 0;
    pool->size = 0;
    pool->queue_top = 0;
    pool->unique_counter = 0;
}

_SOKOL_PRIVATE uint32_t _sg_pool_alloc_id(_sg_pool* pool) {
    SOKOL_ASSERT(pool);
    SOKOL_ASSERT(pool->free_queue);
    if (pool->queue_top > 0) {
        int slot_index = pool->free_queue[--pool->queue_top];
        return ((pool->unique_counter++)<<_SG_SLOT_SHIFT)|slot_index;
    }
    else {
        /* pool exhausted */
        return SG_INVALID_ID;
    }
}

_SOKOL_PRIVATE void _sg_pool_free_id(_sg_pool* pool, uint32_t id) {
    SOKOL_ASSERT(id != SG_INVALID_ID);
    SOKOL_ASSERT(pool);
    SOKOL_ASSERT(pool->free_queue);
    SOKOL_ASSERT(pool->queue_top < pool->size);
    #ifdef SOKOL_DEBUG
    /* debug check against double-free */
    int slot_index = _sg_slot_index(id);
    for (int i = 0; i < pool->queue_top; i++) {
        SOKOL_ASSERT(pool->free_queue[i] != slot_index);
    }
    #endif
    pool->free_queue[pool->queue_top++] = id;
    SOKOL_ASSERT(pool->queue_top <= (pool->size-1));
}

typedef struct {
    _sg_pool buffer_pool;
    _sg_pool image_pool;
    _sg_pool shader_pool;
    _sg_pool pipeline_pool;
    _sg_pool pass_pool;
    _sg_pool context_pool;
    _sg_buffer* buffers;
    _sg_image* images;
    _sg_shader* shaders;
    _sg_pipeline* pipelines;
    _sg_pass* passes;
    _sg_context* contexts;
} _sg_pools;

_SOKOL_PRIVATE void _sg_setup_pools(_sg_pools* p, const sg_desc* desc) {
    SOKOL_ASSERT(p);
    SOKOL_ASSERT(desc);
    /* note: the pools here will have an additional item, since slot 0 is reserved */
    SOKOL_ASSERT((desc->buffer_pool_size >= 0) && (desc->buffer_pool_size < _SG_MAX_POOL_SIZE));
    _sg_init_pool(&p->buffer_pool, _sg_def(desc->buffer_pool_size, _SG_DEFAULT_BUFFER_POOL_SIZE));
    p->buffers = (_sg_buffer*) SOKOL_MALLOC(sizeof(_sg_buffer) * p->buffer_pool.size);
    SOKOL_ASSERT(p->buffers);
    for (int i = 0; i < p->buffer_pool.size; i++) {
        _sg_init_buffer_slot(&p->buffers[i]);
    }

    SOKOL_ASSERT((desc->image_pool_size >= 0) && (desc->image_pool_size < _SG_MAX_POOL_SIZE));
    _sg_init_pool(&p->image_pool, _sg_def(desc->image_pool_size, _SG_DEFAULT_IMAGE_POOL_SIZE));
    p->images = (_sg_image*) SOKOL_MALLOC(sizeof(_sg_image) * p->image_pool.size);
    SOKOL_ASSERT(p->images);
    for (int i = 0; i < p->image_pool.size; i++) {
        _sg_init_image_slot(&p->images[i]);
    }

    SOKOL_ASSERT((desc->shader_pool_size >= 0) && (desc->shader_pool_size < _SG_MAX_POOL_SIZE));
    _sg_init_pool(&p->shader_pool, _sg_def(desc->shader_pool_size, _SG_DEFAULT_SHADER_POOL_SIZE));
    p->shaders = (_sg_shader*) SOKOL_MALLOC(sizeof(_sg_shader) * p->shader_pool.size);
    SOKOL_ASSERT(p->shaders);
    for (int i = 0; i < p->shader_pool.size; i++) {
        _sg_init_shader_slot(&p->shaders[i]);
    }

    SOKOL_ASSERT((desc->pipeline_pool_size >= 0) && (desc->pipeline_pool_size < _SG_MAX_POOL_SIZE));
    _sg_init_pool(&p->pipeline_pool, _sg_def(desc->pipeline_pool_size, _SG_DEFAULT_PIPELINE_POOL_SIZE));
    p->pipelines = (_sg_pipeline*) SOKOL_MALLOC(sizeof(_sg_pipeline) * p->pipeline_pool.size);
    SOKOL_ASSERT(p->pipelines);
    for (int i = 0; i < p->pipeline_pool.size; i++) {
        _sg_init_pipeline_slot(&p->pipelines[i]);
    }

    SOKOL_ASSERT((desc->pass_pool_size >= 0) && (desc->pass_pool_size < _SG_MAX_POOL_SIZE));
    _sg_init_pool(&p->pass_pool, _sg_def(desc->pass_pool_size, _SG_DEFAULT_PASS_POOL_SIZE));
    p->passes = (_sg_pass*) SOKOL_MALLOC(sizeof(_sg_pass) * p->pass_pool.size);
    SOKOL_ASSERT(p->passes);
    for (int i = 0; i < p->pass_pool.size; i++) {
        _sg_init_pass_slot(&p->passes[i]);
    }

    SOKOL_ASSERT((desc->context_pool_size >= 0) && (desc->context_pool_size < _SG_MAX_POOL_SIZE));
    _sg_init_pool(&p->context_pool, _sg_def(desc->context_pool_size, _SG_DEFAULT_CONTEXT_POOL_SIZE));
    p->contexts = (_sg_context*) SOKOL_MALLOC(sizeof(_sg_context) * p->context_pool.size);
    SOKOL_ASSERT(p->contexts);
    for (int i = 0; i < p->context_pool.size; i++) {
        _sg_init_context_slot(&p->contexts[i]);
    }
}

_SOKOL_PRIVATE void _sg_discard_pools(_sg_pools* p) {
    SOKOL_ASSERT(p);
    SOKOL_FREE(p->contexts);    p->contexts = 0;
    SOKOL_FREE(p->passes);      p->passes = 0;
    SOKOL_FREE(p->pipelines);   p->pipelines = 0;
    SOKOL_FREE(p->shaders);     p->shaders = 0;
    SOKOL_FREE(p->images);      p->images = 0;
    SOKOL_FREE(p->buffers);     p->buffers = 0;
    _sg_discard_pool(&p->context_pool);
    _sg_discard_pool(&p->pass_pool);
    _sg_discard_pool(&p->pipeline_pool);
    _sg_discard_pool(&p->shader_pool);
    _sg_discard_pool(&p->image_pool);
    _sg_discard_pool(&p->buffer_pool);
}

/* returns pointer to resource by id without matching id check */
_SOKOL_PRIVATE _sg_buffer* _sg_buffer_at(const _sg_pools* p, uint32_t buf_id) {
    SOKOL_ASSERT(p && SG_INVALID_ID != buf_id);
    int slot_index = _sg_slot_index(buf_id);
    SOKOL_ASSERT((slot_index >= 0) && (slot_index < p->buffer_pool.size));
    return &p->buffers[slot_index];
}

_SOKOL_PRIVATE _sg_image* _sg_image_at(const _sg_pools* p, uint32_t img_id) {
    SOKOL_ASSERT(p && SG_INVALID_ID != img_id);
    int slot_index = _sg_slot_index(img_id);
    SOKOL_ASSERT((slot_index >= 0) && (slot_index < p->image_pool.size));
    return &p->images[slot_index];
}

_SOKOL_PRIVATE _sg_shader* _sg_shader_at(const _sg_pools* p, uint32_t shd_id) {
    SOKOL_ASSERT(p && SG_INVALID_ID != shd_id);
    int slot_index = _sg_slot_index(shd_id);
    SOKOL_ASSERT((slot_index >= 0) && (slot_index < p->shader_pool.size));
    return &p->shaders[slot_index];
}

_SOKOL_PRIVATE _sg_pipeline* _sg_pipeline_at(const _sg_pools* p, uint32_t pip_id) {
    SOKOL_ASSERT(p && SG_INVALID_ID != pip_id);
    int slot_index = _sg_slot_index(pip_id);
    SOKOL_ASSERT((slot_index >= 0) && (slot_index < p->pipeline_pool.size));
    return &p->pipelines[slot_index];
}

_SOKOL_PRIVATE _sg_pass* _sg_pass_at(const _sg_pools* p, uint32_t pass_id) {
    SOKOL_ASSERT(p && SG_INVALID_ID != pass_id);
    int slot_index = _sg_slot_index(pass_id);
    SOKOL_ASSERT((slot_index >= 0) && (slot_index < p->pass_pool.size));
    return &p->passes[slot_index];
}

_SOKOL_PRIVATE _sg_context* _sg_context_at(const _sg_pools* p, uint32_t context_id) {
    SOKOL_ASSERT(p && SG_INVALID_ID != context_id);
    int slot_index = _sg_slot_index(context_id);
    SOKOL_ASSERT((slot_index >= 0) && (slot_index < p->context_pool.size));
    return &p->contexts[slot_index];
}

/* returns pointer to resource with matching id check, may return 0 */
_SOKOL_PRIVATE _sg_buffer* _sg_lookup_buffer(const _sg_pools* p, uint32_t buf_id) {
    if (SG_INVALID_ID != buf_id) {
        _sg_buffer* buf = _sg_buffer_at(p, buf_id);
        if (buf->slot.id == buf_id) {
            return buf;
        }
    }
    return 0;
}

_SOKOL_PRIVATE _sg_image* _sg_lookup_image(const _sg_pools* p, uint32_t img_id) {
    if (SG_INVALID_ID != img_id) {
        _sg_image* img = _sg_image_at(p, img_id); 
        if (img->slot.id == img_id) {
            return img;
        }
    }
    return 0;
}

_SOKOL_PRIVATE _sg_shader* _sg_lookup_shader(const _sg_pools* p, uint32_t shd_id) {
    SOKOL_ASSERT(p);
    if (SG_INVALID_ID != shd_id) {
        _sg_shader* shd = _sg_shader_at(p, shd_id);
        if (shd->slot.id == shd_id) {
            return shd;
        }
    }
    return 0;
}

_SOKOL_PRIVATE _sg_pipeline* _sg_lookup_pipeline(const _sg_pools* p, uint32_t pip_id) {
    SOKOL_ASSERT(p);
    if (SG_INVALID_ID != pip_id) {
        _sg_pipeline* pip = _sg_pipeline_at(p, pip_id);
        if (pip->slot.id == pip_id) {
            return pip;
        }
    }
    return 0;
}

_SOKOL_PRIVATE _sg_pass* _sg_lookup_pass(const _sg_pools* p, uint32_t pass_id) {
    SOKOL_ASSERT(p);
    if (SG_INVALID_ID != pass_id) {
        _sg_pass* pass = _sg_pass_at(p, pass_id);
        if (pass->slot.id == pass_id) {
            return pass;
        }
    }
    return 0;
}

_SOKOL_PRIVATE _sg_context* _sg_lookup_context(const _sg_pools* p, uint32_t ctx_id) {
    SOKOL_ASSERT(p);
    if (SG_INVALID_ID != ctx_id) {
        _sg_context* ctx = _sg_context_at(p, ctx_id);
        if (ctx->slot.id == ctx_id) {
            return ctx;
        }
    }
    return 0;
}

_SOKOL_PRIVATE void _sg_destroy_all_resources(_sg_pools* p, uint32_t ctx_id) {
    /*  this is a bit dumb since it loops over all pool slots to
        find the occupied slots, on the other hand it is only ever
        executed at shutdown
    */
    for (int i = 0; i < p->buffer_pool.size; i++) {
        if (p->buffers[i].slot.state == SG_RESOURCESTATE_VALID) {
            if (p->buffers[i].slot.ctx_id == ctx_id) {
                _sg_destroy_buffer(&p->buffers[i]);
            }
        }
    }
    for (int i = 0; i < p->image_pool.size; i++) {
        if (p->images[i].slot.state == SG_RESOURCESTATE_VALID) {
            if (p->images[i].slot.ctx_id == ctx_id) {
                _sg_destroy_image(&p->images[i]);
            }
        }
    }
    for (int i = 0; i < p->shader_pool.size; i++) {
        if (p->shaders[i].slot.state == SG_RESOURCESTATE_VALID) {
            if (p->shaders[i].slot.ctx_id == ctx_id) {
                _sg_destroy_shader(&p->shaders[i]);
            }
        }
    }
    for (int i = 0; i < p->pipeline_pool.size; i++) {
        if (p->pipelines[i].slot.state == SG_RESOURCESTATE_VALID) {
            if (p->pipelines[i].slot.ctx_id == ctx_id) {
                _sg_destroy_pipeline(&p->pipelines[i]);
            }
        }
    }
    for (int i = 0; i < p->pass_pool.size; i++) {
        if (p->passes[i].slot.state == SG_RESOURCESTATE_VALID) {
            if (p->passes[i].slot.ctx_id == ctx_id) {
                _sg_destroy_pass(&p->passes[i]);
            }
        }
    }
}

/*== VALIDATION LAYER ========================================================*/
#if defined(SOKOL_DEBUG)
typedef enum {
    /* special case 'validation was successful' */
    _SG_VALIDATE_SUCCESS,

    /* buffer creation */
    _SG_VALIDATE_BUFFERDESC_CANARY,
    _SG_VALIDATE_BUFFERDESC_SIZE,
    _SG_VALIDATE_BUFFERDESC_CONTENT,
    _SG_VALIDATE_BUFFERDESC_NO_CONTENT,

    /* image creation */
    _SG_VALIDATE_IMAGEDESC_CANARY,
    _SG_VALIDATE_IMAGEDESC_WIDTH,
    _SG_VALIDATE_IMAGEDESC_HEIGHT,
    _SG_VALIDATE_IMAGEDESC_RT_PIXELFORMAT,
    _SG_VALIDATE_IMAGEDESC_NONRT_PIXELFORMAT,
    _SG_VALIDATE_IMAGEDESC_MSAA_BUT_NO_RT,
    _SG_VALIDATE_IMAGEDESC_NO_MSAA_RT_SUPPORT,
    _SG_VALIDATE_IMAGEDESC_RT_IMMUTABLE,
    _SG_VALIDATE_IMAGEDESC_RT_NO_CONTENT,
    _SG_VALIDATE_IMAGEDESC_CONTENT,
    _SG_VALIDATE_IMAGEDESC_NO_CONTENT,

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
    _SG_VALIDATE_SHADERDESC_IMG_NAME,

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
    _SG_VALIDATE_PASSDESC_COLOR_PIXELFORMATS,
    _SG_VALIDATE_PASSDESC_COLOR_INV_PIXELFORMAT,
    _SG_VALIDATE_PASSDESC_DEPTH_INV_PIXELFORMAT,
    _SG_VALIDATE_PASSDESC_IMAGE_SIZES,
    _SG_VALIDATE_PASSDESC_IMAGE_SAMPLE_COUNTS,

    /* sg_begin_pass validation */
    _SG_VALIDATE_BEGINPASS_PASS,
    _SG_VALIDATE_BEGINPASS_IMAGE,

    /* sg_apply_draw_state validation */
    _SG_VALIDATE_ADS_PIP,
    _SG_VALIDATE_ADS_VBS,
    _SG_VALIDATE_ADS_VB_TYPE,
    _SG_VALIDATE_ADS_NO_IB,
    _SG_VALIDATE_ADS_IB,
    _SG_VALIDATE_ADS_IB_TYPE,
    _SG_VALIDATE_ADS_VS_IMGS,
    _SG_VALIDATE_ADS_VS_IMG_TYPES,
    _SG_VALIDATE_ADS_FS_IMGS,
    _SG_VALIDATE_ADS_FS_IMG_TYPES,
    _SG_VALIDATE_ADS_ATT_COUNT,
    _SG_VALIDATE_ADS_COLOR_FORMAT,
    _SG_VALIDATE_ADS_DEPTH_FORMAT,
    _SG_VALIDATE_ADS_SAMPLE_COUNT,

    /* sg_apply_uniform_block validation */
    _SG_VALIDATE_AUB_NO_PIPELINE,
    _SG_VALIDATE_AUB_NO_UB_AT_SLOT,
    _SG_VALIDATE_AUB_SIZE,
        
    /* sg_update_buffer validation */
    _SG_VALIDATE_UPDBUF_USAGE,
    _SG_VALIDATE_UPDBUF_SIZE,
    _SG_VALIDATE_UPDBUF_ONCE,
    
    /* sg_update_image validation */
    _SG_VALIDATE_UPDIMG_USAGE,
    _SG_VALIDATE_UPDIMG_NOTENOUGHDATA,
    _SG_VALIDATE_UPDIMG_SIZE,
    _SG_VALIDATE_UPDIMG_COMPRESSED,
    _SG_VALIDATE_UPDIMG_ONCE

} _sg_validate_error;

/* return a human readable string for an _sg_validate_error */
_SOKOL_PRIVATE const char* _sg_validate_string(_sg_validate_error err) {
    switch (err) {
        /* buffer creation validation errors */
        case _SG_VALIDATE_BUFFERDESC_CANARY:        return "sg_buffer_desc not initialized";
        case _SG_VALIDATE_BUFFERDESC_SIZE:          return "sg_buffer_desc.size cannot be 0";
        case _SG_VALIDATE_BUFFERDESC_CONTENT:       return "immutable buffers must be initialized with content (sg_buffer_desc.content)";
        case _SG_VALIDATE_BUFFERDESC_NO_CONTENT:    return "dynamic/stream usage buffers cannot be initialized with content";

        /* image creation validation errros */
        case _SG_VALIDATE_IMAGEDESC_CANARY:             return "sg_image_desc not initialized";
        case _SG_VALIDATE_IMAGEDESC_WIDTH:              return "sg_image_desc.width must be > 0";
        case _SG_VALIDATE_IMAGEDESC_HEIGHT:             return "sg_image_desc.height must be > 0";
        case _SG_VALIDATE_IMAGEDESC_RT_PIXELFORMAT:     return "invalid pixel format for render-target image";
        case _SG_VALIDATE_IMAGEDESC_NONRT_PIXELFORMAT:  return "invalid pixel format for non-render-target image";
        case _SG_VALIDATE_IMAGEDESC_MSAA_BUT_NO_RT:     return "non-render-target images cannot be multisampled";
        case _SG_VALIDATE_IMAGEDESC_NO_MSAA_RT_SUPPORT: return "MSAA render targets not supported (SG_FEATURE_MSAA_RENDER_TARGETS)";
        case _SG_VALIDATE_IMAGEDESC_RT_IMMUTABLE:       return "render target images must be SG_USAGE_IMMUTABLE";
        case _SG_VALIDATE_IMAGEDESC_RT_NO_CONTENT:      return "render target images cannot be initialized with content";
        case _SG_VALIDATE_IMAGEDESC_CONTENT:            return "missing or invalid content for immutable image";
        case _SG_VALIDATE_IMAGEDESC_NO_CONTENT:         return "dynamic/stream usage images cannot be initialized with content";

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
        case _SG_VALIDATE_SHADERDESC_NO_CONT_IMGS:          return "shader images must occupy continuous slots";
        case _SG_VALIDATE_SHADERDESC_IMG_NAME:              return "GL backend requires uniform block member names";
        
        /* pipeline creation */
        case _SG_VALIDATE_PIPELINEDESC_CANARY:          return "sg_pipeline_desc not initialized";
        case _SG_VALIDATE_PIPELINEDESC_SHADER:          return "sg_pipeline_desc.shader missing or invalid";
        case _SG_VALIDATE_PIPELINEDESC_NO_ATTRS:        return "sg_pipeline_desc.layout.attrs is empty or not continuous";
        case _SG_VALIDATE_PIPELINEDESC_LAYOUT_STRIDE4:  return "sg_pipeline_desc.layout.buffers[].stride must be multiple of 4";
        case _SG_VALIDATE_PIPELINEDESC_ATTR_NAME:       return "GLES2/WebGL vertex layouts must have attribute names";
        case _SG_VALIDATE_PIPELINEDESC_ATTR_SEMANTICS:  return "D3D11 vertex layouts must have attribute semantics (sem_name and sem_index)";

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
        case _SG_VALIDATE_PASSDESC_COLOR_PIXELFORMATS:      return "all pass color attachment images must have the same pixel format";
        case _SG_VALIDATE_PASSDESC_COLOR_INV_PIXELFORMAT:   return "pass color-attachment images must have a renderable pixel format";
        case _SG_VALIDATE_PASSDESC_DEPTH_INV_PIXELFORMAT:   return "pass depth-attachment image must have depth pixel format";
        case _SG_VALIDATE_PASSDESC_IMAGE_SIZES:             return "all pass attachments must have the same size";
        case _SG_VALIDATE_PASSDESC_IMAGE_SAMPLE_COUNTS:     return "all pass attachments must have the same sample count";

        /* sg_begin_pass */
        case _SG_VALIDATE_BEGINPASS_PASS:       return "sg_begin_pass: pass must be valid";
        case _SG_VALIDATE_BEGINPASS_IMAGE:      return "sg_begin_pass: one or more attachment images are not valid";

        /* sg_apply_draw_state */
        case _SG_VALIDATE_ADS_PIP:          return "sg_apply_draw_state: pipeline object required";
        case _SG_VALIDATE_ADS_VBS:          return "sg_apply_draw_state: number of vertex buffers doesn't match number of pipeline vertex layouts";
        case _SG_VALIDATE_ADS_VB_TYPE:      return "sg_apply_draw_state: buffer in vertex buffer slot is not a SG_BUFFERTYPE_VERTEXBUFFER";
        case _SG_VALIDATE_ADS_NO_IB:        return "sg_apply_draw_state: pipeline object defines indexed rendering, but no index buffer provided";
        case _SG_VALIDATE_ADS_IB:           return "sg_apply_draw_state: pipeline object defines non-indexed rendering, but index buffer provided";
        case _SG_VALIDATE_ADS_IB_TYPE:      return "sg_apply_draw_state: buffer in index buffer slot is not a SG_BUFFERTYPE_INDEXBUFFER";
        case _SG_VALIDATE_ADS_VS_IMGS:      return "sg_apply_draw_state: vertex shader image count doesn't match sg_shader_desc";
        case _SG_VALIDATE_ADS_VS_IMG_TYPES: return "sg_apply_draw_state: one or more vertex shader image types don't match sg_shader_desc";
        case _SG_VALIDATE_ADS_FS_IMGS:      return "sg_apply_draw_state: fragment shader image count doesn't match sg_shader_desc";
        case _SG_VALIDATE_ADS_FS_IMG_TYPES: return "sg_apply_draw_state: one or more fragment shader image types don't match sg_shader_desc";
        case _SG_VALIDATE_ADS_ATT_COUNT:    return "sg_apply_draw_state: color_attachment_count in pipeline doesn't match number of pass color attachments";
        case _SG_VALIDATE_ADS_COLOR_FORMAT: return "sg_apply_draw_state: color_format in pipeline doesn't match pass color attachment pixel format";
        case _SG_VALIDATE_ADS_DEPTH_FORMAT: return "sg_apply_draw_state: depth_format in pipeline doesn't match pass depth attachment pixel format";
        case _SG_VALIDATE_ADS_SAMPLE_COUNT: return "sg_apply_draw_state: MSAA sample count in pipeline doesn't match render pass attachment sample count";

        /* sg_apply_uniform_block */
        case _SG_VALIDATE_AUB_NO_PIPELINE:      return "sg_apply_uniform_block: must be called after sg_apply_draw_state()";
        case _SG_VALIDATE_AUB_NO_UB_AT_SLOT:    return "sg_apply_uniform_block: no uniform block declaration at this shader stage UB slot";
        case _SG_VALIDATE_AUB_SIZE:             return "sg_apply_uniform_block: data size exceeds declared uniform block size";

        /* sg_update_buffer */
        case _SG_VALIDATE_UPDBUF_USAGE:     return "sg_update_buffer: cannot update immutable buffer";
        case _SG_VALIDATE_UPDBUF_SIZE:      return "sg_update_buffer: update size is bigger than buffer size";
        case _SG_VALIDATE_UPDBUF_ONCE:      return "sg_update_buffer: only one update allowed per buffer and frame";

        /* sg_update_image */
        case _SG_VALIDATE_UPDIMG_USAGE:         return "sg_update_image: cannot update immutable image";
        case _SG_VALIDATE_UPDIMG_NOTENOUGHDATA: return "sg_update_image: not enough subimage data provided";
        case _SG_VALIDATE_UPDIMG_SIZE:          return "sg_update_image: provided subimage data size too big";
        case _SG_VALIDATE_UPDIMG_COMPRESSED:    return "sg_update_image: cannot update images with compressed format";
        case _SG_VALIDATE_UPDIMG_ONCE:          return "sg_update_image: only one update allowed per image and frame";

        default: return "unknown validation error";
    }
}
#endif /* defined(SOKOL_DEBUG) */

/*-- generic backend state ---------------------------------------------------*/ 
typedef struct {
    _sg_pools pools;
    bool valid;
    uint32_t frame_index;
    sg_context active_context;
    sg_pass cur_pass;
    sg_pipeline cur_pipeline;
    bool pass_valid;
    bool next_draw_valid;
    #if defined(SOKOL_DEBUG)
    _sg_validate_error validate_error;
    #endif
} _sg_state;
static _sg_state _sg;

/*-- validation checks -------------------------------------------------------*/
#if defined(SOKOL_DEBUG)
_SOKOL_PRIVATE void _sg_validate_begin() {
    _sg.validate_error = _SG_VALIDATE_SUCCESS;
}

_SOKOL_PRIVATE void _sg_validate(bool cond, _sg_validate_error err) {
    if (!cond) {
        _sg.validate_error = err;
        SOKOL_LOG(_sg_validate_string(err));
    }
}

_SOKOL_PRIVATE bool _sg_validate_end() {
    if (_sg.validate_error != _SG_VALIDATE_SUCCESS) {
        #if !defined(SOKOL_VALIDATE_NON_FATAL)
            SOKOL_LOG("^^^^  VALIDATION FAILED, TERMINATING ^^^^");
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
        return true;
    #else
        SOKOL_ASSERT(desc);
        SOKOL_VALIDATE_BEGIN();
        SOKOL_VALIDATE(desc->_start_canary == 0, _SG_VALIDATE_BUFFERDESC_CANARY);
        SOKOL_VALIDATE(desc->_end_canary == 0, _SG_VALIDATE_BUFFERDESC_CANARY);
        SOKOL_VALIDATE(desc->size > 0, _SG_VALIDATE_BUFFERDESC_SIZE);
        bool ext = (0 != desc->gl_buffers[0]) || (0 != desc->mtl_buffers[0]) || (0 != desc->d3d11_buffer);
        if (!ext && (_sg_def(desc->usage, SG_USAGE_IMMUTABLE) == SG_USAGE_IMMUTABLE)) {
            SOKOL_VALIDATE(0 != desc->content, _SG_VALIDATE_BUFFERDESC_CONTENT);
        }
        else {
            SOKOL_VALIDATE(0 == desc->content, _SG_VALIDATE_BUFFERDESC_NO_CONTENT);
        }
        return SOKOL_VALIDATE_END();
    #endif
}

_SOKOL_PRIVATE bool _sg_validate_image_desc(const sg_image_desc* desc) {
    #if !defined(SOKOL_DEBUG)
        return true;
    #else
        SOKOL_ASSERT(desc);
        SOKOL_VALIDATE_BEGIN();
        SOKOL_VALIDATE(desc->_start_canary == 0, _SG_VALIDATE_IMAGEDESC_CANARY);
        SOKOL_VALIDATE(desc->_end_canary == 0, _SG_VALIDATE_IMAGEDESC_CANARY);
        SOKOL_VALIDATE(desc->width > 0, _SG_VALIDATE_IMAGEDESC_WIDTH);
        SOKOL_VALIDATE(desc->height > 0, _SG_VALIDATE_IMAGEDESC_HEIGHT);
        const sg_pixel_format fmt = _sg_def(desc->pixel_format, SG_PIXELFORMAT_RGBA8);
        const sg_usage usage = _sg_def(desc->usage, SG_USAGE_IMMUTABLE);
        const bool ext = (0 != desc->gl_textures[0]) || (0 != desc->mtl_textures[0]) || (0 != desc->d3d11_texture);
        if (desc->render_target) {
            if (desc->sample_count > 1) {
                SOKOL_VALIDATE(_sg_query_feature(SG_FEATURE_MSAA_RENDER_TARGETS), _SG_VALIDATE_IMAGEDESC_NO_MSAA_RT_SUPPORT);
            }
            const bool valid_color_fmt = _sg_is_valid_rendertarget_color_format(fmt);
            const bool valid_depth_fmt = _sg_is_valid_rendertarget_depth_format(fmt);
            SOKOL_VALIDATE(valid_color_fmt || valid_depth_fmt, _SG_VALIDATE_IMAGEDESC_RT_PIXELFORMAT);
            SOKOL_VALIDATE(usage == SG_USAGE_IMMUTABLE, _SG_VALIDATE_IMAGEDESC_RT_IMMUTABLE);
            SOKOL_VALIDATE(desc->content.subimage[0][0].ptr==0, _SG_VALIDATE_IMAGEDESC_RT_NO_CONTENT);
        }
        else {
            SOKOL_VALIDATE(desc->sample_count <= 1, _SG_VALIDATE_IMAGEDESC_MSAA_BUT_NO_RT);
            const bool valid_nonrt_fmt = !_sg_is_valid_rendertarget_depth_format(fmt);
            SOKOL_VALIDATE(valid_nonrt_fmt, _SG_VALIDATE_IMAGEDESC_NONRT_PIXELFORMAT);
            /* FIXME: should use the same "expected size" computation as in _sg_validate_update_image() here */
            if (!ext && (usage == SG_USAGE_IMMUTABLE)) {
                const int num_faces = _sg_def(desc->type, SG_IMAGETYPE_2D)==SG_IMAGETYPE_CUBE ? 6:1;
                const int num_mips = _sg_def(desc->num_mipmaps, 1);
                for (int face_index = 0; face_index < num_faces; face_index++) {
                    for (int mip_index = 0; mip_index < num_mips; mip_index++) {
                        const bool has_data = desc->content.subimage[face_index][mip_index].ptr != 0;
                        const bool has_size = desc->content.subimage[face_index][mip_index].size > 0;
                        SOKOL_VALIDATE(has_data && has_size, _SG_VALIDATE_IMAGEDESC_CONTENT);
                    }
                }
            }
            else {
                for (int face_index = 0; face_index < SG_CUBEFACE_NUM; face_index++) {
                    for (int mip_index = 0; mip_index < SG_MAX_MIPMAPS; mip_index++) {
                        const bool no_data = 0 == desc->content.subimage[face_index][mip_index].ptr;
                        const bool no_size = 0 == desc->content.subimage[face_index][mip_index].size;
                        SOKOL_VALIDATE(no_data && no_size, _SG_VALIDATE_IMAGEDESC_NO_CONTENT);
                    }
                }
            }
        }
        return SOKOL_VALIDATE_END();
    #endif
}

_SOKOL_PRIVATE bool _sg_validate_shader_desc(const sg_shader_desc* desc) {
    #if !defined(SOKOL_DEBUG)
        return true;
    #else
        SOKOL_ASSERT(desc);
        SOKOL_VALIDATE_BEGIN();
        SOKOL_VALIDATE(desc->_start_canary == 0, _SG_VALIDATE_SHADERDESC_CANARY);
        SOKOL_VALIDATE(desc->_end_canary == 0, _SG_VALIDATE_SHADERDESC_CANARY);
        #if defined(SOKOL_GLCORE33) || defined(SOKOL_GLES2) || defined(SOKOL_GLES3)
            /* on GL, must provide shader source code */
            SOKOL_VALIDATE(0 != desc->vs.source, _SG_VALIDATE_SHADERDESC_SOURCE);
            SOKOL_VALIDATE(0 != desc->fs.source, _SG_VALIDATE_SHADERDESC_SOURCE);
        #elif defined(SOKOL_METAL) || defined(SOKOL_D3D11_SHADER_COMPILER)
            /* on Metal or D3D with shader compiler, must provide shader source code or byte code */
            SOKOL_VALIDATE((0 != desc->vs.source)||(0 != desc->vs.byte_code), _SG_VALIDATE_SHADERDESC_SOURCE_OR_BYTECODE);
            SOKOL_VALIDATE((0 != desc->fs.source)||(0 != desc->fs.byte_code), _SG_VALIDATE_SHADERDESC_SOURCE_OR_BYTECODE);
        #else
            /* on D3D11 without shader compiler, must provide byte code */
            SOKOL_VALIDATE(0 != desc->vs.byte_code, _SG_VALIDATE_SHADERDESC_BYTECODE);
            SOKOL_VALIDATE(0 != desc->fs.byte_code, _SG_VALIDATE_SHADERDESC_BYTECODE); 
        #endif
        /* if shader byte code, the size must also be provided */
        if (0 != desc->vs.byte_code) {
            SOKOL_VALIDATE(desc->vs.byte_code_size > 0, _SG_VALIDATE_SHADERDESC_NO_BYTECODE_SIZE);
        }
        if (0 != desc->fs.byte_code) {
            SOKOL_VALIDATE(desc->fs.byte_code_size > 0, _SG_VALIDATE_SHADERDESC_NO_BYTECODE_SIZE);
        }
        for (int stage_index = 0; stage_index < SG_NUM_SHADER_STAGES; stage_index++) {
            const sg_shader_stage_desc* stage_desc = (stage_index == 0)? &desc->vs : &desc->fs;
            bool uniform_blocks_continuous = true;
            for (int ub_index = 0; ub_index < SG_MAX_SHADERSTAGE_UBS; ub_index++) {
                const sg_shader_uniform_block_desc* ub_desc = &stage_desc->uniform_blocks[ub_index];
                if (ub_desc->size > 0) {
                    SOKOL_VALIDATE(uniform_blocks_continuous, _SG_VALIDATE_SHADERDESC_NO_CONT_UBS);
                    bool uniforms_continuous = true;
                    int uniform_offset = 0;
                    int num_uniforms = 0;
                    for (int u_index = 0; u_index < SG_MAX_UB_MEMBERS; u_index++) {
                        const sg_shader_uniform_desc* u_desc = &ub_desc->uniforms[u_index];
                        if (u_desc->type != SG_UNIFORMTYPE_INVALID) {
                            SOKOL_VALIDATE(uniforms_continuous, _SG_VALIDATE_SHADERDESC_NO_CONT_UB_MEMBERS);
                            #if defined(SOKOL_GLES2) || defined(SOKOL_GLES3)
                            SOKOL_VALIDATE(u_desc->name, _SG_VALIDATE_SHADERDESC_UB_MEMBER_NAME);
                            #endif
                            const int array_count = _sg_def(u_desc->array_count, 1);
                            uniform_offset += _sg_uniform_size(u_desc->type, array_count);
                            num_uniforms++;
                        }
                        else {
                            uniforms_continuous = false;
                        }
                    }
                    #if defined(SOKOL_GLCORE33) || defined(SOKOL_GLES2) || defined(SOKOL_GLES3)
                    SOKOL_VALIDATE(uniform_offset == ub_desc->size, _SG_VALIDATE_SHADERDESC_UB_SIZE_MISMATCH);
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
                if (img_desc->type != _SG_IMAGETYPE_DEFAULT) {
                    SOKOL_VALIDATE(images_continuous, _SG_VALIDATE_SHADERDESC_NO_CONT_IMGS);
                    #if defined(SOKOL_GLES2)
                    SOKOL_VALIDATE(img_desc->name, _SG_VALIDATE_SHADERDESC_IMG_NAME);
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
        return true;
    #else
        SOKOL_ASSERT(desc);
        SOKOL_VALIDATE_BEGIN();
        SOKOL_VALIDATE(desc->_start_canary == 0, _SG_VALIDATE_PIPELINEDESC_CANARY);
        SOKOL_VALIDATE(desc->_end_canary == 0, _SG_VALIDATE_PIPELINEDESC_CANARY);
        SOKOL_VALIDATE(desc->shader.id != SG_INVALID_ID, _SG_VALIDATE_PIPELINEDESC_SHADER);
        const _sg_shader* shd = _sg_lookup_shader(&_sg.pools, desc->shader.id);
        SOKOL_VALIDATE(shd && shd->slot.state == SG_RESOURCESTATE_VALID, _SG_VALIDATE_PIPELINEDESC_SHADER);
        for (int buf_index = 0; buf_index < SG_MAX_SHADERSTAGE_BUFFERS; buf_index++) {
            const sg_buffer_layout_desc* l_desc = &desc->layout.buffers[buf_index];
            if (l_desc->stride == 0) {
                continue;
            }
            SOKOL_VALIDATE((l_desc->stride & 3) == 0, _SG_VALIDATE_PIPELINEDESC_LAYOUT_STRIDE4);
        }
        SOKOL_VALIDATE(desc->layout.attrs[0].format != SG_VERTEXFORMAT_INVALID, _SG_VALIDATE_PIPELINEDESC_NO_ATTRS);
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
            SOKOL_VALIDATE(a_desc->name, _SG_VALIDATE_PIPELINEDESC_ATTR_NAME);
            #elif defined(SOKOL_D3D11)
            /* on D3D11, semantic names (and semantic indices) must be provided */
            SOKOL_VALIDATE(a_desc->sem_name, _SG_VALIDATE_PIPELINEDESC_ATTR_SEMANTICS);
            #endif
        }
        return SOKOL_VALIDATE_END();
    #endif
}

_SOKOL_PRIVATE bool _sg_validate_pass_desc(const sg_pass_desc* desc) {
    #if !defined(SOKOL_DEBUG)
        return true;
    #else
        SOKOL_ASSERT(desc);
        SOKOL_VALIDATE_BEGIN();
        SOKOL_VALIDATE(desc->_start_canary == 0, _SG_VALIDATE_PASSDESC_CANARY);
        SOKOL_VALIDATE(desc->_end_canary == 0, _SG_VALIDATE_PASSDESC_CANARY);
        bool atts_cont = true;
        sg_pixel_format color_fmt = SG_PIXELFORMAT_NONE;
        int width = -1, height = -1, sample_count = -1;
        for (int att_index = 0; att_index < SG_MAX_COLOR_ATTACHMENTS; att_index++) {
            const sg_attachment_desc* att = &desc->color_attachments[att_index];
            if (att->image.id == SG_INVALID_ID) {
                SOKOL_VALIDATE(att_index > 0, _SG_VALIDATE_PASSDESC_NO_COLOR_ATTS);
                atts_cont = false;
                continue;
            }
            SOKOL_VALIDATE(atts_cont, _SG_VALIDATE_PASSDESC_NO_CONT_COLOR_ATTS);
            const _sg_image* img = _sg_lookup_image(&_sg.pools, att->image.id);
            SOKOL_VALIDATE(img && img->slot.state == SG_RESOURCESTATE_VALID, _SG_VALIDATE_PASSDESC_IMAGE);
            SOKOL_VALIDATE(att->mip_level < img->num_mipmaps, _SG_VALIDATE_PASSDESC_MIPLEVEL);
            if (img->type == SG_IMAGETYPE_CUBE) {
                SOKOL_VALIDATE(att->face < 6, _SG_VALIDATE_PASSDESC_FACE);
            }
            else if (img->type == SG_IMAGETYPE_ARRAY) {
                SOKOL_VALIDATE(att->layer < img->depth, _SG_VALIDATE_PASSDESC_LAYER);
            }
            else if (img->type == SG_IMAGETYPE_3D) {
                SOKOL_VALIDATE(att->slice < img->depth, _SG_VALIDATE_PASSDESC_SLICE);
            }
            SOKOL_VALIDATE(img->render_target, _SG_VALIDATE_PASSDESC_IMAGE_NO_RT);
            if (att_index == 0) {
                color_fmt = img->pixel_format;
                width = img->width >> att->mip_level;
                height = img->height >> att->mip_level;
                sample_count = img->sample_count;
            }
            else {
                SOKOL_VALIDATE(img->pixel_format == color_fmt, _SG_VALIDATE_PASSDESC_COLOR_PIXELFORMATS);
                SOKOL_VALIDATE(width == img->width >> att->mip_level, _SG_VALIDATE_PASSDESC_IMAGE_SIZES);
                SOKOL_VALIDATE(height == img->height >> att->mip_level, _SG_VALIDATE_PASSDESC_IMAGE_SIZES);
                SOKOL_VALIDATE(sample_count == img->sample_count, _SG_VALIDATE_PASSDESC_IMAGE_SAMPLE_COUNTS);
            }
            SOKOL_VALIDATE(_sg_is_valid_rendertarget_color_format(img->pixel_format), _SG_VALIDATE_PASSDESC_COLOR_INV_PIXELFORMAT);
        }
        if (desc->depth_stencil_attachment.image.id != SG_INVALID_ID) {
            const sg_attachment_desc* att = &desc->depth_stencil_attachment;
            const _sg_image* img = _sg_lookup_image(&_sg.pools, att->image.id);
            SOKOL_VALIDATE(img && img->slot.state == SG_RESOURCESTATE_VALID, _SG_VALIDATE_PASSDESC_IMAGE);
            SOKOL_VALIDATE(att->mip_level < img->num_mipmaps, _SG_VALIDATE_PASSDESC_MIPLEVEL);
            if (img->type == SG_IMAGETYPE_CUBE) {
                SOKOL_VALIDATE(att->face < 6, _SG_VALIDATE_PASSDESC_FACE);
            }
            else if (img->type == SG_IMAGETYPE_ARRAY) {
                SOKOL_VALIDATE(att->layer < img->depth, _SG_VALIDATE_PASSDESC_LAYER);
            }
            else if (img->type == SG_IMAGETYPE_3D) {
                SOKOL_VALIDATE(att->slice < img->depth, _SG_VALIDATE_PASSDESC_SLICE);
            }
            SOKOL_VALIDATE(img->render_target, _SG_VALIDATE_PASSDESC_IMAGE_NO_RT);
            SOKOL_VALIDATE(width == img->width >> att->mip_level, _SG_VALIDATE_PASSDESC_IMAGE_SIZES);
            SOKOL_VALIDATE(height == img->height >> att->mip_level, _SG_VALIDATE_PASSDESC_IMAGE_SIZES);
            SOKOL_VALIDATE(sample_count == img->sample_count, _SG_VALIDATE_PASSDESC_IMAGE_SAMPLE_COUNTS);
            SOKOL_VALIDATE(_sg_is_valid_rendertarget_depth_format(img->pixel_format), _SG_VALIDATE_PASSDESC_DEPTH_INV_PIXELFORMAT);
        }
        return SOKOL_VALIDATE_END();
    #endif
}

_SOKOL_PRIVATE bool _sg_validate_begin_pass(_sg_pass* pass) {
    #if !defined(SOKOL_DEBUG)
        return true;
    #else
        SOKOL_VALIDATE_BEGIN();
        SOKOL_VALIDATE(pass->slot.state == SG_RESOURCESTATE_VALID, _SG_VALIDATE_BEGINPASS_PASS);
        for (int i = 0; i < SG_MAX_COLOR_ATTACHMENTS; i++) {
            const _sg_attachment* att = &pass->color_atts[i];
            if (att->image) {
                SOKOL_VALIDATE(att->image->slot.state == SG_RESOURCESTATE_VALID, _SG_VALIDATE_BEGINPASS_IMAGE);
                SOKOL_VALIDATE(att->image->slot.id == att->image_id.id, _SG_VALIDATE_BEGINPASS_IMAGE);
            }
        }
        if (pass->ds_att.image) {
            const _sg_attachment* att = &pass->ds_att;
            SOKOL_VALIDATE(att->image->slot.state == SG_RESOURCESTATE_VALID, _SG_VALIDATE_BEGINPASS_IMAGE);
            SOKOL_VALIDATE(att->image->slot.id == att->image_id.id, _SG_VALIDATE_BEGINPASS_IMAGE);
        }
        return SOKOL_VALIDATE_END();
    #endif
}

_SOKOL_PRIVATE bool _sg_validate_draw_state(const sg_draw_state* ds) {
    #if !defined(SOKOL_DEBUG)
        return true;
    #else
        SOKOL_VALIDATE_BEGIN();
        /* has pipeline and pipeline still exists */
        SOKOL_VALIDATE(ds->pipeline.id != SG_INVALID_ID, _SG_VALIDATE_ADS_PIP);
        const _sg_pipeline* pip = _sg_lookup_pipeline(&_sg.pools, ds->pipeline.id);
        if (!pip) {
            /* cannot continue with validation without pipeline object */
            return SOKOL_VALIDATE_END();
        }
        SOKOL_ASSERT(pip->shader);

        /* has expected vertex buffers, and vertex buffers still exist */
        for (int i = 0; i < SG_MAX_SHADERSTAGE_BUFFERS; i++) {
            if (ds->vertex_buffers[i].id != SG_INVALID_ID) {
                SOKOL_VALIDATE(pip->vertex_layout_valid[i], _SG_VALIDATE_ADS_VBS);
                /* buffers in vertex-buffer-slots must be of type SG_BUFFERTYPE_VERTEXBUFFER */
                const _sg_buffer* buf = _sg_lookup_buffer(&_sg.pools, ds->vertex_buffers[i].id);
                SOKOL_ASSERT(buf);
                if (buf->slot.state == SG_RESOURCESTATE_VALID) {
                    SOKOL_VALIDATE(SG_BUFFERTYPE_VERTEXBUFFER == buf->type, _SG_VALIDATE_ADS_VB_TYPE);
                }
            }
            else {
                /* vertex buffer provided in a slot which has no vertex layout in pipeline */
                SOKOL_VALIDATE(!pip->vertex_layout_valid[i], _SG_VALIDATE_ADS_VBS);
            }
        }

        /* index buffer expected or not, and index buffer still exists */
        if (pip->index_type == SG_INDEXTYPE_NONE) {
            /* pipeline defines non-indexed rendering, but index buffer provided */
            SOKOL_VALIDATE(ds->index_buffer.id == SG_INVALID_ID, _SG_VALIDATE_ADS_IB);
        }
        else {
            /* pipeline defines indexed rendering, but no index buffer provided */
            SOKOL_VALIDATE(ds->index_buffer.id != SG_INVALID_ID, _SG_VALIDATE_ADS_NO_IB);
        }
        if (ds->index_buffer.id != SG_INVALID_ID) {
            /* buffer in index-buffer-slot must be of type SG_BUFFERTYPE_INDEXBUFFER */
            const _sg_buffer* buf = _sg_lookup_buffer(&_sg.pools, ds->index_buffer.id);
            SOKOL_ASSERT(buf);
            if (buf->slot.state == SG_RESOURCESTATE_VALID) {
                SOKOL_VALIDATE(SG_BUFFERTYPE_INDEXBUFFER == buf->type, _SG_VALIDATE_ADS_IB_TYPE);
            }
        }

        /* has expected vertex shader images */
        for (int i = 0; i < SG_MAX_SHADERSTAGE_IMAGES; i++) {
            _sg_shader_stage* stage = &pip->shader->stage[SG_SHADERSTAGE_VS];
            if (ds->vs_images[i].id != SG_INVALID_ID) {
                SOKOL_VALIDATE(i < stage->num_images, _SG_VALIDATE_ADS_VS_IMGS);
                const _sg_image* img = _sg_lookup_image(&_sg.pools, ds->vs_images[i].id);
                SOKOL_ASSERT(img);
                if (img->slot.state == SG_RESOURCESTATE_VALID) {
                    SOKOL_VALIDATE(img->type == stage->images[i].type, _SG_VALIDATE_ADS_VS_IMG_TYPES);
                }
            }
            else {
                SOKOL_VALIDATE(i >= stage->num_images, _SG_VALIDATE_ADS_VS_IMGS);
            }
        }

        /* has expected fragment shader images */
        for (int i = 0; i < SG_MAX_SHADERSTAGE_IMAGES; i++) {
            _sg_shader_stage* stage = &pip->shader->stage[SG_SHADERSTAGE_FS];
            if (ds->fs_images[i].id != SG_INVALID_ID) {
                SOKOL_VALIDATE(i < stage->num_images, _SG_VALIDATE_ADS_FS_IMGS);
                const _sg_image* img = _sg_lookup_image(&_sg.pools, ds->fs_images[i].id);
                SOKOL_ASSERT(img);
                if (img->slot.state == SG_RESOURCESTATE_VALID) {
                    SOKOL_VALIDATE(img->type == stage->images[i].type, _SG_VALIDATE_ADS_FS_IMG_TYPES);
                }
            }
            else {
                SOKOL_VALIDATE(i >= stage->num_images, _SG_VALIDATE_ADS_FS_IMGS);
            }
        }

        /* check that pipeline attributes match current pass attributes */
        const _sg_pass* pass = _sg_lookup_pass(&_sg.pools, _sg.cur_pass.id);
        if (pass) {
            /* an offscreen pass */
            SOKOL_VALIDATE(pip->color_attachment_count == pass->num_color_atts, _SG_VALIDATE_ADS_ATT_COUNT);
            SOKOL_VALIDATE(pip->color_format == pass->color_atts[0].image->pixel_format, _SG_VALIDATE_ADS_COLOR_FORMAT);
            SOKOL_VALIDATE(pip->sample_count == pass->color_atts[0].image->sample_count, _SG_VALIDATE_ADS_SAMPLE_COUNT);
            if (pass->ds_att.image) {
                SOKOL_VALIDATE(pip->depth_format == pass->ds_att.image->pixel_format, _SG_VALIDATE_ADS_DEPTH_FORMAT);
            }
            else {
                SOKOL_VALIDATE(pip->depth_format == SG_PIXELFORMAT_NONE, _SG_VALIDATE_ADS_DEPTH_FORMAT);
            }
        }
        else {
            /* default pass */
            SOKOL_VALIDATE(pip->color_attachment_count == 1, _SG_VALIDATE_ADS_ATT_COUNT);
            SOKOL_VALIDATE(pip->color_format == SG_PIXELFORMAT_RGBA8, _SG_VALIDATE_ADS_COLOR_FORMAT);
            SOKOL_VALIDATE(pip->depth_format == SG_PIXELFORMAT_DEPTHSTENCIL, _SG_VALIDATE_ADS_DEPTH_FORMAT);
            /* FIXME: hmm, we don't know if the default framebuffer is multisampled here */
        }
        return SOKOL_VALIDATE_END();
    #endif
}

_SOKOL_PRIVATE bool _sg_validate_apply_uniform_block(sg_shader_stage stage_index, int ub_index, const void* data, int num_bytes) {
    #if !defined(SOKOL_DEBUG)
        return true;
    #else
        SOKOL_ASSERT((stage_index == SG_SHADERSTAGE_VS) || (stage_index == SG_SHADERSTAGE_FS));
        SOKOL_ASSERT((ub_index >= 0) && (ub_index < SG_MAX_SHADERSTAGE_UBS));
        SOKOL_VALIDATE_BEGIN();
        SOKOL_VALIDATE(_sg.cur_pipeline.id != SG_INVALID_ID, _SG_VALIDATE_AUB_NO_PIPELINE);
        const _sg_pipeline* pip = _sg_lookup_pipeline(&_sg.pools, _sg.cur_pipeline.id);
        SOKOL_ASSERT(pip && (pip->slot.id == _sg.cur_pipeline.id));
        SOKOL_ASSERT(pip->shader && (pip->shader->slot.id == pip->shader_id.id));

        /* check that there is a uniform block at 'stage' and 'ub_index' */
        const _sg_shader_stage* stage = &pip->shader->stage[stage_index];
        SOKOL_VALIDATE(ub_index < stage->num_uniform_blocks, _SG_VALIDATE_AUB_NO_UB_AT_SLOT);

        /* check that the provided data size doesn't exceed the uniform block size */
        SOKOL_VALIDATE(num_bytes <= stage->uniform_blocks[ub_index].size, _SG_VALIDATE_AUB_SIZE);

        return SOKOL_VALIDATE_END();
    #endif
}

_SOKOL_PRIVATE bool _sg_validate_update_buffer(const _sg_buffer* buf, const void* data, int size) {
    #if !defined(SOKOL_DEBUG)
        return true;
    #else
        SOKOL_ASSERT(buf && data);
        SOKOL_VALIDATE_BEGIN();
        SOKOL_VALIDATE(buf->usage != SG_USAGE_IMMUTABLE, _SG_VALIDATE_UPDBUF_USAGE);
        SOKOL_VALIDATE(buf->size >= size, _SG_VALIDATE_UPDBUF_SIZE);
        SOKOL_VALIDATE(buf->upd_frame_index != _sg.frame_index, _SG_VALIDATE_UPDBUF_ONCE);
        return SOKOL_VALIDATE_END();
    #endif
}

_SOKOL_PRIVATE bool _sg_validate_update_image(const _sg_image* img, const sg_image_content* data) {
    #if !defined(SOKOL_DEBUG)
        return true;
    #else
        SOKOL_ASSERT(img && data);
        SOKOL_VALIDATE_BEGIN();
        SOKOL_VALIDATE(img->usage != SG_USAGE_IMMUTABLE, _SG_VALIDATE_UPDIMG_USAGE);
        SOKOL_VALIDATE(img->upd_frame_index != _sg.frame_index, _SG_VALIDATE_UPDIMG_ONCE);
        SOKOL_VALIDATE(!_sg_is_compressed_pixel_format(img->pixel_format), _SG_VALIDATE_UPDIMG_COMPRESSED);
        const int num_faces = (img->type == SG_IMAGETYPE_CUBE) ? 6 : 1;
        const int num_mips = img->num_mipmaps;
        for (int face_index = 0; face_index < num_faces; face_index++) {
            for (int mip_index = 0; mip_index < num_mips; mip_index++) {
                SOKOL_VALIDATE(0 != data->subimage[face_index][mip_index].ptr, _SG_VALIDATE_UPDIMG_NOTENOUGHDATA);
                const int mip_width = _sg_max(img->width >> mip_index, 1);
                const int mip_height = _sg_max(img->height >> mip_index, 1);
                const int bytes_per_slice = _sg_surface_pitch(img->pixel_format, mip_width, mip_height);
                const int expected_size = bytes_per_slice * img->depth;
                SOKOL_VALIDATE(data->subimage[face_index][mip_index].size <= expected_size, _SG_VALIDATE_UPDIMG_SIZE);
            }
        }
        return SOKOL_VALIDATE_END();
    #endif
}

/*== PUBLIC API FUNCTIONS ====================================================*/
void sg_setup(const sg_desc* desc) {
    SOKOL_ASSERT(desc);
    SOKOL_ASSERT((desc->_start_canary == 0) && (desc->_end_canary == 0));
    memset(&_sg, 0, sizeof(_sg));
    _sg_setup_pools(&_sg.pools, desc);
    _sg.frame_index = 1;
    _sg.next_draw_valid = false;
    _sg_setup_backend(desc);
    sg_setup_context();
    _sg.valid = true;
}

void sg_shutdown(void) {
    /* can only delete resources for the currently set context here, if multiple
    contexts are used, the app code must take care of properly releasing them
    (since only the app code can switch between 3D-API contexts)
    */
    if (_sg.active_context.id != SG_INVALID_ID) {
        _sg_context* ctx = _sg_lookup_context(&_sg.pools, _sg.active_context.id);
        if (ctx) {
            _sg_destroy_all_resources(&_sg.pools, _sg.active_context.id);
            _sg_destroy_context(ctx);
        }
    }
    _sg_discard_backend();
    _sg_discard_pools(&_sg.pools);
    _sg.valid = false;
}

bool sg_isvalid(void) {
    return _sg.valid;
}

bool sg_query_feature(sg_feature f) {
    return _sg_query_feature(f);
}

sg_context sg_setup_context(void) {
    sg_context res;
    res.id = _sg_pool_alloc_id(&_sg.pools.context_pool);
    if (res.id != SG_INVALID_ID) {
        _sg_context* ctx = _sg_context_at(&_sg.pools, res.id);
        SOKOL_ASSERT(ctx);
        ctx->slot.id = res.id;
        ctx->slot.state = SG_RESOURCESTATE_ALLOC;
        _sg_create_context(ctx);
        SOKOL_ASSERT(ctx->slot.state == SG_RESOURCESTATE_VALID);
        _sg_activate_context(ctx);
    }
    _sg.active_context = res;
    return res;
}

void sg_discard_context(sg_context ctx_id) {
    _sg_destroy_all_resources(&_sg.pools, ctx_id.id);
    _sg_context* ctx = _sg_lookup_context(&_sg.pools, ctx_id.id);
    if (ctx) {
        _sg_destroy_context(ctx);
        _sg_pool_free_id(&_sg.pools.context_pool, ctx_id.id);
    }
    _sg.active_context.id = SG_INVALID_ID;
    _sg_activate_context(0);
}

void sg_activate_context(sg_context ctx_id) {
    _sg.active_context = ctx_id;
    _sg_context* ctx = _sg_lookup_context(&_sg.pools, ctx_id.id);
    /* NOTE: ctx can be 0 here if the context is no longer valid */
    _sg_activate_context(ctx);
}

/*-- allocate resource id ----------------------------------------------------*/
sg_buffer sg_alloc_buffer(void) {
    sg_buffer res;
    res.id = _sg_pool_alloc_id(&_sg.pools.buffer_pool);
    if (res.id != SG_INVALID_ID) {
        _sg_buffer* buf = _sg_buffer_at(&_sg.pools, res.id);
        SOKOL_ASSERT(buf && (buf->slot.state == SG_RESOURCESTATE_INITIAL) && (buf->slot.id == SG_INVALID_ID));
        buf->slot.id = res.id;
        buf->slot.state = SG_RESOURCESTATE_ALLOC;
    }
    return res;
}

sg_image sg_alloc_image(void) {
    sg_image res;
    res.id = _sg_pool_alloc_id(&_sg.pools.image_pool);
    if (res.id != SG_INVALID_ID) {
        _sg_image* img = _sg_image_at(&_sg.pools, res.id);
        SOKOL_ASSERT(img && (img->slot.state == SG_RESOURCESTATE_INITIAL) && (img->slot.id == SG_INVALID_ID));
        img->slot.id = res.id;
        img->slot.state = SG_RESOURCESTATE_ALLOC;
    }
    return res;
}

sg_shader sg_alloc_shader(void) {
    sg_shader res;
    res.id = _sg_pool_alloc_id(&_sg.pools.shader_pool);
    if (res.id != SG_INVALID_ID) {
        _sg_shader* shd = _sg_shader_at(&_sg.pools, res.id);
        SOKOL_ASSERT(shd && (shd->slot.state == SG_RESOURCESTATE_INITIAL) && (shd->slot.id == SG_INVALID_ID));
        shd->slot.id = res.id;
        shd->slot.state = SG_RESOURCESTATE_ALLOC;
    }
    return res;
}

sg_pipeline sg_alloc_pipeline(void) {
    sg_pipeline res;
    res.id = _sg_pool_alloc_id(&_sg.pools.pipeline_pool);
    if (res.id != SG_INVALID_ID) {
        _sg_pipeline* pip = _sg_pipeline_at(&_sg.pools, res.id);
        SOKOL_ASSERT(pip && (pip->slot.state == SG_RESOURCESTATE_INITIAL) && (pip->slot.id == SG_INVALID_ID));
        pip->slot.id = res.id;
        pip->slot.state = SG_RESOURCESTATE_ALLOC;
    }
    return res;
}

sg_pass sg_alloc_pass(void) {
    sg_pass res;
    res.id = _sg_pool_alloc_id(&_sg.pools.pass_pool);
    if (res.id != SG_INVALID_ID) {
        _sg_pass* pass = _sg_pass_at(&_sg.pools, res.id);
        SOKOL_ASSERT(pass && (pass->slot.state == SG_RESOURCESTATE_INITIAL) && (pass->slot.id == SG_INVALID_ID));
        pass->slot.id = res.id;
        pass->slot.state = SG_RESOURCESTATE_ALLOC;
    }
    return res;
}

/*-- initialize an allocated resource ----------------------------------------*/
void sg_init_buffer(sg_buffer buf_id, const sg_buffer_desc* desc) {
    SOKOL_ASSERT(buf_id.id != SG_INVALID_ID && desc);
    _sg_buffer* buf = _sg_lookup_buffer(&_sg.pools, buf_id.id);
    SOKOL_ASSERT(buf && buf->slot.state == SG_RESOURCESTATE_ALLOC);
    if (_sg_validate_buffer_desc(desc)) {
        _sg_create_buffer(buf, desc);
        buf->slot.ctx_id = _sg.active_context.id;
    }
    else {
        buf->slot.state = SG_RESOURCESTATE_FAILED;
    }
    SOKOL_ASSERT((buf->slot.state == SG_RESOURCESTATE_VALID)||(buf->slot.state == SG_RESOURCESTATE_FAILED));
}

void sg_init_image(sg_image img_id, const sg_image_desc* desc) {
    SOKOL_ASSERT(img_id.id != SG_INVALID_ID && desc);
    _sg_image* img = _sg_lookup_image(&_sg.pools, img_id.id);
    SOKOL_ASSERT(img && img->slot.state == SG_RESOURCESTATE_ALLOC);
    if (_sg_validate_image_desc(desc)) {
        _sg_create_image(img, desc);
        img->slot.ctx_id = _sg.active_context.id;
    }
    else {
        img->slot.state = SG_RESOURCESTATE_FAILED;
    }
    SOKOL_ASSERT((img->slot.state == SG_RESOURCESTATE_VALID)||(img->slot.state == SG_RESOURCESTATE_FAILED));
}

void sg_init_shader(sg_shader shd_id, const sg_shader_desc* desc) {
    SOKOL_ASSERT(shd_id.id != SG_INVALID_ID && desc);
    _sg_shader* shd = _sg_lookup_shader(&_sg.pools, shd_id.id);
    SOKOL_ASSERT(shd && shd->slot.state == SG_RESOURCESTATE_ALLOC);
    if (_sg_validate_shader_desc(desc)) {
        _sg_create_shader(shd, desc);
        shd->slot.ctx_id = _sg.active_context.id;
    }
    else {
        shd->slot.state = SG_RESOURCESTATE_FAILED;
    }
    SOKOL_ASSERT((shd->slot.state == SG_RESOURCESTATE_VALID)||(shd->slot.state == SG_RESOURCESTATE_FAILED));
}

void sg_init_pipeline(sg_pipeline pip_id, const sg_pipeline_desc* desc) {
    SOKOL_ASSERT(pip_id.id != SG_INVALID_ID && desc);
    _sg_pipeline* pip = _sg_lookup_pipeline(&_sg.pools, pip_id.id);
    SOKOL_ASSERT(pip && pip->slot.state == SG_RESOURCESTATE_ALLOC);
    if (_sg_validate_pipeline_desc(desc)) {
        _sg_shader* shd = _sg_lookup_shader(&_sg.pools, desc->shader.id);
        SOKOL_ASSERT(shd && shd->slot.state == SG_RESOURCESTATE_VALID);
        _sg_create_pipeline(pip, shd, desc);
        pip->slot.ctx_id = _sg.active_context.id;
    }
    else {
        pip->slot.state = SG_RESOURCESTATE_FAILED;
    }
    SOKOL_ASSERT((pip->slot.state == SG_RESOURCESTATE_VALID)||(pip->slot.state == SG_RESOURCESTATE_FAILED)); 
}

void sg_init_pass(sg_pass pass_id, const sg_pass_desc* desc) {
    SOKOL_ASSERT(pass_id.id != SG_INVALID_ID && desc);
    _sg_pass* pass = _sg_lookup_pass(&_sg.pools, pass_id.id);
    SOKOL_ASSERT(pass && pass->slot.state == SG_RESOURCESTATE_ALLOC);
    if (_sg_validate_pass_desc(desc)) {
        /* lookup pass attachment image pointers */
        _sg_image* att_imgs[SG_MAX_COLOR_ATTACHMENTS + 1];
        for (int i = 0; i < SG_MAX_COLOR_ATTACHMENTS; i++) {
            if (desc->color_attachments[i].image.id) {
                att_imgs[i] = _sg_lookup_image(&_sg.pools, desc->color_attachments[i].image.id);
                SOKOL_ASSERT(att_imgs[i] && att_imgs[i]->slot.state == SG_RESOURCESTATE_VALID);
            }
            else {
                att_imgs[i] = 0;
            }
        }
        const int ds_att_index = SG_MAX_COLOR_ATTACHMENTS;
        if (desc->depth_stencil_attachment.image.id) {
            att_imgs[ds_att_index] = _sg_lookup_image(&_sg.pools, desc->depth_stencil_attachment.image.id);
            SOKOL_ASSERT(att_imgs[ds_att_index] && att_imgs[ds_att_index]->slot.state == SG_RESOURCESTATE_VALID);
        }
        else {
            att_imgs[ds_att_index] = 0;
        }
        _sg_create_pass(pass, att_imgs, desc);
        pass->slot.ctx_id = _sg.active_context.id;
    }
    else {
        pass->slot.state = SG_RESOURCESTATE_FAILED;
    }
    SOKOL_ASSERT((pass->slot.state == SG_RESOURCESTATE_VALID)||(pass->slot.state == SG_RESOURCESTATE_FAILED)); 
}

/*-- set allocated resource to failed state ----------------------------------*/
void sg_fail_buffer(sg_buffer buf_id) {
    SOKOL_ASSERT(buf_id.id != SG_INVALID_ID);
    _sg_buffer* buf = _sg_lookup_buffer(&_sg.pools, buf_id.id);
    SOKOL_ASSERT(buf && buf->slot.state == SG_RESOURCESTATE_ALLOC);
    buf->slot.state = SG_RESOURCESTATE_FAILED;
}

void sg_fail_image(sg_image img_id) {
    SOKOL_ASSERT(img_id.id != SG_INVALID_ID);
    _sg_image* img = _sg_lookup_image(&_sg.pools, img_id.id);
    SOKOL_ASSERT(img && img->slot.state == SG_RESOURCESTATE_ALLOC);
    img->slot.state = SG_RESOURCESTATE_FAILED;
}

void sg_fail_shader(sg_shader shd_id) {
    SOKOL_ASSERT(shd_id.id != SG_INVALID_ID);
    _sg_shader* shd = _sg_lookup_shader(&_sg.pools, shd_id.id);
    SOKOL_ASSERT(shd && shd->slot.state == SG_RESOURCESTATE_ALLOC);
    shd->slot.state = SG_RESOURCESTATE_FAILED;
}

void sg_fail_pipeline(sg_pipeline pip_id) {
    SOKOL_ASSERT(pip_id.id != SG_INVALID_ID);
    _sg_pipeline* pip = _sg_lookup_pipeline(&_sg.pools, pip_id.id);
    SOKOL_ASSERT(pip && pip->slot.state == SG_RESOURCESTATE_ALLOC);
    pip->slot.state = SG_RESOURCESTATE_FAILED;
}

void sg_fail_pass(sg_pass pass_id) {
    SOKOL_ASSERT(pass_id.id != SG_INVALID_ID);
    _sg_pass* pass = _sg_lookup_pass(&_sg.pools, pass_id.id);
    SOKOL_ASSERT(pass && pass->slot.state == SG_RESOURCESTATE_ALLOC);
    pass->slot.state = SG_RESOURCESTATE_FAILED;
}

/*-- get resource state */
sg_resource_state sg_query_buffer_state(sg_buffer buf_id) {
    if (buf_id.id != SG_INVALID_ID) {
        _sg_buffer* buf = _sg_lookup_buffer(&_sg.pools, buf_id.id);
        if (buf) {
            return buf->slot.state;
        }
    }
    return SG_RESOURCESTATE_INVALID;
}

sg_resource_state sg_query_image_state(sg_image img_id) {
    if (img_id.id != SG_INVALID_ID) {
        _sg_image* img = _sg_lookup_image(&_sg.pools, img_id.id);
        if (img) {
            return img->slot.state;
        }
    }
    return SG_RESOURCESTATE_INVALID;
}

sg_resource_state sg_query_shader_state(sg_shader shd_id) {
    if (shd_id.id != SG_INVALID_ID) {
        _sg_shader* shd = _sg_lookup_shader(&_sg.pools, shd_id.id);
        if (shd) {
            return shd->slot.state;
        }
    }
    return SG_RESOURCESTATE_INVALID;
}

sg_resource_state sg_query_pipeline_state(sg_pipeline pip_id) {
    if (pip_id.id != SG_INVALID_ID) {
        _sg_pipeline* pip = _sg_lookup_pipeline(&_sg.pools, pip_id.id);
        if (pip) {
            return pip->slot.state;
        }
    }
    return SG_RESOURCESTATE_INVALID;
}

sg_resource_state sg_query_pass_state(sg_pass pass_id) {
    if (pass_id.id != SG_INVALID_ID) {
        _sg_pass* pass = _sg_lookup_pass(&_sg.pools, pass_id.id);
        if (pass) {
            return pass->slot.state;
        }
    }
    return SG_RESOURCESTATE_INVALID;
}

/*-- allocate and initialize resource ----------------------------------------*/
sg_buffer sg_make_buffer(const sg_buffer_desc* desc) {
    SOKOL_ASSERT(desc);
    sg_buffer buf_id = sg_alloc_buffer();
    if (buf_id.id != SG_INVALID_ID) {
        sg_init_buffer(buf_id, desc);
    }
    else {
        SOKOL_LOG("buffer pool exhausted!");
    }
    return buf_id;
}

sg_image sg_make_image(const sg_image_desc* desc) {
    SOKOL_ASSERT(desc);
    sg_image img_id = sg_alloc_image();
    if (img_id.id != SG_INVALID_ID) {
        sg_init_image(img_id, desc);
    }
    else {
        SOKOL_LOG("image pool exhausted!");
    }
    return img_id;
}

sg_shader sg_make_shader(const sg_shader_desc* desc) {
    SOKOL_ASSERT(desc);
    sg_shader shd_id = sg_alloc_shader();
    if (shd_id.id != SG_INVALID_ID) {
        sg_init_shader(shd_id, desc);
    }
    else {
        SOKOL_LOG("shader pool exhausted!");
    }
    return shd_id;
}

sg_pipeline sg_make_pipeline(const sg_pipeline_desc* desc) {
    SOKOL_ASSERT(desc);
    sg_pipeline pip_id = sg_alloc_pipeline();
    if (pip_id.id != SG_INVALID_ID) {
        sg_init_pipeline(pip_id, desc);
    }
    else {
        SOKOL_LOG("pipeline pool exhausted!");
    }
    return pip_id;
}

sg_pass sg_make_pass(const sg_pass_desc* desc) {
    SOKOL_ASSERT(desc);
    sg_pass pass_id = sg_alloc_pass();
    if (pass_id.id != SG_INVALID_ID) {
        sg_init_pass(pass_id, desc);
    }
    else {
        SOKOL_LOG("pass pool exhausted!");
    }
    return pass_id;
}

/*-- destroy resource --------------------------------------------------------*/
void sg_destroy_buffer(sg_buffer buf_id) {
    _sg_buffer* buf = _sg_lookup_buffer(&_sg.pools, buf_id.id);
    if (buf) {
        if (buf->slot.ctx_id == _sg.active_context.id) {
            _sg_destroy_buffer(buf);
            _sg_pool_free_id(&_sg.pools.buffer_pool, buf_id.id);
        }
        else {
            SOKOL_LOG("sg_destroy_buffer: active context mismatch (must be same as for creation)");
        }
    }
}

void sg_destroy_image(sg_image img_id) {
    _sg_image* img = _sg_lookup_image(&_sg.pools, img_id.id);
    if (img) {
        if (img->slot.ctx_id == _sg.active_context.id) {
            _sg_destroy_image(img);
            _sg_pool_free_id(&_sg.pools.image_pool, img_id.id);
        }
        else {
            SOKOL_LOG("sg_destroy_image: active context mismatch (must be same as for creation)");
        }
    }
}

void sg_destroy_shader(sg_shader shd_id) {
    _sg_shader* shd = _sg_lookup_shader(&_sg.pools, shd_id.id);
    if (shd) {
        if (shd->slot.ctx_id == _sg.active_context.id) {
            _sg_destroy_shader(shd);
            _sg_pool_free_id(&_sg.pools.shader_pool, shd_id.id);
        }
        else {
            SOKOL_LOG("sg_destroy_shader: active context mismatch (must be same as for creation)");
        }
    }
}

void sg_destroy_pipeline(sg_pipeline pip_id) {
    _sg_pipeline* pip = _sg_lookup_pipeline(&_sg.pools, pip_id.id);
    if (pip) {
        if (pip->slot.ctx_id == _sg.active_context.id) {
            _sg_destroy_pipeline(pip);
            _sg_pool_free_id(&_sg.pools.pipeline_pool, pip_id.id);
        }
        else {
            SOKOL_LOG("sg_destroy_pipeline: active context mismatch (must be same as for creation)");
        }
    }
}

void sg_destroy_pass(sg_pass pass_id) {
    _sg_pass* pass = _sg_lookup_pass(&_sg.pools, pass_id.id);
    if (pass) {
        if (pass->slot.ctx_id == _sg.active_context.id) {
            _sg_destroy_pass(pass);
            _sg_pool_free_id(&_sg.pools.pass_pool, pass_id.id);
        }
        else {
            SOKOL_LOG("sg_destroy_pass: active context mismatch (must be same as for creation)");
        }
    }
}

void sg_begin_default_pass(const sg_pass_action* pass_action, int width, int height) {
    SOKOL_ASSERT(pass_action);
    SOKOL_ASSERT((pass_action->_start_canary == 0) && (pass_action->_end_canary == 0));
    sg_pass_action pa;
    _sg_resolve_default_pass_action(pass_action, &pa);
    _sg.cur_pass.id = SG_INVALID_ID;
    _sg.pass_valid = true;
    _sg_begin_pass(0, &pa, width, height);
}

void sg_begin_pass(sg_pass pass_id, const sg_pass_action* pass_action) {
    SOKOL_ASSERT(pass_action);
    SOKOL_ASSERT((pass_action->_start_canary == 0) && (pass_action->_end_canary == 0));
    _sg.cur_pass = pass_id;
    _sg_pass* pass = _sg_lookup_pass(&_sg.pools, pass_id.id);
    if (pass && _sg_validate_begin_pass(pass)) {
        _sg.pass_valid = true;
        sg_pass_action pa;
        _sg_resolve_default_pass_action(pass_action, &pa);
        const int w = pass->color_atts[0].image->width;
        const int h = pass->color_atts[0].image->height;
        _sg_begin_pass(pass, &pa, w, h);
    }
    else {
        _sg.pass_valid = false;
    }
}

void sg_apply_viewport(int x, int y, int width, int height, bool origin_top_left) {
    if (!_sg.pass_valid) {
        return;
    }
    _sg_apply_viewport(x, y, width, height, origin_top_left);
}

void sg_apply_scissor_rect(int x, int y, int width, int height, bool origin_top_left) {
    if (!_sg.pass_valid) {
        return;
    }
    _sg_apply_scissor_rect(x, y, width, height, origin_top_left);
}

void sg_apply_draw_state(const sg_draw_state* ds) {
    SOKOL_ASSERT(ds);
    SOKOL_ASSERT((ds->_start_canary==0) && (ds->_end_canary==0));
    if (!_sg_validate_draw_state(ds)) {
        _sg.next_draw_valid = false;
        return;
    }
    if (!_sg.pass_valid) {
        return;
    }
    _sg.next_draw_valid = true;
    _sg.cur_pipeline = ds->pipeline;

    /* lookup resource pointers, resources which are not in SG_RESOURCESTATE_VALID
       are not a fatal error, but supress the following drawcalls, this is to
       allow for simple asynchronous resource setup
    */
    _sg_pipeline* pip = _sg_lookup_pipeline(&_sg.pools, ds->pipeline.id);
    SOKOL_ASSERT(pip);
    _sg.next_draw_valid &= (SG_RESOURCESTATE_VALID == pip->slot.state);
    SOKOL_ASSERT(pip->shader && (pip->shader->slot.id == pip->shader_id.id));

    _sg_buffer* vbs[SG_MAX_SHADERSTAGE_BUFFERS] = { 0 };
    int num_vbs = 0;
    for (int i = 0; i < SG_MAX_SHADERSTAGE_BUFFERS; i++, num_vbs++) {
        if (ds->vertex_buffers[i].id) {
            vbs[i] = _sg_lookup_buffer(&_sg.pools, ds->vertex_buffers[i].id);
            SOKOL_ASSERT(vbs[i]);
            _sg.next_draw_valid &= (SG_RESOURCESTATE_VALID == vbs[i]->slot.state);
        }
        else {
            break;
        }
    }

    _sg_buffer* ib = 0;
    if (ds->index_buffer.id) {
        ib = _sg_lookup_buffer(&_sg.pools, ds->index_buffer.id);
        SOKOL_ASSERT(ib);
        _sg.next_draw_valid &= (SG_RESOURCESTATE_VALID == ib->slot.state);
    }

    _sg_image* vs_imgs[SG_MAX_SHADERSTAGE_IMAGES] = { 0 };
    int num_vs_imgs = 0;
    for (int i = 0; i < SG_MAX_SHADERSTAGE_IMAGES; i++, num_vs_imgs++) {
        if (ds->vs_images[i].id) {
            vs_imgs[i] = _sg_lookup_image(&_sg.pools, ds->vs_images[i].id);
            SOKOL_ASSERT(vs_imgs[i]);
            _sg.next_draw_valid &= (SG_RESOURCESTATE_VALID == vs_imgs[i]->slot.state);
        }
        else {
            break;
        }
    }

    _sg_image* fs_imgs[SG_MAX_SHADERSTAGE_IMAGES] = { 0 };
    int num_fs_imgs = 0;
    for (int i = 0; i < SG_MAX_SHADERSTAGE_IMAGES; i++, num_fs_imgs++) {
        if (ds->fs_images[i].id) {
            fs_imgs[i] = _sg_lookup_image(&_sg.pools, ds->fs_images[i].id);
            SOKOL_ASSERT(fs_imgs[i]);
            _sg.next_draw_valid &= (SG_RESOURCESTATE_VALID == fs_imgs[i]->slot.state);
        }
        else {
            break;
        }
    }
    if (_sg.next_draw_valid) {
        const uint32_t* vb_offsets = ds->vertex_buffer_offsets;
        uint32_t ib_offset = ds->index_buffer_offset;
        _sg_apply_draw_state(pip, vbs, vb_offsets, num_vbs, ib, ib_offset, vs_imgs, num_vs_imgs, fs_imgs, num_fs_imgs);
    }
}

void sg_apply_uniform_block(sg_shader_stage stage, int ub_index, const void* data, int num_bytes) {
    SOKOL_ASSERT((stage == SG_SHADERSTAGE_VS) || (stage == SG_SHADERSTAGE_FS));
    SOKOL_ASSERT((ub_index >= 0) && (ub_index < SG_MAX_SHADERSTAGE_UBS));
    SOKOL_ASSERT(data && (num_bytes > 0));
    if (!_sg_validate_apply_uniform_block(stage, ub_index, data, num_bytes)) {
        _sg.next_draw_valid = false;
        return;
    }
    if (!(_sg.pass_valid && _sg.next_draw_valid)) {
        return;
    }
    _sg_apply_uniform_block(stage, ub_index, data, num_bytes);
}

void sg_draw(int base_element, int num_elements, int num_instances) {
    if (!(_sg.pass_valid && _sg.next_draw_valid)) {
        return;
    }
    _sg_draw(base_element, num_elements, num_instances);
}

void sg_end_pass(void) {
    if (!_sg.pass_valid) {
        return;
    }
    _sg_end_pass();
    _sg.cur_pass.id = SG_INVALID_ID;
    _sg.cur_pipeline.id = SG_INVALID_ID;
    _sg.pass_valid = false;
}

void sg_commit() {
    _sg_commit();
    _sg.frame_index++;
}

void sg_reset_state_cache(void) {
    _sg_reset_state_cache();
}

void sg_update_buffer(sg_buffer buf_id, const void* data, int num_bytes) {
    if (num_bytes == 0) {
        return;
    }
    _sg_buffer* buf = _sg_lookup_buffer(&_sg.pools, buf_id.id);
    if (!(buf && buf->slot.state == SG_RESOURCESTATE_VALID)) {
        return;
    }
    if (_sg_validate_update_buffer(buf, data, num_bytes)) {
        SOKOL_ASSERT(buf->upd_frame_index != _sg.frame_index);
        _sg_update_buffer(buf, data, num_bytes);
        buf->upd_frame_index = _sg.frame_index;
    }
}

void sg_update_image(sg_image img_id, const sg_image_content* data) {
    _sg_image* img = _sg_lookup_image(&_sg.pools, img_id.id);
    if (!(img && img->slot.state == SG_RESOURCESTATE_VALID)) {
        return;
    }
    if (_sg_validate_update_image(img, data)) {
        SOKOL_ASSERT(img->upd_frame_index != _sg.frame_index);
        _sg_update_image(img, data);
        img->upd_frame_index = _sg.frame_index;
    }
}
#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* SOKOL_IMPL */
