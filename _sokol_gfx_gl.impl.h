/*
    Sokol Gfx GL rendering backend
*/

/*-- buffer implementation ---------------------------------------------------*/
typedef struct {
    _sg_slot slot;
} _sg_buffer;

static void _sg_init_buffer(_sg_buffer* buf) {
    SOKOL_ASSERT(buf);
    _sg_init_slot(&buf->slot);
}

/*-- image implementation ----------------------------------------------------*/
typedef struct {
    _sg_slot slot;
} _sg_image;

static void _sg_init_image(_sg_image* img) {
    SOKOL_ASSERT(img);
    _sg_init_slot(&img->slot);
}

/*-- shader implementation ---------------------------------------------------*/
typedef struct {
    _sg_slot slot;
} _sg_shader;

static void _sg_init_shader(_sg_shader* shd) {
    SOKOL_ASSERT(shd);
    _sg_init_slot(&shd->slot);
}

/*-- pipeline implementation -------------------------------------------------*/
typedef struct {
    _sg_slot slot;
} _sg_pipeline;

static void _sg_init_pipeline(_sg_pipeline* pip) {
    SOKOL_ASSERT(pip);
    _sg_init_slot(&pip->slot);
}

/*-- pass implementation -----------------------------------------------------*/
typedef struct {
    _sg_slot slot;
} _sg_pass;

static void _sg_init_pass(_sg_pass* pass) {
    SOKOL_ASSERT(pass);
    _sg_init_slot(&pass->slot);
}

/*-- state cache implementation ----------------------------------------------*/
typedef struct {
    sg_stencil_op fail_op;
    sg_stencil_op depth_fail_op;
    sg_stencil_op pass_op;
    sg_compare_func compare_func;
} _sg_stencil_state;

typedef struct {
    _sg_stencil_state stencil_front;
    _sg_stencil_state stencil_back;
    sg_compare_func depth_compare_func;
    bool depth_write_enabled;
    bool stencil_enabled;
    uint8_t stencil_read_mask;
    uint8_t stencil_write_mask;
    uint8_t stencil_ref;
} _sg_depth_stencil_state;

typedef struct {
    bool enabled;
    sg_blend_factor src_factor_rgb;
    sg_blend_factor dst_factor_rgb;
    sg_blend_op op_rgb;
    sg_blend_factor src_factor_alpha;
    sg_blend_factor dst_factor_alpha;
    sg_blend_op op_alpha;
    uint8_t color_write_mask;
    float blend_color[4];
} _sg_blend_state;

typedef struct {
    bool cull_face_enabled;
    bool scissor_test_enabled;
    bool dither_enabled;
    bool alpha_to_coverage_enabled;
    sg_face cull_face;
} _sg_rasterizer_state;

typedef struct {
    _sg_depth_stencil_state ds;
    _sg_blend_state blend;
    _sg_rasterizer_state rast;
} _sg_state_cache;

static void _sg_init_state_cache(_sg_state_cache* state) {
    SOKOL_ASSERT(state);
    /* depth-stencil state */
    state->ds.stencil_front.fail_op         = SG_STENCILOP_KEEP;
    state->ds.stencil_front.depth_fail_op   = SG_STENCILOP_KEEP;
    state->ds.stencil_front.pass_op         = SG_STENCILOP_KEEP;
    state->ds.stencil_front.compare_func    = SG_ALWAYS;
    state->ds.stencil_back.fail_op          = SG_STENCILOP_KEEP;
    state->ds.stencil_back.depth_fail_op    = SG_STENCILOP_KEEP;
    state->ds.stencil_back.pass_op          = SG_STENCILOP_KEEP;
    state->ds.stencil_back.compare_func     = SG_ALWAYS;
    state->ds.depth_compare_func    = SG_ALWAYS;
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
