/*
    LLM maintained.

    metal_mock.h -- public API of the mocked Metal runtime.

    Include this header *before* sokol_gfx.h. The mock headers in
    tests/mocks/metal/include shadow Metal.framework and the CAMetalDrawable
    part of QuartzCore, so the sokol_gfx.h Metal backend can be built and
    unit-tested without a GPU, a driver or a window.

    The mock offers four inspection and control areas:

    - object tracking: live and created counts plus per-object state for
      buffers, textures, samplers, libraries, functions, pipeline states,
      depth-stencil states, command queues, command buffers and encoders
    - call log: every mocked Metal call with its arguments, in call order
    - pass and encoder state: the last render-pass descriptor and the
      bindings and render state set on the current encoder
    - fault injection: forced creation failures with a custom NSError message
      and a configurable MTLGPUFamily support mask

    Object handles are `const void*` so the values returned by
    sg_mtl_query_*_info() can be passed in directly.

    Known limitations:

    - object identity is the raw heap address. A handle of a released object
      may compare equal to a later object allocated at the same address, so
      only compare handles of objects which are known to be alive
    - there is one global render-encoder and one global compute-encoder state.
      sokol-gfx has at most one active encoder, so this is enough, but the
      state of a previous encoder is lost when a new one is created

    Not thread-safe -- this is intended for the sokol-gfx unit-test loop,
    which runs on a single thread.
*/
#ifndef METAL_MOCK_H_INCLUDED
#define METAL_MOCK_H_INCLUDED

#import <Metal/Metal.h>
#import <QuartzCore/CoreAnimation.h>
#include <stdbool.h>

#define METAL_MOCK_MAX_CALLS (4096)
#define METAL_MOCK_MAX_CALL_ARGS (8)
#define METAL_MOCK_MAX_COLOR_ATTACHMENTS (8)
#define METAL_MOCK_MAX_VERTEX_ATTRIBUTES (16)
// sokol maps vertex buffer bind slots into the upper Metal buffer slots (23..30)
#define METAL_MOCK_MAX_VERTEX_LAYOUTS (32)
#define METAL_MOCK_MAX_PIPELINE_BUFFERS (32)
#define METAL_MOCK_MAX_BINDING_SLOTS (32)
#define METAL_MOCK_MAX_STRING (128)

// every mocked Metal method, as an x-macro list
#define _MTLM_FUNCS \
    _MTLM_XMACRO(newCommandQueue) \
    _MTLM_XMACRO(newBufferWithLength) \
    _MTLM_XMACRO(newBufferWithBytes) \
    _MTLM_XMACRO(newTextureWithDescriptor) \
    _MTLM_XMACRO(newSamplerStateWithDescriptor) \
    _MTLM_XMACRO(newLibraryWithSource) \
    _MTLM_XMACRO(newLibraryWithData) \
    _MTLM_XMACRO(newRenderPipelineStateWithDescriptor) \
    _MTLM_XMACRO(newComputePipelineStateWithDescriptor) \
    _MTLM_XMACRO(newDepthStencilStateWithDescriptor) \
    _MTLM_XMACRO(supportsFamily) \
    _MTLM_XMACRO(contents) \
    _MTLM_XMACRO(didModifyRange) \
    _MTLM_XMACRO(replaceRegion) \
    _MTLM_XMACRO(newTextureViewWithPixelFormat) \
    _MTLM_XMACRO(newFunctionWithName) \
    _MTLM_XMACRO(commandBuffer) \
    _MTLM_XMACRO(commandBufferWithUnretainedReferences) \
    _MTLM_XMACRO(enqueue) \
    _MTLM_XMACRO(commit) \
    _MTLM_XMACRO(addCompletedHandler) \
    _MTLM_XMACRO(presentDrawable) \
    _MTLM_XMACRO(renderCommandEncoderWithDescriptor) \
    _MTLM_XMACRO(computeCommandEncoder) \
    _MTLM_XMACRO(endEncoding) \
    _MTLM_XMACRO(pushDebugGroup) \
    _MTLM_XMACRO(popDebugGroup) \
    _MTLM_XMACRO(setViewport) \
    _MTLM_XMACRO(setScissorRect) \
    _MTLM_XMACRO(setCullMode) \
    _MTLM_XMACRO(setFrontFacingWinding) \
    _MTLM_XMACRO(setBlendColor) \
    _MTLM_XMACRO(setStencilReferenceValue) \
    _MTLM_XMACRO(setDepthBias) \
    _MTLM_XMACRO(setRenderPipelineState) \
    _MTLM_XMACRO(setDepthStencilState) \
    _MTLM_XMACRO(setVertexBuffer) \
    _MTLM_XMACRO(setVertexBufferOffset) \
    _MTLM_XMACRO(setVertexTexture) \
    _MTLM_XMACRO(setVertexSamplerState) \
    _MTLM_XMACRO(setFragmentBuffer) \
    _MTLM_XMACRO(setFragmentBufferOffset) \
    _MTLM_XMACRO(setFragmentTexture) \
    _MTLM_XMACRO(setFragmentSamplerState) \
    _MTLM_XMACRO(drawPrimitives) \
    _MTLM_XMACRO(drawPrimitivesBaseInstance) \
    _MTLM_XMACRO(drawIndexedPrimitives) \
    _MTLM_XMACRO(drawIndexedPrimitivesBaseVertex) \
    _MTLM_XMACRO(setComputePipelineState) \
    _MTLM_XMACRO(setBuffer) \
    _MTLM_XMACRO(setBufferOffset) \
    _MTLM_XMACRO(setTexture) \
    _MTLM_XMACRO(setSamplerState) \
    _MTLM_XMACRO(dispatchThreadgroups)

typedef enum {
    METAL_MOCK_FUNC_INVALID = 0,
    #define _MTLM_XMACRO(name) METAL_MOCK_FUNC_##name,
    _MTLM_FUNCS
    #undef _MTLM_XMACRO
    METAL_MOCK_FUNC_NUM,
} metal_mock_func_t;

// tracked object kinds
typedef enum {
    METAL_MOCK_OBJ_BUFFER = 0,
    METAL_MOCK_OBJ_TEXTURE,
    METAL_MOCK_OBJ_TEXTURE_VIEW,    // created via newTextureViewWithPixelFormat
    METAL_MOCK_OBJ_SAMPLER,
    METAL_MOCK_OBJ_LIBRARY,
    METAL_MOCK_OBJ_FUNCTION,
    METAL_MOCK_OBJ_RENDER_PIPELINE,
    METAL_MOCK_OBJ_COMPUTE_PIPELINE,
    METAL_MOCK_OBJ_DEPTH_STENCIL,
    METAL_MOCK_OBJ_COMMAND_QUEUE,
    METAL_MOCK_OBJ_COMMAND_BUFFER,
    METAL_MOCK_OBJ_RENDER_ENCODER,
    METAL_MOCK_OBJ_COMPUTE_ENCODER,
    METAL_MOCK_OBJ_DRAWABLE,
    METAL_MOCK_OBJ_NUM,
} metal_mock_obj_t;

// one logged call argument, the reader must know the argument type
typedef union {
    int64_t i;
    uint64_t u;
    double f;
    const void* p;
} metal_mock_arg_t;

typedef struct {
    metal_mock_func_t func;
    const void* obj;        // receiver of the message
    int num_args;
    metal_mock_arg_t args[METAL_MOCK_MAX_CALL_ARGS];
    // String arguments are copied here (truncated) and the matching
    // args[].p points into this buffer. Never store the NSString bytes
    // directly, those die with the autorelease pool.
    char str[METAL_MOCK_MAX_STRING];
} metal_mock_call_t;

typedef struct {
    NSUInteger length;
    MTLResourceOptions options;
    bool with_bytes;            // created via newBufferWithBytes
    int num_did_modify_range;
    NSUInteger last_modified_offset;
    NSUInteger last_modified_length;
    char label[METAL_MOCK_MAX_STRING];
} metal_mock_buffer_info_t;

typedef struct {
    MTLTextureType texture_type;
    MTLPixelFormat pixel_format;
    NSUInteger width, height, depth;
    NSUInteger mipmap_level_count;
    NSUInteger sample_count;
    NSUInteger array_length;
    MTLTextureUsage usage;
    MTLResourceOptions resource_options;
    bool is_view;               // created via newTextureViewWithPixelFormat
    const void* view_source;    // only valid if 'is_view' is true, retained by the view
    int num_replace_region;
    char label[METAL_MOCK_MAX_STRING];
} metal_mock_texture_info_t;

typedef struct {
    MTLSamplerMinMagFilter min_filter, mag_filter;
    MTLSamplerMipFilter mip_filter;
    MTLSamplerAddressMode s_address_mode, t_address_mode, r_address_mode;
    MTLSamplerBorderColor border_color;
    NSUInteger max_anisotropy;
    float lod_min_clamp, lod_max_clamp;
    MTLCompareFunction compare_function;
    BOOL normalized_coordinates;
    char label[METAL_MOCK_MAX_STRING];
} metal_mock_sampler_info_t;

typedef struct {
    bool from_bytecode;         // created via newLibraryWithData
    char source[METAL_MOCK_MAX_STRING];   // first source chars, truncated
    char label[METAL_MOCK_MAX_STRING];
} metal_mock_library_info_t;

typedef struct {
    const void* library;
    char name[METAL_MOCK_MAX_STRING];
} metal_mock_function_info_t;

typedef struct {
    MTLPixelFormat pixel_format;
    BOOL blending_enabled;
    MTLBlendFactor src_rgb, dst_rgb;
    MTLBlendFactor src_alpha, dst_alpha;
    MTLBlendOperation op_rgb, op_alpha;
    MTLColorWriteMask write_mask;
} metal_mock_color_attachment_info_t;

typedef struct {
    MTLVertexFormat format;
    NSUInteger offset;
    NSUInteger buffer_index;
} metal_mock_vertex_attr_info_t;

typedef struct {
    NSUInteger stride;
    MTLVertexStepFunction step_function;
    NSUInteger step_rate;
} metal_mock_vertex_layout_info_t;

typedef struct {
    const void* vertex_function;
    const void* fragment_function;
    bool has_vertex_descriptor;
    NSUInteger raster_sample_count;
    BOOL alpha_to_coverage_enabled;
    BOOL alpha_to_one_enabled;
    BOOL rasterization_enabled;
    MTLPixelFormat depth_attachment_pixel_format;
    MTLPixelFormat stencil_attachment_pixel_format;
    metal_mock_color_attachment_info_t color_attachments[METAL_MOCK_MAX_COLOR_ATTACHMENTS];
    metal_mock_vertex_attr_info_t vertex_attrs[METAL_MOCK_MAX_VERTEX_ATTRIBUTES];
    metal_mock_vertex_layout_info_t vertex_layouts[METAL_MOCK_MAX_VERTEX_LAYOUTS];
    MTLMutability vertex_buffer_mutability[METAL_MOCK_MAX_PIPELINE_BUFFERS];
    MTLMutability fragment_buffer_mutability[METAL_MOCK_MAX_PIPELINE_BUFFERS];
    char label[METAL_MOCK_MAX_STRING];
} metal_mock_render_pipeline_info_t;

typedef struct {
    const void* compute_function;
    BOOL threadgroup_size_is_multiple_of_thread_execution_width;
    MTLMutability buffer_mutability[METAL_MOCK_MAX_PIPELINE_BUFFERS];
    char label[METAL_MOCK_MAX_STRING];
} metal_mock_compute_pipeline_info_t;

typedef struct {
    MTLCompareFunction compare_function;
    MTLStencilOperation fail_op, depth_fail_op, pass_op;
    uint32_t read_mask, write_mask;
} metal_mock_stencil_info_t;

typedef struct {
    MTLCompareFunction depth_compare_function;
    BOOL depth_write_enabled;
    bool has_front_stencil, has_back_stencil;
    metal_mock_stencil_info_t front, back;
    char label[METAL_MOCK_MAX_STRING];
} metal_mock_depth_stencil_info_t;

// snapshot of one MTLRenderPassAttachmentDescriptor
typedef struct {
    const void* texture;
    NSUInteger level, slice, depth_plane;
    const void* resolve_texture;
    NSUInteger resolve_level, resolve_slice, resolve_depth_plane;
    MTLLoadAction load_action;
    MTLStoreAction store_action;
    MTLClearColor clear_color;      // color attachments only
    double clear_depth;             // depth attachment only
    uint32_t clear_stencil;         // stencil attachment only
} metal_mock_pass_attachment_t;

// snapshot of the MTLRenderPassDescriptor of the most recent
// renderCommandEncoderWithDescriptor: call
typedef struct {
    int num_color_attachments;      // highest used index + 1
    metal_mock_pass_attachment_t color_attachments[METAL_MOCK_MAX_COLOR_ATTACHMENTS];
    bool has_depth_attachment;
    metal_mock_pass_attachment_t depth_attachment;
    bool has_stencil_attachment;
    metal_mock_pass_attachment_t stencil_attachment;
} metal_mock_render_pass_info_t;

typedef struct {
    const void* buffer;
    NSUInteger offset;
} metal_mock_buffer_binding_t;

// bindings and render state of the most recent render command encoder
typedef struct {
    bool ended;
    const void* pipeline_state;
    const void* depth_stencil_state;
    metal_mock_buffer_binding_t vertex_buffers[METAL_MOCK_MAX_BINDING_SLOTS];
    metal_mock_buffer_binding_t fragment_buffers[METAL_MOCK_MAX_BINDING_SLOTS];
    const void* vertex_textures[METAL_MOCK_MAX_BINDING_SLOTS];
    const void* fragment_textures[METAL_MOCK_MAX_BINDING_SLOTS];
    const void* vertex_samplers[METAL_MOCK_MAX_BINDING_SLOTS];
    const void* fragment_samplers[METAL_MOCK_MAX_BINDING_SLOTS];
    MTLViewport viewport;
    MTLScissorRect scissor_rect;
    MTLCullMode cull_mode;
    MTLWinding winding;
    float blend_color[4];
    uint32_t stencil_ref;
    float depth_bias, depth_bias_slope_scale, depth_bias_clamp;
    int debug_group_depth;
    char label[METAL_MOCK_MAX_STRING];
} metal_mock_render_encoder_state_t;

// bindings of the most recent compute command encoder
typedef struct {
    bool ended;
    const void* pipeline_state;
    metal_mock_buffer_binding_t buffers[METAL_MOCK_MAX_BINDING_SLOTS];
    const void* textures[METAL_MOCK_MAX_BINDING_SLOTS];
    const void* samplers[METAL_MOCK_MAX_BINDING_SLOTS];
    MTLSize last_threadgroups_per_grid;
    MTLSize last_threads_per_threadgroup;
    int num_dispatches;
    int debug_group_depth;
    char label[METAL_MOCK_MAX_STRING];
} metal_mock_compute_encoder_state_t;

#ifdef __cplusplus
extern "C" {
#endif

// setup and teardown
extern void metal_mock_setup(void);
extern void metal_mock_shutdown(void);

// the mock device, feed into sg_desc.environment.metal.device
extern const void* metal_mock_device(void);

// swapchain helpers, feed into sg_swapchain.metal.*, release when done
extern const void* metal_mock_create_drawable(int width, int height, MTLPixelFormat fmt);
extern const void* metal_mock_create_texture(int width, int height, MTLPixelFormat fmt, int sample_count);
// refcount helpers for objects which the test owns, for example the swapchain
// textures above
extern void metal_mock_retain(const void* obj);
extern void metal_mock_release(const void* obj);
// current refcount, to check that sokol-gfx balances its retains and releases
extern int metal_mock_retain_count(const void* obj);

// run the completion handlers of all committed command buffers
extern void metal_mock_complete_pending(void);
extern int metal_mock_num_pending_handlers(void);

// Drain and recreate the mock's autorelease pool. Command buffers, command
// encoders and pass descriptors are autoreleased, so their live counts only
// drop after this call. Only call it while no pass is active.
extern void metal_mock_drain_pool(void);

// object tracking
extern int metal_mock_live_objects(metal_mock_obj_t kind);
extern int metal_mock_live_objects_total(void);
extern int metal_mock_num_created(metal_mock_obj_t kind);
extern bool metal_mock_is_object(metal_mock_obj_t kind, const void* obj);
extern bool metal_mock_buffer_info(const void* obj, metal_mock_buffer_info_t* out);
// accepts both METAL_MOCK_OBJ_TEXTURE and METAL_MOCK_OBJ_TEXTURE_VIEW objects
extern bool metal_mock_texture_info(const void* obj, metal_mock_texture_info_t* out);
extern bool metal_mock_sampler_info(const void* obj, metal_mock_sampler_info_t* out);
extern bool metal_mock_library_info(const void* obj, metal_mock_library_info_t* out);
extern bool metal_mock_function_info(const void* obj, metal_mock_function_info_t* out);
extern bool metal_mock_render_pipeline_info(const void* obj, metal_mock_render_pipeline_info_t* out);
extern bool metal_mock_compute_pipeline_info(const void* obj, metal_mock_compute_pipeline_info_t* out);
extern bool metal_mock_depth_stencil_info(const void* obj, metal_mock_depth_stencil_info_t* out);

// pass and encoder state
extern const metal_mock_render_pass_info_t* metal_mock_last_render_pass(void);
extern const metal_mock_render_encoder_state_t* metal_mock_render_encoder_state(void);
extern const metal_mock_compute_encoder_state_t* metal_mock_compute_encoder_state(void);

// call log
extern int metal_mock_num_calls(void);
extern const metal_mock_call_t* metal_mock_call(int index);
extern int metal_mock_count_calls(metal_mock_func_t func);
extern int metal_mock_find_call(metal_mock_func_t func, int start_index);   // -1 if not found
extern const metal_mock_call_t* metal_mock_last_call(metal_mock_func_t func);  // 0 if not found
extern void metal_mock_clear_calls(void);
extern bool metal_mock_call_log_overflow(void);
extern const char* metal_mock_func_name(metal_mock_func_t func);

// fault injection
extern void metal_mock_fail_next(metal_mock_obj_t kind, int n);
// let the next 'n' creations fail, but only after 'skip' successful ones
extern void metal_mock_fail_next_after(metal_mock_obj_t kind, int skip, int n);
extern void metal_mock_set_error_message(const char* msg);
extern void metal_mock_set_supports_family(MTLGPUFamily family, bool supported);

#ifdef __cplusplus
} // extern "C"
#endif

#endif // METAL_MOCK_H_INCLUDED
