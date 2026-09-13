/*
    LLM maintained.

    gl_mock.h -- public API of the mocked GL 4.x / GLES 3.x runtime.

    Include this header instead of gl.h, and always *before* sokol_gfx.h.

    The mock offers four inspection and control areas:

    - object tracking: live counts and per-object state for buffers,
      textures, samplers, framebuffers, renderbuffers, vertex arrays,
      shaders and programs
    - call log: every GL call with its arguments, in call order
    - bound-state tracking: current bindings and render state
    - fault injection: forced shader/link failures, framebuffer status,
      GL errors, missing attribute/uniform locations and limit values

    Not thread-safe -- this is intended for the sokol-gfx unit-test loop,
    which runs on a single thread.
*/
#ifndef GL_MOCK_H_INCLUDED
#define GL_MOCK_H_INCLUDED

#include "gl.h"
#include <stdbool.h>

#define GL_MOCK_MAX_OBJECTS (256)
#define GL_MOCK_MAX_CALLS (4096)
#define GL_MOCK_MAX_CALL_ARGS (12)
#define GL_MOCK_MAX_TEXTURE_UNITS (32)
#define GL_MOCK_MAX_TEXTURE_TARGETS (6)
#define GL_MOCK_MAX_IMAGE_UNITS (16)
#define GL_MOCK_MAX_BUFFER_BINDINGS (16)
#define GL_MOCK_MAX_COLOR_ATTACHMENTS (8)
#define GL_MOCK_MAX_FB_ATTACHMENTS (10)
#define GL_MOCK_MAX_ATTACHED_SHADERS (3)
#define GL_MOCK_MAX_SHADER_SOURCE (256)
#define GL_MOCK_MAX_INFO_LOG (256)

#ifdef __cplusplus
extern "C" {
#endif

// function ids, one per entry in the _GLM_GL_FUNCS x-macro list
typedef enum {
    GL_MOCK_FUNC_INVALID = 0,
    #define _GLM_XMACRO(name, ret, args) GL_MOCK_FUNC_##name,
    _GLM_GL_FUNCS
    #undef _GLM_XMACRO
    GL_MOCK_FUNC_NUM,
} gl_mock_func_t;

// object kinds
typedef enum {
    GL_MOCK_OBJ_BUFFER = 0,
    GL_MOCK_OBJ_TEXTURE,
    GL_MOCK_OBJ_SAMPLER,
    GL_MOCK_OBJ_FRAMEBUFFER,
    GL_MOCK_OBJ_RENDERBUFFER,
    GL_MOCK_OBJ_VERTEXARRAY,
    GL_MOCK_OBJ_SHADER,
    GL_MOCK_OBJ_PROGRAM,
    GL_MOCK_OBJ_NUM,
} gl_mock_obj_t;

// one logged call argument, the reader must know the argument type
typedef union {
    int64_t i;
    uint64_t u;
    double f;
    const void* p;
} gl_mock_arg_t;

typedef struct {
    gl_mock_func_t func;
    int num_args;
    gl_mock_arg_t args[GL_MOCK_MAX_CALL_ARGS];
} gl_mock_call_t;

typedef struct {
    GLenum target;              // target of the most recent bind
    GLsizeiptr size;            // size of the most recent glBufferData
    GLenum usage;               // usage of the most recent glBufferData
    int num_data;               // number of glBufferData calls
    int num_subdata;            // number of glBufferSubData calls
} gl_mock_buffer_info_t;

typedef struct {
    GLenum target;              // target of the most recent bind
    GLenum internal_format;
    GLsizei width, height, depth;
    GLsizei levels;             // only valid if 'storage' is true
    GLsizei samples;            // >0 for multisample textures
    bool storage;               // created via one of the glTexStorage* calls
    bool is_view;               // created via glTextureView
    GLuint view_orig_texture;   // only valid if 'is_view' is true
    int num_image;              // number of glTexImage* / glCompressedTexImage* calls
    int num_subimage;           // number of glTexSubImage* / glCompressedTexSubImage* calls
} gl_mock_texture_info_t;

typedef struct {
    GLint min_filter, mag_filter;
    GLint wrap_s, wrap_t, wrap_r;
    GLint compare_mode, compare_func;
    GLfloat min_lod, max_lod;
    GLfloat max_anisotropy;
    GLfloat border_color[4];
} gl_mock_sampler_info_t;

typedef struct {
    GLenum attachment;          // GL_COLOR_ATTACHMENT0+n, GL_DEPTH_ATTACHMENT, ...
    GLenum textarget;           // 0 if a renderbuffer is attached
    GLuint texture;
    GLuint renderbuffer;
    GLint level;
    GLint layer;                // -1 if not a layered attachment
} gl_mock_fb_attachment_t;

typedef struct {
    int num_attachments;
    gl_mock_fb_attachment_t attachments[GL_MOCK_MAX_FB_ATTACHMENTS];
    int num_draw_buffers;
    GLenum draw_buffers[GL_MOCK_MAX_COLOR_ATTACHMENTS];
    GLenum read_buffer;
} gl_mock_framebuffer_info_t;

typedef struct {
    GLenum internal_format;
    GLsizei width, height;
    GLsizei samples;            // 0 if created via glRenderbufferStorage
} gl_mock_renderbuffer_info_t;

typedef struct {
    GLenum type;                // GL_VERTEX_SHADER, GL_FRAGMENT_SHADER, GL_COMPUTE_SHADER
    bool compiled;
    char source[GL_MOCK_MAX_SHADER_SOURCE];  // first source string, truncated
} gl_mock_shader_info_t;

typedef struct {
    bool linked;
    int num_attached_shaders;
    GLuint attached_shaders[GL_MOCK_MAX_ATTACHED_SHADERS];
} gl_mock_program_info_t;

typedef struct {
    GLuint program;
    GLuint vertex_array;
    GLuint draw_framebuffer;
    GLuint read_framebuffer;
    GLuint renderbuffer;
    GLuint array_buffer;
    GLuint element_array_buffer;
    GLuint shader_storage_buffer;
    GLenum active_texture;      // GL_TEXTURE0 + unit
} gl_mock_bindings_t;

typedef struct {
    GLuint texture;
    GLint level;
    GLboolean layered;
    GLint layer;
    GLenum access;
    GLenum format;
} gl_mock_image_binding_t;

typedef struct {
    GLuint buffer;
    GLintptr offset;
    GLsizeiptr size;            // 0 if bound via glBindBufferBase
} gl_mock_buffer_binding_t;

typedef struct {
    GLenum func;                // from glStencilFunc / glStencilFuncSeparate
    GLint ref;
    GLuint mask;
    GLenum fail_op;             // from glStencilOp / glStencilOpSeparate
    GLenum depth_fail_op;
    GLenum pass_op;
} gl_mock_stencil_face_t;

typedef struct {
    GLenum depth_func;
    GLboolean depth_mask;
    gl_mock_stencil_face_t stencil_front;
    gl_mock_stencil_face_t stencil_back;
    GLuint stencil_write_mask;
    GLenum blend_src_rgb, blend_dst_rgb;
    GLenum blend_src_alpha, blend_dst_alpha;
    GLenum blend_op_rgb, blend_op_alpha;
    GLfloat blend_color[4];
    GLboolean color_mask[GL_MOCK_MAX_COLOR_ATTACHMENTS][4];
    GLenum cull_face;
    GLenum front_face;
    GLfloat polygon_offset_factor, polygon_offset_units;
    GLint viewport[4];
    GLint scissor[4];
    GLint unpack_alignment;
    GLint unpack_row_length;
    GLint unpack_image_height;
} gl_mock_render_state_t;

typedef struct {
    GLuint num_groups_x, num_groups_y, num_groups_z;
} gl_mock_dispatch_t;

// setup and teardown
extern void gl_mock_setup(void);
extern void gl_mock_shutdown(void);
extern int gl_mock_version(void);       // 410, 430, 300, 310 or 320

// object tracking
extern int gl_mock_live_objects(gl_mock_obj_t kind);
extern int gl_mock_live_objects_total(void);
extern bool gl_mock_is_object(gl_mock_obj_t kind, GLuint name);
extern bool gl_mock_buffer_info(GLuint name, gl_mock_buffer_info_t* out);
extern bool gl_mock_texture_info(GLuint name, gl_mock_texture_info_t* out);
extern bool gl_mock_sampler_info(GLuint name, gl_mock_sampler_info_t* out);
extern bool gl_mock_framebuffer_info(GLuint name, gl_mock_framebuffer_info_t* out);
extern bool gl_mock_renderbuffer_info(GLuint name, gl_mock_renderbuffer_info_t* out);
extern bool gl_mock_shader_info(GLuint name, gl_mock_shader_info_t* out);
extern bool gl_mock_program_info(GLuint name, gl_mock_program_info_t* out);

// call log
extern int gl_mock_num_calls(void);
extern const gl_mock_call_t* gl_mock_call(int index);
extern int gl_mock_count_calls(gl_mock_func_t func);
extern int gl_mock_find_call(gl_mock_func_t func, int start_index);   // -1 if not found
extern const gl_mock_call_t* gl_mock_last_call(gl_mock_func_t func);  // 0 if not found
extern void gl_mock_clear_calls(void);
extern bool gl_mock_call_log_overflow(void);
extern const char* gl_mock_func_name(gl_mock_func_t func);

// bound state and render state
extern const gl_mock_bindings_t* gl_mock_bindings(void);
extern GLuint gl_mock_bound_texture(int unit, GLenum target);
extern GLuint gl_mock_bound_sampler(int unit);
extern const gl_mock_image_binding_t* gl_mock_image_binding(int unit);
extern const gl_mock_buffer_binding_t* gl_mock_buffer_binding(GLenum target, int index);
extern bool gl_mock_is_enabled(GLenum cap);
extern const gl_mock_render_state_t* gl_mock_render_state(void);
extern bool gl_mock_vertex_attrib_enabled(int index);
extern const gl_mock_dispatch_t* gl_mock_last_dispatch(void);

// fault injection
extern void gl_mock_fail_next_compile(int n);
extern void gl_mock_fail_next_link(int n);
extern void gl_mock_fail_next_attrib_location(int n);
extern void gl_mock_fail_next_uniform_location(int n);
extern void gl_mock_set_info_log(const char* msg);
extern void gl_mock_set_framebuffer_status(GLenum status);
extern void gl_mock_push_error(GLenum err);
extern void gl_mock_set_int(GLenum pname, GLint value);   // override a glGetIntegerv limit

#ifdef __cplusplus
} // extern "C"
#endif

#endif // GL_MOCK_H_INCLUDED
