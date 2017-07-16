# Sokol

*Work In Progress*

**Sokol (Сокол)**: Russian for Falcon, a smaller and more nimble 
bird of prey than the Eagle (Орёл, Oryol)

Minimalistic header-only platform abstraction libs in C:

- sokol\_gfx.h
- sokol\_input.h
- ...???

These are the internal parts of the Oryol C++ framework 
rewritten in pure C as standalone header-only libs.

Sample code is in a separate repo: https://github.com/floooh/sokol-samples

### Why C:

- easier integration with other languages
- easier integration into other projects
- allows even smaller program binaries than Oryol

Sokol will be a bit less convenient to use than Oryol, but that's ok since
the Sokol headers are intended to be low-level building blocks.

Eventually Oryol will just be a thin C++ layer over Sokol.

### sokol_gfx.h:

- wrapper around GLES2/WebGL, GL3.3, D3D11 and Metal with a 
  'simple modern 3D API style'
- does not handle window creation or 3D API context initialization
- does not provide shader dialect cross-translation