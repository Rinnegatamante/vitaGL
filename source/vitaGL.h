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

#include <vitasdk.h>
#ifdef HAVE_SHARK
#include <vitashark.h>
#else
#define shark_opt int32_t
#endif

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

#define GL_POINTS                                    0X0000
#define GL_LINES                                     0X0001
#define GL_LINE_LOOP                                 0X0002
#define GL_LINE_STRIP                                0X0003
#define GL_TRIANGLES                                 0X0004
#define GL_TRIANGLE_STRIP                            0X0005
#define GL_TRIANGLE_FAN                              0X0006
#define GL_QUADS                                     0X0007
#define GL_ADD                                       0X0104
#define GL_NEVER                                     0X0200
#define GL_NEVER                                     0X0200
#define GL_LESS                                      0X0201
#define GL_EQUAL                                     0X0202
#define GL_LEQUAL                                    0X0203
#define GL_GREATER                                   0X0204
#define GL_NOTEQUAL                                  0X0205
#define GL_GEQUAL                                    0X0206
#define GL_ALWAYS                                    0X0207
#define GL_SRC_COLOR                                 0X0300
#define GL_ONE_MINUS_SRC_COLOR                       0X0301
#define GL_SRC_ALPHA                                 0X0302
#define GL_ONE_MINUS_SRC_ALPHA                       0X0303
#define GL_DST_ALPHA                                 0X0304
#define GL_ONE_MINUS_DST_ALPHA                       0X0305
#define GL_DST_COLOR                                 0X0306
#define GL_ONE_MINUS_DST_COLOR                       0X0307
#define GL_SRC_ALPHA_SATURATE                        0X0308
#define GL_FRONT                                     0X0404
#define GL_BACK                                      0X0405
#define GL_FRONT_AND_BACK                            0X0408
#define GL_INVALID_ENUM                              0X0500
#define GL_INVALID_VALUE                             0X0501
#define GL_INVALID_OPERATION                         0X0502
#define GL_STACK_OVERFLOW                            0X0503
#define GL_STACK_UNDERFLOW                           0X0504
#define GL_OUT_OF_MEMORY                             0X0505
#define GL_EXP                                       0X0800
#define GL_EXP2                                      0X0801
#define GL_CW                                        0X0900
#define GL_CCW                                       0X0901
#define GL_POLYGON_MODE                              0X0B40
#define GL_CULL_FACE                                 0X0B44
#define GL_FOG                                       0X0B60
#define GL_FOG_DENSITY                               0X0B62
#define GL_FOG_START                                 0X0B63
#define GL_FOG_END                                   0X0B64
#define GL_FOG_MODE                                  0X0B65
#define GL_FOG_COLOR                                 0X0B66
#define GL_DEPTH_TEST                                0X0B71
#define GL_STENCIL_TEST                              0X0B90
#define GL_VIEWPORT                                  0X0BA2
#define GL_MODELVIEW_MATRIX                          0X0BA6
#define GL_PROJECTION_MATRIX                         0X0BA7
#define GL_TEXTURE_MATRIX                            0X0BA8
#define GL_ALPHA_TEST                                0X0BC0
#define GL_BLEND                                     0X0BE2
#define GL_SCISSOR_BOX                               0X0C10
#define GL_SCISSOR_TEST                              0X0C11
#define GL_MAX_TEXTURE_SIZE                          0X0D33
#define GL_MAX_MODELVIEW_STACK_DEPTH                 0X0D36
#define GL_MAX_PROJECTION_STACK_DEPTH                0X0D38
#define GL_MAX_TEXTURE_STACK_DEPTH                   0X0D39
#define GL_DEPTH_BITS                                0X0D56
#define GL_STENCIL_BITS                              0X0D57
#define GL_TEXTURE_2D                                0X0DE1
#define GL_DONT_CARE                                 0X1100
#define GL_FASTEST                                   0X1101
#define GL_NICEST                                    0X1102
#define GL_BYTE                                      0X1400
#define GL_UNSIGNED_BYTE                             0X1401
#define GL_SHORT                                     0X1402
#define GL_UNSIGNED_SHORT                            0X1403
#define GL_FLOAT                                     0X1406
#define GL_HALF_FLOAT                                0x140B
#define GL_FIXED                                     0X140C
#define GL_INVERT                                    0X150A
#define GL_MODELVIEW                                 0X1700
#define GL_PROJECTION                                0X1701
#define GL_TEXTURE                                   0X1702
#define GL_COLOR_INDEX                               0X1900
#define GL_RED                                       0X1903
#define GL_GREEN                                     0X1904
#define GL_BLUE                                      0X1905
#define GL_ALPHA                                     0X1906
#define GL_RGB                                       0X1907
#define GL_RGBA                                      0X1908
#define GL_LUMINANCE                                 0X1909
#define GL_LUMINANCE_ALPHA                           0X190A
#define GL_POINT                                     0X1B00
#define GL_LINE                                      0X1B01
#define GL_FILL                                      0X1B02
#define GL_KEEP                                      0X1E00
#define GL_REPLACE                                   0X1E01
#define GL_INCR                                      0X1E02
#define GL_DECR                                      0X1E03
#define GL_VENDOR                                    0X1F00
#define GL_RENDERER                                  0X1F01
#define GL_VERSION                                   0X1F02
#define GL_EXTENSIONS                                0X1F03
#define GL_MODULATE                                  0X2100
#define GL_DECAL                                     0X2101
#define GL_TEXTURE_ENV_MODE                          0X2200
#define GL_TEXTURE_ENV_COLOR                         0X2201
#define GL_TEXTURE_ENV                               0X2300
#define GL_NEAREST                                   0X2600
#define GL_LINEAR                                    0X2601
#define GL_NEAREST_MIPMAP_NEAREST                    0X2700
#define GL_LINEAR_MIPMAP_NEAREST                     0X2701
#define GL_NEAREST_MIPMAP_LINEAR                     0X2702
#define GL_LINEAR_MIPMAP_LINEAR                      0X2703
#define GL_TEXTURE_MAG_FILTER                        0X2800
#define GL_TEXTURE_MIN_FILTER                        0X2801
#define GL_TEXTURE_WRAP_S                            0X2802
#define GL_TEXTURE_WRAP_T                            0X2803
#define GL_REPEAT                                    0X2901
#define GL_POLYGON_OFFSET_UNITS                      0X2A00
#define GL_POLYGON_OFFSET_POINT                      0X2A01
#define GL_POLYGON_OFFSET_LINE                       0X2A02
#define GL_CLIP_PLANE0                               0X3000
#define GL_FUNC_ADD                                  0X8006
#define GL_MIN                                       0X8007
#define GL_MAX                                       0X8008
#define GL_FUNC_SUBTRACT                             0X800A
#define GL_FUNC_REVERSE_SUBTRACT                     0X800B
#define GL_UNSIGNED_SHORT_4_4_4_4                    0X8033
#define GL_UNSIGNED_SHORT_5_5_5_1                    0X8034
#define GL_POLYGON_OFFSET_FILL                       0X8037
#define GL_POLYGON_OFFSET_FACTOR                     0X8038
#define GL_INTENSITY                                 0X8049
#define GL_TEXTURE_BINDING_2D                        0X8069
#define GL_VERTEX_ARRAY                              0X8074
#define GL_COLOR_ARRAY                               0X8076
#define GL_TEXTURE_COORD_ARRAY                       0X8078
#define GL_BLEND_DST_RGB                             0X80C8
#define GL_BLEND_SRC_RGB                             0X80C9
#define GL_BLEND_DST_ALPHA                           0X80CA
#define GL_BLEND_SRC_ALPHA                           0X80CB
#define GL_COLOR_TABLE                               0X80D0
#define GL_BGR                                       0X80E0
#define GL_BGRA                                      0X80E1
#define GL_COLOR_INDEX8_EXT                          0X80E5
#define GL_CLAMP_TO_EDGE                             0X812F
#define GL_RG                                        0X8227
#define GL_UNSIGNED_SHORT_5_6_5                      0X8363
#define GL_MIRRORED_REPEAT                           0X8370
#define GL_COMPRESSED_RGB_S3TC_DXT1_EXT              0X83F0
#define GL_COMPRESSED_RGBA_S3TC_DXT1_EXT             0X83F1
#define GL_COMPRESSED_RGBA_S3TC_DXT5_EXT             0X83F3
#define GL_TEXTURE0                                  0X84C0
#define GL_TEXTURE1                                  0X84C1
#define GL_TEXTURE2                                  0X84C2
#define GL_TEXTURE3                                  0X84C3
#define GL_TEXTURE4                                  0X84C4
#define GL_TEXTURE5                                  0X84C5
#define GL_TEXTURE6                                  0X84C6
#define GL_TEXTURE7                                  0X84C7
#define GL_TEXTURE8                                  0X84C8
#define GL_TEXTURE9                                  0X84C9
#define GL_TEXTURE10                                 0X84CA
#define GL_TEXTURE11                                 0X84CB
#define GL_TEXTURE12                                 0X84CC
#define GL_TEXTURE13                                 0X84CD
#define GL_TEXTURE14                                 0X84CE
#define GL_TEXTURE15                                 0X84CF
#define GL_TEXTURE16                                 0X84D0
#define GL_TEXTURE17                                 0X84D1
#define GL_TEXTURE18                                 0X84D2
#define GL_TEXTURE19                                 0X84D3
#define GL_TEXTURE20                                 0X84D4
#define GL_TEXTURE21                                 0X84D5
#define GL_TEXTURE22                                 0X84D6
#define GL_TEXTURE23                                 0X84D7
#define GL_TEXTURE24                                 0X84D8
#define GL_TEXTURE25                                 0X84D9
#define GL_TEXTURE26                                 0X84DA
#define GL_TEXTURE27                                 0X84DB
#define GL_TEXTURE28                                 0X84DC
#define GL_TEXTURE29                                 0X84DD
#define GL_TEXTURE30                                 0X84DE
#define GL_TEXTURE31                                 0X84DF
#define GL_ACTIVE_TEXTURE                            0X84E0
#define GL_TEXTURE_COMPRESSION_HINT                  0X84EF
#define GL_TEXTURE_LOD_BIAS                          0X8501
#define GL_INCR_WRAP                                 0X8507
#define GL_NUM_COMPRESSED_TEXTURE_FORMATS            0X86A2
#define GL_COMPRESSED_TEXTURE_FORMATS                0X86A3
#define GL_MIRROR_CLAMP_EXT                          0X8742
#define GL_DECR_WRAP                                 0X8508
#define GL_ARRAY_BUFFER                              0X8892
#define GL_ELEMENT_ARRAY_BUFFER                      0X8893
#define GL_STREAM_DRAW                               0X88E0
#define GL_STREAM_READ                               0X88E1
#define GL_STREAM_COPY                               0X88E2
#define GL_STATIC_DRAW                               0X88E4
#define GL_STATIC_READ                               0X88E5
#define GL_STATIC_COPY                               0X88E6
#define GL_DYNAMIC_DRAW                              0X88E8
#define GL_DYNAMIC_READ                              0X88E9
#define GL_DYNAMIC_COPY                              0X88EA
#define GL_FRAGMENT_SHADER                           0X8B30
#define GL_VERTEX_SHADER                             0X8B31
#define GL_SHADER_TYPE                               0X8B4F
#define GL_COMPILE_STATUS                            0X8B81
#define GL_INFO_LOG_LENGTH                           0X8B84
#define GL_COMPRESSED_RGB_PVRTC_4BPPV1_IMG           0X8C00
#define GL_COMPRESSED_RGB_PVRTC_2BPPV1_IMG           0X8C01
#define GL_COMPRESSED_RGBA_PVRTC_4BPPV1_IMG          0X8C02
#define GL_COMPRESSED_RGBA_PVRTC_2BPPV1_IMG          0X8C03
#define GL_FRAMEBUFFER_BINDING                       0x8CA6
#define GL_READ_FRAMEBUFFER                          0X8CA8
#define GL_DRAW_FRAMEBUFFER                          0X8CA9
#define GL_READ_FRAMEBUFFER_BINDING                  0x8CAA
#define GL_COLOR_ATTACHMENT0                         0x8CE0
#define GL_FRAMEBUFFER_COMPLETE                      0x8CD5
#define GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT 0x8CD7
#define GL_FRAMEBUFFER                               0x8D40
#define GL_COMPRESSED_RGBA_PVRTC_2BPPV2_IMG          0x9137
#define GL_COMPRESSED_RGBA_PVRTC_4BPPV2_IMG          0x9138

#define GL_MAX_COMBINED_TEXTURE_IMAGE_UNITS   2
#define GL_MAX_TEXTURE_LOD_BIAS               31
#define GL_MAX_VERTEX_ATTRIBS                 8
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
void glArrayElement(GLint i);
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
void glColorMask(GLboolean red, GLboolean green, GLboolean blue, GLboolean alpha);
void glColorPointer(GLint size, GLenum type, GLsizei stride, const GLvoid *pointer);
void glColorTable(GLenum target, GLenum internalformat, GLsizei width, GLenum format, GLenum type, const GLvoid *data);
void glCompileShader(GLuint shader);
void glCompressedTexImage2D(GLenum target, GLint level, GLenum internalformat, GLsizei width, GLsizei height, GLint border, GLsizei imageSize, const void *data); // Mipmap levels are ignored currently
GLuint glCreateProgram(void);
GLuint glCreateShader(GLenum shaderType);
void glCullFace(GLenum mode);
void glDeleteBuffers(GLsizei n, const GLuint *gl_buffers);
void glDeleteFramebuffers(GLsizei n, GLuint *framebuffers);
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
GLint glGetAttribLocation(GLuint prog, const GLchar *name);
void glGetBooleanv(GLenum pname, GLboolean *params);
void glGetFloatv(GLenum pname, GLfloat *data);
GLenum glGetError(void);
void glGetIntegerv(GLenum pname, GLint *data);
void glGetShaderInfoLog(GLuint handle, GLsizei maxLength, GLsizei *length, GLchar *infoLog);
void glGetShaderiv(GLuint handle, GLenum pname, GLint *params);
const GLubyte *glGetString(GLenum name);
GLint glGetUniformLocation(GLuint prog, const GLchar *name);
void glHint(GLenum target, GLenum mode);
GLboolean glIsEnabled(GLenum cap);
void glLineWidth(GLfloat width);
void glLinkProgram(GLuint progr);
void glLoadIdentity(void);
void glLoadMatrixf(const GLfloat *m);
void glMatrixMode(GLenum mode);
void glMultMatrixf(const GLfloat *m);
void glOrtho(GLdouble left, GLdouble right, GLdouble bottom, GLdouble top, GLdouble nearVal, GLdouble farVal);
void glPointSize(GLfloat size);
void glPolygonMode(GLenum face, GLenum mode);
void glPolygonOffset(GLfloat factor, GLfloat units);
void glPopMatrix(void);
void glPushMatrix(void);
void glReadPixels(GLint x, GLint y, GLsizei width, GLsizei height, GLenum format, GLenum type, GLvoid *data);
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
void glUniform2f(GLint location, GLfloat v0, GLfloat v1);
void glUniform2i(GLint location, GLint v0, GLint v1);
void glUniform2fv(GLint location, GLsizei count, const GLfloat *value);
void glUniform3f(GLint location, GLfloat v0, GLfloat v1, GLfloat v2);
void glUniform3fv(GLint location, GLsizei count, const GLfloat *value);
void glUniform4f(GLint location, GLfloat v0, GLfloat v1, GLfloat v2, GLfloat v3);
void glUniform4fv(GLint location, GLsizei count, const GLfloat *value);
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
	VGL_MEM_ALL = 0, // any memory type (used to monitor total heap usage)
	VGL_MEM_VRAM, // CDRAM
	VGL_MEM_RAM, // USER_RW RAM
	VGL_MEM_SLOW, // PHYCONT_USER_RW RAM
	VGL_MEM_EXTERNAL, // newlib mem
	VGL_MEM_TYPE_COUNT
} vglMemType;

// vgl*
void *vglAlloc(uint32_t size, vglMemType type);
void vglEnableRuntimeShaderCompiler(GLboolean usage);
void vglEnd(void);
void *vglForceAlloc(uint32_t size);
void vglFree(void *addr);
SceGxmTexture *vglGetGxmTexture(GLenum target);
void *vglGetTexDataPointer(GLenum target);
GLboolean vglHasRuntimeShaderCompiler(void);
void vglInit(void);
void vglInitExtended(int width, int height, int ram_threshold, SceGxmMultisampleMode msaa);
void vglInitWithCustomSizes(int width, int height, int ram_pool_size, int cdram_pool_size, int phycont_pool_size, SceGxmMultisampleMode msaa);
size_t vglMemFree(vglMemType type);
void vglSetParamBufferSize(uint32_t size);
void vglSetupRuntimeShaderCompiler(shark_opt opt_level, int32_t use_fastmath, int32_t use_fastprecision, int32_t use_fastint);
void vglSwapBuffers(void);
void vglStartRendering(void);
void vglStopRendering(GLboolean swap_buffers);
void vglTexImageDepthBuffer(GLenum target);
void vglUpdateCommonDialog(void);
void vglUseTripleBuffering(GLboolean usage);
void vglUseVram(GLboolean usage);
void vglUseVramForUSSE(GLboolean usage);
void vglUseExtraMem(GLboolean usage);
void vglWaitVblankStart(GLboolean enable);
void *vglGetProcAddress(const char *name);

// NEON optimized memcpy
void *memcpy_neon(void *destination, const void *source, size_t num);

#ifdef __cplusplus
}
#endif

#endif
