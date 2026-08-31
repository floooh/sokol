# LLM maintained.
if (NOT ANDROID)

file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/assets/comsi.s3m DESTINATION ${CMAKE_BINARY_DIR})
file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/assets/comsi.s3m DESTINATION ${CMAKE_BINARY_DIR}/Debug)

# sokol-test bundles util tests (sokol_app.h etc.) whose backend paths are not
# covered by the mock libraries yet -- skip in mock mode.
if (NOT BACKEND_MOCK_ONLY)
    set(c_sources
        sokol_cmdbuf_test.c
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
endif()

# D3D11 backend tests. Only built in mock mode with SOKOL_D3D11 selected.
if (BACKEND_MOCK_ONLY AND SOKOL_BACKEND STREQUAL SOKOL_D3D11)
    add_subdirectory(d3d11)
endif()

endif()
