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

static sspine_range load_data(const char* path) {
    assert(path);
    FILE* fp = fopen(path, "rb");
    assert(fp);
    fseek(fp, 0, SEEK_END);
    size_t size = (size_t)ftell(fp);
    fseek(fp, 0, SEEK_SET);
    void* ptr = malloc(size);
    fread(ptr, size, 1, fp);
    fclose(fp);
    return (sspine_range) { .ptr = ptr, .size = size };
}

static void free_data(sspine_range r) {
    free((void*)r.ptr);
}

UTEST(sokol_spine, default_init_shutdown) {
    // FIXME!
    T(true);
}

UTEST(sokol_spine, make_atlas_fail_no_data) {
    init();
    sspine_atlas atlas = sspine_make_atlas(&(sspine_atlas_desc){0});
    T(atlas.id != SSPINE_INVALID_ID);
    T(sspine_get_atlas_state(atlas) == SSPINE_RESOURCESTATE_FAILED);
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

UTEST(sokol_spine, make_atlas_ok) {
    init();
    sspine_range data = load_data("spineboy.atlas");
    sspine_atlas atlas = sspine_make_atlas(&(sspine_atlas_desc){
        .data = data
    });
    free_data(data);
    T(sspine_get_atlas_state(atlas) == SSPINE_RESOURCESTATE_VALID);
    T(sspine_atlas_valid(atlas));
    shutdown();
}

UTEST(sokol_spine, atlas_image_info) {
    init();
    sspine_range data = load_data("spineboy.atlas");
    sspine_atlas atlas = sspine_make_atlas(&(sspine_atlas_desc){
        .data = data
    });
    free_data(data);
    T(sspine_atlas_valid(atlas));
    T(sspine_get_num_images(atlas) == 1);
    const sspine_image_info img_info = sspine_get_image_info(atlas, 0);
    T(img_info.image.id != SG_INVALID_ID);
    T(sg_query_image_state(img_info.image) == SG_RESOURCESTATE_ALLOC);
    T(strcmp(img_info.filename, "spineboy.png") == 0);
    T(img_info.min_filter == SG_FILTER_LINEAR);
    T(img_info.mag_filter == SG_FILTER_LINEAR);
    shutdown();
}
