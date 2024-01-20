//------------------------------------------------------------------------------
//  sokol_spine_test.c
//------------------------------------------------------------------------------
#include "sokol_gfx.h"
#define SOKOL_SPINE_IMPL
#include "spine/spine.h"
#include "sokol_spine.h"
#include "utest.h"

#define T(b) EXPECT_TRUE(b)

static sspine_log_item last_logitem = SSPINE_LOGITEM_OK;
static void log_func(const char* tag, uint32_t log_level, uint32_t log_item, const char* message, uint32_t line_nr, const char* filename, void* user_data) {
    (void)tag; (void)log_level; (void)message; (void)line_nr; (void)filename; (void)user_data;
    last_logitem = log_item;
}

static void init(void) {
    last_logitem = SSPINE_LOGITEM_OK;
    sg_setup(&(sg_desc){0});
    sspine_setup(&(sspine_desc){ .logger = { .func = log_func } });
}

static void init_with_desc(const sspine_desc* desc) {
    last_logitem = SSPINE_LOGITEM_OK;
    sspine_desc desc1 = *desc;
    desc1.logger.func = log_func;
    sg_setup(&(sg_desc){0});
    sspine_setup(&desc1);
}

static void shutdown(void) {
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
    // NOTE: GCC warns if result of fread() is ignored
    size_t num_bytes = fread(ptr, size, 1, fp);
    (void)num_bytes;
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

static sspine_skeleton create_skeleton_binary(sspine_atlas atlas) {
    sspine_range skeleton_binary_data = load_data("spineboy-pro.skel");
    sspine_skeleton skeleton = sspine_make_skeleton(&(sspine_skeleton_desc){
        .atlas = atlas,
        .binary_data = skeleton_binary_data
    });
    free_data(skeleton_binary_data);
    return skeleton;
}

static sspine_skeleton create_skeleton(void) {
    return create_skeleton_json(create_atlas());
}

static sspine_instance create_instance(void) {
    return sspine_make_instance(&(sspine_instance_desc){
        .skeleton = create_skeleton(),
    });
}

UTEST(sokol_spine, default_init_shutdown) {
    // FIXME!
    T(true);
}

UTEST(sokol_spine, atlas_pool_exhausted) {
    init_with_desc(&(sspine_desc){
        .atlas_pool_size = 4,
    });
    for (int i = 0; i < 4; i++) {
        sspine_atlas atlas = sspine_make_atlas(&(sspine_atlas_desc){0});
        T(sspine_get_atlas_resource_state(atlas) == SSPINE_RESOURCESTATE_FAILED);
        T(last_logitem == SSPINE_LOGITEM_ATLAS_DESC_NO_DATA);
    }
    sspine_atlas atlas = sspine_make_atlas(&(sspine_atlas_desc){0});
    T(SSPINE_INVALID_ID == atlas.id);
    T(sspine_get_atlas_resource_state(atlas) == SSPINE_RESOURCESTATE_INVALID);
    T(last_logitem == SSPINE_LOGITEM_ATLAS_POOL_EXHAUSTED);
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
    T(last_logitem == SSPINE_LOGITEM_ATLAS_DESC_NO_DATA);
    T(sspine_get_atlas_resource_state(atlas) == SSPINE_RESOURCESTATE_FAILED);
    T(!sspine_atlas_valid(atlas));
    shutdown();
}

// an invalid atlas must return zero number of images
UTEST(sokol_spine, failed_atlas_no_images) {
    init();
    sspine_atlas atlas = sspine_make_atlas(&(sspine_atlas_desc){0});
    T(last_logitem == SSPINE_LOGITEM_ATLAS_DESC_NO_DATA);
    T(atlas.id != SSPINE_INVALID_ID);
    T(!sspine_atlas_valid(atlas));
    T(sspine_num_images(atlas) == 0);
    shutdown();

}

// NOTE: spine-c doesn't detect wrong/corrupt atlas file data, so we can't test for that

UTEST(sokol_spine, image_valid) {
    init();
    sspine_atlas atlas = create_atlas();
    T(sspine_image_valid(sspine_image_by_index(atlas, 0)));
    T(!sspine_image_valid(sspine_image_by_index(atlas, 1)));
    T(!sspine_image_valid(sspine_image_by_index(atlas, -1)));
    sspine_destroy_atlas(atlas);
    T(!sspine_image_valid(sspine_image_by_index(atlas, 0)));
    shutdown();
}

UTEST(sokol_spine, atlas_image_info) {
    init();
    sspine_atlas atlas = create_atlas();
    T(sspine_atlas_valid(atlas));
    T(sspine_num_images(atlas) == 1);
    const sspine_image_info img_info = sspine_get_image_info(sspine_image_by_index(atlas, 0));
    T(img_info.valid);
    T(img_info.sgimage.id != SG_INVALID_ID);
    T(sg_query_image_state(img_info.sgimage) == SG_RESOURCESTATE_ALLOC);
    T(strcmp(img_info.filename.cstr, "spineboy.png") == 0);
    T(img_info.min_filter == SG_FILTER_LINEAR);
    T(img_info.mag_filter == SG_FILTER_LINEAR);
    T(img_info.wrap_u == SG_WRAP_CLAMP_TO_EDGE);
    T(img_info.wrap_v == SG_WRAP_CLAMP_TO_EDGE);
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
            .min_filter = SG_FILTER_NEAREST,
            .mag_filter = SG_FILTER_NEAREST,
            .mipmap_filter = SG_FILTER_LINEAR,
            .wrap_u = SG_WRAP_REPEAT,
            .wrap_v = SG_WRAP_CLAMP_TO_EDGE,
            .premul_alpha_enabled = true,
        }
    });
    T(sspine_atlas_valid(atlas));
    T(sspine_num_images(atlas) == 1);
    const sspine_image_info img_info = sspine_get_image_info(sspine_image_by_index(atlas, 0));
    T(img_info.valid);
    T(img_info.sgimage.id != SG_INVALID_ID);
    T(sg_query_image_state(img_info.sgimage) == SG_RESOURCESTATE_ALLOC);
    T(strcmp(img_info.filename.cstr, "spineboy.png") == 0);
    T(img_info.min_filter == SG_FILTER_NEAREST);
    T(img_info.mag_filter == SG_FILTER_NEAREST);
    T(img_info.mipmap_filter == SG_FILTER_LINEAR);
    T(img_info.wrap_u == SG_WRAP_REPEAT);
    T(img_info.wrap_v == SG_WRAP_CLAMP_TO_EDGE);
    T(img_info.width == 1024);
    T(img_info.height == 256);
    T(img_info.premul_alpha == true);
    shutdown();
}

UTEST(sokol_spine, skeleton_pool_exhausted) {
    init_with_desc(&(sspine_desc){
        .skeleton_pool_size = 4
    });
    for (int i = 0; i < 4; i++) {
        sspine_skeleton skeleton = sspine_make_skeleton(&(sspine_skeleton_desc){0});
        T(sspine_get_skeleton_resource_state(skeleton) == SSPINE_RESOURCESTATE_FAILED);
        T(last_logitem == SSPINE_LOGITEM_SKELETON_DESC_NO_DATA);
    }
    sspine_skeleton skeleton = sspine_make_skeleton(&(sspine_skeleton_desc){0});
    T(SSPINE_INVALID_ID == skeleton.id);
    T(sspine_get_skeleton_resource_state(skeleton) == SSPINE_RESOURCESTATE_INVALID);
    T(last_logitem == SSPINE_LOGITEM_SKELETON_POOL_EXHAUSTED);
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
    T(last_logitem == SSPINE_LOGITEM_SKELETON_DESC_NO_DATA);
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
    T(last_logitem == SSPINE_LOGITEM_SKELETON_DESC_NO_ATLAS);
    shutdown();
}

UTEST(sokol_spine, make_skeleton_fail_with_failed_atlas) {
    init();
    sspine_atlas atlas = sspine_make_atlas(&(sspine_atlas_desc){0});
    T(last_logitem == SSPINE_LOGITEM_ATLAS_DESC_NO_DATA);
    T(sspine_get_atlas_resource_state(atlas) == SSPINE_RESOURCESTATE_FAILED);
    sspine_skeleton skeleton = create_skeleton_json(atlas);
    T(sspine_get_skeleton_resource_state(skeleton) == SSPINE_RESOURCESTATE_FAILED);
    T(!sspine_skeleton_valid(skeleton));
    T(last_logitem == SSPINE_LOGITEM_SKELETON_ATLAS_NOT_VALID);
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
    T(last_logitem == SSPINE_LOGITEM_CREATE_SKELETON_DATA_FROM_JSON_FAILED);
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
    init_with_desc(&(sspine_desc){
        .instance_pool_size = 4
    });
    for (int i = 0; i < 4; i++) {
        sspine_instance instance = sspine_make_instance(&(sspine_instance_desc){0});
        T(sspine_get_instance_resource_state(instance) == SSPINE_RESOURCESTATE_FAILED);
        T(last_logitem == SSPINE_LOGITEM_INSTANCE_DESC_NO_SKELETON);
    }
    sspine_instance instance = sspine_make_instance(&(sspine_instance_desc){0});
    T(SSPINE_INVALID_ID == instance.id);
    T(sspine_get_instance_resource_state(instance) == SSPINE_RESOURCESTATE_INVALID);
    T(last_logitem == SSPINE_LOGITEM_INSTANCE_POOL_EXHAUSTED);
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
    T(last_logitem == SSPINE_LOGITEM_INSTANCE_DESC_NO_SKELETON);
    sspine_destroy_instance(instance);
    T(sspine_get_instance_resource_state(instance) == SSPINE_RESOURCESTATE_INVALID);
    shutdown();
}

UTEST(sokol_spine, make_instance_fail_with_failed_skeleton) {
    init();
    sspine_skeleton failed_skeleton = sspine_make_skeleton(&(sspine_skeleton_desc){0});
    T(last_logitem == SSPINE_LOGITEM_SKELETON_DESC_NO_DATA);
    T(sspine_get_skeleton_resource_state(failed_skeleton) == SSPINE_RESOURCESTATE_FAILED);
    sspine_instance instance = sspine_make_instance(&(sspine_instance_desc){
        .skeleton = failed_skeleton
    });
    T(sspine_get_instance_resource_state(instance) == SSPINE_RESOURCESTATE_FAILED);
    T(last_logitem == SSPINE_LOGITEM_INSTANCE_SKELETON_NOT_VALID);
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
    T(last_logitem == SSPINE_LOGITEM_INSTANCE_ATLAS_NOT_VALID);
    shutdown();
}

UTEST(sokol_spine, get_skeleton_atlas) {
    init();
    sspine_atlas atlas = create_atlas();
    sspine_skeleton skeleton = create_skeleton_json(atlas);
    T(sspine_get_skeleton_atlas(skeleton).id == atlas.id);
    sspine_destroy_skeleton(skeleton);
    T(sspine_get_skeleton_atlas(skeleton).id == SSPINE_INVALID_ID);
    shutdown();
}

UTEST(sokol_spine, get_instance_skeleton) {
    init();
    sspine_atlas atlas = create_atlas();
    sspine_skeleton skeleton = create_skeleton_json(atlas);
    sspine_instance instance = sspine_make_instance(&(sspine_instance_desc){
        .skeleton = skeleton
    });
    T(sspine_get_instance_skeleton(instance).id == skeleton.id);
    sspine_destroy_instance(instance);
    T(sspine_get_instance_skeleton(instance).id == SSPINE_INVALID_ID);
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

UTEST(sokol_spine, anim_by_name) {
    init();
    sspine_skeleton skeleton = create_skeleton();
    sspine_anim a0 = sspine_anim_by_name(skeleton, "hoverboard");
    T((a0.skeleton_id == skeleton.id) && (a0.index == 2));
    sspine_anim a1 = sspine_anim_by_name(skeleton, "bla");
    T((a1.skeleton_id == 0) && (a1.index == 0));
    shutdown();
}

UTEST(sokol_spine, anim_by_name_destroyed_instance) {
    init();
    sspine_skeleton skeleton = create_skeleton();
    sspine_destroy_skeleton(skeleton);
    sspine_anim a0 = sspine_anim_by_name(skeleton, "hoverboard");
    T((a0.skeleton_id == 0) && (a0.index == 0));
    shutdown();
}

UTEST(sokol_spine, anim_valid) {
    init();
    sspine_skeleton skeleton = create_skeleton();
    T(sspine_anim_valid(sspine_anim_by_index(skeleton, 0)));
    T(sspine_anim_valid(sspine_anim_by_index(skeleton, 10)));
    T(!sspine_anim_valid(sspine_anim_by_index(skeleton, -1)));
    T(!sspine_anim_valid(sspine_anim_by_index(skeleton, 11)));
    sspine_destroy_skeleton(skeleton);
    T(!sspine_anim_valid(sspine_anim_by_index(skeleton, 0)));
    shutdown();
}

UTEST(sokol_spine, anim_equal) {
    init();
    T(sspine_anim_equal((sspine_anim){ 1, 2 }, (sspine_anim){ 1, 2 }));
    T(!sspine_anim_equal((sspine_anim){ 2, 2 }, (sspine_anim){ 1, 2 }));
    T(!sspine_anim_equal((sspine_anim){ 1, 3 }, (sspine_anim){ 1, 2 }));
    shutdown();
}

UTEST(sokol_spine, num_anims) {
    init();
    sspine_skeleton skeleton = create_skeleton();
    T(sspine_num_anims(skeleton) == 11);
    sspine_destroy_skeleton(skeleton);
    T(sspine_num_anims(skeleton) == 0);
    shutdown();
}

UTEST(sokol_spine, get_anim_info) {
    init();
    sspine_skeleton skeleton = create_skeleton();
    sspine_anim anim = sspine_anim_by_name(skeleton, "hoverboard");
    const sspine_anim_info info = sspine_get_anim_info(anim);
    T(info.valid);
    T(info.index == 2);
    T(strcmp(info.name.cstr, "hoverboard") == 0);
    T(info.duration == 1.0f);
    shutdown();
}

UTEST(sokol_spine, get_anim_info_destroyed_skeleton) {
    init();
    sspine_skeleton skeleton = create_skeleton();
    sspine_anim anim = sspine_anim_by_name(skeleton, "hoverboard");
    sspine_destroy_skeleton(skeleton);
    const sspine_anim_info info = sspine_get_anim_info(anim);
    T(!info.valid);
    shutdown();
}

UTEST(sokol_spine, get_anim_info_invalid_index) {
    init();
    sspine_skeleton skeleton = create_skeleton();
    const sspine_anim_info i0 = sspine_get_anim_info(sspine_anim_by_index(skeleton, -1));
    T(!i0.valid);
    T(!i0.name.valid);
    const sspine_anim_info i1 = sspine_get_anim_info(sspine_anim_by_index(skeleton, 1234));
    T(!i1.valid);
    T(!i1.name.valid);
    shutdown();
}

UTEST(sokol_spine, atlas_page_valid) {
    init();
    sspine_atlas atlas = create_atlas();
    T(sspine_atlas_page_valid(sspine_atlas_page_by_index(atlas, 0)));
    T(!sspine_atlas_page_valid(sspine_atlas_page_by_index(atlas, -1)));
    T(!sspine_atlas_page_valid(sspine_atlas_page_by_index(atlas, 1)));
    sspine_destroy_atlas(atlas);
    T(!sspine_atlas_page_valid(sspine_atlas_page_by_index(atlas, 0)));
    shutdown();
}

UTEST(sokol_spine, num_atlas_pages) {
    init();
    sspine_atlas atlas = create_atlas();
    T(sspine_num_atlas_pages(atlas) == 1);
    sspine_destroy_atlas(atlas);
    T(sspine_num_atlas_pages(atlas) == 0);
    shutdown();
}

UTEST(sokol_spine, get_atlas_page_info) {
    init();
    sspine_atlas atlas = create_atlas();
    const sspine_atlas_page_info info = sspine_get_atlas_page_info(sspine_atlas_page_by_index(atlas, 0));
    T(info.valid);
    T(info.atlas.id == atlas.id);
    T(info.image.valid);
    T(info.image.sgimage.id != SG_INVALID_ID);
    T(sg_query_image_state(info.image.sgimage) == SG_RESOURCESTATE_ALLOC);
    T(strcmp(info.image.filename.cstr, "spineboy.png") == 0);
    T(info.image.min_filter == SG_FILTER_LINEAR);
    T(info.image.mag_filter == SG_FILTER_LINEAR);
    T(info.image.wrap_u == SG_WRAP_CLAMP_TO_EDGE);
    T(info.image.wrap_v == SG_WRAP_CLAMP_TO_EDGE);
    T(info.image.width == 1024);
    T(info.image.height == 256);
    T(info.image.premul_alpha == false);
    T(info.overrides.min_filter == _SG_FILTER_DEFAULT);
    T(info.overrides.mag_filter == _SG_FILTER_DEFAULT);
    T(info.overrides.wrap_u == _SG_WRAP_DEFAULT);
    T(info.overrides.wrap_v == _SG_WRAP_DEFAULT);
    T(!info.overrides.premul_alpha_enabled);
    T(!info.overrides.premul_alpha_disabled);
    shutdown();
}

UTEST(sokol_spine, get_atlas_page_info_destroyed_atlas) {
    init();
    sspine_atlas atlas = create_atlas();
    sspine_destroy_atlas(atlas);
    const sspine_atlas_page_info info = sspine_get_atlas_page_info(sspine_atlas_page_by_index(atlas, 0));
    T(!info.valid);
    T(info.atlas.id == SSPINE_INVALID_ID);
    shutdown();
}

UTEST(sokol_spine, get_atlas_page_info_invalid_index) {
    init();
    sspine_atlas atlas = create_atlas();
    sspine_destroy_atlas(atlas);
    const sspine_atlas_page_info i0 = sspine_get_atlas_page_info(sspine_atlas_page_by_index(atlas, -1));
    T(!i0.valid);
    T(i0.atlas.id == SSPINE_INVALID_ID);
    const sspine_atlas_page_info i1 = sspine_get_atlas_page_info(sspine_atlas_page_by_index(atlas, 1234));
    T(!i0.valid);
    T(i1.atlas.id == SSPINE_INVALID_ID);
    shutdown();
}

UTEST(sokol_spine, atlas_get_atlas_page_info_with_overrides) {
    init();
    sspine_range atlas_data = load_data("spineboy.atlas");
    sspine_atlas atlas = sspine_make_atlas(&(sspine_atlas_desc){
        .data = atlas_data,
        .override = {
            .min_filter = SG_FILTER_NEAREST,
            .mag_filter = SG_FILTER_NEAREST,
            .mipmap_filter = SG_FILTER_NEAREST,
            .wrap_u = SG_WRAP_REPEAT,
            .wrap_v = SG_WRAP_CLAMP_TO_EDGE,
            .premul_alpha_enabled = true,
        }
    });
    const sspine_atlas_page_info info = sspine_get_atlas_page_info(sspine_atlas_page_by_index(atlas, 0));
    T(info.valid);
    T(info.atlas.id == atlas.id);
    T(info.image.valid);
    T(info.image.sgimage.id != SG_INVALID_ID);
    T(sg_query_image_state(info.image.sgimage) == SG_RESOURCESTATE_ALLOC);
    T(strcmp(info.image.filename.cstr, "spineboy.png") == 0);
    T(info.image.min_filter == SG_FILTER_LINEAR);
    T(info.image.mag_filter == SG_FILTER_LINEAR);
    T(info.image.mipmap_filter == SG_FILTER_NONE);
    T(info.image.wrap_u == SG_WRAP_CLAMP_TO_EDGE);
    T(info.image.wrap_v == SG_WRAP_CLAMP_TO_EDGE);
    T(info.image.width == 1024);
    T(info.image.height == 256);
    T(info.image.premul_alpha == true); // FIXME: hmm, this is actually inconsistent
    T(info.overrides.min_filter == SG_FILTER_NEAREST);
    T(info.overrides.mag_filter == SG_FILTER_NEAREST);
    T(info.overrides.mipmap_filter == SG_FILTER_NEAREST);
    T(info.overrides.wrap_u == SG_WRAP_REPEAT);
    T(info.overrides.wrap_v == SG_WRAP_CLAMP_TO_EDGE);
    T(info.overrides.premul_alpha_enabled);
    T(!info.overrides.premul_alpha_disabled);
    shutdown();
}

UTEST(sokol_spine, bone_by_name) {
    init();
    sspine_skeleton skeleton = create_skeleton();
    sspine_bone b0 = sspine_bone_by_name(skeleton, "crosshair");
    T((b0.skeleton_id == skeleton.id) && (b0.index == 2));
    sspine_bone b1 = sspine_bone_by_name(skeleton, "blablub");
    T((b1.skeleton_id == 0) && (b1.index == 0));
    shutdown();
}

UTEST(sokol_spine, bone_by_name_destroyed_skeleton) {
    init();
    sspine_skeleton skeleton = create_skeleton();
    sspine_destroy_skeleton(skeleton);
    sspine_bone b0 = sspine_bone_by_name(skeleton, "crosshair");
    T((b0.skeleton_id == 0) && (b0.index == 0));
    shutdown();
}

UTEST(sokol_spine, bone_valid) {
    init();
    sspine_skeleton skeleton = create_skeleton();
    T(sspine_bone_valid(sspine_bone_by_index(skeleton, 0)));
    T(sspine_bone_valid(sspine_bone_by_index(skeleton, 66)));
    T(!sspine_bone_valid(sspine_bone_by_index(skeleton, -1)));
    T(!sspine_bone_valid(sspine_bone_by_index(skeleton, 67)));
    sspine_destroy_skeleton(skeleton);
    T(!sspine_bone_valid(sspine_bone_by_index(skeleton, 0)));
    shutdown();
}

UTEST(sokol_spine, bone_equal) {
    init();
    T(sspine_bone_equal((sspine_bone){ 1, 2 }, (sspine_bone){ 1, 2 }));
    T(!sspine_bone_equal((sspine_bone){ 2, 2 }, (sspine_bone){ 1, 2 }));
    T(!sspine_bone_equal((sspine_bone){ 1, 3 }, (sspine_bone){ 1, 2 }));
    shutdown();
}

UTEST(sokol_spine, num_bones) {
    init();
    sspine_skeleton skeleton = create_skeleton();
    T(sspine_num_bones(skeleton) == 67);
    sspine_destroy_skeleton(skeleton);
    T(sspine_num_bones(skeleton) == 0);
    shutdown();
}

UTEST(sokol_spine, get_bone_info_root) {
    init();
    sspine_skeleton skeleton = create_skeleton();
    const sspine_bone_info info = sspine_get_bone_info(sspine_bone_by_name(skeleton, "root"));
    T(info.valid);
    T(info.index == 0);
    T((info.parent_bone.skeleton_id == 0) && (info.parent_bone.index == 0));
    T(strcmp(info.name.cstr, "root") == 0);
    T(info.length == 0.0f);
    T(info.pose.position.x == 0.0f);
    T(info.pose.position.y == 0.0f);
    T(info.pose.rotation == 0.05f);
    T(info.pose.scale.x == 1.0f);
    T(info.pose.scale.y == 1.0f);
    T(info.pose.shear.x == 0.0f);
    T(info.pose.shear.y == 0.0f);
    shutdown();
}

UTEST(sokol_spine, get_bone_info_parent_bone) {
    init();
    sspine_skeleton skeleton = create_skeleton();
    const sspine_bone_info info = sspine_get_bone_info(sspine_bone_by_name(skeleton, "rear-shin"));
    T(info.valid);
    T(info.index == 7);
    T((info.parent_bone.skeleton_id == skeleton.id) && (info.parent_bone.index == 6));
    shutdown();
}

UTEST(sokol_spine, get_bone_info_destroyed_skeleton) {
    init();
    sspine_skeleton skeleton = create_skeleton();
    sspine_bone bone = sspine_bone_by_name(skeleton, "root");
    sspine_destroy_skeleton(skeleton);
    const sspine_bone_info info = sspine_get_bone_info(bone);
    T(!info.valid);
    T(!info.name.valid);
    shutdown();
}

UTEST(sokol_spine, get_bone_info_invalid_index) {
    init();
    sspine_skeleton skeleton = create_skeleton();
    const sspine_bone_info i0 = sspine_get_bone_info(sspine_bone_by_index(skeleton, -1));
    T(!i0.valid);
    T(!i0.name.valid);
    const sspine_bone_info i1 = sspine_get_bone_info(sspine_bone_by_index(skeleton, 1234));
    T(!i1.valid);
    T(!i1.name.valid);
    shutdown();
}

UTEST(sokol_spine, set_get_bone_transform) {
    init();
    sspine_instance instance = create_instance();
    sspine_skeleton skeleton = sspine_get_instance_skeleton(instance);
    sspine_bone bone = sspine_bone_by_name(skeleton, "root");
    sspine_set_bone_transform(instance, bone, &(sspine_bone_transform){
        .position = { 1.0f, 2.0f },
        .rotation = 3.0f,
        .scale = { 4.0f, 5.0f },
        .shear = { 6.0f, 7.0f }
    });
    const sspine_bone_transform tform = sspine_get_bone_transform(instance, bone);
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
    sspine_skeleton skeleton = sspine_get_instance_skeleton(instance);
    sspine_bone bone = sspine_bone_by_name(skeleton, "root");
    sspine_destroy_instance(instance);
    sspine_set_bone_transform(instance, bone, &(sspine_bone_transform){
        .position = { 1.0f, 2.0f },
        .rotation = 3.0f,
        .scale = { 4.0f, 5.0f },
        .shear = { 6.0f, 7.0f }
    });
    const sspine_bone_transform tform = sspine_get_bone_transform(instance, bone);
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
    sspine_skeleton skeleton = sspine_get_instance_skeleton(instance);
    sspine_bone bone = sspine_bone_by_name(skeleton, "root");
    sspine_set_bone_position(instance, bone, (sspine_vec2){ 1.0f, 2.0f });
    const sspine_vec2 p0 = sspine_get_bone_position(instance, bone);
    T(p0.x == 1.0f);
    T(p0.y == 2.0f);
    sspine_destroy_instance(instance);
    const sspine_vec2 p1 = sspine_get_bone_position(instance, bone);
    T(p1.x == 0.0f);
    T(p1.y == 0.0f);
    shutdown();
}

UTEST(sokol_spine, set_get_bone_rotation) {
    init();
    sspine_instance instance = create_instance();
    sspine_skeleton skeleton = sspine_get_instance_skeleton(instance);
    sspine_bone bone = sspine_bone_by_name(skeleton, "root");
    sspine_set_bone_rotation(instance, bone, 5.0f);
    T(sspine_get_bone_rotation(instance, bone) == 5.0f);
    sspine_destroy_instance(instance);
    T(sspine_get_bone_rotation(instance, bone) == 0.0f);
    shutdown();
}

UTEST(sokol_spine, set_get_bone_scale) {
    init();
    sspine_instance instance = create_instance();
    sspine_skeleton skeleton = sspine_get_instance_skeleton(instance);
    sspine_bone bone = sspine_bone_by_name(skeleton, "root");
    sspine_set_bone_scale(instance, bone, (sspine_vec2){ 1.0f, 2.0f });
    const sspine_vec2 s0 = sspine_get_bone_scale(instance, bone);
    T(s0.x == 1.0f);
    T(s0.y == 2.0f);
    sspine_destroy_instance(instance);
    const sspine_vec2 s1 = sspine_get_bone_scale(instance, bone);
    T(s1.x == 0.0f);
    T(s1.y == 0.0f);
    shutdown();
}

UTEST(sokol_spine, set_get_bone_shear) {
    init();
    sspine_instance instance = create_instance();
    sspine_skeleton skeleton = sspine_get_instance_skeleton(instance);
    sspine_bone bone = sspine_bone_by_name(skeleton, "root");
    sspine_set_bone_shear(instance, bone, (sspine_vec2){ 1.0f, 2.0f });
    const sspine_vec2 s0 = sspine_get_bone_shear(instance, bone);
    T(s0.x == 1.0f);
    T(s0.y == 2.0f);
    sspine_destroy_instance(instance);
    const sspine_vec2 s1 = sspine_get_bone_shear(instance, bone);
    T(s1.x == 0.0f);
    T(s1.y == 0.0f);
    shutdown();
}

UTEST(sokol_spine, slot_by_name) {
    init();
    sspine_skeleton skeleton = create_skeleton();
    sspine_slot s0 = sspine_slot_by_name(skeleton, "portal-streaks1");
    T((s0.skeleton_id == skeleton.id) && (s0.index == 3));
    sspine_slot s1 = sspine_slot_by_name(skeleton, "blablub");
    T((s1.skeleton_id == 0) && (s1.index == 0));
    shutdown();
}

UTEST(sokol_spine, slot_by_name_destroyed_skeleton) {
    init();
    sspine_skeleton skeleton = create_skeleton();
    sspine_destroy_skeleton(skeleton);
    sspine_slot s0 = sspine_slot_by_name(skeleton, "portal-streaks1");
    T((s0.skeleton_id == 0) && (s0.index == 0));
    shutdown();
}

UTEST(sokol_spine, num_slots) {
    init();
    sspine_skeleton skeleton = create_skeleton();
    T(sspine_num_slots(skeleton) == 52);
    sspine_destroy_skeleton(skeleton);
    T(sspine_num_slots(skeleton) == 0);
    shutdown();
}

UTEST(sokol_spine, slot_valid) {
    init();
    sspine_skeleton skeleton = create_skeleton();
    T(sspine_slot_valid(sspine_slot_by_index(skeleton, 0)));
    T(sspine_slot_valid(sspine_slot_by_index(skeleton, 51)));
    T(!sspine_slot_valid(sspine_slot_by_index(skeleton, -1)));
    T(!sspine_slot_valid(sspine_slot_by_index(skeleton, 52)));
    sspine_destroy_skeleton(skeleton);
    T(!sspine_slot_valid(sspine_slot_by_index(skeleton, 0)));
    shutdown();
}

UTEST(sokol_spine, slot_equal) {
    init();
    T(sspine_slot_equal((sspine_slot){ 1, 2 }, (sspine_slot){ 1, 2 }));
    T(!sspine_slot_equal((sspine_slot){ 2, 2 }, (sspine_slot){ 1, 2 }));
    T(!sspine_slot_equal((sspine_slot){ 1, 3 }, (sspine_slot){ 1, 2 }));
    shutdown();
}

UTEST(sokol_spine, get_slot_info) {
    init();
    sspine_skeleton skeleton = create_skeleton();
    const sspine_slot_info info = sspine_get_slot_info(sspine_slot_by_name(skeleton, "portal-streaks1"));
    T(info.valid);
    T(info.index == 3);
    T(strcmp(info.name.cstr, "portal-streaks1") == 0);
    T(!info.attachment_name.valid);
    T((info.bone.skeleton_id == skeleton.id) && (info.bone.index == 62));
    T(info.color.r == 1.0f);
    T(info.color.g == 1.0f);
    T(info.color.b == 1.0f);
    T(info.color.a == 1.0f);
    shutdown();
}

UTEST(sokol_spine, get_slot_info_destroyed_skeleton) {
    init();
    sspine_skeleton skeleton = create_skeleton();
    sspine_slot slot = sspine_slot_by_name(skeleton, "portal-streaks1");
    sspine_destroy_skeleton(skeleton);
    const sspine_slot_info info = sspine_get_slot_info(slot);
    T(!info.valid);
    T(!info.name.valid);
    shutdown();
}

UTEST(sokol_spine, get_slot_info_invalid_index) {
    init();
    sspine_skeleton skeleton = create_skeleton();
    const sspine_slot_info i0 = sspine_get_slot_info(sspine_slot_by_index(skeleton, -1));
    T(!i0.valid);
    T(!i0.name.valid);
    const sspine_slot_info i1 = sspine_get_slot_info(sspine_slot_by_index(skeleton, 1234));
    T(!i1.valid);
    T(!i1.name.valid);
    shutdown();
}

UTEST(sokol_spine, set_get_slot_color) {
    init();
    sspine_instance instance = create_instance();
    sspine_skeleton skeleton = sspine_get_instance_skeleton(instance);
    sspine_slot slot = sspine_slot_by_name(skeleton, "portal-streaks1");
    sspine_set_slot_color(instance, slot, (sspine_color){ 1.0f, 2.0f, 3.0f, 4.0f });
    const sspine_color color = sspine_get_slot_color(instance, slot);
    T(color.r == 1.0f);
    T(color.g == 2.0f);
    T(color.b == 3.0f);
    T(color.a == 4.0f);
    const sspine_slot_info info = sspine_get_slot_info(slot);
    T(info.color.r == 1.0f);
    T(info.color.g == 1.0f);
    T(info.color.b == 1.0f);
    T(info.color.a == 1.0f);
    shutdown();
}

UTEST(sokol_spine, event_by_name) {
    init();
    sspine_skeleton skeleton = create_skeleton();
    sspine_event e0 = sspine_event_by_name(skeleton, "footstep");
    T((e0.skeleton_id == skeleton.id) && (e0.index == 0));
    sspine_event e1 = sspine_event_by_name(skeleton, "bla");
    T((e1.skeleton_id == 0) && (e1.index == 0));
    shutdown();
}

UTEST(sokol_spine, event_by_name_destroyed_skeleton) {
    init();
    sspine_skeleton skeleton = create_skeleton();
    sspine_destroy_skeleton(skeleton);
    sspine_event e0 = sspine_event_by_name(skeleton, "footstep");
    T((e0.skeleton_id == 0) && (e0.index == 0));
    shutdown();
}

UTEST(sokol_spine, event_valid) {
    init();
    sspine_skeleton skeleton = create_skeleton();
    T(sspine_event_valid(sspine_event_by_index(skeleton, 0)));
    T(!sspine_event_valid(sspine_event_by_index(skeleton, 1)));
    T(!sspine_event_valid(sspine_event_by_index(skeleton, -1)));
    sspine_destroy_skeleton(skeleton);
    T(!sspine_event_valid(sspine_event_by_index(skeleton, 0)));
    shutdown();
}

UTEST(sokol_spine, event_equal) {
    init();
    T(sspine_event_equal((sspine_event){ 1, 2 }, (sspine_event){ 1, 2 }));
    T(!sspine_event_equal((sspine_event){ 2, 2 }, (sspine_event){ 1, 2 }));
    T(!sspine_event_equal((sspine_event){ 1, 3 }, (sspine_event){ 1, 2 }));
    shutdown();
}

UTEST(sokol_spine, num_events) {
    init();
    sspine_skeleton skeleton = create_skeleton();
    T(sspine_num_events(skeleton) == 1);
    sspine_destroy_skeleton(skeleton);
    T(sspine_num_events(skeleton) == 0);
    shutdown();
}

UTEST(sokol_spine, get_event_info) {
    init();
    sspine_skeleton skeleton = create_skeleton();
    const sspine_event_info info = sspine_get_event_info(sspine_event_by_index(skeleton, 0));
    T(info.valid);
    T(0 == strcmp(info.name.cstr, "footstep"));
    T(0 == info.index);
    T(0 == info.int_value);
    T(0.0f == info.float_value);
    T(!info.string_value.valid);
    T(!info.audio_path.valid);
    T(0.0f == info.volume);
    T(0.0f == info.balance);
    shutdown();
}

UTEST(sokol_spine, get_event_info_destroyed_skeleton) {
    init();
    sspine_skeleton skeleton = create_skeleton();
    sspine_destroy_skeleton(skeleton);
    const sspine_event_info info = sspine_get_event_info(sspine_event_by_index(skeleton, 0));
    T(!info.valid);
    T(!info.name.valid);
    shutdown();
}

UTEST(sokol_spine, iktarget_by_name) {
    init();
    sspine_skeleton skeleton = create_skeleton();
    sspine_iktarget ik0 = sspine_iktarget_by_name(skeleton, "board-ik");
    T((ik0.skeleton_id == skeleton.id) && (ik0.index == 2));
    sspine_iktarget ik1 = sspine_iktarget_by_name(skeleton, "bla");
    T((ik1.skeleton_id == 0) && (ik1.index == 0));
    shutdown();
}

UTEST(sokol_spine, iktarget_by_name_destroyed_skeleton) {
    init();
    sspine_skeleton skeleton = create_skeleton();
    sspine_destroy_skeleton(skeleton);
    sspine_iktarget ik0 = sspine_iktarget_by_name(skeleton, "board-ik");
    T((ik0.skeleton_id == 0) && (ik0.index == 0));
    shutdown();
}

UTEST(sokol_spine, iktarget_valid) {
    init();
    sspine_skeleton skeleton = create_skeleton();
    T(sspine_iktarget_valid(sspine_iktarget_by_index(skeleton, 0)));
    T(sspine_iktarget_valid(sspine_iktarget_by_index(skeleton, 6)));
    T(!sspine_iktarget_valid(sspine_iktarget_by_index(skeleton, -1)));
    T(!sspine_iktarget_valid(sspine_iktarget_by_index(skeleton, 7)));
    sspine_destroy_skeleton(skeleton);
    T(!sspine_iktarget_valid(sspine_iktarget_by_index(skeleton, 0)));
    shutdown();
}

UTEST(sokol_spine, iktarget_equal) {
    init();
    T(sspine_iktarget_equal((sspine_iktarget){ 1, 2 }, (sspine_iktarget){ 1, 2 }));
    T(!sspine_iktarget_equal((sspine_iktarget){ 2, 2 }, (sspine_iktarget){ 1, 2 }));
    T(!sspine_iktarget_equal((sspine_iktarget){ 1, 3 }, (sspine_iktarget){ 1, 2 }));
    shutdown();
}

UTEST(sokol_spine, num_iktargets) {
    init();
    sspine_skeleton skeleton = create_skeleton();
    T(sspine_num_iktargets(skeleton) == 7);
    sspine_destroy_skeleton(skeleton);
    T(sspine_num_iktargets(skeleton) == 0);
    shutdown();
}

UTEST(sokol_spine, get_iktarget_info) {
    init();
    sspine_skeleton skeleton = create_skeleton();
    const sspine_iktarget_info info = sspine_get_iktarget_info(sspine_iktarget_by_index(skeleton, 1));
    T(info.valid);
    T(1 == info.index);
    T(0 == strcmp(info.name.cstr, "aim-torso-ik"));
    T((info.target_bone.skeleton_id == skeleton.id) && (info.target_bone.index == 2));
    shutdown();
}

UTEST(sokol_spine, get_iktarget_info_destroyed_skeleton) {
    init();
    sspine_skeleton skeleton = create_skeleton();
    sspine_destroy_skeleton(skeleton);
    const sspine_iktarget_info info = sspine_get_iktarget_info(sspine_iktarget_by_index(skeleton, 1));
    T(!info.valid);
    T(!info.name.valid);
    shutdown();
}

UTEST(sokol_spine, get_iktarget_info_out_of_bounds) {
    init();
    sspine_skeleton skeleton = create_skeleton();
    sspine_destroy_skeleton(skeleton);
    const sspine_iktarget_info info0 = sspine_get_iktarget_info(sspine_iktarget_by_index(skeleton, -1));
    T(!info0.name.valid);
    const sspine_iktarget_info info1 = sspine_get_iktarget_info(sspine_iktarget_by_index(skeleton, 7));
    T(!info1.name.valid);
    shutdown();
}

UTEST(sokol_spine, skin_by_name) {
    init();
    sspine_skeleton skeleton = create_skeleton();
    sspine_skin s0 = sspine_skin_by_name(skeleton, "default");
    T((s0.skeleton_id == skeleton.id) && (s0.index == 0));
    sspine_skin s1 = sspine_skin_by_name(skeleton, "bla");
    T((s1.skeleton_id == 0) && (s1.index == 0));
    sspine_destroy_skeleton(skeleton);
    sspine_skin s2 = sspine_skin_by_name(skeleton, "default");
    T((s2.skeleton_id == 0) && (s2.index == 0));
    shutdown();
}

UTEST(sokol_spine, skin_valid) {
    init();
    sspine_skeleton skeleton = create_skeleton();
    T(sspine_skin_valid(sspine_skin_by_index(skeleton, 0)));
    T(!sspine_skin_valid(sspine_skin_by_index(skeleton, -1)));
    T(!sspine_skin_valid(sspine_skin_by_index(skeleton, 1)));
    sspine_destroy_skeleton(skeleton);
    T(!sspine_skin_valid(sspine_skin_by_index(skeleton, 0)));
    shutdown();
}

UTEST(sokol_spine, skin_equal) {
    init();
    T(sspine_skin_equal((sspine_skin){ 1, 2 }, (sspine_skin){ 1, 2 }));
    T(!sspine_skin_equal((sspine_skin){ 2, 2 }, (sspine_skin){ 1, 2 }));
    T(!sspine_skin_equal((sspine_skin){ 1, 3 }, (sspine_skin){ 1, 2 }));
    shutdown();
}

UTEST(sokol_spine, num_skins) {
    init();
    sspine_skeleton skeleton = create_skeleton();
    T(sspine_num_skins(skeleton) == 1);
    sspine_destroy_skeleton(skeleton);
    T(sspine_num_skins(skeleton) == 0);
    shutdown();
}

UTEST(sokol_spine, get_skin_info) {
    init();
    sspine_skeleton skeleton = create_skeleton();
    const sspine_skin_info info = sspine_get_skin_info(sspine_skin_by_index(skeleton, 0));
    T(info.valid);
    T(0 == info.index);
    T(0 == strcmp(info.name.cstr, "default"));
    shutdown();
}

UTEST(sokol_spine, get_skin_info_destroyed_skeleton) {
    init();
    sspine_skeleton skeleton = create_skeleton();
    sspine_destroy_skeleton(skeleton);
    const sspine_skin_info info = sspine_get_skin_info(sspine_skin_by_index(skeleton, 0));
    T(!info.valid);
    T(!info.name.valid);
    shutdown();
}
