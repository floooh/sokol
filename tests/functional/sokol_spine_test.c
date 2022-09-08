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

// NOTE: this guarantees that the data is zero terminated because the loaded data
// might either be binary or text (the zero sentinel is NOT counted in the returned size)
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

static sspine_instance create_instance() {
    return sspine_make_instance(&(sspine_instance_desc){
        .skeleton = create_skeleton_json(create_atlas()),
    });
}

static sspine_skeleton create_skeleton_binary(sspine_atlas atlas) {
    sspine_range skeleton_binary_data = load_data("spineboy-pro.skel");
    sspine_skeleton skeleton = sspine_make_skeleton(&(sspine_skeleton_desc){
        .atlas = atlas,
        .binary_data = skeleton_binary_data
    });
    free_data(skeleton_binary_data);
    return skeleton;
}

UTEST(sokol_spine, default_init_shutdown) {
    // FIXME!
    T(true);
}

UTEST(sokol_spine, atlas_pool_exhausted) {
    sg_setup(&(sg_desc){0});
    sspine_setup(&(sspine_desc){
        .atlas_pool_size = 4
    });
    for (int i = 0; i < 4; i++) {
        sspine_atlas atlas = sspine_make_atlas(&(sspine_atlas_desc){0});
        T(sspine_get_atlas_resource_state(atlas) == SSPINE_RESOURCESTATE_FAILED);
    }
    sspine_atlas atlas = sspine_make_atlas(&(sspine_atlas_desc){0});
    T(SSPINE_INVALID_ID == atlas.id);
    T(sspine_get_atlas_resource_state(atlas) == SSPINE_RESOURCESTATE_INVALID);
    shutdown();
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
    T(sspine_num_images(atlas) == 0);
    shutdown();

}

// NOTE: spine-c doesn't detect wrong/corrupt atlas file data, so we can't test for that

UTEST(sokol_spine, atlas_image_info) {
    init();
    sspine_atlas atlas = create_atlas();
    T(sspine_atlas_valid(atlas));
    T(sspine_num_images(atlas) == 1);
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
    T(img_info.premul_alpha == false);
    shutdown();
}

UTEST(sokol_spine, atlas_with_overrides) {
    init();
    sspine_range atlas_data = load_data("spineboy.atlas");
    sspine_atlas atlas = sspine_make_atlas(&(sspine_atlas_desc){
        .data = atlas_data,
        .override = {
            .min_filter = SG_FILTER_NEAREST_MIPMAP_NEAREST,
            .mag_filter = SG_FILTER_NEAREST,
            .wrap_u = SG_WRAP_REPEAT,
            .wrap_v = SG_WRAP_CLAMP_TO_EDGE,
            .premul_alpha_enabled = true,
        }
    });
    T(sspine_atlas_valid(atlas));
    T(sspine_num_images(atlas) == 1);
    const sspine_image_info img_info = sspine_get_image_info(atlas, 0);
    T(img_info.image.id != SG_INVALID_ID);
    T(sg_query_image_state(img_info.image) == SG_RESOURCESTATE_ALLOC);
    T(strcmp(img_info.filename, "spineboy.png") == 0);
    T(img_info.min_filter == SG_FILTER_NEAREST_MIPMAP_NEAREST);
    T(img_info.mag_filter == SG_FILTER_NEAREST);
    T(img_info.wrap_u == SG_WRAP_REPEAT);
    T(img_info.wrap_v == SG_WRAP_CLAMP_TO_EDGE);
    T(img_info.width == 1024);
    T(img_info.height == 256);
    T(img_info.premul_alpha == true);
    shutdown();
}

UTEST(sokol_spine, skeleton_pool_exhausted) {
    sg_setup(&(sg_desc){0});
    sspine_setup(&(sspine_desc){
        .skeleton_pool_size = 4
    });
    for (int i = 0; i < 4; i++) {
        sspine_skeleton skeleton = sspine_make_skeleton(&(sspine_skeleton_desc){0});
        T(sspine_get_skeleton_resource_state(skeleton) == SSPINE_RESOURCESTATE_FAILED);
    }
    sspine_skeleton skeleton = sspine_make_skeleton(&(sspine_skeleton_desc){0});
    T(SSPINE_INVALID_ID == skeleton.id);
    T(sspine_get_skeleton_resource_state(skeleton) == SSPINE_RESOURCESTATE_INVALID);
    shutdown();
}

UTEST(sokol_spine, make_destroy_skeleton_json_ok) {
    init();
    sspine_skeleton skeleton = create_skeleton_json(create_atlas());
    T(sspine_get_skeleton_resource_state(skeleton) == SSPINE_RESOURCESTATE_VALID);
    T(sspine_skeleton_valid(skeleton));
    sspine_destroy_skeleton(skeleton);
    T(sspine_get_skeleton_resource_state(skeleton) == SSPINE_RESOURCESTATE_INVALID);
    T(!sspine_skeleton_valid(skeleton));
    shutdown();
}

UTEST(sokol_spine, make_destroy_skeleton_binary_ok) {
    init();
    sspine_skeleton skeleton = create_skeleton_binary(create_atlas());
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

UTEST(sokol_spine, make_skeleton_json_fail_corrupt_data) {
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

// FIXME: this crashes the spine-c runtime
/*
UTEST(sokol_spine, make_skeleton_binary_fail_corrupt_data) {
    init();
    sspine_atlas atlas = create_atlas();
    uint8_t invalid_binary_data[] = { 0x23, 0x63, 0x11, 0xFF };
    sspine_skeleton skeleton = sspine_make_skeleton(&(sspine_skeleton_desc){
        .atlas = atlas,
        .binary_data = { .ptr = invalid_binary_data, .size = sizeof(invalid_binary_data) }
    });
    T(sspine_get_skeleton_resource_state(skeleton) == SSPINE_RESOURCESTATE_FAILED);
    sspine_destroy_skeleton(skeleton);
    T(sspine_get_skeleton_resource_state(skeleton) == SSPINE_RESOURCESTATE_INVALID);
    shutdown();
}
*/

UTEST(sokol_spine, instance_pool_exhausted) {
    sg_setup(&(sg_desc){0});
    sspine_setup(&(sspine_desc){
        .instance_pool_size = 4
    });
    for (int i = 0; i < 4; i++) {
        sspine_instance instance = sspine_make_instance(&(sspine_instance_desc){0});
        T(sspine_get_instance_resource_state(instance) == SSPINE_RESOURCESTATE_FAILED);
    }
    sspine_instance instance = sspine_make_instance(&(sspine_instance_desc){0});
    T(SSPINE_INVALID_ID == instance.id);
    T(sspine_get_instance_resource_state(instance) == SSPINE_RESOURCESTATE_INVALID);
    shutdown();
}

UTEST(sokol_spine, make_destroy_instance_ok) {
    init();
    sspine_instance instance = sspine_make_instance(&(sspine_instance_desc){
        .skeleton = create_skeleton_json(create_atlas())
    });
    T(sspine_get_instance_resource_state(instance) == SSPINE_RESOURCESTATE_VALID);
    T(sspine_instance_valid(instance));
    sspine_destroy_instance(instance);
    T(sspine_get_instance_resource_state(instance) == SSPINE_RESOURCESTATE_INVALID);
    T(!sspine_instance_valid(instance));
    shutdown();
}

UTEST(sokol_spine, make_instance_fail_no_skeleton) {
    init();
    sspine_instance instance = sspine_make_instance(&(sspine_instance_desc){0});
    T(sspine_get_instance_resource_state(instance) == SSPINE_RESOURCESTATE_FAILED);
    sspine_destroy_instance(instance);
    T(sspine_get_instance_resource_state(instance) == SSPINE_RESOURCESTATE_INVALID);
    shutdown();
}

UTEST(sokol_spine, make_instance_fail_with_failed_skeleton) {
    init();
    sspine_skeleton failed_skeleton = sspine_make_skeleton(&(sspine_skeleton_desc){0});
    T(sspine_get_skeleton_resource_state(failed_skeleton) == SSPINE_RESOURCESTATE_FAILED);
    sspine_instance instance = sspine_make_instance(&(sspine_instance_desc){
        .skeleton = failed_skeleton
    });
    T(sspine_get_instance_resource_state(instance) == SSPINE_RESOURCESTATE_FAILED);
    shutdown();
}

UTEST(sokol_spine, make_instance_fail_with_destroyed_atlas) {
    init();
    sspine_atlas atlas = create_atlas();
    T(sspine_atlas_valid(atlas));
    sspine_skeleton skeleton = create_skeleton_json(atlas);
    T(sspine_skeleton_valid(skeleton));
    sspine_destroy_atlas(atlas);
    T(!sspine_atlas_valid(atlas));
    sspine_instance instance = sspine_make_instance(&(sspine_instance_desc){
        .skeleton = skeleton
    });
    T(sspine_get_instance_resource_state(instance) == SSPINE_RESOURCESTATE_FAILED);
    shutdown();
}

UTEST(sokol_spine, set_get_position) {
    init();
    sspine_instance instance = create_instance();
    sspine_set_position(instance, (sspine_vec2){ .x=1.0f, .y=2.0f });
    const sspine_vec2 pos = sspine_get_position(instance);
    T(pos.x == 1.0f);
    T(pos.y == 2.0f);
    shutdown();
}

UTEST(sokol_spine, set_get_position_destroyed_instance) {
    init();
    sspine_instance instance = create_instance();
    sspine_set_position(instance, (sspine_vec2){ .x=1.0f, .y=2.0f });
    sspine_destroy_instance(instance);
    const sspine_vec2 pos = sspine_get_position(instance);
    T(pos.x == 0.0f);
    T(pos.y == 0.0f);
    shutdown();
}

UTEST(sokol_spine, set_get_scale) {
    init();
    sspine_instance instance = create_instance();
    sspine_set_scale(instance, (sspine_vec2){ .x=2.0f, .y=3.0f });
    const sspine_vec2 scale = sspine_get_scale(instance);
    T(scale.x == 2.0f);
    T(scale.y == 3.0f);
    shutdown();
}

UTEST(sokol_spine, set_get_scale_destroyed_instance) {
    init();
    sspine_instance instance = create_instance();
    sspine_set_scale(instance, (sspine_vec2){ .x=2.0f, .y=3.0f });
    sspine_destroy_instance(instance);
    const sspine_vec2 scale = sspine_get_scale(instance);
    T(scale.x == 0.0f);
    T(scale.y == 0.0f);
    shutdown();
}

UTEST(sokol_spine, set_get_color) {
    init();
    sspine_instance instance = create_instance();
    sspine_set_color(instance, (sspine_color) { .r=1.0f, .g=2.0f, .b=3.0f, .a=4.0f });
    const sspine_color color = sspine_get_color(instance);
    T(color.r == 1.0f);
    T(color.g == 2.0f);
    T(color.b == 3.0f);
    T(color.a == 4.0f);
    shutdown();
}

UTEST(sokol_spine, set_get_color_destroyed_instance) {
    init();
    sspine_instance instance = create_instance();
    sspine_set_color(instance, (sspine_color) { .r=1.0f, .g=2.0f, .b=3.0f, .a=4.0f });
    sspine_destroy_instance(instance);
    const sspine_color color = sspine_get_color(instance);
    T(color.r == 0.0f);
    T(color.g == 0.0f);
    T(color.b == 0.0f);
    T(color.a == 0.0f);
    shutdown();
}

UTEST(sokol_spine, find_anim) {
    init();
    sspine_instance instance = create_instance();
    sspine_anim a0 = sspine_find_anim(instance, "hoverboard");
    T(a0.instance.id == instance.id);
    T(a0.index == 2);
    sspine_anim a1 = sspine_find_anim(instance, "bla");
    T(a1.instance.id == SSPINE_INVALID_ID);
    T(a1.index == 0);
    shutdown();
}

UTEST(sokol_spine, find_anim_destroyed_instance) {
    init();
    sspine_instance instance = create_instance();
    sspine_destroy_instance(instance);
    sspine_anim a0 = sspine_find_anim(instance, "hoverboard");
    T(a0.instance.id == SSPINE_INVALID_ID);
    T(a0.index == 0);
    shutdown();
}

UTEST(sokol_spine, anim_valid) {
    init();
    sspine_instance instance = create_instance();
    sspine_anim a0 = sspine_find_anim(instance, "hoverboard");
    T(sspine_anim_valid(a0));
    sspine_anim a1 = sspine_find_anim(instance, "blablub");
    T(!sspine_anim_valid(a1));
    sspine_destroy_instance(instance);
    sspine_anim a2 = sspine_find_anim(instance, "hoverboard");
    T(!sspine_anim_valid(a2));
    shutdown();
}

UTEST(sokol_spine, num_anims) {
    init();
    sspine_instance instance = create_instance();
    T(sspine_num_anims(instance) == 11);
    sspine_destroy_instance(instance);
    T(sspine_num_anims(instance) == 0);
    shutdown();
}

UTEST(sokol_spine, anim_at) {
    init();
    sspine_instance instance = create_instance();
    sspine_anim a0 = sspine_anim_at(instance, 0);
    T(a0.instance.id == instance.id);
    T(a0.index == 0);
    T(sspine_anim_valid(a0));
    sspine_anim a1 = sspine_anim_at(instance, -1);
    T(a1.instance.id == SSPINE_INVALID_ID);
    T(a1.index == 0);
    T(!sspine_anim_valid(a1));
    sspine_anim a2 = sspine_anim_at(instance, 3);
    T(a2.instance.id == instance.id);
    T(a2.index == 3);
    T(sspine_anim_valid(a2));
    sspine_anim a3 = sspine_anim_at(instance, 11);
    T(a3.instance.id == SSPINE_INVALID_ID);
    T(a3.index == 0);
    T(!sspine_anim_valid(a3));
    sspine_anim a4 = sspine_anim_at(instance, 10);
    T(a4.instance.id == instance.id);
    T(a4.index == 10);
    T(sspine_anim_valid(a4));
    shutdown();
}

UTEST(sokol_spine, get_anim_info) {
    init();
    sspine_instance instance = create_instance();
    sspine_anim anim = sspine_find_anim(instance, "hoverboard");
    const sspine_anim_info info = sspine_get_anim_info(anim);
    T(info.index == 2);
    T(strcmp(info.name, "hoverboard") == 0);
    T(info.duration == 1.0f);
    shutdown();
}

UTEST(sokol_spine, atlas_page_at) {
    init();
    sspine_atlas atlas = create_atlas();
    T(sspine_num_atlas_pages(atlas) == 1);
    sspine_atlas_page p0 = sspine_atlas_page_at(atlas, 0);
    T(p0.atlas.id == atlas.id);
    T(p0.index == 0);
    sspine_atlas_page p1 = sspine_atlas_page_at(atlas, 1);
    T(p1.atlas.id == SSPINE_INVALID_ID);
    T(p1.index == 0);
    shutdown();
}

UTEST(sokol_spine, atlas_page_at_destroyed_atlas) {
    init();
    sspine_atlas atlas = create_atlas();
    sspine_destroy_atlas(atlas);
    sspine_atlas_page p0 = sspine_atlas_page_at(atlas, 0);
    T(p0.atlas.id == SSPINE_INVALID_ID);
    T(p0.index == 0);
    shutdown();
}

UTEST(sokol_spine, atlas_get_atlas_page_info) {
    init();
    sspine_atlas atlas = create_atlas();
    const sspine_atlas_page_info info = sspine_get_atlas_page_info(sspine_atlas_page_at(atlas, 0));
    T(info.atlas.id == atlas.id);
    T(info.image.id != SG_INVALID_ID);
    T(sg_query_image_state(info.image) == SG_RESOURCESTATE_ALLOC);
    T(strcmp(info.name, "spineboy.png") == 0);
    T(info.min_filter == SG_FILTER_LINEAR);
    T(info.mag_filter == SG_FILTER_LINEAR);
    T(info.wrap_u == SG_WRAP_MIRRORED_REPEAT);
    T(info.wrap_v == SG_WRAP_MIRRORED_REPEAT);
    T(info.width == 1024);
    T(info.height == 256);
    T(info.premul_alpha == false);
    T(info.overrides.min_filter == _SG_FILTER_DEFAULT);
    T(info.overrides.mag_filter == _SG_FILTER_DEFAULT);
    T(info.overrides.wrap_u == _SG_WRAP_DEFAULT);
    T(info.overrides.wrap_v == _SG_WRAP_DEFAULT);
    T(!info.overrides.premul_alpha_enabled);
    T(!info.overrides.premul_alpha_disabled);
    shutdown();
}

UTEST(sokol_spine, atlas_get_atlas_page_info_destroyed_atlas) {
    init();
    sspine_atlas atlas = create_atlas();
    sspine_atlas_page page = sspine_atlas_page_at(atlas, 0);
    sspine_destroy_atlas(atlas);
    const sspine_atlas_page_info info = sspine_get_atlas_page_info(page);
    T(info.atlas.id == SSPINE_INVALID_ID);
    shutdown();
}

UTEST(sokol_spine, atlas_get_atlas_page_info_with_overrides) {
    init();
    sspine_range atlas_data = load_data("spineboy.atlas");
    sspine_atlas atlas = sspine_make_atlas(&(sspine_atlas_desc){
        .data = atlas_data,
        .override = {
            .min_filter = SG_FILTER_NEAREST_MIPMAP_NEAREST,
            .mag_filter = SG_FILTER_NEAREST,
            .wrap_u = SG_WRAP_REPEAT,
            .wrap_v = SG_WRAP_CLAMP_TO_EDGE,
            .premul_alpha_enabled = true,
        }
    });
    const sspine_atlas_page_info info = sspine_get_atlas_page_info(sspine_atlas_page_at(atlas, 0));
    T(info.atlas.id == atlas.id);
    T(info.image.id != SG_INVALID_ID);
    T(sg_query_image_state(info.image) == SG_RESOURCESTATE_ALLOC);
    T(strcmp(info.name, "spineboy.png") == 0);
    T(info.min_filter == SG_FILTER_LINEAR);
    T(info.mag_filter == SG_FILTER_LINEAR);
    T(info.wrap_u == SG_WRAP_MIRRORED_REPEAT);
    T(info.wrap_v == SG_WRAP_MIRRORED_REPEAT);
    T(info.width == 1024);
    T(info.height == 256);
    T(info.premul_alpha == true); // FIXME: hmm, this is actually inconsistent
    T(info.overrides.min_filter == SG_FILTER_NEAREST_MIPMAP_NEAREST);
    T(info.overrides.mag_filter == SG_FILTER_NEAREST);
    T(info.overrides.wrap_u == SG_WRAP_REPEAT);
    T(info.overrides.wrap_v == SG_WRAP_CLAMP_TO_EDGE);
    T(info.overrides.premul_alpha_enabled);
    T(!info.overrides.premul_alpha_disabled);
    shutdown();
}

UTEST(sokol_spine, find_bone) {
    init();
    sspine_instance instance = create_instance();
    sspine_bone b0 = sspine_find_bone(instance, "crosshair");
    T(b0.instance.id == instance.id);
    T(b0.index == 2);
    sspine_bone b1 = sspine_find_bone(instance, "blablub");
    T(b1.instance.id == SSPINE_INVALID_ID);
    T(b1.index == 0);
    shutdown();
}

UTEST(sokol_spine, find_bone_destroyed_instance) {
    init();
    sspine_instance instance = create_instance();
    sspine_destroy_instance(instance);
    sspine_bone b0 = sspine_find_bone(instance, "crosshair");
    T(b0.instance.id == SSPINE_INVALID_ID);
    T(b0.index == 0);
    shutdown();
}

UTEST(sokol_spine, bone_valid) {
    init();
    sspine_instance instance = create_instance();
    sspine_bone b0 = sspine_find_bone(instance, "crosshair");
    T(sspine_bone_valid(b0));
    sspine_bone b1 = sspine_find_bone(instance, "blablub");
    T(!sspine_bone_valid(b1));
    sspine_destroy_instance(instance);
    sspine_bone b2 = sspine_find_bone(instance, "crosshair");
    T(!sspine_bone_valid(b2));
    shutdown();
}

UTEST(sokol_spine, num_bones) {
    init();
    sspine_instance instance = create_instance();
    T(sspine_num_bones(instance) == 67);
    sspine_destroy_instance(instance);
    T(sspine_num_bones(instance) == 0);
    shutdown();
}

UTEST(sokol_spine, bone_at) {
    init();
    sspine_instance instance = create_instance();
    sspine_bone b0 = sspine_bone_at(instance, 0);
    T(b0.instance.id == instance.id);
    T(b0.index == 0);
    sspine_bone b1 = sspine_bone_at(instance, -1);
    T(b1.instance.id == SSPINE_INVALID_ID);
    T(b1.index == 0);
    sspine_bone b2 = sspine_bone_at(instance, 23);
    T(b2.instance.id == instance.id);
    T(b2.index == 23);
    sspine_bone b3 = sspine_bone_at(instance, 67);
    T(b3.instance.id == SSPINE_INVALID_ID);
    T(b3.index == 0);
    sspine_bone b4 = sspine_bone_at(instance, 66);
    T(b4.instance.id == instance.id);
    T(b4.index == 66);
    shutdown();
}

UTEST(sokol_spine, get_bone_info) {
    init();
    sspine_instance instance = create_instance();
    sspine_bone bone = sspine_find_bone(instance, "root");
    const sspine_bone_info info = sspine_get_bone_info(bone);
    T(info.index == 0);
    T(strcmp(info.name, "root") == 0);
    T(info.length == 0.0f);
    T(info.pose_tform.position.x == 0.0f);
    T(info.pose_tform.position.y == 0.0f);
    T(info.pose_tform.rotation == 0.05f);
    T(info.pose_tform.scale.x == 1.0f);
    T(info.pose_tform.scale.y == 1.0f);
    T(info.pose_tform.shear.x == 0.0f);
    T(info.pose_tform.shear.y == 0.0f);
    shutdown();
}

UTEST(sokol_spine, get_bone_info_destroyed_instance) {
    init();
    sspine_instance instance = create_instance();
    sspine_bone bone = sspine_find_bone(instance, "root");
    sspine_destroy_instance(instance);
    const sspine_bone_info info = sspine_get_bone_info(bone);
    T(info.name == 0);
    shutdown();
}

UTEST(sokol_spine, set_get_bone_transform) {
    init();
    sspine_instance instance = create_instance();
    sspine_bone bone = sspine_find_bone(instance, "root");
    sspine_set_bone_transform(bone, &(sspine_bone_transform){
        .position = { 1.0f, 2.0f },
        .rotation = 3.0f,
        .scale = { 4.0f, 5.0f },
        .shear = { 6.0f, 7.0f }
    });
    const sspine_bone_transform tform = sspine_get_bone_transform(bone);
    T(tform.position.x == 1.0f);
    T(tform.position.y == 2.0f);
    T(tform.rotation == 3.0f);
    T(tform.scale.x == 4.0f);
    T(tform.scale.y == 5.0f);
    T(tform.shear.x == 6.0f);
    T(tform.shear.y == 7.0f);
    shutdown();
}

UTEST(sokol_spine, set_get_bone_transform_destroyed_instance) {
    init();
    sspine_instance instance = create_instance();
    sspine_bone bone = sspine_find_bone(instance, "root");
    sspine_destroy_instance(instance);
    sspine_set_bone_transform(bone, &(sspine_bone_transform){
        .position = { 1.0f, 2.0f },
        .rotation = 3.0f,
        .scale = { 4.0f, 5.0f },
        .shear = { 6.0f, 7.0f }
    });
    const sspine_bone_transform tform = sspine_get_bone_transform(bone);
    T(tform.position.x == 0.0f);
    T(tform.position.y == 0.0f);
    T(tform.rotation == 0.0f);
    T(tform.scale.x == 0.0f);
    T(tform.scale.y == 0.0f);
    T(tform.shear.x == 0.0f);
    T(tform.shear.y == 0.0f);
    shutdown();
}

UTEST(sokol_spine, set_get_bone_position) {
    init();
    sspine_instance instance = create_instance();
    sspine_bone bone = sspine_find_bone(instance, "root");
    sspine_set_bone_position(bone, (sspine_vec2){ 1.0f, 2.0f });
    const sspine_vec2 p0 = sspine_get_bone_position(bone);
    T(p0.x == 1.0f);
    T(p0.y == 2.0f);
    sspine_destroy_instance(instance);
    const sspine_vec2 p1 = sspine_get_bone_position(bone);
    T(p1.x == 0.0f);
    T(p1.y == 0.0f);
    shutdown();
}

UTEST(sokol_spine, set_get_bone_rotation) {
    init();
    sspine_instance instance = create_instance();
    sspine_bone bone = sspine_find_bone(instance, "root");
    sspine_set_bone_rotation(bone, 5.0f);
    T(sspine_get_bone_rotation(bone) == 5.0f);
    sspine_destroy_instance(instance);
    T(sspine_get_bone_rotation(bone) == 0.0f);
    shutdown();
}

UTEST(sokol_spine, set_get_bone_scale) {
    init();
    sspine_instance instance = create_instance();
    sspine_bone bone = sspine_find_bone(instance, "root");
    sspine_set_bone_scale(bone, (sspine_vec2){ 1.0f, 2.0f });
    const sspine_vec2 s0 = sspine_get_bone_scale(bone);
    T(s0.x == 1.0f);
    T(s0.y == 2.0f);
    sspine_destroy_instance(instance);
    const sspine_vec2 s1 = sspine_get_bone_scale(bone);
    T(s1.x == 0.0f);
    T(s1.y == 0.0f);
    shutdown();
}

UTEST(sokol_spine, set_get_bone_shear) {
    init();
    sspine_instance instance = create_instance();
    sspine_bone bone = sspine_find_bone(instance, "root");
    sspine_set_bone_shear(bone, (sspine_vec2){ 1.0f, 2.0f });
    const sspine_vec2 s0 = sspine_get_bone_shear(bone);
    T(s0.x == 1.0f);
    T(s0.y == 2.0f);
    sspine_destroy_instance(instance);
    const sspine_vec2 s1 = sspine_get_bone_shear(bone);
    T(s1.x == 0.0f);
    T(s1.y == 0.0f);
    shutdown();
}

UTEST(sokol_spine, find_slot) {
    init();
    sspine_instance instance = create_instance();
    sspine_slot s0 = sspine_find_slot(instance, "portal-streaks1");
    T(s0.instance.id == instance.id);
    T(s0.index == 3);
    sspine_slot s1 = sspine_find_slot(instance, "blablub");
    T(s1.instance.id == SSPINE_INVALID_ID);
    T(s1.index == 0);
    shutdown();
}

UTEST(sokol_spine, find_slot_destroyed_instance) {
    init();
    sspine_instance instance = create_instance();
    sspine_destroy_instance(instance);
    sspine_slot s0 = sspine_find_slot(instance, "portal-streaks1");
    T(s0.instance.id == SSPINE_INVALID_ID);
    T(s0.index == 0);
    shutdown();
}

UTEST(sokol_spine, slot_valid) {
    init();
    sspine_instance instance = create_instance();
    sspine_slot s0 = sspine_find_slot(instance, "portal-streaks1");
    T(sspine_slot_valid(s0));
    sspine_slot s1 = sspine_find_slot(instance, "blablub");
    T(!sspine_slot_valid(s1));
    sspine_destroy_instance(instance);
    sspine_slot s2 = sspine_find_slot(instance, "blablub");
    T(!sspine_slot_valid(s2));
    shutdown();
}

UTEST(sokol_spine, num_slots) {
    init();
    sspine_instance instance = create_instance();
    T(sspine_num_slots(instance) == 52);
    sspine_destroy_instance(instance);
    T(sspine_num_slots(instance) == 0);
    shutdown();
}

UTEST(sokol_spine, slot_at) {
    init();
    sspine_instance instance = create_instance();
    sspine_slot s0 = sspine_slot_at(instance, 0);
    T(s0.instance.id == instance.id);
    T(s0.index == 0);
    sspine_slot s1 = sspine_slot_at(instance, -1);
    T(s1.instance.id == SSPINE_INVALID_ID);
    T(s1.index == 0);
    sspine_slot s2 = sspine_slot_at(instance, 23);
    T(s2.instance.id == instance.id);
    T(s2.index == 23);
    sspine_slot s3 = sspine_slot_at(instance, 52);
    T(s3.instance.id == SSPINE_INVALID_ID);
    T(s3.index == 0);
    sspine_slot s4 = sspine_slot_at(instance, 51);
    T(s4.instance.id == instance.id);
    T(s4.index == 51);
    shutdown();
}

UTEST(sokol_spine, get_slot_info) {
    init();
    sspine_instance instance = create_instance();
    sspine_slot slot = sspine_find_slot(instance, "portal-streaks1");
    const sspine_slot_info info = sspine_get_slot_info(slot);
    T(info.index == 3);
    T(strcmp(info.name, "portal-streaks1") == 0);
    T(info.attachment_name == 0);
    T(info.bone.instance.id == instance.id);
    T(info.bone.index == 62);
    T(info.color.r == 1.0f);
    T(info.color.g == 1.0f);
    T(info.color.b == 1.0f);
    T(info.color.a == 1.0f);
    shutdown();
}

UTEST(sokol_spine, get_slot_info_destroyed_instance) {
    init();
    sspine_instance instance = create_instance();
    sspine_slot slot = sspine_find_slot(instance, "portal-streaks1");
    sspine_destroy_instance(instance);
    const sspine_slot_info info = sspine_get_slot_info(slot);
    T(info.name == 0);
    shutdown();
}

UTEST(sokol_spine, set_get_slot_color) {
    init();
    sspine_instance instance = create_instance();
    sspine_slot slot = sspine_find_slot(instance, "portal-streaks1");
    sspine_set_slot_color(slot, (sspine_color){ 1.0f, 2.0f, 3.0f, 4.0f });
    const sspine_color color = sspine_get_slot_color(slot);
    T(color.r == 1.0f);
    T(color.g == 2.0f);
    T(color.b == 3.0f);
    T(color.a == 4.0f);
    const sspine_slot_info info = sspine_get_slot_info(slot);
    T(info.color.r == 1.0f);
    T(info.color.g == 2.0f);
    T(info.color.b == 3.0f);
    T(info.color.a == 4.0f);
    shutdown();
}
