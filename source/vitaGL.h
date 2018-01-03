#ifndef _VITAGL_H_

#define _VITAGL_H_
#include <vitasdk.h>

#define GLfloat       float
#define GLint         int32_t
#define GLdouble      double
#define GLshort       int16_t
#define GLuint        uint32_t
#define GLsizei       int32_t
#define GLenum        uint16_t
#define GLubyte       uint8_t
#define GLvoid        void
#define GLbyte        int8_t
#define GLboolean     uint8_t

#define GL_FALSE 0
#define GL_TRUE  1

#define GL_MODELVIEW                      0x1700
#define GL_PROJECTION                     0x1701
#define GL_TEXTURE                        0x1702

#define GL_POINTS                         0x0000
#define GL_LINES                          0x0001
#define GL_LINE_LOOP                      0x0002
#define GL_LINE_STRIP                     0x0003
#define GL_TRIANGLES                      0x0004
#define GL_TRIANGLE_STRIP                 0x0005
#define GL_TRIANGLE_FAN                   0x0006
#define GL_QUADS                          0x0007

#define GL_NO_ERROR                       0x0000
#define GL_INVALID_ENUM                   0x0500
#define GL_INVALID_VALUE                  0x0501
#define GL_INVALID_OPERATION              0x0502
#define GL_STACK_OVERFLOW                 0x0503
#define GL_STACK_UNDERFLOW                0x0504
#define GL_OUT_OF_MEMORY                  0x0505

#define GL_TEXTURE0                       0x84C0
#define GL_TEXTURE1                       0x84C1
#define GL_TEXTURE2                       0x84C2
#define GL_TEXTURE3                       0x84C3
#define GL_TEXTURE4                       0x84C4
#define GL_TEXTURE5                       0x84C5
#define GL_TEXTURE6                       0x84C6
#define GL_TEXTURE7                       0x84C7
#define GL_TEXTURE8                       0x84C8
#define GL_TEXTURE9                       0x84C9
#define GL_TEXTURE10                      0x84CA
#define GL_TEXTURE11                      0x84CB
#define GL_TEXTURE12                      0x84CC
#define GL_TEXTURE13                      0x84CD
#define GL_TEXTURE14                      0x84CE
#define GL_TEXTURE15                      0x84CF
#define GL_TEXTURE16                      0x84D0
#define GL_TEXTURE17                      0x84D1
#define GL_TEXTURE18                      0x84D2
#define GL_TEXTURE19                      0x84D3
#define GL_TEXTURE20                      0x84D4
#define GL_TEXTURE21                      0x84D5
#define GL_TEXTURE22                      0x84D6
#define GL_TEXTURE23                      0x84D7
#define GL_TEXTURE24                      0x84D8
#define GL_TEXTURE25                      0x84D9
#define GL_TEXTURE26                      0x84DA
#define GL_TEXTURE27                      0x84DB
#define GL_TEXTURE28                      0x84DC
#define GL_TEXTURE29                      0x84DD
#define GL_TEXTURE30                      0x84DE
#define GL_TEXTURE31                      0x84DF

#define GL_RGB                            0x1907
#define GL_RGBA                           0x1908
#define GL_VITA2D_TEXTURE                 0x190B

#define GL_BYTE                           0x1400
#define GL_UNSIGNED_BYTE                  0x1401
#define GL_SHORT                          0x1402
#define GL_UNSIGNED_SHORT                 0x1403
#define GL_FLOAT                          0x1406
#define GL_FIXED                          0x140C
#define GL_UNSIGNED_SHORT_4_4_4_4         0x8033
#define GL_UNSIGNED_SHORT_5_5_5_1         0x8034
#define GL_UNSIGNED_SHORT_5_6_5           0x8363

#define GL_TEXTURE_2D                     0x0DE1

#define GL_NEAREST                        0x2600
#define GL_LINEAR                         0x2601
#define GL_NEAREST_MIPMAP_NEAREST         0x2700
#define GL_LINEAR_MIPMAP_NEAREST          0x2701
#define GL_NEAREST_MIPMAP_LINEAR          0x2702
#define GL_LINEAR_MIPMAP_LINEAR           0x2703

#define GL_TEXTURE_MAG_FILTER             0x2800
#define GL_TEXTURE_MIN_FILTER             0x2801

#define GL_NEVER                          0x0200
#define GL_LESS                           0x0201
#define GL_EQUAL                          0x0202
#define GL_LEQUAL                         0x0203
#define GL_GREATER                        0x0204
#define GL_NOTEQUAL                       0x0205
#define GL_GEQUAL                         0x0206
#define GL_ALWAYS                         0x0207

#define GL_CULL_FACE                      0x0B44
#define GL_DEPTH_TEST                     0x0B71
#define GL_STENCIL_TEST                   0x0B90
#define GL_BLEND                          0x0BE2
#define GL_SCISSOR_TEST                   0x0C11

#define GL_VERTEX_ARRAY                   0x8074
#define GL_COLOR_ARRAY                    0x8076
#define GL_TEXTURE_COORD_ARRAY            0x8078

#define GL_ZERO                           0
#define GL_ONE                            1
#define GL_SRC_COLOR                      0x0300
#define GL_ONE_MINUS_SRC_COLOR            0x0301
#define GL_SRC_ALPHA                      0x0302
#define GL_ONE_MINUS_SRC_ALPHA            0x0303
#define GL_DST_ALPHA                      0x0304
#define GL_ONE_MINUS_DST_ALPHA            0x0305
#define GL_DST_COLOR                      0x0306
#define GL_ONE_MINUS_DST_COLOR            0x0307

#define GL_KEEP                           0x1E00
#define GL_REPLACE                        0x1E01
#define GL_INCR                           0x1E02
#define GL_DECR                           0x1E03
#define GL_INVERT                         0x150A
#define GL_INCR_WRAP                      0x8507
#define GL_DECR_WRAP                      0x8508

#define GL_CW                             0x0900
#define GL_CCW                            0x0901

#define GL_FRONT                          0x0404
#define GL_BACK                           0x0405
#define GL_FRONT_AND_BACK                 0x0408

#define GL_MAX_COMBINED_TEXTURE_IMAGE_UNITS 31

typedef enum GLbitfield{
	GL_DEPTH_BUFFER_BIT   = 0x00000100,
	GL_STENCIL_BUFFER_BIT = 0x00000400,
	GL_COLOR_BUFFER_BIT   = 0x00004000
} GLbitfield;

GLenum glGetError(void);

void glClear(GLbitfield mask);
void glClearColor(GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha);

void glEnable(GLenum cap);
void glDisable(GLenum cap);

void glBegin(GLenum mode);
void glEnd(void);

void glGenTextures(GLsizei n, GLuint* textures);
void glBindTexture(GLenum target, GLuint texture);

void glTexImage2D(GLenum target, GLint level, GLint internalFormat, GLsizei width, GLsizei height, GLint border, GLenum format, GLenum type, const GLvoid * data);
void glTexParameteri(GLenum target, GLenum pname, GLint param);
void glTexCoord2i(GLint s, GLint t);
void glActiveTexture(GLenum texture);

void glVertex3f(GLfloat x, GLfloat y, GLfloat z);
void glArrayElement(GLint i);

void glMatrixMode(GLenum mode);
void glLoadIdentity(void);
void glViewport(GLint x, GLint y, GLsizei width, GLsizei height);

void glOrtho(GLdouble left, GLdouble right, GLdouble bottom, GLdouble top, GLdouble nearVal, GLdouble farVal);
void glFrustum(GLdouble left, GLdouble right, GLdouble bottom, GLdouble top, GLdouble nearVal, GLdouble farVal);

void glMultMatrixf(const GLfloat* m);
void glLoadMatrixf(const GLfloat* m);

void glTranslatef(GLfloat x, GLfloat y, GLfloat z);
void glScalef(GLfloat x, GLfloat y, GLfloat z);
void glRotatef(GLfloat angle,  GLfloat x,  GLfloat y,  GLfloat z);

void glColor3f(GLfloat red, GLfloat green, GLfloat blue);
void glColor3ub(GLubyte red, GLubyte green, GLubyte blue);
void glColor4f(GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha);
void glColor4ub(GLubyte red, GLubyte green, GLubyte blue, GLubyte alpha);

void glDepthFunc(GLenum func);
void glClearDepth(GLdouble depth);

void glBlendFunc(GLenum sfactor, GLenum dfactor);

void glStencilOp(GLenum sfail, GLenum dpfail, GLenum dppass);
void glStencilFunc(GLenum func, GLint ref, GLuint mask);

void glCullFace(GLenum mode);
void glFrontFace(GLenum mode);

void glScissor(GLint x,  GLint y,  GLsizei width,  GLsizei height);

GLboolean glIsEnabled(GLenum cap);

void glPushMatrix(void);
void glPopMatrix(void);

void glVertexPointer(GLint size, GLenum type, GLsizei stride, const GLvoid* pointer);
void glColorPointer(GLint size, GLenum type, GLsizei stride, const GLvoid* pointer);
void glTexCoordPointer(GLint size, GLenum type, GLsizei stride, const GLvoid* pointer);
void glDrawArrays(GLenum mode, GLint first, GLsizei count);

void glEnableClientState(GLenum array);
void glDisableClientState(GLenum array);
void glClientActiveTexture(GLenum texture);

void glFinish(void);

void vglInit(uint32_t gpu_pool_size);
void vglEnd(void);

#endif