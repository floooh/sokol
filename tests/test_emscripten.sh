#!/usr/bin/env bash
set -e
source test_common.sh
setup_emsdk
build emsc_webgl2_debug emsc_webgl2_debug
build emsc_webgl2_release emsc_webgl2_release
build emsc_webgl1_debug emsc_webgl1_debug
build emsc_webgl1_release emsc_webgl1_release
