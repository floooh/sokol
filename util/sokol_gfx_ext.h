/*
sokol_gfx_ext.h - extensions for sokol_gfx
https://github.com/edubart/sokol_gp
*/

#if defined(SOKOL_IMPL) && !defined(SOKOL_GFX_EXT_IMPL)
#define SOKOL_GFX_EXT_IMPL
#endif

#ifndef SOKOL_GFX_EXT_INCLUDED
#define SOKOL_GFX_EXT_INCLUDED

#ifndef SOKOL_GFX_INCLUDED
#error "Please include sokol_gfx.h before sokol_gfx_ext.h"
#endif

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

SOKOL_GFX_API_DECL void sg_query_image_pixels(sg_image img_id, void* pixels, int size);
SOKOL_GFX_API_DECL void sg_query_pixels(int x, int y, int w, int h, bool origin_top_left, void *pixels, int size);

#ifdef __cplusplus
} // extern "C"
#endif

#endif // SOKOL_GFX_EXT_INCLUDED

#ifdef SOKOL_GFX_EXT_IMPL
#ifndef SOKOL_GFX_EXT_IMPL_INCLUDED
#define SOKOL_GFX_EXT_IMPL_INCLUDED

#ifndef SOKOL_GFX_IMPL_INCLUDED
#error "Please include sokol_gfx.h implementation before sokol_gfx_ext.h implementation"
#endif

#include <SDL.h>

#if defined(_SOKOL_ANY_GL)

static void _sg_gl_query_image_pixels(_sg_image_t* img, void* pixels) {
    SOKOL_ASSERT(img->gl.target == GL_TEXTURE_2D);
    SOKOL_ASSERT(0 != img->gl.tex[img->cmn.active_slot]);
    static GLuint newFbo = 0;
    GLuint oldFbo = 0;
    if(newFbo == 0) {
        glGenFramebuffers(1, &newFbo);
    }
    glGetIntegerv(GL_FRAMEBUFFER_BINDING, (GLint*)&oldFbo);
    glBindFramebuffer(GL_FRAMEBUFFER, newFbo);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, img->gl.tex[img->cmn.active_slot], 0);
    glReadPixels(0, 0, img->cmn.width, img->cmn.height, GL_RGBA, GL_UNSIGNED_BYTE, pixels);
    glBindFramebuffer(GL_FRAMEBUFFER, oldFbo);
    //glDeleteFramebuffers(1, &newFbo);
    _SG_GL_CHECK_ERROR();
}

static void _sg_gl_query_pixels(int x, int y, int w, int h, bool origin_top_left, void *pixels) {
    SOKOL_ASSERT(pixels);
    GLuint gl_fb;
    GLint dims[4];
    glGetIntegerv(GL_FRAMEBUFFER_BINDING, (GLint*)&gl_fb);
    _SG_GL_CHECK_ERROR();
    glGetIntegerv(GL_VIEWPORT, dims);
    int cur_height = dims[3];
    y = origin_top_left ? (cur_height - (y+h)) : y;
    _SG_GL_CHECK_ERROR();
#if defined(SOKOL_GLES2) // use NV extension instead
    glReadBufferNV(gl_fb == 0 ? GL_BACK : GL_COLOR_ATTACHMENT0);
#else
    glReadBuffer(gl_fb == 0 ? GL_BACK : GL_COLOR_ATTACHMENT0);
#endif
    _SG_GL_CHECK_ERROR();
    glReadPixels(x, y, w, h, GL_RGBA, GL_UNSIGNED_BYTE, pixels);
    _SG_GL_CHECK_ERROR();
}

#elif defined(SOKOL_D3D11)

static inline void _sgext_d3d11_Texture2D_GetDesc(ID3D11Texture2D* self, D3D11_TEXTURE2D_DESC* pDesc) {
    #if defined(__cplusplus)
        self->GetDesc(pDesc);
    #else
        self->lpVtbl->GetDesc(self, pDesc);
    #endif
}

static inline void _sgext_d3d11_SamplerState_GetDesc(ID3D11SamplerState* self, D3D11_SAMPLER_DESC* pDesc) {
    #if defined(__cplusplus)
        self->GetDesc(pDesc);
    #else
        self->lpVtbl->GetDesc(self, pDesc);
    #endif
}

static inline void _sgext_d3d11_CopySubresourceRegion(ID3D11DeviceContext* self, ID3D11Resource *pDstResource, UINT DstSubresource, UINT DstX, UINT DstY, UINT DstZ, ID3D11Resource *pSrcResource, UINT SrcSubresource, const D3D11_BOX *pSrcBox) {
    #if defined(__cplusplus)
        self->CopySubresourceRegion(pDstResource, DstSubresource, DstX, DstY, DstZ, pSrcResource, SrcSubresource, pSrcBox);
    #else
        self->lpVtbl->CopySubresourceRegion(self, pDstResource, DstSubresource, DstX, DstY, DstZ, pSrcResource, SrcSubresource, pSrcBox);
    #endif
}

static inline void _sgext_d3d11_OMGetRenderTargets(ID3D11DeviceContext* self, UINT NumViews, ID3D11RenderTargetView **ppRenderTargetViews, ID3D11DepthStencilView **ppDepthStencilView) {
    #if defined(__cplusplus)
        self->OMGetRenderTargets(NumViews, ppRenderTargetViews, ppDepthStencilView);
    #else
        self->lpVtbl->OMGetRenderTargets(self, NumViews, ppRenderTargetViews, ppDepthStencilView);
    #endif
}

static inline void _sgext_d3d11_RenderTargetView_GetResource(ID3D11RenderTargetView* self, ID3D11Resource** ppResource) {
    #if defined(__cplusplus)
        self->GetResource(ppResource);
    #else
        self->lpVtbl->GetResource(self, ppResource);
    #endif
}

static uint32_t _sg_d3d11_dxgi_format_to_sdl_pixel_format(DXGI_FORMAT dxgi_format) {
    switch(dxgi_format) {
        case DXGI_FORMAT_B8G8R8A8_UNORM:
            return SDL_PIXELFORMAT_ARGB8888;
        case DXGI_FORMAT_B8G8R8X8_UNORM:
            return SDL_PIXELFORMAT_RGB888;
        default:
            return SDL_PIXELFORMAT_UNKNOWN;
    }
}

static void _sg_d3d11_query_image_pixels(_sg_image_t* img, void* pixels) {
    SOKOL_ASSERT(_sg.d3d11.ctx);
    SOKOL_ASSERT(img->d3d11.tex2d);
    HRESULT hr;
    _SOKOL_UNUSED(hr);

    // create staging texture
    ID3D11Texture2D* staging_tex = NULL;
    D3D11_TEXTURE2D_DESC staging_desc = {};
    staging_desc.Width = (UINT)img->cmn.width;
    staging_desc.Height = (UINT)img->cmn.height;
    staging_desc.MipLevels = 1;
    staging_desc.ArraySize = 1;
    staging_desc.Format = img->d3d11.format;
    staging_desc.SampleDesc.Count = 1;
    staging_desc.SampleDesc.Quality = 0;
    staging_desc.Usage = D3D11_USAGE_STAGING;
    staging_desc.BindFlags = 0;
    staging_desc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
    staging_desc.MiscFlags = 0;
    hr = _sg_d3d11_CreateTexture2D(_sg.d3d11.dev, &staging_desc, NULL, &staging_tex);
    SOKOL_ASSERT(SUCCEEDED(hr));

    // copy pixels to staging texture
    _sgext_d3d11_CopySubresourceRegion(_sg.d3d11.ctx,
        (ID3D11Resource*)staging_tex,
        0, 0, 0, 0,
        (ID3D11Resource*)img->d3d11.tex2d,
        0, NULL);

    // map the staging texture's data to CPU-accessible memory
    D3D11_MAPPED_SUBRESOURCE msr = {};
    msr.pData = NULL;
    hr = _sg_d3d11_Map(_sg.d3d11.ctx, (ID3D11Resource*)staging_tex, 0, D3D11_MAP_READ, 0, &msr);
    SOKOL_ASSERT(SUCCEEDED(hr));

    // copy the data into the desired buffer, converting pixels to the desired format at the same time
    int res = SDL_ConvertPixels(
        img->cmn.width, img->cmn.height,
        _sg_d3d11_dxgi_format_to_sdl_pixel_format(staging_desc.Format),
        msr.pData, msr.RowPitch,
        SDL_PIXELFORMAT_RGBA32,
        pixels, img->cmn.width * 4);
    SOKOL_ASSERT(res == 0);
    _SOKOL_UNUSED(res);

    // unmap the texture
    _sg_d3d11_Unmap(_sg.d3d11.ctx, (ID3D11Resource*)staging_tex, 0);

    if(staging_tex) _sg_d3d11_Release(staging_tex);
}

static void _sg_d3d11_query_pixels(int x, int y, int w, int h, bool origin_top_left, void *pixels) {
    // get current render target
    ID3D11RenderTargetView* render_target_view = NULL;
    _sgext_d3d11_OMGetRenderTargets(_sg.d3d11.ctx, 1, &render_target_view, NULL);

    // fallback to window render target
    if(!render_target_view)
        render_target_view = (ID3D11RenderTargetView*)_sg.d3d11.rtv_cb();
    SOKOL_ASSERT(render_target_view);

    // get the back buffer texture
    ID3D11Texture2D *back_buffer = NULL;
    _sgext_d3d11_RenderTargetView_GetResource(render_target_view, (ID3D11Resource**)&back_buffer);
    SOKOL_ASSERT(back_buffer);

    // create a staging texture to copy the screen's data to
    D3D11_TEXTURE2D_DESC staging_desc;
    _sgext_d3d11_Texture2D_GetDesc(back_buffer, &staging_desc);
    staging_desc.Width = w;
    staging_desc.Height = h;
    staging_desc.BindFlags = 0;
    staging_desc.MiscFlags = 0;
    staging_desc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
    staging_desc.Usage = D3D11_USAGE_STAGING;
    ID3D11Texture2D *staging_tex = NULL;
    HRESULT hr = _sg_d3d11_CreateTexture2D(_sg.d3d11.dev, &staging_desc, NULL, &staging_tex);
    SOKOL_ASSERT(SUCCEEDED(hr));
    _SOKOL_UNUSED(hr);

    // copy the desired portion of the back buffer to the staging texture
    y = (origin_top_left ? y : (_sg.d3d11.cur_height - (y + h)));
    D3D11_BOX src_box = {};
    src_box.left = (UINT)x;
    src_box.top = (UINT)y;
    src_box.front = 0;
    src_box.right = (UINT)(x + w);
    src_box.bottom = (UINT)(y + w);
    src_box.back = 1;
    _sgext_d3d11_CopySubresourceRegion(_sg.d3d11.ctx,
        (ID3D11Resource*)staging_tex,
        0, 0, 0, 0,
        (ID3D11Resource*)back_buffer,
        0, &src_box);

    // map the staging texture's data to CPU-accessible memory
    D3D11_MAPPED_SUBRESOURCE msr = {};
    msr.pData = NULL;
    hr = _sg_d3d11_Map(_sg.d3d11.ctx, (ID3D11Resource*)staging_tex, 0, D3D11_MAP_READ, 0, &msr);
    SOKOL_ASSERT(SUCCEEDED(hr));

    // copy the data into the desired buffer, converting pixels to the desired format at the same time
    int res = SDL_ConvertPixels(
        w, h,
        _sg_d3d11_dxgi_format_to_sdl_pixel_format(staging_desc.Format),
        msr.pData, msr.RowPitch,
        SDL_PIXELFORMAT_RGBA32,
        pixels, w * 4);
    SOKOL_ASSERT(res == 0);
    _SOKOL_UNUSED(res);

    // unmap the texture
    _sg_d3d11_Unmap(_sg.d3d11.ctx, (ID3D11Resource*)staging_tex, 0);

    if(back_buffer) _sg_d3d11_Release(back_buffer);
    if(staging_tex) _sg_d3d11_Release(staging_tex);
}

#elif defined(SOKOL_METAL)

#import <Metal/Metal.h>
#import <QuartzCore/CAMetalLayer.h>

static uint32_t _sg_metal_texture_format_to_sdl_pixel_format(MTLPixelFormat texture_format) {
    switch(texture_format) {
        case MTLPixelFormatBGRA8Unorm:
            return SDL_PIXELFORMAT_ARGB8888;
        case MTLPixelFormatRGBA8Unorm:
            return SDL_PIXELFORMAT_ABGR8888;
        default:
            return SDL_PIXELFORMAT_UNKNOWN;
    }
}

static void _sg_metal_commit_command_buffer() {
    SOKOL_ASSERT(!_sg.mtl.in_pass);
    if(_sg.mtl.cmd_buffer) {
        #if defined(_SG_TARGET_MACOS)
        [_sg.mtl.uniform_buffers[_sg.mtl.cur_frame_rotate_index] didModifyRange:NSMakeRange(0, _sg.mtl.cur_ub_offset)];
        #endif
        [_sg.mtl.cmd_buffer commit];
        [_sg.mtl.cmd_buffer waitUntilCompleted];
        _sg.mtl.cmd_buffer = [_sg.mtl.cmd_queue commandBufferWithUnretainedReferences];
    }
}

static void _sg_metal_encode_texture_pixels(int x, int y, int w, int h, bool origin_top_left, id<MTLTexture> mtl_src_texture, void* pixels) {
    SOKOL_ASSERT(!_sg.mtl.in_pass);
    _sg_metal_commit_command_buffer();
    MTLTextureDescriptor* mtl_dst_texture_desc = [MTLTextureDescriptor texture2DDescriptorWithPixelFormat:mtl_src_texture.pixelFormat width:w height:h mipmapped:NO];
    mtl_dst_texture_desc.storageMode = MTLStorageModeManaged;
    mtl_dst_texture_desc.resourceOptions = MTLResourceStorageModeManaged;
    mtl_dst_texture_desc.usage = MTLTextureUsageShaderRead + MTLTextureUsageShaderWrite;
    id<MTLTexture> mtl_dst_texture = [mtl_src_texture.device newTextureWithDescriptor:mtl_dst_texture_desc];
    id<MTLCommandBuffer> cmd_buffer = [_sg.mtl.cmd_queue commandBuffer];
    id<MTLBlitCommandEncoder> blit_encoder = [cmd_buffer blitCommandEncoder];
    [blit_encoder copyFromTexture:mtl_src_texture
        sourceSlice:0
        sourceLevel:0
        sourceOrigin:MTLOriginMake(x,(origin_top_left ? y : (mtl_src_texture.height - (y + h))),0)
        sourceSize:MTLSizeMake(w,h,1)
        toTexture:mtl_dst_texture
        destinationSlice:0
        destinationLevel:0
        destinationOrigin:MTLOriginMake(0,0,0)
    ];
    [blit_encoder synchronizeTexture:mtl_dst_texture slice:0 level:0];
    [blit_encoder endEncoding];
    [cmd_buffer commit];
    [cmd_buffer waitUntilCompleted];

    MTLRegion mtl_region = MTLRegionMake2D(0, 0, w, h);
    void* temp_pixels = _sg_malloc(w * 4 * h);
    SOKOL_ASSERT(temp_pixels);
    [mtl_dst_texture getBytes:temp_pixels bytesPerRow:w * 4 fromRegion:mtl_region mipmapLevel:0];
    int res = SDL_ConvertPixels(w, h, _sg_metal_texture_format_to_sdl_pixel_format(mtl_dst_texture_desc.pixelFormat), temp_pixels, w * 4, SDL_PIXELFORMAT_RGBA32, pixels, w * 4);
    _sg_free(temp_pixels);
    SOKOL_ASSERT(res == 0);
    _SOKOL_UNUSED(res);
}

static void _sg_metal_query_image_pixels(_sg_image_t* img, void* pixels) {
    id<MTLTexture> mtl_src_texture = _sg.mtl.idpool.pool[img->mtl.tex[0]];
    _sg_metal_encode_texture_pixels(0, 0, (int)mtl_src_texture.width, (int)mtl_src_texture.height, true, mtl_src_texture, pixels);
}

static void _sg_metal_query_pixels(int x, int y, int w, int h, bool origin_top_left, void *pixels) {
    id<CAMetalDrawable> mtl_drawable = (__bridge id<CAMetalDrawable>)_sg.mtl.drawable_cb();
    _sg_metal_encode_texture_pixels(x, y, w, h, origin_top_left, mtl_drawable.texture, pixels);
}

#endif

void sg_query_image_pixels(sg_image img_id, void* pixels, int size) {
    SOKOL_ASSERT(pixels);
    SOKOL_ASSERT(img_id.id != SG_INVALID_ID);
    _sg_image_t* img = _sg_lookup_image(&_sg.pools, img_id.id);
    SOKOL_ASSERT(img);
    SOKOL_ASSERT(size >= (img->cmn.width * img->cmn.height * 4));
    _SOKOL_UNUSED(size);
#if defined(_SOKOL_ANY_GL)
    _sg_gl_query_image_pixels(img, pixels);
#elif defined(SOKOL_D3D11)
    _sg_d3d11_query_image_pixels(img, pixels);
#elif defined(SOKOL_METAL)
    _sg_metal_query_image_pixels(img, pixels);
#endif
}

void sg_query_pixels(int x, int y, int w, int h, bool origin_top_left, void *pixels, int size) {
    SOKOL_ASSERT(pixels);
    SOKOL_ASSERT(size >= w*h);
    _SOKOL_UNUSED(size);
#if defined(_SOKOL_ANY_GL)
    _sg_gl_query_pixels(x, y, w, h, origin_top_left, pixels);
#elif defined(SOKOL_D3D11)
    _sg_d3d11_query_pixels(x, y, w, h, origin_top_left, pixels);
#elif defined(SOKOL_METAL)
    _sg_metal_query_pixels(x, y, w, h, origin_top_left, pixels);
#endif
}

#endif // SOKOL_GFX_EXT_IMPL_INCLUDED
#endif // SOKOL_GFX_EXT_IMPL
