/*
    LLM maintained.

    metal_mock.m -- implementation of the mocked Metal runtime.

    Compiled without ARC so the mock can manage its own object lifetimes and
    keep working no matter whether the test target uses ARC or not.

    Object lifetime notes:

    - the device is a singleton owned by the mock and lives for the whole
      process. Its refcount is real, so an unbalanced release in sokol_gfx.h
      aborts
    - command buffers, command encoders and render pass descriptors are
      autoreleased into a pool owned by the mock, matching the real Metal
      behaviour. Call metal_mock_drain_pool() to free them
    - completion handlers run synchronously in -commit, so the semaphore which
      sokol_gfx.h waits on in sg_begin_pass() is signalled deterministically
*/
#import "metal_mock.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#if __has_feature(objc_arc)
#error "metal_mock.m must be compiled without ARC (-fno-objc-arc)"
#endif

// The mock checks its own invariants in release builds too, so don't use
// the assert() from <assert.h> which NDEBUG compiles out.
#define _MTLM_ASSERT(c) do { if (!(c)) { _mtlm_panic(#c, __FILE__, __LINE__); } } while (0)

static void _mtlm_panic(const char* msg, const char* file, int line) {
    fprintf(stderr, "metal_mock: %s:%d: check failed: %s\n", file, line, msg);
    abort();
}

#define _MTLM_MAX_OBJECTS (1024)
#define _MTLM_MAX_FAMILIES (16)
#define _MTLM_MAX_PASS_COLOR_ATTS (METAL_MOCK_MAX_COLOR_ATTACHMENTS)
#define _MTLM_MAX_PIP_COLOR_ATTS (METAL_MOCK_MAX_COLOR_ATTACHMENTS)
#define _MTLM_MAX_VERTEX_ATTRS (METAL_MOCK_MAX_VERTEX_ATTRIBUTES)
#define _MTLM_MAX_VERTEX_LAYOUTS (METAL_MOCK_MAX_VERTEX_LAYOUTS)
#define _MTLM_MAX_PIPELINE_BUFFERS (METAL_MOCK_MAX_PIPELINE_BUFFERS)

typedef struct {
    const void* ptr;
    metal_mock_obj_t kind;
} _mtlm_slot_t;

typedef struct {
    MTLGPUFamily family;
    bool supported;
} _mtlm_family_t;

static struct {
    bool valid;
    NSAutoreleasePool* pool;
    _mtlm_slot_t objects[_MTLM_MAX_OBJECTS];
    int num_created[METAL_MOCK_OBJ_NUM];
    int num_live[METAL_MOCK_OBJ_NUM];
    int fail_next[METAL_MOCK_OBJ_NUM];
    int fail_skip[METAL_MOCK_OBJ_NUM];
    char err_msg[METAL_MOCK_MAX_STRING];
    _mtlm_family_t families[_MTLM_MAX_FAMILIES];
    int num_families;
    int num_pending_handlers;
    int num_calls;
    bool call_log_overflow;
    metal_mock_call_t calls[METAL_MOCK_MAX_CALLS];
    metal_mock_call_t dummy_call;
    metal_mock_render_pass_info_t last_render_pass;
    metal_mock_render_encoder_state_t render_encoder_state;
    metal_mock_compute_encoder_state_t compute_encoder_state;
} _mtlm;

//== helpers ===================================================================

static void _mtlm_register(const void* ptr, metal_mock_obj_t kind) {
    for (int i = 0; i < _MTLM_MAX_OBJECTS; i++) {
        if (0 == _mtlm.objects[i].ptr) {
            _mtlm.objects[i].ptr = ptr;
            _mtlm.objects[i].kind = kind;
            return;
        }
    }
    _MTLM_ASSERT(false && "object registry full");
}

static void _mtlm_unregister(const void* ptr) {
    for (int i = 0; i < _MTLM_MAX_OBJECTS; i++) {
        if (ptr == _mtlm.objects[i].ptr) {
            _mtlm.objects[i].ptr = 0;
            return;
        }
    }
}

static bool _mtlm_is_obj(const void* ptr, metal_mock_obj_t kind) {
    if (0 == ptr) {
        return false;
    }
    for (int i = 0; i < _MTLM_MAX_OBJECTS; i++) {
        if (ptr == _mtlm.objects[i].ptr) {
            return _mtlm.objects[i].kind == kind;
        }
    }
    return false;
}

static metal_mock_call_t* _mtlm_log(metal_mock_func_t func, const void* obj) {
    metal_mock_call_t* call;
    if (_mtlm.num_calls < METAL_MOCK_MAX_CALLS) {
        call = &_mtlm.calls[_mtlm.num_calls++];
    } else {
        _mtlm.call_log_overflow = true;
        call = &_mtlm.dummy_call;
    }
    memset(call, 0, sizeof(metal_mock_call_t));
    call->func = func;
    call->obj = obj;
    return call;
}

static void _mtlm_copy_str(char* dst, NSString* src) {
    if (src) {
        const char* cstr = [src UTF8String];
        strncpy(dst, cstr, METAL_MOCK_MAX_STRING - 1);
        dst[METAL_MOCK_MAX_STRING - 1] = 0;
    } else {
        dst[0] = 0;
    }
}

// Log a string argument. The NSString bytes belong to the autorelease pool,
// so copy them into the call record and point the argument at the copy.
static void _mtlm_log_str(metal_mock_call_t* call, int arg_index, NSString* src) {
    _MTLM_ASSERT((arg_index >= 0) && (arg_index < METAL_MOCK_MAX_CALL_ARGS));
    _mtlm_copy_str(call->str, src);
    call->args[arg_index].p = call->str;
}

static bool _mtlm_fail(metal_mock_obj_t kind) {
    if (_mtlm.fail_next[kind] > 0) {
        if (_mtlm.fail_skip[kind] > 0) {
            _mtlm.fail_skip[kind]--;
            return false;
        }
        _mtlm.fail_next[kind]--;
        return true;
    }
    return false;
}

static NSError* _mtlm_error(void) {
    NSString* msg = [NSString stringWithUTF8String:_mtlm.err_msg];
    return [NSError errorWithDomain:@"MetalMockErrorDomain"
                               code:1
                           userInfo:@{ NSLocalizedDescriptionKey: msg }];
}

//== tracked base class ========================================================

@interface _mtlm_obj : NSObject {
@public
    metal_mock_obj_t mock_kind;
}
@property (nullable, copy, atomic) NSString* label;
- (instancetype)initWithKind:(metal_mock_obj_t)kind;
@end

@implementation _mtlm_obj
- (instancetype)initWithKind:(metal_mock_obj_t)kind {
    self = [super init];
    if (self) {
        mock_kind = kind;
        _mtlm.num_created[kind]++;
        _mtlm.num_live[kind]++;
        _mtlm_register(self, kind);
    }
    return self;
}
- (void)dealloc {
    _mtlm.num_live[mock_kind]--;
    _mtlm_unregister(self);
    [_label release];
    [super dealloc];
}
@end

//== descriptor classes ========================================================

@implementation MTLCompileOptions
@end

@implementation MTLTextureDescriptor
- (instancetype)init {
    self = [super init];
    if (self) {
        _textureType = MTLTextureType2D;
        _pixelFormat = MTLPixelFormatRGBA8Unorm;
        _width = 1;
        _height = 1;
        _depth = 1;
        _mipmapLevelCount = 1;
        _sampleCount = 1;
        _arrayLength = 1;
        _usage = MTLTextureUsageShaderRead;
    }
    return self;
}
- (void)dealloc {
    [_label release];
    [super dealloc];
}
@end

@implementation MTLSamplerDescriptor
- (instancetype)init {
    self = [super init];
    if (self) {
        _lodMaxClamp = FLT_MAX;
        _maxAnisotropy = 1;
        _normalizedCoordinates = YES;
        _compareFunction = MTLCompareFunctionNever;
    }
    return self;
}
- (void)dealloc {
    [_label release];
    [super dealloc];
}
@end

@implementation MTLVertexBufferLayoutDescriptor
- (instancetype)init {
    self = [super init];
    if (self) {
        _stepFunction = MTLVertexStepFunctionPerVertex;
        _stepRate = 1;
    }
    return self;
}
@end

@implementation MTLVertexAttributeDescriptor
@end

// Fixed-size descriptor arrays. Every slot holds a live object, so the
// `array[i].field = value` pattern used by sokol_gfx.h always works.
@implementation MTLVertexBufferLayoutDescriptorArray {
    NSMutableArray* _items;
}
- (instancetype)init {
    self = [super init];
    if (self) {
        _items = [[NSMutableArray alloc] initWithCapacity:_MTLM_MAX_VERTEX_LAYOUTS];
        for (int i = 0; i < _MTLM_MAX_VERTEX_LAYOUTS; i++) {
            MTLVertexBufferLayoutDescriptor* item = [[MTLVertexBufferLayoutDescriptor alloc] init];
            [_items addObject:item];
            [item release];
        }
    }
    return self;
}
- (void)dealloc {
    [_items release];
    [super dealloc];
}
- (MTLVertexBufferLayoutDescriptor*)objectAtIndexedSubscript:(NSUInteger)index {
    _MTLM_ASSERT(index < _MTLM_MAX_VERTEX_LAYOUTS);
    return [_items objectAtIndex:index];
}
@end

@implementation MTLVertexAttributeDescriptorArray {
    NSMutableArray* _items;
}
- (instancetype)init {
    self = [super init];
    if (self) {
        _items = [[NSMutableArray alloc] initWithCapacity:_MTLM_MAX_VERTEX_ATTRS];
        for (int i = 0; i < _MTLM_MAX_VERTEX_ATTRS; i++) {
            MTLVertexAttributeDescriptor* item = [[MTLVertexAttributeDescriptor alloc] init];
            [_items addObject:item];
            [item release];
        }
    }
    return self;
}
- (void)dealloc {
    [_items release];
    [super dealloc];
}
- (MTLVertexAttributeDescriptor*)objectAtIndexedSubscript:(NSUInteger)index {
    _MTLM_ASSERT(index < _MTLM_MAX_VERTEX_ATTRS);
    return [_items objectAtIndex:index];
}
@end

@implementation MTLVertexDescriptor
+ (MTLVertexDescriptor*)vertexDescriptor {
    return [[[MTLVertexDescriptor alloc] init] autorelease];
}
- (instancetype)init {
    self = [super init];
    if (self) {
        _layouts = [[MTLVertexBufferLayoutDescriptorArray alloc] init];
        _attributes = [[MTLVertexAttributeDescriptorArray alloc] init];
    }
    return self;
}
- (void)dealloc {
    [_layouts release];
    [_attributes release];
    [super dealloc];
}
@end

@implementation MTLPipelineBufferDescriptor
@end

@implementation MTLPipelineBufferDescriptorArray {
    NSMutableArray* _items;
}
- (instancetype)init {
    self = [super init];
    if (self) {
        _items = [[NSMutableArray alloc] initWithCapacity:_MTLM_MAX_PIPELINE_BUFFERS];
        for (int i = 0; i < _MTLM_MAX_PIPELINE_BUFFERS; i++) {
            MTLPipelineBufferDescriptor* item = [[MTLPipelineBufferDescriptor alloc] init];
            [_items addObject:item];
            [item release];
        }
    }
    return self;
}
- (void)dealloc {
    [_items release];
    [super dealloc];
}
- (MTLPipelineBufferDescriptor*)objectAtIndexedSubscript:(NSUInteger)index {
    _MTLM_ASSERT(index < _MTLM_MAX_PIPELINE_BUFFERS);
    return [_items objectAtIndex:index];
}
@end

@implementation MTLRenderPipelineColorAttachmentDescriptor
- (instancetype)init {
    self = [super init];
    if (self) {
        _writeMask = MTLColorWriteMaskAll;
        _sourceRGBBlendFactor = MTLBlendFactorOne;
        _destinationRGBBlendFactor = MTLBlendFactorZero;
        _sourceAlphaBlendFactor = MTLBlendFactorOne;
        _destinationAlphaBlendFactor = MTLBlendFactorZero;
    }
    return self;
}
@end

@implementation MTLRenderPipelineColorAttachmentDescriptorArray {
    NSMutableArray* _items;
}
- (instancetype)init {
    self = [super init];
    if (self) {
        _items = [[NSMutableArray alloc] initWithCapacity:_MTLM_MAX_PIP_COLOR_ATTS];
        for (int i = 0; i < _MTLM_MAX_PIP_COLOR_ATTS; i++) {
            MTLRenderPipelineColorAttachmentDescriptor* item = [[MTLRenderPipelineColorAttachmentDescriptor alloc] init];
            [_items addObject:item];
            [item release];
        }
    }
    return self;
}
- (void)dealloc {
    [_items release];
    [super dealloc];
}
- (MTLRenderPipelineColorAttachmentDescriptor*)objectAtIndexedSubscript:(NSUInteger)attachmentIndex {
    _MTLM_ASSERT(attachmentIndex < _MTLM_MAX_PIP_COLOR_ATTS);
    return [_items objectAtIndex:attachmentIndex];
}
@end

@implementation MTLRenderPipelineDescriptor
- (instancetype)init {
    self = [super init];
    if (self) {
        _rasterSampleCount = 1;
        _rasterizationEnabled = YES;
        _colorAttachments = [[MTLRenderPipelineColorAttachmentDescriptorArray alloc] init];
        _vertexBuffers = [[MTLPipelineBufferDescriptorArray alloc] init];
        _fragmentBuffers = [[MTLPipelineBufferDescriptorArray alloc] init];
    }
    return self;
}
- (void)dealloc {
    [_colorAttachments release];
    [_vertexBuffers release];
    [_fragmentBuffers release];
    [_vertexDescriptor release];
    [_vertexFunction release];
    [_fragmentFunction release];
    [_label release];
    [super dealloc];
}
@end

@implementation MTLComputePipelineDescriptor
- (instancetype)init {
    self = [super init];
    if (self) {
        _buffers = [[MTLPipelineBufferDescriptorArray alloc] init];
    }
    return self;
}
- (void)dealloc {
    [_buffers release];
    [_computeFunction release];
    [_label release];
    [super dealloc];
}
@end

@implementation MTLStencilDescriptor
- (instancetype)init {
    self = [super init];
    if (self) {
        _stencilCompareFunction = MTLCompareFunctionAlways;
        _readMask = 0xFFFFFFFF;
        _writeMask = 0xFFFFFFFF;
    }
    return self;
}
@end

@implementation MTLDepthStencilDescriptor
- (instancetype)init {
    self = [super init];
    if (self) {
        _depthCompareFunction = MTLCompareFunctionAlways;
    }
    return self;
}
- (void)dealloc {
    [_frontFaceStencil release];
    [_backFaceStencil release];
    [_label release];
    [super dealloc];
}
@end

@implementation MTLRenderPassAttachmentDescriptor
- (void)dealloc {
    [_texture release];
    [_resolveTexture release];
    [super dealloc];
}
@end

@implementation MTLRenderPassColorAttachmentDescriptor
@end

@implementation MTLRenderPassDepthAttachmentDescriptor
- (instancetype)init {
    self = [super init];
    if (self) {
        _clearDepth = 1.0;
    }
    return self;
}
@end

@implementation MTLRenderPassStencilAttachmentDescriptor
@end

@implementation MTLRenderPassColorAttachmentDescriptorArray {
    NSMutableArray* _items;
}
- (instancetype)init {
    self = [super init];
    if (self) {
        _items = [[NSMutableArray alloc] initWithCapacity:_MTLM_MAX_PASS_COLOR_ATTS];
        for (int i = 0; i < _MTLM_MAX_PASS_COLOR_ATTS; i++) {
            MTLRenderPassColorAttachmentDescriptor* item = [[MTLRenderPassColorAttachmentDescriptor alloc] init];
            [_items addObject:item];
            [item release];
        }
    }
    return self;
}
- (void)dealloc {
    [_items release];
    [super dealloc];
}
- (MTLRenderPassColorAttachmentDescriptor*)objectAtIndexedSubscript:(NSUInteger)attachmentIndex {
    _MTLM_ASSERT(attachmentIndex < _MTLM_MAX_PASS_COLOR_ATTS);
    return [_items objectAtIndex:attachmentIndex];
}
@end

@implementation MTLRenderPassDescriptor
+ (MTLRenderPassDescriptor*)renderPassDescriptor {
    return [[[MTLRenderPassDescriptor alloc] init] autorelease];
}
- (instancetype)init {
    self = [super init];
    if (self) {
        _colorAttachments = [[MTLRenderPassColorAttachmentDescriptorArray alloc] init];
        _depthAttachment = [[MTLRenderPassDepthAttachmentDescriptor alloc] init];
        _stencilAttachment = [[MTLRenderPassStencilAttachmentDescriptor alloc] init];
    }
    return self;
}
- (void)dealloc {
    [_colorAttachments release];
    [_depthAttachment release];
    [_stencilAttachment release];
    [super dealloc];
}
@end

//== resource classes ==========================================================

@interface _mtlm_buffer : _mtlm_obj <MTLBuffer> {
@public
    metal_mock_buffer_info_t info;
    uint8_t* data;
}
@end

@interface _mtlm_texture : _mtlm_obj <MTLTexture> {
@public
    metal_mock_texture_info_t info;
    id<MTLTexture> view_src;    // retained source texture of a texture view
}
@end

@interface _mtlm_sampler : _mtlm_obj <MTLSamplerState> {
@public
    metal_mock_sampler_info_t info;
}
@end

@interface _mtlm_function : _mtlm_obj <MTLFunction> {
@public
    metal_mock_function_info_t info;
}
@end

@interface _mtlm_library : _mtlm_obj <MTLLibrary> {
@public
    metal_mock_library_info_t info;
}
@end

@interface _mtlm_render_pipeline : _mtlm_obj <MTLRenderPipelineState> {
@public
    metal_mock_render_pipeline_info_t info;
}
@end

@interface _mtlm_compute_pipeline : _mtlm_obj <MTLComputePipelineState> {
@public
    metal_mock_compute_pipeline_info_t info;
}
@end

@interface _mtlm_depth_stencil : _mtlm_obj <MTLDepthStencilState> {
@public
    metal_mock_depth_stencil_info_t info;
}
@end

@interface _mtlm_drawable : _mtlm_obj <CAMetalDrawable> {
@public
    id<MTLTexture> tex;
}
@end

@interface _mtlm_render_encoder : _mtlm_obj <MTLRenderCommandEncoder>
@end

@interface _mtlm_compute_encoder : _mtlm_obj <MTLComputeCommandEncoder>
@end

@interface _mtlm_command_buffer : _mtlm_obj <MTLCommandBuffer> {
@public
    NSMutableArray* handlers;
}
- (void)runCompletedHandlers;
@end

@interface _mtlm_command_queue : _mtlm_obj <MTLCommandQueue>
@end

@implementation _mtlm_buffer
- (void)dealloc {
    free(data);
    [super dealloc];
}
- (void*)contents {
    _mtlm_log(METAL_MOCK_FUNC_contents, self);
    return data;
}
- (void)didModifyRange:(NSRange)range {
    metal_mock_call_t* c = _mtlm_log(METAL_MOCK_FUNC_didModifyRange, self);
    c->num_args = 2;
    c->args[0].u = range.location;
    c->args[1].u = range.length;
    info.num_did_modify_range++;
    info.last_modified_offset = range.location;
    info.last_modified_length = range.length;
}
- (void)setLabel:(NSString*)label {
    [super setLabel:label];
    _mtlm_copy_str(info.label, label);
}
@end

@implementation _mtlm_texture
- (void)dealloc {
    [view_src release];
    [super dealloc];
}
- (MTLTextureType)textureType { return info.texture_type; }
- (MTLPixelFormat)pixelFormat { return info.pixel_format; }
- (NSUInteger)width { return info.width; }
- (NSUInteger)height { return info.height; }
- (NSUInteger)depth { return info.depth; }
- (NSUInteger)mipmapLevelCount { return info.mipmap_level_count; }
- (NSUInteger)sampleCount { return info.sample_count; }
- (NSUInteger)arrayLength { return info.array_length; }
- (MTLTextureUsage)usage { return info.usage; }
- (void)setLabel:(NSString*)label {
    [super setLabel:label];
    _mtlm_copy_str(info.label, label);
}
- (void)replaceRegion:(MTLRegion)region
          mipmapLevel:(NSUInteger)level
                slice:(NSUInteger)slice
            withBytes:(const void*)pixelBytes
          bytesPerRow:(NSUInteger)bytesPerRow
        bytesPerImage:(NSUInteger)bytesPerImage
{
    metal_mock_call_t* c = _mtlm_log(METAL_MOCK_FUNC_replaceRegion, self);
    c->num_args = 8;
    c->args[0].u = region.origin.x;
    c->args[1].u = region.origin.y;
    c->args[2].u = region.size.width;
    c->args[3].u = region.size.height;
    c->args[4].u = level;
    c->args[5].u = slice;
    c->args[6].u = bytesPerRow;
    c->args[7].u = bytesPerImage;
    (void)pixelBytes;
    info.num_replace_region++;
}
- (id<MTLTexture>)newTextureViewWithPixelFormat:(MTLPixelFormat)pixelFormat
                                    textureType:(MTLTextureType)textureType
                                         levels:(NSRange)levelRange
                                         slices:(NSRange)sliceRange
{
    metal_mock_call_t* c = _mtlm_log(METAL_MOCK_FUNC_newTextureViewWithPixelFormat, self);
    c->num_args = 6;
    c->args[0].u = pixelFormat;
    c->args[1].u = textureType;
    c->args[2].u = levelRange.location;
    c->args[3].u = levelRange.length;
    c->args[4].u = sliceRange.location;
    c->args[5].u = sliceRange.length;
    if (_mtlm_fail(METAL_MOCK_OBJ_TEXTURE_VIEW)) {
        return nil;
    }
    _mtlm_texture* view = [[_mtlm_texture alloc] initWithKind:METAL_MOCK_OBJ_TEXTURE_VIEW];
    view->info = info;
    view->info.label[0] = 0;
    view->info.pixel_format = pixelFormat;
    view->info.texture_type = textureType;
    view->info.mipmap_level_count = levelRange.length;
    view->info.array_length = sliceRange.length;
    view->info.is_view = true;
    // a texture view keeps its source alive, same as real Metal
    view->view_src = [self retain];
    view->info.view_source = self;
    view->info.num_replace_region = 0;
    return view;
}
@end

@implementation _mtlm_sampler
@end

@implementation _mtlm_function
@end

@implementation _mtlm_library
- (void)setLabel:(NSString*)label {
    [super setLabel:label];
    _mtlm_copy_str(info.label, label);
}
- (id<MTLFunction>)newFunctionWithName:(NSString*)functionName {
    metal_mock_call_t* c = _mtlm_log(METAL_MOCK_FUNC_newFunctionWithName, self);
    c->num_args = 1;
    _mtlm_log_str(c, 0, functionName);
    if (_mtlm_fail(METAL_MOCK_OBJ_FUNCTION)) {
        return nil;
    }
    _mtlm_function* func = [[_mtlm_function alloc] initWithKind:METAL_MOCK_OBJ_FUNCTION];
    func->info.library = self;
    _mtlm_copy_str(func->info.name, functionName);
    return func;
}
@end

@implementation _mtlm_render_pipeline
@end

@implementation _mtlm_compute_pipeline
@end

@implementation _mtlm_depth_stencil
@end

@implementation _mtlm_drawable
- (void)dealloc {
    [tex release];
    [super dealloc];
}
- (id<MTLTexture>)texture { return tex; }
- (void)present { }
@end

//== command encoders ==========================================================

@implementation _mtlm_render_encoder

- (void)endEncoding {
    _mtlm_log(METAL_MOCK_FUNC_endEncoding, self);
    _mtlm.render_encoder_state.ended = true;
}

- (void)pushDebugGroup:(NSString*)string {
    metal_mock_call_t* c = _mtlm_log(METAL_MOCK_FUNC_pushDebugGroup, self);
    c->num_args = 1;
    _mtlm_log_str(c, 0, string);
    _mtlm.render_encoder_state.debug_group_depth++;
}

- (void)popDebugGroup {
    _mtlm_log(METAL_MOCK_FUNC_popDebugGroup, self);
    _mtlm.render_encoder_state.debug_group_depth--;
}

- (void)setLabel:(NSString*)label {
    [super setLabel:label];
    _mtlm_copy_str(_mtlm.render_encoder_state.label, label);
}

- (void)setViewport:(MTLViewport)viewport {
    metal_mock_call_t* c = _mtlm_log(METAL_MOCK_FUNC_setViewport, self);
    c->num_args = 6;
    c->args[0].f = viewport.originX;
    c->args[1].f = viewport.originY;
    c->args[2].f = viewport.width;
    c->args[3].f = viewport.height;
    c->args[4].f = viewport.znear;
    c->args[5].f = viewport.zfar;
    _mtlm.render_encoder_state.viewport = viewport;
}

- (void)setScissorRect:(MTLScissorRect)rect {
    metal_mock_call_t* c = _mtlm_log(METAL_MOCK_FUNC_setScissorRect, self);
    c->num_args = 4;
    c->args[0].u = rect.x;
    c->args[1].u = rect.y;
    c->args[2].u = rect.width;
    c->args[3].u = rect.height;
    _mtlm.render_encoder_state.scissor_rect = rect;
}

- (void)setCullMode:(MTLCullMode)cullMode {
    metal_mock_call_t* c = _mtlm_log(METAL_MOCK_FUNC_setCullMode, self);
    c->num_args = 1;
    c->args[0].u = cullMode;
    _mtlm.render_encoder_state.cull_mode = cullMode;
}

- (void)setFrontFacingWinding:(MTLWinding)frontFacingWinding {
    metal_mock_call_t* c = _mtlm_log(METAL_MOCK_FUNC_setFrontFacingWinding, self);
    c->num_args = 1;
    c->args[0].u = frontFacingWinding;
    _mtlm.render_encoder_state.winding = frontFacingWinding;
}

- (void)setBlendColorRed:(float)red green:(float)green blue:(float)blue alpha:(float)alpha {
    metal_mock_call_t* c = _mtlm_log(METAL_MOCK_FUNC_setBlendColor, self);
    c->num_args = 4;
    c->args[0].f = red;
    c->args[1].f = green;
    c->args[2].f = blue;
    c->args[3].f = alpha;
    _mtlm.render_encoder_state.blend_color[0] = red;
    _mtlm.render_encoder_state.blend_color[1] = green;
    _mtlm.render_encoder_state.blend_color[2] = blue;
    _mtlm.render_encoder_state.blend_color[3] = alpha;
}

- (void)setStencilReferenceValue:(uint32_t)referenceValue {
    metal_mock_call_t* c = _mtlm_log(METAL_MOCK_FUNC_setStencilReferenceValue, self);
    c->num_args = 1;
    c->args[0].u = referenceValue;
    _mtlm.render_encoder_state.stencil_ref = referenceValue;
}

- (void)setDepthBias:(float)depthBias slopeScale:(float)slopeScale clamp:(float)clamp {
    metal_mock_call_t* c = _mtlm_log(METAL_MOCK_FUNC_setDepthBias, self);
    c->num_args = 3;
    c->args[0].f = depthBias;
    c->args[1].f = slopeScale;
    c->args[2].f = clamp;
    _mtlm.render_encoder_state.depth_bias = depthBias;
    _mtlm.render_encoder_state.depth_bias_slope_scale = slopeScale;
    _mtlm.render_encoder_state.depth_bias_clamp = clamp;
}

- (void)setRenderPipelineState:(id<MTLRenderPipelineState>)pipelineState {
    metal_mock_call_t* c = _mtlm_log(METAL_MOCK_FUNC_setRenderPipelineState, self);
    c->num_args = 1;
    c->args[0].p = pipelineState;
    _mtlm.render_encoder_state.pipeline_state = pipelineState;
}

- (void)setDepthStencilState:(id<MTLDepthStencilState>)depthStencilState {
    metal_mock_call_t* c = _mtlm_log(METAL_MOCK_FUNC_setDepthStencilState, self);
    c->num_args = 1;
    c->args[0].p = depthStencilState;
    _mtlm.render_encoder_state.depth_stencil_state = depthStencilState;
}

- (void)setVertexBuffer:(id<MTLBuffer>)buffer offset:(NSUInteger)offset atIndex:(NSUInteger)index {
    metal_mock_call_t* c = _mtlm_log(METAL_MOCK_FUNC_setVertexBuffer, self);
    c->num_args = 3;
    c->args[0].p = buffer;
    c->args[1].u = offset;
    c->args[2].u = index;
    _MTLM_ASSERT(index < METAL_MOCK_MAX_BINDING_SLOTS);
    _mtlm.render_encoder_state.vertex_buffers[index].buffer = buffer;
    _mtlm.render_encoder_state.vertex_buffers[index].offset = offset;
}

- (void)setVertexBufferOffset:(NSUInteger)offset atIndex:(NSUInteger)index {
    metal_mock_call_t* c = _mtlm_log(METAL_MOCK_FUNC_setVertexBufferOffset, self);
    c->num_args = 2;
    c->args[0].u = offset;
    c->args[1].u = index;
    _MTLM_ASSERT(index < METAL_MOCK_MAX_BINDING_SLOTS);
    _mtlm.render_encoder_state.vertex_buffers[index].offset = offset;
}

- (void)setVertexTexture:(id<MTLTexture>)texture atIndex:(NSUInteger)index {
    metal_mock_call_t* c = _mtlm_log(METAL_MOCK_FUNC_setVertexTexture, self);
    c->num_args = 2;
    c->args[0].p = texture;
    c->args[1].u = index;
    _MTLM_ASSERT(index < METAL_MOCK_MAX_BINDING_SLOTS);
    _mtlm.render_encoder_state.vertex_textures[index] = texture;
}

- (void)setVertexSamplerState:(id<MTLSamplerState>)sampler atIndex:(NSUInteger)index {
    metal_mock_call_t* c = _mtlm_log(METAL_MOCK_FUNC_setVertexSamplerState, self);
    c->num_args = 2;
    c->args[0].p = sampler;
    c->args[1].u = index;
    _MTLM_ASSERT(index < METAL_MOCK_MAX_BINDING_SLOTS);
    _mtlm.render_encoder_state.vertex_samplers[index] = sampler;
}

- (void)setFragmentBuffer:(id<MTLBuffer>)buffer offset:(NSUInteger)offset atIndex:(NSUInteger)index {
    metal_mock_call_t* c = _mtlm_log(METAL_MOCK_FUNC_setFragmentBuffer, self);
    c->num_args = 3;
    c->args[0].p = buffer;
    c->args[1].u = offset;
    c->args[2].u = index;
    _MTLM_ASSERT(index < METAL_MOCK_MAX_BINDING_SLOTS);
    _mtlm.render_encoder_state.fragment_buffers[index].buffer = buffer;
    _mtlm.render_encoder_state.fragment_buffers[index].offset = offset;
}

- (void)setFragmentBufferOffset:(NSUInteger)offset atIndex:(NSUInteger)index {
    metal_mock_call_t* c = _mtlm_log(METAL_MOCK_FUNC_setFragmentBufferOffset, self);
    c->num_args = 2;
    c->args[0].u = offset;
    c->args[1].u = index;
    _MTLM_ASSERT(index < METAL_MOCK_MAX_BINDING_SLOTS);
    _mtlm.render_encoder_state.fragment_buffers[index].offset = offset;
}

- (void)setFragmentTexture:(id<MTLTexture>)texture atIndex:(NSUInteger)index {
    metal_mock_call_t* c = _mtlm_log(METAL_MOCK_FUNC_setFragmentTexture, self);
    c->num_args = 2;
    c->args[0].p = texture;
    c->args[1].u = index;
    _MTLM_ASSERT(index < METAL_MOCK_MAX_BINDING_SLOTS);
    _mtlm.render_encoder_state.fragment_textures[index] = texture;
}

- (void)setFragmentSamplerState:(id<MTLSamplerState>)sampler atIndex:(NSUInteger)index {
    metal_mock_call_t* c = _mtlm_log(METAL_MOCK_FUNC_setFragmentSamplerState, self);
    c->num_args = 2;
    c->args[0].p = sampler;
    c->args[1].u = index;
    _MTLM_ASSERT(index < METAL_MOCK_MAX_BINDING_SLOTS);
    _mtlm.render_encoder_state.fragment_samplers[index] = sampler;
}

- (void)drawPrimitives:(MTLPrimitiveType)primitiveType
           vertexStart:(NSUInteger)vertexStart
           vertexCount:(NSUInteger)vertexCount
         instanceCount:(NSUInteger)instanceCount
{
    metal_mock_call_t* c = _mtlm_log(METAL_MOCK_FUNC_drawPrimitives, self);
    c->num_args = 4;
    c->args[0].u = primitiveType;
    c->args[1].u = vertexStart;
    c->args[2].u = vertexCount;
    c->args[3].u = instanceCount;
}

- (void)drawPrimitives:(MTLPrimitiveType)primitiveType
           vertexStart:(NSUInteger)vertexStart
           vertexCount:(NSUInteger)vertexCount
         instanceCount:(NSUInteger)instanceCount
          baseInstance:(NSUInteger)baseInstance
{
    metal_mock_call_t* c = _mtlm_log(METAL_MOCK_FUNC_drawPrimitivesBaseInstance, self);
    c->num_args = 5;
    c->args[0].u = primitiveType;
    c->args[1].u = vertexStart;
    c->args[2].u = vertexCount;
    c->args[3].u = instanceCount;
    c->args[4].u = baseInstance;
}

- (void)drawIndexedPrimitives:(MTLPrimitiveType)primitiveType
                   indexCount:(NSUInteger)indexCount
                    indexType:(MTLIndexType)indexType
                  indexBuffer:(id<MTLBuffer>)indexBuffer
            indexBufferOffset:(NSUInteger)indexBufferOffset
                instanceCount:(NSUInteger)instanceCount
{
    metal_mock_call_t* c = _mtlm_log(METAL_MOCK_FUNC_drawIndexedPrimitives, self);
    c->num_args = 6;
    c->args[0].u = primitiveType;
    c->args[1].u = indexCount;
    c->args[2].u = indexType;
    c->args[3].p = indexBuffer;
    c->args[4].u = indexBufferOffset;
    c->args[5].u = instanceCount;
}

- (void)drawIndexedPrimitives:(MTLPrimitiveType)primitiveType
                   indexCount:(NSUInteger)indexCount
                    indexType:(MTLIndexType)indexType
                  indexBuffer:(id<MTLBuffer>)indexBuffer
            indexBufferOffset:(NSUInteger)indexBufferOffset
                instanceCount:(NSUInteger)instanceCount
                   baseVertex:(NSInteger)baseVertex
                 baseInstance:(NSUInteger)baseInstance
{
    metal_mock_call_t* c = _mtlm_log(METAL_MOCK_FUNC_drawIndexedPrimitivesBaseVertex, self);
    c->num_args = 8;
    c->args[0].u = primitiveType;
    c->args[1].u = indexCount;
    c->args[2].u = indexType;
    c->args[3].p = indexBuffer;
    c->args[4].u = indexBufferOffset;
    c->args[5].u = instanceCount;
    c->args[6].i = baseVertex;
    c->args[7].u = baseInstance;
}
@end

@implementation _mtlm_compute_encoder

- (void)endEncoding {
    _mtlm_log(METAL_MOCK_FUNC_endEncoding, self);
    _mtlm.compute_encoder_state.ended = true;
}

- (void)pushDebugGroup:(NSString*)string {
    metal_mock_call_t* c = _mtlm_log(METAL_MOCK_FUNC_pushDebugGroup, self);
    c->num_args = 1;
    _mtlm_log_str(c, 0, string);
    _mtlm.compute_encoder_state.debug_group_depth++;
}

- (void)popDebugGroup {
    _mtlm_log(METAL_MOCK_FUNC_popDebugGroup, self);
    _mtlm.compute_encoder_state.debug_group_depth--;
}

- (void)setLabel:(NSString*)label {
    [super setLabel:label];
    _mtlm_copy_str(_mtlm.compute_encoder_state.label, label);
}

- (void)setComputePipelineState:(id<MTLComputePipelineState>)state {
    metal_mock_call_t* c = _mtlm_log(METAL_MOCK_FUNC_setComputePipelineState, self);
    c->num_args = 1;
    c->args[0].p = state;
    _mtlm.compute_encoder_state.pipeline_state = state;
}

- (void)setBuffer:(id<MTLBuffer>)buffer offset:(NSUInteger)offset atIndex:(NSUInteger)index {
    metal_mock_call_t* c = _mtlm_log(METAL_MOCK_FUNC_setBuffer, self);
    c->num_args = 3;
    c->args[0].p = buffer;
    c->args[1].u = offset;
    c->args[2].u = index;
    _MTLM_ASSERT(index < METAL_MOCK_MAX_BINDING_SLOTS);
    _mtlm.compute_encoder_state.buffers[index].buffer = buffer;
    _mtlm.compute_encoder_state.buffers[index].offset = offset;
}

- (void)setBufferOffset:(NSUInteger)offset atIndex:(NSUInteger)index {
    metal_mock_call_t* c = _mtlm_log(METAL_MOCK_FUNC_setBufferOffset, self);
    c->num_args = 2;
    c->args[0].u = offset;
    c->args[1].u = index;
    _MTLM_ASSERT(index < METAL_MOCK_MAX_BINDING_SLOTS);
    _mtlm.compute_encoder_state.buffers[index].offset = offset;
}

- (void)setTexture:(id<MTLTexture>)texture atIndex:(NSUInteger)index {
    metal_mock_call_t* c = _mtlm_log(METAL_MOCK_FUNC_setTexture, self);
    c->num_args = 2;
    c->args[0].p = texture;
    c->args[1].u = index;
    _MTLM_ASSERT(index < METAL_MOCK_MAX_BINDING_SLOTS);
    _mtlm.compute_encoder_state.textures[index] = texture;
}

- (void)setSamplerState:(id<MTLSamplerState>)sampler atIndex:(NSUInteger)index {
    metal_mock_call_t* c = _mtlm_log(METAL_MOCK_FUNC_setSamplerState, self);
    c->num_args = 2;
    c->args[0].p = sampler;
    c->args[1].u = index;
    _MTLM_ASSERT(index < METAL_MOCK_MAX_BINDING_SLOTS);
    _mtlm.compute_encoder_state.samplers[index] = sampler;
}

- (void)dispatchThreadgroups:(MTLSize)threadgroupsPerGrid threadsPerThreadgroup:(MTLSize)threadsPerThreadgroup {
    metal_mock_call_t* c = _mtlm_log(METAL_MOCK_FUNC_dispatchThreadgroups, self);
    c->num_args = 6;
    c->args[0].u = threadgroupsPerGrid.width;
    c->args[1].u = threadgroupsPerGrid.height;
    c->args[2].u = threadgroupsPerGrid.depth;
    c->args[3].u = threadsPerThreadgroup.width;
    c->args[4].u = threadsPerThreadgroup.height;
    c->args[5].u = threadsPerThreadgroup.depth;
    _mtlm.compute_encoder_state.last_threadgroups_per_grid = threadgroupsPerGrid;
    _mtlm.compute_encoder_state.last_threads_per_threadgroup = threadsPerThreadgroup;
    _mtlm.compute_encoder_state.num_dispatches++;
}
@end

//== command buffer and queue ==================================================

// copy one MTLRenderPassAttachmentDescriptor into the snapshot struct
static void _mtlm_snapshot_att(metal_mock_pass_attachment_t* dst, MTLRenderPassAttachmentDescriptor* src) {
    dst->texture = src.texture;
    dst->level = src.level;
    dst->slice = src.slice;
    dst->depth_plane = src.depthPlane;
    dst->resolve_texture = src.resolveTexture;
    dst->resolve_level = src.resolveLevel;
    dst->resolve_slice = src.resolveSlice;
    dst->resolve_depth_plane = src.resolveDepthPlane;
    dst->load_action = src.loadAction;
    dst->store_action = src.storeAction;
}

static void _mtlm_snapshot_pass(MTLRenderPassDescriptor* desc) {
    metal_mock_render_pass_info_t* dst = &_mtlm.last_render_pass;
    memset(dst, 0, sizeof(metal_mock_render_pass_info_t));
    for (int i = 0; i < _MTLM_MAX_PASS_COLOR_ATTS; i++) {
        MTLRenderPassColorAttachmentDescriptor* src = desc.colorAttachments[(NSUInteger)i];
        if (nil == src.texture) {
            continue;
        }
        _mtlm_snapshot_att(&dst->color_attachments[i], src);
        dst->color_attachments[i].clear_color = src.clearColor;
        dst->num_color_attachments = i + 1;
    }
    if (desc.depthAttachment.texture) {
        dst->has_depth_attachment = true;
        _mtlm_snapshot_att(&dst->depth_attachment, desc.depthAttachment);
        dst->depth_attachment.clear_depth = desc.depthAttachment.clearDepth;
    }
    if (desc.stencilAttachment.texture) {
        dst->has_stencil_attachment = true;
        _mtlm_snapshot_att(&dst->stencil_attachment, desc.stencilAttachment);
        dst->stencil_attachment.clear_stencil = desc.stencilAttachment.clearStencil;
    }
}

@implementation _mtlm_command_buffer

- (instancetype)initWithKind:(metal_mock_obj_t)kind {
    self = [super initWithKind:kind];
    if (self) {
        handlers = [[NSMutableArray alloc] init];
    }
    return self;
}

- (void)dealloc {
    _mtlm.num_pending_handlers -= (int)[handlers count];
    [handlers release];
    [super dealloc];
}

- (void)runCompletedHandlers {
    NSArray* pending = [[handlers copy] autorelease];
    _mtlm.num_pending_handlers -= (int)[handlers count];
    [handlers removeAllObjects];
    for (MTLCommandBufferHandler handler in pending) {
        handler(self);
    }
}

- (void)enqueue {
    _mtlm_log(METAL_MOCK_FUNC_enqueue, self);
}

- (void)commit {
    _mtlm_log(METAL_MOCK_FUNC_commit, self);
    // the mock runs completion handlers right away, so the semaphore which
    // sokol_gfx.h waits on in sg_begin_pass() is signalled deterministically
    [self runCompletedHandlers];
}

- (void)addCompletedHandler:(MTLCommandBufferHandler)block {
    _mtlm_log(METAL_MOCK_FUNC_addCompletedHandler, self);
    MTLCommandBufferHandler copy = [block copy];
    [handlers addObject:copy];
    [copy release];
    _mtlm.num_pending_handlers++;
}

- (void)presentDrawable:(id<MTLDrawable>)drawable {
    metal_mock_call_t* c = _mtlm_log(METAL_MOCK_FUNC_presentDrawable, self);
    c->num_args = 1;
    c->args[0].p = drawable;
}

- (id<MTLRenderCommandEncoder>)renderCommandEncoderWithDescriptor:(MTLRenderPassDescriptor*)renderPassDescriptor {
    metal_mock_call_t* c = _mtlm_log(METAL_MOCK_FUNC_renderCommandEncoderWithDescriptor, self);
    c->num_args = 1;
    c->args[0].p = renderPassDescriptor;
    _mtlm_snapshot_pass(renderPassDescriptor);
    if (_mtlm_fail(METAL_MOCK_OBJ_RENDER_ENCODER)) {
        return nil;
    }
    memset(&_mtlm.render_encoder_state, 0, sizeof(_mtlm.render_encoder_state));
    _mtlm_render_encoder* enc = [[_mtlm_render_encoder alloc] initWithKind:METAL_MOCK_OBJ_RENDER_ENCODER];
    return [enc autorelease];
}

- (id<MTLComputeCommandEncoder>)computeCommandEncoder {
    _mtlm_log(METAL_MOCK_FUNC_computeCommandEncoder, self);
    if (_mtlm_fail(METAL_MOCK_OBJ_COMPUTE_ENCODER)) {
        return nil;
    }
    memset(&_mtlm.compute_encoder_state, 0, sizeof(_mtlm.compute_encoder_state));
    _mtlm_compute_encoder* enc = [[_mtlm_compute_encoder alloc] initWithKind:METAL_MOCK_OBJ_COMPUTE_ENCODER];
    return [enc autorelease];
}
@end

@implementation _mtlm_command_queue

- (id<MTLCommandBuffer>)commandBuffer {
    _mtlm_log(METAL_MOCK_FUNC_commandBuffer, self);
    if (_mtlm_fail(METAL_MOCK_OBJ_COMMAND_BUFFER)) {
        return nil;
    }
    _mtlm_command_buffer* cmd_buf = [[_mtlm_command_buffer alloc] initWithKind:METAL_MOCK_OBJ_COMMAND_BUFFER];
    return [cmd_buf autorelease];
}

- (id<MTLCommandBuffer>)commandBufferWithUnretainedReferences {
    _mtlm_log(METAL_MOCK_FUNC_commandBufferWithUnretainedReferences, self);
    if (_mtlm_fail(METAL_MOCK_OBJ_COMMAND_BUFFER)) {
        return nil;
    }
    _mtlm_command_buffer* cmd_buf = [[_mtlm_command_buffer alloc] initWithKind:METAL_MOCK_OBJ_COMMAND_BUFFER];
    return [cmd_buf autorelease];
}
@end

//== device ====================================================================

// The device is a singleton owned by the mock, created on first setup and
// never released. Refcounting is left to NSObject, so an unbalanced release
// by sokol_gfx.h aborts the test.
@interface _mtlm_device : NSObject <MTLDevice>
@end

@implementation _mtlm_device

- (BOOL)supportsFamily:(MTLGPUFamily)gpuFamily {
    metal_mock_call_t* c = _mtlm_log(METAL_MOCK_FUNC_supportsFamily, self);
    c->num_args = 1;
    c->args[0].i = gpuFamily;
    for (int i = 0; i < _mtlm.num_families; i++) {
        if (_mtlm.families[i].family == gpuFamily) {
            return _mtlm.families[i].supported ? YES : NO;
        }
    }
    return YES;
}

- (id<MTLCommandQueue>)newCommandQueue {
    _mtlm_log(METAL_MOCK_FUNC_newCommandQueue, self);
    if (_mtlm_fail(METAL_MOCK_OBJ_COMMAND_QUEUE)) {
        return nil;
    }
    return [[_mtlm_command_queue alloc] initWithKind:METAL_MOCK_OBJ_COMMAND_QUEUE];
}

- (id<MTLBuffer>)newBufferWithLength:(NSUInteger)length options:(MTLResourceOptions)options {
    metal_mock_call_t* c = _mtlm_log(METAL_MOCK_FUNC_newBufferWithLength, self);
    c->num_args = 2;
    c->args[0].u = length;
    c->args[1].u = options;
    if (_mtlm_fail(METAL_MOCK_OBJ_BUFFER)) {
        return nil;
    }
    _mtlm_buffer* buf = [[_mtlm_buffer alloc] initWithKind:METAL_MOCK_OBJ_BUFFER];
    buf->info.length = length;
    buf->info.options = options;
    buf->data = (uint8_t*) calloc(1, length > 0 ? length : 1);
    return buf;
}

- (id<MTLBuffer>)newBufferWithBytes:(const void*)pointer length:(NSUInteger)length options:(MTLResourceOptions)options {
    metal_mock_call_t* c = _mtlm_log(METAL_MOCK_FUNC_newBufferWithBytes, self);
    c->num_args = 3;
    c->args[0].p = pointer;
    c->args[1].u = length;
    c->args[2].u = options;
    if (_mtlm_fail(METAL_MOCK_OBJ_BUFFER)) {
        return nil;
    }
    _mtlm_buffer* buf = [[_mtlm_buffer alloc] initWithKind:METAL_MOCK_OBJ_BUFFER];
    buf->info.length = length;
    buf->info.options = options;
    buf->info.with_bytes = true;
    buf->data = (uint8_t*) calloc(1, length > 0 ? length : 1);
    if (pointer && (length > 0)) {
        memcpy(buf->data, pointer, length);
    }
    return buf;
}

- (id<MTLTexture>)newTextureWithDescriptor:(MTLTextureDescriptor*)descriptor {
    metal_mock_call_t* c = _mtlm_log(METAL_MOCK_FUNC_newTextureWithDescriptor, self);
    c->num_args = 1;
    c->args[0].p = descriptor;
    if (_mtlm_fail(METAL_MOCK_OBJ_TEXTURE)) {
        return nil;
    }
    _mtlm_texture* tex = [[_mtlm_texture alloc] initWithKind:METAL_MOCK_OBJ_TEXTURE];
    tex->info.texture_type = descriptor.textureType;
    tex->info.pixel_format = descriptor.pixelFormat;
    tex->info.width = descriptor.width;
    tex->info.height = descriptor.height;
    tex->info.depth = descriptor.depth;
    tex->info.mipmap_level_count = descriptor.mipmapLevelCount;
    tex->info.sample_count = descriptor.sampleCount;
    tex->info.array_length = descriptor.arrayLength;
    tex->info.usage = descriptor.usage;
    tex->info.resource_options = descriptor.resourceOptions;
    _mtlm_copy_str(tex->info.label, descriptor.label);
    return tex;
}

- (id<MTLSamplerState>)newSamplerStateWithDescriptor:(MTLSamplerDescriptor*)descriptor {
    metal_mock_call_t* c = _mtlm_log(METAL_MOCK_FUNC_newSamplerStateWithDescriptor, self);
    c->num_args = 1;
    c->args[0].p = descriptor;
    if (_mtlm_fail(METAL_MOCK_OBJ_SAMPLER)) {
        return nil;
    }
    _mtlm_sampler* smp = [[_mtlm_sampler alloc] initWithKind:METAL_MOCK_OBJ_SAMPLER];
    smp->info.min_filter = descriptor.minFilter;
    smp->info.mag_filter = descriptor.magFilter;
    smp->info.mip_filter = descriptor.mipFilter;
    smp->info.s_address_mode = descriptor.sAddressMode;
    smp->info.t_address_mode = descriptor.tAddressMode;
    smp->info.r_address_mode = descriptor.rAddressMode;
    smp->info.border_color = descriptor.borderColor;
    smp->info.max_anisotropy = descriptor.maxAnisotropy;
    smp->info.lod_min_clamp = descriptor.lodMinClamp;
    smp->info.lod_max_clamp = descriptor.lodMaxClamp;
    smp->info.compare_function = descriptor.compareFunction;
    smp->info.normalized_coordinates = descriptor.normalizedCoordinates;
    _mtlm_copy_str(smp->info.label, descriptor.label);
    return smp;
}

- (id<MTLLibrary>)newLibraryWithSource:(NSString*)source
                               options:(MTLCompileOptions*)options
                                 error:(__autoreleasing NSError**)error
{
    metal_mock_call_t* c = _mtlm_log(METAL_MOCK_FUNC_newLibraryWithSource, self);
    c->num_args = 1;
    _mtlm_log_str(c, 0, source);
    (void)options;
    if (_mtlm_fail(METAL_MOCK_OBJ_LIBRARY)) {
        if (error) {
            *error = _mtlm_error();
        }
        return nil;
    }
    _mtlm_library* lib = [[_mtlm_library alloc] initWithKind:METAL_MOCK_OBJ_LIBRARY];
    _mtlm_copy_str(lib->info.source, source);
    return lib;
}

- (id<MTLLibrary>)newLibraryWithData:(dispatch_data_t)data error:(__autoreleasing NSError**)error {
    metal_mock_call_t* c = _mtlm_log(METAL_MOCK_FUNC_newLibraryWithData, self);
    c->num_args = 1;
    c->args[0].u = dispatch_data_get_size(data);
    if (_mtlm_fail(METAL_MOCK_OBJ_LIBRARY)) {
        if (error) {
            *error = _mtlm_error();
        }
        return nil;
    }
    _mtlm_library* lib = [[_mtlm_library alloc] initWithKind:METAL_MOCK_OBJ_LIBRARY];
    lib->info.from_bytecode = true;
    return lib;
}

- (id<MTLRenderPipelineState>)newRenderPipelineStateWithDescriptor:(MTLRenderPipelineDescriptor*)descriptor
                                                             error:(__autoreleasing NSError**)error
{
    metal_mock_call_t* c = _mtlm_log(METAL_MOCK_FUNC_newRenderPipelineStateWithDescriptor, self);
    c->num_args = 1;
    c->args[0].p = descriptor;
    if (_mtlm_fail(METAL_MOCK_OBJ_RENDER_PIPELINE)) {
        if (error) {
            *error = _mtlm_error();
        }
        return nil;
    }
    _mtlm_render_pipeline* rps = [[_mtlm_render_pipeline alloc] initWithKind:METAL_MOCK_OBJ_RENDER_PIPELINE];
    metal_mock_render_pipeline_info_t* info = &rps->info;
    info->vertex_function = descriptor.vertexFunction;
    info->fragment_function = descriptor.fragmentFunction;
    info->has_vertex_descriptor = (nil != descriptor.vertexDescriptor);
    info->raster_sample_count = descriptor.rasterSampleCount;
    info->alpha_to_coverage_enabled = descriptor.alphaToCoverageEnabled;
    info->alpha_to_one_enabled = descriptor.alphaToOneEnabled;
    info->rasterization_enabled = descriptor.rasterizationEnabled;
    info->depth_attachment_pixel_format = descriptor.depthAttachmentPixelFormat;
    info->stencil_attachment_pixel_format = descriptor.stencilAttachmentPixelFormat;
    _mtlm_copy_str(info->label, descriptor.label);
    for (NSUInteger i = 0; i < _MTLM_MAX_PIP_COLOR_ATTS; i++) {
        MTLRenderPipelineColorAttachmentDescriptor* src = descriptor.colorAttachments[i];
        metal_mock_color_attachment_info_t* dst = &info->color_attachments[i];
        dst->pixel_format = src.pixelFormat;
        dst->blending_enabled = src.blendingEnabled;
        dst->src_rgb = src.sourceRGBBlendFactor;
        dst->dst_rgb = src.destinationRGBBlendFactor;
        dst->src_alpha = src.sourceAlphaBlendFactor;
        dst->dst_alpha = src.destinationAlphaBlendFactor;
        dst->op_rgb = src.rgbBlendOperation;
        dst->op_alpha = src.alphaBlendOperation;
        dst->write_mask = src.writeMask;
    }
    if (descriptor.vertexDescriptor) {
        for (NSUInteger i = 0; i < _MTLM_MAX_VERTEX_ATTRS; i++) {
            MTLVertexAttributeDescriptor* src = descriptor.vertexDescriptor.attributes[i];
            info->vertex_attrs[i].format = src.format;
            info->vertex_attrs[i].offset = src.offset;
            info->vertex_attrs[i].buffer_index = src.bufferIndex;
        }
        for (NSUInteger i = 0; i < _MTLM_MAX_VERTEX_LAYOUTS; i++) {
            MTLVertexBufferLayoutDescriptor* src = descriptor.vertexDescriptor.layouts[i];
            info->vertex_layouts[i].stride = src.stride;
            info->vertex_layouts[i].step_function = src.stepFunction;
            info->vertex_layouts[i].step_rate = src.stepRate;
        }
    }
    for (NSUInteger i = 0; i < _MTLM_MAX_PIPELINE_BUFFERS; i++) {
        info->vertex_buffer_mutability[i] = descriptor.vertexBuffers[i].mutability;
        info->fragment_buffer_mutability[i] = descriptor.fragmentBuffers[i].mutability;
    }
    return rps;
}

- (id<MTLComputePipelineState>)newComputePipelineStateWithDescriptor:(MTLComputePipelineDescriptor*)descriptor
                                                             options:(MTLPipelineOption)options
                                                          reflection:(MTLAutoreleasedComputePipelineReflection*)reflection
                                                               error:(__autoreleasing NSError**)error
{
    metal_mock_call_t* c = _mtlm_log(METAL_MOCK_FUNC_newComputePipelineStateWithDescriptor, self);
    c->num_args = 2;
    c->args[0].p = descriptor;
    c->args[1].u = options;
    (void)reflection;
    if (_mtlm_fail(METAL_MOCK_OBJ_COMPUTE_PIPELINE)) {
        if (error) {
            *error = _mtlm_error();
        }
        return nil;
    }
    _mtlm_compute_pipeline* cps = [[_mtlm_compute_pipeline alloc] initWithKind:METAL_MOCK_OBJ_COMPUTE_PIPELINE];
    metal_mock_compute_pipeline_info_t* info = &cps->info;
    info->compute_function = descriptor.computeFunction;
    info->threadgroup_size_is_multiple_of_thread_execution_width = descriptor.threadGroupSizeIsMultipleOfThreadExecutionWidth;
    _mtlm_copy_str(info->label, descriptor.label);
    for (NSUInteger i = 0; i < _MTLM_MAX_PIPELINE_BUFFERS; i++) {
        info->buffer_mutability[i] = descriptor.buffers[i].mutability;
    }
    return cps;
}

- (id<MTLDepthStencilState>)newDepthStencilStateWithDescriptor:(MTLDepthStencilDescriptor*)descriptor {
    metal_mock_call_t* c = _mtlm_log(METAL_MOCK_FUNC_newDepthStencilStateWithDescriptor, self);
    c->num_args = 1;
    c->args[0].p = descriptor;
    if (_mtlm_fail(METAL_MOCK_OBJ_DEPTH_STENCIL)) {
        return nil;
    }
    _mtlm_depth_stencil* dss = [[_mtlm_depth_stencil alloc] initWithKind:METAL_MOCK_OBJ_DEPTH_STENCIL];
    metal_mock_depth_stencil_info_t* info = &dss->info;
    info->depth_compare_function = descriptor.depthCompareFunction;
    info->depth_write_enabled = descriptor.depthWriteEnabled;
    _mtlm_copy_str(info->label, descriptor.label);
    MTLStencilDescriptor* front = descriptor.frontFaceStencil;
    if (front) {
        info->has_front_stencil = true;
        info->front.compare_function = front.stencilCompareFunction;
        info->front.fail_op = front.stencilFailureOperation;
        info->front.depth_fail_op = front.depthFailureOperation;
        info->front.pass_op = front.depthStencilPassOperation;
        info->front.read_mask = front.readMask;
        info->front.write_mask = front.writeMask;
    }
    MTLStencilDescriptor* back = descriptor.backFaceStencil;
    if (back) {
        info->has_back_stencil = true;
        info->back.compare_function = back.stencilCompareFunction;
        info->back.fail_op = back.stencilFailureOperation;
        info->back.depth_fail_op = back.depthFailureOperation;
        info->back.pass_op = back.depthStencilPassOperation;
        info->back.read_mask = back.readMask;
        info->back.write_mask = back.writeMask;
    }
    return dss;
}
@end

static _mtlm_device* _mtlm_the_device = nil;

//== public API ================================================================

void metal_mock_setup(void) {
    _MTLM_ASSERT(!_mtlm.valid);
    // Objects of the previous session must all be released, otherwise the
    // memset below drops them from the registry and their later dealloc
    // corrupts the live counts.
    for (int i = 0; i < METAL_MOCK_OBJ_NUM; i++) {
        if (0 != _mtlm.num_live[i]) {
            fprintf(stderr, "metal_mock: %d leaked objects of kind %d\n", _mtlm.num_live[i], i);
        }
    }
    _MTLM_ASSERT(0 == metal_mock_live_objects_total());
    memset(&_mtlm, 0, sizeof(_mtlm));
    _mtlm.valid = true;
    _mtlm.pool = [[NSAutoreleasePool alloc] init];
    strncpy(_mtlm.err_msg, "metal_mock: injected failure", METAL_MOCK_MAX_STRING - 1);
    if (nil == _mtlm_the_device) {
        _mtlm_the_device = [[_mtlm_device alloc] init];
    }
}

void metal_mock_shutdown(void) {
    _MTLM_ASSERT(_mtlm.valid);
    [_mtlm.pool release];
    _mtlm.pool = nil;
    _mtlm.valid = false;
}

void metal_mock_drain_pool(void) {
    _MTLM_ASSERT(_mtlm.valid);
    [_mtlm.pool release];
    _mtlm.pool = [[NSAutoreleasePool alloc] init];
}

const void* metal_mock_device(void) {
    _MTLM_ASSERT(_mtlm.valid);
    return _mtlm_the_device;
}

const void* metal_mock_create_texture(int width, int height, MTLPixelFormat fmt, int sample_count) {
    _MTLM_ASSERT(_mtlm.valid);
    MTLTextureDescriptor* desc = [[MTLTextureDescriptor alloc] init];
    desc.textureType = (sample_count > 1) ? MTLTextureType2DMultisample : MTLTextureType2D;
    desc.pixelFormat = fmt;
    desc.width = (NSUInteger)width;
    desc.height = (NSUInteger)height;
    desc.sampleCount = (NSUInteger)sample_count;
    desc.usage = MTLTextureUsageRenderTarget;
    id<MTLTexture> tex = [_mtlm_the_device newTextureWithDescriptor:desc];
    [desc release];
    return tex;
}

const void* metal_mock_create_drawable(int width, int height, MTLPixelFormat fmt) {
    _MTLM_ASSERT(_mtlm.valid);
    _mtlm_drawable* drawable = [[_mtlm_drawable alloc] initWithKind:METAL_MOCK_OBJ_DRAWABLE];
    drawable->tex = (id<MTLTexture>) metal_mock_create_texture(width, height, fmt, 1);
    return drawable;
}

void metal_mock_retain(const void* obj) {
    [(id)obj retain];
}

void metal_mock_release(const void* obj) {
    [(id)obj release];
}

int metal_mock_retain_count(const void* obj) {
    return (int)[(id)obj retainCount];
}

void metal_mock_complete_pending(void) {
    _MTLM_ASSERT(_mtlm.valid);
    for (int i = 0; i < _MTLM_MAX_OBJECTS; i++) {
        if (_mtlm.objects[i].ptr && (METAL_MOCK_OBJ_COMMAND_BUFFER == _mtlm.objects[i].kind)) {
            [(_mtlm_command_buffer*)_mtlm.objects[i].ptr runCompletedHandlers];
        }
    }
}

int metal_mock_num_pending_handlers(void) {
    return _mtlm.num_pending_handlers;
}

int metal_mock_live_objects(metal_mock_obj_t kind) {
    _MTLM_ASSERT((kind >= 0) && (kind < METAL_MOCK_OBJ_NUM));
    return _mtlm.num_live[kind];
}

int metal_mock_live_objects_total(void) {
    int total = 0;
    for (int i = 0; i < METAL_MOCK_OBJ_NUM; i++) {
        total += _mtlm.num_live[i];
    }
    return total;
}

int metal_mock_num_created(metal_mock_obj_t kind) {
    _MTLM_ASSERT((kind >= 0) && (kind < METAL_MOCK_OBJ_NUM));
    return _mtlm.num_created[kind];
}

bool metal_mock_is_object(metal_mock_obj_t kind, const void* obj) {
    return _mtlm_is_obj(obj, kind);
}

bool metal_mock_buffer_info(const void* obj, metal_mock_buffer_info_t* out) {
    _MTLM_ASSERT(out);
    if (!_mtlm_is_obj(obj, METAL_MOCK_OBJ_BUFFER)) {
        return false;
    }
    *out = ((_mtlm_buffer*)obj)->info;
    return true;
}

bool metal_mock_texture_info(const void* obj, metal_mock_texture_info_t* out) {
    _MTLM_ASSERT(out);
    if (!_mtlm_is_obj(obj, METAL_MOCK_OBJ_TEXTURE) && !_mtlm_is_obj(obj, METAL_MOCK_OBJ_TEXTURE_VIEW)) {
        return false;
    }
    *out = ((_mtlm_texture*)obj)->info;
    return true;
}

bool metal_mock_sampler_info(const void* obj, metal_mock_sampler_info_t* out) {
    _MTLM_ASSERT(out);
    if (!_mtlm_is_obj(obj, METAL_MOCK_OBJ_SAMPLER)) {
        return false;
    }
    *out = ((_mtlm_sampler*)obj)->info;
    return true;
}

bool metal_mock_library_info(const void* obj, metal_mock_library_info_t* out) {
    _MTLM_ASSERT(out);
    if (!_mtlm_is_obj(obj, METAL_MOCK_OBJ_LIBRARY)) {
        return false;
    }
    *out = ((_mtlm_library*)obj)->info;
    return true;
}

bool metal_mock_function_info(const void* obj, metal_mock_function_info_t* out) {
    _MTLM_ASSERT(out);
    if (!_mtlm_is_obj(obj, METAL_MOCK_OBJ_FUNCTION)) {
        return false;
    }
    *out = ((_mtlm_function*)obj)->info;
    return true;
}

bool metal_mock_render_pipeline_info(const void* obj, metal_mock_render_pipeline_info_t* out) {
    _MTLM_ASSERT(out);
    if (!_mtlm_is_obj(obj, METAL_MOCK_OBJ_RENDER_PIPELINE)) {
        return false;
    }
    *out = ((_mtlm_render_pipeline*)obj)->info;
    return true;
}

bool metal_mock_compute_pipeline_info(const void* obj, metal_mock_compute_pipeline_info_t* out) {
    _MTLM_ASSERT(out);
    if (!_mtlm_is_obj(obj, METAL_MOCK_OBJ_COMPUTE_PIPELINE)) {
        return false;
    }
    *out = ((_mtlm_compute_pipeline*)obj)->info;
    return true;
}

bool metal_mock_depth_stencil_info(const void* obj, metal_mock_depth_stencil_info_t* out) {
    _MTLM_ASSERT(out);
    if (!_mtlm_is_obj(obj, METAL_MOCK_OBJ_DEPTH_STENCIL)) {
        return false;
    }
    *out = ((_mtlm_depth_stencil*)obj)->info;
    return true;
}

const metal_mock_render_pass_info_t* metal_mock_last_render_pass(void) {
    return &_mtlm.last_render_pass;
}

const metal_mock_render_encoder_state_t* metal_mock_render_encoder_state(void) {
    return &_mtlm.render_encoder_state;
}

const metal_mock_compute_encoder_state_t* metal_mock_compute_encoder_state(void) {
    return &_mtlm.compute_encoder_state;
}

int metal_mock_num_calls(void) {
    return _mtlm.num_calls;
}

const metal_mock_call_t* metal_mock_call(int index) {
    if ((index < 0) || (index >= _mtlm.num_calls)) {
        return 0;
    }
    return &_mtlm.calls[index];
}

int metal_mock_count_calls(metal_mock_func_t func) {
    int count = 0;
    for (int i = 0; i < _mtlm.num_calls; i++) {
        if (_mtlm.calls[i].func == func) {
            count++;
        }
    }
    return count;
}

int metal_mock_find_call(metal_mock_func_t func, int start_index) {
    for (int i = (start_index < 0) ? 0 : start_index; i < _mtlm.num_calls; i++) {
        if (_mtlm.calls[i].func == func) {
            return i;
        }
    }
    return -1;
}

const metal_mock_call_t* metal_mock_last_call(metal_mock_func_t func) {
    for (int i = _mtlm.num_calls - 1; i >= 0; i--) {
        if (_mtlm.calls[i].func == func) {
            return &_mtlm.calls[i];
        }
    }
    return 0;
}

void metal_mock_clear_calls(void) {
    _mtlm.num_calls = 0;
    _mtlm.call_log_overflow = false;
}

bool metal_mock_call_log_overflow(void) {
    return _mtlm.call_log_overflow;
}

const char* metal_mock_func_name(metal_mock_func_t func) {
    static const char* names[METAL_MOCK_FUNC_NUM] = {
        "INVALID",
        #define _MTLM_XMACRO(name) #name,
        _MTLM_FUNCS
        #undef _MTLM_XMACRO
    };
    if ((func <= METAL_MOCK_FUNC_INVALID) || (func >= METAL_MOCK_FUNC_NUM)) {
        return "INVALID";
    }
    return names[func];
}

void metal_mock_fail_next(metal_mock_obj_t kind, int n) {
    _MTLM_ASSERT((kind >= 0) && (kind < METAL_MOCK_OBJ_NUM));
    _mtlm.fail_next[kind] = n;
    _mtlm.fail_skip[kind] = 0;
}

void metal_mock_fail_next_after(metal_mock_obj_t kind, int skip, int n) {
    _MTLM_ASSERT((kind >= 0) && (kind < METAL_MOCK_OBJ_NUM));
    _MTLM_ASSERT(skip >= 0);
    _mtlm.fail_next[kind] = n;
    _mtlm.fail_skip[kind] = skip;
}

void metal_mock_set_error_message(const char* msg) {
    strncpy(_mtlm.err_msg, msg ? msg : "", METAL_MOCK_MAX_STRING - 1);
    _mtlm.err_msg[METAL_MOCK_MAX_STRING - 1] = 0;
}

void metal_mock_set_supports_family(MTLGPUFamily family, bool supported) {
    for (int i = 0; i < _mtlm.num_families; i++) {
        if (_mtlm.families[i].family == family) {
            _mtlm.families[i].supported = supported;
            return;
        }
    }
    _MTLM_ASSERT(_mtlm.num_families < _MTLM_MAX_FAMILIES);
    _mtlm.families[_mtlm.num_families].family = family;
    _mtlm.families[_mtlm.num_families].supported = supported;
    _mtlm.num_families++;
}
