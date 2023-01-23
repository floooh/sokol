#!/usr/bin/env bash
set -e
source test_common.sh
setup_android
build android_debug android_debug
build android_release android_release
build android_sles_debug android_sles_debug
build android_sles_release android_sles_release
