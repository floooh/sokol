# external dependencies
FetchContent_Declare(
    spineruntimes
    GIT_REPOSITORY https://github.com/EsotericSoftware/spine-runtimes
    GIT_SUBMODULES_RECURSE ON
    GIT_SHALLOW ON
    GIT_TAG "4.1"
)
FetchContent_Declare(
    fipscimgui
    GIT_REPOSITORY https://github.com/fips-libs/fips-cimgui
    GIT_SUBMODULES_RECURSE ON
    GIT_SHALLOW ON
)
FetchContent_Populate(fipscimgui)
FetchContent_Populate(spineruntimes)

add_library(cimgui
    ${fipscimgui_SOURCE_DIR}/cimgui/cimgui.cpp
    ${fipscimgui_SOURCE_DIR}/cimgui/imgui/imgui.cpp
    ${fipscimgui_SOURCE_DIR}/cimgui/imgui/imgui_demo.cpp
    ${fipscimgui_SOURCE_DIR}/cimgui/imgui/imgui_draw.cpp
    ${fipscimgui_SOURCE_DIR}/cimgui/imgui/imgui_tables.cpp
    ${fipscimgui_SOURCE_DIR}/cimgui/imgui/imgui_widgets.cpp)
target_include_directories(cimgui SYSTEM PUBLIC ${fipscimgui_SOURCE_DIR})

add_library(imgui
    ${fipscimgui_SOURCE_DIR}/cimgui/imgui/imgui.cpp
    ${fipscimgui_SOURCE_DIR}/cimgui/imgui/imgui_demo.cpp
    ${fipscimgui_SOURCE_DIR}/cimgui/imgui/imgui_draw.cpp
    ${fipscimgui_SOURCE_DIR}/cimgui/imgui/imgui_tables.cpp
    ${fipscimgui_SOURCE_DIR}/cimgui/imgui/imgui_widgets.cpp)
target_include_directories(imgui SYSTEM PUBLIC ${fipscimgui_SOURCE_DIR}/cimgui/imgui)

add_library(spine
    ${spineruntimes_SOURCE_DIR}/spine-c/spine-c/src/spine/Animation.c
    ${spineruntimes_SOURCE_DIR}/spine-c/spine-c/src/spine/AnimationState.c
    ${spineruntimes_SOURCE_DIR}/spine-c/spine-c/src/spine/AnimationStateData.c
    ${spineruntimes_SOURCE_DIR}/spine-c/spine-c/src/spine/Array.c
    ${spineruntimes_SOURCE_DIR}/spine-c/spine-c/src/spine/Atlas.c
    ${spineruntimes_SOURCE_DIR}/spine-c/spine-c/src/spine/AtlasAttachmentLoader.c
    ${spineruntimes_SOURCE_DIR}/spine-c/spine-c/src/spine/Attachment.c
    ${spineruntimes_SOURCE_DIR}/spine-c/spine-c/src/spine/AttachmentLoader.c
    ${spineruntimes_SOURCE_DIR}/spine-c/spine-c/src/spine/Bone.c
    ${spineruntimes_SOURCE_DIR}/spine-c/spine-c/src/spine/BoneData.c
    ${spineruntimes_SOURCE_DIR}/spine-c/spine-c/src/spine/BoundingBoxAttachment.c
    ${spineruntimes_SOURCE_DIR}/spine-c/spine-c/src/spine/ClippingAttachment.c
    ${spineruntimes_SOURCE_DIR}/spine-c/spine-c/src/spine/Color.c
    ${spineruntimes_SOURCE_DIR}/spine-c/spine-c/src/spine/Debug.c
    ${spineruntimes_SOURCE_DIR}/spine-c/spine-c/src/spine/Event.c
    ${spineruntimes_SOURCE_DIR}/spine-c/spine-c/src/spine/EventData.c
    ${spineruntimes_SOURCE_DIR}/spine-c/spine-c/src/spine/IkConstraint.c
    ${spineruntimes_SOURCE_DIR}/spine-c/spine-c/src/spine/IkConstraintData.c
    ${spineruntimes_SOURCE_DIR}/spine-c/spine-c/src/spine/Json.c
    ${spineruntimes_SOURCE_DIR}/spine-c/spine-c/src/spine/Json.h
    ${spineruntimes_SOURCE_DIR}/spine-c/spine-c/src/spine/MeshAttachment.c
    ${spineruntimes_SOURCE_DIR}/spine-c/spine-c/src/spine/PathAttachment.c
    ${spineruntimes_SOURCE_DIR}/spine-c/spine-c/src/spine/PathConstraint.c
    ${spineruntimes_SOURCE_DIR}/spine-c/spine-c/src/spine/PathConstraintData.c
    ${spineruntimes_SOURCE_DIR}/spine-c/spine-c/src/spine/PointAttachment.c
    ${spineruntimes_SOURCE_DIR}/spine-c/spine-c/src/spine/RegionAttachment.c
    ${spineruntimes_SOURCE_DIR}/spine-c/spine-c/src/spine/Sequence.c
    ${spineruntimes_SOURCE_DIR}/spine-c/spine-c/src/spine/Skeleton.c
    ${spineruntimes_SOURCE_DIR}/spine-c/spine-c/src/spine/SkeletonBinary.c
    ${spineruntimes_SOURCE_DIR}/spine-c/spine-c/src/spine/SkeletonBounds.c
    ${spineruntimes_SOURCE_DIR}/spine-c/spine-c/src/spine/SkeletonClipping.c
    ${spineruntimes_SOURCE_DIR}/spine-c/spine-c/src/spine/SkeletonData.c
    ${spineruntimes_SOURCE_DIR}/spine-c/spine-c/src/spine/SkeletonJson.c
    ${spineruntimes_SOURCE_DIR}/spine-c/spine-c/src/spine/Skin.c
    ${spineruntimes_SOURCE_DIR}/spine-c/spine-c/src/spine/Slot.c
    ${spineruntimes_SOURCE_DIR}/spine-c/spine-c/src/spine/SlotData.c
    ${spineruntimes_SOURCE_DIR}/spine-c/spine-c/src/spine/TransformConstraint.c
    ${spineruntimes_SOURCE_DIR}/spine-c/spine-c/src/spine/TransformConstraintData.c
    ${spineruntimes_SOURCE_DIR}/spine-c/spine-c/src/spine/Triangulator.c
    ${spineruntimes_SOURCE_DIR}/spine-c/spine-c/src/spine/VertexAttachment.c
    ${spineruntimes_SOURCE_DIR}/spine-c/spine-c/src/spine/extension.c)
target_include_directories(spine SYSTEM PUBLIC ${spineruntimes_SOURCE_DIR}/spine-c/spine-c/include)
if (CMAKE_CXX_COMPILER_ID MATCHES "MSVC")
    target_compile_options(spine PRIVATE /wd4267 /wd4244)   # conversion from 'x' to 'y' possible loss of data
endif()
if (CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    target_compile_options(spine PRIVATE -Wno-shorten-64-to-32)
endif()

file(COPY ${spineruntimes_SOURCE_DIR}/examples/spineboy/export/spineboy-pro.json DESTINATION ${CMAKE_BINARY_DIR})
file(COPY ${spineruntimes_SOURCE_DIR}/examples/spineboy/export/spineboy-pro.skel DESTINATION ${CMAKE_BINARY_DIR})
file(COPY ${spineruntimes_SOURCE_DIR}/examples/spineboy/export/spineboy.atlas DESTINATION ${CMAKE_BINARY_DIR})
file(COPY ${spineruntimes_SOURCE_DIR}/examples/spineboy/export/spineboy.png DESTINATION ${CMAKE_BINARY_DIR})

file(COPY ${spineruntimes_SOURCE_DIR}/examples/spineboy/export/spineboy-pro.json DESTINATION ${CMAKE_BINARY_DIR}/Debug)
file(COPY ${spineruntimes_SOURCE_DIR}/examples/spineboy/export/spineboy-pro.skel DESTINATION ${CMAKE_BINARY_DIR}/Debug)
file(COPY ${spineruntimes_SOURCE_DIR}/examples/spineboy/export/spineboy.atlas DESTINATION ${CMAKE_BINARY_DIR}/Debug)
file(COPY ${spineruntimes_SOURCE_DIR}/examples/spineboy/export/spineboy.png DESTINATION ${CMAKE_BINARY_DIR}/Debug)

add_library(nuklear nuklear.c)
