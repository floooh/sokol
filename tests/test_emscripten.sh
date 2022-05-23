#!/usr/bin/env bash
set -e
source test_common.sh
prepare
setup_emsdk
build_emsc emsc_webgl2_debug SOKOL_GLES3 Debug
build_emsc emsc_webgl2_release SOKOL_GLES3 Release
build_emsc emsc_webgl1_debug SOKOL_GLES2 Debug
build_emsc emsc_webgl1_release SOKOL_GLES2 Release
