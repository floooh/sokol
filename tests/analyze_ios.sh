set -e
source test_common.sh
prepare

analyze_ios ios_metal_analyze SOKOL_METAL Debug
analyze_ios ios_gl_analyze SOKOL_GLES3 Debug

analyze_arc_ios ios_arc_metal_analyze SOKOL_METAL Debug
analyze_arc_ios ios_arc_gl_analyze SOKOL_GLES3 Debug
