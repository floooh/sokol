/*
    Sokol Metal rendering backend.
*/
#ifndef SOKOL_IMPL_GUARD
#error "Please do not include *.impl.h files directly"
#endif

/* memset() */
#include <string.h>

#ifdef __cplusplus
extern "C" {
#endif

/*-- Metal backend resource structs ------------------------------------------*/
typedef struct {
    _sg_slot slot;
    int size;
    sg_buffer_type type;
    sg_usage usage;
} _sg_buffer;

_SOKOL_PRIVATE void _sg_init_buffer(_sg_buffer* buf) {
    SOKOL_ASSERT(buf);
    memset(buf, 0, sizeof(_sg_buffer));
}

typedef struct {
    _sg_slot slot;
    sg_image_type type;
    bool render_target;
    uint16_t width;
    uint16_t height;
    uint16_t depth;
    uint16_t num_mipmaps;
    sg_usage usage;
    sg_pixel_format pixel_format;
    int sample_count;
} _sg_image;

_SOKOL_PRIVATE void _sg_init_image(_sg_image* img) {
    SOKOL_ASSERT(img);
    memset(img, 0, sizeof(_sg_image));
}

typedef struct {
    uint16_t size;
} _sg_uniform_block;

typedef struct {
    sg_image_type type;
} _sg_shader_image;

typedef struct {
    uint16_t num_uniform_blocks;
    uint16_t num_images;
    _sg_uniform_block uniform_blocks[SG_MAX_SHADERSTAGE_UBS];
    _sg_shader_image images[SG_MAX_SHADERSTAGE_IMAGES];
} _sg_shader_stage;

typedef struct {
    _sg_slot slot;
    _sg_shader_stage stage[SG_NUM_SHADER_STAGES];
} _sg_shader;

_SOKOL_PRIVATE void _sg_init_shader(_sg_shader* shd) {
    SOKOL_ASSERT(shd);
    memset(shd, 0, sizeof(_sg_shader));
}

typedef struct {
    _sg_slot slot;
    _sg_shader* shader;
    sg_shader shader_id;
    sg_primitive_type primitive_type;
    sg_index_type index_type;
} _sg_pipeline;

_SOKOL_PRIVATE void _sg_init_pipeline(_sg_pipeline* pip) {
    SOKOL_ASSERT(pip);
    memset(pip, 0, sizeof(_sg_pipeline));
}

typedef struct {
    _sg_image* image;
    sg_image image_id;
    int mip_level;
    int slice;
} _sg_attachment;

typedef struct {
    _sg_slot slot;
    _sg_attachment color_atts[SG_MAX_COLOR_ATTACHMENTS];
    _sg_attachment ds_att;
} _sg_pass;

_SOKOL_PRIVATE void _sg_init_pass(_sg_pass* pass) {
    SOKOL_ASSERT(pass);
    memset(pass, 0, sizeof(_sg_pass));
}

/*-- main Metal backend state and functions ----------------------------------*/
typedef struct {
    bool next_draw_valid;
} _sg_backend;

_SOKOL_PRIVATE void _sg_setup_backend(_sg_backend* state) {
    SOKOL_ASSERT(state);
    state->next_draw_valid = false;
}

_SOKOL_PRIVATE void _sg_discard_backend(_sg_backend* state) {
    SOKOL_ASSERT(state);
}

_SOKOL_PRIVATE bool _sg_query_feature(_sg_backend* state, sg_feature f) {
    SOKOL_ASSERT(state);
    // FIXME
    return false;
}

_SOKOL_PRIVATE void _sg_create_buffer(_sg_backend* state, _sg_buffer* buf, const sg_buffer_desc* desc) {
    SOKOL_ASSERT(state && buf && desc);
    SOKOL_ASSERT(buf->slot.state == SG_RESOURCESTATE_ALLOC);
    SOKOL_ASSERT(desc->data_size <= desc->size);
}

_SOKOL_PRIVATE void _sg_destroy_buffer(_sg_backend* state, _sg_buffer* buf) {
    SOKOL_ASSERT(state && buf);
    // FIXME
    _sg_init_buffer(buf);
}

_SOKOL_PRIVATE void _sg_create_image(_sg_backend* state, _sg_image* img, const sg_image_desc* desc) {
    SOKOL_ASSERT(state && img && desc);
    SOKOL_ASSERT(img->slot.state == SG_RESOURCESTATE_ALLOC);
}

_SOKOL_PRIVATE void _sg_destroy_image(_sg_backend* state, _sg_image* img) {
    SOKOL_ASSERT(state && img);
    // FIXME
    _sg_init_image(img);
}

_SOKOL_PRIVATE void _sg_create_shader(_sg_backend* state, _sg_shader* shd, const sg_shader_desc* desc) {
    SOKOL_ASSERT(state && shd && desc);
    SOKOL_ASSERT(shd->slot.state == SG_RESOURCESTATE_ALLOC);
}

_SOKOL_PRIVATE void _sg_destroy_shader(_sg_backend* state, _sg_shader* shd) {
    SOKOL_ASSERT(state && shd);
    // FIXME
    _sg_init_shader(shd);
}

_SOKOL_PRIVATE void _sg_create_pipeline(_sg_backend* state, _sg_pipeline* pip, _sg_shader* shd, const sg_pipeline_desc* desc) {
    SOKOL_ASSERT(state && pip && shd && desc);
    SOKOL_ASSERT(pip->slot.state == SG_RESOURCESTATE_ALLOC);
    SOKOL_ASSERT(desc->shader.id == shd->slot.id);
}

_SOKOL_PRIVATE void _sg_destroy_pipeline(_sg_backend* state, _sg_pipeline* pip) {
    SOKOL_ASSERT(state && pip);
    // FIXME
    _sg_init_pipeline(pip);
}

_SOKOL_PRIVATE void _sg_create_pass(_sg_backend* state, _sg_pass* pass, _sg_image** att_images, const sg_pass_desc* desc) {
    SOKOL_ASSERT(state && pass && desc);
    SOKOL_ASSERT(pass->slot.state == SG_RESOURCESTATE_ALLOC);
}

_SOKOL_PRIVATE void _sg_destroy_pass(_sg_backend* state, _sg_pass* pass) {
    SOKOL_ASSERT(state && pass);
    // FIXME
    _sg_init_pass(pass);
}

_SOKOL_PRIVATE void _sg_begin_pass(_sg_backend* state, _sg_pass* pass, const sg_pass_action* action, int w, int h) {
    SOKOL_ASSERT(state);
    SOKOL_ASSERT(action);
    // FIXME
}

_SOKOL_PRIVATE void _sg_end_pass(_sg_backend* state) {
    SOKOL_ASSERT(state);
    // FIXME
}

_SOKOL_PRIVATE void _sg_apply_viewport(_sg_backend* state, int x, int y, int w, int h, bool origin_top_left) {
    SOKOL_ASSERT(state);
    // FIXME
}

_SOKOL_PRIVATE void _sg_apply_scissor_rect(_sg_backend* state, int x, int y, int w, int h, bool origin_top_left) {
    SOKOL_ASSERT(state);
    // FIXME
}

_SOKOL_PRIVATE void _sg_apply_draw_state(_sg_backend* state,
    _sg_pipeline* pip,
    _sg_buffer** vbs, int num_vbs, _sg_buffer* ib,
    _sg_image** vs_imgs, int num_vs_imgs,
    _sg_image** fs_imgs, int num_fs_imgs)
{
    SOKOL_ASSERT(state);
    SOKOL_ASSERT(pip);
    // FIXME
}

_SOKOL_PRIVATE void _sg_apply_uniform_block(_sg_backend* state, sg_shader_stage stage_index, int ub_index, const void* data, int num_bytes) {
    SOKOL_ASSERT(state);
    SOKOL_ASSERT(data && (num_bytes > 0));
    SOKOL_ASSERT((stage_index >= 0) && ((int)stage_index < SG_NUM_SHADER_STAGES));
}

_SOKOL_PRIVATE void _sg_draw(_sg_backend* state, int base_element, int num_elements, int num_instances) {
    SOKOL_ASSERT(state);
    // FIXME
}

_SOKOL_PRIVATE void _sg_commit(_sg_backend* state) {
    SOKOL_ASSERT(state);
    // FIXME
}

_SOKOL_PRIVATE void _sg_update_buffer(_sg_backend* state, _sg_buffer* buf, const void* data, int data_size) {
    SOKOL_ASSERT(state && buf && data && (data_size > 0));
    // FIXME
}

#ifdef __cplusplus
} /* extern "C" */
#endif

