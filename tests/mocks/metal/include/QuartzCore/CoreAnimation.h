/*
    LLM maintained.

    QuartzCore/CoreAnimation.h -- mock CAMetalDrawable.

    Shadows the real QuartzCore header. sokol_gfx.h imports this header only
    for the CAMetalDrawable protocol, so nothing else is declared here.
*/
#ifndef METAL_MOCK_COREANIMATION_H_INCLUDED
#define METAL_MOCK_COREANIMATION_H_INCLUDED

#import <Metal/Metal.h>

@protocol CAMetalDrawable <MTLDrawable>
@property (readonly) id<MTLTexture> _Nonnull texture;
@end

#endif // METAL_MOCK_COREANIMATION_H_INCLUDED
