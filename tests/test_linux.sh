#!/usr/bin/env bash
set -e
source test_common.sh
prepare
build linux_gl_debug SOKOL_GLCORE33 Debug
build linux_gl_release SOKOL_GLCORE33 Release
build linux_gles3_debug SOKOL_GLES3 Debug
build linux_gles3_release SOKOL_GLES3 Release
build_force_egl linux_gl_egl_debug SOKOL_GLCORE33 Debug
build_force_egl linux_gl_egl_release SOKOL_GLCORE33 Release
runtest linux_gl_debug

