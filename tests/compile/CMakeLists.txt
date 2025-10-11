set(c_sources
    sokol_app.c
    sokol_glue.c
    sokol_gfx.c
    sokol_time.c
    sokol_args.c
    sokol_audio.c
    sokol_debugtext.c
    sokol_gl.c
    sokol_fontstash.c
    sokol_imgui.c
    sokol_gfx_imgui.c
    sokol_shape.c
    sokol_nuklear.c
    sokol_color.c
    sokol_spine.c
    sokol_log.c
    sokol_main.c)
if (NOT ANDROID)
    set(c_sources ${c_sources} sokol_fetch.c)
endif()

set(cxx_sources
    sokol_app.cc
    sokol_glue.cc
    sokol_gfx.cc
    sokol_time.cc
    sokol_args.cc
    sokol_audio.cc
    sokol_debugtext.cc
    sokol_gl.cc
    sokol_fontstash.cc
    sokol_imgui.cc
    sokol_gfx_imgui.cc
    sokol_shape.cc
    sokol_color.cc
    sokol_spine.cc
    sokol_log.cc
    sokol_main.cc)
if (NOT ANDROID)
    set(cxx_sources ${cxx_sources} sokol_fetch.cc)
endif()

if (ANDROID)
    add_library(sokol-compiletest-c SHARED ${c_sources})
else()
    add_executable(sokol-compiletest-c ${exe_type} ${c_sources})
    add_executable(sokol-compiletest-nosokolapp-c sokol_gfx.c sokol_imgui_nosokolapp.c sokol_nuklear_nosokolapp.c)
    target_link_libraries(sokol-compiletest-nosokolapp-c PUBLIC imgui nuklear)
    configure_c(sokol-compiletest-nosokolapp-c)
endif()
target_link_libraries(sokol-compiletest-c PUBLIC imgui nuklear spine)
configure_c(sokol-compiletest-c)

if (ANDROID)
    add_library(sokol-compiletest-cxx SHARED ${cxx_sources})
else()
    add_executable(sokol-compiletest-cxx ${exe_type} ${cxx_sources})
    add_executable(sokol-compiletest-nosokolapp-cxx sokol_gfx.cc sokol_imgui_nosokolapp.cc)
    target_link_libraries(sokol-compiletest-nosokolapp-cxx PUBLIC imgui)
    configure_cxx(sokol-compiletest-nosokolapp-cxx)
endif()
target_link_libraries(sokol-compiletest-cxx PUBLIC imgui nuklear spine)
configure_cxx(sokol-compiletest-cxx)
