## Updates

- **01-Apr-2021**: some fixes in sokol_app.h's iOS backend:
    - In the iOS Metal backend, high-dpi vs low-dpi works again. Some time
    ago (around iOS 12.x) MTKView started to ignore the contentScaleFactor
    property, which lead to sokol_app.h always setting up a HighDPI
    framebuffer even when sapp_desc.high_dpi wasn't set. The fix is to set
    the MTKView's drawableSize explicitely now.
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
as Objective-C when targetting iOS, also note that a new framework must be linked: ```AVFoundation```.
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
