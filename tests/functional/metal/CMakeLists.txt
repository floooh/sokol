# LLM maintained.
#
# Sokol Metal backend tests. Only pulled in under a mock-mode preset with
# SOKOL_METAL selected, so configure_c() has already linked the mock library
# in tests/mocks/metal and set the SOKOL_METAL define.

add_executable(sokol-metal-test sokol_gfx_metal_test.c)
# The configure_c() macro adds include paths relative to tests/functional/;
# they don't reach the sokol headers from this deeper subdirectory. Add the
# sokol root and utest.h dir explicitly.
target_include_directories(sokol-metal-test PRIVATE
    ${CMAKE_SOURCE_DIR}/..
    ${CMAKE_SOURCE_DIR}/../util
    ${CMAKE_CURRENT_SOURCE_DIR}/..)
configure_c(sokol-metal-test)
