/*
    LLM maintained.

    d3d11.h -- mock replacement for the Windows d3d11.h SDK header.

    Provides just the D3D11/DXGI enums, structs and COM-style interfaces that
    the sokol_gfx.h D3D11 backend actually references. The runtime side lives
    in d3d11_mock.c.

    The header supports the C-style COM ABI used by sokol_gfx.h when compiled
    as C: `self->lpVtbl->Method(self, ...)`. It is not intended for the C++
    inline path.
*/
#ifndef MOCK_D3D11_H_INCLUDED
#define MOCK_D3D11_H_INCLUDED

#include "windows.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
    DXGI
============================================================================ */

/* DXGI_FORMAT values match the real DXGI to keep any cross-platform code that
   inspects them consistent. */
typedef enum DXGI_FORMAT {
    DXGI_FORMAT_UNKNOWN                   = 0,
    DXGI_FORMAT_R32G32B32A32_TYPELESS     = 1,
    DXGI_FORMAT_R32G32B32A32_FLOAT        = 2,
    DXGI_FORMAT_R32G32B32A32_UINT         = 3,
    DXGI_FORMAT_R32G32B32A32_SINT         = 4,
    DXGI_FORMAT_R32G32B32_TYPELESS        = 5,
    DXGI_FORMAT_R32G32B32_FLOAT           = 6,
    DXGI_FORMAT_R32G32B32_UINT            = 7,
    DXGI_FORMAT_R32G32B32_SINT            = 8,
    DXGI_FORMAT_R16G16B16A16_TYPELESS     = 9,
    DXGI_FORMAT_R16G16B16A16_FLOAT        = 10,
    DXGI_FORMAT_R16G16B16A16_UNORM        = 11,
    DXGI_FORMAT_R16G16B16A16_UINT         = 12,
    DXGI_FORMAT_R16G16B16A16_SNORM        = 13,
    DXGI_FORMAT_R16G16B16A16_SINT         = 14,
    DXGI_FORMAT_R32G32_TYPELESS           = 15,
    DXGI_FORMAT_R32G32_FLOAT              = 16,
    DXGI_FORMAT_R32G32_UINT               = 17,
    DXGI_FORMAT_R32G32_SINT               = 18,
    DXGI_FORMAT_R32G8X24_TYPELESS         = 19,
    DXGI_FORMAT_D32_FLOAT_S8X24_UINT      = 20,
    DXGI_FORMAT_R32_FLOAT_X8X24_TYPELESS  = 21,
    DXGI_FORMAT_R10G10B10A2_UNORM         = 24,
    DXGI_FORMAT_R11G11B10_FLOAT           = 26,
    DXGI_FORMAT_R8G8B8A8_TYPELESS         = 27,
    DXGI_FORMAT_R8G8B8A8_UNORM            = 28,
    DXGI_FORMAT_R8G8B8A8_UNORM_SRGB       = 29,
    DXGI_FORMAT_R8G8B8A8_UINT             = 30,
    DXGI_FORMAT_R8G8B8A8_SNORM            = 31,
    DXGI_FORMAT_R8G8B8A8_SINT             = 32,
    DXGI_FORMAT_R16G16_TYPELESS           = 33,
    DXGI_FORMAT_R16G16_FLOAT              = 34,
    DXGI_FORMAT_R16G16_UNORM              = 35,
    DXGI_FORMAT_R16G16_UINT               = 36,
    DXGI_FORMAT_R16G16_SNORM              = 37,
    DXGI_FORMAT_R16G16_SINT               = 38,
    DXGI_FORMAT_R32_TYPELESS              = 39,
    DXGI_FORMAT_D32_FLOAT                 = 40,
    DXGI_FORMAT_R32_FLOAT                 = 41,
    DXGI_FORMAT_R32_UINT                  = 42,
    DXGI_FORMAT_R32_SINT                  = 43,
    DXGI_FORMAT_R8G8_UNORM                = 49,
    DXGI_FORMAT_R8G8_UINT                 = 50,
    DXGI_FORMAT_R8G8_SNORM                = 51,
    DXGI_FORMAT_R8G8_SINT                 = 52,
    DXGI_FORMAT_R16_FLOAT                 = 54,
    DXGI_FORMAT_R16_UNORM                 = 56,
    DXGI_FORMAT_R16_UINT                  = 57,
    DXGI_FORMAT_R16_SNORM                 = 58,
    DXGI_FORMAT_R16_SINT                  = 59,
    DXGI_FORMAT_R8_UNORM                  = 61,
    DXGI_FORMAT_R8_UINT                   = 62,
    DXGI_FORMAT_R8_SNORM                  = 63,
    DXGI_FORMAT_R8_SINT                   = 64,
    DXGI_FORMAT_R9G9B9E5_SHAREDEXP        = 67,
    DXGI_FORMAT_BC1_UNORM                 = 71,
    DXGI_FORMAT_BC2_UNORM                 = 74,
    DXGI_FORMAT_BC3_UNORM                 = 77,
    DXGI_FORMAT_BC3_UNORM_SRGB            = 78,
    DXGI_FORMAT_BC4_UNORM                 = 80,
    DXGI_FORMAT_BC4_SNORM                 = 81,
    DXGI_FORMAT_BC5_UNORM                 = 83,
    DXGI_FORMAT_BC5_SNORM                 = 84,
    DXGI_FORMAT_BC6H_UF16                 = 95,
    DXGI_FORMAT_BC6H_SF16                 = 96,
    DXGI_FORMAT_BC7_UNORM                 = 98,
    DXGI_FORMAT_BC7_UNORM_SRGB            = 99,
    DXGI_FORMAT_B8G8R8A8_UNORM            = 87,
    DXGI_FORMAT_B8G8R8A8_TYPELESS         = 90,
    DXGI_FORMAT_B8G8R8A8_UNORM_SRGB       = 91,
    _DXGI_FORMAT_FORCE_UINT               = 0x7fffffff
} DXGI_FORMAT;

typedef struct DXGI_SAMPLE_DESC {
    UINT Count;
    UINT Quality;
} DXGI_SAMPLE_DESC;

/* ============================================================================
    D3D11 enums / flag constants
============================================================================ */

typedef enum D3D_FEATURE_LEVEL {
    D3D_FEATURE_LEVEL_9_1  = 0x9100,
    D3D_FEATURE_LEVEL_9_2  = 0x9200,
    D3D_FEATURE_LEVEL_9_3  = 0x9300,
    D3D_FEATURE_LEVEL_10_0 = 0xa000,
    D3D_FEATURE_LEVEL_10_1 = 0xa100,
    D3D_FEATURE_LEVEL_11_0 = 0xb000,
    D3D_FEATURE_LEVEL_11_1 = 0xb100,
    D3D_FEATURE_LEVEL_12_0 = 0xc000,
    D3D_FEATURE_LEVEL_12_1 = 0xc100
} D3D_FEATURE_LEVEL;

typedef enum D3D_PRIMITIVE_TOPOLOGY {
    D3D_PRIMITIVE_TOPOLOGY_UNDEFINED     = 0,
    D3D_PRIMITIVE_TOPOLOGY_POINTLIST     = 1,
    D3D_PRIMITIVE_TOPOLOGY_LINELIST      = 2,
    D3D_PRIMITIVE_TOPOLOGY_LINESTRIP     = 3,
    D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST  = 4,
    D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP = 5
} D3D_PRIMITIVE_TOPOLOGY;
typedef D3D_PRIMITIVE_TOPOLOGY D3D11_PRIMITIVE_TOPOLOGY;

#define D3D11_PRIMITIVE_TOPOLOGY_POINTLIST     D3D_PRIMITIVE_TOPOLOGY_POINTLIST
#define D3D11_PRIMITIVE_TOPOLOGY_LINELIST      D3D_PRIMITIVE_TOPOLOGY_LINELIST
#define D3D11_PRIMITIVE_TOPOLOGY_LINESTRIP     D3D_PRIMITIVE_TOPOLOGY_LINESTRIP
#define D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST  D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST
#define D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP

typedef enum D3D11_USAGE {
    D3D11_USAGE_DEFAULT   = 0,
    D3D11_USAGE_IMMUTABLE = 1,
    D3D11_USAGE_DYNAMIC   = 2,
    D3D11_USAGE_STAGING   = 3
} D3D11_USAGE;

/* D3D11_BIND_FLAG (used as UINT bitmask, not enum) */
#define D3D11_BIND_VERTEX_BUFFER    0x0001u
#define D3D11_BIND_INDEX_BUFFER     0x0002u
#define D3D11_BIND_CONSTANT_BUFFER  0x0004u
#define D3D11_BIND_SHADER_RESOURCE  0x0008u
#define D3D11_BIND_STREAM_OUTPUT    0x0010u
#define D3D11_BIND_RENDER_TARGET    0x0020u
#define D3D11_BIND_DEPTH_STENCIL    0x0040u
#define D3D11_BIND_UNORDERED_ACCESS 0x0080u

/* D3D11_CPU_ACCESS_FLAG */
#define D3D11_CPU_ACCESS_WRITE      0x00010000u
#define D3D11_CPU_ACCESS_READ       0x00020000u

/* D3D11_RESOURCE_MISC_FLAG */
#define D3D11_RESOURCE_MISC_TEXTURECUBE            0x0004u
#define D3D11_RESOURCE_MISC_BUFFER_ALLOW_RAW_VIEWS 0x0020u

typedef enum D3D11_FILTER {
    D3D11_FILTER_MIN_MAG_MIP_POINT                          = 0x00,
    D3D11_FILTER_MIN_MAG_POINT_MIP_LINEAR                   = 0x01,
    D3D11_FILTER_MIN_POINT_MAG_LINEAR_MIP_POINT             = 0x04,
    D3D11_FILTER_MIN_POINT_MAG_MIP_LINEAR                   = 0x05,
    D3D11_FILTER_MIN_LINEAR_MAG_MIP_POINT                   = 0x10,
    D3D11_FILTER_MIN_LINEAR_MAG_POINT_MIP_LINEAR            = 0x11,
    D3D11_FILTER_MIN_MAG_LINEAR_MIP_POINT                   = 0x14,
    D3D11_FILTER_MIN_MAG_MIP_LINEAR                         = 0x15,
    D3D11_FILTER_ANISOTROPIC                                = 0x55,
    D3D11_FILTER_COMPARISON_MIN_MAG_MIP_POINT               = 0x80,
    D3D11_FILTER_COMPARISON_MIN_MAG_POINT_MIP_LINEAR        = 0x81,
    D3D11_FILTER_COMPARISON_MIN_POINT_MAG_LINEAR_MIP_POINT  = 0x84,
    D3D11_FILTER_COMPARISON_MIN_POINT_MAG_MIP_LINEAR        = 0x85,
    D3D11_FILTER_COMPARISON_MIN_LINEAR_MAG_MIP_POINT        = 0x90,
    D3D11_FILTER_COMPARISON_MIN_LINEAR_MAG_POINT_MIP_LINEAR = 0x91,
    D3D11_FILTER_COMPARISON_MIN_MAG_LINEAR_MIP_POINT        = 0x94,
    D3D11_FILTER_COMPARISON_MIN_MAG_MIP_LINEAR              = 0x95,
    D3D11_FILTER_COMPARISON_ANISOTROPIC                     = 0xd5
} D3D11_FILTER;

typedef enum D3D11_TEXTURE_ADDRESS_MODE {
    D3D11_TEXTURE_ADDRESS_WRAP        = 1,
    D3D11_TEXTURE_ADDRESS_MIRROR      = 2,
    D3D11_TEXTURE_ADDRESS_CLAMP       = 3,
    D3D11_TEXTURE_ADDRESS_BORDER      = 4,
    D3D11_TEXTURE_ADDRESS_MIRROR_ONCE = 5
} D3D11_TEXTURE_ADDRESS_MODE;

typedef enum D3D11_COMPARISON_FUNC {
    D3D11_COMPARISON_NEVER         = 1,
    D3D11_COMPARISON_LESS          = 2,
    D3D11_COMPARISON_EQUAL         = 3,
    D3D11_COMPARISON_LESS_EQUAL    = 4,
    D3D11_COMPARISON_GREATER       = 5,
    D3D11_COMPARISON_NOT_EQUAL     = 6,
    D3D11_COMPARISON_GREATER_EQUAL = 7,
    D3D11_COMPARISON_ALWAYS        = 8
} D3D11_COMPARISON_FUNC;

typedef enum D3D11_STENCIL_OP {
    D3D11_STENCIL_OP_KEEP     = 1,
    D3D11_STENCIL_OP_ZERO     = 2,
    D3D11_STENCIL_OP_REPLACE  = 3,
    D3D11_STENCIL_OP_INCR_SAT = 4,
    D3D11_STENCIL_OP_DECR_SAT = 5,
    D3D11_STENCIL_OP_INVERT   = 6,
    D3D11_STENCIL_OP_INCR     = 7,
    D3D11_STENCIL_OP_DECR     = 8
} D3D11_STENCIL_OP;

typedef enum D3D11_BLEND {
    D3D11_BLEND_ZERO             = 1,
    D3D11_BLEND_ONE              = 2,
    D3D11_BLEND_SRC_COLOR        = 3,
    D3D11_BLEND_INV_SRC_COLOR    = 4,
    D3D11_BLEND_SRC_ALPHA        = 5,
    D3D11_BLEND_INV_SRC_ALPHA    = 6,
    D3D11_BLEND_DEST_ALPHA       = 7,
    D3D11_BLEND_INV_DEST_ALPHA   = 8,
    D3D11_BLEND_DEST_COLOR       = 9,
    D3D11_BLEND_INV_DEST_COLOR   = 10,
    D3D11_BLEND_SRC_ALPHA_SAT    = 11,
    D3D11_BLEND_BLEND_FACTOR     = 14,
    D3D11_BLEND_INV_BLEND_FACTOR = 15,
    D3D11_BLEND_SRC1_COLOR       = 16,
    D3D11_BLEND_INV_SRC1_COLOR   = 17,
    D3D11_BLEND_SRC1_ALPHA       = 18,
    D3D11_BLEND_INV_SRC1_ALPHA   = 19
} D3D11_BLEND;

typedef enum D3D11_BLEND_OP {
    D3D11_BLEND_OP_ADD          = 1,
    D3D11_BLEND_OP_SUBTRACT     = 2,
    D3D11_BLEND_OP_REV_SUBTRACT = 3,
    D3D11_BLEND_OP_MIN          = 4,
    D3D11_BLEND_OP_MAX          = 5
} D3D11_BLEND_OP;

/* D3D11_COLOR_WRITE_ENABLE bitmask */
#define D3D11_COLOR_WRITE_ENABLE_RED   0x1u
#define D3D11_COLOR_WRITE_ENABLE_GREEN 0x2u
#define D3D11_COLOR_WRITE_ENABLE_BLUE  0x4u
#define D3D11_COLOR_WRITE_ENABLE_ALPHA 0x8u
#define D3D11_COLOR_WRITE_ENABLE_ALL   0xFu

typedef enum D3D11_INPUT_CLASSIFICATION {
    D3D11_INPUT_PER_VERTEX_DATA   = 0,
    D3D11_INPUT_PER_INSTANCE_DATA = 1
} D3D11_INPUT_CLASSIFICATION;

typedef enum D3D11_CULL_MODE {
    D3D11_CULL_NONE  = 1,
    D3D11_CULL_FRONT = 2,
    D3D11_CULL_BACK  = 3
} D3D11_CULL_MODE;

typedef enum D3D11_FILL_MODE {
    D3D11_FILL_WIREFRAME = 2,
    D3D11_FILL_SOLID     = 3
} D3D11_FILL_MODE;

typedef enum D3D11_MAP {
    D3D11_MAP_READ               = 1,
    D3D11_MAP_WRITE              = 2,
    D3D11_MAP_READ_WRITE         = 3,
    D3D11_MAP_WRITE_DISCARD      = 4,
    D3D11_MAP_WRITE_NO_OVERWRITE = 5
} D3D11_MAP;

/* D3D11_CLEAR_FLAG bitmask */
#define D3D11_CLEAR_DEPTH   0x1u
#define D3D11_CLEAR_STENCIL 0x2u

/* D3D11_FORMAT_SUPPORT bitmask */
#define D3D11_FORMAT_SUPPORT_TEXTURE2D                   (1u <<  1)
#define D3D11_FORMAT_SUPPORT_SHADER_SAMPLE               (1u <<  9)
#define D3D11_FORMAT_SUPPORT_RENDER_TARGET               (1u << 14)
#define D3D11_FORMAT_SUPPORT_BLENDABLE                   (1u << 15)
#define D3D11_FORMAT_SUPPORT_DEPTH_STENCIL               (1u << 16)
#define D3D11_FORMAT_SUPPORT_MULTISAMPLE_RENDERTARGET    (1u << 18)
#define D3D11_FORMAT_SUPPORT_TYPED_UNORDERED_ACCESS_VIEW (1u << 24)

/* view dimension enums */
typedef enum D3D11_SRV_DIMENSION {
    D3D11_SRV_DIMENSION_UNKNOWN          = 0,
    D3D11_SRV_DIMENSION_BUFFER           = 1,
    D3D11_SRV_DIMENSION_TEXTURE1D        = 2,
    D3D11_SRV_DIMENSION_TEXTURE1DARRAY   = 3,
    D3D11_SRV_DIMENSION_TEXTURE2D        = 4,
    D3D11_SRV_DIMENSION_TEXTURE2DARRAY   = 5,
    D3D11_SRV_DIMENSION_TEXTURE2DMS      = 6,
    D3D11_SRV_DIMENSION_TEXTURE2DMSARRAY = 7,
    D3D11_SRV_DIMENSION_TEXTURE3D        = 8,
    D3D11_SRV_DIMENSION_TEXTURECUBE      = 9,
    D3D11_SRV_DIMENSION_TEXTURECUBEARRAY = 10,
    D3D11_SRV_DIMENSION_BUFFEREX         = 11
} D3D11_SRV_DIMENSION;

typedef enum D3D11_UAV_DIMENSION {
    D3D11_UAV_DIMENSION_UNKNOWN        = 0,
    D3D11_UAV_DIMENSION_BUFFER         = 1,
    D3D11_UAV_DIMENSION_TEXTURE1D      = 2,
    D3D11_UAV_DIMENSION_TEXTURE1DARRAY = 3,
    D3D11_UAV_DIMENSION_TEXTURE2D      = 4,
    D3D11_UAV_DIMENSION_TEXTURE2DARRAY = 5,
    D3D11_UAV_DIMENSION_TEXTURE3D      = 8
} D3D11_UAV_DIMENSION;

typedef enum D3D11_RTV_DIMENSION {
    D3D11_RTV_DIMENSION_UNKNOWN          = 0,
    D3D11_RTV_DIMENSION_BUFFER           = 1,
    D3D11_RTV_DIMENSION_TEXTURE1D        = 2,
    D3D11_RTV_DIMENSION_TEXTURE1DARRAY   = 3,
    D3D11_RTV_DIMENSION_TEXTURE2D        = 4,
    D3D11_RTV_DIMENSION_TEXTURE2DARRAY   = 5,
    D3D11_RTV_DIMENSION_TEXTURE2DMS      = 6,
    D3D11_RTV_DIMENSION_TEXTURE2DMSARRAY = 7,
    D3D11_RTV_DIMENSION_TEXTURE3D        = 8
} D3D11_RTV_DIMENSION;

typedef enum D3D11_DSV_DIMENSION {
    D3D11_DSV_DIMENSION_UNKNOWN          = 0,
    D3D11_DSV_DIMENSION_TEXTURE1D        = 1,
    D3D11_DSV_DIMENSION_TEXTURE1DARRAY   = 2,
    D3D11_DSV_DIMENSION_TEXTURE2D        = 3,
    D3D11_DSV_DIMENSION_TEXTURE2DARRAY   = 4,
    D3D11_DSV_DIMENSION_TEXTURE2DMS      = 5,
    D3D11_DSV_DIMENSION_TEXTURE2DMSARRAY = 6
} D3D11_DSV_DIMENSION;

typedef enum D3D11_DEPTH_WRITE_MASK {
    D3D11_DEPTH_WRITE_MASK_ZERO = 0,
    D3D11_DEPTH_WRITE_MASK_ALL  = 1
} D3D11_DEPTH_WRITE_MASK;

/* used as MSAA quality level for RTV/DSV MSAA descs */
#define D3D11_STANDARD_MULTISAMPLE_PATTERN 0xffffffffu

/* D3D11_BUFFEREX_SRV_FLAG / D3D11_BUFFER_UAV_FLAG bitmask */
#define D3D11_BUFFEREX_SRV_FLAG_RAW 0x1u
#define D3D11_BUFFER_UAV_FLAG_RAW   0x1u

/* ============================================================================
    D3D11 descriptor structs
============================================================================ */

typedef struct D3D11_BUFFER_DESC {
    UINT ByteWidth;
    D3D11_USAGE Usage;
    UINT BindFlags;
    UINT CPUAccessFlags;
    UINT MiscFlags;
    UINT StructureByteStride;
} D3D11_BUFFER_DESC;

typedef struct D3D11_SUBRESOURCE_DATA {
    const void* pSysMem;
    UINT SysMemPitch;
    UINT SysMemSlicePitch;
} D3D11_SUBRESOURCE_DATA;

typedef struct D3D11_TEXTURE2D_DESC {
    UINT Width;
    UINT Height;
    UINT MipLevels;
    UINT ArraySize;
    DXGI_FORMAT Format;
    DXGI_SAMPLE_DESC SampleDesc;
    D3D11_USAGE Usage;
    UINT BindFlags;
    UINT CPUAccessFlags;
    UINT MiscFlags;
} D3D11_TEXTURE2D_DESC;

typedef struct D3D11_TEXTURE3D_DESC {
    UINT Width;
    UINT Height;
    UINT Depth;
    UINT MipLevels;
    DXGI_FORMAT Format;
    D3D11_USAGE Usage;
    UINT BindFlags;
    UINT CPUAccessFlags;
    UINT MiscFlags;
} D3D11_TEXTURE3D_DESC;

typedef struct D3D11_BUFFER_SRV {
    UINT FirstElement;
    UINT NumElements;
} D3D11_BUFFER_SRV;

typedef struct D3D11_BUFFEREX_SRV {
    UINT FirstElement;
    UINT NumElements;
    UINT Flags;
} D3D11_BUFFEREX_SRV;

typedef struct D3D11_TEX2D_SRV {
    UINT MostDetailedMip;
    UINT MipLevels;
} D3D11_TEX2D_SRV;

typedef struct D3D11_TEX2D_ARRAY_SRV {
    UINT MostDetailedMip;
    UINT MipLevels;
    UINT FirstArraySlice;
    UINT ArraySize;
} D3D11_TEX2D_ARRAY_SRV;

typedef struct D3D11_TEX3D_SRV {
    UINT MostDetailedMip;
    UINT MipLevels;
} D3D11_TEX3D_SRV;

typedef struct D3D11_TEXCUBE_SRV {
    UINT MostDetailedMip;
    UINT MipLevels;
} D3D11_TEXCUBE_SRV;

typedef struct D3D11_TEX2DMS_ARRAY_SRV {
    UINT FirstArraySlice;
    UINT ArraySize;
} D3D11_TEX2DMS_ARRAY_SRV;

typedef struct D3D11_SHADER_RESOURCE_VIEW_DESC {
    DXGI_FORMAT Format;
    D3D11_SRV_DIMENSION ViewDimension;
    union {
        D3D11_BUFFER_SRV        Buffer;
        D3D11_BUFFEREX_SRV      BufferEx;
        D3D11_TEX2D_SRV         Texture2D;
        D3D11_TEX2D_ARRAY_SRV   Texture2DArray;
        D3D11_TEX2DMS_ARRAY_SRV Texture2DMSArray;
        D3D11_TEX3D_SRV         Texture3D;
        D3D11_TEXCUBE_SRV       TextureCube;
    };
} D3D11_SHADER_RESOURCE_VIEW_DESC;

typedef struct D3D11_BUFFER_UAV {
    UINT FirstElement;
    UINT NumElements;
    UINT Flags;
} D3D11_BUFFER_UAV;

typedef struct D3D11_TEX2D_UAV {
    UINT MipSlice;
} D3D11_TEX2D_UAV;

typedef struct D3D11_TEX2D_ARRAY_UAV {
    UINT MipSlice;
    UINT FirstArraySlice;
    UINT ArraySize;
} D3D11_TEX2D_ARRAY_UAV;

typedef struct D3D11_TEX3D_UAV {
    UINT MipSlice;
    UINT FirstWSlice;
    UINT WSize;
} D3D11_TEX3D_UAV;

typedef struct D3D11_UNORDERED_ACCESS_VIEW_DESC {
    DXGI_FORMAT Format;
    D3D11_UAV_DIMENSION ViewDimension;
    union {
        D3D11_BUFFER_UAV      Buffer;
        D3D11_TEX2D_UAV       Texture2D;
        D3D11_TEX2D_ARRAY_UAV Texture2DArray;
        D3D11_TEX3D_UAV       Texture3D;
    };
} D3D11_UNORDERED_ACCESS_VIEW_DESC;

typedef struct D3D11_TEX2D_RTV {
    UINT MipSlice;
} D3D11_TEX2D_RTV;

typedef struct D3D11_TEX2D_ARRAY_RTV {
    UINT MipSlice;
    UINT FirstArraySlice;
    UINT ArraySize;
} D3D11_TEX2D_ARRAY_RTV;

typedef struct D3D11_TEX2DMS_ARRAY_RTV {
    UINT FirstArraySlice;
    UINT ArraySize;
} D3D11_TEX2DMS_ARRAY_RTV;

typedef struct D3D11_TEX3D_RTV {
    UINT MipSlice;
    UINT FirstWSlice;
    UINT WSize;
} D3D11_TEX3D_RTV;

typedef struct D3D11_RENDER_TARGET_VIEW_DESC {
    DXGI_FORMAT Format;
    D3D11_RTV_DIMENSION ViewDimension;
    union {
        D3D11_TEX2D_RTV         Texture2D;
        D3D11_TEX2D_ARRAY_RTV   Texture2DArray;
        D3D11_TEX2DMS_ARRAY_RTV Texture2DMSArray;
        D3D11_TEX3D_RTV         Texture3D;
    };
} D3D11_RENDER_TARGET_VIEW_DESC;

typedef struct D3D11_TEX2D_DSV {
    UINT MipSlice;
} D3D11_TEX2D_DSV;

typedef struct D3D11_TEX2D_ARRAY_DSV {
    UINT MipSlice;
    UINT FirstArraySlice;
    UINT ArraySize;
} D3D11_TEX2D_ARRAY_DSV;

typedef struct D3D11_TEX2DMS_ARRAY_DSV {
    UINT FirstArraySlice;
    UINT ArraySize;
} D3D11_TEX2DMS_ARRAY_DSV;

typedef struct D3D11_DEPTH_STENCIL_VIEW_DESC {
    DXGI_FORMAT Format;
    D3D11_DSV_DIMENSION ViewDimension;
    UINT Flags;
    union {
        D3D11_TEX2D_DSV         Texture2D;
        D3D11_TEX2D_ARRAY_DSV   Texture2DArray;
        D3D11_TEX2DMS_ARRAY_DSV Texture2DMSArray;
    };
} D3D11_DEPTH_STENCIL_VIEW_DESC;

typedef struct D3D11_SAMPLER_DESC {
    D3D11_FILTER Filter;
    D3D11_TEXTURE_ADDRESS_MODE AddressU;
    D3D11_TEXTURE_ADDRESS_MODE AddressV;
    D3D11_TEXTURE_ADDRESS_MODE AddressW;
    FLOAT MipLODBias;
    UINT MaxAnisotropy;
    D3D11_COMPARISON_FUNC ComparisonFunc;
    FLOAT BorderColor[4];
    FLOAT MinLOD;
    FLOAT MaxLOD;
} D3D11_SAMPLER_DESC;

typedef struct D3D11_INPUT_ELEMENT_DESC {
    LPCSTR SemanticName;
    UINT SemanticIndex;
    DXGI_FORMAT Format;
    UINT InputSlot;
    UINT AlignedByteOffset;
    D3D11_INPUT_CLASSIFICATION InputSlotClass;
    UINT InstanceDataStepRate;
} D3D11_INPUT_ELEMENT_DESC;

typedef struct D3D11_RASTERIZER_DESC {
    D3D11_FILL_MODE FillMode;
    D3D11_CULL_MODE CullMode;
    BOOL FrontCounterClockwise;
    INT DepthBias;
    FLOAT DepthBiasClamp;
    FLOAT SlopeScaledDepthBias;
    BOOL DepthClipEnable;
    BOOL ScissorEnable;
    BOOL MultisampleEnable;
    BOOL AntialiasedLineEnable;
} D3D11_RASTERIZER_DESC;

typedef struct D3D11_DEPTH_STENCILOP_DESC {
    D3D11_STENCIL_OP StencilFailOp;
    D3D11_STENCIL_OP StencilDepthFailOp;
    D3D11_STENCIL_OP StencilPassOp;
    D3D11_COMPARISON_FUNC StencilFunc;
} D3D11_DEPTH_STENCILOP_DESC;

typedef struct D3D11_DEPTH_STENCIL_DESC {
    BOOL DepthEnable;
    D3D11_DEPTH_WRITE_MASK DepthWriteMask;
    D3D11_COMPARISON_FUNC DepthFunc;
    BOOL StencilEnable;
    UINT8 StencilReadMask;
    UINT8 StencilWriteMask;
    D3D11_DEPTH_STENCILOP_DESC FrontFace;
    D3D11_DEPTH_STENCILOP_DESC BackFace;
} D3D11_DEPTH_STENCIL_DESC;

typedef struct D3D11_RENDER_TARGET_BLEND_DESC {
    BOOL BlendEnable;
    D3D11_BLEND SrcBlend;
    D3D11_BLEND DestBlend;
    D3D11_BLEND_OP BlendOp;
    D3D11_BLEND SrcBlendAlpha;
    D3D11_BLEND DestBlendAlpha;
    D3D11_BLEND_OP BlendOpAlpha;
    UINT8 RenderTargetWriteMask;
} D3D11_RENDER_TARGET_BLEND_DESC;

typedef struct D3D11_BLEND_DESC {
    BOOL AlphaToCoverageEnable;
    BOOL IndependentBlendEnable;
    D3D11_RENDER_TARGET_BLEND_DESC RenderTarget[8];
} D3D11_BLEND_DESC;

typedef struct D3D11_MAPPED_SUBRESOURCE {
    void* pData;
    UINT RowPitch;
    UINT DepthPitch;
} D3D11_MAPPED_SUBRESOURCE;

typedef struct D3D11_VIEWPORT {
    FLOAT TopLeftX;
    FLOAT TopLeftY;
    FLOAT Width;
    FLOAT Height;
    FLOAT MinDepth;
    FLOAT MaxDepth;
} D3D11_VIEWPORT;

typedef struct D3D11_RECT {
    LONG left;
    LONG top;
    LONG right;
    LONG bottom;
} D3D11_RECT;

typedef struct D3D11_BOX {
    UINT left;
    UINT top;
    UINT front;
    UINT right;
    UINT bottom;
    UINT back;
} D3D11_BOX;

/* ============================================================================
    D3D11 / D3D10 COM interfaces (C-style vtbl only)
============================================================================ */

/* Forward-declares. */
typedef struct ID3D11Device               ID3D11Device;
typedef struct ID3D11DeviceContext        ID3D11DeviceContext;
typedef struct ID3D11Buffer               ID3D11Buffer;
typedef struct ID3D11Texture2D            ID3D11Texture2D;
typedef struct ID3D11Texture3D            ID3D11Texture3D;
typedef struct ID3D11Resource             ID3D11Resource;
typedef struct ID3D11View                 ID3D11View;
typedef struct ID3D11ShaderResourceView   ID3D11ShaderResourceView;
typedef struct ID3D11UnorderedAccessView  ID3D11UnorderedAccessView;
typedef struct ID3D11RenderTargetView     ID3D11RenderTargetView;
typedef struct ID3D11DepthStencilView     ID3D11DepthStencilView;
typedef struct ID3D11SamplerState         ID3D11SamplerState;
typedef struct ID3D11InputLayout          ID3D11InputLayout;
typedef struct ID3D11RasterizerState      ID3D11RasterizerState;
typedef struct ID3D11DepthStencilState    ID3D11DepthStencilState;
typedef struct ID3D11BlendState           ID3D11BlendState;
typedef struct ID3D11VertexShader         ID3D11VertexShader;
typedef struct ID3D11PixelShader          ID3D11PixelShader;
typedef struct ID3D11ComputeShader        ID3D11ComputeShader;
typedef struct ID3D11ClassInstance        ID3D11ClassInstance;
typedef struct ID3D11ClassLinkage         ID3D11ClassLinkage;
typedef struct ID3D10Blob                 ID3D10Blob;
typedef ID3D10Blob                        ID3DBlob;

/* ---- IUnknown/DeviceChild-style vtbl shared by every mock resource ----
   Each concrete interface below has a Vtbl whose first three slots are
   AddRef, Release, SetPrivateData -- matching the layout sokol_gfx.h
   requires. */
#define _D3D11_MOCK_DEVICE_CHILD_METHODS(SELF) \
    ULONG   (STDMETHODCALLTYPE *AddRef)(SELF* self); \
    ULONG   (STDMETHODCALLTYPE *Release)(SELF* self); \
    HRESULT (STDMETHODCALLTYPE *SetPrivateData)(SELF* self, REFGUID guid, UINT DataSize, const void* pData)

/* ---- ID3D11Buffer ---- */
typedef struct ID3D11BufferVtbl {
    _D3D11_MOCK_DEVICE_CHILD_METHODS(ID3D11Buffer);
} ID3D11BufferVtbl;
struct ID3D11Buffer { const ID3D11BufferVtbl* lpVtbl; };

/* ---- ID3D11Texture2D ---- */
typedef struct ID3D11Texture2DVtbl {
    _D3D11_MOCK_DEVICE_CHILD_METHODS(ID3D11Texture2D);
} ID3D11Texture2DVtbl;
struct ID3D11Texture2D { const ID3D11Texture2DVtbl* lpVtbl; };

/* ---- ID3D11Texture3D ---- */
typedef struct ID3D11Texture3DVtbl {
    _D3D11_MOCK_DEVICE_CHILD_METHODS(ID3D11Texture3D);
} ID3D11Texture3DVtbl;
struct ID3D11Texture3D { const ID3D11Texture3DVtbl* lpVtbl; };

/* ---- ID3D11Resource ---- Only needs AddRef/Release/SetPrivateData; sokol
   casts Texture2D/Tex3D/Buffer to Resource. The vtbls are laid out so the
   first three slots match. */
typedef struct ID3D11ResourceVtbl {
    _D3D11_MOCK_DEVICE_CHILD_METHODS(ID3D11Resource);
} ID3D11ResourceVtbl;
struct ID3D11Resource { const ID3D11ResourceVtbl* lpVtbl; };

/* ---- View interfaces: AddRef/Release/SetPrivateData + GetResource. ---- */
#define _D3D11_MOCK_VIEW_METHODS(SELF) \
    _D3D11_MOCK_DEVICE_CHILD_METHODS(SELF); \
    void (STDMETHODCALLTYPE *GetResource)(SELF* self, ID3D11Resource** ppResource)

typedef struct ID3D11ViewVtbl {
    _D3D11_MOCK_VIEW_METHODS(ID3D11View);
} ID3D11ViewVtbl;
struct ID3D11View { const ID3D11ViewVtbl* lpVtbl; };

typedef struct ID3D11ShaderResourceViewVtbl {
    _D3D11_MOCK_VIEW_METHODS(ID3D11ShaderResourceView);
} ID3D11ShaderResourceViewVtbl;
struct ID3D11ShaderResourceView { const ID3D11ShaderResourceViewVtbl* lpVtbl; };

typedef struct ID3D11UnorderedAccessViewVtbl {
    _D3D11_MOCK_VIEW_METHODS(ID3D11UnorderedAccessView);
} ID3D11UnorderedAccessViewVtbl;
struct ID3D11UnorderedAccessView { const ID3D11UnorderedAccessViewVtbl* lpVtbl; };

typedef struct ID3D11RenderTargetViewVtbl {
    _D3D11_MOCK_VIEW_METHODS(ID3D11RenderTargetView);
} ID3D11RenderTargetViewVtbl;
struct ID3D11RenderTargetView { const ID3D11RenderTargetViewVtbl* lpVtbl; };

typedef struct ID3D11DepthStencilViewVtbl {
    _D3D11_MOCK_VIEW_METHODS(ID3D11DepthStencilView);
} ID3D11DepthStencilViewVtbl;
struct ID3D11DepthStencilView { const ID3D11DepthStencilViewVtbl* lpVtbl; };

/* ---- Simple resource-owning objects (states, samplers, layouts, shaders) --- */
typedef struct ID3D11SamplerStateVtbl {
    _D3D11_MOCK_DEVICE_CHILD_METHODS(ID3D11SamplerState);
} ID3D11SamplerStateVtbl;
struct ID3D11SamplerState { const ID3D11SamplerStateVtbl* lpVtbl; };

typedef struct ID3D11InputLayoutVtbl {
    _D3D11_MOCK_DEVICE_CHILD_METHODS(ID3D11InputLayout);
} ID3D11InputLayoutVtbl;
struct ID3D11InputLayout { const ID3D11InputLayoutVtbl* lpVtbl; };

typedef struct ID3D11RasterizerStateVtbl {
    _D3D11_MOCK_DEVICE_CHILD_METHODS(ID3D11RasterizerState);
} ID3D11RasterizerStateVtbl;
struct ID3D11RasterizerState { const ID3D11RasterizerStateVtbl* lpVtbl; };

typedef struct ID3D11DepthStencilStateVtbl {
    _D3D11_MOCK_DEVICE_CHILD_METHODS(ID3D11DepthStencilState);
} ID3D11DepthStencilStateVtbl;
struct ID3D11DepthStencilState { const ID3D11DepthStencilStateVtbl* lpVtbl; };

typedef struct ID3D11BlendStateVtbl {
    _D3D11_MOCK_DEVICE_CHILD_METHODS(ID3D11BlendState);
} ID3D11BlendStateVtbl;
struct ID3D11BlendState { const ID3D11BlendStateVtbl* lpVtbl; };

typedef struct ID3D11VertexShaderVtbl {
    _D3D11_MOCK_DEVICE_CHILD_METHODS(ID3D11VertexShader);
} ID3D11VertexShaderVtbl;
struct ID3D11VertexShader { const ID3D11VertexShaderVtbl* lpVtbl; };

typedef struct ID3D11PixelShaderVtbl {
    _D3D11_MOCK_DEVICE_CHILD_METHODS(ID3D11PixelShader);
} ID3D11PixelShaderVtbl;
struct ID3D11PixelShader { const ID3D11PixelShaderVtbl* lpVtbl; };

typedef struct ID3D11ComputeShaderVtbl {
    _D3D11_MOCK_DEVICE_CHILD_METHODS(ID3D11ComputeShader);
} ID3D11ComputeShaderVtbl;
struct ID3D11ComputeShader { const ID3D11ComputeShaderVtbl* lpVtbl; };

/* ---- ID3D10Blob (a.k.a. ID3DBlob) ---- */
typedef struct ID3D10BlobVtbl {
    ULONG   (STDMETHODCALLTYPE *AddRef)(ID3D10Blob* self);
    ULONG   (STDMETHODCALLTYPE *Release)(ID3D10Blob* self);
    LPVOID  (STDMETHODCALLTYPE *GetBufferPointer)(ID3D10Blob* self);
    SIZE_T  (STDMETHODCALLTYPE *GetBufferSize)(ID3D10Blob* self);
} ID3D10BlobVtbl;
struct ID3D10Blob { const ID3D10BlobVtbl* lpVtbl; };

/* ---- ID3D11Device ---- */
typedef struct ID3D11DeviceVtbl {
    HRESULT (STDMETHODCALLTYPE *CheckFormatSupport)(ID3D11Device* self, DXGI_FORMAT Format, UINT* pFormatSupport);
    D3D_FEATURE_LEVEL (STDMETHODCALLTYPE *GetFeatureLevel)(ID3D11Device* self);
    HRESULT (STDMETHODCALLTYPE *CreateBuffer)(ID3D11Device* self, const D3D11_BUFFER_DESC* pDesc, const D3D11_SUBRESOURCE_DATA* pInitialData, ID3D11Buffer** ppBuffer);
    HRESULT (STDMETHODCALLTYPE *CreateTexture2D)(ID3D11Device* self, const D3D11_TEXTURE2D_DESC* pDesc, const D3D11_SUBRESOURCE_DATA* pInitialData, ID3D11Texture2D** ppTexture2D);
    HRESULT (STDMETHODCALLTYPE *CreateTexture3D)(ID3D11Device* self, const D3D11_TEXTURE3D_DESC* pDesc, const D3D11_SUBRESOURCE_DATA* pInitialData, ID3D11Texture3D** ppTexture3D);
    HRESULT (STDMETHODCALLTYPE *CreateShaderResourceView)(ID3D11Device* self, ID3D11Resource* pResource, const D3D11_SHADER_RESOURCE_VIEW_DESC* pDesc, ID3D11ShaderResourceView** ppSRView);
    HRESULT (STDMETHODCALLTYPE *CreateUnorderedAccessView)(ID3D11Device* self, ID3D11Resource* pResource, const D3D11_UNORDERED_ACCESS_VIEW_DESC* pDesc, ID3D11UnorderedAccessView** ppUAView);
    HRESULT (STDMETHODCALLTYPE *CreateRenderTargetView)(ID3D11Device* self, ID3D11Resource* pResource, const D3D11_RENDER_TARGET_VIEW_DESC* pDesc, ID3D11RenderTargetView** ppRTView);
    HRESULT (STDMETHODCALLTYPE *CreateDepthStencilView)(ID3D11Device* self, ID3D11Resource* pResource, const D3D11_DEPTH_STENCIL_VIEW_DESC* pDesc, ID3D11DepthStencilView** ppDSView);
    HRESULT (STDMETHODCALLTYPE *CreateSamplerState)(ID3D11Device* self, const D3D11_SAMPLER_DESC* pDesc, ID3D11SamplerState** ppSamplerState);
    HRESULT (STDMETHODCALLTYPE *CreateVertexShader)(ID3D11Device* self, const void* pShaderBytecode, SIZE_T BytecodeLength, ID3D11ClassLinkage* pClassLinkage, ID3D11VertexShader** ppVertexShader);
    HRESULT (STDMETHODCALLTYPE *CreatePixelShader)(ID3D11Device* self, const void* pShaderBytecode, SIZE_T BytecodeLength, ID3D11ClassLinkage* pClassLinkage, ID3D11PixelShader** ppPixelShader);
    HRESULT (STDMETHODCALLTYPE *CreateComputeShader)(ID3D11Device* self, const void* pShaderBytecode, SIZE_T BytecodeLength, ID3D11ClassLinkage* pClassLinkage, ID3D11ComputeShader** ppComputeShader);
    HRESULT (STDMETHODCALLTYPE *CreateInputLayout)(ID3D11Device* self, const D3D11_INPUT_ELEMENT_DESC* pInputElementDescs, UINT NumElements, const void* pShaderBytecodeWithInputSignature, SIZE_T BytecodeLength, ID3D11InputLayout** ppInputLayout);
    HRESULT (STDMETHODCALLTYPE *CreateRasterizerState)(ID3D11Device* self, const D3D11_RASTERIZER_DESC* pDesc, ID3D11RasterizerState** ppRasterizerState);
    HRESULT (STDMETHODCALLTYPE *CreateDepthStencilState)(ID3D11Device* self, const D3D11_DEPTH_STENCIL_DESC* pDesc, ID3D11DepthStencilState** ppDepthStencilState);
    HRESULT (STDMETHODCALLTYPE *CreateBlendState)(ID3D11Device* self, const D3D11_BLEND_DESC* pDesc, ID3D11BlendState** ppBlendState);
    ULONG   (STDMETHODCALLTYPE *AddRef)(ID3D11Device* self);
    ULONG   (STDMETHODCALLTYPE *Release)(ID3D11Device* self);
} ID3D11DeviceVtbl;
struct ID3D11Device { const ID3D11DeviceVtbl* lpVtbl; };

/* ---- ID3D11DeviceContext ---- */
typedef struct ID3D11DeviceContextVtbl {
    void    (STDMETHODCALLTYPE *ClearState)(ID3D11DeviceContext* self);
    void    (STDMETHODCALLTYPE *OMSetRenderTargets)(ID3D11DeviceContext* self, UINT NumViews, ID3D11RenderTargetView* const* ppRenderTargetViews, ID3D11DepthStencilView* pDepthStencilView);
    void    (STDMETHODCALLTYPE *RSSetState)(ID3D11DeviceContext* self, ID3D11RasterizerState* pRS);
    void    (STDMETHODCALLTYPE *OMSetDepthStencilState)(ID3D11DeviceContext* self, ID3D11DepthStencilState* pDSS, UINT StencilRef);
    void    (STDMETHODCALLTYPE *OMSetBlendState)(ID3D11DeviceContext* self, ID3D11BlendState* pBS, const FLOAT BlendFactor[4], UINT SampleMask);
    void    (STDMETHODCALLTYPE *IASetVertexBuffers)(ID3D11DeviceContext* self, UINT StartSlot, UINT NumBuffers, ID3D11Buffer* const* ppVertexBuffers, const UINT* pStrides, const UINT* pOffsets);
    void    (STDMETHODCALLTYPE *IASetIndexBuffer)(ID3D11DeviceContext* self, ID3D11Buffer* pIndexBuffer, DXGI_FORMAT Format, UINT Offset);
    void    (STDMETHODCALLTYPE *IASetInputLayout)(ID3D11DeviceContext* self, ID3D11InputLayout* pInputLayout);
    void    (STDMETHODCALLTYPE *VSSetShader)(ID3D11DeviceContext* self, ID3D11VertexShader* pShader, ID3D11ClassInstance* const* ppInst, UINT NumInst);
    void    (STDMETHODCALLTYPE *PSSetShader)(ID3D11DeviceContext* self, ID3D11PixelShader* pShader, ID3D11ClassInstance* const* ppInst, UINT NumInst);
    void    (STDMETHODCALLTYPE *CSSetShader)(ID3D11DeviceContext* self, ID3D11ComputeShader* pShader, ID3D11ClassInstance* const* ppInst, UINT NumInst);
    void    (STDMETHODCALLTYPE *VSSetConstantBuffers)(ID3D11DeviceContext* self, UINT StartSlot, UINT NumBuffers, ID3D11Buffer* const* ppConstantBuffers);
    void    (STDMETHODCALLTYPE *PSSetConstantBuffers)(ID3D11DeviceContext* self, UINT StartSlot, UINT NumBuffers, ID3D11Buffer* const* ppConstantBuffers);
    void    (STDMETHODCALLTYPE *CSSetConstantBuffers)(ID3D11DeviceContext* self, UINT StartSlot, UINT NumBuffers, ID3D11Buffer* const* ppConstantBuffers);
    void    (STDMETHODCALLTYPE *VSSetShaderResources)(ID3D11DeviceContext* self, UINT StartSlot, UINT NumViews, ID3D11ShaderResourceView* const* ppSRVs);
    void    (STDMETHODCALLTYPE *PSSetShaderResources)(ID3D11DeviceContext* self, UINT StartSlot, UINT NumViews, ID3D11ShaderResourceView* const* ppSRVs);
    void    (STDMETHODCALLTYPE *CSSetShaderResources)(ID3D11DeviceContext* self, UINT StartSlot, UINT NumViews, ID3D11ShaderResourceView* const* ppSRVs);
    void    (STDMETHODCALLTYPE *VSSetSamplers)(ID3D11DeviceContext* self, UINT StartSlot, UINT NumSamplers, ID3D11SamplerState* const* ppSamplers);
    void    (STDMETHODCALLTYPE *PSSetSamplers)(ID3D11DeviceContext* self, UINT StartSlot, UINT NumSamplers, ID3D11SamplerState* const* ppSamplers);
    void    (STDMETHODCALLTYPE *CSSetSamplers)(ID3D11DeviceContext* self, UINT StartSlot, UINT NumSamplers, ID3D11SamplerState* const* ppSamplers);
    void    (STDMETHODCALLTYPE *CSSetUnorderedAccessViews)(ID3D11DeviceContext* self, UINT StartSlot, UINT NumUAVs, ID3D11UnorderedAccessView* const* ppUAVs, const UINT* pUAVInitialCounts);
    void    (STDMETHODCALLTYPE *RSSetViewports)(ID3D11DeviceContext* self, UINT NumViewports, const D3D11_VIEWPORT* pViewports);
    void    (STDMETHODCALLTYPE *RSSetScissorRects)(ID3D11DeviceContext* self, UINT NumRects, const D3D11_RECT* pRects);
    void    (STDMETHODCALLTYPE *ClearRenderTargetView)(ID3D11DeviceContext* self, ID3D11RenderTargetView* pRTV, const FLOAT ColorRGBA[4]);
    void    (STDMETHODCALLTYPE *ClearDepthStencilView)(ID3D11DeviceContext* self, ID3D11DepthStencilView* pDSV, UINT ClearFlags, FLOAT Depth, UINT8 Stencil);
    void    (STDMETHODCALLTYPE *ResolveSubresource)(ID3D11DeviceContext* self, ID3D11Resource* pDst, UINT DstSubres, ID3D11Resource* pSrc, UINT SrcSubres, DXGI_FORMAT Format);
    void    (STDMETHODCALLTYPE *IASetPrimitiveTopology)(ID3D11DeviceContext* self, D3D11_PRIMITIVE_TOPOLOGY Topology);
    void    (STDMETHODCALLTYPE *UpdateSubresource)(ID3D11DeviceContext* self, ID3D11Resource* pDst, UINT DstSubres, const D3D11_BOX* pDstBox, const void* pSrcData, UINT SrcRowPitch, UINT SrcDepthPitch);
    void    (STDMETHODCALLTYPE *DrawIndexed)(ID3D11DeviceContext* self, UINT IndexCount, UINT StartIndexLocation, INT BaseVertexLocation);
    void    (STDMETHODCALLTYPE *DrawIndexedInstanced)(ID3D11DeviceContext* self, UINT IndexCountPerInstance, UINT InstanceCount, UINT StartIndexLocation, INT BaseVertexLocation, UINT StartInstanceLocation);
    void    (STDMETHODCALLTYPE *Draw)(ID3D11DeviceContext* self, UINT VertexCount, UINT StartVertexLocation);
    void    (STDMETHODCALLTYPE *DrawInstanced)(ID3D11DeviceContext* self, UINT VertexCountPerInstance, UINT InstanceCount, UINT StartVertexLocation, UINT StartInstanceLocation);
    void    (STDMETHODCALLTYPE *Dispatch)(ID3D11DeviceContext* self, UINT X, UINT Y, UINT Z);
    HRESULT (STDMETHODCALLTYPE *Map)(ID3D11DeviceContext* self, ID3D11Resource* pResource, UINT Subresource, D3D11_MAP MapType, UINT MapFlags, D3D11_MAPPED_SUBRESOURCE* pMapped);
    void    (STDMETHODCALLTYPE *Unmap)(ID3D11DeviceContext* self, ID3D11Resource* pResource, UINT Subresource);
    ULONG   (STDMETHODCALLTYPE *AddRef)(ID3D11DeviceContext* self);
    ULONG   (STDMETHODCALLTYPE *Release)(ID3D11DeviceContext* self);
} ID3D11DeviceContextVtbl;
struct ID3D11DeviceContext { const ID3D11DeviceContextVtbl* lpVtbl; };

#ifdef __cplusplus
}
#endif

#endif /* MOCK_D3D11_H_INCLUDED */
