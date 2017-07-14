/*
    FIXME: Sokol Gfx GL rendering backend
*/

typedef struct {
    _sg_slot slot;
} _sg_buffer;

typedef struct {
    _sg_slot slot;
} _sg_image;

typedef struct {
    _sg_slot slot;
} _sg_shader;

typedef struct {
    _sg_slot slot;
} _sg_pipeline;

typedef struct {
    _sg_slot slot;
} _sg_pass;

static void _sg_init_buffer(_sg_buffer* buf) {
    SOKOL_ASSERT(buf);
    _sg_init_slot(&buf->slot);
}

static void _sg_init_image(_sg_image* img) {
    SOKOL_ASSERT(img);
    _sg_init_slot(&img->slot);
}

static void _sg_init_shader(_sg_shader* shd) {
    SOKOL_ASSERT(shd);
    _sg_init_slot(&shd->slot);
}

static void _sg_init_pipeline(_sg_pipeline* pip) {
    SOKOL_ASSERT(pip);
    _sg_init_slot(&pip->slot);
}

static void _sg_init_pass(_sg_pass* pass) {
    SOKOL_ASSERT(pass);
    _sg_init_slot(&pass->slot);
}

