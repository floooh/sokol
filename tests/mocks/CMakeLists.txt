# LLM maintained.
#
# Mock libraries which replace the backend SDKs, so the sokol_gfx.h backends
# can be built and unit-tested without a driver, a device or a window.

# D3D11 mock library. Provides a stubbed d3d11.h + d3dcompiler.h so the
# sokol_gfx.h D3D11 backend can be built and unit-tested without a device.
# Works on any host, Windows included: there the mock headers shadow the
# Windows SDK ones, so no SDK header is read and no d3d11.lib/dxgi.lib is
# linked.

add_library(sokol_d3d11_mock STATIC
    d3d11/d3d11_mock.c
    d3d11/d3d11_mock.h
    d3d11/d3d11.h
    d3d11/d3dcompiler.h
    d3d11/windef.h
    d3d11/winbase.h
    d3d11/windows.h)

# Callers get the mock d3d11.h/d3dcompiler.h from this include dir. BEFORE
# keeps it ahead of every other include path, so `#include <d3d11.h>` cannot
# reach the Windows SDK.
target_include_directories(sokol_d3d11_mock BEFORE PUBLIC d3d11)

# sokol_gfx.h has `#pragma comment (lib, "d3d11")` / `"dxgi"` in its D3D11
# section. Those must not take effect in mock mode -- the mock replaces the
# whole D3D11 API, no system graphics lib may be linked.
if (CMAKE_CXX_COMPILER_ID MATCHES "MSVC")
    target_link_options(sokol_d3d11_mock INTERFACE
        /NODEFAULTLIB:d3d11.lib
        /NODEFAULTLIB:dxgi.lib
        /NODEFAULTLIB:d3dcompiler.lib)
endif()

# The mock implementation itself is tolerant of stricter test flags.
if (NOT CMAKE_CXX_COMPILER_ID MATCHES "MSVC")
    target_compile_options(sokol_d3d11_mock PRIVATE
        -Wall -Wextra -Werror -Wstrict-prototypes)
endif()

# Metal mock library. Provides a stubbed Metal.framework (and the
# CAMetalDrawable part of QuartzCore) so the sokol_gfx.h Metal backend can be
# built and unit-tested without a device, a driver or a window. macOS only,
# because the mock is written in Objective-C against the real Foundation.
if (CMAKE_SYSTEM_NAME STREQUAL Darwin)
    add_library(sokol_metal_mock STATIC
        metal/metal_mock.m
        metal/metal_mock.h
        metal/include/Metal/Metal.h
        metal/include/QuartzCore/CoreAnimation.h)

    # Callers get the mock Metal.h / CoreAnimation.h from this include dir.
    # BEFORE keeps it ahead of every other include path, so `#import
    # <Metal/Metal.h>` cannot reach the real framework headers.
    target_include_directories(sokol_metal_mock BEFORE PUBLIC metal/include)
    target_include_directories(sokol_metal_mock PUBLIC metal)

    # Foundation is the real system framework, only Metal itself is mocked.
    target_link_libraries(sokol_metal_mock PUBLIC "-framework Foundation")

    # The mock manages its own object lifetimes, so it is always built
    # without ARC, no matter how the test target is built.
    target_compile_options(sokol_metal_mock PRIVATE
        -fno-objc-arc -Wall -Wextra -Werror -Wstrict-prototypes)
endif()

# GL 4.x / GLES 3.x mock library. Provides a stubbed gl.h plus linkable
# implementations of every GL function which sokol_gfx.h calls, so the GL
# backend can be built and unit-tested without a GL driver or a window.
#
# This is an INTERFACE library, not a static one: the selected GL version
# (GL_MOCK_VERSION) and the backend define (SOKOL_GLCORE / SOKOL_GLES3) are
# per-test-target settings, so gl_mock.c must be compiled once per target
# with that target's flags.
add_library(sokol_gl_mock INTERFACE)
target_include_directories(sokol_gl_mock INTERFACE gl)
target_sources(sokol_gl_mock INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/gl/gl_mock.c)
