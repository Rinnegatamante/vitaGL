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
#define GLbitfield    uint32_t
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
#define GLclampx      int32_t

#define EGLBoolean    int32_t
#define EGLDisplay    void*
#define EGLenum       uint32_t
#define EGLSurface    void*
#define EGLContext    void*
#define EGLConfig     void*
#define EGLint        int32_t

#define EGLint64      int64_t
#define EGLuint64     uint64_t

#define NativeDisplayType void*

#define GL_FALSE                              0
#define GL_TRUE                               1
#define EGL_FALSE                             0
#define EGL_TRUE                              1

#define GL_NO_ERROR                           0

#define GL_ZERO                               0
#define GL_ONE                                1

#define GL_NONE                               0

#define GL_POINTS                                       0X0000
#define GL_LINES                                        0X0001
#define GL_LINE_LOOP                                    0X0002
#define GL_LINE_STRIP                                   0X0003
#define GL_TRIANGLES                                    0X0004
#define GL_TRIANGLE_STRIP                               0X0005
#define GL_TRIANGLE_FAN                                 0X0006
#define GL_QUADS                                        0X0007
#define GL_ADD                                          0X0104
#define GL_NEVER                                        0X0200
#define GL_NEVER                                        0X0200
#define GL_LESS                                         0X0201
#define GL_EQUAL                                        0X0202
#define GL_LEQUAL                                       0X0203
#define GL_GREATER                                      0X0204
#define GL_NOTEQUAL                                     0X0205
#define GL_GEQUAL                                       0X0206
#define GL_ALWAYS                                       0X0207
#define GL_SRC_COLOR                                    0X0300
#define GL_ONE_MINUS_SRC_COLOR                          0X0301
#define GL_SRC_ALPHA                                    0X0302
#define GL_ONE_MINUS_SRC_ALPHA                          0X0303
#define GL_DST_ALPHA                                    0X0304
#define GL_ONE_MINUS_DST_ALPHA                          0X0305
#define GL_DST_COLOR                                    0X0306
#define GL_ONE_MINUS_DST_COLOR                          0X0307
#define GL_SRC_ALPHA_SATURATE                           0X0308
#define GL_FRONT                                        0X0404
#define GL_BACK                                         0X0405
#define GL_FRONT_AND_BACK                               0X0408
#define GL_INVALID_ENUM                                 0X0500
#define GL_INVALID_VALUE                                0X0501
#define GL_INVALID_OPERATION                            0X0502
#define GL_STACK_OVERFLOW                               0X0503
#define GL_STACK_UNDERFLOW                              0X0504
#define GL_OUT_OF_MEMORY                                0X0505
#define GL_EXP                                          0X0800
#define GL_EXP2                                         0X0801
#define GL_CW                                           0X0900
#define GL_CCW                                          0X0901
#define GL_POLYGON_MODE                                 0X0B40
#define GL_CULL_FACE                                    0X0B44
#define GL_LIGHTING                                     0X0B50
#define GL_LIGHT_MODEL_AMBIENT                          0X0B53
#define GL_COLOR_MATERIAL                               0X0B57
#define GL_FOG                                          0X0B60
#define GL_FOG_DENSITY                                  0X0B62
#define GL_FOG_START                                    0X0B63
#define GL_FOG_END                                      0X0B64
#define GL_FOG_MODE                                     0X0B65
#define GL_FOG_COLOR                                    0X0B66
#define GL_DEPTH_TEST                                   0X0B71
#define GL_DEPTH_WRITEMASK                              0X0B72
#define GL_STENCIL_TEST                                 0X0B90
#define GL_NORMALIZE                                    0X0BA1
#define GL_VIEWPORT                                     0X0BA2
#define GL_MODELVIEW_MATRIX                             0X0BA6
#define GL_PROJECTION_MATRIX                            0X0BA7
#define GL_TEXTURE_MATRIX                               0X0BA8
#define GL_ALPHA_TEST                                   0X0BC0
#define GL_BLEND                                        0X0BE2
#define GL_SCISSOR_BOX                                  0X0C10
#define GL_SCISSOR_TEST                                 0X0C11
#define GL_PACK_ALIGNMENT                               0X0D05
#define GL_ALPHA_SCALE                                  0X0D1C
#define GL_MAX_CLIP_PLANES                              0X0D32
#define GL_MAX_TEXTURE_SIZE                             0X0D33
#define GL_MAX_MODELVIEW_STACK_DEPTH                    0X0D36
#define GL_MAX_PROJECTION_STACK_DEPTH                   0X0D38
#define GL_MAX_TEXTURE_STACK_DEPTH                      0X0D39
#define GL_DEPTH_BITS                                   0X0D56
#define GL_STENCIL_BITS                                 0X0D57
#define GL_TEXTURE_2D                                   0X0DE1
#define GL_DONT_CARE                                    0X1100
#define GL_FASTEST                                      0X1101
#define GL_NICEST                                       0X1102
#define GL_AMBIENT                                      0X1200
#define GL_DIFFUSE                                      0X1201
#define GL_SPECULAR                                     0X1202
#define GL_POSITION                                     0X1203
#define GL_CONSTANT_ATTENUATION                         0X1207
#define GL_LINEAR_ATTENUATION                           0X1208
#define GL_QUADRATIC_ATTENUATION                        0X1209
#define GL_BYTE                                         0X1400
#define GL_UNSIGNED_BYTE                                0X1401
#define GL_SHORT                                        0X1402
#define GL_UNSIGNED_SHORT                               0X1403
#define GL_INT                                          0X1404
#define GL_UNSIGNED_INT                                 0X1405
#define GL_FLOAT                                        0X1406
#define GL_HALF_FLOAT                                   0X140B
#define GL_FIXED                                        0X140C
#define GL_INVERT                                       0X150A
#define GL_EMISSION                                     0X1600
#define GL_AMBIENT_AND_DIFFUSE                          0X1602
#define GL_MODELVIEW                                    0X1700
#define GL_PROJECTION                                   0X1701
#define GL_TEXTURE                                      0X1702
#define GL_COLOR_INDEX                                  0X1900
#define GL_DEPTH_COMPONENT                              0X1902
#define GL_RED                                          0X1903
#define GL_GREEN                                        0X1904
#define GL_BLUE                                         0X1905
#define GL_ALPHA                                        0X1906
#define GL_RGB                                          0X1907
#define GL_RGBA                                         0X1908
#define GL_LUMINANCE                                    0X1909
#define GL_LUMINANCE_ALPHA                              0X190A
#define GL_POINT                                        0X1B00
#define GL_LINE                                         0X1B01
#define GL_FILL                                         0X1B02
#define GL_FLAT                                         0X1D00
#define GL_SMOOTH                                       0X1D01
#define GL_KEEP                                         0X1E00
#define GL_REPLACE                                      0X1E01
#define GL_INCR                                         0X1E02
#define GL_DECR                                         0X1E03
#define GL_VENDOR                                       0X1F00
#define GL_RENDERER                                     0X1F01
#define GL_VERSION                                      0X1F02
#define GL_EXTENSIONS                                   0X1F03
#define GL_MODULATE                                     0X2100
#define GL_DECAL                                        0X2101
#define GL_TEXTURE_ENV_MODE                             0X2200
#define GL_TEXTURE_ENV_COLOR                            0X2201
#define GL_TEXTURE_ENV                                  0X2300
#define GL_NEAREST                                      0X2600
#define GL_LINEAR                                       0X2601
#define GL_NEAREST_MIPMAP_NEAREST                       0X2700
#define GL_LINEAR_MIPMAP_NEAREST                        0X2701
#define GL_NEAREST_MIPMAP_LINEAR                        0X2702
#define GL_LINEAR_MIPMAP_LINEAR                         0X2703
#define GL_TEXTURE_MAG_FILTER                           0X2800
#define GL_TEXTURE_MIN_FILTER                           0X2801
#define GL_TEXTURE_WRAP_S                               0X2802
#define GL_TEXTURE_WRAP_T                               0X2803
#define GL_CLAMP                                        0X2900
#define GL_REPEAT                                       0X2901
#define GL_POLYGON_OFFSET_UNITS                         0X2A00
#define GL_POLYGON_OFFSET_POINT                         0X2A01
#define GL_POLYGON_OFFSET_LINE                          0X2A02
#define GL_V2F                                          0X2A20
#define GL_V3F                                          0X2A21
#define GL_C4UB_V2F                                     0X2A22
#define GL_C4UB_V3F                                     0X2A23
#define GL_C3F_V3F                                      0X2A24
#define GL_T2F_V3F                                      0X2A27
#define GL_T4F_V4F                                      0X2A28
#define GL_T2F_C4UB_V3F                                 0X2A29
#define GL_T2F_C3F_V3F                                  0X2A2A
#define GL_CLIP_PLANE0                                  0X3000
#define GL_CLIP_PLANE1                                  0X3001
#define GL_CLIP_PLANE2                                  0X3002
#define GL_CLIP_PLANE3                                  0X3003
#define GL_CLIP_PLANE4                                  0X3004
#define GL_CLIP_PLANE5                                  0X3005
#define GL_CLIP_PLANE6                                  0X3006
#define GL_LIGHT0                                       0X4000
#define GL_LIGHT1                                       0X4001
#define GL_LIGHT2                                       0X4002
#define GL_LIGHT3                                       0X4003
#define GL_LIGHT4                                       0X4004
#define GL_LIGHT5                                       0X4005
#define GL_LIGHT6                                       0X4006
#define GL_LIGHT7                                       0X4007
#define GL_ABGR_EXT                                     0X8000
#define GL_FUNC_ADD                                     0X8006
#define GL_MIN                                          0X8007
#define GL_MAX                                          0X8008
#define GL_FUNC_SUBTRACT                                0X800A
#define GL_FUNC_REVERSE_SUBTRACT                        0X800B
#define GL_UNSIGNED_SHORT_4_4_4_4                       0X8033
#define GL_UNSIGNED_SHORT_5_5_5_1                       0X8034
#define GL_POLYGON_OFFSET_FILL                          0X8037
#define GL_POLYGON_OFFSET_FACTOR                        0X8038
#define GL_INTENSITY                                    0X8049
#define GL_TEXTURE_BINDING_2D                           0X8069
#define GL_VERTEX_ARRAY                                 0X8074
#define GL_NORMAL_ARRAY                                 0X8075
#define GL_COLOR_ARRAY                                  0X8076
#define GL_TEXTURE_COORD_ARRAY                          0X8078
#define GL_BLEND_DST_RGB                                0X80C8
#define GL_BLEND_SRC_RGB                                0X80C9
#define GL_BLEND_DST_ALPHA                              0X80CA
#define GL_BLEND_SRC_ALPHA                              0X80CB
#define GL_COLOR_TABLE                                  0X80D0
#define GL_BGR                                          0X80E0
#define GL_BGRA                                         0X80E1
#define GL_COLOR_INDEX8_EXT                             0X80E5
#define GL_PHONG_WIN                                    0X80EA
#define GL_CLAMP_TO_EDGE                                0X812F
#define GL_DEPTH_COMPONENT16                            0X81A5
#define GL_DEPTH_COMPONENT24                            0X81A6
#define GL_DEPTH_COMPONENT32                            0X81A7
#define GL_DEPTH_STENCIL_ATTACHMENT                     0X821A
#define GL_MAJOR_VERSION                                0X821B
#define GL_MINOR_VERSION                                0X821C
#define GL_NUM_EXTENSIONS                               0X821D
#define GL_RG                                           0X8227
#define GL_UNSIGNED_SHORT_5_6_5                         0X8363
#define GL_MIRRORED_REPEAT                              0X8370
#define GL_COMPRESSED_RGB_S3TC_DXT1_EXT                 0X83F0
#define GL_COMPRESSED_RGBA_S3TC_DXT1_EXT                0X83F1
#define GL_COMPRESSED_RGBA_S3TC_DXT3_EXT                0X83F2
#define GL_COMPRESSED_RGBA_S3TC_DXT5_EXT                0X83F3
#define GL_TEXTURE0                                     0X84C0
#define GL_TEXTURE1                                     0X84C1
#define GL_TEXTURE2                                     0X84C2
#define GL_TEXTURE3                                     0X84C3
#define GL_TEXTURE4                                     0X84C4
#define GL_TEXTURE5                                     0X84C5
#define GL_TEXTURE6                                     0X84C6
#define GL_TEXTURE7                                     0X84C7
#define GL_TEXTURE8                                     0X84C8
#define GL_TEXTURE9                                     0X84C9
#define GL_TEXTURE10                                    0X84CA
#define GL_TEXTURE11                                    0X84CB
#define GL_TEXTURE12                                    0X84CC
#define GL_TEXTURE13                                    0X84CD
#define GL_TEXTURE14                                    0X84CE
#define GL_TEXTURE15                                    0X84CF
#define GL_ACTIVE_TEXTURE                               0X84E0
#define GL_MAX_TEXTURE_UNITS                            0X84E2
#define GL_SUBTRACT                                     0X84E7
#define GL_TEXTURE_COMPRESSION_HINT                     0X84EF
#define GL_TEXTURE_MAX_ANISOTROPY_EXT                   0X84FE
#define GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT               0X84FF
#define GL_TEXTURE_LOD_BIAS                             0X8501
#define GL_INCR_WRAP                                    0X8507
#define GL_DECR_WRAP                                    0X8508
#define GL_TEXTURE_CUBE_MAP                             0X8513
#define GL_TEXTURE_CUBE_MAP_POSITIVE_X                  0X8515
#define GL_TEXTURE_CUBE_MAP_NEGATIVE_X                  0X8516
#define GL_TEXTURE_CUBE_MAP_POSITIVE_Y                  0X8517
#define GL_TEXTURE_CUBE_MAP_NEGATIVE_Y                  0X8518
#define GL_TEXTURE_CUBE_MAP_POSITIVE_Z                  0X8519
#define GL_TEXTURE_CUBE_MAP_NEGATIVE_Z                  0X851A
#define GL_COMBINE                                      0X8570
#define GL_COMBINE_RGB                                  0X8571
#define GL_COMBINE_ALPHA                                0X8572
#define GL_RGB_SCALE                                    0X8573
#define GL_ADD_SIGNED                                   0X8574
#define GL_INTERPOLATE                                  0X8575
#define GL_CONSTANT                                     0X8576
#define GL_PRIMARY_COLOR                                0X8577
#define GL_PREVIOUS                                     0X8578
#define GL_SRC0_RGB                                     0X8580
#define GL_SRC1_RGB                                     0X8581
#define GL_SRC2_RGB                                     0X8582
#define GL_SRC0_ALPHA                                   0X8588
#define GL_SRC1_ALPHA                                   0X8589
#define GL_SRC2_ALPHA                                   0X858A
#define GL_OPERAND0_RGB                                 0X8590
#define GL_OPERAND1_RGB                                 0X8591
#define GL_OPERAND2_RGB                                 0X8592
#define GL_OPERAND0_ALPHA                               0X8598
#define GL_OPERAND1_ALPHA                               0X8599
#define GL_OPERAND2_ALPHA                               0X859A
#define GL_VERTEX_ATTRIB_ARRAY_ENABLED                  0X8622
#define GL_VERTEX_ATTRIB_ARRAY_SIZE                     0X8623
#define GL_VERTEX_ATTRIB_ARRAY_STRIDE                   0X8624
#define GL_VERTEX_ATTRIB_ARRAY_TYPE                     0X8625
#define GL_CURRENT_VERTEX_ATTRIB                        0X8626
#define GL_VERTEX_ATTRIB_ARRAY_POINTER                  0X8645
#define GL_PROGRAM_ERROR_POSITION_ARB                   0X864B
#define GL_NUM_COMPRESSED_TEXTURE_FORMATS               0X86A2
#define GL_COMPRESSED_TEXTURE_FORMATS                   0X86A3
#define GL_MIRROR_CLAMP_EXT                             0X8742
#define GL_BUFFER_SIZE                                  0X8764
#define GL_ATC_RGBA_INTERPOLATED_ALPHA_AMD              0X87EE
#define GL_MAX_VERTEX_ATTRIBS                           0X8869
#define GL_VERTEX_ATTRIB_ARRAY_NORMALIZED               0X886A
#define GL_MAX_TEXTURE_COORDS                           0X8871
#define GL_MAX_TEXTURE_IMAGE_UNITS                      0X8872
#define GL_ARRAY_BUFFER                                 0X8892
#define GL_ELEMENT_ARRAY_BUFFER                         0X8893
#define GL_VERTEX_ATTRIB_ARRAY_BUFFER_BINDING           0X889F
#define GL_READ_ONLY                                    0X88B8
#define GL_WRITE_ONLY                                   0X88B9
#define GL_READ_WRITE                                   0X88BA
#define GL_STREAM_DRAW                                  0X88E0
#define GL_STREAM_READ                                  0X88E1
#define GL_STREAM_COPY                                  0X88E2
#define GL_STATIC_DRAW                                  0X88E4
#define GL_STATIC_READ                                  0X88E5
#define GL_STATIC_COPY                                  0X88E6
#define GL_DYNAMIC_DRAW                                 0X88E8
#define GL_DYNAMIC_READ                                 0X88E9
#define GL_DYNAMIC_COPY                                 0X88EA
#define GL_DEPTH24_STENCIL8                             0X88F0
#define GL_FRAGMENT_SHADER                              0X8B30
#define GL_VERTEX_SHADER                                0X8B31
#define GL_SHADER_TYPE                                  0X8B4F
#define GL_FLOAT_VEC2                                   0X8B50
#define GL_FLOAT_VEC3                                   0X8B51
#define GL_FLOAT_VEC4                                   0X8B52
#define GL_INT_VEC2                                     0X8B53
#define GL_INT_VEC3                                     0X8B54
#define GL_INT_VEC4                                     0X8B55
#define GL_FLOAT_MAT2                                   0X8B5A
#define GL_FLOAT_MAT3                                   0X8B5B
#define GL_FLOAT_MAT4                                   0X8B5C
#define GL_COMPILE_STATUS                               0X8B81
#define GL_LINK_STATUS                                  0X8B82
#define GL_INFO_LOG_LENGTH                              0X8B84
#define GL_ATTACHED_SHADERS                             0X8B85
#define GL_ACTIVE_UNIFORMS                              0X8B86
#define GL_ACTIVE_UNIFORM_MAX_LENGTH                    0X8B87
#define GL_SHADER_SOURCE_LENGTH                         0X8B88
#define GL_ACTIVE_ATTRIBUTES                            0X8B89
#define GL_ACTIVE_ATTRIBUTE_MAX_LENGTH                  0X8B8A
#define GL_SHADING_LANGUAGE_VERSION                     0X8B8C
#define GL_CURRENT_PROGRAM                              0x8B8D
#define GL_PALETTE4_RGB8_OES                            0X8B90
#define GL_PALETTE4_RGBA8_OES                           0X8B91
#define GL_PALETTE4_R5_G6_B5_OES                        0X8B92
#define GL_PALETTE4_RGBA4_OES                           0X8B93
#define GL_PALETTE4_RGB5_A1_OES                         0X8B94
#define GL_PALETTE8_RGB8_OES                            0X8B95
#define GL_PALETTE8_RGBA8_OES                           0X8B96
#define GL_PALETTE8_R5_G6_B5_OES                        0X8B97
#define GL_PALETTE8_RGBA4_OES                           0X8B98
#define GL_PALETTE8_RGB5_A1_OES                         0X8B99
#define GL_COMPRESSED_RGB_PVRTC_4BPPV1_IMG              0X8C00
#define GL_COMPRESSED_RGB_PVRTC_2BPPV1_IMG              0X8C01
#define GL_COMPRESSED_RGBA_PVRTC_4BPPV1_IMG             0X8C02
#define GL_COMPRESSED_RGBA_PVRTC_2BPPV1_IMG             0X8C03
#define GL_SRGB                                         0X8C40
#define GL_SRGB8                                        0X8C41
#define GL_SRGB_ALPHA                                   0X8C42
#define GL_SRGB8_ALPHA8                                 0X8C43
#define GL_SLUMINANCE_ALPHA                             0X8C44
#define GL_SLUMINANCE8_ALPHA8                           0X8C45
#define GL_SLUMINANCE                                   0X8C46
#define GL_SLUMINANCE8                                  0X8C47
#define GL_COMPRESSED_SRGB                              0X8C48
#define GL_COMPRESSED_SRGB_ALPHA                        0X8C49
#define GL_COMPRESSED_SRGB_S3TC_DXT1                    0X8C4C
#define GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT1              0X8C4D
#define GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT3              0X8C4E
#define GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT5              0X8C4F
#define GL_ATC_RGB_AMD                                  0X8C92
#define GL_ATC_RGBA_EXPLICIT_ALPHA_AMD                  0X8C93
#define GL_FRAMEBUFFER_BINDING                          0X8CA6
#define GL_READ_FRAMEBUFFER                             0X8CA8
#define GL_DRAW_FRAMEBUFFER                             0X8CA9
#define GL_READ_FRAMEBUFFER_BINDING                     0X8CAA
#define GL_COLOR_ATTACHMENT0                            0X8CE0
#define GL_FRAMEBUFFER_ATTACHMENT_OBJECT_TYPE           0X8CD0
#define GL_DEPTH_ATTACHMENT                             0X8D00
#define GL_DEPTH_COMPONENT32F                           0X8DAB
#define GL_DEPTH32F_STENCIL8                            0X8DAC
#define GL_FRAMEBUFFER_COMPLETE                         0X8CD5
#define GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT    0X8CD7
#define GL_FRAMEBUFFER                                  0X8D40
#define GL_RENDERBUFFER                                 0X8D41
#define GL_MAX_COMBINED_TEXTURE_IMAGE_UNITS             0X8B4D
#define GL_SHADER_BINARY_FORMATS                        0X8DF8
#define GL_NUM_SHADER_BINARY_FORMATS                    0X8DF9
#define GL_SHADER_COMPILER                              0X8DFA
#define GL_MAX_VERTEX_UNIFORM_VECTORS                   0X8DFB
#define GL_MAX_VARYING_VECTORS                          0X8DFC
#define GL_MAX_FRAGMENT_UNIFORM_VECTORS                 0X8DFD
#define GL_GPU_MEMORY_INFO_DEDICATED_VIDMEM_NVX         0x9047
#define GL_GPU_MEMORY_INFO_TOTAL_AVAILABLE_MEMORY_NVX   0x9048
#define GL_GPU_MEMORY_INFO_CURRENT_AVAILABLE_VIDMEM_NVX 0x9049
#define GL_COMPRESSED_RGBA_PVRTC_2BPPV2_IMG             0x9137
#define GL_COMPRESSED_RGBA_PVRTC_4BPPV2_IMG             0x9138

#define EGL_SUCCESS                                  0x3000
#define EGL_BAD_PARAMETER                            0x300C
#define EGL_OPENGL_ES_API                            0x30A0
#define EGL_OPENGL_API                               0x30A2

#define EGL_DEFAULT_DISPLAY ((NativeDisplayType)0)
#define EGL_NO_CONTEXT      ((EGLContext)0)
#define EGL_NO_DISPLAY      ((EGLDisplay)0)
#define EGL_NO_SURFACE      ((EGLSurface)0)

#define GL_MAX_TEXTURE_LOD_BIAS               31

#define GL_POINT_BIT          0x00000002
#define GL_LINE_BIT           0x00000004
#define GL_POLYGON_BIT        0x00000008
#define GL_LIGHTING_BIT       0x00000040
#define GL_FOG_BIT            0x00000080
#define GL_DEPTH_BUFFER_BIT   0x00000100
#define GL_STENCIL_BUFFER_BIT 0x00000400
#define GL_VIEWPORT_BIT       0x00000800
#define GL_TRANSFORM_BIT      0x00001000
#define GL_ENABLE_BIT         0x00002000
#define GL_COLOR_BUFFER_BIT   0x00004000
#define GL_HINT_BIT           0x00008000
#define GL_SCISSOR_BIT        0x00080000
#define GL_ALL_ATTRIB_BITS    0xFFFFFFFF

#define GL_MAP_READ_BIT                   0x0001
#define GL_MAP_WRITE_BIT                  0x0002
#define GL_MAP_INVALIDATE_RANGE_BIT       0x0004
#define GL_MAP_INVALIDATE_BUFFER_BIT      0x0008
#define GL_MAP_FLUSH_EXPLICIT_BIT         0x0010
#define GL_MAP_UNSYNCHRONIZED_BIT         0x0020

// Aliases
#define GL_DRAW_FRAMEBUFFER_BINDING GL_FRAMEBUFFER_BINDING

// clang-format on

// gl*
void glActiveTexture(GLenum texture);
void glAlphaFunc(GLenum func, GLfloat ref);
void glAttachShader(GLuint prog, GLuint shad);
void glBegin(GLenum mode);
void glBindAttribLocation(GLuint program, GLuint index, const GLchar *name);
void glBindBuffer(GLenum target, GLuint buffer);
void glBindFramebuffer(GLenum target, GLuint framebuffer);
void glBindRenderbuffer(GLenum target, GLuint renderbuffer);
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
void glClearColorx(GLclampx red, GLclampx green, GLclampx blue, GLclampx alpha);
void glClearDepth(GLdouble depth);
void glClearDepthf(GLclampf depth);
void glClearDepthx(GLclampx depth);
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
void glColorMaterial(GLenum face, GLenum mode);
void glColorPointer(GLint size, GLenum type, GLsizei stride, const GLvoid *pointer);
void glColorTable(GLenum target, GLenum internalformat, GLsizei width, GLenum format, GLenum type, const GLvoid *data);
void glCompileShader(GLuint shader);
void glCompressedTexImage2D(GLenum target, GLint level, GLenum internalformat, GLsizei width, GLsizei height, GLint border, GLsizei imageSize, const void *data); // Mipmap levels are ignored currently
GLuint glCreateProgram(void);
GLuint glCreateShader(GLenum shaderType);
void glCullFace(GLenum mode);
void glDeleteBuffers(GLsizei n, const GLuint *gl_buffers);
void glDeleteFramebuffers(GLsizei n, const GLuint *framebuffers);
void glDeleteProgram(GLuint prog);
void glDeleteRenderbuffers(GLsizei n, const GLuint *renderbuffers);
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
void glDrawElementsBaseVertex(GLenum mode, GLsizei count, GLenum type, const GLvoid *gl_indices, GLint baseVertex);
void glEnable(GLenum cap);
void glEnableClientState(GLenum array);
void glEnableVertexAttribArray(GLuint index);
void glEnd(void);
void glFinish(void);
void glFlush(void);
void glFlushMappedBufferRange(GLenum target, GLintptr offset, GLsizeiptr length);
void glFogf(GLenum pname, GLfloat param);
void glFogfv(GLenum pname, const GLfloat *params);
void glFogi(GLenum pname, const GLint param);
void glFramebufferRenderbuffer(GLenum target, GLenum attachment, GLenum renderbuffertarget, GLuint renderbuffer);
void glFramebufferTexture(GLenum target, GLenum attachment, GLuint texture, GLint level);
void glFramebufferTexture2D(GLenum target, GLenum attachment, GLenum textarget, GLuint texture, GLint level);
void glFrontFace(GLenum mode);
void glFrustum(GLdouble left, GLdouble right, GLdouble bottom, GLdouble top, GLdouble nearVal, GLdouble farVal);
void glFrustumf(GLfloat left, GLfloat right, GLfloat bottom, GLfloat top, GLfloat nearVal, GLfloat farVal);
void glFrustumx(GLfixed left, GLfixed right, GLfixed bottom, GLfixed top, GLfixed nearVal, GLfixed farVal);
void glGenBuffers(GLsizei n, GLuint *buffers);
void glGenerateMipmap(GLenum target);
void glGenFramebuffers(GLsizei n, GLuint *framebuffers);
void glGenRenderbuffers(GLsizei n, GLuint *renderbuffers);
void glGenTextures(GLsizei n, GLuint *textures);
void glGetActiveAttrib(GLuint prog, GLuint index, GLsizei bufSize, GLsizei *length, GLint *size, GLenum *type, GLchar *name);
void glGetActiveUniform(GLuint prog, GLuint index, GLsizei bufSize, GLsizei *length, GLint *size, GLenum *type, GLchar *name);
GLint glGetAttribLocation(GLuint prog, const GLchar *name);
void glGetBooleanv(GLenum pname, GLboolean *params);
void glGetBufferParameteriv(GLenum target, GLenum pname, GLint *params);
void glGetFloatv(GLenum pname, GLfloat *data);
void glGetFramebufferAttachmentParameteriv(GLenum target, GLenum attachment, GLenum pname, GLint *params);
GLenum glGetError(void);
void glGetIntegerv(GLenum pname, GLint *data);
void glGetProgramInfoLog(GLuint program, GLsizei maxLength, GLsizei *length, GLchar *infoLog);
void glGetProgramiv(GLuint program, GLenum pname, GLint *params);
void glGetShaderInfoLog(GLuint handle, GLsizei maxLength, GLsizei *length, GLchar *infoLog);
void glGetShaderiv(GLuint handle, GLenum pname, GLint *params);
void glGetShaderSource(GLuint handle, GLsizei bufSize, GLsizei *length, GLchar *source);
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
void glLightModelfv(GLenum pname, const GLfloat *params);
void glLightModelxv(GLenum pname, const GLfixed *params);
void glLightxv(GLenum light, GLenum pname, const GLfixed *params);
void glLineWidth(GLfloat width);
void glLinkProgram(GLuint progr);
void glLoadIdentity(void);
void glLoadMatrixf(const GLfloat *m);
void *glMapBuffer(GLenum target, GLbitfield access);
void *glMapBufferRange(GLenum target, GLintptr offset, GLsizeiptr length, GLbitfield access);
void glMaterialfv(GLenum face, GLenum pname, const GLfloat *params);
void glMaterialxv(GLenum face, GLenum pname, const GLfixed *params);
void glMatrixMode(GLenum mode);
void glMultiTexCoord2f(GLenum target, GLfloat s, GLfloat t);
void glMultiTexCoord2fv(GLenum target, GLfloat *f);
void glMultiTexCoord2i(GLenum target, GLint s, GLint t);
void glMultMatrixf(const GLfloat *m);
void glMultMatrixx(const GLfixed *m);
void glNormal3f(GLfloat x, GLfloat y, GLfloat z);
void glNormal3fv(const GLfloat *v);
void glNormalPointer(GLenum type, GLsizei stride, const void *pointer);
void glOrtho(GLdouble left, GLdouble right, GLdouble bottom, GLdouble top, GLdouble nearVal, GLdouble farVal);
void glOrthof(GLfloat left, GLfloat right, GLfloat bottom, GLfloat top, GLfloat nearVal, GLfloat farVal);
void glPointSize(GLfloat size);
void glPolygonMode(GLenum face, GLenum mode);
void glPolygonOffset(GLfloat factor, GLfloat units);
void glPopAttrib(void);
void glPopMatrix(void);
void glPushAttrib(GLbitfield mask);
void glPushMatrix(void);
void glReadPixels(GLint x, GLint y, GLsizei width, GLsizei height, GLenum format, GLenum type, GLvoid *data);
void glReleaseShaderCompiler(void);
void glRenderbufferStorage(GLenum target, GLenum internalformat, GLsizei width, GLsizei height);
void glRotatef(GLfloat angle, GLfloat x, GLfloat y, GLfloat z);
void glRotatex(GLfixed angle, GLfixed x, GLfixed y, GLfixed z);
void glScalef(GLfloat x, GLfloat y, GLfloat z);
void glScalex(GLfixed x, GLfixed y, GLfixed z);
void glScissor(GLint x, GLint y, GLsizei width, GLsizei height);
void glShadeModel(GLenum mode);
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
void glTexEnvx(GLenum target, GLenum pname, GLfixed param);
void glTexEnvxv(GLenum target, GLenum pname, GLfixed *param);
void glTexImage2D(GLenum target, GLint level, GLint internalFormat, GLsizei width, GLsizei height, GLint border, GLenum format, GLenum type, const GLvoid *data);
void glTexParameterf(GLenum target, GLenum pname, GLfloat param);
void glTexParameteri(GLenum target, GLenum pname, GLint param);
void glTexSubImage2D(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLenum type, const GLvoid *pixels);
void glTranslatef(GLfloat x, GLfloat y, GLfloat z);
void glTranslatex(GLfixed x, GLfixed y, GLfixed z);
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
GLboolean glUnmapBuffer(GLenum target);
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
void gluBuild2DMipmaps(GLenum target, GLint internalFormat, GLsizei width, GLsizei height, GLenum format, GLenum type, const void *data);
void gluLookAt(GLdouble eyeX, GLdouble eyeY, GLdouble eyeZ, GLdouble centerX, GLdouble centerY, GLdouble centerZ, GLdouble upX, GLdouble upY, GLdouble upZ);
void gluPerspective(GLdouble fovy, GLdouble aspect, GLdouble zNear, GLdouble zFar);

// egl*
EGLBoolean eglBindAPI(EGLenum api);
EGLDisplay eglGetDisplay(NativeDisplayType native_display);
EGLint eglGetError(void);
void (*eglGetProcAddress(char const *procname))(void);
EGLuint64 eglGetSystemTimeFrequencyNV(void);
EGLuint64 eglGetSystemTimeNV(void);
EGLenum eglQueryAPI(void);
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
void vglGetShaderBinary(GLuint index, GLsizei bufSize, GLsizei *length, void *binary);

typedef enum {
	VGL_MEM_VRAM, // CDRAM
	VGL_MEM_RAM, // USER_RW RAM
	VGL_MEM_SLOW, // PHYCONT_USER_RW RAM
	VGL_MEM_BUDGET, // CDLG RAM
	VGL_MEM_EXTERNAL, // newlib mem
	VGL_MEM_ALL
} vglMemType;

// vgl*
void *vglAlloc(uint32_t size, vglMemType type);
void *vglCalloc(uint32_t nmember, uint32_t size);
void vglEnableRuntimeShaderCompiler(GLboolean usage);
void vglEnd(void);
void *vglForceAlloc(uint32_t size);
void vglFree(void *addr);
SceGxmTexture *vglGetGxmTexture(GLenum target);
void *vglGetProcAddress(const char *name);
void *vglGetTexDataPointer(GLenum target);
GLboolean vglHasRuntimeShaderCompiler(void);
GLboolean vglInit(int legacy_pool_size);
GLboolean vglInitExtended(int legacy_pool_size, int width, int height, int ram_threshold, SceGxmMultisampleMode msaa);
GLboolean vglInitWithCustomSizes(int legacy_pool_size, int width, int height, int ram_pool_size, int cdram_pool_size, int phycont_pool_size, int cdlg_pool_size, SceGxmMultisampleMode msaa);
GLboolean vglInitWithCustomThreshold(int pool_size, int width, int height, int ram_threshold, int cdram_threshold, int phycont_threshold, int cdlg_threshold, SceGxmMultisampleMode msaa);
void *vglMalloc(uint32_t size);
void *vglMemalign(uint32_t alignment, uint32_t size);
size_t vglMemFree(vglMemType type);
void *vglRealloc(void *ptr, uint32_t size);
void vglSetFragmentBufferSize(uint32_t size);
void vglSetParamBufferSize(uint32_t size);
void vglSetUSSEBufferSize(uint32_t size);
void vglSetVDMBufferSize(uint32_t size);
void vglSetVertexBufferSize(uint32_t size);
void vglSetVertexPoolSize(uint32_t size);
void vglSetupDisplayQueue(uint32_t flags);
void vglSetupGarbageCollector(int priority, int affinity);
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
