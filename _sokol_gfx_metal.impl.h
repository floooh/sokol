/*
    Sokol Metal rendering backend.
*/
#ifndef SOKOL_IMPL_GUARD
#error "Please do not include *.impl.h files directly"
#endif

#if !__has_feature(objc_arc)
#error "Please enable ARC when using the Metal backend"
#endif

/* memset() */
#include <string.h>
#import <Metal/Metal.h>

#ifdef __cplusplus
extern "C" {
#endif
    
enum {
    _SG_MTL_DEFAULT_UB_SIZE = 4 * 1024 * 1024,
    #if defined(SOKOL_METAL_MACOS)
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
            #if defined(SOKOL_METAL_MACOS)
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
        #if defined(SOKOL_METAL_MACOS)
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
    deferred-release queue, and the resource will then be releases N frames later,
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

_SOKOL_PRIVATE void _sg_init_buffer(_sg_buffer* buf) {
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

_SOKOL_PRIVATE void _sg_init_image(_sg_image* img) {
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

_SOKOL_PRIVATE void _sg_init_shader(_sg_shader* shd) {
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

_SOKOL_PRIVATE void _sg_init_pipeline(_sg_pipeline* pip) {
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

_SOKOL_PRIVATE void _sg_init_pass(_sg_pass* pass) {
    SOKOL_ASSERT(pass);
    memset(pass, 0, sizeof(_sg_pass));
}

/*-- a simple state cache for the resource bindings --------------------------*/
static const _sg_pipeline* _sg_mtl_cur_pipeline;
static sg_pipeline _sg_mtl_cur_pipeline_id;
static const _sg_buffer* _sg_mtl_cur_indexbuffer;
static sg_buffer _sg_mtl_cur_indexbuffer_id;
static const _sg_buffer* _sg_mtl_cur_vertexbuffers[SG_MAX_SHADERSTAGE_BUFFERS];
static sg_buffer _sg_mtl_cur_vertexbuffer_ids[SG_MAX_SHADERSTAGE_BUFFERS];
static const _sg_image* _sg_mtl_cur_vs_images[SG_MAX_SHADERSTAGE_IMAGES];
static sg_image _sg_mtl_cur_vs_image_ids[SG_MAX_SHADERSTAGE_IMAGES];
static const _sg_image* _sg_mtl_cur_fs_images[SG_MAX_SHADERSTAGE_IMAGES];
static sg_image _sg_mtl_cur_fs_image_ids[SG_MAX_SHADERSTAGE_IMAGES];

_SOKOL_PRIVATE void _sg_mtl_clear_state_cache() {
    _sg_mtl_cur_pipeline = 0;
    _sg_mtl_cur_pipeline_id.id = SG_INVALID_ID;
    _sg_mtl_cur_indexbuffer = 0;
    _sg_mtl_cur_indexbuffer_id.id = SG_INVALID_ID;
    for (int i = 0; i < SG_MAX_SHADERSTAGE_BUFFERS; i++) {
        _sg_mtl_cur_vertexbuffers[i] = 0;
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
    #if defined(SOKOL_METAL_MACOS)
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
        #if defined(SOKOL_METAL_MACOS)
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
    _sg_init_buffer(buf);
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
    _sg_init_image(img);
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
    _sg_init_shader(shd);
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
    int auto_mtl_attr_index = 0;
    for (int layout_index = 0; layout_index < SG_MAX_SHADERSTAGE_BUFFERS; layout_index++) {
        const sg_vertex_layout_desc* layout_desc = &desc->vertex_layouts[layout_index];
        if (layout_desc->stride == 0) {
            break;
        }
        pip->vertex_layout_valid[layout_index] = true;
        const int mtl_vb_slot = layout_index + SG_MAX_SHADERSTAGE_UBS;
        vtx_desc.layouts[mtl_vb_slot].stride = layout_desc->stride;
        vtx_desc.layouts[mtl_vb_slot].stepFunction = _sg_mtl_step_function(_sg_def(layout_desc->step_func, SG_VERTEXSTEP_PER_VERTEX));
        vtx_desc.layouts[mtl_vb_slot].stepRate = _sg_def(layout_desc->step_rate, 1);
        for (int attr_index = 0; attr_index < SG_MAX_VERTEX_ATTRIBUTES; attr_index++) {
            const sg_vertex_attr_desc* attr_desc = &layout_desc->attrs[attr_index];
            if (attr_desc->format == SG_VERTEXFORMAT_INVALID) {
                break;
            }
            SOKOL_ASSERT(auto_mtl_attr_index < SG_MAX_VERTEX_ATTRIBUTES);
            /* if an attribute name is provided, lookup the Metal attribute index,
               otherwise use the implicit location
            */
            int mtl_attr_index = -1;
            if (attr_desc->name) {
                id<MTLFunction> mtl_vs_func = _sg_mtl_pool[shd->stage[SG_SHADERSTAGE_VS].mtl_func];
                for (MTLVertexAttribute* mtl_attr in mtl_vs_func.vertexAttributes) {
                    if (0 == strcmp(mtl_attr.name.UTF8String, attr_desc->name)) {
                        mtl_attr_index = mtl_attr.attributeIndex;
                        break;
                    }
                }
                if (-1 == mtl_attr_index) {
                    SOKOL_LOG("Named vertex attribute not found in shader: ");
                    SOKOL_LOG(attr_desc->name);
                }
            }
            else {
                mtl_attr_index = auto_mtl_attr_index;
            }
            if (mtl_attr_index != -1) {
                vtx_desc.attributes[mtl_attr_index].format = _sg_mtl_vertex_format(attr_desc->format);
                vtx_desc.attributes[mtl_attr_index].offset = attr_desc->offset;
                vtx_desc.attributes[mtl_attr_index].bufferIndex = mtl_vb_slot;
            }
            auto_mtl_attr_index++;
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
    _sg_init_pipeline(pip);
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
    _sg_init_pass(pass);
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

    #if defined(SOKOL_METAL_MACOS)
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
    _sg_buffer** vbs, int num_vbs, _sg_buffer* ib,
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
        if ((_sg_mtl_cur_vertexbuffers[slot] != vb) || (_sg_mtl_cur_vertexbuffer_ids[slot].id != vb->slot.id)) {
            _sg_mtl_cur_vertexbuffers[slot] = vb;
            _sg_mtl_cur_vertexbuffer_ids[slot].id = vb->slot.id;
            const NSUInteger mtl_slot = SG_MAX_SHADERSTAGE_UBS + slot;
            SOKOL_ASSERT(vb->mtl_buf[vb->active_slot] != _SG_MTL_INVALID_POOL_INDEX);
            [_sg_mtl_cmd_encoder setVertexBuffer:_sg_mtl_pool[vb->mtl_buf[vb->active_slot]] offset:0 atIndex:mtl_slot];
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
        const NSUInteger index_buffer_offset = base_element * _sg_mtl_cur_pipeline->mtl_index_size;
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
    #if defined(SOKOL_METAL_MACOS)
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

_SOKOL_PRIVATE void _sg_reset_state_cache() {
    _sg_mtl_clear_state_cache();
}

#ifdef __cplusplus
} /* extern "C" */
#endif

