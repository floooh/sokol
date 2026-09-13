# LLM maintained.
#
# Sokol GL backend tests. Only pulled in under a mock-mode preset with
# SOKOL_GLCORE or SOKOL_GLES3 selected, so configure_c() has already linked
# the mock library in tests/mocks/gl and set the backend define.
#
# The GL version is a compile-time choice, so one executable is built per
# supported version, all from the same source file.

if (SOKOL_BACKEND STREQUAL SOKOL_GLCORE)
    set(gl_versions 410 430)
    set(gl_names gl41 gl43)
else()
    set(gl_versions 300 310 320)
    set(gl_names gles30 gles31 gles32)
endif()

list(LENGTH gl_versions num_gl_versions)
math(EXPR last_gl_version "${num_gl_versions} - 1")
foreach (i RANGE ${last_gl_version})
    list(GET gl_versions ${i} gl_version)
    list(GET gl_names ${i} gl_name)
    set(target sokol-${gl_name}-test)
    add_executable(${target} sokol_gfx_gl_test.c)
    target_compile_definitions(${target} PRIVATE GL_MOCK_VERSION=${gl_version})
    # The configure_c() macro adds include paths relative to tests/functional/;
    # they don't reach the sokol headers from this deeper subdirectory. Add the
    # sokol root and utest.h dir explicitly.
    target_include_directories(${target} PRIVATE
        ${CMAKE_SOURCE_DIR}/..
        ${CMAKE_SOURCE_DIR}/../util
        ${CMAKE_CURRENT_SOURCE_DIR}/..)
    configure_c(${target})
endforeach()
