/*
    Sokol Gfx generic implementation code
*/
enum {
    _SG_CONST_SLOT_SHIFT = 16,
    _SG_CONST_SLOT_MASK = (1<<_SG_CONST_SLOT_SHIFT)-1,
    _SG_CONST_MAX_POOL_SIZE = (1<<_SG_CONST_SLOT_SHIFT),
};

/* return byte size of a vertex format */
static int _sg_vertexformat_bytesize(sg_vertex_format fmt) {
    switch (fmt) {
        case SG_VERTEXFORMAT_FLOAT:     return 4;
        case SG_VERTEXFORMAT_FLOAT2:    return 8;
        case SG_VERTEXFORMAT_FLOAT3:    return 12;
        case SG_VERTEXFORMAT_FLOAT4:    return 16;
        case SG_VERTEXFORMAT_BYTE4:     return 4;
        case SG_VERTEXFORMAT_BYTE4N:    return 4;
        case SG_VERTEXFORMAT_UBYTE4:    return 4;
        case SG_VERTEXFORMAT_UBYTE4N:   return 4;
        case SG_VERTEXFORMAT_SHORT2:    return 4;
        case SG_VERTEXFORMAT_SHORT2N:   return 4;
        case SG_VERTEXFORMAT_SHORT4:    return 8;
        case SG_VERTEXFORMAT_SHORT4N:   return 8;
        case SG_VERTEXFORMAT_UINT10_N2: return 4;
        case SG_VERTEXFORMAT_INVALID:   return 0;
    }
}

/* return byte size of a vertex layout */
static int _sg_vertexlayout_byte_size(sg_vertex_layout* layout) {
    SOKOL_ASSERT(layout);
    int byte_size = 0;
    for (int i = 0; i < layout->num_attrs; i++) {
        byte_size += _sg_vertexformat_bytesize(layout->attrs[i].format);
    }
    return byte_size;
}

/* return the byte offset of a vertex layout attribute */
static int _sg_vertexlayout_attr_offset(sg_vertex_layout* layout, int index) {
    SOKOL_ASSERT(layout && (index < layout->num_attrs));
    int byte_offset = 0;
    for (int i = 0; i < index; i++) {
        byte_offset += _sg_vertexformat_bytesize(layout->attrs[i].format);
    }
    return byte_offset;
}

//------------------------------------------------------------------------------
typedef struct {
    sg_id id;
    sg_resource_state state;
} _sg_slot;

static void _sg_init_slot(_sg_slot* slot) {
    SOKOL_ASSERT(slot);
    slot->id = SG_INVALID_ID;
    slot->state = SG_RESOURCESTATE_INITIAL;
}

static int _sg_slot_index(sg_id id) {
    return id & _SG_CONST_SLOT_MASK;
}

//------------------------------------------------------------------------------
#if defined(SOKOL_USE_GL) || defined(SOKOL_USE_GLES2) || defined(SOKOL_USE_GLES3)
#include "_sokol_gfx_gl.impl.h"
#elif defined(SOKOL_USE_D3D11)
#include "_sokol_gfx_d3d11.impl.h"
#elif defined(SOKOL_USE_METAL)
#include "_sokol_gfx_metal.impl.h"
#else
#error "No rendering backend configured (SOKOL_USE_GL, SOKOL_USE_GLES2, SOKOL_USE_GLES3, SOKOL_USE_D3D11 or SOKOL_GFX_USE_METAL"
#endif
#include "_sokol_gfx_pools.impl.h"

typedef struct {
    _sg_pools pools;
    _sg_backend backend;
} _sg_state;
_sg_state* _sg = 0;

void sg_setup(sg_desc* desc) {
    SOKOL_ASSERT(!_sg);
    SOKOL_ASSERT(desc);
    SOKOL_ASSERT((desc->width > 0) && (desc->height > 0));
    SOKOL_ASSERT(desc->sample_count >= 1);
    _sg = SOKOL_MALLOC(sizeof(_sg_state));
    _sg_setup_pools(&_sg->pools, desc);
    _sg_setup_backend(&_sg->backend);
}

void sg_discard() {
    SOKOL_ASSERT(_sg);
    _sg_discard_backend(&_sg->backend);
    _sg_discard_pools(&_sg->pools);
    SOKOL_FREE(_sg);
    _sg = 0;
}

bool sg_isvalid() {
    return _sg != 0;
}

/*-- allocate resource id ----------------------------------------------------*/
sg_id sg_alloc_buffer() {
    SOKOL_ASSERT(_sg);
    sg_id buf_id = _sg_pool_alloc_id(&_sg->pools.pool[SG_RESOURCETYPE_BUFFER]);
    if (buf_id != SG_INVALID_ID) {
        _sg_buffer* buf = _sg_buffer_at(&_sg->pools, buf_id);
        SOKOL_ASSERT(buf && (buf->slot.state == SG_RESOURCESTATE_INITIAL) && (buf->slot.id == SG_INVALID_ID));
        buf->slot.id = buf_id;
        buf->slot.state = SG_RESOURCESTATE_ALLOC;
    }
    return buf_id;
}

sg_id sg_alloc_image() {
    SOKOL_ASSERT(_sg);
    sg_id img_id = _sg_pool_alloc_id(&_sg->pools.pool[SG_RESOURCETYPE_IMAGE]);
    if (img_id != SG_INVALID_ID) {
        _sg_image* img = _sg_image_at(&_sg->pools, img_id);
        SOKOL_ASSERT(img && (img->slot.state == SG_RESOURCESTATE_INITIAL) && (img->slot.id == SG_INVALID_ID));
        img->slot.id = img_id;
        img->slot.state = SG_RESOURCESTATE_ALLOC;
    }
    return img_id;
}

sg_id sg_alloc_shader() {
    SOKOL_ASSERT(_sg);
    sg_id shd_id = _sg_pool_alloc_id(&_sg->pools.pool[SG_RESOURCETYPE_SHADER]);
    if (shd_id != SG_INVALID_ID) {
        _sg_shader* shd = _sg_shader_at(&_sg->pools, shd_id);
        SOKOL_ASSERT(shd && (shd->slot.state == SG_RESOURCESTATE_INITIAL) && (shd->slot.id == SG_INVALID_ID));
        shd->slot.id = shd_id;
        shd->slot.state = SG_RESOURCESTATE_ALLOC;
    }
    return shd_id;
}

sg_id sg_alloc_pipeline() {
    SOKOL_ASSERT(_sg);
    sg_id pip_id = _sg_pool_alloc_id(&_sg->pools.pool[SG_RESOURCETYPE_PIPELINE]);
    if (pip_id != SG_INVALID_ID) {
        _sg_pipeline* pip = _sg_pipeline_at(&_sg->pools, pip_id);
        SOKOL_ASSERT(pip && (pip->slot.state == SG_RESOURCESTATE_INITIAL) && (pip->slot.id == SG_INVALID_ID));
        pip->slot.id = pip_id;
        pip->slot.state = SG_RESOURCESTATE_ALLOC;
    }
    return pip_id;
}

sg_id sg_alloc_pass() {
    SOKOL_ASSERT(_sg);
    sg_id pass_id = _sg_pool_alloc_id(&_sg->pools.pool[SG_RESOURCETYPE_PASS]);
    if (pass_id != SG_INVALID_ID) {
        _sg_pass* pass = _sg_pass_at(&_sg->pools, pass_id);
        SOKOL_ASSERT(pass && (pass->slot.state == SG_RESOURCESTATE_INITIAL) && (pass->slot.id == SG_INVALID_ID));
        pass->slot.id = pass_id;
        pass->slot.state = SG_RESOURCESTATE_ALLOC;
    }
    return pass_id;
}

/*-- validate description structs --------------------------------------------*/
void _sg_validate_buffer_desc(sg_buffer_desc* desc) {
    SOKOL_ASSERT(_sg && desc);
    SOKOL_ASSERT(desc->size > 0);
    SOKOL_ASSERT((desc->type==SG_BUFFERTYPE_VERTEX_BUFFER)||(desc->type==SG_BUFFERTYPE_INDEX_BUFFER));
    SOKOL_ASSERT((desc->usage==SG_USAGE_IMMUTABLE)||(desc->usage==SG_USAGE_DYNAMIC)||(desc->usage==SG_USAGE_STREAM));
    SOKOL_ASSERT(desc->data_size <= desc->size);
    #ifdef _DEBUG
    if (desc->usage == SG_USAGE_IMMUTABLE) {
        SOKOL_ASSERT(desc->data_ptr);
    }
    #endif
}

void _sg_validate_image_desc(sg_image_desc* desc) {
    // FIXME
}

void _sg_validate_shader_desc(sg_shader_desc* desc) {
    SOKOL_ASSERT(_sg && desc);
    #if defined(SOKOL_USE_GL) || defined(SOKOL_USE_GLES2) || defined(SOKOL_USE_GLES3)
    SOKOL_ASSERT(desc->vs.source);
    SOKOL_ASSERT(desc->fs.source);
    #endif
    SOKOL_ASSERT(desc->num_attrs <= SG_MAX_VERTEX_ATTRIBUTES);
    for (int i = 0; i < desc->num_attrs; i++) {
        SOKOL_ASSERT(desc->attrs[i].name);
        SOKOL_ASSERT(desc->attrs[i].format != SG_VERTEXFORMAT_INVALID);
    }
}

void _sg_validate_pipeline_desc(sg_pipeline_desc* desc) {
    SOKOL_ASSERT(_sg && desc);
    SOKOL_ASSERT(desc->shader != SG_INVALID_ID);
    SOKOL_ASSERT(desc->layouts[0].num_attrs > 0);
    #ifdef _DEBUG
    int num_attrs = 0;
    for (int i = 0; i < SG_MAX_SHADERSTAGE_BUFFERS; i++) {
        num_attrs += desc->layouts[i].num_attrs;
    }
    SOKOL_ASSERT(num_attrs < SG_MAX_VERTEX_ATTRIBUTES);
    #endif
}

void _sg_validate_pass_desc(sg_pass_desc* desc) {
    // FIXME
}

/*-- initialize an allocated resource ----------------------------------------*/
void sg_init_buffer(sg_id buf_id, sg_buffer_desc* desc) {
    SOKOL_ASSERT(_sg && buf_id != SG_INVALID_ID && desc);
    _sg_validate_buffer_desc(desc);
    _sg_buffer* buf = _sg_lookup_buffer(&_sg->pools, buf_id);
    SOKOL_ASSERT(buf && buf->slot.state == SG_RESOURCESTATE_ALLOC);
    _sg_create_buffer(buf, desc);
    SOKOL_ASSERT((buf->slot.state == SG_RESOURCESTATE_VALID)||(buf->slot.state == SG_RESOURCESTATE_FAILED));
}

void sg_init_image(sg_id img_id, sg_image_desc* desc) {
    SOKOL_ASSERT(_sg && img_id != SG_INVALID_ID && desc);
    _sg_validate_image_desc(desc);
    _sg_image* img = _sg_lookup_image(&_sg->pools, img_id);
    SOKOL_ASSERT(img && img->slot.state == SG_RESOURCESTATE_ALLOC);
    _sg_create_image(img, desc);
    SOKOL_ASSERT((img->slot.state == SG_RESOURCESTATE_VALID)||(img->slot.state == SG_RESOURCESTATE_FAILED));
}

void sg_init_shader(sg_id shd_id, sg_shader_desc* desc) {
    SOKOL_ASSERT(_sg && shd_id != SG_INVALID_ID && desc);
    _sg_validate_shader_desc(desc);
    _sg_shader* shd = _sg_lookup_shader(&_sg->pools, shd_id);
    SOKOL_ASSERT(shd && shd->slot.state == SG_RESOURCESTATE_ALLOC);
    _sg_create_shader(shd, desc);
    SOKOL_ASSERT((shd->slot.state == SG_RESOURCESTATE_VALID)||(shd->slot.state == SG_RESOURCESTATE_FAILED));
}

void sg_init_pipeline(sg_id pip_id, sg_pipeline_desc* desc) {
    SOKOL_ASSERT(_sg && pip_id != SG_INVALID_ID && desc);
    _sg_validate_pipeline_desc(desc);
    _sg_pipeline* pip = _sg_lookup_pipeline(&_sg->pools, pip_id);
    SOKOL_ASSERT(pip && pip->slot.state == SG_RESOURCESTATE_ALLOC);
    _sg_shader* shd = _sg_lookup_shader(&_sg->pools, desc->shader);
    SOKOL_ASSERT(shd && shd->slot.state == SG_RESOURCESTATE_VALID);
    _sg_create_pipeline(pip, shd, desc);
    SOKOL_ASSERT((pip->slot.state == SG_RESOURCESTATE_VALID)||(pip->slot.state == SG_RESOURCESTATE_FAILED)); 
}

void sg_init_pass(sg_id pass_id, sg_pass_desc* desc) {
    SOKOL_ASSERT(_sg && pass_id != SG_INVALID_ID && desc);
    _sg_validate_pass_desc(desc);
    _sg_pass* pass = _sg_lookup_pass(&_sg->pools, pass_id);
    SOKOL_ASSERT(pass && pass->slot.state == SG_RESOURCESTATE_ALLOC);
    _sg_create_pass(pass, desc);
    SOKOL_ASSERT((pass->slot.state == SG_RESOURCESTATE_VALID)||(pass->slot.state == SG_RESOURCESTATE_FAILED)); 
}

/*-- allocate and initialize resource ----------------------------------------*/
sg_id sg_make_buffer(sg_buffer_desc* desc) {
    SOKOL_ASSERT(_sg && desc);
    sg_id buf_id = sg_alloc_buffer();
    if (buf_id != SG_INVALID_ID) {
        sg_init_buffer(buf_id, desc);
    }
    return buf_id;
}

sg_id sg_make_image(sg_image_desc* desc) {
    SOKOL_ASSERT(_sg && desc);
    sg_id img_id = sg_alloc_image();
    if (img_id != SG_INVALID_ID) {
        sg_init_image(img_id, desc);
    }
    return img_id;
}

sg_id sg_make_shader(sg_shader_desc* desc) {
    SOKOL_ASSERT(_sg && desc);
    sg_id shd_id = sg_alloc_shader();
    if (shd_id != SG_INVALID_ID) {
        sg_init_shader(shd_id, desc);
    }
    return shd_id;
}

sg_id sg_make_pipeline(sg_pipeline_desc* desc) {
    SOKOL_ASSERT(_sg && desc);
    sg_id pip_id = sg_alloc_pipeline();
    if (pip_id != SG_INVALID_ID) {
        sg_init_pipeline(pip_id, desc);
    }
    return pip_id;
}

sg_id sg_make_pass(sg_pass_desc* desc) {
    SOKOL_ASSERT(_sg && desc);
    sg_id pass_id = sg_alloc_pass();
    if (pass_id != SG_INVALID_ID) {
        sg_init_pass(pass_id, desc);
    }
    return pass_id;
}

/*-- destroy resource --------------------------------------------------------*/
void sg_destroy_buffer(sg_id buf_id) {
    SOKOL_ASSERT(_sg);
    _sg_buffer* buf = _sg_lookup_buffer(&_sg->pools, buf_id);
    if (buf) {
        _sg_destroy_buffer(buf);
        _sg_pool_free_id(&_sg->pools.pool[SG_RESOURCETYPE_BUFFER], buf_id);
    }
}

void sg_destroy_shader(sg_id shd_id) {
    SOKOL_ASSERT(_sg);
    _sg_shader* shd = _sg_lookup_shader(&_sg->pools, shd_id);
    if (shd) {
        _sg_destroy_shader(shd);
        _sg_pool_free_id(&_sg->pools.pool[SG_RESOURCETYPE_SHADER], shd_id);
    }
}

void sg_destroy_pipeline(sg_id pip_id) {
    SOKOL_ASSERT(_sg);
    _sg_pipeline* pip = _sg_lookup_pipeline(&_sg->pools, pip_id);
    if (pip) {
        _sg_destroy_pipeline(pip);
        _sg_pool_free_id(&_sg->pools.pool[SG_RESOURCETYPE_PIPELINE], pip_id);
    }
}

void sg_begin_pass(sg_id pass_id, sg_pass_action* pass_action, int width, int height) {
    SOKOL_ASSERT(_sg);
    _sg_pass* pass = _sg_lookup_pass(&_sg->pools, pass_id);  // can be 0
    _sg_begin_pass(&_sg->backend, pass, pass_action, width, height);
}

void sg_end_pass() {
    _sg_end_pass(&_sg->backend);
}

void sg_commit() {
    _sg_commit(&_sg->backend);
} 
