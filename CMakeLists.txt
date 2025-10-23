#
# project: sokol-turbo
#
cmake_minimum_required(VERSION 3.21)
project(sokol-turbo)
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Platform detection
set(SOKOL_TURBO_PLATFORM "")
if(WIN32)
    set(SOKOL_TURBO_PLATFORM "WIN32")
elseif(APPLE)
    set(SOKOL_TURBO_PLATFORM "MACOS")
elseif(UNIX AND NOT APPLE)
    set(SOKOL_TURBO_PLATFORM "LINUX")
else()
    message(FATAL_ERROR "Unsupported platform")
endif()

message(STATUS "Building for platform: ${SOKOL_TURBO_PLATFORM}")

add_library(sokol_turbo STATIC)

# Platform-specific compiler flags and libraries
if(SOKOL_TURBO_PLATFORM STREQUAL "WIN32")
    target_sources(sokol_turbo PRIVATE sokol-turbo.c)
    target_compile_definitions(sokol_turbo PRIVATE _WIN32 SOKOL_IMPL=1 SOKOL_GLCORE=1)
    
    target_link_libraries(sokol_turbo PUBLIC
        user32
        gdi32
        shell32
        ole32
        winmm
        opengl32
    )

    # Enable Unicode support
    target_compile_definitions(sokol_turbo PRIVATE
        UNICODE
        _UNICODE
    )
elseif(SOKOL_TURBO_PLATFORM STREQUAL "LINUX")
    target_sources(sokol_turbo PRIVATE sokol-turbo.c)
    target_compile_definitions(sokol_turbo PRIVATE __linux__ SOKOL_IMPL=1 SOKOL_GLCORE=1)

    # Find required packages
    find_package(PkgConfig REQUIRED)
    find_package(OpenGL REQUIRED)
    find_package(X11 REQUIRED)
    # pkg_check_modules(X11 REQUIRED x11)
    # pkg_check_modules(Xi REQUIRED Xi)
    # pkg_check_modules(XCursor REQUIRED x11)
    # pkg_check_modules(XRANDR REQUIRED xrandr)
    # pkg_check_modules(XEXT REQUIRED xext)

    target_include_directories(sokol_turbo PRIVATE
        ${X11_INCLUDE_DIRS}
        ${OPENGL_INCLUDE_DIR}
    )
    target_link_libraries(sokol_turbo PRIVATE
        ${X11_LIBRARIES}
        ${X11_Xi_LIB}
        ${X11_Xcursor_LIB}
        ${XRANDR_LIBRARIES}
        ${XEXT_LIBRARIES}
        ${OPENGL_LIBRARIES}
        GL
        pthread
        m
        rt
    )

    # Add compile flags
    target_compile_options(sokol_turbo PRIVATE ${X11_CFLAGS_OTHER})
elseif(SOKOL_TURBO_PLATFORM STREQUAL "MACOS")
    target_sources(sokol_turbo PRIVATE sokol-turbo.m)
    target_compile_definitions(sokol_turbo PRIVATE __APPLE__ SOKOL_IMPL=1 SOKOL_GLCORE=1)
    
    # Enable Objective-C for the .m file
    set_source_files_properties(sokol-turbo.m PROPERTIES
        COMPILE_FLAGS "-x objective-c"
    )

    # Find required frameworks
    find_library(COCOA_FRAMEWORK Cocoa)
    find_library(CARBON_FRAMEWORK Carbon)
    find_library(IOKIT_FRAMEWORK IOKit)
    find_library(COREVIDEO_FRAMEWORK CoreVideo)
    find_library(COREGRAPHICS_FRAMEWORK CoreGraphics)
    find_library(OPENGL_FRAMEWORK OpenGL)

    target_link_libraries(sokol_turbo PUBLIC
        ${COCOA_FRAMEWORK}
        ${CARBON_FRAMEWORK}
        ${IOKIT_FRAMEWORK}
        ${COREVIDEO_FRAMEWORK}
        ${COREGRAPHICS_FRAMEWORK}
        ${OPENGL_FRAMEWORK}
    )
endif()

# Include directories
target_include_directories(sokol_turbo PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
    $<INSTALL_INTERFACE:include>
)

# Compiler warnings
if(MSVC)
    target_compile_options(sokol_turbo PRIVATE /W4)
else()
    target_compile_options(sokol_turbo PRIVATE -Wall -Wextra -pedantic)
endif()

# Example executable (optional)
option(BUILD_EXAMPLES "Build example programs" ON)

if(BUILD_EXAMPLES)
    add_subdirectory("samples")
endif()

# Display configuration summary
message(STATUS "")
message(STATUS "Sokol Turbo Library Configuration Summary:")
message(STATUS "  Platform: ${SOKOL_TURBO_PLATFORM}")
message(STATUS "  Build type: ${CMAKE_BUILD_TYPE}")
# message(STATUS "  Build examples: ${BUILD_EXAMPLES}")
# message(STATUS "  Install prefix: ${CMAKE_INSTALL_PREFIX}")
message(STATUS "")
