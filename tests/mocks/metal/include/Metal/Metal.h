/*
    LLM maintained.

    Metal/Metal.h -- mock Metal API.

    Shadows the real Metal.framework header. Declares only what the
    sokol_gfx.h Metal backend uses. Enum values and property types match the
    real SDK, so sokol_gfx.h compiles with the same warnings as against the
    real framework.

    Foundation is the real system framework, only Metal itself is mocked.
    The mock classes which implement the protocols live in metal_mock.m.

    One deviation from the SDK: sub-descriptor properties are 'strong' instead
    of 'copy', because the mock descriptor classes do not implement NSCopying.
    sokol_gfx.h assigns a fresh sub-descriptor and then mutates it in place,
    which behaves the same either way.
*/
#ifndef METAL_MOCK_METAL_H_INCLUDED
#define METAL_MOCK_METAL_H_INCLUDED

#import <Foundation/Foundation.h>

// Rename the mock descriptor classes. Foundation loads the real
// Metal.framework at runtime, and identical ObjC class names in both the
// test executable and the framework make the ObjC runtime print duplicate
// class warnings.
#define MTLTextureDescriptor _mtlm_MTLTextureDescriptor
#define MTLSamplerDescriptor _mtlm_MTLSamplerDescriptor
#define MTLCompileOptions _mtlm_MTLCompileOptions
#define MTLVertexBufferLayoutDescriptor _mtlm_MTLVertexBufferLayoutDescriptor
#define MTLVertexBufferLayoutDescriptorArray _mtlm_MTLVertexBufferLayoutDescriptorArray
#define MTLVertexAttributeDescriptor _mtlm_MTLVertexAttributeDescriptor
#define MTLVertexAttributeDescriptorArray _mtlm_MTLVertexAttributeDescriptorArray
#define MTLVertexDescriptor _mtlm_MTLVertexDescriptor
#define MTLPipelineBufferDescriptor _mtlm_MTLPipelineBufferDescriptor
#define MTLPipelineBufferDescriptorArray _mtlm_MTLPipelineBufferDescriptorArray
#define MTLRenderPipelineColorAttachmentDescriptor _mtlm_MTLRenderPipelineColorAttachmentDescriptor
#define MTLRenderPipelineColorAttachmentDescriptorArray _mtlm_MTLRenderPipelineColorAttachmentDescriptorArray
#define MTLRenderPipelineDescriptor _mtlm_MTLRenderPipelineDescriptor
#define MTLComputePipelineDescriptor _mtlm_MTLComputePipelineDescriptor
#define MTLStencilDescriptor _mtlm_MTLStencilDescriptor
#define MTLDepthStencilDescriptor _mtlm_MTLDepthStencilDescriptor
#define MTLRenderPassAttachmentDescriptor _mtlm_MTLRenderPassAttachmentDescriptor
#define MTLRenderPassColorAttachmentDescriptor _mtlm_MTLRenderPassColorAttachmentDescriptor
#define MTLRenderPassDepthAttachmentDescriptor _mtlm_MTLRenderPassDepthAttachmentDescriptor
#define MTLRenderPassStencilAttachmentDescriptor _mtlm_MTLRenderPassStencilAttachmentDescriptor
#define MTLRenderPassColorAttachmentDescriptorArray _mtlm_MTLRenderPassColorAttachmentDescriptorArray
#define MTLRenderPassDescriptor _mtlm_MTLRenderPassDescriptor

//== MTLTypes.h ================================================================

typedef struct {
    NSUInteger x, y, z;
} MTLOrigin;

static inline MTLOrigin MTLOriginMake(NSUInteger x, NSUInteger y, NSUInteger z) {
    MTLOrigin o; o.x = x; o.y = y; o.z = z; return o;
}

typedef struct {
    NSUInteger width, height, depth;
} MTLSize;

static inline MTLSize MTLSizeMake(NSUInteger width, NSUInteger height, NSUInteger depth) {
    MTLSize s; s.width = width; s.height = height; s.depth = depth; return s;
}

typedef struct {
    MTLOrigin origin;
    MTLSize size;
} MTLRegion;

static inline MTLRegion MTLRegionMake2D(NSUInteger x, NSUInteger y, NSUInteger width, NSUInteger height) {
    MTLRegion r;
    r.origin.x = x; r.origin.y = y; r.origin.z = 0;
    r.size.width = width; r.size.height = height; r.size.depth = 1;
    return r;
}

static inline MTLRegion MTLRegionMake3D(NSUInteger x, NSUInteger y, NSUInteger z, NSUInteger width, NSUInteger height, NSUInteger depth) {
    MTLRegion r;
    r.origin.x = x; r.origin.y = y; r.origin.z = z;
    r.size.width = width; r.size.height = height; r.size.depth = depth;
    return r;
}

//== MTLPixelFormat.h ==========================================================

typedef NS_ENUM(NSUInteger, MTLPixelFormat) {
    MTLPixelFormatInvalid = 0,

    MTLPixelFormatA8Unorm = 1,

    MTLPixelFormatR8Unorm = 10,
    MTLPixelFormatR8Snorm = 12,
    MTLPixelFormatR8Uint = 13,
    MTLPixelFormatR8Sint = 14,

    MTLPixelFormatR16Unorm = 20,
    MTLPixelFormatR16Snorm = 22,
    MTLPixelFormatR16Uint = 23,
    MTLPixelFormatR16Sint = 24,
    MTLPixelFormatR16Float = 25,

    MTLPixelFormatRG8Unorm = 30,
    MTLPixelFormatRG8Snorm = 32,
    MTLPixelFormatRG8Uint = 33,
    MTLPixelFormatRG8Sint = 34,

    MTLPixelFormatR32Uint = 53,
    MTLPixelFormatR32Sint = 54,
    MTLPixelFormatR32Float = 55,

    MTLPixelFormatRG16Unorm = 60,
    MTLPixelFormatRG16Snorm = 62,
    MTLPixelFormatRG16Uint = 63,
    MTLPixelFormatRG16Sint = 64,
    MTLPixelFormatRG16Float = 65,

    MTLPixelFormatRGBA8Unorm = 70,
    MTLPixelFormatRGBA8Unorm_sRGB = 71,
    MTLPixelFormatRGBA8Snorm = 72,
    MTLPixelFormatRGBA8Uint = 73,
    MTLPixelFormatRGBA8Sint = 74,

    MTLPixelFormatBGRA8Unorm = 80,
    MTLPixelFormatBGRA8Unorm_sRGB = 81,

    MTLPixelFormatRGB10A2Unorm = 90,
    MTLPixelFormatRG11B10Float = 92,
    MTLPixelFormatRGB9E5Float = 93,

    MTLPixelFormatRG32Uint = 103,
    MTLPixelFormatRG32Sint = 104,
    MTLPixelFormatRG32Float = 105,

    MTLPixelFormatRGBA16Unorm = 110,
    MTLPixelFormatRGBA16Snorm = 112,
    MTLPixelFormatRGBA16Uint = 113,
    MTLPixelFormatRGBA16Sint = 114,
    MTLPixelFormatRGBA16Float = 115,

    MTLPixelFormatRGBA32Uint = 123,
    MTLPixelFormatRGBA32Sint = 124,
    MTLPixelFormatRGBA32Float = 125,

    MTLPixelFormatBC1_RGBA = 130,
    MTLPixelFormatBC1_RGBA_sRGB = 131,
    MTLPixelFormatBC2_RGBA = 132,
    MTLPixelFormatBC2_RGBA_sRGB = 133,
    MTLPixelFormatBC3_RGBA = 134,
    MTLPixelFormatBC3_RGBA_sRGB = 135,
    MTLPixelFormatBC4_RUnorm = 140,
    MTLPixelFormatBC4_RSnorm = 141,
    MTLPixelFormatBC5_RGUnorm = 142,
    MTLPixelFormatBC5_RGSnorm = 143,
    MTLPixelFormatBC6H_RGBFloat = 150,
    MTLPixelFormatBC6H_RGBUfloat = 151,
    MTLPixelFormatBC7_RGBAUnorm = 152,
    MTLPixelFormatBC7_RGBAUnorm_sRGB = 153,

    MTLPixelFormatEAC_R11Unorm = 170,
    MTLPixelFormatEAC_R11Snorm = 172,
    MTLPixelFormatEAC_RG11Unorm = 174,
    MTLPixelFormatEAC_RG11Snorm = 176,
    MTLPixelFormatEAC_RGBA8 = 178,
    MTLPixelFormatEAC_RGBA8_sRGB = 179,

    MTLPixelFormatETC2_RGB8 = 180,
    MTLPixelFormatETC2_RGB8_sRGB = 181,
    MTLPixelFormatETC2_RGB8A1 = 182,
    MTLPixelFormatETC2_RGB8A1_sRGB = 183,

    MTLPixelFormatASTC_4x4_sRGB = 186,
    MTLPixelFormatASTC_4x4_LDR = 204,

    MTLPixelFormatDepth32Float = 252,
    MTLPixelFormatDepth32Float_Stencil8 = 260,
};

//== MTLResource.h =============================================================

typedef NS_ENUM(NSUInteger, MTLCPUCacheMode) {
    MTLCPUCacheModeDefaultCache = 0,
    MTLCPUCacheModeWriteCombined = 1,
};

typedef NS_ENUM(NSUInteger, MTLStorageMode) {
    MTLStorageModeShared = 0,
    MTLStorageModeManaged = 1,
    MTLStorageModePrivate = 2,
    MTLStorageModeMemoryless = 3,
};

#define MTLResourceCPUCacheModeShift (0)
#define MTLResourceStorageModeShift (4)

typedef NS_OPTIONS(NSUInteger, MTLResourceOptions) {
    MTLResourceCPUCacheModeDefaultCache = MTLCPUCacheModeDefaultCache << MTLResourceCPUCacheModeShift,
    MTLResourceCPUCacheModeWriteCombined = MTLCPUCacheModeWriteCombined << MTLResourceCPUCacheModeShift,
    MTLResourceStorageModeShared = MTLStorageModeShared << MTLResourceStorageModeShift,
    MTLResourceStorageModeManaged = MTLStorageModeManaged << MTLResourceStorageModeShift,
    MTLResourceStorageModePrivate = MTLStorageModePrivate << MTLResourceStorageModeShift,
};

@protocol MTLDevice;

@protocol MTLResource <NSObject>
@property (nullable, copy, atomic) NSString* label;
@end

//== MTLBuffer.h ===============================================================

@protocol MTLBuffer <MTLResource>
- (void* _Nonnull)contents;
- (void)didModifyRange:(NSRange)range;
@end

//== MTLTexture.h ==============================================================

typedef NS_ENUM(NSUInteger, MTLTextureType) {
    MTLTextureType1D = 0,
    MTLTextureType1DArray = 1,
    MTLTextureType2D = 2,
    MTLTextureType2DArray = 3,
    MTLTextureType2DMultisample = 4,
    MTLTextureTypeCube = 5,
    MTLTextureTypeCubeArray = 6,
    MTLTextureType3D = 7,
    MTLTextureType2DMultisampleArray = 8,
    MTLTextureTypeTextureBuffer = 9,
};

typedef NS_OPTIONS(NSUInteger, MTLTextureUsage) {
    MTLTextureUsageUnknown = 0x0000,
    MTLTextureUsageShaderRead = 0x0001,
    MTLTextureUsageShaderWrite = 0x0002,
    MTLTextureUsageRenderTarget = 0x0004,
};

@interface MTLTextureDescriptor : NSObject
@property (readwrite, nonatomic) MTLTextureType textureType;
@property (readwrite, nonatomic) MTLPixelFormat pixelFormat;
@property (readwrite, nonatomic) NSUInteger width;
@property (readwrite, nonatomic) NSUInteger height;
@property (readwrite, nonatomic) NSUInteger depth;
@property (readwrite, nonatomic) NSUInteger mipmapLevelCount;
@property (readwrite, nonatomic) NSUInteger sampleCount;
@property (readwrite, nonatomic) NSUInteger arrayLength;
@property (readwrite, nonatomic) MTLResourceOptions resourceOptions;
@property (readwrite, nonatomic) MTLTextureUsage usage;
@property (nullable, copy, nonatomic) NSString* label;
@end

@protocol MTLTexture <MTLResource>
@property (readonly) MTLTextureType textureType;
@property (readonly) MTLPixelFormat pixelFormat;
@property (readonly) NSUInteger width;
@property (readonly) NSUInteger height;
@property (readonly) NSUInteger depth;
@property (readonly) NSUInteger mipmapLevelCount;
@property (readonly) NSUInteger sampleCount;
@property (readonly) NSUInteger arrayLength;
@property (readonly) MTLTextureUsage usage;
- (void)replaceRegion:(MTLRegion)region
          mipmapLevel:(NSUInteger)level
                slice:(NSUInteger)slice
            withBytes:(const void* _Nonnull)pixelBytes
          bytesPerRow:(NSUInteger)bytesPerRow
        bytesPerImage:(NSUInteger)bytesPerImage;
- (id<MTLTexture> _Nullable)newTextureViewWithPixelFormat:(MTLPixelFormat)pixelFormat
                                              textureType:(MTLTextureType)textureType
                                                   levels:(NSRange)levelRange
                                                   slices:(NSRange)sliceRange;
@end

//== MTLSampler.h ==============================================================

typedef NS_ENUM(NSUInteger, MTLSamplerMinMagFilter) {
    MTLSamplerMinMagFilterNearest = 0,
    MTLSamplerMinMagFilterLinear = 1,
};

typedef NS_ENUM(NSUInteger, MTLSamplerMipFilter) {
    MTLSamplerMipFilterNotMipmapped = 0,
    MTLSamplerMipFilterNearest = 1,
    MTLSamplerMipFilterLinear = 2,
};

typedef NS_ENUM(NSUInteger, MTLSamplerAddressMode) {
    MTLSamplerAddressModeClampToEdge = 0,
    MTLSamplerAddressModeMirrorClampToEdge = 1,
    MTLSamplerAddressModeRepeat = 2,
    MTLSamplerAddressModeMirrorRepeat = 3,
    MTLSamplerAddressModeClampToZero = 4,
    MTLSamplerAddressModeClampToBorderColor = 5,
};

typedef NS_ENUM(NSUInteger, MTLSamplerBorderColor) {
    MTLSamplerBorderColorTransparentBlack = 0,
    MTLSamplerBorderColorOpaqueBlack = 1,
    MTLSamplerBorderColorOpaqueWhite = 2,
};

typedef NS_ENUM(NSUInteger, MTLCompareFunction) {
    MTLCompareFunctionNever = 0,
    MTLCompareFunctionLess = 1,
    MTLCompareFunctionEqual = 2,
    MTLCompareFunctionLessEqual = 3,
    MTLCompareFunctionGreater = 4,
    MTLCompareFunctionNotEqual = 5,
    MTLCompareFunctionGreaterEqual = 6,
    MTLCompareFunctionAlways = 7,
};

@interface MTLSamplerDescriptor : NSObject
@property (nonatomic) MTLSamplerMinMagFilter minFilter;
@property (nonatomic) MTLSamplerMinMagFilter magFilter;
@property (nonatomic) MTLSamplerMipFilter mipFilter;
@property (nonatomic) NSUInteger maxAnisotropy;
@property (nonatomic) MTLSamplerAddressMode sAddressMode;
@property (nonatomic) MTLSamplerAddressMode tAddressMode;
@property (nonatomic) MTLSamplerAddressMode rAddressMode;
@property (nonatomic) MTLSamplerBorderColor borderColor;
@property (nonatomic) BOOL normalizedCoordinates;
@property (nonatomic) float lodMinClamp;
@property (nonatomic) float lodMaxClamp;
@property (nonatomic) MTLCompareFunction compareFunction;
@property (nullable, copy, nonatomic) NSString* label;
@end

@protocol MTLSamplerState <NSObject>
@end

//== MTLLibrary.h ==============================================================

@protocol MTLFunction <NSObject>
@end

@interface MTLCompileOptions : NSObject
@end

@protocol MTLLibrary <NSObject>
@property (nullable, copy, atomic) NSString* label;
- (id<MTLFunction> _Nullable)newFunctionWithName:(NSString* _Nonnull)functionName;
@end

//== MTLVertexDescriptor.h =====================================================

typedef NS_ENUM(NSUInteger, MTLVertexFormat) {
    MTLVertexFormatInvalid = 0,
    MTLVertexFormatUChar2 = 1,
    MTLVertexFormatUChar3 = 2,
    MTLVertexFormatUChar4 = 3,
    MTLVertexFormatChar2 = 4,
    MTLVertexFormatChar3 = 5,
    MTLVertexFormatChar4 = 6,
    MTLVertexFormatUChar2Normalized = 7,
    MTLVertexFormatUChar3Normalized = 8,
    MTLVertexFormatUChar4Normalized = 9,
    MTLVertexFormatChar2Normalized = 10,
    MTLVertexFormatChar3Normalized = 11,
    MTLVertexFormatChar4Normalized = 12,
    MTLVertexFormatUShort2 = 13,
    MTLVertexFormatUShort3 = 14,
    MTLVertexFormatUShort4 = 15,
    MTLVertexFormatShort2 = 16,
    MTLVertexFormatShort3 = 17,
    MTLVertexFormatShort4 = 18,
    MTLVertexFormatUShort2Normalized = 19,
    MTLVertexFormatUShort3Normalized = 20,
    MTLVertexFormatUShort4Normalized = 21,
    MTLVertexFormatShort2Normalized = 22,
    MTLVertexFormatShort3Normalized = 23,
    MTLVertexFormatShort4Normalized = 24,
    MTLVertexFormatHalf2 = 25,
    MTLVertexFormatHalf3 = 26,
    MTLVertexFormatHalf4 = 27,
    MTLVertexFormatFloat = 28,
    MTLVertexFormatFloat2 = 29,
    MTLVertexFormatFloat3 = 30,
    MTLVertexFormatFloat4 = 31,
    MTLVertexFormatInt = 32,
    MTLVertexFormatInt2 = 33,
    MTLVertexFormatInt3 = 34,
    MTLVertexFormatInt4 = 35,
    MTLVertexFormatUInt = 36,
    MTLVertexFormatUInt2 = 37,
    MTLVertexFormatUInt3 = 38,
    MTLVertexFormatUInt4 = 39,
    MTLVertexFormatInt1010102Normalized = 40,
    MTLVertexFormatUInt1010102Normalized = 41,
};

typedef NS_ENUM(NSUInteger, MTLVertexStepFunction) {
    MTLVertexStepFunctionConstant = 0,
    MTLVertexStepFunctionPerVertex = 1,
    MTLVertexStepFunctionPerInstance = 2,
    MTLVertexStepFunctionPerPatch = 3,
    MTLVertexStepFunctionPerPatchControlPoint = 4,
};

@interface MTLVertexBufferLayoutDescriptor : NSObject
@property (assign, nonatomic) NSUInteger stride;
@property (assign, nonatomic) MTLVertexStepFunction stepFunction;
@property (assign, nonatomic) NSUInteger stepRate;
@end

@interface MTLVertexBufferLayoutDescriptorArray : NSObject
- (MTLVertexBufferLayoutDescriptor* _Nonnull)objectAtIndexedSubscript:(NSUInteger)index;
@end

@interface MTLVertexAttributeDescriptor : NSObject
@property (assign, nonatomic) MTLVertexFormat format;
@property (assign, nonatomic) NSUInteger offset;
@property (assign, nonatomic) NSUInteger bufferIndex;
@end

@interface MTLVertexAttributeDescriptorArray : NSObject
- (MTLVertexAttributeDescriptor* _Nonnull)objectAtIndexedSubscript:(NSUInteger)index;
@end

@interface MTLVertexDescriptor : NSObject
+ (MTLVertexDescriptor* _Nonnull)vertexDescriptor;
@property (readonly) MTLVertexBufferLayoutDescriptorArray* _Nonnull layouts;
@property (readonly) MTLVertexAttributeDescriptorArray* _Nonnull attributes;
@end

//== MTLPipeline.h =============================================================

typedef NS_ENUM(NSUInteger, MTLMutability) {
    MTLMutabilityDefault = 0,
    MTLMutabilityMutable = 1,
    MTLMutabilityImmutable = 2,
};

typedef NS_OPTIONS(NSUInteger, MTLPipelineOption) {
    MTLPipelineOptionNone = 0,
};

@interface MTLPipelineBufferDescriptor : NSObject
@property (nonatomic) MTLMutability mutability;
@end

@interface MTLPipelineBufferDescriptorArray : NSObject
- (MTLPipelineBufferDescriptor* _Nonnull)objectAtIndexedSubscript:(NSUInteger)index;
@end

//== MTLRenderPipeline.h =======================================================

typedef NS_ENUM(NSUInteger, MTLBlendFactor) {
    MTLBlendFactorZero = 0,
    MTLBlendFactorOne = 1,
    MTLBlendFactorSourceColor = 2,
    MTLBlendFactorOneMinusSourceColor = 3,
    MTLBlendFactorSourceAlpha = 4,
    MTLBlendFactorOneMinusSourceAlpha = 5,
    MTLBlendFactorDestinationColor = 6,
    MTLBlendFactorOneMinusDestinationColor = 7,
    MTLBlendFactorDestinationAlpha = 8,
    MTLBlendFactorOneMinusDestinationAlpha = 9,
    MTLBlendFactorSourceAlphaSaturated = 10,
    MTLBlendFactorBlendColor = 11,
    MTLBlendFactorOneMinusBlendColor = 12,
    MTLBlendFactorBlendAlpha = 13,
    MTLBlendFactorOneMinusBlendAlpha = 14,
    MTLBlendFactorSource1Color = 15,
    MTLBlendFactorOneMinusSource1Color = 16,
    MTLBlendFactorSource1Alpha = 17,
    MTLBlendFactorOneMinusSource1Alpha = 18,
};

typedef NS_ENUM(NSUInteger, MTLBlendOperation) {
    MTLBlendOperationAdd = 0,
    MTLBlendOperationSubtract = 1,
    MTLBlendOperationReverseSubtract = 2,
    MTLBlendOperationMin = 3,
    MTLBlendOperationMax = 4,
};

typedef NS_OPTIONS(NSUInteger, MTLColorWriteMask) {
    MTLColorWriteMaskNone = 0,
    MTLColorWriteMaskAlpha = 0x1 << 0,
    MTLColorWriteMaskBlue = 0x1 << 1,
    MTLColorWriteMaskGreen = 0x1 << 2,
    MTLColorWriteMaskRed = 0x1 << 3,
    MTLColorWriteMaskAll = 0xf,
};

@interface MTLRenderPipelineColorAttachmentDescriptor : NSObject
@property (nonatomic) MTLPixelFormat pixelFormat;
@property (nonatomic, getter=isBlendingEnabled) BOOL blendingEnabled;
@property (nonatomic) MTLBlendFactor sourceRGBBlendFactor;
@property (nonatomic) MTLBlendFactor destinationRGBBlendFactor;
@property (nonatomic) MTLBlendOperation rgbBlendOperation;
@property (nonatomic) MTLBlendFactor sourceAlphaBlendFactor;
@property (nonatomic) MTLBlendFactor destinationAlphaBlendFactor;
@property (nonatomic) MTLBlendOperation alphaBlendOperation;
@property (nonatomic) MTLColorWriteMask writeMask;
@end

@interface MTLRenderPipelineColorAttachmentDescriptorArray : NSObject
- (MTLRenderPipelineColorAttachmentDescriptor* _Nonnull)objectAtIndexedSubscript:(NSUInteger)attachmentIndex;
@end

@interface MTLRenderPipelineDescriptor : NSObject
@property (nullable, copy, nonatomic) NSString* label;
@property (nullable, readwrite, nonatomic, strong) id<MTLFunction> vertexFunction;
@property (nullable, readwrite, nonatomic, strong) id<MTLFunction> fragmentFunction;
// 'strong' instead of the SDK's 'copy', see the note at the top of this file
@property (nullable, strong, nonatomic) MTLVertexDescriptor* vertexDescriptor;
@property (readwrite, nonatomic) NSUInteger rasterSampleCount;
@property (readwrite, nonatomic, getter=isAlphaToCoverageEnabled) BOOL alphaToCoverageEnabled;
@property (readwrite, nonatomic, getter=isAlphaToOneEnabled) BOOL alphaToOneEnabled;
@property (readwrite, nonatomic, getter=isRasterizationEnabled) BOOL rasterizationEnabled;
@property (readonly) MTLRenderPipelineColorAttachmentDescriptorArray* _Nonnull colorAttachments;
@property (nonatomic) MTLPixelFormat depthAttachmentPixelFormat;
@property (nonatomic) MTLPixelFormat stencilAttachmentPixelFormat;
@property (readonly) MTLPipelineBufferDescriptorArray* _Nonnull vertexBuffers;
@property (readonly) MTLPipelineBufferDescriptorArray* _Nonnull fragmentBuffers;
@end

@protocol MTLRenderPipelineState <NSObject>
@end

//== MTLComputePipeline.h ======================================================

@interface MTLComputePipelineDescriptor : NSObject
@property (nullable, copy, nonatomic) NSString* label;
@property (nullable, readwrite, nonatomic, strong) id<MTLFunction> computeFunction;
@property (readwrite, nonatomic) BOOL threadGroupSizeIsMultipleOfThreadExecutionWidth;
@property (readonly) MTLPipelineBufferDescriptorArray* _Nonnull buffers;
@end

typedef NSObject MTLComputePipelineReflection;
typedef __autoreleasing MTLComputePipelineReflection* _Nullable MTLAutoreleasedComputePipelineReflection;

@protocol MTLComputePipelineState <NSObject>
@end

//== MTLDepthStencil.h =========================================================

typedef NS_ENUM(NSUInteger, MTLStencilOperation) {
    MTLStencilOperationKeep = 0,
    MTLStencilOperationZero = 1,
    MTLStencilOperationReplace = 2,
    MTLStencilOperationIncrementClamp = 3,
    MTLStencilOperationDecrementClamp = 4,
    MTLStencilOperationInvert = 5,
    MTLStencilOperationIncrementWrap = 6,
    MTLStencilOperationDecrementWrap = 7,
};

@interface MTLStencilDescriptor : NSObject
@property (nonatomic) MTLCompareFunction stencilCompareFunction;
@property (nonatomic) MTLStencilOperation stencilFailureOperation;
@property (nonatomic) MTLStencilOperation depthFailureOperation;
@property (nonatomic) MTLStencilOperation depthStencilPassOperation;
@property (nonatomic) uint32_t readMask;
@property (nonatomic) uint32_t writeMask;
@end

@interface MTLDepthStencilDescriptor : NSObject
@property (nonatomic) MTLCompareFunction depthCompareFunction;
@property (nonatomic, getter=isDepthWriteEnabled) BOOL depthWriteEnabled;
@property (nullable, strong, nonatomic) MTLStencilDescriptor* frontFaceStencil;
@property (nullable, strong, nonatomic) MTLStencilDescriptor* backFaceStencil;
@property (nullable, copy, nonatomic) NSString* label;
@end

@protocol MTLDepthStencilState <NSObject>
@end

//== MTLRenderPass.h ===========================================================

typedef struct {
    double red, green, blue, alpha;
} MTLClearColor;

static inline MTLClearColor MTLClearColorMake(double red, double green, double blue, double alpha) {
    MTLClearColor c; c.red = red; c.green = green; c.blue = blue; c.alpha = alpha; return c;
}

typedef NS_ENUM(NSUInteger, MTLLoadAction) {
    MTLLoadActionDontCare = 0,
    MTLLoadActionLoad = 1,
    MTLLoadActionClear = 2,
};

typedef NS_ENUM(NSUInteger, MTLStoreAction) {
    MTLStoreActionDontCare = 0,
    MTLStoreActionStore = 1,
    MTLStoreActionMultisampleResolve = 2,
    MTLStoreActionStoreAndMultisampleResolve = 3,
    MTLStoreActionUnknown = 4,
    MTLStoreActionCustomSampleDepthStore = 5,
};

@interface MTLRenderPassAttachmentDescriptor : NSObject
@property (nullable, nonatomic, strong) id<MTLTexture> texture;
@property (nonatomic) NSUInteger level;
@property (nonatomic) NSUInteger slice;
@property (nonatomic) NSUInteger depthPlane;
@property (nullable, nonatomic, strong) id<MTLTexture> resolveTexture;
@property (nonatomic) NSUInteger resolveLevel;
@property (nonatomic) NSUInteger resolveSlice;
@property (nonatomic) NSUInteger resolveDepthPlane;
@property (nonatomic) MTLLoadAction loadAction;
@property (nonatomic) MTLStoreAction storeAction;
@end

@interface MTLRenderPassColorAttachmentDescriptor : MTLRenderPassAttachmentDescriptor
@property (nonatomic) MTLClearColor clearColor;
@end

@interface MTLRenderPassDepthAttachmentDescriptor : MTLRenderPassAttachmentDescriptor
@property (nonatomic) double clearDepth;
@end

@interface MTLRenderPassStencilAttachmentDescriptor : MTLRenderPassAttachmentDescriptor
@property (nonatomic) uint32_t clearStencil;
@end

@interface MTLRenderPassColorAttachmentDescriptorArray : NSObject
- (MTLRenderPassColorAttachmentDescriptor* _Nonnull)objectAtIndexedSubscript:(NSUInteger)attachmentIndex;
@end

@interface MTLRenderPassDescriptor : NSObject
+ (MTLRenderPassDescriptor* _Nonnull)renderPassDescriptor;
@property (readonly) MTLRenderPassColorAttachmentDescriptorArray* _Nonnull colorAttachments;
@property (nullable, strong, nonatomic) MTLRenderPassDepthAttachmentDescriptor* depthAttachment;
@property (nullable, strong, nonatomic) MTLRenderPassStencilAttachmentDescriptor* stencilAttachment;
@end

//== MTLCommandEncoder.h =======================================================

@protocol MTLCommandEncoder <NSObject>
@property (nullable, copy, atomic) NSString* label;
- (void)endEncoding;
- (void)pushDebugGroup:(NSString* _Nonnull)string;
- (void)popDebugGroup;
@end

//== MTLRenderCommandEncoder.h =================================================

typedef NS_ENUM(NSUInteger, MTLPrimitiveType) {
    MTLPrimitiveTypePoint = 0,
    MTLPrimitiveTypeLine = 1,
    MTLPrimitiveTypeLineStrip = 2,
    MTLPrimitiveTypeTriangle = 3,
    MTLPrimitiveTypeTriangleStrip = 4,
};

typedef NS_ENUM(NSUInteger, MTLIndexType) {
    MTLIndexTypeUInt16 = 0,
    MTLIndexTypeUInt32 = 1,
};

typedef NS_ENUM(NSUInteger, MTLCullMode) {
    MTLCullModeNone = 0,
    MTLCullModeFront = 1,
    MTLCullModeBack = 2,
};

typedef NS_ENUM(NSUInteger, MTLWinding) {
    MTLWindingClockwise = 0,
    MTLWindingCounterClockwise = 1,
};

typedef struct {
    double originX, originY, width, height, znear, zfar;
} MTLViewport;

typedef struct {
    NSUInteger x, y, width, height;
} MTLScissorRect;

@protocol MTLRenderCommandEncoder <MTLCommandEncoder>
- (void)setViewport:(MTLViewport)viewport;
- (void)setScissorRect:(MTLScissorRect)rect;
- (void)setCullMode:(MTLCullMode)cullMode;
- (void)setFrontFacingWinding:(MTLWinding)frontFacingWinding;
- (void)setBlendColorRed:(float)red green:(float)green blue:(float)blue alpha:(float)alpha;
- (void)setStencilReferenceValue:(uint32_t)referenceValue;
- (void)setDepthBias:(float)depthBias slopeScale:(float)slopeScale clamp:(float)clamp;
- (void)setRenderPipelineState:(id<MTLRenderPipelineState> _Nonnull)pipelineState;
- (void)setDepthStencilState:(id<MTLDepthStencilState> _Nullable)depthStencilState;
- (void)setVertexBuffer:(id<MTLBuffer> _Nullable)buffer offset:(NSUInteger)offset atIndex:(NSUInteger)index;
- (void)setVertexBufferOffset:(NSUInteger)offset atIndex:(NSUInteger)index;
- (void)setVertexTexture:(id<MTLTexture> _Nullable)texture atIndex:(NSUInteger)index;
- (void)setVertexSamplerState:(id<MTLSamplerState> _Nullable)sampler atIndex:(NSUInteger)index;
- (void)setFragmentBuffer:(id<MTLBuffer> _Nullable)buffer offset:(NSUInteger)offset atIndex:(NSUInteger)index;
- (void)setFragmentBufferOffset:(NSUInteger)offset atIndex:(NSUInteger)index;
- (void)setFragmentTexture:(id<MTLTexture> _Nullable)texture atIndex:(NSUInteger)index;
- (void)setFragmentSamplerState:(id<MTLSamplerState> _Nullable)sampler atIndex:(NSUInteger)index;
- (void)drawPrimitives:(MTLPrimitiveType)primitiveType
           vertexStart:(NSUInteger)vertexStart
           vertexCount:(NSUInteger)vertexCount
         instanceCount:(NSUInteger)instanceCount;
- (void)drawPrimitives:(MTLPrimitiveType)primitiveType
           vertexStart:(NSUInteger)vertexStart
           vertexCount:(NSUInteger)vertexCount
         instanceCount:(NSUInteger)instanceCount
          baseInstance:(NSUInteger)baseInstance;
- (void)drawIndexedPrimitives:(MTLPrimitiveType)primitiveType
                   indexCount:(NSUInteger)indexCount
                    indexType:(MTLIndexType)indexType
                  indexBuffer:(id<MTLBuffer> _Nonnull)indexBuffer
            indexBufferOffset:(NSUInteger)indexBufferOffset
                instanceCount:(NSUInteger)instanceCount;
- (void)drawIndexedPrimitives:(MTLPrimitiveType)primitiveType
                   indexCount:(NSUInteger)indexCount
                    indexType:(MTLIndexType)indexType
                  indexBuffer:(id<MTLBuffer> _Nonnull)indexBuffer
            indexBufferOffset:(NSUInteger)indexBufferOffset
                instanceCount:(NSUInteger)instanceCount
                   baseVertex:(NSInteger)baseVertex
                 baseInstance:(NSUInteger)baseInstance;
@end

//== MTLComputeCommandEncoder.h ================================================

@protocol MTLComputeCommandEncoder <MTLCommandEncoder>
- (void)setComputePipelineState:(id<MTLComputePipelineState> _Nonnull)state;
- (void)setBuffer:(id<MTLBuffer> _Nullable)buffer offset:(NSUInteger)offset atIndex:(NSUInteger)index;
- (void)setBufferOffset:(NSUInteger)offset atIndex:(NSUInteger)index;
- (void)setTexture:(id<MTLTexture> _Nullable)texture atIndex:(NSUInteger)index;
- (void)setSamplerState:(id<MTLSamplerState> _Nullable)sampler atIndex:(NSUInteger)index;
- (void)dispatchThreadgroups:(MTLSize)threadgroupsPerGrid threadsPerThreadgroup:(MTLSize)threadsPerThreadgroup;
@end

//== MTLDrawable.h =============================================================

@protocol MTLDrawable <NSObject>
- (void)present;
@end

//== MTLCommandBuffer.h ========================================================

@protocol MTLCommandBuffer;
typedef void (^MTLCommandBufferHandler)(id<MTLCommandBuffer> _Nonnull);

@protocol MTLCommandBuffer <NSObject>
- (void)enqueue;
- (void)commit;
- (void)addCompletedHandler:(MTLCommandBufferHandler _Nonnull)block;
- (void)presentDrawable:(id<MTLDrawable> _Nonnull)drawable;
- (id<MTLRenderCommandEncoder> _Nullable)renderCommandEncoderWithDescriptor:(MTLRenderPassDescriptor* _Nonnull)renderPassDescriptor;
- (id<MTLComputeCommandEncoder> _Nullable)computeCommandEncoder;
@end

//== MTLCommandQueue.h =========================================================

@protocol MTLCommandQueue <NSObject>
- (id<MTLCommandBuffer> _Nullable)commandBuffer;
- (id<MTLCommandBuffer> _Nullable)commandBufferWithUnretainedReferences;
@end

//== MTLDevice.h ===============================================================

typedef NS_ENUM(NSInteger, MTLGPUFamily) {
    MTLGPUFamilyApple1 = 1001,
    MTLGPUFamilyApple2 = 1002,
    MTLGPUFamilyApple3 = 1003,
    MTLGPUFamilyApple4 = 1004,
    MTLGPUFamilyApple5 = 1005,
    MTLGPUFamilyApple6 = 1006,
    MTLGPUFamilyApple7 = 1007,
    MTLGPUFamilyApple8 = 1008,
    MTLGPUFamilyApple9 = 1009,
    MTLGPUFamilyMac1 = 2001,
    MTLGPUFamilyMac2 = 2002,
    MTLGPUFamilyCommon1 = 3001,
    MTLGPUFamilyCommon2 = 3002,
    MTLGPUFamilyCommon3 = 3003,
    MTLGPUFamilyMetal3 = 5001,
};

@protocol MTLDevice <NSObject>
- (BOOL)supportsFamily:(MTLGPUFamily)gpuFamily;
- (id<MTLCommandQueue> _Nullable)newCommandQueue;
- (id<MTLBuffer> _Nullable)newBufferWithLength:(NSUInteger)length options:(MTLResourceOptions)options;
- (id<MTLBuffer> _Nullable)newBufferWithBytes:(const void* _Nonnull)pointer length:(NSUInteger)length options:(MTLResourceOptions)options;
- (id<MTLTexture> _Nullable)newTextureWithDescriptor:(MTLTextureDescriptor* _Nonnull)descriptor;
- (id<MTLSamplerState> _Nullable)newSamplerStateWithDescriptor:(MTLSamplerDescriptor* _Nonnull)descriptor;
- (id<MTLLibrary> _Nullable)newLibraryWithSource:(NSString* _Nonnull)source
                                         options:(MTLCompileOptions* _Nullable)options
                                           error:(__autoreleasing NSError* _Nullable* _Nullable)error;
- (id<MTLLibrary> _Nullable)newLibraryWithData:(dispatch_data_t _Nonnull)data
                                         error:(__autoreleasing NSError* _Nullable* _Nullable)error;
- (id<MTLRenderPipelineState> _Nullable)newRenderPipelineStateWithDescriptor:(MTLRenderPipelineDescriptor* _Nonnull)descriptor
                                                                      error:(__autoreleasing NSError* _Nullable* _Nullable)error;
- (id<MTLComputePipelineState> _Nullable)newComputePipelineStateWithDescriptor:(MTLComputePipelineDescriptor* _Nonnull)descriptor
                                                                      options:(MTLPipelineOption)options
                                                                   reflection:(MTLAutoreleasedComputePipelineReflection* _Nullable)reflection
                                                                        error:(__autoreleasing NSError* _Nullable* _Nullable)error;
- (id<MTLDepthStencilState> _Nullable)newDepthStencilStateWithDescriptor:(MTLDepthStencilDescriptor* _Nonnull)descriptor;
@end

#endif // METAL_MOCK_METAL_H_INCLUDED
