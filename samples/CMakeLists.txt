# Include dcimgui
message(STATUS "Fetching dcimgui...")
FetchContent_Declare(
  dcimgui
  GIT_REPOSITORY https://github.com/floooh/dcimgui
  GIT_TAG d5fb4e3d27b79062dc5981db3631dadc4f204654
)
set(GLFW_BUILD_DOCS OFF)
set(GLFW_BUILD_EXAMPLES OFF)
set(GLFW_BUILD_TESTS OFF)
set(GLFW_BUILD_WAYLAND OFF)
set(GLFW_INSTALL OFF)
FetchContent_MakeAvailable(dcimgui)
message(STATUS "dcimgui ready at: ${dcimgui_SOURCE_DIR}")

set(DCIMGUI_SOURCES 
  ${dcimgui_SOURCE_DIR}/src/cimgui.cpp
  ${dcimgui_SOURCE_DIR}/src/cimgui_internal.cpp
  ${dcimgui_SOURCE_DIR}/src/imgui_demo.cpp
  ${dcimgui_SOURCE_DIR}/src/imgui_draw.cpp
  ${dcimgui_SOURCE_DIR}/src/imgui_tables.cpp
  ${dcimgui_SOURCE_DIR}/src/imgui_widgets.cpp
  ${dcimgui_SOURCE_DIR}/src/imgui.cpp
  
)
add_library(dcimgui SHARED ${DCIMGUI_SOURCES})
target_include_directories(dcimgui PUBLIC ${dcimgui_SOURCE_DIR}/src)
if ((${CMAKE_CXX_COMPILER_ID} MATCHES "Clang") OR (${CMAKE_CXX_COMPILER_ID} MATCHES "GNU"))
    target_compile_options(dcimgui PUBLIC -Wno-return-type-c-linkage -Wno-unused-function)
elseif (MSVC)
    target_compile_options(dcimgui PUBLIC /wd4190)
endif()


add_executable(triangle-bufferless-sapp sapp_turbo/triangle-bufferless-sapp.c)
target_compile_definitions(triangle-bufferless-sapp PRIVATE SOKOL_GLCORE=1)
target_link_libraries(triangle-bufferless-sapp PRIVATE sokol_turbo dcimgui)