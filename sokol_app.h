#pragma once
/*
    sokol_app.h -- cross-platform application wrapper

    WORK IN PROGRESS!

    Do this:
        #define SOKOL_IMPL
    before you include this file in *one* C or C++ file to create the 
    implementation.

    Optionally provide the following defines with your own implementations:

    SOKOL_ASSERT(c)     - your own assert macro (default: assert(c))
    SOKOL_LOG(msg)      - your own logging function (default: puts(msg))
    SOKOL_UNREACHABLE() - a guard macro for unreachable code (default: assert(false))

    Optionally define the following to force debug checks and validations
    even in release mode:

    SOKOL_DEBUG         - by default this is defined if _DEBUG is defined

    TODO: documentation

    zlib/libpng license

    Copyright (c) 2018 Andre Weissflog

    This software is provided 'as-is', without any express or implied warranty.
    In no event will the authors be held liable for any damages arising from the
    use of this software.

    Permission is granted to anyone to use this software for any purpose,
    including commercial applications, and to alter it and redistribute it
    freely, subject to the following restrictions:

        1. The origin of this software must not be misrepresented; you must not
        claim that you wrote the original software. If you use this software in a
        product, an acknowledgment in the product documentation would be
        appreciated but is not required.

        2. Altered source versions must be plainly marked as such, and must not
        be misrepresented as being the original software.

        3. This notice may not be removed or altered from any source
        distribution.
*/
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

enum {
    SAPP_MAX_TOUCHPOINTS = 8,
};

typedef struct {
    int width;
    int height;
    int sample_count;
    bool fullscreen;
    bool alpha;
    bool depth;
    bool stencil;
    bool premultiplied_alpha;
    bool preserve_drawing_buffer;
    const char* window_title;
} sapp_desc;

typedef enum {
    SAPP_EVENTTYPE_INVALID,
    SAPP_EVENTTYPE_KEY_DOWN,
    SAPP_EVENTTYPE_KEY_UP,
    SAPP_EVENTTYPE_CHAR,
    SAPP_EVENTTYPE_MOUSE_DOWN,
    SAPP_EVENTTYPE_MOUSE_UP,
    SAPP_EVENTTYPE_MOUSE_SCROLL,
    SAPP_EVENTTYPE_MOUSE_MOVE,
    SAPP_EVENTTYPE_TOUCH_BEGAN,
    SAPP_EVENTTYPE_TOUCH_ENDED,
    SAPP_EVENTTYPE_TOUCH_CANCELLED,
    _SAPP_EVENTTYPE_NUM,
    _SAPP_EVENTTYPE_FORCE_U32 = 0x7FFFFFF
} sapp_event_type;

typedef struct {
    uintptr_t id;
    float pos_x;
    float pos_y;
    bool changed;
} sapp_touchpoint;

enum {
    SAPP_MODIFIER_SHIFT = (1<<0),
    SAPP_MODIFIER_CTRL = (1<<1),
    SAPP_MODIFIER_ALT = (1<<2),
    SAPP_MODIFIER_SUPER = (1<<3)
};

typedef struct {
    sapp_event_type type;
    uint32_t frame_count;
    uint32_t key_code;
    uint32_t char_code;
    uint32_t mouse_button;
    uint32_t modifiers;
    float mouse_x;
    float mouse_y;
    float scroll_x;
    float scroll_y;
    int num_touches;
    sapp_touchpoint touches[SAPP_MAX_TOUCHPOINTS];
} sapp_event;

typedef void (*sapp_event_callback)(const sapp_event*);

/* user-provided functions */
extern void sokol_init();
extern void sokol_frame();
extern void sokol_shutdown();

/* sokol_app API functions */
extern void sapp_setup(const sapp_desc* desc);
extern void sapp_shutdown();
extern bool sapp_isvalid();
extern int sapp_width();
extern int sapp_height();

/* Metal specific functions */
extern const void* sapp_metal_get_device();
extern const void* sapp_metal_get_renderpass_descriptor();
extern const void* sapp_metal_get_drawable(); 

#ifdef __cplusplus
} /* extern "C" */
#endif

/*-- IMPLEMENTATION ----------------------------------------------------------*/
#ifdef SOKOL_IMPL

#ifndef SOKOL_DEBUG
    #ifdef _DEBUG
        #define SOKOL_DEBUG (1)
    #endif
#endif
#ifndef SOKOL_ASSERT
    #include <assert.h>
    #define SOKOL_ASSERT(c) assert(c)
#endif
#ifndef SOKOL_LOG
    #ifdef SOKOL_DEBUG 
        #include <stdio.h>
        #define SOKOL_LOG(s) { SOKOL_ASSERT(s); puts(s); }
    #else
        #define SOKOL_LOG(s)
    #endif
#endif
#ifndef _SOKOL_PRIVATE
    #if defined(__GNUC__)
        #define _SOKOL_PRIVATE __attribute__((unused)) static
    #else
        #define _SOKOL_PRIVATE static
    #endif
#endif

#ifdef __cplusplus
extern "C"
#endif

/* helper macros */
#define _sapp_def(val, def) (((val) == 0) ? (def) : (val))

enum {
    _SAPP_MAX_TITLE_LENGTH = 128,
};

typedef struct {
    bool valid;
    int width;
    int height;
    int sample_count;
    char window_title[_SAPP_MAX_TITLE_LENGTH];
} _sapp_state;
static _sapp_state _sapp;

/*== MacOS/iOS ===============================================================*/

#if defined(__APPLE__)

#if !__has_feature(objc_arc)
#error "sokol_app.h requires ARC (Automatic Reference Counting) on MacOS and iOS"
#endif

#include <TargetConditionals.h>

/*== MacOS ===================================================================*/
#if !TARGET_OS_IPHONE
#if !defined(SOKOL_METAL) && !defined(SOKOL_GLCORE33)
#error("sokol_app.h: unknown 3D API selected for MacOS, must be SOKOL_METAL or SOKOL_GLCORE33")
#endif

#import <Cocoa/Cocoa.h>
#import <QuartzCore/QuartzCore.h>
#if defined(SOKOL_METAL)
#import <Metal/Metal.h>
#import <MetalKit/MetalKit.h>
#elif defined(SOKOL_GLCORE33)
#import <GLKit/GLKit.h>
#import <QuartzCore/CoreAnimation.h>
#endif

@interface _sapp_app_delegate : NSObject<NSApplicationDelegate>
@end
@interface _sapp_window_delegate : NSObject<NSWindowDelegate>
@end
#if defined(SOKOL_METAL)
@interface _sapp_mtk_view_dlg : NSObject<MTKViewDelegate>
@end
@interface _sapp_mtk_view : MTKView;
@end
#else
@interface _sapp_gl_view : NSOpenGLView
{
    CVDisplayLinkRef display_link;
}
- (CVReturn) getFrameForTime:(const CVTimeStamp*)outputTime;
@end
#endif

static NSWindow* _sapp_window_obj;
static _sapp_window_delegate* _sapp_win_dlg_obj;
static _sapp_app_delegate* _sapp_app_dlg_obj;
#if defined(SOKOL_METAL)
static _sapp_mtk_view_dlg* _sapp_mtk_view_dlg_obj;
static _sapp_mtk_view* _sapp_mtk_view_obj;
static id<MTLDevice> _sapp_mtl_device_obj;
#elif defined(SOKOL_GLCORE33)
static NSOpenGLPixelFormat* _sapp_nsglpixelformat_obj;
static _sapp_gl_view* _sapp_gl_view_obj;
#endif

/* MacOS entry function */
int main() {
    [NSApplication sharedApplication];
    [NSApp setActivationPolicy:NSApplicationActivationPolicyRegular];
    _sapp_app_dlg_obj = [[_sapp_app_delegate alloc] init];
    [NSApp setDelegate:_sapp_app_dlg_obj];
    [NSApp activateIgnoringOtherApps:YES];
    [NSApp run];
    return 0;
}

@implementation _sapp_app_delegate
- (void)applicationDidFinishLaunching:(NSNotification*)aNotification {
    sokol_init();
}
- (BOOL)applicationShouldTerminateAfterLastWindowClosed:(NSApplication*)sender {
    return YES;
}
@end

@implementation _sapp_window_delegate
- (BOOL)windowShouldClose:(id)sender {
    sokol_shutdown();
    return YES;
}

- (void)windowDidResize:(NSNotification*)notification {
    /* FIXME */
}

- (void)windowDidMove:(NSNotification*)notification {
    /* FIXME */
}

- (void)windowDidMiniaturize:(NSNotification*)notification {
    /* FIXME */
}

- (void)windowDidDeminiaturize:(NSNotification*)notification {
    /* FIXME */
}

- (void)windowDidBecomeKey:(NSNotification*)notification {
    /* FIXME */
}

- (void)windowDidResignKey:(NSNotification*)notification {
    /* FIXME */
}
@end

#if defined(SOKOL_METAL)
@implementation _sapp_mtk_view_dlg
- (void)mtkView:(MTKView*)view drawableSizeWillChange:(CGSize)size {
    /* FIXME */
}

- (void)drawInMTKView:(MTKView*)view {
    @autoreleasepool {
        sokol_frame();
    }
}
@end

@implementation _sapp_mtk_view
- (BOOL)isOpaque {
    return YES;
}
- (BOOL)canBecomeKey {
    return YES;
}
- (BOOL)acceptsFirstResponder {
    return YES;
}
- (void)mouseDown:(NSEvent*)event {
    /* FIXME */
}
- (void)mouseDragged:(NSEvent*)event {
    /* FIXME */
}
- (void)mouseUp:(NSEvent*)event {
    /* FIXME */
}
- (void)mouseMoved:(NSEvent*)event {
    /* FIXME */
}
- (void)rightMouseDown:(NSEvent*)event {
    /* FIXME */
}
- (void)rightMouseDragged:(NSEvent*)event {
    /* FIXME */
}
- (void)rightMouseUp:(NSEvent*)event {
    /* FIXME */
}
- (void)keyDown:(NSEvent*)event {
    /* FIXME */
}
- (void)flagsChanged:(NSEvent*)event {
    /* FIXME */
}
- (void)keyUp:(NSEvent*)event {
    /* FIXME */
}
- (void)scrollWheel:(NSEvent*)event {
    /* FIXME */
}
@end
#else
static CVReturn _sapp_displaylink_cb(CVDisplayLinkRef displayLink,
    const CVTimeStamp* now,
    const CVTimeStamp* output_time,
    CVOptionFlags flags_in,
    CVOptionFlags* flags_out,
    void* display_link_context)
{
    CVReturn result = [(__bridge _sapp_gl_view*)display_link_context getFrameForTime:output_time];
    return result;
}

@implementation _sapp_gl_view
- (void) prepareOpenGL {
    GLint swapInt = 1;
    [[self openGLContext] setValues:&swapInt forParameter:NSOpenGLCPSwapInterval];
    CVDisplayLinkCreateWithActiveCGDisplays(&display_link);
    CVDisplayLinkSetOutputCallback(display_link, &_sapp_displaylink_cb, (__bridge void*) self);
    CGLContextObj cglContext = [[self openGLContext] CGLContextObj];
    CGLPixelFormatObj cglPixelFormat = [[self pixelFormat] CGLPixelFormatObj];
    CVDisplayLinkSetCurrentCGDisplayFromOpenGLContext(display_link, cglContext, cglPixelFormat);
    CVDisplayLinkStart(display_link);
}

- (CVReturn) getFrameForTime:(const CVTimeStamp*)outputTime {
    /* FIXME: this won't work, since this function is called from a
       different thread :/
    */

    sokol_frame();
    glFlush();
    return kCVReturnSuccess;
}

/*
- (void) drawRect:(NSRect)bound {
    sokol_frame();
    glFlush();
}
*/
@end
#endif

_SOKOL_PRIVATE void _sapp_setup(const sapp_desc* desc) {
    _sapp.width = _sapp_def(desc->width, 640);
    _sapp.height = _sapp_def(desc->height, 480);
    _sapp.sample_count = _sapp_def(desc->sample_count, 1);
    strncpy(_sapp.window_title, desc->window_title, sizeof(_sapp.window_title));
    _sapp.window_title[_SAPP_MAX_TITLE_LENGTH-1] = 0;

    const NSUInteger style =
        NSWindowStyleMaskTitled |
        NSWindowStyleMaskClosable |
        NSWindowStyleMaskMiniaturizable |
        NSWindowStyleMaskResizable;
    _sapp_window_obj = [[NSWindow alloc]
        initWithContentRect:NSMakeRect(0, 0, _sapp.width, _sapp.height)
        styleMask:style
        backing:NSBackingStoreBuffered
        defer:NO];
    [_sapp_window_obj setTitle:[NSString stringWithUTF8String:_sapp.window_title]];
    [_sapp_window_obj setAcceptsMouseMovedEvents:YES];
    [_sapp_window_obj center];
    [_sapp_window_obj setRestorable:YES];
    _sapp_win_dlg_obj = [[_sapp_window_delegate alloc] init];
    [_sapp_window_obj setDelegate:_sapp_win_dlg_obj];

    #if defined(SOKOL_METAL)
        _sapp_mtl_device_obj = MTLCreateSystemDefaultDevice();
        _sapp_mtk_view_dlg_obj = [[_sapp_mtk_view_dlg alloc] init];
        _sapp_mtk_view_obj = [[_sapp_mtk_view alloc] init];
        [_sapp_mtk_view_obj setPreferredFramesPerSecond:60];
        [_sapp_mtk_view_obj setDelegate:_sapp_mtk_view_dlg_obj];
        [_sapp_mtk_view_obj setDevice:_sapp_mtl_device_obj];
        [_sapp_mtk_view_obj setColorPixelFormat:MTLPixelFormatBGRA8Unorm];
        [_sapp_mtk_view_obj setDepthStencilPixelFormat:MTLPixelFormatDepth32Float_Stencil8];
        [_sapp_mtk_view_obj setSampleCount:_sapp.sample_count];
        /* FIXME: HighDPI */
        [_sapp_window_obj setContentView:_sapp_mtk_view_obj];
        [_sapp_window_obj makeFirstResponder:_sapp_mtk_view_obj];
        CGSize drawable_size = { (CGFloat) _sapp.width, (CGFloat) _sapp.height };
        [_sapp_mtk_view_obj setDrawableSize:drawable_size];
        [[_sapp_mtk_view_obj layer] setMagnificationFilter:kCAFilterNearest];
    #elif defined(SOKOL_GLCORE33)
        NSOpenGLPixelFormatAttribute attrs[32];
        int i = 0;
        attrs[i++] = NSOpenGLPFAOpenGLProfile; attrs[i++] = NSOpenGLProfileVersion3_2Core;
        attrs[i++] = NSOpenGLPFAColorSize; attrs[i++] = 24;
        attrs[i++] = NSOpenGLPFAAlphaSize; attrs[i++] = 8;
        attrs[i++] = NSOpenGLPFADepthSize; attrs[i++] = 24;
        attrs[i++] = NSOpenGLPFAStencilSize; attrs[i++] = 8;
        if (_sapp.sample_count > 1) {
            attrs[i++] = NSOpenGLPFAMultisample;
            attrs[i++] = NSOpenGLPFASampleBuffers; attrs[i++] = 1;
            attrs[i++] = NSOpenGLPFASamples; attrs[i++] = _sapp.sample_count;
        }
        else {
            attrs[i++] = NSOpenGLPFASampleBuffers; attrs[i++] = 0;
        }
        attrs[i++] = 0;
        _sapp_nsglpixelformat_obj = [[NSOpenGLPixelFormat alloc] initWithAttributes:attrs];
        _sapp_gl_view_obj = [[_sapp_gl_view alloc]
            initWithFrame:NSMakeRect(0, 0, _sapp.width, _sapp.height)
            pixelFormat:_sapp_nsglpixelformat_obj];
        [_sapp_window_obj setContentView:_sapp_gl_view_obj];
        [_sapp_window_obj makeFirstResponder:_sapp_gl_view_obj];
    #endif

    [_sapp_window_obj makeKeyAndOrderFront:nil];
}

_SOKOL_PRIVATE void _sapp_shutdown() {
    /* FIXME */
}

_SOKOL_PRIVATE int _sapp_width() {
    #if defined(SOKOL_METAL)
    return (int) [_sapp_mtk_view_obj drawableSize].width;
    #else
    const NSRect r = [_sapp_gl_view_obj convertRectToBacking:[_sapp_gl_view_obj frame]];
    return (int) r.size.width;
    #endif
}

_SOKOL_PRIVATE int _sapp_height() {
    #if defined(SOKOL_METAL)
    return (int) [_sapp_mtk_view_obj drawableSize].height;
    #else
    const NSRect r = [_sapp_gl_view_obj convertRectToBacking:[_sapp_gl_view_obj frame]];
    return (int) r.size.height;
    #endif
}
#endif

#if TARGET_OS_IPHONE

#endif

#else
#error "sokol_app.h: Unknown OS"
#endif

/*== PUBLIC API FUNCTIONS ====================================================*/
void sapp_setup(const sapp_desc* desc) {
    SOKOL_ASSERT(desc);
    memset(&_sapp, 0, sizeof(_sapp));
    _sapp_setup(desc);
    _sapp.valid = true;
}

bool sapp_isvalid() {
    return _sapp.valid;
}

void sapp_shutdown() {
    _sapp_shutdown();
    _sapp.valid = false;
}

int sapp_width() {
    return _sapp_width();
}

int sapp_height() {
    return _sapp_height();
}

const void* sapp_metal_get_device() {
    SOKOL_ASSERT(_sapp.valid);
    #if defined(SOKOL_METAL)
        const void* obj = (__bridge const void*) _sapp_mtl_device_obj;
        SOKOL_ASSERT(obj);
        return obj;
    #else
        return 0;
    #endif
}

const void* sapp_metal_get_renderpass_descriptor() {
    SOKOL_ASSERT(_sapp.valid);
    #if defined(SOKOL_METAL)
        const void* obj =  (__bridge const void*) [_sapp_mtk_view_obj currentRenderPassDescriptor];
        SOKOL_ASSERT(obj);
        return obj;
    #else
        return 0;
    #endif
}

const void* sapp_metal_get_drawable() {
    SOKOL_ASSERT(_sapp.valid);
    #if defined(SOKOL_METAL)
        const void* obj = (__bridge const void*) [_sapp_mtk_view_obj currentDrawable];
        SOKOL_ASSERT(obj);
        return obj;
    #else
        return 0;
    #endif
}

#undef _sapp_def

#endif /* SOKOL_IMPL */

#ifdef __cplusplus
} /* extern "C" */
#endif
