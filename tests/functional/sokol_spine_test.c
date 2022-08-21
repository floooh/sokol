//------------------------------------------------------------------------------
//  sokol_spine_test.c
//------------------------------------------------------------------------------
#include "sokol_gfx.h"
#define SOKOL_SPINE_IMPL
#include "spine/spine.h"
#include "sokol_spine.h"
#include "utest.h"

#define T(b) EXPECT_TRUE(b)

static void init() {
    sg_setup(&(sg_desc){0});
    sspine_setup(&(sspine_desc){0});
}

static void shutdown() {
    sspine_shutdown();
    sg_shutdown();
}

// NOTE: this guarantees that the data is zero terminated (the returned size doesn't count the zero!)
static sspine_range load_data(const char* path) {
    assert(path);
    FILE* fp = fopen(path, "rb");
    assert(fp);
    fseek(fp, 0, SEEK_END);
    const size_t size = (size_t)ftell(fp);
    fseek(fp, 0, SEEK_SET);
    // room for terminating zero
    const size_t alloc_size = size + 1;
    uint8_t* ptr = (uint8_t*)malloc(alloc_size);
    memset(ptr, 0, alloc_size);
    fread(ptr, size, 1, fp);
    fclose(fp);
    return (sspine_range) { .ptr = ptr, .size = size };
}

static void free_data(sspine_range r) {
    free((void*)r.ptr);
}

static sspine_atlas create_atlas(void) {
    sspine_range atlas_data = load_data("spineboy.atlas");
    sspine_atlas atlas = sspine_make_atlas(&(sspine_atlas_desc){
        .data = atlas_data
    });
    free_data(atlas_data);
    return atlas;
}

static sspine_skeleton create_skeleton_json(sspine_atlas atlas) {
    sspine_range skeleton_json_data = load_data("spineboy-pro.json");
    sspine_skeleton skeleton = sspine_make_skeleton(&(sspine_skeleton_desc){
        .atlas = atlas,
        .json_data = (const char*)skeleton_json_data.ptr
    });
    free_data(skeleton_json_data);
    return skeleton;
}

UTEST(sokol_spine, default_init_shutdown) {
    // FIXME!
    T(true);
}

UTEST(sokol_spine, make_destroy_atlas_ok) {
    init();
    sspine_atlas atlas = create_atlas();
    T(sspine_get_atlas_resource_state(atlas) == SSPINE_RESOURCESTATE_VALID);
    T(sspine_atlas_valid(atlas));
    sspine_destroy_atlas(atlas);
    T(sspine_get_atlas_resource_state(atlas) == SSPINE_RESOURCESTATE_INVALID);
    T(!sspine_atlas_valid(atlas))
    shutdown();
}

UTEST(sokol_spine, make_atlas_fail_no_data) {
    init();
    sspine_atlas atlas = sspine_make_atlas(&(sspine_atlas_desc){0});
    T(atlas.id != SSPINE_INVALID_ID);
    T(sspine_get_atlas_resource_state(atlas) == SSPINE_RESOURCESTATE_FAILED);
    T(!sspine_atlas_valid(atlas));
    shutdown();
}

// an invalid atlas must return zero number of images
UTEST(sokol_spine, failed_atlas_no_images) {
    init();
    sspine_atlas atlas = sspine_make_atlas(&(sspine_atlas_desc){0});
    T(atlas.id != SSPINE_INVALID_ID);
    T(!sspine_atlas_valid(atlas));
    T(sspine_get_num_images(atlas) == 0);
    shutdown();

}

// NOTE: spine-c doesn't detect wrong/corrupt atlas file data, so we can't test for that

UTEST(sokol_spine, atlas_image_info) {
    init();
    sspine_atlas atlas = create_atlas();
    T(sspine_atlas_valid(atlas));
    T(sspine_get_num_images(atlas) == 1);
    const sspine_image_info img_info = sspine_get_image_info(atlas, 0);
    T(img_info.image.id != SG_INVALID_ID);
    T(sg_query_image_state(img_info.image) == SG_RESOURCESTATE_ALLOC);
    T(strcmp(img_info.filename, "spineboy.png") == 0);
    T(img_info.min_filter == SG_FILTER_LINEAR);
    T(img_info.mag_filter == SG_FILTER_LINEAR);
    T(img_info.wrap_u == SG_WRAP_MIRRORED_REPEAT);
    T(img_info.wrap_v == SG_WRAP_MIRRORED_REPEAT);
    T(img_info.width == 1024);
    T(img_info.height == 256);
    shutdown();
}

UTEST(sokol_spine, make_destroy_skeleton_ok) {
    init();
    sspine_skeleton skeleton = create_skeleton_json(create_atlas());
    T(sspine_get_skeleton_resource_state(skeleton) == SSPINE_RESOURCESTATE_VALID);
    T(sspine_skeleton_valid(skeleton));
    sspine_destroy_skeleton(skeleton);
    T(sspine_get_skeleton_resource_state(skeleton) == SSPINE_RESOURCESTATE_INVALID);
    T(!sspine_skeleton_valid(skeleton));
    shutdown();
}

UTEST(sokol_spine, make_skeleton_fail_no_data) {
    init();
    sspine_atlas atlas = create_atlas();
    sspine_skeleton skeleton = sspine_make_skeleton(&(sspine_skeleton_desc){
        .atlas = atlas
    });
    T(sspine_get_skeleton_resource_state(skeleton) == SSPINE_RESOURCESTATE_FAILED);
    T(!sspine_skeleton_valid(skeleton));
    shutdown();
}

UTEST(sokol_spine, make_skeleton_fail_no_atlas) {
    init();
    sspine_range skeleton_json_data = load_data("spineboy-pro.json");
    sspine_skeleton skeleton = sspine_make_skeleton(&(sspine_skeleton_desc){
        .json_data = (const char*)skeleton_json_data.ptr
    });
    free_data(skeleton_json_data);
    T(sspine_get_skeleton_resource_state(skeleton) == SSPINE_RESOURCESTATE_FAILED);
    T(!sspine_skeleton_valid(skeleton));
    shutdown();
}

UTEST(sokol_spine, make_skeleton_fail_with_failed_atlas) {
    init();
    sspine_atlas atlas = sspine_make_atlas(&(sspine_atlas_desc){0});
    T(sspine_get_atlas_resource_state(atlas) == SSPINE_RESOURCESTATE_FAILED);
    sspine_skeleton skeleton = create_skeleton_json(atlas);
    T(sspine_get_skeleton_resource_state(skeleton) == SSPINE_RESOURCESTATE_FAILED);
    T(!sspine_skeleton_valid(skeleton));
    shutdown();
}

UTEST(sokol_spine, make_skeleton_fail_corrupt_data) {
    init();
    sspine_atlas atlas = create_atlas();
    const char* invalid_json_data = "This is not valid JSON!";
    sspine_skeleton skeleton = sspine_make_skeleton(&(sspine_skeleton_desc){
        .atlas = atlas,
        .json_data = (const char*)invalid_json_data,
    });
    T(sspine_get_skeleton_resource_state(skeleton) == SSPINE_RESOURCESTATE_FAILED);
    sspine_destroy_skeleton(skeleton);
    T(sspine_get_skeleton_resource_state(skeleton) == SSPINE_RESOURCESTATE_INVALID);
    shutdown();
}

// FIXME: test for binary skeleton data
