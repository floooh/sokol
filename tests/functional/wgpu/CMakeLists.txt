# LLM maintained.
#
# Sokol WebGPU backend tests. Only pulled in under a mock-mode preset with
# SOKOL_WGPU selected, so configure_c() has already linked the mock library
# in tests/mocks/wgpu and set the SOKOL_WGPU define.

add_executable(sokol-wgpu-test sokol_gfx_wgpu_test.c)
# The configure_c() macro adds include paths relative to tests/functional/;
# they don't reach the sokol headers from this deeper subdirectory. Add the
# sokol root and utest.h dir explicitly.
target_include_directories(sokol-wgpu-test PRIVATE
    ${CMAKE_SOURCE_DIR}/..
    ${CMAKE_SOURCE_DIR}/../util
    ${CMAKE_CURRENT_SOURCE_DIR}/..)
configure_c(sokol-wgpu-test)
