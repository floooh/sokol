set -e
source test_common.sh
prepare
build_toolchain ios_gl SOKOL_GLES3 cmake/ios.toolchain.cmake
build_toolchain ios_metal SOKOL_METAL cmake/ios.toolchain.cmake
build_arc_toolchain ios_arc_gl SOKOL_GLES3 cmake/ios.toolchain.cmake
build_arc_toolchain ios_arc_metal SOKOL_METAL cmake/ios.toolchain.cmake
