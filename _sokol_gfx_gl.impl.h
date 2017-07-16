/*
    Sokol Gfx GL rendering backend
*/

enum {
    _SG_GL_NUM_UPDATE_SLOTS = 2,
};

#define _SG_GL_CHECK_ERROR() { /*FIXME*/ } 

/*-- type translation --------------------------------------------------------*/
static GLenum _sg_gl_buffer_target(sg_buffer_type t) {
    switch (t) {
        case SG_BUFFERTYPE_VERTEX_BUFFER: return GL_ARRAY_BUFFER;
        default: return GL_ELEMENT_ARRAY_BUFFER;
    }
}

static GLenum _sg_gl_usage(sg_usage u) {
    switch (u) {
        case SG_USAGE_IMMUTABLE:    return GL_STATIC_DRAW;
        case SG_USAGE_DYNAMIC:      return GL_DYNAMIC_DRAW;
        default:                    return GL_STREAM_DRAW;
    }
}

static GLenum _sg_gl_shader_stage(sg_shader_stage stage) {
    switch (stage) {
        case SG_SHADERSTAGE_VS:     return GL_VERTEX_SHADER;
        default:                    return GL_FRAGMENT_SHADER;
    }
}

static GLint _sg_gl_vertexformat_size(fmt) {
    switch (fmt) {
        case SG_VERTEXFORMAT_FLOAT:     return 1;
        case SG_VERTEXFORMAT_FLOAT2:    return 2;
        case SG_VERTEXFORMAT_FLOAT3:    return 3;
        case SG_VERTEXFORMAT_FLOAT4:    return 4;
        case SG_VERTEXFORMAT_BYTE4:     return 4;
        case SG_VERTEXFORMAT_BYTE4N:    return 4;
        case SG_VERTEXFORMAT_UBYTE4:    return 4;
        case SG_VERTEXFORMAT_UBYTE4N:   return 4;
        case SG_VERTEXFORMAT_SHORT2:    return 2;
        case SG_VERTEXFORMAT_SHORT2N:   return 2;
        case SG_VERTEXFORMAT_SHORT4:    return 4;
        case SG_VERTEXFORMAT_SHORT4N:   return 4;
        case SG_VERTEXFORMAT_UINT10_N2: return 4;
        default:    return 0;
    }
}

static GLenum _sg_gl_vertexformat_type(fmt) {
    switch (fmt) {
        case SG_VERTEXFORMAT_FLOAT:
        case SG_VERTEXFORMAT_FLOAT2:
        case SG_VERTEXFORMAT_FLOAT3:
        case SG_VERTEXFORMAT_FLOAT4:
            return GL_FLOAT;
        case SG_VERTEXFORMAT_BYTE4:
        case SG_VERTEXFORMAT_BYTE4N:
            return GL_BYTE;
        case SG_VERTEXFORMAT_UBYTE4:
        case SG_VERTEXFORMAT_UBYTE4N:
            return GL_UNSIGNED_BYTE;
        case SG_VERTEXFORMAT_SHORT2:
        case SG_VERTEXFORMAT_SHORT2N:
        case SG_VERTEXFORMAT_SHORT4:
        case SG_VERTEXFORMAT_SHORT4N:
            return GL_SHORT;
        case SG_VERTEXFORMAT_UINT10_N2:
            return GL_UNSIGNED_INT_2_10_10_10_REV;
        default:
            return 0;
    }
}

static GLboolean _sg_gl_vertexformat_normalized(fmt) {
    switch (fmt) {
        case SG_VERTEXFORMAT_BYTE4N:
        case SG_VERTEXFORMAT_UBYTE4N:
        case SG_VERTEXFORMAT_SHORT2N:
        case SG_VERTEXFORMAT_SHORT4N:
        case SG_VERTEXFORMAT_UINT10_N2:
            return GL_TRUE;
        default:
            return GL_FALSE;
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
    GLuint gl_prog;
} _sg_shader;

static void _sg_init_shader(_sg_shader* shd) {
    SOKOL_ASSERT(shd);
    _sg_init_slot(&shd->slot);
    shd->gl_prog = 0;
}

typedef struct {
    uint8_t index;
    uint8_t enabled;
    uint8_t vb_index;
    uint8_t divisor;
    uint8_t stride;
    uint8_t size;
    uint8_t normalized;
    uint32_t offset;
    GLenum type;
} _sg_gl_attr;

typedef struct {
    _sg_slot slot;
    _sg_shader* shader;
    sg_id shader_id;
    int num_attrs;
    _sg_gl_attr gl_attrs[SG_MAX_VERTEX_ATTRIBUTES];
    sg_depth_stencil_state depth_stencil;
    sg_blend_state blend;
    sg_rasterizer_state rast;
} _sg_pipeline;

static void _sg_init_pipeline(_sg_pipeline* pip) {
    SOKOL_ASSERT(pip);
    _sg_init_slot(&pip->slot);
    pip->shader = 0;
    pip->shader_id = SG_INVALID_ID;
    pip->num_attrs = 0;
    for (int i = 0; i < SG_MAX_VERTEX_ATTRIBUTES; i++) {
        _sg_gl_attr* attr = &pip->gl_attrs[i];
        attr->index = 0;
        attr->enabled = 0;
        attr->vb_index = 0;
        attr->divisor = 0;
        attr->stride = 0;
        attr->normalized = 0;
        attr->offset = 0;
        attr->type = 0;
    }
    _sg_init_depth_stencil_state(&pip->depth_stencil);
    _sg_init_blend_state(&pip->blend);
    _sg_init_rasterizer_state(&pip->rast);
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
    _sg_init_depth_stencil_state(&state->ds);
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_ALWAYS);
    glDepthMask(GL_FALSE);
    glDisable(GL_STENCIL_TEST);
    glStencilFunc(GL_ALWAYS, 0, 0xFFFFFFFF);
    glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);
    glStencilMask(0xFFFFFFFF);

    /* blend state */
    _sg_init_blend_state(&state->blend);
    glDisable(GL_BLEND);
    glBlendFuncSeparate(GL_ONE, GL_ZERO, GL_ONE, GL_ZERO);
    glBlendEquationSeparate(GL_FUNC_ADD, GL_FUNC_ADD);
    glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
    glBlendColor(1.0f, 1.0f, 1.0f, 1.0f);

    /* rasterizer state */
    _sg_init_rasterizer_state(&state->rast);
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
    GLenum gl_target = _sg_gl_buffer_target(buf->type);
    GLenum gl_usage  = _sg_gl_usage(buf->usage);
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

static GLuint _sg_compile_shader(sg_shader_stage stage, const char* src) {
    SOKOL_ASSERT(src);
    _SG_GL_CHECK_ERROR();
    GLuint gl_shd = glCreateShader(_sg_gl_shader_stage(stage));
    int src_len = strlen(src);
    glShaderSource(gl_shd, 1, &src, &src_len);
    glCompileShader(gl_shd);
    GLint compile_status = 0;
    glGetShaderiv(gl_shd, GL_COMPILE_STATUS, &compile_status);
    // FIXME: error logging
    if (!compile_status) {
        /* compilation failed */
        glDeleteShader(gl_shd);
        gl_shd = 0;
    }
    _SG_GL_CHECK_ERROR();
    return gl_shd;
}

static void _sg_create_shader(_sg_shader* shd, sg_shader_desc* desc) {
    SOKOL_ASSERT(shd && desc);
    SOKOL_ASSERT(!shd->gl_prog);
    _SG_GL_CHECK_ERROR();
    GLuint gl_vs = _sg_compile_shader(SG_SHADERSTAGE_VS, desc->vs.source);
    GLuint gl_fs = _sg_compile_shader(SG_SHADERSTAGE_FS, desc->fs.source);
    if (!(gl_vs && gl_vs)) {
        shd->slot.state = SG_RESOURCESTATE_FAILED;
        return;
    }
    GLuint gl_prog = glCreateProgram();
    glAttachShader(gl_prog, gl_vs);
    glAttachShader(gl_prog, gl_fs);
    for (int attr_index = 0; attr_index < desc->num_attrs; attr_index++) {
        glBindAttribLocation(gl_prog, attr_index, desc->attrs[attr_index].name);
    }
    glLinkProgram(gl_prog);
    glDeleteShader(gl_vs);
    glDeleteShader(gl_fs);
    _SG_GL_CHECK_ERROR();

    GLint link_status;
    glGetProgramiv(gl_prog, GL_LINK_STATUS, &link_status);
    if (!link_status) {
        // FIXME: error logging
        glDeleteProgram(gl_prog);
        shd->slot.state = SG_RESOURCESTATE_FAILED;
        return;
    }
    shd->gl_prog = gl_prog;

    // FIXME: resolve uniform and texture locations
    // FIXME: use GetAttribLocation (if config-defined)

    shd->slot.state = SG_RESOURCESTATE_VALID;
}

static void _sg_destroy_shader(_sg_shader* shd) {
    SOKOL_ASSERT(shd);
    if (shd->gl_prog) {
        glDeleteShader(shd->gl_prog);
    }
    _sg_init_shader(shd);
}

static void _sg_create_pipeline(_sg_pipeline* pip, _sg_shader* shd, sg_pipeline_desc* desc) {
    SOKOL_ASSERT(pip && desc);
    SOKOL_ASSERT(!pip->shader && pip->shader_id == SG_INVALID_ID);
    SOKOL_ASSERT(pip->num_attrs == 0);
    SOKOL_ASSERT(desc->shader == shd->slot.id);

    pip->shader = shd;
    pip->shader_id = desc->shader;
    pip->depth_stencil = desc->depth_stencil;
    pip->blend = desc->blend;
    pip->rast = desc->rast;
    
    // FIXME: hmmmmm use glGetAttribLocation here?
    pip->num_attrs = 0;
    for (int slot = 0; slot < SG_MAX_SHADERSTAGE_BUFFERS; slot++) {
        sg_vertex_layout_desc* layout = &desc->layouts[slot];
        int layout_byte_size = _sg_vertexlayout_byte_size(layout);
        for (int i = 0; i < layout->num_attrs; i++) {
            SOKOL_ASSERT(pip->num_attrs < SG_MAX_VERTEX_ATTRIBUTES);
            _sg_gl_attr* gl_attr = &pip->gl_attrs[pip->num_attrs];
            sg_vertex_format fmt = layout->attrs[i].format;
            gl_attr->index = pip->num_attrs++;
            gl_attr->enabled = GL_TRUE;
            gl_attr->vb_index = slot;
            if (layout->step_func == SG_STEPFUNC_PER_VERTEX) {
                gl_attr->divisor = 0;
            }
            else {
                gl_attr->divisor = layout->step_rate;
            }
            gl_attr->stride = layout_byte_size;
            gl_attr->offset = _sg_vertexlayout_attr_offset(layout, i);
            gl_attr->size = _sg_gl_vertexformat_size(fmt);
            gl_attr->type = _sg_gl_vertexformat_type(fmt);
            gl_attr->normalized = _sg_gl_vertexformat_normalized(fmt);
        }
    }
    pip->slot.state = SG_RESOURCESTATE_VALID;
}

static void _sg_destroy_pipeline(_sg_pipeline* pip) {
    SOKOL_ASSERT(pip);
    _sg_init_pipeline(pip);
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
