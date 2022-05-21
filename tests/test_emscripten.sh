#!/usr/bin/env bash
set -e
source test_common.sh
prepare
setup_emsdk
build_emsc emsc_webgl2 SOKOL_GLES3
build_emsc emsc_webgl1 SOKOL_GLES2
