/*
    LLM maintained.

    wgpu_mock.h -- public API of the mocked WebGPU runtime.

    Include this header *before* sokol_gfx.h. The mock header
    tests/mocks/wgpu/webgpu/webgpu.h shadows the real <webgpu/webgpu.h>, so
    the sokol_gfx.h WebGPU backend can be built and unit-tested without a
    GPU, a driver or a window.

    The mock offers five inspection and control areas:

    - object tracking: live and created counts, refcounts and per-object
      state for buffers, textures, texture views, samplers, shader modules,
      bind group layouts, bind groups, pipeline layouts, render and compute
      pipelines, command encoders, command buffers and pass encoders
    - call log: every mocked WebGPU call with its arguments, in call order
    - pass and encoder state: the last render and compute pass descriptors
      and the state set on the current pass encoders
    - validation: the mock checks a subset of the WebGPU validation rules
      (alignments, usage flags, bounds, bind group compatibility, use of
      released objects) and records a message for every violation. Real
      WebGPU would raise a validation error in these cases, so a test run
      without recorded messages is a useful correctness check.
    - fault injection: forced creation failures plus configurable device
      features and limits

    Object handles are `const void*` so the values returned by
    sg_wgpu_query_*_info() can be passed in directly.

    Known limitations:

    - object memory is only freed in wgpu_mock_shutdown(), so a handle of a
      released object stays valid for inspection and never compares equal to
      a later object
    - there is one global render-pass-encoder state and one global
      compute-pass-encoder state. sokol-gfx has at most one active pass, so
      this is enough, but the state of a previous pass is lost when a new one
      of the same kind begins
    - buffer-to-buffer copies and queue writes are executed immediately on
      the mock buffer storage, not when the command buffer is submitted
    - textures only keep metadata, texel data is not stored
    - asynchronous APIs (mapping, error scopes, callbacks) are not mocked
      because sokol-gfx does not use them

    Not thread-safe -- this is intended for the sokol-gfx unit-test loop,
    which runs on a single thread.
*/
#ifndef WGPU_MOCK_H_INCLUDED
#define WGPU_MOCK_H_INCLUDED

#include <webgpu/webgpu.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define WGPU_MOCK_MAX_CALLS (8192)
#define WGPU_MOCK_MAX_CALL_ARGS (8)
#define WGPU_MOCK_MAX_STRING (128)
#define WGPU_MOCK_MAX_ERRORS (64)
#define WGPU_MOCK_MAX_ERROR_STRING (192)
#define WGPU_MOCK_MAX_BIND_ENTRIES (64)
#define WGPU_MOCK_MAX_BIND_GROUPS (4)
#define WGPU_MOCK_MAX_DYN_OFFSETS (16)
#define WGPU_MOCK_MAX_COLOR_ATTACHMENTS (8)
#define WGPU_MOCK_MAX_VERTEX_BUFFERS (16)
#define WGPU_MOCK_MAX_VERTEX_ATTRIBUTES (16)

// every mocked WebGPU function, as an x-macro list
#define _WGPUM_FUNCS \
    _WGPUM_XMACRO(wgpuDeviceHasFeature) \
    _WGPUM_XMACRO(wgpuDeviceGetLimits) \
    _WGPUM_XMACRO(wgpuDeviceGetQueue) \
    _WGPUM_XMACRO(wgpuDeviceCreateBuffer) \
    _WGPUM_XMACRO(wgpuDeviceCreateTexture) \
    _WGPUM_XMACRO(wgpuDeviceCreateSampler) \
    _WGPUM_XMACRO(wgpuDeviceCreateShaderModule) \
    _WGPUM_XMACRO(wgpuDeviceCreateBindGroupLayout) \
    _WGPUM_XMACRO(wgpuDeviceCreateBindGroup) \
    _WGPUM_XMACRO(wgpuDeviceCreatePipelineLayout) \
    _WGPUM_XMACRO(wgpuDeviceCreateRenderPipeline) \
    _WGPUM_XMACRO(wgpuDeviceCreateComputePipeline) \
    _WGPUM_XMACRO(wgpuDeviceCreateCommandEncoder) \
    _WGPUM_XMACRO(wgpuBufferAddRef) \
    _WGPUM_XMACRO(wgpuBufferRelease) \
    _WGPUM_XMACRO(wgpuBufferGetMappedRange) \
    _WGPUM_XMACRO(wgpuBufferUnmap) \
    _WGPUM_XMACRO(wgpuTextureAddRef) \
    _WGPUM_XMACRO(wgpuTextureRelease) \
    _WGPUM_XMACRO(wgpuTextureCreateView) \
    _WGPUM_XMACRO(wgpuTextureViewRelease) \
    _WGPUM_XMACRO(wgpuSamplerAddRef) \
    _WGPUM_XMACRO(wgpuSamplerRelease) \
    _WGPUM_XMACRO(wgpuShaderModuleRelease) \
    _WGPUM_XMACRO(wgpuBindGroupLayoutRelease) \
    _WGPUM_XMACRO(wgpuBindGroupRelease) \
    _WGPUM_XMACRO(wgpuPipelineLayoutRelease) \
    _WGPUM_XMACRO(wgpuRenderPipelineRelease) \
    _WGPUM_XMACRO(wgpuComputePipelineRelease) \
    _WGPUM_XMACRO(wgpuQueueRelease) \
    _WGPUM_XMACRO(wgpuQueueSubmit) \
    _WGPUM_XMACRO(wgpuQueueWriteBuffer) \
    _WGPUM_XMACRO(wgpuQueueWriteTexture) \
    _WGPUM_XMACRO(wgpuCommandEncoderBeginComputePass) \
    _WGPUM_XMACRO(wgpuCommandEncoderBeginRenderPass) \
    _WGPUM_XMACRO(wgpuCommandEncoderCopyBufferToBuffer) \
    _WGPUM_XMACRO(wgpuCommandEncoderCopyBufferToTexture) \
    _WGPUM_XMACRO(wgpuCommandEncoderCopyTextureToTexture) \
    _WGPUM_XMACRO(wgpuCommandEncoderFinish) \
    _WGPUM_XMACRO(wgpuCommandEncoderRelease) \
    _WGPUM_XMACRO(wgpuCommandBufferRelease) \
    _WGPUM_XMACRO(wgpuComputePassEncoderSetPipeline) \
    _WGPUM_XMACRO(wgpuComputePassEncoderSetBindGroup) \
    _WGPUM_XMACRO(wgpuComputePassEncoderDispatchWorkgroups) \
    _WGPUM_XMACRO(wgpuComputePassEncoderEnd) \
    _WGPUM_XMACRO(wgpuComputePassEncoderRelease) \
    _WGPUM_XMACRO(wgpuRenderPassEncoderSetPipeline) \
    _WGPUM_XMACRO(wgpuRenderPassEncoderSetBindGroup) \
    _WGPUM_XMACRO(wgpuRenderPassEncoderSetBlendConstant) \
    _WGPUM_XMACRO(wgpuRenderPassEncoderSetStencilReference) \
    _WGPUM_XMACRO(wgpuRenderPassEncoderSetViewport) \
    _WGPUM_XMACRO(wgpuRenderPassEncoderSetScissorRect) \
    _WGPUM_XMACRO(wgpuRenderPassEncoderSetIndexBuffer) \
    _WGPUM_XMACRO(wgpuRenderPassEncoderSetVertexBuffer) \
    _WGPUM_XMACRO(wgpuRenderPassEncoderDraw) \
    _WGPUM_XMACRO(wgpuRenderPassEncoderDrawIndexed) \
    _WGPUM_XMACRO(wgpuRenderPassEncoderEnd) \
    _WGPUM_XMACRO(wgpuRenderPassEncoderRelease)

typedef enum {
    WGPU_MOCK_FUNC_INVALID = 0,
    #define _WGPUM_XMACRO(name) WGPU_MOCK_FUNC_##name,
    _WGPUM_FUNCS
    #undef _WGPUM_XMACRO
    WGPU_MOCK_FUNC_NUM,
} wgpu_mock_func_t;

// tracked object kinds
typedef enum {
    WGPU_MOCK_OBJ_BUFFER = 0,
    WGPU_MOCK_OBJ_TEXTURE,
    WGPU_MOCK_OBJ_TEXTURE_VIEW,
    WGPU_MOCK_OBJ_SAMPLER,
    WGPU_MOCK_OBJ_SHADER_MODULE,
    WGPU_MOCK_OBJ_BIND_GROUP_LAYOUT,
    WGPU_MOCK_OBJ_BIND_GROUP,
    WGPU_MOCK_OBJ_PIPELINE_LAYOUT,
    WGPU_MOCK_OBJ_RENDER_PIPELINE,
    WGPU_MOCK_OBJ_COMPUTE_PIPELINE,
    WGPU_MOCK_OBJ_COMMAND_ENCODER,
    WGPU_MOCK_OBJ_COMMAND_BUFFER,
    WGPU_MOCK_OBJ_RENDER_PASS_ENCODER,
    WGPU_MOCK_OBJ_COMPUTE_PASS_ENCODER,
    // the device and the queue exist from setup to shutdown, they are not
    // part of wgpu_mock_live_objects_total()
    WGPU_MOCK_OBJ_QUEUE,
    WGPU_MOCK_OBJ_DEVICE,
    WGPU_MOCK_OBJ_NUM,
} wgpu_mock_obj_t;

/*
    One logged call argument, the reader must know the argument type.
    Logged arguments per function (not listed: 0 arguments, the receiver is
    always in call.obj):

    DeviceHasFeature            u[0]=feature, u[1]=result
    DeviceGetQueue              p[0]=queue
    DeviceCreate*               p[0]=created object (0 if creation failed)
    BufferGetMappedRange        u[0]=offset, u[1]=size, p[2]=result
    TextureCreateView           p[0]=created view (0 if creation failed)
    QueueSubmit                 u[0]=command buffer count
    QueueWriteBuffer            p[0]=buffer, u[1]=offset, u[2]=size
    QueueWriteTexture           p[0]=texture, u[1]=mip level, u[2]=data size
    CommandEncoderBegin*Pass    p[0]=pass encoder
    CopyBufferToBuffer          p[0]=src, u[1]=src offset, p[2]=dst, u[3]=dst offset, u[4]=size
    CopyBufferToTexture         p[0]=src buffer, p[1]=dst texture, u[2]=bytes per row, u[3]=mip level
    CopyTextureToTexture        p[0]=src texture, p[1]=dst texture
    CommandEncoderFinish        p[0]=command buffer
    *PassEncoderSetPipeline     p[0]=pipeline
    *PassEncoderSetBindGroup    u[0]=group index, p[1]=group, u[2]=dynamic offset count
    DispatchWorkgroups          u[0..2]=x,y,z
    SetBlendConstant            f[0..3]=r,g,b,a
    SetStencilReference         u[0]=reference
    SetViewport                 f[0..5]=x,y,w,h,min depth,max depth
    SetScissorRect              u[0..3]=x,y,w,h
    SetIndexBuffer              p[0]=buffer, u[1]=format, u[2]=offset, u[3]=size
    SetVertexBuffer             u[0]=slot, p[1]=buffer, u[2]=offset, u[3]=size
    Draw                        u[0..3]=vertex count, instance count, first vertex, first instance
    DrawIndexed                 u[0]=index count, u[1]=instance count, u[2]=first index,
                                i[3]=base vertex, u[4]=first instance
*/
typedef union {
    int64_t i;
    uint64_t u;
    double f;
    const void* p;
} wgpu_mock_arg_t;

typedef struct {
    wgpu_mock_func_t func;
    const void* obj;        // receiver of the call
    int num_args;
    wgpu_mock_arg_t args[WGPU_MOCK_MAX_CALL_ARGS];
} wgpu_mock_call_t;

// per-object state snapshots
typedef struct {
    uint64_t size;
    WGPUBufferUsage usage;
    bool mapped_at_creation;
    bool mapped;                    // currently mapped
    int num_queue_writes;
    int num_unmaps;
    int num_mapped_ranges;
    uint64_t last_write_offset;
    uint64_t last_write_size;
    char label[WGPU_MOCK_MAX_STRING];
} wgpu_mock_buffer_info_t;

typedef struct {
    WGPUTextureDimension dimension;
    uint32_t width, height, depth_or_layers;
    WGPUTextureFormat format;
    uint32_t mip_level_count;
    uint32_t sample_count;
    WGPUTextureUsage usage;
    int num_views;                  // views created from this texture
    int num_queue_writes;
    // last wgpuQueueWriteTexture() into this texture
    uint32_t last_write_mip_level;
    WGPUOrigin3D last_write_origin;
    WGPUExtent3D last_write_size;
    WGPUTexelCopyBufferLayout last_write_layout;
    size_t last_write_data_size;
    char label[WGPU_MOCK_MAX_STRING];
} wgpu_mock_texture_info_t;

typedef struct {
    const void* texture;
    WGPUTextureFormat format;
    WGPUTextureViewDimension dimension;
    uint32_t base_mip_level, mip_level_count;
    uint32_t base_array_layer, array_layer_count;
    WGPUTextureAspect aspect;
    char label[WGPU_MOCK_MAX_STRING];
} wgpu_mock_texture_view_info_t;

typedef struct {
    WGPUAddressMode address_u, address_v, address_w;
    WGPUFilterMode mag_filter, min_filter;
    WGPUMipmapFilterMode mipmap_filter;
    float lod_min_clamp, lod_max_clamp;
    WGPUCompareFunction compare;
    uint16_t max_anisotropy;
    char label[WGPU_MOCK_MAX_STRING];
} wgpu_mock_sampler_info_t;

typedef struct {
    const char* code;               // WGSL source, valid until wgpu_mock_shutdown()
    size_t code_length;
    char label[WGPU_MOCK_MAX_STRING];
} wgpu_mock_shader_module_info_t;

typedef struct {
    int num_entries;
    WGPUBindGroupLayoutEntry entries[WGPU_MOCK_MAX_BIND_ENTRIES];
    int num_dynamic_offsets;        // number of entries with hasDynamicOffset
    char label[WGPU_MOCK_MAX_STRING];
} wgpu_mock_bind_group_layout_info_t;

typedef struct {
    const void* layout;
    int num_entries;
    WGPUBindGroupEntry entries[WGPU_MOCK_MAX_BIND_ENTRIES];
    char label[WGPU_MOCK_MAX_STRING];
} wgpu_mock_bind_group_info_t;

typedef struct {
    int num_bind_group_layouts;
    const void* bind_group_layouts[WGPU_MOCK_MAX_BIND_GROUPS];
    char label[WGPU_MOCK_MAX_STRING];
} wgpu_mock_pipeline_layout_info_t;

typedef struct {
    WGPUVertexStepMode step_mode;
    uint64_t array_stride;
    int num_attributes;
    WGPUVertexAttribute attributes[WGPU_MOCK_MAX_VERTEX_ATTRIBUTES];
} wgpu_mock_vertex_buffer_layout_t;

typedef struct {
    WGPUTextureFormat format;
    bool has_blend;
    WGPUBlendState blend;
    WGPUColorWriteMask write_mask;
} wgpu_mock_color_target_t;

typedef struct {
    const void* layout;
    const void* vertex_module;
    char vertex_entry_point[WGPU_MOCK_MAX_STRING];
    int num_vertex_buffers;
    wgpu_mock_vertex_buffer_layout_t vertex_buffers[WGPU_MOCK_MAX_VERTEX_BUFFERS];
    WGPUPrimitiveState primitive;
    bool has_depth_stencil;
    WGPUDepthStencilState depth_stencil;
    WGPUMultisampleState multisample;
    bool has_fragment;
    const void* fragment_module;
    char fragment_entry_point[WGPU_MOCK_MAX_STRING];
    int num_targets;
    wgpu_mock_color_target_t targets[WGPU_MOCK_MAX_COLOR_ATTACHMENTS];
    char label[WGPU_MOCK_MAX_STRING];
} wgpu_mock_render_pipeline_info_t;

typedef struct {
    const void* layout;
    const void* module;
    char entry_point[WGPU_MOCK_MAX_STRING];
    char label[WGPU_MOCK_MAX_STRING];
} wgpu_mock_compute_pipeline_info_t;

// snapshot of one WGPURenderPassColorAttachment
typedef struct {
    const void* view;
    const void* resolve_target;
    uint32_t depth_slice;
    WGPULoadOp load_op;
    WGPUStoreOp store_op;
    WGPUColor clear_value;
} wgpu_mock_color_attachment_t;

// snapshot of the WGPURenderPassDescriptor of the most recent
// wgpuCommandEncoderBeginRenderPass() call
typedef struct {
    int num_color_attachments;
    wgpu_mock_color_attachment_t color_attachments[WGPU_MOCK_MAX_COLOR_ATTACHMENTS];
    bool has_depth_stencil;
    const void* ds_view;
    WGPULoadOp depth_load_op;
    WGPUStoreOp depth_store_op;
    float depth_clear_value;
    bool depth_read_only;
    WGPULoadOp stencil_load_op;
    WGPUStoreOp stencil_store_op;
    uint32_t stencil_clear_value;
    bool stencil_read_only;
    char label[WGPU_MOCK_MAX_STRING];
} wgpu_mock_render_pass_info_t;

// bindgroup slot as set by wgpuXxxPassEncoderSetBindGroup()
typedef struct {
    const void* group;              // 0 if the slot was cleared
    int num_dynamic_offsets;
    uint32_t dynamic_offsets[WGPU_MOCK_MAX_DYN_OFFSETS];
    int num_set_calls;              // SetBindGroup calls for this slot in this pass
} wgpu_mock_bind_slot_t;

typedef struct {
    const void* buffer;
    uint64_t offset;
    uint64_t size;
} wgpu_mock_buffer_binding_t;

// state of the most recent render pass encoder
typedef struct {
    bool ended;
    const void* pipeline;
    wgpu_mock_bind_slot_t bind_groups[WGPU_MOCK_MAX_BIND_GROUPS];
    wgpu_mock_buffer_binding_t vertex_buffers[WGPU_MOCK_MAX_VERTEX_BUFFERS];
    wgpu_mock_buffer_binding_t index_buffer;
    WGPUIndexFormat index_format;
    bool has_viewport;
    float viewport[6];              // x, y, w, h, min depth, max depth
    bool has_scissor_rect;
    uint32_t scissor_rect[4];       // x, y, w, h
    bool has_blend_constant;
    WGPUColor blend_constant;
    bool has_stencil_reference;
    uint32_t stencil_reference;
    int num_draws;
    int num_indexed_draws;
    // arguments of the last draw
    uint32_t last_vertex_or_index_count;
    uint32_t last_instance_count;
    uint32_t last_first_vertex_or_index;
    int32_t last_base_vertex;
    uint32_t last_first_instance;
} wgpu_mock_render_pass_encoder_state_t;

// state of the most recent compute pass encoder
typedef struct {
    bool ended;
    char label[WGPU_MOCK_MAX_STRING];
    const void* pipeline;
    wgpu_mock_bind_slot_t bind_groups[WGPU_MOCK_MAX_BIND_GROUPS];
    int num_dispatches;
    uint32_t last_dispatch[3];
} wgpu_mock_compute_pass_encoder_state_t;

// copy commands recorded on the most recent command encoder
typedef struct {
    bool finished;
    int num_render_passes;
    int num_compute_passes;
    int num_buffer_to_buffer_copies;
    int num_buffer_to_texture_copies;
    int num_texture_to_texture_copies;
    // last buffer-to-buffer copy
    const void* buffer_copy_src;
    const void* buffer_copy_dst;
    uint64_t buffer_copy_src_offset;
    uint64_t buffer_copy_dst_offset;
    uint64_t buffer_copy_size;
    // last buffer-to-texture copy
    const void* b2t_buffer;
    WGPUTexelCopyBufferLayout b2t_layout;
    const void* b2t_texture;
    uint32_t b2t_mip_level;
    WGPUOrigin3D b2t_origin;
    WGPUExtent3D b2t_size;
    // last texture-to-texture copy
    const void* t2t_src_texture;
    uint32_t t2t_src_mip_level;
    WGPUOrigin3D t2t_src_origin;
    const void* t2t_dst_texture;
    uint32_t t2t_dst_mip_level;
    WGPUOrigin3D t2t_dst_origin;
    WGPUExtent3D t2t_size;
} wgpu_mock_command_encoder_state_t;

typedef struct {
    int num_submits;
    int num_command_buffers;        // total, over all submits
    int num_write_buffers;
    int num_write_textures;
} wgpu_mock_queue_state_t;

#ifdef __cplusplus
extern "C" {
#endif

// setup and teardown, shutdown frees all objects and clears the state
extern void wgpu_mock_setup(void);
extern void wgpu_mock_shutdown(void);

// the mock device, feed into sg_desc.environment.wgpu.device
extern WGPUDevice wgpu_mock_device(void);
// the mock queue (not ref-counted for the test, see wgpu_mock_ref_count())
extern WGPUQueue wgpu_mock_queue(void);

// Swapchain helper: creates a 2D render-attachment texture and a view on it
// and returns the view. The texture is only owned by the view. Not logged.
// Release the returned view with wgpuTextureViewRelease() when done.
extern WGPUTextureView wgpu_mock_create_view(int width, int height, WGPUTextureFormat fmt, int sample_count);

// current refcount, to check that sokol-gfx balances its AddRef and Release calls (0 if dead)
extern int wgpu_mock_ref_count(const void* obj);

// object tracking
extern int wgpu_mock_live_objects(wgpu_mock_obj_t kind);
// all live objects except device and queue
extern int wgpu_mock_live_objects_total(void);
extern int wgpu_mock_num_created(wgpu_mock_obj_t kind);
// true if the handle is a known object of this kind and not yet destroyed
extern bool wgpu_mock_is_alive(wgpu_mock_obj_t kind, const void* obj);
extern bool wgpu_mock_buffer_info(const void* obj, wgpu_mock_buffer_info_t* out);
// CPU-side bytes of a mock buffer (not logged), 0 if not a live buffer
extern const void* wgpu_mock_buffer_data(const void* obj);
extern bool wgpu_mock_texture_info(const void* obj, wgpu_mock_texture_info_t* out);
extern bool wgpu_mock_texture_view_info(const void* obj, wgpu_mock_texture_view_info_t* out);
extern bool wgpu_mock_sampler_info(const void* obj, wgpu_mock_sampler_info_t* out);
extern bool wgpu_mock_shader_module_info(const void* obj, wgpu_mock_shader_module_info_t* out);
extern bool wgpu_mock_bind_group_layout_info(const void* obj, wgpu_mock_bind_group_layout_info_t* out);
extern bool wgpu_mock_bind_group_info(const void* obj, wgpu_mock_bind_group_info_t* out);
extern bool wgpu_mock_pipeline_layout_info(const void* obj, wgpu_mock_pipeline_layout_info_t* out);
extern bool wgpu_mock_render_pipeline_info(const void* obj, wgpu_mock_render_pipeline_info_t* out);
extern bool wgpu_mock_compute_pipeline_info(const void* obj, wgpu_mock_compute_pipeline_info_t* out);

// pass and encoder state
extern const wgpu_mock_render_pass_info_t* wgpu_mock_last_render_pass(void);
extern const wgpu_mock_render_pass_encoder_state_t* wgpu_mock_render_pass_encoder_state(void);
extern const wgpu_mock_compute_pass_encoder_state_t* wgpu_mock_compute_pass_encoder_state(void);
extern const wgpu_mock_command_encoder_state_t* wgpu_mock_command_encoder_state(void);
extern const wgpu_mock_queue_state_t* wgpu_mock_queue_state(void);

// call log
extern int wgpu_mock_num_calls(void);
extern const wgpu_mock_call_t* wgpu_mock_call(int index);
extern int wgpu_mock_count_calls(wgpu_mock_func_t func);
extern int wgpu_mock_find_call(wgpu_mock_func_t func, int start_index);   // -1 if not found
extern const wgpu_mock_call_t* wgpu_mock_last_call(wgpu_mock_func_t func);  // 0 if not found
extern void wgpu_mock_clear_calls(void);
extern bool wgpu_mock_call_log_overflow(void);
extern const char* wgpu_mock_func_name(wgpu_mock_func_t func);

// validation messages, see the header comment
extern int wgpu_mock_num_errors(void);
extern const char* wgpu_mock_error(int index);
// index of the first message containing 'substr', -1 if not found
extern int wgpu_mock_find_error(const char* substr);
extern void wgpu_mock_clear_errors(void);
// total number of messages including the ones which did not fit into the stored list
extern int wgpu_mock_total_errors(void);

// fault injection
// let the next 'n' creations of 'kind' fail (return 0)
extern void wgpu_mock_fail_next(wgpu_mock_obj_t kind, int n);
// let the next 'n' creations fail, but only after 'skip' successful ones
extern void wgpu_mock_fail_next_after(wgpu_mock_obj_t kind, int skip, int n);
// device features, by default only Depth32FloatStencil8 is enabled (sokol-gfx
// maps SG_PIXELFORMAT_DEPTH_STENCIL to it without a feature check)
extern void wgpu_mock_set_feature(WGPUFeatureName feature, bool supported);
// device limits, by default the WebGPU baseline limits
extern void wgpu_mock_set_limits(const WGPULimits* limits);
extern void wgpu_mock_get_default_limits(WGPULimits* out);

#ifdef __cplusplus
} // extern "C"
#endif

#endif // WGPU_MOCK_H_INCLUDED
