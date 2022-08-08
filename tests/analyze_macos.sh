set -e
source test_common.sh
prepare

analyze macos_metal_analyze SOKOL_METAL Debug
analyze macos_gl_analyze SOKOL_METAL Debug

analyze_arc macos_arc_metal_analyze SOKOL_GLCORE33 Debug
analyze_arc macos_arc_gl_analyze SOKOL_GLCORE33 Debug

