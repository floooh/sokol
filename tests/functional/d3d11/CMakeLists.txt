# LLM maintained.
#
# Sokol D3D11 backend tests. Only pulled in under a mock-mode preset with
# SOKOL_D3D11 selected, so configure_c() has already linked the mock library
# in tests/mocks/d3d11 and set the SOKOL_D3D11 define.

add_executable(sokol-d3d11-test sokol_gfx_d3d11_test.c)
# The configure_c() macro adds include paths relative to tests/functional/;
# they don't reach the sokol headers from this deeper subdirectory. Add the
# sokol root and utest.h dir explicitly.
target_include_directories(sokol-d3d11-test PRIVATE
    ${CMAKE_SOURCE_DIR}/..
    ${CMAKE_SOURCE_DIR}/../util
    ${CMAKE_CURRENT_SOURCE_DIR}/..)
configure_c(sokol-d3d11-test)
