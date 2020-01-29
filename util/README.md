# Sokol Utility Headers

These are optional utility headers on top of the Sokol headers. Unlike the
'core headers' they are not standalone but depend on other Sokol headers
and sometimes also external libraries.

### What's in here:

- **sokol_imgui.h**: implements a renderer for [Dear ImGui](https://github.com/ocornut/imgui) on top of sokol_gfx.h and sokol_app.h (the latter being optional if you do your own input-forwarding to ImGui), the implementation
can be compiled as C++ or C.
- **sokol_gfx_imgui.h**: a debug-inspection UI for sokol_gfx.h, this hooks into the sokol-gfx API and lets you inspect resource objects and captured API calls
- **sokol_gl.h**: an OpenGL 1.x style immediate-mode rendering API
on top of sokol_gfx.h
- **sokol_fontstash.h**: a renderer for [fontstash.h](https://github.com/memononen/fontstash) on
on top of sokol_gl.h

See the embedded header-documentation for build- and usage-details.
