# LLM maintained.
if (NOT ANDROID)

file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/assets/comsi.s3m DESTINATION ${CMAKE_BINARY_DIR})
file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/assets/comsi.s3m DESTINATION ${CMAKE_BINARY_DIR}/Debug)

set(c_sources
    sokol_log_test.c
    sokol_args_test.c
    sokol_audio_test.c
    sokol_debugtext_test.c
    sokol_fetch_test.c
    sokol_gfx_test.c
    sokol_gl_test.c
    sokol_shape_test.c
    sokol_color_test.c
    sokol_spine_test.c
    sokol_test.c
)
add_executable(sokol-test ${c_sources})
target_link_libraries(sokol-test PUBLIC spine)
configure_c(sokol-test)

# Sokol-gfx D3D11 backend smoke test -- only on non-Windows hosts where the
# tests/mocks/d3d11 stub library replaces the real Windows SDK.
if (NOT WINDOWS)
    add_executable(sokol-gfx-d3d11-test sokol_gfx_d3d11_test.c)
    target_link_libraries(sokol-gfx-d3d11-test PRIVATE sokol_d3d11_mock)
    configure_c(sokol-gfx-d3d11-test)
endif()

endif()
