/*
 * This file is part of vitaGL
 * Copyright 2017, 2018, 2019, 2020 Rinnegatamante
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published
 * by the Free Software Foundation, version 3 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef _VITAGL_H_
#define _VITAGL_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <vitashark.h>

// clang-format off
#define GLboolean     uint8_t
#define GLbyte        int8_t
#define GLubyte       uint8_t
#define GLchar        char
#define GLshort       int16_t
#define GLushort      uint16_t
#define GLint         int32_t
#define GLuint        uint32_t
#define GLfixed       int32_t
#define GLint64       int64_t
#define GLuint64      uint64_t
#define GLsizei       int32_t
#define GLenum        uint32_t
#define GLintptr      int32_t
#define GLsizeiptr    uint32_t
#define GLsync        int32_t
#define GLfloat       float
#define GLclampf      float
#define GLdouble      double
#define GLclampd      double
#define GLvoid        void

#define EGLBoolean    uint8_t
#define EGLDisplay    void*
#define EGLSurface    void*
#define EGLint        int32_t

#define GL_FALSE                              0
#define GL_TRUE                               1

#define GL_NO_ERROR                           0

#define GL_ZERO                               0
#define GL_ONE                                1

#define GL_NONE                               0

#define GL_POINTS                                    0x0000
#define GL_LINES                                     0x0001
#define GL_LINE_LOOP                                 0x0002
#define GL_LINE_STRIP                                0x0003
#define GL_TRIANGLES                                 0x0004
#define GL_TRIANGLE_STRIP                            0x0005
#define GL_TRIANGLE_FAN                              0x0006
#define GL_QUADS                                     0x0007
#define GL_ADD                                       0x0104
#define GL_NEVER                                     0x0200
#define GL_NEVER                                     0x0200
#define GL_LESS                                      0x0201
#define GL_EQUAL                                     0x0202
#define GL_LEQUAL                                    0x0203
#define GL_GREATER                                   0x0204
#define GL_NOTEQUAL                                  0x0205
#define GL_GEQUAL                                    0x0206
#define GL_ALWAYS                                    0x0207
#define GL_SRC_COLOR                                 0x0300
#define GL_ONE_MINUS_SRC_COLOR                       0x0301
#define GL_SRC_ALPHA                                 0x0302
#define GL_ONE_MINUS_SRC_ALPHA                       0x0303
#define GL_DST_ALPHA                                 0x0304
#define GL_ONE_MINUS_DST_ALPHA                       0x0305
#define GL_DST_COLOR                                 0x0306
#define GL_ONE_MINUS_DST_COLOR                       0x0307
#define GL_SRC_ALPHA_SATURATE                        0x0308
#define GL_FRONT                                     0x0404
#define GL_BACK                                      0x0405
#define GL_FRONT_AND_BACK                            0x0408
#define GL_INVALID_ENUM                              0x0500
#define GL_INVALID_VALUE                             0x0501
#define GL_INVALID_OPERATION                         0x0502
#define GL_STACK_OVERFLOW                            0x0503
#define GL_STACK_UNDERFLOW                           0x0504
#define GL_OUT_OF_MEMORY                             0x0505
#define GL_EXP                                       0x0800
#define GL_EXP2                                      0x0801
#define GL_CW                                        0x0900
#define GL_CCW                                       0x0901
#define GL_POLYGON_MODE                              0x0B40
#define GL_CULL_FACE                                 0x0B44
#define GL_LIGHTING                                  0x0B50
#define GL_FOG                                       0x0B60
#define GL_FOG_DENSITY                               0x0B62
#define GL_FOG_START                                 0x0B63
#define GL_FOG_END                                   0x0B64
#define GL_FOG_MODE                                  0x0B65
#define GL_FOG_COLOR                                 0x0B66
#define GL_DEPTH_TEST                                0x0B71
#define GL_DEPTH_WRITEMASK                           0x0B72
#define GL_STENCIL_TEST                              0x0B90
#define GL_VIEWPORT                                  0x0BA2
#define GL_MODELVIEW_MATRIX                          0x0BA6
#define GL_PROJECTION_MATRIX                         0x0BA7
#define GL_TEXTURE_MATRIX                            0x0BA8
#define GL_ALPHA_TEST                                0x0BC0
#define GL_BLEND                                     0x0BE2
#define GL_SCISSOR_BOX                               0x0C10
#define GL_SCISSOR_TEST                              0x0C11
#define GL_MAX_CLIP_PLANES                           0x0D32
#define GL_MAX_TEXTURE_SIZE                          0x0D33
#define GL_MAX_MODELVIEW_STACK_DEPTH                 0x0D36
#define GL_MAX_PROJECTION_STACK_DEPTH                0x0D38
#define GL_MAX_TEXTURE_STACK_DEPTH                   0x0D39
#define GL_DEPTH_BITS                                0x0D56
#define GL_STENCIL_BITS                              0x0D57
#define GL_TEXTURE_2D                                0x0DE1
#define GL_DONT_CARE                                 0x1100
#define GL_FASTEST                                   0x1101
#define GL_NICEST                                    0x1102
#define GL_AMBIENT                                   0x1200
#define GL_DIFFUSE                                   0x1201
#define GL_SPECULAR                                  0x1202
#define GL_POSITION                                  0x1203
#define GL_CONSTANT_ATTENUATION                      0x1207
#define GL_LINEAR_ATTENUATION                        0x1208
#define GL_QUADRATIC_ATTENUATION                     0x1209
#define GL_BYTE                                      0x1400
#define GL_UNSIGNED_BYTE                             0x1401
#define GL_SHORT                                     0x1402
#define GL_UNSIGNED_SHORT                            0x1403
#define GL_INT                                       0x1404
#define GL_FLOAT                                     0x1406
#define GL_HALF_FLOAT                                0x140B
#define GL_FIXED                                     0x140C
#define GL_INVERT                                    0x150A
#define GL_EMISSION                                  0x1600
#define GL_AMBIENT_AND_DIFFUSE                       0x1602
#define GL_MODELVIEW                                 0x1700
#define GL_PROJECTION                                0x1701
#define GL_TEXTURE                                   0x1702
#define GL_COLOR_INDEX                               0x1900
#define GL_RED                                       0x1903
#define GL_GREEN                                     0x1904
#define GL_BLUE                                      0x1905
#define GL_ALPHA                                     0x1906
#define GL_RGB                                       0x1907
#define GL_RGBA                                      0x1908
#define GL_LUMINANCE                                 0x1909
#define GL_LUMINANCE_ALPHA                           0x190A
#define GL_POINT                                     0x1B00
#define GL_LINE                                      0x1B01
#define GL_FILL                                      0x1B02
#define GL_KEEP                                      0x1E00
#define GL_REPLACE                                   0x1E01
#define GL_INCR                                      0x1E02
#define GL_DECR                                      0x1E03
#define GL_VENDOR                                    0x1F00
#define GL_RENDERER                                  0x1F01
#define GL_VERSION                                   0x1F02
#define GL_EXTENSIONS                                0x1F03
#define GL_MODULATE                                  0x2100
#define GL_DECAL                                     0x2101
#define GL_TEXTURE_ENV_MODE                          0x2200
#define GL_TEXTURE_ENV_COLOR                         0x2201
#define GL_TEXTURE_ENV                               0x2300
#define GL_NEAREST                                   0x2600
#define GL_LINEAR                                    0x2601
#define GL_NEAREST_MIPMAP_NEAREST                    0x2700
#define GL_LINEAR_MIPMAP_NEAREST                     0x2701
#define GL_NEAREST_MIPMAP_LINEAR                     0x2702
#define GL_LINEAR_MIPMAP_LINEAR                      0x2703
#define GL_TEXTURE_MAG_FILTER                        0x2800
#define GL_TEXTURE_MIN_FILTER                        0x2801
#define GL_TEXTURE_WRAP_S                            0x2802
#define GL_TEXTURE_WRAP_T                            0x2803
#define GL_REPEAT                                    0x2901
#define GL_POLYGON_OFFSET_UNITS                      0x2A00
#define GL_POLYGON_OFFSET_POINT                      0x2A01
#define GL_POLYGON_OFFSET_LINE                       0x2A02
#define GL_V2F                                       0x2A20
#define GL_V3F                                       0x2A21
#define GL_C4UB_V2F                                  0x2A22
#define GL_C4UB_V3F                                  0x2A23
#define GL_C3F_V3F                                   0x2A24
#define GL_T2F_V3F                                   0x2A27
#define GL_T4F_V4F                                   0x2A28
#define GL_T2F_C4UB_V3F                              0x2A29
#define GL_T2F_C3F_V3F                               0x2A2A
#define GL_CLIP_PLANE0                               0x3000
#define GL_CLIP_PLANE1                               0x3001
#define GL_CLIP_PLANE2                               0x3002
#define GL_CLIP_PLANE3                               0x3003
#define GL_CLIP_PLANE4                               0x3004
#define GL_CLIP_PLANE5                               0x3005
#define GL_CLIP_PLANE6                               0x3006
#define GL_LIGHT0                                    0x4000
#define GL_LIGHT1                                    0x4001
#define GL_LIGHT2                                    0x4002
#define GL_LIGHT3                                    0x4003
#define GL_LIGHT4                                    0x4004
#define GL_LIGHT5                                    0x4005
#define GL_LIGHT6                                    0x4006
#define GL_LIGHT7                                    0x4007
#define GL_FUNC_ADD                                  0x8006
#define GL_MIN                                       0x8007
#define GL_MAX                                       0x8008
#define GL_FUNC_SUBTRACT                             0x800A
#define GL_FUNC_REVERSE_SUBTRACT                     0x800B
#define GL_UNSIGNED_SHORT_4_4_4_4                    0x8033
#define GL_UNSIGNED_SHORT_5_5_5_1                    0x8034
#define GL_POLYGON_OFFSET_FILL                       0x8037
#define GL_POLYGON_OFFSET_FACTOR                     0x8038
#define GL_INTENSITY                                 0x8049
#define GL_TEXTURE_BINDING_2D                        0x8069
#define GL_VERTEX_ARRAY                              0x8074
#define GL_COLOR_ARRAY                               0x8076
#define GL_TEXTURE_COORD_ARRAY                       0x8078
#define GL_BLEND_DST_RGB                             0x80C8
#define GL_BLEND_SRC_RGB                             0x80C9
#define GL_BLEND_DST_ALPHA                           0x80CA
#define GL_BLEND_SRC_ALPHA                           0x80CB
#define GL_COLOR_TABLE                               0x80D0
#define GL_BGR                                       0x80E0
#define GL_BGRA                                      0x80E1
#define GL_COLOR_INDEX8_EXT                          0x80E5
#define GL_CLAMP_TO_EDGE                             0x812F
#define GL_MAJOR_VERSION                             0x821B
#define GL_MINOR_VERSION                             0x821C
#define GL_NUM_EXTENSIONS                            0x821D
#define GL_RG                                        0x8227
#define GL_UNSIGNED_SHORT_5_6_5                      0x8363
#define GL_MIRRORED_REPEAT                           0x8370
#define GL_COMPRESSED_RGB_S3TC_DXT1_EXT              0x83F0
#define GL_COMPRESSED_RGBA_S3TC_DXT1_EXT             0x83F1
#define GL_COMPRESSED_RGBA_S3TC_DXT5_EXT             0x83F3
#define GL_TEXTURE0                                  0x84C0
#define GL_TEXTURE1                                  0x84C1
#define GL_TEXTURE2                                  0x84C2
#define GL_TEXTURE3                                  0x84C3
#define GL_TEXTURE4                                  0x84C4
#define GL_TEXTURE5                                  0x84C5
#define GL_TEXTURE6                                  0x84C6
#define GL_TEXTURE7                                  0x84C7
#define GL_TEXTURE8                                  0x84C8
#define GL_TEXTURE9                                  0x84C9
#define GL_TEXTURE10                                 0x84CA
#define GL_TEXTURE11                                 0x84CB
#define GL_TEXTURE12                                 0x84CC
#define GL_TEXTURE13                                 0x84CD
#define GL_TEXTURE14                                 0x84CE
#define GL_TEXTURE15                                 0x84CF
#define GL_ACTIVE_TEXTURE                            0x84E0
#define GL_TEXTURE_COMPRESSION_HINT                  0x84EF
#define GL_TEXTURE_LOD_BIAS                          0x8501
#define GL_INCR_WRAP                                 0x8507
#define GL_DECR_WRAP                                 0x8508
#define GL_VERTEX_ATTRIB_ARRAY_ENABLED               0x8622
#define GL_VERTEX_ATTRIB_ARRAY_SIZE                  0x8623
#define GL_VERTEX_ATTRIB_ARRAY_STRIDE                0x8624
#define GL_VERTEX_ATTRIB_ARRAY_TYPE                  0x8625
#define GL_CURRENT_VERTEX_ATTRIB                     0x8626
#define GL_VERTEX_ATTRIB_ARRAY_POINTER               0x8645
#define GL_NUM_COMPRESSED_TEXTURE_FORMATS            0x86A2
#define GL_COMPRESSED_TEXTURE_FORMATS                0x86A3
#define GL_MIRROR_CLAMP_EXT                          0x8742
#define GL_VERTEX_ATTRIB_ARRAY_NORMALIZED            0x886A
#define GL_ARRAY_BUFFER                              0x8892
#define GL_ELEMENT_ARRAY_BUFFER                      0x8893
#define GL_VERTEX_ATTRIB_ARRAY_BUFFER_BINDING        0x889F
#define GL_STREAM_DRAW                               0x88E0
#define GL_STREAM_READ                               0x88E1
#define GL_STREAM_COPY                               0x88E2
#define GL_STATIC_DRAW                               0x88E4
#define GL_STATIC_READ                               0x88E5
#define GL_STATIC_COPY                               0x88E6
#define GL_DYNAMIC_DRAW                              0x88E8
#define GL_DYNAMIC_READ                              0x88E9
#define GL_DYNAMIC_COPY                              0x88EA
#define GL_FRAGMENT_SHADER                           0x8B30
#define GL_VERTEX_SHADER                             0x8B31
#define GL_SHADER_TYPE                               0x8B4F
#define GL_FLOAT_VEC2                                0x8B50
#define GL_FLOAT_VEC3                                0x8B51
#define GL_FLOAT_VEC4                                0x8B52
#define GL_INT_VEC2                                  0x8B53
#define GL_INT_VEC3                                  0x8B54
#define GL_INT_VEC4                                  0x8B55
#define GL_FLOAT_MAT2                                0x8B5A
#define GL_FLOAT_MAT3                                0x8B5B
#define GL_FLOAT_MAT4                                0x8B5C
#define GL_COMPILE_STATUS                            0x8B81
#define GL_LINK_STATUS                               0x8B82
#define GL_INFO_LOG_LENGTH                           0x8B84
#define GL_ATTACHED_SHADERS                          0x8B85
#define GL_ACTIVE_UNIFORMS                           0x8B86
#define GL_ACTIVE_UNIFORM_MAX_LENGTH                 0x8B87
#define GL_ACTIVE_ATTRIBUTES                         0x8B89
#define GL_ACTIVE_ATTRIBUTE_MAX_LENGTH               0x8B8A
#define GL_SHADING_LANGUAGE_VERSION                  0x8B8C
#define GL_COMPRESSED_RGB_PVRTC_4BPPV1_IMG           0x8C00
#define GL_COMPRESSED_RGB_PVRTC_2BPPV1_IMG           0x8C01
#define GL_COMPRESSED_RGBA_PVRTC_4BPPV1_IMG          0x8C02
#define GL_COMPRESSED_RGBA_PVRTC_2BPPV1_IMG          0x8C03
#define GL_SRGB                                      0x8C40
#define GL_SRGB8                                     0x8C41
#define GL_SRGB_ALPHA                                0x8C42
#define GL_SRGB8_ALPHA8                              0x8C43
#define GL_SLUMINANCE_ALPHA                          0x8C44
#define GL_SLUMINANCE8_ALPHA8                        0x8C45
#define GL_SLUMINANCE                                0x8C46
#define GL_SLUMINANCE8                               0x8C47
#define GL_COMPRESSED_SRGB                           0x8C48
#define GL_COMPRESSED_SRGB_ALPHA                     0x8C49
#define GL_COMPRESSED_SRGB_S3TC_DXT1                 0x8C4C
#define GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT1           0x8C4D
#define GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT3           0x8C4E
#define GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT5           0x8C4F
#define GL_FRAMEBUFFER_BINDING                       0x8CA6
#define GL_READ_FRAMEBUFFER                          0x8CA8
#define GL_DRAW_FRAMEBUFFER                          0x8CA9
#define GL_READ_FRAMEBUFFER_BINDING                  0x8CAA
#define GL_COLOR_ATTACHMENT0                         0x8CE0
#define GL_FRAMEBUFFER_COMPLETE                      0x8CD5
#define GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT 0x8CD7
#define GL_FRAMEBUFFER                               0x8D40
#define GL_MAX_VERTEX_UNIFORM_VECTORS                0x8DFB
#define GL_COMPRESSED_RGBA_PVRTC_2BPPV2_IMG          0x9137
#define GL_COMPRESSED_RGBA_PVRTC_4BPPV2_IMG          0x9138

#define GL_MAX_COMBINED_TEXTURE_IMAGE_UNITS   16
#define GL_MAX_TEXTURE_LOD_BIAS               31
#define GL_MAX_VERTEX_ATTRIBS                 16
#define GL_MAX_TEXTURE_IMAGE_UNITS            3

// Aliases
#define GL_CLAMP GL_CLAMP_TO_EDGE
#define GL_DRAW_FRAMEBUFFER_BINDING GL_FRAMEBUFFER_BINDING

typedef enum GLbitfield{
	GL_DEPTH_BUFFER_BIT   = 0x00000100,
	GL_STENCIL_BUFFER_BIT = 0x00000400,
	GL_COLOR_BUFFER_BIT   = 0x00004000
} GLbitfield;
// clang-format on

// gl*
void glActiveTexture(GLenum texture);
void glAlphaFunc(GLenum func, GLfloat ref);
void glAttachShader(GLuint prog, GLuint shad);
void glBegin(GLenum mode);
void glBindAttribLocation(GLuint program, GLuint index, const GLchar *name);
void glBindBuffer(GLenum target, GLuint buffer);
void glBindFramebuffer(GLenum target, GLuint framebuffer);
void glBindTexture(GLenum target, GLuint texture);
void glBlendEquation(GLenum mode);
void glBlendEquationSeparate(GLenum modeRGB, GLenum modeAlpha);
void glBlendFunc(GLenum sfactor, GLenum dfactor);
void glBlendFuncSeparate(GLenum srcRGB, GLenum dstRGB, GLenum srcAlpha, GLenum dstAlpha);
void glBufferData(GLenum target, GLsizei size, const GLvoid *data, GLenum usage);
void glBufferSubData(GLenum target, GLintptr offset, GLsizeiptr size, const void *data);
GLenum glCheckFramebufferStatus(GLenum target);
void glClear(GLbitfield mask);
void glClearColor(GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha);
void glClearDepth(GLdouble depth);
void glClearDepthf(GLclampf depth);
void glClearStencil(GLint s);
void glClientActiveTexture(GLenum texture);
void glClipPlane(GLenum plane, const GLdouble *equation);
void glColor3f(GLfloat red, GLfloat green, GLfloat blue);
void glColor3fv(const GLfloat *v);
void glColor3ub(GLubyte red, GLubyte green, GLubyte blue);
void glColor3ubv(const GLubyte *v);
void glColor4f(GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha);
void glColor4fv(const GLfloat *v);
void glColor4ub(GLubyte red, GLubyte green, GLubyte blue, GLubyte alpha);
void glColor4ubv(const GLubyte *v);
void glColor4x(GLfixed red, GLfixed green, GLfixed blue, GLfixed alpha);
void glColorMask(GLboolean red, GLboolean green, GLboolean blue, GLboolean alpha);
void glColorPointer(GLint size, GLenum type, GLsizei stride, const GLvoid *pointer);
void glColorTable(GLenum target, GLenum internalformat, GLsizei width, GLenum format, GLenum type, const GLvoid *data);
void glCompileShader(GLuint shader);
void glCompressedTexImage2D(GLenum target, GLint level, GLenum internalformat, GLsizei width, GLsizei height, GLint border, GLsizei imageSize, const void *data); // Mipmap levels are ignored currently
GLuint glCreateProgram(void);
GLuint glCreateShader(GLenum shaderType);
void glCullFace(GLenum mode);
void glDeleteBuffers(GLsizei n, const GLuint *gl_buffers);
void glDeleteFramebuffers(GLsizei n, const GLuint *ids);
void glDeleteProgram(GLuint prog);
void glDeleteShader(GLuint shad);
void glDeleteTextures(GLsizei n, const GLuint *textures);
void glDepthFunc(GLenum func);
void glDepthMask(GLboolean flag);
void glDepthRange(GLdouble nearVal, GLdouble farVal);
void glDepthRangef(GLfloat nearVal, GLfloat farVal);
void glDisable(GLenum cap);
void glDisableClientState(GLenum array);
void glDisableVertexAttribArray(GLuint index);
void glDrawArrays(GLenum mode, GLint first, GLsizei count);
void glDrawElements(GLenum mode, GLsizei count, GLenum type, const GLvoid *indices);
void glEnable(GLenum cap);
void glEnableClientState(GLenum array);
void glEnableVertexAttribArray(GLuint index);
void glEnd(void);
void glFinish(void);
void glFlush(void);
void glFogf(GLenum pname, GLfloat param);
void glFogfv(GLenum pname, const GLfloat *params);
void glFogi(GLenum pname, const GLint param);
void glFramebufferTexture(GLenum target, GLenum attachment, GLuint texture, GLint level);
void glFramebufferTexture2D(GLenum target, GLenum attachment, GLenum textarget, GLuint texture, GLint level);
void glFrontFace(GLenum mode);
void glFrustum(GLdouble left, GLdouble right, GLdouble bottom, GLdouble top, GLdouble nearVal, GLdouble farVal);
void glGenBuffers(GLsizei n, GLuint *buffers);
void glGenerateMipmap(GLenum target);
void glGenFramebuffers(GLsizei n, GLuint *ids);
void glGenTextures(GLsizei n, GLuint *textures);
void glGetActiveAttrib(GLuint prog, GLuint index, GLsizei bufSize, GLsizei *length, GLint *size, GLenum *type, GLchar *name);
void glGetActiveUniform(GLuint prog, GLuint index, GLsizei bufSize, GLsizei *length, GLint *size, GLenum *type, GLchar *name);
GLint glGetAttribLocation(GLuint prog, const GLchar *name);
void glGetBooleanv(GLenum pname, GLboolean *params);
void glGetFloatv(GLenum pname, GLfloat *data);
GLenum glGetError(void);
void glGetIntegerv(GLenum pname, GLint *data);
void glGetProgramInfoLog(GLuint program, GLsizei maxLength, GLsizei *length, GLchar *infoLog);
void glGetProgramiv(GLuint program, GLenum pname, GLint *params);
void glGetShaderInfoLog(GLuint handle, GLsizei maxLength, GLsizei *length, GLchar *infoLog);
void glGetShaderiv(GLuint handle, GLenum pname, GLint *params);
const GLubyte *glGetString(GLenum name);
const GLubyte *glGetStringi(GLenum name, GLuint index);
GLint glGetUniformLocation(GLuint prog, const GLchar *name);
void glGetVertexAttribfv(GLuint index, GLenum pname, GLfloat *params);
void glGetVertexAttribiv(GLuint index, GLenum pname, GLint *params);
void glGetVertexAttribPointerv(GLuint index, GLenum pname, void **pointer);
void glHint(GLenum target, GLenum mode);
void glInterleavedArrays(GLenum format, GLsizei stride, const void *pointer);
GLboolean glIsEnabled(GLenum cap);
GLboolean glIsFramebuffer(GLuint fb);
GLboolean glIsTexture(GLuint texture);
void glLightfv(GLenum light, GLenum pname, const GLfloat *params);
void glLineWidth(GLfloat width);
void glLinkProgram(GLuint progr);
void glLoadIdentity(void);
void glLoadMatrixf(const GLfloat *m);
void glMaterialfv(GLenum face, GLenum pname, const GLfloat *params);
void glMatrixMode(GLenum mode);
void glMultMatrixf(const GLfloat *m);
void glNormal3f(GLfloat x, GLfloat y, GLfloat z);
void glNormal3fv(const GLfloat *v);
void glOrtho(GLdouble left, GLdouble right, GLdouble bottom, GLdouble top, GLdouble nearVal, GLdouble farVal);
void glPointSize(GLfloat size);
void glPolygonMode(GLenum face, GLenum mode);
void glPolygonOffset(GLfloat factor, GLfloat units);
void glPopMatrix(void);
void glPushMatrix(void);
void glReadPixels(GLint x, GLint y, GLsizei width, GLsizei height, GLenum format, GLenum type, GLvoid *data);
void glReleaseShaderCompiler(void);
void glRotatef(GLfloat angle, GLfloat x, GLfloat y, GLfloat z);
void glScalef(GLfloat x, GLfloat y, GLfloat z);
void glScissor(GLint x, GLint y, GLsizei width, GLsizei height);
void glShaderBinary(GLsizei count, const GLuint *handles, GLenum binaryFormat, const void *binary, GLsizei length); // NOTE: Uses GXP shaders
void glShaderSource(GLuint handle, GLsizei count, const GLchar *const *string, const GLint *length); // NOTE: Uses CG shader sources
void glStencilFunc(GLenum func, GLint ref, GLuint mask);
void glStencilFuncSeparate(GLenum face, GLenum func, GLint ref, GLuint mask);
void glStencilMask(GLuint mask);
void glStencilMaskSeparate(GLenum face, GLuint mask);
void glStencilOp(GLenum sfail, GLenum dpfail, GLenum dppass);
void glStencilOpSeparate(GLenum face, GLenum sfail, GLenum dpfail, GLenum dppass);
void glTexCoord2f(GLfloat s, GLfloat t);
void glTexCoord2fv(GLfloat *f);
void glTexCoord2i(GLint s, GLint t);
void glTexCoordPointer(GLint size, GLenum type, GLsizei stride, const GLvoid *pointer);
void glTexEnvf(GLenum target, GLenum pname, GLfloat param);
void glTexEnvfv(GLenum target, GLenum pname, GLfloat *param);
void glTexEnvi(GLenum target, GLenum pname, GLint param);
void glTexImage2D(GLenum target, GLint level, GLint internalFormat, GLsizei width, GLsizei height, GLint border, GLenum format, GLenum type, const GLvoid *data);
void glTexParameterf(GLenum target, GLenum pname, GLfloat param);
void glTexParameteri(GLenum target, GLenum pname, GLint param);
void glTexSubImage2D(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLenum type, const GLvoid *pixels);
void glTranslatef(GLfloat x, GLfloat y, GLfloat z);
void glUniform1f(GLint location, GLfloat v0);
void glUniform1fv(GLint location, GLsizei count, const GLfloat *value);
void glUniform1i(GLint location, GLint v0);
void glUniform1iv(GLint location, GLsizei count, const GLint *value);
void glUniform2f(GLint location, GLfloat v0, GLfloat v1);
void glUniform2fv(GLint location, GLsizei count, const GLfloat *value);
void glUniform2i(GLint location, GLint v0, GLint v1);
void glUniform2iv(GLint location, GLsizei count, const GLint *value);
void glUniform3f(GLint location, GLfloat v0, GLfloat v1, GLfloat v2);
void glUniform3fv(GLint location, GLsizei count, const GLfloat *value);
void glUniform3i(GLint location, GLint v0, GLint v1, GLint v2);
void glUniform3iv(GLint location, GLsizei count, const GLint *value);
void glUniform4f(GLint location, GLfloat v0, GLfloat v1, GLfloat v2, GLfloat v3);
void glUniform4fv(GLint location, GLsizei count, const GLfloat *value);
void glUniform4i(GLint location, GLint v0, GLint v1, GLint v2, GLint v3);
void glUniform4iv(GLint location, GLsizei count, const GLint *value);
void glUniformMatrix2fv(GLint location, GLsizei count, GLboolean transpose, const GLfloat *value);
void glUniformMatrix3fv(GLint location, GLsizei count, GLboolean transpose, const GLfloat *value);
void glUniformMatrix4fv(GLint location, GLsizei count, GLboolean transpose, const GLfloat *value);
void glUseProgram(GLuint program);
void glVertex2f(GLfloat x, GLfloat y);
void glVertex3f(GLfloat x, GLfloat y, GLfloat z);
void glVertex3fv(const GLfloat *v);
void glVertexAttrib1f(GLuint index, GLfloat v0);
void glVertexAttrib1fv(GLuint index, const GLfloat *v);
void glVertexAttrib2f(GLuint index, GLfloat v0, GLfloat v1);
void glVertexAttrib2fv(GLuint index, const GLfloat *v);
void glVertexAttrib3f(GLuint index, GLfloat v0, GLfloat v1, GLfloat v2);
void glVertexAttrib3fv(GLuint index, const GLfloat *v);
void glVertexAttrib4f(GLuint index, GLfloat v0, GLfloat v1, GLfloat v2, GLfloat v3);
void glVertexAttrib4fv(GLuint index, const GLfloat *v);
void glVertexAttribPointer(GLuint index, GLint size, GLenum type, GLboolean normalized, GLsizei stride, const void *pointer);
void glVertexPointer(GLint size, GLenum type, GLsizei stride, const GLvoid *pointer);
void glViewport(GLint x, GLint y, GLsizei width, GLsizei height);

// glu*
void gluPerspective(GLdouble fovy, GLdouble aspect, GLdouble zNear, GLdouble zFar);

// egl*
EGLBoolean eglSwapInterval(EGLDisplay display, EGLint interval);
EGLBoolean eglSwapBuffers(EGLDisplay display, EGLSurface surface);

// VGL_EXT_gpu_objects_array extension
void vglColorPointer(GLint size, GLenum type, GLsizei stride, GLuint count, const GLvoid *pointer);
void vglColorPointerMapped(GLenum type, const GLvoid *pointer);
void vglDrawObjects(GLenum mode, GLsizei count, GLboolean implicit_wvp);
void vglIndexPointer(GLenum type, GLsizei stride, GLuint count, const GLvoid *pointer);
void vglIndexPointerMapped(const GLvoid *pointer);
void vglTexCoordPointer(GLint size, GLenum type, GLsizei stride, GLuint count, const GLvoid *pointer);
void vglTexCoordPointerMapped(const GLvoid *pointer);
void vglVertexPointer(GLint size, GLenum type, GLsizei stride, GLuint count, const GLvoid *pointer);
void vglVertexPointerMapped(const GLvoid *pointer);

// VGL_EXT_gxp_shaders extension implementation
void vglBindAttribLocation(GLuint prog, GLuint index, const GLchar *name, const GLuint num, const GLenum type);
GLint vglBindPackedAttribLocation(GLuint prog, const GLchar *name, const GLuint num, const GLenum type, GLuint offset, GLint stride);
void vglVertexAttribPointer(GLuint index, GLint size, GLenum type, GLboolean normalized, GLsizei stride, GLuint count, const GLvoid *pointer);
void vglVertexAttribPointerMapped(GLuint index, const GLvoid *pointer);

typedef enum {
	VGL_MEM_VRAM, // CDRAM
	VGL_MEM_RAM, // USER_RW RAM
	VGL_MEM_SLOW, // PHYCONT_USER_RW RAM
	VGL_MEM_EXTERNAL, // newlib mem
	VGL_MEM_ALL
} vglMemType;

// vgl*
void *vglAlloc(uint32_t size, vglMemType type);
void vglEnableRuntimeShaderCompiler(GLboolean usage);
void vglEnd(void);
void *vglForceAlloc(uint32_t size);
void vglFree(void *addr);
SceGxmTexture *vglGetGxmTexture(GLenum target);
void *vglGetProcAddress(const char *name);
void *vglGetTexDataPointer(GLenum target);
GLboolean vglHasRuntimeShaderCompiler(void);
void vglInit(int legacy_pool_size);
void vglInitExtended(int legacy_pool_size, int width, int height, int ram_threshold, SceGxmMultisampleMode msaa);
void vglInitWithCustomSizes(int legacy_pool_size, int width, int height, int ram_pool_size, int cdram_pool_size, int phycont_pool_size, SceGxmMultisampleMode msaa);
void vglInitWithCustomThreshold(int pool_size, int width, int height, int ram_threshold, int cdram_threshold, int phycont_threshold, SceGxmMultisampleMode msaa);
size_t vglMemFree(vglMemType type);
void vglSetFragmentBufferSize(uint32_t size);
void vglSetParamBufferSize(uint32_t size);
void vglSetUSSEBufferSize(uint32_t size);
void vglSetVDMBufferSize(uint32_t size);
void vglSetVertexBufferSize(uint32_t size);
void vglSetVertexPoolSize(uint32_t size);
void vglSetupRuntimeShaderCompiler(shark_opt opt_level, int32_t use_fastmath, int32_t use_fastprecision, int32_t use_fastint);
void vglSwapBuffers(GLboolean has_commondialog);
void vglTexImageDepthBuffer(GLenum target);
void vglUseTripleBuffering(GLboolean usage);
void vglUseVram(GLboolean usage);
void vglUseVramForUSSE(GLboolean usage);
void vglUseExtraMem(GLboolean usage);
void vglWaitVblankStart(GLboolean enable);

#ifdef __cplusplus
}
#endif

#endif
