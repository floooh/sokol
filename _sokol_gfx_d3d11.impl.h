/*
    Sokol Gfx D3D11 rendering backend.
*/
#ifndef SOKOL_IMPL_GUARD
#error "Please do not include *.impl.h files directly"
#endif

#ifndef UNICODE
#define UNICODE
#endif
#define COBJMACROS
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <d3d11.h>

#pragma comment (lib, "user32.lib")
#pragma comment (lib, "dxgi.lib")
#pragma comment (lib, "d3d11.lib")
#pragma comment (lib, "dxguid.lib")

#ifdef __cplusplus
extern "C" {
#endif

/*-- backend resource structures ---------------------------------------------*/
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
    int width;
    int height;
    int depth;
    int num_mipmaps;
    sg_usage usage;
    sg_pixel_format pixel_format;
    int sample_count;
    sg_filter min_filter;
    sg_filter mag_filter;
    sg_wrap wrap_u;
    sg_wrap wrap_v;
    sg_wrap wrap_w;
} _sg_image;

_SOKOL_PRIVATE void _sg_init_image(_sg_image* img) {
    SOKOL_ASSERT(img);
    memset(img, 0, sizeof(_sg_image));
}

typedef struct {
    int size;
} _sg_uniform_block;

typedef struct {
    sg_image_type type;
} _sg_shader_image;

typedef struct {
    int num_uniform_blocks;
    int num_images;
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

/*-- main D3D11 backend state and functions ----------------------------------*/

_SOKOL_PRIVATE void _sg_setup_backend(const sg_desc* desc) {
    // FIXME
}

_SOKOL_PRIVATE void _sg_discard_backend() {
    // FIXME
}

_SOKOL_PRIVATE bool _sg_query_feature() {
    // FIXME
    return false;
}

_SOKOL_PRIVATE void _sg_create_buffer(_sg_buffer* buf, const sg_buffer_desc* desc) {
    // FIXME
}

_SOKOL_PRIVATE void _sg_destroy_buffer(_sg_buffer* buf) {
    // FIXME
}

_SOKOL_PRIVATE void _sg_create_image(_sg_image* img, const sg_image_desc* desc) {
    // FIXME
}

_SOKOL_PRIVATE void _sg_destroy_image(_sg_image* img) {
    // FIXME
}

_SOKOL_PRIVATE void _sg_create_shader(_sg_shader* shd, const sg_shader_desc* desc) {
    // FIXME
}

_SOKOL_PRIVATE void _sg_destroy_shader(_sg_shader* shd) {
    // FIXME
}

_SOKOL_PRIVATE void _sg_create_pipeline(_sg_pipeline* pip, _sg_shader* shd, const sg_pipeline_desc* desc) {
    // FIXME
}

_SOKOL_PRIVATE void _sg_destroy_pipeline(_sg_pipeline* pip) {
    // FIXME
}

_SOKOL_PRIVATE void _sg_create_pass(_sg_pass* pass, _sg_image** att_images, const sg_pass_desc* desc) {
    // FIXME
}

_SOKOL_PRIVATE void _sg_destroy_pass(_sg_pass* pass) {
    // FIXME
}

_SOKOL_PRIVATE void _sg_begin_pass(_sg_pass* pass, const sg_pass_action* action, int w, int h) {
    // FIXME
}

_SOKOL_PRIVATE void _sg_end_pass() {
    // FIXME
}

_SOKOL_PRIVATE void _sg_apply_viewport(int x, int y, int w, int h, bool origin_top_left) {
    // FIXME
}

_SOKOL_PRIVATE void _sg_apply_scissor_rect(int x, int y, int w, int h, bool origin_top_left) {
    // FIXME
}

_SOKOL_PRIVATE void _sg_apply_draw_state(
    _sg_pipeline* pip,
    _sg_buffer** vbs, int num_vbs, _sg_buffer* ib,
    _sg_image** vs_imgs, int num_vs_imgs,
    _sg_image** fs_imgs, int num_fs_imgs)
{
    // FIXME
}

_SOKOL_PRIVATE void _sg_apply_uniform_block(sg_shader_stage stage_index, int ub_index, const void* data, int num_bytes) {
    // FIXME
}

_SOKOL_PRIVATE void _sg_draw(int base_element, int num_elements, int num_instances) {
    // FIXME
}

_SOKOL_PRIVATE void _sg_commit() {
    // FIXME
}

_SOKOL_PRIVATE void _sg_update_buffer(_sg_buffer* buf, const void* data_ptr, int data_size) {
    // FIXME
}

_SOKOL_PRIVATE void _sg_update_image(_sg_image* img, const sg_image_content* data) {
    // FIXME
}

_SOKOL_PRIVATE void _sg_reset_state_cache() {
    // FIXME
}

#ifdef __cplusplus
} // extern "C"
#endif
