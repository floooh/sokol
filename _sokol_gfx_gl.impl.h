/*
    Sokol Gfx GL rendering backend
*/
#ifndef SOKOL_IMPL_GUARD
#error "Please do not include *.impl.h files directly"
#endif

enum {
    _SG_GL_NUM_UPDATE_SLOTS = 2,
};

#ifndef GL_UNSIGNED_INT_2_10_10_10_REV
#define GL_UNSIGNED_INT_2_10_10_10_REV 0x8368
#endif

#define _SG_GL_CHECK_ERROR() { SOKOL_ASSERT(glGetError() == GL_NO_ERROR); } 

/*-- type translation --------------------------------------------------------*/
static GLenum _sg_gl_buffer_target(sg_buffer_type t) {
    switch (t) {
        case SG_BUFFERTYPE_VERTEXBUFFER:    return GL_ARRAY_BUFFER;
        case SG_BUFFERTYPE_INDEXBUFFER:     return GL_ELEMENT_ARRAY_BUFFER;
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

static GLenum _sg_gl_blend_factor(sg_blend_factor f) {
    switch (f) {
        case SG_BLENDFACTOR_ZERO:                   return GL_ZERO;
        case SG_BLENDFACTOR_ONE:                    return GL_ONE;
        case SG_BLENDFACTOR_SRC_COLOR:              return GL_SRC_COLOR;
        case SG_BLENDFACTOR_ONE_MINUS_SRC_COLOR:    return GL_ONE_MINUS_SRC_COLOR;
        case SG_BLENDFACTOR_SRC_ALPHA:              return GL_SRC_ALPHA;
        case SG_BLENDFACTOR_ONE_MINUS_SRC_ALPHA:    return GL_ONE_MINUS_SRC_ALPHA;
        case SG_BLENDFACTOR_DST_COLOR:              return GL_DST_COLOR;
        case SG_BLENDFACTOR_ONE_MINUS_DST_COLOR:    return GL_ONE_MINUS_DST_COLOR;
        case SG_BLENDFACTOR_DST_ALPHA:              return GL_DST_ALPHA;
        case SG_BLENDFACTOR_ONE_MINUS_DST_ALPHA:    return GL_ONE_MINUS_DST_ALPHA;
        case SG_BLENDFACTOR_SRC_ALPHA_SATURATED:    return GL_SRC_ALPHA_SATURATE;
        case SG_BLENDFACTOR_BLEND_COLOR:            return GL_CONSTANT_COLOR;
        case SG_BLENDFACTOR_ONE_MINUS_BLEND_COLOR:  return GL_ONE_MINUS_CONSTANT_COLOR;
        case SG_BLENDFACTOR_BLEND_ALPHA:            return GL_CONSTANT_ALPHA;
        case SG_BLENDFACTOR_ONE_MINUS_BLEND_ALPHA:  return GL_ONE_MINUS_CONSTANT_ALPHA;
    }
}

static GLenum _sg_gl_blend_op(sg_blend_op op) {
    switch (op) {
        case SG_BLENDOP_ADD:                return GL_FUNC_ADD;
        case SG_BLENDOP_SUBTRACT:           return GL_FUNC_SUBTRACT;
        case SG_BLENDOP_REVERSE_SUBTRACT:   return GL_FUNC_REVERSE_SUBTRACT;
    }
}

static GLenum _sg_gl_cull_face(sg_face f) {
    switch (f) {
        case SG_FACE_FRONT: return GL_FRONT;
        case SG_FACE_BACK:  return GL_BACK;
        case SG_FACE_BOTH:  return GL_FRONT_AND_BACK;
    }
}

/*-- GL backend resource declarations ----------------------------------------*/
typedef struct {
    _sg_slot slot;
    int size;
    sg_buffer_type type;
    sg_usage usage;
    uint32_t upd_frame_index;
    int num_slots;
    int active_slot;
    GLuint gl_buf[_SG_GL_NUM_UPDATE_SLOTS];
} _sg_buffer;

static void _sg_init_buffer(_sg_buffer* buf) {
    SOKOL_ASSERT(buf);
    _sg_init_slot(&buf->slot);
    buf->size = 0;
    buf->type = SG_BUFFERTYPE_VERTEXBUFFER;
    buf->usage = SG_USAGE_IMMUTABLE;
    buf->upd_frame_index = 0;
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
    GLint gl_loc;
    sg_uniform_type type;
    int offset;
    int count;
} _sg_uniform;

typedef struct {
    int size;
    int num_uniforms;
    _sg_uniform uniforms[SG_MAX_UNIFORMS];
} _sg_uniform_block;

typedef struct {
    int num_uniform_blocks;
    _sg_uniform_block uniform_blocks[SG_MAX_SHADERSTAGE_UBS];
    /* FIXME: shader image */
} _sg_shader_stage;

typedef struct {
    _sg_slot slot;
    GLuint gl_prog;
    _sg_shader_stage stage[SG_NUM_SHADER_STAGES];
} _sg_shader;

static void _sg_init_shader(_sg_shader* shd) {
    SOKOL_ASSERT(shd);
    _sg_init_slot(&shd->slot);
    shd->gl_prog = 0;
    for (int stage_index = 0; stage_index < SG_NUM_SHADER_STAGES; stage_index++) {
        _sg_shader_stage* stage = &shd->stage[stage_index];
        stage->num_uniform_blocks = 0;
        for (int ub_index = 0; ub_index < SG_MAX_SHADERSTAGE_UBS; ub_index++) {
            _sg_uniform_block* ub = &stage->uniform_blocks[ub_index];
            ub->size = 0;
            ub->num_uniforms = 0;
            for (int u_index = 0; u_index < SG_MAX_UNIFORMS; u_index++) {
                _sg_uniform* u = &ub->uniforms[u_index];
                u->gl_loc = 0;
                u->type = SG_UNIFORMTYPE_INVALID;
                u->offset = 0;
                u->count = 0;
            }
        }
    }
}

typedef struct {
    int8_t vb_index;        /* -1 if attr is not enabled */
    int8_t divisor;         /* -1 if not initialized */
    uint8_t stride;
    uint8_t size;
    uint8_t normalized;
    uint8_t offset;
    GLenum type;
} _sg_gl_attr;

static void _sg_init_gl_attr(_sg_gl_attr* attr) {
    attr->vb_index = -1;
    attr->divisor = -1;
    attr->stride = 0;
    attr->size = 0;
    attr->normalized = 0;
    attr->offset = 0;
    attr->type = 0;
}

typedef struct {
    _sg_slot slot;
    _sg_shader* shader;
    sg_id shader_id;
    sg_primitive_type primitive_type;
    sg_index_type index_type;
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
    for (int i = 0; i < SG_MAX_VERTEX_ATTRIBUTES; i++) {
        _sg_init_gl_attr(&pip->gl_attrs[i]);
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
    _sg_gl_attr attrs[SG_MAX_VERTEX_ATTRIBUTES];
} _sg_state_cache;

static void _sg_init_state_cache(_sg_state_cache* state) {
    SOKOL_ASSERT(state);
    
    for (int i = 0; i < SG_MAX_VERTEX_ATTRIBUTES; i++) {
        _sg_init_gl_attr(&state->attrs[i]);
        glDisableVertexAttribArray(i);
    }

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
    #if defined(SOKOL_USE_GLCORE33)
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
    _sg_pipeline* cur_pipeline;
    sg_id cur_pipeline_id; 
    _sg_state_cache cache;
    bool features[SG_NUM_FEATURES];
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
    state->frame_index = 1;
    state->cur_primitive_type = GL_TRIANGLES;
    state->cur_index_type = 0;
    state->cur_pipeline = 0;
    state->cur_pipeline_id = SG_INVALID_ID;
    state->valid = true;
    _sg_init_state_cache(&state->cache);
    
    /* initialize feature flags */
    for (int i = 0; i < SG_NUM_FEATURES; i++) {
        state->features[i] = false;
    }
    state->features[SG_FEATURE_ORIGIN_BOTTOM_LEFT] = true;
    #if !defined(SOKOL_USE_GLCORE33)
        const char* ext = (const char*) glGetString(GL_EXTENSIONS);
        state->features[SG_FEATURE_TEXTURE_COMPRESSION_DXT] =
            strstr(ext, "_texture_compression_s3tc") ||
            strstr(ext, "_compressed_texture_s3tc") ||
            strstr(ext, "texture_compression_dxt1");
        state->features[SG_FEATURE_TEXTURE_COMPRESSION_PVRTC] =
            strstr(ext, "_texture_compression_pvrtc") ||
            strstr(ext, "_compressed_texture_pvrtc");
        state->features[SG_FEATURE_TEXTURE_COMPRESSION_ATC] = strstr(ext, "_compressed_texture_atc");
        state->features[SG_FEATURE_TEXTURE_FLOAT] = strstr(ext, "_texture_float");
        state->features[SG_FEATURE_INSTANCED_ARRAYS] = strstr(ext, "_instanced_arrays");
        #if defined(SOKOL_USE_GLES2)
            state->features[SG_FEATURE_TEXTURE_HALF_FLOAT] = strstr(ext, "_texture_half_float");
        #else
            state->features[SG_FEATURE_TEXTURE_HALF_FLOAT] = state->features[SG_FEATURE_TEXTURE_FLOAT];
        #endif
    #endif
    #if defined(SOKOL_USE_GLCORE33) || defined(SOKOL_USE_GLES3)
        #if defined(SOKOL_USE_GLCORE33)
        state->features[SG_FEATURE_TEXTURE_COMPRESSION_DXT] = true;
        #endif
        state->features[SG_FEATURE_INSTANCED_ARRAYS] = true;
        state->features[SG_FEATURE_TEXTURE_FLOAT] = true;
        state->features[SG_FEATURE_TEXTURE_HALF_FLOAT] = true;
        state->features[SG_FEATURE_MSAA_RENDER_TARGETS] = true;
        state->features[SG_FEATURE_PACKED_VERTEX_FORMAT_10_2] = true;
        state->features[SG_FEATURE_MULTIPLE_RENDER_TARGET] = true;
        state->features[SG_FEATURE_TEXTURE_3D] = true;
        state->features[SG_FEATURE_TEXTURE_ARRAY] = true;
    #endif
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

static bool _sg_query_feature(_sg_backend* state, sg_feature f) {
    SOKOL_ASSERT(state && (f>=0) && (f<SG_NUM_FEATURES));
    return state->features[f];
}

/*-- GL backend resource creation and destruction ----------------------------*/
static void _sg_create_buffer(_sg_buffer* buf, const sg_buffer_desc* desc) {
    SOKOL_ASSERT(buf && desc);
    SOKOL_ASSERT(buf->slot.state == SG_RESOURCESTATE_ALLOC);
    SOKOL_ASSERT(desc->data_size <= desc->size);
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

static void _sg_create_image(_sg_image* img, const sg_image_desc* desc) {
    SOKOL_ASSERT(img && desc);
    /* FIXME */
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
    if (!compile_status) {
        /* compilation failed, log error and delete shader */
        GLint log_len = 0;
        glGetShaderiv(gl_shd, GL_INFO_LOG_LENGTH, &log_len);
        if (log_len > 0) {
            GLchar* log_buf = SOKOL_MALLOC(log_len);
            glGetShaderInfoLog(gl_shd, log_len, &log_len, log_buf);
            SOKOL_LOG(log_buf);
            SOKOL_FREE(log_buf);
        }
        glDeleteShader(gl_shd);
        gl_shd = 0;
    }
    _SG_GL_CHECK_ERROR();
    return gl_shd;
}

static void _sg_create_shader(_sg_shader* shd, const sg_shader_desc* desc) {
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
        GLint log_len = 0;
        glGetProgramiv(gl_prog, GL_INFO_LOG_LENGTH, &log_len);
        if (log_len > 0) {
            GLchar* log_buf = SOKOL_MALLOC(log_len);
            glGetProgramInfoLog(gl_prog, log_len, &log_len, log_buf);
            SOKOL_LOG(log_buf);
            SOKOL_FREE(log_buf);
        }
        glDeleteProgram(gl_prog);
        shd->slot.state = SG_RESOURCESTATE_FAILED;
        return;
    }
    shd->gl_prog = gl_prog;

    /* resolve uniforms */
    for (int stage_index = 0; stage_index < SG_NUM_SHADER_STAGES; stage_index++) {
        const sg_shader_stage_desc* stage_desc = (stage_index == SG_SHADERSTAGE_VS)? &desc->vs : &desc->fs;
        _sg_shader_stage* stage = &shd->stage[stage_index];
        SOKOL_ASSERT(stage->num_uniform_blocks == 0);
        for (int ub_index = 0; ub_index < SG_MAX_SHADERSTAGE_UBS; ub_index++) {
            const sg_shader_uniform_block_desc* ub_desc = &stage_desc->ub[ub_index];
            if (ub_desc->size == 0) {
                break;
            }
            _sg_uniform_block* ub = &stage->uniform_blocks[stage->num_uniform_blocks++];
            ub->size = ub_desc->size;
            SOKOL_ASSERT(ub->num_uniforms == 0);
            for (int u_index = 0; u_index < SG_MAX_UNIFORMS; u_index++) {
                const sg_shader_uniform_desc* u_desc = &ub_desc->u[u_index];
                if (u_desc->type == SG_UNIFORMTYPE_INVALID) {
                    break;
                }
                _sg_uniform* u = &ub->uniforms[ub->num_uniforms++];
                u->type = u_desc->type;
                u->offset = u_desc->offset;
                u->count = u_desc->count;
                if (u_desc->name) {
                    u->gl_loc = glGetUniformLocation(gl_prog, u_desc->name);
                }
                else {
                    u->gl_loc = u_index;
                }
            }
        }
    }

    /* FIXME: resolve image locations */

    shd->slot.state = SG_RESOURCESTATE_VALID;
}

static void _sg_destroy_shader(_sg_shader* shd) {
    SOKOL_ASSERT(shd);
    if (shd->gl_prog) {
        glDeleteShader(shd->gl_prog);
    }
    _sg_init_shader(shd);
}

static void _sg_create_pipeline(_sg_pipeline* pip, _sg_shader* shd, const sg_pipeline_desc* desc) {
    SOKOL_ASSERT(pip && desc);
    SOKOL_ASSERT(!pip->shader && pip->shader_id == SG_INVALID_ID);
    SOKOL_ASSERT(desc->shader == shd->slot.id);
    SOKOL_ASSERT(shd->gl_prog);
    #ifdef SOKOL_DEBUG
    for (int i = 0; i < SG_MAX_VERTEX_ATTRIBUTES; i++) {
        SOKOL_ASSERT(pip->gl_attrs[i].vb_index == -1);
    }
    #endif

    pip->shader = shd;
    pip->shader_id = desc->shader;
    pip->primitive_type = desc->primitive_type;
    pip->index_type = desc->index_type;
    pip->depth_stencil = desc->depth_stencil;
    pip->blend = desc->blend;
    pip->rast = desc->rast;
    
    /* resolve vertex attributes */
    for (int slot = 0; slot < SG_MAX_SHADERSTAGE_BUFFERS; slot++) {
        const sg_vertex_layout_desc* layout_desc = &desc->input_layouts[slot];
        int layout_byte_size = _sg_vertexlayout_byte_size(layout_desc);
        for (int i = 0; i < layout_desc->num_attrs; i++) {
            const sg_vertex_attr_desc* attr_desc = &layout_desc->attrs[i];
            #ifdef SOKOL_USE_GLES2
            /* on GLES2, attribute vertices must be bound by name */
            SOKOL_ASSERT(attr_desc->name);
            #else
            SOKOL_ASSERT(attr_desc->name || (attr_desc->index >= 0));
            #endif
            GLint attr_loc = attr_desc->index;
            if (attr_desc->name) {
                attr_loc = glGetAttribLocation(pip->shader->gl_prog, attr_desc->name);
            }
            SOKOL_ASSERT(attr_loc < SG_MAX_VERTEX_ATTRIBUTES);
            if (attr_loc != -1) {
                _sg_gl_attr* gl_attr = &pip->gl_attrs[attr_loc];
                gl_attr->vb_index = slot;
                if (layout_desc->step_func == SG_STEPFUNC_PER_VERTEX) {
                    gl_attr->divisor = 0;
                }
                else {
                    gl_attr->divisor = layout_desc->step_rate;
                }
                gl_attr->stride = layout_byte_size;
                gl_attr->offset = _sg_vertexlayout_attr_offset(layout_desc, i);
                sg_vertex_format fmt = attr_desc->format;
                gl_attr->size = _sg_gl_vertexformat_size(fmt);
                gl_attr->type = _sg_gl_vertexformat_type(fmt);
                gl_attr->normalized = _sg_gl_vertexformat_normalized(fmt);
            }
        }
    }
    pip->slot.state = SG_RESOURCESTATE_VALID;
}

static void _sg_destroy_pipeline(_sg_pipeline* pip) {
    SOKOL_ASSERT(pip);
    _sg_init_pipeline(pip);
}

static void _sg_create_pass(_sg_pass* pass, const sg_pass_desc* desc) {
    SOKOL_ASSERT(pass && desc);
    /* FIXME */
    pass->slot.state = SG_RESOURCESTATE_FAILED;
}

/*-- GL backend rendering functions ------------------------------------------*/
static void _sg_begin_pass(_sg_backend* state, _sg_pass* pass, const sg_pass_action* action, int w, int h) {
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
    /* FIXME: multiple-render-target! */
    GLbitfield clear_mask = 0;
    if (action->actions & SG_PASSACTION_CLEAR_COLOR0) {
        clear_mask |= GL_COLOR_BUFFER_BIT;
        const float* c = action->color[0];
        glClearColor(c[0], c[1], c[2], c[3]);
    }
    if (action->actions & SG_PASSACTION_CLEAR_DEPTH_STENCIL) {
        /* FIXME: hmm separate depth/stencil clear? */
        clear_mask |= GL_DEPTH_BUFFER_BIT|GL_STENCIL_BUFFER_BIT;
        #ifdef SOKOL_USE_GLCORE33
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
    /* FIXME: bind default framebuffer */
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
    _SG_GL_CHECK_ERROR();

    state->cur_primitive_type = _sg_gl_primitive_type(pip->primitive_type);
    state->cur_index_type = _sg_gl_index_type(pip->index_type);
    state->cur_pipeline = pip;
    state->cur_pipeline_id = pip->slot.id;

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

    /* update blend state */
    const sg_blend_state* new_b = &pip->blend;
    sg_blend_state* cache_b = &state->cache.blend;
    if (new_b->enabled != cache_b->enabled) {
        cache_b->enabled = new_b->enabled;
        if (new_b->enabled) glEnable(GL_BLEND);
        else glDisable(GL_BLEND);
    }
    if ((new_b->src_factor_rgb != cache_b->src_factor_rgb) ||
        (new_b->dst_factor_rgb != cache_b->dst_factor_rgb) ||
        (new_b->src_factor_alpha != cache_b->src_factor_alpha) ||
        (new_b->dst_factor_alpha != cache_b->dst_factor_alpha))
    {
        cache_b->src_factor_rgb = new_b->src_factor_rgb;
        cache_b->dst_factor_rgb = new_b->dst_factor_rgb;
        cache_b->src_factor_alpha = new_b->src_factor_alpha;
        cache_b->dst_factor_alpha = new_b->dst_factor_alpha;
        glBlendFuncSeparate(_sg_gl_blend_factor(new_b->src_factor_rgb),
            _sg_gl_blend_factor(new_b->dst_factor_rgb),
            _sg_gl_blend_factor(new_b->src_factor_alpha),
            _sg_gl_blend_factor(new_b->dst_factor_alpha));
    }
    if ((new_b->op_rgb != cache_b->op_rgb) || (new_b->op_alpha != cache_b->op_alpha)) {
        cache_b->op_rgb = new_b->op_rgb;
        cache_b->op_alpha = new_b->op_alpha;
        glBlendEquationSeparate(_sg_gl_blend_op(new_b->op_rgb), _sg_gl_blend_op(new_b->op_alpha));
    }
    if (new_b->color_write_mask != cache_b->color_write_mask) {
        cache_b->color_write_mask = new_b->color_write_mask;
        glColorMask((new_b->color_write_mask & SG_COLORMASK_R) != 0,
                    (new_b->color_write_mask & SG_COLORMASK_G) != 0,
                    (new_b->color_write_mask & SG_COLORMASK_B) != 0,
                    (new_b->color_write_mask & SG_COLORMASK_A) != 0);
    }
    /* FIXME: fuzzy compare? */
    if ((new_b->blend_color[0] != cache_b->blend_color[0]) ||
        (new_b->blend_color[1] != cache_b->blend_color[1]) ||
        (new_b->blend_color[2] != cache_b->blend_color[2]) ||
        (new_b->blend_color[3] != cache_b->blend_color[3]))
    {
        const float* bc = new_b->blend_color;
        for (int i=0; i<4; i++) {
            cache_b->blend_color[i] = bc[i];
        }
        glBlendColor(bc[0], bc[1], bc[2], bc[3]);
    }

    /* update rasterizer state */
    const sg_rasterizer_state* new_r = &pip->rast;
    sg_rasterizer_state* cache_r = &state->cache.rast;
    if (new_r->cull_face_enabled != cache_r->cull_face_enabled) {
        cache_r->cull_face_enabled = new_r->cull_face_enabled;
        if (new_r->cull_face_enabled) glEnable(GL_CULL_FACE);
        else glDisable(GL_CULL_FACE);
    }
    if (new_r->cull_face != cache_r->cull_face) {
        cache_r->cull_face = new_r->cull_face;
        glCullFace(_sg_gl_cull_face(new_r->cull_face));
    }
    if (new_r->scissor_test_enabled != cache_r->scissor_test_enabled) {
        cache_r->scissor_test_enabled = new_r->scissor_test_enabled;
        if (new_r->scissor_test_enabled) glEnable(GL_SCISSOR_TEST);
        else glDisable(GL_SCISSOR_TEST);
    }
    if (new_r->dither_enabled != cache_r->dither_enabled) {
        cache_r->dither_enabled = new_r->dither_enabled;
        if (new_r->dither_enabled) glEnable(GL_DITHER);
        else glDisable(GL_DITHER);
    }
    #ifdef SOKOL_USE_GLCORE33
    if (new_r->sample_count != cache_r->sample_count) {
        cache_r->sample_count = new_r->sample_count;
        if (new_r->sample_count > 1) glEnable(GL_MULTISAMPLE);
        else glDisable(GL_MULTISAMPLE);
    }
    #endif

    /* bind shader program */
    glUseProgram(pip->shader->gl_prog);

    /* FIXME: bind textures */

    /* index buffer (can be 0) */
    if (ib) {
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ib->gl_buf[ib->active_slot]);
    }
    else {
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    }
    /* vertex attributes */
    GLuint gl_vb = 0;
    for (int attr_index = 0; attr_index < SG_MAX_VERTEX_ATTRIBUTES; attr_index++) {
        _sg_gl_attr* attr = &pip->gl_attrs[attr_index];
        _sg_gl_attr* cache_attr = &state->cache.attrs[attr_index];
        if (attr->vb_index >= 0) {
            /* attribute is enabled */
            SOKOL_ASSERT(attr->vb_index < num_vbs);
            _sg_buffer* vb = vbs[attr->vb_index];
            SOKOL_ASSERT(vb);
            if (gl_vb != vb->gl_buf[vb->active_slot]) {
                gl_vb = vb->gl_buf[vb->active_slot];
                glBindBuffer(GL_ARRAY_BUFFER, gl_vb);
            }
            glVertexAttribPointer(attr_index, attr->size, attr->type, 
                attr->normalized, attr->stride, 
                (const GLvoid*)(GLintptr)attr->offset);
            if (cache_attr->vb_index == -1) {
                glEnableVertexAttribArray(attr_index);
            }
            if (cache_attr->divisor != attr->divisor) {
                #ifdef SOKOL_USE_GLES2
                if (state->features[SG_FEATURE_INSTANCED_ARRAYS]) {
                    glVertexAttribDivisorEXT(attr_index, attr->divisor);
                }
                #else
                glVertexAttribDivisor(attr_index, attr->divisor);
                #endif
            }
        }
        else {
            /* attribute is disabled */
            if (cache_attr->vb_index != -1) {
                glDisableVertexAttribArray(attr_index);
            }
        }
        *cache_attr = *attr;
    }
}

static void _sg_apply_uniform_block(_sg_backend* state, sg_shader_stage stage_index, int ub_index, const void* data, int num_bytes) {
    SOKOL_ASSERT(state);
    SOKOL_ASSERT(data && (num_bytes > 0));
    SOKOL_ASSERT((stage_index >= 0) && ((int)stage_index < SG_NUM_SHADER_STAGES));
    if (!state->next_draw_valid) {
        return;
    }
    if (state->cur_pipeline->slot.id != state->cur_pipeline_id) {
        /* pipeline object was destroyed */
        return;
    }
    if (state->cur_pipeline->shader->slot.id != state->cur_pipeline->shader_id) {
        /* shader object was destroyed */
    }
    _sg_shader_stage* stage = &state->cur_pipeline->shader->stage[stage_index];
    SOKOL_ASSERT(ub_index < stage->num_uniform_blocks);
    _sg_uniform_block* ub = &stage->uniform_blocks[ub_index];
    SOKOL_ASSERT(ub->size == num_bytes);
    for (int u_index = 0; u_index < ub->num_uniforms; u_index++) {
        _sg_uniform* u = &ub->uniforms[u_index];
        SOKOL_ASSERT(u->type != SG_UNIFORMTYPE_INVALID);
        if (u->gl_loc == -1) {
            continue;
        }
        GLfloat* ptr = (GLfloat*) (((uint8_t*)data) + u->offset);
        switch (u->type) {
            case SG_UNIFORMTYPE_INVALID:
                break;
            case SG_UNIFORMTYPE_FLOAT:
                glUniform1fv(u->gl_loc, u->count, ptr);
                break;
            case SG_UNIFORMTYPE_FLOAT2:
                glUniform2fv(u->gl_loc, u->count, ptr);
                break;
            case SG_UNIFORMTYPE_FLOAT3:
                glUniform3fv(u->gl_loc, u->count, ptr);
                break;
            case SG_UNIFORMTYPE_FLOAT4:
                glUniform4fv(u->gl_loc, u->count, ptr);
                break;
            case SG_UNIFORMTYPE_MAT4:
                glUniformMatrix4fv(u->gl_loc, u->count, GL_FALSE, ptr);
                break;
        }
    }

    /* FIXME: apply images */
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
            #ifdef SOKOL_USE_GLES2
            if (state->features[SG_FEATURE_INSTANCED_ARRAYS]) {
                glDrawElementsInstancedEXT(p_type, num_elements, i_type, indices, num_instances);
            }
            #else
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
            #ifdef SOKOL_USE_GLES2
            if (state->features[SG_FEATURE_INSTANCED_ARRAYS]) {
                glDrawArraysInstancedEXT(p_type, base_element, num_elements, num_instances);
            }
            #else
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

static void _sg_update_buffer(_sg_backend* state, _sg_buffer* buf, const void* data_ptr, int data_size) {
    SOKOL_ASSERT(state && buf && data_ptr && data_size > 0);
    /* only one update per buffer per frame allowed */
    SOKOL_ASSERT(buf->upd_frame_index != state->frame_index);
    SOKOL_ASSERT((buf->usage == SG_USAGE_DYNAMIC) || (buf->usage == SG_USAGE_STREAM));
    SOKOL_ASSERT(data_size <= buf->size);
    buf->upd_frame_index = state->frame_index;
    if (++buf->active_slot >= buf->num_slots) {
        buf->active_slot = 0;
    }
    GLenum gl_tgt = _sg_gl_buffer_target(buf->type);
    SOKOL_ASSERT(buf->active_slot < _SG_GL_NUM_UPDATE_SLOTS);
    GLuint gl_buf = buf->gl_buf[buf->active_slot];
    SOKOL_ASSERT(gl_buf);
    _SG_GL_CHECK_ERROR();
    glBindBuffer(gl_tgt, gl_buf);
    glBufferSubData(gl_tgt, 0, data_size, data_ptr);
    _SG_GL_CHECK_ERROR();
}
