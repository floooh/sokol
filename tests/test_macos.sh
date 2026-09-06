set -e
source test_common.sh
build macos_gl_debug macos_gl_debug
build macos_gl_release macos_gl_release
build macos_metal_debug macos_metal_debug
build macos_metal_release macos_metal_release
build macos_arc_gl_debug macos_arc_gl_debug
build macos_arc_gl_release macos_arc_gl_release
build macos_arc_metal_debug macos_arc_metal_debug
build macos_arc_metal_release macos_arc_metal_release
# Backend-agnostic preset -- builds the sokol-gfx D3D11 backend against the
# mock library in tests/mocks/d3d11.
build d3d11_mock_debug d3d11_mock_debug
build d3d11_mock_release d3d11_mock_release
runtest macos_gl_debug
runtest d3d11_mock_debug sokol-d3d11-test
