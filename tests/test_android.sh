#!/usr/bin/env bash
set -e
source test_common.sh
prepare
setup_android
build_android android SOKOL_GLES3
