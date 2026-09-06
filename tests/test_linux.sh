#!/usr/bin/env bash
set -e
source test_common.sh
build linux_gl_debug linux_gl_debug
build linux_gl_release linux_gl_release
build linux_vulkan_debug linux_vulkan_debug
build linux_vulkan_release linux_vulkan_release
build linux_gles3_debug linux_gles3_debug
build linux_gles3_release linux_gles3_release
build linux_gl_egl_debug linux_gl_egl_debug
build linux_gl_egl_release linux_gl_egl_release
# Backend-agnostic preset -- builds the sokol-gfx D3D11 backend against the
# mock library in tests/mocks/d3d11.
build d3d11_mock_debug d3d11_mock_debug
build d3d11_mock_release d3d11_mock_release
runtest linux_gl_debug
runtest d3d11_mock_debug sokol-d3d11-test
