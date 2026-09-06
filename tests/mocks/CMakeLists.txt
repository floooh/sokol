# LLM maintained.
#
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
