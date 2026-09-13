/*
    LLM maintained.

    gl_mock.c -- runtime side of the GL 4.x / GLES 3.x mock library.

    Implements every GL function which sokol_gfx.h calls, plus the
    inspection and fault-injection API declared in gl_mock.h.

    The mock is 'good enough' for unit tests, it is not a GL validator:

    - object names are handed out from a per-kind table, the same numeric
      name can exist in two different object namespaces (as in real GL)
    - deleting an object also unbinds it, as in real GL
    - queries answer from tracked state (current program, framebuffer
      binding, version, extensions) or from a table of default limits
      which tests can override with gl_mock_set_int()
    - calls are logged with their scalar arguments; array arguments are
      logged partially, see the comments at the respective functions

    Not thread-safe -- this is intended for the sokol-gfx unit-test loop,
    which runs on a single thread.
*/
#include "gl_mock.h"
#include <string.h>
#include <assert.h>

#define _GLM_MAX_ERRORS (8)
#define _GLM_MAX_INT_OVERRIDES (32)
#define _GLM_MAX_VERTEX_ATTRIBS (16)

#define _GLM_UNUSED(x) (void)(x)

// capabilities which sokol_gfx.h enables or disables
static const GLenum _glm_cap_table[] = {
    GL_DEPTH_TEST,
    GL_STENCIL_TEST,
    GL_BLEND,
    GL_POLYGON_OFFSET_FILL,
    GL_CULL_FACE,
    GL_SCISSOR_TEST,
    GL_SAMPLE_ALPHA_TO_COVERAGE,
    GL_DITHER,
    GL_MULTISAMPLE,
    GL_PROGRAM_POINT_SIZE,
    GL_TEXTURE_CUBE_MAP_SEAMLESS,
    GL_FRAMEBUFFER_SRGB,
};
#define _GLM_NUM_CAPS ((int)(sizeof(_glm_cap_table)/sizeof(_glm_cap_table[0])))

// texture targets, the index order must match _glm_tex_target_index()
static const GLenum _glm_tex_target_table[GL_MOCK_MAX_TEXTURE_TARGETS] = {
    GL_TEXTURE_2D,
    GL_TEXTURE_CUBE_MAP,
    GL_TEXTURE_3D,
    GL_TEXTURE_2D_ARRAY,
    GL_TEXTURE_2D_MULTISAMPLE,
    GL_TEXTURE_2D_MULTISAMPLE_ARRAY,
};

// reported extensions, a superset of what both caps paths look for
static const char* _glm_ext_table[] = {
    "GL_EXT_texture_compression_s3tc",
    "GL_EXT_texture_compression_rgtc",
    "GL_EXT_texture_compression_bptc",
    "GL_ARB_ES3_compatibility",
    "GL_WEBGL_compressed_texture_etc",
    "GL_KHR_texture_compression_astc_ldr",
    "GL_WEBGL_compressed_texture_astc",
    "GL_EXT_color_buffer_float",
    "GL_EXT_color_buffer_half_float",
    "GL_OES_texture_float_linear",
    "GL_EXT_float_blend",
    "GL_EXT_texture_filter_anisotropic",
};
#define _GLM_NUM_EXTS ((int)(sizeof(_glm_ext_table)/sizeof(_glm_ext_table[0])))

// default answers for the glGetIntegerv limit queries
typedef struct { GLenum pname; GLint value; } _glm_int_item_t;
static const _glm_int_item_t _glm_default_ints[] = {
    { GL_MAX_TEXTURE_SIZE, 16384 },
    { GL_MAX_CUBE_MAP_TEXTURE_SIZE, 16384 },
    { GL_MAX_3D_TEXTURE_SIZE, 2048 },
    { GL_MAX_ARRAY_TEXTURE_LAYERS, 2048 },
    { GL_MAX_VERTEX_ATTRIBS, _GLM_MAX_VERTEX_ATTRIBS },
    { GL_MAX_DRAW_BUFFERS, GL_MOCK_MAX_COLOR_ATTACHMENTS },
    { GL_MAX_TEXTURE_IMAGE_UNITS, 16 },
    { GL_MAX_SHADER_STORAGE_BUFFER_BINDINGS, GL_MOCK_MAX_BUFFER_BINDINGS },
    { GL_MAX_IMAGE_UNITS, 8 },
    { GL_MAX_VERTEX_UNIFORM_COMPONENTS, 4096 },
    { GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, 16 },
    { GL_MAX_COMBINED_TEXTURE_IMAGE_UNITS, GL_MOCK_MAX_TEXTURE_UNITS },
};
#define _GLM_NUM_DEFAULT_INTS ((int)(sizeof(_glm_default_ints)/sizeof(_glm_default_ints[0])))

// per-program bookkeeping which is not part of the public info struct
typedef struct {
    GLint next_attrib_loc;
    GLint next_uniform_loc;
} _glm_prog_priv_t;

typedef struct {
    bool valid;

    // object tracking
    bool alive[GL_MOCK_OBJ_NUM][GL_MOCK_MAX_OBJECTS];
    int live_count[GL_MOCK_OBJ_NUM];
    gl_mock_buffer_info_t buffer[GL_MOCK_MAX_OBJECTS];
    gl_mock_texture_info_t texture[GL_MOCK_MAX_OBJECTS];
    gl_mock_sampler_info_t sampler[GL_MOCK_MAX_OBJECTS];
    gl_mock_framebuffer_info_t framebuffer[GL_MOCK_MAX_OBJECTS];
    gl_mock_framebuffer_info_t default_framebuffer;
    gl_mock_renderbuffer_info_t renderbuffer[GL_MOCK_MAX_OBJECTS];
    gl_mock_shader_info_t shader[GL_MOCK_MAX_OBJECTS];
    gl_mock_program_info_t program[GL_MOCK_MAX_OBJECTS];
    _glm_prog_priv_t prog_priv[GL_MOCK_MAX_OBJECTS];

    // bound state
    gl_mock_bindings_t bindings;
    GLuint tex_bindings[GL_MOCK_MAX_TEXTURE_UNITS][GL_MOCK_MAX_TEXTURE_TARGETS];
    GLuint smp_bindings[GL_MOCK_MAX_TEXTURE_UNITS];
    gl_mock_image_binding_t img_bindings[GL_MOCK_MAX_IMAGE_UNITS];
    gl_mock_buffer_binding_t sbuf_bindings[GL_MOCK_MAX_BUFFER_BINDINGS];
    bool cap_enabled[_GLM_NUM_CAPS];
    bool attrib_enabled[_GLM_MAX_VERTEX_ATTRIBS];
    gl_mock_render_state_t render_state;
    gl_mock_dispatch_t last_dispatch;

    // call log
    gl_mock_call_t calls[GL_MOCK_MAX_CALLS];
    int num_calls;
    bool call_log_overflow;

    // fault injection
    GLenum errors[_GLM_MAX_ERRORS];
    int num_errors;
    _glm_int_item_t int_overrides[_GLM_MAX_INT_OVERRIDES];
    int num_int_overrides;
    int fail_compile;
    int fail_link;
    int fail_attrib_loc;
    int fail_uniform_loc;
    GLenum framebuffer_status;
    char info_log[GL_MOCK_MAX_INFO_LOG];
} _glm_state_t;

static _glm_state_t _glm;

//-- helpers -------------------------------------------------------------------
static gl_mock_arg_t _glm_ai(int64_t v) {
    gl_mock_arg_t a = {0};
    a.i = v;
    return a;
}

static gl_mock_arg_t _glm_af(double v) {
    gl_mock_arg_t a = {0};
    a.f = v;
    return a;
}

static gl_mock_arg_t _glm_ap(const void* v) {
    gl_mock_arg_t a = {0};
    a.p = v;
    return a;
}

static void _glm_record(gl_mock_func_t fn, int num_args, const gl_mock_arg_t* args) {
    assert((num_args >= 0) && (num_args <= GL_MOCK_MAX_CALL_ARGS));
    if (_glm.num_calls >= GL_MOCK_MAX_CALLS) {
        _glm.call_log_overflow = true;
        return;
    }
    gl_mock_call_t* c = &_glm.calls[_glm.num_calls++];
    memset(c, 0, sizeof(*c));
    c->func = fn;
    c->num_args = num_args;
    for (int i = 0; i < num_args; i++) {
        c->args[i] = args[i];
    }
}

#define _GLM_REC0(fn) _glm_record(GL_MOCK_FUNC_##fn, 0, 0)
#define _GLM_REC(fn, ...) do { \
    const gl_mock_arg_t _glm_args[] = { __VA_ARGS__ }; \
    _glm_record(GL_MOCK_FUNC_##fn, (int)(sizeof(_glm_args)/sizeof(_glm_args[0])), _glm_args); \
} while (0)

static bool _glm_valid(gl_mock_obj_t kind, GLuint name) {
    return (name > 0) && (name <= (GLuint)GL_MOCK_MAX_OBJECTS) && _glm.alive[kind][name - 1];
}

static int _glm_idx(GLuint name) {
    assert(name > 0);
    return (int)name - 1;
}

static GLuint _glm_alloc(gl_mock_obj_t kind) {
    for (int i = 0; i < GL_MOCK_MAX_OBJECTS; i++) {
        if (!_glm.alive[kind][i]) {
            _glm.alive[kind][i] = true;
            _glm.live_count[kind]++;
            return (GLuint)(i + 1);
        }
    }
    assert(false && "gl_mock: out of object slots");
    return 0;
}

static void _glm_free(gl_mock_obj_t kind, GLuint name) {
    if (_glm_valid(kind, name)) {
        _glm.alive[kind][name - 1] = false;
        _glm.live_count[kind]--;
    }
}

static int _glm_cap_index(GLenum cap) {
    for (int i = 0; i < _GLM_NUM_CAPS; i++) {
        if (_glm_cap_table[i] == cap) {
            return i;
        }
    }
    assert(false && "gl_mock: unknown capability");
    return -1;
}

static GLenum _glm_norm_tex_target(GLenum target) {
    switch (target) {
        case GL_TEXTURE_CUBE_MAP_POSITIVE_X:
        case GL_TEXTURE_CUBE_MAP_NEGATIVE_X:
        case GL_TEXTURE_CUBE_MAP_POSITIVE_Y:
        case GL_TEXTURE_CUBE_MAP_NEGATIVE_Y:
        case GL_TEXTURE_CUBE_MAP_POSITIVE_Z:
        case GL_TEXTURE_CUBE_MAP_NEGATIVE_Z:
            return GL_TEXTURE_CUBE_MAP;
        default:
            return target;
    }
}

static int _glm_tex_target_index(GLenum target) {
    const GLenum t = _glm_norm_tex_target(target);
    for (int i = 0; i < GL_MOCK_MAX_TEXTURE_TARGETS; i++) {
        if (_glm_tex_target_table[i] == t) {
            return i;
        }
    }
    assert(false && "gl_mock: unknown texture target");
    return -1;
}

static int _glm_active_unit(void) {
    const int unit = (int)(_glm.bindings.active_texture - GL_TEXTURE0);
    assert((unit >= 0) && (unit < GL_MOCK_MAX_TEXTURE_UNITS));
    return unit;
}

// the texture currently bound to 'target' at the active texture unit
static gl_mock_texture_info_t* _glm_cur_tex(GLenum target) {
    const GLuint name = _glm.tex_bindings[_glm_active_unit()][_glm_tex_target_index(target)];
    if (_glm_valid(GL_MOCK_OBJ_TEXTURE, name)) {
        return &_glm.texture[_glm_idx(name)];
    }
    return 0;
}

// the buffer currently bound to 'target'
static gl_mock_buffer_info_t* _glm_cur_buf(GLenum target) {
    GLuint name = 0;
    switch (target) {
        case GL_ARRAY_BUFFER: name = _glm.bindings.array_buffer; break;
        case GL_ELEMENT_ARRAY_BUFFER: name = _glm.bindings.element_array_buffer; break;
        case GL_SHADER_STORAGE_BUFFER: name = _glm.bindings.shader_storage_buffer; break;
        default: assert(false && "gl_mock: unknown buffer target"); break;
    }
    if (_glm_valid(GL_MOCK_OBJ_BUFFER, name)) {
        return &_glm.buffer[_glm_idx(name)];
    }
    return 0;
}

static gl_mock_framebuffer_info_t* _glm_fb_info(GLuint name) {
    if (name == 0) {
        return &_glm.default_framebuffer;
    } else if (_glm_valid(GL_MOCK_OBJ_FRAMEBUFFER, name)) {
        return &_glm.framebuffer[_glm_idx(name)];
    }
    return 0;
}

// the framebuffer addressed by a framebuffer-target enum
static gl_mock_framebuffer_info_t* _glm_cur_fb(GLenum target) {
    switch (target) {
        case GL_FRAMEBUFFER:
        case GL_DRAW_FRAMEBUFFER:
            return _glm_fb_info(_glm.bindings.draw_framebuffer);
        case GL_READ_FRAMEBUFFER:
            return _glm_fb_info(_glm.bindings.read_framebuffer);
        default:
            assert(false && "gl_mock: unknown framebuffer target");
            return 0;
    }
}

static gl_mock_renderbuffer_info_t* _glm_cur_rbuf(GLenum target) {
    assert(target == GL_RENDERBUFFER);
    _GLM_UNUSED(target);
    const GLuint name = _glm.bindings.renderbuffer;
    if (_glm_valid(GL_MOCK_OBJ_RENDERBUFFER, name)) {
        return &_glm.renderbuffer[_glm_idx(name)];
    }
    return 0;
}

// add, replace or clear a framebuffer attachment. A call with texture == 0
// and renderbuffer == 0 is the real-GL "detach" idiom and clears the slot.
static void _glm_set_attachment(GLenum target, GLenum attachment, GLenum textarget, GLuint texture, GLuint renderbuffer, GLint level, GLint layer) {
    gl_mock_framebuffer_info_t* fb = _glm_cur_fb(target);
    if (0 == fb) {
        return;
    }
    int slot_index = -1;
    for (int i = 0; i < fb->num_attachments; i++) {
        if (fb->attachments[i].attachment == attachment) {
            slot_index = i;
            break;
        }
    }
    const bool detach = (0 == texture) && (0 == renderbuffer);
    if (detach) {
        if (slot_index >= 0) {
            // compact the tail down so num_attachments stays tight
            for (int i = slot_index; i < fb->num_attachments - 1; i++) {
                fb->attachments[i] = fb->attachments[i + 1];
            }
            fb->num_attachments--;
            fb->attachments[fb->num_attachments] = (gl_mock_fb_attachment_t){0};
        }
        return;
    }
    gl_mock_fb_attachment_t* slot = 0;
    if (slot_index >= 0) {
        slot = &fb->attachments[slot_index];
    } else {
        assert(fb->num_attachments < GL_MOCK_MAX_FB_ATTACHMENTS);
        if (fb->num_attachments >= GL_MOCK_MAX_FB_ATTACHMENTS) {
            return;
        }
        slot = &fb->attachments[fb->num_attachments++];
    }
    slot->attachment = attachment;
    slot->textarget = textarget;
    slot->texture = texture;
    slot->renderbuffer = renderbuffer;
    slot->level = level;
    slot->layer = layer;
}

static void _glm_unbind_texture(GLuint name) {
    for (int unit = 0; unit < GL_MOCK_MAX_TEXTURE_UNITS; unit++) {
        for (int t = 0; t < GL_MOCK_MAX_TEXTURE_TARGETS; t++) {
            if (_glm.tex_bindings[unit][t] == name) {
                _glm.tex_bindings[unit][t] = 0;
            }
        }
    }
    for (int unit = 0; unit < GL_MOCK_MAX_IMAGE_UNITS; unit++) {
        if (_glm.img_bindings[unit].texture == name) {
            memset(&_glm.img_bindings[unit], 0, sizeof(_glm.img_bindings[unit]));
        }
    }
}

static void _glm_unbind_buffer(GLuint name) {
    if (_glm.bindings.array_buffer == name) { _glm.bindings.array_buffer = 0; }
    if (_glm.bindings.element_array_buffer == name) { _glm.bindings.element_array_buffer = 0; }
    if (_glm.bindings.shader_storage_buffer == name) { _glm.bindings.shader_storage_buffer = 0; }
    for (int i = 0; i < GL_MOCK_MAX_BUFFER_BINDINGS; i++) {
        if (_glm.sbuf_bindings[i].buffer == name) {
            memset(&_glm.sbuf_bindings[i], 0, sizeof(_glm.sbuf_bindings[i]));
        }
    }
}

static void _glm_copy_info_log(GLsizei bufSize, GLsizei* length, GLchar* infoLog) {
    if (bufSize <= 0) {
        if (length) { *length = 0; }
        return;
    }
    size_t n = strlen(_glm.info_log);
    if (n > (size_t)bufSize - 1) {
        n = (size_t)bufSize - 1;
    }
    if (infoLog) {
        memcpy(infoLog, _glm.info_log, n);
        infoLog[n] = 0;
    }
    if (length) { *length = (GLsizei)n; }
}

//-- public control API --------------------------------------------------------
void gl_mock_setup(void) {
    memset(&_glm, 0, sizeof(_glm));
    _glm.valid = true;
    _glm.bindings.active_texture = GL_TEXTURE0;
    _glm.framebuffer_status = GL_FRAMEBUFFER_COMPLETE;
    strcpy(_glm.info_log, "gl_mock info log");

    // initial render state as defined by the GL spec
    gl_mock_render_state_t* rs = &_glm.render_state;
    rs->depth_func = GL_LESS;
    rs->depth_mask = GL_TRUE;
    rs->stencil_front.func = GL_ALWAYS;
    rs->stencil_front.mask = 0xFFFFFFFF;
    rs->stencil_front.fail_op = GL_KEEP;
    rs->stencil_front.depth_fail_op = GL_KEEP;
    rs->stencil_front.pass_op = GL_KEEP;
    rs->stencil_back = rs->stencil_front;
    rs->stencil_write_mask = 0xFFFFFFFF;
    rs->blend_src_rgb = GL_ONE;
    rs->blend_dst_rgb = GL_ZERO;
    rs->blend_src_alpha = GL_ONE;
    rs->blend_dst_alpha = GL_ZERO;
    rs->blend_op_rgb = GL_FUNC_ADD;
    rs->blend_op_alpha = GL_FUNC_ADD;
    for (int i = 0; i < GL_MOCK_MAX_COLOR_ATTACHMENTS; i++) {
        for (int c = 0; c < 4; c++) {
            rs->color_mask[i][c] = GL_TRUE;
        }
    }
    rs->cull_face = GL_BACK;
    rs->front_face = GL_CCW;
    rs->unpack_alignment = 4;
    _glm.cap_enabled[_glm_cap_index(GL_DITHER)] = true;
}

void gl_mock_shutdown(void) {
    memset(&_glm, 0, sizeof(_glm));
}

int gl_mock_version(void) {
    return GL_MOCK_VERSION;
}

int gl_mock_live_objects(gl_mock_obj_t kind) {
    assert((kind >= 0) && (kind < GL_MOCK_OBJ_NUM));
    return _glm.live_count[kind];
}

int gl_mock_live_objects_total(void) {
    int n = 0;
    for (int i = 0; i < GL_MOCK_OBJ_NUM; i++) {
        n += _glm.live_count[i];
    }
    return n;
}

bool gl_mock_is_object(gl_mock_obj_t kind, GLuint name) {
    assert((kind >= 0) && (kind < GL_MOCK_OBJ_NUM));
    return _glm_valid(kind, name);
}

#define _GLM_INFO_GETTER(func_name, kind, array, type) \
    bool func_name(GLuint name, type* out) { \
        assert(out); \
        if (!_glm_valid(kind, name)) { \
            memset(out, 0, sizeof(*out)); \
            return false; \
        } \
        *out = _glm.array[_glm_idx(name)]; \
        return true; \
    }

_GLM_INFO_GETTER(gl_mock_buffer_info, GL_MOCK_OBJ_BUFFER, buffer, gl_mock_buffer_info_t)
_GLM_INFO_GETTER(gl_mock_texture_info, GL_MOCK_OBJ_TEXTURE, texture, gl_mock_texture_info_t)
_GLM_INFO_GETTER(gl_mock_sampler_info, GL_MOCK_OBJ_SAMPLER, sampler, gl_mock_sampler_info_t)
_GLM_INFO_GETTER(gl_mock_renderbuffer_info, GL_MOCK_OBJ_RENDERBUFFER, renderbuffer, gl_mock_renderbuffer_info_t)
_GLM_INFO_GETTER(gl_mock_shader_info, GL_MOCK_OBJ_SHADER, shader, gl_mock_shader_info_t)
_GLM_INFO_GETTER(gl_mock_program_info, GL_MOCK_OBJ_PROGRAM, program, gl_mock_program_info_t)

// framebuffer name 0 is valid here and returns the default framebuffer state
bool gl_mock_framebuffer_info(GLuint name, gl_mock_framebuffer_info_t* out) {
    assert(out);
    const gl_mock_framebuffer_info_t* fb = _glm_fb_info(name);
    if (0 == fb) {
        memset(out, 0, sizeof(*out));
        return false;
    }
    *out = *fb;
    return true;
}

int gl_mock_num_calls(void) {
    return _glm.num_calls;
}

const gl_mock_call_t* gl_mock_call(int index) {
    if ((index < 0) || (index >= _glm.num_calls)) {
        return 0;
    }
    return &_glm.calls[index];
}

int gl_mock_count_calls(gl_mock_func_t func) {
    int n = 0;
    for (int i = 0; i < _glm.num_calls; i++) {
        if (_glm.calls[i].func == func) {
            n++;
        }
    }
    return n;
}

int gl_mock_find_call(gl_mock_func_t func, int start_index) {
    for (int i = (start_index < 0) ? 0 : start_index; i < _glm.num_calls; i++) {
        if (_glm.calls[i].func == func) {
            return i;
        }
    }
    return -1;
}

const gl_mock_call_t* gl_mock_last_call(gl_mock_func_t func) {
    for (int i = _glm.num_calls - 1; i >= 0; i--) {
        if (_glm.calls[i].func == func) {
            return &_glm.calls[i];
        }
    }
    return 0;
}

void gl_mock_clear_calls(void) {
    _glm.num_calls = 0;
    _glm.call_log_overflow = false;
}

bool gl_mock_call_log_overflow(void) {
    return _glm.call_log_overflow;
}

const char* gl_mock_func_name(gl_mock_func_t func) {
    static const char* names[GL_MOCK_FUNC_NUM] = {
        "GL_MOCK_FUNC_INVALID",
        #define _GLM_XMACRO(name, ret, args) #name,
        _GLM_GL_FUNCS
        #undef _GLM_XMACRO
    };
    if ((func < 0) || (func >= GL_MOCK_FUNC_NUM)) {
        return "???";
    }
    return names[func];
}

const gl_mock_bindings_t* gl_mock_bindings(void) {
    return &_glm.bindings;
}

GLuint gl_mock_bound_texture(int unit, GLenum target) {
    assert((unit >= 0) && (unit < GL_MOCK_MAX_TEXTURE_UNITS));
    return _glm.tex_bindings[unit][_glm_tex_target_index(target)];
}

GLuint gl_mock_bound_sampler(int unit) {
    assert((unit >= 0) && (unit < GL_MOCK_MAX_TEXTURE_UNITS));
    return _glm.smp_bindings[unit];
}

const gl_mock_image_binding_t* gl_mock_image_binding(int unit) {
    assert((unit >= 0) && (unit < GL_MOCK_MAX_IMAGE_UNITS));
    return &_glm.img_bindings[unit];
}

const gl_mock_buffer_binding_t* gl_mock_buffer_binding(GLenum target, int index) {
    assert(target == GL_SHADER_STORAGE_BUFFER);
    _GLM_UNUSED(target);
    assert((index >= 0) && (index < GL_MOCK_MAX_BUFFER_BINDINGS));
    return &_glm.sbuf_bindings[index];
}

bool gl_mock_is_enabled(GLenum cap) {
    const int i = _glm_cap_index(cap);
    return (i >= 0) ? _glm.cap_enabled[i] : false;
}

const gl_mock_render_state_t* gl_mock_render_state(void) {
    return &_glm.render_state;
}

bool gl_mock_vertex_attrib_enabled(int index) {
    assert((index >= 0) && (index < _GLM_MAX_VERTEX_ATTRIBS));
    return _glm.attrib_enabled[index];
}

const gl_mock_dispatch_t* gl_mock_last_dispatch(void) {
    return &_glm.last_dispatch;
}

void gl_mock_fail_next_compile(int n) {
    _glm.fail_compile = n;
}

void gl_mock_fail_next_link(int n) {
    _glm.fail_link = n;
}

void gl_mock_fail_next_attrib_location(int n) {
    _glm.fail_attrib_loc = n;
}

void gl_mock_fail_next_uniform_location(int n) {
    _glm.fail_uniform_loc = n;
}

void gl_mock_set_info_log(const char* msg) {
    assert(msg);
    memset(_glm.info_log, 0, sizeof(_glm.info_log));
    strncpy(_glm.info_log, msg, sizeof(_glm.info_log) - 1);
}

void gl_mock_set_framebuffer_status(GLenum status) {
    _glm.framebuffer_status = status;
}

void gl_mock_push_error(GLenum err) {
    assert(_glm.num_errors < _GLM_MAX_ERRORS);
    if (_glm.num_errors < _GLM_MAX_ERRORS) {
        _glm.errors[_glm.num_errors++] = err;
    }
}

void gl_mock_set_int(GLenum pname, GLint value) {
    for (int i = 0; i < _glm.num_int_overrides; i++) {
        if (_glm.int_overrides[i].pname == pname) {
            _glm.int_overrides[i].value = value;
            return;
        }
    }
    assert(_glm.num_int_overrides < _GLM_MAX_INT_OVERRIDES);
    if (_glm.num_int_overrides < _GLM_MAX_INT_OVERRIDES) {
        _glm.int_overrides[_glm.num_int_overrides].pname = pname;
        _glm.int_overrides[_glm.num_int_overrides].value = value;
        _glm.num_int_overrides++;
    }
}

//-- queries -------------------------------------------------------------------
GLenum glGetError(void) {
    // not logged, sokol_gfx.h calls this after nearly every other call
    if (_glm.num_errors > 0) {
        const GLenum err = _glm.errors[0];
        for (int i = 1; i < _glm.num_errors; i++) {
            _glm.errors[i - 1] = _glm.errors[i];
        }
        _glm.num_errors--;
        return err;
    }
    return GL_NO_ERROR;
}

void glGetIntegerv(GLenum pname, GLint* data) {
    assert(data);
    // not logged, this is a pure query
    switch (pname) {
        case GL_MAJOR_VERSION: *data = GL_MOCK_VERSION / 100; return;
        case GL_MINOR_VERSION: *data = (GL_MOCK_VERSION / 10) % 10; return;
        case GL_NUM_EXTENSIONS: *data = _GLM_NUM_EXTS; return;
        case GL_CURRENT_PROGRAM: *data = (GLint)_glm.bindings.program; return;
        case GL_FRAMEBUFFER_BINDING: *data = (GLint)_glm.bindings.draw_framebuffer; return;
        default: break;
    }
    for (int i = 0; i < _glm.num_int_overrides; i++) {
        if (_glm.int_overrides[i].pname == pname) {
            *data = _glm.int_overrides[i].value;
            return;
        }
    }
    for (int i = 0; i < _GLM_NUM_DEFAULT_INTS; i++) {
        if (_glm_default_ints[i].pname == pname) {
            *data = _glm_default_ints[i].value;
            return;
        }
    }
    assert(false && "gl_mock: unhandled glGetIntegerv pname");
    *data = 0;
}

const GLubyte* glGetStringi(GLenum name, GLuint index) {
    // not logged, this is a pure query
    assert(name == GL_EXTENSIONS);
    _GLM_UNUSED(name);
    if (index < (GLuint)_GLM_NUM_EXTS) {
        return (const GLubyte*) _glm_ext_table[index];
    }
    return 0;
}

//-- buffers -------------------------------------------------------------------
void glGenBuffers(GLsizei n, GLuint* buffers) {
    _GLM_REC(glGenBuffers, _glm_ai(n), _glm_ap(buffers));
    assert(buffers);
    for (GLsizei i = 0; i < n; i++) {
        buffers[i] = _glm_alloc(GL_MOCK_OBJ_BUFFER);
        memset(&_glm.buffer[_glm_idx(buffers[i])], 0, sizeof(gl_mock_buffer_info_t));
    }
}

void glDeleteBuffers(GLsizei n, const GLuint* buffers) {
    _GLM_REC(glDeleteBuffers, _glm_ai(n), _glm_ap(buffers));
    assert(buffers);
    for (GLsizei i = 0; i < n; i++) {
        _glm_unbind_buffer(buffers[i]);
        _glm_free(GL_MOCK_OBJ_BUFFER, buffers[i]);
    }
}

void glBindBuffer(GLenum target, GLuint buffer) {
    _GLM_REC(glBindBuffer, _glm_ai(target), _glm_ai(buffer));
    switch (target) {
        case GL_ARRAY_BUFFER: _glm.bindings.array_buffer = buffer; break;
        case GL_ELEMENT_ARRAY_BUFFER: _glm.bindings.element_array_buffer = buffer; break;
        case GL_SHADER_STORAGE_BUFFER: _glm.bindings.shader_storage_buffer = buffer; break;
        default: assert(false && "gl_mock: unknown buffer target"); break;
    }
    if (_glm_valid(GL_MOCK_OBJ_BUFFER, buffer)) {
        _glm.buffer[_glm_idx(buffer)].target = target;
    }
}

void glBindBufferBase(GLenum target, GLuint index, GLuint buffer) {
    _GLM_REC(glBindBufferBase, _glm_ai(target), _glm_ai(index), _glm_ai(buffer));
    assert(target == GL_SHADER_STORAGE_BUFFER);
    _GLM_UNUSED(target);
    assert(index < (GLuint)GL_MOCK_MAX_BUFFER_BINDINGS);
    gl_mock_buffer_binding_t* b = &_glm.sbuf_bindings[index];
    b->buffer = buffer;
    b->offset = 0;
    b->size = 0;
}

void glBindBufferRange(GLenum target, GLuint index, GLuint buffer, GLintptr offset, GLsizeiptr size) {
    _GLM_REC(glBindBufferRange, _glm_ai(target), _glm_ai(index), _glm_ai(buffer), _glm_ai(offset), _glm_ai(size));
    assert(target == GL_SHADER_STORAGE_BUFFER);
    _GLM_UNUSED(target);
    assert(index < (GLuint)GL_MOCK_MAX_BUFFER_BINDINGS);
    gl_mock_buffer_binding_t* b = &_glm.sbuf_bindings[index];
    b->buffer = buffer;
    b->offset = offset;
    b->size = size;
}

void glBufferData(GLenum target, GLsizeiptr size, const void* data, GLenum usage) {
    _GLM_REC(glBufferData, _glm_ai(target), _glm_ai(size), _glm_ap(data), _glm_ai(usage));
    gl_mock_buffer_info_t* buf = _glm_cur_buf(target);
    if (buf) {
        buf->size = size;
        buf->usage = usage;
        buf->num_data++;
    }
}

void glBufferSubData(GLenum target, GLintptr offset, GLsizeiptr size, const void* data) {
    _GLM_REC(glBufferSubData, _glm_ai(target), _glm_ai(offset), _glm_ai(size), _glm_ap(data));
    gl_mock_buffer_info_t* buf = _glm_cur_buf(target);
    if (buf) {
        buf->num_subdata++;
    }
}

//-- vertex arrays -------------------------------------------------------------
void glGenVertexArrays(GLsizei n, GLuint* arrays) {
    _GLM_REC(glGenVertexArrays, _glm_ai(n), _glm_ap(arrays));
    assert(arrays);
    for (GLsizei i = 0; i < n; i++) {
        arrays[i] = _glm_alloc(GL_MOCK_OBJ_VERTEXARRAY);
    }
}

void glDeleteVertexArrays(GLsizei n, const GLuint* arrays) {
    _GLM_REC(glDeleteVertexArrays, _glm_ai(n), _glm_ap(arrays));
    assert(arrays);
    for (GLsizei i = 0; i < n; i++) {
        if (_glm.bindings.vertex_array == arrays[i]) {
            _glm.bindings.vertex_array = 0;
        }
        _glm_free(GL_MOCK_OBJ_VERTEXARRAY, arrays[i]);
    }
}

void glBindVertexArray(GLuint array) {
    _GLM_REC(glBindVertexArray, _glm_ai(array));
    _glm.bindings.vertex_array = array;
}

void glEnableVertexAttribArray(GLuint index) {
    _GLM_REC(glEnableVertexAttribArray, _glm_ai(index));
    assert(index < (GLuint)_GLM_MAX_VERTEX_ATTRIBS);
    _glm.attrib_enabled[index] = true;
}

void glDisableVertexAttribArray(GLuint index) {
    _GLM_REC(glDisableVertexAttribArray, _glm_ai(index));
    assert(index < (GLuint)_GLM_MAX_VERTEX_ATTRIBS);
    _glm.attrib_enabled[index] = false;
}

void glVertexAttribPointer(GLuint index, GLint size, GLenum type, GLboolean normalized, GLsizei stride, const void* pointer) {
    _GLM_REC(glVertexAttribPointer, _glm_ai(index), _glm_ai(size), _glm_ai(type), _glm_ai(normalized), _glm_ai(stride), _glm_ap(pointer));
}

void glVertexAttribIPointer(GLuint index, GLint size, GLenum type, GLsizei stride, const void* pointer) {
    _GLM_REC(glVertexAttribIPointer, _glm_ai(index), _glm_ai(size), _glm_ai(type), _glm_ai(stride), _glm_ap(pointer));
}

void glVertexAttribDivisor(GLuint index, GLuint divisor) {
    _GLM_REC(glVertexAttribDivisor, _glm_ai(index), _glm_ai(divisor));
}

//-- textures ------------------------------------------------------------------
void glGenTextures(GLsizei n, GLuint* textures) {
    _GLM_REC(glGenTextures, _glm_ai(n), _glm_ap(textures));
    assert(textures);
    for (GLsizei i = 0; i < n; i++) {
        textures[i] = _glm_alloc(GL_MOCK_OBJ_TEXTURE);
        memset(&_glm.texture[_glm_idx(textures[i])], 0, sizeof(gl_mock_texture_info_t));
    }
}

void glDeleteTextures(GLsizei n, const GLuint* textures) {
    _GLM_REC(glDeleteTextures, _glm_ai(n), _glm_ap(textures));
    assert(textures);
    for (GLsizei i = 0; i < n; i++) {
        _glm_unbind_texture(textures[i]);
        _glm_free(GL_MOCK_OBJ_TEXTURE, textures[i]);
    }
}

void glActiveTexture(GLenum texture) {
    _GLM_REC(glActiveTexture, _glm_ai(texture));
    assert(texture >= GL_TEXTURE0);
    assert((texture - GL_TEXTURE0) < (GLenum)GL_MOCK_MAX_TEXTURE_UNITS);
    _glm.bindings.active_texture = texture;
}

void glBindTexture(GLenum target, GLuint texture) {
    _GLM_REC(glBindTexture, _glm_ai(target), _glm_ai(texture));
    _glm.tex_bindings[_glm_active_unit()][_glm_tex_target_index(target)] = texture;
    if (_glm_valid(GL_MOCK_OBJ_TEXTURE, texture)) {
        _glm.texture[_glm_idx(texture)].target = _glm_norm_tex_target(target);
    }
}

void glTexImage2D(GLenum target, GLint level, GLint internalformat, GLsizei width, GLsizei height, GLint border, GLenum format, GLenum type, const void* pixels) {
    _GLM_REC(glTexImage2D, _glm_ai(target), _glm_ai(level), _glm_ai(internalformat), _glm_ai(width), _glm_ai(height), _glm_ai(border), _glm_ai(format), _glm_ai(type), _glm_ap(pixels));
    gl_mock_texture_info_t* tex = _glm_cur_tex(target);
    if (tex) {
        if (level == 0) {
            tex->internal_format = (GLenum)internalformat;
            tex->width = width;
            tex->height = height;
            tex->depth = 1;
        }
        tex->num_image++;
    }
}

void glTexImage3D(GLenum target, GLint level, GLint internalformat, GLsizei width, GLsizei height, GLsizei depth, GLint border, GLenum format, GLenum type, const void* pixels) {
    _GLM_REC(glTexImage3D, _glm_ai(target), _glm_ai(level), _glm_ai(internalformat), _glm_ai(width), _glm_ai(height), _glm_ai(depth), _glm_ai(border), _glm_ai(format), _glm_ai(type), _glm_ap(pixels));
    gl_mock_texture_info_t* tex = _glm_cur_tex(target);
    if (tex) {
        if (level == 0) {
            tex->internal_format = (GLenum)internalformat;
            tex->width = width;
            tex->height = height;
            tex->depth = depth;
        }
        tex->num_image++;
    }
}

void glTexSubImage2D(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLenum type, const void* pixels) {
    _GLM_REC(glTexSubImage2D, _glm_ai(target), _glm_ai(level), _glm_ai(xoffset), _glm_ai(yoffset), _glm_ai(width), _glm_ai(height), _glm_ai(format), _glm_ai(type), _glm_ap(pixels));
    gl_mock_texture_info_t* tex = _glm_cur_tex(target);
    if (tex) {
        tex->num_subimage++;
    }
}

void glTexSubImage3D(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLsizei width, GLsizei height, GLsizei depth, GLenum format, GLenum type, const void* pixels) {
    _GLM_REC(glTexSubImage3D, _glm_ai(target), _glm_ai(level), _glm_ai(xoffset), _glm_ai(yoffset), _glm_ai(zoffset), _glm_ai(width), _glm_ai(height), _glm_ai(depth), _glm_ai(format), _glm_ai(type), _glm_ap(pixels));
    gl_mock_texture_info_t* tex = _glm_cur_tex(target);
    if (tex) {
        tex->num_subimage++;
    }
}

void glCompressedTexImage2D(GLenum target, GLint level, GLenum internalformat, GLsizei width, GLsizei height, GLint border, GLsizei imageSize, const void* data) {
    _GLM_REC(glCompressedTexImage2D, _glm_ai(target), _glm_ai(level), _glm_ai(internalformat), _glm_ai(width), _glm_ai(height), _glm_ai(border), _glm_ai(imageSize), _glm_ap(data));
    gl_mock_texture_info_t* tex = _glm_cur_tex(target);
    if (tex) {
        if (level == 0) {
            tex->internal_format = internalformat;
            tex->width = width;
            tex->height = height;
            tex->depth = 1;
        }
        tex->num_image++;
    }
}

void glCompressedTexImage3D(GLenum target, GLint level, GLenum internalformat, GLsizei width, GLsizei height, GLsizei depth, GLint border, GLsizei imageSize, const void* data) {
    _GLM_REC(glCompressedTexImage3D, _glm_ai(target), _glm_ai(level), _glm_ai(internalformat), _glm_ai(width), _glm_ai(height), _glm_ai(depth), _glm_ai(border), _glm_ai(imageSize), _glm_ap(data));
    gl_mock_texture_info_t* tex = _glm_cur_tex(target);
    if (tex) {
        if (level == 0) {
            tex->internal_format = internalformat;
            tex->width = width;
            tex->height = height;
            tex->depth = depth;
        }
        tex->num_image++;
    }
}

void glCompressedTexSubImage2D(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLsizei imageSize, const void* data) {
    _GLM_REC(glCompressedTexSubImage2D, _glm_ai(target), _glm_ai(level), _glm_ai(xoffset), _glm_ai(yoffset), _glm_ai(width), _glm_ai(height), _glm_ai(format), _glm_ai(imageSize), _glm_ap(data));
    gl_mock_texture_info_t* tex = _glm_cur_tex(target);
    if (tex) {
        tex->num_subimage++;
    }
}

void glCompressedTexSubImage3D(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLsizei width, GLsizei height, GLsizei depth, GLenum format, GLsizei imageSize, const void* data) {
    _GLM_REC(glCompressedTexSubImage3D, _glm_ai(target), _glm_ai(level), _glm_ai(xoffset), _glm_ai(yoffset), _glm_ai(zoffset), _glm_ai(width), _glm_ai(height), _glm_ai(depth), _glm_ai(format), _glm_ai(imageSize), _glm_ap(data));
    gl_mock_texture_info_t* tex = _glm_cur_tex(target);
    if (tex) {
        tex->num_subimage++;
    }
}

void glTexStorage2D(GLenum target, GLsizei levels, GLenum internalformat, GLsizei width, GLsizei height) {
    _GLM_REC(glTexStorage2D, _glm_ai(target), _glm_ai(levels), _glm_ai(internalformat), _glm_ai(width), _glm_ai(height));
    gl_mock_texture_info_t* tex = _glm_cur_tex(target);
    if (tex) {
        tex->storage = true;
        tex->levels = levels;
        tex->internal_format = internalformat;
        tex->width = width;
        tex->height = height;
        tex->depth = 1;
    }
}

void glTexStorage3D(GLenum target, GLsizei levels, GLenum internalformat, GLsizei width, GLsizei height, GLsizei depth) {
    _GLM_REC(glTexStorage3D, _glm_ai(target), _glm_ai(levels), _glm_ai(internalformat), _glm_ai(width), _glm_ai(height), _glm_ai(depth));
    gl_mock_texture_info_t* tex = _glm_cur_tex(target);
    if (tex) {
        tex->storage = true;
        tex->levels = levels;
        tex->internal_format = internalformat;
        tex->width = width;
        tex->height = height;
        tex->depth = depth;
    }
}

void glTexStorage2DMultisample(GLenum target, GLsizei samples, GLenum internalformat, GLsizei width, GLsizei height, GLboolean fixedsamplelocations) {
    _GLM_REC(glTexStorage2DMultisample, _glm_ai(target), _glm_ai(samples), _glm_ai(internalformat), _glm_ai(width), _glm_ai(height), _glm_ai(fixedsamplelocations));
    gl_mock_texture_info_t* tex = _glm_cur_tex(target);
    if (tex) {
        tex->storage = true;
        tex->levels = 1;
        tex->samples = samples;
        tex->internal_format = internalformat;
        tex->width = width;
        tex->height = height;
        tex->depth = 1;
    }
}

void glTexStorage3DMultisample(GLenum target, GLsizei samples, GLenum internalformat, GLsizei width, GLsizei height, GLsizei depth, GLboolean fixedsamplelocations) {
    _GLM_REC(glTexStorage3DMultisample, _glm_ai(target), _glm_ai(samples), _glm_ai(internalformat), _glm_ai(width), _glm_ai(height), _glm_ai(depth), _glm_ai(fixedsamplelocations));
    gl_mock_texture_info_t* tex = _glm_cur_tex(target);
    if (tex) {
        tex->storage = true;
        tex->levels = 1;
        tex->samples = samples;
        tex->internal_format = internalformat;
        tex->width = width;
        tex->height = height;
        tex->depth = depth;
    }
}

void glTexImage2DMultisample(GLenum target, GLsizei samples, GLenum internalformat, GLsizei width, GLsizei height, GLboolean fixedsamplelocations) {
    _GLM_REC(glTexImage2DMultisample, _glm_ai(target), _glm_ai(samples), _glm_ai(internalformat), _glm_ai(width), _glm_ai(height), _glm_ai(fixedsamplelocations));
    gl_mock_texture_info_t* tex = _glm_cur_tex(target);
    if (tex) {
        tex->samples = samples;
        tex->internal_format = internalformat;
        tex->width = width;
        tex->height = height;
        tex->depth = 1;
        tex->num_image++;
    }
}

void glTexImage3DMultisample(GLenum target, GLsizei samples, GLenum internalformat, GLsizei width, GLsizei height, GLsizei depth, GLboolean fixedsamplelocations) {
    _GLM_REC(glTexImage3DMultisample, _glm_ai(target), _glm_ai(samples), _glm_ai(internalformat), _glm_ai(width), _glm_ai(height), _glm_ai(depth), _glm_ai(fixedsamplelocations));
    gl_mock_texture_info_t* tex = _glm_cur_tex(target);
    if (tex) {
        tex->samples = samples;
        tex->internal_format = internalformat;
        tex->width = width;
        tex->height = height;
        tex->depth = depth;
        tex->num_image++;
    }
}

void glTexParameteri(GLenum target, GLenum pname, GLint param) {
    _GLM_REC(glTexParameteri, _glm_ai(target), _glm_ai(pname), _glm_ai(param));
}

void glTexParameterf(GLenum target, GLenum pname, GLfloat param) {
    _GLM_REC(glTexParameterf, _glm_ai(target), _glm_ai(pname), _glm_af(param));
}

// only the first four values are logged
void glTexParameterfv(GLenum target, GLenum pname, const GLfloat* params) {
    assert(params);
    _GLM_REC(glTexParameterfv, _glm_ai(target), _glm_ai(pname), _glm_af(params[0]), _glm_af(params[1]), _glm_af(params[2]), _glm_af(params[3]));
}

void glTextureView(GLuint texture, GLenum target, GLuint origtexture, GLenum internalformat, GLuint minlevel, GLuint numlevels, GLuint minlayer, GLuint numlayers) {
    _GLM_REC(glTextureView, _glm_ai(texture), _glm_ai(target), _glm_ai(origtexture), _glm_ai(internalformat), _glm_ai(minlevel), _glm_ai(numlevels), _glm_ai(minlayer), _glm_ai(numlayers));
    if (_glm_valid(GL_MOCK_OBJ_TEXTURE, texture)) {
        gl_mock_texture_info_t* tex = &_glm.texture[_glm_idx(texture)];
        tex->target = _glm_norm_tex_target(target);
        tex->internal_format = internalformat;
        tex->levels = (GLsizei)numlevels;
        tex->is_view = true;
        tex->view_orig_texture = origtexture;
        if (_glm_valid(GL_MOCK_OBJ_TEXTURE, origtexture)) {
            const gl_mock_texture_info_t* orig = &_glm.texture[_glm_idx(origtexture)];
            tex->width = orig->width;
            tex->height = orig->height;
            tex->depth = (GLsizei)numlayers;
            tex->samples = orig->samples;
        }
    }
}

void glBindImageTexture(GLuint unit, GLuint texture, GLint level, GLboolean layered, GLint layer, GLenum access, GLenum format) {
    _GLM_REC(glBindImageTexture, _glm_ai(unit), _glm_ai(texture), _glm_ai(level), _glm_ai(layered), _glm_ai(layer), _glm_ai(access), _glm_ai(format));
    assert(unit < (GLuint)GL_MOCK_MAX_IMAGE_UNITS);
    gl_mock_image_binding_t* b = &_glm.img_bindings[unit];
    b->texture = texture;
    b->level = level;
    b->layered = layered;
    b->layer = layer;
    b->access = access;
    b->format = format;
}

//-- samplers ------------------------------------------------------------------
void glGenSamplers(GLsizei n, GLuint* samplers) {
    _GLM_REC(glGenSamplers, _glm_ai(n), _glm_ap(samplers));
    assert(samplers);
    for (GLsizei i = 0; i < n; i++) {
        samplers[i] = _glm_alloc(GL_MOCK_OBJ_SAMPLER);
        memset(&_glm.sampler[_glm_idx(samplers[i])], 0, sizeof(gl_mock_sampler_info_t));
    }
}

void glDeleteSamplers(GLsizei n, const GLuint* samplers) {
    _GLM_REC(glDeleteSamplers, _glm_ai(n), _glm_ap(samplers));
    assert(samplers);
    for (GLsizei i = 0; i < n; i++) {
        for (int unit = 0; unit < GL_MOCK_MAX_TEXTURE_UNITS; unit++) {
            if (_glm.smp_bindings[unit] == samplers[i]) {
                _glm.smp_bindings[unit] = 0;
            }
        }
        _glm_free(GL_MOCK_OBJ_SAMPLER, samplers[i]);
    }
}

void glBindSampler(GLuint unit, GLuint sampler) {
    _GLM_REC(glBindSampler, _glm_ai(unit), _glm_ai(sampler));
    assert(unit < (GLuint)GL_MOCK_MAX_TEXTURE_UNITS);
    _glm.smp_bindings[unit] = sampler;
}

void glSamplerParameteri(GLuint sampler, GLenum pname, GLint param) {
    _GLM_REC(glSamplerParameteri, _glm_ai(sampler), _glm_ai(pname), _glm_ai(param));
    if (!_glm_valid(GL_MOCK_OBJ_SAMPLER, sampler)) {
        return;
    }
    gl_mock_sampler_info_t* smp = &_glm.sampler[_glm_idx(sampler)];
    switch (pname) {
        case GL_TEXTURE_MIN_FILTER: smp->min_filter = param; break;
        case GL_TEXTURE_MAG_FILTER: smp->mag_filter = param; break;
        case GL_TEXTURE_WRAP_S: smp->wrap_s = param; break;
        case GL_TEXTURE_WRAP_T: smp->wrap_t = param; break;
        case GL_TEXTURE_WRAP_R: smp->wrap_r = param; break;
        case GL_TEXTURE_COMPARE_MODE: smp->compare_mode = param; break;
        case GL_TEXTURE_COMPARE_FUNC: smp->compare_func = param; break;
        case GL_TEXTURE_MAX_ANISOTROPY_EXT: smp->max_anisotropy = (GLfloat)param; break;
        default: assert(false && "gl_mock: unhandled glSamplerParameteri pname"); break;
    }
}

void glSamplerParameterf(GLuint sampler, GLenum pname, GLfloat param) {
    _GLM_REC(glSamplerParameterf, _glm_ai(sampler), _glm_ai(pname), _glm_af(param));
    if (!_glm_valid(GL_MOCK_OBJ_SAMPLER, sampler)) {
        return;
    }
    gl_mock_sampler_info_t* smp = &_glm.sampler[_glm_idx(sampler)];
    switch (pname) {
        case GL_TEXTURE_MIN_LOD: smp->min_lod = param; break;
        case GL_TEXTURE_MAX_LOD: smp->max_lod = param; break;
        case GL_TEXTURE_MAX_ANISOTROPY_EXT: smp->max_anisotropy = param; break;
        default: assert(false && "gl_mock: unhandled glSamplerParameterf pname"); break;
    }
}

void glSamplerParameterfv(GLuint sampler, GLenum pname, const GLfloat* params) {
    assert(params);
    _GLM_REC(glSamplerParameterfv, _glm_ai(sampler), _glm_ai(pname), _glm_af(params[0]), _glm_af(params[1]), _glm_af(params[2]), _glm_af(params[3]));
    if (!_glm_valid(GL_MOCK_OBJ_SAMPLER, sampler)) {
        return;
    }
    gl_mock_sampler_info_t* smp = &_glm.sampler[_glm_idx(sampler)];
    switch (pname) {
        case GL_TEXTURE_BORDER_COLOR:
            for (int i = 0; i < 4; i++) {
                smp->border_color[i] = params[i];
            }
            break;
        default:
            assert(false && "gl_mock: unhandled glSamplerParameterfv pname");
            break;
    }
}

//-- framebuffers and renderbuffers --------------------------------------------
void glGenFramebuffers(GLsizei n, GLuint* framebuffers) {
    _GLM_REC(glGenFramebuffers, _glm_ai(n), _glm_ap(framebuffers));
    assert(framebuffers);
    for (GLsizei i = 0; i < n; i++) {
        framebuffers[i] = _glm_alloc(GL_MOCK_OBJ_FRAMEBUFFER);
        memset(&_glm.framebuffer[_glm_idx(framebuffers[i])], 0, sizeof(gl_mock_framebuffer_info_t));
    }
}

void glDeleteFramebuffers(GLsizei n, const GLuint* framebuffers) {
    _GLM_REC(glDeleteFramebuffers, _glm_ai(n), _glm_ap(framebuffers));
    assert(framebuffers);
    for (GLsizei i = 0; i < n; i++) {
        if (_glm.bindings.draw_framebuffer == framebuffers[i]) {
            _glm.bindings.draw_framebuffer = 0;
        }
        if (_glm.bindings.read_framebuffer == framebuffers[i]) {
            _glm.bindings.read_framebuffer = 0;
        }
        _glm_free(GL_MOCK_OBJ_FRAMEBUFFER, framebuffers[i]);
    }
}

void glBindFramebuffer(GLenum target, GLuint framebuffer) {
    _GLM_REC(glBindFramebuffer, _glm_ai(target), _glm_ai(framebuffer));
    switch (target) {
        case GL_FRAMEBUFFER:
            _glm.bindings.draw_framebuffer = framebuffer;
            _glm.bindings.read_framebuffer = framebuffer;
            break;
        case GL_DRAW_FRAMEBUFFER:
            _glm.bindings.draw_framebuffer = framebuffer;
            break;
        case GL_READ_FRAMEBUFFER:
            _glm.bindings.read_framebuffer = framebuffer;
            break;
        default:
            assert(false && "gl_mock: unknown framebuffer target");
            break;
    }
}

void glFramebufferTexture2D(GLenum target, GLenum attachment, GLenum textarget, GLuint texture, GLint level) {
    _GLM_REC(glFramebufferTexture2D, _glm_ai(target), _glm_ai(attachment), _glm_ai(textarget), _glm_ai(texture), _glm_ai(level));
    _glm_set_attachment(target, attachment, textarget, texture, 0, level, -1);
}

void glFramebufferTextureLayer(GLenum target, GLenum attachment, GLuint texture, GLint level, GLint layer) {
    _GLM_REC(glFramebufferTextureLayer, _glm_ai(target), _glm_ai(attachment), _glm_ai(texture), _glm_ai(level), _glm_ai(layer));
    _glm_set_attachment(target, attachment, 0, texture, 0, level, layer);
}

void glFramebufferRenderbuffer(GLenum target, GLenum attachment, GLenum renderbuffertarget, GLuint renderbuffer) {
    _GLM_REC(glFramebufferRenderbuffer, _glm_ai(target), _glm_ai(attachment), _glm_ai(renderbuffertarget), _glm_ai(renderbuffer));
    assert(renderbuffertarget == GL_RENDERBUFFER);
    _glm_set_attachment(target, attachment, 0, 0, renderbuffer, 0, -1);
}

GLenum glCheckFramebufferStatus(GLenum target) {
    _GLM_REC(glCheckFramebufferStatus, _glm_ai(target));
    return _glm.framebuffer_status;
}

void glDrawBuffers(GLsizei n, const GLenum* bufs) {
    assert(bufs);
    _GLM_REC(glDrawBuffers, _glm_ai(n), _glm_ap(bufs));
    gl_mock_framebuffer_info_t* fb = _glm_fb_info(_glm.bindings.draw_framebuffer);
    if (fb) {
        fb->num_draw_buffers = 0;
        for (GLsizei i = 0; (i < n) && (i < GL_MOCK_MAX_COLOR_ATTACHMENTS); i++) {
            fb->draw_buffers[fb->num_draw_buffers++] = bufs[i];
        }
    }
}

void glReadBuffer(GLenum src) {
    _GLM_REC(glReadBuffer, _glm_ai(src));
    gl_mock_framebuffer_info_t* fb = _glm_fb_info(_glm.bindings.read_framebuffer);
    if (fb) {
        fb->read_buffer = src;
    }
}

void glBlitFramebuffer(GLint srcX0, GLint srcY0, GLint srcX1, GLint srcY1, GLint dstX0, GLint dstY0, GLint dstX1, GLint dstY1, GLbitfield mask, GLenum filter) {
    _GLM_REC(glBlitFramebuffer, _glm_ai(srcX0), _glm_ai(srcY0), _glm_ai(srcX1), _glm_ai(srcY1), _glm_ai(dstX0), _glm_ai(dstY0), _glm_ai(dstX1), _glm_ai(dstY1), _glm_ai(mask), _glm_ai(filter));
}

// only the first four attachment enums are logged
void glInvalidateFramebuffer(GLenum target, GLsizei numAttachments, const GLenum* attachments) {
    assert(attachments || (numAttachments == 0));
    GLenum att[4] = { 0, 0, 0, 0 };
    for (GLsizei i = 0; (i < numAttachments) && (i < 4); i++) {
        att[i] = attachments[i];
    }
    _GLM_REC(glInvalidateFramebuffer, _glm_ai(target), _glm_ai(numAttachments), _glm_ai(att[0]), _glm_ai(att[1]), _glm_ai(att[2]), _glm_ai(att[3]));
}

void glGenRenderbuffers(GLsizei n, GLuint* renderbuffers) {
    _GLM_REC(glGenRenderbuffers, _glm_ai(n), _glm_ap(renderbuffers));
    assert(renderbuffers);
    for (GLsizei i = 0; i < n; i++) {
        renderbuffers[i] = _glm_alloc(GL_MOCK_OBJ_RENDERBUFFER);
        memset(&_glm.renderbuffer[_glm_idx(renderbuffers[i])], 0, sizeof(gl_mock_renderbuffer_info_t));
    }
}

void glDeleteRenderbuffers(GLsizei n, const GLuint* renderbuffers) {
    _GLM_REC(glDeleteRenderbuffers, _glm_ai(n), _glm_ap(renderbuffers));
    assert(renderbuffers);
    for (GLsizei i = 0; i < n; i++) {
        if (_glm.bindings.renderbuffer == renderbuffers[i]) {
            _glm.bindings.renderbuffer = 0;
        }
        _glm_free(GL_MOCK_OBJ_RENDERBUFFER, renderbuffers[i]);
    }
}

void glBindRenderbuffer(GLenum target, GLuint renderbuffer) {
    _GLM_REC(glBindRenderbuffer, _glm_ai(target), _glm_ai(renderbuffer));
    assert(target == GL_RENDERBUFFER);
    _GLM_UNUSED(target);
    _glm.bindings.renderbuffer = renderbuffer;
}

void glRenderbufferStorage(GLenum target, GLenum internalformat, GLsizei width, GLsizei height) {
    _GLM_REC(glRenderbufferStorage, _glm_ai(target), _glm_ai(internalformat), _glm_ai(width), _glm_ai(height));
    gl_mock_renderbuffer_info_t* rbuf = _glm_cur_rbuf(target);
    if (rbuf) {
        rbuf->internal_format = internalformat;
        rbuf->width = width;
        rbuf->height = height;
        rbuf->samples = 0;
    }
}

void glRenderbufferStorageMultisample(GLenum target, GLsizei samples, GLenum internalformat, GLsizei width, GLsizei height) {
    _GLM_REC(glRenderbufferStorageMultisample, _glm_ai(target), _glm_ai(samples), _glm_ai(internalformat), _glm_ai(width), _glm_ai(height));
    gl_mock_renderbuffer_info_t* rbuf = _glm_cur_rbuf(target);
    if (rbuf) {
        rbuf->internal_format = internalformat;
        rbuf->width = width;
        rbuf->height = height;
        rbuf->samples = samples;
    }
}

//-- shaders and programs ------------------------------------------------------
GLuint glCreateShader(GLenum type) {
    _GLM_REC(glCreateShader, _glm_ai(type));
    const GLuint shd = _glm_alloc(GL_MOCK_OBJ_SHADER);
    gl_mock_shader_info_t* info = &_glm.shader[_glm_idx(shd)];
    memset(info, 0, sizeof(*info));
    info->type = type;
    return shd;
}

// only the first source string is kept, and only truncated
void glShaderSource(GLuint shader, GLsizei count, const GLchar* const* string, const GLint* length) {
    _GLM_REC(glShaderSource, _glm_ai(shader), _glm_ai(count), _glm_ap(string), _glm_ap(length));
    if (!_glm_valid(GL_MOCK_OBJ_SHADER, shader) || (count <= 0) || (0 == string) || (0 == string[0])) {
        return;
    }
    gl_mock_shader_info_t* info = &_glm.shader[_glm_idx(shader)];
    memset(info->source, 0, sizeof(info->source));
    strncpy(info->source, string[0], sizeof(info->source) - 1);
}

void glCompileShader(GLuint shader) {
    _GLM_REC(glCompileShader, _glm_ai(shader));
    bool ok = true;
    if (_glm.fail_compile > 0) {
        _glm.fail_compile--;
        ok = false;
    }
    if (_glm_valid(GL_MOCK_OBJ_SHADER, shader)) {
        _glm.shader[_glm_idx(shader)].compiled = ok;
    }
}

void glGetShaderiv(GLuint shader, GLenum pname, GLint* params) {
    assert(params);
    // not logged, this is a pure query
    const bool compiled = _glm_valid(GL_MOCK_OBJ_SHADER, shader) && _glm.shader[_glm_idx(shader)].compiled;
    switch (pname) {
        case GL_COMPILE_STATUS: *params = compiled ? GL_TRUE : GL_FALSE; break;
        case GL_INFO_LOG_LENGTH: *params = (GLint)strlen(_glm.info_log) + 1; break;
        default: assert(false && "gl_mock: unhandled glGetShaderiv pname"); *params = 0; break;
    }
}

void glGetShaderInfoLog(GLuint shader, GLsizei bufSize, GLsizei* length, GLchar* infoLog) {
    _GLM_UNUSED(shader);
    // not logged, this is a pure query
    _glm_copy_info_log(bufSize, length, infoLog);
}

void glDeleteShader(GLuint shader) {
    _GLM_REC(glDeleteShader, _glm_ai(shader));
    _glm_free(GL_MOCK_OBJ_SHADER, shader);
}

GLuint glCreateProgram(void) {
    _GLM_REC0(glCreateProgram);
    const GLuint prog = _glm_alloc(GL_MOCK_OBJ_PROGRAM);
    memset(&_glm.program[_glm_idx(prog)], 0, sizeof(gl_mock_program_info_t));
    memset(&_glm.prog_priv[_glm_idx(prog)], 0, sizeof(_glm_prog_priv_t));
    return prog;
}

void glAttachShader(GLuint program, GLuint shader) {
    _GLM_REC(glAttachShader, _glm_ai(program), _glm_ai(shader));
    if (!_glm_valid(GL_MOCK_OBJ_PROGRAM, program)) {
        return;
    }
    gl_mock_program_info_t* info = &_glm.program[_glm_idx(program)];
    assert(info->num_attached_shaders < GL_MOCK_MAX_ATTACHED_SHADERS);
    if (info->num_attached_shaders < GL_MOCK_MAX_ATTACHED_SHADERS) {
        info->attached_shaders[info->num_attached_shaders++] = shader;
    }
}

void glLinkProgram(GLuint program) {
    _GLM_REC(glLinkProgram, _glm_ai(program));
    bool ok = true;
    if (_glm.fail_link > 0) {
        _glm.fail_link--;
        ok = false;
    }
    if (_glm_valid(GL_MOCK_OBJ_PROGRAM, program)) {
        _glm.program[_glm_idx(program)].linked = ok;
    }
}

void glGetProgramiv(GLuint program, GLenum pname, GLint* params) {
    assert(params);
    // not logged, this is a pure query
    const bool linked = _glm_valid(GL_MOCK_OBJ_PROGRAM, program) && _glm.program[_glm_idx(program)].linked;
    switch (pname) {
        case GL_LINK_STATUS: *params = linked ? GL_TRUE : GL_FALSE; break;
        case GL_INFO_LOG_LENGTH: *params = (GLint)strlen(_glm.info_log) + 1; break;
        default: assert(false && "gl_mock: unhandled glGetProgramiv pname"); *params = 0; break;
    }
}

void glGetProgramInfoLog(GLuint program, GLsizei bufSize, GLsizei* length, GLchar* infoLog) {
    _GLM_UNUSED(program);
    // not logged, this is a pure query
    _glm_copy_info_log(bufSize, length, infoLog);
}

void glDeleteProgram(GLuint program) {
    _GLM_REC(glDeleteProgram, _glm_ai(program));
    if (_glm.bindings.program == program) {
        _glm.bindings.program = 0;
    }
    _glm_free(GL_MOCK_OBJ_PROGRAM, program);
}

void glUseProgram(GLuint program) {
    _GLM_REC(glUseProgram, _glm_ai(program));
    _glm.bindings.program = program;
}

// hands out successive locations, starting at 0 for each new program
GLint glGetAttribLocation(GLuint program, const GLchar* name) {
    _GLM_REC(glGetAttribLocation, _glm_ai(program), _glm_ap(name));
    if (_glm.fail_attrib_loc > 0) {
        _glm.fail_attrib_loc--;
        return -1;
    }
    if (!_glm_valid(GL_MOCK_OBJ_PROGRAM, program)) {
        return -1;
    }
    _glm_prog_priv_t* priv = &_glm.prog_priv[_glm_idx(program)];
    if (priv->next_attrib_loc >= _GLM_MAX_VERTEX_ATTRIBS) {
        return -1;
    }
    return priv->next_attrib_loc++;
}

// hands out successive locations, starting at 0 for each new program
GLint glGetUniformLocation(GLuint program, const GLchar* name) {
    _GLM_REC(glGetUniformLocation, _glm_ai(program), _glm_ap(name));
    if (_glm.fail_uniform_loc > 0) {
        _glm.fail_uniform_loc--;
        return -1;
    }
    if (!_glm_valid(GL_MOCK_OBJ_PROGRAM, program)) {
        return -1;
    }
    return _glm.prog_priv[_glm_idx(program)].next_uniform_loc++;
}

//-- uniforms ------------------------------------------------------------------
// the *v variants log at most the first four values
#define _GLM_UNIFORM_FV(func_name, num_comps) \
    void func_name(GLint location, GLsizei count, const GLfloat* value) { \
        assert(value); \
        GLfloat v[4] = { 0.0f, 0.0f, 0.0f, 0.0f }; \
        for (int i = 0; (i < (num_comps)) && (i < 4); i++) { \
            v[i] = value[i]; \
        } \
        _GLM_REC(func_name, _glm_ai(location), _glm_ai(count), _glm_af(v[0]), _glm_af(v[1]), _glm_af(v[2]), _glm_af(v[3])); \
    }

#define _GLM_UNIFORM_IV(func_name, num_comps) \
    void func_name(GLint location, GLsizei count, const GLint* value) { \
        assert(value); \
        GLint v[4] = { 0, 0, 0, 0 }; \
        for (int i = 0; (i < (num_comps)) && (i < 4); i++) { \
            v[i] = value[i]; \
        } \
        _GLM_REC(func_name, _glm_ai(location), _glm_ai(count), _glm_ai(v[0]), _glm_ai(v[1]), _glm_ai(v[2]), _glm_ai(v[3])); \
    }

_GLM_UNIFORM_FV(glUniform1fv, 1)
_GLM_UNIFORM_FV(glUniform2fv, 2)
_GLM_UNIFORM_FV(glUniform3fv, 3)
_GLM_UNIFORM_FV(glUniform4fv, 4)
_GLM_UNIFORM_IV(glUniform1iv, 1)
_GLM_UNIFORM_IV(glUniform2iv, 2)
_GLM_UNIFORM_IV(glUniform3iv, 3)
_GLM_UNIFORM_IV(glUniform4iv, 4)

void glUniform1i(GLint location, GLint v0) {
    _GLM_REC(glUniform1i, _glm_ai(location), _glm_ai(v0));
}

// only the first four matrix elements are logged
void glUniformMatrix4fv(GLint location, GLsizei count, GLboolean transpose, const GLfloat* value) {
    assert(value);
    _GLM_REC(glUniformMatrix4fv, _glm_ai(location), _glm_ai(count), _glm_ai(transpose), _glm_af(value[0]), _glm_af(value[1]), _glm_af(value[2]), _glm_af(value[3]));
}

//-- render state --------------------------------------------------------------
void glEnable(GLenum cap) {
    _GLM_REC(glEnable, _glm_ai(cap));
    const int i = _glm_cap_index(cap);
    if (i >= 0) {
        _glm.cap_enabled[i] = true;
    }
}

void glDisable(GLenum cap) {
    _GLM_REC(glDisable, _glm_ai(cap));
    const int i = _glm_cap_index(cap);
    if (i >= 0) {
        _glm.cap_enabled[i] = false;
    }
}

void glDepthFunc(GLenum func) {
    _GLM_REC(glDepthFunc, _glm_ai(func));
    _glm.render_state.depth_func = func;
}

void glDepthMask(GLboolean flag) {
    _GLM_REC(glDepthMask, _glm_ai(flag));
    _glm.render_state.depth_mask = flag;
}

void glStencilFunc(GLenum func, GLint ref, GLuint mask) {
    _GLM_REC(glStencilFunc, _glm_ai(func), _glm_ai(ref), _glm_ai(mask));
    gl_mock_stencil_face_t* faces[2] = { &_glm.render_state.stencil_front, &_glm.render_state.stencil_back };
    for (int i = 0; i < 2; i++) {
        faces[i]->func = func;
        faces[i]->ref = ref;
        faces[i]->mask = mask;
    }
}

void glStencilFuncSeparate(GLenum face, GLenum func, GLint ref, GLuint mask) {
    _GLM_REC(glStencilFuncSeparate, _glm_ai(face), _glm_ai(func), _glm_ai(ref), _glm_ai(mask));
    gl_mock_stencil_face_t* f = (face == GL_FRONT) ? &_glm.render_state.stencil_front : &_glm.render_state.stencil_back;
    assert((face == GL_FRONT) || (face == GL_BACK));
    f->func = func;
    f->ref = ref;
    f->mask = mask;
}

void glStencilOp(GLenum fail, GLenum zfail, GLenum zpass) {
    _GLM_REC(glStencilOp, _glm_ai(fail), _glm_ai(zfail), _glm_ai(zpass));
    gl_mock_stencil_face_t* faces[2] = { &_glm.render_state.stencil_front, &_glm.render_state.stencil_back };
    for (int i = 0; i < 2; i++) {
        faces[i]->fail_op = fail;
        faces[i]->depth_fail_op = zfail;
        faces[i]->pass_op = zpass;
    }
}

void glStencilOpSeparate(GLenum face, GLenum sfail, GLenum dpfail, GLenum dppass) {
    _GLM_REC(glStencilOpSeparate, _glm_ai(face), _glm_ai(sfail), _glm_ai(dpfail), _glm_ai(dppass));
    gl_mock_stencil_face_t* f = (face == GL_FRONT) ? &_glm.render_state.stencil_front : &_glm.render_state.stencil_back;
    assert((face == GL_FRONT) || (face == GL_BACK));
    f->fail_op = sfail;
    f->depth_fail_op = dpfail;
    f->pass_op = dppass;
}

void glStencilMask(GLuint mask) {
    _GLM_REC(glStencilMask, _glm_ai(mask));
    _glm.render_state.stencil_write_mask = mask;
}

void glBlendFunc(GLenum sfactor, GLenum dfactor) {
    _GLM_REC(glBlendFunc, _glm_ai(sfactor), _glm_ai(dfactor));
    _glm.render_state.blend_src_rgb = sfactor;
    _glm.render_state.blend_dst_rgb = dfactor;
    _glm.render_state.blend_src_alpha = sfactor;
    _glm.render_state.blend_dst_alpha = dfactor;
}

void glBlendFuncSeparate(GLenum sfactorRGB, GLenum dfactorRGB, GLenum sfactorAlpha, GLenum dfactorAlpha) {
    _GLM_REC(glBlendFuncSeparate, _glm_ai(sfactorRGB), _glm_ai(dfactorRGB), _glm_ai(sfactorAlpha), _glm_ai(dfactorAlpha));
    _glm.render_state.blend_src_rgb = sfactorRGB;
    _glm.render_state.blend_dst_rgb = dfactorRGB;
    _glm.render_state.blend_src_alpha = sfactorAlpha;
    _glm.render_state.blend_dst_alpha = dfactorAlpha;
}

void glBlendEquationSeparate(GLenum modeRGB, GLenum modeAlpha) {
    _GLM_REC(glBlendEquationSeparate, _glm_ai(modeRGB), _glm_ai(modeAlpha));
    _glm.render_state.blend_op_rgb = modeRGB;
    _glm.render_state.blend_op_alpha = modeAlpha;
}

void glBlendColor(GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha) {
    _GLM_REC(glBlendColor, _glm_af(red), _glm_af(green), _glm_af(blue), _glm_af(alpha));
    _glm.render_state.blend_color[0] = red;
    _glm.render_state.blend_color[1] = green;
    _glm.render_state.blend_color[2] = blue;
    _glm.render_state.blend_color[3] = alpha;
}

void glColorMask(GLboolean red, GLboolean green, GLboolean blue, GLboolean alpha) {
    _GLM_REC(glColorMask, _glm_ai(red), _glm_ai(green), _glm_ai(blue), _glm_ai(alpha));
    for (int i = 0; i < GL_MOCK_MAX_COLOR_ATTACHMENTS; i++) {
        _glm.render_state.color_mask[i][0] = red;
        _glm.render_state.color_mask[i][1] = green;
        _glm.render_state.color_mask[i][2] = blue;
        _glm.render_state.color_mask[i][3] = alpha;
    }
}

void glColorMaski(GLuint buf, GLboolean red, GLboolean green, GLboolean blue, GLboolean alpha) {
    _GLM_REC(glColorMaski, _glm_ai(buf), _glm_ai(red), _glm_ai(green), _glm_ai(blue), _glm_ai(alpha));
    assert(buf < (GLuint)GL_MOCK_MAX_COLOR_ATTACHMENTS);
    _glm.render_state.color_mask[buf][0] = red;
    _glm.render_state.color_mask[buf][1] = green;
    _glm.render_state.color_mask[buf][2] = blue;
    _glm.render_state.color_mask[buf][3] = alpha;
}

void glCullFace(GLenum mode) {
    _GLM_REC(glCullFace, _glm_ai(mode));
    _glm.render_state.cull_face = mode;
}

void glFrontFace(GLenum mode) {
    _GLM_REC(glFrontFace, _glm_ai(mode));
    _glm.render_state.front_face = mode;
}

void glPolygonOffset(GLfloat factor, GLfloat units) {
    _GLM_REC(glPolygonOffset, _glm_af(factor), _glm_af(units));
    _glm.render_state.polygon_offset_factor = factor;
    _glm.render_state.polygon_offset_units = units;
}

void glViewport(GLint x, GLint y, GLsizei width, GLsizei height) {
    _GLM_REC(glViewport, _glm_ai(x), _glm_ai(y), _glm_ai(width), _glm_ai(height));
    _glm.render_state.viewport[0] = x;
    _glm.render_state.viewport[1] = y;
    _glm.render_state.viewport[2] = width;
    _glm.render_state.viewport[3] = height;
}

void glScissor(GLint x, GLint y, GLsizei width, GLsizei height) {
    _GLM_REC(glScissor, _glm_ai(x), _glm_ai(y), _glm_ai(width), _glm_ai(height));
    _glm.render_state.scissor[0] = x;
    _glm.render_state.scissor[1] = y;
    _glm.render_state.scissor[2] = width;
    _glm.render_state.scissor[3] = height;
}

void glPixelStorei(GLenum pname, GLint param) {
    _GLM_REC(glPixelStorei, _glm_ai(pname), _glm_ai(param));
    switch (pname) {
        case GL_UNPACK_ALIGNMENT: _glm.render_state.unpack_alignment = param; break;
        case GL_UNPACK_ROW_LENGTH: _glm.render_state.unpack_row_length = param; break;
        case GL_UNPACK_IMAGE_HEIGHT: _glm.render_state.unpack_image_height = param; break;
        default: assert(false && "gl_mock: unhandled glPixelStorei pname"); break;
    }
}

//-- clear ---------------------------------------------------------------------
void glClearBufferfv(GLenum buffer, GLint drawbuffer, const GLfloat* value) {
    assert(value);
    GLfloat v[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
    const int n = (buffer == GL_COLOR) ? 4 : 1;
    for (int i = 0; i < n; i++) {
        v[i] = value[i];
    }
    _GLM_REC(glClearBufferfv, _glm_ai(buffer), _glm_ai(drawbuffer), _glm_af(v[0]), _glm_af(v[1]), _glm_af(v[2]), _glm_af(v[3]));
}

void glClearBufferiv(GLenum buffer, GLint drawbuffer, const GLint* value) {
    assert(value);
    GLint v[4] = { 0, 0, 0, 0 };
    const int n = (buffer == GL_COLOR) ? 4 : 1;
    for (int i = 0; i < n; i++) {
        v[i] = value[i];
    }
    _GLM_REC(glClearBufferiv, _glm_ai(buffer), _glm_ai(drawbuffer), _glm_ai(v[0]), _glm_ai(v[1]), _glm_ai(v[2]), _glm_ai(v[3]));
}

void glClearBufferuiv(GLenum buffer, GLint drawbuffer, const GLuint* value) {
    assert(value);
    GLuint v[4] = { 0, 0, 0, 0 };
    const int n = (buffer == GL_COLOR) ? 4 : 1;
    for (int i = 0; i < n; i++) {
        v[i] = value[i];
    }
    _GLM_REC(glClearBufferuiv, _glm_ai(buffer), _glm_ai(drawbuffer), _glm_ai(v[0]), _glm_ai(v[1]), _glm_ai(v[2]), _glm_ai(v[3]));
}

void glClearBufferfi(GLenum buffer, GLint drawbuffer, GLfloat depth, GLint stencil) {
    _GLM_REC(glClearBufferfi, _glm_ai(buffer), _glm_ai(drawbuffer), _glm_af(depth), _glm_ai(stencil));
}

//-- draw and dispatch ---------------------------------------------------------
void glDrawArrays(GLenum mode, GLint first, GLsizei count) {
    _GLM_REC(glDrawArrays, _glm_ai(mode), _glm_ai(first), _glm_ai(count));
}

void glDrawArraysInstanced(GLenum mode, GLint first, GLsizei count, GLsizei instancecount) {
    _GLM_REC(glDrawArraysInstanced, _glm_ai(mode), _glm_ai(first), _glm_ai(count), _glm_ai(instancecount));
}

void glDrawArraysInstancedBaseInstance(GLenum mode, GLint first, GLsizei count, GLsizei instancecount, GLuint baseinstance) {
    _GLM_REC(glDrawArraysInstancedBaseInstance, _glm_ai(mode), _glm_ai(first), _glm_ai(count), _glm_ai(instancecount), _glm_ai(baseinstance));
}

void glDrawElements(GLenum mode, GLsizei count, GLenum type, const void* indices) {
    _GLM_REC(glDrawElements, _glm_ai(mode), _glm_ai(count), _glm_ai(type), _glm_ap(indices));
}

void glDrawElementsInstanced(GLenum mode, GLsizei count, GLenum type, const void* indices, GLsizei instancecount) {
    _GLM_REC(glDrawElementsInstanced, _glm_ai(mode), _glm_ai(count), _glm_ai(type), _glm_ap(indices), _glm_ai(instancecount));
}

void glDrawElementsBaseVertex(GLenum mode, GLsizei count, GLenum type, const void* indices, GLint basevertex) {
    _GLM_REC(glDrawElementsBaseVertex, _glm_ai(mode), _glm_ai(count), _glm_ai(type), _glm_ap(indices), _glm_ai(basevertex));
}

void glDrawElementsInstancedBaseVertex(GLenum mode, GLsizei count, GLenum type, const void* indices, GLsizei instancecount, GLint basevertex) {
    _GLM_REC(glDrawElementsInstancedBaseVertex, _glm_ai(mode), _glm_ai(count), _glm_ai(type), _glm_ap(indices), _glm_ai(instancecount), _glm_ai(basevertex));
}

void glDrawElementsInstancedBaseVertexBaseInstance(GLenum mode, GLsizei count, GLenum type, const void* indices, GLsizei instancecount, GLint basevertex, GLuint baseinstance) {
    _GLM_REC(glDrawElementsInstancedBaseVertexBaseInstance, _glm_ai(mode), _glm_ai(count), _glm_ai(type), _glm_ap(indices), _glm_ai(instancecount), _glm_ai(basevertex), _glm_ai(baseinstance));
}

void glDispatchCompute(GLuint num_groups_x, GLuint num_groups_y, GLuint num_groups_z) {
    _GLM_REC(glDispatchCompute, _glm_ai(num_groups_x), _glm_ai(num_groups_y), _glm_ai(num_groups_z));
    _glm.last_dispatch.num_groups_x = num_groups_x;
    _glm.last_dispatch.num_groups_y = num_groups_y;
    _glm.last_dispatch.num_groups_z = num_groups_z;
}

void glMemoryBarrier(GLbitfield barriers) {
    _GLM_REC(glMemoryBarrier, _glm_ai(barriers));
}
