# LLM maintained.
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
# Backend-agnostic presets -- build the sokol-gfx D3D11 and GL backends
# against the mock libraries in tests/mocks/.
build d3d11_mock_debug d3d11_mock_debug
build d3d11_mock_release d3d11_mock_release
build glcore_mock_debug glcore_mock_debug
build glcore_mock_release glcore_mock_release
build gles3_mock_debug gles3_mock_debug
build gles3_mock_release gles3_mock_release
runtest macos_gl_debug
runtest d3d11_mock_debug sokol-d3d11-test
runtest glcore_mock_debug sokol-gl41-test
runtest glcore_mock_debug sokol-gl43-test
runtest gles3_mock_debug sokol-gles30-test
runtest gles3_mock_debug sokol-gles31-test
runtest gles3_mock_debug sokol-gles32-test
