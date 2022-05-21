#!/usr/bin/env bash
set -e
source test_common.sh
prepare
build linux_gl SOKOL_GLCORE33
runtest linux_gl

