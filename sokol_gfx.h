#pragma once
/*
    Configuration defines:

    SOKOL_IMPL      - define this exactly once to include implementation files
    SOKOL_ASSERT    - your own assert macro (default: assert())
    SOKOL_MALLOC    - your own malloc func (default: void* malloc(size))
    SOKOL_FREE      - your own free func (default: void free(void* p))
    SOKOL_USE_GL    - use the desktop GL3.3 backend
    SOKOL_USE_GLES2 - use the GLES2 backend
    SOKOL_USE_GLES3 - use the GLES3 backend (with soft fallback to GLES2)
    SOKOL_USE_D3D11 - use the D3D11 backend
    SOKOL_USE_METAL - use the Metal backend
*/
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>

#ifndef SOKOL_ASSERT
#include <assert.h>
#define SOKOL_ASSERT(c) assert(c)
#endif
#ifndef SOKOL_MALLOC
#define SOKOL_MALLOC(s) malloc(s)
#define SOKOL_FREE(p) free(p)
#endif

/* 
    sg_id

    Instead of pointers, resource creation functions return a 32-bit
    number which uniquely identifies the resource object.
*/
typedef uint32_t sg_id;

/* various constants */
enum {
    SG_INVALID_ID = 0,
    SG_DEFAULT_PASS = SG_INVALID_ID,
    SG_MAX_COLOR_ATTACHMENTS = 4,
    SG_MAX_SHADERSTAGE_BUFFERS = 4,
    SG_MAX_SHADERSTAGE_IMAGES = 12,
    SG_MAX_SHADERSTAGE_UBS = 4,
    SG_MAX_UNIFORMS = 16,
    SG_MAX_VERTEX_ATTRIBUTES = 16,
};

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
    SG_FEATURE_NATIVE_TEXTURE,
} sg_feature;

typedef enum {
    SG_SHADERSTAGE_VS,
    SG_SHADERSTAGE_FS,
} sg_shader_stage;

typedef enum {
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
    SG_CLAMP_TO_EDGE,
    SG_REPEAT,
    SG_MIRRORED_REPEAT,
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

/* describe a vertex attribute */
typedef struct {
    const char* name;
    sg_vertex_format format;
} sg_vertex_attr_desc;

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
    int width;
    int height;
    int sample_count;
    int resource_pool_size[SG_NUM_RESOURCETYPES];
} sg_desc;

typedef struct {
    uint32_t _init_guard;
    int size;
    sg_buffer_type type;
    sg_usage usage;
    void* data_ptr;
    int data_size;
} sg_buffer_desc;

typedef struct {
    uint32_t _init_guard;
    /* FIXME */
} sg_image_desc;

/* describe a uniform in a uniform block */
typedef struct {
    const char* name;
    sg_uniform_type type;
    int offset;
    int array_size; 
} sg_shader_uniform_desc;

typedef struct {
    int num_uniforms;
    sg_shader_uniform_desc uniforms[SG_MAX_UNIFORMS];
} sg_shader_uniform_block_desc;

typedef struct {
    const char* name;
    sg_image_type type;
} sg_shader_image_desc;

typedef struct {
    /* source code (only used in GL backends) */
    const char* source;
    /* number of uniform blocks on the shader stage */
    int num_uniform_blocks;
    /* number of textures on the shader stage */
    int num_textures;
    /* uniform block descriptions */
    sg_shader_uniform_block_desc uniform_blocks[SG_MAX_SHADERSTAGE_UBS];
    /* image descriptions */
    sg_shader_image_desc images[SG_MAX_SHADERSTAGE_IMAGES];
} sg_shader_stage_desc;

typedef struct {
    uint32_t _init_guard;
    sg_shader_stage_desc vs;
    sg_shader_stage_desc fs;
} sg_shader_desc;

typedef struct {
    int num_attrs;
    sg_vertex_attr_desc attrs[SG_MAX_VERTEX_ATTRIBUTES];
    sg_step_func step_func;
    int step_rate;
} sg_vertex_layout_desc;

typedef struct {
    uint32_t _init_guard;
    sg_id shader;
    sg_primitive_type primitive_type;
    sg_index_type index_type;
    sg_vertex_layout_desc layouts[SG_MAX_SHADERSTAGE_BUFFERS];
    sg_depth_stencil_state depth_stencil;
    sg_blend_state blend;
    sg_rasterizer_state rast;
} sg_pipeline_desc;

typedef struct {
    uint32_t _init_guard;
    /* FIXME */
} sg_pass_desc;

typedef struct {
    uint32_t _init_guard;
    sg_id pipeline;
    sg_id vertex_buffers[SG_MAX_SHADERSTAGE_BUFFERS];
    sg_id index_buffer;
    sg_id vs_images[SG_MAX_SHADERSTAGE_IMAGES];
    sg_id fs_images[SG_MAX_SHADERSTAGE_IMAGES];
} sg_draw_state;

typedef struct {
    uint32_t _init_guard;
    /* FIXME */
} sg_update_image_desc;

/* struct initializers */
extern void sg_init_desc(sg_desc* desc);
extern void sg_init_pass_action(sg_pass_action* pa);
extern void sg_init_buffer_desc(sg_buffer_desc* desc);
extern void sg_init_shader_desc(sg_shader_desc* desc);
extern void sg_shader_desc_attr(sg_shader_desc* desc, const char* name, sg_vertex_format format);
extern void sg_init_pipeline_desc(sg_pipeline_desc* desc);
extern void sg_pipeline_desc_named_attr(sg_pipeline_desc* desc, int slot, const char* name, sg_vertex_format format);
extern void sg_init_draw_state(sg_draw_state* ds);

/* setup */
extern void sg_setup(sg_desc* desc);
extern void sg_discard();
extern bool sg_isvalid();
extern bool sg_query_feature(sg_feature feature);

/* resources */
extern sg_id sg_make_buffer(sg_buffer_desc* desc);
extern void sg_destroy_buffer(sg_id buf);
extern sg_id sg_make_image(sg_image_desc* desc);
extern void sg_destroy_image(sg_id img);
extern sg_id sg_make_shader(sg_shader_desc* desc);
extern void sg_destroy_shader(sg_id shd);
extern sg_id sg_make_pipeline(sg_pipeline_desc* desc);
extern void sg_destroy_pipeline(sg_id pip);
extern sg_id sg_make_pass(sg_pass_desc* desc);
extern void sg_destroy_pass(sg_id pass);
extern void sg_update_buffer(sg_id buf, void* data, int num_bytes);
extern void sg_update_image(sg_id img, void* data, sg_update_image_desc* desc);

/* rendering */
extern void sg_begin_pass(sg_id pass, sg_pass_action* pass_action, int width, int height);
extern void sg_apply_viewport(int x, int y, int width, int height, bool origin_top_left);
extern void sg_apply_scissor_rect(int x, int y, int width, int height, bool origin_top_left);
extern void sg_apply_draw_state(sg_draw_state* ds);
extern void sg_apply_uniform_block(sg_shader_stage stage, int slot, void* data, int num_bytes);
extern void sg_draw(int base_element, int num_elements, int num_instances);
extern void sg_end_pass();
extern void sg_commit();
extern void sg_reset_state_cache();

/* separate resource allocation and initialization (for async setup) */ 
extern sg_id sg_alloc_buffer();
extern sg_id sg_alloc_image();
extern sg_id sg_alloc_shader();
extern sg_id sg_alloc_pipeline();
extern sg_id sg_alloc_pass();
extern void sg_init_buffer(sg_id buf_id, sg_buffer_desc* desc);
extern void sg_init_image(sg_id img_id, sg_image_desc* desc);
extern void sg_init_shader(sg_id shd_id, sg_shader_desc* desc);
extern void sg_init_pipeline(sg_id pip_id, sg_pipeline_desc* desc);
extern void sg_init_pass(sg_id pass_id, sg_pass_desc* desc);

#ifdef SOKOL_IMPL
#include "_sokol_gfx.impl.h"
#endif

