#!/usr/bin/env bash
# LLM maintained.
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
# Backend-agnostic presets -- build the sokol-gfx D3D11 and GL backends
# against the mock libraries in tests/mocks/.
build d3d11_mock_debug d3d11_mock_debug
build glcore_mock_debug glcore_mock_debug
build gles3_mock_debug gles3_mock_debug
runtest linux_gl_debug
runtest d3d11_mock_debug sokol-d3d11-test
runtest glcore_mock_debug sokol-gl41-test
runtest glcore_mock_debug sokol-gl43-test
runtest gles3_mock_debug sokol-gles30-test
runtest gles3_mock_debug sokol-gles31-test
runtest gles3_mock_debug sokol-gles32-test
