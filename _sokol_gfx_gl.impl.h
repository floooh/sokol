/*
    Sokol Gfx GL rendering backend
*/

enum {
    _SG_GL_NUM_UPDATE_SLOTS = 2,
};

#ifdef SOKOL_USE_GLES2
#define GL_UNSIGNED_INT_2_10_10_10_REV 0x8368
#endif

#define _SG_GL_CHECK_ERROR() { /*FIXME*/ } 

/*-- type translation --------------------------------------------------------*/
static GLenum _sg_gl_buffer_target(sg_buffer_type t) {
    switch (t) {
        case SG_BUFFERTYPE_VERTEX_BUFFER:   return GL_ARRAY_BUFFER;
        case SG_BUFFERTYPE_INDEX_BUFFER:    return GL_ELEMENT_ARRAY_BUFFER;
    }
}

static GLenum _sg_gl_usage(sg_usage u) {
    switch (u) {
        case SG_USAGE_IMMUTABLE:    return GL_STATIC_DRAW;
        case SG_USAGE_DYNAMIC:      return GL_DYNAMIC_DRAW;
        case SG_USAGE_STREAM:       return GL_STREAM_DRAW;
    }
}

static GLenum _sg_gl_shader_stage(sg_shader_stage stage) {
    switch (stage) {
        case SG_SHADERSTAGE_VS:     return GL_VERTEX_SHADER;
        case SG_SHADERSTAGE_FS:     return GL_FRAGMENT_SHADER;
    }
}

static GLint _sg_gl_vertexformat_size(sg_vertex_format fmt) {
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

static GLenum _sg_gl_vertexformat_type(sg_vertex_format fmt) {
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

static GLboolean _sg_gl_vertexformat_normalized(sg_vertex_format fmt) {
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

static GLenum _sg_gl_primitive_type(sg_primitive_type t) {
    switch (t) {
        case SG_PRIMITIVETYPE_POINTS:           return GL_POINTS;
        case SG_PRIMITIVETYPE_LINES:            return GL_LINES;
        case SG_PRIMITIVETYPE_LINE_STRIP:       return GL_LINE_STRIP;
        case SG_PRIMITIVETYPE_TRIANGLES:        return GL_TRIANGLES;
        case SG_PRIMITIVETYPE_TRIANLE_STRIP:    return GL_TRIANGLE_STRIP;
    }
}

static GLenum _sg_gl_index_type(sg_index_type t) {
    switch (t) {
        case SG_INDEXTYPE_NONE:     return 0;
        case SG_INDEXTYPE_UINT16:   return GL_UNSIGNED_SHORT;
        case SG_INDEXTYPE_UINT32:   return GL_UNSIGNED_INT;
    }
}

static GLenum _sg_gl_compare_func(sg_compare_func cmp) {
    switch (cmp) {
        case SG_COMPAREFUNC_NEVER:          return GL_NEVER;
        case SG_COMPAREFUNC_LESS:           return GL_LESS;
        case SG_COMPAREFUNC_EQUAL:          return GL_EQUAL;
        case SG_COMPAREFUNC_LESS_EQUAL:     return GL_LEQUAL;
        case SG_COMPAREFUNC_GREATER:        return GL_GREATER;
        case SG_COMPAREFUNC_NOT_EQUAL:      return GL_NOTEQUAL;
        case SG_COMPAREFUNC_GREATER_EQUAL:  return GL_GEQUAL;
        case SG_COMPAREFUNC_ALWAYS:         return GL_ALWAYS;
    }
}

static GLenum _sg_gl_stencil_op(sg_stencil_op op) {
    switch (op) {
        case SG_STENCILOP_KEEP:         return GL_KEEP;
        case SG_STENCILOP_ZERO:         return GL_ZERO;
        case SG_STENCILOP_REPLACE:      return GL_REPLACE;
        case SG_STENCILOP_INCR_CLAMP:   return GL_INCR;
        case SG_STENCILOP_DECR_CLAMP:   return GL_DECR;
        case SG_STENCILOP_INVERT:       return GL_INVERT;
        case SG_STENCILOP_INCR_WRAP:    return GL_INCR_WRAP;
        case SG_STENCILOP_DECR_WRAP:    return GL_DECR_WRAP;
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
    int8_t index;
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
    sg_primitive_type primitive_type;
    sg_index_type index_type;
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
    bool next_draw_valid;
    uint32_t frame_index;
    GLenum cur_primitive_type;
    GLenum cur_index_type; 
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
    state->next_draw_valid = false;
    state->frame_index = 0;
    state->cur_primitive_type = GL_TRIANGLES;
    state->cur_index_type = 0;
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
    if (!(gl_vs && gl_fs)) {
        shd->slot.state = SG_RESOURCESTATE_FAILED;
        return;
    }
    GLuint gl_prog = glCreateProgram();
    glAttachShader(gl_prog, gl_vs);
    glAttachShader(gl_prog, gl_fs);
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
    SOKOL_ASSERT(shd->gl_prog);

    pip->shader = shd;
    pip->shader_id = desc->shader;
    pip->primitive_type = desc->primitive_type;
    pip->index_type = desc->index_type;
    pip->depth_stencil = desc->depth_stencil;
    pip->blend = desc->blend;
    pip->rast = desc->rast;
    
    /* resolve vertex attributes */
    pip->num_attrs = 0;
    for (int slot = 0; slot < SG_MAX_SHADERSTAGE_BUFFERS; slot++) {
        sg_vertex_layout_desc* layout = &desc->layouts[slot];
        int layout_byte_size = _sg_vertexlayout_byte_size(layout);
        for (int i = 0; i < layout->num_attrs; i++, pip->num_attrs++) {
            SOKOL_ASSERT(pip->num_attrs < SG_MAX_VERTEX_ATTRIBUTES);
            SOKOL_ASSERT(layout->attrs[i].name);
            _sg_gl_attr* gl_attr = &pip->gl_attrs[pip->num_attrs];
            GLint attr_loc = glGetAttribLocation(pip->shader->gl_prog, layout->attrs[i].name);
            if (attr_loc != -1) {
                gl_attr->index = attr_loc;
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
                sg_vertex_format fmt = layout->attrs[i].format;
                gl_attr->size = _sg_gl_vertexformat_size(fmt);
                gl_attr->type = _sg_gl_vertexformat_type(fmt);
                gl_attr->normalized = _sg_gl_vertexformat_normalized(fmt);
            }
            else {
                /* shader doesn't know this attribute */
                gl_attr->index = -1;
                gl_attr->enabled = GL_FALSE;
            }
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
}

static void _sg_apply_draw_state(_sg_backend* state, 
    _sg_pipeline* pip, 
    _sg_buffer** vbs, int num_vbs, _sg_buffer* ib,
    _sg_image** vs_imgs, int num_vs_imgs,
    _sg_image** fs_imgs, int num_fs_imgs)
{
    SOKOL_ASSERT(state);
    SOKOL_ASSERT(pip);
    SOKOL_ASSERT(pip->shader);
    state->cur_primitive_type = _sg_gl_primitive_type(pip->primitive_type);
    state->cur_index_type = _sg_gl_index_type(pip->index_type);

    /* update depth-stencil state */
    const sg_depth_stencil_state* new_ds = &pip->depth_stencil;
    sg_depth_stencil_state* cache_ds = &state->cache.ds;
    if (new_ds->depth_compare_func != cache_ds->depth_compare_func) {
        cache_ds->depth_compare_func = new_ds->depth_compare_func;
        glDepthFunc(_sg_gl_compare_func(new_ds->depth_compare_func));
    }
    if (new_ds->depth_write_enabled != cache_ds->depth_write_enabled) {
        cache_ds->depth_write_enabled = new_ds->depth_write_enabled;
        glDepthMask(new_ds->depth_write_enabled);
    }
    if (new_ds->stencil_enabled != cache_ds->stencil_enabled) {
        cache_ds->stencil_enabled = new_ds->stencil_enabled;
        if (new_ds->stencil_enabled) glEnable(GL_STENCIL_TEST); 
        else glDisable(GL_STENCIL_TEST);
    }
    if (new_ds->stencil_write_mask != cache_ds->stencil_write_mask) {
        cache_ds->stencil_write_mask = new_ds->stencil_write_mask;
        glStencilMask(new_ds->stencil_write_mask);
    }
    for (int i = 0; i < 2; i++) {
        const sg_stencil_state* new_ss = (i==0)? &new_ds->stencil_front : &new_ds->stencil_back;
        sg_stencil_state* cache_ss = (i==0)? &cache_ds->stencil_front : &cache_ds->stencil_back;
        GLenum gl_face = (i==0)? GL_FRONT : GL_BACK;
        if ((new_ss->compare_func != cache_ss->compare_func) ||
            (new_ds->stencil_read_mask != cache_ds->stencil_read_mask) ||
            (new_ds->stencil_ref != cache_ds->stencil_ref))
        {
            cache_ss->compare_func = new_ss->compare_func;
            cache_ds->stencil_read_mask = new_ds->stencil_read_mask;
            cache_ds->stencil_ref = new_ds->stencil_ref;
            glStencilFuncSeparate(gl_face, 
                _sg_gl_compare_func(new_ss->compare_func), 
                new_ds->stencil_ref, 
                new_ds->stencil_read_mask);
        }
        if ((new_ss->fail_op != cache_ss->fail_op) ||
            (new_ss->depth_fail_op != cache_ss->depth_fail_op) ||
            (new_ss->pass_op != cache_ss->pass_op))
        {
            cache_ss->fail_op = new_ss->fail_op;
            cache_ss->depth_fail_op = new_ss->depth_fail_op;
            cache_ss->pass_op = new_ss->pass_op;
            glStencilOpSeparate(gl_face,
                _sg_gl_stencil_op(new_ss->fail_op),
                _sg_gl_stencil_op(new_ss->depth_fail_op),
                _sg_gl_stencil_op(new_ss->pass_op));
        }
    }

    /* FIXME: update blend state */
    /* FIXME: update rasterizer state */

    /* bind shader program */
    glUseProgram(pip->shader->gl_prog);

    /* FIXME: bind textures */

    /* bind index buffer (can be 0) */
    if (ib) {
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ib->gl_buf[ib->active_slot]);
    }
    else {
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    }
    /* bind vertex attributes */
    /* FIXME: caching! */ 
    GLuint gl_vb = 0;
    int attr_index = 0;
    SOKOL_ASSERT(pip->num_attrs < SG_MAX_VERTEX_ATTRIBUTES);
    for (attr_index = 0; attr_index < pip->num_attrs; attr_index++) {
        _sg_gl_attr* attr = &pip->gl_attrs[attr_index];
        if (attr->enabled) {
            SOKOL_ASSERT(attr->index >= 0);
            SOKOL_ASSERT(attr->vb_index < num_vbs);
            _sg_buffer* vb = vbs[attr->vb_index];
            SOKOL_ASSERT(vb);
            if (gl_vb != vb->gl_buf[vb->active_slot]) {
                gl_vb = vb->gl_buf[vb->active_slot];
                glBindBuffer(GL_ARRAY_BUFFER, gl_vb);
            }
            glVertexAttribPointer(attr->index, attr->size, attr->type, 
                attr->normalized, attr->stride, 
                (const GLvoid*)(GLintptr)attr->offset);
            glEnableVertexAttribArray(attr->index);
        }
        else {
            // FIXME: caching!
            glDisableVertexAttribArray(attr->index);
        }
        // FIXME: caching!
        // FIXME: GL Extensions!
        #ifndef SOKOL_USE_GLES2
        glVertexAttribDivisor(attr->index, attr->divisor);
        #endif
    }
    /* FIXME: caching! */
    for (; attr_index < SG_MAX_VERTEX_ATTRIBUTES; attr_index++) {
        glDisableVertexAttribArray(attr_index);
    }
}

static void _sg_draw(_sg_backend* state, int base_element, int num_elements, int num_instances) {
    SOKOL_ASSERT(state);
    if (!state->next_draw_valid) {
        return;
    }
    const GLenum i_type = state->cur_index_type;
    const GLenum p_type = state->cur_primitive_type;
    if (0 != i_type) {
        /* indexed rendering */
        const int i_size = (i_type == GL_UNSIGNED_SHORT) ? 2 : 4;
        const GLvoid* indices = (const GLvoid*)(GLintptr)(base_element*i_size);
        if (num_instances == 1) {
            glDrawElements(p_type, num_elements, i_type, indices);
        }
        else {
            #ifndef SOKOL_USE_GLES2
            /* FIXME! */
            glDrawElementsInstanced(p_type, num_elements, i_type, indices, num_instances);
            #endif
        }
    }
    else {
        /* non-indexed rendering */
        if (num_instances == 1) {
            glDrawArrays(p_type, base_element, num_elements);
        }
        else {
            #ifndef SOKOL_USE_GLES2
            /* FIXME! */
            glDrawArraysInstanced(p_type, base_element, num_elements, num_instances);
            #endif
        }
    }
}

static void _sg_commit(_sg_backend* state) {
    SOKOL_ASSERT(state);
    SOKOL_ASSERT(!state->in_pass);
    state->frame_index++;
}
