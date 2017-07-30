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

    Optionally provide the following defines with your own implementations:

    SOKOL_ASSERT(c)     - your own assert macro (default: assert(c))
    SOKOL_MALLOC(s)     - your own malloc function (default: malloc(s))
    SOKOL_FREE(p)       - your own free function (default: free(p))
    SOKOL_LOG(msg)      - your own logging function (default: puts(msg))
    SOKOL_UNREACHABLE() - a guard macro for unreachable code (default: assert(false))

    Optionally define the following to force debug checks and validations
    even in release mode:

    SOKOL_DEBUG         - by default this is defined if _DEBUG is defined
*/
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/* 
    Resource id typedefs:

    sg_buffer
    sg_image
    sg_shader
    sg_pipeline
    sg_pass

    Instead of pointers, resource creation functions return a 32-bit
    number which uniquely identifies the resource object.

    The 32-bit resource id is split into a 16-bit pool index in the lower bits, 
    and a 16-bit 'unique counter' in the upper bits. The index allows fast
    pool lookups, and combined with the unique-mask it allows to detect
    'dangling accesses' (trying to use an object which longer exists, and
    its pool slot has been reused for a new object)
*/
typedef uint32_t sg_buffer;
typedef uint32_t sg_image;
typedef uint32_t sg_shader;
typedef uint32_t sg_pipeline;
typedef uint32_t sg_pass;

/* 
    various compile-time constants

    FIXME: it may make sense to convert some of those into defines so
    that the user code can override them.
*/
enum {
    SG_INVALID_ID = 0,
    SG_DEFAULT_PASS = SG_INVALID_ID,
    SG_NUM_SHADER_STAGES = 2,
    SG_MAX_COLOR_ATTACHMENTS = 4,
    SG_MAX_SHADERSTAGE_BUFFERS = 4,
    SG_MAX_SHADERSTAGE_IMAGES = 12,
    SG_MAX_SHADERSTAGE_UBS = 4,
    SG_MAX_UNIFORMS = 16,
    SG_MAX_VERTEX_ATTRIBUTES = 16,
    SG_MAX_MIPMAPS = 16,
};

/*
    sg_feature

    These are optional features, use the function
    sg_query_feature() to check whether the feature is supported.
*/
typedef enum {
    SG_FEATURE_INSTANCED_ARRAYS = 0,
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
    SG_FEATURE_TEXTURE_3D,
    SG_FEATURE_TEXTURE_ARRAY,
    SG_NUM_FEATURES
} sg_feature;

/*
    sg_resource_type

    sokol gfx has 5 resource types:
    - buffer:   vertex and index buffers
    - image:    textures and render targets
    - shaders:  vertex and fragment shaders, uniform blocks
    - pipeline: encapsulates shader, render states and vertex layouts
    - pass:     encapsulates render pass operations (clear, msaa resolve, etc)
*/
typedef enum {
    SG_RESOURCETYPE_BUFFER = 0,
    SG_RESOURCETYPE_IMAGE,
    SG_RESOURCETYPE_SHADER,
    SG_RESOURCETYPE_PIPELINE,
    SG_RESOURCETYPE_PASS,

    SG_NUM_RESOURCETYPES
} sg_resource_type;

/*
    sg_resource_state

    The current state of a resource in one of the resource pools.
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
*/
typedef enum {
    SG_RESOURCESTATE_INITIAL,
    SG_RESOURCESTATE_ALLOC,
    SG_RESOURCESTATE_VALID,
    SG_RESOURCESTATE_FAILED,
} sg_resource_state;

/*
    sg_buffer_type

    Buffers come in 2 flavours, vertex- and index-buffers.
*/
typedef enum {
    SG_BUFFERTYPE_VERTEXBUFFER,
    SG_BUFFERTYPE_INDEXBUFFER
} sg_buffer_type;

typedef enum {
    SG_IMAGETYPE_INVALID,
    SG_IMAGETYPE_2D,
    SG_IMAGETYPE_CUBE,
    SG_IMAGETYPE_3D,
    SG_IMAGETYPE_ARRAY,
} sg_image_type;

typedef enum {
    SG_INDEXTYPE_NONE,
    SG_INDEXTYPE_UINT16,
    SG_INDEXTYPE_UINT32,
} sg_index_type;

typedef enum {
    SG_SHADERSTAGE_VS,
    SG_SHADERSTAGE_FS,
} sg_shader_stage;

typedef enum {
    SG_PIXELFORMAT_NONE = 0,
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
} sg_pixel_format;

typedef enum {
    SG_PRIMITIVETYPE_POINTS,
    SG_PRIMITIVETYPE_LINES,
    SG_PRIMITIVETYPE_LINE_STRIP,
    SG_PRIMITIVETYPE_TRIANGLES,
    SG_PRIMITIVETYPE_TRIANLE_STRIP,
} sg_primitive_type;

typedef enum {
    SG_FILTER_NEAREST,
    SG_FILTER_LINEAR,
    SG_FILTER_NEAREST_MIPMAP_NEAREST,
    SG_FILTER_NEAREST_MIPMAP_LINEAR,
    SG_FILTER_LINEAR_MIPMAP_NEAREST,
    SG_FILTER_LINEAR_MIPMAP_LINEAR,
} sg_filter;

typedef enum {
    SG_WRAP_CLAMP_TO_EDGE,
    SG_WRAP_REPEAT,
    SG_WRAP_MIRRORED_REPEAT,
} sg_wrap;

typedef enum {
    SG_USAGE_IMMUTABLE,
    SG_USAGE_DYNAMIC,
    SG_USAGE_STREAM,
} sg_usage;

typedef enum {
    SG_VERTEXFORMAT_INVALID = 0,
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
} sg_vertex_format;

typedef enum {
    SG_UNIFORMTYPE_INVALID,
    SG_UNIFORMTYPE_FLOAT,
    SG_UNIFORMTYPE_FLOAT2,
    SG_UNIFORMTYPE_FLOAT3,
    SG_UNIFORMTYPE_FLOAT4,
    SG_UNIFORMTYPE_MAT4,
} sg_uniform_type;

typedef enum {
    SG_FACE_FRONT,
    SG_FACE_BACK,
    SG_FACE_BOTH,
} sg_face;

typedef enum {
    SG_COMPAREFUNC_NEVER,
    SG_COMPAREFUNC_LESS,
    SG_COMPAREFUNC_EQUAL,
    SG_COMPAREFUNC_LESS_EQUAL,
    SG_COMPAREFUNC_GREATER,
    SG_COMPAREFUNC_NOT_EQUAL,
    SG_COMPAREFUNC_GREATER_EQUAL,
    SG_COMPAREFUNC_ALWAYS,
} sg_compare_func;

typedef enum {
    SG_STENCILOP_KEEP,
    SG_STENCILOP_ZERO,
    SG_STENCILOP_REPLACE,
    SG_STENCILOP_INCR_CLAMP,
    SG_STENCILOP_DECR_CLAMP,
    SG_STENCILOP_INVERT,
    SG_STENCILOP_INCR_WRAP,
    SG_STENCILOP_DECR_WRAP,
} sg_stencil_op;

typedef enum {
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
} sg_blend_factor;

typedef enum {
    SG_BLENDOP_ADD,
    SG_BLENDOP_SUBTRACT,
    SG_BLENDOP_REVERSE_SUBTRACT,
} sg_blend_op;

typedef enum {
    SG_STEPFUNC_PER_VERTEX,
    SG_STEPFUNC_PER_INSTANCE,
} sg_step_func;

typedef enum {
    SG_COLORMASK_R = (1<<0),
    SG_COLORMASK_G = (1<<1),
    SG_COLORMASK_B = (1<<2),
    SG_COLORMASK_A = (1<<3),
    SG_COLORMASK_RGBA = 0xF,
} sg_color_mask;

/*
    sg_pass_action_bits

    Describes the actions that should be performed at the start of
    a rendering pass, this includes clearing the color, depth and 
    stencil buffers, or loading the previous content of those
    buffers. If no pass actions bits are set, the associated
    buffer content will be discarded.
*/
typedef enum {
    SG_PASSACTION_CLEAR_COLOR0  = (1<<0),
    SG_PASSACTION_CLEAR_COLOR1  = (1<<1),
    SG_PASSACTION_CLEAR_COLOR2  = (1<<2),
    SG_PASSACTION_CLEAR_COLOR3  = (1<<3),
    SG_PASSACTION_CLEAR_COLOR   = (1<<0)|(1<<1)|(1<<2)|(1<<3),
    SG_PASSACTION_CLEAR_DEPTH   = (1<<4),
    SG_PASSACTION_CLEAR_STENCIL = (1<<5),
    SG_PASSACTION_CLEAR_DEPTH_STENCIL = (1<<4)|(1<<5),
    SG_PASSACTION_CLEAR_ALL     = SG_PASSACTION_CLEAR_COLOR|SG_PASSACTION_CLEAR_DEPTH_STENCIL,
    SG_PASSACTION_LOAD_COLOR0   = (1<<6),
    SG_PASSACTION_LOAD_COLOR1   = (1<<7),
    SG_PASSACTION_LOAD_COLOR2   = (1<<8),
    SG_PASSACTION_LOAD_COLOR3   = (1<<9),
    SG_PASSACTION_LOAD_COLOR    = (1<<6)|(1<<7)|(1<<8)|(1<<9),
    SG_PASSACTION_LOAD_DEPTH    = (1<<10),
    SG_PASSACTION_LOAD_STENCIL  = (1<<11),
    SG_PASSACTION_LOAD_DEPTH_STENCIL = (1<<10)|(1<<11),
    SG_PASSACTION_LOAD_ALL = SG_PASSACTION_LOAD_COLOR|SG_PASSACTION_LOAD_DEPTH_STENCIL,
} sg_pass_action_bits;

typedef struct {
    uint32_t _init_guard;
    float color[SG_MAX_COLOR_ATTACHMENTS][4];
    float depth;
    uint8_t stencil;
    sg_pass_action_bits actions;
} sg_pass_action;

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
    float blend_color[4];
} sg_blend_state;

typedef struct {
    bool cull_face_enabled;
    bool scissor_test_enabled;
    bool dither_enabled;
    bool alpha_to_coverage_enabled;
    sg_face cull_face;
    int sample_count;
} sg_rasterizer_state;

/*-- setup descriptor structs ------------------------------------------------*/
/*
    sg_desc

    This describes initialization attributes for the entire sokol_gfx
    library. First initialize the structure with sg_init_desc(),
    then modify struct members to your needs and finally call
    sg_setup():

    sg_desc desc;
    sg_init_desc(&desc);
    desc.width = WIDTH;
    ...
    sg_setup(&desc);
*/
typedef struct {
    uint32_t _init_guard;
    int resource_pool_size[SG_NUM_RESOURCETYPES];
} sg_desc;

typedef struct {
    uint32_t _init_guard;
    int size;
    sg_buffer_type type;
    sg_usage usage;
    const void* data_ptr;
    int data_size;
} sg_buffer_desc;

typedef struct {
    uint32_t _init_guard;
    sg_image_type type;
    bool render_target;
    uint16_t width;
    uint16_t height;
    union {
        uint16_t depth;
        uint16_t layers;
    };
    uint16_t num_mipmaps;
    sg_usage usage;
    sg_pixel_format color_format;
    sg_pixel_format depth_format;   /* render targets only */
    int sample_count;               /* render targets only */
    sg_filter min_filter;
    sg_filter mag_filter;
    sg_wrap wrap_u;
    sg_wrap wrap_v;
    sg_wrap wrap_w;
    int num_data_items;
    const void** data_ptrs;
    const int* data_sizes;
} sg_image_desc;

/* describe a uniform in a uniform block */
typedef struct {
    const char* name;
    int offset;
    sg_uniform_type type;   /* SG_UNIFORMTYPE_INVALID if not used */
    int array_count; 
} sg_shader_uniform_desc;

typedef struct {
    int size;
    int num_uniforms;
    sg_shader_uniform_desc u[SG_MAX_UNIFORMS];
} sg_shader_uniform_block_desc;

typedef struct {
    const char* name;
    sg_image_type type;     /* SG_IMAGETYPE_INVALID if not used */
} sg_shader_image_desc;

typedef struct {
    /* source code (only used in GL backends) */
    const char* source;
    /* uniform block descriptions */
    int num_ubs;
    sg_shader_uniform_block_desc ub[SG_MAX_SHADERSTAGE_UBS];
    /* image descriptions */
    int num_images;
    sg_shader_image_desc image[SG_MAX_SHADERSTAGE_IMAGES];
} sg_shader_stage_desc;

typedef struct {
    uint32_t _init_guard;
    sg_shader_stage_desc vs;
    sg_shader_stage_desc fs;
} sg_shader_desc;

/*
    sg_vertex_attr_desc

    A vertex attribute can be either bound by name or by bind location,
    with the restriction that on GLES2, only binding by attribute
    name is allowed.
*/
typedef struct {
    const char* name;
    int index;
    int offset;
    sg_vertex_format format;
} sg_vertex_attr_desc;

typedef struct {
    int stride;
    int num_attrs;
    sg_vertex_attr_desc attrs[SG_MAX_VERTEX_ATTRIBUTES];
    sg_step_func step_func;
    int step_rate;
} sg_vertex_layout_desc;

typedef struct {
    uint32_t _init_guard;
    sg_shader shader;
    sg_primitive_type primitive_type;
    sg_index_type index_type;
    sg_vertex_layout_desc input_layouts[SG_MAX_SHADERSTAGE_BUFFERS];
    sg_depth_stencil_state depth_stencil;
    sg_blend_state blend;
    sg_rasterizer_state rast;
} sg_pipeline_desc;

typedef struct {
    sg_image image;
    uint16_t mip_level;
    uint16_t slice;
} sg_attachment_desc;

typedef struct {
    uint32_t _init_guard;
    sg_attachment_desc color_attachments[SG_MAX_COLOR_ATTACHMENTS];
    sg_attachment_desc depth_stencil_attachment;
} sg_pass_desc;

typedef struct {
    uint32_t _init_guard;
    sg_pipeline pipeline;
    sg_buffer vertex_buffers[SG_MAX_SHADERSTAGE_BUFFERS];
    sg_buffer index_buffer;
    sg_image vs_images[SG_MAX_SHADERSTAGE_IMAGES];
    sg_image fs_images[SG_MAX_SHADERSTAGE_IMAGES];
} sg_draw_state;

/* struct initializers */
extern void sg_init_desc(sg_desc* desc);
extern void sg_init_buffer_desc(sg_buffer_desc* desc);
extern void sg_init_image_desc(sg_image_desc* desc);
extern void sg_init_shader_desc(sg_shader_desc* desc);
extern void sg_init_uniform_block(sg_shader_desc* desc, sg_shader_stage stage, int ub_size); 
extern void sg_init_named_uniform(sg_shader_desc* desc, sg_shader_stage stage, const char* name, int ub_offset, sg_uniform_type type, int array_count);
extern void sg_init_named_image(sg_shader_desc* desc, sg_shader_stage stage, const char* name, sg_image_type type);
extern void sg_init_pipeline_desc(sg_pipeline_desc* desc);
extern void sg_init_vertex_stride(sg_pipeline_desc* desc, int input_slot, int stride);
extern void sg_init_vertex_step(sg_pipeline_desc* desc, int input_slot, sg_step_func step_func, int step_rate);
extern void sg_init_named_vertex_attr(sg_pipeline_desc* desc, int input_slot, const char* name, int offset, sg_vertex_format format);
extern void sg_init_indexed_vertex_attr(sg_pipeline_desc* desc, int input_slot, int attr_index, int offset, sg_vertex_format format);
extern void sg_init_pass_desc(sg_pass_desc* desc);
extern void sg_init_pass_action(sg_pass_action* pa);
extern void sg_init_draw_state(sg_draw_state* ds);

/* setup */
extern void sg_setup(const sg_desc* desc);
extern void sg_shutdown();
extern bool sg_isvalid();
extern bool sg_query_feature(sg_feature feature);

/* resources */
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
extern void sg_update_image(sg_image img, int num_data_items, const void** data_ptrs, int* data_sizes); 

/* rendering */
extern void sg_begin_default_pass(const sg_pass_action* pass_action, int width, int height);
extern void sg_begin_pass(sg_pass pass, const sg_pass_action* pass_action);
extern void sg_apply_viewport(int x, int y, int width, int height, bool origin_top_left);
extern void sg_apply_scissor_rect(int x, int y, int width, int height, bool origin_top_left);
extern void sg_apply_draw_state(const sg_draw_state* ds);
extern void sg_apply_uniform_block(sg_shader_stage stage, int ub_index, const void* data, int num_bytes);
extern void sg_draw(int base_element, int num_elements, int num_instances);
extern void sg_end_pass();
extern void sg_commit();
extern void sg_reset_state_cache();

/* separate resource allocation and initialization (for async setup) */ 
extern sg_buffer sg_alloc_buffer();
extern sg_image sg_alloc_image();
extern sg_shader sg_alloc_shader();
extern sg_pipeline sg_alloc_pipeline();
extern sg_pass sg_alloc_pass();
extern void sg_init_buffer(sg_buffer buf_id, const sg_buffer_desc* desc);
extern void sg_init_image(sg_image img_id, const sg_image_desc* desc);
extern void sg_init_shader(sg_shader shd_id, const sg_shader_desc* desc);
extern void sg_init_pipeline(sg_pipeline pip_id, const sg_pipeline_desc* desc);
extern void sg_init_pass(sg_pass pass_id, const sg_pass_desc* desc);

#ifdef __cplusplus
} /* extern "C" */
#endif

#ifdef SOKOL_IMPL
#define SOKOL_IMPL_GUARD
#include "_sokol_gfx.impl.h"
#undef SOKOL_IMPL_GUARD
#endif

