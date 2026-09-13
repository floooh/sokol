# LLM maintained.
#
# Mock libraries which replace the backend SDKs, so the sokol_gfx.h backends
# can be built and unit-tested without a driver, a device or a window.

# D3D11 mock library. Provides a stubbed d3d11.h + d3dcompiler.h so the
# sokol_gfx.h D3D11 backend can be built and unit-tested on non-Windows hosts.
# Do NOT link this on Windows -- the real Windows SDK is expected there.

add_library(sokol_d3d11_mock STATIC
    d3d11/d3d11_mock.c
    d3d11/d3d11_mock.h
    d3d11/d3d11.h
    d3d11/d3dcompiler.h
    d3d11/windows.h)

# Callers get the mock d3d11.h/d3dcompiler.h from this include dir. When
# compiling sokol_gfx.h with SOKOL_D3D11, this dir must sit ahead of any
# system include path so `#include <d3d11.h>` resolves here.
target_include_directories(sokol_d3d11_mock PUBLIC d3d11)

# The mock implementation itself is tolerant of stricter test flags.
if (NOT CMAKE_CXX_COMPILER_ID MATCHES "MSVC")
    target_compile_options(sokol_d3d11_mock PRIVATE
        -Wall -Wextra -Werror -Wstrict-prototypes)
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
