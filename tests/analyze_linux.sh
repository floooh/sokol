#!/usr/bin/env bash
set -e
source test_common.sh
prepare

analyze linux_gl_analyze SOKOL_GLCORE33 Debug
analyze linux_gles3_analyze SOKOL_GLES3 Debug