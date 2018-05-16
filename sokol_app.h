#pragma once
/*
    sokol_app.h -- cross-platform app model wrapper

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
extern sapp_desc sokol_main(int argc, char* argv[]);
extern void sokol_init();
extern void sokol_frame();
extern void sokol_shutdown();

/* sokol_app API functions */
extern bool sapp_isvalid();
extern int sapp_width();
extern int sapp_height();

/* GL/GLES specific functions */
extern bool sapp_gles2_fallback();

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
    bool gles2_fallback;
    bool first_frame;
    char window_title[_SAPP_MAX_TITLE_LENGTH];
    sapp_desc desc;
    int argc;
    char** argv;
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
#if defined(SOKOL_METAL)
#import <Metal/Metal.h>
#import <MetalKit/MetalKit.h>
#else
#include <OpenGL/gl3.h>
#endif

@interface _sapp_app_delegate : NSObject<NSApplicationDelegate>
@end
@interface _sapp_window_delegate : NSObject<NSWindowDelegate>
@end
#if defined(SOKOL_METAL)
@interface _sapp_mtk_view_dlg : NSObject<MTKViewDelegate>
@end
@interface _sapp_view : MTKView;
@end
#else
@interface _sapp_view : NSView
- (void)timerFired:(id)sender;
@end
#endif

static NSWindow* _sapp_window_obj;
static _sapp_window_delegate* _sapp_win_dlg_obj;
static _sapp_app_delegate* _sapp_app_dlg_obj;
static _sapp_view* _sapp_view_obj;
#if defined(SOKOL_METAL)
static _sapp_mtk_view_dlg* _sapp_mtk_view_dlg_obj;
static id<MTLDevice> _sapp_mtl_device_obj;
#elif defined(SOKOL_GLCORE33)
static NSOpenGLPixelFormat* _sapp_glpixelformat_obj;
static NSOpenGLContext* _sapp_glcontext_obj;
static NSTimer* _sapp_timer_obj;
#endif

/* MacOS entry function */
int main(int argc, char* argv[]) {
    memset(&_sapp, 0, sizeof(_sapp));
    _sapp.argc = argc;
    _sapp.argv = argv;
    _sapp.desc = sokol_main(argc, argv);
    [NSApplication sharedApplication];
    [NSApp setActivationPolicy:NSApplicationActivationPolicyRegular];
    _sapp_app_dlg_obj = [[_sapp_app_delegate alloc] init];
    [NSApp setDelegate:_sapp_app_dlg_obj];
    [NSApp activateIgnoringOtherApps:YES];
    [NSApp run];
    return 0;
}

_SOKOL_PRIVATE void _sapp_setup(const sapp_desc* desc) {
    _sapp.first_frame = true;
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
        _sapp_view_obj = [[_sapp_view alloc] init];
        [_sapp_view_obj setPreferredFramesPerSecond:60];
        [_sapp_view_obj setDelegate:_sapp_mtk_view_dlg_obj];
        [_sapp_view_obj setDevice:_sapp_mtl_device_obj];
        [_sapp_view_obj setColorPixelFormat:MTLPixelFormatBGRA8Unorm];
        [_sapp_view_obj setDepthStencilPixelFormat:MTLPixelFormatDepth32Float_Stencil8];
        [_sapp_view_obj setSampleCount:_sapp.sample_count];
        /* FIXME: HighDPI */
        [_sapp_window_obj setContentView:_sapp_view_obj];
        [_sapp_window_obj makeFirstResponder:_sapp_view_obj];
        CGSize drawable_size = { (CGFloat) _sapp.width, (CGFloat) _sapp.height };
        [_sapp_view_obj setDrawableSize:drawable_size];
        [[_sapp_view_obj layer] setMagnificationFilter:kCAFilterNearest];
    #elif defined(SOKOL_GLCORE33)
        NSOpenGLPixelFormatAttribute attrs[32];
        int i = 0;
        attrs[i++] = NSOpenGLPFAAccelerated;
        attrs[i++] = NSOpenGLPFADoubleBuffer;
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
        _sapp_glpixelformat_obj = [[NSOpenGLPixelFormat alloc] initWithAttributes:attrs];
        SOKOL_ASSERT(_sapp_glpixelformat_obj != nil);
        _sapp_glcontext_obj = [[NSOpenGLContext alloc] initWithFormat:_sapp_glpixelformat_obj shareContext:NULL];
        SOKOL_ASSERT(_sapp_glcontext_obj != nil);
        _sapp_view_obj = [[_sapp_view alloc] init];
        [_sapp_window_obj setContentView:_sapp_view_obj];
        [_sapp_window_obj makeFirstResponder:_sapp_view_obj];
        // FIXME HighDPI: [_sapp_view_obj setWantsBestRsolutionOpenGLSurface:YES];
        [_sapp_glcontext_obj setView:_sapp_view_obj];
        [_sapp_glcontext_obj makeCurrentContext];

        GLint swapInt = 1;
        [_sapp_glcontext_obj setValues:&swapInt forParameter:NSOpenGLCPSwapInterval];
        _sapp_timer_obj = [NSTimer timerWithTimeInterval:0.001
            target:_sapp_view_obj
            selector:@selector(timerFired:)
            userInfo:nil
            repeats:YES];
        [[NSRunLoop currentRunLoop] addTimer:_sapp_timer_obj forMode:NSDefaultRunLoopMode];
        [[NSRunLoop currentRunLoop] addTimer:_sapp_timer_obj forMode:NSEventTrackingRunLoopMode];
    #endif
    [_sapp_window_obj makeKeyAndOrderFront:nil];
}

@implementation _sapp_app_delegate
- (void)applicationDidFinishLaunching:(NSNotification*)aNotification {
    _sapp_setup(&_sapp.desc);
    _sapp.valid = true;
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
    #if !defined(SOKOL_METAL)
    [_sapp_glcontext_obj update];
    #endif
}

- (void)windowDidMove:(NSNotification*)notification {
    #if !defined(SOKOL_METAL)
    [_sapp_glcontext_obj update];
    #endif
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
- (void)drawInMTKView:(MTKView*)view {
    @autoreleasepool {
        const CGSize size = [_sapp_view_obj drawableSize];
        _sapp.width = size.width;
        _sapp.height = size.height;
        if (_sapp.first_frame) {
            _sapp.first_frame = false;
            sokol_init();
        }
        sokol_frame();
    }
}

- (void)mtkView:(MTKView*)view drawableSizeWillChange:(CGSize)size {
    /* this is required by the protocol, but we can't do anything useful here */
}
@end
#endif

@implementation _sapp_view
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
#if !defined(SOKOL_METAL)
- (void)timerFired:(id)sender {
    [self setNeedsDisplay:YES];
}

- (void) drawRect:(NSRect)bound {
    const NSRect r = [_sapp_view_obj convertRectToBacking:[_sapp_view_obj frame]];
    _sapp.width = r.size.width;
    _sapp.height = r.size.height;
    [_sapp_glcontext_obj makeCurrentContext];
    if (_sapp.first_frame) {
        _sapp.first_frame = false;
        sokol_init();
    }
    sokol_frame();
    glFlush();
    [_sapp_glcontext_obj flushBuffer];
}
#endif
@end

#endif

/*== MacOS ===================================================================*/
#if TARGET_OS_IPHONE
#if !defined(SOKOL_METAL) && !defined(SOKOL_GLES3)
#error("sokol_app.h: unknown 3D API selected for iOS, must be SOKOL_METAL or SOKOL_GLES3")
#endif

#import <UIKit/UIKit.h>
#if defined(SOKOL_METAL)
#import <Metal/Metal.h>
#import <MetalKit/MetalKit.h>
#else
#import <GLKit/GLKit.h>
#include <OpenGLES/ES3/gl.h>
#include <OpenGLES/ES3/glext.h>
#endif

@interface _sapp_app_delegate : NSObject<UIApplicationDelegate>
@end
#if defined(SOKOL_METAL)
@interface _sapp_mtk_view_dlg : NSObject<MTKViewDelegate>
@end
@interface _sapp_view : MTKView;
@end
#else
@interface _sapp_glk_view_dlg : NSObject<GLKViewDelegate>
@end
@interface _sapp_view : GLKView
@end
#endif

static UIWindow* _sapp_window_obj;
static _sapp_view* _sapp_view_obj;
#if defined(SOKOL_METAL)
static _sapp_mtk_view_dlg* _sapp_mtk_view_dlg_obj;
static UIViewController<MTKViewDelegate>* _sapp_mtk_view_ctrl_obj;
static id<MTLDevice> _sapp_mtl_device_obj;
#else
static EAGLContext* _sapp_eagl_ctx_obj;
static _sapp_glk_view_dlg* _sapp_glk_view_dlg_obj;
static GLKViewController* _sapp_glk_view_ctrl_obj;
#endif

/* iOS entry function */
int main(int argc, char** argv) {
    @autoreleasepool {
        memset(&_sapp, 0, sizeof(_sapp));
        _sapp.argc = argc;
        _sapp.argv = argv;
        _sapp.desc = sokol_main(argc, argv);
        UIApplicationMain(argc, argv, nil, NSStringFromClass([_sapp_app_delegate class]));
    }
    return 0;
}

_SOKOL_PRIVATE void _sapp_setup(const sapp_desc* desc) {
    _sapp.first_frame = true;
    _sapp.width = _sapp_def(desc->width, 640);
    _sapp.height = _sapp_def(desc->height, 480);
    _sapp.sample_count = _sapp_def(desc->sample_count, 1);
    strncpy(_sapp.window_title, desc->window_title, sizeof(_sapp.window_title));
    _sapp.window_title[_SAPP_MAX_TITLE_LENGTH-1] = 0;

    CGRect screen_rect = [[UIScreen mainScreen] bounds];
    _sapp_window_obj = [[UIWindow alloc] initWithFrame:screen_rect];

    #if defined(SOKOL_METAL)
        _sapp_mtl_device_obj = MTLCreateSystemDefaultDevice();
        _sapp_mtk_view_dlg_obj = [[_sapp_mtk_view_dlg alloc] init];
        _sapp_view_obj = [[_sapp_view alloc] init];
        [_sapp_view_obj setPreferredFramesPerSecond:60];
        [_sapp_view_obj setDelegate:_sapp_mtk_view_dlg_obj];
        [_sapp_view_obj setDevice:_sapp_mtl_device_obj];
        [_sapp_view_obj setColorPixelFormat:MTLPixelFormatBGRA8Unorm];
        [_sapp_view_obj setDepthStencilPixelFormat:MTLPixelFormatDepth32Float_Stencil8];
        [_sapp_view_obj setSampleCount:_sapp.sample_count];
        /* FIXME: HighDPI */
        [_sapp_view_obj setContentScaleFactor:1.0];
        [_sapp_view_obj setUserInteractionEnabled:YES];
        [_sapp_view_obj setMultipleTouchEnabled:YES];
        [_sapp_window_obj addSubview:_sapp_view_obj];
        _sapp_mtk_view_ctrl_obj = [[UIViewController<MTKViewDelegate> alloc] init];
        [_sapp_mtk_view_ctrl_obj setView:_sapp_view_obj];
        [_sapp_window_obj setRootViewController:_sapp_mtk_view_ctrl_obj];
    #else
        _sapp_eagl_ctx_obj = [[EAGLContext alloc] initWithAPI:kEAGLRenderingAPIOpenGLES3];
        if (_sapp_eagl_ctx_obj == nil) {
            _sapp_eagl_ctx_obj = [[EAGLContext alloc] initWithAPI:kEAGLRenderingAPIOpenGLES2];
            _sapp.gles2_fallback = true;
        }
        _sapp_glk_view_dlg_obj = [[_sapp_glk_view_dlg alloc] init];
        _sapp_view_obj = [[_sapp_view alloc] initWithFrame:screen_rect];
        [_sapp_view_obj setDrawableColorFormat:GLKViewDrawableColorFormatRGBA8888];
        [_sapp_view_obj setDrawableDepthFormat:GLKViewDrawableDepthFormat24];
        [_sapp_view_obj setDrawableStencilFormat:GLKViewDrawableStencilFormatNone];
        [_sapp_view_obj setDrawableMultisample:GLKViewDrawableMultisampleNone]; /* FIXME */
        [_sapp_view_obj setContext:_sapp_eagl_ctx_obj];
        [_sapp_view_obj setDelegate:_sapp_glk_view_dlg_obj];
        [_sapp_view_obj setEnableSetNeedsDisplay:NO];
        [_sapp_view_obj setUserInteractionEnabled:YES];
        [_sapp_view_obj setMultipleTouchEnabled:YES];
        /* FIXME: HighDPI */
        [_sapp_view_obj setContentScaleFactor:1.0];
        [_sapp_window_obj addSubview:_sapp_view_obj];
        _sapp_glk_view_ctrl_obj = [[GLKViewController alloc] init];
        [_sapp_glk_view_ctrl_obj setView:_sapp_view_obj];
        [_sapp_glk_view_ctrl_obj setPreferredFramesPerSecond:60];
        [_sapp_window_obj setRootViewController:_sapp_glk_view_ctrl_obj];
    #endif
    [_sapp_window_obj makeKeyAndVisible];
}

@implementation _sapp_app_delegate
- (BOOL)application:(UIApplication*)application didFinishLaunchingWithOptions:(NSDictionary*)launchOptions {
    _sapp_setup(&_sapp.desc);
    _sapp.valid = true;
    return YES;
}
@end

#if defined(SOKOL_METAL)
@implementation _sapp_mtk_view_dlg
- (void)drawInMTKView:(MTKView*)view {
    @autoreleasepool {
        const CGSize size = [_sapp_view_obj drawableSize];
        _sapp.width = size.width;
        _sapp.height = size.height;
        if (_sapp.first_frame) {
            _sapp.first_frame = false;
            sokol_init();
        }
        sokol_frame();
    }
}

- (void)mtkView:(MTKView*)view drawableSizeWillChange:(CGSize)size {
    /* this is required by the protocol, but we can't do anything useful here */
}
@end
#else
@implementation _sapp_glk_view_dlg
- (void)glkView:(GLKView*)view drawInRect:(CGRect)rect {
    @autoreleasepool {
        _sapp.width = (int) [_sapp_view_obj drawableWidth];
        _sapp.height = (int) [_sapp_view_obj drawableHeight];
        if (_sapp.first_frame) {
            _sapp.first_frame = false;
            sokol_init();
        }
        sokol_frame();
    }
}
@end
#endif

@implementation _sapp_view
- (BOOL) isOpaque {
    return YES;
}
@end
#endif /* TARGET_OS_IPHONE */

#else /* __APPLE */
#error "sokol_app.h: Unknown OS"
#endif

/*== PUBLIC API FUNCTIONS ====================================================*/
bool sapp_isvalid() {
    return _sapp.valid;
}

int sapp_width() {
    return _sapp.width;
}

int sapp_height() {
    return _sapp.height;
}

bool sapp_gles2_fallback() {
    return _sapp.gles2_fallback;
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
        const void* obj =  (__bridge const void*) [_sapp_view_obj currentRenderPassDescriptor];
        SOKOL_ASSERT(obj);
        return obj;
    #else
        return 0;
    #endif
}

const void* sapp_metal_get_drawable() {
    SOKOL_ASSERT(_sapp.valid);
    #if defined(SOKOL_METAL)
        const void* obj = (__bridge const void*) [_sapp_view_obj currentDrawable];
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
