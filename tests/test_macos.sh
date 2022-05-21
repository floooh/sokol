set -e
source test_common.sh
prepare
build macos_gl SOKOL_GLCORE33
build macos_metal SOKOL_METAL
build_arc macos_arc_gl SOKOL_GLCORE33
build_arc macos_arc_metal SOKOL_METAL
runtest macos_gl
