set -e
source test_common.sh
prepare
build_ios ios_gl SOKOL_GLES3
build_ios ios_metal SOKOL_METAL
build_arc_ios ios_arc_gl SOKOL_GLES3
build_arc_ios ios_arc_metal SOKOL_METAL
