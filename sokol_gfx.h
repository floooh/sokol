#pragma once
/*
    FIXME
*/
#include <stdint.h>
#include <stdbool.h>

typedef uint32_t sg_label;
typedef uint32_t sg_id;

struct sg_pass_action {
    /* FIXME! */
};

typedef struct {
    int width;
    int height;
    int sample_count;
    sg_pass_action default_pass_action;
    int buffer_pool_size;
    int image_pool_size;
    int shader_pool_size;
    int pipeline_pool_size;
    int pass_pool_size;
} sg_setup_desc;

typedef enum {
    SG_UINT16,
    SG_UINT32,
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
    SG_STAGE_VS,
    SG_STAGE_FS,
} sg_stage;

typedef enum {
    SG_RGBA8,
    SG_RGB8,
    SG_RGBA4,
    SG_R5G6B5,
    SG_R5G5B5A1,
    SG_R10G10B10A2,
    SG_RGBA32F,
    SG_RGBA16F,
    SG_R32F,
    SG_R16F,
    SG_L8,
    SG_DXT1,
    SG_DXT3,
    SG_DXT5,
    SG_DEPTH,
    SG_DEPTHSTENCIL,
    SG_PVRTC2_RGB,
    SG_PVRTC4_RGB,
    SG_PVRTC2_RGBA,
    SG_ETC2_RGB8,
    SG_ETC2_SRGB8,
} sg_pixel_format;

typedef enum {
    SG_POINTS,
    SG_LINES,
    SG_LINE_STRIP,
    SG_TRIANGLES,
    SG_TRIANLE_STRIP,
} sg_primitive_type;

typedef enum {
    SG_NEAREST,
    SG_LINEAR,
    SG_NEAREST_MIPMAP_NEAREST,
    SG_NEAREST_MIPMAP_LINEAR,
    SG_LINEAR_MIPMAP_NEAREST,
    SG_LINEAR_MIPMAP_LINEAR,
} sg_filter;

typedef enum {
    SG_IMAGE_2D,
    SG_IMAGE_CUBE,
    SG_IMAGE_3D,
    SG_IMAGE_ARRAY,
} sg_image_type;

typedef enum {
    SG_CLAMP_TO_EDGE,
    SG_REPEAT,
    SG_MIRRORED_REPEAT,
} sg_wrap;

typedef enum {
    SG_IMMUTABLE,
    SG_DYNAMIC,
    SG_STREAM,
} sg_usage;

typedef enum {
    SG_POSITION,
    SG_NORMAL,
    SG_TEXCOORD_0,
    SG_TEXCOORD_1,
    SG_EXCOORD_2,
    SG_TEXCOORD_3,
    SG_TANGENT,
    SG_BINORMAL,
    SG_WEIGHTS,
    SG_INDICES,
    SG_COLOR_0,
    SG_COLOR_1,
    SG_INSTANCE_0,
    SG_INSTANCE_1,
    SG_INSTANCE_2,
    SG_INSTANCE_3,
} sg_vertex_attr;

typedef enum {
    SG_FLOAT,
    SG_FLOAT2,
    SG_FLOAT3,
    SG_BYTE4,
    SG_BYTE4N,
    SG_UBYTE4,
    SG_SHORT2,
    SG_SHORT2N,
    SG_SHORT4,
    SG_SHORT4N,
    SG_UINT10_N2,
} sg_vertex_format;

typedef enum {
    SG_GLSL100,
    SG_GLSL330,
    SG_GLSLES3,
    SG_HLSL5,
    SG_METAL,
} sg_shader_lang;

typedef enum {
    SG_FACE_FRONT,
    SG_FACE_BACK,
    SG_FACE_BOTH,
} sg_face;

typedef enum {
    SG_CMP_NEVER,
    SG_CMP_LESS,
    SG_CMP_EQUAL,
    SG_CMP_LESS_EQUAL,
    SG_CMP_GREATER,
    SG_CMP_NOT_EQUAL,
    SG_CMP_GREATER_EQUAL,
    SG_CMP_ALWAYS,
} sg_compare_func;

typedef enum {
    SG_STENCIL_KEEP,
    SG_STENCIL_ZERO,
    SG_STENCIL_REPLACE,
    SG_STENCIL_INCR_CLAMP,
    SG_STENCIL_DECR_CLAMP,
    SG_STENCIL_INVERT,
    SG_STENCIL_INCR_WRAP,
    SG_STENCIL_DECR_WRAP,
} sg_stencil_op;

typedef enum {
    SG_BLEND_FACTOR_ZERO,
    SG_BLEND_FACTOR_ONE,
    SG_BLEND_FACTOR_SRC_COLOR,
    SG_BLEND_FACTOR_ONE_MINUS_SRC_COLOR,
    SG_BLEND_FACTOR_SRC_ALPHA,
    SG_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,
    SG_BLEND_FACTOR_DST_COLOR,
    SG_BLEND_FACTOR_ONE_MINUS_DST_COLOR,
    SG_BLEND_FACTOR_DST_ALPHA,
    SG_BLEND_FACTOR_ONE_MINUS_DST_ALPHA,
    SG_BLEND_FACTOR_SRC_ALPHA_SATURATED,
    SG_BLEND_FACTOR_BLEND_COLOR,
    SG_BLEND_FACTOR_ONE_MINUS_BLEND_COLOR,
    SG_BLEND_FACTOR_BLEND_ALPHA,
    SG_BLEND_FACTOR_ONE_MINUS_BLEND_ALPHA,
} sg_blend_factor;

typedef enum {
    SG_BLEND_OP_ADD,
    SG_BLEND_OP_SUBTRACT,
    SG_BLEND_OP_REVERSE_SUBTRACT,
} sg_blend_op;

typedef enum {
    SG_STEP_PER_VERTEX,
    SG_STEP_PER_INSTANCE,
} sg_step_func;

typedef struct {
    /* FIXME */
} sg_buffer_desc;

typedef struct {
    /* FIXME */
} sg_image_desc;

typedef struct {
    /* FIXME */
} sg_shader_desc;

typedef struct {
    /* FIXME */
} sg_pipeline_desc;

typedef struct {
    /* FIXME */
} sg_pass_desc;

typedef struct {
    /* FIXME */
} sg_draw_state;

typedef struct {
    /* FIXME */
} sg_update_image_desc;

extern void sg_setup(sg_setup_desc* desc);
extern void sg_discard();
extern bool sg_isvalid();
extern bool sg_query_feature(sg_feature feature);

extern sg_label sg_gen_label();
extern void sg_push_label(sg_label);
extern sg_label sg_pop_label();

extern void sg_init_buffer_desc(sg_buffer_desc* desc);
extern void sg_init_image_desc(sg_image_desc* desc);
extern void sg_init_shader_desc(sg_shader_desc* desc);
extern void sg_init_pipeline_desc(sg_pipeline_desc* desc);
extern void sg_init_pass_desc(sg_pass_desc* desc);
extern void sg_init_pass_action(sg_pass_action* pass_action);

extern sg_id sg_make_buffer(sg_buffer_desc* desc, void* opt_data, int opt_bytes);
extern sg_id sg_make_image(sg_image_desc* desc, void* opt_data, int opt_bytes);
extern sg_id sg_make_pipeline(sg_pipeline_desc* desc);
extern sg_id sg_make_shader(sg_shader_desc* desc);
extern sg_id sg_make_pass(sg_pass_desc* desc);
extern void sg_destroy(sg_label label);

extern void sg_begin_pass(sg_id pass_id, sg_pass_action* pass_action);
extern void sg_end_pass();

extern void sg_apply_viewport(int x, int y, int width, int height, bool origin_top_left);
extern void sg_apply_scissor_rect(int x, int y, int width, int height, bool origin_top_left);
extern void sg_apply_draw_state(sg_draw_state* ds);
extern void sg_apply_uniform_block(sg_stage stage, int slot, void* data, int num_bytes);

extern void sg_update_buffer(sg_id buf_id, void* data, int num_bytes);
extern void sg_update_image(sg_id img_id, void* data, sg_update_image_desc* desc);

extern void sg_draw(int base_element, int num_elements, int num_instances);

extern void sg_commit();
extern void sg_reset_state_cache();

extern sg_id sg_alloc_buffer();
extern sg_id sg_alloc_image();
extern void sg_init_buffer(sg_id buf_id, sg_buffer_desc* desc);
extern void sg_init_image(sg_id img_id, sg_image_desc* desc);

#ifdef SOKOL_IMPLEMENTATION
#ifdef SOKOL_GFX_USE_GL
#include "_sokol_gfx_gl.inl"
#endif
#include "_sokol_gfx.inl"
#endif

