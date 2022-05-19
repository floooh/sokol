cmake_minimum_required(VERSION 3.10)
project(sokol-test)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 11)

include_directories(ext)

# SOKOL_GLCORE33, SOKOL_GLES2, SOKOL_GLES3, SOKOL_D3D11, SOKOL_METAL, SOKOL_WGPU, SOKOL_DUMMY
option(SOKOL_BACKEND "Select 3D backend API" SOKOL_GLCORE33)

if (CMAKE_SYSTEM_NAME STREQUAL Emscripten)
    set(EMSCRIPTEN 1)
elseif (CMAKE_SYSTEM_NAME STREQUAL iOS)
    set(OSX_IOS 1)
elseif (CMAKE_SYSTEM_NAME STREQUAL Android)
    set(ANDROID 1)
elseif (CMAKE_SYSTEM_NAME STREQUAL Linux)
    set(LINUX 1)
elseif (CMAKE_SYSTEM_NAME STREQUAL Darwin)
    set(OSX_MACOS 1)
elseif (CMAKE_SYSTEM_NAME STREQUAL WindowsStore)
    set(UWP 1)
elseif (CMAKE_SYSTEM_NAME STREQUAL Windows)
    set(WINDOWS 1)
endif()

if (CMAKE_CXX_COMPILER_ID MATCHES "MSVC")
    set(c_flags /W4 /WX)
    set(cxx_flags /W4 /WX /EHsc)
else()
    set(c_flags -Wall -Wextra -Werror -Wsign-conversion)
    set(cxx_flags -Wall -Wextra -Werror -Wsign-conversion -fno-rtti -fno-exceptions)
endif()

# FIXME: Android, iOS, UWP
if (EMSCRIPTEN)
    set(CMAKE_EXECUTABLE_SUFFIX ".html")
    set(link_flags -sNO_FILESYSTEM=1 -sASSERTIONS=0 -sMALLOC=emmalloc --closure=1)
elseif (OSX_IOS)
    set(exe_type MACOSX_BUNDLE)
elseif (ANDROID)
    # FIXME
elseif (LINUX)
    set(THREADS_PREFER_PTHREAD_FLAG ON)
    find_package(Threads REQUIRED)
    set(system_libs X11 Xi Xcursor GL asound dl m Threads::Threads)
elseif (OSX_MACOS)
    set(exe_type MACOSX_BUNDLE)
    set(system_libs "-framework QuartzCore" "-framework Cocoa" "-framework AudioToolbox")
    if (SOKOL_BACKEND STREQUAL SOKOL_METAL)
        set(system_libs ${system_libs} "-framework MetalKit" "-framework Metal")
    else()
        set(system_libs ${system_libs} "-framework OpenGL")
    endif()
elseif (UWP)
    set(exe_type WIN32)
elseif (WINDOWS)
    set(exe_type WIN32)
endif()

macro(configure_common target)
    target_compile_definitions(${target} PRIVATE ${SOKOL_BACKEND})
    target_link_options(${target} PRIVATE ${link_flags})
    target_link_libraries(${target} PRIVATE ${system_libs})
    target_include_directories(${target} PRIVATE ../.. ../../util)

endmacro()

macro(configure_c target)
    configure_common(${target})
    target_compile_options(${target} PRIVATE ${cxx_flags})
    if (OSX_MACOS OR OSX_IOS)
        target_compile_options(${target} PRIVATE -x objective-c)
    endif()
endmacro()

macro(configure_cxx target)
    configure_common(${target})
    target_compile_options(${target} PRIVATE ${c_flags})
    if (OSX_MACOS OR OSX_IOS)
        target_compile_options(${target} PRIVATE -x objective-c++)
    endif()
endmacro()

#--- cimgui
add_library(cimgui
    ext/fips-cimgui/cimgui/cimgui.cpp
    ext/fips-cimgui/cimgui/imgui/imgui.cpp
    ext/fips-cimgui/cimgui/imgui/imgui_demo.cpp
    ext/fips-cimgui/cimgui/imgui/imgui_draw.cpp
    ext/fips-cimgui/cimgui/imgui/imgui_tables.cpp
    ext/fips-cimgui/cimgui/imgui/imgui_widgets.cpp)
target_include_directories(cimgui PUBLIC ext/fips-cimgui)

add_library(imgui
    ext/fips-cimgui/cimgui/imgui/imgui.cpp
    ext/fips-cimgui/cimgui/imgui/imgui_demo.cpp
    ext/fips-cimgui/cimgui/imgui/imgui_draw.cpp
    ext/fips-cimgui/cimgui/imgui/imgui_tables.cpp
    ext/fips-cimgui/cimgui/imgui/imgui_widgets.cpp)
target_include_directories(imgui PUBLIC ext/fips-cimgui/cimgui/imgui)

add_subdirectory(compile)