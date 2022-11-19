# external dependencies

# NOTE FetchContent is so frigging slow that we just run git directly
set(cimgui_dir ${CMAKE_BINARY_DIR}/../_deps/cimgui)
set(spineruntimes_dir ${CMAKE_BINARY_DIR}/../_deps/spineruntimes)

if (IS_DIRECTORY ${cimgui_dir})
    message("### ${cimgui_dir} exists...")
else()
    message("### Fetching cimgui to ${cimgui_dir} (this may take a while...)")
    execute_process(COMMAND git clone --depth=1 --recursive https://github.com/fips-libs/fips-cimgui ${cimgui_dir})
endif()
if (IS_DIRECTORY ${spineruntimes_dir})
    message("### ${spineruntimes_dir} exists...")
else()
    message("### Fetching spine runtimes to ${spineruntimes_dir} (this may take a while...)")
    execute_process(COMMAND git clone --depth=1 --recursive https://github.com/EsotericSoftware/spine-runtimes ${spineruntimes_dir})
endif()

add_library(cimgui
    ${cimgui_dir}/cimgui/cimgui.cpp
    ${cimgui_dir}/cimgui/imgui/imgui.cpp
    ${cimgui_dir}/cimgui/imgui/imgui_demo.cpp
    ${cimgui_dir}/cimgui/imgui/imgui_draw.cpp
    ${cimgui_dir}/cimgui/imgui/imgui_tables.cpp
    ${cimgui_dir}/cimgui/imgui/imgui_widgets.cpp)
target_include_directories(cimgui SYSTEM PUBLIC ${cimgui_dir})

add_library(imgui
    ${cimgui_dir}/cimgui/imgui/imgui.cpp
    ${cimgui_dir}/cimgui/imgui/imgui_demo.cpp
    ${cimgui_dir}/cimgui/imgui/imgui_draw.cpp
    ${cimgui_dir}/cimgui/imgui/imgui_tables.cpp
    ${cimgui_dir}/cimgui/imgui/imgui_widgets.cpp)
target_include_directories(imgui SYSTEM PUBLIC ${cimgui_dir}/cimgui/imgui)

add_library(spine
    ${spineruntimes_dir}/spine-c/spine-c/src/spine/Animation.c
    ${spineruntimes_dir}/spine-c/spine-c/src/spine/AnimationState.c
    ${spineruntimes_dir}/spine-c/spine-c/src/spine/AnimationStateData.c
    ${spineruntimes_dir}/spine-c/spine-c/src/spine/Array.c
    ${spineruntimes_dir}/spine-c/spine-c/src/spine/Atlas.c
    ${spineruntimes_dir}/spine-c/spine-c/src/spine/AtlasAttachmentLoader.c
    ${spineruntimes_dir}/spine-c/spine-c/src/spine/Attachment.c
    ${spineruntimes_dir}/spine-c/spine-c/src/spine/AttachmentLoader.c
    ${spineruntimes_dir}/spine-c/spine-c/src/spine/Bone.c
    ${spineruntimes_dir}/spine-c/spine-c/src/spine/BoneData.c
    ${spineruntimes_dir}/spine-c/spine-c/src/spine/BoundingBoxAttachment.c
    ${spineruntimes_dir}/spine-c/spine-c/src/spine/ClippingAttachment.c
    ${spineruntimes_dir}/spine-c/spine-c/src/spine/Color.c
    ${spineruntimes_dir}/spine-c/spine-c/src/spine/Debug.c
    ${spineruntimes_dir}/spine-c/spine-c/src/spine/Event.c
    ${spineruntimes_dir}/spine-c/spine-c/src/spine/EventData.c
    ${spineruntimes_dir}/spine-c/spine-c/src/spine/IkConstraint.c
    ${spineruntimes_dir}/spine-c/spine-c/src/spine/IkConstraintData.c
    ${spineruntimes_dir}/spine-c/spine-c/src/spine/Json.c
    ${spineruntimes_dir}/spine-c/spine-c/src/spine/Json.h
    ${spineruntimes_dir}/spine-c/spine-c/src/spine/MeshAttachment.c
    ${spineruntimes_dir}/spine-c/spine-c/src/spine/PathAttachment.c
    ${spineruntimes_dir}/spine-c/spine-c/src/spine/PathConstraint.c
    ${spineruntimes_dir}/spine-c/spine-c/src/spine/PathConstraintData.c
    ${spineruntimes_dir}/spine-c/spine-c/src/spine/PointAttachment.c
    ${spineruntimes_dir}/spine-c/spine-c/src/spine/RegionAttachment.c
    ${spineruntimes_dir}/spine-c/spine-c/src/spine/Sequence.c
    ${spineruntimes_dir}/spine-c/spine-c/src/spine/Skeleton.c
    ${spineruntimes_dir}/spine-c/spine-c/src/spine/SkeletonBinary.c
    ${spineruntimes_dir}/spine-c/spine-c/src/spine/SkeletonBounds.c
    ${spineruntimes_dir}/spine-c/spine-c/src/spine/SkeletonClipping.c
    ${spineruntimes_dir}/spine-c/spine-c/src/spine/SkeletonData.c
    ${spineruntimes_dir}/spine-c/spine-c/src/spine/SkeletonJson.c
    ${spineruntimes_dir}/spine-c/spine-c/src/spine/Skin.c
    ${spineruntimes_dir}/spine-c/spine-c/src/spine/Slot.c
    ${spineruntimes_dir}/spine-c/spine-c/src/spine/SlotData.c
    ${spineruntimes_dir}/spine-c/spine-c/src/spine/TransformConstraint.c
    ${spineruntimes_dir}/spine-c/spine-c/src/spine/TransformConstraintData.c
    ${spineruntimes_dir}/spine-c/spine-c/src/spine/Triangulator.c
    ${spineruntimes_dir}/spine-c/spine-c/src/spine/VertexAttachment.c
    ${spineruntimes_dir}/spine-c/spine-c/src/spine/extension.c)
target_include_directories(spine SYSTEM PUBLIC ${spineruntimes_dir}/spine-c/spine-c/include)
if (CMAKE_CXX_COMPILER_ID MATCHES "MSVC")
    target_compile_options(spine PRIVATE /wd4267 /wd4244)   # conversion from 'x' to 'y' possible loss of data
endif()
if (CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    target_compile_options(spine PRIVATE -Wno-shorten-64-to-32)
endif()

file(COPY ${spineruntimes_dir}/examples/spineboy/export/spineboy-pro.json DESTINATION ${CMAKE_BINARY_DIR})
file(COPY ${spineruntimes_dir}/examples/spineboy/export/spineboy-pro.skel DESTINATION ${CMAKE_BINARY_DIR})
file(COPY ${spineruntimes_dir}/examples/spineboy/export/spineboy.atlas DESTINATION ${CMAKE_BINARY_DIR})
file(COPY ${spineruntimes_dir}/examples/spineboy/export/spineboy.png DESTINATION ${CMAKE_BINARY_DIR})

file(COPY ${spineruntimes_dir}/examples/spineboy/export/spineboy-pro.json DESTINATION ${CMAKE_BINARY_DIR}/Debug)
file(COPY ${spineruntimes_dir}/examples/spineboy/export/spineboy-pro.skel DESTINATION ${CMAKE_BINARY_DIR}/Debug)
file(COPY ${spineruntimes_dir}/examples/spineboy/export/spineboy.atlas DESTINATION ${CMAKE_BINARY_DIR}/Debug)
file(COPY ${spineruntimes_dir}/examples/spineboy/export/spineboy.png DESTINATION ${CMAKE_BINARY_DIR}/Debug)

add_library(nuklear nuklear.c)
