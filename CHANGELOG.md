## Updates

### 27-Feb-2024:

- Merged PR https://github.com/floooh/sokol/pull/1001, this is a small fix for GLES3 to avoid
  calling glInvalidateFramebuffer() on non-existing depth/stencil surfaces.

  Many thanks to @danielchasehooper!

#### 26-Feb-2024:

- Minor fix in sokol_imgui.h: The drawing code now detects and skips the special
  `ImDrawCallback_ResetRenderState` constant, not doing so would try to call a function
  at address (-8) which then results in a crash.

  See for what this is: https://github.com/ocornut/imgui/blob/277ae93c41314ba5f4c7444f37c4319cdf07e8cf/imgui.h#L2583-L2587

  sokol_imgui.h doesn't have any handling for this special callback, it will just ignore it.

  As a minor additional behaviour change, any user callback will now also cause `sg_reset_state_cache()`
  to be called. This is just a precaution in case the user callback code calls any native 3D backend API
  functions.

  Related issue: https://github.com/floooh/sokol/issues/1000

#### 21-Feb-2024:

- PR https://github.com/floooh/sokol/pull/993 has been merged, this allows to inject
  additional GL functions into the Win32 GL loader of sokol_gfx.h (TBH, it's a very specialized
  feature for people who know what they're doing, but it also fixes a very specific problem
  while at the same time resolving to 'nothing' when not used).

  Many thanks for @kcbanner for the PR!

#### 31-Jan-2024:

- sokol_app.h macOS: merged a workaround for the application window not being focused
  if the init callback takes a while (not reproducible on my M1 Mac with latest Sonoma,
  but might fix the issue for older Macs, and the change seems harmless enough -
  sokol_app.h essentially sends a focusEvent to itself)

  Related issue: https://github.com/floooh/sokol/issues/757
  Implemented in PR: https://github.com/floooh/sokol/pull/982

  Many thanks to @zoo-3d for investigating the issue and the PR!

#### 28-Jan-2024:

- sokol_app.h web: the canvas resize callback is now unregistered on cleanup.

  Related issue: https://github.com/floooh/sokol/issues/983 and PR: https://github.com/floooh/sokol/pull/984
  Many thanks to @edubart!

#### 27-Jan-2024

- sokol_app.h web: The HTML5 event bubbling changes introduced in the 02-Jan-2024
  update have been reverted because they introduced some undesired side effects.
  By default, most input events now *don't* bubble up (which restores the
  old behaviour), but it's now possible to enable bubbling for categories
  of input events (mouse, touch, wheel, keys and chars) during sokol-app setup.
  It's then possible to control bubbling of individual events by calling
  `sapp_consume_event()` from within the sokol-app event callback.

  See issue https://github.com/floooh/sokol/issues/972 for details and
  PR https://github.com/floooh/sokol/pull/975 for the actual changes.

  Also check out the new doc section `INPUT EVENT BUBBLING ON THE WEB PLATFORM`
  in the sokol_app.h header documentation block.

- sokol_gfx.h metal: Merged PR https://github.com/floooh/sokol/pull/980. When
  only the offset changes in a vertex buffer binding, only the buffer offset
  is now updated (e.g. instead of the Metal method `setVertexBuffer:offset:atIndex`,
  the leaner method `setVertexBufferOffset:atIndex` is called. Apart from the
  actual PR I also removed a couple of actually unused items from the Metal
  backend state cache. Many thanks to @staminajim for the PR!

  Related issue: https://github.com/floooh/sokol/issues/979

#### 23-Jan-2024

- sokol_app.h android: Touch event coordinates are now using AMotionEvent_getX/Y() instead
  of AMotionEvent_getRawX/Y(). The raw functions don't work well in multi-window
  scenarios. See PR https://github.com/floooh/sokol/pull/974 for details.
  Many thanks to Github user @Comanx!

#### 19-Jan-2024

- sokol_app.h wgpu: tiny fix for a breaking API change in webgpu.h in the Emscripten 3.1.52 SDK
- Merged PR https://github.com/floooh/sokol/pull/970 (many thanks to @waywardmonkeys) which
  fixes a couple of strict-prototype warnings (e.g. C functions using func() instead of func(void)).
  I also enabled `-Wstrict-prototypes` now in the CI tests for GCC and Clang, so such cases
  should be caught in the future.

#### 18-Jan-2024

- sokol_gfx.h: added support for the following pixel formats:
  - BC3_SRGBA
  - BC7_SRGBA
  - ETC2_SRGB8
  - ETC2_SRGB8A8
  - ASTC_4x4_RGBA
  - ASTC_4x4_SRGBA

  Related PR: https://github.com/floooh/sokol/pull/967

  Many thanks to GH user @allcreater!

#### 07-Jan-2024

- sokol_app.h (macos+metal): window content no longer 'wobbles' during window resizing. Many
  thanks to @Seb-degraff for picking up and investigating this longstanding issue
  (https://github.com/floooh/sokol/issues/700), finding a fix for the remaining problem
  and providing a really nice PR (https://github.com/floooh/sokol/pull/963)

#### 06-Jan-2024

> NOTE: if you use sokol_gfx.h and sokol_app.h together, make sure to update both. This is
because the pixel format enum in sokol_gfx.h has been shuffled around a bit, and as a result, some internal
pixel format constants in sokol_app.h had to move too!

- sokol_gfx.h: some minor new features (non-breaking):
  - the struct `sg_pixel_format` has two new items:
    - `bool compressed`: true if this is a hardware-compressed pixel format
    - `int bytes_per_pixel`: as the name says, with the caveat that this is
      zero for compressed pixel formats (because the smallest element in compressed formats is a block, not a pixel)
  - two previously private helper functions have been exposed to help with size computations
    for texture data, these may be useful when preparing image data for consumption by `sg_make_image()`
    and `sg_update_image()`:
      - `int sg_query_row_pitch(sg_pixel_format fmt, int width, int row_align_bytes)`:
        Computes the number of bytes in a texture row for a given pixel format. A 'row' has
        different meanings for uncompressed vs compressed formats: For uncompressed pixel
        formats, a row is a single line of pixels, while for compressed formats, a row is
        a line of 'compression blocks'. `width` is always in pixels.
      - `int sg_query_surface_pitch(sg_pixel_format fmt, int width, int height, int row_align_bytes)`:
        Computes number of bytes in a texture surface (e.g. a single mipmap) for a given
        pixel format. `width` and `height` are always in pixels.

    The `row_align_bytes` parameter is for added flexibility. For image data that goes into
    the `sg_make_image()` or `sg_update_image()` functions this should generally be 1, because these
    functions take tightly packed image data as input no matter what alignment restrictions
    exist in the backend 3D APIs.
- Related issue: https://github.com/floooh/sokol/issues/946, and PR: https://github.com/floooh/sokol/pull/962

#### 03-Jan-2024

- sokol_nuklear.h: `snk_handle_event()` now returns a bool to indicate whether the
  event was handled by Nuklear (this allows an application to skip its own event
  handling if Nuklear already handled the event). Issue link: https://github.com/floooh/sokol/issues/958,
  fixed in PR: https://github.com/floooh/sokol/pull/959. Many thanks to @adamrt for the PR!

#### 02-Jan-2024

Happy New Year! A couple of input-related changes in the sokol_app.h Emscripten backend:

- Mouse and touch events now bubble up to the HTML document instead of being consumed, in some scenarios this
  allows better integration with the surrounding web page. To prevent event bubbling,
  call `sapp_consume_event()` from within the sokol_app.h event callback function.
- **NOTE**: wheel/scroll events behave as before and are always consumed. This prevents
  an ugly "scroll bumping" effect when a wheel event bubbles up on a page where
  scrolling shouldn't be possible.
- The hidden HTML text input field hack for text input on mobile browsers has been
  removed. This idea never really worked across all browsers, and it actually
  interfered with Dear ImGui text input fields because the hidden HTML text field
  generated focus-in/out events which confused the Dear ImGui input handling code.

Those changes fix a couple of problem when trying to integrate sokol_app.h applications
into VSCode webview panels, see: https://marketplace.visualstudio.com/items?itemName=floooh.vscode-kcide

Related PR: https://github.com/floooh/sokol/pull/939

#### 10-Nov-2023

A small change in the sokol_gfx.h GL backend on Windows only:

PR https://github.com/floooh/sokol/pull/839 has been merged, in debug mode this creates
the GL context with WGL_CONTEXT_DEBUG_BIT_ARB. Thanks to @castano for the PR!

#### 06-Nov-2023

A bugfix in the sokol_gfx.h D3D11 backend, and some related cleanup when creating depth-stencil
render target images and resource views:

- fixed: render target images with format SG_PIXELFORMAT_DEPTH_STENCIL triggered a validation
  error because the pixel format capabilities code marked them as non-renderable. Now
  the SG_PIXELFORMAT_DEPTH_STENCIL pixel format is properly reported as renderable.
- the DXGIFormats for SG_PIXELFORMAT_DEPTH_STENCIL images are now as follows:
  - D3D11 texture object: DXGI_FORMAT_R24G8_TYPELESS
  - D3D11 shader-resource-view object: DXGI_FORMAT_R24_UNORM_X8_TYPELESS
  - D3D11 depth-stencil-view object: DXGI_FORMAT_D24_UNORM_S8_UINT

Related PR: https://github.com/floooh/sokol/pull/937

#### 30-Oct-2023

Some sokol_gfx.h backend-specific updates and tweaks (very minor chance that this is breaking if you are injecting textures into the D3D11 backend).

- a new set of public API functions to access the native backend 3D-API resource objects of
  sokol-gfx resource objects:

  ```
  sg_[api]_[type]_info sg_[api]_query_[type]_info(sg_[type])
  ```
  ...where `[api]` is any of `[gl, d3d11, mtl, wgpu]` and `[type]` is any of `[buffer, image, sampler, shader, pipeline, pass]`.

  This is mainly useful when mixing native 3D-API code with sokol-gfx code.

  See issue https://github.com/floooh/sokol/issues/931 for details.

- WebGPU backend: `sg_make_image()` will no longer automatically create a WebGPU texture-view object when injecting a WebGPU texture object, instead
this must now be explicitly provided.

- D3D11 backend: `sg_make_image()` will no longer automatically create a
shader-resource-view object when injecting a D3D11 texture object, and
vice versa, a texture object will no longer be looked up from an injected
shader-resource-view object (e.g. the injection rules are now more straightforward and explicit). See issue https://github.com/floooh/sokol/issues/930 for details.

For the detailed changes, see PR https://github.com/floooh/sokol/pull/932.

#### 27-Oct-2023

Fix broken render-to-mipmap in the sokol_gfx.h GL backend.

There was a subtle bug / "feature gap" lurking in sokol_gfx.h GL backend: trying
to render to any mipmap except the top-level mipmap resulted in a black screen
because of an incomplete-framebuffer error. This is fixed now. The changes in detail:

- creating a texture in the GL backend now sets the GL_TEXTURE_MAX_LEVEL property
  (this is the fix to make everything work)
- the framebuffer completeness check in the GL backend now has more detailed error logging
- in the validation layer, the requirement that a sampler that's used with a
  single-mipmap-texture must use `.mipmap_filter = SG_FILTER_NONE` has been
  relaxed (a later update will remove SG_FILTER_NONE entirely since it's not needed anymore
  and the concept of a "none" mipmap filter only exists in GL and Metal, but not D3D, WebGPU
  and Vulkan)

Ticket: https://github.com/floooh/sokol/issues/923

PR: https://github.com/floooh/sokol/pull/924

There's also a new render-to-mipmap sample which covers to close this 'feature gap':

https://floooh.github.io/sokol-html5/miprender-sapp.html

A couple of similar samples will follow over the next few days
(rendering to texture array layers and 3d texture slices).

#### 26-Oct-2023

- sokol_app.h gl: fix a regression introduced in https://github.com/floooh/sokol/pull/916
  which could select the wrong framebuffer pixel format and break rendering
  on some GL drivers (in my case: an older Intel GPU).

  If you are using the GL backend on Windows, please make sure to upgrade!

#### 23-Oct-2023

- sokol_app.h gl: some further startup optimizations in the WGL code path
  via PR https://github.com/floooh/sokol/pull/916

#### 21-Oct-2023

The major topic of this update is the 'finalized' WebGPU support in sokol_gfx.h and sokol_app.h.

- WebGPU samples are hosted here:

  https://floooh.github.io/sokol-webgpu/

- WebGL2 samples remain hosted here:

  https://floooh.github.io/sokol-html5/

- Please read the following blog post as introduction:

  https://floooh.github.io/2023/10/16/sokol-webgpu.html

- ...and the changelog and updated documentation in the sokol-shdc repository:

  https://github.com/floooh/sokol-tools

- You'll also need to update the sokol-shdc binaries:

  https://github.com/floooh/sokol-tools-bin

- Please also read the following new or updated sections in the embedded   sokol_gfx.h header documentation:

  - `ON SHADER CREATION`
  - `ON SG_IMAGESAMPLETYPE_UNFILTERABLE_FLOAT AND SG_SAMPLERTYPE_NONFILTERING`
  - `WEBGPU CAVEATS`

  Please do this especially when using any of the following texture pixel formats, as you will most likely encounter new validation layer errors:

  - `SG_PIXELFORMAT_R32F`
  - `SG_PIXELFORMAT_RG32F`
  - `SG_PIXELFORMAT_RGBA32F`

- There is a tiny breaking change in the sokol_gfx.h API (only requires action when not using sokol-shdc):

  - the following `sg_sampler_type` enum items have been renamed to better match their WebGPU counterparts:
    - SG_SAMPLERTYPE_SAMPLE => SG_SAMPLERTYPE_FILTERING
    - SG_SAMPLERTYPE_COMPARE => SG_SAMPLERTYPE_COMPARISON

  - the enum `sg_image_sample_type` gained a new item:
    - SG_IMAGESAMPLETYPE_UNFILTERABLE_FLOAT

  - the enum `sg_sampler_type` gained a new item:
    - SG_SAMPLERTYPE_NONFILTERING

- The sokol_gfx.h struct `sg_desc` has two new items:
  - `.wgpu_bindgroups_cache_size` - must be power-of-2, default: 1024
  - `.wgpu_disable_bindgroups_cache` - default: false

- sokol_gfx.h gained the following new public API functions to query per-frame information:
  - `sg_frame_stats sg_query_frame_stats()`
  - `void sg_enable_frame_stats(void)`
  - `void sg_disable_frame_stats(void)`
  - `bool sg_frame_stats_enabled(void)`

  Frame statistics gathering is enabled after startup, but can be temporarily
  disabled and enabled again via `sg_disable_frame_stats()` and `sg_enable_frame_stats`.

- The sokol_gfx.h validation layer has new validation checks in `sg_make_shader()`
  regarding image/sampler pair compatibility (WebGPU is particularly strict about
  this stuff).

- In sokol_app.h, the old wip WebGPU device and swapchain setup code is now implemented
  in pure C code (previously this was a mix of Javascript and C).

- Also note that sokol_app.h currently only supports WebGPU in the Emscripten backend.
  If you want to use sokol_gfx.h with the WebGPU backend in a native scenario, you'll have
  to use a different window system glue library (like GLFW). The sokol-samples directory
  has a handful of examples for using sokol_gfx.h + Dawn + GLFW.

- The following headers have been made compatible with the sokol_gfx.h WebGPU backend
  (mainly by embedding WGSL shader code):
  - sokol_debugtext.h
  - sokol_fontstash.h
  - sokol_gl.h
  - sokol_spine.h
  - sokol_imgui.h (also required some more changes for embedding `unfilterable-float`
    textures, since these now require separate shader and pipeline objects)
  - sokol_nuklear.h (works in WebGPU, but doesn't contain the work from sokol_imgui.h
    to support `unfilterable-float` user textures)

- sokol_gfx_imgui.h gained a new function `sg_imgui_draw_menu()` which renders a
  menu panel to show/hide all debug windows. Previously this had to be done
  outside the header.

- sokol_gfx_imgui.h gained a new 'frame stats' window, which allows to peak into
  sokol_gfx.h frame-rendering internals. This basically visualizes the struct
  `sg_frame_stats` returned by the new sokol_gfx.h function `sg_query_frame_stats()`.

- The sokol-samples repository gained 3 new samples:
  - cubemap-jpeg-sapp.c (load a cubemap from separate JPEG files)
  - cubemaprt-sapp.c (render into cubemap faces - this demo actually existed a while but wasn't "official" so far)
  - drawcallperf-sapp.c (a sample to explore the performance overhead of sg_apply_bindings, sg_apply_uniforms and sg_draw)

#### 03-Oct-2023

- sokol_app.h win/gl: PR https://github.com/floooh/sokol/pull/886 has been merged, this makes
  GL context initialization on Windows slightly more efficient. Many thanks to @dtrebilco!

#### 25-Sep-2023

- The allocator callback functions in all headers that support custom allocators have been renamed
  from `alloc` and `free` to `alloc_fn` and `free_fn`, this is because the symbol `free` is quite
  likely to collide with a preprocessor macro of the same name if the standard C allocator is
  replaced with a custom allocator.

  This is a breaking change only if you've been providing your own allocator functions to
  the sokol headers.

  See issue https://github.com/floooh/sokol/issues/903 and PR https://github.com/floooh/sokol/pull/908
  for details.

#### 23-Sep-2023

- sokol_gfx.h gl: Allow to inject an external GL framebuffer id into the sokol-gfx default
  pass. See PR https://github.com/floooh/sokol/pull/899 and issue https://github.com/floooh/sokol/issues/892
  for details. Many thanks to @danielchasehooper for the discussion and PR!

  Further down the road I want to make the whole topic more flexible while at the same time
  simplifying the sokol-gfx API, see here: https://github.com/floooh/sokol/issues/904

#### 22-Sep-2023

- sokol_gfx.h: Fixed a Metal validation error on Intel Macs when creating textures (Intel Macs
  have unified memory, but don't support textures in shared storage mode). This was a regression
  in the image/sampler split update in mid-July 2023. Fixes issue https://github.com/floooh/sokol/issues/905
  via PR https://github.com/floooh/sokol/pull/907.

#### 19-Sep-2023

- sokol_fetch.h: fixed a minor issue where a request that was cancelled before it was dispatched
  had an incomplete response state set in the response callback (the `finished`, `failed` and
  `error_code` fields were not set). This fixes issue https://github.com/floooh/sokol/issues/882
  via PR https://github.com/floooh/sokol/pull/898

#### 18-Sep-2023

- PR https://github.com/floooh/sokol/pull/893 has been merged, this fixes a minor issue
  in the GL backend when using an injected texture as framebuffer attachment.
- Issue https://github.com/floooh/sokol/issues/884 has been fixed via PR https://github.com/floooh/sokol/pull/894,
  this adds missing error code paths in the Metal backend when Metal object creation fails.
- Clarified `sapp_run()` behaviour in the sokol_app.h documentation header (search for `OPTIONAL: DON'T HIJACK main()`)
- sokol_args.h now fully supports "key-only args", see issue https://github.com/floooh/sokol/issues/876 for details,
  fixed via PR https://github.com/floooh/sokol/pull/896

#### 17-Sep-2023

- The sokol-gfx Metal backend now adds debug labels to Metal resource objects and
  also passes through the `sg_push/pop_debug_group()` calls. If you use the push/pop
  debug group calls, please be aware of the following limitations:

  - a push inside a render pass must have an associated pop inside the same render pass
  - a push outside any render pass must have an associated pop outside any render pass
  - Metal will ignore any push/pop calls outside render passes (this is because in Metal
    these are MTLCommandEncoder methods)

  Associated issue: https://github.com/floooh/sokol/issues/889, and PR: https://github.com/floooh/sokol/pull/890.

#### 09-Sep-2023

- a small PR has been merged which fixes a redundant glBindFramebuffer() in the GLES3 backend
  in `sg_end_pass()` (see: https://github.com/floooh/sokol/pull/878), many thanks to @danielchasehooper
  for catching that issue!
- sokol_imgui.h has been fixed for cimgui 1.89.9 (see https://github.com/floooh/sokol/issues/879)

#### 28-Aug-2023

**sokol_gfx.h metal**: A new attempt at fixing a rare Metal validation layer
error about MTKView swapchain resource lifetimes. See PR https://github.com/floooh/sokol/pull/873
for details.

#### 26-Jul-2023

**sokol_nuklear.h**: The same image+sampler support has been added as in sokol_imgui.h
three days ago:

- a new object type `snk_image_t` which wraps a sokol-gfx image and sampler
  under a common handle
- new functions:
  - snk_make_image()
  - snk_destroy_image()
  - snk_query_image_desc()
  - snk_image_from_nkhandle()
- the function snk_nkhandle() now takes an snk_image_t handle instead of an sg_image handle
- the nuklear.h header needs to be included before the declaration (not just the implementation),
  this was already required before, but now you get a proper error message if the include is missing
- the 'standard' logging- and error-reporting callback has been added as in the other sokol headers
  (don't forget to add a logging callback in snk_setup(), otherwise sokol-nuklear will be silent)
- since sokol-nuklear now needs to allocate memory, an allocator can now be provided to the
  snk_setup() call (otherwise malloc/free will be used)

Please also read the new documentation section `ON USER-PROVIDED IMAGES AND SAMPLERS`
in sokol_nuklear.h, and also check out the (rewritten) sample:

https://floooh.github.io/sokol-html5/nuklear-images-sapp.html

Associated PR: https://github.com/floooh/sokol/pull/862

#### 23-Jul-2023

**sokol_imgui.h**: Add proper support for injecting user-provided sokol-gfx
images and samplers into Dear ImGui UIs. With the introduction of separate
sampler objects in sokol_gfx.h there's a temporary feature regression in
sokol_imgui.h and sokol_nuklear.h in that user provided images had to use a
shared sampler that's hardwired into the respective headers. This update fixes
this problem for sokol_imgui.h, with a similar fix for sokol_nuklear.h coming
up next.

The sokol_imgui.h changes in detail are:

- a new object type `simgui_image_t` which wraps a sokol-gfx image and sampler
  object under a common handle
- two new function `simgui_make_image()` and `simgui_destroy_image()` to
  create and destroy such a new `simgui_image_t` object.
- the existing function `simgui_imtextureid()` has been changed to take
  an `simgui_image_t`
- sokol_imgui.h now also uses the same error-handling and logging callback
  as the other sokol headers (this was needed because creating an `simgui_image_t`
  object may fail because the object pool is exhausted) - don't forget
  to provide a logging callback (for instance via sokol_log.h), otherwise
  sokol_imgui.h will be entirely silent in case of errors.

Please also read the new documentation section `ON USER-PROVIDED IMAGES AND SAMPLERS`
in sokol_imgui.h, and also check out the new sample:

https://floooh.github.io/sokol-html5/imgui-images-sapp.html

Associated PR: https://github.com/floooh/sokol/pull/861

#### 16-Jul-2023

**BREAKING CHANGES**

The main topic of this update is to separate sampler state from image state in
sokol_gfx.h which became possible after GLES2 support had been removed from
sokol_gfx.h.

This also causes some 'collateral changes' in shader authoring and
other sokol headers, but there was opportunity to fill a few feature gaps
in sokol_gfx.h as well:

- it's now possible to sample depth textures in shaders both with regular
  samplers, and with 'comparison samplers' (which is mainly useful for shadow mapping)
- it's now possible to create render passes without color attachments for
  'depth-only' rendering

See the new [shadows-depthtex-sapp](https://floooh.github.io/sokol-html5/shadows-depthtex-sapp.html) sample which demonstrates both features.

> NOTE: all related projects have a git tag `pre-separate-samplers` in case you are not ready yet to make the switch

> NOTE 2: if you use sokol-gfx with the sokol-shdc shader compiler, you'll also need
> to update the sokol-shdc binaries from https://github.com/floooh/sokol-tools-bin

##### **sokol_gfx.h**

- texture sampler state has been removed from `sg_image_desc`, instead you now
  need to create separate sampler objects:

    ```c
    sg_sampler smp = sg_make_sampler(&(sg_sampler_desc){
        .min_filter = SG_FILTER_LINEAR,
        .mag_filter = SG_FILTER_LINEAR,
        .wrap_u = SG_WRAP_CLAMP_TO_EDGE,
        .wrap_v = SG_WRAP_CLAMP_TO_EDGE
    });
    ```

- texture filtering is now described by 3 separate filters:
    - min_filter = SG_FILTER_NEAREST | SG_FILTER_LINEAR
    - mag_filter = SG_FILTER_NEAREST | SG_FILTER_LINEAR
    - mipmap_filter = SG_FILTER_NONE | SG_FILTER_NEAREST | SG_FILTER_LINEAR

  ...this basically switches from the esoteric GL convention to a convention
  that's used by all other 3D APIs. There's still a limitation that's caused by
  GL though: a sampler which is going to be used with an image that has a
  `mipmap_count = 1` requires that `.mipmap_filter = SG_FILTER_NONE`.

- another new sampler state in `sg_sampler_desc` is `sg_compare_func compare;`,
  this allows to create 'comparison samplers' for shadow mapping

- when calling `sg_apply_bindings()` the struct `sg_bindings` now has changed
  to also include sampler objects, note that there is no 1:1 relationship
  between images and samplers required:

    ```c
    sg_apply_bindings(&(sg_bindings){
        .vertex_buffers[0] = vbuf,
        .fs = {
            .images = {
                [SLOT_tex0] = img0,
                [SLOT_tex1] = img1,
                [SLOT_tex2] = img2,
            },
            .samplers[SLOT_smp] = smp,
        }
    });
    ```

- if you use sokol-shdc, you need to rewrite your shaders from 'OpenGL GLSL style' (with
  combined image samplers) to 'Vulkan GLSL style' (with separate textures and samplers):

  E.g. the old GL-style shader with combined image samplers:

  ```glsl
  uniform sampler2D tex;

  void main() {
      frag_color = texture(tex, uv);
  }
  ```
  ...now needs to look like this:

  ```glsl
  uniform texture2D tex;
  uniform sampler smp;

  void main() {
      frag_color = texture(sampler2D(tex, smp), uv);
  }
  ```

  sokol-shdc will now throw an error if it encounters an 'old' shader using combined
  image-samplers, this helps you to catch all places where a rewrite to separate
  texture and sampler objects is required.

- If you *don't* use sokol-shdc and instead provide your own backend-specific
  shaders, you need to provide more shader interface reflection info about the texture
  and sampler usage in a shader when calling `sg_make_shader`.
  Please see the new documentation block `ON SHADER CREATION` in sokol_gfx.h for more details!

  Also refer to the updated 3D-backend-specific samples here:

  - for GL: https://github.com/floooh/sokol-samples/tree/master/glfw
  - for GLES3: https://github.com/floooh/sokol-samples/tree/master/html5
  - for D3D11: https://github.com/floooh/sokol-samples/tree/master/d3d11
  - for Metal: https://github.com/floooh/sokol-samples/tree/master/metal

- it's now possible to create `sg_pass` objects without color attachments to
  enable depth-only rendering, see the new sample [shadows-depthtex-sapp](https://floooh.github.io/sokol-html5/shadows-depthtex-sapp.html) for details,
  specifically be aware of the caveat that a depth-only-compatible `sg_pipeline` object
  needs to 'deactivate' the first color target by setting its pixel format
  to `NONE`:

  ```c
  sg_pipeline pip = sg_make_pipeline(&(sg_pipeline_desc){
      ...
      .colors[0].pixel_format = SG_PIXELFORMAT_NONE,
      ...
  });
  ```

- the following struct names have been changed to be more in line with related
  struct names, this also makes those names similar to WebGPU types:

    - `sg_buffer_layout_desc` => `sg_vertex_buffer_layout_state`
    - `sg_vertex_attr_desc` => `sg_vertex_attr_state`
    - `sg_layout_desc` => `sg_vertex_layout_state`
    - `sg_color_state` => `sg_color_target_state`

- bugfixes and under-the-hood changes
    - `sg_begin_pass()` used the wrong framebuffer size when rendering to a mip-level != 0
    - the Metal backend code started to use the `if (@available(...))` statement
      to check for runtime-availability of macOS/iOS API features
    - **NOTE:** this change (`if (@available(...))`) caused linking problems in
      the Zig and Rust bindings on GH Actions (missing symbol
      `___isPlatformVersionAtLeast`) which I could not reproduce locally on my
      M1 Mac. On Zig this could be fixed by moving to the latest zig-0.11.0-dev
      version, but for Rust this still needs to be fixed).
    - on macOS the Metal backend now creates resources in Shared resource storage mode if
      supported by the device
    - on iOS the Metal backend now supports clamp-to-border-color if possible (depends on
      iOS version and GPU family)

##### **sokol_gl.h**

- The function `sgl_texture(sg_image img)` has been changed to accept a sampler
  object to `sgl_texture(sg_image img, sg_sampler smp)`. Passing an invalid image handle
  will use the builtin default (white) texture, and passing an invalid sampler
  handle will use the builtin default sampler.

##### **sokol_shape.h**

- Some sokol-shape functions have been renamed to match renamed structs in sokol-gfx:

    - `sshape_buffer_layout_desc()` => `sshape_vertex_buffer_layout_state()`
    - `sshape_position_attr_desc()` => `sshape_position_vertex_attr_state()`
    - `sshape_normal_attr_desc()` => `sshape_normal_vertex_attr_state()`
    - `sshape_texcoord_attr_desc()` => `sshape_texcoord_vertex_attr_state()`
    - `sshape_color_attr_desc()` => `sshape_color_vertex_attr_state()`

##### **sokol_spine.h**

- A sokol-spine atlas object now allocates both an `sg_image` and `sg_sampler` handle
  and expects the user code to initialize those handles to complete image and
  sampler objects. Check the updated sokol-spine samples here for more details:

  https://github.com/floooh/sokol-samples/tree/master/sapp

##### **sokol_imgui.h**

- sokol_imgui.h has a new public function to create an ImTextureID handle from
  an `sg_image` handle which can be used like this:

    ```c
    ImTextureID tex_id = simgui_imtextureid(img);
    ```

  Note that sokol-imgui currently doesn't currently allow to pass user-provided `sg_sampler`
  object with the user-provided image.

##### **sokol_nuklear.h**

- similar to sokol_imgui.h, there's a new public function `snk_nkhandle()`
  which creates a Nuklear handle from a sokol-gfx image handle which can be
  used like this to create a Nuklear image handle:

    ```c
    nk_image nki = nk_image_handle(snk_nkhandle(img));
    ```

  As with sokol_imgui.h, it's currently not possible to pass a user-provided `sg_sampler`
  object with the image.


#### 20-May-2023

Some minor event-related cleanup in sokol_app.h and a touchscreen fix in sokol_imgui.h

- in the event `SAPP_EVENTTYPE_FILESDROPPED`:
    - the `sapp_event.modifier` field now contains the active modifier keys
      at the time of the file drop operations on the platforms macOS, Emscripten
      and Win32 (on Linux I haven't figured out how this might work with the
      Xlib API)
    - on macOS, the `sapp_event.mouse_x/y` fields now contain the window-relative
      mouse position where the drop happened (this already worked as expected on
      the other desktop platforms)
    - on macOS and Linux, the `sapp_event.mouse_dx/dy` fields are now set to zero
      (this already was the case on Emscripten and Win32)
- in the events `SAPP_EVENTTYPE_MOUSE_ENTER` and `SAPP_EVENTTYPE_MOUSE_LEAVE`:
    - the `sapp_event.mouse_dx/dy` fields are now set to zero, previously this
      could be a very big value on some desktop platforms

Many thanks to @castano for the initial PR (https://github.com/floooh/sokol/pull/830)!

- In sokol_imgui.h, the new io.AddMouseSourceEvent() function in Dear ImGui 1.89.5
  is called to differentiate between mouse- and touch-events, this makes ui tabs
  work with a single tap (previously a double-tap on the tab was needed). The code
  won't break if the ImGui version is older (in this case the function simply isn't called)


#### 19-May-2023

**BREAKING CHANGES**_ in sokol_gfx.h: Render passes are now more 'harmonized'
with Metal and WebGPU by exposing a 'store action', and making MSAA resolve attachments
explicit. The changes in detail:

  - A new documentation section `ON RENDER PASSES` has been added to sokol_gfx.h, this
    gives a much more detailed overview of the new render pass behaviour than this
    changelog, please make sure to give it a read - especially when you are using
    MSAA offscreen render passes in your code.
  - `sg_action` has been renamed to `sg_load_action`.
  - A new enum `sg_store_action` has been added.
  - In `sg_pass_action`:
    - `.action` has been renamed to `.load_action`.
    - `.value` has been renamed to `.clear_value`.
    - A new field `.store_action` has been added.
  - An `sg_image` object with a sample count > 1 no longer contains a second implicit
    texture for the msaa-resolve operation.
  - When creating a pass object, there's now an array of `sg_image` objects
    called `resolve_attachments[]`. When a resolve attachment image is set, the
    color attachment at the same slot index must be an image with a sample count >
    1, and an 'msaa-resolve' operation from the color attachment into the
    resolve attachment will take place in `sg_end_pass()`.
  - Pass attachments are now more flexible (there were a couple of gaps where specific
    image types were not allowed as pass attachments, especially for the depth-stencil-
    attachment - but this hadn't actually been checked by the validation layer).
  - Some gaps in the validation layer around images and passes have been tightened up,
    those usually don't work in one backend or another, but have been ignored so far
    in the validation layer, mainly:
    - MSAA images must have num_mipmaps = 1.
    - 3D images cannot have a sample_count > 1.
    - 3D images cannot have depth or depth-stencil image formats.
    - It's not allowed to bind MSAA images as texture.
    - It's not allowed to bind depth or depth-stencil images as texture.
    - (I'll see if I can relax some of those restrictions after the WebGPU backend release)
  - **A lot** of new tests have been added to cover validation layer checks when creating
    image and pass objects.

  Next up: WebGPU!

#### 30-Apr-2023

GLES2/WebGL1 support has been removed from the sokol headers (now that
  all browsers support WebGL2, and WebGPU is around the corner I feel like it's finally
  time to ditch GLES2.

  This is a breaking API change in sokol_gfx.h and sokol_app.h.

  Common changes across all headers:
  - (breaking change) the `SOKOL_GLES2` config define is no longer accepted and will cause a compile error
    (use `SOKOL_GLES3` instead)
  - (breaking change) on Emscripten use the linker option `-s USE_WEBGL2=1`
  - any embedded GLES shaders have been updated from glsl100 to glsl300es (but glsl100 shaders
    still work fine with the GLES3 backend)

  Changes in sokol_gfx.h:
  - (breaking change) the following `sg_features` members have been removed (because those features
    are no longer optional, but guaranteed across all backends):
      - `sg_features.instancing`
      - `sg_features.multiple_render_targets`
      - `sg_features.msaa_render_targets`
      - `sg_features.imagetype_3d`
      - `sg_features.imagetype_array`
  - (breaking change) the struct `sg_gl_context_desc` and its embedded instance `sg_desc.gl` have been removed
  - `sg_image` objects with `SG_PIXELFORMAT_DEPTH` or `SG_PIXELFORMAT_DEPTH_STENCIL` with
    a `sample_count == 1` are now regular textures in the GL backend (this is not true
    for MSAA depth textures unfortunately, those are still GL render buffer objects)
  - in the GL backend, `SG_PIXELFORMAT_DEPTH` now resolves to `GL_DEPTH_COMPONENT32F` (same
    as in the other backends), previously it was `GL_DEPTH_COMPONENT16`
  - in `sg_begin_pass()`, the GL backend now only uses the new `glClearBuffer*` functions, the
    old GLES2 clear functions have been removed
  - in `sg_end_pass()`, the GLES3 backend now invalidates MSAA render buffers after they have
    been resolved (via `glInvalidateFramebuffer`) - more control over this will come soon-ish
    when this ticket is implemented: https://github.com/floooh/sokol/issues/816
  - the instanced rendering functions are no longer wrapped in C macros in the GL backend

  Changes in sokol_app.h:
  - (breaking) the config item `sapp_desc.gl_force_gles2` has been removed
  - (breaking) the function `sapp_gles2()` has been removed
  - any fallback logic from GLES3 to GLES2 has been removed (in the Emscripten, Android and
    iOS backends)

- **20-Feb-2023**: sokol_gfx.h has a new set of functions to get a 'best-effort'
  desc struct with the creation parameters of a specific resource object:

    ```c
    sg_buffer_desc sg_query_buffer_desc(sg_buffer buf);
    sg_image_desc sg_query_image_desc(sg_image img);
    sg_shader_desc sg_query_shader_desc(sg_shader shd);
    sg_pipeline_desc sg_query_pipeline_desc(sg_pipeline pip);
    sg_pass_desc sg_query_pass_desc(sg_pass pass);
    ```

  The returned structs will *not* be an exact copy of the desc struct that
  was used for creation the resource object, instead:

    - references to external data (like buffer and image content or
      shader sources) will be zeroed
    - any attributes that have not been kept around internally after
      creation will be zeroed (the ```sg_shader_desc``` struct is most
      affected by this, the other structs are fairly complete).

  Calling the functions with an invalid or dangling resource handle
  will return a completely zeroed struct (thus it may make sense
  to first check the resource state via ```sg_query_*_state()```)

  Nevertheless, those functions may be useful to get a partially filled out
  'creation blueprint' for creating similar resources without the need
  to keep and pass around the original desc structs.

  >MINOR BREAKING CHANGE: the struct members ```sg_image_info.width``` and
  ```sg_image_info.height``` have been removed, this information is now
  returned by ```sg_query_image_desc()```.

  PR: https://github.com/floooh/sokol/pull/796, fixes: https://github.com/floooh/sokol/issues/568

- **17-Feb-2023**: sokol_app.h on macOS now has a proper fix for the problem
  that macOS doesn't send key-up events while the Cmd key is held down.
  Previously this was handled through a workaround of immediately sending a
  key-up event after its key-down event if the Cmd key is currently held down
  to prevent a 'stuck key'. The proper fix is now to install an "event monitor"
  callback (many thanks to GLFW for finding and implementing the solution).
  Unfortunately there's no such solution for the Emscripten code path, which
  also don't send a key-up event while Cmd is pressed on macOS (the workaround
  there to send a key-up event right on key-down while Cmd is held down to
  prevent a stuck key is still in place) For more details, see:
  https://github.com/floooh/sokol/issues/794

- **15-Feb-2023**: A fix in the sokol_gfx.h GL backend: due to a bug in the
  state cache, the GL backend could only bind a total of
  SG_MAX_SHADERSTAGE_IMAGES (= 12) when it actually should be twice that amount
  (12 per shader stage). Note however that the total amount of texture bindings
  is still internally limited by the GL_MAX_COMBINED_TEXTURE_IMAGE_UNITS
  runtime variable (~~currently this is not exposed in sg_limits though~~). Many
  thanks to @allcreater for PR https://github.com/floooh/sokol/pull/787.
  PS: sg_limits now exposes GL_MAX_COMBINED_TEXTURE_IMAGE_UNITS as
  ```sg_limits.gl_max_combined_texture_image_units```, and the
  value can also be inspected via the debug UI in sokol_gfx_imgui.h.

- **13-Feb-2023**: The way logging works has been completely revamped in
  the sokol headers. UWP support has been removed from sokol_audio.h
  and sokol_app.h (this also means that the sokol headers no longer contain
  any C++ code).

  **REQUIRED ACTION**: Since the sokol headers are now completely silent
  without a logging callback (explanation below), it is highly recommended
  to use the standard logging callback provided by the new header ```sokol_log.h```.
  For instance for sokol_gfx.h it looks like this:

    ```c
    #include "sokol_log.h"
    //...
        sg_setup(&(sg_desc){
            //...
            .logger.func = slog_func,
        });
    ```

  All sokol samples have been updated to use sokol_log.h for logging.

  The former logging callback is now a combined
  logging- and error-reporting callback, and more information is available
  to the logging function:
    - a 'tag string' which identifies the sokol headers, this string
      is identical with the API prefix (e.g. "sg" for sokol_gfx.h,
      "sapp" for sokol_app.h etc...)
    - a numeric log level: 0=panic, 1=error, 2=warning, 3=info
    - a numeric 'log item id' (think of it as error code, but since
      not only errors are reported I called it a log item id)
    - a human readable error message
    - a source file line number where the log item was reported
    - the file path of the sokol header

  Log level ```panic``` is special in that it terminates execution inside
  the log function. When a sokol header issues a panic log message, it means
  that the problem is so big that execution can not continue. By default,
  the sokol headers and the standard log function in sokol_log.h call
  ```abort()``` when a panic log message is issued.

  In debug mode (NDEBUG not defined, or SOKOL_DEBUG defined), a log message
  (in this case from sokol_spine.h) will look like this:

  ```
  [sspine][error][id:12] /Users/floh/projects/sokol/util/sokol_spine.h:3472:0:
      SKELETON_DESC_NO_ATLAS: no atlas object provided in sspine_skeleton_desc.atlas
  ```
  The information can be 'parsed' like this:
    - ```[sspine]```: it's a message from sokol_spine.h
    - ```[error]```: it's an error
    - ```[id:12]```: the numeric log item id (associated with ```SKELETON_DESC_NO_ATLAS``` below)
    - source file path and line number in a compiler-specific format - in some IDEs and terminals
      this is a clickable link
    - the line below is the human readable log item id and message

  In release mode (NDEBUG is defined and SOKOL_DEBUG is not defined), log messages
  are drastically reduced (the reason is to not bloat the executable with all the extra string data):

  ```
  [sspine][error][id:12][line:3472]
  ```
  ...this reduced information still gives all the necessary information to identify the location and type of error.

  A custom logging function must adhere to a few rules:

    - must be re-entrant because it might be called from different threads
    - must treat **all** provided string pointers as optional (can be null)
    - don't store the string pointers, copy the string data instead
    - must not return for log level panic

  A new header ```sokol_log.h``` has been added to provide a standard logging callback implementation
  which provides logging output on all platforms to stderr and/or platform specific logging
  facilities. ```sokol_log.h``` only uses fputs() and platform specific logging function instead
  of fprintf() to preserve some executable size.

  **QUESTION**: Why are the sokol headers now silent, unless a logging callback is installed?
  This is mainly because a standard logging function which does something meaningful on all
  platforms (including Windows and Android) isn't trivial. E.g. printing to stderr is not
  enough. It's better to move that stuff into a centralized place in a separate header,
  but since the core sokol headers must not (statically) depend on other sokol headers
  the only solution that made sense was to provide a standard logging function which must
  be 'registered' as a callback.

- **26-Jan-2023**: Work on SRGB support in sokol_gfx.h has started, but
  this requires more effort to be really usable. For now, only a new
  pixel format has been added: SG_PIXELFORMAT_SRGB8A8 (see https://github.com/floooh/sokol/pull/758,
  many thanks to @allcreater). The sokol-gfx GL backend has a temporary
  workaround to align behaviour with D3D11 and Metal: automatic SRGB conversion
  is enabled for offscreen render passes, but disabled for the default
  framebuffer. A proper fix will require separate work on sokol_app.h to
  support an SRGB default framebuffer and communicate to sokol-gfx
  whether the default framebuffer is SRGB enabled or not.

- **24-Jan-2023**: sokol_gfx.h Metal: A minor inconsistency has been fixed in
  the validation layer and an assert for the function ```sg_apply_uniforms()```
  which checks the size of the incoming data against the uniform block size.
  The validation layer and Metal backend did a ```<=``` test while the D3D11
  and GL backends checked for an exact size match. Both the validation layer
  and the Metal backend now also check for an exact match. Thanks to @nmr8acme
  for noticing the issue and providing a PR! (https://github.com/floooh/sokol/pull/776)

- **23-Jan-2023**: A couple more sokol_audio.h updates:
  - an AAudio backend has been added for Android, and made the default. This
    means you now need to link with ```aaudio``` instead of ```OpenSLES``` when
    using sokol_audio.h on Android. The OpenSLES backend code still exists (for
    now), but must be explicitly selected by compiling the sokol_audio.h
    implementation with the define ```SAUDIO_ANDROID_SLES``` (e.g. there is
    no runtime fallback from AAudio to OpenSLES). AAudio is fully supported
    since Android 8.1. Many thanks to @oviano for the initial AAudio PR
    (https://github.com/floooh/sokol/pull/484)
  - in the WebAudio backend, WebAudio is now properly activated on the first
    input action again on Chrome for Android (at some point activating WebAudio
    via a ```touchstart``` event stopped working and had to be moved to the
    ```touchend``` event, see https://github.com/floooh/sokol/issues/701)
  - audio backend initialization on iOS and macOS is now a bit more fault-tolerant,
    errors during initialization now properly set sokol_audio.h to 'silent mode'
    instead of asserting (or in release mode ignoring the error)
  - ...and some minor general code cleanup things in sokol_audio.h: backend-specific
    functions now generally have a matching prefix (like ```_saudio_alsa_...()```)
    for better searchability

- **16-Jan-2023**:
  - sokol_audio.h android: https://github.com/floooh/sokol/pull/747 has been merged
    which adds a couple more error checks at OpenSLES startup.
  - sokol_gfx.h: support for half-float vertex formats has been added via
    PR https://github.com/floooh/sokol/pull/745
  - sokol_imgui.h: fixes for Dear ImGui 1.89 deprecations (via PR https://github.com/floooh/sokol/pull/761)

- **15-Jan-2023**: two bugfixes in sokol_app.h and sokol_gfx.h:
  - sokol_app.h x11: Mouse button events now always return valid mouse
    coordinates, also when no mouse movement happened yet
    (fixes https://github.com/floooh/sokol/issues/770)
  - sokol_gfx.h gl: The GL context is now configured with
    GL_UNPACK_ALIGNMENT = 1, this should bring texture creation and updating
    behaviour in line with the other backends for tightly packed texture
    data that doesn't have a row-pitch with a multiple of 4
    (fixes https://github.com/floooh/sokol/issues/767)

- **14-Jan-2023**: sokol_app.h x11: a drag'n'drop related bugfix, the
  XdndFinished reply event was sent with the wrong window handle which
  confused some apps where the drag operation originated
  (see https://github.com/floooh/sokol/pull/765#issuecomment-1382750611)

- **16-Dec-2022**: In the sokol_gfx.h Metal backend: A fix for a Metal
  validation layer error which I just discovered yesterday (seems to be new in
  macOS 13). When the validation layer is active, and the application window
  becomes fully obscured, the validation layer throws an error after a short
  time (for details see: https://github.com/floooh/sokol/issues/762).
  The reason appears to be that sokol_gfx.h creates a command buffer with
  'unretained references' (e.g. the command buffer doesn't manage the
  lifetime of resources used by the commands stored in the buffer). This
  seems to clash with MTKView's and/or CAMetalLayer's expectations. I fixed
  this now by creating a second command buffer with 'retained references',
  which only holds the ```presentDrawable``` command. That way, regular
  draw commands don't have the refcounting overhead (because they're stored
  in an unretained-references cmdbuffer), while the drawable surface is
  still properly lifetime managed (because it's used in a separate command
  buffer with retained references).

- **15-Dec-2022**: A small but important update in sokol_imgui.h which fixes
  touch input handling on mobile devices. Many thanks to github user @Xadiant
  for the bug investigation and [PR](https://github.com/floooh/sokol/pull/760).

- **25-Nov-2022**: Some code cleanup around resource creation and destruction in sokol_gfx.h:
    - It's now safe to call the destroy, uninit and dealloc functions in any
      resource state, in general, the functions will do the right thing without
      assertions getting in the way (there are however new log warnings in some
      cases though, such as attempting to call an ```sg_dealloc_*()``` function on
      a resource object that's not in ALLOC state)
    - A related **minor breaking change**: the ```sg_uninit_*()``` functions now return
      void instead of bool, this is because ```sg_dealloc_*()``` no longer asserts
      when called in the wrong resource state
    - Related internal code cleanup in the backend-agnostic resource creation
      and cleanup code, better or more consistent function names, etc...
    - The validation layer can now be disabled in debug mode with a runtime
      flag during setup: ```sg_desc.disable_validation```. This is mainly useful
      for test code.
    - Creating a pass object with invalid image objects now no longer asserts,
      but instead results in a pass object in FAILED state. In debug mode,
      the validation layer will still stop at this problem though (it's mostly
      an 'undefined API behaviour' fix in release mode).
    - Calling ```sg_shutdown()``` with existing resources in ALLOC state will
      no longer print a log message about an 'active context mismatch'.
    - A new header documentation blurb about the two-step resource creation
      and destruction functions (search for RESOURCE CREATION AND DESTRUCTION IN DETAIL)

- **16-Nov-2022**: Render layer support has been added to sokol_debugtext.h,
  same general changes as in sokol_gl.h with two new functions:
  sdtx_layer(layer_id) to select the layer to record text into, and
  sdtx_draw_layer(layer_id) to draw the recorded text in that layer inside a
  sokol-gfx render pass. The new sample [debugtext-layers-sapp](https://floooh.github.io/sokol-html5/debugtext-layers-sapp) demonstrates the feature together with
  sokol-gl.


- **11-Nov-2022**: sokol_gl.h has 2 new public API functions which enable
  layered rendering: sgl_layer(), sgl_draw_layer() (technically it's three
  functions: there's also sgl_context_draw_layer(), but that's just a variant of
  sgl_draw_layer()). This allows to 'interleave' sokol-gl rendering
  with other render operations. The [spine-layers-sapp](https://floooh.github.io/sokol-html5/spine-layers-sapp.html)
  sample has been updated to use multiple sokol-gl layers.

- **09-Nov-2022**: sokol_gfx.h now allows to add 'commit listeners', these
  are callback functions which are called from inside sg_commit(). This is
  mainly useful for libraries which build on top of sokol-gfx to be notified
  about the start/end point of a frame, which in turn may simplify the public
  API, or the internal implementation, because the library no longer needs to
  'guess' when a new frame starts.

  For more details, search for 'COMMIT LISTENERS' in the sokol_gfx.h header.

  This also results in a minor breaking change in sokol_spine.h: The function
  ```sspine_new_frame()``` has been removed and replaced with an internal commit
  listener.

  Likewise, sokol_gl.h now uses a commit listener in the implementation, but
  without changing the public API (the feature will be important for an upcoming
  sokol-gl feature to support rendering layers, and for this a 'new-frame-function'
  would have been needed).

- **05-Nov-2022** A breaking change in sokol_fetch.h, and a minor change in
  sokol_app.h which should only break for very few users:
  - An ```sfetch_range_t``` ptr/size pair struct has been added to sokol_fetch.h,
    and discrete ptr/size pairs have been replaced with sfetch_range_t
    items. This affects the structs ```sfetch_request_t``` and ```sfetch_response_t```,
    and the function ```sfetch_bind_buffer()```.
  - The required changes in ```sfetch_response_t``` might be a bit non-obviois: To
    access the fetched data, previous ```.buffer_ptr``` and ```.fetched_size```
    was used. The fetched data is now accessible through an ```sfetch_range_t data```
    item (```data.ptr``` and ```data.size```). The old ```.fetched_offset``` item
    has been renamed to ```.data_offset``` to better conform with the new naming.
  - The last two occurrences of discrete ptr/size pairs in sokol_app.h now have also
    been replaced with ```sapp_range_t``` items, this only affects the structs
    ```sapp_html5_fetch_request``` and ```sapp_html5_fetch_response```.

- **03-Nov-2022** The language bindings generation has been updated for Zig 0.10.0,
  and clang-14 (there was a minor change in the JSON ast-dump format).
  Many thanks to github user @kcbanner for the Zig PR!

- **02-Nov-2022** A new header sokol_spine.h (in the util dir), this is a
  renderer and 'handle wrapper' around the spine-c runtime (Spine is a popular 2D
  character anim system: http://esotericsoftware.com/). This turned out a much bigger
  rabbit-hole than I initially expected, but the effort is justified by being a
  experimentation testbed for a couple of things I want to add to other sokol
  headers (for instance cleaned up handle pool code, a new logging- and error-reporting
  system, render layers which will be useful for sokol_gl.h and sokol_debugtext.h).

- **22-Oct-2022** All sokol headers now allow to override logging with a
  callback function (installed in the setup call) instead of defining a SOKOL_LOG
  macro. Overriding SOKOL_LOG still works as default fallback, but this is no
  longer documented, consider this deprecated. Many thanks to github user
  @Manuzor for the PR (see https://github.com/floooh/sokol/pull/721 for details)

- **21-Oct-2022** RGB9E5 pixel format support in sokol_gfx.h and a GLES2 related
  bugfix in the sokol_app.h Android backend:
  - sokol_gfx.h now supports RGB9E5 textures (3*9 bit RGB + 5 bit shared exponent),
    this works in all backends except GLES2 and WebGL1 (use ```sg_query_pixelformat()```
    to check for runtime support). Many thanks to github user @allcreater for the PR!
  - a bugfix in the sokol_app.h Android backend: when forcing a GLES2 context via
    sapp_desc.gl_force_gles2, the Android backend correctly created a GLES2 context,
    but then didn't communicate this through the function ```sapp_gles2()``` (which
    still returned false in this case). This caused the sokol_gfx.h GL backend to
    use the GLES3 code path instead GLES2 (which surprisingly seemed to have worked
    fine, at least for the sokol samples which force GLES2).

- **19-Oct-2022** Some fixes in the embedded Javascript code blocks (via EM_JS)
  in sokol_app.h, sokol_args.h, sokol_audio.h and sokol_fetch.h:
  - the JS code has been 'modernized' (e.g. const and let instead of var,
    ```() => { ... }``` instead of ```function () { ... }``` for callbacks)
  - false positives in the Closure static analysis have been suppressed
    via inline hints

- **16-Oct-2022** The Odin bindings generator and the generated bindings have
  been simplified (the Odin binding now don't have separate wrapper functions).
  Requires the latest Odin release. Also note: On M1 Macs I'm currently seeing
  what looks like an ABI problem (in functions which pass color values to the C
  side as uint8_t, the colors come out wrong). This also happened with the
  previous binding version, so it looks like a regression in Odin. Might be
  related to this recent bugfix (which I haven't tested yet):
  https://github.com/odin-lang/Odin/issues/2121 Many thanks to @thePHTest for the
  PR! (https://github.com/floooh/sokol/pull/719)

- **15-Oct-2022**
    - fixes for Emscripten 3.1.24: the sokol headers now use the new
    **EM_JS_DEPS()** macro to declare 'indirect dependencies on JS library functions'.
    This is a (much more robust) follow-up fix to the Emscripten related fixes from 10-Sep-2022.
    The new Emscripten SDK also displays a couple of Javascript "static analyzer" warnings
    by the Closure compiler (used in release mode to optimize and minify the generated
    JS code). I fixed a couple of those warnings, but some warnings persist (all of them
    false positives). Not sure yet if these can be fixed or need to be suppressed, but
    that's for another time.
    - the webkitAudioContext() fallback in sokol_audio.h's Emscripten backend
    has been removed (only AudioContext is supported now), the fallback also
    triggered a Closure warning, so it probably never worked as intended anyway.
    - I also had to undo an older workaround in sokol_app.h on iOS (https://github.com/floooh/sokol/issues/645)
    because this is now triggering a Metal validation layer error (https://github.com/floooh/sokol/issues/726).
    The original case is no longer reproducible, so undoing the old workaround seems to
    be a quick fix. Eventually I want to get rid of MTKView though, and go down to
    CAMetalLayer.

- **08-Oct-2022** sokol_app.h Android backend: the ```sapp_touchpoint``` struct
  now has a new item ```sapp_android_tooltype android_tooltype;```. This exposes the
  result of the Android NDK function ```AMotionEvent_getToolType()```.
  Many thanks to @Wertzui123 for the initial PR (https://github.com/floooh/sokol/pull/717).

- **25-Sep-2022**: sokol_app.h on Linux now optionally supports EGL instead of
  GLX for the window system glue code and can create a GLES2 or GLES3 context
  instead of a 'desktop GL' context.
  To get EGL+GLES2/GLES3, just define SOKOL_GLES2 or SOKOL_GLES3 to compile the
  implementation. To get EGL+GL, define SOKOL_GLCORE33 *and* SOKOL_FORCE_EGL.
  By default, defining just SOKOL_GLCORE33 uses GLX for the window system glue
  (just as before). Many thanks to GH user @billzez for the PR!

- **10-Sep-2022**: sokol_app.h and sokol_args.h has been fixed for Emscripten 3.21, those headers
  used the Emscripten Javascript helper function ```ccall()``` which is now part of the
  'legacy runtime' and causes linker errors. Instead of ```ccall()``` sokol_app.h and sokol_args.h
  now drop down to a lower level set of Emscripten JS helper functions (which hopefully won't
  go away anytime soon).

- **05-Aug-2022**: New officially supported and automatically updated language bindings for Odin:
  https://github.com/floooh/sokol-odin (also see [gen_odin.py](https://github.com/floooh/sokol/blob/master/bindgen/gen_odin.py))

- **10-Jul-2022**: New features in sokol_app.h and sokol_imgui.h:
    - In sokol_app.h it's now possible to set a mouse cursor type from a number of predefined
      types via the new function ```sapp_set_mouse_cursor(sapp_mouse_cursor cursor)```. The
      available cursor types are compatible with GLFW and Dear ImGui. Supported platforms
      are: macOS, linux, win32, uwp and web.
    - ```sapp_show_mouse(bool shown)``` now also works on the web platform.
    - In sokol_app.h, the poorly defined 'user cursor' feature has been removed (```sapp_desc.user_cursor```
      and ```SAPP_EVENTTYPE_UPDATE_CURSOR```). This was a hack to allow changing the mouse cursor and
      only worked on Win32 and macOS (with different behaviour). Since setting the cursor type
      is now 'properly supported, this hack was removed.
    - sokol_imgui.h will now set the cursor type via ```sapp_set_mouse_cursor()```. This can be
      disabled with the new ```simgui_desc_t``` item ```disable_set_mouse_cursor```.
    - sokol_imgui.h now automatically enables resizing windows from edges (not just the bottom-right corner),
      this behaviour can be disabled with the new ```simgui_desc_t``` item ```disable_windows_resize_from_edges```.
    - sokol_imgui.h can now optionally write to the alpha channel (useful if you want to render the UI
      into a separate render target, which is later composed onto the default framebuffer). The feature
      is enabled with the new ```simgui_desc_t``` item ```write_alpha_channel```.

  Many thanks to **@tomc1998** for the initial [Linux/X11 mouse cursor type PR](https://github.com/floooh/sokol/pull/678) and
  **@luigi-rosso** for the [sokol_imgui.h alpha channel PR](https://github.com/floooh/sokol/pull/687)!

- **03-Jul-2022**: A new sokol_gfx.h function ```bool sg_query_buffer_will_overflow(sg_buffer buf, size_t size)```
which allows to check if a call to ```sg_append_buffer()``` would overflow the buffer. This
is an alternative to the ```sg_query_buffer_overflow()``` function which only reports
the overflow after the fact. Many thanks to @RandyGaul for the PR!

- **29-Jun-2022**: In sokol_app.h with the D3D11 backend, if SOKOL_DEBUG is
defined, and the D3D11 device creation fails, there's now a fallback code
path which tries to create the device again without the D3D11_CREATE_DEVICE_DEBUG
flag. Turns out the D3D11 debug support may suddenly stop working (just happened
to me, indicated by the Win10 "Graphics Tool" feature being silently uninstalled
and failing to install when asked to do so). This fix at least allows sokol_app.h
applications compiled in debug mode to run, even if the D3D11 debug layer doesn't
work.

- **29-May-2022**: The code generation scripts for the
[sokol-nim](https://github.com/floooh/sokol-nim) language bindings have been
revised and updated, many thanks to Gustav Olsson for the PR! (I'm planning to
spend a few more days integrating the bindings generation with Github Actions,
so that it's easier to publish new bindings after updates to the sokol headers).

- **26-May-2022**: The GL backend in sokol_app.h now allows to override the GL
  context version via two new items in the ```sapp_desc``` struct:
  ```sapp_desc.gl_major_version``` and ```sapp_desc.gl_minor_version```. The
  default GL context version remains at 3.2. Overriding the GL version might make
  sense if you're not using sokol_app.h together with sokol_gfx.h, or otherwise
  want to call GL functions directly. Note that this only works for the
  'desktop GL' backends (Windows, Linux and macOS), but not for the GLES backends
  (Android, iOS, web). Furthermore, on macOS only the GL versions 3.2 and 4.1
  are available (plus the special config major=1 minor=0 creates an
  NSOpenGLProfileVersionLegacy context). In general: use at your risk :) Many
  thanks to Github user @pplux for the PR!

- **15-May-2022**: The way internal memory allocation can be overridden with
  your own functions has been changed from global macros to callbacks
  provided in the API setup call. For instance in sokol_gfx.h:

  ```c
  void* my_malloc(size_t size, void* userdata) {
    (void)userdata; // unused
    return malloc(size);
  }

  void my_free(void* ptr, void* userdata) {
    (void)userdata; // unused
    free(ptr);
  }

  //...
    sg_setup(&(sg_desc){
      //...
      .allocator = {
        .alloc = my_malloc,
        .free = my_free,
        .user_data = ...,
      }
    });
  ```

  sokol_gfx.h will now call ```my_malloc()``` and ```my_free()``` whenever it needs
  to allocate or free memory (note however that allocations inside OS
  functions or 3rd party libraries are not affected).

  If no override functions are provided, the standard library functions ```malloc()``` and ```free()```
  will be used, just as before.

  This change breaks source compatibility in the following headers:

    - **sokol_fontstash.h**: the function signature of ```sfons_create()``` has changed,
      this now takes a pointer to a new ```sfons_desc_t``` struct instead of
      individual parameters.
    - **sokol_gfx_imgui.h** (NOT sokol_imgui.h!): likewise, the function signature of
      ```sg_imgui_init()``` has changed, this now takes an additional parameter
      which is a pointer to a new ```sg_imgui_desc_t``` struct.

  All affected headers also have a preprocessor check for the outdated
  macros ```SOKOL_MALLOC```, ```SOKOL_CALLOC``` and ```SOKOL_FREE``` and throw
  a compilation error if those macros are detected.

  (if configuration through macros is still desired this could be added back in
  the future, but I figured that the new way is more flexible in most situations).

  The header sokol_memtrack.h and the sample [restart-sapp](https://floooh.github.io/sokol-html5/restart-sapp.html) have been updated accordingly.

  Also search for ```MEMORY ALLOCATION OVERRIDE``` in the header documentation block
  for more details.

- **14-May-2022**: added a helper function ```simgui_map_keycode()``` to
  sokol_imgui.h to map sokol_app.h keycodes (```sapp_keycode```,
  ```SAPP_KEYCODE_*```) to Dear ImGui keycodes (```ImGuiKey```, ```ImGuiKey_*```).
  If you're using Dear ImGui function to check for key input, you'll need to
  update the code like this:

  - Old:
    ```cpp
    ImGui::IsKeyPressed(SAPP_KEYCODE_A);
    ```
  - New:
    ```cpp
    ImGui::IsKeyPressed(simgui_map_keycode(SAPP_KEYCODE_A));
    ```

  This was basically 'fallout' from rewriting the input system in sokol_imgui.h
  to the new evented IO system in Dear ImGui.

- **08-Feb-2022**: sokol_imgui.h has been updated for Dear ImGui 1.87:
  - sokol_imgui.h's input code has been rewritten to use the new evented IO
    system and extended virtual key codes in Dear ImGui
  - on non-Emscripten platforms, mouse buttons are no longer "cancelled" when
    the mouse leaves the window (since the native desktop platforms
    automatically capture the mouse when mouse buttons are pressed, but mouse
    capture is not supported in the sokol_app.h Emscripten backend)

- **28-Jan-2022**: some window size behaviour changes in sokol_app.h.
  - Asking for a default-sized window (via sapp_desc.width/height = 0) now
    behaves a bit differently on desktop platforms. Previously this set the
    window size to 640x480, now a default window covers more screen area:
      - on Windows CW_USEDEFAULT will be used for the size
      - on macOS and Linux, the window size will be 4/5 of the
        display size
      - no behaviour changes on other platforms
  - On Windows and Linux, the window is now centered (in a later update,
    more control over the initial window position, and new functions for
    positioning and sizing might be provided)
  - On Windows, when toggling between windowed and fullscreen, the
    window position and size will now be restored (on other platforms
    this already happened automatically through the window system)
  - On all desktop platforms if an application starts in fullscreen and
    then is toggled back to windowed, the window will now be of the
    expected size (provided in sapp_desc.width/height)

- **20-Jan-2022**:
  - sokol_audio.h: A compatibility fix in the sokol_audio.h WASAPI backend (Windows): On
    some configs the IAudioClient::Initialize() call could fail because
    of a mismatch between the requested number of channels and speaker config.
    See [#614](https://github.com/floooh/sokol/issues/614) for details.
  - sokol_app.h D3D11/DXGI: Fix an (uncritical) COM interface leak warning for IDXGIAdapter and
    IDXGIFactory at shutdown, introduced with the recent disabling of Alt-Enter.

- **18-Jan-2022**:
  - sokol_app.h now has per-monitor DPI support on Windows and macOS: when
    the application window is moved to a monitor with different DPI, the values
    returned by sapp_dpi_scale(), sapp_width() and sapp_height() will update
    accordingly (only if the application requested high-dpi rendering with
    ```sapp_desc.high_dpi=true```, otherwise the dpi scale value remains
    fixed at 1.0f). The application will receive an SAPP_EVENTTYPE_RESIZED event
    if the default framebuffer size has changed because of a DPI change.
    On Windows this feature requires Win10 version 1703 or later (aka the
    'Creators Update'), older Windows version simply behave as before.
    Many thank to @tjachmann for the initial PR with the Windows implementation!
  - sokol_app.h: DPI scale computation on macOS is now more robust using the
    NSScreen.backingScaleFactor value
  - sokol_app.h: the new frame timing code in sokol_app.h now detects if the display
    refresh rate changes and adjusts itself accordingly (for instance if the
    window is moved between displays with different refresh rate)
  - sokol_app.h D3D11/DXGI: during window movement and resize, the frame is now
    presented with DXGI_PRESENT_DO_NOT_WAIT, this fixes some window system
    stuttering issues on Win10 configs with recent NVIDIA drivers.
  - sokol_app.h D3D11/DXGI: the application will no longer appear to freeze for
    0.5 seconds when the title bar is grabbed with the mouse for movement, but
    then not moving the mouse.
  - sokol_app.h D3D11/DXGI: DXGI's automatic windowed/fullscreen switching via
    Alt-Enter has been disabled, because this switched to 'real' fullscreen mode,
    while sokol_app.h's fullscreen mode uses a borderless window. Use the
    programmatic fullscreen/window switching via ```sapp_toggle_fullscreen()```
    instead.
  - **BREAKING CHANGE** in sokol_imgui.h: because the applications' DPI scale
    can now change at any time, the DPI scale value is now communicated to
    sokol_imgui.h in the ```simgui_new_frame()``` function. This has been
    changed to accept a pointer to a new ```simgui_frame_desc_t``` struct.
    With C99, change the simgui_new_frame() call as follows (if also using
    sokol_app.h):
    ```c
    simgui_new_frame(&(simgui_frame_desc_t){
        .width = sapp_width(),
        .height = sapp_height(),
        .delta_time = sapp_frame_duration(),
        .dpi_scale = sapp_dpi_scale()
    });
    ```
    On C++ this works:
    ```c++
    simgui_new_frame({ sapp_width(), sapp_height(), sapp_frame_duration(), sapp_dpi_scale() });
    ```
    ...or in C++20:
    ```c++
    simgui_new_frame({
        .width = sapp_width(),
        .height = sapp_height(),
        .delta_time = sapp_frame_duration(),
        .dpi_scale = sapp_dpi_scale()
    });
    ```
  - **KNOWN ISSUE**: the recent change in sokol-audio's WASAPI backend to directly consume
    float samples doesn't appear to work on some configs (see [#614](https://github.com/floooh/sokol/issues/614)),
    investigation is underway

- **15-Jan-2022**:
  - A bugfix in the GL backend for uniform arrays using the 'native' uniform block layout.
    The bug was a regression in the recent 'uniform data handling' update. See
    [PR #611](https://github.com/floooh/sokol/pull/611) for details, and this [new sample/test](https://github.com/floooh/sokol-samples/blob/master/glfw/uniformarrays-glfw.c).
    Many thanks to @nmr8acme for the PR!

- **08-Jan-2022**: some enhancements and cleanup to uniform data handling in sokol_gfx.h
  and the sokol-shdc shader compiler:
    - *IMPORTANT*: when updating sokol_gfx.h (and you're using the sokol-shdc shader compiler),
      don't forget to update the sokol-shdc binaries too!
    - The GLSL uniform types int, ivec2, ivec3 and
      ivec4 can now be used in shader code, those are exposed to the GL
      backends with the new ```sg_uniform_type``` items
      ```SG_UNIFORM_TYPE_INT[2,3,4]```.
    - A new enum ```sg_uniform_layout```, currently with the values SG_UNIFORMLAYOUT_NATIVE
      and SG_UNIFORMLAYOUT_STD140. The enum is used in ```sg_shader_uniform_block_desc```
      as a 'packing rule hint', so that the GL backend can properly locate the offset
      of uniform block members. The default (SG_UNIFORMLAYOUT_NATIVE) keeps the same
      behaviour, so existing code shouldn't need to be changed. With the packing
      rule SG_UNIFORMLAYOUT_STD140 the uniform block interior is expected to be
      laid out according to the OpenGL std140 packing rule.
    - Note that the SG_UNIFORMLAYOUT_STD140 only allows a subset of the actual std140
      packing rule: arrays are only allowed for the types vec4, int4 and mat4.
      This is because the uniform data must still be compatible with
      ```glUniform()``` calls in the GL backends (which have different
      'interior alignment' for arrays).
    - The sokol-shdc compiler supports the new uniform types and will annotate the
      code-generated sg_shader_desc structs with SG_UNIFORMLAYOUT_STD140,
      and there are new errors to make sure that uniform blocks are compatible
      with all sokol_gfx.h backends.
    - Likewise, sokol_gfx.h has tighter validation for the ```sg_shader_uniform_block```
      desc struct, but only when the GL backend is used (in general, the interior
      layout of uniform blocks is only relevant for GL backends, on all other backends
      sokol_gfx.h just passes the uniform data as an opaque block to the shader)
  For more details see:
    - [new sections in the sokol_gfx.h documentation](https://github.com/floooh/sokol/blob/ba64add0b67cac16fc86fb6b64d1da5f67e80c0f/sokol_gfx.h#L343-L450)
    - [documentation of ```sg_uniform_layout```](https://github.com/floooh/sokol/blob/ba64add0b67cac16fc86fb6b64d1da5f67e80c0f/sokol_gfx.h#L1322-L1355)
    - [enhanced sokol-shdc documentation](https://github.com/floooh/sokol-tools/blob/master/docs/sokol-shdc.md#glsl-uniform-blocks-and-c-structs)
    - [a new sample 'uniformtypes-sapp'](https://floooh.github.io/sokol-html5/uniformtypes-sapp.html)

  PS: and an unrelated change: the frame latency on Win32+D3D11 has been slightly improved
  via IDXGIDevice1::SetMaximumFrameLatency()

- **27-Dec-2021**: sokol_app.h frame timing improvements:
  - A new function ```double sapp_frame_duration(void)``` which returns the frame
    duration in seconds, averaged over the last 256 frames to smooth out
    jittering spikes. If available, this uses platform/backend specific
    functions of the swapchain API:
      - On Windows: DXGI's GetFrameStatistics().SyncQPCTime.
      - On Emscripten: the timestamp provided by the RAF callback, this will
        still be clamped and jittered on some browsers, but averaged over
        a number of frames yields a pretty accurate approximation
        of the actual frame duration.
      - On Metal, ```MTLDrawable addPresentedHandler + presentedTime```
        doesn't appear to function correctly on macOS Monterey and/or M1 Macs, so
        instead mach_absolute_time() is called at the start of the MTKView
        frame callback.
      - In all other situations, the same timing method is used as
        in sokol_time.h.
  - On macOS and iOS, sokol_app.h now queries the maximum display refresh rate
    of the main display and uses this as base to compute the preferred frame
    rate (by multiplying with ```sapp_desc.swap_interval```), previously the
    preferred frame rate was hardwired to ```60 * swap_interval```. This means
    that native macOS and iOS applications may now run at 120Hz instead of
    60Hz depending on the device (I realize that this isn't ideal, there
    will probably be a different way to hint the preferred interval at
    which the frame callback is called, which would also support disabling
    vsync and probably also adaptive vsync).

- **19-Dec-2021**: some sokol_audio.h changes:
  - on Windows, sokol_audio.h no longer converts audio samples
    from float to int16_t, but instead configures WASAPI to directly accept
    float samples. Many thanks to github user iOrange for the PR!
  - sokol_audio.h has a new public function ```saudio_suspended()``` which
    returns true if the audio device/context is currently in suspended mode.
    On all backends except WebAudio this always returns false. This allows
    to show a visual hint to the user that audio is muted until the first
    input event is received.

- **18-Dec-2021**: the sokol_gfx.h ```sg_draw()``` function now uses the currently applied
  pipeline object to decide if the GL or D3D11 backend's instanced drawing function
  should be called instead of the ```num_instances``` argument. This fixes a
  bug on some WebGL configs when instanced rendering is configured
  but ```sg_draw()``` is called with an instance count of 1.

- **18-Nov-2021**: sokol_gl.h has a new function to control the point size for
  point list rendering: ```void sgl_point_size(float size)```. Note that on D3D11
  the point size is currently ignored (since D3D11 doesn't support a point size at
  all, the feature will need to be emulated in sokol_gl.h when the D3D11 backend is active).
  Also note that points cannot currently be textured, only colored.

- **08-Oct-2021**: texture compression support in sokol_gfx.h has been revisited:
    - tighter validation checks on texture creation:
        - content data validation now also happens in ```sg_make_image()``` (previously only in ```sg_update_image()```)
        - validate that compressed textures are immutable
        - separate "no data" validation checks for immutable vs dynamic/stream textures
        - provided data size for creating or updating textures must match the expected surface sizes exactly
    - fix PVRTC row and surface pitch computation according to the GL PVRTC extension spec
    - better adhere to Metal documentation for the ```MTLTexture.replaceRegion``` parameters (when bytesPerImage is expected to be zero or not)

- **02-Sep-2021**: some minor non-breaking additions:
    - sokol_app.h: new events FOCUSED and UNFOCUSED to indicate that the
      window has gained or lost the focused state (Win32: WM_SETFOCUS/WM_KILLFOCUS,
      macOS: windowDidBecomeKey/windowDidResignKey, X11: FocusIn/FocusOut,
      HTML5: focus/blur).
    - sokol_app.h Emscripten backend: the input event keycode is now extracted
      from the HTML5 code string which yields the actual unmapped virtual key code.

- **21-Aug-2021**: some minor API tweaks in sokol_gl.h and sokol_debugtext.h,
  one of them breaking (still minor though):
    - sokol_gl.h has a new function ```sgl_default_context()``` which returns the
      default context handle, it's the same as the global constant SGL_DEFAULT_CONTEXT,
      but wrapping this in a function is better for language bindings
    - ...and a similar function in sokol_debugtext.h: ```sdtx_default_context()```
    - The sokol_gl.h function ```sgl_default_pipeline()``` has been renamed to
      ```sgl_load_default_pipeline()```. This fits better with the related
      function ```sgl_load_pipeline()``` and doesn't 'semantically clash'
      with the new function sgl_default_context(). The sgl_default_pipeline()
      function is rarely used, so it's quite unlikely that this change breaks
      your code.

- **19-Aug-2021**: sokol_gl.h gained rendering context support, this allows
  sokol-gl to render into different sokol-gfx render passes. No changes are
  needed for existing sokol-gl code. Check the updated
  [header documentation](https://github.com/floooh/sokol/blob/master/util/sokol_gl.h)
  and the new sample
  [sgl-context-sapp](https://floooh.github.io/sokol-html5/sgl-context-sapp.html)
  for details!

- **21-Jun-2021**: A new utility header sokol_color.h has been added, which adds
  sokol_gfx.h-compatible named color constants and a handful initial utility
  functions. See the [header documentation](https://github.com/floooh/sokol/blob/master/util/sokol_color.h)
  for details. Many thanks to Stuart Adams (@nyalloc) for contributing the header!

- **12-Apr-2021**: Minor new feature in sokol_app.h: mouse buttons are now
  also reported as modifier flags in most input events (similar to the
  Ctrl-, Alt-, Shift- and Super-key modifiers). This lets you quickly check
  what mouse buttons are currently pressed in any input event without having
  to keep track of pressed mouse buttons yourself. This is implemented in the following
  sokol_app.h backends: Win32, UWP, Emscripten, X11 and macOS. Example
  code is in the [events-sapp.cc](https://floooh.github.io/sokol-html5/events-sapp.html) sample

- **10-Apr-2021**: followup fixes from yesterday: custom icon support on macOS
  has been added (since macOS has no regular window icons, the dock icon is
  updated instead), and a bugfix in the internal helper which select the
  best matching candidate image (this actually always selected the first
  candidate image)

- **09-Apr-2021**: sokol_app.h now allows to programmatically set the window
  icon in the Win32, X11 and HTML5 backends. Search for "WINDOW ICON SUPPORT"
  in sokol_app.h for documentation, and see the new
  [icon sample](https://floooh.github.io/sokol-html5/icon-sapp.html) for example code.

- **01-Apr-2021**: some fixes in sokol_app.h's iOS backend:
    - In the iOS Metal backend, high-dpi vs low-dpi works again. Some time
    ago (around iOS 12.x) MTKView started to ignore the contentScaleFactor
    property, which lead to sokol_app.h always setting up a HighDPI
    framebuffer even when sapp_desc.high_dpi wasn't set. The fix is to set
    the MTKView's drawableSize explicitly now.
    - The iOS GL backend didn't support MSAA multisampling so far, this has
    been fixed now, but only one MSAA mode (4x) is available, which will be
    selected when sapp_desc.sample_count is greater than 1.

- **31-Mar-2021**: sokol_audio.h on macOS no longer includes system framework
  headers (AudioToolbox/AudioToolbox.h), instead the necessary declarations
  are embedded directly in sokol_audio.h (to get the old behaviour and
  force inclusion of AudioToolbox/AudioToolbox.h, define
  ```SAUDIO_OSX_USE_SYSTEM_HEADERS``` before including the sokol_audio.h
  implementation). This "fix" is both an experiment and an immediate workaround
  for a current issue in Zig's HEAD version (what will eventually become
  zig 0.8.0). See this issue for details: https://github.com/ziglang/zig/issues/8360).
  The experiment is basically to see whether this approach generally makes sense
  (replacing system headers with embedded declarations, so that the sokol headers
  only depend on C standard library headers). This approach might
  simplify cross-compilation and integration with other languages than C and C++.

- **20-Mar-2021**: The Windows-specific OpenGL loader, and the platform-specific
GL header includes have been moved from sokol_app.h to sokol_gfx.h. This means:
  - In general, the sokol_gfx.h implementation can now simply be included
    without having to include other headers which provide the GL API declarations
    first (e.g. when sokol_gfx.h is used without sokol_app.h, you don't need to
    use a GL loader, or include the system-specific GL headers yourself).
  - When sokol_gfx.h is used together with sokol_app.h, the include order
    for the implementations doesn't matter anymore (until now, the sokol_app.h
    implementation had to be included before the sokol_gfx.h implementation).
  - The only "downside" (not really a downside) is that sokol_gfx.h now has
    platform detection ifdefs to include the correct GL headers for a given
    platform. Until now this problem was "delegated" to the library user.
  - The old macro **SOKOL_WIN32_NO_GL_LOADER** has been removed, and replaced
    with a more general **SOKOL_EXTERNAL_GL_LOADER**. Define this before
    including the sokol_gfx.h implementation if you are using your own GL
    loader or provide the GL API declarations in any other way. In this case,
    sokol_gfx.h will not include any platform GL headers, and the embedded
    Win32 GL loader will be disabled.

- **22-Feb-2021**: Mouse input latency in sokol_app.h's macOS backend has been
  quite significantly reduced, please see the detailed explanation [in this
  PR](https://github.com/floooh/sokol/pull/483). Many thanks to @randrew for
  the PR!

- **19-Feb-2021**: sokol_app.h learned some Windows-specific config options
to redirect stdout/stderr to the parent terminal or a separate console
window, and allow outputting UTF-8 encoded text. For details, search for
"WINDOWS CONSOLE OUTPUT" in
[sokol_app.h](https://github.com/floooh/sokol/blob/master/sokol_app.h). Many
thanks to @garettbass for the initial PR!

- **17-Feb-2021**: When compiled for iOS, the sokol_audio.h CoreAudio backend now
uses the **AVAudioSession** class to activate and deactivate audio output as needed.
This fixes sokol_audio.h for iPhones (so far, sokol_audio.h accidentally only worked
for iPads). Please see [this issue](https://github.com/floooh/sokol/issues/431) for details.
A somewhat unfortunate side effect of this fix is that sokol_audio.h must now be compiled
as Objective-C when targeting iOS, also note that a new framework must be linked: ```AVFoundation```.
Many thanks to @oviano for providing the PR!

- **14-Feb-2021**: The Dear ImGui rendering backend in [sokol_imgui.h](https://github.com/floooh/sokol/blob/master/util/sokol_imgui.h) has been rewritten to only do a single
buffer-update per frame each for vertex- and index-data. This addresses performance-problems
with sg_append_buffer() in the GL backend on some platforms (see [this issue](https://github.com/floooh/sokol/issues/399) for details.

- **13-Feb-2021**: A new utility header [sokol_nuklear.h](https://github.com/floooh/sokol/blob/master/util/sokol_nuklear.h)
has been added which implements a rendering backend for [Nuklear](https://github.com/Immediate-Mode-UI/Nuklear)
on top of sokol_gfx.h. Also see the new sample [nuklear-sapp](https://floooh.github.io/sokol-html5/nuklear-sapp.html).
Many thanks to **@wmerrifield** for the PR!

- **10-Feb-2021**: The breaking API-update has been merged (mainly sokol_gfx.h).
Please see [this blogpost](https://floooh.github.io/2021/02/07/sokol-api-overhaul.html)
and the updates [sokol samples](https://floooh.github.io/sokol-html5/) for details.
I also created a git tag named 'pre-feb2021-api-changes' which captures the previous
state in all related projects. Please also update the [sokol-tools-bin](https://github.com/floooh/sokol-tools-bin) if you're using the sokol-shdc shader compiler.

- **07-Feb-2021**: A PSA about upcoming breaking changes in (mainly) sokol_gfx.h: https://floooh.github.io/2021/02/07/sokol-api-overhaul.html

- **20-Dec-2020**: A couple of minor breaking changes in the sokol_gfx.h and
sokol_app.h APIs as preparation for the upcoming automatic language binding
generation:
    - in **sokol_gfx.h** nested unions have been removed:
        - **sg_image_desc.depth/.layers** has been renamed to **.num_slices**
        - **sg_attachment_desc.face/.layer/.slice** has been unified to **.slice**
    - in **sokol_app.h** the return value of **sapp_run()** has been changed from
      **int** to **void** (the function always returned zero anyway)

    Non-breaking (or at most potentially breaking) changes:
    - expressions in enums have been replaced with integer literals (e.g. (1<<2) becomes 4)
    - the value of **SAPP_MOUSEBUTTON_INVALID** has been changed from -1 to 0x100

    For more information about the upcoming automatic language-bindings generation [see this bog post](https://floooh.github.io/2020/08/23/sokol-bindgen.html)

- **02-Dec-2020**: sokol_gfx.h has a couple new public API functions for
destroying resources in two steps:
    - sg_uninit_buffer + sg_dealloc_buffer
    - sg_uninit_image + sg_dealloc_image
    - sg_uninit_shader + sg_dealloc_shader
    - sg_uninit_pipeline + sg_dealloc_pipeline
    - sg_uninit_pass + sg_dealloc_pass

    Calling both functions in this order is identical with calling the
    traditional sg_destroy_xxx() functions. See this PR for more details:
    https://github.com/floooh/sokol/pull/435. Many thanks to @oviano for the
    PR!

- **28-Nov-2020**: In addition to the generic SOKOL_API_DECL and SOKOL_IMPL
defines there are now header-specific versions SOKOL_xxx_API_DECL and
SOKOL_xxx_IMPL (for instance SOKOL_GFX_API_DECL and SOKOL_GFX_IMPL). The
original motivation for splitting the SOKOL_API_DECL defines up is described
here: https://github.com/floooh/sokol/issues/428). The same change for
SOKOL_IMPL also finally unifies the approach used in the utility headers (in
the ```util``` subdirectory), which exclusively used the SOKOL_xxx_IMPL
pattern with the core headers which exclusively used SOKOL_IMPL before (all
headers accept both patterns now). Many thanks to @iboB for providing the
API_DECL PR!

- **17-Nov-2020**: A new utility header **sokol_shape.h** to generate
  vertices+indices for simple shapes (plane, box, sphere, cylinder and torus),
  which seamlessly plug into the sokol_gfx.h resource creation functions. As
  with most new utility headers, the initial functionality is a bit bare bones
  and the public API shouldn't be considered stable yet. Check the sokol-samples
  webpage for new and updates samples: https://floooh.github.io/sokol-html5/

- **08-Nov-2020** PSA: It appears that RenderDoc v1.10 chokes on the new
  D3D11/DXGI swapchain code from 10-Oct-2020 in sokol_app.h. The current
  RenderDoc Nightly Build works, so I guess in v1.11 everything will be fine.

- **03-Nov-2020**: sokol_app.h: the missing drag'n'drop support for HTML5/WASM
  has been added. This adds two platform-specific functions
  ```sapp_html5_get_dropped_file_size()``` and
  ```sapp_html5_fetch_dropped_file()```. Please read the documentation
  section in sokol_app.h under 'DRAG AND DROP SUPPORT' for additional
  details and example code. Also consult the source code of the new
  ```droptest-sapp``` sample for an example of how to load the content
  of dropped files on the web and native platforms:

  https://floooh.github.io/sokol-html5/droptest-sapp.html


- **27-Oct-2020**: I committed a bugfix for a longstanding WebGL canvas id versus
  css-selector confusion in the emscripten/WASM backend code in sokol_app.h.
  I think the fix should not require any changes in your code (because if
  you'd be using a canvas name different from the default "canvas" it wouldn't
  have worked before anyway). See this bug for details: https://github.com/floooh/sokol/issues/407

- **22-Oct-2020**: sokol_app.h now has file drag'n'drop support on Win32,
  macOS and Linux. WASM/HTML5 support will be added soon-ish. This will
  work a bit differently because of security-related restrictions in the
  HTML5 drag'n'drop API, but more on that later. For documentation,
  search for 'DRAG AND DROP SUPPORT' in [sokol_app.h](https://github.com/floooh/sokol/blob/master/sokol_app.h).

  Check out [events-sapp.c](https://github.com/floooh/sokol-samples/blob/master/sapp/events-sapp.cc)
  for a simple usage example (I will also add a more real-world example to my
  chips emulators once the WASM/HTML5 implementation is ready).

  Many thanks for @prime31 and @hb3p8 for the initial PRs and valuable feature
  discussions!

- **10-Oct-2020**: Improvements to the sokol_app.h Win32+D3D11 and UWP+D3D11 swapchain code:
  - In the Win32+D3D11 backend and when running on Win10,
    ```DXGI_SWAP_EFFECT_FLIP_DISCARD``` is now used.  This gets rid of a
    deprecation warning in the debugger console and also should allow slightly
    more efficient swaps in some situations. When running on Win7 or Win8, the
    traditional ```DXGI_SWAP_EFFECT_DISCARD``` is used.
  - The UWP backend now supports MSAA multisampling (the required fixes for
    this are the same as in the Win32 backend with the new swap effect: a
    separate MSAA texture and render-target-view is created where
    rendering goes into, and this MSAA texture is resolved into the actual
    swapchain surface before presentation).

- **07-Oct-2020**:
    A fix in the ALSA/Linux backend initialization in sokol_audio.h: Previously,
    initialization would fail if ALSA can't allocate the exact requested
    buffer size. Instead sokol_audio.h let's now pick ALSA a suitable buffer
    size. Also better log messages in the ALSA initialization code if something
    goes wrong. Unfortunately I'm not able to reproduce the buffer allocation
    problem on my Linux machine. Details are in this issue: https://github.com/floooh/sokol/issues/400

    **NARRATOR**: the fix didn't work.

- **02-Oct-2020**:
    The sokol_app.h Win32 backend can now render while moving and resizing
    the window. NOTE that resizing the swapchain buffers (and receiving
    SAPP_EVENTTYPE_RESIZED events) is deferred until the resizing finished.
    Resizing the swapchain buffers each frame created a substantial temporary
    memory spike of up to several hundred MBytes. I need to figure out a better
    swapchain resizing strategy.

- **30-Sep-2020**:
    sokol_audio.h now works on UWP, thanks again to Alberto Fustinoni
    (@albertofustinoni) for the PR!

- **26-Sep-2020**:
    sokol_app.h gained a new function sapp_set_window_title() to change
    the window title on Windows, macOS and Linux. Many thanks to
    @medvednikov for the initial PR!

- **23-Sep-2020**:
    sokol_app.h now has initial UWP support using the C++/WinRT set of APIs.
    Currently this requires "bleeding edge" tools: A recent VS2019 version,
    and a very recent Windows SDK version (at least version 10.0.19041.0).
    Furthermore the sokol_app.h implementation must be compiled as C++17
    (this is a requirement of the C++/WinRT headers). Note that the Win32
    backend will remain the primary and recommended backend on Windows. The UWP
    backend should only be used when the Win32 backend is not an option.
    The [sokol-samples](https://github.com/floooh/sokol-samples) project
    has two new build configs ```sapp-uwp-vstudio-debug``` and
    ```sapp-uwp-vstudio-release``` to build the sokol-app samples for UWP.

    Many thanks to Alberto Fustinoni (@albertofustinoni) for providing
    the initial PR!

    (also NOTE: UWP-related fixes in other sokol headers will follow)

- **22-Sep-2020**:
    A small fix in sokol_app.h's Win32 backend: when a mouse button is pressed,
    mouse input is now 'captured' by calling SetCapture(), and when the last
    mouse button is released, ReleaseCapture() is called. This also provides
    mouse events outside the window area as long as a mouse button is pressed,
    which is useful for windowed UI applicactions (this is not the same as the
    more 'rigorous' and explicit pointer-lock feature which is more useful for
    camera-controls)

- **31-Aug-2020**:
    Internal change: The D3D11/DXGI backend code in sokol_gfx.h and sokol_app.h
    now use the D3D11 and DXGI C++-APIs when the implementation is compiled as
    C++, and the C-APIs when the implementation is compiled as C (before, the C
    API was also used when the implementation is compiled as C++). The new
    behaviour is useful when another header *must* use the D3D11/DXGI C++ APIs
    but should be included in the same compilation unit as sokol_gfx.h an
    sokol_app.h (for example see this PR:
    https://github.com/floooh/sokol/pull/351).

- **24-Aug-2020**:
    The backend-specific callback functions that are provided to sokol_gfx.h
    in the ```sg_setup()``` initialization call now have alternative
    versions which accept a userdata-pointer argument. The userdata-free functions
    still exist, so no changes are required for existing code.

- **02-Aug-2020**:
    - sokol_app.h now has a mouse-lock feature (aka pointer-lock) via two
      new functions ```void sapp_lock_mouse(bool lock)``` and ```bool sapp_mouse_locked(void)```.
      For documentation, please search for 'MOUSE LOCK' in sokol_app.h.
      The sokol-app samples [events-sapp](https://floooh.github.io/sokol-html5/events-sapp.html)
      and [cgltf-sapp](https://floooh.github.io/sokol-html5/cgltf-sapp.html) have been
      updated to demonstrate the feature.
    - sokol_app.h Linux: mouse pointer visibility (via ```void sapp_show_mouse(bool show)```)
      has been implemented for Linux/X11
    - sokol_app.h WASM: mouse wheel scroll deltas are now 'normalized' between
      the different scroll modes (pixels, lines, pages). See this issue:
      https://github.com/floooh/sokol/issues/339. Many thanks to @bqqbarbhg for
      investigating the issue and providing a solution!
    - sokol_app.h now has [better documentation](https://github.com/floooh/sokol/blob/89a3bb8da0a2df843d6cc60a270ddc69f9aa69d6/sokol_app.h#L70)
      what system libraries must be linked on the various platforms (and on Linux two additional libraries must be
      linked now: Xcursor and Xi)

- **22-Jul-2020**: **PLEASE NOTE** cmake 3.18 breaks some of sokol samples when
  compiling with the Visual Studio toolchain because some C files now actually
  compile as C++ for some reason (see:
  https://twitter.com/FlohOfWoe/status/1285996526117040128).  Until this is
  fixed, or I have come up with a workaround, please use an older cmake version
  to build the sokol samples with the Visual Studio compiler.

  (Update: I have added a workaround to fips: https://github.com/floooh/fips/commit/89997b8ebdca6fc9455a5cfe6145eecaa017df49
  which fixes the issue at least for fips projects)

- **14-Jul-2020**:
    - sapp_mouse_shown() has been implemented for macOS (thanks to @slmjkdbtl for
      providing the initial PR!)
    - On macOS, the lower-level functions CGDisplayShowCursor and CGDisplayHideCursor
      are now used instead of the NSCursor class. This is in preparation for the
      'pointer lock' feature which will also use CGDisplay* functions.
    - Calling ```sapp_show_mouse(bool visible)``` no longer 'stacks' (e.g. there's
      no 'hidden counter' underneath anymore, instead calling ```sapp_show_mouse(true)```
      will always show the cursor and ```sapp_show_mouse(false)``` will always
      hide it. This is a different behaviour than the underlying Win32 and
      macOS functions ShowCursor() and CGDisplaShow/HideCursor()
    - The mouse show/hide behaviour can now be tested in the ```events-sapp``` sample
      (so far this only works on Windows and macOS).

- **13-Jul-2020**:
    - On macOS and iOS, sokol_app.h and sokol_gfx.h can now be compiled with
      ARC (Automatic Reference Counting) **disabled** (previously ARC had to be
      enabled).
    - Compiling with ARC enabled is still supported but with a little caveat:
      if you're compiling sokol_app.h or sokol_gfx.h in ObjC mode (not ObjC++
      mode) *AND* ARC is enabled, then the Xcode version must be more recent
      than before (the language feature ```__has_feature(objc_arc_fields)```
      must be supported, which I think has been added in Xcode 10.2, I couldn't
      find this mentioned in any Xcode release notes though). Compiling with
      ARC disabled should also work on older Xcode versions though.
    - Various internal code cleanup things:
        - sokol_app.h had the same 'structural cleanup' as sokol_gfx.h in
          January, all internal state (including ObjC id's) has been merged into
          a single big state structure. Backend specific struct declarations
          have been moved closer together in the header, and
          backend-specific structures and functions have been named more
          consistently for better 'searchability'
        - The 'mini GL' loader in the sokol_app.h Win32+WGL backend has been
          rewritten to use X-Macros (less redundant lines of code)
        - All macOS and iOS code has been revised and cleaned up
        - On macOS a workaround for a (what looks like) post-Catalina
          NSOpenGLView issue has been added: if the sokol_app.h window doesn't
          fit on screen (and was thus 'clamped' by Cocoa) *AND* the
          content-size was not set to native Retina resolution, the initial
          content size was reported as if it was in Retina resolution. This
          caused an empty screen to be rendered in the imgui-sapp demo. The
          workaround is to hook into the NSOpenGLView reshape event at which
          point the reported content size is correct.
        - On macOS and iOS, the various 'view delegate' objects have been
          removed, and rendering happens instead in the subclasses of MTKView,
          GLKView and NSOpenGLView.
        - On macOS and iOS, there's now proper cleanup code in the
          applicationWillTerminate callback (although note that on iOS this
          function isn't guaranteed to be called, because an application can
          also simply be killed by the operating system.

- **22-Jun-2020**: The X11/GLX backend in sokol_app.h now has (soft-)fullscreen
support, bringing the feature on par with Windows and macOS. Many thanks to
@medvednikov for the PR!

- **20-Jun-2020**: Some work to better support older DX10-level GPUs in the
sokol_gfx.h D3D11 backend:
    - sg_make_shader() now by default compiles HLSL shader code as shader model 4.0
      (previously shader model 5.0 which caused problems with some older
      Intel GPUs still in use, see this issue: https://github.com/floooh/sokol/issues/179)
    - A new string item ```const char* d3d11_target``` in ```sg_shader_stage_desc``` now allows
      to pass in the D3D shader model for compiling shaders. This defaults to
      "vs_4_0" for the vertex shader stage and "ps_4_0" for the fragment shader stage.
      The minimal DX shader model for use with the sokol_gfx.h D3D11 backend is
      shader model 4.0, because that's the first shader model supporting
      constant buffers.
    - The *sokol-shdc* shader compiler tool has a new output option ```hlsl4```
      to generate HLSL4 source code and shader model 4.0 byte code.
    - All embedded D3D shader byte code in the sokol utility headers has been
      changed from shader model 5.0 to 4.0

    If you are using sokol_gfx.h with sokol-shdc, please update both at the same time
    to avoid compilation errors caused by the new ```sg_shader_stage_desc.d3d11_target```
    item. The sg_shader_desc initialization code in sokol-shdc has now been made more
    robust to prevent similar problems in the future.

- **14-Jun-2020**: I have added a very simple utility header ```sokol_memtrack.h```
which allows to track memory allocations in sokol headers (number and overall
size of allocations) by overriding the macros SOKOL_MALLOC, SOKOL_CALLOC and
SOKOL_FREE. Simply include ```sokol_memtrack.h``` before the other sokol
header implementation includes to enable memory tracking in those headers
(but remember that the sokol_memtrack.h implementation must only be included
once in the whole project, so this only works when all other sokol header
implementations are included in the same compilation unit).

- **06-Jun-2020**: Some optimizations in the sokol_gfx.h GL backend to avoid
  redundant GL calls in two areas: in the sg_begin_pass() calls when not
  clearing the color- and depth-stencil-attachments, and in sg_apply_bindings()
  when binding textures.  Everything should behave exactly as before, but if
  you notice any problems in those areas, please file a bug. Many thanks to
  @edubart for the PRs!

- **01-Jun-2020**: sokol_app.h now allows to toggle to and from fullscreen
programmatically and to query the current fullscreen state via 2 new
functions: ```sapp_toggle_fullscreen()``` and ```sapp_is_fullscreen()```.
Currently this is only implemented for Windows and macOS (not Linux).
Thanks to @mattiasljungstrom for getting the feature started and providing
the Win32 implementation!

- **28-May-2020**: a small quality-of-life improvement for C++ coders: when the
sokol headers are included into C++, all public API functions which take a
pointer to a struct now have a C++ overload which instead takes a const-ref.
This allows to move the struct initialization right into the function call
just like in C99. For instance, in C99 one can write:
    ```c
    sg_buffer buf = sg_make_buffer(&(sg_buffer_desc){
        .size = sizeof(vertices),
        .type = SG_BUFFERTYPE_VERTEXBUFFER,
        .content = vertices
    });
    ```
    In C++ it isn't possible to take the address of an 'adhoc-initialized'
    struct like this, but with the new reference-wrapper functions (and C++20
    designated initialization) this should work now:
    ```cpp
    sg_buffer buf = sg_make_buffer({
        .size = sizeof(vertices),
        .type = SG_BUFFERTYPE_VERTEXBUFFER,
        .content = vertices
    });
    ```
    Many thanks to @garettbass for providing the PR!


- **27-May-2020**: a new utility header [sokol_debugtext.h](https://github.com/floooh/sokol/blob/master/util/sokol_debugtext.h)
for rendering simple ASCII text using vintage home computer fonts via sokol_gfx.h

- **13-May-2020**: a new function in sokol_time.h to round a measured frame time
to common display refresh rates: ```stm_round_to_common_refresh_rate()```.
See the header documentation for the motivation behind this function.

- **02-May-2020**: sokol_app.h: the 'programmatic quit' behaviour on the
web-platform is now more in line with other platforms: calling
```sapp_quit()``` will invoke the cleanup callback function, perform
platform-specific cleanup (like unregistering JS event handlers), and finally
exit the frame loop. In typical scenarios this isn't very useful (because
usually the user will simply close the tab, which doesn't allow to run
cleanup code), but it's useful for situations where the same
code needs to run repeatedly on a web page. Many thanks to @caiiiycuk
for providing the PR!

- **30-Apr-2020**: experimental WebGPU backend and a minor breaking change:
    - sokol_gfx.h: a new WebGPU backend, expect frequent breakage for a while
      because the WebGPU API is still in flux
    - a new header sokol_glue.h, with interop helper functions when specific combinations
      of sokol headers are used together
    - changes in the way sokol_gfx.h is initialized via a new layout of the
      sg_desc structure
    - sokol_gfx.h: a new ```sg_sampler_type``` enum which is required for
      shader creation to tell the WebGPU backend about the sampler data types
      (float, signed int, or unsigned int) used in the shader
    - sokol_app.h: a handful new functions to query default framebuffer attributes (color- and
      depth-buffer pixel formats, and MSAA sample count)
    - sokol_app.h: WebGPU device and swapchain initialization (currently only
      in the emscripten code path)
    - [sokol-shdc](https://github.com/floooh/sokol-tools/blob/master/docs/sokol-shdc.md) has
      been updated with WebGPU support (currently outputs SPIRV bytecode), and to output the new
      ```sg_sampler_type``` enum in ```sg_shader_image_desc```
    - [sokol-samples](https://github.com/floooh/sokol-samples/) has a new set of
      backend-specific WebGPU samples, and the other samples have been updated
      for the new sokol-gfx initialization
    - ```pre-webgpu``` tags have been added to the [sokol](https://github.com/floooh/sokol/releases/tag/pre-webgpu), [sokol-samples](https://github.com/floooh/sokol-samples/releases/tag/pre-webgpu), [sokol-tools](https://github.com/floooh/sokol-tools/releases/tag/pre-webgpu)
      and [sokol-tools-bin](https://github.com/floooh/sokol-tools-bin/releases/tag/pre-webgpu) github repositories (in case you need to continue working with
      the older versions)
    - please see this [blog post](https://floooh.github.io/2020/04/26/sokol-spring-2020-update.html)
      for more details

- **05-Apr-2020**: A bugfix in sokol_gl.h, the (fairly recent) optimization for
    merging draw calls contained a bug that could be triggered in an "empty"
    sgl_begin/sgl_end pair (with no vertices recorded inbetween). This could
    lead to the following draw call being rendered with the wrong uniform data.

- **30-Jan-2020**: Some cleanup in sokol_gfx.h in the backend implementation code,
    internal data structures and documentation comments. The public
    API hasn't changed, so the change should be completely invisible
    from the outside.

- **02-Dec-2019**: Initial clipboard support in sokol_app.h for Windows, macOS
    and HTML5. This allows to read and write UTF-8 encoded strings from and
    to the target platform's shared clipboard.

    A 'real-world' example usage is in the [Visual6502 Remix project](https://github.com/floooh/v6502r).

    Unfortunately clipboard support on the HTML5 platform comes with a lot of
    platform-specific caveats which can't be solved in sokol_app.h alone
    because of the restrictions the web platform puts on clipboard access and
    different behaviours and support levels of the various HTML5 clipboard
    APIs. I'm not really happy with the current HTML5 clipboard
    implementation. It sorta works, but it sure ain't pretty :)

    Maybe the situation will improve in a few years when all browsers agree
    on and support the new [permission-based clipboard
    API](https://developer.mozilla.org/en-US/docs/Web/API/Clipboard_API).

    For documentation of the clipboard feature, search for CLIPBOARD SUPPORT
    in sokol_app.h

- **08-Sep-2019**: sokol_gfx.h now supports clamp-to-border texture sampling:
    - the enum ```sg_wrap``` has a new member ```SG_WRAP_CLAMP_TO_BORDER```
    - there's a new enum ```sg_border_color```
    - the struct ```sg_image_desc``` has a new member ```sg_border_color border_color```
    - new feature flag in ```sg_features```: ```image_clamp_to_border```

  Note the following caveats:

    - clamp-to-border is only supported on a subset of platforms, support can
    be checked at runtime via ```sg_query_features().image_clamp_to_border```
    (D3D11, desktop-GL and macOS-Metal support clamp-to-border,
    all other platforms don't)
    - there are three hardwired border colors: transparent-black,
    opaque-black and opaque-white (modern 3D APIs have moved away from
    a freely programmable border color)
    - if clamp-to-border is not supported, sampling will fall back to
    clamp-to-edge without a validation warning

  Many thanks to @martincohen for suggesting the feature and providing the initial
D3D11 implementation!

- **31-Aug-2019**: The header **sokol_gfx_cimgui.h** has been merged into
[**sokol_gfx_imgui.h**](https://github.com/floooh/sokol/blob/master/util/sokol_gfx_imgui.h).
Same idea as merging sokol_cimgui.h into sokol_imgui.h, the implementation
is now "bilingual", and can either be included into a C++ file or into a C file.
When included into a C++ file, the Dear ImGui C++ API will be called directly,
otherwise the C API bindings via cimgui.h

- **28-Aug-2019**: The header **sokol_cimgui.h** has been merged into
[**sokol_imgui.h**](https://github.com/floooh/sokol/blob/master/util/sokol_imgui.h).
The sokol_cimgui.h header had been created to implement Dear ImGui UIs from
pure C applications, instead of having to fall back to C++ just for the UI
code. However, there was a lot of code duplication between sokol_imgui.h and
sokol_cimgui.h, so that it made more sense to merge the two headers. The C vs
C++ code path will be selected automatically: When the implementation of
sokol_imgui.h is included into a C++ source file, the Dear ImGui C++ API will
be used. Otherwise, when the implementation is included into a C source file,
the C API via cimgui.h

- **27-Aug-2019**: [**sokol_audio.h**](https://github.com/floooh/sokol/blob/master/sokol_audio.h)
  now has an OpenSLES backend for Android. Many thanks to Sepehr Taghdisian (@septag)
  for the PR!

- **26-Aug-2019**: new utility header for text rendering, and fixes in sokol_gl.h:
    - a new utility header [**sokol_fontstash.h**](https://github.com/floooh/sokol/blob/master/util/sokol_fontstash.h)
      which implements a renderer for [fontstash.h](https://github.com/memononen/fontstash)
      on top of sokol_gl.h
    - **sokol_gl.h** updates:
        - Optimization: If no relevant state between two begin/end pairs has
        changed, draw commands will be merged into a single sokol-gfx draw
        call. This is especially useful for text- and sprite-rendering (previously,
        each begin/end pair would always result in one draw call).
        - Bugfix: When calling sgl_disable_texture() the previously active
        texture would still remain active which could lead to rendering
        artefacts. This has been fixed.
        - Feature: It's now possible to provide a custom shader in the
        'desc' argument of *sgl_make_pipeline()*, as long as the shader
        is "compatible" with sokol_gl.h, see the sokol_fontstash.h
        header for an example. This feature isn't "advertised" in the
        sokol_gl.h documentation because it's a bit brittle (for instance
        if sokol_gl.h updates uniform block structures, custom shaders
        would break), but it may still come in handy in some situations.

- **20-Aug-2019**: sokol_gfx.h has a couple new query functions to inspect the
  default values of resource-creation desc structures:

    ```c
    sg_buffer_desc sg_query_buffer_defaults(const sg_buffer_desc* desc);
    sg_image_desc sg_query_image_defaults(const sg_image_desc* desc);
    sg_shader_desc sg_query_shader_defaults(const sg_shader_desc* desc);
    sg_pipeline_desc sg_query_pipeline_defaults(const sg_pipeline_desc* desc);
    sg_pass_desc sg_query_pass_defaults(const sg_pass_desc* desc);
    ```
  These functions take a pointer to a resource creation desc struct that
  may contain zero-initialized values (to indicate default values) and
  return a new struct where the zero-init values have been replaced with
  concrete values. This is useful to inspect the actual creation attributes
  of a resource.

- **18-Aug-2019**:
    - Pixelformat and runtime capabilities modernization in sokol_gfx.h (breaking changes):
        - The list of pixel formats supported in sokol_gfx.h has been modernized,
          many new formats are available, and some formats have been removed. The
          supported pixel formats are now identical with what WebGPU provides,
          minus the SRGB formats (if SRGB conversion is needed it should be done
          in the pixel shader)
        - The pixel format list is now more "orthogonal":
            - one, two or four color components (R, RG, RGBA)
            - 8-, 16- or 32-bit component width
            - unsigned-normalized (no postfix), signed-normalized (SN postfix),
              unsigned-integer (UI postfix) and signed-integer (SI postfix)
              and float (F postfix) component types.
            - special pixel formats BGRA8 (default render target format on
              Metal and D3D11), RGB10A2 and RG11B10F
            - DXT compressed formats replaced with BC1 to BC7 (BC1 to BC3
              are identical to the old DXT pixel formats)
            - packed 16-bit formats (like RGBA4) have been removed
            - packed 24-bit formats (RGB8) have been removed
        - Use the new function ```sg_query_pixelformat()``` to get detailed
          runtime capability information about a pixelformat (for instance
          whether it is supported at all, can be used as render target etc...).
        - Use the new function ```sg_query_limits()``` to query "numeric limits"
          like maximum texture dimensions for different texture types.
        - The enumeration ```sg_feature``` and the function ```sg_query_feature()```
          has been replaced with the new function ```sg_query_features()```, which
          returns a struct ```sg_features``` (this contains a bool for each
          optional feature).
        - The default pixelformat for render target images and pipeline objects
          now depends on the backend:
            - for GL backends, the default pixelformat stays the same: RGBA8
            - for the Metal and D3D11 backends, the default pixelformat for
            render target images is now BGRA8 (the reason is because
            MTKView's pixelformat was always BGRA8 but this was "hidden"
            through an internal hack, and a BGRA swapchain is more efficient
            than RGBA in D3D11/DXGI)
        - Because of the above RGBA/BGRA change, you may see pixelformat validation
          errors in existing code if the code assumes that a render target image is
          always created with a default pixelformat of RGBA8.
    - Changes in sokol_app.h:
        - The D3D11 backend now creates the DXGI swapchain with BGRA8 pixelformat
          (previously: RGBA8), this allows more efficient presentation in some
          situations since no format-conversion-blit needs to happen.

- **18-Jul-2019**:
    - sokol_fetch.h has been fixed and can be used again :)

- **11-Jul-2019**:
    - Don't use sokol_fetch.h for now, the current version assumes that
      it is possible to obtain the content size of a file from the
      HTTP server without downloading the entire file first. Turns out
      that's not possible with vanilla HTTP when the web server serves
      files compressed (in that case the Content-Length is the _compressed_
      size, yet JS/WASM only has access to the uncompressed data).
      Long story short, I need to go back to the drawing board :)

- **06-Jul-2019**:
    - new header [sokol_fetch.h](https://github.com/floooh/sokol/blob/master/sokol_fetch.h) for asynchronously loading data.
        - make sure to carefully read the embedded documentation
        for making the best use of the header
        - two new samples: [simple PNG file loadng with stb_image.h](https://floooh.github.io/sokol-html5/loadpng-sapp.html) and  [MPEG1 streaming with pl_mpeg.h](https://floooh.github.io/sokol-html5/plmpeg-sapp.html)
    - sokol_gfx.h: increased SG_MAX_SHADERSTAGE_BUFFERS configuration
    constant from 4 to 8.

- **10-Jun-2019**: sokol_app.h now has proper "application quit handling":
    - a pending quit can be intercepted, for instance to show a "Really Quit?" dialog box
    - application code can now initiate a "soft quit" (interceptable) or
      "hard quit" (not interceptable)
    - on the web platform, the standard "Leave Site?" dialog box implemented
      by browsers can be shown when the user leaves the site
    - Android and iOS currently don't have any of those features (since the
      operating system may decide to terminate mobile applications at any time
      anyway, if similar features are added they will most likely have
      similar limitations as the web platform)
  For details, search for 'APPLICATION QUIT' in the sokol_app.h documentation
  header: https://github.com/floooh/sokol/blob/master/sokol_app.h

  The [imgui-highdpi-sapp](https://github.com/floooh/sokol-samples/tree/master/sapp)
  contains sample code for all new quit-related features.

- **08-Jun-2019**: some new stuff in sokol_app.h:
    - the ```sapp_event``` struct has a new field ```bool key_repeat```
    which is true when a keyboard event is a key-repeat (for the
    event types ```SAPP_EVENTTYPE_KEY_DOWN``` and ```SAPP_EVENTTYPE_CHAR```).
    Many thanks to [Scott Lembcke](https://github.com/slembcke) for
    the pull request!
    - a new function to poll the internal frame counter:
    ```uint64_t sapp_frame_count(void)```, previously the frame counter
    was only available via ```sapp_event```.
    - also check out the new [event-inspector sample](https://floooh.github.io/sokol-html5/wasm/events-sapp.html)

- **04-Jun-2019**: All sokol headers now recognize a config-define ```SOKOL_DLL```
  if sokol should be compiled into a DLL (when used with ```SOKOL_IMPL```)
  or used as a DLL. On Windows, this will prepend the public function declarations
  with ```__declspec(dllexport)``` or ```__declspec(dllimport)```.

- **31-May-2019**: if you're working with emscripten and fips, please note the
  following changes:

    https://github.com/floooh/fips#public-service-announcements

- **27-May-2019**: some D3D11 updates:
    - The shader-cross-compiler can now generate D3D bytecode when
    running on Windows, see the [sokol-shdc docs](https://github.com/floooh/sokol-tools/blob/master/docs/sokol-shdc.md) for more
details.
    - sokol_gfx.h no longer needs to be compiled with a
    SOKOL_D3D11_SHADER_COMPILER define to enable shader compilation in the
    D3D11 backend. Instead, the D3D shader compiler DLL (d3dcompiler_47.dll)
    will be loaded on-demand when the first HLSL shader needs to be compiled.
    If an application only uses D3D shader byte code, the compiler DLL won't
    be loaded into the process.

- **24-May-2019** The shader-cross-compiler can now generate Metal byte code
for macOS or iOS when the build is running on macOS. This is enabled
automatically with the fips-integration files in [sokol-tools-bin](https://github.com/floooh/sokol-tools-bin),
see the [sokol-shdc docs](https://github.com/floooh/sokol-tools/blob/master/docs/sokol-shdc.md) for more
details.

- **16-May-2019** two new utility headers: *sokol_cimgui.h* and *sokol_gfx_cimgui.h*,
those are the same as their counterparts sokol_imgui.h and sokol_gfx_imgui.h, but
use the [cimgui](https://github.com/cimgui/cimgui) C-API for Dear ImGui. This
is useful if you don't want to - or cannot - use C++ for creating Dear ImGui UIs.

    Many thanks to @prime31 for contributing those!

    sokol_cimgui.h [is used
here](https://floooh.github.io/sokol-html5/wasm/cimgui-sapp.html), and
sokol_gfx_cimgui.h is used for the [debugging UI
here](https://floooh.github.io/sokol-html5/wasm/sgl-microui-sapp-ui.html)

- **15-May-2019** there's now an optional shader-cross-compiler solution for
sokol_gfx.h: [see here for details](https://github.com/floooh/sokol-tools/blob/master/docs/sokol-shdc.md).
This is "V1.0" with two notable features missing:

    - an include-file feature for GLSL shaders
    - compilation to Metal- and D3D-bytecode (currently
      only source-code generation is supported)

    The [sokol-app samples](https://floooh.github.io/sokol-html5/) have been
    ported to the new shader-cross-compilation, follow the ```src``` and
    ```glsl``` links on the specific sample webpages to see the C- and GLSL-
    source-code.

- **02-May-2019** sokol_gfx.h has a new function ```sg_query_backend()```, this
will return an enum ```sg_backend``` identifying the backend sokol-gfx is
currently running on, which is one of the following values:

    - SG_BACKEND_GLCORE33
    - SG_BACKEND_GLES2
    - SG_BACKEND_GLES3
    - SG_BACKEND_D3D11
    - SG_BACKEND_METAL_MACOS
    - SG_BACKEND_METAL_IOS

    When compiled with SOKOL_GLES3, sg_query_backend() may return SG_BACKEND_GLES2
    when the runtime platform doesn't support GLES3/WebGL2 and had to fallback
    to GLES2/WebGL2.

    When compiled with SOKOL_METAL, sg_query_backend() will return SG_BACKEND_METAL_MACOS
    when the compile target is macOS, and SG_BACKEND_METAL_IOS when the target is iOS.

- **26-Apr-2019** Small but breaking change in **sokol_gfx.h** how the vertex
layout definition in sg_pipeline_desc works:

    Vertex component names and semantics (needed by the GLES2 and D3D11 backends) have moved from ```sg_pipeline_desc``` into ```sg_shader_desc```.

    This may seem like a rather pointless small detail to change, especially
    for breaking existing code, but the whole thing will make a bit more
    sense when the new shader-cross-compiler will be integrated which I'm
    currently working on (here: https://github.com/floooh/sokol-tools).

    While working on getting reflection data out of the shaders (e.g. what
    uniform blocks and textures the shader uses), it occurred to me that
    vertex-attribute-names and -semantics are actually part of the reflection
    info and belong to the shader, not to the vertex layout in the pipeline
    object (which only describes how the incoming vertex data maps to
    vertex-component **slots**. Instead of (optionally) mapping this
    association through a name, the pipeline's vertex layout is now always
    strictly defined in terms of numeric 'bind slots' for **all** sokol_gfx.h
    backends. For 3D APIs where the vertex component slot isn't explicitly
    defined in the shader language (GLES2/WebGL, D3D11, and optionally
    GLES3/GL), the shader merely offers a lookup table how vertex-layout
    slot-indices map to names/semantics (and the underlying 3D API than maps
    those names back to slot indices, which shows that Metal and GL made the
    right choice defining the slots right in the shader).

    Here's how the code changes (taken from the triangle-sapp.c sample):

    **OLD**:
    ```c
    /* create a shader */
    sg_shader shd = sg_make_shader(&(sg_shader_desc){
        .vs.source = vs_src,
        .fs.source = fs_src,
    });

    /* create a pipeline object (default render states are fine for triangle) */
    pip = sg_make_pipeline(&(sg_pipeline_desc){
        /* if the vertex layout doesn't have gaps, don't need to provide strides and offsets */
        .shader = shd,
        .layout = {
            .attrs = {
                [0] = { .name="position", .sem_name="POS", .format=SG_VERTEXFORMAT_FLOAT3 },
                [1] = { .name="color0", .sem_name="COLOR", .format=SG_VERTEXFORMAT_FLOAT4 }
            }
        },
    });
    ```

    **NEW**:
    ```c
        /* create a shader */
        sg_shader shd = sg_make_shader(&(sg_shader_desc){
            .attrs = {
                [0] = { .name="position", .sem_name="POS" },
                [1] = { .name="color0", .sem_name="COLOR" }
            },
            .vs.source = vs_src,
            .fs.source = fs_src,
        });

        /* create a pipeline object (default render states are fine for triangle) */
        pip = sg_make_pipeline(&(sg_pipeline_desc){
            /* if the vertex layout doesn't have gaps, don't need to provide strides and offsets */
            .shader = shd,
            .layout = {
                .attrs = {
                    [0].format=SG_VERTEXFORMAT_FLOAT3,
                    [1].format=SG_VERTEXFORMAT_FLOAT4
                }
            },
        });
    ```

    ```sg_shader_desc``` has a new embedded struct ```attrs``` which
    contains a vertex attribute _name_ (for GLES2/WebGL) and
    _sem_name/sem_index_ (for D3D11). For the Metal backend this struct is
    ignored completely, and for GLES3/GL it is optional, and not required
    when the vertex shader inputs are annotated with ```layout(location=N)```.

    The remaining attribute description members in ```sg_pipeline_desc``` are:
    - **.format**: the format of input vertex data (this can be different
          from the vertex shader's inputs when data is extended during
          vertex fetch (e.g. input can be vec3 while the vertex shader
          expects vec4)
    - **.offset**: optional offset of the vertex component data (not needed
          when the input vertex has no gaps between the components)
    - **.buffer**: the vertex buffer bind slot if the vertex data is coming
          from different buffers

    Also check out the various samples:

    - for GLSL (explicit slots via ```layout(location=N)```): https://github.com/floooh/sokol-samples/tree/master/glfw
    - for D3D11 (semantic names/indices): https://github.com/floooh/sokol-samples/tree/master/d3d11
    - for GLES2: (vertex attribute names): https://github.com/floooh/sokol-samples/tree/master/html5
    - for Metal: (explicit slots): https://github.com/floooh/sokol-samples/tree/master/metal
    - ...and all of the above combined: https://github.com/floooh/sokol-samples/tree/master/sapp

- **19-Apr-2019** I have replaced the rather inflexible render-state handling
in **sokol_gl.h** with a *pipeline stack* (like the GL matrix stack, but with
pipeline-state-objects), along with a couple of other minor API tweaks.

    These are the new pipeline-stack functions:
    ```c
    sgl_pipeline sgl_make_pipeline(const sg_pipeline_desc* desc);
    void sgl_destroy_pipeline(sgl_pipeline pip);
    void sgl_default_pipeline(void);
    void sgl_load_pipeline(sgl_pipeline pip);
    void sgl_push_pipeline(void);
    void sgl_pop_pipeline(void);
    ```

    A pipeline object is created just like in sokol_gfx.h, but without shaders,
    vertex layout, pixel formats, primitive-type and sample count (these details
    are filled in by the ```sgl_make_pipeline()``` wrapper function. For instance
    to create a pipeline object for additive transparency:

    ```c
    sgl_pipeline additive_pip = sgl_make_pipeline(&(sg_pipeline_desc){
        .blend = {
            .enabled = true,
            .src_factor_rgb = SG_BLENDFACTOR_ONE,
            .dst_factor_rgb = SG_BLENDFACTOR_ONE
        }
    });
    ```

    And to render with this, simply call ```sgl_load_pipeline()```:

    ```c
    sgl_load_pipeline(additive_pip);
    sgl_begin_triangles();
    ...
    sgl_end();
    ```

    Or to preserve and restore the previously active pipeline state:

    ```c
    sgl_push_pipeline();
    sgl_load_pipeline(additive_pip);
    sgl_begin_quads();
    ...
    sgl_end();
    sgl_pop_pipeline();
    ```

    You can also load the 'default pipeline' explicitly on the top of the
    pipeline stack with ```sgl_default_pipeline()```.

    The other API change is:

    - ```sgl_state_texture(bool b)``` has been replaced with ```sgl_enable_texture()```
      and ```sgl_disable_texture()```

    The code samples have been updated accordingly:

    - [sgl-sapp.c](https://github.com/floooh/sokol-samples/blob/master/sapp/sgl-sapp.c)
    - [sgl-lines-sapp.c](https://github.com/floooh/sokol-samples/blob/master/sapp/sgl-lines-sapp.c)
    - [sgl-microui-sapp.c](https://github.com/floooh/sokol-samples/blob/master/sapp/sgl-microui-sapp.c)

- **01-Apr-2019** (not an April Fool's joke): There's a new **sokol_gl.h**
util header which implements an 'OpenGL-1.x-in-spirit' rendering API on top
of sokol_gfx.h (vertex specification via begin/end, and a matrix stack). This is
only a small subset of OpenGL 1.x, mainly intended for debug-visualization or
simple tasks like 2D UI rendering. As always, sample code is in the
[sokol-samples](https://github.com/floooh/sokol-samples) project.

- **15-Mar-2019**: various Dear ImGui related changes:
    - there's a new utility header sokol_imgui.h with a simple drop-in
      renderer for Dear ImGui on top of sokol_gfx.h and sokol_app.h
      (sokol_app.h is optional, and only used for input handling)
    - the sokol_gfx_imgui.h debug inspection header no longer
      depends on internal data structures and functions of sokol_gfx.h, as such
      it is now a normal *utility header* and has been moved to the *utils*
      directory
    - the implementation macro for sokol_gfx_imgui.h has been changed
      from SOKOL_IMPL to SOKOL_GFX_IMGUI_IMPL (so when you suddenly get
      unresolved linker errors, that's the reason)
    - all headers now have two preprocessor defines for the declaration
      and implementation (for instance in sokol_gfx.h: SOKOL_GFX_INCLUDED
      and SOKOL_GFX_IMPL_INCLUDED) these are checked in the utility-headers
      to provide useful error message when dependent headers are missing

- **05-Mar-2019**: sokol_gfx.h now has a 'trace hook' API, and I have started
implementing optional debug-inspection-UI headers on top of Dear ImGui:
    - sokol_gfx.h has a new function *sg_install_trace_hooks()*, this allows
      you to install a callback function for each public sokol_gfx.h function
      (and a couple of error callbacks). For more details, search for "TRACE HOOKS"
      in sokol_gfx.h
    - when creating sokol_gfx.h resources, you can now set a 'debug label'
      in the desc structure, this is ignored by sokol_gfx.h itself, but is
      useful for debuggers or profilers hooking in via the new trace hooks
    - likewise, two new functions *sg_push_debug_group()* and *sg_pop_debug_group()*
      can be used to group related drawing functions under a name, this
      is also ignored by sokol_gfx.h itself and only useful when hooking
      into the API calls
    - I have started a new 'subproject' in the 'imgui' directory, this will
      contain a slowly growing set of optional debug-inspection-UI headers
      which allow to peek under the hood of the Sokol headers. The UIs are
      implemented with [Dear ImGui](https://github.com/ocornut/imgui). Again,
      see the README in the 'imgui' directory and the headers in there
      for details, and check out the live demos on the [Sokol Sample Webpage](https://floooh.github.io/sokol-html5/)
      (click on the little UI buttons in the top right corner of each thumbnail)

- **21-Feb-2019**: sokol_app.h and sokol_audio.h now have an alternative
set of callbacks with user_data arguments. This is useful if you don't
want or cannot store your own application state in global variables.
See the header documentation in sokol_app.h and sokol_audio.h for details,
and check out the samples *sapp/noentry-sapp.c* and *sapp/modplay-sapp.c*
in https://github.com/floooh/sokol-samples

- **19-Feb-2019**: sokol_app.h now has an alternative mode where it doesn't
"hijack" the platform's main() function. Search for SOKOL_NO_ENTRY in
sokol_app.h for details and documentation.

- **26-Jan-2019**: sokol_app.h now has an Android backend contributed by
  [Gustav Olsson](https://github.com/gustavolsson)!
  See the [sokol-samples readme](https://github.com/floooh/sokol-samples/blob/master/README.md)
  for build instructions.

- **21-Jan-2019**: sokol_gfx.h - pool-slot-generation-counters and a dummy backend:
    - Resource pool slots now have a generation-counter for the resource-id
      unique-tag, instead of a single counter for the whole pool. This allows
      to create many more unique handles.
    - sokol_gfx.h now has a dummy backend, activated by defining SOKOL_DUMMY_BACKEND
      (instead of SOKOL_METAL, SOKOL_D3D11, ...), this allows to write
      'headless' tests (and there's now a sokol-gfx-test in the sokol-samples
      repository which mainly tests the resource pool system)

- **12-Jan-2019**: sokol_gfx.h - setting the pipeline state and resource
bindings now happens in separate calls, specifically:
    - *sg_apply_draw_state()* has been replaced with *sg_apply_pipeline()* and
    *sg_apply_bindings()*
    - the *sg_draw_state* struct has been replaced with *sg_bindings*
    - *sg_apply_uniform_block()* has been renamed to *sg_apply_uniforms()*

All existing code will still work. See [this blog
post](https://floooh.github.io/2019/01/12/sokol-apply-pipeline.html) for
details.

- **29-Oct-2018**:
    - sokol_gfx.h has a new function **sg_append_buffer()** which allows to
    append new data to a buffer multiple times per frame and interleave this
    with draw calls. This basically implements the
    D3D11_MAP_WRITE_NO_OVERWRITE update strategy for buffer objects. For
    example usage, see the updated Dear ImGui samples in the [sokol_samples
    repo](https://github.com/floooh/sokol-samples)
    - the GL state cache in sokol_gfx.h handles buffers bindings in a
    more robust way, previously it might have happened that the
    buffer binding gets confused when creating buffers or updating
    buffer contents in the render loop

- **17-Oct-2018**: sokol_args.h added

- **26-Sep-2018**: sokol_audio.h ready for prime time :)

- **11-May-2018**: sokol_gfx.h now autodetects iOS vs MacOS in the Metal
backend during compilation using the standard define TARGET_OS_IPHONE defined
in the TargetConditionals.h system header, please replace the old
backend-selection defines SOKOL_METAL_MACOS and SOKOL_METAL_IOS with
**SOKOL_METAL**

- **20-Apr-2018**: 3 new context-switching functions have been added
to make it possible to use sokol together with applications that
use multiple GL contexts. On D3D11 and Metal, the functions are currently
empty. See the new section ```WORKING WITH CONTEXTS``` in the sokol_gfx.h
header documentation, and the new sample [multiwindow-glfw](https://github.com/floooh/sokol-samples/blob/master/glfw/multiwindow-glfw.c)

- **31-Jan-2018**: The vertex layout declaration in sg\_pipeline\_desc had
some fairly subtle flaws and has been changed to work like Metal or Vulkan.
The gist is that the vertex-buffer-layout properties (vertex stride,
vertex-step-rate and -step-function for instancing) is now defined in a
separate array from the vertex attributes. This removes some brittle backend
code which tries to guess the right vertex attribute slot if no attribute
names are given, and which was wrong for shader-code-generation pipelines
which reorder the vertex attributes (I stumbled over this when porting the
Oryol Gfx module over to sokol-gfx). Some code samples:

```c
// a complete vertex layout declaration with a single input buffer
// with two vertex attributes
sg_pipeline pip = sg_make_pipeline(&(sg_pipeline_desc){
    .layout = {
        .buffers = {
            [0] = {
                .stride = 20,
                .step_func = SG_VERTEXSTEP_PER_VERTEX,
                .step_rate = 1
            }
        },
        .attrs = {
            [0] = {
                .name = "pos",
                .offset = 0,
                .format = SG_VERTEXFORMAT_FLOAT3,
                .buffer_index = 0
            },
            [1] = {
                .name = "uv",
                .offset = 12,
                .format = SG_VERTEXFORMAT_FLOAT2,
                .buffer_index = 0
            }
        }
    },
    ...
});

// if the vertex layout has no gaps, we can get rid of the strides and offsets:
sg_pipeline pip = sg_make_pipeline(&(sg_pipeline_desc){
    .layout = {
        .buffers = {
            [0] = {
                .step_func = SG_VERTEXSTEP_PER_VERTEX,
                .step_rate=1
            }
        },
        .attrs = {
            [0] = {
                .name = "pos",
                .format = SG_VERTEXFORMAT_FLOAT3,
                .buffer_index = 0
            },
            [1] = {
                .name = "uv",
                .format = SG_VERTEXFORMAT_FLOAT2,
                .buffer_index = 0
            }
        }
    },
    ...
});

// we can also get rid of the other default-values, which leaves buffers[0]
// as all-defaults, so it can disappear completely:
sg_pipeline pip = sg_make_pipeline(&(sg_pipeline_desc){
    .layout = {
        .attrs = {
            [0] = { .name = "pos", .format = SG_VERTEXFORMAT_FLOAT3 },
            [1] = { .name = "uv", .format = SG_VERTEXFORMAT_FLOAT2 }
        }
    },
    ...
});

// and finally on GL3.3 and Metal and we don't need the attribute names
// (on D3D11, a semantic name and index must be provided though)
sg_pipeline pip = sg_make_pipeline(&(sg_pipeline_desc){
    .layout = {
        .attrs = {
            [0] = { .format = SG_VERTEXFORMAT_FLOAT3 },
            [1] = { .format = SG_VERTEXFORMAT_FLOAT2 }
        }
    },
    ...
});
```

Please check the sample code in https://github.com/floooh/sokol-samples for
more examples!

Enjoy!
