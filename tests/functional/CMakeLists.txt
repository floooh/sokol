if (NOT ANDROID)

file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/assets/comsi.s3m DESTINATION ${CMAKE_BINARY_DIR})
file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/assets/comsi.s3m DESTINATION ${CMAKE_BINARY_DIR}/Debug)

set(c_sources
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

endif()
