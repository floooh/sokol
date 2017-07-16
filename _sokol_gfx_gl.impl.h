/*
    Sokol Gfx GL rendering backend
*/

enum {
    _SG_GL_NUM_UPDATE_SLOTS = 2,
};

#define _SG_GL_CHECK_ERROR() { /*FIXME*/ } 

/*-- type translation --------------------------------------------------------*/
static GLenum _sg_as_buffer_target(sg_buffer_type t) {
    switch (t) {
        case SG_BUFFERTYPE_VERTEX_BUFFER: return GL_ARRAY_BUFFER;
        default: return GL_ELEMENT_ARRAY_BUFFER;
    }
}

static GLenum _sg_as_usage(sg_usage u) {
    switch (u) {
        case SG_USAGE_IMMUTABLE:    return GL_STATIC_DRAW;
        case SG_USAGE_DYNAMIC:      return GL_DYNAMIC_DRAW;
        default:                    return GL_STREAM_DRAW;
    }
}

/*-- GL backend resource declarations ----------------------------------------*/
typedef struct {
    _sg_slot slot;
    int size;
    sg_buffer_type type;
    sg_usage usage;
    int num_slots;
    int active_slot;
    GLuint gl_buf[_SG_GL_NUM_UPDATE_SLOTS];
} _sg_buffer;

static void _sg_init_buffer(_sg_buffer* buf) {
    SOKOL_ASSERT(buf);
    _sg_init_slot(&buf->slot);
    buf->size = 0;
    buf->type = SG_BUFFERTYPE_VERTEX_BUFFER;
    buf->usage = SG_USAGE_IMMUTABLE;
    buf->num_slots = 0;
    buf->active_slot = 0;
    for (int i = 0; i < _SG_GL_NUM_UPDATE_SLOTS; i++) {
        buf->gl_buf[i] = 0;
    }
}

typedef struct {
    _sg_slot slot;
} _sg_image;

static void _sg_init_image(_sg_image* img) {
    SOKOL_ASSERT(img);
    _sg_init_slot(&img->slot);
}

typedef struct {
    _sg_slot slot;
} _sg_shader;

static void _sg_init_shader(_sg_shader* shd) {
    SOKOL_ASSERT(shd);
    _sg_init_slot(&shd->slot);
}

typedef struct {
    _sg_slot slot;
} _sg_pipeline;

static void _sg_init_pipeline(_sg_pipeline* pip) {
    SOKOL_ASSERT(pip);
    _sg_init_slot(&pip->slot);
}

typedef struct {
    _sg_slot slot;
} _sg_pass;

static void _sg_init_pass(_sg_pass* pass) {
    SOKOL_ASSERT(pass);
    _sg_init_slot(&pass->slot);
}

/*-- state cache implementation ----------------------------------------------*/
typedef struct {
    sg_depth_stencil_state ds;
    sg_blend_state blend;
    sg_rasterizer_state rast;
} _sg_state_cache;

static void _sg_init_state_cache(_sg_state_cache* state) {
    SOKOL_ASSERT(state);
    /* depth-stencil state */
    state->ds.stencil_front.fail_op         = SG_STENCILOP_KEEP;
    state->ds.stencil_front.depth_fail_op   = SG_STENCILOP_KEEP;
    state->ds.stencil_front.pass_op         = SG_STENCILOP_KEEP;
    state->ds.stencil_front.compare_func    = SG_COMPAREFUNC_ALWAYS;
    state->ds.stencil_back.fail_op          = SG_STENCILOP_KEEP;
    state->ds.stencil_back.depth_fail_op    = SG_STENCILOP_KEEP;
    state->ds.stencil_back.pass_op          = SG_STENCILOP_KEEP;
    state->ds.stencil_back.compare_func     = SG_COMPAREFUNC_ALWAYS;
    state->ds.depth_compare_func    = SG_COMPAREFUNC_ALWAYS;
    state->ds.depth_write_enabled   = false;
    state->ds.stencil_enabled       = false;
    state->ds.stencil_read_mask     = 0xFF;
    state->ds.stencil_write_mask    = 0xFF;
    state->ds.stencil_ref           = 0;
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_ALWAYS);
    glDepthMask(GL_FALSE);
    glDisable(GL_STENCIL_TEST);
    glStencilFunc(GL_ALWAYS, 0, 0xFFFFFFFF);
    glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);
    glStencilMask(0xFFFFFFFF);

    /* blend state */
    state->blend.enabled = false;
    state->blend.src_factor_rgb = SG_BLENDFACTOR_ONE;
    state->blend.dst_factor_rgb = SG_BLENDFACTOR_ZERO;
    state->blend.op_rgb = SG_BLENDOP_ADD;
    state->blend.src_factor_alpha = SG_BLENDFACTOR_ONE;
    state->blend.dst_factor_alpha = SG_BLENDFACTOR_ZERO;
    state->blend.op_alpha = SG_BLENDOP_ADD;
    state->blend.color_write_mask = SG_COLORMASK_RGBA;
    for (int i = 0; i < 4; i++) {
        state->blend.blend_color[i] = 1.0f;
    }
    glDisable(GL_BLEND);
    glBlendFuncSeparate(GL_ONE, GL_ZERO, GL_ONE, GL_ZERO);
    glBlendEquationSeparate(GL_FUNC_ADD, GL_FUNC_ADD);
    glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
    glBlendColor(1.0f, 1.0f, 1.0f, 1.0f);

    /* rasterizer state */
    state->rast.cull_face_enabled = false;
    state->rast.scissor_test_enabled = false;
    state->rast.dither_enabled = true;
    state->rast.alpha_to_coverage_enabled = false;
    state->rast.cull_face = SG_FACE_BACK;
    glDisable(GL_CULL_FACE);
    glFrontFace(GL_CW);
    glCullFace(GL_BACK);
    glDisable(GL_POLYGON_OFFSET_FILL);
    glDisable(GL_SCISSOR_TEST);
    glEnable(GL_DITHER);
    #if defined(SOKOL_USE_GL)
        glEnable(GL_MULTISAMPLE);
    #endif
}

/*-- main GL backend state and functions -------------------------------------*/
typedef struct {
    bool valid;
    bool in_pass;
    uint32_t frame_index;
    _sg_pass* cur_pass;
    _sg_pipeline* cur_pipeline;
    _sg_state_cache cache;
    #if !defined(SOKOL_USE_GLES2)
    GLuint vao; 
    #endif
} _sg_backend;

static void _sg_setup_backend(_sg_backend* state) {
    SOKOL_ASSERT(state);
    #if !defined(SOKOL_USE_GLES2)
    glGenVertexArrays(1, &state->vao);
    glBindVertexArray(state->vao);
    #endif
    state->in_pass = false;
    state->frame_index = 0;
    state->cur_pass = 0;
    state->valid = true;
    _sg_init_state_cache(&state->cache);
}

static void _sg_discard_backend(_sg_backend* state) {
    SOKOL_ASSERT(state);
    SOKOL_ASSERT(state->valid);
    #if !defined(SOKOL_USE_GLES2)
    glDeleteVertexArrays(1, &state->vao);
    state->vao = 0;
    #endif
    state->cur_pass = 0;
    state->valid = false;
}

/*-- GL backend resource creation and destruction ----------------------------*/
static void _sg_create_buffer(_sg_buffer* buf, sg_buffer_desc* desc) {
    SOKOL_ASSERT(buf && desc);
    SOKOL_ASSERT(buf->slot.state == SG_RESOURCESTATE_ALLOC);
    _SG_GL_CHECK_ERROR();
    buf->size = desc->size;
    buf->type = desc->type;
    buf->usage = desc->usage;
    if (desc->usage == SG_USAGE_IMMUTABLE) {
        buf->num_slots = 1;
    }
    else {
        buf->num_slots = _SG_GL_NUM_UPDATE_SLOTS;
    }
    buf->active_slot = 0;
    GLenum gl_target = _sg_as_buffer_target(buf->type);
    GLenum gl_usage  = _sg_as_usage(buf->usage);
    for (int slot = 0; slot < buf->num_slots; slot++) {
        GLuint gl_buf;
        glGenBuffers(1, &gl_buf);
        glBindBuffer(gl_target, gl_buf);
        glBufferData(gl_target, buf->size, 0, gl_usage);
        if (desc->data_ptr) {
            glBufferSubData(gl_target, 0, desc->data_size, desc->data_ptr);
        }
        buf->gl_buf[slot] = gl_buf;
    }
    _SG_GL_CHECK_ERROR();
    buf->slot.state = SG_RESOURCESTATE_VALID;
}

static void _sg_destroy_buffer(_sg_buffer* buf) {
    SOKOL_ASSERT(buf);
    for (int slot = 0; slot < buf->num_slots; slot++) {
        if (buf->gl_buf[slot]) {
            glDeleteBuffers(1, &buf->gl_buf[slot]);
        }
    }
    _sg_init_buffer(buf);
}

static void _sg_create_image(_sg_image* img, sg_image_desc* desc) {
    SOKOL_ASSERT(img && desc);
    // FIXME
    img->slot.state = SG_RESOURCESTATE_FAILED;
}

static void _sg_create_shader(_sg_shader* shd, sg_shader_desc* desc) {
    SOKOL_ASSERT(shd && desc);
    // FIXME
    shd->slot.state = SG_RESOURCESTATE_FAILED;
}

static void _sg_create_pipeline(_sg_pipeline* pip, sg_pipeline_desc* desc) {
    SOKOL_ASSERT(pip && desc);
    // FIXME
    pip->slot.state = SG_RESOURCESTATE_FAILED;
}

static void _sg_create_pass(_sg_pass* pass, sg_pass_desc* desc) {
    SOKOL_ASSERT(pass && desc);
    // FIXME
    pass->slot.state = SG_RESOURCESTATE_FAILED;
}

/*-- GL backend rendering functions ------------------------------------------*/
static void _sg_begin_pass(_sg_backend* state, _sg_pass* pass, sg_pass_action* action, int w, int h) {
    SOKOL_ASSERT(state);
    SOKOL_ASSERT(action);
    SOKOL_ASSERT(!state->in_pass);
    state->in_pass = true;
    state->cur_pass = pass;
    if (pass) {

    }
    else {
        /* default pass */
        /* FIXME: on some platforms default frame buffer isn't 0! */
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }
    glViewport(0, 0, w, h);
    if (state->cache.rast.scissor_test_enabled) {
        state->cache.rast.scissor_test_enabled = false;
        glDisable(GL_SCISSOR_TEST);
    }
    if (state->cache.blend.color_write_mask != SG_COLORMASK_RGBA) {
        state->cache.blend.color_write_mask = SG_COLORMASK_RGBA;
        glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
    }
    if (!state->cache.ds.depth_write_enabled) {
        state->cache.ds.depth_write_enabled = true;
        glDepthMask(GL_TRUE);
    }
    if (state->cache.ds.stencil_write_mask != 0xFF) {
        state->cache.ds.stencil_write_mask = 0xFF;
        glStencilMask(0xFF);
    }
    // FIXME: multiple-render-target!
    GLbitfield clear_mask = 0;
    if (action->actions & SG_PASSACTION_CLEAR_COLOR0) {
        clear_mask |= GL_COLOR_BUFFER_BIT;
        float* c = action->color[0];
        glClearColor(c[0], c[1], c[2], c[3]);
    }
    if (action->actions & SG_PASSACTION_CLEAR_DEPTH_STENCIL) {
        // FIXME: hmm separate depth/stencil clear?
        clear_mask |= GL_DEPTH_BUFFER_BIT|GL_STENCIL_BUFFER_BIT;
        #ifdef SOKOL_USE_GL
        glClearDepth(action->depth);
        #else
        glClearDepthf(action->depth);
        #endif
        glClearStencil(action->stencil);
    }
    if (0 != clear_mask) {
        glClear(clear_mask);
    }
}

static void _sg_end_pass(_sg_backend* state) {
    SOKOL_ASSERT(state);
    SOKOL_ASSERT(state->in_pass);
    // FIXME: bind default framebuffer
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    state->in_pass = false;
    state->cur_pass = 0;
}

static void _sg_commit(_sg_backend* state) {
    SOKOL_ASSERT(state);
    SOKOL_ASSERT(!state->in_pass);
    state->frame_index++;
}
