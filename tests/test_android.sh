#!/usr/bin/env bash
set -e
source tests/test_common.sh
setup_android
build android_debug android_debug
build android_release android_release
