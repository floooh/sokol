#!/usr/bin/env bash
set -e
source test_common.sh
prepare
build linux_gl_debug SOKOL_GLCORE33 Debug
build linux_gl_release SOKOL_GLCORE33 Release
runtest linux_gl_debug

