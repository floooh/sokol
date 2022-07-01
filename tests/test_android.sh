#!/usr/bin/env bash
set -e
source test_common.sh
prepare
setup_android
build_android android_debug SOKOL_GLES3 Debug
build_android android_release SOKOL_GLES3 Release
