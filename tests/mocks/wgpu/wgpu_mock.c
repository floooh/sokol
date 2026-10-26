/*
    LLM maintained.

    wgpu_mock.c -- implementation of the mocked WebGPU runtime, see
    wgpu_mock.h for the public API and the validation rules which are checked.

    Design notes:

    - every WebGPU object is a heap allocated 'obj_t' registered in a global
      registry, the opaque WebGPU handles are the obj_t pointers
    - objects are only freed in wgpu_mock_shutdown(), a released object stays
      inspectable and is flagged as not alive
    - objects which keep other objects alive in real WebGPU (views, bind
      groups, pipeline layouts, pipelines) hold a reference in obj_t.refs[]
    - creation functions never return NULL because of a validation error
      (like in real WebGPU, where an invalid object is returned), they only
      record a message. NULL is only returned by fault injection.
*/
#include "wgpu_mock.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdarg.h>
#include <ctype.h>
#include <math.h>

#if defined(__GNUC__) || defined(__clang__)
#define WM_PRINTF(a, b) __attribute__((format(printf, a, b)))
#else
#define WM_PRINTF(a, b)
#endif

#define WM_MAGIC (0x57474D4Bu)
#define WM_MAX_REFS (WGPU_MOCK_MAX_BIND_ENTRIES + 4)
#define WM_MAX_FEATURES (32)
#define WM_NUM_STAGES (3)

typedef struct obj_t obj_t;
struct obj_t {
    uint32_t magic;
    wgpu_mock_obj_t kind;
    int refcount;
    bool alive;
    int num_refs;
    obj_t* refs[WM_MAX_REFS];
    union {
        struct { wgpu_mock_buffer_info_t info; uint8_t* data; } buffer;
        struct { wgpu_mock_texture_info_t info; } texture;
        struct { wgpu_mock_texture_view_info_t info; WGPUTextureUsage usage; } view;
        struct { wgpu_mock_sampler_info_t info; } sampler;
        struct { wgpu_mock_shader_module_info_t info; char* code; } shader;
        struct { wgpu_mock_bind_group_layout_info_t info; } bgl;
        struct { wgpu_mock_bind_group_info_t info; } bg;
        struct { wgpu_mock_pipeline_layout_info_t info; } pl;
        struct { wgpu_mock_render_pipeline_info_t info; } rp;
        struct { wgpu_mock_compute_pipeline_info_t info; } cp;
        struct { bool finished; bool pass_open; } enc;
        struct { bool submitted; } cmdbuf;
        struct { obj_t* enc; bool ended; } pass;
    } u;
};

#define WM_PSIZE(m) sizeof(((obj_t*)0)->u.m)

// texture format info
#define F_COMP    (1)
#define F_DEPTH   (2)
#define F_STENCIL (4)
#define F_UINT    (8)
#define F_SINT    (16)
#define F_F32     (32)

typedef struct {
    WGPUTextureFormat fmt;
    WGPUTextureFormat base;     // the non-sRGB variant
    uint32_t bw, bh;            // block size in pixels
    uint32_t bytes;             // bytes per block
    int flags;
} fmt_info_t;

static const fmt_info_t g_fmt_table[] = {
    { WGPUTextureFormat_R8Unorm, WGPUTextureFormat_R8Unorm, 1, 1, 1, 0 },
    { WGPUTextureFormat_R8Snorm, WGPUTextureFormat_R8Snorm, 1, 1, 1, 0 },
    { WGPUTextureFormat_R8Uint, WGPUTextureFormat_R8Uint, 1, 1, 1, F_UINT },
    { WGPUTextureFormat_R8Sint, WGPUTextureFormat_R8Sint, 1, 1, 1, F_SINT },
    { WGPUTextureFormat_R16Unorm, WGPUTextureFormat_R16Unorm, 1, 1, 2, 0 },
    { WGPUTextureFormat_R16Snorm, WGPUTextureFormat_R16Snorm, 1, 1, 2, 0 },
    { WGPUTextureFormat_R16Uint, WGPUTextureFormat_R16Uint, 1, 1, 2, F_UINT },
    { WGPUTextureFormat_R16Sint, WGPUTextureFormat_R16Sint, 1, 1, 2, F_SINT },
    { WGPUTextureFormat_R16Float, WGPUTextureFormat_R16Float, 1, 1, 2, 0 },
    { WGPUTextureFormat_RG8Unorm, WGPUTextureFormat_RG8Unorm, 1, 1, 2, 0 },
    { WGPUTextureFormat_RG8Snorm, WGPUTextureFormat_RG8Snorm, 1, 1, 2, 0 },
    { WGPUTextureFormat_RG8Uint, WGPUTextureFormat_RG8Uint, 1, 1, 2, F_UINT },
    { WGPUTextureFormat_RG8Sint, WGPUTextureFormat_RG8Sint, 1, 1, 2, F_SINT },
    { WGPUTextureFormat_R32Float, WGPUTextureFormat_R32Float, 1, 1, 4, F_F32 },
    { WGPUTextureFormat_R32Uint, WGPUTextureFormat_R32Uint, 1, 1, 4, F_UINT },
    { WGPUTextureFormat_R32Sint, WGPUTextureFormat_R32Sint, 1, 1, 4, F_SINT },
    { WGPUTextureFormat_RG16Unorm, WGPUTextureFormat_RG16Unorm, 1, 1, 4, 0 },
    { WGPUTextureFormat_RG16Snorm, WGPUTextureFormat_RG16Snorm, 1, 1, 4, 0 },
    { WGPUTextureFormat_RG16Uint, WGPUTextureFormat_RG16Uint, 1, 1, 4, F_UINT },
    { WGPUTextureFormat_RG16Sint, WGPUTextureFormat_RG16Sint, 1, 1, 4, F_SINT },
    { WGPUTextureFormat_RG16Float, WGPUTextureFormat_RG16Float, 1, 1, 4, 0 },
    { WGPUTextureFormat_RGBA8Unorm, WGPUTextureFormat_RGBA8Unorm, 1, 1, 4, 0 },
    { WGPUTextureFormat_RGBA8UnormSrgb, WGPUTextureFormat_RGBA8Unorm, 1, 1, 4, 0 },
    { WGPUTextureFormat_RGBA8Snorm, WGPUTextureFormat_RGBA8Snorm, 1, 1, 4, 0 },
    { WGPUTextureFormat_RGBA8Uint, WGPUTextureFormat_RGBA8Uint, 1, 1, 4, F_UINT },
    { WGPUTextureFormat_RGBA8Sint, WGPUTextureFormat_RGBA8Sint, 1, 1, 4, F_SINT },
    { WGPUTextureFormat_BGRA8Unorm, WGPUTextureFormat_BGRA8Unorm, 1, 1, 4, 0 },
    { WGPUTextureFormat_BGRA8UnormSrgb, WGPUTextureFormat_BGRA8Unorm, 1, 1, 4, 0 },
    { WGPUTextureFormat_RGB10A2Unorm, WGPUTextureFormat_RGB10A2Unorm, 1, 1, 4, 0 },
    { WGPUTextureFormat_RG11B10Ufloat, WGPUTextureFormat_RG11B10Ufloat, 1, 1, 4, 0 },
    { WGPUTextureFormat_RGB9E5Ufloat, WGPUTextureFormat_RGB9E5Ufloat, 1, 1, 4, 0 },
    { WGPUTextureFormat_RG32Float, WGPUTextureFormat_RG32Float, 1, 1, 8, F_F32 },
    { WGPUTextureFormat_RG32Uint, WGPUTextureFormat_RG32Uint, 1, 1, 8, F_UINT },
    { WGPUTextureFormat_RG32Sint, WGPUTextureFormat_RG32Sint, 1, 1, 8, F_SINT },
    { WGPUTextureFormat_RGBA16Unorm, WGPUTextureFormat_RGBA16Unorm, 1, 1, 8, 0 },
    { WGPUTextureFormat_RGBA16Snorm, WGPUTextureFormat_RGBA16Snorm, 1, 1, 8, 0 },
    { WGPUTextureFormat_RGBA16Uint, WGPUTextureFormat_RGBA16Uint, 1, 1, 8, F_UINT },
    { WGPUTextureFormat_RGBA16Sint, WGPUTextureFormat_RGBA16Sint, 1, 1, 8, F_SINT },
    { WGPUTextureFormat_RGBA16Float, WGPUTextureFormat_RGBA16Float, 1, 1, 8, 0 },
    { WGPUTextureFormat_RGBA32Float, WGPUTextureFormat_RGBA32Float, 1, 1, 16, F_F32 },
    { WGPUTextureFormat_RGBA32Uint, WGPUTextureFormat_RGBA32Uint, 1, 1, 16, F_UINT },
    { WGPUTextureFormat_RGBA32Sint, WGPUTextureFormat_RGBA32Sint, 1, 1, 16, F_SINT },
    { WGPUTextureFormat_Depth32Float, WGPUTextureFormat_Depth32Float, 1, 1, 4, F_DEPTH },
    { WGPUTextureFormat_Depth32FloatStencil8, WGPUTextureFormat_Depth32FloatStencil8, 1, 1, 8, F_DEPTH|F_STENCIL },
    { WGPUTextureFormat_BC1RGBAUnorm, WGPUTextureFormat_BC1RGBAUnorm, 4, 4, 8, F_COMP },
    { WGPUTextureFormat_BC2RGBAUnorm, WGPUTextureFormat_BC2RGBAUnorm, 4, 4, 16, F_COMP },
    { WGPUTextureFormat_BC3RGBAUnorm, WGPUTextureFormat_BC3RGBAUnorm, 4, 4, 16, F_COMP },
    { WGPUTextureFormat_BC3RGBAUnormSrgb, WGPUTextureFormat_BC3RGBAUnorm, 4, 4, 16, F_COMP },
    { WGPUTextureFormat_BC4RUnorm, WGPUTextureFormat_BC4RUnorm, 4, 4, 8, F_COMP },
    { WGPUTextureFormat_BC4RSnorm, WGPUTextureFormat_BC4RSnorm, 4, 4, 8, F_COMP },
    { WGPUTextureFormat_BC5RGUnorm, WGPUTextureFormat_BC5RGUnorm, 4, 4, 16, F_COMP },
    { WGPUTextureFormat_BC5RGSnorm, WGPUTextureFormat_BC5RGSnorm, 4, 4, 16, F_COMP },
    { WGPUTextureFormat_BC6HRGBUfloat, WGPUTextureFormat_BC6HRGBUfloat, 4, 4, 16, F_COMP },
    { WGPUTextureFormat_BC6HRGBFloat, WGPUTextureFormat_BC6HRGBFloat, 4, 4, 16, F_COMP },
    { WGPUTextureFormat_BC7RGBAUnorm, WGPUTextureFormat_BC7RGBAUnorm, 4, 4, 16, F_COMP },
    { WGPUTextureFormat_BC7RGBAUnormSrgb, WGPUTextureFormat_BC7RGBAUnorm, 4, 4, 16, F_COMP },
    { WGPUTextureFormat_ETC2RGB8Unorm, WGPUTextureFormat_ETC2RGB8Unorm, 4, 4, 8, F_COMP },
    { WGPUTextureFormat_ETC2RGB8UnormSrgb, WGPUTextureFormat_ETC2RGB8Unorm, 4, 4, 8, F_COMP },
    { WGPUTextureFormat_ETC2RGB8A1Unorm, WGPUTextureFormat_ETC2RGB8A1Unorm, 4, 4, 8, F_COMP },
    { WGPUTextureFormat_ETC2RGBA8Unorm, WGPUTextureFormat_ETC2RGBA8Unorm, 4, 4, 16, F_COMP },
    { WGPUTextureFormat_ETC2RGBA8UnormSrgb, WGPUTextureFormat_ETC2RGBA8Unorm, 4, 4, 16, F_COMP },
    { WGPUTextureFormat_EACR11Unorm, WGPUTextureFormat_EACR11Unorm, 4, 4, 8, F_COMP },
    { WGPUTextureFormat_EACR11Snorm, WGPUTextureFormat_EACR11Snorm, 4, 4, 8, F_COMP },
    { WGPUTextureFormat_EACRG11Unorm, WGPUTextureFormat_EACRG11Unorm, 4, 4, 16, F_COMP },
    { WGPUTextureFormat_EACRG11Snorm, WGPUTextureFormat_EACRG11Snorm, 4, 4, 16, F_COMP },
    { WGPUTextureFormat_ASTC4x4Unorm, WGPUTextureFormat_ASTC4x4Unorm, 4, 4, 16, F_COMP },
    { WGPUTextureFormat_ASTC4x4UnormSrgb, WGPUTextureFormat_ASTC4x4Unorm, 4, 4, 16, F_COMP },
};

// global state
static obj_t** g_reg;
static int g_reg_num;
static int g_reg_cap;
static int g_live[WGPU_MOCK_OBJ_NUM];
static int g_created[WGPU_MOCK_OBJ_NUM];
static struct { int skip; int count; } g_fail[WGPU_MOCK_OBJ_NUM];
static WGPUFeatureName g_features[WM_MAX_FEATURES];
static int g_num_features;
static WGPULimits g_limits;
static obj_t* g_device;
static obj_t* g_queue;

static wgpu_mock_call_t g_calls[WGPU_MOCK_MAX_CALLS];
static int g_num_calls;
static bool g_call_overflow;
static wgpu_mock_call_t g_scratch_call;

static char g_errors[WGPU_MOCK_MAX_ERRORS][WGPU_MOCK_MAX_ERROR_STRING];
static int g_num_errors;
static int g_total_errors;

static wgpu_mock_render_pass_info_t g_last_render_pass;
static wgpu_mock_render_pass_encoder_state_t g_rpe;
static wgpu_mock_compute_pass_encoder_state_t g_cpe;
static wgpu_mock_command_encoder_state_t g_cmd;
static wgpu_mock_queue_state_t g_queue_state;
static obj_t* g_cur_rpe;
static obj_t* g_cur_cpe;

// attachment setup of the current render pass, used to validate pipelines and scissor rects
static struct {
    int num_color;
    WGPUTextureFormat color_fmt[WGPU_MOCK_MAX_COLOR_ATTACHMENTS];
    WGPUTextureFormat ds_fmt;
    uint32_t sample_count;
    uint32_t width, height;
} g_rp;

// ---- messages
static void err(const char* fmt, ...) WM_PRINTF(1, 2);
static void err(const char* fmt, ...) {
    g_total_errors++;
    if (g_num_errors < WGPU_MOCK_MAX_ERRORS) {
        va_list args;
        va_start(args, fmt);
        vsnprintf(g_errors[g_num_errors], WGPU_MOCK_MAX_ERROR_STRING, fmt, args);
        va_end(args);
        g_num_errors++;
    }
}

#define LL(x) ((unsigned long long)(x))

static const char* kind_name(wgpu_mock_obj_t k) {
    switch (k) {
        case WGPU_MOCK_OBJ_BUFFER: return "buffer";
        case WGPU_MOCK_OBJ_TEXTURE: return "texture";
        case WGPU_MOCK_OBJ_TEXTURE_VIEW: return "texture view";
        case WGPU_MOCK_OBJ_SAMPLER: return "sampler";
        case WGPU_MOCK_OBJ_SHADER_MODULE: return "shader module";
        case WGPU_MOCK_OBJ_BIND_GROUP_LAYOUT: return "bind group layout";
        case WGPU_MOCK_OBJ_BIND_GROUP: return "bind group";
        case WGPU_MOCK_OBJ_PIPELINE_LAYOUT: return "pipeline layout";
        case WGPU_MOCK_OBJ_RENDER_PIPELINE: return "render pipeline";
        case WGPU_MOCK_OBJ_COMPUTE_PIPELINE: return "compute pipeline";
        case WGPU_MOCK_OBJ_COMMAND_ENCODER: return "command encoder";
        case WGPU_MOCK_OBJ_COMMAND_BUFFER: return "command buffer";
        case WGPU_MOCK_OBJ_RENDER_PASS_ENCODER: return "render pass encoder";
        case WGPU_MOCK_OBJ_COMPUTE_PASS_ENCODER: return "compute pass encoder";
        case WGPU_MOCK_OBJ_QUEUE: return "queue";
        case WGPU_MOCK_OBJ_DEVICE: return "device";
        default: return "?";
    }
}

// ---- call log
static wgpu_mock_call_t* begin_call(wgpu_mock_func_t func, const void* obj) {
    wgpu_mock_call_t* c;
    if (g_num_calls >= WGPU_MOCK_MAX_CALLS) {
        g_call_overflow = true;
        c = &g_scratch_call;
    } else {
        c = &g_calls[g_num_calls++];
    }
    memset(c, 0, sizeof(*c));
    c->func = func;
    c->obj = obj;
    return c;
}

static wgpu_mock_arg_t* next_arg(wgpu_mock_call_t* c) {
    if (c->num_args >= WGPU_MOCK_MAX_CALL_ARGS) {
        return &c->args[WGPU_MOCK_MAX_CALL_ARGS - 1];
    }
    return &c->args[c->num_args++];
}
static void arg_u(wgpu_mock_call_t* c, uint64_t v) { next_arg(c)->u = v; }
static void arg_i(wgpu_mock_call_t* c, int64_t v) { next_arg(c)->i = v; }
static void arg_f(wgpu_mock_call_t* c, double v) { next_arg(c)->f = v; }
static void arg_p(wgpu_mock_call_t* c, const void* v) { next_arg(c)->p = v; }

// ---- fault injection
static bool should_fail(wgpu_mock_obj_t kind) {
    if (g_fail[kind].count <= 0) {
        return false;
    }
    if (g_fail[kind].skip > 0) {
        g_fail[kind].skip--;
        return false;
    }
    g_fail[kind].count--;
    return true;
}

// ---- features and limits
static bool has_feature(WGPUFeatureName f) {
    for (int i = 0; i < g_num_features; i++) {
        if (g_features[i] == f) {
            return true;
        }
    }
    return false;
}

static void default_limits(WGPULimits* l) {
    memset(l, 0, sizeof(*l));
    l->maxTextureDimension1D = 8192;
    l->maxTextureDimension2D = 8192;
    l->maxTextureDimension3D = 2048;
    l->maxTextureArrayLayers = 256;
    l->maxBindGroups = 4;
    l->maxBindGroupsPlusVertexBuffers = 24;
    l->maxBindingsPerBindGroup = 1000;
    l->maxDynamicUniformBuffersPerPipelineLayout = 8;
    l->maxDynamicStorageBuffersPerPipelineLayout = 4;
    l->maxSampledTexturesPerShaderStage = 16;
    l->maxSamplersPerShaderStage = 16;
    l->maxStorageBuffersPerShaderStage = 8;
    l->maxStorageTexturesPerShaderStage = 4;
    l->maxUniformBuffersPerShaderStage = 12;
    l->maxUniformBufferBindingSize = 65536;
    l->maxStorageBufferBindingSize = 134217728;
    l->minUniformBufferOffsetAlignment = 256;
    l->minStorageBufferOffsetAlignment = 256;
    l->maxVertexBuffers = 8;
    l->maxBufferSize = 268435456;
    l->maxVertexAttributes = 16;
    l->maxVertexBufferArrayStride = 2048;
    l->maxInterStageShaderVariables = 16;
    l->maxColorAttachments = 8;
    l->maxColorAttachmentBytesPerSample = 32;
    l->maxComputeWorkgroupStorageSize = 16384;
    l->maxComputeInvocationsPerWorkgroup = 256;
    l->maxComputeWorkgroupSizeX = 256;
    l->maxComputeWorkgroupSizeY = 256;
    l->maxComputeWorkgroupSizeZ = 64;
    l->maxComputeWorkgroupsPerDimension = 65535;
    l->maxImmediateSize = 0;
}

// ---- formats
static const fmt_info_t* fmt_info(WGPUTextureFormat f) {
    for (size_t i = 0; i < sizeof(g_fmt_table) / sizeof(g_fmt_table[0]); i++) {
        if (g_fmt_table[i].fmt == f) {
            return &g_fmt_table[i];
        }
    }
    return 0;
}

static bool fmt_is_norm16(WGPUTextureFormat f) {
    return (f == WGPUTextureFormat_R16Unorm) || (f == WGPUTextureFormat_R16Snorm) ||
           (f == WGPUTextureFormat_RG16Unorm) || (f == WGPUTextureFormat_RG16Snorm) ||
           (f == WGPUTextureFormat_RGBA16Unorm) || (f == WGPUTextureFormat_RGBA16Snorm);
}

static bool fmt_is_snorm8(WGPUTextureFormat f) {
    return (f == WGPUTextureFormat_R8Snorm) || (f == WGPUTextureFormat_RG8Snorm) || (f == WGPUTextureFormat_RGBA8Snorm);
}

// returns the feature required to create a texture of this format, or 0
static WGPUFeatureName fmt_required_feature(WGPUTextureFormat f) {
    const fmt_info_t* fi = fmt_info(f);
    if (!fi) {
        return (WGPUFeatureName)0;
    }
    if (f == WGPUTextureFormat_Depth32FloatStencil8) {
        return WGPUFeatureName_Depth32FloatStencil8;
    }
    if (fmt_is_norm16(f)) {
        return WGPUFeatureName_TextureFormatsTier1;
    }
    if (fi->flags & F_COMP) {
        if (f >= WGPUTextureFormat_ASTC4x4Unorm) {
            return WGPUFeatureName_TextureCompressionASTC;
        }
        if (f >= WGPUTextureFormat_ETC2RGB8Unorm) {
            return WGPUFeatureName_TextureCompressionETC2;
        }
        return WGPUFeatureName_TextureCompressionBC;
    }
    return (WGPUFeatureName)0;
}

static bool fmt_supported(WGPUTextureFormat f) {
    WGPUFeatureName req = fmt_required_feature(f);
    return (req == 0) || has_feature(req);
}

static bool fmt_renderable(WGPUTextureFormat f) {
    const fmt_info_t* fi = fmt_info(f);
    if (!fi || (fi->flags & F_COMP)) {
        return false;
    }
    if (f == WGPUTextureFormat_RGB9E5Ufloat) {
        return false;
    }
    if (f == WGPUTextureFormat_RG11B10Ufloat) {
        return has_feature(WGPUFeatureName_RG11B10UfloatRenderable);
    }
    if (fmt_is_snorm8(f)) {
        return has_feature(WGPUFeatureName_TextureFormatsTier1);
    }
    return true;
}

static bool fmt_blendable(WGPUTextureFormat f) {
    const fmt_info_t* fi = fmt_info(f);
    if (!fi || !fmt_renderable(f)) {
        return false;
    }
    if (fi->flags & (F_UINT | F_SINT | F_DEPTH | F_STENCIL)) {
        return false;
    }
    if (fi->flags & F_F32) {
        return has_feature(WGPUFeatureName_Float32Blendable);
    }
    return true;
}

static uint32_t vertex_format_size(WGPUVertexFormat f) {
    switch (f) {
        case WGPUVertexFormat_Uint8x4: case WGPUVertexFormat_Sint8x4:
        case WGPUVertexFormat_Unorm8x4: case WGPUVertexFormat_Snorm8x4:
        case WGPUVertexFormat_Uint16x2: case WGPUVertexFormat_Sint16x2:
        case WGPUVertexFormat_Unorm16x2: case WGPUVertexFormat_Snorm16x2:
        case WGPUVertexFormat_Float16x2: case WGPUVertexFormat_Float32:
        case WGPUVertexFormat_Uint32: case WGPUVertexFormat_Sint32:
        case WGPUVertexFormat_Unorm10_10_10_2:
            return 4;
        case WGPUVertexFormat_Uint16x4: case WGPUVertexFormat_Sint16x4:
        case WGPUVertexFormat_Unorm16x4: case WGPUVertexFormat_Snorm16x4:
        case WGPUVertexFormat_Float16x4: case WGPUVertexFormat_Float32x2:
        case WGPUVertexFormat_Uint32x2: case WGPUVertexFormat_Sint32x2:
            return 8;
        case WGPUVertexFormat_Float32x3: case WGPUVertexFormat_Uint32x3: case WGPUVertexFormat_Sint32x3:
            return 12;
        case WGPUVertexFormat_Float32x4: case WGPUVertexFormat_Uint32x4: case WGPUVertexFormat_Sint32x4:
            return 16;
        default:
            return 0;
    }
}

// ---- small helpers
static uint32_t u32_max(uint32_t a, uint32_t b) { return (a > b) ? a : b; }
static uint32_t u32_min(uint32_t a, uint32_t b) { return (a < b) ? a : b; }

static uint32_t mip_dim(uint32_t v, uint32_t level) {
    return u32_max(1, (level < 32) ? (v >> level) : 0);
}

static uint32_t roundup_u32(uint32_t v, uint32_t align) {
    return ((v + align - 1) / align) * align;
}

static void sv_copy(char* dst, size_t cap, WGPUStringView sv) {
    dst[0] = 0;
    if (!sv.data) {
        return;
    }
    size_t len = (sv.length == WGPU_STRLEN) ? strlen(sv.data) : sv.length;
    if (len >= cap) {
        len = cap - 1;
    }
    memcpy(dst, sv.data, len);
    dst[len] = 0;
}

static bool is_ident_char(char c) {
    return isalnum((unsigned char)c) || (c == '_');
}

static bool is_space_char(char c) {
    return (c == ' ') || (c == '\t') || (c == '\n') || (c == '\r');
}

// true if the code contains a function declaration 'fn <name>'
static bool code_has_entry(const char* code, const char* name) {
    size_t n = strlen(name);
    if (!code || (n == 0)) {
        return false;
    }
    const char* p = code;
    while ((p = strstr(p, name)) != 0) {
        if (!is_ident_char(p[n])) {
            const char* q = p;
            while ((q > code) && is_space_char(q[-1])) {
                q--;
            }
            if ((q != p) && ((q - code) >= 2) && (q[-2] == 'f') && (q[-1] == 'n') &&
                (((q - 2) == code) || !is_ident_char(q[-3])))
            {
                return true;
            }
        }
        p++;
    }
    return false;
}

// ---- object registry
static size_t payload_size(wgpu_mock_obj_t kind) {
    switch (kind) {
        case WGPU_MOCK_OBJ_BUFFER: return WM_PSIZE(buffer);
        case WGPU_MOCK_OBJ_TEXTURE: return WM_PSIZE(texture);
        case WGPU_MOCK_OBJ_TEXTURE_VIEW: return WM_PSIZE(view);
        case WGPU_MOCK_OBJ_SAMPLER: return WM_PSIZE(sampler);
        case WGPU_MOCK_OBJ_SHADER_MODULE: return WM_PSIZE(shader);
        case WGPU_MOCK_OBJ_BIND_GROUP_LAYOUT: return WM_PSIZE(bgl);
        case WGPU_MOCK_OBJ_BIND_GROUP: return WM_PSIZE(bg);
        case WGPU_MOCK_OBJ_PIPELINE_LAYOUT: return WM_PSIZE(pl);
        case WGPU_MOCK_OBJ_RENDER_PIPELINE: return WM_PSIZE(rp);
        case WGPU_MOCK_OBJ_COMPUTE_PIPELINE: return WM_PSIZE(cp);
        case WGPU_MOCK_OBJ_COMMAND_ENCODER: return WM_PSIZE(enc);
        case WGPU_MOCK_OBJ_COMMAND_BUFFER: return WM_PSIZE(cmdbuf);
        case WGPU_MOCK_OBJ_RENDER_PASS_ENCODER:
        case WGPU_MOCK_OBJ_COMPUTE_PASS_ENCODER: return WM_PSIZE(pass);
        default: return 0;
    }
}

static obj_t* obj_new(wgpu_mock_obj_t kind) {
    size_t sz = offsetof(obj_t, u) + payload_size(kind);
    obj_t* o = (obj_t*)calloc(1, sz);
    if (!o) {
        fprintf(stderr, "wgpu_mock: out of memory\n");
        abort();
    }
    o->magic = WM_MAGIC;
    o->kind = kind;
    o->refcount = 1;
    o->alive = true;
    if (g_reg_num == g_reg_cap) {
        g_reg_cap = (g_reg_cap == 0) ? 256 : (g_reg_cap * 2);
        g_reg = (obj_t**)realloc(g_reg, (size_t)g_reg_cap * sizeof(obj_t*));
        if (!g_reg) {
            fprintf(stderr, "wgpu_mock: out of memory\n");
            abort();
        }
    }
    g_reg[g_reg_num++] = o;
    g_live[kind]++;
    g_created[kind]++;
    return o;
}

// find a handle in the registry without dereferencing it
static obj_t* find_obj(const void* h) {
    for (int i = 0; i < g_reg_num; i++) {
        if ((const void*)g_reg[i] == h) {
            return g_reg[i];
        }
    }
    return 0;
}

// validate a handle for use in a call, records a message and returns NULL if invalid
static obj_t* lookup(const void* h, wgpu_mock_obj_t kind, const char* fn) {
    obj_t* o = (obj_t*)h;
    if (!o) {
        err("%s: %s handle is null", fn, kind_name(kind));
        return 0;
    }
    if ((o->magic != WM_MAGIC) || (o->kind != kind)) {
        err("%s: invalid %s handle", fn, kind_name(kind));
        return 0;
    }
    if (!o->alive) {
        err("%s: use of released %s", fn, kind_name(kind));
        return 0;
    }
    return o;
}

static void add_ref(obj_t* parent, obj_t* child) {
    if (!child) {
        return;
    }
    if (parent->num_refs >= WM_MAX_REFS) {
        err("mock: too many references held by one %s", kind_name(parent->kind));
        return;
    }
    child->refcount++;
    parent->refs[parent->num_refs++] = child;
}

static void obj_release(obj_t* o);

static void obj_destroy(obj_t* o) {
    o->alive = false;
    g_live[o->kind]--;
    switch (o->kind) {
        case WGPU_MOCK_OBJ_BUFFER:
            free(o->u.buffer.data);
            o->u.buffer.data = 0;
            break;
        case WGPU_MOCK_OBJ_SHADER_MODULE:
            free(o->u.shader.code);
            o->u.shader.code = 0;
            break;
        default:
            break;
    }
    int n = o->num_refs;
    o->num_refs = 0;
    for (int i = 0; i < n; i++) {
        obj_release(o->refs[i]);
    }
}

static void obj_release(obj_t* o) {
    if (!o || !o->alive) {
        return;
    }
    if (--o->refcount <= 0) {
        obj_destroy(o);
    }
}

// release a handle passed by the application
static void api_release(const void* h, wgpu_mock_obj_t kind, const char* fn) {
    obj_t* o = lookup(h, kind, fn);
    if (o) {
        obj_release(o);
    }
}

static void api_add_ref(const void* h, wgpu_mock_obj_t kind, const char* fn) {
    obj_t* o = lookup(h, kind, fn);
    if (o) {
        o->refcount++;
    }
}

// the texture of a view object
static obj_t* view_texture(obj_t* view) {
    return view->refs[0];
}

// ---- validation helpers
static bool range_ok(uint64_t off, uint64_t size, uint64_t total) {
    return (off <= total) && (size <= (total - off));
}

static uint32_t tex_layers(const wgpu_mock_texture_info_t* t) {
    return (t->dimension == WGPUTextureDimension_3D) ? 1 : t->depth_or_layers;
}

static uint32_t log2_count(uint32_t m) {
    uint32_t n = 1;
    while (m > 1) {
        m >>= 1;
        n++;
    }
    return n;
}

// storage texture capable formats
static bool fmt_storage(WGPUTextureFormat f) {
    switch (f) {
        case WGPUTextureFormat_RGBA8Unorm: case WGPUTextureFormat_RGBA8Snorm:
        case WGPUTextureFormat_RGBA8Uint: case WGPUTextureFormat_RGBA8Sint:
        case WGPUTextureFormat_RGBA16Uint: case WGPUTextureFormat_RGBA16Sint: case WGPUTextureFormat_RGBA16Float:
        case WGPUTextureFormat_R32Uint: case WGPUTextureFormat_R32Sint: case WGPUTextureFormat_R32Float:
        case WGPUTextureFormat_RG32Uint: case WGPUTextureFormat_RG32Sint: case WGPUTextureFormat_RG32Float:
        case WGPUTextureFormat_RGBA32Uint: case WGPUTextureFormat_RGBA32Sint: case WGPUTextureFormat_RGBA32Float:
            return true;
        case WGPUTextureFormat_R8Unorm: case WGPUTextureFormat_R8Snorm:
        case WGPUTextureFormat_R8Uint: case WGPUTextureFormat_R8Sint:
        case WGPUTextureFormat_RG8Unorm: case WGPUTextureFormat_RG8Snorm:
        case WGPUTextureFormat_RG8Uint: case WGPUTextureFormat_RG8Sint:
        case WGPUTextureFormat_R16Uint: case WGPUTextureFormat_R16Sint: case WGPUTextureFormat_R16Float:
        case WGPUTextureFormat_RG16Uint: case WGPUTextureFormat_RG16Sint: case WGPUTextureFormat_RG16Float:
            return has_feature(WGPUFeatureName_TextureFormatsTier2);
        default:
            return false;
    }
}

// render target byte cost of a format
static uint32_t fmt_target_cost(WGPUTextureFormat f) {
    const fmt_info_t* fi = fmt_info(f);
    if (!fi) {
        return 0;
    }
    if ((f == WGPUTextureFormat_RGB10A2Unorm) || (f == WGPUTextureFormat_RG11B10Ufloat)) {
        return 8;
    }
    return fi->bytes;
}

// is a sample type usable with a texture format and aspect
static bool sample_type_ok(WGPUTextureFormat f, WGPUTextureAspect aspect, WGPUTextureSampleType st) {
    const fmt_info_t* fi = fmt_info(f);
    if (!fi) {
        return false;
    }
    if (aspect == WGPUTextureAspect_StencilOnly) {
        return st == WGPUTextureSampleType_Uint;
    }
    if (fi->flags & F_DEPTH) {
        return (st == WGPUTextureSampleType_Depth) || (st == WGPUTextureSampleType_UnfilterableFloat);
    }
    if (fi->flags & F_UINT) {
        return st == WGPUTextureSampleType_Uint;
    }
    if (fi->flags & F_SINT) {
        return st == WGPUTextureSampleType_Sint;
    }
    if (st == WGPUTextureSampleType_UnfilterableFloat) {
        return true;
    }
    if (st == WGPUTextureSampleType_Float) {
        if (fi->flags & F_F32) {
            return has_feature(WGPUFeatureName_Float32Filterable);
        }
        return true;
    }
    return false;
}

static const char* label_or_empty(const char* s) {
    return s[0] ? s : "(no label)";
}

// ---- buffers
#define WM_MAX_BUFFER_BYTES ((uint64_t)1 << 30)

static obj_t* buffer_create(const WGPUBufferDescriptor* d) {
    obj_t* o = obj_new(WGPU_MOCK_OBJ_BUFFER);
    wgpu_mock_buffer_info_t* bi = &o->u.buffer.info;
    bi->size = d->size;
    bi->usage = d->usage;
    bi->mapped_at_creation = (d->mappedAtCreation != 0);
    bi->mapped = bi->mapped_at_creation;
    sv_copy(bi->label, sizeof(bi->label), d->label);
    const char* l = label_or_empty(bi->label);
    if ((d->usage == 0) || ((d->usage & ~(uint64_t)0xFF) != 0)) {
        err("CreateBuffer(%s): invalid usage 0x%llx", l, LL(d->usage));
    }
    if (((d->usage & WGPUBufferUsage_MapRead) != 0) && ((d->usage & ~(WGPUBufferUsage_MapRead | WGPUBufferUsage_CopyDst)) != 0)) {
        err("CreateBuffer(%s): MapRead can only be combined with CopyDst", l);
    }
    if (((d->usage & WGPUBufferUsage_MapWrite) != 0) && ((d->usage & ~(WGPUBufferUsage_MapWrite | WGPUBufferUsage_CopySrc)) != 0)) {
        err("CreateBuffer(%s): MapWrite can only be combined with CopySrc", l);
    }
    if (d->size > g_limits.maxBufferSize) {
        err("CreateBuffer(%s): size %llu exceeds maxBufferSize", l, LL(d->size));
    }
    if (d->mappedAtCreation && ((d->size & 3) != 0)) {
        err("CreateBuffer(%s): mappedAtCreation requires a size multiple of 4 (size is %llu)", l, LL(d->size));
    }
    if (d->size <= WM_MAX_BUFFER_BYTES) {
        o->u.buffer.data = (uint8_t*)calloc(1, (d->size > 0) ? (size_t)d->size : 1);
    }
    return o;
}

// effective size of a buffer binding (WGPU_WHOLE_SIZE means the rest of the buffer)
static uint64_t buffer_binding_size(const obj_t* b, uint64_t off, uint64_t size) {
    if (size == WGPU_WHOLE_SIZE) {
        return (off <= b->u.buffer.info.size) ? (b->u.buffer.info.size - off) : 0;
    }
    return size;
}

// ---- textures
static obj_t* texture_create(const WGPUTextureDescriptor* d) {
    obj_t* o = obj_new(WGPU_MOCK_OBJ_TEXTURE);
    wgpu_mock_texture_info_t* ti = &o->u.texture.info;
    ti->dimension = d->dimension;
    ti->width = d->size.width;
    ti->height = d->size.height;
    ti->depth_or_layers = d->size.depthOrArrayLayers;
    ti->format = d->format;
    ti->mip_level_count = d->mipLevelCount;
    ti->sample_count = d->sampleCount;
    ti->usage = d->usage;
    sv_copy(ti->label, sizeof(ti->label), d->label);
    const char* l = label_or_empty(ti->label);
    const fmt_info_t* fi = fmt_info(d->format);
    if (!fi) {
        err("CreateTexture(%s): unknown format %d", l, (int)d->format);
        return o;
    }
    if (!fmt_supported(d->format)) {
        err("CreateTexture(%s): format %d requires feature %d which is not enabled", l, (int)d->format, (int)fmt_required_feature(d->format));
    }
    if ((d->usage == 0) || ((d->usage & ~(uint64_t)0x1F) != 0)) {
        err("CreateTexture(%s): invalid usage 0x%llx", l, LL(d->usage));
    }
    if ((d->size.width == 0) || (d->size.height == 0) || (d->size.depthOrArrayLayers == 0)) {
        err("CreateTexture(%s): size must not be zero (%u x %u x %u)", l, d->size.width, d->size.height, d->size.depthOrArrayLayers);
        return o;
    }
    uint32_t max_dim = 0;
    switch (d->dimension) {
        case WGPUTextureDimension_1D:
            if ((d->size.height != 1) || (d->size.depthOrArrayLayers != 1)) {
                err("CreateTexture(%s): 1D texture must have height and depth 1", l);
            }
            if (d->size.width > g_limits.maxTextureDimension1D) {
                err("CreateTexture(%s): width %u exceeds maxTextureDimension1D", l, d->size.width);
            }
            if ((fi->flags & (F_COMP | F_DEPTH | F_STENCIL)) || (d->usage & WGPUTextureUsage_RenderAttachment)) {
                err("CreateTexture(%s): invalid format or usage for a 1D texture", l);
            }
            max_dim = d->size.width;
            break;
        case WGPUTextureDimension_2D:
            if ((d->size.width > g_limits.maxTextureDimension2D) || (d->size.height > g_limits.maxTextureDimension2D)) {
                err("CreateTexture(%s): size %ux%u exceeds maxTextureDimension2D", l, d->size.width, d->size.height);
            }
            if (d->size.depthOrArrayLayers > g_limits.maxTextureArrayLayers) {
                err("CreateTexture(%s): %u array layers exceed maxTextureArrayLayers", l, d->size.depthOrArrayLayers);
            }
            max_dim = u32_max(d->size.width, d->size.height);
            break;
        case WGPUTextureDimension_3D:
            if ((d->size.width > g_limits.maxTextureDimension3D) || (d->size.height > g_limits.maxTextureDimension3D) ||
                (d->size.depthOrArrayLayers > g_limits.maxTextureDimension3D))
            {
                err("CreateTexture(%s): size exceeds maxTextureDimension3D", l);
            }
            if (fi->flags & (F_DEPTH | F_STENCIL)) {
                err("CreateTexture(%s): depth/stencil formats cannot be used for 3D textures", l);
            }
            if ((fi->flags & F_COMP) && !has_feature(WGPUFeatureName_TextureCompressionBCSliced3D) &&
                !has_feature(WGPUFeatureName_TextureCompressionASTCSliced3D))
            {
                err("CreateTexture(%s): compressed formats cannot be used for 3D textures", l);
            }
            max_dim = u32_max(d->size.width, u32_max(d->size.height, d->size.depthOrArrayLayers));
            break;
        default:
            err("CreateTexture(%s): invalid dimension %d", l, (int)d->dimension);
            return o;
    }
    if ((d->mipLevelCount == 0) || (d->mipLevelCount > log2_count(max_dim))) {
        err("CreateTexture(%s): invalid mip level count %u (max is %u)", l, d->mipLevelCount, log2_count(max_dim));
    }
    if ((d->sampleCount != 1) && (d->sampleCount != 4)) {
        err("CreateTexture(%s): invalid sample count %u", l, d->sampleCount);
    }
    if (d->sampleCount > 1) {
        if ((d->dimension != WGPUTextureDimension_2D) || (d->mipLevelCount != 1) || (d->size.depthOrArrayLayers != 1)) {
            err("CreateTexture(%s): multisampled textures must be 2D with one mip level and one layer", l);
        }
        if (!(d->usage & WGPUTextureUsage_RenderAttachment) || (d->usage & WGPUTextureUsage_StorageBinding)) {
            err("CreateTexture(%s): multisampled textures need RenderAttachment and no StorageBinding usage", l);
        }
        if (!fmt_renderable(d->format)) {
            err("CreateTexture(%s): format is not multisample-capable", l);
        }
    }
    if (fi->flags & F_COMP) {
        if (((d->size.width % fi->bw) != 0) || ((d->size.height % fi->bh) != 0)) {
            err("CreateTexture(%s): size %ux%u is not a multiple of the block size", l, d->size.width, d->size.height);
        }
        if (d->usage & (WGPUTextureUsage_RenderAttachment | WGPUTextureUsage_StorageBinding)) {
            err("CreateTexture(%s): compressed formats cannot be render or storage targets", l);
        }
    }
    if ((d->usage & WGPUTextureUsage_RenderAttachment) && !fmt_renderable(d->format)) {
        err("CreateTexture(%s): format %d is not renderable", l, (int)d->format);
    }
    if ((d->usage & WGPUTextureUsage_StorageBinding) && !fmt_storage(d->format)) {
        err("CreateTexture(%s): format %d does not support storage binding", l, (int)d->format);
    }
    if ((d->usage & WGPUTextureUsage_StorageBinding) && (fi->flags & (F_DEPTH | F_STENCIL))) {
        err("CreateTexture(%s): depth/stencil formats cannot be storage textures", l);
    }
    return o;
}

static obj_t* view_create(obj_t* tex, const WGPUTextureViewDescriptor* desc) {
    const wgpu_mock_texture_info_t* ti = &tex->u.texture.info;
    WGPUTextureViewDescriptor d;
    if (desc) {
        d = *desc;
    } else {
        memset(&d, 0, sizeof(d));
        d.mipLevelCount = WGPU_MIP_LEVEL_COUNT_UNDEFINED;
        d.arrayLayerCount = WGPU_ARRAY_LAYER_COUNT_UNDEFINED;
    }
    obj_t* o = obj_new(WGPU_MOCK_OBJ_TEXTURE_VIEW);
    wgpu_mock_texture_view_info_t* vi = &o->u.view.info;
    add_ref(o, tex);
    tex->u.texture.info.num_views++;
    sv_copy(vi->label, sizeof(vi->label), d.label);
    const char* l = label_or_empty(vi->label);
    const fmt_info_t* fi = fmt_info(ti->format);
    const uint32_t layers = tex_layers(ti);

    WGPUTextureAspect aspect = (d.aspect == WGPUTextureAspect_Undefined) ? WGPUTextureAspect_All : d.aspect;
    WGPUTextureFormat fmt = (d.format == WGPUTextureFormat_Undefined) ? ti->format : d.format;
    if ((d.format == WGPUTextureFormat_Undefined) && (aspect == WGPUTextureAspect_DepthOnly) &&
        (ti->format == WGPUTextureFormat_Depth32FloatStencil8))
    {
        fmt = WGPUTextureFormat_Depth32Float;
    }
    WGPUTextureViewDimension dim = d.dimension;
    if (dim == WGPUTextureViewDimension_Undefined) {
        switch (ti->dimension) {
            case WGPUTextureDimension_1D: dim = WGPUTextureViewDimension_1D; break;
            case WGPUTextureDimension_3D: dim = WGPUTextureViewDimension_3D; break;
            default: dim = (ti->depth_or_layers == 1) ? WGPUTextureViewDimension_2D : WGPUTextureViewDimension_2DArray; break;
        }
    }
    uint32_t mip_count = d.mipLevelCount;
    if (mip_count == WGPU_MIP_LEVEL_COUNT_UNDEFINED) {
        mip_count = (ti->mip_level_count > d.baseMipLevel) ? (ti->mip_level_count - d.baseMipLevel) : 0;
    }
    uint32_t layer_count = d.arrayLayerCount;
    if (layer_count == WGPU_ARRAY_LAYER_COUNT_UNDEFINED) {
        switch (dim) {
            case WGPUTextureViewDimension_Cube: layer_count = 6; break;
            case WGPUTextureViewDimension_2DArray:
            case WGPUTextureViewDimension_CubeArray:
                layer_count = (layers > d.baseArrayLayer) ? (layers - d.baseArrayLayer) : 0;
                break;
            default: layer_count = 1; break;
        }
    }
    vi->texture = tex;
    vi->format = fmt;
    vi->dimension = dim;
    vi->base_mip_level = d.baseMipLevel;
    vi->mip_level_count = mip_count;
    vi->base_array_layer = d.baseArrayLayer;
    vi->array_layer_count = layer_count;
    vi->aspect = aspect;

    if (!fi) {
        err("CreateView(%s): texture has an unknown format", l);
        return o;
    }
    if ((mip_count == 0) || (!range_ok(d.baseMipLevel, mip_count, ti->mip_level_count))) {
        err("CreateView(%s): mip range (base %u, count %u) exceeds the texture's %u levels", l, d.baseMipLevel, mip_count, ti->mip_level_count);
    }
    if ((layer_count == 0) || (!range_ok(d.baseArrayLayer, layer_count, layers))) {
        err("CreateView(%s): layer range (base %u, count %u) exceeds the texture's %u layers", l, d.baseArrayLayer, layer_count, layers);
    }
    bool dim_ok = false;
    switch (ti->dimension) {
        case WGPUTextureDimension_1D: dim_ok = (dim == WGPUTextureViewDimension_1D); break;
        case WGPUTextureDimension_2D:
            dim_ok = (dim == WGPUTextureViewDimension_2D) || (dim == WGPUTextureViewDimension_2DArray) ||
                     (dim == WGPUTextureViewDimension_Cube) || (dim == WGPUTextureViewDimension_CubeArray);
            break;
        case WGPUTextureDimension_3D: dim_ok = (dim == WGPUTextureViewDimension_3D); break;
        default: break;
    }
    if (!dim_ok) {
        err("CreateView(%s): view dimension %d is not compatible with texture dimension %d", l, (int)dim, (int)ti->dimension);
    }
    switch (dim) {
        case WGPUTextureViewDimension_1D:
        case WGPUTextureViewDimension_2D:
        case WGPUTextureViewDimension_3D:
            if (layer_count != 1) {
                err("CreateView(%s): array layer count must be 1 for this view dimension (is %u)", l, layer_count);
            }
            break;
        case WGPUTextureViewDimension_Cube:
            if (layer_count != 6) {
                err("CreateView(%s): cube views need exactly 6 layers (is %u)", l, layer_count);
            }
            break;
        case WGPUTextureViewDimension_CubeArray:
            if ((layer_count == 0) || ((layer_count % 6) != 0)) {
                err("CreateView(%s): cube array views need a multiple of 6 layers (is %u)", l, layer_count);
            }
            break;
        default:
            break;
    }
    if (((dim == WGPUTextureViewDimension_Cube) || (dim == WGPUTextureViewDimension_CubeArray)) && (ti->width != ti->height)) {
        err("CreateView(%s): cube views need a square texture (%ux%u)", l, ti->width, ti->height);
    }
    if (fmt != ti->format) {
        const bool depth_only_ok = (ti->format == WGPUTextureFormat_Depth32FloatStencil8) &&
                                   (fmt == WGPUTextureFormat_Depth32Float) && (aspect == WGPUTextureAspect_DepthOnly);
        const fmt_info_t* vfi = fmt_info(fmt);
        const bool srgb_ok = vfi && (vfi->base == fi->base) && !(fi->flags & (F_DEPTH | F_STENCIL));
        if (!depth_only_ok && !srgb_ok) {
            err("CreateView(%s): view format %d is not compatible with texture format %d", l, (int)fmt, (int)ti->format);
        }
    }
    switch (aspect) {
        case WGPUTextureAspect_All:
            break;
        case WGPUTextureAspect_DepthOnly:
            if (!(fi->flags & F_DEPTH)) {
                err("CreateView(%s): DepthOnly aspect needs a format with a depth aspect", l);
            }
            break;
        case WGPUTextureAspect_StencilOnly:
            if (!(fi->flags & F_STENCIL)) {
                err("CreateView(%s): StencilOnly aspect needs a format with a stencil aspect", l);
            }
            break;
        default:
            err("CreateView(%s): invalid aspect %d", l, (int)aspect);
            break;
    }
    if (d.usage == 0) {
        o->u.view.usage = ti->usage;
    } else {
        o->u.view.usage = d.usage;
        if ((d.usage & ~ti->usage) != 0) {
            err("CreateView(%s): view usage 0x%llx is not a subset of the texture usage 0x%llx", l, LL(d.usage), LL(ti->usage));
        }
    }
    return o;
}

// ---- samplers
static obj_t* sampler_create(const WGPUSamplerDescriptor* d) {
    obj_t* o = obj_new(WGPU_MOCK_OBJ_SAMPLER);
    wgpu_mock_sampler_info_t* si = &o->u.sampler.info;
    si->address_u = d->addressModeU;
    si->address_v = d->addressModeV;
    si->address_w = d->addressModeW;
    si->mag_filter = d->magFilter;
    si->min_filter = d->minFilter;
    si->mipmap_filter = d->mipmapFilter;
    si->lod_min_clamp = d->lodMinClamp;
    si->lod_max_clamp = d->lodMaxClamp;
    si->compare = d->compare;
    si->max_anisotropy = d->maxAnisotropy;
    sv_copy(si->label, sizeof(si->label), d->label);
    const char* l = label_or_empty(si->label);
    if (!(d->lodMinClamp >= 0.0f) || !(d->lodMaxClamp >= d->lodMinClamp)) {
        err("CreateSampler(%s): invalid lod clamp range [%f, %f]", l, (double)d->lodMinClamp, (double)d->lodMaxClamp);
    }
    if (d->maxAnisotropy < 1) {
        err("CreateSampler(%s): maxAnisotropy must be at least 1 (is %u)", l, (unsigned)d->maxAnisotropy);
    }
    if (d->maxAnisotropy > 1) {
        if ((d->magFilter != WGPUFilterMode_Linear) || (d->minFilter != WGPUFilterMode_Linear) ||
            (d->mipmapFilter != WGPUMipmapFilterMode_Linear))
        {
            err("CreateSampler(%s): maxAnisotropy %u needs linear mag, min and mipmap filters", l, (unsigned)d->maxAnisotropy);
        }
    }
    return o;
}

// ---- shader modules
static obj_t* shader_create(const WGPUShaderModuleDescriptor* d) {
    obj_t* o = obj_new(WGPU_MOCK_OBJ_SHADER_MODULE);
    wgpu_mock_shader_module_info_t* mi = &o->u.shader.info;
    sv_copy(mi->label, sizeof(mi->label), d->label);
    const char* l = label_or_empty(mi->label);
    const WGPUShaderSourceWGSL* src = 0;
    for (const WGPUChainedStruct* n = d->nextInChain; n != 0; n = n->next) {
        if (n->sType == WGPUSType_ShaderSourceWGSL) {
            src = (const WGPUShaderSourceWGSL*)n;
        } else {
            err("CreateShaderModule(%s): unsupported chained struct type %d", l, (int)n->sType);
        }
    }
    if (!src || !src->code.data) {
        err("CreateShaderModule(%s): no WGSL source code in the descriptor", l);
        return o;
    }
    size_t len = (src->code.length == WGPU_STRLEN) ? strlen(src->code.data) : src->code.length;
    o->u.shader.code = (char*)malloc(len + 1);
    memcpy(o->u.shader.code, src->code.data, len);
    o->u.shader.code[len] = 0;
    mi->code = o->u.shader.code;
    mi->code_length = len;
    return o;
}

// ---- bind group layouts
typedef struct {
    uint32_t ubo, sbo, tex, smp, stex;
} stage_counts_t;

static int bgl_entry_types(const WGPUBindGroupLayoutEntry* e) {
    int n = 0;
    n += (e->buffer.type != WGPUBufferBindingType_BindingNotUsed);
    n += (e->sampler.type != WGPUSamplerBindingType_BindingNotUsed);
    n += (e->texture.sampleType != WGPUTextureSampleType_BindingNotUsed);
    n += (e->storageTexture.access != WGPUStorageTextureAccess_BindingNotUsed);
    return n;
}

static bool bgl_entry_is_buffer(const WGPUBindGroupLayoutEntry* e) {
    return e->buffer.type != WGPUBufferBindingType_BindingNotUsed;
}

static bool bgl_entry_is_storage_buffer(const WGPUBindGroupLayoutEntry* e) {
    return (e->buffer.type == WGPUBufferBindingType_Storage) || (e->buffer.type == WGPUBufferBindingType_ReadOnlyStorage);
}

static void count_stage_entries(stage_counts_t sc[WM_NUM_STAGES], const wgpu_mock_bind_group_layout_info_t* info, uint32_t* dyn_ubo, uint32_t* dyn_sbo) {
    for (int i = 0; i < info->num_entries; i++) {
        const WGPUBindGroupLayoutEntry* e = &info->entries[i];
        if (bgl_entry_is_buffer(e) && e->buffer.hasDynamicOffset) {
            if (bgl_entry_is_storage_buffer(e)) {
                (*dyn_sbo)++;
            } else {
                (*dyn_ubo)++;
            }
        }
        for (int s = 0; s < WM_NUM_STAGES; s++) {
            if (!(e->visibility & (WGPUShaderStage)(1u << s))) {
                continue;
            }
            if (bgl_entry_is_buffer(e)) {
                if (bgl_entry_is_storage_buffer(e)) {
                    sc[s].sbo++;
                } else {
                    sc[s].ubo++;
                }
            } else if (e->sampler.type != WGPUSamplerBindingType_BindingNotUsed) {
                sc[s].smp++;
            } else if (e->texture.sampleType != WGPUTextureSampleType_BindingNotUsed) {
                sc[s].tex++;
            } else if (e->storageTexture.access != WGPUStorageTextureAccess_BindingNotUsed) {
                sc[s].stex++;
            }
        }
    }
}

static void check_stage_limits(const stage_counts_t sc[WM_NUM_STAGES], const char* what, const char* l) {
    for (int s = 0; s < WM_NUM_STAGES; s++) {
        if (sc[s].ubo > g_limits.maxUniformBuffersPerShaderStage) {
            err("%s(%s): %u uniform buffers in stage %d exceed the limit", what, l, sc[s].ubo, s);
        }
        if (sc[s].sbo > g_limits.maxStorageBuffersPerShaderStage) {
            err("%s(%s): %u storage buffers in stage %d exceed the limit", what, l, sc[s].sbo, s);
        }
        if (sc[s].tex > g_limits.maxSampledTexturesPerShaderStage) {
            err("%s(%s): %u sampled textures in stage %d exceed the limit", what, l, sc[s].tex, s);
        }
        if (sc[s].smp > g_limits.maxSamplersPerShaderStage) {
            err("%s(%s): %u samplers in stage %d exceed the limit", what, l, sc[s].smp, s);
        }
        if (sc[s].stex > g_limits.maxStorageTexturesPerShaderStage) {
            err("%s(%s): %u storage textures in stage %d exceed the limit", what, l, sc[s].stex, s);
        }
    }
}

static const WGPUBindGroupLayoutEntry* find_bgl_entry(const wgpu_mock_bind_group_layout_info_t* info, uint32_t binding) {
    for (int i = 0; i < info->num_entries; i++) {
        if (info->entries[i].binding == binding) {
            return &info->entries[i];
        }
    }
    return 0;
}

static bool bgl_entry_equal(const WGPUBindGroupLayoutEntry* a, const WGPUBindGroupLayoutEntry* b) {
    return (a->visibility == b->visibility) &&
           (a->buffer.type == b->buffer.type) &&
           (a->buffer.hasDynamicOffset == b->buffer.hasDynamicOffset) &&
           (a->buffer.minBindingSize == b->buffer.minBindingSize) &&
           (a->sampler.type == b->sampler.type) &&
           (a->texture.sampleType == b->texture.sampleType) &&
           (a->texture.viewDimension == b->texture.viewDimension) &&
           (a->texture.multisampled == b->texture.multisampled) &&
           (a->storageTexture.access == b->storageTexture.access) &&
           (a->storageTexture.format == b->storageTexture.format) &&
           (a->storageTexture.viewDimension == b->storageTexture.viewDimension);
}

// group-equivalent layouts, same object or identical entries
static bool bgl_compatible(const obj_t* a, const obj_t* b) {
    if (a == b) {
        return true;
    }
    const wgpu_mock_bind_group_layout_info_t* ia = &a->u.bgl.info;
    const wgpu_mock_bind_group_layout_info_t* ib = &b->u.bgl.info;
    if (ia->num_entries != ib->num_entries) {
        return false;
    }
    for (int i = 0; i < ia->num_entries; i++) {
        const WGPUBindGroupLayoutEntry* eb = find_bgl_entry(ib, ia->entries[i].binding);
        if (!eb || !bgl_entry_equal(&ia->entries[i], eb)) {
            return false;
        }
    }
    return true;
}

static obj_t* bgl_create(const WGPUBindGroupLayoutDescriptor* d) {
    obj_t* o = obj_new(WGPU_MOCK_OBJ_BIND_GROUP_LAYOUT);
    wgpu_mock_bind_group_layout_info_t* bi = &o->u.bgl.info;
    sv_copy(bi->label, sizeof(bi->label), d->label);
    const char* l = label_or_empty(bi->label);
    size_t n = d->entryCount;
    if (n > 0 && !d->entries) {
        err("CreateBindGroupLayout(%s): entry count is %u but entries is null", l, (unsigned)n);
        return o;
    }
    if (n > WGPU_MOCK_MAX_BIND_ENTRIES) {
        err("CreateBindGroupLayout(%s): %u entries exceed the mock limit of %d", l, (unsigned)n, WGPU_MOCK_MAX_BIND_ENTRIES);
        n = WGPU_MOCK_MAX_BIND_ENTRIES;
    }
    bi->num_entries = (int)n;
    for (size_t i = 0; i < n; i++) {
        WGPUBindGroupLayoutEntry* e = &bi->entries[i];
        *e = d->entries[i];
        const int num_types = bgl_entry_types(e);
        if (num_types != 1) {
            err("CreateBindGroupLayout(%s): binding %u must have exactly one resource type (has %d)", l, e->binding, num_types);
        }
        // resolve defaults
        if (e->buffer.type == WGPUBufferBindingType_Undefined) {
            e->buffer.type = WGPUBufferBindingType_Uniform;
        }
        if (e->sampler.type == WGPUSamplerBindingType_Undefined) {
            e->sampler.type = WGPUSamplerBindingType_Filtering;
        }
        if (e->texture.sampleType == WGPUTextureSampleType_Undefined) {
            e->texture.sampleType = WGPUTextureSampleType_Float;
        }
        if (e->texture.sampleType != WGPUTextureSampleType_BindingNotUsed) {
            if (e->texture.viewDimension == WGPUTextureViewDimension_Undefined) {
                e->texture.viewDimension = WGPUTextureViewDimension_2D;
            }
        }
        if (e->storageTexture.access == WGPUStorageTextureAccess_Undefined) {
            e->storageTexture.access = WGPUStorageTextureAccess_WriteOnly;
        }
        if (e->storageTexture.access != WGPUStorageTextureAccess_BindingNotUsed) {
            if (e->storageTexture.viewDimension == WGPUTextureViewDimension_Undefined) {
                e->storageTexture.viewDimension = WGPUTextureViewDimension_2D;
            }
        }
        // validate
        if (e->binding >= g_limits.maxBindingsPerBindGroup) {
            err("CreateBindGroupLayout(%s): binding %u exceeds maxBindingsPerBindGroup", l, e->binding);
        }
        for (size_t j = 0; j < i; j++) {
            if (bi->entries[j].binding == e->binding) {
                err("CreateBindGroupLayout(%s): duplicate binding %u", l, e->binding);
            }
        }
        if ((e->visibility & ~(WGPUShaderStage)7) != 0) {
            err("CreateBindGroupLayout(%s): invalid visibility 0x%llx for binding %u", l, LL(e->visibility), e->binding);
        }
        if (e->buffer.hasDynamicOffset) {
            if (!bgl_entry_is_buffer(e)) {
                err("CreateBindGroupLayout(%s): hasDynamicOffset is only valid for buffer bindings (binding %u)", l, e->binding);
            } else {
                bi->num_dynamic_offsets++;
            }
        }
        if (e->buffer.type == WGPUBufferBindingType_Storage && (e->visibility & WGPUShaderStage_Vertex)) {
            err("CreateBindGroupLayout(%s): read-write storage buffer binding %u cannot be visible to the vertex stage", l, e->binding);
        }
        if ((e->storageTexture.access != WGPUStorageTextureAccess_BindingNotUsed) && (e->visibility & WGPUShaderStage_Vertex) &&
            (e->storageTexture.access != WGPUStorageTextureAccess_ReadOnly))
        {
            err("CreateBindGroupLayout(%s): writable storage texture binding %u cannot be visible to the vertex stage", l, e->binding);
        }
        if (e->texture.sampleType != WGPUTextureSampleType_BindingNotUsed) {
            if (e->texture.multisampled) {
                if (e->texture.sampleType == WGPUTextureSampleType_Float) {
                    err("CreateBindGroupLayout(%s): multisampled texture binding %u cannot use sample type Float", l, e->binding);
                }
                if (e->texture.viewDimension != WGPUTextureViewDimension_2D) {
                    err("CreateBindGroupLayout(%s): multisampled texture binding %u needs a 2D view dimension", l, e->binding);
                }
            }
        }
        if (e->storageTexture.access != WGPUStorageTextureAccess_BindingNotUsed) {
            if (!fmt_storage(e->storageTexture.format)) {
                err("CreateBindGroupLayout(%s): storage texture binding %u has a format which does not support storage", l, e->binding);
            }
            if ((e->storageTexture.viewDimension == WGPUTextureViewDimension_Cube) ||
                (e->storageTexture.viewDimension == WGPUTextureViewDimension_CubeArray))
            {
                err("CreateBindGroupLayout(%s): storage texture binding %u cannot use a cube view dimension", l, e->binding);
            }
        }
    }
    stage_counts_t sc[WM_NUM_STAGES];
    memset(sc, 0, sizeof(sc));
    uint32_t dyn_ubo = 0, dyn_sbo = 0;
    count_stage_entries(sc, bi, &dyn_ubo, &dyn_sbo);
    check_stage_limits(sc, "CreateBindGroupLayout", l);
    if (dyn_ubo > g_limits.maxDynamicUniformBuffersPerPipelineLayout) {
        err("CreateBindGroupLayout(%s): %u dynamic uniform buffers exceed the limit", l, dyn_ubo);
    }
    if (dyn_sbo > g_limits.maxDynamicStorageBuffersPerPipelineLayout) {
        err("CreateBindGroupLayout(%s): %u dynamic storage buffers exceed the limit", l, dyn_sbo);
    }
    return o;
}

// ---- bind groups
static void bg_check_buffer(const char* l, const WGPUBindGroupLayoutEntry* le, const WGPUBindGroupEntry* e, obj_t* b) {
    const wgpu_mock_buffer_info_t* bi = &b->u.buffer.info;
    const bool storage = bgl_entry_is_storage_buffer(le);
    const WGPUBufferUsage need = storage ? WGPUBufferUsage_Storage : WGPUBufferUsage_Uniform;
    if (!(bi->usage & need)) {
        err("CreateBindGroup(%s): buffer at binding %u lacks the %s usage", l, e->binding, storage ? "Storage" : "Uniform");
    }
    const uint32_t align = storage ? g_limits.minStorageBufferOffsetAlignment : g_limits.minUniformBufferOffsetAlignment;
    if ((align > 0) && ((e->offset % align) != 0)) {
        err("CreateBindGroup(%s): buffer offset %llu at binding %u is not a multiple of %u", l, LL(e->offset), e->binding, align);
    }
    if (e->offset > bi->size) {
        err("CreateBindGroup(%s): buffer offset %llu at binding %u exceeds the buffer size %llu", l, LL(e->offset), e->binding, LL(bi->size));
        return;
    }
    const uint64_t size = buffer_binding_size(b, e->offset, e->size);
    if (size == 0) {
        err("CreateBindGroup(%s): buffer binding size at binding %u is zero", l, e->binding);
    }
    if (!range_ok(e->offset, size, bi->size)) {
        err("CreateBindGroup(%s): buffer range (offset %llu, size %llu) at binding %u exceeds the buffer size %llu", l, LL(e->offset), LL(size), e->binding, LL(bi->size));
    }
    if (storage) {
        if (size > g_limits.maxStorageBufferBindingSize) {
            err("CreateBindGroup(%s): storage binding size %llu at binding %u exceeds the limit", l, LL(size), e->binding);
        }
        if ((size % 4) != 0) {
            err("CreateBindGroup(%s): storage binding size %llu at binding %u is not a multiple of 4", l, LL(size), e->binding);
        }
    } else if (size > g_limits.maxUniformBufferBindingSize) {
        err("CreateBindGroup(%s): uniform binding size %llu at binding %u exceeds the limit", l, LL(size), e->binding);
    }
    if (size < le->buffer.minBindingSize) {
        err("CreateBindGroup(%s): binding size %llu at binding %u is below minBindingSize %llu", l, LL(size), e->binding, LL(le->buffer.minBindingSize));
    }
}

static void bg_check_sampler(const char* l, const WGPUBindGroupLayoutEntry* le, const WGPUBindGroupEntry* e, obj_t* s) {
    const wgpu_mock_sampler_info_t* si = &s->u.sampler.info;
    const bool has_cmp = (si->compare != WGPUCompareFunction_Undefined);
    switch (le->sampler.type) {
        case WGPUSamplerBindingType_Comparison:
            if (!has_cmp) {
                err("CreateBindGroup(%s): sampler at binding %u must be a comparison sampler", l, e->binding);
            }
            break;
        case WGPUSamplerBindingType_NonFiltering:
            if (has_cmp) {
                err("CreateBindGroup(%s): sampler at binding %u must not be a comparison sampler", l, e->binding);
            }
            if ((si->mag_filter == WGPUFilterMode_Linear) || (si->min_filter == WGPUFilterMode_Linear) ||
                (si->mipmap_filter == WGPUMipmapFilterMode_Linear))
            {
                err("CreateBindGroup(%s): sampler at binding %u must not use linear filtering", l, e->binding);
            }
            break;
        default:
            if (has_cmp) {
                err("CreateBindGroup(%s): sampler at binding %u must not be a comparison sampler", l, e->binding);
            }
            break;
    }
}

static void bg_check_view(const char* l, const WGPUBindGroupLayoutEntry* le, const WGPUBindGroupEntry* e, obj_t* v) {
    const wgpu_mock_texture_view_info_t* vi = &v->u.view.info;
    const obj_t* tex = view_texture(v);
    const wgpu_mock_texture_info_t* ti = &tex->u.texture.info;
    if (le->texture.sampleType != WGPUTextureSampleType_BindingNotUsed) {
        if (!(v->u.view.usage & WGPUTextureUsage_TextureBinding)) {
            err("CreateBindGroup(%s): view at binding %u lacks the TextureBinding usage", l, e->binding);
        }
        if (vi->dimension != le->texture.viewDimension) {
            err("CreateBindGroup(%s): view dimension %d at binding %u does not match the layout (%d)", l, (int)vi->dimension, e->binding, (int)le->texture.viewDimension);
        }
        if ((ti->sample_count > 1) != (le->texture.multisampled != 0)) {
            err("CreateBindGroup(%s): multisample state of the view at binding %u does not match the layout", l, e->binding);
        }
        if (!sample_type_ok(vi->format, vi->aspect, le->texture.sampleType)) {
            err("CreateBindGroup(%s): sample type %d at binding %u is not compatible with view format %d (aspect %d)", l, (int)le->texture.sampleType, e->binding, (int)vi->format, (int)vi->aspect);
        }
    } else {
        if (!(v->u.view.usage & WGPUTextureUsage_StorageBinding)) {
            err("CreateBindGroup(%s): view at binding %u lacks the StorageBinding usage", l, e->binding);
        }
        if (vi->format != le->storageTexture.format) {
            err("CreateBindGroup(%s): view format %d at binding %u does not match the layout format %d", l, (int)vi->format, e->binding, (int)le->storageTexture.format);
        }
        if (vi->mip_level_count != 1) {
            err("CreateBindGroup(%s): storage view at binding %u must have exactly one mip level", l, e->binding);
        }
        if (vi->dimension != le->storageTexture.viewDimension) {
            err("CreateBindGroup(%s): view dimension %d at binding %u does not match the layout (%d)", l, (int)vi->dimension, e->binding, (int)le->storageTexture.viewDimension);
        }
    }
}

static obj_t* bg_create(const WGPUBindGroupDescriptor* d) {
    obj_t* o = obj_new(WGPU_MOCK_OBJ_BIND_GROUP);
    wgpu_mock_bind_group_info_t* bi = &o->u.bg.info;
    sv_copy(bi->label, sizeof(bi->label), d->label);
    const char* l = label_or_empty(bi->label);
    obj_t* layout = lookup(d->layout, WGPU_MOCK_OBJ_BIND_GROUP_LAYOUT, "CreateBindGroup");
    bi->layout = d->layout;
    size_t n = d->entryCount;
    if (n > 0 && !d->entries) {
        err("CreateBindGroup(%s): entry count is %u but entries is null", l, (unsigned)n);
        return o;
    }
    if (n > WGPU_MOCK_MAX_BIND_ENTRIES) {
        err("CreateBindGroup(%s): %u entries exceed the mock limit of %d", l, (unsigned)n, WGPU_MOCK_MAX_BIND_ENTRIES);
        n = WGPU_MOCK_MAX_BIND_ENTRIES;
    }
    bi->num_entries = (int)n;
    if (layout) {
        add_ref(o, layout);
        if ((int)d->entryCount != layout->u.bgl.info.num_entries) {
            err("CreateBindGroup(%s): %u entries do not match the %d entries of the layout", l, (unsigned)d->entryCount, layout->u.bgl.info.num_entries);
        }
    }
    for (size_t i = 0; i < n; i++) {
        const WGPUBindGroupEntry* e = &d->entries[i];
        bi->entries[i] = *e;
        for (size_t j = 0; j < i; j++) {
            if (d->entries[j].binding == e->binding) {
                err("CreateBindGroup(%s): duplicate binding %u", l, e->binding);
            }
        }
        const int num_res = (e->buffer != 0) + (e->sampler != 0) + (e->textureView != 0);
        if (num_res != 1) {
            err("CreateBindGroup(%s): binding %u must have exactly one resource (has %d)", l, e->binding, num_res);
            continue;
        }
        const WGPUBindGroupLayoutEntry* le = layout ? find_bgl_entry(&layout->u.bgl.info, e->binding) : 0;
        if (layout && !le) {
            err("CreateBindGroup(%s): binding %u does not exist in the layout", l, e->binding);
        }
        if (e->buffer) {
            obj_t* b = lookup(e->buffer, WGPU_MOCK_OBJ_BUFFER, "CreateBindGroup");
            if (b) {
                add_ref(o, b);
                if (le && bgl_entry_is_buffer(le)) {
                    bg_check_buffer(l, le, e, b);
                } else if (le) {
                    err("CreateBindGroup(%s): binding %u is not a buffer binding in the layout", l, e->binding);
                }
            }
        } else if (e->sampler) {
            obj_t* s = lookup(e->sampler, WGPU_MOCK_OBJ_SAMPLER, "CreateBindGroup");
            if (s) {
                add_ref(o, s);
                if (le && (le->sampler.type != WGPUSamplerBindingType_BindingNotUsed)) {
                    bg_check_sampler(l, le, e, s);
                } else if (le) {
                    err("CreateBindGroup(%s): binding %u is not a sampler binding in the layout", l, e->binding);
                }
            }
        } else {
            obj_t* v = lookup(e->textureView, WGPU_MOCK_OBJ_TEXTURE_VIEW, "CreateBindGroup");
            if (v) {
                add_ref(o, v);
                if (le && ((le->texture.sampleType != WGPUTextureSampleType_BindingNotUsed) ||
                           (le->storageTexture.access != WGPUStorageTextureAccess_BindingNotUsed)))
                {
                    bg_check_view(l, le, e, v);
                } else if (le) {
                    err("CreateBindGroup(%s): binding %u is not a texture binding in the layout", l, e->binding);
                }
            }
        }
    }
    if (layout) {
        for (int i = 0; i < layout->u.bgl.info.num_entries; i++) {
            const uint32_t binding = layout->u.bgl.info.entries[i].binding;
            bool found = false;
            for (size_t j = 0; j < n; j++) {
                found |= (d->entries[j].binding == binding);
            }
            if (!found) {
                err("CreateBindGroup(%s): layout binding %u is missing in the bind group", l, binding);
            }
        }
    }
    return o;
}

// ---- pipeline layouts
static obj_t* pipeline_layout_create(const WGPUPipelineLayoutDescriptor* d) {
    obj_t* o = obj_new(WGPU_MOCK_OBJ_PIPELINE_LAYOUT);
    wgpu_mock_pipeline_layout_info_t* pi = &o->u.pl.info;
    sv_copy(pi->label, sizeof(pi->label), d->label);
    const char* l = label_or_empty(pi->label);
    size_t n = d->bindGroupLayoutCount;
    if (n > 0 && !d->bindGroupLayouts) {
        err("CreatePipelineLayout(%s): layout count is %u but layouts is null", l, (unsigned)n);
        return o;
    }
    if ((n > g_limits.maxBindGroups) || (n > WGPU_MOCK_MAX_BIND_GROUPS)) {
        err("CreatePipelineLayout(%s): %u bind group layouts exceed the limit", l, (unsigned)n);
        n = u32_min((uint32_t)n, WGPU_MOCK_MAX_BIND_GROUPS);
    }
    if (d->immediateSize > g_limits.maxImmediateSize) {
        err("CreatePipelineLayout(%s): immediateSize %u exceeds the limit", l, d->immediateSize);
    }
    stage_counts_t sc[WM_NUM_STAGES];
    memset(sc, 0, sizeof(sc));
    uint32_t dyn_ubo = 0, dyn_sbo = 0;
    pi->num_bind_group_layouts = (int)n;
    for (size_t i = 0; i < n; i++) {
        pi->bind_group_layouts[i] = d->bindGroupLayouts[i];
        obj_t* bgl = lookup(d->bindGroupLayouts[i], WGPU_MOCK_OBJ_BIND_GROUP_LAYOUT, "CreatePipelineLayout");
        if (bgl) {
            add_ref(o, bgl);
            count_stage_entries(sc, &bgl->u.bgl.info, &dyn_ubo, &dyn_sbo);
        }
    }
    check_stage_limits(sc, "CreatePipelineLayout", l);
    if (dyn_ubo > g_limits.maxDynamicUniformBuffersPerPipelineLayout) {
        err("CreatePipelineLayout(%s): %u dynamic uniform buffers exceed the limit", l, dyn_ubo);
    }
    if (dyn_sbo > g_limits.maxDynamicStorageBuffersPerPipelineLayout) {
        err("CreatePipelineLayout(%s): %u dynamic storage buffers exceed the limit", l, dyn_sbo);
    }
    return o;
}

// ---- pipelines
static bool entry_point_ok(const char* l, const char* stage, const obj_t* module, WGPUStringView ep, char* out, size_t out_cap) {
    sv_copy(out, out_cap, ep);
    if (!ep.data) {
        return true;
    }
    if (!code_has_entry(module->u.shader.code, out)) {
        err("%s: entry point '%s' of the %s stage is not defined in the shader module", l, out, stage);
        return false;
    }
    return true;
}

static bool is_strip_topology(WGPUPrimitiveTopology t) {
    return (t == WGPUPrimitiveTopology_LineStrip) || (t == WGPUPrimitiveTopology_TriangleStrip);
}

static bool stencil_face_active(const WGPUStencilFaceState* f) {
    const bool cmp = (f->compare != WGPUCompareFunction_Undefined) && (f->compare != WGPUCompareFunction_Always);
    const bool fail = (f->failOp != WGPUStencilOperation_Undefined) && (f->failOp != WGPUStencilOperation_Keep);
    const bool dfail = (f->depthFailOp != WGPUStencilOperation_Undefined) && (f->depthFailOp != WGPUStencilOperation_Keep);
    const bool pass = (f->passOp != WGPUStencilOperation_Undefined) && (f->passOp != WGPUStencilOperation_Keep);
    return cmp || fail || dfail || pass;
}

static bool stencil_face_depth_fail_active(const WGPUStencilFaceState* f) {
    return (f->depthFailOp != WGPUStencilOperation_Undefined) && (f->depthFailOp != WGPUStencilOperation_Keep);
}

static bool blend_factor_uses_src1(WGPUBlendFactor f) {
    return (f == WGPUBlendFactor_Src1) || (f == WGPUBlendFactor_OneMinusSrc1) ||
           (f == WGPUBlendFactor_Src1Alpha) || (f == WGPUBlendFactor_OneMinusSrc1Alpha);
}

static void check_blend_component(const char* l, const char* what, const WGPUBlendComponent* c) {
    const WGPUBlendFactor src = (c->srcFactor == WGPUBlendFactor_Undefined) ? WGPUBlendFactor_One : c->srcFactor;
    const WGPUBlendFactor dst = (c->dstFactor == WGPUBlendFactor_Undefined) ? WGPUBlendFactor_Zero : c->dstFactor;
    if ((c->operation == WGPUBlendOperation_Min) || (c->operation == WGPUBlendOperation_Max)) {
        if ((src != WGPUBlendFactor_One) || (dst != WGPUBlendFactor_One)) {
            err("%s: %s blend op Min/Max needs One for both factors", l, what);
        }
    }
    if ((blend_factor_uses_src1(src) || blend_factor_uses_src1(dst)) && !has_feature(WGPUFeatureName_DualSourceBlending)) {
        err("%s: %s blend factor needs the DualSourceBlending feature", l, what);
    }
}

static obj_t* render_pipeline_create(const WGPURenderPipelineDescriptor* d) {
    obj_t* o = obj_new(WGPU_MOCK_OBJ_RENDER_PIPELINE);
    wgpu_mock_render_pipeline_info_t* pi = &o->u.rp.info;
    sv_copy(pi->label, sizeof(pi->label), d->label);
    char lbuf[WGPU_MOCK_MAX_STRING + 32];
    snprintf(lbuf, sizeof(lbuf), "CreateRenderPipeline(%s)", label_or_empty(pi->label));
    const char* l = lbuf;

    obj_t* layout = lookup(d->layout, WGPU_MOCK_OBJ_PIPELINE_LAYOUT, l);
    pi->layout = d->layout;
    add_ref(o, layout);
    obj_t* vs = lookup(d->vertex.module, WGPU_MOCK_OBJ_SHADER_MODULE, l);
    pi->vertex_module = d->vertex.module;
    add_ref(o, vs);
    if (vs) {
        entry_point_ok(l, "vertex", vs, d->vertex.entryPoint, pi->vertex_entry_point, sizeof(pi->vertex_entry_point));
    }

    // vertex buffers
    size_t nvb = d->vertex.bufferCount;
    if (nvb > 0 && !d->vertex.buffers) {
        err("%s: vertex buffer count is %u but buffers is null", l, (unsigned)nvb);
        nvb = 0;
    }
    if (nvb > g_limits.maxVertexBuffers) {
        err("%s: %u vertex buffers exceed the limit of %u", l, (unsigned)nvb, g_limits.maxVertexBuffers);
    }
    if (nvb > WGPU_MOCK_MAX_VERTEX_BUFFERS) {
        nvb = WGPU_MOCK_MAX_VERTEX_BUFFERS;
    }
    pi->num_vertex_buffers = (int)nvb;
    uint32_t total_attrs = 0;
    for (size_t i = 0; i < nvb; i++) {
        const WGPUVertexBufferLayout* vb = &d->vertex.buffers[i];
        wgpu_mock_vertex_buffer_layout_t* out = &pi->vertex_buffers[i];
        out->step_mode = vb->stepMode;
        out->array_stride = vb->arrayStride;
        if (vb->arrayStride > g_limits.maxVertexBufferArrayStride) {
            err("%s: vertex buffer %u stride %llu exceeds the limit", l, (unsigned)i, LL(vb->arrayStride));
        }
        if ((vb->arrayStride % 4) != 0) {
            err("%s: vertex buffer %u stride %llu is not a multiple of 4", l, (unsigned)i, LL(vb->arrayStride));
        }
        size_t na = vb->attributeCount;
        if (na > 0 && !vb->attributes) {
            err("%s: vertex buffer %u attribute count is %u but attributes is null", l, (unsigned)i, (unsigned)na);
            na = 0;
        }
        total_attrs += (uint32_t)na;
        if (na > WGPU_MOCK_MAX_VERTEX_ATTRIBUTES) {
            na = WGPU_MOCK_MAX_VERTEX_ATTRIBUTES;
        }
        out->num_attributes = (int)na;
        for (size_t j = 0; j < na; j++) {
            const WGPUVertexAttribute* a = &vb->attributes[j];
            out->attributes[j] = *a;
            const uint32_t asize = vertex_format_size(a->format);
            if (asize == 0) {
                err("%s: vertex buffer %u attribute %u has an invalid format %d", l, (unsigned)i, (unsigned)j, (int)a->format);
                continue;
            }
            const uint64_t stride_limit = (vb->arrayStride == 0) ? g_limits.maxVertexBufferArrayStride : vb->arrayStride;
            if (a->offset + asize > stride_limit) {
                err("%s: vertex buffer %u attribute %u (offset %llu, size %u) exceeds the stride %llu", l, (unsigned)i, (unsigned)j, LL(a->offset), asize, LL(stride_limit));
            }
            const uint32_t align = (asize < 4) ? asize : 4;
            if ((a->offset % align) != 0) {
                err("%s: vertex buffer %u attribute %u offset %llu is not aligned to %u", l, (unsigned)i, (unsigned)j, LL(a->offset), align);
            }
            if (a->shaderLocation >= g_limits.maxVertexAttributes) {
                err("%s: vertex attribute shader location %u exceeds the limit", l, a->shaderLocation);
            }
            for (size_t k = 0; k < i; k++) {
                for (int m = 0; m < pi->vertex_buffers[k].num_attributes; m++) {
                    if (pi->vertex_buffers[k].attributes[m].shaderLocation == a->shaderLocation) {
                        err("%s: duplicate vertex attribute shader location %u", l, a->shaderLocation);
                    }
                }
            }
            for (size_t m = 0; m < j; m++) {
                if (out->attributes[m].shaderLocation == a->shaderLocation) {
                    err("%s: duplicate vertex attribute shader location %u", l, a->shaderLocation);
                }
            }
        }
    }
    if (total_attrs > g_limits.maxVertexAttributes) {
        err("%s: %u vertex attributes exceed the limit of %u", l, total_attrs, g_limits.maxVertexAttributes);
    }

    // primitive state
    pi->primitive = d->primitive;
    if ((d->primitive.stripIndexFormat != WGPUIndexFormat_Undefined) && !is_strip_topology(d->primitive.topology)) {
        err("%s: stripIndexFormat is only valid for strip topologies", l);
    }
    if (d->primitive.unclippedDepth && !has_feature(WGPUFeatureName_DepthClipControl)) {
        err("%s: unclippedDepth needs the DepthClipControl feature", l);
    }

    // depth-stencil state
    if (d->depthStencil) {
        const WGPUDepthStencilState* ds = d->depthStencil;
        pi->has_depth_stencil = true;
        pi->depth_stencil = *ds;
        const fmt_info_t* fi = fmt_info(ds->format);
        if (!fi || !(fi->flags & (F_DEPTH | F_STENCIL))) {
            err("%s: depth-stencil format %d is not a depth/stencil format", l, (int)ds->format);
        } else {
            if (!fmt_supported(ds->format)) {
                err("%s: depth-stencil format %d needs a feature which is not enabled", l, (int)ds->format);
            }
            const bool front_active = stencil_face_active(&ds->stencilFront);
            const bool back_active = stencil_face_active(&ds->stencilBack);
            if ((fi->flags & F_DEPTH) && (ds->depthWriteEnabled == WGPUOptionalBool_Undefined)) {
                err("%s: depthWriteEnabled must be defined for a format with a depth aspect", l);
            }
            const bool depth_write = (ds->depthWriteEnabled == WGPUOptionalBool_True);
            const bool dfail = stencil_face_depth_fail_active(&ds->stencilFront) || stencil_face_depth_fail_active(&ds->stencilBack);
            if ((fi->flags & F_DEPTH) && (depth_write || dfail) && (ds->depthCompare == WGPUCompareFunction_Undefined)) {
                err("%s: depthCompare must be defined when depth writes are enabled or the stencil depthFailOp is not Keep", l);
            }
            if (!(fi->flags & F_DEPTH) && (depth_write || (ds->depthCompare != WGPUCompareFunction_Undefined && ds->depthCompare != WGPUCompareFunction_Always))) {
                err("%s: depth state is set for a format without a depth aspect", l);
            }
            if (!(fi->flags & F_STENCIL) && (front_active || back_active)) {
                err("%s: stencil state is set for a format without a stencil aspect", l);
            }
        }
        const bool no_bias = (d->primitive.topology == WGPUPrimitiveTopology_LineList) ||
                             (d->primitive.topology == WGPUPrimitiveTopology_LineStrip) ||
                             (d->primitive.topology == WGPUPrimitiveTopology_PointList);
        if (no_bias && ((ds->depthBias != 0) || (ds->depthBiasSlopeScale != 0.0f) || (ds->depthBiasClamp != 0.0f))) {
            err("%s: depth bias is not allowed for line and point topologies", l);
        }
    }

    // multisample state
    pi->multisample = d->multisample;
    if ((d->multisample.count != 1) && (d->multisample.count != 4)) {
        err("%s: invalid sample count %u", l, d->multisample.count);
    }
    if (d->multisample.alphaToCoverageEnabled && (d->multisample.count != 4)) {
        err("%s: alphaToCoverage needs a sample count of 4", l);
    }

    // fragment state
    bool any_target = false;
    if (d->fragment) {
        const WGPUFragmentState* fs = d->fragment;
        pi->has_fragment = true;
        pi->fragment_module = fs->module;
        obj_t* fm = lookup(fs->module, WGPU_MOCK_OBJ_SHADER_MODULE, l);
        add_ref(o, fm);
        if (fm) {
            entry_point_ok(l, "fragment", fm, fs->entryPoint, pi->fragment_entry_point, sizeof(pi->fragment_entry_point));
        }
        size_t nt = fs->targetCount;
        if (nt > 0 && !fs->targets) {
            err("%s: target count is %u but targets is null", l, (unsigned)nt);
            nt = 0;
        }
        if (nt > g_limits.maxColorAttachments) {
            err("%s: %u color targets exceed the limit of %u", l, (unsigned)nt, g_limits.maxColorAttachments);
        }
        if (nt > WGPU_MOCK_MAX_COLOR_ATTACHMENTS) {
            nt = WGPU_MOCK_MAX_COLOR_ATTACHMENTS;
        }
        pi->num_targets = (int)nt;
        uint32_t bytes_per_sample = 0;
        for (size_t i = 0; i < nt; i++) {
            const WGPUColorTargetState* t = &fs->targets[i];
            wgpu_mock_color_target_t* out = &pi->targets[i];
            out->format = t->format;
            out->write_mask = t->writeMask;
            if (t->blend) {
                out->has_blend = true;
                out->blend = *t->blend;
            }
            if (t->format == WGPUTextureFormat_Undefined) {
                continue;
            }
            any_target = true;
            if (!fmt_info(t->format) || !fmt_renderable(t->format)) {
                err("%s: color target %u format %d is not renderable", l, (unsigned)i, (int)t->format);
                continue;
            }
            const fmt_info_t* fi = fmt_info(t->format);
            if (fi->flags & (F_DEPTH | F_STENCIL)) {
                err("%s: color target %u has a depth/stencil format", l, (unsigned)i);
            }
            if (!fmt_supported(t->format)) {
                err("%s: color target %u format %d needs a feature which is not enabled", l, (unsigned)i, (int)t->format);
            }
            if (t->blend && !fmt_blendable(t->format)) {
                err("%s: color target %u format %d is not blendable", l, (unsigned)i, (int)t->format);
            }
            if (t->writeMask > 0xF) {
                err("%s: color target %u has an invalid write mask 0x%llx", l, (unsigned)i, LL(t->writeMask));
            }
            if (t->blend) {
                check_blend_component(l, "color", &t->blend->color);
                check_blend_component(l, "alpha", &t->blend->alpha);
            }
            bytes_per_sample += fmt_target_cost(t->format);
        }
        if (bytes_per_sample > g_limits.maxColorAttachmentBytesPerSample) {
            err("%s: color targets use %u bytes per sample, the limit is %u", l, bytes_per_sample, g_limits.maxColorAttachmentBytesPerSample);
        }
    }
    if (!any_target && !d->depthStencil) {
        err("%s: pipeline has neither a color target nor a depth-stencil state", l);
    }
    return o;
}

static obj_t* compute_pipeline_create(const WGPUComputePipelineDescriptor* d) {
    obj_t* o = obj_new(WGPU_MOCK_OBJ_COMPUTE_PIPELINE);
    wgpu_mock_compute_pipeline_info_t* pi = &o->u.cp.info;
    sv_copy(pi->label, sizeof(pi->label), d->label);
    char lbuf[WGPU_MOCK_MAX_STRING + 32];
    snprintf(lbuf, sizeof(lbuf), "CreateComputePipeline(%s)", label_or_empty(pi->label));
    obj_t* layout = lookup(d->layout, WGPU_MOCK_OBJ_PIPELINE_LAYOUT, lbuf);
    pi->layout = d->layout;
    add_ref(o, layout);
    obj_t* m = lookup(d->compute.module, WGPU_MOCK_OBJ_SHADER_MODULE, lbuf);
    pi->module = d->compute.module;
    add_ref(o, m);
    if (m) {
        entry_point_ok(lbuf, "compute", m, d->compute.entryPoint, pi->entry_point, sizeof(pi->entry_point));
    }
    return o;
}

// ---- copy helpers
// bytes needed in a linear buffer for a texture copy, returns false (and records messages) if the layout is invalid
static bool copy_layout_required(const fmt_info_t* fi, const WGPUExtent3D* ext, const WGPUTexelCopyBufferLayout* layout, const char* fn, uint64_t* out_required) {
    *out_required = 0;
    if ((ext->width == 0) || (ext->height == 0) || (ext->depthOrArrayLayers == 0)) {
        return true;
    }
    bool ok = true;
    const uint64_t wb = ext->width / fi->bw;
    const uint64_t hb = ext->height / fi->bh;
    const uint64_t d = ext->depthOrArrayLayers;
    const uint64_t last_row = wb * fi->bytes;
    if (((hb > 1) || (d > 1)) && (layout->bytesPerRow == WGPU_COPY_STRIDE_UNDEFINED)) {
        err("%s: bytesPerRow is required for copies with more than one row", fn);
        ok = false;
    }
    if ((d > 1) && (layout->rowsPerImage == WGPU_COPY_STRIDE_UNDEFINED)) {
        err("%s: rowsPerImage is required for copies with more than one image", fn);
        ok = false;
    }
    if ((layout->bytesPerRow != WGPU_COPY_STRIDE_UNDEFINED) && (layout->bytesPerRow < last_row)) {
        err("%s: bytesPerRow %u is smaller than the %llu bytes of a row", fn, layout->bytesPerRow, LL(last_row));
        ok = false;
    }
    if ((layout->rowsPerImage != WGPU_COPY_STRIDE_UNDEFINED) && (layout->rowsPerImage < hb)) {
        err("%s: rowsPerImage %u is smaller than the %llu rows of an image", fn, layout->rowsPerImage, LL(hb));
        ok = false;
    }
    if (!ok) {
        return false;
    }
    const uint64_t bpr = (layout->bytesPerRow == WGPU_COPY_STRIDE_UNDEFINED) ? last_row : layout->bytesPerRow;
    const uint64_t rpi = (layout->rowsPerImage == WGPU_COPY_STRIDE_UNDEFINED) ? hb : layout->rowsPerImage;
    *out_required = bpr * rpi * (d - 1) + bpr * (hb - 1) + last_row;
    return true;
}

// check origin and extent of a texture copy against a mip level
static bool tex_copy_bounds(const wgpu_mock_texture_info_t* ti, uint32_t mip, const WGPUOrigin3D* o, const WGPUExtent3D* e, bool whole, const char* fn) {
    const fmt_info_t* fi = fmt_info(ti->format);
    if (!fi) {
        return false;
    }
    if (mip >= ti->mip_level_count) {
        err("%s: mip level %u is out of range (texture has %u)", fn, mip, ti->mip_level_count);
        return false;
    }
    bool ok = true;
    const uint32_t pw = roundup_u32(mip_dim(ti->width, mip), fi->bw);
    const uint32_t ph = roundup_u32(mip_dim(ti->height, mip), fi->bh);
    const uint32_t pd = (ti->dimension == WGPUTextureDimension_3D) ? mip_dim(ti->depth_or_layers, mip) : tex_layers(ti);
    if (((o->x % fi->bw) != 0) || ((o->y % fi->bh) != 0) || ((e->width % fi->bw) != 0) || ((e->height % fi->bh) != 0)) {
        err("%s: origin (%u,%u) or size (%u,%u) is not a multiple of the %ux%u block size", fn, o->x, o->y, e->width, e->height, fi->bw, fi->bh);
        ok = false;
    }
    if (!range_ok(o->x, e->width, pw) || !range_ok(o->y, e->height, ph) || !range_ok(o->z, e->depthOrArrayLayers, pd)) {
        err("%s: copy box (%u,%u,%u)+(%u,%u,%u) exceeds the mip level size (%u,%u,%u)", fn, o->x, o->y, o->z, e->width, e->height, e->depthOrArrayLayers, pw, ph, pd);
        ok = false;
    }
    if (whole && ((fi->flags & (F_DEPTH | F_STENCIL)) || (ti->sample_count > 1))) {
        if ((o->x != 0) || (o->y != 0) || (e->width != pw) || (e->height != ph)) {
            err("%s: depth/stencil and multisampled textures must be copied as whole subresources", fn);
            ok = false;
        }
    }
    return ok;
}

static bool is_finite_f(double v) {
    return isfinite(v) != 0;
}

// ---- passes
static void begin_pass_common_checks(obj_t* enc, const char* fn) {
    if (enc->u.enc.finished) {
        err("%s: command encoder is already finished", fn);
    }
    if (enc->u.enc.pass_open) {
        err("%s: another pass is still open on the command encoder", fn);
    }
}

// size and sample count of all attachments of a pass must match
typedef struct {
    bool have;
    uint32_t w, h, samples;
} att_size_t;

static void note_attachment(att_size_t* as, const obj_t* view, const char* what, int index) {
    const wgpu_mock_texture_view_info_t* vi = &view->u.view.info;
    const wgpu_mock_texture_info_t* ti = &((const obj_t*)vi->texture)->u.texture.info;
    const uint32_t w = mip_dim(ti->width, vi->base_mip_level);
    const uint32_t h = mip_dim(ti->height, vi->base_mip_level);
    if (!as->have) {
        as->have = true;
        as->w = w;
        as->h = h;
        as->samples = ti->sample_count;
    } else if ((as->w != w) || (as->h != h) || (as->samples != ti->sample_count)) {
        err("BeginRenderPass: %s %d has size %ux%u/%u samples, different from the other attachments (%ux%u/%u samples)", what, index, w, h, ti->sample_count, as->w, as->h, as->samples);
    }
}

static obj_t* begin_render_pass(obj_t* enc, const WGPURenderPassDescriptor* d) {
    const char* fn = "BeginRenderPass";
    obj_t* o = obj_new(WGPU_MOCK_OBJ_RENDER_PASS_ENCODER);
    o->u.pass.enc = enc;
    add_ref(o, enc);
    memset(&g_last_render_pass, 0, sizeof(g_last_render_pass));
    memset(&g_rp, 0, sizeof(g_rp));
    memset(&g_rpe, 0, sizeof(g_rpe));
    g_cur_rpe = o;
    g_cmd.num_render_passes++;
    begin_pass_common_checks(enc, fn);
    enc->u.enc.pass_open = true;
    if (!d) {
        err("%s: descriptor is null", fn);
        return o;
    }
    sv_copy(g_last_render_pass.label, sizeof(g_last_render_pass.label), d->label);
    if (d->occlusionQuerySet || d->timestampWrites) {
        err("%s: query sets and timestamp writes are not supported by the mock", fn);
    }
    size_t n = d->colorAttachmentCount;
    if (n > g_limits.maxColorAttachments) {
        err("%s: %u color attachments exceed the limit of %u", fn, (unsigned)n, g_limits.maxColorAttachments);
    }
    if (n > WGPU_MOCK_MAX_COLOR_ATTACHMENTS) {
        n = WGPU_MOCK_MAX_COLOR_ATTACHMENTS;
    }
    if (n > 0 && !d->colorAttachments) {
        err("%s: color attachment count is %u but colorAttachments is null", fn, (unsigned)n);
        n = 0;
    }
    g_last_render_pass.num_color_attachments = (int)n;
    g_rp.num_color = (int)n;
    att_size_t as;
    memset(&as, 0, sizeof(as));
    uint32_t bytes_per_sample = 0;
    for (size_t i = 0; i < n; i++) {
        const WGPURenderPassColorAttachment* a = &d->colorAttachments[i];
        wgpu_mock_color_attachment_t* out = &g_last_render_pass.color_attachments[i];
        out->view = a->view;
        out->resolve_target = a->resolveTarget;
        out->depth_slice = a->depthSlice;
        out->load_op = a->loadOp;
        out->store_op = a->storeOp;
        out->clear_value = a->clearValue;
        if (!a->view) {
            continue;
        }
        obj_t* v = lookup(a->view, WGPU_MOCK_OBJ_TEXTURE_VIEW, fn);
        if (!v) {
            continue;
        }
        const wgpu_mock_texture_view_info_t* vi = &v->u.view.info;
        const obj_t* tex = view_texture(v);
        const wgpu_mock_texture_info_t* ti = &tex->u.texture.info;
        g_rp.color_fmt[i] = vi->format;
        note_attachment(&as, v, "color attachment", (int)i);
        bytes_per_sample += fmt_target_cost(vi->format);
        if (!(v->u.view.usage & WGPUTextureUsage_RenderAttachment)) {
            err("%s: color attachment %u view lacks the RenderAttachment usage", fn, (unsigned)i);
        }
        if (!fmt_renderable(vi->format) || (fmt_info(vi->format)->flags & (F_DEPTH | F_STENCIL))) {
            err("%s: color attachment %u format %d is not a renderable color format", fn, (unsigned)i, (int)vi->format);
        }
        if ((vi->mip_level_count != 1) || (vi->array_layer_count != 1)) {
            err("%s: color attachment %u view must have exactly one mip level and one array layer", fn, (unsigned)i);
        }
        if (vi->aspect != WGPUTextureAspect_All) {
            err("%s: color attachment %u view must use the All aspect", fn, (unsigned)i);
        }
        if ((vi->dimension != WGPUTextureViewDimension_2D) && (vi->dimension != WGPUTextureViewDimension_2DArray) &&
            (vi->dimension != WGPUTextureViewDimension_3D))
        {
            err("%s: color attachment %u view dimension %d is not renderable", fn, (unsigned)i, (int)vi->dimension);
        }
        if (vi->dimension == WGPUTextureViewDimension_3D) {
            if (a->depthSlice == WGPU_DEPTH_SLICE_UNDEFINED) {
                err("%s: color attachment %u is a 3D view and needs a depthSlice", fn, (unsigned)i);
            } else if (a->depthSlice >= mip_dim(ti->depth_or_layers, vi->base_mip_level)) {
                err("%s: color attachment %u depthSlice %u is out of range", fn, (unsigned)i, a->depthSlice);
            }
        } else if (a->depthSlice != WGPU_DEPTH_SLICE_UNDEFINED) {
            err("%s: color attachment %u is not a 3D view and must not set a depthSlice (is %u)", fn, (unsigned)i, a->depthSlice);
        }
        if ((a->loadOp != WGPULoadOp_Load) && (a->loadOp != WGPULoadOp_Clear)) {
            err("%s: color attachment %u has an undefined loadOp", fn, (unsigned)i);
        }
        if ((a->storeOp != WGPUStoreOp_Store) && (a->storeOp != WGPUStoreOp_Discard)) {
            err("%s: color attachment %u has an undefined storeOp", fn, (unsigned)i);
        }
        if ((a->loadOp == WGPULoadOp_Clear) &&
            !(is_finite_f(a->clearValue.r) && is_finite_f(a->clearValue.g) && is_finite_f(a->clearValue.b) && is_finite_f(a->clearValue.a)))
        {
            err("%s: color attachment %u has a non-finite clear value", fn, (unsigned)i);
        }
        if (a->resolveTarget) {
            obj_t* rv = lookup(a->resolveTarget, WGPU_MOCK_OBJ_TEXTURE_VIEW, fn);
            if (rv) {
                const wgpu_mock_texture_view_info_t* ri = &rv->u.view.info;
                const obj_t* rtex = view_texture(rv);
                const wgpu_mock_texture_info_t* rti = &rtex->u.texture.info;
                if (ti->sample_count <= 1) {
                    err("%s: color attachment %u has a resolve target but is not multisampled", fn, (unsigned)i);
                }
                if (rti->sample_count != 1) {
                    err("%s: resolve target of color attachment %u must not be multisampled", fn, (unsigned)i);
                }
                if (ri->format != vi->format) {
                    err("%s: resolve target of color attachment %u has a different format", fn, (unsigned)i);
                }
                if ((mip_dim(rti->width, ri->base_mip_level) != mip_dim(ti->width, vi->base_mip_level)) ||
                    (mip_dim(rti->height, ri->base_mip_level) != mip_dim(ti->height, vi->base_mip_level)))
                {
                    err("%s: resolve target of color attachment %u has a different size", fn, (unsigned)i);
                }
                if (!(rv->u.view.usage & WGPUTextureUsage_RenderAttachment)) {
                    err("%s: resolve target of color attachment %u lacks the RenderAttachment usage", fn, (unsigned)i);
                }
                if ((ri->mip_level_count != 1) || (ri->array_layer_count != 1)) {
                    err("%s: resolve target of color attachment %u must have one mip level and one layer", fn, (unsigned)i);
                }
                if (rtex == tex) {
                    err("%s: color attachment %u resolves into its own texture", fn, (unsigned)i);
                }
            }
        }
    }
    if (bytes_per_sample > g_limits.maxColorAttachmentBytesPerSample) {
        err("%s: color attachments use %u bytes per sample, the limit is %u", fn, bytes_per_sample, g_limits.maxColorAttachmentBytesPerSample);
    }
    if (d->depthStencilAttachment) {
        const WGPURenderPassDepthStencilAttachment* a = d->depthStencilAttachment;
        g_last_render_pass.has_depth_stencil = true;
        g_last_render_pass.ds_view = a->view;
        g_last_render_pass.depth_load_op = a->depthLoadOp;
        g_last_render_pass.depth_store_op = a->depthStoreOp;
        g_last_render_pass.depth_clear_value = a->depthClearValue;
        g_last_render_pass.depth_read_only = (a->depthReadOnly != 0);
        g_last_render_pass.stencil_load_op = a->stencilLoadOp;
        g_last_render_pass.stencil_store_op = a->stencilStoreOp;
        g_last_render_pass.stencil_clear_value = a->stencilClearValue;
        g_last_render_pass.stencil_read_only = (a->stencilReadOnly != 0);
        obj_t* v = lookup(a->view, WGPU_MOCK_OBJ_TEXTURE_VIEW, fn);
        if (v) {
            const wgpu_mock_texture_view_info_t* vi = &v->u.view.info;
            const fmt_info_t* fi = fmt_info(vi->format);
            g_rp.ds_fmt = vi->format;
            note_attachment(&as, v, "depth-stencil attachment", 0);
            if (!(v->u.view.usage & WGPUTextureUsage_RenderAttachment)) {
                err("%s: depth-stencil view lacks the RenderAttachment usage", fn);
            }
            if (!fi || !(fi->flags & (F_DEPTH | F_STENCIL))) {
                err("%s: depth-stencil attachment format %d is not a depth/stencil format", fn, (int)vi->format);
            } else {
                if ((vi->mip_level_count != 1) || (vi->array_layer_count != 1)) {
                    err("%s: depth-stencil view must have exactly one mip level and one array layer", fn);
                }
                if (vi->aspect != WGPUTextureAspect_All) {
                    err("%s: depth-stencil view must use the All aspect", fn);
                }
                const bool has_depth = (fi->flags & F_DEPTH) != 0;
                const bool has_stencil = (fi->flags & F_STENCIL) != 0;
                const bool depth_ops = (a->depthLoadOp != WGPULoadOp_Undefined) || (a->depthStoreOp != WGPUStoreOp_Undefined);
                const bool stencil_ops = (a->stencilLoadOp != WGPULoadOp_Undefined) || (a->stencilStoreOp != WGPUStoreOp_Undefined);
                if (has_depth) {
                    if (a->depthReadOnly) {
                        if (depth_ops) {
                            err("%s: depth ops must be undefined if depthReadOnly is set", fn);
                        }
                    } else {
                        if ((a->depthLoadOp == WGPULoadOp_Undefined) || (a->depthStoreOp == WGPUStoreOp_Undefined)) {
                            err("%s: depth load and store ops must be defined if depth is not read-only", fn);
                        }
                        if ((a->depthLoadOp == WGPULoadOp_Clear) && !(a->depthClearValue >= 0.0f && a->depthClearValue <= 1.0f)) {
                            err("%s: depthClearValue %f is not in the range [0, 1]", fn, (double)a->depthClearValue);
                        }
                    }
                } else if (depth_ops || a->depthReadOnly) {
                    err("%s: depth ops or depthReadOnly are set for a format without a depth aspect", fn);
                }
                if (has_stencil) {
                    if (a->stencilReadOnly) {
                        if (stencil_ops) {
                            err("%s: stencil ops must be undefined if stencilReadOnly is set", fn);
                        }
                    } else if ((a->stencilLoadOp == WGPULoadOp_Undefined) || (a->stencilStoreOp == WGPUStoreOp_Undefined)) {
                        err("%s: stencil load and store ops must be defined if stencil is not read-only", fn);
                    }
                } else if (stencil_ops || a->stencilReadOnly) {
                    err("%s: stencil ops or stencilReadOnly are set for a format without a stencil aspect", fn);
                }
            }
        }
    }
    if ((n == 0) && !d->depthStencilAttachment) {
        err("%s: a render pass needs at least one attachment", fn);
    }
    bool any_color = false;
    for (size_t i = 0; i < n; i++) {
        any_color |= (d->colorAttachments[i].view != 0);
    }
    if (!any_color && !d->depthStencilAttachment && (n > 0)) {
        err("%s: all color attachments are empty and there is no depth-stencil attachment", fn);
    }
    g_rp.sample_count = as.samples;
    g_rp.width = as.w;
    g_rp.height = as.h;
    return o;
}

static obj_t* begin_compute_pass(obj_t* enc, const WGPUComputePassDescriptor* d) {
    const char* fn = "BeginComputePass";
    obj_t* o = obj_new(WGPU_MOCK_OBJ_COMPUTE_PASS_ENCODER);
    o->u.pass.enc = enc;
    add_ref(o, enc);
    memset(&g_cpe, 0, sizeof(g_cpe));
    g_cur_cpe = o;
    g_cmd.num_compute_passes++;
    begin_pass_common_checks(enc, fn);
    enc->u.enc.pass_open = true;
    if (d) {
        sv_copy(g_cpe.label, sizeof(g_cpe.label), d->label);
        if (d->timestampWrites) {
            err("%s: timestamp writes are not supported by the mock", fn);
        }
    }
    return o;
}

// validate a render pass encoder handle: must be the current, not yet ended pass
static obj_t* rpass(const void* h, const char* fn) {
    obj_t* o = lookup(h, WGPU_MOCK_OBJ_RENDER_PASS_ENCODER, fn);
    if (!o) {
        return 0;
    }
    if (o->u.pass.ended) {
        err("%s: render pass is already ended", fn);
        return 0;
    }
    if (o != g_cur_rpe) {
        err("%s: not the current render pass", fn);
        return 0;
    }
    return o;
}

static obj_t* cpass(const void* h, const char* fn) {
    obj_t* o = lookup(h, WGPU_MOCK_OBJ_COMPUTE_PASS_ENCODER, fn);
    if (!o) {
        return 0;
    }
    if (o->u.pass.ended) {
        err("%s: compute pass is already ended", fn);
        return 0;
    }
    if (o != g_cur_cpe) {
        err("%s: not the current compute pass", fn);
        return 0;
    }
    return o;
}

static void end_pass(obj_t* pass) {
    pass->u.pass.ended = true;
    pass->u.pass.enc->u.enc.pass_open = false;
}

// size of a buffer binding on a pass encoder
static uint64_t bound_size(const wgpu_mock_buffer_binding_t* b) {
    const obj_t* buf = (const obj_t*)b->buffer;
    const uint64_t total = buf->u.buffer.info.size;
    if (b->size == WGPU_WHOLE_SIZE) {
        return (b->offset <= total) ? (total - b->offset) : 0;
    }
    return b->size;
}

static void set_bind_group(wgpu_mock_bind_slot_t* slots, uint32_t index, WGPUBindGroup group, size_t n, const uint32_t* offs, const char* fn) {
    if ((index >= g_limits.maxBindGroups) || (index >= WGPU_MOCK_MAX_BIND_GROUPS)) {
        err("%s: bind group index %u is out of range", fn, index);
        return;
    }
    wgpu_mock_bind_slot_t* s = &slots[index];
    s->num_set_calls++;
    s->group = 0;
    s->num_dynamic_offsets = 0;
    if (!group) {
        if (n != 0) {
            err("%s: dynamic offsets are not allowed when clearing a bind group", fn);
        }
        return;
    }
    obj_t* bg = lookup(group, WGPU_MOCK_OBJ_BIND_GROUP, fn);
    if (!bg) {
        return;
    }
    const obj_t* bgl = (const obj_t*)bg->u.bg.info.layout;
    const wgpu_mock_bind_group_layout_info_t* li = &bgl->u.bgl.info;
    if ((int)n != li->num_dynamic_offsets) {
        err("%s: bind group %u needs %d dynamic offsets but %u were provided", fn, index, li->num_dynamic_offsets, (unsigned)n);
        return;
    }
    if (n > WGPU_MOCK_MAX_DYN_OFFSETS) {
        err("%s: %u dynamic offsets exceed the mock limit of %d", fn, (unsigned)n, WGPU_MOCK_MAX_DYN_OFFSETS);
        return;
    }
    if ((n > 0) && !offs) {
        err("%s: dynamic offsets are null", fn);
        return;
    }
    // dynamic offsets are applied in order of the binding numbers
    int idx[WGPU_MOCK_MAX_BIND_ENTRIES];
    int cnt = 0;
    for (int i = 0; i < li->num_entries; i++) {
        if (bgl_entry_is_buffer(&li->entries[i]) && li->entries[i].buffer.hasDynamicOffset) {
            int j = cnt++;
            while ((j > 0) && (li->entries[idx[j - 1]].binding > li->entries[i].binding)) {
                idx[j] = idx[j - 1];
                j--;
            }
            idx[j] = i;
        }
    }
    for (size_t k = 0; k < n; k++) {
        const WGPUBindGroupLayoutEntry* le = &li->entries[idx[k]];
        const uint32_t align = bgl_entry_is_storage_buffer(le) ? g_limits.minStorageBufferOffsetAlignment : g_limits.minUniformBufferOffsetAlignment;
        if ((align > 0) && ((offs[k] % align) != 0)) {
            err("%s: dynamic offset %u for binding %u is not a multiple of %u", fn, offs[k], le->binding, align);
        }
        for (int e = 0; e < bg->u.bg.info.num_entries; e++) {
            const WGPUBindGroupEntry* be = &bg->u.bg.info.entries[e];
            if ((be->binding == le->binding) && be->buffer) {
                const obj_t* b = (const obj_t*)be->buffer;
                const uint64_t size = buffer_binding_size(b, be->offset, be->size);
                if (be->offset + offs[k] + size > b->u.buffer.info.size) {
                    err("%s: dynamic offset %u for binding %u moves the range (offset %llu, size %llu) beyond the buffer size %llu", fn, offs[k], le->binding, LL(be->offset), LL(size), LL(b->u.buffer.info.size));
                }
            }
        }
    }
    s->group = group;
    s->num_dynamic_offsets = (int)n;
    for (size_t k = 0; k < n; k++) {
        s->dynamic_offsets[k] = offs[k];
    }
}

// all bind groups required by a pipeline layout must be set and compatible
static void check_bind_groups(const wgpu_mock_bind_slot_t* slots, const obj_t* pl, const char* fn) {
    for (int i = 0; i < pl->u.pl.info.num_bind_group_layouts; i++) {
        const wgpu_mock_bind_slot_t* s = &slots[i];
        const obj_t* want = (const obj_t*)pl->u.pl.info.bind_group_layouts[i];
        if (!s->group) {
            // an explicitly cleared (null) bind group is valid for an empty layout
            if ((s->num_set_calls == 0) || (want->u.bgl.info.num_entries != 0)) {
                err("%s: bind group %d required by the pipeline layout is not set", fn, i);
            }
            continue;
        }
        const obj_t* bg = (const obj_t*)s->group;
        const obj_t* have = (const obj_t*)bg->u.bg.info.layout;
        if (!bgl_compatible(have, want)) {
            err("%s: bind group %d has a layout which is incompatible with the pipeline layout", fn, i);
        }
    }
}

static void render_set_pipeline(obj_t* pass, WGPURenderPipeline pip) {
    const char* fn = "RenderPassEncoderSetPipeline";
    (void)pass;
    obj_t* p = lookup(pip, WGPU_MOCK_OBJ_RENDER_PIPELINE, fn);
    if (!p) {
        return;
    }
    const wgpu_mock_render_pipeline_info_t* pi = &p->u.rp.info;
    g_rpe.pipeline = pip;
    if (pi->multisample.count != g_rp.sample_count) {
        err("%s: pipeline sample count %u does not match the pass sample count %u", fn, pi->multisample.count, g_rp.sample_count);
    }
    const int nt = (pi->num_targets > g_rp.num_color) ? pi->num_targets : g_rp.num_color;
    for (int i = 0; i < nt; i++) {
        const WGPUTextureFormat pf = (i < pi->num_targets) ? pi->targets[i].format : WGPUTextureFormat_Undefined;
        const WGPUTextureFormat af = (i < g_rp.num_color) ? g_rp.color_fmt[i] : WGPUTextureFormat_Undefined;
        if (pf != af) {
            err("%s: pipeline color target %d format %d does not match the pass attachment format %d", fn, i, (int)pf, (int)af);
        }
    }
    const WGPUTextureFormat pds = pi->has_depth_stencil ? pi->depth_stencil.format : WGPUTextureFormat_Undefined;
    if (pds != g_rp.ds_fmt) {
        err("%s: pipeline depth-stencil format %d does not match the pass depth-stencil format %d", fn, (int)pds, (int)g_rp.ds_fmt);
    }
}

static uint32_t index_format_size(WGPUIndexFormat f) {
    return (f == WGPUIndexFormat_Uint16) ? 2 : ((f == WGPUIndexFormat_Uint32) ? 4 : 0);
}

static void render_set_index_buffer(WGPUBuffer buf, WGPUIndexFormat fmt, uint64_t offset, uint64_t size) {
    const char* fn = "RenderPassEncoderSetIndexBuffer";
    obj_t* b = lookup(buf, WGPU_MOCK_OBJ_BUFFER, fn);
    if (!b) {
        return;
    }
    const wgpu_mock_buffer_info_t* bi = &b->u.buffer.info;
    const uint32_t isize = index_format_size(fmt);
    if (isize == 0) {
        err("%s: index format is undefined", fn);
    } else if ((offset % isize) != 0) {
        err("%s: index buffer offset %llu is not a multiple of the index size %u", fn, LL(offset), isize);
    }
    if (!(bi->usage & WGPUBufferUsage_Index)) {
        err("%s: buffer lacks the Index usage", fn);
    }
    if (bi->mapped) {
        err("%s: buffer is mapped", fn);
    }
    wgpu_mock_buffer_binding_t bb = { buf, offset, size };
    if (!range_ok(offset, bound_size(&bb), bi->size)) {
        err("%s: range (offset %llu, size %llu) exceeds buffer size %llu", fn, LL(offset), LL(size), LL(bi->size));
    }
    g_rpe.index_buffer = bb;
    g_rpe.index_format = fmt;
}

static void render_set_vertex_buffer(uint32_t slot, WGPUBuffer buf, uint64_t offset, uint64_t size) {
    const char* fn = "RenderPassEncoderSetVertexBuffer";
    if ((slot >= g_limits.maxVertexBuffers) || (slot >= WGPU_MOCK_MAX_VERTEX_BUFFERS)) {
        err("%s: vertex buffer slot %u is out of range", fn, slot);
        return;
    }
    wgpu_mock_buffer_binding_t bb = { buf, offset, size };
    if (!buf) {
        memset(&g_rpe.vertex_buffers[slot], 0, sizeof(bb));
        return;
    }
    obj_t* b = lookup(buf, WGPU_MOCK_OBJ_BUFFER, fn);
    if (!b) {
        return;
    }
    const wgpu_mock_buffer_info_t* bi = &b->u.buffer.info;
    if (!(bi->usage & WGPUBufferUsage_Vertex)) {
        err("%s: buffer lacks the Vertex usage", fn);
    }
    if ((offset % 4) != 0) {
        err("%s: vertex buffer offset %llu is not a multiple of 4", fn, LL(offset));
    }
    if (bi->mapped) {
        err("%s: buffer is mapped", fn);
    }
    if (!range_ok(offset, bound_size(&bb), bi->size)) {
        err("%s: range (offset %llu, size %llu) exceeds buffer size %llu", fn, LL(offset), LL(size), LL(bi->size));
    }
    g_rpe.vertex_buffers[slot] = bb;
}

static void render_check_draw(const char* fn, bool indexed, uint32_t count, uint32_t instance_count, uint32_t first, uint32_t first_instance) {
    const obj_t* p = (const obj_t*)g_rpe.pipeline;
    if (!p) {
        err("%s: no pipeline is set", fn);
        return;
    }
    const wgpu_mock_render_pipeline_info_t* pi = &p->u.rp.info;
    check_bind_groups(g_rpe.bind_groups, (const obj_t*)pi->layout, fn);
    for (int i = 0; i < pi->num_vertex_buffers; i++) {
        const wgpu_mock_vertex_buffer_layout_t* vb = &pi->vertex_buffers[i];
        if (vb->num_attributes == 0) {
            continue;
        }
        const wgpu_mock_buffer_binding_t* bb = &g_rpe.vertex_buffers[i];
        if (!bb->buffer) {
            err("%s: vertex buffer slot %d required by the pipeline is not set", fn, i);
            continue;
        }
        const bool instanced = (vb->step_mode == WGPUVertexStepMode_Instance);
        if (indexed && !instanced) {
            continue;
        }
        const uint64_t n = instanced ? ((uint64_t)first_instance + instance_count) : ((uint64_t)first + count);
        const uint64_t n_valid = instanced ? instance_count : count;
        if (n_valid == 0) {
            continue;
        }
        uint64_t last = 0;
        for (int a = 0; a < vb->num_attributes; a++) {
            const uint64_t end = vb->attributes[a].offset + vertex_format_size(vb->attributes[a].format);
            if (end > last) {
                last = end;
            }
        }
        const uint64_t required = vb->array_stride * (n - 1) + last;
        if (required > bound_size(bb)) {
            err("%s: vertex buffer slot %d is too small, %llu bytes needed but %llu bound", fn, i, LL(required), LL(bound_size(bb)));
        }
    }
    if (indexed) {
        if (!g_rpe.index_buffer.buffer) {
            err("%s: no index buffer is set", fn);
            return;
        }
        const uint64_t required = ((uint64_t)first + count) * index_format_size(g_rpe.index_format);
        if ((count > 0) && (required > bound_size(&g_rpe.index_buffer))) {
            err("%s: index buffer is too small, %llu bytes needed but %llu bound", fn, LL(required), LL(bound_size(&g_rpe.index_buffer)));
        }
        if (is_strip_topology(pi->primitive.topology) && (pi->primitive.stripIndexFormat != g_rpe.index_format)) {
            err("%s: pipeline stripIndexFormat %d does not match the index format %d", fn, (int)pi->primitive.stripIndexFormat, (int)g_rpe.index_format);
        }
    }
}

// ---- copies
static void copy_buffer_to_buffer(obj_t* enc, WGPUBuffer src, uint64_t src_offset, WGPUBuffer dst, uint64_t dst_offset, uint64_t size) {
    const char* fn = "CommandEncoderCopyBufferToBuffer";
    if (enc->u.enc.finished) {
        err("%s: command encoder is already finished", fn);
    }
    if (enc->u.enc.pass_open) {
        err("%s: a pass is still open on the command encoder", fn);
    }
    obj_t* s = lookup(src, WGPU_MOCK_OBJ_BUFFER, fn);
    obj_t* d = lookup(dst, WGPU_MOCK_OBJ_BUFFER, fn);
    g_cmd.num_buffer_to_buffer_copies++;
    g_cmd.buffer_copy_src = src;
    g_cmd.buffer_copy_dst = dst;
    g_cmd.buffer_copy_src_offset = src_offset;
    g_cmd.buffer_copy_dst_offset = dst_offset;
    g_cmd.buffer_copy_size = size;
    if (!s || !d) {
        return;
    }
    bool ok = true;
    if (!(s->u.buffer.info.usage & WGPUBufferUsage_CopySrc)) {
        err("%s: source buffer lacks the CopySrc usage", fn);
        ok = false;
    }
    if (!(d->u.buffer.info.usage & WGPUBufferUsage_CopyDst)) {
        err("%s: destination buffer lacks the CopyDst usage", fn);
        ok = false;
    }
    if (((size % 4) != 0) || ((src_offset % 4) != 0) || ((dst_offset % 4) != 0)) {
        err("%s: offsets (%llu, %llu) and size %llu must be multiples of 4", fn, LL(src_offset), LL(dst_offset), LL(size));
        ok = false;
    }
    if (!range_ok(src_offset, size, s->u.buffer.info.size) || !range_ok(dst_offset, size, d->u.buffer.info.size)) {
        err("%s: copy range exceeds the source (size %llu) or destination (size %llu) buffer", fn, LL(s->u.buffer.info.size), LL(d->u.buffer.info.size));
        ok = false;
    }
    if ((s == d) && (src_offset < dst_offset + size) && (dst_offset < src_offset + size)) {
        err("%s: source and destination ranges overlap", fn);
        ok = false;
    }
    if (ok && s->u.buffer.data && d->u.buffer.data) {
        memmove(d->u.buffer.data + dst_offset, s->u.buffer.data + src_offset, (size_t)size);
    }
}

static void copy_buffer_to_texture(obj_t* enc, const WGPUTexelCopyBufferInfo* src, const WGPUTexelCopyTextureInfo* dst, const WGPUExtent3D* size) {
    const char* fn = "CommandEncoderCopyBufferToTexture";
    if (enc->u.enc.finished) {
        err("%s: command encoder is already finished", fn);
    }
    if (enc->u.enc.pass_open) {
        err("%s: a pass is still open on the command encoder", fn);
    }
    if (!src || !dst || !size) {
        err("%s: null argument", fn);
        return;
    }
    obj_t* b = lookup(src->buffer, WGPU_MOCK_OBJ_BUFFER, fn);
    obj_t* t = lookup(dst->texture, WGPU_MOCK_OBJ_TEXTURE, fn);
    g_cmd.num_buffer_to_texture_copies++;
    g_cmd.b2t_buffer = src->buffer;
    g_cmd.b2t_layout = src->layout;
    g_cmd.b2t_texture = dst->texture;
    g_cmd.b2t_mip_level = dst->mipLevel;
    g_cmd.b2t_origin = dst->origin;
    g_cmd.b2t_size = *size;
    if (!b || !t) {
        return;
    }
    const wgpu_mock_texture_info_t* ti = &t->u.texture.info;
    const fmt_info_t* fi = fmt_info(ti->format);
    if (!fi) {
        return;
    }
    if (!(b->u.buffer.info.usage & WGPUBufferUsage_CopySrc)) {
        err("%s: buffer lacks the CopySrc usage", fn);
    }
    if (!(ti->usage & WGPUTextureUsage_CopyDst)) {
        err("%s: texture lacks the CopyDst usage", fn);
    }
    if (ti->sample_count != 1) {
        err("%s: multisampled textures cannot be copy destinations", fn);
    }
    if (fi->flags & F_DEPTH) {
        err("%s: the depth aspect of format %d cannot be written by a buffer copy", fn, (int)ti->format);
    }
    if ((dst->aspect != WGPUTextureAspect_All) && (dst->aspect != WGPUTextureAspect_Undefined)) {
        err("%s: only the All aspect is supported by the mock", fn);
    }
    if ((src->layout.bytesPerRow != WGPU_COPY_STRIDE_UNDEFINED) && ((src->layout.bytesPerRow % 256) != 0)) {
        err("%s: bytesPerRow %u is not a multiple of 256", fn, src->layout.bytesPerRow);
    }
    if ((src->layout.offset % fi->bytes) != 0) {
        err("%s: buffer offset %llu is not a multiple of the texel block size %u", fn, LL(src->layout.offset), fi->bytes);
    }
    if (!tex_copy_bounds(ti, dst->mipLevel, &dst->origin, size, false, fn)) {
        return;
    }
    uint64_t required = 0;
    if (copy_layout_required(fi, size, &src->layout, fn, &required)) {
        if (!range_ok(src->layout.offset, required, b->u.buffer.info.size)) {
            err("%s: copy needs %llu bytes at offset %llu but the buffer has only %llu bytes", fn, LL(required), LL(src->layout.offset), LL(b->u.buffer.info.size));
        }
    }
}

static bool tex_boxes_overlap(const wgpu_mock_texture_info_t* ti, const WGPUOrigin3D* a, const WGPUOrigin3D* b, const WGPUExtent3D* e) {
    const bool z = (a->z < b->z + e->depthOrArrayLayers) && (b->z < a->z + e->depthOrArrayLayers);
    if (ti->dimension != WGPUTextureDimension_3D) {
        return z;
    }
    const bool x = (a->x < b->x + e->width) && (b->x < a->x + e->width);
    const bool y = (a->y < b->y + e->height) && (b->y < a->y + e->height);
    return x && y && z;
}

static void copy_texture_to_texture(obj_t* enc, const WGPUTexelCopyTextureInfo* src, const WGPUTexelCopyTextureInfo* dst, const WGPUExtent3D* size) {
    const char* fn = "CommandEncoderCopyTextureToTexture";
    if (enc->u.enc.finished) {
        err("%s: command encoder is already finished", fn);
    }
    if (enc->u.enc.pass_open) {
        err("%s: a pass is still open on the command encoder", fn);
    }
    if (!src || !dst || !size) {
        err("%s: null argument", fn);
        return;
    }
    obj_t* s = lookup(src->texture, WGPU_MOCK_OBJ_TEXTURE, fn);
    obj_t* d = lookup(dst->texture, WGPU_MOCK_OBJ_TEXTURE, fn);
    g_cmd.num_texture_to_texture_copies++;
    g_cmd.t2t_src_texture = src->texture;
    g_cmd.t2t_src_mip_level = src->mipLevel;
    g_cmd.t2t_src_origin = src->origin;
    g_cmd.t2t_dst_texture = dst->texture;
    g_cmd.t2t_dst_mip_level = dst->mipLevel;
    g_cmd.t2t_dst_origin = dst->origin;
    g_cmd.t2t_size = *size;
    if (!s || !d) {
        return;
    }
    const wgpu_mock_texture_info_t* si = &s->u.texture.info;
    const wgpu_mock_texture_info_t* di = &d->u.texture.info;
    const fmt_info_t* sfi = fmt_info(si->format);
    const fmt_info_t* dfi = fmt_info(di->format);
    if (!sfi || !dfi) {
        return;
    }
    if (!(si->usage & WGPUTextureUsage_CopySrc)) {
        err("%s: source texture lacks the CopySrc usage", fn);
    }
    if (!(di->usage & WGPUTextureUsage_CopyDst)) {
        err("%s: destination texture lacks the CopyDst usage", fn);
    }
    if (si->sample_count != di->sample_count) {
        err("%s: source and destination sample counts differ (%u, %u)", fn, si->sample_count, di->sample_count);
    }
    if (sfi->base != dfi->base) {
        err("%s: source format %d and destination format %d are not copy-compatible", fn, (int)si->format, (int)di->format);
    }
    const bool src_ok = tex_copy_bounds(si, src->mipLevel, &src->origin, size, true, fn);
    const bool dst_ok = tex_copy_bounds(di, dst->mipLevel, &dst->origin, size, true, fn);
    if (src_ok && dst_ok && (s == d) && (src->mipLevel == dst->mipLevel) && tex_boxes_overlap(si, &src->origin, &dst->origin, size)) {
        err("%s: source and destination subresources overlap", fn);
    }
}

// ---- queue writes
static void queue_write_buffer(WGPUBuffer buffer, uint64_t offset, const void* data, size_t size) {
    const char* fn = "QueueWriteBuffer";
    g_queue_state.num_write_buffers++;
    obj_t* b = lookup(buffer, WGPU_MOCK_OBJ_BUFFER, fn);
    if (!b) {
        return;
    }
    wgpu_mock_buffer_info_t* bi = &b->u.buffer.info;
    bi->num_queue_writes++;
    bi->last_write_offset = offset;
    bi->last_write_size = size;
    bool ok = true;
    if (!(bi->usage & WGPUBufferUsage_CopyDst)) {
        err("%s: buffer lacks the CopyDst usage", fn);
        ok = false;
    }
    if (((offset % 4) != 0) || ((size % 4) != 0)) {
        err("%s: offset %llu and size %llu must be multiples of 4", fn, LL(offset), LL(size));
        ok = false;
    }
    if (!range_ok(offset, size, bi->size)) {
        err("%s: range (offset %llu, size %llu) exceeds buffer size %llu", fn, LL(offset), LL(size), LL(bi->size));
        ok = false;
    }
    if (bi->mapped) {
        err("%s: buffer is mapped", fn);
        ok = false;
    }
    if ((size > 0) && !data) {
        err("%s: data pointer is null", fn);
        ok = false;
    }
    if (ok && (size > 0) && b->u.buffer.data) {
        memcpy(b->u.buffer.data + offset, data, size);
    }
}

static void queue_write_texture(const WGPUTexelCopyTextureInfo* dst, const void* data, size_t data_size, const WGPUTexelCopyBufferLayout* layout, const WGPUExtent3D* size) {
    const char* fn = "QueueWriteTexture";
    g_queue_state.num_write_textures++;
    if (!dst || !layout || !size) {
        err("%s: null argument", fn);
        return;
    }
    obj_t* t = lookup(dst->texture, WGPU_MOCK_OBJ_TEXTURE, fn);
    if (!t) {
        return;
    }
    wgpu_mock_texture_info_t* ti = &t->u.texture.info;
    ti->num_queue_writes++;
    ti->last_write_mip_level = dst->mipLevel;
    ti->last_write_origin = dst->origin;
    ti->last_write_size = *size;
    ti->last_write_layout = *layout;
    ti->last_write_data_size = data_size;
    const fmt_info_t* fi = fmt_info(ti->format);
    if (!fi) {
        return;
    }
    if (!(ti->usage & WGPUTextureUsage_CopyDst)) {
        err("%s: texture lacks the CopyDst usage", fn);
    }
    if (ti->sample_count != 1) {
        err("%s: multisampled textures cannot be written", fn);
    }
    if (fi->flags & F_DEPTH) {
        err("%s: the depth aspect of format %d cannot be written by a queue write", fn, (int)ti->format);
    }
    if (!data) {
        err("%s: data pointer is null", fn);
    }
    if (!tex_copy_bounds(ti, dst->mipLevel, &dst->origin, size, false, fn)) {
        return;
    }
    uint64_t required = 0;
    if (copy_layout_required(fi, size, layout, fn, &required)) {
        if (!range_ok(layout->offset, required, data_size)) {
            err("%s: copy needs %llu bytes at offset %llu but only %llu bytes of data are provided", fn, LL(required), LL(layout->offset), LL(data_size));
        }
    }
}

// ---- public WebGPU API
#define WM_DEFINE_CREATE(ret, fname, dtype, kind, impl) \
    ret fname(WGPUDevice device, const dtype* desc) { \
        wgpu_mock_call_t* c = begin_call(WGPU_MOCK_FUNC_##fname, device); \
        obj_t* o = 0; \
        if (lookup(device, WGPU_MOCK_OBJ_DEVICE, #fname)) { \
            if (!desc) { \
                err(#fname ": descriptor is null"); \
            } else if (!should_fail(kind)) { \
                o = impl(desc); \
            } \
        } \
        arg_p(c, o); \
        return (ret)o; \
    }

#define WM_DEFINE_RELEASE(fname, htype, kind) \
    void fname(htype h) { \
        begin_call(WGPU_MOCK_FUNC_##fname, h); \
        api_release(h, kind, #fname); \
    }

#define WM_DEFINE_ADDREF(fname, htype, kind) \
    void fname(htype h) { \
        begin_call(WGPU_MOCK_FUNC_##fname, h); \
        api_add_ref(h, kind, #fname); \
    }

WM_DEFINE_CREATE(WGPUBuffer, wgpuDeviceCreateBuffer, WGPUBufferDescriptor, WGPU_MOCK_OBJ_BUFFER, buffer_create)
WM_DEFINE_CREATE(WGPUTexture, wgpuDeviceCreateTexture, WGPUTextureDescriptor, WGPU_MOCK_OBJ_TEXTURE, texture_create)
WM_DEFINE_CREATE(WGPUSampler, wgpuDeviceCreateSampler, WGPUSamplerDescriptor, WGPU_MOCK_OBJ_SAMPLER, sampler_create)
WM_DEFINE_CREATE(WGPUShaderModule, wgpuDeviceCreateShaderModule, WGPUShaderModuleDescriptor, WGPU_MOCK_OBJ_SHADER_MODULE, shader_create)
WM_DEFINE_CREATE(WGPUBindGroupLayout, wgpuDeviceCreateBindGroupLayout, WGPUBindGroupLayoutDescriptor, WGPU_MOCK_OBJ_BIND_GROUP_LAYOUT, bgl_create)
WM_DEFINE_CREATE(WGPUBindGroup, wgpuDeviceCreateBindGroup, WGPUBindGroupDescriptor, WGPU_MOCK_OBJ_BIND_GROUP, bg_create)
WM_DEFINE_CREATE(WGPUPipelineLayout, wgpuDeviceCreatePipelineLayout, WGPUPipelineLayoutDescriptor, WGPU_MOCK_OBJ_PIPELINE_LAYOUT, pipeline_layout_create)
WM_DEFINE_CREATE(WGPURenderPipeline, wgpuDeviceCreateRenderPipeline, WGPURenderPipelineDescriptor, WGPU_MOCK_OBJ_RENDER_PIPELINE, render_pipeline_create)
WM_DEFINE_CREATE(WGPUComputePipeline, wgpuDeviceCreateComputePipeline, WGPUComputePipelineDescriptor, WGPU_MOCK_OBJ_COMPUTE_PIPELINE, compute_pipeline_create)

WM_DEFINE_ADDREF(wgpuBufferAddRef, WGPUBuffer, WGPU_MOCK_OBJ_BUFFER)
WM_DEFINE_ADDREF(wgpuTextureAddRef, WGPUTexture, WGPU_MOCK_OBJ_TEXTURE)
WM_DEFINE_ADDREF(wgpuSamplerAddRef, WGPUSampler, WGPU_MOCK_OBJ_SAMPLER)

WM_DEFINE_RELEASE(wgpuBufferRelease, WGPUBuffer, WGPU_MOCK_OBJ_BUFFER)
WM_DEFINE_RELEASE(wgpuTextureRelease, WGPUTexture, WGPU_MOCK_OBJ_TEXTURE)
WM_DEFINE_RELEASE(wgpuTextureViewRelease, WGPUTextureView, WGPU_MOCK_OBJ_TEXTURE_VIEW)
WM_DEFINE_RELEASE(wgpuSamplerRelease, WGPUSampler, WGPU_MOCK_OBJ_SAMPLER)
WM_DEFINE_RELEASE(wgpuShaderModuleRelease, WGPUShaderModule, WGPU_MOCK_OBJ_SHADER_MODULE)
WM_DEFINE_RELEASE(wgpuBindGroupLayoutRelease, WGPUBindGroupLayout, WGPU_MOCK_OBJ_BIND_GROUP_LAYOUT)
WM_DEFINE_RELEASE(wgpuBindGroupRelease, WGPUBindGroup, WGPU_MOCK_OBJ_BIND_GROUP)
WM_DEFINE_RELEASE(wgpuPipelineLayoutRelease, WGPUPipelineLayout, WGPU_MOCK_OBJ_PIPELINE_LAYOUT)
WM_DEFINE_RELEASE(wgpuRenderPipelineRelease, WGPURenderPipeline, WGPU_MOCK_OBJ_RENDER_PIPELINE)
WM_DEFINE_RELEASE(wgpuComputePipelineRelease, WGPUComputePipeline, WGPU_MOCK_OBJ_COMPUTE_PIPELINE)
WM_DEFINE_RELEASE(wgpuQueueRelease, WGPUQueue, WGPU_MOCK_OBJ_QUEUE)
WM_DEFINE_RELEASE(wgpuCommandEncoderRelease, WGPUCommandEncoder, WGPU_MOCK_OBJ_COMMAND_ENCODER)
WM_DEFINE_RELEASE(wgpuCommandBufferRelease, WGPUCommandBuffer, WGPU_MOCK_OBJ_COMMAND_BUFFER)
WM_DEFINE_RELEASE(wgpuComputePassEncoderRelease, WGPUComputePassEncoder, WGPU_MOCK_OBJ_COMPUTE_PASS_ENCODER)
WM_DEFINE_RELEASE(wgpuRenderPassEncoderRelease, WGPURenderPassEncoder, WGPU_MOCK_OBJ_RENDER_PASS_ENCODER)

WGPUBool wgpuDeviceHasFeature(WGPUDevice device, WGPUFeatureName feature) {
    wgpu_mock_call_t* c = begin_call(WGPU_MOCK_FUNC_wgpuDeviceHasFeature, device);
    bool res = false;
    if (lookup(device, WGPU_MOCK_OBJ_DEVICE, "wgpuDeviceHasFeature")) {
        res = has_feature(feature);
    }
    arg_u(c, (uint64_t)feature);
    arg_u(c, res ? 1 : 0);
    return res ? 1 : 0;
}

WGPUStatus wgpuDeviceGetLimits(WGPUDevice device, WGPULimits* limits) {
    begin_call(WGPU_MOCK_FUNC_wgpuDeviceGetLimits, device);
    if (!lookup(device, WGPU_MOCK_OBJ_DEVICE, "wgpuDeviceGetLimits")) {
        return 0;
    }
    if (!limits) {
        err("wgpuDeviceGetLimits: limits is null");
        return 0;
    }
    *limits = g_limits;
    return 1;
}

WGPUQueue wgpuDeviceGetQueue(WGPUDevice device) {
    wgpu_mock_call_t* c = begin_call(WGPU_MOCK_FUNC_wgpuDeviceGetQueue, device);
    obj_t* q = 0;
    if (lookup(device, WGPU_MOCK_OBJ_DEVICE, "wgpuDeviceGetQueue") && lookup(g_queue, WGPU_MOCK_OBJ_QUEUE, "wgpuDeviceGetQueue")) {
        q = g_queue;
        q->refcount++;
    }
    arg_p(c, q);
    return (WGPUQueue)q;
}

WGPUCommandEncoder wgpuDeviceCreateCommandEncoder(WGPUDevice device, const WGPUCommandEncoderDescriptor* desc) {
    (void)desc;
    wgpu_mock_call_t* c = begin_call(WGPU_MOCK_FUNC_wgpuDeviceCreateCommandEncoder, device);
    obj_t* o = 0;
    if (lookup(device, WGPU_MOCK_OBJ_DEVICE, "wgpuDeviceCreateCommandEncoder") && !should_fail(WGPU_MOCK_OBJ_COMMAND_ENCODER)) {
        o = obj_new(WGPU_MOCK_OBJ_COMMAND_ENCODER);
        memset(&g_cmd, 0, sizeof(g_cmd));
    }
    arg_p(c, o);
    return (WGPUCommandEncoder)o;
}

void* wgpuBufferGetMappedRange(WGPUBuffer buffer, size_t offset, size_t size) {
    const char* fn = "wgpuBufferGetMappedRange";
    wgpu_mock_call_t* c = begin_call(WGPU_MOCK_FUNC_wgpuBufferGetMappedRange, buffer);
    arg_u(c, offset);
    arg_u(c, size);
    void* res = 0;
    obj_t* b = lookup(buffer, WGPU_MOCK_OBJ_BUFFER, fn);
    if (b) {
        wgpu_mock_buffer_info_t* bi = &b->u.buffer.info;
        const uint64_t sz = (size == SIZE_MAX) ? ((offset <= bi->size) ? (bi->size - offset) : 0) : size;
        bool ok = true;
        if (!bi->mapped) {
            err("%s: buffer is not mapped", fn);
            ok = false;
        }
        if (((offset % 8) != 0) || ((sz % 4) != 0)) {
            err("%s: offset %llu must be a multiple of 8 and size %llu a multiple of 4", fn, LL(offset), LL(sz));
            ok = false;
        }
        if (!range_ok(offset, sz, bi->size)) {
            err("%s: range (offset %llu, size %llu) exceeds buffer size %llu", fn, LL(offset), LL(sz), LL(bi->size));
            ok = false;
        }
        if (ok) {
            bi->num_mapped_ranges++;
            if (b->u.buffer.data) {
                res = b->u.buffer.data + offset;
            }
        }
    }
    arg_p(c, res);
    return res;
}

void wgpuBufferUnmap(WGPUBuffer buffer) {
    begin_call(WGPU_MOCK_FUNC_wgpuBufferUnmap, buffer);
    obj_t* b = lookup(buffer, WGPU_MOCK_OBJ_BUFFER, "wgpuBufferUnmap");
    if (!b) {
        return;
    }
    if (!b->u.buffer.info.mapped) {
        err("wgpuBufferUnmap: buffer is not mapped");
        return;
    }
    b->u.buffer.info.mapped = false;
    b->u.buffer.info.num_unmaps++;
}

WGPUTextureView wgpuTextureCreateView(WGPUTexture texture, const WGPUTextureViewDescriptor* desc) {
    wgpu_mock_call_t* c = begin_call(WGPU_MOCK_FUNC_wgpuTextureCreateView, texture);
    obj_t* o = 0;
    obj_t* t = lookup(texture, WGPU_MOCK_OBJ_TEXTURE, "wgpuTextureCreateView");
    if (t && !should_fail(WGPU_MOCK_OBJ_TEXTURE_VIEW)) {
        o = view_create(t, desc);
    }
    arg_p(c, o);
    return (WGPUTextureView)o;
}

void wgpuQueueSubmit(WGPUQueue queue, size_t command_count, const WGPUCommandBuffer* commands) {
    wgpu_mock_call_t* c = begin_call(WGPU_MOCK_FUNC_wgpuQueueSubmit, queue);
    arg_u(c, command_count);
    if (!lookup(queue, WGPU_MOCK_OBJ_QUEUE, "wgpuQueueSubmit")) {
        return;
    }
    if ((command_count > 0) && !commands) {
        err("wgpuQueueSubmit: command count is %u but commands is null", (unsigned)command_count);
        return;
    }
    g_queue_state.num_submits++;
    for (size_t i = 0; i < command_count; i++) {
        obj_t* cb = lookup(commands[i], WGPU_MOCK_OBJ_COMMAND_BUFFER, "wgpuQueueSubmit");
        if (!cb) {
            continue;
        }
        if (cb->u.cmdbuf.submitted) {
            err("wgpuQueueSubmit: command buffer was already submitted");
            continue;
        }
        cb->u.cmdbuf.submitted = true;
        g_queue_state.num_command_buffers++;
    }
}

void wgpuQueueWriteBuffer(WGPUQueue queue, WGPUBuffer buffer, uint64_t buffer_offset, const void* data, size_t size) {
    wgpu_mock_call_t* c = begin_call(WGPU_MOCK_FUNC_wgpuQueueWriteBuffer, queue);
    arg_p(c, buffer);
    arg_u(c, buffer_offset);
    arg_u(c, size);
    if (lookup(queue, WGPU_MOCK_OBJ_QUEUE, "wgpuQueueWriteBuffer")) {
        queue_write_buffer(buffer, buffer_offset, data, size);
    }
}

void wgpuQueueWriteTexture(WGPUQueue queue, const WGPUTexelCopyTextureInfo* dst, const void* data, size_t data_size, const WGPUTexelCopyBufferLayout* layout, const WGPUExtent3D* write_size) {
    wgpu_mock_call_t* c = begin_call(WGPU_MOCK_FUNC_wgpuQueueWriteTexture, queue);
    arg_p(c, dst ? dst->texture : 0);
    arg_u(c, dst ? dst->mipLevel : 0);
    arg_u(c, data_size);
    if (lookup(queue, WGPU_MOCK_OBJ_QUEUE, "wgpuQueueWriteTexture")) {
        queue_write_texture(dst, data, data_size, layout, write_size);
    }
}

WGPUComputePassEncoder wgpuCommandEncoderBeginComputePass(WGPUCommandEncoder enc, const WGPUComputePassDescriptor* desc) {
    wgpu_mock_call_t* c = begin_call(WGPU_MOCK_FUNC_wgpuCommandEncoderBeginComputePass, enc);
    obj_t* o = 0;
    obj_t* e = lookup(enc, WGPU_MOCK_OBJ_COMMAND_ENCODER, "wgpuCommandEncoderBeginComputePass");
    if (e) {
        o = begin_compute_pass(e, desc);
    }
    arg_p(c, o);
    return (WGPUComputePassEncoder)o;
}

WGPURenderPassEncoder wgpuCommandEncoderBeginRenderPass(WGPUCommandEncoder enc, const WGPURenderPassDescriptor* desc) {
    wgpu_mock_call_t* c = begin_call(WGPU_MOCK_FUNC_wgpuCommandEncoderBeginRenderPass, enc);
    obj_t* o = 0;
    obj_t* e = lookup(enc, WGPU_MOCK_OBJ_COMMAND_ENCODER, "wgpuCommandEncoderBeginRenderPass");
    if (e) {
        o = begin_render_pass(e, desc);
    }
    arg_p(c, o);
    return (WGPURenderPassEncoder)o;
}

void wgpuCommandEncoderCopyBufferToBuffer(WGPUCommandEncoder enc, WGPUBuffer src, uint64_t src_offset, WGPUBuffer dst, uint64_t dst_offset, uint64_t size) {
    wgpu_mock_call_t* c = begin_call(WGPU_MOCK_FUNC_wgpuCommandEncoderCopyBufferToBuffer, enc);
    arg_p(c, src);
    arg_u(c, src_offset);
    arg_p(c, dst);
    arg_u(c, dst_offset);
    arg_u(c, size);
    obj_t* e = lookup(enc, WGPU_MOCK_OBJ_COMMAND_ENCODER, "wgpuCommandEncoderCopyBufferToBuffer");
    if (e) {
        copy_buffer_to_buffer(e, src, src_offset, dst, dst_offset, size);
    }
}

void wgpuCommandEncoderCopyBufferToTexture(WGPUCommandEncoder enc, const WGPUTexelCopyBufferInfo* src, const WGPUTexelCopyTextureInfo* dst, const WGPUExtent3D* size) {
    wgpu_mock_call_t* c = begin_call(WGPU_MOCK_FUNC_wgpuCommandEncoderCopyBufferToTexture, enc);
    arg_p(c, src ? src->buffer : 0);
    arg_p(c, dst ? dst->texture : 0);
    arg_u(c, src ? src->layout.bytesPerRow : 0);
    arg_u(c, dst ? dst->mipLevel : 0);
    obj_t* e = lookup(enc, WGPU_MOCK_OBJ_COMMAND_ENCODER, "wgpuCommandEncoderCopyBufferToTexture");
    if (e) {
        copy_buffer_to_texture(e, src, dst, size);
    }
}

void wgpuCommandEncoderCopyTextureToTexture(WGPUCommandEncoder enc, const WGPUTexelCopyTextureInfo* src, const WGPUTexelCopyTextureInfo* dst, const WGPUExtent3D* size) {
    wgpu_mock_call_t* c = begin_call(WGPU_MOCK_FUNC_wgpuCommandEncoderCopyTextureToTexture, enc);
    arg_p(c, src ? src->texture : 0);
    arg_p(c, dst ? dst->texture : 0);
    obj_t* e = lookup(enc, WGPU_MOCK_OBJ_COMMAND_ENCODER, "wgpuCommandEncoderCopyTextureToTexture");
    if (e) {
        copy_texture_to_texture(e, src, dst, size);
    }
}

WGPUCommandBuffer wgpuCommandEncoderFinish(WGPUCommandEncoder enc, const WGPUCommandBufferDescriptor* desc) {
    (void)desc;
    wgpu_mock_call_t* c = begin_call(WGPU_MOCK_FUNC_wgpuCommandEncoderFinish, enc);
    obj_t* o = 0;
    obj_t* e = lookup(enc, WGPU_MOCK_OBJ_COMMAND_ENCODER, "wgpuCommandEncoderFinish");
    if (e) {
        if (e->u.enc.finished) {
            err("wgpuCommandEncoderFinish: command encoder is already finished");
        } else if (e->u.enc.pass_open) {
            err("wgpuCommandEncoderFinish: a pass was not ended");
        }
        if (!should_fail(WGPU_MOCK_OBJ_COMMAND_BUFFER)) {
            o = obj_new(WGPU_MOCK_OBJ_COMMAND_BUFFER);
        }
        e->u.enc.finished = true;
        g_cmd.finished = true;
    }
    arg_p(c, o);
    return (WGPUCommandBuffer)o;
}

// compute pass
void wgpuComputePassEncoderSetPipeline(WGPUComputePassEncoder enc, WGPUComputePipeline pip) {
    wgpu_mock_call_t* c = begin_call(WGPU_MOCK_FUNC_wgpuComputePassEncoderSetPipeline, enc);
    arg_p(c, pip);
    if (cpass(enc, "wgpuComputePassEncoderSetPipeline") && lookup(pip, WGPU_MOCK_OBJ_COMPUTE_PIPELINE, "wgpuComputePassEncoderSetPipeline")) {
        g_cpe.pipeline = pip;
    }
}

void wgpuComputePassEncoderSetBindGroup(WGPUComputePassEncoder enc, uint32_t group_index, WGPUBindGroup group, size_t num_dyn_offsets, const uint32_t* dyn_offsets) {
    wgpu_mock_call_t* c = begin_call(WGPU_MOCK_FUNC_wgpuComputePassEncoderSetBindGroup, enc);
    arg_u(c, group_index);
    arg_p(c, group);
    arg_u(c, num_dyn_offsets);
    if (cpass(enc, "wgpuComputePassEncoderSetBindGroup")) {
        set_bind_group(g_cpe.bind_groups, group_index, group, num_dyn_offsets, dyn_offsets, "wgpuComputePassEncoderSetBindGroup");
    }
}

void wgpuComputePassEncoderDispatchWorkgroups(WGPUComputePassEncoder enc, uint32_t x, uint32_t y, uint32_t z) {
    const char* fn = "wgpuComputePassEncoderDispatchWorkgroups";
    wgpu_mock_call_t* c = begin_call(WGPU_MOCK_FUNC_wgpuComputePassEncoderDispatchWorkgroups, enc);
    arg_u(c, x);
    arg_u(c, y);
    arg_u(c, z);
    if (!cpass(enc, fn)) {
        return;
    }
    const obj_t* p = (const obj_t*)g_cpe.pipeline;
    if (!p) {
        err("%s: no pipeline is set", fn);
    } else {
        check_bind_groups(g_cpe.bind_groups, (const obj_t*)p->u.cp.info.layout, fn);
    }
    const uint32_t max = g_limits.maxComputeWorkgroupsPerDimension;
    if ((x > max) || (y > max) || (z > max)) {
        err("%s: dispatch size (%u, %u, %u) exceeds maxComputeWorkgroupsPerDimension %u", fn, x, y, z, max);
    }
    g_cpe.num_dispatches++;
    g_cpe.last_dispatch[0] = x;
    g_cpe.last_dispatch[1] = y;
    g_cpe.last_dispatch[2] = z;
}

void wgpuComputePassEncoderEnd(WGPUComputePassEncoder enc) {
    begin_call(WGPU_MOCK_FUNC_wgpuComputePassEncoderEnd, enc);
    obj_t* o = cpass(enc, "wgpuComputePassEncoderEnd");
    if (o) {
        end_pass(o);
        g_cpe.ended = true;
    }
}

// render pass
void wgpuRenderPassEncoderSetPipeline(WGPURenderPassEncoder enc, WGPURenderPipeline pip) {
    wgpu_mock_call_t* c = begin_call(WGPU_MOCK_FUNC_wgpuRenderPassEncoderSetPipeline, enc);
    arg_p(c, pip);
    obj_t* o = rpass(enc, "wgpuRenderPassEncoderSetPipeline");
    if (o) {
        render_set_pipeline(o, pip);
    }
}

void wgpuRenderPassEncoderSetBindGroup(WGPURenderPassEncoder enc, uint32_t group_index, WGPUBindGroup group, size_t num_dyn_offsets, const uint32_t* dyn_offsets) {
    wgpu_mock_call_t* c = begin_call(WGPU_MOCK_FUNC_wgpuRenderPassEncoderSetBindGroup, enc);
    arg_u(c, group_index);
    arg_p(c, group);
    arg_u(c, num_dyn_offsets);
    if (rpass(enc, "wgpuRenderPassEncoderSetBindGroup")) {
        set_bind_group(g_rpe.bind_groups, group_index, group, num_dyn_offsets, dyn_offsets, "wgpuRenderPassEncoderSetBindGroup");
    }
}

void wgpuRenderPassEncoderSetBlendConstant(WGPURenderPassEncoder enc, const WGPUColor* color) {
    wgpu_mock_call_t* c = begin_call(WGPU_MOCK_FUNC_wgpuRenderPassEncoderSetBlendConstant, enc);
    if (color) {
        arg_f(c, color->r);
        arg_f(c, color->g);
        arg_f(c, color->b);
        arg_f(c, color->a);
    }
    if (rpass(enc, "wgpuRenderPassEncoderSetBlendConstant")) {
        if (!color) {
            err("wgpuRenderPassEncoderSetBlendConstant: color is null");
            return;
        }
        if (!(is_finite_f(color->r) && is_finite_f(color->g) && is_finite_f(color->b) && is_finite_f(color->a))) {
            err("wgpuRenderPassEncoderSetBlendConstant: non-finite blend constant");
        }
        g_rpe.has_blend_constant = true;
        g_rpe.blend_constant = *color;
    }
}

void wgpuRenderPassEncoderSetStencilReference(WGPURenderPassEncoder enc, uint32_t ref) {
    wgpu_mock_call_t* c = begin_call(WGPU_MOCK_FUNC_wgpuRenderPassEncoderSetStencilReference, enc);
    arg_u(c, ref);
    if (rpass(enc, "wgpuRenderPassEncoderSetStencilReference")) {
        g_rpe.has_stencil_reference = true;
        g_rpe.stencil_reference = ref;
    }
}

void wgpuRenderPassEncoderSetViewport(WGPURenderPassEncoder enc, float x, float y, float w, float h, float min_depth, float max_depth) {
    const char* fn = "wgpuRenderPassEncoderSetViewport";
    wgpu_mock_call_t* c = begin_call(WGPU_MOCK_FUNC_wgpuRenderPassEncoderSetViewport, enc);
    arg_f(c, (double)x);
    arg_f(c, (double)y);
    arg_f(c, (double)w);
    arg_f(c, (double)h);
    arg_f(c, (double)min_depth);
    arg_f(c, (double)max_depth);
    if (!rpass(enc, fn)) {
        return;
    }
    if (!(is_finite_f((double)x) && is_finite_f((double)y) && is_finite_f((double)w) && is_finite_f((double)h) &&
          is_finite_f((double)min_depth) && is_finite_f((double)max_depth)))
    {
        err("%s: non-finite viewport value", fn);
    } else {
        if ((w < 0.0f) || (h < 0.0f)) {
            err("%s: viewport size (%f, %f) must not be negative", fn, (double)w, (double)h);
        }
        if (!((min_depth >= 0.0f) && (min_depth <= max_depth) && (max_depth <= 1.0f))) {
            err("%s: depth range [%f, %f] is invalid", fn, (double)min_depth, (double)max_depth);
        }
    }
    g_rpe.has_viewport = true;
    g_rpe.viewport[0] = x;
    g_rpe.viewport[1] = y;
    g_rpe.viewport[2] = w;
    g_rpe.viewport[3] = h;
    g_rpe.viewport[4] = min_depth;
    g_rpe.viewport[5] = max_depth;
}

void wgpuRenderPassEncoderSetScissorRect(WGPURenderPassEncoder enc, uint32_t x, uint32_t y, uint32_t w, uint32_t h) {
    const char* fn = "wgpuRenderPassEncoderSetScissorRect";
    wgpu_mock_call_t* c = begin_call(WGPU_MOCK_FUNC_wgpuRenderPassEncoderSetScissorRect, enc);
    arg_u(c, x);
    arg_u(c, y);
    arg_u(c, w);
    arg_u(c, h);
    if (!rpass(enc, fn)) {
        return;
    }
    if (!range_ok(x, w, g_rp.width) || !range_ok(y, h, g_rp.height)) {
        err("%s: scissor rect (%u, %u, %u, %u) exceeds the attachment size %ux%u", fn, x, y, w, h, g_rp.width, g_rp.height);
    }
    g_rpe.has_scissor_rect = true;
    g_rpe.scissor_rect[0] = x;
    g_rpe.scissor_rect[1] = y;
    g_rpe.scissor_rect[2] = w;
    g_rpe.scissor_rect[3] = h;
}

void wgpuRenderPassEncoderSetIndexBuffer(WGPURenderPassEncoder enc, WGPUBuffer buf, WGPUIndexFormat fmt, uint64_t offset, uint64_t size) {
    wgpu_mock_call_t* c = begin_call(WGPU_MOCK_FUNC_wgpuRenderPassEncoderSetIndexBuffer, enc);
    arg_p(c, buf);
    arg_u(c, (uint64_t)fmt);
    arg_u(c, offset);
    arg_u(c, size);
    if (rpass(enc, "wgpuRenderPassEncoderSetIndexBuffer")) {
        render_set_index_buffer(buf, fmt, offset, size);
    }
}

void wgpuRenderPassEncoderSetVertexBuffer(WGPURenderPassEncoder enc, uint32_t slot, WGPUBuffer buf, uint64_t offset, uint64_t size) {
    wgpu_mock_call_t* c = begin_call(WGPU_MOCK_FUNC_wgpuRenderPassEncoderSetVertexBuffer, enc);
    arg_u(c, slot);
    arg_p(c, buf);
    arg_u(c, offset);
    arg_u(c, size);
    if (rpass(enc, "wgpuRenderPassEncoderSetVertexBuffer")) {
        render_set_vertex_buffer(slot, buf, offset, size);
    }
}

void wgpuRenderPassEncoderDraw(WGPURenderPassEncoder enc, uint32_t vertex_count, uint32_t instance_count, uint32_t first_vertex, uint32_t first_instance) {
    const char* fn = "wgpuRenderPassEncoderDraw";
    wgpu_mock_call_t* c = begin_call(WGPU_MOCK_FUNC_wgpuRenderPassEncoderDraw, enc);
    arg_u(c, vertex_count);
    arg_u(c, instance_count);
    arg_u(c, first_vertex);
    arg_u(c, first_instance);
    if (!rpass(enc, fn)) {
        return;
    }
    render_check_draw(fn, false, vertex_count, instance_count, first_vertex, first_instance);
    g_rpe.num_draws++;
    g_rpe.last_vertex_or_index_count = vertex_count;
    g_rpe.last_instance_count = instance_count;
    g_rpe.last_first_vertex_or_index = first_vertex;
    g_rpe.last_base_vertex = 0;
    g_rpe.last_first_instance = first_instance;
}

void wgpuRenderPassEncoderDrawIndexed(WGPURenderPassEncoder enc, uint32_t index_count, uint32_t instance_count, uint32_t first_index, int32_t base_vertex, uint32_t first_instance) {
    const char* fn = "wgpuRenderPassEncoderDrawIndexed";
    wgpu_mock_call_t* c = begin_call(WGPU_MOCK_FUNC_wgpuRenderPassEncoderDrawIndexed, enc);
    arg_u(c, index_count);
    arg_u(c, instance_count);
    arg_u(c, first_index);
    arg_i(c, base_vertex);
    arg_u(c, first_instance);
    if (!rpass(enc, fn)) {
        return;
    }
    render_check_draw(fn, true, index_count, instance_count, first_index, first_instance);
    g_rpe.num_indexed_draws++;
    g_rpe.last_vertex_or_index_count = index_count;
    g_rpe.last_instance_count = instance_count;
    g_rpe.last_first_vertex_or_index = first_index;
    g_rpe.last_base_vertex = base_vertex;
    g_rpe.last_first_instance = first_instance;
}

void wgpuRenderPassEncoderEnd(WGPURenderPassEncoder enc) {
    begin_call(WGPU_MOCK_FUNC_wgpuRenderPassEncoderEnd, enc);
    obj_t* o = rpass(enc, "wgpuRenderPassEncoderEnd");
    if (o) {
        end_pass(o);
        g_rpe.ended = true;
    }
}

// ---- public mock API
void wgpu_mock_shutdown(void) {
    for (int i = 0; i < g_reg_num; i++) {
        obj_t* o = g_reg[i];
        if (o->alive) {
            if (o->kind == WGPU_MOCK_OBJ_BUFFER) {
                free(o->u.buffer.data);
            } else if (o->kind == WGPU_MOCK_OBJ_SHADER_MODULE) {
                free(o->u.shader.code);
            }
        }
        free(o);
    }
    free(g_reg);
    g_reg = 0;
    g_reg_num = 0;
    g_reg_cap = 0;
    memset(g_live, 0, sizeof(g_live));
    memset(g_created, 0, sizeof(g_created));
    memset(g_fail, 0, sizeof(g_fail));
    g_num_features = 0;
    g_device = 0;
    g_queue = 0;
    g_num_calls = 0;
    g_call_overflow = false;
    g_num_errors = 0;
    g_total_errors = 0;
    memset(&g_last_render_pass, 0, sizeof(g_last_render_pass));
    memset(&g_rpe, 0, sizeof(g_rpe));
    memset(&g_cpe, 0, sizeof(g_cpe));
    memset(&g_cmd, 0, sizeof(g_cmd));
    memset(&g_queue_state, 0, sizeof(g_queue_state));
    memset(&g_rp, 0, sizeof(g_rp));
    g_cur_rpe = 0;
    g_cur_cpe = 0;
}

void wgpu_mock_setup(void) {
    wgpu_mock_shutdown();
    g_features[g_num_features++] = WGPUFeatureName_Depth32FloatStencil8;
    default_limits(&g_limits);
    g_device = obj_new(WGPU_MOCK_OBJ_DEVICE);
    g_queue = obj_new(WGPU_MOCK_OBJ_QUEUE);
}

WGPUDevice wgpu_mock_device(void) {
    return (WGPUDevice)g_device;
}

WGPUQueue wgpu_mock_queue(void) {
    return (WGPUQueue)g_queue;
}

WGPUTextureView wgpu_mock_create_view(int width, int height, WGPUTextureFormat fmt, int sample_count) {
    WGPUTextureDescriptor td;
    memset(&td, 0, sizeof(td));
    td.usage = WGPUTextureUsage_RenderAttachment | WGPUTextureUsage_CopySrc;
    td.dimension = WGPUTextureDimension_2D;
    td.size.width = (uint32_t)width;
    td.size.height = (uint32_t)height;
    td.size.depthOrArrayLayers = 1;
    td.format = fmt;
    td.mipLevelCount = 1;
    td.sampleCount = (uint32_t)sample_count;
    obj_t* tex = texture_create(&td);
    obj_t* view = view_create(tex, 0);
    obj_release(tex);
    return (WGPUTextureView)view;
}

int wgpu_mock_ref_count(const void* obj) {
    const obj_t* o = find_obj(obj);
    return (o && o->alive) ? o->refcount : 0;
}

int wgpu_mock_live_objects(wgpu_mock_obj_t kind) {
    return ((kind >= 0) && (kind < WGPU_MOCK_OBJ_NUM)) ? g_live[kind] : 0;
}

int wgpu_mock_live_objects_total(void) {
    int n = 0;
    for (int i = 0; i < WGPU_MOCK_OBJ_NUM; i++) {
        if ((i != WGPU_MOCK_OBJ_DEVICE) && (i != WGPU_MOCK_OBJ_QUEUE)) {
            n += g_live[i];
        }
    }
    return n;
}

int wgpu_mock_num_created(wgpu_mock_obj_t kind) {
    return ((kind >= 0) && (kind < WGPU_MOCK_OBJ_NUM)) ? g_created[kind] : 0;
}

bool wgpu_mock_is_alive(wgpu_mock_obj_t kind, const void* obj) {
    const obj_t* o = find_obj(obj);
    return o && o->alive && (o->kind == kind);
}

// registered object of a kind, alive or not
static const obj_t* find_kind(const void* h, wgpu_mock_obj_t kind) {
    const obj_t* o = find_obj(h);
    return (o && (o->kind == kind)) ? o : 0;
}

#define WM_DEFINE_INFO_GETTER(fname, kind, member, itype) \
    bool fname(const void* obj, itype* out) { \
        const obj_t* o = find_kind(obj, kind); \
        if (!o || !out) { \
            return false; \
        } \
        *out = o->u.member.info; \
        return true; \
    }

WM_DEFINE_INFO_GETTER(wgpu_mock_buffer_info, WGPU_MOCK_OBJ_BUFFER, buffer, wgpu_mock_buffer_info_t)
WM_DEFINE_INFO_GETTER(wgpu_mock_texture_info, WGPU_MOCK_OBJ_TEXTURE, texture, wgpu_mock_texture_info_t)
WM_DEFINE_INFO_GETTER(wgpu_mock_texture_view_info, WGPU_MOCK_OBJ_TEXTURE_VIEW, view, wgpu_mock_texture_view_info_t)
WM_DEFINE_INFO_GETTER(wgpu_mock_sampler_info, WGPU_MOCK_OBJ_SAMPLER, sampler, wgpu_mock_sampler_info_t)
WM_DEFINE_INFO_GETTER(wgpu_mock_shader_module_info, WGPU_MOCK_OBJ_SHADER_MODULE, shader, wgpu_mock_shader_module_info_t)
WM_DEFINE_INFO_GETTER(wgpu_mock_bind_group_layout_info, WGPU_MOCK_OBJ_BIND_GROUP_LAYOUT, bgl, wgpu_mock_bind_group_layout_info_t)
WM_DEFINE_INFO_GETTER(wgpu_mock_bind_group_info, WGPU_MOCK_OBJ_BIND_GROUP, bg, wgpu_mock_bind_group_info_t)
WM_DEFINE_INFO_GETTER(wgpu_mock_pipeline_layout_info, WGPU_MOCK_OBJ_PIPELINE_LAYOUT, pl, wgpu_mock_pipeline_layout_info_t)
WM_DEFINE_INFO_GETTER(wgpu_mock_render_pipeline_info, WGPU_MOCK_OBJ_RENDER_PIPELINE, rp, wgpu_mock_render_pipeline_info_t)
WM_DEFINE_INFO_GETTER(wgpu_mock_compute_pipeline_info, WGPU_MOCK_OBJ_COMPUTE_PIPELINE, cp, wgpu_mock_compute_pipeline_info_t)

const void* wgpu_mock_buffer_data(const void* obj) {
    const obj_t* o = find_kind(obj, WGPU_MOCK_OBJ_BUFFER);
    return (o && o->alive) ? o->u.buffer.data : 0;
}

const wgpu_mock_render_pass_info_t* wgpu_mock_last_render_pass(void) {
    return &g_last_render_pass;
}

const wgpu_mock_render_pass_encoder_state_t* wgpu_mock_render_pass_encoder_state(void) {
    return &g_rpe;
}

const wgpu_mock_compute_pass_encoder_state_t* wgpu_mock_compute_pass_encoder_state(void) {
    return &g_cpe;
}

const wgpu_mock_command_encoder_state_t* wgpu_mock_command_encoder_state(void) {
    return &g_cmd;
}

const wgpu_mock_queue_state_t* wgpu_mock_queue_state(void) {
    return &g_queue_state;
}

int wgpu_mock_num_calls(void) {
    return g_num_calls;
}

const wgpu_mock_call_t* wgpu_mock_call(int index) {
    return ((index >= 0) && (index < g_num_calls)) ? &g_calls[index] : 0;
}

int wgpu_mock_count_calls(wgpu_mock_func_t func) {
    int n = 0;
    for (int i = 0; i < g_num_calls; i++) {
        if (g_calls[i].func == func) {
            n++;
        }
    }
    return n;
}

int wgpu_mock_find_call(wgpu_mock_func_t func, int start_index) {
    for (int i = (start_index < 0) ? 0 : start_index; i < g_num_calls; i++) {
        if (g_calls[i].func == func) {
            return i;
        }
    }
    return -1;
}

const wgpu_mock_call_t* wgpu_mock_last_call(wgpu_mock_func_t func) {
    for (int i = g_num_calls - 1; i >= 0; i--) {
        if (g_calls[i].func == func) {
            return &g_calls[i];
        }
    }
    return 0;
}

void wgpu_mock_clear_calls(void) {
    g_num_calls = 0;
    g_call_overflow = false;
}

bool wgpu_mock_call_log_overflow(void) {
    return g_call_overflow;
}

const char* wgpu_mock_func_name(wgpu_mock_func_t func) {
    switch (func) {
        #define _WGPUM_XMACRO(name) case WGPU_MOCK_FUNC_##name: return #name;
        _WGPUM_FUNCS
        #undef _WGPUM_XMACRO
        default: return "INVALID";
    }
}

int wgpu_mock_num_errors(void) {
    return g_num_errors;
}

const char* wgpu_mock_error(int index) {
    return ((index >= 0) && (index < g_num_errors)) ? g_errors[index] : "";
}

int wgpu_mock_find_error(const char* substr) {
    for (int i = 0; i < g_num_errors; i++) {
        if (strstr(g_errors[i], substr)) {
            return i;
        }
    }
    return -1;
}

void wgpu_mock_clear_errors(void) {
    g_num_errors = 0;
    g_total_errors = 0;
}

int wgpu_mock_total_errors(void) {
    return g_total_errors;
}

void wgpu_mock_fail_next(wgpu_mock_obj_t kind, int n) {
    wgpu_mock_fail_next_after(kind, 0, n);
}

void wgpu_mock_fail_next_after(wgpu_mock_obj_t kind, int skip, int n) {
    if ((kind >= 0) && (kind < WGPU_MOCK_OBJ_NUM)) {
        g_fail[kind].skip = skip;
        g_fail[kind].count = n;
    }
}

void wgpu_mock_set_feature(WGPUFeatureName feature, bool supported) {
    for (int i = 0; i < g_num_features; i++) {
        if (g_features[i] == feature) {
            if (!supported) {
                g_features[i] = g_features[--g_num_features];
            }
            return;
        }
    }
    if (supported && (g_num_features < WM_MAX_FEATURES)) {
        g_features[g_num_features++] = feature;
    }
}

void wgpu_mock_set_limits(const WGPULimits* limits) {
    g_limits = *limits;
}

void wgpu_mock_get_default_limits(WGPULimits* out) {
    default_limits(out);
}
