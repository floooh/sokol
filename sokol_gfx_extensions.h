// Copyright (c) 2020-2022 Thomas Stehle. All rights reserved.
//
// Custom sokol extensions based on https://github.com/MeshGeometry/sokol-ext

#ifndef SOKOL_GFX_EXTENSIONS_INCLUDED

/*
    sokol_gfx_extensions.h
*/
#define SOKOL_GFX_EXTENSIONS_INCLUDED (1)
#include <stdint.h>

#ifndef SOKOL_API_DECL
#if defined(_WIN32) && defined(SOKOL_DLL) && defined(SOKOL_IMPL)
#define SOKOL_API_DECL __declspec(dllexport)
#elif defined(_WIN32) && defined(SOKOL_DLL)
#define SOKOL_API_DECL __declspec(dllimport)
#else
#define SOKOL_API_DECL extern
#endif
#endif

#ifdef __cplusplus
extern "C" {
#endif

// Get backend renderer info
SOKOL_API_DECL const char* sgx_get_backend_renderer();

// Get backend shading language version info
SOKOL_API_DECL const char* sgx_get_backend_shading_language_version();

// Get backend vendor info
SOKOL_API_DECL const char* sgx_get_backend_vendor();

// Get backend version info
SOKOL_API_DECL const char* sgx_get_backend_version();

// Read pixels from the frame buffer
// Beware: assumes an RGBA unsigned byte frame buffer
SOKOL_API_DECL void sgx_read_pixels(int x, int y, int w, int h, void* pixels);

#ifdef __cplusplus
}  // extern "C"
#endif

#endif  // SOKOL_GFX_EXTENSIONS_INCLUDED

/*-- IMPLEMENTATION ----------------------------------------------------------*/
#ifdef SOKOL_IMPL
#define SOKOL_GFX_EXTENSIONS_IMPL_INCLUDED (1)

#include <string.h>  // memset

#ifndef SOKOL_API_IMPL
#define SOKOL_API_IMPL
#endif
#ifndef SOKOL_ASSERT
#include <assert.h>
#define SOKOL_ASSERT(c) assert(c)
#endif
#ifndef _SOKOL_PRIVATE
#if defined(__GNUC__)
#define _SOKOL_PRIVATE __attribute__((unused)) static
#else
#define _SOKOL_PRIVATE static
#endif
#endif

#ifdef _SOKOL_ANY_GL

SOKOL_API_IMPL const char* sgx_get_backend_renderer() {
  return (const char*)glGetString(GL_RENDERER);
}

SOKOL_API_IMPL const char* sgx_get_backend_shading_language_version() {
  return (const char*)glGetString(GL_SHADING_LANGUAGE_VERSION);
}

SOKOL_API_IMPL const char* sgx_get_backend_vendor() {
  return (const char*)glGetString(GL_VENDOR);
}

SOKOL_API_IMPL const char* sgx_get_backend_version() {
  return (const char*)glGetString(GL_VERSION);
}

SOKOL_API_IMPL void sgx_read_pixels(int x, int y, int w, int h, void* pixels) {
  glReadPixels(x, y, w, h, GL_RGBA, GL_UNSIGNED_BYTE, pixels);
}

#elif defined(SOKOL_METAL)

SOKOL_API_IMPL const char* sgx_get_backend_renderer() { return "Metal"; }

SOKOL_API_IMPL const char* sgx_get_backend_shading_language_version() {
  return "n/a";
}

SOKOL_API_IMPL const char* sgx_get_backend_vendor() { return "Apple"; }

SOKOL_API_IMPL const char* sgx_get_backend_version() { return "n/a"; }

SOKOL_API_IMPL void sgx_read_pixels(int x, int y, int w, int h, void* pixels) {
  // Not implemented yet
  (void)x;
  (void)y;
  (void)w;
  (void)h;
  (void)pixels;
}

#endif

#endif  // SOKOL_IMPL
