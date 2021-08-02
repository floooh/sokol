get_filename_component(LIBRARY_NAME "${CMAKE_CURRENT_SOURCE_DIR}" NAME)
string(REPLACE " " "_" LIBRARY_NAME "${LIBRARY_NAME}")

set(Header_Files
        "sokol_color.h"
        "sokol_debugtext.h"
        "sokol_fontstash.h"
        "sokol_gfx_imgui.h"
        "sokol_gl.h"
        "sokol_imgui.h"
        "sokol_memtrack.h"
        "sokol_nuklear.h"
        "sokol_shape.h")

source_group("Header Files" FILES "${Header_Files}")

add_library("${LIBRARY_NAME}" INTERFACE "${Header_Files}")

target_include_directories(
        "${LIBRARY_NAME}"
        INTERFACE
        "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>"
        "$<INSTALL_INTERFACE:include>"
)

target_link_libraries("${LIBRARY_NAME}" INTERFACE "${PROJECT_LOWER_NAME}_compiler_flags")

#if (CMAKE_SYSTEM_NAME STREQUAL "Darwin")
#  find_library(CORE_FOUNDATION_LIBRARY CoreFoundation ONLY)
#  target_link_libraries("${LIBRARY_NAME}" PUBLIC "${CORE_FOUNDATION_LIBRARY}")
#endif ()

set_target_properties(
        "${LIBRARY_NAME}"
        PROPERTIES
        LINKER_LANGUAGE
        C
)

# setup the version numbering
set_property(TARGET "${LIBRARY_NAME}" PROPERTY VERSION "1.0.0")
set_property(TARGET "${LIBRARY_NAME}" PROPERTY SOVERSION "1")

# install rules
set(installable_libs "${LIBRARY_NAME}" "${PROJECT_LOWER_NAME}_compiler_flags")
if (TARGET "${DEPENDANT_LIBRARY}")
  list(APPEND installable_libs "${DEPENDANT_LIBRARY}")
endif ()
install(TARGETS ${installable_libs}
        DESTINATION "bin"
        EXPORT "${LIBRARY_NAME}Targets")
install(FILES "${Header_Files}" DESTINATION "include")
