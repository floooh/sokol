/*
    LLM maintained.

    webgpu.h -- mock of the WebGPU C API subset used by sokol_gfx.h

    This header shadows the real <webgpu/webgpu.h> (Dawn / wgpu-native /
    emdawnwebgpu) when building the sokol_gfx.h WebGPU backend against the
    mock library in tests/mocks/wgpu. Only types, enums and functions used by
    sokol_gfx.h are declared. Struct layouts and enum values follow the
    current 'webgpu-headers' API (WGPUStringView, WGPUTexelCopy*, WGPUOptionalBool).

    The functions are implemented in wgpu_mock.c, see wgpu_mock.h for the
    inspection and fault-injection API.
*/
#ifndef WEBGPU_MOCK_H_INCLUDED
#define WEBGPU_MOCK_H_INCLUDED

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

#define WGPU_EXPORT
#define WGPU_NULLABLE
#define WGPU_ARRAY_LAYER_COUNT_UNDEFINED (UINT32_MAX)
#define WGPU_COPY_STRIDE_UNDEFINED (UINT32_MAX)
#define WGPU_DEPTH_SLICE_UNDEFINED (UINT32_MAX)
#define WGPU_LIMIT_U32_UNDEFINED (UINT32_MAX)
#define WGPU_MIP_LEVEL_COUNT_UNDEFINED (UINT32_MAX)
#define WGPU_STRLEN (SIZE_MAX)
#define WGPU_WHOLE_SIZE (UINT64_MAX)

typedef uint64_t WGPUFlags;
typedef uint32_t WGPUBool;

typedef struct WGPUStringView {
    const char* data;
    size_t length;
} WGPUStringView;

// opaque object handles
typedef struct WGPUBindGroupImpl* WGPUBindGroup;
typedef struct WGPUBindGroupLayoutImpl* WGPUBindGroupLayout;
typedef struct WGPUBufferImpl* WGPUBuffer;
typedef struct WGPUCommandBufferImpl* WGPUCommandBuffer;
typedef struct WGPUCommandEncoderImpl* WGPUCommandEncoder;
typedef struct WGPUComputePassEncoderImpl* WGPUComputePassEncoder;
typedef struct WGPUComputePipelineImpl* WGPUComputePipeline;
typedef struct WGPUDeviceImpl* WGPUDevice;
typedef struct WGPUPipelineLayoutImpl* WGPUPipelineLayout;
typedef struct WGPUQueueImpl* WGPUQueue;
typedef struct WGPURenderPassEncoderImpl* WGPURenderPassEncoder;
typedef struct WGPURenderPipelineImpl* WGPURenderPipeline;
typedef struct WGPUSamplerImpl* WGPUSampler;
typedef struct WGPUShaderModuleImpl* WGPUShaderModule;
typedef struct WGPUTextureImpl* WGPUTexture;
typedef struct WGPUTextureViewImpl* WGPUTextureView;
typedef struct WGPUQuerySetImpl* WGPUQuerySet;
typedef struct WGPUPassTimestampWrites WGPUPassTimestampWrites;
typedef struct WGPUConstantEntry WGPUConstantEntry;

// enums
typedef enum WGPUAddressMode {
    WGPUAddressMode_Undefined = 0x00000000,
    WGPUAddressMode_ClampToEdge = 0x00000001,
    WGPUAddressMode_Repeat = 0x00000002,
    WGPUAddressMode_MirrorRepeat = 0x00000003,
    WGPUAddressMode_Force32 = 0x7FFFFFFF,
} WGPUAddressMode;
typedef enum WGPUBlendFactor {
    WGPUBlendFactor_Undefined = 0x00000000,
    WGPUBlendFactor_Zero = 0x00000001,
    WGPUBlendFactor_One = 0x00000002,
    WGPUBlendFactor_Src = 0x00000003,
    WGPUBlendFactor_OneMinusSrc = 0x00000004,
    WGPUBlendFactor_SrcAlpha = 0x00000005,
    WGPUBlendFactor_OneMinusSrcAlpha = 0x00000006,
    WGPUBlendFactor_Dst = 0x00000007,
    WGPUBlendFactor_OneMinusDst = 0x00000008,
    WGPUBlendFactor_DstAlpha = 0x00000009,
    WGPUBlendFactor_OneMinusDstAlpha = 0x0000000A,
    WGPUBlendFactor_SrcAlphaSaturated = 0x0000000B,
    WGPUBlendFactor_Constant = 0x0000000C,
    WGPUBlendFactor_OneMinusConstant = 0x0000000D,
    WGPUBlendFactor_Src1 = 0x0000000E,
    WGPUBlendFactor_OneMinusSrc1 = 0x0000000F,
    WGPUBlendFactor_Src1Alpha = 0x00000010,
    WGPUBlendFactor_OneMinusSrc1Alpha = 0x00000011,
    WGPUBlendFactor_Force32 = 0x7FFFFFFF,
} WGPUBlendFactor;
typedef enum WGPUBlendOperation {
    WGPUBlendOperation_Undefined = 0x00000000,
    WGPUBlendOperation_Add = 0x00000001,
    WGPUBlendOperation_Subtract = 0x00000002,
    WGPUBlendOperation_ReverseSubtract = 0x00000003,
    WGPUBlendOperation_Min = 0x00000004,
    WGPUBlendOperation_Max = 0x00000005,
    WGPUBlendOperation_Force32 = 0x7FFFFFFF,
} WGPUBlendOperation;
typedef enum WGPUBufferBindingType {
    WGPUBufferBindingType_BindingNotUsed = 0x00000000,
    WGPUBufferBindingType_Undefined = 0x00000001,
    WGPUBufferBindingType_Uniform = 0x00000002,
    WGPUBufferBindingType_Storage = 0x00000003,
    WGPUBufferBindingType_ReadOnlyStorage = 0x00000004,
    WGPUBufferBindingType_Force32 = 0x7FFFFFFF,
} WGPUBufferBindingType;
typedef enum WGPUCompareFunction {
    WGPUCompareFunction_Undefined = 0x00000000,
    WGPUCompareFunction_Never = 0x00000001,
    WGPUCompareFunction_Less = 0x00000002,
    WGPUCompareFunction_Equal = 0x00000003,
    WGPUCompareFunction_LessEqual = 0x00000004,
    WGPUCompareFunction_Greater = 0x00000005,
    WGPUCompareFunction_NotEqual = 0x00000006,
    WGPUCompareFunction_GreaterEqual = 0x00000007,
    WGPUCompareFunction_Always = 0x00000008,
    WGPUCompareFunction_Force32 = 0x7FFFFFFF,
} WGPUCompareFunction;
typedef enum WGPUCullMode {
    WGPUCullMode_Undefined = 0x00000000,
    WGPUCullMode_None = 0x00000001,
    WGPUCullMode_Front = 0x00000002,
    WGPUCullMode_Back = 0x00000003,
    WGPUCullMode_Force32 = 0x7FFFFFFF,
} WGPUCullMode;
typedef enum WGPUFeatureName {
    WGPUFeatureName_CoreFeaturesAndLimits = 0x00000001,
    WGPUFeatureName_DepthClipControl = 0x00000002,
    WGPUFeatureName_Depth32FloatStencil8 = 0x00000003,
    WGPUFeatureName_TextureCompressionBC = 0x00000004,
    WGPUFeatureName_TextureCompressionBCSliced3D = 0x00000005,
    WGPUFeatureName_TextureCompressionETC2 = 0x00000006,
    WGPUFeatureName_TextureCompressionASTC = 0x00000007,
    WGPUFeatureName_TextureCompressionASTCSliced3D = 0x00000008,
    WGPUFeatureName_TimestampQuery = 0x00000009,
    WGPUFeatureName_IndirectFirstInstance = 0x0000000A,
    WGPUFeatureName_ShaderF16 = 0x0000000B,
    WGPUFeatureName_RG11B10UfloatRenderable = 0x0000000C,
    WGPUFeatureName_BGRA8UnormStorage = 0x0000000D,
    WGPUFeatureName_Float32Filterable = 0x0000000E,
    WGPUFeatureName_Float32Blendable = 0x0000000F,
    WGPUFeatureName_ClipDistances = 0x00000010,
    WGPUFeatureName_DualSourceBlending = 0x00000011,
    WGPUFeatureName_Subgroups = 0x00000012,
    WGPUFeatureName_TextureFormatsTier1 = 0x00000013,
    WGPUFeatureName_TextureFormatsTier2 = 0x00000014,
    WGPUFeatureName_PrimitiveIndex = 0x00000015,
    WGPUFeatureName_TextureComponentSwizzle = 0x00000016,
    WGPUFeatureName_Unorm16TextureFormats = 0x0005000C,
    WGPUFeatureName_MultiDrawIndirect = 0x00050031,
    WGPUFeatureName_Force32 = 0x7FFFFFFF,
} WGPUFeatureName;
typedef enum WGPUFilterMode {
    WGPUFilterMode_Undefined = 0x00000000,
    WGPUFilterMode_Nearest = 0x00000001,
    WGPUFilterMode_Linear = 0x00000002,
    WGPUFilterMode_Force32 = 0x7FFFFFFF,
} WGPUFilterMode;
typedef enum WGPUFrontFace {
    WGPUFrontFace_Undefined = 0x00000000,
    WGPUFrontFace_CCW = 0x00000001,
    WGPUFrontFace_CW = 0x00000002,
    WGPUFrontFace_Force32 = 0x7FFFFFFF,
} WGPUFrontFace;
typedef enum WGPUIndexFormat {
    WGPUIndexFormat_Undefined = 0x00000000,
    WGPUIndexFormat_Uint16 = 0x00000001,
    WGPUIndexFormat_Uint32 = 0x00000002,
    WGPUIndexFormat_Force32 = 0x7FFFFFFF,
} WGPUIndexFormat;
typedef enum WGPULoadOp {
    WGPULoadOp_Undefined = 0x00000000,
    WGPULoadOp_Load = 0x00000001,
    WGPULoadOp_Clear = 0x00000002,
    WGPULoadOp_Force32 = 0x7FFFFFFF,
} WGPULoadOp;
typedef enum WGPUMipmapFilterMode {
    WGPUMipmapFilterMode_Undefined = 0x00000000,
    WGPUMipmapFilterMode_Nearest = 0x00000001,
    WGPUMipmapFilterMode_Linear = 0x00000002,
    WGPUMipmapFilterMode_Force32 = 0x7FFFFFFF,
} WGPUMipmapFilterMode;
typedef enum WGPUOptionalBool {
    WGPUOptionalBool_False = 0x00000000,
    WGPUOptionalBool_True = 0x00000001,
    WGPUOptionalBool_Undefined = 0x00000002,
    WGPUOptionalBool_Force32 = 0x7FFFFFFF,
} WGPUOptionalBool;
typedef enum WGPUPrimitiveTopology {
    WGPUPrimitiveTopology_Undefined = 0x00000000,
    WGPUPrimitiveTopology_PointList = 0x00000001,
    WGPUPrimitiveTopology_LineList = 0x00000002,
    WGPUPrimitiveTopology_LineStrip = 0x00000003,
    WGPUPrimitiveTopology_TriangleList = 0x00000004,
    WGPUPrimitiveTopology_TriangleStrip = 0x00000005,
    WGPUPrimitiveTopology_Force32 = 0x7FFFFFFF,
} WGPUPrimitiveTopology;
typedef enum WGPUSType {
    WGPUSType_ShaderSourceSPIRV = 0x00000001,
    WGPUSType_ShaderSourceWGSL = 0x00000002,
    WGPUSType_RenderPassMaxDrawCount = 0x00000003,
    WGPUSType_SurfaceSourceMetalLayer = 0x00000004,
    WGPUSType_SurfaceSourceWindowsHWND = 0x00000005,
    WGPUSType_SurfaceSourceXlibWindow = 0x00000006,
    WGPUSType_SurfaceSourceWaylandSurface = 0x00000007,
    WGPUSType_SurfaceSourceAndroidNativeWindow = 0x00000008,
    WGPUSType_SurfaceSourceXCBWindow = 0x00000009,
    WGPUSType_SurfaceColorManagement = 0x0000000A,
    WGPUSType_RequestAdapterWebXROptions = 0x0000000B,
    WGPUSType_TextureComponentSwizzleDescriptor = 0x0000000C,
    WGPUSType_ExternalTextureBindingLayout = 0x0000000D,
    WGPUSType_ExternalTextureBindingEntry = 0x0000000E,
    WGPUSType_CompatibilityModeLimits = 0x0000000F,
    WGPUSType_TextureBindingViewDimension = 0x00000010,
    WGPUSType_EmscriptenSurfaceSourceCanvasHTMLSelector = 0x00040000,
    WGPUSType_DawnCompilationMessageUtf16 = 0x0005003F,
    WGPUSType_Force32 = 0x7FFFFFFF,
} WGPUSType;
typedef enum WGPUSamplerBindingType {
    WGPUSamplerBindingType_BindingNotUsed = 0x00000000,
    WGPUSamplerBindingType_Undefined = 0x00000001,
    WGPUSamplerBindingType_Filtering = 0x00000002,
    WGPUSamplerBindingType_NonFiltering = 0x00000003,
    WGPUSamplerBindingType_Comparison = 0x00000004,
    WGPUSamplerBindingType_Force32 = 0x7FFFFFFF,
} WGPUSamplerBindingType;
typedef enum WGPUStencilOperation {
    WGPUStencilOperation_Undefined = 0x00000000,
    WGPUStencilOperation_Keep = 0x00000001,
    WGPUStencilOperation_Zero = 0x00000002,
    WGPUStencilOperation_Replace = 0x00000003,
    WGPUStencilOperation_Invert = 0x00000004,
    WGPUStencilOperation_IncrementClamp = 0x00000005,
    WGPUStencilOperation_DecrementClamp = 0x00000006,
    WGPUStencilOperation_IncrementWrap = 0x00000007,
    WGPUStencilOperation_DecrementWrap = 0x00000008,
    WGPUStencilOperation_Force32 = 0x7FFFFFFF,
} WGPUStencilOperation;
typedef enum WGPUStorageTextureAccess {
    WGPUStorageTextureAccess_BindingNotUsed = 0x00000000,
    WGPUStorageTextureAccess_Undefined = 0x00000001,
    WGPUStorageTextureAccess_WriteOnly = 0x00000002,
    WGPUStorageTextureAccess_ReadOnly = 0x00000003,
    WGPUStorageTextureAccess_ReadWrite = 0x00000004,
    WGPUStorageTextureAccess_Force32 = 0x7FFFFFFF,
} WGPUStorageTextureAccess;
typedef enum WGPUStoreOp {
    WGPUStoreOp_Undefined = 0x00000000,
    WGPUStoreOp_Store = 0x00000001,
    WGPUStoreOp_Discard = 0x00000002,
    WGPUStoreOp_Force32 = 0x7FFFFFFF,
} WGPUStoreOp;
typedef enum WGPUTextureAspect {
    WGPUTextureAspect_Undefined = 0x00000000,
    WGPUTextureAspect_All = 0x00000001,
    WGPUTextureAspect_StencilOnly = 0x00000002,
    WGPUTextureAspect_DepthOnly = 0x00000003,
    WGPUTextureAspect_Force32 = 0x7FFFFFFF,
} WGPUTextureAspect;
typedef enum WGPUTextureDimension {
    WGPUTextureDimension_Undefined = 0x00000000,
    WGPUTextureDimension_1D = 0x00000001,
    WGPUTextureDimension_2D = 0x00000002,
    WGPUTextureDimension_3D = 0x00000003,
    WGPUTextureDimension_Force32 = 0x7FFFFFFF,
} WGPUTextureDimension;
typedef enum WGPUTextureSampleType {
    WGPUTextureSampleType_BindingNotUsed = 0x00000000,
    WGPUTextureSampleType_Undefined = 0x00000001,
    WGPUTextureSampleType_Float = 0x00000002,
    WGPUTextureSampleType_UnfilterableFloat = 0x00000003,
    WGPUTextureSampleType_Depth = 0x00000004,
    WGPUTextureSampleType_Sint = 0x00000005,
    WGPUTextureSampleType_Uint = 0x00000006,
    WGPUTextureSampleType_Force32 = 0x7FFFFFFF,
} WGPUTextureSampleType;
typedef enum WGPUTextureViewDimension {
    WGPUTextureViewDimension_Undefined = 0x00000000,
    WGPUTextureViewDimension_1D = 0x00000001,
    WGPUTextureViewDimension_2D = 0x00000002,
    WGPUTextureViewDimension_2DArray = 0x00000003,
    WGPUTextureViewDimension_Cube = 0x00000004,
    WGPUTextureViewDimension_CubeArray = 0x00000005,
    WGPUTextureViewDimension_3D = 0x00000006,
    WGPUTextureViewDimension_Force32 = 0x7FFFFFFF,
} WGPUTextureViewDimension;
typedef enum WGPUVertexStepMode {
    WGPUVertexStepMode_Undefined = 0x00000000,
    WGPUVertexStepMode_Vertex = 0x00000001,
    WGPUVertexStepMode_Instance = 0x00000002,
    WGPUVertexStepMode_Force32 = 0x7FFFFFFF,
} WGPUVertexStepMode;

typedef enum WGPUTextureFormat {
    WGPUTextureFormat_Undefined = 0x00000000,
    WGPUTextureFormat_R8Unorm = 0x00000001,
    WGPUTextureFormat_R8Snorm = 0x00000002,
    WGPUTextureFormat_R8Uint = 0x00000003,
    WGPUTextureFormat_R8Sint = 0x00000004,
    WGPUTextureFormat_R16Unorm = 0x00000005,
    WGPUTextureFormat_R16Snorm = 0x00000006,
    WGPUTextureFormat_R16Uint = 0x00000007,
    WGPUTextureFormat_R16Sint = 0x00000008,
    WGPUTextureFormat_R16Float = 0x00000009,
    WGPUTextureFormat_RG8Unorm = 0x0000000A,
    WGPUTextureFormat_RG8Snorm = 0x0000000B,
    WGPUTextureFormat_RG8Uint = 0x0000000C,
    WGPUTextureFormat_RG8Sint = 0x0000000D,
    WGPUTextureFormat_R32Float = 0x0000000E,
    WGPUTextureFormat_R32Uint = 0x0000000F,
    WGPUTextureFormat_R32Sint = 0x00000010,
    WGPUTextureFormat_RG16Unorm = 0x00000011,
    WGPUTextureFormat_RG16Snorm = 0x00000012,
    WGPUTextureFormat_RG16Uint = 0x00000013,
    WGPUTextureFormat_RG16Sint = 0x00000014,
    WGPUTextureFormat_RG16Float = 0x00000015,
    WGPUTextureFormat_RGBA8Unorm = 0x00000016,
    WGPUTextureFormat_RGBA8UnormSrgb = 0x00000017,
    WGPUTextureFormat_RGBA8Snorm = 0x00000018,
    WGPUTextureFormat_RGBA8Uint = 0x00000019,
    WGPUTextureFormat_RGBA8Sint = 0x0000001A,
    WGPUTextureFormat_BGRA8Unorm = 0x0000001B,
    WGPUTextureFormat_BGRA8UnormSrgb = 0x0000001C,
    WGPUTextureFormat_RGB10A2Unorm = 0x0000001E,
    WGPUTextureFormat_RG11B10Ufloat = 0x0000001F,
    WGPUTextureFormat_RGB9E5Ufloat = 0x00000020,
    WGPUTextureFormat_RG32Float = 0x00000021,
    WGPUTextureFormat_RG32Uint = 0x00000022,
    WGPUTextureFormat_RG32Sint = 0x00000023,
    WGPUTextureFormat_RGBA16Unorm = 0x00000024,
    WGPUTextureFormat_RGBA16Snorm = 0x00000025,
    WGPUTextureFormat_RGBA16Uint = 0x00000026,
    WGPUTextureFormat_RGBA16Sint = 0x00000027,
    WGPUTextureFormat_RGBA16Float = 0x00000028,
    WGPUTextureFormat_RGBA32Float = 0x00000029,
    WGPUTextureFormat_RGBA32Uint = 0x0000002A,
    WGPUTextureFormat_RGBA32Sint = 0x0000002B,
    WGPUTextureFormat_Depth32Float = 0x00000030,
    WGPUTextureFormat_Depth32FloatStencil8 = 0x00000031,
    WGPUTextureFormat_BC1RGBAUnorm = 0x00000032,
    WGPUTextureFormat_BC2RGBAUnorm = 0x00000034,
    WGPUTextureFormat_BC3RGBAUnorm = 0x00000036,
    WGPUTextureFormat_BC3RGBAUnormSrgb = 0x00000037,
    WGPUTextureFormat_BC4RUnorm = 0x00000038,
    WGPUTextureFormat_BC4RSnorm = 0x00000039,
    WGPUTextureFormat_BC5RGUnorm = 0x0000003A,
    WGPUTextureFormat_BC5RGSnorm = 0x0000003B,
    WGPUTextureFormat_BC6HRGBUfloat = 0x0000003C,
    WGPUTextureFormat_BC6HRGBFloat = 0x0000003D,
    WGPUTextureFormat_BC7RGBAUnorm = 0x0000003E,
    WGPUTextureFormat_BC7RGBAUnormSrgb = 0x0000003F,
    WGPUTextureFormat_ETC2RGB8Unorm = 0x00000040,
    WGPUTextureFormat_ETC2RGB8UnormSrgb = 0x00000041,
    WGPUTextureFormat_ETC2RGB8A1Unorm = 0x00000042,
    WGPUTextureFormat_ETC2RGBA8Unorm = 0x00000044,
    WGPUTextureFormat_ETC2RGBA8UnormSrgb = 0x00000045,
    WGPUTextureFormat_EACR11Unorm = 0x00000046,
    WGPUTextureFormat_EACR11Snorm = 0x00000047,
    WGPUTextureFormat_EACRG11Unorm = 0x00000048,
    WGPUTextureFormat_EACRG11Snorm = 0x00000049,
    WGPUTextureFormat_ASTC4x4Unorm = 0x0000004A,
    WGPUTextureFormat_ASTC4x4UnormSrgb = 0x0000004B,
    WGPUTextureFormat_Force32 = 0x7FFFFFFF,
} WGPUTextureFormat;
typedef enum WGPUVertexFormat {
    WGPUVertexFormat_Uint8x4 = 0x00000003,
    WGPUVertexFormat_Sint8x4 = 0x00000006,
    WGPUVertexFormat_Unorm8x4 = 0x00000009,
    WGPUVertexFormat_Snorm8x4 = 0x0000000C,
    WGPUVertexFormat_Uint16x2 = 0x0000000E,
    WGPUVertexFormat_Uint16x4 = 0x0000000F,
    WGPUVertexFormat_Sint16x2 = 0x00000011,
    WGPUVertexFormat_Sint16x4 = 0x00000012,
    WGPUVertexFormat_Unorm16x2 = 0x00000014,
    WGPUVertexFormat_Unorm16x4 = 0x00000015,
    WGPUVertexFormat_Snorm16x2 = 0x00000017,
    WGPUVertexFormat_Snorm16x4 = 0x00000018,
    WGPUVertexFormat_Float16x2 = 0x0000001A,
    WGPUVertexFormat_Float16x4 = 0x0000001B,
    WGPUVertexFormat_Float32 = 0x0000001C,
    WGPUVertexFormat_Float32x2 = 0x0000001D,
    WGPUVertexFormat_Float32x3 = 0x0000001E,
    WGPUVertexFormat_Float32x4 = 0x0000001F,
    WGPUVertexFormat_Uint32 = 0x00000020,
    WGPUVertexFormat_Uint32x2 = 0x00000021,
    WGPUVertexFormat_Uint32x3 = 0x00000022,
    WGPUVertexFormat_Uint32x4 = 0x00000023,
    WGPUVertexFormat_Sint32 = 0x00000024,
    WGPUVertexFormat_Sint32x2 = 0x00000025,
    WGPUVertexFormat_Sint32x3 = 0x00000026,
    WGPUVertexFormat_Sint32x4 = 0x00000027,
    WGPUVertexFormat_Unorm10_10_10_2 = 0x00000028,
    WGPUVertexFormat_Force32 = 0x7FFFFFFF,
} WGPUVertexFormat;

// bit flags
typedef WGPUFlags WGPUBufferUsage;
static const WGPUBufferUsage WGPUBufferUsage_None = 0x0000000000000000;
static const WGPUBufferUsage WGPUBufferUsage_MapRead = 0x0000000000000001;
static const WGPUBufferUsage WGPUBufferUsage_MapWrite = 0x0000000000000002;
static const WGPUBufferUsage WGPUBufferUsage_CopySrc = 0x0000000000000004;
static const WGPUBufferUsage WGPUBufferUsage_CopyDst = 0x0000000000000008;
static const WGPUBufferUsage WGPUBufferUsage_Index = 0x0000000000000010;
static const WGPUBufferUsage WGPUBufferUsage_Vertex = 0x0000000000000020;
static const WGPUBufferUsage WGPUBufferUsage_Uniform = 0x0000000000000040;
static const WGPUBufferUsage WGPUBufferUsage_Storage = 0x0000000000000080;

typedef WGPUFlags WGPUColorWriteMask;
static const WGPUColorWriteMask WGPUColorWriteMask_None = 0x0000000000000000;
static const WGPUColorWriteMask WGPUColorWriteMask_Red = 0x0000000000000001;
static const WGPUColorWriteMask WGPUColorWriteMask_Green = 0x0000000000000002;
static const WGPUColorWriteMask WGPUColorWriteMask_Blue = 0x0000000000000004;
static const WGPUColorWriteMask WGPUColorWriteMask_Alpha = 0x0000000000000008;

typedef WGPUFlags WGPUShaderStage;
static const WGPUShaderStage WGPUShaderStage_None = 0x0000000000000000;
static const WGPUShaderStage WGPUShaderStage_Vertex = 0x0000000000000001;
static const WGPUShaderStage WGPUShaderStage_Fragment = 0x0000000000000002;
static const WGPUShaderStage WGPUShaderStage_Compute = 0x0000000000000004;

typedef WGPUFlags WGPUTextureUsage;
static const WGPUTextureUsage WGPUTextureUsage_None = 0x0000000000000000;
static const WGPUTextureUsage WGPUTextureUsage_CopySrc = 0x0000000000000001;
static const WGPUTextureUsage WGPUTextureUsage_CopyDst = 0x0000000000000002;
static const WGPUTextureUsage WGPUTextureUsage_TextureBinding = 0x0000000000000004;
static const WGPUTextureUsage WGPUTextureUsage_StorageBinding = 0x0000000000000008;
static const WGPUTextureUsage WGPUTextureUsage_RenderAttachment = 0x0000000000000010;

// structs
typedef struct WGPUChainedStruct {
    struct WGPUChainedStruct* next;
    WGPUSType sType;
} WGPUChainedStruct;

typedef struct WGPUColor { double r, g, b, a; } WGPUColor;
typedef struct WGPUExtent3D { uint32_t width, height, depthOrArrayLayers; } WGPUExtent3D;
typedef struct WGPUOrigin3D { uint32_t x, y, z; } WGPUOrigin3D;

typedef struct WGPULimits {
    WGPUChainedStruct* nextInChain;
    uint32_t maxTextureDimension1D;
    uint32_t maxTextureDimension2D;
    uint32_t maxTextureDimension3D;
    uint32_t maxTextureArrayLayers;
    uint32_t maxBindGroups;
    uint32_t maxBindGroupsPlusVertexBuffers;
    uint32_t maxBindingsPerBindGroup;
    uint32_t maxDynamicUniformBuffersPerPipelineLayout;
    uint32_t maxDynamicStorageBuffersPerPipelineLayout;
    uint32_t maxSampledTexturesPerShaderStage;
    uint32_t maxSamplersPerShaderStage;
    uint32_t maxStorageBuffersPerShaderStage;
    uint32_t maxStorageTexturesPerShaderStage;
    uint32_t maxUniformBuffersPerShaderStage;
    uint64_t maxUniformBufferBindingSize;
    uint64_t maxStorageBufferBindingSize;
    uint32_t minUniformBufferOffsetAlignment;
    uint32_t minStorageBufferOffsetAlignment;
    uint32_t maxVertexBuffers;
    uint64_t maxBufferSize;
    uint32_t maxVertexAttributes;
    uint32_t maxVertexBufferArrayStride;
    uint32_t maxInterStageShaderVariables;
    uint32_t maxColorAttachments;
    uint32_t maxColorAttachmentBytesPerSample;
    uint32_t maxComputeWorkgroupStorageSize;
    uint32_t maxComputeInvocationsPerWorkgroup;
    uint32_t maxComputeWorkgroupSizeX;
    uint32_t maxComputeWorkgroupSizeY;
    uint32_t maxComputeWorkgroupSizeZ;
    uint32_t maxComputeWorkgroupsPerDimension;
    uint32_t maxImmediateSize;
} WGPULimits;

typedef int WGPUStatus;

typedef struct WGPUBufferDescriptor {
    WGPUChainedStruct* nextInChain;
    WGPUStringView label;
    WGPUBufferUsage usage;
    uint64_t size;
    WGPUBool mappedAtCreation;
} WGPUBufferDescriptor;

typedef struct WGPUTextureDescriptor {
    WGPUChainedStruct* nextInChain;
    WGPUStringView label;
    WGPUTextureUsage usage;
    WGPUTextureDimension dimension;
    WGPUExtent3D size;
    WGPUTextureFormat format;
    uint32_t mipLevelCount;
    uint32_t sampleCount;
    size_t viewFormatCount;
    const WGPUTextureFormat* viewFormats;
} WGPUTextureDescriptor;

typedef struct WGPUTextureViewDescriptor {
    WGPUChainedStruct* nextInChain;
    WGPUStringView label;
    WGPUTextureFormat format;
    WGPUTextureViewDimension dimension;
    uint32_t baseMipLevel;
    uint32_t mipLevelCount;
    uint32_t baseArrayLayer;
    uint32_t arrayLayerCount;
    WGPUTextureAspect aspect;
    WGPUTextureUsage usage;
} WGPUTextureViewDescriptor;

typedef struct WGPUSamplerDescriptor {
    WGPUChainedStruct* nextInChain;
    WGPUStringView label;
    WGPUAddressMode addressModeU;
    WGPUAddressMode addressModeV;
    WGPUAddressMode addressModeW;
    WGPUFilterMode magFilter;
    WGPUFilterMode minFilter;
    WGPUMipmapFilterMode mipmapFilter;
    float lodMinClamp;
    float lodMaxClamp;
    WGPUCompareFunction compare;
    uint16_t maxAnisotropy;
} WGPUSamplerDescriptor;

typedef struct WGPUShaderModuleDescriptor {
    WGPUChainedStruct* nextInChain;
    WGPUStringView label;
} WGPUShaderModuleDescriptor;

typedef struct WGPUShaderSourceWGSL {
    WGPUChainedStruct chain;
    WGPUStringView code;
} WGPUShaderSourceWGSL;

typedef struct WGPUBufferBindingLayout {
    WGPUChainedStruct* nextInChain;
    WGPUBufferBindingType type;
    WGPUBool hasDynamicOffset;
    uint64_t minBindingSize;
} WGPUBufferBindingLayout;

typedef struct WGPUSamplerBindingLayout {
    WGPUChainedStruct* nextInChain;
    WGPUSamplerBindingType type;
} WGPUSamplerBindingLayout;

typedef struct WGPUTextureBindingLayout {
    WGPUChainedStruct* nextInChain;
    WGPUTextureSampleType sampleType;
    WGPUTextureViewDimension viewDimension;
    WGPUBool multisampled;
} WGPUTextureBindingLayout;

typedef struct WGPUStorageTextureBindingLayout {
    WGPUChainedStruct* nextInChain;
    WGPUStorageTextureAccess access;
    WGPUTextureFormat format;
    WGPUTextureViewDimension viewDimension;
} WGPUStorageTextureBindingLayout;

typedef struct WGPUBindGroupLayoutEntry {
    WGPUChainedStruct* nextInChain;
    uint32_t binding;
    WGPUShaderStage visibility;
    uint32_t bindingArraySize;
    WGPUBufferBindingLayout buffer;
    WGPUSamplerBindingLayout sampler;
    WGPUTextureBindingLayout texture;
    WGPUStorageTextureBindingLayout storageTexture;
} WGPUBindGroupLayoutEntry;

typedef struct WGPUBindGroupLayoutDescriptor {
    WGPUChainedStruct* nextInChain;
    WGPUStringView label;
    size_t entryCount;
    const WGPUBindGroupLayoutEntry* entries;
} WGPUBindGroupLayoutDescriptor;

typedef struct WGPUBindGroupEntry {
    WGPUChainedStruct* nextInChain;
    uint32_t binding;
    WGPUBuffer buffer;
    uint64_t offset;
    uint64_t size;
    WGPUSampler sampler;
    WGPUTextureView textureView;
} WGPUBindGroupEntry;

typedef struct WGPUBindGroupDescriptor {
    WGPUChainedStruct* nextInChain;
    WGPUStringView label;
    WGPUBindGroupLayout layout;
    size_t entryCount;
    const WGPUBindGroupEntry* entries;
} WGPUBindGroupDescriptor;

typedef struct WGPUPipelineLayoutDescriptor {
    WGPUChainedStruct* nextInChain;
    WGPUStringView label;
    size_t bindGroupLayoutCount;
    const WGPUBindGroupLayout* bindGroupLayouts;
    uint32_t immediateSize;
} WGPUPipelineLayoutDescriptor;

typedef struct WGPUBlendComponent {
    WGPUBlendOperation operation;
    WGPUBlendFactor srcFactor;
    WGPUBlendFactor dstFactor;
} WGPUBlendComponent;

typedef struct WGPUBlendState {
    WGPUBlendComponent color;
    WGPUBlendComponent alpha;
} WGPUBlendState;

typedef struct WGPUColorTargetState {
    WGPUChainedStruct* nextInChain;
    WGPUTextureFormat format;
    const WGPUBlendState* blend;
    WGPUColorWriteMask writeMask;
} WGPUColorTargetState;

typedef struct WGPUStencilFaceState {
    WGPUCompareFunction compare;
    WGPUStencilOperation failOp;
    WGPUStencilOperation depthFailOp;
    WGPUStencilOperation passOp;
} WGPUStencilFaceState;

typedef struct WGPUDepthStencilState {
    WGPUChainedStruct* nextInChain;
    WGPUTextureFormat format;
    WGPUOptionalBool depthWriteEnabled;
    WGPUCompareFunction depthCompare;
    WGPUStencilFaceState stencilFront;
    WGPUStencilFaceState stencilBack;
    uint32_t stencilReadMask;
    uint32_t stencilWriteMask;
    int32_t depthBias;
    float depthBiasSlopeScale;
    float depthBiasClamp;
} WGPUDepthStencilState;

typedef struct WGPUFragmentState {
    WGPUChainedStruct* nextInChain;
    WGPUShaderModule module;
    WGPUStringView entryPoint;
    size_t constantCount;
    const WGPUConstantEntry* constants;
    size_t targetCount;
    const WGPUColorTargetState* targets;
} WGPUFragmentState;

typedef struct WGPUPrimitiveState {
    WGPUChainedStruct* nextInChain;
    WGPUPrimitiveTopology topology;
    WGPUIndexFormat stripIndexFormat;
    WGPUFrontFace frontFace;
    WGPUCullMode cullMode;
    WGPUBool unclippedDepth;
} WGPUPrimitiveState;

typedef struct WGPUMultisampleState {
    WGPUChainedStruct* nextInChain;
    uint32_t count;
    uint32_t mask;
    WGPUBool alphaToCoverageEnabled;
} WGPUMultisampleState;

typedef struct WGPUVertexAttribute {
    WGPUChainedStruct* nextInChain;
    WGPUVertexFormat format;
    uint64_t offset;
    uint32_t shaderLocation;
} WGPUVertexAttribute;

typedef struct WGPUVertexBufferLayout {
    WGPUChainedStruct* nextInChain;
    WGPUVertexStepMode stepMode;
    uint64_t arrayStride;
    size_t attributeCount;
    const WGPUVertexAttribute* attributes;
} WGPUVertexBufferLayout;

typedef struct WGPUVertexState {
    WGPUChainedStruct* nextInChain;
    WGPUShaderModule module;
    WGPUStringView entryPoint;
    size_t constantCount;
    const WGPUConstantEntry* constants;
    size_t bufferCount;
    const WGPUVertexBufferLayout* buffers;
} WGPUVertexState;

typedef struct WGPURenderPipelineDescriptor {
    WGPUChainedStruct* nextInChain;
    WGPUStringView label;
    WGPUPipelineLayout layout;
    WGPUVertexState vertex;
    WGPUPrimitiveState primitive;
    const WGPUDepthStencilState* depthStencil;
    WGPUMultisampleState multisample;
    const WGPUFragmentState* fragment;
} WGPURenderPipelineDescriptor;

typedef struct WGPUComputeState {
    WGPUChainedStruct* nextInChain;
    WGPUShaderModule module;
    WGPUStringView entryPoint;
    size_t constantCount;
    const WGPUConstantEntry* constants;
} WGPUComputeState;

typedef struct WGPUComputePipelineDescriptor {
    WGPUChainedStruct* nextInChain;
    WGPUStringView label;
    WGPUPipelineLayout layout;
    WGPUComputeState compute;
} WGPUComputePipelineDescriptor;

typedef struct WGPUCommandEncoderDescriptor {
    WGPUChainedStruct* nextInChain;
    WGPUStringView label;
} WGPUCommandEncoderDescriptor;

typedef struct WGPUCommandBufferDescriptor {
    WGPUChainedStruct* nextInChain;
    WGPUStringView label;
} WGPUCommandBufferDescriptor;

typedef struct WGPUComputePassDescriptor {
    WGPUChainedStruct* nextInChain;
    WGPUStringView label;
    const WGPUPassTimestampWrites* timestampWrites;
} WGPUComputePassDescriptor;

typedef struct WGPURenderPassColorAttachment {
    WGPUChainedStruct* nextInChain;
    WGPUTextureView view;
    uint32_t depthSlice;
    WGPUTextureView resolveTarget;
    WGPULoadOp loadOp;
    WGPUStoreOp storeOp;
    WGPUColor clearValue;
} WGPURenderPassColorAttachment;

typedef struct WGPURenderPassDepthStencilAttachment {
    WGPUChainedStruct* nextInChain;
    WGPUTextureView view;
    WGPULoadOp depthLoadOp;
    WGPUStoreOp depthStoreOp;
    float depthClearValue;
    WGPUBool depthReadOnly;
    WGPULoadOp stencilLoadOp;
    WGPUStoreOp stencilStoreOp;
    uint32_t stencilClearValue;
    WGPUBool stencilReadOnly;
} WGPURenderPassDepthStencilAttachment;

typedef struct WGPURenderPassDescriptor {
    WGPUChainedStruct* nextInChain;
    WGPUStringView label;
    size_t colorAttachmentCount;
    const WGPURenderPassColorAttachment* colorAttachments;
    const WGPURenderPassDepthStencilAttachment* depthStencilAttachment;
    WGPUQuerySet occlusionQuerySet;
    const WGPUPassTimestampWrites* timestampWrites;
} WGPURenderPassDescriptor;

typedef struct WGPUTexelCopyBufferLayout {
    uint64_t offset;
    uint32_t bytesPerRow;
    uint32_t rowsPerImage;
} WGPUTexelCopyBufferLayout;

typedef struct WGPUTexelCopyBufferInfo {
    WGPUTexelCopyBufferLayout layout;
    WGPUBuffer buffer;
} WGPUTexelCopyBufferInfo;

typedef struct WGPUTexelCopyTextureInfo {
    WGPUTexture texture;
    uint32_t mipLevel;
    WGPUOrigin3D origin;
    WGPUTextureAspect aspect;
} WGPUTexelCopyTextureInfo;

// functions
WGPUBool wgpuDeviceHasFeature(WGPUDevice device, WGPUFeatureName feature);
WGPUStatus wgpuDeviceGetLimits(WGPUDevice device, WGPULimits* limits);
WGPUQueue wgpuDeviceGetQueue(WGPUDevice device);
WGPUBuffer wgpuDeviceCreateBuffer(WGPUDevice device, const WGPUBufferDescriptor* desc);
WGPUTexture wgpuDeviceCreateTexture(WGPUDevice device, const WGPUTextureDescriptor* desc);
WGPUSampler wgpuDeviceCreateSampler(WGPUDevice device, const WGPUSamplerDescriptor* desc);
WGPUShaderModule wgpuDeviceCreateShaderModule(WGPUDevice device, const WGPUShaderModuleDescriptor* desc);
WGPUBindGroupLayout wgpuDeviceCreateBindGroupLayout(WGPUDevice device, const WGPUBindGroupLayoutDescriptor* desc);
WGPUBindGroup wgpuDeviceCreateBindGroup(WGPUDevice device, const WGPUBindGroupDescriptor* desc);
WGPUPipelineLayout wgpuDeviceCreatePipelineLayout(WGPUDevice device, const WGPUPipelineLayoutDescriptor* desc);
WGPURenderPipeline wgpuDeviceCreateRenderPipeline(WGPUDevice device, const WGPURenderPipelineDescriptor* desc);
WGPUComputePipeline wgpuDeviceCreateComputePipeline(WGPUDevice device, const WGPUComputePipelineDescriptor* desc);
WGPUCommandEncoder wgpuDeviceCreateCommandEncoder(WGPUDevice device, const WGPUCommandEncoderDescriptor* desc);

void wgpuBufferAddRef(WGPUBuffer buffer);
void wgpuBufferRelease(WGPUBuffer buffer);
void* wgpuBufferGetMappedRange(WGPUBuffer buffer, size_t offset, size_t size);
void wgpuBufferUnmap(WGPUBuffer buffer);
void wgpuTextureAddRef(WGPUTexture texture);
void wgpuTextureRelease(WGPUTexture texture);
WGPUTextureView wgpuTextureCreateView(WGPUTexture texture, const WGPUTextureViewDescriptor* desc);
void wgpuTextureViewRelease(WGPUTextureView view);
void wgpuSamplerAddRef(WGPUSampler sampler);
void wgpuSamplerRelease(WGPUSampler sampler);
void wgpuShaderModuleRelease(WGPUShaderModule module);
void wgpuBindGroupLayoutRelease(WGPUBindGroupLayout layout);
void wgpuBindGroupRelease(WGPUBindGroup group);
void wgpuPipelineLayoutRelease(WGPUPipelineLayout layout);
void wgpuRenderPipelineRelease(WGPURenderPipeline pip);
void wgpuComputePipelineRelease(WGPUComputePipeline pip);
void wgpuQueueRelease(WGPUQueue queue);
void wgpuQueueSubmit(WGPUQueue queue, size_t command_count, const WGPUCommandBuffer* commands);
void wgpuQueueWriteBuffer(WGPUQueue queue, WGPUBuffer buffer, uint64_t buffer_offset, const void* data, size_t size);
void wgpuQueueWriteTexture(WGPUQueue queue, const WGPUTexelCopyTextureInfo* dst, const void* data, size_t data_size, const WGPUTexelCopyBufferLayout* layout, const WGPUExtent3D* write_size);

WGPUComputePassEncoder wgpuCommandEncoderBeginComputePass(WGPUCommandEncoder enc, const WGPUComputePassDescriptor* desc);
WGPURenderPassEncoder wgpuCommandEncoderBeginRenderPass(WGPUCommandEncoder enc, const WGPURenderPassDescriptor* desc);
void wgpuCommandEncoderCopyBufferToBuffer(WGPUCommandEncoder enc, WGPUBuffer src, uint64_t src_offset, WGPUBuffer dst, uint64_t dst_offset, uint64_t size);
void wgpuCommandEncoderCopyBufferToTexture(WGPUCommandEncoder enc, const WGPUTexelCopyBufferInfo* src, const WGPUTexelCopyTextureInfo* dst, const WGPUExtent3D* size);
void wgpuCommandEncoderCopyTextureToTexture(WGPUCommandEncoder enc, const WGPUTexelCopyTextureInfo* src, const WGPUTexelCopyTextureInfo* dst, const WGPUExtent3D* size);
WGPUCommandBuffer wgpuCommandEncoderFinish(WGPUCommandEncoder enc, const WGPUCommandBufferDescriptor* desc);
void wgpuCommandEncoderRelease(WGPUCommandEncoder enc);
void wgpuCommandBufferRelease(WGPUCommandBuffer cmd_buf);

void wgpuComputePassEncoderSetPipeline(WGPUComputePassEncoder enc, WGPUComputePipeline pip);
void wgpuComputePassEncoderSetBindGroup(WGPUComputePassEncoder enc, uint32_t group_index, WGPUBindGroup group, size_t num_dyn_offsets, const uint32_t* dyn_offsets);
void wgpuComputePassEncoderDispatchWorkgroups(WGPUComputePassEncoder enc, uint32_t x, uint32_t y, uint32_t z);
void wgpuComputePassEncoderEnd(WGPUComputePassEncoder enc);
void wgpuComputePassEncoderRelease(WGPUComputePassEncoder enc);

void wgpuRenderPassEncoderSetPipeline(WGPURenderPassEncoder enc, WGPURenderPipeline pip);
void wgpuRenderPassEncoderSetBindGroup(WGPURenderPassEncoder enc, uint32_t group_index, WGPUBindGroup group, size_t num_dyn_offsets, const uint32_t* dyn_offsets);
void wgpuRenderPassEncoderSetBlendConstant(WGPURenderPassEncoder enc, const WGPUColor* color);
void wgpuRenderPassEncoderSetStencilReference(WGPURenderPassEncoder enc, uint32_t ref);
void wgpuRenderPassEncoderSetViewport(WGPURenderPassEncoder enc, float x, float y, float w, float h, float min_depth, float max_depth);
void wgpuRenderPassEncoderSetScissorRect(WGPURenderPassEncoder enc, uint32_t x, uint32_t y, uint32_t w, uint32_t h);
void wgpuRenderPassEncoderSetIndexBuffer(WGPURenderPassEncoder enc, WGPUBuffer buf, WGPUIndexFormat fmt, uint64_t offset, uint64_t size);
void wgpuRenderPassEncoderSetVertexBuffer(WGPURenderPassEncoder enc, uint32_t slot, WGPUBuffer buf, uint64_t offset, uint64_t size);
void wgpuRenderPassEncoderDraw(WGPURenderPassEncoder enc, uint32_t vertex_count, uint32_t instance_count, uint32_t first_vertex, uint32_t first_instance);
void wgpuRenderPassEncoderDrawIndexed(WGPURenderPassEncoder enc, uint32_t index_count, uint32_t instance_count, uint32_t first_index, int32_t base_vertex, uint32_t first_instance);
void wgpuRenderPassEncoderEnd(WGPURenderPassEncoder enc);
void wgpuRenderPassEncoderRelease(WGPURenderPassEncoder enc);

#ifdef __cplusplus
} // extern "C"
#endif

#endif // WEBGPU_MOCK_H_INCLUDED
