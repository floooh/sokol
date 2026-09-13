/*
    LLM maintained.

    gl.h -- GL 4.x / GLES 3.x declarations for the sokol-gfx GL mock.

    This header replaces the platform GL headers. sokol_gfx.h must be
    built with SOKOL_EXTERNAL_GL_LOADER so it does not pull in any
    platform GL header of its own.

    The declared function set is exactly the set which sokol_gfx.h calls
    (the _SG_GL_FUNCS x-macro list, plus glInvalidateFramebuffer). The
    enum set is the set which sokol_gfx.h uses.

    Include this header (or gl_mock.h) *before* sokol_gfx.h.

    The selected GL version comes from GL_MOCK_VERSION:

        GLCORE: 410, 430
        GLES3:  300, 310, 320

    GL_MOCK_VERSION drives two things:

    - the GL_VERSION_* / GL_ES_VERSION_* defines which sokol_gfx.h uses
      to derive its _SOKOL_GL_HAS_* feature macros
    - the _SOKOL_GL_HAS_* feature macros themselves, which are predefined
      here so that the feature set follows the *selected* version and not
      the *host* platform

    Predefining the _SOKOL_GL_HAS_* macros is a benign redefinition:
    sokol_gfx.h spells them '(1)' and so do we, so a later identical
    redefinition is not a diagnostic.

    Known deviations:

    sokol_gfx.h can only add feature macros on top of the set predefined
    here, never remove them, and what it adds depends on the *host*
    platform. So the macros are a superset of the selected version:

    - Linux host, GLES3: adds _SOKOL_GL_HAS_COMPUTE, _COLORMASKI and
      _BASEVERTEX at every version, so GL_MOCK_VERSION 300 and 310
      compile in code which the selected version does not have
    - macOS host, GLES3: adds _SOKOL_GL_HAS_COLORMASKI, _BASEVERTEX and
      _DUALSOURCEBLENDING
    - Windows host, GLES3: adds _SOKOL_GL_HAS_COLORMASKI
    - GLCORE matches the selected version exactly on all three hosts

    The extra macros are harmless at runtime: every use site is also
    gated on an _sg.features flag derived from the version the mock
    reports. But tests must never key on _SOKOL_GL_HAS_*, gate on
    GL_MOCK_VERSION instead (see TEST_HAS_* in sokol_gfx_gl_test.c).
*/
#ifndef GL_MOCK_GL_H_INCLUDED
#define GL_MOCK_GL_H_INCLUDED

#include <stddef.h>
#include <stdint.h>

#if !defined(SOKOL_GLCORE) && !defined(SOKOL_GLES3)
#error "gl.h: define SOKOL_GLCORE or SOKOL_GLES3"
#endif

// selected GL version
#if !defined(GL_MOCK_VERSION)
    #if defined(SOKOL_GLES3)
        #define GL_MOCK_VERSION 300
    #else
        #define GL_MOCK_VERSION 410
    #endif
#endif

#if defined(SOKOL_GLES3)
    #if (GL_MOCK_VERSION != 300) && (GL_MOCK_VERSION != 310) && (GL_MOCK_VERSION != 320)
    #error "gl.h: GL_MOCK_VERSION must be 300, 310 or 320 for GLES3"
    #endif
#else
    #if (GL_MOCK_VERSION != 410) && (GL_MOCK_VERSION != 430)
    #error "gl.h: GL_MOCK_VERSION must be 410 or 430 for GLCORE"
    #endif
#endif

// version defines, sokol_gfx.h derives feature macros from those
#if defined(SOKOL_GLES3)
    #define GL_ES_VERSION_3_0 (1)
    #if GL_MOCK_VERSION >= 310
        #define GL_ES_VERSION_3_1 (1)
    #endif
    #if GL_MOCK_VERSION >= 320
        #define GL_ES_VERSION_3_2 (1)
    #endif
#else
    #define GL_VERSION_3_2 (1)
    #define GL_VERSION_3_3 (1)
    #define GL_VERSION_4_0 (1)
    #define GL_VERSION_4_1 (1)
    #if GL_MOCK_VERSION >= 420
        #define GL_VERSION_4_2 (1)
    #endif
    #if GL_MOCK_VERSION >= 430
        #define GL_VERSION_4_3 (1)
    #endif
#endif

// feature macros, predefined so they follow GL_MOCK_VERSION and not the host
#if defined(SOKOL_GLES3)
    #define _SOKOL_GL_HAS_TEXSTORAGE (1)
    #if GL_MOCK_VERSION >= 310
        #define _SOKOL_GL_HAS_COMPUTE (1)
    #endif
    #if GL_MOCK_VERSION >= 320
        #define _SOKOL_GL_HAS_COLORMASKI (1)
        #define _SOKOL_GL_HAS_BASEVERTEX (1)
    #endif
#else
    #define _SOKOL_GL_HAS_COLORMASKI (1)
    #define _SOKOL_GL_HAS_BASEVERTEX (1)
    #define _SOKOL_GL_HAS_DUALSOURCEBLENDING (1)
    #define _SOKOL_GL_HAS_MSAA_TEXTURES (1)
    #if GL_MOCK_VERSION >= 420
        #define _SOKOL_GL_HAS_TEXSTORAGE (1)
        #define _SOKOL_GL_HAS_BASEINSTANCE (1)
    #endif
    #if GL_MOCK_VERSION >= 430
        #define _SOKOL_GL_HAS_COMPUTE (1)
        #define _SOKOL_GL_HAS_TEXVIEWS (1)
    #endif
#endif

// GL types
typedef unsigned int    GLenum;
typedef unsigned int    GLuint;
typedef int             GLint;
typedef int             GLsizei;
typedef unsigned int    GLbitfield;
typedef char            GLchar;
typedef signed char     GLbyte;
typedef unsigned char   GLubyte;
typedef unsigned char   GLboolean;
typedef short           GLshort;
typedef unsigned short  GLushort;
typedef unsigned short  GLhalf;
typedef float           GLfloat;
typedef float           GLclampf;
typedef double          GLdouble;
typedef double          GLclampd;
typedef void            GLvoid;
typedef intptr_t        GLintptr;
typedef ptrdiff_t       GLsizeiptr;
typedef int64_t         GLint64;
typedef uint64_t        GLuint64;

// GL enums
#define GL_ALWAYS                                      0x0207
#define GL_ARRAY_BUFFER                                0x8892
#define GL_BACK                                        0x0405
#define GL_BLEND                                       0x0BE2
#define GL_BYTE                                        0x1400
#define GL_CCW                                         0x0901
#define GL_CLAMP_TO_BORDER                             0x812D
#define GL_CLAMP_TO_EDGE                               0x812F
#define GL_COLOR                                       0x1800
#define GL_COLOR_ATTACHMENT0                           0x8CE0
#define GL_COLOR_BUFFER_BIT                            0x00004000
#define GL_COMPARE_REF_TO_TEXTURE                      0x884E
#define GL_COMPILE_STATUS                              0x8B81
#define GL_COMPRESSED_R11_EAC                          0x9270
#define GL_COMPRESSED_RED_GREEN_RGTC2                  0x8DBD
#define GL_COMPRESSED_RED_RGTC1                        0x8DBB
#define GL_COMPRESSED_RG11_EAC                         0x9272
#define GL_COMPRESSED_RGB8_ETC2                        0x9274
#define GL_COMPRESSED_RGB8_PUNCHTHROUGH_ALPHA1_ETC2    0x9276
#define GL_COMPRESSED_RGBA8_ETC2_EAC                   0x9278
#define GL_COMPRESSED_RGBA_ASTC_4x4_KHR                0x93B0
#define GL_COMPRESSED_RGBA_BPTC_UNORM_ARB              0x8E8C
#define GL_COMPRESSED_RGBA_S3TC_DXT1_EXT               0x83F1
#define GL_COMPRESSED_RGBA_S3TC_DXT3_EXT               0x83F2
#define GL_COMPRESSED_RGBA_S3TC_DXT5_EXT               0x83F3
#define GL_COMPRESSED_RGB_BPTC_SIGNED_FLOAT_ARB        0x8E8E
#define GL_COMPRESSED_RGB_BPTC_UNSIGNED_FLOAT_ARB      0x8E8F
#define GL_COMPRESSED_SIGNED_R11_EAC                   0x9271
#define GL_COMPRESSED_SIGNED_RED_GREEN_RGTC2           0x8DBE
#define GL_COMPRESSED_SIGNED_RED_RGTC1                 0x8DBC
#define GL_COMPRESSED_SIGNED_RG11_EAC                  0x9273
#define GL_COMPRESSED_SRGB8_ALPHA8_ASTC_4x4_KHR        0x93D0
#define GL_COMPRESSED_SRGB8_ALPHA8_ETC2_EAC            0x9279
#define GL_COMPRESSED_SRGB8_ETC2                       0x9275
#define GL_COMPRESSED_SRGB_ALPHA_BPTC_UNORM_ARB        0x8E8D
#define GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT5_EXT         0x8C4F
#define GL_COMPUTE_SHADER                              0x91B9
#define GL_CONSTANT_ALPHA                              0x8003
#define GL_CONSTANT_COLOR                              0x8001
#define GL_CULL_FACE                                   0x0B44
#define GL_CURRENT_PROGRAM                             0x8B8D
#define GL_CW                                          0x0900
#define GL_DECR                                        0x1E03
#define GL_DECR_WRAP                                   0x8508
#define GL_DEPTH                                       0x1801
#define GL_DEPTH24_STENCIL8                            0x88F0
#define GL_DEPTH32F_STENCIL8                           0x8CAD
#define GL_DEPTH_ATTACHMENT                            0x8D00
#define GL_DEPTH_BUFFER_BIT                            0x00000100
#define GL_DEPTH_COMPONENT                             0x1902
#define GL_DEPTH_COMPONENT32F                          0x8CAC
#define GL_DEPTH_STENCIL                               0x84F9
#define GL_DEPTH_STENCIL_ATTACHMENT                    0x821A
#define GL_DEPTH_TEST                                  0x0B71
#define GL_DITHER                                      0x0BD0
#define GL_DRAW_FRAMEBUFFER                            0x8CA9
#define GL_DST_ALPHA                                   0x0304
#define GL_DST_COLOR                                   0x0306
#define GL_DYNAMIC_DRAW                                0x88E8
#define GL_ELEMENT_ARRAY_BARRIER_BIT                   0x00000002
#define GL_ELEMENT_ARRAY_BUFFER                        0x8893
#define GL_EQUAL                                       0x0202
#define GL_EXTENSIONS                                  0x1F03
#define GL_FALSE                                       0
#define GL_FLOAT                                       0x1406
#define GL_FLOAT_32_UNSIGNED_INT_24_8_REV              0x8DAD
#define GL_FRAGMENT_SHADER                             0x8B30
#define GL_FRAMEBUFFER                                 0x8D40
#define GL_FRAMEBUFFER_BARRIER_BIT                     0x00000400
#define GL_FRAMEBUFFER_BINDING                         0x8CA6
#define GL_FRAMEBUFFER_COMPLETE                        0x8CD5
#define GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT           0x8CD6
#define GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT   0x8CD7
#define GL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE          0x8D56
#define GL_FRAMEBUFFER_SRGB                            0x8DB9
#define GL_FRAMEBUFFER_UNDEFINED                       0x8219
#define GL_FRAMEBUFFER_UNSUPPORTED                     0x8CDD
#define GL_FRONT                                       0x0404
#define GL_FUNC_ADD                                    0x8006
#define GL_FUNC_REVERSE_SUBTRACT                       0x800B
#define GL_FUNC_SUBTRACT                               0x800A
#define GL_GEQUAL                                      0x0206
#define GL_GREATER                                     0x0204
#define GL_HALF_FLOAT                                  0x140B
#define GL_INCR                                        0x1E02
#define GL_INCR_WRAP                                   0x8507
#define GL_INFO_LOG_LENGTH                             0x8B84
#define GL_INT                                         0x1404
#define GL_INT_2_10_10_10_REV                          0x8D9F
#define GL_INVERT                                      0x150A
#define GL_KEEP                                        0x1E00
#define GL_LEQUAL                                      0x0203
#define GL_LESS                                        0x0201
#define GL_LINEAR                                      0x2601
#define GL_LINEAR_MIPMAP_LINEAR                        0x2703
#define GL_LINEAR_MIPMAP_NEAREST                       0x2701
#define GL_LINES                                       0x0001
#define GL_LINE_STRIP                                  0x0003
#define GL_LINK_STATUS                                 0x8B82
#define GL_LUMINANCE                                   0x1909
#define GL_MAJOR_VERSION                               0x821B
#define GL_MAX                                         0x8008
#define GL_MAX_3D_TEXTURE_SIZE                         0x8073
#define GL_MAX_ARRAY_TEXTURE_LAYERS                    0x88FF
#define GL_MAX_COMBINED_TEXTURE_IMAGE_UNITS            0x8B4D
#define GL_MAX_CUBE_MAP_TEXTURE_SIZE                   0x851C
#define GL_MAX_DRAW_BUFFERS                            0x8824
#define GL_MAX_IMAGE_UNITS                             0x8F38
#define GL_MAX_SHADER_STORAGE_BUFFER_BINDINGS          0x90DD
#define GL_MAX_TEXTURE_IMAGE_UNITS                     0x8872
#define GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT              0x84FF
#define GL_MAX_TEXTURE_SIZE                            0x0D33
#define GL_MAX_VERTEX_ATTRIBS                          0x8869
#define GL_MAX_VERTEX_UNIFORM_COMPONENTS               0x8B4A
#define GL_MIN                                         0x8007
#define GL_MINOR_VERSION                               0x821C
#define GL_MIRRORED_REPEAT                             0x8370
#define GL_MULTISAMPLE                                 0x809D
#define GL_NEAREST                                     0x2600
#define GL_NEAREST_MIPMAP_LINEAR                       0x2702
#define GL_NEAREST_MIPMAP_NEAREST                      0x2700
#define GL_NEVER                                       0x0200
#define GL_NONE                                        0
#define GL_NOTEQUAL                                    0x0205
#define GL_NO_ERROR                                    0
#define GL_NUM_EXTENSIONS                              0x821D
#define GL_ONE                                         1
#define GL_ONE_MINUS_CONSTANT_ALPHA                    0x8004
#define GL_ONE_MINUS_CONSTANT_COLOR                    0x8002
#define GL_ONE_MINUS_DST_ALPHA                         0x0305
#define GL_ONE_MINUS_DST_COLOR                         0x0307
#define GL_ONE_MINUS_SRC1_ALPHA                        0x88FB
#define GL_ONE_MINUS_SRC1_COLOR                        0x88FA
#define GL_ONE_MINUS_SRC_ALPHA                         0x0303
#define GL_ONE_MINUS_SRC_COLOR                         0x0301
#define GL_POINTS                                      0x0000
#define GL_POLYGON_OFFSET_FILL                         0x8037
#define GL_PROGRAM_POINT_SIZE                          0x8642
#define GL_R11F_G11F_B10F                              0x8C3A
#define GL_R16                                         0x822A
#define GL_R16F                                        0x822D
#define GL_R16I                                        0x8233
#define GL_R16UI                                       0x8234
#define GL_R16_SNORM                                   0x8F98
#define GL_R32F                                        0x822E
#define GL_R32I                                        0x8235
#define GL_R32UI                                       0x8236
#define GL_R8                                          0x8229
#define GL_R8I                                         0x8231
#define GL_R8UI                                        0x8232
#define GL_R8_SNORM                                    0x8F94
#define GL_READ_FRAMEBUFFER                            0x8CA8
#define GL_READ_WRITE                                  0x88BA
#define GL_RED                                         0x1903
#define GL_RED_INTEGER                                 0x8D94
#define GL_RENDERBUFFER                                0x8D41
#define GL_REPEAT                                      0x2901
#define GL_REPLACE                                     0x1E01
#define GL_RG                                          0x8227
#define GL_RG16                                        0x822C
#define GL_RG16F                                       0x822F
#define GL_RG16I                                       0x8239
#define GL_RG16UI                                      0x823A
#define GL_RG16_SNORM                                  0x8F99
#define GL_RG32F                                       0x8230
#define GL_RG32I                                       0x823B
#define GL_RG32UI                                      0x823C
#define GL_RG8                                         0x822B
#define GL_RG8I                                        0x8237
#define GL_RG8UI                                       0x8238
#define GL_RG8_SNORM                                   0x8F95
#define GL_RGB                                         0x1907
#define GL_RGB10_A2                                    0x8059
#define GL_RGB16I                                      0x8D89
#define GL_RGB16UI                                     0x8D77
#define GL_RGB16_SNORM                                 0x8F9A
#define GL_RGB32I                                      0x8D83
#define GL_RGB32UI                                     0x8D71
#define GL_RGB5                                        0x8050
#define GL_RGB5_A1                                     0x8057
#define GL_RGB8                                        0x8051
#define GL_RGB8I                                       0x8D8F
#define GL_RGB8UI                                      0x8D7D
#define GL_RGB8_SNORM                                  0x8F96
#define GL_RGB9_E5                                     0x8C3D
#define GL_RGBA                                        0x1908
#define GL_RGBA16                                      0x805B
#define GL_RGBA16F                                     0x881A
#define GL_RGBA16I                                     0x8D88
#define GL_RGBA16UI                                    0x8D76
#define GL_RGBA16_SNORM                                0x8F9B
#define GL_RGBA32F                                     0x8814
#define GL_RGBA32I                                     0x8D82
#define GL_RGBA32UI                                    0x8D70
#define GL_RGBA4                                       0x8056
#define GL_RGBA8                                       0x8058
#define GL_RGBA8I                                      0x8D8E
#define GL_RGBA8UI                                     0x8D7C
#define GL_RGBA8_SNORM                                 0x8F97
#define GL_RGBA_INTEGER                                0x8D99
#define GL_RG_INTEGER                                  0x8228
#define GL_SAMPLE_ALPHA_TO_COVERAGE                    0x809E
#define GL_SCISSOR_TEST                                0x0C11
#define GL_SHADER_IMAGE_ACCESS_BARRIER_BIT             0x00000020
#define GL_SHADER_STORAGE_BARRIER_BIT                  0x2000
#define GL_SHADER_STORAGE_BUFFER                       0x90D2
#define GL_SHORT                                       0x1402
#define GL_SRC1_ALPHA                                  0x8589
#define GL_SRC1_COLOR                                  0x88F9
#define GL_SRC_ALPHA                                   0x0302
#define GL_SRC_ALPHA_SATURATE                          0x0308
#define GL_SRC_COLOR                                   0x0300
#define GL_SRGB8_ALPHA8                                0x8C43
#define GL_STATIC_DRAW                                 0x88E4
#define GL_STENCIL                                     0x1802
#define GL_STENCIL_ATTACHMENT                          0x8D20
#define GL_STENCIL_BUFFER_BIT                          0x00000400
#define GL_STENCIL_TEST                                0x0B90
#define GL_STREAM_DRAW                                 0x88E0
#define GL_TEXTURE0                                    0x84C0
#define GL_TEXTURE_2D                                  0x0DE1
#define GL_TEXTURE_2D_ARRAY                            0x8C1A
#define GL_TEXTURE_2D_MULTISAMPLE                      0x9100
#define GL_TEXTURE_2D_MULTISAMPLE_ARRAY                0x9102
#define GL_TEXTURE_3D                                  0x806F
#define GL_TEXTURE_BORDER_COLOR                        0x1004
#define GL_TEXTURE_COMPARE_FUNC                        0x884D
#define GL_TEXTURE_COMPARE_MODE                        0x884C
#define GL_TEXTURE_CUBE_MAP                            0x8513
#define GL_TEXTURE_CUBE_MAP_NEGATIVE_X                 0x8516
#define GL_TEXTURE_CUBE_MAP_NEGATIVE_Y                 0x8518
#define GL_TEXTURE_CUBE_MAP_NEGATIVE_Z                 0x851A
#define GL_TEXTURE_CUBE_MAP_POSITIVE_X                 0x8515
#define GL_TEXTURE_CUBE_MAP_POSITIVE_Y                 0x8517
#define GL_TEXTURE_CUBE_MAP_POSITIVE_Z                 0x8519
#define GL_TEXTURE_CUBE_MAP_SEAMLESS                   0x884F
#define GL_TEXTURE_FETCH_BARRIER_BIT                   0x00000008
#define GL_TEXTURE_MAG_FILTER                          0x2800
#define GL_TEXTURE_MAX_ANISOTROPY_EXT                  0x84FE
#define GL_TEXTURE_MAX_LEVEL                           0x813D
#define GL_TEXTURE_MAX_LOD                             0x813B
#define GL_TEXTURE_MIN_FILTER                          0x2801
#define GL_TEXTURE_MIN_LOD                             0x813A
#define GL_TEXTURE_WRAP_R                              0x8072
#define GL_TEXTURE_WRAP_S                              0x2802
#define GL_TEXTURE_WRAP_T                              0x2803
#define GL_TRIANGLES                                   0x0004
#define GL_TRIANGLE_STRIP                              0x0005
#define GL_TRUE                                        1
#define GL_UNPACK_ALIGNMENT                            0x0CF5
#define GL_UNPACK_IMAGE_HEIGHT                         0x806E
#define GL_UNPACK_ROW_LENGTH                           0x0CF2
#define GL_UNSIGNED_BYTE                               0x1401
#define GL_UNSIGNED_INT                                0x1405
#define GL_UNSIGNED_INT_10F_11F_11F_REV                0x8C3B
#define GL_UNSIGNED_INT_24_8                           0x84FA
#define GL_UNSIGNED_INT_2_10_10_10_REV                 0x8368
#define GL_UNSIGNED_INT_5_9_9_9_REV                    0x8C3E
#define GL_UNSIGNED_SHORT                              0x1403
#define GL_UNSIGNED_SHORT_4_4_4_4                      0x8033
#define GL_UNSIGNED_SHORT_5_5_5_1                      0x8034
#define GL_UNSIGNED_SHORT_5_6_5                        0x8363
#define GL_VERTEX_ATTRIB_ARRAY_BARRIER_BIT             0x00000001
#define GL_VERTEX_SHADER                               0x8B31
#define GL_WRITE_ONLY                                  0x88B9
#define GL_ZERO                                        0

// X-macro list of all GL functions called by sokol_gfx.h. Also reused by
// gl_mock.h to generate the gl_mock_func_t enum, and by gl_mock.c to build
// the function name string table, so adding a new GL entry point only
// requires editing this one list.
#define _GLM_GL_FUNCS \
    _GLM_XMACRO(glBindVertexArray,                 void, (GLuint array)) \
    _GLM_XMACRO(glFramebufferTextureLayer,         void, (GLenum target, GLenum attachment, GLuint texture, GLint level, GLint layer)) \
    _GLM_XMACRO(glGenFramebuffers,                 void, (GLsizei n, GLuint * framebuffers)) \
    _GLM_XMACRO(glBindFramebuffer,                 void, (GLenum target, GLuint framebuffer)) \
    _GLM_XMACRO(glBindRenderbuffer,                void, (GLenum target, GLuint renderbuffer)) \
    _GLM_XMACRO(glGetStringi,                      const GLubyte *, (GLenum name, GLuint index)) \
    _GLM_XMACRO(glClearBufferfi,                   void, (GLenum buffer, GLint drawbuffer, GLfloat depth, GLint stencil)) \
    _GLM_XMACRO(glClearBufferfv,                   void, (GLenum buffer, GLint drawbuffer, const GLfloat * value)) \
    _GLM_XMACRO(glClearBufferuiv,                  void, (GLenum buffer, GLint drawbuffer, const GLuint * value)) \
    _GLM_XMACRO(glClearBufferiv,                   void, (GLenum buffer, GLint drawbuffer, const GLint * value)) \
    _GLM_XMACRO(glDeleteRenderbuffers,             void, (GLsizei n, const GLuint * renderbuffers)) \
    _GLM_XMACRO(glUniform1fv,                      void, (GLint location, GLsizei count, const GLfloat * value)) \
    _GLM_XMACRO(glUniform2fv,                      void, (GLint location, GLsizei count, const GLfloat * value)) \
    _GLM_XMACRO(glUniform3fv,                      void, (GLint location, GLsizei count, const GLfloat * value)) \
    _GLM_XMACRO(glUniform4fv,                      void, (GLint location, GLsizei count, const GLfloat * value)) \
    _GLM_XMACRO(glUniform1iv,                      void, (GLint location, GLsizei count, const GLint * value)) \
    _GLM_XMACRO(glUniform2iv,                      void, (GLint location, GLsizei count, const GLint * value)) \
    _GLM_XMACRO(glUniform3iv,                      void, (GLint location, GLsizei count, const GLint * value)) \
    _GLM_XMACRO(glUniform4iv,                      void, (GLint location, GLsizei count, const GLint * value)) \
    _GLM_XMACRO(glUniformMatrix4fv,                void, (GLint location, GLsizei count, GLboolean transpose, const GLfloat * value)) \
    _GLM_XMACRO(glUseProgram,                      void, (GLuint program)) \
    _GLM_XMACRO(glShaderSource,                    void, (GLuint shader, GLsizei count, const GLchar *const* string, const GLint * length)) \
    _GLM_XMACRO(glLinkProgram,                     void, (GLuint program)) \
    _GLM_XMACRO(glGetUniformLocation,              GLint, (GLuint program, const GLchar * name)) \
    _GLM_XMACRO(glGetShaderiv,                     void, (GLuint shader, GLenum pname, GLint * params)) \
    _GLM_XMACRO(glGetProgramInfoLog,               void, (GLuint program, GLsizei bufSize, GLsizei * length, GLchar * infoLog)) \
    _GLM_XMACRO(glGetAttribLocation,               GLint, (GLuint program, const GLchar * name)) \
    _GLM_XMACRO(glDisableVertexAttribArray,        void, (GLuint index)) \
    _GLM_XMACRO(glDeleteShader,                    void, (GLuint shader)) \
    _GLM_XMACRO(glDeleteProgram,                   void, (GLuint program)) \
    _GLM_XMACRO(glCompileShader,                   void, (GLuint shader)) \
    _GLM_XMACRO(glStencilFuncSeparate,             void, (GLenum face, GLenum func, GLint ref, GLuint mask)) \
    _GLM_XMACRO(glStencilOpSeparate,               void, (GLenum face, GLenum sfail, GLenum dpfail, GLenum dppass)) \
    _GLM_XMACRO(glRenderbufferStorageMultisample,  void, (GLenum target, GLsizei samples, GLenum internalformat, GLsizei width, GLsizei height)) \
    _GLM_XMACRO(glDrawBuffers,                     void, (GLsizei n, const GLenum * bufs)) \
    _GLM_XMACRO(glVertexAttribDivisor,             void, (GLuint index, GLuint divisor)) \
    _GLM_XMACRO(glBufferSubData,                   void, (GLenum target, GLintptr offset, GLsizeiptr size, const void * data)) \
    _GLM_XMACRO(glGenBuffers,                      void, (GLsizei n, GLuint * buffers)) \
    _GLM_XMACRO(glCheckFramebufferStatus,          GLenum, (GLenum target)) \
    _GLM_XMACRO(glFramebufferRenderbuffer,         void, (GLenum target, GLenum attachment, GLenum renderbuffertarget, GLuint renderbuffer)) \
    _GLM_XMACRO(glCompressedTexImage2D,            void, (GLenum target, GLint level, GLenum internalformat, GLsizei width, GLsizei height, GLint border, GLsizei imageSize, const void * data)) \
    _GLM_XMACRO(glCompressedTexImage3D,            void, (GLenum target, GLint level, GLenum internalformat, GLsizei width, GLsizei height, GLsizei depth, GLint border, GLsizei imageSize, const void * data)) \
    _GLM_XMACRO(glActiveTexture,                   void, (GLenum texture)) \
    _GLM_XMACRO(glTexSubImage3D,                   void, (GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLsizei width, GLsizei height, GLsizei depth, GLenum format, GLenum type, const void * pixels)) \
    _GLM_XMACRO(glRenderbufferStorage,             void, (GLenum target, GLenum internalformat, GLsizei width, GLsizei height)) \
    _GLM_XMACRO(glGenTextures,                     void, (GLsizei n, GLuint * textures)) \
    _GLM_XMACRO(glPolygonOffset,                   void, (GLfloat factor, GLfloat units)) \
    _GLM_XMACRO(glDrawElements,                    void, (GLenum mode, GLsizei count, GLenum type, const void * indices)) \
    _GLM_XMACRO(glDeleteFramebuffers,              void, (GLsizei n, const GLuint * framebuffers)) \
    _GLM_XMACRO(glBlendEquationSeparate,           void, (GLenum modeRGB, GLenum modeAlpha)) \
    _GLM_XMACRO(glDeleteTextures,                  void, (GLsizei n, const GLuint * textures)) \
    _GLM_XMACRO(glGetProgramiv,                    void, (GLuint program, GLenum pname, GLint * params)) \
    _GLM_XMACRO(glBindTexture,                     void, (GLenum target, GLuint texture)) \
    _GLM_XMACRO(glTexImage3D,                      void, (GLenum target, GLint level, GLint internalformat, GLsizei width, GLsizei height, GLsizei depth, GLint border, GLenum format, GLenum type, const void * pixels)) \
    _GLM_XMACRO(glCreateShader,                    GLuint, (GLenum type)) \
    _GLM_XMACRO(glTexSubImage2D,                   void, (GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLenum type, const void * pixels)) \
    _GLM_XMACRO(glFramebufferTexture2D,            void, (GLenum target, GLenum attachment, GLenum textarget, GLuint texture, GLint level)) \
    _GLM_XMACRO(glCreateProgram,                   GLuint, (void)) \
    _GLM_XMACRO(glViewport,                        void, (GLint x, GLint y, GLsizei width, GLsizei height)) \
    _GLM_XMACRO(glDeleteBuffers,                   void, (GLsizei n, const GLuint * buffers)) \
    _GLM_XMACRO(glDrawArrays,                      void, (GLenum mode, GLint first, GLsizei count)) \
    _GLM_XMACRO(glDrawElementsInstanced,           void, (GLenum mode, GLsizei count, GLenum type, const void * indices, GLsizei instancecount)) \
    _GLM_XMACRO(glVertexAttribPointer,             void, (GLuint index, GLint size, GLenum type, GLboolean normalized, GLsizei stride, const void * pointer)) \
    _GLM_XMACRO(glVertexAttribIPointer,            void, (GLuint index, GLint size, GLenum type, GLsizei stride, const void * pointer)) \
    _GLM_XMACRO(glUniform1i,                       void, (GLint location, GLint v0)) \
    _GLM_XMACRO(glDisable,                         void, (GLenum cap)) \
    _GLM_XMACRO(glColorMask,                       void, (GLboolean red, GLboolean green, GLboolean blue, GLboolean alpha)) \
    _GLM_XMACRO(glColorMaski,                      void, (GLuint buf, GLboolean red, GLboolean green, GLboolean blue, GLboolean alpha)) \
    _GLM_XMACRO(glBindBuffer,                      void, (GLenum target, GLuint buffer)) \
    _GLM_XMACRO(glDeleteVertexArrays,              void, (GLsizei n, const GLuint * arrays)) \
    _GLM_XMACRO(glDepthMask,                       void, (GLboolean flag)) \
    _GLM_XMACRO(glDrawArraysInstanced,             void, (GLenum mode, GLint first, GLsizei count, GLsizei instancecount)) \
    _GLM_XMACRO(glScissor,                         void, (GLint x, GLint y, GLsizei width, GLsizei height)) \
    _GLM_XMACRO(glGenRenderbuffers,                void, (GLsizei n, GLuint * renderbuffers)) \
    _GLM_XMACRO(glBufferData,                      void, (GLenum target, GLsizeiptr size, const void * data, GLenum usage)) \
    _GLM_XMACRO(glBlendFuncSeparate,               void, (GLenum sfactorRGB, GLenum dfactorRGB, GLenum sfactorAlpha, GLenum dfactorAlpha)) \
    _GLM_XMACRO(glTexParameteri,                   void, (GLenum target, GLenum pname, GLint param)) \
    _GLM_XMACRO(glGetIntegerv,                     void, (GLenum pname, GLint * data)) \
    _GLM_XMACRO(glEnable,                          void, (GLenum cap)) \
    _GLM_XMACRO(glBlitFramebuffer,                 void, (GLint srcX0, GLint srcY0, GLint srcX1, GLint srcY1, GLint dstX0, GLint dstY0, GLint dstX1, GLint dstY1, GLbitfield mask, GLenum filter)) \
    _GLM_XMACRO(glStencilMask,                     void, (GLuint mask)) \
    _GLM_XMACRO(glAttachShader,                    void, (GLuint program, GLuint shader)) \
    _GLM_XMACRO(glGetError,                        GLenum, (void)) \
    _GLM_XMACRO(glBlendColor,                      void, (GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha)) \
    _GLM_XMACRO(glTexParameterf,                   void, (GLenum target, GLenum pname, GLfloat param)) \
    _GLM_XMACRO(glTexParameterfv,                  void, (GLenum target, GLenum pname, const GLfloat* params)) \
    _GLM_XMACRO(glGetShaderInfoLog,                void, (GLuint shader, GLsizei bufSize, GLsizei * length, GLchar * infoLog)) \
    _GLM_XMACRO(glDepthFunc,                       void, (GLenum func)) \
    _GLM_XMACRO(glStencilOp,                      void, (GLenum fail, GLenum zfail, GLenum zpass)) \
    _GLM_XMACRO(glStencilFunc,                     void, (GLenum func, GLint ref, GLuint mask)) \
    _GLM_XMACRO(glEnableVertexAttribArray,         void, (GLuint index)) \
    _GLM_XMACRO(glBlendFunc,                       void, (GLenum sfactor, GLenum dfactor)) \
    _GLM_XMACRO(glReadBuffer,                      void, (GLenum src)) \
    _GLM_XMACRO(glTexImage2D,                      void, (GLenum target, GLint level, GLint internalformat, GLsizei width, GLsizei height, GLint border, GLenum format, GLenum type, const void * pixels)) \
    _GLM_XMACRO(glGenVertexArrays,                 void, (GLsizei n, GLuint * arrays)) \
    _GLM_XMACRO(glFrontFace,                       void, (GLenum mode)) \
    _GLM_XMACRO(glCullFace,                        void, (GLenum mode)) \
    _GLM_XMACRO(glPixelStorei,                     void, (GLenum pname, GLint param)) \
    _GLM_XMACRO(glBindSampler,                     void, (GLuint unit, GLuint sampler)) \
    _GLM_XMACRO(glGenSamplers,                     void, (GLsizei n, GLuint* samplers)) \
    _GLM_XMACRO(glSamplerParameteri,               void, (GLuint sampler, GLenum pname, GLint param)) \
    _GLM_XMACRO(glSamplerParameterf,               void, (GLuint sampler, GLenum pname, GLfloat param)) \
    _GLM_XMACRO(glSamplerParameterfv,              void, (GLuint sampler, GLenum pname, const GLfloat* params)) \
    _GLM_XMACRO(glDeleteSamplers,                  void, (GLsizei n, const GLuint* samplers)) \
    _GLM_XMACRO(glBindBufferBase,                  void, (GLenum target, GLuint index, GLuint buffer)) \
    _GLM_XMACRO(glBindBufferRange,                 void, (GLenum target, GLuint index, GLuint buffer, GLintptr offset, GLsizeiptr size)) \
    _GLM_XMACRO(glTexImage2DMultisample,           void, (GLenum target, GLsizei samples, GLenum internalformat, GLsizei width, GLsizei height, GLboolean fixedsamplelocations)) \
    _GLM_XMACRO(glTexImage3DMultisample,           void, (GLenum target, GLsizei samples, GLenum internalformat, GLsizei width, GLsizei height, GLsizei depth, GLboolean fixedsamplelocations)) \
    _GLM_XMACRO(glDispatchCompute,                 void, (GLuint num_groups_x, GLuint num_groups_y, GLuint num_groups_z)) \
    _GLM_XMACRO(glMemoryBarrier,                   void, (GLbitfield barriers)) \
    _GLM_XMACRO(glBindImageTexture,                void, (GLuint unit, GLuint texture, GLint level, GLboolean layered, GLint layer, GLenum access, GLenum format)) \
    _GLM_XMACRO(glTexStorage2DMultisample,         void, (GLenum target, GLsizei samples, GLenum internalformat, GLsizei width, GLsizei height, GLboolean fixedsamplelocations)) \
    _GLM_XMACRO(glTexStorage2D,                    void, (GLenum target, GLsizei levels, GLenum internalformat, GLsizei width, GLsizei height)) \
    _GLM_XMACRO(glTexStorage3DMultisample,         void, (GLenum target, GLsizei samples, GLenum internalformat, GLsizei width, GLsizei height, GLsizei depth, GLboolean fixedsamplelocations)) \
    _GLM_XMACRO(glTexStorage3D,                    void, (GLenum target, GLsizei levels, GLenum internalformat, GLsizei width, GLsizei height, GLsizei depth)) \
    _GLM_XMACRO(glCompressedTexSubImage2D,         void, (GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLsizei imageSize, const void *data)) \
    _GLM_XMACRO(glCompressedTexSubImage3D,         void, (GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLsizei width, GLsizei height, GLsizei depth, GLenum format, GLsizei imageSize, const void *data)) \
    _GLM_XMACRO(glTextureView,                     void, (GLuint texture, GLenum target, GLuint origtexture, GLenum internalformat, GLuint minlevel, GLuint numlevels, GLuint minlayer, GLuint numlayers)) \
    _GLM_XMACRO(glDrawElementsBaseVertex,          void, (GLenum mode, GLsizei count, GLenum type, const void* indices, GLint basevertex)) \
    _GLM_XMACRO(glDrawElementsInstancedBaseVertex, void, (GLenum mode, GLsizei count, GLenum type, const void* indices, GLsizei instancecount, GLint basevertex)) \
    _GLM_XMACRO(glDrawElementsInstancedBaseVertexBaseInstance, void, (GLenum mode, GLsizei count, GLenum type, const void* indices, GLsizei instancecount, GLint basevertex, GLuint baseinstance)) \
    _GLM_XMACRO(glDrawArraysInstancedBaseInstance, void, (GLenum mode, GLint first, GLsizei count, GLsizei instancecount, GLuint baseinstance)) \
    _GLM_XMACRO(glInvalidateFramebuffer,          void, (GLenum target, GLsizei numAttachments, const GLenum* attachments))

// generate GL function prototypes
#ifdef __cplusplus
extern "C" {
#endif

#define _GLM_XMACRO(name, ret, args) extern ret name args;
_GLM_GL_FUNCS
#undef _GLM_XMACRO

#ifdef __cplusplus
} // extern "C"
#endif

#endif // GL_MOCK_GL_H_INCLUDED
