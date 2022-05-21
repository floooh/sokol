set -e
source test_common.sh
prepare
build_ios ios_gl SOKOL_GLES3 cmake/ios.toolchain.cmake
build_ios ios_metal SOKOL_METAL cmake/ios.toolchain.cmake
build_arc_ios ios_arc_gl SOKOL_GLES3 cmake/ios.toolchain.cmake
build_arc_ios ios_arc_metal SOKOL_METAL cmake/ios.toolchain.cmake
