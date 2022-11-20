#!/usr/bin/env bash
set -e
source test_common.sh

build linux_gl_analyze linux_gl_analyze
build linux_gles3_analyze linux_gles3_analyze
